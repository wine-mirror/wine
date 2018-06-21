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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#define COBJMACROS
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "cpl.h"
#include "winreg.h"
#include "ole2.h"
#include "commctrl.h"
#include "advpub.h"
#include "wininet.h"
#include "shellapi.h"
#include "urlmon.h"
#include "msi.h"
#include "bcrypt.h"

#include "appwiz.h"
#include "res.h"

#include "wine/debug.h"
#include "wine/library.h"

WINE_DEFAULT_DEBUG_CHANNEL(appwizcpl);

#define GECKO_VERSION "2.47"

#ifdef __i386__
#define ARCH_STRING "x86"
#define GECKO_SHA "3b8a361f5d63952d21caafd74e849a774994822fb96c5922b01d554f1677643a"
#elif defined(__x86_64__)
#define ARCH_STRING "x86_64"
#define GECKO_SHA "c565ea25e50ea953937d4ab01299e4306da4a556946327d253ea9b28357e4a7d"
#else
#define ARCH_STRING ""
#define GECKO_SHA "???"
#endif

#define MONO_VERSION "4.7.1"
#define MONO_SHA "2c8d5db7f833c3413b2519991f5af1f433d59a927564ec6f38a3f1f8b2c629aa"

typedef struct {
    const char *version;
    const char *file_name;
    const char *subdir_name;
    const char *sha;
    const char *url_default;
    const char *config_key;
    const char *url_config_key;
    const char *dir_config_key;
    LPCWSTR dialog_template;
} addon_info_t;

/* Download addon files over HTTP because Wine depends on an external library
 * for TLS, so we can't be sure that HTTPS will work. The integrity of each file
 * is checked with a hardcoded cryptographically secure hash. */
static const addon_info_t addons_info[] = {
    {
        GECKO_VERSION,
        "wine_gecko-" GECKO_VERSION "-" ARCH_STRING ".msi",
        "gecko",
        GECKO_SHA,
        "http://source.winehq.org/winegecko.php",
        "MSHTML", "GeckoUrl", "GeckoCabDir",
        MAKEINTRESOURCEW(ID_DWL_GECKO_DIALOG)
    },
    {
        MONO_VERSION,
        "wine-mono-" MONO_VERSION ".msi",
        "mono",
        MONO_SHA,
        "http://source.winehq.org/winemono.php",
        "Dotnet", "MonoUrl", "MonoCabDir",
        MAKEINTRESOURCEW(ID_DWL_MONO_DIALOG)
    }
};

static const addon_info_t *addon;

static HWND install_dialog = NULL;
static LPWSTR url = NULL;
static IBinding *dwl_binding;
static WCHAR *msi_file;

static WCHAR * (CDECL *p_wine_get_dos_file_name)(const char*);
static const WCHAR kernel32_dllW[] = {'k','e','r','n','e','l','3','2','.','d','l','l',0};

