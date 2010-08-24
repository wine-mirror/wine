/*
 * Copyright 2010 Damjan Jovanovic
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

#ifdef HAVE_ICNS_H
#include <icns.h>
#endif

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "wincodec.h"

#include "wincodecs_private.h"

#include "wine/debug.h"
#include "wine/library.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

#ifdef SONAME_LIBICNS

static void *libicns_handle;
#define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(icns_create_family);
MAKE_FUNCPTR(icns_export_family_data);
MAKE_FUNCPTR(icns_free_image);
MAKE_FUNCPTR(icns_get_mask_type_for_icon_type);
MAKE_FUNCPTR(icns_get_type_from_image_info);
MAKE_FUNCPTR(icns_init_image_for_type);
MAKE_FUNCPTR(icns_new_element_from_image);
MAKE_FUNCPTR(icns_set_element_in_family);
#undef MAKE_FUNCPTR

static void *load_libicns(void)
{
    if((libicns_handle = wine_dlopen(SONAME_LIBICNS, RTLD_NOW, NULL, 0)) != NULL) {

#define LOAD_FUNCPTR(f) \
    if((p##f = wine_dlsym(libicns_handle, #f, NULL, 0)) == NULL) { \
        libicns_handle = NULL; \
        return NULL; \
    }
        LOAD_FUNCPTR(icns_create_family);
        LOAD_FUNCPTR(icns_export_family_data);
        LOAD_FUNCPTR(icns_free_image);
        LOAD_FUNCPTR(icns_get_mask_type_for_icon_type);
        LOAD_FUNCPTR(icns_get_type_from_image_info);
        LOAD_FUNCPTR(icns_init_image_for_type);
        LOAD_FUNCPTR(icns_new_element_from_image);
        LOAD_FUNCPTR(icns_set_element_in_family);
#undef LOAD_FUNCPTR
    }
    return libicns_handle;
}

typedef struct IcnsEncoder {
    const IWICBitmapEncoderVtbl *lpVtbl;
    LONG ref;
    IStream *stream;
    icns_family_t *icns_family;
    BOOL any_frame_committed;
    int outstanding_commits;
    BOOL committed;
    CRITICAL_SECTION lock;
} IcnsEncoder;

typedef struct IcnsFrameEncode {
    const IWICBitmapFrameEncodeVtbl *lpVtbl;
    IcnsEncoder *encoder;
    LONG ref;
    BOOL initialized;
    UINT width;
    UINT height;
    icns_type_t icns_type;
    icns_image_t icns_image;
    int lines_written;
    BOOL committed;
} IcnsFrameEncode;

static HRESULT WINAPI IcnsFrameEncode_QueryInterface(IWICBitmapFrameEncode *iface, REFIID iid,
    void **ppv)
{
    IcnsFrameEncode *This = (IcnsFrameEncode*)iface;
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapFrameEncode, iid))
    {
        *ppv = &This->lpVtbl;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI IcnsFrameEncode_AddRef(IWICBitmapFrameEncode *iface)
{
    IcnsFrameEncode *This = (IcnsFrameEncode*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI IcnsFrameEncode_Release(IWICBitmapFrameEncode *iface)
{
    IcnsFrameEncode *This = (IcnsFrameEncode*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        if (!This->committed)
        {
            EnterCriticalSection(&This->encoder->lock);
            This->encoder->outstanding_commits--;
            LeaveCriticalSection(&This->encoder->lock);
        }
        if (This->icns_image.imageData != NULL)
            picns_free_image(&This->icns_image);

        IUnknown_Release((IUnknown*)This->encoder);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI IcnsFrameEncode_Initialize(IWICBitmapFrameEncode *iface,
    IPropertyBag2 *pIEncoderOptions)
{
    IcnsFrameEncode *This = (IcnsFrameEncode*)iface;
    HRESULT hr = S_OK;

    TRACE("(%p,%p)\n", iface, pIEncoderOptions);

    EnterCriticalSection(&This->encoder->lock);

    if (This->initialized)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }
    This->initialized = TRUE;

end:
    LeaveCriticalSection(&This->encoder->lock);
    return hr;
}

static HRESULT WINAPI IcnsFrameEncode_SetSize(IWICBitmapFrameEncode *iface,
    UINT uiWidth, UINT uiHeight)
{
    IcnsFrameEncode *This = (IcnsFrameEncode*)iface;
    HRESULT hr = S_OK;

    TRACE("(%p,%u,%u)\n", iface, uiWidth, uiHeight);

    EnterCriticalSection(&This->encoder->lock);

    if (!This->initialized || This->icns_image.imageData)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }

    This->width = uiWidth;
    This->height = uiHeight;

end:
    LeaveCriticalSection(&This->encoder->lock);
    return hr;
}

static HRESULT WINAPI IcnsFrameEncode_SetResolution(IWICBitmapFrameEncode *iface,
    double dpiX, double dpiY)
{
    IcnsFrameEncode *This = (IcnsFrameEncode*)iface;
    HRESULT hr = S_OK;

    TRACE("(%p,%0.2f,%0.2f)\n", iface, dpiX, dpiY);

    EnterCriticalSection(&This->encoder->lock);

    if (!This->initialized || This->icns_image.imageData)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }

end:
    LeaveCriticalSection(&This->encoder->lock);
    return S_OK;
}

static HRESULT WINAPI IcnsFrameEncode_SetPixelFormat(IWICBitmapFrameEncode *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    IcnsFrameEncode *This = (IcnsFrameEncode*)iface;
    HRESULT hr = S_OK;

    TRACE("(%p,%s)\n", iface, debugstr_guid(pPixelFormat));

    EnterCriticalSection(&This->encoder->lock);

    if (!This->initialized || This->icns_image.imageData)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }

    memcpy(pPixelFormat, &GUID_WICPixelFormat32bppBGRA, sizeof(GUID));

end:
    LeaveCriticalSection(&This->encoder->lock);
    return S_OK;
}

static HRESULT WINAPI IcnsFrameEncode_SetColorContexts(IWICBitmapFrameEncode *iface,
    UINT cCount, IWICColorContext **ppIColorContext)
{
    FIXME("(%p,%u,%p): stub\n", iface, cCount, ppIColorContext);
    return E_NOTIMPL;
}

static HRESULT WINAPI IcnsFrameEncode_SetPalette(IWICBitmapFrameEncode *iface,
    IWICPalette *pIPalette)
{
    FIXME("(%p,%p): stub\n", iface, pIPalette);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI IcnsFrameEncode_SetThumbnail(IWICBitmapFrameEncode *iface,
    IWICBitmapSource *pIThumbnail)
{
    FIXME("(%p,%p): stub\n", iface, pIThumbnail);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI IcnsFrameEncode_WritePixels(IWICBitmapFrameEncode *iface,
    UINT lineCount, UINT cbStride, UINT cbBufferSize, BYTE *pbPixels)
{
    IcnsFrameEncode *This = (IcnsFrameEncode*)iface;
    HRESULT hr = S_OK;
    UINT i;
    int ret;

    TRACE("(%p,%u,%u,%u,%p)\n", iface, lineCount, cbStride, cbBufferSize, pbPixels);

    EnterCriticalSection(&This->encoder->lock);

    if (!This->initialized || !This->width || !This->height)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }
    if (lineCount == 0 || lineCount + This->lines_written > This->height)
    {
        hr = E_INVALIDARG;
        goto end;
    }

    if (!This->icns_image.imageData)
    {
        icns_icon_info_t icns_info;
        icns_info.isImage = 1;
        icns_info.iconWidth = This->width;
        icns_info.iconHeight = This->height;
        icns_info.iconBitDepth = 32;
        icns_info.iconChannels = 4;
        icns_info.iconPixelDepth = icns_info.iconBitDepth / icns_info.iconChannels;
        This->icns_type = picns_get_type_from_image_info(icns_info);
        if (This->icns_type == ICNS_NULL_TYPE)
        {
            WARN("cannot generate ICNS icon from %dx%d image\n", This->width, This->height);
            hr = E_INVALIDARG;
            goto end;
        }
        ret = picns_init_image_for_type(This->icns_type, &This->icns_image);
        if (ret != ICNS_STATUS_OK)
        {
            WARN("error %d in icns_init_image_for_type\n", ret);
            hr = E_FAIL;
            goto end;
        }
    }

    for (i = 0; i < lineCount; i++)
    {
        BYTE *src_row, *dst_row;
        UINT j;
        src_row = pbPixels + cbStride * i;
        dst_row = This->icns_image.imageData + (This->lines_written + i)*(This->width*4);
        /* swap bgr -> rgb */
        for (j = 0; j < This->width*4; j += 4)
        {
            dst_row[j] = src_row[j+2];
            dst_row[j+1] = src_row[j+1];
            dst_row[j+2] = src_row[j];
            dst_row[j+3] = src_row[j+3];
        }
    }
    This->lines_written += lineCount;

