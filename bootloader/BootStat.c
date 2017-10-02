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

static EFI_FILE_PROTOCOL    *BootStatDir_g = NULL;

#define BootState_ReportToFile( filename, buffer, buffersize ) \
            Common_CreateWriteCloseFile(  BootStatDir_g, filename, \
                                          buffer, buffersize )

#define BOOTSTAT_LOG( format, ... ) LogPrint( logstatep_g, format, ##__VA_ARGS__ )

static LogPrint_state_t *logstatep_g = NULL;

static CHAR16           *ImageName_g = NULL;

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

    TimeStamp_TSC = __rdtsc();

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

    Status = BootState_ReportToFile(    L"bum_timestamp",
                                        TimeStampStringBuffer,
                                        TIME_STAMP_STRING_LENGTH);
    if( EFI_ERROR(Status) )
        BOOTSTAT_LOG(   L"BootStat_TimeStamp: BootState_ReportToFile"
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
    Status = BootState_ReportToFile(L"smbiosinfo_vendor",
                                    Vendorp, VendorLen);
    if( EFI_ERROR(Status) )
        BOOTSTAT_LOG(   L"BootStat_SmBiosInfo: BootState_ReportToFile"
                        L" failed (%d) for vendor", Status);

    RetStatus = BootState_ReportToFile(L"smbiosinfo_version",
                                        Versionp, VersionLen);
    if( EFI_ERROR(RetStatus) ){
        Status = RetStatus;
        BOOTSTAT_LOG(   L"BootStat_SmBiosInfo: BootState_ReportToFile"
                        L" failed (%d) for version", Status);
    }

    RetStatus = BootState_ReportToFile(L"smbiosinfo_releasedate",
                                        ReleaseDatep, ReleaseDateLen);
    if( EFI_ERROR(RetStatus) ){
        Status = RetStatus;
        BOOTSTAT_LOG(   L"BootStat_SmBiosInfo: BootState_ReportToFile"
                        L" failed (%d) for release date", Status);
    }

    /* Free all the allocated strings. */
    gBS->FreePool(Vendorp);
    gBS->FreePool(Versionp);
    gBS->FreePool(ReleaseDatep);
exit0:
    return Status;
}

