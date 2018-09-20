/** @file
  The OP-TEE RPC Interface that handles return calls from the OpTEE OS in
  secure world that need to be processed by the normal world OS.

  Copyright (c) 2015, Microsoft Corporation.
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
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/TimerLib.h>
#include <Library/PerformanceLib.h>
#include <Library/tee_client_api.h>
#include <Protocol/RpmbIo.h>
#include <string.h>

#include "OpteeClientDefs.h"
#include "OpteeClientRPC.h"
#include "OpteeClientMem.h"
#include "Optee/tee_rpmb_fs.h"
#include "Optee/optee_smc.h"

#undef ARRAY_SIZE
#undef BIT32
#include "Optee/optee_msg.h"
#include "Optee/optee_msg_supplicant.h"

/*
  Performance Tokens
*/
#define TEEC_RPMB_READ_TOK             "TEEC:RPMB:R"
#define TEEC_RPMB_WRITE_TOK            "TEEC:RPMB:W"

// typedef used optee internal structs to avoid prefexing variables declaration with
// struct keyword everywhere.
typedef struct optee_msg_param_tmem optee_msg_param_tmem_t;
typedef struct optee_msg_param_rmem optee_msg_param_rmem_t;
typedef struct optee_msg_param_value optee_msg_param_value_t;
typedef struct optee_msg_param optee_msg_param_t;
typedef struct optee_msg_arg optee_msg_arg_t;
typedef struct rpmb_req rpmb_req_t;
typedef struct rpmb_dev_info rpmb_dev_info_t;

typedef union {
  UINT64 D64;
  VOID  *P;
  struct {
    UINT32 L32;
    UINT32 H32;
  } Fields;
} ADDRESS64;

TEEC_Result
OpteeRpcAlloc (
  IN OUT ARM_SMC_ARGS   *ArmSmcArgs
  )
{
  TEEC_Result TeecResult = TEEC_SUCCESS;
  ADDRESS64 Address = { 0 };
  UINTN Size = ArmSmcArgs->Arg1;

  LOG_TRACE ("Size=0x%p", Size);

  if (Size != 0) {
    Address.P = OpteeClientMemAlloc (Size);
    if (Address.P == NULL) {
      LOG_ERROR ("OpteeClientMemAlloc(..) failed.");
      TeecResult = TEEC_ERROR_OUT_OF_MEMORY;
      goto Exit;
    }
  }

  ArmSmcArgs->Arg1 = Address.Fields.H32;
  ArmSmcArgs->Arg2 = Address.Fields.L32;

  ArmSmcArgs->Arg4 = Address.Fields.H32;
  ArmSmcArgs->Arg5 = Address.Fields.L32;

Exit:
  LOG_TRACE ("TeecResult=0x%X", TeecResult);
  return TeecResult;
}

TEEC_Result
OpteeRpcFree (
  IN OUT ARM_SMC_ARGS   *ArmSmcArgs
  )
{
  EFI_STATUS Status;
  TEEC_Result TeecResult = TEEC_SUCCESS;
  ADDRESS64 Address = { 0 };

  Address.Fields.H32 = ArmSmcArgs->Arg1;
  Address.Fields.L32 = ArmSmcArgs->Arg2;

  LOG_TRACE ("Address=0x%p", Address.P);

  Status = OpteeClientMemFree (Address.P);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("OpteeClientMemAlloc(..) failed. (Status=%r)", Status);
    TeecResult = TEEC_ERROR_BAD_PARAMETERS;
  }

  LOG_TRACE ("TeecResult=0x%X", TeecResult);
  return TeecResult;
}

