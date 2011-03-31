/*
 * Common Item Dialog
 *
 * Copyright 2010,2011 David Hedberg
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

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "winreg.h"
#include "shlwapi.h"

#include "commdlg.h"
#include "cdlg.h"
#include "filedlgbrowser.h"

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(commdlg);

enum ITEMDLG_TYPE {
    ITEMDLG_TYPE_OPEN,
    ITEMDLG_TYPE_SAVE
};

typedef struct {
    struct list entry;
    IFileDialogEvents *pfde;
    DWORD cookie;
} events_client;

typedef struct FileDialogImpl {
    IFileDialog2 IFileDialog2_iface;
    union {
        IFileOpenDialog IFileOpenDialog_iface;
        IFileSaveDialog IFileSaveDialog_iface;
    } u;
    enum ITEMDLG_TYPE dlg_type;
    LONG ref;

    FILEOPENDIALOGOPTIONS options;
    COMDLG_FILTERSPEC *filterspecs;
    UINT filterspec_count;
    UINT filetypeindex;

    struct list events_clients;
    DWORD events_next_cookie;

    IShellItemArray *psia_selection;
    IShellItemArray *psia_results;
    IShellItem *psi_defaultfolder;
    IShellItem *psi_setfolder;
    IShellItem *psi_folder;

    HWND dlg_hwnd;
} FileDialogImpl;

/**************************************************************************
 * Window related functions.
 */
