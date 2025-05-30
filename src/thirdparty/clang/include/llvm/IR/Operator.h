﻿//===-- llvm/Operator.h - Operator utility subclass -------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines various classes for working with Instructions and
// ConstantExprs.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_IR_OPERATOR_H
#define LLVM_IR_OPERATOR_H

#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/GetElementPtrTypeIterator.h"

namespace llvm {

class GetElementPtrInst;
class BinaryOperator;
class ConstantExpr;

/// Operator - This is a utility class that provides an abstraction for the
/// common functionality between Instructions and ConstantExprs.
///
class Operator : public User {
private:
  // The Operator class is intended to be used as a utility, and is never itself
  // instantiated.
  void *operator new(size_t, unsigned) LLVM_DELETED_FUNCTION;
  void *operator new(size_t s) LLVM_DELETED_FUNCTION;
  Operator() LLVM_DELETED_FUNCTION;

protected:
  // NOTE: Cannot use LLVM_DELETED_FUNCTION because it's not legal to delete
  // an overridden method that's not deleted in the base class. Cannot leave
  // this unimplemented because that leads to an ODR-violation.
  ~Operator();

public:
  /// getOpcode - Return the opcode for this Instruction or ConstantExpr.
  ///
  unsigned getOpcode() const {
    if (const Instruction *I = dyn_cast<Instruction>(this))
      return I->getOpcode();
    return cast<ConstantExpr>(this)->getOpcode();
  }

  /// getOpcode - If V is an Instruction or ConstantExpr, return its
  /// opcode. Otherwise return UserOp1.
  ///
  static unsigned getOpcode(const Value *V) {
    if (const Instruction *I = dyn_cast<Instruction>(V))
      return I->getOpcode();
    if (const ConstantExpr *CE = dyn_cast<ConstantExpr>(V))
      return CE->getOpcode();
    return Instruction::UserOp1;
  }

  static inline bool classof(const Instruction *) { return true; }
  static inline bool classof(const ConstantExpr *) { return true; }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) || isa<ConstantExpr>(V);
  }
};

/// OverflowingBinaryOperator - Utility class for integer arithmetic operators
/// which may exhibit overflow - Add, Sub, and Mul. It does not include SDiv,
/// despite that operator having the potential for overflow.
///
class OverflowingBinaryOperator : public Operator {
public:
  enum {
    NoUnsignedWrap = (1 << 0),
    NoSignedWrap   = (1 << 1)
  };

private:
  friend class BinaryOperator;
  friend class ConstantExpr;
  void setHasNoUnsignedWrap(bool B) {
    SubclassOptionalData =
      (SubclassOptionalData & ~NoUnsignedWrap) | (B * NoUnsignedWrap);
  }
  void setHasNoSignedWrap(bool B) {
    SubclassOptionalData =
      (SubclassOptionalData & ~NoSignedWrap) | (B * NoSignedWrap);
  }

public:
  /// hasNoUnsignedWrap - Test whether this operation is known to never
  /// undergo unsigned overflow, aka the nuw property.
  bool hasNoUnsignedWrap() const {
    return SubclassOptionalData & NoUnsignedWrap;
  }

  /// hasNoSignedWrap - Test whether this operation is known to never
  /// undergo signed overflow, aka the nsw property.
  bool hasNoSignedWrap() const {
    return (SubclassOptionalData & NoSignedWrap) != 0;
  }

  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == Instruction::Add ||
           I->getOpcode() == Instruction::Sub ||
           I->getOpcode() == Instruction::Mul ||
           I->getOpcode() == Instruction::Shl;
  }
  static inline bool classof(const ConstantExpr *CE) {
    return CE->getOpcode() == Instruction::Add ||
           CE->getOpcode() == Instruction::Sub ||
           CE->getOpcode() == Instruction::Mul ||
           CE->getOpcode() == Instruction::Shl;
  }
  static inline bool classof(const Value *V) {
    return (isa<Instruction>(V) && classof(cast<Instruction>(V))) ||
           (isa<ConstantExpr>(V) && classof(cast<ConstantExpr>(V)));
  }
};

/// PossiblyExactOperator - A udiv or sdiv instruction, which can be marked as
/// "exact", indicating that no bits are destroyed.
class PossiblyExactOperator : public Operator {
public:
  enum {
    IsExact = (1 << 0)
  };

private:
  friend class BinaryOperator;
  friend class ConstantExpr;
  void setIsExact(bool B) {
    SubclassOptionalData = (SubclassOptionalData & ~IsExact) | (B * IsExact);
  }

public:
  /// isExact - Test whether this division is known to be exact, with
  /// zero remainder.
  bool isExact() const {
    return SubclassOptionalData & IsExact;
  }

  static bool isPossiblyExactOpcode(unsigned OpC) {
    return OpC == Instruction::SDiv ||
           OpC == Instruction::UDiv ||
           OpC == Instruction::AShr ||
           OpC == Instruction::LShr;
  }
  static inline bool classof(const ConstantExpr *CE) {
    return isPossiblyExactOpcode(CE->getOpcode());
  }
  static inline bool classof(const Instruction *I) {
    return isPossiblyExactOpcode(I->getOpcode());
  }
  static inline bool classof(const Value *V) {
    return (isa<Instruction>(V) && classof(cast<Instruction>(V))) ||
           (isa<ConstantExpr>(V) && classof(cast<ConstantExpr>(V)));
  }
};

