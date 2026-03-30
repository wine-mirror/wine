/*
 * Copyright 2011 Jacek Caban for CodeWeavers
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
#define CONST_VTABLE

#define PSAPI_VERSION 1
#include <initguid.h>
#include <windows.h>
#include <psapi.h>
#include <oaidl.h>

#include "wine/test.h"

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CLEAR_CALLED(func) \
    expect_ ## func = called_ ## func = FALSE

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func); \
        expect_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

DEFINE_EXPECT(reportSuccess);

#define DISPID_TESTOBJ_OK                        10000
#define DISPID_TESTOBJ_TRACE                     10001
#define DISPID_TESTOBJ_REPORTSUCCESS             10002
#define DISPID_TESTOBJ_WSCRIPTFULLNAME           10003
#define DISPID_TESTOBJ_WSCRIPTPATH               10004
#define DISPID_TESTOBJ_WSCRIPTSCRIPTNAME         10005
#define DISPID_TESTOBJ_WSCRIPTSCRIPTFULLNAME     10006

#define TESTOBJ_CLSID "{178fc166-f585-4e24-9c13-4bb7faf80646}"

static const GUID CLSID_TestObj =
    {0x178fc166,0xf585,0x4e24,{0x9c,0x13,0x4b,0xb7,0xfa,0xf8,0x06,0x46}};

static const char *script_name;
static HANDLE wscript_process;

static BSTR a2bstr(const char *str)
{
    BSTR ret;
    int len;

    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    ret = SysAllocStringLen(NULL, len-1);
    MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);

    return ret;
}

static HRESULT WINAPI Dispatch_QueryInterface(IDispatch *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDispatch)) {
        *ppv = iface;
	return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI Dispatch_AddRef(IDispatch *iface)
{
    return 2;
}

static ULONG WINAPI Dispatch_Release(IDispatch *iface)
{
    return 1;
}

static HRESULT WINAPI Dispatch_GetTypeInfoCount(IDispatch *iface, UINT *pctinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Dispatch_GetTypeInfo(IDispatch *iface, UINT iTInfo,
	LCID lcid, ITypeInfo **ppTInfo)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI Dispatch_GetIDsOfNames(IDispatch *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    unsigned i;

    for(i=0; i<cNames; i++) {
        if(!lstrcmpW(rgszNames[i], L"ok")) {
            rgDispId[i] = DISPID_TESTOBJ_OK;
        }else if(!lstrcmpW(rgszNames[i], L"trace")) {
            rgDispId[i] = DISPID_TESTOBJ_TRACE;
        }else if(!lstrcmpW(rgszNames[i], L"reportSuccess")) {
            rgDispId[i] = DISPID_TESTOBJ_REPORTSUCCESS;
        }else if(!lstrcmpW(rgszNames[i], L"wscriptFullName")) {
            rgDispId[i] = DISPID_TESTOBJ_WSCRIPTFULLNAME;
        }else if(!lstrcmpW(rgszNames[i], L"wscriptPath")) {
            rgDispId[i] = DISPID_TESTOBJ_WSCRIPTPATH;
        }else if(!lstrcmpW(rgszNames[i], L"wscriptScriptName")) {
            rgDispId[i] = DISPID_TESTOBJ_WSCRIPTSCRIPTNAME;
        }else if(!lstrcmpW(rgszNames[i], L"wscriptScriptFullName")) {
            rgDispId[i] = DISPID_TESTOBJ_WSCRIPTSCRIPTFULLNAME;
        }else {
            ok(0, "unexpected name %s\n", wine_dbgstr_w(rgszNames[i]));
            return DISP_E_UNKNOWNNAME;
        }
    }

    return S_OK;
}

static HRESULT WINAPI Dispatch_Invoke(IDispatch *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
				      WORD wFlags, DISPPARAMS *pdp, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    switch(dispIdMember) {
    case DISPID_TESTOBJ_OK: {
        VARIANT *expr, *msg;

        ok(wFlags == INVOKE_FUNC, "wFlags = %x\n", wFlags);
        ok(pdp->cArgs == 2, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);

        expr = pdp->rgvarg+1;
        if(V_VT(expr) == (VT_VARIANT|VT_BYREF))
            expr = V_VARIANTREF(expr);

        msg = pdp->rgvarg;
        if(V_VT(msg) == (VT_VARIANT|VT_BYREF))
            msg = V_VARIANTREF(msg);

        ok(V_VT(msg) == VT_BSTR, "V_VT(psp->rgvargs) = %d\n", V_VT(msg));
        ok(V_VT(expr) == VT_BOOL, "V_VT(psp->rgvargs+1) = %d\n", V_VT(expr));
        ok(V_BOOL(expr), "%s: %s\n", script_name, wine_dbgstr_w(V_BSTR(msg)));
        if(pVarResult)
            V_VT(pVarResult) = VT_EMPTY;
        break;
    }
    case DISPID_TESTOBJ_TRACE:
        ok(wFlags == INVOKE_FUNC, "wFlags = %x\n", wFlags);
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(V_VT(pdp->rgvarg) == VT_BSTR, "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));
        trace("%s: %s\n", script_name, wine_dbgstr_w(V_BSTR(pdp->rgvarg)));
        if(pVarResult)
            V_VT(pVarResult) = VT_EMPTY;
        break;
    case DISPID_TESTOBJ_REPORTSUCCESS:
        CHECK_EXPECT(reportSuccess);

        ok(wFlags == INVOKE_FUNC, "wFlags = %x\n", wFlags);
        ok(pdp->cArgs == 0, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        if(pVarResult)
            V_VT(pVarResult) = VT_EMPTY;
        break;
    case DISPID_TESTOBJ_WSCRIPTFULLNAME:
    {
        WCHAR fullName[MAX_PATH];
        DWORD res;

        ok(wFlags == INVOKE_PROPERTYGET, "wFlags = %x\n", wFlags);
        ok(pdp->cArgs == 0, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        V_VT(pVarResult) = VT_BSTR;
        res = GetModuleFileNameExW(wscript_process, NULL, fullName, ARRAY_SIZE(fullName));
        if(res == 0)
            return E_FAIL;
        if(!(V_BSTR(pVarResult) = SysAllocString(fullName)))
            return E_OUTOFMEMORY;
        break;
    }
    case DISPID_TESTOBJ_WSCRIPTPATH:
    {
        WCHAR fullPath[MAX_PATH];
        DWORD res;
        const WCHAR *pos;

        ok(wFlags == INVOKE_PROPERTYGET, "wFlags = %x\n", wFlags);
        ok(pdp->cArgs == 0, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        V_VT(pVarResult) = VT_BSTR;
        res = GetModuleFileNameExW(wscript_process, NULL, fullPath, ARRAY_SIZE(fullPath));
        if(res == 0)
            return E_FAIL;
        pos = wcsrchr(fullPath, '\\');
        if(!(V_BSTR(pVarResult) = SysAllocStringLen(fullPath, pos-fullPath)))
            return E_OUTOFMEMORY;
        break;
    }
    case DISPID_TESTOBJ_WSCRIPTSCRIPTNAME:
    {
        char fullPath[MAX_PATH];
        char *pos;
        long res;

        ok(wFlags == INVOKE_PROPERTYGET, "wFlags = %x\n", wFlags);
        ok(pdp->cArgs == 0, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        V_VT(pVarResult) = VT_BSTR;
        res = GetFullPathNameA(script_name, sizeof(fullPath), fullPath, &pos);
        if(!res || res > sizeof(fullPath))
            return E_FAIL;
        if(!(V_BSTR(pVarResult) = a2bstr(pos)))
            return E_OUTOFMEMORY;
        break;
    }
    case DISPID_TESTOBJ_WSCRIPTSCRIPTFULLNAME:
    {
        char fullPath[MAX_PATH];
        long res;

        ok(wFlags == INVOKE_PROPERTYGET, "wFlags = %x\n", wFlags);
        ok(pdp->cArgs == 0, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        V_VT(pVarResult) = VT_BSTR;
        res = GetFullPathNameA(script_name, sizeof(fullPath), fullPath, NULL);
        if(!res || res > sizeof(fullPath))
            return E_FAIL;
        if(!(V_BSTR(pVarResult) = a2bstr(fullPath)))
            return E_OUTOFMEMORY;
        break;
    }
    default:
        ok(0, "unexpected dispIdMember %ld\n", dispIdMember);
        return E_NOTIMPL;
    }

    return S_OK;
}

static IDispatchVtbl testobj_vtbl = {
    Dispatch_QueryInterface,
    Dispatch_AddRef,
    Dispatch_Release,
    Dispatch_GetTypeInfoCount,
    Dispatch_GetTypeInfo,
    Dispatch_GetIDsOfNames,
    Dispatch_Invoke
};

static IDispatch testobj = { &testobj_vtbl };

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IClassFactory, riid)) {
        *ppv = iface;
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI ClassFactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    ok(!outer, "outer = %p\n", outer);
    return IDispatch_QueryInterface(&testobj, riid, ppv);
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL dolock)
{
    return S_OK;
}

static const IClassFactoryVtbl ClassFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ClassFactory_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory testobj_cf = { &ClassFactoryVtbl };

static void run_script_file(const char *file_name, DWORD expected_exit_code)
{
    char command[MAX_PATH];
    SECURITY_ATTRIBUTES sa = {sizeof(sa), NULL, TRUE};
    STARTUPINFOA si = {sizeof(si)};
    PROCESS_INFORMATION pi;
    HANDLE stdout_read, stdout_write;
    HANDLE stderr_read, stderr_write;
    char stdout_buf[4096], stderr_buf[4096];
    DWORD exit_code, size;
    BOOL bres;

    script_name = file_name;
    sprintf(command, "wscript.exe %s arg1 2 ar3", file_name);

    bres = CreatePipe(&stdout_read, &stdout_write, &sa, 0);
    ok(bres, "CreatePipe failed: %lu\n", GetLastError());
    if(!bres)
        return;

    bres = CreatePipe(&stderr_read, &stderr_write, &sa, 0);
    ok(bres, "CreatePipe failed: %lu\n", GetLastError());
    if(!bres) {
        CloseHandle(stdout_read);
        CloseHandle(stdout_write);
        return;
    }

    SetHandleInformation(stdout_read, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(stderr_read, HANDLE_FLAG_INHERIT, 0);

    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = stdout_write;
    si.hStdError = stderr_write;

    SET_EXPECT(reportSuccess);

    bres = CreateProcessA(NULL, command, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
    CloseHandle(stdout_write);
    CloseHandle(stderr_write);
    if(!bres) {
        win_skip("script.exe is not available\n");
        CLEAR_CALLED(reportSuccess);
        CloseHandle(stdout_read);
        CloseHandle(stderr_read);
        return;
    }

    wscript_process = pi.hProcess;
    WaitForSingleObject(pi.hProcess, INFINITE);

    bres = GetExitCodeProcess(pi.hProcess, &exit_code);
    ok(bres, "GetExitCodeProcess failed: %lu\n", GetLastError());
    ok(exit_code == expected_exit_code, "exit_code = %lu, expected %lu\n", exit_code, expected_exit_code);

    memset(stderr_buf, 0, sizeof(stderr_buf));
    ReadFile(stderr_read, stderr_buf, sizeof(stderr_buf) - 1, &size, NULL);
    stderr_buf[size] = 0;
    ok(stderr_buf[0] == 0, "expected no stderr output, got: %s\n", stderr_buf);

    /* Drain stdout to prevent child from blocking on a full pipe */
    ReadFile(stdout_read, stdout_buf, sizeof(stdout_buf) - 1, &size, NULL);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(stdout_read);
    CloseHandle(stderr_read);

    CHECK_CALLED(reportSuccess);
}

