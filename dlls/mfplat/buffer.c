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

#include "initguid.h"
#include "d3d9.h"
#include "evr.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

#define ALIGN_SIZE(size, alignment) (((size) + (alignment)) & ~((alignment)))

struct memory_buffer
{
    IMFMediaBuffer IMFMediaBuffer_iface;
    IMF2DBuffer2 IMF2DBuffer2_iface;
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
    } _2d;
    struct
    {
        IDirect3DSurface9 *surface;
        D3DLOCKED_RECT rect;
    } d3d9_surface;

    CRITICAL_SECTION cs;
};

enum sample_prop_flags
{
    SAMPLE_PROP_HAS_DURATION  = 1 << 0,
    SAMPLE_PROP_HAS_TIMESTAMP = 1 << 1,
};

struct sample
{
    struct attributes attributes;
    IMFSample IMFSample_iface;
    IMFTrackedSample IMFTrackedSample_iface;

    IMFMediaBuffer **buffers;
    size_t buffer_count;
    size_t capacity;
    DWORD flags;
    DWORD prop_flags;
    LONGLONG duration;
    LONGLONG timestamp;

    /* Tracked sample functionality. */
    IRtwqAsyncResult *tracked_result;
    LONG tracked_refcount;
};

static inline struct memory_buffer *impl_from_IMFMediaBuffer(IMFMediaBuffer *iface)
{
    return CONTAINING_RECORD(iface, struct memory_buffer, IMFMediaBuffer_iface);
}

static struct memory_buffer *impl_from_IMF2DBuffer2(IMF2DBuffer2 *iface)
{
    return CONTAINING_RECORD(iface, struct memory_buffer, IMF2DBuffer2_iface);
}

static struct memory_buffer *impl_from_IMFGetService(IMFGetService *iface)
{
    return CONTAINING_RECORD(iface, struct memory_buffer, IMFGetService_iface);
}

static inline struct sample *impl_from_IMFSample(IMFSample *iface)
{
    return CONTAINING_RECORD(iface, struct sample, IMFSample_iface);
}

static struct sample *impl_from_IMFTrackedSample(IMFTrackedSample *iface)
{
    return CONTAINING_RECORD(iface, struct sample, IMFTrackedSample_iface);
}

static HRESULT WINAPI memory_buffer_QueryInterface(IMFMediaBuffer *iface, REFIID riid, void **out)
{
    struct memory_buffer *buffer = impl_from_IMFMediaBuffer(iface);

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
    struct memory_buffer *buffer = impl_from_IMFMediaBuffer(iface);
    ULONG refcount = InterlockedIncrement(&buffer->refcount);

    TRACE("%p, refcount %u.\n", buffer, refcount);

    return refcount;
}

static ULONG WINAPI memory_buffer_Release(IMFMediaBuffer *iface)
{
    struct memory_buffer *buffer = impl_from_IMFMediaBuffer(iface);
    ULONG refcount = InterlockedDecrement(&buffer->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        if (buffer->d3d9_surface.surface)
            IDirect3DSurface9_Release(buffer->d3d9_surface.surface);
        DeleteCriticalSection(&buffer->cs);
        heap_free(buffer->_2d.linear_buffer);
        heap_free(buffer->data);
        heap_free(buffer);
    }

    return refcount;
}

static HRESULT WINAPI memory_buffer_Lock(IMFMediaBuffer *iface, BYTE **data, DWORD *max_length, DWORD *current_length)
{
    struct memory_buffer *buffer = impl_from_IMFMediaBuffer(iface);

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
    struct memory_buffer *buffer = impl_from_IMFMediaBuffer(iface);

    TRACE("%p.\n", iface);

    if (!current_length)
        return E_INVALIDARG;

    *current_length = buffer->current_length;

    return S_OK;
}

static HRESULT WINAPI memory_buffer_SetCurrentLength(IMFMediaBuffer *iface, DWORD current_length)
{
    struct memory_buffer *buffer = impl_from_IMFMediaBuffer(iface);

    TRACE("%p, %u.\n", iface, current_length);

    if (current_length > buffer->max_length)
        return E_INVALIDARG;

    buffer->current_length = current_length;

    return S_OK;
}

