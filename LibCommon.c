/* LibCommon.c -    Collection of functions implementing commonly-used
 *                  functionality.
 *
 * Author: Safayet N Ahmed (GE Global Research) <Safayet.Ahmed@ge.com>
 *
 * Copyright (c) 2016, General Electric Company. All rights reserved.
 *
 */

#include "__LibCommon.h"

EFI_STATUS EFIAPI Common_ReadUEFIVariable(  IN  EFI_GUID*   guid_p,
                                            IN  CHAR16*     name_p,
                                            OUT VOID*       *buffer_p,
                                            OUT UINTN       *buffersize_p,
                                            OUT UINT32      *attrs_p )
{
    EFI_STATUS  Status;
    UINT32  attrs = 0;
    UINTN   buffersize = 0;
    VOID    *buffer = NULL;

    Status = gRT->GetVariable(  name_p, guid_p, &attrs, &buffersize, NULL);
    if( EFI_ERROR(Status)  && (Status != EFI_BUFFER_TOO_SMALL) ){
        attrs = 0;
        buffersize = 0;
        goto exit0;
    }

    Status = gBS->AllocatePool( EfiLoaderData, buffersize, &buffer);
    if( EFI_ERROR(Status) ){
        attrs = 0;
        buffersize = 0;
        goto exit0;
    }

    Status = gRT->GetVariable(  name_p, guid_p, &attrs, &buffersize, buffer);
    if( EFI_ERROR(Status) ){
        gBS->FreePool( buffer );
        attrs = 0;
        buffersize = 0;
        buffer = NULL;
    }

exit0:
    *buffer_p       = buffer;
    *buffersize_p   = buffersize;
    *attrs_p        = attrs;
    return Status;
}


EFI_STATUS EFIAPI Common_GetFileInfo(   IN  EFI_FILE_PROTOCOL   *filep,
                                        OUT EFI_FILE_INFO       **fileinfo_pp,
                                        OUT UINTN               *fileinfosize_p)
{
    EFI_STATUS Status;
    EFI_FILE_INFO	*fileinfo_p = NULL;
    UINTN           fileinfosize = 0;

    /*  call GetFileInfo with a zero-length buffer */
    Status = filep->GetInfo(filep, &gEfiFileInfoGuid,
                            &fileinfosize, fileinfo_p);
    if( EFI_ERROR(Status) && (Status != EFI_BUFFER_TOO_SMALL) )
        goto exit0;

    /*  allocate FileInfo buffer */
    Status = gBS->AllocatePool( EfiLoaderData,
                                fileinfosize, (VOID**)&fileinfo_p );
    if( EFI_ERROR(Status) ){
        fileinfosize = 0;
        goto exit0;
    }

    /*  call GetFileInfo with a propper buffer */
    Status = filep->GetInfo(filep, &gEfiFileInfoGuid,
                            &fileinfosize, fileinfo_p);
    if( EFI_ERROR(Status) ){
        /*  free the allocated buffer */
        gBS->FreePool( fileinfo_p );

        /*  reset the file-info buffer, buffer size */
        fileinfosize = 0;
        fileinfo_p = NULL;
    }

exit0:
    /*  output the file-info buffer, buffer size, and return status */
    *fileinfo_pp = fileinfo_p;
    *fileinfosize_p = fileinfosize;
    return Status;
}


