/*
 * Copyright (C) 2009 Tony Wasserka
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

#include "wine/debug.h"
#include "wine/unicode.h"
#include "d3dx9_36_private.h"

#include "initguid.h"
#include "wincodec.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);


/************************************************************
 * D3DXGetImageInfoFromFileInMemory
 *
 * Fills a D3DXIMAGE_INFO structure with info about an image
 *
 * PARAMS
 *   data     [I] pointer to the image file data
 *   datasize [I] size of the passed data
 *   info     [O] pointer to the destination structure
 *
 * RETURNS
 *   Success: D3D_OK, if info is not NULL and data and datasize make up a valid image file or
 *                    if info is NULL and data and datasize are not NULL
 *   Failure: D3DXERR_INVALIDDATA, if data is no valid image file and datasize and info are not NULL
 *            D3DERR_INVALIDCALL, if data is NULL or
 *                                if datasize is 0
 *
 * NOTES
 *   datasize may be bigger than the actual file size
 *
 */
HRESULT WINAPI D3DXGetImageInfoFromFileInMemory(LPCVOID data, UINT datasize, D3DXIMAGE_INFO *info)
{
    IWICImagingFactory *factory;
    IWICBitmapDecoder *decoder = NULL;
    IWICStream *stream;
    HRESULT hr;
    HRESULT initresult;

    FIXME("(%p, %d, %p): partially implemented\n", data, datasize, info);

    /* TODO: Add support for (or at least detect) TGA, DDS, PPM and DIB */

    if (!data || !datasize)
        return D3DERR_INVALIDCALL;

    if (!info)
        return D3D_OK;

    initresult = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, (void**)&factory);

    if (SUCCEEDED(hr)) {
        IWICImagingFactory_CreateStream(factory, &stream);
        IWICStream_InitializeFromMemory(stream, (BYTE*)data, datasize);
        hr = IWICImagingFactory_CreateDecoderFromStream(factory, (IStream*)stream, NULL, 0, &decoder);
        IStream_Release(stream);
        IWICImagingFactory_Release(factory);
    }

    if (SUCCEEDED(hr)) {
        GUID container_format;
        UINT frame_count;

        hr = IWICBitmapDecoder_GetContainerFormat(decoder, &container_format);
        if (SUCCEEDED(hr)) {
            if (IsEqualGUID(&container_format, &GUID_ContainerFormatBmp)) {
                TRACE("File type is BMP\n");
                info->ImageFileFormat = D3DXIFF_BMP;
            } else if (IsEqualGUID(&container_format, &GUID_ContainerFormatPng)) {
                TRACE("File type is PNG\n");
                info->ImageFileFormat = D3DXIFF_PNG;
            } else if(IsEqualGUID(&container_format, &GUID_ContainerFormatJpeg)) {
                TRACE("File type is JPG\n");
                info->ImageFileFormat = D3DXIFF_JPG;
            } else {
                WARN("Unsupported image file format %s\n", debugstr_guid(&container_format));
                hr = D3DXERR_INVALIDDATA;
            }
        }

        if (SUCCEEDED(hr))
            hr = IWICBitmapDecoder_GetFrameCount(decoder, &frame_count);
        if (SUCCEEDED(hr) && !frame_count)
            hr = D3DXERR_INVALIDDATA;

        if (SUCCEEDED(hr)) {
            IWICBitmapFrameDecode *frame = NULL;

            hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);

            if (SUCCEEDED(hr))
                hr = IWICBitmapFrameDecode_GetSize(frame, &info->Width, &info->Height);

            if (SUCCEEDED(hr)) {
                WICPixelFormatGUID pixel_format;

                hr = IWICBitmapFrameDecode_GetPixelFormat(frame, &pixel_format);
                if (SUCCEEDED(hr)) {
                    if (IsEqualGUID(&pixel_format, &GUID_WICPixelFormat1bppIndexed))
                        info->Format = D3DFMT_L8;
                    else if (IsEqualGUID(&pixel_format, &GUID_WICPixelFormat4bppIndexed))
                        info->Format = D3DFMT_L8;
                    else if (IsEqualGUID(&pixel_format, &GUID_WICPixelFormat8bppIndexed))
                        info->Format = D3DFMT_L8;
                    else if (IsEqualGUID(&pixel_format, &GUID_WICPixelFormat16bppBGR555))
                        info->Format = D3DFMT_X1R5G5B5;
                    else if (IsEqualGUID(&pixel_format, &GUID_WICPixelFormat24bppBGR))
                        info->Format = D3DFMT_R8G8B8;
                    else if (IsEqualGUID(&pixel_format, &GUID_WICPixelFormat32bppBGR))
                        info->Format = D3DFMT_X8R8G8B8;
                    else if (IsEqualGUID(&pixel_format, &GUID_WICPixelFormat32bppBGRA))
                        info->Format = D3DFMT_A8R8G8B8;
                    else {
                        WARN("Unsupported pixel format %s\n", debugstr_guid(&pixel_format));
                        hr = D3DXERR_INVALIDDATA;
                    }
                }
            }

            if (frame)
                 IWICBitmapFrameDecode_Release(frame);

            info->Depth = 1;
            info->MipLevels = 1;
            info->ResourceType = D3DRTYPE_TEXTURE;
        }
    }

    if (decoder)
        IWICBitmapDecoder_Release(decoder);

    if (SUCCEEDED(initresult))
        CoUninitialize();

    if (FAILED(hr)) {
        /* Missing formats are not detected yet and will fail silently without the FIXME */
        FIXME("Invalid or unsupported image file\n");
        return D3DXERR_INVALIDDATA;
    }

    return D3D_OK;
}

