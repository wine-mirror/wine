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

#include "corerror.h"
#include "cor.h"
#include "mscoree.h"
#include "corhdr.h"
#include "cordebug.h"
#include "metahost.h"
#include "fusion.h"
#include "wine/list.h"
#include "mscoree_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL( mscoree );

static const struct ICLRRuntimeInfoVtbl CLRRuntimeInfoVtbl;

#define NUM_RUNTIMES 4

static struct CLRRuntimeInfo runtimes[NUM_RUNTIMES] = {
    {{&CLRRuntimeInfoVtbl}, 1, 0, 3705, 0},
    {{&CLRRuntimeInfoVtbl}, 1, 1, 4322, 0},
    {{&CLRRuntimeInfoVtbl}, 2, 0, 50727, 0},
    {{&CLRRuntimeInfoVtbl}, 4, 0, 30319, 0}
};

static CRITICAL_SECTION runtime_list_cs;
static CRITICAL_SECTION_DEBUG runtime_list_cs_debug =
{
    0, 0, &runtime_list_cs,
    { &runtime_list_cs_debug.ProcessLocksList,
      &runtime_list_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": runtime_list_cs") }
};
static CRITICAL_SECTION runtime_list_cs = { &runtime_list_cs_debug, -1, 0, 0, 0, 0 };

static HMODULE mono_handle;

BOOL is_mono_started;
static BOOL is_mono_shutdown;

MonoImage* (CDECL *mono_assembly_get_image)(MonoAssembly *assembly);
MonoAssembly* (CDECL *mono_assembly_load_from)(MonoImage *image, const char *fname, MonoImageOpenStatus *status);
MonoAssembly* (CDECL *mono_assembly_open)(const char *filename, MonoImageOpenStatus *status);
MonoClass* (CDECL *mono_class_from_mono_type)(MonoType *type);
MonoClass* (CDECL *mono_class_from_name)(MonoImage *image, const char* name_space, const char *name);
MonoMethod* (CDECL *mono_class_get_method_from_name)(MonoClass *klass, const char *name, int param_count);
static void (CDECL *mono_config_parse)(const char *filename);
MonoAssembly* (CDECL *mono_domain_assembly_open)(MonoDomain *domain, const char *name);
MonoDomain* (CDECL *mono_domain_get)(void);
MonoDomain* (CDECL *mono_domain_get_by_id)(int id);
BOOL (CDECL *mono_domain_set)(MonoDomain *domain,BOOL force);
void (CDECL *mono_domain_set_config)(MonoDomain *domain,const char *base_dir,const char *config_file_name);
static void (CDECL *mono_free)(void *);
static MonoImage* (CDECL *mono_image_open)(const char *fname, MonoImageOpenStatus *status);
MonoImage* (CDECL *mono_image_open_from_module_handle)(HMODULE module_handle, char* fname, UINT has_entry_point, MonoImageOpenStatus* status);
static void (CDECL *mono_install_assembly_preload_hook)(MonoAssemblyPreLoadFunc func, void *user_data);
int (CDECL *mono_jit_exec)(MonoDomain *domain, MonoAssembly *assembly, int argc, char *argv[]);
MonoDomain* (CDECL *mono_jit_init_version)(const char *domain_name, const char *runtime_version);
static int (CDECL *mono_jit_set_trace_options)(const char* options);
void* (CDECL *mono_marshal_get_vtfixup_ftnptr)(MonoImage *image, DWORD token, WORD type);
MonoDomain* (CDECL *mono_object_get_domain)(MonoObject *obj);
MonoMethod* (CDECL *mono_object_get_virtual_method)(MonoObject *obj, MonoMethod *method);
MonoObject* (CDECL *mono_object_new)(MonoDomain *domain, MonoClass *klass);
void* (CDECL *mono_object_unbox)(MonoObject *obj);
static void (CDECL *mono_profiler_install)(MonoProfiler *prof, MonoProfileFunc shutdown_callback);
MonoType* (CDECL *mono_reflection_type_from_name)(char *name, MonoImage *image);
MonoObject* (CDECL *mono_runtime_invoke)(MonoMethod *method, void *obj, void **params, MonoObject **exc);
void (CDECL *mono_runtime_object_init)(MonoObject *this_obj);
void (CDECL *mono_runtime_quit)(void);
static void (CDECL *mono_set_crash_chaining)(BOOL chain_signals);
static void (CDECL *mono_set_dirs)(const char *assembly_dir, const char *config_dir);
static void (CDECL *mono_set_verbose_level)(DWORD level);
MonoString* (CDECL *mono_string_new)(MonoDomain *domain, const char *str);
static char* (CDECL *mono_stringify_assembly_name)(MonoAssemblyName *aname);
MonoThread* (CDECL *mono_thread_attach)(MonoDomain *domain);
void (CDECL *mono_thread_manage)(void);
void (CDECL *mono_trace_set_assembly)(MonoAssembly *assembly);
void (CDECL *mono_trace_set_print_handler)(MonoPrintCallback callback);
void (CDECL *mono_trace_set_printerr_handler)(MonoPrintCallback callback);

