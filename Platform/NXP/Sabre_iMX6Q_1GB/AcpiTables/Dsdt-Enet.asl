/*
*
* Description: 10/100/1000-Mbps Ethernet MAC (ENET)
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

Device (ENET)
{
    Name (_HID, "NXP010D")
    Name (_UID, 0x0)
    Method (_STA)
    {
        Return(0xF)
    }
    Method (_CRS, 0x0, NotSerialized) {
        Name (RBUF, ResourceTemplate () {
            MEMORY32FIXED(ReadWrite, 0x02188000, 0x4000, )                      // ENET MAC Core Base address
            Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 150 }   // ENET MAC 0 Core IRQ
            Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 151 }   // ENET MAC 0 1588 Timer interrupt
            GpioInt (Level, ActiveHigh, Shared, PullDefault, 0, "\\_SB.GPIO",) { 6 }  // Errata ERR006687: workaround for GPIO interrupt to connect Enet interrupt to the GPC
        })
        Return(RBUF)
    }
    Name (MAC, Buffer (6) {})
    CreateDWordField(MAC, 0x00, MACL)
    CreateWordField(MAC, 0x04, MACH)

    //
    // Device Specific Method takes 4 args:
    //  Arg0 : Buffer containing a UUID [16 bytes]
    //  Arg1 : Integer containing the Revision ID
    //  Arg2 : Integer containing the Function Index
    //  Arg3 : Package that contains function-specific arguments
    //
    Method (_DSM, 0x4, NotSerialized) {
        switch(ToBuffer(Arg0))
        {
            // iMX Enet interface identifier
            case(ToUUID("4EBBCB9D-4087-4791-80BD-8F8C08024206"))
            {
                // Function selector
                switch(Arg2)
                {
                    //
                    // Function 0: Query support
                    //   Bit  Description
                    //   ---  -------------------------------
                    //     0  Get property (Function 0)
                    //     1  Enet PHY address on MDIO bus (Function 1)
                    //     2  Get Mac Address (Function 2)
                    //     3  Get MDIO BAse Address (Function 3)
                    //
                    case(0)
                    {
                        switch(Arg1)
                        {
                            // Revision 0:
                            case(0)
                            {
                                // Functions {0,1,2} supported
                                Return(Buffer(){0x07});
                            }
                            default
                            {
                                Return(Buffer(){0x00});
                            }
                        }
                    }

                    //
                    // Function 1: Return Enet PHY address on MDIO bus.
                    //
                    case(1)
                    {
                        Return(0x01);
                    }

                    //
                    // Function 2: Return Mac Address
                    //
                    case(2)
                    {
                        if (LEqual(SIGN, 0x474C424C)) {
                            if (LEqual(REVN, 1)) {
                                if (Lequal(MC0V, 1)) {
                                    Store(MC0L, MACL);
                                    Store(MC0H, MACH);
                                    Return (MAC)
                                }
                            }
                        }
                        Return(Buffer(){0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
                    }

                    default
                    {
                        Return(Buffer(){0x00});
                    }
                }
            }
        }
    }
}