static HRESULT WINAPI memory_buffer_GetMaxLength(IMFMediaBuffer *iface, DWORD *max_length)
{
    struct memory_buffer *buffer = impl_from_IMFMediaBuffer(iface);

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
    struct memory_buffer *buffer = impl_from_IMFMediaBuffer(iface);

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
    struct memory_buffer *buffer = impl_from_IMFMediaBuffer(iface);
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
        if (!(buffer->_2d.linear_buffer = heap_alloc(ALIGN_SIZE(buffer->_2d.plane_size, MF_64_BYTE_ALIGNMENT))))
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
    struct memory_buffer *buffer = impl_from_IMFMediaBuffer(iface);

    TRACE("%p.\n", iface);

    EnterCriticalSection(&buffer->cs);

    if (buffer->_2d.linear_buffer && !--buffer->_2d.locks)
    {
        MFCopyImage(buffer->data, buffer->_2d.pitch, buffer->_2d.linear_buffer, buffer->_2d.width,
                buffer->_2d.width, buffer->_2d.height);

        heap_free(buffer->_2d.linear_buffer);
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
    struct memory_buffer *buffer = impl_from_IMFMediaBuffer(iface);
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

        if (!(buffer->_2d.linear_buffer = heap_alloc(ALIGN_SIZE(buffer->_2d.plane_size, MF_64_BYTE_ALIGNMENT))))
            hr = E_OUTOFMEMORY;

        if (SUCCEEDED(hr))
        {
            hr = IDirect3DSurface9_LockRect(buffer->d3d9_surface.surface, &rect, NULL, 0);
            if (SUCCEEDED(hr))
            {
                MFCopyImage(buffer->_2d.linear_buffer, buffer->_2d.width, rect.pBits, rect.Pitch,
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
    struct memory_buffer *buffer = impl_from_IMFMediaBuffer(iface);
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
            MFCopyImage(rect.pBits, rect.Pitch, buffer->_2d.linear_buffer, buffer->_2d.width,
                    buffer->_2d.width, buffer->_2d.height);
            IDirect3DSurface9_UnlockRect(buffer->d3d9_surface.surface);
        }

        heap_free(buffer->_2d.linear_buffer);
        buffer->_2d.linear_buffer = NULL;
    }

    LeaveCriticalSection(&buffer->cs);

    return hr;
}

static const IMFMediaBufferVtbl d3d9_surface_1d_buffer_vtbl =
{
    memory_1d_2d_buffer_QueryInterface,
    memory_buffer_AddRef,
    memory_buffer_Release,
    d3d9_surface_buffer_Lock,
    d3d9_surface_buffer_Unlock,
    memory_buffer_GetCurrentLength,
    memory_buffer_SetCurrentLength,
    memory_buffer_GetMaxLength,
};

static HRESULT WINAPI memory_2d_buffer_QueryInterface(IMF2DBuffer2 *iface, REFIID riid, void **obj)
{
    struct memory_buffer *buffer = impl_from_IMF2DBuffer2(iface);
    return IMFMediaBuffer_QueryInterface(&buffer->IMFMediaBuffer_iface, riid, obj);
}

static ULONG WINAPI memory_2d_buffer_AddRef(IMF2DBuffer2 *iface)
{
    struct memory_buffer *buffer = impl_from_IMF2DBuffer2(iface);
    return IMFMediaBuffer_AddRef(&buffer->IMFMediaBuffer_iface);
}

static ULONG WINAPI memory_2d_buffer_Release(IMF2DBuffer2 *iface)
{
    struct memory_buffer *buffer = impl_from_IMF2DBuffer2(iface);
    return IMFMediaBuffer_Release(&buffer->IMFMediaBuffer_iface);
}

static HRESULT memory_2d_buffer_lock(struct memory_buffer *buffer, BYTE **scanline0, LONG *pitch,
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
    struct memory_buffer *buffer = impl_from_IMF2DBuffer2(iface);
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
    struct memory_buffer *buffer = impl_from_IMF2DBuffer2(iface);
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
    struct memory_buffer *buffer = impl_from_IMF2DBuffer2(iface);
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
    struct memory_buffer *buffer = impl_from_IMF2DBuffer2(iface);

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
    struct memory_buffer *buffer = impl_from_IMF2DBuffer2(iface);
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
    struct memory_buffer *buffer = impl_from_IMF2DBuffer2(iface);
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
    struct memory_buffer *buffer = impl_from_IMF2DBuffer2(iface);
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
    struct memory_buffer *buffer = impl_from_IMF2DBuffer2(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p, %p.\n", iface, scanline0, pitch);

    if (!scanline0 || !pitch)
        return E_POINTER;

    EnterCriticalSection(&buffer->cs);

    if (!buffer->_2d.locks)
        hr = HRESULT_FROM_WIN32(ERROR_WAS_UNLOCKED);
    else
    {
        *scanline0 = buffer->d3d9_surface.rect.pBits;
        *pitch = buffer->d3d9_surface.rect.Pitch;
    }

    LeaveCriticalSection(&buffer->cs);

    return hr;
}

static HRESULT WINAPI d3d9_surface_buffer_GetContiguousLength(IMF2DBuffer2 *iface, DWORD *length)
{
    FIXME("%p, %p.\n", iface, length);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3d9_surface_buffer_Lock2DSize(IMF2DBuffer2 *iface, MF2DBuffer_LockFlags flags, BYTE **scanline0,
        LONG *pitch, BYTE **buffer_start, DWORD *buffer_length)
{
    struct memory_buffer *buffer = impl_from_IMF2DBuffer2(iface);
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
    d3d9_surface_buffer_GetContiguousLength,
    memory_2d_buffer_ContiguousCopyTo,
    memory_2d_buffer_ContiguousCopyFrom,
    d3d9_surface_buffer_Lock2DSize,
    memory_2d_buffer_Copy2DTo,
};

static HRESULT WINAPI memory_2d_buffer_gs_QueryInterface(IMFGetService *iface, REFIID riid, void **obj)
{
    struct memory_buffer *buffer = impl_from_IMFGetService(iface);
    return IMFMediaBuffer_QueryInterface(&buffer->IMFMediaBuffer_iface, riid, obj);
}

static ULONG WINAPI memory_2d_buffer_gs_AddRef(IMFGetService *iface)
{
    struct memory_buffer *buffer = impl_from_IMFGetService(iface);
    return IMFMediaBuffer_AddRef(&buffer->IMFMediaBuffer_iface);
}

static ULONG WINAPI memory_2d_buffer_gs_Release(IMFGetService *iface)
{
    struct memory_buffer *buffer = impl_from_IMFGetService(iface);
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
    struct memory_buffer *buffer = impl_from_IMFGetService(iface);

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

static HRESULT memory_buffer_init(struct memory_buffer *buffer, DWORD max_length, DWORD alignment,
        const IMFMediaBufferVtbl *vtbl)
{
    buffer->data = heap_alloc_zero(ALIGN_SIZE(max_length, alignment));
    if (!buffer->data)
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
    struct memory_buffer *object;
    HRESULT hr;

    if (!buffer)
        return E_POINTER;

    *buffer = NULL;

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    hr = memory_buffer_init(object, max_length, alignment, &memory_1d_buffer_vtbl);
    if (FAILED(hr))
    {
        heap_free(object);
        return hr;
    }

    *buffer = &object->IMFMediaBuffer_iface;

    return S_OK;
}

static HRESULT create_2d_buffer(DWORD width, DWORD height, DWORD fourcc, BOOL bottom_up, IMFMediaBuffer **buffer)
{
    unsigned int stride, max_length, plane_size;
    struct memory_buffer *object;
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

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
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
            max_length = pitch * height * 3 / 2;
            break;
        default:
            max_length = pitch * height;
    }

    if (FAILED(hr = memory_buffer_init(object, max_length, MF_1_BYTE_ALIGNMENT, &memory_1d_2d_buffer_vtbl)))
    {
        heap_free(object);
        return hr;
    }

    object->IMF2DBuffer2_iface.lpVtbl = &memory_2d_buffer_vtbl;
    object->IMFGetService_iface.lpVtbl = &memory_2d_buffer_gs_vtbl;
    object->_2d.plane_size = plane_size;
    object->_2d.width = stride;
    object->_2d.height = height;
    object->_2d.pitch = bottom_up ? -pitch : pitch;
    object->_2d.scanline0 = bottom_up ? object->data + pitch * (object->_2d.height - 1) : object->data;

    *buffer = &object->IMFMediaBuffer_iface;

    return S_OK;
}

static HRESULT create_d3d9_surface_buffer(IUnknown *surface, BOOL bottom_up, IMFMediaBuffer **buffer)
{
    struct memory_buffer *object;
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

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
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

static HRESULT WINAPI sample_QueryInterface(IMFSample *iface, REFIID riid, void **out)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFSample) ||
            IsEqualIID(riid, &IID_IMFAttributes) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = &sample->IMFSample_iface;
    }
    else if (sample->IMFTrackedSample_iface.lpVtbl && IsEqualIID(riid, &IID_IMFTrackedSample))
    {
        *out = &sample->IMFTrackedSample_iface;
    }
    else
    {
        WARN("Unsupported %s.\n", debugstr_guid(riid));
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI sample_AddRef(IMFSample *iface)
{
    struct sample *sample = impl_from_IMFSample(iface);
    ULONG refcount = InterlockedIncrement(&sample->attributes.ref);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static void release_sample_object(struct sample *sample)
{
    size_t i;

    for (i = 0; i < sample->buffer_count; ++i)
        IMFMediaBuffer_Release(sample->buffers[i]);
    clear_attributes_object(&sample->attributes);
    heap_free(sample->buffers);
    heap_free(sample);
}

static ULONG WINAPI sample_Release(IMFSample *iface)
{
    struct sample *sample = impl_from_IMFSample(iface);
    ULONG refcount = InterlockedDecrement(&sample->attributes.ref);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
        release_sample_object(sample);

    return refcount;
}

static ULONG WINAPI sample_tracked_Release(IMFSample *iface)
{
    struct sample *sample = impl_from_IMFSample(iface);
    ULONG refcount;
    HRESULT hr;

    EnterCriticalSection(&sample->attributes.cs);
    refcount = InterlockedDecrement(&sample->attributes.ref);
    if (sample->tracked_result && sample->tracked_refcount == refcount)
    {
        /* Call could fail if queue system is not initialized, it's not critical. */
        if (FAILED(hr = RtwqInvokeCallback(sample->tracked_result)))
            WARN("Failed to invoke tracking callback, hr %#x.\n", hr);
        IRtwqAsyncResult_Release(sample->tracked_result);
        sample->tracked_result = NULL;
        sample->tracked_refcount = 0;
    }
    LeaveCriticalSection(&sample->attributes.cs);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
        release_sample_object(sample);

    return refcount;
}

static HRESULT WINAPI sample_GetItem(IMFSample *iface, REFGUID key, PROPVARIANT *value)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetItem(&sample->attributes, key, value);
}

static HRESULT WINAPI sample_GetItemType(IMFSample *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), type);

    return attributes_GetItemType(&sample->attributes, key, type);
}

static HRESULT WINAPI sample_CompareItem(IMFSample *iface, REFGUID key, REFPROPVARIANT value, BOOL *result)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_attr(key), debugstr_propvar(value), result);

    return attributes_CompareItem(&sample->attributes, key, value, result);
}

