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

#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

EFI_STATUS
InitializeDevice (
  IN SDHC_INSTANCE  *HostInst
  );

EFI_STATUS
InitializeSdDevice (
  IN SDHC_INSTANCE  *HostInst
  );

EFI_STATUS
InitializeMmcDevice (
  IN SDHC_INSTANCE  *HostInst
  );

EFI_STATUS
SdhcSendCommand (
  IN SDHC_INSTANCE      *HostInst,
  IN CONST SD_COMMAND   *Cmd,
  IN UINT32             Arg
  );

EFI_STATUS
SdhcSendDataCommand (
  IN SDHC_INSTANCE      *HostInst,
  IN CONST SD_COMMAND   *Cmd,
  IN UINT32             Arg,
  IN UINT32             BufferByteSize,
  IN OUT VOID           *Buffer
  );

// SD/SDIO/MMC Generic Functions

EFI_STATUS
SdhcRecoverFromErrors (
  IN SDHC_INSTANCE      *HostInst,
  IN CONST SD_COMMAND   *Cmd
  );

EFI_STATUS
SdhcQueryCardType (
  IN SDHC_INSTANCE  *HostInst
  );

EFI_STATUS
SdhcGoIdleState (
  IN SDHC_INSTANCE  *HostInst
  );

EFI_STATUS
SdhcSendCidAll (
  IN SDHC_INSTANCE  *HostInst
  );

EFI_STATUS
SdhcSendRelativeAddr (
  IN SDHC_INSTANCE  *HostInst
  );

EFI_STATUS
SdhcWaitForTranStateAndReadyForData (
  IN SDHC_INSTANCE  *HostInst
  );

EFI_STATUS
SdhcSendCid (
  IN SDHC_INSTANCE  *HostInst
  );

EFI_STATUS
SdhcSendCsd (
  IN SDHC_INSTANCE  *HostInst
  );

EFI_STATUS
SdhcSendAppCmd (
  IN SDHC_INSTANCE  *HostInst
  );

EFI_STATUS
SdhcSelectDevice (
  IN SDHC_INSTANCE  *HostInst
  );

EFI_STATUS
SdhcDeselectDevice (
  IN SDHC_INSTANCE  *HostInst
  );

EFI_STATUS
SdhcStopTransmission (
  IN SDHC_INSTANCE  *HostInst
  );

EFI_STATUS
SdhcSetMaxClockFrequency (
  IN SDHC_INSTANCE  *HostInst
  );

EFI_STATUS
SdhcSetBlockLength (
  IN SDHC_INSTANCE  *HostInst,
  IN UINT32          BlockByteLength
  );

EFI_STATUS
SdhcSetBlockCount (
  IN SDHC_INSTANCE  *HostInst,
  IN UINT32         BlockCount,
  IN BOOLEAN        ReliableWrite
  );

EFI_STATUS
SdhcSendStatus (
  IN SDHC_INSTANCE  *HostInst,
  OUT CARD_STATUS   *CardStatus
  );

// SD Specific Functions

EFI_STATUS
SdhcSendScrSd (
  IN SDHC_INSTANCE  *HostInst
  );

EFI_STATUS
SdhcSendOpCondSd (
  IN SDHC_INSTANCE  *HostInst
  );

EFI_STATUS
SdhcSendIfCondSd (
  IN SDHC_INSTANCE  *HostInst
  );

EFI_STATUS
SdhcSwitchBusWidthSd (
  IN SDHC_INSTANCE  *HostInst
  );

EFI_STATUS
SdhcSwitchSpeedModeSd (
  IN SDHC_INSTANCE  *HostInst
  );

// SDIO Specific Functions

EFI_STATUS
SdhcSendOpCondSdio (
  IN SDHC_INSTANCE  *HostInst
  );

// MMC Specific Functions

EFI_STATUS
SdhcSendOpCondMmc (
  IN SDHC_INSTANCE  *HostInst
  );

EFI_STATUS
SdhcSwitchBusWidthMmc (
  IN SDHC_INSTANCE  *HostInst
  );


EFI_STATUS
SdhcSwitchSpeedModeMmc (
  IN SDHC_INSTANCE  *HostInst
  );

EFI_STATUS
SdhcSendExtCsdMmc (
  IN SDHC_INSTANCE  *HostInst
  );

EFI_STATUS
SdhcSwitchPartitionMmc (
  IN SDHC_INSTANCE                  *HostInst,
  IN MMC_EXT_CSD_PARTITION_ACCESS   Partition
  );

// SD/MMC Commands

extern CONST SD_COMMAND CmdGoIdleState;
extern CONST SD_COMMAND CmdSendOpCondMmc;
extern CONST SD_COMMAND CmdSendCidAll;
extern CONST SD_COMMAND CmdSendRelativeAddr;
extern CONST SD_COMMAND CmdSendOpCondSdio;
extern CONST SD_COMMAND CmdSwitchSd;
extern CONST SD_COMMAND CmdSwitchMmc;
extern CONST SD_COMMAND CmdAppSetBusWidthSd;
extern CONST SD_COMMAND CmdSelect;
extern CONST SD_COMMAND CmdDeselect;
extern CONST SD_COMMAND CmdSendIfCondSd;
extern CONST SD_COMMAND CmdSendExtCsdMmc;
extern CONST SD_COMMAND CmdSendCsd;
extern CONST SD_COMMAND CmdSendCid;
extern CONST SD_COMMAND CmdSwitchVoltageSd;
extern CONST SD_COMMAND CmdStopTransmission;
extern CONST SD_COMMAND CmdSendStatus;
extern CONST SD_COMMAND CmdBusTestReadMmc;
extern CONST SD_COMMAND CmdSetBlockLength;
extern CONST SD_COMMAND CmdReadSingleBlock;
extern CONST SD_COMMAND CmdReadMultiBlock;
extern CONST SD_COMMAND CmdBusTestWriteMmc;
extern CONST SD_COMMAND CmdSetBlockCount;
extern CONST SD_COMMAND CmdWriteSingleBlock;
extern CONST SD_COMMAND CmdWriteMultiBlock;
extern CONST SD_COMMAND CmdAppSendOpCondSd;
extern CONST SD_COMMAND CmdAppSetClrCardDetectSd;
extern CONST SD_COMMAND CmdAppSendScrSd;
extern CONST SD_COMMAND CmdAppSd;

#endif // __PROTOCOL_H__
