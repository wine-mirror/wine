/*
 * ICLRMetaHost - discovery and management of available .NET runtimes
 *
 * Copyright 2010 Vincent Povirk for CodeWeavers
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
#include <stdarg.h>
#include <assert.h>

#define COBJMACROS

#include "wine/unicode.h"
#include "wine/library.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "ole2.h"

#include "mscoree.h"
#include "metahost.h"
#include "mscoree_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL( mscoree );

static const WCHAR net_11_subdir[] = {'1','.','0',0};
static const WCHAR net_20_subdir[] = {'2','.','0',0};
static const WCHAR net_40_subdir[] = {'4','.','0',0};

struct CLRRuntimeInfo
{
    const struct ICLRRuntimeInfoVtbl *ICLRRuntimeInfo_vtbl;
    LPCWSTR mono_libdir;
    DWORD major;
    DWORD minor;
    DWORD build;
    int mono_abi_version;
};

const struct ICLRRuntimeInfoVtbl CLRRuntimeInfoVtbl;

#define NUM_RUNTIMES 3

static struct CLRRuntimeInfo runtimes[NUM_RUNTIMES] = {
    {&CLRRuntimeInfoVtbl, net_11_subdir, 1, 1, 4322, 0},
    {&CLRRuntimeInfoVtbl, net_20_subdir, 2, 0, 50727, 0},
    {&CLRRuntimeInfoVtbl, net_40_subdir, 4, 0, 30319, 0}
};

static int runtimes_initialized;

static CRITICAL_SECTION runtime_list_cs;
static CRITICAL_SECTION_DEBUG runtime_list_cs_debug =
{
    0, 0, &runtime_list_cs,
    { &runtime_list_cs_debug.ProcessLocksList,
      &runtime_list_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": runtime_list_cs") }
};
static CRITICAL_SECTION runtime_list_cs = { &runtime_list_cs_debug, -1, 0, 0, 0, 0 };

static HRESULT WINAPI CLRRuntimeInfo_QueryInterface(ICLRRuntimeInfo* iface,
        REFIID riid,
        void **ppvObject)
{
    TRACE("%p %s %p\n", iface, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_ICLRRuntimeInfo ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("Unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    ICLRRuntimeInfo_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI CLRRuntimeInfo_AddRef(ICLRRuntimeInfo* iface)
{
    return 2;
}

static ULONG WINAPI CLRRuntimeInfo_Release(ICLRRuntimeInfo* iface)
{
    return 1;
}

static HRESULT WINAPI CLRRuntimeInfo_GetVersionString(ICLRRuntimeInfo* iface,
    LPWSTR pwzBuffer, DWORD *pcchBuffer)
{
    struct CLRRuntimeInfo *This = (struct CLRRuntimeInfo*)iface;
    DWORD buffer_size = *pcchBuffer;
    HRESULT hr = S_OK;
    char version[11];
    DWORD size;

    TRACE("%p %p %p\n", iface, pwzBuffer, pcchBuffer);

    size = snprintf(version, sizeof(version), "v%u.%u.%u", This->major, This->minor, This->build);

    assert(size <= sizeof(version));

    *pcchBuffer = MultiByteToWideChar(CP_UTF8, 0, version, -1, NULL, 0);

    if (pwzBuffer)
    {
        if (buffer_size >= *pcchBuffer)
            MultiByteToWideChar(CP_UTF8, 0, version, -1, pwzBuffer, buffer_size);
        else
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    return hr;
}

static HRESULT WINAPI CLRRuntimeInfo_GetRuntimeDirectory(ICLRRuntimeInfo* iface,
    LPWSTR pwzBuffer, DWORD *pcchBuffer)
{
    FIXME("%p %p %p\n", iface, pwzBuffer, pcchBuffer);

    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeInfo_IsLoaded(ICLRRuntimeInfo* iface,
    HANDLE hndProcess, BOOL *pbLoaded)
{
    FIXME("%p %p %p\n", iface, hndProcess, pbLoaded);

    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeInfo_LoadErrorString(ICLRRuntimeInfo* iface,
    UINT iResourceID, LPWSTR pwzBuffer, DWORD *pcchBuffer, LONG iLocaleid)
{
    FIXME("%p %u %p %p %x\n", iface, iResourceID, pwzBuffer, pcchBuffer, iLocaleid);

    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeInfo_LoadLibrary(ICLRRuntimeInfo* iface,
    LPCWSTR pwzDllName, HMODULE *phndModule)
{
    FIXME("%p %s %p\n", iface, debugstr_w(pwzDllName), phndModule);

    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeInfo_GetProcAddress(ICLRRuntimeInfo* iface,
    LPCSTR pszProcName, LPVOID *ppProc)
{
    FIXME("%p %s %p\n", iface, debugstr_a(pszProcName), ppProc);

    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeInfo_GetInterface(ICLRRuntimeInfo* iface,
    REFCLSID rclsid, REFIID riid, LPVOID *ppUnk)
{
    FIXME("%p %s %s %p\n", iface, debugstr_guid(rclsid), debugstr_guid(riid), ppUnk);

    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeInfo_IsLoadable(ICLRRuntimeInfo* iface,
    BOOL *pbLoadable)
{
    FIXME("%p %p\n", iface, pbLoadable);

    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeInfo_SetDefaultStartupFlags(ICLRRuntimeInfo* iface,
    DWORD dwStartupFlags, LPCWSTR pwzHostConfigFile)
{
    FIXME("%p %x %s\n", iface, dwStartupFlags, debugstr_w(pwzHostConfigFile));

    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeInfo_GetDefaultStartupFlags(ICLRRuntimeInfo* iface,
    DWORD *pdwStartupFlags, LPWSTR pwzHostConfigFile, DWORD *pcchHostConfigFile)
{
    FIXME("%p %p %p %p\n", iface, pdwStartupFlags, pwzHostConfigFile, pcchHostConfigFile);

    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeInfo_BindAsLegacyV2Runtime(ICLRRuntimeInfo* iface)
{
    FIXME("%p\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeInfo_IsStarted(ICLRRuntimeInfo* iface,
    BOOL *pbStarted, DWORD *pdwStartupFlags)
{
    FIXME("%p %p %p\n", iface, pbStarted, pdwStartupFlags);

    return E_NOTIMPL;
}

const struct ICLRRuntimeInfoVtbl CLRRuntimeInfoVtbl = {
    CLRRuntimeInfo_QueryInterface,
    CLRRuntimeInfo_AddRef,
    CLRRuntimeInfo_Release,
    CLRRuntimeInfo_GetVersionString,
    CLRRuntimeInfo_GetRuntimeDirectory,
    CLRRuntimeInfo_IsLoaded,
    CLRRuntimeInfo_LoadErrorString,
    CLRRuntimeInfo_LoadLibrary,
    CLRRuntimeInfo_GetProcAddress,
    CLRRuntimeInfo_GetInterface,
    CLRRuntimeInfo_IsLoadable,
    CLRRuntimeInfo_SetDefaultStartupFlags,
    CLRRuntimeInfo_GetDefaultStartupFlags,
    CLRRuntimeInfo_BindAsLegacyV2Runtime,
    CLRRuntimeInfo_IsStarted
};

static BOOL find_mono_dll(LPCWSTR path, LPWSTR dll_path, int abi_version)
{
    static const WCHAR mono_dll[] = {'\\','b','i','n','\\','m','o','n','o','.','d','l','l',0};
    static const WCHAR libmono_dll[] = {'\\','b','i','n','\\','l','i','b','m','o','n','o','.','d','l','l',0};
    DWORD attributes=INVALID_FILE_ATTRIBUTES;

    if (abi_version == 1)
    {
        strcpyW(dll_path, path);
        strcatW(dll_path, mono_dll);
        attributes = GetFileAttributesW(dll_path);

        if (attributes == INVALID_FILE_ATTRIBUTES)
        {
            strcpyW(dll_path, path);
            strcatW(dll_path, libmono_dll);
            attributes = GetFileAttributesW(dll_path);
        }
    }

    return (attributes != INVALID_FILE_ATTRIBUTES);
}

static BOOL get_mono_path_from_registry(LPWSTR path, int abi_version)
{
    static const WCHAR mono_key[] = {'S','o','f','t','w','a','r','e','\\','N','o','v','e','l','l','\\','M','o','n','o',0};
    static const WCHAR defaul_clr[] = {'D','e','f','a','u','l','t','C','L','R',0};
    static const WCHAR install_root[] = {'S','d','k','I','n','s','t','a','l','l','R','o','o','t',0};
    static const WCHAR slash[] = {'\\',0};

    WCHAR version[64], version_key[MAX_PATH];
    DWORD len;
    HKEY key;
    WCHAR dll_path[MAX_PATH];

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, mono_key, 0, KEY_READ, &key))
        return FALSE;

    len = sizeof(version);
    if (RegQueryValueExW(key, defaul_clr, 0, NULL, (LPBYTE)version, &len))
    {
        RegCloseKey(key);
        return FALSE;
    }
    RegCloseKey(key);

    lstrcpyW(version_key, mono_key);
    lstrcatW(version_key, slash);
    lstrcatW(version_key, version);

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, version_key, 0, KEY_READ, &key))
        return FALSE;

    len = sizeof(WCHAR) * MAX_PATH;
    if (RegQueryValueExW(key, install_root, 0, NULL, (LPBYTE)path, &len))
    {
        RegCloseKey(key);
        return FALSE;
    }
    RegCloseKey(key);

    return find_mono_dll(path, dll_path, abi_version);
}

static BOOL get_mono_path_from_folder(LPCWSTR folder, LPWSTR mono_path, int abi_version)
{
    static const WCHAR mono_one_dot_zero[] = {'\\','m','o','n','o','-','1','.','0', 0};
    WCHAR mono_dll_path[MAX_PATH];
    BOOL found = FALSE;

    strcpyW(mono_path, folder);

    if (abi_version == 1)
        strcatW(mono_path, mono_one_dot_zero);

    found = find_mono_dll(mono_path, mono_dll_path, abi_version);

    return found;
}

static BOOL get_mono_path(LPWSTR path, int abi_version)
{
    static const WCHAR subdir_mono[] = {'\\','m','o','n','o',0};
    static const WCHAR sibling_mono[] = {'\\','.','.','\\','m','o','n','o',0};
    WCHAR base_path[MAX_PATH];
    const char *unix_data_dir;
    WCHAR *dos_data_dir;
    int build_tree=0;
    static WCHAR* (CDECL *wine_get_dos_file_name)(const char*);

    /* First try c:\windows\mono */
    GetWindowsDirectoryW(base_path, MAX_PATH);
    strcatW(base_path, subdir_mono);

    if (get_mono_path_from_folder(base_path, path, abi_version))
        return TRUE;

    /* Next: /usr/share/wine/mono */
    unix_data_dir = wine_get_data_dir();

    if (!unix_data_dir)
    {
        unix_data_dir = wine_get_build_dir();
        build_tree = 1;
    }

    if (unix_data_dir)
    {
        if (!wine_get_dos_file_name)
            wine_get_dos_file_name = (void*)GetProcAddress(GetModuleHandleA("kernel32"), "wine_get_dos_file_name");

        if (wine_get_dos_file_name)
        {
            dos_data_dir = wine_get_dos_file_name(unix_data_dir);

            if (dos_data_dir)
            {
                strcpyW(base_path, dos_data_dir);
                strcatW(base_path, build_tree ? sibling_mono : subdir_mono);

                HeapFree(GetProcessHeap(), 0, dos_data_dir);

                if (get_mono_path_from_folder(base_path, path, abi_version))
                    return TRUE;
            }
        }
    }

    /* Last: the registry */
    return get_mono_path_from_registry(path, abi_version);
}