static BOOL sha_check(const WCHAR *file_name)
{
    const unsigned char *file_map;
    HANDLE file, map;
    DWORD size, i;
    BCRYPT_HASH_HANDLE hash = NULL;
    BCRYPT_ALG_HANDLE alg = NULL;
    UCHAR sha[32];
    char buf[1024];
    BOOL ret = FALSE;

    file = CreateFileW(file_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    if(file == INVALID_HANDLE_VALUE) {
        WARN("Could not open file: %u\n", GetLastError());
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

    if(BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, MS_PRIMITIVE_PROVIDER, 0))
        goto end;
    if(BCryptCreateHash(alg, &hash, NULL, 0, NULL, 0, 0))
        goto end;
    if(BCryptHashData(hash, (UCHAR *)file_map, size, 0))
        goto end;
    if(BCryptFinishHash(hash, sha, sizeof(sha), 0))
        goto end;

    for(i=0; i < sizeof(sha); i++)
        sprintf(buf + i * 2, "%02x", sha[i]);

    ret = !strcmp(buf, addon->sha);
    if(!ret)
        WARN("Got %s, expected %s\n", buf, addon->sha);

end:
    UnmapViewOfFile(file_map);
    if(hash) BCryptDestroyHash(hash);
    if(alg) BCryptCloseAlgorithmProvider(alg, 0);
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
    static const WCHAR update_cmd[] = {
        'R','E','I','N','S','T','A','L','L','=','A','L','L',' ',
        'R','E','I','N','S','T','A','L','L','M','O','D','E','=','v','o','m','u','s',0};
    ULONG res;

    res = MsiInstallProductW(file_name, NULL);
    if(res == ERROR_PRODUCT_VERSION)
        res = MsiInstallProductW(file_name, update_cmd);
    if(res != ERROR_SUCCESS) {
        ERR("MsiInstallProduct failed: %u\n", res);
        return INSTALL_FAILED;
    }

    return INSTALL_OK;
}

static enum install_res install_from_unix_file(const char *dir, const char *subdir, const char *file_name)
{
    LPWSTR dos_file_name;
    char *file_path;
    int fd, len;
    enum install_res ret;

    len = strlen(dir);
    file_path = heap_alloc(len+strlen(subdir)+strlen(file_name)+3);
    if(!file_path)
        return INSTALL_FAILED;

    memcpy(file_path, dir, len);
    if(len && file_path[len-1] != '/' && file_path[len-1] != '\\')
        file_path[len++] = '/';
    if(*subdir) {
        strcpy(file_path+len, subdir);
        len += strlen(subdir);
        file_path[len++] = '/';
    }
    strcpy(file_path+len, file_name);

    fd = open(file_path, O_RDONLY);
    if(fd == -1) {
        TRACE("%s not found\n", debugstr_a(file_path));
        heap_free(file_path);
        return INSTALL_NEXT;
    }

    close(fd);

    if(p_wine_get_dos_file_name) { /* Wine UNIX mode */
	dos_file_name = p_wine_get_dos_file_name(file_path);
	if(!dos_file_name) {
	    ERR("Could not get dos file name of %s\n", debugstr_a(file_path));
            heap_free(file_path);
	    return INSTALL_FAILED;
	}
    } else { /* Windows mode */
	UINT res;
	WARN("Could not get wine_get_dos_file_name function, calling install_cab directly.\n");
	res = MultiByteToWideChar( CP_ACP, 0, file_path, -1, 0, 0);
	dos_file_name = heap_alloc (res*sizeof(WCHAR));
	MultiByteToWideChar( CP_ACP, 0, file_path, -1, dos_file_name, res);
    }

    heap_free(file_path);

    ret = install_file(dos_file_name);

    heap_free(dos_file_name);
    return ret;
}

static HKEY open_config_key(void)
{
    HKEY hkey, ret;
    DWORD res;

    static const WCHAR wine_keyW[] = {'S','o','f','t','w','a','r','e','\\','W','i','n','e',0};

    /* @@ Wine registry key: HKCU\Software\Wine\$config_key */
    res = RegOpenKeyW(HKEY_CURRENT_USER, wine_keyW, &hkey);
    if(res != ERROR_SUCCESS)
        return NULL;

    res = RegOpenKeyA(hkey, addon->config_key, &ret);
    RegCloseKey(hkey);
    return res == ERROR_SUCCESS ? ret : NULL;
}

static enum install_res install_from_registered_dir(void)
{
    char *package_dir;
    HKEY hkey;
    DWORD res, type, size = MAX_PATH;
    enum install_res ret;

    hkey = open_config_key();
    if(!hkey)
        return INSTALL_NEXT;

    package_dir = heap_alloc(size);
    res = RegGetValueA(hkey, NULL, addon->dir_config_key, RRF_RT_ANY, &type, (PBYTE)package_dir, &size);
    if(res == ERROR_MORE_DATA) {
        package_dir = heap_realloc(package_dir, size);
        res = RegGetValueA(hkey, NULL, addon->dir_config_key, RRF_RT_ANY, &type, (PBYTE)package_dir, &size);
    }
    RegCloseKey(hkey);
    if(res == ERROR_FILE_NOT_FOUND) {
        heap_free(package_dir);
        return INSTALL_NEXT;
    } else if(res != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ)) {
        heap_free(package_dir);
        return INSTALL_FAILED;
    }

    TRACE("Trying %s/%s\n", debugstr_a(package_dir), debugstr_a(addon->file_name));

    ret = install_from_unix_file(package_dir, "", addon->file_name);

    heap_free(package_dir);
    return ret;
}

