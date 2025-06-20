/*
 * Copyright 2006-2010 Jacek Caban for CodeWeavers
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
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "cpl.h"
#include "winreg.h"
#include "ole2.h"
#include "commctrl.h"
#include "advpub.h"
#include "wininet.h"
#include "pathcch.h"
#include "shellapi.h"
#include "urlmon.h"
#include "msi.h"
#include "bcrypt.h"

#include "appwiz.h"
#include "res.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(appwizcpl);

#define GECKO_VERSION "2.47.4"
#ifdef __i386__
#define GECKO_ARCH "x86"
#define GECKO_SHA "26cecc47706b091908f7f814bddb074c61beb8063318e9efc5a7f789857793d6"
#elif defined(__x86_64__)
#define GECKO_ARCH "x86_64"
#define GECKO_SHA "e590b7d988a32d6aa4cf1d8aa3aa3d33766fdd4cf4c89c2dcc2095ecb28d066f"
#else
#define GECKO_ARCH ""
#define GECKO_SHA "???"
#endif

#define MONO_VERSION "10.1.0"
#if defined(__i386__) || defined(__x86_64__)
#define MONO_ARCH "x86"
#define MONO_SHA "c88c2431890bc32cacec8d7ea70e53a5ae4b95c8c55ca6e75ef8db0e4ccf1070"
#else
#define MONO_ARCH ""
#define MONO_SHA "???"
#endif

typedef struct {
    const char *version;
    const WCHAR *file_name;
    const WCHAR *subdir_name;
    const char *sha;
    const char *url_default;
    const WCHAR *config_key;
    const WCHAR *url_config_key;
    const char *dir_config_key;
    LPCWSTR dialog_template;
} addon_info_t;

/* Download addon files over HTTP because Wine depends on an external library
 * for TLS, so we can't be sure that HTTPS will work. The integrity of each file
 * is checked with a hardcoded cryptographically secure hash. */
static const addon_info_t addons_info[] = {
    {
        GECKO_VERSION,
        L"wine-gecko-" GECKO_VERSION "-" GECKO_ARCH ".msi",
        L"gecko",
        GECKO_SHA,
        "http://source.winehq.org/winegecko.php",
        L"MSHTML", L"GeckoUrl", "GeckoCabDir",
        MAKEINTRESOURCEW(ID_DWL_GECKO_DIALOG)
    },
    {
        MONO_VERSION,
        L"wine-mono-" MONO_VERSION "-" MONO_ARCH ".msi",
        L"mono",
        MONO_SHA,
        "http://source.winehq.org/winemono.php",
        L"Dotnet", L"MonoUrl", "MonoCabDir",
        MAKEINTRESOURCEW(ID_DWL_MONO_DIALOG)
    }
};

static const addon_info_t *addon;

static HWND install_dialog = NULL;
static LPWSTR url = NULL;
static IBinding *dwl_binding;
static WCHAR *msi_file;

static const char * (CDECL *p_wine_get_version)(void);
static WCHAR * (CDECL *p_wine_get_dos_file_name)(const char*);

