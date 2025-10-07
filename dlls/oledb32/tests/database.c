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
#define CONST_VTABLE
#define DBINITCONSTANTS

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "msdadc.h"
#include "msdasc.h"
#include "msdaguid.h"
#include "initguid.h"
#include "oledb.h"
#include "oledberr.h"
#include "msdasql.h"

#include "wine/test.h"

DEFINE_GUID(CSLID_MSDAER, 0xc8b522cf,0x5cf3,0x11ce,0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d);

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown* obj, ULONG ref, int line)
{
    ULONG rc;
    IUnknown_AddRef(obj);
    rc = IUnknown_Release(obj);
    ok_(__FILE__, line)(rc == ref, "expected refcount %ld, got %ld\n", ref, rc);
}

static void free_dbpropinfoset(ULONG count, DBPROPINFOSET *propinfoset)
{
    ULONG i, j;

    for (i = 0; i < count; i++)
    {
        for (j = 0; j < propinfoset[i].cPropertyInfos; j++)
            VariantClear(&propinfoset[i].rgPropertyInfos[j].vValues);
        CoTaskMemFree(propinfoset[i].rgPropertyInfos);
    }
    CoTaskMemFree(propinfoset);
}

static void test_GetDataSource(const WCHAR *initstring)
{
    IDataInitialize *datainit = NULL;
    IDBInitialize *dbinit = NULL;
    HRESULT hr;

    trace("Data Source: %s\n", wine_dbgstr_w(initstring));

    hr = CoCreateInstance(&CLSID_MSDAINITIALIZE, NULL, CLSCTX_INPROC_SERVER, &IID_IDataInitialize,(void**)&datainit);
    ok(hr == S_OK, "got %08lx\n", hr);

    EXPECT_REF(datainit, 1);

    /* a failure to create data source here may indicate provider is simply not present */
    hr = IDataInitialize_GetDataSource(datainit, NULL, CLSCTX_INPROC_SERVER, initstring, &IID_IDBInitialize, (IUnknown**)&dbinit);
    if(SUCCEEDED(hr))
    {
        IDBProperties *props = NULL;

        EXPECT_REF(datainit, 1);
        EXPECT_REF(dbinit, 1);

        hr = IDBInitialize_QueryInterface(dbinit, &IID_IDBProperties, (void**)&props);
        ok(hr == S_OK, "got %08lx\n", hr);
        if(SUCCEEDED(hr))
        {
            ULONG cnt;
            DBPROPINFOSET *pInfoset;
            OLECHAR *ary;

            EXPECT_REF(dbinit, 2);
            EXPECT_REF(props, 2);
            hr = IDBProperties_GetPropertyInfo(props, 0, NULL, &cnt, &pInfoset, &ary);
            ok(hr == S_OK, "got %08lx\n", hr);
            if(hr == S_OK)
            {
                ULONG i;
                for(i =0; i < pInfoset->cPropertyInfos; i++)
                {
                    trace("(0x%04lx) '%s' %d\n", pInfoset->rgPropertyInfos[i].dwPropertyID, wine_dbgstr_w(pInfoset->rgPropertyInfos[i].pwszDescription),
                                             pInfoset->rgPropertyInfos[i].vtType);
                }

                free_dbpropinfoset(cnt, pInfoset);
                CoTaskMemFree(ary);
            }

            IDBProperties_Release(props);
        }

        EXPECT_REF(dbinit, 1);
        IDBInitialize_Release(dbinit);
    }

    EXPECT_REF(datainit, 1);
    IDataInitialize_Release(datainit);
}