static enum install_res install_from_default_dir(void)
{
    const char *data_dir, *package_dir;
    char *dir_buf = NULL;
    int len;
    enum install_res ret;

    if((data_dir = wine_get_data_dir())) {
        package_dir = data_dir;
    }else if((data_dir = wine_get_build_dir())) {
        len = strlen(data_dir);
        dir_buf = heap_alloc(len + sizeof("/../"));
        memcpy(dir_buf, data_dir, len);
        strcpy(dir_buf+len, "/../");
        package_dir = dir_buf;
    }else {
        return INSTALL_NEXT;
    }

    ret = install_from_unix_file(package_dir, addon->subdir_name, addon->file_name);
    heap_free(dir_buf);

    if (ret == INSTALL_NEXT)
        ret = install_from_unix_file(INSTALL_DATADIR "/wine/", addon->subdir_name, addon->file_name);
    if (ret == INSTALL_NEXT && strcmp(INSTALL_DATADIR, "/usr/share"))
        ret = install_from_unix_file("/usr/share/wine/", addon->subdir_name, addon->file_name);
    return ret;
}

static WCHAR *get_cache_file_name(BOOL ensure_exists)
{
    const char *home_dir = NULL, *xdg_cache_dir;
    size_t len, size = strlen(addon->file_name) + 7; /* strlen("/wine/"), '\0' */
    char *cache_file_name;
    WCHAR *ret;

    /* non-Wine (eg. Windows) cache is currently not supported */
    if(!p_wine_get_dos_file_name)
        return NULL;

    xdg_cache_dir = getenv("XDG_CACHE_HOME");
    if(xdg_cache_dir && *xdg_cache_dir) {
        size += strlen(xdg_cache_dir);
    }else {
        home_dir = getenv("HOME");
        if(!home_dir)
            return NULL;

        size += strlen(home_dir) + 8; /* strlen("/.cache/") */
    }

    cache_file_name = heap_alloc(size);
    if(!cache_file_name)
        return NULL;

    if(xdg_cache_dir && *xdg_cache_dir) {
        len = strlen(xdg_cache_dir);
        if(len > 1 && xdg_cache_dir[len-1] == '/')
            len--;
        memcpy(cache_file_name, xdg_cache_dir, len);
        cache_file_name[len] = 0;
    }else {
        len = strlen(home_dir);
        memcpy(cache_file_name, home_dir, len);
        strcpy(cache_file_name+len, "/.cache");
        len += 7;
    }

    if(ensure_exists && mkdir(cache_file_name, 0777) && errno != EEXIST) {
        WARN("%s does not exist and could not be created: %s\n", cache_file_name, strerror(errno));
        heap_free(cache_file_name);
        return NULL;
    }

    strcpy(cache_file_name+len, "/wine");
    len += 5;

    if(ensure_exists && mkdir(cache_file_name, 0777) && errno != EEXIST) {
        WARN("%s does not exist and could not be created: %s\n", cache_file_name, strerror(errno));
        return NULL;
    }

    cache_file_name[len++] = '/';
    strcpy(cache_file_name+len, addon->file_name);
    ret = p_wine_get_dos_file_name(cache_file_name);

    TRACE("%s -> %s\n", cache_file_name, debugstr_w(ret));

    heap_free(cache_file_name);
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
        heap_free(cache_file_name);
        return INSTALL_NEXT;
    }

    res = install_file(cache_file_name);
    heap_free(cache_file_name);
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
    if(dwl_binding) {
        IBinding_Release(dwl_binding);
        dwl_binding = NULL;
    }

    if(FAILED(hresult)) {
        if(hresult == E_ABORT)
            TRACE("Binding aborted\n");
        else
            ERR("Binding failed %08x\n", hresult);
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
            MoveFileW(msi_file, cache_file_name);
            heap_free(cache_file_name);
        }
    }else {
        WCHAR message[256];

        if(LoadStringW(hInst, IDS_INVALID_SHA, message, ARRAY_SIZE(message)))
            MessageBoxW(NULL, message, NULL, MB_ICONERROR);
    }

    DeleteFileW(msi_file);
    heap_free(msi_file);
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
        msi_file = heap_strdupW(pstgmed->u.lpszFileName);
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
    static const WCHAR wine_addon_downloaderW[] =
        {'W','i','n','e',' ','A','d','d','o','n',' ','D','o','w','n','l','o','a','d','e','r',0};

    switch(string_type) {
    case BINDSTRING_USER_AGENT:
        TRACE("BINDSTRING_USER_AGENT\n");

        *strs = CoTaskMemAlloc(sizeof(wine_addon_downloaderW));
        if(!*strs)
            return E_OUTOFMEMORY;

        memcpy(*strs, wine_addon_downloaderW, sizeof(wine_addon_downloaderW));
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
    static const WCHAR arch_formatW[] = {'?','a','r','c','h','='};
    static const WCHAR v_formatW[] = {'&','v','='};
    static const WCHAR winevW[] = {'&','w','i','n','e','v','='};
    DWORD size = INTERNET_MAX_URL_LENGTH * sizeof(WCHAR);
    DWORD len = strlenW(url);

    memcpy(url+len, arch_formatW, sizeof(arch_formatW));
    len += sizeof(arch_formatW)/sizeof(WCHAR);
    len += MultiByteToWideChar(CP_ACP, 0, ARCH_STRING, sizeof(ARCH_STRING),
                               url+len, size/sizeof(WCHAR)-len)-1;
    memcpy(url+len, v_formatW, sizeof(v_formatW));
    len += sizeof(v_formatW)/sizeof(WCHAR);
    len += MultiByteToWideChar(CP_ACP, 0, addon->version, -1, url+len, size/sizeof(WCHAR)-len)-1;
    memcpy(url+len, winevW, sizeof(winevW));
    len += sizeof(winevW)/sizeof(WCHAR);
    MultiByteToWideChar(CP_ACP, 0, PACKAGE_VERSION, -1, url+len, size/sizeof(WCHAR)-len);
}

