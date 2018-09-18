/*
* Description: iMX6 Sabre Resource Hub Proxy
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

Device(RHPX)
{
    Name(_HID, "MSFT8000")
    Name(_CID, "MSFT8000")
    Name(_UID, 1)

    Name(_CRS, ResourceTemplate()
    {
        // Index 0
        I2CSerialBus(0xFFFF,, 0,, "\\_SB.I2C1",,,,)
		
        // Index 1
        SPISerialBus(              // SCKL - 
                                   // MOSI - 
                                   // MISO - 
                                   // CE0  - 
            0,                     // Device selection (CE0)
            PolarityLow,           // Device selection polarity
            FourWireMode,          // wiremode
            8,                     // databit len
            ControllerInitiated,   // slave mode
            4000000,               // connection speed
            ClockPolarityLow,      // clock polarity
            ClockPhaseFirst,       // clock phase
            "\\_SB.SPI1",          // ResourceSource: SPI bus controller name
            0,                     // ResourceSourceIndex
                                   // Resource usage
                                   // DescriptorName: creates name for offset of resource descriptor
            )                      // Vendor Data

        // Index 2
        SPISerialBus(              // SCKL - 
                                   // MOSI - 
                                   // MISO - 
                                   // CE0  - 
            0,                     // Device selection (CE0)
            PolarityLow,           // Device selection polarity
            FourWireMode,          // wiremode
            8,                     // databit len
            ControllerInitiated,   // slave mode
            4000000,               // connection speed
            ClockPolarityLow,      // clock polarity
            ClockPhaseFirst,       // clock phase
            "\\_SB.SPI2",          // ResourceSource: SPI bus controller name
            0,                     // ResourceSourceIndex
                                   // Resource usage
                                   // DescriptorName: creates name for offset of resource descriptor
            )                      // Vendor Data
			
        // Index 3
        SPISerialBus(              // SCKL - 
                                   // MOSI - 
                                   // MISO - 
                                   // CE0  - 
            0,                     // Device selection (CE0)
            PolarityLow,           // Device selection polarity
            FourWireMode,          // wiremode
            8,                     // databit len
            ControllerInitiated,   // slave mode
            4000000,               // connection speed
            ClockPolarityLow,      // clock polarity
            ClockPhaseFirst,       // clock phase
            "\\_SB.SPI3",          // ResourceSource: SPI bus controller name
            0,                     // ResourceSourceIndex
                                   // Resource usage
                                   // DescriptorName: creates name for offset of resource descriptor
            )                      // Vendor Data

        // Index 4
        SPISerialBus(              // SCKL - 
                                   // MOSI - 
                                   // MISO - 
                                   // CE0  - 
            0,                     // Device selection (CE0)
            PolarityLow,           // Device selection polarity
            FourWireMode,          // wiremode
            8,                     // databit len
            ControllerInitiated,   // slave mode
            4000000,               // connection speed
            ClockPolarityLow,      // clock polarity
            ClockPhaseFirst,       // clock phase
            "\\_SB.SPI4",          // ResourceSource: SPI bus controller name
            0,                     // ResourceSourceIndex
                                   // Resource usage
                                   // DescriptorName: creates name for offset of resource descriptor
            )                      // Vendor Data
			
        // Index 5
        SPISerialBus(              // SCKL - 
                                   // MOSI - 
                                   // MISO - 
                                   // CE0  - 
            0,                     // Device selection (CE0)
            PolarityLow,           // Device selection polarity
            FourWireMode,          // wiremode
            8,                     // databit len
            ControllerInitiated,   // slave mode
            4000000,               // connection speed
            ClockPolarityLow,      // clock polarity
            ClockPhaseFirst,       // clock phase
            "\\_SB.SPI5",          // ResourceSource: SPI bus controller name
            0,                     // ResourceSourceIndex
                                   // Resource usage
                                   // DescriptorName: creates name for offset of resource descriptor
            )                      // Vendor Data
            
       // GPIO1_IO00 PAD_GPIO00
       GpioIO(Shared, PullUp, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 0 }
       GpioInt(Edge, ActiveBoth, Shared, PullUp, 0, "\\_SB.GPIO",) { 0 }

       // GPIO1_IO02 PAD_GPIO02
       GpioIO(Shared, PullUp, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 2 }
       GpioInt(Edge, ActiveBoth, Shared, PullUp, 0, "\\_SB.GPIO",) { 2 }

       // GPIO1_IO03 PAD_GPIO03
       GpioIO(Shared, PullUp, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 3 }
       GpioInt(Edge, ActiveBoth, Shared, PullUp, 0, "\\_SB.GPIO",) { 3 }

       // GPIO1_IO04 PAD_GPIO04
       GpioIO(Shared, PullUp, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 4 }
       GpioInt(Edge, ActiveBoth, Shared, PullUp, 0, "\\_SB.GPIO",) { 4 }

       // GPIO1_IO05 PAD_GPIO05
       GpioIO(Shared, PullUp, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 5 }
       GpioInt(Edge, ActiveBoth, Shared, PullUp, 0, "\\_SB.GPIO",) { 5 }

       // GPIO1_IO06 PAD_GPIO06
       GpioIO(Shared, PullUp, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 6 }
       GpioInt(Edge, ActiveBoth, Shared, PullUp, 0, "\\_SB.GPIO",) { 6 }

       // GPIO1_IO07 PAD_GPIO07
       GpioIO(Shared, PullUp, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 7 }
       GpioInt(Edge, ActiveBoth, Shared, PullUp, 0, "\\_SB.GPIO",) { 7 }

       // GPIO1_IO08 PAD_GPIO08
       GpioIO(Shared, PullUp, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 8 }
       GpioInt(Edge, ActiveBoth, Shared, PullUp, 0, "\\_SB.GPIO",) { 8 }

       // GPIO1_IO09 PAD_GPIO09
       GpioIO(Shared, PullUp, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 9 }
       GpioInt(Edge, ActiveBoth, Shared, PullUp, 0, "\\_SB.GPIO",) { 9 }

       // GPIO2_IO02 PAD_NAND_DATA02
       GpioIO(Shared, PullUp, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 34 } // 1 * 32 + 2
       GpioInt(Edge, ActiveBoth, Shared, PullUp, 0, "\\_SB.GPIO",) { 34 }

       // GPIO2_IO03 PAD_NAND_DATA03
       GpioIO(Shared, PullUp, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 35 } // 1 * 32 + 3
       GpioInt(Edge, ActiveBoth, Shared, PullUp, 0, "\\_SB.GPIO",) { 35 }

       // GPIO2_IO31 PAD_EIM_EB3_B
       GpioIO(Shared, PullUp, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 63 } // 1 * 32 + 31
       GpioInt(Edge, ActiveBoth, Shared, PullUp, 0, "\\_SB.GPIO",) { 63 }

       // GPIO4_IO05 PAD_GPIO19
       GpioIO(Shared, PullUp, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 101 } // 3 * 32 + 5
       GpioInt(Edge, ActiveBoth, Shared, PullUp, 0, "\\_SB.GPIO",) { 101 }

       // GPIO4_IO10 PAD_KEY_COL2
       GpioIO(Shared, PullUp, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 106 } // 3 * 32 + 10
       GpioInt(Edge, ActiveBoth, Shared, PullUp, 0, "\\_SB.GPIO",) { 101 }

       // GPIO5_IO25 PAD_CSI0_DATA07
       GpioIO(Shared, PullUp, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 153 } // 4 * 32 + 25
       GpioInt(Edge, ActiveBoth, Shared, PullUp, 0, "\\_SB.GPIO",) { 153 }

       // GPIO5_IO26 PAD_CSI0_DATA08
       GpioIO(Shared, PullUp, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 154 } // 4 * 32 + 26
       GpioInt(Edge, ActiveBoth, Shared, PullUp, 0, "\\_SB.GPIO",) { 154 }

       // GPIO5_IO27 PAD_CSI0_DATA09
       GpioIO(Shared, PullUp, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 155 } // 4 * 32 + 27
       GpioInt(Edge, ActiveBoth, Shared, PullUp, 0, "\\_SB.GPIO",) { 155 }

       // GPIO7_IO08 PAD_SD3_RESET
       GpioIO(Shared, PullUp, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 200 } // 6 * 32 + 8
       GpioInt(Edge, ActiveBoth, Shared, PullUp, 0, "\\_SB.GPIO",) { 200 }

       // GPIO7_IO11 PAD_GPIO16
       GpioIO(Shared, PullUp, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 203 } // 6 * 32 + 11
       GpioInt(Edge, ActiveBoth, Shared, PullUp, 0, "\\_SB.GPIO",) { 203 }

       // GPIO7_IO12 PAD_GPIO17
       GpioIO(Shared, PullUp, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 204 } // 6 * 32 + 12
       GpioInt(Edge, ActiveBoth, Shared, PullUp, 0, "\\_SB.GPIO",) { 204 }

       // GPIO7_IO13 PAD_GPIO18
       GpioIO(Shared, PullUp, 0, 0, IoRestrictionNone, "\\_SB.GPIO", 0, ResourceConsumer, , ) { 205 } // 6 * 32 + 13
       GpioInt(Edge, ActiveBoth, Shared, PullUp, 0, "\\_SB.GPIO",) { 205 }            
			
    })

    Name(_DSD, Package()
    {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package()
        {
            // I2C1
            Package(2) { "bus-I2C-I2C1", Package() { 0 }},
		
            // SPI 1
            Package(2) { "bus-SPI-SPI1", Package() { 1 }},                        // Index 2
            Package(2) { "SPI1-MinClockInHz", 115 },                              // 115 Hz
            Package(2) { "SPI1-MaxClockInHz", 8000000 },                          // 8 MHz
            Package(2) { "SPI1-SupportedDataBitLengths", Package() { 8,16,32 }},  // Data bit length
            // SPI 2
            Package(2) { "bus-SPI-SPI2", Package() { 2 }},                        // Index 3
            Package(2) { "SPI2-MinClockInHz", 115 },                              // 115 Hz
            Package(2) { "SPI2-MaxClockInHz", 8000000 },                          // 8 MHz
            Package(2) { "SPI2-SupportedDataBitLengths", Package() { 8,16,32 }},  // Data bit length
            // SPI 3
            Package(2) { "bus-SPI-SPI3", Package() { 3 }},                        // Index 4
            Package(2) { "SPI3-MinClockInHz", 115 },                              // 115 Hz
            Package(2) { "SPI3-MaxClockInHz", 8000000 },                          // 8 MHz
            Package(2) { "SPI3-SupportedDataBitLengths", Package() { 8,16,32 }},  // Data bit length
            // SPI 4
            Package(2) { "bus-SPI-SPI4", Package() { 4 }},                        // Index 5
            Package(2) { "SPI4-MinClockInHz", 115 },                              // 115 Hz
            Package(2) { "SPI4-MaxClockInHz", 8000000 },                          // 8 MHz
            Package(2) { "SPI4-SupportedDataBitLengths", Package() { 8,16,32 }},  // Data bit length
            // SPI 5
            Package(2) { "bus-SPI-SPI5", Package() { 5 }},                        // Index 6
            Package(2) { "SPI5-MinClockInHz", 115 },                              // 115 Hz
            Package(2) { "SPI5-MaxClockInHz", 8000000 },                          // 8 MHz
            Package(2) { "SPI5-SupportedDataBitLengths", Package() { 8,16,32 }},  // Data bit length
            
            // GPIO Pin Count and supported drive modes
            Package (2) { "GPIO-PinCount", 206 },
            Package (2) { "GPIO-UseDescriptorPinNumbers", 1 },

            // InputHighImpedance, InputPullUp, InputPullDown, OutputCmos
            Package (2) { "GPIO-SupportedDriveModes", 0xf },
        }
    })
}
