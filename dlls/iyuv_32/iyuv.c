/*
 * iyuv Video "Decoder"
 * Copyright 2026 Brendan McGrath for CodeWeavers
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
 *
 */

#include "windef.h"
#include <stdarg.h>
#include <stdlib.h>

#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "commdlg.h"
#include "initguid.h"
#include "vfw.h"
#include "wmcodecdsp.h"

#include "iyuv_private.h"
#include "wine/debug.h"

#define COBJMACROS
#include "mfapi.h"
#include "mferror.h"
#include "mfidl.h"
#include "mfobjects.h"
#include "mftransform.h"

WINE_DEFAULT_DEBUG_CHANNEL(iyuv_32);

static HINSTANCE IYUV_32_module;

#define FOURCC_I420 mmioFOURCC('I', '4', '2', '0')
#define FOURCC_IYUV mmioFOURCC('I', 'Y', 'U', 'V')

#define compare_fourcc(fcc1, fcc2) (((fcc1) ^ (fcc2)) & ~0x20202020)

static LRESULT IYUV_Open(const ICINFO *icinfo)
{
    FIXME("DRV_OPEN %p\n", icinfo);

    return 0;
}

static LRESULT IYUV_DecompressQuery(const BITMAPINFOHEADER *in, const BITMAPINFOHEADER *out)
{
    TRACE("in->planes  = %d\n", in->biPlanes);
    TRACE("in->bpp     = %d\n", in->biBitCount);
    TRACE("in->height  = %ld\n", in->biHeight);
    TRACE("in->width   = %ld\n", in->biWidth);
    TRACE("in->compr   = %#lx\n", in->biCompression);

    if (compare_fourcc(in->biCompression, FOURCC_I420) && compare_fourcc(in->biCompression, FOURCC_IYUV))
    {
        TRACE("can't do %#lx decompression\n", in->biCompression);
        return ICERR_BADFORMAT;
    }

    if (!in->biHeight || !in->biWidth)
        return ICERR_BADFORMAT;

    /* output must be same dimensions as input */
    if (out)
    {
        TRACE("out->planes = %d\n", out->biPlanes);
        TRACE("out->bpp    = %d\n", out->biBitCount);
        TRACE("out->height = %ld\n", out->biHeight);
        TRACE("out->width  = %ld\n", out->biWidth);
        TRACE("out->compr  = %#lx\n", out->biCompression);

        if (out->biCompression != BI_RGB)
            return ICERR_BADFORMAT;

        if (out->biBitCount != 24 && out->biBitCount != 16 && out->biBitCount != 8)
            return ICERR_BADFORMAT;

        if (in->biWidth != out->biWidth || in->biHeight != out->biHeight)
            return ICERR_BADFORMAT;
    }

    return ICERR_OK;
}

static LRESULT IYUV_DecompressGetFormat(BITMAPINFOHEADER *in, BITMAPINFOHEADER *out)
{
    FIXME("ICM_DECOMPRESS_GETFORMAT %p %p\n", in, out);

    return ICERR_UNSUPPORTED;
}

static LRESULT IYUV_DecompressBegin(IMFTransform *transform, const BITMAPINFOHEADER *in, const BITMAPINFOHEADER *out)
{
    FIXME("ICM_DECOMPRESS_BEGIN %p %p %p\n", transform, in, out);

    return ICERR_UNSUPPORTED;
}

static LRESULT IYUV_Decompress(IMFTransform *transform, const ICDECOMPRESS *params)
{
    FIXME("ICM_DECOMPRESS %p %p\n", transform, params);

    return ICERR_UNSUPPORTED;
}

static LRESULT IYUV_GetInfo(ICINFO *icinfo, DWORD size)
{
    TRACE("ICM_GETINFO %p %lu\n", icinfo, size);

    if (!icinfo)
        return sizeof(ICINFO);
    if (size < sizeof(ICINFO))
        return 0;

    icinfo->dwSize = sizeof(ICINFO);
    icinfo->fccType = ICTYPE_VIDEO;
    icinfo->fccHandler = FOURCC_IYUV;
    icinfo->dwFlags = 0;
    icinfo->dwVersion = 0;
    icinfo->dwVersionICM = ICVERSION;

    LoadStringW(IYUV_32_module, IDS_NAME, icinfo->szName, ARRAY_SIZE(icinfo->szName));
    LoadStringW(IYUV_32_module, IDS_DESCRIPTION, icinfo->szDescription, ARRAY_SIZE(icinfo->szDescription));
    /* msvfw32 will fill icinfo->szDriver for us */

    return sizeof(ICINFO);
}

/***********************************************************************
 *              DriverProc (IYUV_32.@)
 */
LRESULT WINAPI IYUV_DriverProc(DWORD_PTR driver_id, HDRVR hdrvr, UINT msg, LPARAM param1, LPARAM param2)
{
    IMFTransform *transform = (IMFTransform *)driver_id;
    LRESULT r = ICERR_UNSUPPORTED;

    TRACE("%Id %p %04x %08Ix %08Ix\n", driver_id, hdrvr, msg, param1, param2);

    switch (msg)
    {
    case DRV_LOAD:
        TRACE("DRV_LOAD\n");
        r = TRUE;
        break;

    case DRV_OPEN:
        r = IYUV_Open((ICINFO *)param2);
        break;

    case DRV_CLOSE:
        TRACE("DRV_CLOSE\n");
        if (transform)
            IMFTransform_Release(transform);
        r = TRUE;
        break;

    case DRV_ENABLE:
    case DRV_DISABLE:
    case DRV_FREE:
        break;

    case ICM_GETINFO:
        r = IYUV_GetInfo((ICINFO *)param1, (DWORD)param2);
        break;

    case ICM_DECOMPRESS_QUERY:
        r = IYUV_DecompressQuery((BITMAPINFOHEADER *)param1, (BITMAPINFOHEADER *)param2);
        break;

    case ICM_DECOMPRESS_GET_FORMAT:
        r = IYUV_DecompressGetFormat((BITMAPINFOHEADER *)param1, (BITMAPINFOHEADER *)param2);
        break;

    case ICM_DECOMPRESS_GET_PALETTE:
        FIXME("ICM_DECOMPRESS_GET_PALETTE\n");
        break;

    case ICM_DECOMPRESS:
        r = IYUV_Decompress(transform, (ICDECOMPRESS *)param1);
        break;

    case ICM_DECOMPRESS_BEGIN:
        r = IYUV_DecompressBegin(transform, (BITMAPINFOHEADER *)param1, (BITMAPINFOHEADER *)param2);
        break;

    case ICM_DECOMPRESS_END:
        r = ICERR_OK;
        break;

    case ICM_DECOMPRESSEX_QUERY:
    case ICM_DECOMPRESSEX_BEGIN:
    case ICM_DECOMPRESSEX:
    case ICM_DECOMPRESSEX_END:
        /* unsupported */
        break;

    case ICM_COMPRESS_QUERY:
        r = ICERR_BADFORMAT;
        /* fall through */
    case ICM_COMPRESS_GET_FORMAT:
    case ICM_COMPRESS_END:
    case ICM_COMPRESS:
        FIXME("compression not implemented\n");
        break;

    case ICM_CONFIGURE:
        break;

    default:
        FIXME("Unknown message: %04x %Id %Id\n", msg, param1, param2);
    }

    return r;
}

/***********************************************************************
 *              DllMain
 */
BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID reserved)
{
    TRACE("(%p,%lu,%p)\n", module, reason, reserved);

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(module);
        IYUV_32_module = module;
        break;
    }
    return TRUE;
}