static HRESULT WINAPI sample_Compare(IMFSample *iface, IMFAttributes *theirs, MF_ATTRIBUTES_MATCH_TYPE type,
        BOOL *result)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %p, %d, %p.\n", iface, theirs, type, result);

    return attributes_Compare(&sample->attributes, theirs, type, result);
}

static HRESULT WINAPI sample_GetUINT32(IMFSample *iface, REFGUID key, UINT32 *value)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetUINT32(&sample->attributes, key, value);
}

static HRESULT WINAPI sample_GetUINT64(IMFSample *iface, REFGUID key, UINT64 *value)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetUINT64(&sample->attributes, key, value);
}

static HRESULT WINAPI sample_GetDouble(IMFSample *iface, REFGUID key, double *value)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetDouble(&sample->attributes, key, value);
}

static HRESULT WINAPI sample_GetGUID(IMFSample *iface, REFGUID key, GUID *value)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetGUID(&sample->attributes, key, value);
}

static HRESULT WINAPI sample_GetStringLength(IMFSample *iface, REFGUID key, UINT32 *length)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), length);

    return attributes_GetStringLength(&sample->attributes, key, length);
}

static HRESULT WINAPI sample_GetString(IMFSample *iface, REFGUID key, WCHAR *value, UINT32 size, UINT32 *length)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p, %u, %p.\n", iface, debugstr_attr(key), value, size, length);

    return attributes_GetString(&sample->attributes, key, value, size, length);
}

