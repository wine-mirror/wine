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
    len = ARRAY_SIZE(comp_name);
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
    ok(hr == HRESULT_FROM_WIN32(ERROR_INVALID_NAME) || hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) /* win7 */,
       "expected ERROR_INVALID_NAME, got %#x\n", hr);

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
    ok(hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) || hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) /* win7 */,
       "expected ERROR_PATH_NOT_FOUND, got %#x\n", hr);

    hr = ITaskFolder_CreateFolder(folder, bslash, v_null, &subfolder);
    todo_wine
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#x\n", hr);

    hr = ITaskFolder_CreateFolder(folder, Wine_Folder1_Folder2, v_null, &subfolder);
    ok(hr == S_OK, "CreateFolder error %#x\n", hr);
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

    hr = ITaskFolder_DeleteFolder(folder, Wine, 0);
    ok(hr == HRESULT_FROM_WIN32(ERROR_DIR_NOT_EMPTY), "expected ERROR_DIR_NOT_EMPTY, got %#x\n", hr);

    hr = ITaskFolder_DeleteFolder(folder, Wine_Folder1_Folder2, 0);
    ok(hr == S_OK, "DeleteFolder error %#x\n", hr);
    hr = ITaskFolder_DeleteFolder(folder, Wine_Folder1+1, 0);
    ok(hr == S_OK, "DeleteFolder error %#x\n", hr);
    hr = ITaskFolder_DeleteFolder(folder, Wine+1, 0);
    ok(hr == S_OK, "DeleteFolder error %#x\n", hr);

    hr = ITaskFolder_DeleteFolder(folder, Wine, 0);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) || hr == S_OK /* win7 */, "expected ERROR_FILE_NOT_FOUND, got %#x\n", hr);

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
    BOOL is_first;
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

    for (i = 0; i < ARRAY_SIZE(vt); i++)
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
        is_first = !lstrcmpW(bstr, Wine_Folder1);
        if (is_first)
            ok(!lstrcmpW(bstr, Wine_Folder1), "expected \\Wine\\Folder1, got %s\n", wine_dbgstr_w(bstr));
        else
            ok(!lstrcmpW(bstr, Wine_Folder2), "expected \\Wine\\Folder2, got %s\n", wine_dbgstr_w(bstr));
        SysFreeString(bstr);

        hr = ITaskFolder_get_Name(subfolder, &bstr);
        ok(hr == S_OK, "get_Name error %#x\n", hr);
        if (is_first)
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
        if (is_first)
            ok(!lstrcmpW(bstr, Wine_Folder1), "expected \\Wine\\Folder1, got %s\n", wine_dbgstr_w(bstr));
        else
            ok(!lstrcmpW(bstr, Wine_Folder2), "expected \\Wine\\Folder2, got %s\n", wine_dbgstr_w(bstr));
        SysFreeString(bstr);

        hr = ITaskFolder_get_Name(subfolder, &bstr);
        ok(hr == S_OK, "get_Name error %#x\n", hr);
        if (is_first)
            ok(!lstrcmpW(bstr, Folder1), "expected Folder1, got %s\n", wine_dbgstr_w(bstr));
        else
            ok(!lstrcmpW(bstr, Folder2), "expected Folder2, got %s\n", wine_dbgstr_w(bstr));
        SysFreeString(bstr);

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
    IUnknown_Release(unknown);

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
    is_first = !lstrcmpW(bstr, Wine_Folder1);
    if (is_first)
        ok(!lstrcmpW(bstr, Wine_Folder1), "expected \\Wine\\Folder1, got %s\n", wine_dbgstr_w(bstr));
    else
        ok(!lstrcmpW(bstr, Wine_Folder2), "expected \\Wine\\Folder2, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = ITaskFolder_get_Name((ITaskFolder *)V_DISPATCH(&var[0]), &bstr);
    ok(hr == S_OK, "get_Name error %#x\n", hr);
    if (is_first)
        ok(!lstrcmpW(bstr, Folder1), "expected Folder1, got %s\n", wine_dbgstr_w(bstr));
    else
        ok(!lstrcmpW(bstr, Folder2), "expected Folder2, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
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
        is_first = !lstrcmpW(bstr, Wine_Folder1);
        if (is_first)
            ok(!lstrcmpW(bstr, Wine_Folder1), "expected \\Wine\\Folder1, got %s\n", wine_dbgstr_w(bstr));
        else
            ok(!lstrcmpW(bstr, Wine_Folder2), "expected \\Wine\\Folder2, got %s\n", wine_dbgstr_w(bstr));
        SysFreeString(bstr);

        hr = ITaskFolder_get_Name(subfolder, &bstr);
        ok(hr == S_OK, "get_Name error %#x\n", hr);
        if (is_first)
            ok(!lstrcmpW(bstr, Folder1), "expected Folder1, got %s\n", wine_dbgstr_w(bstr));
        else
            ok(!lstrcmpW(bstr, Folder2), "expected Folder2, got %s\n", wine_dbgstr_w(bstr));
        SysFreeString(bstr);

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
    ok(hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) || hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) /* win7 */,
       "expected ERROR_PATH_NOT_FOUND, got %#x\n", hr);

    hr = ITaskFolder_CreateFolder(root, Wine, v_null, &folder);
    ok(hr == S_OK, "CreateFolder error %#x\n", hr);

    hr = ITaskFolder_GetTask(root, Wine, &task1);
    ok(hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) || hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) /* win7 */,
       "expected ERROR_PATH_NOT_FOUND, got %#x\n", hr);

    MultiByteToWideChar(CP_ACP, 0, xml1, -1, xmlW, ARRAY_SIZE(xmlW));

    for (i = 0; i < ARRAY_SIZE(create_new_task); i++)
    {
        hr = ITaskFolder_RegisterTask(root, Wine_Task1, xmlW, create_new_task[i].flags, v_null, v_null, TASK_LOGON_NONE, v_null, &task1);
        ok(hr == create_new_task[i].hr, "%d: expected %#x, got %#x\n", i, create_new_task[i].hr, hr);
        if (hr == S_OK)
        {
            hr = ITaskFolder_DeleteTask(root, Wine_Task1, 0);
            ok(hr == S_OK, "DeleteTask error %#x\n", hr);
            IRegisteredTask_Release(task1);
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

    for (i = 0; i < ARRAY_SIZE(open_existing_task); i++)
    {
        hr = ITaskFolder_RegisterTask(root, Wine_Task1, xmlW, open_existing_task[i].flags, v_null, v_null, TASK_LOGON_NONE, v_null, &task2);
        ok(hr == open_existing_task[i].hr, "%d: expected %#x, got %#x\n", i, open_existing_task[i].hr, hr);
        if (hr == S_OK)
            IRegisteredTask_Release(task2);
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
    ok(hr == S_OK, "get_State error %#x\n", hr);
    ok(state == TASK_STATE_DISABLED, "expected TASK_STATE_DISABLED, got %d\n", state);
    hr = IRegisteredTask_get_Enabled(task1, &vbool);
    ok(hr == S_OK, "get_Enabled error %#x\n", hr);
    ok(vbool == VARIANT_FALSE, "expected VARIANT_FALSE, got %d\n", vbool);

    IRegisteredTask_Release(task1);

    hr = ITaskFolder_RegisterTask(folder, Task1, xmlW, TASK_CREATE, v_null, v_null, TASK_LOGON_NONE, v_null, &task2);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), "expected ERROR_ALREADY_EXISTS, got %#x\n", hr);

    MultiByteToWideChar(CP_ACP, 0, xml2, -1, xmlW, ARRAY_SIZE(xmlW));

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
    ok(hr == S_OK, "get_State error %#x\n", hr);
    todo_wine
    ok(state == TASK_STATE_READY, "expected TASK_STATE_READY, got %d\n", state);
    hr = IRegisteredTask_get_Enabled(task2, &vbool);
    ok(hr == S_OK, "get_Enabled error %#x\n", hr);
    todo_wine
    ok(vbool == VARIANT_TRUE, "expected VARIANT_TRUE, got %d\n", vbool);

    IRegisteredTask_Release(task2);

    hr = ITaskFolder_GetTask(root, NULL, &task1);
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
    ok(hr == S_OK, "get_State error %#x\n", hr);
    ok(state == TASK_STATE_DISABLED, "expected TASK_STATE_DISABLED, got %d\n", state);
    hr = IRegisteredTask_get_Enabled(task1, &vbool);
    ok(hr == S_OK, "get_Enabled error %#x\n", hr);
    ok(vbool == VARIANT_FALSE, "expected VARIANT_FALSE, got %d\n", vbool);

    hr = IRegisteredTask_put_Enabled(task1, VARIANT_TRUE);
    ok(hr == S_OK, "put_Enabled error %#x\n", hr);
    hr = IRegisteredTask_get_State(task1, &state);
    ok(hr == S_OK, "get_State error %#x\n", hr);
    todo_wine
    ok(state == TASK_STATE_READY, "expected TASK_STATE_READY, got %d\n", state);
    hr = IRegisteredTask_get_Enabled(task1, &vbool);
    ok(hr == S_OK, "get_Enabled error %#x\n", hr);
    todo_wine
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
    ok(hr == S_OK, "get_State error %#x\n", hr);
    todo_wine
    ok(state == TASK_STATE_READY, "expected TASK_STATE_READY, got %d\n", state);
    hr = IRegisteredTask_get_Enabled(task2, &vbool);
    ok(hr == S_OK, "get_Enabled error %#x\n", hr);
    todo_wine
    ok(vbool == VARIANT_TRUE, "expected VARIANT_TRUE, got %d\n", vbool);

    hr = IRegisteredTask_get_State(task2, NULL);
    ok(hr == E_POINTER, "expected E_POINTER, got %#x\n", hr);
    hr = IRegisteredTask_get_Enabled(task2, NULL);
    ok(hr == E_POINTER, "expected E_POINTER, got %#x\n", hr);

    IRegisteredTask_Release(task2);

    hr = ITaskFolder_DeleteTask(folder, NULL, 0);
    todo_wine
    ok(hr == HRESULT_FROM_WIN32(ERROR_DIR_NOT_EMPTY), "expected ERROR_DIR_NOT_EMPTY, got %#x\n", hr);

    hr = ITaskFolder_DeleteTask(root, Wine_Task1, 0);
    ok(hr == S_OK, "DeleteTask error %#x\n", hr);
    hr = ITaskFolder_DeleteTask(folder, Task2, 0);
    ok(hr == S_OK, "DeleteTask error %#x\n", hr);

    hr = ITaskFolder_DeleteTask(folder, Task2, 0);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) || hr == S_OK /* win7 */, "expected ERROR_FILE_NOT_FOUND, got %#x\n", hr);

    hr = ITaskFolder_RegisterTask(root, NULL, xmlW, TASK_CREATE, v_null, v_null, TASK_LOGON_NONE, v_null, &task1);
    if(hr == E_ACCESSDENIED)
    {
        skip("Access denied\n");
        goto no_access;
    }
    ok(hr == S_OK, "RegisterTask error %#x\n", hr);

    hr = IRegisteredTask_get_Name(task1, &bstr);
    ok(hr == S_OK, "get_Name error %#x\n", hr);
    hr = IIDFromString(bstr, &iid);
    ok(hr == S_OK, "IIDFromString error %#x\n", hr);

    IRegisteredTask_Release(task1);

    hr = ITaskFolder_DeleteTask(root, bstr, 0);
    ok(hr == S_OK, "DeleteTask error %#x\n", hr);
    SysFreeString(bstr);

    hr = ITaskFolder_RegisterTask(folder, NULL, xmlW, TASK_CREATE, v_null, v_null, TASK_LOGON_NONE, v_null, &task1);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#x\n", hr);

