/*
* Description: iMX6 Sabre I2C Controllers
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

Device (I2C1)
{
   Name (_HID, "NXP0104")
   Name (_UID, 0x1)
   Method (_STA)
   {
       Return(0xf)
   }
   Method (_CRS, 0x0, NotSerialized) {
       Name (RBUF, ResourceTemplate () {
           MEMORY32FIXED(ReadWrite, 0x021A0000, 0x14, )
           Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 68 }
       })
       Return(RBUF)
   }
}
