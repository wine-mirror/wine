/*
 * Copyright 2019 Alistair Leslie-Hughes
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

#include <stdio.h>
#define COBJMACROS

#include "msdasc.h"
#include "oledb.h"
#include "odbcinst.h"
#include "wtypes.h"

#include "initguid.h"

#include "msdasql.h"
#include "oledberr.h"

#include "wine/test.h"

DEFINE_GUID(DBPROPSET_DATASOURCEINFO, 0xc8b522bb, 0x5cf3, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);
DEFINE_GUID(DBPROPSET_DBINITALL, 0xc8b522ca, 0x5cf3, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);
DEFINE_GUID(DBPROPSET_DBINIT,    0xc8b522bc, 0x5cf3, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);

DEFINE_GUID(DBGUID_DEFAULT,      0xc8b521fb, 0x5cf3, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);

static BOOL db_created;
static char mdbpath[MAX_PATH];

static const VARTYPE intptr_vartype = (sizeof(void *) == 8 ? VT_I8 : VT_I4);

static void test_msdasql(void)
{
    HRESULT hr;
    IUnknown *unk;
    IPersist *persist;
    CLSID classid;

    hr = CoCreateInstance( &CLSID_MSDASQL, NULL, CLSCTX_ALL, &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Failed to create object 0x%08x\n", hr);
    if (FAILED(hr))
    {
        return;
    }

    hr = IUnknown_QueryInterface(unk, &IID_IPersist, (void**)&persist);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IPersist_GetClassID(persist, &classid);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(IsEqualGUID(&classid, &CLSID_MSDASQL), "got %s\n", debugstr_guid(&classid));

    IPersist_Release(persist);

    IUnknown_Release(unk);
}

static void test_Properties(void)
{
    HRESULT hr;
    IDBProperties *props;
    DBPROPIDSET propidset;
    DBPROPID propid;
    ULONG infocount;
    DBPROPINFOSET *propinfoset;
    DBPROPIDSET propidlist;
    DBPROPSET *propset;
    WCHAR *desc;
    ULONG propcnt;
    ULONG i;
    DBPROPID properties[14] =
    {
        DBPROP_AUTH_PASSWORD, DBPROP_AUTH_PERSIST_SENSITIVE_AUTHINFO, DBPROP_AUTH_USERID,
        DBPROP_INIT_DATASOURCE, DBPROP_INIT_HWND, DBPROP_INIT_LOCATION,
        DBPROP_INIT_MODE, DBPROP_INIT_PROMPT, DBPROP_INIT_TIMEOUT,
        DBPROP_INIT_PROVIDERSTRING, DBPROP_INIT_LCID, DBPROP_INIT_CATALOG,
        DBPROP_INIT_OLEDBSERVICES, DBPROP_INIT_GENERALTIMEOUT
    };

    hr = CoCreateInstance( &CLSID_MSDASQL, NULL, CLSCTX_ALL, &IID_IDBProperties, (void **)&props);
    ok(hr == S_OK, "Failed to create object 0x%08x\n", hr);

    propidset.rgPropertyIDs = NULL;
    propidset.cPropertyIDs = 0;
    propidset.guidPropertySet = DBPROPSET_DBINITALL;

    infocount = 0;
    hr = IDBProperties_GetPropertyInfo(props, 1, &propidset, &infocount, &propinfoset, &desc);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    if (hr == S_OK)
    {
        VARTYPE types[14] = { VT_BSTR, VT_BOOL, VT_BSTR, VT_BSTR, intptr_vartype, VT_BSTR, VT_I4, VT_I2 , VT_I4, VT_BSTR, VT_I4, VT_BSTR, VT_I4, VT_I4 };

        ok(IsEqualGUID(&propinfoset->guidPropertySet, &DBPROPSET_DBINIT), "got %s\n", debugstr_guid(&propinfoset->guidPropertySet));
        ok(propinfoset->cPropertyInfos == 14, "got %d\n", propinfoset->cPropertyInfos);

        propidlist.guidPropertySet = DBPROPSET_DBINIT;
        propidlist.cPropertyIDs = propinfoset->cPropertyInfos;
        propidlist.rgPropertyIDs = CoTaskMemAlloc(propinfoset->cPropertyInfos * sizeof(DBPROP));

        for (i = 0; i < propinfoset->cPropertyInfos; i++)
        {
            ok(propinfoset->rgPropertyInfos[i].vtType == types[i], "got %d\n", propinfoset->rgPropertyInfos[i].vtType);
            ok(propinfoset->rgPropertyInfos[i].dwFlags == (DBPROPFLAGS_DBINIT | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE),
                "got %d\n", propinfoset->rgPropertyInfos[i].dwFlags);
            ok(properties[i] == propinfoset->rgPropertyInfos[i].dwPropertyID, "%d, got %d\n", i,
                    propinfoset->rgPropertyInfos[i].dwPropertyID);
            ok(propinfoset->rgPropertyInfos[i].vtType != VT_EMPTY, "%d, got %d\n", i,
                    propinfoset->rgPropertyInfos[i].vtType);

            propidlist.rgPropertyIDs[i] = propinfoset->rgPropertyInfos[i].dwPropertyID;
        }

        for (i = 0; i < propinfoset->cPropertyInfos; i++)
            VariantClear(&propinfoset->rgPropertyInfos[i].vValues);

        CoTaskMemFree(propinfoset->rgPropertyInfos);
        CoTaskMemFree(propinfoset);

        hr = IDBProperties_GetProperties(props, 1, &propidlist, &propcnt, &propset);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(propidlist.cPropertyIDs == 14, "got %d\n", propidlist.cPropertyIDs);
        ok(propset->cProperties == 14, "got %d\n", propset->cProperties);

        for (i = 0; i < propidlist.cPropertyIDs; i++)
        {
            VARTYPE vartype = VT_EMPTY;

            ok(properties[i] == propidlist.rgPropertyIDs[i], "%d, got %d\n", i, propidlist.rgPropertyIDs[i]);

            if(properties[i] == DBPROP_INIT_PROMPT)
            {
                ok(V_I2(&propset->rgProperties[i].vValue) == 4, "wrong value %s\n", debugstr_variant(&propset->rgProperties[i].vValue));
                vartype = VT_I2;
            }
            else if(properties[i] == DBPROP_INIT_LCID)
            {
                ok(V_I4(&propset->rgProperties[i].vValue) == GetUserDefaultLCID(), "wrong value %s\n", debugstr_variant(&propset->rgProperties[i].vValue));
                vartype = VT_I4;
            }
            else if(properties[i] == DBPROP_INIT_OLEDBSERVICES)
            {
                ok(V_I4(&propset->rgProperties[i].vValue) == -1, "wrong value %s\n", debugstr_variant(&propset->rgProperties[i].vValue));
                vartype = VT_I4;
            }

            ok(V_VT(&propset->rgProperties[i].vValue) == vartype, "%d wrong type %d\n", i, V_VT(&propset->rgProperties[i].vValue));
        }

        CoTaskMemFree(propidlist.rgPropertyIDs);
        CoTaskMemFree(propset);
    }

    propid = DBPROP_MULTIPLERESULTS;
    propidlist.rgPropertyIDs = &propid;
    propidlist.cPropertyIDs = 1;
    propidlist.guidPropertySet = DBPROPSET_DATASOURCEINFO;

    propcnt = 0;
    propset = NULL;
    hr = IDBProperties_GetProperties(props, 1, &propidlist, &propcnt, &propset);
    ok(hr == DB_E_ERRORSOCCURRED, "got 0x%08x\n", hr);
    ok(IsEqualGUID(&propset->guidPropertySet, &DBPROPSET_DATASOURCEINFO), "got %s\n", debugstr_guid(&propset->guidPropertySet));
    ok(propset->cProperties == 1, "got %d\n", propset->cProperties);
    ok(propset->rgProperties[0].dwPropertyID == DBPROP_MULTIPLERESULTS, "got %d\n", propset->rgProperties[0].dwPropertyID);
    ok(propset->rgProperties[0].dwStatus == DBPROPSTATUS_NOTSUPPORTED, "got %d\n", propset->rgProperties[0].dwStatus);

    CoTaskMemFree(propset);

    IDBProperties_Release(props);
}

static void test_command_interfaces(IUnknown *cmd)
{
    HRESULT hr;
    ICommandProperties *commandProp;
    ICommandText *command_text;
    IConvertType *convertype;
    ICommandPrepare *commandprepare;
    ICommandStream *commandstream;
    IColumnsInfo *colinfo;
    IMultipleResults *multiple;
    ICommandWithParameters *cmdwithparams;
    IUnknown *unk;

    hr = IUnknown_QueryInterface(cmd, &IID_ICommandProperties, (void**)&commandProp);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ICommandProperties_Release(commandProp);

    hr = IUnknown_QueryInterface(cmd, &IID_ICommandWithParameters, (void**)&cmdwithparams);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ICommandWithParameters_Release(cmdwithparams);

    hr = IUnknown_QueryInterface(cmd, &IID_ICommandText, (void**)&command_text);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ICommandText_Release(command_text);

    hr = IUnknown_QueryInterface(cmd, &IID_IConvertType, (void**)&convertype);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IConvertType_Release(convertype);

    hr = IUnknown_QueryInterface(cmd, &IID_ICommandPrepare, (void**)&commandprepare);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ICommandPrepare_Release(commandprepare);

    hr = IUnknown_QueryInterface(cmd, &IID_IColumnsInfo, (void**)&colinfo);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IColumnsInfo_Release(colinfo);

    hr = IUnknown_QueryInterface(cmd, &IID_ICommandStream, (void**)&commandstream);
    ok(hr == E_NOINTERFACE, "got 0x%08x\n", hr);

    hr = IUnknown_QueryInterface(cmd, &IID_IMultipleResults, (void**)&multiple);
    ok(hr == E_NOINTERFACE, "got 0x%08x\n", hr);

    hr = IUnknown_QueryInterface(cmd, &IID_IRowsetChange, (void**)&unk);
    ok(hr == E_NOINTERFACE, "got 0x%08x\n", hr);

    hr = IUnknown_QueryInterface(cmd, &IID_IRowsetUpdate, (void**)&unk);
    ok(hr == E_NOINTERFACE, "got 0x%08x\n", hr);

    hr = IUnknown_QueryInterface(cmd, &IID_IRowsetLocate, (void**)&unk);
    ok(hr == E_NOINTERFACE, "got 0x%08x\n", hr);
}

static void test_command_text(IUnknown *cmd)
{
    ICommandText *command_text;
    HRESULT hr;
    OLECHAR *str;
    GUID dialect;

    hr = IUnknown_QueryInterface(cmd, &IID_ICommandText, (void**)&command_text);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = ICommandText_GetCommandText(command_text, &dialect, &str);
    ok(hr == DB_E_NOCOMMAND, "got 0x%08x\n", hr);

if (0)
{
    /* Crashes under windows */
    hr = ICommandText_SetCommandText(command_text, NULL, L"select * from testing");
    ok(hr == S_OK, "got 0x%08x\n", hr);
}

    hr = ICommandText_SetCommandText(command_text, &DBGUID_DEFAULT, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = ICommandText_GetCommandText(command_text, &dialect, &str);
    ok(hr == DB_E_NOCOMMAND, "got 0x%08x\n", hr);

    hr = ICommandText_SetCommandText(command_text, &DBGUID_DEFAULT, L"select * from testing");
    ok(hr == S_OK, "got 0x%08x\n", hr);


    hr = ICommandText_GetCommandText(command_text, NULL, &str);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok (!lstrcmpW(L"select * from testing", str), "got %s\n", debugstr_w(str));
    HeapFree(GetProcessHeap(), 0, str);

    /* dialect empty value */
    hr = ICommandText_GetCommandText(command_text, &dialect, &str);
    ok(hr == DB_S_DIALECTIGNORED, "got 0x%08x\n", hr);
    ok(IsEqualGUID(&DBGUID_DEFAULT, &dialect), "got %s\n", debugstr_guid(&dialect));
    ok (!lstrcmpW(L"select * from testing", str), "got %s\n", debugstr_w(str));
    HeapFree(GetProcessHeap(), 0, str);

    dialect = DBGUID_DEFAULT;
    hr = ICommandText_GetCommandText(command_text, &dialect, &str);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(IsEqualGUID(&DBGUID_DEFAULT, &dialect), "got %s\n", debugstr_guid(&dialect));
    ok (!lstrcmpW(L"select * from testing", str), "got %s\n", debugstr_w(str));
    HeapFree(GetProcessHeap(), 0, str);

    ICommandText_Release(command_text);
}

