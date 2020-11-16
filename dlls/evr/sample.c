/*
 * Copyright 2020 Nikolay Sivov
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

#include "evr.h"
#include "mfapi.h"
#include "mferror.h"
#include "d3d9.h"
#include "dxva2api.h"

#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(evr);

static const char *debugstr_time(LONGLONG time)
{
    ULONGLONG abstime = time >= 0 ? time : -time;
    unsigned int i = 0, j = 0;
    char buffer[23], rev[23];

    while (abstime || i <= 8)
    {
        buffer[i++] = '0' + (abstime % 10);
        abstime /= 10;
        if (i == 7) buffer[i++] = '.';
    }
    if (time < 0) buffer[i++] = '-';

    while (i--) rev[j++] = buffer[i];
    while (rev[j-1] == '0' && rev[j-2] != '.') --j;
    rev[j] = 0;

    return wine_dbg_sprintf("%s", rev);
}

struct surface_buffer
{
    IMFMediaBuffer IMFMediaBuffer_iface;
    IMFGetService IMFGetService_iface;
    LONG refcount;

    IUnknown *surface;
    ULONG length;
};

enum sample_prop_flags
{
    SAMPLE_PROP_HAS_DURATION      = 1 << 0,
    SAMPLE_PROP_HAS_TIMESTAMP     = 1 << 1,
    SAMPLE_PROP_HAS_DESIRED_PROPS = 1 << 2,
};

struct video_sample
{
    IMFSample IMFSample_iface;
    IMFTrackedSample IMFTrackedSample_iface;
    IMFDesiredSample IMFDesiredSample_iface;
    LONG refcount;

    IMFSample *sample;

    IMFAsyncResult *tracked_result;
    LONG tracked_refcount;

    LONGLONG timestamp;
    LONGLONG duration;
    LONGLONG desired_timestamp;
    LONGLONG desired_duration;
    unsigned int flags;
    CRITICAL_SECTION cs;
};

static struct video_sample *impl_from_IMFSample(IMFSample *iface)
{
    return CONTAINING_RECORD(iface, struct video_sample, IMFSample_iface);
}

static struct video_sample *impl_from_IMFTrackedSample(IMFTrackedSample *iface)
{
    return CONTAINING_RECORD(iface, struct video_sample, IMFTrackedSample_iface);
}

static struct video_sample *impl_from_IMFDesiredSample(IMFDesiredSample *iface)
{
    return CONTAINING_RECORD(iface, struct video_sample, IMFDesiredSample_iface);
}

static struct surface_buffer *impl_from_IMFMediaBuffer(IMFMediaBuffer *iface)
{
    return CONTAINING_RECORD(iface, struct surface_buffer, IMFMediaBuffer_iface);
}

static struct surface_buffer *impl_from_IMFGetService(IMFGetService *iface)
{
    return CONTAINING_RECORD(iface, struct surface_buffer, IMFGetService_iface);
}

struct tracked_async_result
{
    MFASYNCRESULT result;
    LONG refcount;
    IUnknown *object;
    IUnknown *state;
};

static struct tracked_async_result *impl_from_IMFAsyncResult(IMFAsyncResult *iface)
{
    return CONTAINING_RECORD(iface, struct tracked_async_result, result.AsyncResult);
}

static HRESULT WINAPI tracked_async_result_QueryInterface(IMFAsyncResult *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFAsyncResult) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFAsyncResult_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI tracked_async_result_AddRef(IMFAsyncResult *iface)
{
    struct tracked_async_result *result = impl_from_IMFAsyncResult(iface);
    ULONG refcount = InterlockedIncrement(&result->refcount);

    TRACE("%p, %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI tracked_async_result_Release(IMFAsyncResult *iface)
{
    struct tracked_async_result *result = impl_from_IMFAsyncResult(iface);
    ULONG refcount = InterlockedDecrement(&result->refcount);

    TRACE("%p, %u.\n", iface, refcount);

    if (!refcount)
    {
        if (result->result.pCallback)
            IMFAsyncCallback_Release(result->result.pCallback);
        if (result->object)
            IUnknown_Release(result->object);
        if (result->state)
            IUnknown_Release(result->state);
        heap_free(result);
    }

    return refcount;
}

static HRESULT WINAPI tracked_async_result_GetState(IMFAsyncResult *iface, IUnknown **state)
{
    struct tracked_async_result *result = impl_from_IMFAsyncResult(iface);

    TRACE("%p, %p.\n", iface, state);

    if (!result->state)
        return E_POINTER;

    *state = result->state;
    IUnknown_AddRef(*state);

    return S_OK;
}

static HRESULT WINAPI tracked_async_result_GetStatus(IMFAsyncResult *iface)
{
    struct tracked_async_result *result = impl_from_IMFAsyncResult(iface);

    TRACE("%p.\n", iface);

    return result->result.hrStatusResult;
}

static HRESULT WINAPI tracked_async_result_SetStatus(IMFAsyncResult *iface, HRESULT status)
{
    struct tracked_async_result *result = impl_from_IMFAsyncResult(iface);

    TRACE("%p, %#x.\n", iface, status);

    result->result.hrStatusResult = status;

    return S_OK;
}

static HRESULT WINAPI tracked_async_result_GetObject(IMFAsyncResult *iface, IUnknown **object)
{
    struct tracked_async_result *result = impl_from_IMFAsyncResult(iface);

    TRACE("%p, %p.\n", iface, object);

    if (!result->object)
        return E_POINTER;

    *object = result->object;
    IUnknown_AddRef(*object);

    return S_OK;
}

static IUnknown * WINAPI tracked_async_result_GetStateNoAddRef(IMFAsyncResult *iface)
{
    struct tracked_async_result *result = impl_from_IMFAsyncResult(iface);

    TRACE("%p.\n", iface);

    return result->state;
}

static const IMFAsyncResultVtbl tracked_async_result_vtbl =
{
    tracked_async_result_QueryInterface,
    tracked_async_result_AddRef,
    tracked_async_result_Release,
    tracked_async_result_GetState,
    tracked_async_result_GetStatus,
    tracked_async_result_SetStatus,
    tracked_async_result_GetObject,
    tracked_async_result_GetStateNoAddRef,
};

static HRESULT create_async_result(IUnknown *object, IMFAsyncCallback *callback,
        IUnknown *state, IMFAsyncResult **out)
{
    struct tracked_async_result *result;

    result = heap_alloc_zero(sizeof(*result));
    if (!result)
        return E_OUTOFMEMORY;

    result->result.AsyncResult.lpVtbl = &tracked_async_result_vtbl;
    result->refcount = 1;
    result->object = object;
    if (result->object)
        IUnknown_AddRef(result->object);
    result->result.pCallback = callback;
    if (result->result.pCallback)
        IMFAsyncCallback_AddRef(result->result.pCallback);
    result->state = state;
    if (result->state)
        IUnknown_AddRef(result->state);

    *out = &result->result.AsyncResult;

    return S_OK;
}

struct tracking_thread
{
    HANDLE hthread;
    DWORD tid;
    LONG refcount;
};
static struct tracking_thread tracking_thread;

static CRITICAL_SECTION tracking_thread_cs = { NULL, -1, 0, 0, 0, 0 };

enum tracking_thread_message
{
    TRACKM_STOP = WM_USER,
    TRACKM_INVOKE = WM_USER + 1,
};

static DWORD CALLBACK tracking_thread_proc(void *arg)
{
    HANDLE ready_event = arg;
    BOOL stop_thread = FALSE;
    IMFAsyncResult *result;
    MFASYNCRESULT *data;
    MSG msg;

    PeekMessageW(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    SetEvent(ready_event);

    while (!stop_thread)
    {
        MsgWaitForMultipleObjects(0, NULL, FALSE, INFINITE, QS_POSTMESSAGE);

        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            switch (msg.message)
            {
                case TRACKM_INVOKE:
                    result = (IMFAsyncResult *)msg.lParam;
                    data = (MFASYNCRESULT *)result;
                    if (data->pCallback)
                        IMFAsyncCallback_Invoke(data->pCallback, result);
                    IMFAsyncResult_Release(result);
                    break;

                case TRACKM_STOP:
                    stop_thread = TRUE;
                    break;

                default:
                    ;
            }
        }
    }

    TRACE("Tracking thread exiting.\n");

    return 0;
}

static void video_sample_create_tracking_thread(void)
{
    EnterCriticalSection(&tracking_thread_cs);

    if (++tracking_thread.refcount == 1)
    {
        HANDLE ready_event;

        ready_event = CreateEventW(NULL, FALSE, FALSE, NULL);

        if (!(tracking_thread.hthread = CreateThread(NULL, 0, tracking_thread_proc,
                ready_event, 0, &tracking_thread.tid)))
        {
            WARN("Failed to create sample tracking thread.\n");
            CloseHandle(ready_event);
            return;
        }

        WaitForSingleObject(ready_event, INFINITE);
        CloseHandle(ready_event);

        TRACE("Create tracking thread %#x.\n", tracking_thread.tid);
    }

    LeaveCriticalSection(&tracking_thread_cs);
}

static void video_sample_stop_tracking_thread(void)
{
    EnterCriticalSection(&tracking_thread_cs);

    if (!--tracking_thread.refcount)
    {
        PostThreadMessageW(tracking_thread.tid, TRACKM_STOP, 0, 0);
        CloseHandle(tracking_thread.hthread);
        memset(&tracking_thread, 0, sizeof(tracking_thread));
    }

    LeaveCriticalSection(&tracking_thread_cs);
}

static void video_sample_tracking_thread_invoke(IMFAsyncResult *result)
{
    if (!tracking_thread.tid)
    {
        WARN("Sample tracking thread is not initialized.\n");
        return;
    }

    IMFAsyncResult_AddRef(result);
    PostThreadMessageW(tracking_thread.tid, TRACKM_INVOKE, 0, (LPARAM)result);
}

struct sample_allocator
{
    IMFVideoSampleAllocator IMFVideoSampleAllocator_iface;
    IMFVideoSampleAllocatorCallback IMFVideoSampleAllocatorCallback_iface;
    IMFAsyncCallback tracking_callback;
    LONG refcount;

    IMFVideoSampleAllocatorNotify *callback;
    IDirect3DDeviceManager9 *device_manager;
    unsigned int free_sample_count;
    struct list free_samples;
    struct list used_samples;
    CRITICAL_SECTION cs;
};

struct queued_sample
{
    struct list entry;
    IMFSample *sample;
};

static struct sample_allocator *impl_from_IMFVideoSampleAllocator(IMFVideoSampleAllocator *iface)
{
    return CONTAINING_RECORD(iface, struct sample_allocator, IMFVideoSampleAllocator_iface);
}

static struct sample_allocator *impl_from_IMFVideoSampleAllocatorCallback(IMFVideoSampleAllocatorCallback *iface)
{
    return CONTAINING_RECORD(iface, struct sample_allocator, IMFVideoSampleAllocatorCallback_iface);
}

static struct sample_allocator *impl_from_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct sample_allocator, tracking_callback);
}

static HRESULT WINAPI sample_allocator_QueryInterface(IMFVideoSampleAllocator *iface, REFIID riid, void **obj)
{
    struct sample_allocator *allocator = impl_from_IMFVideoSampleAllocator(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFVideoSampleAllocator) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = &allocator->IMFVideoSampleAllocator_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFVideoSampleAllocatorCallback))
    {
        *obj = &allocator->IMFVideoSampleAllocatorCallback_iface;
    }
    else
    {
        WARN("Unsupported interface %s.\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);
    return S_OK;
}

static ULONG WINAPI sample_allocator_AddRef(IMFVideoSampleAllocator *iface)
{
    struct sample_allocator *allocator = impl_from_IMFVideoSampleAllocator(iface);
    ULONG refcount = InterlockedIncrement(&allocator->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static void sample_allocator_release_samples(struct sample_allocator *allocator)
{
    struct queued_sample *iter, *iter2;

    LIST_FOR_EACH_ENTRY_SAFE(iter, iter2, &allocator->free_samples, struct queued_sample, entry)
    {
        list_remove(&iter->entry);
        IMFSample_Release(iter->sample);
        heap_free(iter);
    }

    LIST_FOR_EACH_ENTRY_SAFE(iter, iter2, &allocator->used_samples, struct queued_sample, entry)
    {
        list_remove(&iter->entry);
        heap_free(iter);
    }
}

static ULONG WINAPI sample_allocator_Release(IMFVideoSampleAllocator *iface)
{
    struct sample_allocator *allocator = impl_from_IMFVideoSampleAllocator(iface);
    ULONG refcount = InterlockedDecrement(&allocator->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        if (allocator->callback)
            IMFVideoSampleAllocatorNotify_Release(allocator->callback);
        if (allocator->device_manager)
            IDirect3DDeviceManager9_Release(allocator->device_manager);
        sample_allocator_release_samples(allocator);
        DeleteCriticalSection(&allocator->cs);
        heap_free(allocator);
    }

    return refcount;
}

static HRESULT WINAPI sample_allocator_SetDirectXManager(IMFVideoSampleAllocator *iface,
        IUnknown *manager)
{
    struct sample_allocator *allocator = impl_from_IMFVideoSampleAllocator(iface);
    IDirect3DDeviceManager9 *device_manager = NULL;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, manager);

    if (manager && FAILED(hr = IUnknown_QueryInterface(manager, &IID_IDirect3DDeviceManager9,
            (void **)&device_manager)))
    {
        return hr;
    }

    EnterCriticalSection(&allocator->cs);

    if (allocator->device_manager)
        IDirect3DDeviceManager9_Release(allocator->device_manager);
    allocator->device_manager = device_manager;

    LeaveCriticalSection(&allocator->cs);

    return S_OK;
}

static HRESULT WINAPI sample_allocator_UninitializeSampleAllocator(IMFVideoSampleAllocator *iface)
{
    struct sample_allocator *allocator = impl_from_IMFVideoSampleAllocator(iface);

    TRACE("%p.\n", iface);

    EnterCriticalSection(&allocator->cs);

    sample_allocator_release_samples(allocator);
    allocator->free_sample_count = 0;

    LeaveCriticalSection(&allocator->cs);

    return S_OK;
}

static HRESULT sample_allocator_create_samples(struct sample_allocator *allocator, unsigned int sample_count,
        IMFMediaType *media_type)
{
    IDirectXVideoProcessorService *service = NULL;
    unsigned int i, width, height;
    IDirect3DSurface9 *surface;
    HANDLE hdevice = NULL;
    GUID major, subtype;
    UINT64 frame_size;
    IMFSample *sample;
    D3DFORMAT format;
    HRESULT hr;

    if (FAILED(IMFMediaType_GetMajorType(media_type, &major)))
        return MF_E_INVALIDMEDIATYPE;

    if (!IsEqualGUID(&major, &MFMediaType_Video))
        return MF_E_INVALIDMEDIATYPE;

    if (FAILED(IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &frame_size)))
        return MF_E_INVALIDMEDIATYPE;

    if (FAILED(IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &subtype)))
        return MF_E_INVALIDMEDIATYPE;

    format = subtype.Data1;
    height = frame_size;
    width = frame_size >> 32;

    if (allocator->device_manager)
    {
        if (SUCCEEDED(hr = IDirect3DDeviceManager9_OpenDeviceHandle(allocator->device_manager, &hdevice)))
        {
            hr = IDirect3DDeviceManager9_GetVideoService(allocator->device_manager, hdevice,
                    &IID_IDirectXVideoProcessorService, (void **)&service);
        }

        if (FAILED(hr))
        {
            WARN("Failed to get processor service, %#x.\n", hr);
            return hr;
        }
    }

    sample_allocator_release_samples(allocator);

    for (i = 0; i < sample_count; ++i)
    {
        struct queued_sample *queued_sample = heap_alloc(sizeof(*queued_sample));
        IMFMediaBuffer *buffer;

        if (SUCCEEDED(hr = MFCreateVideoSampleFromSurface(NULL, &sample)))
        {
            if (service)
            {
                if (SUCCEEDED(hr = IDirectXVideoProcessorService_CreateSurface(service, width, height,
                        0, format, D3DPOOL_DEFAULT, 0, DXVA2_VideoProcessorRenderTarget, &surface, NULL)))
                {
                    hr = MFCreateDXSurfaceBuffer(&IID_IDirect3DSurface9, (IUnknown *)surface, FALSE, &buffer);
                    IDirect3DSurface9_Release(surface);
                }
            }
            else
            {
                hr = MFCreate2DMediaBuffer(width, height, format, FALSE, &buffer);
            }

            if (SUCCEEDED(hr))
            {
                hr = IMFSample_AddBuffer(sample, buffer);
                IMFMediaBuffer_Release(buffer);
            }
        }

        if (FAILED(hr))
        {
            WARN("Unable to allocate %u samples.\n", sample_count);
            sample_allocator_release_samples(allocator);
            break;
        }

        queued_sample = heap_alloc(sizeof(*queued_sample));
        queued_sample->sample = sample;
        list_add_tail(&allocator->free_samples, &queued_sample->entry);
        allocator->free_sample_count++;
    }

    if (service)
        IDirectXVideoProcessorService_Release(service);

    if (allocator->device_manager)
        IDirect3DDeviceManager9_CloseDeviceHandle(allocator->device_manager, hdevice);

    return hr;
}

static HRESULT WINAPI sample_allocator_InitializeSampleAllocator(IMFVideoSampleAllocator *iface,
        DWORD sample_count, IMFMediaType *media_type)
{
    struct sample_allocator *allocator = impl_from_IMFVideoSampleAllocator(iface);
    HRESULT hr;

    TRACE("%p, %u, %p.\n", iface, sample_count, media_type);

    if (!sample_count)
        sample_count = 1;

    EnterCriticalSection(&allocator->cs);

    hr = sample_allocator_create_samples(allocator, sample_count, media_type);

    LeaveCriticalSection(&allocator->cs);

    return hr;
}

static HRESULT WINAPI sample_allocator_AllocateSample(IMFVideoSampleAllocator *iface, IMFSample **out)
{
    struct sample_allocator *allocator = impl_from_IMFVideoSampleAllocator(iface);
    IMFTrackedSample *tracked_sample;
    IMFSample *sample;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, out);

    EnterCriticalSection(&allocator->cs);

    if (list_empty(&allocator->free_samples) && list_empty(&allocator->used_samples))
        hr = MF_E_NOT_INITIALIZED;
    else if (list_empty(&allocator->free_samples))
        hr = MF_E_SAMPLEALLOCATOR_EMPTY;
    else
    {
        struct list *head = list_head(&allocator->free_samples);

        sample = LIST_ENTRY(head, struct queued_sample, entry)->sample;

        if (SUCCEEDED(hr = IMFSample_QueryInterface(sample, &IID_IMFTrackedSample, (void **)&tracked_sample)))
        {
            hr = IMFTrackedSample_SetAllocator(tracked_sample, &allocator->tracking_callback, NULL);
            IMFTrackedSample_Release(tracked_sample);
        }

        if (SUCCEEDED(hr))
        {
            list_remove(head);
            list_add_tail(&allocator->used_samples, head);
            allocator->free_sample_count--;

            /* Reference counter is not increased when sample is returned, so next release could trigger
               tracking condition. This is balanced by incremented reference counter when sample is returned
               back to the free list. */
            *out = sample;
        }
    }

    LeaveCriticalSection(&allocator->cs);

    return hr;
}

