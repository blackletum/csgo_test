﻿//===--- ASTContext.h - Context to hold long-lived AST nodes ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief Defines the clang::ASTContext interface.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_ASTCONTEXT_H
#define LLVM_CLANG_AST_ASTCONTEXT_H

#include "clang/AST/ASTTypeTraits.h"
#include "clang/AST/CanonicalType.h"
#include "clang/AST/CommentCommandTraits.h"
#include "clang/AST/Decl.h"
#include "clang/AST/LambdaMangleContext.h"
#include "clang/AST/NestedNameSpecifier.h"
#include "clang/AST/PrettyPrinter.h"
#include "clang/AST/RawCommentList.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/TemplateName.h"
#include "clang/AST/Type.h"
#include "clang/Basic/AddressSpaces.h"
#include "clang/Basic/IdentifierTable.h"
#include "clang/Basic/LangOptions.h"
#include "clang/Basic/OperatorKinds.h"
#include "clang/Basic/PartialDiagnostic.h"
#include "clang/Basic/VersionTuple.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/ADT/OwningPtr.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/TinyPtrVector.h"
#include "llvm/Support/Allocator.h"
#include <vector>

namespace llvm {
  struct fltSemantics;
}

namespace clang {
  class FileManager;
  class ASTRecordLayout;
  class BlockExpr;
  class CharUnits;
  class DiagnosticsEngine;
  class Expr;
  class ExternalASTSource;
  class ASTMutationListener;
  class IdentifierTable;
  class SelectorTable;
  class TargetInfo;
  class CXXABI;
  // Decls
  class MangleContext;
  class ObjCIvarDecl;
  class ObjCPropertyDecl;
  class UnresolvedSetIterator;
  class UsingDecl;
  class UsingShadowDecl;

  namespace Builtin { class Context; }

  namespace comments {
    class FullComment;
  }

/// \brief Holds long-lived AST nodes (such as types and decls) that can be
/// referred to throughout the semantic analysis of a file.
class ASTContext : public RefCountedBase<ASTContext> {
  ASTContext &this_() { return *this; }

  mutable SmallVector<Type *, 0> Types;
  mutable llvm::FoldingSet<ExtQuals> ExtQualNodes;
  mutable llvm::FoldingSet<ComplexType> ComplexTypes;
  mutable llvm::FoldingSet<PointerType> PointerTypes;
  mutable llvm::FoldingSet<BlockPointerType> BlockPointerTypes;
  mutable llvm::FoldingSet<LValueReferenceType> LValueReferenceTypes;
  mutable llvm::FoldingSet<RValueReferenceType> RValueReferenceTypes;
  mutable llvm::FoldingSet<MemberPointerType> MemberPointerTypes;
  mutable llvm::FoldingSet<ConstantArrayType> ConstantArrayTypes;
  mutable llvm::FoldingSet<IncompleteArrayType> IncompleteArrayTypes;
  mutable std::vector<VariableArrayType*> VariableArrayTypes;
  mutable llvm::FoldingSet<DependentSizedArrayType> DependentSizedArrayTypes;
  mutable llvm::FoldingSet<DependentSizedExtVectorType>
    DependentSizedExtVectorTypes;
  mutable llvm::FoldingSet<VectorType> VectorTypes;
  mutable llvm::FoldingSet<FunctionNoProtoType> FunctionNoProtoTypes;
  mutable llvm::ContextualFoldingSet<FunctionProtoType, ASTContext&>
    FunctionProtoTypes;
  mutable llvm::FoldingSet<DependentTypeOfExprType> DependentTypeOfExprTypes;
  mutable llvm::FoldingSet<DependentDecltypeType> DependentDecltypeTypes;
  mutable llvm::FoldingSet<TemplateTypeParmType> TemplateTypeParmTypes;
  mutable llvm::FoldingSet<SubstTemplateTypeParmType>
    SubstTemplateTypeParmTypes;
  mutable llvm::FoldingSet<SubstTemplateTypeParmPackType>
    SubstTemplateTypeParmPackTypes;
  mutable llvm::ContextualFoldingSet<TemplateSpecializationType, ASTContext&>
    TemplateSpecializationTypes;
  mutable llvm::FoldingSet<ParenType> ParenTypes;
  mutable llvm::FoldingSet<ElaboratedType> ElaboratedTypes;
  mutable llvm::FoldingSet<DependentNameType> DependentNameTypes;
  mutable llvm::ContextualFoldingSet<DependentTemplateSpecializationType,
                                     ASTContext&>
    DependentTemplateSpecializationTypes;
  llvm::FoldingSet<PackExpansionType> PackExpansionTypes;
  mutable llvm::FoldingSet<ObjCObjectTypeImpl> ObjCObjectTypes;
  mutable llvm::FoldingSet<ObjCObjectPointerType> ObjCObjectPointerTypes;
  mutable llvm::FoldingSet<AutoType> AutoTypes;
  mutable llvm::FoldingSet<AtomicType> AtomicTypes;
  llvm::FoldingSet<AttributedType> AttributedTypes;

  mutable llvm::FoldingSet<QualifiedTemplateName> QualifiedTemplateNames;
  mutable llvm::FoldingSet<DependentTemplateName> DependentTemplateNames;
  mutable llvm::FoldingSet<SubstTemplateTemplateParmStorage> 
    SubstTemplateTemplateParms;
  mutable llvm::ContextualFoldingSet<SubstTemplateTemplateParmPackStorage,
                                     ASTContext&> 
    SubstTemplateTemplateParmPacks;
  
  /// \brief The set of nested name specifiers.
  ///
  /// This set is managed by the NestedNameSpecifier class.
  mutable llvm::FoldingSet<NestedNameSpecifier> NestedNameSpecifiers;
  mutable NestedNameSpecifier *GlobalNestedNameSpecifier;
  friend class NestedNameSpecifier;

  /// \brief A cache mapping from RecordDecls to ASTRecordLayouts.
  ///
  /// This is lazily created.  This is intentionally not serialized.
  mutable llvm::DenseMap<const RecordDecl*, const ASTRecordLayout*>
    ASTRecordLayouts;
  mutable llvm::DenseMap<const ObjCContainerDecl*, const ASTRecordLayout*>
    ObjCLayouts;

  /// \brief A cache from types to size and alignment information.
  typedef llvm::DenseMap<const Type*,
                         std::pair<uint64_t, unsigned> > TypeInfoMap;
  mutable TypeInfoMap MemoizedTypeInfo;

  /// \brief A cache mapping from CXXRecordDecls to key functions.
  llvm::DenseMap<const CXXRecordDecl*, const CXXMethodDecl*> KeyFunctions;
  
  /// \brief Mapping from ObjCContainers to their ObjCImplementations.
  llvm::DenseMap<ObjCContainerDecl*, ObjCImplDecl*> ObjCImpls;
  
  /// \brief Mapping from ObjCMethod to its duplicate declaration in the same
  /// interface.
  llvm::DenseMap<const ObjCMethodDecl*,const ObjCMethodDecl*> ObjCMethodRedecls;

  /// \brief Mapping from __block VarDecls to their copy initialization expr.
  llvm::DenseMap<const VarDecl*, Expr*> BlockVarCopyInits;
    
  /// \brief Mapping from class scope functions specialization to their
  /// template patterns.
  llvm::DenseMap<const FunctionDecl*, FunctionDecl*>
    ClassScopeSpecializationPattern;

  /// \brief Representation of a "canonical" template template parameter that
  /// is used in canonical template names.
  class CanonicalTemplateTemplateParm : public llvm::FoldingSetNode {
    TemplateTemplateParmDecl *Parm;
    
  public:
    CanonicalTemplateTemplateParm(TemplateTemplateParmDecl *Parm) 
      : Parm(Parm) { }
    
    TemplateTemplateParmDecl *getParam() const { return Parm; }
    
    void Profile(llvm::FoldingSetNodeID &ID) { Profile(ID, Parm); }
    
    static void Profile(llvm::FoldingSetNodeID &ID, 
                        TemplateTemplateParmDecl *Parm);
  };
  mutable llvm::FoldingSet<CanonicalTemplateTemplateParm>
    CanonTemplateTemplateParms;
  
  TemplateTemplateParmDecl *
    getCanonicalTemplateTemplateParmDecl(TemplateTemplateParmDecl *TTP) const;

  /// \brief The typedef for the __int128_t type.
  mutable TypedefDecl *Int128Decl;

  /// \brief The typedef for the __uint128_t type.
  mutable TypedefDecl *UInt128Decl;
  
  /// \brief The typedef for the target specific predefined
  /// __builtin_va_list type.
  mutable TypedefDecl *BuiltinVaListDecl;

  /// \brief The typedef for the predefined \c id type.
  mutable TypedefDecl *ObjCIdDecl;
  
  /// \brief The typedef for the predefined \c SEL type.
  mutable TypedefDecl *ObjCSelDecl;

  /// \brief The typedef for the predefined \c Class type.
  mutable TypedefDecl *ObjCClassDecl;

  /// \brief The typedef for the predefined \c Protocol class in Objective-C.
  mutable ObjCInterfaceDecl *ObjCProtocolClassDecl;
  
  /// \brief The typedef for the predefined 'BOOL' type.
  mutable TypedefDecl *BOOLDecl;

  // Typedefs which may be provided defining the structure of Objective-C
  // pseudo-builtins
  QualType ObjCIdRedefinitionType;
  QualType ObjCClassRedefinitionType;
  QualType ObjCSelRedefinitionType;

  QualType ObjCConstantStringType;
  mutable RecordDecl *CFConstantStringTypeDecl;
  
  mutable QualType ObjCSuperType;
  
  QualType ObjCNSStringType;

  /// \brief The typedef declaration for the Objective-C "instancetype" type.
  TypedefDecl *ObjCInstanceTypeDecl;
  
  /// \brief The type for the C FILE type.
  TypeDecl *FILEDecl;

  /// \brief The type for the C jmp_buf type.
  TypeDecl *jmp_bufDecl;

  /// \brief The type for the C sigjmp_buf type.
  TypeDecl *sigjmp_bufDecl;

  /// \brief The type for the C ucontext_t type.
  TypeDecl *ucontext_tDecl;

  /// \brief Type for the Block descriptor for Blocks CodeGen.
  ///
  /// Since this is only used for generation of debug info, it is not
  /// serialized.
  mutable RecordDecl *BlockDescriptorType;

  /// \brief Type for the Block descriptor for Blocks CodeGen.
  ///
  /// Since this is only used for generation of debug info, it is not
  /// serialized.
  mutable RecordDecl *BlockDescriptorExtendedType;

  /// \brief Declaration for the CUDA cudaConfigureCall function.
  FunctionDecl *cudaConfigureCallDecl;

  TypeSourceInfo NullTypeSourceInfo;

  /// \brief Keeps track of all declaration attributes.
  ///
  /// Since so few decls have attrs, we keep them in a hash map instead of
  /// wasting space in the Decl class.
  llvm::DenseMap<const Decl*, AttrVec*> DeclAttrs;

  /// \brief Keeps track of the static data member templates from which
  /// static data members of class template specializations were instantiated.
  ///
  /// This data structure stores the mapping from instantiations of static
  /// data members to the static data member representations within the
  /// class template from which they were instantiated along with the kind
  /// of instantiation or specialization (a TemplateSpecializationKind - 1).
  ///
  /// Given the following example:
  ///
  /// \code
  /// template<typename T>
  /// struct X {
  ///   static T value;
  /// };
  ///
  /// template<typename T>
  ///   T X<T>::value = T(17);
  ///
  /// int *x = &X<int>::value;
  /// \endcode
  ///
  /// This mapping will contain an entry that maps from the VarDecl for
  /// X<int>::value to the corresponding VarDecl for X<T>::value (within the
  /// class template X) and will be marked TSK_ImplicitInstantiation.
  llvm::DenseMap<const VarDecl *, MemberSpecializationInfo *> 
    InstantiatedFromStaticDataMember;

