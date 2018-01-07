/* BootStat.c - Implement the Boot-state reporting feature for
 *              Secure-Boot-enabled CPE400.
 *
 * Author: Safayet N Ahmed (GE Global Research) <Safayet.Ahmed@ge.com>
 *
 * Based on initial idea by Glenn Smith
 *
 * Copyright (c) 2016, General Electric Company. All rights reserved.
 */

/* includes (header file) */

#include "__BootStat.h"

#define BOOTSTATDIR "\\bootstatus"

/**/
EFI_STATUS BootStat_CreateDir(VOID)
{
    EFI_STATUS Status;
    EFI_FILE_PROTOCOL *BootStatDir;
    Status = Common_CreateOpenFile( &BootStatDir,
                                    L"" BOOTSTATDIR,
                                    ( EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE),
                                    EFI_FILE_DIRECTORY);
    if(EFI_ERROR(Status))
        LogPrint(L"BootStat_CreateDir: Common_CreateOpenFile failed (%d)",
                    Status);
    else{
        Status = BootStatDir->Close( BootStatDir );
        if(EFI_ERROR(Status))
            LogPrint(L"BootStat_CreateDir: BootStatDir->Close failed (%d)",
                        Status);
    }
    return Status;
}

EFI_STATUS BootStat_ReportToFile(   IN CHAR8   *filename,
                                    IN VOID*    buffer,
                                    IN UINTN    buffersize )
{
    EFI_STATUS Status;
    /*  generate path */
    Status = Common_CreateWriteCloseDirFile(BOOTSTATDIR,
                                            filename,
                                            buffer,
                                            buffersize);
    if(EFI_ERROR(Status))
        LogPrint(L"BootStat_ReportToFile: Common_CreateWriteCloseDirFile "
                    L"failed (%d)", Status);
    return Status;
}

#define BOOTSTAT_LOG( format, ... ) LogPrint( format, ##__VA_ARGS__ )

static EFI_STATUS BootStat_TimeStamp( void )
{
    EFI_STATUS Status;

    EFI_TIME    TimeStamp;
    UINT64      TimeStamp_TSC;

    #define TIME_STAMP_STRING_FORMAT L"%04d-%02d-%02d %02d:%02d:%02d %016llx"
    #define TIME_STAMP_STRING_LENGTH ( (4 + 1 + 2 + 1 + 2) + 1 + (2+1+2+1+2) \
                                        + 1 + 16 )
    CHAR8   TimeStampStringBuffer[(TIME_STAMP_STRING_LENGTH+1)];

    UINTN Printed;

    TimeStamp_TSC = AsmReadTsc();

    /* Get the current time */
    Status = gRT->GetTime( &TimeStamp, NULL );
    if( EFI_ERROR(Status) ){
        BOOTSTAT_LOG(   L"BootStat_TimeStamp: gRT->GetTime failed (%d). ",
                        Status );
        return Status;
    }

    Printed = AsciiSPrintUnicodeFormat( TimeStampStringBuffer,
                                        (TIME_STAMP_STRING_LENGTH+1),
                                        TIME_STAMP_STRING_FORMAT,
                                        TimeStamp.Year,   TimeStamp.Month,
                                        TimeStamp.Day,
                                        TimeStamp.Hour,   TimeStamp.Minute,
                                        TimeStamp.Second, TimeStamp_TSC );
    if( TIME_STAMP_STRING_LENGTH != Printed ){
        BOOTSTAT_LOG(   L"BootStat_TimeStamp: AsciiSPrintUnicodeFormat "
                        L"returned (%d). Expected (%d).",
                        Printed, TIME_STAMP_STRING_LENGTH );
        return Status;
    }

    Status = BootStat_ReportToFile(    "bum_timestamp",
                                        TimeStampStringBuffer,
                                        TIME_STAMP_STRING_LENGTH);
    if( EFI_ERROR(Status) )
        BOOTSTAT_LOG(   L"BootStat_TimeStamp: BootStat_ReportToFile"
                        L" failed (%d)", Status);
    return Status;
}

