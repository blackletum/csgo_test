﻿//===--- ImmutableSet.h - Immutable (functional) set interface --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the ImutAVLTree and ImmutableSet classes.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_ADT_IMMUTABLESET_H
#define LLVM_ADT_IMMUTABLESET_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/Support/Allocator.h"
#include "llvm/Support/DataTypes.h"
#include "llvm/Support/ErrorHandling.h"
#include <cassert>
#include <functional>
#include <vector>

namespace llvm {

//===----------------------------------------------------------------------===//
// Immutable AVL-Tree Definition.
//===----------------------------------------------------------------------===//

template <typename ImutInfo> class ImutAVLFactory;
template <typename ImutInfo> class ImutIntervalAVLFactory;
template <typename ImutInfo> class ImutAVLTreeInOrderIterator;
template <typename ImutInfo> class ImutAVLTreeGenericIterator;

template <typename ImutInfo >
class ImutAVLTree {
public:
  typedef typename ImutInfo::key_type_ref   key_type_ref;
  typedef typename ImutInfo::value_type     value_type;
  typedef typename ImutInfo::value_type_ref value_type_ref;

  typedef ImutAVLFactory<ImutInfo>          Factory;
  friend class ImutAVLFactory<ImutInfo>;
  friend class ImutIntervalAVLFactory<ImutInfo>;

  friend class ImutAVLTreeGenericIterator<ImutInfo>;

  typedef ImutAVLTreeInOrderIterator<ImutInfo>  iterator;

  //===----------------------------------------------------===//
  // Public Interface.
  //===----------------------------------------------------===//

  /// Return a pointer to the left subtree.  This value
  ///  is NULL if there is no left subtree.
  ImutAVLTree *getLeft() const { return left; }

  /// Return a pointer to the right subtree.  This value is
  ///  NULL if there is no right subtree.
  ImutAVLTree *getRight() const { return right; }

  /// getHeight - Returns the height of the tree.  A tree with no subtrees
  ///  has a height of 1.
  unsigned getHeight() const { return height; }

  /// getValue - Returns the data value associated with the tree node.
  const value_type& getValue() const { return value; }

  /// find - Finds the subtree associated with the specified key value.
  ///  This method returns NULL if no matching subtree is found.
  ImutAVLTree* find(key_type_ref K) {
    ImutAVLTree *T = this;
    while (T) {
      key_type_ref CurrentKey = ImutInfo::KeyOfValue(T->getValue());
      if (ImutInfo::isEqual(K,CurrentKey))
        return T;
      else if (ImutInfo::isLess(K,CurrentKey))
        T = T->getLeft();
      else
        T = T->getRight();
    }
    return NULL;
  }

  /// getMaxElement - Find the subtree associated with the highest ranged
  ///  key value.
  ImutAVLTree* getMaxElement() {
    ImutAVLTree *T = this;
    ImutAVLTree *Right = T->getRight();
    while (Right) { T = Right; Right = T->getRight(); }
    return T;
  }

  /// size - Returns the number of nodes in the tree, which includes
  ///  both leaves and non-leaf nodes.
  unsigned size() const {
    unsigned n = 1;
    if (const ImutAVLTree* L = getLeft())
      n += L->size();
    if (const ImutAVLTree* R = getRight())
      n += R->size();
    return n;
  }

  /// begin - Returns an iterator that iterates over the nodes of the tree
  ///  in an inorder traversal.  The returned iterator thus refers to the
  ///  the tree node with the minimum data element.
  iterator begin() const { return iterator(this); }

  /// end - Returns an iterator for the tree that denotes the end of an
  ///  inorder traversal.
  iterator end() const { return iterator(); }

  bool isElementEqual(value_type_ref V) const {
    // Compare the keys.
    if (!ImutInfo::isEqual(ImutInfo::KeyOfValue(getValue()),
                           ImutInfo::KeyOfValue(V)))
      return false;

    // Also compare the data values.
    if (!ImutInfo::isDataEqual(ImutInfo::DataOfValue(getValue()),
                               ImutInfo::DataOfValue(V)))
      return false;

    return true;
  }

  bool isElementEqual(const ImutAVLTree* RHS) const {
    return isElementEqual(RHS->getValue());
  }

  /// isEqual - Compares two trees for structural equality and returns true
  ///   if they are equal.  This worst case performance of this operation is
  //    linear in the sizes of the trees.
  bool isEqual(const ImutAVLTree& RHS) const {
    if (&RHS == this)
      return true;

    iterator LItr = begin(), LEnd = end();
    iterator RItr = RHS.begin(), REnd = RHS.end();

    while (LItr != LEnd && RItr != REnd) {
      if (*LItr == *RItr) {
        LItr.skipSubTree();
        RItr.skipSubTree();
        continue;
      }

      if (!LItr->isElementEqual(*RItr))
        return false;

      ++LItr;
      ++RItr;
    }

    return LItr == LEnd && RItr == REnd;
  }

  /// isNotEqual - Compares two trees for structural inequality.  Performance
  ///  is the same is isEqual.
  bool isNotEqual(const ImutAVLTree& RHS) const { return !isEqual(RHS); }