static BOOL get_mono_path(LPWSTR path);

static BOOL find_mono_dll(LPCWSTR path, LPWSTR dll_path);

static MonoAssembly* CDECL mono_assembly_preload_hook_fn(MonoAssemblyName *aname, char **assemblies_path, void *user_data);

static void CDECL mono_shutdown_callback_fn(MonoProfiler *prof);

static void CDECL mono_print_handler_fn(const char *string, INT is_stdout);

static MonoImage* CDECL image_open_module_handle_dummy(HMODULE module_handle,
    char* fname, UINT has_entry_point, MonoImageOpenStatus* status)
{
    return mono_image_open(fname, status);
}

static void CDECL set_crash_chaining_dummy(BOOL crash_chaining)
{
}

static void CDECL set_print_handler_dummy(MonoPrintCallback callback)
{
}

static HRESULT load_mono(LPCWSTR mono_path)
{
    static const WCHAR lib[] = {'\\','l','i','b',0};
    static const WCHAR etc[] = {'\\','e','t','c',0};
    WCHAR mono_dll_path[MAX_PATH+16];
    WCHAR mono_lib_path[MAX_PATH+4], mono_etc_path[MAX_PATH+4];
    char mono_lib_path_a[MAX_PATH], mono_etc_path_a[MAX_PATH];
    int trace_size;
    char trace_setting[256];
    int verbose_size;
    char verbose_setting[256];

    if (is_mono_shutdown)
    {
        ERR("Cannot load Mono after it has been shut down.\n");
        return E_FAIL;
    }

    if (!mono_handle)
    {
        strcpyW(mono_lib_path, mono_path);
        strcatW(mono_lib_path, lib);
        WideCharToMultiByte(CP_UTF8, 0, mono_lib_path, -1, mono_lib_path_a, MAX_PATH, NULL, NULL);

        strcpyW(mono_etc_path, mono_path);
        strcatW(mono_etc_path, etc);
        WideCharToMultiByte(CP_UTF8, 0, mono_etc_path, -1, mono_etc_path_a, MAX_PATH, NULL, NULL);

        if (!find_mono_dll(mono_path, mono_dll_path)) goto fail;

        mono_handle = LoadLibraryW(mono_dll_path);

        if (!mono_handle) goto fail;

#define LOAD_MONO_FUNCTION(x) do { \
    x = (void*)GetProcAddress(mono_handle, #x); \
    if (!x) { \
        goto fail; \
    } \
} while (0);

        LOAD_MONO_FUNCTION(mono_assembly_get_image);
        LOAD_MONO_FUNCTION(mono_assembly_load_from);
        LOAD_MONO_FUNCTION(mono_assembly_open);
        LOAD_MONO_FUNCTION(mono_config_parse);
        LOAD_MONO_FUNCTION(mono_class_from_mono_type);
        LOAD_MONO_FUNCTION(mono_class_from_name);
        LOAD_MONO_FUNCTION(mono_class_get_method_from_name);
        LOAD_MONO_FUNCTION(mono_domain_assembly_open);
        LOAD_MONO_FUNCTION(mono_domain_get);
        LOAD_MONO_FUNCTION(mono_domain_get_by_id);
        LOAD_MONO_FUNCTION(mono_domain_set);
        LOAD_MONO_FUNCTION(mono_domain_set_config);
        LOAD_MONO_FUNCTION(mono_free);
        LOAD_MONO_FUNCTION(mono_image_open);
        LOAD_MONO_FUNCTION(mono_install_assembly_preload_hook);
        LOAD_MONO_FUNCTION(mono_jit_exec);
        LOAD_MONO_FUNCTION(mono_jit_init_version);
        LOAD_MONO_FUNCTION(mono_jit_set_trace_options);
        LOAD_MONO_FUNCTION(mono_marshal_get_vtfixup_ftnptr);
        LOAD_MONO_FUNCTION(mono_object_get_domain);
        LOAD_MONO_FUNCTION(mono_object_get_virtual_method);
        LOAD_MONO_FUNCTION(mono_object_new);
        LOAD_MONO_FUNCTION(mono_object_unbox);
        LOAD_MONO_FUNCTION(mono_profiler_install);
        LOAD_MONO_FUNCTION(mono_reflection_type_from_name);
        LOAD_MONO_FUNCTION(mono_runtime_invoke);
        LOAD_MONO_FUNCTION(mono_runtime_object_init);
        LOAD_MONO_FUNCTION(mono_runtime_quit);
        LOAD_MONO_FUNCTION(mono_set_dirs);
        LOAD_MONO_FUNCTION(mono_set_verbose_level);
        LOAD_MONO_FUNCTION(mono_stringify_assembly_name);
        LOAD_MONO_FUNCTION(mono_string_new);
        LOAD_MONO_FUNCTION(mono_thread_attach);
        LOAD_MONO_FUNCTION(mono_thread_manage);
        LOAD_MONO_FUNCTION(mono_trace_set_assembly);

#undef LOAD_MONO_FUNCTION

#define LOAD_OPT_MONO_FUNCTION(x, default) do { \
    x = (void*)GetProcAddress(mono_handle, #x); \
    if (!x) { \
        x = default; \
    } \
} while (0);

        LOAD_OPT_MONO_FUNCTION(mono_image_open_from_module_handle, image_open_module_handle_dummy);
        LOAD_OPT_MONO_FUNCTION(mono_set_crash_chaining, set_crash_chaining_dummy);
        LOAD_OPT_MONO_FUNCTION(mono_trace_set_print_handler, set_print_handler_dummy);
        LOAD_OPT_MONO_FUNCTION(mono_trace_set_printerr_handler, set_print_handler_dummy);

#undef LOAD_OPT_MONO_FUNCTION

        mono_profiler_install(NULL, mono_shutdown_callback_fn);

        mono_set_crash_chaining(TRUE);

        mono_trace_set_print_handler(mono_print_handler_fn);
        mono_trace_set_printerr_handler(mono_print_handler_fn);

        mono_set_dirs(mono_lib_path_a, mono_etc_path_a);

        mono_config_parse(NULL);

        mono_install_assembly_preload_hook(mono_assembly_preload_hook_fn, NULL);

        trace_size = GetEnvironmentVariableA("WINE_MONO_TRACE", trace_setting, sizeof(trace_setting));

        if (trace_size)
        {
            mono_jit_set_trace_options(trace_setting);
        }

        verbose_size = GetEnvironmentVariableA("WINE_MONO_VERBOSE", verbose_setting, sizeof(verbose_setting));

        if (verbose_size)
        {
            mono_set_verbose_level(verbose_setting[0] - '0');
        }
    }

    return S_OK;

fail:
    ERR("Could not load Mono into this process\n");
    FreeLibrary(mono_handle);
    mono_handle = NULL;
    return E_FAIL;
}

