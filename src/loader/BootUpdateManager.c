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

/* (un|)comment the following to (en|dis)able TSC-based timing of the
   SetVariable operation
#define BUM_TIME_KEYLOAD */

/******************************************************************************/
/*  Global Variables                                                          */
/******************************************************************************/

static EFI_HANDLE gLoadedImageHandle;
static EFI_LOADED_IMAGE_PROTOCOL *gLoadedImageProtocol = NULL;

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
    LogPrint(L"        Vendor GUID:");
    LogPrint(L"            { %08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x }",
                guid_p->Data1, guid_p->Data2, guid_p->Data3,
                guid_p->Data4[0], guid_p->Data4[1], guid_p->Data4[2],
                guid_p->Data4[3], guid_p->Data4[4], guid_p->Data4[5],
                guid_p->Data4[6], guid_p->Data4[7] );
    LogPrint(L"        Name:                   %s", name_p);

    /* Get current varriable info. */
    curr_attributes = 0;
    curr_size = 0;
    Status = Common_ReadUEFIVariable( guid_p, name_p,
                                        &curr_data, &curr_size,
                                        &curr_attributes );
    if( EFI_ERROR (Status) )
        LogPrint(L"        Common_ReadUEFIVariable failed (%d)", Status );
    else{
        LogPrint(L"        Current Varriable Size: %d bytes", curr_size );
        LogPrint(L"        Current Attributes: 0x%08x {", curr_attributes);
        if( curr_attributes == 0 )
            LogPrint(L"                NONE");
        else {
            if( curr_attributes & EFI_VARIABLE_NON_VOLATILE )
                LogPrint(L"                NON_VOLATILE");
            if( curr_attributes & EFI_VARIABLE_BOOTSERVICE_ACCESS )
                LogPrint(L"                BOOTSERVICE_ACCESS");
            if( curr_attributes & EFI_VARIABLE_RUNTIME_ACCESS )
                LogPrint(L"                RUNTIME_ACCESS");
            if( curr_attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD)
                LogPrint(L"                HARDWARE_ERROR_RECORD");
            if( curr_attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS)
                LogPrint(L"                AUTHENTICATED_WRITE_ACCESS");
            if( curr_attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS)
                LogPrint(L"                TIME_BASED_AUTHENTICATED_WRITE_ACCESS");
            if( curr_attributes & EFI_VARIABLE_APPEND_WRITE)
                LogPrint(L"                APPEND_WRITE");
        }
        LogPrint(L"        }");
        Status = gBS->FreePool(curr_data);
        if( EFI_ERROR(Status) )
            LogPrint(L"        < gBS->FreePool failed (%d) >", Status );
    }

    /* Get varriable-storage info. */
    Status = gRT->QueryVariableInfo( (UINT32)req_attributes,
                                        &MaximumVariableStorageSize,
                                        &RemainingVariableStorageSize,
                                        &MaximumVariableSize);
    if( EFI_ERROR(Status) ){
        LogPrint(L"        < gRT->QueryVariableInfo failed (%d) >", Status);
    } else {
        LogPrint(L"        Maximum Varriable-Storage Size:   %d bytes",
                    MaximumVariableStorageSize );
        LogPrint(L"        Remaining Varriable-Storage Size:   %d bytes",
                    RemainingVariableStorageSize );
        LogPrint(L"        Maximum Variable Size:   %d bytes",
                    MaximumVariableSize );
    }

    /* Get info on requested operation. */
    LogPrint(L"        Requested Write Size:   %d bytes", req_size);
    LogPrint(L"        Request Attributes: 0x%08x {",
                req_attributes );
    if( req_attributes == 0 )
        LogPrint(L"                NONE");
    else {
        if( req_attributes & EFI_VARIABLE_NON_VOLATILE )
            LogPrint(L"                NON_VOLATILE");
        if( req_attributes & EFI_VARIABLE_BOOTSERVICE_ACCESS )
            LogPrint(L"                BOOTSERVICE_ACCESS");
        if( req_attributes & EFI_VARIABLE_RUNTIME_ACCESS )
            LogPrint(L"                RUNTIME_ACCESS");
        if( req_attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD)
            LogPrint(L"                HARDWARE_ERROR_RECORD");
        if( req_attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS)
            LogPrint(L"                AUTHENTICATED_WRITE_ACCESS");
        if( req_attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS)
            LogPrint(L"                TIME_BASED_AUTHENTICATED_WRITE_ACCESS");
        if( req_attributes & EFI_VARIABLE_APPEND_WRITE)
            LogPrint(L"                APPEND_WRITE");
    }
    LogPrint(L"            }");
}

