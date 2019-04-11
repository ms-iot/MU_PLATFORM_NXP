/*++

    Copyright (C) 2018 Microsoft Corporation. All Rights Reserved.

Module Name:

    MsPlatformDevicesLib.c

Abstract:

    This module will provide the device path to the SDCard, the list of
    devices that must always be connected, and the list of devices that
    need to be connected for ConIn OnDemand.

--*/

#include <Protocol/DevicePath.h>
#include <Library/DeviceBootManagerLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MsPlatformDevicesLib.h>
#include <Protocol/GraphicsOutput.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

typedef struct {
  ACPI_HID_DEVICE_PATH      PciRootBridge;
  PCI_DEVICE_PATH           PciDevice;
  USB_CLASS_DEVICE_PATH     Usb;
  EFI_DEVICE_PATH_PROTOCOL  End;
} GENERIC_USB_CLASS_DEVICE_PATH;


#define gEndEntire \
  { \
    END_DEVICE_PATH_TYPE, \
    END_ENTIRE_DEVICE_PATH_SUBTYPE, \
    { \
      END_DEVICE_PATH_LENGTH, \
      0 \
    } \
  }

//89464DAE-8DAA-41FE-A4C8-40 D2 17 5A F1 E9
#define GRAPHICS_DEVICE_PATH_GUID  {0x89464DAE, 0x8DAA, 0x41FE, {0xa4, 0xc8, 0x40, 0xd2, 0x17, 0x5a, 0xf1, 0xe9}}

typedef struct {
    VENDOR_DEVICE_PATH             VendorDevicePath;
    EFI_DEVICE_PATH_PROTOCOL       End;
} GRAPHICS_DEVICE_PATH;

static GRAPHICS_DEVICE_PATH DisplayDevicePath = {
  {
    HARDWARE_DEVICE_PATH,
    HW_VENDOR_DP,
      {
        (UINT8)(sizeof(VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof(VENDOR_DEVICE_PATH)) >> 8)
      },
    GRAPHICS_DEVICE_PATH_GUID
  },
  gEndEntire
};


#define gPciRootBridge \
  { \
    { \
      ACPI_DEVICE_PATH, \
      ACPI_DP, \
      { \
        (UINT8) (sizeof (ACPI_HID_DEVICE_PATH)), \
        (UINT8) ((sizeof (ACPI_HID_DEVICE_PATH)) >> 8), \
      }, \
    }, \
    EISA_PNP_ID (0x0A03), \
    0 \
  }

#define gPciXhciDevice \
  { \
    { \
      HARDWARE_DEVICE_PATH, \
      HW_PCI_DP, \
      { \
        (UINT8) ( sizeof (PCI_DEVICE_PATH)), \
        (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8), \
      }, \
    }, \
    0x00, \
    0x14 \
  }

#define gUsbClassKeyboard \
  { \
    { \
      MESSAGING_DEVICE_PATH, \
      MSG_USB_CLASS_DP, \
      { \
        (UINT8) ( sizeof (USB_CLASS_DEVICE_PATH)), \
        (UINT8) ((sizeof (USB_CLASS_DEVICE_PATH)) >> 8), \
      }, \
    }, \
    0xFFFF,                                  \
    0xFFFF,                                  \
    0x03,  /* HID Class */                   \
    0x01,  /* Boot Interface Subclass  */    \
    0x01   /* Keyboard Protocol */           \
  }

#define gUsbClassMouse \
  { \
    { \
      MESSAGING_DEVICE_PATH, \
      MSG_USB_CLASS_DP, \
      { \
        (UINT8) ( sizeof (USB_CLASS_DEVICE_PATH)), \
        (UINT8) ((sizeof (USB_CLASS_DEVICE_PATH)) >> 8), \
      }, \
    }, \
    0xFFFF,                                  \
    0xFFFF,                                  \
    0x03,  /* HID Class */                   \
    0x01,  /* Boot Interface Subclass  */    \
    0x02   /* Keyboard Protocol */           \
  }

typedef struct {
  VENDOR_DEVICE_PATH        Guid;
  UART_DEVICE_PATH          Uart;
  EFI_DEVICE_PATH_PROTOCOL  End;
} SIMPLE_TEXT_OUT_DEVICE_PATH;

static SIMPLE_TEXT_OUT_DEVICE_PATH gUartDevicePath = {
  {
    { HARDWARE_DEVICE_PATH, HW_VENDOR_DP, {sizeof (VENDOR_DEVICE_PATH), 0}}
  },
  {
    { MESSAGING_DEVICE_PATH, MSG_UART_DP,
      {sizeof (UART_DEVICE_PATH), (UINT8) ((sizeof (UART_DEVICE_PATH)) >> 8)}},
    0,        // Reserved
    FixedPcdGet64 (PcdUartDefaultBaudRate),
    FixedPcdGet8 (PcdUartDefaultDataBits),
    FixedPcdGet8 (PcdUartDefaultParity),
    FixedPcdGet8 (PcdUartDefaultStopBits)
  },
  gEndEntire
};

