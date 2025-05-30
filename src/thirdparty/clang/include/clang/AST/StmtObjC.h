﻿//===--- StmtObjC.h - Classes for representing ObjC statements --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

/// \file
/// \brief Defines the Objective-C statement AST node classes.

#ifndef LLVM_CLANG_AST_STMTOBJC_H
#define LLVM_CLANG_AST_STMTOBJC_H

#include "clang/AST/Stmt.h"
#include "llvm/Support/Compiler.h"

namespace clang {

/// \brief Represents Objective-C's collection statement.
///
/// This is represented as 'for (element 'in' collection-expression)' stmt.
class ObjCForCollectionStmt : public Stmt {
  enum { ELEM, COLLECTION, BODY, END_EXPR };
  Stmt* SubExprs[END_EXPR]; // SubExprs[ELEM] is an expression or declstmt.
  SourceLocation ForLoc;
  SourceLocation RParenLoc;
public:
  ObjCForCollectionStmt(Stmt *Elem, Expr *Collect, Stmt *Body,
                        SourceLocation FCL, SourceLocation RPL);
  explicit ObjCForCollectionStmt(EmptyShell Empty) :
    Stmt(ObjCForCollectionStmtClass, Empty) { }

  Stmt *getElement() { return SubExprs[ELEM]; }
  Expr *getCollection() {
    return reinterpret_cast<Expr*>(SubExprs[COLLECTION]);
  }
  Stmt *getBody() { return SubExprs[BODY]; }

  const Stmt *getElement() const { return SubExprs[ELEM]; }
  const Expr *getCollection() const {
    return reinterpret_cast<Expr*>(SubExprs[COLLECTION]);
  }
  const Stmt *getBody() const { return SubExprs[BODY]; }

  void setElement(Stmt *S) { SubExprs[ELEM] = S; }
  void setCollection(Expr *E) {
    SubExprs[COLLECTION] = reinterpret_cast<Stmt*>(E);
  }
  void setBody(Stmt *S) { SubExprs[BODY] = S; }

  SourceLocation getForLoc() const { return ForLoc; }
  void setForLoc(SourceLocation Loc) { ForLoc = Loc; }
  SourceLocation getRParenLoc() const { return RParenLoc; }
  void setRParenLoc(SourceLocation Loc) { RParenLoc = Loc; }

  SourceLocation getLocStart() const LLVM_READONLY { return ForLoc; }
  SourceLocation getLocEnd() const LLVM_READONLY {
    return SubExprs[BODY]->getLocEnd();
  }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == ObjCForCollectionStmtClass;
  }

  // Iterators
  child_range children() {
    return child_range(&SubExprs[0], &SubExprs[END_EXPR]);
  }
};

/// \brief Represents Objective-C's \@catch statement.
class ObjCAtCatchStmt : public Stmt {
private:
  VarDecl *ExceptionDecl;
  Stmt *Body;
  SourceLocation AtCatchLoc, RParenLoc;

public:
  ObjCAtCatchStmt(SourceLocation atCatchLoc, SourceLocation rparenloc,
                  VarDecl *catchVarDecl,
                  Stmt *atCatchStmt)
    : Stmt(ObjCAtCatchStmtClass), ExceptionDecl(catchVarDecl), 
    Body(atCatchStmt), AtCatchLoc(atCatchLoc), RParenLoc(rparenloc) { }

  explicit ObjCAtCatchStmt(EmptyShell Empty) :
    Stmt(ObjCAtCatchStmtClass, Empty) { }

  const Stmt *getCatchBody() const { return Body; }
  Stmt *getCatchBody() { return Body; }
  void setCatchBody(Stmt *S) { Body = S; }

  const VarDecl *getCatchParamDecl() const {
    return ExceptionDecl;
  }
  VarDecl *getCatchParamDecl() {
    return ExceptionDecl;
  }
  void setCatchParamDecl(VarDecl *D) { ExceptionDecl = D; }

  SourceLocation getAtCatchLoc() const { return AtCatchLoc; }
  void setAtCatchLoc(SourceLocation Loc) { AtCatchLoc = Loc; }
  SourceLocation getRParenLoc() const { return RParenLoc; }
  void setRParenLoc(SourceLocation Loc) { RParenLoc = Loc; }

  SourceLocation getLocStart() const LLVM_READONLY { return AtCatchLoc; }
  SourceLocation getLocEnd() const LLVM_READONLY { return Body->getLocEnd(); }

  bool hasEllipsis() const { return getCatchParamDecl() == 0; }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == ObjCAtCatchStmtClass;
  }

  child_range children() { return child_range(&Body, &Body + 1); }
};

/// \brief Represents Objective-C's \@finally statement
class ObjCAtFinallyStmt : public Stmt {
  Stmt *AtFinallyStmt;
  SourceLocation AtFinallyLoc;
public:
  ObjCAtFinallyStmt(SourceLocation atFinallyLoc, Stmt *atFinallyStmt)
  : Stmt(ObjCAtFinallyStmtClass),
    AtFinallyStmt(atFinallyStmt), AtFinallyLoc(atFinallyLoc) {}

