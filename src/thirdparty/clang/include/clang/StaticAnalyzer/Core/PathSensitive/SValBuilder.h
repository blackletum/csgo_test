﻿// SValBuilder.h - Construction of SVals from evaluating expressions -*- C++ -*-
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines SValBuilder, a class that defines the interface for
//  "symbolical evaluators" which construct an SVal from an expression.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_GR_SVALBUILDER
#define LLVM_CLANG_GR_SVALBUILDER

#include "clang/AST/ASTContext.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprObjC.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/BasicValueFactory.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/MemRegion.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/SVals.h"

namespace clang {

class CXXBoolLiteralExpr;

namespace ento {

class SValBuilder {
  virtual void anchor();
protected:
  ASTContext &Context;
  
  /// Manager of APSInt values.
  BasicValueFactory BasicVals;

  /// Manages the creation of symbols.
  SymbolManager SymMgr;

  /// Manages the creation of memory regions.
  MemRegionManager MemMgr;

  ProgramStateManager &StateMgr;

  /// The scalar type to use for array indices.
  const QualType ArrayIndexTy;
  
  /// The width of the scalar type used for array indices.
  const unsigned ArrayIndexWidth;

  virtual SVal evalCastFromNonLoc(NonLoc val, QualType castTy) = 0;
  virtual SVal evalCastFromLoc(Loc val, QualType castTy) = 0;

public:
  // FIXME: Make these protected again once RegionStoreManager correctly
  // handles loads from different bound value types.
  virtual SVal dispatchCast(SVal val, QualType castTy) = 0;

public:
  SValBuilder(llvm::BumpPtrAllocator &alloc, ASTContext &context,
              ProgramStateManager &stateMgr)
    : Context(context), BasicVals(context, alloc),
      SymMgr(context, BasicVals, alloc),
      MemMgr(context, alloc),
      StateMgr(stateMgr),
      ArrayIndexTy(context.IntTy),
      ArrayIndexWidth(context.getTypeSize(ArrayIndexTy)) {}

  virtual ~SValBuilder() {}

  bool haveSameType(const SymExpr *Sym1, const SymExpr *Sym2) {
    return haveSameType(Sym1->getType(), Sym2->getType());
  }

  bool haveSameType(QualType Ty1, QualType Ty2) {
    // FIXME: Remove the second disjunct when we support symbolic
    // truncation/extension.
    return (Context.getCanonicalType(Ty1) == Context.getCanonicalType(Ty2) ||
            (Ty1->isIntegralOrEnumerationType() &&
             Ty2->isIntegralOrEnumerationType()));
  }

  SVal evalCast(SVal val, QualType castTy, QualType originalType);
  
  virtual SVal evalMinus(NonLoc val) = 0;

  virtual SVal evalComplement(NonLoc val) = 0;

  /// Create a new value which represents a binary expression with two non
  /// location operands.
  virtual SVal evalBinOpNN(ProgramStateRef state, BinaryOperator::Opcode op,
                           NonLoc lhs, NonLoc rhs, QualType resultTy) = 0;

  /// Create a new value which represents a binary expression with two memory
  /// location operands.
  virtual SVal evalBinOpLL(ProgramStateRef state, BinaryOperator::Opcode op,
                           Loc lhs, Loc rhs, QualType resultTy) = 0;

  /// Create a new value which represents a binary expression with a memory
  /// location and non location operands. For example, this would be used to
  /// evaluate a pointer arithmetic operation.
  virtual SVal evalBinOpLN(ProgramStateRef state, BinaryOperator::Opcode op,
                           Loc lhs, NonLoc rhs, QualType resultTy) = 0;

  /// Evaluates a given SVal. If the SVal has only one possible (integer) value,
  /// that value is returned. Otherwise, returns NULL.
  virtual const llvm::APSInt *getKnownValue(ProgramStateRef state, SVal val) = 0;
  
  /// Constructs a symbolic expression for two non-location values.
  SVal makeSymExprValNN(ProgramStateRef state, BinaryOperator::Opcode op,
                      NonLoc lhs, NonLoc rhs, QualType resultTy);

  SVal evalBinOp(ProgramStateRef state, BinaryOperator::Opcode op,
                 SVal lhs, SVal rhs, QualType type);
  