static const IMFVideoSampleAllocatorVtbl sample_allocator_vtbl =
{
    sample_allocator_QueryInterface,
    sample_allocator_AddRef,
    sample_allocator_Release,
    sample_allocator_SetDirectXManager,
    sample_allocator_UninitializeSampleAllocator,
    sample_allocator_InitializeSampleAllocator,
    sample_allocator_AllocateSample,
};

static HRESULT WINAPI sample_allocator_callback_QueryInterface(IMFVideoSampleAllocatorCallback *iface,
        REFIID riid, void **obj)
{
    struct sample_allocator *allocator = impl_from_IMFVideoSampleAllocatorCallback(iface);
    return IMFVideoSampleAllocator_QueryInterface(&allocator->IMFVideoSampleAllocator_iface, riid, obj);
}

static ULONG WINAPI sample_allocator_callback_AddRef(IMFVideoSampleAllocatorCallback *iface)
{
    struct sample_allocator *allocator = impl_from_IMFVideoSampleAllocatorCallback(iface);
    return IMFVideoSampleAllocator_AddRef(&allocator->IMFVideoSampleAllocator_iface);
}

static ULONG WINAPI sample_allocator_callback_Release(IMFVideoSampleAllocatorCallback *iface)
{
    struct sample_allocator *allocator = impl_from_IMFVideoSampleAllocatorCallback(iface);
    return IMFVideoSampleAllocator_Release(&allocator->IMFVideoSampleAllocator_iface);
}

