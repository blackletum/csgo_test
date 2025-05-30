﻿//===-- llvm/InlineAsm.h - Class to represent inline asm strings-*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class represents the inline asm strings, which are Value*'s that are
// used as the callee operand of call instructions.  InlineAsm's are uniqued
// like constants, and created via InlineAsm::get(...).
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_IR_INLINEASM_H
#define LLVM_IR_INLINEASM_H

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Value.h"
#include <vector>

namespace llvm {

class PointerType;
class FunctionType;
class Module;
struct InlineAsmKeyType;
template<class ValType, class ValRefType, class TypeClass, class ConstantClass,
         bool HasLargeKey>
class ConstantUniqueMap;
template<class ConstantClass, class TypeClass, class ValType>
struct ConstantCreator;

class InlineAsm : public Value {
public:
  enum AsmDialect {
    AD_ATT,
    AD_Intel
  };

private:
  friend struct ConstantCreator<InlineAsm, PointerType, InlineAsmKeyType>;
  friend class ConstantUniqueMap<InlineAsmKeyType, const InlineAsmKeyType&,
                                 PointerType, InlineAsm, false>;

  InlineAsm(const InlineAsm &) LLVM_DELETED_FUNCTION;
  void operator=(const InlineAsm&) LLVM_DELETED_FUNCTION;

  std::string AsmString, Constraints;
  bool HasSideEffects;
  bool IsAlignStack;
  AsmDialect Dialect;

  InlineAsm(PointerType *Ty, const std::string &AsmString,
            const std::string &Constraints, bool hasSideEffects,
            bool isAlignStack, AsmDialect asmDialect);
  virtual ~InlineAsm();

  /// When the ConstantUniqueMap merges two types and makes two InlineAsms
  /// identical, it destroys one of them with this method.
  void destroyConstant();
public:

  /// InlineAsm::get - Return the specified uniqued inline asm string.
  ///
  static InlineAsm *get(FunctionType *Ty, StringRef AsmString,
                        StringRef Constraints, bool hasSideEffects,
                        bool isAlignStack = false,
                        AsmDialect asmDialect = AD_ATT);
  
  bool hasSideEffects() const { return HasSideEffects; }
  bool isAlignStack() const { return IsAlignStack; }
  AsmDialect getDialect() const { return Dialect; }

  /// getType - InlineAsm's are always pointers.
  ///
  PointerType *getType() const {
    return reinterpret_cast<PointerType*>(Value::getType());
  }
  
  /// getFunctionType - InlineAsm's are always pointers to functions.
  ///
  FunctionType *getFunctionType() const;
  
  const std::string &getAsmString() const { return AsmString; }
  const std::string &getConstraintString() const { return Constraints; }

  /// Verify - This static method can be used by the parser to check to see if
  /// the specified constraint string is legal for the type.  This returns true
  /// if legal, false if not.
  ///
  static bool Verify(FunctionType *Ty, StringRef Constraints);

  // Constraint String Parsing 
  enum ConstraintPrefix {
    isInput,            // 'x'
    isOutput,           // '=x'
    isClobber           // '~x'
  };
  
  typedef std::vector<std::string> ConstraintCodeVector;
  
  struct SubConstraintInfo {
    /// MatchingInput - If this is not -1, this is an output constraint where an
    /// input constraint is required to match it (e.g. "0").  The value is the
    /// constraint number that matches this one (for example, if this is
    /// constraint #0 and constraint #4 has the value "0", this will be 4).
    signed char MatchingInput;
    /// Code - The constraint code, either the register name (in braces) or the
    /// constraint letter/number.
    ConstraintCodeVector Codes;
    /// Default constructor.
    SubConstraintInfo() : MatchingInput(-1) {}
  };

  typedef std::vector<SubConstraintInfo> SubConstraintInfoVector;
  struct ConstraintInfo;
  typedef std::vector<ConstraintInfo> ConstraintInfoVector;
  
  struct ConstraintInfo {
    /// Type - The basic type of the constraint: input/output/clobber
    ///
    ConstraintPrefix Type;
    
    /// isEarlyClobber - "&": output operand writes result before inputs are all
    /// read.  This is only ever set for an output operand.
    bool isEarlyClobber; 
    
    /// MatchingInput - If this is not -1, this is an output constraint where an
    /// input constraint is required to match it (e.g. "0").  The value is the
    /// constraint number that matches this one (for example, if this is
    /// constraint #0 and constraint #4 has the value "0", this will be 4).
    signed char MatchingInput;
    
    /// hasMatchingInput - Return true if this is an output constraint that has
    /// a matching input constraint.
    bool hasMatchingInput() const { return MatchingInput != -1; }
    
    /// isCommutative - This is set to true for a constraint that is commutative
    /// with the next operand.
    bool isCommutative;
    
    /// isIndirect - True if this operand is an indirect operand.  This means
    /// that the address of the source or destination is present in the call
    /// instruction, instead of it being returned or passed in explicitly.  This
    /// is represented with a '*' in the asm string.
    bool isIndirect;
    
    /// Code - The constraint code, either the register name (in braces) or the
    /// constraint letter/number.
    ConstraintCodeVector Codes;
    
    /// isMultipleAlternative - '|': has multiple-alternative constraints.
    bool isMultipleAlternative;
    
    /// multipleAlternatives - If there are multiple alternative constraints,
    /// this array will contain them.  Otherwise it will be empty.
    SubConstraintInfoVector multipleAlternatives;
    
    /// The currently selected alternative constraint index.
    unsigned currentAlternativeIndex;
    
    ///Default constructor.
    ConstraintInfo();
    
    /// Copy constructor.
    ConstraintInfo(const ConstraintInfo &other);
    
