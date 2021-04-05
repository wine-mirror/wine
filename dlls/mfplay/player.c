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

#include "windef.h"
#include "winbase.h"
#include "mfapi.h"
#include "mfplay.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

static LONG startup_refcount;

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

struct media_player
{
    IMFPMediaPlayer IMFPMediaPlayer_iface;
    IPropertyStore IPropertyStore_iface;
    LONG refcount;
    IMFPMediaPlayerCallback *callback;
    IPropertyStore *propstore;
};

static struct media_player *impl_from_IMFPMediaPlayer(IMFPMediaPlayer *iface)
{
    return CONTAINING_RECORD(iface, struct media_player, IMFPMediaPlayer_iface);
}

static struct media_player *impl_from_IPropertyStore(IPropertyStore *iface)
{
    return CONTAINING_RECORD(iface, struct media_player, IPropertyStore_iface);
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

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI media_player_Release(IMFPMediaPlayer *iface)
{
    struct media_player *player = impl_from_IMFPMediaPlayer(iface);
    ULONG refcount = InterlockedDecrement(&player->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        if (player->callback)
            IMFPMediaPlayerCallback_Release(player->callback);
        if (player->propstore)
            IPropertyStore_Release(player->propstore);
        heap_free(player);

        platform_shutdown();
    }

    return refcount;
}

static HRESULT WINAPI media_player_Play(IMFPMediaPlayer *iface)
{
    FIXME("%p.\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_player_Pause(IMFPMediaPlayer *iface)
{
    FIXME("%p.\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_player_Stop(IMFPMediaPlayer *iface)
{
    FIXME("%p.\n", iface);

    return E_NOTIMPL;
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
    FIXME("%p, %s, %p.\n", iface, debugstr_guid(postype), position);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_player_GetDuration(IMFPMediaPlayer *iface, REFGUID postype, PROPVARIANT *position)
{
    FIXME("%p, %s, %p.\n", iface, debugstr_guid(postype), position);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_player_SetRate(IMFPMediaPlayer *iface, float rate)
{
    FIXME("%p, %f.\n", iface, rate);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_player_GetRate(IMFPMediaPlayer *iface, float *rate)
{
    FIXME("%p, %p.\n", iface, rate);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_player_GetSupportedRates(IMFPMediaPlayer *iface, BOOL forward, float *slowest_rate, float *fastest_rate)
{
    FIXME("%p, %d, %p, %p.\n", iface, forward, slowest_rate, fastest_rate);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_player_GetState(IMFPMediaPlayer *iface, MFP_MEDIAPLAYER_STATE *state)
{
    FIXME("%p, %p.\n", iface, state);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_player_CreateMediaItemFromURL(IMFPMediaPlayer *iface,
        const WCHAR *url, BOOL sync, DWORD_PTR user_data, IMFPMediaItem **item)
{
    FIXME("%p, %s, %d, %lx, %p.\n", iface, debugstr_w(url), sync, user_data, item);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_player_CreateMediaItemFromObject(IMFPMediaPlayer *iface,
        IUnknown *object, BOOL sync, DWORD_PTR user_data, IMFPMediaItem **item)
{
    FIXME("%p, %p, %d, %lx, %p.\n", iface, object, sync, user_data, item);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_player_SetMediaItem(IMFPMediaPlayer *iface, IMFPMediaItem *item)
{
    FIXME("%p, %p.\n", iface, item);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_player_ClearMediaItem(IMFPMediaPlayer *iface)
{
    FIXME("%p.\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_player_GetMediaItem(IMFPMediaPlayer *iface, IMFPMediaItem **item)
{
    FIXME("%p, %p.\n", iface, item);

    return E_NOTIMPL;
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

static HRESULT WINAPI media_player_GetNativeVideoSize(IMFPMediaPlayer *iface,
        SIZE *video, SIZE *arvideo)
{
    FIXME("%p, %p, %p.\n", iface, video, arvideo);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_player_GetIdealVideoSize(IMFPMediaPlayer *iface,
        SIZE *min_size, SIZE *max_size)
{
    FIXME("%p, %p, %p.\n", iface, min_size, max_size);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_player_SetVideoSourceRect(IMFPMediaPlayer *iface,
        MFVideoNormalizedRect const *rect)
{
    FIXME("%p, %p.\n", iface, rect);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_player_GetVideoSourceRect(IMFPMediaPlayer *iface,
        MFVideoNormalizedRect *rect)
{
    FIXME("%p, %p.\n", iface, rect);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_player_SetAspectRatioMode(IMFPMediaPlayer *iface, DWORD mode)
{
    FIXME("%p, %u.\n", iface, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_player_GetAspectRatioMode(IMFPMediaPlayer *iface,
        DWORD *mode)
{
    FIXME("%p, %p.\n", iface, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_player_GetVideoWindow(IMFPMediaPlayer *iface, HWND *hwnd)
{
    FIXME("%p, %p.\n", iface, hwnd);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_player_UpdateVideo(IMFPMediaPlayer *iface)
{
    FIXME("%p.\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_player_SetBorderColor(IMFPMediaPlayer *iface, COLORREF color)
{
    FIXME("%p, %#x.\n", iface, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_player_GetBorderColor(IMFPMediaPlayer *iface, COLORREF *color)
{
    FIXME("%p, %p.\n", iface, color);

    return E_NOTIMPL;
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
    FIXME("%p.\n", iface);

    return E_NOTIMPL;
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

    TRACE("%p, %u, %p.\n", iface, prop, key);

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

HRESULT WINAPI MFPCreateMediaPlayer(const WCHAR *url, BOOL start_playback, MFP_CREATION_OPTIONS options,
        IMFPMediaPlayerCallback *callback, HWND hwnd, IMFPMediaPlayer **player)
{
    struct media_player *object;
    HRESULT hr;

    TRACE("%s, %d, %#x, %p, %p, %p.\n", debugstr_w(url), start_playback, options, callback, hwnd, player);

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    platform_startup();

    object->IMFPMediaPlayer_iface.lpVtbl = &media_player_vtbl;
    object->IPropertyStore_iface.lpVtbl = &media_player_propstore_vtbl;
    object->refcount = 1;
    object->callback = callback;
    if (object->callback)
        IMFPMediaPlayerCallback_AddRef(object->callback);
    if (FAILED(hr = CreatePropertyStore(&object->propstore)))
        goto failed;

    *player = &object->IMFPMediaPlayer_iface;

    return S_OK;

failed:

    IMFPMediaPlayer_Release(&object->IMFPMediaPlayer_iface);

    return hr;
}
