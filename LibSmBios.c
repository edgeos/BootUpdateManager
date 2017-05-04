/* LibSmBios.c - Code for accessing SmBios information
 *
 * Author: Safayet N Ahmed (GE Global Research) <Safayet.Ahmed@ge.com>
 *
 * Based on following code:
 *      BiosInforExpert.(c/h) by Glenn Smith
 *      MdeModulePkg/Application/UiApp/FrontPage.c from edk2
 *          Copyright (c) 2004 - 2016, Intel Corporation
 *      MdeModulePkg/Universal/SmBiosDxe/SmBiosDxe.c
 *          Copyright (c) 2009 - 2016, Intel Corporation
 *
 * Copyright (c) 2016, General Electric Company. All rights reserved.
 */

#include "__LibSmBios.h"

static EFI_STATUS GetSMBiosTableString( IN  CHAR8   *StringList,
                                        IN  CHAR8   *StringList_lim,
                                        IN  SMBIOS_TABLE_STRING Index,
                                        OUT CHAR8*  *Stringp,
                                        OUT UINTN   *StringLenp)
{
    EFI_STATUS Status;

    CHAR8   *String, *String_next;
    UINTN   StringLen;

    /* Make sure the StringList and StringList_lim make sense*/
    if( StringList >= StringList_lim)
        return EFI_INVALID_PARAMETER;

    /* Handle special case */
    if (Index == 0) {
        /* Return a CHAR16 buffer containing a single NULL terminator */
        String = "\0";
        StringLen = 0;
        Status = EFI_SUCCESS;
    } else {
        SMBIOS_TABLE_STRING String_i;
        CHAR8   *String_off;

        /* Assume the Index string is found (loop exits without breaking). */
        Status = EFI_SUCCESS;

        /*  Starting with the first string, iterate over the string list until
                1) reaching the desired index in the list (loop-end condition)
                2) or reaching a zero-length string (end of the string list, break-condition 1)
                3) or falling off the known end of the string list (break-condition 2) */
        for(String_i = 0, String_next = StringList;
                String_i < Index; String_i++ ){
            /*  Starting with the first character, iterate over the string.
                Loop until reaching the NULL terminator or the end of the
                string list. */
            for(String_off = String_next;
                 (( String_off[0] != '\0') && (String_off < StringList_lim));
                    String_off++ ){}

            /*  Only if the NULL terminator was reached (still within the list)
                AND after the start of the string (not a zero-length string),
                count this as a valid string. */
            if( (*String_off == '\0') && (String_off != String_next) ){
                /*  Compute the string length from the offset. */
                String = String_next;
                StringLen = String_off - String;
                /*  Move String_next to the next string. */
                String_next = String_off + 1;
            }else{
                /*  A zero-length string was reached (end-of-list marker)
                    or the NULL terminator was not reached (which necessarily
                    means that (String_off >= StringList_lim)) */
                String = "\0";
                StringLen = 0;
                Status = EFI_NOT_FOUND;
                break;
            }
        }
    }

    /* Check if String and StringLen contain valid values */
    if( ! EFI_ERROR(Status) ){
        CHAR8* String_out;
        RETURN_STATUS StrConvStatus;

        /* Allocate output buffers for string and null terminator */
        Status = gBS->AllocatePool( EfiLoaderData,
                                    ((StringLen+1)*sizeof(CHAR8)),
                                    (VOID**)(&String_out));
        if(! EFI_ERROR(Status)){
            /* Copy String to the output buffer */
            StrConvStatus = AsciiStrnCpyS(  String_out, (StringLen+1),
                                            String, StringLen);
            if( RETURN_SUCCESS != StrConvStatus ){
                /* Free the allocated output buffer */
                gBS->FreePool(String_out);
                Status = EFI_INVALID_PARAMETER;
            } else {
                /* Explicitly set the NULL terminator */
                String_out[StringLen] = L'\0';
                /* Set the output arguments*/
                *Stringp = String_out;
                *StringLenp = StringLen;
            }
        }
    }

    return Status;
}

