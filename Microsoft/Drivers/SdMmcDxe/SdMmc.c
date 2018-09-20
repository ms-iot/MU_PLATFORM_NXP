/** @file
*
*  Copyright (c) Microsoft Corporation. All rights reserved.
*  Copyright (c) 2011-2014, ARM Limited. All rights reserved.
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

// A template EFI_BLOCK_IO media to use for creating new EFI_BLOCK_IO protocols
// for new SDHC instances.
EFI_BLOCK_IO_MEDIA gSdhcMediaTemplate = {
  0,                      // MediaId
  TRUE,                   // RemovableMedia
  FALSE,                  // MediaPresent
  FALSE,                  // LogicalPartition
  FALSE,                  // ReadOnly
  FALSE,                  // WriteCaching
  SD_BLOCK_LENGTH_BYTES,  // BlockSize
  4,                      // IoAlign
  0,                      // Pad
  0                       // LastBlock
};

// This structure is serviced as a header.Its next field points to the first root
// bridge device node.
LIST_ENTRY  gSdhcInstancePool;
UINT32 gNextSdhcInstanceId = 0;

// Event triggered by the timer to check if any cards have been removed
// or if new ones have been plugged in.
EFI_EVENT gCheckCardsEvent;

// The ARM high-performance counter frequency.
UINT64 gHpcTicksPerSeconds = 0;

VOID
EFIAPI
CheckCardsCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

EFI_STATUS
EFIAPI
UninstallAllProtocols (
  IN SDHC_INSTANCE *HostInst
  );

BOOLEAN
EFIAPI
IsRpmbInstalledOnTheSystem (
  VOID
  );

/** Initialize the SDHC Pool to support multiple SD/MMC devices.
**/
VOID
InitializeSdhcPool (
  VOID
  )
{
  InitializeListHead (&gSdhcInstancePool);
}

/** Insert a new SDHC instance in the host pool.

  @param[in] HostInst The SDHC instance context data.
**/
VOID
InsertSdhcInstance (
  IN SDHC_INSTANCE  *HostInst
  )
{
  InsertTailList (&gSdhcInstancePool, &(HostInst->Link));
}

/** Removes an existing SDHC instance context data from the host pool.

  @param[in] HostInst The SDHC instance data.
**/
VOID
RemoveSdhcInstance (
  IN SDHC_INSTANCE  *HostInst
)
{
  RemoveEntryList (&(HostInst->Link));
}

