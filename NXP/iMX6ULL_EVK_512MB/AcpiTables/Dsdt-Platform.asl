/*
* Description: Processor Devices
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

OperationRegion(GLBL,SystemMemory,0x80817000,0x10)
Field(GLBL, AnyAcc, Nolock, Preserve)
{
  Offset(0),        // Miscellaneous Dynamic Registers:
  SIGN, 32,         // Global Page Signature 'GLBL'
  REVN, 8,          // Revision
      , 8,          // Reserved
      , 8,          // Reserved
      , 8,          // Reserved
  M0ID, 8,          // MAC 0 ID
  MC0V, 8,          // MAC 0 Valid
  MC0L, 32,         // MAC Address 0 Low
  MC0H, 16,         // MAC Address 0 High
}

// Description: This is a Processor #0 Device

Device (CPU0)
{
  Name (_HID, "ACPI0007")
  Name (_UID, 0x0)
  Method (_STA)
  {
    Return(0xf)
  }
}

// Description: Timers HAL extension

Device (EPIT)
{
  Name (_HID, "NXP0101")
  Name (_UID, 0x0)
  Method (_STA)
  {
    Return(0xf)
  }
}