static HRESULT WINAPI sample_GetAllocatedString(IMFSample *iface, REFGUID key, WCHAR **value, UINT32 *length)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_attr(key), value, length);

    return attributes_GetAllocatedString(&sample->attributes, key, value, length);
}

static HRESULT WINAPI sample_GetBlobSize(IMFSample *iface, REFGUID key, UINT32 *size)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), size);

    return attributes_GetBlobSize(&sample->attributes, key, size);
}

static HRESULT WINAPI sample_GetBlob(IMFSample *iface, REFGUID key, UINT8 *buf, UINT32 bufsize, UINT32 *blobsize)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p, %u, %p.\n", iface, debugstr_attr(key), buf, bufsize, blobsize);

    return attributes_GetBlob(&sample->attributes, key, buf, bufsize, blobsize);
}

static HRESULT WINAPI sample_GetAllocatedBlob(IMFSample *iface, REFGUID key, UINT8 **buf, UINT32 *size)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_attr(key), buf, size);

    return attributes_GetAllocatedBlob(&sample->attributes, key, buf, size);
}

static HRESULT WINAPI sample_GetUnknown(IMFSample *iface, REFGUID key, REFIID riid, void **out)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_attr(key), debugstr_guid(riid), out);

    return attributes_GetUnknown(&sample->attributes, key, riid, out);
}

static HRESULT WINAPI sample_SetItem(IMFSample *iface, REFGUID key, REFPROPVARIANT value)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_propvar(value));

    return attributes_SetItem(&sample->attributes, key, value);
}

static HRESULT WINAPI sample_DeleteItem(IMFSample *iface, REFGUID key)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s.\n", iface, debugstr_attr(key));

    return attributes_DeleteItem(&sample->attributes, key);
}

static HRESULT WINAPI sample_DeleteAllItems(IMFSample *iface)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p.\n", iface);

    return attributes_DeleteAllItems(&sample->attributes);
}

static HRESULT WINAPI sample_SetUINT32(IMFSample *iface, REFGUID key, UINT32 value)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %u.\n", iface, debugstr_attr(key), value);

    return attributes_SetUINT32(&sample->attributes, key, value);
}

static HRESULT WINAPI sample_SetUINT64(IMFSample *iface, REFGUID key, UINT64 value)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), wine_dbgstr_longlong(value));

    return attributes_SetUINT64(&sample->attributes, key, value);
}

