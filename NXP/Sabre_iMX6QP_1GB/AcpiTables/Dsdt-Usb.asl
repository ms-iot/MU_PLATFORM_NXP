/*
* Description: iMX6 Sabre Universal Serial Bus Controller (USB/OTG) Core 0
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

Device(URS0)
{
   Name(_HID, "PNP0C90")
   Name(_UID, 0x0)

   //
   // URS requires device to at least be wakable from D2 state
   // WDF also requires that _DSW (enable & disable wake ability) to be present
   //
   Name(_S0W, 0x3)
   Name(_PRW, Package() {0,0})
   Method(_DSW, 0x3, NotSerialized) {

   }

   Method (_STA)
   {
       Return(0xf)
   }

   Method (_CRS, 0x0, NotSerialized) {
       Name (RBUF, ResourceTemplate () {
           //
           // Controller register address space. URS driver would add 0x0100
           // offset for host mode
           //
           MEMORY32FIXED(ReadWrite, 0x02184000, 0x200, )

           //
           // USB_OTG_ID pin, needs to be declared as *Wake as this device is
           // expected to be wakable. The USB PHY is capable to detect
           // USB ID changes but the interrupt cannot be acknowledge
           // and the behaviour is undefined based on NXP feedback. So
           // the the only way to reliably detect USB ID changed is to
           // either to share interrupts or assign a GPIO to detect.
           // The URS driver does not properly handle level sensitive
           // interrupts which can lead to an interrupt storm. Therefore we use
           // an edge sensitive GPIO interrupt.
           //
           // USB_OTG_ID connected to ENET_RX_ER (GPIO1_IO24). Use 1ms debounce.
           //
           GpioInt (Edge, ActiveBoth, SharedAndWake, PullDefault, 100, "\\_SB.GPIO",) { 24 }
       })
       Return(RBUF)
   }

   Name (OTGR, ResourceTemplate()
   {
       GpioIO (Shared, PullDefault, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer,,) { 24 }
   })

   Scope (\_SB_.GPIO)
   {
       OperationRegion (OTGP, GeneralPurposeIO, Zero, One)
   }

   Field (\_SB_.GPIO.OTGP, ByteAcc, NoLock, Preserve)
   {
       Connection (\_SB_.URS0.OTGR),
       OTGF, 1
   }

   //
   // Device Specific Method takes 4 args:
   //  Arg0 : Buffer containing a UUID [16 bytes]
   //  Arg1 : Integer containing the Revision ID
   //  Arg2 : Integer containing the Function Index
   //  Arg3 : Package that contains function-specific arguments (Unused?)
   //
   Method (_DSM, 0x4, NotSerialized) {
       Name (RET, 0x0); // Declare return variable
       Name (PVAL, 0x0); // Declare pin value variable

       // Check UUID
       switch(ToBuffer(Arg0))
       {
           // URS UUID
           case(ToUUID("14EB0A6A-79ED-4B37-A8C7-84604B55C5C3"))
           {
               // Function index
               switch(Arg2)
               {
                   //
                   // Function 0: Return supported functions, based on revision
                   // Return value and revision ID lack documentation
                   //
                   case(0)
                   {
                       switch(Arg1)
                       {
                           // Revision 0: function {1,2} supported
                           case(0)
                           {
                               Return(0x03);
                           }
                           default
                           {
                               Return(0x0);
                           }
                       }
                   }

                   //
                   // Function 1: Read USB_OTG_ID pin value
                   //
                   // Return value
                   // 0 = UrsHardwareEventIdFloat (Function)
                   // 1 = UrsHardwareEventIdGround (Host)
                   //
                   case(1)
                   {
                       Store(OTGF, PVAL);                   // Read value of OTG_ID Pin
                       Store(LEqual(PVAL, 0), RET);         // Complement value
                       Return(RET);
                   }

                   //
                   // Function 2: Read USB_OTG_ID pin value
                   //
                   // Return value
                   // 0 = UrsHardwareEventIdFloat (Function)
                   // 1 = UrsHardwareEventIdGround (Host)
                   //
                   case(2)
                   {
                       Store(OTGF, PVAL);                   // Read value of OTG_ID Pin
                       Store(LEqual(PVAL, 0), RET);         // Complement value
                       Return(RET);
                   }

                   //
                   // Unknown function index
                   //
                   default
                   {
                       Return(0x0);
                   }
               } // Function index
           }

           //
           // Unknown UUID
           //
           default
           {
               Return(0x0);
           }
       } // Check UUID
   } // _DSM

   Device (USB0)
   {
       //
       // The host controller device node needs to have an address of '0'
       //
       Name(_ADR, 0x0)
       Name(_UID, 0x0)
       Name(_S0W, 3)
       Method (_STA)
       {
           Return(0xf)
       }
       Method (_CRS, 0x0, NotSerialized) {
           Name (RBUF, ResourceTemplate () {
               Interrupt(ResourceConsumer, Level, ActiveHigh, SharedAndWake) { 75 }
           })
           Return(RBUF)
       }

       OperationRegion (OTGM, SystemMemory, 0x02184100, 0x100)
       Field (OTGM, WordAcc, NoLock, Preserve)
       {
           Offset(0x84),   // skip to register 84h
           PTSC, 32,       // port status control
           Offset(0xA8),   // skip to register A8h
           DSBM, 32,       // UOG_USBMOD
       }

       Name (REG, 0x0);    // Declare register read variable
       Method (_UBF, 0x0, NotSerialized) {
           //
           // Reset handled by driver so no reset required here
           //
           Store(0x03, DSBM);          // set host mode & little endian
           Store(PTSC, REG);           // read PORTSC status
           Store(OR(REG,0x2),PTSC);    // clear current PORTSC status
       }
   }

   Device(UFN0)
   {
       //
       // The function controller device node needs to have an address of '1'
       //
       Name(_ADR, 0x1)

       Method (_CRS, 0x0, NotSerialized) {
           Name (RBUF, ResourceTemplate () {
               Interrupt(ResourceConsumer, Level, ActiveHigh, SharedAndWake) { 75 }
           })
           Return(RBUF)
       }

       OperationRegion (OTGM, SystemMemory, 0x02184100, 0x100)
       Field (OTGM, WordAcc, NoLock, Preserve)
       {
           Offset(0x84),   // skip to register 84h
           PTSC, 32,       // port status control
           Offset(0xA8),   // skip to register A8h
           DSBM, 32,       // UOG_USBMOD
       }

       Name (REG, 0x0);    // Declare register read variable
       Method (_UBF, 0x0, NotSerialized) {
           //
           // Reset handled by driver so no reset required here
           //
           Store(0x02, DSBM);          // set device mode & little endian
           Store(PTSC, REG);           // read PORTSC status
           Store(OR(REG,0x2),PTSC);    // clear current PORTSC status
       }

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
               // UFX Chipidea interface identifier
               case(ToUUID("732b85d5-b7a7-4a1b-9ba0-4bbd00ffd511"))
               {
                   // Function selector
                   switch(Arg2)
                   {
                       //
                       // Function 0: Querry support
                       //   Bit  Description
                       //   ---  -------------------------------
                       //     0  Get property
                       //     1  Get properties (Function 1)
                       //     2  Set USB device state
                       //
                       case(0)
                       {
                           switch(Arg1)
                           {
                               // Revision 0: functions {0,1} supported
                               case(0)
                               {
                                   Return(Buffer(){0x03});
                               }
                               default
                               {
                                   Return(Buffer(){0x01});
                               }
                           }
                       }

                       //
                       // Function 1: Return device capabilities bitmap
                       //   Bit  Description
                       //   ---  -------------------------------
                       //     0  Attach detach
                       //     1  Software charging
                       //
                       case(1)
                       {
                            Return(0x01);
                       }

                       //
                       // Function 2: Get port type
                       //     0x00  Unknown port
                       //     0x01  Standard downstream
                       //     0x02  Charging downstream
                       //     0x03  Dedicated charging
                       //
                       case(2)
                       {
                            Return(0x01);
                       }

                       //
                       // Function 3: Set device state
                       //
                       case(3)
                       {
                             Return (Buffer(){0x0});
                       }

                       //
                       // Unknown function
                       //
                       default
                       {
                            Return (Buffer(){0x0});
                       }
                   }
               }

               // UFX interface identifier
               case(ToUUID("FE56CFEB-49D5-4378-A8A2-2978DBE54AD2"))
               {
                   // Function selector
                   switch(Arg2)
                   {
                       // Function 1: Return number of supported USB PHYSICAL endpoints
                       // Based on chapter 65.1.1 - Up to 8 bidirectional endpoints
                       case(1)
                       {
                            Return(8);
                       }
                       default
                       {
                            Return (Buffer(){0x0});
                       }
                   }
               }

               //
               // Unknown UUID
               //
               default
               {
                   Return(0x0);
               }
           } // UUID
       } // _DSM

   }
}