static SIZE update_layout(FileDialogImpl *This)
{
    HDWP hdwp;
    HWND hwnd;
    RECT dialog_rc;
    RECT cancel_rc, open_rc;
    RECT filetype_rc, filename_rc, filenamelabel_rc;
    int missing_width, missing_height;
    static const UINT vspacing = 4, hspacing = 4;
    SIZE ret;

    GetClientRect(This->dlg_hwnd, &dialog_rc);

    missing_width = max(0, 320 - dialog_rc.right);
    missing_height = max(0, 200 - dialog_rc.bottom);

    if(missing_width || missing_height)
    {
        TRACE("Missing (%d, %d)\n", missing_width, missing_height);
        ret.cx = missing_width;
        ret.cy = missing_height;
        return ret;
    }

    /****
     * Calculate the size of the dialog and all the parts.
     */

    /* Cancel button */
    hwnd = GetDlgItem(This->dlg_hwnd, IDCANCEL);
    if(hwnd)
    {
        int cancel_width, cancel_height;
        GetWindowRect(hwnd, &cancel_rc);
        cancel_width = cancel_rc.right - cancel_rc.left;
        cancel_height = cancel_rc.bottom - cancel_rc.top;

        cancel_rc.left = dialog_rc.right - cancel_width - hspacing;
        cancel_rc.top = dialog_rc.bottom - cancel_height - vspacing;
        cancel_rc.right = cancel_rc.left + cancel_width;
        cancel_rc.bottom = cancel_rc.top + cancel_height;
    }

    /* Open/Save button */
    if(hwnd)
    {
        int open_width, open_height;
        GetWindowRect(hwnd, &open_rc);
        open_width = open_rc.right - open_rc.left;
        open_height = open_rc.bottom - open_rc.top;

        open_rc.left = cancel_rc.left - open_width - hspacing;
        open_rc.top = cancel_rc.top;
        open_rc.right = open_rc.left + open_width;
        open_rc.bottom = open_rc.top + open_height;
    }

    /* The filetype combobox. */
    hwnd = GetDlgItem(This->dlg_hwnd, IDC_FILETYPE);
    if(hwnd)
    {
        int filetype_width, filetype_height;
        GetWindowRect(hwnd, &filetype_rc);

        filetype_width = filetype_rc.right - filetype_rc.left;
        filetype_height = filetype_rc.bottom - filetype_rc.top;

        filetype_rc.right = cancel_rc.right;

        filetype_rc.left = filetype_rc.right - filetype_width;
        filetype_rc.top = cancel_rc.top - filetype_height - vspacing;
        filetype_rc.bottom = filetype_rc.top + filetype_height;

        if(!This->filterspec_count)
            filetype_rc.left = filetype_rc.right;
    }

    /* Filename label. */
    hwnd = GetDlgItem(This->dlg_hwnd, IDC_FILENAMESTATIC);
    if(hwnd)
    {
        int filetypelabel_width, filetypelabel_height;
        GetWindowRect(hwnd, &filenamelabel_rc);

        filetypelabel_width = filenamelabel_rc.right - filenamelabel_rc.left;
        filetypelabel_height = filenamelabel_rc.bottom - filenamelabel_rc.top;

        filenamelabel_rc.left = 160; /* FIXME */
        filenamelabel_rc.top = filetype_rc.top;
        filenamelabel_rc.right = filenamelabel_rc.left + filetypelabel_width;
        filenamelabel_rc.bottom = filenamelabel_rc.top + filetypelabel_height;
    }

    /* Filename edit box. */
    hwnd = GetDlgItem(This->dlg_hwnd, IDC_FILENAME);
    if(hwnd)
    {
        int filename_width, filename_height;
        GetWindowRect(hwnd, &filename_rc);

        filename_width = filetype_rc.left - filenamelabel_rc.right - hspacing*2;
        filename_height = filename_rc.bottom - filename_rc.top;

        filename_rc.left = filenamelabel_rc.right + hspacing;
        filename_rc.top = filetype_rc.top;
        filename_rc.right = filename_rc.left + filename_width;
        filename_rc.bottom = filename_rc.top + filename_height;
    }

    /****
     * Move everything to the right place.
     */

    /* FIXME: The Save Dialog uses a slightly different layout. */
    hdwp = BeginDeferWindowPos(5);

    /* The default controls */
    if(hdwp && (hwnd = GetDlgItem(This->dlg_hwnd, IDC_FILETYPE)) )
        DeferWindowPos(hdwp, hwnd, NULL, filetype_rc.left, filetype_rc.top, 0, 0,
                       SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

    if(hdwp && (hwnd = GetDlgItem(This->dlg_hwnd, IDC_FILENAME)) )
        DeferWindowPos(hdwp, hwnd, NULL, filename_rc.left, filename_rc.top,
                       filename_rc.right - filename_rc.left, filename_rc.bottom - filename_rc.top,
                       SWP_NOZORDER | SWP_NOACTIVATE);

    if(hdwp && (hwnd = GetDlgItem(This->dlg_hwnd, IDC_FILENAMESTATIC)) )
        DeferWindowPos(hdwp, hwnd, NULL, filenamelabel_rc.left, filenamelabel_rc.top, 0, 0,
                       SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

    if(hdwp && (hwnd = GetDlgItem(This->dlg_hwnd, IDOK)) )
        DeferWindowPos(hdwp, hwnd, NULL, open_rc.left, open_rc.top, 0, 0,
                       SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

    if(hdwp && (hwnd = GetDlgItem(This->dlg_hwnd, IDCANCEL)) )
        DeferWindowPos(hdwp, hwnd, NULL, cancel_rc.left, cancel_rc.top, 0, 0,
                       SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

    if(hdwp)
        EndDeferWindowPos(hdwp);
    else
        ERR("Failed to position dialog controls.\n");

    ret.cx = 0; ret.cy = 0;
    return ret;
}

static LRESULT on_wm_initdialog(HWND hwnd, LPARAM lParam)
{
    FileDialogImpl *This = (FileDialogImpl*)lParam;
    HWND hitem;

    TRACE("(%p, %p)\n", This, hwnd);

    SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LPARAM)This);
    This->dlg_hwnd = hwnd;

    hitem = GetDlgItem(This->dlg_hwnd, pshHelp);
    if(hitem) ShowWindow(hitem, SW_HIDE);

    hitem = GetDlgItem(This->dlg_hwnd, IDC_FILETYPESTATIC);
    if(hitem) ShowWindow(hitem, SW_HIDE);

    /* Fill filetypes combobox, or hide it. */
    hitem = GetDlgItem(This->dlg_hwnd, IDC_FILETYPE);
    if(This->filterspec_count)
    {
        UINT i;
        for(i = 0; i < This->filterspec_count; i++)
            SendMessageW(hitem, CB_ADDSTRING, 0, (LPARAM)This->filterspecs[i].pszName);

        SendMessageW(hitem, CB_SETCURSEL, This->filetypeindex, 0);
    }
    else
        ShowWindow(hitem, SW_HIDE);

    update_layout(This);

    return TRUE;
}

static LRESULT on_wm_size(FileDialogImpl *This)
{
    update_layout(This);
    return FALSE;
}

static LRESULT on_wm_getminmaxinfo(FileDialogImpl *This, LPARAM lparam)
{
    MINMAXINFO *mmi = (MINMAXINFO*)lparam;
    TRACE("%p (%p)\n", This, mmi);

    /* FIXME */
    mmi->ptMinTrackSize.x = 640;
    mmi->ptMinTrackSize.y = 480;

    return FALSE;
}

static LRESULT on_wm_destroy(FileDialogImpl *This)
{
    TRACE("%p\n", This);

    This->dlg_hwnd = NULL;

    return TRUE;
}

static LRESULT on_idok(FileDialogImpl *This)
{
    TRACE("%p\n", This);

    EndDialog(This->dlg_hwnd, S_OK);

    return FALSE;
}

static LRESULT on_idcancel(FileDialogImpl *This)
{
    TRACE("%p\n", This);

    EndDialog(This->dlg_hwnd, HRESULT_FROM_WIN32(ERROR_CANCELLED));

    return FALSE;
}

static LRESULT on_wm_command(FileDialogImpl *This, WPARAM wparam, LPARAM lparam)
{
    switch(wparam)
    {
    case IDOK:                return on_idok(This);
    case IDCANCEL:            return on_idcancel(This);
    default:                  TRACE("Unknown command.\n");
    }
    return FALSE;
}

static LRESULT CALLBACK itemdlg_dlgproc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam)
{
    FileDialogImpl *This = (FileDialogImpl*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch(umessage)
    {
    case WM_INITDIALOG:       return on_wm_initdialog(hwnd, lparam);
    case WM_COMMAND:          return on_wm_command(This, wparam, lparam);
    case WM_SIZE:             return on_wm_size(This);
    case WM_GETMINMAXINFO:    return on_wm_getminmaxinfo(This, lparam);
    case WM_DESTROY:          return on_wm_destroy(This);
    }

    return FALSE;
}

static HRESULT create_dialog(FileDialogImpl *This, HWND parent)
{
    INT_PTR res;

    SetLastError(0);
    res = DialogBoxParamW(COMDLG32_hInstance,
                          MAKEINTRESOURCEW(NEWFILEOPENV3ORD),
                          parent, itemdlg_dlgproc, (LPARAM)This);
    This->dlg_hwnd = NULL;
    if(res == -1)
    {
        ERR("Failed to show dialog (LastError: %d)\n", GetLastError());
        return E_FAIL;
    }

    TRACE("Returning 0x%08x\n", (HRESULT)res);
    return (HRESULT)res;
}

/**************************************************************************
 * IFileDialog implementation
 */
static inline FileDialogImpl *impl_from_IFileDialog2(IFileDialog2 *iface)
{
    return CONTAINING_RECORD(iface, FileDialogImpl, IFileDialog2_iface);
}

static HRESULT WINAPI IFileDialog2_fnQueryInterface(IFileDialog2 *iface,
                                                    REFIID riid,
                                                    void **ppvObject)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    TRACE("%p (%s, %p)\n", This, debugstr_guid(riid), ppvObject);

    *ppvObject = NULL;
    if(IsEqualGUID(riid, &IID_IUnknown) ||
       IsEqualGUID(riid, &IID_IFileDialog) ||
       IsEqualGUID(riid, &IID_IFileDialog2))
    {
        *ppvObject = iface;
    }
    else if(IsEqualGUID(riid, &IID_IFileOpenDialog) && This->dlg_type == ITEMDLG_TYPE_OPEN)
    {
        *ppvObject = &This->u.IFileOpenDialog_iface;
    }
    else if(IsEqualGUID(riid, &IID_IFileSaveDialog) && This->dlg_type == ITEMDLG_TYPE_SAVE)
    {
        *ppvObject = &This->u.IFileSaveDialog_iface;
    }
    else
        FIXME("Unknown interface requested: %s.\n", debugstr_guid(riid));

    if(*ppvObject)
    {
        IUnknown_AddRef((IUnknown*)*ppvObject);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI IFileDialog2_fnAddRef(IFileDialog2 *iface)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("%p - ref %d\n", This, ref);

    return ref;
}

static ULONG WINAPI IFileDialog2_fnRelease(IFileDialog2 *iface)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    LONG ref = InterlockedDecrement(&This->ref);
    TRACE("%p - ref %d\n", This, ref);

    if(!ref)
    {
        UINT i;
        for(i = 0; i < This->filterspec_count; i++)
        {
            LocalFree((void*)This->filterspecs[i].pszName);
            LocalFree((void*)This->filterspecs[i].pszSpec);
        }
        HeapFree(GetProcessHeap(), 0, This->filterspecs);

        if(This->psi_defaultfolder) IShellItem_Release(This->psi_defaultfolder);
        if(This->psi_setfolder)     IShellItem_Release(This->psi_setfolder);
        if(This->psi_folder)        IShellItem_Release(This->psi_folder);
        if(This->psia_selection)    IShellItemArray_Release(This->psia_selection);
        if(This->psia_results)      IShellItemArray_Release(This->psia_results);

        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI IFileDialog2_fnShow(IFileDialog2 *iface, HWND hwndOwner)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    TRACE("%p (%p)\n", iface, hwndOwner);

    return create_dialog(This, hwndOwner);
}

static HRESULT WINAPI IFileDialog2_fnSetFileTypes(IFileDialog2 *iface, UINT cFileTypes,
                                                  const COMDLG_FILTERSPEC *rgFilterSpec)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    UINT i;
    TRACE("%p (%d, %p)\n", This, cFileTypes, rgFilterSpec);

    if(This->filterspecs)
        return E_UNEXPECTED;

    if(!rgFilterSpec)
        return E_INVALIDARG;

    if(!cFileTypes)
        return S_OK;

    This->filterspecs = HeapAlloc(GetProcessHeap(), 0, sizeof(COMDLG_FILTERSPEC)*cFileTypes);
    for(i = 0; i < cFileTypes; i++)
    {
        This->filterspecs[i].pszName = StrDupW(rgFilterSpec[i].pszName);
        This->filterspecs[i].pszSpec = StrDupW(rgFilterSpec[i].pszSpec);
    }
    This->filterspec_count = cFileTypes;

    return S_OK;
}

static HRESULT WINAPI IFileDialog2_fnSetFileTypeIndex(IFileDialog2 *iface, UINT iFileType)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    TRACE("%p (%d)\n", This, iFileType);

    if(!This->filterspecs)
        return E_FAIL;

    if(iFileType >= This->filterspec_count)
        This->filetypeindex = This->filterspec_count - 1;
    else
        This->filetypeindex = iFileType;

    return S_OK;
}

