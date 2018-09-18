/** @file
* Description: NXP iMX8MQ EVK Resource Hub Proxy
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
*/

Device(RHPX)
{
  Name(_HID, "MSFT8000")
  Name(_CID, "MSFT8000")
  Name(_UID, 1)

  Name(_CRS, ResourceTemplate()
  {
    // Index 0
    I2CSerialBus(0xFFFF,, 0,, "\\_SB.I2C1",,,,)

    // Index 1
    I2CSerialBus(0xFFFF,, 0,, "\\_SB.I2C2",,,,)

    // Index 2
    I2CSerialBus(0xFFFF,, 0,, "\\_SB.I2C3",,,,)

    // GPIO5_IO16 PAD_I2C2_SCL - J801 pin 1
    GpioIO(Shared, PullUp, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 144 } // 4 * 32 + 16
    GpioInt(Edge, ActiveBoth, Shared, PullUp, 0, "\\_SB.GPIO",) { 144 }

    // GPIO5_IO17 PAD_I2C2_SDA - J801 pin 3
    GpioIO(Shared, PullUp, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 145 } // 4 * 32 + 17
    GpioInt(Edge, ActiveBoth, Shared, PullUp, 0, "\\_SB.GPIO",) { 145 }
  })

  Name(_DSD, Package()
  {
    ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
    Package()
    {
      // I2C busses 1-3
      Package(2) { "bus-I2C-I2C1", Package() { 0 }},
      Package(2) { "bus-I2C-I2C2", Package() { 1 }},
      Package(2) { "bus-I2C-I2C3", Package() { 2 }},

      // GPIO Pin Count and supported drive modes
      Package (2) { "GPIO-PinCount", 157 },
      Package (2) { "GPIO-UseDescriptorPinNumbers", 1 },

      // InputHighImpedance, InputPullUp, OutputCmos
      Package (2) { "GPIO-SupportedDriveModes", 0x0b },
    }
  })
}