end:
    LeaveCriticalSection(&This->encoder->lock);
    return hr;
}

static HRESULT WINAPI IcnsFrameEncode_WriteSource(IWICBitmapFrameEncode *iface,
    IWICBitmapSource *pIBitmapSource, WICRect *prc)
{
    IcnsFrameEncode *This = (IcnsFrameEncode*)iface;
    HRESULT hr;
    WICRect rc;
    WICPixelFormatGUID guid;
    UINT stride;
    BYTE *pixeldata = NULL;

    TRACE("(%p,%p,%p)\n", iface, pIBitmapSource, prc);

    if (!This->initialized || !This->width || !This->height)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }

    hr = IWICBitmapSource_GetPixelFormat(pIBitmapSource, &guid);
    if (FAILED(hr))
        goto end;
    if (!IsEqualGUID(&guid, &GUID_WICPixelFormat32bppBGRA))
    {
        FIXME("format %s unsupported, could use WICConvertBitmapSource to convert\n", debugstr_guid(&guid));
        hr = E_FAIL;
        goto end;
    }

    if (!prc)
    {
        UINT width, height;
        hr = IWICBitmapSource_GetSize(pIBitmapSource, &width, &height);
        if (FAILED(hr))
            goto end;
        rc.X = 0;
        rc.Y = 0;
        rc.Width = width;
        rc.Height = height;
        prc = &rc;
    }

    if (prc->Width != This->width)
    {
        hr = E_INVALIDARG;
        goto end;
    }

    stride = (32 * This->width + 7)/8;
    pixeldata = HeapAlloc(GetProcessHeap(), 0, stride * prc->Height);
    if (!pixeldata)
    {
        hr = E_OUTOFMEMORY;
        goto end;
    }

    hr = IWICBitmapSource_CopyPixels(pIBitmapSource, prc, stride,
        stride*prc->Height, pixeldata);
    if (SUCCEEDED(hr))
    {
        hr = IWICBitmapFrameEncode_WritePixels(iface, prc->Height, stride,
            stride*prc->Height, pixeldata);
    }

