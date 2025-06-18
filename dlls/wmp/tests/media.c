/*
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

#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#include <wmp.h>
#include <olectl.h>
#include <nserror.h>
#include <wmpids.h>
#include <math.h>
#include <assert.h>

#include "wine/test.h"

#define DEFINE_EXPECT(kind) \
    static DWORD expect_ ## kind = 0, called_ ## kind = 0

#define SET_EXPECT(kind, index) \
    do { \
        assert(index < 8 * sizeof(expect_ ## kind)); \
        expect_ ## kind |= (1 << index); \
    }while(0)

#define CHECK_EXPECT(kind, index) \
    do { \
        ok(expect_ ##kind & (1 << index), "unexpected event for  " #kind ", index:%ld\n", index); \
        called_ ## kind |= (1 << index); \
    }while(0)

#define CLEAR_CALLED(kind, index) \
    do { \
        expect_ ## kind &= ~(1 << index); \
        called_ ## kind &= ~(1 << index); \
    }while(0)

#define CHECK_CALLED(kind, index) \
    do { \
        ok(called_ ## kind & (1 << index), "expected " #kind ", %d\n", index); \
        expect_ ## kind &= ~(1 << index); \
        called_ ## kind &= ~(1 << index); \
    }while(0)

#define CHECK_NOT_CALLED(kind, index) \
    do { \
        ok(!(called_ ## kind & (1 << index)), "not expected " #kind ", %d\n", index); \
        expect_ ## kind &= ~(1 << index); \
        called_ ## kind &= ~(1 << index); \
    }while(0)

DEFINE_EXPECT(PLAYSTATE);
DEFINE_EXPECT(OPENSTATE);

static HANDLE playing_event;
static HANDLE completed_event;
static DWORD main_thread_id;

static const WCHAR mp3file[] = L"test.mp3";
static const WCHAR mp3file1s[] = L"test1s.mp3";
static inline WCHAR *load_resource(const WCHAR *name)
{
    static WCHAR pathW[MAX_PATH];
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    GetTempPathW(ARRAY_SIZE(pathW), pathW);
    lstrcatW(pathW, name);

    file = CreateFileW(pathW, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "file creation failed, at %s, error %ld\n", wine_dbgstr_w(pathW),
        GetLastError());

    res = FindResourceW(NULL, name, (LPCWSTR)RT_RCDATA);
    ok( res != 0, "couldn't find resource\n" );
    ptr = LockResource( LoadResource( GetModuleHandleA(NULL), res ));
    WriteFile( file, ptr, SizeofResource( GetModuleHandleA(NULL), res ), &written, NULL );
    ok( written == SizeofResource( GetModuleHandleA(NULL), res ), "couldn't write resource\n" );
    CloseHandle( file );

    return pathW;
}

static ULONG WINAPI Dispatch_AddRef(IDispatch *iface)
{
    return 2;
}

static ULONG WINAPI Dispatch_Release(IDispatch *iface)
{
    return 1;
}

static HRESULT WINAPI Dispatch_GetTypeInfoCount(IDispatch *iface, UINT *pctinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Dispatch_GetTypeInfo(IDispatch *iface, UINT iTInfo, LCID lcid,
        ITypeInfo **ppTInfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Dispatch_GetIDsOfNames(IDispatch *iface, REFIID riid, LPOLESTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPOCXEvents_QueryInterface(IDispatch *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID__WMPOCXEvents, riid) || IsEqualGUID(&IID_IDispatch, riid)) {
        *ppv = iface;
        return S_OK;
    }

    ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static HRESULT WINAPI WMPOCXEvents_Invoke(IDispatch *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ok(main_thread_id == GetCurrentThreadId(), "Got notification outside of main thread!\n");
    switch(dispIdMember) {
        case DISPID_WMPCOREEVENT_OPENSTATECHANGE:
            CHECK_EXPECT(OPENSTATE, V_UI4(pDispParams->rgvarg));
            if (winetest_debug > 1)
                trace("DISPID_WMPCOREEVENT_OPENSTATECHANGE, %ld\n", V_UI4(pDispParams->rgvarg));
            break;
        case DISPID_WMPCOREEVENT_PLAYSTATECHANGE:
            CHECK_EXPECT(PLAYSTATE, V_UI4(pDispParams->rgvarg));
            if (V_UI4(pDispParams->rgvarg) == wmppsPlaying) {
                SetEvent(playing_event);
            } else if (V_UI4(pDispParams->rgvarg) == wmppsMediaEnded) {
                SetEvent(completed_event);
            }
            if (winetest_debug > 1)
                trace("DISPID_WMPCOREEVENT_PLAYSTATECHANGE, %ld\n", V_UI4(pDispParams->rgvarg));
            break;
        case DISPID_WMPCOREEVENT_MEDIACHANGE:
            if (winetest_debug > 1)
                trace("DISPID_WMPCOREEVENT_MEDIACHANGE\n");
            break;
        case DISPID_WMPCOREEVENT_CURRENTITEMCHANGE:
            if (winetest_debug > 1)
                trace("DISPID_WMPCOREEVENT_CURRENTITEMCHANGE\n");
            break;
        case DISPID_WMPCOREEVENT_STATUSCHANGE:
            if (winetest_debug > 1)
                trace("DISPID_WMPCOREEVENT_STATUSCHANGE\n");
            break;
        default:
            if (winetest_debug > 1)
                trace("event: %ld\n", dispIdMember);
            break;
    }

    return E_NOTIMPL;
}

static IDispatchVtbl WMPOcxEventsVtbl = {
    WMPOCXEvents_QueryInterface,
    Dispatch_AddRef,
    Dispatch_Release,
    Dispatch_GetTypeInfoCount,
    Dispatch_GetTypeInfo,
    Dispatch_GetIDsOfNames,
    WMPOCXEvents_Invoke,
};

static IDispatch WMPOCXEvents = { &WMPOcxEventsVtbl };

static HRESULT pump_messages(DWORD timeout, DWORD count, const HANDLE *handles) {
    MSG msg;
    HRESULT res;
    DWORD start_time = GetTickCount();
    do {
        DWORD now = GetTickCount();
        res = MsgWaitForMultipleObjectsEx(count, handles, start_time + timeout - now,
                QS_ALLINPUT ,MWMO_ALERTABLE | MWMO_INPUTAVAILABLE);
        if (res == WAIT_OBJECT_0 + 1) {
            GetMessageW(&msg, 0, 0, 0);
            if (winetest_debug > 1)
                trace("Dispatching %d\n", msg.message);
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    while (res == WAIT_OBJECT_0 + 1);
    return res;
}

static void test_completion_event(void)
{
    DWORD res = 0;
    IWMPPlayer4 *player4;
    HRESULT hres;
    BSTR filename;
    IConnectionPointContainer *container;
    IConnectionPoint *point;
    IOleObject *oleobj;
    static DWORD dw = 100;

    hres = CoCreateInstance(&CLSID_WindowsMediaPlayer, NULL, CLSCTX_INPROC_SERVER, &IID_IOleObject, (void**)&oleobj);
    if(hres == REGDB_E_CLASSNOTREG) {
        win_skip("CLSID_WindowsMediaPlayer not registered\n");
        return;
    }
    ok(hres == S_OK, "Could not create CLSID_WindowsMediaPlayer instance: %08lx\n", hres);

    hres = IOleObject_QueryInterface(oleobj, &IID_IConnectionPointContainer, (void**)&container);
    ok(hres == S_OK, "QueryInterface(IID_IConnectionPointContainer) failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IConnectionPointContainer_FindConnectionPoint(container, &IID__WMPOCXEvents, &point);
    IConnectionPointContainer_Release(container);
    ok(hres == S_OK, "FindConnectionPoint failed: %08lx\n", hres);

    hres = IConnectionPoint_Advise(point, (IUnknown*)&WMPOCXEvents, &dw);
    ok(hres == S_OK, "Advise failed: %08lx\n", hres);

    hres = IOleObject_QueryInterface(oleobj, &IID_IWMPPlayer4, (void**)&player4);
    ok(hres == S_OK, "Could not get IWMPPlayer4 iface: %08lx\n", hres);

    filename = SysAllocString(load_resource(mp3file1s));

    SET_EXPECT(OPENSTATE, wmposPlaylistChanging);
    SET_EXPECT(OPENSTATE, wmposPlaylistOpenNoMedia);
    SET_EXPECT(OPENSTATE, wmposPlaylistChanged);
    SET_EXPECT(OPENSTATE, wmposOpeningUnknownURL);
    SET_EXPECT(OPENSTATE, wmposMediaOpen);
    SET_EXPECT(OPENSTATE, wmposMediaOpening);
    SET_EXPECT(PLAYSTATE, wmppsPlaying);
    SET_EXPECT(PLAYSTATE, wmppsMediaEnded);
    SET_EXPECT(PLAYSTATE, wmppsStopped);
    SET_EXPECT(PLAYSTATE, wmppsTransitioning);
    /* following two are sent on vistau64 vms only */
    SET_EXPECT(OPENSTATE, wmposMediaChanging);
    SET_EXPECT(PLAYSTATE, wmppsReady);
    hres = IWMPPlayer4_put_URL(player4, filename);
    ok(hres == S_OK, "IWMPPlayer4_put_URL failed: %08lx\n", hres);
    res = pump_messages(3000, 1, &completed_event);
    ok(res == WAIT_OBJECT_0, "Timed out while waiting for media to complete\n");

    /* following two are sent on vistau64 vms only */
    CLEAR_CALLED(OPENSTATE, wmposMediaChanging);
    CLEAR_CALLED(PLAYSTATE, wmppsReady);

    CHECK_CALLED(OPENSTATE, wmposPlaylistChanging);
    CHECK_CALLED(OPENSTATE, wmposPlaylistChanged);
    CHECK_CALLED(OPENSTATE, wmposPlaylistOpenNoMedia);
    CHECK_CALLED(PLAYSTATE, wmppsTransitioning);
    CHECK_CALLED(OPENSTATE, wmposOpeningUnknownURL);
    CHECK_CALLED(OPENSTATE, wmposMediaOpen);
    CHECK_CALLED(PLAYSTATE, wmppsPlaying);
    CHECK_CALLED(PLAYSTATE, wmppsMediaEnded);
    CHECK_CALLED(PLAYSTATE, wmppsStopped);
    /* MediaOpening happens only on xp, 2003 */
    CLEAR_CALLED(OPENSTATE, wmposMediaOpening);

    hres = IConnectionPoint_Unadvise(point, dw);
    ok(hres == S_OK, "Unadvise failed: %08lx\n", hres);

    IConnectionPoint_Release(point);
    IWMPPlayer4_Release(player4);
    IOleObject_Release(oleobj);
    DeleteFileW(filename);
    SysFreeString(filename);
}