static BOOL sha_check(const WCHAR *file_name)
{
    const unsigned char *file_map;
    HANDLE file, map;
    DWORD size, i;
    UCHAR sha[32];
    char buf[1024];
    BOOL ret = FALSE;

    file = CreateFileW(file_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    if(file == INVALID_HANDLE_VALUE) {
        WARN("Could not open file: %lu\n", GetLastError());
        return FALSE;
    }

    size = GetFileSize(file, NULL);

    map = CreateFileMappingW(file, NULL, PAGE_READONLY, 0, 0, NULL);
    CloseHandle(file);
    if(!map)
        return FALSE;

    file_map = MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(map);
    if(!file_map)
        return FALSE;

    if(!BCryptHash(BCRYPT_SHA256_ALG_HANDLE, NULL, 0, (UCHAR *)file_map, size, sha, sizeof(sha))) {
        for(i=0; i < sizeof(sha); i++)
            sprintf(buf + i * 2, "%02x", sha[i]);

        ret = !strcmp(buf, addon->sha);
        if(!ret)
            WARN("Got %s, expected %s\n", buf, addon->sha);
    }

    UnmapViewOfFile(file_map);
    return ret;
}

static void set_status(DWORD id)
{
    HWND status = GetDlgItem(install_dialog, ID_DWL_STATUS);
    WCHAR buf[64];

    LoadStringW(hInst, id, buf, ARRAY_SIZE(buf));
    SendMessageW(status, WM_SETTEXT, 0, (LPARAM)buf);
}

enum install_res {
    INSTALL_OK = 0,
    INSTALL_FAILED,
    INSTALL_NEXT,
};

static enum install_res install_file(const WCHAR *file_name)
{
    ULONG res;

    res = MsiInstallProductW(file_name, NULL);
    if(res == ERROR_PRODUCT_VERSION)
        res = MsiInstallProductW(file_name, L"REINSTALL=ALL REINSTALLMODE=vomus");
    if(res != ERROR_SUCCESS) {
        ERR("MsiInstallProduct failed: %lu\n", res);
        return INSTALL_FAILED;
    }

    return INSTALL_OK;
}

static enum install_res install_from_dos_file(const WCHAR *dir, const WCHAR *subdir, const WCHAR *file_name)
{
    WCHAR *path, *canonical_path;
    enum install_res ret;
    int len = lstrlenW( dir );
    int size = len + 1;
    HRESULT hr;

    size += lstrlenW( subdir ) + lstrlenW( file_name ) + 2;
    if (!(path = malloc( size * sizeof(WCHAR) ))) return INSTALL_FAILED;

    lstrcpyW( path, dir );
    if (!wcsncmp( path, L"\\??\\", 4 ))  path[1] = '\\';  /* change \??\ into \\?\ */
    if (len && path[len-1] != '/' && path[len-1] != '\\') path[len++] = '\\';

    lstrcpyW( path + len, subdir );
    lstrcatW( path, L"\\" );
    lstrcatW( path, file_name );

    hr = PathAllocCanonicalize( path, PATHCCH_ALLOW_LONG_PATHS, &canonical_path );
    if (FAILED( hr ))
    {
        ERR( "Failed to canonicalize %s, hr %#lx\n", debugstr_w(path), hr );
        free( path );
        return INSTALL_NEXT;
    }
    free( path );

    if (GetFileAttributesW( canonical_path ) == INVALID_FILE_ATTRIBUTES)
    {
        TRACE( "%s not found\n", debugstr_w(canonical_path) );
        LocalFree( canonical_path );
        return INSTALL_NEXT;
    }

    ret = install_file( canonical_path );

    LocalFree( canonical_path );
    return ret;
}

static enum install_res install_from_unix_file(const char *dir, const WCHAR *subdir, const WCHAR *file_name)
{
    WCHAR *dos_dir;
    enum install_res ret = INSTALL_NEXT;

    if (p_wine_get_dos_file_name && (dos_dir = p_wine_get_dos_file_name( dir )))
    {
        ret = install_from_dos_file( dos_dir, subdir, file_name );
        HeapFree( GetProcessHeap(), 0, dos_dir );
    }
    return ret;
}

static HKEY open_config_key(void)
{
    HKEY hkey, ret;
    DWORD res;

    /* @@ Wine registry key: HKCU\Software\Wine\$config_key */
    res = RegOpenKeyW(HKEY_CURRENT_USER, L"Software\\Wine", &hkey);
    if(res != ERROR_SUCCESS)
        return NULL;

    res = RegOpenKeyW(hkey, addon->config_key, &ret);
    RegCloseKey(hkey);
    return res == ERROR_SUCCESS ? ret : NULL;
}

static enum install_res install_from_registered_dir(void)
{
    char *package_dir, *new_package_dir;
    HKEY hkey;
    DWORD res, type, size = MAX_PATH;
    enum install_res ret;

    hkey = open_config_key();
    if(!hkey)
        return INSTALL_NEXT;

    package_dir = malloc(size);
    res = RegGetValueA(hkey, NULL, addon->dir_config_key, RRF_RT_ANY, &type, (PBYTE)package_dir, &size);
    if(res == ERROR_MORE_DATA) {
        new_package_dir = realloc(package_dir, size);
        if(new_package_dir) {
            package_dir = new_package_dir;
            res = RegGetValueA(hkey, NULL, addon->dir_config_key, RRF_RT_ANY, &type, (PBYTE)package_dir, &size);
        }
    }
    RegCloseKey(hkey);
    if(res == ERROR_FILE_NOT_FOUND) {
        free(package_dir);
        return INSTALL_NEXT;
    } else if(res != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ)) {
        free(package_dir);
        return INSTALL_FAILED;
    }

    ret = install_from_unix_file(package_dir, L"", addon->file_name);

    free(package_dir);
    return ret;
}

