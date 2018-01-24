/* LibCommon.h - Function headers for utils/LibCommon.c in user space
 *
 * Author: Safayet N Ahmed (GE Global Research) <Safayet.Ahmed@ge.com>
 *
 * Copyright (c) 2016, General Electric Company. All rights reserved.
 */

#ifndef __LIB_COMMON__
#define __LIB_COMMON__

EFI_STATUS EFIAPI Common_FreePath(  IN  CHAR16 *Path);

EFI_STATUS EFIAPI Common_GetPathFromParts(  IN  CHAR8   *DirPath,
                                            IN  CHAR8   *FileName,
                                            OUT CHAR16  **Path_p);

EFI_STATUS EFIAPI Common_OpenFile(  OUT EFI_FILE_PROTOCOL   **NewHandle,
                                    IN  CHAR16              *FileName,
                                    IN  char                *OpenMode);

EFI_STATUS EFIAPI Common_ReadFile(  IN  EFI_FILE_PROTOCOL   *filep,
                                    OUT VOID*               *buffer_p,
                                    OUT UINTN               *buffersize_p );

EFI_STATUS EFIAPI Common_FreeReadBuffer(IN VOID*    buffer_p,
                                        IN UINTN    buffersize);

EFI_STATUS EFIAPI Common_CreateWriteCloseFile(  IN CHAR16   *filename,
                                                IN VOID*    buffer,
                                                IN UINTN    buffersize );

EFI_STATUS EFIAPI Common_OpenReadCloseFile( IN  CHAR16      *filename,
                                            OUT VOID*       *buffer_p,
                                            OUT UINTN       *buffersize_p );

EFI_STATUS EFIAPI Common_CreateWriteCloseDirFile(   IN CHAR8  *dirpath,
                                                    IN CHAR8  *filename,
                                                    IN VOID*  buffer,
                                                    IN UINTN  buffersize);

EFI_STATUS EFIAPI Common_OpenReadCloseDirFile(  IN CHAR8    *dirpath,
                                                IN CHAR8    *filename,
                                                OUT VOID*   *buffer_p,
                                                OUT UINTN   *buffersize_p);

#endif
