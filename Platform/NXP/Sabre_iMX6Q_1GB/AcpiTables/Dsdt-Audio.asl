/*
* iMX6 Sabre Description: Synchronous Serial Interface (SSI)
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

Device (SSI1)
{
   Name (_HID, "NXP010A")
   Name (_UID, 0x1)
   Method (_STA)
   {
       Return(0xf)
   }
   Method (_CRS, 0x0, NotSerialized) {
       Name (RBUF, ResourceTemplate () {
           MEMORY32FIXED(ReadWrite, 0x02028000, 0x4000, )
           Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 78 }
       })
       Return(RBUF)
   }
}

Device (SSI2)
{
   Name (_HID, "NXP010A")
   Name (_UID, 0x2)
   Method (_STA)
   {
       Return(0xf)
   }
   Method (_CRS, 0x0, NotSerialized) {
       Name (RBUF, ResourceTemplate () {
           MEMORY32FIXED(ReadWrite, 0x0202C000, 0x4000, )
           Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 79 }
       })
       Return(RBUF)
   }
}

Device (SSI3)
{
   Name (_HID, "NXP010A")
   Name (_UID, 0x3)
   Method (_STA)
   {
       Return(0xf)
   }
   Method (_CRS, 0x0, NotSerialized) {
       Name (RBUF, ResourceTemplate () {
           MEMORY32FIXED(ReadWrite, 0x02030000, 0x4000, )
           MEMORY32FIXED(ReadWrite, 0x021d8000, 0x38, ) // AUDMUX block
           Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 80 }
           MsftFunctionConfig(Shared, PullDown, 0, "\\_SB.SSI3", 0, ResourceConsumer, ) { 7, 3 }
       })
       Return(RBUF)
   }
}

//
// Description: Asynchronous Sample Rate Converter (ASRC)
//

Device (ASRC)
{
   Name (_HID, "NXP010B")
   Name (_UID, 0x0)
   Method (_STA)
   {
       Return(0x0)
   }
   Method (_CRS, 0x0, NotSerialized) {
       Name (RBUF, ResourceTemplate () {
           MEMORY32FIXED(ReadWrite, 0x02034000, 0x4000, )
           Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 82 }
       })
       Return(RBUF)
   }
}