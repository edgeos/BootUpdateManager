/* LogPrint.h - Macros, structure definitions, and function headers for
 *              LogPrint.c
 *
 * Author: Safayet N Ahmed (GE Global Research) <Safayet.Ahmed@ge.com>
 *
 * Copyright (c) 2016, General Electric Company. All rights reserved.
 */

#ifndef __LOG_PRINT__
#define __LOG_PRINT__

#define LOG_PRINT_MODE_VALID    (0x3)
#define LOG_PRINT_MODE_CONSOLE  (0x2)
#define LOG_PRINT_MODE_FILE     (0x1)
typedef UINT8   LogPrint_mode_t;

typedef struct LogPrint_state_s {
    LogPrint_mode_t     modes;
    EFI_FILE_PROTOCOL   *logdir;
    CHAR16              *context;
} LogPrint_state_t;

#define ZERO_LOG_PRINT_STATE() {0, NULL, NULL}

UINTN EFIAPI LogPrint(  IN LogPrint_state_t *state_p,
                        IN CONST CHAR16  *Format,
                        ... );

EFI_STATUS EFIAPI LogPrint_init(IN OUT LogPrint_state_t *statep,
                                IN LogPrint_mode_t      modes,
                                IN EFI_FILE_PROTOCOL    *logdir,
                                IN CHAR16               *context);

EFI_STATUS EFIAPI LogPrint_close(   IN OUT LogPrint_state_t *statep );

#endif