static enum install_res install_from_default_dir(void)
{
    const WCHAR *package_dir;
    WCHAR *dir_buf = NULL;
    enum install_res ret = INSTALL_NEXT;

    if ((package_dir = _wgetenv( L"WINEBUILDDIR" )))
    {
        dir_buf = malloc( wcslen(package_dir) * sizeof(WCHAR) + sizeof(L"\\..\\") );
        lstrcpyW( dir_buf, package_dir );
        lstrcatW( dir_buf, L"\\..\\" );
        package_dir = dir_buf;
    }
    else package_dir = _wgetenv( L"WINEDATADIR" );

    if (package_dir)
    {
        ret = install_from_dos_file(package_dir, addon->subdir_name, addon->file_name);
        free(dir_buf);
    }

    if (ret == INSTALL_NEXT)
        ret = install_from_unix_file(INSTALL_DATADIR "/wine/", addon->subdir_name, addon->file_name);
    if (ret == INSTALL_NEXT && strcmp(INSTALL_DATADIR, "/usr/share") != 0)
        ret = install_from_unix_file("/usr/share/wine/", addon->subdir_name, addon->file_name);
    if (ret == INSTALL_NEXT)
        ret = install_from_unix_file("/opt/wine/", addon->subdir_name, addon->file_name);
    return ret;
}

static WCHAR *get_cache_file_name(BOOL ensure_exists)
{
    const WCHAR *xdg_dir;
    const WCHAR *home_dir;
    WCHAR *cache_dir=NULL, *ret;
    size_t len, size;

    xdg_dir = _wgetenv( L"XDG_CACHE_HOME" );
    if (xdg_dir && *xdg_dir)
    {
        if (!(cache_dir = HeapAlloc( GetProcessHeap(), 0, wcslen(xdg_dir) * sizeof(WCHAR) + sizeof(L"\\\\?\\unix") ))) return NULL;
        lstrcpyW( cache_dir, L"\\\\?\\unix" );
        lstrcatW( cache_dir, xdg_dir );
        TRACE("cache dir %s\n", debugstr_w(cache_dir));
    }
    else if ((home_dir = _wgetenv( L"WINEHOMEDIR" )))
    {
        if (!(cache_dir = HeapAlloc( GetProcessHeap(), 0, wcslen(home_dir) * sizeof(WCHAR) + sizeof(L"\\.cache") ))) return NULL;
        lstrcpyW( cache_dir, home_dir );
        lstrcatW( cache_dir, L"\\.cache" );
        cache_dir[1] = '\\';  /* change \??\ into \\?\ */
    }
    else return NULL;

    if (ensure_exists && !CreateDirectoryW( cache_dir, NULL ) && GetLastError() != ERROR_ALREADY_EXISTS)
    {
        WARN( "%s does not exist and could not be created (%lu)\n", debugstr_w(cache_dir), GetLastError() );
        HeapFree( GetProcessHeap(), 0, cache_dir );
        return NULL;
    }

    size = lstrlenW( cache_dir ) + ARRAY_SIZE(L"\\wine") + lstrlenW( addon->file_name ) + 1;
    if (!(ret = malloc( size * sizeof(WCHAR) )))
    {
        HeapFree( GetProcessHeap(), 0, cache_dir );
        return NULL;
    }
    lstrcpyW( ret, cache_dir );
    lstrcatW( ret, L"\\wine" );
    HeapFree( GetProcessHeap(), 0, cache_dir );

    if (ensure_exists && !CreateDirectoryW( ret, NULL ) && GetLastError() != ERROR_ALREADY_EXISTS)
    {
        WARN( "%s does not exist and could not be created (%lu)\n", debugstr_w(ret), GetLastError() );
        free( ret );
        return NULL;
    }
    len = lstrlenW( ret );
    ret[len++] = '\\';
    lstrcpyW( ret + len, addon->file_name );

    TRACE( "got %s\n", debugstr_w(ret) );
    return ret;
}

static enum install_res install_from_cache(void)
{
    WCHAR *cache_file_name;
    enum install_res res;

