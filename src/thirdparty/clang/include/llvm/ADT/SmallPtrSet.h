﻿//===- llvm/ADT/SmallPtrSet.h - 'Normally small' pointer set ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the SmallPtrSet class.  See the doxygen comment for
// SmallPtrSetImpl for more details on the algorithm used.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_ADT_SMALLPTRSET_H
#define LLVM_ADT_SMALLPTRSET_H

#include "llvm/Support/Compiler.h"
#include "llvm/Support/DataTypes.h"
#include "llvm/Support/PointerLikeTypeTraits.h"
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iterator>

namespace llvm {

class SmallPtrSetIteratorImpl;

/// SmallPtrSetImpl - This is the common code shared among all the
/// SmallPtrSet<>'s, which is almost everything.  SmallPtrSet has two modes, one
/// for small and one for large sets.
///
/// Small sets use an array of pointers allocated in the SmallPtrSet object,
/// which is treated as a simple array of pointers.  When a pointer is added to
/// the set, the array is scanned to see if the element already exists, if not
/// the element is 'pushed back' onto the array.  If we run out of space in the
/// array, we grow into the 'large set' case.  SmallSet should be used when the
/// sets are often small.  In this case, no memory allocation is used, and only
/// light-weight and cache-efficient scanning is used.
///
/// Large sets use a classic exponentially-probed hash table.  Empty buckets are
/// represented with an illegal pointer value (-1) to allow null pointers to be
/// inserted.  Tombstones are represented with another illegal pointer value
/// (-2), to allow deletion.  The hash table is resized when the table is 3/4 or
/// more.  When this happens, the table is doubled in size.
///
class SmallPtrSetImpl {
  friend class SmallPtrSetIteratorImpl;
protected:
  /// SmallArray - Points to a fixed size set of buckets, used in 'small mode'.
  const void **SmallArray;
  /// CurArray - This is the current set of buckets.  If equal to SmallArray,
  /// then the set is in 'small mode'.
  const void **CurArray;
  /// CurArraySize - The allocated size of CurArray, always a power of two.
  unsigned CurArraySize;

  // If small, this is # elts allocated consecutively
  unsigned NumElements;
  unsigned NumTombstones;

  // Helper to copy construct a SmallPtrSet.
  SmallPtrSetImpl(const void **SmallStorage, const SmallPtrSetImpl& that);
  explicit SmallPtrSetImpl(const void **SmallStorage, unsigned SmallSize) :
    SmallArray(SmallStorage), CurArray(SmallStorage), CurArraySize(SmallSize) {
    assert(SmallSize && (SmallSize & (SmallSize-1)) == 0 &&
           "Initial size must be a power of two!");
    clear();
  }
  ~SmallPtrSetImpl();

public:
  bool empty() const { return size() == 0; }
  unsigned size() const { return NumElements; }

  void clear() {
    // If the capacity of the array is huge, and the # elements used is small,
    // shrink the array.
    if (!isSmall() && NumElements*4 < CurArraySize && CurArraySize > 32)
      return shrink_and_clear();

    // Fill the array with empty markers.
    memset(CurArray, -1, CurArraySize*sizeof(void*));
    NumElements = 0;
    NumTombstones = 0;
  }

protected:
  static void *getTombstoneMarker() { return reinterpret_cast<void*>(-2); }
  static void *getEmptyMarker() {
    // Note that -1 is chosen to make clear() efficiently implementable with
    // memset and because it's not a valid pointer value.
    return reinterpret_cast<void*>(-1);
  }

  /// insert_imp - This returns true if the pointer was new to the set, false if
  /// it was already in the set.  This is hidden from the client so that the
  /// derived class can check that the right type of pointer is passed in.
  bool insert_imp(const void * Ptr);

  /// erase_imp - If the set contains the specified pointer, remove it and
  /// return true, otherwise return false.  This is hidden from the client so
  /// that the derived class can check that the right type of pointer is passed
  /// in.
  bool erase_imp(const void * Ptr);

  bool count_imp(const void * Ptr) const {
    if (isSmall()) {
      // Linear search for the item.
      for (const void *const *APtr = SmallArray,
                      *const *E = SmallArray+NumElements; APtr != E; ++APtr)
        if (*APtr == Ptr)
          return true;
      return false;
    }

    // Big set case.
    return *FindBucketFor(Ptr) == Ptr;
  }

private:
  bool isSmall() const { return CurArray == SmallArray; }

  const void * const *FindBucketFor(const void *Ptr) const;
  void shrink_and_clear();

  /// Grow - Allocate a larger backing store for the buckets and move it over.
  void Grow(unsigned NewSize);

  void operator=(const SmallPtrSetImpl &RHS) LLVM_DELETED_FUNCTION;
protected:
  /// swap - Swaps the elements of two sets.
  /// Note: This method assumes that both sets have the same small size.
  void swap(SmallPtrSetImpl &RHS);

  void CopyFrom(const SmallPtrSetImpl &RHS);
};

/// SmallPtrSetIteratorImpl - This is the common base class shared between all
/// instances of SmallPtrSetIterator.
class SmallPtrSetIteratorImpl {
protected:
  const void *const *Bucket;
  const void *const *End;
public:
  explicit SmallPtrSetIteratorImpl(const void *const *BP, const void*const *E)
    : Bucket(BP), End(E) {
      AdvanceIfNotValid();
  }