/************************************************************
 * D3DXGetImageInfoFromFile
 *
 * RETURNS
 *   Success: D3D_OK, if we successfully load a valid image file or
 *                    if we successfully load a file which is no valid image and info is NULL
 *   Failure: D3DXERR_INVALIDDATA, if we fail to load file or
 *                                 if file is not a valid image file and info is not NULL
 *            D3DERR_INVALIDCALL, if file is NULL
 *
 */
HRESULT WINAPI D3DXGetImageInfoFromFileA(LPCSTR file, D3DXIMAGE_INFO *info)
{
    LPWSTR widename;
    HRESULT hr;
    int strlength;

    TRACE("(%s, %p): relay\n", debugstr_a(file), info);

    if( !file ) return D3DERR_INVALIDCALL;

    strlength = MultiByteToWideChar(CP_ACP, 0, file, -1, NULL, 0);
    widename = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, strlength * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, file, -1, widename, strlength);

    hr = D3DXGetImageInfoFromFileW(widename, info);
    HeapFree(GetProcessHeap(), 0, widename);

    return hr;
}

HRESULT WINAPI D3DXGetImageInfoFromFileW(LPCWSTR file, D3DXIMAGE_INFO *info)
{
    HRESULT hr;
    DWORD size;
    LPVOID buffer;

    TRACE("(%s, %p): relay\n", debugstr_w(file), info);

    if( !file ) return D3DERR_INVALIDCALL;

    hr = map_view_of_file(file, &buffer, &size);
    if(FAILED(hr)) return D3DXERR_INVALIDDATA;

    hr = D3DXGetImageInfoFromFileInMemory(buffer, size, info);
    UnmapViewOfFile(buffer);

    return hr;
}

/************************************************************
 * D3DXGetImageInfoFromResource
 *
 * RETURNS
 *   Success: D3D_OK, if resource is a valid image file
 *   Failure: D3DXERR_INVALIDDATA, if resource is no valid image file or NULL or
 *                                 if we fail to load resource
 *
 */
