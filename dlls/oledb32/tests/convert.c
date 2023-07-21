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
#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "msdadc.h"

#include "oledberr.h"

#include "initguid.h"
#include "msdaguid.h"

#include "wine/test.h"

static IDataConvert *convert;

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
    ok(hr == S_OK, "got %08lx\n", hr);

    ok(inf->eInfoType == DCINFOTYPE_VERSION, "got %08lx\n", inf->eInfoType);
    ok(V_VT(&inf->vData) == VT_UI4, "got %08x\n", V_VT(&inf->vData));
    ok(V_UI4(&inf->vData) == 0x110, "got %08lx\n", V_UI4(&inf->vData));

    V_UI4(&inf->vData) = 0x200;
    hr = IDCInfo_SetInfo(info, 1, inf);
    ok(hr == S_OK, "got %08lx\n", hr);
    CoTaskMemFree(inf);

    hr = IDCInfo_GetInfo(info, 1, types, &inf);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(inf->eInfoType == DCINFOTYPE_VERSION, "got %08lx\n", inf->eInfoType);
    ok(V_VT(&inf->vData) == VT_UI4, "got %08x\n", V_VT(&inf->vData));
    ok(V_UI4(&inf->vData) == 0x200, "got %08lx\n", V_UI4(&inf->vData));

    V_UI4(&inf->vData) = 0x100;
    hr = IDCInfo_SetInfo(info, 1, inf);
    ok(hr == S_OK, "got %08lx\n", hr);
    CoTaskMemFree(inf);

    hr = IDCInfo_GetInfo(info, 1, types, &inf);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(inf->eInfoType == DCINFOTYPE_VERSION, "got %08lx\n", inf->eInfoType);
    ok(V_VT(&inf->vData) == VT_UI4, "got %08x\n", V_VT(&inf->vData));
    ok(V_UI4(&inf->vData) == 0x100, "got %08lx\n", V_UI4(&inf->vData));

    V_UI4(&inf->vData) = 0x500;
    hr = IDCInfo_SetInfo(info, 1, inf);
    ok(hr == S_OK, "got %08lx\n", hr);
    CoTaskMemFree(inf);

    hr = IDCInfo_GetInfo(info, 1, types, &inf);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(inf->eInfoType == DCINFOTYPE_VERSION, "got %08lx\n", inf->eInfoType);
    ok(V_VT(&inf->vData) == VT_UI4, "got %08x\n", V_VT(&inf->vData));
    ok(V_UI4(&inf->vData) == 0x500, "got %08lx\n", V_UI4(&inf->vData));

    V_UI4(&inf->vData) = 0xffff;
    hr = IDCInfo_SetInfo(info, 1, inf);
    ok(hr == S_OK, "got %08lx\n", hr);
    CoTaskMemFree(inf);

    hr = IDCInfo_GetInfo(info, 1, types, &inf);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(inf->eInfoType == DCINFOTYPE_VERSION, "got %08lx\n", inf->eInfoType);
    ok(V_VT(&inf->vData) == VT_UI4, "got %08x\n", V_VT(&inf->vData));
    ok(V_UI4(&inf->vData) == 0xffff, "got %08lx\n", V_UI4(&inf->vData));

    V_UI4(&inf->vData) = 0x12345678;
    hr = IDCInfo_SetInfo(info, 1, inf);
    ok(hr == S_OK, "got %08lx\n", hr);
    CoTaskMemFree(inf);

    hr = IDCInfo_GetInfo(info, 1, types, &inf);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(inf->eInfoType == DCINFOTYPE_VERSION, "got %08lx\n", inf->eInfoType);
    ok(V_VT(&inf->vData) == VT_UI4, "got %08x\n", V_VT(&inf->vData));
    ok(V_UI4(&inf->vData) == 0x12345678, "got %08lx\n", V_UI4(&inf->vData));

    /* Try setting a version variant of something other than VT_UI4 */
    V_VT(&inf->vData) = VT_I4;
    V_I4(&inf->vData) = 0x200;
    hr = IDCInfo_SetInfo(info, 1, inf);
    ok(hr == DB_S_ERRORSOCCURRED, "got %08lx\n", hr);
    CoTaskMemFree(inf);

    hr = IDCInfo_GetInfo(info, 1, types, &inf);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(inf->eInfoType == DCINFOTYPE_VERSION, "got %08lx\n", inf->eInfoType);
    ok(V_VT(&inf->vData) == VT_UI4, "got %08x\n", V_VT(&inf->vData));
    ok(V_UI4(&inf->vData) == 0x12345678, "got %08lx\n", V_UI4(&inf->vData));
    CoTaskMemFree(inf);

    /* More than one type */
    types[1] = 2;
    hr = IDCInfo_GetInfo(info, 2, types, &inf);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(inf[0].eInfoType == DCINFOTYPE_VERSION, "got %08lx\n", inf[0].eInfoType);
    ok(V_VT(&inf[0].vData) == VT_UI4, "got %08x\n", V_VT(&inf[0].vData));
    ok(V_UI4(&inf[0].vData) == 0x12345678, "got %08lx\n", V_UI4(&inf[0].vData));
    ok(inf[1].eInfoType == 2, "got %08lx\n", inf[1].eInfoType);
    ok(V_VT(&inf[1].vData) == VT_EMPTY, "got %08x\n", V_VT(&inf[1].vData));

    hr = IDCInfo_SetInfo(info, 2, inf);
    ok(hr == S_OK, "got %08lx\n", hr);
    CoTaskMemFree(inf);


    IDCInfo_Release(info);
}

static const struct can_convert
{
    DBTYPE type;
    DWORD can_convert_to;
} simple_convert[] =
{
    {DBTYPE_EMPTY,       0x63bfd9ff},
    {DBTYPE_NULL,        0x40001002},
    {DBTYPE_I2,          0x3b9fd9ff},
    {DBTYPE_I4,          0x3bdfd9ff},

    {DBTYPE_R4,          0x3b9fd9ff},
    {DBTYPE_R8,          0x3b9fd9ff},
    {DBTYPE_CY,          0x039fd97f},
    {DBTYPE_DATE,        0x799f99bf},

    {DBTYPE_BSTR,        0x7bffd9ff},
    {DBTYPE_IDISPATCH,   0x7bffffff},
    {DBTYPE_ERROR,       0x01001500},
    {DBTYPE_BOOL,        0x039fd9ff},

    {DBTYPE_VARIANT,     0x7bffffff},
    {DBTYPE_IUNKNOWN,    0x00003203},
    {DBTYPE_DECIMAL,     0x3b9fd97f},
    {DBTYPE_I1,          0x3b9fd9ff},

    {DBTYPE_UI1,         0x3b9fd9ff},
    {DBTYPE_UI2,         0x3b9fd9ff},
    {DBTYPE_UI4,         0x3bdfd9ff},
    {DBTYPE_I8,          0x43dfd97f},

    {DBTYPE_UI8,         0x43dfd97f},
    {DBTYPE_GUID,        0x01e01103},
    {DBTYPE_BYTES,       0x01fc110b},
    {DBTYPE_STR,         0x7bffd9ff},

    {DBTYPE_WSTR,        0x7bffd9ff},
    {DBTYPE_NUMERIC,     0x039fd97f},
    {DBTYPE_UDT,         0x00000000},
    {DBTYPE_DBDATE,      0x79801183},

    {DBTYPE_DBTIME,      0x79801183},
    {DBTYPE_DBTIMESTAMP, 0x79801183},
    {DBTYPE_FILETIME,    0x79981183}
};


static inline BOOL array_type(DBTYPE type)
{
    return (type >= DBTYPE_I2 && type <= DBTYPE_UI4);
}

static void test_canconvert(void)
{
    HRESULT hr;
    int src_idx, dst_idx;

    /* Some older versions of the library don't support several conversions, we'll skip
       if we have such a library */
    hr = IDataConvert_CanConvert(convert, DBTYPE_EMPTY, DBTYPE_DBTIMESTAMP);
    if(hr == S_FALSE)
    {
        win_skip("Doesn't handle DBTYPE_EMPTY -> DBTYPE_DBTIMESTAMP conversion so skipping\n");
        return;
    }

    /* Some older versions of the library don't support several conversions, we'll skip
       if we have such a library */
    hr = IDataConvert_CanConvert(convert, DBTYPE_EMPTY, DBTYPE_DBTIMESTAMP);
    if(hr == S_FALSE)
    {
        win_skip("Doesn't handle DBTYPE_EMPTY -> DBTYPE_DBTIMESTAMP conversion so skipping\n");
        return;
    }

    for(src_idx = 0; src_idx < ARRAY_SIZE(simple_convert); src_idx++)
        for(dst_idx = 0; dst_idx < ARRAY_SIZE(simple_convert); dst_idx++)
        {
            BOOL expect, simple_expect;
            simple_expect = (simple_convert[src_idx].can_convert_to >> dst_idx) & 1;

            hr = IDataConvert_CanConvert(convert, simple_convert[src_idx].type, simple_convert[dst_idx].type);
            expect = simple_expect;
            ok((hr == S_OK && expect == TRUE) ||
               (hr == S_FALSE && expect == FALSE),
               "%04x -> %04x: got %08lx expect conversion to be %spossible\n", simple_convert[src_idx].type,
               simple_convert[dst_idx].type, hr, expect ? "" : "not ");

            /* src DBTYPE_BYREF */
            hr = IDataConvert_CanConvert(convert, simple_convert[src_idx].type | DBTYPE_BYREF, simple_convert[dst_idx].type);
            expect = simple_expect;
            ok((hr == S_OK && expect == TRUE) ||
               (hr == S_FALSE && expect == FALSE),
               "%04x -> %04x: got %08lx expect conversion to be %spossible\n", simple_convert[src_idx].type | DBTYPE_BYREF,
               simple_convert[dst_idx].type, hr, expect ? "" : "not ");

            /* dst DBTYPE_BYREF */
            hr = IDataConvert_CanConvert(convert, simple_convert[src_idx].type, simple_convert[dst_idx].type | DBTYPE_BYREF);
            expect = FALSE;
            if(simple_expect &&
               (simple_convert[dst_idx].type == DBTYPE_BYTES ||
                simple_convert[dst_idx].type == DBTYPE_STR ||
                simple_convert[dst_idx].type == DBTYPE_WSTR))
                expect = TRUE;
            ok((hr == S_OK && expect == TRUE) ||
               (hr == S_FALSE && expect == FALSE),
               "%04x -> %04x: got %08lx expect conversion to be %spossible\n", simple_convert[src_idx].type,
               simple_convert[dst_idx].type | DBTYPE_BYREF, hr, expect ? "" : "not ");

            /* src & dst DBTYPE_BYREF */
            hr = IDataConvert_CanConvert(convert, simple_convert[src_idx].type | DBTYPE_BYREF, simple_convert[dst_idx].type | DBTYPE_BYREF);
            expect = FALSE;
            if(simple_expect &&
               (simple_convert[dst_idx].type == DBTYPE_BYTES ||
                simple_convert[dst_idx].type == DBTYPE_STR ||
                simple_convert[dst_idx].type == DBTYPE_WSTR))
                expect = TRUE;
            ok((hr == S_OK && expect == TRUE) ||
               (hr == S_FALSE && expect == FALSE),
               "%04x -> %04x: got %08lx expect conversion to be %spossible\n", simple_convert[src_idx].type | DBTYPE_BYREF,
               simple_convert[dst_idx].type | DBTYPE_BYREF, hr, expect ? "" : "not ");

            /* src DBTYPE_ARRAY */
            hr = IDataConvert_CanConvert(convert, simple_convert[src_idx].type | DBTYPE_ARRAY, simple_convert[dst_idx].type);
            expect = FALSE;
            if(array_type(simple_convert[src_idx].type) && simple_convert[dst_idx].type == DBTYPE_VARIANT)
                expect = TRUE;
            ok((hr == S_OK && expect == TRUE) ||
               (hr == S_FALSE && expect == FALSE),
               "%04x -> %04x: got %08lx expect conversion to be %spossible\n", simple_convert[src_idx].type | DBTYPE_ARRAY,
               simple_convert[dst_idx].type, hr, expect ? "" : "not ");

            /* dst DBTYPE_ARRAY */
            hr = IDataConvert_CanConvert(convert, simple_convert[src_idx].type, simple_convert[dst_idx].type | DBTYPE_ARRAY);
            expect = FALSE;
            if(array_type(simple_convert[dst_idx].type) &&
               (simple_convert[src_idx].type == DBTYPE_IDISPATCH ||
                simple_convert[src_idx].type == DBTYPE_VARIANT))
                expect = TRUE;
            ok((hr == S_OK && expect == TRUE) ||
               (hr == S_FALSE && expect == FALSE),
               "%04x -> %04x: got %08lx expect conversion to be %spossible\n", simple_convert[src_idx].type,
               simple_convert[dst_idx].type | DBTYPE_ARRAY, hr, expect ? "" : "not ");

            /* src & dst DBTYPE_ARRAY */
            hr = IDataConvert_CanConvert(convert, simple_convert[src_idx].type | DBTYPE_ARRAY, simple_convert[dst_idx].type | DBTYPE_ARRAY);
            expect = FALSE;
            if(array_type(simple_convert[src_idx].type) &&
               simple_convert[src_idx].type == simple_convert[dst_idx].type)
                expect = TRUE;
            ok((hr == S_OK && expect == TRUE) ||
               (hr == S_FALSE && expect == FALSE),
               "%04x -> %04x: got %08lx expect conversion to be %spossible\n", simple_convert[src_idx].type | DBTYPE_ARRAY,
               simple_convert[dst_idx].type | DBTYPE_ARRAY, hr, expect ? "" : "not ");

            /* src DBTYPE_VECTOR */
            hr = IDataConvert_CanConvert(convert, simple_convert[src_idx].type | DBTYPE_VECTOR, simple_convert[dst_idx].type);
            ok(hr == S_FALSE,
               "%04x -> %04x: got %08lx expect conversion to not be possible\n", simple_convert[src_idx].type | DBTYPE_VECTOR,
               simple_convert[dst_idx].type, hr);

            /* dst DBTYPE_VECTOR */
            hr = IDataConvert_CanConvert(convert, simple_convert[src_idx].type, simple_convert[dst_idx].type | DBTYPE_VECTOR);
            ok(hr == S_FALSE,
               "%04x -> %04x: got %08lx expect conversion to not be possible\n", simple_convert[src_idx].type,
               simple_convert[dst_idx].type | DBTYPE_VECTOR, hr);

            /* src & dst DBTYPE_VECTOR */
            hr = IDataConvert_CanConvert(convert, simple_convert[src_idx].type | DBTYPE_VECTOR, simple_convert[dst_idx].type | DBTYPE_VECTOR);
            ok(hr == S_FALSE,
               "%04x -> %04x: got %08lx expect conversion to not be possible\n", simple_convert[src_idx].type | DBTYPE_VECTOR,
               simple_convert[dst_idx].type | DBTYPE_VECTOR, hr);


        }
}