/** Creates a new SDHC instance context data.

  It initializes the context data but does not attempt to perform any hardware
  access nor install any EFI protocol. This happens later on when a card presence
  state changes during the card check callback.

  @param[in] HostExt The EFI_SDHC_PROTOCOL instance data to use as the basis for
  SDHC instance context data creation.

  @retval An SDHC instance context data on successful instance creation and return
  NULL otherwise.
**/
SDHC_INSTANCE*
CreateSdhcInstance (
  IN EFI_SDHC_PROTOCOL  *HostExt
  )
{
  EFI_STATUS Status;
  SDHC_INSTANCE *HostInst = NULL;

  HostInst = AllocateZeroPool (sizeof (SDHC_INSTANCE));
  if (HostInst == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  HostInst->Signature = SDHC_INSTANCE_SIGNATURE;
  HostInst->InstanceId = gNextSdhcInstanceId;
  HostInst->HostExt = HostExt;

  HostExt->GetCapabilities (HostExt, &HostInst->HostCapabilities);
  ASSERT (HostInst->HostCapabilities.MaximumBlockSize > 0);
  ASSERT (HostInst->HostCapabilities.MaximumBlockCount > 0);

  // We will support 512 byte blocks only.
  if (HostInst->HostCapabilities.MaximumBlockSize < SD_BLOCK_LENGTH_BYTES) {
    LOG_ERROR (
      "Unsupported max block size of %d bytes",
      HostInst->HostCapabilities.MaximumBlockSize);
    Status = EFI_UNSUPPORTED;
    goto Exit;
  }

  LOG_TRACE (
    "Host Capabilities: MaximumBlockSize:%d MaximumBlockCount:%d",
    HostInst->HostCapabilities.MaximumBlockSize,
    HostInst->HostCapabilities.MaximumBlockCount);

  HostInst->DevicePathProtocolInstalled = FALSE;
  HostInst->BlockIoProtocolInstalled = FALSE;
  HostInst->RpmbIoProtocolInstalled = FALSE;

  // Initialize BlockIo Protocol.

  HostInst->BlockIo.Media = AllocateCopyPool (sizeof (EFI_BLOCK_IO_MEDIA), &gSdhcMediaTemplate);
  if (HostInst->BlockIo.Media == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  HostInst->BlockIo.Revision = EFI_BLOCK_IO_INTERFACE_REVISION;
  HostInst->BlockIo.Reset = BlockIoReset;
  HostInst->BlockIo.ReadBlocks = BlockIoReadBlocks;
  HostInst->BlockIo.WriteBlocks = BlockIoWriteBlocks;
  HostInst->BlockIo.FlushBlocks = BlockIoFlushBlocks;

  // Initialize DevicePath Protocol.

  SDHC_DEVICE_PATH *DevicePath = &HostInst->DevicePath;

  // Initialize device path based on SDHC DeviceId and delay SlotNode initialization
  // until the card in Slot0 is type identified.

  DevicePath->SdhcNode.Header.Type = HARDWARE_DEVICE_PATH;
  DevicePath->SdhcNode.Header.SubType = HW_VENDOR_DP;
  *((UINT16*) &DevicePath->SdhcNode.Header.Length) = SDHC_NODE_PATH_LENGTH;
  DevicePath->SdhcId = HostInst->HostExt->SdhcId;
  GUID SdhcDevicePathGuid = SDHC_DEVICE_PATH_GUID;
  CopyGuid (&DevicePath->SdhcNode.Guid, &SdhcDevicePathGuid);

  SetDevicePathEndNode (&DevicePath->EndNode);

  // Initialize RpmbIo Protocol.

  HostInst->RpmbIo.Revision = EFI_RPMB_IO_PROTOCOL_REVISION;
  HostInst->RpmbIo.AuthenticatedRead = RpmbIoAuthenticatedRead;
  HostInst->RpmbIo.AuthenticatedWrite = RpmbIoAuthenticatedWrite;
  HostInst->RpmbIo.ProgramKey = RpmbIoProgramKey;
  HostInst->RpmbIo.ReadCounter = RpmbIoReadCounter;

  // Don't publish any protocol yet, until the SDHC device is fully initialized and
  // ready for IO.

  ++gNextSdhcInstanceId;

  Status = EFI_SUCCESS;

Exit:
  if (EFI_ERROR (Status)) {
    if (HostInst != NULL && HostInst->BlockIo.Media != NULL) {
      FreePool (HostInst->BlockIo.Media);
      HostInst->BlockIo.Media = NULL;
    }

    if (HostInst != NULL) {
      FreePool (HostInst);
      HostInst = NULL;
    }
  }

  return HostInst;
}

/** Destroys an existing SDHC instance context data.

  It uninstalls all protocols installed on that instance, free allocated memory
  and invokes the SDHC clean-up callback.

  @param[in] HostInst The SDHC instance context data to destroy.

  @retval EFI_SUCCESS on successful destruction and cleanup, return an EFI error
  code otherwise.
**/
EFI_STATUS
DestroySdhcInstance (
  IN SDHC_INSTANCE  *HostInst
  )
{
  EFI_STATUS Status;

  Status = UninstallAllProtocols (HostInst);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Free Memory allocated for the EFI_BLOCK_IO protocol
  if (HostInst->BlockIo.Media) {
    FreePool (HostInst->BlockIo.Media);
  }

  HostInst->HostExt->Cleanup (HostInst->HostExt);
  HostInst->HostExt = NULL;

  FreePool (HostInst);

  return EFI_SUCCESS;
}

// UEFI Driver Model EFI_DRIVER_BINDING_PROTOCOL Callbacks

/**
    Tests to see if this driver supports a given controller. If a child device is provided,
    it further tests to see if this driver supports creating a handle for the specified child device.

    @param This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
    @param ControllerHandle     The handle of the controller to test. This handle must support a protocol
                                interface that supplies an I/O abstraction to the driver. Sometimes
                                just the presence of this I/O abstraction is enough for the driver to
                                determine if it supports ControllerHandle. Sometimes, the driver may
                                use the services of the I/O abstraction to determine if this driver
                                supports ControllerHandle.
    @param RemainingDevicePath  A pointer to the remaining portion of a device path. For bus drivers,
                                if this parameter is not NULL, then the bus driver must determine if
                                the bus controller specified by ControllerHandle and the child controller
                                specified by RemainingDevicePath
**/
EFI_STATUS
EFIAPI
SdMmcDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS Status;
  EFI_SDHC_PROTOCOL *HostExt;
  EFI_DEV_PATH_PTR Node;

  // Check RemainingDevicePath validation.
  if (RemainingDevicePath != NULL) {

    // Check if RemainingDevicePath is the End of Device Path Node,
    // if yes, go on checking other conditions.
    if (!IsDevicePathEnd (RemainingDevicePath)) {

      // If RemainingDevicePath isn't the End of Device Path Node,
      // check its validation.

      Node.DevPath = RemainingDevicePath;
      if (Node.DevPath->Type != HARDWARE_DEVICE_PATH ||
          Node.DevPath->SubType != HW_VENDOR_DP ||
          DevicePathNodeLength (Node.DevPath) != sizeof (VENDOR_DEVICE_PATH)) {
        Status = EFI_UNSUPPORTED;
        goto Exit;
      }
    }
  }

  // Check if SDHC protocol is installed by platform.

  Status = gBS->OpenProtocol (
    Controller,
    &gEfiSdhcProtocolGuid,
    (VOID **) &HostExt,
    This->DriverBindingHandle,
    Controller,
    EFI_OPEN_PROTOCOL_BY_DRIVER);
  if (Status == EFI_ALREADY_STARTED) {
    Status = EFI_SUCCESS;
    goto Exit;
  }

  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  // Close the SDHC used to perform the supported test.
  gBS->CloseProtocol (
    Controller,
    &gEfiSdhcProtocolGuid,
    This->DriverBindingHandle,
    Controller);

Exit:
  return Status;
}

/**
    Starts a device controller or a bus controller. The Start() and Stop() services of the
    EFI_DRIVER_BINDING_PROTOCOL mirror each other.

    @param This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
    @param ControllerHandle     The handle of the controller to start. This handle must support a
                                protocol interface that supplies an I/O abstraction to the driver.
    @param RemainingDevicePath  A pointer to the remaining portion of a device path. For a bus driver,
                                if this parameter is NULL, then handles for all the children of Controller
                                are created by this driver.
                                If this parameter is not NULL and the first Device Path Node is not
                                the End of Device Path Node, then only the handle for the child device
                                specified by the first Device Path Node of RemainingDevicePath is created
                                by this driver.
                                If the first Device Path Node of RemainingDevicePath is the End of Device
                                Path Node, no child handle is created by this driver.
**/
EFI_STATUS
EFIAPI
SdMmcDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS Status;
  SDHC_INSTANCE *HostInst;
  EFI_SDHC_PROTOCOL *HostExt;

  LOG_TRACE ("SdMmcDriverStart()");

  // Check RemainingDevicePath validation.
  if (RemainingDevicePath != NULL) {

    // Check if RemainingDevicePath is the End of Device Path Node,
    // if yes, return EFI_SUCCESS.
    if (IsDevicePathEnd (RemainingDevicePath)) {
      Status = EFI_SUCCESS;
      goto Exit;
    }
  }

  // Get the SDHC protocol.

  Status = gBS->OpenProtocol (
    Controller,
    &gEfiSdhcProtocolGuid,
    (VOID **) &HostExt,
    This->DriverBindingHandle,
    Controller,
    EFI_OPEN_PROTOCOL_BY_DRIVER);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_ALREADY_STARTED) {
      Status = EFI_SUCCESS;
    }
    goto Exit;
  }

  HostInst = CreateSdhcInstance (HostExt);
  if (HostInst != NULL) {
    HostInst->MmcHandle = Controller;
    InsertSdhcInstance (HostInst);

    LOG_INFO (
      "SDHC%d instance creation completed. Detecting card presence...",
      HostInst->HostExt->SdhcId);

    // Detect card presence now which will initialize the SDHC.
    CheckCardsCallback (NULL, NULL);
  } else {
    LOG_ERROR ("CreateSdhcInstance failed. %r", Status);
  }

