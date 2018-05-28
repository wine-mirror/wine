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
        ok(expect_ ##kind & (1 << index), "unexpected event for  " #kind ", index:%d\n", index); \
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

static const WCHAR mp3file[] = {'t','e','s','t','.','m','p','3',0};
static const WCHAR mp3file1s[] = {'t','e','s','t','1','s','.','m','p','3',0};
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
    ok(file != INVALID_HANDLE_VALUE, "file creation failed, at %s, error %d\n", wine_dbgstr_w(pathW),
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
                trace("DISPID_WMPCOREEVENT_OPENSTATECHANGE, %d\n", V_UI4(pDispParams->rgvarg));
            break;
        case DISPID_WMPCOREEVENT_PLAYSTATECHANGE:
            CHECK_EXPECT(PLAYSTATE, V_UI4(pDispParams->rgvarg));
            if (V_UI4(pDispParams->rgvarg) == wmppsPlaying) {
                SetEvent(playing_event);
            } else if (V_UI4(pDispParams->rgvarg) == wmppsMediaEnded) {
                SetEvent(completed_event);
            }
            if (winetest_debug > 1)
                trace("DISPID_WMPCOREEVENT_PLAYSTATECHANGE, %d\n", V_UI4(pDispParams->rgvarg));
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
                trace("event: %d\n", dispIdMember);
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
    ok(hres == S_OK, "Could not create CLSID_WindowsMediaPlayer instance: %08x\n", hres);

    hres = IOleObject_QueryInterface(oleobj, &IID_IConnectionPointContainer, (void**)&container);
    ok(hres == S_OK, "QueryInterface(IID_IConnectionPointContainer) failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IConnectionPointContainer_FindConnectionPoint(container, &IID__WMPOCXEvents, &point);
    IConnectionPointContainer_Release(container);
    ok(hres == S_OK, "FindConnectionPoint failed: %08x\n", hres);

    hres = IConnectionPoint_Advise(point, (IUnknown*)&WMPOCXEvents, &dw);
    ok(hres == S_OK, "Advise failed: %08x\n", hres);

    hres = IOleObject_QueryInterface(oleobj, &IID_IWMPPlayer4, (void**)&player4);
    ok(hres == S_OK, "Could not get IWMPPlayer4 iface: %08x\n", hres);

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
    ok(hres == S_OK, "IWMPPlayer4_put_URL failed: %08x\n", hres);
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
    ok(hres == S_OK, "Unadvise failed: %08x\n", hres);

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
    static const WCHAR currentPosition[] = {'c','u','r','r','e','n','t','P','o','s','i','t','i','o','n',0};
    BSTR bstrcurrentPosition = SysAllocString(currentPosition);

    hres = CoCreateInstance(&CLSID_WindowsMediaPlayer, NULL, CLSCTX_INPROC_SERVER, &IID_IOleObject, (void**)&oleobj);
    if(hres == REGDB_E_CLASSNOTREG) {
        win_skip("CLSID_WindowsMediaPlayer not registered\n");
        return FALSE;
    }
    ok(hres == S_OK, "Could not create CLSID_WindowsMediaPlayer instance: %08x\n", hres);

    hres = IOleObject_QueryInterface(oleobj, &IID_IConnectionPointContainer, (void**)&container);
    ok(hres == S_OK, "QueryInterface(IID_IConnectionPointContainer) failed: %08x\n", hres);

    hres = IConnectionPointContainer_FindConnectionPoint(container, &IID__WMPOCXEvents, &point);
    IConnectionPointContainer_Release(container);
    ok(hres == S_OK, "FindConnectionPoint failed: %08x\n", hres);

    hres = IConnectionPoint_Advise(point, (IUnknown*)&WMPOCXEvents, &dw);
    ok(hres == S_OK, "Advise failed: %08x\n", hres);

    hres = IOleObject_QueryInterface(oleobj, &IID_IWMPPlayer4, (void**)&player4);
    ok(hres == S_OK, "Could not get IWMPPlayer4 iface: %08x\n", hres);

    settings = NULL;
    hres = IWMPPlayer4_get_settings(player4, &settings);
    ok(hres == S_OK, "get_settings failed: %08x\n", hres);
    ok(settings != NULL, "settings = NULL\n");

    hres = IWMPSettings_put_autoStart(settings, VARIANT_FALSE);
    ok(hres == S_OK, "Could not put autoStart in IWMPSettings: %08x\n", hres);

    controls = NULL;
    hres = IWMPPlayer4_get_controls(player4, &controls);
    ok(hres == S_OK, "get_controls failed: %08x\n", hres);
    ok(controls != NULL, "controls = NULL\n");

    hres = IWMPControls_get_isAvailable(controls, bstrcurrentPosition, &vbool);
    ok(hres == S_OK, "IWMPControls_get_isAvailable failed: %08x\n", hres);
    ok(vbool == VARIANT_FALSE, "unexpected value\n");

    hres = IWMPControls_play(controls);
    ok(hres == NS_S_WMPCORE_COMMAND_NOT_AVAILABLE, "IWMPControls_play is available: %08x\n", hres);

    hres = IWMPSettings_put_volume(settings, 36);
    ok(hres == S_OK, "IWMPSettings_put_volume failed: %08x\n", hres);
    hres = IWMPSettings_get_volume(settings, &progress);
    ok(hres == S_OK, "IWMPSettings_get_volume failed: %08x\n", hres);
    ok(progress == 36, "unexpected value: %d\n", progress);

    filename = SysAllocString(load_resource(mp3file));

    SET_EXPECT(OPENSTATE, wmposPlaylistChanging);
    SET_EXPECT(OPENSTATE, wmposPlaylistOpenNoMedia);
    SET_EXPECT(OPENSTATE, wmposPlaylistChanged);
    SET_EXPECT(PLAYSTATE, wmppsTransitioning);
    SET_EXPECT(PLAYSTATE, wmppsReady);
    hres = IWMPPlayer4_put_URL(player4, filename);
    ok(hres == S_OK, "IWMPPlayer4_put_URL failed: %08x\n", hres);
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
    ok(hres == S_OK, "IWMPControls_play failed: %08x\n", hres);
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
    ok(hres == S_OK, "IWMPControls_get_isAvailable failed: %08x\n", hres);
    ok(vbool == VARIANT_TRUE, "unexpected value\n");

    duration = 0.0;
    hres = IWMPControls_get_currentPosition(controls, &duration);
    ok(hres == S_OK, "IWMPControls_get_currentPosition failed: %08x\n", hres);
    ok((int)duration == 0, "unexpected value %f\n", duration);

    duration = 1.1;
    hres = IWMPControls_put_currentPosition(controls, duration);
    ok(hres == S_OK, "IWMPControls_put_currentPosition failed: %08x\n", hres);

    duration = 0.0;
    hres = IWMPControls_get_currentPosition(controls, &duration);
    ok(hres == S_OK, "IWMPControls_get_currentPosition failed: %08x\n", hres);
    /* builtin quartz does not handle this currently and resets to 0.0, works
     * with native quartz */
    todo_wine ok(duration >= 1.05 /* save some fp errors */, "unexpected value %f\n", duration);

    hres = IWMPPlayer4_get_currentMedia(player4, &media);
    ok(hres == S_OK, "IWMPPlayer4_get_currentMedia failed: %08x\n", hres);
    hres = IWMPMedia_get_duration(media, &duration);
    ok(hres == S_OK, "IWMPMedia_get_duration failed: %08x\n", hres);
    ok(round(duration) == 3, "unexpected value: %f\n", duration);
    IWMPMedia_Release(media);

    network = NULL;
    hres = IWMPPlayer4_get_network(player4, &network);
    ok(hres == S_OK, "get_network failed: %08x\n", hres);
    ok(network != NULL, "network = NULL\n");
    progress = 0;
    hres = IWMPNetwork_get_bufferingProgress(network, &progress);
    ok(hres == S_OK || broken(hres == S_FALSE), "IWMPNetwork_get_bufferingProgress failed: %08x\n", hres);
    ok(progress == 100, "unexpected value: %d\n", progress);
    progress = 0;
    hres = IWMPNetwork_get_downloadProgress(network, &progress);
    ok(hres == S_OK, "IWMPNetwork_get_downloadProgress failed: %08x\n", hres);
    ok(progress == 100, "unexpected value: %d\n", progress);
    IWMPNetwork_Release(network);

    SET_EXPECT(PLAYSTATE, wmppsStopped);
    /* The following happens on wine only since we close media on stop */
    SET_EXPECT(OPENSTATE, wmposPlaylistOpenNoMedia);
    hres = IWMPControls_stop(controls);
    ok(hres == S_OK, "IWMPControls_stop failed: %08x\n", hres);
    CHECK_CALLED(PLAYSTATE, wmppsStopped);
    todo_wine CHECK_NOT_CALLED(OPENSTATE, wmposPlaylistOpenNoMedia);

    /* Already Stopped */
    hres = IWMPControls_stop(controls);
    ok(hres == NS_S_WMPCORE_COMMAND_NOT_AVAILABLE, "IWMPControls_stop is available: %08x\n", hres);

    SET_EXPECT(PLAYSTATE, wmppsPlaying);
    /* The following happens on wine only since we close media on stop */
    SET_EXPECT(OPENSTATE, wmposOpeningUnknownURL);
    SET_EXPECT(OPENSTATE, wmposMediaOpen);
    SET_EXPECT(PLAYSTATE, wmppsTransitioning);
    hres = IWMPControls_play(controls);
    ok(hres == S_OK, "IWMPControls_play failed: %08x\n", hres);
    CHECK_CALLED(PLAYSTATE, wmppsPlaying);
    todo_wine CHECK_NOT_CALLED(OPENSTATE, wmposOpeningUnknownURL);
    todo_wine CHECK_NOT_CALLED(OPENSTATE, wmposMediaOpen);
    todo_wine CHECK_NOT_CALLED(PLAYSTATE, wmppsTransitioning);

playback_skip:
    hres = IConnectionPoint_Unadvise(point, dw);
    ok(hres == S_OK, "Unadvise failed: %08x\n", hres);

    hres = IWMPSettings_get_volume(settings, &progress);
    ok(hres == S_OK, "IWMPSettings_get_volume failed: %08x\n", hres);
    ok(progress == 36, "unexpected value: %d\n", progress);
    hres = IWMPSettings_put_volume(settings, 99);
    ok(hres == S_OK, "IWMPSettings_put_volume failed: %08x\n", hres);
    hres = IWMPSettings_get_volume(settings, &progress);
    ok(hres == S_OK, "IWMPSettings_get_volume failed: %08x\n", hres);
    ok(progress == 99, "unexpected value: %d\n", progress);

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

START_TEST(media)
{
    CoInitialize(NULL);

    main_thread_id = GetCurrentThreadId();
    playing_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    completed_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (test_wmp()) {
        test_completion_event();
    } else {
        win_skip("Failed to play media\n");
    }

    CloseHandle(playing_event);
    CloseHandle(completed_event);

    CoUninitialize();
}
