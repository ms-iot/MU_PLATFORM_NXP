
#include <Uefi.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Library/OpteeClientApiLib.h>
#include <Library/tee_client_api.h>
#include <Guid/OpteeTrustedAppGuids.h>
#include <Library/TimerLib.h>

#include <Protocol/RpmbIo.h>
#include <EdkTest.h>
#include <string.h>

MODULE ("TEE Client API Test for OPTEE Client API Lib");

#define OPTEE_TEST_HELLO_WORLD_CMD_INC_A  (0)

#define OPTEE_TEST_STORAGE_CMD_WRITE (0)
#define OPTEE_TEST_STORAGE_CMD_READ  (1)

EFI_HANDLE mImageHandle;
TEEC_Session mSession;
TEEC_Context mContext;
TEEC_SharedMemory mSharedMem;

VOID
TeecSharedMemAllocateReleaseTest (
  )
{
  TEEC_Result TeecResult;

  TeecResult = TEEC_InitializeContext (NULL, &mContext);
  ASSERT (TeecResult == TEEC_SUCCESS);

  mSharedMem.flags = 0;

  {
    mSharedMem.size = 0x7FFFFFFF;
    TeecResult = TEEC_AllocateSharedMemory (&mContext, &mSharedMem);
    ASSERT (TeecResult == TEEC_ERROR_OUT_OF_MEMORY);
    ASSERT (mSharedMem.buffer == NULL);
    ASSERT (mSharedMem.shadow_buffer == mSharedMem.buffer);
  }

  {
    mSharedMem.size = 0;
    TeecResult = TEEC_AllocateSharedMemory (&mContext, &mSharedMem);
    ASSERT (TeecResult == TEEC_SUCCESS);
    ASSERT (mSharedMem.buffer != NULL);
    ASSERT (mSharedMem.shadow_buffer == mSharedMem.buffer);

    TEEC_ReleaseSharedMemory (&mSharedMem);
    ASSERT (mSharedMem.buffer == NULL);
    ASSERT (mSharedMem.size == 0);
    ASSERT (mSharedMem.shadow_buffer == mSharedMem.buffer);
  }

  {
    mSharedMem.size = 1;
    TeecResult = TEEC_AllocateSharedMemory (&mContext, &mSharedMem);
    ASSERT (TeecResult == TEEC_SUCCESS);
    ASSERT (mSharedMem.buffer != NULL);
    ASSERT (mSharedMem.shadow_buffer == mSharedMem.buffer);

    SetMem (mSharedMem.buffer, mSharedMem.size, 0xA5);

    TEEC_ReleaseSharedMemory (&mSharedMem);
    ASSERT (mSharedMem.buffer == NULL);
    ASSERT (mSharedMem.size == 0);
    ASSERT (mSharedMem.shadow_buffer == mSharedMem.buffer);
  }

  {
    mSharedMem.size = 0x10000;
    TeecResult = TEEC_AllocateSharedMemory (&mContext, &mSharedMem);
    ASSERT (TeecResult == TEEC_SUCCESS);
    ASSERT (mSharedMem.buffer != NULL);
    ASSERT (mSharedMem.shadow_buffer == mSharedMem.buffer);

    SetMem (mSharedMem.buffer, mSharedMem.size, 0xA5);

    TEEC_ReleaseSharedMemory (&mSharedMem);
    ASSERT (mSharedMem.buffer == NULL);
    ASSERT (mSharedMem.size == 0);
    ASSERT (mSharedMem.shadow_buffer == mSharedMem.buffer);
  }

  // Bulk allocation
  {
    TEEC_SharedMemory TeecMemoryBulk[16];
    int Loop;

    // Allocate & write
    //
    for (Loop = 0; Loop < 16; Loop++) {

      TeecMemoryBulk[Loop].size = 256;
      TeecMemoryBulk[Loop].flags = 0;

      TeecResult = TEEC_AllocateSharedMemory (&mContext, &TeecMemoryBulk[Loop]);
      ASSERT (TeecResult == TEEC_SUCCESS);
      ASSERT (TeecMemoryBulk[Loop].shadow_buffer != NULL);

      SetMem (TeecMemoryBulk[Loop].buffer, TeecMemoryBulk[Loop].size, Loop);
    }

    // Free
    //
    for (Loop = 0; Loop < 16; Loop++) {

      TEEC_ReleaseSharedMemory (&TeecMemoryBulk[Loop]);
      ASSERT (TeecMemoryBulk[Loop].shadow_buffer == NULL);
    }
  }
}

