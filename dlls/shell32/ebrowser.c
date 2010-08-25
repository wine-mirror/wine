/*
 * ExplorerBrowser Control implementation.
 *
 * Copyright 2010 David Hedberg
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

#include "winerror.h"
#include "windef.h"
#include "winbase.h"

#include "wine/list.h"
#include "wine/debug.h"
#include "debughlp.h"

#include "shell32_main.h"
#include "pidl.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

typedef struct _event_client {
    struct list entry;
    IExplorerBrowserEvents *pebe;
    DWORD cookie;
} event_client;

typedef struct _travellog_entry {
    struct list entry;
    LPITEMIDLIST pidl;
} travellog_entry;

typedef struct _ExplorerBrowserImpl {
    const IExplorerBrowserVtbl *lpVtbl;
    const IShellBrowserVtbl *lpsbVtbl;
    const ICommDlgBrowser3Vtbl *lpcdb3Vtbl;
    const IObjectWithSiteVtbl *lpowsVtbl;
    LONG ref;
    BOOL destroyed;

    HWND hwnd_main;
    HWND hwnd_sv;

    EXPLORER_BROWSER_OPTIONS eb_options;
    FOLDERSETTINGS fs;

    struct list event_clients;
    DWORD events_next_cookie;
    struct list travellog;
    travellog_entry *travellog_cursor;
    int travellog_count;

    IShellView *psv;
    RECT sv_rc;
    LPITEMIDLIST current_pidl;

    IUnknown *punk_site;
} ExplorerBrowserImpl;

/**************************************************************************
 * Event functions.
 */
static void events_unadvise_all(ExplorerBrowserImpl *This)
{
    event_client *client, *curs;
    TRACE("%p\n", This);

    LIST_FOR_EACH_ENTRY_SAFE(client, curs, &This->event_clients, event_client, entry)
    {
        TRACE("Removing %p\n", client);
        list_remove(&client->entry);
        IExplorerBrowserEvents_Release(client->pebe);
        HeapFree(GetProcessHeap(), 0, client);
    }
}

static HRESULT events_NavigationPending(ExplorerBrowserImpl *This, PCIDLIST_ABSOLUTE pidl)
{
    event_client *cursor;
    HRESULT hres = S_OK;

    TRACE("%p\n", This);

    LIST_FOR_EACH_ENTRY(cursor, &This->event_clients, event_client, entry)
    {
        TRACE("Notifying %p\n", cursor);
        hres = IExplorerBrowserEvents_OnNavigationPending(cursor->pebe, pidl);

        /* If this failed for any reason, the browsing is supposed to be aborted. */
        if(FAILED(hres))
            break;
    }

    return hres;
}

static void events_NavigationComplete(ExplorerBrowserImpl *This, PCIDLIST_ABSOLUTE pidl)
{
    event_client *cursor;

    TRACE("%p\n", This);

    LIST_FOR_EACH_ENTRY(cursor, &This->event_clients, event_client, entry)
    {
        TRACE("Notifying %p\n", cursor);
        IExplorerBrowserEvents_OnNavigationComplete(cursor->pebe, pidl);
    }
}

static void events_NavigationFailed(ExplorerBrowserImpl *This, PCIDLIST_ABSOLUTE pidl)
{
    event_client *cursor;

    TRACE("%p\n", This);

    LIST_FOR_EACH_ENTRY(cursor, &This->event_clients, event_client, entry)
    {
        TRACE("Notifying %p\n", cursor);
        IExplorerBrowserEvents_OnNavigationFailed(cursor->pebe, pidl);
    }
}

static void events_ViewCreated(ExplorerBrowserImpl *This, IShellView *psv)
{
    event_client *cursor;

    TRACE("%p\n", This);

    LIST_FOR_EACH_ENTRY(cursor, &This->event_clients, event_client, entry)
    {
        TRACE("Notifying %p\n", cursor);
        IExplorerBrowserEvents_OnViewCreated(cursor->pebe, psv);
    }
}

/**************************************************************************
 * Travellog functions.
 */
static void travellog_remove_entry(ExplorerBrowserImpl *This, travellog_entry *entry)
{
    TRACE("Removing %p\n", entry);

    list_remove(&entry->entry);
    HeapFree(GetProcessHeap(), 0, entry);
    This->travellog_count--;
}

static void travellog_remove_all_entries(ExplorerBrowserImpl *This)
{
    travellog_entry *cursor, *cursor2;
    TRACE("%p\n", This);

    LIST_FOR_EACH_ENTRY_SAFE(cursor, cursor2, &This->travellog, travellog_entry, entry)
        travellog_remove_entry(This, cursor);

    This->travellog_cursor = NULL;
}

static void travellog_add_entry(ExplorerBrowserImpl *This, LPITEMIDLIST pidl)
{
    travellog_entry *new, *cursor, *cursor2;
    TRACE("%p (old count %d)\n", pidl, This->travellog_count);

    /* Replace the old tail, if any, with the new entry */
    if(This->travellog_cursor)
    {
        LIST_FOR_EACH_ENTRY_SAFE_REV(cursor, cursor2, &This->travellog, travellog_entry, entry)
        {
            if(cursor == This->travellog_cursor)
                break;
            travellog_remove_entry(This, cursor);
        }
    }

    /* Create and add the new entry */
    new = HeapAlloc(GetProcessHeap(), 0, sizeof(travellog_entry));
    new->pidl = ILClone(pidl);
    list_add_tail(&This->travellog, &new->entry);
    This->travellog_cursor = new;
    This->travellog_count++;

    /* Remove the first few entries if the size limit is reached. */
    if(This->travellog_count > 200)
    {
        UINT i = 0;
        LIST_FOR_EACH_ENTRY_SAFE(cursor, cursor2, &This->travellog, travellog_entry, entry)
        {
            if(i++ > 10)
                break;
            travellog_remove_entry(This, cursor);
        }
    }
}

