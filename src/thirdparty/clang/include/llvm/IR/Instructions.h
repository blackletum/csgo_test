﻿//===-- llvm/Instructions.h - Instruction subclass definitions --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file exposes the class definitions of all of the subclasses of the
// Instruction class.  This is meant to be an easy way to get access to all
// instruction subclasses.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_IR_INSTRUCTIONS_H
#define LLVM_IR_INSTRUCTIONS_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/IntegersSubset.h"
#include "llvm/Support/IntegersSubsetMapping.h"
#include <iterator>

namespace llvm {

class APInt;
class ConstantInt;
class ConstantRange;
class DataLayout;
class LLVMContext;

enum AtomicOrdering {
  NotAtomic = 0,
  Unordered = 1,
  Monotonic = 2,
  // Consume = 3,  // Not specified yet.
  Acquire = 4,
  Release = 5,
  AcquireRelease = 6,
  SequentiallyConsistent = 7
};

enum SynchronizationScope {
  SingleThread = 0,
  CrossThread = 1
};

//===----------------------------------------------------------------------===//
//                                AllocaInst Class
//===----------------------------------------------------------------------===//

/// AllocaInst - an instruction to allocate memory on the stack
///
class AllocaInst : public UnaryInstruction {
protected:
  virtual AllocaInst *clone_impl() const;
public:
  explicit AllocaInst(Type *Ty, Value *ArraySize = 0,
                      const Twine &Name = "", Instruction *InsertBefore = 0);
  AllocaInst(Type *Ty, Value *ArraySize,
             const Twine &Name, BasicBlock *InsertAtEnd);

  AllocaInst(Type *Ty, const Twine &Name, Instruction *InsertBefore = 0);
  AllocaInst(Type *Ty, const Twine &Name, BasicBlock *InsertAtEnd);

  AllocaInst(Type *Ty, Value *ArraySize, unsigned Align,
             const Twine &Name = "", Instruction *InsertBefore = 0);
  AllocaInst(Type *Ty, Value *ArraySize, unsigned Align,
             const Twine &Name, BasicBlock *InsertAtEnd);

  // Out of line virtual method, so the vtable, etc. has a home.
  virtual ~AllocaInst();

  /// isArrayAllocation - Return true if there is an allocation size parameter
  /// to the allocation instruction that is not 1.
  ///
  bool isArrayAllocation() const;

  /// getArraySize - Get the number of elements allocated. For a simple
  /// allocation of a single element, this will return a constant 1 value.
  ///
  const Value *getArraySize() const { return getOperand(0); }
  Value *getArraySize() { return getOperand(0); }

  /// getType - Overload to return most specific pointer type
  ///
  PointerType *getType() const {
    return cast<PointerType>(Instruction::getType());
  }

  /// getAllocatedType - Return the type that is being allocated by the
  /// instruction.
  ///
  Type *getAllocatedType() const;

  /// getAlignment - Return the alignment of the memory that is being allocated
  /// by the instruction.
  ///
  unsigned getAlignment() const {
    return (1u << getSubclassDataFromInstruction()) >> 1;
  }
  void setAlignment(unsigned Align);

  /// isStaticAlloca - Return true if this alloca is in the entry block of the
  /// function and is a constant size.  If so, the code generator will fold it
  /// into the prolog/epilog code, so it is basically free.
  bool isStaticAlloca() const;

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return (I->getOpcode() == Instruction::Alloca);
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
private:
  // Shadow Instruction::setInstructionSubclassData with a private forwarding
  // method so that subclasses cannot accidentally use it.
  void setInstructionSubclassData(unsigned short D) {
    Instruction::setInstructionSubclassData(D);
  }
};


//===----------------------------------------------------------------------===//
//                                LoadInst Class
//===----------------------------------------------------------------------===//

/// LoadInst - an instruction for reading from memory.  This uses the
/// SubclassData field in Value to store whether or not the load is volatile.
///
class LoadInst : public UnaryInstruction {
  void AssertOK();
protected:
  virtual LoadInst *clone_impl() const;
public:
  LoadInst(Value *Ptr, const Twine &NameStr, Instruction *InsertBefore);
  LoadInst(Value *Ptr, const Twine &NameStr, BasicBlock *InsertAtEnd);
  LoadInst(Value *Ptr, const Twine &NameStr, bool isVolatile = false,
           Instruction *InsertBefore = 0);
  LoadInst(Value *Ptr, const Twine &NameStr, bool isVolatile,
           BasicBlock *InsertAtEnd);
  LoadInst(Value *Ptr, const Twine &NameStr, bool isVolatile,
           unsigned Align, Instruction *InsertBefore = 0);
  LoadInst(Value *Ptr, const Twine &NameStr, bool isVolatile,
           unsigned Align, BasicBlock *InsertAtEnd);
  LoadInst(Value *Ptr, const Twine &NameStr, bool isVolatile,
           unsigned Align, AtomicOrdering Order,
           SynchronizationScope SynchScope = CrossThread,
           Instruction *InsertBefore = 0);
  LoadInst(Value *Ptr, const Twine &NameStr, bool isVolatile,
           unsigned Align, AtomicOrdering Order,
           SynchronizationScope SynchScope,
           BasicBlock *InsertAtEnd);

  LoadInst(Value *Ptr, const char *NameStr, Instruction *InsertBefore);
  LoadInst(Value *Ptr, const char *NameStr, BasicBlock *InsertAtEnd);
  explicit LoadInst(Value *Ptr, const char *NameStr = 0,
                    bool isVolatile = false,  Instruction *InsertBefore = 0);
  LoadInst(Value *Ptr, const char *NameStr, bool isVolatile,
           BasicBlock *InsertAtEnd);

  /// isVolatile - Return true if this is a load from a volatile memory
  /// location.
  ///
  bool isVolatile() const { return getSubclassDataFromInstruction() & 1; }

  /// setVolatile - Specify whether this is a volatile load or not.
  ///
  void setVolatile(bool V) {
    setInstructionSubclassData((getSubclassDataFromInstruction() & ~1) |
                               (V ? 1 : 0));
  }

  /// getAlignment - Return the alignment of the access that is being performed
  ///
  unsigned getAlignment() const {
    return (1 << ((getSubclassDataFromInstruction() >> 1) & 31)) >> 1;
  }

  void setAlignment(unsigned Align);

  /// Returns the ordering effect of this fence.
  AtomicOrdering getOrdering() const {
    return AtomicOrdering((getSubclassDataFromInstruction() >> 7) & 7);
  }

  /// Set the ordering constraint on this load. May not be Release or
  /// AcquireRelease.
  void setOrdering(AtomicOrdering Ordering) {
    setInstructionSubclassData((getSubclassDataFromInstruction() & ~(7 << 7)) |
                               (Ordering << 7));
  }

  SynchronizationScope getSynchScope() const {
    return SynchronizationScope((getSubclassDataFromInstruction() >> 6) & 1);
  }

  /// Specify whether this load is ordered with respect to all
  /// concurrently executing threads, or only with respect to signal handlers
  /// executing in the same thread.
  void setSynchScope(SynchronizationScope xthread) {
    setInstructionSubclassData((getSubclassDataFromInstruction() & ~(1 << 6)) |
                               (xthread << 6));
  }

  bool isAtomic() const { return getOrdering() != NotAtomic; }
  void setAtomic(AtomicOrdering Ordering,
                 SynchronizationScope SynchScope = CrossThread) {
    setOrdering(Ordering);
    setSynchScope(SynchScope);
  }

  bool isSimple() const { return !isAtomic() && !isVolatile(); }
  bool isUnordered() const {
    return getOrdering() <= Unordered && !isVolatile();
  }

  Value *getPointerOperand() { return getOperand(0); }
  const Value *getPointerOperand() const { return getOperand(0); }
  static unsigned getPointerOperandIndex() { return 0U; }

  /// \brief Returns the address space of the pointer operand.
  unsigned getPointerAddressSpace() const {
    return getPointerOperand()->getType()->getPointerAddressSpace();
  }


  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == Instruction::Load;
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
private:
  // Shadow Instruction::setInstructionSubclassData with a private forwarding
  // method so that subclasses cannot accidentally use it.
  void setInstructionSubclassData(unsigned short D) {
    Instruction::setInstructionSubclassData(D);
  }
};


//===----------------------------------------------------------------------===//
//                                StoreInst Class
//===----------------------------------------------------------------------===//

/// StoreInst - an instruction for storing to memory
///
class StoreInst : public Instruction {
  void *operator new(size_t, unsigned) LLVM_DELETED_FUNCTION;
  void AssertOK();
protected:
  virtual StoreInst *clone_impl() const;
public:
  // allocate space for exactly two operands
  void *operator new(size_t s) {
    return User::operator new(s, 2);
  }
  StoreInst(Value *Val, Value *Ptr, Instruction *InsertBefore);
  StoreInst(Value *Val, Value *Ptr, BasicBlock *InsertAtEnd);
  StoreInst(Value *Val, Value *Ptr, bool isVolatile = false,
            Instruction *InsertBefore = 0);
  StoreInst(Value *Val, Value *Ptr, bool isVolatile, BasicBlock *InsertAtEnd);
  StoreInst(Value *Val, Value *Ptr, bool isVolatile,
            unsigned Align, Instruction *InsertBefore = 0);
  StoreInst(Value *Val, Value *Ptr, bool isVolatile,
            unsigned Align, BasicBlock *InsertAtEnd);
  StoreInst(Value *Val, Value *Ptr, bool isVolatile,
            unsigned Align, AtomicOrdering Order,
            SynchronizationScope SynchScope = CrossThread,
            Instruction *InsertBefore = 0);
  StoreInst(Value *Val, Value *Ptr, bool isVolatile,
            unsigned Align, AtomicOrdering Order,
            SynchronizationScope SynchScope,
            BasicBlock *InsertAtEnd);


  /// isVolatile - Return true if this is a store to a volatile memory
  /// location.
  ///
  bool isVolatile() const { return getSubclassDataFromInstruction() & 1; }

  /// setVolatile - Specify whether this is a volatile store or not.
  ///
  void setVolatile(bool V) {
    setInstructionSubclassData((getSubclassDataFromInstruction() & ~1) |
                               (V ? 1 : 0));
  }

  /// Transparently provide more efficient getOperand methods.
  DECLARE_TRANSPARENT_OPERAND_ACCESSORS(Value);

  /// getAlignment - Return the alignment of the access that is being performed
  ///
  unsigned getAlignment() const {
    return (1 << ((getSubclassDataFromInstruction() >> 1) & 31)) >> 1;
  }

  void setAlignment(unsigned Align);

  /// Returns the ordering effect of this store.
  AtomicOrdering getOrdering() const {
    return AtomicOrdering((getSubclassDataFromInstruction() >> 7) & 7);
  }

  /// Set the ordering constraint on this store.  May not be Acquire or
  /// AcquireRelease.
  void setOrdering(AtomicOrdering Ordering) {
    setInstructionSubclassData((getSubclassDataFromInstruction() & ~(7 << 7)) |
                               (Ordering << 7));
  }

  SynchronizationScope getSynchScope() const {
    return SynchronizationScope((getSubclassDataFromInstruction() >> 6) & 1);
  }

  /// Specify whether this store instruction is ordered with respect to all
  /// concurrently executing threads, or only with respect to signal handlers
  /// executing in the same thread.
  void setSynchScope(SynchronizationScope xthread) {
    setInstructionSubclassData((getSubclassDataFromInstruction() & ~(1 << 6)) |
                               (xthread << 6));
  }

  bool isAtomic() const { return getOrdering() != NotAtomic; }
  void setAtomic(AtomicOrdering Ordering,
                 SynchronizationScope SynchScope = CrossThread) {
    setOrdering(Ordering);
    setSynchScope(SynchScope);
  }

  bool isSimple() const { return !isAtomic() && !isVolatile(); }
  bool isUnordered() const {
    return getOrdering() <= Unordered && !isVolatile();
  }

  Value *getValueOperand() { return getOperand(0); }
  const Value *getValueOperand() const { return getOperand(0); }

  Value *getPointerOperand() { return getOperand(1); }
  const Value *getPointerOperand() const { return getOperand(1); }
  static unsigned getPointerOperandIndex() { return 1U; }

  /// \brief Returns the address space of the pointer operand.
  unsigned getPointerAddressSpace() const {
    return getPointerOperand()->getType()->getPointerAddressSpace();
  }

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == Instruction::Store;
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
private:
  // Shadow Instruction::setInstructionSubclassData with a private forwarding
  // method so that subclasses cannot accidentally use it.
  void setInstructionSubclassData(unsigned short D) {
    Instruction::setInstructionSubclassData(D);
  }
};

template <>
struct OperandTraits<StoreInst> : public FixedNumOperandTraits<StoreInst, 2> {
};

DEFINE_TRANSPARENT_OPERAND_ACCESSORS(StoreInst, Value)

//===----------------------------------------------------------------------===//
//                                FenceInst Class
//===----------------------------------------------------------------------===//

/// FenceInst - an instruction for ordering other memory operations
///
class FenceInst : public Instruction {
  void *operator new(size_t, unsigned) LLVM_DELETED_FUNCTION;
  void Init(AtomicOrdering Ordering, SynchronizationScope SynchScope);
protected:
  virtual FenceInst *clone_impl() const;
public:
  // allocate space for exactly zero operands
  void *operator new(size_t s) {
    return User::operator new(s, 0);
  }

  // Ordering may only be Acquire, Release, AcquireRelease, or
  // SequentiallyConsistent.
  FenceInst(LLVMContext &C, AtomicOrdering Ordering,
            SynchronizationScope SynchScope = CrossThread,
            Instruction *InsertBefore = 0);
  FenceInst(LLVMContext &C, AtomicOrdering Ordering,
            SynchronizationScope SynchScope,
            BasicBlock *InsertAtEnd);

  /// Returns the ordering effect of this fence.
  AtomicOrdering getOrdering() const {
    return AtomicOrdering(getSubclassDataFromInstruction() >> 1);
  }

  /// Set the ordering constraint on this fence.  May only be Acquire, Release,
  /// AcquireRelease, or SequentiallyConsistent.
  void setOrdering(AtomicOrdering Ordering) {
    setInstructionSubclassData((getSubclassDataFromInstruction() & 1) |
                               (Ordering << 1));
  }

  SynchronizationScope getSynchScope() const {
    return SynchronizationScope(getSubclassDataFromInstruction() & 1);
  }

  /// Specify whether this fence orders other operations with respect to all
  /// concurrently executing threads, or only with respect to signal handlers
  /// executing in the same thread.
  void setSynchScope(SynchronizationScope xthread) {
    setInstructionSubclassData((getSubclassDataFromInstruction() & ~1) |
                               xthread);
  }

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == Instruction::Fence;
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
private:
  // Shadow Instruction::setInstructionSubclassData with a private forwarding
  // method so that subclasses cannot accidentally use it.
  void setInstructionSubclassData(unsigned short D) {
    Instruction::setInstructionSubclassData(D);
  }
};

//===----------------------------------------------------------------------===//
//                                AtomicCmpXchgInst Class
//===----------------------------------------------------------------------===//

/// AtomicCmpXchgInst - an instruction that atomically checks whether a
/// specified value is in a memory location, and, if it is, stores a new value
/// there.  Returns the value that was loaded.
///
class AtomicCmpXchgInst : public Instruction {
  void *operator new(size_t, unsigned) LLVM_DELETED_FUNCTION;
  void Init(Value *Ptr, Value *Cmp, Value *NewVal,
            AtomicOrdering Ordering, SynchronizationScope SynchScope);
protected:
  virtual AtomicCmpXchgInst *clone_impl() const;
public:
  // allocate space for exactly three operands
  void *operator new(size_t s) {
    return User::operator new(s, 3);
  }
  AtomicCmpXchgInst(Value *Ptr, Value *Cmp, Value *NewVal,
                    AtomicOrdering Ordering, SynchronizationScope SynchScope,
                    Instruction *InsertBefore = 0);
  AtomicCmpXchgInst(Value *Ptr, Value *Cmp, Value *NewVal,
                    AtomicOrdering Ordering, SynchronizationScope SynchScope,
                    BasicBlock *InsertAtEnd);

  /// isVolatile - Return true if this is a cmpxchg from a volatile memory
  /// location.
  ///
  bool isVolatile() const {
    return getSubclassDataFromInstruction() & 1;
  }

  /// setVolatile - Specify whether this is a volatile cmpxchg.
  ///
  void setVolatile(bool V) {
     setInstructionSubclassData((getSubclassDataFromInstruction() & ~1) |
                                (unsigned)V);
  }

