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

      Status = gRT->GetNextVariableName (
                    &VariableNameSize,
                    mEnumVariableName,
                    &VendorGuid
                    );
    }

    if (Status == EFI_NOT_FOUND) {
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

  SET_LOG_LEVEL(TestLogComment);
  if (ExpectedFound != Found) {
    LOG_COMMENT("Was expecting to find value? %S Found value? %S\n",
      ExpectedFound ? L"YES" : L"NO",
      Found ? L"YES" : L"NO");
  }
  VERIFY_ARE_EQUAL (BOOLEAN, ExpectedFound, Found);
  if (mEnumVariableName) {
    FreePool(mEnumVariableName);
    mEnumVariableName = NULL;
  }
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

  // TODO: Verify the data after stripping out the header.
  //        The binary includes authentication information
  //        which is not preserved by the variable store.

  if (VerifyData != FALSE) {
    VERIFY_ARE_EQUAL(UINTN, ActualDataSize, DataSize);
  }

  mActualImageData = AllocatePool(ActualDataSize);
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
                          0,
                          DataSize,
                          Data),
                    "Deleting the variable"
                    );

  VerifyVariableEnumerable (VariableName,
                            VendorGuid,
                            FALSE
                            );

  ActualDataSize = 0;
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

/**
  Append/Create/Replace a variable, then verify the operation by requesting the variable back.

  @param  VariableName        Name of the variable to operate on.
  @param  VendorGuid          Variable GUID
  @param  BaseAttributes      The attributes the varaible is expected to have after it has been updated.
  @param  ExtraSetAttributes  Any additional attributes which should be passed to the Set command
                              ie EFI_VARIABLE_APPEND_WRITE
  @param  DataAddSize         Size of the data to add.
  @param  DataAdd             Buffer containing the data to add to the varaible.
  @param  DataCheckSize       The size of the expected contents of the varaible after the Set operation.
  @param  DataCheck           The expected contents of the variable.
**/
VOID
SetAndCheckVariable(
    IN CHAR16   *VariableName,
    IN EFI_GUID *VendorGuid,
    IN UINT32   BaseAttributes,
    IN UINT32   ExtraSetAttributes,
    IN UINTN    DataAddSize,
    IN VOID     *DataAdd,
    IN UINTN    DataCheckSize,
    IN VOID     *DataCheck
  )
{
  UINTN ActualDataSize = DataCheckSize;
  UINT32 ActualVarAttributes;

  // Note: Stomps on mActualImageData
  SET_LOG_LEVEL(TestLogError);
  VERIFY_IS_NOT_NULL(mActualImageData);
  SET_LOG_LEVEL(TestLogComment);

  VERIFY_SUCCEEDED (gRT->SetVariable (
                            VariableName,
                            VendorGuid,
                            BaseAttributes | ExtraSetAttributes,
                            DataAddSize,
                            DataAdd),
                    "Setting %s with initial value",
                    VariableName
                    );

  VERIFY_SUCCEEDED (gRT->GetVariable (
                          VariableName,
                          VendorGuid,
                          &ActualVarAttributes,
                          &ActualDataSize,
                          mActualImageData),
                    "Checking %s is set correctly",
                    VariableName
                    );
  VERIFY_ARE_EQUAL (UINT32, BaseAttributes, ActualVarAttributes);
  VERIFY_ARE_EQUAL (UINTN, DataCheckSize, ActualDataSize);
  VerifyAreEqualBytes (DataCheck, mActualImageData, ActualDataSize);

  VerifyVariableEnumerable (VariableName,
                            VendorGuid,
                            TRUE
                            );
}

