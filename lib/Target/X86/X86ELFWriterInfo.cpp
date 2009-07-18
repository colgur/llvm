//===-- X86ELFWriterInfo.cpp - ELF Writer Info for the X86 backend --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements ELF writer information for the X86 backend.
//
//===----------------------------------------------------------------------===//

#include "X86ELFWriterInfo.h"
#include "X86Relocations.h"
#include "llvm/Function.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

//===----------------------------------------------------------------------===//
//  Implementation of the X86ELFWriterInfo class
//===----------------------------------------------------------------------===//

X86ELFWriterInfo::X86ELFWriterInfo(TargetMachine &TM)
  : TargetELFWriterInfo(TM) {
    bool is64Bit = TM.getTargetData()->getPointerSizeInBits() == 64;
    EMachine = is64Bit ? EM_X86_64 : EM_386;
  }

X86ELFWriterInfo::~X86ELFWriterInfo() {}

unsigned X86ELFWriterInfo::getRelocationType(unsigned MachineRelTy) const {
  if (is64Bit) {
    switch(MachineRelTy) {
    case X86::reloc_pcrel_word:
      return R_X86_64_PC32;
    case X86::reloc_absolute_word:
      return R_X86_64_32;
    case X86::reloc_absolute_dword:
      return R_X86_64_64;
    case X86::reloc_picrel_word:
    default:
      llvm_unreachable("unknown x86_64 machine relocation type");
    }
  } else {
    switch(MachineRelTy) {
    case X86::reloc_pcrel_word:
      return R_386_PC32;
    case X86::reloc_absolute_word:
      return R_386_32;
    case X86::reloc_absolute_dword:
    case X86::reloc_picrel_word:
    default:
      llvm_unreachable("unknown x86 machine relocation type");
    }
  }
  return 0;
}

long int X86ELFWriterInfo::getDefaultAddendForRelTy(unsigned RelTy) const {
  if (is64Bit) {
    switch(RelTy) {
    case R_X86_64_PC32: return -4;
    case R_X86_64_32: return 0;
    default:
      llvm_unreachable("unknown x86_64 relocation type");
    }
  } else {
    switch(RelTy) {
      case R_386_PC32: return -4;
      case R_386_32: return 0;
    default:
      llvm_unreachable("unknown x86 relocation type");
    }
  }
  return 0;
}

unsigned X86ELFWriterInfo::getRelocationTySize(unsigned RelTy) const {
  if (is64Bit) {
    switch(RelTy) {
      case R_X86_64_PC32:
      case R_X86_64_32:
        return 32;
      case R_X86_64_64:
        return 64;
    default:
      llvm_unreachable("unknown x86_64 relocation type");
    }
  } else {
    switch(RelTy) {
      case R_386_PC32:
      case R_386_32:
        return 32;
    default:
      llvm_unreachable("unknown x86 relocation type");
    }
  }
  return 0;
}

unsigned X86ELFWriterInfo::getJumpTableMachineRelocationTy() const {
  return X86::reloc_absolute_dword;
}