HRESULT WINAPI D3DXGetImageInfoFromResourceA(HMODULE module, LPCSTR resource, D3DXIMAGE_INFO *info)
{
    HRSRC resinfo;

    TRACE("(%p, %s, %p)\n", module, debugstr_a(resource), info);

    resinfo = FindResourceA(module, resource, (LPCSTR)RT_RCDATA);
    if(resinfo) {
        LPVOID buffer;
        HRESULT hr;
        DWORD size;

        hr = load_resource_into_memory(module, resinfo, &buffer, &size);
        if(FAILED(hr)) return D3DXERR_INVALIDDATA;
        return D3DXGetImageInfoFromFileInMemory(buffer, size, info);
    }

    resinfo = FindResourceA(module, resource, (LPCSTR)RT_BITMAP);
    if(resinfo) {
        FIXME("Implement loading bitmaps from resource type RT_BITMAP\n");
        return E_NOTIMPL;
    }
    return D3DXERR_INVALIDDATA;
}

HRESULT WINAPI D3DXGetImageInfoFromResourceW(HMODULE module, LPCWSTR resource, D3DXIMAGE_INFO *info)
{
    HRSRC resinfo;

    TRACE("(%p, %s, %p)\n", module, debugstr_w(resource), info);

    resinfo = FindResourceW(module, resource, (LPCWSTR)RT_RCDATA);
    if(resinfo) {
        LPVOID buffer;
        HRESULT hr;
        DWORD size;

        hr = load_resource_into_memory(module, resinfo, &buffer, &size);
        if(FAILED(hr)) return D3DXERR_INVALIDDATA;
        return D3DXGetImageInfoFromFileInMemory(buffer, size, info);
    }

    resinfo = FindResourceW(module, resource, (LPCWSTR)RT_BITMAP);
    if(resinfo) {
        FIXME("Implement loading bitmaps from resource type RT_BITMAP\n");
        return E_NOTIMPL;
    }
    return D3DXERR_INVALIDDATA;
}

/************************************************************
 * D3DXLoadSurfaceFromFileInMemory
 *
 * Loads data from a given buffer into a surface and fills a given
 * D3DXIMAGE_INFO structure with info about the source data.
 *
 * PARAMS
 *   pDestSurface [I] pointer to the surface
 *   pDestPalette [I] palette to use
 *   pDestRect    [I] to be filled area of the surface
 *   pSrcData     [I] pointer to the source data
 *   SrcDataSize  [I] size of the source data in bytes
 *   pSrcRect     [I] area of the source data to load
 *   dwFilter     [I] filter to apply on stretching
 *   Colorkey     [I] colorkey
 *   pSrcInfo     [O] pointer to a D3DXIMAGE_INFO structure
 *
 * RETURNS
 *   Success: D3D_OK
 *   Failure: D3DERR_INVALIDCALL, if pDestSurface or pSrcData or SrcDataSize are NULL
 *            D3DXERR_INVALIDDATA, if pSrcData is no valid image file
 *
 */
HRESULT WINAPI D3DXLoadSurfaceFromFileInMemory(LPDIRECT3DSURFACE9 pDestSurface,
                                               CONST PALETTEENTRY *pDestPalette,
                                               CONST RECT *pDestRect,
                                               LPCVOID pSrcData,
                                               UINT SrcDataSize,
                                               CONST RECT *pSrcRect,
                                               DWORD dwFilter,
                                               D3DCOLOR Colorkey,
                                               D3DXIMAGE_INFO *pSrcInfo)
{
    FIXME("(%p, %p, %p, %p, %d, %p, %d, %x, %p): stub\n", pDestSurface, pDestPalette,
        pDestRect, pSrcData, SrcDataSize, pSrcRect, dwFilter, Colorkey, pSrcInfo);

    if( !pDestSurface || !pSrcData | !SrcDataSize ) return D3DERR_INVALIDCALL;
    return E_NOTIMPL;
}

/************************************************************
 * D3DXLoadSurfaceFromFile
 */
