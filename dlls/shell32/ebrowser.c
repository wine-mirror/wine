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
#include "winerror.h"
#include "windef.h"
#include "winbase.h"

#include "wine/list.h"
#include "wine/debug.h"
#include "debughlp.h"

#include "shell32_main.h"
#include "pidl.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#define SPLITTER_WIDTH 2
#define NP_MIN_WIDTH 60
#define NP_DEFAULT_WIDTH 150
#define SV_MIN_WIDTH 150

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
    IExplorerBrowser  IExplorerBrowser_iface;
    IShellBrowser     IShellBrowser_iface;
    ICommDlgBrowser3  ICommDlgBrowser3_iface;
    IObjectWithSite   IObjectWithSite_iface;
    INameSpaceTreeControlEvents INameSpaceTreeControlEvents_iface;
    IInputObject      IInputObject_iface;
    LONG ref;
    BOOL destroyed;

    HWND hwnd_main;
    HWND hwnd_sv;

    RECT splitter_rc;
    struct {
        INameSpaceTreeControl2 *pnstc2;
        HWND hwnd_splitter, hwnd_nstc;
        DWORD nstc_cookie;
        UINT width;
        BOOL show;
        RECT rc;
    } navpane;

    EXPLORER_BROWSER_OPTIONS eb_options;
    FOLDERSETTINGS fs;

    struct list event_clients;
    DWORD events_next_cookie;
    struct list travellog;
    travellog_entry *travellog_cursor;
    int travellog_count;

    int dpix;

    IShellView *psv;
    RECT sv_rc;
    LPITEMIDLIST current_pidl;

    IUnknown *punk_site;
    ICommDlgBrowser *pcdb_site;
    ICommDlgBrowser2 *pcdb2_site;
    ICommDlgBrowser3 *pcdb3_site;
    IExplorerPaneVisibility *pepv_site;
} ExplorerBrowserImpl;

static void initialize_navpane(ExplorerBrowserImpl *This, HWND hwnd_parent, RECT *rc);

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
        free(client);
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
    ILFree(entry->pidl);
    free(entry);
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
    new = malloc(sizeof(*new));
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
    INT navpane_width_actual;
    INT shellview_width_actual;
    int np_min_width = MulDiv(NP_MIN_WIDTH, This->dpix, USER_DEFAULT_SCREEN_DPI);
    int sv_min_width = MulDiv(SV_MIN_WIDTH, This->dpix, USER_DEFAULT_SCREEN_DPI);
    TRACE("%p (navpane: %d, EBO_SHOWFRAMES: %d)\n",
          This, This->navpane.show, This->eb_options & EBO_SHOWFRAMES);

    GetClientRect(This->hwnd_main, &rc);

    if((This->eb_options & EBO_SHOWFRAMES) && This->navpane.show)
        navpane_width_actual = This->navpane.width;
    else
        navpane_width_actual = 0;

    shellview_width_actual = rc.right - navpane_width_actual;
    if(shellview_width_actual < sv_min_width && navpane_width_actual)
    {
        INT missing_width = sv_min_width - shellview_width_actual;
        if(missing_width < (navpane_width_actual - np_min_width))
        {
            /* Shrink the navpane */
            navpane_width_actual -= missing_width;
            shellview_width_actual += missing_width;
        }
        else
        {
            /* Hide the navpane */
            shellview_width_actual += navpane_width_actual;
            navpane_width_actual = 0;
        }
    }

    /**************************************************************
     *  Calculate rectangles for the panes. All rectangles contain
     *  the position of the panes relative to hwnd_main.
     */

    if(navpane_width_actual)
    {
        SetRect(&This->navpane.rc, 0, 0, navpane_width_actual, rc.bottom);

        if(!This->navpane.hwnd_splitter)
            initialize_navpane(This, This->hwnd_main, &This->navpane.rc);
    }
    else
        ZeroMemory(&This->navpane.rc, sizeof(RECT));

    SetRect(&This->sv_rc, navpane_width_actual, 0, navpane_width_actual + shellview_width_actual,
            rc.bottom);
}

static void size_panes(ExplorerBrowserImpl *This)
{
    int splitter_width = MulDiv(SPLITTER_WIDTH, This->dpix, USER_DEFAULT_SCREEN_DPI);
    MoveWindow(This->navpane.hwnd_splitter,
               This->navpane.rc.right - splitter_width, This->navpane.rc.top,
               splitter_width, This->navpane.rc.bottom - This->navpane.rc.top,
               TRUE);

    MoveWindow(This->hwnd_sv,
               This->sv_rc.left, This->sv_rc.top,
               This->sv_rc.right - This->sv_rc.left, This->sv_rc.bottom - This->sv_rc.top,
               TRUE);
}

static HRESULT create_new_shellview(ExplorerBrowserImpl *This, IShellItem *psi)
{
    IShellBrowser *psb = &This->IShellBrowser_iface;
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

            This->fs.fFlags |= FWF_USESEARCHFOLDER | FWF_FULLROWSELECT | FWF_NOCLIENTEDGE;
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
                ERR("CreateViewWindow failed (0x%lx)\n", hr);
                IShellView_Release(psv);
            }
        }
        else
            ERR("CreateViewObject failed (0x%lx)\n", hr);

        IShellFolder_Release(psf);
    }
    else
        ERR("SI::BindToHandler failed (0x%lx)\n", hr);

    return hr;
}

static void get_interfaces_from_site(ExplorerBrowserImpl *This)
{
    IServiceProvider *psp;
    HRESULT hr;

    /* Calling this with This->punk_site set to NULL should properly
     * release any previously fetched interfaces.
     */

    if(This->pcdb_site)
    {
        ICommDlgBrowser_Release(This->pcdb_site);
        if(This->pcdb2_site) ICommDlgBrowser2_Release(This->pcdb2_site);
        if(This->pcdb3_site) ICommDlgBrowser3_Release(This->pcdb3_site);

        This->pcdb_site = NULL;
        This->pcdb2_site = NULL;
        This->pcdb3_site = NULL;
    }

    if(This->pepv_site)
    {
        IExplorerPaneVisibility_Release(This->pepv_site);
        This->pepv_site = NULL;
    }

    if(!This->punk_site)
        return;

    hr = IUnknown_QueryInterface(This->punk_site, &IID_IServiceProvider, (void**)&psp);
    if(FAILED(hr))
    {
        ERR("Failed to get IServiceProvider from site.\n");
        return;
    }

    /* ICommDlgBrowser */
    IServiceProvider_QueryService(psp, &SID_SExplorerBrowserFrame, &IID_ICommDlgBrowser,
                                  (void**)&This->pcdb_site);
    IServiceProvider_QueryService(psp, &SID_SExplorerBrowserFrame, &IID_ICommDlgBrowser2,
                                  (void**)&This->pcdb2_site);
    IServiceProvider_QueryService(psp, &SID_SExplorerBrowserFrame, &IID_ICommDlgBrowser3,
                                  (void**)&This->pcdb3_site);

    /* IExplorerPaneVisibility */
    IServiceProvider_QueryService(psp, &SID_ExplorerPaneVisibility, &IID_IExplorerPaneVisibility,
                                  (void**)&This->pepv_site);

    IServiceProvider_Release(psp);
}

