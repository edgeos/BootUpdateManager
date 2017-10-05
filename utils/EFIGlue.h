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
typedef char16_t            CHAR16;
typedef char                CHAR8;
typedef unsigned long       UINTN;
typedef UINTN               EFI_STATUS;
typedef VOID*               EFI_FILE_PROTOCOL;

#define EFIAPI
#define IN
#define OUT
#define INOUT

#define EFI_SUCCESS (0)
#define EFI_INVALID_PARAMETER (0xFFFFFFFFFFFFFFFF)
#define EFI_ERROR(stat) (EFI_SUCCESS != stat)

#endif
