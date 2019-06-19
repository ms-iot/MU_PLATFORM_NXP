/* @file
Runtime DXE part corresponding to OpTEE authenticated variable trusted application.
This module installs variable arch protocol and variable write arch protocol to provide
variable service. This module works together with OpTEE authenticated variable trusted
application.
Note that after ExitBootServices the UEFI variable services is not available at runtime
in this implementation and instead Windows uses the TREE driver to directly interact
with the OpTEE Auth. Var. Trusted Application.
Caution: This module requires additional review when modified.
This driver will have external input - variable data.
This external input must be validated carefully to avoid security issues such as
buffer overflow or integer overflow.
Copyright (c), Microsoft Corporation. All rights reserved.
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
#include <Protocol/SimpleFileSystem.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/OpteeClientApiLib.h>
#include <Library/tee_client_api.h>
#include <Library/TimerLib.h>
#include <Library/PerformanceLib.h>
#include <Library/DevicePathLib.h>

#include <Guid/GlobalVariable.h>
#include <Guid/EventGroup.h>
#include <Guid/AuthenticatedVariableFormat.h>
#include <Guid/ImageAuthentication.h>
#include <Guid/OpteeTrustedAppGuids.h>

#include <limits.h>

#ifndef min
# define min(a,b) ((a) < (b) ? (a) : (b))
#endif

/*
Performance Tokens
*/
#define GET_VAR_TOK                     "VAR:GET"
#define SET_VAR_TOK                     "VAR:SET"
#define GET_NEXT_VAR_NAME_TOK           "VAR:GET-NEXT"
#define QUERY_VAR_INFO_TOK              "VAR:QUERY"

/*
Logging Macros
*/

#define LOG_TRACE_FMT_HELPER(FUNC, FMT, ...) \
  "AUTH-VAR[T]:%a:" FMT "%a\n", FUNC, __VA_ARGS__

#define LOG_INFO_FMT_HELPER(FMT, ...) \
  "AUTH-VAR[I]:" FMT "%a\n", __VA_ARGS__

#define LOG_ERROR_FMT_HELPER(FMT, ...) \
  "AUTH-VAR[E]:" FMT " (%a: %a, %d)\n", __VA_ARGS__

#define LOG_INFO(...) \
  DEBUG ((DEBUG_INFO | DEBUG_VARIABLE, LOG_INFO_FMT_HELPER (__VA_ARGS__, "")))

#define LOG_VANILLA_TRACE(...) \
  DEBUG ((DEBUG_VERBOSE | DEBUG_VARIABLE, __VA_ARGS__))

#define LOG_TRACE(...) \
  DEBUG ((DEBUG_VERBOSE | DEBUG_VARIABLE, LOG_TRACE_FMT_HELPER (__FUNCTION__, __VA_ARGS__, "")))

#define LOG_ERROR(...) \
  DEBUG ((DEBUG_ERROR, LOG_ERROR_FMT_HELPER (__VA_ARGS__, __FUNCTION__, __FILE__, __LINE__)))

/*
Adjustments for the inclusion of  UEFIVarServices.h as is.
*/
#if !defined(_Field_size_bytes_)
# define _Field_size_bytes_(count)
#endif

typedef UINT8  BYTE;
typedef CHAR16 WCHAR;
typedef CHAR8 CHAR;

#include "UEFIVarServices.h"

/*
NTSTATUS codes from the Auth. Var. TA tha we need to specially handle.
*/
#define STATUS_BUFFER_TOO_SMALL          (0xC0000023L)
#define STATUS_NOT_FOUND                 (0xC0000225L)

/*
Macro to turn on memory leak check on exit boot services.
*/
//#define MEMORY_LEAK_CHECK_ON_EXIT_BOOT

/*
Define a single union of all the outbound parameter structures.
Some are variable sized and thus overlay a single allocated buffer.
*/
typedef union _VARIABLE_PARAM {

  VARIABLE_GET_PARAM       GetParam;
  VARIABLE_GET_NEXT_PARAM  GetNextParam;
  VARIABLE_SET_PARAM       SetParam;
  VARIABLE_QUERY_PARAM     QueryParam;

} VARIABLE_PARAM, *PVARIABLE_PARAM;


/*
Define a single union of all the inbound parameter structures.
Some are variable sized and thus overlay a single allocated buffer.
*/
typedef union _VARIABLE_RESULT {

  VARIABLE_GET_RESULT       GetResult;
  VARIABLE_GET_NEXT_RESULT  GetNextResult;
  //  No VARIABLE_SET_RESULT since no data is returned
  VARIABLE_QUERY_RESULT     QueryResult;

} VARIABLE_RESULT, *PVARIABLE_RESULT;

static EFI_HANDLE mImageHandle = NULL;

GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8 *mOperationStr[] = {
  "",
  "",
  "VSGetOp",
  "VSGetNextVarOp",
  "VSSetOp",
  "VSQueryInfoOp",
  "VSSignalExitBootServicesOp",
};

VOID
EFIAPI
SecureBootHook(
  IN CHAR16                                 *VariableName,
  IN EFI_GUID                               *VendorGuid
);

EFI_HANDLE  mHandle = NULL;

/*
OpTEE GP Client API state for the Auth Var TA communications.
*/

TEEC_Context  mTeecContext;
TEEC_Session  mTeecSession;
BOOLEAN       mTeecSessionOpened;

/*
OpTEE Auth. Var. TA transport buffers.
These must be allocated using the OpTEE client API set.
*/

TEEC_SharedMemory  mVariableParamMem;
TEEC_SharedMemory  mVariableResultMem;

GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8 *mSetupModeNames[] = {
  "UserMode",
  "SetupMode"
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8 *mCustomModeNames[] = {
  "Standard",
  "Custom"
};

/**
Populate a GP API parameter set and invoke a command on the Auth. Var. TA.
@param[in]      Command            The command to invoke.
@param[in]      VariableParamSize  Size of the parameter buffer.
@param[in]      VariableParam      GP buffer describing the variable parameters.
@param[in]      VariableResultSize Size of the result buffer.
@param[in]      VariableResult     GP buffer describing the variable results.
@param[out]     ResultSize         The actual size of the data in the result buffer.
@param[out]     AuthVarStatus      The actual OP-TEE status result of the executed command.
@retval EFI_SUCCESS                Found the specified variable.
@retval EFI_NOT_FOUND              Variable not found.
@retval EFI_BUFFER_TO_SMALL        DataSize is too small for the result.
**/
EFI_STATUS
EFIAPI
OpteeRuntimeVariableInvokeCommand(
  IN VARIABLE_SERVICE_OPS  Command,
  IN UINT32                VariableParamSize,
  IN TEEC_SharedMemory     *VariableParam,
  IN UINT32                VariableResultSize,
  IN TEEC_SharedMemory     *VariableResult,
  OUT UINT32               *ResultSize,
  OUT UINT32               *AuthVarStatus
)
{
  EFI_STATUS Status = EFI_SUCCESS;
  TEEC_Operation TeecOperation = { 0 };
  TEEC_Result TeecResult = TEEC_SUCCESS;
  uint32_t ErrorOrigin = 0;
  uint32_t VariableParamType = TEEC_NONE;
  uint32_t ResultParamType = TEEC_NONE;

  if (VariableParam == NULL) {
    VariableParam = &mVariableParamMem;
    VariableParamSize = sizeof(UINT32);
  }

  TeecOperation.params[0].tmpref.buffer = VariableParam->buffer;
  TeecOperation.params[0].tmpref.size = VariableParamSize;
  VariableParamType = TEEC_MEMREF_TEMP_INPUT;

  if (VariableResult == NULL) {
    VariableResult = &mVariableResultMem;
    VariableResultSize = sizeof(UINT32);
  }

  TeecOperation.params[1].tmpref.buffer = VariableResult->buffer;
  TeecOperation.params[1].tmpref.size = VariableResultSize;
  ResultParamType = TEEC_MEMREF_TEMP_OUTPUT;

  TeecOperation.paramTypes = TEEC_PARAM_TYPES(
    VariableParamType,
    ResultParamType,
    TEEC_VALUE_OUTPUT,
    TEEC_NONE);

  TeecResult = TEEC_InvokeCommand(
    &mTeecSession,
    Command,
    &TeecOperation,
    &ErrorOrigin);

  *ResultSize = TeecOperation.params[2].value.a;
  *AuthVarStatus = TeecOperation.params[2].value.b;

  if (TeecResult != TEEC_SUCCESS) {
    switch (TeecResult) {
    case TEEC_ERROR_ITEM_NOT_FOUND:
      LOG_TRACE("TEEC not found");
      Status = EFI_NOT_FOUND;
      break;

    case TEEC_ERROR_SHORT_BUFFER:
      LOG_TRACE("TEEC short buffer");
      Status = EFI_BUFFER_TOO_SMALL;
      break;

    case TEEC_ERROR_OUT_OF_MEMORY:
      LOG_TRACE("TEEC out of memory");
      Status = EFI_OUT_OF_RESOURCES;
      break;

    case TEEC_ERROR_ACCESS_DENIED:
      LOG_TRACE("TEEC acccess denied");
      Status = EFI_SECURITY_VIOLATION;
      break;

    default:
      LOG_ERROR(
        "TEEC_InvokeCommand() failed. (TeecResult=0x%X, ErrorOrigin=%d)",
        TeecResult,
        ErrorOrigin);
      Status = EFI_PROTOCOL_ERROR;
    }

    goto Exit;
  }

Exit:
  return Status;
}


/**
This code finds variable in storage blocks (Volatile or Non-Volatile).
Caution: This function may receive untrusted input.
The data size is external input, so this function will validate it carefully
to avoid buffer overflow.
@param[in]      VariableName       Name of Variable to be found.
@param[in]      VendorGuid         Variable vendor GUID.
@param[out]     Attributes         Attribute value of the variable found.
@param[in, out] DataSize           Size of Data found. If size is less than the
data, this value contains the required size.
@param[out]     Data               Data pointer.
@retval EFI_INVALID_PARAMETER      Invalid parameter.
@retval EFI_SUCCESS                Find the specified variable.
@retval EFI_NOT_FOUND              Not found.
@retval EFI_BUFFER_TOO_SMALL       The supplied buffer cannot hold the result data.
**/
EFI_STATUS
EFIAPI
OpteeRuntimeGetVariable(
  IN      CHAR16                            *VariableName,
  IN      EFI_GUID                          *VendorGuid,
  OUT     UINT32                            *Attributes OPTIONAL,
  IN OUT  UINTN                             *DataSize,
  OUT     VOID                              *Data
)
{
  //PERF_START(mImageHandle, GET_VAR_TOK, NULL, 0);

  EFI_STATUS Status = EFI_SUCCESS;
  UINT32 LocalVariableNameSize;
  UINT32 LocalDataSize;

  if (EfiAtRuntime()) {
    Status = EFI_NOT_FOUND;
    goto Exit;
  }

  if (VariableName == NULL || VendorGuid == NULL || DataSize == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  if ((*DataSize != 0) && (Data == NULL)) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  LOG_TRACE(
    "%g\\%s (DataSize=0x%p)",
    VendorGuid,
    VariableName,
    (DataSize != NULL ? *DataSize : 0));

  //
  // Test hook to allow the test to acquire the session ID used with the TA.
  // The test DXE needs to close & open the session as a way to simulate the boot
  // code path in the TA that reconstructs the variable state from the
  // actual storage
  //
  if ((VariableName[0] == 1) && (VendorGuid->Data1 == 1) && (*DataSize == sizeof(UINTN))) {

    UINTN *SessionId = (UINTN *)Data;
    *SessionId = mTeecSession.session_id;
    LOG_TRACE("Test Hook for Session ID");
    Status = EFI_SUCCESS;
    goto Exit;
  }

  if (!mTeecSessionOpened) {
    LOG_ERROR("No active session with the TA");
    Status = EFI_NOT_READY;
    goto Exit;
  }

  if ((mVariableParamMem.buffer == NULL) ||
    (mVariableResultMem.buffer == NULL)) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Security: make sure external pointer values are copied locally to prevent
  // concurrent modification after validation.
  //
  LocalVariableNameSize = StrSize(VariableName);
  LocalDataSize = *DataSize;

  LOG_TRACE("LocalDataSize is 0x%x", LocalDataSize);

  {
    PVARIABLE_PARAM  VariableParam = (PVARIABLE_PARAM)mVariableParamMem.buffer;
    PVARIABLE_RESULT VariableResult = (PVARIABLE_RESULT)mVariableResultMem.buffer;
    UINT32 VariableParamSize;
    UINT32 VariableResultSize;
    UINT32 ResultSize = 0;
    UINT32 AuthvarStatus = 0;

    //
    // Validate that the buffers will fit.
    // The first test validates that the variable fields are sensible
    // to start with and catches the case that they are some huge number
    // that will overflow when added to.
    //
    if ((LocalVariableNameSize >= mVariableParamMem.size) ||
      (LocalDataSize >= mVariableResultMem.size)) {

      Status = EFI_BAD_BUFFER_SIZE;
      goto Exit;
    }

    //
    // A note about sizes. The correct calculations should be of the form:
    //   OFFSET_OF(VARIABLE_GET_PARAM, VariableName)
    //   OFFSET_OF(VARIABLE_GET_RESULT, Data)
    // However the Auth. Var. TA is coded based on sizeof() that is thus
    // including the first byte of the variable field and it's alignment.
    // The net effect is that the size calculation is too long by 4 bytes.
    // This doesn't matter in practice since the extra 4 bytes is not used.
    //
    VariableParamSize = sizeof(VARIABLE_GET_PARAM) +
      LocalVariableNameSize;

    VariableResultSize = sizeof(VARIABLE_GET_RESULT) +
      LocalDataSize;

    //
    // The second test validates that the data will actually fit in the buffer.
    //
    if ((VariableParamSize > mVariableParamMem.size) ||
      (VariableResultSize > mVariableResultMem.size)) {

      Status = EFI_BAD_BUFFER_SIZE;
      goto Exit;
    }

    //
    // Clear the buffers entirely to start with.
    //
    ZeroMem(mVariableParamMem.buffer, mVariableParamMem.size);
    ZeroMem(mVariableResultMem.buffer, mVariableResultMem.size);

    //
    // Fill in the parameter fields.
    //
    VariableParam->GetParam.Size = sizeof(VARIABLE_GET_PARAM);
    VariableParam->GetParam.VariableNameSize = LocalVariableNameSize;

    CopyMem(&VariableParam->GetParam.VendorGuid, VendorGuid, sizeof(VariableParam->GetParam.VendorGuid));
    CopyMem(&VariableParam->GetParam.VariableName, VariableName, LocalVariableNameSize);
    LOG_TRACE("Get Variable: '%S'", VariableName);
    Status = OpteeRuntimeVariableInvokeCommand(
      VSGetOp,
      VariableParamSize,
      &mVariableParamMem,
      VariableResultSize,
      &mVariableResultMem,
      &ResultSize,
      &AuthvarStatus);

    if (Status == EFI_BUFFER_TOO_SMALL) {
      LOG_TRACE("Get Variable: Buffer too small");

      //
      // In this case the Auth. Var. TA returns the size of the *buffer* required
      // so in order to turn this into the size of the *data* required we need
      // to subtract header.
      //
      ASSERT(ResultSize >= sizeof(VARIABLE_GET_RESULT));
      *DataSize = ResultSize - sizeof(VARIABLE_GET_RESULT);

    } else if (Status == EFI_SUCCESS) {
      LOG_TRACE("Get Variable Success");
      *DataSize = VariableResult->GetResult.DataSize;
    } else {
      if (Status == EFI_NOT_FOUND) {
        LOG_TRACE("Get: Not found");
      } else {
        LOG_ERROR("GET VARIABLE FAIL '%S': 0x%x (OP-TEE Status:0x%x)", VariableName, Status, AuthvarStatus);
      }
    }

    //
    // This is the behvaiour that was defined in the SMM implementation where on
    // any error the attributes are still sent back.
    //
    if (Attributes != NULL) {
      *Attributes = VariableResult->GetResult.Attributes;
    }

    if (EFI_ERROR(Status)) {
      goto Exit;
    }

    //
    // Shouldn't happen, but...
    //
    if (VariableResult->GetResult.DataSize > LocalDataSize) {
      LOG_ERROR(
        "### Data buffer overflow, result DataSize=0x%X ###",
        VariableResult->GetResult.DataSize);
      LOG_ERROR("Local data is 0x%x bytes", LocalDataSize);

      Status = EFI_BAD_BUFFER_SIZE;
      goto Exit;
    }

    CopyMem(Data, VariableResult->GetResult.Data, VariableResult->GetResult.DataSize);

  }

Exit:

  //PERF_END(mImageHandle, GET_VAR_TOK, NULL, 0);
  return Status;
}


/**
This code Finds the Next available variable.
@param[in, out] VariableNameSize   Size of the variable name.
@param[in, out] VariableName       Pointer to variable name.
@param[in, out] VendorGuid         Variable Vendor Guid.
@retval EFI_INVALID_PARAMETER      Invalid parameter.
@retval EFI_SUCCESS                Find the specified variable.
@retval EFI_NOT_FOUND              Not found.
@retval EFI_BUFFER_TO_SMALL        DataSize is too small for the result.
**/
EFI_STATUS
EFIAPI
OpteeRuntimeGetNextVariableName(
  IN OUT  UINTN                             *VariableNameSize,
  IN OUT  CHAR16                            *VariableName,
  IN OUT  EFI_GUID                          *VendorGuid
)
{
  //PERF_START(mImageHandle, GET_NEXT_VAR_NAME_TOK, NULL, 0);

  EFI_STATUS Status = EFI_SUCCESS;
  UINT32 LocalInVariableNameSize;
  UINT32 LocalOutVariableNameSize;

  if (EfiAtRuntime()) {
    Status = EFI_UNSUPPORTED;
    goto Exit;
  }

  if (VariableNameSize == NULL || VariableName == NULL || VendorGuid == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  if (!mTeecSessionOpened) {
    Status = EFI_NOT_READY;
    goto Exit;
  }

  if ((mVariableParamMem.buffer == NULL) ||
    (mVariableResultMem.buffer == NULL)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  //
  // Security: make sure external pointer values are copied locally to prevent
  // concurrent modification after validation.
  //
  // The Auth. Var. TA expects that input name size == 0 as the indicator that this
  // is the initial query for the first name that exists. Since the StrSize above
  // includes the NULL terminator we need to handle this case specially.
  //
  if (*VariableName == 0) {
    LocalInVariableNameSize = 0;
  }
  else {
    LocalInVariableNameSize = StrSize(VariableName);
  }

  LocalOutVariableNameSize = *VariableNameSize;

  {
    PVARIABLE_PARAM  VariableParam = (PVARIABLE_PARAM)mVariableParamMem.buffer;
    PVARIABLE_RESULT VariableResult = (PVARIABLE_RESULT)mVariableResultMem.buffer;
    UINT32 VariableParamSize;
    UINT32 VariableResultSize;
    UINT32 ResultSize = 0;
    UINT32 AuthvarStatus = 0;

    //
    // Validate that the buffers will fit.
    // The first test validates that the variable fields are sensible
    // to start with and catches the case that they are some huge number
    // that will overflow when added to.
    //
    if ((LocalInVariableNameSize >= mVariableParamMem.size) ||
      (LocalOutVariableNameSize >= mVariableResultMem.size)) {

      Status = EFI_BAD_BUFFER_SIZE;
      goto Exit;
    }

    //
    // A note about sizes. The correct calculations should be of the form:
    //   OFFSET_OF(VARIABLE_GET_NEXT_PARAM,  VariableName)
    //   OFFSET_OF(VARIABLE_GET_NEXT_RESULT, VariableName)
    // However the Auth. Var. TA is coded based on sizeof() that is thus
    // including the first byte of the variable field and it's alignment.
    // The net effect is that the size calculation is too long by 4 bytes.
    // This doesn't matter in practice since the extra 4 bytes is not used.
    //
    VariableParamSize = sizeof(VARIABLE_GET_NEXT_PARAM) +
      LocalInVariableNameSize;

    VariableResultSize = sizeof(VARIABLE_GET_NEXT_RESULT) +
      LocalOutVariableNameSize;

    //
    // The second test validates that the data will actually fit in the buffer.
    //
    if ((VariableParamSize > mVariableParamMem.size) ||
      (VariableResultSize > mVariableResultMem.size)) {

      Status = EFI_BAD_BUFFER_SIZE;
      goto Exit;
    }

    //
    // Clear the buffers entirely to start with.
    //
    SetMem(mVariableParamMem.buffer, mVariableParamMem.size, 0);
    SetMem(mVariableResultMem.buffer, mVariableResultMem.size, 0);

    //
    // Fill in the parameter fields.
    //
    VariableParam->GetNextParam.Size = sizeof(VARIABLE_GET_NEXT_PARAM);
    VariableParam->GetNextParam.VariableNameSize = LocalInVariableNameSize;

    CopyMem(&VariableParam->GetNextParam.VendorGuid, VendorGuid, sizeof(VariableParam->GetNextParam.VendorGuid));
    CopyMem(&VariableParam->GetNextParam.VariableName, VariableName, LocalInVariableNameSize);
    LOG_TRACE("Get Next Variable: '%S'", VariableName);
    Status = OpteeRuntimeVariableInvokeCommand(VSGetNextVarOp,
      VariableParamSize,
      &mVariableParamMem,
      VariableResultSize,
      &mVariableResultMem,
      &ResultSize,
      &AuthvarStatus);

    if (Status == EFI_BUFFER_TOO_SMALL) {
      LOG_TRACE("Get Next Variable: Buffer too small");

      //
      // In this case the Auth. Var. TA returns the size of the *buffer* required
      // so in order to turn this into the size of the *data* required we need
      // to subtract header.
      //
      ASSERT(ResultSize >= sizeof(VARIABLE_GET_NEXT_RESULT));
      *VariableNameSize = ResultSize - sizeof(VARIABLE_GET_NEXT_RESULT);

    } else if (Status == EFI_SUCCESS) {
      LOG_TRACE("Get Next Variable Success");
      *VariableNameSize = VariableResult->GetNextResult.VariableNameSize;
    } else {
      if (Status == EFI_NOT_FOUND) {
        LOG_TRACE("Get next: Not found");
      } else {
        LOG_ERROR("GET NEXT VARIABLE FAIL '%S': 0x%x (OP-TEE Status:0x%x)", VariableName, Status, AuthvarStatus);
      }
    }

    if (EFI_ERROR(Status)) {
      goto Exit;
    }

    //
    // Shouldn't happen, but...
    //
    if (VariableResult->GetNextResult.VariableNameSize > LocalOutVariableNameSize) {

      DEBUG((DEBUG_VARIABLE, "Name buffer overflow, result VariableNameSize=0x%X\n",
        VariableResult->GetNextResult.VariableNameSize));

      Status = EFI_BAD_BUFFER_SIZE;
      goto Exit;
    }

    CopyMem(VendorGuid, &VariableResult->GetNextResult.VendorGuid, sizeof(VariableResult->GetNextResult.VendorGuid));
    CopyMem(VariableName, &VariableResult->GetNextResult.VariableName, VariableResult->GetNextResult.VariableNameSize);
  }

Exit:

  //PERF_END(mImageHandle, GET_NEXT_VAR_NAME_TOK, NULL, 0);

  return Status;
}

/**
This code sets variable in storage blocks (Volatile or Non-Volatile).
Caution: This function may receive untrusted input.
The data size and data are external input, so this function will validate it carefully to avoid buffer overflow.
@param[in] VariableName                 Name of Variable to be found.
@param[in] VendorGuid                   Variable vendor GUID.
@param[in] Attributes                   Attribute value of the variable found
@param[in] DataSize                     Size of Data found. If size is less than the
                                        data, this value contains the required size.
@param[in] Data                         Data pointer.
@retval EFI_INVALID_PARAMETER           Invalid parameter.
@retval EFI_SUCCESS                     Set successfully.
@retval EFI_OUT_OF_RESOURCES            Resource not enough to set variable.
@retval EFI_NOT_FOUND                   Not found.
@retval EFI_WRITE_PROTECTED             Variable is read-only.
**/
EFI_STATUS
EFIAPI
OpteeRuntimeSetVariable(
  IN CHAR16     *VariableName,
  IN EFI_GUID   *VendorGuid,
  IN UINT32     Attributes,
  IN UINTN      DataSize,
  IN VOID       *Data
)
{
  //PERF_START(mImageHandle, SET_VAR_TOK, NULL, 0);

  EFI_STATUS Status = EFI_SUCCESS;
  UINTN VariableNameSize;
  UINTN VariableParamSize;

  if (EfiAtRuntime()) {
    Status = EFI_UNSUPPORTED;
    goto Exit;
  }

  if (VariableName == NULL || VariableName[0] == L'\0' || VendorGuid == NULL) {
    LOG_ERROR("Var name invalid");
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  if (DataSize != 0 && Data == NULL) {
    LOG_ERROR("data invalid");
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  if (!mTeecSessionOpened) {
    LOG_ERROR("not ready");
    Status = EFI_NOT_READY;
    goto Exit;
  }

  LOG_TRACE("%g\\%s (DataSize=0x%p)", VendorGuid, VariableName, DataSize);

  //
  // Test hook to allow the test to set the session ID used with the TA.
  // The test DXE needs to close & open the session as a way to simulate the boot
  // code path in the TA that reconstructs the variable state from the
  // actual storage
  //f
  if ((VariableName[0] == 1) && (VendorGuid->Data1 == 1) && (DataSize == sizeof(UINTN))) {

    UINTN *SessionId = (UINTN *)Data;
    mTeecSession.session_id = *SessionId;
    LOG_INFO("Test Hook for Injecting Session ID");
    return EFI_SUCCESS;
  }

  //
  // Security: make sure external pointer values are copied locally to prevent
  // concurrent modification after validation.
  //
  VariableNameSize = StrSize(VariableName);

  {
    PVARIABLE_PARAM  VariableParam = (PVARIABLE_PARAM)mVariableParamMem.buffer;
    UINT32 ResultSize = 0;
    UINT32 AuthvarStatus = 0;

    //
    // Validate that the buffers will fit.
    // The first test validates that the variable fields are sensible
    // to start with and catches the case that they are some huge number
    // that will overflow when added to.
    //
    if ((VariableNameSize >= mVariableParamMem.size) ||
      (DataSize >= mVariableParamMem.size)) {

      Status = EFI_BAD_BUFFER_SIZE;
      goto Exit;
    }

    VariableParamSize = sizeof(VARIABLE_SET_PARAM) +
      VariableNameSize +
      DataSize;

    //
    // The second test validates that the data will actually fit in the buffer.
    //
    if (VariableParamSize > mVariableParamMem.size) {
      Status = EFI_BAD_BUFFER_SIZE;
      goto Exit;
    }

    //
    // Clear the buffers entirely to start with.
    //
    ZeroMem(mVariableParamMem.buffer, mVariableParamMem.size);
    ZeroMem(mVariableResultMem.buffer, mVariableResultMem.size);

    //
    // Fill in the parameter fields.
    //
    VariableParam->SetParam.Size = sizeof(VARIABLE_SET_PARAM);
    VariableParam->SetParam.VariableNameSize = VariableNameSize;
    VariableParam->SetParam.Attributes = Attributes;
    VariableParam->SetParam.DataSize = DataSize;
    VariableParam->SetParam.OffsetVariableName = 0;
    VariableParam->SetParam.OffsetData = VariableNameSize;

    CopyMem(&VariableParam->SetParam.VendorGuid, VendorGuid, sizeof(VariableParam->SetParam.VendorGuid));
    CopyMem(&VariableParam->SetParam.Payload[0], VariableName, VariableNameSize);
    CopyMem(&VariableParam->SetParam.Payload[VariableNameSize], Data, DataSize);
    LOG_TRACE("Set Variable: '%S', Attribs 0x%x", VariableName, Attributes);
    Status = OpteeRuntimeVariableInvokeCommand(
      VSSetOp,
      VariableParamSize,
      &mVariableParamMem,
      0,
      NULL,
      &ResultSize,
      &AuthvarStatus);

    if (EFI_ERROR(Status)) {
      if (Status == EFI_NOT_FOUND && !Attributes) {
        LOG_TRACE("Set - Delete : Not found");
      } else {
        LOG_ERROR("SET VARIABLE FAIL '%S': 0x%x (OP-TEE Status:0x%x)", VariableName, Status, AuthvarStatus);
      }
      goto Exit;
    } else {
      LOG_TRACE("Set Variable Success");
    }
  }

Exit:
  if (!EFI_ERROR(Status)) {
    SecureBootHook(VariableName, VendorGuid);
  }

  //PERF_END(mImageHandle, SET_VAR_TOK, NULL, 0);

  return Status;
}


/**
This code returns information about the EFI variables.
@param[in]  Attributes                    Attributes bitmask to specify the type of variables
                                          on which to return information.
@param[out] MaximumVariableStorageSize    Pointer to the maximum size of the storage space available
                                          for the EFI variables associated with the attributes specified.
@param[out] RemainingVariableStorageSize  Pointer to the remaining size of the storage space available
                                          for EFI variables associated with the attributes specified.
@param[out] MaximumVariableSize           Pointer to the maximum size of an individual EFI variables
                                          associated with the attributes specified.
@retval EFI_INVALID_PARAMETER             An invalid combination of attribute bits was supplied.
@retval EFI_SUCCESS                       Query successfully.
@retval EFI_UNSUPPORTED                   The attribute is not supported on this platform.
**/
EFI_STATUS
EFIAPI
OpteeRuntimeQueryVariableInfo(
  IN  UINT32                                Attributes,
  OUT UINT64                                *MaximumVariableStorageSize,
  OUT UINT64                                *RemainingVariableStorageSize,
  OUT UINT64                                *MaximumVariableSize
)
{
  //PERF_START(mImageHandle, QUERY_VAR_INFO_TOK, NULL, 0);

  EFI_STATUS Status = EFI_SUCCESS;

  if (EfiAtRuntime()) {
    Status = EFI_UNSUPPORTED;
    goto Exit;
  }

  if (MaximumVariableStorageSize == NULL || RemainingVariableStorageSize == NULL || MaximumVariableSize == NULL || Attributes == 0) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  if (!mTeecSessionOpened) {
    Status = EFI_NOT_READY;
    goto Exit;
  }

  if ((mVariableParamMem.buffer == NULL) ||
    (mVariableResultMem.buffer == NULL)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  {
    PVARIABLE_PARAM  VariableParam = (PVARIABLE_PARAM)mVariableParamMem.buffer;
    PVARIABLE_RESULT VariableResult = (PVARIABLE_RESULT)mVariableResultMem.buffer;
    UINT32 VariableParamSize;
    UINT32 VariableResultSize;
    UINT32 ResultSize = 0;
    UINT32 AuthvarStatus = 0;

    VariableParamSize = sizeof(VARIABLE_QUERY_PARAM);
    VariableResultSize = sizeof(VARIABLE_QUERY_RESULT);

    //
    // Validates that the data will actually fit in the buffer.
    //
    if ((VariableParamSize > mVariableParamMem.size) ||
      (VariableResultSize > mVariableResultMem.size)) {

      Status = EFI_BAD_BUFFER_SIZE;
      goto Exit;
    }

    //
    // Clear the buffers entirely to start with.
    //
    SetMem(mVariableParamMem.buffer, mVariableParamMem.size, 0);
    SetMem(mVariableResultMem.buffer, mVariableResultMem.size, 0);

    //
    // Fill in the parameter fields.
    //
    VariableParam->QueryParam.Size = sizeof(VARIABLE_QUERY_PARAM);
    VariableParam->QueryParam.Attributes = Attributes;
    LOG_TRACE("Query Variable");
    Status = OpteeRuntimeVariableInvokeCommand(VSQueryInfoOp,
      VariableParamSize,
      &mVariableParamMem,
      VariableResultSize,
      &mVariableResultMem,
      &ResultSize,
      &AuthvarStatus);

    if (EFI_ERROR(Status)) {
      LOG_ERROR("QUERY VARIABLE FAIL (Attribs 0x%x): 0x%x (OP-TEE Status:0x%x)", Attributes, Status, AuthvarStatus);
      goto Exit;
    } else {
      LOG_TRACE("Query Variable Success");
    }

    *MaximumVariableStorageSize = VariableResult->QueryResult.MaximumVariableStorageSize;
    *RemainingVariableStorageSize = VariableResult->QueryResult.RemainingVariableStorageSize;
    *MaximumVariableSize = VariableResult->QueryResult.MaximumVariableSize;
  }



Exit:

  //PERF_END(mImageHandle, QUERY_VAR_INFO_TOK, NULL, 0);

  return Status;
}


/**
Opens a session to the AuthVar TA that will be closed on exit boot services.
Allocatest the shared memory that will be used for parameter passing to the
TA.
**/
EFI_STATUS
EFIAPI
TaOpenSession(
  VOID
)
{
  TEEC_Result  TeecResult;
  uint32_t     ErrorOrigin;
  TEEC_UUID    TeecUuid;
  EFI_STATUS   Status = EFI_SUCCESS;

  CopyMem(&TeecUuid, &gOpteeAuthVarTaGuid, sizeof(TeecUuid));

  TeecUuid.timeLow = SwapBytes32(TeecUuid.timeLow);
  TeecUuid.timeMid = SwapBytes16(TeecUuid.timeMid);
  TeecUuid.timeHiAndVersion = SwapBytes16(TeecUuid.timeHiAndVersion);

  //
  // Open a session to the Auth Var TA
  //
  TeecResult = TEEC_OpenSession(
    &mTeecContext,
    &mTeecSession,
    &TeecUuid,
    TEEC_LOGIN_PUBLIC,
    NULL,
    NULL,
    &ErrorOrigin);

  if (TeecResult != TEEC_SUCCESS) {
    LOG_ERROR("TEEC_OpenSession() failed. (TeecResult=0x%X)", TeecResult);
    Status = EFI_PROTOCOL_ERROR;
    goto Exit;
  }

  mTeecSessionOpened = TRUE;

  //
  // Allocate the transport buffers. We don't have to worry about alignment because the
  // OpTEE Client API will align the buffers for us.
  //
  mVariableParamMem.size = MAX(
    PcdGet32(PcdMaxVariableSize),
    PcdGet32(PcdMaxHardwareErrorVariableSize)) +
    sizeof(VARIABLE_PARAM);

  TeecResult = TEEC_AllocateSharedMemory(&mTeecContext, &mVariableParamMem);
  if (TeecResult != TEEC_SUCCESS) {
    LOG_ERROR("TEEC_AllocateSharedMemory failed. (TeecResult=0x%X)", TeecResult);
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  mVariableResultMem.size = mVariableParamMem.size;

  TeecResult = TEEC_AllocateSharedMemory(&mTeecContext, &mVariableResultMem);
  if (TeecResult != TEEC_SUCCESS) {
    LOG_ERROR("TEEC_AllocateSharedMemory failed. (TeecResult=0x%X)", TeecResult);
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

Exit:
  return Status;
}

/**
Initializes the SecureBoot and SetupMode variables.
This needs to be done before the variable services protocol is
installed such that when the TrEE DXE runs its measurement code
it will find the variable.
**/
EFI_STATUS
InitSecureBootVariables(
  VOID
)
{
  UINT8 Data;
  UINTN DataSize;
  BOOLEAN PkPresent = FALSE;
  UINT8 SetupMode;
  UINT8 SecureBootMode;
  UINT8 SecureBootEnable;
  EFI_STATUS Status;

  LOG_INFO(
    "Initializing SecureBoot Variables (PcdSecureBootEnable=%d)",
    PcdGetBool(PcdSecureBootEnable));

  //
  // If the PK exits then
  //

  DataSize = sizeof(Data);
  Status = OpteeRuntimeGetVariable(
    EFI_PLATFORM_KEY_NAME,
    &gEfiGlobalVariableGuid,
    NULL,
    &DataSize,
    &Data);

  if (Status == EFI_BUFFER_TOO_SMALL) {
    PkPresent = TRUE;
  }

  //
  // Create "SetupMode" variable with BS+RT attribute set.
  //

  if (PkPresent == FALSE) {
    LOG_INFO("SecureBoot PK is not present. SecureBoot is in SetupMode");
    SetupMode = SETUP_MODE;
  }
  else {
    SetupMode = USER_MODE;
    LOG_INFO("SecureBoot PK is present. SecureBoot is in UserMode");
  }

  Status = OpteeRuntimeSetVariable(
    EFI_SETUP_MODE_NAME,
    &gEfiGlobalVariableGuid,
    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
    sizeof(SetupMode),
    &SetupMode);

  if (EFI_ERROR(Status)) {
    LOG_ERROR(
      "OpteeRuntimeSetVariable(%g\\%s) failed. (Status=%r)",
      &gEfiGlobalVariableGuid,
      EFI_SETUP_MODE_NAME,
      Status);

    goto Exit;
  }

  //
  // If "SecureBootEnable" variable exists, then update "SecureBoot" variable.
  // If "SecureBootEnable" variable is SECURE_BOOT_ENABLE and in USER_MODE,
  //   Set "SecureBoot" variable to SECURE_BOOT_MODE_ENABLE.
  // If "SecureBootEnable" variable is SECURE_BOOT_DISABLE,
  //   Set "SecureBoot" variable to SECURE_BOOT_MODE_DISABLE.
  //
  SecureBootEnable = SECURE_BOOT_DISABLE;

  DataSize = sizeof(SecureBootEnable);
  Status = OpteeRuntimeGetVariable(
    EFI_SECURE_BOOT_ENABLE_NAME,
    &gEfiSecureBootEnableDisableGuid,
    NULL,
    &DataSize,
    &Data);

  if (!EFI_ERROR(Status)) {
    if (SetupMode == USER_MODE) {
      SecureBootEnable = Data;
    }
  }
  else if (SetupMode == USER_MODE) {

    //
    // "SecureBootEnable" not exist, initialize it in USER_MODE.
    //
    SecureBootEnable = SECURE_BOOT_DISABLE;
    if (PcdGetBool(PcdSecureBootEnable)) {
      SecureBootEnable = SECURE_BOOT_ENABLE;
    }

    Status = OpteeRuntimeSetVariable(
      EFI_SECURE_BOOT_ENABLE_NAME,
      &gEfiSecureBootEnableDisableGuid,
      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
      sizeof(SecureBootEnable),
      &SecureBootEnable);

    if (EFI_ERROR(Status)) {
      LOG_ERROR(
        "OpteeRuntimeSetVariable(%g\\%s) failed. (Status=%r)",
        &gEfiSecureBootEnableDisableGuid,
        EFI_SECURE_BOOT_ENABLE_NAME,
        Status);

      goto Exit;
    }
  }

  if ((SecureBootEnable == SECURE_BOOT_ENABLE) && (SetupMode == USER_MODE)) {
    SecureBootMode = SECURE_BOOT_MODE_ENABLE;
  }
  else {
    SecureBootMode = SECURE_BOOT_MODE_DISABLE;
  }

  //
  // Create "SecureBoot" variable with BS+RT attribute set.
  //
  Status = OpteeRuntimeSetVariable(
    EFI_SECURE_BOOT_MODE_NAME,
    &gEfiGlobalVariableGuid,
    EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
    sizeof(SecureBootMode),
    &SecureBootMode);

  if (EFI_ERROR(Status)) {
    LOG_ERROR(
      "OpteeRuntimeSetVariable(%g\\%s) failed. (Status=%r)",
      &gEfiGlobalVariableGuid,
      EFI_SECURE_BOOT_MODE_NAME,
      Status);

    goto Exit;
  }

  //
  // Initialize "CustomMode" in STANDARD_SECURE_BOOT_MODE state.
  //

  Data = STANDARD_SECURE_BOOT_MODE;
  Status = OpteeRuntimeSetVariable(
    EFI_CUSTOM_MODE_NAME,
    &gEfiCustomModeEnableGuid,
    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
    sizeof(Data),
    &Data);

  if (EFI_ERROR(Status)) {
    LOG_ERROR(
      "OpteeRuntimeSetVariable(%g\\%s) failed. (Status=%r)",
      &gEfiCustomModeEnableGuid,
      EFI_CUSTOM_MODE_NAME,
      Status);

    goto Exit;
  }

  LOG_INFO("SecureBoot Variables:");
  LOG_INFO("- %s = %a", EFI_SETUP_MODE_NAME, mSetupModeNames[SetupMode]);
  LOG_INFO("- %s = %x", EFI_SECURE_BOOT_ENABLE_NAME, (UINT32)SecureBootEnable);
  LOG_INFO("- %s = %x", EFI_SECURE_BOOT_MODE_NAME, (UINT32)SecureBootMode);
  LOG_INFO("- %s = %a", EFI_CUSTOM_MODE_NAME, mCustomModeNames[Data]);

Exit:
  return Status;
}

EFI_STATUS
OpteeRuntimeVariableWriteLogToDisk(
  IN CONST VAR_LOG_ENTRY  *LogBuffer,
  IN UINTN                LogCount
)
{
  EFI_STATUS Status = EFI_SUCCESS;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Fs = NULL;
  EFI_FILE_HANDLE RootVolume = NULL;
  EFI_FILE_PROTOCOL *File = NULL;
  EFI_HANDLE MediaHandle = NULL;
  EFI_DEVICE_PATH_PROTOCOL *DevicePath = NULL;
  EFI_DEVICE_PATH_PROTOCOL *IntermediateDevicePath = NULL;
  CONST UINTN StrBufferCharCount = 256;
  CHAR16 StrBuffer[StrBufferCharCount];
  UINTN FormattedCharCount;
  UINTN LogIndex;
  UINTN WriteByteCount;
  CONST CHAR16 *DevicePathText;

  if (LogCount == 0) {
    goto Exit;
  }

  DevicePathText = (CONST CHAR16 *) FixedPcdGetPtr(
    PcdStorageMediaPartitionDevicePath);

  if ((DevicePathText == NULL) || (*DevicePathText == L'\0')) {
    Status = EFI_INVALID_PARAMETER;
    LOG_ERROR("PcdStorageMediaPartition is unspecified");
    goto Exit;
  }

  IntermediateDevicePath = DevicePath;
  Status = gBS->LocateDevicePath(
    &gEfiSimpleFileSystemProtocolGuid,
    &IntermediateDevicePath,
    &MediaHandle);

  if (Status == EFI_NOT_FOUND) {
    Status = EFI_SUCCESS;
    LOG_INFO("%s FAT partition is not ready yet", DevicePathText);
    goto Exit;
  }

  if (EFI_ERROR(Status)) {
    LOG_ERROR("gBS->LocateDevicePath() failed. (Status=%r)", Status);
    goto Exit;
  }

  LOG_INFO(
    "Writing %u log entries to FAT partition %s",
    (UINT32)LogCount,
    DevicePathText);

  Status = gBS->OpenProtocol(
    MediaHandle,
    &gEfiSimpleFileSystemProtocolGuid,
    (VOID **)&Fs,
    mImageHandle,
    NULL,
    EFI_OPEN_PROTOCOL_GET_PROTOCOL);

  if (EFI_ERROR(Status)) {
    LOG_ERROR("gBS->OpenProtocol(gEfiSimpleFileSystemProtocolGuid) failed. (Status=%r)", Status);
    goto Exit;
  }

  Status = Fs->OpenVolume(Fs, &RootVolume);
  if (EFI_ERROR(Status)) {
    LOG_ERROR("Fs->OpenVolume() failed. (Status=%r)", Status);
    goto Exit;
  }
  ASSERT(RootVolume != NULL);

  Status = RootVolume->Open(
    RootVolume,
    &File,
    L"VarLog.txt",
    EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
    0);

  if (EFI_ERROR(Status)) {
    LOG_ERROR("RootVolume->Open() failed. (Status=%r)", Status);
    goto Exit;
  }

  Status = File->SetPosition(File, 0);
  if (EFI_ERROR(Status)) {
    LOG_ERROR("File->SetPosition(0) failed. (Status=%r)", Status);
    goto Exit;
  }

  for (LogIndex = 0; LogIndex < LogCount; ++LogIndex) {

    FormattedCharCount = UnicodeSPrint(
      StrBuffer,
      sizeof(StrBuffer),
      L"%u\t%u\t%a\t%s\t%d\t%u\t"
      L"%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t0x%08x\n",
      (UINT32)LogIndex,
      LogBuffer->StartTimeMs,
      mOperationStr[LogBuffer->Operation],
      (LogBuffer->StrParam[0] == L'\0') ? L"NA" : LogBuffer->StrParam,
      LogBuffer->IntParam,
      LogBuffer->DurationMs,
      (UINT32)LogBuffer->CacheRebuildCount,
      LogBuffer->CacheRebuildDurationMs,
      (UINT32)LogBuffer->IsNV,
      (UINT32)LogBuffer->IsBS,
      (UINT32)LogBuffer->IsTimeAuthWrite,
      (UINT32)LogBuffer->IsAuthWrite,
      (UINT32)LogBuffer->IsAppendWrite,
      (UINT32)LogBuffer->IsHwError,
      LogBuffer->Status);

    WriteByteCount = FormattedCharCount * sizeof(CHAR16);
    Status = File->Write(
      File,
      &WriteByteCount,
      StrBuffer);

    if (EFI_ERROR(Status)) {
      LOG_ERROR("File->Write() failed. (Status=%r)", Status);
      goto Exit;
    }
    ASSERT(WriteByteCount == FormattedCharCount * sizeof(CHAR16));

    LogBuffer++;
  }

  LOG_INFO("Opened media and log is written successfully");

Exit:

  if (File != NULL) {
    File->Flush(File);
    File->Close(File);
    File = NULL;
  }

  if (RootVolume != NULL) {
    RootVolume->Close(RootVolume);
    RootVolume = NULL;
  }

  if (MediaHandle != NULL) {
    EFI_STATUS exitStatus = gBS->CloseProtocol(
      MediaHandle,
      &gEfiSimpleFileSystemProtocolGuid,
      mImageHandle,
      NULL);

    if (EFI_ERROR(exitStatus)) {
      LOG_ERROR("gBS->CloseProtocol() failed. (Status=%r)", exitStatus);
    }

    Fs = NULL;
  }


  if (DevicePath != NULL) {
    FreePool(DevicePath);
  }

  return Status;
}

EFI_GUID AuthVarTestGuid = {0xDEADBEEF, 0x1234, 0x4321, {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}};
#define BUILD_TEST_BUFFER(name, number, size) UINT8 name[size];{UINT32 i = 0; for (i = 0; i < size; i++) {name[i] = number;} }
#define TEST_BUFFER_SIZE 100

CHAR8 BitsToChar(UINT8 bits) {
  if (bits < 0xA) {
    return '0' + bits;
  } else {
    return 'A' + (bits - 'A');
  }
}

VOID AuthVarHexDump(UINT8 *buffer, UINT32 size) {
  CHAR8 string[10000];
  CHAR8 *ptr = string;
  UINT32 i = 0;
  size = min(size, (sizeof(string) - 1) / 2);
  for(i = 0; i < size; i++) {
    UINT8 byteH = buffer[i] >> 4;
    UINT8 byteL = buffer[i] & 0xF;
    *(ptr++) = BitsToChar(byteH);
    *(ptr++) = BitsToChar(byteL);
  }
  *ptr = '\0';
  DEBUG((DEBUG_ERROR, "0x%p:", buffer));
  DEBUG((DEBUG_ERROR, string));
}

VOID AuthVarFixBuffer(void *buf, UINT32 size) {
  UINT32 i;
  CHAR16 *charBuf = (CHAR16 *)buf;
  size /= 2;
  for(i=0; i < size - 1; i++) {
    if (charBuf[i] == L'\0') {
      charBuf[i] = L'_';
    }
  }
}

/**
Exit Boot Services Event notification handler.
Notify OpTEE Auth. Var. TA about the event.
@param[in]  Event     Event whose notification function is being invoked.
@param[in]  Context   Pointer to the notification function's context.
**/
VOID
EFIAPI
OpteeRuntimeOnExitBootServices(
  IN  EFI_EVENT  Event,
  IN VOID       *Context
)
{
  EFI_STATUS Status = EFI_SUCCESS;
  UINT32 ResultSize = 0;
  UINT32 AuthvarStatus = 0;

  LOG_INFO("Notifying auth_var TA for ExitBootServices event");

  //
  // Plain signal with no parameters or results.
  //
  Status = OpteeRuntimeVariableInvokeCommand(
    VSSignalExitBootServicesOp,
    0,
    NULL,
    0,
    NULL,
    &ResultSize,
    &AuthvarStatus);

  if (Status != EFI_SUCCESS) {
    //
    // Reboot the system.
    //
    LOG_ERROR("Signalling exit boot service failed. (Status=%r)\nRebooting the system...", Status);
    gRT->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, L"Variable Exit Boot Services Notification Returned Error.");
  }

  //
  // Close session & destroy the context
  //
  if (mTeecSessionOpened) {
    TEEC_CloseSession(&mTeecSession);
    mTeecSessionOpened = FALSE;
  }

  TEEC_FinalizeContext(&mTeecContext);

  //
  // Release shared memories allocated for transport buffers.
  //
  TEEC_ReleaseSharedMemory(&mVariableParamMem);
  TEEC_ReleaseSharedMemory(&mVariableResultMem);

  OpteeClientApiFinalize();

  LOG_TRACE("Status=%r", Status);
}


/**
Variable Driver main entry point. The Variable driver places the 4 EFI
runtime services in the EFI System Table and installs arch protocols
for variable read and write services being available. It also registers
a notification function for an EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.
@param[in] ImageHandle    The firmware allocated handle for the EFI image.
@param[in] SystemTable    A pointer to the EFI System Table.
@retval EFI_SUCCESS       Variable service successfully initialized.
**/
EFI_STATUS
EFIAPI
VariableAuthOpteeRuntimeInitialize(
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
)
{
  EFI_STATUS   Status;
  TEEC_Result  TeecResult;
  EFI_EVENT    ExitBootServiceEvent;

  mImageHandle = ImageHandle;

  //
  // Initialize the library.
  //
  Status = OpteeClientApiInitialize(ImageHandle);
  if (EFI_ERROR(Status)) {
    LOG_ERROR("OpteeClientApiInitialize() failed. (Status=%r)", Status);
    goto Exit;
  }

  //
  // Setup the TEE context.
  //
  TeecResult = TEEC_InitializeContext(NULL, &mTeecContext);
  if (TeecResult != TEEC_SUCCESS) {
    LOG_ERROR("TEEC_InitializeContext() failed. (TeecResult=0x%x)", TeecResult);
    goto Exit;
  }

  //
  // Open Session to the TA
  //
  Status = TaOpenSession();
  if (EFI_ERROR(Status)) {
    LOG_ERROR("TaOpenSession() failed. (Status=%r)", Status);
    goto Exit;
  }

  Status = InitSecureBootVariables();
  if (EFI_ERROR(Status)) {
    LOG_ERROR("InitSecureBootVariables() failed. (Status=%r)", Status);
    goto Exit;
  }

  //
  // Publish the runtime services.
  //
  gRT->GetVariable = OpteeRuntimeGetVariable;
  gRT->GetNextVariableName = OpteeRuntimeGetNextVariableName;
  gRT->SetVariable = OpteeRuntimeSetVariable;
  gRT->QueryVariableInfo = OpteeRuntimeQueryVariableInfo;

  //
  // Install the Variable Architectural Protocol on a new handle.
  //
  Status = gBS->InstallProtocolInterface(
    &mHandle,
    &gEfiVariableArchProtocolGuid,
    EFI_NATIVE_INTERFACE,
    NULL);

  if (EFI_ERROR(Status)) {
    LOG_ERROR(
      "InstallProtocolInterface(gEfiVariableArchProtocolGuid) failed. (Status=%r)",
      Status);

    goto Exit;
  }

  //
  // Install the Variable Write Architectural Protocol on a new handle.
  //
  Status = gBS->InstallProtocolInterface(
    &mHandle,
    &gEfiVariableWriteArchProtocolGuid,
    EFI_NATIVE_INTERFACE,
    NULL);

  if (EFI_ERROR(Status)) {
    LOG_ERROR(
      "InstallProtocolInterface(gEfiVariableWriteArchProtocolGuid) failed. (Status=%r)",
      Status);

    goto Exit;
  }

  //
  // Register the event to inform auth. variable that it is at runtime.
  //
  Status = gBS->CreateEventEx(
    EVT_NOTIFY_SIGNAL,
    TPL_CALLBACK,
    OpteeRuntimeOnExitBootServices,
    NULL,
    &gEfiEventExitBootServicesGuid,
    &ExitBootServiceEvent);

  if (EFI_ERROR(Status)) {
    LOG_ERROR(
      "CreateEventEx(gEfiEventExitBootServicesGuid) failed. (Status=%r)",
      Status);

    goto Exit;
  }

Exit:
  return Status;
}