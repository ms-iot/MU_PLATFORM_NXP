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

#ifndef __SDMMCHW_H__
#define __SDMMCHW_H__

#pragma pack(1)

#define SD_BLOCK_LENGTH_BYTES               512
#define SD_BLOCK_WORD_COUNT                 (SD_BLOCK_LENGTH_BYTES / sizeof (UINT32))
#define SD_IDENT_MODE_CLOCK_FREQ_HZ         400000    // 400 KHz
#define MMC_HIGH_SPEED_MODE_CLOCK_FREQ_HZ   52000000  // 52 MHz

typedef enum {
  CardSpeedModeUndefined = 0,
  CardSpeedModeNormalSpeed,
  CardSpeedModeHighSpeed
} CARD_SPEED_MODE;

typedef enum {
  CardFunctionUnknown = 0,
  CardFunctionSd,
  CardFunctionSdio,
  CardFunctionComboSdSdio,
  CardFunctionMmc
} CARD_FUNCTION;

typedef enum {
  CardStateIdle = 0,
  CardStateReady,
  CardStateIdent,
  CardStateStdby,
  CardStateTran,
  CardStateData,
  CardStateRcv,
  CardStatePrg,
  CardStateDis,
  CardStateBtst,
  CardStateSlp
} CARD_STATE;

typedef union {
  UINT32 AsUint32;
  struct {
    UINT32 RESERVED_1 : 2;            // [1:0]
    UINT32 RESERVED_2 : 1;            // [2]
    UINT32 AKE_SEQ_ERROR : 1;         // [3] SD
    UINT32 RESERVED_3 : 1;            // [4]
    UINT32 APP_CMD : 1;               // [5]
    UINT32 URGENT_BKOPS : 1;          // [6] MMC
    UINT32 SWITCH_ERROR : 1;          // [7] MMC
    UINT32 READY_FOR_DATA : 1;        // [8]
    UINT32 CURRENT_STATE : 4;         // [12:9]
    UINT32 ERASE_RESET : 1;           // [13]
    UINT32 RESERVED_4 : 1;            // [14]
    UINT32 WP_ERASE_SKIP : 1;         // [15]
    UINT32 CID_CSD_OVERWRITE : 1;     // [16]
    UINT32 OVERRUN : 1;               // [17] MMC
    UINT32 UNDERRUN : 1;              // [18] MMC
    UINT32 ERROR : 1;                 // [19]
    UINT32 CC_ERROR : 1;              // [20]
    UINT32 CARD_ECC_FAILED : 1;       // [21]
    UINT32 ILLEGAL_COMMAND : 1;       // [22]
    UINT32 COM_CRC_ERROR : 1;         // [23]
    UINT32 LOCK_UNLOCK_FAILED : 1;    // [24]
    UINT32 CARD_IS_LOCKED : 1;        // [25]
    UINT32 WP_VIOLATION : 1;          // [26]
    UINT32 ERASE_PARAM : 1;           // [27]
    UINT32 ERASE_SEQ_ERROR : 1;       // [28]
    UINT32 BLOCK_LEN_ERROR : 1;       // [29]
    UINT32 ADDRESS_MISALIGN : 1;      // [30]
    UINT32 ADDRESS_OUT_OF_RANGE : 1;  // [31]
  } Fields;
} CARD_STATUS;

#define MMC_CARD_STATUS_ERROR_MASK \
          (BIT7 | BIT13 | BIT15 | BIT16 | BIT17 | BIT18 | BIT19 | BIT20 | \
          BIT21 | BIT22 | BIT23 | BIT24 | BIT26 | BIT27 | BIT28 | BIT29 | \
          BIT30 | BIT31)

#define SD_CARD_STATUS_ERROR_MASK \
          (BIT3 | BIT13 | BIT15 | BIT16 | BIT19 | BIT20 | \
          BIT21 | BIT22 | BIT23 | BIT24 | BIT26 | BIT27 | BIT28 | BIT29 | \
          BIT30 | BIT31)

typedef union {
  UINT32 AsUint32;
  struct {
    UINT32 CheckPattern : 8;
    UINT32 VoltageSupplied : 4;
    UINT32 RESERVED_1 : 16;
    UINT32 CommandIndex : 6;
  } Fields;
} SEND_IF_COND_ARG;