  /// Transparently provide more efficient getOperand methods.
  DECLARE_TRANSPARENT_OPERAND_ACCESSORS(Value);

  /// Set the ordering constraint on this cmpxchg.
  void setOrdering(AtomicOrdering Ordering) {
    assert(Ordering != NotAtomic &&
           "CmpXchg instructions can only be atomic.");
    setInstructionSubclassData((getSubclassDataFromInstruction() & 3) |
                               (Ordering << 2));
  }

  /// Specify whether this cmpxchg is atomic and orders other operations with
  /// respect to all concurrently executing threads, or only with respect to
  /// signal handlers executing in the same thread.
  void setSynchScope(SynchronizationScope SynchScope) {
    setInstructionSubclassData((getSubclassDataFromInstruction() & ~2) |
                               (SynchScope << 1));
  }

  /// Returns the ordering constraint on this cmpxchg.
  AtomicOrdering getOrdering() const {
    return AtomicOrdering(getSubclassDataFromInstruction() >> 2);
  }

  /// Returns whether this cmpxchg is atomic between threads or only within a
  /// single thread.
  SynchronizationScope getSynchScope() const {
    return SynchronizationScope((getSubclassDataFromInstruction() & 2) >> 1);
  }

  Value *getPointerOperand() { return getOperand(0); }
  const Value *getPointerOperand() const { return getOperand(0); }
  static unsigned getPointerOperandIndex() { return 0U; }

  Value *getCompareOperand() { return getOperand(1); }
  const Value *getCompareOperand() const { return getOperand(1); }

  Value *getNewValOperand() { return getOperand(2); }
  const Value *getNewValOperand() const { return getOperand(2); }

  /// \brief Returns the address space of the pointer operand.
  unsigned getPointerAddressSpace() const {
    return getPointerOperand()->getType()->getPointerAddressSpace();
  }

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == Instruction::AtomicCmpXchg;
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
private:
  // Shadow Instruction::setInstructionSubclassData with a private forwarding
  // method so that subclasses cannot accidentally use it.
  void setInstructionSubclassData(unsigned short D) {
    Instruction::setInstructionSubclassData(D);
  }
};

template <>
struct OperandTraits<AtomicCmpXchgInst> :
    public FixedNumOperandTraits<AtomicCmpXchgInst, 3> {
};

DEFINE_TRANSPARENT_OPERAND_ACCESSORS(AtomicCmpXchgInst, Value)

//===----------------------------------------------------------------------===//
//                                AtomicRMWInst Class
//===----------------------------------------------------------------------===//

/// AtomicRMWInst - an instruction that atomically reads a memory location,
/// combines it with another value, and then stores the result back.  Returns
/// the old value.
///
class AtomicRMWInst : public Instruction {
  void *operator new(size_t, unsigned) LLVM_DELETED_FUNCTION;
protected:
  virtual AtomicRMWInst *clone_impl() const;
public:
  /// This enumeration lists the possible modifications atomicrmw can make.  In
  /// the descriptions, 'p' is the pointer to the instruction's memory location,
  /// 'old' is the initial value of *p, and 'v' is the other value passed to the
  /// instruction.  These instructions always return 'old'.
  enum BinOp {
    /// *p = v
    Xchg,
    /// *p = old + v
    Add,
    /// *p = old - v
    Sub,
    /// *p = old & v
    And,
    /// *p = ~old & v
    Nand,
    /// *p = old | v
    Or,
    /// *p = old ^ v
    Xor,
    /// *p = old >signed v ? old : v
    Max,
    /// *p = old <signed v ? old : v
    Min,
    /// *p = old >unsigned v ? old : v
    UMax,
    /// *p = old <unsigned v ? old : v
    UMin,

    FIRST_BINOP = Xchg,
    LAST_BINOP = UMin,
    BAD_BINOP
  };

  // allocate space for exactly two operands
  void *operator new(size_t s) {
    return User::operator new(s, 2);
  }
  AtomicRMWInst(BinOp Operation, Value *Ptr, Value *Val,
                AtomicOrdering Ordering, SynchronizationScope SynchScope,
                Instruction *InsertBefore = 0);
  AtomicRMWInst(BinOp Operation, Value *Ptr, Value *Val,
                AtomicOrdering Ordering, SynchronizationScope SynchScope,
                BasicBlock *InsertAtEnd);

  BinOp getOperation() const {
    return static_cast<BinOp>(getSubclassDataFromInstruction() >> 5);
  }

  void setOperation(BinOp Operation) {
    unsigned short SubclassData = getSubclassDataFromInstruction();
    setInstructionSubclassData((SubclassData & 31) |
                               (Operation << 5));
  }

  /// isVolatile - Return true if this is a RMW on a volatile memory location.
  ///
  bool isVolatile() const {
    return getSubclassDataFromInstruction() & 1;
  }

  /// setVolatile - Specify whether this is a volatile RMW or not.
  ///
  void setVolatile(bool V) {
     setInstructionSubclassData((getSubclassDataFromInstruction() & ~1) |
                                (unsigned)V);
  }

  /// Transparently provide more efficient getOperand methods.
  DECLARE_TRANSPARENT_OPERAND_ACCESSORS(Value);

  /// Set the ordering constraint on this RMW.
  void setOrdering(AtomicOrdering Ordering) {
    assert(Ordering != NotAtomic &&
           "atomicrmw instructions can only be atomic.");
    setInstructionSubclassData((getSubclassDataFromInstruction() & ~(7 << 2)) |
                               (Ordering << 2));
  }

  /// Specify whether this RMW orders other operations with respect to all
  /// concurrently executing threads, or only with respect to signal handlers
  /// executing in the same thread.
  void setSynchScope(SynchronizationScope SynchScope) {
    setInstructionSubclassData((getSubclassDataFromInstruction() & ~2) |
                               (SynchScope << 1));
  }

  /// Returns the ordering constraint on this RMW.
  AtomicOrdering getOrdering() const {
    return AtomicOrdering((getSubclassDataFromInstruction() >> 2) & 7);
  }

  /// Returns whether this RMW is atomic between threads or only within a
  /// single thread.
  SynchronizationScope getSynchScope() const {
    return SynchronizationScope((getSubclassDataFromInstruction() & 2) >> 1);
  }

  Value *getPointerOperand() { return getOperand(0); }
  const Value *getPointerOperand() const { return getOperand(0); }
  static unsigned getPointerOperandIndex() { return 0U; }

  Value *getValOperand() { return getOperand(1); }
  const Value *getValOperand() const { return getOperand(1); }

  /// \brief Returns the address space of the pointer operand.
  unsigned getPointerAddressSpace() const {
    return getPointerOperand()->getType()->getPointerAddressSpace();
  }

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == Instruction::AtomicRMW;
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
private:
  void Init(BinOp Operation, Value *Ptr, Value *Val,
            AtomicOrdering Ordering, SynchronizationScope SynchScope);
  // Shadow Instruction::setInstructionSubclassData with a private forwarding
  // method so that subclasses cannot accidentally use it.
  void setInstructionSubclassData(unsigned short D) {
    Instruction::setInstructionSubclassData(D);
  }
};

template <>
struct OperandTraits<AtomicRMWInst>
    : public FixedNumOperandTraits<AtomicRMWInst,2> {
};

DEFINE_TRANSPARENT_OPERAND_ACCESSORS(AtomicRMWInst, Value)

//===----------------------------------------------------------------------===//
//                             GetElementPtrInst Class
//===----------------------------------------------------------------------===//

// checkGEPType - Simple wrapper function to give a better assertion failure
// message on bad indexes for a gep instruction.
//
inline Type *checkGEPType(Type *Ty) {
  assert(Ty && "Invalid GetElementPtrInst indices for type!");
  return Ty;
}

/// GetElementPtrInst - an instruction for type-safe pointer arithmetic to
/// access elements of arrays and structs
///
class GetElementPtrInst : public Instruction {
  GetElementPtrInst(const GetElementPtrInst &GEPI);
  void init(Value *Ptr, ArrayRef<Value *> IdxList, const Twine &NameStr);

  /// Constructors - Create a getelementptr instruction with a base pointer an
  /// list of indices. The first ctor can optionally insert before an existing
  /// instruction, the second appends the new instruction to the specified
  /// BasicBlock.
  inline GetElementPtrInst(Value *Ptr, ArrayRef<Value *> IdxList,
                           unsigned Values, const Twine &NameStr,
                           Instruction *InsertBefore);
  inline GetElementPtrInst(Value *Ptr, ArrayRef<Value *> IdxList,
                           unsigned Values, const Twine &NameStr,
                           BasicBlock *InsertAtEnd);
protected:
  virtual GetElementPtrInst *clone_impl() const;
public:
  static GetElementPtrInst *Create(Value *Ptr, ArrayRef<Value *> IdxList,
                                   const Twine &NameStr = "",
                                   Instruction *InsertBefore = 0) {
    unsigned Values = 1 + unsigned(IdxList.size());
    return new(Values)
      GetElementPtrInst(Ptr, IdxList, Values, NameStr, InsertBefore);
  }
  static GetElementPtrInst *Create(Value *Ptr, ArrayRef<Value *> IdxList,
                                   const Twine &NameStr,
                                   BasicBlock *InsertAtEnd) {
    unsigned Values = 1 + unsigned(IdxList.size());
    return new(Values)
      GetElementPtrInst(Ptr, IdxList, Values, NameStr, InsertAtEnd);
  }

  /// Create an "inbounds" getelementptr. See the documentation for the
  /// "inbounds" flag in LangRef.html for details.
  static GetElementPtrInst *CreateInBounds(Value *Ptr,
                                           ArrayRef<Value *> IdxList,
                                           const Twine &NameStr = "",
                                           Instruction *InsertBefore = 0) {
    GetElementPtrInst *GEP = Create(Ptr, IdxList, NameStr, InsertBefore);
    GEP->setIsInBounds(true);
    return GEP;
  }
  static GetElementPtrInst *CreateInBounds(Value *Ptr,
                                           ArrayRef<Value *> IdxList,
                                           const Twine &NameStr,
                                           BasicBlock *InsertAtEnd) {
    GetElementPtrInst *GEP = Create(Ptr, IdxList, NameStr, InsertAtEnd);
    GEP->setIsInBounds(true);
    return GEP;
  }

  /// Transparently provide more efficient getOperand methods.
  DECLARE_TRANSPARENT_OPERAND_ACCESSORS(Value);

  // getType - Overload to return most specific sequential type.
  SequentialType *getType() const {
    return cast<SequentialType>(Instruction::getType());
  }

  /// \brief Returns the address space of this instruction's pointer type.
  unsigned getAddressSpace() const {
    // Note that this is always the same as the pointer operand's address space
    // and that is cheaper to compute, so cheat here.
    return getPointerAddressSpace();
  }

  /// getIndexedType - Returns the type of the element that would be loaded with
  /// a load instruction with the specified parameters.
  ///
  /// Null is returned if the indices are invalid for the specified
  /// pointer type.
  ///
  static Type *getIndexedType(Type *Ptr, ArrayRef<Value *> IdxList);
  static Type *getIndexedType(Type *Ptr, ArrayRef<Constant *> IdxList);
  static Type *getIndexedType(Type *Ptr, ArrayRef<uint64_t> IdxList);

  inline op_iterator       idx_begin()       { return op_begin()+1; }
  inline const_op_iterator idx_begin() const { return op_begin()+1; }
  inline op_iterator       idx_end()         { return op_end(); }
  inline const_op_iterator idx_end()   const { return op_end(); }

  Value *getPointerOperand() {
    return getOperand(0);
  }
  const Value *getPointerOperand() const {
    return getOperand(0);
  }
  static unsigned getPointerOperandIndex() {
    return 0U;    // get index for modifying correct operand.
  }

  /// getPointerOperandType - Method to return the pointer operand as a
  /// PointerType.
  Type *getPointerOperandType() const {
    return getPointerOperand()->getType();
  }

  /// \brief Returns the address space of the pointer operand.
  unsigned getPointerAddressSpace() const {
    return getPointerOperandType()->getPointerAddressSpace();
  }

  /// GetGEPReturnType - Returns the pointer type returned by the GEP
  /// instruction, which may be a vector of pointers.
  static Type *getGEPReturnType(Value *Ptr, ArrayRef<Value *> IdxList) {
    Type *PtrTy = PointerType::get(checkGEPType(
                                   getIndexedType(Ptr->getType(), IdxList)),
                                   Ptr->getType()->getPointerAddressSpace());
    // Vector GEP
    if (Ptr->getType()->isVectorTy()) {
      unsigned NumElem = cast<VectorType>(Ptr->getType())->getNumElements();
      return VectorType::get(PtrTy, NumElem);
    }

    // Scalar GEP
    return PtrTy;
  }

  unsigned getNumIndices() const {  // Note: always non-negative
    return getNumOperands() - 1;
  }

  bool hasIndices() const {
    return getNumOperands() > 1;
  }

  /// hasAllZeroIndices - Return true if all of the indices of this GEP are
  /// zeros.  If so, the result pointer and the first operand have the same
  /// value, just potentially different types.
  bool hasAllZeroIndices() const;

  /// hasAllConstantIndices - Return true if all of the indices of this GEP are
  /// constant integers.  If so, the result pointer and the first operand have
  /// a constant offset between them.
  bool hasAllConstantIndices() const;

  /// setIsInBounds - Set or clear the inbounds flag on this GEP instruction.
  /// See LangRef.html for the meaning of inbounds on a getelementptr.
  void setIsInBounds(bool b = true);

  /// isInBounds - Determine whether the GEP has the inbounds flag.
  bool isInBounds() const;

  /// \brief Accumulate the constant address offset of this GEP if possible.
  ///
  /// This routine accepts an APInt into which it will accumulate the constant
  /// offset of this GEP if the GEP is in fact constant. If the GEP is not
  /// all-constant, it returns false and the value of the offset APInt is
  /// undefined (it is *not* preserved!). The APInt passed into this routine
  /// must be at least as wide as the IntPtr type for the address space of
  /// the base GEP pointer.
  bool accumulateConstantOffset(const DataLayout &DL, APInt &Offset) const;

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return (I->getOpcode() == Instruction::GetElementPtr);
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
};

template <>
struct OperandTraits<GetElementPtrInst> :
  public VariadicOperandTraits<GetElementPtrInst, 1> {
};

GetElementPtrInst::GetElementPtrInst(Value *Ptr,
                                     ArrayRef<Value *> IdxList,
                                     unsigned Values,
                                     const Twine &NameStr,
                                     Instruction *InsertBefore)
  : Instruction(getGEPReturnType(Ptr, IdxList),
                GetElementPtr,
                OperandTraits<GetElementPtrInst>::op_end(this) - Values,
                Values, InsertBefore) {
  init(Ptr, IdxList, NameStr);
}
GetElementPtrInst::GetElementPtrInst(Value *Ptr,
                                     ArrayRef<Value *> IdxList,
                                     unsigned Values,
                                     const Twine &NameStr,
                                     BasicBlock *InsertAtEnd)
  : Instruction(getGEPReturnType(Ptr, IdxList),
                GetElementPtr,
                OperandTraits<GetElementPtrInst>::op_end(this) - Values,
                Values, InsertAtEnd) {
  init(Ptr, IdxList, NameStr);
}


DEFINE_TRANSPARENT_OPERAND_ACCESSORS(GetElementPtrInst, Value)


//===----------------------------------------------------------------------===//
//                               ICmpInst Class
//===----------------------------------------------------------------------===//

/// This instruction compares its operands according to the predicate given
/// to the constructor. It only operates on integers or pointers. The operands
/// must be identical types.
/// \brief Represent an integer comparison operator.
class ICmpInst: public CmpInst {
protected:
  /// \brief Clone an identical ICmpInst
  virtual ICmpInst *clone_impl() const;
public:
  /// \brief Constructor with insert-before-instruction semantics.
  ICmpInst(
    Instruction *InsertBefore,  ///< Where to insert
    Predicate pred,  ///< The predicate to use for the comparison
    Value *LHS,      ///< The left-hand-side of the expression
    Value *RHS,      ///< The right-hand-side of the expression
    const Twine &NameStr = ""  ///< Name of the instruction
  ) : CmpInst(makeCmpResultType(LHS->getType()),
              Instruction::ICmp, pred, LHS, RHS, NameStr,
              InsertBefore) {
    assert(pred >= CmpInst::FIRST_ICMP_PREDICATE &&
           pred <= CmpInst::LAST_ICMP_PREDICATE &&
           "Invalid ICmp predicate value");
    assert(getOperand(0)->getType() == getOperand(1)->getType() &&
          "Both operands to ICmp instruction are not of the same type!");
    // Check that the operands are the right type
    assert((getOperand(0)->getType()->isIntOrIntVectorTy() ||
            getOperand(0)->getType()->getScalarType()->isPointerTy()) &&
           "Invalid operand types for ICmp instruction");
  }