static void test_converttoi1(void)
{
    HRESULT hr;
    signed char dst;
    BYTE src[sizeof(VARIANT)]; /* assuming that VARIANT is larger than all the types used in src */
    DBSTATUS dst_status;
    DBLENGTH dst_len;
    static const WCHAR ten[] = {'1','0',0};
    BSTR b;

    dst_len = dst = 0x12;
    hr = IDataConvert_DataConvert(convert, DBTYPE_EMPTY, DBTYPE_I1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0, "got %08x\n", dst);

    dst_len = dst = 0x12;
    hr = IDataConvert_DataConvert(convert, DBTYPE_NULL, DBTYPE_I1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == DB_E_UNSUPPORTEDCONVERSION, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_BADACCESSOR, "got %08lx\n", dst_status);
    ok(dst_len == 0x12, "got %Id\n", dst_len);
    ok(dst == 0x12, "got %08x\n", dst);

    dst_len = dst = 0x12;
    *(short *)src = 0x43;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I2, DBTYPE_I1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x43, "got %08x\n", dst);

    dst_len = dst = 0x12;
    *(int *)src = 0x4321cafe;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I4, DBTYPE_I1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    todo_wine
    ok(hr == DB_E_DATAOVERFLOW, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_DATAOVERFLOW, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x12, "got %08x\n", dst);

    dst_len = dst = 0x12;
    *(int *)src = 0x43;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I4, DBTYPE_I1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x43, "got %08x\n", dst);

    dst_len = dst = 0x12;
    *(FLOAT *)src = 10.75;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R4, DBTYPE_I1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 11, "got %08x\n", dst);

    dst_len = dst = 0x12;
    *(FLOAT *)src = -10.75;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R4, DBTYPE_I1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == -11, "got %08x\n", dst);

    dst_len = dst = 0x12;
    *(double *)src = 10.75;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R8, DBTYPE_I1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 11, "got %08x\n", dst);

    dst_len = dst = 0x12;
    ((LARGE_INTEGER *)src)->QuadPart = 107500;
    hr = IDataConvert_DataConvert(convert, DBTYPE_CY, DBTYPE_I1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 11, "got %08x\n", dst);

    dst_len = dst = 0x12;
    *(DATE *)src = 10.7500;
    hr = IDataConvert_DataConvert(convert, DBTYPE_DATE, DBTYPE_I1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 11, "got %08x\n", dst);

    dst_len = dst = 0x12;
    b = SysAllocString(ten);
    *(BSTR *)src = b;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BSTR, DBTYPE_I1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 10, "got %08x\n", dst);
    SysFreeString(b);

    dst_len = dst = 0x12;
    *(SCODE *)src = 0x4321cafe;
    hr = IDataConvert_DataConvert(convert, DBTYPE_ERROR, DBTYPE_I1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == DB_E_UNSUPPORTEDCONVERSION, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_BADACCESSOR, "got %08lx\n", dst_status);
    ok(dst_len == 0x12, "got %Id\n", dst_len);
    ok(dst == 0x12, "got %08x\n", dst);

    dst_len = dst = 0x12;
    *(VARIANT_BOOL *)src = VARIANT_TRUE;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BOOL, DBTYPE_I1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == -1, "got %08x\n", dst);

    dst_len = dst = 0x12;
    *(VARIANT_BOOL *)src = VARIANT_FALSE;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BOOL, DBTYPE_I1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0, "got %08x\n", dst);

    dst_len = dst = 0x12;
    V_VT((VARIANT*)src) = VT_I2;
    V_I2((VARIANT*)src) = 0x43;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_I1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x43, "got %08x\n", dst);

    V_VT((VARIANT*)src) = VT_NULL;
    dst_len = 0x12;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_I1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_ISNULL, "got %08lx\n", dst_status);
    ok(dst_len == 0x12, "got %Id\n", dst_len);

    dst_len = dst = 0x12;
    memset(src, 0, sizeof(DECIMAL));
    ((DECIMAL*)src)->Lo64 = 0x43;
    hr = IDataConvert_DataConvert(convert, DBTYPE_DECIMAL, DBTYPE_I1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x43, "got %08x\n", dst);

    dst_len = dst = 0x12;
    *(signed char*)src = 0x70;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I1, DBTYPE_I1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x70, "got %08x\n", dst);

    dst_len = dst = 0x12;
    *(BYTE*)src = 0x70;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI1, DBTYPE_I1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x70, "got %08x\n", dst);

    dst_len = dst = 0x12;
    *(WORD*)src = 0xC8;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI1, DBTYPE_I1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == DB_E_ERRORSOCCURRED, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_DATAOVERFLOW, "got %08lx\n", dst_status);
    todo_wine
    ok(broken(dst_len == sizeof(dst)) || dst_len == 0x12 /* W2K+ */, "got %Id\n", dst_len);
    ok(dst == 0x12, "got %08x\n", dst);

    dst_len = dst = 0x12;
    *(WORD*)src = 0x43;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI2, DBTYPE_I1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x43, "got %08x\n", dst);

    dst_len = dst = 0x12;
    *(DWORD*)src = 0xabcd1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI4, DBTYPE_I1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    todo_wine
    ok(hr == DB_E_DATAOVERFLOW, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_DATAOVERFLOW, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x12, "got %08x\n", dst);

    dst_len = dst = 0x12;
    *(DWORD*)src = 0x12abcd;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI4, DBTYPE_I1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    todo_wine
    ok(hr == DB_E_DATAOVERFLOW, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_DATAOVERFLOW, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x12, "got %08x\n", dst);

    dst_len = dst = 0x12;
    *(DWORD*)src = 0x43;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI4, DBTYPE_I1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x43, "got %08x\n", dst);

    dst_len = dst = 0x12;
    ((LARGE_INTEGER*)src)->QuadPart = 0x12abcd;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I8, DBTYPE_I1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == DB_E_ERRORSOCCURRED ||
       broken(hr == DB_E_UNSUPPORTEDCONVERSION), /* win98 */
       "got %08lx\n", hr);
    if(hr != DB_E_UNSUPPORTEDCONVERSION) /* win98 doesn't support I8/UI8 */
    {
        ok(dst_status == DBSTATUS_E_DATAOVERFLOW, "got %08lx\n", dst_status);
        ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
        ok(dst == 0x12, "got %08x\n", dst);

        dst_len = dst = 0x12;
        ((LARGE_INTEGER*)src)->QuadPart = 0x43;
        hr = IDataConvert_DataConvert(convert, DBTYPE_I8, DBTYPE_I1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
        ok(hr == S_OK, "got %08lx\n", hr);
        ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
        ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
        ok(dst == 0x43, "got %08x\n", dst);

        dst_len = dst = 0x12;
        ((ULARGE_INTEGER*)src)->QuadPart = 0x43;
        hr = IDataConvert_DataConvert(convert, DBTYPE_UI8, DBTYPE_I1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
        ok(hr == S_OK, "got %08lx\n", hr);
        ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
        ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
        ok(dst == 0x43, "got %08x\n", dst);
    }

    dst_len = dst = 0x12;
    strcpy((char *)src, "10");
    hr = IDataConvert_DataConvert(convert, DBTYPE_STR, DBTYPE_I1, 2, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 10, "got %08x\n", dst);

    dst_len = dst = 0x12;
    strcpy((char *)src, "10");
    hr = IDataConvert_DataConvert(convert, DBTYPE_STR, DBTYPE_I1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, DBDATACONVERT_LENGTHFROMNTS);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 10, "got %08x\n", dst);

    dst_len = dst = 0x12;
    memcpy(src, ten, sizeof(ten));
    hr = IDataConvert_DataConvert(convert, DBTYPE_WSTR, DBTYPE_I1, 4, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 10, "got %08x\n", dst);

    dst_len = dst = 0x12;
    memcpy(src, ten, sizeof(ten));
    hr = IDataConvert_DataConvert(convert, DBTYPE_WSTR, DBTYPE_I1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, DBDATACONVERT_LENGTHFROMNTS);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 10, "got %08x\n", dst);

    dst_len = dst = 0x12;
    *(WORD*)src = 0x43;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI2, DBTYPE_UI1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x43, "got %08x\n", dst);

    dst_len = dst = 0x12;
    *(DWORD*)src = 0xabcd1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI4, DBTYPE_UI1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    todo_wine
    ok(hr == DB_E_DATAOVERFLOW, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_DATAOVERFLOW, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x12, "got %08x\n", dst);

    dst_len = dst = 0x12;
    *(DWORD*)src = 0x12abcd;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI4, DBTYPE_UI1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    todo_wine
    ok(hr == DB_E_DATAOVERFLOW, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_DATAOVERFLOW, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x12, "got %08x\n", dst);

    dst_len = dst = 0x12;
    *(DWORD*)src = 0x43;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI4, DBTYPE_UI1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x43, "got %08x\n", dst);

    dst_len = dst = 0x12;
    memcpy(src, ten, sizeof(ten));
    hr = IDataConvert_DataConvert(convert, DBTYPE_WSTR, DBTYPE_UI1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, DBDATACONVERT_LENGTHFROMNTS);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 10, "got %08x\n", dst);
}

static void test_converttoi2(void)
{
    HRESULT hr;
    signed short dst;
    BYTE src[sizeof(VARIANT)]; /* assuming that VARIANT is larger than all the types used in src */
    DBSTATUS dst_status;
    DBLENGTH dst_len;
    static const WCHAR ten[] = {'1','0',0};
    BSTR b;

    dst_len = dst = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_EMPTY, DBTYPE_I2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0, "got %08x\n", dst);

    dst_len = dst = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_NULL, DBTYPE_I2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == DB_E_UNSUPPORTEDCONVERSION, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_BADACCESSOR, "got %08lx\n", dst_status);
    ok(dst_len == 0x1234, "got %Id\n", dst_len);
    ok(dst == 0x1234, "got %08x\n", dst);

    dst_len = dst = 0x1234;
    *(short *)src = 0x4321;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I2, DBTYPE_I2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x4321, "got %08x\n", dst);

    dst_len = dst = 0x1234;
    *(int *)src = 0x4321cafe;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I4, DBTYPE_I2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    todo_wine
    ok(hr == DB_E_DATAOVERFLOW, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_DATAOVERFLOW, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x1234, "got %08x\n", dst);

    dst_len = dst = 0x1234;
    *(int *)src = 0x4321;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I4, DBTYPE_I2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x4321, "got %08x\n", dst);

    dst_len = dst = 0x1234;
    *(FLOAT *)src = 10.75;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R4, DBTYPE_I2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 11, "got %08x\n", dst);

    dst_len = dst = 0x1234;
    *(FLOAT *)src = -10.75;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R4, DBTYPE_I2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == -11, "got %08x\n", dst);

    dst_len = dst = 0x1234;
    *(double *)src = 10.75;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R8, DBTYPE_I2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 11, "got %08x\n", dst);

    dst_len = dst = 0x1234;
    ((LARGE_INTEGER *)src)->QuadPart = 107500;
    hr = IDataConvert_DataConvert(convert, DBTYPE_CY, DBTYPE_I2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 11, "got %08x\n", dst);

    dst_len = dst = 0x1234;
    *(DATE *)src = 10.7500;
    hr = IDataConvert_DataConvert(convert, DBTYPE_DATE, DBTYPE_I2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 11, "got %08x\n", dst);

    dst_len = dst = 0x1234;
    b = SysAllocString(ten);
    *(BSTR *)src = b;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BSTR, DBTYPE_I2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 10, "got %08x\n", dst);
    SysFreeString(b);

    dst_len = dst = 0x1234;
    *(SCODE *)src = 0x4321cafe;
    hr = IDataConvert_DataConvert(convert, DBTYPE_ERROR, DBTYPE_I2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == DB_E_UNSUPPORTEDCONVERSION, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_BADACCESSOR, "got %08lx\n", dst_status);
    ok(dst_len == 0x1234, "got %Id\n", dst_len);
    ok(dst == 0x1234, "got %08x\n", dst);

    dst_len = dst = 0x1234;
    *(VARIANT_BOOL *)src = VARIANT_TRUE;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BOOL, DBTYPE_I2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == -1, "got %08x\n", dst);

    dst_len = dst = 0x1234;
    *(VARIANT_BOOL *)src = VARIANT_FALSE;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BOOL, DBTYPE_I2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0, "got %08x\n", dst);

    dst_len = dst = 0x1234;
    V_VT((VARIANT*)src) = VT_I2;
    V_I2((VARIANT*)src) = 0x4321;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_I2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x4321, "got %08x\n", dst);

    V_VT((VARIANT*)src) = VT_NULL;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_I2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_ISNULL, "got %08lx\n", dst_status);
    ok(dst_len == 0x1234, "got %Id\n", dst_len);

    dst_len = dst = 0x1234;
    memset(src, 0, sizeof(DECIMAL));
    ((DECIMAL*)src)->Lo64 = 0x4321;
    hr = IDataConvert_DataConvert(convert, DBTYPE_DECIMAL, DBTYPE_I2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x4321, "got %08x\n", dst);

    dst_len = dst = 0x1234;
    *(signed char*)src = 0xab;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I1, DBTYPE_I2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == (signed short)0xffab, "got %08x\n", dst);

    dst_len = dst = 0x1234;
    *(BYTE*)src = 0xab;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI1, DBTYPE_I2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0xab, "got %08x\n", dst);

    dst_len = dst = 0x1234;
    *(WORD*)src = 0x4321;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI2, DBTYPE_I2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x4321, "got %08x\n", dst);

    dst_len = dst = 0x1234;
    *(WORD*)src = 0xabcd;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI2, DBTYPE_I2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == DB_E_ERRORSOCCURRED, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_DATAOVERFLOW, "got %08lx\n", dst_status);
    todo_wine
    ok(broken(dst_len == sizeof(dst)) || dst_len == 0x1234 /* W2K+ */, "got %Id\n", dst_len);
    ok(dst == 0x1234, "got %08x\n", dst);

    dst_len = dst = 0x1234;
    *(DWORD*)src = 0xabcd1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI4, DBTYPE_I2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    todo_wine
    ok(hr == DB_E_DATAOVERFLOW, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_DATAOVERFLOW, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x1234, "got %08x\n", dst);

    dst_len = dst = 0x1234;
    *(DWORD*)src = 0x1234abcd;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI4, DBTYPE_I2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    todo_wine
    ok(hr == DB_E_DATAOVERFLOW, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_DATAOVERFLOW, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x1234, "got %08x\n", dst);

    dst_len = dst = 0x1234;
    *(DWORD*)src = 0x4321;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI4, DBTYPE_I2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x4321, "got %08x\n", dst);

    dst_len = dst = 0x1234;
    ((LARGE_INTEGER*)src)->QuadPart = 0x1234abcd;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I8, DBTYPE_I2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == DB_E_ERRORSOCCURRED ||
       broken(hr == DB_E_UNSUPPORTEDCONVERSION), /* win98 */
       "got %08lx\n", hr);
    if(hr != DB_E_UNSUPPORTEDCONVERSION) /* win98 doesn't support I8/UI8 */
    {
        ok(dst_status == DBSTATUS_E_DATAOVERFLOW, "got %08lx\n", dst_status);
        ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
        ok(dst == 0x1234, "got %08x\n", dst);

        dst_len = dst = 0x1234;
        ((LARGE_INTEGER*)src)->QuadPart = 0x4321;
        hr = IDataConvert_DataConvert(convert, DBTYPE_I8, DBTYPE_I2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
        ok(hr == S_OK, "got %08lx\n", hr);
        ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
        ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
        ok(dst == 0x4321, "got %08x\n", dst);

        dst_len = dst = 0x1234;
        ((ULARGE_INTEGER*)src)->QuadPart = 0x4321;
        hr = IDataConvert_DataConvert(convert, DBTYPE_UI8, DBTYPE_I2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
        ok(hr == S_OK, "got %08lx\n", hr);
        ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
        ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
        ok(dst == 0x4321, "got %08x\n", dst);
    }

    dst_len = dst = 0x1234;
    strcpy((char *)src, "10");
    hr = IDataConvert_DataConvert(convert, DBTYPE_STR, DBTYPE_I2, 2, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 10, "got %08x\n", dst);

    dst_len = dst = 0x1234;
    strcpy((char *)src, "10");
    hr = IDataConvert_DataConvert(convert, DBTYPE_STR, DBTYPE_I2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, DBDATACONVERT_LENGTHFROMNTS);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 10, "got %08x\n", dst);

    dst_len = dst = 0x1234;
    memcpy(src, ten, sizeof(ten));
    hr = IDataConvert_DataConvert(convert, DBTYPE_WSTR, DBTYPE_I2, 4, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 10, "got %08x\n", dst);

    dst_len = dst = 0x1234;
    memcpy(src, ten, sizeof(ten));
    hr = IDataConvert_DataConvert(convert, DBTYPE_WSTR, DBTYPE_I2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, DBDATACONVERT_LENGTHFROMNTS);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 10, "got %08x\n", dst);

    /* */
    dst_len = dst = 0x1234;
    *(WORD*)src = 0x4321;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI2, DBTYPE_UI2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x4321, "got %08x\n", dst);

    dst_len = dst = 0x1234;
    *(DWORD*)src = 0xabcd1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI4, DBTYPE_UI2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    todo_wine
    ok(hr == DB_E_DATAOVERFLOW, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_DATAOVERFLOW, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x1234, "got %08x\n", dst);

    dst_len = dst = 0x1234;
    *(DWORD*)src = 0x1234abcd;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI4, DBTYPE_UI2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    todo_wine
    ok(hr == DB_E_DATAOVERFLOW, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_DATAOVERFLOW, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x1234, "got %08x\n", dst);

    dst_len = dst = 0x1234;
    *(DWORD*)src = 0x4321;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI4, DBTYPE_UI2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x4321, "got %08x\n", dst);

    dst_len = dst = 0x1234;
    memcpy(src, ten, sizeof(ten));
    hr = IDataConvert_DataConvert(convert, DBTYPE_WSTR, DBTYPE_UI2, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, DBDATACONVERT_LENGTHFROMNTS);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 10, "got %08x\n", dst);
}

