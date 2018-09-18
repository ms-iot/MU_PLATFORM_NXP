/** @file
* iMX6 Sabre board initialization
*
*  Copyright (c) Microsoft Corporation. All rights reserved.
*  Copyright 2018 NXP
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

#include <Library/IoLib.h>
#include <Library/ArmPlatformLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/PcdLib.h>
#include <Library/SerialPortLib.h>
#include <Library/TimerLib.h>

#include <iMX6.h>
#include <iMX6BoardLib.h>
#include <iMX6ClkPwr.h>
#include <iMX6IoMux.h>
#include <iMX6UsbPhy.h>

//
// Prebaked pad configurations that include mux and drive settings where
// each enum named as IMX_<MODULE-NAME>_PADCFG contains configurations
// for pads used by that module
//

typedef enum {
    IMX_PAD_ENET_MDIO_ENET_MDIO = _IMX_MAKE_PADCFG(
                              IMX_SRE_SLOW,
                              IMX_DSE_40_OHM,
                              IMX_SPEED_MEDIUM,
                              IMX_ODE_ENABLE,
                              IMX_PKE_DISABLE,
                              IMX_PUE_PULL,
                              IMX_PUS_100K_OHM_PU,
                              IMX_HYS_ENABLED,
                              IMX_SION_DISABLED,
                              IMX_IOMUXC_ENET_MDIO_ALT1_ENET_MDIO),

    IMX_PAD_ENET_MDC_ENET_MDC = _IMX_MAKE_PADCFG(
                              IMX_SRE_SLOW,
                              IMX_DSE_40_OHM,
                              IMX_SPEED_MEDIUM,
                              IMX_ODE_DISABLE,
                              IMX_PKE_DISABLE,
                              IMX_PUE_PULL,
                              IMX_PUS_100K_OHM_PU,
                              IMX_HYS_ENABLED,
                              IMX_SION_DISABLED,
                              IMX_IOMUXC_ENET_MDC_ALT1_ENET_MDC),

    IMX_PAD_CFG_ENET_CRS_DV_GPIO1_IO25 = _IMX_MAKE_PADCFG(
                              IMX_SRE_SLOW,
                              IMX_DSE_260_OHM,
                              IMX_SPEED_LOW,
                              IMX_ODE_DISABLE,
                              IMX_PKE_DISABLE,
                              IMX_PUE_PULL,
                              IMX_PUS_100K_OHM_PU,
                              IMX_HYS_DISABLED,
                              IMX_SION_DISABLED,
                              IMX_IOMUXC_ENET_CRS_DV_ALT5_GPIO1_IO25),

    IMX_PAD_RGMII_TXC_RGMII_TXC = _IMX_MAKE_PADCFG(
                              IMX_SRE_SLOW,
                              IMX_DSE_40_OHM,
                              IMX_SPEED_MEDIUM,
                              IMX_ODE_DISABLE,
                              IMX_PKE_DISABLE,
                              IMX_PUE_PULL,
                              IMX_PUS_100K_OHM_PU,
                              IMX_HYS_ENABLED,
                              IMX_SION_DISABLED,
                              IMX_IOMUXC_RGMII_TXC_ALT1_RGMII_TXC),

    IMX_PAD_RGMII_TD0_RGMII_TD0 = _IMX_MAKE_PADCFG(
                              IMX_SRE_SLOW,
                              IMX_DSE_40_OHM,
                              IMX_SPEED_MEDIUM,
                              IMX_ODE_DISABLE,
                              IMX_PKE_DISABLE,
                              IMX_PUE_KEEP,
                              IMX_PUS_100K_OHM_PU,
                              IMX_HYS_ENABLED,
                              IMX_SION_DISABLED,
                              IMX_IOMUXC_RGMII_TD0_ALT1_RGMII_TD0),

    IMX_PAD_RGMII_TD1_RGMII_TD1 = _IMX_MAKE_PADCFG(
                              IMX_SRE_SLOW,
                              IMX_DSE_40_OHM,
                              IMX_SPEED_MEDIUM,
                              IMX_ODE_DISABLE,
                              IMX_PKE_DISABLE,
                              IMX_PUE_KEEP,
                              IMX_PUS_100K_OHM_PU,
                              IMX_HYS_ENABLED,
                              IMX_SION_DISABLED,
                              IMX_IOMUXC_RGMII_TD1_ALT1_RGMII_TD1),

    IMX_PAD_RGMII_TD2_RGMII_TD2 = _IMX_MAKE_PADCFG(
                              IMX_SRE_SLOW,
                              IMX_DSE_40_OHM,
                              IMX_SPEED_MEDIUM,
                              IMX_ODE_DISABLE,
                              IMX_PKE_DISABLE,
                              IMX_PUE_KEEP,
                              IMX_PUS_100K_OHM_PU,
                              IMX_HYS_ENABLED,
                              IMX_SION_DISABLED,
                              IMX_IOMUXC_RGMII_TD2_ALT1_RGMII_TD2),

    IMX_PAD_RGMII_TD3_RGMII_TD3 = _IMX_MAKE_PADCFG(
                              IMX_SRE_SLOW,
                              IMX_DSE_40_OHM,
                              IMX_SPEED_MEDIUM,
                              IMX_ODE_DISABLE,
                              IMX_PKE_DISABLE,
                              IMX_PUE_KEEP,
                              IMX_PUS_100K_OHM_PU,
                              IMX_HYS_ENABLED,
                              IMX_SION_DISABLED,
                              IMX_IOMUXC_RGMII_TD3_ALT1_RGMII_TD3),

    IMX_PAD_RGMII_TX_CTL_RGMII_TX_CTL = _IMX_MAKE_PADCFG(
                              IMX_SRE_SLOW,
                              IMX_DSE_40_OHM,
                              IMX_SPEED_MEDIUM,
                              IMX_ODE_DISABLE,
                              IMX_PKE_DISABLE,
                              IMX_PUE_PULL,
                              IMX_PUS_100K_OHM_PU,
                              IMX_HYS_ENABLED,
                              IMX_SION_DISABLED,
                              IMX_IOMUXC_RGMII_TX_CTL_ALT1_RGMII_TX_CTL),

    IMX_PAD_ENET_REF_CLK_ENET_REF_CLK = _IMX_MAKE_PADCFG(
                              IMX_SRE_FAST,
                              IMX_DSE_40_OHM,
                              IMX_SPEED_MEDIUM,
                              IMX_ODE_DISABLE,
                              IMX_PKE_DISABLE,
                              IMX_PUE_KEEP,
                              IMX_PUS_100K_OHM_PU,
                              IMX_HYS_DISABLED,
                              IMX_SION_DISABLED,
                              IMX_IOMUXC_ENET_REF_CLK_ALT1_ENET_TX_CLK),

    IMX_PAD_RGMII_RXC_ENET_RGMII_RXC = _IMX_MAKE_PADCFG(
                              IMX_SRE_SLOW,
                              IMX_DSE_40_OHM,
                              IMX_SPEED_MEDIUM,
                              IMX_ODE_DISABLE,
                              IMX_PKE_DISABLE,
                              IMX_PUE_KEEP,
                              IMX_PUS_100K_OHM_PU,
                              IMX_HYS_ENABLED,
                              IMX_SION_DISABLED,
                              IMX_IOMUXC_RGMII_RXC_ALT1_RGMII_RXC),

    IMX_PAD_RGMII_RD0_ENET_RGMII_RD0 = _IMX_MAKE_PADCFG(
                              IMX_SRE_SLOW,
                              IMX_DSE_40_OHM,
                              IMX_SPEED_MEDIUM,
                              IMX_ODE_DISABLE,
                              IMX_PKE_DISABLE,
                              IMX_PUE_KEEP,
                              IMX_PUS_100K_OHM_PU,
                              IMX_HYS_ENABLED,
                              IMX_SION_DISABLED,
                              IMX_IOMUXC_RGMII_RD0_ALT1_RGMII_RD0),

    IMX_PAD_RGMII_RD1_ENET_RGMII_RD1 = _IMX_MAKE_PADCFG(
                              IMX_SRE_SLOW,
                              IMX_DSE_40_OHM,
                              IMX_SPEED_MEDIUM,
                              IMX_ODE_DISABLE,
                              IMX_PKE_DISABLE,
                              IMX_PUE_KEEP,
                              IMX_PUS_100K_OHM_PU,
                              IMX_HYS_ENABLED,
                              IMX_SION_DISABLED,
                              IMX_IOMUXC_RGMII_RD1_ALT1_RGMII_RD1),

    IMX_PAD_RGMII_RD2_ENET_RGMII_RD2 = _IMX_MAKE_PADCFG(
                              IMX_SRE_SLOW,
                              IMX_DSE_40_OHM,
                              IMX_SPEED_MEDIUM,
                              IMX_ODE_DISABLE,
                              IMX_PKE_DISABLE,
                              IMX_PUE_KEEP,
                              IMX_PUS_100K_OHM_PU,
                              IMX_HYS_ENABLED,
                              IMX_SION_DISABLED,
                              IMX_IOMUXC_RGMII_RD2_ALT1_RGMII_RD2),

    IMX_PAD_RGMII_RD3_ENET_RGMII_RD3 = _IMX_MAKE_PADCFG(
                              IMX_SRE_SLOW,
                              IMX_DSE_40_OHM,
                              IMX_SPEED_MEDIUM,
                              IMX_ODE_DISABLE,
                              IMX_PKE_DISABLE,
                              IMX_PUE_KEEP,
                              IMX_PUS_100K_OHM_PU,
                              IMX_HYS_ENABLED,
                              IMX_SION_DISABLED,
                              IMX_IOMUXC_RGMII_RD3_ALT1_RGMII_RD3),

    IMX_PAD_RGMII_RX_CTL_RGMII_RX_CTL = _IMX_MAKE_PADCFG(
                              IMX_SRE_SLOW,
                              IMX_DSE_40_OHM,
                              IMX_SPEED_MEDIUM,
                              IMX_ODE_DISABLE,
                              IMX_PKE_DISABLE,
                              IMX_PUE_KEEP,
                              IMX_PUS_100K_OHM_PU,
                              IMX_HYS_ENABLED,
                              IMX_SION_DISABLED,
                              IMX_IOMUXC_RGMII_RX_CTL_ALT1_RGMII_RX_CTL),
} IMX_ENET_PADCFG;

typedef enum {
    IMX_PAD_CFG_EIM_DATA22_USB_OTG_PWR = _IMX_MAKE_PADCFG(
                              IMX_SRE_SLOW,
                              IMX_DSE_40_OHM,
                              IMX_SPEED_LOW,
                              IMX_ODE_DISABLE,
                              IMX_PKE_ENABLE,
                              IMX_PUE_PULL,
                              IMX_PUS_100K_OHM_PU,
                              IMX_HYS_ENABLED,
                              IMX_SION_DISABLED,
                              IMX_IOMUXC_EIM_D22_ALT4_USB_OTG_PWR),

    IMX_PAD_CFG_EIM_DATA21_USB_OTG_OC = _IMX_MAKE_PADCFG_INPSEL(
                              IMX_SRE_SLOW,
                              IMX_DSE_40_OHM,
                              IMX_SPEED_LOW,
                              IMX_ODE_DISABLE,
                              IMX_PKE_ENABLE,
                              IMX_PUE_PULL,
                              IMX_PUS_100K_OHM_PU,
                              IMX_HYS_ENABLED,
                              IMX_SION_DISABLED,
                              IMX_IOMUXC_EIM_D21_ALT4_USB_OTG_OC,
                              IOMUXC_USB_OTG_OC_SELECT_INPUT,
                              EIM_DATA21_ALT4),

    IMX_PAD_CFG_ENET_RX_ER_USB_OTG_ID = _IMX_MAKE_PADCFG(
                              IMX_SRE_FAST,
                              IMX_DSE_90_OHM,
                              IMX_SPEED_LOW,
                              IMX_ODE_DISABLE,
                              IMX_PKE_ENABLE,
                              IMX_PUE_PULL,
                              IMX_PUS_100K_OHM_PD,
                              IMX_HYS_ENABLED,
                              IMX_SION_DISABLED,
                              IMX_IOMUXC_ENET_RX_ER_ALT0_USB_OTG_ID),

} IMX_EHCI_PADCFG;

typedef enum {
  IMX_USDHC_PAD_CTL = _IMX_MAKE_PAD_CTL(
                              IMX_SRE_FAST,
                              IMX_DSE_33_OHM,
                              IMX_SPEED_MEDIUM,
                              IMX_ODE_DISABLE,
                              IMX_PKE_ENABLE,
                              IMX_PUE_PULL,
                              IMX_PUS_47K_OHM_PU,
                              IMX_HYS_ENABLED),

  IMX_USDHC_CLK_PAD_CTL = _IMX_MAKE_PAD_CTL(
                              IMX_SRE_FAST,
                              IMX_DSE_33_OHM,
                              IMX_SPEED_MEDIUM,
                              IMX_ODE_DISABLE,
                              IMX_PKE_DISABLE,
                              0,
                              0,
                              IMX_HYS_ENABLED),

  IMX_USDHC_GPIO_PAD_CTL = _IMX_MAKE_PAD_CTL(
                              IMX_SRE_FAST,
                              IMX_DSE_40_OHM,
                              IMX_SPEED_LOW,
                              IMX_ODE_DISABLE,
                              IMX_PKE_ENABLE,
                              IMX_PUE_PULL,
                              IMX_PUS_22K_OHM_PU,
                              IMX_HYS_ENABLED),

  IMX_PAD_CFG_SD3_CLK_SD3_CLK = _IMX_MAKE_PADCFG2(
                              IMX_USDHC_CLK_PAD_CTL,
                              IMX_SION_DISABLED,
                              IMX_IOMUXC_SD3_CLK_ALT0_SD3_CLK),

  IMX_PAD_CFG_SD3_CMD_SD3_CMD = _IMX_MAKE_PADCFG2(
                              IMX_USDHC_PAD_CTL,
                              IMX_SION_DISABLED,
                              IMX_IOMUXC_SD3_CMD_ALT0_SD3_CMD),

  IMX_PAD_CFG_SD3_DAT0_SD3_DATA0 = _IMX_MAKE_PADCFG2(
                              IMX_USDHC_PAD_CTL,
                              IMX_SION_DISABLED,
                              IMX_IOMUXC_SD3_DAT0_ALT0_SD3_DATA0),

  IMX_PAD_CFG_SD3_DAT1_SD3_DATA1 = _IMX_MAKE_PADCFG2(
                              IMX_USDHC_PAD_CTL,
                              IMX_SION_DISABLED,
                              IMX_IOMUXC_SD3_DAT1_ALT0_SD3_DATA1),

  IMX_PAD_CFG_SD3_DAT2_SD3_DATA2 = _IMX_MAKE_PADCFG2(
                              IMX_USDHC_PAD_CTL,
                              IMX_SION_DISABLED,
                              IMX_IOMUXC_SD3_DAT2_ALT0_SD3_DATA2),

  IMX_PAD_CFG_SD3_DAT3_SD3_DATA3 = _IMX_MAKE_PADCFG2(
                              IMX_USDHC_PAD_CTL,
                              IMX_SION_DISABLED,
                              IMX_IOMUXC_SD3_DAT3_ALT0_SD3_DATA3),

  IMX_PAD_CFG_NANDF_D0_GPIO2_IO00 = _IMX_MAKE_PADCFG2(
                              IMX_USDHC_GPIO_PAD_CTL,
                              IMX_SION_DISABLED,
                              IMX_IOMUXC_NANDF_D0_ALT5_GPIO2_IO00),

  IMX_PAD_CFG_NANDF_D1_GPIO2_IO01 = _IMX_MAKE_PADCFG2(
                              IMX_USDHC_GPIO_PAD_CTL,
                              IMX_SION_DISABLED,
                              IMX_IOMUXC_NANDF_D1_ALT5_GPIO2_IO01),
} IMX_USDHC3_PADCFG;

typedef enum {
  IMX_PAD_CFG_CSI0_DAT9_I2C1_SCL = _IMX_MAKE_PADCFG_INPSEL(
                              IMX_SRE_FAST, // 1
                              IMX_DSE_40_OHM, // 110
                              IMX_SPEED_MEDIUM, // 10
                              IMX_ODE_ENABLE, // 1
                              IMX_PKE_ENABLE, // 1
                              IMX_PUE_PULL, // 1
                              IMX_PUS_100K_OHM_PU, // 10
                              IMX_HYS_ENABLED, // 1
                              IMX_SION_ENABLED, // 1
                              IMX_IOMUXC_CSI0_DAT9_ALT4_I2C1_SCL,
                              IOMUXC_I2C1_SCL_IN_SELECT_INPUT,
                              CSI0_DAT9_ALT4),

  IMX_PAD_CFG_CSI0_DAT8_ALT4_I2C1_SDA = _IMX_MAKE_PADCFG_INPSEL(
                              IMX_SRE_FAST, // 1
                              IMX_DSE_40_OHM, // 110
                              IMX_SPEED_MEDIUM, // 10
                              IMX_ODE_ENABLE, // 1
                              IMX_PKE_ENABLE, // 1
                              IMX_PUE_PULL, // 1
                              IMX_PUS_100K_OHM_PU, // 10
                              IMX_HYS_ENABLED, // 1
                              IMX_SION_ENABLED, // 1
                              IMX_IOMUXC_CSI0_DAT8_ALT4_I2C1_SDA,
                              IOMUXC_I2C1_SDA_IN_SELECT_INPUT,
                              CSI0_DAT8_ALT4),

  IMX_PAD_CFG_EIM_EB3_GPIO2_IO31 = _IMX_MAKE_PADCFG(
                              IMX_SRE_SLOW,
                              IMX_DSE_260_OHM,
                              IMX_SPEED_LOW,
                              IMX_ODE_DISABLE,
                              IMX_PKE_DISABLE,
                              IMX_PUE_PULL,
                              IMX_PUS_100K_OHM_PU,
                              IMX_HYS_DISABLED,
                              IMX_SION_ENABLED,
                              IMX_IOMUXC_EIM_EB3_ALT5_GPIO2_IO31),
} IMX_I2C_PADCFG;

typedef enum {
    IMX_PAD_CFG_EIM_DATA19_GPIO3_IO19 = _IMX_MAKE_PADCFG(
                              IMX_SRE_SLOW,
                              IMX_DSE_40_OHM,
                              IMX_SPEED_MEDIUM,
                              IMX_ODE_DISABLE,
                              IMX_PKE_ENABLE,
                              IMX_PUE_PULL,
                              IMX_PUS_100K_OHM_PU,
                              IMX_HYS_ENABLED,
                              IMX_SION_DISABLED,
                              IMX_IOMUXC_EIM_D19_ALT5_GPIO3_IO19),

    IMX_PAD_CFG_GPIO17_GPIO7_IO12 = _IMX_MAKE_PADCFG(
                              IMX_SRE_SLOW,
                              IMX_DSE_40_OHM,
                              IMX_SPEED_MEDIUM,
                              IMX_ODE_DISABLE,
                              IMX_PKE_ENABLE,
                              IMX_PUE_PULL,
                              IMX_PUS_100K_OHM_PU,
                              IMX_HYS_ENABLED,
                              IMX_SION_DISABLED,
                              IMX_IOMUXC_GPIO_17_ALT5_GPIO7_IO12),

} IMX_IMX6QP_PCIE_PADCFG;

typedef enum {
  //
  // CCM_CLKO1 - this output is the master clock to the audio codec.
  //
  IMX_PAD_CFG_GPIO0_CCM_CLKO1 = _IMX_MAKE_PADCFG(
                              IMX_SRE_SLOW,
                              IMX_DSE_40_OHM,
                              IMX_SPEED_MEDIUM,
                              IMX_ODE_DISABLE,
                              IMX_PKE_ENABLE,
                              IMX_PUE_PULL,
                              IMX_PUS_100K_OHM_PU,
                              IMX_HYS_ENABLED,
                              IMX_SION_DISABLED,
                              IMX_IOMUXC_GPIO_0_ALT0_CCM_CLKO1),
  //
  // KEY_COL2 - this GPIO controls CODEC_PWR_EN.
  //
  IMX_PAD_CFG_KEY_COL2_GPIO4_IO10 = _IMX_MAKE_PADCFG(
                              IMX_SRE_SLOW,
                              IMX_DSE_260_OHM,
                              IMX_SPEED_LOW,
                              IMX_ODE_DISABLE,
                              IMX_PKE_DISABLE,
                              IMX_PUE_PULL,
                              IMX_PUS_100K_OHM_PD,
                              IMX_HYS_DISABLED,
                              IMX_SION_ENABLED,
                              IMX_IOMUXC_KEY_COL2_ALT5_GPIO4_IO10),
  //
  // CSI0_DATA04 thru 08 are alt4 to select the AUD3 I2s lines (TXC/TXD/TXFS/RXD).
  //
  IMX_PAD_CFG_CSI0_DATA04_AUD3_TXC = _IMX_MAKE_PADCFG(
                              IMX_SRE_SLOW,
                              IMX_DSE_40_OHM,
                              IMX_SPEED_MEDIUM,
                              IMX_ODE_DISABLE,
                              IMX_PKE_ENABLE,
                              IMX_PUE_PULL,
                              IMX_PUS_100K_OHM_PU,
                              IMX_HYS_ENABLED,
                              IMX_SION_DISABLED,
                              IMX_IOMUXC_CSI0_DAT4_ALT4_AUD3_TXC),

  IMX_PAD_CFG_CSI0_DATA05_AUD3_TXD = IMX_PAD_CFG_CSI0_DATA04_AUD3_TXC,
  IMX_PAD_CFG_CSI0_DATA06_AUD3_TXFS = IMX_PAD_CFG_CSI0_DATA04_AUD3_TXC,
  IMX_PAD_CFG_CSI0_DATA07_AUD3_RXD = IMX_PAD_CFG_CSI0_DATA04_AUD3_TXC,
} IMX_AUDIO_PADCFG;

typedef enum {
    IMX_PAD_CFG_SD1_DATA3_GPIO1_IO21 = _IMX_MAKE_PADCFG(
                              IMX_SRE_SLOW,
                              IMX_DSE_40_OHM,
                              IMX_SPEED_MEDIUM,
                              IMX_ODE_DISABLE,
                              IMX_PKE_ENABLE,
                              IMX_PUE_PULL,
                              IMX_PUS_100K_OHM_PU,
                              IMX_HYS_ENABLED,
                              IMX_SION_DISABLED,
                              IMX_IOMUXC_SD1_DAT3_ALT5_GPIO1_IO21),

} IMX_IMX6QP_LDB_PADCFG;

/**
  Turn off clock gates which are not needed during boot. The PEP is responsible
  to ungate clocks as needed.
**/
static VOID GateUnusedClocks ()
{
  static const IMX_CLK_GATE gatesToTurnOff[] = {
    IMX_APBHDMA_HCLK_ENABLE,
    IMX_APBHDMA_HCLK_ENABLE,
    IMX_ASRC_CLK_ENABLE,
    IMX_CAN1_CLK_ENABLE,
    IMX_CAN1_SERIAL_CLK_ENABLE,
    IMX_CAN2_CLK_ENABLE,
    IMX_CAN2_SERIAL_CLK_ENABLE,
    IMX_DCIC1_CLK_ENABLE,
    IMX_DCIC2_CLK_ENABLE,
    IMX_DTCP_CLK_ENABLE,
    IMX_ECSPI1_CLK_ENABLE,
    IMX_ECSPI2_CLK_ENABLE,
    IMX_ECSPI3_CLK_ENABLE,
    IMX_ECSPI4_CLK_ENABLE,
    IMX_ECSPI5_CLK_ENABLE,
    IMX_ENET_CLK_ENABLE,
    IMX_ESAI_CLK_ENABLE,
    // IMX_GPT_SERIAL_CLK_ENABLE,
    // IMX_GPU2D_CLK_ENABLE,
    // IMX_GPU3D_CLK_ENABLE,
    IMX_I2C1_SERIAL_CLK_ENABLE,
    IMX_I2C2_SERIAL_CLK_ENABLE,
    IMX_I2C3_SERIAL_CLK_ENABLE,
    IMX_IIM_CLK_ENABLE,
    IMX_IPSYNC_IP2APB_TZASC1_IPG_MASTER_CLK_ENABLE,
    IMX_IPSYNC_IP2APB_TZASC2_IPG_MASTER_CLK_ENABLE,
    IMX_IPSYNC_VDOA_IPG_MASTER_CLK_ENABLE,
    IMX_IPU1_DI1_CLK_ENABLE,
    IMX_IPU2_CLK_ENABLE,
    IMX_IPU2_DI0_CLK_ENABLE,
    IMX_IPU2_DI1_CLK_ENABLE,
    IMX_LDB_DI0_CLK_ENABLE,
    IMX_LDB_DI1_CLK_ENABLE,
    IMX_MIPI_CORE_CFG_CLK_ENABLE,
    IMX_MLB_CLK_ENABLE,
    IMX_PL301_MX6QPER1_BCHCLK_ENABLE,
    IMX_PWM1_CLK_ENABLE,
    IMX_PWM2_CLK_ENABLE,
    IMX_PWM3_CLK_ENABLE,
    IMX_PWM4_CLK_ENABLE,
    IMX_RAWNAND_U_BCH_INPUT_APB_CLK_ENABLE,
    IMX_RAWNAND_U_GPMI_BCH_INPUT_BCH_CLK_ENABLE,
    IMX_RAWNAND_U_GPMI_BCH_INPUT_GPMI_IO_CLK_ENABLE,
    IMX_RAWNAND_U_GPMI_INPUT_APB_CLK_ENABLE,
    IMX_SPDIF_CLK_ENABLE,
    IMX_SSI1_CLK_ENABLE,
    IMX_SSI2_CLK_ENABLE,
    IMX_SSI3_CLK_ENABLE,
    IMX_EIM_SLOW_CLK_ENABLE,
    IMX_VDOAXICLK_CLK_ENABLE,
    IMX_VPU_CLK_ENABLE,
  };

  ImxClkPwrSetClockGates (
      gatesToTurnOff,
      sizeof (gatesToTurnOff) / sizeof (gatesToTurnOff[0]),
      IMX_CLOCK_GATE_STATE_OFF);
}