static HRESULT WINAPI sample_allocator_callback_SetCallback(IMFVideoSampleAllocatorCallback *iface,
        IMFVideoSampleAllocatorNotify *callback)
{
    struct sample_allocator *allocator = impl_from_IMFVideoSampleAllocatorCallback(iface);

    TRACE("%p, %p.\n", iface, callback);

    EnterCriticalSection(&allocator->cs);
    if (allocator->callback)
        IMFVideoSampleAllocatorNotify_Release(allocator->callback);
    allocator->callback = callback;
    if (allocator->callback)
        IMFVideoSampleAllocatorNotify_AddRef(allocator->callback);
    LeaveCriticalSection(&allocator->cs);

    return S_OK;
}

static HRESULT WINAPI sample_allocator_callback_GetFreeSampleCount(IMFVideoSampleAllocatorCallback *iface,
        LONG *count)
{
    struct sample_allocator *allocator = impl_from_IMFVideoSampleAllocatorCallback(iface);

    TRACE("%p, %p.\n", iface, count);

    EnterCriticalSection(&allocator->cs);
    if (count)
        *count = allocator->free_sample_count;
    LeaveCriticalSection(&allocator->cs);

    return S_OK;
}

static const IMFVideoSampleAllocatorCallbackVtbl sample_allocator_callback_vtbl =
{
    sample_allocator_callback_QueryInterface,
    sample_allocator_callback_AddRef,
    sample_allocator_callback_Release,
    sample_allocator_callback_SetCallback,
    sample_allocator_callback_GetFreeSampleCount,
};

