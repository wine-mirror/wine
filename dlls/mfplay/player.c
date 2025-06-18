/*
 * Copyright 2019 Nikolay Sivov for CodeWeavers
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

#include <stdarg.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "mfapi.h"
#include "mfplay.h"
#include "mferror.h"

#include "wine/debug.h"

#include "initguid.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

DEFINE_GUID(_MF_TOPO_MEDIA_ITEM, 0x6c1bb4df, 0x59ba, 0x4020, 0x85, 0x0c, 0x35, 0x79, 0xa2, 0x7a, 0xe2, 0x51);
DEFINE_GUID(_MF_CUSTOM_SINK, 0x7c1bb4df, 0x59ba, 0x4020, 0x85, 0x0c, 0x35, 0x79, 0xa2, 0x7a, 0xe2, 0x51);

static const WCHAR eventclassW[] = L"MediaPlayerEventCallbackClass";

static LONG startup_refcount;
static HINSTANCE mfplay_instance;

static void platform_startup(void)
{
    if (InterlockedIncrement(&startup_refcount) == 1)
        MFStartup(MF_VERSION, MFSTARTUP_FULL);
}

static void platform_shutdown(void)
{
    if (InterlockedDecrement(&startup_refcount) == 0)
        MFShutdown();
}

static inline const char *debugstr_normalized_rect(const MFVideoNormalizedRect *rect)
{
    if (!rect) return "(null)";
    return wine_dbg_sprintf("(%.8e,%.8e)-(%.8e,%.8e)", rect->left, rect->top, rect->right, rect->bottom);
}

struct media_player;

enum media_item_flags
{
    MEDIA_ITEM_SOURCE_NEEDS_SHUTDOWN = 0x1,
};

struct media_item
{
    IMFPMediaItem IMFPMediaItem_iface;
    LONG refcount;
    struct media_player *player;
    IMFMediaSource *source;
    IMFPresentationDescriptor *pd;
    DWORD_PTR user_data;
    WCHAR *url;
    IUnknown *object;
    LONGLONG start_position;
    LONGLONG stop_position;
    unsigned int flags;
};

struct media_player
{
    IMFPMediaPlayer IMFPMediaPlayer_iface;
    IPropertyStore IPropertyStore_iface;
    IMFAsyncCallback resolver_callback;
    IMFAsyncCallback events_callback;
    IMFAsyncCallback session_events_callback;
    LONG refcount;
    IMFPMediaPlayerCallback *callback;
    IPropertyStore *propstore;
    IMFSourceResolver *resolver;
    IMFMediaSession *session;
    IMFPMediaItem *item;
    MFP_CREATION_OPTIONS options;
    MFP_MEDIAPLAYER_STATE state;
    HWND event_window;
    HWND output_window;
    CRITICAL_SECTION cs;
};

struct generic_event
{
    MFP_EVENT_HEADER header;
    IMFPMediaItem *item;
};

struct media_event
{
    IUnknown IUnknown_iface;
    LONG refcount;
    union
    {
        MFP_EVENT_HEADER header;
        struct generic_event generic;
        MFP_PLAY_EVENT play;
        MFP_PAUSE_EVENT pause;
        MFP_STOP_EVENT stop;
        MFP_POSITION_SET_EVENT position_set;
        MFP_RATE_SET_EVENT rate_set;
        MFP_MEDIAITEM_CREATED_EVENT item_created;
        MFP_MEDIAITEM_SET_EVENT item_set;
        MFP_MEDIAITEM_CLEARED_EVENT item_cleared;
        MFP_MF_EVENT event;
        MFP_ERROR_EVENT error;
        MFP_PLAYBACK_ENDED_EVENT ended;
        MFP_ACQUIRE_USER_CREDENTIAL_EVENT acquire_creds;
    } u;
};

static struct media_player *impl_from_IMFPMediaPlayer(IMFPMediaPlayer *iface)
{
    return CONTAINING_RECORD(iface, struct media_player, IMFPMediaPlayer_iface);
}

static struct media_player *impl_from_IPropertyStore(IPropertyStore *iface)
{
    return CONTAINING_RECORD(iface, struct media_player, IPropertyStore_iface);
}

static struct media_player *impl_from_resolver_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct media_player, resolver_callback);
}

static struct media_player *impl_from_events_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct media_player, events_callback);
}

static struct media_player *impl_from_session_events_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct media_player, session_events_callback);
}

static struct media_item *impl_from_IMFPMediaItem(IMFPMediaItem *iface)
{
    return CONTAINING_RECORD(iface, struct media_item, IMFPMediaItem_iface);
}

static struct media_event *impl_event_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct media_event, IUnknown_iface);
}

static HRESULT WINAPI media_event_QueryInterface(IUnknown *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI media_event_AddRef(IUnknown *iface)
{
    struct media_event *event = impl_event_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&event->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI media_event_Release(IUnknown *iface)
{
    struct media_event *event = impl_event_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&event->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (event->u.header.pMediaPlayer)
            IMFPMediaPlayer_Release(event->u.header.pMediaPlayer);
        if (event->u.header.pPropertyStore)
            IPropertyStore_Release(event->u.header.pPropertyStore);

        switch (event->u.header.eEventType)
        {
            /* Most types share same layout. */
            case MFP_EVENT_TYPE_PLAY:
            case MFP_EVENT_TYPE_PAUSE:
            case MFP_EVENT_TYPE_STOP:
            case MFP_EVENT_TYPE_POSITION_SET:
            case MFP_EVENT_TYPE_RATE_SET:
            case MFP_EVENT_TYPE_MEDIAITEM_CREATED:
            case MFP_EVENT_TYPE_MEDIAITEM_SET:
            case MFP_EVENT_TYPE_FRAME_STEP:
            case MFP_EVENT_TYPE_MEDIAITEM_CLEARED:
            case MFP_EVENT_TYPE_PLAYBACK_ENDED:
                if (event->u.generic.item)
                    IMFPMediaItem_Release(event->u.generic.item);
                break;
            case MFP_EVENT_TYPE_MF:
                if (event->u.event.pMFMediaEvent)
                    IMFMediaEvent_Release(event->u.event.pMFMediaEvent);
                if (event->u.event.pMediaItem)
                    IMFPMediaItem_Release(event->u.event.pMediaItem);
                break;
            default:
                FIXME("Unsupported event %u.\n", event->u.header.eEventType);
                break;
        }

        free(event);
    }

    return refcount;
}

static const IUnknownVtbl media_event_vtbl =
{
    media_event_QueryInterface,
    media_event_AddRef,
    media_event_Release,
};

static HRESULT media_event_create(struct media_player *player, MFP_EVENT_TYPE event_type,
        HRESULT hr, IMFPMediaItem *item, struct media_event **event)
{
    struct media_event *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IUnknown_iface.lpVtbl = &media_event_vtbl;
    object->refcount = 1;
    object->u.header.eEventType = event_type;
    object->u.header.hrEvent = hr;
    object->u.header.pMediaPlayer = &player->IMFPMediaPlayer_iface;
    IMFPMediaPlayer_AddRef(object->u.header.pMediaPlayer);
    object->u.header.eState = player->state;
    switch (event_type)
    {
        case MFP_EVENT_TYPE_PLAY:
        case MFP_EVENT_TYPE_PAUSE:
        case MFP_EVENT_TYPE_STOP:
        case MFP_EVENT_TYPE_POSITION_SET:
        case MFP_EVENT_TYPE_RATE_SET:
        case MFP_EVENT_TYPE_MEDIAITEM_CREATED:
        case MFP_EVENT_TYPE_MEDIAITEM_SET:
        case MFP_EVENT_TYPE_FRAME_STEP:
        case MFP_EVENT_TYPE_MEDIAITEM_CLEARED:
        case MFP_EVENT_TYPE_PLAYBACK_ENDED:
            object->u.generic.item = item;
            if (object->u.generic.item)
                IMFPMediaItem_AddRef(object->u.generic.item);
            break;
        case MFP_EVENT_TYPE_MF:
            object->u.event.pMediaItem = item;
            if (object->u.event.pMediaItem)
                IMFPMediaItem_AddRef(object->u.event.pMediaItem);
            break;
        default:
            ;
    }

    /* FIXME: set properties for some events? */

    *event = object;

    return S_OK;
}