static HRESULT WINAPI IFileDialog2_fnGetFileTypeIndex(IFileDialog2 *iface, UINT *piFileType)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    TRACE("%p (%p)\n", This, piFileType);

    if(!piFileType)
        return E_INVALIDARG;

    *piFileType = This->filetypeindex;

    return S_OK;
}

static HRESULT WINAPI IFileDialog2_fnAdvise(IFileDialog2 *iface, IFileDialogEvents *pfde, DWORD *pdwCookie)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    events_client *client;
    TRACE("%p (%p, %p)\n", This, pfde, pdwCookie);

    if(!pfde || !pdwCookie)
        return E_INVALIDARG;

    client = HeapAlloc(GetProcessHeap(), 0, sizeof(events_client));
    client->pfde = pfde;
    client->cookie = ++This->events_next_cookie;

    IFileDialogEvents_AddRef(pfde);
    *pdwCookie = client->cookie;

    list_add_tail(&This->events_clients, &client->entry);

    return S_OK;
}

static HRESULT WINAPI IFileDialog2_fnUnadvise(IFileDialog2 *iface, DWORD dwCookie)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    events_client *client, *found = NULL;
    TRACE("%p (%d)\n", This, dwCookie);

    LIST_FOR_EACH_ENTRY(client, &This->events_clients, events_client, entry)
    {
        if(client->cookie == dwCookie)
        {
            found = client;
            break;
        }
    }

    if(found)
    {
        list_remove(&found->entry);
        IFileDialogEvents_Release(found->pfde);
        HeapFree(GetProcessHeap(), 0, found);
        return S_OK;
    }

    return E_INVALIDARG;
}

