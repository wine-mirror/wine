/*
 * Copyright 2014 Dmitry Timoshkov
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
#include "initguid.h"
#include "objbase.h"
#include "taskschd.h"

#include <wine/test.h>

static void test_Connect(void)
{
    static WCHAR empty[] = { 0 };
    static const WCHAR deadbeefW[] = { '0','.','0','.','0','.','0',0 };
    WCHAR comp_name[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD len;
    HRESULT hr;
    BSTR bstr;
    VARIANT v_null, v_comp;
    VARIANT_BOOL vbool;
    BOOL was_connected;
    ITaskService *service;

    hr = CoCreateInstance(&CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, &IID_ITaskService, (void **)&service);
    if (hr != S_OK)
    {
        win_skip("CoCreateInstance(CLSID_TaskScheduler) error %#x\n", hr);
        return;
    }

    hr = ITaskService_get_Connected(service, NULL);
    ok(hr == E_POINTER, "expected E_POINTER, got %#x\n", hr);

    vbool = 0xdead;
    hr = ITaskService_get_Connected(service, &vbool);
    ok(hr == S_OK, "get_Connected error %#x\n", hr);
    ok(vbool == VARIANT_FALSE, "expected VARIANT_FALSE, got %d\n", vbool);

    hr = ITaskService_get_TargetServer(service, NULL);
    ok(hr == E_POINTER, "expected E_POINTER, got %#x\n", hr);

    hr = ITaskService_get_TargetServer(service, &bstr);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ONLY_IF_CONNECTED), "expected ERROR_ONLY_IF_CONNECTED, got %#x\n", hr);

    /* Win7 doesn't support UNC \\ prefix, but according to a user
     * comment on MSDN Win8 supports both ways.
     */
    len = sizeof(comp_name)/sizeof(comp_name[0]);
    GetComputerNameW(comp_name, &len);

    V_VT(&v_null) = VT_NULL;

    V_VT(&v_comp) = VT_BSTR;
    V_BSTR(&v_comp) = SysAllocString(comp_name);

    hr = ITaskService_Connect(service, v_comp, v_null, v_null, v_null);
    ok(hr == S_OK || hr == E_ACCESSDENIED /* not an administrator */, "Connect error %#x\n", hr);
    was_connected = hr == S_OK;
    SysFreeString(V_BSTR(&v_comp));

    V_BSTR(&v_comp) = SysAllocString(deadbeefW);
    hr = ITaskService_Connect(service, v_comp, v_null, v_null, v_null);
todo_wine
    ok(hr == HRESULT_FROM_WIN32(RPC_S_INVALID_NET_ADDR), "expected RPC_S_INVALID_NET_ADDR, got %#x\n", hr);
    SysFreeString(V_BSTR(&v_comp));

    vbool = 0xdead;
    hr = ITaskService_get_Connected(service, &vbool);
    ok(hr == S_OK, "get_Connected error %#x\n", hr);
    ok(vbool == VARIANT_FALSE || (was_connected && vbool == VARIANT_TRUE),
       "Connect shouldn't trash an existing connection, got %d (was connected %d)\n", vbool, was_connected);

    V_BSTR(&v_comp) = SysAllocString(empty);
    hr = ITaskService_Connect(service, v_comp, v_null, v_null, v_null);
    ok(hr == S_OK, "Connect error %#x\n", hr);
    SysFreeString(V_BSTR(&v_comp));

    V_BSTR(&v_comp) = NULL;
    hr = ITaskService_Connect(service, v_comp, v_null, v_null, v_null);
    ok(hr == S_OK, "Connect error %#x\n", hr);

    hr = ITaskService_Connect(service, v_null, v_null, v_null, v_null);
    ok(hr == S_OK, "Connect error %#x\n", hr);

    vbool = 0xdead;
    hr = ITaskService_get_Connected(service, &vbool);
    ok(hr == S_OK, "get_Connected error %#x\n", hr);
    ok(vbool == VARIANT_TRUE, "expected VARIANT_TRUE, got %d\n", vbool);

    hr = ITaskService_get_TargetServer(service, &bstr);
    ok(hr == S_OK, "get_TargetServer error %#x\n", hr);
    ok(!lstrcmpW(comp_name, bstr), "compname %s != server name %s\n", wine_dbgstr_w(comp_name), wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    ITaskService_Release(service);
}

