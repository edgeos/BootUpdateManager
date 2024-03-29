/* LogPrint.c - Implement generic logging (printing) code.
 *              NOTE:   No error logging in this code since this IS the
 *                      error-logging code.
 *
 * Author: Safayet N Ahmed (GE Global Research) <Safayet.Ahmed@ge.com>
 *
 * Copyright (c) 2016, General Electric Company. All rights reserved.
 */

/* includes (header file) */

#include "__LogPrint.h"

#define LOG_PRINT_LINE_PREFIX_FORMAT L"%04d-%02d-%02d %02d:%02d:%02d %016LX %s) "
/* The fixed length of the prefix line given the above format (minus the context). */
#define LOG_PRINT_LINE_PREFIX_LENGTH ( (4 + 1 + 2 + 1 + 2) + 1 + (2+1+2+1+2) \
                                        + 1 + 16 + 1 + 1 + 1)
/* Maximum allowed length for the context */
#define LOG_PRINT_LINE_CONTEXT_MAXLENGTH (80 - LOG_PRINT_LINE_PREFIX_LENGTH)

#define LOG_PRINT_MODE_DEFAULT  (LOG_PRINT_MODE_FILE | LOG_PRINT_MODE_CONSOLE)
#define LOG_PRINT_CTXLBL_DEFAULT    L""

/******************************************************************************/
/*  Functions and definitions related to logging to the file system           */
/******************************************************************************/

static UINT16 modes = 0;
static const CHAR16 *context = NULL;

EFI_STATUS LogPrint_setContextLabel(const CHAR16 *setcontext)
{
    /* Make sure the context label is smaller than the maximum supported */
    if(StrLen(setcontext) > LOG_PRINT_LINE_CONTEXT_MAXLENGTH)
        return EFI_INVALID_PARAMETER;
    else{
        context = setcontext;
        return EFI_SUCCESS;
    }
}

EFI_STATUS LogPrint_setModes(UINT16 setmodes)
{
    if(0 != (setmodes & ~LOG_PRINT_MODE_VALID))
        return EFI_INVALID_PARAMETER;
    else{
        modes = setmodes;
        return EFI_SUCCESS;
    }
}

#define LOG_PRINT_LINENO_MAX_BITS   (9)
#define LOG_PRINT_LINENO_MAX        (1 << LOG_PRINT_LINENO_MAX_BITS)
#define LOG_PRINT_LINENO_MAX_MASK   (LOG_PRINT_LINENO_MAX - 1)
#define LOG_PRINT_LINENO_HEXDIGITS  ((LOG_PRINT_LINENO_MAX_BITS + 3) / 4)

#define LOGDIR_PATH      L"\\bootlog"
#define LOGLINE_PATH    LOGDIR_PATH L"\\logline.txt"
#define LOGFILE_FORMAT  LOGDIR_PATH L"\\%03d.txt"
#define LOGFILE_PATHLEN (sizeof(LOGDIR_PATH) + 8 + 1)

