﻿//===- LLVMBitCodes.h - Enum values for the LLVM bitcode format -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This header defines Bitcode enum values for LLVM IR bitcode files.
//
// The enum values defined in this file should be considered permanent.  If
// new features are added, they should have values added at the end of the
// respective lists.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_BITCODE_LLVMBITCODES_H
#define LLVM_BITCODE_LLVMBITCODES_H

#include "llvm/Bitcode/BitCodes.h"

namespace llvm {
namespace bitc {
  // The only top-level block type defined is for a module.
  enum BlockIDs {
    // Blocks
    MODULE_BLOCK_ID          = FIRST_APPLICATION_BLOCKID,

    // Module sub-block id's.
    PARAMATTR_BLOCK_ID,
    PARAMATTR_GROUP_BLOCK_ID,

    CONSTANTS_BLOCK_ID,
    FUNCTION_BLOCK_ID,

    UNUSED_ID1,

    VALUE_SYMTAB_BLOCK_ID,
    METADATA_BLOCK_ID,
    METADATA_ATTACHMENT_ID,

    TYPE_BLOCK_ID_NEW,

    USELIST_BLOCK_ID
  };


  /// MODULE blocks have a number of optional fields and subblocks.
  enum ModuleCodes {
    MODULE_CODE_VERSION     = 1,    // VERSION:     [version#]
    MODULE_CODE_TRIPLE      = 2,    // TRIPLE:      [strchr x N]
    MODULE_CODE_DATALAYOUT  = 3,    // DATALAYOUT:  [strchr x N]
    MODULE_CODE_ASM         = 4,    // ASM:         [strchr x N]
    MODULE_CODE_SECTIONNAME = 5,    // SECTIONNAME: [strchr x N]

    // FIXME: Remove DEPLIB in 4.0.
    MODULE_CODE_DEPLIB      = 6,    // DEPLIB:      [strchr x N]

    // GLOBALVAR: [pointer type, isconst, initid,
    //             linkage, alignment, section, visibility, threadlocal]
    MODULE_CODE_GLOBALVAR   = 7,

    // FUNCTION:  [type, callingconv, isproto, linkage, paramattrs, alignment,
    //             section, visibility, gc, unnamed_addr]
    MODULE_CODE_FUNCTION    = 8,

    // ALIAS: [alias type, aliasee val#, linkage, visibility]
    MODULE_CODE_ALIAS       = 9,

    // MODULE_CODE_PURGEVALS: [numvals]
    MODULE_CODE_PURGEVALS   = 10,

    MODULE_CODE_GCNAME      = 11   // GCNAME: [strchr x N]
  };

  /// PARAMATTR blocks have code for defining a parameter attribute set.
  enum AttributeCodes {
    // FIXME: Remove `PARAMATTR_CODE_ENTRY_OLD' in 4.0
    PARAMATTR_CODE_ENTRY_OLD  = 1, // ENTRY: [paramidx0, attr0,
                                   //         paramidx1, attr1...]
    PARAMATTR_CODE_ENTRY      = 2, // ENTRY: [paramidx0, attrgrp0,
                                   //         paramidx1, attrgrp1, ...]
    PARAMATTR_GRP_CODE_ENTRY  = 3  // ENTRY: [id, attr0, att1, ...]
  };

  /// TYPE blocks have codes for each type primitive they use.
  enum TypeCodes {
    TYPE_CODE_NUMENTRY =  1,    // NUMENTRY: [numentries]

    // Type Codes
    TYPE_CODE_VOID     =  2,    // VOID
    TYPE_CODE_FLOAT    =  3,    // FLOAT
    TYPE_CODE_DOUBLE   =  4,    // DOUBLE
    TYPE_CODE_LABEL    =  5,    // LABEL
    TYPE_CODE_OPAQUE   =  6,    // OPAQUE
    TYPE_CODE_INTEGER  =  7,    // INTEGER: [width]
    TYPE_CODE_POINTER  =  8,    // POINTER: [pointee type]

    TYPE_CODE_FUNCTION_OLD = 9, // FUNCTION: [vararg, attrid, retty,
                                //            paramty x N]

    TYPE_CODE_HALF     =  10,   // HALF

    TYPE_CODE_ARRAY    = 11,    // ARRAY: [numelts, eltty]
    TYPE_CODE_VECTOR   = 12,    // VECTOR: [numelts, eltty]