static EFI_STATUS BootStat_LoadedImageType( void )
{
    EFI_STATUS Status;
    UINTN   NameLen;
    RETURN_STATUS ret;
    CHAR8   ImageName_l[IMAGENAME_MAX];

    NameLen = StrnLenS(ImageName_g, IMAGENAME_MAX);

    if(NameLen >= IMAGENAME_MAX)
        return EFI_INVALID_PARAMETER;

    BOOTSTAT_LOG(   L"BootStat_LoadedImageType: L\"%S\"", ImageName_g);

    ret = UnicodeStrToAsciiStrS( ImageName_g, ImageName_l, IMAGENAME_MAX );
    if( RETURN_ERROR(ret) ){
        BOOTSTAT_LOG(   L"BootStat_LoadedImageType: UnicodeStrToAsciiStrS"
                        L" failed");
        return EFI_INVALID_PARAMETER;
    }

    Status = BootState_ReportToFile(L"bum_ldimgtype", ImageName_l, NameLen);
    if( EFI_ERROR(Status) )
        BOOTSTAT_LOG(   L"BootStat_LoadedImageType: BootState_ReportToFile"
                        L" failed");

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
    CurrStatus = BootState_ReportToFile(L"nvinfo_NVSize",
                                        &MaximumVariableStorageSize,
                                        sizeof(UINT64));
    if( EFI_ERROR(CurrStatus) ){
        BOOTSTAT_LOG(   L"BootStat_UEFINVInfo: BootState_ReportToFile failed "
                        L"for \"nvinfo_NVSize\"" );
        Status = CurrStatus;
    }

    BOOTSTAT_LOG(   L"BootStat_UEFINVInfo: NVRemaining  = %d bytes",
                    RemainingVariableStorageSize );
    CurrStatus = BootState_ReportToFile(L"nvinfo_NVRemaining",
                                        &RemainingVariableStorageSize,
                                        sizeof(UINT64));
    if( EFI_ERROR(CurrStatus) ){
        BOOTSTAT_LOG(   L"BootStat_UEFINVInfo: BootState_ReportToFile failed "
                        L"for \"nvinfo_NVRemaining\"" );
        if( !EFI_ERROR(Status) )
            Status = CurrStatus;
    }

    BOOTSTAT_LOG(   L"BootStat_UEFINVInfo: MaxVarSize   = %d bytes",
                    MaximumVariableSize );
    CurrStatus = BootState_ReportToFile(L"nvinfo_MaxVarSize",
                                        &MaximumVariableSize,
                                        sizeof(UINT64));
    if( EFI_ERROR(CurrStatus) ){
        BOOTSTAT_LOG(   L"BootStat_UEFINVInfo: BootState_ReportToFile failed "
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

    CHAR16 StatFileName_l[IMAGENAME_MAX];
    UINTN  StatFileNameLen;

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
    StatFileNameLen = UnicodeSPrint(StatFileName_l, sizeof(StatFileName_l),
                                    L"uefivars_%s",
                                    BOOTSTAT_UEFIVAR_NAMES[var_i] );
    if( (StatFileNameLen == 0) || (StatFileNameLen == IMAGENAME_MAX) ){
        BOOTSTAT_LOG(   L"BootStat_UEFIVar: UnicodeSPrint failed to produce "
                        L"file name for \"%s\"",
                        BOOTSTAT_UEFIVAR_NAMES[ var_i ]);
        goto exit1;
    }

    /* Write the varriable to the file. */
    Status = BootState_ReportToFile(StatFileName_l, buffer, buffersize);
    if( EFI_ERROR(Status) )
        BOOTSTAT_LOG(   L"BootStat_UEFIVar: BootState_ReportToFile failed (%d) "
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
static EFI_STATUS BootStat_LoadedImageType( void );
static EFI_STATUS BootStat_UEFINVInfo( void );
static EFI_STATUS BootStat_UEFIVars( void );

static BootStat_Descriptor_t BootStat_array[BOOTSTAT_ENUM_COUNT] =
        {   { TRUE, L"bum_timestamp",   BootStat_TimeStamp },
            { TRUE, L"smbios_info",     BootStat_SmBiosInfo },
            { TRUE, L"bum_ldimgtype",   BootStat_LoadedImageType },
            { TRUE, L"uefi_nvinfo",     BootStat_UEFINVInfo},
            { TRUE, L"uefi_vars",       BootStat_UEFIVars},
        };

EFI_STATUS EFIAPI ReportBootStat(   IN UINT64               BootStat_bitmap,
                                    IN EFI_FILE_PROTOCOL    *BootStatDir_l,
                                    IN CHAR16               *ImageName_l,
                                    IN LogPrint_state_t     *logstatep_l )
{
    BootStat_enum_t i;
    EFI_STATUS Status;

    BootStatDir_g   = BootStatDir_l;
    ImageName_g     = ImageName_l;
    logstatep_g     = logstatep_l;

    for( i = 0; i < BOOTSTAT_ENUM_COUNT; i++ ){
        if( BootStat_bitmap & ((UINT64)1 << i) ){
            if( ! BootStat_array[i].supported ){
                BOOTSTAT_LOG(   L"ReportBootStat: WARNING Status (%d) not "
                                L"supported", i );
            } else {
                Status = BootStat_array[i].report();
                if( EFI_ERROR(Status) ){
                    BOOTSTAT_LOG(   L"ReportBootStat: Failed to report status "
                                    L"\"%s\" (%d)", BootStat_array[i].name, i );
                }
            }
        }
    }

    logstatep_g = NULL;
    ImageName_g = NULL;

    return EFI_SUCCESS;
}