/** Load a TA from storage into memory and provide it back to OpTEE.

  param[0].u.value The TA UUID (in OPTEE format)
  param[1].u.tmem If size is not big enough to load the TA binary in, return the
  required size for loading the TA, otherwise load the TA from the FFS and copy
  it to the supplied buffer param[1].u.tmem.buf_ptr.
**/
TEEC_Result
OpteeRpcCmdLoadTa (
  IN OUT optee_msg_arg_t  *MsgArg
  )
{
  TEEC_Result TeecResult = TEEC_SUCCESS;
  optee_msg_param_t*MsgParam = NULL;
  TEEC_UUID TaUuid;
  UINTN TaBufferSize;
  VOID *ImageData = NULL;
  UINTN ImageSize = 0;
  EFI_STATUS Status;

  if (MsgArg->num_params != 2) {
    TeecResult = TEEC_ERROR_BAD_PARAMETERS;
    goto Exit;
  }

  MsgParam = MsgArg->params;

  if ((OPTEE_MSG_ATTR_GET_TYPE (MsgParam[0].attr) != OPTEE_MSG_ATTR_TYPE_VALUE_INPUT) ||
      (OPTEE_MSG_ATTR_GET_TYPE (MsgParam[1].attr) != OPTEE_MSG_ATTR_TYPE_TMEM_OUTPUT)) {

      TeecResult = TEEC_ERROR_BAD_PARAMETERS;
      goto Exit;
  }

  // It should be safe to treat TEEC_UUID as the UEFI native EFI_GUID since they
  // have the same layout, but differ in some fields byte-order.
  C_ASSERT (sizeof (EFI_GUID) == sizeof (TEEC_UUID));

  CopyMem (&TaUuid, &MsgParam[0].u.value, sizeof (TEEC_UUID));
  TaBufferSize = MsgParam[1].u.tmem.size;

  // OPTEE UUID format has the first 3 fields in big-endian byte order, swap
  // them back to little-endian.
  TaUuid.timeLow= SWAP_B32 (TaUuid.timeLow);
  TaUuid.timeMid = SWAP_B16 (TaUuid.timeMid);
  TaUuid.timeHiAndVersion = SWAP_B16 (TaUuid.timeHiAndVersion);

  LOG_TRACE ("TaUuid=%g, TaBufferSize=0x%p", (EFI_GUID *) &TaUuid, TaBufferSize);

  // Load the TA image from the Flash Volume (FV) and then allocate a suitably
  // sized OpTEE shared memory buffer to copy it into.
  Status = GetSectionFromAnyFv (
              (EFI_GUID *) &TaUuid,
              EFI_SECTION_RAW,
              0,
              &ImageData,
              &ImageSize);

  if (EFI_ERROR (Status)) {
    LOG_ERROR("GetSectionFromAnyFv(..) failed. (Status=%r)", Status);
    TeecResult = TEEC_ERROR_ITEM_NOT_FOUND;
    goto Exit;
  }

  // Supplied buffer size is not big enough, update the required size and
  // succeed. OPTEE expects that the call will succeed even if the size wasn't
  // big enouhg. That's a bug in OPTEE rpc_load function in ree_fs_ta.c
  if (TaBufferSize < ImageSize) {
    LOG_TRACE ("Supplied buffer is too small, TA ImageSize = 0x%p", ImageSize);
    MsgParam[1].u.tmem.size = ImageSize;
    TeecResult = TEEC_SUCCESS;
    goto Exit;
  }

  CopyMem ((VOID *)(UINTN) MsgParam[1].u.tmem.buf_ptr, ImageData, ImageSize);

  LOG_INFO (
    "TA %g loaded at 0x%p, size=0x%p bytes",
    (EFI_GUID *) &TaUuid,
    ImageData,
    ImageSize);

Exit:
  MsgArg->ret = TeecResult;
  MsgArg->ret_origin = TEEC_ORIGIN_API;

  if (ImageData != NULL) {
    FreePool (ImageData);
  }

  LOG_TRACE ("TeecResult=0x%X", TeecResult);
  return TeecResult;
}