/**
  Delete a variable, then verify the operation by requesting the variable back.

  @param  VariableName        Name of the variable to operate on.
  @param  VendorGuid          Variable GUID
  @param  BaseAttributes      The attributes the varaible is expected to have after it has been updated.
  @param  ExtraSetAttributes  Any additional attributes which should be passed to the Set command
**/
VOID
DeleteAndCheckVariable(
    IN CHAR16   *VariableName,
    IN EFI_GUID *VendorGuid,
    IN UINT32   BaseAttributes,
    IN UINT32   ExtraSetAttributes
  )
{
  UINTN DataSize = 0;
  UINTN ActualDataSize = 0;
  UINT32 ActualVarAttributes;

  VERIFY_SUCCEEDED (gRT->SetVariable (
                            VariableName,
                            VendorGuid,
                            BaseAttributes | ExtraSetAttributes,
                            DataSize,
                            NULL),
                    "Deleting %s",
                    VariableName
                    );

  VERIFY_ARE_EQUAL (EFI_STATUS,
                    EFI_NOT_FOUND,
                    gRT->GetVariable (
                          VariableName,
                          VendorGuid,
                          &ActualVarAttributes,
                          &ActualDataSize,
                          NULL),
                    "Checking %s was deleted",
                    VariableName
                    );

  VerifyVariableEnumerable (VariableName,
                            VendorGuid,
                            FALSE
                            );
}