static void test_converttoi4(void)
{
    HRESULT hr;
    INT i4;
    BYTE src[sizeof(VARIANT)];  /* assuming that VARIANT is larger than all the types used in src */
    DBSTATUS dst_status;
    DBLENGTH dst_len;
    static const WCHAR ten[] = {'1','0',0};
    BSTR b;

    i4 = 0x12345678;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_EMPTY, DBTYPE_I4, 0, &dst_len, src, &i4, sizeof(i4), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(i4), "got %Id\n", dst_len);
    ok(i4 == 0, "got %08x\n", i4);

    i4 = 0x12345678;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_NULL, DBTYPE_I4, 0, &dst_len, src, &i4, sizeof(i4), 0, &dst_status, 0, 0, 0);
    ok(hr == DB_E_UNSUPPORTEDCONVERSION, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_BADACCESSOR, "got %08lx\n", dst_status);
    ok(dst_len == 0x1234, "got %Id\n", dst_len);
    ok(i4 == 0x12345678, "got %08x\n", i4);

    i4 = 0x12345678;
    *(short *)src = 0x4321;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I2, DBTYPE_I4, 0, &dst_len, src, &i4, sizeof(i4), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(i4), "got %Id\n", dst_len);
    ok(i4 == 0x4321, "got %08x\n", i4);

    i4 = 0x12345678;
    *(int *)src = 0x4321cafe;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I4, DBTYPE_I4, 0, &dst_len, src, &i4, sizeof(i4), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(i4), "got %Id\n", dst_len);
    ok(i4 == 0x4321cafe, "got %08x\n", i4);

    i4 = 0x12345678;
    *(FLOAT *)src = 10.75;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R4, DBTYPE_I4, 0, &dst_len, src, &i4, sizeof(i4), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(i4), "got %Id\n", dst_len);
    ok(i4 == 11, "got %08x\n", i4);

    i4 = 0x12345678;
    *(FLOAT *)src = -10.75;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R4, DBTYPE_I4, 0, &dst_len, src, &i4, sizeof(i4), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(i4), "got %Id\n", dst_len);
    ok(i4 == -11, "got %08x\n", i4);

    i4 = 0x12345678;
    *(double *)src = 10.75;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R8, DBTYPE_I4, 0, &dst_len, src, &i4, sizeof(i4), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(i4), "got %Id\n", dst_len);
    ok(i4 == 11, "got %08x\n", i4);

    i4 = 0x12345678;
    ((LARGE_INTEGER *)src)->QuadPart = 107500;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_CY, DBTYPE_I4, 0, &dst_len, src, &i4, sizeof(i4), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(i4), "got %Id\n", dst_len);
    ok(i4 == 11, "got %08x\n", i4);

    i4 = 0x12345678;
    *(DATE *)src = 10.7500;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_DATE, DBTYPE_I4, 0, &dst_len, src, &i4, sizeof(i4), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(i4), "got %Id\n", dst_len);
    ok(i4 == 11, "got %08x\n", i4);

    i4 = 0x12345678;
    b = SysAllocString(ten);
    *(BSTR *)src = b;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BSTR, DBTYPE_I4, 0, &dst_len, src, &i4, sizeof(i4), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(i4), "got %Id\n", dst_len);
    ok(i4 == 10, "got %08x\n", i4);
    SysFreeString(b);

    i4 = 0x12345678;
    *(SCODE *)src = 0x4321cafe;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_ERROR, DBTYPE_I4, 0, &dst_len, src, &i4, sizeof(i4), 0, &dst_status, 0, 0, 0);
    ok(hr == DB_E_UNSUPPORTEDCONVERSION, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_BADACCESSOR, "got %08lx\n", dst_status);
    ok(dst_len == 0x1234, "got %Id\n", dst_len);
    ok(i4 == 0x12345678, "got %08x\n", i4);

    i4 = 0x12345678;
    *(VARIANT_BOOL *)src = VARIANT_TRUE;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BOOL, DBTYPE_I4, 0, &dst_len, src, &i4, sizeof(i4), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(i4), "got %Id\n", dst_len);
    ok(i4 == 0xffffffff, "got %08x\n", i4);

    i4 = 0x12345678;
    *(VARIANT_BOOL *)src = VARIANT_FALSE;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BOOL, DBTYPE_I4, 0, &dst_len, src, &i4, sizeof(i4), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(i4), "got %Id\n", dst_len);
    ok(i4 == 0, "got %08x\n", i4);

    i4 = 0x12345678;
    V_VT((VARIANT*)src) = VT_I2;
    V_I2((VARIANT*)src) = 0x1234;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_I4, 0, &dst_len, src, &i4, sizeof(i4), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(i4), "got %Id\n", dst_len);
    ok(i4 == 0x1234, "got %08x\n", i4);

    V_VT((VARIANT*)src) = VT_NULL;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_I4, 0, &dst_len, src, &i4, sizeof(i4), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_ISNULL, "got %08lx\n", dst_status);
    ok(dst_len == 0x1234, "got %Id\n", dst_len);

    i4 = 0x12345678;
    memset(src, 0, sizeof(DECIMAL));
    ((DECIMAL*)src)->Lo64 = 0x1234;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_DECIMAL, DBTYPE_I4, 0, &dst_len, src, &i4, sizeof(i4), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(i4), "got %Id\n", dst_len);
    ok(i4 == 0x1234, "got %08x\n", i4);

    i4 = 0x12345678;
    *(signed char*)src = 0xab;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I1, DBTYPE_I4, 0, &dst_len, src, &i4, sizeof(i4), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(i4), "got %Id\n", dst_len);
    ok(i4 == 0xffffffab, "got %08x\n", i4);

    i4 = 0x12345678;
    *(BYTE*)src = 0xab;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI1, DBTYPE_I4, 0, &dst_len, src, &i4, sizeof(i4), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(i4), "got %Id\n", dst_len);
    ok(i4 == 0xab, "got %08x\n", i4);

    i4 = 0x12345678;
    *(WORD*)src = 0xabcd;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI2, DBTYPE_I4, 0, &dst_len, src, &i4, sizeof(i4), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(i4), "got %Id\n", dst_len);
    ok(i4 == 0xabcd, "got %08x\n", i4);

    i4 = 0x12345678;
    *(DWORD*)src = 0xabcd1234;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI4, DBTYPE_I4, 0, &dst_len, src, &i4, sizeof(i4), 0, &dst_status, 0, 0, 0);
    ok(hr == DB_E_ERRORSOCCURRED, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_DATAOVERFLOW, "got %08lx\n", dst_status);
    todo_wine
    ok(broken(dst_len == sizeof(i4)) || dst_len == 0x1234 /* W2K+ */, "got %Id\n", dst_len);
    ok(i4 == 0x12345678, "got %08x\n", i4);

    i4 = 0x12345678;
    *(DWORD*)src = 0x1234abcd;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI4, DBTYPE_I4, 0, &dst_len, src, &i4, sizeof(i4), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(i4), "got %Id\n", dst_len);
    ok(i4 == 0x1234abcd, "got %08x\n", i4);

    i4 = 0x12345678;
    ((LARGE_INTEGER*)src)->QuadPart = 0x1234abcd;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I8, DBTYPE_I4, 0, &dst_len, src, &i4, sizeof(i4), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK ||
       broken(hr == DB_E_UNSUPPORTEDCONVERSION), /* win98 */
       "got %08lx\n", hr);
    if(hr != DB_E_UNSUPPORTEDCONVERSION) /* win98 doesn't support I8/UI8 */
    {
        ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
        ok(dst_len == sizeof(i4), "got %Id\n", dst_len);
        ok(i4 == 0x1234abcd, "got %08x\n", i4);

        i4 = 0x12345678;
        ((ULARGE_INTEGER*)src)->QuadPart = 0x1234abcd;
        dst_len = 0x1234;
        hr = IDataConvert_DataConvert(convert, DBTYPE_UI8, DBTYPE_I4, 0, &dst_len, src, &i4, sizeof(i4), 0, &dst_status, 0, 0, 0);
        ok(hr == S_OK, "got %08lx\n", hr);
        ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
        ok(dst_len == sizeof(i4), "got %Id\n", dst_len);
        ok(i4 == 0x1234abcd, "got %08x\n", i4);
    }

    i4 = 0x12345678;
    strcpy((char *)src, "10");
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_STR, DBTYPE_I4, 2, &dst_len, src, &i4, sizeof(i4), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(i4), "got %Id\n", dst_len);
    ok(i4 == 10, "got %08x\n", i4);

    i4 = 0x12345678;
    strcpy((char *)src, "10");
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_STR, DBTYPE_I4, 0, &dst_len, src, &i4, sizeof(i4), 0, &dst_status, 0, 0, DBDATACONVERT_LENGTHFROMNTS);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(i4), "got %Id\n", dst_len);
    ok(i4 == 10, "got %08x\n", i4);

    i4 = 0x12345678;
    memcpy(src, ten, sizeof(ten));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_WSTR, DBTYPE_I4, 4, &dst_len, src, &i4, sizeof(i4), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(i4), "got %Id\n", dst_len);
    ok(i4 == 10, "got %08x\n", i4);

    i4 = 0x12345678;
    memcpy(src, ten, sizeof(ten));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_WSTR, DBTYPE_I4, 0, &dst_len, src, &i4, sizeof(i4), 0, &dst_status, 0, 0, DBDATACONVERT_LENGTHFROMNTS);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(i4), "got %Id\n", dst_len);
    ok(i4 == 10, "got %08x\n", i4);

    /* src_status = DBSTATUS_S_ISNULL */
    i4 = 0x12345678;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I4, DBTYPE_I4, 0, &dst_len, src, &i4, sizeof(i4), DBSTATUS_S_ISNULL, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_ISNULL, "got %08lx\n", dst_status);
    ok(dst_len == 0, "got %Id\n", dst_len);

    /* dst = NULL */
    *(int *)src = 0x4321cafe;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I4, DBTYPE_I4, 0, &dst_len, src, NULL, 0, 0, NULL, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_len == sizeof(i4), "got %Id\n", dst_len);

}

static void test_converttoi8(void)
{
    HRESULT hr;
    LARGE_INTEGER dst;
    BYTE src[sizeof(VARIANT)];  /* assuming that VARIANT is larger than all the types used in src */
    DBSTATUS dst_status;
    DBLENGTH dst_len;
    static const WCHAR ten[] = {'1','0',0};
    BSTR b;

    dst.QuadPart = 0xcc;
    ((ULARGE_INTEGER*)src)->QuadPart = 1234;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I8, DBTYPE_I8, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst.QuadPart == 1234, "got %d\n", (int)dst.QuadPart);

    dst.QuadPart = 0xcc;
    ((ULARGE_INTEGER*)src)->QuadPart = 1234;
    b = SysAllocString(ten);
    *(BSTR *)src = b;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BSTR, DBTYPE_I8, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst.QuadPart == 10, "got %d\n", (int)dst.QuadPart);
    SysFreeString(b);

    V_VT((VARIANT*)src) = VT_NULL;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_I8, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_ISNULL, "got %08lx\n", dst_status);
    ok(dst_len == 0x1234, "got %Id\n", dst_len);
}

static void test_converttobstr(void)
{
    HRESULT hr;
    BSTR dst;
    BYTE src[20];
    DBSTATUS dst_status;
    DBLENGTH dst_len;
    static const WCHAR ten[] = {'1','0',0};
    static const WCHAR tsW[] = {'2','0','1','3','-','0','5','-','1','4',' ','0','2',':','0','4',':','1','2',0};
    static const WCHAR ts1W[] = {'2','0','1','3','-','0','5','-','1','4',' ','0','2',':','0','4',':','1','2','.','0','0','0','0','0','0','0','0','3',0};
    static const WCHAR ts2W[] = {'2','0','1','3','-','0','5','-','1','4',' ','0','2',':','0','4',':','1','2','.','0','0','0','0','0','0','2','0','0',0};
    DBTIMESTAMP ts = {2013, 5, 14, 2, 4, 12, 0};
    VARIANT v;
    BSTR b;

    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_EMPTY, DBTYPE_BSTR, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst != NULL, "got %p\n", dst);
    ok(SysStringLen(dst) == 0, "got %d\n", SysStringLen(dst));
    SysFreeString(dst);

    dst = (void*)0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_NULL, DBTYPE_BSTR, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == DB_E_UNSUPPORTEDCONVERSION, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_BADACCESSOR, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == (void*)0x1234, "got %p\n", dst);

    *(short *)src = 4321;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I2, DBTYPE_BSTR, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst != NULL, "got %p\n", dst);
    ok(SysStringLen(dst) == 4, "got %d\n", SysStringLen(dst));
    SysFreeString(dst);

    b = SysAllocString(ten);
    *(BSTR *)src = b;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BSTR, DBTYPE_BSTR, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst != NULL, "got %p\n", dst);
    ok(dst != b, "got %p src %p\n", dst, b);
    ok(!lstrcmpW(b, dst), "got %s\n", wine_dbgstr_w(dst));
    SysFreeString(dst);
    SysFreeString(b);

    b = SysAllocString(ten);
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = b;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_BSTR, 0, &dst_len, &v, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst != NULL, "got %p\n", dst);
    ok(dst != b, "got %p src %p\n", dst, b);
    ok(!lstrcmpW(b, dst), "got %s\n", wine_dbgstr_w(dst));
    SysFreeString(dst);
    SysFreeString(b);

    V_VT(&v) = VT_NULL;
    dst = (void*)0x1234;
    dst_len = 33;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_BSTR, 0, &dst_len, &v, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_ISNULL, "got %08lx\n", dst_status);
    ok(dst_len == 33, "got %Id\n", dst_len);
    ok(dst == (void*)0x1234, "got %p\n", dst);

    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_DBTIMESTAMP, DBTYPE_BSTR, 0, &dst_len, &ts, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(BSTR), "got %Id\n", dst_len);
    ok(!lstrcmpW(tsW, dst), "got %s\n", wine_dbgstr_w(dst));
    SysFreeString(dst);

    dst_len = 0x1234;
    ts.fraction = 3;
    hr = IDataConvert_DataConvert(convert, DBTYPE_DBTIMESTAMP, DBTYPE_BSTR, 0, &dst_len, &ts, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(BSTR), "got %Id\n", dst_len);
    ok(!lstrcmpW(ts1W, dst), "got %s\n", wine_dbgstr_w(dst));
    SysFreeString(dst);

    dst_len = 0x1234;
    ts.fraction = 200;
    hr = IDataConvert_DataConvert(convert, DBTYPE_DBTIMESTAMP, DBTYPE_BSTR, 0, &dst_len, &ts, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(BSTR), "got %Id\n", dst_len);
    ok(!lstrcmpW(ts2W, dst), "got %s\n", wine_dbgstr_w(dst));
    SysFreeString(dst);
}

