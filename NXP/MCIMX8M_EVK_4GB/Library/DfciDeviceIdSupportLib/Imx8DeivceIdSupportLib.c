/** @file
DfciDeviceidSupportLib.c

This library provides access to platform data the becomes the DFCI DeviceId.

Copyright (c) 2018, Microsoft Corporation

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#include <PiDxe.h>
#include <Library/DfciDeviceIdSupportLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

/**
Gets the serial number for this device.

@para[out] SerialNumber - UINTN value of serial number

@return EFI_SUCCESS - SerialNumber has been updated to equal the serial number of the device
@return EFI_ERROR   - Error getting number

**/
EFI_STATUS
EFIAPI
DfciIdSupportV1GetSerialNumber(
  OUT UINTN*  SerialNumber
  ) {
    DEBUG((DEBUG_INFO, "%a - \n", __FUNCTION__));
   *SerialNumber = 1;
   return EFI_SUCCESS;
}

/**
 * Get the Manufacturer Name
 *
 * @param Manufacturer
 * @param ManufacturerSize
 *
 * It is the callers responsibility to free the buffer returned.
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
DfciIdSupportGetManufacturer (
    CHAR8   **Manufacturer,
    UINTN    *ManufacturerSize   OPTIONAL
  ) {
    DEBUG((DEBUG_INFO, "%a - \n", __FUNCTION__));
    CHAR8* name = "NXP";
    UINTN size = AsciiStrnSizeS(name, 50);
    *Manufacturer = AllocateCopyPool(size, name);
    DEBUG((DEBUG_INFO, "%a - \n", name));

    if (ManufacturerSize) {
      *ManufacturerSize = size;
    }
    return EFI_SUCCESS;
}

/**
 * Get the ProductName
 *
 * @param ProductName
 * @param ProductNameSize
 *
 * It is the callers responsibility to free the buffer returned.
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
DfciIdSupportGetProductName (
    CHAR8   **ProductName,
    UINTN    *ProductNameSize  OPTIONAL
  ) {
    CHAR8* name = "IMX8";
    UINTN size = AsciiStrnSizeS(name, 50);
    *ProductName = AllocateCopyPool(size, name);
    DEBUG((DEBUG_INFO, "%a - %a\n", __FUNCTION__, *ProductName));
    if (ProductNameSize) {
      *ProductNameSize = size;
    }
    return EFI_SUCCESS;
}

/**
 * Get the SerialNumber
 *
 * @param SerialNumber
 * @param SerialNumberSize
 *
 * It is the callers responsibility to free the buffer returned.
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
DfciIdSupportGetSerialNumber (
    CHAR8   **SerialNumber,
    UINTN    *SerialNumberSize  OPTIONAL
  ) {
    CHAR8* serialNumber = "1";
    UINTN size = AsciiStrnSizeS(serialNumber, 50);
    *SerialNumber = AllocateCopyPool(size, serialNumber);
    DEBUG((DEBUG_INFO, "%a - %a\n", __FUNCTION__, *SerialNumber));
    if (SerialNumberSize) {
      *SerialNumberSize = size;
    }
    return EFI_SUCCESS;
}

/**
 * Get the Uuid
 *
 * @param Uuid
 * @param UuidSize
 *
 * It is the callers responsibility to free the buffer returned.
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
DfciIdSupportGetUuid (
    CHAR8   **Uuid,
    UINTN    *UuidSize  OPTIONAL
  ) {
    CHAR8* uuid = "{0BC8D9D3-D9F6-4C25-92CB-2C63C47977A7}"; // fake
    UINTN size = AsciiStrnSizeS(uuid, 50);
    *Uuid = AllocateCopyPool(size, uuid);
    DEBUG((DEBUG_INFO, "%a - %a\n", __FUNCTION__, *Uuid));
    if (UuidSize) {
      *UuidSize = size;
    }
    return EFI_SUCCESS;
}
