#/** @file
# iMX6 ULL board description
# The board is iMX6 ULL with 512MB DRAM
#
#  Copyright (c) 2018 Microsoft Corporation. All rights reserved.
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#**/

################################################################################
#
# Board [Defines] Section
#
################################################################################

[Defines]
  DEFINE BOARD_NAME               = iMX6ULL_EVK_512MB
  DEFINE IMX_FAMILY               = IMX6ULL
  DEFINE DRAM_SIZE                = DRAM_512MB
  DEFINE CONFIG_DUMP_SYMBOL_INFO  = TRUE
  DEFINE CONFIG_L2CACHE_OFF       = TRUE
  DEFINE CONFIG_HEADLESS          = TRUE
  DEFINE CONFIG_NOT_SECURE_UEFI   = TRUE
  DEFINE CONFIG_MEASURED_BOOT     = FALSE
  DEFINE CONFIG_USB               = FALSE
  BOARD_DIR                       = NXP/$(BOARD_NAME)
  FLASH_DEFINITION                = $(BOARD_DIR)/$(BOARD_NAME).fdf

################################################################################
#
# [BuildOptions] Section
#
################################################################################
[BuildOptions]
  GCC:*_*_*_CC_FLAGS = -D$(BOARD_NAME) -DCPU_$(IMX_FAMILY) -Wno-unused-local-typedefs

[BuildOptions.common.EDKII.DXE_CORE,BuildOptions.common.EDKII.DXE_DRIVER,BuildOptions.common.EDKII.UEFI_DRIVER,BuildOptions.common.EDKII.UEFI_APPLICATION]
  GCC:*_*_*_DLINK_FLAGS = -z common-page-size=0x1000

[BuildOptions.common.EDKII.DXE_RUNTIME_DRIVER]
  GCC:*_*_ARM_DLINK_FLAGS = -z common-page-size=0x1000

################################################################################
#
# Platform Description
#
################################################################################
!include iMX6Pkg/iMX6CommonDsc.inc

[LibraryClasses.common]
  ArmPlatformLib|$(BOARD_DIR)/Library/iMX6BoardLib/iMX6BoardLib.inf
[Components.common]
  #
  # ACPI Support
  #
  MdeModulePkg/Universal/Acpi/AcpiTableDxe/AcpiTableDxe.inf
  MdeModulePkg/Universal/Acpi/AcpiPlatformDxe/AcpiPlatformDxe.inf
  $(BOARD_DIR)/AcpiTables/AcpiTables.inf

  # SMBIOS Support
  iMX6Pkg/Drivers/PlatformSmbiosDxe/PlatformSmbiosDxe.inf
  MdeModulePkg/Universal/SmbiosDxe/SmbiosDxe.inf

################################################################################
#
# Board PCD Sections
#
################################################################################

########################
#
# iMX6Pkg PCDs
#
########################
[PcdsFixedAtBuild.common]

# SMBIOS Type0 Strings
gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareVendor|L"NXP"
gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareRevision|0x00000001 # FirmwareRevision 0.1

# SMBIOS Type1 Strings
giMX6TokenSpaceGuid.PcdSystemFamily|L"MX6"
giMX6TokenSpaceGuid.PcdSystemManufacturer|L"NXP"
giMX6TokenSpaceGuid.PcdSystemProductName|L"i.MX 6ULL"
giMX6TokenSpaceGuid.PcdSystemSkuNumber|L"MCIMX6ULL"
giMX6TokenSpaceGuid.PcdSystemVersionNumber|L"B"
giMX6TokenSpaceGuid.PcdSystemUuid|{0xe2,0xd4,0xe7,0x83,0x18,0xe9,0x47,0x21,0x85,0x1f,0x70,0x19,0x62,0x35,0xbf,0xe1}

# SMBIOS Type2 Strings
giMX6TokenSpaceGuid.PcdBoardAssetTag|L"0"
giMX6TokenSpaceGuid.PcdBoardLocationInChassis|L"Open Board"
giMX6TokenSpaceGuid.PcdBoardManufacturer|L"NXP"
giMX6TokenSpaceGuid.PcdBoardProductName|L"i.MX 6ULL"
giMX6TokenSpaceGuid.PcdBoardVersionNumber|L"B"

# SMBIOS Type3 Strings
giMX6TokenSpaceGuid.PcdChassisAssetTag|L"0"
giMX6TokenSpaceGuid.PcdChassisManufacturer|L"NXP"
giMX6TokenSpaceGuid.PcdChassisVersionNumber|L"B"

# SMBIOS Type4 Strings
giMX6TokenSpaceGuid.PcdProcessorAssetTag|L"0"
giMX6TokenSpaceGuid.PcdProcessorManufacturer|L"NXP"
giMX6TokenSpaceGuid.PcdProcessorPartNumber|L"i.MX 6ULL"
giMX6TokenSpaceGuid.PcdProcessorSocketDesignation|L"FCPBGA"
giMX6TokenSpaceGuid.PcdProcessorVersionNumber|L"1.0"

# SMBIOS Type16
giMX6TokenSpaceGuid.PcdPhysicalMemoryMaximumCapacity|0x80000 # 512MB

# SMBIOS Type17
giMX6TokenSpaceGuid.PcdMemoryBankLocation|L"Bank 0"
giMX6TokenSpaceGuid.PcdMemoryDeviceLocation|L"On SoM"

  #
  # uSDHC Controllers as connected on iMX6DQ SabreSD
  #
  # uSDHCx | SabreSD On-board Connection | Physical Base
  #-----------------------------------------------------
  # uSDHC1 | SDLOT                       | 0x02190000
  # uSDHC2 | J500   SD2 AUX SDIO Socket  | 0x02194000
  #
  giMXPlatformTokenSpaceGuid.PcdSdhc1Base|0x02190000
  giMXPlatformTokenSpaceGuid.PcdSdhc2Base|0x02194000
  #
  # SDCard Slot (uSDHC1)
  #---------------------------------------------------
  # Card Detect Signal   | NANDF_D0 | ALT5 | GPIO1_IO3
  # Write Protect Signal | NANDF_D1 | ALT5 | GPIO1_IO2
  #
  # MSB byte is GPIO bank number, and LSB byte is IO number
  #
  giMXPlatformTokenSpaceGuid.PcdSdhc1Enable|TRUE
  giMXPlatformTokenSpaceGuid.PcdSdhc1CardDetectSignal|0x0103
  giMXPlatformTokenSpaceGuid.PcdSdhc1WriteProtectSignal|0x0102

  #
  # No UART initialization required leveraging the first boot loader
  #
  giMXPlatformTokenSpaceGuid.PcdSerialRegisterBase|0x02020000   # UART1
  giMXPlatformTokenSpaceGuid.PcdKdUartInstance|1                # UART1
