﻿// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: google/protobuf/unittest_import_public_lite.proto

#if _MSC_VER >= 1300
#ifndef _PS3
#pragma warning(push)
#pragma warning(disable : 4530)	// warning C4530: C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc (disabled due to std headers having exception syntax)
#pragma warning(disable : 4512)	// warning C4512: assignment operator could not be generated
#pragma warning(disable : 4244)	// warning C4244:  warning C4244: '=' : conversion from '__w64 int' to 'int', possible loss of data
#pragma warning(disable : 4267)	// warning C4267: 'argument' : conversion from 'size_t' to 'int', possible loss of data
#pragma warning(disable : 4125)	// warning C4125: decimal digit terminates octal escape sequence
#pragma warning(disable : 4127)	// warning C4127: conditional expression is constant
#pragma warning(disable : 4100)	// warning C4100: 'op' : unreferenced formal parameter
#endif // _PS3
#endif // _MSC_VER
#if _GNUC
#ifndef _PS3
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#endif // _PS3
#endif // _GNUC

#ifndef PROTOBUF_google_2fprotobuf_2funittest_5fimport_5fpublic_5flite_2eproto__INCLUDED
#define PROTOBUF_google_2fprotobuf_2funittest_5fimport_5fpublic_5flite_2eproto__INCLUDED

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 2005000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 2005000 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/message_lite.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
// @@protoc_insertion_point(includes)

namespace protobuf_unittest_import {

// Internal implementation detail -- do not call these.
void  protobuf_AddDesc_google_2fprotobuf_2funittest_5fimport_5fpublic_5flite_2eproto();
void protobuf_AssignDesc_google_2fprotobuf_2funittest_5fimport_5fpublic_5flite_2eproto();
void protobuf_ShutdownFile_google_2fprotobuf_2funittest_5fimport_5fpublic_5flite_2eproto();

class PublicImportMessageLite;

// ===================================================================

class PublicImportMessageLite : public ::google::protobuf::MessageLite {
 public:
  PublicImportMessageLite();
  virtual ~PublicImportMessageLite();

  PublicImportMessageLite(const PublicImportMessageLite& from);

  inline PublicImportMessageLite& operator=(const PublicImportMessageLite& from) {
    CopyFrom(from);
    return *this;
  }

  static const PublicImportMessageLite& default_instance();

  #ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  // Returns the internal default instance pointer. This function can
  // return NULL thus should not be used by the user. This is intended
  // for Protobuf internal code. Please use default_instance() declared
  // above instead.
  static inline const PublicImportMessageLite* internal_default_instance() {
    return default_instance_;
  }
  #endif

  void Swap(PublicImportMessageLite* other);

  // implements Message ----------------------------------------------

  PublicImportMessageLite* New() const;
  void CheckTypeAndMergeFrom(const ::google::protobuf::MessageLite& from);
  void CopyFrom(const PublicImportMessageLite& from);
  void MergeFrom(const PublicImportMessageLite& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:

  ::std::string GetTypeName() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // optional int32 e = 1;
  inline bool has_e() const;
  inline void clear_e();
  static const int kEFieldNumber = 1;
  inline ::google::protobuf::int32 e() const;
  inline void set_e(::google::protobuf::int32 value);

  // @@protoc_insertion_point(class_scope:protobuf_unittest_import.PublicImportMessageLite)
 private:
  inline void set_has_e();
  inline void clear_has_e();

  ::google::protobuf::int32 e_;

  mutable int _cached_size_;
  ::google::protobuf::uint32 _has_bits_[(1 + 31) / 32];

  #ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  friend void  protobuf_AddDesc_google_2fprotobuf_2funittest_5fimport_5fpublic_5flite_2eproto_impl();
  #else
  friend void  protobuf_AddDesc_google_2fprotobuf_2funittest_5fimport_5fpublic_5flite_2eproto();
  #endif
  friend void protobuf_AssignDesc_google_2fprotobuf_2funittest_5fimport_5fpublic_5flite_2eproto();
  friend void protobuf_ShutdownFile_google_2fprotobuf_2funittest_5fimport_5fpublic_5flite_2eproto();

  void InitAsDefaultInstance();
  static PublicImportMessageLite* default_instance_;
};
// ===================================================================


// ===================================================================

// PublicImportMessageLite

// optional int32 e = 1;
inline bool PublicImportMessageLite::has_e() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void PublicImportMessageLite::set_has_e() {
  _has_bits_[0] |= 0x00000001u;
}
inline void PublicImportMessageLite::clear_has_e() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void PublicImportMessageLite::clear_e() {
  e_ = 0;
  clear_has_e();
}
inline ::google::protobuf::int32 PublicImportMessageLite::e() const {
  return e_;
}
inline void PublicImportMessageLite::set_e(::google::protobuf::int32 value) {
  set_has_e();
  e_ = value;
}


// @@protoc_insertion_point(namespace_scope)

}  // namespace protobuf_unittest_import

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_google_2fprotobuf_2funittest_5fimport_5fpublic_5flite_2eproto__INCLUDED