  /// \brief Constructor with insert-at-end semantics.
  ICmpInst(
    BasicBlock &InsertAtEnd, ///< Block to insert into.
    Predicate pred,  ///< The predicate to use for the comparison
    Value *LHS,      ///< The left-hand-side of the expression
    Value *RHS,      ///< The right-hand-side of the expression
    const Twine &NameStr = ""  ///< Name of the instruction
  ) : CmpInst(makeCmpResultType(LHS->getType()),
              Instruction::ICmp, pred, LHS, RHS, NameStr,
              &InsertAtEnd) {
    assert(pred >= CmpInst::FIRST_ICMP_PREDICATE &&
          pred <= CmpInst::LAST_ICMP_PREDICATE &&
          "Invalid ICmp predicate value");
    assert(getOperand(0)->getType() == getOperand(1)->getType() &&
          "Both operands to ICmp instruction are not of the same type!");
    // Check that the operands are the right type
    assert((getOperand(0)->getType()->isIntOrIntVectorTy() ||
            getOperand(0)->getType()->getScalarType()->isPointerTy()) &&
           "Invalid operand types for ICmp instruction");
  }

  /// \brief Constructor with no-insertion semantics
  ICmpInst(
    Predicate pred, ///< The predicate to use for the comparison
    Value *LHS,     ///< The left-hand-side of the expression
    Value *RHS,     ///< The right-hand-side of the expression
    const Twine &NameStr = "" ///< Name of the instruction
  ) : CmpInst(makeCmpResultType(LHS->getType()),
              Instruction::ICmp, pred, LHS, RHS, NameStr) {
    assert(pred >= CmpInst::FIRST_ICMP_PREDICATE &&
           pred <= CmpInst::LAST_ICMP_PREDICATE &&
           "Invalid ICmp predicate value");
    assert(getOperand(0)->getType() == getOperand(1)->getType() &&
          "Both operands to ICmp instruction are not of the same type!");
    // Check that the operands are the right type
    assert((getOperand(0)->getType()->isIntOrIntVectorTy() ||
            getOperand(0)->getType()->getScalarType()->isPointerTy()) &&
           "Invalid operand types for ICmp instruction");
  }

  /// For example, EQ->EQ, SLE->SLE, UGT->SGT, etc.
  /// @returns the predicate that would be the result if the operand were
  /// regarded as signed.
  /// \brief Return the signed version of the predicate
  Predicate getSignedPredicate() const {
    return getSignedPredicate(getPredicate());
  }

  /// This is a static version that you can use without an instruction.
  /// \brief Return the signed version of the predicate.
  static Predicate getSignedPredicate(Predicate pred);

  /// For example, EQ->EQ, SLE->ULE, UGT->UGT, etc.
  /// @returns the predicate that would be the result if the operand were
  /// regarded as unsigned.
  /// \brief Return the unsigned version of the predicate
  Predicate getUnsignedPredicate() const {
    return getUnsignedPredicate(getPredicate());
  }

  /// This is a static version that you can use without an instruction.
  /// \brief Return the unsigned version of the predicate.
  static Predicate getUnsignedPredicate(Predicate pred);

  /// isEquality - Return true if this predicate is either EQ or NE.  This also
  /// tests for commutativity.
  static bool isEquality(Predicate P) {
    return P == ICMP_EQ || P == ICMP_NE;
  }

  /// isEquality - Return true if this predicate is either EQ or NE.  This also
  /// tests for commutativity.
  bool isEquality() const {
    return isEquality(getPredicate());
  }

  /// @returns true if the predicate of this ICmpInst is commutative
  /// \brief Determine if this relation is commutative.
  bool isCommutative() const { return isEquality(); }

  /// isRelational - Return true if the predicate is relational (not EQ or NE).
  ///
  bool isRelational() const {
    return !isEquality();
  }

  /// isRelational - Return true if the predicate is relational (not EQ or NE).
  ///
  static bool isRelational(Predicate P) {
    return !isEquality(P);
  }

  /// Initialize a set of values that all satisfy the predicate with C.
  /// \brief Make a ConstantRange for a relation with a constant value.
  static ConstantRange makeConstantRange(Predicate pred, const APInt &C);

  /// Exchange the two operands to this instruction in such a way that it does
  /// not modify the semantics of the instruction. The predicate value may be
  /// changed to retain the same result if the predicate is order dependent
  /// (e.g. ult).
  /// \brief Swap operands and adjust predicate.
  void swapOperands() {
    setPredicate(getSwappedPredicate());
    Op<0>().swap(Op<1>());
  }

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == Instruction::ICmp;
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }

};

//===----------------------------------------------------------------------===//
//                               FCmpInst Class
//===----------------------------------------------------------------------===//

/// This instruction compares its operands according to the predicate given
/// to the constructor. It only operates on floating point values or packed
/// vectors of floating point values. The operands must be identical types.
/// \brief Represents a floating point comparison operator.
class FCmpInst: public CmpInst {
protected:
  /// \brief Clone an identical FCmpInst
  virtual FCmpInst *clone_impl() const;
public:
  /// \brief Constructor with insert-before-instruction semantics.
  FCmpInst(
    Instruction *InsertBefore, ///< Where to insert
    Predicate pred,  ///< The predicate to use for the comparison
    Value *LHS,      ///< The left-hand-side of the expression
    Value *RHS,      ///< The right-hand-side of the expression
    const Twine &NameStr = ""  ///< Name of the instruction
  ) : CmpInst(makeCmpResultType(LHS->getType()),
              Instruction::FCmp, pred, LHS, RHS, NameStr,
              InsertBefore) {
    assert(pred <= FCmpInst::LAST_FCMP_PREDICATE &&
           "Invalid FCmp predicate value");
    assert(getOperand(0)->getType() == getOperand(1)->getType() &&
           "Both operands to FCmp instruction are not of the same type!");
    // Check that the operands are the right type
    assert(getOperand(0)->getType()->isFPOrFPVectorTy() &&
           "Invalid operand types for FCmp instruction");
  }

  /// \brief Constructor with insert-at-end semantics.
  FCmpInst(
    BasicBlock &InsertAtEnd, ///< Block to insert into.
    Predicate pred,  ///< The predicate to use for the comparison
    Value *LHS,      ///< The left-hand-side of the expression
    Value *RHS,      ///< The right-hand-side of the expression
    const Twine &NameStr = ""  ///< Name of the instruction
  ) : CmpInst(makeCmpResultType(LHS->getType()),
              Instruction::FCmp, pred, LHS, RHS, NameStr,
              &InsertAtEnd) {
    assert(pred <= FCmpInst::LAST_FCMP_PREDICATE &&
           "Invalid FCmp predicate value");
    assert(getOperand(0)->getType() == getOperand(1)->getType() &&
           "Both operands to FCmp instruction are not of the same type!");
    // Check that the operands are the right type
    assert(getOperand(0)->getType()->isFPOrFPVectorTy() &&
           "Invalid operand types for FCmp instruction");
  }

  /// \brief Constructor with no-insertion semantics
  FCmpInst(
    Predicate pred, ///< The predicate to use for the comparison
    Value *LHS,     ///< The left-hand-side of the expression
    Value *RHS,     ///< The right-hand-side of the expression
    const Twine &NameStr = "" ///< Name of the instruction
  ) : CmpInst(makeCmpResultType(LHS->getType()),
              Instruction::FCmp, pred, LHS, RHS, NameStr) {
    assert(pred <= FCmpInst::LAST_FCMP_PREDICATE &&
           "Invalid FCmp predicate value");
    assert(getOperand(0)->getType() == getOperand(1)->getType() &&
           "Both operands to FCmp instruction are not of the same type!");
    // Check that the operands are the right type
    assert(getOperand(0)->getType()->isFPOrFPVectorTy() &&
           "Invalid operand types for FCmp instruction");
  }

  /// @returns true if the predicate of this instruction is EQ or NE.
  /// \brief Determine if this is an equality predicate.
  bool isEquality() const {
    return getPredicate() == FCMP_OEQ || getPredicate() == FCMP_ONE ||
           getPredicate() == FCMP_UEQ || getPredicate() == FCMP_UNE;
  }

  /// @returns true if the predicate of this instruction is commutative.
  /// \brief Determine if this is a commutative predicate.
  bool isCommutative() const {
    return isEquality() ||
           getPredicate() == FCMP_FALSE ||
           getPredicate() == FCMP_TRUE ||
           getPredicate() == FCMP_ORD ||
           getPredicate() == FCMP_UNO;
  }

  /// @returns true if the predicate is relational (not EQ or NE).
  /// \brief Determine if this a relational predicate.
  bool isRelational() const { return !isEquality(); }

  /// Exchange the two operands to this instruction in such a way that it does
  /// not modify the semantics of the instruction. The predicate value may be
  /// changed to retain the same result if the predicate is order dependent
  /// (e.g. ult).
  /// \brief Swap operands and adjust predicate.
  void swapOperands() {
    setPredicate(getSwappedPredicate());
    Op<0>().swap(Op<1>());
  }

  /// \brief Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == Instruction::FCmp;
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
};

//===----------------------------------------------------------------------===//
/// CallInst - This class represents a function call, abstracting a target
/// machine's calling convention.  This class uses low bit of the SubClassData
/// field to indicate whether or not this is a tail call.  The rest of the bits
/// hold the calling convention of the call.
///
class CallInst : public Instruction {
  AttributeSet AttributeList; ///< parameter attributes for call
  CallInst(const CallInst &CI);
  void init(Value *Func, ArrayRef<Value *> Args, const Twine &NameStr);
  void init(Value *Func, const Twine &NameStr);

  /// Construct a CallInst given a range of arguments.
  /// \brief Construct a CallInst from a range of arguments
  inline CallInst(Value *Func, ArrayRef<Value *> Args,
                  const Twine &NameStr, Instruction *InsertBefore);

  /// Construct a CallInst given a range of arguments.
  /// \brief Construct a CallInst from a range of arguments
  inline CallInst(Value *Func, ArrayRef<Value *> Args,
                  const Twine &NameStr, BasicBlock *InsertAtEnd);

  CallInst(Value *F, Value *Actual, const Twine &NameStr,
           Instruction *InsertBefore);
  CallInst(Value *F, Value *Actual, const Twine &NameStr,
           BasicBlock *InsertAtEnd);
  explicit CallInst(Value *F, const Twine &NameStr,
                    Instruction *InsertBefore);
  CallInst(Value *F, const Twine &NameStr, BasicBlock *InsertAtEnd);
protected:
  virtual CallInst *clone_impl() const;
public:
  static CallInst *Create(Value *Func,
                          ArrayRef<Value *> Args,
                          const Twine &NameStr = "",
                          Instruction *InsertBefore = 0) {
    return new(unsigned(Args.size() + 1))
      CallInst(Func, Args, NameStr, InsertBefore);
  }
  static CallInst *Create(Value *Func,
                          ArrayRef<Value *> Args,
                          const Twine &NameStr, BasicBlock *InsertAtEnd) {
    return new(unsigned(Args.size() + 1))
      CallInst(Func, Args, NameStr, InsertAtEnd);
  }
  static CallInst *Create(Value *F, const Twine &NameStr = "",
                          Instruction *InsertBefore = 0) {
    return new(1) CallInst(F, NameStr, InsertBefore);
  }
  static CallInst *Create(Value *F, const Twine &NameStr,
                          BasicBlock *InsertAtEnd) {
    return new(1) CallInst(F, NameStr, InsertAtEnd);
  }
  /// CreateMalloc - Generate the IR for a call to malloc:
  /// 1. Compute the malloc call's argument as the specified type's size,
  ///    possibly multiplied by the array size if the array size is not
  ///    constant 1.
  /// 2. Call malloc with that argument.
  /// 3. Bitcast the result of the malloc call to the specified type.
  static Instruction *CreateMalloc(Instruction *InsertBefore,
                                   Type *IntPtrTy, Type *AllocTy,
                                   Value *AllocSize, Value *ArraySize = 0,
                                   Function* MallocF = 0,
                                   const Twine &Name = "");
  static Instruction *CreateMalloc(BasicBlock *InsertAtEnd,
                                   Type *IntPtrTy, Type *AllocTy,
                                   Value *AllocSize, Value *ArraySize = 0,
                                   Function* MallocF = 0,
                                   const Twine &Name = "");
  /// CreateFree - Generate the IR for a call to the builtin free function.
  static Instruction* CreateFree(Value* Source, Instruction *InsertBefore);
  static Instruction* CreateFree(Value* Source, BasicBlock *InsertAtEnd);

  ~CallInst();

  bool isTailCall() const { return getSubclassDataFromInstruction() & 1; }
  void setTailCall(bool isTC = true) {
    setInstructionSubclassData((getSubclassDataFromInstruction() & ~1) |
                               unsigned(isTC));
  }

  /// Provide fast operand accessors
  DECLARE_TRANSPARENT_OPERAND_ACCESSORS(Value);

  /// getNumArgOperands - Return the number of call arguments.
  ///
  unsigned getNumArgOperands() const { return getNumOperands() - 1; }

  /// getArgOperand/setArgOperand - Return/set the i-th call argument.
  ///
  Value *getArgOperand(unsigned i) const { return getOperand(i); }
  void setArgOperand(unsigned i, Value *v) { setOperand(i, v); }

  /// getCallingConv/setCallingConv - Get or set the calling convention of this
  /// function call.
  CallingConv::ID getCallingConv() const {
    return static_cast<CallingConv::ID>(getSubclassDataFromInstruction() >> 1);
  }
  void setCallingConv(CallingConv::ID CC) {
    setInstructionSubclassData((getSubclassDataFromInstruction() & 1) |
                               (static_cast<unsigned>(CC) << 1));
  }

  /// getAttributes - Return the parameter attributes for this call.
  ///
  const AttributeSet &getAttributes() const { return AttributeList; }

  /// setAttributes - Set the parameter attributes for this call.
  ///
  void setAttributes(const AttributeSet &Attrs) { AttributeList = Attrs; }

  /// addAttribute - adds the attribute to the list of attributes.
  void addAttribute(unsigned i, Attribute::AttrKind attr);

  /// removeAttribute - removes the attribute from the list of attributes.
  void removeAttribute(unsigned i, Attribute attr);

  /// \brief Determine whether this call has the given attribute.
  bool hasFnAttr(Attribute::AttrKind A) const;

  /// \brief Determine whether the call or the callee has the given attributes.
  bool paramHasAttr(unsigned i, Attribute::AttrKind A) const;

  /// \brief Extract the alignment for a call or parameter (0=unknown).
  unsigned getParamAlignment(unsigned i) const {
    return AttributeList.getParamAlignment(i);
  }

  /// \brief Return true if the call should not be inlined.
  bool isNoInline() const { return hasFnAttr(Attribute::NoInline); }
  void setIsNoInline() {
    addAttribute(AttributeSet::FunctionIndex, Attribute::NoInline);
  }

  /// \brief Return true if the call can return twice
  bool canReturnTwice() const {
    return hasFnAttr(Attribute::ReturnsTwice);
  }
  void setCanReturnTwice() {
    addAttribute(AttributeSet::FunctionIndex, Attribute::ReturnsTwice);
  }

  /// \brief Determine if the call does not access memory.
  bool doesNotAccessMemory() const {
    return hasFnAttr(Attribute::ReadNone);
  }
  void setDoesNotAccessMemory() {
    addAttribute(AttributeSet::FunctionIndex, Attribute::ReadNone);
  }

  /// \brief Determine if the call does not access or only reads memory.
  bool onlyReadsMemory() const {
    return doesNotAccessMemory() || hasFnAttr(Attribute::ReadOnly);
  }
  void setOnlyReadsMemory() {
    addAttribute(AttributeSet::FunctionIndex, Attribute::ReadOnly);
  }

  /// \brief Determine if the call cannot return.
  bool doesNotReturn() const { return hasFnAttr(Attribute::NoReturn); }
  void setDoesNotReturn() {
    addAttribute(AttributeSet::FunctionIndex, Attribute::NoReturn);
  }

  /// \brief Determine if the call cannot unwind.
  bool doesNotThrow() const { return hasFnAttr(Attribute::NoUnwind); }
  void setDoesNotThrow() {
    addAttribute(AttributeSet::FunctionIndex, Attribute::NoUnwind);
  }