HRESULT WINAPI D3DXLoadSurfaceFromFileA(LPDIRECT3DSURFACE9 pDestSurface,
                                        CONST PALETTEENTRY *pDestPalette,
                                        CONST RECT *pDestRect,
                                        LPCSTR pSrcFile,
                                        CONST RECT *pSrcRect,
                                        DWORD dwFilter,
                                        D3DCOLOR Colorkey,
                                        D3DXIMAGE_INFO *pSrcInfo)
{
    LPWSTR pWidename;
    HRESULT hr;
    int strlength;

    TRACE("(%p, %p, %p, %s, %p, %u, %#x, %p): relay\n", pDestSurface, pDestPalette, pDestRect, debugstr_a(pSrcFile),
        pSrcRect, dwFilter, Colorkey, pSrcInfo);

    if( !pSrcFile || !pDestSurface ) return D3DERR_INVALIDCALL;

    strlength = MultiByteToWideChar(CP_ACP, 0, pSrcFile, -1, NULL, 0);
    pWidename = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, strlength * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, pSrcFile, -1, pWidename, strlength);

    hr = D3DXLoadSurfaceFromFileW(pDestSurface, pDestPalette, pDestRect, pWidename, pSrcRect, dwFilter, Colorkey, pSrcInfo);
    HeapFree(GetProcessHeap(), 0, pWidename);

    return hr;
}

HRESULT WINAPI D3DXLoadSurfaceFromFileW(LPDIRECT3DSURFACE9 pDestSurface,
                                        CONST PALETTEENTRY *pDestPalette,
                                        CONST RECT *pDestRect,
                                        LPCWSTR pSrcFile,
                                        CONST RECT *pSrcRect,
                                        DWORD Filter,
                                        D3DCOLOR Colorkey,
                                        D3DXIMAGE_INFO *pSrcInfo)
{
    HRESULT hr;
    DWORD dwSize;
    LPVOID pBuffer;

    TRACE("(%p, %p, %p, %s, %p, %u, %#x, %p): relay\n", pDestSurface, pDestPalette, pDestRect, debugstr_w(pSrcFile),
        pSrcRect, Filter, Colorkey, pSrcInfo);

    if( !pSrcFile || !pDestSurface ) return D3DERR_INVALIDCALL;

    hr = map_view_of_file(pSrcFile, &pBuffer, &dwSize);
    if(FAILED(hr)) return D3DXERR_INVALIDDATA;

    hr = D3DXLoadSurfaceFromFileInMemory(pDestSurface, pDestPalette, pDestRect, pBuffer, dwSize, pSrcRect, Filter, Colorkey, pSrcInfo);
    UnmapViewOfFile(pBuffer);

    return hr;
}

/************************************************************
 * D3DXLoadSurfaceFromResource
 */
HRESULT WINAPI D3DXLoadSurfaceFromResourceA(LPDIRECT3DSURFACE9 pDestSurface,
                                            CONST PALETTEENTRY *pDestPalette,
                                            CONST RECT *pDestRect,
                                            HMODULE hSrcModule,
                                            LPCSTR pResource,
                                            CONST RECT *pSrcRect,
                                            DWORD dwFilter,
                                            D3DCOLOR Colorkey,
                                            D3DXIMAGE_INFO *pSrcInfo)
{
    HRSRC hResInfo;

    TRACE("(%p, %p, %p, %p, %s, %p, %u, %#x, %p): relay\n", pDestSurface, pDestPalette, pDestRect, hSrcModule,
        debugstr_a(pResource), pSrcRect, dwFilter, Colorkey, pSrcInfo);

    if( !pDestSurface ) return D3DERR_INVALIDCALL;

    hResInfo = FindResourceA(hSrcModule, pResource, (LPCSTR)RT_RCDATA);
    if(hResInfo) {
        LPVOID pBuffer;
        HRESULT hr;
        DWORD dwSize;

        hr = load_resource_into_memory(hSrcModule, hResInfo, &pBuffer, &dwSize);
        if(FAILED(hr)) return D3DXERR_INVALIDDATA;
        return D3DXLoadSurfaceFromFileInMemory(pDestSurface, pDestPalette, pDestRect, pBuffer, dwSize, pSrcRect, dwFilter, Colorkey, pSrcInfo);
    }

    hResInfo = FindResourceA(hSrcModule, pResource, (LPCSTR)RT_BITMAP);
    if(hResInfo) {
        FIXME("Implement loading bitmaps from resource type RT_BITMAP\n");
        return E_NOTIMPL;
    }
    return D3DXERR_INVALIDDATA;
}

