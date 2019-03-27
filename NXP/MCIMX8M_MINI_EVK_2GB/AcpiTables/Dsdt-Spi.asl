/*
* Description: iMX8M Mini SPI Controllers
*
*  Copyright (c) Microsoft Corporation. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
*/

Device (SPI1)
{
  Name (_HID, "NXP0105")
  Name (_HRV, 0x1)  // REV_0001
  Name (_UID, 0x1)
  Method (_STA)
  {
    Return(0xf)
  }
  Name (_CRS, ResourceTemplate () {
    MEMORY32FIXED(ReadWrite, 0x30820000, 0x4000, )
    Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 63 }

    // SS0 (PAD_ECSPI1_SS0) - GPIO5_IO09 (137) - J1003 pin 11
    GpioIO (Shared, PullDown, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 137 }

    // MISO (PAD_ECSPI1_MISO) - GPIO5_IO08 (136) - J1003 pin 7
    // MOSI (PAD_ECSPI1_MOSI) - GPIO5_IO07 (135) - J1003 pin 8
    // SCLK (PAD_ECSPI1_SCLK) - GPIO5_IO06 (134) - J1003 pin 10
    // MsftFunctionConfig (Exclusive, PullDown, IMX_ALT0, "\\_SB.GPIO", 0,
    //                     ResourceConsumer, ) { 134, 135, 136 }
    //
    // MsftFunctionConfig (Arg0, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6) { Pin List }
    VendorLong () {
      MSFT_UUID,            // Vendor UUID (MSFT UUID)
      MSFT_FUNCTION_CONFIG, // Resource Identifier (MSFT Function Config)
      0x1f,0x00,            // Length (0xF + sizeof(PinList) + sizeof(ResourceName))
      0x01,                 // Revision (0x1)
      RESOURCECONSUMER_EXCLUSIVE, // Flags (Arg5 | Arg0: ResourceConsumer | Exclusive)
      PULL_DOWN,            // Pin configuration (Arg1: PullDown)
      IMX_ALT0,0x00,        // Function Number (Arg2: IMX_ALT0)
      PIN_TABLE_OFFSET,     // Pin Table Offset (0x12)
      0x00,                 // Resource Source Index (Arg4: 0)
      0x18,0x00,            // Resource Source Name Offset (0x12 + sizeof(PinList))
      0x22,0x00,            // Vendor Data Offset (0x12 + sizeof(PinList) + sizeof(ResourceName))
      0x00,0x00,            // Vendor Data Length (sizeof(Arg6) = 0)
      0x86,0x00,0x87,0x00,0x88,0x00,  // Pin List (134, 135, 136)
      SB_GPIO               // Resource Name (Arg3: \_SB.GPIO in ASCII)
    }
  })
}

Device (SPI2)
{
  Name (_HID, "NXP0105")
  Name (_HRV, 0x1)  // REV_0001
  Name (_UID, 0x2)
  Method (_STA)
  {
    Return(0xf)
  }
  Name (_CRS, ResourceTemplate () {
    MEMORY32FIXED(ReadWrite, 0x30830000, 0x4000, )
    Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 64 }

    // SS0 (PAD_ECSPI2_SS0) - GPIO5_IO13 (141) - J1003 pin 24
    GpioIO (Shared, PullDown, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 141 }

    // MISO (PAD_ECSPI2_MISO) - GPIO5_IO12 (140) - J1003 pin 21
    // MOSI (PAD_ECSPI2_MOSI) - GPIO5_IO11 (139) - J1003 pin 19
    // SCLK (PAD_ECSPI2_SCLK) - GPIO5_IO10 (138) - J1003 pin 23
    // MsftFunctionConfig (Exclusive, PullDown, IMX_ALT0, "\\_SB.GPIO", 0,
    //                     ResourceConsumer, ) { 138, 139, 140 }
    //
    // MsftFunctionConfig (Arg0, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6) { Pin List }
    VendorLong () {
      MSFT_UUID,            // Vendor UUID (MSFT UUID)
      MSFT_FUNCTION_CONFIG, // Resource Identifier (MSFT Function Config)
      0x1f,0x00,            // Length (0xF + sizeof(PinList) + sizeof(ResourceName))
      0x01,                 // Revision (0x1)
      RESOURCECONSUMER_EXCLUSIVE, // Flags (Arg5 | Arg0: ResourceConsumer | Exclusive)
      PULL_DOWN,            // Pin configuration (Arg1: PullDown)
      IMX_ALT0,0x00,        // Function Number (Arg2: IMX_ALT0)
      PIN_TABLE_OFFSET,     // Pin Table Offset (0x12)
      0x00,                 // Resource Source Index (Arg4: 0)
      0x18,0x00,            // Resource Source Name Offset (0x12 + sizeof(PinList))
      0x22,0x00,            // Vendor Data Offset (0x12 + sizeof(PinList) + sizeof(ResourceName))
      0x00,0x00,            // Vendor Data Length (sizeof(Arg6) = 0)
      0x8A,0x00,0x8B,0x00,0x8C,0x00,  // Pin List (138, 139, 140)
      SB_GPIO               // Resource Name (Arg3: \_SB.GPIO in ASCII)
    }
  })
}

Device (SPI3)
{
  Name (_HID, "NXP0105")
  Name (_HRV, 0x1)  // REV_0001
  Name (_UID, 0x3)
  Method (_STA)
  {
    Return(0x0)
  }
  Name (_CRS, ResourceTemplate () {
      MEMORY32FIXED(ReadWrite, 0x30840000, 0x4000, )
      Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 65 }
  })
}

