/* BootUpdateManager.c - Implement the key-update and fall-back functionality
 *                       for Secure-Boot-enabled CPE400.
 *
 * Author: Safayet N Ahmed (GE Global Research) <Safayet.Ahmed@ge.com>
 *
 * Based on initial idea and code by Glenn Smith
 *
 * Copyright (c) 2016, General Electric Company. All rights reserved.
 */

/* includes (header file) */

#include "__BootUpdateManager.h"

/******************************************************************************/
/*  Type and Constant Definitions                                             */
/******************************************************************************/

static EFI_GUID gEfiBUMVariableGuid = { 0x3B56CA66, 0x94B1, 0x11E6,
                                                { 0x98, 0x06, 0xD8, 0x9D,
                                                  0x67, 0xF4, 0x0B, 0xD7 } };

#define BUM_LOADEDIMAGE_STATE_VARNAME L"BUM_LOADEDIMAGE_STATE"

typedef enum {
    BUM_LOADEDIMAGE_FWTOROOT = 0,
    BUM_LOADEDIMAGE_ROOTTOPRIM = 1,
    BUM_LOADEDIMAGE_PRIMTOBACK = 2,
    BUM_LOADEDIMAGE_PRIMTOGRUB = 3,
    BUM_LOADEDIMAGE_ROOTTOBACK = 4,
    BUM_LOADEDIMAGE_BACKTOGRUB = 5,
    BUM_LOADEDIMAGE_FAIL = 6,
    BUM_LOADEDIMAGE_STATE_COUNT
} BUM_LOADEDIMAGE_STATE_t;

#define BUM_LOADEDIMAGE_STATE_SIZE (sizeof(BUM_LOADEDIMAGE_STATE_t))

typedef enum {
    BUM_LOADEDIMAGE_ROOT,
    BUM_LOADEDIMAGE_PRIM,
    BUM_LOADEDIMAGE_BACK,
    BUM_LOADEDIMAGE_UNKNOWN,
    BUM_LOADEDIMAGE_TYPE_COUNT
} BUM_LOADEDIMAGE_TYPE_t;

static BUM_LOADEDIMAGE_TYPE_t BUM_STATE_TO_TYPE_TABLE[BUM_LOADEDIMAGE_STATE_COUNT] =
        {   BUM_LOADEDIMAGE_ROOT, /* FWTOROOT */
            BUM_LOADEDIMAGE_PRIM, /* ROOTTOPRIM */
            BUM_LOADEDIMAGE_BACK, /* PRIMTOBACK */
            BUM_LOADEDIMAGE_BACK, /* PRIMTOGRUB (actually means GRUBTOBACK) */
            BUM_LOADEDIMAGE_BACK, /* ROOTTOBACK */
            BUM_LOADEDIMAGE_UNKNOWN,/* BACKTOGRUB <This is an error> */
            BUM_LOADEDIMAGE_UNKNOWN,/* BACKTOGRUB <This is an error> */
        };

static CHAR16* BUM_LOADEDIMAGE_TYPE_TEXT[BUM_LOADEDIMAGE_TYPE_COUNT]
                = { L"Root",
                    L"Primary",
                    L"Backup",
                    L"Unknown" };

/*  Boot-status directory is common to all image types. The image that runs
    last will have its state in the boot-status directory. */
#define BOOTSTAT_PATH   L"\\bootstatus"

#define ROOT_BUM_PATH       L"\\efi\\boot\\bootx64.efi"
#define ROOT_BUMLOG_PATH    L"\\root\\log"

/* Load path compare strings for boot from "primary" directory.
 *  Grub can mix forward slash and back slash in chainloader so check 2 types.
 */
#define PRIMARY_BUM_PATH   L"\\primary\\bootx64.efi"
#define PRIMARY_BUMLOG_PATH   L"\\primary\\log"
#define PRIMARY_GRUB_PATH  L"\\primary\\grubx64.efi"
#define PRIMARY_KEYDIR_PATH  L"\\primary\\keys"

/* Load path compare strings for boot from "backup" directory.
 *  Grub can mix forward slash and back slash in chainloader so check 2 types.
 */
#define BACKUP_BUM_PATH   L"\\backup\\bootx64.efi"
#define BACKUP_BUMLOG_PATH   L"\\backup\\log"
#define BACKUP_GRUB_PATH  L"\\backup\\grubx64.efi"
#define BACKUP_KEYDIR_PATH  L"\\backup\\keys"

/* - key-update mechanism is a mail-box mechanism
   - key-update operations are requested by the OS placing an appropriately
     named *.auth file in the appropriate key directory.
   - when attempting a boot path (primary or backup), BUM will check the key
     directory for *.auth files.
   - when a *.auth file is found, BUM will attempt a key-update based on the
     file name.
   - if the key-update operation succeeds, the corresponding *.auth file will
     be deleted to prevent repeated update attempts with the same *.auth file.
*/

typedef enum {
    BUM_KEYUPDATE_PK_UPDAT,
    BUM_KEYUPDATE_KEK_UPDAT,
    BUM_KEYUPDATE_KEK_APPND,
    BUM_KEYUPDATE_db_UPDAT,
    BUM_KEYUPDATE_db_APPND,
    BUM_KEYUPDATE_dbx_UPDAT,
    BUM_KEYUPDATE_dbx_APPND,
    BUM_KEYUPDATE_TYPE_COUNT
} BUM_KEYUPDATE_TYPE_t;

static CHAR16* BUM_KEYUPDATE_FILE_NAMES[BUM_KEYUPDATE_TYPE_COUNT] =
                {   L"PK.update.auth",
                    L"KEK.update.auth",
                    L"KEK.append.auth",
                    L"db.update.auth",
                    L"db.append.auth",
                    L"dbx.update.auth",
                    L"dbx.append.auth"};

static CHAR16 *BUM_KEYUPDATE_VAR_NAMES[BUM_KEYUPDATE_TYPE_COUNT] =
                {   EFI_PLATFORM_KEY_NAME,          /* PK */
                    EFI_KEY_EXCHANGE_KEY_NAME,      /* KEK */
                    EFI_KEY_EXCHANGE_KEY_NAME,      /* KEK */
                    EFI_IMAGE_SECURITY_DATABASE,    /* db */
                    EFI_IMAGE_SECURITY_DATABASE,    /* db */
                    EFI_IMAGE_SECURITY_DATABASE1,   /* dbx */
                    EFI_IMAGE_SECURITY_DATABASE1    /* dbx */
                };

static EFI_GUID *BUM_KEYUPDATE_VENDOR_GUIDS[BUM_KEYUPDATE_TYPE_COUNT] =
                {   &gEfiGlobalVariableGuid, /* PK */
                    &gEfiGlobalVariableGuid, /* KEK */
                    &gEfiGlobalVariableGuid, /* KEK */
                    &gEfiImageSecurityDatabaseGuid, /* db */
                    &gEfiImageSecurityDatabaseGuid, /* db */
                    &gEfiImageSecurityDatabaseGuid, /* dbx */
                    &gEfiImageSecurityDatabaseGuid, /* dbx */
                };

