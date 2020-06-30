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

#include "evr_classes.h"
#include "evr_private.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(evr);

struct video_presenter
{
    IMFVideoPresenter IMFVideoPresenter_iface;
    IMFVideoDeviceID IMFVideoDeviceID_iface;
    IMFTopologyServiceLookupClient IMFTopologyServiceLookupClient_iface;
    IMFVideoDisplayControl IMFVideoDisplayControl_iface;
    IUnknown IUnknown_inner;
    IUnknown *outer_unk;
    LONG refcount;
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

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI video_presenter_inner_Release(IUnknown *iface)
{
    struct video_presenter *presenter = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&presenter->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        heap_free(presenter);
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
    FIXME("%p, %s, %s.\n", iface, debugstr_time(systime), wine_dbgstr_longlong(offset));

    return E_NOTIMPL;
}

static HRESULT WINAPI video_presenter_OnClockStop(IMFVideoPresenter *iface, MFTIME systime)
{
    FIXME("%p, %s.\n", iface, debugstr_time(systime));

    return E_NOTIMPL;
}

static HRESULT WINAPI video_presenter_OnClockPause(IMFVideoPresenter *iface, MFTIME systime)
{
    FIXME("%p, %s.\n", iface, debugstr_time(systime));

    return E_NOTIMPL;
}

static HRESULT WINAPI video_presenter_OnClockRestart(IMFVideoPresenter *iface, MFTIME systime)
{
    FIXME("%p, %s.\n", iface, debugstr_time(systime));

    return E_NOTIMPL;
}

static HRESULT WINAPI video_presenter_OnClockSetRate(IMFVideoPresenter *iface, MFTIME systime, float rate)
{
    FIXME("%p, %s, %f.\n", iface, debugstr_time(systime), rate);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_presenter_ProcessMessage(IMFVideoPresenter *iface, MFVP_MESSAGE_TYPE message, ULONG_PTR param)
{
    FIXME("%p, %d, %lu.\n", iface, message, param);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_presenter_GetCurrentMediaType(IMFVideoPresenter *iface, IMFVideoMediaType **media_type)
{
    FIXME("%p, %p.\n", iface, media_type);

    return E_NOTIMPL;
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

static HRESULT WINAPI video_presenter_service_client_InitServicePointers(IMFTopologyServiceLookupClient *iface,
        IMFTopologyServiceLookup *service_lookup)
{
    FIXME("%p, %p.\n", iface, service_lookup);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_presenter_service_client_ReleaseServicePointers(IMFTopologyServiceLookupClient *iface)
{
    FIXME("%p.\n", iface);

    return E_NOTIMPL;
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
    FIXME("%p, %p, %p.\n", iface, video_size, aspect_ratio);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_presenter_control_GetIdealVideoSize(IMFVideoDisplayControl *iface, SIZE *min_size,
        SIZE *max_size)
{
    FIXME("%p, %p, %p.\n", iface, min_size, max_size);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_presenter_control_SetVideoPosition(IMFVideoDisplayControl *iface,
        const MFVideoNormalizedRect *source, const RECT *dest)
{
    FIXME("%p, %p, %p.\n", iface, source, dest);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_presenter_control_GetVideoPosition(IMFVideoDisplayControl *iface, MFVideoNormalizedRect *source,
        RECT *dest)
{
    FIXME("%p, %p, %p.\n", iface, source, dest);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_presenter_control_SetAspectRatioMode(IMFVideoDisplayControl *iface, DWORD mode)
{
    FIXME("%p, %d.\n", iface, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_presenter_control_GetAspectRatioMode(IMFVideoDisplayControl *iface, DWORD *mode)
{
    FIXME("%p, %p.\n", iface, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_presenter_control_SetVideoWindow(IMFVideoDisplayControl *iface, HWND window)
{
    FIXME("%p, %p.\n", iface, window);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_presenter_control_GetVideoWindow(IMFVideoDisplayControl *iface, HWND *window)
{
    FIXME("%p, %p.\n", iface, window);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_presenter_control_RepaintVideo(IMFVideoDisplayControl *iface)
{
    FIXME("%p.\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_presenter_control_GetCurrentImage(IMFVideoDisplayControl *iface, BITMAPINFOHEADER *header,
        BYTE **dib, DWORD *dib_size, LONGLONG *timestamp)
{
    FIXME("%p, %p, %p, %p, %p.\n", iface, header, dib, dib_size, timestamp);

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
};

HRESULT WINAPI MFCreateVideoPresenter(IUnknown *owner, REFIID riid_device, REFIID riid, void **obj)
{
    TRACE("%p, %s, %s, %p.\n", owner, debugstr_guid(riid_device), debugstr_guid(riid), obj);

    *obj = NULL;

    if (!IsEqualIID(riid_device, &IID_IDirect3DDevice9))
        return E_INVALIDARG;

    return CoCreateInstance(&CLSID_MFVideoPresenter9, owner, CLSCTX_INPROC_SERVER, riid, obj);
}

HRESULT evr_presenter_create(IUnknown *outer, void **out)
{
    struct video_presenter *object;

    if (!(object = heap_alloc(sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFVideoPresenter_iface.lpVtbl = &video_presenter_vtbl;
    object->IMFVideoDeviceID_iface.lpVtbl = &video_presenter_device_id_vtbl;
    object->IMFTopologyServiceLookupClient_iface.lpVtbl = &video_presenter_service_client_vtbl;
    object->IMFVideoDisplayControl_iface.lpVtbl = &video_presenter_control_vtbl;
    object->IUnknown_inner.lpVtbl = &video_presenter_inner_vtbl;
    object->outer_unk = outer ? outer : &object->IUnknown_inner;
    object->refcount = 1;

    *out = &object->IUnknown_inner;

    return S_OK;
}