static void CDECL mono_shutdown_callback_fn(MonoProfiler *prof)
{
    is_mono_shutdown = TRUE;
}

static void CDECL mono_print_handler_fn(const char *string, INT is_stdout)
{
    const char *p;
    for (; *string; string = p)
    {
        if ((p = strstr(string, "\n"))) p++;
        else p = string + strlen(string);
        wine_dbg_printf("%.*s", (int)(p - string), string);
    }
}

static HRESULT CLRRuntimeInfo_GetRuntimeHost(CLRRuntimeInfo *This, RuntimeHost **result)
{
    HRESULT hr = S_OK;
    WCHAR mono_path[MAX_PATH];

    if (This->loaded_runtime)
    {
        *result = This->loaded_runtime;
        return hr;
    }

    if (!get_mono_path(mono_path))
    {
        ERR("Wine Mono is not installed\n");
        return CLR_E_SHIM_RUNTIME;
    }

    EnterCriticalSection(&runtime_list_cs);

    hr = load_mono(mono_path);

    if (SUCCEEDED(hr))
        hr = RuntimeHost_Construct(This, &This->loaded_runtime);

    LeaveCriticalSection(&runtime_list_cs);

    if (SUCCEEDED(hr))
        *result = This->loaded_runtime;

    return hr;
}

void expect_no_runtimes(void)
{
    if (mono_handle && is_mono_started && !is_mono_shutdown)
    {
        ERR("Process exited with a Mono runtime loaded.\n");
        return;
    }
}

static inline CLRRuntimeInfo *impl_from_ICLRRuntimeInfo(ICLRRuntimeInfo *iface)
{
    return CONTAINING_RECORD(iface, CLRRuntimeInfo, ICLRRuntimeInfo_iface);
}

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
    struct CLRRuntimeInfo *This = impl_from_ICLRRuntimeInfo(iface);
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
            hr = E_NOT_SUFFICIENT_BUFFER;
    }

    return hr;
}