  /// \brief Determine if the call cannot be duplicated.
  bool cannotDuplicate() const {return hasFnAttr(Attribute::NoDuplicate); }
  void setCannotDuplicate() {
    addAttribute(AttributeSet::FunctionIndex, Attribute::NoDuplicate);
  }

  /// \brief Determine if the call returns a structure through first
  /// pointer argument.
  bool hasStructRetAttr() const {
    // Be friendly and also check the callee.
    return paramHasAttr(1, Attribute::StructRet);
  }

  /// \brief Determine if any call argument is an aggregate passed by value.
  bool hasByValArgument() const {
    return AttributeList.hasAttrSomewhere(Attribute::ByVal);
  }

  /// getCalledFunction - Return the function called, or null if this is an
  /// indirect function invocation.
  ///
  Function *getCalledFunction() const {
    return dyn_cast<Function>(Op<-1>());
  }

  /// getCalledValue - Get a pointer to the function that is invoked by this
  /// instruction.
  const Value *getCalledValue() const { return Op<-1>(); }
        Value *getCalledValue()       { return Op<-1>(); }

  /// setCalledFunction - Set the function called.
  void setCalledFunction(Value* Fn) {
    Op<-1>() = Fn;
  }

  /// isInlineAsm - Check if this call is an inline asm statement.
  bool isInlineAsm() const {
    return isa<InlineAsm>(Op<-1>());
  }

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == Instruction::Call;
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
private:
  // Shadow Instruction::setInstructionSubclassData with a private forwarding
  // method so that subclasses cannot accidentally use it.
  void setInstructionSubclassData(unsigned short D) {
    Instruction::setInstructionSubclassData(D);
  }
};

template <>
struct OperandTraits<CallInst> : public VariadicOperandTraits<CallInst, 1> {
};

CallInst::CallInst(Value *Func, ArrayRef<Value *> Args,
                   const Twine &NameStr, BasicBlock *InsertAtEnd)
  : Instruction(cast<FunctionType>(cast<PointerType>(Func->getType())
                                   ->getElementType())->getReturnType(),
                Instruction::Call,
                OperandTraits<CallInst>::op_end(this) - (Args.size() + 1),
                unsigned(Args.size() + 1), InsertAtEnd) {
  init(Func, Args, NameStr);
}

CallInst::CallInst(Value *Func, ArrayRef<Value *> Args,
                   const Twine &NameStr, Instruction *InsertBefore)
  : Instruction(cast<FunctionType>(cast<PointerType>(Func->getType())
                                   ->getElementType())->getReturnType(),
                Instruction::Call,
                OperandTraits<CallInst>::op_end(this) - (Args.size() + 1),
                unsigned(Args.size() + 1), InsertBefore) {
  init(Func, Args, NameStr);
}


// Note: if you get compile errors about private methods then
//       please update your code to use the high-level operand
//       interfaces. See line 943 above.
DEFINE_TRANSPARENT_OPERAND_ACCESSORS(CallInst, Value)

//===----------------------------------------------------------------------===//
//                               SelectInst Class
//===----------------------------------------------------------------------===//

/// SelectInst - This class represents the LLVM 'select' instruction.
///
class SelectInst : public Instruction {
  void init(Value *C, Value *S1, Value *S2) {
    assert(!areInvalidOperands(C, S1, S2) && "Invalid operands for select");
    Op<0>() = C;
    Op<1>() = S1;
    Op<2>() = S2;
  }

  SelectInst(Value *C, Value *S1, Value *S2, const Twine &NameStr,
             Instruction *InsertBefore)
    : Instruction(S1->getType(), Instruction::Select,
                  &Op<0>(), 3, InsertBefore) {
    init(C, S1, S2);
    setName(NameStr);
  }
  SelectInst(Value *C, Value *S1, Value *S2, const Twine &NameStr,
             BasicBlock *InsertAtEnd)
    : Instruction(S1->getType(), Instruction::Select,
                  &Op<0>(), 3, InsertAtEnd) {
    init(C, S1, S2);
    setName(NameStr);
  }
protected:
  virtual SelectInst *clone_impl() const;
public:
  static SelectInst *Create(Value *C, Value *S1, Value *S2,
                            const Twine &NameStr = "",
                            Instruction *InsertBefore = 0) {
    return new(3) SelectInst(C, S1, S2, NameStr, InsertBefore);
  }
  static SelectInst *Create(Value *C, Value *S1, Value *S2,
                            const Twine &NameStr,
                            BasicBlock *InsertAtEnd) {
    return new(3) SelectInst(C, S1, S2, NameStr, InsertAtEnd);
  }

  const Value *getCondition() const { return Op<0>(); }
  const Value *getTrueValue() const { return Op<1>(); }
  const Value *getFalseValue() const { return Op<2>(); }
  Value *getCondition() { return Op<0>(); }
  Value *getTrueValue() { return Op<1>(); }
  Value *getFalseValue() { return Op<2>(); }

  /// areInvalidOperands - Return a string if the specified operands are invalid
  /// for a select operation, otherwise return null.
  static const char *areInvalidOperands(Value *Cond, Value *True, Value *False);

  /// Transparently provide more efficient getOperand methods.
  DECLARE_TRANSPARENT_OPERAND_ACCESSORS(Value);

  OtherOps getOpcode() const {
    return static_cast<OtherOps>(Instruction::getOpcode());
  }

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == Instruction::Select;
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
};

template <>
struct OperandTraits<SelectInst> : public FixedNumOperandTraits<SelectInst, 3> {
};

DEFINE_TRANSPARENT_OPERAND_ACCESSORS(SelectInst, Value)

//===----------------------------------------------------------------------===//
//                                VAArgInst Class
//===----------------------------------------------------------------------===//

/// VAArgInst - This class represents the va_arg llvm instruction, which returns
/// an argument of the specified type given a va_list and increments that list
///
class VAArgInst : public UnaryInstruction {
protected:
  virtual VAArgInst *clone_impl() const;

public:
  VAArgInst(Value *List, Type *Ty, const Twine &NameStr = "",
             Instruction *InsertBefore = 0)
    : UnaryInstruction(Ty, VAArg, List, InsertBefore) {
    setName(NameStr);
  }
  VAArgInst(Value *List, Type *Ty, const Twine &NameStr,
            BasicBlock *InsertAtEnd)
    : UnaryInstruction(Ty, VAArg, List, InsertAtEnd) {
    setName(NameStr);
  }

  Value *getPointerOperand() { return getOperand(0); }
  const Value *getPointerOperand() const { return getOperand(0); }
  static unsigned getPointerOperandIndex() { return 0U; }

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == VAArg;
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
};

//===----------------------------------------------------------------------===//
//                                ExtractElementInst Class
//===----------------------------------------------------------------------===//

/// ExtractElementInst - This instruction extracts a single (scalar)
/// element from a VectorType value
///
class ExtractElementInst : public Instruction {
  ExtractElementInst(Value *Vec, Value *Idx, const Twine &NameStr = "",
                     Instruction *InsertBefore = 0);
  ExtractElementInst(Value *Vec, Value *Idx, const Twine &NameStr,
                     BasicBlock *InsertAtEnd);
protected:
  virtual ExtractElementInst *clone_impl() const;

public:
  static ExtractElementInst *Create(Value *Vec, Value *Idx,
                                   const Twine &NameStr = "",
                                   Instruction *InsertBefore = 0) {
    return new(2) ExtractElementInst(Vec, Idx, NameStr, InsertBefore);
  }
  static ExtractElementInst *Create(Value *Vec, Value *Idx,
                                   const Twine &NameStr,
                                   BasicBlock *InsertAtEnd) {
    return new(2) ExtractElementInst(Vec, Idx, NameStr, InsertAtEnd);
  }

  /// isValidOperands - Return true if an extractelement instruction can be
  /// formed with the specified operands.
  static bool isValidOperands(const Value *Vec, const Value *Idx);

  Value *getVectorOperand() { return Op<0>(); }
  Value *getIndexOperand() { return Op<1>(); }
  const Value *getVectorOperand() const { return Op<0>(); }
  const Value *getIndexOperand() const { return Op<1>(); }

  VectorType *getVectorOperandType() const {
    return cast<VectorType>(getVectorOperand()->getType());
  }


  /// Transparently provide more efficient getOperand methods.
  DECLARE_TRANSPARENT_OPERAND_ACCESSORS(Value);

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == Instruction::ExtractElement;
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
};

template <>
struct OperandTraits<ExtractElementInst> :
  public FixedNumOperandTraits<ExtractElementInst, 2> {
};

DEFINE_TRANSPARENT_OPERAND_ACCESSORS(ExtractElementInst, Value)

//===----------------------------------------------------------------------===//
//                                InsertElementInst Class
//===----------------------------------------------------------------------===//

/// InsertElementInst - This instruction inserts a single (scalar)
/// element into a VectorType value
///
class InsertElementInst : public Instruction {
  InsertElementInst(Value *Vec, Value *NewElt, Value *Idx,
                    const Twine &NameStr = "",
                    Instruction *InsertBefore = 0);
  InsertElementInst(Value *Vec, Value *NewElt, Value *Idx,
                    const Twine &NameStr, BasicBlock *InsertAtEnd);
protected:
  virtual InsertElementInst *clone_impl() const;

public:
  static InsertElementInst *Create(Value *Vec, Value *NewElt, Value *Idx,
                                   const Twine &NameStr = "",
                                   Instruction *InsertBefore = 0) {
    return new(3) InsertElementInst(Vec, NewElt, Idx, NameStr, InsertBefore);
  }
  static InsertElementInst *Create(Value *Vec, Value *NewElt, Value *Idx,
                                   const Twine &NameStr,
                                   BasicBlock *InsertAtEnd) {
    return new(3) InsertElementInst(Vec, NewElt, Idx, NameStr, InsertAtEnd);
  }

  /// isValidOperands - Return true if an insertelement instruction can be
  /// formed with the specified operands.
  static bool isValidOperands(const Value *Vec, const Value *NewElt,
                              const Value *Idx);

  /// getType - Overload to return most specific vector type.
  ///
  VectorType *getType() const {
    return cast<VectorType>(Instruction::getType());
  }

  /// Transparently provide more efficient getOperand methods.
  DECLARE_TRANSPARENT_OPERAND_ACCESSORS(Value);

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == Instruction::InsertElement;
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
};

template <>
struct OperandTraits<InsertElementInst> :
  public FixedNumOperandTraits<InsertElementInst, 3> {
};

DEFINE_TRANSPARENT_OPERAND_ACCESSORS(InsertElementInst, Value)

//===----------------------------------------------------------------------===//
//                           ShuffleVectorInst Class
//===----------------------------------------------------------------------===//

/// ShuffleVectorInst - This instruction constructs a fixed permutation of two
/// input vectors.
///
class ShuffleVectorInst : public Instruction {
protected:
  virtual ShuffleVectorInst *clone_impl() const;

public:
  // allocate space for exactly three operands
  void *operator new(size_t s) {
    return User::operator new(s, 3);
  }
  ShuffleVectorInst(Value *V1, Value *V2, Value *Mask,
                    const Twine &NameStr = "",
                    Instruction *InsertBefor = 0);
  ShuffleVectorInst(Value *V1, Value *V2, Value *Mask,
                    const Twine &NameStr, BasicBlock *InsertAtEnd);

  /// isValidOperands - Return true if a shufflevector instruction can be
  /// formed with the specified operands.
  static bool isValidOperands(const Value *V1, const Value *V2,
                              const Value *Mask);

  /// getType - Overload to return most specific vector type.
  ///
  VectorType *getType() const {
    return cast<VectorType>(Instruction::getType());
  }

  /// Transparently provide more efficient getOperand methods.
  DECLARE_TRANSPARENT_OPERAND_ACCESSORS(Value);

  Constant *getMask() const {
    return cast<Constant>(getOperand(2));
  }

  /// getMaskValue - Return the index from the shuffle mask for the specified
  /// output result.  This is either -1 if the element is undef or a number less
  /// than 2*numelements.
  static int getMaskValue(Constant *Mask, unsigned i);

  int getMaskValue(unsigned i) const {
    return getMaskValue(getMask(), i);
  }

  /// getShuffleMask - Return the full mask for this instruction, where each
  /// element is the element number and undef's are returned as -1.
  static void getShuffleMask(Constant *Mask, SmallVectorImpl<int> &Result);

  void getShuffleMask(SmallVectorImpl<int> &Result) const {
    return getShuffleMask(getMask(), Result);
  }

  SmallVector<int, 16> getShuffleMask() const {
    SmallVector<int, 16> Mask;
    getShuffleMask(Mask);
    return Mask;
  }


  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == Instruction::ShuffleVector;
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
};

template <>
struct OperandTraits<ShuffleVectorInst> :
  public FixedNumOperandTraits<ShuffleVectorInst, 3> {
};

DEFINE_TRANSPARENT_OPERAND_ACCESSORS(ShuffleVectorInst, Value)

//===----------------------------------------------------------------------===//
//                                ExtractValueInst Class
//===----------------------------------------------------------------------===//

/// ExtractValueInst - This instruction extracts a struct member or array
/// element value from an aggregate value.
///
class ExtractValueInst : public UnaryInstruction {
  SmallVector<unsigned, 4> Indices;

  ExtractValueInst(const ExtractValueInst &EVI);
  void init(ArrayRef<unsigned> Idxs, const Twine &NameStr);

  /// Constructors - Create a extractvalue instruction with a base aggregate
  /// value and a list of indices.  The first ctor can optionally insert before
  /// an existing instruction, the second appends the new instruction to the
  /// specified BasicBlock.
  inline ExtractValueInst(Value *Agg,
                          ArrayRef<unsigned> Idxs,
                          const Twine &NameStr,
                          Instruction *InsertBefore);
  inline ExtractValueInst(Value *Agg,
                          ArrayRef<unsigned> Idxs,
                          const Twine &NameStr, BasicBlock *InsertAtEnd);

  // allocate space for exactly one operand
  void *operator new(size_t s) {
    return User::operator new(s, 1);
  }
protected:
  virtual ExtractValueInst *clone_impl() const;

public:
  static ExtractValueInst *Create(Value *Agg,
                                  ArrayRef<unsigned> Idxs,
                                  const Twine &NameStr = "",
                                  Instruction *InsertBefore = 0) {
    return new
      ExtractValueInst(Agg, Idxs, NameStr, InsertBefore);
  }
  static ExtractValueInst *Create(Value *Agg,
                                  ArrayRef<unsigned> Idxs,
                                  const Twine &NameStr,
                                  BasicBlock *InsertAtEnd) {
    return new ExtractValueInst(Agg, Idxs, NameStr, InsertAtEnd);
  }

  /// getIndexedType - Returns the type of the element that would be extracted
  /// with an extractvalue instruction with the specified parameters.
  ///
  /// Null is returned if the indices are invalid for the specified type.
  static Type *getIndexedType(Type *Agg, ArrayRef<unsigned> Idxs);

  typedef const unsigned* idx_iterator;
  inline idx_iterator idx_begin() const { return Indices.begin(); }
  inline idx_iterator idx_end()   const { return Indices.end(); }

  Value *getAggregateOperand() {
    return getOperand(0);
  }
  const Value *getAggregateOperand() const {
    return getOperand(0);
  }
  static unsigned getAggregateOperandIndex() {
    return 0U;                      // get index for modifying correct operand
  }

  ArrayRef<unsigned> getIndices() const {
    return Indices;
  }

  unsigned getNumIndices() const {
    return (unsigned)Indices.size();
  }

  bool hasIndices() const {
    return true;
  }

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == Instruction::ExtractValue;
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
};

ExtractValueInst::ExtractValueInst(Value *Agg,
                                   ArrayRef<unsigned> Idxs,
                                   const Twine &NameStr,
                                   Instruction *InsertBefore)
  : UnaryInstruction(checkGEPType(getIndexedType(Agg->getType(), Idxs)),
                     ExtractValue, Agg, InsertBefore) {
  init(Idxs, NameStr);
}
ExtractValueInst::ExtractValueInst(Value *Agg,
                                   ArrayRef<unsigned> Idxs,
                                   const Twine &NameStr,
                                   BasicBlock *InsertAtEnd)
  : UnaryInstruction(checkGEPType(getIndexedType(Agg->getType(), Idxs)),
                     ExtractValue, Agg, InsertAtEnd) {
  init(Idxs, NameStr);
}


//===----------------------------------------------------------------------===//
//                                InsertValueInst Class
//===----------------------------------------------------------------------===//

