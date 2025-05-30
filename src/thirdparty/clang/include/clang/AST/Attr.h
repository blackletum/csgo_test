﻿//===--- Attr.h - Classes for representing attributes ----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the Attr interface and subclasses.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_ATTR_H
#define LLVM_CLANG_AST_ATTR_H

#include "clang/AST/AttrIterator.h"
#include "clang/AST/Type.h"
#include "clang/Basic/AttrKinds.h"
#include "clang/Basic/LLVM.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/VersionTuple.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include <cassert>
#include <cstring>

namespace clang {
  class ASTContext;
  class IdentifierInfo;
  class ObjCInterfaceDecl;
  class Expr;
  class QualType;
  class FunctionDecl;
  class TypeSourceInfo;

/// Attr - This represents one attribute.
class Attr {
private:
  SourceRange Range;
  unsigned AttrKind : 16;

protected:
  /// An index into the spelling list of an
  /// attribute defined in Attr.td file.
  unsigned SpellingListIndex : 4;

  bool Inherited : 1;

  bool IsPackExpansion : 1;

  virtual ~Attr();

  void* operator new(size_t bytes) throw() {
    llvm_unreachable("Attrs cannot be allocated with regular 'new'.");
  }
  void operator delete(void* data) throw() {
    llvm_unreachable("Attrs cannot be released with regular 'delete'.");
  }

public:
  // Forward so that the regular new and delete do not hide global ones.
  void* operator new(size_t Bytes, ASTContext &C,
                     size_t Alignment = 16) throw() {
    return ::operator new(Bytes, C, Alignment);
  }
  void operator delete(void *Ptr, ASTContext &C,
                       size_t Alignment) throw() {
    return ::operator delete(Ptr, C, Alignment);
  }

protected:
  Attr(attr::Kind AK, SourceRange R, unsigned SpellingListIndex = 0)
    : Range(R), AttrKind(AK), SpellingListIndex(SpellingListIndex),
      Inherited(false), IsPackExpansion(false) {}

public:

  attr::Kind getKind() const {
    return static_cast<attr::Kind>(AttrKind);
  }
  
  unsigned getSpellingListIndex() const { return SpellingListIndex; }

  SourceLocation getLocation() const { return Range.getBegin(); }
  SourceRange getRange() const { return Range; }
  void setRange(SourceRange R) { Range = R; }

  bool isInherited() const { return Inherited; }

  void setPackExpansion(bool PE) { IsPackExpansion = PE; }
  bool isPackExpansion() const { return IsPackExpansion; }

  // Clone this attribute.
  virtual Attr *clone(ASTContext &C) const = 0;

  virtual bool isLateParsed() const { return false; }

  // Pretty print this attribute.
  virtual void printPretty(raw_ostream &OS,
                           const PrintingPolicy &Policy) const = 0;
};

class InheritableAttr : public Attr {
  virtual void anchor();
protected:
  InheritableAttr(attr::Kind AK, SourceRange R, unsigned SpellingListIndex = 0)
    : Attr(AK, R, SpellingListIndex) {}

public:
  void setInherited(bool I) { Inherited = I; }

  // Implement isa/cast/dyncast/etc.
  static bool classof(const Attr *A) {
    return A->getKind() <= attr::LAST_INHERITABLE;
  }
};

class InheritableParamAttr : public InheritableAttr {
  virtual void anchor();
protected:
  InheritableParamAttr(attr::Kind AK, SourceRange R,
                       unsigned SpellingListIndex = 0)
    : InheritableAttr(AK, R, SpellingListIndex) {}

public:
  // Implement isa/cast/dyncast/etc.
  static bool classof(const Attr *A) {
    // Relies on relative order of enum emission with respect to MS inheritance
    // attrs.
    return A->getKind() <= attr::LAST_INHERITABLE_PARAM;
  }
};

class MSInheritanceAttr : public InheritableAttr {
  virtual void anchor();
protected:
  MSInheritanceAttr(attr::Kind AK, SourceRange R, unsigned SpellingListIndex = 0)
    : InheritableAttr(AK, R, SpellingListIndex) {}

public:
  // Implement isa/cast/dyncast/etc.
  static bool classof(const Attr *A) {
    // Relies on relative order of enum emission with respect to param attrs.
    return (A->getKind() <= attr::LAST_MS_INHERITABLE &&
            A->getKind() > attr::LAST_INHERITABLE_PARAM);
  }
};

#include "clang/AST/Attrs.inc"

}  // end namespace clang

#endif