static BOOL get_install_root(LPWSTR install_dir)
{
    const WCHAR dotnet_key[] = {'S','O','F','T','W','A','R','E','\\','M','i','c','r','o','s','o','f','t','\\','.','N','E','T','F','r','a','m','e','w','o','r','k','\\',0};
    const WCHAR install_root[] = {'I','n','s','t','a','l','l','R','o','o','t',0};

    DWORD len;
    HKEY key;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, dotnet_key, 0, KEY_READ, &key))
        return FALSE;

    len = MAX_PATH;
    if (RegQueryValueExW(key, install_root, 0, NULL, (LPBYTE)install_dir, &len))
    {
        RegCloseKey(key);
        return FALSE;
    }
    RegCloseKey(key);

    return TRUE;
}

static HRESULT WINAPI CLRRuntimeInfo_GetRuntimeDirectory(ICLRRuntimeInfo* iface,
    LPWSTR pwzBuffer, DWORD *pcchBuffer)
{
    static const WCHAR slash[] = {'\\',0};
    DWORD buffer_size = *pcchBuffer;
    WCHAR system_dir[MAX_PATH];
    WCHAR version[MAX_PATH];
    DWORD version_size, size;
    HRESULT hr = S_OK;

    TRACE("%p %p %p\n", iface, pwzBuffer, pcchBuffer);

    if (!get_install_root(system_dir))
    {
        ERR("error reading registry key for installroot\n");
        return E_FAIL;
    }
    else
    {
        version_size = MAX_PATH;
        ICLRRuntimeInfo_GetVersionString(iface, version, &version_size);
        lstrcatW(system_dir, version);
        lstrcatW(system_dir, slash);
        size = lstrlenW(system_dir) + 1;
    }

    *pcchBuffer = size;

    if (pwzBuffer)
    {
        if (buffer_size >= size)
            strcpyW(pwzBuffer, system_dir);
        else
            hr = E_NOT_SUFFICIENT_BUFFER;
    }

    return hr;
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
    WCHAR version[MAX_PATH];
    HRESULT hr;
    DWORD cchBuffer;

    TRACE("%p %s %p\n", iface, debugstr_w(pwzDllName), phndModule);

    cchBuffer = MAX_PATH;
    hr = ICLRRuntimeInfo_GetVersionString(iface, version, &cchBuffer);
    if (FAILED(hr)) return hr;

    return LoadLibraryShim(pwzDllName, version, NULL, phndModule);
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
    struct CLRRuntimeInfo *This = impl_from_ICLRRuntimeInfo(iface);
    RuntimeHost *host;
    HRESULT hr;

    TRACE("%p %s %s %p\n", iface, debugstr_guid(rclsid), debugstr_guid(riid), ppUnk);

    hr = CLRRuntimeInfo_GetRuntimeHost(This, &host);

    if (SUCCEEDED(hr))
        hr = RuntimeHost_GetInterface(host, rclsid, riid, ppUnk);

    return hr;
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

static const struct ICLRRuntimeInfoVtbl CLRRuntimeInfoVtbl = {
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

HRESULT ICLRRuntimeInfo_GetRuntimeHost(ICLRRuntimeInfo *iface, RuntimeHost **result)
{
    struct CLRRuntimeInfo *This = impl_from_ICLRRuntimeInfo(iface);

    assert(This->ICLRRuntimeInfo_iface.lpVtbl == &CLRRuntimeInfoVtbl);

    return CLRRuntimeInfo_GetRuntimeHost(This, result);
}

#ifdef __i386__
static const WCHAR libmono2_arch_dll[] = {'\\','b','i','n','\\','l','i','b','m','o','n','o','-','2','.','0','-','x','8','6','.','d','l','l',0};
#elif defined(__x86_64__)
static const WCHAR libmono2_arch_dll[] = {'\\','b','i','n','\\','l','i','b','m','o','n','o','-','2','.','0','-','x','8','6','_','6','4','.','d','l','l',0};
#else
static const WCHAR libmono2_arch_dll[] = {'\\','b','i','n','\\','l','i','b','m','o','n','o','-','2','.','0','.','d','l','l',0};
#endif

static BOOL find_mono_dll(LPCWSTR path, LPWSTR dll_path)
{
    static const WCHAR mono2_dll[] = {'\\','b','i','n','\\','m','o','n','o','-','2','.','0','.','d','l','l',0};
    static const WCHAR libmono2_dll[] = {'\\','b','i','n','\\','l','i','b','m','o','n','o','-','2','.','0','.','d','l','l',0};
    DWORD attributes=INVALID_FILE_ATTRIBUTES;

    strcpyW(dll_path, path);
    strcatW(dll_path, libmono2_arch_dll);
    attributes = GetFileAttributesW(dll_path);

    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        strcpyW(dll_path, path);
        strcatW(dll_path, mono2_dll);
        attributes = GetFileAttributesW(dll_path);
    }

    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        strcpyW(dll_path, path);
        strcatW(dll_path, libmono2_dll);
        attributes = GetFileAttributesW(dll_path);
    }

    return (attributes != INVALID_FILE_ATTRIBUTES);
}

