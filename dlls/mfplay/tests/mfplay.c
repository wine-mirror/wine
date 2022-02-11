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
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#lx, expected %#lx.\n", hr, expected_hr);
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

    hr = MFPCreateMediaPlayer(NULL, FALSE, 0, NULL, NULL, NULL);
    ok(FAILED(hr), "Unexpected hr %#lx.\n", hr);

    hr = MFPCreateMediaPlayer(NULL, TRUE, 0, NULL, NULL, NULL);
    ok(FAILED(hr), "Unexpected hr %#lx.\n", hr);

    player = (void *)0xdeadbeef;
    hr = MFPCreateMediaPlayer(NULL, TRUE, 0, NULL, NULL, &player);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(!player, "Unexpected pointer %p.\n", player);

    hr = MFPCreateMediaPlayer(L"doesnotexist.mp4", FALSE, 0, &callback, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = MFPCreateMediaPlayer(L"doesnotexist.mp4", FALSE, 0, &callback, NULL, &player);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "Unexpected hr %#lx.\n", hr);

    hr = MFPCreateMediaPlayer(NULL, TRUE, 0, &callback, NULL, &player);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = MFPCreateMediaPlayer(NULL, FALSE, 0, NULL, NULL, &player);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_interface(player, &IID_IMFPMediaPlayer, TRUE);
    check_interface(player, &IID_IPropertyStore, TRUE);

    hr = IMFPMediaPlayer_QueryInterface(player, &IID_IPropertyStore, (void **)&propstore);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_interface(propstore, &IID_IMFPMediaPlayer, TRUE);

    hr = IPropertyStore_QueryInterface(propstore, &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFPMediaPlayer_QueryInterface(player, &IID_IUnknown, (void **)&unk2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(unk == unk2, "Unexpected interface.\n");
    IUnknown_Release(unk);
    IUnknown_Release(unk2);

    IPropertyStore_Release(propstore);

    IMFPMediaPlayer_Release(player);

    hr = MFPCreateMediaPlayer(NULL, FALSE, 0, &callback, (HWND)0x1, &player);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetVideoWindow(player, &window);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(window == (HWND)0x1, "Unexpected window.\n");

    IMFPMediaPlayer_Release(player);
}

static void test_shutdown(void)
{
    SIZE size, min_size, max_size;
    MFP_MEDIAPLAYER_STATE state;
    MFVideoNormalizedRect rect;
    IMFPMediaPlayer *player;
    float slowest, fastest;
    IMFPMediaItem *item;
    PROPVARIANT propvar;
    COLORREF color;
    HWND window;
    DWORD mode;
    HRESULT hr;

    hr = MFPCreateMediaPlayer(NULL, FALSE, 0, NULL, NULL, &player);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetState(player, &state);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(state == MFP_MEDIAPLAYER_STATE_EMPTY, "Unexpected state %d.\n", state);

    hr = IMFPMediaPlayer_Shutdown(player);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Check methods in shutdown state. */
    hr = IMFPMediaPlayer_Play(player);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_Pause(player);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_Stop(player);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetSupportedRates(player, TRUE, &slowest, &fastest);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetState(player, &state);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(state == MFP_MEDIAPLAYER_STATE_SHUTDOWN, "Unexpected state %d.\n", state);

    hr = IMFPMediaPlayer_GetIdealVideoSize(player, &min_size, &max_size);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetVideoSourceRect(player, &rect);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetBorderColor(player, &color);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetAspectRatioMode(player, &mode);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetNativeVideoSize(player, &size, &size);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_SetBorderColor(player, 0);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_SetAspectRatioMode(player, MFVideoARMode_None);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_UpdateVideo(player);
    todo_wine
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_CreateMediaItemFromURL(player, L"url", TRUE, 0, &item);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_ClearMediaItem(player);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetMediaItem(player, &item);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetVideoWindow(player, &window);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetDuration(player, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetDuration(player, &MFP_POSITIONTYPE_100NS, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetDuration(player, &MFP_POSITIONTYPE_100NS, &propvar);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetPosition(player, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetPosition(player, &MFP_POSITIONTYPE_100NS, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetPosition(player, &MFP_POSITIONTYPE_100NS, &propvar);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_Shutdown(player);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMFPMediaPlayer_Release(player);
}

static void test_media_item(void)
{
    IMFPMediaPlayer *player;
    IMFPMediaItem *item;
    HRESULT hr;

    hr = MFPCreateMediaPlayer(NULL, FALSE, 0, NULL, NULL, &player);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_SetMediaItem(player, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    /* Async mode, no callback was specified. */
    hr = IMFPMediaPlayer_CreateMediaItemFromURL(player, L"url", FALSE, 0, &item);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_CreateMediaItemFromURL(player, L"url", FALSE, 0, NULL);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_CreateMediaItemFromURL(player, L"url", TRUE, 0, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    IMFPMediaPlayer_Release(player);
}

static void test_video_control(void)
{
    MFVideoNormalizedRect rect;
    IMFPMediaPlayer *player;
    COLORREF color;
    HWND window;
    DWORD mode;
    HRESULT hr;
    SIZE size;

    window = CreateWindowA("static", "mfplay_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    ok(!!window, "Failed to create output window.\n");

    hr = MFPCreateMediaPlayer(NULL, FALSE, 0, NULL, window, &player);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* No active media item */

    rect.left = rect.top = 0.0f;
    rect.right = rect.bottom = 1.0f;
    hr = IMFPMediaPlayer_SetVideoSourceRect(player, &rect);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_SetVideoSourceRect(player, NULL);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_SetBorderColor(player, 0);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_SetAspectRatioMode(player, MFVideoARMode_None);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetVideoSourceRect(player, &rect);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetBorderColor(player, &color);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetAspectRatioMode(player, &mode);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetIdealVideoSize(player, &size, &size);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetNativeVideoSize(player, &size, &size);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_UpdateVideo(player);
    todo_wine
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    IMFPMediaPlayer_Release(player);

    /* No active media item, no output window.*/

    hr = MFPCreateMediaPlayer(NULL, FALSE, 0, NULL, NULL, &player);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    rect.left = rect.top = 0.0f;
    rect.right = rect.bottom = 1.0f;
    hr = IMFPMediaPlayer_SetVideoSourceRect(player, &rect);
    ok(hr == HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE), "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_SetVideoSourceRect(player, NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE), "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_SetBorderColor(player, 0);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_SetAspectRatioMode(player, MFVideoARMode_None);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetVideoSourceRect(player, &rect);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetBorderColor(player, &color);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetAspectRatioMode(player, &mode);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetIdealVideoSize(player, &size, &size);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetNativeVideoSize(player, &size, &size);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_UpdateVideo(player);
    todo_wine
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    IMFPMediaPlayer_Release(player);

    DestroyWindow(window);
}

static void test_duration(void)
{
    IMFPMediaPlayer *player;
    PROPVARIANT propvar;
    HRESULT hr;

    hr = MFPCreateMediaPlayer(NULL, FALSE, 0, NULL, NULL, &player);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* No media item. */
    hr = IMFPMediaPlayer_GetDuration(player, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetDuration(player, &MFP_POSITIONTYPE_100NS, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetDuration(player, &IID_IUnknown, &propvar);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetDuration(player, &MFP_POSITIONTYPE_100NS, &propvar);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetPosition(player, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetPosition(player, &MFP_POSITIONTYPE_100NS, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetPosition(player, &IID_IUnknown, &propvar);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetPosition(player, &MFP_POSITIONTYPE_100NS, &propvar);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    IMFPMediaPlayer_Release(player);
}

START_TEST(mfplay)
{
    test_create_player();
    test_shutdown();
    test_media_item();
    test_video_control();
    test_duration();
}
