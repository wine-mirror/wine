/*
 * Copyright 2018 Alistair Leslie-Hughes
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

#define COBJMACROS

#include "mfplat_private.h"
#include "rtworkq.h"

#include "d3d11.h"
#include "initguid.h"
#include "d3d9.h"
#include "evr.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

#define ALIGN_SIZE(size, alignment) (((size) + (alignment)) & ~((alignment)))

typedef void (*p_copy_image_func)(BYTE *dest, LONG dest_stride, const BYTE *src, LONG src_stride, DWORD width, DWORD lines);

struct buffer
{
    IMFMediaBuffer IMFMediaBuffer_iface;
    IMF2DBuffer2 IMF2DBuffer2_iface;
    IMFDXGIBuffer IMFDXGIBuffer_iface;
    IMFGetService IMFGetService_iface;
    LONG refcount;

    BYTE *data;
    DWORD max_length;
    DWORD current_length;

    struct
    {
        BYTE *linear_buffer;
        unsigned int plane_size;

        BYTE *scanline0;
        unsigned int width;
        unsigned int height;
        int pitch;
        unsigned int locks;
        p_copy_image_func copy_image;
    } _2d;
    struct
    {
        IDirect3DSurface9 *surface;
        D3DLOCKED_RECT rect;
    } d3d9_surface;
    struct
    {
        ID3D11Texture2D *texture;
        unsigned int sub_resource_idx;
        ID3D11Texture2D *rb_texture;
        D3D11_MAPPED_SUBRESOURCE map_desc;
        struct attributes attributes;
    } dxgi_surface;

    CRITICAL_SECTION cs;
};

static void copy_image(const struct buffer *buffer, BYTE *dest, LONG dest_stride, const BYTE *src,
        LONG src_stride, DWORD width, DWORD lines)
{
    MFCopyImage(dest, dest_stride, src, src_stride, width, lines);

    if (buffer->_2d.copy_image)
    {
        dest += dest_stride * lines;
        src += src_stride * lines;
        buffer->_2d.copy_image(dest, dest_stride, src, src_stride, width, lines);
    }
}

static void copy_image_nv12(BYTE *dest, LONG dest_stride, const BYTE *src, LONG src_stride, DWORD width, DWORD lines)
{
    MFCopyImage(dest, dest_stride, src, src_stride, width, lines / 2);
}

static void copy_image_imc1(BYTE *dest, LONG dest_stride, const BYTE *src, LONG src_stride, DWORD width, DWORD lines)
{
    MFCopyImage(dest, dest_stride, src, src_stride, width / 2, lines);
}

static void copy_image_imc2(BYTE *dest, LONG dest_stride, const BYTE *src, LONG src_stride, DWORD width, DWORD lines)
{
    MFCopyImage(dest, dest_stride, src, src_stride, width / 2, lines / 2);
    MFCopyImage(dest + dest_stride / 2, dest_stride, src + src_stride / 2, src_stride, width / 2, lines / 2);
}

static inline struct buffer *impl_from_IMFMediaBuffer(IMFMediaBuffer *iface)
{
    return CONTAINING_RECORD(iface, struct buffer, IMFMediaBuffer_iface);
}

static struct buffer *impl_from_IMF2DBuffer2(IMF2DBuffer2 *iface)
{
    return CONTAINING_RECORD(iface, struct buffer, IMF2DBuffer2_iface);
}

static struct buffer *impl_from_IMFGetService(IMFGetService *iface)
{
    return CONTAINING_RECORD(iface, struct buffer, IMFGetService_iface);
}

static struct buffer *impl_from_IMFDXGIBuffer(IMFDXGIBuffer *iface)
{
    return CONTAINING_RECORD(iface, struct buffer, IMFDXGIBuffer_iface);
}

static HRESULT WINAPI memory_buffer_QueryInterface(IMFMediaBuffer *iface, REFIID riid, void **out)
{
    struct buffer *buffer = impl_from_IMFMediaBuffer(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFMediaBuffer) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = &buffer->IMFMediaBuffer_iface;
        IMFMediaBuffer_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI memory_buffer_AddRef(IMFMediaBuffer *iface)
{
    struct buffer *buffer = impl_from_IMFMediaBuffer(iface);
    ULONG refcount = InterlockedIncrement(&buffer->refcount);

    TRACE("%p, refcount %u.\n", buffer, refcount);

    return refcount;
}

static ULONG WINAPI memory_buffer_Release(IMFMediaBuffer *iface)
{
    struct buffer *buffer = impl_from_IMFMediaBuffer(iface);
    ULONG refcount = InterlockedDecrement(&buffer->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        if (buffer->d3d9_surface.surface)
            IDirect3DSurface9_Release(buffer->d3d9_surface.surface);
        if (buffer->dxgi_surface.texture)
        {
            ID3D11Texture2D_Release(buffer->dxgi_surface.texture);
            if (buffer->dxgi_surface.rb_texture)
                ID3D11Texture2D_Release(buffer->dxgi_surface.rb_texture);
            clear_attributes_object(&buffer->dxgi_surface.attributes);
        }
        DeleteCriticalSection(&buffer->cs);
        free(buffer->_2d.linear_buffer);
        free(buffer->data);
        free(buffer);
    }

    return refcount;
}

static HRESULT WINAPI memory_buffer_Lock(IMFMediaBuffer *iface, BYTE **data, DWORD *max_length, DWORD *current_length)
{
    struct buffer *buffer = impl_from_IMFMediaBuffer(iface);

    TRACE("%p, %p %p, %p.\n", iface, data, max_length, current_length);

    if (!data)
        return E_INVALIDARG;

    *data = buffer->data;
    if (max_length)
        *max_length = buffer->max_length;
    if (current_length)
        *current_length = buffer->current_length;

    return S_OK;
}

static HRESULT WINAPI memory_buffer_Unlock(IMFMediaBuffer *iface)
{
    TRACE("%p.\n", iface);

    return S_OK;
}

static HRESULT WINAPI memory_buffer_GetCurrentLength(IMFMediaBuffer *iface, DWORD *current_length)
{
    struct buffer *buffer = impl_from_IMFMediaBuffer(iface);

    TRACE("%p.\n", iface);

    if (!current_length)
        return E_INVALIDARG;

    *current_length = buffer->current_length;

    return S_OK;
}

static HRESULT WINAPI memory_buffer_SetCurrentLength(IMFMediaBuffer *iface, DWORD current_length)
{
    struct buffer *buffer = impl_from_IMFMediaBuffer(iface);

    TRACE("%p, %u.\n", iface, current_length);

    if (current_length > buffer->max_length)
        return E_INVALIDARG;

    buffer->current_length = current_length;

    return S_OK;
}

static HRESULT WINAPI memory_buffer_GetMaxLength(IMFMediaBuffer *iface, DWORD *max_length)
{
    struct buffer *buffer = impl_from_IMFMediaBuffer(iface);

    TRACE("%p, %p.\n", iface, max_length);

    if (!max_length)
        return E_INVALIDARG;

    *max_length = buffer->max_length;

    return S_OK;
}

static const IMFMediaBufferVtbl memory_1d_buffer_vtbl =
{
    memory_buffer_QueryInterface,
    memory_buffer_AddRef,
    memory_buffer_Release,
    memory_buffer_Lock,
    memory_buffer_Unlock,
    memory_buffer_GetCurrentLength,
    memory_buffer_SetCurrentLength,
    memory_buffer_GetMaxLength,
};

static HRESULT WINAPI memory_1d_2d_buffer_QueryInterface(IMFMediaBuffer *iface, REFIID riid, void **out)
{
    struct buffer *buffer = impl_from_IMFMediaBuffer(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFMediaBuffer) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = &buffer->IMFMediaBuffer_iface;
    }
    else if (IsEqualIID(riid, &IID_IMF2DBuffer2) ||
            IsEqualIID(riid, &IID_IMF2DBuffer))
    {
        *out = &buffer->IMF2DBuffer2_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFGetService))
    {
        *out = &buffer->IMFGetService_iface;
    }
    else
    {
        WARN("Unsupported interface %s.\n", debugstr_guid(riid));
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*out);
    return S_OK;
}

static HRESULT WINAPI memory_1d_2d_buffer_Lock(IMFMediaBuffer *iface, BYTE **data, DWORD *max_length, DWORD *current_length)
{
    struct buffer *buffer = impl_from_IMFMediaBuffer(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p, %p, %p.\n", iface, data, max_length, current_length);

    if (!data)
        return E_POINTER;

    /* Allocate linear buffer and return it as a copy of current content. Maximum and current length are
       unrelated to 2D buffer maximum allocate length, or maintained current length. */

    EnterCriticalSection(&buffer->cs);

    if (!buffer->_2d.linear_buffer && buffer->_2d.locks)
        hr = MF_E_INVALIDREQUEST;
    else if (!buffer->_2d.linear_buffer)
    {
        if (!(buffer->_2d.linear_buffer = malloc(ALIGN_SIZE(buffer->_2d.plane_size, MF_64_BYTE_ALIGNMENT))))
            hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        ++buffer->_2d.locks;
        *data = buffer->_2d.linear_buffer;
        if (max_length)
            *max_length = buffer->_2d.plane_size;
        if (current_length)
            *current_length = buffer->_2d.plane_size;
    }

    LeaveCriticalSection(&buffer->cs);

    return hr;
}

