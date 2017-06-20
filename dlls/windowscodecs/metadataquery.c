/*
 * Copyright 2016 Andrew Eikum for CodeWeavers
 * Copyright 2017 Dmitry Timoshkov
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

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

typedef struct {
    IWICMetadataQueryReader IWICMetadataQueryReader_iface;

    LONG ref;

    IWICMetadataBlockReader *block;
} QueryReader;

static inline QueryReader *impl_from_IWICMetadataQueryReader(IWICMetadataQueryReader *iface)
{
    return CONTAINING_RECORD(iface, QueryReader, IWICMetadataQueryReader_iface);
}

static HRESULT WINAPI mqr_QueryInterface(IWICMetadataQueryReader *iface, REFIID riid,
        void **ppvObject)
{
    QueryReader *This = impl_from_IWICMetadataQueryReader(iface);

    TRACE("(%p,%s,%p)\n", This, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
            IsEqualGUID(riid, &IID_IWICMetadataQueryReader))
        *ppvObject = &This->IWICMetadataQueryReader_iface;
    else
        *ppvObject = NULL;

    if (*ppvObject)
    {
        IUnknown_AddRef((IUnknown*)*ppvObject);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI mqr_AddRef(IWICMetadataQueryReader *iface)
{
    QueryReader *This = impl_from_IWICMetadataQueryReader(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) refcount=%u\n", This, ref);
    return ref;
}

static ULONG WINAPI mqr_Release(IWICMetadataQueryReader *iface)
{
    QueryReader *This = impl_from_IWICMetadataQueryReader(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p) refcount=%u\n", This, ref);
    if (!ref)
    {
        IWICMetadataBlockReader_Release(This->block);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

static HRESULT WINAPI mqr_GetContainerFormat(IWICMetadataQueryReader *iface,
        GUID *pguidContainerFormat)
{
    QueryReader *This = impl_from_IWICMetadataQueryReader(iface);
    FIXME("(%p,%p)\n", This, pguidContainerFormat);
    return E_NOTIMPL;
}

static HRESULT WINAPI mqr_GetLocation(IWICMetadataQueryReader *iface,
        UINT cchMaxLength, WCHAR *wzNamespace, UINT *pcchActualLength)
{
    QueryReader *This = impl_from_IWICMetadataQueryReader(iface);
    FIXME("(%p,%u,%p,%p)\n", This, cchMaxLength, wzNamespace, pcchActualLength);
    return E_NOTIMPL;
}

static HRESULT WINAPI mqr_GetMetadataByName(IWICMetadataQueryReader *iface,
        LPCWSTR wzName, PROPVARIANT *pvarValue)
{
    QueryReader *This = impl_from_IWICMetadataQueryReader(iface);
    FIXME("(%p,%s,%p)\n", This, wine_dbgstr_w(wzName), pvarValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI mqr_GetEnumerator(IWICMetadataQueryReader *iface,
        IEnumString **ppIEnumString)
{
    QueryReader *This = impl_from_IWICMetadataQueryReader(iface);
    FIXME("(%p,%p)\n", This, ppIEnumString);
    return E_NOTIMPL;
}

static IWICMetadataQueryReaderVtbl mqr_vtbl = {
    mqr_QueryInterface,
    mqr_AddRef,
    mqr_Release,
    mqr_GetContainerFormat,
    mqr_GetLocation,
    mqr_GetMetadataByName,
    mqr_GetEnumerator
};

HRESULT MetadataQueryReader_CreateInstance(IWICMetadataBlockReader *mbr, IWICMetadataQueryReader **out)
{
    QueryReader *obj;

    obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*obj));
    if (!obj)
        return E_OUTOFMEMORY;

    obj->IWICMetadataQueryReader_iface.lpVtbl = &mqr_vtbl;
    obj->ref = 1;

    IWICMetadataBlockReader_AddRef(mbr);
    obj->block = mbr;

    *out = &obj->IWICMetadataQueryReader_iface;

    return S_OK;
}

static const WCHAR bmpW[] = { 'b','m','p',0 };
static const WCHAR pngW[] = { 'p','n','g',0 };
static const WCHAR icoW[] = { 'i','c','o',0 };
static const WCHAR jpgW[] = { 'j','p','g',0 };
static const WCHAR tiffW[] = { 't','i','f','f',0 };
static const WCHAR gifW[] = { 'g','i','f',0 };
static const WCHAR wmphotoW[] = { 'w','m','p','h','o','t','o',0 };
static const WCHAR unknownW[] = { 'u','n','k','n','o','w','n',0 };
static const WCHAR ifdW[] = { 'i','f','d',0 };
static const WCHAR subW[] = { 's','u','b',0 };
static const WCHAR exifW[] = { 'e','x','i','f',0 };
static const WCHAR gpsW[] = { 'g','p','s',0 };
static const WCHAR interopW[] = { 'i','n','t','e','r','o','p',0 };
static const WCHAR app0W[] = { 'a','p','p','0',0 };
static const WCHAR app1W[] = { 'a','p','p','1',0 };
static const WCHAR app13W[] = { 'a','p','p','1','3',0 };
static const WCHAR iptcW[] = { 'i','p','t','c',0 };
static const WCHAR irbW[] = { 'i','r','b',0 };
static const WCHAR _8bimiptcW[] = { '8','b','i','m','i','p','t','c',0 };
static const WCHAR _8bimResInfoW[] = { '8','b','i','m','R','e','s','I','n','f','o',0 };
static const WCHAR _8bimiptcdigestW[] = { '8','b','i','m','i','p','t','c','d','i','g','e','s','t',0 };
static const WCHAR xmpW[] = { 'x','m','p',0 };
static const WCHAR thumbW[] = { 't','h','u','m','b',0 };
static const WCHAR tEXtW[] = { 't','E','X','t',0 };
static const WCHAR xmpstructW[] = { 'x','m','p','s','t','r','u','c','t',0 };
static const WCHAR xmpbagW[] = { 'x','m','p','b','a','g',0 };
static const WCHAR xmpseqW[] = { 'x','m','p','s','e','q',0 };
static const WCHAR xmpaltW[] = { 'x','m','p','a','l','t',0 };
static const WCHAR logscrdescW[] = { 'l','o','g','s','c','r','d','e','s','c',0 };
static const WCHAR imgdescW[] = { 'i','m','g','d','e','s','c',0 };
static const WCHAR grctlextW[] = { 'g','r','c','t','l','e','x','t',0 };
static const WCHAR appextW[] = { 'a','p','p','e','x','t',0 };
static const WCHAR chrominanceW[] = { 'c','h','r','o','m','i','n','a','n','c','e',0 };
static const WCHAR luminanceW[] = { 'l','u','m','i','n','a','n','c','e',0 };
static const WCHAR comW[] = { 'c','o','m',0 };
static const WCHAR commentextW[] = { 'c','o','m','m','e','n','t','e','x','t',0 };
static const WCHAR gAMAW[] = { 'g','A','M','A',0 };
static const WCHAR bKGDW[] = { 'b','K','G','D',0 };
static const WCHAR iTXtW[] = { 'i','T','X','t',0 };
static const WCHAR cHRMW[] = { 'c','H','R','M',0 };
static const WCHAR hISTW[] = { 'h','I','S','T',0 };
static const WCHAR iCCPW[] = { 'i','C','C','P',0 };
static const WCHAR sRGBW[] = { 's','R','G','B',0 };
static const WCHAR tIMEW[] = { 't','I','M','E',0 };

static const struct
{
    const GUID *guid;
    const WCHAR *name;
} guid2name[] =
{
    { &GUID_ContainerFormatBmp, bmpW },
    { &GUID_ContainerFormatPng, pngW },
    { &GUID_ContainerFormatIco, icoW },
    { &GUID_ContainerFormatJpeg, jpgW },
    { &GUID_ContainerFormatTiff, tiffW },
    { &GUID_ContainerFormatGif, gifW },
    { &GUID_ContainerFormatWmp, wmphotoW },
    { &GUID_MetadataFormatUnknown, unknownW },
    { &GUID_MetadataFormatIfd, ifdW },
    { &GUID_MetadataFormatSubIfd, subW },
    { &GUID_MetadataFormatExif, exifW },
    { &GUID_MetadataFormatGps, gpsW },
    { &GUID_MetadataFormatInterop, interopW },
    { &GUID_MetadataFormatApp0, app0W },
    { &GUID_MetadataFormatApp1, app1W },
    { &GUID_MetadataFormatApp13, app13W },
    { &GUID_MetadataFormatIPTC, iptcW },
    { &GUID_MetadataFormatIRB, irbW },
    { &GUID_MetadataFormat8BIMIPTC, _8bimiptcW },
    { &GUID_MetadataFormat8BIMResolutionInfo, _8bimResInfoW },
    { &GUID_MetadataFormat8BIMIPTCDigest, _8bimiptcdigestW },
    { &GUID_MetadataFormatXMP, xmpW },
    { &GUID_MetadataFormatThumbnail, thumbW },
    { &GUID_MetadataFormatChunktEXt, tEXtW },
    { &GUID_MetadataFormatXMPStruct, xmpstructW },
    { &GUID_MetadataFormatXMPBag, xmpbagW },
    { &GUID_MetadataFormatXMPSeq, xmpseqW },
    { &GUID_MetadataFormatXMPAlt, xmpaltW },
    { &GUID_MetadataFormatLSD, logscrdescW },
    { &GUID_MetadataFormatIMD, imgdescW },
    { &GUID_MetadataFormatGCE, grctlextW },
    { &GUID_MetadataFormatAPE, appextW },
    { &GUID_MetadataFormatJpegChrominance, chrominanceW },
    { &GUID_MetadataFormatJpegLuminance, luminanceW },
    { &GUID_MetadataFormatJpegComment, comW },
    { &GUID_MetadataFormatGifComment, commentextW },
    { &GUID_MetadataFormatChunkgAMA, gAMAW },
    { &GUID_MetadataFormatChunkbKGD, bKGDW },
    { &GUID_MetadataFormatChunkiTXt, iTXtW },
    { &GUID_MetadataFormatChunkcHRM, cHRMW },
    { &GUID_MetadataFormatChunkhIST, hISTW },
    { &GUID_MetadataFormatChunkiCCP, iCCPW },
    { &GUID_MetadataFormatChunksRGB, sRGBW },
    { &GUID_MetadataFormatChunktIME, tIMEW }
};

HRESULT WINAPI WICMapGuidToShortName(REFGUID guid, UINT len, WCHAR *name, UINT *ret_len)
{
    UINT i;

    TRACE("%s,%u,%p,%p\n", wine_dbgstr_guid(guid), len, name, ret_len);

    if (!guid) return E_INVALIDARG;

    for (i = 0; i < sizeof(guid2name)/sizeof(guid2name[0]); i++)
    {
        if (IsEqualGUID(guid, guid2name[i].guid))
        {
            if (name)
            {
                if (!len) return E_INVALIDARG;

                len = min(len - 1, lstrlenW(guid2name[i].name));
                memcpy(name, guid2name[i].name, len * sizeof(WCHAR));
                name[len] = 0;

                if (len < lstrlenW(guid2name[i].name))
                    return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            }
            if (ret_len) *ret_len = lstrlenW(guid2name[i].name) + 1;
            return S_OK;
        }
    }

    return WINCODEC_ERR_PROPERTYNOTFOUND;
}