UINT16
RpmbBytesToUint16 (
  IN CONST UINT8  *RpmbBytes
  )
{
  ASSERT (RpmbBytes != NULL);

  return ((UINT16) RpmbBytes[0] << 8) | ((UINT16) RpmbBytes[1]);
}

VOID
HexDump (
  IN CONST UINT8  *Buffer,
  IN UINTN        Size
  )
{
  UINTN i;

  for (i = 0; i < Size; i++) {
    if ((i > 0 ) && ((i % 16) == 0)) {
      LOG_VANILLA_TRACE ("\n");
    }
    LOG_VANILLA_TRACE ("%02x ", Buffer[i]);
  }
  LOG_VANILLA_TRACE ("\n");
}

VOID
RpmbDumpPacket (
  IN EFI_RPMB_DATA_PACKET *Packet
  )
{
#if 0
  LOG_TRACE ("Key/MAC");
  HexDump (Packet->KeyOrMAC, EFI_RPMB_PACKET_KEY_MAC_SIZE);
  LOG_TRACE ("Data");
  HexDump (Packet->PacketData, EFI_RPMB_PACKET_DATA_SIZE);
#endif

  LOG_VANILLA_TRACE ("Write Counter: ");
  HexDump (Packet->WriteCounter, EFI_RPMB_PACKET_WCOUNTER_SIZE);
  LOG_VANILLA_TRACE ("Address: ");
  HexDump (Packet->Address, EFI_RPMB_PACKET_ADDRESS_SIZE);
  LOG_VANILLA_TRACE ("Block Count: ");
  HexDump (Packet->BlockCount, EFI_RPMB_PACKET_BLOCKCOUNT_SIZE);
  LOG_VANILLA_TRACE ("Result: ");
  HexDump (Packet->OperationResult, EFI_RPMB_PACKET_RESULT_SIZE);
  LOG_VANILLA_TRACE ("Req/Res Type: ");
  HexDump (Packet->RequestOrResponseType, EFI_RPMB_PACKET_TYPE_SIZE);
}

