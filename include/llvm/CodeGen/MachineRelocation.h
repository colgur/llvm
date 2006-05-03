//===-- llvm/CodeGen/MachineRelocation.h - Target Relocation ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the MachineRelocation class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CODEGEN_MACHINERELOCATION_H
#define LLVM_CODEGEN_MACHINERELOCATION_H

#include "llvm/Support/DataTypes.h"
#include <cassert>

namespace llvm {
class GlobalValue;

/// MachineRelocation - This represents a target-specific relocation value,
/// produced by the code emitter.  This relocation is resolved after the has
/// been emitted, either to an object file or to memory, when the target of the
/// relocation can be resolved.
///
/// A relocation is made up of the following logical portions:
///   1. An offset in the machine code buffer, the location to modify.
///   2. A target specific relocation type (a number from 0 to 63).
///   3. A symbol being referenced, either as a GlobalValue* or as a string.
///   4. An optional constant value to be added to the reference.
///   5. A bit, CanRewrite, which indicates to the JIT that a function stub is
///      not needed for the relocation.
///   6. An index into the GOT, if the target uses a GOT
///
class MachineRelocation {
  enum AddressType {
    isResult,         // Relocation has be transformed into its result pointer.
    isGV,             // The Target.GV field is valid.
    isExtSym,         // The Target.ExtSym field is valid.
    isConstPool,      // The Target.ConstPool field is valid.
    isGOTIndex        // The Target.GOTIndex field is valid.
  };
  
  /// Offset - This is the offset from the start of the code buffer of the
  /// relocation to perform.
  unsigned Offset;
  
  /// ConstantVal - A field that may be used by the target relocation type.
  intptr_t ConstantVal;

  union {
    void *Result;        // If this has been resolved to a resolved pointer
    GlobalValue *GV;     // If this is a pointer to an LLVM global
    const char *ExtSym;  // If this is a pointer to a named symbol
    unsigned ConstPool;  // In this is a pointer to a constant pool entry
    unsigned GOTIndex;   // Index in the GOT of this symbol/global
  } Target;

  unsigned TargetReloType : 6; // The target relocation ID.
  AddressType AddrType    : 3; // The field of Target to use.
  bool DoesntNeedFnStub   : 1; // True if we don't need a fn stub.
  bool GOTRelative        : 1; // Should this relocation be relative to the GOT?

public:
  MachineRelocation(unsigned offset, unsigned RelocationType, GlobalValue *GV,
                    intptr_t cst = 0, bool DoesntNeedFunctionStub = 0,
                    bool GOTrelative = 0)
    : Offset(offset), ConstantVal(cst), TargetReloType(RelocationType),
      AddrType(isGV), DoesntNeedFnStub(DoesntNeedFunctionStub),
      GOTRelative(GOTrelative){
    assert((RelocationType & ~63) == 0 && "Relocation type too large!");
    Target.GV = GV;
  }

  MachineRelocation(unsigned offset, unsigned RelocationType, const char *ES,
                    intptr_t cst = 0, bool GOTrelative = 0)
    : Offset(offset), ConstantVal(cst), TargetReloType(RelocationType),
      AddrType(isExtSym), DoesntNeedFnStub(false), GOTRelative(GOTrelative) {
    assert((RelocationType & ~63) == 0 && "Relocation type too large!");
    Target.ExtSym = ES;
  }

  MachineRelocation(unsigned offset, unsigned RelocationType, unsigned CPI,
                    intptr_t cst = 0)
    : Offset(offset), ConstantVal(cst), TargetReloType(RelocationType),
      AddrType(isConstPool), DoesntNeedFnStub(false), GOTRelative(0) {
    assert((RelocationType & ~63) == 0 && "Relocation type too large!");
    Target.ConstPool = CPI;
  }

  /// getMachineCodeOffset - Return the offset into the code buffer that the
  /// relocation should be performed.
  unsigned getMachineCodeOffset() const {
    return Offset;
  }

  /// getRelocationType - Return the target-specific relocation ID for this
  /// relocation.
  unsigned getRelocationType() const {
    return TargetReloType;
  }

  /// getConstantVal - Get the constant value associated with this relocation.
  /// This is often an offset from the symbol.
  ///
  intptr_t getConstantVal() const {
    return ConstantVal;
  }

  /// isGlobalValue - Return true if this relocation is a GlobalValue, as
  /// opposed to a constant string.
  bool isGlobalValue() const {
    return AddrType == isGV;
  }

  /// isString - Return true if this is a constant string.
  ///
  bool isString() const {
    return AddrType == isExtSym;
  }

  /// isConstantPoolIndex - Return true if this is a constant pool reference.
  ///
  bool isConstantPoolIndex() const {
    return AddrType == isConstPool;
  }

  /// isGOTRelative - Return true the target wants the index into the GOT of
  /// the symbol rather than the address of the symbol.
  bool isGOTRelative() const {
    return GOTRelative;
  }

  /// doesntNeedFunctionStub - This function returns true if the JIT for this
  /// target is capable of directly handling the relocated instruction without
  /// using a stub function.  It is always conservatively correct for this flag
  /// to be false, but targets can improve their compilation callback functions
  /// to handle more general cases if they want improved performance.
  bool doesntNeedFunctionStub() const {
    return DoesntNeedFnStub;
  }

  /// getGlobalValue - If this is a global value reference, return the
  /// referenced global.
  GlobalValue *getGlobalValue() const {
    assert(isGlobalValue() && "This is not a global value reference!");
    return Target.GV;
  }

  /// getString - If this is a string value, return the string reference.
  ///
  const char *getString() const {
    assert(isString() && "This is not a string reference!");
    return Target.ExtSym;
  }

  /// getConstantPoolIndex - If this is a const pool reference, return
  /// the index into the constant pool.
  unsigned getConstantPoolIndex() const {
    assert(isConstantPoolIndex() && "This is not a constant pool reference!");
    return Target.ConstPool;
  }

  /// getResultPointer - Once this has been resolved to point to an actual
  /// address, this returns the pointer.
  void *getResultPointer() const {
    assert(AddrType == isResult && "Result pointer isn't set yet!");
    return Target.Result;
  }

  /// setResultPointer - Set the result to the specified pointer value.
  ///
  void setResultPointer(void *Ptr) {
    Target.Result = Ptr;
    AddrType = isResult;
  }

  /// setGOTIndex - Set the GOT index to a specific value.
  void setGOTIndex(unsigned idx) {
    AddrType = isGOTIndex;
    Target.GOTIndex = idx;
  }

  /// getGOTIndex - Once this has been resolved to an entry in the GOT,
  /// this returns that index.  The index is from the lowest address entry
  /// in the GOT.
  unsigned getGOTIndex() const {
    assert(AddrType == isGOTIndex);
    return Target.GOTIndex;
  }
};
}

#endif
