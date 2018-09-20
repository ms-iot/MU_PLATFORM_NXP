#/** @file
# iMX6QP SabreSD board description
# The board is iMX6QP with 1GB DRAM
#
#  Copyright (c) Microsoft Corporation. All rights reserved.
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
  DEFINE BOARD_NAME               = Sabre_iMX6QP_1GB
  DEFINE IMX_FAMILY               = IMX6DQP
  DEFINE IMX_CHIP_TYPE            = QUADPLUS
  DEFINE DRAM_SIZE                = DRAM_1GB
  DEFINE CONFIG_DUMP_SYMBOL_INFO  = TRUE
  BOARD_DIR                       = NXP/$(BOARD_NAME)
  FLASH_DEFINITION                = $(BOARD_DIR)/$(BOARD_NAME).fdf

################################################################################
#
# Platform Description
#
################################################################################
!include iMX6Pkg/iMX6CommonDsc.inc

[LibraryClasses.common]
  ArmPlatformLib|$(BOARD_DIR)/Library/iMX6BoardLib/iMX6BoardLib.inf

[Components.common]
  # Display Support
!if $(CONFIG_HEADLESS) == FALSE
  MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf
  MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf
  !if $(IMX_FAMILY) == IMX6SX
    #
    # Use board-specific GOP for SoloX
    #
    $(BOARD_DIR)/Drivers/GraphicsOutputDxe/GraphicsOutputDxe.inf
  !else
    iMX6Pkg/Drivers/GopDxe/GopDxe.inf
  !endif
!endif

  # ACPI Support
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
giMX6TokenSpaceGuid.PcdSystemFamily|L"SABRE"
giMX6TokenSpaceGuid.PcdSystemManufacturer|L"NXP"
giMX6TokenSpaceGuid.PcdSystemProductName|L"i.MX 6QuadPlus SABRE"
giMX6TokenSpaceGuid.PcdSystemSkuNumber|L"MCIMX6QP-SDB"
giMX6TokenSpaceGuid.PcdSystemVersionNumber|L"A1"
giMX6TokenSpaceGuid.PcdSystemUuid|{0x0c,0xef,0x06,0x16,0x83,0x3d,0x4d,0x03,0xb9,0x71,0xd7,0x40,0xa6,0xcb,0xf7,0x2b}

# SMBIOS Type2 Strings
giMX6TokenSpaceGuid.PcdBoardAssetTag|L"0"
giMX6TokenSpaceGuid.PcdBoardLocationInChassis|L"Open Board"
giMX6TokenSpaceGuid.PcdBoardManufacturer|L"NXP"
giMX6TokenSpaceGuid.PcdBoardProductName|L"i.MX 6QuadPlus SABRE"
giMX6TokenSpaceGuid.PcdBoardVersionNumber|L"B"

# SMBIOS Type3 Strings
giMX6TokenSpaceGuid.PcdChassisAssetTag|L"0"
giMX6TokenSpaceGuid.PcdChassisManufacturer|L"NXP"
giMX6TokenSpaceGuid.PcdChassisVersionNumber|L"B"

# SMBIOS Type4 Strings
giMX6TokenSpaceGuid.PcdProcessorAssetTag|L"0"
giMX6TokenSpaceGuid.PcdProcessorManufacturer|L"NXP"
giMX6TokenSpaceGuid.PcdProcessorPartNumber|L"i.MX 6QuadPlus"
giMX6TokenSpaceGuid.PcdProcessorSocketDesignation|L"FCPBGA"
giMX6TokenSpaceGuid.PcdProcessorVersionNumber|L"1.0"

# SMBIOS Type16
giMX6TokenSpaceGuid.PcdPhysicalMemoryMaximumCapacity|0x100000 # 1GB

# SMBIOS Type17
giMX6TokenSpaceGuid.PcdMemoryBankLocation|L"Bank 0"
giMX6TokenSpaceGuid.PcdMemoryDeviceLocation|L"On SoM"

  #
  # uSDHC Controllers as connected on iMX6DQP SabreSD
  #
  # uSDHCx | SabreSD On-board Connection | Physical Base
  #-----------------------------------------------------
  # uSDHC1 | Not Connected               | 0x02190000
  # uSDHC2 | J500 SD2 AUX SDIO Socket    | 0x02194000
  # uSDHC3 | J507 SD3 Card Socket        | 0x02198000
  # uSDHC4 | eMMC (SanDisk 8GB MMC 4.41) | 0x0219C000
  #
  giMXPlatformTokenSpaceGuid.PcdSdhc1Base|0x02190000
  giMXPlatformTokenSpaceGuid.PcdSdhc2Base|0x02194000
  giMXPlatformTokenSpaceGuid.PcdSdhc3Base|0x02198000
  giMXPlatformTokenSpaceGuid.PcdSdhc4Base|0x0219C000
  
  #
  # SDCard Slot (uSDHC3)
  #---------------------------------------------------
  # Card Detect Signal   | NANDF_D0 | ALT5 | GPIO2_IO0
  # Write Protect Signal | NANDF_D1 | ALT5 | GPIO2_IO1
  #
  # MSB byte is GPIO bank number, and LSB byte is IO number
  #
  giMXPlatformTokenSpaceGuid.PcdSdhc3Enable|TRUE
  giMXPlatformTokenSpaceGuid.PcdSdhc3CardDetectSignal|0x0200
  #giMXPlatformTokenSpaceGuid.PcdSdhc3WriteProtectSignal|0x0201

  #
  # eMMC Slot (uSDHC4)
  #
  giMXPlatformTokenSpaceGuid.PcdSdhc4Enable|TRUE
  giMXPlatformTokenSpaceGuid.PcdSdhc4CardDetectSignal|0xFF00
  giMXPlatformTokenSpaceGuid.PcdSdhc4WriteProtectSignal|0xFF01

  #
  # GPIO reset pin
  #
  giMX6TokenSpaceGuid.PcdPcieResetGpio|TRUE
  giMX6TokenSpaceGuid.PcdPcieResetGpioBankNumber|7
  giMX6TokenSpaceGuid.PcdPcieResetGpioIoNumber|12

  #
  # No UART initialization required leveraging the first boot loader
  #
  giMXPlatformTokenSpaceGuid.PcdSerialRegisterBase|0x02020000   # UART1
  giMXPlatformTokenSpaceGuid.PcdKdUartInstance|1                # UART1

