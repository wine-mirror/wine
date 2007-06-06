/*
 * Copyright 2006 Jacek Caban for CodeWeavers
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

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "ole2.h"
#include "commctrl.h"
#include "advpub.h"
#include "wininet.h"
#include "shellapi.h"

#include "wine/debug.h"
#include "wine/unicode.h"

#include "mshtml_private.h"
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

static HWND install_dialog = NULL;
static LPWSTR tmp_file_name = NULL;
static HANDLE tmp_file = INVALID_HANDLE_VALUE;
static LPWSTR url = NULL;

static void clean_up(void)
{
    if(tmp_file != INVALID_HANDLE_VALUE)
        CloseHandle(tmp_file);

    if(tmp_file_name) {
        DeleteFileW(tmp_file_name);
        mshtml_free(tmp_file_name);
        tmp_file_name = NULL;
    }

    if(tmp_file != INVALID_HANDLE_VALUE) {
        CloseHandle(tmp_file);
        tmp_file = INVALID_HANDLE_VALUE;
    }

    if(install_dialog)
        EndDialog(install_dialog, 0);
}

static void set_status(DWORD id)
{
    HWND status = GetDlgItem(install_dialog, ID_DWL_STATUS);
    WCHAR buf[64];

    LoadStringW(hInst, id, buf, sizeof(buf)/sizeof(WCHAR));
    SendMessageW(status, WM_SETTEXT, 0, (LPARAM)buf);
}

static void set_registry(LPCSTR install_dir)
{
    WCHAR mshtml_key[100];
    LPWSTR gecko_path;
    HKEY hkey;
    DWORD res, len, size;

    static const WCHAR wszMshtmlKey[] = {
        'S','o','f','t','w','a','r','e','\\','W','i','n','e',
        '\\','M','S','H','T','M','L','\\'};
    static const WCHAR wszGeckoPath[] = {'G','e','c','k','o','P','a','t','h',0};
    static const WCHAR wszWineGecko[] = {'w','i','n','e','_','g','e','c','k','o',0};

    memcpy(mshtml_key, wszMshtmlKey, sizeof(wszMshtmlKey));
    MultiByteToWideChar(CP_ACP, 0, GECKO_VERSION, sizeof(GECKO_VERSION),
            mshtml_key+sizeof(wszMshtmlKey)/sizeof(WCHAR),
            (sizeof(mshtml_key)-sizeof(wszMshtmlKey))/sizeof(WCHAR));

    /* @@ Wine registry key: HKCU\Software\Wine\MSHTML\<version> */
    res = RegCreateKeyW(HKEY_CURRENT_USER, mshtml_key, &hkey);
    if(res != ERROR_SUCCESS) {
        ERR("Faild to create MSHTML key: %d\n", res);
        return;
    }

    len = MultiByteToWideChar(CP_ACP, 0, install_dir, -1, NULL, 0)-1;
    gecko_path = mshtml_alloc((len+1)*sizeof(WCHAR)+sizeof(wszWineGecko));
    MultiByteToWideChar(CP_ACP, 0, install_dir, -1, gecko_path, (len+1)*sizeof(WCHAR));

    if (len && gecko_path[len-1] != '\\')
        gecko_path[len++] = '\\';

    memcpy(gecko_path+len, wszWineGecko, sizeof(wszWineGecko));

    size = len*sizeof(WCHAR)+sizeof(wszWineGecko);
    res = RegSetValueExW(hkey, wszGeckoPath, 0, REG_SZ, (LPVOID)gecko_path,
                       len*sizeof(WCHAR)+sizeof(wszWineGecko));
    mshtml_free(gecko_path);
    RegCloseKey(hkey);
    if(res != ERROR_SUCCESS)
        ERR("Failed to set GeckoPath value: %08x\n", res);
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
    WCHAR tmp_dir[MAX_PATH];

    set_status(IDS_DOWNLOADING);

    GetTempPathW(sizeof(tmp_dir)/sizeof(WCHAR), tmp_dir);

    tmp_file_name = mshtml_alloc(MAX_PATH*sizeof(WCHAR));
    GetTempFileNameW(tmp_dir, NULL, 0, tmp_file_name);

    TRACE("creating temp file %s\n", debugstr_w(tmp_file_name));

    tmp_file = CreateFileW(tmp_file_name, GENERIC_WRITE, 0, NULL, 
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if(tmp_file == INVALID_HANDLE_VALUE) {
        ERR("Could not create file: %d\n", GetLastError());
        clean_up();
        return E_FAIL;
    }

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
    LPSTR file_name;
    DWORD len;
    HMODULE advpack;
    char install_dir[MAX_PATH];
    typeof(ExtractFilesA) *pExtractFilesA;
    BOOL res;
    HRESULT hres;

    static const WCHAR wszAdvpack[] = {'a','d','v','p','a','c','k','.','d','l','l',0};

    if(FAILED(hresult)) {
        ERR("Binding failed %08x\n", hresult);
        clean_up();
        return S_OK;
    }

    CloseHandle(tmp_file);
    tmp_file = INVALID_HANDLE_VALUE;

    set_status(IDS_INSTALLING);

    GetWindowsDirectoryA(install_dir, sizeof(install_dir));
    strcat(install_dir, "\\gecko\\");
    res = CreateDirectoryA(install_dir, NULL);
    if(!res && GetLastError() != ERROR_ALREADY_EXISTS) {
        ERR("Could not create directory: %08u\n", GetLastError());
        return S_OK;
    }

    strcat(install_dir, GECKO_VERSION);
    res = CreateDirectoryA(install_dir, NULL);
    if(!res && GetLastError() != ERROR_ALREADY_EXISTS) {
        ERR("Could not create directory: %08u\n", GetLastError());
        return S_OK;
    }

    advpack = LoadLibraryW(wszAdvpack);
    pExtractFilesA = (typeof(ExtractFilesA)*)GetProcAddress(advpack, "ExtractFiles");

    len = WideCharToMultiByte(CP_ACP, 0, tmp_file_name, -1, NULL, 0, NULL, NULL);
    file_name = mshtml_alloc(len);
    WideCharToMultiByte(CP_ACP, 0, tmp_file_name, -1, file_name, -1, NULL, NULL);

    /* FIXME: Use unicode version (not yet implemented) */
    hres = pExtractFilesA(file_name, install_dir, 0, NULL, NULL, 0);
    FreeLibrary(advpack);
    mshtml_free(file_name);
    if(FAILED(hres)) {
        ERR("Could not extract package: %08x\n", hres);
        clean_up();
    }

    set_registry(install_dir);
    clean_up();

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
    IStream *str = pstgmed->u.pstm;
    BYTE buf[1024];
    DWORD size;
    HRESULT hres;

    do {
        size = 0;
        hres = IStream_Read(str, buf, sizeof(buf), &size);
        if(size)
            WriteFile(tmp_file, buf, size, NULL, NULL);
    }while(hres == S_OK);

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

static LPWSTR get_url(void)
{
    HKEY hkey;
    DWORD res, type;
    DWORD size = INTERNET_MAX_URL_LENGTH*sizeof(WCHAR);
    LPWSTR url;

    static const WCHAR wszMshtmlKey[] = {
        'S','o','f','t','w','a','r','e','\\','W','i','n','e',
        '\\','M','S','H','T','M','L',0};
    static const WCHAR wszGeckoUrl[] = {'G','e','c','k','o','U','r','l',0};
    static const WCHAR httpW[] = {'h','t','t','p'};
    static const WCHAR v_formatW[] = {'?','v','=',0};

    /* @@ Wine registry key: HKCU\Software\Wine\MSHTML */
    res = RegOpenKeyW(HKEY_CURRENT_USER, wszMshtmlKey, &hkey);
    if(res != ERROR_SUCCESS)
        return NULL;

    url = mshtml_alloc(size);

    res = RegQueryValueExW(hkey, wszGeckoUrl, NULL, &type, (LPBYTE)url, &size);
    RegCloseKey(hkey);
    if(res != ERROR_SUCCESS || type != REG_SZ) {
        mshtml_free(url);
        return NULL;
    }

    if(size > sizeof(httpW) && !memcmp(url, httpW, sizeof(httpW))) {
        strcatW(url, v_formatW);
        MultiByteToWideChar(CP_ACP, 0, GECKO_VERSION, -1, url+strlenW(url), -1);
    }

    TRACE("Got URL %s\n", debugstr_w(url));
    return url;
}

static DWORD WINAPI download_proc(PVOID arg)
{
    IMoniker *mon;
    IBindCtx *bctx;
    IStream *str = NULL;
    HRESULT hres;

    CreateURLMoniker(NULL, url, &mon);
    mshtml_free(url);
    url = NULL;

    CreateAsyncBindCtx(0, &InstallCallback, 0, &bctx);

    hres = IMoniker_BindToStorage(mon, bctx, NULL, &IID_IStream, (void**)&str);
    IBindCtx_Release(bctx);
    if(FAILED(hres)) {
        ERR("BindToStorage failed: %08x\n", hres);
        return 0;
    }

    if(str)
        IStream_Release(str);

    return 0;
}

static INT_PTR CALLBACK installer_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
    case WM_INITDIALOG:
        install_dialog = hwnd;
        return TRUE;

    case WM_COMMAND:
        switch(wParam) {
        case IDCANCEL:
            EndDialog(hwnd, 0);
            return FALSE;

        case ID_DWL_INSTALL:
            EnableWindow(GetDlgItem(hwnd, ID_DWL_INSTALL), 0);
            EnableWindow(GetDlgItem(hwnd, IDCANCEL), 0); /* FIXME */
            CreateThread(NULL, 0, download_proc, NULL, 0, NULL);
            return FALSE;
        }
    }

    return FALSE;
}

BOOL install_wine_gecko(void)
{
    HANDLE hsem;

    SetLastError(ERROR_SUCCESS);
    hsem = CreateSemaphoreA( NULL, 0, 1, "mshtml_install_semaphore");

    if(GetLastError() == ERROR_ALREADY_EXISTS) {
        WaitForSingleObject(hsem, INFINITE);
    }else {
        if((url = get_url()))
            DialogBoxW(hInst, MAKEINTRESOURCEW(ID_DWL_DIALOG), 0, installer_proc);
    }

    ReleaseSemaphore(hsem, 1, NULL);
    CloseHandle(hsem);

    return TRUE;
}
