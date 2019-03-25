/** @file
*
*  iMX6ULL UART Controllers
*
*  Copyright (c) 2019 Microsoft Corporation. All rights reserved.
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

Device (UAR2)
{
  Name (_HID, "NXP0107")
  Name (_UID, 0x2)
  Name (_DDN, "UART2")
  Method (_STA) {
    Return (0xf)
  }
  Name (_CRS, ResourceTemplate () {
    MEMORY32FIXED (ReadWrite, 0x021E8000, 0x4000, )
    Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 59 }

    // DMA channel 4, SDMA_REQ_UART2_RX for UART2 RX DMA
    FixedDMA (SDMA_REQ_UART2_RX, 4, Width8Bit, )
    // DMA channel 3, SDMA_REQ_UART2_TX for UART2 TX DMA
    FixedDMA (SDMA_REQ_UART2_TX, 3, Width8Bit, )

    // UART2_TX_DATA - UART2_TX - GPIO1_IO20 - 20
    // UART2_RX_DATA - UART2_RX - GPIO1_IO21 - 21
    // MsftFunctionConfig (Exclusive, PullUp, IMX_ALT0, "\\_SB.GPIO", 0,
    //                     ResourceConsumer, ) { 20, 21 }
    //
    // MsftFunctionConfig (Arg0, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6) { Pin List }
    VendorLong () {
      MSFT_UUID,            // Vendor UUID (MSFT UUID)
      MSFT_FUNCTION_CONFIG, // Resource Identifier (MSFT Function Config)
      0x1d,0x00,            // Length (0xF + sizeof(PinList) + sizeof(ResourceName))
      0x01,                 // Revision (0x1)
      RESOURCECONSUMER_EXCLUSIVE, // Flags (Arg5 | Arg0: ResourceConsumer | Exclusive)
      PULL_UP,              // Pin configuration (Arg1: PullUp)
      IMX_ALT0,0x00,        // Function Number (Arg2: IMX_ALT0)
      PIN_TABLE_OFFSET,     // Pin Table Offset (0x12)
      0x00,                 // Resource Source Index (Arg4: 0)
      0x16,0x00,            // Resource Source Name Offset (0x12 + sizeof(PinList))
      0x20,0x00,            // Vendor Data Offset (0x12 + sizeof(PinList) + sizeof(ResourceName))
      0x00,0x00,            // Vendor Data Length (sizeof(Arg6) = 0)
      0x14,0x00,0x15,0x00,  // Pin List (20, 21)
      SB_GPIO               // Resource Name (Arg3: \_SB.GPIO in ASCII)
    }

    UARTSerialBus (
      115200,
      DataBitsEight,
      StopBitsOne,
      0xC0,                // LinesInUse
      LittleEndian,
      ParityTypeNone,
      FlowControlNone,
      0,
      0,
      "\\_SB.CPU0",
      0,
      ResourceConsumer,
      ,)
  })

  Name (_DSD, Package () {
    ToUUID ("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
        Package (2) {"SerCx-FriendlyName", "UART2"}
      }
  })
}