static LPCITEMIDLIST travellog_go_back(ExplorerBrowserImpl *This)
{
    travellog_entry *prev;
    TRACE("%p, %p\n", This, This->travellog_cursor);

    if(!This->travellog_cursor)
        return NULL;

    prev = LIST_ENTRY(list_prev(&This->travellog, &This->travellog_cursor->entry),
                      travellog_entry, entry);
    if(!prev)
        return NULL;

    This->travellog_cursor = prev;
    return prev->pidl;
}

static LPCITEMIDLIST travellog_go_forward(ExplorerBrowserImpl *This)
{
    travellog_entry *next;
    TRACE("%p, %p\n", This, This->travellog_cursor);

    if(!This->travellog_cursor)
        return NULL;

    next = LIST_ENTRY(list_next(&This->travellog, &This->travellog_cursor->entry),
                      travellog_entry, entry);
    if(!next)
        return NULL;

    This->travellog_cursor = next;
    return next->pidl;
}

/**************************************************************************
 * Helper functions
 */
static void update_layout(ExplorerBrowserImpl *This)
{
    RECT rc;
    TRACE("%p\n", This);

    GetClientRect(This->hwnd_main, &rc);
    CopyRect(&This->sv_rc, &rc);
}

static void size_panes(ExplorerBrowserImpl *This)
{
    MoveWindow(This->hwnd_sv,
               This->sv_rc.left, This->sv_rc.top,
               This->sv_rc.right - This->sv_rc.left, This->sv_rc.bottom - This->sv_rc.top,
               TRUE);
}

static HRESULT change_viewmode(ExplorerBrowserImpl *This, UINT viewmode)
{
    IFolderView *pfv;
    HRESULT hr;

    if(!This->psv)
        return E_FAIL;

    hr = IShellView_QueryInterface(This->psv, &IID_IFolderView, (void*)&pfv);
    if(SUCCEEDED(hr))
    {
        hr = IFolderView_SetCurrentViewMode(pfv, This->fs.ViewMode);
        IFolderView_Release(pfv);
    }

    return hr;
}

static HRESULT create_new_shellview(ExplorerBrowserImpl *This, IShellItem *psi)
{
    IShellBrowser *psb = (IShellBrowser*)&This->lpsbVtbl;
    IShellFolder *psf;
    IShellView *psv;
    HWND hwnd_new;
    HRESULT hr;

    TRACE("%p, %p\n", This, psi);

    hr = IShellItem_BindToHandler(psi, NULL, &BHID_SFObject, &IID_IShellFolder, (void**)&psf);
    if(SUCCEEDED(hr))
    {
        hr = IShellFolder_CreateViewObject(psf, This->hwnd_main, &IID_IShellView, (void**)&psv);
        if(SUCCEEDED(hr))
        {
            if(This->hwnd_sv)
            {
                IShellView_DestroyViewWindow(This->psv);
                This->hwnd_sv = NULL;
            }

            hr = IShellView_CreateViewWindow(psv, This->psv, &This->fs, psb, &This->sv_rc, &hwnd_new);
            if(SUCCEEDED(hr))
            {
                /* Replace the old shellview */
                if(This->psv) IShellView_Release(This->psv);

                This->psv = psv;
                This->hwnd_sv = hwnd_new;
                events_ViewCreated(This, psv);
            }
            else
            {
                ERR("CreateViewWindow failed (0x%x)\n", hr);
                IShellView_Release(psv);
            }
        }
        else
            ERR("CreateViewObject failed (0x%x)\n", hr);

        IShellFolder_Release(psf);
    }
    else
        ERR("SI::BindToHandler failed (0x%x)\n", hr);

    return hr;
}

static void get_interfaces_from_site(ExplorerBrowserImpl *This)
{
    IServiceProvider *psp;
    HRESULT hr;

    /* Calling this with This->punk_site set to NULL should properly
     * release any previously fetched interfaces.
     */

    if(This->punk_site)
    {
        hr = IUnknown_QueryInterface(This->punk_site, &IID_IServiceProvider, (void**)&psp);
        if(SUCCEEDED(hr))
        {
            FIXME("Not requesting any interfaces.\n");
            IServiceProvider_Release(psp);
        }
        else
            ERR("Failed to get IServiceProvider from site.\n");
    }
}

/**************************************************************************
 * Main window related functions.
 */
static LRESULT main_on_wm_create(HWND hWnd, CREATESTRUCTW *crs)
{
    ExplorerBrowserImpl *This = crs->lpCreateParams;
    TRACE("%p\n", This);

    SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LPARAM)This);
    This->hwnd_main = hWnd;

    return TRUE;
}

static LRESULT main_on_wm_size(ExplorerBrowserImpl *This)
{
    update_layout(This);
    size_panes(This);

    return TRUE;
}

static LRESULT CALLBACK main_wndproc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    ExplorerBrowserImpl *This = (ExplorerBrowserImpl*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);

    switch(uMessage)
    {
    case WM_CREATE:           return main_on_wm_create(hWnd, (CREATESTRUCTW*)lParam);
    case WM_SIZE:             return main_on_wm_size(This);
    default:                  return DefWindowProcW(hWnd, uMessage, wParam, lParam);
    }

    return 0;
}

/**************************************************************************
 * IExplorerBrowser Implementation
 */
