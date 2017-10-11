/* LibCommon.h - Function headers for LibCommon.c
 *
 * Author: Safayet N Ahmed (GE Global Research) <Safayet.Ahmed@ge.com>
 *
 * Copyright (c) 2016, General Electric Company. All rights reserved.
 */

#ifndef __LIB_COMMON__
#define __LIB_COMMON__
 
EFI_STATUS EFIAPI Common_ReadUEFIVariable(  IN  EFI_GUID*   guid,
                                            IN  CHAR16*     name,
                                            OUT VOID*       *buffer_p,
                                            OUT UINTN       *buffersize_p,
                                            OUT UINT32      *attrs_p );

EFI_STATUS EFIAPI Common_GetFileInfo(   IN  EFI_FILE_PROTOCOL   *filep,
                                        OUT EFI_FILE_INFO       **fileinfo_pp,
                                        OUT UINTN               *fileinfosize_p );

EFI_STATUS EFIAPI Common_ReadFile(  IN  EFI_FILE_PROTOCOL   *filep,
                                    OUT VOID*               *buffer_p,
                                    OUT UINTN               *buffersize_p );

EFI_STATUS EFIAPI Common_WriteFile( IN EFI_FILE_PROTOCOL    *filep,
                                    IN VOID*                buffer,
                                    IN UINTN                buffersize );

EFI_STATUS EFIAPI Common_CreateWriteCloseFile(  IN EFI_FILE_PROTOCOL    *dir_p,
                                                IN CHAR16               *filename,
                                                IN VOID*                buffer,
                                                IN UINTN                buffersize );

#endif
