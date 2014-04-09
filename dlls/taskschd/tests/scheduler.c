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
#include <assert.h>

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
    ok(hr == HRESULT_FROM_WIN32(RPC_S_INVALID_NET_ADDR) || hr == HRESULT_FROM_WIN32(ERROR_BAD_NETPATH) /* VM */,
       "expected RPC_S_INVALID_NET_ADDR, got %#x\n", hr);
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

    hr = ITaskService_GetFolder(service, NULL, &folder);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ONLY_IF_CONNECTED), "expected ERROR_ONLY_IF_CONNECTED, got %#x\n", hr);

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
    ok(hr == S_OK, "GetFolder error %#x\n", hr);
    ITaskFolder_Release(folder);

    hr = ITaskService_GetFolder(service, NULL, NULL);
    ok(hr == E_POINTER, "expected E_POINTER, got %#x\n", hr);

    hr = ITaskService_GetFolder(service, empty, &folder);
    ok(hr == S_OK, "GetFolder error %#x\n", hr);
    ITaskFolder_Release(folder);

    hr = ITaskService_GetFolder(service, NULL, &folder);
    ok(hr == S_OK, "GetFolder error %#x\n", hr);

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
todo_wine
    ok(hr == HRESULT_FROM_WIN32(ERROR_INVALID_NAME), "expected ERROR_INVALID_NAME, got %#x\n", hr);

    hr = ITaskService_GetFolder(service, Wine_Folder1_Folder2, &subfolder);
    ok(hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND), "expected ERROR_PATH_NOT_FOUND, got %#x\n", hr);

    hr = ITaskFolder_CreateFolder(folder, bslash, v_null, &subfolder);
todo_wine
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#x\n", hr);

    hr = ITaskFolder_CreateFolder(folder, Wine_Folder1_Folder2, v_null, &subfolder);
    ok(hr == S_OK, "CreateFolder error %#x\n", hr);
    ITaskFolder_Release(subfolder);

    hr = ITaskFolder_CreateFolder(folder, Wine, v_null, NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), "expected ERROR_ALREADY_EXISTS, got %#x\n", hr);

    hr = ITaskFolder_CreateFolder(folder, Wine_Folder1_, v_null, &subfolder);
todo_wine
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
todo_wine
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

    hr = ITaskFolder_DeleteFolder(folder, Wine, 0);
    ok(hr == HRESULT_FROM_WIN32(ERROR_DIR_NOT_EMPTY), "expected ERROR_DIR_NOT_EMPTY, got %#x\n", hr);

    hr = ITaskFolder_DeleteFolder(folder, Wine_Folder1_Folder2, 0);
    ok(hr == S_OK, "DeleteFolder error %#x\n", hr);
    ITaskFolder_DeleteFolder(folder, Wine_Folder1+1, 0);
    ok(hr == S_OK, "DeleteFolder error %#x\n", hr);
    hr = ITaskFolder_DeleteFolder(folder, Wine+1, 0);
    ok(hr == S_OK, "DeleteFolder error %#x\n", hr);

    hr = ITaskFolder_DeleteFolder(folder, Wine, 0);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "expected ERROR_FILE_NOT_FOUND, got %#x\n", hr);

    hr = ITaskFolder_DeleteFolder(folder, NULL, 0);
    ok(hr == E_ACCESSDENIED || hr == E_INVALIDARG /* Vista */, "expected E_ACCESSDENIED, got %#x\n", hr);

    hr = ITaskFolder_DeleteFolder(folder, empty, 0);
    ok(hr == E_ACCESSDENIED || hr == E_INVALIDARG /* Vista */, "expected E_ACCESSDENIED, got %#x\n", hr);

    hr = ITaskFolder_DeleteFolder(folder, slash, 0);
todo_wine
    ok(hr == HRESULT_FROM_WIN32(ERROR_INVALID_NAME), "expected ERROR_INVALID_NAME, got %#x\n", hr);

    ITaskFolder_Release(folder);
    ITaskService_Release(service);
}

static void set_var(int vt, VARIANT *var, LONG val)
{
    V_VT(var) = vt;

    switch(vt)
    {
    case VT_I1:
    case VT_UI1:
        V_UI1(var) = (BYTE)val;
        break;

    case VT_I2:
    case VT_UI2:
        V_UI2(var) = (USHORT)val;
        break;

    case VT_I4:
    case VT_UI4:
        V_UI4(var) = val;
        break;

    case VT_I8:
    case VT_UI8:
        V_UI8(var) = val;
        break;

    case VT_INT:
    case VT_UINT:
        V_UINT(var) = val;
        break;

    default:
        assert(0);
        break;
    }
}