  bool operator==(const SmallPtrSetIteratorImpl &RHS) const {
    return Bucket == RHS.Bucket;
  }
  bool operator!=(const SmallPtrSetIteratorImpl &RHS) const {
    return Bucket != RHS.Bucket;
  }

protected:
  /// AdvanceIfNotValid - If the current bucket isn't valid, advance to a bucket
  /// that is.   This is guaranteed to stop because the end() bucket is marked
  /// valid.
  void AdvanceIfNotValid() {
    assert(Bucket <= End);
    while (Bucket != End &&
           (*Bucket == SmallPtrSetImpl::getEmptyMarker() ||
            *Bucket == SmallPtrSetImpl::getTombstoneMarker()))
      ++Bucket;
  }
};

/// SmallPtrSetIterator - This implements a const_iterator for SmallPtrSet.
template<typename PtrTy>
class SmallPtrSetIterator : public SmallPtrSetIteratorImpl {
  typedef PointerLikeTypeTraits<PtrTy> PtrTraits;
  
public:
  typedef PtrTy                     value_type;
  typedef PtrTy                     reference;
  typedef PtrTy                     pointer;
  typedef std::ptrdiff_t            difference_type;
  typedef std::forward_iterator_tag iterator_category;
  
  explicit SmallPtrSetIterator(const void *const *BP, const void *const *E)
    : SmallPtrSetIteratorImpl(BP, E) {}

  // Most methods provided by baseclass.

  const PtrTy operator*() const {
    assert(Bucket < End);
    return PtrTraits::getFromVoidPointer(const_cast<void*>(*Bucket));
  }

  inline SmallPtrSetIterator& operator++() {          // Preincrement
    ++Bucket;
    AdvanceIfNotValid();
    return *this;
  }

  SmallPtrSetIterator operator++(int) {        // Postincrement
    SmallPtrSetIterator tmp = *this; ++*this; return tmp;
  }
};

/// RoundUpToPowerOfTwo - This is a helper template that rounds N up to the next
/// power of two (which means N itself if N is already a power of two).
template<unsigned N>
struct RoundUpToPowerOfTwo;

/// RoundUpToPowerOfTwoH - If N is not a power of two, increase it.  This is a
/// helper template used to implement RoundUpToPowerOfTwo.
template<unsigned N, bool isPowerTwo>
struct RoundUpToPowerOfTwoH {
  enum { Val = N };
};
template<unsigned N>
struct RoundUpToPowerOfTwoH<N, false> {
  enum {
    // We could just use NextVal = N+1, but this converges faster.  N|(N-1) sets
    // the right-most zero bits to one all at once, e.g. 0b0011000 -> 0b0011111.
    Val = RoundUpToPowerOfTwo<(N|(N-1)) + 1>::Val
  };
};

template<unsigned N>
struct RoundUpToPowerOfTwo {
  enum { Val = RoundUpToPowerOfTwoH<N, (N&(N-1)) == 0>::Val };
};
  

/// SmallPtrSet - This class implements a set which is optimized for holding
/// SmallSize or less elements.  This internally rounds up SmallSize to the next
/// power of two if it is not already a power of two.  See the comments above
/// SmallPtrSetImpl for details of the algorithm.
template<class PtrType, unsigned SmallSize>
class SmallPtrSet : public SmallPtrSetImpl {
  // Make sure that SmallSize is a power of two, round up if not.
  enum { SmallSizePowTwo = RoundUpToPowerOfTwo<SmallSize>::Val };
  /// SmallStorage - Fixed size storage used in 'small mode'.
  const void *SmallStorage[SmallSizePowTwo];
  typedef PointerLikeTypeTraits<PtrType> PtrTraits;
public:
  SmallPtrSet() : SmallPtrSetImpl(SmallStorage, SmallSizePowTwo) {}
  SmallPtrSet(const SmallPtrSet &that) : SmallPtrSetImpl(SmallStorage, that) {}

  template<typename It>
  SmallPtrSet(It I, It E) : SmallPtrSetImpl(SmallStorage, SmallSizePowTwo) {
    insert(I, E);
  }

  /// insert - This returns true if the pointer was new to the set, false if it
  /// was already in the set.
  bool insert(PtrType Ptr) {
    return insert_imp(PtrTraits::getAsVoidPointer(Ptr));
  }

  /// erase - If the set contains the specified pointer, remove it and return
  /// true, otherwise return false.
  bool erase(PtrType Ptr) {
    return erase_imp(PtrTraits::getAsVoidPointer(Ptr));
  }

  /// count - Return true if the specified pointer is in the set.
  bool count(PtrType Ptr) const {
    return count_imp(PtrTraits::getAsVoidPointer(Ptr));
  }

  template <typename IterT>
  void insert(IterT I, IterT E) {
    for (; I != E; ++I)
      insert(*I);
  }

  typedef SmallPtrSetIterator<PtrType> iterator;
  typedef SmallPtrSetIterator<PtrType> const_iterator;
  inline iterator begin() const {
    return iterator(CurArray, CurArray+CurArraySize);
  }
  inline iterator end() const {
    return iterator(CurArray+CurArraySize, CurArray+CurArraySize);
  }

  // Allow assignment from any smallptrset with the same element type even if it
  // doesn't have the same smallsize.
  const SmallPtrSet<PtrType, SmallSize>&
  operator=(const SmallPtrSet<PtrType, SmallSize> &RHS) {
    CopyFrom(RHS);
    return *this;
  }

  /// swap - Swaps the elements of two sets.
  void swap(SmallPtrSet<PtrType, SmallSize> &RHS) {
    SmallPtrSetImpl::swap(RHS);
  }
};

}

namespace std {
  /// Implement std::swap in terms of SmallPtrSet swap.
  template<class T, unsigned N>
  inline void swap(llvm::SmallPtrSet<T, N> &LHS, llvm::SmallPtrSet<T, N> &RHS) {
    LHS.swap(RHS);
  }
}

#endif
