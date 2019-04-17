/** @file
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
**/

#include <Uefi.h>
#include <Library/BaseCryptLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/ShellParameters.h>
#include <Protocol/SimpleFileSystem.h>
#include <EdkTest.h>

MODULE ("Storage FAT media functional tests");

EFI_HANDLE mImageHandle;
EFI_DEVICE_PATH_PROTOCOL *DevicePath;
CHAR16 *TempDevicePathText;
EFI_HANDLE MediaHandle;
EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Fs;
EFI_FILE_HANDLE RootVolume;
UINTN  Argc;
CHAR16 **Argv;

EFI_FILE_PROTOCOL *File;
UINT8 *WriteBuffer;
UINT8 *ReadBuffer;

UINT32
Rand (
  VOID
  )
{
  UINT32 val;

  RandomBytes((UINT8 *) &val, sizeof(val));
  val = val % 32768;
  return val;
}

VOID
VerifyAreEqualBytes (
  IN CONST UINT8   *LeftBuffer,
  IN CONST UINT8   *RightBuffer,
  IN UINTN          BufferSize
  )
{
  UINTN Index;

  ASSERT (LeftBuffer != NULL);
  ASSERT (RightBuffer != NULL);

  for (Index = 0; Index < BufferSize; ++Index, LeftBuffer++, RightBuffer++) {
    if (*LeftBuffer != *RightBuffer) {
      VERIFY_IS_TRUE (
        FALSE,
        "Buffers don't match at byte 0x%p. "
        "(Left Byte = %02x, Right Byte = %02x)",
        Index,
        *LeftBuffer,
        *RightBuffer);
    }
  }
}

VOID
TestRandomWriteRead (
  VOID
  )
{
  static CONST UINTN BufferSizes[] = {
    1, 2, 3, 4, 8, 16, 32, 64, 128, 512, SIZE_1KB, SIZE_4KB, SIZE_8KB,
    SIZE_64KB, SIZE_1MB
  };

  UINTN i;
  UINTN j;
  UINTN WriteBufferSize;
  UINTN ReadBufferSize;
  UINT64 FilePos;
  UINT64 ActualFilePos;

  VERIFY_SUCCEEDED (RootVolume->Open (
                                  RootVolume,
                                  &File,
                                  L"StorageTest.bin",
                                  EFI_FILE_MODE_READ |
                                    EFI_FILE_MODE_WRITE |
                                    EFI_FILE_MODE_CREATE,
                                  0));

  for (i = 0; i < ARRAY_SIZE (BufferSizes); ++i) {

    LOG_COMMENT ("Test Write/Read Size: %u\n", BufferSizes[i]);

    SET_LOG_LEVEL (TestLogError);

    WriteBuffer = AllocateZeroPool (BufferSizes[i]);
    VERIFY_IS_NOT_NULL (WriteBuffer);

    ReadBuffer = AllocateZeroPool (BufferSizes[i]);
    VERIFY_IS_NOT_NULL (ReadBuffer);

    for (j = 0; j < 100 / (i + 1); ++j) {
      SET_LOG_LEVEL (TestLogError);
      FilePos = (Rand() % SIZE_1KB);
      VERIFY_SUCCEEDED (File->SetPosition (File, FilePos));
      VERIFY_SUCCEEDED (File->GetPosition (File, &ActualFilePos));
      VERIFY_ARE_EQUAL (UINT64, FilePos, ActualFilePos);

      RandomBytes (WriteBuffer, BufferSizes[i]);

      WriteBufferSize = BufferSizes[i];
      VERIFY_SUCCEEDED (File->Write (File, &WriteBufferSize, WriteBuffer));
      VERIFY_ARE_EQUAL (UINTN, BufferSizes[i], WriteBufferSize);
      VERIFY_SUCCEEDED (File->GetPosition (File, &ActualFilePos));
      VERIFY_ARE_EQUAL (UINT64, FilePos + BufferSizes[i], ActualFilePos);
      VERIFY_SUCCEEDED (File->Flush(File));

      VERIFY_SUCCEEDED (File->SetPosition (File, FilePos));
      ReadBufferSize = BufferSizes[i];
      VERIFY_SUCCEEDED (File->Read (File, &ReadBufferSize, ReadBuffer));
      VERIFY_ARE_EQUAL (UINTN, BufferSizes[i], ReadBufferSize);
      VerifyAreEqualBytes (WriteBuffer, ReadBuffer, BufferSizes[i]);

      SET_LOG_LEVEL (TestLogComment);
      LOG_COMMENT (".");
    }

    LOG_COMMENT ("\n");

    FreePool (WriteBuffer);
    WriteBuffer = NULL;

    FreePool (ReadBuffer);
    ReadBuffer = NULL;
  }

  LOG_COMMENT ("\n");
}

