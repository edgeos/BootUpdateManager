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

static EFI_FILE_PROTOCOL *sgBootPart_RootDir = NULL;

EFI_STATUS EFIAPI Common_FileOpsInit(   EFI_HANDLE  BootPartHandle)
{
    EFI_STATUS Status;

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SimpleFileSystem;

    /*  Find the EFI_SIMPLE_FILE_SYSTEM_PROTOCOL interface for the boot
        partition. */
    Status = gBS->HandleProtocol(   BootPartHandle,
                                    &gEfiSimpleFileSystemProtocolGuid,
                                    (VOID**)&SimpleFileSystem);
    if(!EFI_ERROR(Status)){
        /*  Open the boot partition. */
        Status = SimpleFileSystem->OpenVolume(  SimpleFileSystem,
                                                &sgBootPart_RootDir);
        if (EFI_ERROR(Status))
            /*  On failing to open the root directory, set protocol to NULL */
            sgBootPart_RootDir = NULL;
    }
    return Status;
}

EFI_STATUS EFIAPI Common_FileOpsClose( VOID )
{
    EFI_STATUS Status;
    /* Close the EFI_FILE_PROTOCOL interface for the root directory. */
    /* Close never fails. */
    Status = sgBootPart_RootDir->Close( sgBootPart_RootDir );
    sgBootPart_RootDir = NULL;
    return Status;
}

#define PATHLEN_MAX (512)

EFI_STATUS EFIAPI Common_FreePath(  IN  CHAR16 *Path)
{
    return gBS->FreePool(Path);
}

EFI_STATUS EFIAPI Common_GetPathFromParts(  IN  CHAR8   *DirPath,
                                            IN  CHAR8   *FileName,
                                            OUT CHAR16  **Path_p)
{
    EFI_STATUS ret;
    UINTN DirPath_len, FileName_len, Path_len;
    CHAR16 *Path;
    /*  Get lengths of the two strings to be generated */
    DirPath_len = AsciiStrnLenS(DirPath, PATHLEN_MAX);
    FileName_len = AsciiStrnLenS(FileName, PATHLEN_MAX);
    Path_len = DirPath_len + 1 + FileName_len;
    if(PATHLEN_MAX < Path_len)
        ret = EFI_UNSUPPORTED;
    else{
        /*  Allocate space for Path */
        ret = gBS->AllocatePool(EfiLoaderData,
                                (Path_len+1)*sizeof(CHAR16),
                                (VOID**)&Path);
        if(!EFI_ERROR(ret)){
            /*  Generate the path */
            ret = AsciiStrToUnicodeStrS(DirPath,
                                        Path,
                                        DirPath_len+1);
            if(!EFI_ERROR(ret)){
                Path[DirPath_len] = L'\\';
                ret = AsciiStrToUnicodeStrS(FileName,
                                            &(Path[DirPath_len+1]),
                                            FileName_len+1);
            }
            /*  Free the buffer or assign it to the output variable */
            if(EFI_ERROR(ret))
                Common_FreePath(Path);
            else
                *Path_p = Path;
        }
    }
    return ret;
}

EFI_STATUS EFIAPI Common_CreateOpenFile(OUT EFI_FILE_PROTOCOL   **NewHandle,
                                        IN  CHAR16              *FileName,
                                        IN  UINT64              OpenMode,
                                        IN  UINT64              Attributes)
{
    if(NULL != sgBootPart_RootDir)
        return sgBootPart_RootDir->Open(sgBootPart_RootDir,
                                        NewHandle,
                                        FileName,
                                        (OpenMode | EFI_FILE_MODE_CREATE),
                                        Attributes);
    else
        return EFI_NOT_READY;
}

EFI_STATUS EFIAPI Common_OpenFile(  OUT EFI_FILE_PROTOCOL   **NewHandle,
                                    IN  CHAR16              *FileName,
                                    IN  UINT64              OpenMode)
{
   if(NULL != sgBootPart_RootDir)
        return sgBootPart_RootDir->Open(sgBootPart_RootDir,
                                        NewHandle,
                                        FileName,
                                        OpenMode,
                                        0);
    else
        return EFI_NOT_READY;
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

EFI_STATUS EFIAPI Common_FreeReadBuffer(IN VOID*    buffer_p,
                                        IN UINTN    buffersize)
{
    return gBS->FreePool(buffer_p);
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

EFI_STATUS EFIAPI Common_CreateWriteCloseFile(  IN CHAR16 *filepath,
                                                IN VOID*  buffer,
                                                IN UINTN  buffersize)
{
    EFI_STATUS Status, CloseStatus;
    EFI_FILE_PROTOCOL *filep;

    /* Open file */
    Status = Common_CreateOpenFile( &filep,
                                    filepath,
                                    (EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE),
                                    0);
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

EFI_STATUS EFIAPI Common_OpenReadCloseFile( IN  CHAR16      *filepath,
                                            OUT VOID*       *buffer_p,
                                            OUT UINTN       *buffersize_p)
{
    EFI_STATUS Status, CloseStatus;
    EFI_FILE_PROTOCOL *filep;

    /* Open file */
    Status = Common_OpenFile(   &filep,
                                filepath,
                                EFI_FILE_MODE_READ);
    if( EFI_ERROR(Status) ){
        goto exit0;
    }

    /* Read the file */
    Status = Common_ReadFile(   filep,
                                buffer_p,
                                buffersize_p);

    /* close file */
    CloseStatus = filep->Close( filep );
    if( EFI_ERROR(CloseStatus) )
        if( ! EFI_ERROR(Status) )
            Status = CloseStatus;
exit0:
    return Status;
}

EFI_STATUS EFIAPI Common_CreateWriteCloseDirFile(   IN CHAR8  *dirpath,
                                                    IN CHAR8  *filename,
                                                    IN VOID*  buffer,
                                                    IN UINTN  buffersize)
{
    EFI_STATUS Status, CloseStatus;
    CHAR16 *filepath;
    /* Form file path */
    Status = Common_GetPathFromParts(   dirpath,
                                        filename,
                                        &filepath);
    if(!EFI_ERROR(Status)){
        /* Create, write, and close the file */
        Status = Common_CreateWriteCloseFile(   filepath,
                                                buffer,
                                                buffersize);
        /* Free the file path */
        CloseStatus = Common_FreePath(filepath);
        if( EFI_ERROR(CloseStatus) )
            if( ! EFI_ERROR(Status) )
                Status = CloseStatus;
    }
    return Status;
}

EFI_STATUS EFIAPI Common_OpenReadCloseDirFile(  IN CHAR8    *dirpath,
                                                IN CHAR8    *filename,
                                                OUT VOID*   *buffer_p,
                                                OUT UINTN   *buffersize_p)
{
    EFI_STATUS Status, CloseStatus;
    CHAR16 *filepath;
    /* Form file path */
    Status = Common_GetPathFromParts(   dirpath,
                                        filename,
                                        &filepath);
    if(!EFI_ERROR(Status)){
        /* Open, read, and close the file */
        Common_OpenReadCloseFile(   filepath,
                                    buffer_p,
                                    buffersize_p);
        /* Free the file path */
        CloseStatus = Common_FreePath(filepath);
        if( EFI_ERROR(CloseStatus) ){
            if( ! EFI_ERROR(Status) ){
                Common_FreeReadBuffer(  *buffer_p,
                                        *buffersize_p);
                *buffer_p = NULL;
                *buffersize_p = 0;
                Status = CloseStatus;
            }
        }
    }
    return Status;
}