  DefinedOrUnknownSVal evalEQ(ProgramStateRef state, DefinedOrUnknownSVal lhs,
                              DefinedOrUnknownSVal rhs);

  ASTContext &getContext() { return Context; }
  const ASTContext &getContext() const { return Context; }

  ProgramStateManager &getStateManager() { return StateMgr; }
  
  QualType getConditionType() const {
    return Context.getLangOpts().CPlusPlus ? Context.BoolTy : Context.IntTy;
  }
  
  QualType getArrayIndexType() const {
    return ArrayIndexTy;
  }

  BasicValueFactory &getBasicValueFactory() { return BasicVals; }
  const BasicValueFactory &getBasicValueFactory() const { return BasicVals; }

  SymbolManager &getSymbolManager() { return SymMgr; }
  const SymbolManager &getSymbolManager() const { return SymMgr; }

  MemRegionManager &getRegionManager() { return MemMgr; }
  const MemRegionManager &getRegionManager() const { return MemMgr; }

  // Forwarding methods to SymbolManager.

  const SymbolConjured* conjureSymbol(const Stmt *stmt,
                                      const LocationContext *LCtx,
                                      QualType type,
                                      unsigned visitCount,
                                      const void *symbolTag = 0) {
    return SymMgr.conjureSymbol(stmt, LCtx, type, visitCount, symbolTag);
  }

  const SymbolConjured* conjureSymbol(const Expr *expr,
                                      const LocationContext *LCtx,
                                      unsigned visitCount,
                                      const void *symbolTag = 0) {
    return SymMgr.conjureSymbol(expr, LCtx, visitCount, symbolTag);
  }

  /// Construct an SVal representing '0' for the specified type.
  DefinedOrUnknownSVal makeZeroVal(QualType type);

  /// Make a unique symbol for value of region.
  DefinedOrUnknownSVal getRegionValueSymbolVal(const TypedValueRegion *region);

  /// \brief Create a new symbol with a unique 'name'.
  ///
  /// We resort to conjured symbols when we cannot construct a derived symbol.
  /// The advantage of symbols derived/built from other symbols is that we
  /// preserve the relation between related(or even equivalent) expressions, so
  /// conjured symbols should be used sparingly.
  DefinedOrUnknownSVal conjureSymbolVal(const void *symbolTag,
                                        const Expr *expr,
                                        const LocationContext *LCtx,
                                        unsigned count);
  DefinedOrUnknownSVal conjureSymbolVal(const void *symbolTag,
                                        const Expr *expr,
                                        const LocationContext *LCtx,
                                        QualType type,
                                        unsigned count);
  
  DefinedOrUnknownSVal conjureSymbolVal(const Stmt *stmt,
                                        const LocationContext *LCtx,
                                        QualType type,
                                        unsigned visitCount);
  /// \brief Conjure a symbol representing heap allocated memory region.
  ///
  /// Note, the expression should represent a location.
  DefinedOrUnknownSVal getConjuredHeapSymbolVal(const Expr *E,
                                                const LocationContext *LCtx,
                                                unsigned Count);

  DefinedOrUnknownSVal getDerivedRegionValueSymbolVal(
      SymbolRef parentSymbol, const TypedValueRegion *region);

  DefinedSVal getMetadataSymbolVal(
      const void *symbolTag, const MemRegion *region,
      const Expr *expr, QualType type, unsigned count);

  DefinedSVal getFunctionPointer(const FunctionDecl *func);
  
  DefinedSVal getBlockPointer(const BlockDecl *block, CanQualType locTy,
                              const LocationContext *locContext);

  NonLoc makeCompoundVal(QualType type, llvm::ImmutableList<SVal> vals) {
    return nonloc::CompoundVal(BasicVals.getCompoundValData(type, vals));
  }

  NonLoc makeLazyCompoundVal(const StoreRef &store, 
                             const TypedValueRegion *region) {
    return nonloc::LazyCompoundVal(
        BasicVals.getLazyCompoundValData(store, region));
  }

  NonLoc makeZeroArrayIndex() {
    return nonloc::ConcreteInt(BasicVals.getValue(0, ArrayIndexTy));
  }