static HRESULT WINAPI sample_allocator_tracking_callback_QueryInterface(IMFAsyncCallback *iface,
        REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFAsyncCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFAsyncCallback_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI sample_allocator_tracking_callback_AddRef(IMFAsyncCallback *iface)
{
    struct sample_allocator *allocator = impl_from_IMFAsyncCallback(iface);
    return IMFVideoSampleAllocator_AddRef(&allocator->IMFVideoSampleAllocator_iface);
}

static ULONG WINAPI sample_allocator_tracking_callback_Release(IMFAsyncCallback *iface)
{
    struct sample_allocator *allocator = impl_from_IMFAsyncCallback(iface);
    return IMFVideoSampleAllocator_Release(&allocator->IMFVideoSampleAllocator_iface);
}

static HRESULT WINAPI sample_allocator_tracking_callback_GetParameters(IMFAsyncCallback *iface,
        DWORD *flags, DWORD *queue)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI sample_allocator_tracking_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct sample_allocator *allocator = impl_from_IMFAsyncCallback(iface);
    struct queued_sample *iter;
    IUnknown *object = NULL;
    IMFSample *sample = NULL;
    HRESULT hr;

    if (FAILED(IMFAsyncResult_GetObject(result, &object)))
        return E_UNEXPECTED;

    hr = IUnknown_QueryInterface(object, &IID_IMFSample, (void **)&sample);
    IUnknown_Release(object);
    if (FAILED(hr))
        return E_UNEXPECTED;

    EnterCriticalSection(&allocator->cs);

    LIST_FOR_EACH_ENTRY(iter, &allocator->used_samples, struct queued_sample, entry)
    {
        if (sample == iter->sample)
        {
            list_remove(&iter->entry);
            list_add_tail(&allocator->free_samples, &iter->entry);
            IMFSample_AddRef(iter->sample);
            allocator->free_sample_count++;
            break;
        }
    }

    IMFSample_Release(sample);

    if (allocator->callback)
        IMFVideoSampleAllocatorNotify_NotifyRelease(allocator->callback);

    LeaveCriticalSection(&allocator->cs);

    return S_OK;
}

static const IMFAsyncCallbackVtbl sample_allocator_tracking_callback_vtbl =
{
    sample_allocator_tracking_callback_QueryInterface,
    sample_allocator_tracking_callback_AddRef,
    sample_allocator_tracking_callback_Release,
    sample_allocator_tracking_callback_GetParameters,
    sample_allocator_tracking_callback_Invoke,
};