  /// \brief Keeps track of the declaration from which a UsingDecl was
  /// created during instantiation.
  ///
  /// The source declaration is always a UsingDecl, an UnresolvedUsingValueDecl,
  /// or an UnresolvedUsingTypenameDecl.
  ///
  /// For example:
  /// \code
  /// template<typename T>
  /// struct A {
  ///   void f();
  /// };
  ///
  /// template<typename T>
  /// struct B : A<T> {
  ///   using A<T>::f;
  /// };
  ///
  /// template struct B<int>;
  /// \endcode
  ///
  /// This mapping will contain an entry that maps from the UsingDecl in
  /// B<int> to the UnresolvedUsingDecl in B<T>.
  llvm::DenseMap<UsingDecl *, NamedDecl *> InstantiatedFromUsingDecl;

  llvm::DenseMap<UsingShadowDecl*, UsingShadowDecl*>
    InstantiatedFromUsingShadowDecl;

  llvm::DenseMap<FieldDecl *, FieldDecl *> InstantiatedFromUnnamedFieldDecl;

  /// \brief Mapping that stores the methods overridden by a given C++
  /// member function.
  ///
  /// Since most C++ member functions aren't virtual and therefore
  /// don't override anything, we store the overridden functions in
  /// this map on the side rather than within the CXXMethodDecl structure.
  typedef llvm::TinyPtrVector<const CXXMethodDecl*> CXXMethodVector;
  llvm::DenseMap<const CXXMethodDecl *, CXXMethodVector> OverriddenMethods;

  /// \brief Mapping from each declaration context to its corresponding lambda 
  /// mangling context.
  llvm::DenseMap<const DeclContext *, LambdaMangleContext> LambdaMangleContexts;

  llvm::DenseMap<const DeclContext *, unsigned> UnnamedMangleContexts;
  llvm::DenseMap<const TagDecl *, unsigned> UnnamedMangleNumbers;

  /// \brief Mapping that stores parameterIndex values for ParmVarDecls when
  /// that value exceeds the bitfield size of ParmVarDeclBits.ParameterIndex.
  typedef llvm::DenseMap<const VarDecl *, unsigned> ParameterIndexTable;
  ParameterIndexTable ParamIndices;  
  
  ImportDecl *FirstLocalImport;
  ImportDecl *LastLocalImport;
  
  TranslationUnitDecl *TUDecl;

  /// \brief The associated SourceManager object.a
  SourceManager &SourceMgr;

  /// \brief The language options used to create the AST associated with
  ///  this ASTContext object.
  LangOptions &LangOpts;

  /// \brief The allocator used to create AST objects.
  ///
  /// AST objects are never destructed; rather, all memory associated with the
  /// AST objects will be released when the ASTContext itself is destroyed.
  mutable llvm::BumpPtrAllocator BumpAlloc;

  /// \brief Allocator for partial diagnostics.
  PartialDiagnostic::StorageAllocator DiagAllocator;

  /// \brief The current C++ ABI.
  OwningPtr<CXXABI> ABI;
  CXXABI *createCXXABI(const TargetInfo &T);

  /// \brief The logical -> physical address space map.
  const LangAS::Map *AddrSpaceMap;

  friend class ASTDeclReader;
  friend class ASTReader;
  friend class ASTWriter;
  friend class CXXRecordDecl;

  const TargetInfo *Target;
  clang::PrintingPolicy PrintingPolicy;
  
public:
  IdentifierTable &Idents;
  SelectorTable &Selectors;
  Builtin::Context &BuiltinInfo;
  mutable DeclarationNameTable DeclarationNames;
  OwningPtr<ExternalASTSource> ExternalSource;
  ASTMutationListener *Listener;

  /// \brief Contains parents of a node.
  typedef llvm::SmallVector<ast_type_traits::DynTypedNode, 1> ParentVector;

  /// \brief Maps from a node to its parents.
  typedef llvm::DenseMap<const void *, ParentVector> ParentMap;

  /// \brief Returns the parents of the given node.
  ///
  /// Note that this will lazily compute the parents of all nodes
  /// and store them for later retrieval. Thus, the first call is O(n)
  /// in the number of AST nodes.
  ///
  /// Caveats and FIXMEs:
  /// Calculating the parent map over all AST nodes will need to load the
  /// full AST. This can be undesirable in the case where the full AST is
  /// expensive to create (for example, when using precompiled header
  /// preambles). Thus, there are good opportunities for optimization here.
  /// One idea is to walk the given node downwards, looking for references
  /// to declaration contexts - once a declaration context is found, compute
  /// the parent map for the declaration context; if that can satisfy the
  /// request, loading the whole AST can be avoided. Note that this is made
  /// more complex by statements in templates having multiple parents - those
  /// problems can be solved by building closure over the templated parts of
  /// the AST, which also avoids touching large parts of the AST.
  /// Additionally, we will want to add an interface to already give a hint
  /// where to search for the parents, for example when looking at a statement
  /// inside a certain function.
  ///
  /// 'NodeT' can be one of Decl, Stmt, Type, TypeLoc,
  /// NestedNameSpecifier or NestedNameSpecifierLoc.
  template <typename NodeT>
  ParentVector getParents(const NodeT &Node) {
    return getParents(ast_type_traits::DynTypedNode::create(Node));
  }

  ParentVector getParents(const ast_type_traits::DynTypedNode &Node) {
    assert(Node.getMemoizationData() &&
           "Invariant broken: only nodes that support memoization may be "
           "used in the parent map.");
    if (!AllParents) {
      // We always need to run over the whole translation unit, as
      // hasAncestor can escape any subtree.
      AllParents.reset(
          ParentMapASTVisitor::buildMap(*getTranslationUnitDecl()));
    }
    ParentMap::const_iterator I = AllParents->find(Node.getMemoizationData());
    if (I == AllParents->end()) {
      return ParentVector();
    }
    return I->second;
  }

  const clang::PrintingPolicy &getPrintingPolicy() const {
    return PrintingPolicy;
  }

  void setPrintingPolicy(const clang::PrintingPolicy &Policy) {
    PrintingPolicy = Policy;
  }
  
  SourceManager& getSourceManager() { return SourceMgr; }
  const SourceManager& getSourceManager() const { return SourceMgr; }

  llvm::BumpPtrAllocator &getAllocator() const {
    return BumpAlloc;
  }

  void *Allocate(unsigned Size, unsigned Align = 8) const {
    return BumpAlloc.Allocate(Size, Align);
  }
  void Deallocate(void *Ptr) const { }
  
  /// Return the total amount of physical memory allocated for representing
  /// AST nodes and type information.
  size_t getASTAllocatedMemory() const {
    return BumpAlloc.getTotalMemory();
  }
  /// Return the total memory used for various side tables.
  size_t getSideTableAllocatedMemory() const;
  
  PartialDiagnostic::StorageAllocator &getDiagAllocator() {
    return DiagAllocator;
  }

  const TargetInfo &getTargetInfo() const { return *Target; }
  
  const LangOptions& getLangOpts() const { return LangOpts; }

  DiagnosticsEngine &getDiagnostics() const;

  FullSourceLoc getFullLoc(SourceLocation Loc) const {
    return FullSourceLoc(Loc,SourceMgr);
  }

  /// \brief All comments in this translation unit.
  RawCommentList Comments;

  /// \brief True if comments are already loaded from ExternalASTSource.
  mutable bool CommentsLoaded;

  class RawCommentAndCacheFlags {
  public:
    enum Kind {
      /// We searched for a comment attached to the particular declaration, but
      /// didn't find any.
      ///
      /// getRaw() == 0.
      NoCommentInDecl = 0,

      /// We have found a comment attached to this particular declaration.
      ///
      /// getRaw() != 0.
      FromDecl,

      /// This declaration does not have an attached comment, and we have
      /// searched the redeclaration chain.
      ///
      /// If getRaw() == 0, the whole redeclaration chain does not have any
      /// comments.
      ///
      /// If getRaw() != 0, it is a comment propagated from other
      /// redeclaration.
      FromRedecl
    };

    Kind getKind() const LLVM_READONLY {
      return Data.getInt();
    }

    void setKind(Kind K) {
      Data.setInt(K);
    }

    const RawComment *getRaw() const LLVM_READONLY {
      return Data.getPointer();
    }

    void setRaw(const RawComment *RC) {
      Data.setPointer(RC);
    }

    const Decl *getOriginalDecl() const LLVM_READONLY {
      return OriginalDecl;
    }

    void setOriginalDecl(const Decl *Orig) {
      OriginalDecl = Orig;
    }

  private:
    llvm::PointerIntPair<const RawComment *, 2, Kind> Data;
    const Decl *OriginalDecl;
  };

  /// \brief Mapping from declarations to comments attached to any
  /// redeclaration.
  ///
  /// Raw comments are owned by Comments list.  This mapping is populated
  /// lazily.
  mutable llvm::DenseMap<const Decl *, RawCommentAndCacheFlags> RedeclComments;

  /// \brief Mapping from declarations to parsed comments attached to any
  /// redeclaration.
  mutable llvm::DenseMap<const Decl *, comments::FullComment *> ParsedComments;

  /// \brief Return the documentation comment attached to a given declaration,
  /// without looking into cache.
  RawComment *getRawCommentForDeclNoCache(const Decl *D) const;

public:
  RawCommentList &getRawCommentList() {
    return Comments;
  }

  void addComment(const RawComment &RC) {
    assert(LangOpts.RetainCommentsFromSystemHeaders ||
           !SourceMgr.isInSystemHeader(RC.getSourceRange().getBegin()));
    Comments.addComment(RC, BumpAlloc);
  }

  /// \brief Return the documentation comment attached to a given declaration.
  /// Returns NULL if no comment is attached.
  ///
  /// \param OriginalDecl if not NULL, is set to declaration AST node that had
  /// the comment, if the comment we found comes from a redeclaration.
  const RawComment *getRawCommentForAnyRedecl(
                                      const Decl *D,
                                      const Decl **OriginalDecl = NULL) const;

  /// Return parsed documentation comment attached to a given declaration.
  /// Returns NULL if no comment is attached.
  ///
  /// \param PP the Preprocessor used with this TU.  Could be NULL if
  /// preprocessor is not available.
  comments::FullComment *getCommentForDecl(const Decl *D,
                                           const Preprocessor *PP) const;
  
  comments::FullComment *cloneFullComment(comments::FullComment *FC,
                                         const Decl *D) const;

private:
  mutable comments::CommandTraits CommentCommandTraits;

public:
  comments::CommandTraits &getCommentCommandTraits() const {
    return CommentCommandTraits;
  }

  /// \brief Retrieve the attributes for the given declaration.
  AttrVec& getDeclAttrs(const Decl *D);

  /// \brief Erase the attributes corresponding to the given declaration.
  void eraseDeclAttrs(const Decl *D);

  /// \brief If this variable is an instantiated static data member of a
  /// class template specialization, returns the templated static data member
  /// from which it was instantiated.
  MemberSpecializationInfo *getInstantiatedFromStaticDataMember(
                                                           const VarDecl *Var);

  FunctionDecl *getClassScopeSpecializationPattern(const FunctionDecl *FD);

  void setClassScopeSpecializationPattern(FunctionDecl *FD,
                                          FunctionDecl *Pattern);

  /// \brief Note that the static data member \p Inst is an instantiation of
  /// the static data member template \p Tmpl of a class template.
  void setInstantiatedFromStaticDataMember(VarDecl *Inst, VarDecl *Tmpl,
                                           TemplateSpecializationKind TSK,
                        SourceLocation PointOfInstantiation = SourceLocation());

