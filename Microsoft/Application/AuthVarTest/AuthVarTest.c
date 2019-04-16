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

#include <PiDxe.h>
#include <Guid/AuthenticatedVariableFormat.h>
#include <Guid/AuthVarTestFfs.h>
#include <Guid/GlobalVariable.h>
#include <Guid/ImageAuthentication.h>
#include <Library/BaseCryptLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/ShellParameters.h>

#include <EdkTest.h>

MODULE ("UEFI authenticated variables functional tests");

EFI_HANDLE mImageHandle;
VOID *mImageData = NULL;
VOID *mActualImageData = NULL;
CHAR16 *mEnumVariableName = NULL;

// {1FE7ACE3-C36D-4CA7-B4EA-70FF5ADF97DC}
EFI_GUID TestVendorGuid =
{ 0x1fe7ace3, 0xc36d, 0x4ca7, { 0xb4, 0xea, 0x70, 0xff, 0x5a, 0xdf, 0x97, 0xdc } };

typedef struct {
  EFI_GUID *BinaryBlobFvFfsNameGuid;
  EFI_GUID *VendorGuid;
  CHAR16 *VariableName;
} AuthVarInfo;

// Well-known authenticated variables with valid
// data binary blobs baked into the FV.
AuthVarInfo mSecureBootVariables[] = {
  {
    &gEfiSecureBootDbImageGuid,
    &gEfiImageSecurityDatabaseGuid,
    EFI_IMAGE_SECURITY_DATABASE
  },
  {
    &gEfiSecureBootKekImageGuid,
    &gEfiGlobalVariableGuid,
    EFI_KEY_EXCHANGE_KEY_NAME
  }
};

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
        *RightBuffer
        );
    }
  }
}

VOID
VerifyVariableEnumerable (
  IN CHAR16    *TargetVariableName,
  IN EFI_GUID  *TargetVendorGuid,
  IN BOOLEAN   ExpectedFound
  )
{
  BOOLEAN Found;
  UINTN VariableNameBufferSize;
  UINTN VariableNameSize;
  EFI_STATUS Status;
  EFI_GUID VendorGuid;

  SET_LOG_LEVEL(TestLogError);

  Found = FALSE;
  VariableNameBufferSize = sizeof (CHAR16);
  mEnumVariableName = AllocateZeroPool (VariableNameBufferSize);

  for (;;) {
    VariableNameSize = VariableNameBufferSize;
    Status = gRT->GetNextVariableName (
                    &VariableNameSize,
                    mEnumVariableName,
                    &VendorGuid
                    );
    if (Status == EFI_BUFFER_TOO_SMALL) {
      mEnumVariableName = ReallocatePool (
                            VariableNameBufferSize,
                            VariableNameSize,
                            mEnumVariableName
                            );
      VERIFY_IS_NOT_NULL (mEnumVariableName);
      VariableNameBufferSize = VariableNameSize;

      VERIFY_SUCCEEDED (gRT->GetNextVariableName (
                              &VariableNameBufferSize,
                              mEnumVariableName,
                              &VendorGuid)
                              );
    } else if (Status == EFI_NOT_FOUND) {
      // Reached end of variable list.
      break;
    } else {
      VERIFY_SUCCEEDED (Status, "Verify variable retrieval");

      if (CompareGuid (TargetVendorGuid, &VendorGuid) &&
          (StrCmp (TargetVariableName, mEnumVariableName) == 0)) {
        Found = TRUE;
        break;
      }
    }
  }

  VERIFY_ARE_EQUAL (BOOLEAN, ExpectedFound, Found);
  SET_LOG_LEVEL(TestLogComment);
}

