/*
* Ultra Secured Digital Host Controllers (uSDHC)
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
*/

//
// uSDHC1
//
Device (SDH1)
{
   Name (_HID, "NXP0108")
   Name (_UID, 0x1)

   Method (_STA) // State
   {
       Return(0xf) // Enabled
   }

   Method (_CRS, 0x0, NotSerialized) {
       Name (RBUF, ResourceTemplate () {
           MEMORY32FIXED(ReadWrite, 0x30B40000, 0x4000, )
           Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 54 }
       })
       Return(RBUF)
   }

   //
   // Child node to represent the only SD/MMC slot on this SD/MMC bus
   // In theory an SDHC can be connected to multiple SD/MMC slots at
   // the same time, but only 1 device will be selected and active at
   // a time
   //
   Device (SD0)
   {
       Method (_ADR) // Address
       {
         Return (0) // SD/MMC Slot 0
       }

       Method (_RMV) // Remove
       {
         Return (0) // Fixed
       }
   }
}