static void test_GetFolder(void)
{
    static WCHAR dot[] = { '.',0 };
    static WCHAR empty[] = { 0 };
    static WCHAR slash[] = { '/',0 };
    static WCHAR bslash[] = { '\\',0 };
    static WCHAR Wine[] = { '\\','W','i','n','e',0 };
    static WCHAR Wine_Folder1[] = { '\\','W','i','n','e','\\','F','o','l','d','e','r','1',0 };
    static WCHAR Wine_Folder1_[] = { '\\','W','i','n','e','\\','F','o','l','d','e','r','1','\\',0 };
    static WCHAR Wine_Folder1_Folder2[] = { '\\','W','i','n','e','\\','F','o','l','d','e','r','1','\\','F','o','l','d','e','r','2',0 };
    static WCHAR Folder1_Folder2[] = { '\\','F','o','l','d','e','r','1','\\','F','o','l','d','e','r','2',0 };
    static const WCHAR Folder1[] = { 'F','o','l','d','e','r','1',0 };
    static const WCHAR Folder2[] = { 'F','o','l','d','e','r','2',0 };
    HRESULT hr;
    BSTR bstr;
    VARIANT v_null;
    ITaskService *service;
    ITaskFolder *folder, *subfolder, *subfolder2;

    hr = CoCreateInstance(&CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, &IID_ITaskService, (void **)&service);
    if (hr != S_OK)
    {
        win_skip("CoCreateInstance(CLSID_TaskScheduler) error %#x\n", hr);
        return;
    }

    V_VT(&v_null) = VT_NULL;

    hr = ITaskService_Connect(service, v_null, v_null, v_null, v_null);
    ok(hr == S_OK, "Connect error %#x\n", hr);

    hr = ITaskService_GetFolder(service, slash, &folder);
todo_wine
    ok(hr == HRESULT_FROM_WIN32(ERROR_INVALID_NAME), "expected ERROR_INVALID_NAME, got %#x\n", hr);

    hr = ITaskService_GetFolder(service, dot, &folder);
todo_wine
    ok(hr == HRESULT_FROM_WIN32(ERROR_INVALID_NAME), "expected ERROR_INVALID_NAME, got %#x\n", hr);

    hr = ITaskService_GetFolder(service, bslash, &folder);
todo_wine
    ok(hr == S_OK, "GetFolder error %#x\n", hr);
    if (hr == S_OK)
        ITaskFolder_Release(folder);

    hr = ITaskService_GetFolder(service, NULL, NULL);
todo_wine
    ok(hr == E_POINTER, "expected E_POINTER, got %#x\n", hr);

    hr = ITaskService_GetFolder(service, empty, &folder);
todo_wine
    ok(hr == S_OK, "GetFolder error %#x\n", hr);
    if (hr == S_OK)
        ITaskFolder_Release(folder);

    hr = ITaskService_GetFolder(service, NULL, &folder);
todo_wine
    ok(hr == S_OK, "GetFolder error %#x\n", hr);
    if (hr != S_OK)
    {
        ITaskService_Release(service);
        return;
    }

    hr = ITaskFolder_get_Name(folder, NULL);
    ok(hr == E_POINTER, "expected E_POINTER, got %#x\n", hr);

    hr = ITaskFolder_get_Name(folder, &bstr);
    ok (hr == S_OK, "get_Name error %#x\n", hr);
    ok(!lstrcmpW(bstr, bslash), "expected '\\', got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    hr = ITaskFolder_get_Path(folder, NULL);
    ok(hr == E_POINTER, "expected E_POINTER, got %#x\n", hr);

    hr = ITaskFolder_get_Path(folder, &bstr);
    ok(hr == S_OK, "get_Path error %#x\n", hr);
    ok(!lstrcmpW(bstr, bslash), "expected '\\', got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    hr = ITaskFolder_CreateFolder(folder, NULL, v_null, &subfolder);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#x\n", hr);

    /* Just in case something was left from previous runs */
    ITaskFolder_DeleteFolder(folder, Wine_Folder1_Folder2, 0);
    ITaskFolder_DeleteFolder(folder, Wine_Folder1, 0);
    ITaskFolder_DeleteFolder(folder, Wine, 0);

    hr = ITaskFolder_CreateFolder(folder, slash, v_null, &subfolder);
    ok(hr == HRESULT_FROM_WIN32(ERROR_INVALID_NAME), "expected ERROR_INVALID_NAME, got %#x\n", hr);

    hr = ITaskService_GetFolder(service, Wine_Folder1_Folder2, &subfolder);
    ok(hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND), "expected ERROR_PATH_NOT_FOUND, got %#x\n", hr);

    hr = ITaskFolder_CreateFolder(folder, bslash, v_null, &subfolder);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#x\n", hr);

    hr = ITaskFolder_CreateFolder(folder, Wine_Folder1_Folder2, v_null, &subfolder);
    ok(hr == S_OK, "CreateFolder error %#x\n", hr);
    if (hr == S_OK)
        ITaskFolder_Release(subfolder);

    hr = ITaskFolder_CreateFolder(folder, Wine, v_null, NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), "expected ERROR_ALREADY_EXISTS, got %#x\n", hr);

    hr = ITaskFolder_CreateFolder(folder, Wine_Folder1_, v_null, &subfolder);
    ok(hr == HRESULT_FROM_WIN32(ERROR_INVALID_NAME), "expected ERROR_INVALID_NAME, got %#x\n", hr);

    hr = ITaskFolder_CreateFolder(folder, Wine, v_null, &subfolder);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), "expected ERROR_ALREADY_EXISTS, got %#x\n", hr);
    hr = ITaskFolder_CreateFolder(folder, Wine+1, v_null, &subfolder);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), "expected ERROR_ALREADY_EXISTS, got %#x\n", hr);
    hr = ITaskFolder_CreateFolder(folder, Wine_Folder1, v_null, &subfolder);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), "expected ERROR_ALREADY_EXISTS, got %#x\n", hr);
    hr = ITaskFolder_CreateFolder(folder, Wine_Folder1+1, v_null, &subfolder);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), "expected ERROR_ALREADY_EXISTS, got %#x\n", hr);
    hr = ITaskFolder_CreateFolder(folder, Wine_Folder1_Folder2, v_null, &subfolder);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), "expected ERROR_ALREADY_EXISTS, got %#x\n", hr);
    hr = ITaskFolder_CreateFolder(folder, Wine_Folder1_Folder2+1, v_null, &subfolder);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), "expected ERROR_ALREADY_EXISTS, got %#x\n", hr);

    hr = ITaskService_GetFolder(service, Wine_Folder1_Folder2, NULL);
    ok(hr == E_POINTER, "expected E_POINTER, got %#x\n", hr);

    hr = ITaskService_GetFolder(service, Wine_Folder1_Folder2, &subfolder);
    ok(hr == S_OK, "GetFolder error %#x\n", hr);
    if (hr != S_OK)
    {
        ITaskService_Release(service);
        return;
    }

    hr = ITaskFolder_get_Name(subfolder, &bstr);
    ok (hr == S_OK, "get_Name error %#x\n", hr);
    ok(!lstrcmpW(bstr, Folder2), "expected Folder2, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = ITaskFolder_get_Path(subfolder, &bstr);
    ok(hr == S_OK, "get_Path error %#x\n", hr);
    ok(!lstrcmpW(bstr, Wine_Folder1_Folder2), "expected \\Wine\\Folder1\\Folder2, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    ITaskFolder_Release(subfolder);

    hr = ITaskService_GetFolder(service, Wine_Folder1_Folder2+1, &subfolder);
    ok(hr == S_OK, "GetFolder error %#x\n", hr);
    hr = ITaskFolder_get_Name(subfolder, &bstr);
    ok (hr == S_OK, "get_Name error %#x\n", hr);
    ok(!lstrcmpW(bstr, Folder2), "expected Folder2, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = ITaskFolder_get_Path(subfolder, &bstr);
    ok(hr == S_OK, "get_Path error %#x\n", hr);
    ok(!lstrcmpW(bstr, Wine_Folder1_Folder2+1), "expected Wine\\Folder1\\Folder2, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    ITaskFolder_Release(subfolder);

    hr = ITaskService_GetFolder(service, Wine_Folder1, &subfolder);
    ok(hr == S_OK, "GetFolder error %#x\n", hr);
    hr = ITaskFolder_get_Name(subfolder, &bstr);
    ok (hr == S_OK, "get_Name error %#x\n", hr);
    ok(!lstrcmpW(bstr, Folder1), "expected Folder1, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = ITaskFolder_get_Path(subfolder, &bstr);
    ok(hr == S_OK, "get_Path error %#x\n", hr);
    ok(!lstrcmpW(bstr, Wine_Folder1), "expected \\Wine\\Folder1, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    ITaskFolder_Release(subfolder);

    hr = ITaskService_GetFolder(service, Wine, &subfolder);
    ok(hr == S_OK, "GetFolder error %#x\n", hr);
    hr = ITaskFolder_get_Name(subfolder, &bstr);
    ok (hr == S_OK, "get_Name error %#x\n", hr);
    ok(!lstrcmpW(bstr, Wine+1), "expected Wine, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = ITaskFolder_get_Path(subfolder, &bstr);
    ok(hr == S_OK, "get_Path error %#x\n", hr);
    ok(!lstrcmpW(bstr, Wine), "expected \\Wine, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    ITaskFolder_Release(subfolder);

    hr = ITaskService_GetFolder(service, Wine+1, &subfolder);
    ok(hr == S_OK, "GetFolder error %#x\n", hr);
    hr = ITaskFolder_get_Name(subfolder, &bstr);
    ok (hr == S_OK, "get_Name error %#x\n", hr);
    ok(!lstrcmpW(bstr, Wine+1), "expected Wine, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = ITaskFolder_get_Path(subfolder, &bstr);
    ok(hr == S_OK, "get_Path error %#x\n", hr);
    ok(!lstrcmpW(bstr, Wine+1), "expected Wine, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    hr = ITaskFolder_GetFolder(subfolder, bslash, &subfolder2);
    ok(hr == HRESULT_FROM_WIN32(ERROR_INVALID_NAME), "expected ERROR_INVALID_NAME, got %#x\n", hr);

    hr = ITaskFolder_GetFolder(subfolder, NULL, &subfolder2);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#x\n", hr);

    hr = ITaskFolder_GetFolder(subfolder, empty, &subfolder2);
    ok(hr == S_OK, "GetFolder error %#x\n", hr);
    hr = ITaskFolder_get_Name(subfolder2, &bstr);
    ok (hr == S_OK, "get_Name error %#x\n", hr);
    ok(!lstrcmpW(bstr, Wine+1), "expected Wine, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = ITaskFolder_get_Path(subfolder2, &bstr);
    ok(hr == S_OK, "get_Path error %#x\n", hr);
    ok(!lstrcmpW(bstr, Wine+1), "expected Wine, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    ITaskFolder_Release(subfolder2);

    hr = ITaskFolder_GetFolder(subfolder, Folder1_Folder2, &subfolder2);
    ok(hr == S_OK, "GetFolder error %#x\n", hr);
    hr = ITaskFolder_get_Name(subfolder2, &bstr);
    ok (hr == S_OK, "get_Name error %#x\n", hr);
    ok(!lstrcmpW(bstr, Folder2), "expected Folder2, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = ITaskFolder_get_Path(subfolder2, &bstr);
    ok(hr == S_OK, "get_Path error %#x\n", hr);
    ok(!lstrcmpW(bstr, Wine_Folder1_Folder2+1), "expected Wine\\Folder1\\Folder2, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    ITaskFolder_Release(subfolder2);

    hr = ITaskFolder_GetFolder(subfolder, Folder1_Folder2+1, &subfolder2);
    ok(hr == S_OK, "GetFolder error %#x\n", hr);

    hr = ITaskFolder_get_Name(subfolder2, &bstr);
    ok (hr == S_OK, "get_Name error %#x\n", hr);
    ok(!lstrcmpW(bstr, Folder2), "expected Folder2, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = ITaskFolder_get_Path(subfolder2, &bstr);
    ok(hr == S_OK, "get_Path error %#x\n", hr);
    ok(!lstrcmpW(bstr, Wine_Folder1_Folder2+1), "expected Wine\\Folder1\\Folder2, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    ITaskFolder_Release(subfolder2);

    ITaskFolder_Release(subfolder);

    hr = ITaskFolder_DeleteFolder(folder, Wine_Folder1_Folder2, 0);
    ok(hr == S_OK, "DeleteFolder error %#x\n", hr);
    ITaskFolder_DeleteFolder(folder, Wine_Folder1+1, 0);
    ok(hr == S_OK, "DeleteFolder error %#x\n", hr);
    hr = ITaskFolder_DeleteFolder(folder, Wine+1, 0);
    ok(hr == S_OK, "DeleteFolder error %#x\n", hr);

    hr = ITaskFolder_DeleteFolder(folder, Wine, 0);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "expected ERROR_FILE_NOT_FOUND, got %#x\n", hr);

    ITaskFolder_Release(folder);
    ITaskService_Release(service);
}

START_TEST(scheduler)
{
    OleInitialize(NULL);

    test_Connect();
    test_GetFolder();

    OleUninitialize();
}