static HRESULT WINAPI IFileDialog2_fnSetOptions(IFileDialog2 *iface, FILEOPENDIALOGOPTIONS fos)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    TRACE("%p (0x%x)\n", This, fos);

    This->options = fos;

    return S_OK;
}

static HRESULT WINAPI IFileDialog2_fnGetOptions(IFileDialog2 *iface, FILEOPENDIALOGOPTIONS *pfos)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    TRACE("%p (%p)\n", This, pfos);

    if(!pfos)
        return E_INVALIDARG;

    *pfos = This->options;

    return S_OK;
}

static HRESULT WINAPI IFileDialog2_fnSetDefaultFolder(IFileDialog2 *iface, IShellItem *psi)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    TRACE("%p (%p)\n", This, psi);
    if(This->psi_defaultfolder)
        IShellItem_Release(This->psi_defaultfolder);

    This->psi_defaultfolder = psi;

    if(This->psi_defaultfolder)
        IShellItem_AddRef(This->psi_defaultfolder);

    return S_OK;
}

static HRESULT WINAPI IFileDialog2_fnSetFolder(IFileDialog2 *iface, IShellItem *psi)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    TRACE("%p (%p)\n", This, psi);
    if(This->psi_setfolder)
        IShellItem_Release(This->psi_setfolder);

    This->psi_setfolder = psi;

    if(This->psi_setfolder)
        IShellItem_AddRef(This->psi_setfolder);

    return S_OK;
}

static HRESULT WINAPI IFileDialog2_fnGetFolder(IFileDialog2 *iface, IShellItem **ppsi)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    TRACE("%p (%p)\n", This, ppsi);
    if(!ppsi)
        return E_INVALIDARG;

    /* FIXME:
       If the dialog is shown, return the current(ly selected) folder. */

    *ppsi = NULL;
    if(This->psi_folder)
        *ppsi = This->psi_folder;
    else if(This->psi_setfolder)
        *ppsi = This->psi_setfolder;
    else if(This->psi_defaultfolder)
        *ppsi = This->psi_defaultfolder;

    if(*ppsi)
    {
        IShellItem_AddRef(*ppsi);
        return S_OK;
    }

    return E_FAIL;
}

static HRESULT WINAPI IFileDialog2_fnGetCurrentSelection(IFileDialog2 *iface, IShellItem **ppsi)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    HRESULT hr;
    TRACE("%p (%p)\n", This, ppsi);

    if(!ppsi)
        return E_INVALIDARG;

    if(This->psia_selection)
    {
        /* FIXME: Check filename edit box */
        hr = IShellItemArray_GetItemAt(This->psia_selection, 0, ppsi);
        return hr;
    }

    return E_FAIL;
}

static HRESULT WINAPI IFileDialog2_fnSetFileName(IFileDialog2 *iface, LPCWSTR pszName)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    FIXME("stub - %p (%p)\n", This, pszName);
    return E_NOTIMPL;
}

static HRESULT WINAPI IFileDialog2_fnGetFileName(IFileDialog2 *iface, LPWSTR *pszName)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    FIXME("stub - %p (%p)\n", This, pszName);
    return E_NOTIMPL;
}

static HRESULT WINAPI IFileDialog2_fnSetTitle(IFileDialog2 *iface, LPCWSTR pszTitle)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    FIXME("stub - %p (%p)\n", This, pszTitle);
    return E_NOTIMPL;
}

static HRESULT WINAPI IFileDialog2_fnSetOkButtonLabel(IFileDialog2 *iface, LPCWSTR pszText)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    FIXME("stub - %p (%p)\n", This, pszText);
    return E_NOTIMPL;
}

static HRESULT WINAPI IFileDialog2_fnSetFileNameLabel(IFileDialog2 *iface, LPCWSTR pszLabel)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    FIXME("stub - %p (%p)\n", This, pszLabel);
    return E_NOTIMPL;
}

static HRESULT WINAPI IFileDialog2_fnGetResult(IFileDialog2 *iface, IShellItem **ppsi)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    HRESULT hr;
    TRACE("%p (%p)\n", This, ppsi);

    if(!ppsi)
        return E_INVALIDARG;

    if(This->psia_results)
    {
        UINT item_count;
        hr = IShellItemArray_GetCount(This->psia_results, &item_count);
        if(SUCCEEDED(hr))
        {
            if(item_count != 1)
                return E_FAIL;

            /* Adds a reference. */
            hr = IShellItemArray_GetItemAt(This->psia_results, 0, ppsi);
        }

        return hr;
    }

    return E_UNEXPECTED;
}

static HRESULT WINAPI IFileDialog2_fnAddPlace(IFileDialog2 *iface, IShellItem *psi, FDAP fdap)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    FIXME("stub - %p (%p, %d)\n", This, psi, fdap);
    return E_NOTIMPL;
}

static HRESULT WINAPI IFileDialog2_fnSetDefaultExtension(IFileDialog2 *iface, LPCWSTR pszDefaultExtension)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    FIXME("stub - %p (%s)\n", This, debugstr_w(pszDefaultExtension));
    return E_NOTIMPL;
}

static HRESULT WINAPI IFileDialog2_fnClose(IFileDialog2 *iface, HRESULT hr)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    TRACE("%p (0x%08x)\n", This, hr);

    if(This->dlg_hwnd)
        EndDialog(This->dlg_hwnd, hr);

    return S_OK;
}

static HRESULT WINAPI IFileDialog2_fnSetClientGuid(IFileDialog2 *iface, REFGUID guid)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    FIXME("stub - %p (%s)\n", This, debugstr_guid(guid));
    return E_NOTIMPL;
}