end:
    HeapFree(GetProcessHeap(), 0, pixeldata);
    return hr;
}

static HRESULT WINAPI IcnsFrameEncode_Commit(IWICBitmapFrameEncode *iface)
{
    IcnsFrameEncode *This = (IcnsFrameEncode*)iface;
    icns_element_t *icns_element = NULL;
    icns_image_t mask;
    icns_element_t *mask_element = NULL;
    int ret;
    int i;
    HRESULT hr = S_OK;

    TRACE("(%p): stub\n", iface);

    memset(&mask, 0, sizeof(mask));

    EnterCriticalSection(&This->encoder->lock);

    if (!This->icns_image.imageData || This->lines_written != This->height || This->committed)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }

    ret = picns_new_element_from_image(&This->icns_image, This->icns_type, &icns_element);
    if (ret != ICNS_STATUS_OK && icns_element != NULL)
    {
        WARN("icns_new_element_from_image failed with error %d\n", ret);
        hr = E_FAIL;
        goto end;
    }

    if (This->icns_type != ICNS_512x512_32BIT_ARGB_DATA && This->icns_type != ICNS_256x256_32BIT_ARGB_DATA)
    {
        /* we need to write the mask too */
        ret = picns_init_image_for_type(picns_get_mask_type_for_icon_type(This->icns_type), &mask);
        if (ret != ICNS_STATUS_OK)
        {
            WARN("icns_init_image_from_type failed to make mask, error %d\n", ret);
            hr = E_FAIL;
            goto end;
        }
        for (i = 0; i < mask.imageHeight; i++)
        {
            int j;
            for (j = 0; j < mask.imageWidth; j++)
                mask.imageData[i*mask.imageWidth + j] = This->icns_image.imageData[i*mask.imageWidth*4 + j*4 + 3];
        }
        ret = picns_new_element_from_image(&mask, picns_get_mask_type_for_icon_type(This->icns_type), &mask_element);
        if (ret != ICNS_STATUS_OK)
        {
            WARN("icns_new_element_from image failed to make element from mask, error %d\n", ret);
            hr = E_FAIL;
            goto end;
        }
    }

    ret = picns_set_element_in_family(&This->encoder->icns_family, icns_element);
    if (ret != ICNS_STATUS_OK)
    {
        WARN("icns_set_element_in_family failed for image with error %d\n", ret);
        hr = E_FAIL;
        goto end;
    }

    if (mask_element)
    {
        ret = picns_set_element_in_family(&This->encoder->icns_family, mask_element);
        if (ret != ICNS_STATUS_OK)
        {
            WARN("icns_set_element_in_family failed for mask with error %d\n", ret);
            hr = E_FAIL;
            goto end;
        }
    }

    This->committed = TRUE;
    This->encoder->any_frame_committed = TRUE;
    This->encoder->outstanding_commits--;

