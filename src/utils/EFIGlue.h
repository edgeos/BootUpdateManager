/* EFIGlue.h - Header defining types and Macros for EFI code in user space
 *
 * Author: Safayet N Ahmed (GE Global Research) <Safayet.Ahmed@ge.com>
 *
 * Copyright (c) 2017, General Electric Company. All rights reserved.
 */
 
#ifndef __EFI_GLUE__
#define __EFI_GLUE__

#define CONST const

typedef void                VOID;
typedef uint64_t            UINT64;
typedef uint32_t            UINT32;
typedef uint8_t             UINT8;
typedef long                INTN;
typedef unsigned long       UINTN;
typedef char16_t            CHAR16;
typedef char                CHAR8;
typedef UINTN               EFI_STATUS;
typedef VOID                EFI_FILE_PROTOCOL;
typedef bool                BOOLEAN;

#define EFIAPI
#define IN
#define OUT
#define INOUT

#define EFI_SUCCESS             (0)
#define EFI_GENERIC_ERROR       (0xFFFFFFFFFFFFFFFF)
#define EFI_LOAD_ERROR          EFI_GENERIC_ERROR
#define EFI_INVALID_PARAMETER   EFI_GENERIC_ERROR
#define EFI_UNSUPPORTED         EFI_GENERIC_ERROR
#define EFI_NOT_READY           EFI_GENERIC_ERROR
#define EFI_DEVICE_ERROR        EFI_GENERIC_ERROR
#define EFI_OUT_OF_RESOURCES    EFI_GENERIC_ERROR
#define EFI_NOT_FOUND           EFI_GENERIC_ERROR
#define EFI_ERROR(stat)         (EFI_SUCCESS != stat)

UINTN EFIAPI AsciiStrnLenS( IN CONST CHAR8  *String,
                            IN UINTN        MaxSize);

CHAR8* EFIAPI AsciiStrCpy(  OUT CHAR8       *Destination,
                            IN  CONST CHAR8 *Source);

VOID* EFIAPI ZeroMem(   OUT VOID    *Buffer,
                        IN  UINTN   Length);

INTN EFIAPI CompareMem( IN CONST VOID  *DestinationBuffer,
                        IN CONST VOID  *SourceBuffer,
                        IN UINTN       Length);

#endif
