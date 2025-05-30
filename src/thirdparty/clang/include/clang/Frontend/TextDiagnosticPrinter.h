﻿//===--- TextDiagnosticPrinter.h - Text Diagnostic Client -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This is a concrete diagnostic client, which prints the diagnostics to
// standard error.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_FRONTEND_TEXT_DIAGNOSTIC_PRINTER_H_
#define LLVM_CLANG_FRONTEND_TEXT_DIAGNOSTIC_PRINTER_H_

#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/LLVM.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/ADT/OwningPtr.h"

namespace clang {
class DiagnosticOptions;
class LangOptions;
class TextDiagnostic;

class TextDiagnosticPrinter : public DiagnosticConsumer {
  raw_ostream &OS;
  IntrusiveRefCntPtr<DiagnosticOptions> DiagOpts;

  /// \brief Handle to the currently active text diagnostic emitter.
  OwningPtr<TextDiagnostic> TextDiag;

  /// A string to prefix to error messages.
  std::string Prefix;

  unsigned OwnsOutputStream : 1;

public:
  TextDiagnosticPrinter(raw_ostream &os, DiagnosticOptions *diags,
                        bool OwnsOutputStream = false);
  virtual ~TextDiagnosticPrinter();

  /// setPrefix - Set the diagnostic printer prefix string, which will be
  /// printed at the start of any diagnostics. If empty, no prefix string is
  /// used.
  void setPrefix(std::string Value) { Prefix = Value; }

  void BeginSourceFile(const LangOptions &LO, const Preprocessor *PP);
  void EndSourceFile();
  void HandleDiagnostic(DiagnosticsEngine::Level Level, const Diagnostic &Info);
  DiagnosticConsumer *clone(DiagnosticsEngine &Diags) const;
};

} // end namespace clang

#endif
