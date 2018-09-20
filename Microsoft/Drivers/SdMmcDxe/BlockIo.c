/** @file
*
*  Copyright (c) Microsoft Corporation. All rights reserved.
*  Copyright (c) 2011-2014, ARM Limited. All rights reserved.
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

#include <Uefi.h>

#include <Protocol/BlockIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/Sdhc.h>
#include <Protocol/RpmbIo.h>

#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/TimerLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>

#include "SdMmcHw.h"
#include "SdMmc.h"
#include "Protocol.h"

VOID
BenchmarkIo (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN SD_TRANSFER_DIRECTION  TransferDirection,
  IN UINT32                 MediaId,
  IN UINTN                  BufferSize,
  IN UINT32                 Iterations
  );

EFI_STATUS
IoBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN SD_TRANSFER_DIRECTION  TransferDirection,
  IN UINT32                 MediaId,
  IN EFI_LBA                Lba,
  IN UINTN                  BufferSize,
  IN OUT VOID               *Buffer
  );

VOID
SortIoReadStatsByTotalTransferTime (
  IN IoReadStatsEntry*   Table,
  IN UINT32              NumEntries
  )
{
  // Using the simple insertion sort
  UINT32 Idx;
  for (Idx = 1; Idx < NumEntries; ++Idx) {
    IoReadStatsEntry CurrEntry = Table[Idx];
    UINT32 J = Idx;
    while (J > 0 &&
           Table[J - 1].TotalTransferTimeUs < CurrEntry.TotalTransferTimeUs) {
      Table[J] = Table[J - 1];
      --J;
    }
    Table[J] = CurrEntry;
  }
}

VOID
BenchmarkIo (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN SD_TRANSFER_DIRECTION  TransferDirection,
  IN UINT32                 MediaId,
  IN UINTN                  BufferSize,
  IN UINT32                 Iterations
  )
{
  ASSERT (Iterations > 0);

  EFI_STATUS Status;
  UINT32 BufferSizeKB = INT_DIV_ROUND (BufferSize, 1024);
  VOID* Buffer = AllocateZeroPool (BufferSize);
  if (Buffer == NULL) {
    LOG_ERROR ("BenchmarkIo() : No enough memory to allocate %dKB buffer", BufferSizeKB);
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  UINT32 CurrIteration = Iterations;
  UINT64 TotalTransfersTimeUs = 0;

  while (CurrIteration--) {
    UINT64 StartTime = GetPerformanceCounter ();

    Status = IoBlocks (
      This,
      TransferDirection,
      MediaId,
      0, // Lba
      BufferSize,
      Buffer
    );
    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    UINT64 EndTime = GetPerformanceCounter ();
    TotalTransfersTimeUs += (((EndTime - StartTime) * 1000000UL) / gHpcTicksPerSeconds);
  }

  UINT32 KBps;
  KBps = (UINT32) (((UINT64) BufferSizeKB * (UINT64) Iterations * 1000000UL) / TotalTransfersTimeUs);
  LOG_INFO (
    "- BenchmarkIo(%a, %dKB)\t: Xfr Avg:%dus\t%dKBps\t%dMBps",
    (TransferDirection == SdTransferDirectionRead) ? "Read" : "Write",
    BufferSizeKB,
    (UINT32) (TotalTransfersTimeUs / Iterations),
    KBps,
    INT_DIV_ROUND (KBps, 1024));

Exit:
  if (Buffer != NULL) {
    FreePool (Buffer);
  }
}

EFI_STATUS
IoBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN SD_TRANSFER_DIRECTION  TransferDirection,
  IN UINT32                 MediaId,
  IN EFI_LBA                Lba,
  IN UINTN                  BufferSize,
  IN OUT VOID               *Buffer
  )
{
  SDHC_INSTANCE *HostInst;
  EFI_STATUS Status;
  CONST SD_COMMAND *Cmd;
  UINT32 BlockCount;

  HostInst = SDHC_INSTANCE_FROM_BLOCK_IO_THIS (This);
  ASSERT (HostInst);
  ASSERT (HostInst->HostExt);

  if (This->Media->MediaId != MediaId) {
    Status = EFI_MEDIA_CHANGED;
    goto Exit;
  }

  if (Buffer == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  // Check if a Card is Present
  if (!HostInst->BlockIo.Media->MediaPresent) {
    Status = EFI_NO_MEDIA;
    goto Exit;
  }

  // Reading 0 Byte is valid
  if (BufferSize == 0) {
    Status = EFI_SUCCESS;
    goto Exit;
  }

  if ((TransferDirection == SdTransferDirectionWrite) && (This->Media->ReadOnly == TRUE)) {
    Status = EFI_WRITE_PROTECTED;
    goto Exit;
  }

    // The buffer size must be an exact multiple of the block size
  if ((BufferSize % This->Media->BlockSize) != 0) {
    Status = EFI_BAD_BUFFER_SIZE;
    goto Exit;
  }

  BlockCount = BufferSize / This->Media->BlockSize;

  // All blocks must be within the device
  if ((Lba + BlockCount - 1) > (This->Media->LastBlock)) {
    LOG_ERROR (
      "Data span is out of media address range. (Media Last Block LBA = 0x%lx"
      "Data Last Block LBA = 0x%lx)",
      This->Media->LastBlock,
      (Lba + BlockCount - 1));

    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  // Check the alignment
  if ((This->Media->IoAlign > 2) && (((UINTN) Buffer & (This->Media->IoAlign - 1)) != 0)) {
    LOG_ERROR (
      "Invalid buffer address alignment (Buffer = %p, IoAlign = %d)",
      Buffer,
      This->Media->IoAlign);

    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  if (TransferDirection == SdTransferDirectionRead) {
    if (BlockCount == 1) {
      Cmd = &CmdReadSingleBlock;
    } else {
      Cmd = &CmdReadMultiBlock;
    }
  } else {
    if (BlockCount == 1) {
      Cmd = &CmdWriteSingleBlock;
    } else {
      Cmd = &CmdWriteMultiBlock;
    }
  }

  CONST UINT32 DataCommandRetryCount = 3;
  CONST UINT32 MaxTransferSize =
    HostInst->HostCapabilities.MaximumBlockCount * This->Media->BlockSize;
  UINT32 Retry;
  UINT32 BytesRemaining = BufferSize;
  UINTN CurrentBufferSize;
  VOID *CurrentBuffer = Buffer;
  UINT32 CurrentLba = (UINT32) Lba;

  for (; BytesRemaining > 0;) {
    if (BytesRemaining < MaxTransferSize) {
      CurrentBufferSize = BytesRemaining;
    } else {
      CurrentBufferSize = MaxTransferSize;
    }

    for (Retry = 0; Retry < DataCommandRetryCount; ++Retry) {
      Status = SdhcSendDataCommand (
        HostInst,
        Cmd,
        CurrentLba,
        CurrentBufferSize,
        CurrentBuffer);

      if (!EFI_ERROR (Status)) {
        if (Retry > 0) {
          LOG_TRACE ("SdhcSendDataCommand succeeded after %d retry", Retry);
        }
        break;
      }

      // On SdhcSendDataCommand return, proper error recovery has been performed
      // and it should be safe to retry the same transfer.

      LOG_ERROR ("SdhcSendDataCommand failed on retry %d", Retry);
    }

    BytesRemaining -= CurrentBufferSize;
    CurrentLba += CurrentBufferSize / This->Media->BlockSize;
    CurrentBuffer = (VOID*) ((UINTN) CurrentBuffer + CurrentBufferSize);
  }

Exit:
  if (EFI_ERROR (Status)) {
    LOG_ERROR (
      "IoBlocks(%c, LBA:0x%08lx, Size(B):0x%x): SdhcSendDataCommand() failed. %r",
      ((TransferDirection == SdTransferDirectionRead) ? 'R' : 'W'),
      Lba,
      BufferSize,
      Status);
  }

  return Status;
}

// EFI_BLOCK_IO Protocol Callbacks

/**
  Reset the block device.

  This function implements EFI_BLOCK_IO_PROTOCOL.Reset().
  It resets the block device hardware.
  ExtendedVerification is ignored in this implementation.

  @param  This                   Indicates a pointer to the calling context.
  @param  ExtendedVerification   Indicates that the driver may perform a more exhaustive
                                 verification operation of the device during reset.

  @retval EFI_SUCCESS            The block device was reset.
  @retval EFI_DEVICE_ERROR       The block device is not functioning correctly and could not be reset.

**/
EFI_STATUS
EFIAPI
BlockIoReset (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN BOOLEAN                ExtendedVerification
  )
{
  LOG_TRACE ("BlockIoReset()");

  SDHC_INSTANCE *HostInst = SDHC_INSTANCE_FROM_BLOCK_IO_THIS (This);
  return SoftReset (HostInst);
}