  explicit ObjCAtFinallyStmt(EmptyShell Empty) :
    Stmt(ObjCAtFinallyStmtClass, Empty) { }

  const Stmt *getFinallyBody() const { return AtFinallyStmt; }
  Stmt *getFinallyBody() { return AtFinallyStmt; }
  void setFinallyBody(Stmt *S) { AtFinallyStmt = S; }

  SourceLocation getLocStart() const LLVM_READONLY { return AtFinallyLoc; }
  SourceLocation getLocEnd() const LLVM_READONLY {
    return AtFinallyStmt->getLocEnd();
  }

  SourceLocation getAtFinallyLoc() const { return AtFinallyLoc; }
  void setAtFinallyLoc(SourceLocation Loc) { AtFinallyLoc = Loc; }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == ObjCAtFinallyStmtClass;
  }

  child_range children() {
    return child_range(&AtFinallyStmt, &AtFinallyStmt+1);
  }
};

/// \brief Represents Objective-C's \@try ... \@catch ... \@finally statement.
class ObjCAtTryStmt : public Stmt {
private:
  // The location of the @ in the \@try.
  SourceLocation AtTryLoc;
  
  // The number of catch blocks in this statement.
  unsigned NumCatchStmts : 16;
  
  // Whether this statement has a \@finally statement.
  bool HasFinally : 1;
  
  /// \brief Retrieve the statements that are stored after this \@try statement.
  ///
  /// The order of the statements in memory follows the order in the source,
  /// with the \@try body first, followed by the \@catch statements (if any)
  /// and, finally, the \@finally (if it exists).
  Stmt **getStmts() { return reinterpret_cast<Stmt **> (this + 1); }
  const Stmt* const *getStmts() const { 
    return reinterpret_cast<const Stmt * const*> (this + 1); 
  }
  
  ObjCAtTryStmt(SourceLocation atTryLoc, Stmt *atTryStmt,
                Stmt **CatchStmts, unsigned NumCatchStmts,
                Stmt *atFinallyStmt);
  
  explicit ObjCAtTryStmt(EmptyShell Empty, unsigned NumCatchStmts,
                         bool HasFinally)
    : Stmt(ObjCAtTryStmtClass, Empty), NumCatchStmts(NumCatchStmts),
      HasFinally(HasFinally) { }

public:
  static ObjCAtTryStmt *Create(ASTContext &Context, SourceLocation atTryLoc, 
                               Stmt *atTryStmt,
                               Stmt **CatchStmts, unsigned NumCatchStmts,
                               Stmt *atFinallyStmt);
  static ObjCAtTryStmt *CreateEmpty(ASTContext &Context, 
                                    unsigned NumCatchStmts,
                                    bool HasFinally);
  
  /// \brief Retrieve the location of the @ in the \@try.
  SourceLocation getAtTryLoc() const { return AtTryLoc; }
  void setAtTryLoc(SourceLocation Loc) { AtTryLoc = Loc; }

  /// \brief Retrieve the \@try body.
  const Stmt *getTryBody() const { return getStmts()[0]; }
  Stmt *getTryBody() { return getStmts()[0]; }
  void setTryBody(Stmt *S) { getStmts()[0] = S; }

  /// \brief Retrieve the number of \@catch statements in this try-catch-finally
  /// block.
  unsigned getNumCatchStmts() const { return NumCatchStmts; }
  
  /// \brief Retrieve a \@catch statement.
  const ObjCAtCatchStmt *getCatchStmt(unsigned I) const {
    assert(I < NumCatchStmts && "Out-of-bounds @catch index");
    return cast_or_null<ObjCAtCatchStmt>(getStmts()[I + 1]);
  }
  
  /// \brief Retrieve a \@catch statement.
  ObjCAtCatchStmt *getCatchStmt(unsigned I) {
    assert(I < NumCatchStmts && "Out-of-bounds @catch index");
    return cast_or_null<ObjCAtCatchStmt>(getStmts()[I + 1]);
  }
  
  /// \brief Set a particular catch statement.
  void setCatchStmt(unsigned I, ObjCAtCatchStmt *S) {
    assert(I < NumCatchStmts && "Out-of-bounds @catch index");
    getStmts()[I + 1] = S;
  }
  
  /// \brief Retrieve the \@finally statement, if any.
  const ObjCAtFinallyStmt *getFinallyStmt() const {
    if (!HasFinally)
      return 0;
    
    return cast_or_null<ObjCAtFinallyStmt>(getStmts()[1 + NumCatchStmts]);
  }
  ObjCAtFinallyStmt *getFinallyStmt() {
    if (!HasFinally)
      return 0;
    
    return cast_or_null<ObjCAtFinallyStmt>(getStmts()[1 + NumCatchStmts]);
  }
  void setFinallyStmt(Stmt *S) { 
    assert(HasFinally && "@try does not have a @finally slot!");
    getStmts()[1 + NumCatchStmts] = S; 
  }

  SourceLocation getLocStart() const LLVM_READONLY { return AtTryLoc; }
  SourceLocation getLocEnd() const LLVM_READONLY;

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == ObjCAtTryStmtClass;
  }

  child_range children() {
    return child_range(getStmts(),
                       getStmts() + 1 + NumCatchStmts + HasFinally);
  }
};

