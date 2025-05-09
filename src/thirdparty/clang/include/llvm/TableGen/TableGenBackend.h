﻿//===- llvm/TableGen/TableGenBackend.h - Backend utilities ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Useful utilities for TableGen backends.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TABLEGEN_TABLEGENBACKEND_H
#define LLVM_TABLEGEN_TABLEGENBACKEND_H

#include "llvm/ADT/StringRef.h"

namespace llvm {

class raw_ostream;

/// emitSourceFileHeader - Output a LLVM style file header to the specified
/// raw_ostream.
void emitSourceFileHeader(StringRef Desc, raw_ostream &OS);

} // End llvm namespace

#endif