VOID
VerifyValidVarWriteReadEnumDel (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  IN UINT32    Attributes,
  IN UINTN     DataSize,
  IN VOID      *Data,
  IN BOOLEAN   VerifyData
  )
{

  UINT32 ActualVarAttributes;
  UINTN ActualDataSize;

  // The verification sequence can be summarized as follows:
  // - Verify that the target variable doesn't exist already.
  // - Verify it can be set based on the input Attributes/Data/etc.
  // - Verify it is present and enumerable.
  // - Verify it can be fetched.
  // - Verify the fetched variable Data matches the one set.
  // - Verify it can be deleted.
  // - Verify it is not present and not enumerable anymore.

  ActualDataSize = 0;
  VERIFY_ARE_EQUAL (EFI_STATUS,
                    EFI_NOT_FOUND,
                    gRT->GetVariable (
                          VariableName,
                          VendorGuid,
                          NULL,
                          &ActualDataSize,
                          NULL
                          ),
                    "Verify variable doesn't exist initially"
                    );

  VERIFY_SUCCEEDED (gRT->SetVariable (
                            VariableName,
                            VendorGuid,
                            Attributes,
                            DataSize,
                            Data),
                    "Setting variable with loaded binary blob"
                    );

  VerifyVariableEnumerable (VariableName,
                            VendorGuid,
                            TRUE
                            );

  ActualDataSize = 0;
  VERIFY_ARE_EQUAL (EFI_STATUS,
                    EFI_BUFFER_TOO_SMALL,
                    gRT->GetVariable (
                          VariableName,
                          VendorGuid,
                          NULL,
                          &ActualDataSize,
                          NULL),
                    "Query size of the existing variable"
                    );

  VERIFY_ARE_EQUAL(UINTN, ActualDataSize, DataSize);
  mActualImageData = AllocatePool(DataSize);
  VERIFY_IS_NOT_NULL(mActualImageData);

  VERIFY_SUCCEEDED (gRT->GetVariable (
                          VariableName,
                          VendorGuid,
                          &ActualVarAttributes,
                          &ActualDataSize,
                          mActualImageData),
                    "Getting variable with the correct size"
                    );

  if (VerifyData != FALSE) {
    VERIFY_ARE_EQUAL (UINT32, Attributes, ActualVarAttributes);
    VerifyAreEqualBytes (Data, mActualImageData, DataSize);
  }

  FreePool (mActualImageData);
  mActualImageData = NULL;

  VERIFY_SUCCEEDED (gRT->SetVariable (
                          VariableName,
                          VendorGuid,
                          Attributes,
                          0,
                          NULL),
                    "Deleting the variable"
                    );

  VerifyVariableEnumerable (VariableName,
                            VendorGuid,
                            FALSE
                            );

  VERIFY_ARE_EQUAL (EFI_STATUS,
                    EFI_NOT_FOUND,
                    gRT->GetVariable (
                          VariableName,
                          VendorGuid,
                          NULL,
                          &ActualDataSize,
                          NULL),
                    "Verify variable doesn't exist anymore"
                    );
}

VOID
TestValidAuthVars (
  VOID
  )
{
  UINTN Idx;
  UINTN ImageSize;
  UINTN VariableCount;
  STATIC CONST UINT32 ExpectedVarAttributes = (EFI_VARIABLE_NON_VOLATILE |
                                               EFI_VARIABLE_RUNTIME_ACCESS |
                                               EFI_VARIABLE_BOOTSERVICE_ACCESS |
                                               EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);

  // The test reads a predefined well-known secureboot authenticated variables from the
  // flash volume (FV). Each variable Data is pre-baked with both the authentication
  // header followed by the actual Data itself in a single binary blob.
  // The mSecureBootVariables table defines those baked variables, and the test
  // simply enumerate that list and for each it runs the VerifyValidVarWriteReadEnumDel
  // sequence.

  ImageSize = 0;
  VariableCount = ARRAY_SIZE (mSecureBootVariables);

  for (Idx = 0; Idx < VariableCount; ++Idx) {
    VERIFY_IS_NULL (mImageData);
    VERIFY_SUCCEEDED (GetSectionFromAnyFv (
                        mSecureBootVariables[Idx].BinaryBlobFvFfsNameGuid,
                        EFI_SECTION_RAW,
                        0,
                        &mImageData,
                        &ImageSize
                        ),
                      "Getting variable binary blob from FV"
                      );

    LOG_COMMENT ("Verifying well-known auth variable %s\n",\
                 mSecureBootVariables[Idx].VariableName
                 );

    VerifyValidVarWriteReadEnumDel (mSecureBootVariables[Idx].VariableName,
                                    mSecureBootVariables[Idx].VendorGuid,
                                    ExpectedVarAttributes,
                                    ImageSize,
                                    mImageData,
                                    FALSE
                                    );
    FreePool (mImageData);
    mImageData = NULL;
  }
}

