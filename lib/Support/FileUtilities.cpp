//===- Support/FileUtilities.cpp - File System Utilities ------------------===//
// 
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
// 
//===----------------------------------------------------------------------===//
//
// This file implements a family of utility functions which are useful for doing
// various things with files.
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/FileUtilities.h"
#include "llvm/System/Path.h"
#include "llvm/System/MappedFile.h"
#include "llvm/ADT/StringExtras.h"
#include <cmath>
#include <fstream>
#include <iostream>

using namespace llvm;

/// DiffFiles - Compare the two files specified, returning true if they are
/// different or if there is a file error.  If you specify a string to fill in
/// for the error option, it will set the string to an error message if an error
/// occurs, allowing the caller to distinguish between a failed diff and a file
/// system error.
///
bool llvm::DiffFiles(const std::string &FileA, const std::string &FileB,
                     std::string *Error) {
  std::ios::openmode io_mode = std::ios::in | std::ios::binary;
  std::ifstream FileAStream(FileA.c_str(), io_mode);
  if (!FileAStream) {
    if (Error) *Error = "Couldn't open file '" + FileA + "'";
    return true;
  }

  std::ifstream FileBStream(FileB.c_str(), io_mode);
  if (!FileBStream) {
    if (Error) *Error = "Couldn't open file '" + FileB + "'";
    return true;
  }

  // Compare the two files...
  int C1, C2;
  do {
    C1 = FileAStream.get();
    C2 = FileBStream.get();
    if (C1 != C2) return true;
  } while (C1 != EOF);

  return false;
}

/// MoveFileOverIfUpdated - If the file specified by New is different than Old,
/// or if Old does not exist, move the New file over the Old file.  Otherwise,
/// remove the New file.
///
void llvm::MoveFileOverIfUpdated(const std::string &New,
                                 const std::string &Old) {
  if (DiffFiles(New, Old)) {
    if (std::rename(New.c_str(), Old.c_str()))
      std::cerr << "Error renaming '" << New << "' to '" << Old << "'!\n";
  } else {
    std::remove(New.c_str());
  }  
}

static bool isNumberChar(char C) {
  switch (C) {
  case '0': case '1': case '2': case '3': case '4':
  case '5': case '6': case '7': case '8': case '9': 
  case '.': case '+': case '-':
  case 'e':
  case 'E': return true;
  default: return false;
  }
}

static char *BackupNumber(char *Pos, char *FirstChar) {
  // If we didn't stop in the middle of a number, don't backup.
  if (!isNumberChar(*Pos)) return Pos;

  // Otherwise, return to the start of the number.
  while (Pos > FirstChar && isNumberChar(Pos[-1]))
    --Pos;
  return Pos;
}

/// CompareNumbers - compare two numbers, returning true if they are different.
static bool CompareNumbers(char *&F1P, char *&F2P, char *F1End, char *F2End,
                           double AbsTolerance, double RelTolerance,
                           std::string *ErrorMsg) {
  char *F1NumEnd, *F2NumEnd;
  double V1 = 0.0, V2 = 0.0; 
  // If we stop on numbers, compare their difference.
  if (isNumberChar(*F1P) && isNumberChar(*F2P)) {
    V1 = strtod(F1P, &F1NumEnd);
    V2 = strtod(F2P, &F2NumEnd);
  } else {
    // Otherwise, the diff failed.
    F1NumEnd = F1P;
    F2NumEnd = F2P;
  }

  if (F1NumEnd == F1P || F2NumEnd == F2P) {
    if (ErrorMsg) *ErrorMsg = "Comparison failed, not a numeric difference.";
    return true;
  }

  // Check to see if these are inside the absolute tolerance
  if (AbsTolerance < std::abs(V1-V2)) {
    // Nope, check the relative tolerance...
    double Diff;
    if (V2)
      Diff = std::abs(V1/V2 - 1.0);
    else if (V1)
      Diff = std::abs(V2/V1 - 1.0);
    else
      Diff = 0;  // Both zero.
    if (Diff > RelTolerance) {
      if (ErrorMsg) {
        *ErrorMsg = "Compared: " + ftostr(V1) + " and " + ftostr(V2) +
                    ": diff = " + ftostr(Diff) + "\n";
        *ErrorMsg += "Out of tolerance: rel/abs: " + ftostr(RelTolerance) +
                     "/" + ftostr(AbsTolerance);
      }
      return true;
    }
  }

  // Otherwise, advance our read pointers to the end of the numbers.
  F1P = F1NumEnd;  F2P = F2NumEnd;
  return false;
}