  /// contains - Returns true if this tree contains a subtree (node) that
  ///  has an data element that matches the specified key.  Complexity
  ///  is logarithmic in the size of the tree.
  bool contains(key_type_ref K) { return (bool) find(K); }

  /// foreach - A member template the accepts invokes operator() on a functor
  ///  object (specifed by Callback) for every node/subtree in the tree.
  ///  Nodes are visited using an inorder traversal.
  template <typename Callback>
  void foreach(Callback& C) {
    if (ImutAVLTree* L = getLeft())
      L->foreach(C);

    C(value);

    if (ImutAVLTree* R = getRight())
      R->foreach(C);
  }

  /// validateTree - A utility method that checks that the balancing and
  ///  ordering invariants of the tree are satisifed.  It is a recursive
  ///  method that returns the height of the tree, which is then consumed
  ///  by the enclosing validateTree call.  External callers should ignore the
  ///  return value.  An invalid tree will cause an assertion to fire in
  ///  a debug build.
  unsigned validateTree() const {
    unsigned HL = getLeft() ? getLeft()->validateTree() : 0;
    unsigned HR = getRight() ? getRight()->validateTree() : 0;
    (void) HL;
    (void) HR;

    assert(getHeight() == ( HL > HR ? HL : HR ) + 1
            && "Height calculation wrong");

    assert((HL > HR ? HL-HR : HR-HL) <= 2
           && "Balancing invariant violated");

    assert((!getLeft() ||
            ImutInfo::isLess(ImutInfo::KeyOfValue(getLeft()->getValue()),
                             ImutInfo::KeyOfValue(getValue()))) &&
           "Value in left child is not less that current value");


    assert(!(getRight() ||
             ImutInfo::isLess(ImutInfo::KeyOfValue(getValue()),
                              ImutInfo::KeyOfValue(getRight()->getValue()))) &&
           "Current value is not less that value of right child");

    return getHeight();
  }

  //===----------------------------------------------------===//
  // Internal values.
  //===----------------------------------------------------===//

private:
  Factory *factory;
  ImutAVLTree *left;
  ImutAVLTree *right;
  ImutAVLTree *prev;
  ImutAVLTree *next;

  unsigned height         : 28;
  unsigned IsMutable      : 1;
  unsigned IsDigestCached : 1;
  unsigned IsCanonicalized : 1;

  value_type value;
  uint32_t digest;
  uint32_t refCount;

  //===----------------------------------------------------===//
  // Internal methods (node manipulation; used by Factory).
  //===----------------------------------------------------===//

private:
  /// ImutAVLTree - Internal constructor that is only called by
  ///   ImutAVLFactory.
  ImutAVLTree(Factory *f, ImutAVLTree* l, ImutAVLTree* r, value_type_ref v,
              unsigned height)
    : factory(f), left(l), right(r), prev(0), next(0), height(height),
      IsMutable(true), IsDigestCached(false), IsCanonicalized(0),
      value(v), digest(0), refCount(0)
  {
    if (left) left->retain();
    if (right) right->retain();
  }

  /// isMutable - Returns true if the left and right subtree references
  ///  (as well as height) can be changed.  If this method returns false,
  ///  the tree is truly immutable.  Trees returned from an ImutAVLFactory
  ///  object should always have this method return true.  Further, if this
  ///  method returns false for an instance of ImutAVLTree, all subtrees
  ///  will also have this method return false.  The converse is not true.
  bool isMutable() const { return IsMutable; }

  /// hasCachedDigest - Returns true if the digest for this tree is cached.
  ///  This can only be true if the tree is immutable.
  bool hasCachedDigest() const { return IsDigestCached; }

  //===----------------------------------------------------===//
  // Mutating operations.  A tree root can be manipulated as
  // long as its reference has not "escaped" from internal
  // methods of a factory object (see below).  When a tree
  // pointer is externally viewable by client code, the
  // internal "mutable bit" is cleared to mark the tree
  // immutable.  Note that a tree that still has its mutable
  // bit set may have children (subtrees) that are themselves
  // immutable.
  //===----------------------------------------------------===//

  /// markImmutable - Clears the mutable flag for a tree.  After this happens,
  ///   it is an error to call setLeft(), setRight(), and setHeight().
  void markImmutable() {
    assert(isMutable() && "Mutable flag already removed.");
    IsMutable = false;
  }

  /// markedCachedDigest - Clears the NoCachedDigest flag for a tree.
  void markedCachedDigest() {
    assert(!hasCachedDigest() && "NoCachedDigest flag already removed.");
    IsDigestCached = true;
  }

  /// setHeight - Changes the height of the tree.  Used internally by
  ///  ImutAVLFactory.
  void setHeight(unsigned h) {
    assert(isMutable() && "Only a mutable tree can have its height changed.");
    height = h;
  }

  static inline
  uint32_t computeDigest(ImutAVLTree* L, ImutAVLTree* R, value_type_ref V) {
    uint32_t digest = 0;

    if (L)
      digest += L->computeDigest();

    // Compute digest of stored data.
    FoldingSetNodeID ID;
    ImutInfo::Profile(ID,V);
    digest += ID.ComputeHash();

    if (R)
      digest += R->computeDigest();

    return digest;
  }

