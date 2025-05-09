﻿//===--- RewriteRope.h - Rope specialized for rewriter ----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the RewriteRope class, which is a powerful string class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_REWRITEROPE_H
#define LLVM_CLANG_REWRITEROPE_H

#include "llvm/Support/Compiler.h"
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iterator>

namespace clang {
  //===--------------------------------------------------------------------===//
  // RopeRefCountString Class
  //===--------------------------------------------------------------------===//

  /// RopeRefCountString - This struct is allocated with 'new char[]' from the
  /// heap, and represents a reference counted chunk of string data.  When its
  /// ref count drops to zero, it is delete[]'d.  This is primarily managed
  /// through the RopePiece class below.
  struct RopeRefCountString {
    unsigned RefCount;
    char Data[1];  //  Variable sized.

    void addRef() {
      ++RefCount;
    }

    void dropRef() {
      if (--RefCount == 0)
        delete [] (char*)this;
    }
  };

  //===--------------------------------------------------------------------===//
  // RopePiece Class
  //===--------------------------------------------------------------------===//

  /// RopePiece - This class represents a view into a RopeRefCountString object.
  /// This allows references to string data to be efficiently chopped up and
  /// moved around without having to push around the string data itself.
  ///
  /// For example, we could have a 1M RopePiece and want to insert something
  /// into the middle of it.  To do this, we split it into two RopePiece objects
  /// that both refer to the same underlying RopeRefCountString (just with
  /// different offsets) which is a nice constant time operation.
  struct RopePiece {
    RopeRefCountString *StrData;
    unsigned StartOffs;
    unsigned EndOffs;

    RopePiece() : StrData(0), StartOffs(0), EndOffs(0) {}

    RopePiece(RopeRefCountString *Str, unsigned Start, unsigned End)
      : StrData(Str), StartOffs(Start), EndOffs(End) {
      if (StrData)
        StrData->addRef();
    }
    RopePiece(const RopePiece &RP)
      : StrData(RP.StrData), StartOffs(RP.StartOffs), EndOffs(RP.EndOffs) {
      if (StrData)
        StrData->addRef();
    }

    ~RopePiece() {
      if (StrData)
        StrData->dropRef();
    }

    void operator=(const RopePiece &RHS) {
      if (StrData != RHS.StrData) {
        if (StrData)
          StrData->dropRef();
        StrData = RHS.StrData;
        if (StrData)
          StrData->addRef();
      }
      StartOffs = RHS.StartOffs;
      EndOffs = RHS.EndOffs;
    }

    const char &operator[](unsigned Offset) const {
      return StrData->Data[Offset+StartOffs];
    }
    char &operator[](unsigned Offset) {
      return StrData->Data[Offset+StartOffs];
    }

    unsigned size() const { return EndOffs-StartOffs; }
  };

  //===--------------------------------------------------------------------===//
  // RopePieceBTreeIterator Class
  //===--------------------------------------------------------------------===//

  /// RopePieceBTreeIterator - This class provides read-only forward iteration
  /// over bytes that are in a RopePieceBTree.  This first iterates over bytes
  /// in a RopePiece, then iterates over RopePiece's in a RopePieceBTreeLeaf,
  /// then iterates over RopePieceBTreeLeaf's in a RopePieceBTree.
  class RopePieceBTreeIterator :
      public std::iterator<std::forward_iterator_tag, const char, ptrdiff_t> {
    /// CurNode - The current B+Tree node that we are inspecting.
    const void /*RopePieceBTreeLeaf*/ *CurNode;
    /// CurPiece - The current RopePiece in the B+Tree node that we're
    /// inspecting.
    const RopePiece *CurPiece;
    /// CurChar - The current byte in the RopePiece we are pointing to.
    unsigned CurChar;
  public:
    // begin iterator.
    RopePieceBTreeIterator(const void /*RopePieceBTreeNode*/ *N);
    // end iterator
    RopePieceBTreeIterator() : CurNode(0), CurPiece(0), CurChar(0) {}

    char operator*() const {
      return (*CurPiece)[CurChar];
    }

    bool operator==(const RopePieceBTreeIterator &RHS) const {
      return CurPiece == RHS.CurPiece && CurChar == RHS.CurChar;
    }
    bool operator!=(const RopePieceBTreeIterator &RHS) const {
      return !operator==(RHS);
    }

    RopePieceBTreeIterator& operator++() {   // Preincrement
      if (CurChar+1 < CurPiece->size())
        ++CurChar;
      else
        MoveToNextPiece();
      return *this;
    }
    inline RopePieceBTreeIterator operator++(int) { // Postincrement
      RopePieceBTreeIterator tmp = *this; ++*this; return tmp;
    }
  private:
    void MoveToNextPiece();
  };

  //===--------------------------------------------------------------------===//
  // RopePieceBTree Class
  //===--------------------------------------------------------------------===//

  class RopePieceBTree {
    void /*RopePieceBTreeNode*/ *Root;
    void operator=(const RopePieceBTree &) LLVM_DELETED_FUNCTION;
  public:
    RopePieceBTree();
    RopePieceBTree(const RopePieceBTree &RHS);
    ~RopePieceBTree();

    typedef RopePieceBTreeIterator iterator;
    iterator begin() const { return iterator(Root); }
    iterator end() const { return iterator(); }
    unsigned size() const;
    unsigned empty() const { return size() == 0; }

    void clear();

    void insert(unsigned Offset, const RopePiece &R);

    void erase(unsigned Offset, unsigned NumBytes);
  };

  //===--------------------------------------------------------------------===//
  // RewriteRope Class
  //===--------------------------------------------------------------------===//

/// RewriteRope - A powerful string class.  This class supports extremely
/// efficient insertions and deletions into the middle of it, even for
/// ridiculously long strings.
class RewriteRope {
  RopePieceBTree Chunks;

  /// We allocate space for string data out of a buffer of size AllocChunkSize.
  /// This keeps track of how much space is left.
  RopeRefCountString *AllocBuffer;
  unsigned AllocOffs;
  enum { AllocChunkSize = 4080 };

public:
  RewriteRope() :  AllocBuffer(0), AllocOffs(AllocChunkSize) {}
  RewriteRope(const RewriteRope &RHS)
    : Chunks(RHS.Chunks), AllocBuffer(0), AllocOffs(AllocChunkSize) {
  }

  ~RewriteRope() {
    // If we had an allocation buffer, drop our reference to it.
    if (AllocBuffer)
      AllocBuffer->dropRef();
  }

  typedef RopePieceBTree::iterator iterator;
  typedef RopePieceBTree::iterator const_iterator;
  iterator begin() const { return Chunks.begin(); }
  iterator end() const  { return Chunks.end(); }
  unsigned size() const { return Chunks.size(); }

  void clear() {
    Chunks.clear();
  }

  void assign(const char *Start, const char *End) {
    clear();
    if (Start != End)
      Chunks.insert(0, MakeRopeString(Start, End));
  }

  void insert(unsigned Offset, const char *Start, const char *End) {
    assert(Offset <= size() && "Invalid position to insert!");
    if (Start == End) return;
    Chunks.insert(Offset, MakeRopeString(Start, End));
  }

  void erase(unsigned Offset, unsigned NumBytes) {
    assert(Offset+NumBytes <= size() && "Invalid region to erase!");
    if (NumBytes == 0) return;
    Chunks.erase(Offset, NumBytes);
  }

private:
  RopePiece MakeRopeString(const char *Start, const char *End);
};

} // end namespace clang

#endif