static void find_runtimes(void)
{
    int abi_version, i;
    static const WCHAR libmono[] = {'\\','l','i','b','\\','m','o','n','o','\\',0};
    static const WCHAR mscorlib[] = {'\\','m','s','c','o','r','l','i','b','.','d','l','l',0};
    WCHAR mono_path[MAX_PATH], lib_path[MAX_PATH];
    BOOL any_runtimes_found = FALSE;

    if (runtimes_initialized) return;

    EnterCriticalSection(&runtime_list_cs);

    if (runtimes_initialized) goto end;

    for (abi_version=1; abi_version>0; abi_version--)
    {
        if (!get_mono_path(mono_path, abi_version))
            continue;

        for (i=0; i<NUM_RUNTIMES; i++)
        {
            if (runtimes[i].mono_abi_version == 0)
            {
                strcpyW(lib_path, mono_path);
                strcatW(lib_path, libmono);
                strcatW(lib_path, runtimes[i].mono_libdir);
                strcatW(lib_path, mscorlib);

                if (GetFileAttributesW(lib_path) != INVALID_FILE_ATTRIBUTES)
                {
                    runtimes[i].mono_abi_version = abi_version;

                    any_runtimes_found = TRUE;
                }
            }
        }
    }

    runtimes_initialized = 1;

    if (!any_runtimes_found)
        MESSAGE("wine: Install the Windows version of Mono to run .NET executables\n");

end:
    LeaveCriticalSection(&runtime_list_cs);
}