HRESULT WINAPI MFCreateVideoSampleAllocator(REFIID riid, void **obj)
{
    struct sample_allocator *object;
    HRESULT hr;

    TRACE("%s, %p.\n", debugstr_guid(riid), obj);

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFVideoSampleAllocator_iface.lpVtbl = &sample_allocator_vtbl;
    object->IMFVideoSampleAllocatorCallback_iface.lpVtbl = &sample_allocator_callback_vtbl;
    object->tracking_callback.lpVtbl = &sample_allocator_tracking_callback_vtbl;
    object->refcount = 1;
    list_init(&object->used_samples);
    list_init(&object->free_samples);
    InitializeCriticalSection(&object->cs);

    hr = IMFVideoSampleAllocator_QueryInterface(&object->IMFVideoSampleAllocator_iface, riid, obj);
    IMFVideoSampleAllocator_Release(&object->IMFVideoSampleAllocator_iface);

    return hr;
}

static HRESULT WINAPI video_sample_QueryInterface(IMFSample *iface, REFIID riid, void **out)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFSample) ||
            IsEqualIID(riid, &IID_IMFAttributes) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = &sample->IMFSample_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFTrackedSample))
    {
        *out = &sample->IMFTrackedSample_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFDesiredSample))
    {
        *out = &sample->IMFDesiredSample_iface;
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

static ULONG WINAPI video_sample_AddRef(IMFSample *iface)
{
    struct video_sample *sample = impl_from_IMFSample(iface);
    ULONG refcount = InterlockedIncrement(&sample->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI video_sample_Release(IMFSample *iface)
{
    struct video_sample *sample = impl_from_IMFSample(iface);
    ULONG refcount;

    IMFSample_LockStore(sample->sample);
    if (sample->tracked_result && sample->tracked_refcount == (sample->refcount - 1))
    {
        video_sample_tracking_thread_invoke(sample->tracked_result);
        IMFAsyncResult_Release(sample->tracked_result);
        sample->tracked_result = NULL;
        sample->tracked_refcount = 0;
    }
    IMFSample_UnlockStore(sample->sample);

    refcount = InterlockedDecrement(&sample->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        video_sample_stop_tracking_thread();
        if (sample->sample)
            IMFSample_Release(sample->sample);
        DeleteCriticalSection(&sample->cs);
        heap_free(sample);
    }

    return refcount;
}

static HRESULT WINAPI video_sample_GetItem(IMFSample *iface, REFGUID key, PROPVARIANT *value)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFSample_GetItem(sample->sample, key, value);
}

static HRESULT WINAPI video_sample_GetItemType(IMFSample *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), type);

    return IMFSample_GetItemType(sample->sample, key, type);
}

static HRESULT WINAPI video_sample_CompareItem(IMFSample *iface, REFGUID key, REFPROPVARIANT value, BOOL *result)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_guid(key), value, result);

    return IMFSample_CompareItem(sample->sample, key, value, result);
}

static HRESULT WINAPI video_sample_Compare(IMFSample *iface, IMFAttributes *theirs, MF_ATTRIBUTES_MATCH_TYPE type,
        BOOL *result)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %p, %d, %p.\n", iface, theirs, type, result);

    return IMFSample_Compare(sample->sample, theirs, type, result);
}

static HRESULT WINAPI video_sample_GetUINT32(IMFSample *iface, REFGUID key, UINT32 *value)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFSample_GetUINT32(sample->sample, key, value);
}

static HRESULT WINAPI video_sample_GetUINT64(IMFSample *iface, REFGUID key, UINT64 *value)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFSample_GetUINT64(sample->sample, key, value);
}

static HRESULT WINAPI video_sample_GetDouble(IMFSample *iface, REFGUID key, double *value)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFSample_GetDouble(sample->sample, key, value);
}

static HRESULT WINAPI video_sample_GetGUID(IMFSample *iface, REFGUID key, GUID *value)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFSample_GetGUID(sample->sample, key, value);
}

static HRESULT WINAPI video_sample_GetStringLength(IMFSample *iface, REFGUID key, UINT32 *length)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), length);

    return IMFSample_GetStringLength(sample->sample, key, length);
}

static HRESULT WINAPI video_sample_GetString(IMFSample *iface, REFGUID key, WCHAR *value, UINT32 size, UINT32 *length)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p, %u, %p.\n", iface, debugstr_guid(key), value, size, length);

    return IMFSample_GetString(sample->sample, key, value, size, length);
}

static HRESULT WINAPI video_sample_GetAllocatedString(IMFSample *iface, REFGUID key, WCHAR **value, UINT32 *length)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_guid(key), value, length);

    return IMFSample_GetAllocatedString(sample->sample, key, value, length);
}

static HRESULT WINAPI video_sample_GetBlobSize(IMFSample *iface, REFGUID key, UINT32 *size)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), size);

    return IMFSample_GetBlobSize(sample->sample, key, size);
}

static HRESULT WINAPI video_sample_GetBlob(IMFSample *iface, REFGUID key, UINT8 *buf, UINT32 bufsize, UINT32 *blobsize)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p, %u, %p.\n", iface, debugstr_guid(key), buf, bufsize, blobsize);

    return IMFSample_GetBlob(sample->sample, key, buf, bufsize, blobsize);
}

static HRESULT WINAPI video_sample_GetAllocatedBlob(IMFSample *iface, REFGUID key, UINT8 **buf, UINT32 *size)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_guid(key), buf, size);

    return IMFSample_GetAllocatedBlob(sample->sample, key, buf, size);
}

static HRESULT WINAPI video_sample_GetUnknown(IMFSample *iface, REFGUID key, REFIID riid, void **out)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_guid(key), debugstr_guid(riid), out);

    return IMFSample_GetUnknown(sample->sample, key, riid, out);
}

static HRESULT WINAPI video_sample_SetItem(IMFSample *iface, REFGUID key, REFPROPVARIANT value)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFSample_SetItem(sample->sample, key, value);
}

static HRESULT WINAPI video_sample_DeleteItem(IMFSample *iface, REFGUID key)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s.\n", iface, debugstr_guid(key));

    return IMFSample_DeleteItem(sample->sample, key);
}