static HRESULT WINAPI sample_SetDouble(IMFSample *iface, REFGUID key, double value)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %f.\n", iface, debugstr_attr(key), value);

    return attributes_SetDouble(&sample->attributes, key, value);
}

static HRESULT WINAPI sample_SetGUID(IMFSample *iface, REFGUID key, REFGUID value)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_mf_guid(value));

    return attributes_SetGUID(&sample->attributes, key, value);
}

static HRESULT WINAPI sample_SetString(IMFSample *iface, REFGUID key, const WCHAR *value)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_w(value));

    return attributes_SetString(&sample->attributes, key, value);
}

static HRESULT WINAPI sample_SetBlob(IMFSample *iface, REFGUID key, const UINT8 *buf, UINT32 size)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p, %u.\n", iface, debugstr_attr(key), buf, size);

    return attributes_SetBlob(&sample->attributes, key, buf, size);
}

static HRESULT WINAPI sample_SetUnknown(IMFSample *iface, REFGUID key, IUnknown *unknown)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), unknown);

    return attributes_SetUnknown(&sample->attributes, key, unknown);
}

static HRESULT WINAPI sample_LockStore(IMFSample *iface)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p.\n", iface);

    return attributes_LockStore(&sample->attributes);
}

static HRESULT WINAPI sample_UnlockStore(IMFSample *iface)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p.\n", iface);

    return attributes_UnlockStore(&sample->attributes);
}

static HRESULT WINAPI sample_GetCount(IMFSample *iface, UINT32 *count)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %p.\n", iface, count);

    return attributes_GetCount(&sample->attributes, count);
}

static HRESULT WINAPI sample_GetItemByIndex(IMFSample *iface, UINT32 index, GUID *key, PROPVARIANT *value)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %u, %p, %p.\n", iface, index, key, value);

    return attributes_GetItemByIndex(&sample->attributes, index, key, value);
}

static HRESULT WINAPI sample_CopyAllItems(IMFSample *iface, IMFAttributes *dest)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %p.\n", iface, dest);

    return attributes_CopyAllItems(&sample->attributes, dest);
}

static HRESULT WINAPI sample_GetSampleFlags(IMFSample *iface, DWORD *flags)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %p.\n", iface, flags);

    EnterCriticalSection(&sample->attributes.cs);
    *flags = sample->flags;
    LeaveCriticalSection(&sample->attributes.cs);

    return S_OK;
}

static HRESULT WINAPI sample_SetSampleFlags(IMFSample *iface, DWORD flags)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %#x.\n", iface, flags);

    EnterCriticalSection(&sample->attributes.cs);
    sample->flags = flags;
    LeaveCriticalSection(&sample->attributes.cs);

    return S_OK;
}

static HRESULT WINAPI sample_GetSampleTime(IMFSample *iface, LONGLONG *timestamp)
{
    struct sample *sample = impl_from_IMFSample(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, timestamp);

    EnterCriticalSection(&sample->attributes.cs);
    if (sample->prop_flags & SAMPLE_PROP_HAS_TIMESTAMP)
        *timestamp = sample->timestamp;
    else
        hr = MF_E_NO_SAMPLE_TIMESTAMP;
    LeaveCriticalSection(&sample->attributes.cs);

    return hr;
}

static HRESULT WINAPI sample_SetSampleTime(IMFSample *iface, LONGLONG timestamp)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s.\n", iface, debugstr_time(timestamp));

    EnterCriticalSection(&sample->attributes.cs);
    sample->timestamp = timestamp;
    sample->prop_flags |= SAMPLE_PROP_HAS_TIMESTAMP;
    LeaveCriticalSection(&sample->attributes.cs);

    return S_OK;
}

static HRESULT WINAPI sample_GetSampleDuration(IMFSample *iface, LONGLONG *duration)
{
    struct sample *sample = impl_from_IMFSample(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, duration);

    EnterCriticalSection(&sample->attributes.cs);
    if (sample->prop_flags & SAMPLE_PROP_HAS_DURATION)
        *duration = sample->duration;
    else
        hr = MF_E_NO_SAMPLE_DURATION;
    LeaveCriticalSection(&sample->attributes.cs);

    return hr;
}

static HRESULT WINAPI sample_SetSampleDuration(IMFSample *iface, LONGLONG duration)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s.\n", iface, debugstr_time(duration));

    EnterCriticalSection(&sample->attributes.cs);
    sample->duration = duration;
    sample->prop_flags |= SAMPLE_PROP_HAS_DURATION;
    LeaveCriticalSection(&sample->attributes.cs);

    return S_OK;
}

static HRESULT WINAPI sample_GetBufferCount(IMFSample *iface, DWORD *count)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %p.\n", iface, count);

    if (!count)
        return E_INVALIDARG;

    EnterCriticalSection(&sample->attributes.cs);
    *count = sample->buffer_count;
    LeaveCriticalSection(&sample->attributes.cs);

    return S_OK;
}