static void test_FolderCollection(void)
{
    static WCHAR Wine[] = { '\\','W','i','n','e',0 };
    static WCHAR Wine_Folder1[] = { '\\','W','i','n','e','\\','F','o','l','d','e','r','1',0 };
    static WCHAR Wine_Folder2[] = { '\\','W','i','n','e','\\','F','o','l','d','e','r','2',0 };
    static WCHAR Wine_Folder3[] = { '\\','W','i','n','e','\\','F','o','l','d','e','r','3',0 };
    static const WCHAR Folder1[] = { 'F','o','l','d','e','r','1',0 };
    static const WCHAR Folder2[] = { 'F','o','l','d','e','r','2',0 };
    HRESULT hr;
    BSTR bstr;
    VARIANT v_null, var[3];
    ITaskService *service;
    ITaskFolder *root, *folder, *subfolder;
    ITaskFolderCollection *folders;
    IUnknown *unknown;
    IEnumVARIANT *enumvar;
    ULONG count, i;
    VARIANT idx;
    static const int vt[] = { VT_I1, VT_I2, VT_I4, VT_I8, VT_UI1, VT_UI2, VT_UI4, VT_UI8, VT_INT, VT_UINT };

    hr = CoCreateInstance(&CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, &IID_ITaskService, (void **)&service);
    if (hr != S_OK)
    {
        win_skip("CoCreateInstance(CLSID_TaskScheduler) error %#x\n", hr);
        return;
    }

    V_VT(&v_null) = VT_NULL;

    hr = ITaskService_Connect(service, v_null, v_null, v_null, v_null);
    ok(hr == S_OK, "Connect error %#x\n", hr);

    hr = ITaskService_GetFolder(service, NULL, &root);
    ok(hr == S_OK, "GetFolder error %#x\n", hr);

    /* Just in case something was left from previous runs */
    ITaskFolder_DeleteFolder(root, Wine_Folder1, 0);
    ITaskFolder_DeleteFolder(root, Wine_Folder2, 0);
    ITaskFolder_DeleteFolder(root, Wine_Folder3, 0);
    ITaskFolder_DeleteFolder(root, Wine, 0);

    hr = ITaskFolder_CreateFolder(root, Wine_Folder1, v_null, &subfolder);
    ok(hr == S_OK, "CreateFolder error %#x\n", hr);
    ITaskFolder_Release(subfolder);

    hr = ITaskFolder_CreateFolder(root, Wine_Folder2, v_null, &subfolder);
    ok(hr == S_OK, "CreateFolder error %#x\n", hr);
    ITaskFolder_Release(subfolder);

    hr = ITaskFolder_GetFolder(root, Wine, &folder);
    ok(hr == S_OK, "GetFolder error %#x\n", hr);

    hr = ITaskFolder_GetFolders(folder, 0, NULL);
    ok (hr == E_POINTER, "expected E_POINTER, got %#x\n", hr);

    hr = ITaskFolder_GetFolders(folder, 0, &folders);
    ok(hr == S_OK, "GetFolders error %#x\n", hr);

    ITaskFolder_Release(folder);

    hr = ITaskFolderCollection_get_Count(folders, NULL);
    ok (hr == E_POINTER, "expected E_POINTER, got %#x\n", hr);

    count = 0;
    hr = ITaskFolderCollection_get_Count(folders, (LONG *)&count);
    ok(hr == S_OK, "get_Count error %#x\n", hr);
    ok(count == 2, "expected 2, got %d\n", count);

    hr = ITaskFolder_CreateFolder(root, Wine_Folder3, v_null, &subfolder);
    ok(hr == S_OK, "CreateFolder error %#x\n", hr);
    ITaskFolder_Release(subfolder);

    count = 0;
    hr = ITaskFolderCollection_get_Count(folders, (LONG *)&count);
    ok(hr == S_OK, "get_Count error %#x\n", hr);
    ok(count == 2, "expected 2, got %d\n", count);

    set_var(VT_INT, &idx, 0);
    hr = ITaskFolderCollection_get_Item(folders, idx, NULL);
    ok (hr == E_POINTER, "expected E_POINTER, got %#x\n", hr);

    for (i = 0; i < sizeof(vt)/sizeof(vt[0]); i++)
    {
        set_var(vt[i], &idx, 1);
        hr = ITaskFolderCollection_get_Item(folders, idx, &subfolder);
        ok(hr == S_OK, "get_Item(vt = %d) error %#x\n", vt[i], hr);
        ITaskFolder_Release(subfolder);
    }

    for (i = 0; i <= count; i++)
    {
        V_VT(&idx) = VT_I4;
        V_UI4(&idx) = i;
        hr = ITaskFolderCollection_get_Item(folders, idx, &subfolder);
        if (i == 0)
        {
            ok (hr == E_INVALIDARG, "expected E_INVALIDARG, got %#x\n", hr);
            continue;
        }
        ok(hr == S_OK, "get_Item error %#x\n", hr);

        hr = ITaskFolder_get_Path(subfolder, &bstr);
        ok(hr == S_OK, "get_Path error %#x\n", hr);
        if (i == 1)
            ok(!lstrcmpW(bstr, Wine_Folder1), "expected \\Wine\\Folder1, got %s\n", wine_dbgstr_w(bstr));
        else
            ok(!lstrcmpW(bstr, Wine_Folder2), "expected \\Wine\\Folder2, got %s\n", wine_dbgstr_w(bstr));
        SysFreeString(bstr);

        hr = ITaskFolder_get_Name(subfolder, &bstr);
        ok(hr == S_OK, "get_Name error %#x\n", hr);
        if (i == 1)
            ok(!lstrcmpW(bstr, Folder1), "expected Folder1, got %s\n", wine_dbgstr_w(bstr));
        else
            ok(!lstrcmpW(bstr, Folder2), "expected Folder2, got %s\n", wine_dbgstr_w(bstr));

        ITaskFolder_Release(subfolder);

        V_VT(&idx) = VT_BSTR;
        V_BSTR(&idx) = bstr;
        hr = ITaskFolderCollection_get_Item(folders, idx, &subfolder);
        ok(hr == S_OK, "get_Item error %#x\n", hr);
        SysFreeString(bstr);

        hr = ITaskFolder_get_Path(subfolder, &bstr);
        ok(hr == S_OK, "get_Path error %#x\n", hr);
        if (i == 1)
            ok(!lstrcmpW(bstr, Wine_Folder1), "expected \\Wine\\Folder1, got %s\n", wine_dbgstr_w(bstr));
        else
            ok(!lstrcmpW(bstr, Wine_Folder2), "expected \\Wine\\Folder2, got %s\n", wine_dbgstr_w(bstr));
        SysFreeString(bstr);

        hr = ITaskFolder_get_Name(subfolder, &bstr);
        ok(hr == S_OK, "get_Name error %#x\n", hr);
        if (i == 1)
            ok(!lstrcmpW(bstr, Folder1), "expected Folder1, got %s\n", wine_dbgstr_w(bstr));
        else
            ok(!lstrcmpW(bstr, Folder2), "expected Folder2, got %s\n", wine_dbgstr_w(bstr));

        ITaskFolder_Release(subfolder);
    }

    V_VT(&idx) = VT_I4;
    V_UI4(&idx) = 3;
    hr = ITaskFolderCollection_get_Item(folders, idx, &subfolder);
    ok (hr == E_INVALIDARG, "expected E_INVALIDARG, got %#x\n", hr);

    hr = ITaskFolderCollection_QueryInterface(folders, &IID_IEnumVARIANT, (void **)&enumvar);
    ok(hr == E_NOINTERFACE, "expected E_NOINTERFACE, got %#x\n", hr);
    hr = ITaskFolderCollection_QueryInterface(folders, &IID_IEnumUnknown, (void **)&enumvar);
    ok(hr == E_NOINTERFACE, "expected E_NOINTERFACE, got %#x\n", hr);

    hr = ITaskFolderCollection_get__NewEnum(folders, NULL);
    ok (hr == E_POINTER, "expected E_POINTER, got %#x\n", hr);

    hr = ITaskFolderCollection_get__NewEnum(folders, &unknown);
    ok(hr == S_OK, "get__NewEnum error %#x\n", hr);
    hr = IUnknown_QueryInterface(unknown, &IID_IEnumUnknown, (void **)&enumvar);
    ok(hr == E_NOINTERFACE, "expected E_NOINTERFACE, got %#x\n", hr);
    hr = IUnknown_QueryInterface(unknown, &IID_IEnumVARIANT, (void **)&enumvar);
    ok(hr == S_OK, "QueryInterface error %#x\n", hr);
    IEnumVARIANT_Release(enumvar);

    hr = IUnknown_QueryInterface(unknown, &IID_IUnknown, (void **)&enumvar);
    ok(hr == S_OK, "QueryInterface error %#x\n", hr);

    hr = IEnumVARIANT_Skip(enumvar, 0);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);
    hr = IEnumVARIANT_Skip(enumvar, 2);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);
    hr = IEnumVARIANT_Skip(enumvar, 1);
    ok(hr == S_FALSE, "expected S_FALSE, got %#x\n", hr);

    hr = IEnumVARIANT_Reset(enumvar);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);

    count = -1;
    hr = IEnumVARIANT_Next(enumvar, 1, NULL, &count);
    ok(hr == E_POINTER, "expected E_POINTER, got %#x\n", hr);

    memset(var, 0, sizeof(var));
    count = -1;
    hr = IEnumVARIANT_Next(enumvar, 1, var, &count);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);
    ok(count == 1, "expected 1, got %d\n", count);
    hr = ITaskFolder_get_Path((ITaskFolder *)V_DISPATCH(&var[0]), &bstr);
    ok(hr == S_OK, "get_Path error %#x\n", hr);
    ok(!lstrcmpW(bstr, Wine_Folder2), "expected \\Wine\\Folder2, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = ITaskFolder_get_Name((ITaskFolder *)V_DISPATCH(&var[0]), &bstr);
    ok(hr == S_OK, "get_Name error %#x\n", hr);
    ok(!lstrcmpW(bstr, Folder2), "expected Folder2, got %s\n", wine_dbgstr_w(bstr));
    IDispatch_Release(V_DISPATCH(&var[0]));

    memset(var, 0, sizeof(var));
    count = -1;
    hr = IEnumVARIANT_Next(enumvar, 1, var, &count);
    ok(hr == S_FALSE, "expected S_FALSE, got %#x\n", hr);
    ok(count == 0, "expected 0, got %d\n", count);

    count = -1;
    hr = IEnumVARIANT_Next(enumvar, 1, NULL, &count);
    ok(hr == S_FALSE, "expected S_FALSE, got %#x\n", hr);
    ok(count == 0, "expected 0, got %d\n", count);

    hr = IEnumVARIANT_Reset(enumvar);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);

    memset(var, 0, sizeof(var));
    count = -1;
    hr = IEnumVARIANT_Next(enumvar, 3, var, &count);
    ok(hr == S_FALSE, "expected S_FALSE, got %#x\n", hr);
    ok(count == 2, "expected 2, got %d\n", count);
    ok(V_VT(&var[0]) == VT_DISPATCH, "expected VT_DISPATCH, got %d\n", V_VT(&var[0]));
    ok(V_VT(&var[1]) == VT_DISPATCH, "expected VT_DISPATCH, got %d\n", V_VT(&var[1]));
    IEnumVARIANT_Release(enumvar);

    for (i = 0; i < count; i++)
    {
        hr = IDispatch_QueryInterface(V_DISPATCH(&var[i]), &IID_ITaskFolder, (void **)&subfolder);
        ok(hr == S_OK, "QueryInterface error %#x\n", hr);

        hr = ITaskFolder_get_Path(subfolder, &bstr);
        ok(hr == S_OK, "get_Path error %#x\n", hr);
        if (i == 0)
            ok(!lstrcmpW(bstr, Wine_Folder1), "expected \\Wine\\Folder1, got %s\n", wine_dbgstr_w(bstr));
        else
            ok(!lstrcmpW(bstr, Wine_Folder2), "expected \\Wine\\Folder2, got %s\n", wine_dbgstr_w(bstr));
        SysFreeString(bstr);

        hr = ITaskFolder_get_Name(subfolder, &bstr);
        ok(hr == S_OK, "get_Name error %#x\n", hr);
        if (i == 0)
            ok(!lstrcmpW(bstr, Folder1), "expected Folder1, got %s\n", wine_dbgstr_w(bstr));
        else
            ok(!lstrcmpW(bstr, Folder2), "expected Folder2, got %s\n", wine_dbgstr_w(bstr));

        ITaskFolder_Release(subfolder);
    }

    IDispatch_Release(V_DISPATCH(&var[0]));
    IDispatch_Release(V_DISPATCH(&var[1]));

    ITaskFolderCollection_Release(folders);

    hr = ITaskFolder_DeleteFolder(root, Wine_Folder1, 0);
    ok(hr == S_OK, "DeleteFolder error %#x\n", hr);
    hr = ITaskFolder_DeleteFolder(root, Wine_Folder2, 0);
    ok(hr == S_OK, "DeleteFolder error %#x\n", hr);
    hr = ITaskFolder_DeleteFolder(root, Wine_Folder3, 0);
    ok(hr == S_OK, "DeleteFolder error %#x\n", hr);
    hr = ITaskFolder_DeleteFolder(root, Wine, 0);
    ok(hr == S_OK, "DeleteFolder error %#x\n", hr);

    ITaskFolder_Release(root);
    ITaskService_Release(service);
}