/// InsertValueInst - This instruction inserts a struct field of array element
/// value into an aggregate value.
///
class InsertValueInst : public Instruction {
  SmallVector<unsigned, 4> Indices;

  void *operator new(size_t, unsigned) LLVM_DELETED_FUNCTION;
  InsertValueInst(const InsertValueInst &IVI);
  void init(Value *Agg, Value *Val, ArrayRef<unsigned> Idxs,
            const Twine &NameStr);

  /// Constructors - Create a insertvalue instruction with a base aggregate
  /// value, a value to insert, and a list of indices.  The first ctor can
  /// optionally insert before an existing instruction, the second appends
  /// the new instruction to the specified BasicBlock.
  inline InsertValueInst(Value *Agg, Value *Val,
                         ArrayRef<unsigned> Idxs,
                         const Twine &NameStr,
                         Instruction *InsertBefore);
  inline InsertValueInst(Value *Agg, Value *Val,
                         ArrayRef<unsigned> Idxs,
                         const Twine &NameStr, BasicBlock *InsertAtEnd);

  /// Constructors - These two constructors are convenience methods because one
  /// and two index insertvalue instructions are so common.
  InsertValueInst(Value *Agg, Value *Val,
                  unsigned Idx, const Twine &NameStr = "",
                  Instruction *InsertBefore = 0);
  InsertValueInst(Value *Agg, Value *Val, unsigned Idx,
                  const Twine &NameStr, BasicBlock *InsertAtEnd);
protected:
  virtual InsertValueInst *clone_impl() const;
public:
  // allocate space for exactly two operands
  void *operator new(size_t s) {
    return User::operator new(s, 2);
  }

  static InsertValueInst *Create(Value *Agg, Value *Val,
                                 ArrayRef<unsigned> Idxs,
                                 const Twine &NameStr = "",
                                 Instruction *InsertBefore = 0) {
    return new InsertValueInst(Agg, Val, Idxs, NameStr, InsertBefore);
  }
  static InsertValueInst *Create(Value *Agg, Value *Val,
                                 ArrayRef<unsigned> Idxs,
                                 const Twine &NameStr,
                                 BasicBlock *InsertAtEnd) {
    return new InsertValueInst(Agg, Val, Idxs, NameStr, InsertAtEnd);
  }

  /// Transparently provide more efficient getOperand methods.
  DECLARE_TRANSPARENT_OPERAND_ACCESSORS(Value);

  typedef const unsigned* idx_iterator;
  inline idx_iterator idx_begin() const { return Indices.begin(); }
  inline idx_iterator idx_end()   const { return Indices.end(); }

  Value *getAggregateOperand() {
    return getOperand(0);
  }
  const Value *getAggregateOperand() const {
    return getOperand(0);
  }
  static unsigned getAggregateOperandIndex() {
    return 0U;                      // get index for modifying correct operand
  }

  Value *getInsertedValueOperand() {
    return getOperand(1);
  }
  const Value *getInsertedValueOperand() const {
    return getOperand(1);
  }
  static unsigned getInsertedValueOperandIndex() {
    return 1U;                      // get index for modifying correct operand
  }

  ArrayRef<unsigned> getIndices() const {
    return Indices;
  }

  unsigned getNumIndices() const {
    return (unsigned)Indices.size();
  }

  bool hasIndices() const {
    return true;
  }

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == Instruction::InsertValue;
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
};

template <>
struct OperandTraits<InsertValueInst> :
  public FixedNumOperandTraits<InsertValueInst, 2> {
};

InsertValueInst::InsertValueInst(Value *Agg,
                                 Value *Val,
                                 ArrayRef<unsigned> Idxs,
                                 const Twine &NameStr,
                                 Instruction *InsertBefore)
  : Instruction(Agg->getType(), InsertValue,
                OperandTraits<InsertValueInst>::op_begin(this),
                2, InsertBefore) {
  init(Agg, Val, Idxs, NameStr);
}
InsertValueInst::InsertValueInst(Value *Agg,
                                 Value *Val,
                                 ArrayRef<unsigned> Idxs,
                                 const Twine &NameStr,
                                 BasicBlock *InsertAtEnd)
  : Instruction(Agg->getType(), InsertValue,
                OperandTraits<InsertValueInst>::op_begin(this),
                2, InsertAtEnd) {
  init(Agg, Val, Idxs, NameStr);
}

DEFINE_TRANSPARENT_OPERAND_ACCESSORS(InsertValueInst, Value)

//===----------------------------------------------------------------------===//
//                               PHINode Class
//===----------------------------------------------------------------------===//

// PHINode - The PHINode class is used to represent the magical mystical PHI
// node, that can not exist in nature, but can be synthesized in a computer
// scientist's overactive imagination.
//
class PHINode : public Instruction {
  void *operator new(size_t, unsigned) LLVM_DELETED_FUNCTION;
  /// ReservedSpace - The number of operands actually allocated.  NumOperands is
  /// the number actually in use.
  unsigned ReservedSpace;
  PHINode(const PHINode &PN);
  // allocate space for exactly zero operands
  void *operator new(size_t s) {
    return User::operator new(s, 0);
  }
  explicit PHINode(Type *Ty, unsigned NumReservedValues,
                   const Twine &NameStr = "", Instruction *InsertBefore = 0)
    : Instruction(Ty, Instruction::PHI, 0, 0, InsertBefore),
      ReservedSpace(NumReservedValues) {
    setName(NameStr);
    OperandList = allocHungoffUses(ReservedSpace);
  }

  PHINode(Type *Ty, unsigned NumReservedValues, const Twine &NameStr,
          BasicBlock *InsertAtEnd)
    : Instruction(Ty, Instruction::PHI, 0, 0, InsertAtEnd),
      ReservedSpace(NumReservedValues) {
    setName(NameStr);
    OperandList = allocHungoffUses(ReservedSpace);
  }
protected:
  // allocHungoffUses - this is more complicated than the generic
  // User::allocHungoffUses, because we have to allocate Uses for the incoming
  // values and pointers to the incoming blocks, all in one allocation.
  Use *allocHungoffUses(unsigned) const;

  virtual PHINode *clone_impl() const;
public:
  /// Constructors - NumReservedValues is a hint for the number of incoming
  /// edges that this phi node will have (use 0 if you really have no idea).
  static PHINode *Create(Type *Ty, unsigned NumReservedValues,
                         const Twine &NameStr = "",
                         Instruction *InsertBefore = 0) {
    return new PHINode(Ty, NumReservedValues, NameStr, InsertBefore);
  }
  static PHINode *Create(Type *Ty, unsigned NumReservedValues,
                         const Twine &NameStr, BasicBlock *InsertAtEnd) {
    return new PHINode(Ty, NumReservedValues, NameStr, InsertAtEnd);
  }
  ~PHINode();

  /// Provide fast operand accessors
  DECLARE_TRANSPARENT_OPERAND_ACCESSORS(Value);

  // Block iterator interface. This provides access to the list of incoming
  // basic blocks, which parallels the list of incoming values.

  typedef BasicBlock **block_iterator;
  typedef BasicBlock * const *const_block_iterator;

  block_iterator block_begin() {
    Use::UserRef *ref =
      reinterpret_cast<Use::UserRef*>(op_begin() + ReservedSpace);
    return reinterpret_cast<block_iterator>(ref + 1);
  }

  const_block_iterator block_begin() const {
    const Use::UserRef *ref =
      reinterpret_cast<const Use::UserRef*>(op_begin() + ReservedSpace);
    return reinterpret_cast<const_block_iterator>(ref + 1);
  }

  block_iterator block_end() {
    return block_begin() + getNumOperands();
  }

  const_block_iterator block_end() const {
    return block_begin() + getNumOperands();
  }

  /// getNumIncomingValues - Return the number of incoming edges
  ///
  unsigned getNumIncomingValues() const { return getNumOperands(); }

  /// getIncomingValue - Return incoming value number x
  ///
  Value *getIncomingValue(unsigned i) const {
    return getOperand(i);
  }
  void setIncomingValue(unsigned i, Value *V) {
    setOperand(i, V);
  }
  static unsigned getOperandNumForIncomingValue(unsigned i) {
    return i;
  }
  static unsigned getIncomingValueNumForOperand(unsigned i) {
    return i;
  }

  /// getIncomingBlock - Return incoming basic block number @p i.
  ///
  BasicBlock *getIncomingBlock(unsigned i) const {
    return block_begin()[i];
  }

  /// getIncomingBlock - Return incoming basic block corresponding
  /// to an operand of the PHI.
  ///
  BasicBlock *getIncomingBlock(const Use &U) const {
    assert(this == U.getUser() && "Iterator doesn't point to PHI's Uses?");
    return getIncomingBlock(unsigned(&U - op_begin()));
  }

  /// getIncomingBlock - Return incoming basic block corresponding
  /// to value use iterator.
  ///
  template <typename U>
  BasicBlock *getIncomingBlock(value_use_iterator<U> I) const {
    return getIncomingBlock(I.getUse());
  }

  void setIncomingBlock(unsigned i, BasicBlock *BB) {
    block_begin()[i] = BB;
  }

  /// addIncoming - Add an incoming value to the end of the PHI list
  ///
  void addIncoming(Value *V, BasicBlock *BB) {
    assert(V && "PHI node got a null value!");
    assert(BB && "PHI node got a null basic block!");
    assert(getType() == V->getType() &&
           "All operands to PHI node must be the same type as the PHI node!");
    if (NumOperands == ReservedSpace)
      growOperands();  // Get more space!
    // Initialize some new operands.
    ++NumOperands;
    setIncomingValue(NumOperands - 1, V);
    setIncomingBlock(NumOperands - 1, BB);
  }

  /// removeIncomingValue - Remove an incoming value.  This is useful if a
  /// predecessor basic block is deleted.  The value removed is returned.
  ///
  /// If the last incoming value for a PHI node is removed (and DeletePHIIfEmpty
  /// is true), the PHI node is destroyed and any uses of it are replaced with
  /// dummy values.  The only time there should be zero incoming values to a PHI
  /// node is when the block is dead, so this strategy is sound.
  ///
  Value *removeIncomingValue(unsigned Idx, bool DeletePHIIfEmpty = true);

  Value *removeIncomingValue(const BasicBlock *BB, bool DeletePHIIfEmpty=true) {
    int Idx = getBasicBlockIndex(BB);
    assert(Idx >= 0 && "Invalid basic block argument to remove!");
    return removeIncomingValue(Idx, DeletePHIIfEmpty);
  }

  /// getBasicBlockIndex - Return the first index of the specified basic
  /// block in the value list for this PHI.  Returns -1 if no instance.
  ///
  int getBasicBlockIndex(const BasicBlock *BB) const {
    for (unsigned i = 0, e = getNumOperands(); i != e; ++i)
      if (block_begin()[i] == BB)
        return i;
    return -1;
  }

  Value *getIncomingValueForBlock(const BasicBlock *BB) const {
    int Idx = getBasicBlockIndex(BB);
    assert(Idx >= 0 && "Invalid basic block argument!");
    return getIncomingValue(Idx);
  }

  /// hasConstantValue - If the specified PHI node always merges together the
  /// same value, return the value, otherwise return null.
  Value *hasConstantValue() const;

  /// Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == Instruction::PHI;
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
 private:
  void growOperands();
};

template <>
struct OperandTraits<PHINode> : public HungoffOperandTraits<2> {
};

DEFINE_TRANSPARENT_OPERAND_ACCESSORS(PHINode, Value)

//===----------------------------------------------------------------------===//
//                           LandingPadInst Class
//===----------------------------------------------------------------------===//

//===---------------------------------------------------------------------------
/// LandingPadInst - The landingpad instruction holds all of the information
/// necessary to generate correct exception handling. The landingpad instruction
/// cannot be moved from the top of a landing pad block, which itself is
/// accessible only from the 'unwind' edge of an invoke. This uses the
/// SubclassData field in Value to store whether or not the landingpad is a
/// cleanup.
///
class LandingPadInst : public Instruction {
  /// ReservedSpace - The number of operands actually allocated.  NumOperands is
  /// the number actually in use.
  unsigned ReservedSpace;
  LandingPadInst(const LandingPadInst &LP);
public:
  enum ClauseType { Catch, Filter };
private:
  void *operator new(size_t, unsigned) LLVM_DELETED_FUNCTION;
  // Allocate space for exactly zero operands.
  void *operator new(size_t s) {
    return User::operator new(s, 0);
  }
  void growOperands(unsigned Size);
  void init(Value *PersFn, unsigned NumReservedValues, const Twine &NameStr);

  explicit LandingPadInst(Type *RetTy, Value *PersonalityFn,
                          unsigned NumReservedValues, const Twine &NameStr,
                          Instruction *InsertBefore);
  explicit LandingPadInst(Type *RetTy, Value *PersonalityFn,
                          unsigned NumReservedValues, const Twine &NameStr,
                          BasicBlock *InsertAtEnd);
protected:
  virtual LandingPadInst *clone_impl() const;
public:
  /// Constructors - NumReservedClauses is a hint for the number of incoming
  /// clauses that this landingpad will have (use 0 if you really have no idea).
  static LandingPadInst *Create(Type *RetTy, Value *PersonalityFn,
                                unsigned NumReservedClauses,
                                const Twine &NameStr = "",
                                Instruction *InsertBefore = 0);
  static LandingPadInst *Create(Type *RetTy, Value *PersonalityFn,
                                unsigned NumReservedClauses,
                                const Twine &NameStr, BasicBlock *InsertAtEnd);
  ~LandingPadInst();

  /// Provide fast operand accessors
  DECLARE_TRANSPARENT_OPERAND_ACCESSORS(Value);

  /// getPersonalityFn - Get the personality function associated with this
  /// landing pad.
  Value *getPersonalityFn() const { return getOperand(0); }

  /// isCleanup - Return 'true' if this landingpad instruction is a
  /// cleanup. I.e., it should be run when unwinding even if its landing pad
  /// doesn't catch the exception.
  bool isCleanup() const { return getSubclassDataFromInstruction() & 1; }

  /// setCleanup - Indicate that this landingpad instruction is a cleanup.
  void setCleanup(bool V) {
    setInstructionSubclassData((getSubclassDataFromInstruction() & ~1) |
                               (V ? 1 : 0));
  }

  /// addClause - Add a catch or filter clause to the landing pad.
  void addClause(Value *ClauseVal);

  /// getClause - Get the value of the clause at index Idx. Use isCatch/isFilter
  /// to determine what type of clause this is.
  Value *getClause(unsigned Idx) const { return OperandList[Idx + 1]; }

  /// isCatch - Return 'true' if the clause and index Idx is a catch clause.
  bool isCatch(unsigned Idx) const {
    return !isa<ArrayType>(OperandList[Idx + 1]->getType());
  }

  /// isFilter - Return 'true' if the clause and index Idx is a filter clause.
  bool isFilter(unsigned Idx) const {
    return isa<ArrayType>(OperandList[Idx + 1]->getType());
  }

  /// getNumClauses - Get the number of clauses for this landing pad.
  unsigned getNumClauses() const { return getNumOperands() - 1; }

  /// reserveClauses - Grow the size of the operand list to accommodate the new
  /// number of clauses.
  void reserveClauses(unsigned Size) { growOperands(Size); }

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == Instruction::LandingPad;
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
};

template <>
struct OperandTraits<LandingPadInst> : public HungoffOperandTraits<2> {
};

DEFINE_TRANSPARENT_OPERAND_ACCESSORS(LandingPadInst, Value)

//===----------------------------------------------------------------------===//
//                               ReturnInst Class
//===----------------------------------------------------------------------===//

