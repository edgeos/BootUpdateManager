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

EFI_STATUS LogPrint_setModes(UINT16 setmodes);

EFI_STATUS LogPrint_setContextLabel(const CHAR16 *setcontext);

UINTN EFIAPI LogPrint(  IN CONST CHAR16  *Format,
                        ... );

VOID EFIAPI LogPrint_init(VOID);

#endif
