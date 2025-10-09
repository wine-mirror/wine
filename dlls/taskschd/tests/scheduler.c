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
#include "winternl.h"
#include "winbase.h"
#include "initguid.h"
#include "objbase.h"
#include "taskschd.h"

#include <wine/test.h>

static BOOL is_process_elevated(void)
{
    HANDLE token;
    if (OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &token ))
    {
        TOKEN_ELEVATION_TYPE type;
        DWORD size;
        BOOL ret;

        ret = GetTokenInformation( token, TokenElevationType, &type, sizeof(type), &size );
        CloseHandle( token );
        return (ret && type == TokenElevationTypeFull);
    }
    return FALSE;
}

static BOOL check_win_version(int min_major, int min_minor)
{
    HMODULE hntdll = GetModuleHandleA("ntdll.dll");
    NTSTATUS (WINAPI *pRtlGetVersion)(RTL_OSVERSIONINFOEXW *);
    RTL_OSVERSIONINFOEXW rtlver;

    rtlver.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
    pRtlGetVersion = (void *)GetProcAddress(hntdll, "RtlGetVersion");
    pRtlGetVersion(&rtlver);
    return rtlver.dwMajorVersion > min_major ||
           (rtlver.dwMajorVersion == min_major &&
            rtlver.dwMinorVersion >= min_minor);
}
#define is_win8_plus() check_win_version(6, 2)
#define is_win10_plus() check_win_version(10, 0)