static LRESULT WINAPI media_player_event_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    struct media_event *event = (void *)lparam;
    struct media_player *player;

    if (msg == WM_USER)
    {
        player = impl_from_IMFPMediaPlayer(event->u.header.pMediaPlayer);
        if (player->callback)
            IMFPMediaPlayerCallback_OnMediaPlayerEvent(player->callback, &event->u.header);
        IUnknown_Release(&event->IUnknown_iface);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

static void media_player_set_state(struct media_player *player, MFP_MEDIAPLAYER_STATE state)
{
    if (player->state != MFP_MEDIAPLAYER_STATE_SHUTDOWN)
    {
        if (state == MFP_MEDIAPLAYER_STATE_SHUTDOWN)
            IMFMediaSession_Shutdown(player->session);
        player->state = state;
    }
}

static HRESULT media_item_get_pd(const struct media_item *item, IMFPresentationDescriptor **pd)
{
    if (item->player->state == MFP_MEDIAPLAYER_STATE_SHUTDOWN)
        return MF_E_SHUTDOWN;

    *pd = item->pd;
    IMFPresentationDescriptor_AddRef(*pd);
    return S_OK;
}

static HRESULT WINAPI media_item_QueryInterface(IMFPMediaItem *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFPMediaItem) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFPMediaItem_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI media_item_AddRef(IMFPMediaItem *iface)
{
    struct media_item *item = impl_from_IMFPMediaItem(iface);
    ULONG refcount = InterlockedIncrement(&item->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI media_item_Release(IMFPMediaItem *iface)
{
    struct media_item *item = impl_from_IMFPMediaItem(iface);
    ULONG refcount = InterlockedDecrement(&item->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (item->player)
            IMFPMediaPlayer_Release(&item->player->IMFPMediaPlayer_iface);
        if (item->source)
        {
            if (item->flags & MEDIA_ITEM_SOURCE_NEEDS_SHUTDOWN)
                IMFMediaSource_Shutdown(item->source);
            IMFMediaSource_Release(item->source);
        }
        if (item->pd)
            IMFPresentationDescriptor_Release(item->pd);
        if (item->object)
            IUnknown_Release(item->object);
        free(item->url);
        free(item);
    }

    return refcount;
}

static HRESULT WINAPI media_item_GetMediaPlayer(IMFPMediaItem *iface,
        IMFPMediaPlayer **player)
{
    struct media_item *item = impl_from_IMFPMediaItem(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, player);

    EnterCriticalSection(&item->player->cs);
    if (item->player->state == MFP_MEDIAPLAYER_STATE_SHUTDOWN)
    {
        hr = MF_E_SHUTDOWN;
        *player = NULL;
    }
    else
    {
        *player = &item->player->IMFPMediaPlayer_iface;
        IMFPMediaPlayer_AddRef(*player);
    }
    LeaveCriticalSection(&item->player->cs);

    return hr;
}

static HRESULT WINAPI media_item_GetURL(IMFPMediaItem *iface, LPWSTR *url)
{
    struct media_item *item = impl_from_IMFPMediaItem(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, url);

    EnterCriticalSection(&item->player->cs);
    if (item->player->state == MFP_MEDIAPLAYER_STATE_SHUTDOWN)
        hr = MF_E_SHUTDOWN;
    else if (!item->url)
        hr = MF_E_NOT_FOUND;
    else
    {
        if (!(*url = CoTaskMemAlloc((wcslen(item->url) + 1) * sizeof(*item->url))))
            hr = E_OUTOFMEMORY;
        if (*url)
            wcscpy(*url, item->url);
    }
    LeaveCriticalSection(&item->player->cs);

    return hr;
}

static HRESULT WINAPI media_item_GetObject(IMFPMediaItem *iface, IUnknown **object)
{
    struct media_item *item = impl_from_IMFPMediaItem(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, object);

    EnterCriticalSection(&item->player->cs);
    if (item->player->state == MFP_MEDIAPLAYER_STATE_SHUTDOWN)
        hr = MF_E_SHUTDOWN;
    else if (!item->object)
        hr = MF_E_NOT_FOUND;
    else
    {
        *object = item->object;
        IUnknown_AddRef(*object);
    }
    LeaveCriticalSection(&item->player->cs);

    return hr;
}

static HRESULT WINAPI media_item_GetUserData(IMFPMediaItem *iface, DWORD_PTR *user_data)
{
    struct media_item *item = impl_from_IMFPMediaItem(iface);

    TRACE("%p, %p.\n", iface, user_data);

    *user_data = item->user_data;

    return S_OK;
}

static HRESULT WINAPI media_item_SetUserData(IMFPMediaItem *iface, DWORD_PTR user_data)
{
    struct media_item *item = impl_from_IMFPMediaItem(iface);

    TRACE("%p, %Ix.\n", iface, user_data);

    item->user_data = user_data;

    return S_OK;
}

static HRESULT media_item_set_position(const GUID *format, const PROPVARIANT *position, LARGE_INTEGER *ret)
{
    ret->QuadPart = 0;

    if (format && !IsEqualGUID(format, &MFP_POSITIONTYPE_100NS))
        return E_INVALIDARG;

    if ((format != NULL) ^ (position != NULL))
        return E_POINTER;

    if (position && position->vt != VT_EMPTY && position->vt != VT_I8)
        return E_INVALIDARG;

    if ((!format && !position) || position->vt == VT_EMPTY)
        return S_OK;

    if (position->hVal.QuadPart == 0)
        return MF_E_OUT_OF_RANGE;

    ret->QuadPart = position->hVal.QuadPart;

    return S_OK;
}

static void media_item_get_position(LONGLONG value, GUID *format, PROPVARIANT *position)
{
    if (!format)
        return;

    memcpy(format, &MFP_POSITIONTYPE_100NS, sizeof(*format));

    if (value)
    {
        position->vt = VT_I8;
        position->hVal.QuadPart = value;
    }
}

static HRESULT WINAPI media_item_GetStartStopPosition(IMFPMediaItem *iface, GUID *start_format,
        PROPVARIANT *start_position, GUID *stop_format, PROPVARIANT *stop_position)
{
    struct media_item *item = impl_from_IMFPMediaItem(iface);

    TRACE("%p, %p, %p, %p, %p.\n", iface, start_format, start_position, stop_format, stop_position);

    if (start_position)
        start_position->vt = VT_EMPTY;
    if (stop_position)
        stop_position->vt = VT_EMPTY;

    if (((start_format != NULL) ^ (start_position != NULL)) ||
            ((stop_format != NULL) ^ (stop_position != NULL)))
    {
        return E_POINTER;
    }

    media_item_get_position(item->start_position, start_format, start_position);
    media_item_get_position(item->stop_position, stop_format, stop_position);

    return S_OK;
}

static HRESULT WINAPI media_item_SetStartStopPosition(IMFPMediaItem *iface, const GUID *start_format,
        const PROPVARIANT *start_position, const GUID *stop_format, const PROPVARIANT *stop_position)
{
    struct media_item *item = impl_from_IMFPMediaItem(iface);
    LARGE_INTEGER start, stop;
    HRESULT hr;

    TRACE("%p, %s, %p, %s, %p.\n", iface, debugstr_guid(start_format), start_position,
            debugstr_guid(stop_format), stop_position);

    hr = media_item_set_position(start_format, start_position, &start);
    if (SUCCEEDED(hr))
        hr = media_item_set_position(stop_format, stop_position, &stop);

    if (FAILED(hr))
        return hr;

    if (start.QuadPart > stop.QuadPart)
        return MF_E_OUT_OF_RANGE;

    item->start_position = start.QuadPart;
    item->stop_position = stop.QuadPart;

    return hr;
}

static HRESULT media_item_get_stream_type(IMFStreamDescriptor *sd, GUID *major)
{
    IMFMediaTypeHandler *handler;
    HRESULT hr;

    if (SUCCEEDED(hr = IMFStreamDescriptor_GetMediaTypeHandler(sd, &handler)))
    {
        hr = IMFMediaTypeHandler_GetMajorType(handler, major);
        IMFMediaTypeHandler_Release(handler);
    }

    return hr;
}

static HRESULT media_item_has_stream(struct media_item *item, const GUID *major, BOOL *has_stream, BOOL *is_selected)
{
    IMFPresentationDescriptor *pd;
    IMFStreamDescriptor *sd;
    unsigned int idx = 0;
    BOOL selected;
    HRESULT hr;
    GUID guid;

    EnterCriticalSection(&item->player->cs);

    if (SUCCEEDED(hr = media_item_get_pd(item, &pd)))
    {
        *has_stream = *is_selected = FALSE;

        while (SUCCEEDED(IMFPresentationDescriptor_GetStreamDescriptorByIndex(pd, idx++, &selected, &sd)))
        {
            if (SUCCEEDED(media_item_get_stream_type(sd, &guid)) && IsEqualGUID(&guid, major))
            {
                *has_stream = TRUE;
                *is_selected = selected;
            }

            IMFStreamDescriptor_Release(sd);

            if (*has_stream && *is_selected)
                break;
        }
        IMFPresentationDescriptor_Release(pd);
    }

    LeaveCriticalSection(&item->player->cs);

    return hr;
}

static HRESULT WINAPI media_item_HasVideo(IMFPMediaItem *iface, BOOL *has_video, BOOL *selected)
{
    struct media_item *item = impl_from_IMFPMediaItem(iface);

    TRACE("%p, %p, %p.\n", iface, has_video, selected);

    return media_item_has_stream(item, &MFMediaType_Video, has_video, selected);
}

static HRESULT WINAPI media_item_HasAudio(IMFPMediaItem *iface, BOOL *has_audio, BOOL *selected)
{
    struct media_item *item = impl_from_IMFPMediaItem(iface);

    TRACE("%p, %p, %p.\n", iface, has_audio, selected);

    return media_item_has_stream(item, &MFMediaType_Audio, has_audio, selected);
}

static HRESULT WINAPI media_item_IsProtected(IMFPMediaItem *iface, BOOL *protected)
{
    struct media_item *item = impl_from_IMFPMediaItem(iface);
    IMFPresentationDescriptor *pd;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, protected);

    if (!protected)
        return E_POINTER;

    EnterCriticalSection(&item->player->cs);
    if (SUCCEEDED(hr = media_item_get_pd(item, &pd)))
    {
        *protected = (hr = MFRequireProtectedEnvironment(pd)) == S_OK;
        IMFPresentationDescriptor_Release(pd);
    }
    LeaveCriticalSection(&item->player->cs);

    return hr;
}

static HRESULT WINAPI media_item_GetDuration(IMFPMediaItem *iface, REFGUID format, PROPVARIANT *value)
{
    struct media_item *item = impl_from_IMFPMediaItem(iface);
    IMFPresentationDescriptor *pd;
    HRESULT hr;

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(format), value);

    EnterCriticalSection(&item->player->cs);
    if (SUCCEEDED(hr = media_item_get_pd(item, &pd)))
    {
        hr = IMFPresentationDescriptor_GetItem(pd, &MF_PD_DURATION, value);
        IMFPresentationDescriptor_Release(pd);
    }
    LeaveCriticalSection(&item->player->cs);

    return hr;
}

static HRESULT WINAPI media_item_GetNumberOfStreams(IMFPMediaItem *iface, DWORD *count)
{
    struct media_item *item = impl_from_IMFPMediaItem(iface);
    IMFPresentationDescriptor *pd;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, count);

    EnterCriticalSection(&item->player->cs);
    if (SUCCEEDED(hr = media_item_get_pd(item, &pd)))
    {
        hr = IMFPresentationDescriptor_GetStreamDescriptorCount(pd, count);
        IMFPresentationDescriptor_Release(pd);
    }
    LeaveCriticalSection(&item->player->cs);

    return hr;
}