  /// \brief If the given using decl \p Inst is an instantiation of a
  /// (possibly unresolved) using decl from a template instantiation,
  /// return it.
  NamedDecl *getInstantiatedFromUsingDecl(UsingDecl *Inst);

  /// \brief Remember that the using decl \p Inst is an instantiation
  /// of the using decl \p Pattern of a class template.
  void setInstantiatedFromUsingDecl(UsingDecl *Inst, NamedDecl *Pattern);

  void setInstantiatedFromUsingShadowDecl(UsingShadowDecl *Inst,
                                          UsingShadowDecl *Pattern);
  UsingShadowDecl *getInstantiatedFromUsingShadowDecl(UsingShadowDecl *Inst);

  FieldDecl *getInstantiatedFromUnnamedFieldDecl(FieldDecl *Field);

  void setInstantiatedFromUnnamedFieldDecl(FieldDecl *Inst, FieldDecl *Tmpl);
  
  /// \brief Return \c true if \p FD is a zero-length bitfield which follows
  /// the non-bitfield \p LastFD.
  bool ZeroBitfieldFollowsNonBitfield(const FieldDecl *FD, 
                                      const FieldDecl *LastFD) const;

  /// \brief Return \c true if \p FD is a zero-length bitfield which follows
  /// the bitfield \p LastFD.
  bool ZeroBitfieldFollowsBitfield(const FieldDecl *FD,
                                   const FieldDecl *LastFD) const;
  
  /// \brief Return \c true if \p FD is a bitfield which follows the bitfield
  /// \p LastFD.
  bool BitfieldFollowsBitfield(const FieldDecl *FD,
                               const FieldDecl *LastFD) const;
  
  /// \brief Return \c true if \p FD is not a bitfield which follows the
  /// bitfield \p LastFD.
  bool NonBitfieldFollowsBitfield(const FieldDecl *FD,
                                  const FieldDecl *LastFD) const;
  
  /// \brief Return \c true if \p FD is a bitfield which follows the
  /// non-bitfield \p LastFD.
  bool BitfieldFollowsNonBitfield(const FieldDecl *FD,
                                  const FieldDecl *LastFD) const;

  // Access to the set of methods overridden by the given C++ method.
  typedef CXXMethodVector::const_iterator overridden_cxx_method_iterator;
  overridden_cxx_method_iterator
  overridden_methods_begin(const CXXMethodDecl *Method) const;

  overridden_cxx_method_iterator
  overridden_methods_end(const CXXMethodDecl *Method) const;

  unsigned overridden_methods_size(const CXXMethodDecl *Method) const;

  /// \brief Note that the given C++ \p Method overrides the given \p
  /// Overridden method.
  void addOverriddenMethod(const CXXMethodDecl *Method, 
                           const CXXMethodDecl *Overridden);

  /// \brief Return C++ or ObjC overridden methods for the given \p Method.
  ///
  /// An ObjC method is considered to override any method in the class's
  /// base classes, its protocols, or its categories' protocols, that has
  /// the same selector and is of the same kind (class or instance).
  /// A method in an implementation is not considered as overriding the same
  /// method in the interface or its categories.
  void getOverriddenMethods(
                        const NamedDecl *Method,
                        SmallVectorImpl<const NamedDecl *> &Overridden) const;
  
  /// \brief Notify the AST context that a new import declaration has been
  /// parsed or implicitly created within this translation unit.
  void addedLocalImportDecl(ImportDecl *Import);

  static ImportDecl *getNextLocalImport(ImportDecl *Import) {
    return Import->NextLocalImport;
  }
  
  /// \brief Iterator that visits import declarations.
  class import_iterator {
    ImportDecl *Import;
    
  public:
    typedef ImportDecl               *value_type;
    typedef ImportDecl               *reference;
    typedef ImportDecl               *pointer;
    typedef int                       difference_type;
    typedef std::forward_iterator_tag iterator_category;
    
    import_iterator() : Import() { }
    explicit import_iterator(ImportDecl *Import) : Import(Import) { }
    
    reference operator*() const { return Import; }
    pointer operator->() const { return Import; }
    
    import_iterator &operator++() {
      Import = ASTContext::getNextLocalImport(Import);
      return *this;
    }

    import_iterator operator++(int) {
      import_iterator Other(*this);
      ++(*this);
      return Other;
    }
    
    friend bool operator==(import_iterator X, import_iterator Y) {
      return X.Import == Y.Import;
    }

    friend bool operator!=(import_iterator X, import_iterator Y) {
      return X.Import != Y.Import;
    }
  };
  
  import_iterator local_import_begin() const { 
    return import_iterator(FirstLocalImport); 
  }
  import_iterator local_import_end() const { return import_iterator(); }
  
  TranslationUnitDecl *getTranslationUnitDecl() const { return TUDecl; }


  // Builtin Types.
  CanQualType VoidTy;
  CanQualType BoolTy;
  CanQualType CharTy;
  CanQualType WCharTy;  // [C++ 3.9.1p5], integer type in C99.
  CanQualType WIntTy;   // [C99 7.24.1], integer type unchanged by default promotions.
  CanQualType Char16Ty; // [C++0x 3.9.1p5], integer type in C99.
  CanQualType Char32Ty; // [C++0x 3.9.1p5], integer type in C99.
  CanQualType SignedCharTy, ShortTy, IntTy, LongTy, LongLongTy, Int128Ty;
  CanQualType UnsignedCharTy, UnsignedShortTy, UnsignedIntTy, UnsignedLongTy;
  CanQualType UnsignedLongLongTy, UnsignedInt128Ty;
  CanQualType FloatTy, DoubleTy, LongDoubleTy;
  CanQualType HalfTy; // [OpenCL 6.1.1.1], ARM NEON
  CanQualType FloatComplexTy, DoubleComplexTy, LongDoubleComplexTy;
  CanQualType VoidPtrTy, NullPtrTy;
  CanQualType DependentTy, OverloadTy, BoundMemberTy, UnknownAnyTy;
  CanQualType BuiltinFnTy;
  CanQualType PseudoObjectTy, ARCUnbridgedCastTy;
  CanQualType ObjCBuiltinIdTy, ObjCBuiltinClassTy, ObjCBuiltinSelTy;
  CanQualType ObjCBuiltinBoolTy;
  CanQualType OCLImage1dTy, OCLImage1dArrayTy, OCLImage1dBufferTy;
  CanQualType OCLImage2dTy, OCLImage2dArrayTy;
  CanQualType OCLImage3dTy;
  CanQualType OCLSamplerTy, OCLEventTy;

  // Types for deductions in C++0x [stmt.ranged]'s desugaring. Built on demand.
  mutable QualType AutoDeductTy;     // Deduction against 'auto'.
  mutable QualType AutoRRefDeductTy; // Deduction against 'auto &&'.

  // Type used to help define __builtin_va_list for some targets.
  // The type is built when constructing 'BuiltinVaListDecl'.
  mutable QualType VaListTagTy;

  ASTContext(LangOptions& LOpts, SourceManager &SM, const TargetInfo *t,
             IdentifierTable &idents, SelectorTable &sels,
             Builtin::Context &builtins,
             unsigned size_reserve,
             bool DelayInitialization = false);

  ~ASTContext();

  /// \brief Attach an external AST source to the AST context.
  ///
  /// The external AST source provides the ability to load parts of
  /// the abstract syntax tree as needed from some external storage,
  /// e.g., a precompiled header.
  void setExternalSource(OwningPtr<ExternalASTSource> &Source);

  /// \brief Retrieve a pointer to the external AST source associated
  /// with this AST context, if any.
  ExternalASTSource *getExternalSource() const { return ExternalSource.get(); }

  /// \brief Attach an AST mutation listener to the AST context.
  ///
  /// The AST mutation listener provides the ability to track modifications to
  /// the abstract syntax tree entities committed after they were initially
  /// created.
  void setASTMutationListener(ASTMutationListener *Listener) {
    this->Listener = Listener;
  }

  /// \brief Retrieve a pointer to the AST mutation listener associated
  /// with this AST context, if any.
  ASTMutationListener *getASTMutationListener() const { return Listener; }

  void PrintStats() const;
  const SmallVectorImpl<Type *>& getTypes() const { return Types; }

  /// \brief Retrieve the declaration for the 128-bit signed integer type.
  TypedefDecl *getInt128Decl() const;

  /// \brief Retrieve the declaration for the 128-bit unsigned integer type.
  TypedefDecl *getUInt128Decl() const;
  
  //===--------------------------------------------------------------------===//
  //                           Type Constructors
  //===--------------------------------------------------------------------===//

private:
  /// \brief Return a type with extended qualifiers.
  QualType getExtQualType(const Type *Base, Qualifiers Quals) const;

  QualType getTypeDeclTypeSlow(const TypeDecl *Decl) const;

public:
  /// \brief Return the uniqued reference to the type for an address space
  /// qualified type with the specified type and address space.
  ///
  /// The resulting type has a union of the qualifiers from T and the address
  /// space. If T already has an address space specifier, it is silently
  /// replaced.
  QualType getAddrSpaceQualType(QualType T, unsigned AddressSpace) const;

  /// \brief Return the uniqued reference to the type for an Objective-C
  /// gc-qualified type.
  ///
  /// The retulting type has a union of the qualifiers from T and the gc
  /// attribute.
  QualType getObjCGCQualType(QualType T, Qualifiers::GC gcAttr) const;

  /// \brief Return the uniqued reference to the type for a \c restrict
  /// qualified type.
  ///
  /// The resulting type has a union of the qualifiers from \p T and
  /// \c restrict.
  QualType getRestrictType(QualType T) const {
    return T.withFastQualifiers(Qualifiers::Restrict);
  }

  /// \brief Return the uniqued reference to the type for a \c volatile
  /// qualified type.
  ///
  /// The resulting type has a union of the qualifiers from \p T and
  /// \c volatile.
  QualType getVolatileType(QualType T) const {
    return T.withFastQualifiers(Qualifiers::Volatile);
  }

  /// \brief Return the uniqued reference to the type for a \c const
  /// qualified type.
  ///
  /// The resulting type has a union of the qualifiers from \p T and \c const.
  ///
  /// It can be reasonably expected that this will always be equivalent to
  /// calling T.withConst().
  QualType getConstType(QualType T) const { return T.withConst(); }

  /// \brief Change the ExtInfo on a function type.
  const FunctionType *adjustFunctionType(const FunctionType *Fn,
                                         FunctionType::ExtInfo EInfo);

  /// \brief Return the uniqued reference to the type for a complex
  /// number with the specified element type.
  QualType getComplexType(QualType T) const;
  CanQualType getComplexType(CanQualType T) const {
    return CanQualType::CreateUnsafe(getComplexType((QualType) T));
  }

  /// \brief Return the uniqued reference to the type for a pointer to
  /// the specified type.
  QualType getPointerType(QualType T) const;
  CanQualType getPointerType(CanQualType T) const {
    return CanQualType::CreateUnsafe(getPointerType((QualType) T));
  }

  /// \brief Return the uniqued reference to the atomic type for the specified
  /// type.
  QualType getAtomicType(QualType T) const;

  /// \brief Return the uniqued reference to the type for a block of the
  /// specified type.
  QualType getBlockPointerType(QualType T) const;

  /// Gets the struct used to keep track of the descriptor for pointer to
  /// blocks.
  QualType getBlockDescriptorType() const;

  /// Gets the struct used to keep track of the extended descriptor for
  /// pointer to blocks.
  QualType getBlockDescriptorExtendedType() const;

  void setcudaConfigureCallDecl(FunctionDecl *FD) {
    cudaConfigureCallDecl = FD;
  }
  FunctionDecl *getcudaConfigureCallDecl() {
    return cudaConfigureCallDecl;
  }