    // These are not with the other floating point types because they're
    // a late addition, and putting them in the right place breaks
    // binary compatibility.
    TYPE_CODE_X86_FP80 = 13,    // X86 LONG DOUBLE
    TYPE_CODE_FP128    = 14,    // LONG DOUBLE (112 bit mantissa)
    TYPE_CODE_PPC_FP128= 15,    // PPC LONG DOUBLE (2 doubles)

    TYPE_CODE_METADATA = 16,    // METADATA

    TYPE_CODE_X86_MMX = 17,     // X86 MMX

    TYPE_CODE_STRUCT_ANON = 18, // STRUCT_ANON: [ispacked, eltty x N]
    TYPE_CODE_STRUCT_NAME = 19, // STRUCT_NAME: [strchr x N]
    TYPE_CODE_STRUCT_NAMED = 20,// STRUCT_NAMED: [ispacked, eltty x N]

    TYPE_CODE_FUNCTION = 21     // FUNCTION: [vararg, retty, paramty x N]
  };

  // The type symbol table only has one code (TST_ENTRY_CODE).
  enum TypeSymtabCodes {
    TST_CODE_ENTRY = 1     // TST_ENTRY: [typeid, namechar x N]
  };

  // The value symbol table only has one code (VST_ENTRY_CODE).
  enum ValueSymtabCodes {
    VST_CODE_ENTRY   = 1,  // VST_ENTRY: [valid, namechar x N]
    VST_CODE_BBENTRY = 2   // VST_BBENTRY: [bbid, namechar x N]
  };

  enum MetadataCodes {
    METADATA_STRING        = 1,   // MDSTRING:      [values]
    // 2 is unused.
    // 3 is unused.
    METADATA_NAME          = 4,   // STRING:        [values]
    // 5 is unused.
    METADATA_KIND          = 6,   // [n x [id, name]]
    // 7 is unused.
    METADATA_NODE          = 8,   // NODE:          [n x (type num, value num)]
    METADATA_FN_NODE       = 9,   // FN_NODE:       [n x (type num, value num)]
    METADATA_NAMED_NODE    = 10,  // NAMED_NODE:    [n x mdnodes]
    METADATA_ATTACHMENT    = 11   // [m x [value, [n x [id, mdnode]]]
  };

  // The constants block (CONSTANTS_BLOCK_ID) describes emission for each
  // constant and maintains an implicit current type value.
  enum ConstantsCodes {
    CST_CODE_SETTYPE       =  1,  // SETTYPE:       [typeid]
    CST_CODE_NULL          =  2,  // NULL
    CST_CODE_UNDEF         =  3,  // UNDEF
    CST_CODE_INTEGER       =  4,  // INTEGER:       [intval]
    CST_CODE_WIDE_INTEGER  =  5,  // WIDE_INTEGER:  [n x intval]
    CST_CODE_FLOAT         =  6,  // FLOAT:         [fpval]
    CST_CODE_AGGREGATE     =  7,  // AGGREGATE:     [n x value number]
    CST_CODE_STRING        =  8,  // STRING:        [values]
    CST_CODE_CSTRING       =  9,  // CSTRING:       [values]
    CST_CODE_CE_BINOP      = 10,  // CE_BINOP:      [opcode, opval, opval]
    CST_CODE_CE_CAST       = 11,  // CE_CAST:       [opcode, opty, opval]
    CST_CODE_CE_GEP        = 12,  // CE_GEP:        [n x operands]
    CST_CODE_CE_SELECT     = 13,  // CE_SELECT:     [opval, opval, opval]
    CST_CODE_CE_EXTRACTELT = 14,  // CE_EXTRACTELT: [opty, opval, opval]
    CST_CODE_CE_INSERTELT  = 15,  // CE_INSERTELT:  [opval, opval, opval]
    CST_CODE_CE_SHUFFLEVEC = 16,  // CE_SHUFFLEVEC: [opval, opval, opval]
    CST_CODE_CE_CMP        = 17,  // CE_CMP:        [opty, opval, opval, pred]
    CST_CODE_INLINEASM_OLD = 18,  // INLINEASM:     [sideeffect|alignstack,
                                  //                 asmstr,conststr]
    CST_CODE_CE_SHUFVEC_EX = 19,  // SHUFVEC_EX:    [opty, opval, opval, opval]
    CST_CODE_CE_INBOUNDS_GEP = 20,// INBOUNDS_GEP:  [n x operands]
    CST_CODE_BLOCKADDRESS  = 21,  // CST_CODE_BLOCKADDRESS [fnty, fnval, bb#]
    CST_CODE_DATA          = 22,  // DATA:          [n x elements]
    CST_CODE_INLINEASM     = 23   // INLINEASM:     [sideeffect|alignstack|
                                  //                 asmdialect,asmstr,conststr]
  };