Exit:
  return Status;
}

/**
    Stops a device controller or a bus controller. The Start() and Stop() services of the
    EFI_DRIVER_BINDING_PROTOCOL mirror each other.

    @param This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
                                Type EFI_DRIVER_BINDING_PROTOCOL is defined in Section 10.1.
    @param ControllerHandle     A handle to the device being stopped. The handle must support a bus
                                specific I/O protocol for the driver to use to stop the device.
    @param NumberOfChildren     The number of child device handles in ChildHandleBuffer.
    @param ChildHandleBuffer    An array of child handles to be freed. May be NULL if NumberOfChildren is 0.
**/
EFI_STATUS
EFIAPI
SdMmcDriverStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS Status = EFI_SUCCESS;
  LIST_ENTRY *CurrentLink;
  SDHC_INSTANCE *HostInst;

  LOG_TRACE ("SdMmcDriverStop()");

  // For each registered SDHC instance.
  CurrentLink = gSdhcInstancePool.ForwardLink;
  while (CurrentLink != NULL && CurrentLink != &gSdhcInstancePool && (Status == EFI_SUCCESS)) {
    HostInst = SDHC_INSTANCE_FROM_LINK (CurrentLink);
    ASSERT (HostInst != NULL);

    // Close gEfiMmcHostProtocolGuid opened on the SDHC.
    Status = gBS->CloseProtocol (
      Controller,
      &gEfiSdhcProtocolGuid,
      (VOID **) &HostInst->HostExt,
      This->DriverBindingHandle);

    // Remove SDHC instance from the pool.
    RemoveSdhcInstance (HostInst);

    // Destroy SDHC instance.
    DestroySdhcInstance (HostInst);
  }

  return Status;
}

