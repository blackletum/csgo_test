﻿//===- ToolOutputFile.h - Output files for compiler-like tools -----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the tool_output_file class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SUPPORT_TOOLOUTPUTFILE_H
#define LLVM_SUPPORT_TOOLOUTPUTFILE_H

#include "llvm/Support/raw_ostream.h"

namespace llvm {

/// tool_output_file - This class contains a raw_fd_ostream and adds a
/// few extra features commonly needed for compiler-like tool output files:
///   - The file is automatically deleted if the process is killed.
///   - The file is automatically deleted when the tool_output_file
///     object is destroyed unless the client calls keep().
class tool_output_file {
  /// Installer - This class is declared before the raw_fd_ostream so that
  /// it is constructed before the raw_fd_ostream is constructed and
  /// destructed after the raw_fd_ostream is destructed. It installs
  /// cleanups in its constructor and uninstalls them in its destructor.
  class CleanupInstaller {
    /// Filename - The name of the file.
    std::string Filename;
  public:
    /// Keep - The flag which indicates whether we should not delete the file.
    bool Keep;

    explicit CleanupInstaller(const char *filename);
    ~CleanupInstaller();
  } Installer;

  /// OS - The contained stream. This is intentionally declared after
  /// Installer.
  raw_fd_ostream OS;

public:
  /// tool_output_file - This constructor's arguments are passed to
  /// to raw_fd_ostream's constructor.
  tool_output_file(const char *filename, std::string &ErrorInfo,
                   unsigned Flags = 0);

  /// os - Return the contained raw_fd_ostream.
  raw_fd_ostream &os() { return OS; }

  /// keep - Indicate that the tool's job wrt this output file has been
  /// successful and the file should not be deleted.
  void keep() { Installer.Keep = true; }
};

} // end llvm namespace

#endif