static HRESULT WINAPI IFileDialog2_fnClearClientData(IFileDialog2 *iface)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    FIXME("stub - %p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI IFileDialog2_fnSetFilter(IFileDialog2 *iface, IShellItemFilter *pFilter)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    FIXME("stub - %p (%p)\n", This, pFilter);
    return E_NOTIMPL;
}

static HRESULT WINAPI IFileDialog2_fnSetCancelButtonLabel(IFileDialog2 *iface, LPCWSTR pszLabel)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    FIXME("stub - %p (%s)\n", This, debugstr_w(pszLabel));
    return E_NOTIMPL;
}

static HRESULT WINAPI IFileDialog2_fnSetNavigationRoot(IFileDialog2 *iface, IShellItem *psi)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    FIXME("stub - %p (%p)\n", This, psi);
    return E_NOTIMPL;
}

static const IFileDialog2Vtbl vt_IFileDialog2 = {
    IFileDialog2_fnQueryInterface,
    IFileDialog2_fnAddRef,
    IFileDialog2_fnRelease,
    IFileDialog2_fnShow,
    IFileDialog2_fnSetFileTypes,
    IFileDialog2_fnSetFileTypeIndex,
    IFileDialog2_fnGetFileTypeIndex,
    IFileDialog2_fnAdvise,
    IFileDialog2_fnUnadvise,
    IFileDialog2_fnSetOptions,
    IFileDialog2_fnGetOptions,
    IFileDialog2_fnSetDefaultFolder,
    IFileDialog2_fnSetFolder,
    IFileDialog2_fnGetFolder,
    IFileDialog2_fnGetCurrentSelection,
    IFileDialog2_fnSetFileName,
    IFileDialog2_fnGetFileName,
    IFileDialog2_fnSetTitle,
    IFileDialog2_fnSetOkButtonLabel,
    IFileDialog2_fnSetFileNameLabel,
    IFileDialog2_fnGetResult,
    IFileDialog2_fnAddPlace,
    IFileDialog2_fnSetDefaultExtension,
    IFileDialog2_fnClose,
    IFileDialog2_fnSetClientGuid,
    IFileDialog2_fnClearClientData,
    IFileDialog2_fnSetFilter,
    IFileDialog2_fnSetCancelButtonLabel,
    IFileDialog2_fnSetNavigationRoot
};

/**************************************************************************
 * IFileOpenDialog
 */
static inline FileDialogImpl *impl_from_IFileOpenDialog(IFileOpenDialog *iface)
{
    return CONTAINING_RECORD(iface, FileDialogImpl, u.IFileOpenDialog_iface);
}

static HRESULT WINAPI IFileOpenDialog_fnQueryInterface(IFileOpenDialog *iface,
                                                       REFIID riid, void **ppvObject)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_QueryInterface(&This->IFileDialog2_iface, riid, ppvObject);
}

static ULONG WINAPI IFileOpenDialog_fnAddRef(IFileOpenDialog *iface)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_AddRef(&This->IFileDialog2_iface);
}

static ULONG WINAPI IFileOpenDialog_fnRelease(IFileOpenDialog *iface)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_Release(&This->IFileDialog2_iface);
}

static HRESULT WINAPI IFileOpenDialog_fnShow(IFileOpenDialog *iface, HWND hwndOwner)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_Show(&This->IFileDialog2_iface, hwndOwner);
}

static HRESULT WINAPI IFileOpenDialog_fnSetFileTypes(IFileOpenDialog *iface, UINT cFileTypes,
                                                     const COMDLG_FILTERSPEC *rgFilterSpec)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_SetFileTypes(&This->IFileDialog2_iface, cFileTypes, rgFilterSpec);
}

static HRESULT WINAPI IFileOpenDialog_fnSetFileTypeIndex(IFileOpenDialog *iface, UINT iFileType)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_SetFileTypeIndex(&This->IFileDialog2_iface, iFileType);
}

static HRESULT WINAPI IFileOpenDialog_fnGetFileTypeIndex(IFileOpenDialog *iface, UINT *piFileType)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_GetFileTypeIndex(&This->IFileDialog2_iface, piFileType);
}

static HRESULT WINAPI IFileOpenDialog_fnAdvise(IFileOpenDialog *iface, IFileDialogEvents *pfde,
                                               DWORD *pdwCookie)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_Advise(&This->IFileDialog2_iface, pfde, pdwCookie);
}

static HRESULT WINAPI IFileOpenDialog_fnUnadvise(IFileOpenDialog *iface, DWORD dwCookie)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_Unadvise(&This->IFileDialog2_iface, dwCookie);
}

static HRESULT WINAPI IFileOpenDialog_fnSetOptions(IFileOpenDialog *iface, FILEOPENDIALOGOPTIONS fos)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_SetOptions(&This->IFileDialog2_iface, fos);
}

static HRESULT WINAPI IFileOpenDialog_fnGetOptions(IFileOpenDialog *iface, FILEOPENDIALOGOPTIONS *pfos)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_GetOptions(&This->IFileDialog2_iface, pfos);
}

static HRESULT WINAPI IFileOpenDialog_fnSetDefaultFolder(IFileOpenDialog *iface, IShellItem *psi)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_SetDefaultFolder(&This->IFileDialog2_iface, psi);
}

static HRESULT WINAPI IFileOpenDialog_fnSetFolder(IFileOpenDialog *iface, IShellItem *psi)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_SetFolder(&This->IFileDialog2_iface, psi);
}

static HRESULT WINAPI IFileOpenDialog_fnGetFolder(IFileOpenDialog *iface, IShellItem **ppsi)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_GetFolder(&This->IFileDialog2_iface, ppsi);
}

static HRESULT WINAPI IFileOpenDialog_fnGetCurrentSelection(IFileOpenDialog *iface, IShellItem **ppsi)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_GetCurrentSelection(&This->IFileDialog2_iface, ppsi);
}