  /// CastOpcodes - These are values used in the bitcode files to encode which
  /// cast a CST_CODE_CE_CAST or a XXX refers to.  The values of these enums
  /// have no fixed relation to the LLVM IR enum values.  Changing these will
  /// break compatibility with old files.
  enum CastOpcodes {
    CAST_TRUNC    =  0,
    CAST_ZEXT     =  1,
    CAST_SEXT     =  2,
    CAST_FPTOUI   =  3,
    CAST_FPTOSI   =  4,
    CAST_UITOFP   =  5,
    CAST_SITOFP   =  6,
    CAST_FPTRUNC  =  7,
    CAST_FPEXT    =  8,
    CAST_PTRTOINT =  9,
    CAST_INTTOPTR = 10,
    CAST_BITCAST  = 11
  };

  /// BinaryOpcodes - These are values used in the bitcode files to encode which
  /// binop a CST_CODE_CE_BINOP or a XXX refers to.  The values of these enums
  /// have no fixed relation to the LLVM IR enum values.  Changing these will
  /// break compatibility with old files.
  enum BinaryOpcodes {
    BINOP_ADD  =  0,
    BINOP_SUB  =  1,
    BINOP_MUL  =  2,
    BINOP_UDIV =  3,
    BINOP_SDIV =  4,    // overloaded for FP
    BINOP_UREM =  5,
    BINOP_SREM =  6,    // overloaded for FP
    BINOP_SHL  =  7,
    BINOP_LSHR =  8,
    BINOP_ASHR =  9,
    BINOP_AND  = 10,
    BINOP_OR   = 11,
    BINOP_XOR  = 12
  };

  /// These are values used in the bitcode files to encode AtomicRMW operations.
  /// The values of these enums have no fixed relation to the LLVM IR enum
  /// values.  Changing these will break compatibility with old files.
  enum RMWOperations {
    RMW_XCHG = 0,
    RMW_ADD = 1,
    RMW_SUB = 2,
    RMW_AND = 3,
    RMW_NAND = 4,
    RMW_OR = 5,
    RMW_XOR = 6,
    RMW_MAX = 7,
    RMW_MIN = 8,
    RMW_UMAX = 9,
    RMW_UMIN = 10
  };

  /// OverflowingBinaryOperatorOptionalFlags - Flags for serializing
  /// OverflowingBinaryOperator's SubclassOptionalData contents.
  enum OverflowingBinaryOperatorOptionalFlags {
    OBO_NO_UNSIGNED_WRAP = 0,
    OBO_NO_SIGNED_WRAP = 1
  };

  /// PossiblyExactOperatorOptionalFlags - Flags for serializing
  /// PossiblyExactOperator's SubclassOptionalData contents.
  enum PossiblyExactOperatorOptionalFlags {
    PEO_EXACT = 0
  };

  /// Encoded AtomicOrdering values.
  enum AtomicOrderingCodes {
    ORDERING_NOTATOMIC = 0,
    ORDERING_UNORDERED = 1,
    ORDERING_MONOTONIC = 2,
    ORDERING_ACQUIRE = 3,
    ORDERING_RELEASE = 4,
    ORDERING_ACQREL = 5,
    ORDERING_SEQCST = 6
  };

  /// Encoded SynchronizationScope values.
  enum AtomicSynchScopeCodes {
    SYNCHSCOPE_SINGLETHREAD = 0,
    SYNCHSCOPE_CROSSTHREAD = 1
  };

  // The function body block (FUNCTION_BLOCK_ID) describes function bodies.  It
  // can contain a constant block (CONSTANTS_BLOCK_ID).
  enum FunctionCodes {
    FUNC_CODE_DECLAREBLOCKS    =  1, // DECLAREBLOCKS: [n]

