/** @file
* Description: NXP iMX8M Mini EVK Resource Hub Proxy
*
*  Copyright (c) 2019, Microsoft Corporation. All rights reserved.
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

    // Index 3
    I2CSerialBus(0xFFFF,, 0,, "\\_SB.I2C4",,,,)
  })

  Name(_DSD, Package()
  {
    ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
    Package()
    {
      // I2C buses 2-3
      Package(2) { "bus-I2C-I2C2", Package() { 1 }},
      Package(2) { "bus-I2C-I2C3", Package() { 2 }},
    }
  })
}
