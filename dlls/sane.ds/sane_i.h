/*
 * Copyright 2000 Corel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef _TWAIN32_H
#define _TWAIN32_H

#ifndef __WINE_CONFIG_H
# error You must include config.h first
#endif

#include <stdarg.h>

#ifdef SONAME_LIBSANE
# include <sane/sane.h>
# include <sane/saneopts.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "twain.h"

#ifdef SONAME_LIBSANE
#define MAKE_FUNCPTR(f) extern typeof(f) * p##f DECLSPEC_HIDDEN;
MAKE_FUNCPTR(sane_init)
MAKE_FUNCPTR(sane_exit)
MAKE_FUNCPTR(sane_get_devices)
MAKE_FUNCPTR(sane_open)
MAKE_FUNCPTR(sane_close)
MAKE_FUNCPTR(sane_get_option_descriptor)
MAKE_FUNCPTR(sane_control_option)
MAKE_FUNCPTR(sane_get_parameters)
MAKE_FUNCPTR(sane_start)
MAKE_FUNCPTR(sane_read)
MAKE_FUNCPTR(sane_cancel)
MAKE_FUNCPTR(sane_set_io_mode)
MAKE_FUNCPTR(sane_get_select_fd)
MAKE_FUNCPTR(sane_strstatus)
#undef MAKE_FUNCPTR
#endif

extern HINSTANCE SANE_instance DECLSPEC_HIDDEN;

#define TWCC_CHECKSTATUS     (TWCC_CUSTOMBASE + 1)

/* internal information about an active data source */
struct tagActiveDS
{
    struct tagActiveDS	*next;			/* next active DS */
    TW_IDENTITY		identity;		/* identity of the DS */
    TW_UINT16		currentState;		/* current state */
    TW_UINT16		twCC;			/* condition code */
    TW_IDENTITY         appIdentity;            /* identity of the app */
    HWND		hwndOwner;		/* window handle of the app */
    HWND		progressWnd;		/* window handle of the scanning window */
#ifdef SONAME_LIBSANE
    SANE_Handle		deviceHandle;		/* device handle */
    SANE_Parameters     sane_param;             /* parameters about the image
                                                   transferred */
    BOOL                sane_param_valid;  /* true if valid sane_param*/
    BOOL                sane_started;      /* If sane_start has been called */
    INT                 deviceIndex;    /* index of the current device */
#endif
    /* Capabilities */
    TW_UINT16		capXferMech;		/* ICAP_XFERMECH */
    BOOL                PixelTypeSet;
    TW_UINT16		defaultPixelType;		/* ICAP_PIXELTYPE */
    BOOL                XResolutionSet;
    TW_FIX32            defaultXResolution;
    BOOL                YResolutionSet;
    TW_FIX32            defaultYResolution;
};

extern struct tagActiveDS activeDS DECLSPEC_HIDDEN;

/* Helper functions */
extern TW_UINT16 SANE_SaneCapability (pTW_CAPABILITY pCapability, TW_UINT16 action) DECLSPEC_HIDDEN;
extern TW_UINT16 SANE_SaneSetDefaults (void) DECLSPEC_HIDDEN;
extern void SANE_Notify (TW_UINT16 message) DECLSPEC_HIDDEN;

/* Implementation of operation triplets
 * From Application to Source (Control Information) */
TW_UINT16 SANE_CapabilityGet (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_CapabilityGetCurrent
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_CapabilityGetDefault
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_CapabilityQuerySupport
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_CapabilityReset
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_CapabilitySet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_CustomDSDataGet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_CustomDSDataSet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_AutomaticCaptureDirectory
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_ChangeDirectory
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_FileSystemCopy
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_CreateDirectory
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_FileSystemDelete
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_FormatMedia
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_FileSystemGetClose
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_FileSystemGetFirstFile
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_FileSystemGetInfo
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_FileSystemGetNextFile
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_FileSystemRename
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_ProcessEvent
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_PassThrough
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_PendingXfersEndXfer
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_PendingXfersGet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_PendingXfersReset
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_PendingXfersStopFeeder
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_SetupFileXferGet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_SetupFileXferGetDefault
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_SetupFileXferReset
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_SetupFileXferSet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_SetupFileXfer2Get
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_SetupFileXfer2GetDefault
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_SetupFileXfer2Reset
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_SetupFileXfer2Set
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_SetupMemXferGet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_GetDSStatus
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_DisableDSUserInterface
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_EnableDSUserInterface
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_EnableDSUIOnly
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_XferGroupGet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_XferGroupSet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;

/* Implementation of operation triplets
 * From Application to Source (Image Information) */
TW_UINT16 SANE_CIEColorGet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_ExtImageInfoGet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_GrayResponseReset
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_GrayResponseSet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_ImageFileXferGet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_ImageInfoGet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_ImageLayoutGet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_ImageLayoutGetDefault
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_ImageLayoutReset
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_ImageLayoutSet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_ImageMemXferGet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_ImageNativeXferGet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_JPEGCompressionGet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_JPEGCompressionGetDefault
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_JPEGCompressionReset
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_JPEGCompressionSet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_Palette8Get
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_Palette8GetDefault
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_Palette8Reset
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_Palette8Set
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_RGBResponseReset
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;
TW_UINT16 SANE_RGBResponseSet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData) DECLSPEC_HIDDEN;

/* UI function */
BOOL DoScannerUI(void) DECLSPEC_HIDDEN;
HWND ScanningDialogBox(HWND dialog, LONG progress) DECLSPEC_HIDDEN;

/* Option functions */
#ifdef SONAME_LIBSANE
SANE_Status sane_option_get_int(SANE_Handle h, const char *option_name, SANE_Int *val) DECLSPEC_HIDDEN;
SANE_Status sane_option_set_int(SANE_Handle h, const char *option_name, SANE_Int val, SANE_Int *status) DECLSPEC_HIDDEN;
SANE_Status sane_option_get_str(SANE_Handle h, const char *option_name, SANE_String val, size_t len, SANE_Int *status) DECLSPEC_HIDDEN;
SANE_Status sane_option_set_str(SANE_Handle h, const char *option_name, SANE_String val, SANE_Int *status) DECLSPEC_HIDDEN;
SANE_Status sane_option_probe_resolution(SANE_Handle h, const char *option_name, SANE_Int *minval, SANE_Int *maxval, SANE_Int *quant) DECLSPEC_HIDDEN;
SANE_Status sane_option_probe_mode(SANE_Handle h, SANE_String_Const **choices, char *current, int current_size) DECLSPEC_HIDDEN;
SANE_Status sane_option_probe_scan_area(SANE_Handle h, const char *option_name, SANE_Fixed *val,
                                        SANE_Unit *unit, SANE_Fixed *min, SANE_Fixed *max, SANE_Fixed *quant) DECLSPEC_HIDDEN;
SANE_Status sane_option_get_bool(SANE_Handle h, const char *option_name, SANE_Bool *val, SANE_Int *status) DECLSPEC_HIDDEN;
SANE_Status sane_option_set_bool(SANE_Handle h, const char *option_name, SANE_Bool val, SANE_Int *status) DECLSPEC_HIDDEN;
SANE_Status sane_option_set_fixed(SANE_Handle h, const char *option_name, SANE_Fixed val, SANE_Int *status) DECLSPEC_HIDDEN;
TW_UINT16 sane_status_to_twcc(SANE_Status rc) DECLSPEC_HIDDEN;
BOOL convert_sane_res_to_twain(double sane_res, SANE_Unit unit, TW_FIX32 *twain_res, TW_UINT16 twtype) DECLSPEC_HIDDEN;
#endif


#endif
