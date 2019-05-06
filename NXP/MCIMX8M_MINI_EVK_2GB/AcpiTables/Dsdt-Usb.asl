/** @file
*
*  Compulab iMX8M Mini USB OTG and EHCI Controllers
*
*  CL-SOM-iMX8M Mini USB port 1 (i.MX8 Controller Core 0) supports for OTG signaling,
*  session request protocol (SRP), host negotiation protocol (HNP), and attach
*  detection protocol (ADP). ADP support includes dedicated timer hardware and
*  register interface.
*
*  Copyright (c) 2018-2019 Microsoft Corporation. All rights reserved.
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

Device (USB1)
{
  Name (_HID, "NXP010C")
  Name (_CID, "PNP0D20")
  Name (_UID, 0x1)
  // USB DMA is not coherent
  Name (_CCA, 0x0)
  // Hardware revision 1 supports integrated Transaction Translator (TT) in PortSC register
  Name (_HRV, 0x1)
  // D0 is the lowest supported state to wake itself up
  Name (_S0W, 0x0)

  Method (_STA) {
    Return (0xf)
  }

  Name (_CRS, ResourceTemplate () {
    MEMORY32FIXED (ReadWrite, 0x32E40100, 0x100, )
    Interrupt (ResourceConsumer, Level, ActiveHigh, SharedAndWake) { 72 }
  })

  OperationRegion (OTGM, SystemMemory, 0x32E40100, 0x100)
  Field (OTGM, WordAcc, NoLock, Preserve) {
    Offset (0x84),  // skip to register 84h
    PTSC, 32,       // port status control
    Offset (0xA8),  // skip to register A8h
    DSBM, 32,       // UOG_USBMOD
  }

  Name (REG, 0x0)
  Method (_UBF, 0x0, NotSerialized) {
    // Reset handled by driver so no reset required here
    Store (0x03, DSBM)          // set host mode & little endian
    Store (PTSC, REG)           // read PORTSC status
    Store (OR (REG, 0x2), PTSC) // clear current PORTSC status
  }
}

Device (USB2)
{
  Name (_HID, "NXP010C")
  Name (_CID, "PNP0D20")
  Name (_UID, 0x2)
  // USB DMA is not coherent
  Name (_CCA, 0x0)
  // Hardware revision 1 supports integrated Transaction Translator (TT) in PortSC register
  Name (_HRV, 0x1)
  // D0 is the lowest supported state to wake itself up
  Name (_S0W, 0x0)

  Method (_STA) {
    Return (0x0)
  }

  Name (_CRS, ResourceTemplate () {
    MEMORY32FIXED (ReadWrite, 0x32E50100, 0x100, )
    Interrupt (ResourceConsumer, Level, ActiveHigh, SharedAndWake) { 73 }
  })

  OperationRegion (OTGM, SystemMemory, 0x32E50100, 0x100)
  Field (OTGM, WordAcc, NoLock, Preserve) {
    Offset (0x84),  // skip to register 84h
    PTSC, 32,       // port status control
    Offset (0xA8),  // skip to register A8h
    DSBM, 32,       // UOG_USBMOD
  }

  Name (REG, 0x0)
  Method (_UBF, 0x0, NotSerialized) {
    // Reset handled by driver so no reset required here
    Store (0x03, DSBM)          // set host mode & little endian
    Store (PTSC, REG)           // read PORTSC status
    Store (OR (REG, 0x2), PTSC) // clear current PORTSC status
  }
}