static HRESULT WINAPI IFileOpenDialog_fnSetFileName(IFileOpenDialog *iface, LPCWSTR pszName)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_SetFileName(&This->IFileDialog2_iface, pszName);
}

static HRESULT WINAPI IFileOpenDialog_fnGetFileName(IFileOpenDialog *iface, LPWSTR *pszName)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_GetFileName(&This->IFileDialog2_iface, pszName);
}

static HRESULT WINAPI IFileOpenDialog_fnSetTitle(IFileOpenDialog *iface, LPCWSTR pszTitle)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_SetTitle(&This->IFileDialog2_iface, pszTitle);
}

static HRESULT WINAPI IFileOpenDialog_fnSetOkButtonLabel(IFileOpenDialog *iface, LPCWSTR pszText)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_SetOkButtonLabel(&This->IFileDialog2_iface, pszText);
}

static HRESULT WINAPI IFileOpenDialog_fnSetFileNameLabel(IFileOpenDialog *iface, LPCWSTR pszLabel)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_SetFileNameLabel(&This->IFileDialog2_iface, pszLabel);
}

static HRESULT WINAPI IFileOpenDialog_fnGetResult(IFileOpenDialog *iface, IShellItem **ppsi)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_GetResult(&This->IFileDialog2_iface, ppsi);
}

static HRESULT WINAPI IFileOpenDialog_fnAddPlace(IFileOpenDialog *iface, IShellItem *psi, FDAP fdap)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_AddPlace(&This->IFileDialog2_iface, psi, fdap);
}

static HRESULT WINAPI IFileOpenDialog_fnSetDefaultExtension(IFileOpenDialog *iface,
                                                            LPCWSTR pszDefaultExtension)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_SetDefaultExtension(&This->IFileDialog2_iface, pszDefaultExtension);
}

static HRESULT WINAPI IFileOpenDialog_fnClose(IFileOpenDialog *iface, HRESULT hr)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_Close(&This->IFileDialog2_iface, hr);
}

static HRESULT WINAPI IFileOpenDialog_fnSetClientGuid(IFileOpenDialog *iface, REFGUID guid)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_SetClientGuid(&This->IFileDialog2_iface, guid);
}

static HRESULT WINAPI IFileOpenDialog_fnClearClientData(IFileOpenDialog *iface)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_ClearClientData(&This->IFileDialog2_iface);
}

static HRESULT WINAPI IFileOpenDialog_fnSetFilter(IFileOpenDialog *iface, IShellItemFilter *pFilter)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_SetFilter(&This->IFileDialog2_iface, pFilter);
}

static HRESULT WINAPI IFileOpenDialog_fnGetResults(IFileOpenDialog *iface, IShellItemArray **ppenum)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    TRACE("%p (%p)\n", This, ppenum);

    *ppenum = This->psia_results;

    if(*ppenum)
    {
        IShellItemArray_AddRef(*ppenum);
        return S_OK;
    }

    return E_FAIL;
}

static HRESULT WINAPI IFileOpenDialog_fnGetSelectedItems(IFileOpenDialog *iface, IShellItemArray **ppsai)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    TRACE("%p (%p)\n", This, ppsai);

    if(This->psia_selection)
    {
        *ppsai = This->psia_selection;
        IShellItemArray_AddRef(*ppsai);
        return S_OK;
    }

    return E_FAIL;
}

static const IFileOpenDialogVtbl vt_IFileOpenDialog = {
    IFileOpenDialog_fnQueryInterface,
    IFileOpenDialog_fnAddRef,
    IFileOpenDialog_fnRelease,
    IFileOpenDialog_fnShow,
    IFileOpenDialog_fnSetFileTypes,
    IFileOpenDialog_fnSetFileTypeIndex,
    IFileOpenDialog_fnGetFileTypeIndex,
    IFileOpenDialog_fnAdvise,
    IFileOpenDialog_fnUnadvise,
    IFileOpenDialog_fnSetOptions,
    IFileOpenDialog_fnGetOptions,
    IFileOpenDialog_fnSetDefaultFolder,
    IFileOpenDialog_fnSetFolder,
    IFileOpenDialog_fnGetFolder,
    IFileOpenDialog_fnGetCurrentSelection,
    IFileOpenDialog_fnSetFileName,
    IFileOpenDialog_fnGetFileName,
    IFileOpenDialog_fnSetTitle,
    IFileOpenDialog_fnSetOkButtonLabel,
    IFileOpenDialog_fnSetFileNameLabel,
    IFileOpenDialog_fnGetResult,
    IFileOpenDialog_fnAddPlace,
    IFileOpenDialog_fnSetDefaultExtension,
    IFileOpenDialog_fnClose,
    IFileOpenDialog_fnSetClientGuid,
    IFileOpenDialog_fnClearClientData,
    IFileOpenDialog_fnSetFilter,
    IFileOpenDialog_fnGetResults,
    IFileOpenDialog_fnGetSelectedItems
};

/**************************************************************************
 * IFileSaveDialog
 */
static inline FileDialogImpl *impl_from_IFileSaveDialog(IFileSaveDialog *iface)
{
    return CONTAINING_RECORD(iface, FileDialogImpl, u.IFileSaveDialog_iface);
}

static HRESULT WINAPI IFileSaveDialog_fnQueryInterface(IFileSaveDialog *iface,
                                                       REFIID riid,
                                                       void **ppvObject)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_QueryInterface(&This->IFileDialog2_iface, riid, ppvObject);
}

static ULONG WINAPI IFileSaveDialog_fnAddRef(IFileSaveDialog *iface)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_AddRef(&This->IFileDialog2_iface);
}

