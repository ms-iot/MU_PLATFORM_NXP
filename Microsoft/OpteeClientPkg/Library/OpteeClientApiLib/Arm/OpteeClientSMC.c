/** @file
  The OP-TEE SMC Interface that packs the SMC parameters and sends the SMC to
  secure world for handling.

  Copyright (c) Microsoft Corporation.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
**/

#include <Uefi.h>
#include <Library/UefiLib.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/tee_client_api.h>

#include "OpteeClientDefs.h"
#include "OpteeClientSMC.h"
#include "OpteeClientRPC.h"
#include "OpteeClientMem.h"
#include "Optee/optee_smc.h"

#undef ARRAY_SIZE
#undef BIT32
#include "Optee/optee_msg.h"


// typedef used optee internal structs to avoid prefexing variables declaration with
// struct keyword everywhere.
typedef struct optee_msg_param_tmem optee_msg_param_tmem_t;
typedef struct optee_msg_param_rmem optee_msg_param_rmem_t;
typedef struct optee_msg_param_value optee_msg_param_value_t;
typedef struct optee_msg_param optee_msg_param_t;
typedef struct optee_msg_arg optee_msg_arg_t;

VOID
SetMsgParams (
  IN TEEC_Operation       *Operation,
  OUT optee_msg_param_t   *MsgParam
  );

VOID
GetMsgParams (
  IN optee_msg_param_t   *MsgParam,
  OUT TEEC_Operation      *Operation
  );

TEEC_Result
OpteeSmcCall (
  IN optee_msg_arg_t  *MsgArg
  );

/*
 * This function opens a new Session between the Client application and the
 * specified TEE application.
 *
 * Only connection_method == TEEC_LOGIN_PUBLIC is supported connection_data and
 * Operation shall be set to NULL.
 */
TEEC_Result
TEEC_SMC_OpenSession (
  IN TEEC_Context     *Context,
  IN TEEC_Session     *Session,
  IN const TEEC_UUID  *Destination,
  IN TEEC_Operation   *Operation,
  OUT uint32_t        *ErrorOrigin
  )
{
  TEEC_Result TeecResult = TEEC_SUCCESS;
  optee_msg_arg_t *MsgArg = NULL;
  optee_msg_param_t *MsgParam = NULL;
  // Open session operation has 2 meta params that should be placed before any
  // supplied parameter, and they are Service UUID and Login type.
  static const UINTN MetaParamCount = 2;

  *ErrorOrigin = TEEC_ORIGIN_API;

  // Allocate the primary data packet from the OpTEE OS shared pool.
  {
    UINTN MsgArgSize =
      OPTEE_MSG_GET_ARG_SIZE (TEEC_CONFIG_PAYLOAD_REF_COUNT + MetaParamCount);

    MsgArg = (optee_msg_arg_t *) OpteeClientMemAlloc (MsgArgSize);
    if (MsgArg == NULL) {
      TeecResult = TEEC_ERROR_OUT_OF_MEMORY;
      goto Exit;
    }
    ZeroMem (MsgArg, MsgArgSize);
    LOG_TRACE ("MsgArg=0x%p", MsgArg);
  }

  MsgArg->cmd = OPTEE_MSG_CMD_OPEN_SESSION;
  MsgArg->num_params = TEEC_CONFIG_PAYLOAD_REF_COUNT + MetaParamCount;

  MsgParam = MsgArg->params;

  // Set the first meta parameter: service UUID
  MsgParam[0].attr = OPTEE_MSG_ATTR_TYPE_VALUE_INPUT | OPTEE_MSG_ATTR_META;
  CopyMem (&MsgParam[0].u.value, Destination, sizeof (*Destination));

  // Set the second meta parameter: Login type
  MsgParam[1].attr = OPTEE_MSG_ATTR_TYPE_VALUE_INPUT | OPTEE_MSG_ATTR_META;
  MsgParam[1].u.value.c = TEEC_LOGIN_PUBLIC;

  // Fill in the caller supplied operation parameters.
  SetMsgParams (Operation, MsgParam + MetaParamCount);

  *ErrorOrigin = TEEC_ORIGIN_COMMS;

  TeecResult = OpteeSmcCall (MsgArg);
  if (TeecResult != TEEC_SUCCESS) {
    goto Exit;
  }

  Session->session_id = MsgArg->session;
  TeecResult = MsgArg->ret;
  *ErrorOrigin = MsgArg->ret_origin;

  // Update the caller supplied Operation parameters with those returned from the call.
  GetMsgParams (MsgParam, Operation);

Exit:
  if (MsgArg != NULL) {
    OpteeClientMemFree (MsgArg);
  }

  return TeecResult;
}

/*
 * This function closes a Session which has been opened with a TEE
 * application.
 *
 * Note that the GP specification does not allow for this API to fail and return
 * a failure code however we'll support this at the SMC level so we can get
 * see debug information about such failures.
 */
