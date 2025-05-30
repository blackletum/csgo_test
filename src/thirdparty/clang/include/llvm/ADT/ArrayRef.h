﻿//===--- ArrayRef.h - Array Reference Wrapper -------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_ADT_ARRAYREF_H
#define LLVM_ADT_ARRAYREF_H

#include "llvm/ADT/SmallVector.h"
#include <vector>

namespace llvm {

  /// ArrayRef - Represent a constant reference to an array (0 or more elements
  /// consecutively in memory), i.e. a start pointer and a length.  It allows
  /// various APIs to take consecutive elements easily and conveniently.
  ///
  /// This class does not own the underlying data, it is expected to be used in
  /// situations where the data resides in some other buffer, whose lifetime
  /// extends past that of the ArrayRef. For this reason, it is not in general
  /// safe to store an ArrayRef.
  ///
  /// This is intended to be trivially copyable, so it should be passed by
  /// value.
  template<typename T>
  class ArrayRef {
  public:
    typedef const T *iterator;
    typedef const T *const_iterator;
    typedef size_t size_type;

    typedef std::reverse_iterator<iterator> reverse_iterator;

  private:
    /// The start of the array, in an external buffer.
    const T *Data;

    /// The number of elements.
    size_type Length;

  public:
    /// @name Constructors
    /// @{

    /// Construct an empty ArrayRef.
    /*implicit*/ ArrayRef() : Data(0), Length(0) {}

    /// Construct an ArrayRef from a single element.
    /*implicit*/ ArrayRef(const T &OneElt)
      : Data(&OneElt), Length(1) {}

    /// Construct an ArrayRef from a pointer and length.
    /*implicit*/ ArrayRef(const T *data, size_t length)
      : Data(data), Length(length) {}

    /// Construct an ArrayRef from a range.
    ArrayRef(const T *begin, const T *end)
      : Data(begin), Length(end - begin) {}

    /// Construct an ArrayRef from a SmallVector. This is templated in order to
    /// avoid instantiating SmallVectorTemplateCommon<T> whenever we
    /// copy-construct an ArrayRef.
    template<typename U>
    /*implicit*/ ArrayRef(const SmallVectorTemplateCommon<T, U> &Vec)
      : Data(Vec.data()), Length(Vec.size()) {
    }

    /// Construct an ArrayRef from a std::vector.
    template<typename A>
    /*implicit*/ ArrayRef(const std::vector<T, A> &Vec)
      : Data(Vec.empty() ? (T*)0 : &Vec[0]), Length(Vec.size()) {}

    /// Construct an ArrayRef from a C array.
    template <size_t N>
    /*implicit*/ ArrayRef(const T (&Arr)[N])
      : Data(Arr), Length(N) {}

    /// @}
    /// @name Simple Operations
    /// @{

    iterator begin() const { return Data; }
    iterator end() const { return Data + Length; }

    reverse_iterator rbegin() const { return reverse_iterator(end()); }
    reverse_iterator rend() const { return reverse_iterator(begin()); }

    /// empty - Check if the array is empty.
    bool empty() const { return Length == 0; }

    const T *data() const { return Data; }

    /// size - Get the array size.
    size_t size() const { return Length; }

    /// front - Get the first element.
    const T &front() const {
      assert(!empty());
      return Data[0];
    }

    /// back - Get the last element.
    const T &back() const {
      assert(!empty());
      return Data[Length-1];
    }

    /// equals - Check for element-wise equality.
    bool equals(ArrayRef RHS) const {
      if (Length != RHS.Length)
        return false;
      for (size_type i = 0; i != Length; i++)
        if (Data[i] != RHS.Data[i])
          return false;
      return true;
    }

    /// slice(n) - Chop off the first N elements of the array.
    ArrayRef<T> slice(unsigned N) const {
      assert(N <= size() && "Invalid specifier");
      return ArrayRef<T>(data()+N, size()-N);
    }

    /// slice(n, m) - Chop off the first N elements of the array, and keep M
    /// elements in the array.
    ArrayRef<T> slice(unsigned N, unsigned M) const {
      assert(N+M <= size() && "Invalid specifier");
      return ArrayRef<T>(data()+N, M);
    }

    /// @}
    /// @name Operator Overloads
    /// @{
    const T &operator[](size_t Index) const {
      assert(Index < Length && "Invalid index!");
      return Data[Index];
    }

    /// @}
    /// @name Expensive Operations
    /// @{
    std::vector<T> vec() const {
      return std::vector<T>(Data, Data+Length);
    }

    /// @}
    /// @name Conversion operators
    /// @{
    operator std::vector<T>() const {
      return std::vector<T>(Data, Data+Length);
    }

    /// @}
  };

