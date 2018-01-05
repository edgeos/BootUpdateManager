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

EFI_STATUS EFIAPI Common_FileOpsInit(   EFI_HANDLE  BootPartHandle);

EFI_STATUS EFIAPI Common_FileOpsClose( VOID );

#define PATHLEN_MAX (512)

EFI_STATUS EFIAPI Common_GetPathFromParts(  IN  CHAR8   *DirPath,
                                            IN  CHAR8   *FileName,
                                            OUT CHAR16  **Path_p);

EFI_STATUS EFIAPI Common_CreateOpenFile(OUT EFI_FILE_PROTOCOL   **NewHandle,
                                        IN  CHAR16              *FileName,
                                        IN  UINT64              OpenMode,
                                        IN  UINT64              Attributes);

EFI_STATUS EFIAPI Common_OpenFile(  OUT EFI_FILE_PROTOCOL   **NewHandle,
                                    IN  CHAR16              *FileName,
                                    IN  UINT64              OpenMode);

EFI_STATUS EFIAPI Common_GetFileInfo(   IN  EFI_FILE_PROTOCOL   *filep,
                                        OUT EFI_FILE_INFO       **fileinfo_pp,
                                        OUT UINTN               *fileinfosize_p );

EFI_STATUS EFIAPI Common_ReadFile(  IN  EFI_FILE_PROTOCOL   *filep,
                                    OUT VOID*               *buffer_p,
                                    OUT UINTN               *buffersize_p );

EFI_STATUS EFIAPI Common_WriteFile( IN EFI_FILE_PROTOCOL    *filep,
                                    IN VOID*                buffer,
                                    IN UINTN                buffersize );

EFI_STATUS EFIAPI Common_CreateWriteCloseFile(  IN CHAR16   *filename,
                                                IN VOID*    buffer,
                                                IN UINTN    buffersize );

EFI_STATUS EFIAPI Common_OpenReadCloseFile( IN  CHAR16      *filename,
                                            OUT VOID*       *buffer_p,
                                            OUT UINTN       *buffersize_p );

#endif