static void test_converttowstr(void)
{
    HRESULT hr;
    WCHAR dst[100];
    BYTE src[20];
    DBSTATUS dst_status;
    DBLENGTH dst_len;
    static const WCHAR ten[] = {'1','0',0};
    static const WCHAR fourthreetwoone[] = {'4','3','2','1',0};
    static const WCHAR guid_str[] = {
        '{','0','C','7','3','3','A','8','D','-','2','A','1','C','-','1','1','C','E','-',
        'A','D','E','5','-','0','0','A','A','0','0','4','4','7','7','3','D','}',0};
    static const WCHAR hexunpacked_w[] = {'5','7','0','0','6','9','0','0','6','E','0','0','6','5','0','0','0','0','0','0', 0 };
    static const WCHAR hexpacked_w[] = {'W','i','n','e', 0 };
    static const WCHAR tsW[] = {'2','0','1','3','-','0','5','-','1','4',' ','0','2',':','0','4',':','1','2',0};
    static const WCHAR ts1W[] = {'2','0','1','3','-','0','5','-','1','4',' ','0','2',':','0','4',':','1','2','.','0','0','0','0','0','0','0','0','3',0};
    static const WCHAR ts2W[] = {'2','0','1','3','-','0','5','-','1','4',' ','0','2',':','0','4',':','1','2','.','0','0','0','0','0','0','2','0','0',0};
    DBTIMESTAMP ts = {2013, 5, 14, 2, 4, 12, 0};
    BSTR b;
    VARIANT v;

    VariantInit(&v);

    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_EMPTY, DBTYPE_WSTR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 0, "got %Id\n", dst_len);
    ok(dst[0] == 0, "got %02x\n", dst[0]);
    ok(dst[1] == 0xcccc, "got %02x\n", dst[1]);

    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_NULL, DBTYPE_WSTR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == DB_E_UNSUPPORTEDCONVERSION, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_BADACCESSOR, "got %08lx\n", dst_status);
    ok(dst_len == 0x1234, "got %Id\n", dst_len);
    ok(dst[0] == 0xcccc, "got %02x\n", dst[0]);

    *(short *)src = 4321;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I2, DBTYPE_WSTR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 8, "got %Id\n", dst_len);
    ok(!lstrcmpW(dst, fourthreetwoone), "got %s\n", wine_dbgstr_w(dst));

    *(short *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I2, DBTYPE_WSTR, 0, &dst_len, src, dst, 0, 0, &dst_status, 0, 0, 0);
    ok(hr == DB_E_ERRORSOCCURRED, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_DATAOVERFLOW, "got %08lx\n", dst_status);
    ok(dst_len == 8, "got %Id\n", dst_len);
    ok(dst[0] == 0xcccc, "got %02x\n", dst[0]);

    *(short *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I2, DBTYPE_WSTR, 0, &dst_len, src, NULL, 0, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 8, "got %Id\n", dst_len);
    ok(dst[0] == 0xcccc, "got %02x\n", dst[0]);

    *(short *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I2, DBTYPE_WSTR, 0, &dst_len, src, dst, 4, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_TRUNCATED, "got %08lx\n", dst_status);
    ok(dst_len == 8, "got %Id\n", dst_len);
    ok(dst[0] == '4', "got %02x\n", dst[0]);
    ok(dst[1] == 0, "got %02x\n", dst[1]);
    ok(dst[2] == 0xcccc, "got %02x\n", dst[2]);

    *(short *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I2, DBTYPE_WSTR, 0, &dst_len, src, dst, 2, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_TRUNCATED, "got %08lx\n", dst_status);
    ok(dst_len == 8, "got %Id\n", dst_len);
    ok(dst[0] == 0, "got %02x\n", dst[0]);
    ok(dst[1] == 0xcccc, "got %02x\n", dst[1]);

    *(short *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I2, DBTYPE_WSTR, 0, &dst_len, src, dst, 8, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_TRUNCATED, "got %08lx\n", dst_status);
    ok(dst_len == 8, "got %Id\n", dst_len);
    ok(dst[0] == '4', "got %02x\n", dst[0]);
    ok(dst[1] == '3', "got %02x\n", dst[1]);
    ok(dst[2] == '2', "got %02x\n", dst[2]);
    ok(dst[3] == 0, "got %02x\n", dst[3]);
    ok(dst[4] == 0xcccc, "got %02x\n", dst[4]);



    *(int *)src = 4321;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I4, DBTYPE_WSTR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 8, "got %Id\n", dst_len);
    ok(!lstrcmpW(dst, fourthreetwoone), "got %s\n", wine_dbgstr_w(dst));

    *(int *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I4, DBTYPE_WSTR, 0, &dst_len, src, dst, 0, 0, &dst_status, 0, 0, 0);
    ok(hr == DB_E_ERRORSOCCURRED, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_DATAOVERFLOW, "got %08lx\n", dst_status);
    ok(dst_len == 8, "got %Id\n", dst_len);
    ok(dst[0] == 0xcccc, "got %02x\n", dst[0]);

    *(int *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    hr = IDataConvert_DataConvert(convert, DBTYPE_I4, DBTYPE_WSTR, 0, &dst_len, src, NULL, 0, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 8, "got %Id\n", dst_len);
    ok(dst[0] == 0xcccc, "got %02x\n", dst[0]);

    *(int *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I4, DBTYPE_WSTR, 0, &dst_len, src, dst, 4, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_TRUNCATED, "got %08lx\n", dst_status);
    ok(dst_len == 8, "got %Id\n", dst_len);
    ok(dst[0] == '4', "got %02x\n", dst[0]);
    ok(dst[1] == 0, "got %02x\n", dst[1]);
    ok(dst[2] == 0xcccc, "got %02x\n", dst[2]);

    *(int *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I4, DBTYPE_WSTR, 0, &dst_len, src, dst, 2, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_TRUNCATED, "got %08lx\n", dst_status);
    ok(dst_len == 8, "got %Id\n", dst_len);
    ok(dst[0] == 0, "got %02x\n", dst[0]);
    ok(dst[1] == 0xcccc, "got %02x\n", dst[1]);

    *(int *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I4, DBTYPE_WSTR, 0, &dst_len, src, dst, 8, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_TRUNCATED, "got %08lx\n", dst_status);
    ok(dst_len == 8, "got %Id\n", dst_len);
    ok(dst[0] == '4', "got %02x\n", dst[0]);
    ok(dst[1] == '3', "got %02x\n", dst[1]);
    ok(dst[2] == '2', "got %02x\n", dst[2]);
    ok(dst[3] == 0, "got %02x\n", dst[3]);
    ok(dst[4] == 0xcccc, "got %02x\n", dst[4]);



    *(float *)src = 4321;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R4, DBTYPE_WSTR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 8, "got %Id\n", dst_len);
    ok(!lstrcmpW(dst, fourthreetwoone), "got %s\n", wine_dbgstr_w(dst));

    *(float *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R4, DBTYPE_WSTR, 0, &dst_len, src, dst, 0, 0, &dst_status, 0, 0, 0);
    ok(hr == DB_E_ERRORSOCCURRED, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_DATAOVERFLOW, "got %08lx\n", dst_status);
    ok(dst_len == 8, "got %Id\n", dst_len);
    ok(dst[0] == 0xcccc, "got %02x\n", dst[0]);

    *(float *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    hr = IDataConvert_DataConvert(convert, DBTYPE_R4, DBTYPE_WSTR, 0, &dst_len, src, NULL, 0, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 8, "got %Id\n", dst_len);
    ok(dst[0] == 0xcccc, "got %02x\n", dst[0]);

    *(float *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R4, DBTYPE_WSTR, 0, &dst_len, src, dst, 4, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_TRUNCATED, "got %08lx\n", dst_status);
    ok(dst_len == 8, "got %Id\n", dst_len);
    ok(dst[0] == '4', "got %02x\n", dst[0]);
    ok(dst[1] == 0, "got %02x\n", dst[1]);
    ok(dst[2] == 0xcccc, "got %02x\n", dst[2]);

    *(float *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R4, DBTYPE_WSTR, 0, &dst_len, src, dst, 2, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_TRUNCATED, "got %08lx\n", dst_status);
    ok(dst_len == 8, "got %Id\n", dst_len);
    ok(dst[0] == 0, "got %02x\n", dst[0]);
    ok(dst[1] == 0xcccc, "got %02x\n", dst[1]);

    *(float *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R4, DBTYPE_WSTR, 0, &dst_len, src, dst, 8, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_TRUNCATED, "got %08lx\n", dst_status);
    ok(dst_len == 8, "got %Id\n", dst_len);
    ok(dst[0] == '4', "got %02x\n", dst[0]);
    ok(dst[1] == '3', "got %02x\n", dst[1]);
    ok(dst[2] == '2', "got %02x\n", dst[2]);
    ok(dst[3] == 0, "got %02x\n", dst[3]);
    ok(dst[4] == 0xcccc, "got %02x\n", dst[4]);



    *(double *)src = 4321;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R8, DBTYPE_WSTR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 8, "got %Id\n", dst_len);
    ok(!lstrcmpW(dst, fourthreetwoone), "got %s\n", wine_dbgstr_w(dst));

    *(double *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    hr = IDataConvert_DataConvert(convert, DBTYPE_R8, DBTYPE_WSTR, 0, &dst_len, src, dst, 0, 0, &dst_status, 0, 0, 0);
    ok(hr == DB_E_ERRORSOCCURRED, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_DATAOVERFLOW, "got %08lx\n", dst_status);
    ok(dst_len == 8, "got %Id\n", dst_len);
    ok(dst[0] == 0xcccc, "got %02x\n", dst[0]);

    *(double *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R8, DBTYPE_WSTR, 0, &dst_len, src, NULL, 0, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 8, "got %Id\n", dst_len);
    ok(dst[0] == 0xcccc, "got %02x\n", dst[0]);

    *(double *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R8, DBTYPE_WSTR, 0, &dst_len, src, dst, 4, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_TRUNCATED, "got %08lx\n", dst_status);
    ok(dst_len == 8, "got %Id\n", dst_len);
    ok(dst[0] == '4', "got %02x\n", dst[0]);
    ok(dst[1] == 0, "got %02x\n", dst[1]);
    ok(dst[2] == 0xcccc, "got %02x\n", dst[2]);

    *(double *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R8, DBTYPE_WSTR, 0, &dst_len, src, dst, 2, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_TRUNCATED, "got %08lx\n", dst_status);
    ok(dst_len == 8, "got %Id\n", dst_len);
    ok(dst[0] == 0, "got %02x\n", dst[0]);
    ok(dst[1] == 0xcccc, "got %02x\n", dst[1]);

    *(double *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R8, DBTYPE_WSTR, 0, &dst_len, src, dst, 8, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_TRUNCATED, "got %08lx\n", dst_status);
    ok(dst_len == 8, "got %Id\n", dst_len);
    ok(dst[0] == '4', "got %02x\n", dst[0]);
    ok(dst[1] == '3', "got %02x\n", dst[1]);
    ok(dst[2] == '2', "got %02x\n", dst[2]);
    ok(dst[3] == 0, "got %02x\n", dst[3]);
    ok(dst[4] == 0xcccc, "got %02x\n", dst[4]);



    memset(src, 0, sizeof(src));
    ((CY*)src)->int64 = 43210000;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_CY, DBTYPE_WSTR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 8, "got %Id\n", dst_len);
    ok(!lstrcmpW(dst, fourthreetwoone), "got %s\n", wine_dbgstr_w(dst));



    memset(src, 0, sizeof(src));
    *(signed char *)src = 10;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I1, DBTYPE_WSTR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(!lstrcmpW(dst, ten), "got %s\n", wine_dbgstr_w(dst));

    memset(src, 0, sizeof(src));
    *(unsigned char *)src = 10;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI1, DBTYPE_WSTR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(!lstrcmpW(dst, ten), "got %s\n", wine_dbgstr_w(dst));

    memset(src, 0, sizeof(src));
    *(unsigned short *)src = 4321;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI2, DBTYPE_WSTR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 8, "got %Id\n", dst_len);
    ok(!lstrcmpW(dst, fourthreetwoone), "got %s\n", wine_dbgstr_w(dst));

    memset(src, 0, sizeof(src));
    *(unsigned int *)src = 4321;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI4, DBTYPE_WSTR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 8, "got %Id\n", dst_len);
    ok(!lstrcmpW(dst, fourthreetwoone), "got %s\n", wine_dbgstr_w(dst));

    memset(src, 0, sizeof(src));
    ((LARGE_INTEGER*)src)->QuadPart = 4321;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I8, DBTYPE_WSTR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(broken(hr == DB_E_UNSUPPORTEDCONVERSION) || hr == S_OK /* W2K+ */, "got %08lx\n", hr);
    ok(broken(dst_status == DBSTATUS_E_BADACCESSOR) || dst_status == DBSTATUS_S_OK /* W2K+ */, "got %08lx\n", dst_status);
    ok(broken(dst_len == 0x1234) || dst_len == 8 /* W2K+ */, "got %Id\n", dst_len);
    ok(!lstrcmpW(dst, fourthreetwoone), "got %s\n", wine_dbgstr_w(dst));

    memset(src, 0, sizeof(src));
    ((ULARGE_INTEGER*)src)->QuadPart = 4321;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI8, DBTYPE_WSTR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(broken(hr == DB_E_UNSUPPORTEDCONVERSION) || hr == S_OK /* W2K+ */, "got %08lx\n", hr);
    ok(broken(dst_status == DBSTATUS_E_BADACCESSOR) || dst_status == DBSTATUS_S_OK /* W2K+ */, "got %08lx\n", dst_status);
    ok(broken(dst_len == 0x1234) || dst_len == 8 /* W2K+ */, "got %Id\n", dst_len);
    ok(!lstrcmpW(dst, fourthreetwoone), "got %s\n", wine_dbgstr_w(dst));



    memset(src, 0, sizeof(src));
    memcpy(src, &IID_IDataConvert, sizeof(GUID));
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_GUID, DBTYPE_WSTR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 76, "got %Id\n", dst_len);
    ok(!lstrcmpW(dst, guid_str), "got %s\n", wine_dbgstr_w(dst));



    b = SysAllocString(ten);
    *(BSTR *)src = b;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BSTR, DBTYPE_WSTR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(!lstrcmpW(b, dst), "got %s\n", wine_dbgstr_w(dst));
    SysFreeString(b);

    memcpy(src, ten, sizeof(ten));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_WSTR, DBTYPE_WSTR, 2, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 2, "got %Id\n", dst_len);
    ok(dst[0] == '1', "got %02x\n", dst[0]);
    ok(dst[1] == 0, "got %02x\n", dst[1]);

    memcpy(src, ten, sizeof(ten));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_WSTR, DBTYPE_WSTR, 4, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(!lstrcmpW(ten, dst), "got %s\n", wine_dbgstr_w(dst));

    memcpy(src, ten, sizeof(ten));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_WSTR, DBTYPE_WSTR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, DBDATACONVERT_LENGTHFROMNTS);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(!lstrcmpW(ten, dst), "got %s\n", wine_dbgstr_w(dst));

    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_DBTIMESTAMP, DBTYPE_WSTR, 0, &dst_len, &ts, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 38, "got %Id\n", dst_len);
    ok(!lstrcmpW(tsW, dst), "got %s\n", wine_dbgstr_w(dst));

    dst_len = 0x1234;
    ts.fraction = 3;
    hr = IDataConvert_DataConvert(convert, DBTYPE_DBTIMESTAMP, DBTYPE_WSTR, 0, &dst_len, &ts, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 58, "got %Id\n", dst_len);
    ok(!lstrcmpW(ts1W, dst), "got %s\n", wine_dbgstr_w(dst));

    dst_len = 0x1234;
    ts.fraction = 200;
    hr = IDataConvert_DataConvert(convert, DBTYPE_DBTIMESTAMP, DBTYPE_WSTR, 0, &dst_len, &ts, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 58, "got %Id\n", dst_len);
    ok(!lstrcmpW(ts2W, dst), "got %s\n", wine_dbgstr_w(dst));

    /* DBTYPE_BYTES to DBTYPE_*STR unpacks binary data into a hex string */
    memcpy(src, hexpacked_w, sizeof(hexpacked_w));
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BYTES, DBTYPE_WSTR, sizeof(hexpacked_w), &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(hexpacked_w) * 4, "got %Id\n", dst_len);
    ok(!lstrcmpW(hexunpacked_w, dst), "got %s\n", wine_dbgstr_w(dst));
    ok(dst[ARRAY_SIZE(hexpacked_w) * 4 + 1] == 0xcccc, "clobbered buffer\n");

    memcpy(src, hexpacked_w, sizeof(hexpacked_w));
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BYTES, DBTYPE_WSTR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 0, "got %Id\n", dst_len);
    ok(dst[0] == 0, "not null terminated\n");
    ok(dst[1] == 0xcccc, "clobbered buffer\n");

    memcpy(src, hexpacked_w, sizeof(hexpacked_w));
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BYTES, DBTYPE_WSTR, 4, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 2 * sizeof(WCHAR) * 4, "got %Id\n", dst_len);
    ok(!memcmp(hexunpacked_w, dst, 2 * sizeof(WCHAR) * 4 ), "got %s\n", wine_dbgstr_w(dst));
    ok(dst[2 * 4] == 0, "not null terminated\n");
    ok(dst[2 * 4 + 1] == 0xcccc, "clobbered buffer\n");

    memcpy(src, hexpacked_w, sizeof(hexpacked_w));
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BYTES, DBTYPE_WSTR, sizeof(hexpacked_w), &dst_len, src, dst, 2 * sizeof(WCHAR) * 4 + sizeof(WCHAR), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_TRUNCATED, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(hexpacked_w) * 4, "got %Id\n", dst_len);
    ok(!memcmp(hexunpacked_w, dst, 2 * sizeof(WCHAR) * 4 ), "got %s\n", wine_dbgstr_w(dst));
    ok(dst[2 * 4] == 0, "not null terminated\n");
    ok(dst[2 * 4 + 1] == 0xcccc, "clobbered buffer\n");

    memcpy(src, hexpacked_w, sizeof(hexpacked_w));
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BYTES, DBTYPE_WSTR, sizeof(hexpacked_w), &dst_len, src, dst, 2 * sizeof(WCHAR) * 4 +1, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_TRUNCATED, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(hexpacked_w) * 4, "got %Id\n", dst_len);
    ok(!memcmp(hexunpacked_w, dst, 2 * sizeof(WCHAR) * 4 - 2 ), "got %s\n", wine_dbgstr_w(dst));
    ok(dst[2 * 4 - 1] == 0, "not null terminated\n");
    ok(dst[2 * 4] == 0xcccc, "clobbered buffer\n");

    memcpy(src, hexpacked_w, sizeof(hexpacked_w));
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BYTES, DBTYPE_WSTR, sizeof(hexpacked_w), &dst_len, src, dst, 2 * sizeof(WCHAR) * 4, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_TRUNCATED, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(hexpacked_w) * 4, "got %Id\n", dst_len);
    ok(!memcmp(hexunpacked_w, dst, 2 * sizeof(WCHAR) * 4 - 2 ), "got %s\n", wine_dbgstr_w(dst));
    ok(dst[2 * 4 - 1] == 0, "not null terminated\n");
    ok(dst[2 * 4] == 0xcccc, "clobbered buffer\n");

    memcpy(src, hexpacked_w, sizeof(hexpacked_w));
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BYTES, DBTYPE_WSTR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, DBDATACONVERT_LENGTHFROMNTS);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 0, "got %Id\n", dst_len);
    ok(dst[0] == 0, "not null terminated\n");
    ok(dst[1] == 0xcccc, "clobbered buffer\n");

    b = SysAllocStringLen(NULL, 0);
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = b;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_WSTR, 0, &dst_len, &v, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 0, "got %Id\n", dst_len);
    ok(!lstrcmpW(b, dst), "got %s\n", wine_dbgstr_w(dst));
    VariantClear(&v);

    dst_len = 44;
    V_VT(&v) = VT_NULL;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_WSTR, 0, &dst_len, &v, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_ISNULL, "got %08lx\n", dst_status);
    ok(dst_len == 44, "got %Id\n", dst_len);
}

