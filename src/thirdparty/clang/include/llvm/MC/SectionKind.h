﻿//===-- llvm/Target/TargetLoweringObjectFile.h - Object Info ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements classes used to handle lowerings specific to common
// object file formats.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_MC_SECTIONKIND_H
#define LLVM_MC_SECTIONKIND_H

namespace llvm {

/// SectionKind - This is a simple POD value that classifies the properties of
/// a section.  A section is classified into the deepest possible
/// classification, and then the target maps them onto their sections based on
/// what capabilities they have.
///
/// The comments below describe these as if they were an inheritance hierarchy
/// in order to explain the predicates below.
///
class SectionKind {
  enum Kind {
    /// Metadata - Debug info sections or other metadata.
    Metadata,

    /// Text - Text section, used for functions and other executable code.
    Text,

    /// ReadOnly - Data that is never written to at program runtime by the
    /// program or the dynamic linker.  Things in the top-level readonly
    /// SectionKind are not mergeable.
    ReadOnly,

        /// MergableCString - Any null-terminated string which allows merging.
        /// These values are known to end in a nul value of the specified size,
        /// not otherwise contain a nul value, and be mergable.  This allows the
        /// linker to unique the strings if it so desires.

           /// Mergeable1ByteCString - 1 byte mergable, null terminated, string.
           Mergeable1ByteCString,

           /// Mergeable2ByteCString - 2 byte mergable, null terminated, string.
           Mergeable2ByteCString,

           /// Mergeable4ByteCString - 4 byte mergable, null terminated, string.
           Mergeable4ByteCString,

        /// MergeableConst - These are sections for merging fixed-length
        /// constants together.  For example, this can be used to unique
        /// constant pool entries etc.
        MergeableConst,

            /// MergeableConst4 - This is a section used by 4-byte constants,
            /// for example, floats.
            MergeableConst4,

            /// MergeableConst8 - This is a section used by 8-byte constants,
            /// for example, doubles.
            MergeableConst8,

            /// MergeableConst16 - This is a section used by 16-byte constants,
            /// for example, vectors.
            MergeableConst16,

    /// Writeable - This is the base of all segments that need to be written
    /// to during program runtime.

       /// ThreadLocal - This is the base of all TLS segments.  All TLS
       /// objects must be writeable, otherwise there is no reason for them to
       /// be thread local!

           /// ThreadBSS - Zero-initialized TLS data objects.
           ThreadBSS,

           /// ThreadData - Initialized TLS data objects.
           ThreadData,

       /// GlobalWriteableData - Writeable data that is global (not thread
       /// local).

           /// BSS - Zero initialized writeable data.
           BSS,

               /// BSSLocal - This is BSS (zero initialized and writable) data
               /// which has local linkage.
               BSSLocal,

               /// BSSExtern - This is BSS data with normal external linkage.
               BSSExtern,

           /// Common - Data with common linkage.  These represent tentative
           /// definitions, which always have a zero initializer and are never
           /// marked 'constant'.
           Common,

           /// DataRel - This is the most general form of data that is written
           /// to by the program, it can have random relocations to arbitrary
           /// globals.
           DataRel,

               /// DataRelLocal - This is writeable data that has a non-zero
               /// initializer and has relocations in it, but all of the
               /// relocations are known to be within the final linked image
               /// the global is linked into.
               DataRelLocal,

                   /// DataNoRel - This is writeable data that has a non-zero
                   /// initializer, but whose initializer is known to have no
                   /// relocations.
                   DataNoRel,

           /// ReadOnlyWithRel - These are global variables that are never
           /// written to by the program, but that have relocations, so they
           /// must be stuck in a writeable section so that the dynamic linker
           /// can write to them.  If it chooses to, the dynamic linker can
           /// mark the pages these globals end up on as read-only after it is
           /// done with its relocation phase.
           ReadOnlyWithRel,

               /// ReadOnlyWithRelLocal - This is data that is readonly by the
               /// program, but must be writeable so that the dynamic linker
               /// can perform relocations in it.  This is used when we know
               /// that all the relocations are to globals in this final
               /// linked image.
               ReadOnlyWithRelLocal

  } K : 8;
public:

  bool isMetadata() const { return K == Metadata; }
  bool isText() const { return K == Text; }

  bool isReadOnly() const {
    return K == ReadOnly || isMergeableCString() ||
           isMergeableConst();
  }

  bool isMergeableCString() const {
    return K == Mergeable1ByteCString || K == Mergeable2ByteCString ||
           K == Mergeable4ByteCString;
  }
  bool isMergeable1ByteCString() const { return K == Mergeable1ByteCString; }
  bool isMergeable2ByteCString() const { return K == Mergeable2ByteCString; }
  bool isMergeable4ByteCString() const { return K == Mergeable4ByteCString; }

  bool isMergeableConst() const {
    return K == MergeableConst || K == MergeableConst4 ||
           K == MergeableConst8 || K == MergeableConst16;
  }
  bool isMergeableConst4() const { return K == MergeableConst4; }
  bool isMergeableConst8() const { return K == MergeableConst8; }
  bool isMergeableConst16() const { return K == MergeableConst16; }

  bool isWriteable() const {
    return isThreadLocal() || isGlobalWriteableData();
  }

  bool isThreadLocal() const {
    return K == ThreadData || K == ThreadBSS;
  }

  bool isThreadBSS() const { return K == ThreadBSS; }
  bool isThreadData() const { return K == ThreadData; }

  bool isGlobalWriteableData() const {
    return isBSS() || isCommon() || isDataRel() || isReadOnlyWithRel();
  }

  bool isBSS() const { return K == BSS || K == BSSLocal || K == BSSExtern; }
  bool isBSSLocal() const { return K == BSSLocal; }
  bool isBSSExtern() const { return K == BSSExtern; }

  bool isCommon() const { return K == Common; }

  bool isDataRel() const {
    return K == DataRel || K == DataRelLocal || K == DataNoRel;
  }

  bool isDataRelLocal() const {
    return K == DataRelLocal || K == DataNoRel;
  }

  bool isDataNoRel() const { return K == DataNoRel; }

  bool isReadOnlyWithRel() const {
    return K == ReadOnlyWithRel || K == ReadOnlyWithRelLocal;
  }

  bool isReadOnlyWithRelLocal() const {
    return K == ReadOnlyWithRelLocal;
  }
private:
  static SectionKind get(Kind K) {
    SectionKind Res;
    Res.K = K;
    return Res;
  }
public:

  static SectionKind getMetadata() { return get(Metadata); }
  static SectionKind getText() { return get(Text); }
  static SectionKind getReadOnly() { return get(ReadOnly); }
  static SectionKind getMergeable1ByteCString() {
    return get(Mergeable1ByteCString);
  }
  static SectionKind getMergeable2ByteCString() {
    return get(Mergeable2ByteCString);
  }
  static SectionKind getMergeable4ByteCString() {
    return get(Mergeable4ByteCString);
  }
  static SectionKind getMergeableConst() { return get(MergeableConst); }
  static SectionKind getMergeableConst4() { return get(MergeableConst4); }
  static SectionKind getMergeableConst8() { return get(MergeableConst8); }
  static SectionKind getMergeableConst16() { return get(MergeableConst16); }
  static SectionKind getThreadBSS() { return get(ThreadBSS); }
  static SectionKind getThreadData() { return get(ThreadData); }
  static SectionKind getBSS() { return get(BSS); }
  static SectionKind getBSSLocal() { return get(BSSLocal); }
  static SectionKind getBSSExtern() { return get(BSSExtern); }
  static SectionKind getCommon() { return get(Common); }
  static SectionKind getDataRel() { return get(DataRel); }
  static SectionKind getDataRelLocal() { return get(DataRelLocal); }
  static SectionKind getDataNoRel() { return get(DataNoRel); }
  static SectionKind getReadOnlyWithRel() { return get(ReadOnlyWithRel); }
  static SectionKind getReadOnlyWithRelLocal(){
    return get(ReadOnlyWithRelLocal);
  }
};

} // end namespace llvm

#endif
