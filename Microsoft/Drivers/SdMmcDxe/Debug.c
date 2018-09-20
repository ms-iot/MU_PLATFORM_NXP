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

CONST CHAR8 *mStrUnit[] = { "100kbit/s", "1Mbit/s", "10Mbit/s", "100MBit/s",
                            "Unknown", "Unknown", "Unknown", "Unknown" };
CONST CHAR8 *mStrValue[] = { "UNDEF", "1.0", "1.2", "1.3", "1.5", "2.0", "2.5", "3.0", "3.5", "4.0", "4.5", "5.0",
                             "Unknown", "Unknown", "Unknown", "Unknown" };

VOID
PrintCid (
  IN SDHC_INSTANCE  *HostInst
  )
{
  LOG_INFO ("- PrintCid");;

  if (HostInst->CardInfo.CardFunction == CardFunctionSd) {
    SD_CID *Cid;
    Cid = &HostInst->CardInfo.Registers.Sd.Cid;
    LOG_INFO ("\t- Manufacturer ID: 0x%x", (UINT32) Cid->MID);
    LOG_INFO ("\t- OEM ID: 0x%x", (UINT32) Cid->OID);
    LOG_INFO (
      "\t- Product name: %c%c%c%c%c",
      Cid->PNM[4],
      Cid->PNM[3],
      Cid->PNM[2],
      Cid->PNM[1],
      Cid->PNM[0]);

    LOG_INFO (
      "\t- Manufacturing date: %d/%d",
      (UINT32) (Cid->MDT & 0xF),
      (UINT32) (((Cid->MDT >> 4) & 0x3F) + 2000));
    LOG_INFO ("\t- Product serial number: 0x%X", Cid->PSN);
    LOG_INFO ("\t- Product revision: %d", (UINT32) Cid->PRV);

    LOG_INFO ("\t- OEM ID: 0x%x", (UINT32) Cid->OID);
    LOG_INFO ("\t- Manufacturer ID: 0x%x", (UINT32) Cid->MID);
  } else {
    MMC_CID *Cid;
    Cid = &HostInst->CardInfo.Registers.Mmc.Cid;
    LOG_INFO ("\t- Manufacturer ID: 0x%x", (UINT32) Cid->MID);
    LOG_INFO ("\t- OEM ID: 0x%x", (UINT32) Cid->OID);
    LOG_INFO (
      "\t- Product name: %c%c%c%c%c%c\n",
      Cid->PNM[5],
      Cid->PNM[4],
      Cid->PNM[3],
      Cid->PNM[2],
      Cid->PNM[1],
      Cid->PNM[0]);

    LOG_INFO (
      "\t- Manufacturing date: %d/%d",
      (UINT32) (Cid->MDT >> 4),
      (UINT32) (Cid->MDT & 0xF) + 1997);

    LOG_INFO ("\t- Product serial number: 0x%X", Cid->PSN);
    LOG_INFO ("\t- Product revision: %d", (UINT32) Cid->PRV);
  }
}

