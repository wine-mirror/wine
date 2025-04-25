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
#include "d3d9.h"
#include "mfapi.h"
#include "mferror.h"
#include "dxva2api.h"

#include "evr_classes.h"
#include "evr_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(evr);

/*
   Initial state represents just created object, presenter never returns to this state.
   Shutdown state is entered on ReleaseServicePointers(), terminal state.
   Started/stopped/paused states are controlled by clock state changes.
*/

enum presenter_state
{
    PRESENTER_STATE_INITIAL = 0,
    PRESENTER_STATE_SHUT_DOWN,
    PRESENTER_STATE_STARTED,
    PRESENTER_STATE_STOPPED,
    PRESENTER_STATE_PAUSED,
};

enum presenter_flags
{
    PRESENTER_MIXER_HAS_INPUT = 0x1,
};

enum streaming_thread_message
{
    EVRM_STOP = WM_USER,
    EVRM_PRESENT = WM_USER + 1,
};

struct sample_queue
{
    IMFSample **samples;
    unsigned int size;
    unsigned int used;
    unsigned int front;
    unsigned int back;
    IMFSample *last_presented;
    CRITICAL_SECTION cs;
};

struct streaming_thread
{
    HANDLE hthread;
    HANDLE ready_event;
    DWORD tid;
    struct sample_queue queue;
};

struct video_presenter
{
    IMFVideoPresenter IMFVideoPresenter_iface;
    IMFVideoDeviceID IMFVideoDeviceID_iface;
    IMFTopologyServiceLookupClient IMFTopologyServiceLookupClient_iface;
    IMFVideoDisplayControl IMFVideoDisplayControl_iface;
    IMFRateSupport IMFRateSupport_iface;
    IMFGetService IMFGetService_iface;
    IMFVideoPositionMapper IMFVideoPositionMapper_iface;
    IQualProp IQualProp_iface;
    IMFQualityAdvise IMFQualityAdvise_iface;
    IMFQualityAdviseLimits IMFQualityAdviseLimits_iface;
    IDirect3DDeviceManager9 IDirect3DDeviceManager9_iface;
    IMFVideoSampleAllocatorNotify allocator_cb;
    IUnknown IUnknown_inner;
    IUnknown *outer_unk;
    LONG refcount;

    IMFTransform *mixer;
    IMFClock *clock;
    IMediaEventSink *event_sink;

    IDirect3DDeviceManager9 *device_manager;
    IDirect3DSwapChain9 *swapchain;
    HANDLE hdevice;

    IMFVideoSampleAllocator *allocator;
    struct streaming_thread thread;
    unsigned int allocator_capacity;
    IMFMediaType *media_type;
    LONGLONG frame_time_threshold;
    UINT reset_token;
    HWND video_window;
    MFVideoNormalizedRect src_rect;
    RECT dst_rect;
    DWORD rendering_prefs;
    SIZE native_size;
    SIZE native_ratio;
    unsigned int ar_mode;
    unsigned int state;
    unsigned int flags;

    struct
    {
        int presented;
        LONGLONG sampletime;
    } frame_stats;

    CRITICAL_SECTION cs;
};

static struct video_presenter *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct video_presenter, IUnknown_inner);
}

static struct video_presenter *impl_from_IMFVideoPresenter(IMFVideoPresenter *iface)
{
    return CONTAINING_RECORD(iface, struct video_presenter, IMFVideoPresenter_iface);
}

static struct video_presenter *impl_from_IMFVideoDeviceID(IMFVideoDeviceID *iface)
{
    return CONTAINING_RECORD(iface, struct video_presenter, IMFVideoDeviceID_iface);
}

static struct video_presenter *impl_from_IMFTopologyServiceLookupClient(IMFTopologyServiceLookupClient *iface)
{
    return CONTAINING_RECORD(iface, struct video_presenter, IMFTopologyServiceLookupClient_iface);
}

static struct video_presenter *impl_from_IMFVideoDisplayControl(IMFVideoDisplayControl *iface)
{
    return CONTAINING_RECORD(iface, struct video_presenter, IMFVideoDisplayControl_iface);
}

static struct video_presenter *impl_from_IMFRateSupport(IMFRateSupport *iface)
{
    return CONTAINING_RECORD(iface, struct video_presenter, IMFRateSupport_iface);
}

static struct video_presenter *impl_from_IMFGetService(IMFGetService *iface)
{
    return CONTAINING_RECORD(iface, struct video_presenter, IMFGetService_iface);
}

static struct video_presenter *impl_from_IMFVideoPositionMapper(IMFVideoPositionMapper *iface)
{
    return CONTAINING_RECORD(iface, struct video_presenter, IMFVideoPositionMapper_iface);
}

static struct video_presenter *impl_from_IMFVideoSampleAllocatorNotify(IMFVideoSampleAllocatorNotify *iface)
{
    return CONTAINING_RECORD(iface, struct video_presenter, allocator_cb);
}

static struct video_presenter *impl_from_IQualProp(IQualProp *iface)
{
    return CONTAINING_RECORD(iface, struct video_presenter, IQualProp_iface);
}

static struct video_presenter *impl_from_IMFQualityAdvise(IMFQualityAdvise *iface)
{
    return CONTAINING_RECORD(iface, struct video_presenter, IMFQualityAdvise_iface);
}

static struct video_presenter *impl_from_IMFQualityAdviseLimits(IMFQualityAdviseLimits *iface)
{
    return CONTAINING_RECORD(iface, struct video_presenter, IMFQualityAdviseLimits_iface);
}

static struct video_presenter *impl_from_IDirect3DDeviceManager9(IDirect3DDeviceManager9 *iface)
{
    return CONTAINING_RECORD(iface, struct video_presenter, IDirect3DDeviceManager9_iface);
}

static void video_presenter_notify_renderer(struct video_presenter *presenter,
        LONG event, LONG_PTR param1, LONG_PTR param2)
{
    if (presenter->event_sink)
        IMediaEventSink_Notify(presenter->event_sink, event, param1, param2);
}

static unsigned int get_gcd(unsigned int a, unsigned int b)
{
    unsigned int m;

    while (b)
    {
        m = a % b;
        a = b;
        b = m;
    }

    return a;
}

static HRESULT video_presenter_get_device(struct video_presenter *presenter, IDirect3DDevice9 **device)
{
    HRESULT hr;

    if (!presenter->hdevice)
    {
        if (FAILED(hr = IDirect3DDeviceManager9_OpenDeviceHandle(presenter->device_manager, &presenter->hdevice)))
            return hr;
    }

    return IDirect3DDeviceManager9_LockDevice(presenter->device_manager, presenter->hdevice, device, TRUE);
}

static void video_presenter_get_native_video_size(struct video_presenter *presenter)
{
    IMFMediaType *media_type;
    UINT64 frame_size = 0;

    memset(&presenter->native_size, 0, sizeof(presenter->native_size));
    memset(&presenter->native_ratio, 0, sizeof(presenter->native_ratio));

    if (!presenter->mixer)
        return;

    if (FAILED(IMFTransform_GetInputCurrentType(presenter->mixer, 0, &media_type)))
        return;

    if (SUCCEEDED(IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &frame_size)))
    {
        unsigned int gcd;

        presenter->native_size.cx = frame_size >> 32;
        presenter->native_size.cy = frame_size;

        if ((gcd = get_gcd(presenter->native_size.cx, presenter->native_size.cy)))
        {
            presenter->native_ratio.cx = presenter->native_size.cx / gcd;
            presenter->native_ratio.cy = presenter->native_size.cy / gcd;
        }
    }

    IMFMediaType_Release(media_type);
}

/* It is important this is called to reset callback too to break circular referencing,
   when allocator keeps a reference of its container, that created it. */
static void video_presenter_set_allocator_callback(struct video_presenter *presenter,
        IMFVideoSampleAllocatorNotify *notify_cb)
{
    IMFVideoSampleAllocatorCallback *cb;

    IMFVideoSampleAllocator_QueryInterface(presenter->allocator, &IID_IMFVideoSampleAllocatorCallback, (void **)&cb);
    IMFVideoSampleAllocatorCallback_SetCallback(cb, notify_cb);
    IMFVideoSampleAllocatorCallback_Release(cb);
}

static void video_presenter_reset_media_type(struct video_presenter *presenter)
{
    if (presenter->media_type)
        IMFMediaType_Release(presenter->media_type);
    presenter->media_type = NULL;

    if (presenter->allocator)
    {
        IMFVideoSampleAllocator_UninitializeSampleAllocator(presenter->allocator);
        video_presenter_set_allocator_callback(presenter, NULL);
    }
}

static HRESULT video_presenter_set_media_type(struct video_presenter *presenter, IMFMediaType *media_type)
{
    DWORD flags;
    HRESULT hr;

    if (!media_type)
    {
        video_presenter_reset_media_type(presenter);
        return S_OK;
    }

    if (presenter->media_type && IMFMediaType_IsEqual(presenter->media_type, media_type, &flags) == S_OK)
        return S_OK;

    video_presenter_reset_media_type(presenter);

    if (SUCCEEDED(hr = IMFVideoSampleAllocator_InitializeSampleAllocator(presenter->allocator,
            presenter->allocator_capacity, media_type)))
    {
        MFRatio ratio;
        UINT64 rate, frametime;

        presenter->media_type = media_type;
        IMFMediaType_AddRef(presenter->media_type);

        if (SUCCEEDED(IMFMediaType_GetUINT64(presenter->media_type, &MF_MT_FRAME_RATE, &rate)))
        {
            ratio.Denominator = rate;
            ratio.Numerator = rate >> 32;
        }
        else
        {
            ratio.Denominator = 1;
            ratio.Numerator = 30;
        }

        MFFrameRateToAverageTimePerFrame(ratio.Numerator, ratio.Denominator, &frametime);
        presenter->frame_time_threshold = frametime / 4;
    }
    else
        WARN("Failed to initialize sample allocator, hr %#lx.\n", hr);

    return hr;
}