no_access:
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
    {
        ok(!lstrcmpW(bstr, def->restart_interval), "expected %s, got %s\n", wine_dbgstr_w(def->restart_interval), wine_dbgstr_w(bstr));
        SysFreeString(bstr);
    }

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
    {
        ok(!lstrcmpW(bstr, test->execution_time_limit), "expected %s, got %s\n", wine_dbgstr_w(test->execution_time_limit), wine_dbgstr_w(bstr));
        SysFreeString(bstr);
    }

    hr = ITaskSettings_get_Enabled(set, &vbool);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);
    ok(vbool == test->enabled, "expected %d, got %d\n", test->enabled, vbool);

    hr = ITaskSettings_get_DeleteExpiredTaskAfter(set, &bstr);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);
    if (!test->delete_expired_task_after[0])
        ok(bstr == NULL, "expected NULL, got %s\n", wine_dbgstr_w(bstr));
    else
    {
        ok(!lstrcmpW(bstr, test->delete_expired_task_after), "expected %s, got %s\n", wine_dbgstr_w(test->delete_expired_task_after), wine_dbgstr_w(bstr));
        SysFreeString(bstr);
    }

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
    ITriggerCollection *triggers;
    IPrincipal *principal;
    IActionCollection *actions;

    hr = ITaskDefinition_get_Settings(taskdef, &set);
    ok(hr == S_OK, "get_Settings error %#x\n", hr);

    if (!test->restart_interval[0])
        hr = ITaskSettings_put_RestartInterval(set, NULL);
    else
        hr = ITaskSettings_put_RestartInterval(set, test->restart_interval);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);

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

    triggers = NULL;
    hr = ITaskDefinition_get_Triggers(taskdef, &triggers);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);
    ok(triggers != NULL, "triggers not set\n");

    hr = ITaskDefinition_put_Triggers(taskdef, triggers);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);
    if (triggers) ITriggerCollection_Release(triggers);

    principal = NULL;
    hr = ITaskDefinition_get_Principal(taskdef, &principal);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);
    ok(principal != NULL, "principal not set\n");

    hr = ITaskDefinition_put_Principal(taskdef, principal);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);
    if (principal) IPrincipal_Release(principal);

    actions = NULL;
    hr = ITaskDefinition_get_Actions(taskdef, &actions);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);
    ok(actions != NULL, "actions not set\n");

    hr = ITaskDefinition_put_Actions(taskdef, actions);
    ok(hr == S_OK, "expected S_OK, got %#x\n", hr);
    if (actions) IActionCollection_Release(actions);

    /* FIXME: set IIdleSettings and INetworkSettings */

    ITaskSettings_Release(set);
}