static EFI_STATUS LogPrint_file_setline(IN UINT16 logline)
{
    EFI_STATUS  Status;
    INTN i;
    UINT16 lineNumberAcc;
    CHAR8 lineNumber[LOG_PRINT_LINENO_HEXDIGITS];
    static const CHAR8 _IntToHexLT[16] =
                        {   '0', '1', '2', '3', '4', '5', '6', '7',
                            '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    /* Convert line number to hex value */
    lineNumberAcc = logline;
    for( i = (LOG_PRINT_LINENO_HEXDIGITS-1); i >= 0; i--){
        lineNumber[i] = _IntToHexLT[(lineNumberAcc & 0xf)];
        lineNumberAcc = lineNumberAcc >> 4;
    }
    /* Write the line number to the log-line file */
    Status = Common_CreateWriteCloseFile(   LOGLINE_PATH,
                                            lineNumber,
                                            LOG_PRINT_LINENO_HEXDIGITS );
    return Status;
}

static EFI_STATUS LogPrint_file_getline(OUT UINT16 *logline_p)
{
    EFI_STATUS  Status, FreeStatus;
    UINTN i;
    UINT16 lineNumberAcc;
    CHAR8   *lineNumberHexBuf;
    UINTN   lineNumberHexBufSize;
    /* Open the log-line file */
    Status = Common_OpenReadCloseFile(  LOGLINE_PATH,
                                        (VOID**)&lineNumberHexBuf,
                                        &lineNumberHexBufSize);
    if( EFI_ERROR(Status) ||
        (lineNumberHexBufSize != LOG_PRINT_LINENO_HEXDIGITS)){
        lineNumberAcc = 0;
        Status = EFI_NOT_READY;
    }else{
        /* Convert line number from hex */
        lineNumberAcc = 0;
        for( i = 0; i < LOG_PRINT_LINENO_HEXDIGITS; i++){
            if( (lineNumberHexBuf[i] >= '0') && (lineNumberHexBuf[i] <= '9') )
                lineNumberAcc = (lineNumberAcc << 4)
                                    + lineNumberHexBuf[i] - '0';
            else if( (lineNumberHexBuf[i] >= 'A') && (lineNumberHexBuf[i] <= 'F') )
                lineNumberAcc = (lineNumberAcc << 4)
                                    + lineNumberHexBuf[i] - 'A' + 10;
            else if( (lineNumberHexBuf[i] >= 'a') && (lineNumberHexBuf[i] <= 'f') )
                lineNumberAcc = (lineNumberAcc << 4)
                                    + lineNumberHexBuf[i] - 'a' + 10;
            else{
                /* The line number is not valid */
                lineNumberAcc = 0;
                Status = EFI_NOT_READY;
            }
        }
        /* Check the line-number read */
        if( lineNumberAcc >= LOG_PRINT_LINENO_MAX ){
            /* The line number is not valid */
            lineNumberAcc = 0;
            Status = EFI_NOT_READY;
        }
        /* Free the line-number hex buffer */
        FreeStatus = Common_FreeReadBuffer( lineNumberHexBuf,
                                            lineNumberHexBufSize);
        if(EFI_ERROR(FreeStatus))
            if(!EFI_ERROR(Status))
                Status = FreeStatus;
    }
    /* Set the log line */
    *logline_p = lineNumberAcc;
    return Status;
}

static EFI_STATUS LogPrint_file(IN CHAR8*           Buffer,
                                IN UINTN            Length,
                                OUT UINTN           *Printedp)
{
    EFI_STATUS  Status;
    UINT16  logline;
    UINT16  filename[LOGFILE_PATHLEN];
    UINTN Printed = 0;
    /* Get the line number */
    Status = LogPrint_file_getline(&logline);
    if( !EFI_ERROR(Status) ){
        /* Form the log-file name */
        UnicodeSPrint(  filename,
                        sizeof(filename),
                        LOGFILE_FORMAT,
                        (int)logline);
        /* Write the buffer out to the log file. */
        Status = Common_CreateWriteCloseFile(   filename,
                                                Buffer,
                                                Length);
        if(!EFI_ERROR(Status)){
            /*  If Common_CreateWriteCloseFile succeeded,
                the full buffer was written */
            Printed = Length;
            /*  Increment the line number
                (not more than 512 lines upto 512-bytes each ~1MB total
                on FAT fs ) */
            logline = (logline + 1) & LOG_PRINT_LINENO_MAX_MASK;
            Status = LogPrint_file_setline(logline);
        }
    }
    *Printedp = Printed;
    return Status;
}

static EFI_STATUS LogPrint_init_file(VOID)
{
    UINT16  logline;
    /* Check that the LogDir is in fact a directory by attempting to read the
       line file and then attempting to write it. */
    LogPrint_file_getline(&logline);
    /*  On failure, logline should be zero. Otherwise, logline
        should be corretcly set. Attempt to write it back out. */
    return LogPrint_file_setline(logline);
}

/******************************************************************************/
/*  Functions and definitions related to logging to console.                  */
/******************************************************************************/

static EFI_STATUS LogPrint_console( IN CHAR8*           Buffer,
                                    IN UINTN            Length,
                                    OUT UINTN           *Printedp)
{
    EFI_STATUS Status;
    UINTN Printed = 0;

    /* Ensure that the string is NULL-terminated */
    if( '\0' != Buffer[Length] ){
        Status = EFI_INVALID_PARAMETER;
        goto exit0;
    }

    /* Ensure there is a console to print to */
    if( gST->ConOut == NULL ){
        Status = EFI_UNSUPPORTED;
        goto exit0;
    }

    Printed = AsciiPrint(Buffer);
    Status = EFI_SUCCESS;

exit0:
    *Printedp = Printed;
    return Status;
}

/******************************************************************************/
/*  Helper functions and definitions.                                         */
/******************************************************************************/

#define LOG_PRINT_LINE_PREFIX(Buffer, BufferSize, TimeStampp, TSC, Context) \
            AsciiSPrintUnicodeFormat(   Buffer, BufferSize, \
                                        LOG_PRINT_LINE_PREFIX_FORMAT, \
                                        (int)(TimeStampp->Year), \
                                        (int)(TimeStampp->Month), \
                                        (int)(TimeStampp->Day), \
                                        (int)(TimeStampp->Hour), \
                                        (int)(TimeStampp->Minute), \
                                        (int)(TimeStampp->Second), \
                                        (UINT64)TSC, \
                                        Context )

static EFI_STATUS LogPrint_buffer(  IN  EFI_TIME        *TimeStampp,
                                    IN  UINT64          TSC,
                                    IN  CONST CHAR16*   Context,
                                    IN  CONST CHAR16*   Format,
                                    IN  VA_LIST         Marker,
                                    OUT UINTN           *BufferSizep,
                                    OUT CHAR8*          *Bufferp,
                                    OUT UINTN           *Printedp )
{
    EFI_STATUS Status;
    UINTN Printed = 0;

    UINTN BufferSize = 0;
    CHAR8 *Buffer = NULL;

    UINT32 MaxLen;

    /* Check the Format argument value */
    if( NULL == Format ){
        Status = EFI_INVALID_PARAMETER;
        goto exit0;
    }

    /* Check the Format argument alignment */
    if( 0 != ( ((UINTN)Format) & (sizeof(UINT16) - 1) ) ){
        Status = EFI_INVALID_PARAMETER;
        goto exit0;
    }

    /* Determine the required buffer length */
    BufferSize = SPrintLength( Format, Marker );

    /* Add the additional length for the line prefix and new-line character and
        NULL terminator */
    BufferSize = BufferSize + LOG_PRINT_LINE_PREFIX_LENGTH + StrLen(Context) + 1 + 1;

    /* Return EFI_UNSUPPORTED if the output string is too large. */
    MaxLen = PcdGet32(PcdMaximumUnicodeStringLength);
    if( BufferSize > MaxLen ){
        BufferSize = 0;
        Status = EFI_UNSUPPORTED;
        goto exit0;
    }

    /* Allocate the buffer */
    Status = gBS->AllocatePool( EfiLoaderData, BufferSize, (VOID**)&Buffer);
    if( EFI_ERROR(Status) ){
        BufferSize = 0;
        goto exit0;
    }

    /* Write the line prefix to the buffer */
    Printed = LOG_PRINT_LINE_PREFIX(Buffer, BufferSize, TimeStampp, TSC, Context);

    /* Write the line to the buffer */
    Printed += AsciiVSPrintUnicodeFormat(   Buffer + Printed,
                                            BufferSize - Printed,
                                            Format, Marker );

    /* Add a new-line character and NULL terminated */
    Buffer[Printed++] = '\n';
    Buffer[Printed] = '\0';

    /* Return Success */
    Status = EFI_SUCCESS;

exit0:
    /* Write the output varriables and return success */
    *Bufferp = Buffer;
    *BufferSizep = BufferSize;
    *Printedp = Printed;
    return Status;
}

/******************************************************************************/
/*  Externally-visible functions.                                             */
/******************************************************************************/

UINTN EFIAPI LogPrint(  IN CONST CHAR16  *Format,
                        ... )
{
    EFI_STATUS Status;

    UINTN   Ret, PrintedToBuffer, PrintedToLog;

    EFI_TIME    TimeStamp;
    UINT64      TimeStamp_TSC;

    VA_LIST Marker;

    CHAR8   *Buffer = NULL;
    UINTN   BufferSize = 0;

    /* Get the TSC time stamp time */
    TimeStamp_TSC = AsmReadTsc();

    /* Get the current time */
    Status = gRT->GetTime( &TimeStamp, NULL );
    if( EFI_ERROR(Status) ){
        TimeStamp = (EFI_TIME){0};
    }

    /* Get the Varriable-Argument list */
    VA_START (Marker, Format);

    /* Parse the format string and other arguments to get a CHAR8 buffer */
    Status = LogPrint_buffer(   &TimeStamp, TimeStamp_TSC, context,
                                Format, Marker,
                                &BufferSize, &Buffer, &PrintedToBuffer);
    if( EFI_ERROR(Status) ){
        Ret = 0;
        goto exit0;
    }

    /*  Assume that the full printed string will be completely writtten to all
        logs*/
    Ret = PrintedToBuffer;

    /* If one of the requested logging modes, print to console */
    if( 0 != (modes & LOG_PRINT_MODE_CONSOLE ) ){
        LogPrint_console( Buffer, PrintedToBuffer, &PrintedToLog);
        /* Ret should be minimum of what is written to all requested logs */
        Ret = ( Ret > PrintedToLog)? PrintedToLog : Ret;
    }

    /* If one of the requested logging modes, print to log file */
    if( 0 != (modes & LOG_PRINT_MODE_FILE ) ){
        LogPrint_file( Buffer, PrintedToBuffer, &PrintedToLog);
        /* Ret should be minimum of what is written to all requested logs */
        Ret = ( Ret > PrintedToLog)? PrintedToLog : Ret;
    }

    /* Free the LogPrint_buffer */
    gBS->FreePool( Buffer );
exit0:
    /* Done with the Varriable-Argument list */
    VA_END (Marker);
    return Ret;
}

VOID EFIAPI LogPrint_init(VOID)
{
    /*  Initialize the "context" string to a default value (empty) */
    context = LOG_PRINT_CTXLBL_DEFAULT;
    modes = LOG_PRINT_MODE_DEFAULT;
    LogPrint_init_file();
}

