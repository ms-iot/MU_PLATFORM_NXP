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

EFI_STATUS
InitializeDevice (
  IN SDHC_INSTANCE  *HostInst
  )
{
  LOG_TRACE ("InitializDevice()");
  ASSERT (!HostInst->SlotInitialized);

#if SDMMC_BENCHMARK_IO
  UINT64 InitializationStartTime = HpcTimerStart ();
#endif // SDMMC_BENCHMARK_IO

  EFI_SDHC_PROTOCOL *HostExt = HostInst->HostExt;
  ASSERT (HostExt);
  EFI_STATUS Status;

  ASSERT (HostInst->BlockIo.Media->MediaPresent);

  ZeroMem (&HostInst->CardInfo, sizeof (HostInst->CardInfo));
  //
  // SD/MMC cards on reset start in default normal speed mode
  //
  HostInst->CardInfo.CurrentSpeedMode = CardSpeedModeNormalSpeed;

  Status = HostExt->SoftwareReset (HostExt, SdhcResetTypeAll);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("HostExt->SoftwareReset() failed. %r", Status);
    return Status;
  }

  Status = HostExt->SetClock (HostExt, SD_IDENT_MODE_CLOCK_FREQ_HZ);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("HostExt->SetClock() failed. %r", Status);
    return Status;
  }

  Status = SdhcQueryCardType (HostInst);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("SdhcQueryCardType() failed. %r", Status);
    return Status;
  }

  switch (HostInst->CardInfo.CardFunction) {
  case CardFunctionComboSdSdio:
    LOG_ERROR ("Combo SD/SDIO function is not supported");
    return EFI_UNSUPPORTED;
  case CardFunctionSdio:
    LOG_ERROR ("SDIO function is not supported");
    return EFI_UNSUPPORTED;
  case CardFunctionSd:
    Status = InitializeSdDevice (HostInst);
    if (EFI_ERROR (Status)) {
      LOG_ERROR ("InitializeSdDevice() failed. %r", Status);
      return Status;
    }
    break;
  case CardFunctionMmc:
    Status = InitializeMmcDevice (HostInst);
    if (EFI_ERROR (Status)) {
      LOG_ERROR ("InitializeMmcDevice() failed. %r", Status);
      return Status;
    }
    break;
  default:
    LOG_ASSERT ("Unknown device function");
    return EFI_UNSUPPORTED;
  }

  PrintCid (HostInst);
  PrintCsd (HostInst);

  Status = SdhcSetBlockLength (HostInst, SD_BLOCK_LENGTH_BYTES);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("SdhcSetBlockLength() failed. %r", Status);
    return Status;
  }

  ASSERT (HostInst->CardInfo.CardFunction == CardFunctionSd ||
          HostInst->CardInfo.CardFunction == CardFunctionMmc);

  if (HostInst->CardInfo.CardFunction == CardFunctionSd) {
    HostInst->BlockIo.Media->RemovableMedia = TRUE;
    HostInst->BlockIo.Media->MediaId = HostInst->CardInfo.Registers.Sd.Cid.PSN;
  } else {
    //
    // Assume BGA form factor eMMC
    //
    HostInst->BlockIo.Media->RemovableMedia = FALSE;

    //
    // Use the card product serial number as MediaId
    //
    HostInst->BlockIo.Media->MediaId = HostInst->CardInfo.Registers.Mmc.Cid.PSN;
  }

  HostInst->SlotInitialized = TRUE;

#if SDMMC_BENCHMARK_IO
  UINT64 ElapsedTimeMs = HpcTimerElapsedMilliseconds (InitializationStartTime);
  LOG_INFO ("Initialization completed in %ldms", ElapsedTimeMs);
#endif // SDMMC_BENCHMARK_IO

  return EFI_SUCCESS;
}

EFI_STATUS
InitializeSdDevice (
  IN SDHC_INSTANCE  *HostInst
  )
{
  EFI_STATUS Status;

  Status = SdhcSendOpCondSd (HostInst);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("SdhcSendOpCondSd() failed. %r", Status);
    return Status;
  }

  Status = SdhcSendCidAll (HostInst);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("SdhcSendCidAll() failed. %r", Status);
    return Status;
  }

  Status = SdhcSendRelativeAddr (HostInst);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("SdhcSendRelativeAddr() failed. %r", Status);
    return Status;
  }

  Status = SdhcSendCid (HostInst);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("SdhcSendCid() failed. %r", Status);
    return Status;
  }

  Status = SdhcSendCsd (HostInst);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("SdhcSendCsd() failed. %r", Status);
    return Status;
  }

  Status = SdhcSelectDevice (HostInst);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("SdhcSelectDevice() failed. %r", Status);
    return Status;
  }

  Status = SdhcSwitchBusWidthSd (HostInst);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("SdhcSwitchBusWidthSd() failed. %r", Status);
    return Status;
  }

  Status = SdhcSwitchSpeedModeSd (HostInst);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("SdhcSwitchSpeedModeSd() failed. %r", Status);
    return Status;
  }

  Status = SdhcSetMaxClockFrequency (HostInst);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("SdhcSetMaxClockFrequency() failed. %r", Status);
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
InitializeMmcDevice (
  IN SDHC_INSTANCE  *HostInst
  )
{
  EFI_STATUS Status;

  Status = SdhcSendCidAll (HostInst);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("SdhcSendCidAll() failed. %r", Status);
    return Status;
  }

  Status = SdhcSendRelativeAddr (HostInst);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("SdhcSendRelativeAddr() failed. %r", Status);
    return Status;
  }

  Status = SdhcSendCid (HostInst);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("SdhcSendCid() failed. %r", Status);
    return Status;
  }

  Status = SdhcSendCsd (HostInst);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("SdhcSendCsd() failed. %r", Status);
    return Status;
  }

  if (HostInst->CardInfo.Registers.Mmc.Csd.SPEC_VERS < MMC_MIN_SUPPORTED_SPEC_VERS) {
    LOG_ERROR (
      "MMC %d.x is not supported, Minimum supported is MMC %d.x",
      HostInst->CardInfo.Registers.Mmc.Csd.SPEC_VERS,
      MMC_MIN_SUPPORTED_SPEC_VERS);
    return EFI_UNSUPPORTED;
  }

  Status = SdhcSelectDevice (HostInst);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("SdhcSelectDevice() failed. %r", Status);
    return Status;
  }

  Status = SdhcSendExtCsdMmc (HostInst);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("SdhcSendExtCsdMmc() failed. %r", Status);
    return Status;
  }

  Status = SdhcSwitchSpeedModeMmc (HostInst);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("SdhcSwitchSpeedModeMmc() failed. %r", Status);
    return Status;
  }

  Status = SdhcSwitchBusWidthMmc (HostInst);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("SdhcSwitchBusWidthMmc() failed. %r", Status);
    return Status;
  }

  Status = SdhcSetMaxClockFrequency (HostInst);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("SdhcSetMaxClockFrequency() failed. %r", Status);
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
SdhcRecoverFromErrors (
  IN SDHC_INSTANCE      *HostInst,
  IN CONST SD_COMMAND   *Cmd
  )
{
  EFI_STATUS Status;
  CARD_STATUS CardStatus;
  EFI_SDHC_PROTOCOL *HostExt;

  LOG_TRACE (
    "*** %cCMD%d Error recovery sequence start ***",
    (Cmd->Class == SdCommandClassApp ? 'A' : ' '),
    (UINT32) Cmd->Index);

  HostExt = HostInst->HostExt;
  HostInst->ErrorRecoveryAttemptCount += 1;

  if (HostInst->ErrorRecoveryAttemptCount > SDMMC_ERROR_RECOVERY_ATTEMPT_THRESHOLD) {
    LOG_ERROR ("RecursiveErrorRecoveryCount exceeded the threshold. Error is unrecoverable!");
    Status = EFI_PROTOCOL_ERROR;
    goto Exit;
  }

  GetAndPrintCardStatus (HostInst);

  LOG_TRACE ("Reseting CMD line ...");
  Status = HostExt->SoftwareReset (HostExt, SdhcResetTypeCmd);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("HostExt->SoftwareReset(SdhcResetTypeCmd) failed. %r", Status);
  }

  if (Cmd->TransferType != SdTransferTypeNone &&
      Cmd->TransferType != SdTransferTypeUndefined) {

    LOG_TRACE ("Reseting DAT line ...");
    Status = HostExt->SoftwareReset (HostExt, SdhcResetTypeData);
    if (EFI_ERROR (Status)) {
      LOG_ERROR ("HostExt->SoftwareReset(SdhcResetTypeData) failed. %r", Status);
    }
  }

  if (EFI_ERROR (Status)) {
    LOG_TRACE ("CMD and/or DATA normal error recovery failed, trying SDHC soft-reset");
    Status = SoftReset (HostInst);
    if (EFI_ERROR (Status)) {
      LOG_ERROR ("All trials to recover from errors failed. %r");
      goto Exit;
    }
  }

  if (CmdsAreEqual (Cmd, &CmdWriteMultiBlock) ||
      CmdsAreEqual (Cmd, &CmdReadMultiBlock)) {

    Status = SdhcSendStatus (HostInst, &CardStatus);
    if (EFI_ERROR (Status)) {
      LOG_ERROR ("SdhcSendStatus() failed. (Status = %r)", Status);
      goto Exit;
    }

    if (CardStatus.Fields.CURRENT_STATE != CardStateTran) {
      LOG_TRACE ("Stopping transmission ...");
      Status = SdhcStopTransmission (HostInst);
      if (EFI_ERROR (Status)) {
        goto Exit;
      }

      //
      // Multi read/write failure reason will be written in the STOP_TRANSMISSION
      // response as part of the card status error flags.
      //
      CardStatus.AsUint32 = HostInst->CmdResponse[0];
      PrintCardStatus (HostInst, CardStatus);
    }
  }