  inline uint32_t computeDigest() {
    // Check the lowest bit to determine if digest has actually been
    // pre-computed.
    if (hasCachedDigest())
      return digest;

    uint32_t X = computeDigest(getLeft(), getRight(), getValue());
    digest = X;
    markedCachedDigest();
    return X;
  }

  //===----------------------------------------------------===//
  // Reference count operations.
  //===----------------------------------------------------===//

public:
  void retain() { ++refCount; }
  void release() {
    assert(refCount > 0);
    if (--refCount == 0)
      destroy();
  }
  void destroy() {
    if (left)
      left->release();
    if (right)
      right->release();
    if (IsCanonicalized) {
      if (next)
        next->prev = prev;

      if (prev)
        prev->next = next;
      else
        factory->Cache[factory->maskCacheIndex(computeDigest())] = next;
    }

    // We need to clear the mutability bit in case we are
    // destroying the node as part of a sweep in ImutAVLFactory::recoverNodes().
    IsMutable = false;
    factory->freeNodes.push_back(this);
  }
};

//===----------------------------------------------------------------------===//
// Immutable AVL-Tree Factory class.
//===----------------------------------------------------------------------===//

template <typename ImutInfo >
class ImutAVLFactory {
  friend class ImutAVLTree<ImutInfo>;
  typedef ImutAVLTree<ImutInfo> TreeTy;
  typedef typename TreeTy::value_type_ref value_type_ref;
  typedef typename TreeTy::key_type_ref   key_type_ref;

  typedef DenseMap<unsigned, TreeTy*> CacheTy;

  CacheTy Cache;
  uintptr_t Allocator;
  std::vector<TreeTy*> createdNodes;
  std::vector<TreeTy*> freeNodes;

  bool ownsAllocator() const {
    return Allocator & 0x1 ? false : true;
  }

  BumpPtrAllocator& getAllocator() const {
    return *reinterpret_cast<BumpPtrAllocator*>(Allocator & ~0x1);
  }

  //===--------------------------------------------------===//
  // Public interface.
  //===--------------------------------------------------===//

public:
  ImutAVLFactory()
    : Allocator(reinterpret_cast<uintptr_t>(new BumpPtrAllocator())) {}

  ImutAVLFactory(BumpPtrAllocator& Alloc)
    : Allocator(reinterpret_cast<uintptr_t>(&Alloc) | 0x1) {}

  ~ImutAVLFactory() {
    if (ownsAllocator()) delete &getAllocator();
  }

  TreeTy* add(TreeTy* T, value_type_ref V) {
    T = add_internal(V,T);
    markImmutable(T);
    recoverNodes();
    return T;
  }

  TreeTy* remove(TreeTy* T, key_type_ref V) {
    T = remove_internal(V,T);
    markImmutable(T);
    recoverNodes();
    return T;
  }

  TreeTy* getEmptyTree() const { return NULL; }

protected:

  //===--------------------------------------------------===//
  // A bunch of quick helper functions used for reasoning
  // about the properties of trees and their children.
  // These have succinct names so that the balancing code
  // is as terse (and readable) as possible.
  //===--------------------------------------------------===//

  bool            isEmpty(TreeTy* T) const { return !T; }
  unsigned        getHeight(TreeTy* T) const { return T ? T->getHeight() : 0; }
  TreeTy*         getLeft(TreeTy* T) const { return T->getLeft(); }
  TreeTy*         getRight(TreeTy* T) const { return T->getRight(); }
  value_type_ref  getValue(TreeTy* T) const { return T->value; }

  // Make sure the index is not the Tombstone or Entry key of the DenseMap.
  static inline unsigned maskCacheIndex(unsigned I) {
    return (I & ~0x02);
  }

  unsigned incrementHeight(TreeTy* L, TreeTy* R) const {
    unsigned hl = getHeight(L);
    unsigned hr = getHeight(R);
    return (hl > hr ? hl : hr) + 1;
  }

  static bool compareTreeWithSection(TreeTy* T,
                                     typename TreeTy::iterator& TI,
                                     typename TreeTy::iterator& TE) {
    typename TreeTy::iterator I = T->begin(), E = T->end();
    for ( ; I!=E ; ++I, ++TI) {
      if (TI == TE || !I->isElementEqual(*TI))
        return false;
    }
    return true;
  }

  //===--------------------------------------------------===//
  // "createNode" is used to generate new tree roots that link
  // to other trees.  The functon may also simply move links
  // in an existing root if that root is still marked mutable.
  // This is necessary because otherwise our balancing code
  // would leak memory as it would create nodes that are
  // then discarded later before the finished tree is
  // returned to the caller.
  //===--------------------------------------------------===//

  TreeTy* createNode(TreeTy* L, value_type_ref V, TreeTy* R) {
    BumpPtrAllocator& A = getAllocator();
    TreeTy* T;
    if (!freeNodes.empty()) {
      T = freeNodes.back();
      freeNodes.pop_back();
      assert(T != L);
      assert(T != R);
    } else {
      T = (TreeTy*) A.Allocate<TreeTy>();
    }
    new (T) TreeTy(this, L, R, V, incrementHeight(L,R));
    createdNodes.push_back(T);
    return T;
  }

