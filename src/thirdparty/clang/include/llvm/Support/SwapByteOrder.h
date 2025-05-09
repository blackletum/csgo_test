﻿//===- SwapByteOrder.h - Generic and optimized byte swaps -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares generic and optimized functions to swap the byte order of
// an integral type.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SUPPORT_SWAPBYTEORDER_H
#define LLVM_SUPPORT_SWAPBYTEORDER_H

#include "llvm/Support/DataTypes.h"
#include <cstddef>
#include <limits>

namespace llvm {
namespace sys {

/// SwapByteOrder_16 - This function returns a byte-swapped representation of
/// the 16-bit argument.
inline uint16_t SwapByteOrder_16(uint16_t value) {
#if defined(_MSC_VER) && !defined(_DEBUG)
  // The DLL version of the runtime lacks these functions (bug!?), but in a
  // release build they're replaced with BSWAP instructions anyway.
  return _byteswap_ushort(value);
#else
  uint16_t Hi = value << 8;
  uint16_t Lo = value >> 8;
  return Hi | Lo;
#endif
}

/// SwapByteOrder_32 - This function returns a byte-swapped representation of
/// the 32-bit argument.
inline uint32_t SwapByteOrder_32(uint32_t value) {
#if defined(__llvm__) || \
(__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)) && !defined(__ICC)
  return __builtin_bswap32(value);
#elif defined(_MSC_VER) && !defined(_DEBUG)
  return _byteswap_ulong(value);
#else
  uint32_t Byte0 = value & 0x000000FF;
  uint32_t Byte1 = value & 0x0000FF00;
  uint32_t Byte2 = value & 0x00FF0000;
  uint32_t Byte3 = value & 0xFF000000;
  return (Byte0 << 24) | (Byte1 << 8) | (Byte2 >> 8) | (Byte3 >> 24);
#endif
}

/// SwapByteOrder_64 - This function returns a byte-swapped representation of
/// the 64-bit argument.
inline uint64_t SwapByteOrder_64(uint64_t value) {
#if defined(__llvm__) || \
(__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)) && !defined(__ICC)
  return __builtin_bswap64(value);
#elif defined(_MSC_VER) && !defined(_DEBUG)
  return _byteswap_uint64(value);
#else
  uint64_t Hi = SwapByteOrder_32(uint32_t(value));
  uint32_t Lo = SwapByteOrder_32(uint32_t(value >> 32));
  return (Hi << 32) | Lo;
#endif
}

inline unsigned char  SwapByteOrder(unsigned char C) { return C; }
inline   signed char  SwapByteOrder(signed char C) { return C; }
inline          char  SwapByteOrder(char C) { return C; }

inline unsigned short SwapByteOrder(unsigned short C) { return SwapByteOrder_16(C); }
inline   signed short SwapByteOrder(  signed short C) { return SwapByteOrder_16(C); }

inline unsigned int   SwapByteOrder(unsigned int   C) { return SwapByteOrder_32(C); }
inline   signed int   SwapByteOrder(  signed int   C) { return SwapByteOrder_32(C); }

#if __LONG_MAX__ == __INT_MAX__
inline unsigned long  SwapByteOrder(unsigned long  C) { return SwapByteOrder_32(C); }
inline   signed long  SwapByteOrder(  signed long  C) { return SwapByteOrder_32(C); }
#elif __LONG_MAX__ == __LONG_LONG_MAX__
inline unsigned long  SwapByteOrder(unsigned long  C) { return SwapByteOrder_64(C); }
inline   signed long  SwapByteOrder(  signed long  C) { return SwapByteOrder_64(C); }
#else
#error "Unknown long size!"
#endif

inline unsigned long long SwapByteOrder(unsigned long long C) {
  return SwapByteOrder_64(C);
}
inline signed long long SwapByteOrder(signed long long C) {
  return SwapByteOrder_64(C);
}

} // end namespace sys
} // end namespace llvm

#endif