/**************************************************************************
 * General pane functionality.
 */
static void update_panestate(ExplorerBrowserImpl *This)
{
    EXPLORERPANESTATE eps = EPS_DONTCARE;
    BOOL show_navpane;
    TRACE("%p\n", This);

    if(!This->pepv_site) return;

    IExplorerPaneVisibility_GetPaneState(This->pepv_site, (REFEXPLORERPANE) &EP_NavPane, &eps);
    if( !(eps & EPS_DEFAULT_OFF) )
        show_navpane = TRUE;
    else
        show_navpane = FALSE;

    if(This->navpane.show != show_navpane)
    {
        update_layout(This);
        size_panes(This);
    }

    This->navpane.show = show_navpane;
}

static void splitter_draw(HWND hwnd, RECT *rc)
{
    HDC hdc = GetDC(hwnd);
    InvertRect(hdc, rc);
    ReleaseDC(hwnd, hdc);
}

/**************************************************************************
 * The Navigation Pane.
 */
static LRESULT navpane_splitter_beginresize(ExplorerBrowserImpl *This, HWND hwnd, LPARAM lParam)
{
    int splitter_width = MulDiv(SPLITTER_WIDTH, This->dpix, USER_DEFAULT_SCREEN_DPI);

    TRACE("\n");

    SetCapture(hwnd);

    This->splitter_rc = This->navpane.rc;
    This->splitter_rc.left = This->splitter_rc.right - splitter_width;

    splitter_draw(GetParent(hwnd), &This->splitter_rc);

    return TRUE;
}

static LRESULT navpane_splitter_resizing(ExplorerBrowserImpl *This, HWND hwnd, LPARAM lParam)
{
    int new_width, dx;
    RECT rc;
    int splitter_width = MulDiv(SPLITTER_WIDTH, This->dpix, USER_DEFAULT_SCREEN_DPI);
    int np_min_width = MulDiv(NP_MIN_WIDTH, This->dpix, USER_DEFAULT_SCREEN_DPI);
    int sv_min_width = MulDiv(SV_MIN_WIDTH, This->dpix, USER_DEFAULT_SCREEN_DPI);

    if(GetCapture() != hwnd) return FALSE;

    dx = (SHORT)LOWORD(lParam);
    TRACE("%d.\n", dx);

    rc = This->navpane.rc;
    new_width = This->navpane.width + dx;
    if(new_width > np_min_width && This->sv_rc.right - new_width > sv_min_width)
    {
        rc.right = new_width;
        rc.left = rc.right - splitter_width;
        splitter_draw(GetParent(hwnd), &This->splitter_rc);
        splitter_draw(GetParent(hwnd), &rc);
        This->splitter_rc = rc;
    }

    return TRUE;
}

static LRESULT navpane_splitter_endresize(ExplorerBrowserImpl *This, HWND hwnd, LPARAM lParam)
{
    int new_width, dx;
    int np_min_width = MulDiv(NP_MIN_WIDTH, This->dpix, USER_DEFAULT_SCREEN_DPI);
    int sv_min_width = MulDiv(SV_MIN_WIDTH, This->dpix, USER_DEFAULT_SCREEN_DPI);

    if(GetCapture() != hwnd) return FALSE;

    dx = (SHORT)LOWORD(lParam);
    TRACE("%d.\n", dx);

    splitter_draw(GetParent(hwnd), &This->splitter_rc);

    new_width = This->navpane.width + dx;
    if(new_width < np_min_width)
        new_width = np_min_width;
    else if(This->sv_rc.right - new_width < sv_min_width)
        new_width = This->sv_rc.right - sv_min_width;

    This->navpane.width = new_width;

    update_layout(This);
    size_panes(This);

    ReleaseCapture();

    return TRUE;
}

static LRESULT navpane_on_wm_create(HWND hwnd, CREATESTRUCTW *crs)
{
    ExplorerBrowserImpl *This = crs->lpCreateParams;
    INameSpaceTreeControl2 *pnstc2;
    DWORD style;
    HRESULT hr;

    TRACE("%p\n", This);
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LPARAM)This);
    This->navpane.hwnd_splitter = hwnd;

    hr = CoCreateInstance(&CLSID_NamespaceTreeControl, NULL, CLSCTX_INPROC_SERVER,
                          &IID_INameSpaceTreeControl2, (void**)&pnstc2);

    if(SUCCEEDED(hr))
    {
        style = NSTCS_HASEXPANDOS | NSTCS_ROOTHASEXPANDO | NSTCS_SHOWSELECTIONALWAYS;
        hr = INameSpaceTreeControl2_Initialize(pnstc2, GetParent(hwnd), NULL, style);
        if(SUCCEEDED(hr))
        {
            INameSpaceTreeControlEvents *pnstce;
            IShellFolder *psfdesktop;
            IShellItem *psi;
            IOleWindow *pow;
            LPITEMIDLIST pidl;
            DWORD cookie, style2 = NSTCS2_DISPLAYPADDING;

            hr = INameSpaceTreeControl2_SetControlStyle2(pnstc2, 0xFF, style2);
            if(FAILED(hr))
                ERR("SetControlStyle2 failed (0x%08lx)\n", hr);

            hr = INameSpaceTreeControl2_QueryInterface(pnstc2, &IID_IOleWindow, (void**)&pow);
            if(SUCCEEDED(hr))
            {
                IOleWindow_GetWindow(pow, &This->navpane.hwnd_nstc);
                IOleWindow_Release(pow);
            }
            else
                ERR("QueryInterface(IOleWindow) failed (0x%08lx)\n", hr);

            pnstce = &This->INameSpaceTreeControlEvents_iface;
            hr = INameSpaceTreeControl2_TreeAdvise(pnstc2, (IUnknown*)pnstce, &cookie);
            if(FAILED(hr))
                ERR("TreeAdvise failed. (0x%08lx).\n", hr);

            /*
             * Add the default roots
             */

            /* TODO: This should be FOLDERID_Links */
            hr = SHGetSpecialFolderLocation(NULL, CSIDL_FAVORITES, &pidl);
            if(SUCCEEDED(hr))
            {
                hr = SHCreateShellItem(NULL, NULL, pidl, &psi);
                if(SUCCEEDED(hr))
                {
                    hr = INameSpaceTreeControl2_AppendRoot(pnstc2, psi, SHCONTF_NONFOLDERS, NSTCRS_VISIBLE, NULL);
                    IShellItem_Release(psi);
                }
                ILFree(pidl);
            }

            SHGetDesktopFolder(&psfdesktop);
            hr = SHGetItemFromObject((IUnknown*)psfdesktop, &IID_IShellItem, (void**)&psi);
            IShellFolder_Release(psfdesktop);
            if(SUCCEEDED(hr))
            {
                hr = INameSpaceTreeControl2_AppendRoot(pnstc2, psi, SHCONTF_FOLDERS, NSTCRS_EXPANDED, NULL);
                IShellItem_Release(psi);
            }

            /* TODO:
             * We should advertise IID_INameSpaceTreeControl to the site of the
             * host through its IProfferService interface, if any.
             */

            This->navpane.pnstc2 = pnstc2;
            This->navpane.nstc_cookie = cookie;

            return TRUE;
        }
    }

    This->navpane.pnstc2 = NULL;
    ERR("Failed (0x%08lx)\n", hr);

    return FALSE;
}