struct InstalledRuntimeEnum
{
    const struct IEnumUnknownVtbl *Vtbl;
    LONG ref;
    ULONG pos;
};

const struct IEnumUnknownVtbl InstalledRuntimeEnum_Vtbl;

static HRESULT WINAPI InstalledRuntimeEnum_QueryInterface(IEnumUnknown* iface,
        REFIID riid,
        void **ppvObject)
{
    TRACE("%p %s %p\n", iface, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IEnumUnknown ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("Unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IEnumUnknown_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI InstalledRuntimeEnum_AddRef(IEnumUnknown* iface)
{
    struct InstalledRuntimeEnum *This = (struct InstalledRuntimeEnum*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI InstalledRuntimeEnum_Release(IEnumUnknown* iface)
{
    struct InstalledRuntimeEnum *This = (struct InstalledRuntimeEnum*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI InstalledRuntimeEnum_Next(IEnumUnknown *iface, ULONG celt,
    IUnknown **rgelt, ULONG *pceltFetched)
{
    struct InstalledRuntimeEnum *This = (struct InstalledRuntimeEnum*)iface;
    int num_fetched = 0;
    HRESULT hr=S_OK;
    IUnknown *item;

    TRACE("(%p,%u,%p,%p)\n", iface, celt, rgelt, pceltFetched);

    while (num_fetched < celt)
    {
        if (This->pos >= NUM_RUNTIMES)
        {
            hr = S_FALSE;
            break;
        }
        if (runtimes[This->pos].mono_abi_version)
        {
            item = (IUnknown*)&runtimes[This->pos];
            IUnknown_AddRef(item);
            rgelt[num_fetched] = item;
            num_fetched++;
        }
        This->pos++;
    }

    if (pceltFetched)
        *pceltFetched = num_fetched;

    return hr;
}

static HRESULT WINAPI InstalledRuntimeEnum_Skip(IEnumUnknown *iface, ULONG celt)
{
    struct InstalledRuntimeEnum *This = (struct InstalledRuntimeEnum*)iface;
    int num_fetched = 0;
    HRESULT hr=S_OK;

    TRACE("(%p,%u)\n", iface, celt);

    while (num_fetched < celt)
    {
        if (This->pos >= NUM_RUNTIMES)
        {
            hr = S_FALSE;
            break;
        }
        if (runtimes[This->pos].mono_abi_version)
        {
            num_fetched++;
        }
        This->pos++;
    }

    return hr;
}

static HRESULT WINAPI InstalledRuntimeEnum_Reset(IEnumUnknown *iface)
{
    struct InstalledRuntimeEnum *This = (struct InstalledRuntimeEnum*)iface;

    TRACE("(%p)\n", iface);

    This->pos = 0;

    return S_OK;
}

static HRESULT WINAPI InstalledRuntimeEnum_Clone(IEnumUnknown *iface, IEnumUnknown **ppenum)
{
    struct InstalledRuntimeEnum *This = (struct InstalledRuntimeEnum*)iface;
    struct InstalledRuntimeEnum *new_enum;

    TRACE("(%p)\n", iface);

    new_enum = HeapAlloc(GetProcessHeap(), 0, sizeof(*new_enum));
    if (!new_enum)
        return E_OUTOFMEMORY;

    new_enum->Vtbl = &InstalledRuntimeEnum_Vtbl;
    new_enum->ref = 1;
    new_enum->pos = This->pos;

    *ppenum = (IEnumUnknown*)new_enum;

    return S_OK;
}

const struct IEnumUnknownVtbl InstalledRuntimeEnum_Vtbl = {
    InstalledRuntimeEnum_QueryInterface,
    InstalledRuntimeEnum_AddRef,
    InstalledRuntimeEnum_Release,
    InstalledRuntimeEnum_Next,
    InstalledRuntimeEnum_Skip,
    InstalledRuntimeEnum_Reset,
    InstalledRuntimeEnum_Clone
};

struct CLRMetaHost
{
    const struct ICLRMetaHostVtbl *CLRMetaHost_vtbl;
};

static const struct CLRMetaHost GlobalCLRMetaHost;

static HRESULT WINAPI CLRMetaHost_QueryInterface(ICLRMetaHost* iface,
        REFIID riid,
        void **ppvObject)
{
    TRACE("%s %p\n", debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_ICLRMetaHost ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("Unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    ICLRMetaHost_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI CLRMetaHost_AddRef(ICLRMetaHost* iface)
{
    return 2;
}

static ULONG WINAPI CLRMetaHost_Release(ICLRMetaHost* iface)
{
    return 1;
}

static HRESULT WINAPI CLRMetaHost_GetRuntime(ICLRMetaHost* iface,
    LPCWSTR pwzVersion, REFIID iid, LPVOID *ppRuntime)
{
    FIXME("%s %s %p\n", debugstr_w(pwzVersion), debugstr_guid(iid), ppRuntime);

    return E_NOTIMPL;
}

static HRESULT WINAPI CLRMetaHost_GetVersionFromFile(ICLRMetaHost* iface,
    LPCWSTR pwzFilePath, LPWSTR pwzBuffer, DWORD *pcchBuffer)
{
    ASSEMBLY *assembly;
    HRESULT hr;
    LPSTR version;
    ULONG buffer_size=*pcchBuffer;

    TRACE("%s %p %p\n", debugstr_w(pwzFilePath), pwzBuffer, pcchBuffer);

    hr = assembly_create(&assembly, pwzFilePath);

    if (SUCCEEDED(hr))
    {
        hr = assembly_get_runtime_version(assembly, &version);

        if (SUCCEEDED(hr))
        {
            *pcchBuffer = MultiByteToWideChar(CP_UTF8, 0, version, -1, NULL, 0);

            if (pwzBuffer)
            {
                if (buffer_size >= *pcchBuffer)
                    MultiByteToWideChar(CP_UTF8, 0, version, -1, pwzBuffer, buffer_size);
                else
                    hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            }
        }

        assembly_release(assembly);
    }

    return hr;
}

static HRESULT WINAPI CLRMetaHost_EnumerateInstalledRuntimes(ICLRMetaHost* iface,
    IEnumUnknown **ppEnumerator)
{
    struct InstalledRuntimeEnum *new_enum;

    TRACE("%p\n", ppEnumerator);

    find_runtimes();

    new_enum = HeapAlloc(GetProcessHeap(), 0, sizeof(*new_enum));
    if (!new_enum)
        return E_OUTOFMEMORY;

    new_enum->Vtbl = &InstalledRuntimeEnum_Vtbl;
    new_enum->ref = 1;
    new_enum->pos = 0;

    *ppEnumerator = (IEnumUnknown*)new_enum;

    return S_OK;
}

static HRESULT WINAPI CLRMetaHost_EnumerateLoadedRuntimes(ICLRMetaHost* iface,
    HANDLE hndProcess, IEnumUnknown **ppEnumerator)
{
    FIXME("%p %p\n", hndProcess, ppEnumerator);

    return E_NOTIMPL;
}

static HRESULT WINAPI CLRMetaHost_RequestRuntimeLoadedNotification(ICLRMetaHost* iface,
    RuntimeLoadedCallbackFnPtr pCallbackFunction)
{
    FIXME("%p\n", pCallbackFunction);

    return E_NOTIMPL;
}

static HRESULT WINAPI CLRMetaHost_QueryLegacyV2RuntimeBinding(ICLRMetaHost* iface,
    REFIID riid, LPVOID *ppUnk)
{
    FIXME("%s %p\n", debugstr_guid(riid), ppUnk);

    return E_NOTIMPL;
}

static HRESULT WINAPI CLRMetaHost_ExitProcess(ICLRMetaHost* iface, INT32 iExitCode)
{
    FIXME("%i: stub\n", iExitCode);

    ExitProcess(iExitCode);
}

static const struct ICLRMetaHostVtbl CLRMetaHost_vtbl =
{
    CLRMetaHost_QueryInterface,
    CLRMetaHost_AddRef,
    CLRMetaHost_Release,
    CLRMetaHost_GetRuntime,
    CLRMetaHost_GetVersionFromFile,
    CLRMetaHost_EnumerateInstalledRuntimes,
    CLRMetaHost_EnumerateLoadedRuntimes,
    CLRMetaHost_RequestRuntimeLoadedNotification,
    CLRMetaHost_QueryLegacyV2RuntimeBinding,
    CLRMetaHost_ExitProcess
};

static const struct CLRMetaHost GlobalCLRMetaHost = {
    &CLRMetaHost_vtbl
};

extern HRESULT CLRMetaHost_CreateInstance(REFIID riid, void **ppobj)
{
    return ICLRMetaHost_QueryInterface((ICLRMetaHost*)&GlobalCLRMetaHost, riid, ppobj);
}
