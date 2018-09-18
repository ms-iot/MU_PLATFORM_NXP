/*
* Description: iMX6 Sabre Ultra Secured Digital Host Controller (uSDHC)
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

//
// uSDHC2: SDIO Slot
//
Device (SDH2)
{
   Name (_HID, "NXP0108")
   Name (_UID, 0x2)

   Method (_STA) // Status
   {
       Return(0xf) // Enabled
   }

   Name (_S1D, 0x1)
   Name (_S2D, 0x1)
   Name (_S3D, 0x1)
   Name (_S4D, 0x1)

   Method (_CRS, 0x0, NotSerialized) {
       Name (RBUF, ResourceTemplate () {
           MEMORY32FIXED(ReadWrite, 0x02194000, 0x4000, )
           Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 55 }
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
         Return (0) // SD Slot 0
       }

       Method (_RMV) // Remove
       {
         Return (1) // Removable
       }
   }
}

//
// uSDHC3: SDCard Slot
//
Device (SDH3)
{
    Name (_DEP, Package(0x1) {
        \_SB_.GPIO
    })

   Name (_HID, "NXP0108")
   Name (_UID, 0x3)

   Method (_STA) // Status
   {
       Return(0xf) // Enabled
   }

   Name (_S1D, 0x1)
   Name (_S2D, 0x1)
   Name (_S3D, 0x1)
   Name (_S4D, 0x1)

   Method (_CRS, 0x0, NotSerialized) {
       Name (RBUF, ResourceTemplate () {
           MEMORY32FIXED(ReadWrite, 0x02198000, 0x4000, )
           Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 56 }

           //
           // Uncomment once the SD/GPIO boot load dependency is resolved, otherwise
           // booting from SD will get stuck
           //
           // GPIO2_IO00 PAD_NAND_DATA00: Card Detect
           //GpioIO(Shared, PullUp, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 32 } // 1 * 32 + 0
           //GpioInt(Edge, ActiveBoth, Shared, PullUp, 0, "\\_SB.GPIO",) { 32 }

           // GPIO2_IO01 PAD_NAND_DATA01: Write Protect
           //GpioIO(Shared, PullUp, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 33 } // 1 * 32 + 1
       })
       Return(RBUF)
   }

   Device (SD0)
   {
      Method (_ADR) // Address
      {
         Return (0) // SD/MMC Slot0
      }

      Method (_RMV) // Remove
      {
         Return (0) // Removable - must be 0 to enable SDHC power management
      }
   }
}

//
// uSDHC4: eMMC
//
Device (SDH4)
{
   Name (_HID, "NXP0108")
   Name (_UID, 0x4)

   Method (_STA)
   {
       Return(0xf)
   }

   Name (_S1D, 0x1)
   Name (_S2D, 0x1)
   Name (_S3D, 0x1)
   Name (_S4D, 0x1)

   Method (_CRS, 0x0, NotSerialized) {
       Name (RBUF, ResourceTemplate () {
           MEMORY32FIXED(ReadWrite, 0x0219C000, 0x4000, )
           Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 57 }
       })
       Return(RBUF)
   }

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
