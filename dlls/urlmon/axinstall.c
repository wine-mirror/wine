/*
 * Copyright 2012 Jacek Caban for CodeWeavers
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

#define OEMRESOURCE

#include <assert.h>

#include "urlmon_main.h"
#include "resource.h"

#include "advpub.h"
#include "fdi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

static const WCHAR ctxW[] = {'c','t','x',0};
static const WCHAR cab_extW[] = {'.','c','a','b',0};
static const WCHAR infW[] = {'i','n','f',0};
static const WCHAR dllW[] = {'d','l','l',0};
static const WCHAR ocxW[] = {'o','c','x',0};

enum install_type {
    INSTALL_UNKNOWN,
    INSTALL_DLL,
    INSTALL_INF
};

typedef struct {
    IUri *uri;
    IBindStatusCallback *callback;
    BOOL release_on_stop;
    BOOL cancel;
    WCHAR *install_file;
    const WCHAR *cache_file;
    const WCHAR *tmp_dir;
    const WCHAR *file_name;
    enum install_type install_type;
    HWND hwnd;
    int counter;
    INT_PTR timer;
} install_ctx_t;

static void release_install_ctx(install_ctx_t *ctx)
{
    if(ctx->uri)
        IUri_Release(ctx->uri);
    if(ctx->callback)
        IBindStatusCallback_Release(ctx->callback);
    heap_free(ctx->install_file);
    heap_free(ctx);
}

static inline BOOL file_exists(const WCHAR *file_name)
{
    return GetFileAttributesW(file_name) != INVALID_FILE_ATTRIBUTES;
}

static HRESULT extract_cab_file(install_ctx_t *ctx)
{
    size_t path_len, file_len;
    WCHAR *ptr;
    HRESULT hres;

    hres = ExtractFilesW(ctx->cache_file, ctx->tmp_dir, 0, NULL, NULL, 0);
    if(FAILED(hres)) {
        WARN("ExtractFilesW failed: %08x\n", hres);
        return hres;
    }

    path_len = strlenW(ctx->tmp_dir);
    file_len = strlenW(ctx->file_name);
    ctx->install_file = heap_alloc((path_len+file_len+2)*sizeof(WCHAR));
    if(!ctx->install_file)
        return E_OUTOFMEMORY;

    memcpy(ctx->install_file, ctx->tmp_dir, path_len*sizeof(WCHAR));
    ctx->install_file[path_len] = '\\';
    memcpy(ctx->install_file+path_len+1, ctx->file_name, (file_len+1)*sizeof(WCHAR));

    /* NOTE: Assume that file_name contains ".cab" extension */
    ptr = ctx->install_file+path_len+1+file_len-3;

    memcpy(ptr, infW, sizeof(infW));
    if(file_exists(ctx->install_file)) {
        ctx->install_type = INSTALL_INF;
        return S_OK;
    }

    memcpy(ptr, dllW, sizeof(dllW));
    if(file_exists(ctx->install_file)) {
        ctx->install_type = INSTALL_DLL;
        return S_OK;
    }

    memcpy(ptr, ocxW, sizeof(ocxW));
    if(file_exists(ctx->install_file)) {
        ctx->install_type = INSTALL_DLL;
        return S_OK;
    }

    FIXME("No known install file\n");
    return E_NOTIMPL;
}

static HRESULT setup_dll(install_ctx_t *ctx)
{
    HMODULE module;
    HRESULT hres;

    HRESULT (WINAPI *reg_func)(void);

    module = LoadLibraryW(ctx->install_file);
    if(!module)
        return E_FAIL;

    reg_func = (void*)GetProcAddress(module, "DllRegisterServer");
    if(reg_func) {
        hres = reg_func();
    }else {
        WARN("no DllRegisterServer function\n");
        hres = E_FAIL;
    }

    FreeLibrary(module);
    return hres;
}

