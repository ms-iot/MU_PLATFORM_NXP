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

#ifndef __SDMMC_H__
#define __SDMMC_H__

// Define with non-zero to collect IO statistics and dump it to the terminal.
#define SDMMC_COLLECT_STATISTICS  0

// Define with non-zero to benchmark IO on the first MmcReadBlocks call and
// dump to the terminal.
#define SDMMC_BENCHMARK_IO        0

// Lower bound of 2s poll wait time (200 x 10ms)
#define SDMMC_POLL_WAIT_COUNT     200
#define SDMMC_POLL_WAIT_TIME_US   10000

// The period at which to check the presence state of each card on each
// registered SDHC instance.
#define SDMMC_CHECK_CARD_INTERVAL_MS 1000

// The number of recursive error recoveries to reach before considering the
// failure fatal, and not attempting more error recoveries.
#define SDMMC_ERROR_RECOVERY_ATTEMPT_THRESHOLD    3

// Logging Macros

#define LOG_TRACE_FMT_HELPER(FMT, ...)  "SdMmc[T]:" FMT "%a\n", __VA_ARGS__

#define LOG_INFO_FMT_HELPER(FMT, ...)   "SdMmc[I]:" FMT "%a\n", __VA_ARGS__

#define LOG_ERROR_FMT_HELPER(FMT, ...) \
  "SdMmc[E]:" FMT " (%a: %a, %d)\n", __VA_ARGS__

#define LOG_INFO(...) \
  DEBUG((DEBUG_INIT, LOG_INFO_FMT_HELPER(__VA_ARGS__, "")))

#define LOG_VANILLA_TRACE(...) \
  DEBUG((DEBUG_VERBOSE | DEBUG_BLKIO, __VA_ARGS__))

#define LOG_TRACE(...) \
  DEBUG((DEBUG_VERBOSE | DEBUG_BLKIO, LOG_TRACE_FMT_HELPER(__VA_ARGS__, "")))

#define LOG_ERROR(...) \
  DEBUG((DEBUG_ERROR, LOG_ERROR_FMT_HELPER(__VA_ARGS__, __FUNCTION__, __FILE__, __LINE__)))

#define LOG_ASSERT(TXT) ASSERT(!"SdMmc[A]: " TXT "\n")

#ifndef C_ASSERT
#define C_ASSERT(e) _Static_assert(e, #e)
#endif // C_ASSERT

// Perform Integer division DIVIDEND/DIVISOR and return the result rounded up
// or down to the nearest integer, where 3.5 and 3.75 are near 4, while 3.25
// is near 3.
#define INT_DIV_ROUND(DIVIDEND, DIVISOR) \
  (((DIVIDEND) + ((DIVISOR) / 2)) / (DIVISOR))

typedef struct {
  UINT16  NumBlocks;
  UINT16  Count;
  UINT32  TotalTransferTimeUs;
} IoReadStatsEntry;

// Device Path Definitions
//
// eMMC and SD device paths got introduced in UEFI 2.6
// Remove definitions below once code base migrates to UEFI 2.6

#ifndef MSG_SD_DP
// SD (Secure Digital) Device Path SubType.
#define MSG_SD_DP   0x1A

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL  Header;
  UINT8                     SlotNumber;
} SD_DEVICE_PATH;
#endif // MSG_SD_DP

#ifndef MSG_EMMC_DP
// EMMC (Embedded MMC) Device Path SubType.
#define MSG_EMMC_DP   0x1D

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL  Header;
  UINT8                     SlotNumber;
} EMMC_DEVICE_PATH;
#endif // MSG_EMMC_DP

typedef struct {
  VENDOR_DEVICE_PATH  SdhcNode;
  UINT32              SdhcId;
  union {
    EMMC_DEVICE_PATH  MMC;
    SD_DEVICE_PATH    SD;
  } SlotNode;
  EFI_DEVICE_PATH     EndNode;
} SDHC_DEVICE_PATH;

// GUID {AAFB8DAA-7340-43AC-8D49-0CCE14812489}
#define SDHC_DEVICE_PATH_GUID \
  { 0xaafb8daa, 0x7340, 0x43ac, { 0x8d, 0x49, 0xc, 0xce, 0x14, 0x81, 0x24, 0x89 } }

// Size of SDHC node including the Sdhc ID field
#define SDHC_NODE_PATH_LENGTH (sizeof(VENDOR_DEVICE_PATH) + sizeof(UINT32))