VOID
PrintCsd (
  IN SDHC_INSTANCE  *HostInst
  )
{
  LOG_INFO ("- PrintCsd");

  UINT32 CardSizeGB;
  CardSizeGB =
    (UINT32) INT_DIV_ROUND (HostInst->CardInfo.ByteCapacity, (UINT64) (1024 * 1024 * 1024));

  LOG_INFO (
    "\t- CardSize: %ld~%dGB, BlockSize:%d LastBlock LBA:0x%08x",
    HostInst->CardInfo.ByteCapacity,
    CardSizeGB,
    HostInst->BlockIo.Media->BlockSize,
    (UINT32) HostInst->BlockIo.Media->LastBlock);

  if (HostInst->CardInfo.CardFunction == CardFunctionSd) {
    SD_CSD *Csd = &HostInst->CardInfo.Registers.Sd.Csd;
    if (Csd->CSD_STRUCTURE == 0) {
      LOG_INFO ("- SD CSD Version 1.01-1.10/Version 2.00/Standard Capacity");
    } else if (Csd->CSD_STRUCTURE == 1) {
      LOG_INFO ("- SD CSD Version 2.00/High Capacity");
    } else {
      LOG_INFO ("- SD CSD Version Higher than v3.3");
    }

    LOG_INFO (
      "\t- SW Write Protect: Temp:%d Permanent:%d",
      (UINT32) Csd->TMP_WRITE_PROTECT,
      (UINT32) Csd->PERM_WRITE_PROTECT);

    LOG_INFO ("\t- Supported card command class: 0x%X", Csd->CCC);

    LOG_INFO (
      "\t- Speed: %a %a (TRAN_SPEED:%x)",
      mStrValue[(Csd->TRAN_SPEED >> 3) & 0xF],
      mStrUnit[Csd->TRAN_SPEED & 7],
      (UINT32) Csd->TRAN_SPEED);

    LOG_INFO (
      "\t- Maximum Read Data Block: %d (READ_BL_LEN:%x)",
      (UINT32) (1 << Csd->READ_BL_LEN),
      (UINT32) Csd->READ_BL_LEN);

    LOG_INFO (
      "\t- Maximum Write Data Block: %d (WRITE_BL_LEN:%x)",
      (UINT32) (1 << Csd->WRITE_BL_LEN),
      (UINT32) Csd->WRITE_BL_LEN);

    if (!Csd->FILE_FORMAT_GRP) {
      if (Csd->FILE_FORMAT == 0) {
        LOG_INFO ("\t- Format (0): Hard disk-like file system with partition table");
      } else if (Csd->FILE_FORMAT == 1) {
        LOG_INFO ("\t- Format (1): DOS FAT (floppy-like) with boot sector only (no partition table)");
      } else if (Csd->FILE_FORMAT == 2) {
        LOG_INFO ("\t- Format (2): Universal File Format");
      } else {
        LOG_INFO ("\t- Format (3): Others/Unknown");
      }
    } else {
      LOG_INFO ("\t- Format: Reserved");
    }
  } else {
    MMC_CSD *Csd = &HostInst->CardInfo.Registers.Mmc.Csd;
    MMC_EXT_CSD *ExtCsd = &HostInst->CardInfo.Registers.Mmc.ExtCsd;

    if (ExtCsd->CardType & MmcExtCsdCardTypeNormalSpeed) {
      LOG_INFO ("\t- Normal-Speed MMC @ 26MHz");
    }

    if (ExtCsd->CardType & MmcExtCsdCardTypeHighSpeed) {
      LOG_INFO ("\t- High-Speed MMC @ 52MHz");
    }

    if (ExtCsd->CardType & MmcExtCsdCardTypeDdr1v8) {
      LOG_INFO ("\t- High-Speed DDR MMC @ 52MHz - 1.8V or 3V I/O");
    }

    if (ExtCsd->CardType & MmcExtCsdCardTypeDdr1v2) {
      LOG_INFO ("\t- High-Speed DDR MMC @ 52MHz - 1.2VI/O");
    }

    LOG_INFO (
      "\t- SW Write Protect: Temp:%d Permenant:%d",
      (UINT32) Csd->TMP_WRITE_PROTECT,
      (UINT32) Csd->PERM_WRITE_PROTECT);

    LOG_INFO ("\t- SpecVersion: %d.x", (UINT32) Csd->SPEC_VERS);
    LOG_INFO ("\t- Supported card command class: 0x%X", Csd->CCC);

    LOG_INFO (
      "\t- Current Max Speed: %a %a (TRAN_SPEED:%x)",
      mStrValue[(Csd->TRAN_SPEED >> 3) & 0xF],
      mStrUnit[Csd->TRAN_SPEED & 7],
      (UINT32) Csd->TRAN_SPEED);

    LOG_INFO (
      "\t- Maximum Read Data Block: %d (READ_BL_LEN:%x)",
      (UINT32) (1 << Csd->READ_BL_LEN),
      (UINT32) Csd->READ_BL_LEN);

    LOG_INFO (
      "\t- Maximum Write Data Block: %d (WRITE_BL_LEN:%x)",
      (UINT32) (1 << Csd->WRITE_BL_LEN),
      (UINT32) Csd->WRITE_BL_LEN);

    if (!Csd->FILE_FORMAT_GRP) {
      if (Csd->FILE_FORMAT == 0) {
        LOG_INFO ("\t- Format (0): Hard disk-like file system with partition table");
      } else if (Csd->FILE_FORMAT == 1) {
        LOG_INFO ("\t- Format (1): DOS FAT (floppy-like) with boot sector only (no partition table)");
      } else if (Csd->FILE_FORMAT == 2) {
        LOG_INFO ("\t- Format (2): Universal File Format");
      } else {
        LOG_INFO ("\t- Format (3): Others/Unknown");
      }
    } else {
      LOG_INFO ("\t- Format: Reserved");
    }
  }
}

VOID
GetAndPrintCardStatus (
  IN SDHC_INSTANCE  *HostInst
  )
{
  CARD_STATUS CardStatus;
  EFI_STATUS Status;

  //
  // If the card has not been selected, then we can't get its status
  //
  if (HostInst->CardInfo.RCA == 0x0) {
    return;
  }

  Status = SdhcSendCommand (HostInst, &CmdSendStatus, 0);
  if (EFI_ERROR (Status)) {
    return;
  }

  CardStatus.AsUint32 = HostInst->CmdResponse[0];
  PrintCardStatus (HostInst, CardStatus);
}