/// Convenience struct for specifying and reasoning about fast-math flags.
class FastMathFlags {
private:
  friend class FPMathOperator;
  unsigned Flags;
  FastMathFlags(unsigned F) : Flags(F) { }

public:
  enum {
    UnsafeAlgebra   = (1 << 0),
    NoNaNs          = (1 << 1),
    NoInfs          = (1 << 2),
    NoSignedZeros   = (1 << 3),
    AllowReciprocal = (1 << 4)
  };

  FastMathFlags() : Flags(0)
  { }

  /// Whether any flag is set
  bool any() { return Flags != 0; }

  /// Set all the flags to false
  void clear() { Flags = 0; }

  /// Flag queries
  bool noNaNs()          { return 0 != (Flags & NoNaNs); }
  bool noInfs()          { return 0 != (Flags & NoInfs); }
  bool noSignedZeros()   { return 0 != (Flags & NoSignedZeros); }
  bool allowReciprocal() { return 0 != (Flags & AllowReciprocal); }
  bool unsafeAlgebra()   { return 0 != (Flags & UnsafeAlgebra); }

  /// Flag setters
  void setNoNaNs()          { Flags |= NoNaNs; }
  void setNoInfs()          { Flags |= NoInfs; }
  void setNoSignedZeros()   { Flags |= NoSignedZeros; }
  void setAllowReciprocal() { Flags |= AllowReciprocal; }
  void setUnsafeAlgebra() {
    Flags |= UnsafeAlgebra;
    setNoNaNs();
    setNoInfs();
    setNoSignedZeros();
    setAllowReciprocal();
  }
};


/// FPMathOperator - Utility class for floating point operations which can have
/// information about relaxed accuracy requirements attached to them.
class FPMathOperator : public Operator {
private:
  friend class Instruction;

  void setHasUnsafeAlgebra(bool B) {
    SubclassOptionalData =
      (SubclassOptionalData & ~FastMathFlags::UnsafeAlgebra) |
      (B * FastMathFlags::UnsafeAlgebra);

    // Unsafe algebra implies all the others
    if (B) {
      setHasNoNaNs(true);
      setHasNoInfs(true);
      setHasNoSignedZeros(true);
      setHasAllowReciprocal(true);
    }
  }
  void setHasNoNaNs(bool B) {
    SubclassOptionalData =
      (SubclassOptionalData & ~FastMathFlags::NoNaNs) |
      (B * FastMathFlags::NoNaNs);
  }
  void setHasNoInfs(bool B) {
    SubclassOptionalData =
      (SubclassOptionalData & ~FastMathFlags::NoInfs) |
      (B * FastMathFlags::NoInfs);
  }
  void setHasNoSignedZeros(bool B) {
    SubclassOptionalData =
      (SubclassOptionalData & ~FastMathFlags::NoSignedZeros) |
      (B * FastMathFlags::NoSignedZeros);
  }
  void setHasAllowReciprocal(bool B) {
    SubclassOptionalData =
      (SubclassOptionalData & ~FastMathFlags::AllowReciprocal) |
      (B * FastMathFlags::AllowReciprocal);
  }

  /// Convenience function for setting all the fast-math flags
  void setFastMathFlags(FastMathFlags FMF) {
    SubclassOptionalData |= FMF.Flags;
  }

public:
  /// Test whether this operation is permitted to be
  /// algebraically transformed, aka the 'A' fast-math property.
  bool hasUnsafeAlgebra() const {
    return (SubclassOptionalData & FastMathFlags::UnsafeAlgebra) != 0;
  }

  /// Test whether this operation's arguments and results are to be
  /// treated as non-NaN, aka the 'N' fast-math property.
  bool hasNoNaNs() const {
    return (SubclassOptionalData & FastMathFlags::NoNaNs) != 0;
  }

  /// Test whether this operation's arguments and results are to be
  /// treated as NoN-Inf, aka the 'I' fast-math property.
  bool hasNoInfs() const {
    return (SubclassOptionalData & FastMathFlags::NoInfs) != 0;
  }

  /// Test whether this operation can treat the sign of zero
  /// as insignificant, aka the 'S' fast-math property.
  bool hasNoSignedZeros() const {
    return (SubclassOptionalData & FastMathFlags::NoSignedZeros) != 0;
  }

  /// Test whether this operation is permitted to use
  /// reciprocal instead of division, aka the 'R' fast-math property.
  bool hasAllowReciprocal() const {
    return (SubclassOptionalData & FastMathFlags::AllowReciprocal) != 0;
  }

  /// Convenience function for getting all the fast-math flags
  FastMathFlags getFastMathFlags() const {
    return FastMathFlags(SubclassOptionalData);
  }

  /// \brief Get the maximum error permitted by this operation in ULPs.  An
  /// accuracy of 0.0 means that the operation should be performed with the
  /// default precision.
  float getFPAccuracy() const;

