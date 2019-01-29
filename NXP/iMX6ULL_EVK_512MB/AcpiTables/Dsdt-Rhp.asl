/** @file
* Description: NXP iMX6ULL EVK Resource Hub Proxy
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
    I2CSerialBus(0xFFFF,, 0,, "\\_SB.I2C2",,,,)

    // There is no dedicated GPIO pin on EVK board. Here is an example for adding GPIO pin.
    // Sample PIN3 of J1703, PAD UART2_RTS_BB, GPIO bank0 IO25
    GpioIO(Shared, PullUp, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 25 }
    GpioInt(Edge, ActiveBoth, Shared, PullUp, 0, "\\_SB.GPIO",) { 25 }
  })

  Name(_DSD, Package()
  {
    ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
    Package()
    {
      // I2C bus
      Package(2) { "bus-I2C-I2C2", Package() { 0 }},

      // GPIO Pin Count and supported drive modes
      Package (2) { "GPIO-PinCount", 124 },
      Package (2) { "GPIO-UseDescriptorPinNumbers", 1 },

      // InputHighImpedance, InputPullUp, InputPullDown, OutputCmos
      Package (2) { "GPIO-SupportedDriveModes", 0x0f },
    }
  })
}