static HRESULT install_cab_file(install_ctx_t *ctx)
{
    WCHAR tmp_path[MAX_PATH], tmp_dir[MAX_PATH];
    BOOL res = FALSE, leave_temp = FALSE;
    DWORD i;
    HRESULT hres;

    GetTempPathW(sizeof(tmp_path)/sizeof(WCHAR), tmp_path);

    for(i=0; !res && i < 100; i++) {
        GetTempFileNameW(tmp_path, NULL, GetTickCount() + i*17037, tmp_dir);
        res = CreateDirectoryW(tmp_dir, NULL);
    }
    if(!res)
        return E_FAIL;

    ctx->tmp_dir = tmp_dir;

    TRACE("Using temporary directory %s\n", debugstr_w(tmp_dir));

    hres = extract_cab_file(ctx);
    if(SUCCEEDED(hres)) {
        if(ctx->callback)
            IBindStatusCallback_OnProgress(ctx->callback, 0, 0, BINDSTATUS_INSTALLINGCOMPONENTS, ctx->install_file);

        switch(ctx->install_type) {
        case INSTALL_INF:
            hres = RunSetupCommandW(ctx->hwnd, ctx->install_file, NULL, ctx->tmp_dir, NULL, NULL, RSC_FLAG_INF, NULL);
            if(FAILED(hres))
                WARN("RunSetupCommandW failed: %08x\n", hres);
            break;
        case INSTALL_DLL:
            FIXME("Installing DLL, registering in temporary location\n");
            hres = setup_dll(ctx);
            if(SUCCEEDED(hres))
                leave_temp = TRUE;
            break;
        default:
            assert(0);
        }
    }

    if(!leave_temp)
        RemoveDirectoryW(ctx->tmp_dir);
    return hres;
}

static void update_counter(install_ctx_t *ctx, HWND hwnd)
{
    WCHAR text[100];

    if(--ctx->counter <= 0) {
        HWND button_hwnd;

        KillTimer(hwnd, ctx->timer);
        LoadStringW(urlmon_instance, IDS_AXINSTALL_INSTALL, text, sizeof(text)/sizeof(WCHAR));

        button_hwnd = GetDlgItem(hwnd, ID_AXINSTALL_INSTALL_BTN);
        EnableWindow(button_hwnd, TRUE);
    }else {
        WCHAR buf[100];
        LoadStringW(urlmon_instance, IDS_AXINSTALL_INSTALLN, buf, sizeof(buf)/sizeof(WCHAR));
        sprintfW(text, buf, ctx->counter);
    }

    SetDlgItemTextW(hwnd, ID_AXINSTALL_INSTALL_BTN, text);
}

static BOOL init_warning_dialog(HWND hwnd, install_ctx_t *ctx)
{
    BSTR display_uri;
    HRESULT hres;

    if(!SetPropW(hwnd, ctxW, ctx))
        return FALSE;

    hres = IUri_GetDisplayUri(ctx->uri, &display_uri);
    if(FAILED(hres))
        return FALSE;

    SetDlgItemTextW(hwnd, ID_AXINSTALL_LOCATION, display_uri);
    SysFreeString(display_uri);

    SendDlgItemMessageW(hwnd, ID_AXINSTALL_ICON, STM_SETICON,
            (WPARAM)LoadIconW(0, (const WCHAR*)OIC_WARNING), 0);

    ctx->counter = 4;
    update_counter(ctx, hwnd);
    ctx->timer = SetTimer(hwnd, 1, 1000, NULL);
    return TRUE;
}

static INT_PTR WINAPI warning_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch(msg) {
    case WM_INITDIALOG: {
        if(!init_warning_dialog(hwnd, (install_ctx_t*)lparam))
            EndDialog(hwnd, 0);
        return TRUE;
    }
    case WM_COMMAND:
        switch(wparam) {
        case ID_AXINSTALL_INSTALL_BTN: {
            install_ctx_t *ctx = GetPropW(hwnd, ctxW);
            if(ctx)
                ctx->cancel = FALSE;
            EndDialog(hwnd, 0);
            return FALSE;
        }
        case IDCANCEL:
            EndDialog(hwnd, 0);
            return FALSE;
        }
    case WM_TIMER:
        update_counter(GetPropW(hwnd, ctxW), hwnd);
        return TRUE;
    }

    return FALSE;
}

