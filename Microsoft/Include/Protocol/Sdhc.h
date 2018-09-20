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

#ifndef __SDHC_H__
#define __SDHC_H__

//
// Global ID for the SDHC Protocol {46055B0F-992A-4AD7-8F81-148186FFDF72}
//
#define EFI_SDHC_PROTOCOL_GUID \
    { 0x46055b0f, 0x992a, 0x4ad7, { 0x8f, 0x81, 0x14, 0x81, 0x86, 0xff, 0xdf, 0x72 } };

typedef UINT16 SD_COMMAND_INDEX;

typedef enum {
    SdCommandTypeUndefined = 0,
    SdCommandTypeSuspend,
    SdCommandTypeResume,
    SdCommandTypeAbort
} SD_COMMAND_TYPE;

typedef enum {
    SdCommandClassUndefined = 0,
    SdCommandClassStandard,
    SdCommandClassApp
} SD_COMMAND_CLASS;

typedef enum {
    SdResponseTypeUndefined = 0,
    SdResponseTypeNone,
    SdResponseTypeR1,
    SdResponseTypeR1B,
    SdResponseTypeR2,
    SdResponseTypeR3,
    SdResponseTypeR4,
    SdResponseTypeR5,
    SdResponseTypeR5B,
    SdResponseTypeR6
} SD_RESPONSE_TYPE;

typedef enum {
    SdTransferTypeUndefined = 0,
    SdTransferTypeNone,
    SdTransferTypeSingleBlock,
    SdTransferTypeMultiBlock,
    SdTransferTypeMultiBlockNoStop
} SD_TRANSFER_TYPE;

typedef enum {
    SdTransferDirectionUndefined = 0,
    SdTransferDirectionRead,
    SdTransferDirectionWrite
} SD_TRANSFER_DIRECTION;

typedef struct {
    SD_COMMAND_INDEX Index;
    SD_COMMAND_TYPE Type;
    SD_COMMAND_CLASS Class;
    SD_RESPONSE_TYPE ResponseType;
    SD_TRANSFER_TYPE TransferType;
    SD_TRANSFER_DIRECTION TransferDirection;
} SD_COMMAND;

typedef struct {
    UINT32 BlockSize;
    UINT32 BlockCount;
    VOID* Buffer;
} SD_COMMAND_XFR_INFO;

typedef enum {
    SdBusWidthUndefined = 0,
    SdBusWidth1Bit = 1,
    SdBusWidth4Bit = 4,
    SdBusWidth8Bit = 8
} SD_BUS_WIDTH;

typedef enum {
    SdhcResetTypeUndefined = 0,
    SdhcResetTypeAll,
    SdhcResetTypeCmd,
    SdhcResetTypeData
} SDHC_RESET_TYPE;

typedef struct {
  UINT32 MaximumBlockSize;
  UINT32 MaximumBlockCount;
} SDHC_CAPABILITIES;
//
// Forward declaration for EFI_SDHC_PROTOCOL
//
typedef struct _EFI_SDHC_PROTOCOL EFI_SDHC_PROTOCOL;

typedef VOID (EFIAPI *SDHC_GET_CAPABILITIES) (
  IN EFI_SDHC_PROTOCOL *This,
  OUT SDHC_CAPABILITIES *Capabilities
  );

typedef EFI_STATUS (EFIAPI *SDHC_SOFTWARERESET) (
  IN EFI_SDHC_PROTOCOL *This,
  IN SDHC_RESET_TYPE ResetType
  );

typedef EFI_STATUS (EFIAPI *SDHC_SETCLOCK) (
  IN EFI_SDHC_PROTOCOL *This,
  IN UINT32 TargetFreqHz
  );

typedef EFI_STATUS (EFIAPI *SDHC_SETBUSWIDTH) (
  IN EFI_SDHC_PROTOCOL *This,
  IN SD_BUS_WIDTH BusWidth
  );

typedef BOOLEAN (EFIAPI *SDHC_ISCARDPRESENT) (
  IN EFI_SDHC_PROTOCOL *This
  );

typedef BOOLEAN (EFIAPI *SDHC_ISREADONLY) (
  IN EFI_SDHC_PROTOCOL *This
  );

typedef EFI_STATUS (EFIAPI *SDHC_SENDCOMMAND) (
  IN EFI_SDHC_PROTOCOL *This,
  IN const SD_COMMAND *Cmd,
  IN UINT32 Argument,
  IN OPTIONAL const SD_COMMAND_XFR_INFO *XfrInfo
  );

typedef EFI_STATUS (EFIAPI *SDHC_RECEIVERESPONSE) (
  IN EFI_SDHC_PROTOCOL *This,
  IN const SD_COMMAND *Cmd,
  OUT UINT32 *Buffer
  );

typedef EFI_STATUS (EFIAPI *SDHC_READBLOCKDATA) (
  IN EFI_SDHC_PROTOCOL *This,
  IN UINTN LengthInBytes,
  OUT UINT32 *Buffer
  );

typedef EFI_STATUS (EFIAPI *SDHC_WRITEBLOCKDATA) (
  IN EFI_SDHC_PROTOCOL *This,
  IN UINTN LengthInBytes,
  IN const UINT32 *Buffer
  );

typedef VOID (EFIAPI *SDHC_CLEANUP) (
  IN EFI_SDHC_PROTOCOL *This
  );

struct _EFI_SDHC_PROTOCOL {
  UINT32                   Revision;

  //
  // A unique ID that identify the SDHC device among other
  // SDHC devices on the system
  //
  UINT32                   SdhcId;

  //
  // Context area allocated by the SDHC driver
  //
  VOID                     *PrivateContext;

  //
  // SDHHC Callbacks
  //
  SDHC_GET_CAPABILITIES    GetCapabilities;
  SDHC_SOFTWARERESET       SoftwareReset;
  SDHC_SETCLOCK            SetClock;
  SDHC_SETBUSWIDTH         SetBusWidth;
  SDHC_ISCARDPRESENT       IsCardPresent;
  SDHC_ISREADONLY          IsReadOnly;
  SDHC_SENDCOMMAND         SendCommand;
  SDHC_RECEIVERESPONSE     ReceiveResponse;
  SDHC_READBLOCKDATA       ReadBlockData;
  SDHC_WRITEBLOCKDATA      WriteBlockData;
  SDHC_CLEANUP             Cleanup;
};

#define SDHC_PROTOCOL_INTERFACE_REVISION    0x00010000    // 1.0

extern EFI_GUID gEfiSdhcProtocolGuid;

#endif // __SDHC_H__