/* IDBProperties stub */
static HRESULT WINAPI dbprops_QI(IDBProperties *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IDBProperties) || IsEqualIID(riid, &IID_IUnknown)) {
        *obj = iface;
        IDBProperties_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI dbprops_AddRef(IDBProperties *iface)
{
    return 2;
}

static ULONG WINAPI dbprops_Release(IDBProperties *iface)
{
    return 1;
}

static HRESULT WINAPI dbprops_GetProperties(IDBProperties *iface, ULONG cPropertyIDSets,
    const DBPROPIDSET rgPropertyIDSets[], ULONG *pcPropertySets, DBPROPSET **prgPropertySets)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI dbprops_GetPropertyInfo(IDBProperties *iface, ULONG cPropertyIDSets,
    const DBPROPIDSET rgPropertyIDSets[], ULONG *pcPropertyInfoSets, DBPROPINFOSET **prgPropertyInfoSets,
    OLECHAR **ppDescBuffer)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI dbprops_SetProperties(IDBProperties *iface, ULONG set_count, DBPROPSET propsets[])
{
    ok(set_count == 1, "got %lu\n", set_count);

    ok(IsEqualIID(&propsets->guidPropertySet, &DBPROPSET_DBINIT), "set guid %s\n", wine_dbgstr_guid(&propsets->guidPropertySet));
    ok(propsets->cProperties == 2, "got propcount %lu\n", propsets->cProperties);

    ok(propsets->rgProperties[0].dwPropertyID == DBPROP_INIT_DATASOURCE, "got propid[0] %lu\n", propsets->rgProperties[0].dwPropertyID);
    ok(propsets->rgProperties[0].dwOptions == DBPROPOPTIONS_REQUIRED, "got options[0] %lu\n", propsets->rgProperties[0].dwOptions);
    ok(propsets->rgProperties[0].dwStatus == 0, "got status[0] %lu\n", propsets->rgProperties[0].dwStatus);
    ok(V_VT(&propsets->rgProperties[0].vValue) == VT_BSTR, "got vartype[0] %u\n", V_VT(&propsets->rgProperties[0].vValue));

    if (!wcscmp(V_BSTR(&propsets->rgProperties[0].vValue), L"dummy"))
    {
        ok(propsets->rgProperties[1].dwPropertyID == DBPROP_INIT_PROVIDERSTRING, "got propid[1] %lu\n", propsets->rgProperties[1].dwPropertyID);
        ok(propsets->rgProperties[1].dwOptions == DBPROPOPTIONS_REQUIRED, "got options[1] %lu\n", propsets->rgProperties[1].dwOptions);
        ok(propsets->rgProperties[1].dwStatus == 0, "got status[1] %lu\n", propsets->rgProperties[1].dwStatus);
        ok(V_VT(&propsets->rgProperties[1].vValue) == VT_BSTR, "got vartype[1] %u\n", V_VT(&propsets->rgProperties[1].vValue));
    }
    else if (!wcscmp(V_BSTR(&propsets->rgProperties[0].vValue), L"initial_catalog_test"))
    {
        ok(propsets->rgProperties[1].dwPropertyID == DBPROP_INIT_CATALOG, "got propid[1] %lu\n", propsets->rgProperties[1].dwPropertyID);
        ok(propsets->rgProperties[1].dwOptions == DBPROPOPTIONS_REQUIRED, "got options[1] %lu\n", propsets->rgProperties[1].dwOptions);
        ok(propsets->rgProperties[1].dwStatus == 0, "got status[1] %lu\n", propsets->rgProperties[1].dwStatus);
        ok(V_VT(&propsets->rgProperties[1].vValue) == VT_BSTR, "got vartype[1] %u\n", V_VT(&propsets->rgProperties[1].vValue));
        ok(!wcscmp(V_BSTR(&propsets->rgProperties[1].vValue), L"dummy_catalog"), "got initial catalog %s\n",
           wine_dbgstr_variant(&propsets->rgProperties[1].vValue));
    }
    else if (!wcscmp(V_BSTR(&propsets->rgProperties[0].vValue), L"provider_prop_test"))
    {
        ok(propsets->rgProperties[1].dwPropertyID == DBPROP_INIT_PROVIDERSTRING, "got propid[1] %lu\n", propsets->rgProperties[1].dwPropertyID);
        ok(propsets->rgProperties[1].dwOptions == DBPROPOPTIONS_REQUIRED, "got options[1] %lu\n", propsets->rgProperties[1].dwOptions);
        ok(propsets->rgProperties[1].dwStatus == 0, "got status[1] %lu\n", propsets->rgProperties[1].dwStatus);
        ok(V_VT(&propsets->rgProperties[1].vValue) == VT_BSTR, "got vartype[1] %u\n", V_VT(&propsets->rgProperties[1].vValue));
        ok(!wcscmp(V_BSTR(&propsets->rgProperties[1].vValue), L"a=1;b=2;c=3"), "got provider string %s\n",
           wine_dbgstr_variant(&propsets->rgProperties[1].vValue));
    }
    return S_OK;
}

static const IDBPropertiesVtbl dbpropsvtbl = {
    dbprops_QI,
    dbprops_AddRef,
    dbprops_Release,
    dbprops_GetProperties,
    dbprops_GetPropertyInfo,
    dbprops_SetProperties
};

static IDBProperties dbprops = { &dbpropsvtbl };

/* IPersist stub */
static HRESULT WINAPI dbpersist_QI(IPersist *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IPersist) || IsEqualIID(riid, &IID_IUnknown)) {
        *obj = iface;
        IPersist_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI dbpersist_AddRef(IPersist *iface)
{
    return 2;
}

static ULONG WINAPI dbpersist_Release(IPersist *iface)
{
    return 1;
}

static HRESULT WINAPI dbpersist_GetClassID(IPersist *iface, CLSID *clsid)
{
    return CLSIDFromProgID(L"MSDASQL", clsid);
}

static const IPersistVtbl dbpersistvtbl = {
    dbpersist_QI,
    dbpersist_AddRef,
    dbpersist_Release,
    dbpersist_GetClassID
};

static IPersist dbpersist = { &dbpersistvtbl };

/* IDBInitialize stub */
static HRESULT WINAPI dbinit_QI(IDBInitialize *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IDBInitialize) || IsEqualIID(riid, &IID_IUnknown)) {
        *obj = iface;
        IDBInitialize_AddRef(iface);
        return S_OK;
    }
    else if (IsEqualIID(riid, &IID_IPersist)) {
        *obj = &dbpersist;
        return S_OK;
    }
    else if (IsEqualIID(riid, &IID_IDBProperties)) {
        *obj = &dbprops;
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI dbinit_AddRef(IDBInitialize *iface)
{
    return 2;
}

static ULONG WINAPI dbinit_Release(IDBInitialize *iface)
{
    return 1;
}

static HRESULT WINAPI dbinit_Initialize(IDBInitialize *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI dbinit_Uninitialize(IDBInitialize *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IDBInitializeVtbl dbinitvtbl = {
    dbinit_QI,
    dbinit_AddRef,
    dbinit_Release,
    dbinit_Initialize,
    dbinit_Uninitialize
};

static IDBInitialize dbinittest = { &dbinitvtbl };

static void test_GetDataSource2(const WCHAR *initstring)
{
    IDataInitialize *datainit = NULL;
    IDBInitialize *dbinit = NULL;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_MSDAINITIALIZE, NULL, CLSCTX_INPROC_SERVER, &IID_IDataInitialize,(void**)&datainit);
    ok(hr == S_OK, "got %08lx\n", hr);

    dbinit = &dbinittest;
    hr = IDataInitialize_GetDataSource(datainit, NULL, CLSCTX_INPROC_SERVER, initstring, &IID_IDBInitialize, (IUnknown**)&dbinit);
    ok(hr == S_OK, "got %08lx\n", hr);

    IDataInitialize_Release(datainit);
}

static void test_database(void)
{
    static const WCHAR *initstring_default = L"Data Source=dummy;";
    static const WCHAR *initstring_jet = L"Provider=Microsoft.Jet.OLEDB.4.0;Data Source=dummy;Persist Security Info=False;";
    static const WCHAR *initstring_lower = L"data source=dummy;";
    static const WCHAR *customprop = L"data source=dummy;customprop=123.4;";
    static const WCHAR *initial_catalog_prop = L"Data Source=initial_catalog_test;Initial Catalog=dummy_catalog";
    static const WCHAR *extended_prop = L"data source=dummy;Extended Properties=\"DRIVER=A Wine ODBC driver;UID=wine;\";";
    static const WCHAR *extended_prop2 = L"data source=\'dummy\';customprop=\'123.4\';";
    static const WCHAR *multi_provider_prop_test = L"Data Source=provider_prop_test; a=1; b=2;c=3;";
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
    test_GetDataSource2(customprop);
    test_GetDataSource2(initial_catalog_prop);
    test_GetDataSource2(extended_prop);
    test_GetDataSource2(extended_prop2);
    test_GetDataSource2(multi_provider_prop_test);
}

static void free_dispparams(DISPPARAMS *params)
{
    unsigned int i;

    for (i = 0; i < params->cArgs && params->rgvarg; i++)
        VariantClear(&params->rgvarg[i]);
    CoTaskMemFree(params->rgvarg);
    CoTaskMemFree(params->rgdispidNamedArgs);
}

static void test_errorinfo(void)
{
    ICreateErrorInfo *createerror;
    ERRORINFO info, info2, info3;
    IErrorInfo *errorinfo, *errorinfo2;
    IErrorRecords *errrecs;
    IUnknown *unk = NULL, *unk2;
    DISPPARAMS dispparams;
    DWORD context;
    DISPID dispid;
    ULONG cnt = 0;
    VARIANT arg;
    HRESULT hr;
    GUID guid;
    BSTR str;

    hr = CoCreateInstance(&CSLID_MSDAER, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = IUnknown_QueryInterface(unk, &IID_IErrorInfo, (void**)&errorinfo);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = IErrorInfo_GetGUID(errorinfo, NULL);
    ok(hr == E_INVALIDARG, "got %08lx\n", hr);

    hr = IErrorInfo_GetSource(errorinfo, NULL);
    ok(hr == E_INVALIDARG, "got %08lx\n", hr);

    hr = IErrorInfo_GetDescription(errorinfo, NULL);
    ok(hr == E_INVALIDARG, "got %08lx\n", hr);

    hr = IErrorInfo_GetHelpFile(errorinfo, NULL);
    ok(hr == E_INVALIDARG, "got %08lx\n", hr);

    hr = IErrorInfo_GetHelpContext(errorinfo, NULL);
    ok(hr == E_INVALIDARG, "got %08lx\n", hr);

    memset(&guid, 0xac, sizeof(guid));
    hr = IErrorInfo_GetGUID(errorinfo, &guid);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(IsEqualGUID(&guid, &GUID_NULL), "got wrong guid\n");

    str = (BSTR)0x1;
    hr = IErrorInfo_GetSource(errorinfo, &str);
    ok(hr == E_FAIL, "got %08lx\n", hr);
    ok(str == NULL, "got %s\n", wine_dbgstr_w(str));

    str = (BSTR)0x1;
    hr = IErrorInfo_GetDescription(errorinfo, &str);
    ok(hr == E_FAIL, "got %08lx\n", hr);
    ok(str == NULL, "got %s\n", wine_dbgstr_w(str));

    str = (BSTR)0x1;
    hr = IErrorInfo_GetHelpFile(errorinfo, &str);
    ok(hr == E_FAIL, "got %08lx\n", hr);
    ok(str == NULL, "got %s\n", wine_dbgstr_w(str));

    context = 1;
    hr = IErrorInfo_GetHelpContext(errorinfo, &context);
    ok(hr == E_FAIL, "got %08lx\n", hr);
    ok(context == 0, "got %ld\n", context);

    hr = IErrorInfo_QueryInterface(errorinfo, &IID_ICreateErrorInfo, (void**)&createerror);
    ok(hr == E_NOINTERFACE, "got %08lx\n", hr);

    hr = IUnknown_QueryInterface(unk, &IID_IErrorRecords, (void**)&errrecs);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = IErrorRecords_GetRecordCount(errrecs, &cnt);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(cnt == 0, "Got unexpected record count %lu\n", cnt);

    hr = IErrorRecords_GetBasicErrorInfo(errrecs, 0, &info3);
    ok(hr == DB_E_BADRECORDNUM, "got %08lx\n", hr);

    hr = IErrorRecords_GetCustomErrorObject(errrecs, 0, &IID_IUnknown, &unk2);
    ok(hr == DB_E_BADRECORDNUM, "got %08lx\n", hr);

    hr = IErrorRecords_GetErrorInfo(errrecs, 0, 0, &errorinfo2);
    ok(hr == DB_E_BADRECORDNUM, "got %08lx\n", hr);

    hr = IErrorRecords_GetErrorParameters(errrecs, 0, &dispparams);
    ok(hr == DB_E_BADRECORDNUM, "got %08lx\n", hr);

    memset(&info, 0, sizeof(ERRORINFO));
    info.dwMinor = 1;
    memcpy(&info.iid, &IID_IUnknown, sizeof(info.iid));
    memcpy(&info.clsid, &IID_IDispatch, sizeof(info.clsid));
    memset(&info2, 0, sizeof(ERRORINFO));
    info2.dwMinor = 2;
    memcpy(&info2.iid, &IID_IErrorInfo, sizeof(info2.iid));
    memset(&info3, 0, sizeof(ERRORINFO));

    hr = IErrorRecords_AddErrorRecord(errrecs, NULL, IDENTIFIER_SDK_ERROR, NULL, NULL, 0);
    ok(hr == E_INVALIDARG, "got %08lx\n", hr);

    /* Record that doesn't use lookup service. */
    hr = IErrorRecords_AddErrorRecord(errrecs, &info, IDENTIFIER_SDK_ERROR, NULL, NULL, 0);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = IErrorRecords_GetRecordCount(errrecs, &cnt);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(cnt == 1, "expected 1 got %ld\n", cnt);

    memset(&guid, 0xac, sizeof(guid));
    hr = IErrorInfo_GetGUID(errorinfo, &guid);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(IsEqualGUID(&guid, &info.iid), "got wrong guid\n");

    str = (BSTR)0x1;
    hr = IErrorInfo_GetSource(errorinfo, &str);
    ok(hr == E_FAIL, "got %08lx\n", hr);
    ok(str == NULL, "got %s\n", wine_dbgstr_w(str));

    str = NULL;
    hr = IErrorInfo_GetDescription(errorinfo, &str);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(!!str, "Expected error text.\n");
    SysFreeString(str);

    str = (BSTR)0x1;
    hr = IErrorInfo_GetHelpFile(errorinfo, &str);
    ok(hr == E_FAIL, "got %08lx\n", hr);
    ok(str == NULL, "got %s\n", wine_dbgstr_w(str));

    context = 1;
    hr = IErrorInfo_GetHelpContext(errorinfo, &context);
    ok(hr == E_FAIL, "got %08lx\n", hr);
    ok(context == 0, "got %ld\n", context);

    /* Record does not contain custom error object. */
    unk2 = (void*)0xdeadbeef;
    hr = IErrorRecords_GetCustomErrorObject(errrecs, 0, &IID_IUnknown, &unk2);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(unk2 == NULL, "Got custom object %p.\n", unk2);

    hr = IErrorRecords_AddErrorRecord(errrecs, &info2, 2, NULL, NULL, 0);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = IErrorRecords_GetRecordCount(errrecs, &cnt);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(cnt == 2, "expected 2 got %ld\n", cnt);

    hr = IErrorInfo_GetGUID(errorinfo, &guid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &IID_IErrorInfo), "Unexpected guid.\n");

    hr = IErrorRecords_GetBasicErrorInfo(errrecs, 0, NULL);
    ok(hr == E_INVALIDARG, "got %08lx\n", hr);

    hr = IErrorRecords_GetBasicErrorInfo(errrecs, 100, &info3);
    ok(hr == DB_E_BADRECORDNUM, "got %08lx\n", hr);

    hr = IErrorRecords_GetBasicErrorInfo(errrecs, 0, &info3);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(info3.dwMinor == 2, "expected 2 got %ld\n", info3.dwMinor);

    hr = IErrorRecords_GetErrorParameters(errrecs, 0, NULL);
    ok(hr == E_INVALIDARG, "got %08lx\n", hr);

    memset(&dispparams, 0xcc, sizeof(dispparams));
    hr = IErrorRecords_GetErrorParameters(errrecs, 0, &dispparams);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(dispparams.rgvarg == NULL, "Got arguments %p\n", dispparams.rgvarg);
    ok(dispparams.rgdispidNamedArgs == NULL, "Got named arguments %p\n", dispparams.rgdispidNamedArgs);
    ok(dispparams.cArgs == 0, "Got argument count %u\n", dispparams.cArgs);
    ok(dispparams.cNamedArgs == 0, "Got named argument count %u\n", dispparams.cNamedArgs);

    V_VT(&arg) = VT_BSTR;
    V_BSTR(&arg) = SysAllocStringLen(NULL, 0);
    dispid = 0x123;

    dispparams.rgvarg = &arg;
    dispparams.cArgs = 1;
    dispparams.rgdispidNamedArgs = &dispid;
    dispparams.cNamedArgs = 1;
    hr = IErrorRecords_AddErrorRecord(errrecs, &info2, 0, &dispparams, NULL, 0);
    ok(hr == S_OK, "got %08lx\n", hr);

    memset(&dispparams, 0, sizeof(dispparams));
    hr = IErrorRecords_GetErrorParameters(errrecs, 0, &dispparams);
    ok(hr == S_OK, "got %08lx\n", hr);

    ok(V_VT(&dispparams.rgvarg[0]) == VT_BSTR, "Got arg type %d\n", V_VT(&dispparams.rgvarg[0]));
    ok(V_BSTR(&dispparams.rgvarg[0]) != V_BSTR(&arg), "Got arg bstr %d\n", V_VT(&dispparams.rgvarg[0]));

    ok(dispparams.rgdispidNamedArgs[0] == 0x123, "Got named argument %ld\n", dispparams.rgdispidNamedArgs[0]);
    ok(dispparams.cArgs == 1, "Got argument count %u\n", dispparams.cArgs);
    ok(dispparams.cNamedArgs == 1, "Got named argument count %u\n", dispparams.cNamedArgs);

    EXPECT_REF(errrecs, 3);
    EXPECT_REF(errorinfo, 3);
    hr = IErrorRecords_GetErrorInfo(errrecs, 0, 0, &errorinfo2);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(errorinfo == errorinfo2, "different object\n");
    EXPECT_REF(errorinfo, 4);
    EXPECT_REF(errrecs, 4);
    IErrorInfo_Release(errorinfo2);

    free_dispparams(&dispparams);
    VariantClear(&arg);

    IErrorInfo_Release(errorinfo);
    IErrorRecords_Release(errrecs);
    IUnknown_Release(unk);

    /* Shift in record indexing. Same returned IErrorInfo instance refers to a live record collection,
       not a copy. */
    hr = CoCreateInstance(&CSLID_MSDAER, NULL, CLSCTX_INPROC_SERVER, &IID_IErrorRecords, (void **)&errrecs);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    memset(&info, 0, sizeof(info));
    memcpy(&info.iid, &IID_IUnknown, sizeof(info.iid));
    hr = IErrorRecords_AddErrorRecord(errrecs, &info, IDENTIFIER_SDK_ERROR, NULL, NULL, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IErrorRecords_GetErrorInfo(errrecs, 0, 0, &errorinfo);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IErrorInfo_GetGUID(errorinfo, &guid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &IID_IUnknown), "Unexpected guid %s.\n", wine_dbgstr_guid(&guid));

    memset(&info, 0, sizeof(info));
    memcpy(&info.iid, &IID_IDispatch, sizeof(info.iid));
    hr = IErrorRecords_AddErrorRecord(errrecs, &info, IDENTIFIER_SDK_ERROR, NULL, NULL, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IErrorRecords_GetErrorInfo(errrecs, 1, 0, &errorinfo2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IErrorInfo_GetGUID(errorinfo2, &guid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(IsEqualGUID(&guid, &IID_IUnknown), "Unexpected guid %s.\n", wine_dbgstr_guid(&guid));

    hr = IErrorInfo_GetGUID(errorinfo, &guid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &IID_IDispatch), "Unexpected guid %s.\n", wine_dbgstr_guid(&guid));

    memset(&info, 0, sizeof(info));
    memcpy(&info.iid, &IID_IErrorInfo, sizeof(info.iid));
    hr = IErrorRecords_AddErrorRecord(errrecs, &info, IDENTIFIER_SDK_ERROR, NULL, NULL, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IErrorInfo_GetGUID(errorinfo2, &guid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(IsEqualGUID(&guid, &IID_IDispatch), "Unexpected guid %s.\n", wine_dbgstr_guid(&guid));

    hr = IErrorInfo_GetGUID(errorinfo, &guid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &IID_IErrorInfo), "Unexpected guid %s.\n", wine_dbgstr_guid(&guid));

    IErrorInfo_Release(errorinfo2);
    IErrorInfo_Release(errorinfo);
    IErrorRecords_Release(errrecs);
}

#define expect_initstring(a, b, c) _expect_initstring(__LINE__, a, b, c)
static void _expect_initstring(int line, IDataInitialize *datainit, const WCHAR *initstring, const WCHAR *expected)
{
    IDBInitialize *dbinit = NULL;
    WCHAR *result = NULL;
    HRESULT hr;

    hr = IDataInitialize_GetDataSource(datainit, NULL, CLSCTX_INPROC_SERVER, initstring,
                                       &IID_IDBInitialize, (IUnknown**)&dbinit);
    ok_(__FILE__, line)(hr == S_OK, "got %08lx\n", hr);
    hr = IDataInitialize_GetInitializationString(datainit, (IUnknown*)dbinit, 0, &result);
    ok_(__FILE__, line)(hr == S_OK, "got %08lx\n", hr);
    ok_(__FILE__, line)(!lstrcmpW(expected, result), "expected %s, got %s\n",
                        wine_dbgstr_w(expected), wine_dbgstr_w(result));
    CoTaskMemFree(result);
    IDBInitialize_Release(dbinit);
}

static void test_initializationstring(void)
{
    static const WCHAR *initstring_default = L"Data Source=dummy;";
    static const WCHAR *initstring_msdasql = L"Provider=MSDASQL.1;Data Source=dummy";
    static const WCHAR *initstring_msdasql2 = L"pRoViDeR=MSDASQL.1;Data Source=dummy";
    static const WCHAR *initstring_sqloledb = L"Provider=SQLOLEDB.1;Data Source=dummy";
    static const WCHAR *initstring_mode = L"Provider=MSDASQL.1;Data Source=dummy;Mode=invalid";
    static const WCHAR *initstring_mode2 = L"Provider=MSDASQL.1;Data Source=dummy;Mode=WriteRead";
    static const WCHAR *initstring_mode3 = L"Provider=MSDASQL.1;Data Source=dummy;Mode=ReadWRITE";
    static const WCHAR *initstring_mode4 = L"Provider=MSDASQL.1;Data Source=dummy;Mode=ReadWrite|Share Deny None";
    static const WCHAR *initstring_mode5 = L"Provider=MSDASQL.1;Data Source=dummy;Mode=ReadWrite|Share Deny None|Share Exclusive";
    static const WCHAR *initstring_quote_semicolon = L"Provider=MSDASQL.1;"
                                                     "Data Source=dummy;"
                                                     "Extended Properties=\"ConnectTo=11.0;Cell Error Mode=TextValue;Optimize Response=3;\"";
    static const WCHAR *initstring_duplicate = L"Provider=MSDASQL.1;"
                                               "Data Source=dummy;"
                                               "Data Source=dummy;";
    static const WCHAR *initstring_skip_properties = L"Provider=MSDASQL.1;"
                                                     "Data Source=dummy;"
                                                     "Window Handle=0;"
                                                     "Prompt=4;"
                                                     "Connect Timeout=1000;"
                                                     "General Timeout=1000;"
                                                     "OLE DB Services=0;"
                                                     "Locale Identifier=1033";
    IDataInitialize *datainit = NULL;
    IDBInitialize *dbinit;
    HRESULT hr;
    WCHAR *initstring = NULL;

    hr = CoCreateInstance(&CLSID_MSDAINITIALIZE, NULL, CLSCTX_INPROC_SERVER, &IID_IDataInitialize,(void**)&datainit);
    ok(hr == S_OK, "got %08lx\n", hr);
    if(SUCCEEDED(hr))
    {
        EXPECT_REF(datainit, 1);

        dbinit = NULL;
        hr = IDataInitialize_GetDataSource(datainit, NULL, CLSCTX_INPROC_SERVER, initstring_default,
                                                                &IID_IDBInitialize, (IUnknown**)&dbinit);
        if(SUCCEEDED(hr))
        {
            EXPECT_REF(datainit, 1);
            EXPECT_REF(dbinit, 1);

            hr = IDataInitialize_GetInitializationString(datainit, (IUnknown*)dbinit, 0, &initstring);
            ok(hr == S_OK, "got %08lx\n", hr);
            if(hr == S_OK)
            {
                trace("Init String: %s\n", wine_dbgstr_w(initstring));
                ok(!lstrcmpW(initstring_msdasql, initstring) ||
                   !lstrcmpW(initstring_sqloledb, initstring), "got %s\n", wine_dbgstr_w(initstring));
                CoTaskMemFree(initstring);
            }

            IDBInitialize_Release(dbinit);

            /* mixed casing string */
            dbinit = NULL;
            hr = IDataInitialize_GetDataSource(datainit, NULL, CLSCTX_INPROC_SERVER, (WCHAR*)initstring_msdasql2,
                &IID_IDBInitialize, (IUnknown**)&dbinit);
            ok(hr == S_OK, "got 0x%08lx\n", hr);
            IDBInitialize_Release(dbinit);

            /* Invalid Mode value */
            dbinit = NULL;
            hr = IDataInitialize_GetDataSource(datainit, NULL, CLSCTX_INPROC_SERVER, (WCHAR *)initstring_mode,
                &IID_IDBInitialize, (IUnknown **)&dbinit);
            ok(FAILED(hr), "got 0x%08lx\n", hr);

            dbinit = NULL;
            hr = IDataInitialize_GetDataSource(datainit, NULL, CLSCTX_INPROC_SERVER, (WCHAR *)initstring_mode2,
                &IID_IDBInitialize, (IUnknown **)&dbinit);
            ok(FAILED(hr), "got 0x%08lx\n", hr);

            dbinit = NULL;
            hr = IDataInitialize_GetDataSource(datainit, NULL, CLSCTX_INPROC_SERVER, (WCHAR *)initstring_mode3,
                &IID_IDBInitialize, (IUnknown **)&dbinit);
            ok(hr == S_OK, "got 0x%08lx\n", hr);
            IDBInitialize_Release(dbinit);

            dbinit = NULL;
            hr = IDataInitialize_GetDataSource(datainit, NULL, CLSCTX_INPROC_SERVER, (WCHAR *)initstring_mode4,
                &IID_IDBInitialize, (IUnknown **)&dbinit);
            ok(hr == S_OK, "got 0x%08lx\n", hr);
            IDBInitialize_Release(dbinit);

            dbinit = NULL;
            hr = IDataInitialize_GetDataSource(datainit, NULL, CLSCTX_INPROC_SERVER, (WCHAR *)initstring_mode5,
                &IID_IDBInitialize, (IUnknown **)&dbinit);
            ok(hr == S_OK, "got 0x%08lx\n", hr);
            IDBInitialize_Release(dbinit);
        }
        else
            ok(dbinit == NULL, "got %p\n", dbinit);

        /* Test quoting property values */
        expect_initstring(datainit, initstring_quote_semicolon, initstring_quote_semicolon);

        /* Test duplicate properties */
        expect_initstring(datainit, initstring_duplicate, initstring_msdasql);

        /* Test properties that should be skipped */
        expect_initstring(datainit, initstring_skip_properties, initstring_msdasql);

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
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = IRowPosition_QueryInterface(rowpos, &IID_IConnectionPointContainer, (void**)&cpc);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IConnectionPointContainer_EnumConnectionPoints(cpc, &enum_points);
    todo_wine
    ok(hr == S_OK, "got 0x%08lx\n", hr);
if (hr == S_OK) {
    hr = IEnumConnectionPoints_Next(enum_points, 1, &cp, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    hr = IConnectionPoint_GetConnectionInterface(cp, &iid);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(IsEqualIID(&iid, &IID_IRowPositionChange), "got %s\n", wine_dbgstr_guid(&iid));
    IConnectionPoint_Release(cp);

    hr = IEnumConnectionPoints_Next(enum_points, 1, &cp, NULL);
    ok(hr == S_FALSE, "got 0x%08lx\n", hr);

    IEnumConnectionPoints_Release(enum_points);
}
    IConnectionPointContainer_Release(cpc);
    IRowPosition_Release(rowpos);
}

typedef struct
{
    IRowset IRowset_iface;
    IChapteredRowset IChapteredRowset_iface;
} test_rset_t;

static test_rset_t test_rset;

static HRESULT WINAPI rset_QI(IRowset *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IRowset))
    {
        *obj = &test_rset.IRowset_iface;
        return S_OK;
    }
    else if (IsEqualIID(riid, &IID_IChapteredRowset))
    {
        *obj = &test_rset.IChapteredRowset_iface;
        return S_OK;
    }

    ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI rset_AddRef(IRowset *iface)
{
    return 2;
}

static ULONG WINAPI rset_Release(IRowset *iface)
{
    return 1;
}

static HRESULT WINAPI rset_AddRefRows(IRowset *iface, DBCOUNTITEM cRows,
    const HROW rghRows[], DBREFCOUNT rgRefCounts[], DBROWSTATUS rgRowStatus[])
{
    trace("AddRefRows: %Id\n", rghRows[0]);
    return S_OK;
}

static HRESULT WINAPI rset_GetData(IRowset *iface, HROW hRow, HACCESSOR hAccessor, void *pData)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI rset_GetNextRows(IRowset *iface, HCHAPTER hReserved, DBROWOFFSET lRowsOffset,
    DBROWCOUNT cRows, DBCOUNTITEM *pcRowObtained, HROW **prghRows)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI rset_ReleaseRows(IRowset *iface, DBCOUNTITEM cRows, const HROW rghRows[], DBROWOPTIONS rgRowOptions[],
    DBREFCOUNT rgRefCounts[], DBROWSTATUS rgRowStatus[])
{
    return S_OK;
}

static HRESULT WINAPI rset_RestartPosition(IRowset *iface, HCHAPTER hReserved)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IRowsetVtbl rset_vtbl = {
    rset_QI,
    rset_AddRef,
    rset_Release,
    rset_AddRefRows,
    rset_GetData,
    rset_GetNextRows,
    rset_ReleaseRows,
    rset_RestartPosition
};

static HRESULT WINAPI chrset_QI(IChapteredRowset *iface, REFIID riid, void **obj)
{
    return IRowset_QueryInterface(&test_rset.IRowset_iface, riid, obj);
}

static ULONG WINAPI chrset_AddRef(IChapteredRowset *iface)
{
    return IRowset_AddRef(&test_rset.IRowset_iface);
}

static ULONG WINAPI chrset_Release(IChapteredRowset *iface)
{
    return IRowset_Release(&test_rset.IRowset_iface);
}

static HRESULT WINAPI chrset_AddRefChapter(IChapteredRowset *iface, HCHAPTER chapter, DBREFCOUNT *refcount)
{
    return S_OK;
}

static HRESULT WINAPI chrset_ReleaseChapter(IChapteredRowset *iface, HCHAPTER chapter, DBREFCOUNT *refcount)
{
    return S_OK;
}

static const IChapteredRowsetVtbl chrset_vtbl = {
    chrset_QI,
    chrset_AddRef,
    chrset_Release,
    chrset_AddRefChapter,
    chrset_ReleaseChapter
};

static void init_test_rset(void)
{
    test_rset.IRowset_iface.lpVtbl = &rset_vtbl;
    test_rset.IChapteredRowset_iface.lpVtbl = &chrset_vtbl;
}

static void test_rowpos_initialize(void)
{
    IRowPosition *rowpos;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_OLEDB_ROWPOSITIONLIBRARY, NULL, CLSCTX_INPROC_SERVER, &IID_IRowPosition, (void**)&rowpos);
    ok(hr == S_OK, "got %08lx\n", hr);

    init_test_rset();
    hr = IRowPosition_Initialize(rowpos, (IUnknown*)&test_rset.IRowset_iface);
    ok(hr == S_OK, "got %08lx\n", hr);

    IRowPosition_Release(rowpos);
}

static HRESULT WINAPI onchange_QI(IRowPositionChange *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IRowPositionChange))
    {
        *obj = iface;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI onchange_AddRef(IRowPositionChange *iface)
{
    return 2;
}

static ULONG WINAPI onchange_Release(IRowPositionChange *iface)
{
    return 1;
}

static HRESULT WINAPI onchange_OnRowPositionChange(IRowPositionChange *iface, DBREASON reason,
    DBEVENTPHASE phase, BOOL cant_deny)
{
    trace("%ld %ld %d\n", reason, phase, cant_deny);
    return S_OK;
}

static const IRowPositionChangeVtbl onchange_vtbl = {
    onchange_QI,
    onchange_AddRef,
    onchange_Release,
    onchange_OnRowPositionChange
};

static IRowPositionChange onchangesink = { &onchange_vtbl };

static void init_onchange_sink(IRowPosition *rowpos)
{
    IConnectionPointContainer *cpc;
    IConnectionPoint *cp;
    DWORD cookie;
    HRESULT hr;

    hr = IRowPosition_QueryInterface(rowpos, &IID_IConnectionPointContainer, (void**)&cpc);
    ok(hr == S_OK, "got %08lx\n", hr);
    hr = IConnectionPointContainer_FindConnectionPoint(cpc, &IID_IRowPositionChange, &cp);
    ok(hr == S_OK, "got %08lx\n", hr);
    hr = IConnectionPoint_Advise(cp, (IUnknown*)&onchangesink, &cookie);
    ok(hr == S_OK, "got %08lx\n", hr);
    IConnectionPoint_Release(cp);
    IConnectionPointContainer_Release(cpc);
}

static void test_rowpos_clearrowposition(void)
{
    DBPOSITIONFLAGS flags;
    IRowPosition *rowpos;
    HCHAPTER chapter;
    IUnknown *unk;
    HRESULT hr;
    HROW row;

    hr = CoCreateInstance(&CLSID_OLEDB_ROWPOSITIONLIBRARY, NULL, CLSCTX_INPROC_SERVER, &IID_IRowPosition, (void**)&rowpos);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = IRowPosition_ClearRowPosition(rowpos);
    ok(hr == E_UNEXPECTED, "got %08lx\n", hr);

    hr = IRowPosition_GetRowset(rowpos, &IID_IStream, &unk);
    ok(hr == E_UNEXPECTED, "got %08lx\n", hr);

    chapter = 1;
    row = 1;
    flags = DBPOSITION_OK;
    hr = IRowPosition_GetRowPosition(rowpos, &chapter, &row, &flags);
    ok(hr == E_UNEXPECTED, "got %08lx\n", hr);
    ok(chapter == DB_NULL_HCHAPTER, "got %Id\n", chapter);
    ok(row == DB_NULL_HROW, "got %Id\n", row);
    ok(flags == DBPOSITION_NOROW, "got %ld\n", flags);

    init_test_rset();
    hr = IRowPosition_Initialize(rowpos, (IUnknown*)&test_rset.IRowset_iface);
    ok(hr == S_OK, "got %08lx\n", hr);

    chapter = 1;
    row = 1;
    flags = DBPOSITION_OK;
    hr = IRowPosition_GetRowPosition(rowpos, &chapter, &row, &flags);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(chapter == DB_NULL_HCHAPTER, "got %Id\n", chapter);
    ok(row == DB_NULL_HROW, "got %Id\n", row);
    ok(flags == DBPOSITION_NOROW, "got %ld\n", flags);

    hr = IRowPosition_GetRowset(rowpos, &IID_IRowset, &unk);
    ok(hr == S_OK, "got %08lx\n", hr);

    init_onchange_sink(rowpos);
    hr = IRowPosition_ClearRowPosition(rowpos);
    ok(hr == S_OK, "got %08lx\n", hr);

    chapter = 1;
    row = 1;
    flags = DBPOSITION_OK;
    hr = IRowPosition_GetRowPosition(rowpos, &chapter, &row, &flags);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(chapter == DB_NULL_HCHAPTER, "got %Id\n", chapter);
    ok(row == DB_NULL_HROW, "got %Id\n", row);
    ok(flags == DBPOSITION_NOROW, "got %ld\n", flags);

    IRowPosition_Release(rowpos);
}

static void test_rowpos_setrowposition(void)
{
    IRowPosition *rowpos;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_OLEDB_ROWPOSITIONLIBRARY, NULL, CLSCTX_INPROC_SERVER, &IID_IRowPosition, (void**)&rowpos);
    ok(hr == S_OK, "got %08lx\n", hr);

    init_test_rset();
    hr = IRowPosition_Initialize(rowpos, (IUnknown*)&test_rset.IRowset_iface);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = IRowPosition_ClearRowPosition(rowpos);
    ok(hr == S_OK, "got %08lx\n", hr);

    init_onchange_sink(rowpos);
    hr = IRowPosition_SetRowPosition(rowpos, 0x123, 0x456, DBPOSITION_OK);
    ok(hr == S_OK, "got %08lx\n", hr);

    IRowPosition_Release(rowpos);
}

static void test_dslocator(void)
{
    IDataSourceLocator *dslocator = NULL;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DataLinks, NULL, CLSCTX_INPROC_SERVER, &IID_IDataSourceLocator,(void**)&dslocator);
    ok(hr == S_OK, "got %08lx\n", hr);
    if(SUCCEEDED(hr))
    {
        IDataInitialize *datainit, *datainit2;
        IRunnableObject *runable;
        IProvideClassInfo *info;
        IMarshal *marshal;
        IRpcOptions *opts;
        COMPATIBLE_LONG hwnd = 0;

        if (0) /* Crashes under Window 7 */
            hr = IDataSourceLocator_get_hWnd(dslocator, NULL);

        hr = IDataSourceLocator_get_hWnd(dslocator, &hwnd);
        ok(hr == S_OK, "got %08lx\n", hr);
        ok(hwnd == 0, "got %p\n", (HWND)hwnd);

        hwnd = 0xDEADBEEF;
        hr = IDataSourceLocator_get_hWnd(dslocator, &hwnd);
        ok(hr == S_OK, "got %08lx\n", hr);
        ok(hwnd == 0, "got %p\n", (HWND)hwnd);

        hwnd = 0xDEADBEEF;
        hr = IDataSourceLocator_put_hWnd(dslocator, hwnd);
        ok(hr == S_OK, "got %08lx\n", hr);

        hwnd = 0;
        hr = IDataSourceLocator_get_hWnd(dslocator, &hwnd);
        ok(hr == S_OK, "got %08lx\n", hr);
        ok(hwnd == 0xDEADBEEF, "got %p\n", (HWND)hwnd);

        hwnd = 0;
        hr = IDataSourceLocator_put_hWnd(dslocator, hwnd);
        ok(hr == S_OK, "got %08lx\n", hr);

        hwnd = 0xDEADBEEF;
        hr = IDataSourceLocator_get_hWnd(dslocator, &hwnd);
        ok(hr == S_OK, "got %08lx\n", hr);
        ok(hwnd == 0, "got %p\n", (HWND)hwnd);

        hr = IDataSourceLocator_QueryInterface(dslocator, &IID_IDataInitialize, (void **)&datainit);
        ok(hr == S_OK, "got %08lx\n", hr);

        hr = IDataSourceLocator_QueryInterface(dslocator, &IID_IDataInitialize, (void **)&datainit2);
        ok(hr == S_OK, "got %08lx\n", hr);
        ok(datainit == datainit2, "Got %p, previous %p\n", datainit, datainit2);

        hr = IDataSourceLocator_QueryInterface(dslocator, &IID_IRunnableObject, (void **)&runable);
        ok(hr == E_NOINTERFACE, "got %08lx\n", hr);

        hr = IDataSourceLocator_QueryInterface(dslocator, &IID_IMarshal, (void **)&marshal);
        ok(hr == E_NOINTERFACE, "got %08lx\n", hr);

        hr = IDataSourceLocator_QueryInterface(dslocator, &IID_IProvideClassInfo, (void **)&info);
        ok(hr == E_NOINTERFACE, "got %08lx\n", hr);

        hr = IDataSourceLocator_QueryInterface(dslocator, &IID_IRpcOptions, (void **)&opts);
        ok(hr == E_NOINTERFACE, "got %08lx\n", hr);

        if (winetest_interactive)
        {
            IDispatch *disp = NULL;

            hr = IDataSourceLocator_PromptNew(dslocator, NULL);
            ok(hr == E_INVALIDARG, "got %08lx\n", hr);

            hr = IDataSourceLocator_PromptNew(dslocator, &disp);
            if (hr == S_OK)
            {
                ok(disp != NULL, "got %08lx\n", hr);
                IDispatch_Release(disp);
            }
            else
            {
                ok(hr == S_FALSE, "got %08lx\n", hr);
                ok(!disp, "got %08lx\n", hr);
            }
        }

        IDataInitialize_Release(datainit2);
        IDataInitialize_Release(datainit);

        IDataSourceLocator_Release(dslocator);
    }
}

static void test_odbc_enumerator(void)
{
    HRESULT hr;
    ISourcesRowset *source;
    IRowset *rowset;

    hr = CoCreateInstance( &CLSID_MSDASQL_ENUMERATOR, NULL, CLSCTX_ALL, &IID_ISourcesRowset, (void **)&source);
    ok(hr == S_OK, "Failed to create object 0x%08lx\n", hr);
    if (FAILED(hr))
    {
        return;
    }

    hr = ISourcesRowset_GetSourcesRowset(source, NULL, &IID_IRowset, 0, 0, (IUnknown**)&rowset);
    ok(hr == S_OK, "Failed to create object 0x%08lx\n", hr);
    if (hr == S_OK)
    {
        IAccessor *accessor;
        IRowsetInfo *info;

        hr = IRowset_QueryInterface(rowset, &IID_IAccessor, (void **)&accessor);
        ok(hr == S_OK, "got %08lx\n", hr);
        if (hr == S_OK)
            IAccessor_Release(accessor);

        hr = IRowset_QueryInterface(rowset, &IID_IRowsetInfo, (void **)&info);
        todo_wine ok(hr == S_OK, "got %08lx\n", hr);
        if (hr == S_OK)
            IRowsetInfo_Release(info);

        IRowset_Release(rowset);
    }

    ISourcesRowset_Release(source);
}

START_TEST(database)
{
    OleInitialize(NULL);

    test_database();
    test_errorinfo();
    test_initializationstring();
    test_dslocator();
    test_odbc_enumerator();

    /* row position */
    test_rowposition();
    test_rowpos_initialize();
    test_rowpos_clearrowposition();
    test_rowpos_setrowposition();

    OleUninitialize();
}
