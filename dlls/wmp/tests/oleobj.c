/*
 * Copyright 2014 Jacek Caban for CodeWeavers
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

#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#include <initguid.h>
#include <windows.h>
#include <wmp.h>

#include "wine/test.h"

static void test_wmp(void)
{
    IProvideClassInfo2 *class_info;
    IOleObject *oleobj;
    GUID guid;
    LONG ref;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_WindowsMediaPlayer, NULL, CLSCTX_INPROC_SERVER, &IID_IOleObject, (void**)&oleobj);
    if(hres == REGDB_E_CLASSNOTREG) {
        win_skip("CLSID_WindowsMediaPlayer not registered\n");
        return;
    }
    ok(hres == S_OK, "Coult not create CLSID_WindowsMediaPlayer instance: %08x\n", hres);

    hres = IOleObject_QueryInterface(oleobj, &IID_IProvideClassInfo2, (void**)&class_info);
    ok(hres == S_OK, "Could not get IProvideClassInfo2 iface: %08x\n", hres);

    hres = IProvideClassInfo2_GetGUID(class_info, GUIDKIND_DEFAULT_SOURCE_DISP_IID, &guid);
    ok(hres == S_OK, "GetGUID failed: %08x\n", hres);
    ok(IsEqualGUID(&guid, &IID__WMPOCXEvents), "guid = %s\n", wine_dbgstr_guid(&guid));

    IProvideClassInfo2_Release(class_info);

    ref = IOleObject_Release(oleobj);
    ok(!ref, "ref = %d\n", ref);
}

START_TEST(oleobj)
{
    CoInitialize(NULL);

    test_wmp();

    CoUninitialize();
}