static void test_daily_trigger(ITrigger *trigger)
{
    static const WCHAR startW[] =
        {'2','0','0','4','-','0','1','-','0','1','T','0','0',':','0','0',':','0','0',0};
    static const WCHAR start2W[] =
        {'2','0','0','4','-','0','1','-','0','1','T','0','0',':','0','0',':','0','0','Z',0};
    static const WCHAR start3W[] =
        {'2','0','0','4','-','0','1','-','0','1','T','0','0',':','0','0',':','0','0','+','0','1',':','0','0',0};
    static const WCHAR start4W[] =
        {'2','0','0','4','.','0','1','.','0','1','T','0','0','.','0','0','.','0','0',0};
    static const WCHAR start5W[] =
        {'9','9','9','9','-','9','9','-','9','9','T','9','9',':','9','9',':','9','9',0};
    static const WCHAR start6W[] =
        {'i','n','v','a','l','i','d',0};
    static const struct
    {
        const WCHAR *str;
        HRESULT      hr;
    }
    start_test[] =
    {
        {startW, S_OK},
        {start2W, S_OK},
        {start3W, S_OK},
        {start4W, S_OK},
        {start5W, S_OK},
        {start6W, S_OK},
    };
    IDailyTrigger *daily_trigger;
    BSTR start_boundary;
    VARIANT_BOOL enabled;
    short interval;
    HRESULT hr;
    ULONG i;

    hr = ITrigger_QueryInterface(trigger, &IID_IDailyTrigger, (void**)&daily_trigger);
    ok(hr == S_OK, "Could not get IDailyTrigger iface: %08x\n", hr);

    interval = -1;
    hr = IDailyTrigger_get_DaysInterval(daily_trigger, &interval);
    ok(hr == S_OK, "get_DaysInterval failed: %08x\n", hr);
    ok(interval == 1, "interval = %d\n", interval);

    hr = IDailyTrigger_put_DaysInterval(daily_trigger, -2);
    ok(hr == E_INVALIDARG, "put_DaysInterval failed: %08x\n", hr);
    hr = IDailyTrigger_put_DaysInterval(daily_trigger, 0);
    ok(hr == E_INVALIDARG, "put_DaysInterval failed: %08x\n", hr);

    interval = -1;
    hr = IDailyTrigger_get_DaysInterval(daily_trigger, &interval);
    ok(hr == S_OK, "get_DaysInterval failed: %08x\n", hr);
    ok(interval == 1, "interval = %d\n", interval);

    hr = IDailyTrigger_put_DaysInterval(daily_trigger, 2);
    ok(hr == S_OK, "put_DaysInterval failed: %08x\n", hr);

    interval = -1;
    hr = IDailyTrigger_get_DaysInterval(daily_trigger, &interval);
    ok(hr == S_OK, "get_DaysInterval failed: %08x\n", hr);
    ok(interval == 2, "interval = %d\n", interval);

    hr = IDailyTrigger_get_StartBoundary(daily_trigger, NULL);
    ok(hr == E_POINTER, "get_StartBoundary failed: %08x\n", hr);

    start_boundary = (BSTR)0xdeadbeef;
    hr = IDailyTrigger_get_StartBoundary(daily_trigger, &start_boundary);
    ok(hr == S_OK, "get_StartBoundary failed: %08x\n", hr);
    ok(start_boundary == NULL, "start_boundary not set\n");

    for (i = 0; i < ARRAY_SIZE(start_test); i++)
    {
        start_boundary = SysAllocString(start_test[i].str);
        hr = IDailyTrigger_put_StartBoundary(daily_trigger, start_boundary);
        ok(hr == start_test[i].hr, "%u: got %08x expected %08x\n", i, hr, start_test[i].hr);
        SysFreeString(start_boundary);
        if (hr == S_OK)
        {
            start_boundary = NULL;
            hr = IDailyTrigger_get_StartBoundary(daily_trigger, &start_boundary);
            ok(hr == S_OK, "%u: got %08x\n", i, hr);
            ok(start_boundary != NULL, "start_boundary not set\n");
            ok(!lstrcmpW(start_boundary, start_test[i].str), "%u: got %s\n", i, wine_dbgstr_w(start_boundary));
            SysFreeString(start_boundary);
        }
    }

    hr = IDailyTrigger_put_StartBoundary(daily_trigger, NULL);
    ok(hr == S_OK, "put_StartBoundary failed: %08x\n", hr);

    hr = IDailyTrigger_get_Enabled(daily_trigger, NULL);
    ok(hr == E_POINTER, "get_Enabled failed: %08x\n", hr);

    enabled = VARIANT_FALSE;
    hr = IDailyTrigger_get_Enabled(daily_trigger, &enabled);
    ok(hr == S_OK, "get_Enabled failed: %08x\n", hr);
    ok(enabled == VARIANT_TRUE, "got %d\n", enabled);

    hr = IDailyTrigger_put_Enabled(daily_trigger, VARIANT_FALSE);
    ok(hr == S_OK, "put_Enabled failed: %08x\n", hr);

    enabled = VARIANT_TRUE;
    hr = IDailyTrigger_get_Enabled(daily_trigger, &enabled);
    ok(hr == S_OK, "get_Enabled failed: %08x\n", hr);
    ok(enabled == VARIANT_FALSE, "got %d\n", enabled);

    IDailyTrigger_Release(daily_trigger);
}

