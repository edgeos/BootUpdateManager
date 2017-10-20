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

static char* formfilepath(  char    *dirpath,
                            CHAR16  *filename)
{
    size_t dirlen, fnamelen, pathlen;
    char *fullpath;
    /*  compute lengths */
    fnamelen = c16strlen(filename);
    dirlen = strlen(dirpath);
    pathlen = dirlen + 1 + fnamelen;
    /*  allocate memory to store full path */
    fullpath = malloc(sizeof(char)*(pathlen+1));
    if(NULL != fullpath){
        /*  copy the stem */
        strcpy(fullpath, dirpath);
        /*  add the / */
        fullpath[dirlen] = '/';
        /*  copy the leaf */
        c16strtostr(&(fullpath[dirlen+1]), filename);
    }
    return fullpath;
}

EFI_STATUS EFIAPI Common_CreateWriteCloseFile(  IN EFI_FILE_PROTOCOL    *dir_p,
                                                IN CHAR16               *filename,
                                                IN VOID*                buffer,
                                                IN UINTN                buffersize )
{
    EFI_STATUS ret;
    char *fullpath;
    FILE *filep;
    size_t written;
    int flushstat;
    /*  Form the file path */
    fullpath = formfilepath((char*)dir_p, filename);
    if(NULL == fullpath){
        ret = EFI_GENERIC_ERROR;
        goto exit0;
    }
    /*  Create/Open the file */
    filep = fopen(fullpath, "w");
    if(NULL == filep){
        ret = EFI_GENERIC_ERROR;
        goto exit1;
    }
    /*  Write the file */
    written = fwrite(buffer, (size_t)1, buffersize, filep);
    if(written != buffersize){
        ret = EFI_GENERIC_ERROR;
        goto exit2;
    }
    /*  Flush the file */
    flushstat = fflush(filep);
    if(0 != flushstat)
        ret = EFI_GENERIC_ERROR;
    else
        ret = EFI_SUCCESS;
    /*  Close the file */
exit2:
    fclose(filep);
    /*  Free the file path */
exit1:
    free(fullpath);
exit0:
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

static EFI_STATUS Common_ReadFile(  FILE*       filep,
                                    VOID*       *buffer_p,
                                    UINTN       *buffersize_p)
{
    EFI_STATUS ret;
    long filesize;
    VOID *buffer;
    size_t read;
    /*  Get file size. */
    filesize = fsize(filep);
    if(filesize <= 0)
        ret = EFI_GENERIC_ERROR;
    else{
        /*  Allocate buffer. */
        buffer = malloc(filesize);
        if(NULL == buffer)
            ret = EFI_GENERIC_ERROR;
        else{
            /*  Read the file */
            read = fread(buffer, 1, filesize, filep);
            if(read < filesize){
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

EFI_STATUS EFIAPI Common_OpenReadCloseFile( IN EFI_FILE_PROTOCOL    *dir_p,
                                            IN CHAR16               *filename,
                                            IN VOID*                *buffer_p,
                                            IN UINTN                *buffersize_p )
{
    EFI_STATUS ret;
    char *fullpath;
    FILE *filep;
    /*  Form the file path */
    fullpath = formfilepath((char*)dir_p, filename);
    if(NULL == fullpath){
        ret = EFI_GENERIC_ERROR;
        goto exit0;
    }
    /*  Open the file for reading */
    filep = fopen(fullpath, "r");
    if(NULL == filep){
        ret = EFI_GENERIC_ERROR;
        goto exit1;
    }
    /*  Read the file */
    ret = Common_ReadFile( filep,
                           buffer_p,
                           buffersize_p);
    /*  Close the file */
    fclose(filep);
    /*  Free the file path */
exit1:
    free(fullpath);
exit0:
    return ret;
}

EFI_STATUS EFIAPI Common_FreeReadBuffer(IN VOID*    buffer_p,
                                        IN UINTN    buffersize)
{
    free(buffer_p);
    return EFI_SUCCESS;
}