static HRESULT video_presenter_configure_output_type(struct video_presenter *presenter, const MFVideoArea *aperture,
        IMFMediaType *media_type)
{
    GUID subtype;
    LONG stride;
    DWORD size;
    HRESULT hr;

    hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, (UINT64)aperture->Area.cx << 32 | aperture->Area.cy);
    if (SUCCEEDED(hr))
        hr = IMFMediaType_SetBlob(media_type, &MF_MT_GEOMETRIC_APERTURE, (UINT8 *)aperture, sizeof(*aperture));
    if (SUCCEEDED(hr))
        hr = IMFMediaType_SetBlob(media_type, &MF_MT_MINIMUM_DISPLAY_APERTURE, (UINT8 *)aperture, sizeof(*aperture));

    if (SUCCEEDED(hr))
        hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &subtype);

    if (SUCCEEDED(hr))
    {
        hr = MFGetStrideForBitmapInfoHeader(subtype.Data1, aperture->Area.cx, &stride);
        if (SUCCEEDED(hr))
            hr = MFGetPlaneSize(subtype.Data1, aperture->Area.cx, aperture->Area.cy, &size);
        if (SUCCEEDED(hr))
            hr = IMFMediaType_SetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, abs(stride));
        if (SUCCEEDED(hr))
            hr = IMFMediaType_SetUINT32(media_type, &MF_MT_SAMPLE_SIZE, size);
    }

    return hr;
}

static HRESULT video_presenter_invalidate_media_type(struct video_presenter *presenter)
{
    IMFMediaType *media_type, *candidate_type;
    unsigned int idx = 0;
    RECT rect;
    HRESULT hr;

    if (!presenter->mixer)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    if (FAILED(hr = MFCreateMediaType(&media_type)))
        return hr;

    video_presenter_get_native_video_size(presenter);

    while (SUCCEEDED(hr = IMFTransform_GetOutputAvailableType(presenter->mixer, 0, idx++, &candidate_type)))
    {
        MFVideoArea aperture = {{ 0 }};

        rect = presenter->dst_rect;
        if (!IsRectEmpty(&rect))
        {
            aperture.Area.cx = rect.right - rect.left;
            aperture.Area.cy = rect.bottom - rect.top;
        }
        else if (FAILED(IMFMediaType_GetBlob(candidate_type, &MF_MT_GEOMETRIC_APERTURE, (UINT8 *)&aperture,
                sizeof(aperture), NULL)))
        {
            aperture.Area.cx = presenter->native_size.cx;
            aperture.Area.cy = presenter->native_size.cy;
        }

        /* FIXME: check that d3d device supports this format */

        if (FAILED(hr = IMFMediaType_CopyAllItems(candidate_type, (IMFAttributes *)media_type)))
            WARN("Failed to clone a media type, hr %#lx.\n", hr);
        IMFMediaType_Release(candidate_type);

        hr = video_presenter_configure_output_type(presenter, &aperture, media_type);

        if (SUCCEEDED(hr))
            hr = IMFTransform_SetOutputType(presenter->mixer, 0, media_type, MFT_SET_TYPE_TEST_ONLY);

        if (SUCCEEDED(hr))
            hr = video_presenter_set_media_type(presenter, media_type);

        if (SUCCEEDED(hr))
            hr = IMFTransform_SetOutputType(presenter->mixer, 0, media_type, 0);

        if (SUCCEEDED(hr))
            break;
    }

    IMFMediaType_Release(media_type);

    return hr;
}

static HRESULT video_presenter_sample_queue_init(struct video_presenter *presenter)
{
    struct sample_queue *queue = &presenter->thread.queue;

    if (queue->size)
        return S_OK;

    memset(queue, 0, sizeof(*queue));
    if (!(queue->samples = calloc(presenter->allocator_capacity, sizeof(*queue->samples))))
        return E_OUTOFMEMORY;

    queue->size = presenter->allocator_capacity;
    queue->back = queue->size - 1;
    InitializeCriticalSection(&queue->cs);

    return S_OK;
}

static void video_presenter_sample_queue_push(struct video_presenter *presenter, IMFSample *sample,
        BOOL at_front)
{
    struct sample_queue *queue = &presenter->thread.queue;
    unsigned int idx;

    EnterCriticalSection(&queue->cs);
    if (queue->used != queue->size)
    {
        if (at_front)
            idx = queue->front = (queue->size + queue->front - 1) % queue->size;
        else
            idx = queue->back = (queue->back + 1) % queue->size;
        queue->samples[idx] = sample;
        queue->used++;
        IMFSample_AddRef(sample);
    }
    LeaveCriticalSection(&queue->cs);
}

static BOOL video_presenter_sample_queue_pop(struct video_presenter *presenter, IMFSample **sample)
{
    struct sample_queue *queue = &presenter->thread.queue;

    EnterCriticalSection(&queue->cs);
    if (queue->used)
    {
        *sample = queue->samples[queue->front];
        queue->front = (queue->front + 1) % queue->size;
        queue->used--;
    }
    else
        *sample = NULL;
    LeaveCriticalSection(&queue->cs);

    return *sample != NULL;
}


static void video_presenter_sample_queue_free(struct video_presenter *presenter)
{
    struct sample_queue *queue = &presenter->thread.queue;
    IMFSample *sample;

    while (video_presenter_sample_queue_pop(presenter, &sample))
        IMFSample_Release(sample);

    free(queue->samples);
    DeleteCriticalSection(&queue->cs);
}

static HRESULT video_presenter_get_sample_surface(IMFSample *sample, IDirect3DSurface9 **surface)
{
    IMFMediaBuffer *buffer;
    IMFGetService *gs;
    HRESULT hr;

    if (FAILED(hr = IMFSample_GetBufferByIndex(sample, 0, &buffer)))
        return hr;

    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMFGetService, (void **)&gs);
    IMFMediaBuffer_Release(buffer);
    if (FAILED(hr))
        return hr;

    hr = IMFGetService_GetService(gs, &MR_BUFFER_SERVICE, &IID_IDirect3DSurface9, (void **)surface);
    IMFGetService_Release(gs);
    return hr;
}

static void video_presenter_sample_present(struct video_presenter *presenter, IMFSample *sample)
{
    IDirect3DSurface9 *surface, *backbuffer;
    IDirect3DDevice9 *device;
    struct sample_queue *queue = &presenter->thread.queue;
    HRESULT hr;

    if (FAILED(hr = video_presenter_get_sample_surface(sample, &surface)))
    {
        WARN("Failed to get sample surface, hr %#lx.\n", hr);
        return;
    }

    if (presenter->swapchain)
    {
        if (SUCCEEDED(hr = IDirect3DSwapChain9_GetBackBuffer(presenter->swapchain, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer)))
        {
            IDirect3DSwapChain9_GetDevice(presenter->swapchain, &device);
            IDirect3DDevice9_StretchRect(device, surface, NULL, backbuffer, NULL, D3DTEXF_POINT);

            IDirect3DSwapChain9_Present(presenter->swapchain, NULL, NULL, NULL, NULL, 0);
            presenter->frame_stats.presented++;

            IDirect3DDevice9_Release(device);
            IDirect3DSurface9_Release(backbuffer);
        }
        else
            WARN("Failed to get a backbuffer, hr %#lx.\n", hr);
    }

    IMFSample_AddRef(sample);
    if ((sample = InterlockedExchangePointer((void **)&queue->last_presented, sample)))
        IMFSample_Release(sample);

    IDirect3DSurface9_Release(surface);
}

static void video_presenter_check_queue(struct video_presenter *presenter,
        unsigned int *next_wait)
{
    LONGLONG pts, clocktime, delta;
    unsigned int wait = 0;
    IMFSample *sample;
    MFTIME systime;
    BOOL present;
    HRESULT hr;

    while (video_presenter_sample_queue_pop(presenter, &sample))
    {
        present = TRUE;
        wait = 0;

        if (presenter->clock)
        {
            pts = clocktime = 0;

            hr = IMFSample_GetSampleTime(sample, &pts);
            if (SUCCEEDED(hr))
                hr = IMFClock_GetCorrelatedTime(presenter->clock, 0, &clocktime, &systime);

            delta = pts - clocktime;
            if (delta > 3 * presenter->frame_time_threshold)
            {
                /* Convert 100ns -> msec */
                wait = (delta - 3 * presenter->frame_time_threshold) / 10000;
                present = FALSE;
            }
        }

        if (present)
            video_presenter_sample_present(presenter, sample);
        else
            video_presenter_sample_queue_push(presenter, sample, TRUE);

        IMFSample_Release(sample);

        if (wait > 0)
            break;
    }

    if (!wait)
        wait = INFINITE;

    *next_wait = wait;
}

static void video_presenter_schedule_sample(struct video_presenter *presenter, IMFSample *sample)
{
    if (!presenter->thread.tid)
    {
        WARN("Streaming thread hasn't been started.\n");
        return;
    }

    if (presenter->clock)
    {
        video_presenter_sample_queue_push(presenter, sample, FALSE);
        PostThreadMessageW(presenter->thread.tid, EVRM_PRESENT, 0, 0);
    }
    else
    {
        video_presenter_sample_present(presenter, sample);
    }
}