typedef SEND_IF_COND_ARG SEND_IF_COND_CMD_RESPONSE;

// definition for VoltageAccepted in SD_CMD8_STATUS

#define SD_CMD8_VOLTAGE_27_36   0x01
#define SD_CMD8_VOLTAGE_LOW     0x02

typedef union {
  UINT32 AsUint32;
  struct {
    UINT32 VoltageWindow : 24;  // [23:0] Maps to OCR[0:23]
    UINT32 S18R : 1;            // [24] Switching to 1.8V Request
                                // 0b: Use Current signal voltage, 1b: Switch to 1.8V signal voltage

    UINT32 RESERVED_1 : 3;      // [27:25]
    UINT32 XPC : 1;             // SDXC Power Control [28] 0b: Power Saving, 1b: Max Performance
    UINT32 RESERVED_2 : 1;      // [29]
    UINT32 HCS : 1;             // Host Capacity Support [30] 0b: SDSC, 1b: SDHC or SDXC
    UINT32 RESERVED_3 : 1;      // [31]
  } Fields;
} SD_SEND_OP_COND_ARG;

typedef union {
  UINT32 AsUint32;
  struct {
    UINT32 VoltageWindow : 24;  // [23:0] Voltage profile
    UINT32 RESERVED_1 : 5;      // Reserved
    UINT32 AccessMode : 2;      // 00b (byte mode), 10b (sector mode)
    UINT32 PowerUp : 1;         // This bit is set to LOW if the card has not finished the power up routine
  } Fields;
} SD_OCR;

typedef union {
  UINT32 AsUint32;
  struct {
    UINT32 VoltageWindow : 24;  // [23:0] Voltage profile
    UINT32 S18A : 1;            // [24] Switching to 1.8V Accepted
    UINT32 RESERVED_1 : 5;      // Reserved [29:25]
    UINT32 CCS : 1;             // Card Capacity Status [30]
    UINT32 PowerUp : 1;         // This bit is set to LOW if the card has not finished the power up routine [31]
  } Fields;
} SD_OCR_EX;

typedef struct {
  UINT16 MDT : 12;        // Manufacturing date [19:8]
  UINT16 RESERVED_1 : 4;  // Reserved [23:20]
  UINT32 PSN;             // Product serial number [55:24]
  UINT8 PRV;              // Product revision [63:56]
  UINT8 PNM[5];           // Product name [64:103]
  UINT16 OID;             // OEM/Application ID [119:104]
  UINT8 MID;              // Manufacturer ID [127:120]
} SD_CID;

typedef struct {
  UINT16 MDT : 8;         // Manufacturing date [15:8]
  UINT32 PSN;             // Product serial number [47:16]
  UINT8 PRV;              // Product revision [55:48]
  UINT8 PNM[6];           // Product name [103:56]
  UINT8 OID;              // OEM/Application ID [111:104]
  UINT8 CBX : 2;          // Card/BGA [113:112]
  UINT8 RESERVED_1 : 6;   // [119:114]
  UINT8 MID;              // Manufacturer ID [127:120]
} MMC_CID;