/**
  Turn on required clocks, that are not handled by the PEP.
**/
static VOID UngateRequiredClocks ()
{
    static const IMX_CLK_GATE gatesToTurnOn[] = {
        IMX_SDMA_CLK_ENABLE,
        IMX_SPBA_CLK_ENABLE,
        IMX_GPT_SERIAL_CLK_ENABLE,
        IMX_USBOH3_CLK_ENABLE,
    };

    ImxClkPwrSetClockGates(
        gatesToTurnOn,
        sizeof(gatesToTurnOn) / sizeof(gatesToTurnOn[0]),
        IMX_CLOCK_GATE_STATE_ON);
}

VOID EnetInit ()
{
  //
  // Apply ENET pin-muxing configurations
  //
  ImxPadConfig(IMX_PAD_ENET_MDIO, IMX_PAD_ENET_MDIO_ENET_MDIO);
  ImxPadConfig(IMX_PAD_ENET_MDC, IMX_PAD_ENET_MDC_ENET_MDC);

  ImxPadConfig (IMX_PAD_ENET_CRS_DV, IMX_PAD_CFG_ENET_CRS_DV_GPIO1_IO25);
  ImxPadConfig(IMX_PAD_ENET_REF_CLK, IMX_PAD_ENET_REF_CLK_ENET_REF_CLK);

  ImxPadConfig(IMX_PAD_RGMII_TXC, IMX_PAD_RGMII_TXC_RGMII_TXC);
  ImxPadConfig(IMX_PAD_RGMII_TD0, IMX_PAD_RGMII_TD0_RGMII_TD0);
  ImxPadConfig(IMX_PAD_RGMII_TD1, IMX_PAD_RGMII_TD1_RGMII_TD1);
  ImxPadConfig(IMX_PAD_RGMII_TD2, IMX_PAD_RGMII_TD2_RGMII_TD2);
  ImxPadConfig(IMX_PAD_RGMII_TD2, IMX_PAD_RGMII_TD3_RGMII_TD3);
  ImxPadConfig(IMX_PAD_RGMII_TX_CTL, IMX_PAD_RGMII_TX_CTL_RGMII_TX_CTL);

  ImxPadConfig(IMX_PAD_RGMII_RXC, IMX_PAD_RGMII_RXC_ENET_RGMII_RXC);
  ImxPadConfig(IMX_PAD_RGMII_RD0, IMX_PAD_RGMII_RD0_ENET_RGMII_RD0);
  ImxPadConfig(IMX_PAD_RGMII_RD1, IMX_PAD_RGMII_RD1_ENET_RGMII_RD1);
  ImxPadConfig(IMX_PAD_RGMII_RD2, IMX_PAD_RGMII_RD2_ENET_RGMII_RD2);
  ImxPadConfig(IMX_PAD_RGMII_RD3, IMX_PAD_RGMII_RD3_ENET_RGMII_RD3);
  ImxPadConfig(IMX_PAD_RGMII_RX_CTL, IMX_PAD_RGMII_RX_CTL_RGMII_RX_CTL);

  //
  // Reset PHY by pulling RGMII_nRST (GPIO1_IO25) low
  //
  ImxGpioDirection (IMX_GPIO_BANK1, 25, IMX_GPIO_DIR_OUTPUT);
  ImxGpioWrite (IMX_GPIO_BANK1, 25, IMX_GPIO_LOW);
  MicroSecondDelay (500);
  ImxGpioWrite (IMX_GPIO_BANK1, 25, IMX_GPIO_HIGH);

  //
  // Enable ENET PLL, also required for PCIe
  //
  {
    UINT32 counter = 0;
    IMX_CCM_ANALOG_PLL_ENET_REG ccmAnalogPllReg;
    volatile IMX_CCM_ANALOG_REGISTERS *ccmAnalogRegistersPtr =
        (IMX_CCM_ANALOG_REGISTERS *)IMX_CCM_ANALOG_BASE;

    ccmAnalogPllReg.AsUint32 = MmioRead32((UINTN)&ccmAnalogRegistersPtr->PLL_ENET);
    ccmAnalogPllReg.POWERDOWN = 0;
    ccmAnalogPllReg.ENABLE = 1;

    MmioWrite32(
        (UINTN)&ccmAnalogRegistersPtr->PLL_ENET,
        ccmAnalogPllReg.AsUint32);

    do {
        ccmAnalogPllReg.AsUint32 =
            MmioRead32((UINTN)&ccmAnalogRegistersPtr->PLL_ENET);

        MicroSecondDelay(100);
        ++counter;
    } while ((ccmAnalogPllReg.LOCK == 0) && (counter < 100));

    if (ccmAnalogPllReg.LOCK == 0) {
        DEBUG ((DEBUG_ERROR, "PLL_ENET is not locked!\n"));
        return;
    }

    //
    // Enable PCIe output 125Mhz
    //
    ccmAnalogPllReg.BYPASS = 0;
    ccmAnalogPllReg.ENABLE_125M = 1;
	ccmAnalogPllReg.DIV_SELECT = PLL_ENET_DIV_SELECT_50MHZ;

    MmioWrite32(
        (UINTN)&ccmAnalogRegistersPtr->PLL_ENET,
        ccmAnalogPllReg.AsUint32);

    MicroSecondDelay(50000);

    if (ccmAnalogPllReg.LOCK == 0) {
        DEBUG ((DEBUG_ERROR, "PLL_ENET is not locked!\n"));
    }
  }
}