static EFI_STATUS BootStat_SmBiosInfo( void )
{
    EFI_STATUS Status, RetStatus;

    OUT CHAR8*  Vendorp;
    OUT UINTN   VendorLen;
    OUT CHAR8*  Versionp;
    OUT UINTN   VersionLen;
    OUT CHAR8*  ReleaseDatep;
    OUT UINTN   ReleaseDateLen;

    /* Get the desired strings from the SmBios tables. */
    Status = SMBIOS_RetrieveBIOSInfo(&Vendorp, &VendorLen,
                                     &Versionp, &VersionLen,
                                     &ReleaseDatep, &ReleaseDateLen);
    if( EFI_ERROR(Status) ){
        BOOTSTAT_LOG(   L"BootStat_SmBiosInfo: SMBIOS_RetrieveBIOSVersion"
                        L" failed (%d)", Status);
        goto exit0;
    }

    /* Output the strings to the boot-Status files. */
    Status = BootStat_ReportToFile("smbiosinfo_vendor",
                                    Vendorp, VendorLen);
    if( EFI_ERROR(Status) )
        BOOTSTAT_LOG(   L"BootStat_SmBiosInfo: BootStat_ReportToFile"
                        L" failed (%d) for vendor", Status);

    RetStatus = BootStat_ReportToFile("smbiosinfo_version",
                                        Versionp, VersionLen);
    if( EFI_ERROR(RetStatus) ){
        Status = RetStatus;
        BOOTSTAT_LOG(   L"BootStat_SmBiosInfo: BootStat_ReportToFile"
                        L" failed (%d) for version", Status);
    }

    RetStatus = BootStat_ReportToFile("smbiosinfo_releasedate",
                                        ReleaseDatep, ReleaseDateLen);
    if( EFI_ERROR(RetStatus) ){
        Status = RetStatus;
        BOOTSTAT_LOG(   L"BootStat_SmBiosInfo: BootStat_ReportToFile"
                        L" failed (%d) for release date", Status);
    }

    /* Free all the allocated strings. */
    gBS->FreePool(Vendorp);
    gBS->FreePool(Versionp);
    gBS->FreePool(ReleaseDatep);
exit0:
    return Status;
}

/* Attributes for update */
#define BOOTSTAT_UEFIVAR_ATTRIBUTES \
            (   EFI_VARIABLE_NON_VOLATILE | \
                EFI_VARIABLE_BOOTSERVICE_ACCESS | \
                EFI_VARIABLE_RUNTIME_ACCESS | \
                EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS )

static EFI_STATUS BootStat_UEFINVInfo( void )
{
    EFI_STATUS Status, CurrStatus;

    UINT64 MaximumVariableStorageSize;
    UINT64 RemainingVariableStorageSize;
    UINT64 MaximumVariableSize;

    Status = gRT->QueryVariableInfo(BOOTSTAT_UEFIVAR_ATTRIBUTES,
                                    &MaximumVariableStorageSize,
                                    &RemainingVariableStorageSize,
                                    &MaximumVariableSize);
    if( EFI_ERROR(Status) ){
        BOOTSTAT_LOG(   L"BootStat_UEFINVInfo: gRT->QueryVariableInfo failed "
                        L"(%d)", Status);
        goto exit0;
    }

    BOOTSTAT_LOG(   L"BootStat_UEFINVInfo: NVSize       = %d bytes",
                    MaximumVariableStorageSize );
    CurrStatus = BootStat_ReportToFile("nvinfo_NVSize",
                                        &MaximumVariableStorageSize,
                                        sizeof(UINT64));
    if( EFI_ERROR(CurrStatus) ){
        BOOTSTAT_LOG(   L"BootStat_UEFINVInfo: BootStat_ReportToFile failed "
                        L"for \"nvinfo_NVSize\"" );
        Status = CurrStatus;
    }

    BOOTSTAT_LOG(   L"BootStat_UEFINVInfo: NVRemaining  = %d bytes",
                    RemainingVariableStorageSize );
    CurrStatus = BootStat_ReportToFile("nvinfo_NVRemaining",
                                        &RemainingVariableStorageSize,
                                        sizeof(UINT64));
    if( EFI_ERROR(CurrStatus) ){
        BOOTSTAT_LOG(   L"BootStat_UEFINVInfo: BootStat_ReportToFile failed "
                        L"for \"nvinfo_NVRemaining\"" );
        if( !EFI_ERROR(Status) )
            Status = CurrStatus;
    }

    BOOTSTAT_LOG(   L"BootStat_UEFINVInfo: MaxVarSize   = %d bytes",
                    MaximumVariableSize );
    CurrStatus = BootStat_ReportToFile("nvinfo_MaxVarSize",
                                        &MaximumVariableSize,
                                        sizeof(UINT64));
    if( EFI_ERROR(CurrStatus) ){
        BOOTSTAT_LOG(   L"BootStat_UEFINVInfo: BootStat_ReportToFile failed "
                        L"for \"nvinfo_MaxVarSize\"" );
        if( !EFI_ERROR(Status) )
            Status = CurrStatus;
    }

exit0:
    return Status;
}

typedef enum {
    BOOTSTAT_UEFIVAR_PK         = 0,
    BOOTSTAT_UEFIVAR_KEK        = 1,
    BOOTSTAT_UEFIVAR_db         = 2,
    BOOTSTAT_UEFIVAR_dbx        = 3,
    BOOTSTAT_UEFIVAR_SBEnabled  = 4,
    BOOTSTAT_UEFIVAR_SetupMode  = 5,
    BOOTSTAT_UEFIVAR_COUNT
} BOOTSTAT_UEFIVAR_ENUM_t;

static CHAR16 *BOOTSTAT_UEFIVAR_NAMES[BOOTSTAT_UEFIVAR_COUNT] =
                {   EFI_PLATFORM_KEY_NAME,          /* PK */
                    EFI_KEY_EXCHANGE_KEY_NAME,      /* KEK */
                    EFI_IMAGE_SECURITY_DATABASE,    /* db */
                    EFI_IMAGE_SECURITY_DATABASE1,   /* dbx */
                    EFI_SETUP_MODE_NAME,            /* SetupMode */
                    EFI_SECURE_BOOT_MODE_NAME,      /* SecureBoot */
                };

static EFI_GUID *BOOTSTAT_UEFIVAR_GUIDS[BOOTSTAT_UEFIVAR_COUNT] =
                {   &gEfiGlobalVariableGuid, /* PK */
                    &gEfiGlobalVariableGuid, /* KEK */
                    &gEfiImageSecurityDatabaseGuid, /* db */
                    &gEfiImageSecurityDatabaseGuid, /* dbx */
                    &gEfiGlobalVariableGuid, /* SetupMode */
                    &gEfiGlobalVariableGuid, /* SecureBoot */
                };

