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

static WCHAR *load_resource(const WCHAR *name)
{
    static WCHAR pathW[MAX_PATH];
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    GetTempPathW(ARRAY_SIZE(pathW), pathW);
    lstrcatW(pathW, name);

    file = CreateFileW(pathW, GENERIC_READ|GENERIC_WRITE, 0,
                       NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "file creation failed, at %s, error %ld.\n",
       wine_dbgstr_w(pathW), GetLastError());

    res = FindResourceW(NULL, name, (LPCWSTR)RT_RCDATA);
    ok(res != 0, "couldn't find resource\n");
    ptr = LockResource(LoadResource(GetModuleHandleA(NULL), res));
    WriteFile(file, ptr, SizeofResource(GetModuleHandleA(NULL), res),
               &written, NULL);
    ok(written == SizeofResource(GetModuleHandleA(NULL), res),
       "couldn't write resource\n" );
    CloseHandle(file);

    return pathW;
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
    float slowest, fastest, rate;
    MFP_MEDIAPLAYER_STATE state;
    MFVideoNormalizedRect rect;
    IMFPMediaPlayer *player;
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

    hr = IMFPMediaPlayer_SetRate(player, 2.0f);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetRate(player, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetRate(player, &rate);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_Shutdown(player);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMFPMediaPlayer_Release(player);
}

static void test_media_item(void)
{
    IMFPMediaPlayer *player, *player2;
    WCHAR *filename, *url;
    IMFPMediaItem *item;
    DWORD_PTR user_data;
    PROPVARIANT propvar;
    unsigned int flags;
    BOOL ret, selected;
    IUnknown *object;
    DWORD count;
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

    filename = load_resource(L"test.mp4");

    hr = IMFPMediaPlayer_CreateMediaItemFromURL(player, filename, TRUE, 123, &item);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaItem_GetMediaPlayer(item, &player2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(player2 == player, "Unexpected player pointer.\n");
    IMFPMediaPlayer_Release(player2);

    hr = IMFPMediaItem_GetURL(item, &url);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaItem_GetObject(item, &object);
    ok(hr == MF_E_NOT_FOUND, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaItem_GetUserData(item, &user_data);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(user_data == 123, "Unexpected user data %#Ix.\n", user_data);

    hr = IMFPMediaItem_SetUserData(item, 124);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaItem_HasVideo(item, &ret, &selected);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(ret && selected, "Unexpected flags.\n");

    hr = IMFPMediaItem_HasAudio(item, &ret, &selected);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaItem_IsProtected(item, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaItem_IsProtected(item, &ret);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaItem_GetDuration(item, &MFP_POSITIONTYPE_100NS, &propvar);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaItem_GetNumberOfStreams(item, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaItem_GetStreamSelection(item, 0, &ret);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaItem_SetStreamSelection(item, 0, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaItem_GetStreamAttribute(item, 0, &MF_SD_LANGUAGE, &propvar);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(propvar.vt == VT_LPWSTR, "Unexpected vt %u.\n", propvar.vt);
    ok(!wcscmp(propvar.pwszVal, L"en"), "Unexpected value %s.\n", debugstr_w(propvar.pwszVal));
    PropVariantClear(&propvar);

    hr = IMFPMediaItem_GetStreamAttribute(item, 0, &MF_SD_STREAM_NAME, &propvar);
    ok(hr == S_OK || broken(hr == MF_E_ATTRIBUTENOTFOUND) /* Before Win10 1607. */, "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
    {
        ok(propvar.vt == VT_LPWSTR, "Unexpected vt %u.\n", propvar.vt);
        ok(!wcscmp(propvar.pwszVal, L"test"), "Unexpected value %s.\n", debugstr_w(propvar.pwszVal));
        PropVariantClear(&propvar);
    }

    hr = IMFPMediaItem_GetPresentationAttribute(item, &MF_PD_DURATION, &propvar);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaItem_GetCharacteristics(item, &flags);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Player shutdown affects created items. */
    hr = IMFPMediaPlayer_Shutdown(player);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaItem_GetMediaPlayer(item, &player2);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);
    ok(!player2, "Unexpected pointer %p.\n", player2);

    hr = IMFPMediaItem_GetURL(item, &url);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaItem_GetObject(item, &object);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaItem_GetUserData(item, &user_data);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(user_data == 124, "Unexpected user data %#Ix.\n", user_data);

    hr = IMFPMediaItem_SetUserData(item, 125);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaItem_HasVideo(item, &ret, &selected);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaItem_HasAudio(item, &ret, &selected);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaItem_IsProtected(item, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaItem_IsProtected(item, &ret);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaItem_GetDuration(item, &MFP_POSITIONTYPE_100NS, &propvar);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaItem_GetNumberOfStreams(item, &count);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaItem_GetStreamSelection(item, 0, &ret);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaItem_SetStreamSelection(item, 0, FALSE);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaItem_GetStreamAttribute(item, 0, &MF_SD_LANGUAGE, &propvar);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaItem_GetPresentationAttribute(item, &MF_PD_DURATION, &propvar);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaItem_GetCharacteristics(item, &flags);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    IMFPMediaItem_Release(item);

    DeleteFileW(filename);

    IMFPMediaPlayer_Release(player);
}

#define check_media_language(player, resource_name, expected_lang) \
    _check_media_language(__LINE__, FALSE, player, resource_name, expected_lang)
#define todo_check_media_language(player, resource_name, expected_lang) \
    _check_media_language(__LINE__, TRUE, player, resource_name, expected_lang)
static void _check_media_language(unsigned line, BOOL todo, IMFPMediaPlayer *player,
                                  const WCHAR *resource_name, const WCHAR *expected_lang)
{
    WCHAR *filename;
    IMFPMediaItem *item;
    PROPVARIANT propvar;
    const WCHAR *lang = NULL;
    HRESULT hr;

    filename = load_resource(resource_name);

    hr = IMFPMediaPlayer_CreateMediaItemFromURL(player, filename, TRUE, 123, &item);
    ok_(__FILE__, line)(hr == S_OK || broken(hr == MF_E_UNSUPPORTED_BYTESTREAM_TYPE) /* win8 - win10 1507 */,
                        "Unexpected hr %#lx.\n", hr);
    if (hr != S_OK)
    {
        DeleteFileW(filename);
        return;
    }

    hr = IMFPMediaItem_GetStreamAttribute(item, 0, &MF_SD_LANGUAGE, &propvar);
    ok_(__FILE__, line)(hr == S_OK || hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    if (hr == S_OK)
    {
        ok_(__FILE__, line)(propvar.vt == VT_LPWSTR, "Unexpected vt %u.\n", propvar.vt);
        if (propvar.vt == VT_LPWSTR)
            lang = propvar.pwszVal;
    }

    todo_wine_if(todo)
    {
        if (expected_lang)
            ok_(__FILE__, line)(lang && !wcscmp(lang, expected_lang), "Unexpected value %s.\n", debugstr_w(lang));
        else
            ok_(__FILE__, line)(!lang, "Unexpected value %s.\n", debugstr_w(lang));
    }

    PropVariantClear(&propvar);
    IMFPMediaItem_Release(item);
    DeleteFileW(filename);
}

static void test_media_language(void)
{
    IMFPMediaPlayer *player;
    HRESULT hr;

    hr = MFPCreateMediaPlayer(NULL, FALSE, 0, NULL, NULL, &player);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_media_language(player, L"test-eng.mp4", L"en");
    check_media_language(player, L"test-ang.mp4", NULL);
    check_media_language(player, L"test-und.mp4", NULL);
    check_media_language(player, L"test-en-US.mp4", L"en");

    check_media_language(player, L"test-en.wma", L"en");
    check_media_language(player, L"test-eng.wma", L"eng");
    check_media_language(player, L"test-ang.wma", L"ang");
    check_media_language(player, L"test-und.wma", L"und");
    todo_check_media_language(player, L"test-en-US.wma", L"en-US");

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

static void test_playback_rate(void)
{
    IMFPMediaPlayer *player;
    float rate;
    HRESULT hr;

    hr = MFPCreateMediaPlayer(NULL, FALSE, 0, NULL, NULL, &player);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_SetRate(player, 0.0f);
    ok(hr == MF_E_OUT_OF_RANGE, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_SetRate(player, 2.0f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetRate(player, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFPMediaPlayer_GetRate(player, &rate);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(rate == 1.0f, "Unexpected rate %f.\n", rate);

    IMFPMediaPlayer_Release(player);
}

START_TEST(mfplay)
{
    test_create_player();
    test_shutdown();
    test_media_item();
    test_media_language();
    test_video_control();
    test_duration();
    test_playback_rate();
}