/** Execute an RPMB storage operation.
**/
TEEC_Result
OpteeRpcCmdRpmb (
  IN OUT optee_msg_arg_t  *MsgArg
  )
{
  rpmb_dev_info_t *RpmbDevInfo;
  rpmb_req_t *RpmbRequest;
  EFI_STATUS Status;
  EFI_RPMB_IO_PROTOCOL *RpmbProtocol = NULL;
  UINT16 RequestMsgType;
  EFI_RPMB_DATA_PACKET *RequestPackets;
  EFI_RPMB_DATA_PACKET *ResponsePackets;
  EFI_RPMB_DATA_BUFFER RpmbBuffer;

  TEEC_Result TeecResult = TEEC_SUCCESS;
  optee_msg_param_t *MsgParam;

  if (MsgArg->num_params != 2) {
    TeecResult = TEEC_ERROR_BAD_PARAMETERS;
    goto Exit;
  }

  MsgParam = MsgArg->params;

  if ((OPTEE_MSG_ATTR_GET_TYPE (MsgParam[0].attr) != OPTEE_MSG_ATTR_TYPE_TMEM_INPUT) ||
      (OPTEE_MSG_ATTR_GET_TYPE (MsgParam[1].attr) != OPTEE_MSG_ATTR_TYPE_TMEM_OUTPUT)) {

    TeecResult = TEEC_ERROR_BAD_PARAMETERS;
    goto Exit;
  }

  Status = gBS->LocateProtocol (
                  &gEfiRpmbIoProtocolGuid,
                  NULL,
                  (VOID **) &RpmbProtocol);

  if (EFI_ERROR (Status)) {
    LOG_ERROR ("gBS->LocateProtocol(gEfiRpmbIoProtocolGuid) failed. (Status=%r)", Status);
    TeecResult = TEEC_ERROR_NOT_SUPPORTED;
    goto Exit;
  }

  RpmbRequest = (rpmb_req_t *)(UINTN) MsgParam[0].u.tmem.buf_ptr;

  switch (RpmbRequest->cmd) {
    case RPMB_CMD_DATA_REQ:
    {
      RequestPackets = (EFI_RPMB_DATA_PACKET *) (RpmbRequest + 1);
      ResponsePackets = (EFI_RPMB_DATA_PACKET *)(UINTN) MsgParam[1].u.tmem.buf_ptr;

      RequestMsgType =
        RpmbBytesToUint16 (RequestPackets->RequestOrResponseType);

#if 0
      RpmbDumpPacket (RequestPackets);
#endif

      switch (RequestMsgType) {
        case EFI_RPMB_REQUEST_PROGRAM_KEY:
        {
          LOG_TRACE ("EFI_RPMB_REQUEST_PROGRAM_KEY");

          C_ASSERT (EFI_RPMB_REQUEST_PROGRAM_KEY == RPMB_MSG_TYPE_REQ_AUTH_KEY_PROGRAM);
          Status = RpmbProtocol->ProgramKey (
                                    RpmbProtocol,
                                    RequestPackets,
                                    ResponsePackets);

          if (EFI_ERROR (Status)) {
            LOG_ERROR ("RpmbProtocol->ProgramKey() failed. (Status=%r)", Status);
            TeecResult = TEEC_ERROR_GENERIC;
          }
          break;
        }

        case EFI_RPMB_REQUEST_COUNTER_VALUE:
        {
          LOG_TRACE ("EFI_RPMB_REQUEST_COUNTER_VALUE");

          C_ASSERT (EFI_RPMB_REQUEST_COUNTER_VALUE == RPMB_MSG_TYPE_REQ_WRITE_COUNTER_VAL_READ);
          Status = RpmbProtocol->ReadCounter (
                                    RpmbProtocol,
                                    RequestPackets,
                                    ResponsePackets);

          if (EFI_ERROR (Status)) {
            LOG_ERROR ("RpmbProtocol->ReadCounter() failed. (Status=%r)", Status);
            TeecResult = TEEC_ERROR_GENERIC;
          }
          break;
        }

        case EFI_RPMB_REQUEST_AUTH_WRITE:
        {
          PERF_START (gDriverImageHandle, TEEC_RPMB_WRITE_TOK, NULL, 0);

          C_ASSERT (EFI_RPMB_REQUEST_AUTH_WRITE == RPMB_MSG_TYPE_REQ_AUTH_DATA_WRITE);
          RpmbBuffer.Packets = RequestPackets;
          RpmbBuffer.PacketCount = 
            RpmbBytesToUint16 (RequestPackets->BlockCount);

          LOG_TRACE ("EFI_RPMB_REQUEST_AUTH_WRITE. (BlockCount=0x%p)",
            RpmbBuffer.PacketCount);

          Status = RpmbProtocol->AuthenticatedWrite (
                                    RpmbProtocol,
                                    &RpmbBuffer,
                                    ResponsePackets);

          if (EFI_ERROR (Status)) {
            LOG_ERROR ("RpmbProtocol->AuthenticatedWrite() failed. (Status=%r)", Status);
            TeecResult = TEEC_ERROR_GENERIC;
          }

          PERF_END (gDriverImageHandle, TEEC_RPMB_WRITE_TOK, NULL, 0);
          break;
        }

        case EFI_RPMB_REQUEST_AUTH_READ:
        {
          PERF_START (gDriverImageHandle, TEEC_RPMB_READ_TOK, NULL, 0);

          C_ASSERT (EFI_RPMB_REQUEST_AUTH_READ == RPMB_MSG_TYPE_REQ_AUTH_DATA_READ);
          RpmbBuffer.Packets = ResponsePackets;
          RpmbBuffer.PacketCount = RpmbRequest->block_count;

          LOG_TRACE ("EFI_RPMB_REQUEST_AUTH_READ. (BlockCount=0x%p)",
            RpmbBuffer.PacketCount);

          Status = RpmbProtocol->AuthenticatedRead (
                                    RpmbProtocol,
                                    RequestPackets,
                                    &RpmbBuffer);

          if (EFI_ERROR (Status)) {
            LOG_ERROR ("RpmbProtocol->AuthenticatedRead() failed. (Status=%r)", Status);
            TeecResult = TEEC_ERROR_GENERIC;
          }

          PERF_END (gDriverImageHandle, TEEC_RPMB_READ_TOK, NULL, 0);
          break;
        }

        default:
          LOG_ERROR ("Unspported RPMB Request. (RequestMsgType=0x%X)", RequestMsgType);
          ASSERT (FALSE);
          TeecResult = TEEC_ERROR_BAD_PARAMETERS;
          break;
      }

      break;
    }

    case RPMB_CMD_GET_DEV_INFO:
    {
      LOG_INFO (
        "RPMB Get Dev Info: RpmbSizeMult=%d ReliableSectorCount=%d",
        RpmbProtocol->RpmbSizeMult,
        RpmbProtocol->ReliableSectorCount);

      RpmbDevInfo = (rpmb_dev_info_t *)(UINTN) MsgParam[1].u.tmem.buf_ptr;

      C_ASSERT (sizeof(RpmbDevInfo->cid) == sizeof(RpmbProtocol->Cid));
      memcpy (RpmbDevInfo->cid, RpmbProtocol->Cid, RPMB_EMMC_CID_SIZE);
      RpmbDevInfo->rel_wr_sec_c = (uint8_t) RpmbProtocol->ReliableSectorCount;
      RpmbDevInfo->rpmb_size_mult = (uint8_t) RpmbProtocol->RpmbSizeMult;
      RpmbDevInfo->ret_code = RPMB_CMD_GET_DEV_INFO_RET_OK;

      break;
    }

  default:
    LOG_ASSERT ("Unsupported RPMB_CMD_*");
    TeecResult = TEEC_ERROR_NOT_SUPPORTED;
    break;
  }

Exit:
  MsgArg->ret = TeecResult;
  MsgArg->ret_origin = TEEC_ORIGIN_API;

  LOG_TRACE ("TeecResult=0x%X", TeecResult);

  return TeecResult;
}