static LRESULT navpane_on_wm_size_move(ExplorerBrowserImpl *This)
{
    UINT height, width;
    int splitter_width = MulDiv(SPLITTER_WIDTH, This->dpix, USER_DEFAULT_SCREEN_DPI);

    TRACE("%p\n", This);

    width = This->navpane.rc.right - This->navpane.rc.left - splitter_width;
    height = This->navpane.rc.bottom - This->navpane.rc.top;

    MoveWindow(This->navpane.hwnd_nstc,
               This->navpane.rc.left, This->navpane.rc.top,
               width, height,
               TRUE);

    return FALSE;
}

static LRESULT navpane_on_wm_destroy(ExplorerBrowserImpl *This)
{
    INameSpaceTreeControl2_TreeUnadvise(This->navpane.pnstc2, This->navpane.nstc_cookie);
    INameSpaceTreeControl2_Release(This->navpane.pnstc2);
    This->navpane.pnstc2 = NULL;
    return TRUE;
}

static LRESULT CALLBACK navpane_wndproc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    ExplorerBrowserImpl *This = (ExplorerBrowserImpl*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);

    switch(uMessage) {
    case WM_CREATE:           return navpane_on_wm_create(hWnd, (CREATESTRUCTW*)lParam);
    case WM_MOVE:             /* Fall through */
    case WM_SIZE:             return navpane_on_wm_size_move(This);
    case WM_DESTROY:          return navpane_on_wm_destroy(This);
    case WM_LBUTTONDOWN:      return navpane_splitter_beginresize(This, hWnd, lParam);
    case WM_MOUSEMOVE:        return navpane_splitter_resizing(This, hWnd, lParam);
    case WM_LBUTTONUP:        return navpane_splitter_endresize(This, hWnd, lParam);
    default:
        return DefWindowProcW(hWnd, uMessage, wParam, lParam);
    }
    return 0;
}

static void initialize_navpane(ExplorerBrowserImpl *This, HWND hwnd_parent, RECT *rc)
{
    WNDCLASSW wc;
    HWND splitter;
    int splitter_width = MulDiv(SPLITTER_WIDTH, This->dpix, USER_DEFAULT_SCREEN_DPI);

    if( !GetClassInfoW(shell32_hInstance, L"eb_navpane", &wc) )
    {
        wc.style            = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc      = navpane_wndproc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 0;
        wc.hInstance        = shell32_hInstance;
        wc.hIcon            = 0;
        wc.hCursor          = LoadCursorW(0, (LPWSTR)IDC_SIZEWE);
        wc.hbrBackground    = (HBRUSH)(COLOR_3DFACE + 1);
        wc.lpszMenuName     = NULL;
        wc.lpszClassName    = L"eb_navpane";

        if (!RegisterClassW(&wc)) return;
    }

    splitter = CreateWindowExW(0, L"eb_navpane", NULL,
                               WS_CHILD | WS_TABSTOP | WS_VISIBLE,
                               rc->right - splitter_width, rc->top,
                               splitter_width, rc->bottom - rc->top,
                               hwnd_parent, 0, shell32_hInstance, This);
    if(!splitter)
        ERR("Failed to create navpane : %ld.\n", GetLastError());
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

static LRESULT main_on_cwm_getishellbrowser(ExplorerBrowserImpl *This)
{
    return (LRESULT)&This->IShellBrowser_iface;
}

static LRESULT CALLBACK main_wndproc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    ExplorerBrowserImpl *This = (ExplorerBrowserImpl*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);

    switch(uMessage)
    {
    case WM_CREATE:           return main_on_wm_create(hWnd, (CREATESTRUCTW*)lParam);
    case WM_SIZE:             return main_on_wm_size(This);
    case CWM_GETISHELLBROWSER: return main_on_cwm_getishellbrowser(This);
    default:                  return DefWindowProcW(hWnd, uMessage, wParam, lParam);
    }

    return 0;
}

/**************************************************************************
 * IExplorerBrowser Implementation
 */

static inline ExplorerBrowserImpl *impl_from_IExplorerBrowser(IExplorerBrowser *iface)
{
    return CONTAINING_RECORD(iface, ExplorerBrowserImpl, IExplorerBrowser_iface);
}