static HRESULT WINAPI media_item_GetStreamSelection(IMFPMediaItem *iface, DWORD index, BOOL *selected)
{
    struct media_item *item = impl_from_IMFPMediaItem(iface);
    IMFPresentationDescriptor *pd;
    IMFStreamDescriptor *sd;
    HRESULT hr;

    TRACE("%p, %lu, %p.\n", iface, index, selected);

    EnterCriticalSection(&item->player->cs);
    if (SUCCEEDED(hr = media_item_get_pd(item, &pd)))
    {
        if (SUCCEEDED(hr = IMFPresentationDescriptor_GetStreamDescriptorByIndex(pd, index, selected, &sd)))
            IMFStreamDescriptor_Release(sd);
        IMFPresentationDescriptor_Release(pd);
    }
    LeaveCriticalSection(&item->player->cs);

    return hr;
}

static HRESULT WINAPI media_item_SetStreamSelection(IMFPMediaItem *iface, DWORD index, BOOL select)
{
    struct media_item *item = impl_from_IMFPMediaItem(iface);
    IMFPresentationDescriptor *pd;
    HRESULT hr;

    TRACE("%p, %lu, %d.\n", iface, index, select);

    EnterCriticalSection(&item->player->cs);
    if (SUCCEEDED(hr = media_item_get_pd(item, &pd)))
    {
        hr = select ? IMFPresentationDescriptor_SelectStream(pd, index) :
                IMFPresentationDescriptor_DeselectStream(pd, index);
        IMFPresentationDescriptor_Release(pd);
    }
    LeaveCriticalSection(&item->player->cs);

    return hr;
}

static HRESULT WINAPI media_item_GetStreamAttribute(IMFPMediaItem *iface, DWORD index, REFGUID key,
        PROPVARIANT *value)
{
    struct media_item *item = impl_from_IMFPMediaItem(iface);
    IMFPresentationDescriptor *pd;
    IMFStreamDescriptor *sd;
    BOOL selected;
    HRESULT hr;

    TRACE("%p, %lu, %s, %p.\n", iface, index, debugstr_guid(key), value);

    EnterCriticalSection(&item->player->cs);
    if (SUCCEEDED(hr = media_item_get_pd(item, &pd)))
    {
        if (SUCCEEDED(hr = IMFPresentationDescriptor_GetStreamDescriptorByIndex(pd, index, &selected, &sd)))
        {
            hr = IMFStreamDescriptor_GetItem(sd, key, value);
            IMFStreamDescriptor_Release(sd);
        }
        IMFPresentationDescriptor_Release(pd);
    }
    LeaveCriticalSection(&item->player->cs);

    return hr;
}

static HRESULT WINAPI media_item_GetPresentationAttribute(IMFPMediaItem *iface, REFGUID key,
        PROPVARIANT *value)
{
    struct media_item *item = impl_from_IMFPMediaItem(iface);
    IMFPresentationDescriptor *pd;
    HRESULT hr;

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    EnterCriticalSection(&item->player->cs);
    if (SUCCEEDED(hr = media_item_get_pd(item, &pd)))
    {
        hr = IMFPresentationDescriptor_GetItem(pd, key, value);
        IMFPresentationDescriptor_Release(pd);
    }
    LeaveCriticalSection(&item->player->cs);

    return hr;
}

static HRESULT WINAPI media_item_GetCharacteristics(IMFPMediaItem *iface, MFP_MEDIAITEM_CHARACTERISTICS *flags)
{
    struct media_item *item = impl_from_IMFPMediaItem(iface);
    DWORD value = 0;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, flags);

    *flags = 0;

    EnterCriticalSection(&item->player->cs);
    if (item->player->state == MFP_MEDIAPLAYER_STATE_SHUTDOWN)
        hr = MF_E_SHUTDOWN;
    else
    {
        if (SUCCEEDED(hr = IMFMediaSource_GetCharacteristics(item->source, &value)))
        {
            *flags = value & (MFP_MEDIAITEM_IS_LIVE | MFP_MEDIAITEM_CAN_SEEK
                    | MFP_MEDIAITEM_CAN_PAUSE | MFP_MEDIAITEM_HAS_SLOW_SEEK);
        }
    }
    LeaveCriticalSection(&item->player->cs);

    return hr;
}

static HRESULT WINAPI media_item_SetStreamSink(IMFPMediaItem *iface, DWORD index, IUnknown *sink)
{
    struct media_item *item = impl_from_IMFPMediaItem(iface);
    IMFStreamDescriptor *sd;
    IUnknown *sink_object;
    BOOL selected;
    HRESULT hr;

    TRACE("%p, %lu, %p.\n", iface, index, sink);

    if (FAILED(hr = IMFPresentationDescriptor_GetStreamDescriptorByIndex(item->pd, index, &selected, &sd)))
        return hr;

    if (sink)
    {
        if (FAILED(hr = IUnknown_QueryInterface(sink, &IID_IMFStreamSink, (void **)&sink_object)))
            hr = IUnknown_QueryInterface(sink, &IID_IMFActivate, (void **)&sink_object);

        if (sink_object)
        {
            hr = IMFStreamDescriptor_SetUnknown(sd, &_MF_CUSTOM_SINK, sink_object);
            IUnknown_Release(sink_object);
        }
    }
    else
        IMFStreamDescriptor_DeleteItem(sd, &_MF_CUSTOM_SINK);

    IMFStreamDescriptor_Release(sd);

    return hr;
}

static HRESULT WINAPI media_item_GetMetadata(IMFPMediaItem *iface, IPropertyStore **metadata)
{
    struct media_item *item = impl_from_IMFPMediaItem(iface);

    TRACE("%p, %p.\n", iface, metadata);

    return MFGetService((IUnknown *)item->source, &MF_PROPERTY_HANDLER_SERVICE,
            &IID_IPropertyStore, (void **)metadata);
}

static const IMFPMediaItemVtbl media_item_vtbl =
{
    media_item_QueryInterface,
    media_item_AddRef,
    media_item_Release,
    media_item_GetMediaPlayer,
    media_item_GetURL,
    media_item_GetObject,
    media_item_GetUserData,
    media_item_SetUserData,
    media_item_GetStartStopPosition,
    media_item_SetStartStopPosition,
    media_item_HasVideo,
    media_item_HasAudio,
    media_item_IsProtected,
    media_item_GetDuration,
    media_item_GetNumberOfStreams,
    media_item_GetStreamSelection,
    media_item_SetStreamSelection,
    media_item_GetStreamAttribute,
    media_item_GetPresentationAttribute,
    media_item_GetCharacteristics,
    media_item_SetStreamSink,
    media_item_GetMetadata,
};

static struct media_item *unsafe_impl_from_IMFPMediaItem(IMFPMediaItem *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == (IMFPMediaItemVtbl *)&media_item_vtbl);
    return CONTAINING_RECORD(iface, struct media_item, IMFPMediaItem_iface);
}

static HRESULT create_media_item(struct media_player *player, DWORD_PTR user_data, struct media_item **item)
{
    struct media_item *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFPMediaItem_iface.lpVtbl = &media_item_vtbl;
    object->refcount = 1;
    object->user_data = user_data;
    object->player = player;
    IMFPMediaPlayer_AddRef(&object->player->IMFPMediaPlayer_iface);

    *item = object;

    return S_OK;
}

static HRESULT media_item_set_source(struct media_item *item, IUnknown *object)
{
    IMFPresentationDescriptor *pd;
    IMFMediaSource *source;
    HRESULT hr;

    if (FAILED(hr = IUnknown_QueryInterface(object, &IID_IMFMediaSource, (void **)&source)))
        return hr;

    if (FAILED(hr = IMFMediaSource_CreatePresentationDescriptor(source, &pd)))
    {
        WARN("Failed to get presentation descriptor, hr %#lx.\n", hr);
        IMFMediaSource_Release(source);
        return hr;
    }

    item->source = source;
    item->pd = pd;

    return hr;
}

static void media_player_queue_event(struct media_player *player, struct media_event *event)
{
    if (player->options & MFP_OPTION_FREE_THREADED_CALLBACK)
    {
        MFPutWorkItem(MFASYNC_CALLBACK_QUEUE_MULTITHREADED, &player->events_callback, &event->IUnknown_iface);
    }
    else
    {
        IUnknown_AddRef(&event->IUnknown_iface);
        PostMessageW(player->event_window, WM_USER, 0, (LPARAM)event);
    }
}