static HRESULT video_presenter_process_input(struct video_presenter *presenter)
{
    MFT_OUTPUT_DATA_BUFFER buffer;
    HRESULT hr = S_OK;
    IMFSample *sample;
    DWORD status;

    if (presenter->state == PRESENTER_STATE_SHUT_DOWN)
        return MF_E_SHUTDOWN;

    if (!presenter->media_type)
        return S_OK;

    while (hr == S_OK)
    {
        LONGLONG mixing_started, mixing_finished;
        MFTIME systime;

        if (!(presenter->flags & PRESENTER_MIXER_HAS_INPUT))
            break;

        if (FAILED(hr = IMFVideoSampleAllocator_AllocateSample(presenter->allocator, &sample)))
        {
            WARN("Failed to allocate a sample, hr %#lx.\n", hr);
            break;
        }

        memset(&buffer, 0, sizeof(buffer));
        buffer.pSample = sample;

        if (presenter->clock)
            IMFClock_GetCorrelatedTime(presenter->clock, 0, &mixing_started, &systime);

        if (FAILED(hr = IMFTransform_ProcessOutput(presenter->mixer, 0, 1, &buffer, &status)))
        {
            /* FIXME: failure path probably needs to handle some errors specifically */
            presenter->flags &= ~PRESENTER_MIXER_HAS_INPUT;
            IMFSample_Release(sample);
            break;
        }
        else
        {
            if (presenter->clock)
            {
                LONGLONG latency;

                IMFClock_GetCorrelatedTime(presenter->clock, 0, &mixing_finished, &systime);
                latency = mixing_finished - mixing_started;
                video_presenter_notify_renderer(presenter, EC_PROCESSING_LATENCY, (LONG_PTR)&latency, 0);
            }

            if (buffer.pEvents)
                IMFCollection_Release(buffer.pEvents);

            video_presenter_schedule_sample(presenter, sample);

            IMFSample_Release(sample);
        }
    }

    return S_OK;
}

static DWORD CALLBACK video_presenter_streaming_thread(void *arg)
{
    struct video_presenter *presenter = arg;
    unsigned int wait = INFINITE;
    BOOL stop_thread = FALSE;
    MSG msg;

    PeekMessageW(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    SetEvent(presenter->thread.ready_event);

    while (!stop_thread)
    {
        if (MsgWaitForMultipleObjects(0, NULL, FALSE, wait, QS_POSTMESSAGE) == WAIT_TIMEOUT)
            video_presenter_check_queue(presenter, &wait);

        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            BOOL peek = TRUE;

            switch (msg.message)
            {
                case EVRM_STOP:
                    stop_thread = TRUE;
                    break;

                case EVRM_PRESENT:
                    if (peek)
                    {
                        video_presenter_check_queue(presenter, &wait);
                        peek = wait != INFINITE;
                    }
                    break;

                default:
                    ;
            }
        }
    }

    return 0;
}

static HRESULT video_presenter_start_streaming(struct video_presenter *presenter)
{
    HRESULT hr;

    if (presenter->state == PRESENTER_STATE_SHUT_DOWN)
        return MF_E_SHUTDOWN;

    if (presenter->thread.hthread)
        return S_OK;

    if (FAILED(hr = video_presenter_sample_queue_init(presenter)))
        return hr;

    if (!(presenter->thread.ready_event = CreateEventW(NULL, FALSE, FALSE, NULL)))
        return HRESULT_FROM_WIN32(GetLastError());

    if (!(presenter->thread.hthread = CreateThread(NULL, 0, video_presenter_streaming_thread,
            presenter, 0, &presenter->thread.tid)))
    {
        WARN("Failed to create streaming thread.\n");
        CloseHandle(presenter->thread.ready_event);
        presenter->thread.ready_event = NULL;
        return E_FAIL;
    }

    video_presenter_set_allocator_callback(presenter, &presenter->allocator_cb);

    WaitForSingleObject(presenter->thread.ready_event, INFINITE);
    CloseHandle(presenter->thread.ready_event);
    presenter->thread.ready_event = NULL;

    TRACE("Started streaming thread, tid %#lx.\n", presenter->thread.tid);

    return S_OK;
}

static HRESULT video_presenter_end_streaming(struct video_presenter *presenter)
{
    struct sample_queue *queue = &presenter->thread.queue;
    IMFSample *sample;

    if (!presenter->thread.hthread)
        return S_OK;

    PostThreadMessageW(presenter->thread.tid, EVRM_STOP, 0, 0);

    WaitForSingleObject(presenter->thread.hthread, INFINITE);
    CloseHandle(presenter->thread.hthread);

    TRACE("Terminated streaming thread tid %#lx.\n", presenter->thread.tid);

    if ((sample = InterlockedExchangePointer((void **)&queue->last_presented, NULL)))
        IMFSample_Release(sample);

    video_presenter_sample_queue_free(presenter);
    memset(&presenter->thread, 0, sizeof(presenter->thread));
    video_presenter_set_allocator_callback(presenter, NULL);

    return S_OK;
}