/* Attributes for update */
#define BUM_KEYUPDATE_ATTRIBUTES \
            (   EFI_VARIABLE_NON_VOLATILE | \
                EFI_VARIABLE_BOOTSERVICE_ACCESS | \
                EFI_VARIABLE_RUNTIME_ACCESS | \
                EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS )

/* Attributes for append */
#define BUM_KEYAPPEND_ATTRIBUTES \
            (   EFI_VARIABLE_NON_VOLATILE | \
                EFI_VARIABLE_BOOTSERVICE_ACCESS | \
                EFI_VARIABLE_RUNTIME_ACCESS | \
                EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS | \
                EFI_VARIABLE_APPEND_WRITE )

static UINT32 BUM_KEYUPDATE_ATTRIBUTE_LIST[BUM_KEYUPDATE_TYPE_COUNT] =
                {   BUM_KEYUPDATE_ATTRIBUTES, /* PK update*/
                    BUM_KEYUPDATE_ATTRIBUTES, /* KEK update*/
                    BUM_KEYAPPEND_ATTRIBUTES, /* KEK append*/
                    BUM_KEYUPDATE_ATTRIBUTES, /* db update*/
                    BUM_KEYAPPEND_ATTRIBUTES, /* db append*/
                    BUM_KEYUPDATE_ATTRIBUTES, /* dbx update*/
                    BUM_KEYAPPEND_ATTRIBUTES, /* dbx append*/
                };

#define BUM_LOG_FILE_ENABLE   TRUE /*  Set TRUE to write BUM output to a log
                                       file */

#define BUM_LOG( format, ... ) LogPrint( &logstate, format, ##__VA_ARGS__ )

/* (un|)comment the following to (en|dis)able TSC-based timing of the
   SetVariable operation
#define BUM_TIME_KEYLOAD */

/******************************************************************************/
/*  Global Variables                                                          */
/******************************************************************************/

static EFI_HANDLE gLoadedImageHandle;
static EFI_LOADED_IMAGE_PROTOCOL *gLoadedImageProtocol = NULL;
static BUM_LOADEDIMAGE_TYPE_t gLoadedImageType = BUM_LOADEDIMAGE_ROOT;

static EFI_FILE_PROTOCOL *gBootPart_RootDir = NULL;

static LogPrint_state_t logstate = ZERO_LOG_PRINT_STATE();
static EFI_FILE_PROTOCOL *LogDir = NULL;

/******************************************************************************/
/*  Boot-partition file-operation functions                                   */
/******************************************************************************/

static EFI_STATUS BUM_BootPart_OpenRootDir( void )
{
    EFI_STATUS Status;

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SimpleFileSystem;

    /*  Find the EFI_SIMPLE_FILE_SYSTEM_PROTOCOL interface for the boot
        partition. */
    Status = gBS->HandleProtocol(   gLoadedImageProtocol->DeviceHandle,
                                    &gEfiSimpleFileSystemProtocolGuid,
                                    (VOID**)&SimpleFileSystem);
    if (EFI_ERROR(Status)) {
        BUM_LOG(L"BUM_BootPart_OpenRootDir: gBS->HandleProtocol failed for "
                L"SimpleFileSystemProtocol");
        return Status;
    }

    /*  Open the boot partition. */
    Status = SimpleFileSystem->OpenVolume(  SimpleFileSystem,
                                            &gBootPart_RootDir);
    if (EFI_ERROR(Status)) {
        BUM_LOG(L"BUM_BootPart_OpenRootDir: OpenVolume failed for the"
                L"SimpleFileSystemProtocol for the boot partition.");
    }

    return Status;
}

static EFI_STATUS BUM_BootPart_CloseRootDir( void )
{
    EFI_STATUS Status;

    /* Close the EFI_FILE_PROTOCOL interface for the root directory. */
    /* Close never fails. */
    Status = gBootPart_RootDir->Close( gBootPart_RootDir );
    gBootPart_RootDir = NULL;

    return Status;
}

/******************************************************************************/
/*  Miscellaneous functions                                                   */
/******************************************************************************/

/* Print Vriable Information */
static VOID BUM_printVarInfo(   EFI_GUID *guid_p, CHAR16 *name_p,
                                UINT32 req_attributes, UINTN  req_size)
{
    EFI_STATUS Status;

    UINT32  curr_attributes;
    UINTN   curr_size;
    VOID*   curr_data;

    UINT64 MaximumVariableStorageSize;
    UINT64 RemainingVariableStorageSize;
    UINT64 MaximumVariableSize;

    /* Print Vendor GUID and Varriable Name. */
    BUM_LOG(L"        Vendor GUID:");
    BUM_LOG(L"            { %08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x }",
            guid_p->Data1, guid_p->Data2, guid_p->Data3,
            guid_p->Data4[0], guid_p->Data4[1], guid_p->Data4[2],
            guid_p->Data4[3], guid_p->Data4[4], guid_p->Data4[5],
            guid_p->Data4[6], guid_p->Data4[7] );
    BUM_LOG(L"        Name:                   %s", name_p);

    /* Get current varriable info. */
    curr_attributes = 0;
    curr_size = 0;
    Status = Common_ReadUEFIVariable( guid_p, name_p,
                                        &curr_data, &curr_size,
                                        &curr_attributes );
    if( EFI_ERROR (Status) )
        BUM_LOG(L"        Common_ReadUEFIVariable failed (%d)", Status );
    else{
        BUM_LOG(L"        Current Varriable Size: %d bytes", curr_size );
        BUM_LOG(L"        Current Attributes: 0x%08x {", curr_attributes);
        if( curr_attributes == 0 )
            BUM_LOG(L"                NONE");
        else {
            if( curr_attributes & EFI_VARIABLE_NON_VOLATILE )
                BUM_LOG(L"                NON_VOLATILE");
            if( curr_attributes & EFI_VARIABLE_BOOTSERVICE_ACCESS )
                BUM_LOG(L"                BOOTSERVICE_ACCESS");
            if( curr_attributes & EFI_VARIABLE_RUNTIME_ACCESS )
                BUM_LOG(L"                RUNTIME_ACCESS");
            if( curr_attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD)
                BUM_LOG(L"                HARDWARE_ERROR_RECORD");
            if( curr_attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS)
                BUM_LOG(L"                AUTHENTICATED_WRITE_ACCESS");
            if( curr_attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS)
                BUM_LOG(L"                TIME_BASED_AUTHENTICATED_WRITE_ACCESS");
            if( curr_attributes & EFI_VARIABLE_APPEND_WRITE)
                BUM_LOG(L"                APPEND_WRITE");
        }
        BUM_LOG(L"        }");
        Status = gBS->FreePool(curr_data);
        if( EFI_ERROR(Status) )
            BUM_LOG(L"        < gBS->FreePool failed (%d) >", Status );
    }

    /* Get varriable-storage info. */
    Status = gRT->QueryVariableInfo( (UINT32)req_attributes,
                                        &MaximumVariableStorageSize,
                                        &RemainingVariableStorageSize,
                                        &MaximumVariableSize);
    if( EFI_ERROR(Status) ){
        BUM_LOG(L"        < gRT->QueryVariableInfo failed (%d) >", Status);
    } else {
        BUM_LOG(L"        Maximum Varriable-Storage Size:   %d bytes",
                MaximumVariableStorageSize );
        BUM_LOG(L"        Remaining Varriable-Storage Size:   %d bytes",
                RemainingVariableStorageSize );
        BUM_LOG(L"        Maximum Variable Size:   %d bytes",
                MaximumVariableSize );
    }

    /* Get info on requested operation. */
    BUM_LOG(L"        Requested Write Size:   %d bytes", req_size);
    BUM_LOG(L"        Request Attributes: 0x%08x {",
            req_attributes );
    if( req_attributes == 0 )
        BUM_LOG(L"                NONE");
    else {
        if( req_attributes & EFI_VARIABLE_NON_VOLATILE )
            BUM_LOG(L"                NON_VOLATILE");
        if( req_attributes & EFI_VARIABLE_BOOTSERVICE_ACCESS )
            BUM_LOG(L"                BOOTSERVICE_ACCESS");
        if( req_attributes & EFI_VARIABLE_RUNTIME_ACCESS )
            BUM_LOG(L"                RUNTIME_ACCESS");
        if( req_attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD)
            BUM_LOG(L"                HARDWARE_ERROR_RECORD");
        if( req_attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS)
            BUM_LOG(L"                AUTHENTICATED_WRITE_ACCESS");
        if( req_attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS)
            BUM_LOG(L"                TIME_BASED_AUTHENTICATED_WRITE_ACCESS");
        if( req_attributes & EFI_VARIABLE_APPEND_WRITE)
            BUM_LOG(L"                APPEND_WRITE");
    }
    BUM_LOG(L"            }");
}