static void test_converttostr(void)
{
    HRESULT hr;
    char dst[100];
    BYTE src[64];
    DBSTATUS dst_status;
    DBLENGTH dst_len;
    static const WCHAR ten[] = {'1','0',0};
    static const WCHAR idW[] = {'0','C','7','3','3','A','8','D','-','2','A','1','C',0 };
    static const char idA[] = "0C733A8D";
    static const char withnull[] = "test\0ed";
    static const char ten_a[] = "10";
    static const char fourthreetwoone[] = "4321";
    static const char guid_str[] = "{0C733A8D-2A1C-11CE-ADE5-00AA0044773D}";
    static const char hexunpacked_a[] = "57696E6500";
    static const char hexpacked_a[] = "Wine";
    BSTR b;
    VARIANT v;

    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_EMPTY, DBTYPE_STR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 0, "got %Id\n", dst_len);
    ok(dst[0] == 0, "got %02x\n", dst[0]);
    ok(dst[1] == (char)0xcc, "got %02x\n", dst[1]);

    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_NULL, DBTYPE_STR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == DB_E_UNSUPPORTEDCONVERSION, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_BADACCESSOR, "got %08lx\n", dst_status);
    ok(dst_len == 0x1234, "got %Id\n", dst_len);
    ok(dst[0] == (char)0xcc, "got %02x\n", dst[0]);

    *(short *)src = 4321;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I2, DBTYPE_STR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(!lstrcmpA(dst, fourthreetwoone), "got %s\n", dst);

    *(short *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I2, DBTYPE_STR, 0, &dst_len, src, dst, 0, 0, &dst_status, 0, 0, 0);
    ok(hr == DB_E_ERRORSOCCURRED, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_DATAOVERFLOW, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(dst[0] == (char)0xcc, "got %02x\n", dst[0]);

    *(short *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I2, DBTYPE_STR, 0, &dst_len, src, NULL, 0, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(dst[0] == (char)0xcc, "got %02x\n", dst[0]);

    *(short *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I2, DBTYPE_STR, 0, &dst_len, src, dst, 2, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_TRUNCATED, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(dst[0] == '4', "got %02x\n", dst[0]);
    ok(dst[1] == 0, "got %02x\n", dst[1]);
    ok(dst[2] == (char)0xcc, "got %02x\n", dst[2]);

    *(short *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I2, DBTYPE_STR, 0, &dst_len, src, dst, 1, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_TRUNCATED, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(dst[0] == 0, "got %02x\n", dst[0]);
    ok(dst[1] == (char)0xcc, "got %02x\n", dst[1]);

    *(short *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I2, DBTYPE_STR, 0, &dst_len, src, dst, 4, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_TRUNCATED, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(dst[0] == '4', "got %02x\n", dst[0]);
    ok(dst[1] == '3', "got %02x\n", dst[1]);
    ok(dst[2] == '2', "got %02x\n", dst[2]);
    ok(dst[3] == 0, "got %02x\n", dst[3]);
    ok(dst[4] == (char)0xcc, "got %02x\n", dst[4]);


    *(int *)src = 4321;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I4, DBTYPE_STR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(!lstrcmpA(dst, fourthreetwoone), "got %s\n", dst);

    *(int *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I4, DBTYPE_STR, 0, &dst_len, src, dst, 0, 0, &dst_status, 0, 0, 0);
    ok(hr == DB_E_ERRORSOCCURRED, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_DATAOVERFLOW, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(dst[0] == (char)0xcc, "got %02x\n", dst[0]);

    *(int *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I4, DBTYPE_STR, 0, &dst_len, src, NULL, 0, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(dst[0] == (char)0xcc, "got %02x\n", dst[0]);

    *(int *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I4, DBTYPE_STR, 0, &dst_len, src, dst, 2, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_TRUNCATED, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(dst[0] == '4', "got %02x\n", dst[0]);
    ok(dst[1] == 0, "got %02x\n", dst[1]);
    ok(dst[2] == (char)0xcc, "got %02x\n", dst[2]);

    *(int *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I4, DBTYPE_STR, 0, &dst_len, src, dst, 1, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_TRUNCATED, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(dst[0] == 0, "got %02x\n", dst[0]);
    ok(dst[1] == (char)0xcc, "got %02x\n", dst[1]);

    *(int *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I4, DBTYPE_STR, 0, &dst_len, src, dst, 4, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_TRUNCATED, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(dst[0] == '4', "got %02x\n", dst[0]);
    ok(dst[1] == '3', "got %02x\n", dst[1]);
    ok(dst[2] == '2', "got %02x\n", dst[2]);
    ok(dst[3] == 0, "got %02x\n", dst[3]);
    ok(dst[4] == (char)0xcc, "got %02x\n", dst[4]);


    *(float *)src = 4321;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R4, DBTYPE_STR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(!lstrcmpA(dst, fourthreetwoone), "got %s\n", dst);

    *(float *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R4, DBTYPE_STR, 0, &dst_len, src, dst, 0, 0, &dst_status, 0, 0, 0);
    ok(hr == DB_E_ERRORSOCCURRED, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_DATAOVERFLOW, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(dst[0] == (char)0xcc, "got %02x\n", dst[0]);

    *(float *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R4, DBTYPE_STR, 0, &dst_len, src, NULL, 0, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(dst[0] == (char)0xcc, "got %02x\n", dst[0]);

    *(float *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R4, DBTYPE_STR, 0, &dst_len, src, dst, 2, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_TRUNCATED, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(dst[0] == '4', "got %02x\n", dst[0]);
    ok(dst[1] == 0, "got %02x\n", dst[1]);
    ok(dst[2] == (char)0xcc, "got %02x\n", dst[2]);

    *(float *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R4, DBTYPE_STR, 0, &dst_len, src, dst, 1, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_TRUNCATED, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(dst[0] == 0, "got %02x\n", dst[0]);
    ok(dst[1] == (char)0xcc, "got %02x\n", dst[1]);

    *(float *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R4, DBTYPE_STR, 0, &dst_len, src, dst, 4, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_TRUNCATED, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(dst[0] == '4', "got %02x\n", dst[0]);
    ok(dst[1] == '3', "got %02x\n", dst[1]);
    ok(dst[2] == '2', "got %02x\n", dst[2]);
    ok(dst[3] == 0, "got %02x\n", dst[3]);
    ok(dst[4] == (char)0xcc, "got %02x\n", dst[4]);


    *(double *)src = 4321;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R8, DBTYPE_STR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(!lstrcmpA(dst, fourthreetwoone), "got %s\n", dst);

    *(double *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R8, DBTYPE_STR, 0, &dst_len, src, dst, 0, 0, &dst_status, 0, 0, 0);
    ok(hr == DB_E_ERRORSOCCURRED, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_DATAOVERFLOW, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(dst[0] == (char)0xcc, "got %02x\n", dst[0]);

    *(double *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R8, DBTYPE_STR, 0, &dst_len, src, NULL, 0, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(dst[0] == (char)0xcc, "got %02x\n", dst[0]);

    *(double *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R8, DBTYPE_STR, 0, &dst_len, src, dst, 2, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_TRUNCATED, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(dst[0] == '4', "got %02x\n", dst[0]);
    ok(dst[1] == 0, "got %02x\n", dst[1]);
    ok(dst[2] == (char)0xcc, "got %02x\n", dst[2]);

    *(double *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R8, DBTYPE_STR, 0, &dst_len, src, dst, 1, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_TRUNCATED, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(dst[0] == 0, "got %02x\n", dst[0]);
    ok(dst[1] == (char)0xcc, "got %02x\n", dst[1]);

    *(double *)src = 4321;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R8, DBTYPE_STR, 0, &dst_len, src, dst, 4, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_TRUNCATED, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(dst[0] == '4', "got %02x\n", dst[0]);
    ok(dst[1] == '3', "got %02x\n", dst[1]);
    ok(dst[2] == '2', "got %02x\n", dst[2]);
    ok(dst[3] == 0, "got %02x\n", dst[3]);
    ok(dst[4] == (char)0xcc, "got %02x\n", dst[4]);



    memset(src, 0, sizeof(src));
    ((CY*)src)->int64 = 43210000;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_CY, DBTYPE_STR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(!lstrcmpA(dst, fourthreetwoone), "got %s\n", dst);



    memset(src, 0, sizeof(src));
    *(signed char *)src = 10;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I1, DBTYPE_STR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 2, "got %Id\n", dst_len);
    ok(!lstrcmpA(dst, ten_a), "got %s\n", dst);

    memset(src, 0, sizeof(src));
    *(unsigned char *)src = 10;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI1, DBTYPE_STR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 2, "got %Id\n", dst_len);
    ok(!lstrcmpA(dst, ten_a), "got %s\n", dst);

    memset(src, 0, sizeof(src));
    *(unsigned short *)src = 4321;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI2, DBTYPE_STR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(!lstrcmpA(dst, fourthreetwoone), "got %s\n", dst);

    memset(src, 0, sizeof(src));
    *(unsigned int *)src = 4321;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI4, DBTYPE_STR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(!lstrcmpA(dst, fourthreetwoone), "got %s\n", dst);

    memset(src, 0, sizeof(src));
    ((LARGE_INTEGER*)src)->QuadPart = 4321;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I8, DBTYPE_STR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(broken(hr == DB_E_UNSUPPORTEDCONVERSION) || hr == S_OK /* W2K+ */, "got %08lx\n", hr);
    ok(broken(dst_status == DBSTATUS_E_BADACCESSOR) || dst_status == DBSTATUS_S_OK /* W2K+ */, "got %08lx\n", dst_status);
    ok(broken(dst_len == 0x1234) || dst_len == 4 /* W2K+ */, "got %Id\n", dst_len);
    ok(!lstrcmpA(dst, fourthreetwoone), "got %s\n", dst);

    memset(src, 0, sizeof(src));
    ((ULARGE_INTEGER*)src)->QuadPart = 4321;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI8, DBTYPE_STR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(broken(hr == DB_E_UNSUPPORTEDCONVERSION) || hr == S_OK /* W2K+ */, "got %08lx\n", hr);
    ok(broken(dst_status == DBSTATUS_E_BADACCESSOR) || dst_status == DBSTATUS_S_OK /* W2K+ */, "got %08lx\n", dst_status);
    ok(broken(dst_len == 0x1234) || dst_len == 4 /* W2K+ */, "got %Id\n", dst_len);
    ok(!lstrcmpA(dst, fourthreetwoone), "got %s\n", dst);



    memset(src, 0, sizeof(src));
    memcpy(src, &IID_IDataConvert, sizeof(GUID));
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_GUID, DBTYPE_STR, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 38, "got %Id\n", dst_len);
    ok(!lstrcmpA(dst, guid_str), "got %s\n", dst);



    b = SysAllocString(ten);
    *(BSTR *)src = b;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BSTR, DBTYPE_STR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 2, "got %Id\n", dst_len);
    ok(!lstrcmpA(ten_a, dst), "got %s\n", dst);
    SysFreeString(b);

    b = SysAllocString(idW);
    *(BSTR *)src = b;
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BSTR, DBTYPE_STR, 0, &dst_len, src, dst, 9, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_TRUNCATED, "got %08lx\n", dst_status);
    ok(dst_len == 13, "got %Id\n", dst_len);
    ok(!lstrcmpA(idA, dst), "got %s\n", dst);
    SysFreeString(b);

    memcpy(src, withnull, sizeof(withnull));
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_STR, DBTYPE_STR, sizeof(withnull), &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 8, "got %Id\n", dst_len);
    ok(!memcmp(withnull, dst, 8), "got %s\n", dst);
    ok(dst[8] == 0, "got %02x\n", dst[8]);

    memcpy(src, withnull, sizeof(withnull));
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_STR, DBTYPE_STR, 7, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 7, "got %Id\n", dst_len);
    ok(!memcmp(withnull, dst, 7), "got %s\n", dst);
    ok(dst[7] == 0, "got %02x\n", dst[7]);

    memcpy(src, withnull, sizeof(withnull));
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_STR, DBTYPE_STR, 6, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 6, "got %Id\n", dst_len);
    ok(!memcmp(withnull, dst, 6), "got %s\n", dst);
    ok(dst[6] == 0, "got %02x\n", dst[6]);

    memcpy(src, ten, sizeof(ten));
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_WSTR, DBTYPE_STR, 2, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 1, "got %Id\n", dst_len);
    ok(dst[0] == '1', "got %02x\n", dst[0]);
    ok(dst[1] == 0, "got %02x\n", dst[1]);

    memcpy(src, ten, sizeof(ten));
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_WSTR, DBTYPE_STR, 4, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 2, "got %Id\n", dst_len);
    ok(!lstrcmpA(ten_a, dst), "got %s\n", dst);

    memcpy(src, ten, sizeof(ten));
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_WSTR, DBTYPE_STR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, DBDATACONVERT_LENGTHFROMNTS);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 2, "got %Id\n", dst_len);
    ok(!lstrcmpA(ten_a, dst), "got %s\n", dst);

    memcpy(src, ten_a, sizeof(ten_a));
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_STR, DBTYPE_STR, 2, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 2, "got %Id\n", dst_len);
    ok(dst[0] == '1', "got %02x\n", dst[0]);
    ok(dst[1] == '0', "got %02x\n", dst[1]);
    ok(dst[2] == 0, "got %02x\n", dst[2]);
    ok(dst[3] == (char)0xcc, "got %02x\n", dst[3]);

    memcpy(src, ten_a, sizeof(ten_a));
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_STR, DBTYPE_STR, 4, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(!lstrcmpA(ten_a, dst), "got %s\n", dst);

    memcpy(src, ten_a, sizeof(ten_a));
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_STR, DBTYPE_STR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, DBDATACONVERT_LENGTHFROMNTS);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 2, "got %Id\n", dst_len);
    ok(!lstrcmpA(ten_a, dst), "got %s\n", dst);



    /* DBTYPE_BYTES to DBTYPE_*STR unpacks binary data into a hex string */
    memcpy(src, hexpacked_a, sizeof(hexpacked_a));
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BYTES, DBTYPE_STR, sizeof(hexpacked_a), &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(hexpacked_a) * 2, "got %Id\n", dst_len);
    ok(!lstrcmpA(hexunpacked_a, dst), "got %s\n", dst);
    ok(dst[ARRAY_SIZE(hexpacked_a) * 4 + 1] == (char)0xcc, "clobbered buffer\n");

    memcpy(src, hexpacked_a, sizeof(hexpacked_a));
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BYTES, DBTYPE_STR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 0, "got %Id\n", dst_len);
    ok(dst[0] == 0, "not null terminated\n");
    ok(dst[1] == (char)0xcc, "clobbered buffer\n");

    memcpy(src, hexpacked_a, sizeof(hexpacked_a));
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BYTES, DBTYPE_STR, 4, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 2 * sizeof(char) * 4, "got %Id\n", dst_len);
    ok(!memcmp(hexunpacked_a, dst, 2 * sizeof(char) * 4 ), "got %s\n", dst);
    ok(dst[2 * 4] == 0, "not null terminated\n");
    ok(dst[2 * 4 + 1] == (char)0xcc, "clobbered buffer\n");

    memcpy(src, hexpacked_a, sizeof(hexpacked_a));
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BYTES, DBTYPE_STR, sizeof(hexpacked_a), &dst_len, src, dst, 2 * sizeof(char) * 4 + sizeof(char), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_TRUNCATED, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(hexpacked_a) * 2, "got %Id\n", dst_len);
    ok(!memcmp(hexunpacked_a, dst, 2 * sizeof(char) * 4 ), "got %s\n", dst);
    ok(dst[2 * 4] == 0, "not null terminated\n");
    ok(dst[2 * 4 + 1] == (char)0xcc, "clobbered buffer\n");

    memcpy(src, hexpacked_a, sizeof(hexpacked_a));
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BYTES, DBTYPE_STR, sizeof(hexpacked_a), &dst_len, src, dst, 2 * sizeof(char) * 4, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_TRUNCATED, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(hexpacked_a) * 2, "got %Id\n", dst_len);
    ok(!memcmp(hexunpacked_a, dst, 2 * sizeof(char) * 4 - 2 ), "got %s\n", dst);
    ok(dst[2 * 4 - 1] == 0, "not null terminated\n");
    ok(dst[2 * 4] == (char)0xcc, "clobbered buffer\n");

    memcpy(src, hexpacked_a, sizeof(hexpacked_a));
    memset(dst, 0xcc, sizeof(dst));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BYTES, DBTYPE_STR, 0, &dst_len, src, dst, sizeof(dst), 0, &dst_status, 0, 0, DBDATACONVERT_LENGTHFROMNTS);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 0, "got %Id\n", dst_len);
    ok(dst[0] == 0, "not null terminated\n");
    ok(dst[1] == (char)0xcc, "clobbered buffer\n");

    dst_len = 44;
    V_VT(&v) = VT_NULL;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_STR, 0, &dst_len, &v, dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_ISNULL, "got %08lx\n", dst_status);
    ok(dst_len == 44, "got %Id\n", dst_len);
}

static void test_converttobyrefwstr(void)
{
    HRESULT hr;
    WCHAR *dst;
    BYTE src[20];
    DBSTATUS dst_status;
    DBLENGTH dst_len;
    static const WCHAR ten[] = {'1','0',0};
    static const WCHAR fourthreetwoone[] = {'4','3','2','1',0};
    BSTR b;
    VARIANT v;

    VariantInit(&v);

    hr = IDataConvert_DataConvert(convert, DBTYPE_EMPTY, DBTYPE_BYREF | DBTYPE_WSTR, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 0, "got %Id\n", dst_len);
    ok(dst[0] == 0, "got %04x\n", dst[0]);
    CoTaskMemFree(dst);

    dst = (void*)0x12345678;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_NULL, DBTYPE_BYREF | DBTYPE_WSTR, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == DB_E_UNSUPPORTEDCONVERSION, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_BADACCESSOR, "got %08lx\n", dst_status);
    ok(dst_len == 0x1234, "got %Id\n", dst_len);
    ok(dst == (void*)0x12345678, "got %p\n", dst);

    *(short *)src = 4321;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I2, DBTYPE_BYREF | DBTYPE_WSTR, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 8, "got %Id\n", dst_len);
    ok(!lstrcmpW(dst, fourthreetwoone), "got %s\n", wine_dbgstr_w(dst));
    CoTaskMemFree(dst);

    *(short *)src = 4321;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I2, DBTYPE_BYREF | DBTYPE_WSTR, 0, &dst_len, src, &dst, 0, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 8, "got %Id\n", dst_len);
    ok(!lstrcmpW(dst, fourthreetwoone), "got %s\n", wine_dbgstr_w(dst));
    CoTaskMemFree(dst);

    b = SysAllocString(ten);
    *(BSTR *)src = b;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BSTR, DBTYPE_BYREF | DBTYPE_WSTR, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(!lstrcmpW(b, dst), "got %s\n", wine_dbgstr_w(dst));
    CoTaskMemFree(dst);
    SysFreeString(b);

    memcpy(src, ten, sizeof(ten));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_WSTR, DBTYPE_BYREF | DBTYPE_WSTR, 2, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 2, "got %Id\n", dst_len);
    ok(dst[0] == '1', "got %02x\n", dst[0]);
    ok(dst[1] == 0, "got %02x\n", dst[1]);
    CoTaskMemFree(dst);

    memcpy(src, ten, sizeof(ten));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_WSTR, DBTYPE_BYREF | DBTYPE_WSTR, 4, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(!lstrcmpW(ten, dst), "got %s\n", wine_dbgstr_w(dst));
    CoTaskMemFree(dst);

    memcpy(src, ten, sizeof(ten));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_WSTR, DBTYPE_BYREF | DBTYPE_WSTR, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, DBDATACONVERT_LENGTHFROMNTS);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(!lstrcmpW(ten, dst), "got %s\n", wine_dbgstr_w(dst));
    CoTaskMemFree(dst);

    dst_len = 44;
    V_VT(&v) = VT_NULL;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_BYREF | DBTYPE_WSTR, 0, &dst_len, &v, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_ISNULL, "got %08lx\n", dst_status);
    ok(dst_len == 44, "got %Id\n", dst_len);
}