  /// Returns true iff we need copy/dispose helpers for the given type.
  bool BlockRequiresCopying(QualType Ty, const VarDecl *D);
  
  
  /// Returns true, if given type has a known lifetime. HasByrefExtendedLayout is set
  /// to false in this case. If HasByrefExtendedLayout returns true, byref variable
  /// has extended lifetime. 
  bool getByrefLifetime(QualType Ty,
                        Qualifiers::ObjCLifetime &Lifetime,
                        bool &HasByrefExtendedLayout) const;
  
  /// \brief Return the uniqued reference to the type for an lvalue reference
  /// to the specified type.
  QualType getLValueReferenceType(QualType T, bool SpelledAsLValue = true)
    const;

  /// \brief Return the uniqued reference to the type for an rvalue reference
  /// to the specified type.
  QualType getRValueReferenceType(QualType T) const;

  /// \brief Return the uniqued reference to the type for a member pointer to
  /// the specified type in the specified class.
  ///
  /// The class \p Cls is a \c Type because it could be a dependent name.
  QualType getMemberPointerType(QualType T, const Type *Cls) const;

  /// \brief Return a non-unique reference to the type for a variable array of
  /// the specified element type.
  QualType getVariableArrayType(QualType EltTy, Expr *NumElts,
                                ArrayType::ArraySizeModifier ASM,
                                unsigned IndexTypeQuals,
                                SourceRange Brackets) const;

  /// \brief Return a non-unique reference to the type for a dependently-sized
  /// array of the specified element type.
  ///
  /// FIXME: We will need these to be uniqued, or at least comparable, at some
  /// point.
  QualType getDependentSizedArrayType(QualType EltTy, Expr *NumElts,
                                      ArrayType::ArraySizeModifier ASM,
                                      unsigned IndexTypeQuals,
                                      SourceRange Brackets) const;

  /// \brief Return a unique reference to the type for an incomplete array of
  /// the specified element type.
  QualType getIncompleteArrayType(QualType EltTy,
                                  ArrayType::ArraySizeModifier ASM,
                                  unsigned IndexTypeQuals) const;

  /// \brief Return the unique reference to the type for a constant array of
  /// the specified element type.
  QualType getConstantArrayType(QualType EltTy, const llvm::APInt &ArySize,
                                ArrayType::ArraySizeModifier ASM,
                                unsigned IndexTypeQuals) const;
  
  /// \brief Returns a vla type where known sizes are replaced with [*].
  QualType getVariableArrayDecayedType(QualType Ty) const;

  /// \brief Return the unique reference to a vector type of the specified
  /// element type and size.
  ///
  /// \pre \p VectorType must be a built-in type.
  QualType getVectorType(QualType VectorType, unsigned NumElts,
                         VectorType::VectorKind VecKind) const;

  /// \brief Return the unique reference to an extended vector type
  /// of the specified element type and size.
  ///
  /// \pre \p VectorType must be a built-in type.
  QualType getExtVectorType(QualType VectorType, unsigned NumElts) const;

  /// \pre Return a non-unique reference to the type for a dependently-sized
  /// vector of the specified element type.
  ///
  /// FIXME: We will need these to be uniqued, or at least comparable, at some
  /// point.
  QualType getDependentSizedExtVectorType(QualType VectorType,
                                          Expr *SizeExpr,
                                          SourceLocation AttrLoc) const;

  /// \brief Return a K&R style C function type like 'int()'.
  QualType getFunctionNoProtoType(QualType ResultTy,
                                  const FunctionType::ExtInfo &Info) const;

  QualType getFunctionNoProtoType(QualType ResultTy) const {
    return getFunctionNoProtoType(ResultTy, FunctionType::ExtInfo());
  }

  /// \brief Return a normal function type with a typed argument list.
  QualType getFunctionType(QualType ResultTy, ArrayRef<QualType> Args,
                           const FunctionProtoType::ExtProtoInfo &EPI) const;

  /// \brief Return the unique reference to the type for the specified type
  /// declaration.
  QualType getTypeDeclType(const TypeDecl *Decl,
                           const TypeDecl *PrevDecl = 0) const {
    assert(Decl && "Passed null for Decl param");
    if (Decl->TypeForDecl) return QualType(Decl->TypeForDecl, 0);

    if (PrevDecl) {
      assert(PrevDecl->TypeForDecl && "previous decl has no TypeForDecl");
      Decl->TypeForDecl = PrevDecl->TypeForDecl;
      return QualType(PrevDecl->TypeForDecl, 0);
    }

    return getTypeDeclTypeSlow(Decl);
  }

  /// \brief Return the unique reference to the type for the specified
  /// typedef-name decl.
  QualType getTypedefType(const TypedefNameDecl *Decl,
                          QualType Canon = QualType()) const;

  QualType getRecordType(const RecordDecl *Decl) const;

  QualType getEnumType(const EnumDecl *Decl) const;

  QualType getInjectedClassNameType(CXXRecordDecl *Decl, QualType TST) const;

  QualType getAttributedType(AttributedType::Kind attrKind,
                             QualType modifiedType,
                             QualType equivalentType);

  QualType getSubstTemplateTypeParmType(const TemplateTypeParmType *Replaced,
                                        QualType Replacement) const;
  QualType getSubstTemplateTypeParmPackType(
                                          const TemplateTypeParmType *Replaced,
                                            const TemplateArgument &ArgPack);

  QualType getTemplateTypeParmType(unsigned Depth, unsigned Index,
                                   bool ParameterPack,
                                   TemplateTypeParmDecl *ParmDecl = 0) const;

  QualType getTemplateSpecializationType(TemplateName T,
                                         const TemplateArgument *Args,
                                         unsigned NumArgs,
                                         QualType Canon = QualType()) const;

  QualType getCanonicalTemplateSpecializationType(TemplateName T,
                                                  const TemplateArgument *Args,
                                                  unsigned NumArgs) const;

  QualType getTemplateSpecializationType(TemplateName T,
                                         const TemplateArgumentListInfo &Args,
                                         QualType Canon = QualType()) const;

  TypeSourceInfo *
  getTemplateSpecializationTypeInfo(TemplateName T, SourceLocation TLoc,
                                    const TemplateArgumentListInfo &Args,
                                    QualType Canon = QualType()) const;

  QualType getParenType(QualType NamedType) const;

  QualType getElaboratedType(ElaboratedTypeKeyword Keyword,
                             NestedNameSpecifier *NNS,
                             QualType NamedType) const;
  QualType getDependentNameType(ElaboratedTypeKeyword Keyword,
                                NestedNameSpecifier *NNS,
                                const IdentifierInfo *Name,
                                QualType Canon = QualType()) const;

  QualType getDependentTemplateSpecializationType(ElaboratedTypeKeyword Keyword,
                                                  NestedNameSpecifier *NNS,
                                                  const IdentifierInfo *Name,
                                    const TemplateArgumentListInfo &Args) const;
  QualType getDependentTemplateSpecializationType(ElaboratedTypeKeyword Keyword,
                                                  NestedNameSpecifier *NNS,
                                                  const IdentifierInfo *Name,
                                                  unsigned NumArgs,
                                            const TemplateArgument *Args) const;

  QualType getPackExpansionType(QualType Pattern,
                                Optional<unsigned> NumExpansions);

  QualType getObjCInterfaceType(const ObjCInterfaceDecl *Decl,
                                ObjCInterfaceDecl *PrevDecl = 0) const;

  QualType getObjCObjectType(QualType Base,
                             ObjCProtocolDecl * const *Protocols,
                             unsigned NumProtocols) const;

  /// \brief Return a ObjCObjectPointerType type for the given ObjCObjectType.
  QualType getObjCObjectPointerType(QualType OIT) const;

  /// \brief GCC extension.
  QualType getTypeOfExprType(Expr *e) const;
  QualType getTypeOfType(QualType t) const;

  /// \brief C++11 decltype.
  QualType getDecltypeType(Expr *e, QualType UnderlyingType) const;

  /// \brief Unary type transforms
  QualType getUnaryTransformType(QualType BaseType, QualType UnderlyingType,
                                 UnaryTransformType::UTTKind UKind) const;

  /// \brief C++11 deduced auto type.
  QualType getAutoType(QualType DeducedType) const;

  /// \brief C++11 deduction pattern for 'auto' type.
  QualType getAutoDeductType() const;

  /// \brief C++11 deduction pattern for 'auto &&' type.
  QualType getAutoRRefDeductType() const;

  /// \brief Return the unique reference to the type for the specified TagDecl
  /// (struct/union/class/enum) decl.
  QualType getTagDeclType(const TagDecl *Decl) const;

  /// \brief Return the unique type for "size_t" (C99 7.17), defined in
  /// <stddef.h>.
  ///
  /// The sizeof operator requires this (C99 6.5.3.4p4).
  CanQualType getSizeType() const;

  /// \brief Return the unique type for "intmax_t" (C99 7.18.1.5), defined in
  /// <stdint.h>.
  CanQualType getIntMaxType() const;

  /// \brief Return the unique type for "uintmax_t" (C99 7.18.1.5), defined in
  /// <stdint.h>.
  CanQualType getUIntMaxType() const;

  /// \brief In C++, this returns the unique wchar_t type.  In C99, this
  /// returns a type compatible with the type defined in <stddef.h> as defined
  /// by the target.
  QualType getWCharType() const { return WCharTy; }

  /// \brief Return the type of "signed wchar_t".
  ///
  /// Used when in C++, as a GCC extension.
  QualType getSignedWCharType() const;

  /// \brief Return the type of "unsigned wchar_t".
  ///
  /// Used when in C++, as a GCC extension.
  QualType getUnsignedWCharType() const;

  /// \brief In C99, this returns a type compatible with the type
  /// defined in <stddef.h> as defined by the target.
  QualType getWIntType() const { return WIntTy; }

  /// \brief Return a type compatible with "intptr_t" (C99 7.18.1.4),
  /// as defined by the target.
  QualType getIntPtrType() const;

  /// \brief Return a type compatible with "uintptr_t" (C99 7.18.1.4),
  /// as defined by the target.
  QualType getUIntPtrType() const;

  /// \brief Return the unique type for "ptrdiff_t" (C99 7.17) defined in
  /// <stddef.h>. Pointer - pointer requires this (C99 6.5.6p9).
  QualType getPointerDiffType() const;

  /// \brief Return the unique type for "pid_t" defined in
  /// <sys/types.h>. We need this to compute the correct type for vfork().
  QualType getProcessIDType() const;

  /// \brief Return the C structure type used to represent constant CFStrings.
  QualType getCFConstantStringType() const;
  
  /// \brief Returns the C struct type for objc_super
  QualType getObjCSuperType() const;
  void setObjCSuperType(QualType ST) { ObjCSuperType = ST; }
  
  /// Get the structure type used to representation CFStrings, or NULL
  /// if it hasn't yet been built.
  QualType getRawCFConstantStringType() const {
    if (CFConstantStringTypeDecl)
      return getTagDeclType(CFConstantStringTypeDecl);
    return QualType();
  }
  void setCFConstantStringType(QualType T);

  // This setter/getter represents the ObjC type for an NSConstantString.
  void setObjCConstantStringInterface(ObjCInterfaceDecl *Decl);
  QualType getObjCConstantStringInterface() const {
    return ObjCConstantStringType;
  }

  QualType getObjCNSStringType() const {
    return ObjCNSStringType;
  }
  
  void setObjCNSStringType(QualType T) {
    ObjCNSStringType = T;
  }
  
  /// \brief Retrieve the type that \c id has been defined to, which may be
  /// different from the built-in \c id if \c id has been typedef'd.
  QualType getObjCIdRedefinitionType() const {
    if (ObjCIdRedefinitionType.isNull())
      return getObjCIdType();
    return ObjCIdRedefinitionType;
  }
  