  NonLoc makeArrayIndex(uint64_t idx) {
    return nonloc::ConcreteInt(BasicVals.getValue(idx, ArrayIndexTy));
  }

  SVal convertToArrayIndex(SVal val);

  nonloc::ConcreteInt makeIntVal(const IntegerLiteral* integer) {
    return nonloc::ConcreteInt(
        BasicVals.getValue(integer->getValue(),
                     integer->getType()->isUnsignedIntegerOrEnumerationType()));
  }

  nonloc::ConcreteInt makeBoolVal(const ObjCBoolLiteralExpr *boolean) {
    return makeTruthVal(boolean->getValue(), boolean->getType());
  }

  nonloc::ConcreteInt makeBoolVal(const CXXBoolLiteralExpr *boolean);

  nonloc::ConcreteInt makeIntVal(const llvm::APSInt& integer) {
    return nonloc::ConcreteInt(BasicVals.getValue(integer));
  }

  loc::ConcreteInt makeIntLocVal(const llvm::APSInt &integer) {
    return loc::ConcreteInt(BasicVals.getValue(integer));
  }

  NonLoc makeIntVal(const llvm::APInt& integer, bool isUnsigned) {
    return nonloc::ConcreteInt(BasicVals.getValue(integer, isUnsigned));
  }

  DefinedSVal makeIntVal(uint64_t integer, QualType type) {
    if (Loc::isLocType(type))
      return loc::ConcreteInt(BasicVals.getValue(integer, type));

    return nonloc::ConcreteInt(BasicVals.getValue(integer, type));
  }

  NonLoc makeIntVal(uint64_t integer, bool isUnsigned) {
    return nonloc::ConcreteInt(BasicVals.getIntValue(integer, isUnsigned));
  }

  NonLoc makeIntValWithPtrWidth(uint64_t integer, bool isUnsigned) {
    return nonloc::ConcreteInt(
        BasicVals.getIntWithPtrWidth(integer, isUnsigned));
  }

  NonLoc makeLocAsInteger(Loc loc, unsigned bits) {
    return nonloc::LocAsInteger(BasicVals.getPersistentSValWithData(loc, bits));
  }

  NonLoc makeNonLoc(const SymExpr *lhs, BinaryOperator::Opcode op,
                    const llvm::APSInt& rhs, QualType type);

  NonLoc makeNonLoc(const llvm::APSInt& rhs, BinaryOperator::Opcode op,
                    const SymExpr *lhs, QualType type);

  NonLoc makeNonLoc(const SymExpr *lhs, BinaryOperator::Opcode op,
                    const SymExpr *rhs, QualType type);

  /// \brief Create a NonLoc value for cast.
  NonLoc makeNonLoc(const SymExpr *operand, QualType fromTy, QualType toTy);

  nonloc::ConcreteInt makeTruthVal(bool b, QualType type) {
    return nonloc::ConcreteInt(BasicVals.getTruthValue(b, type));
  }

  nonloc::ConcreteInt makeTruthVal(bool b) {
    return nonloc::ConcreteInt(BasicVals.getTruthValue(b));
  }

  Loc makeNull() {
    return loc::ConcreteInt(BasicVals.getZeroWithPtrWidth());
  }

  Loc makeLoc(SymbolRef sym) {
    return loc::MemRegionVal(MemMgr.getSymbolicRegion(sym));
  }

  Loc makeLoc(const MemRegion* region) {
    return loc::MemRegionVal(region);
  }

  Loc makeLoc(const AddrLabelExpr *expr) {
    return loc::GotoLabel(expr->getLabel());
  }

  Loc makeLoc(const llvm::APSInt& integer) {
    return loc::ConcreteInt(BasicVals.getValue(integer));
  }

  /// Return a memory region for the 'this' object reference.
  loc::MemRegionVal getCXXThis(const CXXMethodDecl *D,
                               const StackFrameContext *SFC);

  /// Return a memory region for the 'this' object reference.
  loc::MemRegionVal getCXXThis(const CXXRecordDecl *D,
                               const StackFrameContext *SFC);
};

SValBuilder* createSimpleSValBuilder(llvm::BumpPtrAllocator &alloc,
                                     ASTContext &context,
                                     ProgramStateManager &stateMgr);

} // end GR namespace

} // end clang namespace

#endif