static HRESULT WINAPI IExplorerBrowser_fnQueryInterface(IExplorerBrowser *iface,
                                                        REFIID riid, void **ppvObject)
{
    ExplorerBrowserImpl *This = impl_from_IExplorerBrowser(iface);
    TRACE("%p (%s, %p)\n", This, shdebugstr_guid(riid), ppvObject);

    *ppvObject = NULL;
    if(IsEqualIID(riid, &IID_IExplorerBrowser) ||
       IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObject = &This->IExplorerBrowser_iface;
    }
    else if(IsEqualIID(riid, &IID_IShellBrowser) ||
            IsEqualIID(riid, &IID_IOleWindow))
    {
        *ppvObject = &This->IShellBrowser_iface;
    }
    else if(IsEqualIID(riid, &IID_ICommDlgBrowser) ||
            IsEqualIID(riid, &IID_ICommDlgBrowser2) ||
            IsEqualIID(riid, &IID_ICommDlgBrowser3))
    {
        *ppvObject = &This->ICommDlgBrowser3_iface;
    }
    else if(IsEqualIID(riid, &IID_IObjectWithSite))
    {
        *ppvObject = &This->IObjectWithSite_iface;
    }
    else if(IsEqualIID(riid, &IID_IInputObject))
    {
        *ppvObject = &This->IInputObject_iface;
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
    ExplorerBrowserImpl *This = impl_from_IExplorerBrowser(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("%p - ref %ld\n", This, ref);

    return ref;
}

static ULONG WINAPI IExplorerBrowser_fnRelease(IExplorerBrowser *iface)
{
    ExplorerBrowserImpl *This = impl_from_IExplorerBrowser(iface);
    LONG ref = InterlockedDecrement(&This->ref);
    TRACE("%p - ref %ld\n", This, ref);

    if(!ref)
    {
        TRACE("Freeing.\n");

        if(!This->destroyed)
            IExplorerBrowser_Destroy(iface);

        IObjectWithSite_SetSite(&This->IObjectWithSite_iface, NULL);

        free(This);
    }

    return ref;
}

static HRESULT WINAPI IExplorerBrowser_fnInitialize(IExplorerBrowser *iface,
                                                    HWND hwndParent, const RECT *prc,
                                                    const FOLDERSETTINGS *pfs)
{
    ExplorerBrowserImpl *This = impl_from_IExplorerBrowser(iface);
    WNDCLASSW wc;
    LONG style;
    HDC parent_dc;

    TRACE("%p (%p, %p, %p)\n", This, hwndParent, prc, pfs);

    if(This->hwnd_main)
        return E_UNEXPECTED;

    if(!hwndParent)
        return E_INVALIDARG;

    if( !GetClassInfoW(shell32_hInstance, L"ExplorerBrowserControl", &wc) )
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
        wc.lpszClassName    = L"ExplorerBrowserControl";

        if (!RegisterClassW(&wc)) return E_FAIL;
    }

    parent_dc = GetDC(hwndParent);
    This->dpix = GetDeviceCaps(parent_dc, LOGPIXELSX);
    ReleaseDC(hwndParent, parent_dc);

    This->navpane.width = MulDiv(NP_DEFAULT_WIDTH, This->dpix, USER_DEFAULT_SCREEN_DPI);

    style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS;
    if (!(This->eb_options & EBO_NOBORDER))
        style |= WS_BORDER;
    This->hwnd_main = CreateWindowExW(WS_EX_CONTROLPARENT, L"ExplorerBrowserControl", NULL, style,
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
    ExplorerBrowserImpl *This = impl_from_IExplorerBrowser(iface);
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
    ExplorerBrowserImpl *This = impl_from_IExplorerBrowser(iface);
    TRACE("%p (%p, %s)\n", This, phdwp, wine_dbgstr_rect(&rcBrowser));

    if(phdwp && *phdwp)
    {
        *phdwp = DeferWindowPos(*phdwp, This->hwnd_main, NULL, rcBrowser.left, rcBrowser.top,
                                rcBrowser.right - rcBrowser.left, rcBrowser.bottom - rcBrowser.top,
                                SWP_NOZORDER | SWP_NOACTIVATE);
        if(!*phdwp)
            return E_FAIL;
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
    ExplorerBrowserImpl *This = impl_from_IExplorerBrowser(iface);
    FIXME("stub, %p (%s)\n", This, debugstr_w(pszPropertyBag));

    if(!pszPropertyBag)
        return E_INVALIDARG;

    /* FIXME: This method is currently useless as we don't save any
     * settings anywhere, but at least one application breaks if we
     * return E_NOTIMPL.
     */

    return S_OK;
}

static HRESULT WINAPI IExplorerBrowser_fnSetEmptyText(IExplorerBrowser *iface,
                                                      LPCWSTR pszEmptyText)
{
    ExplorerBrowserImpl *This = impl_from_IExplorerBrowser(iface);
    FIXME("stub, %p (%s)\n", This, debugstr_w(pszEmptyText));

    return E_NOTIMPL;
}

static HRESULT WINAPI IExplorerBrowser_fnSetFolderSettings(IExplorerBrowser *iface,
                                                           const FOLDERSETTINGS *pfs)
{
    ExplorerBrowserImpl *browser = impl_from_IExplorerBrowser(iface);
    IFolderView2 *view;
    HRESULT hr;

    TRACE("explorer browser %p, settings %p.\n", iface, pfs);

    if (!pfs)
        return E_INVALIDARG;

    if (pfs->ViewMode)
        browser->fs.ViewMode = pfs->ViewMode;
    browser->fs.fFlags = pfs->fFlags;

    if (!browser->psv)
        return E_INVALIDARG;

    hr = IShellView_QueryInterface(browser->psv, &IID_IFolderView2, (void **)&view);
    if (SUCCEEDED(hr))
    {
        hr = IFolderView2_SetCurrentViewMode(view, browser->fs.ViewMode);
        if (SUCCEEDED(hr))
            if (SUCCEEDED(hr))
                hr = IFolderView2_SetCurrentFolderFlags(view, ~FWF_NONE, browser->fs.fFlags);
        IFolderView2_Release(view);
    }
    return hr;
}

static HRESULT WINAPI IExplorerBrowser_fnAdvise(IExplorerBrowser *iface,
                                                IExplorerBrowserEvents *psbe,
                                                DWORD *pdwCookie)
{
    ExplorerBrowserImpl *This = impl_from_IExplorerBrowser(iface);
    event_client *client;
    TRACE("%p (%p, %p)\n", This, psbe, pdwCookie);

    client = malloc(sizeof(*client));
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
    ExplorerBrowserImpl *This = impl_from_IExplorerBrowser(iface);
    event_client *client;
    TRACE("%p (0x%lx)\n", This, dwCookie);

    LIST_FOR_EACH_ENTRY(client, &This->event_clients, event_client, entry)
    {
        if(client->cookie == dwCookie)
        {
            list_remove(&client->entry);
            IExplorerBrowserEvents_Release(client->pebe);
            free(client);
            return S_OK;
        }
    }

    return E_INVALIDARG;
}

static HRESULT WINAPI IExplorerBrowser_fnSetOptions(IExplorerBrowser *iface,
                                                    EXPLORER_BROWSER_OPTIONS dwFlag)
{
    ExplorerBrowserImpl *This = impl_from_IExplorerBrowser(iface);
    static const EXPLORER_BROWSER_OPTIONS unsupported_options =
        EBO_ALWAYSNAVIGATE | EBO_NOWRAPPERWINDOW | EBO_HTMLSHAREPOINTVIEW | EBO_NOPERSISTVIEWSTATE;

    TRACE("%p (0x%x)\n", This, dwFlag);

    if(dwFlag & unsupported_options)
        FIXME("Flags 0x%08x contains unsupported options.\n", dwFlag);

    This->eb_options = dwFlag;

    return S_OK;
}

static HRESULT WINAPI IExplorerBrowser_fnGetOptions(IExplorerBrowser *iface,
                                                    EXPLORER_BROWSER_OPTIONS *pdwFlag)
{
    ExplorerBrowserImpl *This = impl_from_IExplorerBrowser(iface);
    TRACE("%p (%p)\n", This, pdwFlag);

    *pdwFlag = This->eb_options;

    return S_OK;
}

static HRESULT WINAPI IExplorerBrowser_fnBrowseToIDList(IExplorerBrowser *iface,
                                                        PCUIDLIST_RELATIVE pidl,
                                                        UINT uFlags)
{
    ExplorerBrowserImpl *This = impl_from_IExplorerBrowser(iface);
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
    update_panestate(This);

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

    /* Expand the NameSpaceTree to the current location. */
    if(This->navpane.show && This->navpane.pnstc2)
    {
        IShellItem *psi;
        hr = SHCreateItemFromIDList(This->current_pidl, &IID_IShellItem, (void**)&psi);
        if(SUCCEEDED(hr))
        {
            INameSpaceTreeControl2_EnsureItemVisible(This->navpane.pnstc2, psi);
            IShellItem_Release(psi);
        }
    }

    return S_OK;
}

static HRESULT WINAPI IExplorerBrowser_fnBrowseToObject(IExplorerBrowser *iface,
                                                        IUnknown *punk, UINT uFlags)
{
    ExplorerBrowserImpl *This = impl_from_IExplorerBrowser(iface);
    LPITEMIDLIST pidl;
    HRESULT hr;
    TRACE("%p (%p, 0x%x)\n", This, punk, uFlags);

    if(!punk)
        return IExplorerBrowser_BrowseToIDList(iface, NULL, uFlags);

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
    ExplorerBrowserImpl *This = impl_from_IExplorerBrowser(iface);
    FIXME("stub, %p (%p, 0x%x)\n", This, punk, dwFlags);

    return E_NOTIMPL;
}

static HRESULT WINAPI IExplorerBrowser_fnRemoveAll(IExplorerBrowser *iface)
{
    ExplorerBrowserImpl *This = impl_from_IExplorerBrowser(iface);
    FIXME("stub, %p\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI IExplorerBrowser_fnGetCurrentView(IExplorerBrowser *iface,
                                                        REFIID riid, void **ppv)
{
    ExplorerBrowserImpl *This = impl_from_IExplorerBrowser(iface);
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
    return CONTAINING_RECORD(iface, ExplorerBrowserImpl, IShellBrowser_iface);
}

static HRESULT WINAPI IShellBrowser_fnQueryInterface(IShellBrowser *iface,
                                                     REFIID riid, void **ppvObject)
{
    ExplorerBrowserImpl *This = impl_from_IShellBrowser(iface);
    TRACE("%p\n", This);
    return IExplorerBrowser_QueryInterface(&This->IExplorerBrowser_iface, riid, ppvObject);
}

static ULONG WINAPI IShellBrowser_fnAddRef(IShellBrowser *iface)
{
    ExplorerBrowserImpl *This = impl_from_IShellBrowser(iface);
    TRACE("%p\n", This);
    return IExplorerBrowser_AddRef(&This->IExplorerBrowser_iface);
}

static ULONG WINAPI IShellBrowser_fnRelease(IShellBrowser *iface)
{
    ExplorerBrowserImpl *This = impl_from_IShellBrowser(iface);
    TRACE("%p\n", This);
    return IExplorerBrowser_Release(&This->IExplorerBrowser_iface);
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

    return IExplorerBrowser_BrowseToIDList(&This->IExplorerBrowser_iface, pidl, wFlags);
}

static HRESULT WINAPI IShellBrowser_fnGetViewStateStream(IShellBrowser *iface,
                                                         DWORD grfMode,
                                                         IStream **ppStrm)
{
    ExplorerBrowserImpl *This = impl_from_IShellBrowser(iface);
    FIXME("stub, %p (0x%lx, %p)\n", This, grfMode, ppStrm);

    *ppStrm = NULL;
    return E_FAIL;
}

static HRESULT WINAPI IShellBrowser_fnGetControlWindow(IShellBrowser *iface,
                                                       UINT id, HWND *phwnd)
{
    ExplorerBrowserImpl *This = impl_from_IShellBrowser(iface);
    TRACE("(%p)->(%d, %p)\n", This, id, phwnd);
    if (phwnd) *phwnd = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowser_fnSendControlMsg(IShellBrowser *iface,
                                                     UINT id, UINT uMsg,
                                                     WPARAM wParam, LPARAM lParam,
                                                     LRESULT *pret)
{
    ExplorerBrowserImpl *This = impl_from_IShellBrowser(iface);
    FIXME("stub, %p (%d, %d, %Ix, %Ix, %p)\n", This, id, uMsg, wParam, lParam, pret);

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
    return CONTAINING_RECORD(iface, ExplorerBrowserImpl, ICommDlgBrowser3_iface);
}

static HRESULT WINAPI ICommDlgBrowser3_fnQueryInterface(ICommDlgBrowser3 *iface,
                                                        REFIID riid,
                                                        void **ppvObject)
{
    ExplorerBrowserImpl *This = impl_from_ICommDlgBrowser3(iface);
    TRACE("%p\n", This);
    return IExplorerBrowser_QueryInterface(&This->IExplorerBrowser_iface, riid, ppvObject);
}

static ULONG WINAPI ICommDlgBrowser3_fnAddRef(ICommDlgBrowser3 *iface)
{
    ExplorerBrowserImpl *This = impl_from_ICommDlgBrowser3(iface);
    TRACE("%p\n", This);
    return IExplorerBrowser_AddRef(&This->IExplorerBrowser_iface);
}

static ULONG WINAPI ICommDlgBrowser3_fnRelease(ICommDlgBrowser3 *iface)
{
    ExplorerBrowserImpl *This = impl_from_ICommDlgBrowser3(iface);
    TRACE("%p\n", This);
    return IExplorerBrowser_Release(&This->IExplorerBrowser_iface);
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
            LPIDA pida = GlobalLock(medium.hGlobal);
            LPCITEMIDLIST pidl_child = (LPCITEMIDLIST) ((LPBYTE)pida+pida->aoffset[1]);

            /* Handle folders by browsing to them. */
            if(_ILIsFolder(pidl_child) || _ILIsDrive(pidl_child) || _ILIsSpecialFolder(pidl_child))
            {
                IExplorerBrowser_BrowseToIDList(&This->IExplorerBrowser_iface, pidl_child, SBSP_RELATIVE);
                ret = S_OK;
            }
            GlobalUnlock(medium.hGlobal);
            GlobalFree(medium.hGlobal);
        }
        else
            ERR("Failed to get data from IDataObject.\n");
    } else
        ERR("Failed to get IDataObject.\n");

    /* If we didn't handle the default command, check if we have a
     * client that does */
    if(ret == S_FALSE && This->pcdb_site)
        return ICommDlgBrowser_OnDefaultCommand(This->pcdb_site, shv);

    return ret;
}
static HRESULT WINAPI ICommDlgBrowser3_fnOnStateChange(ICommDlgBrowser3 *iface,
                                                       IShellView *shv, ULONG uChange)
{
    ExplorerBrowserImpl *This = impl_from_ICommDlgBrowser3(iface);
    TRACE("%p (%p, %ld)\n", This, shv, uChange);

    if(This->pcdb_site)
        return ICommDlgBrowser_OnStateChange(This->pcdb_site, shv, uChange);

    return E_NOTIMPL;
}
static HRESULT WINAPI ICommDlgBrowser3_fnIncludeObject(ICommDlgBrowser3 *iface,
                                                       IShellView *pshv, LPCITEMIDLIST pidl)
{
    ExplorerBrowserImpl *This = impl_from_ICommDlgBrowser3(iface);
    TRACE("%p (%p, %p)\n", This, pshv, pidl);

    if(This->pcdb_site)
        return ICommDlgBrowser_IncludeObject(This->pcdb_site, pshv, pidl);

    return S_OK;
}

static HRESULT WINAPI ICommDlgBrowser3_fnNotify(ICommDlgBrowser3 *iface,
                                                IShellView *pshv,
                                                DWORD dwNotifyType)
{
    ExplorerBrowserImpl *This = impl_from_ICommDlgBrowser3(iface);
    TRACE("%p (%p, 0x%lx)\n", This, pshv, dwNotifyType);

    if(This->pcdb2_site)
        return ICommDlgBrowser2_Notify(This->pcdb2_site, pshv, dwNotifyType);

    return S_OK;
}

static HRESULT WINAPI ICommDlgBrowser3_fnGetDefaultMenuText(ICommDlgBrowser3 *iface,
                                                            IShellView *pshv,
                                                            LPWSTR pszText, int cchMax)
{
    ExplorerBrowserImpl *This = impl_from_ICommDlgBrowser3(iface);
    TRACE("%p (%p, %s, %d)\n", This, pshv, debugstr_w(pszText), cchMax);

    if(This->pcdb2_site)
        return ICommDlgBrowser2_GetDefaultMenuText(This->pcdb2_site, pshv, pszText, cchMax);

    return S_OK;
}

static HRESULT WINAPI ICommDlgBrowser3_fnGetViewFlags(ICommDlgBrowser3 *iface,
                                                      DWORD *pdwFlags)
{
    ExplorerBrowserImpl *This = impl_from_ICommDlgBrowser3(iface);
    TRACE("%p (%p)\n", This, pdwFlags);

    if(This->pcdb2_site)
        return ICommDlgBrowser2_GetViewFlags(This->pcdb2_site, pdwFlags);

    return S_OK;
}

static HRESULT WINAPI ICommDlgBrowser3_fnOnColumnClicked(ICommDlgBrowser3 *iface,
                                                         IShellView *pshv, int iColumn)
{
    ExplorerBrowserImpl *This = impl_from_ICommDlgBrowser3(iface);
    TRACE("%p (%p, %d)\n", This, pshv, iColumn);

    if(This->pcdb3_site)
        return ICommDlgBrowser3_OnColumnClicked(This->pcdb3_site, pshv, iColumn);

    return S_OK;
}

static HRESULT WINAPI ICommDlgBrowser3_fnGetCurrentFilter(ICommDlgBrowser3 *iface,
                                                          LPWSTR pszFileSpec,
                                                          int cchFileSpec)
{
    ExplorerBrowserImpl *This = impl_from_ICommDlgBrowser3(iface);
    TRACE("%p (%s, %d)\n", This, debugstr_w(pszFileSpec), cchFileSpec);

    if(This->pcdb3_site)
        return ICommDlgBrowser3_GetCurrentFilter(This->pcdb3_site, pszFileSpec, cchFileSpec);

    return S_OK;
}

static HRESULT WINAPI ICommDlgBrowser3_fnOnPreViewCreated(ICommDlgBrowser3 *iface,
                                                          IShellView *pshv)
{
    ExplorerBrowserImpl *This = impl_from_ICommDlgBrowser3(iface);
    TRACE("%p (%p)\n", This, pshv);

    if(This->pcdb3_site)
        return ICommDlgBrowser3_OnPreViewCreated(This->pcdb3_site, pshv);

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
    ICommDlgBrowser3_fnOnPreViewCreated
};

/**************************************************************************
 * IObjectWithSite Implementation
 */

static inline ExplorerBrowserImpl *impl_from_IObjectWithSite(IObjectWithSite *iface)
{
    return CONTAINING_RECORD(iface, ExplorerBrowserImpl, IObjectWithSite_iface);
}

static HRESULT WINAPI IObjectWithSite_fnQueryInterface(IObjectWithSite *iface,
                                                       REFIID riid, void **ppvObject)
{
    ExplorerBrowserImpl *This = impl_from_IObjectWithSite(iface);
    TRACE("%p\n", This);
    return IExplorerBrowser_QueryInterface(&This->IExplorerBrowser_iface, riid, ppvObject);
}

static ULONG WINAPI IObjectWithSite_fnAddRef(IObjectWithSite *iface)
{
    ExplorerBrowserImpl *This = impl_from_IObjectWithSite(iface);
    TRACE("%p\n", This);
    return IExplorerBrowser_AddRef(&This->IExplorerBrowser_iface);
}

static ULONG WINAPI IObjectWithSite_fnRelease(IObjectWithSite *iface)
{
    ExplorerBrowserImpl *This = impl_from_IObjectWithSite(iface);
    TRACE("%p\n", This);
    return IExplorerBrowser_Release(&This->IExplorerBrowser_iface);
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

/**************************************************************************
 * INameSpaceTreeControlEvents Implementation
 */
static inline ExplorerBrowserImpl *impl_from_INameSpaceTreeControlEvents(INameSpaceTreeControlEvents *iface)
{
    return CONTAINING_RECORD(iface, ExplorerBrowserImpl, INameSpaceTreeControlEvents_iface);
}

static HRESULT WINAPI NSTCEvents_fnQueryInterface(INameSpaceTreeControlEvents *iface,
                                                  REFIID riid, void **ppvObject)
{
    ExplorerBrowserImpl *This = impl_from_INameSpaceTreeControlEvents(iface);
    TRACE("%p (%s, %p)\n", This, shdebugstr_guid(riid), ppvObject);

    *ppvObject = NULL;
    if(IsEqualIID(riid, &IID_INameSpaceTreeControlEvents) ||
       IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObject = iface;
    }

    if(*ppvObject)
    {
        IUnknown_AddRef((IUnknown*)*ppvObject);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI NSTCEvents_fnAddRef(INameSpaceTreeControlEvents *iface)
{
    ExplorerBrowserImpl *This = impl_from_INameSpaceTreeControlEvents(iface);
    TRACE("%p\n", This);
    return IExplorerBrowser_AddRef(&This->IExplorerBrowser_iface);
}

static ULONG WINAPI NSTCEvents_fnRelease(INameSpaceTreeControlEvents *iface)
{
    ExplorerBrowserImpl *This = impl_from_INameSpaceTreeControlEvents(iface);
    TRACE("%p\n", This);
    return IExplorerBrowser_Release(&This->IExplorerBrowser_iface);
}

static HRESULT WINAPI NSTCEvents_fnOnItemClick(INameSpaceTreeControlEvents *iface,
                                               IShellItem *psi,
                                               NSTCEHITTEST nstceHitTest,
                                               NSTCECLICKTYPE nstceClickType)
{
    ExplorerBrowserImpl *This = impl_from_INameSpaceTreeControlEvents(iface);
    TRACE("%p (%p, 0x%lx, 0x%lx)\n", This, psi, nstceHitTest, nstceClickType);
    return S_OK;
}

static HRESULT WINAPI NSTCEvents_fnOnPropertyItemCommit(INameSpaceTreeControlEvents *iface,
                                                        IShellItem *psi)
{
    ExplorerBrowserImpl *This = impl_from_INameSpaceTreeControlEvents(iface);
    TRACE("%p (%p)\n", This, psi);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTCEvents_fnOnItemStateChanging(INameSpaceTreeControlEvents *iface,
                                                       IShellItem *psi,
                                                       NSTCITEMSTATE nstcisMask,
                                                       NSTCITEMSTATE nstcisState)
{
    ExplorerBrowserImpl *This = impl_from_INameSpaceTreeControlEvents(iface);
    TRACE("%p (%p, 0x%lx, 0x%lx)\n", This, psi, nstcisMask, nstcisState);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTCEvents_fnOnItemStateChanged(INameSpaceTreeControlEvents *iface,
                                                      IShellItem *psi,
                                                      NSTCITEMSTATE nstcisMask,
                                                      NSTCITEMSTATE nstcisState)
{
    ExplorerBrowserImpl *This = impl_from_INameSpaceTreeControlEvents(iface);
    TRACE("%p (%p, 0x%lx, 0x%lx)\n", This, psi, nstcisMask, nstcisState);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTCEvents_fnOnSelectionChanged(INameSpaceTreeControlEvents *iface,
                                                      IShellItemArray *psiaSelection)
{
    ExplorerBrowserImpl *This = impl_from_INameSpaceTreeControlEvents(iface);
    IShellItem *psi;
    HRESULT hr;
    TRACE("%p (%p)\n", This, psiaSelection);

    hr = IShellItemArray_GetItemAt(psiaSelection, 0, &psi);
    if(SUCCEEDED(hr))
    {
        hr = IExplorerBrowser_BrowseToObject(&This->IExplorerBrowser_iface,
                                             (IUnknown*)psi, SBSP_DEFBROWSER);
        IShellItem_Release(psi);
    }

    return hr;
}

static HRESULT WINAPI NSTCEvents_fnOnKeyboardInput(INameSpaceTreeControlEvents *iface,
                                                   UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    ExplorerBrowserImpl *This = impl_from_INameSpaceTreeControlEvents(iface);
    TRACE("%p (%d, 0x%Ix, 0x%Ix)\n", This, uMsg, wParam, lParam);
    return S_OK;
}

static HRESULT WINAPI NSTCEvents_fnOnBeforeExpand(INameSpaceTreeControlEvents *iface,
                                                  IShellItem *psi)
{
    ExplorerBrowserImpl *This = impl_from_INameSpaceTreeControlEvents(iface);
    TRACE("%p (%p)\n", This, psi);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTCEvents_fnOnAfterExpand(INameSpaceTreeControlEvents *iface,
                                                 IShellItem *psi)
{
    ExplorerBrowserImpl *This = impl_from_INameSpaceTreeControlEvents(iface);
    TRACE("%p (%p)\n", This, psi);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTCEvents_fnOnBeginLabelEdit(INameSpaceTreeControlEvents *iface,
                                                    IShellItem *psi)
{
    ExplorerBrowserImpl *This = impl_from_INameSpaceTreeControlEvents(iface);
    TRACE("%p (%p)\n", This, psi);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTCEvents_fnOnEndLabelEdit(INameSpaceTreeControlEvents *iface,
                                                  IShellItem *psi)
{
    ExplorerBrowserImpl *This = impl_from_INameSpaceTreeControlEvents(iface);
    TRACE("%p (%p)\n", This, psi);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTCEvents_fnOnGetToolTip(INameSpaceTreeControlEvents *iface,
                                                IShellItem *psi, LPWSTR pszTip, int cchTip)
{
    ExplorerBrowserImpl *This = impl_from_INameSpaceTreeControlEvents(iface);
    TRACE("%p (%p, %p, %d)\n", This, psi, pszTip, cchTip);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTCEvents_fnOnBeforeItemDelete(INameSpaceTreeControlEvents *iface,
                                                      IShellItem *psi)
{
    ExplorerBrowserImpl *This = impl_from_INameSpaceTreeControlEvents(iface);
    TRACE("%p (%p)\n", This, psi);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTCEvents_fnOnItemAdded(INameSpaceTreeControlEvents *iface,
                                               IShellItem *psi, BOOL fIsRoot)
{
    ExplorerBrowserImpl *This = impl_from_INameSpaceTreeControlEvents(iface);
    TRACE("%p (%p, %d)\n", This, psi, fIsRoot);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTCEvents_fnOnItemDeleted(INameSpaceTreeControlEvents *iface,
                                                 IShellItem *psi, BOOL fIsRoot)
{
    ExplorerBrowserImpl *This = impl_from_INameSpaceTreeControlEvents(iface);
    TRACE("%p (%p, %d)\n", This, psi, fIsRoot);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTCEvents_fnOnBeforeContextMenu(INameSpaceTreeControlEvents *iface,
                                                       IShellItem *psi, REFIID riid, void **ppv)
{
    ExplorerBrowserImpl *This = impl_from_INameSpaceTreeControlEvents(iface);
    TRACE("%p (%p, %s, %p)\n", This, psi, shdebugstr_guid(riid), ppv);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTCEvents_fnOnAfterContextMenu(INameSpaceTreeControlEvents *iface,
                                                      IShellItem *psi, IContextMenu *pcmIn,
                                                      REFIID riid, void **ppv)
{
    ExplorerBrowserImpl *This = impl_from_INameSpaceTreeControlEvents(iface);
    TRACE("%p (%p, %p, %s, %p)\n", This, psi, pcmIn, shdebugstr_guid(riid), ppv);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTCEvents_fnOnBeforeStateImageChange(INameSpaceTreeControlEvents *iface,
                                                            IShellItem *psi)
{
    ExplorerBrowserImpl *This = impl_from_INameSpaceTreeControlEvents(iface);
    TRACE("%p (%p)\n", This, psi);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTCEvents_fnOnGetDefaultIconIndex(INameSpaceTreeControlEvents* iface,
                                                         IShellItem *psi,
                                                         int *piDefaultIcon, int *piOpenIcon)
{
    ExplorerBrowserImpl *This = impl_from_INameSpaceTreeControlEvents(iface);
    TRACE("%p (%p, %p, %p)\n", This, psi, piDefaultIcon, piOpenIcon);
    return E_NOTIMPL;
}


static const INameSpaceTreeControlEventsVtbl vt_INameSpaceTreeControlEvents =  {
    NSTCEvents_fnQueryInterface,
    NSTCEvents_fnAddRef,
    NSTCEvents_fnRelease,
    NSTCEvents_fnOnItemClick,
    NSTCEvents_fnOnPropertyItemCommit,
    NSTCEvents_fnOnItemStateChanging,
    NSTCEvents_fnOnItemStateChanged,
    NSTCEvents_fnOnSelectionChanged,
    NSTCEvents_fnOnKeyboardInput,
    NSTCEvents_fnOnBeforeExpand,
    NSTCEvents_fnOnAfterExpand,
    NSTCEvents_fnOnBeginLabelEdit,
    NSTCEvents_fnOnEndLabelEdit,
    NSTCEvents_fnOnGetToolTip,
    NSTCEvents_fnOnBeforeItemDelete,
    NSTCEvents_fnOnItemAdded,
    NSTCEvents_fnOnItemDeleted,
    NSTCEvents_fnOnBeforeContextMenu,
    NSTCEvents_fnOnAfterContextMenu,
    NSTCEvents_fnOnBeforeStateImageChange,
    NSTCEvents_fnOnGetDefaultIconIndex
};

/**************************************************************************
 * IInputObject Implementation
 */

static inline ExplorerBrowserImpl *impl_from_IInputObject(IInputObject *iface)
{
    return CONTAINING_RECORD(iface, ExplorerBrowserImpl, IInputObject_iface);
}

static HRESULT WINAPI IInputObject_fnQueryInterface(IInputObject *iface,
                                                    REFIID riid, void **ppvObject)
{
    ExplorerBrowserImpl *This = impl_from_IInputObject(iface);
    TRACE("%p\n", This);
    return IExplorerBrowser_QueryInterface(&This->IExplorerBrowser_iface, riid, ppvObject);
}

static ULONG WINAPI IInputObject_fnAddRef(IInputObject *iface)
{
    ExplorerBrowserImpl *This = impl_from_IInputObject(iface);
    TRACE("%p\n", This);
    return IExplorerBrowser_AddRef(&This->IExplorerBrowser_iface);
}

static ULONG WINAPI IInputObject_fnRelease(IInputObject *iface)
{
    ExplorerBrowserImpl *This = impl_from_IInputObject(iface);
    TRACE("%p\n", This);
    return IExplorerBrowser_Release(&This->IExplorerBrowser_iface);
}

static HRESULT WINAPI IInputObject_fnUIActivateIO(IInputObject *iface, BOOL fActivate, MSG *pMsg)
{
    ExplorerBrowserImpl *This = impl_from_IInputObject(iface);
    FIXME("stub, %p (%d, %p)\n", This, fActivate, pMsg);
    return E_NOTIMPL;
}

static HRESULT WINAPI IInputObject_fnHasFocusIO(IInputObject *iface)
{
    ExplorerBrowserImpl *This = impl_from_IInputObject(iface);
    FIXME("stub, %p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI IInputObject_fnTranslateAcceleratorIO(IInputObject *iface, MSG *pMsg)
{
    ExplorerBrowserImpl *This = impl_from_IInputObject(iface);
    FIXME("stub, %p (%p)\n", This, pMsg);
    return E_NOTIMPL;
}

static IInputObjectVtbl vt_IInputObject = {
    IInputObject_fnQueryInterface,
    IInputObject_fnAddRef,
    IInputObject_fnRelease,
    IInputObject_fnUIActivateIO,
    IInputObject_fnHasFocusIO,
    IInputObject_fnTranslateAcceleratorIO
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

    eb = calloc(1, sizeof(*eb));
    eb->ref = 1;
    eb->IExplorerBrowser_iface.lpVtbl = &vt_IExplorerBrowser;
    eb->IShellBrowser_iface.lpVtbl    = &vt_IShellBrowser;
    eb->ICommDlgBrowser3_iface.lpVtbl = &vt_ICommDlgBrowser3;
    eb->IObjectWithSite_iface.lpVtbl  = &vt_IObjectWithSite;
    eb->INameSpaceTreeControlEvents_iface.lpVtbl = &vt_INameSpaceTreeControlEvents;
    eb->IInputObject_iface.lpVtbl     = &vt_IInputObject;

    /* Default settings */
    eb->navpane.show = TRUE;

    list_init(&eb->event_clients);
    list_init(&eb->travellog);

    ret = IExplorerBrowser_QueryInterface(&eb->IExplorerBrowser_iface, riid, ppv);
    IExplorerBrowser_Release(&eb->IExplorerBrowser_iface);

    TRACE("--(%p)\n", ppv);
    return ret;
}