  /// \brief Set the user-written type that redefines \c id.
  void setObjCIdRedefinitionType(QualType RedefType) {
    ObjCIdRedefinitionType = RedefType;
  }

  /// \brief Retrieve the type that \c Class has been defined to, which may be
  /// different from the built-in \c Class if \c Class has been typedef'd.
  QualType getObjCClassRedefinitionType() const {
    if (ObjCClassRedefinitionType.isNull())
      return getObjCClassType();
    return ObjCClassRedefinitionType;
  }
  
  /// \brief Set the user-written type that redefines 'SEL'.
  void setObjCClassRedefinitionType(QualType RedefType) {
    ObjCClassRedefinitionType = RedefType;
  }

  /// \brief Retrieve the type that 'SEL' has been defined to, which may be
  /// different from the built-in 'SEL' if 'SEL' has been typedef'd.
  QualType getObjCSelRedefinitionType() const {
    if (ObjCSelRedefinitionType.isNull())
      return getObjCSelType();
    return ObjCSelRedefinitionType;
  }

  
  /// \brief Set the user-written type that redefines 'SEL'.
  void setObjCSelRedefinitionType(QualType RedefType) {
    ObjCSelRedefinitionType = RedefType;
  }

  /// \brief Retrieve the Objective-C "instancetype" type, if already known;
  /// otherwise, returns a NULL type;
  QualType getObjCInstanceType() {
    return getTypeDeclType(getObjCInstanceTypeDecl());
  }

  /// \brief Retrieve the typedef declaration corresponding to the Objective-C
  /// "instancetype" type.
  TypedefDecl *getObjCInstanceTypeDecl();
  
  /// \brief Set the type for the C FILE type.
  void setFILEDecl(TypeDecl *FILEDecl) { this->FILEDecl = FILEDecl; }

  /// \brief Retrieve the C FILE type.
  QualType getFILEType() const {
    if (FILEDecl)
      return getTypeDeclType(FILEDecl);
    return QualType();
  }

  /// \brief Set the type for the C jmp_buf type.
  void setjmp_bufDecl(TypeDecl *jmp_bufDecl) {
    this->jmp_bufDecl = jmp_bufDecl;
  }

  /// \brief Retrieve the C jmp_buf type.
  QualType getjmp_bufType() const {
    if (jmp_bufDecl)
      return getTypeDeclType(jmp_bufDecl);
    return QualType();
  }

  /// \brief Set the type for the C sigjmp_buf type.
  void setsigjmp_bufDecl(TypeDecl *sigjmp_bufDecl) {
    this->sigjmp_bufDecl = sigjmp_bufDecl;
  }

  /// \brief Retrieve the C sigjmp_buf type.
  QualType getsigjmp_bufType() const {
    if (sigjmp_bufDecl)
      return getTypeDeclType(sigjmp_bufDecl);
    return QualType();
  }

  /// \brief Set the type for the C ucontext_t type.
  void setucontext_tDecl(TypeDecl *ucontext_tDecl) {
    this->ucontext_tDecl = ucontext_tDecl;
  }

  /// \brief Retrieve the C ucontext_t type.
  QualType getucontext_tType() const {
    if (ucontext_tDecl)
      return getTypeDeclType(ucontext_tDecl);
    return QualType();
  }

  /// \brief The result type of logical operations, '<', '>', '!=', etc.
  QualType getLogicalOperationType() const {
    return getLangOpts().CPlusPlus ? BoolTy : IntTy;
  }

  /// \brief Emit the Objective-CC type encoding for the given type \p T into
  /// \p S.
  ///
  /// If \p Field is specified then record field names are also encoded.
  void getObjCEncodingForType(QualType T, std::string &S,
                              const FieldDecl *Field=0) const;

  void getLegacyIntegralTypeEncoding(QualType &t) const;

  /// \brief Put the string version of the type qualifiers \p QT into \p S.
  void getObjCEncodingForTypeQualifier(Decl::ObjCDeclQualifier QT,
                                       std::string &S) const;

  /// \brief Emit the encoded type for the function \p Decl into \p S.
  ///
  /// This is in the same format as Objective-C method encodings.
  ///
  /// \returns true if an error occurred (e.g., because one of the parameter
  /// types is incomplete), false otherwise.
  bool getObjCEncodingForFunctionDecl(const FunctionDecl *Decl, std::string& S);

  /// \brief Emit the encoded type for the method declaration \p Decl into
  /// \p S.
  ///
  /// \returns true if an error occurred (e.g., because one of the parameter
  /// types is incomplete), false otherwise.
  bool getObjCEncodingForMethodDecl(const ObjCMethodDecl *Decl, std::string &S,
                                    bool Extended = false)
    const;

  /// \brief Return the encoded type for this block declaration.
  std::string getObjCEncodingForBlock(const BlockExpr *blockExpr) const;
  
  /// getObjCEncodingForPropertyDecl - Return the encoded type for
  /// this method declaration. If non-NULL, Container must be either
  /// an ObjCCategoryImplDecl or ObjCImplementationDecl; it should
  /// only be NULL when getting encodings for protocol properties.
  void getObjCEncodingForPropertyDecl(const ObjCPropertyDecl *PD,
                                      const Decl *Container,
                                      std::string &S) const;

  bool ProtocolCompatibleWithProtocol(ObjCProtocolDecl *lProto,
                                      ObjCProtocolDecl *rProto) const;

  /// \brief Return the size of type \p T for Objective-C encoding purpose,
  /// in characters.
  CharUnits getObjCEncodingTypeSize(QualType T) const;

  /// \brief Retrieve the typedef corresponding to the predefined \c id type
  /// in Objective-C.
  TypedefDecl *getObjCIdDecl() const;
  
  /// \brief Represents the Objective-CC \c id type.
  ///
  /// This is set up lazily, by Sema.  \c id is always a (typedef for a)
  /// pointer type, a pointer to a struct.
  QualType getObjCIdType() const {
    return getTypeDeclType(getObjCIdDecl());
  }

  /// \brief Retrieve the typedef corresponding to the predefined 'SEL' type
  /// in Objective-C.
  TypedefDecl *getObjCSelDecl() const;
  
  /// \brief Retrieve the type that corresponds to the predefined Objective-C
  /// 'SEL' type.
  QualType getObjCSelType() const { 
    return getTypeDeclType(getObjCSelDecl());
  }

  /// \brief Retrieve the typedef declaration corresponding to the predefined
  /// Objective-C 'Class' type.
  TypedefDecl *getObjCClassDecl() const;
  
  /// \brief Represents the Objective-C \c Class type.
  ///
  /// This is set up lazily, by Sema.  \c Class is always a (typedef for a)
  /// pointer type, a pointer to a struct.
  QualType getObjCClassType() const { 
    return getTypeDeclType(getObjCClassDecl());
  }

  /// \brief Retrieve the Objective-C class declaration corresponding to 
  /// the predefined \c Protocol class.
  ObjCInterfaceDecl *getObjCProtocolDecl() const;

  /// \brief Retrieve declaration of 'BOOL' typedef
  TypedefDecl *getBOOLDecl() const {
    return BOOLDecl;
  }

  /// \brief Save declaration of 'BOOL' typedef
  void setBOOLDecl(TypedefDecl *TD) {
    BOOLDecl = TD;
  }

  /// \brief type of 'BOOL' type.
  QualType getBOOLType() const {
    return getTypeDeclType(getBOOLDecl());
  }
  
  /// \brief Retrieve the type of the Objective-C \c Protocol class.
  QualType getObjCProtoType() const {
    return getObjCInterfaceType(getObjCProtocolDecl());
  }
  
  /// \brief Retrieve the C type declaration corresponding to the predefined
  /// \c __builtin_va_list type.
  TypedefDecl *getBuiltinVaListDecl() const;

  /// \brief Retrieve the type of the \c __builtin_va_list type.
  QualType getBuiltinVaListType() const {
    return getTypeDeclType(getBuiltinVaListDecl());
  }

  /// \brief Retrieve the C type declaration corresponding to the predefined
  /// \c __va_list_tag type used to help define the \c __builtin_va_list type
  /// for some targets.
  QualType getVaListTagType() const;

  /// \brief Return a type with additional \c const, \c volatile, or
  /// \c restrict qualifiers.
  QualType getCVRQualifiedType(QualType T, unsigned CVR) const {
    return getQualifiedType(T, Qualifiers::fromCVRMask(CVR));
  }

  /// \brief Un-split a SplitQualType.
  QualType getQualifiedType(SplitQualType split) const {
    return getQualifiedType(split.Ty, split.Quals);
  }

  /// \brief Return a type with additional qualifiers.
  QualType getQualifiedType(QualType T, Qualifiers Qs) const {
    if (!Qs.hasNonFastQualifiers())
      return T.withFastQualifiers(Qs.getFastQualifiers());
    QualifierCollector Qc(Qs);
    const Type *Ptr = Qc.strip(T);
    return getExtQualType(Ptr, Qc);
  }

  /// \brief Return a type with additional qualifiers.
  QualType getQualifiedType(const Type *T, Qualifiers Qs) const {
    if (!Qs.hasNonFastQualifiers())
      return QualType(T, Qs.getFastQualifiers());
    return getExtQualType(T, Qs);
  }

  /// \brief Return a type with the given lifetime qualifier.
  ///
  /// \pre Neither type.ObjCLifetime() nor \p lifetime may be \c OCL_None.
  QualType getLifetimeQualifiedType(QualType type,
                                    Qualifiers::ObjCLifetime lifetime) {
    assert(type.getObjCLifetime() == Qualifiers::OCL_None);
    assert(lifetime != Qualifiers::OCL_None);

    Qualifiers qs;
    qs.addObjCLifetime(lifetime);
    return getQualifiedType(type, qs);
  }

  DeclarationNameInfo getNameForTemplate(TemplateName Name,
                                         SourceLocation NameLoc) const;

  TemplateName getOverloadedTemplateName(UnresolvedSetIterator Begin,
                                         UnresolvedSetIterator End) const;

  TemplateName getQualifiedTemplateName(NestedNameSpecifier *NNS,
                                        bool TemplateKeyword,
                                        TemplateDecl *Template) const;

  TemplateName getDependentTemplateName(NestedNameSpecifier *NNS,
                                        const IdentifierInfo *Name) const;
  TemplateName getDependentTemplateName(NestedNameSpecifier *NNS,
                                        OverloadedOperatorKind Operator) const;
  TemplateName getSubstTemplateTemplateParm(TemplateTemplateParmDecl *param,
                                            TemplateName replacement) const;
  TemplateName getSubstTemplateTemplateParmPack(TemplateTemplateParmDecl *Param,
                                        const TemplateArgument &ArgPack) const;
  
  enum GetBuiltinTypeError {
    GE_None,              ///< No error
    GE_Missing_stdio,     ///< Missing a type from <stdio.h>
    GE_Missing_setjmp,    ///< Missing a type from <setjmp.h>
    GE_Missing_ucontext   ///< Missing a type from <ucontext.h>
  };

  /// \brief Return the type for the specified builtin.
  ///
  /// If \p IntegerConstantArgs is non-null, it is filled in with a bitmask of
  /// arguments to the builtin that are required to be integer constant
  /// expressions.
  QualType GetBuiltinType(unsigned ID, GetBuiltinTypeError &Error,
                          unsigned *IntegerConstantArgs = 0) const;

private:
  CanQualType getFromTargetType(unsigned Type) const;
  std::pair<uint64_t, unsigned> getTypeInfoImpl(const Type *T) const;

  //===--------------------------------------------------------------------===//
  //                         Type Predicates.
  //===--------------------------------------------------------------------===//

public:
  /// \brief Return one of the GCNone, Weak or Strong Objective-C garbage
  /// collection attributes.
  Qualifiers::GC getObjCGCAttrKind(QualType Ty) const;