//===---------------------------------------------------------------------------
/// ReturnInst - Return a value (possibly void), from a function.  Execution
/// does not continue in this function any longer.
///
class ReturnInst : public TerminatorInst {
  ReturnInst(const ReturnInst &RI);

private:
  // ReturnInst constructors:
  // ReturnInst()                  - 'ret void' instruction
  // ReturnInst(    null)          - 'ret void' instruction
  // ReturnInst(Value* X)          - 'ret X'    instruction
  // ReturnInst(    null, Inst *I) - 'ret void' instruction, insert before I
  // ReturnInst(Value* X, Inst *I) - 'ret X'    instruction, insert before I
  // ReturnInst(    null, BB *B)   - 'ret void' instruction, insert @ end of B
  // ReturnInst(Value* X, BB *B)   - 'ret X'    instruction, insert @ end of B
  //
  // NOTE: If the Value* passed is of type void then the constructor behaves as
  // if it was passed NULL.
  explicit ReturnInst(LLVMContext &C, Value *retVal = 0,
                      Instruction *InsertBefore = 0);
  ReturnInst(LLVMContext &C, Value *retVal, BasicBlock *InsertAtEnd);
  explicit ReturnInst(LLVMContext &C, BasicBlock *InsertAtEnd);
protected:
  virtual ReturnInst *clone_impl() const;
public:
  static ReturnInst* Create(LLVMContext &C, Value *retVal = 0,
                            Instruction *InsertBefore = 0) {
    return new(!!retVal) ReturnInst(C, retVal, InsertBefore);
  }
  static ReturnInst* Create(LLVMContext &C, Value *retVal,
                            BasicBlock *InsertAtEnd) {
    return new(!!retVal) ReturnInst(C, retVal, InsertAtEnd);
  }
  static ReturnInst* Create(LLVMContext &C, BasicBlock *InsertAtEnd) {
    return new(0) ReturnInst(C, InsertAtEnd);
  }
  virtual ~ReturnInst();

  /// Provide fast operand accessors
  DECLARE_TRANSPARENT_OPERAND_ACCESSORS(Value);

  /// Convenience accessor. Returns null if there is no return value.
  Value *getReturnValue() const {
    return getNumOperands() != 0 ? getOperand(0) : 0;
  }

  unsigned getNumSuccessors() const { return 0; }

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return (I->getOpcode() == Instruction::Ret);
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
 private:
  virtual BasicBlock *getSuccessorV(unsigned idx) const;
  virtual unsigned getNumSuccessorsV() const;
  virtual void setSuccessorV(unsigned idx, BasicBlock *B);
};

template <>
struct OperandTraits<ReturnInst> : public VariadicOperandTraits<ReturnInst> {
};

DEFINE_TRANSPARENT_OPERAND_ACCESSORS(ReturnInst, Value)

//===----------------------------------------------------------------------===//
//                               BranchInst Class
//===----------------------------------------------------------------------===//

//===---------------------------------------------------------------------------
/// BranchInst - Conditional or Unconditional Branch instruction.
///
class BranchInst : public TerminatorInst {
  /// Ops list - Branches are strange.  The operands are ordered:
  ///  [Cond, FalseDest,] TrueDest.  This makes some accessors faster because
  /// they don't have to check for cond/uncond branchness. These are mostly
  /// accessed relative from op_end().
  BranchInst(const BranchInst &BI);
  void AssertOK();
  // BranchInst constructors (where {B, T, F} are blocks, and C is a condition):
  // BranchInst(BB *B)                           - 'br B'
  // BranchInst(BB* T, BB *F, Value *C)          - 'br C, T, F'
  // BranchInst(BB* B, Inst *I)                  - 'br B'        insert before I
  // BranchInst(BB* T, BB *F, Value *C, Inst *I) - 'br C, T, F', insert before I
  // BranchInst(BB* B, BB *I)                    - 'br B'        insert at end
  // BranchInst(BB* T, BB *F, Value *C, BB *I)   - 'br C, T, F', insert at end
  explicit BranchInst(BasicBlock *IfTrue, Instruction *InsertBefore = 0);
  BranchInst(BasicBlock *IfTrue, BasicBlock *IfFalse, Value *Cond,
             Instruction *InsertBefore = 0);
  BranchInst(BasicBlock *IfTrue, BasicBlock *InsertAtEnd);
  BranchInst(BasicBlock *IfTrue, BasicBlock *IfFalse, Value *Cond,
             BasicBlock *InsertAtEnd);
protected:
  virtual BranchInst *clone_impl() const;
public:
  static BranchInst *Create(BasicBlock *IfTrue, Instruction *InsertBefore = 0) {
    return new(1) BranchInst(IfTrue, InsertBefore);
  }
  static BranchInst *Create(BasicBlock *IfTrue, BasicBlock *IfFalse,
                            Value *Cond, Instruction *InsertBefore = 0) {
    return new(3) BranchInst(IfTrue, IfFalse, Cond, InsertBefore);
  }
  static BranchInst *Create(BasicBlock *IfTrue, BasicBlock *InsertAtEnd) {
    return new(1) BranchInst(IfTrue, InsertAtEnd);
  }
  static BranchInst *Create(BasicBlock *IfTrue, BasicBlock *IfFalse,
                            Value *Cond, BasicBlock *InsertAtEnd) {
    return new(3) BranchInst(IfTrue, IfFalse, Cond, InsertAtEnd);
  }

  /// Transparently provide more efficient getOperand methods.
  DECLARE_TRANSPARENT_OPERAND_ACCESSORS(Value);

  bool isUnconditional() const { return getNumOperands() == 1; }
  bool isConditional()   const { return getNumOperands() == 3; }

  Value *getCondition() const {
    assert(isConditional() && "Cannot get condition of an uncond branch!");
    return Op<-3>();
  }

  void setCondition(Value *V) {
    assert(isConditional() && "Cannot set condition of unconditional branch!");
    Op<-3>() = V;
  }

  unsigned getNumSuccessors() const { return 1+isConditional(); }

  BasicBlock *getSuccessor(unsigned i) const {
    assert(i < getNumSuccessors() && "Successor # out of range for Branch!");
    return cast_or_null<BasicBlock>((&Op<-1>() - i)->get());
  }

  void setSuccessor(unsigned idx, BasicBlock *NewSucc) {
    assert(idx < getNumSuccessors() && "Successor # out of range for Branch!");
    *(&Op<-1>() - idx) = (Value*)NewSucc;
  }

  /// \brief Swap the successors of this branch instruction.
  ///
  /// Swaps the successors of the branch instruction. This also swaps any
  /// branch weight metadata associated with the instruction so that it
  /// continues to map correctly to each operand.
  void swapSuccessors();

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return (I->getOpcode() == Instruction::Br);
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
private:
  virtual BasicBlock *getSuccessorV(unsigned idx) const;
  virtual unsigned getNumSuccessorsV() const;
  virtual void setSuccessorV(unsigned idx, BasicBlock *B);
};

template <>
struct OperandTraits<BranchInst> : public VariadicOperandTraits<BranchInst, 1> {
};

DEFINE_TRANSPARENT_OPERAND_ACCESSORS(BranchInst, Value)

//===----------------------------------------------------------------------===//
//                               SwitchInst Class
//===----------------------------------------------------------------------===//

//===---------------------------------------------------------------------------
/// SwitchInst - Multiway switch
///
class SwitchInst : public TerminatorInst {
  void *operator new(size_t, unsigned) LLVM_DELETED_FUNCTION;
  unsigned ReservedSpace;
  // Operands format:
  // Operand[0]    = Value to switch on
  // Operand[1]    = Default basic block destination
  // Operand[2n  ] = Value to match
  // Operand[2n+1] = BasicBlock to go to on match

  // Store case values separately from operands list. We needn't User-Use
  // concept here, since it is just a case value, it will always constant,
  // and case value couldn't reused with another instructions/values.
  // Additionally:
  // It allows us to use custom type for case values that is not inherited
  // from Value. Since case value is a complex type that implements
  // the subset of integers, we needn't extract sub-constants within
  // slow getAggregateElement method.
  // For case values we will use std::list to by two reasons:
  // 1. It allows to add/remove cases without whole collection reallocation.
  // 2. In most of cases we needn't random access.
  // Currently case values are also stored in Operands List, but it will moved
  // out in future commits.
  typedef std::list<IntegersSubset> Subsets;
  typedef Subsets::iterator SubsetsIt;
  typedef Subsets::const_iterator SubsetsConstIt;

  Subsets TheSubsets;

  SwitchInst(const SwitchInst &SI);
  void init(Value *Value, BasicBlock *Default, unsigned NumReserved);
  void growOperands();
  // allocate space for exactly zero operands
  void *operator new(size_t s) {
    return User::operator new(s, 0);
  }
  /// SwitchInst ctor - Create a new switch instruction, specifying a value to
  /// switch on and a default destination.  The number of additional cases can
  /// be specified here to make memory allocation more efficient.  This
  /// constructor can also autoinsert before another instruction.
  SwitchInst(Value *Value, BasicBlock *Default, unsigned NumCases,
             Instruction *InsertBefore);

  /// SwitchInst ctor - Create a new switch instruction, specifying a value to
  /// switch on and a default destination.  The number of additional cases can
  /// be specified here to make memory allocation more efficient.  This
  /// constructor also autoinserts at the end of the specified BasicBlock.
  SwitchInst(Value *Value, BasicBlock *Default, unsigned NumCases,
             BasicBlock *InsertAtEnd);
protected:
  virtual SwitchInst *clone_impl() const;
public:

  // FIXME: Currently there are a lot of unclean template parameters,
  // we need to make refactoring in future.
  // All these parameters are used to implement both iterator and const_iterator
  // without code duplication.
  // SwitchInstTy may be "const SwitchInst" or "SwitchInst"
  // ConstantIntTy may be "const ConstantInt" or "ConstantInt"
  // SubsetsItTy may be SubsetsConstIt or SubsetsIt
  // BasicBlockTy may be "const BasicBlock" or "BasicBlock"
  template <class SwitchInstTy, class ConstantIntTy,
            class SubsetsItTy, class BasicBlockTy>
    class CaseIteratorT;

  typedef CaseIteratorT<const SwitchInst, const ConstantInt,
                        SubsetsConstIt, const BasicBlock> ConstCaseIt;
  class CaseIt;

  // -2
  static const unsigned DefaultPseudoIndex = static_cast<unsigned>(~0L-1);

  static SwitchInst *Create(Value *Value, BasicBlock *Default,
                            unsigned NumCases, Instruction *InsertBefore = 0) {
    return new SwitchInst(Value, Default, NumCases, InsertBefore);
  }
  static SwitchInst *Create(Value *Value, BasicBlock *Default,
                            unsigned NumCases, BasicBlock *InsertAtEnd) {
    return new SwitchInst(Value, Default, NumCases, InsertAtEnd);
  }

  ~SwitchInst();

  /// Provide fast operand accessors
  DECLARE_TRANSPARENT_OPERAND_ACCESSORS(Value);

  // Accessor Methods for Switch stmt
  Value *getCondition() const { return getOperand(0); }
  void setCondition(Value *V) { setOperand(0, V); }

  BasicBlock *getDefaultDest() const {
    return cast<BasicBlock>(getOperand(1));
  }

  void setDefaultDest(BasicBlock *DefaultCase) {
    setOperand(1, reinterpret_cast<Value*>(DefaultCase));
  }

  /// getNumCases - return the number of 'cases' in this switch instruction,
  /// except the default case
  unsigned getNumCases() const {
    return getNumOperands()/2 - 1;
  }

  /// Returns a read/write iterator that points to the first
  /// case in SwitchInst.
  CaseIt case_begin() {
    return CaseIt(this, 0, TheSubsets.begin());
  }
  /// Returns a read-only iterator that points to the first
  /// case in the SwitchInst.
  ConstCaseIt case_begin() const {
    return ConstCaseIt(this, 0, TheSubsets.begin());
  }

  /// Returns a read/write iterator that points one past the last
  /// in the SwitchInst.
  CaseIt case_end() {
    return CaseIt(this, getNumCases(), TheSubsets.end());
  }
  /// Returns a read-only iterator that points one past the last
  /// in the SwitchInst.
  ConstCaseIt case_end() const {
    return ConstCaseIt(this, getNumCases(), TheSubsets.end());
  }
  /// Returns an iterator that points to the default case.
  /// Note: this iterator allows to resolve successor only. Attempt
  /// to resolve case value causes an assertion.
  /// Also note, that increment and decrement also causes an assertion and
  /// makes iterator invalid.
  CaseIt case_default() {
    return CaseIt(this, DefaultPseudoIndex, TheSubsets.end());
  }
  ConstCaseIt case_default() const {
    return ConstCaseIt(this, DefaultPseudoIndex, TheSubsets.end());
  }

  /// findCaseValue - Search all of the case values for the specified constant.
  /// If it is explicitly handled, return the case iterator of it, otherwise
  /// return default case iterator to indicate
  /// that it is handled by the default handler.
  CaseIt findCaseValue(const ConstantInt *C) {
    for (CaseIt i = case_begin(), e = case_end(); i != e; ++i)
      if (i.getCaseValueEx().isSatisfies(IntItem::fromConstantInt(C)))
        return i;
    return case_default();
  }
  ConstCaseIt findCaseValue(const ConstantInt *C) const {
    for (ConstCaseIt i = case_begin(), e = case_end(); i != e; ++i)
      if (i.getCaseValueEx().isSatisfies(IntItem::fromConstantInt(C)))
        return i;
    return case_default();
  }

  /// findCaseDest - Finds the unique case value for a given successor. Returns
  /// null if the successor is not found, not unique, or is the default case.
  ConstantInt *findCaseDest(BasicBlock *BB) {
    if (BB == getDefaultDest()) return NULL;

    ConstantInt *CI = NULL;
    for (CaseIt i = case_begin(), e = case_end(); i != e; ++i) {
      if (i.getCaseSuccessor() == BB) {
        if (CI) return NULL;   // Multiple cases lead to BB.
        else CI = i.getCaseValue();
      }
    }
    return CI;
  }

  /// addCase - Add an entry to the switch instruction...
  /// @deprecated
  /// Note:
  /// This action invalidates case_end(). Old case_end() iterator will
  /// point to the added case.
  void addCase(ConstantInt *OnVal, BasicBlock *Dest);

  /// addCase - Add an entry to the switch instruction.
  /// Note:
  /// This action invalidates case_end(). Old case_end() iterator will
  /// point to the added case.
  void addCase(IntegersSubset& OnVal, BasicBlock *Dest);

  /// removeCase - This method removes the specified case and its successor
  /// from the switch instruction. Note that this operation may reorder the
  /// remaining cases at index idx and above.
  /// Note:
  /// This action invalidates iterators for all cases following the one removed,
  /// including the case_end() iterator.
  void removeCase(CaseIt& i);

  unsigned getNumSuccessors() const { return getNumOperands()/2; }
  BasicBlock *getSuccessor(unsigned idx) const {
    assert(idx < getNumSuccessors() &&"Successor idx out of range for switch!");
    return cast<BasicBlock>(getOperand(idx*2+1));
  }
  void setSuccessor(unsigned idx, BasicBlock *NewSucc) {
    assert(idx < getNumSuccessors() && "Successor # out of range for switch!");
    setOperand(idx*2+1, (Value*)NewSucc);
  }

  uint16_t hash() const {
    uint32_t NumberOfCases = (uint32_t)getNumCases();
    uint16_t Hash = (0xFFFF & NumberOfCases) ^ (NumberOfCases >> 16);
    for (ConstCaseIt i = case_begin(), e = case_end();
         i != e; ++i) {
      uint32_t NumItems = (uint32_t)i.getCaseValueEx().getNumItems();
      Hash = (Hash << 1) ^ (0xFFFF & NumItems) ^ (NumItems >> 16);
    }
    return Hash;
  }

  // Case iterators definition.

  template <class SwitchInstTy, class ConstantIntTy,
            class SubsetsItTy, class BasicBlockTy>
  class CaseIteratorT {
  protected:

    SwitchInstTy *SI;
    unsigned Index;
    SubsetsItTy SubsetIt;

    /// Initializes case iterator for given SwitchInst and for given
    /// case number.
    friend class SwitchInst;
    CaseIteratorT(SwitchInstTy *SI, unsigned SuccessorIndex,
                  SubsetsItTy CaseValueIt) {
      this->SI = SI;
      Index = SuccessorIndex;
      this->SubsetIt = CaseValueIt;
    }

  public:
    typedef typename SubsetsItTy::reference IntegersSubsetRef;
    typedef CaseIteratorT<SwitchInstTy, ConstantIntTy,
                          SubsetsItTy, BasicBlockTy> Self;

    CaseIteratorT(SwitchInstTy *SI, unsigned CaseNum) {
          this->SI = SI;
          Index = CaseNum;
          SubsetIt = SI->TheSubsets.begin();
          std::advance(SubsetIt, CaseNum);
        }


    /// Initializes case iterator for given SwitchInst and for given
    /// TerminatorInst's successor index.
    static Self fromSuccessorIndex(SwitchInstTy *SI, unsigned SuccessorIndex) {
      assert(SuccessorIndex < SI->getNumSuccessors() &&
             "Successor index # out of range!");
      return SuccessorIndex != 0 ?
             Self(SI, SuccessorIndex - 1) :
             Self(SI, DefaultPseudoIndex);
    }