BOOLEAN
TestSetup (
  VOID
  )
{
  File = NULL;
  WriteBuffer = NULL;
  ReadBuffer = NULL;
  RandomSeed (NULL, 0);
  return TRUE;
}

VOID
TestCleanup (
  VOID
)
{
  if (File) {
    File->Delete (File);
    File = NULL;
  }

  if (WriteBuffer) {
    FreePool(WriteBuffer);
    WriteBuffer = NULL;
  }

  if (ReadBuffer) {
    FreePool(ReadBuffer);
    ReadBuffer = NULL;
  }
}

EFI_STATUS
GetArg (
  VOID
  )
{
  EFI_STATUS Status;
  EFI_SHELL_PARAMETERS_PROTOCOL *ShellParameters;

  Status = gBS->HandleProtocol (
                  gImageHandle,
                  &gEfiShellParametersProtocolGuid,
                  (VOID**)&ShellParameters
                  );
  if (EFI_ERROR(Status)) {
    return Status;
  }

  Argc = ShellParameters->Argc;
  Argv = ShellParameters->Argv;
  return EFI_SUCCESS;
}

BOOLEAN
TestModuleSetup (
  VOID
  )
{
  CONST CHAR16 *DevicePathText;
  EFI_STATUS Status;
  EFI_DEVICE_PATH *TempDevicePath;

  if ((GetArg() != EFI_SUCCESS) || (Argc < 2)) {
    LOG_ERROR ("Expected FS device path command line parameter");
    return FALSE;
  }

  DevicePathText = Argv[1];

  if ((!DevicePathText) || (*DevicePathText == L'\0')) {
    LOG_ERROR ("Storage media device path is invalid");
    return FALSE;
  }

  LOG_COMMENT("Storage media device path '%s'\n", DevicePathText);
  DevicePath = ConvertTextToDevicePath (DevicePathText);
  if (!DevicePath) {
    LOG_ERROR ("ConvertTextToDevicePath(..) failed");
    return FALSE;
  }

  TempDevicePath = DevicePath;
  LOG_COMMENT ("Looking for FAT filesystem\n");
  Status = gBS->LocateDevicePath (
                  &gEfiSimpleFileSystemProtocolGuid,
                  &TempDevicePath,
                  &MediaHandle);

  if (Status == EFI_NOT_FOUND) {
    LOG_ERROR ("%s FAT partition is not ready", DevicePathText);
    return FALSE;
  }

  if (EFI_ERROR (Status)) {
    LOG_ERROR ("gBS->LocateDevicePath(..) failed. (Status=%r)", Status);
    return FALSE;
  }

  TempDevicePathText = ConvertDevicePathToText (TempDevicePath, FALSE, FALSE);
  if (!TempDevicePathText) {
    LOG_ERROR ("ConvertDevicePathToText(..) failed");
    return FALSE;
  }

  LOG_COMMENT ("Opening FAT filesystem '%s'\n", TempDevicePathText);
  Status = gBS->OpenProtocol(
                  MediaHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **)&Fs,
                  mImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL);

  if (EFI_ERROR(Status)) {
    LOG_ERROR (
      "gBS->OpenProtocol(gEfiSimpleFileSystemProtocolGuid) failed. (Status=%r)",
       Status);
    return FALSE;
  }

  Status = Fs->OpenVolume(Fs, &RootVolume);
  if (EFI_ERROR(Status)) {
    LOG_ERROR ("Fs->OpenVolume(..) failed. (Status=%r)", Status);
    return FALSE;
  }

  return TRUE;
}

VOID
TestModuleCleanup (
  VOID
  )
{
  EFI_STATUS Status;

  if (DevicePath) {
    FreePool (DevicePath);
    DevicePath = NULL;
  }

  if (TempDevicePathText) {
    FreePool (TempDevicePathText);
    TempDevicePathText = NULL;
  }

  if (RootVolume) {
    LOG_COMMENT ("Closing volume\n");
    RootVolume->Close (RootVolume);
    RootVolume = NULL;
  }

  if (Fs) {
    LOG_COMMENT ("Closing file-system\n");
    Status = gBS->CloseProtocol (
                    MediaHandle,
                    &gEfiSimpleFileSystemProtocolGuid,
                    mImageHandle,
                    NULL);

    if (EFI_ERROR (Status)) {
      LOG_ERROR ("gBS->CloseProtocol(..) failed. (Status=%r)", Status);
    }
    Fs = NULL;
  }
}

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  mImageHandle = ImageHandle;

  MODULE_SETUP (TestModuleSetup);
  MODULE_CLEANUP (TestModuleCleanup);
  TEST_SETUP (TestSetup);
  TEST_CLEANUP (TestCleanup);
  TEST_FUNC (TestRandomWriteRead);

  if (!RUN_MODULE(0, NULL)) {
    return 1;
  }

  return 0;
}