  /// \brief Return true if the given vector types are of the same unqualified
  /// type or if they are equivalent to the same GCC vector type.
  ///
  /// \note This ignores whether they are target-specific (AltiVec or Neon)
  /// types.
  bool areCompatibleVectorTypes(QualType FirstVec, QualType SecondVec);

  /// \brief Return true if this is an \c NSObject object with its \c NSObject
  /// attribute set.
  static bool isObjCNSObjectType(QualType Ty) {
    return Ty->isObjCNSObjectType();
  }

  //===--------------------------------------------------------------------===//
  //                         Type Sizing and Analysis
  //===--------------------------------------------------------------------===//

  /// \brief Return the APFloat 'semantics' for the specified scalar floating
  /// point type.
  const llvm::fltSemantics &getFloatTypeSemantics(QualType T) const;

  /// \brief Get the size and alignment of the specified complete type in bits.
  std::pair<uint64_t, unsigned> getTypeInfo(const Type *T) const;
  std::pair<uint64_t, unsigned> getTypeInfo(QualType T) const {
    return getTypeInfo(T.getTypePtr());
  }

  /// \brief Return the size of the specified (complete) type \p T, in bits.
  uint64_t getTypeSize(QualType T) const {
    return getTypeInfo(T).first;
  }
  uint64_t getTypeSize(const Type *T) const {
    return getTypeInfo(T).first;
  }

  /// \brief Return the size of the character type, in bits.
  uint64_t getCharWidth() const {
    return getTypeSize(CharTy);
  }
  
  /// \brief Convert a size in bits to a size in characters.
  CharUnits toCharUnitsFromBits(int64_t BitSize) const;

  /// \brief Convert a size in characters to a size in bits.
  int64_t toBits(CharUnits CharSize) const;

  /// \brief Return the size of the specified (complete) type \p T, in
  /// characters.
  CharUnits getTypeSizeInChars(QualType T) const;
  CharUnits getTypeSizeInChars(const Type *T) const;

  /// \brief Return the ABI-specified alignment of a (complete) type \p T, in
  /// bits.
  unsigned getTypeAlign(QualType T) const {
    return getTypeInfo(T).second;
  }
  unsigned getTypeAlign(const Type *T) const {
    return getTypeInfo(T).second;
  }

  /// \brief Return the ABI-specified alignment of a (complete) type \p T, in 
  /// characters.
  CharUnits getTypeAlignInChars(QualType T) const;
  CharUnits getTypeAlignInChars(const Type *T) const;
  
  // getTypeInfoDataSizeInChars - Return the size of a type, in chars. If the
  // type is a record, its data size is returned.
  std::pair<CharUnits, CharUnits> getTypeInfoDataSizeInChars(QualType T) const;

  std::pair<CharUnits, CharUnits> getTypeInfoInChars(const Type *T) const;
  std::pair<CharUnits, CharUnits> getTypeInfoInChars(QualType T) const;

  /// \brief Return the "preferred" alignment of the specified type \p T for
  /// the current target, in bits.
  ///
  /// This can be different than the ABI alignment in cases where it is
  /// beneficial for performance to overalign a data type.
  unsigned getPreferredTypeAlign(const Type *T) const;

  /// \brief Return a conservative estimate of the alignment of the specified
  /// decl \p D.
  ///
  /// \pre \p D must not be a bitfield type, as bitfields do not have a valid
  /// alignment.
  ///
  /// If \p RefAsPointee, references are treated like their underlying type
  /// (for alignof), else they're treated like pointers (for CodeGen).
  CharUnits getDeclAlign(const Decl *D, bool RefAsPointee = false) const;

  /// \brief Get or compute information about the layout of the specified
  /// record (struct/union/class) \p D, which indicates its size and field
  /// position information.
  const ASTRecordLayout &getASTRecordLayout(const RecordDecl *D) const;

  /// \brief Get or compute information about the layout of the specified
  /// Objective-C interface.
  const ASTRecordLayout &getASTObjCInterfaceLayout(const ObjCInterfaceDecl *D)
    const;

  void DumpRecordLayout(const RecordDecl *RD, raw_ostream &OS,
                        bool Simple = false) const;

  /// \brief Get or compute information about the layout of the specified
  /// Objective-C implementation.
  ///
  /// This may differ from the interface if synthesized ivars are present.
  const ASTRecordLayout &
  getASTObjCImplementationLayout(const ObjCImplementationDecl *D) const;

  /// \brief Get our current best idea for the key function of the
  /// given record decl, or NULL if there isn't one.
  ///
  /// The key function is, according to the Itanium C++ ABI section 5.2.3:
  ///   ...the first non-pure virtual function that is not inline at the
  ///   point of class definition.
  ///
  /// Other ABIs use the same idea.  However, the ARM C++ ABI ignores
  /// virtual functions that are defined 'inline', which means that
  /// the result of this computation can change.
  const CXXMethodDecl *getCurrentKeyFunction(const CXXRecordDecl *RD);

  /// \brief Observe that the given method cannot be a key function.
  /// Checks the key-function cache for the method's class and clears it
  /// if matches the given declaration.
  ///
  /// This is used in ABIs where out-of-line definitions marked
  /// inline are not considered to be key functions.
  ///
  /// \param method should be the declaration from the class definition
  void setNonKeyFunction(const CXXMethodDecl *method);

  /// Get the offset of a FieldDecl or IndirectFieldDecl, in bits.
  uint64_t getFieldOffset(const ValueDecl *FD) const;

  bool isNearlyEmpty(const CXXRecordDecl *RD) const;

  MangleContext *createMangleContext();
  
  void DeepCollectObjCIvars(const ObjCInterfaceDecl *OI, bool leafClass,
                            SmallVectorImpl<const ObjCIvarDecl*> &Ivars) const;
  
  unsigned CountNonClassIvars(const ObjCInterfaceDecl *OI) const;
  void CollectInheritedProtocols(const Decl *CDecl,
                          llvm::SmallPtrSet<ObjCProtocolDecl*, 8> &Protocols);

  //===--------------------------------------------------------------------===//
  //                            Type Operators
  //===--------------------------------------------------------------------===//

  /// \brief Return the canonical (structural) type corresponding to the
  /// specified potentially non-canonical type \p T.
  ///
  /// The non-canonical version of a type may have many "decorated" versions of
  /// types.  Decorators can include typedefs, 'typeof' operators, etc. The
  /// returned type is guaranteed to be free of any of these, allowing two
  /// canonical types to be compared for exact equality with a simple pointer
  /// comparison.
  CanQualType getCanonicalType(QualType T) const {
    return CanQualType::CreateUnsafe(T.getCanonicalType());
  }

  const Type *getCanonicalType(const Type *T) const {
    return T->getCanonicalTypeInternal().getTypePtr();
  }

  /// \brief Return the canonical parameter type corresponding to the specific
  /// potentially non-canonical one.
  ///
  /// Qualifiers are stripped off, functions are turned into function
  /// pointers, and arrays decay one level into pointers.
  CanQualType getCanonicalParamType(QualType T) const;

  /// \brief Determine whether the given types \p T1 and \p T2 are equivalent.
  bool hasSameType(QualType T1, QualType T2) const {
    return getCanonicalType(T1) == getCanonicalType(T2);
  }

  /// \brief Return this type as a completely-unqualified array type,
  /// capturing the qualifiers in \p Quals.
  ///
  /// This will remove the minimal amount of sugaring from the types, similar
  /// to the behavior of QualType::getUnqualifiedType().
  ///
  /// \param T is the qualified type, which may be an ArrayType
  ///
  /// \param Quals will receive the full set of qualifiers that were
  /// applied to the array.
  ///
  /// \returns if this is an array type, the completely unqualified array type
  /// that corresponds to it. Otherwise, returns T.getUnqualifiedType().
  QualType getUnqualifiedArrayType(QualType T, Qualifiers &Quals);

  /// \brief Determine whether the given types are equivalent after
  /// cvr-qualifiers have been removed.
  bool hasSameUnqualifiedType(QualType T1, QualType T2) const {
    return getCanonicalType(T1).getTypePtr() ==
           getCanonicalType(T2).getTypePtr();
  }

  bool UnwrapSimilarPointerTypes(QualType &T1, QualType &T2);
  
  /// \brief Retrieves the "canonical" nested name specifier for a
  /// given nested name specifier.
  ///
  /// The canonical nested name specifier is a nested name specifier
  /// that uniquely identifies a type or namespace within the type
  /// system. For example, given:
  ///
  /// \code
  /// namespace N {
  ///   struct S {
  ///     template<typename T> struct X { typename T* type; };
  ///   };
  /// }
  ///
  /// template<typename T> struct Y {
  ///   typename N::S::X<T>::type member;
  /// };
  /// \endcode
  ///
  /// Here, the nested-name-specifier for N::S::X<T>:: will be
  /// S::X<template-param-0-0>, since 'S' and 'X' are uniquely defined
  /// by declarations in the type system and the canonical type for
  /// the template type parameter 'T' is template-param-0-0.
  NestedNameSpecifier *
  getCanonicalNestedNameSpecifier(NestedNameSpecifier *NNS) const;

  /// \brief Retrieves the default calling convention to use for
  /// C++ instance methods.
  CallingConv getDefaultCXXMethodCallConv(bool isVariadic);

  /// \brief Retrieves the canonical representation of the given
  /// calling convention.
  CallingConv getCanonicalCallConv(CallingConv CC) const;

  /// \brief Determines whether two calling conventions name the same
  /// calling convention.
  bool isSameCallConv(CallingConv lcc, CallingConv rcc) {
    return (getCanonicalCallConv(lcc) == getCanonicalCallConv(rcc));
  }

  /// \brief Retrieves the "canonical" template name that refers to a
  /// given template.
  ///
  /// The canonical template name is the simplest expression that can
  /// be used to refer to a given template. For most templates, this
  /// expression is just the template declaration itself. For example,
  /// the template std::vector can be referred to via a variety of
  /// names---std::vector, \::std::vector, vector (if vector is in
  /// scope), etc.---but all of these names map down to the same
  /// TemplateDecl, which is used to form the canonical template name.
  ///
  /// Dependent template names are more interesting. Here, the
  /// template name could be something like T::template apply or
  /// std::allocator<T>::template rebind, where the nested name
  /// specifier itself is dependent. In this case, the canonical
  /// template name uses the shortest form of the dependent
  /// nested-name-specifier, which itself contains all canonical
  /// types, values, and templates.
  TemplateName getCanonicalTemplateName(TemplateName Name) const;

  /// \brief Determine whether the given template names refer to the same
  /// template.
  bool hasSameTemplateName(TemplateName X, TemplateName Y);
  
  /// \brief Retrieve the "canonical" template argument.
  ///
  /// The canonical template argument is the simplest template argument
  /// (which may be a type, value, expression, or declaration) that
  /// expresses the value of the argument.
  TemplateArgument getCanonicalTemplateArgument(const TemplateArgument &Arg)
    const;

  /// Type Query functions.  If the type is an instance of the specified class,
  /// return the Type pointer for the underlying maximally pretty type.  This
  /// is a member of ASTContext because this may need to do some amount of
  /// canonicalization, e.g. to move type qualifiers into the element type.
  const ArrayType *getAsArrayType(QualType T) const;
  const ConstantArrayType *getAsConstantArrayType(QualType T) const {
    return dyn_cast_or_null<ConstantArrayType>(getAsArrayType(T));
  }
  const VariableArrayType *getAsVariableArrayType(QualType T) const {
    return dyn_cast_or_null<VariableArrayType>(getAsArrayType(T));
  }
  const IncompleteArrayType *getAsIncompleteArrayType(QualType T) const {
    return dyn_cast_or_null<IncompleteArrayType>(getAsArrayType(T));
  }
  const DependentSizedArrayType *getAsDependentSizedArrayType(QualType T)
    const {
    return dyn_cast_or_null<DependentSizedArrayType>(getAsArrayType(T));
  }
  