    /// Resolves case value for current case.
    /// @deprecated
    ConstantIntTy *getCaseValue() {
      assert(Index < SI->getNumCases() && "Index out the number of cases.");
      IntegersSubsetRef CaseRanges = *SubsetIt;

      // FIXME: Currently we work with ConstantInt based cases.
      // So return CaseValue as ConstantInt.
      return CaseRanges.getSingleNumber(0).toConstantInt();
    }

    /// Resolves case value for current case.
    IntegersSubsetRef getCaseValueEx() {
      assert(Index < SI->getNumCases() && "Index out the number of cases.");
      return *SubsetIt;
    }

    /// Resolves successor for current case.
    BasicBlockTy *getCaseSuccessor() {
      assert((Index < SI->getNumCases() ||
              Index == DefaultPseudoIndex) &&
             "Index out the number of cases.");
      return SI->getSuccessor(getSuccessorIndex());
    }

    /// Returns number of current case.
    unsigned getCaseIndex() const { return Index; }

    /// Returns TerminatorInst's successor index for current case successor.
    unsigned getSuccessorIndex() const {
      assert((Index == DefaultPseudoIndex || Index < SI->getNumCases()) &&
             "Index out the number of cases.");
      return Index != DefaultPseudoIndex ? Index + 1 : 0;
    }

    Self operator++() {
      // Check index correctness after increment.
      // Note: Index == getNumCases() means end().
      assert(Index+1 <= SI->getNumCases() && "Index out the number of cases.");
      ++Index;
      if (Index == 0)
        SubsetIt = SI->TheSubsets.begin();
      else
        ++SubsetIt;
      return *this;
    }
    Self operator++(int) {
      Self tmp = *this;
      ++(*this);
      return tmp;
    }
    Self operator--() {
      // Check index correctness after decrement.
      // Note: Index == getNumCases() means end().
      // Also allow "-1" iterator here. That will became valid after ++.
      unsigned NumCases = SI->getNumCases();
      assert((Index == 0 || Index-1 <= NumCases) &&
             "Index out the number of cases.");
      --Index;
      if (Index == NumCases) {
        SubsetIt = SI->TheSubsets.end();
        return *this;
      }

      if (Index != -1U)
        --SubsetIt;

      return *this;
    }
    Self operator--(int) {
      Self tmp = *this;
      --(*this);
      return tmp;
    }
    bool operator==(const Self& RHS) const {
      assert(RHS.SI == SI && "Incompatible operators.");
      return RHS.Index == Index;
    }
    bool operator!=(const Self& RHS) const {
      assert(RHS.SI == SI && "Incompatible operators.");
      return RHS.Index != Index;
    }
  };

  class CaseIt : public CaseIteratorT<SwitchInst, ConstantInt,
                                      SubsetsIt, BasicBlock> {
    typedef CaseIteratorT<SwitchInst, ConstantInt, SubsetsIt, BasicBlock>
      ParentTy;

  protected:
    friend class SwitchInst;
    CaseIt(SwitchInst *SI, unsigned CaseNum, SubsetsIt SubsetIt) :
      ParentTy(SI, CaseNum, SubsetIt) {}

    void updateCaseValueOperand(IntegersSubset& V) {
      SI->setOperand(2 + Index*2, reinterpret_cast<Value*>((Constant*)V));
    }

  public:

    CaseIt(SwitchInst *SI, unsigned CaseNum) : ParentTy(SI, CaseNum) {}

    CaseIt(const ParentTy& Src) : ParentTy(Src) {}

    /// Sets the new value for current case.
    /// @deprecated.
    void setValue(ConstantInt *V) {
      assert(Index < SI->getNumCases() && "Index out the number of cases.");
      IntegersSubsetToBB Mapping;
      // FIXME: Currently we work with ConstantInt based cases.
      // So inititalize IntItem container directly from ConstantInt.
      Mapping.add(IntItem::fromConstantInt(V));
      *SubsetIt = Mapping.getCase();
      updateCaseValueOperand(*SubsetIt);
    }

    /// Sets the new value for current case.
    void setValueEx(IntegersSubset& V) {
      assert(Index < SI->getNumCases() && "Index out the number of cases.");
      *SubsetIt = V;
      updateCaseValueOperand(*SubsetIt);
    }

    /// Sets the new successor for current case.
    void setSuccessor(BasicBlock *S) {
      SI->setSuccessor(getSuccessorIndex(), S);
    }
  };

  // Methods for support type inquiry through isa, cast, and dyn_cast:

  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == Instruction::Switch;
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
private:
  virtual BasicBlock *getSuccessorV(unsigned idx) const;
  virtual unsigned getNumSuccessorsV() const;
  virtual void setSuccessorV(unsigned idx, BasicBlock *B);
};

template <>
struct OperandTraits<SwitchInst> : public HungoffOperandTraits<2> {
};

DEFINE_TRANSPARENT_OPERAND_ACCESSORS(SwitchInst, Value)


//===----------------------------------------------------------------------===//
//                             IndirectBrInst Class
//===----------------------------------------------------------------------===//

//===---------------------------------------------------------------------------
/// IndirectBrInst - Indirect Branch Instruction.
///
class IndirectBrInst : public TerminatorInst {
  void *operator new(size_t, unsigned) LLVM_DELETED_FUNCTION;
  unsigned ReservedSpace;
  // Operand[0]    = Value to switch on
  // Operand[1]    = Default basic block destination
  // Operand[2n  ] = Value to match
  // Operand[2n+1] = BasicBlock to go to on match
  IndirectBrInst(const IndirectBrInst &IBI);
  void init(Value *Address, unsigned NumDests);
  void growOperands();
  // allocate space for exactly zero operands
  void *operator new(size_t s) {
    return User::operator new(s, 0);
  }
  /// IndirectBrInst ctor - Create a new indirectbr instruction, specifying an
  /// Address to jump to.  The number of expected destinations can be specified
  /// here to make memory allocation more efficient.  This constructor can also
  /// autoinsert before another instruction.
  IndirectBrInst(Value *Address, unsigned NumDests, Instruction *InsertBefore);

  /// IndirectBrInst ctor - Create a new indirectbr instruction, specifying an
  /// Address to jump to.  The number of expected destinations can be specified
  /// here to make memory allocation more efficient.  This constructor also
  /// autoinserts at the end of the specified BasicBlock.
  IndirectBrInst(Value *Address, unsigned NumDests, BasicBlock *InsertAtEnd);
protected:
  virtual IndirectBrInst *clone_impl() const;
public:
  static IndirectBrInst *Create(Value *Address, unsigned NumDests,
                                Instruction *InsertBefore = 0) {
    return new IndirectBrInst(Address, NumDests, InsertBefore);
  }
  static IndirectBrInst *Create(Value *Address, unsigned NumDests,
                                BasicBlock *InsertAtEnd) {
    return new IndirectBrInst(Address, NumDests, InsertAtEnd);
  }
  ~IndirectBrInst();

  /// Provide fast operand accessors.
  DECLARE_TRANSPARENT_OPERAND_ACCESSORS(Value);

  // Accessor Methods for IndirectBrInst instruction.
  Value *getAddress() { return getOperand(0); }
  const Value *getAddress() const { return getOperand(0); }
  void setAddress(Value *V) { setOperand(0, V); }


  /// getNumDestinations - return the number of possible destinations in this
  /// indirectbr instruction.
  unsigned getNumDestinations() const { return getNumOperands()-1; }

  /// getDestination - Return the specified destination.
  BasicBlock *getDestination(unsigned i) { return getSuccessor(i); }
  const BasicBlock *getDestination(unsigned i) const { return getSuccessor(i); }

  /// addDestination - Add a destination.
  ///
  void addDestination(BasicBlock *Dest);

  /// removeDestination - This method removes the specified successor from the
  /// indirectbr instruction.
  void removeDestination(unsigned i);

  unsigned getNumSuccessors() const { return getNumOperands()-1; }
  BasicBlock *getSuccessor(unsigned i) const {
    return cast<BasicBlock>(getOperand(i+1));
  }
  void setSuccessor(unsigned i, BasicBlock *NewSucc) {
    setOperand(i+1, (Value*)NewSucc);
  }

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == Instruction::IndirectBr;
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
private:
  virtual BasicBlock *getSuccessorV(unsigned idx) const;
  virtual unsigned getNumSuccessorsV() const;
  virtual void setSuccessorV(unsigned idx, BasicBlock *B);
};

template <>
struct OperandTraits<IndirectBrInst> : public HungoffOperandTraits<1> {
};

DEFINE_TRANSPARENT_OPERAND_ACCESSORS(IndirectBrInst, Value)


//===----------------------------------------------------------------------===//
//                               InvokeInst Class
//===----------------------------------------------------------------------===//

/// InvokeInst - Invoke instruction.  The SubclassData field is used to hold the
/// calling convention of the call.
///
class InvokeInst : public TerminatorInst {
  AttributeSet AttributeList;
  InvokeInst(const InvokeInst &BI);
  void init(Value *Func, BasicBlock *IfNormal, BasicBlock *IfException,
            ArrayRef<Value *> Args, const Twine &NameStr);

  /// Construct an InvokeInst given a range of arguments.
  ///
  /// \brief Construct an InvokeInst from a range of arguments
  inline InvokeInst(Value *Func, BasicBlock *IfNormal, BasicBlock *IfException,
                    ArrayRef<Value *> Args, unsigned Values,
                    const Twine &NameStr, Instruction *InsertBefore);

  /// Construct an InvokeInst given a range of arguments.
  ///
  /// \brief Construct an InvokeInst from a range of arguments
  inline InvokeInst(Value *Func, BasicBlock *IfNormal, BasicBlock *IfException,
                    ArrayRef<Value *> Args, unsigned Values,
                    const Twine &NameStr, BasicBlock *InsertAtEnd);
protected:
  virtual InvokeInst *clone_impl() const;
public:
  static InvokeInst *Create(Value *Func,
                            BasicBlock *IfNormal, BasicBlock *IfException,
                            ArrayRef<Value *> Args, const Twine &NameStr = "",
                            Instruction *InsertBefore = 0) {
    unsigned Values = unsigned(Args.size()) + 3;
    return new(Values) InvokeInst(Func, IfNormal, IfException, Args,
                                  Values, NameStr, InsertBefore);
  }
  static InvokeInst *Create(Value *Func,
                            BasicBlock *IfNormal, BasicBlock *IfException,
                            ArrayRef<Value *> Args, const Twine &NameStr,
                            BasicBlock *InsertAtEnd) {
    unsigned Values = unsigned(Args.size()) + 3;
    return new(Values) InvokeInst(Func, IfNormal, IfException, Args,
                                  Values, NameStr, InsertAtEnd);
  }

  /// Provide fast operand accessors
  DECLARE_TRANSPARENT_OPERAND_ACCESSORS(Value);

  /// getNumArgOperands - Return the number of invoke arguments.
  ///
  unsigned getNumArgOperands() const { return getNumOperands() - 3; }

  /// getArgOperand/setArgOperand - Return/set the i-th invoke argument.
  ///
  Value *getArgOperand(unsigned i) const { return getOperand(i); }
  void setArgOperand(unsigned i, Value *v) { setOperand(i, v); }

  /// getCallingConv/setCallingConv - Get or set the calling convention of this
  /// function call.
  CallingConv::ID getCallingConv() const {
    return static_cast<CallingConv::ID>(getSubclassDataFromInstruction());
  }
  void setCallingConv(CallingConv::ID CC) {
    setInstructionSubclassData(static_cast<unsigned>(CC));
  }

  /// getAttributes - Return the parameter attributes for this invoke.
  ///
  const AttributeSet &getAttributes() const { return AttributeList; }

  /// setAttributes - Set the parameter attributes for this invoke.
  ///
  void setAttributes(const AttributeSet &Attrs) { AttributeList = Attrs; }

  /// addAttribute - adds the attribute to the list of attributes.
  void addAttribute(unsigned i, Attribute::AttrKind attr);

  /// removeAttribute - removes the attribute from the list of attributes.
  void removeAttribute(unsigned i, Attribute attr);

  /// \brief Determine whether this call has the NoAlias attribute.
  bool hasFnAttr(Attribute::AttrKind A) const;

  /// \brief Determine whether the call or the callee has the given attributes.
  bool paramHasAttr(unsigned i, Attribute::AttrKind A) const;

  /// \brief Extract the alignment for a call or parameter (0=unknown).
  unsigned getParamAlignment(unsigned i) const {
    return AttributeList.getParamAlignment(i);
  }

  /// \brief Return true if the call should not be inlined.
  bool isNoInline() const { return hasFnAttr(Attribute::NoInline); }
  void setIsNoInline() {
    addAttribute(AttributeSet::FunctionIndex, Attribute::NoInline);
  }

  /// \brief Determine if the call does not access memory.
  bool doesNotAccessMemory() const {
    return hasFnAttr(Attribute::ReadNone);
  }
  void setDoesNotAccessMemory() {
    addAttribute(AttributeSet::FunctionIndex, Attribute::ReadNone);
  }

  /// \brief Determine if the call does not access or only reads memory.
  bool onlyReadsMemory() const {
    return doesNotAccessMemory() || hasFnAttr(Attribute::ReadOnly);
  }
  void setOnlyReadsMemory() {
    addAttribute(AttributeSet::FunctionIndex, Attribute::ReadOnly);
  }

  /// \brief Determine if the call cannot return.
  bool doesNotReturn() const { return hasFnAttr(Attribute::NoReturn); }
  void setDoesNotReturn() {
    addAttribute(AttributeSet::FunctionIndex, Attribute::NoReturn);
  }

  /// \brief Determine if the call cannot unwind.
  bool doesNotThrow() const { return hasFnAttr(Attribute::NoUnwind); }
  void setDoesNotThrow() {
    addAttribute(AttributeSet::FunctionIndex, Attribute::NoUnwind);
  }

  /// \brief Determine if the call returns a structure through first
  /// pointer argument.
  bool hasStructRetAttr() const {
    // Be friendly and also check the callee.
    return paramHasAttr(1, Attribute::StructRet);
  }

  /// \brief Determine if any call argument is an aggregate passed by value.
  bool hasByValArgument() const {
    return AttributeList.hasAttrSomewhere(Attribute::ByVal);
  }

  /// getCalledFunction - Return the function called, or null if this is an
  /// indirect function invocation.
  ///
  Function *getCalledFunction() const {
    return dyn_cast<Function>(Op<-3>());
  }

  /// getCalledValue - Get a pointer to the function that is invoked by this
  /// instruction
  const Value *getCalledValue() const { return Op<-3>(); }
        Value *getCalledValue()       { return Op<-3>(); }

  /// setCalledFunction - Set the function called.
  void setCalledFunction(Value* Fn) {
    Op<-3>() = Fn;
  }

  // get*Dest - Return the destination basic blocks...
  BasicBlock *getNormalDest() const {
    return cast<BasicBlock>(Op<-2>());
  }
  BasicBlock *getUnwindDest() const {
    return cast<BasicBlock>(Op<-1>());
  }
  void setNormalDest(BasicBlock *B) {
    Op<-2>() = reinterpret_cast<Value*>(B);
  }
  void setUnwindDest(BasicBlock *B) {
    Op<-1>() = reinterpret_cast<Value*>(B);
  }

  /// getLandingPadInst - Get the landingpad instruction from the landing pad
  /// block (the unwind destination).
  LandingPadInst *getLandingPadInst() const;

  BasicBlock *getSuccessor(unsigned i) const {
    assert(i < 2 && "Successor # out of range for invoke!");
    return i == 0 ? getNormalDest() : getUnwindDest();
  }

  void setSuccessor(unsigned idx, BasicBlock *NewSucc) {
    assert(idx < 2 && "Successor # out of range for invoke!");
    *(&Op<-2>() + idx) = reinterpret_cast<Value*>(NewSucc);
  }

  unsigned getNumSuccessors() const { return 2; }

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return (I->getOpcode() == Instruction::Invoke);
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }

private:
  virtual BasicBlock *getSuccessorV(unsigned idx) const;
  virtual unsigned getNumSuccessorsV() const;
  virtual void setSuccessorV(unsigned idx, BasicBlock *B);

  // Shadow Instruction::setInstructionSubclassData with a private forwarding
  // method so that subclasses cannot accidentally use it.
  void setInstructionSubclassData(unsigned short D) {
    Instruction::setInstructionSubclassData(D);
  }
};

template <>
struct OperandTraits<InvokeInst> : public VariadicOperandTraits<InvokeInst, 3> {
};

