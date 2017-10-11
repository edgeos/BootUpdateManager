/* LibSmBios.h - Function headers for LibSmBios.c
 *
 * Author: Safayet N Ahmed (GE Global Research) <Safayet.Ahmed@ge.com>
 *
 * Copyright (c) 2016, General Electric Company. All rights reserved.
 */

#ifndef __LIB_SMBIOS__
#define __LIB_SMBIOS__

EFI_STATUS EFIAPI SMBIOS_RetrieveBIOSInfo(  OUT CHAR8*  *Vendorpp,
                                            OUT UINTN*  VendorLenp,
                                            OUT CHAR8*  *Versionpp,
                                            OUT UINTN*  VersionLenp,
                                            OUT CHAR8*  *ReleaseDatepp,
                                            OUT UINTN*  ReleaseDateLenp);

#endif

