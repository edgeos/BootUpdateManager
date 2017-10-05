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