/**
  Set SSI3 clock dividers to provide a bit clock capable of
  providing a clock suitable for stereo 44.1 KHz playback at 32 bit
  from PLL3 PFD2 which runs at 508.2 MHz.

  We only need to scale down the PLL3 FPD2 clock by 1:15 to provide a
  reference clock.  The WDM audio class driver will later setup additional
  dividers in the SSI3 block to provide the required bit clock rate.

  This routine also handles all GPIO/PadConfig init required for audio.
**/
VOID SetupAudio ()
{
    volatile IMX_CCM_REGISTERS *ccmRegisters = (IMX_CCM_REGISTERS *) IMX_CCM_BASE;

    IMX_CCM_CS1CDR_REG csicdr = (IMX_CCM_CS1CDR_REG) MmioRead32((UINTN) &ccmRegisters->CS1CDR);

    csicdr.ssi3_clk_pred = 0; // divide by 1.
    csicdr.ssi3_clk_podf = (15 - 1); // divide by 15.

    MmioWrite32((UINTN) &ccmRegisters->CS1CDR, csicdr.AsUint32);

    //
    // Set GPIO00 alt config to ALT0 (CCM_CLKO1).
    // This clock drives the master clock for the audio codec.
    //

    ImxPadConfig(IMX_PAD_GPIO_0, IMX_PAD_CFG_GPIO0_CCM_CLKO1);

    //
    // Set CODEC_PWR_EN to 1 via GPIO4/10 to enable power for the audio codec.
    //

    ImxPadConfig(IMX_PAD_KEY_COL2, IMX_PAD_CFG_KEY_COL2_GPIO4_IO10);
    ImxGpioDirection(IMX_GPIO_BANK4, 10, IMX_GPIO_DIR_OUTPUT);
    ImxGpioWrite (IMX_GPIO_BANK4, 10, IMX_GPIO_HIGH);

    //
    // Set alt config for CSI0_DATA04 thru CSI_DATA07 to ALT4 to select AUD3_TXC/TXD/TXFS/RXD.
    //

    ImxPadConfig(IMX_PAD_CSI0_DAT4, IMX_PAD_CFG_CSI0_DATA04_AUD3_TXC);
    ImxPadConfig(IMX_PAD_CSI0_DAT5, IMX_PAD_CFG_CSI0_DATA05_AUD3_TXD);
    ImxPadConfig(IMX_PAD_CSI0_DAT6, IMX_PAD_CFG_CSI0_DATA06_AUD3_TXFS);
    ImxPadConfig(IMX_PAD_CSI0_DAT7, IMX_PAD_CFG_CSI0_DATA07_AUD3_RXD);
}

