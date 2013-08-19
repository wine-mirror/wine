/* OLEDB Database tests
 *
 * Copyright 2012 Alistair Leslie-Hughes
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
#include <stdio.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "msdadc.h"
#include "msdasc.h"
#include "shlobj.h"
#include "msdaguid.h"
#include "initguid.h"
#include "oledberr.h"

#include "wine/test.h"

DEFINE_GUID(CSLID_MSDAER, 0xc8b522cf,0x5cf3,0x11ce,0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d);

static WCHAR initstring_default[] = {'D','a','t','a',' ','S','o','u','r','c','e','=','d','u','m','m','y',';',0};

static const char *debugstr_guid(REFIID riid)
{
    static char buf[50];

    if(!riid)
        return "(null)";

    sprintf(buf, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            riid->Data1, riid->Data2, riid->Data3, riid->Data4[0],
            riid->Data4[1], riid->Data4[2], riid->Data4[3], riid->Data4[4],
            riid->Data4[5], riid->Data4[6], riid->Data4[7]);

    return buf;
}

static void test_GetDataSource(WCHAR *initstring)
{
    IDataInitialize *datainit = NULL;
    IDBInitialize *dbinit = NULL;
    HRESULT hr;

    trace("Data Source: %s\n", wine_dbgstr_w(initstring));

    hr = CoCreateInstance(&CLSID_MSDAINITIALIZE, NULL, CLSCTX_INPROC_SERVER, &IID_IDataInitialize,(void**)&datainit);
    ok(hr == S_OK, "got %08x\n", hr);

    /* a failure to create data source here may indicate provider is simply not present */
    hr = IDataInitialize_GetDataSource(datainit, NULL, CLSCTX_INPROC_SERVER, initstring, &IID_IDBInitialize, (IUnknown**)&dbinit);
    if(SUCCEEDED(hr))
    {
        IDBProperties *props = NULL;
        IMalloc *ppM = NULL;

        hr = SHGetMalloc(&ppM);
        if (FAILED(hr))
        {
            ok(0, "Couldn't get IMalloc object.\n");
            goto end;
        }

        hr = IDBInitialize_QueryInterface(dbinit, &IID_IDBProperties, (void**)&props);
        ok(hr == S_OK, "got %08x\n", hr);
        if(SUCCEEDED(hr))
        {
            ULONG cnt;
            DBPROPINFOSET *pInfoset;
            OLECHAR *ary;

            hr = IDBProperties_GetPropertyInfo(props, 0, NULL, &cnt, &pInfoset, &ary);
            todo_wine ok(hr == S_OK, "got %08x\n", hr);
            if(hr == S_OK)
            {
                ULONG i;
                for(i =0; i < pInfoset->cPropertyInfos; i++)
                {
                    trace("(0x%04x) '%s' %d\n", pInfoset->rgPropertyInfos[i].dwPropertyID, wine_dbgstr_w(pInfoset->rgPropertyInfos[i].pwszDescription),
                                             pInfoset->rgPropertyInfos[i].vtType);
                }

                IMalloc_Free(ppM, ary);
            }

            IDBProperties_Release(props);
        }

        IMalloc_Release(ppM);

end:
        IDBInitialize_Release(dbinit);
    }

    IDataInitialize_Release(datainit);
}

static void test_database(void)
{
    static WCHAR initstring_jet[] = {'P','r','o','v','i','d','e','r','=','M','i','c','r','o','s','o','f','t','.',
         'J','e','t','.','O','L','E','D','B','.','4','.','0',';',
         'D','a','t','a',' ','S','o','u','r','c','e','=','d','u','m','m','y',';',
         'P','e','r','s','i','s','t',' ','S','e','c','u','r','i','t','y',' ','I','n','f','o','=','F','a','l','s','e',';',0};
    static WCHAR initstring_lower[] = {'d','a','t','a',' ','s','o','u','r','c','e','=','d','u','m','m','y',';',0};
    IDataInitialize *datainit = NULL;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_MSDAINITIALIZE, NULL, CLSCTX_INPROC_SERVER, &IID_IDataInitialize, (void**)&datainit);
    if (FAILED(hr))
    {
        win_skip("Unable to load oledb library\n");
        return;
    }
    IDataInitialize_Release(datainit);

    test_GetDataSource(NULL);
    test_GetDataSource(initstring_jet);
    test_GetDataSource(initstring_default);
    test_GetDataSource(initstring_lower);
}