static BOOL test_wmp(void)
{
    DWORD res = 0;
    IWMPPlayer4 *player4;
    IWMPControls *controls;
    HRESULT hres;
    BSTR filename;
    IConnectionPointContainer *container;
    IConnectionPoint *point;
    IOleObject *oleobj;
    static DWORD dw = 100;
    IWMPSettings *settings;
    BOOL test_ran = TRUE;
    IWMPNetwork *network;
    DOUBLE duration;
    VARIANT_BOOL vbool;
    LONG progress;
    IWMPMedia *media;
    BSTR bstrcurrentPosition;

    hres = CoCreateInstance(&CLSID_WindowsMediaPlayer, NULL, CLSCTX_INPROC_SERVER, &IID_IOleObject, (void**)&oleobj);
    if(hres == REGDB_E_CLASSNOTREG) {
        win_skip("CLSID_WindowsMediaPlayer not registered\n");
        return FALSE;
    }
    ok(hres == S_OK, "Could not create CLSID_WindowsMediaPlayer instance: %08lx\n", hres);

    hres = IOleObject_QueryInterface(oleobj, &IID_IConnectionPointContainer, (void**)&container);
    ok(hres == S_OK, "QueryInterface(IID_IConnectionPointContainer) failed: %08lx\n", hres);

    hres = IConnectionPointContainer_FindConnectionPoint(container, &IID__WMPOCXEvents, &point);
    IConnectionPointContainer_Release(container);
    ok(hres == S_OK, "FindConnectionPoint failed: %08lx\n", hres);

    hres = IConnectionPoint_Advise(point, (IUnknown*)&WMPOCXEvents, &dw);
    ok(hres == S_OK, "Advise failed: %08lx\n", hres);

    hres = IOleObject_QueryInterface(oleobj, &IID_IWMPPlayer4, (void**)&player4);
    ok(hres == S_OK, "Could not get IWMPPlayer4 iface: %08lx\n", hres);

    settings = NULL;
    hres = IWMPPlayer4_get_settings(player4, &settings);
    ok(hres == S_OK, "get_settings failed: %08lx\n", hres);
    ok(settings != NULL, "settings = NULL\n");

    hres = IWMPSettings_put_autoStart(settings, VARIANT_FALSE);
    ok(hres == S_OK, "Could not put autoStart in IWMPSettings: %08lx\n", hres);

    controls = NULL;
    hres = IWMPPlayer4_get_controls(player4, &controls);
    ok(hres == S_OK, "get_controls failed: %08lx\n", hres);
    ok(controls != NULL, "controls = NULL\n");

    bstrcurrentPosition = SysAllocString(L"currentPosition");
    hres = IWMPControls_get_isAvailable(controls, bstrcurrentPosition, &vbool);
    ok(hres == S_OK, "IWMPControls_get_isAvailable failed: %08lx\n", hres);
    ok(vbool == VARIANT_FALSE, "unexpected value\n");

    hres = IWMPControls_play(controls);
    ok(hres == NS_S_WMPCORE_COMMAND_NOT_AVAILABLE, "IWMPControls_play is available: %08lx\n", hres);

    hres = IWMPSettings_put_volume(settings, 36);
    ok(hres == S_OK, "IWMPSettings_put_volume failed: %08lx\n", hres);
    hres = IWMPSettings_get_volume(settings, &progress);
    ok(hres == S_OK, "IWMPSettings_get_volume failed: %08lx\n", hres);
    ok(progress == 36, "unexpected value: %ld\n", progress);

    filename = SysAllocString(load_resource(mp3file));

    SET_EXPECT(OPENSTATE, wmposPlaylistChanging);
    SET_EXPECT(OPENSTATE, wmposPlaylistOpenNoMedia);
    SET_EXPECT(OPENSTATE, wmposPlaylistChanged);
    SET_EXPECT(PLAYSTATE, wmppsTransitioning);
    SET_EXPECT(PLAYSTATE, wmppsReady);
    hres = IWMPPlayer4_put_URL(player4, filename);
    ok(hres == S_OK, "IWMPPlayer4_put_URL failed: %08lx\n", hres);
    CHECK_CALLED(OPENSTATE, wmposPlaylistChanging);
    CHECK_CALLED(OPENSTATE, wmposPlaylistChanged);
    CHECK_CALLED(OPENSTATE, wmposPlaylistOpenNoMedia);
    CHECK_CALLED(PLAYSTATE, wmppsTransitioning);
    CHECK_CALLED(PLAYSTATE, wmppsReady);

    SET_EXPECT(OPENSTATE, wmposOpeningUnknownURL);
    SET_EXPECT(OPENSTATE, wmposMediaOpen);
    SET_EXPECT(PLAYSTATE, wmppsPlaying);
    SET_EXPECT(PLAYSTATE, wmppsTransitioning);
    /* MediaOpening happens only on xp, 2003 */
    SET_EXPECT(OPENSTATE, wmposMediaOpening);
    hres = IWMPControls_play(controls);
    ok(hres == S_OK, "IWMPControls_play failed: %08lx\n", hres);
    res = pump_messages(1000, 1, &playing_event);
    ok(res == WAIT_OBJECT_0 || broken(res == WAIT_TIMEOUT), "Timed out while waiting for media to become ready\n");
    if (res == WAIT_TIMEOUT) {
        /* This happens on Vista Ultimate 64 vms
         * I have been unable to find out source of this behaviour */
        win_skip("Failed to transition media to playing state.\n");
        test_ran = FALSE;
        goto playback_skip;
    }
    CHECK_CALLED(OPENSTATE, wmposOpeningUnknownURL);
    CHECK_CALLED(OPENSTATE, wmposMediaOpen);
    CHECK_CALLED(PLAYSTATE, wmppsPlaying);
    CHECK_CALLED(PLAYSTATE, wmppsTransitioning);
    /* MediaOpening happens only on xp, 2003 */
    CLEAR_CALLED(OPENSTATE, wmposMediaOpening);

    hres = IWMPControls_get_isAvailable(controls, bstrcurrentPosition, &vbool);
    ok(hres == S_OK, "IWMPControls_get_isAvailable failed: %08lx\n", hres);
    ok(vbool == VARIANT_TRUE, "unexpected value\n");

    duration = 0.0;
    hres = IWMPControls_get_currentPosition(controls, &duration);
    ok(hres == S_OK, "IWMPControls_get_currentPosition failed: %08lx\n", hres);
    ok((int)duration == 0, "unexpected value %f\n", duration);

    duration = 1.1;
    hres = IWMPControls_put_currentPosition(controls, duration);
    ok(hres == S_OK, "IWMPControls_put_currentPosition failed: %08lx\n", hres);

    duration = 0.0;
    hres = IWMPControls_get_currentPosition(controls, &duration);
    ok(hres == S_OK, "IWMPControls_get_currentPosition failed: %08lx\n", hres);
    ok(duration >= 1.05 /* save some fp errors */, "unexpected value %f\n", duration);

    hres = IWMPPlayer4_get_currentMedia(player4, &media);
    ok(hres == S_OK, "IWMPPlayer4_get_currentMedia failed: %08lx\n", hres);
    hres = IWMPMedia_get_duration(media, &duration);
    ok(hres == S_OK, "IWMPMedia_get_duration failed: %08lx\n", hres);
    ok(floor(duration + 0.5) == 3, "unexpected value: %f\n", duration);
    IWMPMedia_Release(media);

    network = NULL;
    hres = IWMPPlayer4_get_network(player4, &network);
    ok(hres == S_OK, "get_network failed: %08lx\n", hres);
    ok(network != NULL, "network = NULL\n");
    progress = 0;
    hres = IWMPNetwork_get_bufferingProgress(network, &progress);
    ok(hres == S_OK || broken(hres == S_FALSE), "IWMPNetwork_get_bufferingProgress failed: %08lx\n", hres);
    ok(progress == 100, "unexpected value: %ld\n", progress);
    progress = 0;
    hres = IWMPNetwork_get_downloadProgress(network, &progress);
    ok(hres == S_OK, "IWMPNetwork_get_downloadProgress failed: %08lx\n", hres);
    ok(progress == 100, "unexpected value: %ld\n", progress);
    IWMPNetwork_Release(network);

    SET_EXPECT(PLAYSTATE, wmppsStopped);
    /* The following happens on wine only since we close media on stop */
    SET_EXPECT(OPENSTATE, wmposPlaylistOpenNoMedia);
    hres = IWMPControls_stop(controls);
    ok(hres == S_OK, "IWMPControls_stop failed: %08lx\n", hres);
    CHECK_CALLED(PLAYSTATE, wmppsStopped);
    todo_wine CHECK_NOT_CALLED(OPENSTATE, wmposPlaylistOpenNoMedia);

    /* Already Stopped */
    hres = IWMPControls_stop(controls);
    ok(hres == NS_S_WMPCORE_COMMAND_NOT_AVAILABLE, "IWMPControls_stop is available: %08lx\n", hres);

    SET_EXPECT(PLAYSTATE, wmppsPlaying);
    /* The following happens on wine only since we close media on stop */
    SET_EXPECT(OPENSTATE, wmposOpeningUnknownURL);
    SET_EXPECT(OPENSTATE, wmposMediaOpen);
    SET_EXPECT(PLAYSTATE, wmppsTransitioning);
    hres = IWMPControls_play(controls);
    ok(hres == S_OK, "IWMPControls_play failed: %08lx\n", hres);
    CHECK_CALLED(PLAYSTATE, wmppsPlaying);
    todo_wine CHECK_NOT_CALLED(OPENSTATE, wmposOpeningUnknownURL);
    todo_wine CHECK_NOT_CALLED(OPENSTATE, wmposMediaOpen);
    todo_wine CHECK_NOT_CALLED(PLAYSTATE, wmppsTransitioning);

playback_skip:
    hres = IConnectionPoint_Unadvise(point, dw);
    ok(hres == S_OK, "Unadvise failed: %08lx\n", hres);

    hres = IWMPSettings_get_volume(settings, &progress);
    ok(hres == S_OK, "IWMPSettings_get_volume failed: %08lx\n", hres);
    ok(progress == 36, "unexpected value: %ld\n", progress);
    hres = IWMPSettings_put_volume(settings, 99);
    ok(hres == S_OK, "IWMPSettings_put_volume failed: %08lx\n", hres);
    hres = IWMPSettings_get_volume(settings, &progress);
    ok(hres == S_OK, "IWMPSettings_get_volume failed: %08lx\n", hres);
    ok(progress == 99, "unexpected value: %ld\n", progress);

    IConnectionPoint_Release(point);
    IWMPSettings_Release(settings);
    IWMPControls_Release(controls);
    IWMPPlayer4_Release(player4);
    IOleObject_Release(oleobj);
    DeleteFileW(filename);
    SysFreeString(filename);
    SysFreeString(bstrcurrentPosition);

    return test_ran;
}