/**
  Writes and updates a set of persistant variables in an interleaved order. The
  order is intended to exercise the TA based AuthVar's memory management.

  NOTE: The authvar TA v1.1 no longer implements block based memory, but this is
  still and interesting test which exercises multiple operations.

  This test will also verify the presence and accuracy of these variables from
  a previous run. If the values do not match the PersistanceTestFlag will be cleared
  and a subsequent run will reset the values.
**/
VOID
VerifyAppendShrinkReplacePersist (
    VOID
  )
{
  STATIC CHAR16 PersistanceTestFlag[] = L"PersistanceTestFlag";
  STATIC CHAR16 TestVariableName1[] = L"InterleaveTestVar1";
  STATIC CHAR16 TestVariableName2[] = L"InterleaveTestVar2";
  STATIC CHAR16 TestVariableName3[] = L"InterleaveTestVar3";
  STATIC CONST UINT32 ChunkSize = 0x10;
  STATIC CONST UINT32 MaxChunksPerVar = 10;
  STATIC CONST UINT32 BufferSize = ChunkSize * MaxChunksPerVar * 3;
  VOID *Buffer1;
  VOID *Buffer2;
  VOID *Buffer3;

  UINTN Idx;
  UINT32 ActualVarAttributes;
  UINTN ActualDataSize;
  UINT8 Persistanceflag;

  STATIC CONST UINT32 BaseAttributes = (EFI_VARIABLE_NON_VOLATILE |
                                        EFI_VARIABLE_RUNTIME_ACCESS |
                                        EFI_VARIABLE_BOOTSERVICE_ACCESS);
  EFI_STATUS Status;

  mImageData = AllocateZeroPool (BufferSize);
  VERIFY_IS_NOT_NULL (mImageData);

  Buffer1 = mImageData;
  Buffer2 = Buffer1 + ChunkSize * MaxChunksPerVar;
  Buffer3 = Buffer2 + ChunkSize * MaxChunksPerVar;

  mActualImageData = AllocatePool(BufferSize);
  VERIFY_IS_NOT_NULL(mActualImageData);

  // To allow for checking persistance it is important that the data
  // is deterministic.
  for(Idx = 0; Idx < BufferSize; Idx++) {
    ((UINT8*)mImageData)[Idx] = (UINT8)Idx;
  }

  ActualDataSize = 0;
  Status = gRT->GetVariable (
                  PersistanceTestFlag,
                  &TestVendorGuid,
                  NULL,
                  &ActualDataSize,
                  NULL
                  );

  // Check if the test  has been run before, if so verify that the variables
  // were persisted correctly.

  if (Status != EFI_NOT_FOUND) {
    VERIFY_ARE_EQUAL (EFI_STATUS, Status, EFI_BUFFER_TOO_SMALL);

    // We only want to check persistance if we succeeded on
    // all of the subsequent writes. Clear the flag so the varaibles
    // don't get left in a bad state if the test fails halfway through.
    DeleteAndCheckVariable(
      PersistanceTestFlag,
      &TestVendorGuid,
      BaseAttributes,
      0
      );

    ActualDataSize = BufferSize;
    VERIFY_SUCCEEDED (gRT->GetVariable (
                            TestVariableName1,
                            &TestVendorGuid,
                            &ActualVarAttributes,
                            &ActualDataSize,
                            mActualImageData),
                      "Checking variable 1 persisted correctly"
                      );
    VERIFY_ARE_EQUAL (UINT32, BaseAttributes, ActualVarAttributes);
    VERIFY_ARE_EQUAL (UINTN, ChunkSize * 4, ActualDataSize);
    VerifyAreEqualBytes (Buffer1, mActualImageData, ActualDataSize);

    ActualDataSize = BufferSize;
    VERIFY_SUCCEEDED (gRT->GetVariable (
                            TestVariableName2,
                            &TestVendorGuid,
                            &ActualVarAttributes,
                            &ActualDataSize,
                            mActualImageData),
                      "Checking variable 2 persisted correctly"
                      );
    VERIFY_ARE_EQUAL (UINT32, BaseAttributes, ActualVarAttributes);
    VERIFY_ARE_EQUAL (UINTN, ChunkSize * 1, ActualDataSize);
    VerifyAreEqualBytes (Buffer2, mActualImageData, ActualDataSize);


    ActualDataSize = BufferSize;
    VERIFY_SUCCEEDED (gRT->GetVariable (
                            TestVariableName3,
                            &TestVendorGuid,
                            &ActualVarAttributes,
                            &ActualDataSize,
                            mActualImageData),
                      "Checking variable 3 persisted correctly"
                      );
    VERIFY_ARE_EQUAL (UINT32, BaseAttributes, ActualVarAttributes);
    VERIFY_ARE_EQUAL (UINTN, ChunkSize * 1, ActualDataSize);
    VerifyAreEqualBytes (Buffer3, mActualImageData, ActualDataSize);
  }

  LOG_COMMENT ("Testing appends and truncates on persistant variables\n");

  LOG_COMMENT ("<11> - Create");
  SetAndCheckVariable (
    TestVariableName1,
    &TestVendorGuid,
    BaseAttributes,
    0,
    ChunkSize,
    Buffer1,
    ChunkSize,
    Buffer1
    );

  LOG_COMMENT ("<1111> - Append");
  SetAndCheckVariable (
    TestVariableName1,
    &TestVendorGuid,
    BaseAttributes,
    EFI_VARIABLE_APPEND_WRITE,
    ChunkSize,
    Buffer1 + ChunkSize,
    ChunkSize * 2,
    Buffer1
    );

  LOG_COMMENT ("<1111> <22> - Create");
  SetAndCheckVariable(
    TestVariableName2,
    &TestVendorGuid,
    BaseAttributes,
    0,
    ChunkSize,
    Buffer2,
    ChunkSize,
    Buffer2
    );

  LOG_COMMENT ("<1111...> <22> <...1111> - Append");
  SetAndCheckVariable(
    TestVariableName1,
    &TestVendorGuid,
    BaseAttributes,
    EFI_VARIABLE_APPEND_WRITE,
    ChunkSize * 2,
    Buffer1 + (ChunkSize * 2),
    ChunkSize * 4,
    Buffer1
    );

  LOG_COMMENT ("<1111...> <22> <...11__> - Trim");
  SetAndCheckVariable(
    TestVariableName1,
    &TestVendorGuid,
    BaseAttributes,
    0,
    ChunkSize * 3,
    Buffer1,
    ChunkSize * 3,
    Buffer1
    );

  LOG_COMMENT ("<1111...> <__> <...11__> - Delete");
  DeleteAndCheckVariable(
    TestVariableName2,
    &TestVendorGuid,
    BaseAttributes,
    0
    );

  LOG_COMMENT ("<1111...> <__> <...11__> <33> - Create");
  SetAndCheckVariable(
    TestVariableName3,
    &TestVendorGuid,
    BaseAttributes,
    0,
    ChunkSize,
    Buffer3,
    ChunkSize,
    Buffer3
    );

  LOG_COMMENT ("<1111...> <__> <...11__...> <33> <...11> - Append");
  SetAndCheckVariable(
    TestVariableName1,
    &TestVendorGuid,
    BaseAttributes,
    EFI_VARIABLE_APPEND_WRITE,
    ChunkSize * 1,
    Buffer1 + (ChunkSize * 3),
    ChunkSize * 4,
    Buffer1
    );

  LOG_COMMENT ("<1111...> <__> <...11__...> <33> <...11> <22>- Create");
  SetAndCheckVariable(
    TestVariableName2,
    &TestVendorGuid,
    BaseAttributes,
    0,
    ChunkSize,
    Buffer2,
    ChunkSize,
    Buffer2
    );

  // Indicate that all writes succeeded. The variables will be checked
  // for persistance if the test is run again after a reboot.
  Persistanceflag = 1;
  VERIFY_SUCCEEDED (gRT->SetVariable (
                          PersistanceTestFlag,
                          &TestVendorGuid,
                          BaseAttributes,
                          1,
                          &Persistanceflag),
                    "Setting persistant test flag"
                    );

  FreePool (mImageData);
  mImageData = NULL;

}

