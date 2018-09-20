/** @file
 * The OP-TEE Client Library represents the implementation of the standard
 * Global Platform TEE Client API specification for the trusted OS normal world
 * client side interface.
 *
 * Copyright (c) Microsoft Corporation. All rights reserved.
 *
 * This program and the accompanying materials are licensed and made available under
 * the terms and conditions of the BSD License which accompanies this distribution.
 * The full text of the license may be found at
 * http://opensource.org/licenses/bsd-license.
 *
 * THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 * WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 */

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/OpteeClientApiLib.h>
#include <Library/tee_client_api.h>

#include "OpteeClientMem.h"
#include "OpteeClientSMC.h"
#include "OpteeClientDefs.h"

// Driver image handle to use for memory allocation.
EFI_HANDLE gDriverImageHandle = NULL;

/** Initlialize the Optee client library
**/
EFI_STATUS
OpteeClientApiInitialize (
  IN EFI_HANDLE   ImageHandle
  )
{
  EFI_STATUS Status;

  ASSERT (gDriverImageHandle == NULL);
  gDriverImageHandle = ImageHandle;

  Status = OpteeClientMemInit ();
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("OpteeClientMemInit() failed. (Status=%r)", Status);
  }

  return Status;
}

/**
  This function initializes a new TEE Context, forming a connection between this
  Client Application and the TEE identified by the string identifier Name.

  The Client Application MAY pass a NULL name, which means that the Implementation
  MUST select a default TEE to connect to. The supported name strings, the mapping
  of these names to a specific TEE, and the nature of the default TEE are
  implementation-defined.

  The caller MUST pass a pointer to a valid TEEC Context in context. The
  Implementation MUST assume that all fields of the TEEC_Context structure are in
  an undefined state.

  @param[in] Name a zero-terminated string that describes the TEE to connect to.
  If this parameter is set to NULL the Implementation MUST select a default TEE.
  @param[out] Context a TEEC_Context structure that MUST be initialized by the
  Implementation.

  @retval TEEC_SUCCESS  The initialization was successful.
  @retval TEEC_Result   Something failed.
**/
TEEC_Result
TEEC_InitializeContext (
  IN const char     *Name,
  OUT TEEC_Context  *Context
  )
{
  TEEC_Result TeecResult = TEEC_SUCCESS;

  LOG_TRACE ("Name=\"%a\", Context=0x%p", Name, Context);

  if (Context == NULL) {
    TeecResult = TEEC_ERROR_BAD_PARAMETERS;
    goto Exit;
  }

  // Current TEEC implementation doesn't support named TEE connections.
  if (Name != NULL) {
    TeecResult = TEEC_ERROR_ITEM_NOT_FOUND;
    goto Exit;
  }

  ZeroMem (Context, sizeof (*Context));

Exit:
  LOG_TRACE ("TeecResult=0x%X", TeecResult);
  return TeecResult;
}

/**
  This function finalizes an initialized TEE Context, closing the connection
  between the Client Application and the TEE. The Client Application MUST only
  call this function when all Sessions inside this TEE Context have been closed
  and all Shared Memory blocks have been released.

  The implementation of this function MUST NOT be able to fail: after this
  function returns the Client Application must be able to consider that the Context
  has been closed. The function implementation MUST do nothing if context is NULL.

  There is nothing to do here since current TEEC implementation has no defined
  Context state.

  @param[in] Context  an initialized TEEC_Context structure which is to be finalized.
*/
void
TEEC_FinalizeContext (
  IN TEEC_Context   *Context
  )
{
  LOG_TRACE ("Context=0x%p", Context);
}

/**
  This function allocates a new block of memory as a block of Shared Memory within
  the scope of the specified TEE Context, in accordance with the parameters which
  have been set by the Client Application inside the TEEC_SharedMemory structure.

  @param[in] Context  a pointer to an initialized TEE Context.
  @param[in,out] SharedMem   a pointer to a Shared Memory structure to allocate
  Before calling this function, the Client Application MUST have set the size,
  and flags fields.
  On return, for a successful allocation the Implementation MUST have set the pointer
  buffer to the address of the allocated block, otherwise it MUST set buffer to NULL.

  @retval TEEC_SUCCESS  the allocation was successful.
  @retval TEEC_ERROR_OUT_OF_MEMORY  the allocation could not be completed due to
  resourc constraints.
  @retval TEEC_Result   Something failed.
**/
TEEC_Result
TEEC_AllocateSharedMemory (
  IN TEEC_Context           *Context,
  IN OUT TEEC_SharedMemory  *SharedMem
  )
{
  TEEC_Result TeecResult = TEEC_SUCCESS;

 LOG_TRACE (
    "Context=0x%p, SharedMem=0x%p",
    Context,
    SharedMem);

  if ((Context == NULL) || (SharedMem == NULL)) {
    TeecResult = TEEC_ERROR_BAD_PARAMETERS;
    goto Exit;
  }

  // Currently not needed but ensure that it's properly set.
  if (SharedMem->flags != 0) {
    TeecResult = TEEC_ERROR_BAD_PARAMETERS;
    goto Exit;
  }

  // Clear the outbound parameters in case we fail out.

  SharedMem->buffer = NULL;
  SharedMem->shadow_buffer = NULL;

  LOG_TRACE ("size=%d, flags=0x%p", SharedMem->size, SharedMem->flags);

  SharedMem->buffer = OpteeClientMemAlloc (SharedMem->size);
  if (SharedMem->buffer == NULL) {
    LOG_ERROR (
      "OpteeClientMemAlloc() failed. (size=%p)",
      SharedMem->size);
    TeecResult = TEEC_ERROR_OUT_OF_MEMORY;
    goto Exit;
  }

  SharedMem->shadow_buffer = SharedMem->buffer;

Exit:
  LOG_TRACE ("TeecResult=0x%X", TeecResult);
  return TeecResult;
}

