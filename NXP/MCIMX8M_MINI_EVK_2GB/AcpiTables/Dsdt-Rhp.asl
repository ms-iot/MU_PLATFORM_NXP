/** @file
* Description: NXP iMX8M Mini EVK Resource Hub Proxy
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
*/

Device(RHPX)
{
  Name(_HID, "MSFT8000")
  Name(_CID, "MSFT8000")
  Name(_UID, 1)

  Name(_CRS, ResourceTemplate()
  {
    // Index 0
    I2CSerialBus(0xFFFF,, 0,, "\\_SB.I2C2",,,,)

    // Index 1
    I2CSerialBus(0xFFFF,, 0,, "\\_SB.I2C3",,,,)

    // Index 2
    SPISerialBus (          // SCLK - GPIO5_IO06 (134) - J1003 pin 10
                            // MOSI - GPIO5_IO07 (135) - J1003 pin 8
                            // MISO - GPIO5_IO08 (136) - J1003 pin 7
                            // SS0  - GPIO5_IO09 (137) - J1003 pin 11
      0,                    // Device selection (CE0)
      PolarityLow,          // Device selection polarity
      FourWireMode,         // wiremode
      0,                    // databit len - placeholder
      ControllerInitiated,  // slave mode
      0,                    // connection speed - placeholder
      ClockPolarityLow,     // clock polarity
      ClockPhaseFirst,      // clock phase
      "\\_SB.SPI1",         // ResourceSource: SPI bus controller name
      0,                    // ResourceSourceIndex
                            // Resource usage
                            // DescriptorName: creates name for offset of resource descriptor
    )                       // Vendor Data

    // Index 3
    SPISerialBus (          // SCLK - GPIO5_IO10 (138) - J1003 pin 23
                            // MOSI - GPIO5_IO11 (139) - J1003 pin 19
                            // MISO - GPIO5_IO12 (140) - J1003 pin 21
                            // SS0  - GPIO5_IO13 (141) - J1003 pin 24
      0,                    // Device selection (CE0)
      PolarityLow,          // Device selection polarity
      FourWireMode,         // wiremode
      0,                    // databit len - placeholder
      ControllerInitiated,  // slave mode
      0,                    // connection speed - placeholder
      ClockPolarityLow,     // clock polarity
      ClockPhaseFirst,      // clock phase
      "\\_SB.SPI2",         // ResourceSource: SPI bus controller name
      0,                    // ResourceSourceIndex
                            // Resource usage
                            // DescriptorName: creates name for offset of resource descriptor
    )                       // Vendor Data

    // GPIO3_IO16 NAND_READY_B - SYS_STATUS LED
    GpioIO(Shared, PullDown, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 80 } // 2 * 32 + 16
    GpioInt(Edge, ActiveBoth, Shared, PullDown, 0, "\\_SB.GPIO",) { 80 }

    // GPIO3_IO20 PAD_SIA5_RXC - J1003 pin 40
    GpioIO(Shared, PullDown, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 84 } // 2 * 32 + 20
    GpioInt(Edge, ActiveBoth, Shared, PullDown, 0, "\\_SB.GPIO",) { 84 }

    // GPIO3_IO21 PAD_SIA5_RXD0 - J1003 pin 38
    GpioIO(Shared, PullDown, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 85 } // 2 * 32 + 21
    GpioInt(Edge, ActiveBoth, Shared, PullDown, 0, "\\_SB.GPIO",) { 85 }

    // GPIO3_IO22 PAD_SIA5_RXD1 - J1003 pin 37
    GpioIO(Shared, PullDown, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 86 } // 2 * 32 + 22
    GpioInt(Edge, ActiveBoth, Shared, PullDown, 0, "\\_SB.GPIO",) { 86 }

    // GPIO3_IO23 PAD_SIA5_RXD2 - J1003 pin 36
    GpioIO(Shared, PullDown, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 87 } // 2 * 32 + 23
    GpioInt(Edge, ActiveBoth, Shared, PullDown, 0, "\\_SB.GPIO",) { 87 }

    // GPIO3_IO24 PAD_SIA5_RXD3 - J1003 pin 35
    GpioIO(Shared, PullDown, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 88 } // 2 * 32 + 24
    GpioInt(Edge, ActiveBoth, Shared, PullDown, 0, "\\_SB.GPIO",) { 88 }

    // GPIO5_IO06 PAD_ECSPI1_SCLK - J1003 pin 10 (UART3_RXD)
    GpioIO(Shared, PullDown, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 134 } // 4 * 32 + 6
    GpioInt(Edge, ActiveBoth, Shared, PullDown, 0, "\\_SB.GPIO",) { 134 }

    // GPIO5_IO07 PAD_ECSPI1_MOSI - J1003 pin 8 (UART3_TXD)
    GpioIO(Shared, PullDown, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 135 } // 4 * 32 + 7
    GpioInt(Edge, ActiveBoth, Shared, PullDown, 0, "\\_SB.GPIO",) { 135 }

    // GPIO5_IO08 PAD_ECSPI1_MISO - J1003 pin 7 (UART3_CTS)
    GpioIO(Shared, PullDown, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 136 } // 4 * 32 + 8
    GpioInt(Edge, ActiveBoth, Shared, PullDown, 0, "\\_SB.GPIO",) { 136 }

    // GPIO5_IO09 PAD_ECSPI1_SS0 - J1003 pin 11 (UART3_RTS)
    GpioIO(Shared, PullDown, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 137 } // 4 * 32 + 9
    GpioInt(Edge, ActiveBoth, Shared, PullDown, 0, "\\_SB.GPIO",) { 137 }

    // GPIO5_IO10 PAD_ECSPI2_SCLK - J1003 pin 23
    GpioIO(Shared, PullDown, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 138 } // 4 * 32 + 10
    GpioInt(Edge, ActiveBoth, Shared, PullDown, 0, "\\_SB.GPIO",) { 138 }

    // GPIO5_IO11 PAD_ECSPI2_MOSI - J1003 pin 19
    GpioIO(Shared, PullDown, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 139 } // 4 * 32 + 11
    GpioInt(Edge, ActiveBoth, Shared, PullDown, 0, "\\_SB.GPIO",) { 139 }

    // GPIO5_IO12 PAD_ECSPI2_MISO - J1003 pin 21
    GpioIO(Shared, PullDown, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 140 } // 4 * 32 + 12
    GpioInt(Edge, ActiveBoth, Shared, PullDown, 0, "\\_SB.GPIO",) { 140 }

    // GPIO5_IO13 PAD_ECSPI2_SS0 - J1003 pin 24
    GpioIO(Shared, PullDown, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 141 } // 4 * 32 + 13
    GpioInt(Edge, ActiveBoth, Shared, PullDown, 0, "\\_SB.GPIO",) { 141 }
  })

  Name(_DSD, Package()
  {
    ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
    Package()
    {
      // I2C buses 2-3
      Package(2) { "bus-I2C-I2C2", Package() { 0 }},
      Package(2) { "bus-I2C-I2C3", Package() { 1 }},

      // SPI buses 1-2
      // Reference clock is 24 MHz
      Package(2) { "bus-SPI-SPI1", Package() { 2 }},
      Package(2) { "SPI1-MinClockInHz", 46 },                              // 46 Hz
      Package(2) { "SPI1-MaxClockInHz", 12000000 },                        // 12 MHz
      Package(2) { "SPI1-SupportedDataBitLengths", Package() { 8,16,32 }}, // Data bit length

      Package(2) { "bus-SPI-SPI2", Package() { 3 }},
      Package(2) { "SPI2-MinClockInHz", 46 },                              // 46 Hz
      Package(2) { "SPI2-MaxClockInHz", 12000000 },                        // 12 MHz
      Package(2) { "SPI2-SupportedDataBitLengths", Package() { 8,16,32 }}, // Data bit length

      // GPIO Pin Count and supported drive modes
      Package (2) { "GPIO-PinCount", 157 },
      Package (2) { "GPIO-UseDescriptorPinNumbers", 1 },

      // InputHighImpedance, InputPullUp, InputPullDown, OutputCmos
      Package (2) { "GPIO-SupportedDriveModes", 0x0F },
    }
  })
}
