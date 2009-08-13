/*
 * Copyright 2009 Vincent Povirk
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

#include "config.h"

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "wincodec.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

struct FormatConverter;

enum pixelformat {
    format_32bppBGR,
    format_32bppBGRA
};

typedef HRESULT (*copyfunc)(struct FormatConverter *This, const WICRect *prc,
    UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer, enum pixelformat source_format);

struct pixelformatinfo {
    enum pixelformat format;
    const WICPixelFormatGUID *guid;
    copyfunc copy_function;
};

typedef struct FormatConverter {
    const IWICFormatConverterVtbl *lpVtbl;
    LONG ref;
    IWICBitmapSource *source;
    const struct pixelformatinfo *dst_format, *src_format;
    WICBitmapDitherType dither;
    double alpha_threshold;
    WICBitmapPaletteType palette_type;
} FormatConverter;

static HRESULT copypixels_to_32bppBGRA(struct FormatConverter *This, const WICRect *prc,
    UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer, enum pixelformat source_format)
{
    switch (source_format)
    {
    case format_32bppBGR:
        if (prc)
        {
            HRESULT res;
            UINT x, y;

            res = IWICBitmapSource_CopyPixels(This->source, prc, cbStride, cbBufferSize, pbBuffer);
            if (FAILED(res)) return res;

            /* set all alpha values to 255 */
            for (y=0; y<prc->Height; y++)
                for (x=0; x<prc->Width; x++)
                    pbBuffer[cbStride*y+4*x+3] = 0xff;
        }
        return S_OK;
    case format_32bppBGRA:
        if (prc)
            return IWICBitmapSource_CopyPixels(This->source, prc, cbStride, cbBufferSize, pbBuffer);
        return S_OK;
    default:
        return WINCODEC_ERR_UNSUPPORTEDOPERATION;
    }
}

static HRESULT copypixels_to_32bppBGR(struct FormatConverter *This, const WICRect *prc,
    UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer, enum pixelformat source_format)
{
    switch (source_format)
    {
    case format_32bppBGR:
    case format_32bppBGRA:
        if (prc)
            return IWICBitmapSource_CopyPixels(This->source, prc, cbStride, cbBufferSize, pbBuffer);
        return S_OK;
    default:
        return copypixels_to_32bppBGRA(This, prc, cbStride, cbBufferSize, pbBuffer, source_format);
    }
}

static const struct pixelformatinfo supported_formats[] = {
    {format_32bppBGR, &GUID_WICPixelFormat32bppBGR, copypixels_to_32bppBGR},
    {format_32bppBGRA, &GUID_WICPixelFormat32bppBGRA, copypixels_to_32bppBGRA},
    {0}
};

static const struct pixelformatinfo *get_formatinfo(const WICPixelFormatGUID *format)
{
    UINT i;

    for (i=0; supported_formats[i].guid; i++)
        if (IsEqualGUID(supported_formats[i].guid, format)) return &supported_formats[i];

    return NULL;
}

