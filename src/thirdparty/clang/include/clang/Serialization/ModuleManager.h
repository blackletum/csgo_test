﻿//===--- ModuleManager.cpp - Module Manager ---------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the ModuleManager class, which manages a set of loaded
//  modules for the ASTReader.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_SERIALIZATION_MODULE_MANAGER_H
#define LLVM_CLANG_SERIALIZATION_MODULE_MANAGER_H

#include "clang/Basic/FileManager.h"
#include "clang/Serialization/Module.h"
#include "llvm/ADT/DenseMap.h"

namespace clang { 

class GlobalModuleIndex;
class ModuleMap;

namespace serialization {

/// \brief Manages the set of modules loaded by an AST reader.
class ModuleManager {
  /// \brief The chain of AST files. The first entry is the one named by the
  /// user, the last one is the one that doesn't depend on anything further.
  SmallVector<ModuleFile *, 2> Chain;
  
  /// \brief All loaded modules, indexed by name.
  llvm::DenseMap<const FileEntry *, ModuleFile *> Modules;
  
  /// \brief FileManager that handles translating between filenames and
  /// FileEntry *.
  FileManager &FileMgr;
  
  /// \brief A lookup of in-memory (virtual file) buffers
  llvm::DenseMap<const FileEntry *, llvm::MemoryBuffer *> InMemoryBuffers;

  /// \brief The visitation order.
  SmallVector<ModuleFile *, 4> VisitOrder;
      
  /// \brief The list of module files that both we and the global module index
  /// know about.
  ///
  /// Either the global index or the module manager may have modules that the
  /// other does not know about, because the global index can be out-of-date
  /// (in which case the module manager could have modules it does not) and
  /// this particular translation unit might not have loaded all of the modules
  /// known to the global index.
  SmallVector<ModuleFile *, 4> ModulesInCommonWithGlobalIndex;

  /// \brief The global module index, if one is attached.
  ///
  /// The global module index will actually be owned by the ASTReader; this is
  /// just an non-owning pointer.
  GlobalModuleIndex *GlobalIndex;

  /// \brief State used by the "visit" operation to avoid malloc traffic in
  /// calls to visit().
  struct VisitState {
    explicit VisitState(unsigned N)
      : VisitNumber(N, 0), NextVisitNumber(1), NextState(0)
    {
      Stack.reserve(N);
    }

    ~VisitState() {
      delete NextState;
    }

    /// \brief The stack used when marking the imports of a particular module
    /// as not-to-be-visited.
    SmallVector<ModuleFile *, 4> Stack;

    /// \brief The visit number of each module file, which indicates when
    /// this module file was last visited.
    SmallVector<unsigned, 4> VisitNumber;

    /// \brief The next visit number to use to mark visited module files.
    unsigned NextVisitNumber;

    /// \brief The next visit state.
    VisitState *NextState;
  };

  /// \brief The first visit() state in the chain.
  VisitState *FirstVisitState;

  VisitState *allocateVisitState();
  void returnVisitState(VisitState *State);

public:
  typedef SmallVector<ModuleFile*, 2>::iterator ModuleIterator;
  typedef SmallVector<ModuleFile*, 2>::const_iterator ModuleConstIterator;
  typedef SmallVector<ModuleFile*, 2>::reverse_iterator ModuleReverseIterator;
  typedef std::pair<uint32_t, StringRef> ModuleOffset;
  
  explicit ModuleManager(FileManager &FileMgr);
  ~ModuleManager();
  
  /// \brief Forward iterator to traverse all loaded modules.  This is reverse
  /// source-order.
  ModuleIterator begin() { return Chain.begin(); }
  /// \brief Forward iterator end-point to traverse all loaded modules
  ModuleIterator end() { return Chain.end(); }
  
  /// \brief Const forward iterator to traverse all loaded modules.  This is 
  /// in reverse source-order.
  ModuleConstIterator begin() const { return Chain.begin(); }
  /// \brief Const forward iterator end-point to traverse all loaded modules
  ModuleConstIterator end() const { return Chain.end(); }
  
  /// \brief Reverse iterator to traverse all loaded modules.  This is in 
  /// source order.
  ModuleReverseIterator rbegin() { return Chain.rbegin(); }
  /// \brief Reverse iterator end-point to traverse all loaded modules.
  ModuleReverseIterator rend() { return Chain.rend(); }
  
  /// \brief Returns the primary module associated with the manager, that is,
  /// the first module loaded
  ModuleFile &getPrimaryModule() { return *Chain[0]; }
  
  /// \brief Returns the primary module associated with the manager, that is,
  /// the first module loaded.
  ModuleFile &getPrimaryModule() const { return *Chain[0]; }
  
  /// \brief Returns the module associated with the given index
  ModuleFile &operator[](unsigned Index) const { return *Chain[Index]; }
  
  /// \brief Returns the module associated with the given name
  ModuleFile *lookup(StringRef Name);

  /// \brief Returns the module associated with the given module file.
  ModuleFile *lookup(const FileEntry *File);

  /// \brief Returns the in-memory (virtual file) buffer with the given name
  llvm::MemoryBuffer *lookupBuffer(StringRef Name);
  