static void create_action(ITaskDefinition *taskdef)
{
    static WCHAR task1_exe[] = { 't','a','s','k','1','.','e','x','e',0 };
    static WCHAR workdir[] = { 'w','o','r','k','d','i','r',0 };
    static WCHAR args[] = { 'a','r','g','u','m','e','n','s',0 };
    static WCHAR comment[] = { 'c','o','m','m','e','n','t',0 };
    HRESULT hr;
    IActionCollection *actions;
    IAction *action;
    IExecAction *exec_action;
    TASK_ACTION_TYPE type;
    BSTR path, str;

    hr = ITaskDefinition_get_Actions(taskdef, NULL);
    ok(hr == E_POINTER, "got %#x\n", hr);

    hr = ITaskDefinition_get_Actions(taskdef, &actions);
    ok(hr == S_OK, "get_Actions error %#x\n", hr);

    hr = IActionCollection_Create(actions, TASK_ACTION_EXEC, &action);
    ok(hr == S_OK, "Create action error %#x\n", hr);

    hr = IAction_QueryInterface(action, &IID_IExecAction, (void **)&exec_action);
    ok(hr == S_OK, "QueryInterface error %#x\n", hr);

    type = 0xdeadbeef;
    hr = IExecAction_get_Type(exec_action, &type);
    ok(hr == S_OK, "get_Type error %#x\n", hr);
    ok(type == TASK_ACTION_EXEC, "got %u\n", type );

    hr = IExecAction_get_Path(exec_action, NULL);
    ok(hr == E_POINTER, "got %#x\n", hr);

    path = (BSTR)0xdeadbeef;
    hr = IExecAction_get_Path(exec_action, &path);
    ok(hr == S_OK, "get_Path error %#x\n", hr);
    ok(path == NULL, "path not set\n");

    hr = IExecAction_put_Path(exec_action, NULL);
    ok(hr == S_OK, "put_Path error %#x\n", hr);

    hr = IExecAction_put_Path(exec_action, task1_exe);
    ok(hr == S_OK, "put_Path error %#x\n", hr);

    path = NULL;
    hr = IExecAction_get_Path(exec_action, &path);
    ok(hr == S_OK, "get_Path error %#x\n", hr);
    ok(path != NULL, "path not set\n");
    ok(!lstrcmpW(path, task1_exe), "got %s\n", wine_dbgstr_w(path));
    SysFreeString(path);

    hr = IExecAction_get_WorkingDirectory(exec_action, NULL);
    ok(hr == E_POINTER, "got %#x\n", hr);

    path = (BSTR)0xdeadbeef;
    hr = IExecAction_get_WorkingDirectory(exec_action, &path);
    ok(hr == S_OK, "get_WorkingDirectory error %#x\n", hr);
    ok(path == NULL, "workdir not set\n");

    hr = IExecAction_put_WorkingDirectory(exec_action, NULL);
    ok(hr == S_OK, "put_WorkingDirectory error %#x\n", hr);

    hr = IExecAction_put_WorkingDirectory(exec_action, workdir);
    ok(hr == S_OK, "put_WorkingDirectory error %#x\n", hr);

    path = NULL;
    hr = IExecAction_get_WorkingDirectory(exec_action, &path);
    ok(hr == S_OK, "get_WorkingDirectory error %#x\n", hr);
    ok(path != NULL, "workdir not set\n");
    ok(!lstrcmpW(path, workdir), "got %s\n", wine_dbgstr_w(path));
    SysFreeString(path);

    hr = IExecAction_get_Arguments(exec_action, NULL);
    ok(hr == E_POINTER, "got %#x\n", hr);

    path = (BSTR)0xdeadbeef;
    hr = IExecAction_get_Arguments(exec_action, &path);
    ok(hr == S_OK, "get_Arguments error %#x\n", hr);
    ok(path == NULL, "args not set\n");

    hr = IExecAction_put_Arguments(exec_action, NULL);
    ok(hr == S_OK, "put_Arguments error %#x\n", hr);

    hr = IExecAction_put_Arguments(exec_action, args);
    ok(hr == S_OK, "put_Arguments error %#x\n", hr);

    path = NULL;
    hr = IExecAction_get_Arguments(exec_action, &path);
    ok(hr == S_OK, "get_Arguments error %#x\n", hr);
    ok(path != NULL, "args not set\n");
    ok(!lstrcmpW(path, args), "got %s\n", wine_dbgstr_w(path));
    SysFreeString(path);


    str = (BSTR)0xdeadbeef;
    hr = IExecAction_get_Id(exec_action, &str);
    ok(hr == S_OK, "get_Id error %#x\n", hr);
    ok(str == NULL, "id should be NULL\n");

    hr = IExecAction_put_Id(exec_action, NULL);
    ok(hr == S_OK, "put_Id error %#x\n", hr);

    hr = IExecAction_put_Id(exec_action, comment);
    ok(hr == S_OK, "put_Id error %#x\n", hr);

    str = NULL;
    hr = IExecAction_get_Id(exec_action, &str);
    ok(hr == S_OK, "get_Id error %#x\n", hr);
    ok(str != NULL, "should not be NULL\n");
    ok(!lstrcmpW(str, comment), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

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
        "    <Author>author</Author>\n"
        "    <Version>1.0</Version>\n"
        "    <Date>2018-04-02T11:22:33</Date>\n"
        "    <Documentation>doc</Documentation>\n"
        "    <URI>uri</URI>\n"
        "    <Source>source</Source>\n"
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
    static const WCHAR authorW[] = { 'a','u','t','h','o','r',0 };
    static const WCHAR versionW[] = { '1','.','0',0 };
    static const WCHAR dateW[] = { '2','0','1','8','-','0','4','-','0','2','T','1','1',':','2','2',':','3','3',0 };
    static const WCHAR docW[] = { 'd','o','c',0 };
    static const WCHAR uriW[] = { 'u','r','i',0 };
    static const WCHAR sourceW[] = { 's','o','u','r','c','e',0 };
    static WCHAR Task1[] = { '"','T','a','s','k','1','"',0 };
    static struct settings def_settings = { { 0 }, { 'P','T','7','2','H',0 }, { 0 },
        0, 7, TASK_INSTANCES_IGNORE_NEW, TASK_COMPATIBILITY_V2, VARIANT_TRUE, VARIANT_TRUE,
        VARIANT_TRUE, VARIANT_TRUE, VARIANT_FALSE, VARIANT_FALSE, VARIANT_TRUE, VARIANT_FALSE,
        VARIANT_FALSE, VARIANT_FALSE };
    static struct settings new_settings = { { 'P','1','Y',0 }, { 'P','T','1','0','M',0 }, { 0 },
        100, 1, TASK_INSTANCES_STOP_EXISTING, TASK_COMPATIBILITY_V1, VARIANT_FALSE, VARIANT_FALSE,
        VARIANT_FALSE, VARIANT_FALSE, VARIANT_TRUE, VARIANT_TRUE, VARIANT_FALSE, VARIANT_TRUE,
        VARIANT_TRUE, VARIANT_TRUE };
    ITriggerCollection *trigger_col, *trigger_col2;
    HRESULT hr;
    ITaskService *service;
    ITaskDefinition *taskdef;
    IRegistrationInfo *reginfo;
    ITrigger *trigger;
    BSTR xml, bstr;
    VARIANT var;
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

    MultiByteToWideChar(CP_ACP, 0, xml1, -1, xmlW, ARRAY_SIZE(xmlW));
    hr = ITaskDefinition_put_XmlText(taskdef, xmlW);
    ok(hr == S_OK, "put_XmlText error %#x\n", hr);

    MultiByteToWideChar(CP_ACP, 0, xml2, -1, xmlW, ARRAY_SIZE(xmlW));
    hr = ITaskDefinition_put_XmlText(taskdef, xmlW);
    ok(hr == SCHED_E_NAMESPACE, "expected SCHED_E_NAMESPACE, got %#x\n", hr);

    MultiByteToWideChar(CP_ACP, 0, xml3, -1, xmlW, ARRAY_SIZE(xmlW));
    hr = ITaskDefinition_put_XmlText(taskdef, xmlW);
    todo_wine
    ok(hr == SCHED_E_UNEXPECTEDNODE, "expected SCHED_E_UNEXPECTEDNODE, got %#x\n", hr);

    MultiByteToWideChar(CP_ACP, 0, xml4, -1, xmlW, ARRAY_SIZE(xmlW));
    hr = ITaskDefinition_put_XmlText(taskdef, xmlW);
    ok(hr == S_OK, "put_XmlText error %#x\n", hr);

    MultiByteToWideChar(CP_ACP, 0, xml5, -1, xmlW, ARRAY_SIZE(xmlW));
    hr = ITaskDefinition_put_XmlText(taskdef, xmlW);
    todo_wine
    ok(hr == SCHED_E_MISSINGNODE, "expected SCHED_E_MISSINGNODE, got %#x\n", hr);

    MultiByteToWideChar(CP_ACP, 0, xml6, -1, xmlW, ARRAY_SIZE(xmlW));
    hr = ITaskDefinition_put_XmlText(taskdef, xmlW);
    ok(hr == SCHED_E_MALFORMEDXML, "expected SCHED_E_MALFORMEDXML, got %#x\n", hr);

    MultiByteToWideChar(CP_ACP, 0, xml7, -1, xmlW, ARRAY_SIZE(xmlW));
    hr = ITaskDefinition_put_XmlText(taskdef, xmlW);
    ok(hr == SCHED_E_INVALIDVALUE, "expected SCHED_E_INVALIDVALUE, got %#x\n", hr);

    xmlW[0] = 0;
    hr = ITaskDefinition_put_XmlText(taskdef, xmlW);
    ok(hr == SCHED_E_MALFORMEDXML, "expected SCHED_E_MALFORMEDXML, got %#x\n", hr);

    /* test registration info */
    MultiByteToWideChar(CP_ACP, 0, xml1, -1, xmlW, ARRAY_SIZE(xmlW));
    hr = ITaskDefinition_put_XmlText(taskdef, xmlW);
    ok(hr == S_OK, "put_XmlText error %#x\n", hr);
    hr = ITaskDefinition_get_RegistrationInfo(taskdef, &reginfo);
    ok(hr == S_OK, "get_RegistrationInfo error %#x\n", hr);

    hr = IRegistrationInfo_get_Description(reginfo, &bstr);
    ok(hr == S_OK, "get_Description error %#x\n", hr);
    ok(!lstrcmpW(bstr, Task1), "expected Task1, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = IRegistrationInfo_put_Description(reginfo, NULL);
    ok(hr == S_OK, "put_Description error %#x\n", hr);
    bstr = (BSTR)0xdeadbeef;
    hr = IRegistrationInfo_get_Description(reginfo, &bstr);
    ok(hr == S_OK, "get_Description error %#x\n", hr);
    ok(!bstr, "expected NULL, got %s\n", wine_dbgstr_w(bstr));

    hr = IRegistrationInfo_get_Author(reginfo, &bstr);
    ok(hr == S_OK, "get_Author error %#x\n", hr);
    ok(!lstrcmpW(bstr, authorW), "expected %s, got %s\n", wine_dbgstr_w(authorW), wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = IRegistrationInfo_put_Author(reginfo, NULL);
    ok(hr == S_OK, "put_Author error %#x\n", hr);
    bstr = (BSTR)0xdeadbeef;
    hr = IRegistrationInfo_get_Author(reginfo, &bstr);
    ok(hr == S_OK, "get_Author error %#x\n", hr);
    ok(!bstr, "expected NULL, got %s\n", wine_dbgstr_w(bstr));

    hr = IRegistrationInfo_get_Version(reginfo, &bstr);
    ok(hr == S_OK, "get_Version error %#x\n", hr);
    ok(!lstrcmpW(bstr, versionW), "expected %s, got %s\n", wine_dbgstr_w(versionW), wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = IRegistrationInfo_put_Version(reginfo, NULL);
    ok(hr == S_OK, "put_Version error %#x\n", hr);
    bstr = (BSTR)0xdeadbeef;
    hr = IRegistrationInfo_get_Version(reginfo, &bstr);
    ok(hr == S_OK, "get_Version error %#x\n", hr);
    ok(!bstr, "expected NULL, got %s\n", wine_dbgstr_w(bstr));

    hr = IRegistrationInfo_get_Date(reginfo, &bstr);
    ok(hr == S_OK, "get_Date error %#x\n", hr);
    ok(!lstrcmpW(bstr, dateW), "expected %s, got %s\n", wine_dbgstr_w(dateW), wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = IRegistrationInfo_put_Date(reginfo, NULL);
    ok(hr == S_OK, "put_Date error %#x\n", hr);
    bstr = (BSTR)0xdeadbeef;
    hr = IRegistrationInfo_get_Date(reginfo, &bstr);
    ok(hr == S_OK, "get_Date error %#x\n", hr);
    ok(!bstr, "expected NULL, got %s\n", wine_dbgstr_w(bstr));

    hr = IRegistrationInfo_get_Documentation(reginfo, &bstr);
    ok(hr == S_OK, "get_Documentation error %#x\n", hr);
    ok(!lstrcmpW(bstr, docW), "expected %s, got %s\n", wine_dbgstr_w(docW), wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = IRegistrationInfo_put_Documentation(reginfo, NULL);
    ok(hr == S_OK, "put_Documentation error %#x\n", hr);
    bstr = (BSTR)0xdeadbeef;
    hr = IRegistrationInfo_get_Documentation(reginfo, &bstr);
    ok(hr == S_OK, "get_Documentation error %#x\n", hr);
    ok(!bstr, "expected NULL, got %s\n", wine_dbgstr_w(bstr));

    hr = IRegistrationInfo_get_URI(reginfo, &bstr);
    ok(hr == S_OK, "get_URI error %#x\n", hr);
    ok(!lstrcmpW(bstr, uriW), "expected %s, got %s\n", wine_dbgstr_w(uriW), wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = IRegistrationInfo_put_URI(reginfo, NULL);
    ok(hr == S_OK, "put_URI error %#x\n", hr);
    bstr = (BSTR)0xdeadbeef;
    hr = IRegistrationInfo_get_URI(reginfo, &bstr);
    ok(hr == S_OK, "get_URI error %#x\n", hr);
    ok(!bstr, "expected NULL, got %s\n", wine_dbgstr_w(bstr));

    hr = IRegistrationInfo_get_Source(reginfo, &bstr);
    ok(hr == S_OK, "get_Source error %#x\n", hr);
    ok(!lstrcmpW(bstr, sourceW), "expected %s, got %s\n", wine_dbgstr_w(sourceW), wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = IRegistrationInfo_put_Source(reginfo, NULL);
    ok(hr == S_OK, "put_Source error %#x\n", hr);
    bstr = (BSTR)0xdeadbeef;
    hr = IRegistrationInfo_get_Source(reginfo, &bstr);
    ok(hr == S_OK, "get_Source error %#x\n", hr);
    ok(!bstr, "expected NULL, got %s\n", wine_dbgstr_w(bstr));

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = NULL;
    hr = IRegistrationInfo_get_SecurityDescriptor(reginfo, &var);
    todo_wine
    ok(hr == S_OK, "get_SecurityDescriptor error %#x\n", hr);
    if (hr == S_OK)
        ok(V_VT(&var) == VT_EMPTY, "expected VT_EMPTY, got %u\n", V_VT(&var));

    IRegistrationInfo_Release(reginfo);

    MultiByteToWideChar(CP_ACP, 0, xml4, -1, xmlW, ARRAY_SIZE(xmlW));
    hr = ITaskDefinition_put_XmlText(taskdef, xmlW);
    ok(hr == S_OK, "put_XmlText error %#x\n", hr);
    hr = ITaskDefinition_get_RegistrationInfo(taskdef, &reginfo);
    ok(hr == S_OK, "get_RegistrationInfo error %#x\n", hr);

    hr = IRegistrationInfo_get_Description(reginfo, &bstr);
    ok(hr == S_OK, "get_Description error %#x\n", hr);
    ok(!bstr, "expected NULL, got %s\n", wine_dbgstr_w(bstr));

    hr = ITaskDefinition_get_Triggers(taskdef, &trigger_col);
    ok(hr == S_OK, "get_Triggers failed: %08x\n", hr);
    ok(trigger_col != NULL, "Triggers = NULL\n");

    hr = ITriggerCollection_Create(trigger_col, TASK_TRIGGER_DAILY, &trigger);
    ok(hr == S_OK, "Create failed: %08x\n", hr);
    ok(trigger != NULL, "trigger = NULL\n");
    test_daily_trigger(trigger);
    ITrigger_Release(trigger);
    ITriggerCollection_Release(trigger_col);

    hr = ITaskDefinition_get_Triggers(taskdef, &trigger_col2);
    ok(hr == S_OK, "get_Triggers failed: %08x\n", hr);
    ok(trigger_col == trigger_col2, "Mismatched triggers\n");
    ITriggerCollection_Release(trigger_col2);

    IRegistrationInfo_Release(reginfo);
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