  TreeTy* createNode(TreeTy* newLeft, TreeTy* oldTree, TreeTy* newRight) {
    return createNode(newLeft, getValue(oldTree), newRight);
  }

  void recoverNodes() {
    for (unsigned i = 0, n = createdNodes.size(); i < n; ++i) {
      TreeTy *N = createdNodes[i];
      if (N->isMutable() && N->refCount == 0)
        N->destroy();
    }
    createdNodes.clear();
  }

  /// balanceTree - Used by add_internal and remove_internal to
  ///  balance a newly created tree.
  TreeTy* balanceTree(TreeTy* L, value_type_ref V, TreeTy* R) {
    unsigned hl = getHeight(L);
    unsigned hr = getHeight(R);

    if (hl > hr + 2) {
      assert(!isEmpty(L) && "Left tree cannot be empty to have a height >= 2");

      TreeTy *LL = getLeft(L);
      TreeTy *LR = getRight(L);

      if (getHeight(LL) >= getHeight(LR))
        return createNode(LL, L, createNode(LR,V,R));

      assert(!isEmpty(LR) && "LR cannot be empty because it has a height >= 1");

      TreeTy *LRL = getLeft(LR);
      TreeTy *LRR = getRight(LR);

      return createNode(createNode(LL,L,LRL), LR, createNode(LRR,V,R));
    }

    if (hr > hl + 2) {
      assert(!isEmpty(R) && "Right tree cannot be empty to have a height >= 2");

      TreeTy *RL = getLeft(R);
      TreeTy *RR = getRight(R);

      if (getHeight(RR) >= getHeight(RL))
        return createNode(createNode(L,V,RL), R, RR);

      assert(!isEmpty(RL) && "RL cannot be empty because it has a height >= 1");

      TreeTy *RLL = getLeft(RL);
      TreeTy *RLR = getRight(RL);

      return createNode(createNode(L,V,RLL), RL, createNode(RLR,R,RR));
    }

    return createNode(L,V,R);
  }

  /// add_internal - Creates a new tree that includes the specified
  ///  data and the data from the original tree.  If the original tree
  ///  already contained the data item, the original tree is returned.
  TreeTy* add_internal(value_type_ref V, TreeTy* T) {
    if (isEmpty(T))
      return createNode(T, V, T);
    assert(!T->isMutable());

    key_type_ref K = ImutInfo::KeyOfValue(V);
    key_type_ref KCurrent = ImutInfo::KeyOfValue(getValue(T));

    if (ImutInfo::isEqual(K,KCurrent))
      return createNode(getLeft(T), V, getRight(T));
    else if (ImutInfo::isLess(K,KCurrent))
      return balanceTree(add_internal(V, getLeft(T)), getValue(T), getRight(T));
    else
      return balanceTree(getLeft(T), getValue(T), add_internal(V, getRight(T)));
  }

  /// remove_internal - Creates a new tree that includes all the data
  ///  from the original tree except the specified data.  If the
  ///  specified data did not exist in the original tree, the original
  ///  tree is returned.
  TreeTy* remove_internal(key_type_ref K, TreeTy* T) {
    if (isEmpty(T))
      return T;

    assert(!T->isMutable());

    key_type_ref KCurrent = ImutInfo::KeyOfValue(getValue(T));

    if (ImutInfo::isEqual(K,KCurrent)) {
      return combineTrees(getLeft(T), getRight(T));
    } else if (ImutInfo::isLess(K,KCurrent)) {
      return balanceTree(remove_internal(K, getLeft(T)),
                                            getValue(T), getRight(T));
    } else {
      return balanceTree(getLeft(T), getValue(T),
                         remove_internal(K, getRight(T)));
    }
  }

  TreeTy* combineTrees(TreeTy* L, TreeTy* R) {
    if (isEmpty(L))
      return R;
    if (isEmpty(R))
      return L;
    TreeTy* OldNode;
    TreeTy* newRight = removeMinBinding(R,OldNode);
    return balanceTree(L, getValue(OldNode), newRight);
  }

  TreeTy* removeMinBinding(TreeTy* T, TreeTy*& Noderemoved) {
    assert(!isEmpty(T));
    if (isEmpty(getLeft(T))) {
      Noderemoved = T;
      return getRight(T);
    }
    return balanceTree(removeMinBinding(getLeft(T), Noderemoved),
                       getValue(T), getRight(T));
  }