typedef struct {
  UINT8 RESERVED_1 : 2;           // Reserved [9:8]
  UINT8 FILE_FORMAT : 2;          // File format [11:10]
  UINT8 TMP_WRITE_PROTECT : 1;    // Temporary write protection [12:12]
  UINT8 PERM_WRITE_PROTECT : 1;   // Permanent write protection [13:13]
  UINT8 COPY : 1;                 // Copy flag (OTP) [14:14]
  UINT8 FILE_FORMAT_GRP : 1;      // File format group [15:15]
  UINT16 RESERVED_2 : 5;          // Reserved [20:16]
  UINT16 WRITE_BL_PARTIAL : 1;    // Partial blocks for write allowed [21:21]
  UINT16 WRITE_BL_LEN : 4;        // Max. write data block length [25:22]
  UINT16 R2W_FACTOR : 3;          // Write speed factor [28:26]
  UINT16 RESERVED_3 : 2;          // Reserved [30:29]
  UINT16 WP_GRP_ENABLE : 1;       // Write protect group enable [31:31]
  UINT32 WP_GRP_SIZE : 7;         // Write protect group size [38:32]
  UINT32 SECTOR_SIZE : 7;         // Erase sector size [45:39]
  UINT32 ERASE_BLK_EN : 1;        // Erase single block enable [46:46]
  UINT32 C_SIZE_MULT : 3;         // Device size multiplier [49:47]
  UINT32 VDD_W_CURR_MAX : 3;      // Max. write current @ VDD max [52:50]
  UINT32 VDD_W_CURR_MIN : 3;      // Max. write current @ VDD min [55:53]
  UINT32 VDD_R_CURR_MAX : 3;      // Max. read current @ VDD max [58:56]
  UINT32 VDD_R_CURR_MIN : 3;      // Max. read current @ VDD min [61:59]
  UINT32 C_SIZELow2 : 2;          // Device size [63:62]
  UINT32 C_SIZEHigh10 : 10;       // Device size [73:64]
  UINT32 RESERVED_4 : 2;          // Reserved [75:74]
  UINT32 DSR_IMP : 1;             // DSR implemented [76:76]
  UINT32 READ_BLK_MISALIGN : 1;   // Read block misalignment [77:77]
  UINT32 WRITE_BLK_MISALIGN : 1;  // Write block misalignment [78:78]
  UINT32 READ_BL_PARTIAL : 1;     // Partial blocks for read allowed [79:79]
  UINT32 READ_BL_LEN : 4;         // Max. read data block length [83:80]
  UINT32 CCC : 12;                // Card command classes [95:84]
  UINT8 TRAN_SPEED;               // Max. bus clock frequency [103:96]
  UINT8 NSAC;                     // Data read access-time 2 in CLK cycles (NSAC*100) [111:104]
  UINT8 TAAC;                     // Data read access-time 1 [119:112]
  UINT8 RESERVED_5 : 6;           // Reserved [125:120]
  UINT8 CSD_STRUCTURE : 2;        // CSD structure [127:126]
} SD_CSD;

typedef struct {
  UINT8 RESERVED_1 : 2;           // Reserved [9:8]
  UINT8 FILE_FORMAT : 2;          // File format [11:10]
  UINT8 TMP_WRITE_PROTECT : 1;    // Temporary write protection [12:12]
  UINT8 PERM_WRITE_PROTECT : 1;   // Permanent write protection [13:13]
  UINT8 COPY : 1;                 // Copy flag (OTP) [14:14]
  UINT8 FILE_FORMAT_GRP : 1;      // File format group [15:15]
  UINT16 RESERVED_2 : 5;          // Reserved [20:16]
  UINT16 WRITE_BL_PARTIAL : 1;    // Partial blocks for write allowed [21:21]
  UINT16 WRITE_BL_LEN : 4;        // Max. write data block length [25:22]
  UINT16 R2W_FACTOR : 3;          // Write speed factor [28:26]
  UINT16 RESERVED_3 : 2;          // Reserved [30:29]
  UINT16 WP_GRP_ENABLE : 1;       // Write protect group enable [31:31]
  UINT16 WP_GRP_SIZE : 7;         // Write protect group size [38:32]
  UINT16 SECTOR_SIZE : 7;         // Erase sector size [45:39]
  UINT16 ERASE_BLK_EN : 1;        // Erase single block enable [46:46]
  UINT16 RESERVED_4 : 1;          // Reserved [47:47]
  UINT32 C_SIZE : 22;             // Device size [63:62]
  UINT32 RESERVED_5 : 6;          // Reserved [75:74]
  UINT32 DSR_IMP : 1;             // DSR implemented [76:76]
  UINT32 READ_BLK_MISALIGN : 1;   // Read block misalignment [77:77]
  UINT32 WRITE_BLK_MISALIGN : 1;  // Write block misalignment [78:78]
  UINT32 READ_BL_PARTIAL : 1;     // Partial blocks for read allowed [79:79]
  UINT16 READ_BL_LEN : 4;         // Max. read data block length [83:80]
  UINT16 CCC : 12;                // Card command classes [95:84]
  UINT8 TRAN_SPEED;               // Max. bus clock frequency [103:96]
  UINT8 NSAC;                     // Data read access-time 2 in CLK cycles (NSAC*100) [111:104]
  UINT8 TAAC;                     // Data read access-time 1 [119:112]
  UINT8 RESERVED_6 : 6;           // Reserved [125:120]
  UINT8 CSD_STRUCTURE : 2;        // CSD structure [127:126]
} SD_CSD_2;