/******************************************************************************/
/*  Cur-config functions                                                     */
/******************************************************************************/

#define BUM_CURCONFIG_CFGBUMPRFX   "BUM:"
#define BUM_CURCONFIG_CFGPLDPRFX   "PLD:"
#define BUM_CURCONFIG_VARSIZE   (sizeof(BUM_CURCONFIG_CFGBUMPRFX)\
                                    +   BUMSTATE_CONFIG_SIZE)
#define BUM_CURCONFIG_VARNAME   L"BUM_CURCONFIG"

typedef enum {
    BUM_CURIMAGE_ROOTBUM = 0,
    BUM_CURIMAGE_CFGBUM = 1,
    BUM_CURIMAGE_CFGPLD = 2,
    BUM_CURIMAGE_TYPE_COUNT
} BUM_CURIMAGE_TYPE_t;

static CHAR16* BUM_CURIMAGE_TYPE_TEXT[BUM_CURIMAGE_TYPE_COUNT+1]
                = { L"Root BUM",
                    L"Config BUM",
                    L"Config Payload",
                    L"Unknown" };

static EFI_STATUS EFIAPI BUM_parseConfigVarValue(
                            IN  CHAR8 configvar[static BUM_CURCONFIG_VARSIZE],
                            OUT BUM_CURIMAGE_TYPE_t *imgtype_p,
                            OUT CHAR8 config[static BUMSTATE_CONFIG_MAXLEN])
{
    EFI_STATUS ret;
    UINTN prefix_size = sizeof(BUM_CURCONFIG_CFGBUMPRFX)/sizeof(CHAR8) - 1;
    CHAR8 *configsrc = &(configvar[prefix_size]);
    /*  Check the prefix to see if this is a config BUM */
    if(0 == AsciiStrnCmp(   configvar,
                            BUM_CURCONFIG_CFGBUMPRFX,
                            prefix_size)){
        ret = CopyConfig(config,configsrc);
        if(!EFI_ERROR(ret))
            *imgtype_p = BUM_CURIMAGE_CFGBUM;
    }else{
        /*  Check the prefix to see if this is a config payload */
        if(0 == AsciiStrnCmp(   configvar,
                                BUM_CURCONFIG_CFGPLDPRFX,
                                prefix_size)){
            ret = CopyConfig(config,configsrc);
            if(!EFI_ERROR(ret))
                *imgtype_p = BUM_CURIMAGE_CFGPLD;
        } else {
            /*  If its neither, the state is corrupted */
            ret = EFI_COMPROMISED_DATA;
        }
    }
    return ret;
}