static HRESULT WINAPI IExplorerBrowser_fnQueryInterface(IExplorerBrowser *iface,
                                                        REFIID riid, void **ppvObject)
{
    ExplorerBrowserImpl *This = (ExplorerBrowserImpl*)iface;
    TRACE("%p (%s, %p)\n", This, shdebugstr_guid(riid), ppvObject);

    *ppvObject = NULL;
    if(IsEqualIID(riid, &IID_IExplorerBrowser) ||
       IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObject = This;
    }
    else if(IsEqualIID(riid, &IID_IShellBrowser))
    {
        *ppvObject = &This->lpsbVtbl;
    }
    else if(IsEqualIID(riid, &IID_ICommDlgBrowser) ||
            IsEqualIID(riid, &IID_ICommDlgBrowser2) ||
            IsEqualIID(riid, &IID_ICommDlgBrowser3))
    {
        *ppvObject = &This->lpcdb3Vtbl;
    }
    else if(IsEqualIID(riid, &IID_IObjectWithSite))
    {
        *ppvObject = &This->lpowsVtbl;
    }

    if(*ppvObject)
    {
        IUnknown_AddRef((IUnknown*)*ppvObject);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI IExplorerBrowser_fnAddRef(IExplorerBrowser *iface)
{
    ExplorerBrowserImpl *This = (ExplorerBrowserImpl*)iface;
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("%p - ref %d\n", This, ref);

    return ref;
}

static ULONG WINAPI IExplorerBrowser_fnRelease(IExplorerBrowser *iface)
{
    ExplorerBrowserImpl *This = (ExplorerBrowserImpl*)iface;
    LONG ref = InterlockedDecrement(&This->ref);
    TRACE("%p - ref %d\n", This, ref);

    if(!ref)
    {
        TRACE("Freeing.\n");

        if(!This->destroyed)
            IExplorerBrowser_Destroy(iface);

        IObjectWithSite_SetSite((IObjectWithSite*)&This->lpowsVtbl, NULL);

        HeapFree(GetProcessHeap(), 0, This);
        return 0;
    }

    return ref;
}

static HRESULT WINAPI IExplorerBrowser_fnInitialize(IExplorerBrowser *iface,
                                                    HWND hwndParent, const RECT *prc,
                                                    const FOLDERSETTINGS *pfs)
{
    ExplorerBrowserImpl *This = (ExplorerBrowserImpl*)iface;
    WNDCLASSW wc;
    LONG style;
    static const WCHAR EB_CLASS_NAME[] =
        {'E','x','p','l','o','r','e','r','B','r','o','w','s','e','r','C','o','n','t','r','o','l',0};

    TRACE("%p (%p, %p, %p)\n", This, hwndParent, prc, pfs);

    if(This->hwnd_main)
        return E_UNEXPECTED;

    if(!hwndParent)
        return E_INVALIDARG;

    if( !GetClassInfoW(shell32_hInstance, EB_CLASS_NAME, &wc) )
    {
        wc.style            = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc      = main_wndproc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 0;
        wc.hInstance        = shell32_hInstance;
        wc.hIcon            = 0;
        wc.hCursor          = LoadCursorW(0, (LPWSTR)IDC_ARROW);
        wc.hbrBackground    = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszMenuName     = NULL;
        wc.lpszClassName    = EB_CLASS_NAME;

        if (!RegisterClassW(&wc)) return E_FAIL;
    }

    style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_BORDER;
    This->hwnd_main = CreateWindowExW(WS_EX_CONTROLPARENT, EB_CLASS_NAME, NULL, style,
                                      prc->left, prc->top,
                                      prc->right - prc->left, prc->bottom - prc->top,
                                      hwndParent, 0, shell32_hInstance, This);

    if(!This->hwnd_main)
    {
        ERR("Failed to create the window.\n");
        return E_FAIL;
    }

    This->fs.ViewMode = pfs ? pfs->ViewMode : FVM_DETAILS;
    This->fs.fFlags = pfs ? (pfs->fFlags | FWF_NOCLIENTEDGE) : FWF_NOCLIENTEDGE;

    return S_OK;
}

static HRESULT WINAPI IExplorerBrowser_fnDestroy(IExplorerBrowser *iface)
{
    ExplorerBrowserImpl *This = (ExplorerBrowserImpl*)iface;
    TRACE("%p\n", This);

    if(This->psv)
    {
        IShellView_DestroyViewWindow(This->psv);
        IShellView_Release(This->psv);
        This->psv = NULL;
        This->hwnd_sv = NULL;
    }

    events_unadvise_all(This);
    travellog_remove_all_entries(This);

    ILFree(This->current_pidl);
    This->current_pidl = NULL;

    DestroyWindow(This->hwnd_main);
    This->destroyed = TRUE;

    return S_OK;
}

static HRESULT WINAPI IExplorerBrowser_fnSetRect(IExplorerBrowser *iface,
                                                 HDWP *phdwp, RECT rcBrowser)
{
    ExplorerBrowserImpl *This = (ExplorerBrowserImpl*)iface;
    TRACE("%p (%p, %s)\n", This, phdwp, wine_dbgstr_rect(&rcBrowser));

    if(phdwp)
    {
        *phdwp = DeferWindowPos(*phdwp, This->hwnd_main, NULL, rcBrowser.left, rcBrowser.top,
                                rcBrowser.right - rcBrowser.left, rcBrowser.bottom - rcBrowser.top,
                                SWP_NOZORDER | SWP_NOACTIVATE);
    }
    else
    {
        MoveWindow(This->hwnd_main, rcBrowser.left, rcBrowser.top,
                   rcBrowser.right - rcBrowser.left, rcBrowser.bottom - rcBrowser.top, TRUE);
    }

    return S_OK;
}

static HRESULT WINAPI IExplorerBrowser_fnSetPropertyBag(IExplorerBrowser *iface,
                                                        LPCWSTR pszPropertyBag)
{
    ExplorerBrowserImpl *This = (ExplorerBrowserImpl*)iface;
    FIXME("stub, %p (%s)\n", This, debugstr_w(pszPropertyBag));

    return E_NOTIMPL;
}

static HRESULT WINAPI IExplorerBrowser_fnSetEmptyText(IExplorerBrowser *iface,
                                                      LPCWSTR pszEmptyText)
{
    ExplorerBrowserImpl *This = (ExplorerBrowserImpl*)iface;
    FIXME("stub, %p (%s)\n", This, debugstr_w(pszEmptyText));

    return E_NOTIMPL;
}

static HRESULT WINAPI IExplorerBrowser_fnSetFolderSettings(IExplorerBrowser *iface,
                                                           const FOLDERSETTINGS *pfs)
{
    ExplorerBrowserImpl *This = (ExplorerBrowserImpl*)iface;
    TRACE("%p (%p)\n", This, pfs);

    if(!pfs)
        return E_INVALIDARG;

    This->fs.ViewMode = pfs->ViewMode;
    This->fs.fFlags = pfs->fFlags | FWF_NOCLIENTEDGE;

    /* Change the settings of the current view, if any. */
    return change_viewmode(This, This->fs.ViewMode);
}

static HRESULT WINAPI IExplorerBrowser_fnAdvise(IExplorerBrowser *iface,
                                                IExplorerBrowserEvents *psbe,
                                                DWORD *pdwCookie)
{
    ExplorerBrowserImpl *This = (ExplorerBrowserImpl*)iface;
    event_client *client;
    TRACE("%p (%p, %p)\n", This, psbe, pdwCookie);

    client = HeapAlloc(GetProcessHeap(), 0, sizeof(event_client));
    client->pebe = psbe;
    client->cookie = ++This->events_next_cookie;

    IExplorerBrowserEvents_AddRef(psbe);
    *pdwCookie = client->cookie;

    list_add_tail(&This->event_clients, &client->entry);

    return S_OK;
}

static HRESULT WINAPI IExplorerBrowser_fnUnadvise(IExplorerBrowser *iface,
                                                  DWORD dwCookie)
{
    ExplorerBrowserImpl *This = (ExplorerBrowserImpl*)iface;
    event_client *client;
    TRACE("%p (0x%x)\n", This, dwCookie);

    LIST_FOR_EACH_ENTRY(client, &This->event_clients, event_client, entry)
    {
        if(client->cookie == dwCookie)
        {
            list_remove(&client->entry);
            IExplorerBrowserEvents_Release(client->pebe);
            HeapFree(GetProcessHeap(), 0, client);
            return S_OK;
        }
    }

    return E_INVALIDARG;
}

static HRESULT WINAPI IExplorerBrowser_fnSetOptions(IExplorerBrowser *iface,
                                                    EXPLORER_BROWSER_OPTIONS dwFlag)
{
    ExplorerBrowserImpl *This = (ExplorerBrowserImpl*)iface;
    static const EXPLORER_BROWSER_OPTIONS unsupported_options =
        EBO_SHOWFRAMES | EBO_ALWAYSNAVIGATE | EBO_NOWRAPPERWINDOW | EBO_HTMLSHAREPOINTVIEW;

    TRACE("%p (0x%x)\n", This, dwFlag);

    if(dwFlag & unsupported_options)
        FIXME("Flags 0x%08x contains unsupported options.\n", dwFlag);

    This->eb_options = dwFlag;

    return S_OK;
}

static HRESULT WINAPI IExplorerBrowser_fnGetOptions(IExplorerBrowser *iface,
                                                    EXPLORER_BROWSER_OPTIONS *pdwFlag)
{
    ExplorerBrowserImpl *This = (ExplorerBrowserImpl*)iface;
    TRACE("%p (%p)\n", This, pdwFlag);

    *pdwFlag = This->eb_options;

    return S_OK;
}

static HRESULT WINAPI IExplorerBrowser_fnBrowseToIDList(IExplorerBrowser *iface,
                                                        PCUIDLIST_RELATIVE pidl,
                                                        UINT uFlags)
{
    ExplorerBrowserImpl *This = (ExplorerBrowserImpl*)iface;
    LPITEMIDLIST absolute_pidl = NULL;
    HRESULT hr;
    static const UINT unsupported_browse_flags =
        SBSP_NEWBROWSER | EBF_SELECTFROMDATAOBJECT | EBF_NODROPTARGET;
    TRACE("%p (%p, 0x%x)\n", This, pidl, uFlags);

    if(!This->hwnd_main)
        return E_FAIL;

    if(This->destroyed)
        return HRESULT_FROM_WIN32(ERROR_BUSY);

    if(This->current_pidl && (This->eb_options & EBO_NAVIGATEONCE))
        return E_FAIL;

    if(uFlags & SBSP_EXPLOREMODE)
        return E_INVALIDARG;

    if(uFlags & unsupported_browse_flags)
        FIXME("Argument 0x%x contains unsupported flags.\n", uFlags);

    if(uFlags & SBSP_NAVIGATEBACK)
    {
        TRACE("SBSP_NAVIGATEBACK\n");
        absolute_pidl = ILClone(travellog_go_back(This));
        if(!absolute_pidl && !This->current_pidl)
            return E_FAIL;
        else if(!absolute_pidl)
            return S_OK;

    }
    else if(uFlags & SBSP_NAVIGATEFORWARD)
    {
        TRACE("SBSP_NAVIGATEFORWARD\n");
        absolute_pidl = ILClone(travellog_go_forward(This));
        if(!absolute_pidl && !This->current_pidl)
            return E_FAIL;
        else if(!absolute_pidl)
            return S_OK;

    }
    else if(uFlags & SBSP_PARENT)
    {
        if(This->current_pidl)
        {
            if(_ILIsPidlSimple(This->current_pidl))
            {
                absolute_pidl = _ILCreateDesktop();
            }
            else
            {
                absolute_pidl = ILClone(This->current_pidl);
                ILRemoveLastID(absolute_pidl);
            }
        }
        if(!absolute_pidl)
        {
            ERR("Failed to get parent pidl.\n");
            return E_FAIL;
        }

    }
    else if(uFlags & SBSP_RELATIVE)
    {
        /* SBSP_RELATIVE has precedence over SBSP_ABSOLUTE */
        TRACE("SBSP_RELATIVE\n");
        if(This->current_pidl)
        {
            absolute_pidl = ILCombine(This->current_pidl, pidl);
        }
        if(!absolute_pidl)
        {
            ERR("Failed to get absolute pidl.\n");
            return E_FAIL;
        }
    }
    else
    {
        TRACE("SBSP_ABSOLUTE\n");
        absolute_pidl = ILClone(pidl);
        if(!absolute_pidl && !This->current_pidl)
            return E_INVALIDARG;
        else if(!absolute_pidl)
            return S_OK;
    }

    /* TODO: Asynchronous browsing. Return S_OK here and finish in
     * another thread. */

    hr = events_NavigationPending(This, absolute_pidl);
    if(FAILED(hr))
    {
        TRACE("Browsing aborted.\n");
        ILFree(absolute_pidl);
        return E_FAIL;
    }

    get_interfaces_from_site(This);

    /* Only browse if the new pidl differs from the old */
    if(!ILIsEqual(This->current_pidl, absolute_pidl))
    {
        IShellItem *psi;
        hr = SHCreateItemFromIDList(absolute_pidl, &IID_IShellItem, (void**)&psi);
        if(SUCCEEDED(hr))
        {
            hr = create_new_shellview(This, psi);
            if(FAILED(hr))
            {
                events_NavigationFailed(This, absolute_pidl);
                ILFree(absolute_pidl);
                IShellItem_Release(psi);
                return E_FAIL;
            }

            /* Add to travellog */
            if( !(This->eb_options & EBO_NOTRAVELLOG) &&
                !(uFlags & (SBSP_NAVIGATEFORWARD|SBSP_NAVIGATEBACK)) )
            {
                travellog_add_entry(This, absolute_pidl);
            }

            IShellItem_Release(psi);
        }
    }

    events_NavigationComplete(This, absolute_pidl);
    ILFree(This->current_pidl);
    This->current_pidl = absolute_pidl;

    return S_OK;
}

static HRESULT WINAPI IExplorerBrowser_fnBrowseToObject(IExplorerBrowser *iface,
                                                        IUnknown *punk, UINT uFlags)
{
    ExplorerBrowserImpl *This = (ExplorerBrowserImpl*)iface;
    LPITEMIDLIST pidl;
    HRESULT hr;
    TRACE("%p (%p, 0x%x)\n", This, punk, uFlags);

    if(!punk)
        return IExplorerBrowser_fnBrowseToIDList(iface, NULL, uFlags);

    hr = SHGetIDListFromObject(punk, &pidl);
    if(SUCCEEDED(hr))
    {
        hr = IExplorerBrowser_BrowseToIDList(iface, pidl, uFlags);
        ILFree(pidl);
    }

    return hr;
}

static HRESULT WINAPI IExplorerBrowser_fnFillFromObject(IExplorerBrowser *iface,
                                                        IUnknown *punk,
                                                        EXPLORER_BROWSER_FILL_FLAGS dwFlags)
{
    ExplorerBrowserImpl *This = (ExplorerBrowserImpl*)iface;
    FIXME("stub, %p (%p, 0x%x)\n", This, punk, dwFlags);

    return E_NOTIMPL;
}

static HRESULT WINAPI IExplorerBrowser_fnRemoveAll(IExplorerBrowser *iface)
{
    ExplorerBrowserImpl *This = (ExplorerBrowserImpl*)iface;
    FIXME("stub, %p\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI IExplorerBrowser_fnGetCurrentView(IExplorerBrowser *iface,
                                                        REFIID riid, void **ppv)
{
    ExplorerBrowserImpl *This = (ExplorerBrowserImpl*)iface;
    TRACE("%p (%s, %p)\n", This, shdebugstr_guid(riid), ppv);

    if(!This->psv)
        return E_FAIL;

    return IShellView_QueryInterface(This->psv, riid, ppv);
}

static const IExplorerBrowserVtbl vt_IExplorerBrowser =
{
    IExplorerBrowser_fnQueryInterface,
    IExplorerBrowser_fnAddRef,
    IExplorerBrowser_fnRelease,
    IExplorerBrowser_fnInitialize,
    IExplorerBrowser_fnDestroy,
    IExplorerBrowser_fnSetRect,
    IExplorerBrowser_fnSetPropertyBag,
    IExplorerBrowser_fnSetEmptyText,
    IExplorerBrowser_fnSetFolderSettings,
    IExplorerBrowser_fnAdvise,
    IExplorerBrowser_fnUnadvise,
    IExplorerBrowser_fnSetOptions,
    IExplorerBrowser_fnGetOptions,
    IExplorerBrowser_fnBrowseToIDList,
    IExplorerBrowser_fnBrowseToObject,
    IExplorerBrowser_fnFillFromObject,
    IExplorerBrowser_fnRemoveAll,
    IExplorerBrowser_fnGetCurrentView
};

/**************************************************************************
 * IShellBrowser Implementation
 */

static inline ExplorerBrowserImpl *impl_from_IShellBrowser(IShellBrowser *iface)
{
    return (ExplorerBrowserImpl *)((char*)iface - FIELD_OFFSET(ExplorerBrowserImpl, lpsbVtbl));
}

static HRESULT WINAPI IShellBrowser_fnQueryInterface(IShellBrowser *iface,
                                                     REFIID riid, void **ppvObject)
{
    ExplorerBrowserImpl *This = impl_from_IShellBrowser(iface);
    TRACE("%p\n", This);
    return IUnknown_QueryInterface((IUnknown*) This, riid, ppvObject);
}

static ULONG WINAPI IShellBrowser_fnAddRef(IShellBrowser *iface)
{
    ExplorerBrowserImpl *This = impl_from_IShellBrowser(iface);
    TRACE("%p\n", This);
    return IUnknown_AddRef((IUnknown*) This);
}

static ULONG WINAPI IShellBrowser_fnRelease(IShellBrowser *iface)
{
    ExplorerBrowserImpl *This = impl_from_IShellBrowser(iface);
    TRACE("%p\n", This);
    return IUnknown_Release((IUnknown*) This);
}

static HRESULT WINAPI IShellBrowser_fnGetWindow(IShellBrowser *iface, HWND *phwnd)
{
    ExplorerBrowserImpl *This = impl_from_IShellBrowser(iface);
    TRACE("%p (%p)\n", This, phwnd);

    if(!This->hwnd_main)
        return E_FAIL;

    *phwnd = This->hwnd_main;
    return S_OK;
}

static HRESULT WINAPI IShellBrowser_fnContextSensitiveHelp(IShellBrowser *iface,
                                                           BOOL fEnterMode)
{
    ExplorerBrowserImpl *This = impl_from_IShellBrowser(iface);
    FIXME("stub, %p (%d)\n", This, fEnterMode);

    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowser_fnInsertMenusSB(IShellBrowser *iface,
                                                    HMENU hmenuShared,
                                                    LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    ExplorerBrowserImpl *This = impl_from_IShellBrowser(iface);
    TRACE("%p (%p, %p)\n", This, hmenuShared, lpMenuWidths);

    /* Not implemented. */
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowser_fnSetMenuSB(IShellBrowser *iface,
                                                HMENU hmenuShared,
                                                HOLEMENU holemenuReserved,
                                                HWND hwndActiveObject)
{
    ExplorerBrowserImpl *This = impl_from_IShellBrowser(iface);
    TRACE("%p (%p, %p, %p)\n", This, hmenuShared, holemenuReserved, hwndActiveObject);

    /* Not implemented. */
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowser_fnRemoveMenusSB(IShellBrowser *iface,
                                                    HMENU hmenuShared)
{
    ExplorerBrowserImpl *This = impl_from_IShellBrowser(iface);
    TRACE("%p (%p)\n", This, hmenuShared);

    /* Not implemented. */
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowser_fnSetStatusTextSB(IShellBrowser *iface,
                                                      LPCOLESTR pszStatusText)
{
    ExplorerBrowserImpl *This = impl_from_IShellBrowser(iface);
    FIXME("stub, %p (%s)\n", This, debugstr_w(pszStatusText));

    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowser_fnEnableModelessSB(IShellBrowser *iface,
                                                       BOOL fEnable)
{
    ExplorerBrowserImpl *This = impl_from_IShellBrowser(iface);
    FIXME("stub, %p (%d)\n", This, fEnable);

    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowser_fnTranslateAcceleratorSB(IShellBrowser *iface,
                                                             MSG *pmsg, WORD wID)
{
    ExplorerBrowserImpl *This = impl_from_IShellBrowser(iface);
    FIXME("stub, %p (%p, 0x%x)\n", This, pmsg, wID);

    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowser_fnBrowseObject(IShellBrowser *iface,
                                                   LPCITEMIDLIST pidl, UINT wFlags)
{
    ExplorerBrowserImpl *This = impl_from_IShellBrowser(iface);
    TRACE("%p (%p, %x)\n", This, pidl, wFlags);

    return IExplorerBrowser_fnBrowseToIDList((IExplorerBrowser*)This, pidl, wFlags);
}

static HRESULT WINAPI IShellBrowser_fnGetViewStateStream(IShellBrowser *iface,
                                                         DWORD grfMode,
                                                         IStream **ppStrm)
{
    ExplorerBrowserImpl *This = impl_from_IShellBrowser(iface);
    FIXME("stub, %p (0x%x, %p)\n", This, grfMode, ppStrm);

    *ppStrm = NULL;
    return E_FAIL;
}

static HRESULT WINAPI IShellBrowser_fnGetControlWindow(IShellBrowser *iface,
                                                       UINT id, HWND *phwnd)
{
    ExplorerBrowserImpl *This = impl_from_IShellBrowser(iface);
    TRACE("%p (%d, %p)\n", This, id, phwnd);

    /* Not implemented. */
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowser_fnSendControlMsg(IShellBrowser *iface,
                                                     UINT id, UINT uMsg,
                                                     WPARAM wParam, LPARAM lParam,
                                                     LRESULT *pret)
{
    ExplorerBrowserImpl *This = impl_from_IShellBrowser(iface);
    FIXME("stub, %p (%d, %d, %lx, %lx, %p)\n", This, id, uMsg, wParam, lParam, pret);

    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowser_fnQueryActiveShellView(IShellBrowser *iface,
                                                           IShellView **ppshv)
{
    ExplorerBrowserImpl *This = impl_from_IShellBrowser(iface);
    TRACE("%p (%p)\n", This, ppshv);

    if(!This->psv)
        return E_FAIL;

    *ppshv = This->psv;
    IShellView_AddRef(This->psv);

    return S_OK;
}

static HRESULT WINAPI IShellBrowser_fnOnViewWindowActive(IShellBrowser *iface,
                                                         IShellView *pshv)
{
    ExplorerBrowserImpl *This = impl_from_IShellBrowser(iface);
    FIXME("stub, %p (%p)\n", This, pshv);

    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowser_fnSetToolbarItems(IShellBrowser *iface,
                                                      LPTBBUTTONSB lpButtons,
                                                      UINT nButtons, UINT uFlags)
{
    ExplorerBrowserImpl *This = impl_from_IShellBrowser(iface);
    FIXME("stub, %p (%p, %d, 0x%x)\n", This, lpButtons, nButtons, uFlags);

    return E_NOTIMPL;
}

static const IShellBrowserVtbl vt_IShellBrowser = {
    IShellBrowser_fnQueryInterface,
    IShellBrowser_fnAddRef,
    IShellBrowser_fnRelease,
    IShellBrowser_fnGetWindow,
    IShellBrowser_fnContextSensitiveHelp,
    IShellBrowser_fnInsertMenusSB,
    IShellBrowser_fnSetMenuSB,
    IShellBrowser_fnRemoveMenusSB,
    IShellBrowser_fnSetStatusTextSB,
    IShellBrowser_fnEnableModelessSB,
    IShellBrowser_fnTranslateAcceleratorSB,
    IShellBrowser_fnBrowseObject,
    IShellBrowser_fnGetViewStateStream,
    IShellBrowser_fnGetControlWindow,
    IShellBrowser_fnSendControlMsg,
    IShellBrowser_fnQueryActiveShellView,
    IShellBrowser_fnOnViewWindowActive,
    IShellBrowser_fnSetToolbarItems
};

/**************************************************************************
 * ICommDlgBrowser3 Implementation
 */

static inline ExplorerBrowserImpl *impl_from_ICommDlgBrowser3(ICommDlgBrowser3 *iface)
{
    return (ExplorerBrowserImpl *)((char*)iface - FIELD_OFFSET(ExplorerBrowserImpl, lpcdb3Vtbl));
}

static HRESULT WINAPI ICommDlgBrowser3_fnQueryInterface(ICommDlgBrowser3 *iface,
                                                        REFIID riid,
                                                        void **ppvObject)
{
    ExplorerBrowserImpl *This = impl_from_ICommDlgBrowser3(iface);
    TRACE("%p\n", This);
    return IUnknown_QueryInterface((IUnknown*) This, riid, ppvObject);
}

static ULONG WINAPI ICommDlgBrowser3_fnAddRef(ICommDlgBrowser3 *iface)
{
    ExplorerBrowserImpl *This = impl_from_ICommDlgBrowser3(iface);
    TRACE("%p\n", This);
    return IUnknown_AddRef((IUnknown*) This);
}

static ULONG WINAPI ICommDlgBrowser3_fnRelease(ICommDlgBrowser3 *iface)
{
    ExplorerBrowserImpl *This = impl_from_ICommDlgBrowser3(iface);
    TRACE("%p\n", This);
    return IUnknown_Release((IUnknown*) This);
}

static HRESULT WINAPI ICommDlgBrowser3_fnOnDefaultCommand(ICommDlgBrowser3 *iface,
                                                          IShellView *shv)
{
    ExplorerBrowserImpl *This = impl_from_ICommDlgBrowser3(iface);
    IDataObject *pdo;
    HRESULT hr;
    HRESULT ret = S_FALSE;

    TRACE("%p (%p)\n", This, shv);

    hr = IShellView_GetItemObject(shv, SVGIO_SELECTION, &IID_IDataObject, (void**)&pdo);
    if(SUCCEEDED(hr))
    {
        FORMATETC fmt;
        STGMEDIUM medium;

        fmt.cfFormat = RegisterClipboardFormatW(CFSTR_SHELLIDLISTW);
        fmt.ptd = NULL;
        fmt.dwAspect = DVASPECT_CONTENT;
        fmt.lindex = -1;
        fmt.tymed = TYMED_HGLOBAL;

        hr = IDataObject_GetData(pdo, &fmt ,&medium);
        IDataObject_Release(pdo);
        if(SUCCEEDED(hr))
        {
            LPIDA pida = GlobalLock(medium.u.hGlobal);
            LPCITEMIDLIST pidl_child = (LPCITEMIDLIST) ((LPBYTE)pida+pida->aoffset[1]);

            /* Handle folders by browsing to them. */
            if(_ILIsFolder(pidl_child) || _ILIsDrive(pidl_child) || _ILIsSpecialFolder(pidl_child))
            {
                IExplorerBrowser_BrowseToIDList((IExplorerBrowser*)This, pidl_child, SBSP_RELATIVE);
                ret = S_OK;
            }
            GlobalUnlock(medium.u.hGlobal);
            GlobalFree(medium.u.hGlobal);
        }
        else
            ERR("Failed to get data from IDataObject.\n");
    } else
        ERR("Failed to get IDataObject.\n");

    return ret;
}
static HRESULT WINAPI ICommDlgBrowser3_fnOnStateChange(ICommDlgBrowser3 *iface,
                                                       IShellView *shv, ULONG uChange)
{
    ExplorerBrowserImpl *This = impl_from_ICommDlgBrowser3(iface);
    FIXME("stub, %p (%p, %d)\n", This, shv, uChange);
    return E_NOTIMPL;
}
static HRESULT WINAPI ICommDlgBrowser3_fnIncludeObject(ICommDlgBrowser3 *iface,
                                                       IShellView *pshv, LPCITEMIDLIST pidl)
{
    ExplorerBrowserImpl *This = impl_from_ICommDlgBrowser3(iface);
    FIXME("stub, %p (%p, %p)\n", This, pshv, pidl);
    return S_OK;
}

static HRESULT WINAPI ICommDlgBrowser3_fnNotify(ICommDlgBrowser3 *iface,
                                                IShellView *pshv,
                                                DWORD dwNotifyType)
{
    ExplorerBrowserImpl *This = impl_from_ICommDlgBrowser3(iface);
    FIXME("stub, %p (%p, 0x%x)\n", This, pshv, dwNotifyType);
    return S_OK;
}

static HRESULT WINAPI ICommDlgBrowser3_fnGetDefaultMenuText(ICommDlgBrowser3 *iface,
                                                            IShellView *pshv,
                                                            LPWSTR pszText, int cchMax)
{
    ExplorerBrowserImpl *This = impl_from_ICommDlgBrowser3(iface);
    FIXME("stub, %p (%p, %s, %d)\n", This, pshv, debugstr_w(pszText), cchMax);
    return S_OK;
}

static HRESULT WINAPI ICommDlgBrowser3_fnGetViewFlags(ICommDlgBrowser3 *iface,
                                                      DWORD *pdwFlags)
{
    ExplorerBrowserImpl *This = impl_from_ICommDlgBrowser3(iface);
    FIXME("stub, %p (%p)\n", This, pdwFlags);
    return S_OK;
}

static HRESULT WINAPI ICommDlgBrowser3_fnOnColumnClicked(ICommDlgBrowser3 *iface,
                                                         IShellView *pshv, int iColumn)
{
    ExplorerBrowserImpl *This = impl_from_ICommDlgBrowser3(iface);
    FIXME("stub, %p (%p, %d)\n", This, pshv, iColumn);
    return S_OK;
}

static HRESULT WINAPI ICommDlgBrowser3_fnGetCurrentFilter(ICommDlgBrowser3 *iface,
                                                          LPWSTR pszFileSpec,
                                                          int cchFileSpec)
{
    ExplorerBrowserImpl *This = impl_from_ICommDlgBrowser3(iface);
    FIXME("stub, %p (%s, %d)\n", This, debugstr_w(pszFileSpec), cchFileSpec);
    return S_OK;
}

static HRESULT WINAPI ICommDlgBrowser3_fnOnPreviewCreated(ICommDlgBrowser3 *iface,
                                                          IShellView *pshv)
{
    ExplorerBrowserImpl *This = impl_from_ICommDlgBrowser3(iface);
    FIXME("stub, %p (%p)\n", This, pshv);
    return S_OK;
}

static const ICommDlgBrowser3Vtbl vt_ICommDlgBrowser3 = {
    ICommDlgBrowser3_fnQueryInterface,
    ICommDlgBrowser3_fnAddRef,
    ICommDlgBrowser3_fnRelease,
    ICommDlgBrowser3_fnOnDefaultCommand,
    ICommDlgBrowser3_fnOnStateChange,
    ICommDlgBrowser3_fnIncludeObject,
    ICommDlgBrowser3_fnNotify,
    ICommDlgBrowser3_fnGetDefaultMenuText,
    ICommDlgBrowser3_fnGetViewFlags,
    ICommDlgBrowser3_fnOnColumnClicked,
    ICommDlgBrowser3_fnGetCurrentFilter,
    ICommDlgBrowser3_fnOnPreviewCreated
};

/**************************************************************************
 * IObjectWithSite Implementation
 */

static inline ExplorerBrowserImpl *impl_from_IObjectWithSite(IObjectWithSite *iface)
{
    return (ExplorerBrowserImpl *)((char*)iface - FIELD_OFFSET(ExplorerBrowserImpl, lpowsVtbl));
}

static HRESULT WINAPI IObjectWithSite_fnQueryInterface(IObjectWithSite *iface,
                                                       REFIID riid, void **ppvObject)
{
    ExplorerBrowserImpl *This = impl_from_IObjectWithSite(iface);
    TRACE("%p\n", This);
    return IUnknown_QueryInterface((IUnknown*)This, riid, ppvObject);
}

static ULONG WINAPI IObjectWithSite_fnAddRef(IObjectWithSite *iface)
{
    ExplorerBrowserImpl *This = impl_from_IObjectWithSite(iface);
    TRACE("%p\n", This);
    return IUnknown_AddRef((IUnknown*)This);
}

static ULONG WINAPI IObjectWithSite_fnRelease(IObjectWithSite *iface)
{
    ExplorerBrowserImpl *This = impl_from_IObjectWithSite(iface);
    TRACE("%p\n", This);
    return IUnknown_Release((IUnknown*)This);
}

static HRESULT WINAPI IObjectWithSite_fnSetSite(IObjectWithSite *iface, IUnknown *punk_site)
{
    ExplorerBrowserImpl *This = impl_from_IObjectWithSite(iface);
    TRACE("%p (%p)\n", This, punk_site);

    if(This->punk_site)
    {
        IUnknown_Release(This->punk_site);
        This->punk_site = NULL;
        get_interfaces_from_site(This);
    }

    This->punk_site = punk_site;

    if(This->punk_site)
        IUnknown_AddRef(This->punk_site);

    return S_OK;
}

static HRESULT WINAPI IObjectWithSite_fnGetSite(IObjectWithSite *iface, REFIID riid, void **ppvSite)
{
    ExplorerBrowserImpl *This = impl_from_IObjectWithSite(iface);
    TRACE("%p (%s, %p)\n", This, shdebugstr_guid(riid), ppvSite);

    if(!This->punk_site)
        return E_FAIL;

    return IUnknown_QueryInterface(This->punk_site, riid, ppvSite);
}

static const IObjectWithSiteVtbl vt_IObjectWithSite = {
    IObjectWithSite_fnQueryInterface,
    IObjectWithSite_fnAddRef,
    IObjectWithSite_fnRelease,
    IObjectWithSite_fnSetSite,
    IObjectWithSite_fnGetSite
};

HRESULT WINAPI ExplorerBrowser_Constructor(IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    ExplorerBrowserImpl *eb;
    HRESULT ret;

    TRACE("%p %s %p\n", pUnkOuter, shdebugstr_guid (riid), ppv);

    if(!ppv)
        return E_POINTER;
    if(pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    eb = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ExplorerBrowserImpl));
    eb->ref = 1;
    eb->lpVtbl = &vt_IExplorerBrowser;
    eb->lpsbVtbl = &vt_IShellBrowser;
    eb->lpcdb3Vtbl = &vt_ICommDlgBrowser3;
    eb->lpowsVtbl = &vt_IObjectWithSite;

    list_init(&eb->event_clients);
    list_init(&eb->travellog);

    ret = IExplorerBrowser_QueryInterface((IExplorerBrowser*)eb, riid, ppv);
    IExplorerBrowser_Release((IExplorerBrowser*)eb);

    TRACE("--(%p)\n", ppv);
    return ret;
}
