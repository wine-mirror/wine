/*
 * Intel Indeo 5 Video Decoder
 * Copyright 2023 Shaun Ren for CodeWeavers
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

#include <stdarg.h>
#include <stdlib.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "commdlg.h"
#include "vfw.h"
#include "initguid.h"
#include "ir50_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ir50_32);

static HINSTANCE IR50_32_hModule;

#define IV50_MAGIC mmioFOURCC('I','V','5','0')
#define compare_fourcc(fcc1, fcc2) (((fcc1)^(fcc2))&~0x20202020)

DEFINE_MEDIATYPE_GUID(MFVideoFormat_IV50, MAKEFOURCC('I','V','5','0'));

static inline UINT64
make_uint64( UINT32 high, UINT32 low )
{
    return ((UINT64)high << 32) | low;
}


static LRESULT
IV50_Open( const ICINFO *icinfo )
{
    IMFTransform *decoder = NULL;

    TRACE("DRV_OPEN %p\n", icinfo);

    if ( icinfo && compare_fourcc( icinfo->fccType, ICTYPE_VIDEO ) )
        return 0;

    if ( FAILED(winegstreamer_create_video_decoder( &decoder )) )
        return 0;

    return (LRESULT)decoder;
}

static LRESULT
IV50_DecompressQuery( LPBITMAPINFO in, LPBITMAPINFO out )
{
    TRACE("ICM_DECOMPRESS_QUERY %p %p\n", in, out);

    TRACE("in->planes  = %d\n", in->bmiHeader.biPlanes);
    TRACE("in->bpp     = %d\n", in->bmiHeader.biBitCount);
    TRACE("in->height  = %ld\n", in->bmiHeader.biHeight);
    TRACE("in->width   = %ld\n", in->bmiHeader.biWidth);
    TRACE("in->compr   = %#lx\n", in->bmiHeader.biCompression);

    if (compare_fourcc(in->bmiHeader.biCompression, IV50_MAGIC))
    {
        TRACE("can't do %#lx compression\n", in->bmiHeader.biCompression);
        return ICERR_BADFORMAT;
    }

    /* output must be same dimensions as input */
    if ( out )
    {
        TRACE("out->planes = %d\n", out->bmiHeader.biPlanes);
        TRACE("out->bpp    = %d\n", out->bmiHeader.biBitCount);
        TRACE("out->height = %ld\n", out->bmiHeader.biHeight);
        TRACE("out->width  = %ld\n", out->bmiHeader.biWidth);
        TRACE("out->compr  = %#lx\n", out->bmiHeader.biCompression);

        if (out->bmiHeader.biCompression == BI_RGB)
        {
            if (out->bmiHeader.biBitCount != 32 && out->bmiHeader.biBitCount != 24 && out->bmiHeader.biBitCount != 16)
                return ICERR_BADFORMAT;
        }
        else if (out->bmiHeader.biCompression == BI_BITFIELDS)
        {
            const DWORD *masks = (const DWORD *)(&out->bmiColors[0]);

            if (out->bmiHeader.biBitCount != 16
                    || masks[0] != 0xf800 || masks[1] != 0x07e0 || masks[2] != 0x001f)
                return ICERR_BADFORMAT;
        }
        else
        {
            return ICERR_BADFORMAT;
        }

        if (in->bmiHeader.biHeight != abs(out->bmiHeader.biHeight)
                || in->bmiHeader.biWidth != out->bmiHeader.biWidth)
        {
            TRACE("incompatible output dimensions requested\n");
            return ICERR_BADPARAM;
        }
    }

    return ICERR_OK;
}

static LRESULT
IV50_DecompressGetFormat( LPBITMAPINFO in, LPBITMAPINFO out )
{
    TRACE("ICM_DECOMPRESS_GETFORMAT %p %p\n", in, out);

    if (compare_fourcc(in->bmiHeader.biCompression, IV50_MAGIC))
        return ICERR_BADFORMAT;

    if ( out )
    {
        memset(&out->bmiHeader, 0, sizeof(out->bmiHeader));
        out->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        out->bmiHeader.biWidth = in->bmiHeader.biWidth;
        out->bmiHeader.biHeight = abs(in->bmiHeader.biHeight);
        out->bmiHeader.biCompression = BI_RGB;
        out->bmiHeader.biPlanes = 1;
        out->bmiHeader.biBitCount = 24;
        out->bmiHeader.biSizeImage = out->bmiHeader.biWidth * out->bmiHeader.biHeight * 3;
        return ICERR_OK;
    }

    return offsetof(BITMAPINFO, bmiColors[256]);
}

