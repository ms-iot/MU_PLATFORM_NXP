/** @file
*
*  iMX6 Sabre UART Controllers
*
*  Copyright (c) 2018 Microsoft Corporation. All rights reserved.
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

Device (UAR1)
{
  Name (_HID, "NXP0106")
  Name (_UID, 0x1)
  Name (_DDN, "UART1")

  Method (_STA) {
    Return (0xf)
  }

  Name (_CRS, ResourceTemplate () {
    MEMORY32FIXED (ReadWrite, 0x02020000, 0x4000, )
    Interrupt (ResourceConsumer, Level, ActiveHigh, Shared) { 58 }

    // UART1_TX_DATA - CSI0_DAT10 - GPIO5_IO28 - 156
    // UART1_RX_DATA - CSI0_DAT11 - GPIO5_IO29 - 157
    // MsftFunctionConfig (Exclusive, PullUp, IMX_ALT3, "\\_SB.GPIO", 0,
    //                     ResourceConsumer, ) { 156, 157 }
    //
    // MsftFunctionConfig (Arg0, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6) { Pin List }
    VendorLong () {
      MSFT_UUID,            // Vendor UUID (MSFT UUID)
      MSFT_FUNCTION_CONFIG, // Resource Identifier (MSFT Function Config)
      0x1d,0x00,            // Length (0xF + sizeof(PinList) + sizeof(ResourceName))
      0x01,                 // Revision (0x1)
      RESOURCECONSUMER_EXCLUSIVE, // Flags (Arg5 | Arg0: ResourceConsumer | Exclusive)
      PULL_UP,              // Pin configuration (Arg1: PullUp)
      IMX_ALT3,0x00,        // Function Number (Arg2: IMX_ALT3)
      PIN_TABLE_OFFSET,     // Pin Table Offset (0x12)
      0x00,                 // Resource Source Index (Arg4: 0)
      0x16,0x00,            // Resource Source Name Offset (0x12 + sizeof(PinList))
      0x20,0x00,            // Vendor Data Offset (0x12 + sizeof(PinList) + sizeof(ResourceName))
      0x00,0x00,            // Vendor Data Length (sizeof(Arg6) = 0)
      0x9c,0x00,0x9d,0x00,  // Pin List (156, 157)
      SB_GPIO               // Resource Name (Arg3: \_SB.GPIO in ASCII)
    }

    // DMA channel 2, SDMA_REQ_UART1_RX for UART1 RX DMA
    FixedDMA (SDMA_REQ_UART1_RX, 2, Width8Bit, )
    // DMA channel 1, SDMA_REQ_UART1_TX for UART1 TX DMA
    FixedDMA (SDMA_REQ_UART1_TX, 1, Width8Bit, )

    UARTSerialBus (
      115200,
      DataBitsEight,
      StopBitsOne,
      0,                  // LinesInUse
      LittleEndian,
      ParityTypeNone,
      FlowControlNone,
      0,
      0,
      "\\_SB.CPU0",
      0,
      ResourceConsumer,
      ,
    )
  })

  Name (_DSD, Package () {
    ToUUID ("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
        Package (2) {"SerCx-FriendlyName", "UART1"}
      }
  })
}

Device (UAR2)
{
  Name (_HID, "NXP0107")
  Name (_UID, 0x2)
  Name (_DDN, "UART2")
  Method (_STA) {
    Return (0)    // Disabled
  }
  Name (_CRS, ResourceTemplate () {
    MEMORY32FIXED (ReadWrite, 0x021E8000, 0x4000, )
    Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 59 }

    // DMA channel 4, SDMA_REQ_UART2_RX for UART2 RX DMA
    FixedDMA (SDMA_REQ_UART2_RX, 4, Width8Bit, )
    // DMA channel 3, SDMA_REQ_UART2_TX for UART2 TX DMA
    FixedDMA (SDMA_REQ_UART2_TX, 3, Width8Bit, )
  })
}

Device (UAR3)
{
  Name (_HID, "NXP0107")
  Name (_UID, 0x3)
  Name (_DDN, "UART3")
  Method (_STA) {
    Return (0xf)
  }
  Name (_CRS, ResourceTemplate () {
    MEMORY32FIXED (ReadWrite, 0x021EC000, 0x4000, )
    Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 60 }

    // DMA channel 6, SDMA_REQ_UART3_RX for UART3 RX DMA
    FixedDMA (SDMA_REQ_UART3_RX, 6, Width8Bit, )
    // DMA channel 5, SDMA_REQ_UART3_TX for UART3 TX DMA
    FixedDMA (SDMA_REQ_UART3_TX, 5, Width8Bit, )

    // UART3_TX - EIM_D24 - GPIO3_IO24 - 88
    // UART3_RX - EIM_D25 - GPIO3_IO25 - 89
    // MsftFunctionConfig (Exclusive, PullUp, IMX_ALT2, "\\_SB.GPIO", 0,
    //                     ResourceConsumer, ) { 88, 89 }
    //
    // MsftFunctionConfig (Arg0, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6) { Pin List }
    VendorLong () {
      MSFT_UUID,            // Vendor UUID (MSFT UUID)
      MSFT_FUNCTION_CONFIG, // Resource Identifier (MSFT Function Config)
      0x1d,0x00,            // Length (0xF + sizeof(PinList) + sizeof(ResourceName))
      0x01,                 // Revision (0x1)
      RESOURCECONSUMER_EXCLUSIVE, // Flags (Arg5 | Arg0: ResourceConsumer | Exclusive)
      PULL_UP,              // Pin configuration (Arg1: PullUp)
      IMX_ALT2,0x00,        // Function Number (Arg2: IMX_ALT2)
      PIN_TABLE_OFFSET,     // Pin Table Offset (0x12)
      0x00,                 // Resource Source Index (Arg4: 0)
      0x16,0x00,            // Resource Source Name Offset (0x12 + sizeof(PinList))
      0x20,0x00,            // Vendor Data Offset (0x12 + sizeof(PinList) + sizeof(ResourceName))
      0x00,0x00,            // Vendor Data Length (sizeof(Arg6) = 0)
      0x58,0x00,0x59,0x00,  // Pin List (88, 89)
      SB_GPIO               // Resource Name (Arg3: \_SB.GPIO in ASCII)
    }

    UARTSerialBus (
      115200,
      DataBitsEight,
      StopBitsOne,
      0,                  // LinesInUse
      LittleEndian,
      ParityTypeNone,
      FlowControlNone,
      0,
      0,
      "\\_SB.CPU0",
      0,
      ResourceConsumer,
      ,
    )
  })

  Name(_DSD, Package() {
    ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package() {
        Package(2) {"SerCx-FriendlyName", "UART3"}
      }
  })
}

Device (UAR4)
{
  Name (_HID, "NXP0107")
  Name (_UID, 0x4)
  Name (_DDN, "UART4")
  Method (_STA) {
    Return (0xf)
  }
  Name (_CRS, ResourceTemplate () {
    MEMORY32FIXED (ReadWrite, 0x021F0000, 0x4000, )
    Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 61 }

    // DMA channel 8, SDMA_REQ_UART4_RX for UART4 RX DMA
    FixedDMA (SDMA_REQ_UART4_RX, 8, Width8Bit, )
    // DMA channel 7, SDMA_REQ_UART4_TX for UART4 TX DMA
    FixedDMA (SDMA_REQ_UART4_TX, 7, Width8Bit, )

    // UART4_TX_DATA - CSI0_DAT12 - GPIO5_IO30 - 158
    // UART4_RX_DATA - CSI0_DAT13 - GPIO5_IO31 - 159
    // MsftFunctionConfig (Exclusive, PullUp, IMX_ALT3, "\\_SB.GPIO", 0,
    //                     ResourceConsumer, ) { 158, 159 }
    //
    // MsftFunctionConfig (Arg0, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6) { Pin List }
    VendorLong () {
      MSFT_UUID,            // Vendor UUID (MSFT UUID)
      MSFT_FUNCTION_CONFIG, // Resource Identifier (MSFT Function Config)
      0x1d,0x00,            // Length (0xF + sizeof(PinList) + sizeof(ResourceName))
      0x01,                 // Revision (0x1)
      RESOURCECONSUMER_EXCLUSIVE, // Flags (Arg5 | Arg0: ResourceConsumer | Exclusive)
      PULL_UP,              // Pin configuration (Arg1: PullUp)
      IMX_ALT3,0x00,        // Function Number (Arg2: IMX_ALT3)
      PIN_TABLE_OFFSET,     // Pin Table Offset (0x12)
      0x00,                 // Resource Source Index (Arg4: 0)
      0x16,0x00,            // Resource Source Name Offset (0x12 + sizeof(PinList))
      0x20,0x00,            // Vendor Data Offset (0x12 + sizeof(PinList) + sizeof(ResourceName))
      0x00,0x00,            // Vendor Data Length (sizeof(Arg6) = 0)
      0x9e,0x00,0x9f,0x00,  // Pin List (158, 159)
      SB_GPIO               // Resource Name (Arg3: \_SB.GPIO in ASCII)
    }

    // UART4_RTS_B - CSI0_DAT16 - GPIO6_IO02 - 162
    // UART4_CTS_B - CSI0_DAT17 - GPIO6_IO03 - 163
    // MsftFunctionConfig (Exclusive, PullUp, IMX_ALT3, "\\_SB.GPIO", 0,
    //                     ResourceConsumer, ) { 162, 163 }
    //
    // MsftFunctionConfig (Arg0, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6) { Pin List }
    VendorLong () {
      MSFT_UUID,            // Vendor UUID (MSFT UUID)
      MSFT_FUNCTION_CONFIG, // Resource Identifier (MSFT Function Config)
      0x1d,0x00,            // Length (0xF + sizeof(PinList) + sizeof(ResourceName))
      0x01,                 // Revision (0x1)
      RESOURCECONSUMER_EXCLUSIVE, // Flags (Arg5 | Arg0: ResourceConsumer | Exclusive)
      PULL_UP,              // Pin configuration (Arg1: PullUp)
      IMX_ALT3,0x00,        // Function Number (Arg2: IMX_ALT3)
      PIN_TABLE_OFFSET,     // Pin Table Offset (0x12)
      0x00,                 // Resource Source Index (Arg4: 0)
      0x16,0x00,            // Resource Source Name Offset (0x12 + sizeof(PinList))
      0x20,0x00,            // Vendor Data Offset (0x12 + sizeof(PinList) + sizeof(ResourceName))
      0x00,0x00,            // Vendor Data Length (sizeof(Arg6) = 0)
      0xa2,0x00,0xa3,0x00,  // Pin List (162, 163)
      SB_GPIO               // Resource Name (Arg3: \_SB.GPIO in ASCII)
    }

    UARTSerialBus(
      115200,
      DataBitsEight,
      StopBitsOne,
      0xC0,                  // LinesInUse
      LittleEndian,
      ParityTypeNone,
      FlowControlNone,
      0,
      0,
      "\\_SB.CPU0",
      0,
      ResourceConsumer,
      ,
    )
  })

  Name (_DSD, Package () {
    ToUUID ("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
        Package (2) {"SerCx-FriendlyName", "UART4"}
      }
  })
}

Device (UAR5)
{
  Name (_HID, "NXP0107")
  Name (_UID, 0x5)
  Name (_DDN, "UART5")
  Method (_STA) {
    Return (0xf)
  }
  Name (_CRS, ResourceTemplate () {
    MEMORY32FIXED (ReadWrite, 0x021F4000, 0x4000, )
    Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 62 }

    // DMA channel 10, SDMA_REQ_UART5_RX for UART5 RX DMA
    FixedDMA (SDMA_REQ_UART5_RX, 10, Width8Bit, )
    // DMA channel 9, SDMA_REQ_UART5_TX for UART5 TX DMA
    FixedDMA (SDMA_REQ_UART5_TX, 9, Width8Bit, )

    // Sabre BT_UART_
    // UART5_TX_DATA - KEY_COL1 - GPIO4_IO08 - 104
    // UART5_RX_DATA - KEY_ROW1 - GPIO4_IO09 - 105
    // UART5_RTS_B - KEY_COL4 - GPIO4_IO14 - 110
    // UART5_CTS_B - KEY_ROW4 - GPIO4_IO15 - 111
    // MsftFunctionConfig (Exclusive, PullUp, IMX_ALT4, "\\_SB.GPIO", 0,
    //                     ResourceConsumer, ) { 104, 105, 110, 111 }
    //
    // MsftFunctionConfig (Arg0, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6) { Pin List }
    VendorLong () {
      MSFT_UUID,            // Vendor UUID (MSFT UUID)
      MSFT_FUNCTION_CONFIG, // Resource Identifier (MSFT Function Config)
      0x21,0x00,            // Length (0xF + sizeof(PinList) + sizeof(ResourceName))
      0x01,                 // Revision (0x1)
      RESOURCECONSUMER_EXCLUSIVE, // Flags (Arg5 | Arg0: ResourceConsumer | Exclusive)
      PULL_UP,              // Pin configuration (Arg1: PullUp)
      IMX_ALT4,0x00,        // Function Number (Arg2: IMX_ALT4)
      PIN_TABLE_OFFSET,     // Pin Table Offset (0x12)
      0x00,                 // Resource Source Index (Arg4: 0)
      0x1a,0x00,            // Resource Source Name Offset (0x12 + sizeof(PinList))
      0x24,0x00,            // Vendor Data Offset (0x12 + sizeof(PinList) + sizeof(ResourceName))
      0x00,0x00,            // Vendor Data Length (sizeof(Arg6) = 0)
      0x68,0x00,0x69,0x00,0x6e,0x00,0x6f,0x00,  // Pin List (104, 105, 110, 111)
      SB_GPIO               // Resource Name (Arg3: \_SB.GPIO in ASCII)
    }

    UARTSerialBus(
      115200,
      DataBitsEight,
      StopBitsOne,
      0xC0,                  // LinesInUse
      LittleEndian,
      ParityTypeNone,
      FlowControlNone,
      0,
      0,
      "\\_SB.CPU0",
      0,
      ResourceConsumer,
      ,
    )
  })

  Name (_DSD, Package () {
    ToUUID ("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
        Package (2) {"SerCx-FriendlyName", "UART5"}
      }
  })
}