EFI_STATUS EFIAPI SMBIOS_RetrieveBIOSInfo(  OUT CHAR8*  *Vendorpp,
                                            OUT UINTN*  VendorLenp,
                                            OUT CHAR8*  *Versionpp,
                                            OUT UINTN*  VersionLenp,
                                            OUT CHAR8*  *ReleaseDatepp,
                                            OUT UINTN*  ReleaseDateLenp)
{
    EFI_STATUS  Status;
    EFI_SMBIOS_PROTOCOL *EfiSmbiosProtocol;
    EFI_SMBIOS_HANDLE   SmbiosHandle;
    EFI_SMBIOS_TYPE     SmbiosType;
    SMBIOS_STRUCTURE_POINTER    SmbiosStructPtr;

    UINTN   TableMaxLen;
    CHAR8  *StringList;
    CHAR8  *StringList_lim;

    SMBIOS_TABLE_STRING Index;
    CHAR8*  Vendorp;
    UINTN   VendorLen;
    CHAR8*  Versionp;
    UINTN   VersionLen;
    CHAR8*  ReleaseDatep;
    UINTN   ReleaseDateLen;

    /*  Locate the SMBIOS protocol handle */
    Status = gBS->LocateProtocol(   &gEfiSmbiosProtocolGuid,
                                    NULL, (VOID **)&EfiSmbiosProtocol);
    if( EFI_ERROR(Status) )
        goto exit0;

    /*  Loop until finding the SMBIOS_TABLE_TYPE0 */
    SmbiosHandle    = SMBIOS_HANDLE_PI_RESERVED;
    SmbiosType      = SMBIOS_TYPE_BIOS_INFORMATION;
    Status = EfiSmbiosProtocol->GetNext(EfiSmbiosProtocol,
                                        &SmbiosHandle, &SmbiosType,
                                        &(SmbiosStructPtr.Hdr), NULL);
    if( EFI_ERROR(Status) )
        goto exit0;

    /*  Get the maximum possible table length */
    if( EfiSmbiosProtocol->MajorVersion < 3 )
        TableMaxLen = SMBIOS_TABLE_MAX_LENGTH;
    else
        TableMaxLen = SMBIOS_3_0_TABLE_MAX_LENGTH;

    /*  compute the start and limit of the string list */
    StringList      = (CHAR8*)SmbiosStructPtr.Hdr + SmbiosStructPtr.Hdr->Length;
    StringList_lim  = (CHAR8*)SmbiosStructPtr.Hdr + TableMaxLen;

    /*  check for overflow in the limit of the string list */
    if( StringList_lim < StringList ){
        /*  in case of overflow, set to maximum possible value before overflow*/
        StringList_lim = ((CHAR8*)NULL - 1);
    }

    /*  Get the Vendor */
    Index = SmbiosStructPtr.Type0->Vendor;
    Status = GetSMBiosTableString(  StringList, StringList_lim, Index,
                                    &Vendorp, &VendorLen);
    if( EFI_ERROR(Status) )
        goto exit0;

    /*  Get the Version */
    Index = SmbiosStructPtr.Type0->BiosVersion;
    Status = GetSMBiosTableString(  StringList, StringList_lim, Index,
                                    &Versionp, &VersionLen);
    if( EFI_ERROR(Status) ){
        gBS->FreePool(Vendorp);
        goto exit0;
    }

    /*  Get the Release */
    Index = SmbiosStructPtr.Type0->BiosReleaseDate;
    Status = GetSMBiosTableString(  StringList, StringList_lim, Index,
                                    &ReleaseDatep, &ReleaseDateLen);
    if( EFI_ERROR(Status) ){
        gBS->FreePool(Vendorp);
        gBS->FreePool(Versionp);
        goto exit0;
    }

    /*  Set the output variables */
    *Vendorpp           = Vendorp;
    *VendorLenp         = VendorLen;
    *Versionpp          = Versionp;
    *VersionLenp        = VersionLen;
    *ReleaseDatepp      = ReleaseDatep;
    *ReleaseDateLenp    = ReleaseDateLen;

exit0:
    return Status;
}