/**
  Initalize LVDS
**/
VOID LvdsInit ()
{
    volatile IMX_CCM_REGISTERS *ccmRegisters = (IMX_CCM_REGISTERS *) IMX_CCM_BASE;

    IMX_CCM_CCGR3_REG ccgr3 = (IMX_CCM_CCGR3_REG) MmioRead32((UINTN) &ccmRegisters->CCGR[3]);
    ccgr3.ldb_di0_clk_enable = 0x3;
    MmioWrite32((UINTN) &ccmRegisters->CCGR[3], ccgr3.AsUint32);

    // initalize backlight pin
    ImxPadConfig(IMX_PAD_SD1_DAT3, IMX_PAD_CFG_SD1_DATA3_GPIO1_IO21);
    ImxGpioDirection(IMX_GPIO_BANK1, 21, IMX_GPIO_DIR_OUTPUT);
    ImxGpioWrite(IMX_GPIO_BANK1, 21, IMX_GPIO_HIGH);
}

/**
  Initialize clock and power for modules on the SOC.
**/
VOID ImxClkPwrInit ()
{
  EFI_STATUS status;

  GateUnusedClocks();
  UngateRequiredClocks();

  if (FeaturePcdGet (PcdGpuEnable)) {
    status = ImxClkPwrGpuEnable();
    ASSERT_EFI_ERROR (status);
  }

  if (FeaturePcdGet(PcdLvdsEnable)) {
    status = ImxClkPwrIpuLDBxEnable ();
    ASSERT_EFI_ERROR (status);
  }

  status = ImxClkPwrIpuDIxEnable();
  ASSERT_EFI_ERROR (status);

  ASSERT_EFI_ERROR (ImxClkPwrValidateClocks());

  // TODO - move next code to ArmPlatformInitialize() as soon as this method is moved to this file.
  if (FeaturePcdGet(PcdLvdsEnable)) {
    LvdsInit ();
  }

}

