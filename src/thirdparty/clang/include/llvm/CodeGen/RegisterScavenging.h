﻿//===-- RegisterScavenging.h - Machine register scavenging ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the machine register scavenger class. It can provide
// information such as unused register at any point in a machine basic block.
// It also provides a mechanism to make registers availbale by evicting them
// to spill slots.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CODEGEN_REGISTERSCAVENGING_H
#define LLVM_CODEGEN_REGISTERSCAVENGING_H

#include "llvm/ADT/BitVector.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"

namespace llvm {

class MachineRegisterInfo;
class TargetRegisterInfo;
class TargetInstrInfo;
class TargetRegisterClass;

class RegScavenger {
  const TargetRegisterInfo *TRI;
  const TargetInstrInfo *TII;
  MachineRegisterInfo* MRI;
  MachineBasicBlock *MBB;
  MachineBasicBlock::iterator MBBI;
  unsigned NumPhysRegs;

  /// Tracking - True if RegScavenger is currently tracking the liveness of 
  /// registers.
  bool Tracking;

  /// Information on scavenged registers (held in a spill slot).
  struct ScavengedInfo {
    ScavengedInfo(int FI = -1) : FrameIndex(FI), Reg(0), Restore(NULL) {}

    /// A spill slot used for scavenging a register post register allocation.
    int FrameIndex;

    /// If non-zero, the specific register is currently being
    /// scavenged. That is, it is spilled to this scavenging stack slot.
    unsigned Reg;

    /// The instruction that restores the scavenged register from stack.
    const MachineInstr *Restore;
  };

  /// A vector of information on scavenged registers.
  SmallVector<ScavengedInfo, 2> Scavenged;

  /// CalleeSavedrRegs - A bitvector of callee saved registers for the target.
  ///
  BitVector CalleeSavedRegs;

  /// RegsAvailable - The current state of all the physical registers immediately
  /// before MBBI. One bit per physical register. If bit is set that means it's
  /// available, unset means the register is currently being used.
  BitVector RegsAvailable;

  // These BitVectors are only used internally to forward(). They are members
  // to avoid frequent reallocations.
  BitVector KillRegs, DefRegs;

public:
  RegScavenger()
    : MBB(NULL), NumPhysRegs(0), Tracking(false) {}

  /// enterBasicBlock - Start tracking liveness from the begin of the specific
  /// basic block.
  void enterBasicBlock(MachineBasicBlock *mbb);

  /// initRegState - allow resetting register state info for multiple
  /// passes over/within the same function.
  void initRegState();

  /// forward - Move the internal MBB iterator and update register states.
  void forward();

  /// forward - Move the internal MBB iterator and update register states until
  /// it has processed the specific iterator.
  void forward(MachineBasicBlock::iterator I) {
    if (!Tracking && MBB->begin() != I) forward();
    while (MBBI != I) forward();
  }

  /// Invert the behavior of forward() on the current instruction (undo the
  /// changes to the available registers made by forward()).
  void unprocess();

  /// Unprocess instructions until you reach the provided iterator.
  void unprocess(MachineBasicBlock::iterator I) {
    while (MBBI != I) unprocess();
  }

  /// skipTo - Move the internal MBB iterator but do not update register states.
  void skipTo(MachineBasicBlock::iterator I) {
    if (I == MachineBasicBlock::iterator(NULL))
      Tracking = false;
    MBBI = I;
  }

  MachineBasicBlock::iterator getCurrentPosition() const {
    return MBBI;
  }

  /// getRegsUsed - return all registers currently in use in used.
  void getRegsUsed(BitVector &used, bool includeReserved);

  /// getRegsAvailable - Return all available registers in the register class
  /// in Mask.
  BitVector getRegsAvailable(const TargetRegisterClass *RC);

  /// FindUnusedReg - Find a unused register of the specified register class.
  /// Return 0 if none is found.
  unsigned FindUnusedReg(const TargetRegisterClass *RegClass) const;

  /// Add a scavenging frame index.
  void addScavengingFrameIndex(int FI) {
    Scavenged.push_back(ScavengedInfo(FI));
  }

  /// Query whether a frame index is a scavenging frame index.
  bool isScavengingFrameIndex(int FI) const {
    for (SmallVector<ScavengedInfo, 2>::const_iterator I = Scavenged.begin(),
         IE = Scavenged.end(); I != IE; ++I)
      if (I->FrameIndex == FI)
        return true;

    return false;
  }

  /// Get an array of scavenging frame indices.
  void getScavengingFrameIndices(SmallVectorImpl<int> &A) const {
    for (SmallVector<ScavengedInfo, 2>::const_iterator I = Scavenged.begin(),
         IE = Scavenged.end(); I != IE; ++I)
      if (I->FrameIndex >= 0)
        A.push_back(I->FrameIndex);
  }

  /// scavengeRegister - Make a register of the specific register class
  /// available and do the appropriate bookkeeping. SPAdj is the stack
  /// adjustment due to call frame, it's passed along to eliminateFrameIndex().
  /// Returns the scavenged register.
  unsigned scavengeRegister(const TargetRegisterClass *RegClass,
                            MachineBasicBlock::iterator I, int SPAdj);
  unsigned scavengeRegister(const TargetRegisterClass *RegClass, int SPAdj) {
    return scavengeRegister(RegClass, MBBI, SPAdj);
  }

  /// setUsed - Tell the scavenger a register is used.
  ///
  void setUsed(unsigned Reg);
private:
  /// isReserved - Returns true if a register is reserved. It is never "unused".
  bool isReserved(unsigned Reg) const { return MRI->isReserved(Reg); }

  /// isUsed - Test if a register is currently being used.  When called by the
  /// isAliasUsed function, we only check isReserved if this is the original
  /// register, not an alias register.
  ///
  bool isUsed(unsigned Reg, bool CheckReserved = true) const   {
    return !RegsAvailable.test(Reg) || (CheckReserved && isReserved(Reg));
  }

  /// isAliasUsed - Is Reg or an alias currently in use?
  bool isAliasUsed(unsigned Reg) const;

  /// setUsed / setUnused - Mark the state of one or a number of registers.
  ///
  void setUsed(BitVector &Regs) {
    RegsAvailable.reset(Regs);
  }
  void setUnused(BitVector &Regs) {
    RegsAvailable |= Regs;
  }

  /// Processes the current instruction and fill the KillRegs and DefRegs bit
  /// vectors.
  void determineKillsAndDefs();

  /// Add Reg and all its sub-registers to BV.
  void addRegWithSubRegs(BitVector &BV, unsigned Reg);

  /// findSurvivorReg - Return the candidate register that is unused for the
  /// longest after StartMI. UseMI is set to the instruction where the search
  /// stopped.
  ///
  /// No more than InstrLimit instructions are inspected.
  unsigned findSurvivorReg(MachineBasicBlock::iterator StartMI,
                           BitVector &Candidates,
                           unsigned InstrLimit,
                           MachineBasicBlock::iterator &UseMI);

};

} // End llvm namespace

#endif