EFI_STATUS EFIAPI Common_ReadFile(  IN EFI_FILE_PROTOCOL    *file_p,
                                    OUT VOID*               *buffer_p,
                                    OUT UINTN               *buffersize_p)
{
    EFI_STATUS Status;
    EFI_FILE_INFO   *fileinfo_p = NULL;
    OUT UINTN       fileinfosize = 0;

    UINTN buffersize = 0;
    VOID* buffer = NULL;

    UINTN readsize = 0;

    /*  get the File Info structure */
    Status = Common_GetFileInfo( file_p, &fileinfo_p, &fileinfosize);
    if( EFI_ERROR(Status) )
        goto exit0;

    /*  validate file parameters */
    if( (fileinfo_p->Attribute & EFI_FILE_DIRECTORY) != 0 )
        goto exit1;

    /*  get the actual file size */
    buffersize = fileinfo_p->FileSize;

    /*  Allocate buffer AllocPool */
    Status = gBS->AllocatePool( EfiLoaderData, buffersize, &buffer);
    if( EFI_ERROR(Status) ){
        buffersize = 0;
        goto exit1;
    }

    /*  Read the file */
    readsize = buffersize;
    Status = file_p->Read(file_p, &readsize, buffer);
    if( EFI_ERROR(Status) || (readsize != buffersize) ){
        /*  If the amount read was less than the amount requested, still mark
            it as an error. */
        if( !EFI_ERROR(Status) )
            Status = EFI_DEVICE_ERROR;

        /*  Free the file-read buffer. */
        gBS->FreePool(buffer);

        /*  Set the output buffer and buffer-size to 0. */
        buffer = NULL;
        buffersize = 0;
    }

exit1:
    /*  free the file-info structure */
    gBS->FreePool(fileinfo_p);
exit0:
    /*  output the buffer address and size and return status */
    *buffer_p = buffer;
    *buffersize_p = buffersize;
    return Status;
}

EFI_STATUS EFIAPI Common_WriteFile( IN EFI_FILE_PROTOCOL *filep,
                                    IN VOID*  buffer,
                                    IN UINTN  buffersize)
{
    EFI_STATUS Status, FlushStatus;
    UINTN bytes_written;

    EFI_FILE_INFO	*fileinfo_p = NULL;
    UINTN           fileinfosize = 0;

    /* get file info */
    Status = Common_GetFileInfo( filep, &fileinfo_p, &fileinfosize);
    if( EFI_ERROR(Status) )
        goto exit0;

    /* truncate file size */
    fileinfo_p->FileSize = 0;
    /* leave other fields the same */
    fileinfo_p->CreateTime = (EFI_TIME){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    fileinfo_p->LastAccessTime = (EFI_TIME){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    fileinfo_p->ModificationTime = (EFI_TIME){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    /* set file info */
    Status = filep->SetInfo(filep, &gEfiFileInfoGuid, fileinfosize, fileinfo_p);

    /* Flush changes to file. */
    FlushStatus = filep->Flush( filep );
    if( EFI_ERROR(FlushStatus) )
        if( ! EFI_ERROR(Status) )
            Status = FlushStatus;

    /* Free the file-info structure */
    gBS->FreePool(fileinfo_p);

    /* If SetInfo or Flush failed, exit*/
    if( EFI_ERROR(Status) )
        goto exit0;

    /* Write out to file */
    bytes_written = buffersize;
    Status = filep->Write( filep, &bytes_written, buffer);
    if( bytes_written != buffersize )
        if( ! EFI_ERROR(Status) )
            Status = EFI_DEVICE_ERROR;

    /* Flush changes to file. */
    FlushStatus = filep->Flush( filep );
    if( EFI_ERROR(FlushStatus) )
        if( ! EFI_ERROR(Status) )
            Status = FlushStatus;

exit0:
    return Status;
}

EFI_STATUS EFIAPI Common_CreateWriteCloseFile(  IN EFI_FILE_PROTOCOL *dir_p,
                                                IN CHAR16 *filename,
                                                IN VOID*  buffer,
                                                IN UINTN  buffersize)
{
    EFI_STATUS Status, CloseStatus;
    EFI_FILE_PROTOCOL *filep;

    /* Open file */
    Status = dir_p->Open( dir_p, &filep, filename,
                            ( EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE
                                | EFI_FILE_MODE_CREATE ), 0 );
    if( EFI_ERROR(Status) ){
        goto exit0;
    }

    /* Write out to file */
    Status = Common_WriteFile( filep, buffer, buffersize);

    /* close file */
    CloseStatus = filep->Close( filep );
    if( EFI_ERROR(CloseStatus) )
        if( ! EFI_ERROR(Status) )
            Status = CloseStatus;
exit0:
    return Status;
}