/**
  Basic test of unicode handling.
**/
VOID
VerifyNameHandling (
  VOID
  )
{
  STATIC CONST UINT32 Attributes = (EFI_VARIABLE_NON_VOLATILE |
                                    EFI_VARIABLE_RUNTIME_ACCESS |
                                    EFI_VARIABLE_BOOTSERVICE_ACCESS);
  STATIC CHAR16 TestVariableName1[] = L"NameTestVariable";
  STATIC CHAR16 TestVariableName2[] = L"NameTestVariable\0\0\0\0";
  STATIC CHAR16 EmptyString1[] = L"";
  STATIC CHAR16 EmptyString2[] = L"\0";
  CONST UINT32 LongStringLength = 3000;
  STATIC CHAR16 LongString[3000];
  STATIC CHAR16 Reference[] = L"abcd";
  STATIC CONST UINT32 NameListLength = 6;
  STATIC CHAR16 *TestVariableNameList[] = {
                                        L"\u00E0bcd",
                                        L"ab\\cd",
                                        L"ab/cd",
                                        L"AbCd",
                                        L"a bcd",
                                        L"a?cd"
                                      };
  CONST UINTN DataSize = 0x200;
  UINT32 ActualVarAttributes;
  UINTN ActualDataSize;
  UINT32 i;

  mImageData = AllocateZeroPool (DataSize);
  VERIFY_IS_NOT_NULL (mImageData);
  RandomBytes (mImageData, DataSize);

  mActualImageData = AllocatePool(DataSize);
  VERIFY_IS_NOT_NULL(mActualImageData);

  // Check strings with excessive length (extra terminating nulls)
  // First set it with the normal name, then verify it is still
  // accessible with padded name.
  VERIFY_SUCCEEDED (gRT->SetVariable (
                          TestVariableName1,
                          &TestVendorGuid,
                          Attributes,
                          DataSize,
                          mImageData),
                    "Setting %s with initial value",
                    TestVariableName1
                    );

  ActualDataSize = DataSize;
  VERIFY_SUCCEEDED (gRT->GetVariable (
                          TestVariableName1,
                          &TestVendorGuid,
                          &ActualVarAttributes,
                          &ActualDataSize,
                          mActualImageData),
                    "Checking %s is set correctly",
                    TestVariableName1
                    );
  VERIFY_ARE_EQUAL (UINT32, Attributes, ActualVarAttributes);
  VERIFY_ARE_EQUAL (UINTN, DataSize, ActualDataSize);
  VerifyAreEqualBytes (mImageData, mActualImageData, ActualDataSize);

  VerifyVariableEnumerable (
    TestVariableName1,
    &TestVendorGuid,
    TRUE
    );

  // Now check it is still accessible with the null padded name.
  ActualDataSize = DataSize;
  VERIFY_SUCCEEDED (gRT->GetVariable (
                          TestVariableName2,
                          &TestVendorGuid,
                          &ActualVarAttributes,
                          &ActualDataSize,
                          mActualImageData),
                    "Checking %s is set correctly with extra nulls",
                    TestVariableName2
                    );
  VERIFY_ARE_EQUAL (UINT32, Attributes, ActualVarAttributes);
  VERIFY_ARE_EQUAL (UINTN, DataSize, ActualDataSize);
  VerifyAreEqualBytes (mImageData, mActualImageData, ActualDataSize);

  VerifyVariableEnumerable (
    TestVariableName2,
    &TestVendorGuid,
    TRUE
    );

  DeleteAndCheckVariable(
    TestVariableName2,
    &TestVendorGuid,
    Attributes,
    0
    );

  // Make sure empty strings fail correctly.
  VERIFY_ARE_EQUAL (EFI_STATUS,
                    EFI_INVALID_PARAMETER,
                    gRT->SetVariable (
                          EmptyString1,
                          &TestVendorGuid,
                          Attributes,
                          DataSize,
                          mImageData),
                    "Setting with empty string 1"
                    );

  VERIFY_ARE_EQUAL (EFI_STATUS,
                    EFI_INVALID_PARAMETER,
                    gRT->SetVariable (
                          EmptyString2,
                          &TestVendorGuid,
                          Attributes,
                          DataSize,
                          mImageData),
                    "Setting with empty string 2"
                    );

  VERIFY_ARE_EQUAL (EFI_STATUS,
                    EFI_INVALID_PARAMETER,
                    gRT->SetVariable (
                          NULL,
                          &TestVendorGuid,
                          Attributes,
                          DataSize,
                          mImageData),
                    "Setting with null string"
                    );

  // Try very long names (In theory there should be no limit other than the OP-TEE buffer size)
  for (i = 0; i < LongStringLength - 1; i++) {
    // Build a long name of the form "abc...xyzabc..."
    LongString[i] = ('a' + (i % 26));
  }
  LongString[i] = '\0';
  SetAndCheckVariable(
    LongString,
    &TestVendorGuid,
    Attributes,
    0,
    DataSize,
    mImageData,
    ActualDataSize,
    mActualImageData
    );

  DeleteAndCheckVariable(
    LongString,
    &TestVendorGuid,
    Attributes,
    0
    );

  // Run through each name in the list and make sure it can be set, and
  // is not confused with the reference name.
  for (i = 0; i < NameListLength; i++) {
    FreePool (mImageData);
    mImageData = NULL;
    mImageData = AllocateZeroPool (DataSize);
    VERIFY_IS_NOT_NULL (mImageData);
    RandomBytes (mImageData, DataSize);

    // Make sure none of the other variables are confused with
    // this one.
    VerifyVariableEnumerable (
      TestVariableNameList[i],
      &TestVendorGuid,
      FALSE
      );

    // Attempt to set the variable
    SetAndCheckVariable(
      TestVariableNameList[i],
      &TestVendorGuid,
      Attributes,
      0,
      DataSize,
      mImageData,
      ActualDataSize,
      mActualImageData
      );

    // Make sure we don't confuse this with the reference
    VerifyVariableEnumerable (
      Reference,
      &TestVendorGuid,
      FALSE
      );
  }

  // Clean up the varaibles once we are sure there are no
  // conflicts
  for (i = 0; i < NameListLength; i++) {
    DeleteAndCheckVariable(
      TestVariableNameList[i],
      &TestVendorGuid,
      Attributes,
      0
      );
  }
}