static LRESULT IV50_DecompressBegin( IMFTransform *decoder, LPBITMAPINFO in, LPBITMAPINFO out )
{
    IMFMediaType *input_type, *output_type;
    const GUID *output_subtype;
    LRESULT r = ICERR_INTERNAL;
    unsigned int stride;

    TRACE("ICM_DECOMPRESS_BEGIN %p %p %p\n", decoder, in, out);

    if ( !decoder )
        return ICERR_BADPARAM;

    if (out->bmiHeader.biCompression == BI_BITFIELDS)
        output_subtype = &MFVideoFormat_RGB565;
    else if (out->bmiHeader.biBitCount == 32)
        output_subtype = &MFVideoFormat_RGB32;
    else if ( out->bmiHeader.biBitCount == 24 )
        output_subtype = &MFVideoFormat_RGB24;
    else if ( out->bmiHeader.biBitCount == 16 )
        output_subtype = &MFVideoFormat_RGB555;
    else
        return ICERR_BADFORMAT;

    stride = (out->bmiHeader.biWidth + 3) & ~3;
    if (out->bmiHeader.biHeight >= 0)
        stride = -stride;

    if ( FAILED(MFCreateMediaType( &input_type )) )
        return ICERR_INTERNAL;

    if ( FAILED(MFCreateMediaType( &output_type )) )
    {
        IMFMediaType_Release( input_type );
        return ICERR_INTERNAL;
    }

    if ( FAILED(IMFMediaType_SetGUID( input_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video )) ||
         FAILED(IMFMediaType_SetGUID( input_type, &MF_MT_SUBTYPE, &MFVideoFormat_IV50 )) )
        goto done;
    if ( FAILED(IMFMediaType_SetUINT64(
                    input_type, &MF_MT_FRAME_SIZE,
                    make_uint64( in->bmiHeader.biWidth, in->bmiHeader.biHeight ) )) )
        goto done;

    if ( FAILED(IMFMediaType_SetGUID( output_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video )) ||
         FAILED(IMFMediaType_SetGUID( output_type, &MF_MT_SUBTYPE, output_subtype )) )
        goto done;
    if ( FAILED(IMFMediaType_SetUINT64(
                    output_type, &MF_MT_FRAME_SIZE,
                    make_uint64( out->bmiHeader.biWidth, abs(out->bmiHeader.biHeight) ) )) )
        goto done;
    if ( FAILED(IMFMediaType_SetUINT32( output_type, &MF_MT_DEFAULT_STRIDE, stride)) )
        goto done;

    if ( FAILED(IMFTransform_SetInputType( decoder, 0, input_type, 0 )) ||
         FAILED(IMFTransform_SetOutputType( decoder, 0, output_type, 0 )) )
        goto done;

    r = ICERR_OK;

done:
    IMFMediaType_Release( input_type );
    IMFMediaType_Release( output_type );
    return r;
}

static LRESULT IV50_Decompress( IMFTransform *decoder, ICDECOMPRESS *icd, DWORD size )
{
    IMFSample *in_sample = NULL, *out_sample = NULL;
    IMFMediaBuffer *in_buf = NULL, *out_buf = NULL;
    MFT_OUTPUT_DATA_BUFFER mft_buf;
    DWORD mft_status;
    BYTE *data;
    HRESULT hr;
    LRESULT r = ICERR_INTERNAL;

    TRACE("ICM_DECOMPRESS %p %p %lu\n", decoder, icd, size);

    if ( FAILED(MFCreateSample( &in_sample )) )
        return ICERR_INTERNAL;

    if ( FAILED(MFCreateMemoryBuffer( icd->lpbiInput->biSizeImage, &in_buf )) )
        goto done;

    if ( FAILED(IMFSample_AddBuffer( in_sample, in_buf )) )
        goto done;

    if ( FAILED(MFCreateSample( &out_sample )) )
        goto done;

    if ( FAILED(MFCreateMemoryBuffer( icd->lpbiOutput->biSizeImage, &out_buf )) )
        goto done;

    if ( FAILED(IMFSample_AddBuffer( out_sample, out_buf )) )
        goto done;

    if ( FAILED(IMFMediaBuffer_Lock( in_buf, &data, NULL, NULL )))
        goto done;

    memcpy( data, icd->lpInput, icd->lpbiInput->biSizeImage );

    if ( FAILED(IMFMediaBuffer_Unlock( in_buf )) )
        goto done;

    if ( FAILED(IMFMediaBuffer_SetCurrentLength( in_buf, icd->lpbiInput->biSizeImage )) )
        goto done;

    if ( FAILED(IMFTransform_ProcessInput( decoder, 0, in_sample, 0 )) )
        goto done;

    memset( &mft_buf, 0, sizeof(mft_buf) );
    mft_buf.pSample = out_sample;

    hr = IMFTransform_ProcessOutput( decoder, 0, 1, &mft_buf, &mft_status );
    if ( hr == MF_E_TRANSFORM_STREAM_CHANGE )
        hr = IMFTransform_ProcessOutput( decoder, 0, 1, &mft_buf, &mft_status );

    if ( SUCCEEDED(hr) )
    {
        LONG width = icd->lpbiOutput->biWidth * (icd->lpbiOutput->biBitCount / 8);
        LONG height = abs( icd->lpbiOutput->biHeight );
        LONG stride = (width + 3) & ~3;
        BYTE *output = (BYTE *)icd->lpOutput;

        if ( FAILED(IMFMediaBuffer_Lock( out_buf, &data, NULL, NULL )))
            goto done;

        MFCopyImage( output, stride, data, stride, width, height );

        IMFMediaBuffer_Unlock( out_buf );
        r = ICERR_OK;
    }
    else if ( hr == MF_E_TRANSFORM_NEED_MORE_INPUT )
    {
        TRACE("no output received.\n");
        r = ICERR_OK;
    }

done:
    if ( in_buf )
        IMFMediaBuffer_Release( in_buf );
    if ( in_sample )
        IMFSample_Release( in_sample );
    if ( out_buf )
        IMFMediaBuffer_Release( out_buf );
    if ( out_sample )
        IMFSample_Release( out_sample );

    return r;
}