static HRESULT WINAPI video_presenter_inner_QueryInterface(IUnknown *iface, REFIID riid, void **obj)
{
    struct video_presenter *presenter = impl_from_IUnknown(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
    }
    else if (IsEqualIID(riid, &IID_IMFClockStateSink)
            || IsEqualIID(riid, &IID_IMFVideoPresenter))
    {
        *obj = &presenter->IMFVideoPresenter_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFVideoDeviceID))
    {
        *obj = &presenter->IMFVideoDeviceID_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFTopologyServiceLookupClient))
    {
        *obj = &presenter->IMFTopologyServiceLookupClient_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFVideoDisplayControl))
    {
        *obj = &presenter->IMFVideoDisplayControl_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFRateSupport))
    {
        *obj = &presenter->IMFRateSupport_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFGetService))
    {
        *obj = &presenter->IMFGetService_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFVideoPositionMapper))
    {
        *obj = &presenter->IMFVideoPositionMapper_iface;
    }
    else if (IsEqualIID(riid, &IID_IQualProp))
    {
        *obj = &presenter->IQualProp_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFQualityAdvise))
    {
        *obj = &presenter->IMFQualityAdvise_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFQualityAdviseLimits))
    {
        *obj = &presenter->IMFQualityAdviseLimits_iface;
    }
    else if (IsEqualIID(riid, &IID_IDirect3DDeviceManager9))
    {
        *obj = &presenter->IDirect3DDeviceManager9_iface;
    }
    else
    {
        WARN("Unimplemented interface %s.\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);
    return S_OK;
}

static ULONG WINAPI video_presenter_inner_AddRef(IUnknown *iface)
{
    struct video_presenter *presenter = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&presenter->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static void video_presenter_clear_container(struct video_presenter *presenter)
{
    if (presenter->clock)
        IMFClock_Release(presenter->clock);
    if (presenter->mixer)
        IMFTransform_Release(presenter->mixer);
    if (presenter->event_sink)
        IMediaEventSink_Release(presenter->event_sink);
    presenter->clock = NULL;
    presenter->mixer = NULL;
    presenter->event_sink = NULL;
}

static ULONG WINAPI video_presenter_inner_Release(IUnknown *iface)
{
    struct video_presenter *presenter = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&presenter->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        video_presenter_end_streaming(presenter);
        video_presenter_clear_container(presenter);
        video_presenter_reset_media_type(presenter);
        DeleteCriticalSection(&presenter->cs);
        if (presenter->swapchain)
            IDirect3DSwapChain9_Release(presenter->swapchain);
        if (presenter->device_manager)
        {
            IDirect3DDeviceManager9_CloseDeviceHandle(presenter->device_manager, presenter->hdevice);
            IDirect3DDeviceManager9_Release(presenter->device_manager);
        }
        if (presenter->allocator)
            IMFVideoSampleAllocator_Release(presenter->allocator);
        free(presenter);
    }

    return refcount;
}

static const IUnknownVtbl video_presenter_inner_vtbl =
{
    video_presenter_inner_QueryInterface,
    video_presenter_inner_AddRef,
    video_presenter_inner_Release,
};

static HRESULT WINAPI video_presenter_QueryInterface(IMFVideoPresenter *iface, REFIID riid, void **obj)
{
    struct video_presenter *presenter = impl_from_IMFVideoPresenter(iface);
    return IUnknown_QueryInterface(presenter->outer_unk, riid, obj);
}

static ULONG WINAPI video_presenter_AddRef(IMFVideoPresenter *iface)
{
    struct video_presenter *presenter = impl_from_IMFVideoPresenter(iface);
    return IUnknown_AddRef(presenter->outer_unk);
}

static ULONG WINAPI video_presenter_Release(IMFVideoPresenter *iface)
{
    struct video_presenter *presenter = impl_from_IMFVideoPresenter(iface);
    return IUnknown_Release(presenter->outer_unk);
}

static HRESULT WINAPI video_presenter_OnClockStart(IMFVideoPresenter *iface, MFTIME systime, LONGLONG offset)
{
    struct video_presenter *presenter = impl_from_IMFVideoPresenter(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_time(systime), wine_dbgstr_longlong(offset));

    EnterCriticalSection(&presenter->cs);
    presenter->state = PRESENTER_STATE_STARTED;
    LeaveCriticalSection(&presenter->cs);

    return S_OK;
}

static HRESULT WINAPI video_presenter_OnClockStop(IMFVideoPresenter *iface, MFTIME systime)
{
    struct video_presenter *presenter = impl_from_IMFVideoPresenter(iface);

    TRACE("%p, %s.\n", iface, debugstr_time(systime));

    EnterCriticalSection(&presenter->cs);
    presenter->state = PRESENTER_STATE_STOPPED;
    presenter->frame_stats.presented = 0;
    LeaveCriticalSection(&presenter->cs);

    return S_OK;
}

static HRESULT WINAPI video_presenter_OnClockPause(IMFVideoPresenter *iface, MFTIME systime)
{
    struct video_presenter *presenter = impl_from_IMFVideoPresenter(iface);

    TRACE("%p, %s.\n", iface, debugstr_time(systime));

    EnterCriticalSection(&presenter->cs);
    presenter->state = PRESENTER_STATE_PAUSED;
    LeaveCriticalSection(&presenter->cs);

    return S_OK;
}

static HRESULT WINAPI video_presenter_OnClockRestart(IMFVideoPresenter *iface, MFTIME systime)
{
    struct video_presenter *presenter = impl_from_IMFVideoPresenter(iface);

    TRACE("%p, %s.\n", iface, debugstr_time(systime));

    EnterCriticalSection(&presenter->cs);
    presenter->state = PRESENTER_STATE_STARTED;
    LeaveCriticalSection(&presenter->cs);

    return S_OK;
}

static HRESULT WINAPI video_presenter_OnClockSetRate(IMFVideoPresenter *iface, MFTIME systime, float rate)
{
    FIXME("%p, %s, %f.\n", iface, debugstr_time(systime), rate);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_presenter_ProcessMessage(IMFVideoPresenter *iface, MFVP_MESSAGE_TYPE message, ULONG_PTR param)
{
    struct video_presenter *presenter = impl_from_IMFVideoPresenter(iface);
    HRESULT hr;

    TRACE("%p, %d, %Iu.\n", iface, message, param);

    EnterCriticalSection(&presenter->cs);

    switch (message)
    {
        case MFVP_MESSAGE_INVALIDATEMEDIATYPE:
            if (presenter->state == PRESENTER_STATE_SHUT_DOWN)
                hr = MF_E_SHUTDOWN;
            else if (!presenter->mixer)
                hr = MF_E_INVALIDREQUEST;
            else
                hr = video_presenter_invalidate_media_type(presenter);
            break;
        case MFVP_MESSAGE_BEGINSTREAMING:
            hr = video_presenter_start_streaming(presenter);
            break;
        case MFVP_MESSAGE_ENDSTREAMING:
            hr = video_presenter_end_streaming(presenter);
            break;
        case MFVP_MESSAGE_PROCESSINPUTNOTIFY:
            presenter->flags |= PRESENTER_MIXER_HAS_INPUT;
            hr = video_presenter_process_input(presenter);
            break;
        default:
            FIXME("Unsupported message %u.\n", message);
            hr = E_NOTIMPL;
    }

    LeaveCriticalSection(&presenter->cs);

    return hr;
}

static HRESULT WINAPI video_presenter_GetCurrentMediaType(IMFVideoPresenter *iface,
        IMFVideoMediaType **media_type)
{
    struct video_presenter *presenter = impl_from_IMFVideoPresenter(iface);
    HRESULT hr;

    TRACE("%p, %p.\n", iface, media_type);

    EnterCriticalSection(&presenter->cs);

    if (presenter->state == PRESENTER_STATE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (!presenter->media_type)
        hr = MF_E_NOT_INITIALIZED;
    else
    {
        hr = IMFMediaType_QueryInterface(presenter->media_type, &IID_IMFVideoMediaType,
                (void **)media_type);
    }

    LeaveCriticalSection(&presenter->cs);

    return hr;
}

static const IMFVideoPresenterVtbl video_presenter_vtbl =
{
    video_presenter_QueryInterface,
    video_presenter_AddRef,
    video_presenter_Release,
    video_presenter_OnClockStart,
    video_presenter_OnClockStop,
    video_presenter_OnClockPause,
    video_presenter_OnClockRestart,
    video_presenter_OnClockSetRate,
    video_presenter_ProcessMessage,
    video_presenter_GetCurrentMediaType,
};

static HRESULT WINAPI video_presenter_device_id_QueryInterface(IMFVideoDeviceID *iface, REFIID riid, void **obj)
{
    struct video_presenter *presenter = impl_from_IMFVideoDeviceID(iface);
    return IMFVideoPresenter_QueryInterface(&presenter->IMFVideoPresenter_iface, riid, obj);
}

static ULONG WINAPI video_presenter_device_id_AddRef(IMFVideoDeviceID *iface)
{
    struct video_presenter *presenter = impl_from_IMFVideoDeviceID(iface);
    return IMFVideoPresenter_AddRef(&presenter->IMFVideoPresenter_iface);
}

static ULONG WINAPI video_presenter_device_id_Release(IMFVideoDeviceID *iface)
{
    struct video_presenter *presenter = impl_from_IMFVideoDeviceID(iface);
    return IMFVideoPresenter_Release(&presenter->IMFVideoPresenter_iface);
}

static HRESULT WINAPI video_presenter_device_id_GetDeviceID(IMFVideoDeviceID *iface, IID *device_id)
{
    TRACE("%p, %p.\n", iface, device_id);

    if (!device_id)
        return E_POINTER;

    memcpy(device_id, &IID_IDirect3DDevice9, sizeof(*device_id));

    return S_OK;
}

static const IMFVideoDeviceIDVtbl video_presenter_device_id_vtbl =
{
    video_presenter_device_id_QueryInterface,
    video_presenter_device_id_AddRef,
    video_presenter_device_id_Release,
    video_presenter_device_id_GetDeviceID,
};

static HRESULT WINAPI video_presenter_service_client_QueryInterface(IMFTopologyServiceLookupClient *iface,
        REFIID riid, void **obj)
{
    struct video_presenter *presenter = impl_from_IMFTopologyServiceLookupClient(iface);
    return IMFVideoPresenter_QueryInterface(&presenter->IMFVideoPresenter_iface, riid, obj);
}

static ULONG WINAPI video_presenter_service_client_AddRef(IMFTopologyServiceLookupClient *iface)
{
    struct video_presenter *presenter = impl_from_IMFTopologyServiceLookupClient(iface);
    return IMFVideoPresenter_AddRef(&presenter->IMFVideoPresenter_iface);
}

static ULONG WINAPI video_presenter_service_client_Release(IMFTopologyServiceLookupClient *iface)
{
    struct video_presenter *presenter = impl_from_IMFTopologyServiceLookupClient(iface);
    return IMFVideoPresenter_Release(&presenter->IMFVideoPresenter_iface);
}

static void video_presenter_set_mixer_rect(struct video_presenter *presenter)
{
    IMFAttributes *attributes;
    HRESULT hr;

    if (!presenter->mixer)
        return;

    if (SUCCEEDED(IMFTransform_GetAttributes(presenter->mixer, &attributes)))
    {
        if (FAILED(hr = IMFAttributes_SetBlob(attributes, &VIDEO_ZOOM_RECT, (const UINT8 *)&presenter->src_rect,
                sizeof(presenter->src_rect))))
        {
            WARN("Failed to set zoom rectangle attribute, hr %#lx.\n", hr);
        }
        IMFAttributes_Release(attributes);
    }
}

static HRESULT video_presenter_attach_mixer(struct video_presenter *presenter, IMFTopologyServiceLookup *service_lookup)
{
    IMFVideoDeviceID *device_id;
    GUID id = { 0 };
    DWORD count;
    HRESULT hr;

    count = 1;
    if (FAILED(hr = IMFTopologyServiceLookup_LookupService(service_lookup, MF_SERVICE_LOOKUP_GLOBAL, 0,
            &MR_VIDEO_MIXER_SERVICE, &IID_IMFTransform, (void **)&presenter->mixer, &count)))
    {
        WARN("Failed to get mixer interface, hr %#lx.\n", hr);
        return hr;
    }

    if (SUCCEEDED(hr = IMFTransform_QueryInterface(presenter->mixer, &IID_IMFVideoDeviceID, (void **)&device_id)))
    {
        if (SUCCEEDED(hr = IMFVideoDeviceID_GetDeviceID(device_id, &id)))
        {
            if (!IsEqualGUID(&id, &IID_IDirect3DDevice9))
                hr = MF_E_INVALIDREQUEST;
        }

        IMFVideoDeviceID_Release(device_id);
    }

    if (FAILED(hr))
    {
        IMFTransform_Release(presenter->mixer);
        presenter->mixer = NULL;
    }

    video_presenter_set_mixer_rect(presenter);
    video_presenter_get_native_video_size(presenter);

    return hr;
}

static HRESULT WINAPI video_presenter_service_client_InitServicePointers(IMFTopologyServiceLookupClient *iface,
        IMFTopologyServiceLookup *service_lookup)
{
    struct video_presenter *presenter = impl_from_IMFTopologyServiceLookupClient(iface);
    HRESULT hr = S_OK;
    DWORD count;

    TRACE("%p, %p.\n", iface, service_lookup);

    if (!service_lookup)
        return E_POINTER;

    EnterCriticalSection(&presenter->cs);

    if (presenter->state == PRESENTER_STATE_STARTED ||
            presenter->state == PRESENTER_STATE_PAUSED)
    {
        hr = MF_E_INVALIDREQUEST;
    }
    else
    {
        video_presenter_clear_container(presenter);

        count = 1;
        IMFTopologyServiceLookup_LookupService(service_lookup, MF_SERVICE_LOOKUP_GLOBAL, 0,
                &MR_VIDEO_RENDER_SERVICE, &IID_IMFClock, (void **)&presenter->clock, &count);

        hr = video_presenter_attach_mixer(presenter, service_lookup);

        if (SUCCEEDED(hr))
        {
            count = 1;
            if (FAILED(hr = IMFTopologyServiceLookup_LookupService(service_lookup, MF_SERVICE_LOOKUP_GLOBAL, 0,
                    &MR_VIDEO_RENDER_SERVICE, &IID_IMediaEventSink, (void **)&presenter->event_sink, &count)))
            {
                WARN("Failed to get renderer event sink, hr %#lx.\n", hr);
            }
        }

        if (SUCCEEDED(hr))
            presenter->state = PRESENTER_STATE_STOPPED;
    }

    LeaveCriticalSection(&presenter->cs);

    return hr;
}

static HRESULT WINAPI video_presenter_service_client_ReleaseServicePointers(IMFTopologyServiceLookupClient *iface)
{
    struct video_presenter *presenter = impl_from_IMFTopologyServiceLookupClient(iface);

    TRACE("%p.\n", iface);

    EnterCriticalSection(&presenter->cs);

    presenter->state = PRESENTER_STATE_SHUT_DOWN;
    video_presenter_clear_container(presenter);

    LeaveCriticalSection(&presenter->cs);

    return S_OK;
}

static const IMFTopologyServiceLookupClientVtbl video_presenter_service_client_vtbl =
{
    video_presenter_service_client_QueryInterface,
    video_presenter_service_client_AddRef,
    video_presenter_service_client_Release,
    video_presenter_service_client_InitServicePointers,
    video_presenter_service_client_ReleaseServicePointers,
};

static HRESULT WINAPI video_presenter_control_QueryInterface(IMFVideoDisplayControl *iface, REFIID riid, void **obj)
{
    struct video_presenter *presenter = impl_from_IMFVideoDisplayControl(iface);
    return IMFVideoPresenter_QueryInterface(&presenter->IMFVideoPresenter_iface, riid, obj);
}

static ULONG WINAPI video_presenter_control_AddRef(IMFVideoDisplayControl *iface)
{
    struct video_presenter *presenter = impl_from_IMFVideoDisplayControl(iface);
    return IMFVideoPresenter_AddRef(&presenter->IMFVideoPresenter_iface);
}

static ULONG WINAPI video_presenter_control_Release(IMFVideoDisplayControl *iface)
{
    struct video_presenter *presenter = impl_from_IMFVideoDisplayControl(iface);
    return IMFVideoPresenter_Release(&presenter->IMFVideoPresenter_iface);
}

static HRESULT WINAPI video_presenter_control_GetNativeVideoSize(IMFVideoDisplayControl *iface, SIZE *video_size,
        SIZE *aspect_ratio)
{
    struct video_presenter *presenter = impl_from_IMFVideoDisplayControl(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p, %p.\n", iface, video_size, aspect_ratio);

    if (!video_size && !aspect_ratio)
        return E_POINTER;

    EnterCriticalSection(&presenter->cs);

    if (presenter->state == PRESENTER_STATE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
    {
        if (video_size)
            *video_size = presenter->native_size;
        if (aspect_ratio)
            *aspect_ratio = presenter->native_ratio;
    }

    LeaveCriticalSection(&presenter->cs);

    return hr;
}

static HRESULT WINAPI video_presenter_control_GetIdealVideoSize(IMFVideoDisplayControl *iface, SIZE *min_size,
        SIZE *max_size)
{
    struct video_presenter *presenter = impl_from_IMFVideoDisplayControl(iface);
    HRESULT hr;

    FIXME("%p, %p, %p.\n", iface, min_size, max_size);

    EnterCriticalSection(&presenter->cs);

    if (presenter->state == PRESENTER_STATE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
        hr = E_NOTIMPL;

    LeaveCriticalSection(&presenter->cs);

    return hr;
}

static HRESULT WINAPI video_presenter_control_SetVideoPosition(IMFVideoDisplayControl *iface,
        const MFVideoNormalizedRect *src_rect, const RECT *dst_rect)
{
    struct video_presenter *presenter = impl_from_IMFVideoDisplayControl(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %s, %s.\n", iface, debugstr_normalized_rect(src_rect), wine_dbgstr_rect(dst_rect));

    if (!src_rect && !dst_rect)
        return E_POINTER;

    if (src_rect && (src_rect->left < 0.0f || src_rect->top < 0.0f ||
            src_rect->right > 1.0f || src_rect->bottom > 1.0f ||
            src_rect->left > src_rect->right ||
            src_rect->top > src_rect->bottom))
    {
        return E_INVALIDARG;
    }

    if (dst_rect && (dst_rect->left > dst_rect->right ||
            dst_rect->top > dst_rect->bottom))
        return E_INVALIDARG;

    EnterCriticalSection(&presenter->cs);
    if (!presenter->video_window)
        hr = E_POINTER;
    else
    {
        if (src_rect)
        {
            if (memcmp(&presenter->src_rect, src_rect, sizeof(*src_rect)))
            {
                presenter->src_rect = *src_rect;
                video_presenter_set_mixer_rect(presenter);
            }
        }
        if (dst_rect && !EqualRect(dst_rect, &presenter->dst_rect))
        {
            presenter->dst_rect = *dst_rect;
            hr = video_presenter_invalidate_media_type(presenter);
            /* Mixer's input type hasn't been configured yet, this is not an error. */
            if (hr == MF_E_TRANSFORM_TYPE_NOT_SET) hr = S_OK;
            /* FIXME: trigger repaint */
        }
    }
    LeaveCriticalSection(&presenter->cs);

    return hr;
}

static HRESULT WINAPI video_presenter_control_GetVideoPosition(IMFVideoDisplayControl *iface, MFVideoNormalizedRect *src_rect,
        RECT *dst_rect)
{
    struct video_presenter *presenter = impl_from_IMFVideoDisplayControl(iface);

    TRACE("%p, %p, %p.\n", iface, src_rect, dst_rect);

    if (!src_rect || !dst_rect)
        return E_POINTER;

    EnterCriticalSection(&presenter->cs);
    *src_rect = presenter->src_rect;
    *dst_rect = presenter->dst_rect;
    LeaveCriticalSection(&presenter->cs);

    return S_OK;
}

static HRESULT WINAPI video_presenter_control_SetAspectRatioMode(IMFVideoDisplayControl *iface, DWORD mode)
{
    struct video_presenter *presenter = impl_from_IMFVideoDisplayControl(iface);

    TRACE("%p, %#lx.\n", iface, mode);

    if (mode & ~MFVideoARMode_Mask)
        return E_INVALIDARG;

    EnterCriticalSection(&presenter->cs);
    presenter->ar_mode = mode;
    LeaveCriticalSection(&presenter->cs);

    return S_OK;
}

static HRESULT WINAPI video_presenter_control_GetAspectRatioMode(IMFVideoDisplayControl *iface, DWORD *mode)
{
    struct video_presenter *presenter = impl_from_IMFVideoDisplayControl(iface);

    TRACE("%p, %p.\n", iface, mode);

    if (!mode)
        return E_POINTER;

    EnterCriticalSection(&presenter->cs);
    *mode = presenter->ar_mode;
    LeaveCriticalSection(&presenter->cs);

    return S_OK;
}

static HRESULT video_presenter_create_swapchain(struct video_presenter *presenter)
{
    D3DPRESENT_PARAMETERS present_params = { 0 };
    IDirect3DDevice9 *d3d_device;
    HRESULT hr;

    if (SUCCEEDED(hr = video_presenter_get_device(presenter, &d3d_device)))
    {
        present_params.hDeviceWindow = presenter->video_window;
        present_params.Windowed = TRUE;
        present_params.SwapEffect = D3DSWAPEFFECT_COPY;
        present_params.Flags = D3DPRESENTFLAG_VIDEO;
        present_params.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
        hr = IDirect3DDevice9_CreateAdditionalSwapChain(d3d_device, &present_params, &presenter->swapchain);

        IDirect3DDevice9_Release(d3d_device);
        IDirect3DDeviceManager9_UnlockDevice(presenter->device_manager, presenter->hdevice, FALSE);
    }

    return hr;
}

static HRESULT WINAPI video_presenter_control_SetVideoWindow(IMFVideoDisplayControl *iface, HWND window)
{
    struct video_presenter *presenter = impl_from_IMFVideoDisplayControl(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, window);

    if (!IsWindow(window))
        return E_INVALIDARG;

    EnterCriticalSection(&presenter->cs);
    if (presenter->video_window != window)
    {
        if (presenter->swapchain)
            IDirect3DSwapChain9_Release(presenter->swapchain);
        presenter->video_window = window;
        hr = video_presenter_create_swapchain(presenter);
    }
    LeaveCriticalSection(&presenter->cs);

    return hr;
}

static HRESULT WINAPI video_presenter_control_GetVideoWindow(IMFVideoDisplayControl *iface, HWND *window)
{
    struct video_presenter *presenter = impl_from_IMFVideoDisplayControl(iface);

    TRACE("%p, %p.\n", iface, window);

    if (!window)
        return E_POINTER;

    EnterCriticalSection(&presenter->cs);
    *window = presenter->video_window;
    LeaveCriticalSection(&presenter->cs);

    return S_OK;
}

static HRESULT WINAPI video_presenter_control_RepaintVideo(IMFVideoDisplayControl *iface)
{
    struct video_presenter *presenter = impl_from_IMFVideoDisplayControl(iface);
    HRESULT hr;

    FIXME("%p.\n", iface);

    EnterCriticalSection(&presenter->cs);

    if (presenter->state == PRESENTER_STATE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
        hr = E_NOTIMPL;

    LeaveCriticalSection(&presenter->cs);

    return hr;
}

static HRESULT WINAPI video_presenter_control_GetCurrentImage(IMFVideoDisplayControl *iface, BITMAPINFOHEADER *header,
        BYTE **dib, DWORD *dib_size, LONGLONG *timestamp)
{
    struct video_presenter *presenter = impl_from_IMFVideoDisplayControl(iface);
    struct sample_queue *queue = &presenter->thread.queue;
    IDirect3DSurface9 *readback = NULL, *surface;
    D3DSURFACE_DESC surface_desc;
    D3DLOCKED_RECT mapped_rect;
    IDirect3DDevice9 *device;
    IMFSample *sample;
    LONG stride;
    HRESULT hr;

    TRACE("%p, %p, %p, %p, %p.\n", iface, header, dib, dib_size, timestamp);

    EnterCriticalSection(&presenter->cs);

    if (!(sample = InterlockedExchangePointer((void **)&queue->last_presented, NULL)))
    {
        hr = MF_E_INVALIDREQUEST;
    }
    else if (SUCCEEDED(hr = video_presenter_get_sample_surface(sample, &surface)))
    {
        IDirect3DSurface9_GetDevice(surface, &device);
        IDirect3DSurface9_GetDesc(surface, &surface_desc);

        if (surface_desc.Format != D3DFMT_X8R8G8B8)
        {
            FIXME("Unexpected surface format %d.\n", surface_desc.Format);
            hr = E_FAIL;
        }

        if (SUCCEEDED(hr))
        {
            if (FAILED(hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, surface_desc.Width,
                    surface_desc.Height, D3DFMT_X8R8G8B8, D3DPOOL_SYSTEMMEM, &readback, NULL)))
            {
                WARN("Failed to create readback surface, hr %#lx.\n", hr);
            }
        }

        if (SUCCEEDED(hr))
            hr = IDirect3DDevice9_GetRenderTargetData(device, surface, readback);

        if (SUCCEEDED(hr))
        {
            MFGetStrideForBitmapInfoHeader(D3DFMT_X8R8G8B8, surface_desc.Width, &stride);
            *dib_size = abs(stride) * surface_desc.Height;
            if (!(*dib = CoTaskMemAlloc(*dib_size)))
                hr = E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hr))
        {
            if (SUCCEEDED(hr = IDirect3DSurface9_LockRect(readback, &mapped_rect, NULL, D3DLOCK_READONLY)))
            {
                hr = MFCopyImage(stride < 0 ? *dib + *dib_size + stride : *dib, stride,
                        mapped_rect.pBits, mapped_rect.Pitch, abs(stride), surface_desc.Height);
                IDirect3DSurface9_UnlockRect(readback);
            }
        }

        memset(header, 0, sizeof(*header));
        header->biSize = sizeof(*header);
        header->biWidth = surface_desc.Width;
        header->biHeight = surface_desc.Height;
        header->biPlanes = 1;
        header->biBitCount = 32;
        header->biSizeImage = *dib_size;
        IMFSample_GetSampleTime(sample, timestamp);

        if (readback)
            IDirect3DSurface9_Release(readback);
        IDirect3DSurface9_Release(surface);

        IDirect3DDevice9_Release(device);
    }

    if (sample)
        IMFSample_Release(sample);

    LeaveCriticalSection(&presenter->cs);

    return hr;
}

static HRESULT WINAPI video_presenter_control_SetBorderColor(IMFVideoDisplayControl *iface, COLORREF color)
{
    FIXME("%p, %#lx.\n", iface, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_presenter_control_GetBorderColor(IMFVideoDisplayControl *iface, COLORREF *color)
{
    FIXME("%p, %p.\n", iface, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_presenter_control_SetRenderingPrefs(IMFVideoDisplayControl *iface, DWORD flags)
{
    FIXME("%p, %#lx.\n", iface, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_presenter_control_GetRenderingPrefs(IMFVideoDisplayControl *iface, DWORD *flags)
{
    struct video_presenter *presenter = impl_from_IMFVideoDisplayControl(iface);

    TRACE("%p, %p.\n", iface, flags);

    if (!flags)
        return E_POINTER;

    EnterCriticalSection(&presenter->cs);
    *flags = presenter->rendering_prefs;
    LeaveCriticalSection(&presenter->cs);

    return S_OK;
}

static HRESULT WINAPI video_presenter_control_SetFullscreen(IMFVideoDisplayControl *iface, BOOL fullscreen)
{
    FIXME("%p, %d.\n", iface, fullscreen);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_presenter_control_GetFullscreen(IMFVideoDisplayControl *iface, BOOL *fullscreen)
{
    FIXME("%p, %p.\n", iface, fullscreen);

    return E_NOTIMPL;
}

static const IMFVideoDisplayControlVtbl video_presenter_control_vtbl =
{
    video_presenter_control_QueryInterface,
    video_presenter_control_AddRef,
    video_presenter_control_Release,
    video_presenter_control_GetNativeVideoSize,
    video_presenter_control_GetIdealVideoSize,
    video_presenter_control_SetVideoPosition,
    video_presenter_control_GetVideoPosition,
    video_presenter_control_SetAspectRatioMode,
    video_presenter_control_GetAspectRatioMode,
    video_presenter_control_SetVideoWindow,
    video_presenter_control_GetVideoWindow,
    video_presenter_control_RepaintVideo,
    video_presenter_control_GetCurrentImage,
    video_presenter_control_SetBorderColor,
    video_presenter_control_GetBorderColor,
    video_presenter_control_SetRenderingPrefs,
    video_presenter_control_GetRenderingPrefs,
    video_presenter_control_SetFullscreen,
    video_presenter_control_GetFullscreen,
};

static HRESULT WINAPI video_presenter_rate_support_QueryInterface(IMFRateSupport *iface, REFIID riid, void **obj)
{
    struct video_presenter *presenter = impl_from_IMFRateSupport(iface);
    return IMFVideoPresenter_QueryInterface(&presenter->IMFVideoPresenter_iface, riid, obj);
}

static ULONG WINAPI video_presenter_rate_support_AddRef(IMFRateSupport *iface)
{
    struct video_presenter *presenter = impl_from_IMFRateSupport(iface);
    return IMFVideoPresenter_AddRef(&presenter->IMFVideoPresenter_iface);
}

static ULONG WINAPI video_presenter_rate_support_Release(IMFRateSupport *iface)
{
    struct video_presenter *presenter = impl_from_IMFRateSupport(iface);
    return IMFVideoPresenter_Release(&presenter->IMFVideoPresenter_iface);
}

static HRESULT WINAPI video_presenter_rate_support_GetSlowestRate(IMFRateSupport *iface, MFRATE_DIRECTION direction,
        BOOL thin, float *rate)
{
    TRACE("%p, %d, %d, %p.\n", iface, direction, thin, rate);

    *rate = 0.0f;

    return S_OK;
}

static HRESULT WINAPI video_presenter_rate_support_GetFastestRate(IMFRateSupport *iface, MFRATE_DIRECTION direction,
        BOOL thin, float *rate)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI video_presenter_rate_support_IsRateSupported(IMFRateSupport *iface, BOOL thin, float rate,
        float *nearest_supported_rate)
{
    return E_NOTIMPL;
}

static const IMFRateSupportVtbl video_presenter_rate_support_vtbl =
{
    video_presenter_rate_support_QueryInterface,
    video_presenter_rate_support_AddRef,
    video_presenter_rate_support_Release,
    video_presenter_rate_support_GetSlowestRate,
    video_presenter_rate_support_GetFastestRate,
    video_presenter_rate_support_IsRateSupported,
};

static HRESULT WINAPI video_presenter_getservice_QueryInterface(IMFGetService *iface, REFIID riid, void **obj)
{
    struct video_presenter *presenter = impl_from_IMFGetService(iface);
    return IMFVideoPresenter_QueryInterface(&presenter->IMFVideoPresenter_iface, riid, obj);
}

static ULONG WINAPI video_presenter_getservice_AddRef(IMFGetService *iface)
{
    struct video_presenter *presenter = impl_from_IMFGetService(iface);
    return IMFVideoPresenter_AddRef(&presenter->IMFVideoPresenter_iface);
}

static ULONG WINAPI video_presenter_getservice_Release(IMFGetService *iface)
{
    struct video_presenter *presenter = impl_from_IMFGetService(iface);
    return IMFVideoPresenter_Release(&presenter->IMFVideoPresenter_iface);
}

static HRESULT WINAPI video_presenter_getservice_GetService(IMFGetService *iface, REFGUID service, REFIID riid, void **obj)
{
    struct video_presenter *presenter = impl_from_IMFGetService(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_guid(service), debugstr_guid(riid), obj);

    if (IsEqualGUID(&MR_VIDEO_ACCELERATION_SERVICE, service))
        return IDirect3DDeviceManager9_QueryInterface(presenter->device_manager, riid, obj);

    if (IsEqualGUID(&MR_VIDEO_RENDER_SERVICE, service))
        return IMFVideoPresenter_QueryInterface(&presenter->IMFVideoPresenter_iface, riid, obj);

    FIXME("Unimplemented service %s.\n", debugstr_guid(service));

    return MF_E_UNSUPPORTED_SERVICE;
}

static const IMFGetServiceVtbl video_presenter_getservice_vtbl =
{
    video_presenter_getservice_QueryInterface,
    video_presenter_getservice_AddRef,
    video_presenter_getservice_Release,
    video_presenter_getservice_GetService,
};

static HRESULT WINAPI video_presenter_position_mapper_QueryInterface(IMFVideoPositionMapper *iface, REFIID riid, void **obj)
{
    struct video_presenter *presenter = impl_from_IMFVideoPositionMapper(iface);
    return IMFVideoPresenter_QueryInterface(&presenter->IMFVideoPresenter_iface, riid, obj);
}

static ULONG WINAPI video_presenter_position_mapper_AddRef(IMFVideoPositionMapper *iface)
{
    struct video_presenter *presenter = impl_from_IMFVideoPositionMapper(iface);
    return IMFVideoPresenter_AddRef(&presenter->IMFVideoPresenter_iface);
}

static ULONG WINAPI video_presenter_position_mapper_Release(IMFVideoPositionMapper *iface)
{
    struct video_presenter *presenter = impl_from_IMFVideoPositionMapper(iface);
    return IMFVideoPresenter_Release(&presenter->IMFVideoPresenter_iface);
}

static HRESULT WINAPI video_presenter_position_mapper_MapOutputCoordinateToInputStream(IMFVideoPositionMapper *iface,
        float x_out, float y_out, DWORD output_stream, DWORD input_stream, float *x_in, float *y_in)
{
    FIXME("%p, %f, %f, %lu, %lu, %p, %p.\n", iface, x_out, y_out, output_stream, input_stream, x_in, y_in);

    return E_NOTIMPL;
}

static const IMFVideoPositionMapperVtbl video_presenter_position_mapper_vtbl =
{
    video_presenter_position_mapper_QueryInterface,
    video_presenter_position_mapper_AddRef,
    video_presenter_position_mapper_Release,
    video_presenter_position_mapper_MapOutputCoordinateToInputStream,
};

static HRESULT WINAPI video_presenter_allocator_cb_QueryInterface(IMFVideoSampleAllocatorNotify *iface,
        REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFVideoSampleAllocatorNotify) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFVideoSampleAllocatorNotify_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI video_presenter_allocator_cb_AddRef(IMFVideoSampleAllocatorNotify *iface)
{
    struct video_presenter *presenter = impl_from_IMFVideoSampleAllocatorNotify(iface);
    return IMFVideoPresenter_AddRef(&presenter->IMFVideoPresenter_iface);
}

static ULONG WINAPI video_presenter_allocator_cb_Release(IMFVideoSampleAllocatorNotify *iface)
{
    struct video_presenter *presenter = impl_from_IMFVideoSampleAllocatorNotify(iface);
    return IMFVideoPresenter_Release(&presenter->IMFVideoPresenter_iface);
}

static HRESULT WINAPI video_presenter_allocator_cb_NotifyRelease(IMFVideoSampleAllocatorNotify *iface)
{
    struct video_presenter *presenter = impl_from_IMFVideoSampleAllocatorNotify(iface);

    EnterCriticalSection(&presenter->cs);
    video_presenter_process_input(presenter);
    LeaveCriticalSection(&presenter->cs);

    return S_OK;
}

static const IMFVideoSampleAllocatorNotifyVtbl video_presenter_allocator_cb_vtbl =
{
    video_presenter_allocator_cb_QueryInterface,
    video_presenter_allocator_cb_AddRef,
    video_presenter_allocator_cb_Release,
    video_presenter_allocator_cb_NotifyRelease,
};

static HRESULT WINAPI video_presenter_qualprop_QueryInterface(IQualProp *iface, REFIID riid, void **obj)
{
    struct video_presenter *presenter = impl_from_IQualProp(iface);
    return IMFVideoPresenter_QueryInterface(&presenter->IMFVideoPresenter_iface, riid, obj);
}

static ULONG WINAPI video_presenter_qualprop_AddRef(IQualProp *iface)
{
    struct video_presenter *presenter = impl_from_IQualProp(iface);
    return IMFVideoPresenter_AddRef(&presenter->IMFVideoPresenter_iface);
}

static ULONG WINAPI video_presenter_qualprop_Release(IQualProp *iface)
{
    struct video_presenter *presenter = impl_from_IQualProp(iface);
    return IMFVideoPresenter_Release(&presenter->IMFVideoPresenter_iface);
}

static HRESULT WINAPI video_presenter_qualprop_get_FramesDroppedInRenderer(IQualProp *iface, int *frames)
{
    FIXME("%p, %p stub.\n", iface, frames);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_presenter_qualprop_get_FramesDrawn(IQualProp *iface, int *frames)
{
    struct video_presenter *presenter = impl_from_IQualProp(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, frames);

    EnterCriticalSection(&presenter->cs);

    switch (presenter->state)
    {
        case PRESENTER_STATE_STARTED:
        case PRESENTER_STATE_PAUSED:
            if (frames) *frames = presenter->frame_stats.presented;
            else hr = E_POINTER;
            break;
        default:
            hr = E_NOTIMPL;
    }

    LeaveCriticalSection(&presenter->cs);

    return hr;
}

static HRESULT WINAPI video_presenter_qualprop_get_AvgFrameRate(IQualProp *iface, int *avg_frame_rate)
{
    FIXME("%p, %p stub.\n", iface, avg_frame_rate);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_presenter_qualprop_get_Jitter(IQualProp *iface, int *jitter)
{
    FIXME("%p, %p stub.\n", iface, jitter);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_presenter_qualprop_get_AvgSyncOffset(IQualProp *iface, int *offset)
{
    FIXME("%p, %p stub.\n", iface, offset);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_presenter_qualprop_get_DevSyncOffset(IQualProp *iface, int *devoffset)
{
    FIXME("%p, %p stub.\n", iface, devoffset);

    return E_NOTIMPL;
}

static const IQualPropVtbl video_presenter_qualprop_vtbl =
{
    video_presenter_qualprop_QueryInterface,
    video_presenter_qualprop_AddRef,
    video_presenter_qualprop_Release,
    video_presenter_qualprop_get_FramesDroppedInRenderer,
    video_presenter_qualprop_get_FramesDrawn,
    video_presenter_qualprop_get_AvgFrameRate,
    video_presenter_qualprop_get_Jitter,
    video_presenter_qualprop_get_AvgSyncOffset,
    video_presenter_qualprop_get_DevSyncOffset,
};

static HRESULT WINAPI video_presenter_quality_advise_QueryInterface(IMFQualityAdvise *iface, REFIID riid, void **out)
{
    struct video_presenter *presenter = impl_from_IMFQualityAdvise(iface);
    return IMFVideoPresenter_QueryInterface(&presenter->IMFVideoPresenter_iface, riid, out);
}

static ULONG WINAPI video_presenter_quality_advise_AddRef(IMFQualityAdvise *iface)
{
    struct video_presenter *presenter = impl_from_IMFQualityAdvise(iface);
    return IMFVideoPresenter_AddRef(&presenter->IMFVideoPresenter_iface);
}

static ULONG WINAPI video_presenter_quality_advise_Release(IMFQualityAdvise *iface)
{
    struct video_presenter *presenter = impl_from_IMFQualityAdvise(iface);
    return IMFVideoPresenter_Release(&presenter->IMFVideoPresenter_iface);
}

static HRESULT WINAPI video_presenter_quality_advise_SetDropMode(IMFQualityAdvise *iface,
        MF_QUALITY_DROP_MODE mode)
{
    FIXME("%p, %u.\n", iface, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_presenter_quality_advise_SetQualityLevel(IMFQualityAdvise *iface,
        MF_QUALITY_LEVEL level)
{
    FIXME("%p, %u.\n", iface, level);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_presenter_quality_advise_GetDropMode(IMFQualityAdvise *iface,
        MF_QUALITY_DROP_MODE *mode)
{
    FIXME("%p, %p.\n", iface, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_presenter_quality_advise_GetQualityLevel(IMFQualityAdvise *iface,
        MF_QUALITY_LEVEL *level)
{
    FIXME("%p, %p.\n", iface, level);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_presenter_quality_advise_DropTime(IMFQualityAdvise *iface, LONGLONG interval)
{
    FIXME("%p, %s.\n", iface, wine_dbgstr_longlong(interval));

    return E_NOTIMPL;
}

static const IMFQualityAdviseVtbl video_presenter_quality_advise_vtbl =
{
    video_presenter_quality_advise_QueryInterface,
    video_presenter_quality_advise_AddRef,
    video_presenter_quality_advise_Release,
    video_presenter_quality_advise_SetDropMode,
    video_presenter_quality_advise_SetQualityLevel,
    video_presenter_quality_advise_GetDropMode,
    video_presenter_quality_advise_GetQualityLevel,
    video_presenter_quality_advise_DropTime,
};

static HRESULT WINAPI video_presenter_device_manager_QueryInterface(IDirect3DDeviceManager9 *iface,
        REFIID riid, void **obj)
{
    struct video_presenter *presenter = impl_from_IDirect3DDeviceManager9(iface);
    return IMFVideoPresenter_QueryInterface(&presenter->IMFVideoPresenter_iface, riid, obj);
}

static ULONG WINAPI video_presenter_device_manager_AddRef(IDirect3DDeviceManager9 *iface)
{
    struct video_presenter *presenter = impl_from_IDirect3DDeviceManager9(iface);
    return IMFVideoPresenter_AddRef(&presenter->IMFVideoPresenter_iface);
}

static ULONG WINAPI video_presenter_device_manager_Release(IDirect3DDeviceManager9 *iface)
{
    struct video_presenter *presenter = impl_from_IDirect3DDeviceManager9(iface);
    return IMFVideoPresenter_Release(&presenter->IMFVideoPresenter_iface);
}

static HRESULT WINAPI video_presenter_device_manager_ResetDevice(IDirect3DDeviceManager9 *iface,
        IDirect3DDevice9 *device, UINT token)
{
    struct video_presenter *presenter = impl_from_IDirect3DDeviceManager9(iface);
    return IDirect3DDeviceManager9_ResetDevice(presenter->device_manager, device, token);
}

static HRESULT WINAPI video_presenter_device_manager_OpenDeviceHandle(IDirect3DDeviceManager9 *iface, HANDLE *hdevice)
{
    struct video_presenter *presenter = impl_from_IDirect3DDeviceManager9(iface);
    return IDirect3DDeviceManager9_OpenDeviceHandle(presenter->device_manager, hdevice);
}

static HRESULT WINAPI video_presenter_device_manager_CloseDeviceHandle(IDirect3DDeviceManager9 *iface, HANDLE hdevice)
{
    struct video_presenter *presenter = impl_from_IDirect3DDeviceManager9(iface);
    return IDirect3DDeviceManager9_CloseDeviceHandle(presenter->device_manager, hdevice);
}

static HRESULT WINAPI video_presenter_device_manager_TestDevice(IDirect3DDeviceManager9 *iface, HANDLE hdevice)
{
    struct video_presenter *presenter = impl_from_IDirect3DDeviceManager9(iface);
    return IDirect3DDeviceManager9_TestDevice(presenter->device_manager, hdevice);
}

static HRESULT WINAPI video_presenter_device_manager_LockDevice(IDirect3DDeviceManager9 *iface, HANDLE hdevice,
        IDirect3DDevice9 **device, BOOL block)
{
    struct video_presenter *presenter = impl_from_IDirect3DDeviceManager9(iface);
    return IDirect3DDeviceManager9_LockDevice(presenter->device_manager, hdevice, device, block);
}

static HRESULT WINAPI video_presenter_device_manager_UnlockDevice(IDirect3DDeviceManager9 *iface, HANDLE hdevice,
        BOOL savestate)
{
    struct video_presenter *presenter = impl_from_IDirect3DDeviceManager9(iface);
    return IDirect3DDeviceManager9_UnlockDevice(presenter->device_manager, hdevice, savestate);
}

static HRESULT WINAPI video_presenter_device_manager_GetVideoService(IDirect3DDeviceManager9 *iface, HANDLE hdevice,
        REFIID riid, void **service)
{
    struct video_presenter *presenter = impl_from_IDirect3DDeviceManager9(iface);
    return IDirect3DDeviceManager9_GetVideoService(presenter->device_manager, hdevice, riid, service);
}

static const IDirect3DDeviceManager9Vtbl video_presenter_device_manager_vtbl =
{
    video_presenter_device_manager_QueryInterface,
    video_presenter_device_manager_AddRef,
    video_presenter_device_manager_Release,
    video_presenter_device_manager_ResetDevice,
    video_presenter_device_manager_OpenDeviceHandle,
    video_presenter_device_manager_CloseDeviceHandle,
    video_presenter_device_manager_TestDevice,
    video_presenter_device_manager_LockDevice,
    video_presenter_device_manager_UnlockDevice,
    video_presenter_device_manager_GetVideoService,
};

static HRESULT WINAPI video_presenter_qa_limits_QueryInterface(IMFQualityAdviseLimits *iface, REFIID riid, void **obj)
{
    struct video_presenter *presenter = impl_from_IMFQualityAdviseLimits(iface);
    return IMFVideoPresenter_QueryInterface(&presenter->IMFVideoPresenter_iface, riid, obj);
}

static ULONG WINAPI video_presenter_qa_limits_AddRef(IMFQualityAdviseLimits *iface)
{
    struct video_presenter *presenter = impl_from_IMFQualityAdviseLimits(iface);
    return IMFVideoPresenter_AddRef(&presenter->IMFVideoPresenter_iface);
}

static ULONG WINAPI video_presenter_qa_limits_Release(IMFQualityAdviseLimits *iface)
{
    struct video_presenter *presenter = impl_from_IMFQualityAdviseLimits(iface);
    return IMFVideoPresenter_Release(&presenter->IMFVideoPresenter_iface);
}

static HRESULT WINAPI video_presenter_qa_limits_GetMaximumDropMode(IMFQualityAdviseLimits *iface, MF_QUALITY_DROP_MODE *mode)
{
    FIXME("%p, %p.\n", iface, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_presenter_qa_limits_GetMinimumQualityLevel(IMFQualityAdviseLimits *iface, MF_QUALITY_LEVEL *level)
{
    FIXME("%p, %p.\n", iface, level);

    return E_NOTIMPL;
}

static const IMFQualityAdviseLimitsVtbl video_presenter_qa_limits_vtbl =
{
    video_presenter_qa_limits_QueryInterface,
    video_presenter_qa_limits_AddRef,
    video_presenter_qa_limits_Release,
    video_presenter_qa_limits_GetMaximumDropMode,
    video_presenter_qa_limits_GetMinimumQualityLevel,
};

HRESULT WINAPI MFCreateVideoPresenter(IUnknown *owner, REFIID riid_device, REFIID riid, void **obj)
{
    TRACE("%p, %s, %s, %p.\n", owner, debugstr_guid(riid_device), debugstr_guid(riid), obj);

    *obj = NULL;

    if (!IsEqualIID(riid_device, &IID_IDirect3DDevice9))
        return E_INVALIDARG;

    return CoCreateInstance(&CLSID_MFVideoPresenter9, owner, CLSCTX_INPROC_SERVER, riid, obj);
}

static HRESULT video_presenter_init_d3d(struct video_presenter *presenter)
{
    D3DPRESENT_PARAMETERS present_params = { 0 };
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    HRESULT hr;

    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d)
    {
        WARN("Failed to initialize d3d9.\n");
        return E_FAIL;
    }

    present_params.BackBufferCount = 1;
    present_params.SwapEffect = D3DSWAPEFFECT_COPY;
    present_params.hDeviceWindow = GetDesktopWindow();
    present_params.Windowed = TRUE;
    present_params.Flags = D3DPRESENTFLAG_VIDEO;
    present_params.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, GetDesktopWindow(),
            D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED, &present_params, &device);

    IDirect3D9_Release(d3d);

    if (FAILED(hr))
    {
        WARN("Failed to create d3d device, hr %#lx.\n", hr);
        return hr;
    }

    hr = IDirect3DDeviceManager9_ResetDevice(presenter->device_manager, device, presenter->reset_token);
    IDirect3DDevice9_Release(device);
    if (FAILED(hr))
        WARN("Failed to set new device for the manager, hr %#lx.\n", hr);

    if (SUCCEEDED(hr = create_video_sample_allocator(FALSE, &IID_IMFVideoSampleAllocator, (void **)&presenter->allocator)))
    {
        hr = IMFVideoSampleAllocator_SetDirectXManager(presenter->allocator, (IUnknown *)presenter->device_manager);
    }

    return hr;
}

HRESULT evr_presenter_create(IUnknown *outer, void **out)
{
    struct video_presenter *object;
    HRESULT hr;

    *out = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFVideoPresenter_iface.lpVtbl = &video_presenter_vtbl;
    object->IMFVideoDeviceID_iface.lpVtbl = &video_presenter_device_id_vtbl;
    object->IMFTopologyServiceLookupClient_iface.lpVtbl = &video_presenter_service_client_vtbl;
    object->IMFVideoDisplayControl_iface.lpVtbl = &video_presenter_control_vtbl;
    object->IMFRateSupport_iface.lpVtbl = &video_presenter_rate_support_vtbl;
    object->IMFGetService_iface.lpVtbl = &video_presenter_getservice_vtbl;
    object->IMFVideoPositionMapper_iface.lpVtbl = &video_presenter_position_mapper_vtbl;
    object->IQualProp_iface.lpVtbl = &video_presenter_qualprop_vtbl;
    object->IMFQualityAdvise_iface.lpVtbl = &video_presenter_quality_advise_vtbl;
    object->IMFQualityAdviseLimits_iface.lpVtbl = &video_presenter_qa_limits_vtbl;
    object->allocator_cb.lpVtbl = &video_presenter_allocator_cb_vtbl;
    object->IUnknown_inner.lpVtbl = &video_presenter_inner_vtbl;
    object->IDirect3DDeviceManager9_iface.lpVtbl = &video_presenter_device_manager_vtbl;
    object->outer_unk = outer ? outer : &object->IUnknown_inner;
    object->refcount = 1;
    object->src_rect.right = object->src_rect.bottom = 1.0f;
    object->ar_mode = MFVideoARMode_PreservePicture | MFVideoARMode_PreservePixel;
    object->allocator_capacity = 3;
    InitializeCriticalSection(&object->cs);

    if (FAILED(hr = DXVA2CreateDirect3DDeviceManager9(&object->reset_token, &object->device_manager)))
        goto failed;

    if (FAILED(hr = video_presenter_init_d3d(object)))
    {
        WARN("Failed to initialize d3d device, hr %#lx.\n", hr);
        goto failed;
    }

    *out = &object->IUnknown_inner;

    return S_OK;

failed:

    IUnknown_Release(&object->IUnknown_inner);

    return hr;
}