static void test_converttobyrefstr(void)
{
    HRESULT hr;
    char *dst;
    BYTE src[20];
    DBSTATUS dst_status;
    DBLENGTH dst_len;
    static const WCHAR ten[] = {'1','0',0};
    static const char withnull[] = "test\0ed";
    BSTR b;
    VARIANT v;

    VariantInit(&v);

    hr = IDataConvert_DataConvert(convert, DBTYPE_EMPTY, DBTYPE_STR | DBTYPE_BYREF, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 0, "got %Id\n", dst_len);
    ok(dst[0] == 0, "got %04x\n", dst[0]);
    CoTaskMemFree(dst);

    dst = (void*)0x12345678;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_NULL, DBTYPE_STR | DBTYPE_BYREF, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == DB_E_UNSUPPORTEDCONVERSION, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_BADACCESSOR, "got %08lx\n", dst_status);
    ok(dst_len == 0x1234, "got %Id\n", dst_len);
    ok(dst == (void*)0x12345678, "got %p\n", dst);

    *(short *)src = 4321;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I2, DBTYPE_STR | DBTYPE_BYREF, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(!lstrcmpA(dst, "4321"), "got %s\n", dst);
    CoTaskMemFree(dst);

    *(short *)src = 4321;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I2, DBTYPE_STR | DBTYPE_BYREF, 0, &dst_len, src, &dst, 0, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 4, "got %Id\n", dst_len);
    ok(!lstrcmpA(dst, "4321"), "got %s\n", dst);
    CoTaskMemFree(dst);

    b = SysAllocString(ten);
    *(BSTR *)src = b;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BSTR, DBTYPE_STR | DBTYPE_BYREF, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 2, "got %Id\n", dst_len);
    ok(!lstrcmpA("10", dst), "got %s\n", dst);
    CoTaskMemFree(dst);
    SysFreeString(b);

    memcpy(src, ten, sizeof(ten));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_WSTR, DBTYPE_STR | DBTYPE_BYREF, 2, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 1, "got %Id\n", dst_len);
    ok(dst[0] == '1', "got %02x\n", dst[0]);
    ok(dst[1] == 0, "got %02x\n", dst[1]);
    CoTaskMemFree(dst);

    memcpy(src, ten, sizeof(ten));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_WSTR, DBTYPE_STR | DBTYPE_BYREF, 4, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 2, "got %Id\n", dst_len);
    ok(!lstrcmpA("10", dst), "got %s\n", dst);
    CoTaskMemFree(dst);

    memcpy(src, ten, sizeof(ten));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_WSTR, DBTYPE_STR | DBTYPE_BYREF, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, DBDATACONVERT_LENGTHFROMNTS);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 2, "got %Id\n", dst_len);
    ok(!lstrcmpA("10", dst), "got %s\n", dst);
    CoTaskMemFree(dst);

    memcpy(src, withnull, sizeof(withnull));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_STR, DBTYPE_STR | DBTYPE_BYREF, sizeof(withnull), &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 8, "got %Id\n", dst_len);
    ok(!memcmp(withnull, dst, 8), "got %s\n", dst);
    ok(dst[8] == 0, "got %02x\n", dst[8]);
    CoTaskMemFree(dst);

    memcpy(src, withnull, sizeof(withnull));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_STR, DBTYPE_STR | DBTYPE_BYREF, 7, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 7, "got %Id\n", dst_len);
    ok(!memcmp(withnull, dst, 7), "got %s\n", dst);
    ok(dst[7] == 0, "got %02x\n", dst[7]);
    CoTaskMemFree(dst);

    memcpy(src, withnull, sizeof(withnull));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_STR, DBTYPE_STR | DBTYPE_BYREF, 6, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == 6, "got %Id\n", dst_len);
    ok(!memcmp(withnull, dst, 6), "got %s\n", dst);
    ok(dst[6] == 0, "got %02x\n", dst[6]);
    CoTaskMemFree(dst);


    dst_len = 44;
    V_VT(&v) = VT_NULL;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_STR | DBTYPE_BYREF, 0, &dst_len, &v, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_ISNULL, "got %08lx\n", dst_status);
    ok(dst_len == 44, "got %Id\n", dst_len);
}

static void test_converttoguid(void)
{
    HRESULT hr;
    GUID dst;
    BYTE src[20];
    DBSTATUS dst_status;
    DBLENGTH dst_len;
    VARIANT v;

    VariantInit(&v);

    dst = IID_IDCInfo;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_EMPTY, DBTYPE_GUID, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(GUID), "got %Id\n", dst_len);
    ok(IsEqualGUID(&dst, &GUID_NULL), "didn't get GUID_NULL\n");

    dst = IID_IDCInfo;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_NULL, DBTYPE_GUID, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == DB_E_UNSUPPORTEDCONVERSION, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_BADACCESSOR, "got %08lx\n", dst_status);
    ok(dst_len == 0x1234, "got %Id\n", dst_len);
    ok(IsEqualGUID(&dst, &IID_IDCInfo), "dst has changed\n");

    dst = IID_IDCInfo;
    memcpy(src, &IID_IDataConvert, sizeof(GUID));
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_GUID, DBTYPE_GUID, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(GUID), "got %Id\n", dst_len);
    ok(IsEqualGUID(&dst, &IID_IDataConvert), "didn't get IID_IDataConvert\n");

    dst_len = 44;
    V_VT(&v) = VT_NULL;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_GUID, 0, &dst_len, &v, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_ISNULL, "got %08lx\n", dst_status);
    ok(dst_len == 44, "got %Id\n", dst_len);

    dst_len = 0x1234;
    dst = IID_IDCInfo;
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = SysAllocStringLen(L"{0c733a8d-2a1c-11ce-ade5-00aa0044773d}", 38);
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_GUID, 0, &dst_len, &v, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(GUID), "got %Id\n", dst_len);
    ok(IsEqualGUID(&dst, &IID_IDataConvert), "didn't get IID_IDataConvert\n");
    SysFreeString(V_BSTR(&v));

    dst_len = 0x1234;
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = SysAllocStringLen(L"{invalid0-0000-0000-0000-000000000000}", 38);
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_GUID, 0, &dst_len, &v, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == CO_E_CLASSSTRING, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_CANTCONVERTVALUE, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(GUID), "got %Id\n", dst_len);
    SysFreeString(V_BSTR(&v));
}

static void test_converttofiletime(void)
{
    HRESULT hr;
    FILETIME dst;
    BYTE src[20];
    DBSTATUS dst_status;
    DBLENGTH dst_len;
    VARIANT v;

    VariantInit(&v);

    memset(&dst, 0xcc, sizeof(dst));
    ((FILETIME *)src)->dwLowDateTime = 0x12345678;
    ((FILETIME *)src)->dwHighDateTime = 0x9abcdef0;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_FILETIME, DBTYPE_FILETIME, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK ||
       broken(hr == DB_E_BADBINDINFO), /* win98 */
       "got %08lx\n", hr);
    if(SUCCEEDED(hr))
    {
        ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
        ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
        ok(dst.dwLowDateTime == 0x12345678, "got %08lx\n", dst.dwLowDateTime);
        ok(dst.dwHighDateTime == 0x9abcdef0, "got %08lx\n", dst.dwHighDateTime);
    }

    dst_len = 44;
    V_VT(&v) = VT_NULL;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_FILETIME, 0, &dst_len, &v, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_ISNULL, "got %08lx\n", dst_status);
    ok(dst_len == 44, "got %Id\n", dst_len);
}

static void test_converttoui1(void)
{
    HRESULT hr;
    BYTE dst;
    BYTE src[20];
    DBSTATUS dst_status;
    DBLENGTH dst_len;
    VARIANT v;

    VariantInit(&v);

    dst = 0x12;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_EMPTY, DBTYPE_UI1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0, "got %08x\n", dst);

    dst = 0x12;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_NULL, DBTYPE_UI1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == DB_E_UNSUPPORTEDCONVERSION, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_BADACCESSOR, "got %08lx\n", dst_status);
    ok(dst_len == 0x1234, "got %Id\n", dst_len);
    ok(dst == 0x12, "got %08x\n", dst);

    dst = 0x12;
    src[0] = 0x43;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI1, DBTYPE_UI1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x43, "got %08x\n", dst);

    dst = 0x12;
    src[0] = 0xfe;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI1, DBTYPE_UI1, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0xfe, "got %08x\n", dst);

    dst_len = 44;
    V_VT(&v) = VT_NULL;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_UI1, 0, &dst_len, &v, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_ISNULL, "got %08lx\n", dst_status);
    ok(dst_len == 44, "got %Id\n", dst_len);
}

static void test_converttoui4(void)
{
    HRESULT hr;
    DWORD dst;
    BYTE src[sizeof(VARIANT)];
    DBSTATUS dst_status;
    DBLENGTH dst_len;

    dst = 0x12345678;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_EMPTY, DBTYPE_UI4, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0, "got %08lx\n", dst);

    dst = 0x12345678;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_NULL, DBTYPE_UI4, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == DB_E_UNSUPPORTEDCONVERSION, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_BADACCESSOR, "got %08lx\n", dst_status);
    ok(dst_len == 0x1234, "got %Id\n", dst_len);
    ok(dst == 0x12345678, "got %08lx\n", dst);

    dst = 0x12345678;
    *(DWORD*)src = 0x87654321;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI4, DBTYPE_UI4, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x87654321, "got %08lx\n", dst);

    dst = 0x12345678;
    *(signed short *)src = 0x4321;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I2, DBTYPE_UI4, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x4321, "got %08lx\n", dst);

    dst = 0x12345678;
    *(signed short *)src = -1;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I2, DBTYPE_UI4, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == DB_E_ERRORSOCCURRED, "got %08lx\n", hr);
    todo_wine
    ok(dst_status == DBSTATUS_E_SIGNMISMATCH, "got %08lx\n", dst_status);
    todo_wine
    ok(broken(dst_len == sizeof(dst)) || dst_len == 0x1234 /* W2K+ */, "got %Id\n", dst_len);
    ok(dst == 0x12345678, "got %08lx\n", dst);

    dst_len = dst = 0x1234;
    V_VT((VARIANT*)src) = VT_I2;
    V_I2((VARIANT*)src) = 0x4321;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_UI4, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x4321, "got %08lx\n", dst);

    dst_len = 44;
    V_VT((VARIANT*)src) = VT_NULL;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_UI4, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_ISNULL, "got %08lx\n", dst_status);
    ok(dst_len == 44, "got %Id\n", dst_len);
}

static void test_converttor4(void)
{
    HRESULT hr;
    FLOAT dst;
    BYTE src[20];
    DBSTATUS dst_status;
    DBLENGTH dst_len;
    VARIANT v;

    VariantInit(&v);

    dst = 1.0;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_EMPTY, DBTYPE_R4, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0.0, "got %f\n", dst);

    dst = 1.0;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_NULL, DBTYPE_R4, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == DB_E_UNSUPPORTEDCONVERSION, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_BADACCESSOR, "got %08lx\n", dst_status);
    ok(dst_len == 0x1234, "got %Id\n", dst_len);
    ok(dst == 1.0, "got %f\n", dst);

    dst = 1.0;
    *(signed int*)src = 12345678;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I4, DBTYPE_R4, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 12345678.0, "got %f\n", dst);

    dst = 1.0;
    *(FLOAT *)src = 10.0;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R4, DBTYPE_R4, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 10.0, "got %f\n", dst);

    dst_len = 44;
    V_VT(&v) = VT_NULL;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_R4, 0, &dst_len, &v, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_ISNULL, "got %08lx\n", dst_status);
    ok(dst_len == 44, "got %Id\n", dst_len);

    dst_len = dst = 0x1234;
    V_VT(&v) = VT_I2;
    V_I2(&v) = 0x4321;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_R4, 0, &dst_len, &v, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x4321, "got %f\n", dst);
}

static void test_converttor8(void)
{
    HRESULT hr;
    DOUBLE dst;
    BYTE src[20];
    DBSTATUS dst_status;
    DBLENGTH dst_len;
    VARIANT var;

    dst = 1.0;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_EMPTY, DBTYPE_R8, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0.0, "got %f\n", dst);

    dst = 1.0;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_NULL, DBTYPE_R8, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == DB_E_UNSUPPORTEDCONVERSION, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_BADACCESSOR, "got %08lx\n", dst_status);
    ok(dst_len == 0x1234, "got %Id\n", dst_len);
    ok(dst == 1.0, "got %f\n", dst);

    dst = 1.0;
    *(signed int*)src = 12345678;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I4, DBTYPE_R8, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 12345678.0, "got %f\n", dst);

    dst = 1.0;
    *(FLOAT *)src = 10.0;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R4, DBTYPE_R8, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 10.0, "got %f\n", dst);

    dst_len = dst = 0x1234;
    V_VT(&var) = VT_I2;
    V_I2(&var) = 0x4321;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_R8, 0, &dst_len, &var, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0x4321, "got %f\n", dst);

    dst_len = 44;
    V_VT(&var) = VT_NULL;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_R8, 0, &dst_len, &var, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_ISNULL, "got %08lx\n", dst_status);
    ok(dst_len == 44, "got %Id\n", dst_len);
}

static void test_converttocy(void)
{
    HRESULT hr;
    CY dst;
    BYTE src[20];
    DBSTATUS dst_status;
    DBLENGTH dst_len;
    VARIANT v;

    VariantInit(&v);

    dst.int64 = 0xcc;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_EMPTY, DBTYPE_CY, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(CY), "got %Id\n", dst_len);
    ok(dst.int64 == 0, "didn't get 0\n");

    dst.int64 = 0xcc;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_NULL, DBTYPE_CY, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == DB_E_UNSUPPORTEDCONVERSION, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_BADACCESSOR, "got %08lx\n", dst_status);
    ok(dst_len == 0x1234, "got %Id\n", dst_len);
    ok(dst.int64 == 0xcc, "dst changed\n");

    dst.int64 = 0xcc;
    *(int*)src = 1234;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I4, DBTYPE_CY, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(CY), "got %Id\n", dst_len);
    ok(dst.int64 == 12340000, "got %ld\n", dst.Lo);

    dst.int64 = 0xcc;
    ((CY*)src)->int64 = 1234;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_CY, DBTYPE_CY, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(CY), "got %Id\n", dst_len);
    ok(dst.int64 == 1234, "got %ld\n", dst.Lo);

    dst_len = 44;
    V_VT(&v) = VT_NULL;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_CY, 0, &dst_len, &v, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_ISNULL, "got %08lx\n", dst_status);
    ok(dst_len == 44, "got %Id\n", dst_len);
}

static void test_converttoui8(void)
{
    HRESULT hr;
    ULARGE_INTEGER dst;
    BYTE src[20];
    DBSTATUS dst_status;
    DBLENGTH dst_len;
    VARIANT v;

    VariantInit(&v);

    dst.QuadPart = 0xcc;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_EMPTY, DBTYPE_UI8, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst.QuadPart == 0, "got %d\n", (int)dst.QuadPart);

    dst.QuadPart = 0xcc;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_NULL, DBTYPE_UI8, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == DB_E_UNSUPPORTEDCONVERSION, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_BADACCESSOR, "got %08lx\n", dst_status);
    ok(dst_len == 0x1234, "got %Id\n", dst_len);
    ok(dst.QuadPart == 0xcc, "dst changed\n");

    dst.QuadPart = 0xcc;
    *(int*)src = 1234;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I4, DBTYPE_UI8, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst.QuadPart == 1234, "got %d\n", (int)dst.QuadPart);

    dst.QuadPart = 0xcc;
    *(int*)src = -1234;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I4, DBTYPE_UI8, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == DB_E_ERRORSOCCURRED, "got %08lx\n", hr);
    todo_wine
    ok(dst_status == DBSTATUS_E_SIGNMISMATCH, "got %08lx\n", dst_status);
    todo_wine
    ok(dst_len == 0x1234, "got %Id\n", dst_len);
    ok(dst.QuadPart == 0xcc, "got %d\n", (int)dst.QuadPart);

    dst.QuadPart = 0xcc;
    ((ULARGE_INTEGER*)src)->QuadPart = 1234;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_UI8, DBTYPE_UI8, 0, &dst_len, src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst.QuadPart == 1234, "got %d\n", (int)dst.QuadPart);

    dst_len = 44;
    V_VT(&v) = VT_NULL;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_UI8, 0, &dst_len, &v, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_ISNULL, "got %08lx\n", dst_status);
    ok(dst_len == 44, "got %Id\n", dst_len);

    V_VT(&v) = VT_UI8;
    V_UI8(&v) = 4321;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_UI8, 0, &dst_len, &v, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst.QuadPart == 4321, "got %d\n", (int)dst.QuadPart);
}

