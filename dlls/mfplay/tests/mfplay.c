/*
 * Copyright 2021 Nikolay Sivov for CodeWeavers
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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "mfplay.h"
#include "mferror.h"

#include "wine/test.h"

#define check_interface(a, b, c) check_interface_(__LINE__, a, b, c)
static void check_interface_(unsigned int line, void *iface_ptr, REFIID iid, BOOL supported)
{
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected_hr;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#x, expected %#x.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);
}

static HRESULT WINAPI test_callback_QueryInterface(IMFPMediaPlayerCallback *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFPMediaPlayerCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFPMediaPlayerCallback_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_callback_AddRef(IMFPMediaPlayerCallback *iface)
{
    return 2;
}

static ULONG WINAPI test_callback_Release(IMFPMediaPlayerCallback *iface)
{
    return 1;
}

static void WINAPI test_callback_OnMediaPlayerEvent(IMFPMediaPlayerCallback *iface, MFP_EVENT_HEADER *event_header)
{
}

static const IMFPMediaPlayerCallbackVtbl test_callback_vtbl =
{
    test_callback_QueryInterface,
    test_callback_AddRef,
    test_callback_Release,
    test_callback_OnMediaPlayerEvent,
};

static void test_create_player(void)
{
    IMFPMediaPlayerCallback callback = { &test_callback_vtbl };
    IPropertyStore *propstore;
    IMFPMediaPlayer *player;
    IUnknown *unk, *unk2;
    HWND window;
    HRESULT hr;

    hr = MFPCreateMediaPlayer(NULL, FALSE, 0, NULL, NULL, &player);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    check_interface(player, &IID_IMFPMediaPlayer, TRUE);
    check_interface(player, &IID_IPropertyStore, TRUE);

    hr = IMFPMediaPlayer_QueryInterface(player, &IID_IPropertyStore, (void **)&propstore);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    check_interface(propstore, &IID_IMFPMediaPlayer, TRUE);

    hr = IPropertyStore_QueryInterface(propstore, &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    hr = IMFPMediaPlayer_QueryInterface(player, &IID_IUnknown, (void **)&unk2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(unk == unk2, "Unexpected interface.\n");
    IUnknown_Release(unk);
    IUnknown_Release(unk2);

    IPropertyStore_Release(propstore);

    IMFPMediaPlayer_Release(player);

    hr = MFPCreateMediaPlayer(NULL, FALSE, 0, &callback, (HWND)0x1, &player);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFPMediaPlayer_GetVideoWindow(player, &window);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(window == (HWND)0x1, "Unexpected window.\n");

    IMFPMediaPlayer_Release(player);
}

static void test_shutdown(void)
{
    SIZE min_size, max_size;
    IMFPMediaPlayer *player;
    float slowest, fastest;
    HRESULT hr;
    MFP_MEDIAPLAYER_STATE state;
    IMFPMediaItem *item;
    HWND window;

    hr = MFPCreateMediaPlayer(NULL, FALSE, 0, NULL, NULL, &player);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFPMediaPlayer_GetState(player, &state);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(state == MFP_MEDIAPLAYER_STATE_EMPTY, "Unexpected state %d.\n", state);

    hr = IMFPMediaPlayer_Shutdown(player);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    /* Check methods in shutdown state. */
    hr = IMFPMediaPlayer_Play(player);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFPMediaPlayer_Pause(player);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFPMediaPlayer_Stop(player);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFPMediaPlayer_GetSupportedRates(player, TRUE, &slowest, &fastest);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFPMediaPlayer_GetState(player, &state);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(state == MFP_MEDIAPLAYER_STATE_SHUTDOWN, "Unexpected state %d.\n", state);

    hr = IMFPMediaPlayer_GetIdealVideoSize(player, &min_size, &max_size);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFPMediaPlayer_CreateMediaItemFromURL(player, L"url", TRUE, 0, &item);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFPMediaPlayer_ClearMediaItem(player);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFPMediaPlayer_GetMediaItem(player, &item);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFPMediaPlayer_GetVideoWindow(player, &window);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFPMediaPlayer_Shutdown(player);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    IMFPMediaPlayer_Release(player);
}

START_TEST(mfplay)
{
    test_create_player();
    test_shutdown();
}