typedef struct {
  UINT8 ECC : 2;                  // ECC code [9:8]
  UINT8 FILE_FORMAT : 2;          // File format [11:10]
  UINT8 TMP_WRITE_PROTECT : 1;    // Temporary write protection [12:12]
  UINT8 PERM_WRITE_PROTECT : 1;   // Permanent write protection [13:13]
  UINT8 COPY : 1;                 // Copy flag (OTP) [14:14]
  UINT8 FILE_FORMAT_GRP : 1;      // File format group [15:15]
  UINT16 CONTENT_PROT_APP : 1;    // Content protection application [16:16]
  UINT16 RESERVED_1 : 4;          // Reserved [20:17]
  UINT16 WRITE_BL_PARTIAL : 1;    // Partial blocks for write allowed [21:21]
  UINT16 WRITE_BL_LEN : 4;        // Max. write data block length [25:22]
  UINT16 R2W_FACTOR : 3;          // Write speed factor [28:26]
  UINT16 DEFAULT_ECC : 2;         // Manufacturer default ECC [30:29]
  UINT16 WP_GRP_ENABLE : 1;       // Write protect group enable [31:31]
  UINT32 WP_GRP_SIZE : 5;         // Write protect group size [36:32]
  UINT32 ERASE_GRP_MULT : 5;      // Erase group size multiplier [41:37]
  UINT32 ERASE_GRP_SIZE : 5;      // Erase sector size [46:42]
  UINT32 C_SIZE_MULT : 3;         // Device size multiplier [49:47]
  UINT32 VDD_W_CURR_MAX : 3;      // Max. write current @ VDD max [52:50]
  UINT32 VDD_W_CURR_MIN : 3;      // Max. write current @ VDD min [55:53]
  UINT32 VDD_R_CURR_MAX : 3;      // Max. read current @ VDD max [58:56]
  UINT32 VDD_R_CURR_MIN : 3;      // Max. read current @ VDD min [61:59]
  UINT32 C_SIZELow2 : 2;          // Device size [63:62]
  UINT32 C_SIZEHigh10 : 10;       // Device size [73:64]
  UINT32 RESERVED_4 : 2;          // Reserved [75:74]
  UINT32 DSR_IMP : 1;             // DSR implemented [76:76]
  UINT32 READ_BLK_MISALIGN : 1;   // Read block misalignment [77:77]
  UINT32 WRITE_BLK_MISALIGN : 1;  // Write block misalignment [78:78]
  UINT32 READ_BL_PARTIAL : 1;     // Partial blocks for read allowed [79:79]
  UINT32 READ_BL_LEN : 4;         // Max. read data block length [83:80]
  UINT32 CCC : 12;                // Card command classes [95:84]
  UINT8 TRAN_SPEED;               // Max. bus clock frequency [103:96]
  UINT8 NSAC;                     // Data read access-time 2 in CLK cycles (NSAC*100) [111:104]
  UINT8 TAAC;                     // Data read access-time 1 [119:112]
  UINT8 RESERVED_5 : 2;           // Reserved [121:120]
  UINT8 SPEC_VERS : 4;            // System specification version [125:122]
  UINT8 CSD_STRUCTURE : 2;        // CSD structure [127:126]
} MMC_CSD;

// We support spec version 4.X and higher
// If CSD.SPEC_VERS indicates a version 4.0 or higher, the card is a high speed card and supports
// SWITCH and SEND_EXT_CSD commands. Otherwise the card is an old MMC card.
#define MMC_MIN_SUPPORTED_SPEC_VERS     4