static HRESULT WINAPI sample_GetBufferByIndex(IMFSample *iface, DWORD index, IMFMediaBuffer **buffer)
{
    struct sample *sample = impl_from_IMFSample(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %u, %p.\n", iface, index, buffer);

    EnterCriticalSection(&sample->attributes.cs);
    if (index < sample->buffer_count)
    {
        *buffer = sample->buffers[index];
        IMFMediaBuffer_AddRef(*buffer);
    }
    else
        hr = E_INVALIDARG;
    LeaveCriticalSection(&sample->attributes.cs);

    return hr;
}

static unsigned int sample_get_total_length(struct sample *sample)
{
    DWORD total_length = 0, length;
    size_t i;

    for (i = 0; i < sample->buffer_count; ++i)
    {
        length = 0;
        if (SUCCEEDED(IMFMediaBuffer_GetCurrentLength(sample->buffers[i], &length)))
            total_length += length;
    }

    return total_length;
}

static HRESULT sample_copy_to_buffer(struct sample *sample, IMFMediaBuffer *buffer)
{
    DWORD total_length, dst_length, dst_current_length, src_max_length, current_length;
    BYTE *src_ptr, *dst_ptr;
    BOOL locked;
    HRESULT hr;
    size_t i;

    total_length = sample_get_total_length(sample);
    dst_current_length = 0;

    dst_ptr = NULL;
    dst_length = current_length = 0;
    locked = SUCCEEDED(hr = IMFMediaBuffer_Lock(buffer, &dst_ptr, &dst_length, &current_length));
    if (locked)
    {
        if (dst_length < total_length)
            hr = MF_E_BUFFERTOOSMALL;
        else if (dst_ptr)
        {
            for (i = 0; i < sample->buffer_count && SUCCEEDED(hr); ++i)
            {
                src_ptr = NULL;
                src_max_length = current_length = 0;
                if (SUCCEEDED(hr = IMFMediaBuffer_Lock(sample->buffers[i], &src_ptr, &src_max_length, &current_length)))
                {
                    if (src_ptr)
                    {
                        if (current_length > dst_length)
                            hr = MF_E_BUFFERTOOSMALL;
                        else if (current_length)
                        {
                            memcpy(dst_ptr, src_ptr, current_length);
                            dst_length -= current_length;
                            dst_current_length += current_length;
                            dst_ptr += current_length;
                        }
                    }
                    IMFMediaBuffer_Unlock(sample->buffers[i]);
                }
            }
        }
    }

    if (FAILED(IMFMediaBuffer_SetCurrentLength(buffer, dst_current_length)))
        WARN("Failed to set buffer length.\n");

    if (locked)
        IMFMediaBuffer_Unlock(buffer);

    return hr;
}

static HRESULT WINAPI sample_ConvertToContiguousBuffer(IMFSample *iface, IMFMediaBuffer **buffer)
{
    struct sample *sample = impl_from_IMFSample(iface);
    unsigned int total_length, i;
    IMFMediaBuffer *dest_buffer;
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, buffer);

    EnterCriticalSection(&sample->attributes.cs);

    if (sample->buffer_count == 0)
        hr = E_UNEXPECTED;
    else if (sample->buffer_count > 1)
    {
        total_length = sample_get_total_length(sample);
        if (SUCCEEDED(hr = MFCreateMemoryBuffer(total_length, &dest_buffer)))
        {
            if (SUCCEEDED(hr = sample_copy_to_buffer(sample, dest_buffer)))
            {
                for (i = 0; i < sample->buffer_count; ++i)
                    IMFMediaBuffer_Release(sample->buffers[i]);

                sample->buffers[0] = dest_buffer;
                IMFMediaBuffer_AddRef(sample->buffers[0]);

                sample->buffer_count = 1;
            }
            IMFMediaBuffer_Release(dest_buffer);
        }
    }

    if (SUCCEEDED(hr) && buffer)
    {
        *buffer = sample->buffers[0];
        IMFMediaBuffer_AddRef(*buffer);
    }

    LeaveCriticalSection(&sample->attributes.cs);

    return hr;
}

static HRESULT WINAPI sample_AddBuffer(IMFSample *iface, IMFMediaBuffer *buffer)
{
    struct sample *sample = impl_from_IMFSample(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, buffer);

    EnterCriticalSection(&sample->attributes.cs);
    if (!mf_array_reserve((void **)&sample->buffers, &sample->capacity, sample->buffer_count + 1,
            sizeof(*sample->buffers)))
        hr = E_OUTOFMEMORY;
    else
    {
        sample->buffers[sample->buffer_count++] = buffer;
        IMFMediaBuffer_AddRef(buffer);
    }
    LeaveCriticalSection(&sample->attributes.cs);

    return hr;
}

