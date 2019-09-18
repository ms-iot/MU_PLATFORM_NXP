/*
* Description: Processor Devices
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

OperationRegion(GLBL,SystemMemory, FixedPcdGet32 (PcdGlobalDataBaseAddress),0x18)
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

//
// Description: This is a Processor #0 Device
//
Device (CPU0)
{
    Name (_HID, "ACPI0007")
    Name (_UID, 0x0)
    Method (_STA)
    {
        Return(0xf)
    }
    Name (_LPI, Package () {
      0, // Revision
      0, // LevelID
      2, // Count of packages

      // Standby / WFI state.
      Package () {
        0, // Min Residency (us)
        0, // Wakeup Latency (us)
        1, // Flags, set bit0 to 1 to enable this state
        0, // Arch. Context Lost Flags
        0, // Residency Counter Frequency
        0, // Enabled Parent State
        ResourceTemplate () {
          // Entry method for the WFI state. See the ARM FFH Specification (ARM DEN 0048).
          Register (FFixedHW, 0x20, 0, 0x00000000FFFFFFFF, 3)
        },
        ResourceTemplate () {
          Register (SystemMemory,0,0,0,0) // Residency counter register
        },
        ResourceTemplate () {
          Register (SystemMemory,0,0,0,0) // Usage counter register
        },
        "Standby" // Name
      },

      // Retention state.
      Package () {
        1000, // Min Residency (us)
        950, // Wakeup Latency (us)
        1, // Flags, set bit0 to 1 to enable this state
        0, // Arch. Context Lost Flags
        0, // Residency Counter Frequency
        0, // Enabled Parent State
        ResourceTemplate () {
          // Entry method for the WFI state. See the ARM FFH Specification (ARM DEN 0048).
          Register (FFixedHW, 0x20, 0, 0x0000000000000000, 3)
        },
        ResourceTemplate () {
          Register (SystemMemory,0,0,0,0) // Residency counter register
        },
        ResourceTemplate () {
          Register (SystemMemory,0,0,0,0) // Usage counter register
        },
        "Powerdown" // Name
      },
    })
}

//
// Description: This is a Processor #1 Device
//
Device (CPU1)
{
    Name (_HID, "ACPI0007")
    Name (_UID, 0x1)
    Method (_STA)
    {
        Return(0xf)
    }
    Name (_LPI, Package () {
      0, // Revision
      0, // LevelID
      2, // Count of packages

      // Standby / WFI state.
      Package () {
        0, // Min Residency (us)
        0, // Wakeup Latency (us)
        1, // Flags, set bit0 to 1 to enable this state
        0, // Arch. Context Lost Flags
        0, // Residency Counter Frequency
        0, // Enabled Parent State
        ResourceTemplate () {
          // Entry method for the WFI state. See the ARM FFH Specification (ARM DEN 0048).
          Register (FFixedHW, 0x20, 0, 0x00000000FFFFFFFF, 3)
        },
        ResourceTemplate () {
          Register (SystemMemory,0,0,0,0) // Residency counter register
        },
        ResourceTemplate () {
          Register (SystemMemory,0,0,0,0) // Usage counter register
        },
        "Standby" // Name
      },

      // Retention state.
      Package () {
        1000, // Min Residency (us)
        950, // Wakeup Latency (us)
        1, // Flags, set bit0 to 1 to enable this state
        0, // Arch. Context Lost Flags
        0, // Residency Counter Frequency
        0, // Enabled Parent State
        ResourceTemplate () {
          // Entry method for the WFI state. See the ARM FFH Specification (ARM DEN 0048).
          Register (FFixedHW, 0x20, 0, 0x0000000000000000, 3)
        },
        ResourceTemplate () {
          Register (SystemMemory,0,0,0,0) // Residency counter register
        },
        ResourceTemplate () {
          Register (SystemMemory,0,0,0,0) // Usage counter register
        },
        "Powerdown" // Name
      },
    })
}

//
// Description: This is a Processor #2 Device
//
Device (CPU2)
{
    Name (_HID, "ACPI0007")
    Name (_UID, 0x2)
    Method (_STA)
    {
        Return(0xf)
    }
    Name (_LPI, Package () {
      0, // Revision
      0, // LevelID
      2, // Count of packages

      // Standby / WFI state.
      Package () {
        0, // Min Residency (us)
        0, // Wakeup Latency (us)
        1, // Flags, set bit0 to 1 to enable this state
        0, // Arch. Context Lost Flags
        0, // Residency Counter Frequency
        0, // Enabled Parent State
        ResourceTemplate () {
          // Entry method for the WFI state. See the ARM FFH Specification (ARM DEN 0048).
          Register (FFixedHW, 0x20, 0, 0x00000000FFFFFFFF, 3)
        },
        ResourceTemplate () {
          Register (SystemMemory,0,0,0,0) // Residency counter register
        },
        ResourceTemplate () {
          Register (SystemMemory,0,0,0,0) // Usage counter register
        },
        "Standby" // Name
      },

      // Retention state.
      Package () {
        1000, // Min Residency (us)
        950, // Wakeup Latency (us)
        1, // Flags, set bit0 to 1 to enable this state
        0, // Arch. Context Lost Flags
        0, // Residency Counter Frequency
        0, // Enabled Parent State
        ResourceTemplate () {
          // Entry method for the WFI state. See the ARM FFH Specification (ARM DEN 0048).
          Register (FFixedHW, 0x20, 0, 0x0000000000000000, 3)
        },
        ResourceTemplate () {
          Register (SystemMemory,0,0,0,0) // Residency counter register
        },
        ResourceTemplate () {
          Register (SystemMemory,0,0,0,0) // Usage counter register
        },
        "Powerdown" // Name
      },
    })
}

//
// Description: This is a Processor #3 Device
//
Device (CPU3)
{
    Name (_HID, "ACPI0007")
    Name (_UID, 0x3)
    Method (_STA)
    {
        Return(0xf)
    }
    Name (_LPI, Package () {
      0, // Revision
      0, // LevelID
      2, // Count of packages

      // Standby / WFI state.
      Package () {
        0, // Min Residency (us)
        0, // Wakeup Latency (us)
        1, // Flags, set bit0 to 1 to enable this state
        0, // Arch. Context Lost Flags
        0, // Residency Counter Frequency
        0, // Enabled Parent State
        ResourceTemplate () {
          // Entry method for the WFI state. See the ARM FFH Specification (ARM DEN 0048).
          Register (FFixedHW, 0x20, 0, 0x00000000FFFFFFFF, 3)
        },
        ResourceTemplate () {
          Register (SystemMemory,0,0,0,0) // Residency counter register
        },
        ResourceTemplate () {
          Register (SystemMemory,0,0,0,0) // Usage counter register
        },
        "Standby" // Name
      },

      // Retention state.
      Package () {
        1000, // Min Residency (us)
        950, // Wakeup Latency (us)
        1, // Flags, set bit0 to 1 to enable this state
        0, // Arch. Context Lost Flags
        0, // Residency Counter Frequency
        0, // Enabled Parent State
        ResourceTemplate () {
          // Entry method for the WFI state. See the ARM FFH Specification (ARM DEN 0048).
          Register (FFixedHW, 0x20, 0, 0x0000000000000000, 3)
        },
        ResourceTemplate () {
          Register (SystemMemory,0,0,0,0) // Residency counter register
        },
        ResourceTemplate () {
          Register (SystemMemory,0,0,0,0) // Usage counter register
        },
        "Powerdown" // Name
      },
    })
}