HRESULT WINAPI D3DXLoadSurfaceFromResourceW(LPDIRECT3DSURFACE9 pDestSurface,
                                            CONST PALETTEENTRY *pDestPalette,
                                            CONST RECT *pDestRect,
                                            HMODULE hSrcModule,
                                            LPCWSTR pResource,
                                            CONST RECT *pSrcRect,
                                            DWORD dwFilter,
                                            D3DCOLOR Colorkey,
                                            D3DXIMAGE_INFO *pSrcInfo)
{
    HRSRC hResInfo;

    TRACE("(%p, %p, %p, %p, %s, %p, %u, %#x, %p): relay\n", pDestSurface, pDestPalette, pDestRect, hSrcModule,
        debugstr_w(pResource), pSrcRect, dwFilter, Colorkey, pSrcInfo);

    if( !pDestSurface ) return D3DERR_INVALIDCALL;

    hResInfo = FindResourceW(hSrcModule, pResource, (LPCWSTR)RT_RCDATA);
    if(hResInfo) {
        LPVOID pBuffer;
        HRESULT hr;
        DWORD dwSize;

        hr = load_resource_into_memory(hSrcModule, hResInfo, &pBuffer, &dwSize);
        if(FAILED(hr)) return D3DXERR_INVALIDDATA;
        return D3DXLoadSurfaceFromFileInMemory(pDestSurface, pDestPalette, pDestRect, pBuffer, dwSize, pSrcRect, dwFilter, Colorkey, pSrcInfo);
    }

    hResInfo = FindResourceW(hSrcModule, pResource, (LPCWSTR)RT_BITMAP);
    if(hResInfo) {
        FIXME("Implement loading bitmaps from resource type RT_BITMAP\n");
        return E_NOTIMPL;
    }
    return D3DXERR_INVALIDDATA;
}


/************************************************************
 * copy_simple_data
 *
 * Copies the source buffer to the destination buffer, performing
 * any necessary format conversion and color keying.
 * Works only for ARGB formats with 1 - 4 bytes per pixel.
 */
static void copy_simple_data(CONST BYTE *src,  UINT  srcpitch, POINT  srcsize, CONST PixelFormatDesc  *srcformat,
                             CONST BYTE *dest, UINT destpitch, POINT destsize, CONST PixelFormatDesc *destformat,
                             DWORD dwFilter)
{
    DWORD srcshift[4], destshift[4];
    DWORD srcmask[4], destmask[4];
    BOOL process_channel[4];
    DWORD channels[4];
    DWORD channelmask = 0;

    UINT minwidth, minheight;
    BYTE *srcptr, *destptr;
    UINT i, x, y;

    ZeroMemory(channels, sizeof(channels));
    ZeroMemory(process_channel, sizeof(process_channel));

    for(i = 0;i < 4;i++) {
        /* srcshift is used to extract the _relevant_ components */
        srcshift[i]  =  srcformat->shift[i] + max( srcformat->bits[i] - destformat->bits[i], 0);

        /* destshift is used to move the components to the correct position */
        destshift[i] = destformat->shift[i] + max(destformat->bits[i] -  srcformat->bits[i], 0);

        srcmask[i]  = ((1 <<  srcformat->bits[i]) - 1) <<  srcformat->shift[i];
        destmask[i] = ((1 << destformat->bits[i]) - 1) << destformat->shift[i];

        /* channelmask specifies bits which aren't used in the source format but in the destination one */
        if(destformat->bits[i]) {
            if(srcformat->bits[i]) process_channel[i] = TRUE;
            else channelmask |= destmask[i];
        }
    }

    minwidth  = (srcsize.x < destsize.x) ? srcsize.x : destsize.x;
    minheight = (srcsize.y < destsize.y) ? srcsize.y : destsize.y;

    for(y = 0;y < minheight;y++) {
        srcptr  = (BYTE*)( src + y *  srcpitch);
        destptr = (BYTE*)(dest + y * destpitch);
        for(x = 0;x < minwidth;x++) {
            /* extract source color components */
            if(srcformat->type == FORMAT_ARGB) {
                const DWORD col = *(DWORD*)srcptr;
                for(i = 0;i < 4;i++)
                    if(process_channel[i])
                        channels[i] = (col & srcmask[i]) >> srcshift[i];
            }

            /* recombine the components */
            if(destformat->type == FORMAT_ARGB) {
                DWORD* const pixel = (DWORD*)destptr;
                *pixel = 0;

                for(i = 0;i < 4;i++) {
                    if(process_channel[i]) {
                        /* necessary to make sure that e.g. an X4R4G4B4 white maps to an R8G8B8 white instead of 0xf0f0f0 */
                        signed int shift;
                        for(shift = destshift[i]; shift > destformat->shift[i]; shift -= srcformat->bits[i]) *pixel |= channels[i] << shift;
                        *pixel |= (channels[i] >> (destformat->shift[i] - shift)) << destformat->shift[i];
                    }
                }
                *pixel |= channelmask;   /* new channels are set to their maximal value */
            }
            srcptr  +=  srcformat->bytes_per_pixel;
            destptr += destformat->bytes_per_pixel;
        }
    }
}