static EFI_STATUS EFIAPI BUM_setConfigVarValue(
                            OUT CHAR8 configvar[static BUM_CURCONFIG_VARSIZE],
                            IN  BUM_CURIMAGE_TYPE_t imgtype,
                            IN  CHAR8 config[static BUMSTATE_CONFIG_MAXLEN])
{
    EFI_STATUS ret;
    CHAR8 *prefix_src;
    UINTN prefix_len;
    CHAR8 *configdst;
    /*  Determine the prefix and prefix length based on image type */
    switch(imgtype){
        case BUM_CURIMAGE_CFGBUM:
            prefix_src  = BUM_CURCONFIG_CFGBUMPRFX;
            prefix_len  = sizeof(BUM_CURCONFIG_CFGBUMPRFX)/sizeof(CHAR8) - 1;
            configdst   = &(configvar[prefix_len]);
            break;

        case BUM_CURIMAGE_CFGPLD:
            prefix_src  = BUM_CURCONFIG_CFGPLDPRFX;
            prefix_len  = sizeof(BUM_CURCONFIG_CFGPLDPRFX)/sizeof(CHAR8) - 1;
            configdst   = &(configvar[prefix_len]);
            break;

        default:
            /*  unknown image type --> no prefix */
            prefix_src  = "";
            prefix_len  = 0;
            configdst   = configvar;
            ret = EFI_UNSUPPORTED;
            break;
    }
    /*  Only set the output buffer if there is a non-zero length prefix */
    if(prefix_len > 0){
        /*  Copy the prefix */
        ret = AsciiStrnCpyS(configvar,
                            BUM_CURCONFIG_VARSIZE*sizeof(CHAR8),
                            prefix_src,
                            prefix_len);
        if(!EFI_ERROR(ret)){
            /*  Copy the config */
            ret = AsciiStrnCpyS(configdst,
                                (BUM_CURCONFIG_VARSIZE-prefix_len)*sizeof(CHAR8),
                                config,
                                BUMSTATE_CONFIG_MAXLEN);
        }
    }
    return ret;
}

static EFI_STATUS EFIAPI BUM_deleteVar( IN  CHAR16      *VariableName,
                                        IN  EFI_GUID    *VendorGuid)
{
    EFI_STATUS ret;
    /* Delete the variable */
    ret = gRT->SetVariable( VariableName,
                            VendorGuid,
                            0,
                            0,
                            NULL);
    if(EFI_ERROR(ret) && (ret != EFI_NOT_FOUND))
        LogPrint(L"BUM_deleteVar: gRT->SetVariable with zero size and attributes"
                    L"returned %d.", ret);
    return ret;
}

static EFI_STATUS EFIAPI BUM_setCurConfigVar(
                            IN  CHAR8 configvar[static BUM_CURCONFIG_VARSIZE])
{
    EFI_STATUS ret;
    ret = gRT->SetVariable( BUM_CURCONFIG_VARNAME,
                            &gEfiBUMVariableGuid,
                            EFI_VARIABLE_BOOTSERVICE_ACCESS,
                            BUM_CURCONFIG_VARSIZE,
                            configvar);
    if(EFI_ERROR(ret))
        LogPrint(L"BUM_setCurConfigVar: gRT->SetVariable returned %d",
                    ret);
    return ret;
}

static EFI_STATUS EFIAPI BUM_getCurConfigVar(
                            OUT CHAR8 configvar[static BUM_CURCONFIG_VARSIZE])
{
    EFI_STATUS ret;
    UINTN   configvar_size;
    UINT32  configvar_attributes;
    /* Read the variable */
    configvar_size = BUM_CURCONFIG_VARSIZE;
    ret = gRT->GetVariable( BUM_CURCONFIG_VARNAME,
                            &gEfiBUMVariableGuid,
                            &configvar_attributes,
                            &configvar_size,
                            configvar);
    /*  There is an error if there is a failure to read the varriable and it
        isn't because variable is not found. Also, there is an error if the
        variable is not of the appropriate size or attribute. It is not an
        error if the variable is not found; it indicates the FWTOROOT state.
        */
    if( !EFI_ERROR(ret) ){
        if( (configvar_size != BUM_CURCONFIG_VARSIZE)
            || (configvar_attributes != EFI_VARIABLE_BOOTSERVICE_ACCESS) ){
            LogPrint(L"BUM_getCurConfigVar: invalid size or attribute for "
                        L"variable: ");
            BUM_printVarInfo(   &gEfiBUMVariableGuid,
                                BUM_CURCONFIG_VARNAME,
                                configvar_attributes,
                                BUM_CURCONFIG_VARSIZE );
            /*  if the state varriable has inappropriate size or attribute,
                the BUM didn't write it, and the varibale should be considered
                compromised. */
            ret = EFI_COMPROMISED_DATA;
        }
    }
    return ret;
}

