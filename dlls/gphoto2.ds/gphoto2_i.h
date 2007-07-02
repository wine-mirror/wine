/*
 * Copyright 2000 Corel Corporation
 * Copyright 2006 Marcus Meissner
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

#if defined(HAVE_GPHOTO2) && !defined(HAVE_JPEGLIB_H)
# warning "gphoto2 support in twain needs jpeg development headers"
# undef HAVE_GPHOTO2
#endif

#ifdef HAVE_GPHOTO2
/* Hack for gphoto2, which changes behaviour when WIN32 is set. */
#undef WIN32
#include <gphoto2/gphoto2-camera.h>
#define WIN32
#endif

#include <stdio.h>

#ifdef HAVE_JPEGLIB_H
/* This is a hack, so jpeglib.h does not redefine INT32 and the like*/
# define XMD_H
# define UINT8 JPEG_UINT8
# define UINT16 JPEG_UINT16
# undef FAR
#  include <jpeglib.h>
# undef UINT16
# ifndef SONAME_LIBJPEG
#  define SONAME_LIBJPEG "libjpeg" SONAME_EXT
# endif
#endif

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "twain.h"

#include "wine/list.h"

extern HINSTANCE GPHOTO2_instance;

struct gphoto2_file  {
    struct list entry;

    char	*folder;
    char	*filename;
    BOOL	download;	/* flag for downloading, set by GUI or so */
};

/* internal information about an active data source */
struct tagActiveDS
{
    TW_IDENTITY		identity;		/* identity */
    TW_UINT16		currentState;		/* current state */
    TW_EVENT		pendingEvent;		/* pending event to be sent to
                                                   application */
    TW_UINT16		twCC;			/* condition code */
    HWND		hwndOwner;		/* window handle of the app */
    HWND		progressWnd;		/* window handle of the scanning window */

#ifdef HAVE_GPHOTO2
    Camera		*camera;
    GPContext		*context;
#endif

    /* Capabiblities */
    TW_UINT32		capXferMech;		/* ICAP_XFERMECH */
    TW_UINT16		pixeltype;		/* ICAP_PIXELTYPE */
    TW_UINT16		pixelflavor;		/* ICAP_PIXELFLAVOR */

    struct list 	files;

    /* Download and decode JPEG STATE */
#ifdef HAVE_GPHOTO2
    CameraFile				*file;
#endif
#ifdef HAVE_JPEGLIB_H
    struct jpeg_source_mgr		xjsm;
    struct jpeg_decompress_struct	jd;
    struct jpeg_error_mgr		jerr;
#endif
} activeDS;

/* Helper functions */
extern TW_UINT16 GPHOTO2_SaneCapability (pTW_CAPABILITY pCapability, TW_UINT16 action);

/*  */
extern TW_UINT16 GPHOTO2_ControlGroupHandler (
	pTW_IDENTITY pOrigin, TW_UINT16 DAT, TW_UINT16 MSG, TW_MEMREF pData);
extern TW_UINT16 GPHOTO2_ImageGroupHandler (
	pTW_IDENTITY pOrigin, TW_UINT16 DAT, TW_UINT16 MSG, TW_MEMREF pData);
extern TW_UINT16 GPHOTO2_AudioGroupHandler (
	pTW_IDENTITY pOrigin, TW_UINT16 DAT, TW_UINT16 MSG, TW_MEMREF pData);
extern TW_UINT16 GPHOTO2_SourceManagerHandler (
	pTW_IDENTITY pOrigin, TW_UINT16 DAT, TW_UINT16 MSG, TW_MEMREF pData);

/* Implementation of operation triplets
 * From Application to Source (Control Information) */
TW_UINT16 GPHOTO2_CapabilityGet (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_CapabilityGetCurrent
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_CapabilityGetDefault
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_CapabilityQuerySupport
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_CapabilityReset
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_CapabilitySet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_CustomDSDataGet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_CustomDSDataSet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_AutomaticCaptureDirectory
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_ChangeDirectory
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_FileSystemCopy
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_CreateDirectory
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_FileSystemDelete
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_FormatMedia
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_FileSystemGetClose
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_FileSystemGetFirstFile
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_FileSystemGetInfo
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_FileSystemGetNextFile
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_FileSystemRename
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_ProcessEvent
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_PassThrough
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_PendingXfersEndXfer
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_PendingXfersGet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_PendingXfersReset
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_PendingXfersStopFeeder
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_SetupFileXferGet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_SetupFileXferGetDefault
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_SetupFileXferReset
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_SetupFileXferSet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_SetupFileXfer2Get
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_SetupFileXfer2GetDefault
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_SetupFileXfer2Reset
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_SetupFileXfer2Set
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_SetupMemXferGet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_GetDSStatus
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_DisableDSUserInterface
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_EnableDSUserInterface
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_EnableDSUIOnly
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_XferGroupGet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_XferGroupSet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);

/* Implementation of operation triplets
 * From Application to Source (Image Information) */
TW_UINT16 GPHOTO2_CIEColorGet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_ExtImageInfoGet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_GrayResponseReset
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_GrayResponseSet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_ImageFileXferGet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_ImageInfoGet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_ImageLayoutGet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_ImageLayoutGetDefault
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_ImageLayoutReset
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_ImageLayoutSet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_ImageMemXferGet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_ImageNativeXferGet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_JPEGCompressionGet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_JPEGCompressionGetDefault
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_JPEGCompressionReset
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_JPEGCompressionSet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_Palette8Get
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_Palette8GetDefault
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_Palette8Reset
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_Palette8Set
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_RGBResponseReset
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);
TW_UINT16 GPHOTO2_RGBResponseSet
    (pTW_IDENTITY pOrigin, TW_MEMREF pData);

/* UI function */
BOOL DoCameraUI(void);
HWND TransferringDialogBox(HWND dialog, DWORD progress);

#ifdef HAVE_GPHOTO2
/* Helper function for GUI */
TW_UINT16
_get_gphoto2_file_as_DIB(
        const char *folder, const char *filename, CameraFileType type,
        HWND hwnd, HBITMAP *hDIB
);
#endif
#endif