VOID
TestValidNonVolatileVars (
  VOID
  )
{
  UINTN VariableCount;
  UINTN Idx;
  UINTN ImageSize = 0;
  CHAR16 TestVariableName[32];
  STATIC CONST UINTN BufferSizes[] = {
    1, 2, 3, 4, 8, 16, 32, 64, 128, 512, SIZE_1KB, SIZE_4KB, SIZE_8KB, SIZE_16KB
  };

  STATIC CONST UINT32 ExpectedVarAttributes = (EFI_VARIABLE_NON_VOLATILE |
                                               EFI_VARIABLE_RUNTIME_ACCESS |
                                               EFI_VARIABLE_BOOTSERVICE_ACCESS);

  // The test generates on the fly a set of byte buffers generated randomly with
  // increasing size specified in BufferSizes. Each generated variable Data has
  // a genrated VariableName, and all the variables fall under TestVendorGuid.
  // The test treats each variable as non-volatile and non-authenticated, and then
  // run the VerifyValidVarWriteReadEnumDel sequence on that variable.

  VariableCount = ARRAY_SIZE (BufferSizes);
  for (Idx = 0; Idx < VariableCount; ++Idx) {
    ImageSize = BufferSizes[Idx];
    mImageData = AllocateZeroPool (ImageSize);
    VERIFY_IS_NOT_NULL (mImageData);
    RandomBytes ((UINT8 *)mImageData, ImageSize);

    UnicodeSPrint (TestVariableName,
                   sizeof(TestVariableName),
                   L"TestVariable#%u",
                   (UINT32) ImageSize
                   );

    LOG_COMMENT ("Verifying random non-volatile variable %s of size %u\n",
                 TestVariableName,
                 (UINT32) ImageSize
                 );

    VerifyValidVarWriteReadEnumDel (TestVariableName,
                                    &TestVendorGuid,
                                    ExpectedVarAttributes,
                                    ImageSize,
                                    mImageData,
                                    TRUE
                                    );
    FreePool (mImageData);
    mImageData = NULL;
  }
}

BOOLEAN
ModuleSetup (
  VOID
  )
{
  UINTN DataSize;
  EFI_STATUS Status;

  // PK is present, but our provided buffer is too small which means that
  // SecureBoot is provisioned. It may not be enabled, but we still don't want
  // to mess-up with the current configurations.

  DataSize = 0;
  Status = gRT->GetVariable (
                  EFI_PLATFORM_KEY_NAME,
                  &gEfiGlobalVariableGuid,
                  NULL,
                  &DataSize,
                  NULL
                  );

  if (Status == EFI_BUFFER_TOO_SMALL) {
    LOG_ERROR ("SecureBoot seem to be provisioned. Test requires a clean SecureBoot state.");
    return FALSE;
  }

  return TRUE;
}

BOOLEAN
TestSetup (
  VOID
  )
{
  SET_LOG_LEVEL(TestLogComment);
  RandomSeed (NULL, 0);
  return TRUE;
}

VOID
TestCleanup (
  VOID
  )
{
  if (mImageData != NULL) {
    FreePool (mImageData);
    mImageData = NULL;
  }

  if (mActualImageData != NULL) {
    FreePool (mActualImageData);
    mActualImageData = NULL;
  }

  if (mEnumVariableName != NULL) {
    FreePool (mEnumVariableName);
    mEnumVariableName = NULL;
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

  MODULE_SETUP (ModuleSetup);
  TEST_SETUP (TestSetup);
  TEST_CLEANUP (TestCleanup);
  TEST_FUNC (TestValidNonVolatileVars);
  TEST_FUNC (TestValidAuthVars);

  if (!RUN_MODULE(0, NULL)) {
    return 1;
  }

  return 0;
}