static ULONG WINAPI IFileSaveDialog_fnRelease(IFileSaveDialog *iface)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_Release(&This->IFileDialog2_iface);
}

static HRESULT WINAPI IFileSaveDialog_fnShow(IFileSaveDialog *iface, HWND hwndOwner)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_Show(&This->IFileDialog2_iface, hwndOwner);
}

static HRESULT WINAPI IFileSaveDialog_fnSetFileTypes(IFileSaveDialog *iface, UINT cFileTypes,
                                                     const COMDLG_FILTERSPEC *rgFilterSpec)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_SetFileTypes(&This->IFileDialog2_iface, cFileTypes, rgFilterSpec);
}

static HRESULT WINAPI IFileSaveDialog_fnSetFileTypeIndex(IFileSaveDialog *iface, UINT iFileType)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_SetFileTypeIndex(&This->IFileDialog2_iface, iFileType);
}

static HRESULT WINAPI IFileSaveDialog_fnGetFileTypeIndex(IFileSaveDialog *iface, UINT *piFileType)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_GetFileTypeIndex(&This->IFileDialog2_iface, piFileType);
}

static HRESULT WINAPI IFileSaveDialog_fnAdvise(IFileSaveDialog *iface, IFileDialogEvents *pfde,
                                               DWORD *pdwCookie)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_Advise(&This->IFileDialog2_iface, pfde, pdwCookie);
}

static HRESULT WINAPI IFileSaveDialog_fnUnadvise(IFileSaveDialog *iface, DWORD dwCookie)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_Unadvise(&This->IFileDialog2_iface, dwCookie);
}

static HRESULT WINAPI IFileSaveDialog_fnSetOptions(IFileSaveDialog *iface, FILEOPENDIALOGOPTIONS fos)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_SetOptions(&This->IFileDialog2_iface, fos);
}

static HRESULT WINAPI IFileSaveDialog_fnGetOptions(IFileSaveDialog *iface, FILEOPENDIALOGOPTIONS *pfos)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_GetOptions(&This->IFileDialog2_iface, pfos);
}

static HRESULT WINAPI IFileSaveDialog_fnSetDefaultFolder(IFileSaveDialog *iface, IShellItem *psi)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_SetDefaultFolder(&This->IFileDialog2_iface, psi);
}

static HRESULT WINAPI IFileSaveDialog_fnSetFolder(IFileSaveDialog *iface, IShellItem *psi)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_SetFolder(&This->IFileDialog2_iface, psi);
}

static HRESULT WINAPI IFileSaveDialog_fnGetFolder(IFileSaveDialog *iface, IShellItem **ppsi)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_GetFolder(&This->IFileDialog2_iface, ppsi);
}

static HRESULT WINAPI IFileSaveDialog_fnGetCurrentSelection(IFileSaveDialog *iface, IShellItem **ppsi)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_GetCurrentSelection(&This->IFileDialog2_iface, ppsi);
}

static HRESULT WINAPI IFileSaveDialog_fnSetFileName(IFileSaveDialog *iface, LPCWSTR pszName)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_SetFileName(&This->IFileDialog2_iface, pszName);
}

static HRESULT WINAPI IFileSaveDialog_fnGetFileName(IFileSaveDialog *iface, LPWSTR *pszName)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_GetFileName(&This->IFileDialog2_iface, pszName);
}

static HRESULT WINAPI IFileSaveDialog_fnSetTitle(IFileSaveDialog *iface, LPCWSTR pszTitle)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_SetTitle(&This->IFileDialog2_iface, pszTitle);
}

static HRESULT WINAPI IFileSaveDialog_fnSetOkButtonLabel(IFileSaveDialog *iface, LPCWSTR pszText)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_SetOkButtonLabel(&This->IFileDialog2_iface, pszText);
}

static HRESULT WINAPI IFileSaveDialog_fnSetFileNameLabel(IFileSaveDialog *iface, LPCWSTR pszLabel)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_SetFileNameLabel(&This->IFileDialog2_iface, pszLabel);
}

static HRESULT WINAPI IFileSaveDialog_fnGetResult(IFileSaveDialog *iface, IShellItem **ppsi)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_GetResult(&This->IFileDialog2_iface, ppsi);
}

static HRESULT WINAPI IFileSaveDialog_fnAddPlace(IFileSaveDialog *iface, IShellItem *psi, FDAP fdap)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_AddPlace(&This->IFileDialog2_iface, psi, fdap);
}

static HRESULT WINAPI IFileSaveDialog_fnSetDefaultExtension(IFileSaveDialog *iface,
                                                            LPCWSTR pszDefaultExtension)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_SetDefaultExtension(&This->IFileDialog2_iface, pszDefaultExtension);
}

static HRESULT WINAPI IFileSaveDialog_fnClose(IFileSaveDialog *iface, HRESULT hr)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_Close(&This->IFileDialog2_iface, hr);
}

static HRESULT WINAPI IFileSaveDialog_fnSetClientGuid(IFileSaveDialog *iface, REFGUID guid)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_SetClientGuid(&This->IFileDialog2_iface, guid);
}

static HRESULT WINAPI IFileSaveDialog_fnClearClientData(IFileSaveDialog *iface)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_ClearClientData(&This->IFileDialog2_iface);
}

static HRESULT WINAPI IFileSaveDialog_fnSetFilter(IFileSaveDialog *iface, IShellItemFilter *pFilter)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_SetFilter(&This->IFileDialog2_iface, pFilter);
}

static HRESULT WINAPI IFileSaveDialog_fnSetSaveAsItem(IFileSaveDialog* iface, IShellItem *psi)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    FIXME("stub - %p (%p)\n", This, psi);
    return E_NOTIMPL;
}