static HRESULT WINAPI sample_RemoveBufferByIndex(IMFSample *iface, DWORD index)
{
    struct sample *sample = impl_from_IMFSample(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %u.\n", iface, index);

    EnterCriticalSection(&sample->attributes.cs);
    if (index < sample->buffer_count)
    {
        IMFMediaBuffer_Release(sample->buffers[index]);
        if (index < sample->buffer_count - 1)
        {
            memmove(&sample->buffers[index], &sample->buffers[index+1],
                    (sample->buffer_count - index - 1) * sizeof(*sample->buffers));
        }
        sample->buffer_count--;
    }
    else
        hr = E_INVALIDARG;
    LeaveCriticalSection(&sample->attributes.cs);

    return hr;
}

static HRESULT WINAPI sample_RemoveAllBuffers(IMFSample *iface)
{
    struct sample *sample = impl_from_IMFSample(iface);
    size_t i;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&sample->attributes.cs);
    for (i = 0; i < sample->buffer_count; ++i)
         IMFMediaBuffer_Release(sample->buffers[i]);
    sample->buffer_count = 0;
    LeaveCriticalSection(&sample->attributes.cs);

    return S_OK;
}

static HRESULT WINAPI sample_GetTotalLength(IMFSample *iface, DWORD *total_length)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %p.\n", iface, total_length);

    EnterCriticalSection(&sample->attributes.cs);
    *total_length = sample_get_total_length(sample);
    LeaveCriticalSection(&sample->attributes.cs);

    return S_OK;
}

static HRESULT WINAPI sample_CopyToBuffer(IMFSample *iface, IMFMediaBuffer *buffer)
{
    struct sample *sample = impl_from_IMFSample(iface);
    DWORD total_length, dst_length, dst_current_length, src_max_length, current_length;
    BYTE *src_ptr, *dst_ptr;
    BOOL locked;
    HRESULT hr;
    size_t i;

    TRACE("%p, %p.\n", iface, buffer);

    EnterCriticalSection(&sample->attributes.cs);

    total_length = sample_get_total_length(sample);
    dst_current_length = 0;

    dst_ptr = NULL;
    dst_length = current_length = 0;
    locked = SUCCEEDED(hr = IMFMediaBuffer_Lock(buffer, &dst_ptr, &dst_length, &current_length));
    if (locked)
    {
        if (dst_length < total_length)
            hr = MF_E_BUFFERTOOSMALL;
        else if (dst_ptr)
        {
            for (i = 0; i < sample->buffer_count && SUCCEEDED(hr); ++i)
            {
                src_ptr = NULL;
                src_max_length = current_length = 0;
                if (SUCCEEDED(hr = IMFMediaBuffer_Lock(sample->buffers[i], &src_ptr, &src_max_length, &current_length)))
                {
                    if (src_ptr)
                    {
                        if (current_length > dst_length)
                            hr = MF_E_BUFFERTOOSMALL;
                        else if (current_length)
                        {
                            memcpy(dst_ptr, src_ptr, current_length);
                            dst_length -= current_length;
                            dst_current_length += current_length;
                            dst_ptr += current_length;
                        }
                    }
                    IMFMediaBuffer_Unlock(sample->buffers[i]);
                }
            }
        }
    }

    IMFMediaBuffer_SetCurrentLength(buffer, dst_current_length);

    if (locked)
        IMFMediaBuffer_Unlock(buffer);

    LeaveCriticalSection(&sample->attributes.cs);

    return hr;
}

static const IMFSampleVtbl samplevtbl =
{
    sample_QueryInterface,
    sample_AddRef,
    sample_Release,
    sample_GetItem,
    sample_GetItemType,
    sample_CompareItem,
    sample_Compare,
    sample_GetUINT32,
    sample_GetUINT64,
    sample_GetDouble,
    sample_GetGUID,
    sample_GetStringLength,
    sample_GetString,
    sample_GetAllocatedString,
    sample_GetBlobSize,
    sample_GetBlob,
    sample_GetAllocatedBlob,
    sample_GetUnknown,
    sample_SetItem,
    sample_DeleteItem,
    sample_DeleteAllItems,
    sample_SetUINT32,
    sample_SetUINT64,
    sample_SetDouble,
    sample_SetGUID,
    sample_SetString,
    sample_SetBlob,
    sample_SetUnknown,
    sample_LockStore,
    sample_UnlockStore,
    sample_GetCount,
    sample_GetItemByIndex,
    sample_CopyAllItems,
    sample_GetSampleFlags,
    sample_SetSampleFlags,
    sample_GetSampleTime,
    sample_SetSampleTime,
    sample_GetSampleDuration,
    sample_SetSampleDuration,
    sample_GetBufferCount,
    sample_GetBufferByIndex,
    sample_ConvertToContiguousBuffer,
    sample_AddBuffer,
    sample_RemoveBufferByIndex,
    sample_RemoveAllBuffers,
    sample_GetTotalLength,
    sample_CopyToBuffer,
};