    FUNC_CODE_INST_BINOP       =  2, // BINOP:      [opcode, ty, opval, opval]
    FUNC_CODE_INST_CAST        =  3, // CAST:       [opcode, ty, opty, opval]
    FUNC_CODE_INST_GEP         =  4, // GEP:        [n x operands]
    FUNC_CODE_INST_SELECT      =  5, // SELECT:     [ty, opval, opval, opval]
    FUNC_CODE_INST_EXTRACTELT  =  6, // EXTRACTELT: [opty, opval, opval]
    FUNC_CODE_INST_INSERTELT   =  7, // INSERTELT:  [ty, opval, opval, opval]
    FUNC_CODE_INST_SHUFFLEVEC  =  8, // SHUFFLEVEC: [ty, opval, opval, opval]
    FUNC_CODE_INST_CMP         =  9, // CMP:        [opty, opval, opval, pred]

    FUNC_CODE_INST_RET         = 10, // RET:        [opty,opval<both optional>]
    FUNC_CODE_INST_BR          = 11, // BR:         [bb#, bb#, cond] or [bb#]
    FUNC_CODE_INST_SWITCH      = 12, // SWITCH:     [opty, op0, op1, ...]
    FUNC_CODE_INST_INVOKE      = 13, // INVOKE:     [attr, fnty, op0,op1, ...]
    // 14 is unused.
    FUNC_CODE_INST_UNREACHABLE = 15, // UNREACHABLE

    FUNC_CODE_INST_PHI         = 16, // PHI:        [ty, val0,bb0, ...]
    // 17 is unused.
    // 18 is unused.
    FUNC_CODE_INST_ALLOCA      = 19, // ALLOCA:     [instty, op, align]
    FUNC_CODE_INST_LOAD        = 20, // LOAD:       [opty, op, align, vol]
    // 21 is unused.
    // 22 is unused.
    FUNC_CODE_INST_VAARG       = 23, // VAARG:      [valistty, valist, instty]
    // This store code encodes the pointer type, rather than the value type
    // this is so information only available in the pointer type (e.g. address
    // spaces) is retained.
    FUNC_CODE_INST_STORE       = 24, // STORE:      [ptrty,ptr,val, align, vol]
    // 25 is unused.
    FUNC_CODE_INST_EXTRACTVAL  = 26, // EXTRACTVAL: [n x operands]
    FUNC_CODE_INST_INSERTVAL   = 27, // INSERTVAL:  [n x operands]
    // fcmp/icmp returning Int1TY or vector of Int1Ty. Same as CMP, exists to
    // support legacy vicmp/vfcmp instructions.
    FUNC_CODE_INST_CMP2        = 28, // CMP2:       [opty, opval, opval, pred]
    // new select on i1 or [N x i1]
    FUNC_CODE_INST_VSELECT     = 29, // VSELECT:    [ty,opval,opval,predty,pred]
    FUNC_CODE_INST_INBOUNDS_GEP= 30, // INBOUNDS_GEP: [n x operands]
    FUNC_CODE_INST_INDIRECTBR  = 31, // INDIRECTBR: [opty, op0, op1, ...]
    // 32 is unused.
    FUNC_CODE_DEBUG_LOC_AGAIN  = 33, // DEBUG_LOC_AGAIN

    FUNC_CODE_INST_CALL        = 34, // CALL:       [attr, fnty, fnid, args...]

    FUNC_CODE_DEBUG_LOC        = 35, // DEBUG_LOC:  [Line,Col,ScopeVal, IAVal]
    FUNC_CODE_INST_FENCE       = 36, // FENCE: [ordering, synchscope]
    FUNC_CODE_INST_CMPXCHG     = 37, // CMPXCHG: [ptrty,ptr,cmp,new, align, vol,
                                     //           ordering, synchscope]
    FUNC_CODE_INST_ATOMICRMW   = 38, // ATOMICRMW: [ptrty,ptr,val, operation,
                                     //             align, vol,
                                     //             ordering, synchscope]
    FUNC_CODE_INST_RESUME      = 39, // RESUME:     [opval]
    FUNC_CODE_INST_LANDINGPAD  = 40, // LANDINGPAD: [ty,val,val,num,id0,val0...]
    FUNC_CODE_INST_LOADATOMIC  = 41, // LOAD: [opty, op, align, vol,
                                     //        ordering, synchscope]
    FUNC_CODE_INST_STOREATOMIC = 42  // STORE: [ptrty,ptr,val, align, vol
                                     //         ordering, synchscope]
  };

  enum UseListCodes {
    USELIST_CODE_ENTRY = 1   // USELIST_CODE_ENTRY: TBD.
  };
} // End bitc namespace
} // End llvm namespace

#endif
