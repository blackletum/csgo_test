﻿//===-- llvm/ADT/EquivalenceClasses.h - Generic Equiv. Classes --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Generic implementation of equivalence classes through the use Tarjan's
// efficient union-find algorithm.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_ADT_EQUIVALENCECLASSES_H
#define LLVM_ADT_EQUIVALENCECLASSES_H

#include "llvm/Support/DataTypes.h"
#include <cassert>
#include <set>

namespace llvm {

/// EquivalenceClasses - This represents a collection of equivalence classes and
/// supports three efficient operations: insert an element into a class of its
/// own, union two classes, and find the class for a given element.  In
/// addition to these modification methods, it is possible to iterate over all
/// of the equivalence classes and all of the elements in a class.
///
/// This implementation is an efficient implementation that only stores one copy
/// of the element being indexed per entry in the set, and allows any arbitrary
/// type to be indexed (as long as it can be ordered with operator<).
///
/// Here is a simple example using integers:
///
/// \code
///  EquivalenceClasses<int> EC;
///  EC.unionSets(1, 2);                // insert 1, 2 into the same set
///  EC.insert(4); EC.insert(5);        // insert 4, 5 into own sets
///  EC.unionSets(5, 1);                // merge the set for 1 with 5's set.
///
///  for (EquivalenceClasses<int>::iterator I = EC.begin(), E = EC.end();
///       I != E; ++I) {           // Iterate over all of the equivalence sets.
///    if (!I->isLeader()) continue;   // Ignore non-leader sets.
///    for (EquivalenceClasses<int>::member_iterator MI = EC.member_begin(I);
///         MI != EC.member_end(); ++MI)   // Loop over members in this set.
///      cerr << *MI << " ";  // Print member.
///    cerr << "\n";   // Finish set.
///  }
/// \endcode
///
/// This example prints:
///   4
///   5 1 2
///
template <class ElemTy>
class EquivalenceClasses {
  /// ECValue - The EquivalenceClasses data structure is just a set of these.
  /// Each of these represents a relation for a value.  First it stores the
  /// value itself, which provides the ordering that the set queries.  Next, it
  /// provides a "next pointer", which is used to enumerate all of the elements
  /// in the unioned set.  Finally, it defines either a "end of list pointer" or
  /// "leader pointer" depending on whether the value itself is a leader.  A
  /// "leader pointer" points to the node that is the leader for this element,
  /// if the node is not a leader.  A "end of list pointer" points to the last
  /// node in the list of members of this list.  Whether or not a node is a
  /// leader is determined by a bit stolen from one of the pointers.
  class ECValue {
    friend class EquivalenceClasses;
    mutable const ECValue *Leader, *Next;
    ElemTy Data;
    // ECValue ctor - Start out with EndOfList pointing to this node, Next is
    // Null, isLeader = true.
    ECValue(const ElemTy &Elt)
      : Leader(this), Next((ECValue*)(intptr_t)1), Data(Elt) {}

    const ECValue *getLeader() const {
      if (isLeader()) return this;
      if (Leader->isLeader()) return Leader;
      // Path compression.
      return Leader = Leader->getLeader();
    }
    const ECValue *getEndOfList() const {
      assert(isLeader() && "Cannot get the end of a list for a non-leader!");
      return Leader;
    }

    void setNext(const ECValue *NewNext) const {
      assert(getNext() == 0 && "Already has a next pointer!");
      Next = (const ECValue*)((intptr_t)NewNext | (intptr_t)isLeader());
    }
  public:
    ECValue(const ECValue &RHS) : Leader(this), Next((ECValue*)(intptr_t)1),
                                  Data(RHS.Data) {
      // Only support copying of singleton nodes.
      assert(RHS.isLeader() && RHS.getNext() == 0 && "Not a singleton!");
    }

    bool operator<(const ECValue &UFN) const { return Data < UFN.Data; }

    bool isLeader() const { return (intptr_t)Next & 1; }
    const ElemTy &getData() const { return Data; }

    const ECValue *getNext() const {
      return (ECValue*)((intptr_t)Next & ~(intptr_t)1);
    }

    template<typename T>
    bool operator<(const T &Val) const { return Data < Val; }
  };

  /// TheMapping - This implicitly provides a mapping from ElemTy values to the
  /// ECValues, it just keeps the key as part of the value.
  std::set<ECValue> TheMapping;

public:
  EquivalenceClasses() {}
  EquivalenceClasses(const EquivalenceClasses &RHS) {
    operator=(RHS);
  }

  const EquivalenceClasses &operator=(const EquivalenceClasses &RHS) {
    TheMapping.clear();
    for (iterator I = RHS.begin(), E = RHS.end(); I != E; ++I)
      if (I->isLeader()) {
        member_iterator MI = RHS.member_begin(I);
        member_iterator LeaderIt = member_begin(insert(*MI));
        for (++MI; MI != member_end(); ++MI)
          unionSets(LeaderIt, member_begin(insert(*MI)));
      }
    return *this;
  }