  /// \brief Return the innermost element type of an array type.
  ///
  /// For example, will return "int" for int[m][n]
  QualType getBaseElementType(const ArrayType *VAT) const;

  /// \brief Return the innermost element type of a type (which needn't
  /// actually be an array type).
  QualType getBaseElementType(QualType QT) const;

  /// \brief Return number of constant array elements.
  uint64_t getConstantArrayElementCount(const ConstantArrayType *CA) const;

  /// \brief Perform adjustment on the parameter type of a function.
  ///
  /// This routine adjusts the given parameter type @p T to the actual
  /// parameter type used by semantic analysis (C99 6.7.5.3p[7,8],
  /// C++ [dcl.fct]p3). The adjusted parameter type is returned.
  QualType getAdjustedParameterType(QualType T) const;
  
  /// \brief Retrieve the parameter type as adjusted for use in the signature
  /// of a function, decaying array and function types and removing top-level
  /// cv-qualifiers.
  QualType getSignatureParameterType(QualType T) const;
  
  /// \brief Return the properly qualified result of decaying the specified
  /// array type to a pointer.
  ///
  /// This operation is non-trivial when handling typedefs etc.  The canonical
  /// type of \p T must be an array type, this returns a pointer to a properly
  /// qualified element of the array.
  ///
  /// See C99 6.7.5.3p7 and C99 6.3.2.1p3.
  QualType getArrayDecayedType(QualType T) const;

  /// \brief Return the type that \p PromotableType will promote to: C99
  /// 6.3.1.1p2, assuming that \p PromotableType is a promotable integer type.
  QualType getPromotedIntegerType(QualType PromotableType) const;

  /// \brief Recurses in pointer/array types until it finds an Objective-C
  /// retainable type and returns its ownership.
  Qualifiers::ObjCLifetime getInnerObjCOwnership(QualType T) const;

  /// \brief Whether this is a promotable bitfield reference according
  /// to C99 6.3.1.1p2, bullet 2 (and GCC extensions).
  ///
  /// \returns the type this bit-field will promote to, or NULL if no
  /// promotion occurs.
  QualType isPromotableBitField(Expr *E) const;

  /// \brief Return the highest ranked integer type, see C99 6.3.1.8p1. 
  ///
  /// If \p LHS > \p RHS, returns 1.  If \p LHS == \p RHS, returns 0.  If
  /// \p LHS < \p RHS, return -1.
  int getIntegerTypeOrder(QualType LHS, QualType RHS) const;

  /// \brief Compare the rank of the two specified floating point types,
  /// ignoring the domain of the type (i.e. 'double' == '_Complex double').
  ///
  /// If \p LHS > \p RHS, returns 1.  If \p LHS == \p RHS, returns 0.  If
  /// \p LHS < \p RHS, return -1.
  int getFloatingTypeOrder(QualType LHS, QualType RHS) const;

  /// \brief Return a real floating point or a complex type (based on
  /// \p typeDomain/\p typeSize).
  ///
  /// \param typeDomain a real floating point or complex type.
  /// \param typeSize a real floating point or complex type.
  QualType getFloatingTypeOfSizeWithinDomain(QualType typeSize,
                                             QualType typeDomain) const;

  unsigned getTargetAddressSpace(QualType T) const {
    return getTargetAddressSpace(T.getQualifiers());
  }

  unsigned getTargetAddressSpace(Qualifiers Q) const {
    return getTargetAddressSpace(Q.getAddressSpace());
  }

  unsigned getTargetAddressSpace(unsigned AS) const {
    if (AS < LangAS::Offset || AS >= LangAS::Offset + LangAS::Count)
      return AS;
    else
      return (*AddrSpaceMap)[AS - LangAS::Offset];
  }

private:
  // Helper for integer ordering
  unsigned getIntegerRank(const Type *T) const;

public:

  //===--------------------------------------------------------------------===//
  //                    Type Compatibility Predicates
  //===--------------------------------------------------------------------===//

  /// Compatibility predicates used to check assignment expressions.
  bool typesAreCompatible(QualType T1, QualType T2, 
                          bool CompareUnqualified = false); // C99 6.2.7p1

  bool propertyTypesAreCompatible(QualType, QualType); 
  bool typesAreBlockPointerCompatible(QualType, QualType); 

  bool isObjCIdType(QualType T) const {
    return T == getObjCIdType();
  }
  bool isObjCClassType(QualType T) const {
    return T == getObjCClassType();
  }
  bool isObjCSelType(QualType T) const {
    return T == getObjCSelType();
  }
  bool QualifiedIdConformsQualifiedId(QualType LHS, QualType RHS);
  bool ObjCQualifiedIdTypesAreCompatible(QualType LHS, QualType RHS,
                                         bool ForCompare);

  bool ObjCQualifiedClassTypesAreCompatible(QualType LHS, QualType RHS);
  
  // Check the safety of assignment from LHS to RHS
  bool canAssignObjCInterfaces(const ObjCObjectPointerType *LHSOPT,
                               const ObjCObjectPointerType *RHSOPT);
  bool canAssignObjCInterfaces(const ObjCObjectType *LHS,
                               const ObjCObjectType *RHS);
  bool canAssignObjCInterfacesInBlockPointer(
                                          const ObjCObjectPointerType *LHSOPT,
                                          const ObjCObjectPointerType *RHSOPT,
                                          bool BlockReturnType);
  bool areComparableObjCPointerTypes(QualType LHS, QualType RHS);
  QualType areCommonBaseCompatible(const ObjCObjectPointerType *LHSOPT,
                                   const ObjCObjectPointerType *RHSOPT);
  bool canBindObjCObjectType(QualType To, QualType From);

  // Functions for calculating composite types
  QualType mergeTypes(QualType, QualType, bool OfBlockPointer=false,
                      bool Unqualified = false, bool BlockReturnType = false);
  QualType mergeFunctionTypes(QualType, QualType, bool OfBlockPointer=false,
                              bool Unqualified = false);
  QualType mergeFunctionArgumentTypes(QualType, QualType,
                                      bool OfBlockPointer=false,
                                      bool Unqualified = false);
  QualType mergeTransparentUnionType(QualType, QualType,
                                     bool OfBlockPointer=false,
                                     bool Unqualified = false);
  
  QualType mergeObjCGCQualifiers(QualType, QualType);
    
  bool FunctionTypesMatchOnNSConsumedAttrs(
         const FunctionProtoType *FromFunctionType,
         const FunctionProtoType *ToFunctionType);

  void ResetObjCLayout(const ObjCContainerDecl *CD) {
    ObjCLayouts[CD] = 0;
  }

  //===--------------------------------------------------------------------===//
  //                    Integer Predicates
  //===--------------------------------------------------------------------===//

  // The width of an integer, as defined in C99 6.2.6.2. This is the number
  // of bits in an integer type excluding any padding bits.
  unsigned getIntWidth(QualType T) const;

  // Per C99 6.2.5p6, for every signed integer type, there is a corresponding
  // unsigned integer type.  This method takes a signed type, and returns the
  // corresponding unsigned integer type.
  QualType getCorrespondingUnsignedType(QualType T) const;

  //===--------------------------------------------------------------------===//
  //                    Type Iterators.
  //===--------------------------------------------------------------------===//

  typedef SmallVectorImpl<Type *>::iterator       type_iterator;
  typedef SmallVectorImpl<Type *>::const_iterator const_type_iterator;

  type_iterator types_begin() { return Types.begin(); }
  type_iterator types_end() { return Types.end(); }
  const_type_iterator types_begin() const { return Types.begin(); }
  const_type_iterator types_end() const { return Types.end(); }

  //===--------------------------------------------------------------------===//
  //                    Integer Values
  //===--------------------------------------------------------------------===//

  /// \brief Make an APSInt of the appropriate width and signedness for the
  /// given \p Value and integer \p Type.
  llvm::APSInt MakeIntValue(uint64_t Value, QualType Type) const {
    llvm::APSInt Res(getIntWidth(Type), 
                     !Type->isSignedIntegerOrEnumerationType());
    Res = Value;
    return Res;
  }

  bool isSentinelNullExpr(const Expr *E);

  /// \brief Get the implementation of the ObjCInterfaceDecl \p D, or NULL if
  /// none exists.
  ObjCImplementationDecl *getObjCImplementation(ObjCInterfaceDecl *D);
  /// \brief Get the implementation of the ObjCCategoryDecl \p D, or NULL if
  /// none exists.
  ObjCCategoryImplDecl   *getObjCImplementation(ObjCCategoryDecl *D);

  /// \brief Return true if there is at least one \@implementation in the TU.
  bool AnyObjCImplementation() {
    return !ObjCImpls.empty();
  }

  /// \brief Set the implementation of ObjCInterfaceDecl.
  void setObjCImplementation(ObjCInterfaceDecl *IFaceD,
                             ObjCImplementationDecl *ImplD);
  /// \brief Set the implementation of ObjCCategoryDecl.
  void setObjCImplementation(ObjCCategoryDecl *CatD,
                             ObjCCategoryImplDecl *ImplD);

  /// \brief Get the duplicate declaration of a ObjCMethod in the same
  /// interface, or null if none exists.
  const ObjCMethodDecl *getObjCMethodRedeclaration(
                                               const ObjCMethodDecl *MD) const {
    return ObjCMethodRedecls.lookup(MD);
  }

  void setObjCMethodRedeclaration(const ObjCMethodDecl *MD,
                                  const ObjCMethodDecl *Redecl) {
    assert(!getObjCMethodRedeclaration(MD) && "MD already has a redeclaration");
    ObjCMethodRedecls[MD] = Redecl;
  }

  /// \brief Returns the Objective-C interface that \p ND belongs to if it is
  /// an Objective-C method/property/ivar etc. that is part of an interface,
  /// otherwise returns null.
  const ObjCInterfaceDecl *getObjContainingInterface(const NamedDecl *ND) const;
  
  /// \brief Set the copy inialization expression of a block var decl.
  void setBlockVarCopyInits(VarDecl*VD, Expr* Init);
  /// \brief Get the copy initialization expression of the VarDecl \p VD, or
  /// NULL if none exists.
  Expr *getBlockVarCopyInits(const VarDecl* VD);

  /// \brief Allocate an uninitialized TypeSourceInfo.
  ///
  /// The caller should initialize the memory held by TypeSourceInfo using
  /// the TypeLoc wrappers.
  ///
  /// \param T the type that will be the basis for type source info. This type
  /// should refer to how the declarator was written in source code, not to
  /// what type semantic analysis resolved the declarator to.
  ///
  /// \param Size the size of the type info to create, or 0 if the size
  /// should be calculated based on the type.
  TypeSourceInfo *CreateTypeSourceInfo(QualType T, unsigned Size = 0) const;

  /// \brief Allocate a TypeSourceInfo where all locations have been
  /// initialized to a given location, which defaults to the empty
  /// location.
  TypeSourceInfo *
  getTrivialTypeSourceInfo(QualType T, 
                           SourceLocation Loc = SourceLocation()) const;

  TypeSourceInfo *getNullTypeSourceInfo() { return &NullTypeSourceInfo; }

  /// \brief Add a deallocation callback that will be invoked when the 
  /// ASTContext is destroyed.
  ///
  /// \param Callback A callback function that will be invoked on destruction.
  ///
  /// \param Data Pointer data that will be provided to the callback function
  /// when it is called.
  void AddDeallocation(void (*Callback)(void*), void *Data);

  GVALinkage GetGVALinkageForFunction(const FunctionDecl *FD);
  GVALinkage GetGVALinkageForVariable(const VarDecl *VD);