static HRESULT WINAPI video_sample_DeleteAllItems(IMFSample *iface)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p.\n", iface);

    return IMFSample_DeleteAllItems(sample->sample);
}

static HRESULT WINAPI video_sample_SetUINT32(IMFSample *iface, REFGUID key, UINT32 value)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %u.\n", iface, debugstr_guid(key), value);

    return IMFSample_SetUINT32(sample->sample, key, value);
}

static HRESULT WINAPI video_sample_SetUINT64(IMFSample *iface, REFGUID key, UINT64 value)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_guid(key), wine_dbgstr_longlong(value));

    return IMFSample_SetUINT64(sample->sample, key, value);
}

static HRESULT WINAPI video_sample_SetDouble(IMFSample *iface, REFGUID key, double value)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %f.\n", iface, debugstr_guid(key), value);

    return IMFSample_SetDouble(sample->sample, key, value);
}

static HRESULT WINAPI video_sample_SetGUID(IMFSample *iface, REFGUID key, REFGUID value)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_guid(key), debugstr_guid(value));

    return IMFSample_SetGUID(sample->sample, key, value);
}

static HRESULT WINAPI video_sample_SetString(IMFSample *iface, REFGUID key, const WCHAR *value)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_guid(key), debugstr_w(value));

    return IMFSample_SetString(sample->sample, key, value);
}

static HRESULT WINAPI video_sample_SetBlob(IMFSample *iface, REFGUID key, const UINT8 *buf, UINT32 size)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p, %u.\n", iface, debugstr_guid(key), buf, size);

    return IMFSample_SetBlob(sample->sample, key, buf, size);
}

static HRESULT WINAPI video_sample_SetUnknown(IMFSample *iface, REFGUID key, IUnknown *unknown)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), unknown);

    return IMFSample_SetUnknown(sample->sample, key, unknown);
}

static HRESULT WINAPI video_sample_LockStore(IMFSample *iface)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p.\n", iface);

    return IMFSample_LockStore(sample->sample);
}

static HRESULT WINAPI video_sample_UnlockStore(IMFSample *iface)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p.\n", iface);

    return IMFSample_UnlockStore(sample->sample);
}

static HRESULT WINAPI video_sample_GetCount(IMFSample *iface, UINT32 *count)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %p.\n", iface, count);

    return IMFSample_GetCount(sample->sample, count);
}

static HRESULT WINAPI video_sample_GetItemByIndex(IMFSample *iface, UINT32 index, GUID *key, PROPVARIANT *value)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %u, %p, %p.\n", iface, index, key, value);

    return IMFSample_GetItemByIndex(sample->sample, index, key, value);
}

static HRESULT WINAPI video_sample_CopyAllItems(IMFSample *iface, IMFAttributes *dest)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %p.\n", iface, dest);

    return IMFSample_CopyAllItems(sample->sample, dest);
}

static HRESULT WINAPI video_sample_GetSampleFlags(IMFSample *iface, DWORD *flags)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %p.\n", iface, flags);

    return IMFSample_GetSampleFlags(sample->sample, flags);
}

static HRESULT WINAPI video_sample_SetSampleFlags(IMFSample *iface, DWORD flags)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %#x.\n", iface, flags);

    return IMFSample_SetSampleFlags(sample->sample, flags);
}

static HRESULT WINAPI video_sample_GetSampleTime(IMFSample *iface, LONGLONG *timestamp)
{
    struct video_sample *sample = impl_from_IMFSample(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, timestamp);

    EnterCriticalSection(&sample->cs);
    if (sample->flags & SAMPLE_PROP_HAS_TIMESTAMP)
        *timestamp = sample->timestamp;
    else
        hr = MF_E_NO_SAMPLE_TIMESTAMP;
    LeaveCriticalSection(&sample->cs);

    return hr;
}

static HRESULT WINAPI video_sample_SetSampleTime(IMFSample *iface, LONGLONG timestamp)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s.\n", iface, debugstr_time(timestamp));

    EnterCriticalSection(&sample->cs);
    sample->timestamp = timestamp;
    sample->flags |= SAMPLE_PROP_HAS_TIMESTAMP;
    LeaveCriticalSection(&sample->cs);

    return S_OK;
}

static HRESULT WINAPI video_sample_GetSampleDuration(IMFSample *iface, LONGLONG *duration)
{
    struct video_sample *sample = impl_from_IMFSample(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, duration);

    EnterCriticalSection(&sample->cs);
    if (sample->flags & SAMPLE_PROP_HAS_DURATION)
        *duration = sample->duration;
    else
        hr = MF_E_NO_SAMPLE_DURATION;
    LeaveCriticalSection(&sample->cs);

    return hr;
}

static HRESULT WINAPI video_sample_SetSampleDuration(IMFSample *iface, LONGLONG duration)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s.\n", iface, debugstr_time(duration));

    EnterCriticalSection(&sample->cs);
    sample->duration = duration;
    sample->flags |= SAMPLE_PROP_HAS_DURATION;
    LeaveCriticalSection(&sample->cs);

    return S_OK;
}

static HRESULT WINAPI video_sample_GetBufferCount(IMFSample *iface, DWORD *count)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %p.\n", iface, count);

    return IMFSample_GetBufferCount(sample->sample, count);
}

static HRESULT WINAPI video_sample_GetBufferByIndex(IMFSample *iface, DWORD index, IMFMediaBuffer **buffer)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %u, %p.\n", iface, index, buffer);

    return IMFSample_GetBufferByIndex(sample->sample, index, buffer);
}

static HRESULT WINAPI video_sample_ConvertToContiguousBuffer(IMFSample *iface, IMFMediaBuffer **buffer)
{
    TRACE("%p, %p.\n", iface, buffer);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_sample_AddBuffer(IMFSample *iface, IMFMediaBuffer *buffer)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %p.\n", iface, buffer);

    return IMFSample_AddBuffer(sample->sample, buffer);
}

static HRESULT WINAPI video_sample_RemoveBufferByIndex(IMFSample *iface, DWORD index)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %u.\n", iface, index);

    return IMFSample_RemoveBufferByIndex(sample->sample, index);
}

static HRESULT WINAPI video_sample_RemoveAllBuffers(IMFSample *iface)
{
    struct video_sample *sample = impl_from_IMFSample(iface);

    TRACE("%p.\n", iface);

    return IMFSample_RemoveAllBuffers(sample->sample);
}

