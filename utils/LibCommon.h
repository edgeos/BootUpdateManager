/* LibCommon.h - Function headers for utils/LibCommon.c in user space
 *
 * Author: Safayet N Ahmed (GE Global Research) <Safayet.Ahmed@ge.com>
 *
 * Copyright (c) 2016, General Electric Company. All rights reserved.
 */

#ifndef __LIB_COMMON__
#define __LIB_COMMON__
 
EFI_STATUS EFIAPI Common_CreateWriteCloseFile(  IN EFI_FILE_PROTOCOL    *dir_p,
                                                IN CHAR16               *filename,
                                                IN VOID*                buffer,
                                                IN UINTN                buffersize );

UINTN EFIAPI AsciiStrnLenS( IN CONST CHAR8  *String,
                            IN UINTN        MaxSize);

CHAR8* EFIAPI AsciiStrCpy(  OUT CHAR8       *Destination,
                            IN  CONST CHAR8 *Source);

VOID* EFIAPI ZeroMem(   OUT VOID    *Buffer,
                        IN  UINTN   Length);

#endif
