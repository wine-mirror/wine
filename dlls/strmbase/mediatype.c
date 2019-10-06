/*
 * Implementation of MedaType utility functions
 *
 * Copyright 2003 Robert Shearman
 * Copyright 2010 Aric Stewart, CodeWeavers
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

#include "strmbase_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(strmbase);

static const struct
{
    const GUID *guid;
    const char *name;
}
strmbase_guids[] =
{
#define X(g) {&(g), #g}
    X(GUID_NULL),

#undef OUR_GUID_ENTRY
#define OUR_GUID_ENTRY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) X(name),
#include "uuids.h"

#undef X
};

static const char *strmbase_debugstr_guid(const GUID *guid)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(strmbase_guids); ++i)
    {
        if (IsEqualGUID(strmbase_guids[i].guid, guid))
            return wine_dbg_sprintf("%s", strmbase_guids[i].name);
    }

    return debugstr_guid(guid);
}

static const char *debugstr_fourcc(DWORD fourcc)
{
    char str[4] = {fourcc, fourcc >> 8, fourcc >> 16, fourcc >> 24};
    if (isprint(str[0]) && isprint(str[1]) && isprint(str[2]) && isprint(str[3]))
        return wine_dbgstr_an(str, 4);
    return wine_dbg_sprintf("%#x", fourcc);
}

void strmbase_dump_media_type(const AM_MEDIA_TYPE *mt)
{
    if (!TRACE_ON(strmbase) || !mt) return;

    TRACE("Dumping media type %p: major type %s, subtype %s",
            mt, strmbase_debugstr_guid(&mt->majortype), strmbase_debugstr_guid(&mt->subtype));
    if (mt->bFixedSizeSamples) TRACE(", fixed size samples");
    if (mt->bTemporalCompression) TRACE(", temporal compression");
    if (mt->lSampleSize) TRACE(", sample size %d", mt->lSampleSize);
    if (mt->pUnk) TRACE(", pUnk %p", mt->pUnk);
    TRACE(", format type %s.\n", strmbase_debugstr_guid(&mt->formattype));

    if (!mt->pbFormat) return;

    TRACE("Dumping format %p: ", mt->pbFormat);

    if (IsEqualGUID(&mt->formattype, &FORMAT_WaveFormatEx) && mt->cbFormat >= sizeof(WAVEFORMATEX))
    {
        WAVEFORMATEX *wfx = (WAVEFORMATEX *)mt->pbFormat;

        TRACE("tag %#x, %u channels, sample rate %u, %u bytes/sec, alignment %u, %u bits/sample.\n",
                wfx->wFormatTag, wfx->nChannels, wfx->nSamplesPerSec,
                wfx->nAvgBytesPerSec, wfx->nBlockAlign, wfx->wBitsPerSample);

        if (wfx->cbSize)
        {
            const unsigned char *extra = (const unsigned char *)(wfx + 1);
            unsigned int i;

            TRACE("  Extra bytes:");
            for (i = 0; i < wfx->cbSize; ++i)
            {
                if (!(i % 16)) TRACE("\n     ");
                TRACE(" %02x", extra[i]);
            }
            TRACE("\n");
        }
    }
    else if (IsEqualGUID(&mt->formattype, &FORMAT_VideoInfo) && mt->cbFormat >= sizeof(VIDEOINFOHEADER))
    {
        VIDEOINFOHEADER *vih = (VIDEOINFOHEADER *)mt->pbFormat;

        TRACE("source %s, target %s, bitrate %u, error rate %u, %s sec/frame, ",
                wine_dbgstr_rect(&vih->rcSource), wine_dbgstr_rect(&vih->rcTarget),
                vih->dwBitRate, vih->dwBitErrorRate, debugstr_time(vih->AvgTimePerFrame));
        TRACE("size %dx%d, %u planes, %u bpp, compression %s, image size %u",
                vih->bmiHeader.biWidth, vih->bmiHeader.biHeight, vih->bmiHeader.biPlanes,
                vih->bmiHeader.biBitCount, debugstr_fourcc(vih->bmiHeader.biCompression),
                vih->bmiHeader.biSizeImage);
        if (vih->bmiHeader.biXPelsPerMeter || vih->bmiHeader.biYPelsPerMeter)
            TRACE(", resolution %dx%d", vih->bmiHeader.biXPelsPerMeter, vih->bmiHeader.biYPelsPerMeter);
        if (vih->bmiHeader.biClrUsed) TRACE(", %d colours", vih->bmiHeader.biClrUsed);
        if (vih->bmiHeader.biClrImportant) TRACE(", %d important colours", vih->bmiHeader.biClrImportant);
        TRACE(".\n");
    }
    else
        TRACE("not implemented for this format type.\n");
}

HRESULT WINAPI CopyMediaType(AM_MEDIA_TYPE *dest, const AM_MEDIA_TYPE *src)
{
    *dest = *src;
    if (src->pbFormat)
    {
        dest->pbFormat = CoTaskMemAlloc(src->cbFormat);
        if (!dest->pbFormat)
            return E_OUTOFMEMORY;
        memcpy(dest->pbFormat, src->pbFormat, src->cbFormat);
    }
    if (dest->pUnk)
        IUnknown_AddRef(dest->pUnk);
    return S_OK;
}

void WINAPI FreeMediaType(AM_MEDIA_TYPE * pMediaType)
{
    CoTaskMemFree(pMediaType->pbFormat);
    pMediaType->pbFormat = NULL;
    if (pMediaType->pUnk)
    {
        IUnknown_Release(pMediaType->pUnk);
        pMediaType->pUnk = NULL;
    }
}

AM_MEDIA_TYPE * WINAPI CreateMediaType(AM_MEDIA_TYPE const * pSrc)
{
    AM_MEDIA_TYPE * pDest;

    pDest = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
    if (!pDest)
        return NULL;

    if (FAILED(CopyMediaType(pDest, pSrc)))
    {
        CoTaskMemFree(pDest);
        return NULL;
    }

    return pDest;
}

void WINAPI DeleteMediaType(AM_MEDIA_TYPE * pMediaType)
{
    FreeMediaType(pMediaType);
    CoTaskMemFree(pMediaType);
}

typedef struct IEnumMediaTypesImpl
{
    IEnumMediaTypes IEnumMediaTypes_iface;
    LONG refCount;
    struct strmbase_pin *basePin;
    ULONG count;
    ULONG uIndex;
} IEnumMediaTypesImpl;

static inline IEnumMediaTypesImpl *impl_from_IEnumMediaTypes(IEnumMediaTypes *iface)
{
    return CONTAINING_RECORD(iface, IEnumMediaTypesImpl, IEnumMediaTypes_iface);
}

static const struct IEnumMediaTypesVtbl IEnumMediaTypesImpl_Vtbl;

HRESULT enum_media_types_create(struct strmbase_pin *basePin, IEnumMediaTypes **ppEnum)
{
    IEnumMediaTypesImpl * pEnumMediaTypes = CoTaskMemAlloc(sizeof(IEnumMediaTypesImpl));

    *ppEnum = NULL;

    if (!pEnumMediaTypes)
        return E_OUTOFMEMORY;

    pEnumMediaTypes->IEnumMediaTypes_iface.lpVtbl = &IEnumMediaTypesImpl_Vtbl;
    pEnumMediaTypes->refCount = 1;
    pEnumMediaTypes->uIndex = 0;
    IPin_AddRef(&basePin->IPin_iface);
    pEnumMediaTypes->basePin = basePin;

    IEnumMediaTypes_Reset(&pEnumMediaTypes->IEnumMediaTypes_iface);

    *ppEnum = &pEnumMediaTypes->IEnumMediaTypes_iface;
    return S_OK;
}

static HRESULT WINAPI IEnumMediaTypesImpl_QueryInterface(IEnumMediaTypes * iface, REFIID riid, void ** ret_iface)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ret_iface);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IEnumMediaTypes))
    {
        IEnumMediaTypes_AddRef(iface);
        *ret_iface = iface;
        return S_OK;
    }

    *ret_iface = NULL;

    WARN("No interface for %s\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI IEnumMediaTypesImpl_AddRef(IEnumMediaTypes * iface)
{
    IEnumMediaTypesImpl *This = impl_from_IEnumMediaTypes(iface);
    ULONG ref = InterlockedIncrement(&This->refCount);

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

    return ref;
}

static ULONG WINAPI IEnumMediaTypesImpl_Release(IEnumMediaTypes * iface)
{
    IEnumMediaTypesImpl *This = impl_from_IEnumMediaTypes(iface);
    ULONG ref = InterlockedDecrement(&This->refCount);

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

    if (!ref)
    {
        IPin_Release(&This->basePin->IPin_iface);
        CoTaskMemFree(This);
    }
    return ref;
}

static HRESULT WINAPI IEnumMediaTypesImpl_Next(IEnumMediaTypes *iface,
        ULONG count, AM_MEDIA_TYPE **mts, ULONG *ret_count)
{
    IEnumMediaTypesImpl *enummt = impl_from_IEnumMediaTypes(iface);
    ULONG i;

    TRACE("iface %p, count %u, mts %p, ret_count %p.\n", iface, count, mts, ret_count);

    for (i = 0; i < count && enummt->uIndex + i < enummt->count; i++)
    {
        if (!(mts[i] = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE)))
                || FAILED(enummt->basePin->pFuncsTable->pin_get_media_type(enummt->basePin, enummt->uIndex + i, mts[i])))
        {
            while (i--)
                DeleteMediaType(mts[i]);
            *ret_count = 0;
            return E_OUTOFMEMORY;
        }

        if (TRACE_ON(strmbase))
        {
            TRACE("Returning media type %u:\n", enummt->uIndex + i);
            strmbase_dump_media_type(mts[i]);
        }
    }

    if ((count != 1) || ret_count)
        *ret_count = i;

    enummt->uIndex += i;

    return i == count ? S_OK : S_FALSE;
}

static HRESULT WINAPI IEnumMediaTypesImpl_Skip(IEnumMediaTypes *iface, ULONG count)
{
    IEnumMediaTypesImpl *enummt = impl_from_IEnumMediaTypes(iface);

    TRACE("iface %p, count %u.\n", iface, count);

    enummt->uIndex += count;

    return enummt->uIndex > enummt->count ? S_FALSE : S_OK;
}

static HRESULT WINAPI IEnumMediaTypesImpl_Reset(IEnumMediaTypes * iface)
{
    ULONG i;
    AM_MEDIA_TYPE amt;
    IEnumMediaTypesImpl *This = impl_from_IEnumMediaTypes(iface);

    TRACE("(%p)->()\n", iface);

    i = 0;
    while (This->basePin->pFuncsTable->pin_get_media_type(This->basePin, i, &amt) == S_OK)
    {
        FreeMediaType(&amt);
        i++;
    }
    This->count = i;
    This->uIndex = 0;

    return S_OK;
}

static HRESULT WINAPI IEnumMediaTypesImpl_Clone(IEnumMediaTypes * iface, IEnumMediaTypes ** ppEnum)
{
    HRESULT hr;
    IEnumMediaTypesImpl *This = impl_from_IEnumMediaTypes(iface);

    TRACE("(%p)->(%p)\n", iface, ppEnum);

    hr = enum_media_types_create(This->basePin, ppEnum);
    if (FAILED(hr))
        return hr;
    return IEnumMediaTypes_Skip(*ppEnum, This->uIndex);
}

static const IEnumMediaTypesVtbl IEnumMediaTypesImpl_Vtbl =
{
    IEnumMediaTypesImpl_QueryInterface,
    IEnumMediaTypesImpl_AddRef,
    IEnumMediaTypesImpl_Release,
    IEnumMediaTypesImpl_Next,
    IEnumMediaTypesImpl_Skip,
    IEnumMediaTypesImpl_Reset,
    IEnumMediaTypesImpl_Clone
};