/**
  Initialize SDHC modules on the SOC and perform required pin-muxing
**/
VOID SdhcInit ()
{
  //
  // NOTE: uSDHC 1.8V switching is not supported on SabreSD
  //

  //
  // uSDHC3: SDCard Socket
  //
  ImxPadConfig(IMX_PAD_SD3_CLK, IMX_PAD_CFG_SD3_CLK_SD3_CLK);
  ImxPadConfig(IMX_PAD_SD3_CMD, IMX_PAD_CFG_SD3_CMD_SD3_CMD);
  ImxPadConfig(IMX_PAD_SD3_DAT0, IMX_PAD_CFG_SD3_DAT0_SD3_DATA0);
  ImxPadConfig(IMX_PAD_SD3_DAT1, IMX_PAD_CFG_SD3_DAT1_SD3_DATA1);
  ImxPadConfig(IMX_PAD_SD3_DAT2, IMX_PAD_CFG_SD3_DAT2_SD3_DATA2);
  ImxPadConfig(IMX_PAD_SD3_DAT3, IMX_PAD_CFG_SD3_DAT3_SD3_DATA3);

  //
  // SD CardDetect using GPIO
  //
  ImxPadConfig (IMX_PAD_NANDF_D0, IMX_PAD_CFG_NANDF_D0_GPIO2_IO00);
  ImxGpioDirection (IMX_GPIO_BANK2, 0, IMX_GPIO_DIR_INPUT);

  //
  // SD WriteProtect using GPIO
  //
  ImxPadConfig (IMX_PAD_NANDF_D1, IMX_PAD_CFG_NANDF_D1_GPIO2_IO01);
  ImxGpioDirection (IMX_GPIO_BANK2, 1, IMX_GPIO_DIR_INPUT);

  //
  // uSDHC4: eMMC
  //
  // TODO
  //
}

