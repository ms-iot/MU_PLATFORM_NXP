/** @file
  The OP-TEE Share Memory component is used to manage memory allocations
  from the memory block used for parameters & buffers shared between the
  client in normal world and the OpTEE OS in secure world.
**/

/*
 * Copyright (c) 2015, Microsoft Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <Uefi.h>
#include <Library/UefiLib.h>

#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "OpteeClientMem.h"
#include "OpteeClientDefs.h"

// Attributes for reserved memory which are missing from the common headers.

#define EFI_MEMORY_PRESENT      0x0100000000000000ULL
#define EFI_MEMORY_INITIALIZED  0x0200000000000000ULL
#define EFI_MEMORY_TESTED       0x0400000000000000ULL

// Use 16-byte as the default memory alignment for memory allocations.
#define OPTEE_CLIENT_MEM_BYTE_ALIGNMENT   16

static UINT64 mSharedMemAllocationCount = 0;

static UINT64 mSharedMemAllocSize = 0;

// OP-TEE reserved size on shared memory (Used for PL310 mutex)
#define OPTEE_SHM_RESERVED_SIZE   ((UINT64)EFI_PAGE_SIZE)

#define OPTEE_SHM_START \
  ((EFI_PHYSICAL_ADDRESS)(FixedPcdGet64 (PcdTrustZoneSharedMemoryBase) + OPTEE_SHM_RESERVED_SIZE))

#define OPTEE_SHM_SIZE \
  ((UINT64)FixedPcdGet64 (PcdTrustZoneSharedMemorySize) - OPTEE_SHM_RESERVED_SIZE)

#define OPTEE_SHM_END \
  ((EFI_PHYSICAL_ADDRESS) (OPTEE_SHM_START + OPTEE_SHM_SIZE))

#define OPTEE_SHM_TYPE  ((EFI_GCD_MEMORY_TYPE) EfiGcdMemoryTypeReserved)

#define OPTEE_SHM_CAPS \
  ((UINT64) (EFI_MEMORY_PRESENT |EFI_MEMORY_INITIALIZED | EFI_MEMORY_TESTED | EFI_MEMORY_WB))

// Allocated memory blocks header signature.
#define OPTEE_SHM_SIGNATURE  SIGNATURE_32 ('T', 'E', 'E', 'C')

#define OPTEE_SHM_FREE_FENCE  0xCC

#define OPTEE_SHM_ALLOC_ALIGN   EFI_PAGE_SIZE

#define OPTEE_SHM_ALLOC_ALIGN_SHIFT   EFI_PAGE_SHIFT

#define DUMP_GCD_MEMORY_SPACE_MAP 0

/** The UEFI allocation API's require the amount of memory to be freed in addition
  to the pointer to the memory to free. This header on the block contains the length
  allocated for that free.
**/
typedef struct {
  UINT32                Signature;
  EFI_HANDLE            Owner;
  UINT64                Size;
  EFI_PHYSICAL_ADDRESS  Address;
} OPTEE_CLIENT_MEM_HEADER;

//
// Lookup table used to print GCD Memory Space Map
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8 *mGcdMemoryTypeNames[] = {
  "NonExist ",  // EfiGcdMemoryTypeNonExistent
  "Reserved ",  // EfiGcdMemoryTypeReserved
  "SystemMem",  // EfiGcdMemoryTypeSystemMemory
  "MMIO     ",  // EfiGcdMemoryTypeMemoryMappedIo
  "PersisMem",  // EfiGcdMemoryTypePersistentMemory
  "MoreRelia",  // EfiGcdMemoryTypeMoreReliable
  "Unknown  "   // EfiGcdMemoryTypeMaximum
};

BOOLEAN
IsPowerOf2 (
  IN UINTN  Num
  )
{
  // Complement and compare
  return (Num != 0) && ((Num & (~Num + 1)) == Num);
}

UINTN
GetPowerOf2Exponent (
  IN UINTN  Num
  )
{
  ASSERT (IsPowerOf2 (Num));

  UINTN Exponent = 0;

  while (((Num & 1) == 0) && (Num > 1)) {
    ++Exponent;
    Num >>= 1;
  }

  return Exponent;
}

VOID
MarkSharedMemoryRegionAsFree (
  EFI_PHYSICAL_ADDRESS Address,
  UINT64 Size
  )
{
  UINT8 *Mem;
  UINT8 *End;

  Mem = (UINT8*) (UINTN) Address;
  End = (UINT8*) (UINTN) Mem + (UINTN) Size;

  while (Mem != End) {
    *Mem++ = OPTEE_SHM_FREE_FENCE;
  }
}


