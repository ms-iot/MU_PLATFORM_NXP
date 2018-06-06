/*
* Description: iMX6 ULL Ultra Secured Digital Host Controller (uSDHC)
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
*/

// uSDHC1: SDCard Slot
Device (SDH1)
{
  Name (_HID, "NXP0108")
  Name (_UID, 0x1)

  Method (_STA) // Status
  {
    Return(0xf) // Enabled
  }

  Name (_S1D, 0x1)
  Name (_S2D, 0x1)
  Name (_S3D, 0x1)
  Name (_S4D, 0x1)

  Method (_CRS, 0x0, Serialized) {
    Name (RBUF, ResourceTemplate () {
      MEMORY32FIXED(ReadWrite, 0x02190000, 0x4000, )
      Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 54 }

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

// uSDHC2: eMMC
Device (SDH2)
{
  Name (_HID, "NXP0108")
  Name (_UID, 0x2)

  Method (_STA)
  {
    Return(0x0)
  }

  Name (_S1D, 0x1)
  Name (_S2D, 0x1)
  Name (_S3D, 0x1)
  Name (_S4D, 0x1)

  Method (_CRS, 0x0, Serialized) {
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