static void run_script(const char *name, const char *script_data, size_t script_size, DWORD expected_exit_code)
{
    char file_name[MAX_PATH];
    const char *ext;
    HANDLE file;
    DWORD size;
    BOOL res;

    ext = strrchr(name, '.');
    ok(ext != NULL, "no script extension\n");
    if(!ext)
      return;

    sprintf(file_name, "test%s", ext);

    file = CreateFileA(file_name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed: %lu\n", GetLastError());
    if(file == INVALID_HANDLE_VALUE)
        return;

    res = WriteFile(file, script_data, script_size, &size, NULL);
    CloseHandle(file);
    ok(res, "Could not write to file: %lu\n", GetLastError());
    if(!res)
        return;

    run_script_file(file_name, expected_exit_code);

    DeleteFileA(file_name);
}

static void run_simple_script(const char *script, DWORD expected_exit_code)
{
    run_script("simple.js", script, strlen(script), expected_exit_code);
}

static BOOL WINAPI test_enum_proc(HMODULE module, LPCSTR type, LPSTR name, LONG_PTR param)
{
    const char *script_data;
    DWORD script_size;
    HRSRC src;

    trace("running %s test...\n", name);

    src = FindResourceA(NULL, name, type);
    ok(src != NULL, "Could not find resource %s: %lu\n", name, GetLastError());
    if(!src)
        return TRUE;

    script_data = LoadResource(NULL, src);
    script_size = SizeofResource(NULL, src);
    while(script_size && !script_data[script_size-1])
        script_size--;

    run_script(name, script_data, script_size, 0);
    return TRUE;
}

static BOOL init_key(const char *key_name, const char *def_value, BOOL init)
{
    HKEY hkey;
    DWORD res;

    if(!init) {
        RegDeleteKeyA(HKEY_CLASSES_ROOT, key_name);
        return TRUE;
    }

    res = RegCreateKeyA(HKEY_CLASSES_ROOT, key_name, &hkey);
    if(res != ERROR_SUCCESS)
        return FALSE;

    if(def_value)
        res = RegSetValueA(hkey, NULL, REG_SZ, def_value, strlen(def_value));

    RegCloseKey(hkey);
    return res == ERROR_SUCCESS;
}

static BOOL init_registry(BOOL init)
{
    return init_key("Wine.Test\\CLSID", TESTOBJ_CLSID, init);
}

static BOOL register_activex(void)
{
    DWORD regid;
    HRESULT hres;

    if(!init_registry(TRUE)) {
        init_registry(FALSE);
        return FALSE;
    }

    hres = CoRegisterClassObject(&CLSID_TestObj, (IUnknown *)&testobj_cf,
            CLSCTX_SERVER, REGCLS_MULTIPLEUSE, &regid);
    ok(hres == S_OK, "Could not register script engine: %08lx\n", hres);
    return TRUE;
}

static void run_cscript_error_test(void)
{
    static const char script_data[] = "y = \"hello\" + 1\n";
    char file_name[] = "test_err.vbs";
    char command[MAX_PATH];
    SECURITY_ATTRIBUTES sa = {sizeof(sa), NULL, TRUE};
    STARTUPINFOA si = {sizeof(si)};
    PROCESS_INFORMATION pi;
    HANDLE stdout_read, stdout_write;
    HANDLE stderr_read, stderr_write;
    char stdout_buf[4096], stderr_buf[4096];
    DWORD exit_code, size;
    HANDLE file;
    BOOL bres;

    file = CreateFileA(file_name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed: %lu\n", GetLastError());
    if(file == INVALID_HANDLE_VALUE)
        return;

    bres = WriteFile(file, script_data, sizeof(script_data) - 1, &size, NULL);
    CloseHandle(file);
    ok(bres, "Could not write to file: %lu\n", GetLastError());
    if(!bres)
        goto cleanup;

    bres = CreatePipe(&stdout_read, &stdout_write, &sa, 0);
    ok(bres, "CreatePipe failed: %lu\n", GetLastError());
    if(!bres)
        goto cleanup;

    bres = CreatePipe(&stderr_read, &stderr_write, &sa, 0);
    ok(bres, "CreatePipe failed: %lu\n", GetLastError());
    if(!bres) {
        CloseHandle(stdout_read);
        CloseHandle(stdout_write);
        goto cleanup;
    }

    SetHandleInformation(stdout_read, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(stderr_read, HANDLE_FLAG_INHERIT, 0);

    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = stdout_write;
    si.hStdError = stderr_write;

    sprintf(command, "cscript.exe //nologo %s", file_name);
    bres = CreateProcessA(NULL, command, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
    CloseHandle(stdout_write);
    CloseHandle(stderr_write);
    if(!bres) {
        win_skip("cscript.exe is not available\n");
        CloseHandle(stdout_read);
        CloseHandle(stderr_read);
        goto cleanup;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    bres = GetExitCodeProcess(pi.hProcess, &exit_code);
    ok(bres, "GetExitCodeProcess failed: %lu\n", GetLastError());
    ok(exit_code == 0, "exit_code = %lu\n", exit_code);

    memset(stdout_buf, 0, sizeof(stdout_buf));
    ReadFile(stdout_read, stdout_buf, sizeof(stdout_buf) - 1, &size, NULL);
    stdout_buf[size] = 0;
    ok(stdout_buf[0] == 0, "expected no stdout with //nologo, got: %s\n", stdout_buf);

    memset(stderr_buf, 0, sizeof(stderr_buf));
    ReadFile(stderr_read, stderr_buf, sizeof(stderr_buf) - 1, &size, NULL);
    stderr_buf[size] = 0;

    ok(size > 0, "expected error output on stderr, got nothing\n");
    ok(strstr(stderr_buf, "test_err.vbs(1,") != NULL,
       "expected file and line reference in error, got: %s\n", stderr_buf);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(stdout_read);
    CloseHandle(stderr_read);

cleanup:
    DeleteFileA(file_name);
}

static DWORD run_cscript(const char *args, char *stdout_buf, size_t stdout_buf_size,
                         char *stderr_buf, size_t stderr_buf_size)
{
    char command[MAX_PATH];
    SECURITY_ATTRIBUTES sa = {sizeof(sa), NULL, TRUE};
    STARTUPINFOA si = {sizeof(si)};
    PROCESS_INFORMATION pi;
    HANDLE stdout_read, stdout_write;
    HANDLE stderr_read, stderr_write;
    DWORD exit_code, size;
    BOOL bres;

    bres = CreatePipe(&stderr_read, &stderr_write, &sa, 0);
    ok(bres, "CreatePipe failed: %lu\n", GetLastError());
    if(!bres)
        return ~0u;

    bres = CreatePipe(&stdout_read, &stdout_write, &sa, 0);
    ok(bres, "CreatePipe failed: %lu\n", GetLastError());
    if(!bres) {
        CloseHandle(stderr_read);
        CloseHandle(stderr_write);
        return ~0u;
    }

    SetHandleInformation(stderr_read, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(stdout_read, HANDLE_FLAG_INHERIT, 0);

    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = stdout_write;
    si.hStdError = stderr_write;

    sprintf(command, "cscript.exe %s", args);
    bres = CreateProcessA(NULL, command, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
    CloseHandle(stderr_write);
    CloseHandle(stdout_write);
    if(!bres) {
        win_skip("cscript.exe is not available\n");
        CloseHandle(stderr_read);
        CloseHandle(stdout_read);
        return ~0u;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    bres = GetExitCodeProcess(pi.hProcess, &exit_code);
    ok(bres, "GetExitCodeProcess failed: %lu\n", GetLastError());

    memset(stdout_buf, 0, stdout_buf_size);
    ReadFile(stdout_read, stdout_buf, stdout_buf_size - 1, &size, NULL);
    stdout_buf[size] = 0;

    memset(stderr_buf, 0, stderr_buf_size);
    ReadFile(stderr_read, stderr_buf, stderr_buf_size - 1, &size, NULL);
    stderr_buf[size] = 0;

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(stdout_read);
    CloseHandle(stderr_read);
    return exit_code;
}

static void run_cscript_unknown_option_test(void)
{
    char stdout_buf[4096], stderr_buf[4096];
    DWORD exit_code;

    exit_code = run_cscript("//nologo /unknownoption", stdout_buf, sizeof(stdout_buf),
                            stderr_buf, sizeof(stderr_buf));
    if(exit_code == ~0u)
        return;
    ok(exit_code == 1, "exit_code = %lu, expected 1\n", exit_code);
    /* On Windows, cscript reports a localized error on stdout. On Wine,
     * unrecognized single-/ options are treated as filenames to allow Unix-style
     * paths (/tmp/foo.vbs), so the error will be about the file instead. */
    ok(stdout_buf[0] != 0, "expected error message on stdout\n");
    ok(stderr_buf[0] == 0, "expected no stderr output, got: %s\n", stderr_buf);
}

static void run_cscript_no_file_test(void)
{
    char stdout_buf[4096], stderr_buf[4096];
    DWORD exit_code;

    exit_code = run_cscript("//nologo", stdout_buf, sizeof(stdout_buf),
                            stderr_buf, sizeof(stderr_buf));
    if(exit_code == ~0u)
        return;
    ok(exit_code == 1, "exit_code = %lu, expected 1\n", exit_code);
    ok(stdout_buf[0] != 0, "expected error message on stdout\n");
}

static void run_cscript_usage_test(void)
{
    char stdout_buf[4096], stderr_buf[4096];
    DWORD exit_code;

    exit_code = run_cscript("", stdout_buf, sizeof(stdout_buf),
                            stderr_buf, sizeof(stderr_buf));
    if(exit_code == ~0u)
        return;
    ok(exit_code == 0, "exit_code = %lu, expected 0\n", exit_code);
    ok(stdout_buf[0] != 0, "expected usage output on stdout\n");
}

static BOOL create_temp_vbs(const char *file_name, const char *data)
{
    HANDLE file;
    DWORD size;
    BOOL bres;

    file = CreateFileA(file_name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed: %lu\n", GetLastError());
    if(file == INVALID_HANDLE_VALUE)
        return FALSE;

    bres = WriteFile(file, data, strlen(data), &size, NULL);
    CloseHandle(file);
    ok(bres, "Could not write to file: %lu\n", GetLastError());
    return bres;
}

static void run_cscript_logo_test(void)
{
    static const char empty_script[] = "\n";
    char file_name[] = "test_logo.vbs";
    char stdout_buf[4096], stderr_buf[4096];
    DWORD exit_code;
    size_t len;

    if(!create_temp_vbs(file_name, empty_script))
        goto cleanup;

    /* Without //nologo: banner should be shown on stdout */
    exit_code = run_cscript(file_name, stdout_buf, sizeof(stdout_buf),
                            stderr_buf, sizeof(stderr_buf));
    if(exit_code == ~0u)
        goto cleanup;
    ok(exit_code == 0, "exit_code = %lu, expected 0\n", exit_code);
    ok(stdout_buf[0] != 0, "expected banner in stdout\n");

    /* Banner should be followed by a blank line */
    ok(strstr(stdout_buf, "\r\n\r\n") != NULL,
       "expected blank line after banner, got: %s\n", stdout_buf);

    /* Output should end with a blank line (\r\n\r\n at the end) */
    len = strlen(stdout_buf);
    ok(len >= 4 && !memcmp(stdout_buf + len - 4, "\r\n\r\n", 4),
       "expected trailing blank line, got: %s\n", stdout_buf);

cleanup:
    DeleteFileA(file_name);
}

static void run_cscript_nologo_test(void)
{
    static const char empty_script[] = "\n";
    char file_name[] = "test_nologo.vbs";
    char stdout_buf[4096], stderr_buf[4096];
    DWORD exit_code;

    if(!create_temp_vbs(file_name, empty_script))
        goto cleanup;

    /* With //nologo: no banner, no output for empty script */
    exit_code = run_cscript("//nologo test_nologo.vbs", stdout_buf, sizeof(stdout_buf),
                            stderr_buf, sizeof(stderr_buf));
    if(exit_code == ~0u)
        goto cleanup;
    ok(exit_code == 0, "exit_code = %lu, expected 0\n", exit_code);
    ok(stdout_buf[0] == 0, "expected no stdout output with //nologo, got: %s\n", stdout_buf);

cleanup:
    DeleteFileA(file_name);
}

static void run_cscript_host_option_after_filename_test(void)
{
    /* Script that echoes its argument count */
    static const char script_data[] = "WScript.Echo WScript.Arguments.Count\n";
    char file_name[] = "test_args.vbs";
    char stdout_buf[4096], stderr_buf[4096];
    DWORD exit_code;

    if(!create_temp_vbs(file_name, script_data))
        goto cleanup;

    /* //nologo after filename should be consumed as host option, not passed as arg */
    exit_code = run_cscript("test_args.vbs //nologo", stdout_buf, sizeof(stdout_buf),
                            stderr_buf, sizeof(stderr_buf));
    if(exit_code == ~0u)
        goto cleanup;
    ok(exit_code == 0, "exit_code = %lu, expected 0\n", exit_code);
    /* No banner (//nologo consumed) and arg count = 0 */
    ok(stdout_buf[0] == '0',
       "expected '0' as first output (no banner, //nologo consumed as host option), got: %s\n", stdout_buf);

    /* //nologo between script args should be consumed */
    exit_code = run_cscript("//nologo test_args.vbs arg1 //b arg2", stdout_buf, sizeof(stdout_buf),
                            stderr_buf, sizeof(stderr_buf));
    if(exit_code == ~0u)
        goto cleanup;
    ok(exit_code == 0, "exit_code = %lu, expected 0\n", exit_code);
    /* //b suppresses output AND is consumed: arg count should be 2 (arg1 + arg2) */
    ok(stdout_buf[0] == 0 || strstr(stdout_buf, "2") != NULL,
       "//b between args should be consumed, got: %s\n", stdout_buf);

cleanup:
    DeleteFileA(file_name);
}

static void run_cscript_nonexistent_file_test(void)
{
    char stdout_buf[4096], stderr_buf[4096];
    DWORD exit_code;

    /* Nonexistent file with //nologo: error on stdout */
    exit_code = run_cscript("//nologo nonexistent_file.vbs", stdout_buf, sizeof(stdout_buf),
                            stderr_buf, sizeof(stderr_buf));
    if(exit_code == ~0u)
        return;
    ok(exit_code == 1, "exit_code = %lu, expected 1\n", exit_code);
    ok(strstr(stdout_buf, "nonexistent_file") != NULL,
       "expected filename in error on stdout, got: %s\n", stdout_buf);

    /* Nonexistent file without //nologo: banner + error on stdout */
    exit_code = run_cscript("nonexistent_file.vbs", stdout_buf, sizeof(stdout_buf),
                            stderr_buf, sizeof(stderr_buf));
    if(exit_code == ~0u)
        return;
    ok(exit_code == 1, "exit_code = %lu, expected 1\n", exit_code);
    ok(strstr(stdout_buf, "nonexistent_file") != NULL,
       "expected filename in error on stdout, got: %s\n", stdout_buf);
}

static void run_cscript_error_on_stdout_test(void)
{
    char stdout_buf[4096], stderr_buf[4096];
    DWORD exit_code;

    /* Unknown option without //nologo: banner + error on stdout */
    exit_code = run_cscript("/unknownoption", stdout_buf, sizeof(stdout_buf),
                            stderr_buf, sizeof(stderr_buf));
    if(exit_code == ~0u)
        return;
    ok(exit_code == 1, "exit_code = %lu, expected 1\n", exit_code);
    ok(stdout_buf[0] != 0, "expected banner and error on stdout\n");
    /* Error message should be on stdout, not stderr */
    ok(stderr_buf[0] == 0, "expected no stderr output, got: %s\n", stderr_buf);
}

START_TEST(run)
{
    char **argv;
    int argc;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if(!register_activex()) {
        skip("Could not register ActiveX object.\n");
        CoUninitialize();
        return;
    }

    argc = winetest_get_mainargs(&argv);
    if(argc > 2) {
        run_script_file(argv[2], 0);
    }else {
        EnumResourceNamesA(NULL, "TESTSCRIPT", test_enum_proc, 0);

        run_simple_script("var winetest = new ActiveXObject('Wine.Test');\n"
                           "winetest.reportSuccess();\n"
                           "WScript.Quit(3);\n"
                           "winetest.ok(false, 'not quit?');\n", 3);

        run_cscript_error_test();
        run_cscript_unknown_option_test();
        run_cscript_no_file_test();
        run_cscript_usage_test();
        run_cscript_logo_test();
        run_cscript_nologo_test();
        run_cscript_host_option_after_filename_test();
        run_cscript_nonexistent_file_test();
        run_cscript_error_on_stdout_test();
}

    init_registry(FALSE);
    CoUninitialize();
}