/************************************************************
 * D3DXLoadSurfaceFromMemory
 *
 * Loads data from a given memory chunk into a surface,
 * applying any of the specified filters.
 *
 * PARAMS
 *   pDestSurface [I] pointer to the surface
 *   pDestPalette [I] palette to use
 *   pDestRect    [I] to be filled area of the surface
 *   pSrcMemory   [I] pointer to the source data
 *   SrcFormat    [I] format of the source pixel data
 *   SrcPitch     [I] number of bytes in a row
 *   pSrcPalette  [I] palette used in the source image
 *   pSrcRect     [I] area of the source data to load
 *   dwFilter     [I] filter to apply on stretching
 *   Colorkey     [I] colorkey
 *
 * RETURNS
 *   Success: D3D_OK, if we successfully load the pixel data into our surface or
 *                    if pSrcMemory is NULL but the other parameters are valid
 *   Failure: D3DERR_INVALIDCALL, if pDestSurface, SrcPitch or pSrcRect are NULL or
 *                                if SrcFormat is an invalid format (other than D3DFMT_UNKNOWN)
 *            D3DXERR_INVALIDDATA, if we fail to lock pDestSurface
 *            E_FAIL, if SrcFormat is D3DFMT_UNKNOWN or the dimensions of pSrcRect are invalid
 *
 * NOTES
 *   pSrcRect specifies the dimensions of the source data;
 *   negative values for pSrcRect are allowed as we're only looking at the width and height anyway.
 *
 */
HRESULT WINAPI D3DXLoadSurfaceFromMemory(LPDIRECT3DSURFACE9 pDestSurface,
                                         CONST PALETTEENTRY *pDestPalette,
                                         CONST RECT *pDestRect,
                                         LPCVOID pSrcMemory,
                                         D3DFORMAT SrcFormat,
                                         UINT SrcPitch,
                                         CONST PALETTEENTRY *pSrcPalette,
                                         CONST RECT *pSrcRect,
                                         DWORD dwFilter,
                                         D3DCOLOR Colorkey)
{
    CONST PixelFormatDesc *srcformatdesc, *destformatdesc;
    D3DSURFACE_DESC surfdesc;
    D3DLOCKED_RECT lockrect;
    POINT srcsize, destsize;
    HRESULT hr;

    TRACE("(%p, %p, %p, %p, %x, %u, %p, %p %u, %#x)\n", pDestSurface, pDestPalette, pDestRect, pSrcMemory,
        SrcFormat, SrcPitch, pSrcPalette, pSrcRect, dwFilter, Colorkey);

    if( !pDestSurface || !pSrcMemory || !pSrcRect ) return D3DERR_INVALIDCALL;
    if(SrcFormat == D3DFMT_UNKNOWN || pSrcRect->left >= pSrcRect->right || pSrcRect->top >= pSrcRect->bottom) return E_FAIL;

    if(dwFilter != D3DX_FILTER_NONE) return E_NOTIMPL;

    IDirect3DSurface9_GetDesc(pDestSurface, &surfdesc);

    srcformatdesc = get_format_info(SrcFormat);
    destformatdesc = get_format_info(surfdesc.Format);
    if( srcformatdesc->type == FORMAT_UNKNOWN ||  srcformatdesc->bytes_per_pixel > 4) return E_NOTIMPL;
    if(destformatdesc->type == FORMAT_UNKNOWN || destformatdesc->bytes_per_pixel > 4) return E_NOTIMPL;

    srcsize.x = pSrcRect->right - pSrcRect->left;
    srcsize.y = pSrcRect->bottom - pSrcRect->top;
    if( !pDestRect ) {
        destsize.x = surfdesc.Width;
        destsize.y = surfdesc.Height;
    } else {
        destsize.x = pDestRect->right - pDestRect->left;
        destsize.y = pDestRect->bottom - pDestRect->top;
    }

    hr = IDirect3DSurface9_LockRect(pDestSurface, &lockrect, pDestRect, 0);
    if(FAILED(hr)) return D3DXERR_INVALIDDATA;

    copy_simple_data((CONST BYTE*)pSrcMemory, SrcPitch, srcsize, srcformatdesc,
                     (CONST BYTE*)lockrect.pBits, lockrect.Pitch, destsize, destformatdesc,
                     dwFilter);

    IDirect3DSurface9_UnlockRect(pDestSurface);
    return D3D_OK;
}