static EFI_STATUS EFIAPI BUM_getCurConfig(
                                OUT BUM_CURIMAGE_TYPE_t *imgtype_p,
                                OUT CHAR8 config[static BUMSTATE_CONFIG_MAXLEN])
{
    EFI_STATUS ret;
    CHAR8   configvar[BUM_CURCONFIG_VARSIZE];
    /* Read the variable */
    ret = BUM_getCurConfigVar( configvar );
    if( EFI_ERROR(ret) ){
        /*  If the variable is not present, assume this is the root BUM */
        if(ret == EFI_NOT_FOUND){
            *imgtype_p = BUM_CURIMAGE_ROOTBUM;
            config[0] = '\0';
            ret = EFI_SUCCESS;
        }else{
            LogPrint(L"BUM_getCurConfig: BUM_getCurConfigVar returned %d",
                        ret);
        }
    }else{
        /*  Parse the variable */
        ret = BUM_parseConfigVarValue(  configvar,
                                        imgtype_p,
                                        config);
        if(EFI_ERROR(ret)){
            LogPrint(L"BUM_getCurConfig: BUM_parseConfigVarValue returned %d",
                        ret);
        }
    }
    /*  If the variable is inappropriately set, delete it. */
    if(EFI_ERROR(ret)){
        EFI_STATUS cleanup = BUM_deleteVar( BUM_CURCONFIG_VARNAME,
                                            &gEfiBUMVariableGuid);
        if(EFI_ERROR(cleanup))
            LogPrint(L"BUM_getCurConfig: BUM_deleteVar returned %d",
                        cleanup);
    }
    return ret;
}