end:
    LeaveCriticalSection(&This->encoder->lock);
    picns_free_image(&mask);
    free(icns_element);
    free(mask_element);
    return hr;
}

static HRESULT WINAPI IcnsFrameEncode_GetMetadataQueryWriter(IWICBitmapFrameEncode *iface,
    IWICMetadataQueryWriter **ppIMetadataQueryWriter)
{
    FIXME("(%p, %p): stub\n", iface, ppIMetadataQueryWriter);
    return E_NOTIMPL;
}

static const IWICBitmapFrameEncodeVtbl IcnsEncoder_FrameVtbl = {
    IcnsFrameEncode_QueryInterface,
    IcnsFrameEncode_AddRef,
    IcnsFrameEncode_Release,
    IcnsFrameEncode_Initialize,
    IcnsFrameEncode_SetSize,
    IcnsFrameEncode_SetResolution,
    IcnsFrameEncode_SetPixelFormat,
    IcnsFrameEncode_SetColorContexts,
    IcnsFrameEncode_SetPalette,
    IcnsFrameEncode_SetThumbnail,
    IcnsFrameEncode_WritePixels,
    IcnsFrameEncode_WriteSource,
    IcnsFrameEncode_Commit,
    IcnsFrameEncode_GetMetadataQueryWriter
};

static HRESULT WINAPI IcnsEncoder_QueryInterface(IWICBitmapEncoder *iface, REFIID iid,
    void **ppv)
{
    IcnsEncoder *This = (IcnsEncoder*)iface;
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapEncoder, iid))
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

