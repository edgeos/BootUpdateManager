/* LibCommon.c -    Collection of functions implementing functionality
 *                  similar to bootloader/LibCommon.c but for user-space
 *                  utilities.
 *
 * Author: Safayet N Ahmed (GE Global Research) <Safayet.Ahmed@ge.com>
 *
 * Copyright (c) 2017, General Electric Company. All rights reserved.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <uchar.h>
#include "EFIGlue.h"

#include <string.h>

static size_t c16strlen(const CHAR16 *c16str)
{
    size_t i;
    for(i=0; c16str[i] != 0; i++);
    return i;
}

static void c16strtostr(char            *strdst,
                        const CHAR16    *c16strsrc)
{
    size_t i;
    for(i=0; c16strsrc[i] != 0; i++)
        strdst[i] = (char)c16strsrc[i];
    strdst[i] = '\0';
}

static void strtoc16str(CHAR16      *c16strdst,
                        const char  *strsrc)
{
    size_t i;
    for(i=0; strsrc[i] != '\0'; i++)
        c16strdst[i] = (CHAR16)strsrc[i];
    c16strdst[i] = 0;
}

#define PATHLEN_MAX (512)

EFI_STATUS EFIAPI Common_FreePath(  IN  CHAR16 *Path)
{
    free(Path);
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI Common_GetPathFromParts(  IN  CHAR8   *DirPath,
                                            IN  CHAR8   *FileName,
                                            OUT CHAR16  **Path_p)
{
    EFI_STATUS ret;
    UINTN DirPath_len, FileName_len, Path_len;
    CHAR16 *Path;
    /*  Get lengths of the two strings to be generated */
    DirPath_len = strlen(DirPath);
    FileName_len = strlen(FileName);
    Path_len = DirPath_len + 1 + FileName_len;
    if(PATHLEN_MAX < Path_len)
        ret = EFI_UNSUPPORTED;
    else{
        /*  Allocate space for Path */
        Path = (CHAR16*)malloc((Path_len+1)*sizeof(CHAR16));
        if(NULL == Path)
            ret = EFI_OUT_OF_RESOURCES;
        else{
            /*  Generate the path */
            strtoc16str(Path, DirPath);
            Path[DirPath_len] = L'/';
            strtoc16str(&(Path[DirPath_len+1]), FileName);
            *Path_p = Path;
            ret = EFI_SUCCESS;
        }
    }
    return ret;
}

EFI_STATUS EFIAPI Common_OpenFile(  OUT EFI_FILE_PROTOCOL   **NewHandle,
                                    IN  CHAR16              *FileName,
                                    IN  char                *OpenMode)
{
    EFI_STATUS ret;
    size_t pathlen;
    char *filepath8;
    FILE *filep;
    pathlen = c16strlen(FileName);
    filepath8 = (char*)malloc(sizeof(char)*(pathlen+1));
    if(NULL == filepath8)
        ret = EFI_OUT_OF_RESOURCES;
    else{
        c16strtostr(filepath8, FileName);
        filep = fopen(filepath8, OpenMode);
        if(NULL == filep)
            ret = EFI_OUT_OF_RESOURCES;
        else{
            *NewHandle = (EFI_FILE_PROTOCOL*)filep;
            ret = EFI_SUCCESS;
        }
        free(filepath8);
    }
    return ret;
}

static long fsize( FILE* filep )
{
    long saved_file_pos, size = -1;
    int ret;
    saved_file_pos = ftell(filep);
    if( saved_file_pos >= 0 ){
        ret = fseek(filep, 0, SEEK_END);
        if(ret == 0){
            size = ftell(filep);
            ret = fseek(filep, saved_file_pos, SEEK_SET);
            size = (ret == 0)? size : -1;
        }
    }
    return size;
}

EFI_STATUS Common_ReadFile( IN EFI_FILE_PROTOCOL    *filep,
                            VOID*                   *buffer_p,
                            UINTN                   *buffersize_p)
{
    EFI_STATUS ret;
    long filesize;
    VOID *buffer;
    size_t readsize;
    /*  Get file size. */
    filesize = fsize((FILE*)filep);
    if(filesize <= 0)
        ret = EFI_GENERIC_ERROR;
    else{
        /*  Allocate buffer. */
        buffer = malloc(filesize);
        if(NULL == buffer)
            ret = EFI_GENERIC_ERROR;
        else{
            /*  Read the file */
            readsize = fread(buffer, 1, filesize, (FILE*)filep);
            if(readsize < filesize){
                /*  Free the buffer in case of failure */
                free(buffer);
                ret = EFI_GENERIC_ERROR;
            }else{
                /*  Set output variables */
                *buffer_p = buffer;
                *buffersize_p = (UINTN)filesize;
                ret = EFI_SUCCESS;
            }
        }
    }
    return ret;
}

EFI_STATUS EFIAPI Common_FreeReadBuffer(IN VOID*    buffer_p,
                                        IN UINTN    buffersize)
{
    free(buffer_p);
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI Common_OpenReadCloseFile( IN  CHAR16      *filepath,
                                            OUT VOID*       *buffer_p,
                                            OUT UINTN       *buffersize_p)
{
    EFI_STATUS Status;
    EFI_FILE_PROTOCOL *filep;

    /* Open file */
    Status = Common_OpenFile(   &filep,
                                filepath,
                                "r");
    if(!EFI_ERROR(Status)){
        /* Read the file */
        Status = Common_ReadFile(   filep,
                                    buffer_p,
                                    buffersize_p);
        /* close file */
        fclose((FILE*)filep);
    }
    return Status;
}

EFI_STATUS EFIAPI Common_CreateWriteCloseFile(  IN CHAR16 *filepath,
                                                IN VOID*  buffer,
                                                IN UINTN  buffersize)
{
    EFI_STATUS Status;
    EFI_FILE_PROTOCOL *filep;
    size_t writesize;

    /* Open file */
    Status = Common_OpenFile(   &filep,
                                filepath,
                                "w+");
    if( !EFI_ERROR(Status) ){
        /* Write out to file */
        writesize = fwrite(buffer, 1, buffersize, (FILE*)filep);
        if(writesize < buffersize)
            Status = EFI_DEVICE_ERROR;
        else
            Status = EFI_SUCCESS;
        /* close file */
        fclose((FILE*)filep);
    }
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
        Status = Common_OpenReadCloseFile(  filepath,
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