typedef struct {

  // Host modifiable modes

  UINT8 Reserved26[134];
  UINT8 BadBlockManagement;
  UINT8 Reserved25;
  UINT32 EnhancedUserDataStartAddress;
  UINT8 EnhancedUserDataAreaSize[3];
  UINT8 GpPartSizeMult[12];
  UINT8 PartitioningSetting;
  UINT8 PartitionsAttribute;
  UINT8 MaxEnhancedAreaSize[3];
  UINT8 PartitioningSupport;
  UINT8 Reserved24;
  UINT8 HwResetFunc;
  UINT8 Reserved23[5];
  UINT8 RpmbSizeMult;
  UINT8 FwConfig;
  UINT8 Reserved22;
  UINT8 UserWriteProt;
  UINT8 Reserved21;
  UINT8 BootWriteProt;
  UINT8 Reserved20;
  UINT8 EraseGroupDef;
  UINT8 Reserved19;
  UINT8 BootBusWidth;
  UINT8 BootConfigProt;
  UINT8 PartitionConfig;
  UINT8 Reserved18;
  UINT8 ErasedMemContent;
  UINT8 Reserved17;
  UINT8 BusWidth;
  UINT8 Reserved16;
  UINT8 HighSpeedTiming;
  UINT8 Reserved15;
  UINT8 PowerClass;
  UINT8 Reserved14;
  UINT8 CmdSetRevision;
  UINT8 Reserved13;
  UINT8 CmdSet;

  // Non-modifiable properties

  UINT8 ExtendedCsdRevision;
  UINT8 Reserved12;
  UINT8 CsdStructureVersion;
  UINT8 Reserved11;
  UINT8 CardType;
  UINT8 Reserved10[3];
  UINT8 PowerClass52Mhz195V;
  UINT8 PowerClass26Mhz195V;
  UINT8 PowerClass52Mhz36V;
  UINT8 PowerClass26Mhz36V;
  UINT8 Reserved9;
  UINT8 MinReadPerf4bit26Mhz;
  UINT8 MinWritePerf4bit26Mhz;
  UINT8 MinReadPerf8bit26Mhz;
  UINT8 MinWritePerf8bit26Mhz;
  UINT8 MinReadPerf8bit52Mhz;
  UINT8 MinWritePerf8bit52Mhz;
  UINT8 Reserved8;
  UINT32 SectorCount;
  UINT8 Reserved7;
  UINT8 SleepAwakeTimeout;
  UINT8 Reserved6;
  UINT8 SleepCurrentVccq;
  UINT8 SleepCurrentVcc;
  UINT8 HighCapacityWriteProtectSize;
  UINT8 ReliableWriteSectorCount;
  UINT8 HighCapacityEraseTimeout;
  UINT8 HighCapacityEraseSize;
  UINT8 AccessSize;
  UINT8 BootPartitionSize;
  UINT8 Reserved5;
  UINT8 BootInfo;
  UINT8 SecureTrimMultiplier;
  UINT8 SecureEraseMultiplier;
  UINT8 SecureFeatureSupport;
  UINT8 TrimMultiplier;
  UINT8 Reserved4;
  UINT8 MinReadPerf8bit52MhzDdr;
  UINT8 MinWritePerf8bit52MhzDdr;
  UINT16 Reserved3;
  UINT8 PowerClass52MhzDdr36V;
  UINT8 PowerClass52MhzDdr195V;
  UINT8 Reserved2;
  UINT8 InitTimeoutAfterPartitioning;
  UINT8 Reserved1[262];
  UINT8 SupportedCmdSets;
  UINT8 Reserved[7];
} MMC_EXT_CSD;

typedef enum {
  MmcExtCsdCardTypeNormalSpeed = 0x01,
  MmcExtCsdCardTypeHighSpeed = 0x02,
  MmcExtCsdCardTypeDdr1v8 = 0x04,
  MmcExtCsdCardTypeDdr1v2 = 0x08
} MmcExtCsdCardType;

typedef enum {
  MmcExtCsdBusWidth1Bit = 0,
  MmcExtCsdBusWidth4Bit = 1,
  MmcExtCsdBusWidth8Bit = 2
} MmcExtCsdBusWidth;

typedef enum {
  MmcExtCsdPartitionAccessUserArea,
  MmcExtCsdPartitionAccessBootPartition1,
  MmcExtCsdPartitionAccessBootPartition2,
  MmcExtCsdPartitionAccessRpmb,
  MmcExtCsdPartitionAccessGpp1,
  MmcExtCsdPartitionAccessGpp2,
  MmcExtCsdPartitionAccessGpp3,
  MmcExtCsdPartitionAccessGpp4,
} MMC_EXT_CSD_PARTITION_ACCESS;