    cache_file_name = get_cache_file_name(FALSE);
    if(!cache_file_name)
        return INSTALL_NEXT;

    if(!sha_check(cache_file_name)) {
        WARN("could not validate checksum\n");
        DeleteFileW(cache_file_name);
        free(cache_file_name);
        return INSTALL_NEXT;
    }

    res = install_file(cache_file_name);
    free(cache_file_name);
    return res;
}

static IInternetBindInfo InstallCallbackBindInfo;

static HRESULT WINAPI InstallCallback_QueryInterface(IBindStatusCallback *iface,
        REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IBindStatusCallback, riid)) {
        *ppv = iface;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IInternetBindInfo, riid)) {
        TRACE("IID_IInternetBindInfo\n");
        *ppv = &InstallCallbackBindInfo;
        return S_OK;
    }

    return E_INVALIDARG;
}

static ULONG WINAPI InstallCallback_AddRef(IBindStatusCallback *iface)
{
    return 2;
}

static ULONG WINAPI InstallCallback_Release(IBindStatusCallback *iface)
{
    return 1;
}

static HRESULT WINAPI InstallCallback_OnStartBinding(IBindStatusCallback *iface,
        DWORD dwReserved, IBinding *pib)
{
    set_status(IDS_DOWNLOADING);

    IBinding_AddRef(pib);
    dwl_binding = pib;

    return S_OK;
}

static HRESULT WINAPI InstallCallback_GetPriority(IBindStatusCallback *iface,
        LONG *pnPriority)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI InstallCallback_OnLowResource(IBindStatusCallback *iface,
       DWORD dwReserved)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI InstallCallback_OnProgress(IBindStatusCallback *iface, ULONG ulProgress,
        ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    HWND progress = GetDlgItem(install_dialog, ID_DWL_PROGRESS);

    if(ulProgressMax)
        SendMessageW(progress, PBM_SETRANGE32, 0, ulProgressMax);
    if(ulProgress)
        SendMessageW(progress, PBM_SETPOS, ulProgress, 0);

    return S_OK;
}

static HRESULT WINAPI InstallCallback_OnStopBinding(IBindStatusCallback *iface,
        HRESULT hresult, LPCWSTR szError)
{
    WCHAR message[256];

    if(dwl_binding) {
        IBinding_Release(dwl_binding);
        dwl_binding = NULL;
    }

    if(FAILED(hresult)) {
        if(hresult == E_ABORT)
            TRACE("Binding aborted\n");
        else if (hresult == INET_E_DOWNLOAD_FAILURE)
        {
            if(LoadStringW(hInst, IDS_DOWNLOAD_FAILED, message, ARRAY_SIZE(message)))
                MessageBoxW(install_dialog, message, NULL, MB_ICONERROR);
            EndDialog(install_dialog, IDCANCEL);
        }
        else
            ERR("Binding failed %08lx\n", hresult);
        return S_OK;
    }

    if(!msi_file) {
        ERR("No MSI file\n");
        return E_FAIL;
    }

    set_status(IDS_INSTALLING);
    EnableWindow(GetDlgItem(install_dialog, IDCANCEL), 0);

    if(sha_check(msi_file)) {
        WCHAR *cache_file_name;

        install_file(msi_file);

        cache_file_name = get_cache_file_name(TRUE);
        if(cache_file_name) {
            CopyFileW(msi_file, cache_file_name, FALSE);
            free(cache_file_name);
        }
    }else {
        if(LoadStringW(hInst, IDS_INVALID_SHA, message, ARRAY_SIZE(message)))
            MessageBoxW(NULL, message, NULL, MB_ICONERROR);
    }

    DeleteFileW(msi_file);
    free(msi_file);
    msi_file = NULL;

    EndDialog(install_dialog, 0);
    return S_OK;
}

static HRESULT WINAPI InstallCallback_GetBindInfo(IBindStatusCallback *iface,
        DWORD* grfBINDF, BINDINFO* pbindinfo)
{
    TRACE("()\n");

    *grfBINDF = BINDF_ASYNCHRONOUS;
    return S_OK;
}

static HRESULT WINAPI InstallCallback_OnDataAvailable(IBindStatusCallback *iface, DWORD grfBSCF,
        DWORD dwSize, FORMATETC* pformatetc, STGMEDIUM* pstgmed)
{
    if(!msi_file) {
        msi_file = wcsdup(pstgmed->lpszFileName);
        TRACE("got file name %s\n", debugstr_w(msi_file));
    }

    return S_OK;
}

