/* BootStat.h - Macros, structure definitions, and function headers for
 *              BootStat.c
 *
 * Author: Safayet N Ahmed (GE Global Research) <Safayet.Ahmed@ge.com>
 *
 * Copyright (c) 2016, General Electric Company. All rights reserved.
 */

#ifndef __BOOT_STAT__
#define __BOOT_STAT__

typedef enum {
    BOOTSTAT_ENUM_TIMESTAMP     = 0,
    BOOTSTAT_ENUM_SMBIOSINFO    = 1,
    BOOTSTAT_ENUM_UEFINVINFO    = 2,
    BOOTSTAT_ENUM_UEFIVARS      = 3,
    BOOTSTAT_ENUM_COUNT
} BootStat_enum_t;

#define IMAGENAME_MAX (512)

#define BOOTSTAT_BMAP_TIMESTAMP     (1 <<   BOOTSTAT_ENUM_TIMESTAMP)
#define BOOTSTAT_BMAP_SMBIOSINFO    (1 <<   BOOTSTAT_ENUM_SMBIOSINFO)
#define BOOTSTAT_BMAP_UEFINVINFO    (1 <<   BOOTSTAT_ENUM_UEFINVINFO)
#define BOOTSTAT_BMAP_UEFIVARS      (1 <<   BOOTSTAT_ENUM_UEFIVARS)

#define BOOTSTAT_BMAP_FULL          ((1 <<  BOOTSTAT_ENUM_COUNT) - 1)
#define BOOTSTAT_BMAP_VALID         BOOTSTAT_BMAP_FULL

EFI_STATUS EFIAPI ReportBootStat(   IN UINT64               BootStat_bitmap,
                                    IN EFI_FILE_PROTOCOL    *BootStat_dir,
                                    IN LogPrint_state_t     *logstatep_l );

#endif