  /// markImmutable - Clears the mutable bits of a root and all of its
  ///  descendants.
  void markImmutable(TreeTy* T) {
    if (!T || !T->isMutable())
      return;
    T->markImmutable();
    markImmutable(getLeft(T));
    markImmutable(getRight(T));
  }

public:
  TreeTy *getCanonicalTree(TreeTy *TNew) {
    if (!TNew)
      return 0;

    if (TNew->IsCanonicalized)
      return TNew;

    // Search the hashtable for another tree with the same digest, and
    // if find a collision compare those trees by their contents.
    unsigned digest = TNew->computeDigest();
    TreeTy *&entry = Cache[maskCacheIndex(digest)];
    do {
      if (!entry)
        break;
      for (TreeTy *T = entry ; T != 0; T = T->next) {
        // Compare the Contents('T') with Contents('TNew')
        typename TreeTy::iterator TI = T->begin(), TE = T->end();
        if (!compareTreeWithSection(TNew, TI, TE))
          continue;
        if (TI != TE)
          continue; // T has more contents than TNew.
        // Trees did match!  Return 'T'.
        if (TNew->refCount == 0)
          TNew->destroy();
        return T;
      }
      entry->prev = TNew;
      TNew->next = entry;
    }
    while (false);

    entry = TNew;
    TNew->IsCanonicalized = true;
    return TNew;
  }
};

//===----------------------------------------------------------------------===//
// Immutable AVL-Tree Iterators.
//===----------------------------------------------------------------------===//

template <typename ImutInfo>
class ImutAVLTreeGenericIterator {
  SmallVector<uintptr_t,20> stack;
public:
  enum VisitFlag { VisitedNone=0x0, VisitedLeft=0x1, VisitedRight=0x3,
                   Flags=0x3 };

  typedef ImutAVLTree<ImutInfo> TreeTy;
  typedef ImutAVLTreeGenericIterator<ImutInfo> _Self;

  inline ImutAVLTreeGenericIterator() {}
  inline ImutAVLTreeGenericIterator(const TreeTy* Root) {
    if (Root) stack.push_back(reinterpret_cast<uintptr_t>(Root));
  }

  TreeTy* operator*() const {
    assert(!stack.empty());
    return reinterpret_cast<TreeTy*>(stack.back() & ~Flags);
  }

  uintptr_t getVisitState() const {
    assert(!stack.empty());
    return stack.back() & Flags;
  }


  bool atEnd() const { return stack.empty(); }

  bool atBeginning() const {
    return stack.size() == 1 && getVisitState() == VisitedNone;
  }

  void skipToParent() {
    assert(!stack.empty());
    stack.pop_back();
    if (stack.empty())
      return;
    switch (getVisitState()) {
      case VisitedNone:
        stack.back() |= VisitedLeft;
        break;
      case VisitedLeft:
        stack.back() |= VisitedRight;
        break;
      default:
        llvm_unreachable("Unreachable.");
    }
  }

  inline bool operator==(const _Self& x) const {
    if (stack.size() != x.stack.size())
      return false;
    for (unsigned i = 0 ; i < stack.size(); i++)
      if (stack[i] != x.stack[i])
        return false;
    return true;
  }

  inline bool operator!=(const _Self& x) const { return !operator==(x); }

  _Self& operator++() {
    assert(!stack.empty());
    TreeTy* Current = reinterpret_cast<TreeTy*>(stack.back() & ~Flags);
    assert(Current);
    switch (getVisitState()) {
      case VisitedNone:
        if (TreeTy* L = Current->getLeft())
          stack.push_back(reinterpret_cast<uintptr_t>(L));
        else
          stack.back() |= VisitedLeft;
        break;
      case VisitedLeft:
        if (TreeTy* R = Current->getRight())
          stack.push_back(reinterpret_cast<uintptr_t>(R));
        else
          stack.back() |= VisitedRight;
        break;
      case VisitedRight:
        skipToParent();
        break;
      default:
        llvm_unreachable("Unreachable.");
    }
    return *this;
  }

  _Self& operator--() {
    assert(!stack.empty());
    TreeTy* Current = reinterpret_cast<TreeTy*>(stack.back() & ~Flags);
    assert(Current);
    switch (getVisitState()) {
      case VisitedNone:
        stack.pop_back();
        break;
      case VisitedLeft:
        stack.back() &= ~Flags; // Set state to "VisitedNone."
        if (TreeTy* L = Current->getLeft())
          stack.push_back(reinterpret_cast<uintptr_t>(L) | VisitedRight);
        break;
      case VisitedRight:
        stack.back() &= ~Flags;
        stack.back() |= VisitedLeft;
        if (TreeTy* R = Current->getRight())
          stack.push_back(reinterpret_cast<uintptr_t>(R) | VisitedRight);
        break;
      default:
        llvm_unreachable("Unreachable.");
    }
    return *this;
  }
};

template <typename ImutInfo>
class ImutAVLTreeInOrderIterator {
  typedef ImutAVLTreeGenericIterator<ImutInfo> InternalIteratorTy;
  InternalIteratorTy InternalItr;

public:
  typedef ImutAVLTree<ImutInfo> TreeTy;
  typedef ImutAVLTreeInOrderIterator<ImutInfo> _Self;

  ImutAVLTreeInOrderIterator(const TreeTy* Root) : InternalItr(Root) {
    if (Root) operator++(); // Advance to first element.
  }

  ImutAVLTreeInOrderIterator() : InternalItr() {}

  inline bool operator==(const _Self& x) const {
    return InternalItr == x.InternalItr;
  }

  inline bool operator!=(const _Self& x) const { return !operator==(x); }

  inline TreeTy* operator*() const { return *InternalItr; }
  inline TreeTy* operator->() const { return *InternalItr; }

  inline _Self& operator++() {
    do ++InternalItr;
    while (!InternalItr.atEnd() &&
           InternalItr.getVisitState() != InternalIteratorTy::VisitedLeft);

    return *this;
  }

