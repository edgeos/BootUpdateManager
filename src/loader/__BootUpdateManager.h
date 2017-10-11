/* __BootUpdateManager.h - Include header files for BootUpdateManager.c
 *
 * Author: Safayet N Ahmed (GE Global Research) <Safayet.Ahmed@ge.com>
 * 
 * Based on initial idea and code by Glenn Smith
 *
 * Copyright (c) 2016, General Electric Company. All rights reserved.
 */

#ifndef ____BOOT_UPDATE_MANAGER__
#define ____BOOT_UPDATE_MANAGER__

#include <Library/BaseLib.h>
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DevicePathLib.h>

#include <Protocol/LoadedImage.h>
#include <Protocol/DevicePath.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>
#include <Guid/GlobalVariable.h>
#include <Guid/ImageAuthentication.h>
#include <Protocol/UnicodeCollation.h>

#include "LibCommon.h"
#include "LogPrint.h"
#include "BootStat.h"

#endif

