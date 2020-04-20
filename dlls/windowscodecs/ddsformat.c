/*
 * Copyright 2020 Ziqing Hui
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
#include "wine/port.h"

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

#define DDS_MAGIC 0x20534444

typedef struct {
    DWORD size;
    DWORD flags;
    DWORD fourCC;
    DWORD rgbBitCount;
    DWORD rBitMask;
    DWORD gBitMask;
    DWORD bBitMask;
    DWORD aBitMask;
} DDS_PIXELFORMAT;

typedef struct {
    DWORD size;
    DWORD flags;
    DWORD height;
    DWORD width;
    DWORD pitchOrLinearSize;
    DWORD depth;
    DWORD mipMapCount;
    DWORD reserved1[11];
    DDS_PIXELFORMAT ddspf;
    DWORD caps;
    DWORD caps2;
    DWORD caps3;
    DWORD caps4;
    DWORD reserved2;
} DDS_HEADER;

typedef struct DdsDecoder {
    IWICBitmapDecoder IWICBitmapDecoder_iface;
    LONG ref;
    BOOL initialized;
    IStream *stream;
    CRITICAL_SECTION lock;
    DDS_HEADER header;
} DdsDecoder;

static inline DdsDecoder *impl_from_IWICBitmapDecoder(IWICBitmapDecoder *iface)
{
    return CONTAINING_RECORD(iface, DdsDecoder, IWICBitmapDecoder_iface);
}

static HRESULT WINAPI DdsDecoder_QueryInterface(IWICBitmapDecoder *iface, REFIID iid,
                                                void **ppv)
{
    DdsDecoder *This = impl_from_IWICBitmapDecoder(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapDecoder, iid))
    {
        *ppv = &This->IWICBitmapDecoder_iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI DdsDecoder_AddRef(IWICBitmapDecoder *iface)
{
    DdsDecoder *This = impl_from_IWICBitmapDecoder(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI DdsDecoder_Release(IWICBitmapDecoder *iface)
{
    DdsDecoder *This = impl_from_IWICBitmapDecoder(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        if (This->stream) IStream_Release(This->stream);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI DdsDecoder_QueryCapability(IWICBitmapDecoder *iface, IStream *stream,
                                                 DWORD *capability)
{
    FIXME("(%p,%p,%p): stub.\n", iface, stream, capability);

    return E_NOTIMPL;
}

static HRESULT WINAPI DdsDecoder_Initialize(IWICBitmapDecoder *iface, IStream *pIStream,
                                            WICDecodeOptions cacheOptions)
{
    DdsDecoder *This = impl_from_IWICBitmapDecoder(iface);
    HRESULT hr;
    LARGE_INTEGER seek;
    DWORD magic;
    ULONG bytesread;

    TRACE("(%p,%p,%x)\n", iface, pIStream, cacheOptions);

    EnterCriticalSection(&This->lock);

    if (This->initialized) {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }

    seek.QuadPart = 0;
    hr = IStream_Seek(pIStream, seek, SEEK_SET, NULL);
    if (FAILED(hr)) goto end;

    hr = IStream_Read(pIStream, &magic, sizeof(magic), &bytesread);
    if (FAILED(hr)) goto end;
    if (bytesread != sizeof(magic)) {
        hr = WINCODEC_ERR_STREAMREAD;
        goto end;
    }
    if (magic != DDS_MAGIC) {
        hr = WINCODEC_ERR_UNKNOWNIMAGEFORMAT;
        goto end;
    }

    hr = IStream_Read(pIStream, &This->header, sizeof(This->header), &bytesread);
    if (FAILED(hr)) goto end;
    if (bytesread != sizeof(This->header)) {
        hr = WINCODEC_ERR_STREAMREAD;
        goto end;
    }
    if (This->header.size != sizeof(This->header)) {
        hr = WINCODEC_ERR_BADHEADER;
        goto end;
    }

    This->initialized = TRUE;
    This->stream = pIStream;
    IStream_AddRef(pIStream);

end:
    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI DdsDecoder_GetContainerFormat(IWICBitmapDecoder *iface,
                                                    GUID *pguidContainerFormat)
{
    memcpy(pguidContainerFormat, &GUID_ContainerFormatDds, sizeof(GUID));
    return S_OK;
}

static HRESULT WINAPI DdsDecoder_GetDecoderInfo(IWICBitmapDecoder *iface,
                                                IWICBitmapDecoderInfo **ppIDecoderInfo)
{
    FIXME("(%p,%p): stub.\n", iface, ppIDecoderInfo);

    return E_NOTIMPL;
}

static HRESULT WINAPI DdsDecoder_CopyPalette(IWICBitmapDecoder *iface,
                                             IWICPalette *pIPalette)
{
    TRACE("(%p,%p)\n", iface, pIPalette);

    return WINCODEC_ERR_PALETTEUNAVAILABLE;
}

static HRESULT WINAPI DdsDecoder_GetMetadataQueryReader(IWICBitmapDecoder *iface,
                                                        IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    if (!ppIMetadataQueryReader) return E_INVALIDARG;

    FIXME("(%p,%p)\n", iface, ppIMetadataQueryReader);

    return E_NOTIMPL;
}

static HRESULT WINAPI DdsDecoder_GetPreview(IWICBitmapDecoder *iface,
                                            IWICBitmapSource **ppIBitmapSource)
{
    TRACE("(%p,%p)\n", iface, ppIBitmapSource);

    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI DdsDecoder_GetColorContexts(IWICBitmapDecoder *iface,
                                                  UINT cCount, IWICColorContext **ppDdslorContexts, UINT *pcActualCount)
{
    TRACE("(%p,%u,%p,%p)\n", iface, cCount, ppDdslorContexts, pcActualCount);

    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI DdsDecoder_GetThumbnail(IWICBitmapDecoder *iface,
                                              IWICBitmapSource **ppIThumbnail)
{
    TRACE("(%p,%p)\n", iface, ppIThumbnail);

    return WINCODEC_ERR_CODECNOTHUMBNAIL;
}

static HRESULT WINAPI DdsDecoder_GetFrameCount(IWICBitmapDecoder *iface,
                                               UINT *pCount)
{
    FIXME("(%p) <-- %d: stub.\n", iface, *pCount);

    return E_NOTIMPL;
}

static HRESULT WINAPI DdsDecoder_GetFrame(IWICBitmapDecoder *iface,
                                          UINT index, IWICBitmapFrameDecode **ppIBitmapFrame)
{
    FIXME("(%p,%u,%p): stub.\n", iface, index, ppIBitmapFrame);

    return E_NOTIMPL;
}

static const IWICBitmapDecoderVtbl DdsDecoder_Vtbl = {
        DdsDecoder_QueryInterface,
        DdsDecoder_AddRef,
        DdsDecoder_Release,
        DdsDecoder_QueryCapability,
        DdsDecoder_Initialize,
        DdsDecoder_GetContainerFormat,
        DdsDecoder_GetDecoderInfo,
        DdsDecoder_CopyPalette,
        DdsDecoder_GetMetadataQueryReader,
        DdsDecoder_GetPreview,
        DdsDecoder_GetColorContexts,
        DdsDecoder_GetThumbnail,
        DdsDecoder_GetFrameCount,
        DdsDecoder_GetFrame
};

HRESULT DdsDecoder_CreateInstance(REFIID iid, void** ppv)
{
    DdsDecoder *This;
    HRESULT ret;

    TRACE("(%s,%p)\n", debugstr_guid(iid), ppv);

    *ppv = NULL;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(DdsDecoder));
    if (!This) return E_OUTOFMEMORY;

    This->IWICBitmapDecoder_iface.lpVtbl = &DdsDecoder_Vtbl;
    This->ref = 1;
    This->initialized = FALSE;
    This->stream = NULL;
    InitializeCriticalSection(&This->lock);
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": DdsDecoder.lock");

    ret = IWICBitmapDecoder_QueryInterface(&This->IWICBitmapDecoder_iface, iid, ppv);
    IWICBitmapDecoder_Release(&This->IWICBitmapDecoder_iface);

    return ret;
}
