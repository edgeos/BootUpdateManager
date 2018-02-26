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


#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define FILE_LINE_STRING " (" __FILE__ ", line " TOSTRING(__LINE__) "): "

#define CANARYMAGIC (0x57ACBA54F00BAA12)
#define DECLARECANARY()   UINT64 canary= CANARYMAGIC
#define CHECKCANARY()   do{\
                            if(CANARYMAGIC != canary){\
                                Print(L"Dead canary 0x%016LX " FILE_LINE_STRING L"\n", \
                                        canary);\
                            }\
                        }while(0)

#define CHECKPOINT()    do{\
                            static UINT64 checkpointcounter = 1;\
                            Print(FILE_LINE_STRING L" %Ld\n", \
                                  checkpointcounter++);\
                        }while(0)

#endif