    /// Parse - Analyze the specified string (e.g. "=*&{eax}") and fill in the
    /// fields in this structure.  If the constraint string is not understood,
    /// return true, otherwise return false.
    bool Parse(StringRef Str, ConstraintInfoVector &ConstraintsSoFar);
               
    /// selectAlternative - Point this constraint to the alternative constraint
    /// indicated by the index.
    void selectAlternative(unsigned index);
  };
  
  /// ParseConstraints - Split up the constraint string into the specific
  /// constraints and their prefixes.  If this returns an empty vector, and if
  /// the constraint string itself isn't empty, there was an error parsing.
  static ConstraintInfoVector ParseConstraints(StringRef ConstraintString);
  
  /// ParseConstraints - Parse the constraints of this inlineasm object, 
  /// returning them the same way that ParseConstraints(str) does.
  ConstraintInfoVector ParseConstraints() const {
    return ParseConstraints(Constraints);
  }
  
  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Value *V) {
    return V->getValueID() == Value::InlineAsmVal;
  }

  
  // These are helper methods for dealing with flags in the INLINEASM SDNode
  // in the backend.
  
  enum {
    // Fixed operands on an INLINEASM SDNode.
    Op_InputChain = 0,
    Op_AsmString = 1,
    Op_MDNode = 2,
    Op_ExtraInfo = 3,    // HasSideEffects, IsAlignStack, AsmDialect.
    Op_FirstOperand = 4,

    // Fixed operands on an INLINEASM MachineInstr.
    MIOp_AsmString = 0,
    MIOp_ExtraInfo = 1,    // HasSideEffects, IsAlignStack, AsmDialect.
    MIOp_FirstOperand = 2,

    // Interpretation of the MIOp_ExtraInfo bit field.
    Extra_HasSideEffects = 1,
    Extra_IsAlignStack = 2,
    Extra_AsmDialect = 4,
    Extra_MayLoad = 8,
    Extra_MayStore = 16,

    // Inline asm operands map to multiple SDNode / MachineInstr operands.
    // The first operand is an immediate describing the asm operand, the low
    // bits is the kind:
    Kind_RegUse = 1,             // Input register, "r".
    Kind_RegDef = 2,             // Output register, "=r".
    Kind_RegDefEarlyClobber = 3, // Early-clobber output register, "=&r".
    Kind_Clobber = 4,            // Clobbered register, "~r".
    Kind_Imm = 5,                // Immediate.
    Kind_Mem = 6,                // Memory operand, "m".

    Flag_MatchingOperand = 0x80000000
  };
  
  static unsigned getFlagWord(unsigned Kind, unsigned NumOps) {
    assert(((NumOps << 3) & ~0xffff) == 0 && "Too many inline asm operands!");
    assert(Kind >= Kind_RegUse && Kind <= Kind_Mem && "Invalid Kind");
    return Kind | (NumOps << 3);
  }
  
  /// getFlagWordForMatchingOp - Augment an existing flag word returned by
  /// getFlagWord with information indicating that this input operand is tied 
  /// to a previous output operand.
  static unsigned getFlagWordForMatchingOp(unsigned InputFlag,
                                           unsigned MatchedOperandNo) {
    assert(MatchedOperandNo <= 0x7fff && "Too big matched operand");
    assert((InputFlag & ~0xffff) == 0 && "High bits already contain data");
    return InputFlag | Flag_MatchingOperand | (MatchedOperandNo << 16);
  }

  /// getFlagWordForRegClass - Augment an existing flag word returned by
  /// getFlagWord with the required register class for the following register
  /// operands.
  /// A tied use operand cannot have a register class, use the register class
  /// from the def operand instead.
  static unsigned getFlagWordForRegClass(unsigned InputFlag, unsigned RC) {
    // Store RC + 1, reserve the value 0 to mean 'no register class'.
    ++RC;
    assert(RC <= 0x7fff && "Too large register class ID");
    assert((InputFlag & ~0xffff) == 0 && "High bits already contain data");
    return InputFlag | (RC << 16);
  }

  static unsigned getKind(unsigned Flags) {
    return Flags & 7;
  }

  static bool isRegDefKind(unsigned Flag){ return getKind(Flag) == Kind_RegDef;}
  static bool isImmKind(unsigned Flag) { return getKind(Flag) == Kind_Imm; }
  static bool isMemKind(unsigned Flag) { return getKind(Flag) == Kind_Mem; }
  static bool isRegDefEarlyClobberKind(unsigned Flag) {
    return getKind(Flag) == Kind_RegDefEarlyClobber;
  }
  static bool isClobberKind(unsigned Flag) {
    return getKind(Flag) == Kind_Clobber;
  }

  /// getNumOperandRegisters - Extract the number of registers field from the
  /// inline asm operand flag.
  static unsigned getNumOperandRegisters(unsigned Flag) {
    return (Flag & 0xffff) >> 3;
  }

  /// isUseOperandTiedToDef - Return true if the flag of the inline asm
  /// operand indicates it is an use operand that's matched to a def operand.
  static bool isUseOperandTiedToDef(unsigned Flag, unsigned &Idx) {
    if ((Flag & Flag_MatchingOperand) == 0)
      return false;
    Idx = (Flag & ~Flag_MatchingOperand) >> 16;
    return true;
  }

  /// hasRegClassConstraint - Returns true if the flag contains a register
  /// class constraint.  Sets RC to the register class ID.
  static bool hasRegClassConstraint(unsigned Flag, unsigned &RC) {
    if (Flag & Flag_MatchingOperand)
      return false;
    unsigned High = Flag >> 16;
    // getFlagWordForRegClass() uses 0 to mean no register class, and otherwise
    // stores RC + 1.
    if (!High)
      return false;
    RC = High - 1;
    return true;
  }

};

} // End llvm namespace

#endif