/**
  Test of the variable query functionality. The test will check how much space
  is available, and make sure that space updates correctly as variables are added.

**/
VOID
VerifyQuery (
  VOID
  )
{
  STATIC CONST UINT32 AttributesNV = (EFI_VARIABLE_NON_VOLATILE |
                                    EFI_VARIABLE_RUNTIME_ACCESS |
                                    EFI_VARIABLE_BOOTSERVICE_ACCESS);
  STATIC CONST UINT32 AttributesVolatile = (EFI_VARIABLE_RUNTIME_ACCESS |
                                    EFI_VARIABLE_BOOTSERVICE_ACCESS);
  UINT32 Attributes[] = {AttributesNV, AttributesVolatile};

  STATIC CHAR16 QueryVariableName[] = L"QueryTestVariable";
  UINT64 MaximumVariableStorageSize;
  UINT64 RemainingVariableStorageSize;
  UINT64 MaximumVariableSize;
  UINT64 LargerVariableSize;
  UINT64 InitialSpaceAvailable;
  UINT64 InitialMaximumVariableSize;
  UINT32 i;

  for(i = 0; i < 2; i++) {
    VERIFY_SUCCEEDED (gRT->QueryVariableInfo (
                            Attributes[i],
                            &MaximumVariableStorageSize,
                            &RemainingVariableStorageSize,
                            &MaximumVariableSize),
                      "Querying variables"
                      );

    LargerVariableSize = MaximumVariableSize + 0x100;
    InitialSpaceAvailable = RemainingVariableStorageSize;
    InitialMaximumVariableSize = MaximumVariableSize;

    mImageData = AllocateZeroPool (LargerVariableSize);
    VERIFY_IS_NOT_NULL (mImageData);
    RandomBytes (mImageData, LargerVariableSize);
    mActualImageData = AllocatePool(LargerVariableSize);
    VERIFY_IS_NOT_NULL(mActualImageData);

    LOG_COMMENT("Current largest data is 0x%llx, going to make it 0x%llx\n", MaximumVariableSize, LargerVariableSize);

    SetAndCheckVariable(
      QueryVariableName,
      &TestVendorGuid,
      Attributes[i],
      0,
      LargerVariableSize,
      mImageData,
      LargerVariableSize,
      mImageData
      );

    VERIFY_SUCCEEDED (gRT->QueryVariableInfo (
                            Attributes[i],
                            &MaximumVariableStorageSize,
                            &RemainingVariableStorageSize,
                            &MaximumVariableSize),
                      "Querying variables"
                      );

    VERIFY_IS_TRUE (RemainingVariableStorageSize <= (InitialSpaceAvailable - LargerVariableSize),
                    "0x%llx bytes were available, stored 0x%llx bytes, now 0x%llx available",
                    InitialSpaceAvailable,
                    LargerVariableSize,
                    RemainingVariableStorageSize
                    );

  /*  VERIFY_IS_GREATER_THAN (UINT64,
                            RemainingVariableStorageSize - 1,
                            (InitialSpaceAvailable - LargerVariableSize),
                            "0x%llx bytes were available, stored 0x%llx bytes, now 0x%llx available",
                            InitialSpaceAvailable,
                            LargerVariableSize,
                            RemainingVariableStorageSize
                            );*/

    VERIFY_IS_TRUE ((MaximumVariableSize > InitialMaximumVariableSize),
                    "New max size is 0x%llx , was 0x%llx",
                    MaximumVariableSize,
                    InitialMaximumVariableSize
                    );

  /*  VERIFY_IS_GREATER_THAN (UINT64, MaximumVariableSize, InitialMaximumVariableSize,
                            "New max size is 0x%llx , was 0x%llx",
                            MaximumVariableSize,
                            InitialMaximumVariableSize
                            );*/

    DeleteAndCheckVariable(
      QueryVariableName,
      &TestVendorGuid,
      Attributes[i],
      0
      );

    FreePool (mImageData);
    mImageData = NULL;
    FreePool (mActualImageData);
    mActualImageData = NULL;
  }
}