EFI_STATUS
EFIAPI
SoftReset (
  IN SDHC_INSTANCE  *HostInst
  )
{
  EFI_SDHC_PROTOCOL *HostExt = HostInst->HostExt;
  EFI_STATUS Status;
  CHAR16 *DevicePathText = NULL;

  ASSERT (HostInst->HostExt != NULL);
  LOG_TRACE ("Performing Soft-Reset for SDHC%d", HostExt->SdhcId);

  Status = UninstallAllProtocols (HostInst);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  HostInst->SlotInitialized = FALSE;

  // Clear all media settings regardless of card presence.
  HostInst->BlockIo.Media->MediaId = 0;
  HostInst->BlockIo.Media->RemovableMedia = FALSE;
  HostInst->BlockIo.Media->MediaPresent = FALSE;
  HostInst->BlockIo.Media->LogicalPartition = FALSE;
  HostInst->BlockIo.Media->WriteCaching = FALSE;
  HostInst->BlockIo.Media->BlockSize = 0;
  HostInst->BlockIo.Media->IoAlign = 4;
  HostInst->BlockIo.Media->LastBlock = 0;
  HostInst->BlockIo.Media->LowestAlignedLba = 0;
  HostInst->BlockIo.Media->LogicalBlocksPerPhysicalBlock = 0;
  HostInst->BlockIo.Media->OptimalTransferLengthGranularity = 0;
  HostInst->BlockIo.Media->ReadOnly = FALSE;

  HostInst->RpmbIo.ReliableSectorCount = 0;
  HostInst->RpmbIo.RpmbSizeMult = 0;
  ZeroMem (HostInst->RpmbIo.Cid, sizeof (HostInst->RpmbIo.Cid));

  HostInst->BlockIo.Media->MediaPresent = HostInst->HostExt->IsCardPresent (HostInst->HostExt);
  if (!HostInst->BlockIo.Media->MediaPresent) {
    // Even if the media is not present, we`d like to communicate that status up
    // to the storage stack by means of installing the BlockIo protocol.
    Status =
      gBS->InstallMultipleProtocolInterfaces (
        &HostInst->MmcHandle,
        &gEfiBlockIoProtocolGuid,
        &HostInst->BlockIo,
        NULL);

    if (EFI_ERROR (Status)) {
      LOG_ERROR (
        "SoftReset(): Failed installing EFI_BLOCK_IO_PROTOCOL interface. %r",
        Status);

      goto Exit;
    }

    HostInst->BlockIoProtocolInstalled = TRUE;
    LOG_INFO ("SDHC%d media not present, skipping device initialization", HostExt->SdhcId);
    goto Exit;
  } else {
    HostInst->BlockIo.Media->ReadOnly = HostExt->IsReadOnly (HostExt);
    if (HostInst->BlockIo.Media->ReadOnly) {
      LOG_INFO ("SDHC%d media is read-only", HostExt->SdhcId);
    }
  }

  Status = InitializeDevice (HostInst);
  if (EFI_ERROR (Status)) {
    LOG_ERROR ("SoftReset(): InitializeDevice() failed. %r", Status);
    goto Exit;
  }

  // Update SlotNode subtype based on the card type identified.
  // Note that we only support 1 slot per SDHC, i.e It is assumed that no more
  // than 1 SD/MMC card connected to the same SDHC block on the system.
  SDHC_DEVICE_PATH *DevicePath = &HostInst->DevicePath;
  if (HostInst->CardInfo.CardFunction == CardFunctionSd) {
    DevicePath->SlotNode.SD.Header.Type = MESSAGING_DEVICE_PATH;
    DevicePath->SlotNode.SD.Header.SubType = MSG_SD_DP;
    *((UINT16*) &DevicePath->SlotNode.SD.Header.Length) = sizeof (SD_DEVICE_PATH);
    DevicePath->SlotNode.SD.SlotNumber = 0;
  } else {
    ASSERT (HostInst->CardInfo.CardFunction == CardFunctionMmc);
    DevicePath->SlotNode.MMC.Header.Type = MESSAGING_DEVICE_PATH;
    DevicePath->SlotNode.MMC.Header.SubType = MSG_EMMC_DP;
    *((UINT16*) &DevicePath->SlotNode.MMC.Header.Length) = sizeof (EMMC_DEVICE_PATH);
    DevicePath->SlotNode.MMC.SlotNumber = 0;
  }

  // Print a text representation of the Slot device path.
  DevicePathText = ConvertDevicePathToText ((EFI_DEVICE_PATH_PROTOCOL*) DevicePath, FALSE, FALSE);

  if (DevicePathText == NULL) {
    LOG_ERROR ("SoftReset(): ConvertDevicePathToText() failed.");
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  LOG_INFO ("Recognized device: %s", DevicePathText);

  Status =
    gBS->InstallMultipleProtocolInterfaces (
      &HostInst->MmcHandle,
      &gEfiBlockIoProtocolGuid,
      &HostInst->BlockIo,
      NULL);

  if (EFI_ERROR (Status)) {
    LOG_ERROR (
      "SoftReset(): Failed installing EFI_BLOCK_IO_PROTOCOL interface. %r",
      Status);

    goto Exit;
  }

  HostInst->BlockIoProtocolInstalled = TRUE;

  Status =
    gBS->InstallMultipleProtocolInterfaces (
      &HostInst->MmcHandle,
      &gEfiDevicePathProtocolGuid,
      &HostInst->DevicePath,
      NULL);

  if (EFI_ERROR (Status)) {
    LOG_ERROR (
      "SoftReset(): Failed installing EFI_DEVICE_PATH_PROTOCOL interface. %r",
      Status);

    goto Exit;
  }

  HostInst->DevicePathProtocolInstalled = TRUE;

  if (HostInst->CardInfo.CardFunction == CardFunctionMmc) {

    if (!IsRpmbInstalledOnTheSystem ()) {
      Status =
        gBS->InstallMultipleProtocolInterfaces (
          &HostInst->MmcHandle,
          &gEfiRpmbIoProtocolGuid,
          &HostInst->RpmbIo,
          NULL);

      if (EFI_ERROR (Status)) {
        LOG_ERROR (
          "SoftReset(): Failed installing EFI_RPMB_IO_PROTOCOL interface. %r",
          Status);

        goto Exit;
      }

      LOG_INFO ("RpmbIo protocol installed for MMC: %s", DevicePathText);

      HostInst->RpmbIoProtocolInstalled = TRUE;

      // Track current partition in a separate variable to void having to
      // ready the MMC EXT_CSD everytime we do partition switch to have
      // an updated current partition. SdhostSwitchPartitionMmc will keep
      // track of that variable.

      MMC_EXT_CSD_PARTITION_CONFIG partConfig;
      partConfig.AsUint8 = HostInst->CardInfo.Registers.Mmc.ExtCsd.PartitionConfig;
      HostInst->CurrentMmcPartition =
        (MMC_EXT_CSD_PARTITION_ACCESS) partConfig.Fields.PARTITION_ACCESS;

    } else {
      LOG_ERROR (
        "SoftReset(): RpmbIo protocol is already installed on the system. "
        "The requirement is that only 1 MMC on the system should have RpmbIo "
        "protocol installed. Skipping RpmbIo protocol installation for %s",
        DevicePathText);
    }
  }

  LOG_TRACE ("All required protocols installed successfully for %s", DevicePathText);

Exit:

  // On error we should uninstall all protocols except BlockIo, it should
  // always be present since it represents the SDHC physical existance not
  // the card, other protocols are card dependant and are pure representation
  // of one or more card features
  if (EFI_ERROR (Status)) {
    if (HostInst->RpmbIoProtocolInstalled) {
      Status =
        gBS->UninstallMultipleProtocolInterfaces (
          HostInst->MmcHandle,
          &gEfiRpmbIoProtocolGuid,
          &HostInst->RpmbIo,
          NULL);

      if (EFI_ERROR (Status)) {
        LOG_ERROR (
          "SoftReset(): Failed to uninstall EFI_RPMB_IO_PROTOCOL interface. %r",
          Status);
      } else {
        HostInst->RpmbIoProtocolInstalled = FALSE;
      }
    }

    if (HostInst->DevicePathProtocolInstalled) {
      Status =
        gBS->UninstallMultipleProtocolInterfaces (
          HostInst->MmcHandle,
          &gEfiDevicePathProtocolGuid,
          &HostInst->DevicePath,
          NULL);

      if (EFI_ERROR (Status)) {
        LOG_ERROR (
          "SoftReset(): Failed to uninstall EFI_DEVICE_PATH_PROTOCOL interface. %r",
          Status);
      } else {
        HostInst->DevicePathProtocolInstalled = FALSE;
      }
    }
  }

  if (DevicePathText != NULL) {
    FreePool (DevicePathText);
  }

  return Status;
}

VOID
EFIAPI
CheckCardsCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  LIST_ENTRY *CurrentLink;
  SDHC_INSTANCE *HostInst;
  EFI_STATUS Status;
  BOOLEAN IsCardPresent;
  BOOLEAN CardEjected;
  BOOLEAN CardInserted;

  // For each registered SDHC instance
  CurrentLink = gSdhcInstancePool.ForwardLink;
  while (CurrentLink != NULL && CurrentLink != &gSdhcInstancePool) {
    HostInst = SDHC_INSTANCE_FROM_LINK (CurrentLink);
    ASSERT (HostInst != NULL);

    IsCardPresent = HostInst->HostExt->IsCardPresent (HostInst->HostExt);

    // If card is present and not initialized or card no more present but was previously
    // initialized, then reset the instance
    //
    // Present Initialized  Outcome
    // T       T            No action
    // T       F            Reset
    // F       T            Reset
    // F       F            No action

    CardEjected = HostInst->BlockIo.Media->MediaPresent && !IsCardPresent;
    if (CardEjected) {
      LOG_INFO ("Card ejected from SDHC%d slot", HostInst->HostExt->SdhcId);
    }

    CardInserted = !HostInst->BlockIo.Media->MediaPresent && IsCardPresent;
    if (CardInserted) {
      LOG_INFO ("Card inserted into SDHC%d slot", HostInst->HostExt->SdhcId);
    }

    if (CardEjected || CardInserted) {
      Status = SoftReset (HostInst);
      if (EFI_ERROR (Status)) {
        LOG_ERROR ("SoftReset() failed. %r", Status);
      }
    }

    CurrentLink = CurrentLink->ForwardLink;
  }
}