  /// MutableArrayRef - Represent a mutable reference to an array (0 or more
  /// elements consecutively in memory), i.e. a start pointer and a length.  It
  /// allows various APIs to take and modify consecutive elements easily and
  /// conveniently.
  ///
  /// This class does not own the underlying data, it is expected to be used in
  /// situations where the data resides in some other buffer, whose lifetime
  /// extends past that of the MutableArrayRef. For this reason, it is not in
  /// general safe to store a MutableArrayRef.
  ///
  /// This is intended to be trivially copyable, so it should be passed by
  /// value.
  template<typename T>
  class MutableArrayRef : public ArrayRef<T> {
  public:
    typedef T *iterator;

    /// Construct an empty ArrayRef.
    /*implicit*/ MutableArrayRef() : ArrayRef<T>() {}

    /// Construct an MutableArrayRef from a single element.
    /*implicit*/ MutableArrayRef(T &OneElt) : ArrayRef<T>(OneElt) {}

    /// Construct an MutableArrayRef from a pointer and length.
    /*implicit*/ MutableArrayRef(T *data, size_t length)
      : ArrayRef<T>(data, length) {}

    /// Construct an MutableArrayRef from a range.
    MutableArrayRef(T *begin, T *end) : ArrayRef<T>(begin, end) {}

    /// Construct an MutableArrayRef from a SmallVector.
    /*implicit*/ MutableArrayRef(SmallVectorImpl<T> &Vec)
    : ArrayRef<T>(Vec) {}

    /// Construct a MutableArrayRef from a std::vector.
    /*implicit*/ MutableArrayRef(std::vector<T> &Vec)
    : ArrayRef<T>(Vec) {}

    /// Construct an MutableArrayRef from a C array.
    template <size_t N>
    /*implicit*/ MutableArrayRef(T (&Arr)[N])
      : ArrayRef<T>(Arr) {}

    T *data() const { return const_cast<T*>(ArrayRef<T>::data()); }

    iterator begin() const { return data(); }
    iterator end() const { return data() + this->size(); }

    /// front - Get the first element.
    T &front() const {
      assert(!this->empty());
      return data()[0];
    }

    /// back - Get the last element.
    T &back() const {
      assert(!this->empty());
      return data()[this->size()-1];
    }

    /// slice(n) - Chop off the first N elements of the array.
    MutableArrayRef<T> slice(unsigned N) const {
      assert(N <= this->size() && "Invalid specifier");
      return MutableArrayRef<T>(data()+N, this->size()-N);
    }

    /// slice(n, m) - Chop off the first N elements of the array, and keep M
    /// elements in the array.
    MutableArrayRef<T> slice(unsigned N, unsigned M) const {
      assert(N+M <= this->size() && "Invalid specifier");
      return MutableArrayRef<T>(data()+N, M);
    }

    /// @}
    /// @name Operator Overloads
    /// @{
    T &operator[](size_t Index) const {
      assert(Index < this->size() && "Invalid index!");
      return data()[Index];
    }
  };

  /// @name ArrayRef Convenience constructors
  /// @{

  /// Construct an ArrayRef from a single element.
  template<typename T>
  ArrayRef<T> makeArrayRef(const T &OneElt) {
    return OneElt;
  }

  /// Construct an ArrayRef from a pointer and length.
  template<typename T>
  ArrayRef<T> makeArrayRef(const T *data, size_t length) {
    return ArrayRef<T>(data, length);
  }

  /// Construct an ArrayRef from a range.
  template<typename T>
  ArrayRef<T> makeArrayRef(const T *begin, const T *end) {
    return ArrayRef<T>(begin, end);
  }

  /// Construct an ArrayRef from a SmallVector.
  template <typename T>
  ArrayRef<T> makeArrayRef(const SmallVectorImpl<T> &Vec) {
    return Vec;
  }

  /// Construct an ArrayRef from a SmallVector.
  template <typename T, unsigned N>
  ArrayRef<T> makeArrayRef(const SmallVector<T, N> &Vec) {
    return Vec;
  }

  /// Construct an ArrayRef from a std::vector.
  template<typename T>
  ArrayRef<T> makeArrayRef(const std::vector<T> &Vec) {
    return Vec;
  }

  /// Construct an ArrayRef from a C array.
  template<typename T, size_t N>
  ArrayRef<T> makeArrayRef(const T (&Arr)[N]) {
    return ArrayRef<T>(Arr);
  }

  /// @}
  /// @name ArrayRef Comparison Operators
  /// @{

  template<typename T>
  inline bool operator==(ArrayRef<T> LHS, ArrayRef<T> RHS) {
    return LHS.equals(RHS);
  }

  template<typename T>
  inline bool operator!=(ArrayRef<T> LHS, ArrayRef<T> RHS) {
    return !(LHS == RHS);
  }

  /// @}

  // ArrayRefs can be treated like a POD type.
  template <typename T> struct isPodLike;
  template <typename T> struct isPodLike<ArrayRef<T> > {
    static const bool value = true;
  };
}

#endif