static LRESULT IV50_GetInfo( ICINFO *icinfo, DWORD dwSize )
{
    TRACE("ICM_GETINFO %p %lu\n", icinfo, dwSize);

    if ( !icinfo ) return sizeof(ICINFO);
    if ( dwSize < sizeof(ICINFO) ) return 0;

    icinfo->dwSize       = sizeof(ICINFO);
    icinfo->fccType      = ICTYPE_VIDEO;
    icinfo->fccHandler   = IV50_MAGIC;
    icinfo->dwFlags      = 0;
    icinfo->dwVersion    = ICVERSION;
    icinfo->dwVersionICM = ICVERSION;

    LoadStringW( IR50_32_hModule, IDS_NAME, icinfo->szName, ARRAY_SIZE(icinfo->szName) );
    LoadStringW( IR50_32_hModule, IDS_DESCRIPTION, icinfo->szDescription, ARRAY_SIZE(icinfo->szDescription) );
    /* msvfw32 will fill icinfo->szDriver for us */

    return sizeof(ICINFO);
}

/***********************************************************************
 *              DriverProc (IR50_32.@)
 */
LRESULT WINAPI IV50_DriverProc( DWORD_PTR dwDriverId, HDRVR hdrvr, UINT msg,
                                LPARAM lParam1, LPARAM lParam2 )
{
    IMFTransform *decoder = (IMFTransform *) dwDriverId;
    LRESULT r = ICERR_UNSUPPORTED;

    TRACE("%Id %p %04x %08Ix %08Ix\n", dwDriverId, hdrvr, msg, lParam1, lParam2);

    switch( msg )
    {
    case DRV_LOAD:
        TRACE("DRV_LOAD\n");
        r = 1;
        break;

    case DRV_OPEN:
        r = IV50_Open((ICINFO *)lParam2);
        break;

    case DRV_CLOSE:
        TRACE("DRV_CLOSE\n");
        if ( decoder )
            IMFTransform_Release( decoder );
        r = 1;
        break;

    case DRV_ENABLE:
    case DRV_DISABLE:
    case DRV_FREE:
        break;

    case ICM_GETINFO:
        r = IV50_GetInfo( (ICINFO *) lParam1, (DWORD) lParam2 );
        break;

    case ICM_DECOMPRESS_QUERY:
        r = IV50_DecompressQuery( (LPBITMAPINFO) lParam1, (LPBITMAPINFO) lParam2 );
        break;

    case ICM_DECOMPRESS_GET_FORMAT:
        r = IV50_DecompressGetFormat( (LPBITMAPINFO) lParam1, (LPBITMAPINFO) lParam2 );
        break;

    case ICM_DECOMPRESS_GET_PALETTE:
        FIXME("ICM_DECOMPRESS_GET_PALETTE\n");
        break;

    case ICM_DECOMPRESS:
        r = IV50_Decompress( decoder, (ICDECOMPRESS *) lParam1, (DWORD) lParam2 );
        break;

    case ICM_DECOMPRESS_BEGIN:
        r = IV50_DecompressBegin( decoder, (LPBITMAPINFO) lParam1, (LPBITMAPINFO) lParam2 );
        break;

    case ICM_DECOMPRESS_END:
        r = ICERR_OK;
        break;

    case ICM_DECOMPRESSEX_QUERY:
        FIXME("ICM_DECOMPRESSEX_QUERY\n");
        break;

    case ICM_DECOMPRESSEX:
        FIXME("ICM_DECOMPRESSEX\n");
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
        FIXME("Unknown message: %04x %Id %Id\n", msg, lParam1, lParam2);
    }

    return r;
}

/***********************************************************************
 *              DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpReserved)
{
    TRACE("(%p,%lu,%p)\n", hModule, dwReason, lpReserved);

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        IR50_32_hModule = hModule;
        break;
    }
    return TRUE;
}