/** Allocate a piece of shared memory.

  Shared memory can optionally be fragmented, to support that additional
  spare param entries are allocated to make room for eventual fragments.
  The spare param entries has .attr = OPTEE_MSG_ATTR_TYPE_NONE when
  unused. All returned temp memrefs except the last should have the
  OPTEE_MSG_ATTR_FRAGMENT bit set in the attr field.

  [in]  param[0].u.value.a type of memory one of OPTEE_MSG_RPC_SHM_TYPE_* below.
  [in]  param[0].u.value.b requested size.
  [in]  param[0].u.value.c required alignment.

  [out] param[0].u.tmem.buf_ptr physical address (of first fragment).
  [out] param[0].u.tmem.size size (of first fragment).
  [out] param[0].u.tmem.shm_ref shared memory reference.

  [out] param[n].u.tmem.buf_ptr physical address.
  [out] param[n].u.tmem.size size.
  [out] param[n].u.tmem.shm_ref shared memory reference (same value as in
  param[n-1].u.tmem.shm_ref).
 */
TEEC_Result
OpteeRpcCmdShmAlloc (
  IN OUT optee_msg_arg_t  *MsgArg
  )
{
  TEEC_Result TeecResult = TEEC_SUCCESS;
  optee_msg_param_t *MsgParam;
  UINTN BufferSize;
  UINTN BufferAlignment;
  VOID *Buffer;

  if (MsgArg->num_params != 1) {
    TeecResult = TEEC_ERROR_BAD_PARAMETERS;
    goto Exit;
  }

  MsgParam = MsgArg->params;

  if (OPTEE_MSG_ATTR_GET_TYPE (MsgParam[0].attr) != OPTEE_MSG_ATTR_TYPE_VALUE_INPUT) {
    TeecResult = TEEC_ERROR_BAD_PARAMETERS;
    goto Exit;
  }

  switch (MsgParam[0].u.value.a) {
    case OPTEE_MSG_RPC_SHM_TYPE_APPL:
    case OPTEE_MSG_RPC_SHM_TYPE_KERNEL:
      break;

    default:
      LOG_ASSERT ("Unsupported shared memory type");
      TeecResult = TEEC_ERROR_BAD_PARAMETERS;
      break;
  }

  BufferSize = (UINTN) MsgParam[0].u.value.b;
  BufferAlignment = (UINTN) MsgParam[0].u.value.c;

  Buffer = OpteeClientAlignedMemAlloc (BufferSize, BufferAlignment);
  if (Buffer == NULL) {
    LOG_ERROR (
      "OpteeClientAlignedMemAlloc() failed.(BufferSize=0x%p, BufferAlignment=0x%p)",
      BufferSize,
      BufferAlignment);

    TeecResult = TEEC_ERROR_OUT_OF_MEMORY;
    goto Exit;
  }

  MsgParam[0].attr = OPTEE_MSG_ATTR_TYPE_TMEM_OUTPUT;
  MsgParam[0].u.tmem.buf_ptr = (uint64_t) (UINTN) Buffer;
  MsgParam[0].u.tmem.size = (uint64_t) BufferSize;
  MsgParam[0].u.tmem.shm_ref = (uint64_t) (UINTN) Buffer;

  LOG_TRACE (
    "Shared Memory Allocated (Buffer=%p, BufferSize=0x%p)",
    Buffer,
    BufferSize);

Exit:
  return TeecResult;
}