static void test_getconversionsize(void)
{
    DBLENGTH dst_len;
    DBLENGTH src_len;
    HRESULT hr;
    BSTR str;
    static WCHAR strW[] = {'t','e','s','t',0};
    static char strTest[] = "test";
    VARIANT var;
    SAFEARRAY *psa = NULL;
    SAFEARRAYBOUND rgsabound[1];
    int i4 = 200;
    WORD i2 = 201;
    char i1 = (char)203;
    FLOAT f4 = 1.0;
    LONGLONG i8 = 202;
    DATE dbdate;
    DECIMAL dec;
    DBTIMESTAMP ts = {2013, 5, 14, 2, 4, 12, 0};
    DBTIME dbtime;
    DBDATE dbdate1;

    /* same way as CanConvert fails here */
    dst_len = 0;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_NULL, DBTYPE_BSTR, NULL, &dst_len, NULL);
    ok(hr == DB_E_UNSUPPORTEDCONVERSION, "got 0x%08lx\n", hr);

    dst_len = 0;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_I2, DBTYPE_I4, NULL, &dst_len, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 4, "got %Id\n", dst_len);

    hr = IDataConvert_GetConversionSize(convert, DBTYPE_I2, DBTYPE_I4, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    /* size doesn't include string size */
    str = SysAllocStringLen(NULL, 10);
    dst_len = 0;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_BSTR, DBTYPE_VARIANT, NULL, &dst_len, str);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == sizeof(VARIANT), "%Id\n", dst_len);
    SysFreeString(str);

    dst_len = 0;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_WSTR, DBTYPE_WSTR, NULL, &dst_len, strW);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 10, "%Id\n", dst_len);

    dst_len = 0;
    src_len = 2;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_WSTR, DBTYPE_WSTR, &src_len, &dst_len, strW);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 4, "%Id\n", dst_len);

    dst_len = 0;
    src_len = 20;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_WSTR, DBTYPE_WSTR, &src_len, &dst_len, strW);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 22, "%Id\n", dst_len);

    dst_len = 0;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_STR, DBTYPE_WSTR, NULL, &dst_len, strTest);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 10 || broken(dst_len == 12), "%Id\n", dst_len);

    dst_len = 0;
    src_len = 2;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_STR, DBTYPE_WSTR, &src_len, &dst_len, strTest);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 6, "%Id\n", dst_len);

    dst_len = 0;
    src_len = 20;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_STR, DBTYPE_WSTR, &src_len, &dst_len, strTest);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 42, "%Id\n", dst_len);

    dst_len = 0;
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(strW);
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_VARIANT, DBTYPE_WSTR, NULL, &dst_len, &var);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 10, "%Id\n", dst_len);
    VariantClear(&var);

    dst_len = 0;
    src_len = 20;
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(strW);
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_VARIANT, DBTYPE_WSTR, &src_len, &dst_len, &var);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 10, "%Id\n", dst_len);
    VariantClear(&var);

    dst_len = 0;
    src_len = 20;
    V_VT(&var) = VT_I4;
    V_I4(&var) = 4;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_VARIANT, DBTYPE_WSTR, &src_len, &dst_len, &var);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    VariantClear(&var);

    dst_len = 0;
    src_len = 20;
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(strW);
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_VARIANT, DBTYPE_STR, &src_len, &dst_len, &var);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 5, "%Id\n", dst_len);
    VariantClear(&var);

    src_len = 20;
    V_VT(&var) = VT_I4;
    V_I4(&var) = 4;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_VARIANT, DBTYPE_STR, &src_len, &dst_len, &var);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    VariantClear(&var);

    dst_len = 0;
    src_len = 20;
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(strW);
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_VARIANT, DBTYPE_BYTES, &src_len, &dst_len, &var);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 2, "%Id\n", dst_len);
    VariantClear(&var);

    dst_len = 0;
    src_len = 20;
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = 1802;
    psa = SafeArrayCreate(VT_UI1,1,rgsabound);

    V_VT(&var) = VT_ARRAY|VT_UI1;
    V_ARRAY(&var) = psa;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_VARIANT, DBTYPE_BYTES, &src_len, &dst_len, &var);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 1802, "%Id\n", dst_len);
    VariantClear(&var);

    /* On Windows, NULL variants being convert to a non-fixed sized type will return a dst_len of
     * 110 but we aren't testing for this value.
     */
    dst_len = 32;
    V_VT(&var) = VT_NULL;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_VARIANT, DBTYPE_I4, NULL, &dst_len, &var);
    ok(dst_len == 4, "%Id\n", dst_len);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    V_VT(&var) = VT_NULL;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_VARIANT, DBTYPE_STR, NULL, &dst_len, &var);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    V_VT(&var) = VT_NULL;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_VARIANT, DBTYPE_WSTR, NULL, &dst_len, &var);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    src_len = 20;
    V_VT(&var) = VT_NULL;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_VARIANT, DBTYPE_BYTES, &src_len, &dst_len, &var);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);
    VariantClear(&var);

    hr = IDataConvert_GetConversionSize(convert, DBTYPE_NUMERIC, DBTYPE_NUMERIC, NULL, &dst_len, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == sizeof(DB_NUMERIC), "%Id\n", dst_len);

    dst_len = 0;
    src_len = sizeof(i4);
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_I4, DBTYPE_WSTR, &src_len, &dst_len, &i4);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_I4, DBTYPE_WSTR, NULL, &dst_len, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    src_len = sizeof(i4);
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_UI4, DBTYPE_WSTR, &src_len, &dst_len, &i4);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_UI4, DBTYPE_WSTR, NULL, &dst_len, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    src_len = sizeof(i2);
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_I2, DBTYPE_WSTR, &src_len, &dst_len, &i2);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_I2, DBTYPE_WSTR, NULL, &dst_len, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    src_len = sizeof(i2);
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_UI2, DBTYPE_WSTR, &src_len, &dst_len, &i2);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    src_len = sizeof(i1);
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_I1, DBTYPE_WSTR, &src_len, &dst_len, &i1);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_I1, DBTYPE_WSTR, NULL, &dst_len, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    src_len = sizeof(i2);
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_UI2, DBTYPE_WSTR, &src_len, &dst_len, &i2);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_UI2, DBTYPE_WSTR, NULL, &dst_len, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    src_len = sizeof(f4);
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_R4, DBTYPE_WSTR, &src_len, &dst_len, &f4);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_R4, DBTYPE_WSTR, NULL, &dst_len, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    src_len = sizeof(i8);
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_UI8, DBTYPE_WSTR, &src_len, &dst_len, &i8);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_UI8, DBTYPE_WSTR, NULL, &dst_len, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    src_len = sizeof(i8);
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_I8, DBTYPE_WSTR, &src_len, &dst_len, &i8);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_I8, DBTYPE_WSTR, NULL, &dst_len, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    src_len = sizeof(dbdate);
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_DATE, DBTYPE_WSTR, &src_len, &dst_len, &dbdate);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_DATE, DBTYPE_WSTR, NULL, &dst_len, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    src_len = sizeof(dec);
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_DECIMAL, DBTYPE_WSTR, &src_len, &dst_len, &dec);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_DECIMAL, DBTYPE_WSTR, NULL, &dst_len, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_EMPTY, DBTYPE_WSTR, NULL, &dst_len, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    src_len = sizeof(ts);
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_DBTIMESTAMP, DBTYPE_WSTR, &src_len, &dst_len, &ts);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_DBTIMESTAMP, DBTYPE_WSTR, NULL, &dst_len, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    src_len = sizeof(dbtime);
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_DBTIME, DBTYPE_WSTR, &src_len, &dst_len, &dbtime);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_DBTIME, DBTYPE_WSTR, NULL, &dst_len, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    src_len = sizeof(dbdate1);
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_DBDATE, DBTYPE_WSTR, &src_len, &dst_len, &dbdate1);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_DBDATE, DBTYPE_WSTR, NULL, &dst_len, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    src_len = sizeof(i4);
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_I4, DBTYPE_STR, &src_len, &dst_len, &i4);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_I4, DBTYPE_STR, NULL, &dst_len, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    src_len = sizeof(i4);
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_UI4, DBTYPE_STR, &src_len, &dst_len, &i4);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_UI4, DBTYPE_STR, NULL, &dst_len, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    src_len = sizeof(i2);
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_I2, DBTYPE_STR, &src_len, &dst_len, &i2);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_I2, DBTYPE_STR, NULL, &dst_len, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    src_len = sizeof(i2);
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_UI2, DBTYPE_STR, &src_len, &dst_len, &i2);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    src_len = sizeof(i1);
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_I1, DBTYPE_STR, &src_len, &dst_len, &i1);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_I1, DBTYPE_STR, NULL, &dst_len, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    src_len = sizeof(i2);
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_UI2, DBTYPE_STR, &src_len, &dst_len, &i2);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_UI2, DBTYPE_STR, NULL, &dst_len, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    src_len = sizeof(f4);
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_R4, DBTYPE_STR, &src_len, &dst_len, &f4);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_R4, DBTYPE_STR, NULL, &dst_len, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    src_len = sizeof(i8);
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_UI8, DBTYPE_STR, &src_len, &dst_len, &i8);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_UI8, DBTYPE_STR, NULL, &dst_len, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    src_len = sizeof(i8);
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_I8, DBTYPE_STR, &src_len, &dst_len, &i8);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_I8, DBTYPE_STR, NULL, &dst_len, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    src_len = sizeof(dbdate);
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_DATE, DBTYPE_STR, &src_len, &dst_len, &dbdate);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_DATE, DBTYPE_STR, NULL, &dst_len, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    src_len = sizeof(dec);
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_DECIMAL, DBTYPE_STR, &src_len, &dst_len, &dec);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_DECIMAL, DBTYPE_STR, NULL, &dst_len, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_EMPTY, DBTYPE_STR, NULL, &dst_len, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    src_len = sizeof(ts);
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_DBTIMESTAMP, DBTYPE_STR, &src_len, &dst_len, &ts);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_DBTIMESTAMP, DBTYPE_STR, NULL, &dst_len, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    src_len = sizeof(dbtime);
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_DBTIME, DBTYPE_STR, &src_len, &dst_len, &dbtime);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_DBTIME, DBTYPE_STR, NULL, &dst_len, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    src_len = sizeof(dbdate1);
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_DBDATE, DBTYPE_STR, &src_len, &dst_len, &dbdate1);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);

    dst_len = 0;
    hr = IDataConvert_GetConversionSize(convert, DBTYPE_DBDATE, DBTYPE_STR, NULL, &dst_len, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 110, "%Id\n", dst_len);
}

static void test_converttobytes(void)
{
    DBLENGTH dst_len;
    HRESULT hr;
    BYTE byte_src[] = {0, 1, 2, 4, 5};
    BYTE byte_dst[] = {0, 0, 0, 0, 0};
    BYTE dst[10] = {0};
    DBSTATUS dst_status;
    VARIANT v;
    SAFEARRAY *psa = NULL;
    SAFEARRAYBOUND rgsabound[1];

    dst_len = 0;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BYTES, DBTYPE_BYTES, sizeof(byte_src), &dst_len, byte_src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(byte_src), "got %Id\n", dst_len);
    ok(!memcmp(byte_src, dst, dst_len ), "bytes differ\n");

    dst_len = 0;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BYTES, DBTYPE_BYTES, sizeof(byte_src), &dst_len, byte_src, &dst, 2, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_TRUNCATED, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(byte_src), "got %Id\n", dst_len);
    ok(!memcmp(byte_src, dst, 2 ), "bytes differ\n");

    V_VT(&v) = VT_NULL;
    dst_len = 77;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_BYTES, sizeof(v), &dst_len, &v, &dst, 2, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_ISNULL, "got %08lx\n", dst_status);
    ok(dst_len == 77, "got %Id\n", dst_len);

    dst_len = 0;
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = 4;
    psa = SafeArrayCreate(VT_UI1,1,rgsabound);

    V_VT(&v) = VT_ARRAY|VT_UI1;
    V_ARRAY(&v) = psa;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_BYTES, sizeof(v), &dst_len, &v, &dst, 10, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dst_len == 4, "%Id\n", dst_len);
    ok(!memcmp(byte_dst, dst, dst_len), "bytes differ\n");
    VariantClear(&v);

    dst_len = 44;
    V_VT(&v) = VT_NULL;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_BYTES, 0, &dst_len, &v, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_ISNULL, "got %08lx\n", dst_status);
    ok(dst_len == 44, "got %Id\n", dst_len);

}

static void test_converttobytesbyref(void)
{
    DBLENGTH dst_len;
    HRESULT hr;
    BYTE byte_src[] = {0, 1, 2, 4, 5};
    BYTE *dst;
    DBSTATUS dst_status;
    VARIANT v;

    VariantInit(&v);

    dst_len = 0;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BYTES, DBTYPE_BYTES | DBTYPE_BYREF, sizeof(byte_src), &dst_len, byte_src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(byte_src), "got %Id\n", dst_len);
    ok(!memcmp(byte_src, dst, dst_len ), "bytes differ\n");
    CoTaskMemFree(dst);

    dst_len = 44;
    V_VT(&v) = VT_NULL;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_BYTES | DBTYPE_BYREF, 0, &dst_len, &v, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_ISNULL, "got %08lx\n", dst_status);
    ok(dst_len == 44, "got %Id\n", dst_len);
}

static void test_converttodbdate(void)
{
    DBLENGTH dst_len;
    HRESULT hr;
    static const WCHAR strW[] = {'2','0','1','3','-','0','5','-','1','4',0};
    DBDATE ts = {2013, 5, 14};
    DBDATE dst;
    DBSTATUS dst_status;
    VARIANT var;
    BSTR bstr;

    dst_len = 0;
    hr = IDataConvert_DataConvert(convert, DBTYPE_DBDATE, DBTYPE_DBDATE, sizeof(ts), &dst_len, &ts, &dst, 2, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(DBDATE), "got %Id\n", dst_len);
    ok(!memcmp(&ts, &dst, sizeof(DBDATE) ), "bytes differ\n");

    VariantInit(&var);
    V_VT(&var) = VT_DATE;
    V_DATE(&var) = 41408.086250;
    dst_len = 0;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_DBDATE, sizeof(var), &dst_len, &var, &dst, 2, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(DBDATE), "got %Id\n", dst_len);
    ok(!memcmp(&ts, &dst, sizeof(DBDATE) ), "bytes differ\n");

    V_VT(&var) = VT_R8;
    V_R8(&var) = 41408.086250;
    dst_len = 0;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_DBDATE, sizeof(var), &dst_len, &var, &dst, 2, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(DBDATE), "got %Id\n", dst_len);
    ok(!memcmp(&ts, &dst, sizeof(DBDATE) ), "bytes differ\n");

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(strW);
    dst_len = 0;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_DBDATE, sizeof(var), &dst_len, &var, &dst, 2, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(DBDATE), "got %Id\n", dst_len);
    ok(!memcmp(&ts, &dst, sizeof(DBDATE) ), "bytes differ\n");
    VariantClear(&var);

    dst_len = 0;
    bstr = SysAllocString(strW);
    hr = IDataConvert_DataConvert(convert, DBTYPE_BSTR, DBTYPE_DBDATE, 0, &dst_len, &bstr, &dst, 2, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(DBDATE), "got %Id\n", dst_len);
    ok(!memcmp(&ts, &dst, sizeof(DBDATE) ), "bytes differ\n");
    SysFreeString(bstr);

    V_VT(&var) = VT_NULL;
    dst_len = 88;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_DBDATE, sizeof(var), &dst_len, &var, &dst, 2, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_ISNULL, "got %08lx\n", dst_status);
    ok(dst_len == 88, "got %Id\n", dst_len);
}


