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

struct can_convert
{
    DBTYPE type;
    DWORD can_convert_to;
} simple_convert[] =
{
    {DBTYPE_EMPTY,       0x23bfd9ff},
    {DBTYPE_NULL,        0x00001002},
    {DBTYPE_I2,          0x3b9fd9ff},
    {DBTYPE_I4,          0x3bdfd9ff},

    {DBTYPE_R4,          0x3b9fd9ff},
    {DBTYPE_R8,          0x3b9fd9ff},
    {DBTYPE_CY,          0x039fd97f},
    {DBTYPE_DATE,        0x399f99bf},

    {DBTYPE_BSTR,        0x3bffd9ff},
    {DBTYPE_IDISPATCH,   0x3bffffff},
    {DBTYPE_ERROR,       0x01001500},
    {DBTYPE_BOOL,        0x039fd9ff},

    {DBTYPE_VARIANT,     0x3bffffff},
    {DBTYPE_IUNKNOWN,    0x00003203},
    {DBTYPE_DECIMAL,     0x3b9fd97f},
    {DBTYPE_I1,          0x3b9fd9ff},

    {DBTYPE_UI1,         0x3b9fd9ff},
    {DBTYPE_UI2,         0x3b9fd9ff},
    {DBTYPE_UI4,         0x3bdfd9ff},
    {DBTYPE_I8,          0x03dfd97f},

    {DBTYPE_UI8,         0x03dfd97f},
    {DBTYPE_GUID,        0x01e01103},
    {DBTYPE_BYTES,       0x01fc110b},
    {DBTYPE_STR,         0x3bffd9ff},

    {DBTYPE_WSTR,        0x3bffd9ff},
    {DBTYPE_NUMERIC,     0x039fd97f},
    {DBTYPE_UDT,         0x00000000},
    {DBTYPE_DBDATE,      0x39801183},

    {DBTYPE_DBTIME,      0x39801183},
    {DBTYPE_DBTIMESTAMP, 0x39801183}
};


static void test_canconvert(void)
{
    IDataConvert *convert;
    HRESULT hr;
    int src_idx, dst_idx;

    hr = CoCreateInstance(&CLSID_OLEDB_CONVERSIONLIBRARY, NULL, CLSCTX_INPROC_SERVER, &IID_IDataConvert, (void**)&convert);
    if(FAILED(hr))
    {
        win_skip("Unable to load oledb conversion library\n");
        return;
    }

    /* Some older versions of the library don't support several conversions, we'll skip
       if we have such a library */
    hr = IDataConvert_CanConvert(convert, DBTYPE_EMPTY, DBTYPE_DBTIMESTAMP);
    if(hr == S_FALSE)
    {
        win_skip("Doesn't handle DBTYPE_EMPTY -> DBTYPE_DBTIMESTAMP conversion so skipping\n");
        IDataConvert_Release(convert);
        return;
    }

    /* Some older versions of the library don't support several conversions, we'll skip
       if we have such a library */
    hr = IDataConvert_CanConvert(convert, DBTYPE_EMPTY, DBTYPE_DBTIMESTAMP);
    if(hr == S_FALSE)
    {
        win_skip("Doesn't handle DBTYPE_EMPTY -> DBTYPE_DBTIMESTAMP conversion so skipping\n");
        IDataConvert_Release(convert);
        return;
    }

    for(src_idx = 0; src_idx < sizeof(simple_convert) / sizeof(simple_convert[0]); src_idx++)
        for(dst_idx = 0; dst_idx < sizeof(simple_convert) / sizeof(simple_convert[0]); dst_idx++)
        {
            BOOL expect;
            hr = IDataConvert_CanConvert(convert, simple_convert[src_idx].type, simple_convert[dst_idx].type);
            expect = (simple_convert[src_idx].can_convert_to >> dst_idx) & 1;
todo_wine
            ok((hr == S_OK && expect == TRUE) ||
               (hr == S_FALSE && expect == FALSE),
               "%04x -> %04x: got %08x expect conversion to be %spossible\n", simple_convert[src_idx].type,
               simple_convert[dst_idx].type, hr, expect ? "" : "not ");
        }

    IDataConvert_Release(convert);
}

START_TEST(convert)
{
    OleInitialize(NULL);
    test_dcinfo();
    test_canconvert();
    OleUninitialize();
}
