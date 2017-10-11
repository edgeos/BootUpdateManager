/* TestPayload.c -  Simple EFI application for printing image-file path and
 *                  image arguments. Serves as a test EFI payload for the BUM
 *
 * Author: Safayet N Ahmed (GE Global Research) <Safayet.Ahmed@ge.com>
 *
 * Copyright (c) 2017, General Electric Company. All rights reserved.
 */
#include <Library/BaseLib.h>
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>

#include <Protocol/LoadedImage.h>

EFI_STATUS getLoadedImageParams(IN  EFI_HANDLE       LoadedImageHandle,
                                OUT CHAR16*          *ImageName_p,
                                OUT CHAR16*          *ImageArgs_p)
{
    EFI_STATUS ret;
    EFI_LOADED_IMAGE_PROTOCOL *LoadedImageProtocol = NULL;
    ret = gBS->OpenProtocol(LoadedImageHandle,
                            &gEfiLoadedImageProtocolGuid,
                            (VOID**)&LoadedImageProtocol,
                            LoadedImageHandle,
                            NULL,
                            EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if(EFI_ERROR(ret)){
        Print(  L"getLoadedImageParams: gBS->OpenProtocol failed for "
                L"LoadedImageProtocol");
    }else{
        *ImageName_p = ConvertDevicePathToText( LoadedImageProtocol->FilePath,
                                                FALSE,
                                                FALSE);
        if( (((UINTN)(LoadedImageProtocol->LoadOptions) & 1) == 0) &&
            (LoadedImageProtocol->LoadOptionsSize > sizeof(CHAR16)) &&
            ((LoadedImageProtocol->LoadOptionsSize & 1) == 0))
            *ImageArgs_p = (CHAR16*)LoadedImageProtocol->LoadOptions;
        else
            *ImageArgs_p = (CHAR16*)NULL;
    }
    return ret;
}

EFI_STATUS EFIAPI TestPayload_main( IN EFI_HANDLE       LoadedImageHandle,
                                    IN EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS ret;
    CHAR16 *ImageName, *ImageArgs;
    ret = getLoadedImageParams( LoadedImageHandle,
                                &ImageName,
                                &ImageArgs);
    if(EFI_ERROR(ret)){
        ImageName = L"Unknown";
        ImageArgs = L"Unknown";
    }else{
        ImageName = (NULL == ImageName)? L"Unknown" : ImageName;
        ImageArgs = (NULL == ImageArgs)? L"Unknown" : ImageArgs;
    }
    Print(L"\n");
    Print(L"****************************************\n");
    Print(L"    Test Paylod\n");
    Print(L"****************************************\n");
    Print(L"\n");
    Print(L"        Image Name: %s\n", ImageName);
    Print(L"        Image Args: %s\n", ImageArgs);
    Print(L"\n");
    Print(L"****************************************\n");
    Print(L"\n");
    return EFI_SUCCESS;
}

