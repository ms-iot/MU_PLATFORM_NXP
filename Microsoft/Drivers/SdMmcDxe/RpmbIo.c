/** @file
*
*  Copyright (c) Microsoft Corporation. All rights reserved.
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

#include <Protocol/BlockIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/Sdhc.h>
#include <Protocol/RpmbIo.h>

#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/TimerLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>

#include "SdMmcHw.h"
#include "SdMmc.h"
#include "Protocol.h"

C_ASSERT(sizeof(EFI_RPMB_DATA_PACKET) == SD_BLOCK_LENGTH_BYTES);

EFI_RPMB_DATA_PACKET ResultRequest = {
  { 0 }, // Stuff
  { 0 }, // MAC
  { 0 }, // Data
  { 0 }, // Nonce
  { 0 }, // WriteCounter
  { 0 }, // Address
  { 0 }, // PacketCount
  { 0 }, // OperationResult
  { 0x00, EFI_RPMB_REQUEST_RESULT_REQUEST } // RequestType
};

// JEDEC Standard No. 84-A441:
// Byte order of the RPMB data frame is MSB first, e.g. Write Counter MSB [11]
// is storing the upmost byte of the counter value.

VOID
Uint16ToRpmbBytes (
  IN UINT16   Value,
  OUT UINT8   *RpmbBytes
  )
{
  ASSERT (RpmbBytes != NULL);

  RpmbBytes[0] = (UINT8) (Value >> 8);
  RpmbBytes[1] = (UINT8) (Value & 0xF);
}


UINT16
RpmbBytesToUint16 (
  IN CONST UINT8  *RpmbBytes
  )
{
  ASSERT (RpmbBytes != NULL);

  return ((UINT16) RpmbBytes[0] << 8) | ((UINT16) RpmbBytes[1]);
}

EFI_STATUS
RpmbRead (
  IN SDHC_INSTANCE          *HostInst,
  IN EFI_RPMB_DATA_PACKET   *Request,
  OUT EFI_RPMB_DATA_BUFFER  *Response
  )
{
  EFI_STATUS Status;

  Status = SdhcSetBlockCount (HostInst, 1, FALSE);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("SdhcSetBlockCount() failed. (Status = %r)", Status);
    return Status;
  }

  Status = SdhcSendDataCommand (
    HostInst,
    &CmdWriteMultiBlock,
    0, // LBA argument is ignored for RPMB access, the eMMC will use instead the Address
       // field of the RPMB packet
    sizeof (*Request),
    Request);

  if (EFI_ERROR (Status)) {
    LOG_ERROR (
      "Failed to write the Read request data packet. (Status = %r)",
      Status);

    return Status;
  }

  Status = SdhcSetBlockCount (HostInst, Response->PacketCount, FALSE);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("SdhcSetBlockCount() failed. (Status = %r)", Status);
    return Status;
  }

  Status = SdhcSendDataCommand (
    HostInst,
    &CmdReadMultiBlock,
    0, // LBA argument is ignored for RPMB access, the eMMC will use instead the Address
       // field of the RPMB packet
    Response->PacketCount * sizeof(*Response->Packets),
    Response->Packets);

  if (EFI_ERROR (Status)) {
    LOG_ERROR (
      "Failed to read the Read request data packet. (Status = %r)",
      Status);

    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
RpmbWrite (
  IN SDHC_INSTANCE          *HostInst,
  IN EFI_RPMB_DATA_BUFFER   *Request
  )
{
  EFI_STATUS Status;

  Status = SdhcSetBlockCount (HostInst, Request->PacketCount, TRUE);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("SdhcSetBlockCount() failed. (Status = %r)", Status);
    return Status;
  }

  Status = SdhcSendDataCommand (
    HostInst,
    &CmdWriteMultiBlock,
    0, // LBA argument is ignored for RPMB access, the eMMC will use instead the Address
       // field of the RPMB packet
    Request->PacketCount * sizeof(*Request->Packets),
    Request->Packets);

  if (EFI_ERROR (Status)) {
    LOG_ERROR (
      "Failed to write the Read request data packet. (Status = %r)",
      Status);

    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
RpmbQueryResult (
  IN SDHC_INSTANCE          *HostInst,
  OUT EFI_RPMB_DATA_PACKET  *Response
  )
{
  EFI_STATUS Status;
  EFI_RPMB_DATA_BUFFER ResponseBuffer;

  ResponseBuffer.PacketCount = 1;
  ResponseBuffer.Packets = Response;

  Status = RpmbRead (HostInst, &ResultRequest, &ResponseBuffer);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("RpmbRead() failed. (Status = %r)", Status);
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
RpmbProgramKeyRequest (
  IN SDHC_INSTANCE          *HostInst,
  IN EFI_RPMB_DATA_PACKET   *Request,
  OUT EFI_RPMB_DATA_PACKET  *Response
  )
{
  EFI_STATUS Status;
  EFI_RPMB_DATA_BUFFER RequestBuffer;

  RequestBuffer.PacketCount = 1;
  RequestBuffer.Packets = Request;

  Status = RpmbWrite (HostInst, &RequestBuffer);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("RpmbWrite() failed. (Status = %r)", Status);
    return Status;
  }

  Status = RpmbQueryResult (HostInst, Response);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("RpmbQueryResult() failed. (Status = %r)", Status);
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
RpmbReadCounterRequest (
  IN SDHC_INSTANCE          *HostInst,
  IN EFI_RPMB_DATA_PACKET   *Request,
  OUT EFI_RPMB_DATA_PACKET  *Response
  )
{
  EFI_STATUS Status;
  EFI_RPMB_DATA_BUFFER ResponseBuffer;

  ResponseBuffer.PacketCount = 1;
  ResponseBuffer.Packets = Response;

  Status = RpmbRead (HostInst, Request, &ResponseBuffer);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("RpmbRead() failed. (Status = %r)", Status);
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
RpmbAuthenticatedWriteRequest (
  IN SDHC_INSTANCE          *HostInst,
  IN EFI_RPMB_DATA_BUFFER   *Request,
  OUT EFI_RPMB_DATA_PACKET  *Response
  )
{
  EFI_STATUS Status;

  Status = RpmbWrite (HostInst, Request);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("RpmbWrite() failed. (Status = %r)", Status);
    return Status;
  }

  Status = RpmbQueryResult (HostInst, Response);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("RpmbQueryResult() failed. (Status = %r)", Status);
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
RpmbAuthenticatedReadRequest (
  IN SDHC_INSTANCE          *HostInst,
  IN EFI_RPMB_DATA_PACKET   *Request,
  OUT EFI_RPMB_DATA_BUFFER  *Response
  )
{
  EFI_STATUS Status;

  Status = RpmbRead (HostInst, Request, Response);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("RpmbRead() failed. (Status = %r)", Status);
    return Status;
  }

  return EFI_SUCCESS;
}

VOID
RpmbHexDump (
  IN CONST UINT8  *Bytes,
  IN UINTN        Len
  )
{
  UINTN i;

  for (i = 0; i < Len; i++) {
    if ((i > 0) && (i % 16) == 0) {
      LOG_VANILLA_TRACE ("\n");
    }
    LOG_VANILLA_TRACE ("%02x ", *Bytes);
    ++Bytes;
  }
  LOG_VANILLA_TRACE ("\n");
}

VOID
RpmbDumpPacket (
  IN EFI_RPMB_DATA_PACKET *Packet
  )
{
  LOG_TRACE ("*** RPMB Packet Dump (Packet = %p) ***", Packet);
  LOG_TRACE ("- Write Counter");
  RpmbHexDump (Packet->WriteCounter, EFI_RPMB_PACKET_WCOUNTER_SIZE);
  LOG_TRACE ("- Address:");
  RpmbHexDump (Packet->Address, EFI_RPMB_PACKET_ADDRESS_SIZE);
  LOG_TRACE ("- Block Count:");
  RpmbHexDump (Packet->BlockCount, EFI_RPMB_PACKET_BLOCKCOUNT_SIZE);
  LOG_TRACE ("- Result:");
  RpmbHexDump (Packet->OperationResult, EFI_RPMB_PACKET_RESULT_SIZE);
  LOG_TRACE ("- Req/Res Type:");
  RpmbHexDump (Packet->RequestOrResponseType, EFI_RPMB_PACKET_TYPE_SIZE);
}

EFI_STATUS
RpmbRequest (
  IN EFI_RPMB_IO_PROTOCOL   *This,
  IN EFI_RPMB_DATA_BUFFER   *Request,
  OUT EFI_RPMB_DATA_BUFFER  *Response
  )
{
  EFI_STATUS Status;
  SDHC_INSTANCE *HostInst;
  MMC_EXT_CSD_PARTITION_ACCESS CurrentPartition;
  EFI_STATUS SwitchStatus;
  BOOLEAN SwitchPartition;
  UINT16 RequestType;

  SwitchPartition = FALSE;
  Status = EFI_SUCCESS;
  SwitchStatus = EFI_SUCCESS;

  ASSERT (This);
  ASSERT (Request);
  ASSERT (Response);

  HostInst = SDHC_INSTANCE_FROM_RPMB_IO_THIS (This);
  ASSERT (HostInst);
  ASSERT (HostInst->HostExt);

  CurrentPartition = HostInst->CurrentMmcPartition;

  SwitchStatus = SdhcSwitchPartitionMmc (HostInst, MmcExtCsdPartitionAccessRpmb);
  if (EFI_ERROR (SwitchStatus)) {

    LOG_ERROR (
      "SdhcSwitchPartitionMmc() failed. (SwitchStatus = %r)",
      SwitchStatus);

    goto Exit;
  }

  SwitchPartition = TRUE;

  ASSERT (Request->PacketCount > 0);
  ASSERT (Request->Packets != NULL);
  RequestType = RpmbBytesToUint16 (Request->Packets[0].RequestOrResponseType);

  switch (RequestType) {
  case EFI_RPMB_REQUEST_PROGRAM_KEY:
    Status = RpmbProgramKeyRequest (HostInst, Request->Packets, Response->Packets);
    if (EFI_ERROR (Status)) {
      LOG_ERROR ("RpmbProgramKeyRequest() failed. (Status = %r)", Status);
      goto Exit;
    }
    break;

  case EFI_RPMB_REQUEST_COUNTER_VALUE:
    Status = RpmbReadCounterRequest (HostInst, Request->Packets, Response->Packets);
    if (EFI_ERROR (Status)) {
      LOG_ERROR ("RpmbReadCounterRequest() failed. (Status = %r)", Status);
      goto Exit;
    }
    break;

  case EFI_RPMB_REQUEST_AUTH_READ:
    Status = RpmbAuthenticatedReadRequest (HostInst, Request->Packets, Response);
    if (EFI_ERROR (Status)) {
      LOG_ERROR ("RpmbAuthenticatedReadRequest() failed. (Status = %r)", Status);
      goto Exit;
    }
    break;

  case EFI_RPMB_REQUEST_AUTH_WRITE:
    Status = RpmbAuthenticatedWriteRequest (HostInst, Request, Response->Packets);
    if (EFI_ERROR (Status)) {
      LOG_ERROR ("RpmbAuthenticatedWriteRequest() failed. (Status = %r)", Status);
      goto Exit;
    }
    break;

  default:

    ASSERT (FALSE);
  }

Exit:

  if (SwitchPartition) {
    SwitchStatus = SdhcSwitchPartitionMmc (HostInst, CurrentPartition);
    if (EFI_ERROR (SwitchStatus)) {

      LOG_ERROR (
        "SdhcSwitchPartitionMmc() failed. (SwitchStatus = %r)",
        SwitchStatus);
    }
  }

  if (EFI_ERROR (Status)) {
    return Status;
  } else {
    return SwitchStatus;
  }
}

/** Authentication key programming request.

  @param[in] This Indicates a pointer to the calling context.
  @param[in] ProgrammingRequest A data packet describing a key programming request.
  @param[out] ResultResponse A caller allocated data packet which will receive the
  key programming result.

  @retval EFI_SUCCESS RPMB communication sequence with the eMMC succeeded
  according to specs, other values are returned in case of any protocol error.
  Failure during key programming any other eMMC internal failure is reported
  in the Result field of the returned response data packet.
**/
EFI_STATUS
EFIAPI
RpmbIoProgramKey (
  IN EFI_RPMB_IO_PROTOCOL   *This,
  IN EFI_RPMB_DATA_PACKET   *ProgrammingRequest,
  OUT EFI_RPMB_DATA_PACKET  *ResultResponse
  )
{
  EFI_STATUS Status;
  EFI_RPMB_DATA_BUFFER RequestBuffer;
  EFI_RPMB_DATA_BUFFER ResponseBuffer;

  LOG_TRACE ("RpmbIoProgramKey()");

  if ((This == NULL) || (ProgrammingRequest == NULL) || (ResultResponse == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (RpmbBytesToUint16 (ProgrammingRequest->RequestOrResponseType) != EFI_RPMB_REQUEST_PROGRAM_KEY) {
    return EFI_INVALID_PARAMETER;
  }

  RequestBuffer.PacketCount = 1;
  RequestBuffer.Packets = ProgrammingRequest;

  ResponseBuffer.PacketCount = 1;
  ResponseBuffer.Packets = ResultResponse;

  Status = RpmbRequest (This, &RequestBuffer, &ResponseBuffer);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("RpmbRequest() failed. (Status = %r)", Status);
    return Status;
  }

  return Status;
}

/** Reading of the write counter value request.

  @param[in] This Indicates a pointer to the calling context.
  @param[in] ReadRequest A data packet describing a read counter value request.
  @param[out] ReadResponse A caller allocated data packet which will receive
  the counter value read response. If counter has expired bit 7 is set to 1 in
  returned Result.

  @retval EFI_SUCCESS RPMB communication sequence with the eMMC succeeded
  according to specs, other values are returned in case of any protocol error.
  Failure during counter read or any other eMMC internal failure is reported
  in the Result field of the returned response data packet.
**/
EFI_STATUS
EFIAPI
RpmbIoReadCounter (
  IN EFI_RPMB_IO_PROTOCOL   *This,
  IN EFI_RPMB_DATA_PACKET   *ReadRequest,
  OUT EFI_RPMB_DATA_PACKET  *ReadResponse
  )
{
  EFI_STATUS Status;
  EFI_RPMB_DATA_BUFFER RequestBuffer;
  EFI_RPMB_DATA_BUFFER ResponseBuffer;

  LOG_TRACE ("RpmbIoReadCounter()");

  if ((This == NULL) || (ReadRequest == NULL) || (ReadResponse == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (RpmbBytesToUint16 (ReadRequest->RequestOrResponseType) != EFI_RPMB_REQUEST_COUNTER_VALUE) {
    return EFI_INVALID_PARAMETER;
  }

  RequestBuffer.PacketCount = 1;
  RequestBuffer.Packets = ReadRequest;

  ResponseBuffer.PacketCount = 1;
  ResponseBuffer.Packets = ReadResponse;

  Status = RpmbRequest (This, &RequestBuffer, &ResponseBuffer);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("RpmbRequest() failed. (Status = %r)", Status);
    return Status;
  }

  return Status;
}

/** Authenticated data write request.

  @param[in] This Indicates a pointer to the calling context.
  @param[in] WriteRequest A sequence of data packets describing data write
  requests and holds the data to be written.
  @param[out] ResultResponse A caller allocated data packet which will receive
  the data write programming result.

  @retval EFI_SUCCESS RPMB communication sequence with the eMMC succeeded
  according to specs, other values are returned in case of any protocol error.
  Failure during data programming or any other eMMC internal failure is reported
  in the Result field of the returned data packet.
**/
EFI_STATUS
EFIAPI
RpmbIoAuthenticatedWrite (
  IN EFI_RPMB_IO_PROTOCOL   *This,
  IN EFI_RPMB_DATA_BUFFER   *Request,
  OUT EFI_RPMB_DATA_PACKET  *Response
  )
{
  EFI_STATUS Status;
  EFI_RPMB_DATA_BUFFER ResponseBuffer;

  LOG_TRACE ("RpmbIoAuthenticatedWrite()");

  if ((This == NULL) || (Request == NULL) || (Response == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Request->Packets == NULL) || (Request->PacketCount == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if (RpmbBytesToUint16 (Request->Packets->RequestOrResponseType) != EFI_RPMB_REQUEST_AUTH_WRITE) {
    return EFI_INVALID_PARAMETER;
  }

  ResponseBuffer.PacketCount = 1;
  ResponseBuffer.Packets = Response;

  Status = RpmbRequest (This, Request, &ResponseBuffer);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("RpmbRequest() failed. (Status = %r)", Status);
    return Status;
  }

  return Status;
}

/** Authenticated data read request.

  @param[in] This Indicates a pointer to the calling context.
  @param[in] ReadRequest A data packet that describes a data read request.
  @param[out] ReadResponse A caller allocated data packets which will receive
  the data read.

  @retval EFI_SUCCESS RPMB communication sequence with the eMMC succeeded
  according to specs, other values are returned in case of any protocol error.
  Failure during data fetch from the eMMC or any other eMMC internal failure
  is reported in the Result field of the returned data packet.
**/
EFI_STATUS
EFIAPI
RpmbIoAuthenticatedRead (
  IN EFI_RPMB_IO_PROTOCOL   *This,
  IN EFI_RPMB_DATA_PACKET   *ReadRequest,
  OUT EFI_RPMB_DATA_BUFFER  *ReadResponse
  )
{
  EFI_STATUS Status;
  EFI_RPMB_DATA_BUFFER RequestBuffer;

  LOG_TRACE ("RpmbIoAuthenticatedRead()");

  if ((This == NULL) || (ReadRequest == NULL) || (ReadResponse == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((ReadResponse->Packets == NULL) || (ReadResponse->PacketCount == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if (RpmbBytesToUint16 (ReadRequest->RequestOrResponseType) != EFI_RPMB_REQUEST_AUTH_READ) {
    return EFI_INVALID_PARAMETER;
  }

  RequestBuffer.PacketCount = 1;
  RequestBuffer.Packets = ReadRequest;

  Status = RpmbRequest (This, &RequestBuffer, ReadResponse);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("RpmbRequest() failed. (Status = %r)", Status);
    return Status;
  }

  return Status;
}