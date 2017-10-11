/* LibCommon.h - Function headers for utils/LibCommon.c in user space
 *
 * Author: Safayet N Ahmed (GE Global Research) <Safayet.Ahmed@ge.com>
 *
 * Copyright (c) 2016, General Electric Company. All rights reserved.
 */

#ifndef __LIB_COMMON__
#define __LIB_COMMON__

EFI_STATUS EFIAPI Common_CreateWriteCloseFile(  IN  EFI_FILE_PROTOCOL   *dir_p,
                                                IN  CHAR16              *filename,
                                                IN  VOID*               buffer,
                                                IN  UINTN               buffersize );

EFI_STATUS EFIAPI Common_OpenReadCloseFile( IN  EFI_FILE_PROTOCOL   *dir_p,
                                            IN  CHAR16              *filename,
                                            OUT VOID*               *buffer_p,
                                            OUT UINTN               *buffersize_p );

EFI_STATUS EFIAPI Common_FreeReadBuffer(IN VOID*    buffer_p,
                                        IN UINTN    buffersize);

#endif