// PadFileIfNeeded - If the files are not identical, we will have to be doing
// numeric comparisons in here.  There are bad cases involved where we (i.e.,
// strtod) might run off the beginning or end of the file if it starts or ends
// with a number.  Because of this, if needed, we pad the file so that it starts
// and ends with a null character.
static void PadFileIfNeeded(char *&FileStart, char *&FileEnd, char *&FP) {
  if (isNumberChar(FileStart[0]) || isNumberChar(FileEnd[-1])) {
    unsigned FileLen = FileEnd-FileStart;
    char *NewFile = new char[FileLen+2];
    NewFile[0] = 0;              // Add null padding
    NewFile[FileLen+1] = 0;      // Add null padding
    memcpy(NewFile+1, FileStart, FileLen);
    FP = NewFile+(FP-FileStart)+1;
    FileStart = NewFile+1;
    FileEnd = FileStart+FileLen;
  }
}

/// DiffFilesWithTolerance - Compare the two files specified, returning 0 if the
/// files match, 1 if they are different, and 2 if there is a file error.  This
/// function differs from DiffFiles in that you can specify an absolete and
/// relative FP error that is allowed to exist.  If you specify a string to fill
/// in for the error option, it will set the string to an error message if an
/// error occurs, allowing the caller to distinguish between a failed diff and a
/// file system error.
///
int llvm::DiffFilesWithTolerance(const std::string &FileA,
                                 const std::string &FileB,
                                 double AbsTol, double RelTol,
                                 std::string *Error) {
  try {
    // Map in the files into memory.
    sys::MappedFile F1((sys::Path(FileA)));
    sys::MappedFile F2((sys::Path(FileB)));
    F1.map();
    F2.map();

    // Okay, now that we opened the files, scan them for the first difference.
    char *File1Start = F1.charBase();
    char *File2Start = F2.charBase();
    char *File1End = File1Start+F1.size();
    char *File2End = File2Start+F2.size();
    char *F1P = File1Start;
    char *F2P = File2Start;

    // Scan for the end of file or first difference.
    while (F1P < File1End && F2P < File2End && *F1P == *F2P)
      ++F1P, ++F2P;

    // Common case: identifical files.
    if (F1P == File1End && F2P == File2End) return 0;

    char *OrigFile1Start = File1Start;
    char *OrigFile2Start = File2Start;

    // If the files need padding, do so now.
    PadFileIfNeeded(File1Start, File1End, F1P);
    PadFileIfNeeded(File2Start, File2End, F2P);
    
    bool CompareFailed = false;
    while (1) {
      // Scan for the end of file or next difference.
      while (F1P < File1End && F2P < File2End && *F1P == *F2P)
        ++F1P, ++F2P;

      if (F1P >= File1End || F2P >= File2End) break;

      // Okay, we must have found a difference.  Backup to the start of the
      // current number each stream is at so that we can compare from the
      // beginning.
      F1P = BackupNumber(F1P, File1Start);
      F2P = BackupNumber(F2P, File2Start);

      // Now that we are at the start of the numbers, compare them, exiting if
      // they don't match.
      if (CompareNumbers(F1P, F2P, File1End, File2End, AbsTol, RelTol, Error)) {
        CompareFailed = true;
        break;
      }
    }

    // Okay, we reached the end of file.  If both files are at the end, we
    // succeeded.
    bool F1AtEnd = F1P >= File1End;
    bool F2AtEnd = F2P >= File2End;
    if (!CompareFailed && (!F1AtEnd || !F2AtEnd)) {
      // Else, we might have run off the end due to a number: backup and retry.
      if (F1AtEnd && isNumberChar(F1P[-1])) --F1P;
      if (F2AtEnd && isNumberChar(F2P[-1])) --F2P;
      F1P = BackupNumber(F1P, File1Start);
      F2P = BackupNumber(F2P, File2Start);

      // Now that we are at the start of the numbers, compare them, exiting if
      // they don't match.
      if (CompareNumbers(F1P, F2P, File1End, File2End, AbsTol, RelTol, Error))
        CompareFailed = true;

      // If we found the end, we succeeded.
      if (F1P < File1End || F2P < File2End)
        CompareFailed = true;
    }

    if (OrigFile1Start != File1Start)
      delete[] File1Start;
    if (OrigFile2Start != File2Start)
      delete[] File2Start;
    return CompareFailed;
  } catch (const std::string &Msg) {
    if (Error) *Error = Msg;
    return 2;
  }
}
