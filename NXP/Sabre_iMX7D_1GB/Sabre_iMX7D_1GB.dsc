#/** @file
# NXP iMX7D SoM SABRE board description
#
# The board is iMX7 with 1GB DRAM
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
  DEFINE BOARD_NAME       = Sabre_iMX7D_1GB
  DEFINE IMX_FAMILY       = IMX7
  DEFINE IMX_CHIP_TYPE    = DUAL
  DEFINE CONFIG_USB       = FALSE
  DEFINE CONFIG_HEADLESS  = TRUE
  BOARD_DIR               = NXP/$(BOARD_NAME)
  FLASH_DEFINITION        = $(BOARD_DIR)/$(BOARD_NAME).fdf

################################################################################
#
# Platform Description
#
################################################################################
!include iMX7Pkg/iMX7CommonDsc.inc

################################################################################
#
# Board PCD Sections
#
################################################################################

########################
#
# iMX7Pkg PCDs
#
########################
[PcdsFixedAtBuild.common]

  # System memory size (1GB)
  gArmTokenSpaceGuid.PcdSystemMemorySize|0x40000000

  # uSDHCx | SABRE_IMX7 Connections
  #-------------------------------------
  # uSDHC1 | SD Card slot
  # uSDHC2 | LBEH5HMZPC wifi+BT
  # uSDHC3 | eMMC (optional)
  # uSDHC4 | N/A
  #
  giMXPlatformTokenSpaceGuid.PcdSdhc1Enable|TRUE
  giMXPlatformTokenSpaceGuid.PcdSdhc2Enable|FALSE
  giMXPlatformTokenSpaceGuid.PcdSdhc3Enable|FALSE
  giMXPlatformTokenSpaceGuid.PcdSdhc4Enable|FALSE

  giMXPlatformTokenSpaceGuid.PcdSdhc1CardDetectSignal|0xFF00
  giMXPlatformTokenSpaceGuid.PcdSdhc1WriteProtectSignal|0xFF01
  giMXPlatformTokenSpaceGuid.PcdSdhc2CardDetectSignal|0xFF00
  giMXPlatformTokenSpaceGuid.PcdSdhc2WriteProtectSignal|0xFF01
  giMXPlatformTokenSpaceGuid.PcdSdhc3CardDetectSignal|0xFF00
  giMXPlatformTokenSpaceGuid.PcdSdhc3WriteProtectSignal|0xFF01
  giMXPlatformTokenSpaceGuid.PcdSdhc4CardDetectSignal|0xFF00
  giMXPlatformTokenSpaceGuid.PcdSdhc4WriteProtectSignal|0xFF01

  # UART1
  giMXPlatformTokenSpaceGuid.PcdSerialRegisterBase|0x30860000