BOOLEAN
IsSharedMemoryRegionFree (
  EFI_PHYSICAL_ADDRESS Address,
  UINT64 Size
  )
{
  UINT8 *Mem;
  UINT8 *End;

  Mem = (UINT8*) (UINTN) Address;
  End = (UINT8*) (UINTN) Mem + (UINTN) Size;

  while (Mem != End) {
    if (*Mem++ != OPTEE_SHM_FREE_FENCE) return FALSE;
  }

  return TRUE;
}

VOID
DumpGcdMemorySpaceMap (
  VOID
  )
{
#if DUMP_GCD_MEMORY_SPACE_MAP
  EFI_STATUS Status;
  UINTN NumberOfDescriptors;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR *MemorySpaceMap = NULL;
  UINTN Index = 0;

  // Use the DXE Services Table to get the current GCD Memory Space Map
  Status = gDS->GetMemorySpaceMap (
                  &NumberOfDescriptors,
                  &MemorySpaceMap);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  LOG_INFO ("GCDMemType Range                             Capabilities     Attributes      ");
  LOG_INFO ("========== ================================= ================ ================");

  for (Index = 0; Index < NumberOfDescriptors; Index++) {
    LOG_INFO ("%a  %016lx-%016lx %016lx %016lx%c",
      mGcdMemoryTypeNames[MIN (MemorySpaceMap[Index].GcdMemoryType, EfiGcdMemoryTypeMaximum)],
      MemorySpaceMap[Index].BaseAddress,
      MemorySpaceMap[Index].BaseAddress + MemorySpaceMap[Index].Length - 1,
      MemorySpaceMap[Index].Capabilities,
      MemorySpaceMap[Index].Attributes,
      MemorySpaceMap[Index].ImageHandle == NULL ? ' ' : '*');
  }

Exit:

  if (MemorySpaceMap != NULL) {
    Status = gBS->FreePool (MemorySpaceMap);
    if (EFI_ERROR (Status)) {
      LOG_ERROR ("FreePool(..) failed. (Status=%r)", Status);
    }
  }
#endif
}


EFI_STATUS
GcdSharedMemoryInit (
  )
{
  EFI_STATUS Status;
  EFI_PHYSICAL_ADDRESS BaseAddress;

  ASSERT (OPTEE_SHM_START % EFI_PAGE_SIZE == 0);
  ASSERT (OPTEE_SHM_SIZE % EFI_PAGE_SIZE == 0);

  LOG_INFO (
    "Initializing OPTEE Shared Memory. (Start=0x%lX, End=0x%lX, Size=0x%lX)",
    OPTEE_SHM_START,
    OPTEE_SHM_END,
    OPTEE_SHM_SIZE);

  // Add the shared memory to the GCD so that we can allocate memory from it
  // on behalf of the OpTEE OS & OpTEE Client.
  Status = gDS->AddMemorySpace (
                  OPTEE_SHM_TYPE,
                  OPTEE_SHM_START,
                  OPTEE_SHM_SIZE,
                  OPTEE_SHM_CAPS);

  if (EFI_ERROR (Status)) {
    LOG_ERROR (
      "gDS->AddMemorySpace() failed. (Start=0x%lX, Size=0x%lX, Status=%r)",
      OPTEE_SHM_START,
      OPTEE_SHM_SIZE,
      Status);

    goto Exit;
  }

  BaseAddress = OPTEE_SHM_START;
  Status = gDS->AllocateMemorySpace (
                  EfiGcdAllocateAddress,
                  EfiGcdMemoryTypeReserved,
                  0,
                  OPTEE_SHM_SIZE,
                  &BaseAddress,
                  gDriverImageHandle,
                  NULL);

  if (EFI_ERROR (Status)) {
    LOG_ERROR (
      "gDS->AllocateMemorySpace() failed. (Start=0x%lX, Size=0x%lX, Status=%r)",
      OPTEE_SHM_START,
      OPTEE_SHM_SIZE,
      Status);

    goto Exit;
  }
  ASSERT (BaseAddress == OPTEE_SHM_START);

  // Clear the memory region with the fence value
  MarkSharedMemoryRegionAsFree (BaseAddress, OPTEE_SHM_SIZE);

  gDS->FreeMemorySpace (OPTEE_SHM_START, OPTEE_SHM_SIZE);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("gDS->FreeMemorySpace() failed. (Status=%r)", Status);
    goto Exit;
  }

  DumpGcdMemorySpaceMap ();