static void test_GetTask(void)
{
    static WCHAR Wine[] = { '\\','W','i','n','e',0 };
    static WCHAR Wine_Task1[] = { '\\','W','i','n','e','\\','T','a','s','k','1',0 };
    static WCHAR Wine_Task2[] = { '\\','W','i','n','e','\\','T','a','s','k','2',0 };
    static WCHAR Task1[] = { 'T','a','s','k','1',0 };
    static WCHAR Task2[] = { 'T','a','s','k','2',0 };
    static const char xml1[] =
        "<?xml version=\"1.0\"?>\n"
        "<Task xmlns=\"http://schemas.microsoft.com/windows/2004/02/mit/task\">\n"
        "  <RegistrationInfo>\n"
        "    <Description>\"Task1\"</Description>\n"
        "  </RegistrationInfo>\n"
        "  <Settings>\n"
        "    <Enabled>false</Enabled>\n"
        "    <Hidden>false</Hidden>\n"
        "  </Settings>\n"
        "  <Actions>\n"
        "    <Exec>\n"
        "      <Command>\"task1.exe\"</Command>\n"
        "    </Exec>\n"
        "  </Actions>\n"
        "</Task>\n";
    static const char xml2[] =
        "<?xml version=\"1.0\"?>\n"
        "<Task xmlns=\"http://schemas.microsoft.com/windows/2004/02/mit/task\">\n"
        "  <RegistrationInfo>\n"
        "    <Description>\"Task2\"</Description>\n"
        "  </RegistrationInfo>\n"
        "  <Settings>\n"
        "    <Enabled>true</Enabled>\n"
        "    <Hidden>true</Hidden>\n"
        "  </Settings>\n"
        "  <Actions>\n"
        "    <Exec>\n"
        "      <Command>\"task2.exe\"</Command>\n"
        "    </Exec>\n"
        "  </Actions>\n"
        "</Task>\n";
    static const struct
    {
        DWORD flags, hr;
    } create_new_task[] =
    {
        { 0, S_OK },
        { TASK_CREATE, S_OK },
        { TASK_UPDATE, 0x80070002 /* HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) */ },
        { TASK_CREATE | TASK_UPDATE, S_OK }
    };
    static const struct
    {
        DWORD flags, hr;
    } open_existing_task[] =
    {
        { 0, 0x800700b7 /* HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) */ },
        { TASK_CREATE, 0x800700b7 /* HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) */ },
        { TASK_UPDATE, S_OK },
        { TASK_CREATE | TASK_UPDATE, S_OK }
    };
    WCHAR xmlW[sizeof(xml1)];
    HRESULT hr;
    BSTR bstr;
    TASK_STATE state;
    VARIANT_BOOL vbool;
    VARIANT v_null;
    ITaskService *service;
    ITaskFolder *root, *folder;
    IRegisteredTask *task1, *task2;
    IID iid;
    int i;

    hr = CoCreateInstance(&CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, &IID_ITaskService, (void **)&service);
    if (hr != S_OK)
    {
        win_skip("CoCreateInstance(CLSID_TaskScheduler) error %#x\n", hr);
        return;
    }

    V_VT(&v_null) = VT_NULL;

    hr = ITaskService_Connect(service, v_null, v_null, v_null, v_null);
    ok(hr == S_OK, "Connect error %#x\n", hr);

    hr = ITaskService_GetFolder(service, NULL, &root);
    ok(hr == S_OK, "GetFolder error %#x\n", hr);

    /* Just in case something was left from previous runs */
    ITaskFolder_DeleteTask(root, Wine_Task1, 0);
    ITaskFolder_DeleteTask(root, Wine_Task2, 0);
    ITaskFolder_DeleteFolder(root, Wine, 0);

    hr = ITaskFolder_GetTask(root, Wine_Task1, &task1);
    ok(hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND), "expected ERROR_PATH_NOT_FOUND, got %#x\n", hr);

    hr = ITaskFolder_CreateFolder(root, Wine, v_null, &folder);
    ok(hr == S_OK, "CreateFolder error %#x\n", hr);

    hr = ITaskFolder_GetTask(root, Wine, &task1);
    ok(hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND), "expected ERROR_PATH_NOT_FOUND, got %#x\n", hr);

    MultiByteToWideChar(CP_ACP, 0, xml1, -1, xmlW, sizeof(xmlW)/sizeof(xmlW[0]));

    for (i = 0; i < sizeof(create_new_task)/sizeof(create_new_task[0]); i++)
    {
        hr = ITaskFolder_RegisterTask(root, Wine_Task1, xmlW, create_new_task[i].flags, v_null, v_null, TASK_LOGON_NONE, v_null, &task1);
        ok(hr == create_new_task[i].hr, "%d: expected %#x, got %#x\n", i, create_new_task[i].hr, hr);
        if (hr == S_OK)
        {
            hr = ITaskFolder_DeleteTask(root, Wine_Task1, 0);
            ok(hr == S_OK, "DeleteTask error %#x\n", hr);
        }
    }

    hr = ITaskFolder_RegisterTask(root, Wine_Task1, NULL, TASK_CREATE, v_null, v_null, TASK_LOGON_NONE, v_null, NULL);
    ok(hr == HRESULT_FROM_WIN32(RPC_X_NULL_REF_POINTER), "expected RPC_X_NULL_REF_POINTER, got %#x\n", hr);

    hr = ITaskFolder_RegisterTask(root, Wine, xmlW, TASK_VALIDATE_ONLY, v_null, v_null, TASK_LOGON_NONE, v_null, NULL);
    ok(hr == S_OK, "RegisterTask error %#x\n", hr);

    hr = ITaskFolder_RegisterTask(root, Wine, xmlW, TASK_CREATE, v_null, v_null, TASK_LOGON_NONE, v_null, NULL);