  static inline bool classof(const Instruction *I) {
    return I->getType()->isFPOrFPVectorTy();
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
};


/// ConcreteOperator - A helper template for defining operators for individual
/// opcodes.
template<typename SuperClass, unsigned Opc>
class ConcreteOperator : public SuperClass {
public:
  static inline bool classof(const Instruction *I) {
    return I->getOpcode() == Opc;
  }
  static inline bool classof(const ConstantExpr *CE) {
    return CE->getOpcode() == Opc;
  }
  static inline bool classof(const Value *V) {
    return (isa<Instruction>(V) && classof(cast<Instruction>(V))) ||
           (isa<ConstantExpr>(V) && classof(cast<ConstantExpr>(V)));
  }
};

class AddOperator
  : public ConcreteOperator<OverflowingBinaryOperator, Instruction::Add> {
};
class SubOperator
  : public ConcreteOperator<OverflowingBinaryOperator, Instruction::Sub> {
};
class MulOperator
  : public ConcreteOperator<OverflowingBinaryOperator, Instruction::Mul> {
};
class ShlOperator
  : public ConcreteOperator<OverflowingBinaryOperator, Instruction::Shl> {
};


class SDivOperator
  : public ConcreteOperator<PossiblyExactOperator, Instruction::SDiv> {
};
class UDivOperator
  : public ConcreteOperator<PossiblyExactOperator, Instruction::UDiv> {
};
class AShrOperator
  : public ConcreteOperator<PossiblyExactOperator, Instruction::AShr> {
};
class LShrOperator
  : public ConcreteOperator<PossiblyExactOperator, Instruction::LShr> {
};



class GEPOperator
  : public ConcreteOperator<Operator, Instruction::GetElementPtr> {
  enum {
    IsInBounds = (1 << 0)
  };

  friend class GetElementPtrInst;
  friend class ConstantExpr;
  void setIsInBounds(bool B) {
    SubclassOptionalData =
      (SubclassOptionalData & ~IsInBounds) | (B * IsInBounds);
  }

public:
  /// isInBounds - Test whether this is an inbounds GEP, as defined
  /// by LangRef.html.
  bool isInBounds() const {
    return SubclassOptionalData & IsInBounds;
  }

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
    return 0U;                      // get index for modifying correct operand
  }

  /// getPointerOperandType - Method to return the pointer operand as a
  /// PointerType.
  Type *getPointerOperandType() const {
    return getPointerOperand()->getType();
  }

  /// getPointerAddressSpace - Method to return the address space of the
  /// pointer operand.
  unsigned getPointerAddressSpace() const {
    return cast<PointerType>(getPointerOperandType())->getAddressSpace();
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
  bool hasAllZeroIndices() const {
    for (const_op_iterator I = idx_begin(), E = idx_end(); I != E; ++I) {
      if (ConstantInt *C = dyn_cast<ConstantInt>(I))
        if (C->isZero())
          continue;
      return false;
    }
    return true;
  }

  /// hasAllConstantIndices - Return true if all of the indices of this GEP are
  /// constant integers.  If so, the result pointer and the first operand have
  /// a constant offset between them.
  bool hasAllConstantIndices() const {
    for (const_op_iterator I = idx_begin(), E = idx_end(); I != E; ++I) {
      if (!isa<ConstantInt>(I))
        return false;
    }
    return true;
  }

  /// \brief Accumulate the constant address offset of this GEP if possible.
  ///
  /// This routine accepts an APInt into which it will accumulate the constant
  /// offset of this GEP if the GEP is in fact constant. If the GEP is not
  /// all-constant, it returns false and the value of the offset APInt is
  /// undefined (it is *not* preserved!). The APInt passed into this routine
  /// must be at least as wide as the IntPtr type for the address space of
  /// the base GEP pointer.
  bool accumulateConstantOffset(const DataLayout &DL, APInt &Offset) const {
    assert(Offset.getBitWidth() ==
           DL.getPointerSizeInBits(getPointerAddressSpace()) &&
           "The offset must have exactly as many bits as our pointer.");

    for (gep_type_iterator GTI = gep_type_begin(this), GTE = gep_type_end(this);
         GTI != GTE; ++GTI) {
      ConstantInt *OpC = dyn_cast<ConstantInt>(GTI.getOperand());
      if (!OpC)
        return false;
      if (OpC->isZero())
        continue;

      // Handle a struct index, which adds its field offset to the pointer.
      if (StructType *STy = dyn_cast<StructType>(*GTI)) {
        unsigned ElementIdx = OpC->getZExtValue();
        const StructLayout *SL = DL.getStructLayout(STy);
        Offset += APInt(Offset.getBitWidth(),
                        SL->getElementOffset(ElementIdx));
        continue;
      }

      // For array or vector indices, scale the index by the size of the type.
      APInt Index = OpC->getValue().sextOrTrunc(Offset.getBitWidth());
      Offset += Index * APInt(Offset.getBitWidth(),
                              DL.getTypeAllocSize(GTI.getIndexedType()));
    }
    return true;
  }

};

} // End llvm namespace

#endif