static void test_Connect(void)
{
    WCHAR comp_name[MAX_COMPUTERNAME_LENGTH + 1];
    WCHAR user_name[256];
    WCHAR domain_name[256];
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
        win_skip("CoCreateInstance(CLSID_TaskScheduler) error %#lx\n", hr);
        return;
    }

    hr = ITaskService_get_Connected(service, NULL);
    ok(hr == E_POINTER, "expected E_POINTER, got %#lx\n", hr);

    vbool = 0xdead;
    hr = ITaskService_get_Connected(service, &vbool);
    ok(hr == S_OK, "get_Connected error %#lx\n", hr);
    ok(vbool == VARIANT_FALSE, "expected VARIANT_FALSE, got %d\n", vbool);

    hr = ITaskService_get_TargetServer(service, NULL);
    ok(hr == E_POINTER, "expected E_POINTER, got %#lx\n", hr);

    hr = ITaskService_get_TargetServer(service, &bstr);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ONLY_IF_CONNECTED), "expected ERROR_ONLY_IF_CONNECTED, got %#lx\n", hr);

    hr = ITaskService_get_ConnectedUser(service, NULL);
    ok(hr == E_POINTER, "expected E_POINTER, got %#lx\n", hr);

    hr = ITaskService_get_ConnectedUser(service, &bstr);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ONLY_IF_CONNECTED), "expected ERROR_ONLY_IF_CONNECTED, got %#lx\n", hr);

    hr = ITaskService_get_ConnectedDomain(service, NULL);
    ok(hr == E_POINTER, "expected E_POINTER, got %#lx\n", hr);

    hr = ITaskService_get_ConnectedDomain(service, &bstr);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ONLY_IF_CONNECTED), "expected ERROR_ONLY_IF_CONNECTED, got %#lx\n", hr);

    /* Win7 doesn't support UNC \\ prefix, but according to a user
     * comment on MSDN Win8 supports both ways.
     */
    len = ARRAY_SIZE(comp_name);
    GetComputerNameW(comp_name, &len);

    V_VT(&v_null) = VT_NULL;

    V_VT(&v_comp) = VT_BSTR;
    V_BSTR(&v_comp) = SysAllocString(comp_name);

    hr = ITaskService_Connect(service, v_comp, v_null, v_null, v_null);
    ok(hr == S_OK || (hr == E_ACCESSDENIED && !is_process_elevated()),
       "Connect error %#lx\n", hr);
    was_connected = hr == S_OK;
    SysFreeString(V_BSTR(&v_comp));

    V_BSTR(&v_comp) = SysAllocString(L"0.0.0.0");
    hr = ITaskService_Connect(service, v_comp, v_null, v_null, v_null);
    ok(hr == HRESULT_FROM_WIN32(RPC_S_INVALID_NET_ADDR) || hr == HRESULT_FROM_WIN32(ERROR_BAD_NETPATH) /* VM */,
       "expected RPC_S_INVALID_NET_ADDR, got %#lx\n", hr);
    SysFreeString(V_BSTR(&v_comp));

    vbool = 0xdead;
    hr = ITaskService_get_Connected(service, &vbool);
    ok(hr == S_OK, "get_Connected error %#lx\n", hr);
    ok(vbool == VARIANT_FALSE || (was_connected && vbool == VARIANT_TRUE),
       "Connect shouldn't trash an existing connection, got %d (was connected %d)\n", vbool, was_connected);

    V_BSTR(&v_comp) = SysAllocString(L"");
    hr = ITaskService_Connect(service, v_comp, v_null, v_null, v_null);
    ok(hr == S_OK, "Connect error %#lx\n", hr);
    SysFreeString(V_BSTR(&v_comp));

    V_BSTR(&v_comp) = NULL;
    hr = ITaskService_Connect(service, v_comp, v_null, v_null, v_null);
    ok(hr == S_OK, "Connect error %#lx\n", hr);

    hr = ITaskService_Connect(service, v_null, v_null, v_null, v_null);
    ok(hr == S_OK, "Connect error %#lx\n", hr);

    vbool = 0xdead;
    hr = ITaskService_get_Connected(service, &vbool);
    ok(hr == S_OK, "get_Connected error %#lx\n", hr);
    ok(vbool == VARIANT_TRUE, "expected VARIANT_TRUE, got %d\n", vbool);

    hr = ITaskService_get_TargetServer(service, &bstr);
    ok(hr == S_OK, "get_TargetServer error %#lx\n", hr);
    ok(!lstrcmpW(comp_name, bstr), "compname %s != server name %s\n", wine_dbgstr_w(comp_name), wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    len = ARRAY_SIZE(user_name);
    GetUserNameW(user_name, &len);

    hr = ITaskService_get_ConnectedUser(service, &bstr);
    ok(hr == S_OK, "get_ConnectedUser error %#lx\n", hr);
    ok(!lstrcmpW(user_name, bstr), "username %s != user name %s\n", wine_dbgstr_w(user_name), wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    len = ARRAY_SIZE(domain_name);
    if (!GetEnvironmentVariableW(L"USERDOMAIN", domain_name, len))
    {
         GetComputerNameExW(ComputerNameDnsHostname, domain_name, &len);
         if (is_win10_plus())
             wcsupr(domain_name);
    }

    hr = ITaskService_get_ConnectedDomain(service, &bstr);
    ok(hr == S_OK, "get_ConnectedDomain error %#lx\n", hr);
    ok(!lstrcmpW(domain_name, bstr), "domainname %s != domain name %s\n", wine_dbgstr_w(domain_name), wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    ITaskService_Release(service);
}

static void test_GetFolder(void)
{
    static WCHAR dot[] = L".";
    static WCHAR empty[] = L"";
    static WCHAR slash[] = L"/";
    static WCHAR bslash[] = L"\\";
    static WCHAR Wine[] = L"\\Wine";
    static WCHAR Wine_Folder1[] = L"\\Wine\\Folder1";
    static WCHAR Wine_Folder1_[] = L"\\Wine\\Folder1\\";
    static WCHAR Wine_Folder1_Folder2[] = L"\\Wine\\Folder1\\Folder2";
    static WCHAR Folder1_Folder2[] = L"\\Folder1\\Folder2";
    HRESULT hr;
    BSTR bstr;
    VARIANT v_null;
    ITaskService *service;
    ITaskFolder *folder, *subfolder, *subfolder2;

    hr = CoCreateInstance(&CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, &IID_ITaskService, (void **)&service);
    if (hr != S_OK)
    {
        win_skip("CoCreateInstance(CLSID_TaskScheduler) error %#lx\n", hr);
        return;
    }

    hr = ITaskService_GetFolder(service, NULL, &folder);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ONLY_IF_CONNECTED), "expected ERROR_ONLY_IF_CONNECTED, got %#lx\n", hr);

    V_VT(&v_null) = VT_NULL;

    hr = ITaskService_Connect(service, v_null, v_null, v_null, v_null);
    ok(hr == S_OK, "Connect error %#lx\n", hr);

    hr = ITaskService_GetFolder(service, slash, &folder);
    todo_wine
    ok(hr == HRESULT_FROM_WIN32(ERROR_INVALID_NAME), "expected ERROR_INVALID_NAME, got %#lx\n", hr);

    hr = ITaskService_GetFolder(service, dot, &folder);
    todo_wine
    ok(hr == HRESULT_FROM_WIN32(ERROR_INVALID_NAME) || hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) /* win7 */,
       "expected ERROR_INVALID_NAME, got %#lx\n", hr);

    hr = ITaskService_GetFolder(service, bslash, &folder);
    ok(hr == S_OK, "GetFolder error %#lx\n", hr);
    ITaskFolder_Release(folder);

    hr = ITaskService_GetFolder(service, NULL, NULL);
    ok(hr == E_POINTER, "expected E_POINTER, got %#lx\n", hr);

    hr = ITaskService_GetFolder(service, empty, &folder);
    ok(hr == S_OK, "GetFolder error %#lx\n", hr);
    ITaskFolder_Release(folder);

    hr = ITaskService_GetFolder(service, NULL, &folder);
    ok(hr == S_OK, "GetFolder error %#lx\n", hr);

    hr = ITaskFolder_get_Name(folder, NULL);
    ok(hr == E_POINTER, "expected E_POINTER, got %#lx\n", hr);

    hr = ITaskFolder_get_Name(folder, &bstr);
    ok (hr == S_OK, "get_Name error %#lx\n", hr);
    ok(!lstrcmpW(bstr, bslash), "expected '\\', got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    hr = ITaskFolder_get_Path(folder, NULL);
    ok(hr == E_POINTER, "expected E_POINTER, got %#lx\n", hr);

    hr = ITaskFolder_get_Path(folder, &bstr);
    ok(hr == S_OK, "get_Path error %#lx\n", hr);
    ok(!lstrcmpW(bstr, bslash), "expected '\\', got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    hr = ITaskFolder_CreateFolder(folder, NULL, v_null, &subfolder);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);

    /* Just in case something was left from previous runs */
    ITaskFolder_DeleteFolder(folder, Wine_Folder1_Folder2, 0);
    ITaskFolder_DeleteFolder(folder, Wine_Folder1, 0);
    ITaskFolder_DeleteFolder(folder, Wine, 0);

    hr = ITaskFolder_CreateFolder(folder, slash, v_null, &subfolder);
    todo_wine
    ok(hr == HRESULT_FROM_WIN32(ERROR_INVALID_NAME), "expected ERROR_INVALID_NAME, got %#lx\n", hr);

    hr = ITaskService_GetFolder(service, Wine_Folder1_Folder2, &subfolder);
    ok(hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) || hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) /* win7 */,
       "expected ERROR_PATH_NOT_FOUND, got %#lx\n", hr);

    hr = ITaskFolder_CreateFolder(folder, bslash, v_null, &subfolder);
    todo_wine
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);

    if (!is_process_elevated() && !is_win8_plus())
    {
        win_skip("Skipping CreateFolder tests because deleting folders requires elevated privileges on Windows 7\n");
        goto cleanup;
    }

    hr = ITaskFolder_CreateFolder(folder, Wine_Folder1_Folder2, v_null, &subfolder);
    ok(hr == S_OK, "CreateFolder error %#lx\n", hr);
    ITaskFolder_Release(subfolder);

    hr = ITaskFolder_CreateFolder(folder, Wine, v_null, NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), "expected ERROR_ALREADY_EXISTS, got %#lx\n", hr);

    hr = ITaskFolder_CreateFolder(folder, Wine_Folder1_, v_null, &subfolder);
    ok(hr == HRESULT_FROM_WIN32(ERROR_INVALID_NAME), "expected ERROR_INVALID_NAME, got %#lx\n", hr);

    hr = ITaskFolder_CreateFolder(folder, Wine, v_null, &subfolder);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), "expected ERROR_ALREADY_EXISTS, got %#lx\n", hr);
    hr = ITaskFolder_CreateFolder(folder, Wine+1, v_null, &subfolder);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), "expected ERROR_ALREADY_EXISTS, got %#lx\n", hr);
    hr = ITaskFolder_CreateFolder(folder, Wine_Folder1, v_null, &subfolder);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), "expected ERROR_ALREADY_EXISTS, got %#lx\n", hr);
    hr = ITaskFolder_CreateFolder(folder, Wine_Folder1+1, v_null, &subfolder);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), "expected ERROR_ALREADY_EXISTS, got %#lx\n", hr);
    hr = ITaskFolder_CreateFolder(folder, Wine_Folder1_Folder2, v_null, &subfolder);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), "expected ERROR_ALREADY_EXISTS, got %#lx\n", hr);
    hr = ITaskFolder_CreateFolder(folder, Wine_Folder1_Folder2+1, v_null, &subfolder);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), "expected ERROR_ALREADY_EXISTS, got %#lx\n", hr);

    hr = ITaskService_GetFolder(service, Wine_Folder1_Folder2, NULL);
    ok(hr == E_POINTER, "expected E_POINTER, got %#lx\n", hr);

    hr = ITaskService_GetFolder(service, Wine_Folder1_Folder2, &subfolder);
    ok(hr == S_OK, "GetFolder error %#lx\n", hr);

    hr = ITaskFolder_get_Name(subfolder, &bstr);
    ok (hr == S_OK, "get_Name error %#lx\n", hr);
    ok(!lstrcmpW(bstr, L"Folder2"), "expected Folder2, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = ITaskFolder_get_Path(subfolder, &bstr);
    ok(hr == S_OK, "get_Path error %#lx\n", hr);
    ok(!lstrcmpW(bstr, Wine_Folder1_Folder2), "expected \\Wine\\Folder1\\Folder2, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    ITaskFolder_Release(subfolder);

    hr = ITaskService_GetFolder(service, Wine_Folder1_Folder2+1, &subfolder);
    ok(hr == S_OK, "GetFolder error %#lx\n", hr);
    hr = ITaskFolder_get_Name(subfolder, &bstr);
    ok (hr == S_OK, "get_Name error %#lx\n", hr);
    ok(!lstrcmpW(bstr, L"Folder2"), "expected Folder2, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = ITaskFolder_get_Path(subfolder, &bstr);
    ok(hr == S_OK, "get_Path error %#lx\n", hr);
    ok(!lstrcmpW(bstr, Wine_Folder1_Folder2+1), "expected Wine\\Folder1\\Folder2, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    ITaskFolder_Release(subfolder);

    hr = ITaskService_GetFolder(service, Wine_Folder1, &subfolder);
    ok(hr == S_OK, "GetFolder error %#lx\n", hr);
    hr = ITaskFolder_get_Name(subfolder, &bstr);
    ok (hr == S_OK, "get_Name error %#lx\n", hr);
    ok(!lstrcmpW(bstr, L"Folder1"), "expected Folder1, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = ITaskFolder_get_Path(subfolder, &bstr);
    ok(hr == S_OK, "get_Path error %#lx\n", hr);
    ok(!lstrcmpW(bstr, Wine_Folder1), "expected \\Wine\\Folder1, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    ITaskFolder_Release(subfolder);

    hr = ITaskService_GetFolder(service, Wine, &subfolder);
    ok(hr == S_OK, "GetFolder error %#lx\n", hr);
    hr = ITaskFolder_get_Name(subfolder, &bstr);
    ok (hr == S_OK, "get_Name error %#lx\n", hr);
    ok(!lstrcmpW(bstr, Wine+1), "expected Wine, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = ITaskFolder_get_Path(subfolder, &bstr);
    ok(hr == S_OK, "get_Path error %#lx\n", hr);
    ok(!lstrcmpW(bstr, Wine), "expected \\Wine, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    ITaskFolder_Release(subfolder);

    hr = ITaskService_GetFolder(service, Wine+1, &subfolder);
    ok(hr == S_OK, "GetFolder error %#lx\n", hr);
    hr = ITaskFolder_get_Name(subfolder, &bstr);
    ok (hr == S_OK, "get_Name error %#lx\n", hr);
    ok(!lstrcmpW(bstr, Wine+1), "expected Wine, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = ITaskFolder_get_Path(subfolder, &bstr);
    ok(hr == S_OK, "get_Path error %#lx\n", hr);
    ok(!lstrcmpW(bstr, Wine+1), "expected Wine, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    hr = ITaskFolder_GetFolder(subfolder, bslash, &subfolder2);
    ok(hr == HRESULT_FROM_WIN32(ERROR_INVALID_NAME), "expected ERROR_INVALID_NAME, got %#lx\n", hr);

    hr = ITaskFolder_GetFolder(subfolder, NULL, &subfolder2);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);

    hr = ITaskFolder_GetFolder(subfolder, empty, &subfolder2);
    ok(hr == S_OK, "GetFolder error %#lx\n", hr);
    hr = ITaskFolder_get_Name(subfolder2, &bstr);
    ok (hr == S_OK, "get_Name error %#lx\n", hr);
    ok(!lstrcmpW(bstr, Wine+1), "expected Wine, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = ITaskFolder_get_Path(subfolder2, &bstr);
    ok(hr == S_OK, "get_Path error %#lx\n", hr);
    ok(!lstrcmpW(bstr, Wine+1), "expected Wine, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    ITaskFolder_Release(subfolder2);

    hr = ITaskFolder_GetFolder(subfolder, Folder1_Folder2, &subfolder2);
    ok(hr == S_OK, "GetFolder error %#lx\n", hr);
    hr = ITaskFolder_get_Name(subfolder2, &bstr);
    ok (hr == S_OK, "get_Name error %#lx\n", hr);
    ok(!lstrcmpW(bstr, L"Folder2"), "expected Folder2, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = ITaskFolder_get_Path(subfolder2, &bstr);
    ok(hr == S_OK, "get_Path error %#lx\n", hr);
    ok(!lstrcmpW(bstr, Wine_Folder1_Folder2+1), "expected Wine\\Folder1\\Folder2, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    ITaskFolder_Release(subfolder2);

    hr = ITaskFolder_GetFolder(subfolder, Folder1_Folder2+1, &subfolder2);
    ok(hr == S_OK, "GetFolder error %#lx\n", hr);

    hr = ITaskFolder_get_Name(subfolder2, &bstr);
    ok (hr == S_OK, "get_Name error %#lx\n", hr);
    ok(!lstrcmpW(bstr, L"Folder2"), "expected Folder2, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = ITaskFolder_get_Path(subfolder2, &bstr);
    ok(hr == S_OK, "get_Path error %#lx\n", hr);
    ok(!lstrcmpW(bstr, Wine_Folder1_Folder2+1), "expected Wine\\Folder1\\Folder2, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    ITaskFolder_Release(subfolder2);

    ITaskFolder_Release(subfolder);

    hr = ITaskFolder_DeleteFolder(folder, Wine, 0);
    ok(hr == HRESULT_FROM_WIN32(ERROR_DIR_NOT_EMPTY), "expected ERROR_DIR_NOT_EMPTY, got %#lx\n", hr);

    hr = ITaskFolder_DeleteFolder(folder, Wine_Folder1_Folder2, 0);
    ok(hr == S_OK, "DeleteFolder error %#lx\n", hr);
    hr = ITaskFolder_DeleteFolder(folder, Wine_Folder1+1, 0);
    ok(hr == S_OK, "DeleteFolder error %#lx\n", hr);
    hr = ITaskFolder_DeleteFolder(folder, Wine+1, 0);
    ok(hr == S_OK, "DeleteFolder error %#lx\n", hr);

    hr = ITaskFolder_DeleteFolder(folder, Wine, 0);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) || hr == S_OK /* win7 */, "expected ERROR_FILE_NOT_FOUND, got %#lx\n", hr);

    hr = ITaskFolder_DeleteFolder(folder, NULL, 0);
    ok(hr == E_ACCESSDENIED || hr == E_INVALIDARG /* Vista */, "expected E_ACCESSDENIED, got %#lx\n", hr);

    hr = ITaskFolder_DeleteFolder(folder, empty, 0);
    ok(hr == E_ACCESSDENIED || hr == E_INVALIDARG /* Vista */, "expected E_ACCESSDENIED, got %#lx\n", hr);

    hr = ITaskFolder_DeleteFolder(folder, slash, 0);
    todo_wine
    ok(hr == HRESULT_FROM_WIN32(ERROR_INVALID_NAME), "expected ERROR_INVALID_NAME, got %#lx\n", hr);

 cleanup:
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
    static WCHAR Wine[] = L"\\Wine";
    static WCHAR Wine_Folder1[] = L"\\Wine\\Folder1";
    static WCHAR Wine_Folder2[] = L"\\Wine\\Folder2";
    static WCHAR Wine_Folder3[] = L"\\Wine\\Folder3";
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

    if (!is_process_elevated() && !is_win8_plus())
    {
        win_skip("Skipping ITaskFolderCollection tests because deleting folders requires elevated privileges on Windows 7\n");
        return;
    }

    hr = CoCreateInstance(&CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, &IID_ITaskService, (void **)&service);
    if (hr != S_OK)
    {
        win_skip("CoCreateInstance(CLSID_TaskScheduler) error %#lx\n", hr);
        return;
    }

    V_VT(&v_null) = VT_NULL;

    hr = ITaskService_Connect(service, v_null, v_null, v_null, v_null);
    ok(hr == S_OK, "Connect error %#lx\n", hr);

    hr = ITaskService_GetFolder(service, NULL, &root);
    ok(hr == S_OK, "GetFolder error %#lx\n", hr);

    /* Just in case something was left from previous runs */
    ITaskFolder_DeleteFolder(root, Wine_Folder1, 0);
    ITaskFolder_DeleteFolder(root, Wine_Folder2, 0);
    ITaskFolder_DeleteFolder(root, Wine_Folder3, 0);
    ITaskFolder_DeleteFolder(root, Wine, 0);

    hr = ITaskFolder_CreateFolder(root, Wine_Folder1, v_null, &subfolder);
    ok(hr == S_OK, "CreateFolder error %#lx\n", hr);
    ITaskFolder_Release(subfolder);

    hr = ITaskFolder_CreateFolder(root, Wine_Folder2, v_null, &subfolder);
    ok(hr == S_OK, "CreateFolder error %#lx\n", hr);
    ITaskFolder_Release(subfolder);

    hr = ITaskFolder_GetFolder(root, Wine, &folder);
    ok(hr == S_OK, "GetFolder error %#lx\n", hr);

    hr = ITaskFolder_GetFolders(folder, 0, NULL);
    ok (hr == E_POINTER, "expected E_POINTER, got %#lx\n", hr);

    hr = ITaskFolder_GetFolders(folder, 0, &folders);
    ok(hr == S_OK, "GetFolders error %#lx\n", hr);

    ITaskFolder_Release(folder);

    hr = ITaskFolderCollection_get_Count(folders, NULL);
    ok (hr == E_POINTER, "expected E_POINTER, got %#lx\n", hr);

    count = 0;
    hr = ITaskFolderCollection_get_Count(folders, (LONG *)&count);
    ok(hr == S_OK, "get_Count error %#lx\n", hr);
    ok(count == 2, "expected 2, got %ld\n", count);

    hr = ITaskFolder_CreateFolder(root, Wine_Folder3, v_null, &subfolder);
    ok(hr == S_OK, "CreateFolder error %#lx\n", hr);
    ITaskFolder_Release(subfolder);

    count = 0;
    hr = ITaskFolderCollection_get_Count(folders, (LONG *)&count);
    ok(hr == S_OK, "get_Count error %#lx\n", hr);
    ok(count == 2, "expected 2, got %ld\n", count);

    set_var(VT_INT, &idx, 0);
    hr = ITaskFolderCollection_get_Item(folders, idx, NULL);
    ok (hr == E_POINTER, "expected E_POINTER, got %#lx\n", hr);

    for (i = 0; i < ARRAY_SIZE(vt); i++)
    {
        set_var(vt[i], &idx, 1);
        hr = ITaskFolderCollection_get_Item(folders, idx, &subfolder);
        ok(hr == S_OK, "get_Item(vt = %d) error %#lx\n", vt[i], hr);
        ITaskFolder_Release(subfolder);
    }

    for (i = 0; i <= count; i++)
    {
        V_VT(&idx) = VT_I4;
        V_UI4(&idx) = i;
        hr = ITaskFolderCollection_get_Item(folders, idx, &subfolder);
        if (i == 0)
        {
            ok (hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);
            continue;
        }
        ok(hr == S_OK, "get_Item error %#lx\n", hr);

        hr = ITaskFolder_get_Path(subfolder, &bstr);
        ok(hr == S_OK, "get_Path error %#lx\n", hr);
        is_first = !lstrcmpW(bstr, Wine_Folder1);
        if (is_first)
            ok(!lstrcmpW(bstr, Wine_Folder1), "expected \\Wine\\Folder1, got %s\n", wine_dbgstr_w(bstr));
        else
            ok(!lstrcmpW(bstr, Wine_Folder2), "expected \\Wine\\Folder2, got %s\n", wine_dbgstr_w(bstr));
        SysFreeString(bstr);

        hr = ITaskFolder_get_Name(subfolder, &bstr);
        ok(hr == S_OK, "get_Name error %#lx\n", hr);
        if (is_first)
            ok(!lstrcmpW(bstr, L"Folder1"), "expected Folder1, got %s\n", wine_dbgstr_w(bstr));
        else
            ok(!lstrcmpW(bstr, L"Folder2"), "expected Folder2, got %s\n", wine_dbgstr_w(bstr));

        ITaskFolder_Release(subfolder);

        V_VT(&idx) = VT_BSTR;
        V_BSTR(&idx) = bstr;
        hr = ITaskFolderCollection_get_Item(folders, idx, &subfolder);
        ok(hr == S_OK, "get_Item error %#lx\n", hr);
        SysFreeString(bstr);

        hr = ITaskFolder_get_Path(subfolder, &bstr);
        ok(hr == S_OK, "get_Path error %#lx\n", hr);
        if (is_first)
            ok(!lstrcmpW(bstr, Wine_Folder1), "expected \\Wine\\Folder1, got %s\n", wine_dbgstr_w(bstr));
        else
            ok(!lstrcmpW(bstr, Wine_Folder2), "expected \\Wine\\Folder2, got %s\n", wine_dbgstr_w(bstr));
        SysFreeString(bstr);

        hr = ITaskFolder_get_Name(subfolder, &bstr);
        ok(hr == S_OK, "get_Name error %#lx\n", hr);
        if (is_first)
            ok(!lstrcmpW(bstr, L"Folder1"), "expected Folder1, got %s\n", wine_dbgstr_w(bstr));
        else
            ok(!lstrcmpW(bstr, L"Folder2"), "expected Folder2, got %s\n", wine_dbgstr_w(bstr));
        SysFreeString(bstr);

        ITaskFolder_Release(subfolder);
    }

    V_VT(&idx) = VT_I4;
    V_UI4(&idx) = 3;
    hr = ITaskFolderCollection_get_Item(folders, idx, &subfolder);
    ok (hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);

    hr = ITaskFolderCollection_QueryInterface(folders, &IID_IEnumVARIANT, (void **)&enumvar);
    ok(hr == E_NOINTERFACE, "expected E_NOINTERFACE, got %#lx\n", hr);
    hr = ITaskFolderCollection_QueryInterface(folders, &IID_IEnumUnknown, (void **)&enumvar);
    ok(hr == E_NOINTERFACE, "expected E_NOINTERFACE, got %#lx\n", hr);

    hr = ITaskFolderCollection_get__NewEnum(folders, NULL);
    ok (hr == E_POINTER, "expected E_POINTER, got %#lx\n", hr);

    hr = ITaskFolderCollection_get__NewEnum(folders, &unknown);
    ok(hr == S_OK, "get__NewEnum error %#lx\n", hr);
    hr = IUnknown_QueryInterface(unknown, &IID_IEnumUnknown, (void **)&enumvar);
    ok(hr == E_NOINTERFACE, "expected E_NOINTERFACE, got %#lx\n", hr);
    hr = IUnknown_QueryInterface(unknown, &IID_IEnumVARIANT, (void **)&enumvar);
    ok(hr == S_OK, "QueryInterface error %#lx\n", hr);
    IEnumVARIANT_Release(enumvar);

    hr = IUnknown_QueryInterface(unknown, &IID_IUnknown, (void **)&enumvar);
    ok(hr == S_OK, "QueryInterface error %#lx\n", hr);
    IUnknown_Release(unknown);

    hr = IEnumVARIANT_Skip(enumvar, 0);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);
    hr = IEnumVARIANT_Skip(enumvar, 2);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);
    hr = IEnumVARIANT_Skip(enumvar, 1);
    ok(hr == S_FALSE, "expected S_FALSE, got %#lx\n", hr);

    hr = IEnumVARIANT_Reset(enumvar);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);

    count = -1;
    hr = IEnumVARIANT_Next(enumvar, 1, NULL, &count);
    ok(hr == E_POINTER, "expected E_POINTER, got %#lx\n", hr);

    memset(var, 0, sizeof(var));
    count = -1;
    hr = IEnumVARIANT_Next(enumvar, 1, var, &count);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);
    ok(count == 1, "expected 1, got %ld\n", count);
    hr = ITaskFolder_get_Path((ITaskFolder *)V_DISPATCH(&var[0]), &bstr);
    ok(hr == S_OK, "get_Path error %#lx\n", hr);
    is_first = !lstrcmpW(bstr, Wine_Folder1);
    if (is_first)
        ok(!lstrcmpW(bstr, Wine_Folder1), "expected \\Wine\\Folder1, got %s\n", wine_dbgstr_w(bstr));
    else
        ok(!lstrcmpW(bstr, Wine_Folder2), "expected \\Wine\\Folder2, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = ITaskFolder_get_Name((ITaskFolder *)V_DISPATCH(&var[0]), &bstr);
    ok(hr == S_OK, "get_Name error %#lx\n", hr);
    if (is_first)
        ok(!lstrcmpW(bstr, L"Folder1"), "expected Folder1, got %s\n", wine_dbgstr_w(bstr));
    else
        ok(!lstrcmpW(bstr, L"Folder2"), "expected Folder2, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    IDispatch_Release(V_DISPATCH(&var[0]));

    memset(var, 0, sizeof(var));
    count = -1;
    hr = IEnumVARIANT_Next(enumvar, 1, var, &count);
    ok(hr == S_FALSE, "expected S_FALSE, got %#lx\n", hr);
    ok(count == 0, "expected 0, got %ld\n", count);

    count = -1;
    hr = IEnumVARIANT_Next(enumvar, 1, NULL, &count);
    ok(hr == S_FALSE, "expected S_FALSE, got %#lx\n", hr);
    ok(count == 0, "expected 0, got %ld\n", count);

    hr = IEnumVARIANT_Reset(enumvar);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);

    memset(var, 0, sizeof(var));
    count = -1;
    hr = IEnumVARIANT_Next(enumvar, 3, var, &count);
    ok(hr == S_FALSE, "expected S_FALSE, got %#lx\n", hr);
    ok(count == 2, "expected 2, got %ld\n", count);
    ok(V_VT(&var[0]) == VT_DISPATCH, "expected VT_DISPATCH, got %d\n", V_VT(&var[0]));
    ok(V_VT(&var[1]) == VT_DISPATCH, "expected VT_DISPATCH, got %d\n", V_VT(&var[1]));
    IEnumVARIANT_Release(enumvar);

    for (i = 0; i < count; i++)
    {
        hr = IDispatch_QueryInterface(V_DISPATCH(&var[i]), &IID_ITaskFolder, (void **)&subfolder);
        ok(hr == S_OK, "QueryInterface error %#lx\n", hr);

        hr = ITaskFolder_get_Path(subfolder, &bstr);
        ok(hr == S_OK, "get_Path error %#lx\n", hr);
        is_first = !lstrcmpW(bstr, Wine_Folder1);
        if (is_first)
            ok(!lstrcmpW(bstr, Wine_Folder1), "expected \\Wine\\Folder1, got %s\n", wine_dbgstr_w(bstr));
        else
            ok(!lstrcmpW(bstr, Wine_Folder2), "expected \\Wine\\Folder2, got %s\n", wine_dbgstr_w(bstr));
        SysFreeString(bstr);

        hr = ITaskFolder_get_Name(subfolder, &bstr);
        ok(hr == S_OK, "get_Name error %#lx\n", hr);
        if (is_first)
            ok(!lstrcmpW(bstr, L"Folder1"), "expected Folder1, got %s\n", wine_dbgstr_w(bstr));
        else
            ok(!lstrcmpW(bstr, L"Folder2"), "expected Folder2, got %s\n", wine_dbgstr_w(bstr));
        SysFreeString(bstr);

        ITaskFolder_Release(subfolder);
    }

    IDispatch_Release(V_DISPATCH(&var[0]));
    IDispatch_Release(V_DISPATCH(&var[1]));

    ITaskFolderCollection_Release(folders);

    hr = ITaskFolder_DeleteFolder(root, Wine_Folder1, 0);
    ok(hr == S_OK, "DeleteFolder error %#lx\n", hr);
    hr = ITaskFolder_DeleteFolder(root, Wine_Folder2, 0);
    ok(hr == S_OK, "DeleteFolder error %#lx\n", hr);
    hr = ITaskFolder_DeleteFolder(root, Wine_Folder3, 0);
    ok(hr == S_OK, "DeleteFolder error %#lx\n", hr);
    hr = ITaskFolder_DeleteFolder(root, Wine, 0);
    ok(hr == S_OK, "DeleteFolder error %#lx\n", hr);

    ITaskFolder_Release(root);
    ITaskService_Release(service);
}

static void test_GetTask(void)
{
    static WCHAR Wine[] = L"\\Wine";
    static WCHAR Wine_Task1[] = L"\\Wine\\Task1";
    static WCHAR Wine_Task2[] = L"\\Wine\\Task2";
    static WCHAR Task1[] = L"Task1";
    static WCHAR Task2[] = L"Task2";
    static WCHAR xml1[] =
        L"<?xml version=\"1.0\"?>\n"
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
    static WCHAR xml2[] =
        L"<?xml version=\"1.0\"?>\n"
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
        { TASK_UPDATE, __HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) },
        { TASK_CREATE | TASK_UPDATE, S_OK }
    };
    static const struct
    {
        DWORD flags, hr;
    } open_existing_task[] =
    {
        { 0, __HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) },
        { TASK_CREATE, __HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) },
        { TASK_UPDATE, S_OK },
        { TASK_CREATE | TASK_UPDATE, S_OK }
    };
    HRESULT hr;
    BSTR bstr;
    TASK_STATE state;
    VARIANT_BOOL vbool;
    VARIANT v_null;
    ITaskService *service;
    ITaskFolder *root, *folder;
    IRegisteredTask *task1, *task2;
    DATE date;
    IID iid;
    int i;

    if (!is_process_elevated() && !is_win8_plus())
    {
        win_skip("Skipping task creation tests because deleting anything requires elevated privileges on Windows 7\n");
        return;
    }

    hr = CoCreateInstance(&CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, &IID_ITaskService, (void **)&service);
    if (hr != S_OK)
    {
        win_skip("CoCreateInstance(CLSID_TaskScheduler) error %#lx\n", hr);
        return;
    }

    V_VT(&v_null) = VT_NULL;

    hr = ITaskService_Connect(service, v_null, v_null, v_null, v_null);
    ok(hr == S_OK, "Connect error %#lx\n", hr);

    hr = ITaskService_GetFolder(service, NULL, &root);
    ok(hr == S_OK, "GetFolder error %#lx\n", hr);

    /* Just in case something was left from previous runs */
    ITaskFolder_DeleteTask(root, Wine_Task1, 0);
    ITaskFolder_DeleteTask(root, Wine_Task2, 0);
    ITaskFolder_DeleteFolder(root, Wine, 0);

    hr = ITaskFolder_GetTask(root, Wine_Task1, &task1);
    ok(hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) || hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) /* win7 */,
       "expected ERROR_PATH_NOT_FOUND, got %#lx\n", hr);

    hr = ITaskFolder_RegisterTask(root, Wine, xml1, TASK_CREATE, v_null, v_null, TASK_LOGON_NONE, v_null, NULL);
    ok(hr == S_OK, "RegisterTask error %#lx\n", hr);

    /* Without elevated privileges this fails on Windows 7 */
    hr = ITaskFolder_DeleteTask(root, Wine, 0);
    ok(hr == S_OK, "DeleteTask error %#lx\n", hr);

    hr = ITaskFolder_CreateFolder(root, Wine, v_null, &folder);
    ok(hr == S_OK, "CreateFolder error %#lx\n", hr);

    hr = ITaskFolder_RegisterTask(root, Wine, xml1, TASK_CREATE, v_null, v_null, TASK_LOGON_NONE, v_null, NULL);
    todo_wine
    ok(hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) || broken(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS)) /* Vista */, "expected ERROR_ACCESS_DENIED, got %#lx\n", hr);

    /* Delete the folder and recreate it to prevent a crash on w1064v1507 */
    hr = ITaskFolder_DeleteFolder(root, Wine, 0);
    todo_wine
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) || broken(hr == S_OK) /* w1064v1507 */,
       "expected ERROR_FILE_NOT_FOUND, got %#lx\n", hr);
    ITaskFolder_Release(folder);

    hr = ITaskFolder_CreateFolder(root, Wine, v_null, &folder);
    ok(hr == S_OK, "CreateFolder error %#lx\n", hr);

    hr = ITaskFolder_GetTask(root, Wine, &task1);
    ok(hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) || hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) /* win7 */,
       "expected ERROR_PATH_NOT_FOUND, got %#lx\n", hr);

    for (i = 0; i < ARRAY_SIZE(create_new_task); i++)
    {
        winetest_push_context("%d", i);
        hr = ITaskFolder_RegisterTask(root, Wine_Task1, xml1, create_new_task[i].flags, v_null, v_null, TASK_LOGON_NONE, v_null, &task1);
        ok(hr == create_new_task[i].hr, "expected %#lx, got %#lx\n", create_new_task[i].hr, hr);
        if (hr == S_OK)
        {
            hr = ITaskFolder_DeleteTask(root, Wine_Task1, 0);
            ok(hr == S_OK, "DeleteTask error %#lx\n", hr);
            IRegisteredTask_Release(task1);
        }
        winetest_pop_context();
    }

    hr = ITaskFolder_RegisterTask(root, Wine_Task1, NULL, TASK_CREATE, v_null, v_null, TASK_LOGON_NONE, v_null, NULL);
    ok(hr == HRESULT_FROM_WIN32(RPC_X_NULL_REF_POINTER), "expected RPC_X_NULL_REF_POINTER, got %#lx\n", hr);

    hr = ITaskFolder_RegisterTask(root, Wine, xml1, TASK_VALIDATE_ONLY, v_null, v_null, TASK_LOGON_NONE, v_null, NULL);
    ok(hr == S_OK, "RegisterTask error %#lx\n", hr);

    hr = ITaskFolder_RegisterTask(root, Wine_Task1, xml1, TASK_CREATE, v_null, v_null, TASK_LOGON_NONE, v_null, NULL);
    ok(hr == S_OK, "RegisterTask error %#lx\n", hr);

    hr = ITaskFolder_RegisterTask(root, Wine_Task1, xml1, TASK_CREATE, v_null, v_null, TASK_LOGON_NONE, v_null, &task1);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), "expected ERROR_ALREADY_EXISTS, got %#lx\n", hr);

    hr = ITaskFolder_RegisterTask(root, Wine_Task1, xml1, 0, v_null, v_null, TASK_LOGON_NONE, v_null, NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), "expected ERROR_ALREADY_EXISTS, got %#lx\n", hr);

    hr = ITaskFolder_RegisterTask(root, Wine_Task1, xml1, TASK_CREATE_OR_UPDATE, v_null, v_null, TASK_LOGON_NONE, v_null, &task1);
    ok(hr == S_OK, "RegisterTask error %#lx\n", hr);

    for (i = 0; i < ARRAY_SIZE(open_existing_task); i++)
    {
        hr = ITaskFolder_RegisterTask(root, Wine_Task1, xml1, open_existing_task[i].flags, v_null, v_null, TASK_LOGON_NONE, v_null, &task2);
        ok(hr == open_existing_task[i].hr, "%d: expected %#lx, got %#lx\n", i, open_existing_task[i].hr, hr);
        if (hr == S_OK)
            IRegisteredTask_Release(task2);
    }

    hr = IRegisteredTask_get_Name(task1, NULL);
    ok(hr == E_POINTER, "expected E_POINTER, got %#lx\n", hr);

    hr = IRegisteredTask_get_Name(task1, &bstr);
    ok(hr == S_OK, "get_Name error %#lx\n", hr);
    ok(!lstrcmpW(bstr, Task1), "expected Task1, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = IRegisteredTask_get_Path(task1, &bstr);
    ok(hr == S_OK, "get_Path error %#lx\n", hr);
    ok(!lstrcmpW(bstr, Wine_Task1), "expected \\Wine\\Task1, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = IRegisteredTask_get_State(task1, &state);
    ok(hr == S_OK, "get_State error %#lx\n", hr);
    ok(state == TASK_STATE_DISABLED, "expected TASK_STATE_DISABLED, got %d\n", state);
    hr = IRegisteredTask_get_Enabled(task1, &vbool);
    ok(hr == S_OK, "get_Enabled error %#lx\n", hr);
    ok(vbool == VARIANT_FALSE, "expected VARIANT_FALSE, got %d\n", vbool);

    hr = IRegisteredTask_get_LastRunTime(task1, &date);
    ok(hr == SCHED_S_TASK_HAS_NOT_RUN, "got %#lx\n", hr);

    IRegisteredTask_Release(task1);

    hr = ITaskFolder_RegisterTask(folder, Task1, xml1, TASK_CREATE, v_null, v_null, TASK_LOGON_NONE, v_null, &task2);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), "expected ERROR_ALREADY_EXISTS, got %#lx\n", hr);

    hr = ITaskFolder_RegisterTask(folder, Task2, xml2, TASK_CREATE, v_null, v_null, TASK_LOGON_NONE, v_null, &task2);
    ok(hr == S_OK, "RegisterTask error %#lx\n", hr);

    hr = IRegisteredTask_get_Name(task2, &bstr);
    ok(hr == S_OK, "get_Name error %#lx\n", hr);
    ok(!lstrcmpW(bstr, Task2), "expected Task2, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = IRegisteredTask_get_Path(task2, &bstr);
    ok(hr == S_OK, "get_Path error %#lx\n", hr);
    ok(!lstrcmpW(bstr, Wine_Task2), "expected \\Wine\\Task2, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = IRegisteredTask_get_State(task2, &state);
    ok(hr == S_OK, "get_State error %#lx\n", hr);
    ok(state == TASK_STATE_READY, "expected TASK_STATE_READY, got %d\n", state);
    hr = IRegisteredTask_get_Enabled(task2, &vbool);
    ok(hr == S_OK, "get_Enabled error %#lx\n", hr);
    ok(vbool == VARIANT_TRUE, "expected VARIANT_TRUE, got %d\n", vbool);

    hr = IRegisteredTask_get_LastRunTime(task2, &date);
    ok(hr == SCHED_S_TASK_HAS_NOT_RUN, "got %#lx\n", hr);

    IRegisteredTask_Release(task2);

    hr = ITaskFolder_GetTask(root, NULL, &task1);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);

    hr = ITaskFolder_GetTask(root, Wine_Task1, NULL);
    ok(hr == E_POINTER, "expected E_POINTER, got %#lx\n", hr);

    hr = ITaskFolder_GetTask(root, Wine_Task1, &task1);
    ok(hr == S_OK, "GetTask error %#lx\n", hr);

    hr = IRegisteredTask_get_Name(task1, &bstr);
    ok(hr == S_OK, "get_Name error %#lx\n", hr);
    ok(!lstrcmpW(bstr, Task1), "expected Task1, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = IRegisteredTask_get_Path(task1, &bstr);
    ok(hr == S_OK, "get_Path error %#lx\n", hr);
    ok(!lstrcmpW(bstr, Wine_Task1), "expected \\Wine\\Task1, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = IRegisteredTask_get_State(task1, &state);
    ok(hr == S_OK, "get_State error %#lx\n", hr);
    ok(state == TASK_STATE_DISABLED, "expected TASK_STATE_DISABLED, got %d\n", state);
    hr = IRegisteredTask_get_Enabled(task1, &vbool);
    ok(hr == S_OK, "get_Enabled error %#lx\n", hr);
    ok(vbool == VARIANT_FALSE, "expected VARIANT_FALSE, got %d\n", vbool);

    hr = IRegisteredTask_get_LastRunTime(task1, &date);
    ok(hr == SCHED_S_TASK_HAS_NOT_RUN, "got %#lx\n", hr);

    hr = IRegisteredTask_put_Enabled(task1, VARIANT_TRUE);
    ok(hr == S_OK, "put_Enabled error %#lx\n", hr);
    hr = IRegisteredTask_get_State(task1, &state);
    ok(hr == S_OK, "get_State error %#lx\n", hr);
    todo_wine
    ok(state == TASK_STATE_READY, "expected TASK_STATE_READY, got %d\n", state);
    hr = IRegisteredTask_get_Enabled(task1, &vbool);
    ok(hr == S_OK, "get_Enabled error %#lx\n", hr);
    todo_wine
    ok(vbool == VARIANT_TRUE, "expected VARIANT_TRUE, got %d\n", vbool);

    hr = IRegisteredTask_get_LastRunTime(task1, &date);
    ok(hr == SCHED_S_TASK_HAS_NOT_RUN, "got %#lx\n", hr);

    IRegisteredTask_Release(task1);

    hr = ITaskFolder_GetTask(folder, Task2, &task2);
    ok(hr == S_OK, "GetTask error %#lx\n", hr);

    hr = IRegisteredTask_get_Name(task2, &bstr);
    ok(hr == S_OK, "get_Name error %#lx\n", hr);
    ok(!lstrcmpW(bstr, Task2), "expected Task2, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = IRegisteredTask_get_Path(task2, &bstr);
    ok(hr == S_OK, "get_Path error %#lx\n", hr);
    ok(!lstrcmpW(bstr, Wine_Task2), "expected \\Wine\\Task2, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = IRegisteredTask_get_State(task2, &state);
    ok(hr == S_OK, "get_State error %#lx\n", hr);
    ok(state == TASK_STATE_READY, "expected TASK_STATE_READY, got %d\n", state);
    hr = IRegisteredTask_get_Enabled(task2, &vbool);
    ok(hr == S_OK, "get_Enabled error %#lx\n", hr);
    ok(vbool == VARIANT_TRUE, "expected VARIANT_TRUE, got %d\n", vbool);

    hr = IRegisteredTask_get_LastRunTime(task2, &date);
    ok(hr == SCHED_S_TASK_HAS_NOT_RUN, "got %#lx\n", hr);

    hr = IRegisteredTask_get_State(task2, NULL);
    ok(hr == E_POINTER, "expected E_POINTER, got %#lx\n", hr);
    hr = IRegisteredTask_get_Enabled(task2, NULL);
    ok(hr == E_POINTER, "expected E_POINTER, got %#lx\n", hr);

    IRegisteredTask_Release(task2);

    hr = ITaskFolder_DeleteTask(folder, NULL, 0);
    todo_wine
    ok(hr == HRESULT_FROM_WIN32(ERROR_DIR_NOT_EMPTY), "expected ERROR_DIR_NOT_EMPTY, got %#lx\n", hr);

    hr = ITaskFolder_DeleteTask(root, Wine_Task1, 0);
    ok(hr == S_OK, "DeleteTask error %#lx\n", hr);
    hr = ITaskFolder_DeleteTask(folder, Task2, 0);
    ok(hr == S_OK, "DeleteTask error %#lx\n", hr);

    hr = ITaskFolder_DeleteTask(folder, Task2, 0);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) || hr == S_OK /* win7 */, "expected ERROR_FILE_NOT_FOUND, got %#lx\n", hr);

    hr = ITaskFolder_RegisterTask(root, NULL, xml2, TASK_CREATE, v_null, v_null, TASK_LOGON_NONE, v_null, &task1);
    if(hr == E_ACCESSDENIED)
    {
        skip("Access denied\n");
        goto no_access;
    }
    ok(hr == S_OK, "RegisterTask error %#lx\n", hr);

    hr = IRegisteredTask_get_Name(task1, &bstr);
    ok(hr == S_OK, "get_Name error %#lx\n", hr);
    hr = IIDFromString(bstr, &iid);
    ok(hr == S_OK, "IIDFromString error %#lx\n", hr);

    IRegisteredTask_Release(task1);

    hr = ITaskFolder_DeleteTask(root, bstr, 0);
    ok(hr == S_OK, "DeleteTask error %#lx\n", hr);
    SysFreeString(bstr);

    hr = ITaskFolder_RegisterTask(folder, NULL, xml2, TASK_CREATE, v_null, v_null, TASK_LOGON_NONE, v_null, &task1);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);

no_access:
    ITaskFolder_Release(folder);

    hr = ITaskFolder_DeleteFolder(root, Wine, 0);
    ok(hr == S_OK, "DeleteFolder error %#lx\n", hr);

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
    ok(hr == S_OK, "get_Settings error %#lx\n", hr);

    hr = ITaskSettings_get_AllowDemandStart(set, &vbool);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);
    ok(vbool == def->allow_on_demand_start, "expected %d, got %d\n", def->allow_on_demand_start, vbool);

    hr = ITaskSettings_get_RestartInterval(set, &bstr);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);
    if (!def->restart_interval[0])
        ok(bstr == NULL, "expected NULL, got %s\n", wine_dbgstr_w(bstr));
    else
    {
        ok(!lstrcmpW(bstr, def->restart_interval), "expected %s, got %s\n", wine_dbgstr_w(def->restart_interval), wine_dbgstr_w(bstr));
        SysFreeString(bstr);
    }

    hr = ITaskSettings_get_RestartCount(set, &vint);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);
    ok(vint == def->restart_count, "expected %d, got %d\n", def->restart_count, vint);

    hr = ITaskSettings_get_MultipleInstances(set, &policy);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);
    ok(policy == def->policy, "expected %d, got %d\n", def->policy, policy);

    hr = ITaskSettings_get_StopIfGoingOnBatteries(set, &vbool);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);
    ok(vbool == test->stop_if_going_on_batteries, "expected %d, got %d\n", test->stop_if_going_on_batteries, vbool);

    hr = ITaskSettings_get_DisallowStartIfOnBatteries(set, &vbool);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);
    ok(vbool == test->disallow_start_if_on_batteries, "expected %d, got %d\n", test->disallow_start_if_on_batteries, vbool);

    hr = ITaskSettings_get_AllowHardTerminate(set, &vbool);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);
    ok(vbool == def->allow_hard_terminate, "expected %d, got %d\n", def->allow_hard_terminate, vbool);

    hr = ITaskSettings_get_StartWhenAvailable(set, &vbool);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);
    ok(vbool == def->start_when_available, "expected %d, got %d\n", def->start_when_available, vbool);

    hr = ITaskSettings_get_RunOnlyIfNetworkAvailable(set, &vbool);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);
    ok(vbool == def->run_only_if_network_available, "expected %d, got %d\n", def->run_only_if_network_available, vbool);

    hr = ITaskSettings_get_ExecutionTimeLimit(set, &bstr);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);
    if (!test->execution_time_limit[0])
        ok(bstr == NULL, "expected NULL, got %s\n", wine_dbgstr_w(bstr));
    else
    {
        ok(!lstrcmpW(bstr, test->execution_time_limit), "expected %s, got %s\n", wine_dbgstr_w(test->execution_time_limit), wine_dbgstr_w(bstr));
        SysFreeString(bstr);
    }

    hr = ITaskSettings_get_Enabled(set, &vbool);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);
    ok(vbool == test->enabled, "expected %d, got %d\n", test->enabled, vbool);

    hr = ITaskSettings_get_DeleteExpiredTaskAfter(set, &bstr);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);
    if (!test->delete_expired_task_after[0])
        ok(bstr == NULL, "expected NULL, got %s\n", wine_dbgstr_w(bstr));
    else
    {
        ok(!lstrcmpW(bstr, test->delete_expired_task_after), "expected %s, got %s\n", wine_dbgstr_w(test->delete_expired_task_after), wine_dbgstr_w(bstr));
        SysFreeString(bstr);
    }

    hr = ITaskSettings_get_Priority(set, &vint);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);
    ok(vint == test->priority, "expected %d, got %d\n", test->priority, vint);

    hr = ITaskSettings_get_Compatibility(set, &compat);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);
    ok(compat == test->compatibility, "expected %d, got %d\n", test->compatibility, compat);

    hr = ITaskSettings_get_Hidden(set, &vbool);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);
    ok(vbool == test->hidden, "expected %d, got %d\n", test->hidden, vbool);

    hr = ITaskSettings_get_RunOnlyIfIdle(set, &vbool);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);
    ok(vbool == test->run_only_if_idle, "expected %d, got %d\n", test->run_only_if_idle, vbool);

    hr = ITaskSettings_get_WakeToRun(set, &vbool);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);
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
    ok(hr == S_OK, "get_Settings error %#lx\n", hr);

    if (!test->restart_interval[0])
        hr = ITaskSettings_put_RestartInterval(set, NULL);
    else
        hr = ITaskSettings_put_RestartInterval(set, test->restart_interval);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);

    hr = ITaskSettings_put_RestartCount(set, test->restart_count);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);

    hr = ITaskSettings_put_MultipleInstances(set, test->policy);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);

    hr = ITaskSettings_put_StopIfGoingOnBatteries(set, test->stop_if_going_on_batteries);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);

    hr = ITaskSettings_put_DisallowStartIfOnBatteries(set, test->disallow_start_if_on_batteries);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);

    hr = ITaskSettings_put_AllowHardTerminate(set, test->allow_hard_terminate);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);

    hr = ITaskSettings_put_StartWhenAvailable(set, test->start_when_available);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);

    hr = ITaskSettings_put_RunOnlyIfNetworkAvailable(set, test->run_only_if_network_available);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);

    if (!test->execution_time_limit[0])
        hr = ITaskSettings_put_ExecutionTimeLimit(set, NULL);
    else
        hr = ITaskSettings_put_ExecutionTimeLimit(set, test->execution_time_limit);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);

    hr = ITaskSettings_put_Enabled(set, test->enabled);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);

    if (!test->delete_expired_task_after[0])
        hr = ITaskSettings_put_DeleteExpiredTaskAfter(set, NULL);
    else
        hr = ITaskSettings_put_DeleteExpiredTaskAfter(set, test->delete_expired_task_after);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);

    hr = ITaskSettings_put_Priority(set, test->priority);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);

    hr = ITaskSettings_put_Compatibility(set, test->compatibility);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);

    hr = ITaskSettings_put_Hidden(set, test->hidden);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);

    hr = ITaskSettings_put_RunOnlyIfIdle(set, test->run_only_if_idle);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);

    hr = ITaskSettings_put_WakeToRun(set, test->wake_to_run);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);

    hr = ITaskSettings_put_AllowDemandStart(set, test->allow_on_demand_start);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);

    triggers = NULL;
    hr = ITaskDefinition_get_Triggers(taskdef, &triggers);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);
    ok(triggers != NULL, "triggers not set\n");

    hr = ITaskDefinition_put_Triggers(taskdef, triggers);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);
    if (triggers) ITriggerCollection_Release(triggers);

    principal = NULL;
    hr = ITaskDefinition_get_Principal(taskdef, &principal);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);
    ok(principal != NULL, "principal not set\n");

    hr = ITaskDefinition_put_Principal(taskdef, principal);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);
    if (principal) IPrincipal_Release(principal);

    actions = NULL;
    hr = ITaskDefinition_get_Actions(taskdef, &actions);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);
    ok(actions != NULL, "actions not set\n");

    hr = ITaskDefinition_put_Actions(taskdef, actions);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);
    if (actions) IActionCollection_Release(actions);

    /* FIXME: set IIdleSettings and INetworkSettings */

    ITaskSettings_Release(set);
}