  inline _Self& operator--() {
    do --InternalItr;
    while (!InternalItr.atBeginning() &&
           InternalItr.getVisitState() != InternalIteratorTy::VisitedLeft);

    return *this;
  }

  inline void skipSubTree() {
    InternalItr.skipToParent();

    while (!InternalItr.atEnd() &&
           InternalItr.getVisitState() != InternalIteratorTy::VisitedLeft)
      ++InternalItr;
  }
};

//===----------------------------------------------------------------------===//
// Trait classes for Profile information.
//===----------------------------------------------------------------------===//

/// Generic profile template.  The default behavior is to invoke the
/// profile method of an object.  Specializations for primitive integers
/// and generic handling of pointers is done below.
template <typename T>
struct ImutProfileInfo {
  typedef const T  value_type;
  typedef const T& value_type_ref;

  static inline void Profile(FoldingSetNodeID& ID, value_type_ref X) {
    FoldingSetTrait<T>::Profile(X,ID);
  }
};

/// Profile traits for integers.
template <typename T>
struct ImutProfileInteger {
  typedef const T  value_type;
  typedef const T& value_type_ref;

  static inline void Profile(FoldingSetNodeID& ID, value_type_ref X) {
    ID.AddInteger(X);
  }
};

#define PROFILE_INTEGER_INFO(X)\
template<> struct ImutProfileInfo<X> : ImutProfileInteger<X> {};

PROFILE_INTEGER_INFO(char)
PROFILE_INTEGER_INFO(unsigned char)
PROFILE_INTEGER_INFO(short)
PROFILE_INTEGER_INFO(unsigned short)
PROFILE_INTEGER_INFO(unsigned)
PROFILE_INTEGER_INFO(signed)
PROFILE_INTEGER_INFO(long)
PROFILE_INTEGER_INFO(unsigned long)
PROFILE_INTEGER_INFO(long long)
PROFILE_INTEGER_INFO(unsigned long long)

#undef PROFILE_INTEGER_INFO

/// Generic profile trait for pointer types.  We treat pointers as
/// references to unique objects.
template <typename T>
struct ImutProfileInfo<T*> {
  typedef const T*   value_type;
  typedef value_type value_type_ref;

  static inline void Profile(FoldingSetNodeID &ID, value_type_ref X) {
    ID.AddPointer(X);
  }
};

//===----------------------------------------------------------------------===//
// Trait classes that contain element comparison operators and type
//  definitions used by ImutAVLTree, ImmutableSet, and ImmutableMap.  These
//  inherit from the profile traits (ImutProfileInfo) to include operations
//  for element profiling.
//===----------------------------------------------------------------------===//


/// ImutContainerInfo - Generic definition of comparison operations for
///   elements of immutable containers that defaults to using
///   std::equal_to<> and std::less<> to perform comparison of elements.
template <typename T>
struct ImutContainerInfo : public ImutProfileInfo<T> {
  typedef typename ImutProfileInfo<T>::value_type      value_type;
  typedef typename ImutProfileInfo<T>::value_type_ref  value_type_ref;
  typedef value_type      key_type;
  typedef value_type_ref  key_type_ref;
  typedef bool            data_type;
  typedef bool            data_type_ref;

  static inline key_type_ref KeyOfValue(value_type_ref D) { return D; }
  static inline data_type_ref DataOfValue(value_type_ref) { return true; }

  static inline bool isEqual(key_type_ref LHS, key_type_ref RHS) {
    return std::equal_to<key_type>()(LHS,RHS);
  }

  static inline bool isLess(key_type_ref LHS, key_type_ref RHS) {
    return std::less<key_type>()(LHS,RHS);
  }

  static inline bool isDataEqual(data_type_ref,data_type_ref) { return true; }
};

/// ImutContainerInfo - Specialization for pointer values to treat pointers
///  as references to unique objects.  Pointers are thus compared by
///  their addresses.
template <typename T>
struct ImutContainerInfo<T*> : public ImutProfileInfo<T*> {
  typedef typename ImutProfileInfo<T*>::value_type      value_type;
  typedef typename ImutProfileInfo<T*>::value_type_ref  value_type_ref;
  typedef value_type      key_type;
  typedef value_type_ref  key_type_ref;
  typedef bool            data_type;
  typedef bool            data_type_ref;

  static inline key_type_ref KeyOfValue(value_type_ref D) { return D; }
  static inline data_type_ref DataOfValue(value_type_ref) { return true; }

  static inline bool isEqual(key_type_ref LHS, key_type_ref RHS) {
    return LHS == RHS;
  }

  static inline bool isLess(key_type_ref LHS, key_type_ref RHS) {
    return LHS < RHS;
  }