typedef enum {
  MmcExtCsdBootPartitionEnableNotBootEnabled,
  MmcExtCsdBootPartitionBootPartition1BootEnabled,
  MmcExtCsdBootPartitionBootPartition2BootEnabled,
  MmcExtCsdBootPartitionUserAreaBootEnabled = 7,
} MMC_EXT_CSD_BOOT_PARTITION_ENABLE;

typedef union {
  UINT8 AsUint8;
  struct {
    UINT8 PARTITION_ACCESS : 3;       // [2:0]
    UINT8 BOOT_PARTITION_ENABLE : 3;  // [5:3]
    UINT8 BOOT_ACK : 1;               // [6]
    UINT8 RESERVED_1 : 1;             // [7]
  } Fields;
} MMC_EXT_CSD_PARTITION_CONFIG;

typedef enum {
  MmcExtCsdBitIndexPartitionConfig = 179,
  MmcExtCsdBitIndexBusWidth = 183,
  MmcExtCsdBitIndexHsTiming = 185
} MMC_EXT_CSD_BIT_INDEX;

typedef enum {
  MmcSwitchCmdAccessTypeCommandSet,
  MmcSwitchCmdAccessTypeSetBits,
  MmcSwitchCmdAccessTypeClearBits,
  MmcSwitchCmdAccessTypeWriteByte
} MMC_SWITCH_CMD_ACCESS_TYPE;

typedef union {
  UINT32 AsUint32;
  struct {
    UINT32 CmdSet : 3;      // [2:0]
    UINT32 RESERVED_1 : 5;  // Set to 0 [7:3]
    UINT32 Value : 8;       // [15:8]
    UINT32 Index : 8;       // [23:16]
    UINT32 Access : 2;      // A value from MMC_SWITCH_CMD_ACCESS_TYPE [25:24]
    UINT32 RESERVED_2 : 6;  // Set to 0 [31:26]
  } Fields;
} MMC_SWITCH_CMD_ARG;

// Bits [3:0] code the current consumption for the 4 bit bus configuration
#define MMC_EXT_CSD_POWER_CLASS_4BIT(X)         ((X) & 0xF)

// Bits [7:4] code the current consumption for the 8 bit bus configuration
#define MMC_EXT_CSD_POWER_CLASS_8BIT(X)         ((X) >> 4)

typedef struct {
  UINT32 RESERVED_1;
  UINT32 CMD_SUPPORT : 2;
  UINT32 RESERVED_2 : 9;
  UINT32 EX_SECURITY : 4;
  UINT32 SD_SPEC3 : 1;
  UINT32 SD_BUS_WIDTH : 4;
  UINT32 SD_SECURITY : 3;
  UINT32 DATA_STAT_AFTER_ERASE : 1;
  UINT32 SD_SPEC : 4;
  UINT32 SCR_STRUCTURE : 4;
} SD_SCR;

typedef SD_OCR MMC_OCR;
typedef SD_OCR MMC_SEND_OP_COND_ARG;

typedef enum {
  SdOcrAccessByteMode = 0,
  SdOcrAccessSectorMode = 2
} SD_OCR_ACCESS;

// Voltage window that covers from 2.8V and up to 3.6V
#define SD_OCR_HIGH_VOLTAGE_WINDOW        0x00FF8000

typedef struct {
  SD_OCR Ocr;
  SD_CID Cid;
  SD_CSD Csd;
  SD_SCR Scr;
} SD_REGISTERS;

typedef struct {
  MMC_OCR Ocr;
  MMC_CID Cid;
  MMC_CSD Csd;
  MMC_EXT_CSD ExtCsd;
} MMC_REGISTERS;

typedef struct {
  UINT32              RCA;
  CARD_FUNCTION       CardFunction;
  BOOLEAN             HasExtendedOcr;
  BOOLEAN             HighCapacity;
  UINT64              ByteCapacity;
  CARD_SPEED_MODE     CurrentSpeedMode;

  union {
    SD_REGISTERS Sd;
    MMC_REGISTERS Mmc;
  } Registers;

} CARD_INFO;

#pragma pack()

#endif // __SDMMCHW_H__