/**
 Test Case: TEEC_InvokeCommand
  - This uses the "Hello World" test TA that has a single function
    to increment a value parameter.
**/
VOID
TeecInvokeCommandTest (
  )
{
  uint32_t ErrorOrigin;
  TEEC_UUID TeecUuid;

  C_ASSERT (sizeof (TeecUuid) == sizeof (gOpteeHelloWorldTaGuid));
  CopyMem (&TeecUuid, &gOpteeHelloWorldTaGuid, sizeof (TeecUuid));

  TeecUuid.timeLow = SwapBytes32 (TeecUuid.timeLow);
  TeecUuid.timeMid = SwapBytes16 (TeecUuid.timeMid);
  TeecUuid.timeHiAndVersion = SwapBytes16 (TeecUuid.timeHiAndVersion);

  VERIFY_ARE_EQUAL (
    TEEC_Result,
    TEEC_SUCCESS,
    TEEC_OpenSession (
      &mContext,
      &mSession,
      &TeecUuid,
      TEEC_LOGIN_PUBLIC,
      NULL,
      NULL,
      &ErrorOrigin));

  // This command should be failed by the TA.
  //
  {
    TEEC_Operation TeecOperation = { 0 };
    VERIFY_ARE_NOT_EQUAL (
      TEEC_Result,
      TEEC_SUCCESS,
      TEEC_InvokeCommand (
        &mSession,
        0xDEAD,
        &TeecOperation,
        &ErrorOrigin));

    VERIFY_ARE_EQUAL (
      TEEC_Result,
      TEEC_ORIGIN_TRUSTED_APP,
      ErrorOrigin);
  }

  // This command should increment value parameter "a".
  //
  {
    TEEC_Operation TeecOperation = { 0 };
    UINT32 a = 0x11;
    UINT32 b = 0x22;

    TeecOperation.paramTypes = TEEC_PARAM_TYPES (
                                  TEEC_VALUE_INOUT,
                                  TEEC_NONE,
                                  TEEC_NONE,
                                  TEEC_NONE);
    TeecOperation.params[0].value.a = a;
    TeecOperation.params[0].value.b = b;

    VERIFY_ARE_EQUAL (
      TEEC_Result,
      TEEC_SUCCESS,
      TEEC_InvokeCommand (
        &mSession,
        OPTEE_TEST_HELLO_WORLD_CMD_INC_A,
        &TeecOperation,
        &ErrorOrigin));

    VERIFY_ARE_EQUAL (
      uint32_t,
      a + 1,
      TeecOperation.params[0].value.a);

    VERIFY_ARE_EQUAL (
      uint32_t,
      b,
      TeecOperation.params[0].value.b);
  }
}

BOOLEAN
ModuleSetup (
  VOID
  )
{
  EFI_STATUS Status;

  Status = OpteeClientApiInitialize (mImageHandle);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("OpteeClientApiInitialize() failed. (Status=%r)", Status);
    return FALSE;
  }

  return TRUE;
}

VOID
ModuleCleanup (
  VOID
  )
{
  OpteeClientApiFinalize ();
}


BOOLEAN
TestSetup (
  )
{
  if (TEEC_InitializeContext (NULL, &mContext) != TEEC_SUCCESS) {
    LOG_ERROR ("TEEC_InitializeContext() failed.");
    return FALSE;
  }

  ZeroMem (&mSession, sizeof (mSession));
  ZeroMem (&mSharedMem, sizeof (mSharedMem));

  return TRUE;
}

VOID
TestCleanup (
  VOID
  )
{
  if (mSession.session_id != 0) {
    TEEC_CloseSession (&mSession);
    mSession.session_id = 0;
  }

  if (mSharedMem.buffer != NULL) {
    TEEC_ReleaseSharedMemory (&mSharedMem);
    mSharedMem.buffer = NULL;
  }

  TEEC_FinalizeContext (&mContext);
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
  MODULE_CLEANUP (ModuleCleanup);
  TEST_SETUP (TestSetup);
  TEST_CLEANUP (TestCleanup);
  TEST_FUNC (TeecSharedMemAllocateReleaseTest);
  TEST_FUNC (TeecInvokeCommandTest);

  if (!RUN_MODULE (0, NULL)) {
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}