/******************************************************************************/
/*  Functions for finding loaded-image state and type                         */
/******************************************************************************/

/* Verify loaded-image type based on path name */
static EFI_STATUS EFIAPI BUM_verifyLdImageType( OUT BUM_LOADEDIMAGE_TYPE_t *LoadedImageTypep )
{
    EFI_STATUS                      Status;
    CHAR16 *LoadedImageFilePathText = NULL;
    EFI_UNICODE_COLLATION_PROTOCOL  *UnicodeCollation;
    int i;

    /*  Find the UnicodeCollation protocol which is needed to compare CHAR16
        strings */
    Status = gBS->LocateProtocol (  &gEfiUnicodeCollation2ProtocolGuid,
                                    NULL,
                                    (VOID **) &UnicodeCollation );
    if (EFI_ERROR (Status)) {
        Status = gBS->LocateProtocol (  &gEfiUnicodeCollationProtocolGuid,
                                        NULL,
                                        (VOID **) &UnicodeCollation );
        if (EFI_ERROR (Status)){
            BUM_LOG(L"BUM_verifyLdImageType: Failed to find "
                    L"EfiUnicodeCollationProtocol");
            /* Output unknown loaded-image type error and return error status*/
            *LoadedImageTypep = BUM_LOADEDIMAGE_UNKNOWN;
            return Status;
        }
    }

    /*  Obtain the file path from the loaded-image protocol and convert to
        CHAR16 */
    LoadedImageFilePathText = ConvertDevicePathToText(
                                            gLoadedImageProtocol->FilePath,
                                            FALSE, FALSE);
    if( NULL == LoadedImageFilePathText ){
        BUM_LOG(L"BUM_verifyLdImageType: ConvertDevicePathToText failed for"
                L" gLoadedImageProtocol->FilePath");
        /* Output unknown loaded-image type error and return error status*/
        *LoadedImageTypep = BUM_LOADEDIMAGE_UNKNOWN;
        return EFI_NOT_READY;
    }

    /*  Loop through path string until null-terminator.
        Convert forward slashes to back slashes. */
    for( i = 0; (0 != LoadedImageFilePathText[i]); i++ ){
        if( L'/' == LoadedImageFilePathText[i] )
            LoadedImageFilePathText[i] = L'\\';
    }

    BUM_LOG(L"    Loaded Image: \"%s\"", LoadedImageFilePathText);

    /*  Compare loaded-image path against known-expected paths to determine
        loaded-image type.*/
    if( 0 == UnicodeCollation->StriColl(UnicodeCollation,
                                        LoadedImageFilePathText,
                                        PRIMARY_BUM_PATH) ){
        *LoadedImageTypep = BUM_LOADEDIMAGE_PRIM;
    } else if ( 0 == UnicodeCollation->StriColl(UnicodeCollation,
                                                LoadedImageFilePathText,
                                                BACKUP_BUM_PATH) ){
        *LoadedImageTypep = BUM_LOADEDIMAGE_BACK;
    } else if ( 0 == UnicodeCollation->StriColl(UnicodeCollation,
                                                LoadedImageFilePathText,
                                                ROOT_BUM_PATH) ){
        *LoadedImageTypep = BUM_LOADEDIMAGE_ROOT;
    } else{
        /* Unknown loaded-image type. */
        *LoadedImageTypep = BUM_LOADEDIMAGE_UNKNOWN;
    }

    /* Free the file-path text. */
    Status = gBS->FreePool( LoadedImageFilePathText );
    if( EFI_ERROR (Status) ){
        BUM_LOG(L"BUM_verifyLdImageType: gBS->FreePool failed for "
                L"LoadedImageFilePathText.");
        /* This is not serious enough to stop booting. Fall through. */
    }

    return EFI_SUCCESS;
}

/* Set the UEFI LdImageState varriable */
static EFI_STATUS EFIAPI BUM_setLdImageState( IN BUM_LOADEDIMAGE_STATE_t state )
{
    EFI_STATUS Status;

    /* Validate the input */
    if( (state < BUM_LOADEDIMAGE_FWTOROOT)
        || (state >= BUM_LOADEDIMAGE_STATE_COUNT) ){
        BUM_LOG(L"BUM_setLdImageState: invalid state %d", state );
        return EFI_INVALID_PARAMETER;
    }

    /* Write the state */
    Status = gRT->SetVariable(  BUM_LOADEDIMAGE_STATE_VARNAME,
                                &gEfiBUMVariableGuid,
                                EFI_VARIABLE_BOOTSERVICE_ACCESS,
                                sizeof(BUM_LOADEDIMAGE_STATE_t), &state);
    if( EFI_ERROR(Status) ){
        BUM_LOG(L"BUM_setLdImageState: gRT->SetVariable failed with error"
                L"(%d) for variable: ", Status );
        BUM_printVarInfo(   &gEfiBUMVariableGuid, BUM_LOADEDIMAGE_STATE_VARNAME,
                            EFI_VARIABLE_BOOTSERVICE_ACCESS,
                            sizeof(BUM_LOADEDIMAGE_STATE_t) );
    }

    return Status;
}