static ULONG WINAPI IcnsEncoder_AddRef(IWICBitmapEncoder *iface)
{
    IcnsEncoder *This = (IcnsEncoder*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI IcnsEncoder_Release(IWICBitmapEncoder *iface)
{
    IcnsEncoder *This = (IcnsEncoder*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        if (This->icns_family)
            free(This->icns_family);
        if (This->stream)
            IStream_Release(This->stream);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI IcnsEncoder_Initialize(IWICBitmapEncoder *iface,
    IStream *pIStream, WICBitmapEncoderCacheOption cacheOption)
{
    IcnsEncoder *This = (IcnsEncoder*)iface;
    int ret;
    HRESULT hr = S_OK;

    TRACE("(%p,%p,%u)\n", iface, pIStream, cacheOption);

    EnterCriticalSection(&This->lock);

    if (This->icns_family)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }
    ret = picns_create_family(&This->icns_family);
    if (ret != ICNS_STATUS_OK)
    {
        WARN("error %d creating icns family\n", ret);
        hr = E_FAIL;
        goto end;
    }
    IStream_AddRef(pIStream);
    This->stream = pIStream;

end:
    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI IcnsEncoder_GetContainerFormat(IWICBitmapEncoder *iface,
    GUID *pguidContainerFormat)
{
    FIXME("(%p,%s): stub\n", iface, debugstr_guid(pguidContainerFormat));
    return E_NOTIMPL;
}

static HRESULT WINAPI IcnsEncoder_GetEncoderInfo(IWICBitmapEncoder *iface,
    IWICBitmapEncoderInfo **ppIEncoderInfo)
{
    FIXME("(%p,%p): stub\n", iface, ppIEncoderInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI IcnsEncoder_SetColorContexts(IWICBitmapEncoder *iface,
    UINT cCount, IWICColorContext **ppIColorContext)
{
    FIXME("(%p,%u,%p): stub\n", iface, cCount, ppIColorContext);
    return E_NOTIMPL;
}

static HRESULT WINAPI IcnsEncoder_SetPalette(IWICBitmapEncoder *iface, IWICPalette *pIPalette)
{
    TRACE("(%p,%p)\n", iface, pIPalette);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI IcnsEncoder_SetThumbnail(IWICBitmapEncoder *iface, IWICBitmapSource *pIThumbnail)
{
    TRACE("(%p,%p)\n", iface, pIThumbnail);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI IcnsEncoder_SetPreview(IWICBitmapEncoder *iface, IWICBitmapSource *pIPreview)
{
    TRACE("(%p,%p)\n", iface, pIPreview);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI IcnsEncoder_CreateNewFrame(IWICBitmapEncoder *iface,
    IWICBitmapFrameEncode **ppIFrameEncode, IPropertyBag2 **ppIEncoderOptions)
{
    IcnsEncoder *This = (IcnsEncoder*)iface;
    HRESULT hr = S_OK;
    IcnsFrameEncode *frameEncode = NULL;

    TRACE("(%p,%p,%p)\n", iface, ppIFrameEncode, ppIEncoderOptions);

    EnterCriticalSection(&This->lock);

    if (!This->icns_family)
    {
        hr = WINCODEC_ERR_NOTINITIALIZED;
        goto end;
    }

    hr = CreatePropertyBag2(ppIEncoderOptions);
    if (FAILED(hr))
        goto end;

    frameEncode = HeapAlloc(GetProcessHeap(), 0, sizeof(IcnsFrameEncode));
    if (frameEncode == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto end;
    }
    frameEncode->lpVtbl = &IcnsEncoder_FrameVtbl;
    frameEncode->encoder = This;
    frameEncode->ref = 1;
    frameEncode->initialized = FALSE;
    frameEncode->width = 0;
    frameEncode->height = 0;
    memset(&frameEncode->icns_image, 0, sizeof(icns_image_t));
    frameEncode->lines_written = 0;
    frameEncode->committed = FALSE;
    *ppIFrameEncode = (IWICBitmapFrameEncode*)frameEncode;
    This->outstanding_commits++;
    IUnknown_AddRef((IUnknown*)This);

end:
    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI IcnsEncoder_Commit(IWICBitmapEncoder *iface)
{
    IcnsEncoder *This = (IcnsEncoder*)iface;
    icns_byte_t *buffer = NULL;
    icns_size_t buffer_size;
    int ret;
    HRESULT hr = S_OK;
    ULONG byteswritten;

    TRACE("(%p)\n", iface);

    EnterCriticalSection(&This->lock);

    if (!This->any_frame_committed || This->outstanding_commits > 0 || This->committed)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }

    ret = picns_export_family_data(This->icns_family, &buffer_size, &buffer);
    if (ret != ICNS_STATUS_OK)
    {
        WARN("icns_export_family_data failed with error %d\n", ret);
        hr = E_FAIL;
        goto end;
    }
    hr = IStream_Write(This->stream, buffer, buffer_size, &byteswritten);
    if (FAILED(hr) || byteswritten != buffer_size)
    {
        WARN("writing file failed, hr = 0x%08X\n", hr);
        hr = E_FAIL;
        goto end;
    }

    This->committed = TRUE;

end:
    LeaveCriticalSection(&This->lock);
    free(buffer);
    return hr;
}

static HRESULT WINAPI IcnsEncoder_GetMetadataQueryWriter(IWICBitmapEncoder *iface,
    IWICMetadataQueryWriter **ppIMetadataQueryWriter)
{
    FIXME("(%p,%p): stub\n", iface, ppIMetadataQueryWriter);
    return E_NOTIMPL;
}

static const IWICBitmapEncoderVtbl IcnsEncoder_Vtbl = {
    IcnsEncoder_QueryInterface,
    IcnsEncoder_AddRef,
    IcnsEncoder_Release,
    IcnsEncoder_Initialize,
    IcnsEncoder_GetContainerFormat,
    IcnsEncoder_GetEncoderInfo,
    IcnsEncoder_SetColorContexts,
    IcnsEncoder_SetPalette,
    IcnsEncoder_SetThumbnail,
    IcnsEncoder_SetPreview,
    IcnsEncoder_CreateNewFrame,
    IcnsEncoder_Commit,
    IcnsEncoder_GetMetadataQueryWriter
};

HRESULT IcnsEncoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv)
{
    IcnsEncoder *This;
    HRESULT ret;

    TRACE("(%p,%s,%p)\n", pUnkOuter, debugstr_guid(iid), ppv);

    *ppv = NULL;

    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    if (!libicns_handle && !load_libicns())
    {
        ERR("Failed writing ICNS because unable to find %s\n",SONAME_LIBICNS);
        return E_FAIL;
    }

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(IcnsEncoder));
    if (!This) return E_OUTOFMEMORY;

    This->lpVtbl = &IcnsEncoder_Vtbl;
    This->ref = 1;
    This->stream = NULL;
    This->icns_family = NULL;
    This->any_frame_committed = FALSE;
    This->outstanding_commits = 0;
    This->committed = FALSE;
    InitializeCriticalSection(&This->lock);
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": IcnsEncoder.lock");

    ret = IUnknown_QueryInterface((IUnknown*)This, iid, ppv);
    IUnknown_Release((IUnknown*)This);

    return ret;
}

#else /* !defined(SONAME_LIBICNS) */

HRESULT IcnsEncoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv)
{
    ERR("Trying to save ICNS picture, but ICNS support is not compiled in.\n");
    return E_FAIL;
}

#endif
