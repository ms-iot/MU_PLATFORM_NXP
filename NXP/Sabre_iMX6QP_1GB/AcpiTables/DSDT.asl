/*
* iMX6 Quad Plus Sabre DSDT
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

#include "Dsdt-Common.h"

#define SOC_TYPE 0x00000069  // iMX6 Quad Plus

DefinitionBlock ("DSDT.aml", "DSDT", 5, "MSFT", "EDK2", 1)
{
    Scope (\_SB_)
    {
        include("Dsdt-Platform.asl")
        include("Dsdt-Gpio.asl")
        include("Dsdt-Gfx.asl")
        include("Dsdt-Usb.asl")
        include("Dsdt-PCIe.asl")
        include("Dsdt-Sdhc.asl")
        include("Dsdt-Enet.asl")
        include("Dsdt-Audio.asl")
        include("Dsdt-AudioCodec.asl")
        include("Dsdt-Uart.asl")
        include("Dsdt-I2c.asl")
        include("Dsdt-Spi.asl")
        include("Dsdt-Rhp.asl")
        include("Dsdt-Buttons.asl")
        include("Dsdt-TrEE.asl")
    } // \_SB_
}