static void test_converttovar(void)
{
    static WCHAR strW[] = {'t','e','s','t',0};
    BYTE byte_src[5] = {1, 2, 3, 4, 5};
    DBTIMESTAMP ts = {2013, 5, 14, 2, 4, 12, 0};
    DBDATE dbdate = {2013, 5, 15};
    double dvalue = 123.56;
    DBSTATUS dst_status;
    DBLENGTH dst_len;
    VARIANT dst;
    HRESULT hr;
    CY cy, cy2;
    DATE date;
    INT i4;
    signed short i2;
    LARGE_INTEGER i8;
    VARIANT_BOOL boolean = VARIANT_TRUE;
    FLOAT fvalue = 543.21f;
    VARIANT var;

    VariantInit(&var);

    V_VT(&dst) = VT_EMPTY;
    dst_len = 0;
    dst_status = DBSTATUS_S_DEFAULT;
    hr = IDataConvert_DataConvert(convert, DBTYPE_WSTR, DBTYPE_VARIANT, sizeof(strW), &dst_len, strW, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(V_VT(&dst) == VT_BSTR, "got %d\n", V_VT(&dst));
    ok(!lstrcmpW(V_BSTR(&dst), strW), "got %s\n", wine_dbgstr_w(V_BSTR(&dst)));
    VariantClear(&dst);

    /* with null dest length and status */
    V_VT(&dst) = VT_EMPTY;
    hr = IDataConvert_DataConvert(convert, DBTYPE_WSTR, DBTYPE_VARIANT, sizeof(strW), NULL, strW, &dst, sizeof(dst), 0, NULL, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(V_VT(&dst) == VT_BSTR, "got %d\n", V_VT(&dst));
    ok(!lstrcmpW(V_BSTR(&dst), strW), "got %s\n", wine_dbgstr_w(V_BSTR(&dst)));
    VariantClear(&dst);

    V_VT(&dst) = VT_EMPTY;
    dst_status = DBSTATUS_S_DEFAULT;
    i8.QuadPart = 12345;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I8, DBTYPE_VARIANT, sizeof(i8), &dst_len, &i8, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(V_VT(&dst) == VT_DECIMAL, "got %d\n", V_VT(&dst));
    ok(V_DECIMAL(&dst).scale == 0 && V_DECIMAL(&dst).sign == 0 &&
       V_DECIMAL(&dst).Hi32 == 0 && V_DECIMAL(&dst).Lo64 == 12345, "Not Equal\n");

    V_VT(&dst) = VT_EMPTY;
    dst_len = 0;
    dst_status = DBSTATUS_S_DEFAULT;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R4, DBTYPE_VARIANT, sizeof(fvalue), &dst_len, &fvalue, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(V_VT(&dst) == VT_R4, "got %d\n", V_VT(&dst));
    ok(V_R4(&dst) == 543.21f, "got %f\n", V_R4(&dst));

    V_VT(&dst) = VT_EMPTY;
    dst_len = 0;
    dst_status = DBSTATUS_S_DEFAULT;
    hr = IDataConvert_DataConvert(convert, DBTYPE_R8, DBTYPE_VARIANT, sizeof(dvalue), &dst_len, &dvalue, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(V_VT(&dst) == VT_R8, "got %d\n", V_VT(&dst));
    ok(V_R8(&dst) == 123.56, "got %f\n", V_R8(&dst));

    V_VT(&dst) = VT_EMPTY;
    dst_len = 0;
    dst_status = DBSTATUS_S_DEFAULT;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BOOL, DBTYPE_VARIANT, sizeof(boolean), &dst_len, &boolean, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(V_VT(&dst) == VT_BOOL, "got %d\n", V_VT(&dst));
    ok(V_BOOL(&dst) == VARIANT_TRUE, "got %d\n", V_BOOL(&dst));

    V_VT(&dst) = VT_EMPTY;
    dst_len = 0;
    dst_status = DBSTATUS_S_DEFAULT;
    i4 = 123;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I4, DBTYPE_VARIANT, sizeof(i4), &dst_len, &i4, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(V_VT(&dst) == VT_I4, "got %d\n", V_VT(&dst));
    ok(V_I4(&dst) == 123, "got %ld\n", V_I4(&dst));

    V_VT(&dst) = VT_EMPTY;
    dst_len = 0;
    dst_status = DBSTATUS_S_DEFAULT;
    i2 = 123;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I2, DBTYPE_VARIANT, sizeof(i2), &dst_len, &i2, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(V_VT(&dst) == VT_I2, "got %d\n", V_VT(&dst));
    ok(V_I2(&dst) == 123, "got %d\n", V_I2(&dst));

    V_VT(&dst) = VT_EMPTY;
    dst_len = 0;
    dst_status = DBSTATUS_S_DEFAULT;
    date = 123.123;
    hr = IDataConvert_DataConvert(convert, DBTYPE_DATE, DBTYPE_VARIANT, sizeof(date), &dst_len, &date, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(V_VT(&dst) == VT_DATE, "got %d\n", V_VT(&dst));
    ok(V_DATE(&dst) == 123.123, "got %f\n", V_DATE(&dst));

    V_VT(&dst) = VT_EMPTY;
    dst_len = 0;
    dst_status = DBSTATUS_S_DEFAULT;
    cy.Lo = 1;
    cy.Hi = 2;
    hr = IDataConvert_DataConvert(convert, DBTYPE_CY, DBTYPE_VARIANT, sizeof(cy), &dst_len, &cy, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(V_VT(&dst) == VT_CY, "got %d\n", V_VT(&dst));

    cy2 = V_CY(&dst);
    ok(cy2.Lo == cy.Lo && cy2.Hi == cy.Hi, "got %ld,%ld\n", cy2.Lo, cy2.Hi);

    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BYTES, DBTYPE_VARIANT, sizeof(byte_src), &dst_len, &byte_src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(V_VT(&dst) == (VT_ARRAY|VT_UI1), "got %d\n", V_VT(&dst));
    if(V_VT(&dst) == (VT_ARRAY|VT_UI1))
    {
        LONG l;

        hr = SafeArrayGetUBound(V_ARRAY(&dst), 1, &l);
        ok(hr == S_OK, "got %08lx\n", hr);
        ok(l == 4, "got %ld\n", l);  /* 5 elements */
    }
    VariantClear(&dst);

    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BYTES, DBTYPE_VARIANT, 0, &dst_len, &byte_src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(V_VT(&dst) == (VT_ARRAY|VT_UI1), "got %d\n", V_VT(&dst));
    if(V_VT(&dst) == (VT_ARRAY|VT_UI1))
    {
        LONG l;

        hr = SafeArrayGetUBound(V_ARRAY(&dst), 1, &l);
        ok(hr == S_OK, "got %08lx\n", hr);
        ok(l == -1, "got %ld\n", l);  /* 0 elements */
    }
    VariantClear(&dst);

    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BYTES, DBTYPE_VARIANT, 2, &dst_len, &byte_src, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(V_VT(&dst) == (VT_ARRAY|VT_UI1), "got %d\n", V_VT(&dst));
    if(V_VT(&dst) == (VT_ARRAY|VT_UI1))
    {
        LONG l;

        hr = SafeArrayGetUBound(V_ARRAY(&dst), 1, &l);
        ok(hr == S_OK, "got %08lx\n", hr);
        ok(l == 1, "got %ld\n", l);  /* 2 elements */
    }
    VariantClear(&dst);

    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_DBTIMESTAMP, DBTYPE_VARIANT, 0, &dst_len, &ts, &dst, sizeof(ts), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(V_VT(&dst) == VT_DATE, "got %d\n", V_VT(&dst));
    ok( (float)V_DATE(&dst) == 41408.086250f, "got %f\n", V_DATE(&dst));

    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_DBDATE, DBTYPE_VARIANT, 0, &dst_len, &dbdate, &dst, sizeof(dbdate), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(V_VT(&dst) == VT_DATE, "got %d\n", V_VT(&dst));
    ok( (float)V_DATE(&dst) == 41409.0, "got %f\n", V_DATE(&dst));

    /* src_status = DBSTATUS_S_ISNULL */
    i4 = 123;
    dst_len = 99;
    hr = IDataConvert_DataConvert(convert, DBTYPE_I4, DBTYPE_VARIANT, sizeof(i4), &dst_len, &i4, &dst, sizeof(dst), DBSTATUS_S_ISNULL, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_ISNULL, "got %08lx\n", dst_status);
    ok(dst_len == 0, "got %Id\n", dst_len);

    dst_len = 44;
    V_VT(&var) = VT_NULL;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_VARIANT, 0, &dst_len, &var, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_ISNULL, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(VARIANT), "got %Id\n", dst_len);

    dst_len = 44;
    V_VT(&var) = VT_UINT;
    V_UINT(&var) = 1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_VARIANT, 0, &dst_len, &var, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(VARIANT), "got %Id\n", dst_len);
    ok(V_VT(&dst) == VT_UINT, "got %d\n", V_VT(&dst));
    ok(V_UINT(&dst) == 1234, "got %u\n", V_UINT(&dst));
    VariantClear(&dst);
}

static void test_converttotimestamp(void)
{
    static const WCHAR strW[] = {'2','0','1','3','-','0','5','-','1','4',' ','0','2',':','0','4',':','1','2',0};
    static const WCHAR strFullW[] =  {'2','0','1','3','-','0','5','-','1','4',' ','0','2',':','0','4',':','1','2',
                                       '.','0','1','7','0','0','0','0','0','0',0};
    DBTIMESTAMP ts = {2013, 5, 14, 2, 4, 12, 0};
    DBTIMESTAMP ts1 = {2013, 5, 14, 2, 4, 12, 17000000};
    DATE date;
    DBTIMESTAMP dst;
    DBSTATUS dst_status;
    DBLENGTH dst_len;
    VARIANT var;
    HRESULT hr;
    BSTR bstr;

    VariantInit(&var);
    V_VT(&var) = VT_DATE;
    V_DATE(&var) = 41408.086250;
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_DBTIMESTAMP, 0, &dst_len, &var, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(!memcmp(&ts, &dst, sizeof(ts)), "Wrong timestamp\n");

    bstr = SysAllocString(strW);
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BSTR, DBTYPE_DBTIMESTAMP, 0, &dst_len, &bstr, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(!memcmp(&ts, &dst, sizeof(ts)), "Wrong timestamp\n");
    SysFreeString(bstr);

    bstr = SysAllocString(strFullW);
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BSTR, DBTYPE_DBTIMESTAMP, 0, &dst_len, &bstr, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(!memcmp(&ts1, &dst, sizeof(ts1)), "Wrong timestamp\n");
    SysFreeString(bstr);

    bstr = SysAllocString(L"2013-05-14 02:04:12.017000000");
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BSTR, DBTYPE_DBTIMESTAMP, 0, &dst_len, &bstr, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(!memcmp(&ts1, &dst, sizeof(ts1)), "Wrong timestamp\n");
    SysFreeString(bstr);

    bstr = SysAllocString(L"2013/05/14 02:04:12.01700");
    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_BSTR, DBTYPE_DBTIMESTAMP, 0, &dst_len, &bstr, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == DISP_E_TYPEMISMATCH, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_E_CANTCONVERTVALUE, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    SysFreeString(bstr);

    V_VT(&var) = VT_NULL;
    dst_len = 77;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_DBTIMESTAMP, sizeof(var), &dst_len, &var, &dst, 2, 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_ISNULL, "got %08lx\n", dst_status);
    ok(dst_len == 77, "got %Id\n", dst_len);

    dst_len = 0x1234;
    date = 41408.086250;
    hr = IDataConvert_DataConvert(convert, DBTYPE_DATE, DBTYPE_DBTIMESTAMP, 0, &dst_len, &date, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(!memcmp(&ts, &dst, sizeof(ts)), "Wrong timestamp\n");
}

static void test_converttoiunknown(void)
{
    HRESULT hr;
    DBSTATUS dst_status;
    DBLENGTH dst_len;
    IUnknown *dst = NULL;
    static WCHAR strW[] = {'t','e','s','t',0};
    VARIANT var;

    VariantInit(&var);

    dst_len = 0x1234;
    hr = IDataConvert_DataConvert(convert, DBTYPE_WSTR, DBTYPE_IUNKNOWN, sizeof(strW), &dst_len, strW, dst, sizeof(dst), DBSTATUS_S_ISNULL, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_ISNULL, "got %08lx\n", dst_status);
    ok(dst_len == 0, "got %Id\n", dst_len);

    dst_len = 44;
    V_VT(&var) = VT_NULL;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_IUNKNOWN, 0, &dst_len, &var, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_ISNULL, "got %08lx\n", dst_status);
    ok(dst_len == 44, "got %Id\n", dst_len);
}

#define test_numeric_val(current, expected) _test_numeric_val(__LINE__, current, expected);
static inline void _test_numeric_val(unsigned line, DB_NUMERIC *current, DB_NUMERIC *expected)
{
    int same = !memcmp(current, expected, sizeof(DB_NUMERIC));
    ok_(__FILE__,line) (same, "Invalid byte array\n");
    if(!same)
    {
        int i;
        for(i=0; i < sizeof(current->val); i++)
            ok_(__FILE__,line) (current->val[i] == expected->val[i], " byte %d got 0x%02x expected 0x%02x\n", i, current->val[i], expected->val[i]);
    }
}

static void test_converttonumeric(void)
{
    HRESULT hr;
    DBSTATUS dst_status;
    DBLENGTH dst_len;
    DB_NUMERIC dst;
    static WCHAR strW[] = {'1','2','3','.','4','5',0};
    static WCHAR largeW[] = {'1','2','3','4','5','6','7','8','9','0',0};
    INT i;
    BSTR bstr;
    FLOAT fvalue = 543.21f;
    VARIANT_BOOL boolean = VARIANT_TRUE;
    VARIANT var;
    LARGE_INTEGER i8;
    DB_NUMERIC result1 = { 10, 0, 1, {0x02, 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} };       /*  4098      */
    DB_NUMERIC result2 = { 10, 0, 1, {0x39, 0x30, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} };       /*  12345     */
    DB_NUMERIC result3 = { 10, 0, 0, {0x01, 0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} };       /* -1         */
    DB_NUMERIC result4 = { 10, 0, 1, {0x1f, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} };       /*  543.21    */
    DB_NUMERIC result5 = { 10, 0, 1, {0x7b, 0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} };       /*  123.45    */
    DB_NUMERIC result6 = { 10, 0, 1, {0xd2, 0x02, 0x96, 0x49, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} }; /*  123456789 */

    VariantInit(&var);

    i = 4098;
    dst_len = 0x1234;
    dst.scale = 30;
    memset(dst.val, 0xfe, sizeof(dst.val));
    hr = IDataConvert_DataConvert(convert, DBTYPE_I4, DBTYPE_NUMERIC, 0, &dst_len, &i, &dst, sizeof(dst), 0, &dst_status, 10, 0, 0);
    todo_wine ok(hr == S_OK, "got %08lx\n", hr);
    todo_wine ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    todo_wine ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    todo_wine test_numeric_val(&dst, &result1);

    i8.QuadPart = 12345;
    dst_len = 0x1234;
    dst.scale = 30;
    memset(dst.val, 0xfe, sizeof(dst.val));
    hr = IDataConvert_DataConvert(convert, DBTYPE_I8, DBTYPE_NUMERIC, sizeof(i8), &dst_len, &i8, &dst, sizeof(dst), 0, &dst_status, 10, 0, 0);
    todo_wine ok(hr == S_OK, "got %08lx\n", hr);
    todo_wine ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    todo_wine ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    todo_wine test_numeric_val(&dst, &result2);

    dst_len = 0x1234;
    dst.scale = 30;
    dst.sign = 1;
    memset(dst.val, 0xfe, sizeof(dst.val));
    hr = IDataConvert_DataConvert(convert, DBTYPE_BOOL, DBTYPE_NUMERIC, sizeof(boolean), &dst_len, &boolean, &dst, sizeof(dst), 0, &dst_status, 10, 0, 0);
    todo_wine ok(hr == S_OK, "got %08lx\n", hr);
    todo_wine ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    todo_wine ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    todo_wine test_numeric_val(&dst, &result3);

    dst_len = 0x1234;
    dst.scale = 30;
    dst.sign = 0;
    memset(dst.val, 0xfe, sizeof(dst.val));
    hr = IDataConvert_DataConvert(convert, DBTYPE_R4, DBTYPE_NUMERIC, sizeof(fvalue), &dst_len, &fvalue, &dst, sizeof(dst), 0, &dst_status, 10, 0, 0);
    todo_wine ok(hr == S_OK, "got %08lx\n", hr);
    todo_wine ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    todo_wine ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    todo_wine test_numeric_val(&dst, &result4);

    dst_len = 0x1234;
    dst.scale = 30;
    dst.sign = 0;
    memset(dst.val, 0xfe, sizeof(dst.val));
    V_VT(&var) = VT_NULL;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_NUMERIC, sizeof(var), &dst_len, &var, &dst, sizeof(dst), 0, &dst_status, 10, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_ISNULL, "got %08lx\n", dst_status);

    dst_len = 0x1234;
    dst.scale = 30;
    dst.sign = 0;
    memset(dst.val, 0xfe, sizeof(dst.val));
    hr = IDataConvert_DataConvert(convert, DBTYPE_WSTR, DBTYPE_NUMERIC, sizeof(strW), &dst_len, strW, &dst, sizeof(dst), 0, &dst_status, 10, 0, 0);
    todo_wine ok(hr == S_OK, "got %08lx\n", hr);
    todo_wine ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    todo_wine ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    todo_wine test_numeric_val(&dst, &result5);

    bstr = SysAllocString(strW);
    dst_status = 0;
    dst.scale = 30;
    dst.sign = 0;
    dst_len = sizeof(strW);
    memset(dst.val, 0xfe, sizeof(dst.val));
    hr = IDataConvert_DataConvert(convert, DBTYPE_BSTR, DBTYPE_NUMERIC, 0, &dst_len, &bstr, &dst, sizeof(dst), 0, &dst_status, 10, 0, 0);
    todo_wine ok(hr == S_OK, "got %08lx\n", hr);
    todo_wine ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    todo_wine ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    todo_wine test_numeric_val(&dst, &result5);
    SysFreeString(bstr);

    bstr = SysAllocString(largeW);
    dst_status = 0;
    dst.scale = 30;
    dst.sign = 0;
    dst_len = sizeof(largeW);
    memset(dst.val, 0xfe, sizeof(dst.val));
    hr = IDataConvert_DataConvert(convert, DBTYPE_BSTR, DBTYPE_NUMERIC, 0, &dst_len, &bstr, &dst, sizeof(dst), 0, &dst_status, 10, 0, 0);
    todo_wine ok(hr == S_OK, "got %08lx\n", hr);
    todo_wine ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    todo_wine ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    todo_wine test_numeric_val(&dst, &result6);
    SysFreeString(bstr);
}

static void test_converttodate(void)
{
    static const WCHAR strW[] = {'2','0','1','3','-','0','5','-','1','4',0};
    DBLENGTH dst_len;
    HRESULT hr;
    DATE dst, date = 41408.086250;
    DBSTATUS dst_status;
    VARIANT var;
    BSTR bstr;

    dst = 0.0;
    dst_len = 0;
    hr = IDataConvert_DataConvert(convert, DBTYPE_DATE, DBTYPE_DATE, sizeof(date), &dst_len, &date, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 41408.086250, "got %f\n", dst);

    VariantInit(&var);
    V_VT(&var) = VT_DATE;
    V_DATE(&var) = 41408.086250;
    dst = 0.0;
    dst_len = 0;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_DATE, sizeof(var), &dst_len, &var, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 41408.086250, "got %f\n", dst);

    VariantInit(&var);
    V_VT(&var) = VT_R8;
    V_R8(&var) = 41408.086250;
    dst = 0.0;
    dst_len = 0;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_DATE, sizeof(var), &dst_len, &var, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 41408.086250, "got %f\n", dst);

    VariantInit(&var);
    V_VT(&var) = VT_I4;
    V_I4(&var) = 41408;
    dst = 0.0;
    dst_len = 0;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_DATE, sizeof(var), &dst_len, &var, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 41408.000000, "got %f\n", dst);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(strW);
    dst = 0.0;
    dst_len = 0;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_DATE, sizeof(var), &dst_len, &var, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 41408.000000, "got %f\n", dst);
    VariantClear(&var);

    dst = 0.0;
    dst_len = 0;
    bstr = SysAllocString(strW);
    hr = IDataConvert_DataConvert(convert, DBTYPE_BSTR, DBTYPE_DATE, 0, &dst_len, &bstr, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 41408.000000, "got %f\n", dst);
    SysFreeString(bstr);

    V_VT(&var) = VT_EMPTY;
    dst = 1.0;
    dst_len = 0;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_DATE, sizeof(var), &dst_len, &var, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_OK, "got %08lx\n", dst_status);
    ok(dst_len == sizeof(dst), "got %Id\n", dst_len);
    ok(dst == 0.0, "got %f\n", dst);

    V_VT(&var) = VT_NULL;
    dst = 1.0;
    dst_len = 0xdeadbeef;
    hr = IDataConvert_DataConvert(convert, DBTYPE_VARIANT, DBTYPE_DATE, sizeof(var), &dst_len, &var, &dst, sizeof(dst), 0, &dst_status, 0, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dst_status == DBSTATUS_S_ISNULL, "got %08lx\n", dst_status);
    ok(dst_len == 0xdeadbeef, "got %Id\n", dst_len);
    ok(dst == 1.0, "got %f\n", dst);
}

START_TEST(convert)
{
    HRESULT hr;

    OleInitialize(NULL);

    test_dcinfo();

    hr = CoCreateInstance(&CLSID_OLEDB_CONVERSIONLIBRARY, NULL, CLSCTX_INPROC_SERVER, &IID_IDataConvert, (void**)&convert);
    if(FAILED(hr))
    {
        win_skip("Unable to create IDataConvert instance, 0x%08lx\n", hr);
        OleUninitialize();
        return;
    }

    test_canconvert();
    test_converttoi1();
    test_converttoi2();
    test_converttoi4();
    test_converttoi8();
    test_converttostr();
    test_converttobstr();
    test_converttowstr();
    test_converttobyrefwstr();
    test_converttobyrefstr();
    test_converttoguid();
    test_converttoui1();
    test_converttoui4();
    test_converttor4();
    test_converttor8();
    test_converttofiletime();
    test_converttocy();
    test_converttoui8();
    test_converttovar();
    test_converttobytes();
    test_converttobytesbyref();
    test_converttodbdate();
    test_getconversionsize();
    test_converttotimestamp();
    test_converttoiunknown();
    test_converttonumeric();
    test_converttodate();

    IDataConvert_Release(convert);

    OleUninitialize();
}