Exit:

  if (EFI_ERROR (Status)) {
    LOG_ERROR (
      "*** %cCMD%d Error recovery sequence failed "
      "(Status = %r, ErrorRecoveryAttemptCount = %d) ***",
      (Cmd->Class == SdCommandClassApp ? 'A' : ' '),
      (UINT32) Cmd->Index,
      Status,
      HostInst->ErrorRecoveryAttemptCount);

  } else {
    LOG_TRACE (
      "*** %cCMD%d Error recovery sequence completed successfully "
      "(ErrorRecoveryAttemptCount = %d) ***",
      (Cmd->Class == SdCommandClassApp ? 'A' : ' '),
      (UINT32) Cmd->Index,
      HostInst->ErrorRecoveryAttemptCount);
  }

  HostInst->ErrorRecoveryAttemptCount -= 1;

  return Status;
}

EFI_STATUS
SdhcSendCommandHelper (
  IN SDHC_INSTANCE        *HostInst,
  IN CONST SD_COMMAND     *Cmd,
  IN UINT32               Arg,
  IN SD_COMMAND_XFR_INFO  *XfrInfo
  )
{
  EFI_SDHC_PROTOCOL *HostExt = HostInst->HostExt;
  EFI_STATUS Status;
  CARD_STATUS CardStatus;

  if (Cmd->Class == SdCommandClassApp) {
    UINT32 CmdAppArg = HostInst->CardInfo.RCA << 16;
    Status = HostExt->SendCommand (HostExt, &CmdAppSd, CmdAppArg, NULL);
    if (EFI_ERROR (Status)) {
      LOG_ERROR (
        "HostExt->SendCommand(%cCMD%d, 0x%08x) failed. %r",
        (Cmd->Class == SdCommandClassApp ? 'A' : ' '),
        (UINT32) Cmd->Index,
        CmdAppArg,
        Status);

      return Status;
    }
  }

  Status = HostExt->SendCommand (HostExt, Cmd, Arg, XfrInfo);
  if (EFI_ERROR (Status)) {
    LOG_ERROR (
      "HostExt->SendCommand(%cCMD%d, 0x%08x) failed. %r",
      (Cmd->Class == SdCommandClassApp ? 'A' : ' '),
      (UINT32) Cmd->Index,
      Arg,
      Status);

    return Status;
  }

  Status = HostExt->ReceiveResponse (HostExt, Cmd, HostInst->CmdResponse);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("HostExt->ReceiveResponse() failed. %r", Status);
    return Status;
  }

  if ((Cmd->ResponseType == SdResponseTypeR1) ||
    (Cmd->ResponseType == SdResponseTypeR1B)) {

    CardStatus.AsUint32 = HostInst->CmdResponse[0];

    if (IsCardStatusError (HostInst, CardStatus)) {
      LOG_ERROR (
        "%cCMD%d card status error detected",
        (Cmd->Class == SdCommandClassApp ? 'A' : ' '),
        (UINT32) Cmd->Index);

      PrintCardStatus (HostInst, CardStatus);

      return EFI_DEVICE_ERROR;
    }
  }

  HostInst->PreLastSuccessfulCmd = HostInst->LastSuccessfulCmd;
  HostInst->LastSuccessfulCmd = Cmd;

  return EFI_SUCCESS;
}