todo_wine
    ok(hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) || broken(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS)) /* Vista */, "expected ERROR_ACCESS_DENIED, got %#x\n", hr);

    hr = ITaskFolder_RegisterTask(root, Wine_Task1, xmlW, TASK_CREATE, v_null, v_null, TASK_LOGON_NONE, v_null, NULL);
    ok(hr == S_OK, "RegisterTask error %#x\n", hr);

    hr = ITaskFolder_RegisterTask(root, Wine_Task1, xmlW, TASK_CREATE, v_null, v_null, TASK_LOGON_NONE, v_null, &task1);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), "expected ERROR_ALREADY_EXISTS, got %#x\n", hr);

    hr = ITaskFolder_RegisterTask(root, Wine_Task1, xmlW, 0, v_null, v_null, TASK_LOGON_NONE, v_null, NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), "expected ERROR_ALREADY_EXISTS, got %#x\n", hr);

    hr = ITaskFolder_RegisterTask(root, Wine_Task1, xmlW, TASK_CREATE_OR_UPDATE, v_null, v_null, TASK_LOGON_NONE, v_null, &task1);
    ok(hr == S_OK, "RegisterTask error %#x\n", hr);

    for (i = 0; i < sizeof(open_existing_task)/sizeof(open_existing_task[0]); i++)
    {
        hr = ITaskFolder_RegisterTask(root, Wine_Task1, xmlW, open_existing_task[i].flags, v_null, v_null, TASK_LOGON_NONE, v_null, &task1);
        ok(hr == open_existing_task[i].hr, "%d: expected %#x, got %#x\n", i, open_existing_task[i].hr, hr);
    }

    hr = IRegisteredTask_get_Name(task1, NULL);
    ok(hr == E_POINTER, "expected E_POINTER, got %#x\n", hr);

    hr = IRegisteredTask_get_Name(task1, &bstr);
    ok(hr == S_OK, "get_Name error %#x\n", hr);
    ok(!lstrcmpW(bstr, Task1), "expected Task1, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = IRegisteredTask_get_Path(task1, &bstr);
    ok(hr == S_OK, "get_Path error %#x\n", hr);
    ok(!lstrcmpW(bstr, Wine_Task1), "expected \\Wine\\Task1, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = IRegisteredTask_get_State(task1, &state);
todo_wine
    ok(hr == S_OK, "get_State error %#x\n", hr);
if (hr == S_OK)
    ok(state == TASK_STATE_DISABLED, "expected TASK_STATE_DISABLED, got %d\n", state);
    hr = IRegisteredTask_get_Enabled(task1, &vbool);
todo_wine
    ok(hr == S_OK, "get_Enabled error %#x\n", hr);
if (hr == S_OK)
    ok(vbool == VARIANT_FALSE, "expected VARIANT_FALSE, got %d\n", vbool);

    IRegisteredTask_Release(task1);

    hr = ITaskFolder_RegisterTask(folder, Task1, xmlW, TASK_CREATE, v_null, v_null, TASK_LOGON_NONE, v_null, &task2);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), "expected ERROR_ALREADY_EXISTS, got %#x\n", hr);

    MultiByteToWideChar(CP_ACP, 0, xml2, -1, xmlW, sizeof(xmlW)/sizeof(xmlW[0]));

    hr = ITaskFolder_RegisterTask(folder, Task2, xmlW, TASK_CREATE, v_null, v_null, TASK_LOGON_NONE, v_null, &task2);
    ok(hr == S_OK, "RegisterTask error %#x\n", hr);

    hr = IRegisteredTask_get_Name(task2, &bstr);
    ok(hr == S_OK, "get_Name error %#x\n", hr);
    ok(!lstrcmpW(bstr, Task2), "expected Task2, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = IRegisteredTask_get_Path(task2, &bstr);
    ok(hr == S_OK, "get_Path error %#x\n", hr);
    ok(!lstrcmpW(bstr, Wine_Task2), "expected \\Wine\\Task2, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = IRegisteredTask_get_State(task2, &state);
todo_wine
    ok(hr == S_OK, "get_State error %#x\n", hr);
if (hr == S_OK)
    ok(state == TASK_STATE_READY, "expected TASK_STATE_READY, got %d\n", state);
    hr = IRegisteredTask_get_Enabled(task2, &vbool);
todo_wine
    ok(hr == S_OK, "get_Enabled error %#x\n", hr);
if (hr == S_OK)
    ok(vbool == VARIANT_TRUE, "expected VARIANT_TRUE, got %d\n", vbool);

    IRegisteredTask_Release(task2);

    hr = ITaskFolder_GetTask(root, NULL, &task1);
todo_wine
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#x\n", hr);

    hr = ITaskFolder_GetTask(root, Wine_Task1, NULL);
    ok(hr == E_POINTER, "expected E_POINTER, got %#x\n", hr);

    hr = ITaskFolder_GetTask(root, Wine_Task1, &task1);
    ok(hr == S_OK, "GetTask error %#x\n", hr);

    hr = IRegisteredTask_get_Name(task1, &bstr);
    ok(hr == S_OK, "get_Name error %#x\n", hr);
    ok(!lstrcmpW(bstr, Task1), "expected Task1, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = IRegisteredTask_get_Path(task1, &bstr);
    ok(hr == S_OK, "get_Path error %#x\n", hr);
    ok(!lstrcmpW(bstr, Wine_Task1), "expected \\Wine\\Task1, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = IRegisteredTask_get_State(task1, &state);
todo_wine
    ok(hr == S_OK, "get_State error %#x\n", hr);
if (hr == S_OK)
    ok(state == TASK_STATE_DISABLED, "expected TASK_STATE_DISABLED, got %d\n", state);
    hr = IRegisteredTask_get_Enabled(task1, &vbool);
todo_wine
    ok(hr == S_OK, "get_Enabled error %#x\n", hr);
if (hr == S_OK)
    ok(vbool == VARIANT_FALSE, "expected VARIANT_FALSE, got %d\n", vbool);

    hr = IRegisteredTask_put_Enabled(task1, VARIANT_TRUE);
todo_wine
    ok(hr == S_OK, "put_Enabled error %#x\n", hr);
    hr = IRegisteredTask_get_State(task1, &state);
todo_wine
    ok(hr == S_OK, "get_State error %#x\n", hr);
if (hr == S_OK)
    ok(state == TASK_STATE_READY, "expected TASK_STATE_READY, got %d\n", state);
    hr = IRegisteredTask_get_Enabled(task1, &vbool);
todo_wine
    ok(hr == S_OK, "get_Enabled error %#x\n", hr);
if (hr == S_OK)
    ok(vbool == VARIANT_TRUE, "expected VARIANT_TRUE, got %d\n", vbool);

    IRegisteredTask_Release(task1);

    hr = ITaskFolder_GetTask(folder, Task2, &task2);
    ok(hr == S_OK, "GetTask error %#x\n", hr);

    hr = IRegisteredTask_get_Name(task2, &bstr);
    ok(hr == S_OK, "get_Name error %#x\n", hr);
    ok(!lstrcmpW(bstr, Task2), "expected Task2, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = IRegisteredTask_get_Path(task2, &bstr);
    ok(hr == S_OK, "get_Path error %#x\n", hr);
    ok(!lstrcmpW(bstr, Wine_Task2), "expected \\Wine\\Task2, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = IRegisteredTask_get_State(task2, &state);
todo_wine
    ok(hr == S_OK, "get_State error %#x\n", hr);
if (hr == S_OK)
    ok(state == TASK_STATE_READY, "expected TASK_STATE_READY, got %d\n", state);
    hr = IRegisteredTask_get_Enabled(task2, &vbool);
todo_wine
    ok(hr == S_OK, "get_Enabled error %#x\n", hr);
if (hr == S_OK)
    ok(vbool == VARIANT_TRUE, "expected VARIANT_TRUE, got %d\n", vbool);

    IRegisteredTask_Release(task2);

    hr = ITaskFolder_DeleteTask(folder, NULL, 0);
todo_wine
    ok(hr == HRESULT_FROM_WIN32(ERROR_DIR_NOT_EMPTY), "expected ERROR_DIR_NOT_EMPTY, got %#x\n", hr);

    hr = ITaskFolder_DeleteTask(root, Wine_Task1, 0);
    ok(hr == S_OK, "DeleteTask error %#x\n", hr);
    hr = ITaskFolder_DeleteTask(folder, Task2, 0);
    ok(hr == S_OK, "DeleteTask error %#x\n", hr);

    hr = ITaskFolder_DeleteTask(folder, Task2, 0);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "expected ERROR_FILE_NOT_FOUND, got %#x\n", hr);

    hr = ITaskFolder_RegisterTask(root, NULL, xmlW, TASK_CREATE, v_null, v_null, TASK_LOGON_NONE, v_null, &task1);
todo_wine
    ok(hr == S_OK, "RegisterTask error %#x\n", hr);
    /* FIXME: Remove once implemented */
    if (hr != S_OK) goto failed;
    hr = IRegisteredTask_get_Name(task1, &bstr);
    ok(hr == S_OK, "get_Name error %#x\n", hr);
    hr = IIDFromString(bstr, &iid);
    ok(hr == S_OK, "IIDFromString error %#x\n", hr);

    IRegisteredTask_Release(task1);

    hr = ITaskFolder_DeleteTask(root, bstr, 0);
    ok(hr == S_OK, "DeleteTask error %#x\n", hr);

failed:
    ITaskFolder_Release(folder);

    hr = ITaskFolder_DeleteFolder(root, Wine, 0);
    ok(hr == S_OK, "DeleteFolder error %#x\n", hr);

    ITaskFolder_Release(root);
    ITaskService_Release(service);
}

struct settings
{
    WCHAR restart_interval[20];
    WCHAR execution_time_limit[20];
    WCHAR delete_expired_task_after[20];
    int restart_count;
    int priority;
    TASK_INSTANCES_POLICY policy;
    TASK_COMPATIBILITY compatibility;
    VARIANT_BOOL allow_on_demand_start;
    VARIANT_BOOL stop_if_going_on_batteries;
    VARIANT_BOOL disallow_start_if_on_batteries;
    VARIANT_BOOL allow_hard_terminate;
    VARIANT_BOOL start_when_available;
    VARIANT_BOOL run_only_if_network_available;
    VARIANT_BOOL enabled;
    VARIANT_BOOL hidden;
    VARIANT_BOOL run_only_if_idle;
    VARIANT_BOOL wake_to_run;
};

/* test the version 1 compatibility settings */
static void test_settings_v1(ITaskDefinition *taskdef, struct settings *test, struct settings *def)
{
    HRESULT hr;
    ITaskSettings *set;
    int vint;
    VARIANT_BOOL vbool;
    BSTR bstr;
    TASK_INSTANCES_POLICY policy;
    TASK_COMPATIBILITY compat;

    hr = ITaskDefinition_get_Settings(taskdef, &set);
    ok(hr == S_OK, "get_Settings error %#x\n", hr);

    hr = ITaskSettings_get_AllowDemandStart(set, &vbool);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);
    ok(vbool == def->allow_on_demand_start, "expected %d, got %d\n", def->allow_on_demand_start, vbool);

    hr = ITaskSettings_get_RestartInterval(set, &bstr);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);
    if (!def->restart_interval[0])
        ok(bstr == NULL, "expected NULL, got %s\n", wine_dbgstr_w(bstr));
    else
        ok(!lstrcmpW(bstr, def->restart_interval), "expected %s, got %s\n", wine_dbgstr_w(def->restart_interval), wine_dbgstr_w(bstr));

    hr = ITaskSettings_get_RestartCount(set, &vint);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);
    ok(vint == def->restart_count, "expected %d, got %d\n", def->restart_count, vint);

    hr = ITaskSettings_get_MultipleInstances(set, &policy);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);
    ok(policy == def->policy, "expected %d, got %d\n", def->policy, policy);

    hr = ITaskSettings_get_StopIfGoingOnBatteries(set, &vbool);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);
    ok(vbool == test->stop_if_going_on_batteries, "expected %d, got %d\n", test->stop_if_going_on_batteries, vbool);

    hr = ITaskSettings_get_DisallowStartIfOnBatteries(set, &vbool);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);
    ok(vbool == test->disallow_start_if_on_batteries, "expected %d, got %d\n", test->disallow_start_if_on_batteries, vbool);

    hr = ITaskSettings_get_AllowHardTerminate(set, &vbool);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);
    ok(vbool == def->allow_hard_terminate, "expected %d, got %d\n", def->allow_hard_terminate, vbool);

    hr = ITaskSettings_get_StartWhenAvailable(set, &vbool);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);
    ok(vbool == def->start_when_available, "expected %d, got %d\n", def->start_when_available, vbool);

    hr = ITaskSettings_get_RunOnlyIfNetworkAvailable(set, &vbool);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);
    ok(vbool == def->run_only_if_network_available, "expected %d, got %d\n", def->run_only_if_network_available, vbool);

    hr = ITaskSettings_get_ExecutionTimeLimit(set, &bstr);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);
    if (!test->execution_time_limit[0])
        ok(bstr == NULL, "expected NULL, got %s\n", wine_dbgstr_w(bstr));
    else
        ok(!lstrcmpW(bstr, test->execution_time_limit), "expected %s, got %s\n", wine_dbgstr_w(test->execution_time_limit), wine_dbgstr_w(bstr));

    hr = ITaskSettings_get_Enabled(set, &vbool);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);
    ok(vbool == test->enabled, "expected %d, got %d\n", test->enabled, vbool);

    hr = ITaskSettings_get_DeleteExpiredTaskAfter(set, &bstr);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);
    if (!test->delete_expired_task_after[0])
        ok(bstr == NULL, "expected NULL, got %s\n", wine_dbgstr_w(bstr));
    else
        ok(!lstrcmpW(bstr, test->delete_expired_task_after), "expected %s, got %s\n", wine_dbgstr_w(test->delete_expired_task_after), wine_dbgstr_w(bstr));

    hr = ITaskSettings_get_Priority(set, &vint);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);
    ok(vint == test->priority, "expected %d, got %d\n", test->priority, vint);

    hr = ITaskSettings_get_Compatibility(set, &compat);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);
    ok(compat == test->compatibility, "expected %d, got %d\n", test->compatibility, compat);

    hr = ITaskSettings_get_Hidden(set, &vbool);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);
    ok(vbool == test->hidden, "expected %d, got %d\n", test->hidden, vbool);

    hr = ITaskSettings_get_RunOnlyIfIdle(set, &vbool);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);
    ok(vbool == test->run_only_if_idle, "expected %d, got %d\n", test->run_only_if_idle, vbool);

    hr = ITaskSettings_get_WakeToRun(set, &vbool);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);
    ok(vbool == test->wake_to_run, "expected %d, got %d\n", test->wake_to_run, vbool);

    /* FIXME: test IIdleSettings and INetworkSettings */

    ITaskSettings_Release(set);
}