static HRESULT WINAPI media_player_QueryInterface(IMFPMediaPlayer *iface, REFIID riid, void **obj)
{
    struct media_player *player = impl_from_IMFPMediaPlayer(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFPMediaPlayer) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = &player->IMFPMediaPlayer_iface;
    }
    else if (IsEqualIID(riid, &IID_IPropertyStore))
    {
        *obj = &player->IPropertyStore_iface;
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

static ULONG WINAPI media_player_AddRef(IMFPMediaPlayer *iface)
{
    struct media_player *player = impl_from_IMFPMediaPlayer(iface);
    ULONG refcount = InterlockedIncrement(&player->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI media_player_Release(IMFPMediaPlayer *iface)
{
    struct media_player *player = impl_from_IMFPMediaPlayer(iface);
    ULONG refcount = InterlockedDecrement(&player->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (player->callback)
            IMFPMediaPlayerCallback_Release(player->callback);
        if (player->propstore)
            IPropertyStore_Release(player->propstore);
        if (player->resolver)
            IMFSourceResolver_Release(player->resolver);
        if (player->session)
            IMFMediaSession_Release(player->session);
        DestroyWindow(player->event_window);
        DeleteCriticalSection(&player->cs);
        free(player);

        platform_shutdown();
    }

    return refcount;
}

static HRESULT WINAPI media_player_Play(IMFPMediaPlayer *iface)
{
    struct media_player *player = impl_from_IMFPMediaPlayer(iface);
    PROPVARIANT pos;

    TRACE("%p.\n", iface);

    pos.vt = VT_EMPTY;
    return IMFMediaSession_Start(player->session, &GUID_NULL, &pos);
}

static HRESULT WINAPI media_player_Pause(IMFPMediaPlayer *iface)
{
    struct media_player *player = impl_from_IMFPMediaPlayer(iface);

    TRACE("%p.\n", iface);

    return IMFMediaSession_Pause(player->session);
}

static HRESULT WINAPI media_player_Stop(IMFPMediaPlayer *iface)
{
    struct media_player *player = impl_from_IMFPMediaPlayer(iface);

    TRACE("%p.\n", iface);

    return IMFMediaSession_Stop(player->session);
}

static HRESULT WINAPI media_player_FrameStep(IMFPMediaPlayer *iface)
{
    FIXME("%p.\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_player_SetPosition(IMFPMediaPlayer *iface, REFGUID postype, const PROPVARIANT *position)
{
    FIXME("%p, %s, %p.\n", iface, debugstr_guid(postype), position);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_player_GetPosition(IMFPMediaPlayer *iface, REFGUID postype, PROPVARIANT *position)
{
    struct media_player *player = impl_from_IMFPMediaPlayer(iface);
    IMFPresentationClock *presentation_clock;
    IMFClock *clock;
    HRESULT hr;

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(postype), position);

    if (!position)
        return E_POINTER;

    if (!IsEqualGUID(postype, &MFP_POSITIONTYPE_100NS))
        return E_INVALIDARG;

    EnterCriticalSection(&player->cs);
    if (player->state == MFP_MEDIAPLAYER_STATE_SHUTDOWN)
        hr = MF_E_SHUTDOWN;
    else if (!player->item)
        hr = MF_E_INVALIDREQUEST;
    else
    {
        if (SUCCEEDED(hr = IMFMediaSession_GetClock(player->session, &clock)))
        {
            if (SUCCEEDED(hr = IMFClock_QueryInterface(clock, &IID_IMFPresentationClock, (void **)&presentation_clock)))
            {
                position->vt = VT_UI8;
                hr = IMFPresentationClock_GetTime(presentation_clock, (MFTIME *)&position->uhVal.QuadPart);
                IMFPresentationClock_Release(presentation_clock);
            }
            IMFClock_Release(clock);
        }
    }
    LeaveCriticalSection(&player->cs);

    return hr;
}

static HRESULT WINAPI media_player_GetDuration(IMFPMediaPlayer *iface, REFGUID postype, PROPVARIANT *duration)
{
    struct media_player *player = impl_from_IMFPMediaPlayer(iface);
    HRESULT hr;

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(postype), duration);

    if (!duration)
        return E_POINTER;

    if (!IsEqualGUID(postype, &MFP_POSITIONTYPE_100NS))
        return E_INVALIDARG;

    EnterCriticalSection(&player->cs);
    if (player->state == MFP_MEDIAPLAYER_STATE_SHUTDOWN)
        hr = MF_E_SHUTDOWN;
    else if (!player->item)
        hr = MF_E_INVALIDREQUEST;
    else
        /* FIXME: use start/stop markers for resulting duration */
        hr = IMFPMediaItem_GetDuration(player->item, postype, duration);
    LeaveCriticalSection(&player->cs);

    return hr;
}

static HRESULT WINAPI media_player_SetRate(IMFPMediaPlayer *iface, float rate)
{
    struct media_player *player = impl_from_IMFPMediaPlayer(iface);
    IMFRateControl *rate_control;
    HRESULT hr;

    TRACE("%p, %f.\n", iface, rate);

    if (rate == 0.0f)
        return MF_E_OUT_OF_RANGE;

    if (SUCCEEDED(hr = MFGetService((IUnknown *)player->session, &MF_RATE_CONTROL_SERVICE, &IID_IMFRateControl,
            (void **)&rate_control)))
    {
        hr = IMFRateControl_SetRate(rate_control, FALSE, rate);
        IMFRateControl_Release(rate_control);
    }

    return hr;
}

static HRESULT WINAPI media_player_GetRate(IMFPMediaPlayer *iface, float *rate)
{
    struct media_player *player = impl_from_IMFPMediaPlayer(iface);
    IMFRateControl *rate_control;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, rate);

    if (!rate)
        return E_POINTER;

    if (SUCCEEDED(hr = MFGetService((IUnknown *)player->session, &MF_RATE_CONTROL_SERVICE, &IID_IMFRateControl,
            (void **)&rate_control)))
    {
        hr = IMFRateControl_GetRate(rate_control, NULL, rate);
        IMFRateControl_Release(rate_control);
    }

    return hr;
}

static HRESULT WINAPI media_player_GetSupportedRates(IMFPMediaPlayer *iface, BOOL forward,
        float *slowest_rate, float *fastest_rate)
{
    struct media_player *player = impl_from_IMFPMediaPlayer(iface);
    IMFRateSupport *rs;
    HRESULT hr;

    TRACE("%p, %d, %p, %p.\n", iface, forward, slowest_rate, fastest_rate);

    if (SUCCEEDED(hr = MFGetService((IUnknown *)player->session, &MF_RATE_CONTROL_SERVICE, &IID_IMFRateSupport, (void **)&rs)))
    {
        if (SUCCEEDED(hr = IMFRateSupport_GetSlowestRate(rs, forward ? MFRATE_FORWARD : MFRATE_REVERSE, FALSE, slowest_rate)))
            hr = IMFRateSupport_GetFastestRate(rs, forward ? MFRATE_FORWARD : MFRATE_REVERSE, FALSE, fastest_rate);
        IMFRateSupport_Release(rs);
    }

    return hr;
}

static HRESULT WINAPI media_player_GetState(IMFPMediaPlayer *iface, MFP_MEDIAPLAYER_STATE *state)
{
    struct media_player *player = impl_from_IMFPMediaPlayer(iface);

    TRACE("%p, %p.\n", iface, state);

    *state = player->state;

    return S_OK;
}

static HRESULT media_player_create_item_from_url(struct media_player *player,
        const WCHAR *url, BOOL sync, DWORD_PTR user_data, IMFPMediaItem **ret)
{
    struct media_item *item;
    MF_OBJECT_TYPE obj_type;
    IUnknown *object;
    HRESULT hr;

    if (sync && !ret)
        return E_POINTER;

    if (!sync && !player->callback)
    {
        WARN("Asynchronous item creation is not supported without user callback.\n");
        return MF_E_INVALIDREQUEST;
    }

    if (FAILED(hr = create_media_item(player, user_data, &item)))
        return hr;

    if (url && !(item->url = wcsdup(url)))
    {
        IMFPMediaItem_Release(&item->IMFPMediaItem_iface);
        return E_OUTOFMEMORY;
    }
    item->flags |= MEDIA_ITEM_SOURCE_NEEDS_SHUTDOWN;

    if (sync)
    {
        *ret = NULL;

        if (SUCCEEDED(hr = IMFSourceResolver_CreateObjectFromURL(player->resolver, url, MF_RESOLUTION_MEDIASOURCE
                | MF_RESOLUTION_CONTENT_DOES_NOT_HAVE_TO_MATCH_EXTENSION_OR_MIME_TYPE, player->propstore, &obj_type, &object)))
        {
            hr = media_item_set_source(item, object);
            IUnknown_Release(object);
        }

        if (SUCCEEDED(hr))
        {
            *ret = &item->IMFPMediaItem_iface;
            IMFPMediaItem_AddRef(*ret);
        }

        IMFPMediaItem_Release(&item->IMFPMediaItem_iface);
    }
    else
    {
        if (ret) *ret = NULL;

        hr = IMFSourceResolver_BeginCreateObjectFromURL(player->resolver, url, MF_RESOLUTION_MEDIASOURCE
                | MF_RESOLUTION_CONTENT_DOES_NOT_HAVE_TO_MATCH_EXTENSION_OR_MIME_TYPE, player->propstore, NULL,
                &player->resolver_callback, (IUnknown *)&item->IMFPMediaItem_iface);

        IMFPMediaItem_Release(&item->IMFPMediaItem_iface);
    }

    return hr;
}

static HRESULT WINAPI media_player_CreateMediaItemFromURL(IMFPMediaPlayer *iface,
        const WCHAR *url, BOOL sync, DWORD_PTR user_data, IMFPMediaItem **item)
{
    struct media_player *player = impl_from_IMFPMediaPlayer(iface);
    HRESULT hr;

    TRACE("%p, %s, %d, %Ix, %p.\n", iface, debugstr_w(url), sync, user_data, item);

    EnterCriticalSection(&player->cs);
    if (player->state == MFP_MEDIAPLAYER_STATE_SHUTDOWN)
        hr = MF_E_SHUTDOWN;
    else
        hr = media_player_create_item_from_url(player, url, sync, user_data, item);
    LeaveCriticalSection(&player->cs);

    return hr;
}

static HRESULT media_player_create_item_from_object(struct media_player *player,
        IUnknown *object, BOOL sync, DWORD_PTR user_data, IMFPMediaItem **ret)
{
    IMFMediaSource *source = NULL;
    IMFByteStream *stream = NULL;
    struct media_item *item;
    MF_OBJECT_TYPE obj_type;
    HRESULT hr;

    *ret = NULL;

    if (FAILED(hr = create_media_item(player, user_data, &item)))
        return hr;

    item->object = object;
    IUnknown_AddRef(item->object);

    if (FAILED(IUnknown_QueryInterface(object, &IID_IMFMediaSource, (void **)&source)))
        IUnknown_QueryInterface(object, &IID_IMFByteStream, (void **)&stream);

    if (!source && !stream)
    {
        WARN("Unsupported object type.\n");
        IMFPMediaItem_Release(&item->IMFPMediaItem_iface);
        return E_UNEXPECTED;
    }

    if (source)
        item->flags |= MEDIA_ITEM_SOURCE_NEEDS_SHUTDOWN;

    if (sync)
    {
        if (stream)
            hr = IMFSourceResolver_CreateObjectFromByteStream(player->resolver, stream, NULL, MF_RESOLUTION_MEDIASOURCE
                    | MF_RESOLUTION_CONTENT_DOES_NOT_HAVE_TO_MATCH_EXTENSION_OR_MIME_TYPE, player->propstore, &obj_type, &object);
        else
            IUnknown_AddRef(object);

        if (SUCCEEDED(hr))
            hr = media_item_set_source(item, object);

        IUnknown_Release(object);

        if (SUCCEEDED(hr))
        {
            *ret = &item->IMFPMediaItem_iface;
            IMFPMediaItem_AddRef(*ret);
        }

        IMFPMediaItem_Release(&item->IMFPMediaItem_iface);
    }
    else
    {
        if (stream)
        {
            hr = IMFSourceResolver_BeginCreateObjectFromByteStream(player->resolver, stream, NULL, MF_RESOLUTION_MEDIASOURCE
                    | MF_RESOLUTION_CONTENT_DOES_NOT_HAVE_TO_MATCH_EXTENSION_OR_MIME_TYPE, player->propstore, NULL,
                    &player->resolver_callback, (IUnknown *)&item->IMFPMediaItem_iface);
        }
        else
        {
            /* Resolver callback will check again if item's object is a source. */
            hr = MFPutWorkItem(MFASYNC_CALLBACK_QUEUE_MULTITHREADED, &player->resolver_callback,
                    (IUnknown *)&item->IMFPMediaItem_iface);
        }

        IMFPMediaItem_Release(&item->IMFPMediaItem_iface);
    }

    if (source)
        IMFMediaSource_Release(source);
    if (stream)
        IMFByteStream_Release(stream);

    return hr;
}

static HRESULT WINAPI media_player_CreateMediaItemFromObject(IMFPMediaPlayer *iface,
        IUnknown *object, BOOL sync, DWORD_PTR user_data, IMFPMediaItem **item)
{
    struct media_player *player = impl_from_IMFPMediaPlayer(iface);
    HRESULT hr;

    TRACE("%p, %p, %d, %Ix, %p.\n", iface, object, sync, user_data, item);

    EnterCriticalSection(&player->cs);
    if (player->state == MFP_MEDIAPLAYER_STATE_SHUTDOWN)
        hr = MF_E_SHUTDOWN;
    else
        hr = media_player_create_item_from_object(player, object, sync, user_data, item);
    LeaveCriticalSection(&player->cs);

    return hr;
}

static HRESULT media_item_create_source_node(struct media_item *item, IMFStreamDescriptor *sd,
        IMFTopologyNode **node)
{
    HRESULT hr;

    if (SUCCEEDED(hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, node)))
    {
        IMFTopologyNode_SetUnknown(*node, &MF_TOPONODE_SOURCE, (IUnknown *)item->source);
        IMFTopologyNode_SetUnknown(*node, &MF_TOPONODE_PRESENTATION_DESCRIPTOR, (IUnknown *)item->pd);
        IMFTopologyNode_SetUnknown(*node, &MF_TOPONODE_STREAM_DESCRIPTOR, (IUnknown *)sd);
        if (item->start_position)
            IMFTopologyNode_SetUINT64(*node, &MF_TOPONODE_MEDIASTART, item->start_position);
        if (item->stop_position)
            IMFTopologyNode_SetUINT64(*node, &MF_TOPONODE_MEDIASTOP, item->stop_position);
    }

    return hr;
}

static HRESULT media_item_create_sink_node(IUnknown *sink, IMFTopologyNode **node)
{
    HRESULT hr;

    if (SUCCEEDED(hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, node)))
        IMFTopologyNode_SetObject(*node, sink);

    return hr;
}

static HRESULT media_item_create_topology(struct media_player *player, struct media_item *item, IMFTopology **out)
{
    BOOL selected, video_added = FALSE;
    IMFStreamDescriptor *sd;
    IMFTopology *topology;
    unsigned int idx;
    IUnknown *sink;
    HRESULT hr;
    GUID major;

    if (FAILED(hr = MFCreateTopology(&topology)))
        return hr;

    /* Set up branches for all selected streams. */

    idx = 0;
    while (SUCCEEDED(IMFPresentationDescriptor_GetStreamDescriptorByIndex(item->pd, idx++, &selected, &sd)))
    {
        if (!selected || FAILED(media_item_get_stream_type(sd, &major)))
        {
            IMFStreamDescriptor_Release(sd);
            continue;
        }

        sink = NULL;

        if (SUCCEEDED(IMFStreamDescriptor_GetUnknown(sd, &_MF_CUSTOM_SINK, &IID_IUnknown, (void **)&sink)))
        {
            /* User sink is attached as-is. */
        }
        else if (IsEqualGUID(&major, &MFMediaType_Audio))
        {
            if (FAILED(hr = MFCreateAudioRendererActivate((IMFActivate **)&sink)))
                WARN("Failed to create SAR activation object, hr %#lx.\n", hr);
        }
        else if (IsEqualGUID(&major, &MFMediaType_Video) && player->output_window && !video_added)
        {
            if (FAILED(hr = MFCreateVideoRendererActivate(player->output_window, (IMFActivate **)&sink)))
                WARN("Failed to create EVR activation object, hr %#lx.\n", hr);
            video_added = SUCCEEDED(hr);
        }

        if (sink)
        {
            IMFTopologyNode *src_node = NULL, *sink_node = NULL;

            hr = media_item_create_source_node(item, sd, &src_node);
            if (SUCCEEDED(hr))
                hr = media_item_create_sink_node(sink, &sink_node);

            if (SUCCEEDED(hr))
            {
                IMFTopology_AddNode(topology, src_node);
                IMFTopology_AddNode(topology, sink_node);
                IMFTopologyNode_ConnectOutput(src_node, 0, sink_node, 0);
            }

            if (src_node)
                IMFTopologyNode_Release(src_node);
            if (sink_node)
                IMFTopologyNode_Release(sink_node);

            IUnknown_Release(sink);
        }

        IMFStreamDescriptor_Release(sd);
    }

    IMFTopology_SetUINT32(topology, &MF_TOPOLOGY_ENUMERATE_SOURCE_TYPES, TRUE);

    *out = topology;

    return S_OK;
}

static HRESULT WINAPI media_player_SetMediaItem(IMFPMediaPlayer *iface, IMFPMediaItem *item_iface)
{
    struct media_player *player = impl_from_IMFPMediaPlayer(iface);
    struct media_item *item;
    IMFTopology *topology;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, item_iface);

    if (!item_iface)
        return E_POINTER;

    item = unsafe_impl_from_IMFPMediaItem(item_iface);
    if (item->player != player)
        return E_INVALIDARG;

    if (FAILED(hr = media_item_create_topology(player, item, &topology)))
        return hr;

    IMFTopology_SetUnknown(topology, &_MF_TOPO_MEDIA_ITEM, (IUnknown *)item_iface);
    hr = IMFMediaSession_SetTopology(player->session, MFSESSION_SETTOPOLOGY_IMMEDIATE, topology);
    IMFTopology_Release(topology);

    return hr;
}

