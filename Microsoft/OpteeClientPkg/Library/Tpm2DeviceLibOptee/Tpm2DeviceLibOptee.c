/** @file
  Ihis library is TPM2 TREE protocol lib.

Copyright (c) 2015, Microsoft Corporation. All rights reserved. <BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Library/OpteeClientApiLib.h>
#include <Library/tee_client_api.h>

#include <Protocol/TrEEProtocol.h>
#include <Protocol/RpmbIo.h>

#include <Guid/OpteeTrustedAppGuids.h>

#include <IndustryStandard/Tpm20.h>
#include <IndustryStandard/Tpm2Acpi.h>

#define FTPM_PRINT_ERROR(...) DEBUG((DEBUG_ERROR, __VA_ARGS__))
#define FTPM_PRINT(...) DEBUG((DEBUG_INFO, __VA_ARGS__))

#define TA_FTPM_SUBMIT_COMMAND (0)

static BOOLEAN Initialized = FALSE;
static TEEC_Context FtpmContext;
static TEEC_Session FtpmSession;

/**
  This service enables the sending of commands to the TPM2.

  @param[in]      InputParameterBlockSize  Size of the TPM2 input parameter block.
  @param[in]      InputParameterBlock      Pointer to the TPM2 input parameter block.
  @param[in,out]  OutputParameterBlockSize Size of the TPM2 output parameter block.
  @param[in]      OutputParameterBlock     Pointer to the TPM2 output parameter block.

  @retval EFI_SUCCESS            The command byte stream was successfully sent to the device and a response was successfully received.
  @retval EFI_DEVICE_ERROR       The command was not successfully sent to the device or a response was not successfully received from the device.
  @retval EFI_BUFFER_TOO_SMALL   The output parameter block is too small. 
**/
EFI_STATUS
EFIAPI
Tpm2SubmitCommand (
  IN UINT32            InputParameterBlockSize,
  IN UINT8             *InputParameterBlock,
  IN OUT UINT32        *OutputParameterBlockSize,
  IN UINT8             *OutputParameterBlock
  )
{
  EFI_STATUS EfiStatus;
  uint32_t ErrorOrigin;
  BOOLEAN ReleaseMemory;
  UINT8 *SharedInput;
  UINT8 *SharedOutput;
  TEEC_Operation TeecOperation = {0};
  TEEC_Result TeecResult;
  TEEC_SharedMemory TeecSharedMem = {0};

  FTPM_PRINT("%a: Called\n", __func__);

  ReleaseMemory = FALSE;
  EfiStatus = EFI_DEVICE_ERROR;
  if (Initialized == FALSE) {
    EfiStatus = EFI_NOT_READY;
    goto Tpm2SubmitCommandEnd;
  }

  TeecSharedMem.size = InputParameterBlockSize + *OutputParameterBlockSize;
  TeecResult = TEEC_AllocateSharedMemory(&FtpmContext, &TeecSharedMem);
  if (TeecResult != TEEC_SUCCESS) {
    ASSERT(FALSE);
    if (TeecResult == TEEC_ERROR_OUT_OF_MEMORY) {
      EfiStatus = EFI_OUT_OF_RESOURCES;
    }

    goto Tpm2SubmitCommandEnd;
  }

  ReleaseMemory = TRUE;
  SharedInput = (UINT8 *)TeecSharedMem.buffer;
  SharedOutput = SharedInput + InputParameterBlockSize;
  CopyMem(SharedInput, InputParameterBlock, InputParameterBlockSize);

  TeecOperation.params[0].tmpref.buffer = SharedInput;
  TeecOperation.params[0].tmpref.size = InputParameterBlockSize;
  TeecOperation.params[1].tmpref.buffer = SharedOutput;
  TeecOperation.params[1].tmpref.size = *OutputParameterBlockSize;
  TeecOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
                                              TEEC_MEMREF_TEMP_INOUT,
                                              TEEC_NONE,
                                              TEEC_NONE);

  //
  // Send the command to the OpTEE FTPM TA.
  //
  
  TeecResult = TEEC_InvokeCommand(&FtpmSession,
                                  TA_FTPM_SUBMIT_COMMAND, // TODO: Used a shared header file
                                  &TeecOperation,
                                  &ErrorOrigin);

  FTPM_PRINT("%a: InvokeCommand: Result 0x%X 0x%X, Response Size %d\n",
             __func__, TeecResult, ErrorOrigin,
             TeecOperation.params[1].memref.size);

  if (TeecResult != TEEC_SUCCESS) {
    ASSERT(FALSE);
    goto Tpm2SubmitCommandEnd;
  }

  CopyMem(OutputParameterBlock, SharedOutput, TeecOperation.params[1].memref.size);
  *OutputParameterBlockSize = TeecOperation.params[1].memref.size;
  EfiStatus = EFI_SUCCESS;