/**
  This routine initializes PHY0(OTG).
**/
VOID EhciInit ()
{
    //
    // Pin-mux OTG Over Current
    //
    ImxPadConfig(IMX_PAD_EIM_D21, IMX_PAD_CFG_EIM_DATA21_USB_OTG_OC);

    //
    // Pin-mux OTG power
    //
    ImxPadConfig(IMX_PAD_EIM_D22, IMX_PAD_CFG_EIM_DATA22_USB_OTG_PWR);

    //
    // Pin-mux USB_OTG_ID
    //
    ImxPadConfig(IMX_PAD_ENET_RX_ER, IMX_PAD_CFG_ENET_RX_ER_USB_OTG_ID);

    //
    //  USB_OTG_ID pin input select
    //
    {
        volatile IMX_IOMUXC_GPR_REGISTERS* IoMuxGprRegsPtr = (IMX_IOMUXC_GPR_REGISTERS*)IOMUXC_GPR_BASE_ADDRESS;

        IMX_IOMUXC_GPR1_REG IoMuxGpr1Reg = { MmioRead32((UINTN)&IoMuxGprRegsPtr->GPR1) };
        IoMuxGpr1Reg.USB_OTG_ID_SEL = IMX_IOMUXC_GPR1_USB_OTG_ID_SEL_ENET_RX_ER;
        MmioWrite32((UINTN)&IoMuxGprRegsPtr->GPR1, IoMuxGpr1Reg.AsUint32);
    }

    //
    // Initialize PHY0 (OTG)
    //
    ImxUsbPhyInit(IMX_USBPHY0);
}