static void change_settings(ITaskDefinition *taskdef, struct settings *test)
{
    HRESULT hr;
    ITaskSettings *set;

    hr = ITaskDefinition_get_Settings(taskdef, &set);
    ok(hr == S_OK, "get_Settings error %#x\n", hr);

    if (!test->restart_interval[0])
        hr = ITaskSettings_put_RestartInterval(set, NULL);
    else
        hr = ITaskSettings_put_RestartInterval(set, test->restart_interval);
todo_wine
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);
    /* FIXME: Remove once implemented */
    if (hr != S_OK) return;

    hr = ITaskSettings_put_RestartCount(set, test->restart_count);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);

    hr = ITaskSettings_put_MultipleInstances(set, test->policy);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);

    hr = ITaskSettings_put_StopIfGoingOnBatteries(set, test->stop_if_going_on_batteries);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);

    hr = ITaskSettings_put_DisallowStartIfOnBatteries(set, test->disallow_start_if_on_batteries);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);

    hr = ITaskSettings_put_AllowHardTerminate(set, test->allow_hard_terminate);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);

    hr = ITaskSettings_put_StartWhenAvailable(set, test->start_when_available);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);

    hr = ITaskSettings_put_RunOnlyIfNetworkAvailable(set, test->run_only_if_network_available);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);

    if (!test->execution_time_limit[0])
        hr = ITaskSettings_put_ExecutionTimeLimit(set, NULL);
    else
        hr = ITaskSettings_put_ExecutionTimeLimit(set, test->execution_time_limit);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);

    hr = ITaskSettings_put_Enabled(set, test->enabled);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);

    if (!test->delete_expired_task_after[0])
        hr = ITaskSettings_put_DeleteExpiredTaskAfter(set, NULL);
    else
        hr = ITaskSettings_put_DeleteExpiredTaskAfter(set, test->delete_expired_task_after);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);

    hr = ITaskSettings_put_Priority(set, test->priority);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);

    hr = ITaskSettings_put_Compatibility(set, test->compatibility);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);

    hr = ITaskSettings_put_Hidden(set, test->hidden);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);

    hr = ITaskSettings_put_RunOnlyIfIdle(set, test->run_only_if_idle);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);

    hr = ITaskSettings_put_WakeToRun(set, test->wake_to_run);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);

    hr = ITaskSettings_put_AllowDemandStart(set, test->allow_on_demand_start);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);

    /* FIXME: set IIdleSettings and INetworkSettings */

    ITaskSettings_Release(set);
}

