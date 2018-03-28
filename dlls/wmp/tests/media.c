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

#include "wine/test.h"

static const WCHAR mp3file[] = {'t','e','s','t','.','m','p','3',0};
static inline WCHAR *load_resource(const WCHAR *name)
{
    static WCHAR pathW[MAX_PATH];
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    GetTempPathW(sizeof(pathW)/sizeof(WCHAR), pathW);
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

static void test_wmp(void)
{
    IWMPPlayer4 *player4;
    IWMPControls *controls;
    HRESULT hres;
    BSTR filename;

    IOleObject *oleobj;
    IWMPSettings *settings;

    hres = CoCreateInstance(&CLSID_WindowsMediaPlayer, NULL, CLSCTX_INPROC_SERVER, &IID_IOleObject, (void**)&oleobj);
    if(hres == REGDB_E_CLASSNOTREG) {
        win_skip("CLSID_WindowsMediaPlayer not registered\n");
        return;
    }
    ok(hres == S_OK, "Could not create CLSID_WindowsMediaPlayer instance: %08x\n", hres);

    hres = IOleObject_QueryInterface(oleobj, &IID_IWMPPlayer4, (void**)&player4);
    ok(hres == S_OK, "Could not get IWMPPlayer4 iface: %08x\n", hres);

    settings = NULL;
    hres = IWMPPlayer4_get_settings(player4, &settings);
    ok(hres == S_OK, "get_settings failed: %08x\n", hres);
    ok(settings != NULL, "settings = NULL\n");

    hres = IWMPSettings_put_autoStart(settings, VARIANT_FALSE);
    ok(hres == S_OK, "Could not put autoStart in IWMPSettings: %08x\n", hres);
    IWMPSettings_Release(settings);

    controls = NULL;
    hres = IWMPPlayer4_get_controls(player4, &controls);
    ok(hres == S_OK, "get_controls failed: %08x\n", hres);
    ok(controls != NULL, "controls = NULL\n");

    hres = IWMPControls_play(controls);
    ok(hres == NS_S_WMPCORE_COMMAND_NOT_AVAILABLE, "IWMPControls_play is available: %08x\n", hres);

    filename = SysAllocString(load_resource(mp3file));

    hres = IWMPPlayer4_put_URL(player4, filename);
    ok(hres == S_OK, "IWMPPlayer4_put_URL failed: %08x\n", hres);

    hres = IWMPControls_play(controls);
    ok(hres == S_OK, "IWMPControls_play failed: %08x\n", hres);

    hres = IWMPControls_stop(controls);
    ok(hres == S_OK, "IWMPControls_stop failed: %08x\n", hres);

    /* Already Stopped */
    hres = IWMPControls_stop(controls);
    ok(hres == NS_S_WMPCORE_COMMAND_NOT_AVAILABLE, "IWMPControls_stop is available: %08x\n", hres);

    hres = IWMPControls_play(controls);
    ok(hres == S_OK, "IWMPControls_play failed: %08x\n", hres);

    IWMPControls_Release(controls);
    IWMPPlayer4_Release(player4);
    IOleObject_Release(oleobj);
    DeleteFileW(filename);
    SysFreeString(filename);
}

START_TEST(media)
{
    CoInitialize(NULL);

    test_wmp();

    CoUninitialize();
}