Tpm2SubmitCommandEnd:
  if (ReleaseMemory) {
    TEEC_ReleaseSharedMemory(&TeecSharedMem);
  }

  return EfiStatus;
}

/**
  This service requests use TPM2.

  @retval EFI_SUCCESS      Get the control of TPM2 chip.
  @retval EFI_NOT_FOUND    TPM2 not found.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
Tpm2RequestUseTpm (
  VOID
  )
{
  EFI_STATUS EfiStatus;
  uint32_t ErrorOrigin;
  TEEC_Result TeecResult;
  TEEC_UUID TeecUuid;

  CopyMem (&TeecUuid, &gOpteeFtpmTaGuid, sizeof (TeecUuid));

  TeecUuid.timeLow = SwapBytes32 (TeecUuid.timeLow);
  TeecUuid.timeMid = SwapBytes16 (TeecUuid.timeMid);
  TeecUuid.timeHiAndVersion = SwapBytes16 (TeecUuid.timeHiAndVersion);

  EfiStatus = EFI_DEVICE_ERROR;
  TeecResult = TEEC_InitializeContext(NULL, &FtpmContext);
  ASSERT(TeecResult == TEEC_SUCCESS);

  TeecResult = TEEC_OpenSession(&FtpmContext,
                                &FtpmSession,
                                &TeecUuid,
                                TEEC_LOGIN_PUBLIC,
                                NULL,
                                NULL,
                                &ErrorOrigin);

  FTPM_PRINT("%a: OpenSession returned 0x%X, 0x%X\n",
             __func__, TeecResult, ErrorOrigin);

  if (TeecResult != TEEC_SUCCESS) {
    ASSERT(FALSE);
    goto Tpm2RequestUseTpmEnd;
  }

  Initialized = TRUE;
  EfiStatus = EFI_SUCCESS;

Tpm2RequestUseTpmEnd:
  return EfiStatus;
}

/**
  This service indicates that it will no longer use the TPM2. Future use
  requires another call to Tpm2RequestUseTpm.
**/
VOID
EFIAPI
Tpm2RelinquishUseTpm (
  VOID
  )
{
  //
  // Close session & destroy the context
  //

  if (Initialized != FALSE) {
    FTPM_PRINT("%a: closing session and finalizing context\n", __func__);
    TEEC_CloseSession(&FtpmSession);
    TEEC_FinalizeContext(&FtpmContext);
    Initialized = FALSE;
  }

  OpteeClientApiFinalize ();
}

/**
  This service register TPM2 device.

  @param Tpm2Device  TPM2 device

  @retval EFI_SUCCESS          This TPM2 device is registered successfully.
  @retval EFI_UNSUPPORTED      System does not support register this TPM2 device.
  @retval EFI_ALREADY_STARTED  System already register this TPM2 device.
**/
EFI_STATUS
EFIAPI
Tpm2RegisterTpm2DeviceLib (
  IN TPM2_DEVICE_INTERFACE   *Tpm2Device
  )
{
  ASSERT(FALSE);

  return EFI_UNSUPPORTED;
}

/**
**/
static
VOID
EFIAPI
Tpm2AcpiControlAreaInit(
  IN EFI_HANDLE ImageHandle
  )
{
  EFI_PHYSICAL_ADDRESS BaseAddress;
  UINT32 BufferSize;
  EFI_TPM2_ACPI_CONTROL_AREA *ControlArea;

  BaseAddress = PcdGet64(PcdTpm2AcpiBufferBase);
  BufferSize = PcdGet32(PcdTpm2AcpiBufferSize);

  FTPM_PRINT_ERROR("%a: About to write control area.\n", __func__);

  ControlArea = (EFI_TPM2_ACPI_CONTROL_AREA *)((UINTN)BaseAddress);
  ZeroMem(ControlArea, sizeof(EFI_TPM2_ACPI_CONTROL_AREA));
  BufferSize = 4096;
  ControlArea->Command = (UINT64)((UINTN)(ControlArea + 1));
  ControlArea->CommandSize = BufferSize;
  ControlArea->Response = ControlArea->Command + BufferSize;
  ControlArea->ResponseSize = BufferSize;
}

/**
  Initialize the OpTEE Client Lib. This needs to be done in the constructor 

  @param[in]  ImageHandle   ImageHandle of the loaded driver.
  @param[in]  SystemTable   Pointer to the EFI System Table.

  @retval EFI_SUCCESS   The handlers were registered successfully.
**/
EFI_STATUS
EFIAPI
Tpm2DeviceLibOpTEEEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS Status;

  FTPM_PRINT("%a: Entry\n", __func__);

  Tpm2AcpiControlAreaInit(ImageHandle);
  
  Status = OpteeClientApiInitialize(ImageHandle);
  if (EFI_ERROR(Status) != FALSE) {
    FTPM_PRINT_ERROR("%a: Failed to init OpTEE Lib %x\n",
                     __func__, Status);
  }

  return Status;
}