static HRESULT WINAPI memory_1d_2d_buffer_Unlock(IMFMediaBuffer *iface)
{
    struct buffer *buffer = impl_from_IMFMediaBuffer(iface);

    TRACE("%p.\n", iface);

    EnterCriticalSection(&buffer->cs);

    if (buffer->_2d.linear_buffer && !--buffer->_2d.locks)
    {
        copy_image(buffer, buffer->data, buffer->_2d.pitch, buffer->_2d.linear_buffer, buffer->_2d.width,
                buffer->_2d.width, buffer->_2d.height);

        free(buffer->_2d.linear_buffer);
        buffer->_2d.linear_buffer = NULL;
    }

    LeaveCriticalSection(&buffer->cs);

    return S_OK;
}

static const IMFMediaBufferVtbl memory_1d_2d_buffer_vtbl =
{
    memory_1d_2d_buffer_QueryInterface,
    memory_buffer_AddRef,
    memory_buffer_Release,
    memory_1d_2d_buffer_Lock,
    memory_1d_2d_buffer_Unlock,
    memory_buffer_GetCurrentLength,
    memory_buffer_SetCurrentLength,
    memory_buffer_GetMaxLength,
};

static HRESULT WINAPI d3d9_surface_buffer_Lock(IMFMediaBuffer *iface, BYTE **data, DWORD *max_length, DWORD *current_length)
{
    struct buffer *buffer = impl_from_IMFMediaBuffer(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p, %p, %p.\n", iface, data, max_length, current_length);

    if (!data)
        return E_POINTER;

    EnterCriticalSection(&buffer->cs);

    if (!buffer->_2d.linear_buffer && buffer->_2d.locks)
        hr = MF_E_INVALIDREQUEST;
    else if (!buffer->_2d.linear_buffer)
    {
        D3DLOCKED_RECT rect;

        if (!(buffer->_2d.linear_buffer = malloc(ALIGN_SIZE(buffer->_2d.plane_size, MF_64_BYTE_ALIGNMENT))))
            hr = E_OUTOFMEMORY;

        if (SUCCEEDED(hr))
        {
            hr = IDirect3DSurface9_LockRect(buffer->d3d9_surface.surface, &rect, NULL, 0);
            if (SUCCEEDED(hr))
            {
                copy_image(buffer, buffer->_2d.linear_buffer, buffer->_2d.width, rect.pBits, rect.Pitch,
                        buffer->_2d.width, buffer->_2d.height);
                IDirect3DSurface9_UnlockRect(buffer->d3d9_surface.surface);
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        ++buffer->_2d.locks;
        *data = buffer->_2d.linear_buffer;
        if (max_length)
            *max_length = buffer->_2d.plane_size;
        if (current_length)
            *current_length = buffer->_2d.plane_size;
    }

    LeaveCriticalSection(&buffer->cs);

    return hr;
}

static HRESULT WINAPI d3d9_surface_buffer_Unlock(IMFMediaBuffer *iface)
{
    struct buffer *buffer = impl_from_IMFMediaBuffer(iface);
    HRESULT hr = S_OK;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&buffer->cs);

    if (!buffer->_2d.linear_buffer)
        hr = HRESULT_FROM_WIN32(ERROR_WAS_UNLOCKED);
    else if (!--buffer->_2d.locks)
    {
        D3DLOCKED_RECT rect;

        if (SUCCEEDED(hr = IDirect3DSurface9_LockRect(buffer->d3d9_surface.surface, &rect, NULL, 0)))
        {
            copy_image(buffer, rect.pBits, rect.Pitch, buffer->_2d.linear_buffer, buffer->_2d.width,
                    buffer->_2d.width, buffer->_2d.height);
            IDirect3DSurface9_UnlockRect(buffer->d3d9_surface.surface);
        }

        free(buffer->_2d.linear_buffer);
        buffer->_2d.linear_buffer = NULL;
    }

    LeaveCriticalSection(&buffer->cs);

    return hr;
}

static HRESULT WINAPI d3d9_surface_buffer_SetCurrentLength(IMFMediaBuffer *iface, DWORD current_length)
{
    struct buffer *buffer = impl_from_IMFMediaBuffer(iface);

    TRACE("%p, %u.\n", iface, current_length);

    buffer->current_length = current_length;

    return S_OK;
}

static const IMFMediaBufferVtbl d3d9_surface_1d_buffer_vtbl =
{
    memory_1d_2d_buffer_QueryInterface,
    memory_buffer_AddRef,
    memory_buffer_Release,
    d3d9_surface_buffer_Lock,
    d3d9_surface_buffer_Unlock,
    memory_buffer_GetCurrentLength,
    d3d9_surface_buffer_SetCurrentLength,
    memory_buffer_GetMaxLength,
};

static HRESULT WINAPI memory_2d_buffer_QueryInterface(IMF2DBuffer2 *iface, REFIID riid, void **obj)
{
    struct buffer *buffer = impl_from_IMF2DBuffer2(iface);
    return IMFMediaBuffer_QueryInterface(&buffer->IMFMediaBuffer_iface, riid, obj);
}

static ULONG WINAPI memory_2d_buffer_AddRef(IMF2DBuffer2 *iface)
{
    struct buffer *buffer = impl_from_IMF2DBuffer2(iface);
    return IMFMediaBuffer_AddRef(&buffer->IMFMediaBuffer_iface);
}

static ULONG WINAPI memory_2d_buffer_Release(IMF2DBuffer2 *iface)
{
    struct buffer *buffer = impl_from_IMF2DBuffer2(iface);
    return IMFMediaBuffer_Release(&buffer->IMFMediaBuffer_iface);
}

static HRESULT memory_2d_buffer_lock(struct buffer *buffer, BYTE **scanline0, LONG *pitch,
        BYTE **buffer_start, DWORD *buffer_length)
{
    HRESULT hr = S_OK;

    if (buffer->_2d.linear_buffer)
        hr = MF_E_UNEXPECTED;
    else
    {
        ++buffer->_2d.locks;
        *scanline0 = buffer->_2d.scanline0;
        *pitch = buffer->_2d.pitch;
        if (buffer_start)
            *buffer_start = buffer->data;
        if (buffer_length)
            *buffer_length = buffer->max_length;
    }

    return hr;
}

static HRESULT WINAPI memory_2d_buffer_Lock2D(IMF2DBuffer2 *iface, BYTE **scanline0, LONG *pitch)
{
    struct buffer *buffer = impl_from_IMF2DBuffer2(iface);
    HRESULT hr;

    TRACE("%p, %p, %p.\n", iface, scanline0, pitch);

    if (!scanline0 || !pitch)
        return E_POINTER;

    EnterCriticalSection(&buffer->cs);

    hr = memory_2d_buffer_lock(buffer, scanline0, pitch, NULL, NULL);

    LeaveCriticalSection(&buffer->cs);

    return hr;
}

static HRESULT WINAPI memory_2d_buffer_Unlock2D(IMF2DBuffer2 *iface)
{
    struct buffer *buffer = impl_from_IMF2DBuffer2(iface);
    HRESULT hr = S_OK;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&buffer->cs);

    if (!buffer->_2d.linear_buffer)
    {
        if (buffer->_2d.locks)
            --buffer->_2d.locks;
        else
            hr = HRESULT_FROM_WIN32(ERROR_WAS_UNLOCKED);
    }

    LeaveCriticalSection(&buffer->cs);

    return hr;
}

static HRESULT WINAPI memory_2d_buffer_GetScanline0AndPitch(IMF2DBuffer2 *iface, BYTE **scanline0, LONG *pitch)
{
    struct buffer *buffer = impl_from_IMF2DBuffer2(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p, %p.\n", iface, scanline0, pitch);

    if (!scanline0 || !pitch)
        return E_POINTER;

    EnterCriticalSection(&buffer->cs);

    if (buffer->_2d.linear_buffer || !buffer->_2d.locks)
        hr = HRESULT_FROM_WIN32(ERROR_WAS_UNLOCKED);
    else
    {
        *scanline0 = buffer->_2d.scanline0;
        *pitch = buffer->_2d.pitch;
    }

    LeaveCriticalSection(&buffer->cs);

    return hr;
}

static HRESULT WINAPI memory_2d_buffer_IsContiguousFormat(IMF2DBuffer2 *iface, BOOL *is_contiguous)
{
    TRACE("%p, %p.\n", iface, is_contiguous);

    if (!is_contiguous)
        return E_POINTER;

    *is_contiguous = FALSE;

    return S_OK;
}

static HRESULT WINAPI memory_2d_buffer_GetContiguousLength(IMF2DBuffer2 *iface, DWORD *length)
{
    struct buffer *buffer = impl_from_IMF2DBuffer2(iface);

    TRACE("%p, %p.\n", iface, length);

    if (!length)
        return E_POINTER;

    *length = buffer->_2d.plane_size;

    return S_OK;
}

static HRESULT WINAPI memory_2d_buffer_ContiguousCopyTo(IMF2DBuffer2 *iface, BYTE *dest_buffer, DWORD dest_length)
{
    FIXME("%p, %p, %u.\n", iface, dest_buffer, dest_length);

    return E_NOTIMPL;
}

static HRESULT WINAPI memory_2d_buffer_ContiguousCopyFrom(IMF2DBuffer2 *iface, const BYTE *src_buffer, DWORD src_length)
{
    FIXME("%p, %p, %u.\n", iface, src_buffer, src_length);

    return E_NOTIMPL;
}

static HRESULT WINAPI memory_2d_buffer_Lock2DSize(IMF2DBuffer2 *iface, MF2DBuffer_LockFlags flags, BYTE **scanline0,
        LONG *pitch, BYTE **buffer_start, DWORD *buffer_length)
{
    struct buffer *buffer = impl_from_IMF2DBuffer2(iface);
    HRESULT hr;

    TRACE("%p, %#x, %p, %p, %p, %p.\n", iface, flags, scanline0, pitch, buffer_start, buffer_length);

    if (!scanline0 || !pitch || !buffer_start || !buffer_length)
        return E_POINTER;

    EnterCriticalSection(&buffer->cs);

    hr = memory_2d_buffer_lock(buffer, scanline0, pitch, buffer_start, buffer_length);

    LeaveCriticalSection(&buffer->cs);

    return hr;
}

static HRESULT WINAPI memory_2d_buffer_Copy2DTo(IMF2DBuffer2 *iface, IMF2DBuffer2 *dest_buffer)
{
    FIXME("%p, %p.\n", iface, dest_buffer);

    return E_NOTIMPL;
}

static const IMF2DBuffer2Vtbl memory_2d_buffer_vtbl =
{
    memory_2d_buffer_QueryInterface,
    memory_2d_buffer_AddRef,
    memory_2d_buffer_Release,
    memory_2d_buffer_Lock2D,
    memory_2d_buffer_Unlock2D,
    memory_2d_buffer_GetScanline0AndPitch,
    memory_2d_buffer_IsContiguousFormat,
    memory_2d_buffer_GetContiguousLength,
    memory_2d_buffer_ContiguousCopyTo,
    memory_2d_buffer_ContiguousCopyFrom,
    memory_2d_buffer_Lock2DSize,
    memory_2d_buffer_Copy2DTo,
};

static HRESULT WINAPI d3d9_surface_buffer_Lock2D(IMF2DBuffer2 *iface, BYTE **scanline0, LONG *pitch)
{
    struct buffer *buffer = impl_from_IMF2DBuffer2(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p, %p.\n", iface, scanline0, pitch);

    if (!scanline0 || !pitch)
        return E_POINTER;

    EnterCriticalSection(&buffer->cs);

    if (buffer->_2d.linear_buffer)
        hr = MF_E_UNEXPECTED;
    else if (!buffer->_2d.locks++)
    {
        hr = IDirect3DSurface9_LockRect(buffer->d3d9_surface.surface, &buffer->d3d9_surface.rect, NULL, 0);
    }

    if (SUCCEEDED(hr))
    {
        *scanline0 = buffer->d3d9_surface.rect.pBits;
        *pitch = buffer->d3d9_surface.rect.Pitch;
    }

    LeaveCriticalSection(&buffer->cs);

    return hr;
}

static HRESULT WINAPI d3d9_surface_buffer_Unlock2D(IMF2DBuffer2 *iface)
{
    struct buffer *buffer = impl_from_IMF2DBuffer2(iface);
    HRESULT hr = S_OK;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&buffer->cs);

    if (buffer->_2d.locks)
    {
        if (!--buffer->_2d.locks)
        {
            IDirect3DSurface9_UnlockRect(buffer->d3d9_surface.surface);
            memset(&buffer->d3d9_surface.rect, 0, sizeof(buffer->d3d9_surface.rect));
        }
    }
    else
        hr = HRESULT_FROM_WIN32(ERROR_WAS_UNLOCKED);

    LeaveCriticalSection(&buffer->cs);

    return hr;
}

static HRESULT WINAPI d3d9_surface_buffer_GetScanline0AndPitch(IMF2DBuffer2 *iface, BYTE **scanline0, LONG *pitch)
{
    struct buffer *buffer = impl_from_IMF2DBuffer2(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p, %p.\n", iface, scanline0, pitch);

    if (!scanline0 || !pitch)
        return E_POINTER;

    EnterCriticalSection(&buffer->cs);

    if (!buffer->_2d.locks)
    {
        *scanline0 = NULL;
        *pitch = 0;
        hr = HRESULT_FROM_WIN32(ERROR_WAS_UNLOCKED);
    }
    else
    {
        *scanline0 = buffer->d3d9_surface.rect.pBits;
        *pitch = buffer->d3d9_surface.rect.Pitch;
    }

    LeaveCriticalSection(&buffer->cs);

    return hr;
}

static HRESULT WINAPI d3d9_surface_buffer_Lock2DSize(IMF2DBuffer2 *iface, MF2DBuffer_LockFlags flags, BYTE **scanline0,
        LONG *pitch, BYTE **buffer_start, DWORD *buffer_length)
{
    struct buffer *buffer = impl_from_IMF2DBuffer2(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %#x, %p, %p, %p, %p.\n", iface, flags, scanline0, pitch, buffer_start, buffer_length);

    if (!scanline0 || !pitch || !buffer_start || !buffer_length)
        return E_POINTER;

    EnterCriticalSection(&buffer->cs);

    if (buffer->_2d.linear_buffer)
        hr = MF_E_UNEXPECTED;
    else if (!buffer->_2d.locks++)
    {
        hr = IDirect3DSurface9_LockRect(buffer->d3d9_surface.surface, &buffer->d3d9_surface.rect, NULL, 0);
    }

    if (SUCCEEDED(hr))
    {
        *scanline0 = buffer->d3d9_surface.rect.pBits;
        *pitch = buffer->d3d9_surface.rect.Pitch;
        if (buffer_start)
            *buffer_start = *scanline0;
        if (buffer_length)
            *buffer_length = buffer->d3d9_surface.rect.Pitch * buffer->_2d.height;
    }

    LeaveCriticalSection(&buffer->cs);

    return hr;
}

static const IMF2DBuffer2Vtbl d3d9_surface_buffer_vtbl =
{
    memory_2d_buffer_QueryInterface,
    memory_2d_buffer_AddRef,
    memory_2d_buffer_Release,
    d3d9_surface_buffer_Lock2D,
    d3d9_surface_buffer_Unlock2D,
    d3d9_surface_buffer_GetScanline0AndPitch,
    memory_2d_buffer_IsContiguousFormat,
    memory_2d_buffer_GetContiguousLength,
    memory_2d_buffer_ContiguousCopyTo,
    memory_2d_buffer_ContiguousCopyFrom,
    d3d9_surface_buffer_Lock2DSize,
    memory_2d_buffer_Copy2DTo,
};

static HRESULT WINAPI memory_2d_buffer_gs_QueryInterface(IMFGetService *iface, REFIID riid, void **obj)
{
    struct buffer *buffer = impl_from_IMFGetService(iface);
    return IMFMediaBuffer_QueryInterface(&buffer->IMFMediaBuffer_iface, riid, obj);
}

static ULONG WINAPI memory_2d_buffer_gs_AddRef(IMFGetService *iface)
{
    struct buffer *buffer = impl_from_IMFGetService(iface);
    return IMFMediaBuffer_AddRef(&buffer->IMFMediaBuffer_iface);
}

static ULONG WINAPI memory_2d_buffer_gs_Release(IMFGetService *iface)
{
    struct buffer *buffer = impl_from_IMFGetService(iface);
    return IMFMediaBuffer_Release(&buffer->IMFMediaBuffer_iface);
}

static HRESULT WINAPI memory_2d_buffer_gs_GetService(IMFGetService *iface, REFGUID service, REFIID riid, void **obj)
{
    TRACE("%p, %s, %s, %p.\n", iface, debugstr_guid(service), debugstr_guid(riid), obj);

    return E_NOTIMPL;
}

static const IMFGetServiceVtbl memory_2d_buffer_gs_vtbl =
{
    memory_2d_buffer_gs_QueryInterface,
    memory_2d_buffer_gs_AddRef,
    memory_2d_buffer_gs_Release,
    memory_2d_buffer_gs_GetService,
};

static HRESULT WINAPI d3d9_surface_buffer_gs_GetService(IMFGetService *iface, REFGUID service, REFIID riid, void **obj)
{
    struct buffer *buffer = impl_from_IMFGetService(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_guid(service), debugstr_guid(riid), obj);

    if (IsEqualGUID(service, &MR_BUFFER_SERVICE))
    {
        return IDirect3DSurface9_QueryInterface(buffer->d3d9_surface.surface, riid, obj);
    }

    return E_NOTIMPL;
}

static const IMFGetServiceVtbl d3d9_surface_buffer_gs_vtbl =
{
    memory_2d_buffer_gs_QueryInterface,
    memory_2d_buffer_gs_AddRef,
    memory_2d_buffer_gs_Release,
    d3d9_surface_buffer_gs_GetService,
};

static HRESULT WINAPI dxgi_1d_2d_buffer_QueryInterface(IMFMediaBuffer *iface, REFIID riid, void **out)
{
    struct buffer *buffer = impl_from_IMFMediaBuffer(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFMediaBuffer) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = &buffer->IMFMediaBuffer_iface;
    }
    else if (IsEqualIID(riid, &IID_IMF2DBuffer2) ||
            IsEqualIID(riid, &IID_IMF2DBuffer))
    {
        *out = &buffer->IMF2DBuffer2_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFDXGIBuffer))
    {
        *out = &buffer->IMFDXGIBuffer_iface;
    }
    else
    {
        WARN("Unsupported interface %s.\n", debugstr_guid(riid));
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*out);
    return S_OK;
}

static HRESULT dxgi_surface_buffer_create_readback_texture(struct buffer *buffer)
{
    D3D11_TEXTURE2D_DESC texture_desc;
    ID3D11Device *device;
    HRESULT hr;

    if (buffer->dxgi_surface.rb_texture)
        return S_OK;

    ID3D11Texture2D_GetDevice(buffer->dxgi_surface.texture, &device);

    ID3D11Texture2D_GetDesc(buffer->dxgi_surface.texture, &texture_desc);
    texture_desc.Usage = D3D11_USAGE_STAGING;
    texture_desc.BindFlags = 0;
    texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
    texture_desc.MiscFlags = 0;
    texture_desc.MipLevels = 1;
    if (FAILED(hr = ID3D11Device_CreateTexture2D(device, &texture_desc, NULL, &buffer->dxgi_surface.rb_texture)))
        WARN("Failed to create readback texture, hr %#x.\n", hr);

    ID3D11Device_Release(device);

    return hr;
}

static HRESULT dxgi_surface_buffer_map(struct buffer *buffer)
{
    ID3D11DeviceContext *immediate_context;
    ID3D11Device *device;
    HRESULT hr;

    if (FAILED(hr = dxgi_surface_buffer_create_readback_texture(buffer)))
        return hr;

    ID3D11Texture2D_GetDevice(buffer->dxgi_surface.texture, &device);
    ID3D11Device_GetImmediateContext(device, &immediate_context);
    ID3D11DeviceContext_CopySubresourceRegion(immediate_context, (ID3D11Resource *)buffer->dxgi_surface.rb_texture,
            0, 0, 0, 0, (ID3D11Resource *)buffer->dxgi_surface.texture, buffer->dxgi_surface.sub_resource_idx, NULL);

    memset(&buffer->dxgi_surface.map_desc, 0, sizeof(buffer->dxgi_surface.map_desc));
    if (FAILED(hr = ID3D11DeviceContext_Map(immediate_context, (ID3D11Resource *)buffer->dxgi_surface.rb_texture,
            0, D3D11_MAP_READ_WRITE, 0, &buffer->dxgi_surface.map_desc)))
    {
        WARN("Failed to map readback texture, hr %#x.\n", hr);
    }

    ID3D11DeviceContext_Release(immediate_context);
    ID3D11Device_Release(device);

    return hr;
}

static void dxgi_surface_buffer_unmap(struct buffer *buffer)
{
    ID3D11DeviceContext *immediate_context;
    ID3D11Device *device;

    ID3D11Texture2D_GetDevice(buffer->dxgi_surface.texture, &device);
    ID3D11Device_GetImmediateContext(device, &immediate_context);
    ID3D11DeviceContext_Unmap(immediate_context, (ID3D11Resource *)buffer->dxgi_surface.rb_texture, 0);
    memset(&buffer->dxgi_surface.map_desc, 0, sizeof(buffer->dxgi_surface.map_desc));

    ID3D11DeviceContext_CopySubresourceRegion(immediate_context, (ID3D11Resource *)buffer->dxgi_surface.texture,
            buffer->dxgi_surface.sub_resource_idx, 0, 0, 0, (ID3D11Resource *)buffer->dxgi_surface.rb_texture, 0, NULL);

    ID3D11DeviceContext_Release(immediate_context);
    ID3D11Device_Release(device);
}

static HRESULT WINAPI dxgi_surface_buffer_Lock(IMFMediaBuffer *iface, BYTE **data, DWORD *max_length,
        DWORD *current_length)
{
    struct buffer *buffer = impl_from_IMFMediaBuffer(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p, %p, %p.\n", iface, data, max_length, current_length);

    if (!data)
        return E_POINTER;

    EnterCriticalSection(&buffer->cs);

    if (!buffer->_2d.linear_buffer && buffer->_2d.locks)
        hr = MF_E_INVALIDREQUEST;
    else if (!buffer->_2d.linear_buffer)
    {
        if (!(buffer->_2d.linear_buffer = malloc(ALIGN_SIZE(buffer->_2d.plane_size, MF_64_BYTE_ALIGNMENT))))
            hr = E_OUTOFMEMORY;

        if (SUCCEEDED(hr))
        {
            hr = dxgi_surface_buffer_map(buffer);
            if (SUCCEEDED(hr))
            {
                copy_image(buffer, buffer->_2d.linear_buffer, buffer->_2d.width, buffer->dxgi_surface.map_desc.pData,
                        buffer->dxgi_surface.map_desc.RowPitch, buffer->_2d.width, buffer->_2d.height);
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        ++buffer->_2d.locks;
        *data = buffer->_2d.linear_buffer;
        if (max_length)
            *max_length = buffer->_2d.plane_size;
        if (current_length)
            *current_length = buffer->_2d.plane_size;
    }

    LeaveCriticalSection(&buffer->cs);

    return hr;
}

static HRESULT WINAPI dxgi_surface_buffer_Unlock(IMFMediaBuffer *iface)
{
    struct buffer *buffer = impl_from_IMFMediaBuffer(iface);
    HRESULT hr = S_OK;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&buffer->cs);

    if (!buffer->_2d.linear_buffer)
        hr = HRESULT_FROM_WIN32(ERROR_WAS_UNLOCKED);
    else if (!--buffer->_2d.locks)
    {
        copy_image(buffer, buffer->dxgi_surface.map_desc.pData, buffer->dxgi_surface.map_desc.RowPitch,
                buffer->_2d.linear_buffer, buffer->_2d.width, buffer->_2d.width, buffer->_2d.height);
        dxgi_surface_buffer_unmap(buffer);

        free(buffer->_2d.linear_buffer);
        buffer->_2d.linear_buffer = NULL;
    }

    LeaveCriticalSection(&buffer->cs);

    return hr;
}

static HRESULT WINAPI dxgi_surface_buffer_SetCurrentLength(IMFMediaBuffer *iface, DWORD current_length)
{
    struct buffer *buffer = impl_from_IMFMediaBuffer(iface);

    TRACE("%p, %u.\n", iface, current_length);

    buffer->current_length = current_length;

    return S_OK;
}

static HRESULT WINAPI dxgi_surface_buffer_Lock2D(IMF2DBuffer2 *iface, BYTE **scanline0, LONG *pitch)
{
    struct buffer *buffer = impl_from_IMF2DBuffer2(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p, %p.\n", iface, scanline0, pitch);

    if (!scanline0 || !pitch)
        return E_POINTER;

    EnterCriticalSection(&buffer->cs);

    if (buffer->_2d.linear_buffer)
        hr = MF_E_UNEXPECTED;
    else if (!buffer->_2d.locks++)
        hr = dxgi_surface_buffer_map(buffer);

    if (SUCCEEDED(hr))
    {
        *scanline0 = buffer->dxgi_surface.map_desc.pData;
        *pitch = buffer->dxgi_surface.map_desc.RowPitch;
    }

    LeaveCriticalSection(&buffer->cs);

    return hr;
}

static HRESULT WINAPI dxgi_surface_buffer_Unlock2D(IMF2DBuffer2 *iface)
{
    struct buffer *buffer = impl_from_IMF2DBuffer2(iface);
    HRESULT hr = S_OK;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&buffer->cs);

    if (buffer->_2d.locks)
    {
        if (!--buffer->_2d.locks)
            dxgi_surface_buffer_unmap(buffer);
    }
    else
        hr = HRESULT_FROM_WIN32(ERROR_WAS_UNLOCKED);

    LeaveCriticalSection(&buffer->cs);

    return hr;
}

static HRESULT WINAPI dxgi_surface_buffer_GetScanline0AndPitch(IMF2DBuffer2 *iface, BYTE **scanline0, LONG *pitch)
{
    struct buffer *buffer = impl_from_IMF2DBuffer2(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p, %p.\n", iface, scanline0, pitch);

    if (!scanline0 || !pitch)
        return E_POINTER;

    EnterCriticalSection(&buffer->cs);

    if (!buffer->_2d.locks)
    {
        *scanline0 = NULL;
        *pitch = 0;
        hr = HRESULT_FROM_WIN32(ERROR_WAS_UNLOCKED);
    }
    else
    {
        *scanline0 = buffer->dxgi_surface.map_desc.pData;
        *pitch = buffer->dxgi_surface.map_desc.RowPitch;
    }

    LeaveCriticalSection(&buffer->cs);

    return hr;
}

static HRESULT WINAPI dxgi_surface_buffer_Lock2DSize(IMF2DBuffer2 *iface, MF2DBuffer_LockFlags flags,
        BYTE **scanline0, LONG *pitch, BYTE **buffer_start, DWORD *buffer_length)
{
    struct buffer *buffer = impl_from_IMF2DBuffer2(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %#x, %p, %p, %p, %p.\n", iface, flags, scanline0, pitch, buffer_start, buffer_length);

    if (!scanline0 || !pitch || !buffer_start || !buffer_length)
        return E_POINTER;

    EnterCriticalSection(&buffer->cs);

    if (buffer->_2d.linear_buffer)
        hr = MF_E_UNEXPECTED;
    else if (!buffer->_2d.locks++)
        hr = dxgi_surface_buffer_map(buffer);

    if (SUCCEEDED(hr))
    {
        *scanline0 = buffer->dxgi_surface.map_desc.pData;
        *pitch = buffer->dxgi_surface.map_desc.RowPitch;
        if (buffer_start)
            *buffer_start = *scanline0;
        if (buffer_length)
            *buffer_length = buffer->dxgi_surface.map_desc.RowPitch * buffer->_2d.height;
    }

    LeaveCriticalSection(&buffer->cs);

    return hr;
}

static HRESULT WINAPI dxgi_buffer_QueryInterface(IMFDXGIBuffer *iface, REFIID riid, void **obj)
{
    struct buffer *buffer = impl_from_IMFDXGIBuffer(iface);
    return IMFMediaBuffer_QueryInterface(&buffer->IMFMediaBuffer_iface, riid, obj);
}

static ULONG WINAPI dxgi_buffer_AddRef(IMFDXGIBuffer *iface)
{
    struct buffer *buffer = impl_from_IMFDXGIBuffer(iface);
    return IMFMediaBuffer_AddRef(&buffer->IMFMediaBuffer_iface);
}

static ULONG WINAPI dxgi_buffer_Release(IMFDXGIBuffer *iface)
{
    struct buffer *buffer = impl_from_IMFDXGIBuffer(iface);
    return IMFMediaBuffer_Release(&buffer->IMFMediaBuffer_iface);
}

static HRESULT WINAPI dxgi_buffer_GetResource(IMFDXGIBuffer *iface, REFIID riid, void **obj)
{
    struct buffer *buffer = impl_from_IMFDXGIBuffer(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    return ID3D11Texture2D_QueryInterface(buffer->dxgi_surface.texture, riid, obj);
}

static HRESULT WINAPI dxgi_buffer_GetSubresourceIndex(IMFDXGIBuffer *iface, UINT *index)
{
    struct buffer *buffer = impl_from_IMFDXGIBuffer(iface);

    TRACE("%p, %p.\n", iface, index);

    if (!index)
        return E_POINTER;

    *index = buffer->dxgi_surface.sub_resource_idx;

    return S_OK;
}

static HRESULT WINAPI dxgi_buffer_GetUnknown(IMFDXGIBuffer *iface, REFIID guid, REFIID riid, void **object)
{
    struct buffer *buffer = impl_from_IMFDXGIBuffer(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_guid(guid), debugstr_guid(riid), object);

    if (attributes_GetUnknown(&buffer->dxgi_surface.attributes, guid, riid, object) == MF_E_ATTRIBUTENOTFOUND)
        return MF_E_NOT_FOUND;

    return S_OK;
}

static HRESULT WINAPI dxgi_buffer_SetUnknown(IMFDXGIBuffer *iface, REFIID guid, IUnknown *data)
{
    struct buffer *buffer = impl_from_IMFDXGIBuffer(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(guid), data);

    EnterCriticalSection(&buffer->dxgi_surface.attributes.cs);
    if (data)
    {
        if (SUCCEEDED(attributes_GetItem(&buffer->dxgi_surface.attributes, guid, NULL)))
            hr = HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS);
        else
            hr = attributes_SetUnknown(&buffer->dxgi_surface.attributes, guid, data);
    }
    else
    {
        attributes_DeleteItem(&buffer->dxgi_surface.attributes, guid);
    }
    LeaveCriticalSection(&buffer->dxgi_surface.attributes.cs);

    return hr;
}

static const IMFMediaBufferVtbl dxgi_surface_1d_buffer_vtbl =
{
    dxgi_1d_2d_buffer_QueryInterface,
    memory_buffer_AddRef,
    memory_buffer_Release,
    dxgi_surface_buffer_Lock,
    dxgi_surface_buffer_Unlock,
    memory_buffer_GetCurrentLength,
    dxgi_surface_buffer_SetCurrentLength,
    memory_buffer_GetMaxLength,
};

static const IMF2DBuffer2Vtbl dxgi_surface_buffer_vtbl =
{
    memory_2d_buffer_QueryInterface,
    memory_2d_buffer_AddRef,
    memory_2d_buffer_Release,
    dxgi_surface_buffer_Lock2D,
    dxgi_surface_buffer_Unlock2D,
    dxgi_surface_buffer_GetScanline0AndPitch,
    memory_2d_buffer_IsContiguousFormat,
    memory_2d_buffer_GetContiguousLength,
    memory_2d_buffer_ContiguousCopyTo,
    memory_2d_buffer_ContiguousCopyFrom,
    dxgi_surface_buffer_Lock2DSize,
    memory_2d_buffer_Copy2DTo,
};

static const IMFDXGIBufferVtbl dxgi_buffer_vtbl =
{
    dxgi_buffer_QueryInterface,
    dxgi_buffer_AddRef,
    dxgi_buffer_Release,
    dxgi_buffer_GetResource,
    dxgi_buffer_GetSubresourceIndex,
    dxgi_buffer_GetUnknown,
    dxgi_buffer_SetUnknown,
};

static HRESULT memory_buffer_init(struct buffer *buffer, DWORD max_length, DWORD alignment,
        const IMFMediaBufferVtbl *vtbl)
{
    if (!(buffer->data = calloc(1, ALIGN_SIZE(max_length, alignment))))
        return E_OUTOFMEMORY;

    buffer->IMFMediaBuffer_iface.lpVtbl = vtbl;
    buffer->refcount = 1;
    buffer->max_length = max_length;
    buffer->current_length = 0;
    InitializeCriticalSection(&buffer->cs);

    return S_OK;
}

static HRESULT create_1d_buffer(DWORD max_length, DWORD alignment, IMFMediaBuffer **buffer)
{
    struct buffer *object;
    HRESULT hr;

    if (!buffer)
        return E_POINTER;

    *buffer = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    hr = memory_buffer_init(object, max_length, alignment, &memory_1d_buffer_vtbl);
    if (FAILED(hr))
    {
        free(object);
        return hr;
    }

    *buffer = &object->IMFMediaBuffer_iface;

    return S_OK;
}

static p_copy_image_func get_2d_buffer_copy_func(DWORD fourcc)
{
    if (fourcc == MAKEFOURCC('N','V','1','2'))
        return copy_image_nv12;
    if (fourcc == MAKEFOURCC('I','M','C','1') || fourcc == MAKEFOURCC('I','M','C','3'))
        return copy_image_imc1;
    if (fourcc == MAKEFOURCC('I','M','C','2') || fourcc == MAKEFOURCC('I','M','C','4'))
        return copy_image_imc2;
    return NULL;
}

static HRESULT create_2d_buffer(DWORD width, DWORD height, DWORD fourcc, BOOL bottom_up, IMFMediaBuffer **buffer)
{
    unsigned int stride, max_length, plane_size;
    struct buffer *object;
    unsigned int row_alignment;
    GUID subtype;
    BOOL is_yuv;
    HRESULT hr;
    int pitch;

    if (!buffer)
        return E_POINTER;

    *buffer = NULL;

    memcpy(&subtype, &MFVideoFormat_Base, sizeof(subtype));
    subtype.Data1 = fourcc;

    if (!(stride = mf_format_get_stride(&subtype, width, &is_yuv)))
        return MF_E_INVALIDMEDIATYPE;

    if (is_yuv && bottom_up)
        return MF_E_INVALIDMEDIATYPE;

    if (FAILED(hr = MFGetPlaneSize(fourcc, width, height, &plane_size)))
        return hr;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    switch (fourcc)
    {
        case MAKEFOURCC('I','M','C','1'):
        case MAKEFOURCC('I','M','C','2'):
        case MAKEFOURCC('I','M','C','3'):
        case MAKEFOURCC('I','M','C','4'):
        case MAKEFOURCC('Y','V','1','2'):
            row_alignment = MF_128_BYTE_ALIGNMENT;
            break;
        default:
            row_alignment = MF_64_BYTE_ALIGNMENT;
    }

    pitch = ALIGN_SIZE(stride, row_alignment);

    switch (fourcc)
    {
        case MAKEFOURCC('I','M','C','1'):
        case MAKEFOURCC('I','M','C','3'):
            max_length = pitch * height * 2;
            plane_size *= 2;
            break;
        case MAKEFOURCC('N','V','1','2'):
        case MAKEFOURCC('Y','V','1','2'):
        case MAKEFOURCC('I','M','C','2'):
        case MAKEFOURCC('I','M','C','4'):
            max_length = pitch * height * 3 / 2;
            break;
        default:
            max_length = pitch * height;
    }

    if (FAILED(hr = memory_buffer_init(object, max_length, MF_1_BYTE_ALIGNMENT, &memory_1d_2d_buffer_vtbl)))
    {
        free(object);
        return hr;
    }

    object->IMF2DBuffer2_iface.lpVtbl = &memory_2d_buffer_vtbl;
    object->IMFGetService_iface.lpVtbl = &memory_2d_buffer_gs_vtbl;
    object->_2d.plane_size = plane_size;
    object->_2d.width = stride;
    object->_2d.height = height;
    object->_2d.pitch = bottom_up ? -pitch : pitch;
    object->_2d.scanline0 = bottom_up ? object->data + pitch * (object->_2d.height - 1) : object->data;
    object->_2d.copy_image = get_2d_buffer_copy_func(fourcc);

    *buffer = &object->IMFMediaBuffer_iface;

    return S_OK;
}

static HRESULT create_d3d9_surface_buffer(IUnknown *surface, BOOL bottom_up, IMFMediaBuffer **buffer)
{
    struct buffer *object;
    D3DSURFACE_DESC desc;
    unsigned int stride;
    GUID subtype;
    BOOL is_yuv;

    IDirect3DSurface9_GetDesc((IDirect3DSurface9 *)surface, &desc);
    TRACE("format %#x, %u x %u.\n", desc.Format, desc.Width, desc.Height);

    memcpy(&subtype, &MFVideoFormat_Base, sizeof(subtype));
    subtype.Data1 = desc.Format;

    if (!(stride = mf_format_get_stride(&subtype, desc.Width, &is_yuv)))
        return MF_E_INVALIDMEDIATYPE;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFMediaBuffer_iface.lpVtbl = &d3d9_surface_1d_buffer_vtbl;
    object->IMF2DBuffer2_iface.lpVtbl = &d3d9_surface_buffer_vtbl;
    object->IMFGetService_iface.lpVtbl = &d3d9_surface_buffer_gs_vtbl;
    object->refcount = 1;
    InitializeCriticalSection(&object->cs);
    object->d3d9_surface.surface = (IDirect3DSurface9 *)surface;
    IUnknown_AddRef(surface);

    MFGetPlaneSize(desc.Format, desc.Width, desc.Height, &object->_2d.plane_size);
    object->_2d.width = stride;
    object->_2d.height = desc.Height;
    object->max_length = object->_2d.plane_size;
    object->_2d.copy_image = get_2d_buffer_copy_func(desc.Format);

    *buffer = &object->IMFMediaBuffer_iface;

    return S_OK;
}

static HRESULT create_dxgi_surface_buffer(IUnknown *surface, unsigned int sub_resource_idx,
        BOOL bottom_up, IMFMediaBuffer **buffer)
{
    struct buffer *object;
    D3D11_TEXTURE2D_DESC desc;
    ID3D11Texture2D *texture;
    unsigned int stride;
    D3DFORMAT format;
    GUID subtype;
    BOOL is_yuv;
    HRESULT hr;

    if (FAILED(hr = IUnknown_QueryInterface(surface, &IID_ID3D11Texture2D, (void **)&texture)))
    {
        WARN("Failed to get texture interface, hr %#x.\n", hr);
        return hr;
    }

    ID3D11Texture2D_GetDesc(texture, &desc);
    TRACE("format %#x, %u x %u.\n", desc.Format, desc.Width, desc.Height);

    memcpy(&subtype, &MFVideoFormat_Base, sizeof(subtype));
    subtype.Data1 = format = MFMapDXGIFormatToDX9Format(desc.Format);

    if (!(stride = mf_format_get_stride(&subtype, desc.Width, &is_yuv)))
    {
        ID3D11Texture2D_Release(texture);
        return MF_E_INVALIDMEDIATYPE;
    }

    if (!(object = calloc(1, sizeof(*object))))
    {
        ID3D11Texture2D_Release(texture);
        return E_OUTOFMEMORY;
    }

    object->IMFMediaBuffer_iface.lpVtbl = &dxgi_surface_1d_buffer_vtbl;
    object->IMF2DBuffer2_iface.lpVtbl = &dxgi_surface_buffer_vtbl;
    object->IMFDXGIBuffer_iface.lpVtbl = &dxgi_buffer_vtbl;
    object->refcount = 1;
    InitializeCriticalSection(&object->cs);
    object->dxgi_surface.texture = texture;
    object->dxgi_surface.sub_resource_idx = sub_resource_idx;

    MFGetPlaneSize(format, desc.Width, desc.Height, &object->_2d.plane_size);
    object->_2d.width = stride;
    object->_2d.height = desc.Height;
    object->max_length = object->_2d.plane_size;
    object->_2d.copy_image = get_2d_buffer_copy_func(format);

    if (FAILED(hr = init_attributes_object(&object->dxgi_surface.attributes, 0)))
    {
        IMFMediaBuffer_Release(&object->IMFMediaBuffer_iface);
        return hr;
    }

    *buffer = &object->IMFMediaBuffer_iface;

    return S_OK;
}

/***********************************************************************
 *      MFCreateMemoryBuffer (mfplat.@)
 */
HRESULT WINAPI MFCreateMemoryBuffer(DWORD max_length, IMFMediaBuffer **buffer)
{
    TRACE("%u, %p.\n", max_length, buffer);

    return create_1d_buffer(max_length, MF_1_BYTE_ALIGNMENT, buffer);
}

/***********************************************************************
 *      MFCreateAlignedMemoryBuffer (mfplat.@)
 */
HRESULT WINAPI MFCreateAlignedMemoryBuffer(DWORD max_length, DWORD alignment, IMFMediaBuffer **buffer)
{
    TRACE("%u, %u, %p.\n", max_length, alignment, buffer);

    return create_1d_buffer(max_length, alignment, buffer);
}

/***********************************************************************
 *      MFCreate2DMediaBuffer (mfplat.@)
 */
HRESULT WINAPI MFCreate2DMediaBuffer(DWORD width, DWORD height, DWORD fourcc, BOOL bottom_up, IMFMediaBuffer **buffer)
{
    TRACE("%u, %u, %s, %d, %p.\n", width, height, debugstr_fourcc(fourcc), bottom_up, buffer);

    return create_2d_buffer(width, height, fourcc, bottom_up, buffer);
}

/***********************************************************************
 *      MFCreateDXSurfaceBuffer (mfplat.@)
 */
HRESULT WINAPI MFCreateDXSurfaceBuffer(REFIID riid, IUnknown *surface, BOOL bottom_up, IMFMediaBuffer **buffer)
{
    TRACE("%s, %p, %d, %p.\n", debugstr_guid(riid), surface, bottom_up, buffer);

    if (!IsEqualIID(riid, &IID_IDirect3DSurface9))
        return E_INVALIDARG;

    return create_d3d9_surface_buffer(surface, bottom_up, buffer);
}

/***********************************************************************
 *      MFCreateDXGISurfaceBuffer (mfplat.@)
 */
HRESULT WINAPI MFCreateDXGISurfaceBuffer(REFIID riid, IUnknown *surface, UINT subresource, BOOL bottom_up,
        IMFMediaBuffer **buffer)
{
    TRACE("%s, %p, %u, %d, %p.\n", debugstr_guid(riid), surface, subresource, bottom_up, buffer);

    if (!IsEqualIID(riid, &IID_ID3D11Texture2D))
        return E_INVALIDARG;

    return create_dxgi_surface_buffer(surface, subresource, bottom_up, buffer);
}

static unsigned int buffer_get_aligned_length(unsigned int length, unsigned int alignment)
{
    length = (length + alignment) / alignment;
    length *= alignment;

    return length;
}

HRESULT WINAPI MFCreateMediaBufferFromMediaType(IMFMediaType *media_type, LONGLONG duration, DWORD min_length,
        DWORD alignment, IMFMediaBuffer **buffer)
{
    UINT32 length = 0, block_alignment;
    LONGLONG avg_length;
    HRESULT hr;
    GUID major;

    TRACE("%p, %s, %u, %u, %p.\n", media_type, debugstr_time(duration), min_length, alignment, buffer);

    if (!media_type)
        return E_INVALIDARG;

    if (FAILED(hr = IMFMediaType_GetMajorType(media_type, &major)))
        return hr;

    if (IsEqualGUID(&major, &MFMediaType_Audio))
    {
        block_alignment = 0;
        if (FAILED(IMFMediaType_GetUINT32(media_type, &MF_MT_AUDIO_BLOCK_ALIGNMENT, &block_alignment)))
            WARN("Block alignment was not specified.\n");

        if (block_alignment)
        {
            avg_length = 0;

            if (duration)
            {
                length = 0;
                if (SUCCEEDED(IMFMediaType_GetUINT32(media_type, &MF_MT_AUDIO_AVG_BYTES_PER_SECOND, &length)))
                {
                    /* 100 ns -> 1 s */
                    avg_length = length * duration / (10 * 1000 * 1000);
                }
            }

            alignment = max(16, alignment);

            length = buffer_get_aligned_length(avg_length + 1, alignment);
            length = buffer_get_aligned_length(length, block_alignment);
        }
        else
            length = 0;

        length = max(length, min_length);

        return create_1d_buffer(length, MF_1_BYTE_ALIGNMENT, buffer);
    }
    else
        FIXME("Major type %s is not supported.\n", debugstr_guid(&major));

    return E_NOTIMPL;
}