static HRESULT WINAPI video_sample_GetTotalLength(IMFSample *iface, DWORD *total_length)
{
    TRACE("%p, %p.\n", iface, total_length);

    *total_length = 0;

    return S_OK;
}

static HRESULT WINAPI video_sample_CopyToBuffer(IMFSample *iface, IMFMediaBuffer *buffer)
{
    TRACE("%p, %p.\n", iface, buffer);

    return E_NOTIMPL;
}

static const IMFSampleVtbl video_sample_vtbl =
{
    video_sample_QueryInterface,
    video_sample_AddRef,
    video_sample_Release,
    video_sample_GetItem,
    video_sample_GetItemType,
    video_sample_CompareItem,
    video_sample_Compare,
    video_sample_GetUINT32,
    video_sample_GetUINT64,
    video_sample_GetDouble,
    video_sample_GetGUID,
    video_sample_GetStringLength,
    video_sample_GetString,
    video_sample_GetAllocatedString,
    video_sample_GetBlobSize,
    video_sample_GetBlob,
    video_sample_GetAllocatedBlob,
    video_sample_GetUnknown,
    video_sample_SetItem,
    video_sample_DeleteItem,
    video_sample_DeleteAllItems,
    video_sample_SetUINT32,
    video_sample_SetUINT64,
    video_sample_SetDouble,
    video_sample_SetGUID,
    video_sample_SetString,
    video_sample_SetBlob,
    video_sample_SetUnknown,
    video_sample_LockStore,
    video_sample_UnlockStore,
    video_sample_GetCount,
    video_sample_GetItemByIndex,
    video_sample_CopyAllItems,
    video_sample_GetSampleFlags,
    video_sample_SetSampleFlags,
    video_sample_GetSampleTime,
    video_sample_SetSampleTime,
    video_sample_GetSampleDuration,
    video_sample_SetSampleDuration,
    video_sample_GetBufferCount,
    video_sample_GetBufferByIndex,
    video_sample_ConvertToContiguousBuffer,
    video_sample_AddBuffer,
    video_sample_RemoveBufferByIndex,
    video_sample_RemoveAllBuffers,
    video_sample_GetTotalLength,
    video_sample_CopyToBuffer,
};

static HRESULT WINAPI tracked_video_sample_QueryInterface(IMFTrackedSample *iface, REFIID riid, void **obj)
{
    struct video_sample *sample = impl_from_IMFTrackedSample(iface);
    return IMFSample_QueryInterface(&sample->IMFSample_iface, riid, obj);
}

static ULONG WINAPI tracked_video_sample_AddRef(IMFTrackedSample *iface)
{
    struct video_sample *sample = impl_from_IMFTrackedSample(iface);
    return IMFSample_AddRef(&sample->IMFSample_iface);
}

static ULONG WINAPI tracked_video_sample_Release(IMFTrackedSample *iface)
{
    struct video_sample *sample = impl_from_IMFTrackedSample(iface);
    return IMFSample_Release(&sample->IMFSample_iface);
}