/**
  This function deregisters or deallocates a previously initialized block of
  Shared Memory.

  For a memory buffer allocated using TEEC_AllocateSharedMemory the Implementation
  MUST free the underlying memory and the Client Application MUST NOT access this
  region after this function has been called. In this case the Implementation MUST
  set the buffer and size fields of the sharedMem structure to NULL and 0
  respectively before returning.

  For memory registered using TEEC_RegisterSharedMemory the Implementation MUST 
  deregister the underlying memory from the TEE, but the memory region will stay
  available to the Client Application for other purposes as the memory is owned
  by it.

  The Implementation MUST do nothing if the sharedMem parameter is NULL.

  @param[in, out] SharedMem   a pointer to a valid Shared Memory structure.
**/
void
TEEC_ReleaseSharedMemory (
  IN OUT TEEC_SharedMemory  *SharedMem
  )
{
  LOG_TRACE ("SharedMem=0x%p", SharedMem);

  if (SharedMem == NULL) {
    goto Exit;
  }

  if (SharedMem->buffer == NULL) {
    goto Exit;
  }

  if (SharedMem->shadow_buffer != NULL) {
    EFI_STATUS Status;

    Status = OpteeClientMemFree (SharedMem->shadow_buffer);
    if (EFI_ERROR (Status)) {
      LOG_ERROR (
        "OpteeClientMemFree() failed. (shadow_buffer=%p)",
        SharedMem->shadow_buffer);
      goto Exit;
    }

    SharedMem->shadow_buffer = NULL;
  }

  SharedMem->buffer = NULL;
  SharedMem->size = 0;

Exit:
  return;
}

/** Register shared memory
**/
TEEC_Result
TEEC_RegisterSharedMemory (
  IN TEEC_Context           *Context,
  IN OUT TEEC_SharedMemory  *SharedMem
  )
{
  return TEEC_ERROR_NOT_IMPLEMENTED;
}

/**
  This function opens a new Session between the Client Application and the
  specified Trusted Application.

  @param[in] Context  a pointer to an initialized TEE Context.
  @param[in] Session  a pointer to a Session structure to open.
  @param[in] Destination  a pointer to a structure containing the UUID of the
  destination Trusted Application.
  @param[in] ConnectionMethod   the method of connection to use.
  @param[in] ConnectionData   any necessary data required to support the
  connection method chosen.
  @param[in] Operation  a pointer to an Operation containing a set of Parameters
  to exchange with the Trusted Application, or NULL if no Parameters are to be
  exchanged or if the operation cannot be cancelled. Refer to TEEC_InvokeCommand
  for more details.
  @param[out] ReturnOrigin   a pointer to a variable which will contain the return
  origin. This field may be NULL if the return origin is not needed.
**/
TEEC_Result
TEEC_OpenSession (
  IN TEEC_Context     *Context,
  IN TEEC_Session     *Session,
  IN const TEEC_UUID  *Destination,
  IN uint32_t         ConnectionMethod,
  IN const void       *ConnectionData,
  IN TEEC_Operation   *Operation,
  OUT uint32_t        *ReturnOrigin
  )
{
  TEEC_Result TeecResult = TEEC_SUCCESS;
  uint32_t TeecErrorOrigin = TEEC_ORIGIN_API;
  EFI_GUID TaGuid;

  LOG_TRACE (
    "Context=0x%p, Session=0x%p, Destination=0x%p, ConnectionMethod=%d, "
    "ConnectionData=0x%p, Operation=0x%p",
    Context,
    Session,
    Destination,
    ConnectionMethod,
    ConnectionData,
    Operation);

  if ((Context == NULL) || (Session == NULL) || (Destination == NULL)) {
    TeecResult = TEEC_ERROR_BAD_PARAMETERS;
    goto Exit;
  }

  if (ConnectionMethod != TEEC_LOGIN_PUBLIC) {
    TeecResult = TEEC_ERROR_NOT_SUPPORTED;
    goto Exit;
  }

  // The passed in TEEC_UUID is expected to be in the proper TEE UUID format,
  // in which the firsr 3 fields are big endian. In order for the trace function
  // to be able to properly format it we need reverse the endinaness of those
  // fields back to little-endian.
  CopyMem (&TaGuid, Destination, sizeof (TaGuid));
  TaGuid.Data1 = SWAP_B32 (TaGuid.Data1);
  TaGuid.Data2 = SWAP_B16 (TaGuid.Data2);
  TaGuid.Data3 = SWAP_B16 (TaGuid.Data3);

  LOG_TRACE ("TA UUID=%g", &TaGuid);

  {
    TEEC_Operation TeecNullOperation = { 0 };
    TEEC_Operation *TeecOperation;

    // The existing Linux client ensures that Operation is not NULL passing through
    // so do the same and pass along an empty one instead.
    // We'll also do the same for ReturnOrigin.
    if (Operation == NULL) {
      TeecOperation = &TeecNullOperation;
    } else {
      TeecOperation = Operation;
    }

    // How this is packed for the SMC call depends on the architecture so call
    //through to an architecture specific implementation.
    TeecResult = TEEC_SMC_OpenSession (
                    Context,
                    Session,
                    Destination,
                    TeecOperation,
                    &TeecErrorOrigin);
    if (TeecResult != TEEC_SUCCESS) {
      LOG_ERROR ("TEEC_SMC_OpenSession() failed.");
      goto Exit;
    }
  }

  LOG_INFO (
    "Openned Session[0x%x] with TA %g",
    Session->session_id,
    &TaGuid);

Exit:
  if (ReturnOrigin != NULL) {
    *ReturnOrigin = TeecErrorOrigin;
  }

  LOG_TRACE (
    "TeecResult=0x%X, TeecErrorOrigin=0x%X",
    TeecResult,
    TeecErrorOrigin);

  return TeecResult;
}