/**
  Reads the requested number of blocks from the device.

  This function implements EFI_BLOCK_IO_PROTOCOL.ReadBlocks().
  It reads the requested number of blocks from the device.
  All the blocks are read, or an error is returned.

  @param  This                   Indicates a pointer to the calling context.
  @param  MediaId                The media ID that the read request is for.
  @param  Lba                    The starting logical block address to read from on the device.
  @param  BufferSize             The size of the Buffer in bytes.
                                 This must be a multiple of the intrinsic block size of the device.
  @param  Buffer                 A pointer to the destination buffer for the data. The caller is
                                 responsible for either having implicit or explicit ownership of the buffer.

  @retval EFI_SUCCESS            The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR       The device reported an error while attempting to perform the read operation.
  @retval EFI_NO_MEDIA           There is no media in the device.
  @retval EFI_MEDIA_CHANGED      The MediaId is not for the current media.
  @retval EFI_BAD_BUFFER_SIZE    The BufferSize parameter is not a multiple of the intrinsic block size of the device.
  @retval EFI_INVALID_PARAMETER  The read request contains LBAs that are not valid,
                                 or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
BlockIoReadBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 MediaId,
  IN EFI_LBA                Lba,
  IN UINTN                  BufferSize,
  OUT VOID                  *Buffer
  )
{
  LOG_TRACE ("BlockIoReadBlocks()");

#if SDMMC_BENCHMARK_IO
  static BOOLEAN BenchmarkDone = FALSE;
  if (!BenchmarkDone) {

    LOG_INFO ("Benchmarking BlockIo Read");

    UINT32 CurrByteSize = 512;
    CONST UINT32 MaxByteSize = 8388608; // 8MB Max
    for (; CurrByteSize <= MaxByteSize; CurrByteSize *= 2) {
      BenchmarkIo (This, SdTransferDirectionRead, MediaId, CurrByteSize, 10);
    }

    BenchmarkDone = TRUE;
  }
#endif // SDMMC_BENCHMARK_IO

#if SDMMC_COLLECT_STATISTICS
  UINT32 NumBlocks = BufferSize / This->Media->BlockSize;
  SDHC_INSTANCE *HostInst = SDHC_INSTANCE_FROM_BLOCK_IO_THIS (This);
  CONST UINT32 TableSize = ARRAYSIZE (HostInst->IoReadStats);
  IoReadStatsEntry *CurrentReadEntry = NULL;

  UINT32 BlockIdx;
  for (BlockIdx = 0; BlockIdx < TableSize; ++BlockIdx) {
    IoReadStatsEntry *Entry = HostInst->IoReadStats + BlockIdx;
    // Reached end of table and didn't find a match, append an entry
    if (Entry->NumBlocks == 0) {
      ++HostInst->IoReadStatsNumEntries;
      Entry->NumBlocks = NumBlocks;
    }

    if (Entry->NumBlocks == NumBlocks) {
      CurrentReadEntry = Entry;
      ++Entry->Count;
      break;
    }
  }
  ASSERT (BlockIdx < TableSize);

  UINT64 StartTime = GetPerformanceCounter ();

  EFI_STATUS Status = IoBlocks (
    This,
    SdTransferDirectionRead,
    MediaId,
    Lba,
    BufferSize,
    Buffer
  );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  UINT64 EndTime = GetPerformanceCounter ();

  ASSERT (CurrentReadEntry != NULL);
  CurrentReadEntry->TotalTransferTimeUs +=
    (UINT32) (((EndTime - StartTime) * 1000000UL) / gHpcTicksPerSeconds);

  //
  // Run statistics and dump updates
  //
  SortIoReadStatsByTotalTransferTime (
    HostInst->IoReadStats,
    HostInst->IoReadStatsNumEntries);

  IoReadStatsEntry *MaxNumBlocksEntry = HostInst->IoReadStats;
  IoReadStatsEntry *MaxCountEntry = HostInst->IoReadStats;
  UINT32 TotalReadTimeUs = 0;
  UINT32 TotalReadBlocksCount = 0;

  LOG_INFO (" #Blks\tCnt\tAvg(us)\tAll(us)");

  for (BlockIdx = 0; BlockIdx < HostInst->IoReadStatsNumEntries; ++BlockIdx) {
    IoReadStatsEntry *CurrEntry = HostInst->IoReadStats + BlockIdx;

    if (CurrEntry->NumBlocks > MaxNumBlocksEntry->NumBlocks) {
      MaxNumBlocksEntry = CurrEntry;
    }

    if (CurrEntry->Count > MaxCountEntry->Count) {
      MaxCountEntry = CurrEntry;
    }

    TotalReadTimeUs += CurrEntry->TotalTransferTimeUs;
    TotalReadBlocksCount += (CurrEntry->NumBlocks * CurrEntry->Count);

    // Show only the top 5 time consuming transfers
    if (BlockIdx < 5) {
      LOG_INFO (
        " %d\t%d\t%d\t%d",
        (UINT32) CurrEntry->NumBlocks,
        (UINT32) CurrEntry->Count,
        (UINT32) (CurrEntry->TotalTransferTimeUs / CurrEntry->Count),
        (UINT32) CurrEntry->TotalTransferTimeUs);
    }
  }

  LOG_INFO (
    "MaxNumBlocksEntry: %d %d %dus",
    (UINT32) MaxNumBlocksEntry->NumBlocks,
    (UINT32) MaxNumBlocksEntry->Count,
    (UINT32) MaxNumBlocksEntry->TotalTransferTimeUs);

  LOG_INFO (
    "MaxCountEntry: %d %d %dus",
    (UINT32) MaxCountEntry->NumBlocks,
    (UINT32) MaxCountEntry->Count,
    (UINT32) MaxCountEntry->TotalTransferTimeUs);

  LOG_INFO (
    "UEFI spent %dus~%ds reading %dMB from SDCard\n",
    TotalReadTimeUs,
    INT_DIV_ROUND (TotalReadTimeUs, 1000000),
    INT_DIV_ROUND (TotalReadBlocksCount * This->Media->BlockSize, (1024 * 1024)));
Exit:
  return Status;

#else
  return IoBlocks (This, SdTransferDirectionRead, MediaId, Lba, BufferSize, Buffer);
#endif // SDMMC_COLLECT_STATISTICS
}

/**
  Writes a specified number of blocks to the device.

  This function implements EFI_BLOCK_IO_PROTOCOL.WriteBlocks().
  It writes a specified number of blocks to the device.
  All blocks are written, or an error is returned.

  @param  This                   Indicates a pointer to the calling context.
  @param  MediaId                The media ID that the write request is for.
  @param  Lba                    The starting logical block address to be written.
  @param  BufferSize             The size of the Buffer in bytes.
                                 This must be a multiple of the intrinsic block size of the device.
  @param  Buffer                 Pointer to the source buffer for the data.

  @retval EFI_SUCCESS            The data were written correctly to the device.
  @retval EFI_WRITE_PROTECTED    The device cannot be written to.
  @retval EFI_NO_MEDIA           There is no media in the device.
  @retval EFI_MEDIA_CHANGED      The MediaId is not for the current media.
  @retval EFI_DEVICE_ERROR       The device reported an error while attempting to perform the write operation.
  @retval EFI_BAD_BUFFER_SIZE    The BufferSize parameter is not a multiple of the intrinsic
                                 block size of the device.
  @retval EFI_INVALID_PARAMETER  The write request contains LBAs that are not valid,
                                 or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
BlockIoWriteBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 MediaId,
  IN EFI_LBA                Lba,
  IN UINTN                  BufferSize,
  IN VOID                   *Buffer
  )
{
  LOG_TRACE ("BlockIoWriteBlocks()");

  return IoBlocks (This, SdTransferDirectionWrite, MediaId, Lba, BufferSize, Buffer);
}

/**
  Flushes all modified data to a physical block device.

  @param  This                   Indicates a pointer to the calling context.

  @retval EFI_SUCCESS            All outstanding data were written correctly to the device.
  @retval EFI_DEVICE_ERROR       The device reported an error while attempting to write data.
  @retval EFI_NO_MEDIA           There is no media in the device.

**/
EFI_STATUS
EFIAPI
BlockIoFlushBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This
  )
{
  LOG_TRACE ("BlockIoFlushBlocks()");

  return EFI_SUCCESS;
}
