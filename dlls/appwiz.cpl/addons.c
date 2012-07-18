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

#include <stdarg.h>
#include <fcntl.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

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

#include "appwiz.h"
#include "res.h"

#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/library.h"

WINE_DEFAULT_DEBUG_CHANNEL(appwizcpl);

#define GECKO_VERSION "1.7"

#ifdef __i386__
#define ARCH_STRING "x86"
#define GECKO_SHA "efebc4ed7a86708e2dc8581033a3c5d6effe0b0b"
#elif defined(__x86_64__)
#define ARCH_STRING "x86_64"
#define GECKO_SHA "2253e7ce3a699ddd110c6c9ce4c7ca7e6f7c02f5"
#else
#define ARCH_STRING ""
#define GECKO_SHA "???"
#endif

#define MONO_VERSION "0.0.4"
#define MONO_SHA "7d827f7d28a88ae0da95a136573783124ffce4b1"

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

/* SHA definitions are copied from advapi32. They aren't available in headers. */

typedef struct {
   ULONG Unknown[6];
   ULONG State[5];
   ULONG Count[2];
   UCHAR Buffer[64];
} SHA_CTX, *PSHA_CTX;

void WINAPI A_SHAInit(PSHA_CTX);
void WINAPI A_SHAUpdate(PSHA_CTX,const unsigned char*,UINT);
void WINAPI A_SHAFinal(PSHA_CTX,PULONG);

static BOOL sha_check(const WCHAR *file_name)
{
    const unsigned char *file_map;
    HANDLE file, map;
    ULONG sha[5];
    char buf[2*sizeof(sha)+1];
    SHA_CTX ctx;
    DWORD size, i;

    file = CreateFileW(file_name, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    if(file == INVALID_HANDLE_VALUE)
        return FALSE;

    size = GetFileSize(file, NULL);

    map = CreateFileMappingW(file, NULL, PAGE_READONLY, 0, 0, NULL);
    CloseHandle(file);
    if(!map)
        return FALSE;

    file_map = MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(map);
    if(!file_map)
        return FALSE;

    A_SHAInit(&ctx);
    A_SHAUpdate(&ctx, file_map, size);
    A_SHAFinal(&ctx, sha);

    UnmapViewOfFile(file_map);

    for(i=0; i < sizeof(sha); i++)
        sprintf(buf + i*2, "%02x", *((unsigned char*)sha+i));

    if(strcmp(buf, addon->sha)) {
        WCHAR message[256];

        WARN("Got %s, expected %s\n", buf, addon->sha);

        if(LoadStringW(hInst, IDS_INVALID_SHA, message, sizeof(message)/sizeof(WCHAR)))
            MessageBoxW(NULL, message, NULL, MB_ICONERROR);

        return FALSE;
    }

    return TRUE;
}

static void set_status(DWORD id)
{
    HWND status = GetDlgItem(install_dialog, ID_DWL_STATUS);
    WCHAR buf[64];

    LoadStringW(hInst, id, buf, sizeof(buf)/sizeof(WCHAR));
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

    static WCHAR * (CDECL *wine_get_dos_file_name)(const char*);
    static const WCHAR kernel32W[] = {'k','e','r','n','e','l','3','2','.','d','l','l',0};

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

    if(!wine_get_dos_file_name)
        wine_get_dos_file_name = (void*)GetProcAddress(GetModuleHandleW(kernel32W), "wine_get_dos_file_name");

    if(wine_get_dos_file_name) { /* Wine UNIX mode */
	dos_file_name = wine_get_dos_file_name(file_path);
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
    BOOL ret;

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

static HRESULT WINAPI InstallCallback_QueryInterface(IBindStatusCallback *iface,
        REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IBindStatusCallback, riid)) {
        *ppv = iface;
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
    if(FAILED(hresult)) {
        ERR("Binding failed %08x\n", hresult);
        return S_OK;
    }

    set_status(IDS_INSTALLING);
    return S_OK;
}

static HRESULT WINAPI InstallCallback_GetBindInfo(IBindStatusCallback *iface,
        DWORD* grfBINDF, BINDINFO* pbindinfo)
{
    /* FIXME */
    *grfBINDF = 0;
    return S_OK;
}

static HRESULT WINAPI InstallCallback_OnDataAvailable(IBindStatusCallback *iface, DWORD grfBSCF,
        DWORD dwSize, FORMATETC* pformatetc, STGMEDIUM* pstgmed)
{
    ERR("\n");
    return E_NOTIMPL;
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

static void append_url_params( WCHAR *url )
{
    static const WCHAR arch_formatW[] = {'?','a','r','c','h','='};
    static const WCHAR v_formatW[] = {'&','v','='};
    DWORD size = INTERNET_MAX_URL_LENGTH * sizeof(WCHAR);
    DWORD len = strlenW(url);

    memcpy(url+len, arch_formatW, sizeof(arch_formatW));
    len += sizeof(arch_formatW)/sizeof(WCHAR);
    len += MultiByteToWideChar(CP_ACP, 0, ARCH_STRING, sizeof(ARCH_STRING),
                               url+len, size/sizeof(WCHAR)-len)-1;
    memcpy(url+len, v_formatW, sizeof(v_formatW));
    len += sizeof(v_formatW)/sizeof(WCHAR);
    MultiByteToWideChar(CP_ACP, 0, addon->version, -1, url+len, size/sizeof(WCHAR)-len);
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

static DWORD WINAPI download_proc(PVOID arg)
{
    WCHAR tmp_dir[MAX_PATH], tmp_file[MAX_PATH];
    HRESULT hres;

    GetTempPathW(sizeof(tmp_dir)/sizeof(WCHAR), tmp_dir);
    GetTempFileNameW(tmp_dir, NULL, 0, tmp_file);

    TRACE("using temp file %s\n", debugstr_w(tmp_file));

    hres = URLDownloadToFileW(NULL, url, tmp_file, 0, &InstallCallback);
    if(FAILED(hres)) {
        ERR("URLDownloadToFile failed: %08x\n", hres);
        return 0;
    }

    if(sha_check(tmp_file))
        install_file(tmp_file);
    DeleteFileW(tmp_file);
    EndDialog(install_dialog, 0);
    return 0;
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
    ret = CreateProcessW(app, args, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
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
            EndDialog(hwnd, 0);
            return FALSE;

        case ID_DWL_INSTALL:
            ShowWindow(GetDlgItem(hwnd, ID_DWL_PROGRESS), SW_SHOW);
            EnableWindow(GetDlgItem(hwnd, ID_DWL_INSTALL), 0);
            EnableWindow(GetDlgItem(hwnd, IDCANCEL), 0); /* FIXME */
            CloseHandle( CreateThread(NULL, 0, download_proc, NULL, 0, NULL));
            return FALSE;
        }
    }

    return FALSE;
}

BOOL install_addon(addon_t addon_type)
{
    if(!*ARCH_STRING)
        return FALSE;

    addon = addons_info+addon_type;

    /*
     * Try to find addon .msi file in following order:
     * - directory stored in $dir_config_key value of HKCU/Wine/Software/$config_key key
     * - $datadir/$addon_subdir/
     * - $INSTALL_DATADIR/wine/$addon_subdir/
     * - /usr/share/wine/$addon_subdir/
     * - download from URL stored in $url_config_key value of HKCU/Wine/Software/$config_key key
     */
    if (install_from_registered_dir() == INSTALL_NEXT
        && install_from_default_dir() == INSTALL_NEXT
        && (url = get_url()))
        DialogBoxW(hInst, addon->dialog_template, 0, installer_proc);

    heap_free(url);
    url = NULL;
    return TRUE;
}