/**
  This function closes a Session which has been opened with a Trusted Application.
  All Commands within the Session MUST have completed before calling this function.
  The Implementation MUST do nothing if the session parameter is NULL.
  The implementation of this function MUST NOT be able to fail: after this
  function returns the Client Application must be able to consider that the
  Session has been closed.

  @param[in] Session  the session to close.
**/
void
TEEC_CloseSession (
  IN TEEC_Session  *Session
  )
{
  TEEC_Result TeecResult = TEEC_SUCCESS;
  uint32_t TeecErrorOrigin = TEEC_ORIGIN_API;

  LOG_TRACE ("Session=0x%p", Session);

  if (Session == NULL) {
    goto Exit;
  }

  TeecResult = TEEC_SMC_CloseSession (Session, &TeecErrorOrigin);

Exit:
  LOG_TRACE (
    "TeecResult=0x%X, TeecErrorOrigin=0x%X",
    TeecResult,
    TeecErrorOrigin);

  return;
}

/**
  This function invokes a Command within the specified Session.
  The parameter session MUST point to a valid open Session.
  The parameter commandID is an identifier that is used to indicate which of the
  exposed Trusted Application functions should be invoked. The supported command
  identifiers are defined by the Trusted Application.s protocol.
**/
TEEC_Result
TEEC_InvokeCommand (
  IN TEEC_Session     *Session,
  IN uint32_t         CommandID,
  IN TEEC_Operation   *Operation,
  OUT uint32_t        *ReturnOrigin
  )
{
  TEEC_Result TeecResult = TEEC_SUCCESS;
  uint32_t TeecErrorOrigin = TEEC_ORIGIN_API;

  LOG_TRACE (
    "Session=0x%p, CommandID=0x%X, Operation=0x%p",
    Session,
    CommandID,
    Operation);

  if (Session == NULL) {
    TeecResult = TEEC_ERROR_BAD_PARAMETERS;
    goto Exit;
  }

  {
    TEEC_Operation TeecNullOperation = { 0 };
    TEEC_Operation *TeecOperation;

    // The existing Linux client ensures that Operation is not NULL passing through
    // so do the same and pass along an empty one instead.
    // We'll also do the same for ReturnOrigin.
    if (Operation == NULL) {
      TeecOperation = &TeecNullOperation;
    } else {
      TeecOperation = Operation;
    }

    // How this is packed for the SMC call depends on the architecture so call through
    // to an architecture specific implementation.
    TeecResult = TEEC_SMC_InvokeCommand (
                    Session,
                    CommandID,
                    TeecOperation,
                    &TeecErrorOrigin);
  }

Exit:
  if (ReturnOrigin != NULL) {
    *ReturnOrigin = TeecErrorOrigin;
  }

  LOG_TRACE (
    "TeecResult=0x%X, TeecErrorOrigin=0x%X",
    TeecResult,
    TeecErrorOrigin);

  return TeecResult;
}

/**
  This function requests the cancellation of a pending open Session operation or
  a Command invocation operation. As this is a synchronous API, this function must
  be called from a thread other than the one executing the TEEC_OpenSession or
  TEEC_InvokeCommand function.

  @param[in] Operation  a pointer to a Client Application instantiated Operation
  structure.
**/
void
TEEC_RequestCancellation (
  IN TEEC_Operation *Operation
  )
{
  ASSERT (FALSE);
  return;
}

