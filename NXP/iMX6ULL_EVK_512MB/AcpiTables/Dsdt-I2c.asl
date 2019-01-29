/** @file
* Description: iMX6ULL I2C Controllers
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

// I2C2 is exposed on connector J1704
Device (I2C2)
{
  Name (_HID, "NXP0104")
  Name (_UID, 0x2)

  Method (_STA) {
    Return(0xf)
  }

  Name (_CRS, ResourceTemplate () {
      MEMORY32FIXED(ReadWrite, 0x021A4000, 0x14, )
      Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 69 }
  })
}