Exit:

  return Status;
}

/** Initlialize the OPTEE client memory component.
**/
EFI_STATUS
OpteeClientMemInit (
  )
{
  EFI_STATUS Status = EFI_SUCCESS;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR GcdMemDescriptor = { 0 };

  // Get the GCD info to determine if the memory has already been properly added.
  // This check allows the library to be initialized by multiple drivers and thus
  // skip attempting to add the space if it's already been added.
  Status = gDS->GetMemorySpaceDescriptor (
                  OPTEE_SHM_START,
                  &GcdMemDescriptor);

  if ((Status == EFI_NOT_FOUND) ||
    (!EFI_ERROR (Status) && (GcdMemDescriptor.GcdMemoryType ==  EfiGcdMemoryTypeNonExistent))) {
    Status = GcdSharedMemoryInit ();
    if (EFI_ERROR (Status)) {
      LOG_ERROR ("GcdSharedMemoryInit() failed. (Status=%r)", Status);
      goto Exit;
    }

    Status = gDS->GetMemorySpaceDescriptor (OPTEE_SHM_START, &GcdMemDescriptor);
    if (EFI_ERROR (Status)) {
      LOG_ERROR ("GetMemorySpaceDescriptor() failed. (Status=%r)", Status);
      goto Exit;
    }

  } else if (EFI_ERROR (Status)) {
      LOG_ERROR ("GetMemorySpaceDescriptor(..) failed. (Status=%r)", Status);
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
  }

  if (GcdMemDescriptor.GcdMemoryType != OPTEE_SHM_TYPE) {
    ASSERT (OPTEE_SHM_START == GcdMemDescriptor.BaseAddress);
    LOG_ERROR (
      "EFI_GCD_MEMORY_SPACE_DESCRIPTOR Mismatch at BaseAddress=0x%lX: "
      "Length=0x%lX, Expected Type=0x%X, Actual Type=0x%X",
      OPTEE_SHM_START,
      OPTEE_SHM_SIZE,
      OPTEE_SHM_TYPE,
      GcdMemDescriptor.GcdMemoryType);

      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
  }

Exit:

  return Status;
}

