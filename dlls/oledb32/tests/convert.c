/* OLE DB Conversion library tests
 *
 * Copyright 2009 Huw Davies
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
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "msdadc.h"

#include "oledberr.h"

#include "initguid.h"
#include "msdaguid.h"

#include "wine/test.h"

static void test_dcinfo(void)
{
    IDCInfo *info;
    HRESULT hr;
    DCINFOTYPE types[2];
    DCINFO *inf;

    hr = CoCreateInstance(&CLSID_OLEDB_CONVERSIONLIBRARY, NULL, CLSCTX_INPROC_SERVER, &IID_IDCInfo, (void**)&info);
    if(FAILED(hr))
    {
        win_skip("Unable to load oledb conversion library\n");
        return;
    }

    types[0] = DCINFOTYPE_VERSION;
    hr = IDCInfo_GetInfo(info, 1, types, &inf);
    ok(hr == S_OK, "got %08x\n", hr);

    ok(inf->eInfoType == DCINFOTYPE_VERSION, "got %08x\n", inf->eInfoType);
    ok(V_VT(&inf->vData) == VT_UI4, "got %08x\n", V_VT(&inf->vData));
    ok(V_UI4(&inf->vData) == 0x110, "got %08x\n", V_UI4(&inf->vData));

    V_UI4(&inf->vData) = 0x200;
    hr = IDCInfo_SetInfo(info, 1, inf);
    ok(hr == S_OK, "got %08x\n", hr);
    CoTaskMemFree(inf);

    hr = IDCInfo_GetInfo(info, 1, types, &inf);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(inf->eInfoType == DCINFOTYPE_VERSION, "got %08x\n", inf->eInfoType);
    ok(V_VT(&inf->vData) == VT_UI4, "got %08x\n", V_VT(&inf->vData));
    ok(V_UI4(&inf->vData) == 0x200, "got %08x\n", V_UI4(&inf->vData));

    V_UI4(&inf->vData) = 0x100;
    hr = IDCInfo_SetInfo(info, 1, inf);
    ok(hr == S_OK, "got %08x\n", hr);
    CoTaskMemFree(inf);

    hr = IDCInfo_GetInfo(info, 1, types, &inf);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(inf->eInfoType == DCINFOTYPE_VERSION, "got %08x\n", inf->eInfoType);
    ok(V_VT(&inf->vData) == VT_UI4, "got %08x\n", V_VT(&inf->vData));
    ok(V_UI4(&inf->vData) == 0x100, "got %08x\n", V_UI4(&inf->vData));

    V_UI4(&inf->vData) = 0x500;
    hr = IDCInfo_SetInfo(info, 1, inf);
    ok(hr == S_OK, "got %08x\n", hr);
    CoTaskMemFree(inf);

    hr = IDCInfo_GetInfo(info, 1, types, &inf);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(inf->eInfoType == DCINFOTYPE_VERSION, "got %08x\n", inf->eInfoType);
    ok(V_VT(&inf->vData) == VT_UI4, "got %08x\n", V_VT(&inf->vData));
    ok(V_UI4(&inf->vData) == 0x500, "got %08x\n", V_UI4(&inf->vData));

    V_UI4(&inf->vData) = 0xffff;
    hr = IDCInfo_SetInfo(info, 1, inf);
    ok(hr == S_OK, "got %08x\n", hr);
    CoTaskMemFree(inf);

    hr = IDCInfo_GetInfo(info, 1, types, &inf);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(inf->eInfoType == DCINFOTYPE_VERSION, "got %08x\n", inf->eInfoType);
    ok(V_VT(&inf->vData) == VT_UI4, "got %08x\n", V_VT(&inf->vData));
    ok(V_UI4(&inf->vData) == 0xffff, "got %08x\n", V_UI4(&inf->vData));

    V_UI4(&inf->vData) = 0x12345678;
    hr = IDCInfo_SetInfo(info, 1, inf);
    ok(hr == S_OK, "got %08x\n", hr);
    CoTaskMemFree(inf);

    hr = IDCInfo_GetInfo(info, 1, types, &inf);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(inf->eInfoType == DCINFOTYPE_VERSION, "got %08x\n", inf->eInfoType);
    ok(V_VT(&inf->vData) == VT_UI4, "got %08x\n", V_VT(&inf->vData));
    ok(V_UI4(&inf->vData) == 0x12345678, "got %08x\n", V_UI4(&inf->vData));

    /* Try setting a version variant of something other than VT_UI4 */
    V_VT(&inf->vData) = VT_I4;
    V_I4(&inf->vData) = 0x200;
    hr = IDCInfo_SetInfo(info, 1, inf);
    ok(hr == DB_S_ERRORSOCCURRED, "got %08x\n", hr);
    CoTaskMemFree(inf);

    hr = IDCInfo_GetInfo(info, 1, types, &inf);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(inf->eInfoType == DCINFOTYPE_VERSION, "got %08x\n", inf->eInfoType);
    ok(V_VT(&inf->vData) == VT_UI4, "got %08x\n", V_VT(&inf->vData));
    ok(V_UI4(&inf->vData) == 0x12345678, "got %08x\n", V_UI4(&inf->vData));
    CoTaskMemFree(inf);

    /* More than one type */
    types[1] = 2;
    hr = IDCInfo_GetInfo(info, 2, types, &inf);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(inf[0].eInfoType == DCINFOTYPE_VERSION, "got %08x\n", inf[0].eInfoType);
    ok(V_VT(&inf[0].vData) == VT_UI4, "got %08x\n", V_VT(&inf[0].vData));
    ok(V_UI4(&inf[0].vData) == 0x12345678, "got %08x\n", V_UI4(&inf[0].vData));
    ok(inf[1].eInfoType == 2, "got %08x\n", inf[1].eInfoType);
    ok(V_VT(&inf[1].vData) == VT_EMPTY, "got %08x\n", V_VT(&inf[1].vData));

    hr = IDCInfo_SetInfo(info, 2, inf);
    ok(hr == S_OK, "got %08x\n", hr);
    CoTaskMemFree(inf);


    IDCInfo_Release(info);
}

START_TEST(convert)
{
    OleInitialize(NULL);
    test_dcinfo();
    OleUninitialize();
}