/** Free shared memory previously allocated with OPTEE_MSG_RPC_CMD_SHM_ALLOC.

  [in]  param[0].u.value.a   type of memory one of OPTEE_MSG_RPC_SHM_TYPE_*.
  [in]  param[0].u.value.b   value of shared memory reference returned in
  param[0].u.tmem.shm_ref above.
**/
TEEC_Result
OpteeRpcCmdShmFree (
  IN optee_msg_arg_t  *MsgArg
  )
{
  TEEC_Result TeecResult = TEEC_SUCCESS;
  EFI_STATUS Status;
  optee_msg_param_t *MsgParam;

  if (MsgArg->num_params != 1) {
    TeecResult = TEEC_ERROR_BAD_PARAMETERS;
    goto Exit;
  }

  MsgParam = MsgArg->params;

  if (OPTEE_MSG_ATTR_GET_TYPE (MsgParam[0].attr) != OPTEE_MSG_ATTR_TYPE_VALUE_INPUT) {
    TeecResult = TEEC_ERROR_BAD_PARAMETERS;
    goto Exit;
  }

  switch (MsgParam[0].u.value.a) {
    case OPTEE_MSG_RPC_SHM_TYPE_APPL:
    case OPTEE_MSG_RPC_SHM_TYPE_KERNEL:
      break;

    default:
      LOG_ASSERT ("Unexpected shared memory type");
      TeecResult = TEEC_ERROR_BAD_PARAMETERS;
      break;
  }

  // The cookie we set on shared mem alloc is the physical address itself, so
  // we can use it for free directly.
  Status = OpteeClientMemFree ((VOID *) (UINTN) MsgParam[0].u.value.b);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("OpteeClientMemFree() failed. (Status=%r)", Status);
    TeecResult = TEEC_ERROR_BAD_STATE;
    goto Exit;
  }

Exit:
  LOG_TRACE ("TeecResult=0x%X", TeecResult);
  return TeecResult;
}