static HRESULT WINAPI IFileSaveDialog_fnSetProperties(IFileSaveDialog* iface, IPropertyStore *pStore)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    FIXME("stub - %p (%p)\n", This, pStore);
    return E_NOTIMPL;
}

static HRESULT WINAPI IFileSaveDialog_fnSetCollectedProperties(IFileSaveDialog* iface,
                                                               IPropertyDescriptionList *pList,
                                                               BOOL fAppendDefault)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    FIXME("stub - %p (%p, %d)\n", This, pList, fAppendDefault);
    return E_NOTIMPL;
}

static HRESULT WINAPI IFileSaveDialog_fnGetProperties(IFileSaveDialog* iface, IPropertyStore **ppStore)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    FIXME("stub - %p (%p)\n", This, ppStore);
    return E_NOTIMPL;
}

static HRESULT WINAPI IFileSaveDialog_fnApplyProperties(IFileSaveDialog* iface,
                                                        IShellItem *psi,
                                                        IPropertyStore *pStore,
                                                        HWND hwnd,
                                                        IFileOperationProgressSink *pSink)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    FIXME("%p (%p, %p, %p, %p)\n", This, psi, pStore, hwnd, pSink);
    return E_NOTIMPL;
}

static const IFileSaveDialogVtbl vt_IFileSaveDialog = {
    IFileSaveDialog_fnQueryInterface,
    IFileSaveDialog_fnAddRef,
    IFileSaveDialog_fnRelease,
    IFileSaveDialog_fnShow,
    IFileSaveDialog_fnSetFileTypes,
    IFileSaveDialog_fnSetFileTypeIndex,
    IFileSaveDialog_fnGetFileTypeIndex,
    IFileSaveDialog_fnAdvise,
    IFileSaveDialog_fnUnadvise,
    IFileSaveDialog_fnSetOptions,
    IFileSaveDialog_fnGetOptions,
    IFileSaveDialog_fnSetDefaultFolder,
    IFileSaveDialog_fnSetFolder,
    IFileSaveDialog_fnGetFolder,
    IFileSaveDialog_fnGetCurrentSelection,
    IFileSaveDialog_fnSetFileName,
    IFileSaveDialog_fnGetFileName,
    IFileSaveDialog_fnSetTitle,
    IFileSaveDialog_fnSetOkButtonLabel,
    IFileSaveDialog_fnSetFileNameLabel,
    IFileSaveDialog_fnGetResult,
    IFileSaveDialog_fnAddPlace,
    IFileSaveDialog_fnSetDefaultExtension,
    IFileSaveDialog_fnClose,
    IFileSaveDialog_fnSetClientGuid,
    IFileSaveDialog_fnClearClientData,
    IFileSaveDialog_fnSetFilter,
    IFileSaveDialog_fnSetSaveAsItem,
    IFileSaveDialog_fnSetProperties,
    IFileSaveDialog_fnSetCollectedProperties,
    IFileSaveDialog_fnGetProperties,
    IFileSaveDialog_fnApplyProperties
};

static HRESULT FileDialog_constructor(IUnknown *pUnkOuter, REFIID riid, void **ppv, enum ITEMDLG_TYPE type)
{
    FileDialogImpl *fdimpl;
    HRESULT hr;
    IShellFolder *psf;
    TRACE("%p, %s, %p\n", pUnkOuter, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    if(pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    fdimpl = HeapAlloc(GetProcessHeap(), 0, sizeof(FileDialogImpl));
    if(!fdimpl)
        return E_OUTOFMEMORY;

    fdimpl->ref = 1;
    fdimpl->IFileDialog2_iface.lpVtbl = &vt_IFileDialog2;

    if(type == ITEMDLG_TYPE_OPEN)
    {
        fdimpl->dlg_type = ITEMDLG_TYPE_OPEN;
        fdimpl->u.IFileOpenDialog_iface.lpVtbl = &vt_IFileOpenDialog;
        fdimpl->options = FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST | FOS_NOCHANGEDIR;
    }
    else
    {
        fdimpl->dlg_type = ITEMDLG_TYPE_SAVE;
        fdimpl->u.IFileSaveDialog_iface.lpVtbl = &vt_IFileSaveDialog;
        fdimpl->options = FOS_OVERWRITEPROMPT | FOS_NOREADONLYRETURN | FOS_PATHMUSTEXIST | FOS_NOCHANGEDIR;
    }

    fdimpl->filterspecs = NULL;
    fdimpl->filterspec_count = 0;
    fdimpl->filetypeindex = 0;

    fdimpl->psia_selection = fdimpl->psia_results = NULL;
    fdimpl->psi_setfolder = fdimpl->psi_folder = NULL;

    list_init(&fdimpl->events_clients);
    fdimpl->events_next_cookie = 0;

    fdimpl->dlg_hwnd = NULL;

    /* FIXME: The default folder setting should be restored for the
     * application if it was previously set. */
    SHGetDesktopFolder(&psf);
    SHGetItemFromObject((IUnknown*)psf, &IID_IShellItem, (void**)&fdimpl->psi_defaultfolder);
    IShellFolder_Release(psf);

    hr = IUnknown_QueryInterface((IUnknown*)fdimpl, riid, ppv);
    IUnknown_Release((IUnknown*)fdimpl);
    return hr;
}

HRESULT FileOpenDialog_Constructor(IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    return FileDialog_constructor(pUnkOuter, riid, ppv, ITEMDLG_TYPE_OPEN);
}

HRESULT FileSaveDialog_Constructor(IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    return FileDialog_constructor(pUnkOuter, riid, ppv, ITEMDLG_TYPE_SAVE);
}