static BOOL install_warning(install_ctx_t *ctx)
{
    IWindowForBindingUI *window_iface;
    HWND parent_hwnd = NULL;
    HRESULT hres;

    if(!ctx->callback) {
        FIXME("no callback\n");
        return FALSE;
    }

    hres = IBindStatusCallback_QueryInterface(ctx->callback, &IID_IWindowForBindingUI, (void**)&window_iface);
    if(FAILED(hres))
        return FALSE;

    hres = IWindowForBindingUI_GetWindow(window_iface, &IID_ICodeInstall, &ctx->hwnd);
    IWindowForBindingUI_Release(window_iface);
    if(FAILED(hres))
        return FALSE;

    ctx->cancel = TRUE;
    DialogBoxParamW(urlmon_instance, MAKEINTRESOURCEW(ID_AXINSTALL_WARNING_DLG), parent_hwnd, warning_proc, (LPARAM)ctx);
    return !ctx->cancel;
}

static HRESULT install_file(install_ctx_t *ctx, const WCHAR *cache_file)
{
    BSTR path;
    HRESULT hres;

    TRACE("%s\n", debugstr_w(cache_file));

    ctx->cache_file = cache_file;

    if(!install_warning(ctx)) {
        TRACE("Installation cancelled\n");
        return S_OK;
    }

    hres = IUri_GetPath(ctx->uri, &path);
    if(SUCCEEDED(hres)) {
        const WCHAR *ptr, *ptr2, *ext;

        ptr = strrchrW(path, '/');
        if(!ptr)
            ptr = path;
        else
            ptr++;

        ptr2 = strrchrW(ptr, '\\');
        if(ptr2)
            ptr = ptr2+1;

        ctx->file_name = ptr;
        ext = strrchrW(ptr, '.');
        if(!ext)
            ext = ptr;

        if(!strcmpW(ext, cab_extW)) {
            hres = install_cab_file(ctx);
        }else {
            FIXME("Unsupported extension %s\n", debugstr_w(ext));
            hres = E_NOTIMPL;
        }
        SysFreeString(path);
    }

    return hres;
}

static void failure_msgbox(install_ctx_t *ctx, HRESULT hres)
{
    WCHAR buf[1024], fmt[1024];

    LoadStringW(urlmon_instance, IDS_AXINSTALL_FAILURE, fmt, sizeof(fmt)/sizeof(WCHAR));
    sprintfW(buf, fmt, hres);
    MessageBoxW(ctx->hwnd, buf, NULL, MB_OK);
}

static HRESULT distunit_on_stop(void *ctx, const WCHAR *cache_file, HRESULT hresult, const WCHAR *error_str)
{
    install_ctx_t *install_ctx = ctx;

    TRACE("(%p %s %08x %s)\n", ctx, debugstr_w(cache_file), hresult, debugstr_w(error_str));

    if(hresult == S_OK) {
        hresult = install_file(install_ctx, cache_file);
        if(FAILED(hresult))
            failure_msgbox(ctx, hresult);
    }

    if(install_ctx->callback)
        IBindStatusCallback_OnStopBinding(install_ctx->callback, hresult, error_str);

    if(install_ctx->release_on_stop)
        release_install_ctx(install_ctx);
    return S_OK;
}

/***********************************************************************
 *           AsyncInstallDistributionUnit (URLMON.@)
 */
HRESULT WINAPI AsyncInstallDistributionUnit(const WCHAR *szDistUnit, const WCHAR *szTYPE, const WCHAR *szExt,
        DWORD dwFileVersionMS, DWORD dwFileVersionLS, const WCHAR *szURL, IBindCtx *pbc, void *pvReserved, DWORD flags)
{
    install_ctx_t *ctx;
    HRESULT hres;

    TRACE("(%s %s %s %x %x %s %p %p %x)\n", debugstr_w(szDistUnit), debugstr_w(szTYPE), debugstr_w(szExt),
          dwFileVersionMS, dwFileVersionLS, debugstr_w(szURL), pbc, pvReserved, flags);

    if(szDistUnit || szTYPE || szExt)
        FIXME("Unsupported arguments\n");

    ctx = heap_alloc_zero(sizeof(*ctx));
    if(!ctx)
        return E_OUTOFMEMORY;

    hres = CreateUri(szURL, 0, 0, &ctx->uri);
    if(FAILED(hres)) {
        heap_free(ctx);
        return E_OUTOFMEMORY;
    }

    ctx->callback = bsc_from_bctx(pbc);

    hres = download_to_cache(ctx->uri, distunit_on_stop, ctx, ctx->callback);
    if(hres == MK_S_ASYNCHRONOUS)
        ctx->release_on_stop = TRUE;
    else
        release_install_ctx(ctx);

    return hres;
}
