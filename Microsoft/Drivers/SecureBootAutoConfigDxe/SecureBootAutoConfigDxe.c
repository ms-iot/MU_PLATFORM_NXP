/* @file

  The Secure Boot Automatic Configuration Driver checks to see if secure
  boot is provisioned and if it's not properly configures secure boot
  based on the pre-build binary blobs included in the UEFI image.

  Caution: This module requires additional review when modified.
  This driver will have external input - variable data.
  This external input must be validated carefully to avoid security issues such as
  buffer overflow or integer overflow.

 Copyright (c) 2015, Microsoft Corporation. All rights reserved.

 This program and the accompanying materials are licensed and made available under the
 terms and conditions of the BSD License which accompanies this distribution.
 The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include <PiDxe.h>
#include <Protocol/VariableWrite.h>
#include <Protocol/Variable.h>
#include <Protocol/VariableLock.h>
#include <Protocol/VarCheck.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Guid/GlobalVariable.h>
#include <Guid/EventGroup.h>
#include <Guid/AuthenticatedVariableFormat.h>
#include <Guid/ImageAuthentication.h>
#include <Guid/SecureBootAutoConfig.h>

// Logging Macros

#define LOG_TRACE_FMT_HELPER(FUNC, FMT, ...)  "SB-AC[T]:%a:" FMT "%a\n", FUNC, __VA_ARGS__

#define LOG_INFO_FMT_HELPER(FMT, ...)   "SB-AC[I]:" FMT "%a\n", __VA_ARGS__

#define LOG_ERROR_FMT_HELPER(FMT, ...) \
  "SB-AC[E]:" FMT " (%a: %a, %d)\n", __VA_ARGS__

#define LOG_INFO(...) \
  DEBUG ((DEBUG_INIT, LOG_INFO_FMT_HELPER (__VA_ARGS__, "")))

#define LOG_VANILLA_TRACE(...) \
  DEBUG ((DEBUG_VERBOSE | DEBUG_VARIABLE, __VA_ARGS__))

#define LOG_TRACE(...) \
  DEBUG ((DEBUG_VERBOSE | DEBUG_VARIABLE, LOG_TRACE_FMT_HELPER (__FUNCTION__, __VA_ARGS__, "")))

#define LOG_ERROR(...) \
  DEBUG ((DEBUG_ERROR, LOG_ERROR_FMT_HELPER (__VA_ARGS__, __FUNCTION__, __FILE__, __LINE__)))

GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8 *mSetupModeNames[] = {
  "UserMode",
  "SetupMode"
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8 *mCustomModeNames[] = {
  "Standard",
  "Custom"
};

VOID
EFIAPI
SecureBootAutoConfigOnReadyToBoot (
  IN EFI_EVENT  Event,
  IN OUT VOID   *Context
  )
{
  EFI_STATUS Status;
  UINTN DataSize;
  UINT8 Data;
  VOID *ImageData = NULL;
  UINTN ImageSize = 0;

  // If we do not have a PK then we need to perform secure boot configuration
  DataSize = sizeof (UINT8);
  Status = gRT->GetVariable (
                  EFI_PLATFORM_KEY_NAME,
                  &gEfiGlobalVariableGuid,
                  NULL,
                  &DataSize,
                  &Data);

  if (Status == EFI_BUFFER_TOO_SMALL) {
    LOG_INFO ("Skipping  provisioning. SecureBoot is already provisioned");
    // PK is present, but our provided buffer is too small which means that
    // SecureBoot is provisioned.
    Status = EFI_SUCCESS;
    goto Exit;
  }

  // The revoked signature database (dbx) is optional

  Status = GetSectionFromAnyFv (
              &gEfiSecureBootDbxImageGuid,
              EFI_SECTION_RAW,
              0,
              &ImageData,
              &ImageSize);

  if (!EFI_ERROR (Status)) {
    Status = gRT->SetVariable (
                    EFI_IMAGE_SECURITY_DATABASE1,
                    &gEfiImageSecurityDatabaseGuid,
                    (EFI_VARIABLE_NON_VOLATILE |
                     EFI_VARIABLE_RUNTIME_ACCESS |
                     EFI_VARIABLE_BOOTSERVICE_ACCESS |
                     EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS),
                    ImageSize,
                    ImageData);

    if (EFI_ERROR (Status)) {
      LOG_ERROR (
        "gRT->SetVariable(dbx) failed. (Status=%r)",
        Status);
      goto Exit;
    }

    FreePool (ImageData);
    ImageData = NULL;
    ImageSize = 0;
  }else {
    LOG_TRACE ("dbx blob not present in the FV");
  }

  // The signature database (db)

  Status = GetSectionFromAnyFv (
              &gEfiSecureBootDbImageGuid,
              EFI_SECTION_RAW,
              0,
              &ImageData,
              &ImageSize);

  if (EFI_ERROR (Status)) {
    LOG_ERROR ("GetSectionFromAnyFv(db) failed.(Status=%r)", Status);
    goto Exit;
  }

  Status = gRT->SetVariable (
                  EFI_IMAGE_SECURITY_DATABASE,
                  &gEfiImageSecurityDatabaseGuid,
                  (EFI_VARIABLE_NON_VOLATILE |
                   EFI_VARIABLE_RUNTIME_ACCESS |
                   EFI_VARIABLE_BOOTSERVICE_ACCESS |
                   EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS),
                  ImageSize,
                  ImageData);

  if (EFI_ERROR (Status)) {
    LOG_ERROR ("gRT->SetVariable(db) failed. (Status=%r)", Status);
    goto Exit;
  }

  FreePool (ImageData);
  ImageData = NULL;
  ImageSize = 0;

  // Key Enrollment Key database (KEK)

  Status = GetSectionFromAnyFv (
              &gEfiSecureBootKekImageGuid,
              EFI_SECTION_RAW,
              0,
              &ImageData,
              &ImageSize);

  if (EFI_ERROR (Status)) {
    LOG_ERROR ("GetSectionFromAnyFv(KEK) failed.(Status=%r)", Status);
    goto Exit;
  }

  Status = gRT->SetVariable (
                  EFI_KEY_EXCHANGE_KEY_NAME,
                  &gEfiGlobalVariableGuid,
                  (EFI_VARIABLE_NON_VOLATILE |
                   EFI_VARIABLE_RUNTIME_ACCESS |
                   EFI_VARIABLE_BOOTSERVICE_ACCESS |
                   EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS),
                  ImageSize,
                  ImageData);

  if (EFI_ERROR (Status)) {
    LOG_ERROR ("gRT->SetVariable(KEK) failed. (Status=%r)", Status);
    goto Exit;
  }

  FreePool (ImageData);
  ImageData = NULL;
  ImageSize = 0;

  // Platform key (PK)

  Status = GetSectionFromAnyFv (
              &gEfiSecureBootPkImageGuid,
              EFI_SECTION_RAW,
              0,
              &ImageData, &ImageSize);

  if (EFI_ERROR (Status)) {
    LOG_ERROR ("GetSectionFromAnyFv(PK) failed.(Status=%r)", Status);
    goto Exit;
  }

  Status = gRT->SetVariable (
                  EFI_PLATFORM_KEY_NAME,
                  &gEfiGlobalVariableGuid,
                  (EFI_VARIABLE_NON_VOLATILE |
                   EFI_VARIABLE_RUNTIME_ACCESS |
                   EFI_VARIABLE_BOOTSERVICE_ACCESS |
                   EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS),
                  ImageSize,
                  ImageData);

  if (EFI_ERROR (Status)) {
    LOG_ERROR ("gRT->SetVariable(PK) failed. (Status=%r)", Status);
    goto Exit;
  }

  FreePool (ImageData);
  ImageData = NULL;
  ImageSize = 0;

  LOG_INFO (
    "SecureBoot PK, KEK and db have been provisoned successfully. "
    "Resetting system for provisioning to complete");

  gRT->ResetSystem(EfiResetWarm, EFI_SUCCESS, 0, NULL);

Exit:

  if (EFI_ERROR (Status)) {
    LOG_ERROR ("SecureBoot provisioning failed");
  }

  if (ImageData != NULL) {
    FreePool (ImageData);
  }
}


/**
  The Secure Boot Automatic Configuration Driver checks to see if secure
  boot is provisioned and if it's not properly configures secure boot
  based on the pre-build binary blobs included in the UEFI image.

  The configuration is performed shortly before UEFI launches the boot
  application so that it's after variable services are running.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       Variable service successfully initialized.

**/
EFI_STATUS
EFIAPI
SecureBootAutoConfigDxeInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS   Status;
  EFI_EVENT    OnReadyToBootEvent;

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  SecureBootAutoConfigOnReadyToBoot,
                  NULL,
                  &gEfiEventReadyToBootGuid,
                  &OnReadyToBootEvent);

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "CreateEventEx(gEfiEventReadyToBootGuid) failed. (Status=%r)\n",
      Status));

    return Status;
  }

  return EFI_SUCCESS;
}