/**
  Initialize I2C modules on the SOC and perform required pin-muxing
**/
VOID I2cInit ()
{
  // Enable 1.8V and 3.3V power rails for sensors connected to I2C1.
  // The SENSOR_PWR_EN on EIM_EB3 line powers the pullups on I2c1.
  ImxPadConfig (IMX_PAD_EIM_EB3, IMX_PAD_CFG_EIM_EB3_GPIO2_IO31); // mux EIM_EB3 to GPIO - alt5
  ImxGpioDirection(IMX_GPIO_BANK2, 31, IMX_GPIO_DIR_OUTPUT);
  ImxGpioWrite(IMX_GPIO_BANK2, 31, 1); // set GPIO2_IO31 to 1
  DEBUG ((DEBUG_INFO, "muxed IMX_PAD_EIM_EB3 to GPIO via Alt5 and set GPIO2_IO31 to 1\r\n"));

  //
  // Configure I2C1. CSI0_DAT9 is I2C1_SCL and CSI0_DAT8 is I2C1_SDA.
  //
  ImxPadConfig (IMX_PAD_CSI0_DAT9, IMX_PAD_CFG_CSI0_DAT9_I2C1_SCL); // sion 1

  ImxPadConfig (IMX_PAD_CSI0_DAT8, IMX_PAD_CFG_CSI0_DAT8_ALT4_I2C1_SDA); // sion 1

  DEBUG ((DEBUG_INFO, "I2C1 pin muxed via CSI0_DAT8-9\r\n"));

  // do not configure I2C2 SCL via KEY_COL3, SDA via KEY_ROW3
  // GOP in UEFI or Windows display driver might use I2C2 and mux it using Alt2

  //
  // if you want to configure I2C3, please ensure that all peripherals connected to it are functional
  //
}


/**
  Initialize PCI Express module on the SOC and perform required pin-muxing
**/
VOID PcieInit ()
{
    //
    // PCIe GPIO Power
    //
    ImxPadConfig(IMX_PAD_EIM_D19, IMX_PAD_CFG_EIM_DATA19_GPIO3_IO19);
    ImxGpioDirection(IMX_GPIO_BANK3, 19, IMX_GPIO_DIR_OUTPUT);
    ImxGpioWrite(IMX_GPIO_BANK3, 19, IMX_GPIO_HIGH);

    //
    // PCIe GPIO Reset
    //
    ImxPadConfig(IMX_PAD_GPIO_17, IMX_PAD_CFG_GPIO17_GPIO7_IO12);
    ImxGpioDirection(IMX_GPIO_BANK7, 12, IMX_GPIO_DIR_OUTPUT);
}