static GENERIC_USB_CLASS_DEVICE_PATH gUsbKeyboardDevices = {
    gPciRootBridge,
    gPciXhciDevice,
    gUsbClassKeyboard,
    gEndEntire };

static GENERIC_USB_CLASS_DEVICE_PATH gUsbMouseDevices = {
    gPciRootBridge,
    gPciXhciDevice,
    gUsbClassMouse,
    gEndEntire };

static EFI_DEVICE_PATH_PROTOCOL  *gPlatformConInDeviceList[] = {
    (EFI_DEVICE_PATH_PROTOCOL *) &gUartDevicePath,
    (EFI_DEVICE_PATH_PROTOCOL *) &gUsbKeyboardDevices,
    (EFI_DEVICE_PATH_PROTOCOL *) &gUsbMouseDevices,
    NULL
};

//
// Predefined platform default console device path
//
static BDS_CONSOLE_CONNECT_ENTRY   gPlatformConsoles[] = {
  //
  // Place holder for serial console. Any non USB device used for
  // CONIN must be in this table.  Any non display used for CONOUT
  // must also be in this list.
  //
  {
    (EFI_DEVICE_PATH_PROTOCOL *) &gUartDevicePath,
    (CONSOLE_OUT | CONSOLE_IN)
  },
  {
    NULL,
    0
  }
};


/**
 * GetSdCardDevicePath
 *
 * Returns the device path of an internal SD Card slot that is
 * excluded from being bootable.
 *
 * @return EFI_DEVICE_PATH_PROTOCOL
 */
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
GetSdCardDevicePath (VOID) {

    return NULL;  // No SdCard on this platform
}

/**
 * GetPlatformConnectList
 *
 * Returns an array of device paths of devices to connect at every boot.
 *
 * @return NULL                           No devices required to be connected
 * @return EFI_DEVICE_PATH_PROTOCOL       Array of device paths returned
 */
EFI_DEVICE_PATH_PROTOCOL **
EFIAPI
GetPlatformConnectList (VOID) {

    return NULL;    // No devices requiring forced connect
}

/**
 * GetPlatformConsoleList
 *
 * Returns an array of device paths of devices that are unremovable Console Devices.
 *
 * @return NULL                           No unremovable console devices
 * @return EFI_DEVICE_PATH_PROTOCOL       Array of BDS_CONSOLE_CONNECT_ENTRRY's returned
 */
BDS_CONSOLE_CONNECT_ENTRY *
EFIAPI
GetPlatformConsoleList (VOID ) {

     return (BDS_CONSOLE_CONNECT_ENTRY *)&gPlatformConsoles;
}

/**
 * GetPlatformConnectOnConInList
 *
 * Returns an array of device paths of devices to connected when console in is requested.
 *
 * @return NULL                           No devices required to be connected
 * @return EFI_DEVICE_PATH_PROTOCOL       Array of device paths returned
 */
EFI_DEVICE_PATH_PROTOCOL **
EFIAPI
GetPlatformConnectOnConInList (VOID) {

    return (EFI_DEVICE_PATH_PROTOCOL **)&gPlatformConInDeviceList;
}

/**
Library function used to provide the console type.  For ConType == DisplayPath,
device path is filled in to the exact controller to use.  For other ConTypes, DisplayPath
must be NULL. The device path must NOT be freed.
**/
EFI_HANDLE
EFIAPI
GetPlatformPreferredConsole (
    OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath
) {
    EFI_HANDLE                Handle;
    EFI_DEVICE_PATH_PROTOCOL  *TempDevicePath;
    EFI_STATUS                Status;

    //
    // LocateDevicePath modifies DevicePath pointer, so create a copy and pass that back to caller
    //
    *DevicePath = DuplicateDevicePath ((EFI_DEVICE_PATH_PROTOCOL *) &DisplayDevicePath);
    TempDevicePath = *DevicePath;

    Handle = NULL;
    Status = gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &TempDevicePath, &Handle);
    if (EFI_ERROR (Status)) {
      DEBUG((DEBUG_ERROR,"Unable to locate GOP handle using DevicePath, Status %r\n",Status));
      Handle = NULL;
    } else {
      //
      // Connect the GOP driver
      //
      gBS->ConnectController (Handle, NULL, NULL, TRUE);
    }

    return Handle;
}