static void test_daily_trigger(ITrigger *trigger)
{
    static const struct
    {
        const WCHAR *str;
        const WCHAR *end;
        HRESULT      hr;
    }
    start_test[] =
    {
        {L"2004-01-01T00:00:00", L"2004-01-02T00:00:00", S_OK},
        {L"2004-01-01T00:00:00Z", L"2004-01-02T00:00:00Z", S_OK},
        {L"2004-01-01T00:00:00+01:00", L"2004-01-02T00:00:00+01:00", S_OK},
        {L"2004.01.01T00.00.00", L"2004.01.02T00.00.00", S_OK},
        {L"9999-99-99T99:99:99", L"9999-99-99T99:99:99", S_OK},
        {L"invalid", L"invalid", S_OK},
    };
    IDailyTrigger *daily_trigger;
    BSTR start_boundary, end_boundary;
    VARIANT_BOOL enabled;
    short interval;
    HRESULT hr;
    ULONG i;

    hr = ITrigger_QueryInterface(trigger, &IID_IDailyTrigger, (void**)&daily_trigger);
    ok(hr == S_OK, "Could not get IDailyTrigger iface: %08lx\n", hr);

    interval = -1;
    hr = IDailyTrigger_get_DaysInterval(daily_trigger, &interval);
    ok(hr == S_OK, "get_DaysInterval failed: %08lx\n", hr);
    ok(interval == 1, "interval = %d\n", interval);

    hr = IDailyTrigger_put_DaysInterval(daily_trigger, -2);
    ok(hr == E_INVALIDARG, "put_DaysInterval failed: %08lx\n", hr);
    hr = IDailyTrigger_put_DaysInterval(daily_trigger, 0);
    ok(hr == E_INVALIDARG, "put_DaysInterval failed: %08lx\n", hr);

    interval = -1;
    hr = IDailyTrigger_get_DaysInterval(daily_trigger, &interval);
    ok(hr == S_OK, "get_DaysInterval failed: %08lx\n", hr);
    ok(interval == 1, "interval = %d\n", interval);

    hr = IDailyTrigger_put_DaysInterval(daily_trigger, 2);
    ok(hr == S_OK, "put_DaysInterval failed: %08lx\n", hr);

    interval = -1;
    hr = IDailyTrigger_get_DaysInterval(daily_trigger, &interval);
    ok(hr == S_OK, "get_DaysInterval failed: %08lx\n", hr);
    ok(interval == 2, "interval = %d\n", interval);

    hr = IDailyTrigger_get_StartBoundary(daily_trigger, NULL);
    ok(hr == E_POINTER, "get_StartBoundary failed: %08lx\n", hr);

    hr = IDailyTrigger_get_EndBoundary(daily_trigger, NULL);
    ok(hr == E_POINTER, "get_EndBoundary failed: %08lx\n", hr);

    start_boundary = (BSTR)0xdeadbeef;
    hr = IDailyTrigger_get_StartBoundary(daily_trigger, &start_boundary);
    ok(hr == S_OK, "get_StartBoundary failed: %08lx\n", hr);
    ok(start_boundary == NULL, "start_boundary not set\n");

    end_boundary = (BSTR)0xdeadbeef;
    hr = IDailyTrigger_get_EndBoundary(daily_trigger, &end_boundary);
    ok(hr == S_OK, "get_EndBoundary failed: %08lx\n", hr);
    ok(end_boundary == NULL, "end_boundary not set\n");

    for (i = 0; i < ARRAY_SIZE(start_test); i++)
    {
        winetest_push_context("%lu", i);
        start_boundary = SysAllocString(start_test[i].str);
        hr = IDailyTrigger_put_StartBoundary(daily_trigger, start_boundary);
        ok(hr == start_test[i].hr, "got %08lx expected %08lx\n", hr, start_test[i].hr);
        SysFreeString(start_boundary);
        if (hr == S_OK)
        {
            start_boundary = NULL;
            hr = IDailyTrigger_get_StartBoundary(daily_trigger, &start_boundary);
            ok(hr == S_OK, "got %08lx\n", hr);
            ok(start_boundary != NULL, "start_boundary not set\n");
            ok(!lstrcmpW(start_boundary, start_test[i].str), "got %s\n", wine_dbgstr_w(start_boundary));
            SysFreeString(start_boundary);
        }

        end_boundary = SysAllocString(start_test[i].end);
        hr = IDailyTrigger_put_EndBoundary(daily_trigger, end_boundary);
        ok(hr == start_test[i].hr, "got %08lx expected %08lx\n", hr, start_test[i].hr);
        SysFreeString(end_boundary);
        if (hr == S_OK)
        {
            end_boundary = NULL;
            hr = IDailyTrigger_get_EndBoundary(daily_trigger, &end_boundary);
            ok(hr == S_OK, "got %08lx\n", hr);
            ok(end_boundary != NULL, "end_boundary not set\n");
            ok(!lstrcmpW(end_boundary, start_test[i].end), "got %s\n", wine_dbgstr_w(end_boundary));
            SysFreeString(end_boundary);
        }
        winetest_pop_context();
    }

    hr = IDailyTrigger_put_StartBoundary(daily_trigger, NULL);
    ok(hr == S_OK, "put_StartBoundary failed: %08lx\n", hr);

    hr = IDailyTrigger_put_EndBoundary(daily_trigger, NULL);
    ok(hr == S_OK, "put_EndBoundary failed: %08lx\n", hr);

    hr = IDailyTrigger_get_Enabled(daily_trigger, NULL);
    ok(hr == E_POINTER, "get_Enabled failed: %08lx\n", hr);

    enabled = VARIANT_FALSE;
    hr = IDailyTrigger_get_Enabled(daily_trigger, &enabled);
    ok(hr == S_OK, "get_Enabled failed: %08lx\n", hr);
    ok(enabled == VARIANT_TRUE, "got %d\n", enabled);

    hr = IDailyTrigger_put_Enabled(daily_trigger, VARIANT_FALSE);
    ok(hr == S_OK, "put_Enabled failed: %08lx\n", hr);

    enabled = VARIANT_TRUE;
    hr = IDailyTrigger_get_Enabled(daily_trigger, &enabled);
    ok(hr == S_OK, "get_Enabled failed: %08lx\n", hr);
    ok(enabled == VARIANT_FALSE, "got %d\n", enabled);

    IDailyTrigger_Release(daily_trigger);
}