/* Validate the UEFI LdImageState varriable and get its value */
static EFI_STATUS EFIAPI BUM_getLdImageState( OUT BUM_LOADEDIMAGE_STATE_t *statep )
{
    EFI_STATUS Status;

    BUM_LOADEDIMAGE_STATE_t state;

    UINT32  statevar_attributes;
    UINTN   statevar_size = sizeof(state);

    /* Get current varriable info. */
    Status = gRT->GetVariable(  BUM_LOADEDIMAGE_STATE_VARNAME,
                                &gEfiBUMVariableGuid,
                                &statevar_attributes, &statevar_size, &state);
    /*  There is an error if there is a failure to read the varriable and it
        isn't because variable is not found. Also, there is an error if the
        variable is not of the appropriate size or attribute. It is not an
        error if the variable is not found; it indicates the FWTOROOT state.
        */
    if( EFI_ERROR(Status) && (Status != EFI_NOT_FOUND) ){
        BUM_LOG(L"BUM_getLdImageState: gRT->GetVariable failed with error"
                L"(%d) for variable: ", Status );
        BUM_printVarInfo(   &gEfiBUMVariableGuid, BUM_LOADEDIMAGE_STATE_VARNAME,
                            statevar_attributes,
                            sizeof(BUM_LOADEDIMAGE_STATE_t) );
        /* since we can't determine our state propperly, set the FAIL state */
        state = BUM_LOADEDIMAGE_FAIL;
    } else if ( Status == EFI_NOT_FOUND ){
        /* Initialize the variable to FWTOROOT state */
        state = BUM_LOADEDIMAGE_FWTOROOT;
        Status = BUM_setLdImageState( state );
        if( EFI_ERROR(Status) )
            BUM_LOG(L"BUM_getLdImageState: BUM_setLdImageState failed with "
                    L"error (%d) for state %d", Status, state );
    } else /* ( ! EFI_ERROR(Status) ) */ {
        if( ( statevar_size != sizeof(BUM_LOADEDIMAGE_STATE_t) )
            || ( statevar_attributes != EFI_VARIABLE_BOOTSERVICE_ACCESS ) ){
            BUM_LOG(L"BUM_getLdImageState: invalid size or attribute for "
                    L"variable: ");
            BUM_printVarInfo(   &gEfiBUMVariableGuid, BUM_LOADEDIMAGE_STATE_VARNAME,
                                statevar_attributes,
                                sizeof(BUM_LOADEDIMAGE_STATE_t) );
            /*  if the state varriable has inappropriate size or attribute,
                the BUM didn't write it, and the varibale should be considered
                compromised. */
            Status = EFI_COMPROMISED_DATA;
            /*  since we can't determine our state propperly, set the
                FAIL state */
            state = BUM_LOADEDIMAGE_FAIL;
        }
        if( (state < BUM_LOADEDIMAGE_FWTOROOT )
            || (state >= BUM_LOADEDIMAGE_STATE_COUNT) ){
            /*  if the state varriable has inappropriate value, the BUM
                didn't write it, and the varibale should be considered
                compromised. */
            BUM_LOG(L"BUM_getLdImageState: invalid state %d", state );
            /*  since we can't determine our state propperly, set the
                FAIL state */
            state = BUM_LOADEDIMAGE_FAIL;
            Status = EFI_COMPROMISED_DATA;
        }
    }

    /* output state and return status */
    *statep = state;
    return Status;
}

static EFI_STATUS EFIAPI BUM_setLoadedImageType( void )
{
    EFI_STATUS Status, vLIT_Status;

    BUM_LOADEDIMAGE_STATE_t state;
    BUM_LOADEDIMAGE_TYPE_t lLoadedImageType;

    Status = BUM_getLdImageState( &state );
    if( EFI_ERROR(Status) ){
        BUM_LOG(L"BUM_setLoadedImageType: BUM_getLdImageState failed with "
                    L"error (%d)", Status);
        gLoadedImageType = BUM_LOADEDIMAGE_UNKNOWN;
    } else
        gLoadedImageType = BUM_STATE_TO_TYPE_TABLE[state];

    vLIT_Status = BUM_verifyLdImageType( &lLoadedImageType );
    if( EFI_ERROR(Status) ){
        BUM_LOG(L"BUM_setLoadedImageType: (WARNING) BUM_verifyLdImageType "
                    L"failed with error (%d)", vLIT_Status);
    } else {
        if( lLoadedImageType != gLoadedImageType ){
            BUM_LOG(L"BUM_setLoadedImageType: (WARNING) loaded-image path "
                    L"name is inconsistent with UEFI LdImageState varriable");
        }
    }

    return Status;
}

/******************************************************************************/
/*  Key-Loading functions                                                     */
/******************************************************************************/

static EFI_STATUS BUM_loadKeyFromFile(  BUM_KEYUPDATE_TYPE_t updateType,
                                        IN EFI_FILE_PROTOCOL *KeyDir )
{
    EFI_STATUS Status, FreePoolStat;
    EFI_FILE_PROTOCOL *KeyFileProtocol;
    VOID *KeyFileBuffer;
    UINTN KeyFileBufferSize;

    /*  We are not going to delete the key file untill the SetVar operation
        succeeds. */
    BOOLEAN KeyUpdateSuccess = FALSE;

    /* Attempt to open key file */
    Status = KeyDir->Open(  KeyDir,
                            &KeyFileProtocol,
                            BUM_KEYUPDATE_FILE_NAMES[updateType],
                            (   EFI_FILE_MODE_READ |
                                EFI_FILE_MODE_WRITE ), 0);
    if( EFI_ERROR(Status) ){
        if( Status == EFI_NOT_FOUND ){
            /* There is no such file. This is not an error. Return success. */
            Status = EFI_SUCCESS;
        } else
            BUM_LOG(L"BUM_loadKeyFromFile: Open failed for the key "
                    L"file \"%s\"", BUM_KEYUPDATE_FILE_NAMES[updateType]);
        goto exit0;
    }

    BUM_LOG(L"    BUM_loadKeyFromFile: Loading \"%s\"",
            BUM_KEYUPDATE_FILE_NAMES[updateType] );

    /* Read key file */
    Status = Common_ReadFile(   KeyFileProtocol,
                                &KeyFileBuffer, &KeyFileBufferSize);
    if( EFI_ERROR(Status) ){
        BUM_LOG(L"BUM_loadKeyFromFile: Common_ReadFile failed for \"%s\"",
                BUM_KEYUPDATE_FILE_NAMES[updateType] );
        goto exit1;
    }

#ifdef BUM_TIME_KEYLOAD
    {   /*  openning brace for the timing block. the timing block encompasses
            the timing code, the SetVariable call, and error checking for the
            call */
        UINT64  StartTSC, TSCElapsed;

        StartTSC = AsmReadTsc();
#endif
    /* SetVar with the read buffer */
    Status = gRT->SetVariable(  BUM_KEYUPDATE_VAR_NAMES[updateType],
                                BUM_KEYUPDATE_VENDOR_GUIDS[updateType],
                                BUM_KEYUPDATE_ATTRIBUTE_LIST[updateType],
                                KeyFileBufferSize, KeyFileBuffer);
#ifdef BUM_TIME_KEYLOAD
        TSCElapsed = AsmReadTsc() - StartTSC;
#endif

    if( EFI_ERROR(Status) ){
        BUM_LOG(L"BUM_loadKeyFromFile: gRT->SetVariable failed with error"
                L" (%d) for varriable:", Status);
        BUM_printVarInfo(   BUM_KEYUPDATE_VENDOR_GUIDS[updateType],
                            BUM_KEYUPDATE_VAR_NAMES[updateType],
                            BUM_KEYUPDATE_ATTRIBUTE_LIST[updateType],
                            KeyFileBufferSize);
        /*Fall through to exit2 code*/
    } else {
        /* SetVariable command succeeded. Set the Delete flag*/
        KeyUpdateSuccess = TRUE;

        /*Fall through to exit2 code*/

#ifdef BUM_TIME_KEYLOAD
        BUM_LOG(L"    gRT->SetVariable( %s, ...) timing: ",
                    BUM_KEYUPDATE_VAR_NAMES[updateType]);
        BUM_LOG(L"            %llu ticks consumed", TSCElapsed);
    } /* closing brace for the else statement. the else closing brace becomes
         the closing brace for the timing block. */
#endif

    }



/*exit2*/
    /* If control reaches here, read had succeeded, free the read buffer. */
    FreePoolStat = gBS->FreePool(KeyFileBuffer);
    if( EFI_ERROR(FreePoolStat) ){
        BUM_LOG(L"BUM_loadKeyFromFile: gBS->FreePool failed for the"
                L"key-file buffer");
        if( !EFI_ERROR(Status) )
            Status = FreePoolStat;
    }
exit1:
    /*  If the Delete flag is set (i.e. SetVar succeeded), delete the key file.
        Otherwise, Read or SetVar didn't succeed; close the key file*/
    if( KeyUpdateSuccess ){
        Status = KeyFileProtocol->Delete(KeyFileProtocol);
        if( EFI_ERROR(Status) )
            BUM_LOG(L"BUM_loadKeyFromFile: Delete failed for the key file "
                    L"\"%s\"", BUM_KEYUPDATE_FILE_NAMES[updateType]);
    } else
        KeyFileProtocol->Close(KeyFileProtocol);
exit0:
    return Status;
}