  static inline bool isDataEqual(data_type_ref,data_type_ref) { return true; }
};

//===----------------------------------------------------------------------===//
// Immutable Set
//===----------------------------------------------------------------------===//

template <typename ValT, typename ValInfo = ImutContainerInfo<ValT> >
class ImmutableSet {
public:
  typedef typename ValInfo::value_type      value_type;
  typedef typename ValInfo::value_type_ref  value_type_ref;
  typedef ImutAVLTree<ValInfo> TreeTy;

private:
  TreeTy *Root;

public:
  /// Constructs a set from a pointer to a tree root.  In general one
  /// should use a Factory object to create sets instead of directly
  /// invoking the constructor, but there are cases where make this
  /// constructor public is useful.
  explicit ImmutableSet(TreeTy* R) : Root(R) {
    if (Root) { Root->retain(); }
  }
  ImmutableSet(const ImmutableSet &X) : Root(X.Root) {
    if (Root) { Root->retain(); }
  }
  ImmutableSet &operator=(const ImmutableSet &X) {
    if (Root != X.Root) {
      if (X.Root) { X.Root->retain(); }
      if (Root) { Root->release(); }
      Root = X.Root;
    }
    return *this;
  }
  ~ImmutableSet() {
    if (Root) { Root->release(); }
  }

  class Factory {
    typename TreeTy::Factory F;
    const bool Canonicalize;

  public:
    Factory(bool canonicalize = true)
      : Canonicalize(canonicalize) {}

    Factory(BumpPtrAllocator& Alloc, bool canonicalize = true)
      : F(Alloc), Canonicalize(canonicalize) {}

    /// getEmptySet - Returns an immutable set that contains no elements.
    ImmutableSet getEmptySet() {
      return ImmutableSet(F.getEmptyTree());
    }

    /// add - Creates a new immutable set that contains all of the values
    ///  of the original set with the addition of the specified value.  If
    ///  the original set already included the value, then the original set is
    ///  returned and no memory is allocated.  The time and space complexity
    ///  of this operation is logarithmic in the size of the original set.
    ///  The memory allocated to represent the set is released when the
    ///  factory object that created the set is destroyed.
    ImmutableSet add(ImmutableSet Old, value_type_ref V) {
      TreeTy *NewT = F.add(Old.Root, V);
      return ImmutableSet(Canonicalize ? F.getCanonicalTree(NewT) : NewT);
    }

    /// remove - Creates a new immutable set that contains all of the values
    ///  of the original set with the exception of the specified value.  If
    ///  the original set did not contain the value, the original set is
    ///  returned and no memory is allocated.  The time and space complexity
    ///  of this operation is logarithmic in the size of the original set.
    ///  The memory allocated to represent the set is released when the
    ///  factory object that created the set is destroyed.
    ImmutableSet remove(ImmutableSet Old, value_type_ref V) {
      TreeTy *NewT = F.remove(Old.Root, V);
      return ImmutableSet(Canonicalize ? F.getCanonicalTree(NewT) : NewT);
    }

    BumpPtrAllocator& getAllocator() { return F.getAllocator(); }

    typename TreeTy::Factory *getTreeFactory() const {
      return const_cast<typename TreeTy::Factory *>(&F);
    }

  private:
    Factory(const Factory& RHS) LLVM_DELETED_FUNCTION;
    void operator=(const Factory& RHS) LLVM_DELETED_FUNCTION;
  };

  friend class Factory;

  /// Returns true if the set contains the specified value.
  bool contains(value_type_ref V) const {
    return Root ? Root->contains(V) : false;
  }

  bool operator==(const ImmutableSet &RHS) const {
    return Root && RHS.Root ? Root->isEqual(*RHS.Root) : Root == RHS.Root;
  }

  bool operator!=(const ImmutableSet &RHS) const {
    return Root && RHS.Root ? Root->isNotEqual(*RHS.Root) : Root != RHS.Root;
  }

  TreeTy *getRoot() {
    if (Root) { Root->retain(); }
    return Root;
  }

  TreeTy *getRootWithoutRetain() const {
    return Root;
  }

  /// isEmpty - Return true if the set contains no elements.
  bool isEmpty() const { return !Root; }

  /// isSingleton - Return true if the set contains exactly one element.
  ///   This method runs in constant time.
  bool isSingleton() const { return getHeight() == 1; }

  template <typename Callback>
  void foreach(Callback& C) { if (Root) Root->foreach(C); }

  template <typename Callback>
  void foreach() { if (Root) { Callback C; Root->foreach(C); } }

  //===--------------------------------------------------===//
  // Iterators.
  //===--------------------------------------------------===//

  class iterator {
    typename TreeTy::iterator itr;

    iterator() {}
    iterator(TreeTy* t) : itr(t) {}
    friend class ImmutableSet<ValT,ValInfo>;

  public:
    typedef typename ImmutableSet<ValT,ValInfo>::value_type value_type;
    typedef typename ImmutableSet<ValT,ValInfo>::value_type_ref reference;
    typedef typename iterator::value_type *pointer;
    typedef std::bidirectional_iterator_tag iterator_category;

    typename iterator::reference operator*() const { return itr->getValue(); }
    typename iterator::pointer   operator->() const { return &(operator*()); }

    iterator& operator++() { ++itr; return *this; }
    iterator  operator++(int) { iterator tmp(*this); ++itr; return tmp; }
    iterator& operator--() { --itr; return *this; }
    iterator  operator--(int) { iterator tmp(*this); --itr; return tmp; }

    bool operator==(const iterator& RHS) const { return RHS.itr == itr; }
    bool operator!=(const iterator& RHS) const { return RHS.itr != itr; }
  };