static HRESULT WINAPI tracked_video_sample_SetAllocator(IMFTrackedSample *iface,
        IMFAsyncCallback *sample_allocator, IUnknown *state)
{
    struct video_sample *sample = impl_from_IMFTrackedSample(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p, %p.\n", iface, sample_allocator, state);

    IMFSample_LockStore(sample->sample);

    if (sample->tracked_result)
        hr = MF_E_NOTACCEPTING;
    else
    {
        if (SUCCEEDED(hr = create_async_result((IUnknown *)iface, sample_allocator, state, &sample->tracked_result)))
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

    IMFSample_UnlockStore(sample->sample);

    return hr;
}

static const IMFTrackedSampleVtbl tracked_video_sample_vtbl =
{
    tracked_video_sample_QueryInterface,
    tracked_video_sample_AddRef,
    tracked_video_sample_Release,
    tracked_video_sample_SetAllocator,
};

static HRESULT WINAPI desired_video_sample_QueryInterface(IMFDesiredSample *iface, REFIID riid, void **obj)
{
    struct video_sample *sample = impl_from_IMFDesiredSample(iface);
    return IMFSample_QueryInterface(&sample->IMFSample_iface, riid, obj);
}

static ULONG WINAPI desired_video_sample_AddRef(IMFDesiredSample *iface)
{
    struct video_sample *sample = impl_from_IMFDesiredSample(iface);
    return IMFSample_AddRef(&sample->IMFSample_iface);
}

static ULONG WINAPI desired_video_sample_Release(IMFDesiredSample *iface)
{
    struct video_sample *sample = impl_from_IMFDesiredSample(iface);
    return IMFSample_Release(&sample->IMFSample_iface);
}

static HRESULT WINAPI desired_video_sample_GetDesiredSampleTimeAndDuration(IMFDesiredSample *iface,
        LONGLONG *sample_time, LONGLONG *sample_duration)
{
    struct video_sample *sample = impl_from_IMFDesiredSample(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p, %p.\n", iface, sample_time, sample_duration);

    if (!sample_time || !sample_duration)
        return E_POINTER;

    EnterCriticalSection(&sample->cs);
    if (sample->flags & SAMPLE_PROP_HAS_DESIRED_PROPS)
    {
        *sample_time = sample->desired_timestamp;
        *sample_duration = sample->desired_duration;
    }
    else
        hr = MF_E_NOT_AVAILABLE;
    LeaveCriticalSection(&sample->cs);

    return hr;
}

static void WINAPI desired_video_sample_SetDesiredSampleTimeAndDuration(IMFDesiredSample *iface,
        LONGLONG sample_time, LONGLONG sample_duration)
{
    struct video_sample *sample = impl_from_IMFDesiredSample(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_time(sample_time), debugstr_time(sample_duration));

    EnterCriticalSection(&sample->cs);
    sample->flags |= SAMPLE_PROP_HAS_DESIRED_PROPS;
    sample->desired_timestamp = sample_time;
    sample->desired_duration = sample_duration;
    LeaveCriticalSection(&sample->cs);
}

static void WINAPI desired_video_sample_Clear(IMFDesiredSample *iface)
{
    struct video_sample *sample = impl_from_IMFDesiredSample(iface);

    TRACE("%p.\n", iface);

    EnterCriticalSection(&sample->cs);
    sample->flags = 0;
    IMFSample_SetSampleFlags(sample->sample, 0);
    IMFSample_DeleteAllItems(sample->sample);
    LeaveCriticalSection(&sample->cs);
}

static const IMFDesiredSampleVtbl desired_video_sample_vtbl =
{
    desired_video_sample_QueryInterface,
    desired_video_sample_AddRef,
    desired_video_sample_Release,
    desired_video_sample_GetDesiredSampleTimeAndDuration,
    desired_video_sample_SetDesiredSampleTimeAndDuration,
    desired_video_sample_Clear,
};

static HRESULT WINAPI surface_buffer_QueryInterface(IMFMediaBuffer *iface, REFIID riid, void **obj)
{
    struct surface_buffer *buffer = impl_from_IMFMediaBuffer(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFMediaBuffer) || IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = &buffer->IMFMediaBuffer_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFGetService))
    {
        *obj = &buffer->IMFGetService_iface;
    }
    else
    {
        WARN("Unsupported interface %s.\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);
    return S_OK;
}

static ULONG WINAPI surface_buffer_AddRef(IMFMediaBuffer *iface)
{
    struct surface_buffer *buffer = impl_from_IMFMediaBuffer(iface);
    ULONG refcount = InterlockedIncrement(&buffer->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI surface_buffer_Release(IMFMediaBuffer *iface)
{
    struct surface_buffer *buffer = impl_from_IMFMediaBuffer(iface);
    ULONG refcount = InterlockedDecrement(&buffer->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        IUnknown_Release(buffer->surface);
        heap_free(buffer);
    }

    return refcount;
}

static HRESULT WINAPI surface_buffer_Lock(IMFMediaBuffer *iface, BYTE **data, DWORD *maxlength, DWORD *length)
{
    TRACE("%p, %p, %p, %p.\n", iface, data, maxlength, length);

    return E_NOTIMPL;
}

static HRESULT WINAPI surface_buffer_Unlock(IMFMediaBuffer *iface)
{
    TRACE("%p.\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI surface_buffer_GetCurrentLength(IMFMediaBuffer *iface, DWORD *length)
{
    struct surface_buffer *buffer = impl_from_IMFMediaBuffer(iface);

    TRACE("%p.\n", iface);

    *length = buffer->length;

    return S_OK;
}

static HRESULT WINAPI surface_buffer_SetCurrentLength(IMFMediaBuffer *iface, DWORD length)
{
    struct surface_buffer *buffer = impl_from_IMFMediaBuffer(iface);

    TRACE("%p.\n", iface);

    buffer->length = length;

    return S_OK;
}

static HRESULT WINAPI surface_buffer_GetMaxLength(IMFMediaBuffer *iface, DWORD *length)
{
    TRACE("%p.\n", iface);

    return E_NOTIMPL;
}

static const IMFMediaBufferVtbl surface_buffer_vtbl =
{
    surface_buffer_QueryInterface,
    surface_buffer_AddRef,
    surface_buffer_Release,
    surface_buffer_Lock,
    surface_buffer_Unlock,
    surface_buffer_GetCurrentLength,
    surface_buffer_SetCurrentLength,
    surface_buffer_GetMaxLength,
};

static HRESULT WINAPI surface_buffer_gs_QueryInterface(IMFGetService *iface, REFIID riid, void **obj)
{
    struct surface_buffer *buffer = impl_from_IMFGetService(iface);
    return IMFMediaBuffer_QueryInterface(&buffer->IMFMediaBuffer_iface, riid, obj);
}

static ULONG WINAPI surface_buffer_gs_AddRef(IMFGetService *iface)
{
    struct surface_buffer *buffer = impl_from_IMFGetService(iface);
    return IMFMediaBuffer_AddRef(&buffer->IMFMediaBuffer_iface);
}

static ULONG WINAPI surface_buffer_gs_Release(IMFGetService *iface)
{
    struct surface_buffer *buffer = impl_from_IMFGetService(iface);
    return IMFMediaBuffer_Release(&buffer->IMFMediaBuffer_iface);
}

static HRESULT WINAPI surface_buffer_gs_GetService(IMFGetService *iface, REFGUID service, REFIID riid, void **obj)
{
    struct surface_buffer *buffer = impl_from_IMFGetService(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_guid(service), debugstr_guid(riid), obj);

    if (IsEqualGUID(service, &MR_BUFFER_SERVICE))
        return IUnknown_QueryInterface(buffer->surface, riid, obj);

    return E_NOINTERFACE;
}

static const IMFGetServiceVtbl surface_buffer_gs_vtbl =
{
    surface_buffer_gs_QueryInterface,
    surface_buffer_gs_AddRef,
    surface_buffer_gs_Release,
    surface_buffer_gs_GetService,
};

static HRESULT create_surface_buffer(IUnknown *surface, IMFMediaBuffer **buffer)
{
    struct surface_buffer *object;

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFMediaBuffer_iface.lpVtbl = &surface_buffer_vtbl;
    object->IMFGetService_iface.lpVtbl = &surface_buffer_gs_vtbl;
    object->refcount = 1;
    object->surface = surface;
    IUnknown_AddRef(object->surface);

    *buffer = &object->IMFMediaBuffer_iface;

    return S_OK;
}

HRESULT WINAPI MFCreateVideoSampleFromSurface(IUnknown *surface, IMFSample **sample)
{
    struct video_sample *object;
    IMFMediaBuffer *buffer = NULL;
    HRESULT hr;

    TRACE("%p, %p.\n", surface, sample);

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFSample_iface.lpVtbl = &video_sample_vtbl;
    object->IMFTrackedSample_iface.lpVtbl = &tracked_video_sample_vtbl;
    object->IMFDesiredSample_iface.lpVtbl = &desired_video_sample_vtbl;
    object->refcount = 1;
    InitializeCriticalSection(&object->cs);

    if (FAILED(hr = MFCreateSample(&object->sample)))
    {
        heap_free(object);
        return hr;
    }

    if (surface && FAILED(hr = create_surface_buffer(surface, &buffer)))
    {
        IMFSample_Release(&object->IMFSample_iface);
        return hr;
    }

    if (buffer)
        IMFSample_AddBuffer(object->sample, buffer);

    video_sample_create_tracking_thread();

    *sample = &object->IMFSample_iface;

    return S_OK;
}