static EFI_STATUS BUM_loadKeys( IN CHAR16 *KeyDirPathText )
{
    EFI_STATUS FuncStatus, RetStatus;
    EFI_FILE_PROTOCOL *KeyDir = NULL;
    BUM_KEYUPDATE_TYPE_t i;

    BUM_LOG(L"    Loading keys from \"%s\"", KeyDirPathText);

    /* Open the key directory */
    RetStatus = gBootPart_RootDir->Open(gBootPart_RootDir,
                                        &KeyDir,
                                        KeyDirPathText,
                                        (   EFI_FILE_MODE_READ |
                                            EFI_FILE_MODE_WRITE ), 0);
    if( EFI_ERROR(RetStatus) ){
        BUM_LOG(L"BUM_loadKeys: Open failed for the key directory \"%s\"",
                KeyDirPathText);
        return RetStatus;
    }

    /*  Check for each possible key-update operation and perform it if
        requested (i.e. if the corresponding *.auth file is found). */
    for( i = BUM_KEYUPDATE_PK_UPDAT; i < BUM_KEYUPDATE_TYPE_COUNT; i++ ){
        FuncStatus = BUM_loadKeyFromFile( i, KeyDir );
        if( EFI_ERROR(FuncStatus) ){
            BUM_LOG(L"BUM_loadKeys: BUM_loadKeyFromFile failed for"
                    L" \"%s\\%s\"", KeyDirPathText,
                    BUM_KEYUPDATE_FILE_NAMES[i] );
            if( ! EFI_ERROR(RetStatus) )
                RetStatus = FuncStatus;
        }
    }

    /* Close the key directory. Close never fails. */
    KeyDir->Close( KeyDir );
    return RetStatus;
}

/******************************************************************************/
/*  Boot-Status reporting function and helper functions                       */
/******************************************************************************/

static EFI_STATUS BUM_ReportBootStatus( void )
{
    EFI_STATUS Status, CloseStatus;
    EFI_FILE_PROTOCOL *BootStatDir;

    /* Open the log directory from the boot directory. */
    Status = gBootPart_RootDir->Open(   gBootPart_RootDir, &BootStatDir,
                                        BOOTSTAT_PATH,
                                        (   EFI_FILE_MODE_READ |
                                            EFI_FILE_MODE_WRITE |
                                            EFI_FILE_MODE_CREATE ),
                                        EFI_FILE_DIRECTORY );
    if( EFI_ERROR(Status) ){
        /*  If the log directory fails to open, then disable file-based
            logging. */
        BUM_LOG(L"BUM_ReportBootStatus: Open failed for the boot-state"
                L" directory \"%s\"", BOOTSTAT_PATH);
        return Status;
    }

    Status = ReportBootStat(    BOOTSTAT_BMAP_FULL, BootStatDir,
                                BUM_LOADEDIMAGE_TYPE_TEXT[gLoadedImageType],
                                &logstate );
    if( EFI_ERROR(Status) )
        BUM_LOG(L"BUM_ReportBootStatus: ReportBootStat failed");

    CloseStatus = BootStatDir->Close( BootStatDir );
    if( EFI_ERROR(CloseStatus) )
        BUM_LOG(L"BUM_ReportBootStatus: Close failed for the boot-state"
                L" directory \"%s\"", BOOTSTAT_PATH);

    if( EFI_ERROR(Status) )
        return Status;
    else
        return CloseStatus;
}

/******************************************************************************/
/*  Logging related Initialization and Cleanup                                */
/******************************************************************************/

static EFI_STATUS BUM_LogPrint_close( void )
{
    EFI_STATUS Status1, Status2;

    /* Close all logging modes. */
    Status1 = LogPrint_close( &logstate );

    /* If a logging directory was openned, close it. */
    if( NULL != LogDir ){
        Status2 = LogDir->Close( LogDir );
        LogDir = NULL;
    } else
        Status2 = EFI_SUCCESS;

    if( EFI_ERROR(Status1) )
        return Status1;
    else
        return Status2;
}

static EFI_STATUS BUM_LogPrint_init( BOOLEAN EnableFileLogging )
{
    EFI_STATUS Status;
    static CHAR16 *LogDirPath;
    LogPrint_mode_t logmode = LOG_PRINT_MODE_CONSOLE;

    /*  Close any previous logging state. Even if the logging-state was
        previously all zero or LogPrint_close was called on it, the effect of
        this call should be benign. */
    BUM_LogPrint_close( );

    /*  Always request console-based logging. */
    logmode = LOG_PRINT_MODE_CONSOLE;

    /*  A bit more work for file-based logging. */
    if( EnableFileLogging ){

        /* Select the path of the log directory based on the boot path. */
        logmode |= LOG_PRINT_MODE_FILE;
        switch( gLoadedImageType ){
            case BUM_LOADEDIMAGE_ROOT:
            case BUM_LOADEDIMAGE_UNKNOWN:
                LogDirPath = ROOT_BUMLOG_PATH;
                break;
            case BUM_LOADEDIMAGE_PRIM:
                LogDirPath = PRIMARY_BUMLOG_PATH;
                break;
            case BUM_LOADEDIMAGE_BACK:
                LogDirPath = BACKUP_BUMLOG_PATH;
                break;
            default:
                LogDirPath = NULL;
                logmode = LOG_PRINT_MODE_CONSOLE;
                break;
        }

        if( logmode & LOG_PRINT_MODE_FILE ){
            /* Open the log directory from the boot directory. */
            Status = gBootPart_RootDir->Open(   gBootPart_RootDir, &LogDir,
                                                LogDirPath,
                                                (   EFI_FILE_MODE_READ |
                                                    EFI_FILE_MODE_WRITE |
                                                    EFI_FILE_MODE_CREATE ),
                                                EFI_FILE_DIRECTORY );
            if( EFI_ERROR(Status) ){
                /*  If the log directory fails to open, then disable file-based
                    logging. */
                logmode = LOG_PRINT_MODE_CONSOLE;
                LogDir = NULL;
            }
        }
    }

    /*  Initialize the logging code. */
    Status = LogPrint_init( &logstate, logmode, LogDir );

    /*  Even if LogPrint_init fails, the modes field inside logstate should be
        zero, and logstate should still be usable with LogPrint with no
        effect. */

    /*  If there is no file-based logging and the log directory is open,
        close it. */
    if( ((logmode & LOG_PRINT_MODE_FILE) == 0) && ( NULL != LogDir ) ){
        LogDir->Close( LogDir );
        LogDir = NULL;
    }

    /* Return the return value from LogPrint_init */
    return Status;
}