  //===--------------------------------------------------------------------===//
  // Inspection methods
  //

  /// iterator* - Provides a way to iterate over all values in the set.
  typedef typename std::set<ECValue>::const_iterator iterator;
  iterator begin() const { return TheMapping.begin(); }
  iterator end() const { return TheMapping.end(); }

  bool empty() const { return TheMapping.empty(); }

  /// member_* Iterate over the members of an equivalence class.
  ///
  class member_iterator;
  member_iterator member_begin(iterator I) const {
    // Only leaders provide anything to iterate over.
    return member_iterator(I->isLeader() ? &*I : 0);
  }
  member_iterator member_end() const {
    return member_iterator(0);
  }

  /// findValue - Return an iterator to the specified value.  If it does not
  /// exist, end() is returned.
  iterator findValue(const ElemTy &V) const {
    return TheMapping.find(V);
  }

  /// getLeaderValue - Return the leader for the specified value that is in the
  /// set.  It is an error to call this method for a value that is not yet in
  /// the set.  For that, call getOrInsertLeaderValue(V).
  const ElemTy &getLeaderValue(const ElemTy &V) const {
    member_iterator MI = findLeader(V);
    assert(MI != member_end() && "Value is not in the set!");
    return *MI;
  }

  /// getOrInsertLeaderValue - Return the leader for the specified value that is
  /// in the set.  If the member is not in the set, it is inserted, then
  /// returned.
  const ElemTy &getOrInsertLeaderValue(const ElemTy &V) {
    member_iterator MI = findLeader(insert(V));
    assert(MI != member_end() && "Value is not in the set!");
    return *MI;
  }

  /// getNumClasses - Return the number of equivalence classes in this set.
  /// Note that this is a linear time operation.
  unsigned getNumClasses() const {
    unsigned NC = 0;
    for (iterator I = begin(), E = end(); I != E; ++I)
      if (I->isLeader()) ++NC;
    return NC;
  }


  //===--------------------------------------------------------------------===//
  // Mutation methods

  /// insert - Insert a new value into the union/find set, ignoring the request
  /// if the value already exists.
  iterator insert(const ElemTy &Data) {
    return TheMapping.insert(ECValue(Data)).first;
  }

  /// findLeader - Given a value in the set, return a member iterator for the
  /// equivalence class it is in.  This does the path-compression part that
  /// makes union-find "union findy".  This returns an end iterator if the value
  /// is not in the equivalence class.
  ///
  member_iterator findLeader(iterator I) const {
    if (I == TheMapping.end()) return member_end();
    return member_iterator(I->getLeader());
  }
  member_iterator findLeader(const ElemTy &V) const {
    return findLeader(TheMapping.find(V));
  }


  /// union - Merge the two equivalence sets for the specified values, inserting
  /// them if they do not already exist in the equivalence set.
  member_iterator unionSets(const ElemTy &V1, const ElemTy &V2) {
    iterator V1I = insert(V1), V2I = insert(V2);
    return unionSets(findLeader(V1I), findLeader(V2I));
  }
  member_iterator unionSets(member_iterator L1, member_iterator L2) {
    assert(L1 != member_end() && L2 != member_end() && "Illegal inputs!");
    if (L1 == L2) return L1;   // Unifying the same two sets, noop.

    // Otherwise, this is a real union operation.  Set the end of the L1 list to
    // point to the L2 leader node.
    const ECValue &L1LV = *L1.Node, &L2LV = *L2.Node;
    L1LV.getEndOfList()->setNext(&L2LV);

    // Update L1LV's end of list pointer.
    L1LV.Leader = L2LV.getEndOfList();

    // Clear L2's leader flag:
    L2LV.Next = L2LV.getNext();

    // L2's leader is now L1.
    L2LV.Leader = &L1LV;
    return L1;
  }

  class member_iterator : public std::iterator<std::forward_iterator_tag,
                                               const ElemTy, ptrdiff_t> {
    typedef std::iterator<std::forward_iterator_tag,
                          const ElemTy, ptrdiff_t> super;
    const ECValue *Node;
    friend class EquivalenceClasses;
  public:
    typedef size_t size_type;
    typedef typename super::pointer pointer;
    typedef typename super::reference reference;

    explicit member_iterator() {}
    explicit member_iterator(const ECValue *N) : Node(N) {}
    member_iterator(const member_iterator &I) : Node(I.Node) {}

    reference operator*() const {
      assert(Node != 0 && "Dereferencing end()!");
      return Node->getData();
    }
    reference operator->() const { return operator*(); }

    member_iterator &operator++() {
      assert(Node != 0 && "++'d off the end of the list!");
      Node = Node->getNext();
      return *this;
    }

    member_iterator operator++(int) {    // postincrement operators.
      member_iterator tmp = *this;
      ++*this;
      return tmp;
    }

    bool operator==(const member_iterator &RHS) const {
      return Node == RHS.Node;
    }
    bool operator!=(const member_iterator &RHS) const {
      return Node != RHS.Node;
    }
  };
};

} // End llvm namespace

#endif