BOOLEAN
EFIAPI
IsRpmbInstalledOnTheSystem (
  VOID
  )
{
  EFI_STATUS Status;
  EFI_RPMB_IO_PROTOCOL rpmbIo;

  Status = gBS->LocateProtocol (
    &gEfiRpmbIoProtocolGuid,
    NULL,
    (VOID **) &rpmbIo);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  return TRUE;
}

EFI_STATUS
EFIAPI
UninstallAllProtocols (
  IN SDHC_INSTANCE  *HostInst
  )
{
  EFI_STATUS Status;

  LOG_TRACE ("Uninstalling SDHC%d all protocols", HostInst->HostExt->SdhcId);

  if (HostInst->BlockIoProtocolInstalled) {
    Status =
      gBS->UninstallMultipleProtocolInterfaces (
        HostInst->MmcHandle,
        &gEfiBlockIoProtocolGuid,
        &HostInst->BlockIo,
        NULL);

    if (EFI_ERROR (Status)) {
      LOG_ERROR (
        "UninstallAllProtocols(): Failed to uninstall EFI_BLOCK_IO_PROTOCOL. "
        "(Status = %r)",
        Status);

      return Status;
    }

    HostInst->BlockIoProtocolInstalled = FALSE;
  }

  if (HostInst->RpmbIoProtocolInstalled) {
    Status =
      gBS->UninstallMultipleProtocolInterfaces (
        HostInst->MmcHandle,
        &gEfiRpmbIoProtocolGuid,
        &HostInst->RpmbIo,
        NULL);

    if (EFI_ERROR (Status)) {
      LOG_ERROR (
        "UninstallAllProtocols(): Failed to uninstall EFI_RPMB_IO_PROTOCOL. "
        "(Status = %r)",
        Status);

      return Status;
    }

    HostInst->RpmbIoProtocolInstalled = FALSE;
  }

  if (HostInst->DevicePathProtocolInstalled) {
    Status =
      gBS->UninstallMultipleProtocolInterfaces (
        HostInst->MmcHandle,
        &gEfiDevicePathProtocolGuid,
        &HostInst->DevicePath,
        NULL);

    if (EFI_ERROR (Status)) {
      LOG_ERROR (
        "UninstallAllProtocols(): Failed to uninstall EFI_DEVICE_PATH_PROTOCOL. "
        "(Status = %r)",
        Status);

      return Status;
    }

    HostInst->DevicePathProtocolInstalled = FALSE;
  }

  return EFI_SUCCESS;
}