VOID*
InternalMemAlloc (
  IN UINTN  Size,
  IN UINTN  ByteAlignment
  )
{
  EFI_STATUS Status;
  UINT64 BlockSize;
  UINT64 AlignmentShift;
  EFI_PHYSICAL_ADDRESS BaseAddress;
  UINTN UserBaseAddress = 0;

  ASSERT (gDriverImageHandle != NULL);

  if (((ByteAlignment != 0) && !IsPowerOf2(ByteAlignment)) ||
      (ByteAlignment > OPTEE_SHM_ALLOC_ALIGN)) {
    ASSERT (FALSE);
    goto Exit;
  }

  ByteAlignment = OPTEE_SHM_ALLOC_ALIGN;
  AlignmentShift = OPTEE_SHM_ALLOC_ALIGN_SHIFT;

  // Allocate extra (ByteAlignment - 1) to guarantee that we have enough space to
  // cross the alignment boundary when aligning the UEFI allocated address.
  BlockSize = sizeof (OPTEE_CLIENT_MEM_HEADER) + Size + (ByteAlignment - 1);

  LOG_TRACE (
    "Size=0x%p, ByteAlignment=0x%p (1 << 0x%p), BlockSize=0x%p",
    Size,
    ByteAlignment,
    AlignmentShift,
    BlockSize);

  BaseAddress = OPTEE_SHM_END;
  Status = gDS->AllocateMemorySpace (
                  EfiGcdAllocateMaxAddressSearchTopDown,
                  EfiGcdMemoryTypeReserved,
                  AlignmentShift,
                  BlockSize,
                  &BaseAddress,
                  gDriverImageHandle,
                  NULL);

  if (EFI_ERROR (Status)) {
    LOG_ERROR (
      "gDS->AllocateMemorySpace() failed. "
      "(mSharedMemAllocationCount=0x%lX, mSharedMemAllocSize=0x%lX, "
      "BlockSize=0x%lX, AlignmentShift=0x%lX, Status=%r)",
      mSharedMemAllocationCount,
      mSharedMemAllocSize,
      BlockSize,
      AlignmentShift,
      Status);

    goto Exit;
  }

  ASSERT (BaseAddress >= OPTEE_SHM_START);
  ASSERT ((BaseAddress + BlockSize) <= OPTEE_SHM_END);

  if (!IsSharedMemoryRegionFree (BaseAddress, BlockSize)) {
    LOG_ERROR ("!! Potential memory overflow detected !!");
    ASSERT (FALSE);
    goto Exit;
  }

  // Calculate the aligned address by offsetting the UEFI allocated address
  // enough to cross the alignment boundary and at the same time have room for
  // the memory header at the beginning and before the resulting aligned address.
  UserBaseAddress =
    (UINTN) BaseAddress + sizeof (OPTEE_CLIENT_MEM_HEADER) + (ByteAlignment - 1);
  // Mask the offsetted address to align it.
  UserBaseAddress &= ~(ByteAlignment - 1);

  ASSERT (UserBaseAddress > OPTEE_SHM_START);
  ASSERT ((UserBaseAddress + Size) <= OPTEE_SHM_END);
  ASSERT (UserBaseAddress % ByteAlignment == 0);

  LOG_TRACE ("Memory block allocated. (BaseAddress=0x%p)", (UINT32)(UINTN)BaseAddress);

  // Resultant Memory Block Layout:
  //
  // BaseAddress                 UserBaseAddress + Size
  // |                           |
  // [...][Header][Aligned Block][...]
  //      |       |                   |
  //      |       UserBaseAddress     BaseAddress + BlockSize
  //      |
  //      UserBaseAddress - sizeof (OPTEE_CLIENT_MEM_HEADER)

  // Fill in the size into the header and account for the header at the start
  // of the memory block.
  {
    OPTEE_CLIENT_MEM_HEADER *Header =
      (OPTEE_CLIENT_MEM_HEADER *) (UserBaseAddress - sizeof (OPTEE_CLIENT_MEM_HEADER));

    ZeroMem (Header, sizeof (*Header));

    Header->Signature = OPTEE_SHM_SIGNATURE;
    Header->Size = BlockSize;
    Header->Address = BaseAddress;

    mSharedMemAllocationCount += 1;
    mSharedMemAllocSize += Header->Size;
  }

Exit:
  return (VOID *) UserBaseAddress;

}

/** Allocate block of memory from the TrustZone shared memory block.

  The allocated block is guaranteed to be 16-byte (128-bit) aligned
**/
VOID*
OpteeClientMemAlloc (
  IN UINTN  Size
  )
{
  return InternalMemAlloc (Size, OPTEE_CLIENT_MEM_BYTE_ALIGNMENT);
}

/** Allocate aligned block of memory from the TrustZone shared memory block.
**/
VOID*
OpteeClientAlignedMemAlloc (
  IN UINTN  Size,
  IN UINTN  ByteAlignment
  )
{
  return InternalMemAlloc (Size, ByteAlignment);
}

/** Free a block of memory previously allocated using OpteeClientMemAlloc(..).
**/
EFI_STATUS
OpteeClientMemFree (
  IN VOID   *Mem
  )
{
  EFI_STATUS Status;
  OPTEE_CLIENT_MEM_HEADER* Header =
    (OPTEE_CLIENT_MEM_HEADER *) (((UINTN) (Mem)) - sizeof (OPTEE_CLIENT_MEM_HEADER));

  if (Header->Signature != OPTEE_SHM_SIGNATURE) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  EFI_PHYSICAL_ADDRESS Address = Header->Address;
  UINT64 Size = Header->Size;

  MarkSharedMemoryRegionAsFree (Header->Address, Header->Size);

  Status = gDS->FreeMemorySpace (Address, Size);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("gDS->FreeMemorySpace() failed. (Status=%r)", Status);
    goto Exit;
  }

  mSharedMemAllocationCount -= 1;
  mSharedMemAllocSize -= Size;

Exit:
  return Status;
}

VOID
OpteeClientApiFinalize (
  VOID
  )
{
  LOG_INFO ("Finalizing OPTEE Client API Lib");

  DumpGcdMemorySpaceMap ();

  LOG_INFO (
      "mSharedMemAllocaitonCount=0x%lX, mSharedMemAllocSize=0x%lX",
      mSharedMemAllocationCount,
      mSharedMemAllocSize);

  if (mSharedMemAllocationCount != 0 || mSharedMemAllocSize != 0) {
    LOG_ERROR ("!! Potential shared memory leak detected !!");
    ASSERT (FALSE);
    return;
  }

  return;
}