static HRESULT WINAPI InstallCallback_OnObjectAvailable(IBindStatusCallback *iface,
        REFIID riid, IUnknown* punk)
{
    ERR("\n");
    return E_NOTIMPL;
}

static const IBindStatusCallbackVtbl InstallCallbackVtbl = {
    InstallCallback_QueryInterface,
    InstallCallback_AddRef,
    InstallCallback_Release,
    InstallCallback_OnStartBinding,
    InstallCallback_GetPriority,
    InstallCallback_OnLowResource,
    InstallCallback_OnProgress,
    InstallCallback_OnStopBinding,
    InstallCallback_GetBindInfo,
    InstallCallback_OnDataAvailable,
    InstallCallback_OnObjectAvailable
};

static IBindStatusCallback InstallCallback = { &InstallCallbackVtbl };

static HRESULT WINAPI InstallCallbackBindInfo_QueryInterface(IInternetBindInfo *iface, REFIID riid, void **ppv)
{
    return IBindStatusCallback_QueryInterface(&InstallCallback, riid, ppv);
}

static ULONG WINAPI InstallCallbackBindInfo_AddRef(IInternetBindInfo *iface)
{
    return 2;
}

static ULONG WINAPI InstallCallbackBindInfo_Release(IInternetBindInfo *iface)
{
    return 1;
}