typedef struct {
  UINTN                         Signature;
  LIST_ENTRY                    Link;
  UINT32                        InstanceId;
  EFI_HANDLE                    MmcHandle;
  BOOLEAN                       SlotInitialized;
  BOOLEAN                       Disabled;
  SDHC_DEVICE_PATH              DevicePath;
  EFI_BLOCK_IO_PROTOCOL         BlockIo;
  EFI_RPMB_IO_PROTOCOL          RpmbIo;
  EFI_SDHC_PROTOCOL             *HostExt;
  SDHC_CAPABILITIES             HostCapabilities;
  BOOLEAN                       DevicePathProtocolInstalled;
  BOOLEAN                       BlockIoProtocolInstalled;
  BOOLEAN                       RpmbIoProtocolInstalled;
  MMC_EXT_CSD_PARTITION_ACCESS  CurrentMmcPartition;
  CARD_INFO                     CardInfo;
  UINT32                        BlockBuffer[SD_BLOCK_WORD_COUNT];
  UINT32                        CmdResponse[4];
  CONST SD_COMMAND              *PreLastSuccessfulCmd;
  CONST SD_COMMAND              *LastSuccessfulCmd;
  UINT32                        ErrorRecoveryAttemptCount;
#ifdef MMC_COLLECT_STATISTICS
  IoReadStatsEntry              IoReadStats[1024];
  UINT32                        IoReadStatsNumEntries;
#endif // COLLECT_IO_STATISTICS
} SDHC_INSTANCE;

#define SDHC_INSTANCE_SIGNATURE   SIGNATURE_32('s', 'd', 'h', 'c')
#define SDHC_INSTANCE_FROM_BLOCK_IO_THIS(a) \
  CR(a, SDHC_INSTANCE, BlockIo, SDHC_INSTANCE_SIGNATURE)
#define SDHC_INSTANCE_FROM_LINK(a) \
  CR(a, SDHC_INSTANCE, Link, SDHC_INSTANCE_SIGNATURE)
#define SDHC_INSTANCE_FROM_RPMB_IO_THIS(a) \
  CR (a, SDHC_INSTANCE, RpmbIo, SDHC_INSTANCE_SIGNATURE)

// The ARM high-performance counter frequency
extern UINT64 gHpcTicksPerSeconds;

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
  );

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
  );

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
  );

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
  IN EFI_BLOCK_IO_PROTOCOL *This
  );

// EFI_RPMPB_IO Protocol Callbacks

/** Authentication key programming request.

  @param[in] This Indicates a pointer to the calling context.
  @param[in] ProgrammingRequest A data packet describing a key programming request.
  @param[out] ResultResponse A caller allocated data packet which will receive the
  key programming result.

  @retval EFI_SUCCESS RPMB communication sequence with the eMMC succeeded
  according to specs, other values are returned in case of any protocol error.
  Failure during key programming any other eMMC internal failure is reported
  in the Result field of the returned response data packet.
**/
EFI_STATUS
EFIAPI
RpmbIoProgramKey (
  IN EFI_RPMB_IO_PROTOCOL   *This,
  IN EFI_RPMB_DATA_PACKET   *Request,
  OUT EFI_RPMB_DATA_PACKET  *ResultResponse
  );

/** Reading of the write counter value request.

  @param[in] This Indicates a pointer to the calling context.
  @param[in] ReadRequest A data packet describing a read counter value request.
  @param[out] ReadResponse A caller allocated data packet which will receive
  the counter value read response. If counter has expired bit 7 is set to 1 in
  returned Result.

  @retval EFI_SUCCESS RPMB communication sequence with the eMMC succeeded
  according to specs, other values are returned in case of any protocol error.
  Failure during counter read or any other eMMC internal failure is reported
  in the Result field of the returned response data packet.
**/
EFI_STATUS
EFIAPI
RpmbIoReadCounter (
  IN EFI_RPMB_IO_PROTOCOL   *This,
  IN EFI_RPMB_DATA_PACKET   *ReadRequest,
  OUT EFI_RPMB_DATA_PACKET  *ReadResponse
  );

/** Authenticated data write request.

  @param[in] This Indicates a pointer to the calling context.
  @param[in] WriteRequest A sequence of data packets describing data write
  requests and holds the data to be written.
  @param[out] ResultResponse A caller allocated data packet which will receive
  the data write programming result.

  @retval EFI_SUCCESS RPMB communication sequence with the eMMC succeeded
  according to specs, other values are returned in case of any protocol error.
  Failure during data programming or any other eMMC internal failure is reported
  in the Result field of the returned data packet.
**/
EFI_STATUS
EFIAPI
RpmbIoAuthenticatedWrite (
  IN EFI_RPMB_IO_PROTOCOL   *This,
  IN EFI_RPMB_DATA_BUFFER   *WriteRequest,
  OUT EFI_RPMB_DATA_PACKET  *ResultResponse
  );

