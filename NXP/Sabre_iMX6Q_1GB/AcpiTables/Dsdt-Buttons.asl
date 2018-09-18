/*
* Buttons on Sabre Board (Power, Volume Up, Volume Down)
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

Device(BTNS)
{
    Name(_HID, "ACPI0011")
    Name(_CRS, ResourceTemplate() {

        // PWR_BTN_SNS - EIM_D29 - GPIO3_IO29 (ACTIVE_LOW)
        GpioInt(Edge, ActiveBoth, Exclusive, PullUp, 0, "\\_SB.GPIO", ,) { 93 }   // Index 0: Power Button

        // KEY_VOL_UP - KEY_COL7 - GPIO_4 - GPIO1_IO4 (ACTIVE_LOW)
        GpioInt(Edge, ActiveBoth, Exclusive, PullUp, 0, "\\_SB.GPIO", ,) { 4 }   // Index 1: Volume Up Button

        // KEY_VOL_DN - KEY_ROW7 - GPIO_5 - GPIO1_IO5 (ACTIVE_LOW)
        GpioInt(Edge, ActiveBoth, Exclusive, Pullup, 0, "\\_SB.GPIO", ,) { 5 }   // Index 2: Volume Down Button
    })
    Name(_DSD, Package(2) {
        //UUID for HID Button Descriptors:
        ToUUID("FA6BD625-9CE8-470D-A2C7-B3CA36C4282E"),
        //Data structure for this UUID:
        Package() {
            Package(5) {
                0,    // This is a Collection
                1,    // Unique ID for this Collection
                0,    // This is a Top-Level Collection
                0x01, // Usage Page ("Generic Desktop Page")
                0x0D  // Usage ("Portable Device Control")
            },
            Package(5) {
                1,    // This is a Control
                0,    // Interrupt index in _CRS for Power Button
                1,    // Unique ID of Parent Collection
                0x01, // Usage Page ("Generic Desktop Page")
                0x81  // Usage ("System Power Down")
            },
            Package(5) {
                1,    // This is a Control
                1,    // Interrupt index in _CRS for Volume Up Button
                1,    // Unique ID of Parent Collection
                0x0C, // Usage Page ("Consumer Page")
                0xE9  // Usage ("Volume Increment")
            },
            Package(5) {
                1,    // This is a Control
                2,    // Interrupt index in _CRS for Volume Down Button
                1,    // Unique ID of Parent Collection
                0x0C, // Usage Page ("Consumer Page")
                0xEA  // Usage ("Volume Decrement")
            },
        }
    })
}