TEEC_Result
TEEC_SMC_CloseSession (
  IN TEEC_Session   *Session,
  OUT uint32_t      *ErrorOrigin
  )
{
  TEEC_Result TeecResult = TEEC_SUCCESS;
  optee_msg_arg_t *MsgArg = NULL;

  *ErrorOrigin = TEEC_ORIGIN_API;

  // Allocate the primary data packet from the OpTEE OS shared pool.
  {
    UINTN MsgArgSize = OPTEE_MSG_GET_ARG_SIZE (0);

    MsgArg = (optee_msg_arg_t*) OpteeClientMemAlloc (MsgArgSize);
    if (MsgArg == NULL) {
      TeecResult = TEEC_ERROR_OUT_OF_MEMORY;
      goto Exit;
    }
    ZeroMem (MsgArg, MsgArgSize);
  }

  MsgArg->cmd = OPTEE_MSG_CMD_CLOSE_SESSION;
  MsgArg->session = Session->session_id;

  *ErrorOrigin = TEEC_ORIGIN_COMMS;

  TeecResult = OpteeSmcCall (MsgArg);
  if (TeecResult != TEEC_SUCCESS) {
    goto Exit;
  }

  TeecResult = MsgArg->ret;
  *ErrorOrigin = MsgArg->ret_origin;

Exit:
  if (MsgArg != NULL) {
    OpteeClientMemFree (MsgArg);
  }

  return TeecResult;
}

/*
 * Invokes a TEE command (secure service, sub-PA or whatever).
 */
TEEC_Result
TEEC_SMC_InvokeCommand (
  IN TEEC_Session     *Session,
  IN uint32_t         CmdId,
  IN TEEC_Operation   *Operation,
  OUT uint32_t        *ErrorOrigin
  )
{
  TEEC_Result TeecResult = TEEC_SUCCESS;
  optee_msg_arg_t *MsgArg = NULL;
  optee_msg_param_t *MsgParam = NULL;

  *ErrorOrigin = TEEC_ORIGIN_API;

  // Allocate the primary data packet from the OpTEE OS shared pool.
  {
    UINTN MsgArgSize = OPTEE_MSG_GET_ARG_SIZE (TEEC_CONFIG_PAYLOAD_REF_COUNT);

    MsgArg = (optee_msg_arg_t*) OpteeClientMemAlloc (MsgArgSize);
    if (MsgArg == NULL) {
      TeecResult = TEEC_ERROR_OUT_OF_MEMORY;
      goto Exit;
    }
    ZeroMem (MsgArg, MsgArgSize);
  }

  MsgArg->cmd = OPTEE_MSG_CMD_INVOKE_COMMAND;
  MsgArg->func = CmdId;
  MsgArg->session = Session->session_id;
  MsgArg->num_params = TEEC_CONFIG_PAYLOAD_REF_COUNT;

  MsgParam = MsgArg->params;

  // Fill in the caller supplied Operation parameters.
  SetMsgParams (Operation, MsgParam);

  *ErrorOrigin = TEEC_ORIGIN_COMMS;

  TeecResult = OpteeSmcCall (MsgArg);
  if (TeecResult != TEEC_SUCCESS) {
    goto Exit;
  }

  TeecResult = MsgArg->ret;
  *ErrorOrigin = MsgArg->ret_origin;

  // Update the caller supplied Operation parameters with those returned from the call.
  GetMsgParams (MsgParam, Operation);

Exit:
  if (MsgArg != NULL) {
    OpteeClientMemFree (MsgArg);
  }

  return TeecResult;
}

/*
 * Set the call parameter blocks in the SMC call based on the TEEC parameter supplied.
 * This only handles the parameters supplied in the originating call and not those
 * considered internal meta parameters and is thus constrained by the build
 * constants exposed to callers.
 */
