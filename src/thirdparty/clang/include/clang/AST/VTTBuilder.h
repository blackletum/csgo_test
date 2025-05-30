﻿//===--- VTTBuilder.h - C++ VTT layout builder --------------------*- C++ -*-=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This contains code dealing with generation of the layout of virtual table
// tables (VTT).
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_VTTBUILDER_H
#define LLVM_CLANG_AST_VTTBUILDER_H

#include "clang/AST/BaseSubobject.h"
#include "clang/AST/CXXInheritance.h"
#include "clang/AST/GlobalDecl.h"
#include "clang/AST/RecordLayout.h"
#include "clang/Basic/ABI.h"
#include "llvm/ADT/SetVector.h"
#include <utility>

namespace clang {

class VTTVTable {
  llvm::PointerIntPair<const CXXRecordDecl *, 1, bool> BaseAndIsVirtual;
  CharUnits BaseOffset;

public:
  VTTVTable() {}
  VTTVTable(const CXXRecordDecl *Base, CharUnits BaseOffset, bool BaseIsVirtual)
    : BaseAndIsVirtual(Base, BaseIsVirtual), BaseOffset(BaseOffset) {}
  VTTVTable(BaseSubobject Base, bool BaseIsVirtual)
    : BaseAndIsVirtual(Base.getBase(), BaseIsVirtual),
      BaseOffset(Base.getBaseOffset()) {}

  const CXXRecordDecl *getBase() const {
    return BaseAndIsVirtual.getPointer();
  }

  CharUnits getBaseOffset() const {
    return BaseOffset;
  }

  bool isVirtual() const {
    return BaseAndIsVirtual.getInt();
  }

  BaseSubobject getBaseSubobject() const {
    return BaseSubobject(getBase(), getBaseOffset());
  }
};

struct VTTComponent {
  uint64_t VTableIndex;
  BaseSubobject VTableBase;

  VTTComponent() {}
  VTTComponent(uint64_t VTableIndex, BaseSubobject VTableBase)
    : VTableIndex(VTableIndex), VTableBase(VTableBase) {}
};

/// VTT builder - Class for building VTT layout information.
class VTTBuilder {
  
  ASTContext &Ctx;

  /// MostDerivedClass - The most derived class for which we're building this
  /// vtable.
  const CXXRecordDecl *MostDerivedClass;

  typedef SmallVector<VTTVTable, 64> VTTVTablesVectorTy;
  
  /// VTTVTables - The VTT vtables.
  VTTVTablesVectorTy VTTVTables;
  
  typedef SmallVector<VTTComponent, 64> VTTComponentsVectorTy;
  
  /// VTTComponents - The VTT components.
  VTTComponentsVectorTy VTTComponents;
  
  /// MostDerivedClassLayout - the AST record layout of the most derived class.
  const ASTRecordLayout &MostDerivedClassLayout;

  typedef llvm::SmallPtrSet<const CXXRecordDecl *, 4> VisitedVirtualBasesSetTy;

  typedef llvm::DenseMap<BaseSubobject, uint64_t> AddressPointsMapTy;

  /// SubVTTIndicies - The sub-VTT indices for the bases of the most derived
  /// class.
  llvm::DenseMap<BaseSubobject, uint64_t> SubVTTIndicies;

  /// SecondaryVirtualPointerIndices - The secondary virtual pointer indices of
  /// all subobjects of the most derived class.
  llvm::DenseMap<BaseSubobject, uint64_t> SecondaryVirtualPointerIndices;

  /// GenerateDefinition - Whether the VTT builder should generate LLVM IR for
  /// the VTT.
  bool GenerateDefinition;

  /// AddVTablePointer - Add a vtable pointer to the VTT currently being built.
  void AddVTablePointer(BaseSubobject Base, uint64_t VTableIndex,
                        const CXXRecordDecl *VTableClass);
                        
  /// LayoutSecondaryVTTs - Lay out the secondary VTTs of the given base 
  /// subobject.
  void LayoutSecondaryVTTs(BaseSubobject Base);
  
  /// LayoutSecondaryVirtualPointers - Lay out the secondary virtual pointers
  /// for the given base subobject.
  ///
  /// \param BaseIsMorallyVirtual whether the base subobject is a virtual base
  /// or a direct or indirect base of a virtual base.
  void LayoutSecondaryVirtualPointers(BaseSubobject Base, 
                                      bool BaseIsMorallyVirtual,
                                      uint64_t VTableIndex,
                                      const CXXRecordDecl *VTableClass,
                                      VisitedVirtualBasesSetTy &VBases);
  
  /// LayoutSecondaryVirtualPointers - Lay out the secondary virtual pointers
  /// for the given base subobject.
  void LayoutSecondaryVirtualPointers(BaseSubobject Base, 
                                      uint64_t VTableIndex);

  /// LayoutVirtualVTTs - Lay out the VTTs for the virtual base classes of the
  /// given record decl.
  void LayoutVirtualVTTs(const CXXRecordDecl *RD,
                         VisitedVirtualBasesSetTy &VBases);
  
  /// LayoutVTT - Will lay out the VTT for the given subobject, including any
  /// secondary VTTs, secondary virtual pointers and virtual VTTs.
  void LayoutVTT(BaseSubobject Base, bool BaseIsVirtual);
  
public:
  VTTBuilder(ASTContext &Ctx, const CXXRecordDecl *MostDerivedClass,
             bool GenerateDefinition);

  // getVTTComponents - Returns a reference to the VTT components.
  const VTTComponentsVectorTy &getVTTComponents() const {
    return VTTComponents;
  }
  
  // getVTTVTables - Returns a reference to the VTT vtables.
  const VTTVTablesVectorTy &getVTTVTables() const {
    return VTTVTables;
  }
  
  /// getSubVTTIndicies - Returns a reference to the sub-VTT indices.
  const llvm::DenseMap<BaseSubobject, uint64_t> &getSubVTTIndicies() const {
    return SubVTTIndicies;
  }
  
  /// getSecondaryVirtualPointerIndices - Returns a reference to the secondary
  /// virtual pointer indices.
  const llvm::DenseMap<BaseSubobject, uint64_t> &
  getSecondaryVirtualPointerIndices() const {
    return SecondaryVirtualPointerIndices;
  }

};

}

#endif
