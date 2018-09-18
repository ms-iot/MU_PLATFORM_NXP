/*
* iMX8 MQ Differentiated System Description Table Fields (DSDT)
*
*  Copyright (c) 2018, Microsoft Corporation. All rights reserved.
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
#include "iMX8Platform.h"
#include <IndustryStandard/Acpi.h>
#include <Library/AcpiLib.h>
#include <Library/PcdLib.h>

DefinitionBlock("DsdtTable.aml", "DSDT", 5, "MSFT", "EDK2", 1) {
  Scope(_SB) {
    include("Dsdt-Platform.asl")
    include("Dsdt-Sdhc.asl")
    include("Dsdt-Gpio.asl")
    include("Dsdt-Usb.asl")
    include("Dsdt-Spi.asl")
    include("Dsdt-I2c.asl")
    include("Dsdt-Uart.asl")
    include("Dsdt-Rhp.asl")
    include("Dsdt-Enet.asl")
    include("Dsdt-Audio.asl")
  }
}