/** Get time

  Returns number of seconds and nano seconds since the Epoch,
  1970-01-01 00:00:00 +0000 (UTC) where:
  param[0].u.value.a is number of seconds.
  param[0].u.value.b is number of nano seconds.
**/
TEEC_Result
OpteeRpcCmdGetTime (
  OUT optee_msg_arg_t  *MsgArg
  )
{
  TEEC_Result TeecResult = TEEC_SUCCESS;
  optee_msg_param_t *MsgParam;

  if (MsgArg->num_params != 1) {
    TeecResult = TEEC_ERROR_BAD_PARAMETERS;
    goto Exit;
  }

  MsgParam = MsgArg->params;

  if (OPTEE_MSG_ATTR_GET_TYPE (MsgParam[0].attr) != OPTEE_MSG_ATTR_TYPE_VALUE_OUTPUT) {
    TeecResult = TEEC_ERROR_BAD_PARAMETERS;
    goto Exit;
  }

  static const UINT64 NANOS_PER_SEC = 1000000000;
  UINT64 Ticks = GetPerformanceCounter ();
  UINT64 Freq = GetPerformanceCounterProperties (NULL, NULL);

  UINT64 ElapsedSeconds = Ticks / Freq;
  UINT64 ElapsedNanos = (Ticks * NANOS_PER_SEC) / Freq;

  MsgParam[0].u.value.a = Ticks / Freq;
  MsgParam[0].u.value.b = ElapsedNanos - (ElapsedSeconds * NANOS_PER_SEC);

  LOG_TRACE (
    "Seconds:%ld, Nanoseconds:%ld",
    MsgParam[0].u.value.a,
    MsgParam[0].u.value.b);

Exit:
  LOG_TRACE ("TeecResult=0x%X", TeecResult);
  return TeecResult;
}

TEEC_Result
OpteeRpcCmdWaitQueue (
    OUT optee_msg_arg_t  *MsgArg
  )
{
  TEEC_Result TeecResult = TEEC_SUCCESS;
  optee_msg_param_t *MsgParam;

  if (MsgArg->num_params != 1) {
    TeecResult = TEEC_ERROR_BAD_PARAMETERS;
    goto Exit;
  }

  MsgParam = MsgArg->params;

  if (OPTEE_MSG_ATTR_GET_TYPE (MsgParam[0].attr) != OPTEE_MSG_ATTR_TYPE_VALUE_INPUT) {
    TeecResult = TEEC_ERROR_BAD_PARAMETERS;
    goto Exit;
  }

  // The handler simply do nothing, since UEFI is a single thread, single core
  // environment.

  switch (MsgParam[0].u.value.a) {
    case OPTEE_MSG_RPC_WAIT_QUEUE_SLEEP:
      LOG_ERROR ("OPTEE_MSG_RPC_WAIT_QUEUE_SLEEP (key = 0x%lX)", MsgParam[0].u.value.b);
      break;

    case OPTEE_MSG_RPC_WAIT_QUEUE_WAKEUP:
      LOG_ERROR ("OPTEE_MSG_RPC_WAIT_QUEUE_WAKEUP (key = 0x%lX)", MsgParam[0].u.value.b);
      break;

    default:
      LOG_ASSERT ("OPTEE_MSG_RPC_WAIT_QUEUE_*");
      TeecResult = TEEC_ERROR_NOT_IMPLEMENTED;
      break;
  }

Exit:
  LOG_TRACE ("TeecResult=0x%X", TeecResult);
  return TeecResult;
}

/*
 * Handle the callback from secure world.
 */
