/** @file
*
*  Copyright (c) Microsoft Corporation. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#ifndef __OPTEE_CLIENT_DEFS_H__
#define __OPTEE_CLIENT_DEFS_H__

extern EFI_HANDLE gDriverImageHandle;

#ifndef C_ASSERT
#define C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]
#endif // C_ASSERT

// Swap the byte order of a 16-bit (2-byte) value.
#define SWAP_B16(u16) \
  ((((UINT16)(u16)<<8) & 0xFF00) | (((UINT16)(u16)>>8) & 0x00FF))

// Swap the byte order of a 32-bit (4-byte) value.
#define SWAP_B32(u32) \
  ((((UINT32)(u32)<<24) & 0xFF000000) | \
  (((UINT32)(u32)<< 8) & 0x00FF0000) | \
  (((UINT32)(u32)>> 8) & 0x0000FF00) | \
  (((UINT32)(u32)>>24) & 0x000000FF))

// Swap the byte order of a 64-bit (8-byte) value.
#define SWAP_D64(u64) \
  (((UINT64)(u64) >> 32) | (((UINT64)(u64) & 0xFFFFFFFF) << 32))

#define OPTEE_MSG_ATTR_GET_TYPE(ATTR) ((ATTR) & OPTEE_MSG_ATTR_TYPE_MASK)

// OPTEE Client DEBUG flag to use with BaseDebugLib
#define DEBUG_OPTEE   0x00008000

#define DEBUG_CONSOLE 1

#if DEBUG_CONSOLE

// Logging Macros

#define LOG_TRACE_FMT_HELPER(FUNC, FMT, ...)  "TEEC[T]:%a:" FMT "%a\n", FUNC, __VA_ARGS__

#define LOG_INFO_FMT_HELPER(FMT, ...)   "TEEC[I]:" FMT "%a\n", __VA_ARGS__

#define LOG_ERROR_FMT_HELPER(FMT, ...) \
  "TEEC[E]:" FMT " (%a: %a, %d)\n", __VA_ARGS__


#define LOG_INFO(...) \
  DEBUG ((DEBUG_INIT, LOG_INFO_FMT_HELPER (__VA_ARGS__, "")))

#define LOG_VANILLA_TRACE(...) \
  DEBUG ((DEBUG_VERBOSE | DEBUG_OPTEE, __VA_ARGS__))

#define LOG_TRACE(...) \
  DEBUG ((DEBUG_VERBOSE | DEBUG_OPTEE, LOG_TRACE_FMT_HELPER (__FUNCTION__, __VA_ARGS__, "")))

#define LOG_ERROR(...) \
  DEBUG ((DEBUG_ERROR, LOG_ERROR_FMT_HELPER (__VA_ARGS__, __FUNCTION__, __FILE__, __LINE__)))
#else

#define WIDEN(X)   L## X

// Logging Macros

#define LOG_TRACE_FMT_HELPER(FUNC, FMT, ...)  L"TEEC[T]:%a:" WIDEN(FMT) L"%a\n", FUNC, __VA_ARGS__

#define LOG_VANILLA_TRACE_FMT_HELPER(FMT, ...)  WIDEN(FMT), __VA_ARGS__

#define LOG_INFO_FMT_HELPER(FMT, ...)   L"TEEC[I]:" WIDEN(FMT) L"%a\n", __VA_ARGS__

#define LOG_ERROR_FMT_HELPER(FMT, ...) \
  L"TEEC[E]:" WIDEN(FMT) L" (%a: %a, %d)\n", __VA_ARGS__

#define LOG_INFO(...) \
  Print (LOG_INFO_FMT_HELPER (__VA_ARGS__, L""))

#define LOG_VANILLA_TRACE(...) \
  Print (LOG_VANILLA_TRACE_FMT_HELPER (__VA_ARGS__, L""))

#define LOG_TRACE(...) \
  Print (LOG_TRACE_FMT_HELPER (__FUNCTION__, __VA_ARGS__, L""))

#define LOG_ERROR(...) \
  Print (LOG_ERROR_FMT_HELPER (__VA_ARGS__, __FUNCTION__, __FILE__, __LINE__))
#endif

#define LOG_ASSERT(X) ASSERT (!(X))

#endif // __OPTEE_CLIENT_DEFS_H__