/******************************************************************************/
/*  Initialization and Cleanup                                                */
/******************************************************************************/

/* Perform initialization operations, openning protocols etc ... */
static EFI_STATUS BUM_init( IN EFI_HANDLE LoadedImageHandle )
{
    EFI_STATUS Status;

    gLoadedImageHandle = LoadedImageHandle;

    /* Use the loaded-image handle to open the loaded-image protocol */
    Status = gBS->OpenProtocol( LoadedImageHandle,
                                &gEfiLoadedImageProtocolGuid,
                                (VOID**)&gLoadedImageProtocol,
                                LoadedImageHandle,
                                NULL,
                                EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if( EFI_ERROR (Status) ){
        BUM_LOG(L"BUM_init: gBS->OpenProtocol failed for "
                L"LoadedImageProtocol");
        return Status;
    }

    /* Determine the load-path of the image (primary/backup/other). */
    Status = BUM_setLoadedImageType();
    if( EFI_ERROR (Status) ){
        BUM_LOG(L"BUM_init: BUM_setLoadedImageType failed");
        return Status;
    }

    /* Open the root directory of the boot partition for file operations. */
    Status = BUM_BootPart_OpenRootDir( );
    if( EFI_ERROR (Status) ){
        BUM_LOG(L"BUM_init: BUM_BootPart_OpenRootDir failed");
        return Status;
    }

    /* OpenRootDir succeeded. */
    /* Reset the logging code to use file-based logging as well. */
    Status = BUM_LogPrint_init( BUM_LOG_FILE_ENABLE );
    if( EFI_ERROR(Status) )
        BUM_LOG(L"BUM_init: 'BUM_LogPrint_init( TRUE )' failed");

    return EFI_SUCCESS;
}

/* Perform clean-up operations, undo initialization operations in reverse */
static EFI_STATUS BUM_fini( void )
{
    EFI_STATUS Status;

    /*  Close file-based logging but maintain console-based logging. */
    Status = BUM_LogPrint_init( FALSE );
    if( EFI_ERROR(Status) )
        BUM_LOG(L"BUM_fini: 'BUM_LogPrint_init( FALSE )' failed");

    /* Close the root directory of the boot partition */
    Status = BUM_BootPart_CloseRootDir( );
    if( EFI_ERROR (Status) ){
        BUM_LOG(L"BUM_fini: BUM_BootPart_CloseRootDir failed");
        return Status;
    }

    /* Reset the loaded-image type to the default value */
    gLoadedImageType = BUM_LOADEDIMAGE_ROOT;

    /* Close the loaded-image protocol */
    Status = gBS->CloseProtocol( gLoadedImageHandle,
                                &gEfiLoadedImageProtocolGuid,
                                gLoadedImageHandle,
                                NULL);
    if( EFI_ERROR (Status) ){
        BUM_LOG(L"BUM_fini: gBS->CloseProtocol failed for "
                L"LoadedImageProtocol");
        return Status;
    }

    /* Reset the protocol interface pointer and image handle to NULL */
    gLoadedImageProtocol = NULL;
    gLoadedImageHandle = NULL;

    /* Close all logging. */
    BUM_LogPrint_close( );

    return EFI_SUCCESS;
}


/******************************************************************************/
/*  Image-Booting Functions                                                   */
/******************************************************************************/

#define KEYDIR_NAME "keys"
#define PATHLEN_MAX (512)

static EFI_STATUS EFIAPI BUM_GetPathFromParts(  IN  CHAR8   *DirPath,
                                                IN  CHAR8   *FileName,
                                                OUT CHAR16  **Path_p)
{
    EFI_STATUS ret;
    UINTN DirPath_len, FileName_len, Path_len;
    CHAR16 *Path;
    /*  Get lengths of the two strings to be generated */
    DirPath_len = AsciiStrnLenS(DirPath, PATHLEN_MAX);
    FileName_len = AsciiStrnLenS(FileName, PATHLEN_MAX);
    Path_len = DirPath_len + 1 + FileName_len;
    if(PATHLEN_MAX < Path_len){
        BUM_LOG(L"BUM_GetPathFromParts: total path length (%d) too large",
                Path_len);
        ret = EFI_UNSUPPORTED;
        goto exit0;
    }
    /*  Allocate space for Path */
    ret = gBS->AllocatePool(EfiLoaderData,
                            (Path_len+1)*sizeof(CHAR16),
                            (VOID**)&Path);
    if(EFI_ERROR(ret)){
        BUM_LOG(L"BUM_GetPathFromParts: gBS->AllocatePool failed");
        goto exit0;
    }
    /*  Generate the path */
    ret = AsciiStrToUnicodeStrS(DirPath,
                                Path,
                                DirPath_len+1);
    if(EFI_ERROR(ret)){
        BUM_LOG(L"BUM_GetPathFromParts: AsciiStrToUnicodeStrS failed "
                L"for directory path");
        goto exit1;
    }
    Path[DirPath_len] = L'\\';
    ret = AsciiStrToUnicodeStrS(FileName,
                                &(Path[DirPath_len+1]),
                                FileName_len+1);
    if(EFI_ERROR(ret)){
        BUM_LOG(L"BUM_GetPathFromParts: AsciiStrToUnicodeStrS failed"
                L"for file name");
        goto exit1;
    }
    /*  Set output variables */
    *Path_p = Path;
exit1:
    if(EFI_ERROR(ret)){
        EFI_STATUS c_ret = gBS->FreePool(Path);
        if(EFI_ERROR(c_ret))
            BUM_LOG(L"BUM_GetPathFromParts: gBS->FreePool failed");
    }
exit0:
    return ret;
}

static EFI_STATUS EFIAPI BUM_GetImageKeyDirPath(IN  CHAR8   *ConfigDirPath,
                                                IN  CHAR8   *ImageName,
                                                OUT CHAR16  **ImagePath_p,
                                                OUT CHAR16  **KeydirPath_p)
{
    EFI_STATUS ret;
    CHAR16 *KeydirPath, *ImagePath;
    ret = BUM_GetPathFromParts( ConfigDirPath,
                                KEYDIR_NAME,
                                &KeydirPath);
    if(EFI_ERROR(ret))
        BUM_LOG(L"BUM_GetImageKeyDirPath: BUM_GetPathFromParts failed"
                L"for the key directory.");
    else{
        ret = BUM_GetPathFromParts( ConfigDirPath,
                                    ImageName,
                                    &ImagePath);
        if(EFI_ERROR(ret)){
            EFI_STATUS cleanup_ret;
            BUM_LOG(L"BUM_GetImageKeyDirPath: BUM_GetPathFromParts failed"
                    L"for the image path.");
            cleanup_ret = gBS->FreePool(KeydirPath);
            if(EFI_ERROR(cleanup_ret))
                BUM_LOG(L"BUM_GetImageKeyDirPath: gBS->FreePool failed");
        }else{
            *ImagePath_p = ImagePath;
            *KeydirPath_p= KeydirPath;
        }
    }
    /*  Generate the two strings */
    return ret;
}

static EFI_STATUS EFIAPI BUM_FreeImageKeyDirPath(   IN  CHAR16  *ImagePath,
                                                    IN  CHAR16  *KeydirPath)
{
    EFI_STATUS ret1, ret2;
    ret1 = gBS->FreePool(ImagePath);
    if(EFI_ERROR(ret1)){
        BUM_LOG(L"BUM_FreeImageKeyDirPath: gBS->FreePool failed for ImagePath");
    }
    ret2 = gBS->FreePool(KeydirPath);
    if(EFI_ERROR(ret2)){
        BUM_LOG(L"BUM_FreeImageKeyDirPath: gBS->FreePool failed");
    }
    return EFI_ERROR(ret1)? ret1 : ret2;
}

static EFI_STATUS EFIAPI BUM_LoadImage( IN  CHAR16      *ImagePathString,
                                        OUT EFI_HANDLE  *LoadedImageHandle_p)
{
    EFI_STATUS ret;
    EFI_DEVICE_PATH_PROTOCOL *imageDPPp;
    EFI_HANDLE LoadedImageHandle;
    /* Turn the CHAR16 file path into a popper DevicePathProtocol */
    imageDPPp = FileDevicePath( gLoadedImageProtocol->DeviceHandle,
                                ImagePathString);
    if(NULL == imageDPPp){
        BUM_LOG(L"BUM_LoadImage: FileDevicePath returned NULL");
        ret = EFI_OUT_OF_RESOURCES;
    }else{
        /* Load the image using the DevicePathProtocol */
        ret = gBS->LoadImage(   FALSE, /*   Request does not originate from the 
                                            boot manager in the firmware. */
                                gLoadedImageHandle,
                                imageDPPp,
                                NULL, 0, /* There are no images loaded in
                                            memory */
                                &LoadedImageHandle);
        if(EFI_ERROR(ret))
            BUM_LOG(L"BUM_LoadImage: gBS->LoadImage returned (%d) ",  ret);
        else
            *LoadedImageHandle_p = LoadedImageHandle;
        /* Succeed or fail, the DevicePathProtocol should be freed. */
        gBS->FreePool(imageDPPp);
    }
    return ret;
}

static EFI_STATUS EFIAPI BUM_LoadKeyLoadImage(  IN CHAR8        *ConfigDirPath,
                                                IN CHAR8        *ImageName,
                                                IN BOOLEAN      LoadKeysByDefault,
                                                IN EFI_HANDLE   *LoadedImageHandle_p)
{
    EFI_STATUS ret, LoadKey_ret, cleanup_ret;
    CHAR16 *ImagePathString, *KeyDirPathString;
    EFI_HANDLE LoadedImageHandle;
    ret = BUM_GetImageKeyDirPath(   ConfigDirPath,
                                    ImageName,
                                    &ImagePathString,
                                    &KeyDirPathString);
    if(EFI_ERROR(ret))
        BUM_LOG(L"BUM_LoadKeyLoadImage: BUM_GetImageKeyDirPath failed (%d)",
                ret);
    else {
        BUM_LOG(L"    Loading image \"%s\" ... ", ImagePathString);
        /*  Load keys by default if requested */
        if(LoadKeysByDefault){
            LoadKey_ret = BUM_loadKeys(KeyDirPathString);
            /*  Failure in loading keys is not fatal */
        }else
            LoadKey_ret = EFI_SUCCESS;
        /*  Make first attempt at loading the image */
        ret = BUM_LoadImage(ImagePathString,
                            &LoadedImageHandle);
        if(EFI_ERROR(ret)){
            /*  Check if we already attempted to load keys */
            if(!LoadKeysByDefault){
                LoadKey_ret = BUM_loadKeys(KeyDirPathString);
                if(!EFI_ERROR(LoadKey_ret))
                    ret = BUM_LoadImage(ImagePathString,
                                        &LoadedImageHandle);
            }
        }
        /*  Report any loading errors */
        if(EFI_ERROR(LoadKey_ret))
            BUM_LOG(L"BUM_LoadKeyLoadImage: BUM_loadKeys failed for "
                    L"\"%s\"",  KeyDirPathString);
        if(EFI_ERROR(ret))
            BUM_LOG(L"BUM_LoadKeyLoadImage: BUM_LoadImage failed for "
                    L"\"%s\"",  ImagePathString);
        else
            *LoadedImageHandle_p = LoadedImageHandle;
        /*  Free the path-string buffers */
        cleanup_ret = BUM_FreeImageKeyDirPath(  ImagePathString,
                                                KeyDirPathString);
        if(EFI_ERROR(cleanup_ret))
            BUM_LOG(L"BUM_LoadKeyLoadImage: BUM_FreeImageKeyDirPath "
                    L"failed (%d)", cleanup_ret);
    }
    return ret;
}

static EFI_STATUS EFIAPI BUM_SetStateBootImage( IN EFI_HANDLE LoadedImageHandle,
                                                IN BUM_LOADEDIMAGE_STATE_t State,
                                                IN BOOLEAN ReportBootStatus )
{
    EFI_STATUS ret;
    /*  Set the UEFI LdImageState varriable.
        We don't want to load an image without setting the state first since
        the loaded image will not behave correctly without the propper state.
        If we fail to set the state, mark this as a fatal error and abort the
        boot.*/
    ret = BUM_setLdImageState( State );
    if(EFI_ERROR(ret)){
        BUM_LOG(L"BUM_SetStateBootImage: BUM_setLdImageState "
                L"failed (%d) for state %d",  ret, State);
        /*  FIXME: UnloadImage
            cleanup_ret = */
    }else{
        /* Report Boot Status if requested */
        if(ReportBootStatus){
            ret = BUM_ReportBootStatus();
            if(EFI_ERROR(ret))
                BUM_LOG(L"BUM_SetStateBootImage: BUM_ReportBootStatus failed");
        }
        BUM_LOG(L"    Starting image ...");
        ret = gBS->StartImage(LoadedImageHandle, NULL, NULL);
        if(EFI_ERROR(ret)){
            BUM_LOG(L"BUM_SetStateBootImage: gBS->StartImage failed (%d)", ret);
        }
    }
    /*  Control will reach here if gBS->StartImage failed or the loaded image
        exited or returned. Either way, this is a failure. At some stage of
        the boot, something failed to load. */
    return ret;
}

EFI_STATUS EFIAPI BUM_loadKeysSetStateBootImage(IN CHAR8    *ConfigDirPath,
                                                IN CHAR8    *ImageName,
                                                IN BUM_LOADEDIMAGE_STATE_t State,
                                                IN BOOLEAN  LoadKeysByDefault,
                                                IN BOOLEAN  ReportBootStatus )
{
    EFI_STATUS ret;
    EFI_HANDLE LoadedImageHandle;
    /*  Load images and keys. */
    ret = BUM_LoadKeyLoadImage( ConfigDirPath,
                                ImageName,
                                LoadKeysByDefault,
                                &LoadedImageHandle);
    if(EFI_ERROR(ret)){
        BUM_LOG(L"BUM_loadKeysSetStateBootImage: BUM_LoadKeyLoadImage failed"
                L"(%d)",  ret);

    }else{
        /*  Boot the image. */
        ret = BUM_SetStateBootImage(LoadedImageHandle,
                                    State,
                                    ReportBootStatus);
        if(EFI_ERROR(ret))
            BUM_LOG(L"BUM_loadKeysSetStateBootImage: BUM_SetStateBootImage failed"
                    L"(%d)", ret);
    }
    return ret;
}

/******************************************************************************/
/*  Main                                                                      */
/******************************************************************************/

#define PRIMARY_CONFIGDIR   "primary"
#define BACKUP_CONFIGDIR    "backup"
#define BUM_IMAGENAME       "bootx64.efi"
#define PAYLOAD_IMAGENAME   "grubx64.efi"

EFI_STATUS BUM_back_main( VOID )
{
    EFI_STATUS ret;
    /*  Try to load the keys from the backup directory and boot the
        backup GRUB image. Since we are launching GRUB, set boot
        status. */
    ret = BUM_loadKeysSetStateBootImage(BACKUP_CONFIGDIR,
                                        PAYLOAD_IMAGENAME,
                                        BUM_LOADEDIMAGE_BACKTOGRUB,
                                        TRUE,
                                        TRUE);
    if(EFI_ERROR(ret)){
        /* Failed to boot the backup GRUB image. */
        BUM_LOG(L"BUM_back_main: BUM_loadKeysSetStateBootImage "
                L"failed (%d)", ret);
    }
    return ret;
}

EFI_STATUS BUM_prim_main( VOID )
{
    EFI_STATUS ret;
    /*  Try to load the keys from the primary directory and boot the
        primary GRUB image. Since we are launching GRUB, set boot
        status. */
    ret = BUM_loadKeysSetStateBootImage(PRIMARY_CONFIGDIR,
                                        PAYLOAD_IMAGENAME,
                                        BUM_LOADEDIMAGE_PRIMTOGRUB,
                                        TRUE,
                                        TRUE);
    if(EFI_ERROR(ret)){
        /*  Failed to boot the primary GRUB image. */
        BUM_LOG(L"BUM_prim_main: BUM_loadKeysSetStateBootImage "
                L"failed (%d)", ret);
    }
    return ret;
}

EFI_STATUS BUM_root_main( VOID )
{
    EFI_STATUS ret;
    /*  Try to boot the primary UEFI application without loading keys
        or setting boot status.*/
    ret = BUM_loadKeysSetStateBootImage(PRIMARY_CONFIGDIR,
                                        BUM_IMAGENAME,
                                        BUM_LOADEDIMAGE_ROOTTOPRIM,
                                        FALSE,
                                        FALSE);
    if(EFI_ERROR(ret)){
        /* Try to boot the backup UEFI application without loading
           keys or setting boot status. */
        ret = BUM_loadKeysSetStateBootImage(BACKUP_CONFIGDIR,
                                            BUM_IMAGENAME,
                                            BUM_LOADEDIMAGE_ROOTTOBACK,
                                            FALSE,
                                            FALSE);
        if(EFI_ERROR(ret)){
            /* Failed to boot the backup UEFI application. */
            BUM_LOG(L"BUM_main: All Root Load Attempts Failed");
            BUM_LOG(L"BUM_main: BUM_loadKeysSetStateBootImage"
                    L" final error value (%d)", ret);
        }
    }
    return ret;
}

EFI_STATUS EFIAPI BUM_main( IN EFI_HANDLE       LoadedImageHandle,
                            IN EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS AppStatus = EFI_SUCCESS;
    EFI_STATUS FuncStatus;
    /*  Setup initial console-based logging */
    BUM_LogPrint_init( FALSE );
    BUM_LOG( L"initializing BUM logging ... " );
    FuncStatus = BUM_init(LoadedImageHandle);
    if(EFI_ERROR(FuncStatus)){
        AppStatus = FuncStatus;
        goto exit0;
    }
    /*  Log initial information */
    BUM_LOG(L"****************************************"
            L"****************************************" );
    BUM_LOG(L"    Boot Update Manager" );
    BUM_LOG(L"****************************************"
            L"****************************************" );
    BUM_LOG(L"");
    BUM_LOG(L"    Loaded Image Type: %s",
            BUM_LOADEDIMAGE_TYPE_TEXT[gLoadedImageType] );
    switch(gLoadedImageType){
        case BUM_LOADEDIMAGE_ROOT:
            /*  This is the root BUM. */
            AppStatus = BUM_root_main();
            BUM_LOG(L"BUM_main: BUM_root_main failed with status (%d)\n",
                    AppStatus);
            break;
        case BUM_LOADEDIMAGE_PRIM:
            /*  This is the primary UEFI application. */
            AppStatus = BUM_prim_main();
            BUM_LOG(L"BUM_main: BUM_prim_main failed with status (%d)\n",
                    AppStatus);
            break;
        case BUM_LOADEDIMAGE_BACK:
            /*  This is the backup UEFI application. */
            AppStatus = BUM_back_main();
            BUM_LOG(L"BUM_main: BUM_back_main failed with status (%d)\n",
                    AppStatus);
            break;
        case BUM_LOADEDIMAGE_UNKNOWN:
            BUM_LOG(L"BUM_main: loaded-image type = \"ERROR\"");
            AppStatus = EFI_LOAD_ERROR;
            break;
        default :
            BUM_LOG(L"BUM_main: Unsupported loaded-image type");
            AppStatus = EFI_UNSUPPORTED;
            break;
    }
    /*  Attempt to set the EFI State Varriable.
        Already in failure mode, so return value doesn't matter. */
    BUM_setLdImageState(BUM_LOADEDIMAGE_FAIL);
    FuncStatus = BUM_fini();
    if(EFI_ERROR(FuncStatus)){
        /*  The status error from BUM_fini is only important if there were no
            major previous errors. */
        if(!EFI_ERROR(AppStatus))
            AppStatus = FuncStatus;
    }
exit0:
    return EFI_UNSUPPORTED;
}