  iterator begin() const { return iterator(Root); }
  iterator end() const { return iterator(); }

  //===--------------------------------------------------===//
  // Utility methods.
  //===--------------------------------------------------===//

  unsigned getHeight() const { return Root ? Root->getHeight() : 0; }

  static inline void Profile(FoldingSetNodeID& ID, const ImmutableSet& S) {
    ID.AddPointer(S.Root);
  }

  inline void Profile(FoldingSetNodeID& ID) const {
    return Profile(ID,*this);
  }

  //===--------------------------------------------------===//
  // For testing.
  //===--------------------------------------------------===//

  void validateTree() const { if (Root) Root->validateTree(); }
};

// NOTE: This may some day replace the current ImmutableSet.
template <typename ValT, typename ValInfo = ImutContainerInfo<ValT> >
class ImmutableSetRef {
public:
  typedef typename ValInfo::value_type      value_type;
  typedef typename ValInfo::value_type_ref  value_type_ref;
  typedef ImutAVLTree<ValInfo> TreeTy;
  typedef typename TreeTy::Factory          FactoryTy;

private:
  TreeTy *Root;
  FactoryTy *Factory;

public:
  /// Constructs a set from a pointer to a tree root.  In general one
  /// should use a Factory object to create sets instead of directly
  /// invoking the constructor, but there are cases where make this
  /// constructor public is useful.
  explicit ImmutableSetRef(TreeTy* R, FactoryTy *F)
    : Root(R),
      Factory(F) {
    if (Root) { Root->retain(); }
  }
  ImmutableSetRef(const ImmutableSetRef &X)
    : Root(X.Root),
      Factory(X.Factory) {
    if (Root) { Root->retain(); }
  }
  ImmutableSetRef &operator=(const ImmutableSetRef &X) {
    if (Root != X.Root) {
      if (X.Root) { X.Root->retain(); }
      if (Root) { Root->release(); }
      Root = X.Root;
      Factory = X.Factory;
    }
    return *this;
  }
  ~ImmutableSetRef() {
    if (Root) { Root->release(); }
  }

  static inline ImmutableSetRef getEmptySet(FactoryTy *F) {
    return ImmutableSetRef(0, F);
  }

  ImmutableSetRef add(value_type_ref V) {
    return ImmutableSetRef(Factory->add(Root, V), Factory);
  }

  ImmutableSetRef remove(value_type_ref V) {
    return ImmutableSetRef(Factory->remove(Root, V), Factory);
  }

  /// Returns true if the set contains the specified value.
  bool contains(value_type_ref V) const {
    return Root ? Root->contains(V) : false;
  }

  ImmutableSet<ValT> asImmutableSet(bool canonicalize = true) const {
    return ImmutableSet<ValT>(canonicalize ?
                              Factory->getCanonicalTree(Root) : Root);
  }

  TreeTy *getRootWithoutRetain() const {
    return Root;
  }

  bool operator==(const ImmutableSetRef &RHS) const {
    return Root && RHS.Root ? Root->isEqual(*RHS.Root) : Root == RHS.Root;
  }

  bool operator!=(const ImmutableSetRef &RHS) const {
    return Root && RHS.Root ? Root->isNotEqual(*RHS.Root) : Root != RHS.Root;
  }

  /// isEmpty - Return true if the set contains no elements.
  bool isEmpty() const { return !Root; }

  /// isSingleton - Return true if the set contains exactly one element.
  ///   This method runs in constant time.
  bool isSingleton() const { return getHeight() == 1; }

  //===--------------------------------------------------===//
  // Iterators.
  //===--------------------------------------------------===//

  class iterator {
    typename TreeTy::iterator itr;
    iterator(TreeTy* t) : itr(t) {}
    friend class ImmutableSetRef<ValT,ValInfo>;
  public:
    iterator() {}
    inline value_type_ref operator*() const { return itr->getValue(); }
    inline iterator& operator++() { ++itr; return *this; }
    inline iterator  operator++(int) { iterator tmp(*this); ++itr; return tmp; }
    inline iterator& operator--() { --itr; return *this; }
    inline iterator  operator--(int) { iterator tmp(*this); --itr; return tmp; }
    inline bool operator==(const iterator& RHS) const { return RHS.itr == itr; }
    inline bool operator!=(const iterator& RHS) const { return RHS.itr != itr; }
    inline value_type *operator->() const { return &(operator*()); }
  };

  iterator begin() const { return iterator(Root); }
  iterator end() const { return iterator(); }

  //===--------------------------------------------------===//
  // Utility methods.
  //===--------------------------------------------------===//

  unsigned getHeight() const { return Root ? Root->getHeight() : 0; }

  static inline void Profile(FoldingSetNodeID& ID, const ImmutableSetRef& S) {
    ID.AddPointer(S.Root);
  }

  inline void Profile(FoldingSetNodeID& ID) const {
    return Profile(ID,*this);
  }

  //===--------------------------------------------------===//
  // For testing.
  //===--------------------------------------------------===//

  void validateTree() const { if (Root) Root->validateTree(); }
};

} // end namespace llvm

#endif
