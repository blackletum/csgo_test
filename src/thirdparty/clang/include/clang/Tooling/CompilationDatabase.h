﻿//===--- CompilationDatabase.h - --------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file provides an interface and multiple implementations for
//  CompilationDatabases.
//
//  While C++ refactoring and analysis tools are not compilers, and thus
//  don't run as part of the build system, they need the exact information
//  of a build in order to be able to correctly understand the C++ code of
//  the project. This information is provided via the CompilationDatabase
//  interface.
//
//  To create a CompilationDatabase from a build directory one can call
//  CompilationDatabase::loadFromDirectory(), which deduces the correct
//  compilation database from the root of the build tree.
//
//  See the concrete subclasses of CompilationDatabase for currently supported
//  formats.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLING_COMPILATION_DATABASE_H
#define LLVM_CLANG_TOOLING_COMPILATION_DATABASE_H

#include "clang/Basic/LLVM.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/OwningPtr.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
#include <string>
#include <vector>

namespace clang {
namespace tooling {

/// \brief Specifies the working directory and command of a compilation.
struct CompileCommand {
  CompileCommand() {}
  CompileCommand(Twine Directory, ArrayRef<std::string> CommandLine)
    : Directory(Directory.str()), CommandLine(CommandLine) {}

  /// \brief The working directory the command was executed from.
  std::string Directory;

  /// \brief The command line that was executed.
  std::vector<std::string> CommandLine;
};

/// \brief Interface for compilation databases.
///
/// A compilation database allows the user to retrieve all compile command lines
/// that a specified file is compiled with in a project.
/// The retrieved compile command lines can be used to run clang tools over
/// a subset of the files in a project.
class CompilationDatabase {
public:
  virtual ~CompilationDatabase();

  /// \brief Loads a compilation database from a build directory.
  ///
  /// Looks at the specified 'BuildDirectory' and creates a compilation database
  /// that allows to query compile commands for source files in the
  /// corresponding source tree.
  ///
  /// Returns NULL and sets ErrorMessage if we were not able to build up a
  /// compilation database for the build directory.
  ///
  /// FIXME: Currently only supports JSON compilation databases, which
  /// are named 'compile_commands.json' in the given directory. Extend this
  /// for other build types (like ninja build files).
  static CompilationDatabase *loadFromDirectory(StringRef BuildDirectory,
                                                std::string &ErrorMessage);

  /// \brief Tries to detect a compilation database location and load it.
  ///
  /// Looks for a compilation database in all parent paths of file 'SourceFile'
  /// by calling loadFromDirectory.
  static CompilationDatabase *autoDetectFromSource(StringRef SourceFile,
                                                   std::string &ErrorMessage);

  /// \brief Tries to detect a compilation database location and load it.
  ///
  /// Looks for a compilation database in directory 'SourceDir' and all
  /// its parent paths by calling loadFromDirectory.
  static CompilationDatabase *autoDetectFromDirectory(StringRef SourceDir,
                                                      std::string &ErrorMessage);

  /// \brief Returns all compile commands in which the specified file was
  /// compiled.
  ///
  /// This includes compile comamnds that span multiple source files.
  /// For example, consider a project with the following compilations:
  /// $ clang++ -o test a.cc b.cc t.cc
  /// $ clang++ -o production a.cc b.cc -DPRODUCTION
  /// A compilation database representing the project would return both command
  /// lines for a.cc and b.cc and only the first command line for t.cc.
  virtual std::vector<CompileCommand> getCompileCommands(
    StringRef FilePath) const = 0;

  /// \brief Returns the list of all files available in the compilation database.
  virtual std::vector<std::string> getAllFiles() const = 0;

  /// \brief Returns all compile commands for all the files in the compilation
  /// database.
  virtual std::vector<CompileCommand> getAllCompileCommands() const = 0;
};

/// \brief Interface for compilation database plugins.
///
/// A compilation database plugin allows the user to register custom compilation
/// databases that are picked up as compilation database if the corresponding
/// library is linked in. To register a plugin, declare a static variable like:
///
/// \code
/// static CompilationDatabasePluginRegistry::Add<MyDatabasePlugin>
/// X("my-compilation-database", "Reads my own compilation database");
/// \endcode
class CompilationDatabasePlugin {
public:
  virtual ~CompilationDatabasePlugin();

  /// \brief Loads a compilation database from a build directory.
  ///
  /// \see CompilationDatabase::loadFromDirectory().
  virtual CompilationDatabase *loadFromDirectory(StringRef Directory,
                                                 std::string &ErrorMessage) = 0;
};

/// \brief A compilation database that returns a single compile command line.
///
/// Useful when we want a tool to behave more like a compiler invocation.
class FixedCompilationDatabase : public CompilationDatabase {
public:
  /// \brief Creates a FixedCompilationDatabase from the arguments after "--".
  ///
  /// Parses the given command line for "--". If "--" is found, the rest of
  /// the arguments will make up the command line in the returned
  /// FixedCompilationDatabase.
  /// The arguments after "--" must not include positional parameters or the
  /// argv[0] of the tool. Those will be added by the FixedCompilationDatabase
  /// when a CompileCommand is requested. The argv[0] of the returned command
  /// line will be "clang-tool".
  ///
  /// Returns NULL in case "--" is not found.
  ///
  /// The argument list is meant to be compatible with normal llvm command line
  /// parsing in main methods.
  /// int main(int argc, char **argv) {
  ///   OwningPtr<FixedCompilationDatabase> Compilations(
  ///     FixedCompilationDatabase::loadFromCommandLine(argc, argv));
  ///   cl::ParseCommandLineOptions(argc, argv);
  ///   ...
  /// }
  ///
  /// \param Argc The number of command line arguments - will be changed to
  /// the number of arguments before "--", if "--" was found in the argument
  /// list.
  /// \param Argv Points to the command line arguments.
  /// \param Directory The base directory used in the FixedCompilationDatabase.
  static FixedCompilationDatabase *loadFromCommandLine(int &Argc,
                                                       const char **Argv,
                                                       Twine Directory = ".");

  /// \brief Constructs a compilation data base from a specified directory
  /// and command line.
  FixedCompilationDatabase(Twine Directory, ArrayRef<std::string> CommandLine);

  /// \brief Returns the given compile command.
  ///
  /// Will always return a vector with one entry that contains the directory
  /// and command line specified at construction with "clang-tool" as argv[0]
  /// and 'FilePath' as positional argument.
  virtual std::vector<CompileCommand> getCompileCommands(
    StringRef FilePath) const;

  /// \brief Returns the list of all files available in the compilation database.
  ///
  /// Note: This is always an empty list for the fixed compilation database.
  virtual std::vector<std::string> getAllFiles() const;

  /// \brief Returns all compile commands for all the files in the compilation
  /// database.
  ///
  /// Note: This is always an empty list for the fixed compilation database.
  virtual std::vector<CompileCommand> getAllCompileCommands() const;

private:
  /// This is built up to contain a single entry vector to be returned from
  /// getCompileCommands after adding the positional argument.
  std::vector<CompileCommand> CompileCommands;
};

} // end namespace tooling
} // end namespace clang

#endif // LLVM_CLANG_TOOLING_COMPILATION_DATABASE_H