static HRESULT WINAPI InstallCallbackBindInfo_GetBindInfo(IInternetBindInfo *iface, DWORD *bindf, BINDINFO *bindinfo)
{
    ERR("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InstallCallbackBindInfo_GetBindString(IInternetBindInfo *iface, ULONG string_type,
        WCHAR **strs, ULONG cnt, ULONG *fetched)
{
    switch(string_type) {
    case BINDSTRING_USER_AGENT:
        TRACE("BINDSTRING_USER_AGENT\n");

        *strs = CoTaskMemAlloc(sizeof(L"Wine Addon Downloader"));
        if(!*strs)
            return E_OUTOFMEMORY;

        lstrcpyW(*strs, L"Wine Addon Downloader");
        *fetched = 1;
        return S_OK;
    }

    return E_NOTIMPL;
}

static const IInternetBindInfoVtbl InstallCallbackBindInfoVtbl = {
    InstallCallbackBindInfo_QueryInterface,
    InstallCallbackBindInfo_AddRef,
    InstallCallbackBindInfo_Release,
    InstallCallbackBindInfo_GetBindInfo,
    InstallCallbackBindInfo_GetBindString
};

static IInternetBindInfo InstallCallbackBindInfo = { &InstallCallbackBindInfoVtbl };

static void append_url_params( WCHAR *url )
{
    DWORD size = INTERNET_MAX_URL_LENGTH * sizeof(WCHAR);
    DWORD len = lstrlenW(url);

    lstrcpyW(url+len, L"?arch=");
    len += lstrlenW(L"?arch=");
    len += MultiByteToWideChar(CP_ACP, 0, GECKO_ARCH, sizeof(GECKO_ARCH),
                               url+len, size/sizeof(WCHAR)-len)-1;
    lstrcpyW(url+len, L"&v=");
    len += lstrlenW(L"&v=");
    len += MultiByteToWideChar(CP_ACP, 0, addon->version, -1, url+len, size/sizeof(WCHAR)-len)-1;
    lstrcpyW(url+len, L"&winev=");
    len += lstrlenW(L"&winev=");
    MultiByteToWideChar(CP_ACP, 0, p_wine_get_version ? p_wine_get_version() : 0, -1, url+len, size/sizeof(WCHAR)-len);
}

static LPWSTR get_url(void)
{
    DWORD size = INTERNET_MAX_URL_LENGTH*sizeof(WCHAR);
    WCHAR *url;
    HKEY hkey;
    DWORD res, type;
    DWORD returned_size;

    static const WCHAR httpW[] = {'h','t','t','p'};

    url = malloc(size);
    returned_size = size;

    hkey = open_config_key();
    if (hkey)
    {
        res = RegQueryValueExW(hkey, addon->url_config_key, NULL, &type, (LPBYTE)url, &returned_size);
        RegCloseKey(hkey);
        if(res == ERROR_SUCCESS && type == REG_SZ) goto found;
    }

    MultiByteToWideChar( CP_ACP, 0, addon->url_default, -1, url, size / sizeof(WCHAR) );

found:
    if (returned_size > sizeof(httpW) && !memcmp(url, httpW, sizeof(httpW))) append_url_params( url );

    TRACE("Got URL %s\n", debugstr_w(url));
    return url;
}

static BOOL start_download(void)
{
    IBindCtx *bind_ctx;
    IMoniker *mon;
    IUnknown *tmp;
    HRESULT hres;

    hres = CreateURLMoniker(NULL, url, &mon);
    if(FAILED(hres))
        return FALSE;

    hres = CreateAsyncBindCtx(0, &InstallCallback, NULL, &bind_ctx);
    if(SUCCEEDED(hres)) {
        hres = IMoniker_BindToStorage(mon, bind_ctx, NULL, &IID_IUnknown, (void**)&tmp);
        IBindCtx_Release(bind_ctx);
    }
    IMoniker_Release(mon);
    if(FAILED(hres))
        return FALSE;

    if(tmp)
        IUnknown_Release(tmp);
    return TRUE;
}

static void run_winebrowser(const WCHAR *url)
{
    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    WCHAR app[MAX_PATH];
    LONG len, url_len;
    WCHAR *args;
    BOOL ret;

    url_len = lstrlenW(url);

    len = GetSystemDirectoryW(app, MAX_PATH - ARRAY_SIZE(L"\\winebrowser.exe"));
    lstrcpyW(app+len, L"\\winebrowser.exe");
    len += ARRAY_SIZE(L"\\winebrowser.exe") - 1;

    args = malloc((len + 1 + url_len) * sizeof(WCHAR));
    if(!args)
        return;

    memcpy(args, app, len*sizeof(WCHAR));
    args[len++] = ' ';
    memcpy(args+len, url, (url_len+1) * sizeof(WCHAR));

    TRACE("starting %s\n", debugstr_w(args));

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    ret = CreateProcessW(app, args, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &si, &pi);
    free(args);
    if (ret) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
}

static INT_PTR CALLBACK installer_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
    case WM_INITDIALOG:
        ShowWindow(GetDlgItem(hwnd, ID_DWL_PROGRESS), SW_HIDE);
        install_dialog = hwnd;
        return TRUE;

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code)
        {
        case NM_CLICK:
        case NM_RETURN:
            if (wParam == ID_DWL_STATUS)
                run_winebrowser(((NMLINK*)lParam)->item.szUrl);
            break;
        }
        break;

    case WM_COMMAND:
        switch(wParam) {
        case IDCANCEL:
            if(dwl_binding)
                IBinding_Abort(dwl_binding);
            EndDialog(hwnd, 0);
            return FALSE;

        case ID_DWL_INSTALL:
            ShowWindow(GetDlgItem(hwnd, ID_DWL_PROGRESS), SW_SHOW);
            EnableWindow(GetDlgItem(hwnd, ID_DWL_INSTALL), 0);
            if(!start_download())
                EndDialog(install_dialog, 0);
        }
    }

    return FALSE;
}

BOOL install_addon(addon_t addon_type)
{
    if(!*GECKO_ARCH)
        return FALSE;

    addon = addons_info+addon_type;

    p_wine_get_dos_file_name = (void *)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "wine_get_dos_file_name");
    p_wine_get_version = (void *)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "wine_get_version");

    /*
     * Try to find addon .msi file in following order:
     * - directory stored in $dir_config_key value of HKCU/Software/Wine/$config_key key
     * - $datadir/$addon_subdir/
     * - $INSTALL_DATADIR/wine/$addon_subdir/
     * - /usr/share/wine/$addon_subdir/
     * - /opt/wine/$addon_subdir/
     * - download from URL stored in $url_config_key value of HKCU/Software/Wine/$config_key key
     */
    if (install_from_registered_dir() == INSTALL_NEXT
        && install_from_default_dir() == INSTALL_NEXT
        && install_from_cache() == INSTALL_NEXT
        && (url = get_url()))
        DialogBoxW(hInst, addon->dialog_template, 0, installer_proc);

    free(url);
    url = NULL;
    return TRUE;
}