static void test_errorinfo(void)
{
    HRESULT hr;
    IUnknown *unk = NULL;

    hr = CoCreateInstance(&CSLID_MSDAER, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown,(void**)&unk);
    ok(hr == S_OK, "got %08x\n", hr);
    if(hr == S_OK)
    {
        IErrorInfo *errorinfo;
        IErrorRecords *errrecs;

        hr = IUnknown_QueryInterface(unk, &IID_IErrorInfo, (void**)&errorinfo);
        ok(hr == S_OK, "got %08x\n", hr);
        if(hr == S_OK)
        {
            IErrorInfo_Release(errorinfo);
        }

        hr = IUnknown_QueryInterface(unk, &IID_IErrorRecords, (void**)&errrecs);
        ok(hr == S_OK, "got %08x\n", hr);
        if(hr == S_OK)
        {
            ERRORINFO info, info2, info3;
            ULONG cnt = 0;

            memset(&info, 0, sizeof(ERRORINFO));
            info.dwMinor = 1;
            memset(&info2, 0, sizeof(ERRORINFO));
            info2.dwMinor = 2;
            memset(&info3, 0, sizeof(ERRORINFO));

            hr = IErrorRecords_AddErrorRecord(errrecs, NULL, 268435456, NULL, NULL, 0);
            ok(hr == E_INVALIDARG, "got %08x\n", hr);

            hr = IErrorRecords_AddErrorRecord(errrecs, &info, 1, NULL, NULL, 0);
            ok(hr == S_OK, "got %08x\n", hr);

            hr = IErrorRecords_GetRecordCount(errrecs, &cnt);
            ok(hr == S_OK, "got %08x\n", hr);
            ok(cnt == 1, "expected 1 got %d\n", cnt);

            hr = IErrorRecords_AddErrorRecord(errrecs, &info2, 2, NULL, NULL, 0);
            ok(hr == S_OK, "got %08x\n", hr);

            hr = IErrorRecords_GetRecordCount(errrecs, &cnt);
            ok(hr == S_OK, "got %08x\n", hr);
            ok(cnt == 2, "expected 2 got %d\n", cnt);

            hr = IErrorRecords_GetBasicErrorInfo(errrecs, 0, NULL);
            ok(hr == E_INVALIDARG, "got %08x\n", hr);

            hr = IErrorRecords_GetBasicErrorInfo(errrecs, 100, &info3);
            ok(hr == DB_E_BADRECORDNUM, "got %08x\n", hr);

            hr = IErrorRecords_GetBasicErrorInfo(errrecs, 0, &info3);
            todo_wine ok(hr == S_OK, "got %08x\n", hr);
            if(hr == S_OK)
            {
                ok(info3.dwMinor == 2, "expected 2 got %d\n", info3.dwMinor);
            }

            IErrorRecords_Release(errrecs);
        }

        IUnknown_Release(unk);
    }
}

static void test_initializationstring(void)
{
    static WCHAR initstring_msdasql[] = {'P','r','o','v','i','d','e','r','=','M','S','D','A','S','Q','L','.','1',';',
         'D','a','t','a',' ','S','o','u','r','c','e','=','d','u','m','m','y', 0};
    static WCHAR initstring_sqloledb[] = {'P','r','o','v','i','d','e','r','=','S','Q','L','O','L','E','D','B','.','1',';',
         'D','a','t','a',' ','S','o','u','r','c','e','=','d','u','m','m','y', 0};
    IDataInitialize *datainit = NULL;
    IDBInitialize *dbinit = NULL;
    HRESULT hr;
    WCHAR *initstring = NULL;

    hr = CoCreateInstance(&CLSID_MSDAINITIALIZE, NULL, CLSCTX_INPROC_SERVER, &IID_IDataInitialize,(void**)&datainit);
    ok(hr == S_OK, "got %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDataInitialize_GetDataSource(datainit, NULL, CLSCTX_INPROC_SERVER, initstring_default,
                                                                &IID_IDBInitialize, (IUnknown**)&dbinit);
        if(SUCCEEDED(hr))
        {
            hr = IDataInitialize_GetInitializationString(datainit, (IUnknown*)dbinit, 0, &initstring);
            ok(hr == S_OK, "got %08x\n", hr);
            if(hr == S_OK)
            {
                trace("Init String: %s\n", wine_dbgstr_w(initstring));
                todo_wine ok(!lstrcmpW(initstring_msdasql, initstring) ||
                             !lstrcmpW(initstring_sqloledb, initstring), "got %s\n", wine_dbgstr_w(initstring));
                CoTaskMemFree(initstring);
            }

            IDBInitialize_Release(dbinit);
        }

        IDataInitialize_Release(datainit);
    }
}

static void test_rowposition(void)
{
    IEnumConnectionPoints *enum_points;
    IConnectionPointContainer *cpc;
    IConnectionPoint *cp;
    IRowPosition *rowpos;
    HRESULT hr;
    IID iid;

    hr = CoCreateInstance(&CLSID_OLEDB_ROWPOSITIONLIBRARY, NULL, CLSCTX_INPROC_SERVER, &IID_IRowPosition, (void**)&rowpos);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = IRowPosition_QueryInterface(rowpos, &IID_IConnectionPointContainer, (void**)&cpc);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IConnectionPointContainer_EnumConnectionPoints(cpc, &enum_points);
todo_wine
    ok(hr == S_OK, "got 0x%08x\n", hr);
if (hr == S_OK) {
    hr = IEnumConnectionPoints_Next(enum_points, 1, &cp, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IConnectionPoint_GetConnectionInterface(cp, &iid);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(IsEqualIID(&iid, &IID_IRowPositionChange), "got %s\n", debugstr_guid(&iid));
    IConnectionPoint_Release(cp);

    hr = IEnumConnectionPoints_Next(enum_points, 1, &cp, NULL);
    ok(hr == S_FALSE, "got 0x%08x\n", hr);

    IEnumConnectionPoints_Release(enum_points);
}
    IConnectionPointContainer_Release(cpc);
    IRowPosition_Release(rowpos);
}

START_TEST(database)
{
    OleInitialize(NULL);

    test_database();
    test_errorinfo();
    test_initializationstring();

    /* row position */
    test_rowposition();

    OleUninitialize();
}
