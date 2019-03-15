/** @file
*
*  Copyright (c) 2018, Microsoft Corporation. All rights reserved.
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

#include <Library/ArmPlatformLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PrintLib.h>
#include <Library/PcdLib.h>
#include <Library/SerialPortLib.h>
#include <Library/TimerLib.h>
#include <Ppi/ArmMpCoreInfo.h>

#include "iMX8.h"
#include "iMX8ClkPwr.h"
#include "iMX8MIoMux.h"

ARM_CORE_INFO iMX8Ppi[] =
{
  {
    // Cluster 0, Core 0
    0x0, 0x0,

    // MP Core MailBox Set/Get/Clear Addresses and Clear Value.
    // Not used with i.MX8, set to 0
    (EFI_PHYSICAL_ADDRESS)0x00000000,
    (EFI_PHYSICAL_ADDRESS)0x00000000,
    (EFI_PHYSICAL_ADDRESS)0x00000000,
    (UINT64)0
  },

#if FixedPcdGet32(PcdCoreCount) > 1
  {
    // Cluster 0, Core 1
    0x0, 0x1,

    // MP Core MailBox Set/Get/Clear Addresses and Clear Value
    // Not used with i.MX8, set to 0
    (EFI_PHYSICAL_ADDRESS)0x00000000,
    (EFI_PHYSICAL_ADDRESS)0x00000000,
    (EFI_PHYSICAL_ADDRESS)0x00000000,
    (UINT64)0
  },
#endif // FixedPcdGet32(PcdCoreCount) > 1

#if FixedPcdGet32(PcdCoreCount) > 2
  {
    // Cluster 0, Core 2
    0x0, 0x2,

    // MP Core MailBox Set/Get/Clear Addresses and Clear Value
    // Not used with i.MX8, set to 0
    (EFI_PHYSICAL_ADDRESS)0x00000000,
    (EFI_PHYSICAL_ADDRESS)0x00000000,
    (EFI_PHYSICAL_ADDRESS)0x00000000,
    (UINT64)0
  },

  {
    // Cluster 0, Core 3
    0x0, 0x3,

    // MP Core MailBox Set/Get/Clear Addresses and Clear Value
    // Not used with i.MX8, set to 0
    (EFI_PHYSICAL_ADDRESS)0x00000000,
    (EFI_PHYSICAL_ADDRESS)0x00000000,
    (EFI_PHYSICAL_ADDRESS)0x00000000,
    (UINT64)0
  }
#endif // FixedPcdGet32(PcdCoreCount) > 2
};

EFI_STATUS
PrePeiCoreGetMpCoreInfo (
  OUT UINTN                   *CoreCount,
  OUT ARM_CORE_INFO           **ArmCoreTable
  )
{
  // Only support one cluster
  *CoreCount    = sizeof(iMX8Ppi) / sizeof(ARM_CORE_INFO);
  ASSERT (*CoreCount == FixedPcdGet32 (PcdCoreCount));
  *ArmCoreTable = iMX8Ppi;
  return EFI_SUCCESS;
}

ARM_MP_CORE_INFO_PPI mMpCoreInfoPpi = { PrePeiCoreGetMpCoreInfo };

EFI_PEI_PPI_DESCRIPTOR      gPlatformPpiTable[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gArmMpCoreInfoPpiGuid,
    &mMpCoreInfoPpi
  }
};

VOID
ArmPlatformGetPlatformPpiList (
  OUT UINTN                   *PpiListSize,
  OUT EFI_PEI_PPI_DESCRIPTOR  **PpiList
  )
{
  *PpiListSize = sizeof(gPlatformPpiTable);
  *PpiList = gPlatformPpiTable;
}

typedef enum {
  IMX_PAD_SAI2_MCLK_SAI2_MCLK = _IMX_MAKE_PADCFG (
                                IMX_DSE_6_45_OHM,
                                IMX_SRE_2_FAST,
                                IMX_ODE_0_Open_Drain_Disabled,
                                IMX_PUE_1_Pull_Up_Enabled,
                                IMX_HYS_ENABLED,
                                IMX_LVTTL_0_LVTTL_Disabled,
                                IMX_SION_DISABLED,
                                IMX_IOMUXC_SAI2_MCLK_ALT0_SAI2_MCLK),

  IMX_PAD_SAI2_TXFS_SAI2_TX_SYNC = _IMX_MAKE_PADCFG (
                                   IMX_DSE_6_45_OHM,
                                   IMX_SRE_2_FAST,
                                   IMX_ODE_0_Open_Drain_Disabled,
                                   IMX_PUE_1_Pull_Up_Enabled,
                                   IMX_HYS_ENABLED,
                                   IMX_LVTTL_0_LVTTL_Disabled,
                                   IMX_SION_DISABLED,
                                   IMX_IOMUXC_SAI2_TXFS_ALT0_SAI2_TX_SYNC),

  IMX_PAD_SAI2_TXC_SAI2_TX_BCLK =  _IMX_MAKE_PADCFG (
                                  IMX_DSE_6_45_OHM,
                                  IMX_SRE_2_FAST,
                                  IMX_ODE_0_Open_Drain_Disabled,
                                  IMX_PUE_1_Pull_Up_Enabled,
                                  IMX_HYS_ENABLED,
                                  IMX_LVTTL_0_LVTTL_Disabled,
                                  IMX_SION_DISABLED,
                                  IMX_IOMUXC_SAI2_TXC_ALT0_SAI2_TX_BCLK),

  IMX_PAD_SAI2_TXD0_SAI2_DATA0 = _IMX_MAKE_PADCFG (
                                 IMX_DSE_6_45_OHM,
                                 IMX_SRE_2_FAST,
                                 IMX_ODE_0_Open_Drain_Disabled,
                                 IMX_PUE_1_Pull_Up_Enabled,
                                 IMX_HYS_ENABLED,
                                 IMX_LVTTL_0_LVTTL_Disabled,
                                 IMX_SION_DISABLED,
                                 IMX_IOMUXC_SAI2_TXD0_ALT0_SAI2_TX_DATA0),

  IMX_PAD_GPIO1_IO08_GPIO1_IO08 = _IMX_MAKE_PADCFG (
                                  IMX_DSE_6_45_OHM,
                                  IMX_SRE_2_FAST,
                                  IMX_ODE_0_Open_Drain_Disabled,
                                  IMX_PUE_1_Pull_Up_Enabled,
                                  IMX_HYS_ENABLED,
                                  IMX_LVTTL_0_LVTTL_Disabled,
                                  IMX_SION_DISABLED,
                                  IMX_IOMUXC_GPIO1_IO08_ALT0_GPIO1_IO08),
} IMX_AUDIO_PADCFG;

/**
  Initalize the Audio system
**/
VOID
AudioInit (VOID)
{
  EFI_STATUS status;

  // Mux the SAI2 pins to wm8524 codec
  ImxPadConfig (IMX_PAD_SAI2_MCLK, IMX_PAD_SAI2_MCLK_SAI2_MCLK);
  ImxPadConfig (IMX_PAD_SAI2_TXFS, IMX_PAD_SAI2_TXFS_SAI2_TX_SYNC);
  ImxPadConfig (IMX_PAD_SAI2_TXC, IMX_PAD_SAI2_TXC_SAI2_TX_BCLK);
  ImxPadConfig (IMX_PAD_SAI2_TXD0, IMX_PAD_SAI2_TXD0_SAI2_DATA0);

  // unmute audio
  ImxPadConfig (IMX_PAD_GPIO1_IO08,  IMX_PAD_GPIO1_IO08_GPIO1_IO08);
  ImxGpioDirection (IMX_GPIO_BANK1, 8, IMX_GPIO_DIR_OUTPUT);
  ImxGpioWrite (IMX_GPIO_BANK1, 8, IMX_GPIO_HIGH);

  // enable the AUDIO_PLL - 44,100 Hz * 256
  status = ImxSetSAI2ClockRate (11289600);
  if (EFI_ERROR (status)) {
    DebugPrint (DEBUG_ERROR, "AudioInit - ImxSetAudioMclkClockRate failed");
  }
}

/**
  Initialize controllers that must setup at the early stage
**/
RETURN_STATUS
ArmPlatformInitialize (
  IN  UINTN                     MpId
  )
{
  if (!ArmPlatformIsPrimaryCore (MpId)) {
    return RETURN_SUCCESS;
  }

  // Initialize debug serial port
  SerialPortInitialize ();
  SerialPortWrite (
    (UINT8 *)SERIAL_DEBUG_PORT_INIT_MSG,
    (UINTN)sizeof(SERIAL_DEBUG_PORT_INIT_MSG));

  // Initialize peripherals
  ImxUngateActiveClock ();
  AudioInit ();

  return RETURN_SUCCESS;
}

/**
  Return the current Boot Mode

  This function returns the boot reason on the platform
**/
EFI_BOOT_MODE
ArmPlatformGetBootMode (VOID)
{
  return BOOT_WITH_FULL_CONFIGURATION;
}