EFI_DRIVER_BINDING_PROTOCOL gSdMmcDriverBindingCallbacks = {
  SdMmcDriverSupported,
  SdMmcDriverStart,
  SdMmcDriverStop,
  0xa,
  NULL,
  NULL
};

EFI_STATUS
EFIAPI
SdMmcDxeInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS Status;

  LOG_TRACE ("SdMmcDxeInitialize");

  InitializeSdhcPool ();

  // Install driver model protocols.
  Status = EfiLibInstallDriverBindingComponentName2 (
    ImageHandle,
    SystemTable,
    &gSdMmcDriverBindingCallbacks,
    ImageHandle,
    NULL,
    NULL);
  ASSERT_EFI_ERROR (Status);

  // Use a timer to detect if a card has been plugged in or removed.
  Status = gBS->CreateEvent (
    EVT_NOTIFY_SIGNAL | EVT_TIMER,
    TPL_CALLBACK,
    CheckCardsCallback,
    NULL,
    &gCheckCardsEvent);
  ASSERT_EFI_ERROR (Status);

  Status = gBS->SetTimer (
    gCheckCardsEvent,
    TimerPeriodic,
    (UINT64) (10 * 1000 * SDMMC_CHECK_CARD_INTERVAL_MS)); // 200 ms
  ASSERT_EFI_ERROR (Status);

  gHpcTicksPerSeconds = GetPerformanceCounterProperties (NULL, NULL);
  ASSERT (gHpcTicksPerSeconds != 0);

  return Status;
}