TEEC_Result
OpteeRpcCallback (
  IN ARM_SMC_ARGS   *ArmSmcArgs
  )
{
  TEEC_Result TeecResult = TEEC_SUCCESS;

  ASSERT (OPTEE_SMC_RETURN_IS_RPC (ArmSmcArgs->Arg0));

  LOG_TRACE(
    "Arg0=0x%p, Arg1=0x%p, Arg2=0x%p",
    ArmSmcArgs->Arg0,
    ArmSmcArgs->Arg1,
    ArmSmcArgs->Arg2);

  switch (OPTEE_SMC_RETURN_GET_RPC_FUNC (ArmSmcArgs->Arg0)) {
    case OPTEE_SMC_RPC_FUNC_ALLOC:
      LOG_TRACE ("OPTEE_SMC_RPC_FUNC_ALLOC");
      TeecResult = OpteeRpcAlloc (ArmSmcArgs);
      break;

    case OPTEE_SMC_RPC_FUNC_FREE:
      LOG_TRACE ("OPTEE_SMC_RPC_FUNC_FREE");
      TeecResult = OpteeRpcFree (ArmSmcArgs);
      break;

    // The IRQ would have been processed on return to normal world
    // so there is no further action to take other than to return.
    case OPTEE_SMC_RPC_FUNC_FOREIGN_INTR:
      LOG_TRACE ("OPTEE_SMC_RPC_FUNC_FOREIGN_INTR");
      break;

    // This actually means an arg parameter block has come back that breaks
    // down further into various other commands.
    case OPTEE_SMC_RPC_FUNC_CMD:
    {
      ADDRESS64 Address = { 0 };
      Address.Fields.H32 = ArmSmcArgs->Arg1;
      Address.Fields.L32 = ArmSmcArgs->Arg2;

      optee_msg_arg_t *MsgArg = (optee_msg_arg_t *) Address.P;
      ASSERT (MsgArg != NULL);

      LOG_TRACE (
        "OPTEE_SMC_RPC_FUNC_CMD: cmd=0x%X, session=0x%X, num_params=%d",
        MsgArg->cmd,
        MsgArg->session,
        MsgArg->num_params);

      switch (MsgArg->cmd) {
        case OPTEE_MSG_RPC_CMD_LOAD_TA:
          LOG_TRACE ("OPTEE_MSG_RPC_CMD_LOAD_TA");
          TeecResult = OpteeRpcCmdLoadTa (MsgArg);
          break;

        case OPTEE_MSG_RPC_CMD_RPMB:
          LOG_TRACE ("OPTEE_MSG_RPC_CMD_RPMB");
          TeecResult = OpteeRpcCmdRpmb (MsgArg);
          break;

        case OPTEE_MSG_RPC_CMD_GET_TIME:
          LOG_TRACE ("OPTEE_MSG_RPC_CMD_GET_TIME");
          TeecResult = OpteeRpcCmdGetTime (MsgArg);
          break;

        case OPTEE_MSG_RPC_CMD_SHM_ALLOC:
          LOG_TRACE ("OPTEE_MSG_RPC_CMD_SHM_ALLOC");
          TeecResult = OpteeRpcCmdShmAlloc (MsgArg);
          break;

        case OPTEE_MSG_RPC_CMD_SHM_FREE:
          LOG_TRACE ("OPTEE_MSG_RPC_CMD_SHM_FREE");
          TeecResult = OpteeRpcCmdShmFree (MsgArg);
          break;

        case OPTEE_MSG_RPC_CMD_WAIT_QUEUE:
          LOG_TRACE ("OPTEE_MSG_RPC_CMD_WAIT_QUEUE");
          TeecResult = OpteeRpcCmdWaitQueue (MsgArg);
          break;

        default:
          LOG_ASSERT ("Unsupported OPTEE_MSG_RPC_CMD_*");
          TeecResult = TEEC_ERROR_NOT_IMPLEMENTED;
          break;
      }

      MsgArg->ret = TeecResult;
      break;
    }

    default:
      LOG_ASSERT ("Unsupported OPTEE_SMC_RPC_FUNC_*");
      TeecResult = TEEC_ERROR_NOT_IMPLEMENTED;
      break;
  }

  // Send back the return code for the next call.
  ArmSmcArgs->Arg0 = OPTEE_SMC_CALL_RETURN_FROM_RPC;

  LOG_TRACE ("TeecResult=0x%X", TeecResult);
  return TeecResult;
}

