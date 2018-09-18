/** @file
* Description: iMX8MQuad I2C Controllers
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

// I2C1 is exposed via DSI and M.2 connectors
Device (I2C1)
{
  Name (_HID, "NXP0104")
  Name (_HRV, 0x1)
  Name (_UID, 0x1)

  Method (_STA)
  {
    Return(0xf)
  }

  Method (_CRS, 0x0, NotSerialized)
  {
    Name ( RBUF, ResourceTemplate () {
      MEMORY32FIXED(ReadWrite, 0x30A20000, 0x14, )
      Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 67 }
    })
    Return(RBUF)
  }
}

// I2C2 is exposed on I2C connector J801
Device (I2C2)
{
  Name (_HID, "NXP0104")
  Name (_HRV, 0x1)
  Name (_UID, 0x2)

  Method (_STA)
  {
    Return(0xf)
  }

  Method (_CRS, 0x0, NotSerialized)
  {
    Name (RBUF, ResourceTemplate () {
      MEMORY32FIXED(ReadWrite, 0x30A30000, 0x14, )
      Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 68 }
    })
    Return(RBUF)
  }
}

// I2C3 is exposed on audio card connector J1801
Device (I2C3)
{
  Name (_HID, "NXP0104")
  Name (_HRV, 0x1)
  Name (_UID, 0x3)

  Method (_STA)
  {
    Return(0xf)
  }

  Method (_CRS, 0x0, NotSerialized)
  {
    Name (RBUF, ResourceTemplate () {
      MEMORY32FIXED(ReadWrite, 0x30A40000, 0x14, )
      Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 69 }
    })
    Return(RBUF)
  }
}

Device (I2C4)
{
  Name (_HID, "NXP0104")
  Name (_HRV, 0x1)
  Name (_UID, 0x4)

  Method (_STA)
  {
    Return(0x0)
  }

  Method (_CRS, 0x0, NotSerialized)
  {
    Name (RBUF, ResourceTemplate () {
      MEMORY32FIXED(ReadWrite, 0x30A50000, 0x14, )
      Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 70 }
    })
    Return(RBUF)
  }
}