static void create_action(ITaskDefinition *taskdef)
{
    static WCHAR task1_exe[] = { 't','a','s','k','1','.','e','x','e',0 };
    HRESULT hr;
    IActionCollection *actions;
    IAction *action;
    IExecAction *exec_action;

    hr = ITaskDefinition_get_Actions(taskdef, &actions);
todo_wine
    ok(hr == S_OK, "get_Actions error %#x\n", hr);
    /* FIXME: Remove once implemented */
    if (hr != S_OK) return;

    hr = IActionCollection_Create(actions, TASK_ACTION_EXEC, &action);
    ok(hr == S_OK, "Create action error %#x\n", hr);

    hr = IAction_QueryInterface(action, &IID_IExecAction, (void **)&exec_action);
    ok(hr == S_OK, "QueryInterface error %#x\n", hr);

    hr = IExecAction_put_Path(exec_action, task1_exe);
    ok(hr == S_OK, "put_Path error %#x\n", hr);

    IExecAction_Release(exec_action);
    IAction_Release(action);
    IActionCollection_Release(actions);
}

static void test_TaskDefinition(void)
{
    static const char xml1[] =
        "<Task xmlns=\"http://schemas.microsoft.com/windows/2004/02/mit/task\">\n"
        "  <RegistrationInfo>\n"
        "    <Description>\"Task1\"</Description>\n"
        "  </RegistrationInfo>\n"
        "  <Settings>\n"
        "    <Enabled>false</Enabled>\n"
        "    <Hidden>false</Hidden>\n"
        "  </Settings>\n"
        "  <Actions>\n"
        "    <Exec>\n"
        "      <Command>\"task1.exe\"</Command>\n"
        "    </Exec>\n"
        "  </Actions>\n"
        "</Task>\n";
    static const char xml2[] =
        "<Task>\n"
        "  <RegistrationInfo>\n"
        "    <Description>\"Task1\"</Description>\n"
        "  </RegistrationInfo>\n"
        "  <Settings>\n"
        "    <Enabled>false</Enabled>\n"
        "    <Hidden>false</Hidden>\n"
        "  </Settings>\n"
        "  <Actions>\n"
        "    <Exec>\n"
        "      <Command>\"task1.exe\"</Command>\n"
        "    </Exec>\n"
        "  </Actions>\n"
        "</Task>\n";
    static const char xml3[] =
        "<TASK>\n"
        "  <RegistrationInfo>\n"
        "    <Description>\"Task1\"</Description>\n"
        "  </RegistrationInfo>\n"
        "  <Settings>\n"
        "    <Enabled>false</Enabled>\n"
        "    <Hidden>false</Hidden>\n"
        "  </Settings>\n"
        "  <Actions>\n"
        "    <Exec>\n"
        "      <Command>\"task1.exe\"</Command>\n"
        "    </Exec>\n"
        "  </Actions>\n"
        "</TASK>\n";
    static const char xml4[] =
        "<Task xmlns=\"http://schemas.microsoft.com/windows/2004/02/mit/task\">\n"
        "  <RegistrationInfo/>\n"
        "  <Settings/>\n"
        "  <Actions>\n"
        "    <Exec>\n"
        "      <Command>\"task1.exe\"</Command>\n"
        "    </Exec>\n"
        "  </Actions>\n"
        "</Task>\n";
    static const char xml5[] =
        "<Task xmlns=\"http://schemas.microsoft.com/windows/2004/02/mit/task\">\n"
        "  <RegistrationInfo/>\n"
        "  <Settings/>\n"
        "  <Actions/>\n"
        "</Task>\n";
    static const char xml6[] =
        "<Task xmlns=\"http://schemas.microsoft.com/windows/2004/02/mit/task\">\n"
        "  <RegistrationInfo/>\n"
        "  <Actions>\n"
        "    <Exec>\n"
        "      <Command>\"task1.exe\"</Command>\n"
        "    </Exec>\n"
        "  </Actions>\n"
        "  <Settings>\n"
        "</Task>\n";
    static const char xml7[] =
        "<Task xmlns=\"http://schemas.microsoft.com/windows/2004/02/mit/task\">\n"
        "  <RegistrationInfo/>\n"
        "  <Settings>\n"
        "    <Enabled>FALSE</Enabled>\n"
        "  </Settings>\n"
        "  <Actions>\n"
        "    <Exec>\n"
        "      <Command>\"task1.exe\"</Command>\n"
        "    </Exec>\n"
        "  </Actions>\n"
        "</Task>\n";
    static struct settings def_settings = { { 0 }, { 'P','T','7','2','H',0 }, { 0 },
        0, 7, TASK_INSTANCES_IGNORE_NEW, TASK_COMPATIBILITY_V2, VARIANT_TRUE, VARIANT_TRUE,
        VARIANT_TRUE, VARIANT_TRUE, VARIANT_FALSE, VARIANT_FALSE, VARIANT_TRUE, VARIANT_FALSE,
        VARIANT_FALSE, VARIANT_FALSE };
    static struct settings new_settings = { { 'P','1','Y',0 }, { 'P','T','1','0','M',0 }, { 0 },
        100, 1, TASK_INSTANCES_STOP_EXISTING, TASK_COMPATIBILITY_V1, VARIANT_FALSE, VARIANT_FALSE,
        VARIANT_FALSE, VARIANT_FALSE, VARIANT_TRUE, VARIANT_TRUE, VARIANT_FALSE, VARIANT_TRUE,
        VARIANT_TRUE, VARIANT_TRUE };
    HRESULT hr;
    ITaskService *service;
    ITaskDefinition *taskdef;
    BSTR xml;
    WCHAR xmlW[sizeof(xml1)];

    hr = CoCreateInstance(&CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, &IID_ITaskService, (void **)&service);
    if (hr != S_OK)
    {
        win_skip("CoCreateInstance(CLSID_TaskScheduler) error %#x\n", hr);
        return;
    }

    hr = ITaskService_NewTask(service, 0, &taskdef);
    ok(hr == S_OK, "NewTask error %#x\n", hr);

    test_settings_v1(taskdef, &def_settings, &def_settings);
    change_settings(taskdef, &new_settings);

    /* Action is mandatory for v1 compatibility */
    create_action(taskdef);

    hr = ITaskDefinition_get_XmlText(taskdef, &xml);
    ok(hr == S_OK, "get_XmlText error %#x\n", hr);

    ITaskDefinition_Release(taskdef);

    hr = ITaskService_NewTask(service, 0, &taskdef);
    ok(hr == S_OK, "NewTask error %#x\n", hr);

    hr = ITaskDefinition_put_XmlText(taskdef, xml);
    ok(hr == S_OK, "put_XmlText error %#x\n", hr);
    SysFreeString(xml);

    /* FIXME: uncomment once changing settings is implemented
    test_settings_v1(taskdef, &new_settings, &def_settings);
    */

    hr = ITaskDefinition_put_XmlText(taskdef, NULL);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#x\n", hr);

    MultiByteToWideChar(CP_ACP, 0, xml1, -1, xmlW, sizeof(xmlW)/sizeof(xmlW[0]));
    hr = ITaskDefinition_put_XmlText(taskdef, xmlW);
    ok(hr == S_OK, "put_XmlText error %#x\n", hr);

    MultiByteToWideChar(CP_ACP, 0, xml2, -1, xmlW, sizeof(xmlW)/sizeof(xmlW[0]));
    hr = ITaskDefinition_put_XmlText(taskdef, xmlW);
    ok(hr == SCHED_E_NAMESPACE, "expected SCHED_E_NAMESPACE, got %#x\n", hr);

    MultiByteToWideChar(CP_ACP, 0, xml3, -1, xmlW, sizeof(xmlW)/sizeof(xmlW[0]));
    hr = ITaskDefinition_put_XmlText(taskdef, xmlW);
todo_wine
    ok(hr == SCHED_E_UNEXPECTEDNODE, "expected SCHED_E_UNEXPECTEDNODE, got %#x\n", hr);

    MultiByteToWideChar(CP_ACP, 0, xml4, -1, xmlW, sizeof(xmlW)/sizeof(xmlW[0]));
    hr = ITaskDefinition_put_XmlText(taskdef, xmlW);
    ok(hr == S_OK, "put_XmlText error %#x\n", hr);

    MultiByteToWideChar(CP_ACP, 0, xml5, -1, xmlW, sizeof(xmlW)/sizeof(xmlW[0]));
    hr = ITaskDefinition_put_XmlText(taskdef, xmlW);
todo_wine
    ok(hr == SCHED_E_MISSINGNODE, "expected SCHED_E_MISSINGNODE, got %#x\n", hr);

    MultiByteToWideChar(CP_ACP, 0, xml6, -1, xmlW, sizeof(xmlW)/sizeof(xmlW[0]));
    hr = ITaskDefinition_put_XmlText(taskdef, xmlW);
    ok(hr == SCHED_E_MALFORMEDXML, "expected SCHED_E_MALFORMEDXML, got %#x\n", hr);

    MultiByteToWideChar(CP_ACP, 0, xml7, -1, xmlW, sizeof(xmlW)/sizeof(xmlW[0]));
    hr = ITaskDefinition_put_XmlText(taskdef, xmlW);
    ok(hr == SCHED_E_INVALIDVALUE, "expected SCHED_E_INVALIDVALUE, got %#x\n", hr);

    xmlW[0] = 0;
    hr = ITaskDefinition_put_XmlText(taskdef, xmlW);
    ok(hr == SCHED_E_MALFORMEDXML, "expected SCHED_E_MALFORMEDXML, got %#x\n", hr);

    ITaskDefinition_Release(taskdef);
    ITaskService_Release(service);
}

START_TEST(scheduler)
{
    OleInitialize(NULL);

    test_Connect();
    test_GetFolder();
    test_FolderCollection();
    test_GetTask();
    test_TaskDefinition();

    OleUninitialize();
}
