
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/ButtonServices.h>


/*
Get button vol up state when the power button was pressed
*/
EFI_STATUS
EFIAPI
PreBootVolumeUpButtonThenPowerButtonCheck(
    IN  MS_BUTTON_SERVICES_PROTOCOL *This,
    OUT BOOLEAN                     *PreBootVolumeUpButtonThenPowerButton // TRUE if button combo set else FALSE
)
{
    DEBUG((DEBUG_ERROR, "%a \n", __FUNCTION__));
    *PreBootVolumeUpButtonThenPowerButton = TRUE;
    return EFI_SUCCESS;
}


/*
Get button vol down state when the power button was pressed
*/
EFI_STATUS
EFIAPI
PreBootVolumeDownButtonThenPowerButtonCheck(
    IN  MS_BUTTON_SERVICES_PROTOCOL *This,
    OUT BOOLEAN *PreBootVolumeDownButtonThenPowerButton // TRUE if button combo set else FALSE
)
{
    DEBUG((DEBUG_ERROR, "%a \n", __FUNCTION__));
    *PreBootVolumeDownButtonThenPowerButton = FALSE; // default to not pressed
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PreBootClearVolumeButtonState(
    MS_BUTTON_SERVICES_PROTOCOL *This
) 
{
    DEBUG((DEBUG_ERROR, "%a \n", __FUNCTION__));
    return EFI_SUCCESS;
}

/**
 Init routine to install protocol and init anything related to buttons

 **/
EFI_STATUS
EFIAPI
ButtonsInit(
    IN EFI_HANDLE                   ImageHandle,
    IN EFI_SYSTEM_TABLE             *SystemTable
)
{
    MS_BUTTON_SERVICES_PROTOCOL* Protocol = NULL;
    EFI_STATUS Status = EFI_SUCCESS;
    DEBUG((DEBUG_ERROR, "%a \n", __FUNCTION__));

    Protocol = AllocateZeroPool(sizeof(MS_BUTTON_SERVICES_PROTOCOL));
    if (Protocol == NULL)
    {
        DEBUG((DEBUG_ERROR, "Failed to allocate memory for button service protocol.\n"));
        return EFI_OUT_OF_RESOURCES;
    }

    Protocol->PreBootVolumeDownButtonThenPowerButtonCheck = PreBootVolumeDownButtonThenPowerButtonCheck;
    Protocol->PreBootVolumeUpButtonThenPowerButtonCheck = PreBootVolumeUpButtonThenPowerButtonCheck;
    Protocol->PreBootClearVolumeButtonState = PreBootClearVolumeButtonState;

    // Install the protocol
    Status = gBS->InstallMultipleProtocolInterfaces(&ImageHandle,
        &gMsButtonServicesProtocolGuid,
        Protocol,
        NULL
        );

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Button Services Protocol Publisher: install protocol error, Status = %r.\n", Status));
        return Status;
    }

    DEBUG((DEBUG_INFO, "Button Services Protocol Installed!\n"));
    return Status;
}
