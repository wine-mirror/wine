/*
 * Misleadingly named convenience functions for accessing WIC.
 *
 * Copyright 2012 Vincent Povirk for CodeWeavers
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
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "wincodec.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

HRESULT WINAPI IWICBitmapFlipRotator_Initialize_Proxy_W(IWICBitmapFlipRotator *iface,
    IWICBitmapSource *pISource, WICBitmapTransformOptions options)
{
    return IWICBitmapFlipRotator_Initialize(iface, pISource, options);
}

HRESULT WINAPI IWICBitmapLock_GetDataPointer_Proxy_W(IWICBitmapLock *iface,
    UINT *pcbBufferSize, BYTE **ppbData)
{
    return IWICBitmapLock_GetDataPointer(iface, pcbBufferSize, ppbData);
}

HRESULT WINAPI IWICBitmapLock_GetStride_Proxy_W(IWICBitmapLock *iface,
    UINT *pcbStride)
{
    return IWICBitmapLock_GetStride(iface, pcbStride);
}

HRESULT WINAPI IWICBitmapSource_GetSize_Proxy_W(IWICBitmapSource *iface,
    UINT *puiWidth, UINT *puiHeight)
{
    return IWICBitmapSource_GetSize(iface, puiWidth, puiHeight);
}

HRESULT WINAPI IWICBitmapSource_GetPixelFormat_Proxy_W(IWICBitmapSource *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    return IWICBitmapSource_GetPixelFormat(iface, pPixelFormat);
}

HRESULT WINAPI IWICBitmapSource_GetResolution_Proxy_W(IWICBitmapSource *iface,
    double *pDpiX, double *pDpiY)
{
    return IWICBitmapSource_GetResolution(iface, pDpiX, pDpiY);
}

HRESULT WINAPI IWICBitmapSource_CopyPalette_Proxy_W(IWICBitmapSource *iface,
    IWICPalette *pIPalette)
{
    return IWICBitmapSource_CopyPalette(iface, pIPalette);
}

HRESULT WINAPI IWICBitmapSource_CopyPixels_Proxy_W(IWICBitmapSource *iface,
    const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer)
{
    return IWICBitmapSource_CopyPixels(iface, prc, cbStride, cbBufferSize, pbBuffer);
}

HRESULT WINAPI IWICBitmap_Lock_Proxy_W(IWICBitmap *iface,
    const WICRect *prcLock, DWORD flags, IWICBitmapLock **ppILock)
{
    return IWICBitmap_Lock(iface, prcLock, flags, ppILock);
}

HRESULT WINAPI IWICBitmap_SetPalette_Proxy_W(IWICBitmap *iface,
    IWICPalette *pIPalette)
{
    return IWICBitmap_SetPalette(iface, pIPalette);
}

HRESULT WINAPI IWICBitmap_SetResolution_Proxy_W(IWICBitmap *iface,
    double dpiX, double dpiY)
{
    return IWICBitmap_SetResolution(iface, dpiX, dpiY);
}

HRESULT WINAPI IWICColorContext_InitializeFromMemory_Proxy_W(IWICColorContext *iface,
    const BYTE *pbBuffer, UINT cbBufferSize)
{
    return IWICColorContext_InitializeFromMemory(iface, pbBuffer, cbBufferSize);
}

HRESULT WINAPI IWICFastMetadataEncoder_Commit_Proxy_W(IWICFastMetadataEncoder *iface)
{
    return IWICFastMetadataEncoder_Commit(iface);
}

HRESULT WINAPI IWICFastMetadataEncoder_GetMetadataQueryWriter_Proxy_W(IWICFastMetadataEncoder *iface,
    IWICMetadataQueryWriter **ppIMetadataQueryWriter)
{
    return IWICFastMetadataEncoder_GetMetadataQueryWriter(iface, ppIMetadataQueryWriter);
}

HRESULT WINAPI IWICImagingFactory_CreateBitmapClipper_Proxy_W(IWICImagingFactory *pFactory,
    IWICBitmapClipper **ppIBitmapClipper)
{
    return IWICImagingFactory_CreateBitmapClipper(pFactory, ppIBitmapClipper);
}

HRESULT WINAPI IWICImagingFactory_CreateBitmapFlipRotator_Proxy_W(IWICImagingFactory *pFactory,
    IWICBitmapFlipRotator **ppIBitmapFlipRotator)
{
    return IWICImagingFactory_CreateBitmapFlipRotator(pFactory, ppIBitmapFlipRotator);
}

HRESULT WINAPI IWICImagingFactory_CreateBitmapFromHBITMAP_Proxy_W(IWICImagingFactory *pFactory,
    HBITMAP hBitmap, HPALETTE hPalette, WICBitmapAlphaChannelOption options, IWICBitmap **ppIBitmap)
{
    return IWICImagingFactory_CreateBitmapFromHBITMAP(pFactory, hBitmap, hPalette, options, ppIBitmap);
}

HRESULT WINAPI IWICImagingFactory_CreateBitmapFromHICON_Proxy_W(IWICImagingFactory *pFactory,
    HICON hIcon, IWICBitmap **ppIBitmap)
{
    return IWICImagingFactory_CreateBitmapFromHICON(pFactory, hIcon, ppIBitmap);
}

HRESULT WINAPI IWICImagingFactory_CreateBitmapFromMemory_Proxy_W(IWICImagingFactory *pFactory,
    UINT uiWidth, UINT uiHeight, REFWICPixelFormatGUID pixelFormat, UINT cbStride,
    UINT cbBufferSize, BYTE *pbBuffer, IWICBitmap **ppIBitmap)
{
    return IWICImagingFactory_CreateBitmapFromMemory(pFactory, uiWidth, uiHeight,
        pixelFormat, cbStride, cbBufferSize, pbBuffer, ppIBitmap);
}

HRESULT WINAPI IWICImagingFactory_CreateBitmapFromSource_Proxy_W(IWICImagingFactory *pFactory,
    IWICBitmapSource *piBitmapSource, WICBitmapCreateCacheOption option, IWICBitmap **ppIBitmap)
{
    return IWICImagingFactory_CreateBitmapFromSource(pFactory, piBitmapSource, option, ppIBitmap);
}

HRESULT WINAPI IWICImagingFactory_CreateBitmapScaler_Proxy_W(IWICImagingFactory *pFactory,
    IWICBitmapScaler **ppIBitmapScaler)
{
    return IWICImagingFactory_CreateBitmapScaler(pFactory, ppIBitmapScaler);
}

HRESULT WINAPI IWICImagingFactory_CreateBitmap_Proxy_W(IWICImagingFactory *pFactory,
    UINT uiWidth, UINT uiHeight, REFWICPixelFormatGUID pixelFormat,
    WICBitmapCreateCacheOption option, IWICBitmap **ppIBitmap)
{
    return IWICImagingFactory_CreateBitmap(pFactory, uiWidth, uiHeight,
        pixelFormat, option, ppIBitmap);
}

HRESULT WINAPI IWICImagingFactory_CreateComponentInfo_Proxy_W(IWICImagingFactory *pFactory,
    REFCLSID clsidComponent, IWICComponentInfo **ppIInfo)
{
    return IWICImagingFactory_CreateComponentInfo(pFactory, clsidComponent, ppIInfo);
}

HRESULT WINAPI IWICImagingFactory_CreateDecoderFromFileHandle_Proxy_W(IWICImagingFactory *pFactory,
    ULONG_PTR hFile, const GUID *pguidVendor, WICDecodeOptions metadataOptions, IWICBitmapDecoder **ppIDecoder)
{
    return IWICImagingFactory_CreateDecoderFromFileHandle(pFactory, hFile, pguidVendor, metadataOptions, ppIDecoder);
}

HRESULT WINAPI IWICImagingFactory_CreateDecoderFromFilename_Proxy_W(IWICImagingFactory *pFactory,
    LPCWSTR wzFilename, const GUID *pguidVendor, DWORD dwDesiredAccess,
    WICDecodeOptions metadataOptions, IWICBitmapDecoder **ppIDecoder)
{
    return IWICImagingFactory_CreateDecoderFromFilename(pFactory, wzFilename,
        pguidVendor, dwDesiredAccess, metadataOptions, ppIDecoder);
}

HRESULT WINAPI IWICImagingFactory_CreateDecoderFromStream_Proxy_W(IWICImagingFactory *pFactory,
    IStream *pIStream, const GUID *pguidVendor,
    WICDecodeOptions metadataOptions, IWICBitmapDecoder **ppIDecoder)
{
    return IWICImagingFactory_CreateDecoderFromStream(pFactory, pIStream, pguidVendor, metadataOptions, ppIDecoder);
}

HRESULT WINAPI IWICImagingFactory_CreateEncoder_Proxy_W(IWICImagingFactory *pFactory,
    REFGUID guidContainerFormat, const GUID *pguidVendor, IWICBitmapEncoder **ppIEncoder)
{
    return IWICImagingFactory_CreateEncoder(pFactory, guidContainerFormat, pguidVendor, ppIEncoder);
}

HRESULT WINAPI IWICImagingFactory_CreateFastMetadataEncoderFromDecoder_Proxy_W(IWICImagingFactory *pFactory,
    IWICBitmapDecoder *pIDecoder, IWICFastMetadataEncoder **ppIFastEncoder)
{
    return IWICImagingFactory_CreateFastMetadataEncoderFromDecoder(pFactory, pIDecoder, ppIFastEncoder);
}

HRESULT WINAPI IWICImagingFactory_CreateFastMetadataEncoderFromFrameDecode_Proxy_W(IWICImagingFactory *pFactory,
    IWICBitmapFrameDecode *pIFrameDecoder, IWICFastMetadataEncoder **ppIFastEncoder)
{
    return IWICImagingFactory_CreateFastMetadataEncoderFromFrameDecode(pFactory, pIFrameDecoder, ppIFastEncoder);
}

HRESULT WINAPI IWICImagingFactory_CreateFormatConverter_Proxy_W(IWICImagingFactory *pFactory,
    IWICFormatConverter **ppIFormatConverter)
{
    return IWICImagingFactory_CreateFormatConverter(pFactory, ppIFormatConverter);
}

HRESULT WINAPI IWICImagingFactory_CreatePalette_Proxy_W(IWICImagingFactory *pFactory,
    IWICPalette **ppIPalette)
{
    return IWICImagingFactory_CreatePalette(pFactory, ppIPalette);
}

HRESULT WINAPI IWICImagingFactory_CreateQueryWriterFromReader_Proxy_W(IWICImagingFactory *pFactory,
    IWICMetadataQueryReader *pIQueryReader, const GUID *pguidVendor,
    IWICMetadataQueryWriter **ppIQueryWriter)
{
    return IWICImagingFactory_CreateQueryWriterFromReader(pFactory, pIQueryReader, pguidVendor, ppIQueryWriter);
}

HRESULT WINAPI IWICImagingFactory_CreateQueryWriter_Proxy_W(IWICImagingFactory *pFactory,
    REFGUID guidMetadataFormat, const GUID *pguidVendor, IWICMetadataQueryWriter **ppIQueryWriter)
{
    return IWICImagingFactory_CreateQueryWriter(pFactory, guidMetadataFormat, pguidVendor, ppIQueryWriter);
}

HRESULT WINAPI IWICImagingFactory_CreateStream_Proxy_W(IWICImagingFactory *pFactory,
    IWICStream **ppIWICStream)
{
    return IWICImagingFactory_CreateStream(pFactory, ppIWICStream);
}

HRESULT WINAPI WICCreateImagingFactory_Proxy(UINT SDKVersion, IWICImagingFactory **ppIImagingFactory)
{
    TRACE("%x, %p\n", SDKVersion, ppIImagingFactory);

    return ImagingFactory_CreateInstance(NULL, &IID_IWICImagingFactory, (void**)ppIImagingFactory);
}