static HRESULT WINAPI media_player_ClearMediaItem(IMFPMediaPlayer *iface)
{
    struct media_player *player = impl_from_IMFPMediaPlayer(iface);

    TRACE("%p.\n", iface);

    return IMFMediaSession_SetTopology(player->session, MFSESSION_SETTOPOLOGY_CLEAR_CURRENT, NULL);
}

static HRESULT WINAPI media_player_GetMediaItem(IMFPMediaPlayer *iface, IMFPMediaItem **item)
{
    struct media_player *player = impl_from_IMFPMediaPlayer(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, item);

    if (!item)
        return E_POINTER;

    EnterCriticalSection(&player->cs);
    if (player->state == MFP_MEDIAPLAYER_STATE_SHUTDOWN)
        hr = MF_E_SHUTDOWN;
    else if (!player->item)
        hr = MF_E_NOT_FOUND;
    else
    {
        *item = player->item;
        IMFPMediaItem_AddRef(player->item);
    }
    LeaveCriticalSection(&player->cs);

    return hr;
}

static HRESULT WINAPI media_player_GetVolume(IMFPMediaPlayer *iface, float *volume)
{
    FIXME("%p, %p.\n", iface, volume);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_player_SetVolume(IMFPMediaPlayer *iface, float volume)
{
    FIXME("%p, %.8e.\n", iface, volume);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_player_GetBalance(IMFPMediaPlayer *iface, float *balance)
{
    FIXME("%p, %p.\n", iface, balance);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_player_SetBalance(IMFPMediaPlayer *iface, float balance)
{
    FIXME("%p, %.8e.\n", iface, balance);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_player_GetMute(IMFPMediaPlayer *iface, BOOL *mute)
{
    FIXME("%p, %p.\n", iface, mute);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_player_SetMute(IMFPMediaPlayer *iface, BOOL mute)
{
    FIXME("%p, %d.\n", iface, mute);

    return E_NOTIMPL;
}

static HRESULT media_player_get_display_control(const struct media_player *player,
        IMFVideoDisplayControl **display_control)
{
    HRESULT hr = MFGetService((IUnknown *)player->session, &MR_VIDEO_RENDER_SERVICE,
            &IID_IMFVideoDisplayControl, (void **)display_control);
    if (SUCCEEDED(hr)) return hr;
    return hr == MF_E_SHUTDOWN ? hr : MF_E_INVALIDREQUEST;
}

static HRESULT WINAPI media_player_GetNativeVideoSize(IMFPMediaPlayer *iface,
        SIZE *video, SIZE *arvideo)
{
    struct media_player *player = impl_from_IMFPMediaPlayer(iface);
    IMFVideoDisplayControl *display_control;
    HRESULT hr;

    TRACE("%p, %p, %p.\n", iface, video, arvideo);

    if (SUCCEEDED(hr = media_player_get_display_control(player, &display_control)))
    {
        hr = IMFVideoDisplayControl_GetNativeVideoSize(display_control, video, arvideo);
        IMFVideoDisplayControl_Release(display_control);
    }

    return hr;
}

static HRESULT WINAPI media_player_GetIdealVideoSize(IMFPMediaPlayer *iface,
        SIZE *min_size, SIZE *max_size)
{
    struct media_player *player = impl_from_IMFPMediaPlayer(iface);
    IMFVideoDisplayControl *display_control;
    HRESULT hr;

    TRACE("%p, %p, %p.\n", iface, min_size, max_size);

    if (SUCCEEDED(hr = media_player_get_display_control(player, &display_control)))
    {
        hr = IMFVideoDisplayControl_GetIdealVideoSize(display_control, min_size, max_size);
        IMFVideoDisplayControl_Release(display_control);
    }

    return hr;
}

static HRESULT WINAPI media_player_SetVideoSourceRect(IMFPMediaPlayer *iface,
        MFVideoNormalizedRect const *rect)
{
    struct media_player *player = impl_from_IMFPMediaPlayer(iface);
    IMFVideoDisplayControl *display_control;
    RECT dst_rect;
    HRESULT hr;

    TRACE("%p, %s.\n", iface, debugstr_normalized_rect(rect));

    if (!GetClientRect(player->output_window, &dst_rect))
        hr = HRESULT_FROM_WIN32(GetLastError());
    else if (SUCCEEDED(hr = media_player_get_display_control(player, &display_control)))
    {
        hr = IMFVideoDisplayControl_SetVideoPosition(display_control, rect, &dst_rect);
        IMFVideoDisplayControl_Release(display_control);
    }

    return hr;
}

static HRESULT WINAPI media_player_GetVideoSourceRect(IMFPMediaPlayer *iface,
        MFVideoNormalizedRect *rect)
{
    struct media_player *player = impl_from_IMFPMediaPlayer(iface);
    IMFVideoDisplayControl *display_control;
    HRESULT hr;
    RECT dest;

    TRACE("%p, %p.\n", iface, rect);

    if (SUCCEEDED(hr = media_player_get_display_control(player, &display_control)))
    {
        hr = IMFVideoDisplayControl_GetVideoPosition(display_control, rect, &dest);
        IMFVideoDisplayControl_Release(display_control);
    }

    return hr;
}

static HRESULT WINAPI media_player_SetAspectRatioMode(IMFPMediaPlayer *iface, DWORD mode)
{
    struct media_player *player = impl_from_IMFPMediaPlayer(iface);
    IMFVideoDisplayControl *display_control;
    HRESULT hr;

    TRACE("%p, %lu.\n", iface, mode);

    if (SUCCEEDED(hr = media_player_get_display_control(player, &display_control)))
    {
        hr = IMFVideoDisplayControl_SetAspectRatioMode(display_control, mode);
        IMFVideoDisplayControl_Release(display_control);
    }

    return hr;
}

static HRESULT WINAPI media_player_GetAspectRatioMode(IMFPMediaPlayer *iface,
        DWORD *mode)
{
    struct media_player *player = impl_from_IMFPMediaPlayer(iface);
    IMFVideoDisplayControl *display_control;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, mode);

    if (SUCCEEDED(hr = media_player_get_display_control(player, &display_control)))
    {
        hr = IMFVideoDisplayControl_GetAspectRatioMode(display_control, mode);
        IMFVideoDisplayControl_Release(display_control);
    }

    return hr;
}

static HRESULT WINAPI media_player_GetVideoWindow(IMFPMediaPlayer *iface, HWND *window)
{
    struct media_player *player = impl_from_IMFPMediaPlayer(iface);

    TRACE("%p, %p.\n", iface, window);

    *window = player->output_window;

    return S_OK;
}

static HRESULT WINAPI media_player_UpdateVideo(IMFPMediaPlayer *iface)
{
    struct media_player *player = impl_from_IMFPMediaPlayer(iface);
    IMFVideoDisplayControl *display_control;
    HRESULT hr;
    RECT rect;

    TRACE("%p.\n", iface);

    if (SUCCEEDED(hr = media_player_get_display_control(player, &display_control)))
    {
        if (GetClientRect(player->output_window, &rect))
            hr = IMFVideoDisplayControl_SetVideoPosition(display_control, NULL, &rect);
        if (SUCCEEDED(hr))
            hr = IMFVideoDisplayControl_RepaintVideo(display_control);
        IMFVideoDisplayControl_Release(display_control);
    }

    return hr;
}

static HRESULT WINAPI media_player_SetBorderColor(IMFPMediaPlayer *iface, COLORREF color)
{
    struct media_player *player = impl_from_IMFPMediaPlayer(iface);
    IMFVideoDisplayControl *display_control;
    HRESULT hr;

    TRACE("%p, %#lx.\n", iface, color);

    if (SUCCEEDED(hr = media_player_get_display_control(player, &display_control)))
    {
        hr = IMFVideoDisplayControl_SetBorderColor(display_control, color);
        IMFVideoDisplayControl_Release(display_control);
    }

    return hr;
}

static HRESULT WINAPI media_player_GetBorderColor(IMFPMediaPlayer *iface, COLORREF *color)
{
    struct media_player *player = impl_from_IMFPMediaPlayer(iface);
    IMFVideoDisplayControl *display_control;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, color);

    if (SUCCEEDED(hr = media_player_get_display_control(player, &display_control)))
    {
        hr = IMFVideoDisplayControl_GetBorderColor(display_control, color);
        IMFVideoDisplayControl_Release(display_control);
    }

    return hr;
}

static HRESULT WINAPI media_player_InsertEffect(IMFPMediaPlayer *iface, IUnknown *effect,
        BOOL optional)
{
    FIXME("%p, %p, %d.\n", iface, effect, optional);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_player_RemoveEffect(IMFPMediaPlayer *iface, IUnknown *effect)
{
    FIXME("%p, %p.\n", iface, effect);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_player_RemoveAllEffects(IMFPMediaPlayer *iface)
{
    FIXME("%p.\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_player_Shutdown(IMFPMediaPlayer *iface)
{
    struct media_player *player = impl_from_IMFPMediaPlayer(iface);

    TRACE("%p.\n", iface);

    EnterCriticalSection(&player->cs);
    media_player_set_state(player, MFP_MEDIAPLAYER_STATE_SHUTDOWN);
    if (player->item)
    {
        IMFPMediaItem_Release(player->item);
        player->item = NULL;
    }
    LeaveCriticalSection(&player->cs);

    return S_OK;
}

static const IMFPMediaPlayerVtbl media_player_vtbl =
{
    media_player_QueryInterface,
    media_player_AddRef,
    media_player_Release,
    media_player_Play,
    media_player_Pause,
    media_player_Stop,
    media_player_FrameStep,
    media_player_SetPosition,
    media_player_GetPosition,
    media_player_GetDuration,
    media_player_SetRate,
    media_player_GetRate,
    media_player_GetSupportedRates,
    media_player_GetState,
    media_player_CreateMediaItemFromURL,
    media_player_CreateMediaItemFromObject,
    media_player_SetMediaItem,
    media_player_ClearMediaItem,
    media_player_GetMediaItem,
    media_player_GetVolume,
    media_player_SetVolume,
    media_player_GetBalance,
    media_player_SetBalance,
    media_player_GetMute,
    media_player_SetMute,
    media_player_GetNativeVideoSize,
    media_player_GetIdealVideoSize,
    media_player_SetVideoSourceRect,
    media_player_GetVideoSourceRect,
    media_player_SetAspectRatioMode,
    media_player_GetAspectRatioMode,
    media_player_GetVideoWindow,
    media_player_UpdateVideo,
    media_player_SetBorderColor,
    media_player_GetBorderColor,
    media_player_InsertEffect,
    media_player_RemoveEffect,
    media_player_RemoveAllEffects,
    media_player_Shutdown,
};

static HRESULT WINAPI media_player_propstore_QueryInterface(IPropertyStore *iface,
        REFIID riid, void **obj)
{
    struct media_player *player = impl_from_IPropertyStore(iface);
    return IMFPMediaPlayer_QueryInterface(&player->IMFPMediaPlayer_iface, riid, obj);
}

static ULONG WINAPI media_player_propstore_AddRef(IPropertyStore *iface)
{
    struct media_player *player = impl_from_IPropertyStore(iface);
    return IMFPMediaPlayer_AddRef(&player->IMFPMediaPlayer_iface);
}

static ULONG WINAPI media_player_propstore_Release(IPropertyStore *iface)
{
    struct media_player *player = impl_from_IPropertyStore(iface);
    return IMFPMediaPlayer_Release(&player->IMFPMediaPlayer_iface);
}

static HRESULT WINAPI media_player_propstore_GetCount(IPropertyStore *iface, DWORD *count)
{
    struct media_player *player = impl_from_IPropertyStore(iface);

    TRACE("%p, %p.\n", iface, count);

    return IPropertyStore_GetCount(player->propstore, count);
}

static HRESULT WINAPI media_player_propstore_GetAt(IPropertyStore *iface, DWORD prop, PROPERTYKEY *key)
{
    struct media_player *player = impl_from_IPropertyStore(iface);

    TRACE("%p, %lu, %p.\n", iface, prop, key);

    return IPropertyStore_GetAt(player->propstore, prop, key);
}

static HRESULT WINAPI media_player_propstore_GetValue(IPropertyStore *iface, REFPROPERTYKEY key, PROPVARIANT *value)
{
    struct media_player *player = impl_from_IPropertyStore(iface);

    TRACE("%p, %p, %p.\n", iface, key, value);

    return IPropertyStore_GetValue(player->propstore, key, value);
}

static HRESULT WINAPI media_player_propstore_SetValue(IPropertyStore *iface, REFPROPERTYKEY key, REFPROPVARIANT value)
{
    struct media_player *player = impl_from_IPropertyStore(iface);

    TRACE("%p, %p, %p.\n", iface, key, value);

    return IPropertyStore_SetValue(player->propstore, key, value);
}

static HRESULT WINAPI media_player_propstore_Commit(IPropertyStore *iface)
{
    struct media_player *player = impl_from_IPropertyStore(iface);

    TRACE("%p.\n", iface);

    return IPropertyStore_Commit(player->propstore);
}

static const IPropertyStoreVtbl media_player_propstore_vtbl =
{
    media_player_propstore_QueryInterface,
    media_player_propstore_AddRef,
    media_player_propstore_Release,
    media_player_propstore_GetCount,
    media_player_propstore_GetAt,
    media_player_propstore_GetValue,
    media_player_propstore_SetValue,
    media_player_propstore_Commit,
};

static HRESULT WINAPI media_player_callback_QueryInterface(IMFAsyncCallback *iface,
        REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFAsyncCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFAsyncCallback_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI media_player_resolver_callback_AddRef(IMFAsyncCallback *iface)
{
    struct media_player *player = impl_from_resolver_IMFAsyncCallback(iface);
    return IMFPMediaPlayer_AddRef(&player->IMFPMediaPlayer_iface);
}

static ULONG WINAPI media_player_resolver_callback_Release(IMFAsyncCallback *iface)
{
    struct media_player *player = impl_from_resolver_IMFAsyncCallback(iface);
    return IMFPMediaPlayer_Release(&player->IMFPMediaPlayer_iface);
}

static HRESULT WINAPI media_player_callback_GetParameters(IMFAsyncCallback *iface, DWORD *flags,
        DWORD *queue)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI media_player_resolver_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct media_player *player = impl_from_resolver_IMFAsyncCallback(iface);
    struct media_event *event;
    IUnknown *object, *state;
    MF_OBJECT_TYPE obj_type;
    struct media_item *item;
    HRESULT hr;

    if (FAILED(IMFAsyncResult_GetState(result, &state)))
        return S_OK;

    item = impl_from_IMFPMediaItem((IMFPMediaItem *)state);

    if (item->object)
    {
        if (FAILED(hr = IUnknown_QueryInterface(item->object, &IID_IMFMediaSource, (void **)&object)))
            hr = IMFSourceResolver_EndCreateObjectFromByteStream(player->resolver, result, &obj_type, &object);
    }
    else
        hr = IMFSourceResolver_EndCreateObjectFromURL(player->resolver, result, &obj_type, &object);

    if (SUCCEEDED(hr))
    {
        hr = media_item_set_source(item, object);
        IUnknown_Release(object);
    }

    if (FAILED(hr))
        WARN("Failed to set media source, hr %#lx.\n", hr);

    if (FAILED(media_event_create(player, MFP_EVENT_TYPE_MEDIAITEM_CREATED, hr,
            &item->IMFPMediaItem_iface, &event)))
    {
        WARN("Failed to create event object.\n");
        IUnknown_Release(state);
        return S_OK;
    }
    event->u.item_created.dwUserData = item->user_data;

    media_player_queue_event(player, event);

    IUnknown_Release(&event->IUnknown_iface);
    IUnknown_Release(state);

    return S_OK;
}

static const IMFAsyncCallbackVtbl media_player_resolver_callback_vtbl =
{
    media_player_callback_QueryInterface,
    media_player_resolver_callback_AddRef,
    media_player_resolver_callback_Release,
    media_player_callback_GetParameters,
    media_player_resolver_callback_Invoke,
};

static ULONG WINAPI media_player_events_callback_AddRef(IMFAsyncCallback *iface)
{
    struct media_player *player = impl_from_events_IMFAsyncCallback(iface);
    return IMFPMediaPlayer_AddRef(&player->IMFPMediaPlayer_iface);
}

static ULONG WINAPI media_player_events_callback_Release(IMFAsyncCallback *iface)
{
    struct media_player *player = impl_from_events_IMFAsyncCallback(iface);
    return IMFPMediaPlayer_Release(&player->IMFPMediaPlayer_iface);
}

static HRESULT WINAPI media_player_events_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct media_player *player = impl_from_events_IMFAsyncCallback(iface);
    struct media_event *event;
    IUnknown *state;

    if (FAILED(IMFAsyncResult_GetState(result, &state)))
        return S_OK;

    event = impl_event_from_IUnknown(state);

    if (player->callback)
        IMFPMediaPlayerCallback_OnMediaPlayerEvent(player->callback, &event->u.header);

    IUnknown_Release(state);

    return S_OK;
}

static const IMFAsyncCallbackVtbl media_player_events_callback_vtbl =
{
    media_player_callback_QueryInterface,
    media_player_events_callback_AddRef,
    media_player_events_callback_Release,
    media_player_callback_GetParameters,
    media_player_events_callback_Invoke,
};

static ULONG WINAPI media_player_session_events_callback_AddRef(IMFAsyncCallback *iface)
{
    struct media_player *player = impl_from_session_events_IMFAsyncCallback(iface);
    return IMFPMediaPlayer_AddRef(&player->IMFPMediaPlayer_iface);
}

static ULONG WINAPI media_player_session_events_callback_Release(IMFAsyncCallback *iface)
{
    struct media_player *player = impl_from_session_events_IMFAsyncCallback(iface);
    return IMFPMediaPlayer_Release(&player->IMFPMediaPlayer_iface);
}

static void media_player_change_state(struct media_player *player, MFP_MEDIAPLAYER_STATE state,
        HRESULT event_status, struct media_event **event)
{
    MFP_EVENT_TYPE event_type;

    EnterCriticalSection(&player->cs);

    if (state == MFP_MEDIAPLAYER_STATE_PLAYING)
        event_type = MFP_EVENT_TYPE_PLAY;
    else if (state == MFP_MEDIAPLAYER_STATE_PAUSED)
        event_type = MFP_EVENT_TYPE_PAUSE;
    else
        event_type = MFP_EVENT_TYPE_STOP;

    media_player_set_state(player, state);
    media_event_create(player, event_type, event_status, player->item, event);

    LeaveCriticalSection(&player->cs);
}

static void media_player_set_item(struct media_player *player, IMFTopology *topology, HRESULT event_status,
        struct media_event **event)
{
    IMFPMediaItem *item;

    if (FAILED(IMFTopology_GetUnknown(topology, &_MF_TOPO_MEDIA_ITEM, &IID_IMFPMediaItem, (void **)&item)))
        return;

    EnterCriticalSection(&player->cs);

    if (player->item)
        IMFPMediaItem_Release(player->item);
    player->item = item;
    IMFPMediaItem_AddRef(player->item);

    media_event_create(player, MFP_EVENT_TYPE_MEDIAITEM_SET, event_status, item, event);

    LeaveCriticalSection(&player->cs);

    IMFPMediaItem_Release(item);
}

static void media_player_clear_item(struct media_player *player, HRESULT event_status,
        struct media_event **event)
{
    IMFPMediaItem *item;

    EnterCriticalSection(&player->cs);

    item = player->item;
    player->item = NULL;

    media_event_create(player, MFP_EVENT_TYPE_MEDIAITEM_SET, event_status, item, event);

    LeaveCriticalSection(&player->cs);
}

static void media_player_create_forward_event(struct media_player *player, HRESULT event_status, IMFMediaEvent *session_event,
        struct media_event **event)
{
    EnterCriticalSection(&player->cs);

    if (SUCCEEDED(media_event_create(player, MFP_EVENT_TYPE_MF, event_status, player->item, event)))
    {
        IMFMediaEvent_GetType(session_event, &(*event)->u.event.MFEventType);
        (*event)->u.event.pMFMediaEvent = session_event;
        IMFMediaEvent_AddRef((*event)->u.event.pMFMediaEvent);
    }

    LeaveCriticalSection(&player->cs);
}

static void media_player_playback_ended(struct media_player *player, HRESULT event_status,
        struct media_event **event)
{
    EnterCriticalSection(&player->cs);

    media_player_set_state(player, MFP_MEDIAPLAYER_STATE_STOPPED);
    media_event_create(player, MFP_EVENT_TYPE_PLAYBACK_ENDED, event_status, player->item, event);

    LeaveCriticalSection(&player->cs);
}

static void media_player_rate_changed(struct media_player *player, HRESULT event_status,
        float rate, struct media_event **event)
{
    EnterCriticalSection(&player->cs);

    if (SUCCEEDED(media_event_create(player, MFP_EVENT_TYPE_RATE_SET, event_status, player->item, event)))
        (*event)->u.rate_set.flRate = rate;

    LeaveCriticalSection(&player->cs);
}

static HRESULT WINAPI media_player_session_events_callback_Invoke(IMFAsyncCallback *iface,
        IMFAsyncResult *result)
{
    struct media_player *player = impl_from_session_events_IMFAsyncCallback(iface);
    MediaEventType session_event_type = MEUnknown;
    struct media_event *event = NULL;
    IMFMediaEvent *session_event;
    MFP_MEDIAPLAYER_STATE state;
    HRESULT hr, event_status;
    IMFTopology *topology;
    unsigned int status;
    PROPVARIANT value;
    float rate;

    if (FAILED(hr = IMFMediaSession_EndGetEvent(player->session, result, &session_event)))
        return S_OK;

    IMFMediaEvent_GetType(session_event, &session_event_type);
    IMFMediaEvent_GetStatus(session_event, &event_status);

    switch (session_event_type)
    {
        case MESessionStarted:
        case MESessionStopped:
        case MESessionPaused:

            if (session_event_type == MESessionStarted)
                state = MFP_MEDIAPLAYER_STATE_PLAYING;
            else if (session_event_type == MESessionPaused)
                state = MFP_MEDIAPLAYER_STATE_PAUSED;
            else
                state = MFP_MEDIAPLAYER_STATE_STOPPED;

            media_player_change_state(player, state, event_status, &event);

            break;

        case MESessionTopologySet:

            value.vt = VT_EMPTY;
            if (SUCCEEDED(IMFMediaEvent_GetValue(session_event, &value)))
            {
                if (value.vt == VT_EMPTY)
                {
                    media_player_clear_item(player, event_status, &event);
                }
                else if (value.vt == VT_UNKNOWN && value.punkVal &&
                        SUCCEEDED(IUnknown_QueryInterface(value.punkVal, &IID_IMFTopology, (void **)&topology)))
                {
                    media_player_set_item(player, topology, event_status, &event);
                    IMFTopology_Release(topology);
                }
                PropVariantClear(&value);
            }

            break;

        case MESessionTopologyStatus:

            if (SUCCEEDED(IMFMediaEvent_GetUINT32(session_event, &MF_EVENT_TOPOLOGY_STATUS, &status)) &&
                    status == MF_TOPOSTATUS_ENDED)
            {
                media_player_playback_ended(player, event_status, &event);
            }

            break;

        case MESessionRateChanged:

            rate = 0.0f;
            if (SUCCEEDED(IMFMediaEvent_GetValue(session_event, &value)))
            {
                if (value.vt == VT_R4)
                    rate = value.fltVal;
                PropVariantClear(&value);
            }

            media_player_rate_changed(player, event_status, rate, &event);

            break;

        case MEBufferingStarted:
        case MEBufferingStopped:
        case MEExtendedType:
        case MEReconnectStart:
        case MEReconnectEnd:
        case MERendererEvent:
        case MEStreamSinkFormatChanged:

            media_player_create_forward_event(player, event_status, session_event, &event);

            break;

        case MEError:

            media_event_create(player, MFP_EVENT_TYPE_ERROR, event_status, NULL, &event);

            break;

        default:
            ;
    }

    if (event)
    {
        media_player_queue_event(player, event);
        IUnknown_Release(&event->IUnknown_iface);
    }

    IMFMediaSession_BeginGetEvent(player->session, &player->session_events_callback, NULL);
    IMFMediaEvent_Release(session_event);

    return S_OK;
}

static const IMFAsyncCallbackVtbl media_player_session_events_callback_vtbl =
{
    media_player_callback_QueryInterface,
    media_player_session_events_callback_AddRef,
    media_player_session_events_callback_Release,
    media_player_callback_GetParameters,
    media_player_session_events_callback_Invoke,
};

/***********************************************************************
 *      MFPCreateMediaPlayer (mfplay.@)
 */
HRESULT WINAPI MFPCreateMediaPlayer(const WCHAR *url, BOOL start_playback, MFP_CREATION_OPTIONS options,
        IMFPMediaPlayerCallback *callback, HWND window, IMFPMediaPlayer **player)
{
    struct media_player *object;
    IMFPMediaItem *item;
    HRESULT hr;

    TRACE("%s, %d, %#x, %p, %p, %p.\n", debugstr_w(url), start_playback, options, callback, window, player);

    if (!player)
        return E_POINTER;

    *player = NULL;

    if (!url && start_playback)
        return E_INVALIDARG;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    platform_startup();

    object->IMFPMediaPlayer_iface.lpVtbl = &media_player_vtbl;
    object->IPropertyStore_iface.lpVtbl = &media_player_propstore_vtbl;
    object->resolver_callback.lpVtbl = &media_player_resolver_callback_vtbl;
    object->events_callback.lpVtbl = &media_player_events_callback_vtbl;
    object->session_events_callback.lpVtbl = &media_player_session_events_callback_vtbl;
    object->refcount = 1;
    object->callback = callback;
    if (object->callback)
        IMFPMediaPlayerCallback_AddRef(object->callback);
    object->options = options;
    object->output_window = window;
    InitializeCriticalSection(&object->cs);
    if (FAILED(hr = CreatePropertyStore(&object->propstore)))
        goto failed;
    if (FAILED(hr = MFCreateSourceResolver(&object->resolver)))
        goto failed;
    if (FAILED(hr = MFCreateMediaSession(NULL, &object->session)))
        goto failed;
    if (FAILED(hr = IMFMediaSession_BeginGetEvent(object->session, &object->session_events_callback, NULL)))
        goto failed;
    if (!(object->options & MFP_OPTION_FREE_THREADED_CALLBACK))
    {
        object->event_window = CreateWindowW(eventclassW, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE,
                0, mfplay_instance, NULL);
    }

    if (url)
    {
        if (FAILED(hr = media_player_create_item_from_url(object, url, TRUE, 0, &item)))
        {
            WARN("Failed to create media item, hr %#lx.\n", hr);
            goto failed;
        }

        hr = IMFPMediaPlayer_SetMediaItem(&object->IMFPMediaPlayer_iface, item);
        IMFPMediaItem_Release(item);
        if (FAILED(hr))
        {
            WARN("Failed to set media item, hr %#lx.\n", hr);
            goto failed;
        }

        if (start_playback)
            IMFPMediaPlayer_Play(&object->IMFPMediaPlayer_iface);
    }

    *player = &object->IMFPMediaPlayer_iface;

    return S_OK;

failed:

    IMFPMediaPlayer_Release(&object->IMFPMediaPlayer_iface);

    return hr;
}

static void media_player_register_window_class(void)
{
    WNDCLASSW cls = { 0 };

    cls.lpfnWndProc = media_player_event_proc;
    cls.hInstance = mfplay_instance;
    cls.lpszClassName = eventclassW;

    RegisterClassW(&cls);
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved)
{
    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
            mfplay_instance = instance;
            DisableThreadLibraryCalls(instance);
            media_player_register_window_class();
            break;
        case DLL_PROCESS_DETACH:
            if (reserved) break;
            UnregisterClassW(eventclassW, instance);
            break;
    }

    return TRUE;
}