/**
  Attempts to fill the variable store with a large variable, then verifies that additional
  writes fail.

  This test should be run last since, depending on implementation details, it may fill most
  of the available space in the store. (The spec does not require deleted variables to be
  freed until the next reboot)
**/
VOID
VerifyOverflow (
  VOID
  )
{
  STATIC CONST UINT32 Attributes = (EFI_VARIABLE_NON_VOLATILE |
                                    EFI_VARIABLE_RUNTIME_ACCESS |
                                    EFI_VARIABLE_BOOTSERVICE_ACCESS);
  STATIC CHAR16 QueryOverflowVariableName1[] = L"TestVariableOverflow1";
  STATIC CHAR16 QueryOverflowVariableName2[] = L"TestVariableOverflow2";
  UINT64 MaximumVariableStorageSize;
  UINT64 RemainingVariableStorageSize;
  UINT64 MaximumVariableSize;
  UINT64 LargerVariableSize;
  UINT32 OverflowCount, OverflowGoal;

  VERIFY_SUCCEEDED (gRT->QueryVariableInfo (
                          Attributes,
                          &MaximumVariableStorageSize,
                          &RemainingVariableStorageSize,
                          &MaximumVariableSize),
                    "Querying variables"
                    );

  LargerVariableSize = 0x2000;
  OverflowGoal = (RemainingVariableStorageSize / LargerVariableSize);
  LOG_COMMENT("Reporting 0x%llx bytes available, going to try to store 0x%llx in %d chunks",
              RemainingVariableStorageSize,
              LargerVariableSize * OverflowGoal,
              OverflowGoal + 1);

  mImageData = AllocateZeroPool (LargerVariableSize);
  VERIFY_IS_NOT_NULL (mImageData);
  RandomBytes (mImageData, LargerVariableSize);

  for(OverflowCount = 0; OverflowCount < OverflowGoal; OverflowCount++)
  {
    VERIFY_SUCCEEDED(gRT->SetVariable (
                            QueryOverflowVariableName1,
                            &TestVendorGuid,
                            Attributes | EFI_VARIABLE_APPEND_WRITE,
                            LargerVariableSize,
                            mImageData),
                      "Filling storage");
  }

  VERIFY_SUCCEEDED (gRT->QueryVariableInfo (
                          Attributes,
                          &MaximumVariableStorageSize,
                          &RemainingVariableStorageSize,
                          &MaximumVariableSize),
                    "Querying variables"
                    );

  VERIFY_IS_LESS_THAN (UINT64,
                        RemainingVariableStorageSize,
                        LargerVariableSize,
                        "We are going to try to store 0x%llx into 0x%llx available",
                        LargerVariableSize,
                        RemainingVariableStorageSize);

  VERIFY_ARE_EQUAL (EFI_STATUS,
                    EFI_OUT_OF_RESOURCES,
                    gRT->SetVariable (
                          QueryOverflowVariableName1,
                          &TestVendorGuid,
                          Attributes | EFI_VARIABLE_APPEND_WRITE,
                          LargerVariableSize,
                          mImageData),
                    "Trying to overflow the storage by appending"
                    );

  VERIFY_ARE_EQUAL (EFI_STATUS,
                    EFI_OUT_OF_RESOURCES,
                    gRT->SetVariable (
                          QueryOverflowVariableName2,
                          &TestVendorGuid,
                          Attributes,
                          LargerVariableSize,
                          mImageData),
                    "Trying to overflow the storage by creating new variable"
                    );

  DeleteAndCheckVariable(
    QueryOverflowVariableName1,
    &TestVendorGuid,
    Attributes,
    0
    );

  // This variable should never have been created.
  VerifyVariableEnumerable (
    QueryOverflowVariableName2,
    &TestVendorGuid,
    FALSE
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

    if (mImageData != NULL) {
      FreePool (mImageData);
      mImageData = NULL;
    }
  }
}