static BOOL get_mono_path(LPWSTR path)
{
    static const WCHAR subdir_mono[] = {'\\','m','o','n','o','\\','m','o','n','o','-','2','.','0', 0};
    WCHAR base_path[MAX_PATH], mono_dll_path[MAX_PATH];

    /* c:\windows\mono\mono-2.0 */
    GetWindowsDirectoryW(base_path, MAX_PATH);
    strcatW(base_path, subdir_mono);

    if (find_mono_dll(base_path, mono_dll_path))
    {
        strcpyW(path, base_path);
        return TRUE;
    }

    return FALSE;
}

struct InstalledRuntimeEnum
{
    IEnumUnknown IEnumUnknown_iface;
    LONG ref;
    ULONG pos;
};

static const struct IEnumUnknownVtbl InstalledRuntimeEnum_Vtbl;

static inline struct InstalledRuntimeEnum *impl_from_IEnumUnknown(IEnumUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct InstalledRuntimeEnum, IEnumUnknown_iface);
}

static HRESULT WINAPI InstalledRuntimeEnum_QueryInterface(IEnumUnknown* iface, REFIID riid,
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
    struct InstalledRuntimeEnum *This = impl_from_IEnumUnknown(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI InstalledRuntimeEnum_Release(IEnumUnknown* iface)
{
    struct InstalledRuntimeEnum *This = impl_from_IEnumUnknown(iface);
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
    struct InstalledRuntimeEnum *This = impl_from_IEnumUnknown(iface);
    ULONG num_fetched = 0;
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
        item = (IUnknown*)&runtimes[This->pos].ICLRRuntimeInfo_iface;
        IUnknown_AddRef(item);
        rgelt[num_fetched] = item;
        num_fetched++;
        This->pos++;
    }

    if (pceltFetched)
        *pceltFetched = num_fetched;

    return hr;
}

static HRESULT WINAPI InstalledRuntimeEnum_Skip(IEnumUnknown *iface, ULONG celt)
{
    struct InstalledRuntimeEnum *This = impl_from_IEnumUnknown(iface);
    ULONG num_fetched = 0;
    HRESULT hr=S_OK;

    TRACE("(%p,%u)\n", iface, celt);

    while (num_fetched < celt)
    {
        if (This->pos >= NUM_RUNTIMES)
        {
            hr = S_FALSE;
            break;
        }
        num_fetched++;
        This->pos++;
    }

    return hr;
}

static HRESULT WINAPI InstalledRuntimeEnum_Reset(IEnumUnknown *iface)
{
    struct InstalledRuntimeEnum *This = impl_from_IEnumUnknown(iface);

    TRACE("(%p)\n", iface);

    This->pos = 0;

    return S_OK;
}

static HRESULT WINAPI InstalledRuntimeEnum_Clone(IEnumUnknown *iface, IEnumUnknown **ppenum)
{
    struct InstalledRuntimeEnum *This = impl_from_IEnumUnknown(iface);
    struct InstalledRuntimeEnum *new_enum;

    TRACE("(%p)\n", iface);

    new_enum = HeapAlloc(GetProcessHeap(), 0, sizeof(*new_enum));
    if (!new_enum)
        return E_OUTOFMEMORY;

    new_enum->IEnumUnknown_iface.lpVtbl = &InstalledRuntimeEnum_Vtbl;
    new_enum->ref = 1;
    new_enum->pos = This->pos;

    *ppenum = &new_enum->IEnumUnknown_iface;

    return S_OK;
}

static const struct IEnumUnknownVtbl InstalledRuntimeEnum_Vtbl = {
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
    ICLRMetaHost ICLRMetaHost_iface;

    RuntimeLoadedCallbackFnPtr callback;
};

static struct CLRMetaHost GlobalCLRMetaHost;

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

static BOOL parse_runtime_version(LPCWSTR version, DWORD *major, DWORD *minor, DWORD *build)
{
    *major = 0;
    *minor = 0;
    *build = 0;

    if (version[0] == 'v' || version[0] == 'V')
    {
        version++;
        if (!isdigit(*version))
            return FALSE;

        while (isdigit(*version))
            *major = *major * 10 + (*version++ - '0');

        if (*version == 0)
            return TRUE;

        if (*version++ != '.' || !isdigit(*version))
            return FALSE;

        while (isdigit(*version))
            *minor = *minor * 10 + (*version++ - '0');

        if (*version == 0)
            return TRUE;

        if (*version++ != '.' || !isdigit(*version))
            return FALSE;

        while (isdigit(*version))
            *build = *build * 10 + (*version++ - '0');

        return *version == 0;
    }
    else
        return FALSE;
}

static HRESULT get_runtime(LPCWSTR pwzVersion, BOOL allow_short,
    REFIID iid, LPVOID *ppRuntime)
{
    int i;
    DWORD major, minor, build;

    if (!pwzVersion)
        return E_POINTER;

    if (!parse_runtime_version(pwzVersion, &major, &minor, &build))
    {
        ERR("Cannot parse %s\n", debugstr_w(pwzVersion));
        return CLR_E_SHIM_RUNTIME;
    }

    for (i=0; i<NUM_RUNTIMES; i++)
    {
        if (runtimes[i].major == major && runtimes[i].minor == minor &&
            (runtimes[i].build == build || (allow_short && major >= 4 && build == 0)))
        {
            return ICLRRuntimeInfo_QueryInterface(&runtimes[i].ICLRRuntimeInfo_iface, iid,
                    ppRuntime);
        }
    }

    FIXME("Unrecognized version %s\n", debugstr_w(pwzVersion));
    return CLR_E_SHIM_RUNTIME;
}

HRESULT WINAPI CLRMetaHost_GetRuntime(ICLRMetaHost* iface,
    LPCWSTR pwzVersion, REFIID iid, LPVOID *ppRuntime)
{
    TRACE("%s %s %p\n", debugstr_w(pwzVersion), debugstr_guid(iid), ppRuntime);

    return get_runtime(pwzVersion, FALSE, iid, ppRuntime);
}

HRESULT WINAPI CLRMetaHost_GetVersionFromFile(ICLRMetaHost* iface,
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
                    hr = E_NOT_SUFFICIENT_BUFFER;
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

    new_enum = HeapAlloc(GetProcessHeap(), 0, sizeof(*new_enum));
    if (!new_enum)
        return E_OUTOFMEMORY;

    new_enum->IEnumUnknown_iface.lpVtbl = &InstalledRuntimeEnum_Vtbl;
    new_enum->ref = 1;
    new_enum->pos = 0;

    *ppEnumerator = &new_enum->IEnumUnknown_iface;

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
    TRACE("%p\n", pCallbackFunction);

    if(!pCallbackFunction)
        return E_POINTER;

    WARN("Callback currently will not be called.\n");

    GlobalCLRMetaHost.callback = pCallbackFunction;

    return S_OK;
}

static HRESULT WINAPI CLRMetaHost_QueryLegacyV2RuntimeBinding(ICLRMetaHost* iface,
    REFIID riid, LPVOID *ppUnk)
{
    FIXME("%s %p\n", debugstr_guid(riid), ppUnk);

    return E_NOTIMPL;
}

HRESULT WINAPI CLRMetaHost_ExitProcess(ICLRMetaHost* iface, INT32 iExitCode)
{
    TRACE("%i\n", iExitCode);

    EnterCriticalSection(&runtime_list_cs);

    if (is_mono_started && !is_mono_shutdown)
    {
        /* search for a runtime and call System.Environment.Exit() */
        int i;

        for (i=0; i<NUM_RUNTIMES; i++)
            if (runtimes[i].loaded_runtime)
                RuntimeHost_ExitProcess(runtimes[i].loaded_runtime, iExitCode);
    }

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

static struct CLRMetaHost GlobalCLRMetaHost = {
    { &CLRMetaHost_vtbl }
};

HRESULT CLRMetaHost_CreateInstance(REFIID riid, void **ppobj)
{
    return ICLRMetaHost_QueryInterface(&GlobalCLRMetaHost.ICLRMetaHost_iface, riid, ppobj);
}

struct CLRMetaHostPolicy
{
    ICLRMetaHostPolicy ICLRMetaHostPolicy_iface;
};

static struct CLRMetaHostPolicy GlobalCLRMetaHostPolicy;

static HRESULT WINAPI metahostpolicy_QueryInterface(ICLRMetaHostPolicy *iface, REFIID riid, void **obj)
{
    TRACE("%s %p\n", debugstr_guid(riid), obj);

    if ( IsEqualGUID( riid, &IID_ICLRMetaHostPolicy ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        ICLRMetaHostPolicy_AddRef( iface );
        *obj = iface;
        return S_OK;
    }

    FIXME("Unsupported interface %s\n", debugstr_guid(riid));

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI metahostpolicy_AddRef(ICLRMetaHostPolicy *iface)
{
    return 2;
}

static ULONG WINAPI metahostpolicy_Release(ICLRMetaHostPolicy *iface)
{
    return 1;
}

static HRESULT WINAPI metahostpolicy_GetRequestedRuntime(ICLRMetaHostPolicy *iface, METAHOST_POLICY_FLAGS dwPolicyFlags,
    LPCWSTR pwzBinary, IStream *pCfgStream, LPWSTR pwzVersion, DWORD *pcchVersion,
    LPWSTR pwzImageVersion, DWORD *pcchImageVersion, DWORD *pdwConfigFlags, REFIID riid,
    LPVOID *ppRuntime)
{
    ICLRRuntimeInfo *result;
    HRESULT hr;
    WCHAR filename[MAX_PATH];
    const WCHAR *path = NULL;
    int flags = 0;

    TRACE("%d %p %p %p %p %p %p %p %s %p\n", dwPolicyFlags, pwzBinary, pCfgStream,
        pwzVersion, pcchVersion, pwzImageVersion, pcchImageVersion, pdwConfigFlags,
        debugstr_guid(riid), ppRuntime);

    if (pCfgStream)
        FIXME("ignoring config file stream\n");

    if (pdwConfigFlags)
        FIXME("ignoring config flags\n");

    if(dwPolicyFlags & METAHOST_POLICY_USE_PROCESS_IMAGE_PATH)
    {
        GetModuleFileNameW(0, filename, MAX_PATH);
        path = filename;
    }
    else if(pwzBinary)
    {
        path = pwzBinary;
    }

    if(dwPolicyFlags & METAHOST_POLICY_APPLY_UPGRADE_POLICY)
        flags |= RUNTIME_INFO_UPGRADE_VERSION;

    hr = get_runtime_info(path, pwzImageVersion, NULL, 0, flags, FALSE, &result);
    if (SUCCEEDED(hr))
    {
        if (pwzImageVersion)
        {
            /* Ignoring errors on purpose */
            ICLRRuntimeInfo_GetVersionString(result, pwzImageVersion, pcchImageVersion);
        }

        hr = ICLRRuntimeInfo_QueryInterface(result, riid, ppRuntime);

        ICLRRuntimeInfo_Release(result);
    }

    TRACE("<- 0x%08x\n", hr);

    return hr;
}

static const struct ICLRMetaHostPolicyVtbl CLRMetaHostPolicy_vtbl =
{
    metahostpolicy_QueryInterface,
    metahostpolicy_AddRef,
    metahostpolicy_Release,
    metahostpolicy_GetRequestedRuntime
};

static struct CLRMetaHostPolicy GlobalCLRMetaHostPolicy = {
    { &CLRMetaHostPolicy_vtbl }
};

HRESULT CLRMetaHostPolicy_CreateInstance(REFIID riid, void **ppobj)
{
    return ICLRMetaHostPolicy_QueryInterface(&GlobalCLRMetaHostPolicy.ICLRMetaHostPolicy_iface, riid, ppobj);
}

HRESULT get_file_from_strongname(WCHAR* stringnameW, WCHAR* assemblies_path, int path_length)
{
    HRESULT hr=S_OK;
    IAssemblyCache *asmcache;
    ASSEMBLY_INFO info;
    static const WCHAR fusiondll[] = {'f','u','s','i','o','n',0};
    HMODULE hfusion=NULL;
    static HRESULT (WINAPI *pCreateAssemblyCache)(IAssemblyCache**,DWORD);

    if (!pCreateAssemblyCache)
    {
        hr = LoadLibraryShim(fusiondll, NULL, NULL, &hfusion);

        if (SUCCEEDED(hr))
        {
            pCreateAssemblyCache = (void*)GetProcAddress(hfusion, "CreateAssemblyCache");
            if (!pCreateAssemblyCache)
                hr = E_FAIL;
        }
    }

    if (SUCCEEDED(hr))
        hr = pCreateAssemblyCache(&asmcache, 0);

    if (SUCCEEDED(hr))
    {
        info.cbAssemblyInfo = sizeof(info);
        info.pszCurrentAssemblyPathBuf = assemblies_path;
        info.cchBuf = path_length;
        assemblies_path[0] = 0;

        hr = IAssemblyCache_QueryAssemblyInfo(asmcache, 0, stringnameW, &info);

        IAssemblyCache_Release(asmcache);
    }

    return hr;
}

static MonoAssembly* CDECL mono_assembly_preload_hook_fn(MonoAssemblyName *aname, char **assemblies_path, void *user_data)
{
    HRESULT hr;
    MonoAssembly *result=NULL;
    char *stringname=NULL;
    LPWSTR stringnameW;
    int stringnameW_size;
    WCHAR path[MAX_PATH];
    char *pathA;
    MonoImageOpenStatus stat;

    stringname = mono_stringify_assembly_name(aname);

    TRACE("%s\n", debugstr_a(stringname));

    if (!stringname) return NULL;

    /* FIXME: We should search the given paths before the GAC. */

    stringnameW_size = MultiByteToWideChar(CP_UTF8, 0, stringname, -1, NULL, 0);

    stringnameW = HeapAlloc(GetProcessHeap(), 0, stringnameW_size * sizeof(WCHAR));
    if (stringnameW)
    {
        MultiByteToWideChar(CP_UTF8, 0, stringname, -1, stringnameW, stringnameW_size);

        hr = get_file_from_strongname(stringnameW, path, MAX_PATH);

        HeapFree(GetProcessHeap(), 0, stringnameW);
    }
    else
        hr = E_OUTOFMEMORY;

    if (SUCCEEDED(hr))
    {
        TRACE("found: %s\n", debugstr_w(path));

        pathA = WtoA(path);

        if (pathA)
        {
            result = mono_assembly_open(pathA, &stat);

            if (!result)
                ERR("Failed to load %s, status=%u\n", debugstr_w(path), stat);

            HeapFree(GetProcessHeap(), 0, pathA);
        }
    }

    mono_free(stringname);

    return result;
}

HRESULT get_runtime_info(LPCWSTR exefile, LPCWSTR version, LPCWSTR config_file,
    DWORD startup_flags, DWORD runtimeinfo_flags, BOOL legacy, ICLRRuntimeInfo **result)
{
    static const WCHAR dotconfig[] = {'.','c','o','n','f','i','g',0};
    static const DWORD supported_startup_flags = 0;
    static const DWORD supported_runtime_flags = RUNTIME_INFO_UPGRADE_VERSION;
    int i;
    WCHAR local_version[MAX_PATH];
    ULONG local_version_size = MAX_PATH;
    WCHAR local_config_file[MAX_PATH];
    HRESULT hr;
    parsed_config_file parsed_config;

    if (startup_flags & ~supported_startup_flags)
        FIXME("unsupported startup flags %x\n", startup_flags & ~supported_startup_flags);

    if (runtimeinfo_flags & ~supported_runtime_flags)
        FIXME("unsupported runtimeinfo flags %x\n", runtimeinfo_flags & ~supported_runtime_flags);

    if (exefile && !exefile[0])
        exefile = NULL;

    if (exefile && !config_file)
    {
        strcpyW(local_config_file, exefile);
        strcatW(local_config_file, dotconfig);

        config_file = local_config_file;
    }

    if (config_file)
    {
        BOOL found = FALSE;
        hr = parse_config_file(config_file, &parsed_config);

        if (SUCCEEDED(hr))
        {
            supported_runtime *entry;
            LIST_FOR_EACH_ENTRY(entry, &parsed_config.supported_runtimes, supported_runtime, entry)
            {
                hr = get_runtime(entry->version, TRUE, &IID_ICLRRuntimeInfo, (void**)result);
                if (SUCCEEDED(hr))
                {
                    found = TRUE;
                    break;
                }
            }
        }
        else
        {
            WARN("failed to parse config file %s, hr=%x\n", debugstr_w(config_file), hr);
        }

        free_parsed_config_file(&parsed_config);

        if (found)
            return S_OK;
    }

    if (exefile && !version)
    {
        hr = CLRMetaHost_GetVersionFromFile(0, exefile, local_version, &local_version_size);

        version = local_version;

        if (FAILED(hr)) return hr;
    }

    if (version)
    {
        hr = CLRMetaHost_GetRuntime(0, version, &IID_ICLRRuntimeInfo, (void**)result);
        if(SUCCEEDED(hr))
            return hr;
    }

    if (runtimeinfo_flags & RUNTIME_INFO_UPGRADE_VERSION)
    {
        DWORD major, minor, build;

        if (version && !parse_runtime_version(version, &major, &minor, &build))
        {
            ERR("Cannot parse %s\n", debugstr_w(version));
            return CLR_E_SHIM_RUNTIME;
        }

        if (legacy)
            i = 3;
        else
            i = NUM_RUNTIMES;

        while (i--)
        {
            /* Must be greater or equal to the version passed in. */
            if (!version || ((runtimes[i].major >= major && runtimes[i].minor >= minor && runtimes[i].build >= build) ||
                 (runtimes[i].major >= major && runtimes[i].minor > minor) ||
                 (runtimes[i].major > major)))
            {
                return ICLRRuntimeInfo_QueryInterface(&runtimes[i].ICLRRuntimeInfo_iface,
                        &IID_ICLRRuntimeInfo, (void **)result);
            }
        }

        return CLR_E_SHIM_RUNTIME;
    }

    return CLR_E_SHIM_RUNTIME;
}