InvokeInst::InvokeInst(Value *Func,
                       BasicBlock *IfNormal, BasicBlock *IfException,
                       ArrayRef<Value *> Args, unsigned Values,
                       const Twine &NameStr, Instruction *InsertBefore)
  : TerminatorInst(cast<FunctionType>(cast<PointerType>(Func->getType())
                                      ->getElementType())->getReturnType(),
                   Instruction::Invoke,
                   OperandTraits<InvokeInst>::op_end(this) - Values,
                   Values, InsertBefore) {
  init(Func, IfNormal, IfException, Args, NameStr);
}
InvokeInst::InvokeInst(Value *Func,
                       BasicBlock *IfNormal, BasicBlock *IfException,
                       ArrayRef<Value *> Args, unsigned Values,
                       const Twine &NameStr, BasicBlock *InsertAtEnd)
  : TerminatorInst(cast<FunctionType>(cast<PointerType>(Func->getType())
                                      ->getElementType())->getReturnType(),
                   Instruction::Invoke,
                   OperandTraits<InvokeInst>::op_end(this) - Values,
                   Values, InsertAtEnd) {
  init(Func, IfNormal, IfException, Args, NameStr);
}

DEFINE_TRANSPARENT_OPERAND_ACCESSORS(InvokeInst, Value)

//===----------------------------------------------------------------------===//
//                              ResumeInst Class
//===----------------------------------------------------------------------===//

//===---------------------------------------------------------------------------
/// ResumeInst - Resume the propagation of an exception.
///
class ResumeInst : public TerminatorInst {
  ResumeInst(const ResumeInst &RI);

  explicit ResumeInst(Value *Exn, Instruction *InsertBefore=0);
  ResumeInst(Value *Exn, BasicBlock *InsertAtEnd);
protected:
  virtual ResumeInst *clone_impl() const;
public:
  static ResumeInst *Create(Value *Exn, Instruction *InsertBefore = 0) {
    return new(1) ResumeInst(Exn, InsertBefore);
  }
  static ResumeInst *Create(Value *Exn, BasicBlock *InsertAtEnd) {
    return new(1) ResumeInst(Exn, InsertAtEnd);
  }

  /// Provide fast operand accessors
  DECLARE_TRANSPARENT_OPERAND_ACCESSORS(Value);

  /// Convenience accessor.
  Value *getValue() const { return Op<0>(); }

  unsigned getNumSuccessors() const { return 0; }

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == Instruction::Resume;
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
private:
  virtual BasicBlock *getSuccessorV(unsigned idx) const;
  virtual unsigned getNumSuccessorsV() const;
  virtual void setSuccessorV(unsigned idx, BasicBlock *B);
};

template <>
struct OperandTraits<ResumeInst> :
    public FixedNumOperandTraits<ResumeInst, 1> {
};

DEFINE_TRANSPARENT_OPERAND_ACCESSORS(ResumeInst, Value)

//===----------------------------------------------------------------------===//
//                           UnreachableInst Class
//===----------------------------------------------------------------------===//

//===---------------------------------------------------------------------------
/// UnreachableInst - This function has undefined behavior.  In particular, the
/// presence of this instruction indicates some higher level knowledge that the
/// end of the block cannot be reached.
///
class UnreachableInst : public TerminatorInst {
  void *operator new(size_t, unsigned) LLVM_DELETED_FUNCTION;
protected:
  virtual UnreachableInst *clone_impl() const;

public:
  // allocate space for exactly zero operands
  void *operator new(size_t s) {
    return User::operator new(s, 0);
  }
  explicit UnreachableInst(LLVMContext &C, Instruction *InsertBefore = 0);
  explicit UnreachableInst(LLVMContext &C, BasicBlock *InsertAtEnd);

  unsigned getNumSuccessors() const { return 0; }

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == Instruction::Unreachable;
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
private:
  virtual BasicBlock *getSuccessorV(unsigned idx) const;
  virtual unsigned getNumSuccessorsV() const;
  virtual void setSuccessorV(unsigned idx, BasicBlock *B);
};

//===----------------------------------------------------------------------===//
//                                 TruncInst Class
//===----------------------------------------------------------------------===//

/// \brief This class represents a truncation of integer types.
class TruncInst : public CastInst {
protected:
  /// \brief Clone an identical TruncInst
  virtual TruncInst *clone_impl() const;

public:
  /// \brief Constructor with insert-before-instruction semantics
  TruncInst(
    Value *S,                     ///< The value to be truncated
    Type *Ty,               ///< The (smaller) type to truncate to
    const Twine &NameStr = "",    ///< A name for the new instruction
    Instruction *InsertBefore = 0 ///< Where to insert the new instruction
  );

  /// \brief Constructor with insert-at-end-of-block semantics
  TruncInst(
    Value *S,                     ///< The value to be truncated
    Type *Ty,               ///< The (smaller) type to truncate to
    const Twine &NameStr,         ///< A name for the new instruction
    BasicBlock *InsertAtEnd       ///< The block to insert the instruction into
  );

  /// \brief Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == Trunc;
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
};

//===----------------------------------------------------------------------===//
//                                 ZExtInst Class
//===----------------------------------------------------------------------===//

/// \brief This class represents zero extension of integer types.
class ZExtInst : public CastInst {
protected:
  /// \brief Clone an identical ZExtInst
  virtual ZExtInst *clone_impl() const;

public:
  /// \brief Constructor with insert-before-instruction semantics
  ZExtInst(
    Value *S,                     ///< The value to be zero extended
    Type *Ty,               ///< The type to zero extend to
    const Twine &NameStr = "",    ///< A name for the new instruction
    Instruction *InsertBefore = 0 ///< Where to insert the new instruction
  );

  /// \brief Constructor with insert-at-end semantics.
  ZExtInst(
    Value *S,                     ///< The value to be zero extended
    Type *Ty,               ///< The type to zero extend to
    const Twine &NameStr,         ///< A name for the new instruction
    BasicBlock *InsertAtEnd       ///< The block to insert the instruction into
  );

  /// \brief Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == ZExt;
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
};

//===----------------------------------------------------------------------===//
//                                 SExtInst Class
//===----------------------------------------------------------------------===//

/// \brief This class represents a sign extension of integer types.
class SExtInst : public CastInst {
protected:
  /// \brief Clone an identical SExtInst
  virtual SExtInst *clone_impl() const;

public:
  /// \brief Constructor with insert-before-instruction semantics
  SExtInst(
    Value *S,                     ///< The value to be sign extended
    Type *Ty,               ///< The type to sign extend to
    const Twine &NameStr = "",    ///< A name for the new instruction
    Instruction *InsertBefore = 0 ///< Where to insert the new instruction
  );

  /// \brief Constructor with insert-at-end-of-block semantics
  SExtInst(
    Value *S,                     ///< The value to be sign extended
    Type *Ty,               ///< The type to sign extend to
    const Twine &NameStr,         ///< A name for the new instruction
    BasicBlock *InsertAtEnd       ///< The block to insert the instruction into
  );

  /// \brief Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == SExt;
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
};

//===----------------------------------------------------------------------===//
//                                 FPTruncInst Class
//===----------------------------------------------------------------------===//

/// \brief This class represents a truncation of floating point types.
class FPTruncInst : public CastInst {
protected:
  /// \brief Clone an identical FPTruncInst
  virtual FPTruncInst *clone_impl() const;

public:
  /// \brief Constructor with insert-before-instruction semantics
  FPTruncInst(
    Value *S,                     ///< The value to be truncated
    Type *Ty,               ///< The type to truncate to
    const Twine &NameStr = "",    ///< A name for the new instruction
    Instruction *InsertBefore = 0 ///< Where to insert the new instruction
  );

  /// \brief Constructor with insert-before-instruction semantics
  FPTruncInst(
    Value *S,                     ///< The value to be truncated
    Type *Ty,               ///< The type to truncate to
    const Twine &NameStr,         ///< A name for the new instruction
    BasicBlock *InsertAtEnd       ///< The block to insert the instruction into
  );

  /// \brief Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == FPTrunc;
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
};

//===----------------------------------------------------------------------===//
//                                 FPExtInst Class
//===----------------------------------------------------------------------===//

/// \brief This class represents an extension of floating point types.
class FPExtInst : public CastInst {
protected:
  /// \brief Clone an identical FPExtInst
  virtual FPExtInst *clone_impl() const;

public:
  /// \brief Constructor with insert-before-instruction semantics
  FPExtInst(
    Value *S,                     ///< The value to be extended
    Type *Ty,               ///< The type to extend to
    const Twine &NameStr = "",    ///< A name for the new instruction
    Instruction *InsertBefore = 0 ///< Where to insert the new instruction
  );

  /// \brief Constructor with insert-at-end-of-block semantics
  FPExtInst(
    Value *S,                     ///< The value to be extended
    Type *Ty,               ///< The type to extend to
    const Twine &NameStr,         ///< A name for the new instruction
    BasicBlock *InsertAtEnd       ///< The block to insert the instruction into
  );

  /// \brief Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == FPExt;
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
};

//===----------------------------------------------------------------------===//
//                                 UIToFPInst Class
//===----------------------------------------------------------------------===//

/// \brief This class represents a cast unsigned integer to floating point.
class UIToFPInst : public CastInst {
protected:
  /// \brief Clone an identical UIToFPInst
  virtual UIToFPInst *clone_impl() const;

public:
  /// \brief Constructor with insert-before-instruction semantics
  UIToFPInst(
    Value *S,                     ///< The value to be converted
    Type *Ty,               ///< The type to convert to
    const Twine &NameStr = "",    ///< A name for the new instruction
    Instruction *InsertBefore = 0 ///< Where to insert the new instruction
  );

  /// \brief Constructor with insert-at-end-of-block semantics
  UIToFPInst(
    Value *S,                     ///< The value to be converted
    Type *Ty,               ///< The type to convert to
    const Twine &NameStr,         ///< A name for the new instruction
    BasicBlock *InsertAtEnd       ///< The block to insert the instruction into
  );

  /// \brief Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == UIToFP;
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
};

//===----------------------------------------------------------------------===//
//                                 SIToFPInst Class
//===----------------------------------------------------------------------===//

/// \brief This class represents a cast from signed integer to floating point.
class SIToFPInst : public CastInst {
protected:
  /// \brief Clone an identical SIToFPInst
  virtual SIToFPInst *clone_impl() const;

public:
  /// \brief Constructor with insert-before-instruction semantics
  SIToFPInst(
    Value *S,                     ///< The value to be converted
    Type *Ty,               ///< The type to convert to
    const Twine &NameStr = "",    ///< A name for the new instruction
    Instruction *InsertBefore = 0 ///< Where to insert the new instruction
  );

  /// \brief Constructor with insert-at-end-of-block semantics
  SIToFPInst(
    Value *S,                     ///< The value to be converted
    Type *Ty,               ///< The type to convert to
    const Twine &NameStr,         ///< A name for the new instruction
    BasicBlock *InsertAtEnd       ///< The block to insert the instruction into
  );

  /// \brief Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == SIToFP;
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
};

//===----------------------------------------------------------------------===//
//                                 FPToUIInst Class
//===----------------------------------------------------------------------===//

/// \brief This class represents a cast from floating point to unsigned integer
class FPToUIInst  : public CastInst {
protected:
  /// \brief Clone an identical FPToUIInst
  virtual FPToUIInst *clone_impl() const;

public:
  /// \brief Constructor with insert-before-instruction semantics
  FPToUIInst(
    Value *S,                     ///< The value to be converted
    Type *Ty,               ///< The type to convert to
    const Twine &NameStr = "",    ///< A name for the new instruction
    Instruction *InsertBefore = 0 ///< Where to insert the new instruction
  );

  /// \brief Constructor with insert-at-end-of-block semantics
  FPToUIInst(
    Value *S,                     ///< The value to be converted
    Type *Ty,               ///< The type to convert to
    const Twine &NameStr,         ///< A name for the new instruction
    BasicBlock *InsertAtEnd       ///< Where to insert the new instruction
  );

  /// \brief Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == FPToUI;
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
};

//===----------------------------------------------------------------------===//
//                                 FPToSIInst Class
//===----------------------------------------------------------------------===//

/// \brief This class represents a cast from floating point to signed integer.
class FPToSIInst  : public CastInst {
protected:
  /// \brief Clone an identical FPToSIInst
  virtual FPToSIInst *clone_impl() const;

public:
  /// \brief Constructor with insert-before-instruction semantics
  FPToSIInst(
    Value *S,                     ///< The value to be converted
    Type *Ty,               ///< The type to convert to
    const Twine &NameStr = "",    ///< A name for the new instruction
    Instruction *InsertBefore = 0 ///< Where to insert the new instruction
  );

  /// \brief Constructor with insert-at-end-of-block semantics
  FPToSIInst(
    Value *S,                     ///< The value to be converted
    Type *Ty,               ///< The type to convert to
    const Twine &NameStr,         ///< A name for the new instruction
    BasicBlock *InsertAtEnd       ///< The block to insert the instruction into
  );

  /// \brief Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == FPToSI;
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
};

//===----------------------------------------------------------------------===//
//                                 IntToPtrInst Class
//===----------------------------------------------------------------------===//

/// \brief This class represents a cast from an integer to a pointer.
class IntToPtrInst : public CastInst {
public:
  /// \brief Constructor with insert-before-instruction semantics
  IntToPtrInst(
    Value *S,                     ///< The value to be converted
    Type *Ty,               ///< The type to convert to
    const Twine &NameStr = "",    ///< A name for the new instruction
    Instruction *InsertBefore = 0 ///< Where to insert the new instruction
  );

  /// \brief Constructor with insert-at-end-of-block semantics
  IntToPtrInst(
    Value *S,                     ///< The value to be converted
    Type *Ty,               ///< The type to convert to
    const Twine &NameStr,         ///< A name for the new instruction
    BasicBlock *InsertAtEnd       ///< The block to insert the instruction into
  );

  /// \brief Clone an identical IntToPtrInst
  virtual IntToPtrInst *clone_impl() const;

  /// \brief Returns the address space of this instruction's pointer type.
  unsigned getAddressSpace() const {
    return getType()->getPointerAddressSpace();
  }

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == IntToPtr;
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
};

//===----------------------------------------------------------------------===//
//                                 PtrToIntInst Class
//===----------------------------------------------------------------------===//

/// \brief This class represents a cast from a pointer to an integer
class PtrToIntInst : public CastInst {
protected:
  /// \brief Clone an identical PtrToIntInst
  virtual PtrToIntInst *clone_impl() const;

public:
  /// \brief Constructor with insert-before-instruction semantics
  PtrToIntInst(
    Value *S,                     ///< The value to be converted
    Type *Ty,               ///< The type to convert to
    const Twine &NameStr = "",    ///< A name for the new instruction
    Instruction *InsertBefore = 0 ///< Where to insert the new instruction
  );

  /// \brief Constructor with insert-at-end-of-block semantics
  PtrToIntInst(
    Value *S,                     ///< The value to be converted
    Type *Ty,               ///< The type to convert to
    const Twine &NameStr,         ///< A name for the new instruction
    BasicBlock *InsertAtEnd       ///< The block to insert the instruction into
  );

  /// \brief Gets the pointer operand.
  Value *getPointerOperand() { return getOperand(0); }
  /// \brief Gets the pointer operand.
  const Value *getPointerOperand() const { return getOperand(0); }
  /// \brief Gets the operand index of the pointer operand.
  static unsigned getPointerOperandIndex() { return 0U; }

  /// \brief Returns the address space of the pointer operand.
  unsigned getPointerAddressSpace() const {
    return getPointerOperand()->getType()->getPointerAddressSpace();
  }

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == PtrToInt;
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
};

//===----------------------------------------------------------------------===//
//                             BitCastInst Class
//===----------------------------------------------------------------------===//

/// \brief This class represents a no-op cast from one type to another.
class BitCastInst : public CastInst {
protected:
  /// \brief Clone an identical BitCastInst
  virtual BitCastInst *clone_impl() const;

public:
  /// \brief Constructor with insert-before-instruction semantics
  BitCastInst(
    Value *S,                     ///< The value to be casted
    Type *Ty,               ///< The type to casted to
    const Twine &NameStr = "",    ///< A name for the new instruction
    Instruction *InsertBefore = 0 ///< Where to insert the new instruction
  );

  /// \brief Constructor with insert-at-end-of-block semantics
  BitCastInst(
    Value *S,                     ///< The value to be casted
    Type *Ty,               ///< The type to casted to
    const Twine &NameStr,         ///< A name for the new instruction
    BasicBlock *InsertAtEnd       ///< The block to insert the instruction into
  );

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == BitCast;
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
};

} // End llvm namespace

#endif