VOID
TestInvalidAuthVars (
  VOID
  )
{
  UINTN ImageSize;
  STATIC CONST UINT32 ExpectedVarAttributes = (EFI_VARIABLE_NON_VOLATILE |
                                               EFI_VARIABLE_RUNTIME_ACCESS |
                                               EFI_VARIABLE_BOOTSERVICE_ACCESS |
                                               EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);

  // The test attempts two write a badly formed authenticated variable.

  ImageSize = 1000;
  mImageData = AllocateZeroPool (ImageSize);
  VERIFY_IS_NOT_NULL (mImageData);

  RandomBytes (mImageData, ImageSize);

  LOG_COMMENT ("Verifying garbage authenticated variable %s\n",\
                mSecureBootVariables[0].VariableName
                );

   VERIFY_ARE_EQUAL (EFI_STATUS,
                      EFI_SECURITY_VIOLATION,
                      gRT->SetVariable (
                            mSecureBootVariables[0].VariableName,
                            mSecureBootVariables[0].VendorGuid,
                            ExpectedVarAttributes,
                            ImageSize,
                            mImageData),
                    "Setting persistant test flag"
                    );

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
  LOG_COMMENT ("Getting variable %S", EFI_PLATFORM_KEY_NAME);
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

  // Always clear out the overflow variable, this can break
  // other tests.
  gRT->SetVariable (
    L"TestVariableOverflow1",
    &TestVendorGuid,
    0,
    0,
    NULL);
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
  TEST_FUNC (VerifyAppendShrinkReplacePersist);
  TEST_FUNC (VerifyNameHandling);
  TEST_FUNC (VerifyQuery);
  TEST_FUNC (TestValidAuthVars);
  TEST_FUNC (TestInvalidAuthVars);

  // Run this last
  TEST_FUNC (VerifyOverflow);

  if (!RUN_MODULE(0, NULL)) {
    return 1;
  }

  return 0;
}