/** Authenticated data read request.

  @param[in] This Indicates a pointer to the calling context.
  @param[in] ReadRequest A data packet that describes a data read request.
  @param[out] ReadResponse A caller allocated data packets which will receive
  the data read.

  @retval EFI_SUCCESS RPMB communication sequence with the eMMC succeeded
  according to specs, other values are returned in case of any protocol error.
  Failure during data fetch from the eMMC or any other eMMC internal failure
  is reported in the Result field of the returned data packet.
**/
EFI_STATUS
EFIAPI
RpmbIoAuthenticatedRead (
  IN EFI_RPMB_IO_PROTOCOL   *This,
  IN EFI_RPMB_DATA_PACKET   *ReadRequest,
  OUT EFI_RPMB_DATA_BUFFER  *ReadResponse
  );

// Helper Functions

EFI_STATUS
EFIAPI
SoftReset (
  IN SDHC_INSTANCE  *HostInst
  );

// Debugging Helpers

VOID
PrintCsd (
  IN SDHC_INSTANCE  *HostInst
  );

VOID
PrintCid (
  IN SDHC_INSTANCE  *HostInst
  );

VOID
PrintCardStatus (
  IN SDHC_INSTANCE  *HostInst,
  IN CARD_STATUS    CardStatus
  );

VOID
GetAndPrintCardStatus (
  IN SDHC_INSTANCE  *HostInst
  );

// Inlined Helper Functions

__inline__
static
CONST CHAR8*
MmcPartitionAccessToString (
  IN MMC_EXT_CSD_PARTITION_ACCESS   Partition
  )
{
  switch (Partition) {
  case MmcExtCsdPartitionAccessUserArea:
    return "UserArea";
  case MmcExtCsdPartitionAccessBootPartition1:
    return "Boot1";
  case MmcExtCsdPartitionAccessBootPartition2:
    return "Boot2";
  case MmcExtCsdPartitionAccessRpmb:
    return "RPMB";
  case MmcExtCsdPartitionAccessGpp1:
    return "GeneralPurpose1";
  case MmcExtCsdPartitionAccessGpp2:
    return "GeneralPurpose2";
  case MmcExtCsdPartitionAccessGpp3:
    return "GeneralPurpose3";
  case MmcExtCsdPartitionAccessGpp4:
    return "GeneralPurpose4";
  default:
    return "Unknown";
  }
}

__inline__
static
CONST CHAR8*
CardStateToString (
  IN CARD_STATE   State
  )
{
  switch (State) {
  case CardStateIdle:
    return "Idle";
  case CardStateReady:
    return "Ready";
  case CardStateIdent:
    return "Ident";
  case CardStateStdby:
    return "Stdby";
  case CardStateTran:
    return "Tran";
  case CardStateData:
    return "Data";
  case CardStateRcv:
    return "Rcv";
  case CardStatePrg:
    return "Prg";
  case CardStateDis:
    return "Dis";
  case CardStateBtst:
    return "Btst";
  case CardStateSlp:
    return "Slp";
  default:
    return "Reserved";
  }
}

__inline__
static
BOOLEAN
IsCardStatusError (
  IN SDHC_INSTANCE  *HostInst,
  IN CARD_STATUS    CardStatus
  )
{
  ASSERT (HostInst != NULL);

  switch (HostInst->CardInfo.CardFunction) {
  case CardFunctionSd:
    return CardStatus.AsUint32 & SD_CARD_STATUS_ERROR_MASK;
  case CardFunctionMmc:
    return CardStatus.AsUint32 & MMC_CARD_STATUS_ERROR_MASK;
  default:
    ASSERT (FALSE);
    return FALSE;
  }
}

__inline__
static
BOOLEAN
CmdsAreEqual (
  IN CONST SD_COMMAND   *Left,
  IN CONST SD_COMMAND   *Right
  )
{
  ASSERT (Left != NULL);
  ASSERT (Right != NULL);

  if (Left != Right) {
    return (Left->Class == Right->Class) &&
      (Left->Index == Right->Index) &&
      (Left->ResponseType == Right->ResponseType) &&
      (Left->TransferDirection == Right->TransferDirection) &&
      (Left->TransferType == Right->TransferType) &&
      (Left->Type == Right->Type);
  }

  return TRUE;
}

// Timing Helpers

/** Gets the current high-performance counter tick count.

  The returned tick count is considered as a time stamp and can used later in
  elapsed time calculations.

  @retval Current high-performance counter tick count.
**/
__inline__
static
UINT64
HpcTimerStart (
  VOID
  )
{
  return GetPerformanceCounter ();
}

/** Calculates the elapsed milliseconds since a specific timer time stamp.

  @param[in] TimerStartTimestamp The high-performance counter tick count to use
  as the starting point in elapsed time calculation.

  @retval The milliseconds elapsed.
**/
__inline__
static
UINT64
HpcTimerElapsedMilliseconds (
  IN UINT64   TimerStartTimestamp
  )
{
  return (((GetPerformanceCounter () - TimerStartTimestamp) * 1000UL) /
          gHpcTicksPerSeconds);
}

#endif // __SDMMC_H__