EFI_STATUS
SdhcSendCommand (
  IN SDHC_INSTANCE      *HostInst,
  IN CONST SD_COMMAND   *Cmd,
  IN UINT32             Arg
  )
{
  EFI_STATUS Status;

  Status = SdhcSendCommandHelper (HostInst, Cmd, Arg, NULL);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("Send no-data command failed. %r", Status);
    SdhcRecoverFromErrors (HostInst, Cmd);
    return Status;
  }

  //
  // SWITCH command can change card state to prog, we should wait the card to
  // transfer back to tran state and rais the READY_FOR_DATA flag to make sure
  // that switch operation was completed successfully
  //
  if (CmdsAreEqual (Cmd, &CmdSwitchMmc) ||
      CmdsAreEqual (Cmd, &CmdSwitchSd)) {
    Status = SdhcWaitForTranStateAndReadyForData (HostInst);
    if (EFI_ERROR (Status)) {
      LOG_ERROR (
        "SdhcWaitForTranStateAndReadyForData() failed after successful "
        "SWITCH command. (Status = %r)",
        Status);
      return Status;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
SdhcSendDataCommand (
  IN SDHC_INSTANCE      *HostInst,
  IN CONST SD_COMMAND   *Cmd,
  IN UINT32             Arg,
  IN UINT32             BufferByteSize,
  IN VOID               *Buffer
  )
{
  EFI_SDHC_PROTOCOL *HostExt = HostInst->HostExt;
  SD_COMMAND_XFR_INFO XfrInfo;
  EFI_STATUS Status;

  ASSERT (BufferByteSize % SD_BLOCK_LENGTH_BYTES == 0);
  XfrInfo.BlockCount = BufferByteSize / SD_BLOCK_LENGTH_BYTES;
  XfrInfo.BlockSize = SD_BLOCK_LENGTH_BYTES;
  XfrInfo.Buffer = Buffer;

  Status = SdhcSendCommandHelper (HostInst, Cmd, Arg, &XfrInfo);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  if (Cmd->TransferDirection == SdTransferDirectionRead) {
    Status = HostExt->ReadBlockData (
      HostExt,
      BufferByteSize,
      (UINT32*) Buffer);
    if (EFI_ERROR (Status)) {
      LOG_ERROR (
        "HostExt->ReadBlockData(Size: 0x%xB) failed. %r",
        BufferByteSize,
        Status);
      goto Exit;
    }
  } else {
    ASSERT (Cmd->TransferDirection == SdTransferDirectionWrite);
    Status = HostExt->WriteBlockData (
      HostExt,
      BufferByteSize,
      (UINT32*) Buffer);
    if (EFI_ERROR (Status)) {
      LOG_ERROR (
        "HostExt->WriteBlockData(Size: 0x%xB) failed. %r",
        BufferByteSize,
        Status);
      goto Exit;
    }
  }

  //
  // If this is an open-ended multi-block read/write then explicitly send
  // STOP_TRANSMISSION. A multi-block read/write with pre-defined block count
  // will be preceeded with SET_BLOCK_COUNT.
  //
  if ((CmdsAreEqual (Cmd, &CmdWriteMultiBlock) ||
       CmdsAreEqual (Cmd, &CmdReadMultiBlock)) &&
       ((HostInst->LastSuccessfulCmd == NULL) ||
        !CmdsAreEqual (HostInst->PreLastSuccessfulCmd, &CmdSetBlockCount))) {

    Status = SdhcStopTransmission (HostInst);
    if (EFI_ERROR (Status)) {
      LOG_ERROR ("SdhcStopTransmission() failed. (Status = %r)", Status);
      goto Exit;
    }
  }

  Status = SdhcWaitForTranStateAndReadyForData (HostInst);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

Exit:

  if (EFI_ERROR (Status)) {
    LOG_ERROR ("Send data command failed. %r", Status);
    SdhcRecoverFromErrors (HostInst, Cmd);
  }

  return Status;
}

EFI_STATUS
SdhcGoIdleState (
  IN SDHC_INSTANCE  *HostInst
  )
{
  return SdhcSendCommand (HostInst, &CmdGoIdleState, 0);
}

EFI_STATUS
SdhcQueryCardType (
  IN SDHC_INSTANCE  *HostInst
  )
{
  EFI_STATUS Status = SdhcGoIdleState (HostInst);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  HostInst->CardInfo.CardFunction = CardFunctionUnknown;

  Status = SdhcSendIfCondSd (HostInst);
  if (!EFI_ERROR (Status)) {
    HostInst->CardInfo.CardFunction = CardFunctionSd;
  }

  Status = SdhcSendOpCondSdio (HostInst);
  if (!EFI_ERROR (Status)) {
    if (HostInst->CardInfo.CardFunction == CardFunctionSd) {
      //
      // SD/SDIO Combo Device
      //
      HostInst->CardInfo.CardFunction = CardFunctionComboSdSdio;
    } else {
      HostInst->CardInfo.CardFunction = CardFunctionSdio;
    }
  }

  if (HostInst->CardInfo.CardFunction != CardFunctionSd &&
      HostInst->CardInfo.CardFunction != CardFunctionSdio &&
      HostInst->CardInfo.CardFunction != CardFunctionComboSdSdio) {
    Status = SdhcGoIdleState (HostInst);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = SdhcSendOpCondMmc (HostInst);
    if (!EFI_ERROR (Status)) {
      HostInst->CardInfo.CardFunction = CardFunctionMmc;
    }
  }

  if (HostInst->CardInfo.CardFunction == CardFunctionUnknown) {
    return EFI_NO_MEDIA;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
SdhcWaitForTranStateAndReadyForData (
  IN SDHC_INSTANCE  *HostInst
  )
{
  EFI_STATUS Status;
  UINT32 Retry;
  CARD_STATUS CardStatus;

  Status = SdhcSendStatus (HostInst, &CardStatus);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Retry = SDMMC_POLL_WAIT_COUNT;

  while (((!CardStatus.Fields.READY_FOR_DATA && Retry) ||
    (CardStatus.Fields.CURRENT_STATE != CardStateTran)) &&
         Retry) {

    gBS->Stall (SDMMC_POLL_WAIT_TIME_US);
    --Retry;

    Status = SdhcSendStatus (HostInst, &CardStatus);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if (!Retry) {
    LOG_ERROR ("Time-out waiting for card READY_FOR_DATA status flag");
    PrintCardStatus (HostInst, CardStatus);
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
SdhcSendCid (
  IN SDHC_INSTANCE  *HostInst
  )
{
  UINT32 CmdArg = HostInst->CardInfo.RCA << 16;

  EFI_STATUS Status = SdhcSendCommand (HostInst, &CmdSendCid, CmdArg);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (HostInst->CardInfo.CardFunction == CardFunctionSd) {
    gBS->CopyMem (
      (VOID*) &HostInst->CardInfo.Registers.Sd.Cid,
      (VOID*) HostInst->CmdResponse,
      sizeof (SD_CID));

  } else if (HostInst->CardInfo.CardFunction == CardFunctionMmc) {
    gBS->CopyMem (
      (VOID*) &HostInst->CardInfo.Registers.Mmc.Cid,
      (VOID*) HostInst->CmdResponse,
      sizeof (MMC_CID));

    C_ASSERT (sizeof (HostInst->RpmbIo.Cid) == EFI_RPMB_CID_SIZE);
    gBS->CopyMem (
      (VOID*) &HostInst->RpmbIo.Cid,
      (VOID*) HostInst->CmdResponse,
      EFI_RPMB_CID_SIZE);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
SdhcSendCsd (
  IN SDHC_INSTANCE  *HostInst
  )
{
  UINT32 CmdArg = HostInst->CardInfo.RCA << 16;

  EFI_STATUS Status = SdhcSendCommand (HostInst, &CmdSendCsd, CmdArg);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  UINT32 MaxBlockLen;
  UINT64 ByteCapacity = 0;

  if (HostInst->CardInfo.CardFunction == CardFunctionSd) {
    gBS->CopyMem (
      (VOID*) &HostInst->CardInfo.Registers.Sd.Csd,
      (VOID*) HostInst->CmdResponse,
      sizeof (SD_CSD));
    if (HostInst->CardInfo.Registers.Sd.Csd.CSD_STRUCTURE == 0) {
      SD_CSD* Csd = (SD_CSD*) &HostInst->CardInfo.Registers.Sd.Csd;
      UINT32 DeviceSize = ((Csd->C_SIZEHigh10 << 2) | Csd->C_SIZELow2);
      UINT32 MULT = 1 << (Csd->C_SIZE_MULT + 2);
      UINT32 BLOCKNR = (DeviceSize + 1) * MULT;
      MaxBlockLen = 1 << Csd->READ_BL_LEN;
      ByteCapacity = BLOCKNR * MaxBlockLen;
    } else {
      SD_CSD_2* Csd2 = (SD_CSD_2*) &HostInst->CardInfo.Registers.Sd.Csd;
      MaxBlockLen = 1 << Csd2->READ_BL_LEN;
      ByteCapacity = (UINT64) (Csd2->C_SIZE + 1) * 512llu * 1024llu;
    }
  } else if (HostInst->CardInfo.CardFunction == CardFunctionMmc) {
    gBS->CopyMem (
      (VOID*) &HostInst->CardInfo.Registers.Mmc.Csd,
      (VOID*) HostInst->CmdResponse,
      sizeof (MMC_CSD));
    //
    // HighCapacity MMC requires reading EXT_CSD to calculate capacity
    //
    if (!HostInst->CardInfo.HighCapacity) {
      MMC_CSD* Csd = (MMC_CSD*) &HostInst->CardInfo.Registers.Mmc.Csd;
      UINT32 DeviceSize = ((Csd->C_SIZEHigh10 << 2) | Csd->C_SIZELow2);
      UINT32 MULT = 1 << (Csd->C_SIZE_MULT + 2);
      UINT32 BLOCKNR = (DeviceSize + 1) * MULT;
      MaxBlockLen = 1 << Csd->READ_BL_LEN;
      ByteCapacity = BLOCKNR * MaxBlockLen;
    }
  }

  HostInst->CardInfo.ByteCapacity = ByteCapacity;
  HostInst->BlockIo.Media->BlockSize = SD_BLOCK_LENGTH_BYTES;
  UINT64 NumBlocks = (ByteCapacity / SD_BLOCK_LENGTH_BYTES);

  if (NumBlocks > 0) {
    HostInst->BlockIo.Media->LastBlock = (NumBlocks - 1);
  } else {
    HostInst->BlockIo.Media->LastBlock = 0;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
SdhcSelectDevice (
  IN SDHC_INSTANCE  *HostInst
  )
{
  UINT32 CmdArg = HostInst->CardInfo.RCA << 16;
  return SdhcSendCommand (HostInst, &CmdSelect, CmdArg);
}

EFI_STATUS
SdhcDeselectDevice (
  IN SDHC_INSTANCE  *HostInst
  )
{
  return SdhcSendCommand (HostInst, &CmdDeselect, 0);
}

EFI_STATUS
SdhcSendAppCmd (
  IN SDHC_INSTANCE  *HostInst
  )
{
  UINT32 CmdArg = HostInst->CardInfo.RCA << 16;
  return SdhcSendCommand (HostInst, &CmdAppSd, CmdArg);
}

EFI_STATUS
SdhcStopTransmission (
  IN SDHC_INSTANCE  *HostInst
  )
{
  UINT32 CmdArg = HostInst->CardInfo.RCA << 16;
  return SdhcSendCommand (HostInst, &CmdStopTransmission, CmdArg);
}

EFI_STATUS
SdhcSendCidAll (
  IN SDHC_INSTANCE  *HostInst
  )
{
  return SdhcSendCommand (HostInst, &CmdSendCidAll, 0);
}

EFI_STATUS
SdhcSendRelativeAddr (
  IN SDHC_INSTANCE  *HostInst
  )
{
  UINT32 CmdArg = 0;

  //
  // Unlike SD memory, MMC cards don't publish their RCA, instead it should be
  // manually assigned by the SDHC
  //
  if (HostInst->CardInfo.CardFunction == CardFunctionMmc) {
    HostInst->CardInfo.RCA = 0xCCCC;
    CmdArg = HostInst->CardInfo.RCA << 16;
  }

  EFI_STATUS Status = SdhcSendCommand (HostInst, &CmdSendRelativeAddr, CmdArg);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (HostInst->CardInfo.CardFunction != CardFunctionMmc) {
    HostInst->CardInfo.RCA = (HostInst->CmdResponse[0] >> 16);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
CalculateCardMaxFreq (
  IN SDHC_INSTANCE *HostInst,
  OUT UINT32* MaxClkFreqHz
  )
{
  ASSERT (HostInst != NULL);
  ASSERT (MaxClkFreqHz != NULL);

  UINT32 TransferRateBitPerSecond = 0;
  UINT32 TimeValue = 0;
  UINT32 TRAN_SPEED;

  if (HostInst->CardInfo.CardFunction == CardFunctionSd) {
    TRAN_SPEED = HostInst->CardInfo.Registers.Sd.Csd.TRAN_SPEED;
  } else {
    TRAN_SPEED = HostInst->CardInfo.Registers.Mmc.Csd.TRAN_SPEED;
  }

  // Calculate Transfer rate unit (Bits 2:0 of TRAN_SPEED)
  switch (TRAN_SPEED & 0x7) { // 2
  case 0: // 100kbit/s
    TransferRateBitPerSecond = 100 * 1000;
    break;

  case 1: // 1Mbit/s
    TransferRateBitPerSecond = 1 * 1000 * 1000;
    break;

  case 2: // 10Mbit/s
    TransferRateBitPerSecond = 10 * 1000 * 1000;
    break;

  case 3: // 100Mbit/s
    TransferRateBitPerSecond = 100 * 1000 * 1000;
    break;

  default:
    LOG_ERROR ("Invalid parameter");
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  //Calculate Time value (Bits 6:3 of TRAN_SPEED)
  switch ((TRAN_SPEED >> 3) & 0xF) { // 6
  case 0x1:
    TimeValue = 10;
    break;

  case 0x2:
    TimeValue = 12;
    break;

  case 0x3:
    TimeValue = 13;
    break;

  case 0x4:
    TimeValue = 15;
    break;

  case 0x5:
    TimeValue = 20;
    break;

  case 0x6:
    if (HostInst->CardInfo.CardFunction == CardFunctionSd) {
      TimeValue = 25;
    } else {
      TimeValue = 26;
    }
    break;

  case 0x7:
    TimeValue = 30;
    break;

  case 0x8:
    TimeValue = 35;
    break;

  case 0x9:
    TimeValue = 40;
    break;

  case 0xA:
    TimeValue = 45;
    break;

  case 0xB:
    if (HostInst->CardInfo.CardFunction == CardFunctionSd) {
      TimeValue = 50;
    } else {
      TimeValue = 52;
    }
    break;

  case 0xC:
    TimeValue = 55;
    break;

  case 0xD:
    TimeValue = 60;
    break;

  case 0xE:
    TimeValue = 70;
    break;

  case 0xF:
    TimeValue = 80;
    break;

  default:
    LOG_ERROR ("Invalid parameter");
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  *MaxClkFreqHz = (TransferRateBitPerSecond * TimeValue) / 10;

  LOG_TRACE (
    "TransferRateUnitId=%d TimeValue*10=%d, CardFrequency=%dKHz",
    TRAN_SPEED & 0x7,
    TimeValue,
    *MaxClkFreqHz / 1000);

  return EFI_SUCCESS;
}

EFI_STATUS
SdhcSetMaxClockFrequency (
  IN SDHC_INSTANCE  *HostInst
  )
{
  EFI_SDHC_PROTOCOL *HostExt = HostInst->HostExt;
  EFI_STATUS Status;
  UINT32 MaxClkFreqHz = 0;

  //
  // Currently only NormalSpeed and HighSpeed supported
  //
  ASSERT (HostInst->CardInfo.CurrentSpeedMode == CardSpeedModeNormalSpeed ||
          HostInst->CardInfo.CurrentSpeedMode == CardSpeedModeHighSpeed);

  if (HostInst->CardInfo.CardFunction == CardFunctionSd) {
    Status = CalculateCardMaxFreq (HostInst, &MaxClkFreqHz);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } else {
    if (HostInst->CardInfo.CurrentSpeedMode == CardSpeedModeNormalSpeed) {
      Status = CalculateCardMaxFreq (HostInst, &MaxClkFreqHz);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    } else if (HostInst->CardInfo.CurrentSpeedMode == CardSpeedModeHighSpeed) {
      MaxClkFreqHz = MMC_HIGH_SPEED_MODE_CLOCK_FREQ_HZ;
    }
  }

  Status = HostExt->SetClock (HostExt, MaxClkFreqHz);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
SdhcSetBlockLength (
  IN SDHC_INSTANCE  *HostInst,
  IN UINT32         BlockByteLength
  )
{
  LOG_TRACE ("SdhcSetBlockLength(BlockByteLength=%d)", BlockByteLength);

  return SdhcSendCommand (HostInst, &CmdSetBlockLength, BlockByteLength);
}

EFI_STATUS
SdhcSetBlockCount (
  IN SDHC_INSTANCE  *HostInst,
  IN UINT32         BlockCount,
  IN BOOLEAN        ReliableWrite
  )
{
  UINT32 CmdArg;

  LOG_TRACE (
    "SdhcSetBlockCount(BlockCount=%d, ReliableWrite=%d)",
    BlockCount,
    ReliableWrite);

  //
  // JEDEC Standard No. 84-A441, Page 76
  //
  // Set bit[31] as 1 to indicate Reliable Write type of programming access.
  //

  if (ReliableWrite) {
    CmdArg = BlockCount | (1 << 31);
  } else {
    CmdArg = BlockCount;
  }

  return SdhcSendCommand (HostInst, &CmdSetBlockCount, CmdArg);
}

EFI_STATUS
SdhcSendStatus (
  IN SDHC_INSTANCE  *HostInst,
  OUT CARD_STATUS   *CardStatus
  )
{
  EFI_STATUS Status;
  UINT32 CmdArg;

  LOG_TRACE ("SdhcSendStatus()");

  CmdArg = HostInst->CardInfo.RCA << 16;

  Status = SdhcSendCommand (HostInst, &CmdSendStatus, CmdArg);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CardStatus->AsUint32 = HostInst->CmdResponse[0];

  return EFI_SUCCESS;
}

//
// SD Specific Functions
//

EFI_STATUS
SdhcSendScrSd (
  IN SDHC_INSTANCE  *HostInst
  )
{
  UINT32 CmdArg = HostInst->CardInfo.RCA << 16;
  EFI_STATUS Status;
  SD_SCR *Scr;

  Status = SdhcSendDataCommand (
    HostInst,
    &CmdAppSendScrSd,
    CmdArg,
    sizeof (HostInst->BlockBuffer),
    HostInst->BlockBuffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CopyMem (
    &HostInst->CardInfo.Registers.Sd.Scr,
    HostInst->BlockBuffer,
    sizeof (SD_SCR));

  Scr = (SD_SCR*) &HostInst->BlockBuffer;
  LOG_TRACE ("SD_SCR:");
  LOG_TRACE ("  SD_SPEC=%d", (UINT32) Scr->SD_SPEC);
  LOG_TRACE ("  SD_SPEC3=%d", (UINT32) Scr->SD_SPEC3);
  LOG_TRACE (
    "  SD_BUS_WIDTHS=%x, 1-Bit?%d, 4-Bit?%d",
    (UINT32) Scr->SD_BUS_WIDTH,
    (UINT32) ((Scr->SD_BUS_WIDTH & BIT1) ? 1 : 0),
    (UINT32) ((Scr->SD_BUS_WIDTH & BIT2) ? 1 : 0));
  LOG_TRACE (
    "  CMD_SUPPORT=%x, CMD23?%d, CMD20?%d",
    (UINT32) Scr->CMD_SUPPORT,
    (UINT32) ((Scr->CMD_SUPPORT & BIT2) ? 1 : 0),
    (UINT32) ((Scr->CMD_SUPPORT & BIT1) ? 1 : 0));

  return EFI_SUCCESS;
}

EFI_STATUS
SdhcSendIfCondSd (
  IN SDHC_INSTANCE  *HostInst
  )
{
  SEND_IF_COND_ARG CmdArg = { 0 };

  //
  // Recommended check pattern per SD Specs
  //
  CmdArg.Fields.CheckPattern = 0xAA;

  //
  // Our current implementation does not support more than HighSpeed voltage 2V7-3V6 (i.e no 1V8)
  //
  CmdArg.Fields.VoltageSupplied = SD_CMD8_VOLTAGE_27_36;

  EFI_STATUS Status = SdhcSendCommand (HostInst, &CmdSendIfCondSd, CmdArg.AsUint32);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  SEND_IF_COND_CMD_RESPONSE CmdStatus;
  CmdStatus.AsUint32 = HostInst->CmdResponse[0];

  if (CmdStatus.Fields.CheckPattern != CmdArg.Fields.CheckPattern ||
      CmdStatus.Fields.VoltageSupplied != CmdArg.Fields.VoltageSupplied) {
    return EFI_UNSUPPORTED;
  }

  HostInst->CardInfo.HasExtendedOcr = TRUE;

  return EFI_SUCCESS;;
}

EFI_STATUS
SdhcSendOpCondSd (
  IN SDHC_INSTANCE  *HostInst
  )
{
  EFI_STATUS Status;

  //
  // With arg set to 0, it means read OCR
  //
  Status = SdhcSendCommand (HostInst, &CmdAppSendOpCondSd, 0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  HostInst->CardInfo.Registers.Sd.Ocr.AsUint32 = HostInst->CmdResponse[0];

  SD_SEND_OP_COND_ARG CmdArg = { 0 };
  UINT32 Retry = SDMMC_POLL_WAIT_COUNT;
  SD_OCR Ocr;
  SD_OCR_EX *OcrEx;

  CmdArg.Fields.VoltageWindow = HostInst->CardInfo.Registers.Sd.Ocr.Fields.VoltageWindow;
  //
  // Host support for High Capacity is assumed
  //
  CmdArg.Fields.HCS = 1;

  while (Retry) {
    Status = SdhcSendCommand (HostInst, &CmdAppSendOpCondSd, CmdArg.AsUint32);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Ocr.AsUint32 = HostInst->CmdResponse[0];

    if (Ocr.Fields.PowerUp) {
      LOG_TRACE ("SD Card PowerUp Complete");
      if (HostInst->CardInfo.HasExtendedOcr) {
        OcrEx = (SD_OCR_EX*) &Ocr;
        if (OcrEx->Fields.CCS) {
          LOG_TRACE ("Card is SD2.0 or later HighCapacity SDHC or SDXC");
          HostInst->CardInfo.HighCapacity = TRUE;
        } else {
          LOG_TRACE ("Card is SD2.0 or later StandardCapacity SDSC");
          HostInst->CardInfo.HighCapacity = FALSE;
        }
      }
      break;
    }
    gBS->Stall (SDMMC_POLL_WAIT_TIME_US);
    --Retry;
  }

  if (!Retry) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}


EFI_STATUS
SdhcSwitchBusWidthSd (
  IN SDHC_INSTANCE  *HostInst
  )
{
  EFI_SDHC_PROTOCOL *HostExt = HostInst->HostExt;
  EFI_STATUS Status;
  UINT32 CmdArg;

  //Status = SdhcSendScrSd(HostInst);
  //if (EFI_ERROR(Status)) {
  //    return Status;
  //}

  CmdArg = 0x2; // 4-bit
  Status = SdhcSendCommand (HostInst, &CmdAppSetBusWidthSd, CmdArg);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HostExt->SetBusWidth (HostExt, SdBusWidth4Bit);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
SdhcSwitchSpeedModeSd (
  IN SDHC_INSTANCE  *HostInst
  )
{
  return EFI_SUCCESS;
}

//
// SDIO Specific Functions
//

EFI_STATUS
SdhcSendOpCondSdio (
  IN SDHC_INSTANCE  *HostInst
  )
{
  return SdhcSendCommand (HostInst, &CmdSendOpCondSdio, 0);
}

//
// Mmc Specific Functions
//

EFI_STATUS
SdhcSendOpCondMmc (
  IN SDHC_INSTANCE  *HostInst
  )
{
  EFI_STATUS Status;
  MMC_SEND_OP_COND_ARG CmdArg = { 0 };
  UINT32 Retry = SDMMC_POLL_WAIT_COUNT;
  MMC_OCR *Ocr = &HostInst->CardInfo.Registers.Mmc.Ocr;

  CmdArg.Fields.VoltageWindow = SD_OCR_HIGH_VOLTAGE_WINDOW;
  CmdArg.Fields.AccessMode = SdOcrAccessSectorMode;

  while (Retry) {
    Status = SdhcSendCommand (HostInst, &CmdSendOpCondMmc, CmdArg.AsUint32);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    HostInst->CardInfo.Registers.Mmc.Ocr.AsUint32 = HostInst->CmdResponse[0];

    if (Ocr->Fields.PowerUp) {
      LOG_TRACE ("MMC Card PowerUp Complete");
      if (Ocr->Fields.AccessMode == SdOcrAccessSectorMode) {
        LOG_TRACE ("Card is MMC HighCapacity");
        HostInst->CardInfo.HighCapacity = TRUE;
      } else {
        LOG_TRACE ("Card is MMC StandardCapacity");
        HostInst->CardInfo.HighCapacity = FALSE;
      }

      if ((Ocr->Fields.VoltageWindow & SD_OCR_HIGH_VOLTAGE_WINDOW) != SD_OCR_HIGH_VOLTAGE_WINDOW) {
        LOG_ERROR (
          "MMC Card does not support High Voltage, expected profile:%x actual profile:%x",
          (UINT32) SD_OCR_HIGH_VOLTAGE_WINDOW,
          (UINT32) Ocr->Fields.VoltageWindow);
        return EFI_UNSUPPORTED;
      }

      break;
    }
    gBS->Stall (SDMMC_POLL_WAIT_TIME_US);
    --Retry;
  }

  if (!Retry) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
SdhcSwitchBusWidthMmc (
  IN SDHC_INSTANCE  *HostInst
  )
{
  EFI_SDHC_PROTOCOL *HostExt = HostInst->HostExt;
  EFI_STATUS Status;
  UINT32 CmdArg;
  SD_BUS_WIDTH BusWidth = SdBusWidth8Bit;
  MMC_EXT_CSD *ExtCsd = &HostInst->CardInfo.Registers.Mmc.ExtCsd;
  UINT8 ExtCsdPowerClass;
  MMC_SWITCH_CMD_ARG SwitchCmdArg;

  Status = SdhcSendExtCsdMmc (HostInst);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Figure out current requirements for target bus width. An inrease in current consumption
  // may require switching the card to a higher power class
  //
  if (BusWidth == SdBusWidth8Bit) {
    if (HostInst->CardInfo.CurrentSpeedMode == CardSpeedModeHighSpeed) {
      ExtCsdPowerClass = MMC_EXT_CSD_POWER_CLASS_8BIT (ExtCsd->PowerClass52Mhz36V);
    } else {
      ExtCsdPowerClass = MMC_EXT_CSD_POWER_CLASS_8BIT (ExtCsd->PowerClass26Mhz36V);
    }
  } else if (BusWidth == SdBusWidth4Bit) {
    if (HostInst->CardInfo.CurrentSpeedMode == CardSpeedModeHighSpeed) {
      ExtCsdPowerClass = MMC_EXT_CSD_POWER_CLASS_4BIT (ExtCsd->PowerClass52Mhz36V);
    } else {
      ExtCsdPowerClass = MMC_EXT_CSD_POWER_CLASS_4BIT (ExtCsd->PowerClass26Mhz36V);
    }
  } else {
    return EFI_UNSUPPORTED;
  }

  //
  // Only do power class switch if the target bus width requires more current than the
  // allowed by the current power class in EXT_CSD
  //
  if (ExtCsdPowerClass > HostInst->CardInfo.Registers.Mmc.ExtCsd.PowerClass) {
    CmdArg = ExtCsdPowerClass;
    CmdArg <<= 8;
    CmdArg |= 0x03BB0000;
    Status = SdhcSendCommand (
      HostInst,
      &CmdSwitchMmc,
      CmdArg);
    if (EFI_ERROR (Status)) {
      LOG_ERROR ("Error detected on switching PowerClass function");
      return Status;
    }

    Status = SdhcSendExtCsdMmc (HostInst);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Sanity check that wanted power-class are set per requested
    //
    if (HostInst->CardInfo.Registers.Mmc.ExtCsd.PowerClass != ExtCsdPowerClass) {
      LOG_ERROR (
        "MMC EXT_CSD not reporting correct PowerClass after switch. Expected:%x Actual:%x",
        (UINT32) ExtCsdPowerClass,
        (UINT32) HostInst->CardInfo.Registers.Mmc.ExtCsd.PowerClass);
      return EFI_PROTOCOL_ERROR;
    }
  }

  //
  // Switch bus width
  //
  SwitchCmdArg.AsUint32 = 0;
  SwitchCmdArg.Fields.Access = MmcSwitchCmdAccessTypeWriteByte;
  SwitchCmdArg.Fields.Index = MmcExtCsdBitIndexBusWidth;

  if (BusWidth == SdBusWidth8Bit) {
    SwitchCmdArg.Fields.Value = MmcExtCsdBusWidth8Bit;
  } else {
    SwitchCmdArg.Fields.Value = MmcExtCsdBusWidth4Bit;
  }

  Status = SdhcSendCommand (
    HostInst,
    &CmdSwitchMmc,
    SwitchCmdArg.AsUint32);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("Error detected on switching BusWidth function");
    return Status;
  }

  Status = HostExt->SetBusWidth (HostExt, BusWidth);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
SdhcSwitchSpeedModeMmc (
  IN SDHC_INSTANCE  *HostInst
  )
{
  EFI_STATUS Status;
  MMC_SWITCH_CMD_ARG CmdArg;

  CmdArg.AsUint32 = 0;
  CmdArg.Fields.Access = MmcSwitchCmdAccessTypeWriteByte;
  CmdArg.Fields.Index = MmcExtCsdBitIndexHsTiming;
  CmdArg.Fields.Value = 1;

  Status = SdhcSendCommand (
    HostInst,
    &CmdSwitchMmc,
    CmdArg.AsUint32);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("Error detected on switching HighSpeed function");
    return Status;
  }

  HostInst->CardInfo.CurrentSpeedMode = CardSpeedModeHighSpeed;

  Status = SdhcSendExtCsdMmc (HostInst);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!HostInst->CardInfo.Registers.Mmc.ExtCsd.HighSpeedTiming) {
    LOG_ERROR ("MMC EXT_CSD not reporting HighSpeed timing after switch");
    return EFI_PROTOCOL_ERROR;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
SdhcSendExtCsdMmc (
  IN SDHC_INSTANCE  *HostInst
  )
{
  EFI_STATUS Status;
  UINT32 CmdArg;
  MMC_EXT_CSD *ExtCsd;

  CmdArg = HostInst->CardInfo.RCA << 16;
  ExtCsd = &HostInst->CardInfo.Registers.Mmc.ExtCsd;

  Status = SdhcSendDataCommand (
    HostInst,
    &CmdSendExtCsdMmc,
    CmdArg,
    sizeof (HostInst->BlockBuffer),
    HostInst->BlockBuffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CopyMem (ExtCsd, HostInst->BlockBuffer, sizeof (MMC_EXT_CSD));

  HostInst->CardInfo.ByteCapacity = (UINT64) ExtCsd->SectorCount * 512llu;
  HostInst->BlockIo.Media->LastBlock = ExtCsd->SectorCount - 1;

  HostInst->RpmbIo.ReliableSectorCount = ExtCsd->ReliableWriteSectorCount;
  HostInst->RpmbIo.RpmbSizeMult = ExtCsd->RpmbSizeMult;

  return EFI_SUCCESS;
}

EFI_STATUS
SdhcSwitchPartitionMmc (
  IN SDHC_INSTANCE                  *HostInst,
  IN MMC_EXT_CSD_PARTITION_ACCESS   Partition
  )
{
  EFI_STATUS Status;
  MMC_SWITCH_CMD_ARG CmdArg;
  MMC_EXT_CSD_PARTITION_CONFIG PartConfig;

  LOG_TRACE (
    "SdhcSwitchPartitionMmc(Partition=%a)",
    MmcPartitionAccessToString (Partition));

  PartConfig.AsUint8 = HostInst->CardInfo.Registers.Mmc.ExtCsd.PartitionConfig;
  PartConfig.Fields.PARTITION_ACCESS = Partition;

  //
  // Write the partition type to EXT_CSD[PARTITION_CONFIG].PARTITION_ACCESS
  //

  ZeroMem (&CmdArg, sizeof (MMC_SWITCH_CMD_ARG));
  CmdArg.Fields.Access = MmcSwitchCmdAccessTypeWriteByte;
  CmdArg.Fields.Index = MmcExtCsdBitIndexPartitionConfig;
  CmdArg.Fields.Value = PartConfig.AsUint8;

  Status = SdhcSendCommand (HostInst, &CmdSwitchMmc, CmdArg.AsUint32);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("SdhcSendCommand failed. (Status = %r)");
    return Status;
  }

  //
  // Re-read the EXT_CSD to verify the partition switch physically happenned
  //
  Status = SdhcSendExtCsdMmc (HostInst);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  PartConfig.AsUint8 = HostInst->CardInfo.Registers.Mmc.ExtCsd.PartitionConfig;
  if (PartConfig.Fields.PARTITION_ACCESS != Partition) {
    LOG_ERROR (
      "Current partition indicated by EXT_CSD doesn't match the "
      "partition the MMC should have switched to. Expected:%a Actual:%a",
      MmcPartitionAccessToString (Partition),
      MmcPartitionAccessToString (PartConfig.Fields.PARTITION_ACCESS));

    return EFI_DEVICE_ERROR;
  }

  LOG_TRACE (
    "Switched from partition %a to %a successfully",
    MmcPartitionAccessToString (HostInst->CurrentMmcPartition),
    MmcPartitionAccessToString (Partition));

  HostInst->CurrentMmcPartition = Partition;

  return EFI_SUCCESS;
}

//
// SD command definitions
//

CONST SD_COMMAND CmdGoIdleState =
{ 0,
 SdCommandTypeUndefined,
 SdCommandClassStandard,
 SdResponseTypeNone,
 SdTransferTypeNone,
 SdTransferDirectionUndefined };

CONST SD_COMMAND CmdSendOpCondMmc =
{ 1,
 SdCommandTypeUndefined,
 SdCommandClassStandard,
 SdResponseTypeR3,
 SdTransferTypeNone,
 SdTransferDirectionUndefined };

CONST SD_COMMAND CmdSendCidAll =
{ 2,
 SdCommandTypeUndefined,
 SdCommandClassStandard,
 SdResponseTypeR2,
 SdTransferTypeNone,
 SdTransferDirectionUndefined };

CONST SD_COMMAND CmdSendRelativeAddr =
{ 3,
 SdCommandTypeUndefined,
 SdCommandClassStandard,
 SdResponseTypeR6,
 SdTransferTypeNone,
 SdTransferDirectionUndefined };

CONST SD_COMMAND CmdSendOpCondSdio =
{ 5,
 SdCommandTypeUndefined,
 SdCommandClassStandard,
 SdResponseTypeR4,
 SdTransferTypeNone,
 SdTransferDirectionUndefined };

CONST SD_COMMAND CmdSwitchSd =
{ 6,
 SdCommandTypeUndefined,
 SdCommandClassStandard,
 SdResponseTypeR1,
 SdTransferTypeSingleBlock,
 SdTransferDirectionRead };

CONST SD_COMMAND CmdSwitchMmc =
{ 6,
 SdCommandTypeUndefined,
 SdCommandClassStandard,
 SdResponseTypeR1B,
 SdTransferTypeNone,
 SdTransferDirectionUndefined };

CONST SD_COMMAND CmdAppSetBusWidthSd =
{ 6,
 SdCommandTypeUndefined,
 SdCommandClassApp,
 SdResponseTypeR1,
 SdTransferTypeNone,
 SdTransferDirectionUndefined };

CONST SD_COMMAND CmdSelect =
{ 7,
 SdCommandTypeUndefined,
 SdCommandClassStandard,
 SdResponseTypeR1B,
 SdTransferTypeNone,
 SdTransferDirectionUndefined };

CONST SD_COMMAND CmdDeselect =
{ 7,
 SdCommandTypeUndefined,
 SdCommandClassStandard,
 SdResponseTypeNone,
 SdTransferTypeNone,
 SdTransferDirectionUndefined };

CONST SD_COMMAND CmdSendIfCondSd =
{ 8,
 SdCommandTypeUndefined,
 SdCommandClassStandard,
 SdResponseTypeR6,
 SdTransferTypeNone,
 SdTransferDirectionUndefined };

CONST SD_COMMAND CmdSendExtCsdMmc =
{ 8,
 SdCommandTypeUndefined,
 SdCommandClassStandard,
 SdResponseTypeR1,
 SdTransferTypeSingleBlock,
 SdTransferDirectionRead };

CONST SD_COMMAND CmdSendCsd =
{ 9,
 SdCommandTypeUndefined,
 SdCommandClassStandard,
 SdResponseTypeR2,
 SdTransferTypeNone,
 SdTransferDirectionUndefined };

CONST SD_COMMAND CmdSendCid =
{ 10,
 SdCommandTypeUndefined,
 SdCommandClassStandard,
 SdResponseTypeR2,
 SdTransferTypeNone,
 SdTransferDirectionUndefined };

CONST SD_COMMAND CmdSwitchVoltageSd =
{ 11,
 SdCommandTypeUndefined,
 SdCommandClassStandard,
 SdResponseTypeR1,
 SdTransferTypeNone,
 SdTransferDirectionUndefined };

CONST SD_COMMAND CmdStopTransmission =
{ 12,
 SdCommandTypeAbort,
 SdCommandClassStandard,
 SdResponseTypeR1B,
 SdTransferTypeNone,
 SdTransferDirectionUndefined };

CONST SD_COMMAND CmdSendStatus =
{ 13,
 SdCommandTypeUndefined,
 SdCommandClassStandard,
 SdResponseTypeR1,
 SdTransferTypeNone,
 SdTransferDirectionUndefined };

CONST SD_COMMAND CmdBusTestReadMmc =
{ 14,
 SdCommandTypeUndefined,
 SdCommandClassStandard,
 SdResponseTypeR1,
 SdTransferTypeSingleBlock,
 SdTransferDirectionRead };

CONST SD_COMMAND CmdSetBlockLength =
{ 16,
 SdCommandTypeUndefined,
 SdCommandClassStandard,
 SdResponseTypeR1,
 SdTransferTypeNone,
 SdTransferDirectionUndefined };

CONST SD_COMMAND CmdReadSingleBlock =
{ 17,
 SdCommandTypeUndefined,
 SdCommandClassStandard,
 SdResponseTypeR1,
 SdTransferTypeSingleBlock,
 SdTransferDirectionRead };

CONST SD_COMMAND CmdReadMultiBlock =
{ 18,
 SdCommandTypeUndefined,
 SdCommandClassStandard,
 SdResponseTypeR1,
 SdTransferTypeMultiBlock,
 SdTransferDirectionRead };

CONST SD_COMMAND CmdBusTestWriteMmc =
{ 19,
 SdCommandTypeUndefined,
 SdCommandClassStandard,
 SdResponseTypeR1,
 SdTransferTypeSingleBlock,
 SdTransferDirectionWrite };

CONST SD_COMMAND CmdSetBlockCount =
{ 23,
 SdCommandTypeUndefined,
 SdCommandClassStandard,
 SdResponseTypeR1,
 SdTransferTypeNone,
 SdTransferDirectionUndefined };

CONST SD_COMMAND CmdWriteSingleBlock =
{ 24,
 SdCommandTypeUndefined,
 SdCommandClassStandard,
 SdResponseTypeR1,
 SdTransferTypeSingleBlock,
 SdTransferDirectionWrite };

CONST SD_COMMAND CmdWriteMultiBlock =
{ 25,
 SdCommandTypeUndefined,
 SdCommandClassStandard,
 SdResponseTypeR1,
 SdTransferTypeMultiBlock,
 SdTransferDirectionWrite };

CONST SD_COMMAND CmdAppSendOpCondSd =
{ 41,
 SdCommandTypeUndefined,
 SdCommandClassApp,
 SdResponseTypeR3,
 SdTransferTypeNone,
 SdTransferDirectionUndefined };

CONST SD_COMMAND CmdAppSetClrCardDetectSd =
{ 42,
 SdCommandTypeUndefined,
 SdCommandClassApp,
 SdResponseTypeR1,
 SdTransferTypeNone,
 SdTransferDirectionUndefined };

CONST SD_COMMAND CmdAppSendScrSd =
{ 51,
 SdCommandTypeUndefined,
 SdCommandClassApp,
 SdResponseTypeR1,
 SdTransferTypeSingleBlock,
 SdTransferDirectionRead };

CONST SD_COMMAND CmdAppSd =
{ 55,
 SdCommandTypeUndefined,
 SdCommandClassApp,
 SdResponseTypeR1,
 SdTransferTypeNone,
 SdTransferDirectionUndefined };