static void test_media_item(void)
{
    static WCHAR pathW[MAX_PATH];
    static WCHAR currentdirW[MAX_PATH];
    struct {
        const WCHAR *prefix;
        const WCHAR *filename;
        const WCHAR *expected;
    } tests[] = {
        { NULL, L"invalid_url.mp3", L"invalid_url" },
        { currentdirW, mp3file, L"test" },
        { currentdirW, L"invalid_url.mp3", L"invalid_url" },
        { L"http://", L"test.winehq.org/tests/test.mp3", L"test" },
        { L"http://", L"invalid_url.mp3", L"invalid_url.mp3" },
        { L"https://", L"test.winehq.org/tests/test.mp3", L"test" },
        { L"https://", L"invalid_url.mp3", L"invalid_url.mp3" },
        { L"file:///", mp3file, L"test" },
        { L"file:///", L"invalid_url.mp3", L"invalid_url" },
        { L"foo://", mp3file, mp3file },
        { L"foo://", L"invalid_url.mp3", L"invalid_url.mp3" }
    };
    IWMPMedia *media, *media2;
    IWMPPlayer4 *player;
    HRESULT hr;
    BSTR str;
    int i;

    hr = CoCreateInstance(&CLSID_WindowsMediaPlayer, NULL, CLSCTX_INPROC_SERVER, &IID_IWMPPlayer4, (void **)&player);
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip("CLSID_WindowsMediaPlayer is not registered.\n");
        return;
    }
    ok(hr == S_OK, "Failed to create media player instance, hr %#lx.\n", hr);

    hr = IWMPPlayer4_newMedia(player, NULL, &media);
    ok(hr == S_OK, "Failed to create a media item, hr %#lx.\n", hr);
    hr = IWMPMedia_get_name(media, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    hr = IWMPMedia_get_name(media, &str);
    ok(hr == S_OK, "Failed to get item name, hr %#lx.\n", hr);
    ok(*str == 0, "Unexpected name %s.\n", wine_dbgstr_w(str));
    SysFreeString(str);

    media2 = (void *)0xdeadbeef;
    hr = IWMPPlayer4_get_currentMedia(player, &media2);
    ok(hr == S_FALSE, "Failed to get current media, hr %#lx.\n", hr);
    ok(media2 == NULL, "Unexpected media instance.\n");

    hr = IWMPPlayer4_put_currentMedia(player, media);
    ok(hr == S_OK, "Failed to set current media, hr %#lx.\n", hr);

    hr = IWMPPlayer4_get_currentMedia(player, &media2);
    ok(hr == S_OK, "Failed to get current media, hr %#lx.\n", hr);
    ok(media2 != NULL && media != media2, "Unexpected media instance.\n");
    IWMPMedia_Release(media2);

    IWMPMedia_Release(media);

    str = SysAllocStringLen(NULL, 0);
    hr = IWMPPlayer4_newMedia(player, str, &media);
    ok(hr == S_OK, "Failed to create a media item, hr %#lx.\n", hr);
    SysFreeString(str);
    hr = IWMPMedia_get_name(media, &str);
    ok(hr == S_OK, "Failed to get item name, hr %#lx.\n", hr);
    ok(*str == 0, "Unexpected name %s.\n", wine_dbgstr_w(str));
    SysFreeString(str);
    IWMPMedia_Release(media);

    str = SysAllocString(mp3file);
    hr = IWMPPlayer4_newMedia(player, str, &media);
    ok(hr == S_OK, "Failed to create a media item, hr %#lx.\n", hr);
    SysFreeString(str);
    hr = IWMPMedia_get_name(media, &str);
    ok(hr == S_OK, "Failed to get item name, hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"test"), "Expected %s, got %s\n", wine_dbgstr_w(L"test"), wine_dbgstr_w(str));
    SysFreeString(str);
    hr = IWMPMedia_put_name(media, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    hr = IWMPMedia_get_name(media, &str);
    ok(hr == S_OK, "Failed to get item name, hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"test"), "Expected %s, got %s\n", wine_dbgstr_w(L"test"), wine_dbgstr_w(str));
    SysFreeString(str);

    hr = IWMPPlayer4_put_currentMedia(player, media);
    ok(hr == S_OK, "Failed to set current media, hr %#lx.\n", hr);
    IWMPMedia_Release(media);

    hr = IWMPPlayer4_get_currentMedia(player, &media2);
    ok(hr == S_OK, "Failed to get current media, hr %#lx.\n", hr);
    ok(media2 != NULL, "Unexpected media instance.\n");
    hr = IWMPMedia_get_name(media2, &str);
    ok(hr == S_OK, "Failed to get item name, hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"test"), "Expected %s, got %s\n", wine_dbgstr_w(L"test"), wine_dbgstr_w(str));
    SysFreeString(str);
    IWMPMedia_Release(media2);

    GetCurrentDirectoryW(ARRAY_SIZE(currentdirW), currentdirW);
    lstrcatW(currentdirW, L"\\");

    for (i=0; i<ARRAY_SIZE(tests); i++)
    {

        pathW[0] = '\0';
        if(tests[i].prefix) lstrcatW(pathW, tests[i].prefix);
        lstrcatW(pathW, tests[i].filename);

        str = SysAllocString(pathW);
        hr = IWMPPlayer4_newMedia(player, str, &media);
        ok(hr == S_OK, "Failed to create a media item, hr %#lx.\n", hr);
        SysFreeString(str);
        hr = IWMPMedia_get_name(media, &str);
        ok(hr == S_OK, "Failed to get item name, hr %#lx.\n", hr);
        ok(!lstrcmpW(str, tests[i].expected), "Expected %s, got %s\n", wine_dbgstr_w(tests[i].expected), wine_dbgstr_w(str));
        SysFreeString(str);
        IWMPMedia_Release(media);
    }

    IWMPPlayer4_Release(player);
}

static void test_player_url(void)
{
    IWMPPlayer4 *player;
    BSTR str, str2;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_WindowsMediaPlayer, NULL, CLSCTX_INPROC_SERVER, &IID_IWMPPlayer4, (void **)&player);
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip("CLSID_WindowsMediaPlayer is not registered.\n");
        return;
    }
    ok(hr == S_OK, "Failed to create media player instance, hr %#lx.\n", hr);

    hr = IWMPPlayer4_get_URL(player, &str);
    ok(hr == S_OK, "Failed to get url, hr %#lx.\n", hr);
    ok(*str == 0, "Unexpected url %s.\n", wine_dbgstr_w(str));
    SysFreeString(str);

    str2 = SysAllocString(mp3file);
    hr = IWMPPlayer4_put_URL(player, str2);
    ok(hr == S_OK, "Failed to set url, hr %#lx.\n", hr);

    hr = IWMPPlayer4_put_URL(player, NULL);
    ok(hr == S_OK, "Failed to set url, hr %#lx.\n", hr);
    hr = IWMPPlayer4_get_URL(player, &str);
    ok(hr == S_OK, "Failed to set url, hr %#lx.\n", hr);
    ok(*str == 0, "Unexpected url, %s.\n", wine_dbgstr_w(str));
    SysFreeString(str);

    /* Empty url */
    hr = IWMPPlayer4_put_URL(player, str2);
    ok(hr == S_OK, "Failed to set url, hr %#lx.\n", hr);

    str = SysAllocStringLen(NULL, 0);
    hr = IWMPPlayer4_put_URL(player, str);
    ok(hr == S_OK, "Failed to set url, hr %#lx.\n", hr);
    SysFreeString(str);

    hr = IWMPPlayer4_get_URL(player, &str);
    ok(hr == S_OK, "Failed to set url, hr %#lx.\n", hr);
    ok(*str == 0, "Unexpected url, %s.\n", wine_dbgstr_w(str));
    SysFreeString(str);

    SysFreeString(str2);
    IWMPPlayer4_Release(player);
}

static void test_playlist(void)
{
    IWMPPlayer4 *player;
    IWMPPlaylist *playlist, *playlist2;
    HRESULT hr;
    BSTR str, str2;
    LONG count;
    static const WCHAR nameW[] = L"Playlist1";

    hr = CoCreateInstance(&CLSID_WindowsMediaPlayer, NULL, CLSCTX_INPROC_SERVER, &IID_IWMPPlayer4, (void **)&player);
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip("CLSID_WindowsMediaPlayer is not registered.\n");
        return;
    }
    ok(hr == S_OK, "Failed to create media player instance, hr %#lx.\n", hr);

    playlist = NULL;
    hr = IWMPPlayer4_get_currentPlaylist(player, &playlist);
    ok(hr == S_OK, "IWMPPlayer4_get_currentPlaylist failed: %08lx\n", hr);
    ok(playlist != NULL, "playlist == NULL\n");

    if (0) /* fails on non-English locales */
    {
        hr = IWMPPlaylist_get_name(playlist, &str);
        ok(hr == S_OK, "Failed to get playlist name, hr %#lx.\n", hr);
        ok(!lstrcmpW(str, nameW), "Expected %s, got %s\n", wine_dbgstr_w(nameW), wine_dbgstr_w(str));
        SysFreeString(str);
    }

    hr = IWMPPlaylist_get_count(playlist, NULL);
    ok(hr == E_POINTER, "Failed to get count, hr %#lx.\n", hr);

    count = -1;
    hr = IWMPPlaylist_get_count(playlist, &count);
    ok(hr == S_OK, "Failed to get count, hr %#lx.\n", hr);
    ok(count == 0, "Expected 0, got %ld\n", count);

    IWMPPlaylist_Release(playlist);

    /* newPlaylist doesn't change current playlist */
    hr = IWMPPlayer4_newPlaylist(player, NULL, NULL, &playlist);
    ok(hr == S_OK, "Failed to create a playlist, hr %#lx.\n", hr);

    playlist2 = NULL;
    hr = IWMPPlayer4_get_currentPlaylist(player, &playlist2);
    ok(hr == S_OK, "IWMPPlayer4_get_currentPlaylist failed: %08lx\n", hr);
    ok(playlist2 != NULL && playlist2 != playlist, "Unexpected playlist instance\n");

    IWMPPlaylist_Release(playlist2);

    /* different playlists can have the same name */
    str = SysAllocString(nameW);
    hr = IWMPPlaylist_put_name(playlist, str);
    ok(hr == S_OK, "Failed to get playlist name, hr %#lx.\n", hr);

    playlist2 = NULL;
    hr = IWMPPlayer4_newPlaylist(player, str, NULL, &playlist2);
    ok(hr == S_OK, "Failed to create a playlist, hr %#lx.\n", hr);
    hr = IWMPPlaylist_get_name(playlist2, &str2);
    ok(hr == S_OK, "Failed to get playlist name, hr %#lx.\n", hr);
    ok(playlist != playlist2, "Expected playlists to be different");
    ok(!lstrcmpW(str, str2), "Expected names to be the same\n");
    SysFreeString(str);
    SysFreeString(str2);

    IWMPPlaylist_Release(playlist2);

    IWMPPlaylist_Release(playlist);
    IWMPPlayer4_Release(player);
}

START_TEST(media)
{
    CoInitialize(NULL);

    main_thread_id = GetCurrentThreadId();
    playing_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    completed_event = CreateEventW(NULL, FALSE, FALSE, NULL);

    test_playlist();
    test_media_item();
    test_player_url();
    if (test_wmp()) {
        test_completion_event();
    } else {
        win_skip("Failed to play media\n");
    }

    CloseHandle(playing_event);
    CloseHandle(completed_event);

    CoUninitialize();
}
