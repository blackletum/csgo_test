﻿//===--- HeaderSearchOptions.h ----------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LEX_HEADERSEARCHOPTIONS_H
#define LLVM_CLANG_LEX_HEADERSEARCHOPTIONS_H

#include "clang/Basic/LLVM.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/StringRef.h"
#include <string>
#include <vector>

namespace clang {

namespace frontend {
  /// IncludeDirGroup - Identifiers the group a include entry belongs to, which
  /// represents its relative positive in the search list.  A \#include of a ""
  /// path starts at the -iquote group, then searches the Angled group, then
  /// searches the system group, etc.
  enum IncludeDirGroup {
    Quoted = 0,     ///< '\#include ""' paths, added by 'gcc -iquote'.
    Angled,         ///< Paths for '\#include <>' added by '-I'.
    IndexHeaderMap, ///< Like Angled, but marks header maps used when
                       ///  building frameworks.
    System,         ///< Like Angled, but marks system directories.
    ExternCSystem,  ///< Like System, but headers are implicitly wrapped in
                    ///  extern "C".
    CSystem,        ///< Like System, but only used for C.
    CXXSystem,      ///< Like System, but only used for C++.
    ObjCSystem,     ///< Like System, but only used for ObjC.
    ObjCXXSystem,   ///< Like System, but only used for ObjC++.
    After           ///< Like System, but searched after the system directories.
  };
}

/// HeaderSearchOptions - Helper class for storing options related to the
/// initialization of the HeaderSearch object.
class HeaderSearchOptions : public RefCountedBase<HeaderSearchOptions> {
public:
  struct Entry {
    std::string Path;
    frontend::IncludeDirGroup Group;
    unsigned IsFramework : 1;
    
    /// IgnoreSysRoot - This is false if an absolute path should be treated
    /// relative to the sysroot, or true if it should always be the absolute
    /// path.
    unsigned IgnoreSysRoot : 1;

    Entry(StringRef path, frontend::IncludeDirGroup group, bool isFramework,
          bool ignoreSysRoot)
      : Path(path), Group(group), IsFramework(isFramework),
        IgnoreSysRoot(ignoreSysRoot) {}
  };

  struct SystemHeaderPrefix {
    /// A prefix to be matched against paths in \#include directives.
    std::string Prefix;

    /// True if paths beginning with this prefix should be treated as system
    /// headers.
    bool IsSystemHeader;

    SystemHeaderPrefix(StringRef Prefix, bool IsSystemHeader)
      : Prefix(Prefix), IsSystemHeader(IsSystemHeader) {}
  };

  /// If non-empty, the directory to use as a "virtual system root" for include
  /// paths.
  std::string Sysroot;

  /// User specified include entries.
  std::vector<Entry> UserEntries;

  /// User-specified system header prefixes.
  std::vector<SystemHeaderPrefix> SystemHeaderPrefixes;

  /// The directory which holds the compiler resource files (builtin includes,
  /// etc.).
  std::string ResourceDir;

  /// \brief The directory used for the module cache.
  std::string ModuleCachePath;

  /// \brief Whether we should disable the use of the hash string within the
  /// module cache.
  ///
  /// Note: Only used for testing!
  unsigned DisableModuleHash : 1;

  /// \brief The interval (in seconds) between pruning operations.
  ///
  /// This operation is expensive, because it requires Clang to walk through
  /// the directory structure of the module cache, stat()'ing and removing
  /// files.
  ///
  /// The default value is large, e.g., the operation runs once a week.
  unsigned ModuleCachePruneInterval;

  /// \brief The time (in seconds) after which an unused module file will be
  /// considered unused and will, therefore, be pruned.
  ///
  /// When the module cache is pruned, any module file that has not been
  /// accessed in this many seconds will be removed. The default value is
  /// large, e.g., a month, to avoid forcing infrequently-used modules to be
  /// regenerated often.
  unsigned ModuleCachePruneAfter;

  /// \brief The set of macro names that should be ignored for the purposes
  /// of computing the module hash.
  llvm::SetVector<std::string> ModulesIgnoreMacros;

  /// Include the compiler builtin includes.
  unsigned UseBuiltinIncludes : 1;

  /// Include the system standard include search directories.
  unsigned UseStandardSystemIncludes : 1;

  /// Include the system standard C++ library include search directories.
  unsigned UseStandardCXXIncludes : 1;

  /// Use libc++ instead of the default libstdc++.
  unsigned UseLibcxx : 1;

  /// Whether header search information should be output as for -v.
  unsigned Verbose : 1;

public:
  HeaderSearchOptions(StringRef _Sysroot = "/")
    : Sysroot(_Sysroot), DisableModuleHash(0),
      ModuleCachePruneInterval(7*24*60*60),
      ModuleCachePruneAfter(31*24*60*60),
      UseBuiltinIncludes(true),
      UseStandardSystemIncludes(true), UseStandardCXXIncludes(true),
      UseLibcxx(false), Verbose(false) {}

  /// AddPath - Add the \p Path path to the specified \p Group list.
  void AddPath(StringRef Path, frontend::IncludeDirGroup Group,
               bool IsFramework, bool IgnoreSysRoot) {
    UserEntries.push_back(Entry(Path, Group, IsFramework, IgnoreSysRoot));
  }

  /// AddSystemHeaderPrefix - Override whether \#include directives naming a
  /// path starting with \p Prefix should be considered as naming a system
  /// header.
  void AddSystemHeaderPrefix(StringRef Prefix, bool IsSystemHeader) {
    SystemHeaderPrefixes.push_back(SystemHeaderPrefix(Prefix, IsSystemHeader));
  }
};

} // end namespace clang

#endif