static HRESULT WINAPI tracked_sample_QueryInterface(IMFTrackedSample *iface, REFIID riid, void **obj)
{
    struct sample *sample = impl_from_IMFTrackedSample(iface);
    return IMFSample_QueryInterface(&sample->IMFSample_iface, riid, obj);
}

static ULONG WINAPI tracked_sample_AddRef(IMFTrackedSample *iface)
{
    struct sample *sample = impl_from_IMFTrackedSample(iface);
    return IMFSample_AddRef(&sample->IMFSample_iface);
}

static ULONG WINAPI tracked_sample_Release(IMFTrackedSample *iface)
{
    struct sample *sample = impl_from_IMFTrackedSample(iface);
    return IMFSample_Release(&sample->IMFSample_iface);
}

static HRESULT WINAPI tracked_sample_SetAllocator(IMFTrackedSample *iface,
        IMFAsyncCallback *sample_allocator, IUnknown *state)
{
    struct sample *sample = impl_from_IMFTrackedSample(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p, %p.\n", iface, sample_allocator, state);

    EnterCriticalSection(&sample->attributes.cs);

    if (sample->tracked_result)
        hr = MF_E_NOTACCEPTING;
    else
    {
        if (SUCCEEDED(hr = RtwqCreateAsyncResult((IUnknown *)iface, (IRtwqAsyncCallback *)sample_allocator,
                state, &sample->tracked_result)))
        {
            /* Account for additional refcount brought by 'state' object. This threshold is used
               on Release() to invoke tracker callback.  */
            sample->tracked_refcount = 1;
            if (state == (IUnknown *)&sample->IMFTrackedSample_iface ||
                    state == (IUnknown *)&sample->IMFSample_iface)
            {
                ++sample->tracked_refcount;
            }
        }
    }

    LeaveCriticalSection(&sample->attributes.cs);

    return hr;
}

static const IMFTrackedSampleVtbl tracked_sample_vtbl =
{
    tracked_sample_QueryInterface,
    tracked_sample_AddRef,
    tracked_sample_Release,
    tracked_sample_SetAllocator,
};

static const IMFSampleVtbl sample_tracked_vtbl =
{
    sample_QueryInterface,
    sample_AddRef,
    sample_tracked_Release,
    sample_GetItem,
    sample_GetItemType,
    sample_CompareItem,
    sample_Compare,
    sample_GetUINT32,
    sample_GetUINT64,
    sample_GetDouble,
    sample_GetGUID,
    sample_GetStringLength,
    sample_GetString,
    sample_GetAllocatedString,
    sample_GetBlobSize,
    sample_GetBlob,
    sample_GetAllocatedBlob,
    sample_GetUnknown,
    sample_SetItem,
    sample_DeleteItem,
    sample_DeleteAllItems,
    sample_SetUINT32,
    sample_SetUINT64,
    sample_SetDouble,
    sample_SetGUID,
    sample_SetString,
    sample_SetBlob,
    sample_SetUnknown,
    sample_LockStore,
    sample_UnlockStore,
    sample_GetCount,
    sample_GetItemByIndex,
    sample_CopyAllItems,
    sample_GetSampleFlags,
    sample_SetSampleFlags,
    sample_GetSampleTime,
    sample_SetSampleTime,
    sample_GetSampleDuration,
    sample_SetSampleDuration,
    sample_GetBufferCount,
    sample_GetBufferByIndex,
    sample_ConvertToContiguousBuffer,
    sample_AddBuffer,
    sample_RemoveBufferByIndex,
    sample_RemoveAllBuffers,
    sample_GetTotalLength,
    sample_CopyToBuffer,
};


/***********************************************************************
 *      MFCreateSample (mfplat.@)
 */
HRESULT WINAPI MFCreateSample(IMFSample **sample)
{
    struct sample *object;
    HRESULT hr;

    TRACE("%p.\n", sample);

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    if (FAILED(hr = init_attributes_object(&object->attributes, 0)))
    {
        heap_free(object);
        return hr;
    }

    object->IMFSample_iface.lpVtbl = &samplevtbl;

    *sample = &object->IMFSample_iface;

    TRACE("Created sample %p.\n", *sample);

    return S_OK;
}

/***********************************************************************
 *      MFCreateTrackedSample (mfplat.@)
 */
HRESULT WINAPI MFCreateTrackedSample(IMFTrackedSample **sample)
{
    struct sample *object;
    HRESULT hr;

    TRACE("%p.\n", sample);

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    if (FAILED(hr = init_attributes_object(&object->attributes, 0)))
    {
        heap_free(object);
        return hr;
    }

    object->IMFSample_iface.lpVtbl = &sample_tracked_vtbl;
    object->IMFTrackedSample_iface.lpVtbl = &tracked_sample_vtbl;

    *sample = &object->IMFTrackedSample_iface;

    return S_OK;
}