VOID
PrintCardStatus (
  IN SDHC_INSTANCE  *HostInst,
  IN CARD_STATUS    CardStatus
  )
{
  switch (HostInst->CardInfo.CardFunction) {
  case CardFunctionSd:
    LOG_INFO (
      "Status: APP_CMD:%d READY_FOR_DATA:%d CURRENT_STATE:%d(%a) "
      "CARD_IS_LOCKED:%d",
      CardStatus.Fields.APP_CMD,
      CardStatus.Fields.READY_FOR_DATA,
      CardStatus.Fields.CURRENT_STATE,
      CardStateToString (CardStatus.Fields.CURRENT_STATE),
      CardStatus.Fields.CARD_IS_LOCKED);

    break;

  case CardFunctionMmc:
    LOG_INFO (
      "Status: APP_CMD:%d URGENT_BKOPS:%d READY_FOR_DATA:%d "
      "CURRENT_STATE:%d(%a) CARD_IS_LOCKED:%d",
      CardStatus.Fields.APP_CMD,
      CardStatus.Fields.URGENT_BKOPS,
      CardStatus.Fields.READY_FOR_DATA,
      CardStatus.Fields.CURRENT_STATE,
      CardStateToString (CardStatus.Fields.CURRENT_STATE),
      CardStatus.Fields.CARD_IS_LOCKED);

    break;

  default:
    ASSERT (FALSE);
  }

  if (IsCardStatusError (HostInst, CardStatus)) {
    LOG_INFO ("Errors:");

    if (HostInst->CardInfo.CardFunction == CardFunctionSd) {
      if (CardStatus.Fields.AKE_SEQ_ERROR) {
        LOG_INFO ("\t- SWITCH_ERROR");
      }
    }

    if (HostInst->CardInfo.CardFunction == CardFunctionMmc) {
      if (CardStatus.Fields.SWITCH_ERROR) {
        LOG_INFO ("\t- SWITCH_ERROR");
      }

      if (CardStatus.Fields.OVERRUN) {
        LOG_INFO ("\t- OVERRUN");
      }

      if (CardStatus.Fields.UNDERRUN) {
        LOG_INFO ("\t- UNDERRUN");
      }
    }

    if (CardStatus.Fields.ERASE_RESET) {
      LOG_INFO ("\t- ERASE_RESET");
    }

    if (CardStatus.Fields.WP_ERASE_SKIP) {
      LOG_INFO ("\t- WP_ERASE_SKIP");
    }

    if (CardStatus.Fields.CID_CSD_OVERWRITE) {
      LOG_INFO ("\t- CID_CSD_OVERWRITE");
    }

    if (CardStatus.Fields.ERROR) {
      LOG_INFO ("\t- ERROR");
    }

    if (CardStatus.Fields.CC_ERROR) {
      LOG_INFO ("\t- CC_ERROR");
    }

    if (CardStatus.Fields.CARD_ECC_FAILED) {
      LOG_INFO ("\t- CARD_ECC_FAILED");
    }

    if (CardStatus.Fields.ILLEGAL_COMMAND) {
      LOG_INFO ("\t- ILLEGAL_COMMAND");
    }

    if (CardStatus.Fields.COM_CRC_ERROR) {
      LOG_INFO ("\t- COM_CRC_ERROR");
    }

    if (CardStatus.Fields.LOCK_UNLOCK_FAILED) {
      LOG_INFO ("\t- LOCK_UNLOCK_FAILED");
    }

    if (CardStatus.Fields.WP_VIOLATION) {
      LOG_INFO ("\t- WP_VIOLATION");
    }

    if (CardStatus.Fields.ERASE_PARAM) {
      LOG_INFO ("\t- ERASE_PARAM");
    }

    if (CardStatus.Fields.ERASE_SEQ_ERROR) {
      LOG_INFO ("\t- ERASE_SEQ_ERROR");
    }

    if (CardStatus.Fields.BLOCK_LEN_ERROR) {
      LOG_INFO ("\t- BLOCK_LEN_ERROR");
    }

    if (CardStatus.Fields.ADDRESS_MISALIGN) {
      LOG_INFO ("\t- ADDRESS_MISALIGN");
    }

    if (CardStatus.Fields.ADDRESS_OUT_OF_RANGE) {
      LOG_INFO ("\t- ADDRESS_OUT_OF_RANGE");
    }
  }
}