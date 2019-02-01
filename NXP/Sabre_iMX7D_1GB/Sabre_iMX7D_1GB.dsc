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
  DEFINE IMX_FAMILY       = IMX7D
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
  giMXPlatformTokenSpaceGuid.PcdSdhc1CardDetectSignal|0xFF00
  giMXPlatformTokenSpaceGuid.PcdSdhc1WriteProtectSignal|0xFF00

  # UART1
  giMXPlatformTokenSpaceGuid.PcdSerialRegisterBase|0x30860000
  giMXPlatformTokenSpaceGuid.PcdKdUartInstance|1