/************************************************************
 * D3DXLoadSurfaceFromSurface
 *
 * Copies the contents from one surface to another, performing any required
 * format conversion, resizing or filtering.
 *
 * PARAMS
 *   pDestSurface [I] pointer to the destination surface
 *   pDestPalette [I] palette to use
 *   pDestRect    [I] to be filled area of the surface
 *   pSrcSurface  [I] pointer to the source surface
 *   pSrcPalette  [I] palette used for the source surface
 *   pSrcRect     [I] area of the source data to load
 *   dwFilter     [I] filter to apply on resizing
 *   Colorkey     [I] any ARGB value or 0 to disable color-keying
 *
 * RETURNS
 *   Success: D3D_OK
 *   Failure: D3DERR_INVALIDCALL, if pDestSurface or pSrcSurface are NULL
 *            D3DXERR_INVALIDDATA, if one of the surfaces is not lockable
 *
 */
HRESULT WINAPI D3DXLoadSurfaceFromSurface(LPDIRECT3DSURFACE9 pDestSurface,
                                          CONST PALETTEENTRY *pDestPalette,
                                          CONST RECT *pDestRect,
                                          LPDIRECT3DSURFACE9 pSrcSurface,
                                          CONST PALETTEENTRY *pSrcPalette,
                                          CONST RECT *pSrcRect,
                                          DWORD dwFilter,
                                          D3DCOLOR Colorkey)
{
    RECT rect;
    D3DLOCKED_RECT lock;
    D3DSURFACE_DESC SrcDesc;
    HRESULT hr;

    TRACE("(%p, %p, %p, %p, %p, %p, %u, %#x): relay\n", pDestSurface, pDestPalette, pDestRect,
        pSrcSurface, pSrcPalette, pSrcRect, dwFilter, Colorkey);

    if( !pDestSurface || !pSrcSurface ) return D3DERR_INVALIDCALL;

    IDirect3DSurface9_GetDesc(pSrcSurface, &SrcDesc);

    if( !pSrcRect ) SetRect(&rect, 0, 0, SrcDesc.Width, SrcDesc.Height);
    else rect = *pSrcRect;

    hr = IDirect3DSurface9_LockRect(pSrcSurface, &lock, NULL, D3DLOCK_READONLY);
    if(FAILED(hr)) return D3DXERR_INVALIDDATA;

    hr = D3DXLoadSurfaceFromMemory(pDestSurface, pDestPalette, pDestRect,
                                   lock.pBits, SrcDesc.Format, lock.Pitch,
                                   pSrcPalette, &rect, dwFilter, Colorkey);

    IDirect3DSurface9_UnlockRect(pSrcSurface);
    return hr;
}