static LPWSTR get_url(void)
{
    DWORD size = INTERNET_MAX_URL_LENGTH*sizeof(WCHAR);
    WCHAR *url, *config_key;
    HKEY hkey;
    DWORD res, type;
    DWORD returned_size;

    static const WCHAR httpW[] = {'h','t','t','p'};

    url = heap_alloc(size);
    returned_size = size;

    hkey = open_config_key();
    if (hkey)
    {
        config_key = heap_strdupAtoW(addon->url_config_key);
        res = RegQueryValueExW(hkey, config_key, NULL, &type, (LPBYTE)url, &returned_size);
        heap_free(config_key);
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

    static const WCHAR winebrowserW[] = {'\\','w','i','n','e','b','r','o','w','s','e','r','.','e','x','e',0};

    url_len = strlenW(url);

    len = GetSystemDirectoryW(app, MAX_PATH-sizeof(winebrowserW)/sizeof(WCHAR));
    memcpy(app+len, winebrowserW, sizeof(winebrowserW));
    len += sizeof(winebrowserW)/sizeof(WCHAR) -1;

    args = heap_alloc((len+1+url_len)*sizeof(WCHAR));
    if(!args)
        return;

    memcpy(args, app, len*sizeof(WCHAR));
    args[len++] = ' ';
    memcpy(args+len, url, (url_len+1) * sizeof(WCHAR));

    TRACE("starting %s\n", debugstr_w(args));

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    ret = CreateProcessW(app, args, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &si, &pi);
    heap_free(args);
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
    if(!*ARCH_STRING)
        return FALSE;

    addon = addons_info+addon_type;

    p_wine_get_dos_file_name = (void*)GetProcAddress(GetModuleHandleW(kernel32_dllW), "wine_get_dos_file_name");

    /*
     * Try to find addon .msi file in following order:
     * - directory stored in $dir_config_key value of HKCU/Software/Wine/$config_key key
     * - $datadir/$addon_subdir/
     * - $INSTALL_DATADIR/wine/$addon_subdir/
     * - /usr/share/wine/$addon_subdir/
     * - download from URL stored in $url_config_key value of HKCU/Software/Wine/$config_key key
     */
    if (install_from_registered_dir() == INSTALL_NEXT
        && install_from_default_dir() == INSTALL_NEXT
        && install_from_cache() == INSTALL_NEXT
        && (url = get_url()))
        DialogBoxW(hInst, addon->dialog_template, 0, installer_proc);

    heap_free(url);
    url = NULL;
    return TRUE;
}