/// \brief Represents Objective-C's \@synchronized statement.
///
/// Example:
/// \code
///   @synchronized (sem) {
///     do-something;
///   }
/// \endcode
class ObjCAtSynchronizedStmt : public Stmt {
private:
  enum { SYNC_EXPR, SYNC_BODY, END_EXPR };
  Stmt* SubStmts[END_EXPR];
  SourceLocation AtSynchronizedLoc;

public:
  ObjCAtSynchronizedStmt(SourceLocation atSynchronizedLoc, Stmt *synchExpr,
                         Stmt *synchBody)
  : Stmt(ObjCAtSynchronizedStmtClass) {
    SubStmts[SYNC_EXPR] = synchExpr;
    SubStmts[SYNC_BODY] = synchBody;
    AtSynchronizedLoc = atSynchronizedLoc;
  }
  explicit ObjCAtSynchronizedStmt(EmptyShell Empty) :
    Stmt(ObjCAtSynchronizedStmtClass, Empty) { }

  SourceLocation getAtSynchronizedLoc() const { return AtSynchronizedLoc; }
  void setAtSynchronizedLoc(SourceLocation Loc) { AtSynchronizedLoc = Loc; }

  const CompoundStmt *getSynchBody() const {
    return reinterpret_cast<CompoundStmt*>(SubStmts[SYNC_BODY]);
  }
  CompoundStmt *getSynchBody() {
    return reinterpret_cast<CompoundStmt*>(SubStmts[SYNC_BODY]);
  }
  void setSynchBody(Stmt *S) { SubStmts[SYNC_BODY] = S; }

  const Expr *getSynchExpr() const {
    return reinterpret_cast<Expr*>(SubStmts[SYNC_EXPR]);
  }
  Expr *getSynchExpr() {
    return reinterpret_cast<Expr*>(SubStmts[SYNC_EXPR]);
  }
  void setSynchExpr(Stmt *S) { SubStmts[SYNC_EXPR] = S; }

  SourceLocation getLocStart() const LLVM_READONLY { return AtSynchronizedLoc; }
  SourceLocation getLocEnd() const LLVM_READONLY {
    return getSynchBody()->getLocEnd();
  }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == ObjCAtSynchronizedStmtClass;
  }

  child_range children() {
    return child_range(&SubStmts[0], &SubStmts[0]+END_EXPR);
  }
};

/// \brief Represents Objective-C's \@throw statement.
class ObjCAtThrowStmt : public Stmt {
  Stmt *Throw;
  SourceLocation AtThrowLoc;
public:
  ObjCAtThrowStmt(SourceLocation atThrowLoc, Stmt *throwExpr)
  : Stmt(ObjCAtThrowStmtClass), Throw(throwExpr) {
    AtThrowLoc = atThrowLoc;
  }
  explicit ObjCAtThrowStmt(EmptyShell Empty) :
    Stmt(ObjCAtThrowStmtClass, Empty) { }

  const Expr *getThrowExpr() const { return reinterpret_cast<Expr*>(Throw); }
  Expr *getThrowExpr() { return reinterpret_cast<Expr*>(Throw); }
  void setThrowExpr(Stmt *S) { Throw = S; }

  SourceLocation getThrowLoc() { return AtThrowLoc; }
  void setThrowLoc(SourceLocation Loc) { AtThrowLoc = Loc; }

  SourceLocation getLocStart() const LLVM_READONLY { return AtThrowLoc; }
  SourceLocation getLocEnd() const LLVM_READONLY {
    return Throw ? Throw->getLocEnd() : AtThrowLoc;
  }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == ObjCAtThrowStmtClass;
  }

  child_range children() { return child_range(&Throw, &Throw+1); }
};

/// \brief Represents Objective-C's \@autoreleasepool Statement
class ObjCAutoreleasePoolStmt : public Stmt {
  Stmt *SubStmt;
  SourceLocation AtLoc;
public:
  ObjCAutoreleasePoolStmt(SourceLocation atLoc, 
                            Stmt *subStmt)
  : Stmt(ObjCAutoreleasePoolStmtClass),
    SubStmt(subStmt), AtLoc(atLoc) {}

  explicit ObjCAutoreleasePoolStmt(EmptyShell Empty) :
    Stmt(ObjCAutoreleasePoolStmtClass, Empty) { }

  const Stmt *getSubStmt() const { return SubStmt; }
  Stmt *getSubStmt() { return SubStmt; }
  void setSubStmt(Stmt *S) { SubStmt = S; }

  SourceLocation getLocStart() const LLVM_READONLY { return AtLoc; }
  SourceLocation getLocEnd() const LLVM_READONLY { return SubStmt->getLocEnd();}

  SourceLocation getAtLoc() const { return AtLoc; }
  void setAtLoc(SourceLocation Loc) { AtLoc = Loc; }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == ObjCAutoreleasePoolStmtClass;
  }

  child_range children() { return child_range(&SubStmt, &SubStmt + 1); }
};

}  // end namespace clang

#endif