static void test_command_dbsession(IUnknown *cmd, IUnknown *session)
{
    ICommandText *command_text;
    HRESULT hr;
    IUnknown *sess;

    hr = IUnknown_QueryInterface(cmd, &IID_ICommandText, (void**)&command_text);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = ICommandText_GetDBSession(command_text, &IID_IUnknown, &sess);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(session == sess, "different session pointers\n");

    ICommandText_Release(command_text);
}

static void test_rowset_interfaces(IRowset *rowset, ICommandText *commandtext)
{
    IRowsetInfo *info;
    IColumnsInfo *col_info;
    IColumnsRowset *col_rs;
    IAccessor *accessor;
    ICommandText *specification = NULL;
    IUnknown *unk;
    HRESULT hr;

    hr = IRowset_QueryInterface(rowset, &IID_IRowsetInfo, (void**)&info);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IRowsetInfo_GetSpecification(info, &IID_ICommandText, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IRowsetInfo_GetSpecification(info, &IID_ICommandText, (IUnknown**)&specification);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    if (specification)
    {
        ok(commandtext == specification, "got 0x%08x\n", hr);
        ICommandText_Release(specification);
    }
    IRowsetInfo_Release(info);

    hr = IRowset_QueryInterface(rowset, &IID_IColumnsInfo, (void**)&col_info);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IColumnsInfo_Release(col_info);

    hr = IRowset_QueryInterface(rowset, &IID_IAccessor, (void**)&accessor);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IAccessor_Release(accessor);

    hr = IRowset_QueryInterface(rowset, &IID_IColumnsRowset, (void**)&col_rs);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IColumnsRowset_Release(col_rs);

    hr = IRowset_QueryInterface(rowset, &IID_IRowsetChange, (void**)&unk);
    ok(hr == E_NOINTERFACE, "got 0x%08x\n", hr);

    hr = IRowset_QueryInterface(rowset, &IID_IRowsetUpdate, (void**)&unk);
    ok(hr == E_NOINTERFACE, "got 0x%08x\n", hr);

    hr = IRowset_QueryInterface(rowset, &IID_IRowsetLocate, (void**)&unk);
    ok(hr == E_NOINTERFACE, "got 0x%08x\n", hr);
}

static void test_command_rowset(IUnknown *cmd)
{
    ICommandText *command_text;
    ICommandPrepare *commandprepare;
    HRESULT hr;
    IUnknown *unk = NULL;
    IRowset *rowset;
    DBROWCOUNT affected;

    hr = IUnknown_QueryInterface(cmd, &IID_ICommandText, (void**)&command_text);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IUnknown_QueryInterface(cmd, &IID_ICommandPrepare, (void**)&commandprepare);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = ICommandText_SetCommandText(command_text, &DBGUID_DEFAULT, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = ICommandPrepare_Prepare(commandprepare, 1);
    ok(hr == DB_E_NOCOMMAND, "got 0x%08x\n", hr);

    hr = ICommandText_SetCommandText(command_text, &DBGUID_DEFAULT, L"CREATE TABLE testing (col1 INT, col2 SHORT)");
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = ICommandPrepare_Prepare(commandprepare, 1);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ICommandPrepare_Release(commandprepare);

    affected = 9999;
    hr = ICommandText_Execute(command_text, NULL, &IID_IRowset, NULL, &affected, &unk);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    todo_wine ok(unk == NULL, "Unexpected value\n");
    todo_wine ok(affected == -1, "got %ld\n", affected);
    if (unk)
        IUnknown_Release(unk);

    hr = ICommandText_SetCommandText(command_text, &DBGUID_DEFAULT, L"select * from testing");
    ok(hr == S_OK, "got 0x%08x\n", hr);

    affected = 9999;
    hr = ICommandText_Execute(command_text, NULL, &IID_IRowset, NULL, &affected, &unk);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(unk != NULL, "Unexpected value\n");
    if (hr == S_OK)
    {
        ok(affected == -1, "wrong affected value\n");

        hr = IUnknown_QueryInterface(unk, &IID_IRowset, (void**)&rowset);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        test_rowset_interfaces(rowset, command_text);

        IRowset_Release(rowset);
        IUnknown_Release(unk);
    }

    ICommandText_Release(command_text);
}


static void test_sessions(void)
{
    IDBProperties *props, *dsource = NULL;
    IDBInitialize *dbinit = NULL;
    IDataInitialize *datainit;
    IDBCreateSession *dbsession = NULL;
    IUnknown *session = NULL;
    IOpenRowset *openrowset = NULL;
    IDBCreateCommand *create_command = NULL;
    IGetDataSource *datasource = NULL;
    ISessionProperties *session_props = NULL;
    IUnknown *unimplemented = NULL;
    ITransaction *transaction = NULL;
    ITransactionLocal *local = NULL;
    ITransactionObject *object = NULL;
    ITransactionJoin *join = NULL;
    IUnknown *cmd = NULL;
    HRESULT hr;
    BSTR connect_str;

    if (!db_created)
    {
        skip("ODBC source wine_test not available.\n");
        return;
    }

    connect_str = SysAllocString(L"Provider=MSDASQL.1;Persist Security Info=False;Data Source=wine_test");

    hr = CoCreateInstance( &CLSID_MSDAINITIALIZE, NULL, CLSCTX_INPROC_SERVER, &IID_IDataInitialize,
                                (void **)&datainit );
    ok(hr == S_OK, "Failed to create object 0x%08x\n", hr);
    hr = IDataInitialize_GetDataSource( datainit, NULL, CLSCTX_INPROC_SERVER, connect_str, &IID_IDBInitialize,
                                             (IUnknown **)&dbinit );
    SysFreeString(connect_str);
    todo_wine ok(hr == S_OK, "got 0x%08x\n", hr);
    if(FAILED(hr))
    {
        IDataInitialize_Release( datainit );
        return;
    }

    hr = IDBInitialize_QueryInterface( dbinit, &IID_IDBProperties, (void **)&props );
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IDBProperties_Release(props);

    hr = IDBInitialize_Initialize( dbinit );
    ok(hr == S_OK, "got 0x%08x\n", hr);
    if(FAILED(hr))
    {
        IDBInitialize_Release( dbinit );
        IDataInitialize_Release( datainit );
        return;
    }

    hr = IDBInitialize_QueryInterface( dbinit, &IID_IDBCreateSession, (void **)&dbsession );
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDBCreateSession_CreateSession( dbsession, NULL, &IID_IUnknown, &session );
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IUnknown_QueryInterface(session, &IID_IGetDataSource, (void**)&datasource);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IGetDataSource_GetDataSource(datasource, &IID_IDBProperties, (IUnknown**)&dsource);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(dsource == props, "different pointers\n");
    IDBProperties_Release(dsource);
    IGetDataSource_Release(datasource);

    hr = IUnknown_QueryInterface(session, &IID_ITransaction, (void**)&transaction);
    todo_wine ok(hr == S_OK, "got 0x%08x\n", hr);
    if(hr == S_OK)
        ITransaction_Release(transaction);

    hr = IUnknown_QueryInterface(session, &IID_ITransactionLocal, (void**)&local);
    todo_wine ok(hr == S_OK, "got 0x%08x\n", hr);
    if(hr == S_OK)
        ITransactionLocal_Release(local);

    hr = IUnknown_QueryInterface(session, &IID_ITransactionObject, (void**)&object);
    ok(hr == E_NOINTERFACE, "got 0x%08x\n", hr);

    hr = IUnknown_QueryInterface(session, &IID_ITransactionJoin, (void**)&join);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ITransactionJoin_Release(join);

    hr = IUnknown_QueryInterface(session, &IID_IBindResource, (void**)&unimplemented);
    ok(hr == E_NOINTERFACE, "got 0x%08x\n", hr);

    hr = IUnknown_QueryInterface(session, &IID_ICreateRow, (void**)&unimplemented);
    ok(hr == E_NOINTERFACE, "got 0x%08x\n", hr);

    hr = IUnknown_QueryInterface(session, &IID_ISessionProperties, (void**)&session_props);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ISessionProperties_Release(session_props);

    hr = IUnknown_QueryInterface(session, &IID_IOpenRowset, (void**)&openrowset);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IOpenRowset_QueryInterface(openrowset, &IID_IDBCreateCommand, (void**)&create_command);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDBCreateCommand_CreateCommand(create_command, NULL, &IID_IUnknown, (IUnknown **)&cmd);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    if (hr == S_OK)
    {
        test_command_interfaces(cmd);
        test_command_text(cmd);
        test_command_dbsession(cmd, session);
        test_command_rowset(cmd);
        IUnknown_Release(cmd);
    }

    IDBCreateCommand_Release(create_command);
    IOpenRowset_Release(openrowset);
    IUnknown_Release( session );
    IDBCreateSession_Release(dbsession);
    IDBInitialize_Uninitialize( dbinit );
    IDBInitialize_Release( dbinit );
    IDataInitialize_Release( datainit );
}

static void setup_database(void)
{
    char *driver;
    DWORD code;
    char buffer[1024];
    WORD size;

    if (winetest_interactive)
    {
        trace("assuming odbc 'wine_test' is available\n");
        db_created = TRUE;
        return;
    }

    /*
     * 32 bit Windows has a default driver for "Microsoft Access Driver" (Windows 7+)
     *  and has the ability to create files on the fly.
     *
     * 64 bit Windows ONLY has a driver for "SQL Server", which we cannot use since we don't have a
     *   server to connect to.
     *
     * The filename passed to CREATE_DB must end in mdb.
     */
    GetTempPathA(sizeof(mdbpath), mdbpath);
    strcat(mdbpath, "wine_test.mdb");

    driver = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof("DSN=wine_test\0CREATE_DB=") + strlen(mdbpath) + 2);
    memcpy(driver, "DSN=wine_test\0CREATE_DB=", sizeof("DSN=wine_test\0CREATE_DB="));
    strcat(driver+sizeof("DSN=wine_test\0CREATE_DB=")-1, mdbpath);

    db_created = SQLConfigDataSource(NULL, ODBC_ADD_DSN, "Microsoft Access Driver (*.mdb)", driver);
    if (!db_created)
    {
        SQLInstallerError(1, &code, buffer, sizeof(buffer), &size);
        trace("code  %d, buffer %s, size %d\n", code, debugstr_a(buffer), size);

        HeapFree(GetProcessHeap(), 0, driver);

        return;
    }

    memcpy(driver, "DSN=wine_test\0DBQ=", sizeof("DSN=wine_test\0DBQ="));
    strcat(driver+sizeof("DSN=wine_test\0DBQ=")-1, mdbpath);
    db_created = SQLConfigDataSource(NULL, ODBC_ADD_DSN, "Microsoft Access Driver (*.mdb)", driver);

    HeapFree(GetProcessHeap(), 0, driver);
}

static void cleanup_database(void)
{
    BOOL ret;

    if (winetest_interactive)
        return;

    ret = SQLConfigDataSource(NULL, ODBC_REMOVE_DSN, "Microsoft Access Driver (*.mdb)", "DSN=wine_test\0\0");
    if (!ret)
    {
        DWORD code;
        char buffer[1024];
        WORD size;

        SQLInstallerError(1, &code, buffer, sizeof(buffer), &size);
        trace("code  %d, buffer %s, size %d\n", code, debugstr_a(buffer), size);
    }

    DeleteFileA(mdbpath);
}

static void test_enumeration(void)
{
    ISourcesRowset *source;
    IRowset *rowset, *rowset2;
    IColumnsInfo *columninfo;
    IAccessor *accessor;
    DBBINDING bindings = { 1, 0, 0, 512, NULL, NULL, NULL, DBPART_VALUE | DBPART_STATUS, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM, 514, 0, DBTYPE_WSTR, 0, 0 };
    HACCESSOR hacc;
    HRESULT hr;

    DBCOLUMNINFO colinfo_data[] =
    {
        { NULL,                           NULL, 0, DBCOLUMNFLAGS_ISFIXEDLENGTH | DBCOLUMNFLAGS_ISBOOKMARK, 4,  DBTYPE_UI4,  10, 255 },
        { (WCHAR*)L"SOURCES_NAME",        NULL, 1, DBCOLUMNFLAGS_MAYBENULL | DBCOLUMNFLAGS_WRITEUNKNOWN, 128, DBTYPE_WSTR, 255, 255 },
        { (WCHAR*)L"SOURCES_PARSENAME",   NULL, 2, DBCOLUMNFLAGS_MAYBENULL | DBCOLUMNFLAGS_WRITEUNKNOWN, 128, DBTYPE_WSTR, 255, 255 },
        { (WCHAR*)L"SOURCES_DESCRIPTION", NULL, 3, DBCOLUMNFLAGS_MAYBENULL | DBCOLUMNFLAGS_WRITEUNKNOWN, 128, DBTYPE_WSTR, 255, 255 },
        { (WCHAR*)L"SOURCES_TYPE",        NULL, 4, DBCOLUMNFLAGS_ISFIXEDLENGTH | DBCOLUMNFLAGS_MAYBENULL | DBCOLUMNFLAGS_WRITEUNKNOWN, 2, DBTYPE_UI2, 5, 255 },
        { (WCHAR*)L"SOURCES_ISPARENT",    NULL, 5, DBCOLUMNFLAGS_ISFIXEDLENGTH | DBCOLUMNFLAGS_MAYBENULL | DBCOLUMNFLAGS_WRITEUNKNOWN, 2, DBTYPE_BOOL, 255, 255 }
    };

    hr = CoCreateInstance( &CLSID_MSDASQL_ENUMERATOR, NULL, CLSCTX_INPROC_SERVER, &IID_ISourcesRowset,
                                (void **)&source );
    if ( FAILED(hr))
    {
        skip("Failed create Enumerator object\n");
        return;
    }

    hr = ISourcesRowset_GetSourcesRowset(source, NULL, &IID_IRowset, 0, NULL, (IUnknown**)&rowset);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = ISourcesRowset_GetSourcesRowset(source, NULL, &IID_IRowset, 0, NULL, (IUnknown**)&rowset2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(rowset != rowset2, "same pointer\n");
    IRowset_Release(rowset2);

    hr = IRowset_QueryInterface(rowset, &IID_IColumnsInfo, (void**)&columninfo);
    todo_wine ok(hr == S_OK, "got 0x%08x\n", hr);
    if (hr == S_OK)
    {
        DBORDINAL columns;
        DBCOLUMNINFO *dbcolumninfo;
        OLECHAR *buffer;
        int i;

        hr = IColumnsInfo_GetColumnInfo(columninfo, &columns, &dbcolumninfo, &buffer);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(columns == 6, "got %lu\n", columns);

        for( i = 0; i < columns; i++ )
        {
            if (!dbcolumninfo[i].pwszName || !colinfo_data[i].pwszName)
                ok (dbcolumninfo[i].pwszName == colinfo_data[i].pwszName, "got %p/%p", dbcolumninfo[i].pwszName, colinfo_data[i].pwszName);
            else
                ok ( !wcscmp(dbcolumninfo[i].pwszName, colinfo_data[i].pwszName), "got %p/%p",
                     debugstr_w(dbcolumninfo[i].pwszName), debugstr_w(colinfo_data[i].pwszName));

            ok (dbcolumninfo[i].pTypeInfo == colinfo_data[i].pTypeInfo, "got %p/%p", dbcolumninfo[i].pTypeInfo, colinfo_data[i].pTypeInfo);
            ok (dbcolumninfo[i].iOrdinal == colinfo_data[i].iOrdinal, "got %ld/%ld", dbcolumninfo[i].iOrdinal, colinfo_data[i].iOrdinal);
            ok (dbcolumninfo[i].dwFlags == colinfo_data[i].dwFlags, "got 0x%08x/0x%0x8", dbcolumninfo[i].dwFlags, colinfo_data[i].dwFlags);
            ok (dbcolumninfo[i].ulColumnSize == colinfo_data[i].ulColumnSize, "got %lu/%lu", dbcolumninfo[i].ulColumnSize, colinfo_data[i].ulColumnSize);
            ok (dbcolumninfo[i].wType == colinfo_data[i].wType, "got %d/%d", dbcolumninfo[i].wType, colinfo_data[i].wType);
            ok (dbcolumninfo[i].bPrecision == colinfo_data[i].bPrecision, "got %d/%d", dbcolumninfo[i].bPrecision, colinfo_data[i].bPrecision);
            ok (dbcolumninfo[i].bScale == colinfo_data[i].bScale, "got %d/%d", dbcolumninfo[i].bScale, colinfo_data[i].bScale);
        }

        CoTaskMemFree(buffer);
        IColumnsInfo_Release(columninfo);
    }

    hr = IRowset_QueryInterface(rowset, &IID_IAccessor, (void**)&accessor);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* Request only SOURCES_NAME column */
    hr = IAccessor_CreateAccessor(accessor, DBACCESSOR_ROWDATA, 1, &bindings, 0, &hacc, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(hacc != 0, "got %Ix\n", hacc);

    hr = IAccessor_ReleaseAccessor(accessor, hacc, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    IAccessor_Release(accessor);

    IRowset_Release(rowset);
    ISourcesRowset_Release(source);
}

START_TEST(provider)
{
    CoInitialize(0);

    setup_database();

    test_msdasql();
    test_Properties();
    test_sessions();

    test_enumeration();

    cleanup_database();

    CoUninitialize();
}