static HRESULT WINAPI FormatConverter_QueryInterface(IWICFormatConverter *iface, REFIID iid,
    void **ppv)
{
    FormatConverter *This = (FormatConverter*)iface;
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapSource, iid) ||
        IsEqualIID(&IID_IWICFormatConverter, iid))
    {
        *ppv = This;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI FormatConverter_AddRef(IWICFormatConverter *iface)
{
    FormatConverter *This = (FormatConverter*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI FormatConverter_Release(IWICFormatConverter *iface)
{
    FormatConverter *This = (FormatConverter*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        if (This->source) IWICBitmapSource_Release(This->source);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI FormatConverter_GetSize(IWICFormatConverter *iface,
    UINT *puiWidth, UINT *puiHeight)
{
    FormatConverter *This = (FormatConverter*)iface;

    TRACE("(%p,%p,%p)\n", iface, puiWidth, puiHeight);

    if (This->source)
        return IWICBitmapSource_GetSize(This->source, puiWidth, puiHeight);
    else
        return WINCODEC_ERR_NOTINITIALIZED;
}

static HRESULT WINAPI FormatConverter_GetPixelFormat(IWICFormatConverter *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    FormatConverter *This = (FormatConverter*)iface;

    TRACE("(%p,%p): stub\n", iface, pPixelFormat);

    if (This->source)
        memcpy(pPixelFormat, This->dst_format->guid, sizeof(GUID));
    else
        return WINCODEC_ERR_NOTINITIALIZED;

    return S_OK;
}

static HRESULT WINAPI FormatConverter_GetResolution(IWICFormatConverter *iface,
    double *pDpiX, double *pDpiY)
{
    FormatConverter *This = (FormatConverter*)iface;

    TRACE("(%p,%p,%p): stub\n", iface, pDpiX, pDpiY);

    if (This->source)
        return IWICBitmapSource_GetResolution(This->source, pDpiX, pDpiY);
    else
        return WINCODEC_ERR_NOTINITIALIZED;
}

static HRESULT WINAPI FormatConverter_CopyPalette(IWICFormatConverter *iface,
    IWICPalette *pIPalette)
{
    FIXME("(%p,%p): stub\n", iface, pIPalette);
    return E_NOTIMPL;
}

static HRESULT WINAPI FormatConverter_CopyPixels(IWICFormatConverter *iface,
    const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer)
{
    FormatConverter *This = (FormatConverter*)iface;
    TRACE("(%p,%p,%u,%u,%p)\n", iface, prc, cbStride, cbBufferSize, pbBuffer);

    if (This->source)
        return This->dst_format->copy_function(This, prc, cbStride, cbBufferSize,
            pbBuffer, This->src_format->format);
    else
        return WINCODEC_ERR_NOTINITIALIZED;
}

static HRESULT WINAPI FormatConverter_Initialize(IWICFormatConverter *iface,
    IWICBitmapSource *pISource, REFWICPixelFormatGUID dstFormat, WICBitmapDitherType dither,
    IWICPalette *pIPalette, double alphaThresholdPercent, WICBitmapPaletteType paletteTranslate)
{
    FormatConverter *This = (FormatConverter*)iface;
    const struct pixelformatinfo *srcinfo, *dstinfo;
    static INT fixme=0;
    GUID srcFormat;
    HRESULT res;

    TRACE("(%p,%p,%s,%u,%p,%0.1f,%u)\n", iface, pISource, debugstr_guid(dstFormat),
        dither, pIPalette, alphaThresholdPercent, paletteTranslate);

    if (pIPalette && !fixme++) FIXME("ignoring palette");

    if (This->source) return WINCODEC_ERR_WRONGSTATE;

    res = IWICBitmapSource_GetPixelFormat(pISource, &srcFormat);
    if (FAILED(res)) return res;

    srcinfo = get_formatinfo(&srcFormat);
    if (!srcinfo) return WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT;

    dstinfo = get_formatinfo(dstFormat);
    if (!dstinfo) return WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT;

    if (dstinfo->copy_function)
    {
        IWICBitmapSource_AddRef(pISource);
        This->source = pISource;
        This->src_format = srcinfo;
        This->dst_format = dstinfo;
        This->dither = dither;
        This->alpha_threshold = alphaThresholdPercent;
        This->palette_type = paletteTranslate;
    }
    else
        return WINCODEC_ERR_UNSUPPORTEDOPERATION;

    return S_OK;
}

static HRESULT WINAPI FormatConverter_CanConvert(IWICFormatConverter *iface,
    REFWICPixelFormatGUID srcPixelFormat, REFWICPixelFormatGUID dstPixelFormat,
    BOOL *pfCanConvert)
{
    FormatConverter *This = (FormatConverter*)iface;
    const struct pixelformatinfo *srcinfo, *dstinfo;

    TRACE("(%p,%s,%s,%p)\n", iface, debugstr_guid(srcPixelFormat),
        debugstr_guid(dstPixelFormat), pfCanConvert);

    srcinfo = get_formatinfo(srcPixelFormat);
    if (!srcinfo) return WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT;

    dstinfo = get_formatinfo(dstPixelFormat);
    if (!dstinfo) return WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT;

    if (dstinfo->copy_function &&
        SUCCEEDED(dstinfo->copy_function(This, NULL, 0, 0, NULL, dstinfo->format)))
        *pfCanConvert = TRUE;
    else
        *pfCanConvert = FALSE;

    return S_OK;
}

static const IWICFormatConverterVtbl FormatConverter_Vtbl = {
    FormatConverter_QueryInterface,
    FormatConverter_AddRef,
    FormatConverter_Release,
    FormatConverter_GetSize,
    FormatConverter_GetPixelFormat,
    FormatConverter_GetResolution,
    FormatConverter_CopyPalette,
    FormatConverter_CopyPixels,
    FormatConverter_Initialize,
    FormatConverter_CanConvert
};

HRESULT FormatConverter_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv)
{
    FormatConverter *This;
    HRESULT ret;

    TRACE("(%p,%s,%p)\n", pUnkOuter, debugstr_guid(iid), ppv);

    *ppv = NULL;

    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(FormatConverter));
    if (!This) return E_OUTOFMEMORY;

    This->lpVtbl = &FormatConverter_Vtbl;
    This->ref = 1;
    This->source = NULL;

    ret = IUnknown_QueryInterface((IUnknown*)This, iid, ppv);
    IUnknown_Release((IUnknown*)This);

    return ret;
}
