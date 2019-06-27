/** @file
*
* Description: iMX8M Mini UART
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
**/

Device (UAR3)
{
    Name (_HID, "NXP0113") //NXP0113 for 24MHz UART_CLK_ROOT
    Name (_UID, 0x3)
    Name (_DDN, "UART3")
    Method (_STA)
    {
       Return(0xF)
    }
    Method (_CRS, 0x0, NotSerialized) {
        Name (RBUF, ResourceTemplate () {
            MEMORY32FIXED(ReadWrite, 0x30880000, 0x4000, )
            Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 60 }

            UARTSerialBus (
            115200,
            DataBitsEight,
            StopBitsOne,
            0,                // LinesInUse
            LittleEndian,
            ParityTypeNone,
            FlowControlNone,
            0,
            0,
            "\\_SB.CPU0",
            0,
            ResourceConsumer,
            ,)
        })
        Return(RBUF)
    }

  Name (_DSD, Package () {
    ToUUID ("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
        Package (2) {"SerCx-FriendlyName", "UART3"}
      }
  })
}