static inline EFI_STATUS BootStat_UEFIVar( BOOTSTAT_UEFIVAR_ENUM_t var_i )
{
    EFI_STATUS  Status;
    VOID        *buffer;
    UINTN       buffersize;
    UINT32      attrs;

    CHAR8   StatFileName_l[IMAGENAME_MAX];
    UINTN   StatFileNameLen;

    /* Read the UEFI varriable. */
    Status = Common_ReadUEFIVariable(   BOOTSTAT_UEFIVAR_GUIDS[ var_i ],
                                        BOOTSTAT_UEFIVAR_NAMES[ var_i ],
                                        &buffer, &buffersize, &attrs );
    if( EFI_ERROR(Status) ){
        if( Status == EFI_NOT_FOUND){
            buffer = NULL;
            buffersize = 0;
        } else {
            BOOTSTAT_LOG(   L"BootStat_UEFIVar: Common_ReadUEFIVariable "
                            L"failed (%d) for \"%s\"",
                            Status, BOOTSTAT_UEFIVAR_NAMES[ var_i ]);
            goto exit0;
        }
    }

    /* Generate the file name from the varriable name. */
    StatFileNameLen = AsciiSPrint(  StatFileName_l,
                                    sizeof(StatFileName_l),
                                    "uefivars_%s",
                                    BOOTSTAT_UEFIVAR_NAMES[var_i] );
    if( (StatFileNameLen == 0) || (StatFileNameLen == IMAGENAME_MAX) ){
        BOOTSTAT_LOG(   L"BootStat_UEFIVar: UnicodeSPrint failed to produce "
                        L"file name for \"%s\"",
                        BOOTSTAT_UEFIVAR_NAMES[ var_i ]);
        goto exit1;
    }

    /* Write the varriable to the file. */
    Status = BootStat_ReportToFile(StatFileName_l, buffer, buffersize);
    if( EFI_ERROR(Status) )
        BOOTSTAT_LOG(   L"BootStat_UEFIVar: BootStat_ReportToFile failed (%d) "
                        L"for \"%s\"", Status, StatFileName_l);

exit1:
    /* Free the UEFI varriable buffer. */
    gBS->FreePool(buffer);
exit0:
    return Status;
}

static EFI_STATUS BootStat_UEFIVars( void )
{
    EFI_STATUS  Status;
    BOOTSTAT_UEFIVAR_ENUM_t var_i;

    for( var_i = 0; var_i < BOOTSTAT_UEFIVAR_COUNT; var_i++ ){
        Status = BootStat_UEFIVar( var_i );
        if( EFI_ERROR(Status) )
            BOOTSTAT_LOG(   L"BootStat_UEFIVars: BootStat_UEFIVar failed (%d)"
                            L" for %d", Status, var_i);
    }

    return Status;
}

typedef EFI_STATUS (*state_report_func_t)( void );

typedef struct {
    BOOLEAN             supported;
    CHAR16              *name;
    state_report_func_t report;
} BootStat_Descriptor_t;

static EFI_STATUS BootStat_SmBiosInfo( void );
static EFI_STATUS BootStat_UEFINVInfo( void );
static EFI_STATUS BootStat_UEFIVars( void );

static BootStat_Descriptor_t BootStat_array[BOOTSTAT_ENUM_COUNT] =
        {   { TRUE, L"bum_timestamp",   BootStat_TimeStamp },
            { TRUE, L"smbios_info",     BootStat_SmBiosInfo },
            { TRUE, L"uefi_nvinfo",     BootStat_UEFINVInfo},
            { TRUE, L"uefi_vars",       BootStat_UEFIVars},
        };

EFI_STATUS EFIAPI ReportBootStat(IN UINT64 BootStat_bitmap)
{
    BootStat_enum_t i;
    EFI_STATUS Status;

    /*  Ensure we can open the boot sttaus directory.
        Otherwise, we can't write anything else. */
    Status = BootStat_CreateDir();
    if(EFI_ERROR(Status))
        BOOTSTAT_LOG(   L"ReportBootStat: BootStat_CreateDir failed (%d)",
                        Status);
    else{
        for( i = 0; i < BOOTSTAT_ENUM_COUNT; i++ ){
            if( BootStat_bitmap & ((UINT64)1 << i) ){
                if( ! BootStat_array[i].supported ){
                    BOOTSTAT_LOG(   L"ReportBootStat: WARNING Status (%d) not "
                                    L"supported", i );
                } else {
                    Status = BootStat_array[i].report();
                    if( EFI_ERROR(Status) ){
                        BOOTSTAT_LOG(   L"ReportBootStat: Failed to report "
                                        L"status \"%s\" (%d)",
                                        BootStat_array[i].name, i );
                    }
                }
            }
        }
    }
    return EFI_SUCCESS;
}