static EFI_STATUS EFIAPI BUM_setCurConfig(
                                IN  BUM_CURIMAGE_TYPE_t imgtype,
                                IN  CHAR8 config[static BUMSTATE_CONFIG_MAXLEN])
{
    EFI_STATUS ret;
    CHAR8 configvar[BUM_CURCONFIG_VARSIZE];
    ret = BUM_setConfigVarValue(configvar,
                                imgtype,
                                config);
    if(EFI_ERROR(ret)){
        LogPrint(L"BUM_setCurConfig: BUM_setConfigVarValue returned %d",
                    ret);
    }else{
        ret = BUM_setCurConfigVar(configvar);
        if(EFI_ERROR(ret)){
            LogPrint(L"BUM_setCurConfig: BUM_setCurConfigVar returned %d",
                        ret);
        }
    }
    return ret;
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
            LogPrint(L"BUM_loadKeyFromFile: Open failed for the key "
                        L"file \"%s\"", BUM_KEYUPDATE_FILE_NAMES[updateType]);
        goto exit0;
    }

    LogPrint(L"    BUM_loadKeyFromFile: Loading \"%s\"",
                BUM_KEYUPDATE_FILE_NAMES[updateType] );

    /* Read key file */
    Status = Common_ReadFile(   KeyFileProtocol,
                                &KeyFileBuffer, &KeyFileBufferSize);
    if( EFI_ERROR(Status) ){
        LogPrint(L"BUM_loadKeyFromFile: Common_ReadFile failed for \"%s\"",
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
        LogPrint(L"BUM_loadKeyFromFile: gRT->SetVariable failed with error"
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
        LogPrint(L"    gRT->SetVariable( %s, ...) timing: ",
                    BUM_KEYUPDATE_VAR_NAMES[updateType]);
        LogPrint(L"            %llu ticks consumed", TSCElapsed);
    } /* closing brace for the else statement. the else closing brace becomes
         the closing brace for the timing block. */
#endif

    }



/*exit2*/
    /* If control reaches here, read had succeeded, free the read buffer. */
    FreePoolStat = Common_FreeReadBuffer(   KeyFileBuffer,
                                            KeyFileBufferSize);
    if( EFI_ERROR(FreePoolStat) ){
        LogPrint(L"BUM_loadKeyFromFile: gBS->FreePool failed for the"
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
            LogPrint(L"BUM_loadKeyFromFile: Delete failed for the key file "
                        L"\"%s\"", BUM_KEYUPDATE_FILE_NAMES[updateType]);
    } else
        KeyFileProtocol->Close(KeyFileProtocol);
exit0:
    return Status;
}

#define KEYDIR_NAME "keys"

static EFI_STATUS BUM_loadKeys( IN CHAR8 *CfgDirPathText )
{
    EFI_STATUS FuncStatus, RetStatus;
    CHAR16 *KeyDirPathText;
    EFI_FILE_PROTOCOL *KeyDir = NULL;
    BUM_KEYUPDATE_TYPE_t i;

    RetStatus = Common_GetPathFromParts(CfgDirPathText,
                                        KEYDIR_NAME,
                                        &KeyDirPathText);
    LogPrint(L"    Loading keys from \"%s\"", KeyDirPathText);
    if(EFI_ERROR(RetStatus))
        LogPrint(L"BUM_loadKeys: Common_GetPathFromParts failed (%d) "
                    L"for the config directory \"%s\"",
                    CfgDirPathText);
    else{
        /* Open the key directory */
        RetStatus = Common_OpenFile(&KeyDir,
                                    KeyDirPathText,
                                    (EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE));
        if( EFI_ERROR(RetStatus) )
            LogPrint(L"BUM_loadKeys: Common_OpenFile failed (%d) "
                        L"for the key directory \"%s\"",
                        RetStatus, KeyDirPathText);
        else{
            /*  Check for each possible key-update operation and perform it if
                requested (i.e. if the corresponding *.auth file is found). */
            for(i = BUM_KEYUPDATE_PK_UPDAT; i < BUM_KEYUPDATE_TYPE_COUNT; i++){
                FuncStatus = BUM_loadKeyFromFile( i, KeyDir );
                if( EFI_ERROR(FuncStatus) ){
                    LogPrint(L"BUM_loadKeys: BUM_loadKeyFromFile failed for"
                                L" \"%s\\%s\"", KeyDirPathText,
                            BUM_KEYUPDATE_FILE_NAMES[i] );
                    if( ! EFI_ERROR(RetStatus) )
                        RetStatus = FuncStatus;
                }
            }
            /* Close the key directory. Close never fails. */
            KeyDir->Close( KeyDir );
        }
        /*  Free the path string */
        FuncStatus = Common_FreePath(KeyDirPathText);
        if(EFI_ERROR(FuncStatus)){
            LogPrint(L"BUM_loadKeys: Common_FreePath failed (%d)",
                        FuncStatus);
            if(!EFI_ERROR(RetStatus))
                RetStatus = FuncStatus;
        }
    }
    return RetStatus;
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
    if(!EFI_ERROR(Status)){
        /* Open the root directory of the boot partition for file operations. */
        Status = Common_FileOpsInit(gLoadedImageProtocol->DeviceHandle);
        if(!EFI_ERROR(Status)){
            /*  Setup logging */
            LogPrint_init();
            Status = EFI_SUCCESS;
        }
    }
    return Status;
}

/* Perform clean-up operations, undo initialization operations in reverse */
static EFI_STATUS BUM_fini( void )
{
    EFI_STATUS Status;
    /* Close the root directory of the boot partition */
    Status = Common_FileOpsClose( );
    if(!EFI_ERROR(Status)){
        /* Close the loaded-image protocol */
        Status = gBS->CloseProtocol( gLoadedImageHandle,
                                    &gEfiLoadedImageProtocolGuid,
                                    gLoadedImageHandle,
                                    NULL);
        if(!EFI_ERROR(Status)){
            /* Reset the protocol interface pointer and image handle to NULL */
            gLoadedImageProtocol = NULL;
            gLoadedImageHandle = NULL;
            Status = EFI_SUCCESS;
        }
    }
    return Status;
}


/******************************************************************************/
/*  Image-Booting Functions                                                   */
/******************************************************************************/
static EFI_STATUS EFIAPI BUM_LoadImage( IN CHAR8        *ConfigDirPath,
                                        IN CHAR8        *ImageName,
                                        OUT EFI_HANDLE  *LoadedImageHandle_p)
{
    EFI_STATUS ret, cleanup_ret;
    CHAR16 *ImagePathString;
    EFI_DEVICE_PATH_PROTOCOL *imageDPPp;
    EFI_HANDLE LoadedImageHandle;
    /*  Generate the image path */
    ret = Common_GetPathFromParts(  ConfigDirPath,
                                    ImageName,
                                    &ImagePathString);
    if(EFI_ERROR(ret))
        LogPrint(L"BUM_LoadImage: Common_GetPathFromParts failed (%d)",
                    ret);
    else{
        LogPrint(L"    Loading image \"%s\" ... ", ImagePathString);
        /* Turn the CHAR16 file path into a popper DevicePathProtocol */
        imageDPPp = FileDevicePath( gLoadedImageProtocol->DeviceHandle,
                                    ImagePathString);
        if(NULL == imageDPPp){
            LogPrint(L"BUM_LoadImage: FileDevicePath returned NULL");
            ret = EFI_OUT_OF_RESOURCES;
        }else{
            /* Load the image using the DevicePathProtocol */
            ret = gBS->LoadImage(   FALSE, /*   Request does not originate from
                                                the boot manager in the
                                                firmware. */
                                    gLoadedImageHandle,
                                    imageDPPp,
                                    NULL, 0, /* There are no images loaded in
                                                memory */
                                    &LoadedImageHandle);
            if(EFI_ERROR(ret))
                LogPrint(L"BUM_LoadImage: gBS->LoadImage returned (%d) ",  ret);
            else
                *LoadedImageHandle_p = LoadedImageHandle;
            /* Succeed or fail, the DevicePathProtocol should be freed. */
            gBS->FreePool(imageDPPp);
        }
        /* Succeed or fail, the image path should be freed. */
        cleanup_ret = Common_FreePath(ImagePathString);
        if(EFI_ERROR(cleanup_ret)){
            LogPrint(L"BUM_LoadImage: Common_FreePath returned (%d) ",
                        cleanup_ret);
            if(!EFI_ERROR(ret))
                ret = cleanup_ret;
        }
    }
    return ret;
}

static EFI_STATUS EFIAPI BUM_LoadKeyLoadImage(  IN CHAR8        *ConfigDirPath,
                                                IN CHAR8        *ImageName,
                                                IN BOOLEAN      LoadKeysByDefault,
                                                IN EFI_HANDLE   *LoadedImageHandle_p)
{
    EFI_STATUS ret, LoadKey_ret;
    EFI_HANDLE LoadedImageHandle;

    /*  Load keys by default if requested */
    if(LoadKeysByDefault){
        LoadKey_ret = BUM_loadKeys(ConfigDirPath);
        /*  Failure in loading keys is not fatal */
    }else
        LoadKey_ret = EFI_SUCCESS;
    /*  Make first attempt at loading the image */
    ret = BUM_LoadImage(ConfigDirPath,
                        ImageName,
                        &LoadedImageHandle);
    if(EFI_ERROR(ret)){
        /*  Check if we already attempted to load keys */
        if(!LoadKeysByDefault){
            LoadKey_ret = BUM_loadKeys(ConfigDirPath);
            if(!EFI_ERROR(LoadKey_ret))
                ret = BUM_LoadImage(ConfigDirPath,
                                    ImageName,
                                    &LoadedImageHandle);
        }
    }
    /*  Report any loading errors */
    if(EFI_ERROR(LoadKey_ret))
        LogPrint(L"BUM_LoadKeyLoadImage: BUM_loadKeys failed for "
                    L"\"%a\"",  ConfigDirPath);
    if(EFI_ERROR(ret))
        LogPrint(L"BUM_LoadKeyLoadImage: BUM_LoadImage failed for "
                    L"\"%a\\%a\"",  ConfigDirPath, ImageName);
    else
        *LoadedImageHandle_p = LoadedImageHandle;
    return ret;
}

static EFI_STATUS EFIAPI BUM_SetConfigBootImage(
                                IN  EFI_HANDLE LoadedImageHandle,
                                IN  BUM_CURIMAGE_TYPE_t imgtype,
                                IN  CHAR8 Config[static BUMSTATE_CONFIG_MAXLEN], 
                                IN  BOOLEAN ReportBootStatus )
{
    EFI_STATUS ret;
    ret = BUM_setCurConfig( imgtype,
                            Config);
    if(EFI_ERROR(ret)){
        LogPrint(L"BUM_SetStateBootImage: BUM_setCurConfig "
                    L"failed (%d) for Config \"%a\"", ret, Config);
    }else{
        /* Report Boot Status if requested */
        if(ReportBootStatus){
            ret = ReportBootStat(BOOTSTAT_BMAP_FULL);
            if(EFI_ERROR(ret))
                LogPrint(L"BUM_SetStateBootImage: ReportBootStat failed");
        }
        LogPrint(L"    Starting image ...");
        ret = gBS->StartImage(LoadedImageHandle, NULL, NULL);
        if(EFI_ERROR(ret)){
            LogPrint(L"BUM_SetStateBootImage: gBS->StartImage failed (%d)", ret);
        }
        LogPrint(L"BUM_SetConfigBootImage: StartImage returned control ... ");
        /*  If control reaches here, reboot the system */
        LogPrint(L"BUM_SetConfigBootImage: rebooting ... ");
        gRT->ResetSystem(   EfiResetCold,
                            ret,
                            0, NULL);
    }
    return ret;
}

EFI_STATUS EFIAPI BUM_loadKeysSetStateBootImage(
                                IN  CHAR8   Config[static BUMSTATE_CONFIG_MAXLEN],
                                IN  CHAR8   *ImageName,
                                IN  BUM_CURIMAGE_TYPE_t imgtype,
                                IN  BOOLEAN  LoadKeysByDefault,
                                IN  BOOLEAN  ReportBootStatus )
{
    EFI_STATUS ret;
    EFI_HANDLE LoadedImageHandle;
    /*  Load images and keys. */
    ret = BUM_LoadKeyLoadImage( Config,
                                ImageName,
                                LoadKeysByDefault,
                                &LoadedImageHandle);
    if(EFI_ERROR(ret)){
        LogPrint(L"BUM_loadKeysSetStateBootImage: "
                    L"BUM_LoadKeyLoadImage failed (%d)",  ret);

    }else{
        /*  Boot the image. */
        ret = BUM_SetConfigBootImage(   LoadedImageHandle,
                                        imgtype,
                                        Config,
                                        ReportBootStatus);
        LogPrint(L"BUM_loadKeysSetStateBootImage: "
                        L"BUM_SetStateBootImage returned (%d)", ret);
        /*FIXME: unload image here */
    }
    return ret;
}

/******************************************************************************/
/*  Main                                                                      */
/******************************************************************************/

#define BUM_STATEDIR        "\\bumstate"
#define BUM_IMAGENAME       "bootx64.efi"
#define PAYLOAD_IMAGENAME   "payload.efi"

EFI_STATUS BUM_config_main( IN  CHAR8   Config[static BUMSTATE_CONFIG_MAXLEN])
{
    EFI_STATUS ret;
    /*  Try to load the keys from the primary directory and boot the
        primary GRUB image. Since we are launching GRUB, set boot
        status. */
    ret = BUM_loadKeysSetStateBootImage(Config,
                                        PAYLOAD_IMAGENAME,
                                        BUM_CURIMAGE_CFGPLD,
                                        TRUE,
                                        TRUE);
    if(EFI_ERROR(ret)){
        /*  Failed to boot the primary payload image. */
        LogPrint(L"BUM_config_main: BUM_loadKeysSetStateBootImage "
                    L"failed for \"%a\" (%d)", Config, ret);
    }
    return ret;
}

EFI_STATUS BUM_root_main( VOID )
{
    EFI_STATUS ret, cleanup_ret;
    BUM_state_t *BUM_state_p;
    char Config[BUMSTATE_CONFIG_MAXLEN];
    /*  Get the BUM state */
    ret = BUMState_Get(BUM_STATEDIR, &BUM_state_p);
    if(EFI_ERROR(ret))
        LogPrint(L"BUM_root_main: BUMState_Get failed (%d)\n",
                    ret);
    else{
        /*  Perform the boot-time logic. */
        BUMStateNext_BootTime(BUM_state_p);
        /*  Get the actual configurtaion name from the state */
        ret = BUMState_getCurrConfig(BUM_state_p, Config);
        /*  Write the state back out to file */
        cleanup_ret = BUMState_Put( BUM_STATEDIR,
                                    BUM_state_p);
        if(EFI_ERROR(cleanup_ret))
            LogPrint(L"BUM_root_main: BUMState_Put failed (%d)\n",
                        cleanup_ret);
        /*  Free the state */
        cleanup_ret = BUMState_Free(BUM_state_p);
        if(EFI_ERROR(cleanup_ret))
            LogPrint(L"BUM_root_main: BUMState_Free failed (%d)\n",
                        cleanup_ret);
        /*  Check if we successfully got the configurtaion name */
        if(EFI_ERROR(ret))
            LogPrint(L"BUM_root_main: BUMState_getCurrConfig failed (%d)\n",
                        ret);
        else{
            /*  Try to boot the configurtaion-specific BUM image */
            ret = BUM_loadKeysSetStateBootImage(Config,
                                                BUM_IMAGENAME,
                                                BUM_CURIMAGE_CFGBUM,
                                                FALSE,
                                                FALSE);
            if(EFI_ERROR(ret))
                LogPrint(L"BUM_root_main: BUM_loadKeysSetStateBootImage "
                            L"failed (%d)\n", ret);
        }
    }
    return ret;
}

EFI_STATUS EFIAPI BUM_main( IN EFI_HANDLE       LoadedImageHandle,
                            IN EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS AppStatus = EFI_SUCCESS;
    EFI_STATUS FuncStatus;
    BUM_CURIMAGE_TYPE_t imgtype = BUM_CURIMAGE_TYPE_COUNT;
    CHAR8   ConfigLocal[BUMSTATE_CONFIG_MAXLEN]= "\0";

    FuncStatus = BUM_init(LoadedImageHandle);
    if(EFI_ERROR(FuncStatus)){
        AppStatus = FuncStatus;
        goto exit0;
    }
    /*  Log initial information */
    LogPrint(L"****************************************"
                L"****************************************" );
    LogPrint(L"    Boot Update Manager" );
    LogPrint(L"****************************************"
                L"****************************************" );
    AppStatus = BUM_getCurConfig(  &imgtype,
                                   ConfigLocal);
    if(EFI_ERROR(AppStatus)){
        LogPrint(L"BUM_main: BUM_getCurConfig failed with status (%d)",
                    AppStatus);
    }else{
        LogPrint_setContextLabel(BUM_CURIMAGE_TYPE_TEXT[imgtype]);
        switch(imgtype){
            case BUM_CURIMAGE_ROOTBUM:
                /*  This is the root BUM. */
                AppStatus = BUM_root_main();
                LogPrint(L"BUM_main: BUM_root_main failed with status (%d)",
                            AppStatus);
                break;
            case BUM_CURIMAGE_CFGBUM:
                LogPrint(L"    Configuration:    %a",
                            ConfigLocal);
                /*  This is a configuration BUM. */
                AppStatus = BUM_config_main(ConfigLocal);
                LogPrint(L"BUM_main: BUM_config_main failed with status (%d)",
                            AppStatus);
                break;
            default:
                /*  Something is wrong. */
                LogPrint(L"BUM_main: Unknown loaded-image type");
                AppStatus = EFI_UNSUPPORTED;
                break;
        }
    }
    /*  Cleanup */
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

