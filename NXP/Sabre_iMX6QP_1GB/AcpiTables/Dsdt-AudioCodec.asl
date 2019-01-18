/** @file
*
*  iMX6 Sabre Wolfson 8962 Audio Codec
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

Device (ACDK)
{
  Name (_HID, "WML8962")
  Name (_UID, 0x0)

  Method (_STA) {
    Return (0xf)
  }

  Name (_CRS, ResourceTemplate () {
    I2CSerialBus (0x1a, ControllerInitiated, 400000, AddressingMode7Bit,
                  "\\_SB.I2C1", 0, ResourceConsumer)
  })
}
