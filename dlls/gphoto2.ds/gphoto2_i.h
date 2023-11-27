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

#include <stdio.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <basetsd.h>
#include <jpeglib.h>

#include "windef.h"
#include "winbase.h"
#include "twain.h"

#include "wine/list.h"

extern HINSTANCE GPHOTO2_instance;

/* internal information about an active data source */
struct tagActiveDS
{
    TW_IDENTITY		identity;		/* identity of the data source */
    TW_IDENTITY         appIdentity;            /* identity of the app */
    TW_UINT16		currentState;		/* current state */
    TW_UINT16		twCC;			/* condition code */
    HWND		progressWnd;		/* window handle of the scanning window */

    /* Capabilities */
    TW_UINT32		capXferMech;		/* ICAP_XFERMECH */
    TW_UINT16		pixeltype;		/* ICAP_PIXELTYPE */
    TW_UINT16		pixelflavor;		/* ICAP_PIXELFLAVOR */

    unsigned int        file_count;
    unsigned int        download_count;
    BOOL               *download_flags;

    /* Download and decode JPEG STATE */
    UINT64              file_handle;
    unsigned char      *file_data;
    unsigned int        file_size;
    struct jpeg_source_mgr		xjsm;
    struct jpeg_decompress_struct	jd;
    struct jpeg_error_mgr		jerr;
};

extern struct tagActiveDS activeDS;

/* Helper functions */
extern TW_UINT16 GPHOTO2_SaneCapability (pTW_CAPABILITY pCapability, TW_UINT16 action);

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
HWND TransferringDialogBox(HWND dialog, LONG progress);

/* Helper function for GUI */
TW_UINT16
_get_gphoto2_file_as_DIB( unsigned int idx, BOOL preview, HWND hwnd, HBITMAP *hDIB );

#endif