static void test_registration_trigger(ITrigger *trigger)
{
    IRegistrationTrigger *reg_trigger;
    VARIANT_BOOL enabled;
    HRESULT hr;

    hr = ITrigger_QueryInterface(trigger, &IID_IRegistrationTrigger, (void**)&reg_trigger);
    ok(hr == S_OK, "Could not get IRegistrationTrigger iface: %08lx\n", hr);

    enabled = VARIANT_FALSE;
    hr = IRegistrationTrigger_get_Enabled(reg_trigger, &enabled);
    ok(hr == S_OK, "get_Enabled failed: %08lx\n", hr);
    ok(enabled == VARIANT_TRUE, "got %d\n", enabled);

    hr = IRegistrationTrigger_put_Enabled(reg_trigger, VARIANT_FALSE);
    ok(hr == S_OK, "put_Enabled failed: %08lx\n", hr);

    enabled = VARIANT_TRUE;
    hr = IRegistrationTrigger_get_Enabled(reg_trigger, &enabled);
    ok(hr == S_OK, "get_Enabled failed: %08lx\n", hr);
    ok(enabled == VARIANT_FALSE, "got %d\n", enabled);

    IRegistrationTrigger_Release(reg_trigger);
}

static void create_action(ITaskDefinition *taskdef)
{
    static WCHAR task1_exe[] = L"task1.exe";
    static WCHAR workdir[] = L"workdir";
    static WCHAR args[] = L"arguments";
    static WCHAR comment[] = L"comment";
    HRESULT hr;
    IActionCollection *actions;
    IAction *action;
    IExecAction *exec_action;
    TASK_ACTION_TYPE type;
    BSTR path, str;

    hr = ITaskDefinition_get_Actions(taskdef, NULL);
    ok(hr == E_POINTER, "got %#lx\n", hr);

    hr = ITaskDefinition_get_Actions(taskdef, &actions);
    ok(hr == S_OK, "get_Actions error %#lx\n", hr);

    hr = IActionCollection_Create(actions, TASK_ACTION_EXEC, &action);
    ok(hr == S_OK, "Create action error %#lx\n", hr);

    hr = IAction_QueryInterface(action, &IID_IExecAction, (void **)&exec_action);
    ok(hr == S_OK, "QueryInterface error %#lx\n", hr);

    type = 0xdeadbeef;
    hr = IExecAction_get_Type(exec_action, &type);
    ok(hr == S_OK, "get_Type error %#lx\n", hr);
    ok(type == TASK_ACTION_EXEC, "got %u\n", type );

    hr = IExecAction_get_Path(exec_action, NULL);
    ok(hr == E_POINTER, "got %#lx\n", hr);

    path = (BSTR)0xdeadbeef;
    hr = IExecAction_get_Path(exec_action, &path);
    ok(hr == S_OK, "get_Path error %#lx\n", hr);
    ok(path == NULL, "path not set\n");

    hr = IExecAction_put_Path(exec_action, NULL);
    ok(hr == S_OK, "put_Path error %#lx\n", hr);

    hr = IExecAction_put_Path(exec_action, task1_exe);
    ok(hr == S_OK, "put_Path error %#lx\n", hr);

    path = NULL;
    hr = IExecAction_get_Path(exec_action, &path);
    ok(hr == S_OK, "get_Path error %#lx\n", hr);
    ok(path != NULL, "path not set\n");
    ok(!lstrcmpW(path, task1_exe), "got %s\n", wine_dbgstr_w(path));
    SysFreeString(path);

    hr = IExecAction_get_WorkingDirectory(exec_action, NULL);
    ok(hr == E_POINTER, "got %#lx\n", hr);

    path = (BSTR)0xdeadbeef;
    hr = IExecAction_get_WorkingDirectory(exec_action, &path);
    ok(hr == S_OK, "get_WorkingDirectory error %#lx\n", hr);
    ok(path == NULL, "workdir not set\n");

    hr = IExecAction_put_WorkingDirectory(exec_action, NULL);
    ok(hr == S_OK, "put_WorkingDirectory error %#lx\n", hr);

    hr = IExecAction_put_WorkingDirectory(exec_action, workdir);
    ok(hr == S_OK, "put_WorkingDirectory error %#lx\n", hr);

    path = NULL;
    hr = IExecAction_get_WorkingDirectory(exec_action, &path);
    ok(hr == S_OK, "get_WorkingDirectory error %#lx\n", hr);
    ok(path != NULL, "workdir not set\n");
    ok(!lstrcmpW(path, workdir), "got %s\n", wine_dbgstr_w(path));
    SysFreeString(path);

    hr = IExecAction_get_Arguments(exec_action, NULL);
    ok(hr == E_POINTER, "got %#lx\n", hr);

    path = (BSTR)0xdeadbeef;
    hr = IExecAction_get_Arguments(exec_action, &path);
    ok(hr == S_OK, "get_Arguments error %#lx\n", hr);
    ok(path == NULL, "args not set\n");

    hr = IExecAction_put_Arguments(exec_action, NULL);
    ok(hr == S_OK, "put_Arguments error %#lx\n", hr);

    hr = IExecAction_put_Arguments(exec_action, args);
    ok(hr == S_OK, "put_Arguments error %#lx\n", hr);

    path = NULL;
    hr = IExecAction_get_Arguments(exec_action, &path);
    ok(hr == S_OK, "get_Arguments error %#lx\n", hr);
    ok(path != NULL, "args not set\n");
    ok(!lstrcmpW(path, args), "got %s\n", wine_dbgstr_w(path));
    SysFreeString(path);


    str = (BSTR)0xdeadbeef;
    hr = IExecAction_get_Id(exec_action, &str);
    ok(hr == S_OK, "get_Id error %#lx\n", hr);
    ok(str == NULL, "id should be NULL\n");

    hr = IExecAction_put_Id(exec_action, NULL);
    ok(hr == S_OK, "put_Id error %#lx\n", hr);

    hr = IExecAction_put_Id(exec_action, comment);
    ok(hr == S_OK, "put_Id error %#lx\n", hr);

    str = NULL;
    hr = IExecAction_get_Id(exec_action, &str);
    ok(hr == S_OK, "get_Id error %#lx\n", hr);
    ok(str != NULL, "should not be NULL\n");
    ok(!lstrcmpW(str, comment), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    IExecAction_Release(exec_action);
    IAction_Release(action);
    IActionCollection_Release(actions);
}

static void test_TaskDefinition(void)
{
    static WCHAR xml0[] = L"";
    static WCHAR xml1[] =
        L"<Task xmlns=\"http://schemas.microsoft.com/windows/2004/02/mit/task\">\n"
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
        "  <Data>MyTestData</Data>\n"
        "</Task>\n";
    static WCHAR xml2[] =
        L"<Task>\n"
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
    static WCHAR xml3[] =
        L"<TASK>\n"
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
    static WCHAR xml4[] =
        L"<Task xmlns=\"http://schemas.microsoft.com/windows/2004/02/mit/task\">\n"
        "  <RegistrationInfo/>\n"
        "  <Settings/>\n"
        "  <Actions>\n"
        "    <Exec>\n"
        "      <Command>\"task1.exe\"</Command>\n"
        "    </Exec>\n"
        "  </Actions>\n"
        "</Task>\n";
    static WCHAR xml5[] =
        L"<Task xmlns=\"http://schemas.microsoft.com/windows/2004/02/mit/task\">\n"
        "  <RegistrationInfo/>\n"
        "  <Settings/>\n"
        "  <Actions/>\n"
        "</Task>\n";
    static WCHAR xml6[] =
        L"<Task xmlns=\"http://schemas.microsoft.com/windows/2004/02/mit/task\">\n"
        "  <RegistrationInfo/>\n"
        "  <Actions>\n"
        "    <Exec>\n"
        "      <Command>\"task1.exe\"</Command>\n"
        "    </Exec>\n"
        "  </Actions>\n"
        "  <Settings>\n"
        "</Task>\n";
    static WCHAR xml7[] =
        L"<Task xmlns=\"http://schemas.microsoft.com/windows/2004/02/mit/task\">\n"
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
    static struct settings def_settings = { L"", L"PT72H", L"",
        0, 7, TASK_INSTANCES_IGNORE_NEW, TASK_COMPATIBILITY_V2, VARIANT_TRUE, VARIANT_TRUE,
        VARIANT_TRUE, VARIANT_TRUE, VARIANT_FALSE, VARIANT_FALSE, VARIANT_TRUE, VARIANT_FALSE,
        VARIANT_FALSE, VARIANT_FALSE };
    static struct settings new_settings = { L"P1Y", L"PT10M", L"",
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

    hr = CoCreateInstance(&CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, &IID_ITaskService, (void **)&service);
    if (hr != S_OK)
    {
        win_skip("CoCreateInstance(CLSID_TaskScheduler) error %#lx\n", hr);
        return;
    }

    hr = ITaskService_NewTask(service, 0, &taskdef);
    ok(hr == S_OK, "NewTask error %#lx\n", hr);

    test_settings_v1(taskdef, &def_settings, &def_settings);
    change_settings(taskdef, &new_settings);

    /* Action is mandatory for v1 compatibility */
    create_action(taskdef);

    hr = ITaskDefinition_get_XmlText(taskdef, &xml);
    ok(hr == S_OK, "get_XmlText error %#lx\n", hr);

    ITaskDefinition_Release(taskdef);

    hr = ITaskService_NewTask(service, 0, &taskdef);
    ok(hr == S_OK, "NewTask error %#lx\n", hr);

    hr = ITaskDefinition_put_XmlText(taskdef, xml);
    ok(hr == S_OK, "put_XmlText error %#lx\n", hr);
    SysFreeString(xml);

    /* FIXME: uncomment once changing settings is implemented
    test_settings_v1(taskdef, &new_settings, &def_settings);
    */

    hr = ITaskDefinition_put_XmlText(taskdef, NULL);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);

    hr = ITaskDefinition_put_XmlText(taskdef, xml1);
    ok(hr == S_OK, "put_XmlText error %#lx\n", hr);

    hr = ITaskDefinition_put_XmlText(taskdef, xml2);
    ok(hr == SCHED_E_NAMESPACE, "expected SCHED_E_NAMESPACE, got %#lx\n", hr);

    hr = ITaskDefinition_put_XmlText(taskdef, xml3);
    todo_wine
    ok(hr == SCHED_E_UNEXPECTEDNODE, "expected SCHED_E_UNEXPECTEDNODE, got %#lx\n", hr);

    hr = ITaskDefinition_put_XmlText(taskdef, xml4);
    ok(hr == S_OK, "put_XmlText error %#lx\n", hr);

    hr = ITaskDefinition_put_XmlText(taskdef, xml5);
    todo_wine
    ok(hr == SCHED_E_MISSINGNODE, "expected SCHED_E_MISSINGNODE, got %#lx\n", hr);

    hr = ITaskDefinition_put_XmlText(taskdef, xml6);
    ok(hr == SCHED_E_MALFORMEDXML, "expected SCHED_E_MALFORMEDXML, got %#lx\n", hr);

    hr = ITaskDefinition_put_XmlText(taskdef, xml7);
    ok(hr == SCHED_E_INVALIDVALUE, "expected SCHED_E_INVALIDVALUE, got %#lx\n", hr);

    hr = ITaskDefinition_put_XmlText(taskdef, xml0);
    ok(hr == SCHED_E_MALFORMEDXML, "expected SCHED_E_MALFORMEDXML, got %#lx\n", hr);

    /* test registration info */
    hr = ITaskDefinition_put_XmlText(taskdef, xml1);
    ok(hr == S_OK, "put_XmlText error %#lx\n", hr);
    hr = ITaskDefinition_get_RegistrationInfo(taskdef, &reginfo);
    ok(hr == S_OK, "get_RegistrationInfo error %#lx\n", hr);

    hr = IRegistrationInfo_get_Description(reginfo, &bstr);
    ok(hr == S_OK, "get_Description error %#lx\n", hr);
    ok(!lstrcmpW(bstr, L"\"Task1\""), "expected \"Task1\", got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = IRegistrationInfo_put_Description(reginfo, NULL);
    ok(hr == S_OK, "put_Description error %#lx\n", hr);
    bstr = (BSTR)0xdeadbeef;
    hr = IRegistrationInfo_get_Description(reginfo, &bstr);
    ok(hr == S_OK, "get_Description error %#lx\n", hr);
    ok(!bstr, "expected NULL, got %s\n", wine_dbgstr_w(bstr));

    hr = IRegistrationInfo_get_Author(reginfo, &bstr);
    ok(hr == S_OK, "get_Author error %#lx\n", hr);
    ok(!lstrcmpW(bstr, L"author"), "expected author, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = IRegistrationInfo_put_Author(reginfo, NULL);
    ok(hr == S_OK, "put_Author error %#lx\n", hr);
    bstr = (BSTR)0xdeadbeef;
    hr = IRegistrationInfo_get_Author(reginfo, &bstr);
    ok(hr == S_OK, "get_Author error %#lx\n", hr);
    ok(!bstr, "expected NULL, got %s\n", wine_dbgstr_w(bstr));

    hr = IRegistrationInfo_get_Version(reginfo, &bstr);
    ok(hr == S_OK, "get_Version error %#lx\n", hr);
    ok(!lstrcmpW(bstr, L"1.0"), "expected 1.0, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = IRegistrationInfo_put_Version(reginfo, NULL);
    ok(hr == S_OK, "put_Version error %#lx\n", hr);
    bstr = (BSTR)0xdeadbeef;
    hr = IRegistrationInfo_get_Version(reginfo, &bstr);
    ok(hr == S_OK, "get_Version error %#lx\n", hr);
    ok(!bstr, "expected NULL, got %s\n", wine_dbgstr_w(bstr));

    hr = IRegistrationInfo_get_Date(reginfo, &bstr);
    ok(hr == S_OK, "get_Date error %#lx\n", hr);
    ok(!lstrcmpW(bstr, L"2018-04-02T11:22:33"), "expected 2018-04-02T11:22:33, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = IRegistrationInfo_put_Date(reginfo, NULL);
    ok(hr == S_OK, "put_Date error %#lx\n", hr);
    bstr = (BSTR)0xdeadbeef;
    hr = IRegistrationInfo_get_Date(reginfo, &bstr);
    ok(hr == S_OK, "get_Date error %#lx\n", hr);
    ok(!bstr, "expected NULL, got %s\n", wine_dbgstr_w(bstr));

    hr = IRegistrationInfo_get_Documentation(reginfo, &bstr);
    ok(hr == S_OK, "get_Documentation error %#lx\n", hr);
    ok(!lstrcmpW(bstr, L"doc"), "expected doc, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = IRegistrationInfo_put_Documentation(reginfo, NULL);
    ok(hr == S_OK, "put_Documentation error %#lx\n", hr);
    bstr = (BSTR)0xdeadbeef;
    hr = IRegistrationInfo_get_Documentation(reginfo, &bstr);
    ok(hr == S_OK, "get_Documentation error %#lx\n", hr);
    ok(!bstr, "expected NULL, got %s\n", wine_dbgstr_w(bstr));

    hr = IRegistrationInfo_get_URI(reginfo, &bstr);
    ok(hr == S_OK, "get_URI error %#lx\n", hr);
    ok(!lstrcmpW(bstr, L"uri"), "expected uri, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = IRegistrationInfo_put_URI(reginfo, NULL);
    ok(hr == S_OK, "put_URI error %#lx\n", hr);
    bstr = (BSTR)0xdeadbeef;
    hr = IRegistrationInfo_get_URI(reginfo, &bstr);
    ok(hr == S_OK, "get_URI error %#lx\n", hr);
    ok(!bstr, "expected NULL, got %s\n", wine_dbgstr_w(bstr));

    hr = IRegistrationInfo_get_Source(reginfo, &bstr);
    ok(hr == S_OK, "get_Source error %#lx\n", hr);
    ok(!lstrcmpW(bstr, L"source"), "expected source, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    hr = IRegistrationInfo_put_Source(reginfo, NULL);
    ok(hr == S_OK, "put_Source error %#lx\n", hr);
    bstr = (BSTR)0xdeadbeef;
    hr = IRegistrationInfo_get_Source(reginfo, &bstr);
    ok(hr == S_OK, "get_Source error %#lx\n", hr);
    ok(!bstr, "expected NULL, got %s\n", wine_dbgstr_w(bstr));

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = NULL;
    hr = IRegistrationInfo_get_SecurityDescriptor(reginfo, &var);
    todo_wine
    ok(hr == S_OK, "get_SecurityDescriptor error %#lx\n", hr);
    if (hr == S_OK)
        ok(V_VT(&var) == VT_EMPTY, "expected VT_EMPTY, got %u\n", V_VT(&var));

    IRegistrationInfo_Release(reginfo);

    hr = ITaskDefinition_get_Data(taskdef, &bstr);
    ok(hr == S_OK, "get_Data error %#lx\n", hr);
    ok(!lstrcmpW(bstr, L"MyTestData"), "expected \"MyTestData\", got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    hr = ITaskDefinition_put_Data(taskdef, (BSTR) L"NewTest");
    ok(hr == S_OK, "put_Data error %#lx\n", hr);
    bstr = (BSTR)0xdeadbeef;
    hr = ITaskDefinition_get_Data(taskdef, &bstr);
    ok(!lstrcmpW(bstr, L"NewTest"), "expected \"NewTest\", got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    hr = ITaskDefinition_put_Data(taskdef, NULL);
    ok(hr == S_OK, "put_Data error %#lx\n", hr);
    bstr = (BSTR)0xdeadbeef;
    hr = ITaskDefinition_get_Data(taskdef, &bstr);
    ok(hr == S_OK, "get_Data error %#lx\n", hr);
    ok(!bstr, "expected NULL, got %s\n", wine_dbgstr_w(bstr));

    hr = ITaskDefinition_put_XmlText(taskdef, xml4);
    ok(hr == S_OK, "put_XmlText error %#lx\n", hr);
    hr = ITaskDefinition_get_RegistrationInfo(taskdef, &reginfo);
    ok(hr == S_OK, "get_RegistrationInfo error %#lx\n", hr);

    hr = IRegistrationInfo_get_Description(reginfo, &bstr);
    ok(hr == S_OK, "get_Description error %#lx\n", hr);
    ok(!bstr, "expected NULL, got %s\n", wine_dbgstr_w(bstr));

    hr = ITaskDefinition_get_Triggers(taskdef, &trigger_col);
    ok(hr == S_OK, "get_Triggers failed: %08lx\n", hr);
    ok(trigger_col != NULL, "Triggers = NULL\n");

    hr = ITriggerCollection_Create(trigger_col, TASK_TRIGGER_DAILY, &trigger);
    ok(hr == S_OK, "Create failed: %08lx\n", hr);
    ok(trigger != NULL, "trigger = NULL\n");
    test_daily_trigger(trigger);
    ITrigger_Release(trigger);

    hr = ITriggerCollection_Create(trigger_col, TASK_TRIGGER_REGISTRATION, &trigger);
    ok(hr == S_OK, "Create failed: %08lx\n", hr);
    ok(trigger != NULL, "trigger = NULL\n");
    test_registration_trigger(trigger);
    ITrigger_Release(trigger);
    ITriggerCollection_Release(trigger_col);

    hr = ITaskDefinition_get_Triggers(taskdef, &trigger_col2);
    ok(hr == S_OK, "get_Triggers failed: %08lx\n", hr);
    ok(trigger_col == trigger_col2, "Mismatched triggers\n");
    ITriggerCollection_Release(trigger_col2);

    IRegistrationInfo_Release(reginfo);
    ITaskDefinition_Release(taskdef);
    ITaskService_Release(service);
}

static void test_get_Count_and_Item(void)
{
    HRESULT hr;
    LONG num_tasks;
    VARIANT v_null, index;
    ITaskService *service;
    ITaskFolder *folder, *root;
    ITaskDefinition *task_def;
    IRegistrationInfo *reg_info;
    IActionCollection *actions;
    IAction *action;
    IExecAction *exec_action;
    IRegisteredTask *task1, *ret_task1;
    IRegisteredTaskCollection *tasks;
    BSTR ret_task1_name = NULL;

    V_VT(&v_null) = VT_NULL;

    CoCreateInstance(&CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, &IID_ITaskService, (void**)&service);
    ITaskService_Connect(service, v_null, v_null, v_null, v_null);

    ITaskService_GetFolder(service, (BSTR)L"\\", &root);
    ITaskFolder_CreateFolder(root,(BSTR)L"\\Wine", v_null, &folder);

    ITaskService_NewTask(service, 0, &task_def);
    ITaskDefinition_get_RegistrationInfo(task_def, &reg_info);
    IRegistrationInfo_put_Author(reg_info, (BSTR)L"Wine Test");

    ITaskDefinition_get_Actions(task_def, &actions);
    IActionCollection_Create(actions, TASK_ACTION_EXEC, &action);
    IAction_QueryInterface(action, &IID_IExecAction, (void**)&exec_action);
    IExecAction_put_Path(exec_action, (BSTR)L"task1.exe");

    ITaskFolder_RegisterTaskDefinition(folder, (BSTR)L"Task1", task_def, TASK_CREATE, v_null, v_null, TASK_LOGON_NONE, v_null, &task1);
    ITaskFolder_GetTasks(folder, TASK_ENUM_HIDDEN, &tasks);

    /* Test get_Count */
    hr = IRegisteredTaskCollection_get_Count(tasks, NULL);
    ok(hr == E_POINTER, "expected E_POINTER, got %#lx\n", hr);

    hr = IRegisteredTaskCollection_get_Count(tasks, &num_tasks);
    ok(hr == S_OK, "expected S_OK, got %#lx\n", hr);
    ok(num_tasks == 1, "expected 1 task, got %ld\n", num_tasks);

    /* Test get_Item */
    VariantInit(&index);
    index.vt = VT_UI4;
    index.uiVal = 1;

    hr = IRegisteredTaskCollection_get_Item(tasks, index, NULL);
    ok(hr == E_POINTER, "expected E_POINTER, got %#lx\n", hr);

    index.uiVal = 0;
    hr = IRegisteredTaskCollection_get_Item(tasks, index, &ret_task1);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);

    index.uiVal = 2;
    hr = IRegisteredTaskCollection_get_Item(tasks, index, &ret_task1);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);

    index.uiVal = 1;
    hr = IRegisteredTaskCollection_get_Item(tasks, index, &ret_task1);
    ok(hr == S_OK || broken(hr == E_INVALIDARG) /* Win7 */, "expected S_OK, got %#lx\n", hr);

    if (hr == S_OK)
    {
        IRegisteredTask_get_Name(ret_task1, &ret_task1_name);
        ok(!lstrcmpW(L"Task1", ret_task1_name), "expected name \"Task1\", got %ls\n", ret_task1_name);
    }

    ITaskFolder_DeleteTask(folder, (BSTR)L"Task1", 0);
    ITaskFolder_DeleteFolder(root, (BSTR)L"\\Wine", 0);
    IRegisteredTask_Release(task1);
    ITaskFolder_Release(folder);
    ITaskFolder_Release(root);
    IExecAction_Release(exec_action);
    IRegistrationInfo_Release(reg_info);
    ITaskDefinition_Release(task_def);
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
    test_get_Count_and_Item();

    OleUninitialize();
}