  /// \brief Determines if the decl can be CodeGen'ed or deserialized from PCH
  /// lazily, only when used; this is only relevant for function or file scoped
  /// var definitions.
  ///
  /// \returns true if the function/var must be CodeGen'ed/deserialized even if
  /// it is not used.
  bool DeclMustBeEmitted(const Decl *D);

  void addUnnamedTag(const TagDecl *Tag);
  int getUnnamedTagManglingNumber(const TagDecl *Tag) const;

  /// \brief Retrieve the lambda mangling number for a lambda expression.
  unsigned getLambdaManglingNumber(CXXMethodDecl *CallOperator);
  
  /// \brief Used by ParmVarDecl to store on the side the
  /// index of the parameter when it exceeds the size of the normal bitfield.
  void setParameterIndex(const ParmVarDecl *D, unsigned index);

  /// \brief Used by ParmVarDecl to retrieve on the side the
  /// index of the parameter when it exceeds the size of the normal bitfield.
  unsigned getParameterIndex(const ParmVarDecl *D) const;
  
  //===--------------------------------------------------------------------===//
  //                    Statistics
  //===--------------------------------------------------------------------===//

  /// \brief The number of implicitly-declared default constructors.
  static unsigned NumImplicitDefaultConstructors;
  
  /// \brief The number of implicitly-declared default constructors for 
  /// which declarations were built.
  static unsigned NumImplicitDefaultConstructorsDeclared;

  /// \brief The number of implicitly-declared copy constructors.
  static unsigned NumImplicitCopyConstructors;
  
  /// \brief The number of implicitly-declared copy constructors for 
  /// which declarations were built.
  static unsigned NumImplicitCopyConstructorsDeclared;

  /// \brief The number of implicitly-declared move constructors.
  static unsigned NumImplicitMoveConstructors;

  /// \brief The number of implicitly-declared move constructors for
  /// which declarations were built.
  static unsigned NumImplicitMoveConstructorsDeclared;

  /// \brief The number of implicitly-declared copy assignment operators.
  static unsigned NumImplicitCopyAssignmentOperators;
  
  /// \brief The number of implicitly-declared copy assignment operators for 
  /// which declarations were built.
  static unsigned NumImplicitCopyAssignmentOperatorsDeclared;

  /// \brief The number of implicitly-declared move assignment operators.
  static unsigned NumImplicitMoveAssignmentOperators;
  
  /// \brief The number of implicitly-declared move assignment operators for 
  /// which declarations were built.
  static unsigned NumImplicitMoveAssignmentOperatorsDeclared;

  /// \brief The number of implicitly-declared destructors.
  static unsigned NumImplicitDestructors;
  
  /// \brief The number of implicitly-declared destructors for which 
  /// declarations were built.
  static unsigned NumImplicitDestructorsDeclared;
  
private:
  ASTContext(const ASTContext &) LLVM_DELETED_FUNCTION;
  void operator=(const ASTContext &) LLVM_DELETED_FUNCTION;

public:
  /// \brief Initialize built-in types.
  ///
  /// This routine may only be invoked once for a given ASTContext object.
  /// It is normally invoked by the ASTContext constructor. However, the
  /// constructor can be asked to delay initialization, which places the burden
  /// of calling this function on the user of that object.
  ///
  /// \param Target The target 
  void InitBuiltinTypes(const TargetInfo &Target);
  
private:
  void InitBuiltinType(CanQualType &R, BuiltinType::Kind K);

  // Return the Objective-C type encoding for a given type.
  void getObjCEncodingForTypeImpl(QualType t, std::string &S,
                                  bool ExpandPointedToStructures,
                                  bool ExpandStructures,
                                  const FieldDecl *Field,
                                  bool OutermostType = false,
                                  bool EncodingProperty = false,
                                  bool StructField = false,
                                  bool EncodeBlockParameters = false,
                                  bool EncodeClassNames = false,
                                  bool EncodePointerToObjCTypedef = false) const;

  // Adds the encoding of the structure's members.
  void getObjCEncodingForStructureImpl(RecordDecl *RD, std::string &S,
                                       const FieldDecl *Field,
                                       bool includeVBases = true) const;

  // Adds the encoding of a method parameter or return type.
  void getObjCEncodingForMethodParameter(Decl::ObjCDeclQualifier QT,
                                         QualType T, std::string& S,
                                         bool Extended) const;

  const ASTRecordLayout &
  getObjCLayout(const ObjCInterfaceDecl *D,
                const ObjCImplementationDecl *Impl) const;

private:
  /// \brief A set of deallocations that should be performed when the 
  /// ASTContext is destroyed.
  SmallVector<std::pair<void (*)(void*), void *>, 16> Deallocations;
                                       
  // FIXME: This currently contains the set of StoredDeclMaps used
  // by DeclContext objects.  This probably should not be in ASTContext,
  // but we include it here so that ASTContext can quickly deallocate them.
  llvm::PointerIntPair<StoredDeclsMap*,1> LastSDM;

  /// \brief A counter used to uniquely identify "blocks".
  mutable unsigned int UniqueBlockByRefTypeID;
  
  friend class DeclContext;
  friend class DeclarationNameTable;
  void ReleaseDeclContextMaps();

  /// \brief A \c RecursiveASTVisitor that builds a map from nodes to their
  /// parents as defined by the \c RecursiveASTVisitor.
  ///
  /// Note that the relationship described here is purely in terms of AST
  /// traversal - there are other relationships (for example declaration context)
  /// in the AST that are better modeled by special matchers.
  ///
  /// FIXME: Currently only builds up the map using \c Stmt and \c Decl nodes.
  class ParentMapASTVisitor : public RecursiveASTVisitor<ParentMapASTVisitor> {
  public:
    /// \brief Builds and returns the translation unit's parent map.
    ///
    ///  The caller takes ownership of the returned \c ParentMap.
    static ParentMap *buildMap(TranslationUnitDecl &TU) {
      ParentMapASTVisitor Visitor(new ParentMap);
      Visitor.TraverseDecl(&TU);
      return Visitor.Parents;
    }

  private:
    typedef RecursiveASTVisitor<ParentMapASTVisitor> VisitorBase;

    ParentMapASTVisitor(ParentMap *Parents) : Parents(Parents) {
    }

    bool shouldVisitTemplateInstantiations() const {
      return true;
    }
    bool shouldVisitImplicitCode() const {
      return true;
    }
    // Disables data recursion. We intercept Traverse* methods in the RAV, which
    // are not triggered during data recursion.
    bool shouldUseDataRecursionFor(clang::Stmt *S) const {
      return false;
    }

    template <typename T>
    bool TraverseNode(T *Node, bool(VisitorBase:: *traverse) (T *)) {
      if (Node == NULL)
        return true;
      if (ParentStack.size() > 0)
        // FIXME: Currently we add the same parent multiple times, for example
        // when we visit all subexpressions of template instantiations; this is
        // suboptimal, bug benign: the only way to visit those is with
        // hasAncestor / hasParent, and those do not create new matches.
        // The plan is to enable DynTypedNode to be storable in a map or hash
        // map. The main problem there is to implement hash functions /
        // comparison operators for all types that DynTypedNode supports that
        // do not have pointer identity.
        (*Parents)[Node].push_back(ParentStack.back());
      ParentStack.push_back(ast_type_traits::DynTypedNode::create(*Node));
      bool Result = (this ->* traverse) (Node);
      ParentStack.pop_back();
      return Result;
    }

    bool TraverseDecl(Decl *DeclNode) {
      return TraverseNode(DeclNode, &VisitorBase::TraverseDecl);
    }

    bool TraverseStmt(Stmt *StmtNode) {
      return TraverseNode(StmtNode, &VisitorBase::TraverseStmt);
    }

    ParentMap *Parents;
    llvm::SmallVector<ast_type_traits::DynTypedNode, 16> ParentStack;

    friend class RecursiveASTVisitor<ParentMapASTVisitor>;
  };

  llvm::OwningPtr<ParentMap> AllParents;
};

/// \brief Utility function for constructing a nullary selector.
static inline Selector GetNullarySelector(StringRef name, ASTContext& Ctx) {
  IdentifierInfo* II = &Ctx.Idents.get(name);
  return Ctx.Selectors.getSelector(0, &II);
}

/// \brief Utility function for constructing an unary selector.
static inline Selector GetUnarySelector(StringRef name, ASTContext& Ctx) {
  IdentifierInfo* II = &Ctx.Idents.get(name);
  return Ctx.Selectors.getSelector(1, &II);
}

}  // end namespace clang

// operator new and delete aren't allowed inside namespaces.

/// @brief Placement new for using the ASTContext's allocator.
///
/// This placement form of operator new uses the ASTContext's allocator for
/// obtaining memory.
///
/// IMPORTANT: These are also declared in clang/AST/AttrIterator.h! Any changes
/// here need to also be made there.
///
/// We intentionally avoid using a nothrow specification here so that the calls
/// to this operator will not perform a null check on the result -- the
/// underlying allocator never returns null pointers.
///
/// Usage looks like this (assuming there's an ASTContext 'Context' in scope):
/// @code
/// // Default alignment (8)
/// IntegerLiteral *Ex = new (Context) IntegerLiteral(arguments);
/// // Specific alignment
/// IntegerLiteral *Ex2 = new (Context, 4) IntegerLiteral(arguments);
/// @endcode
/// Please note that you cannot use delete on the pointer; it must be
/// deallocated using an explicit destructor call followed by
/// @c Context.Deallocate(Ptr).
///
/// @param Bytes The number of bytes to allocate. Calculated by the compiler.
/// @param C The ASTContext that provides the allocator.
/// @param Alignment The alignment of the allocated memory (if the underlying
///                  allocator supports it).
/// @return The allocated memory. Could be NULL.
inline void *operator new(size_t Bytes, const clang::ASTContext &C,
                          size_t Alignment) {
  return C.Allocate(Bytes, Alignment);
}
/// @brief Placement delete companion to the new above.
///
/// This operator is just a companion to the new above. There is no way of
/// invoking it directly; see the new operator for more details. This operator
/// is called implicitly by the compiler if a placement new expression using
/// the ASTContext throws in the object constructor.
inline void operator delete(void *Ptr, const clang::ASTContext &C, size_t) {
  C.Deallocate(Ptr);
}

/// This placement form of operator new[] uses the ASTContext's allocator for
/// obtaining memory.
///
/// We intentionally avoid using a nothrow specification here so that the calls
/// to this operator will not perform a null check on the result -- the
/// underlying allocator never returns null pointers.
///
/// Usage looks like this (assuming there's an ASTContext 'Context' in scope):
/// @code
/// // Default alignment (8)
/// char *data = new (Context) char[10];
/// // Specific alignment
/// char *data = new (Context, 4) char[10];
/// @endcode
/// Please note that you cannot use delete on the pointer; it must be
/// deallocated using an explicit destructor call followed by
/// @c Context.Deallocate(Ptr).
///
/// @param Bytes The number of bytes to allocate. Calculated by the compiler.
/// @param C The ASTContext that provides the allocator.
/// @param Alignment The alignment of the allocated memory (if the underlying
///                  allocator supports it).
/// @return The allocated memory. Could be NULL.
inline void *operator new[](size_t Bytes, const clang::ASTContext& C,
                            size_t Alignment = 8) {
  return C.Allocate(Bytes, Alignment);
}

/// @brief Placement delete[] companion to the new[] above.
///
/// This operator is just a companion to the new[] above. There is no way of
/// invoking it directly; see the new[] operator for more details. This operator
/// is called implicitly by the compiler if a placement new[] expression using
/// the ASTContext throws in the object constructor.
inline void operator delete[](void *Ptr, const clang::ASTContext &C, size_t) {
  C.Deallocate(Ptr);
}

#endif
