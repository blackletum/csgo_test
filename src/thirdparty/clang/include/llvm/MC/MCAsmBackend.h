﻿//===-- llvm/MC/MCAsmBack.h - MC Asm Backend --------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_MC_MCASMBACKEND_H
#define LLVM_MC_MCASMBACKEND_H

#include "llvm/MC/MCDirectives.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/Support/DataTypes.h"
#include "llvm/Support/ErrorHandling.h"

namespace llvm {
class MCAsmLayout;
class MCAssembler;
class MCELFObjectTargetWriter;
struct MCFixupKindInfo;
class MCFragment;
class MCInst;
class MCRelaxableFragment;
class MCObjectWriter;
class MCSection;
class MCValue;
class raw_ostream;

/// MCAsmBackend - Generic interface to target specific assembler backends.
class MCAsmBackend {
  MCAsmBackend(const MCAsmBackend &) LLVM_DELETED_FUNCTION;
  void operator=(const MCAsmBackend &) LLVM_DELETED_FUNCTION;
protected: // Can only create subclasses.
  MCAsmBackend();

  unsigned HasReliableSymbolDifference : 1;
  unsigned HasDataInCodeSupport : 1;

public:
  virtual ~MCAsmBackend();

  /// lifetime management
  virtual void reset() { }

  /// createObjectWriter - Create a new MCObjectWriter instance for use by the
  /// assembler backend to emit the final object file.
  virtual MCObjectWriter *createObjectWriter(raw_ostream &OS) const = 0;

  /// createELFObjectTargetWriter - Create a new ELFObjectTargetWriter to enable
  /// non-standard ELFObjectWriters.
  virtual  MCELFObjectTargetWriter *createELFObjectTargetWriter() const {
    llvm_unreachable("createELFObjectTargetWriter is not supported by asm "
                     "backend");
  }

  /// hasReliableSymbolDifference - Check whether this target implements
  /// accurate relocations for differences between symbols. If not, differences
  /// between symbols will always be relocatable expressions and any references
  /// to temporary symbols will be assumed to be in the same atom, unless they
  /// reside in a different section.
  ///
  /// This should always be true (since it results in fewer relocations with no
  /// loss of functionality), but is currently supported as a way to maintain
  /// exact object compatibility with Darwin 'as' (on non-x86_64). It should
  /// eventually should be eliminated.
  bool hasReliableSymbolDifference() const {
    return HasReliableSymbolDifference;
  }

  /// hasDataInCodeSupport - Check whether this target implements data-in-code
  /// markers. If not, data region directives will be ignored.
  bool hasDataInCodeSupport() const {
    return HasDataInCodeSupport;
  }

  /// doesSectionRequireSymbols - Check whether the given section requires that
  /// all symbols (even temporaries) have symbol table entries.
  virtual bool doesSectionRequireSymbols(const MCSection &Section) const {
    return false;
  }

  /// isSectionAtomizable - Check whether the given section can be split into
  /// atoms.
  ///
  /// \see MCAssembler::isSymbolLinkerVisible().
  virtual bool isSectionAtomizable(const MCSection &Section) const {
    return true;
  }

  /// @name Target Fixup Interfaces
  /// @{

  /// getNumFixupKinds - Get the number of target specific fixup kinds.
  virtual unsigned getNumFixupKinds() const = 0;

  /// getFixupKindInfo - Get information on a fixup kind.
  virtual const MCFixupKindInfo &getFixupKindInfo(MCFixupKind Kind) const;

  /// processFixupValue - Target hook to adjust the literal value of a fixup
  /// if necessary. IsResolved signals whether the caller believes a relocation
  /// is needed; the target can modify the value. The default does nothing.
  virtual void processFixupValue(const MCAssembler &Asm,
                                 const MCAsmLayout &Layout,
                                 const MCFixup &Fixup, const MCFragment *DF,
                                 MCValue &Target, uint64_t &Value,
                                 bool &IsResolved) {}

  /// @}

  /// applyFixup - Apply the \p Value for given \p Fixup into the provided
  /// data fragment, at the offset specified by the fixup and following the
  /// fixup kind as appropriate.
  virtual void applyFixup(const MCFixup &Fixup, char *Data, unsigned DataSize,
                          uint64_t Value) const = 0;

  /// @}

  /// @name Target Relaxation Interfaces
  /// @{

  /// mayNeedRelaxation - Check whether the given instruction may need
  /// relaxation.
  ///
  /// \param Inst - The instruction to test.
  virtual bool mayNeedRelaxation(const MCInst &Inst) const = 0;

  /// fixupNeedsRelaxation - Target specific predicate for whether a given
  /// fixup requires the associated instruction to be relaxed.
  virtual bool fixupNeedsRelaxation(const MCFixup &Fixup,
                                    uint64_t Value,
                                    const MCRelaxableFragment *DF,
                                    const MCAsmLayout &Layout) const = 0;

  /// RelaxInstruction - Relax the instruction in the given fragment to the next
  /// wider instruction.
  ///
  /// \param Inst The instruction to relax, which may be the same as the
  /// output.
  /// \param [out] Res On return, the relaxed instruction.
  virtual void relaxInstruction(const MCInst &Inst, MCInst &Res) const = 0;

  /// @}

  /// getMinimumNopSize - Returns the minimum size of a nop in bytes on this
  /// target. The assembler will use this to emit excess padding in situations
  /// where the padding required for simple alignment would be less than the
  /// minimum nop size.
  ///
  virtual unsigned getMinimumNopSize() const { return 1; }

  /// writeNopData - Write an (optimal) nop sequence of Count bytes to the given
  /// output. If the target cannot generate such a sequence, it should return an
  /// error.
  ///
  /// \return - True on success.
  virtual bool writeNopData(uint64_t Count, MCObjectWriter *OW) const = 0;

  /// handleAssemblerFlag - Handle any target-specific assembler flags.
  /// By default, do nothing.
  virtual void handleAssemblerFlag(MCAssemblerFlag Flag) {}
};

} // End llvm namespace

#endif