  /// \brief Number of modules loaded
  unsigned size() const { return Chain.size(); }

  /// \brief The result of attempting to add a new module.
  enum AddModuleResult {
    /// \brief The module file had already been loaded.
    AlreadyLoaded,
    /// \brief The module file was just loaded in response to this call.
    NewlyLoaded,
    /// \brief The module file is missing.
    Missing,
    /// \brief The module file is out-of-date.
    OutOfDate
  };

  /// \brief Attempts to create a new module and add it to the list of known
  /// modules.
  ///
  /// \param FileName The file name of the module to be loaded.
  ///
  /// \param Type The kind of module being loaded.
  ///
  /// \param ImportLoc The location at which the module is imported.
  ///
  /// \param ImportedBy The module that is importing this module, or NULL if
  /// this module is imported directly by the user.
  ///
  /// \param Generation The generation in which this module was loaded.
  ///
  /// \param ExpectedSize The expected size of the module file, used for
  /// validation. This will be zero if unknown.
  ///
  /// \param ExpectedModTime The expected modification time of the module
  /// file, used for validation. This will be zero if unknown.
  ///
  /// \param Module A pointer to the module file if the module was successfully
  /// loaded.
  ///
  /// \param ErrorStr Will be set to a non-empty string if any errors occurred
  /// while trying to load the module.
  ///
  /// \return A pointer to the module that corresponds to this file name,
  /// and a value indicating whether the module was loaded.
  AddModuleResult addModule(StringRef FileName, ModuleKind Type,
                            SourceLocation ImportLoc,
                            ModuleFile *ImportedBy, unsigned Generation,
                            off_t ExpectedSize, time_t ExpectedModTime,
                            ModuleFile *&Module,
                            std::string &ErrorStr);

  /// \brief Remove the given set of modules.
  void removeModules(ModuleIterator first, ModuleIterator last,
                     ModuleMap *modMap);

  /// \brief Add an in-memory buffer the list of known buffers
  void addInMemoryBuffer(StringRef FileName, llvm::MemoryBuffer *Buffer);

  /// \brief Set the global module index.
  void setGlobalIndex(GlobalModuleIndex *Index);

  /// \brief Notification from the AST reader that the given module file
  /// has been "accepted", and will not (can not) be unloaded.
  void moduleFileAccepted(ModuleFile *MF);

  /// \brief Visit each of the modules.
  ///
  /// This routine visits each of the modules, starting with the
  /// "root" modules that no other loaded modules depend on, and
  /// proceeding to the leaf modules, visiting each module only once
  /// during the traversal.
  ///
  /// This traversal is intended to support various "lookup"
  /// operations that can find data in any of the loaded modules.
  ///
  /// \param Visitor A visitor function that will be invoked with each
  /// module and the given user data pointer. The return value must be
  /// convertible to bool; when false, the visitation continues to
  /// modules that the current module depends on. When true, the
  /// visitation skips any modules that the current module depends on.
  ///
  /// \param UserData User data associated with the visitor object, which
  /// will be passed along to the visitor.
  ///
  /// \param ModuleFilesHit If non-NULL, contains the set of module files
  /// that we know we need to visit because the global module index told us to.
  /// Any module that is known to both the global module index and the module
  /// manager that is *not* in this set can be skipped.
  void visit(bool (*Visitor)(ModuleFile &M, void *UserData), void *UserData,
             llvm::SmallPtrSet<ModuleFile *, 4> *ModuleFilesHit = 0);
  
  /// \brief Visit each of the modules with a depth-first traversal.
  ///
  /// This routine visits each of the modules known to the module
  /// manager using a depth-first search, starting with the first
  /// loaded module. The traversal invokes the callback both before
  /// traversing the children (preorder traversal) and after
  /// traversing the children (postorder traversal).
  ///
  /// \param Visitor A visitor function that will be invoked with each
  /// module and given a \c Preorder flag that indicates whether we're
  /// visiting the module before or after visiting its children.  The
  /// visitor may return true at any time to abort the depth-first
  /// visitation.
  ///
  /// \param UserData User data ssociated with the visitor object,
  /// which will be passed along to the user.
  void visitDepthFirst(bool (*Visitor)(ModuleFile &M, bool Preorder, 
                                       void *UserData), 
                       void *UserData);

  /// \brief Attempt to resolve the given module file name to a file entry.
  ///
  /// \param FileName The name of the module file.
  ///
  /// \param ExpectedSize The size that the module file is expected to have.
  /// If the actual size differs, the resolver should return \c true.
  ///
  /// \param ExpectedModTime The modification time that the module file is
  /// expected to have. If the actual modification time differs, the resolver
  /// should return \c true.
  ///
  /// \param File Will be set to the file if there is one, or null
  /// otherwise.
  ///
  /// \returns True if a file exists but does not meet the size/
  /// modification time criteria, false if the file is either available and
  /// suitable, or is missing.
  bool lookupModuleFile(StringRef FileName,
                        off_t ExpectedSize,
                        time_t ExpectedModTime,
                        const FileEntry *&File);

  /// \brief View the graphviz representation of the module graph.
  void viewGraph();
};

} } // end namespace clang::serialization

#endif