VOID
SetMsgParams (
  IN TEEC_Operation       *Operation,
  OUT optee_msg_param_t   *MsgParam
  )
{
  UINTN Index;

  for (Index = 0; Index < TEEC_CONFIG_PAYLOAD_REF_COUNT; Index++) {

    uint32_t attr = TEEC_PARAM_TYPE_GET (Operation->paramTypes, Index);

    // Translate the supported memory attribute from TEEC_MEMREF_TEMP_* to
    // OPTEE_MSG_ATTR_TYPE_TMEM_* They are defined in the same sequence in both
    // headers, and the C_ASSERTs below guarantee that.
    C_ASSERT (TEEC_MEMREF_TEMP_OUTPUT == TEEC_MEMREF_TEMP_INPUT + 1);
    C_ASSERT (TEEC_MEMREF_TEMP_INOUT == TEEC_MEMREF_TEMP_OUTPUT + 1);
    C_ASSERT (OPTEE_MSG_ATTR_TYPE_TMEM_OUTPUT == OPTEE_MSG_ATTR_TYPE_TMEM_INPUT + 1);
    C_ASSERT (OPTEE_MSG_ATTR_TYPE_TMEM_INOUT == OPTEE_MSG_ATTR_TYPE_TMEM_OUTPUT + 1);
    C_ASSERT (TEEC_VALUE_OUTPUT == TEEC_VALUE_INPUT + 1);
    C_ASSERT (TEEC_VALUE_INOUT == TEEC_VALUE_OUTPUT + 1);
    C_ASSERT (OPTEE_MSG_ATTR_TYPE_VALUE_OUTPUT == OPTEE_MSG_ATTR_TYPE_VALUE_INPUT + 1);
    C_ASSERT (OPTEE_MSG_ATTR_TYPE_VALUE_INOUT == OPTEE_MSG_ATTR_TYPE_VALUE_OUTPUT + 1);

    switch (attr) {
    case TEEC_VALUE_INPUT:
    case TEEC_VALUE_OUTPUT:
    case TEEC_VALUE_INOUT:
      attr = OPTEE_MSG_ATTR_TYPE_VALUE_INPUT + (attr - TEEC_VALUE_INPUT);
      break;

    case TEEC_MEMREF_TEMP_INPUT:
    case TEEC_MEMREF_TEMP_OUTPUT:
    case TEEC_MEMREF_TEMP_INOUT:
      attr = OPTEE_MSG_ATTR_TYPE_TMEM_INPUT + (attr - TEEC_MEMREF_TEMP_INPUT);
      break;

    default:
      break;
    }

    // This assumes all parameters resolve to two uint32 values.
    MsgParam[Index].attr = attr;
    MsgParam[Index].u.value.a = Operation->params[Index].value.a;
    MsgParam[Index].u.value.b = Operation->params[Index].value.b;
  }
}

/*
 * Get the return parameter blocks from the SMC call into the TEEC parameter supplied.
 * This only handles the parameters supplied in the originating call and not those
 * considered internal meta parameters and is thus constrained by the build
 * constants exposed to callers.
 */
VOID
GetMsgParams (
  IN optee_msg_param_t  *MsgParam,
  OUT TEEC_Operation    *Operation
  )
{
  UINTN Index;

  for (Index = 0; Index < TEEC_CONFIG_PAYLOAD_REF_COUNT; Index++) {

    // This assumes all parameters resolve to two uint32 values.
    Operation->params[Index].value.a = (uint32_t) MsgParam[Index].u.value.a;
    Operation->params[Index].value.b = (uint32_t) MsgParam[Index].u.value.b;
  }
}

/*
 * Populate the SMC registers and make the call with OpTEE specific
 * handling.
 */
TEEC_Result
OpteeSmcCall (
  IN optee_msg_arg_t  *MsgArg
  )
{
  TEEC_Result TeecResult = TEEC_SUCCESS;
  ARM_SMC_ARGS ArmSmcArgs = { 0 };
  EFI_PHYSICAL_ADDRESS MsgAddr = (EFI_PHYSICAL_ADDRESS) (UINTN) MsgArg;

  LOG_TRACE ("MsgArg=0x%p", MsgArg);

  // Use the STD call style.
  // a0: SMC Function ID, OPTEE_SMC*CALL_WITH_ARG
  // a1: Upper 32 bits of a 64-bit physical pointer to a struct optee_msg_arg
  // a2: Lower 32 bits of a 64-bit physical pointer to a struct optee_msg_arg
  ArmSmcArgs.Arg0 = OPTEE_SMC_CALL_WITH_ARG;
  ArmSmcArgs.Arg1 = (UINTN) (MsgAddr >> 32);
  ArmSmcArgs.Arg2 = (UINTN) (MsgAddr & 0xFFFFFFFF);

  // This is a loop because the call may result in RPC's that will need
  // to be processed and may result in further calls until the originating
  // call is completed.
  for (;;) {

    LOG_TRACE (
      "--> SMC Call (MsgArg=0x%p, Arg0=0x%p, Arg1=0x%p, Arg2=0x%p)",
      MsgArg,
      ArmSmcArgs.Arg0,
      ArmSmcArgs.Arg1,
      ArmSmcArgs.Arg2);

    ArmCallSmc (&ArmSmcArgs);

    LOG_TRACE ("<-- SMC Return (Arg0=0x%p)", ArmSmcArgs.Arg0);

    if (OPTEE_SMC_RETURN_IS_RPC (ArmSmcArgs.Arg0)) {

      // We must service the RPC even if it's processing failed
      // and let the OpTEE OS unwind and return back to us with
      // it's error information.
      LOG_TRACE ("--> RPC Call");
      (VOID) OpteeRpcCallback (&ArmSmcArgs);
      LOG_TRACE ("<-- RPC Return");
    } else if (ArmSmcArgs.Arg0 == OPTEE_SMC_RETURN_UNKNOWN_FUNCTION) {
      TeecResult = TEEC_ERROR_NOT_IMPLEMENTED;
      break;
    } else if (ArmSmcArgs.Arg0 != OPTEE_SMC_RETURN_OK) {
      TeecResult = TEEC_ERROR_COMMUNICATION;
      break;
    } else {
      TeecResult = TEEC_SUCCESS;
      break;
    }
  }

  return TeecResult;
}

