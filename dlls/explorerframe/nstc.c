/*
 * NamespaceTreeControl implementation.
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

#include "wine/debug.h"

#include "explorerframe_main.h"

WINE_DEFAULT_DEBUG_CHANNEL(nstc);

typedef struct {
    const INameSpaceTreeControl2Vtbl *lpVtbl;
    const IOleWindowVtbl *lpowVtbl;
    LONG ref;

    HWND hwnd_main;
    HWND hwnd_tv;

    NSTCSTYLE style;
} NSTC2Impl;

static const DWORD unsupported_styles =
    NSTCS_SINGLECLICKEXPAND | NSTCS_NOREPLACEOPEN | NSTCS_NOORDERSTREAM | NSTCS_FAVORITESMODE |
    NSTCS_EMPTYTEXT | NSTCS_ALLOWJUNCTIONS | NSTCS_SHOWTABSBUTTON | NSTCS_SHOWDELETEBUTTON |
    NSTCS_SHOWREFRESHBUTTON | NSTCS_SPRINGEXPAND | NSTCS_RICHTOOLTIP | NSTCS_NOINDENTCHECKS;

/*************************************************************************
 * NamespaceTree helper functions
 */
static DWORD treeview_style_from_nstcs(NSTC2Impl *This, NSTCSTYLE nstcs,
                                       NSTCSTYLE nstcs_mask, DWORD *new_style)
{
    DWORD old_style, tv_mask = 0;
    TRACE("%p, %x, %x, %p\n", This, nstcs, nstcs_mask, new_style);

    if(This->hwnd_tv)
        old_style = GetWindowLongPtrW(This->hwnd_tv, GWL_STYLE);
    else
        old_style = /* The default */
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
            WS_TABSTOP | TVS_NOHSCROLL | TVS_NONEVENHEIGHT | TVS_INFOTIP |
            TVS_EDITLABELS | TVS_TRACKSELECT;

    if(nstcs_mask & NSTCS_HASEXPANDOS)         tv_mask |= TVS_HASBUTTONS;
    if(nstcs_mask & NSTCS_HASLINES)            tv_mask |= TVS_HASLINES;
    if(nstcs_mask & NSTCS_FULLROWSELECT)       tv_mask |= TVS_FULLROWSELECT;
    if(nstcs_mask & NSTCS_HORIZONTALSCROLL)    tv_mask |= TVS_NOHSCROLL;
    if(nstcs_mask & NSTCS_ROOTHASEXPANDO)      tv_mask |= TVS_LINESATROOT;
    if(nstcs_mask & NSTCS_SHOWSELECTIONALWAYS) tv_mask |= TVS_SHOWSELALWAYS;
    if(nstcs_mask & NSTCS_NOINFOTIP)           tv_mask |= TVS_INFOTIP;
    if(nstcs_mask & NSTCS_EVENHEIGHT)          tv_mask |= TVS_NONEVENHEIGHT;
    if(nstcs_mask & NSTCS_DISABLEDRAGDROP)     tv_mask |= TVS_DISABLEDRAGDROP;
    if(nstcs_mask & NSTCS_NOEDITLABELS)        tv_mask |= TVS_EDITLABELS;
    if(nstcs_mask & NSTCS_CHECKBOXES)          tv_mask |= TVS_CHECKBOXES;

    *new_style = 0;

    if(nstcs & NSTCS_HASEXPANDOS)         *new_style |= TVS_HASBUTTONS;
    if(nstcs & NSTCS_HASLINES)            *new_style |= TVS_HASLINES;
    if(nstcs & NSTCS_FULLROWSELECT)       *new_style |= TVS_FULLROWSELECT;
    if(!(nstcs & NSTCS_HORIZONTALSCROLL)) *new_style |= TVS_NOHSCROLL;
    if(nstcs & NSTCS_ROOTHASEXPANDO)      *new_style |= TVS_LINESATROOT;
    if(nstcs & NSTCS_SHOWSELECTIONALWAYS) *new_style |= TVS_SHOWSELALWAYS;
    if(!(nstcs & NSTCS_NOINFOTIP))        *new_style |= TVS_INFOTIP;
    if(!(nstcs & NSTCS_EVENHEIGHT))       *new_style |= TVS_NONEVENHEIGHT;
    if(nstcs & NSTCS_DISABLEDRAGDROP)     *new_style |= TVS_DISABLEDRAGDROP;
    if(!(nstcs & NSTCS_NOEDITLABELS))     *new_style |= TVS_EDITLABELS;
    if(nstcs & NSTCS_CHECKBOXES)          *new_style |= TVS_CHECKBOXES;

    *new_style = (old_style & ~tv_mask) | (*new_style & tv_mask);

    TRACE("old: %08x, new: %08x\n", old_style, *new_style);

    return old_style^*new_style;
}

/*************************************************************************
 * NamespaceTree window functions
 */
static LRESULT create_namespacetree(HWND hWnd, CREATESTRUCTW *crs)
{
    NSTC2Impl *This = crs->lpCreateParams;
    HIMAGELIST ShellSmallIconList;
    DWORD treeview_style, treeview_ex_style;

    TRACE("%p (%p)\n", This, crs);
    SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LPARAM)This);
    This->hwnd_main = hWnd;

    treeview_style_from_nstcs(This, This->style, 0xFFFFFFFF, &treeview_style);

    This->hwnd_tv = CreateWindowExW(0, WC_TREEVIEWW, NULL, treeview_style,
                                    0, 0, crs->cx, crs->cy,
                                    hWnd, NULL, explorerframe_hinstance, NULL);

    if(!This->hwnd_tv)
    {
        ERR("Failed to create treeview!\n");
        return HRESULT_FROM_WIN32(GetLastError());
    }

    treeview_ex_style = TVS_EX_DRAWIMAGEASYNC | TVS_EX_RICHTOOLTIP |
        TVS_EX_DOUBLEBUFFER | TVS_EX_NOSINGLECOLLAPSE;

    if(This->style & NSTCS_AUTOHSCROLL)
        treeview_ex_style |= TVS_EX_AUTOHSCROLL;
    if(This->style & NSTCS_FADEINOUTEXPANDOS)
        treeview_ex_style |= TVS_EX_FADEINOUTEXPANDOS;
    if(This->style & NSTCS_PARTIALCHECKBOXES)
        treeview_ex_style |= TVS_EX_PARTIALCHECKBOXES;
    if(This->style & NSTCS_EXCLUSIONCHECKBOXES)
        treeview_ex_style |= TVS_EX_EXCLUSIONCHECKBOXES;
    if(This->style & NSTCS_DIMMEDCHECKBOXES)
        treeview_ex_style |= TVS_EX_DIMMEDCHECKBOXES;

    SendMessageW(This->hwnd_tv, TVM_SETEXTENDEDSTYLE, treeview_ex_style, 0xffff);

    if(Shell_GetImageLists(NULL, &ShellSmallIconList))
    {
        SendMessageW(This->hwnd_tv, TVM_SETIMAGELIST,
                     (WPARAM)TVSIL_NORMAL, (LPARAM)ShellSmallIconList);
    }
    else
    {
        ERR("Failed to get the System Image List.\n");
    }

    INameSpaceTreeControl_AddRef((INameSpaceTreeControl*)This);

    return TRUE;
}

static LRESULT resize_namespacetree(NSTC2Impl *This)
{
    RECT rc;
    TRACE("%p\n", This);

    GetClientRect(This->hwnd_main, &rc);
    MoveWindow(This->hwnd_tv, 0, 0, rc.right-rc.left, rc.bottom-rc.top, TRUE);

    return TRUE;
}

static LRESULT destroy_namespacetree(NSTC2Impl *This)
{
    TRACE("%p\n", This);

    /* This reference was added in create_namespacetree */
    INameSpaceTreeControl_Release((INameSpaceTreeControl*)This);
    return TRUE;
}

static LRESULT CALLBACK NSTC2_WndProc(HWND hWnd, UINT uMessage,
                                      WPARAM wParam, LPARAM lParam)
{
    NSTC2Impl *This = (NSTC2Impl*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);

    switch(uMessage)
    {
    case WM_NCCREATE:         return create_namespacetree(hWnd, (CREATESTRUCTW*)lParam);
    case WM_SIZE:             return resize_namespacetree(This);
    case WM_DESTROY:          return destroy_namespacetree(This);
    default:                  return DefWindowProcW(hWnd, uMessage, wParam, lParam);
    }
    return 0;
}

/**************************************************************************
 * INameSpaceTreeControl2 Implementation
 */
static HRESULT WINAPI NSTC2_fnQueryInterface(INameSpaceTreeControl2* iface,
                                             REFIID riid,
                                             void **ppvObject)
{
    NSTC2Impl *This = (NSTC2Impl*)iface;
    TRACE("%p (%s, %p)\n", This, debugstr_guid(riid), ppvObject);

    *ppvObject = NULL;
    if(IsEqualIID(riid, &IID_INameSpaceTreeControl2) ||
       IsEqualIID(riid, &IID_INameSpaceTreeControl) ||
       IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObject = This;
    }
    else if(IsEqualIID(riid, &IID_IOleWindow))
    {
        *ppvObject = &This->lpowVtbl;
    }

    if(*ppvObject)
    {
        IUnknown_AddRef((IUnknown*)*ppvObject);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI NSTC2_fnAddRef(INameSpaceTreeControl2* iface)
{
    NSTC2Impl *This = (NSTC2Impl*)iface;
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p - ref %d\n", This, ref);

    return ref;
}

static ULONG WINAPI NSTC2_fnRelease(INameSpaceTreeControl2* iface)
{
    NSTC2Impl *This = (NSTC2Impl*)iface;
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p - ref: %d\n", This, ref);

    if(!ref)
    {
        TRACE("Freeing.\n");
        HeapFree(GetProcessHeap(), 0, This);
        EFRAME_UnlockModule();
        return 0;
    }

    return ref;
}

static HRESULT WINAPI NSTC2_fnInitialize(INameSpaceTreeControl2* iface,
                                         HWND hwndParent,
                                         RECT *prc,
                                         NSTCSTYLE nstcsFlags)
{
    NSTC2Impl *This = (NSTC2Impl*)iface;
    WNDCLASSW wc;
    DWORD window_style, window_ex_style;
    RECT rc;
    static const WCHAR NSTC2_CLASS_NAME[] =
        {'N','a','m','e','s','p','a','c','e','T','r','e','e',
         'C','o','n','t','r','o','l',0};

    TRACE("%p (%p, %p, %x)\n", This, hwndParent, prc, nstcsFlags);

    if(nstcsFlags & unsupported_styles)
        FIXME("0x%08x contains the unsupported style(s) 0x%08x\n",
              nstcsFlags, nstcsFlags & unsupported_styles);

    This->style = nstcsFlags;

    if(!GetClassInfoW(explorerframe_hinstance, NSTC2_CLASS_NAME, &wc))
    {
        wc.style            = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc      = NSTC2_WndProc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 0;
        wc.hInstance        = explorerframe_hinstance;
        wc.hIcon            = 0;
        wc.hCursor          = LoadCursorW(0, (LPWSTR)IDC_ARROW);
        wc.hbrBackground    = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszMenuName     = NULL;
        wc.lpszClassName    = NSTC2_CLASS_NAME;

        if (!RegisterClassW(&wc)) return E_FAIL;
    }

    /* NSTCS_TABSTOP and NSTCS_BORDER affects the host window */
    window_style = WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
        (nstcsFlags & NSTCS_BORDER ? WS_BORDER : 0);
    window_ex_style = nstcsFlags & NSTCS_TABSTOP ? WS_EX_CONTROLPARENT : 0;

    if(prc)
        CopyRect(&rc, prc);
    else
        rc.left = rc.right = rc.top = rc.bottom = 0;

    This->hwnd_main = CreateWindowExW(window_ex_style, NSTC2_CLASS_NAME, NULL, window_style,
                                      rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                                      hwndParent, 0, explorerframe_hinstance, This);

    if(!This->hwnd_main)
    {
        ERR("Failed to create the window.\n");
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

static HRESULT WINAPI NSTC2_fnTreeAdvise(INameSpaceTreeControl2* iface,
                                         IUnknown *punk,
                                         DWORD *pdwCookie)
{
    NSTC2Impl *This = (NSTC2Impl*)iface;
    FIXME("stub, %p (%p, %p)\n", This, punk, pdwCookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTC2_fnTreeUnadvise(INameSpaceTreeControl2* iface,
                                           DWORD dwCookie)
{
    NSTC2Impl *This = (NSTC2Impl*)iface;
    FIXME("stub, %p (%x)\n", This, dwCookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTC2_fnInsertRoot(INameSpaceTreeControl2* iface,
                                         int iIndex,
                                         IShellItem *psiRoot,
                                         SHCONTF grfEnumFlags,
                                         NSTCROOTSTYLE grfRootStyle,
                                         IShellItemFilter *pif)
{
    NSTC2Impl *This = (NSTC2Impl*)iface;
    FIXME("stub, %p, %p, %x, %x, %p\n", This, psiRoot, grfEnumFlags, grfRootStyle, pif);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTC2_fnAppendRoot(INameSpaceTreeControl2* iface,
                                         IShellItem *psiRoot,
                                         SHCONTF grfEnumFlags,
                                         NSTCROOTSTYLE grfRootStyle,
                                         IShellItemFilter *pif)
{
    NSTC2Impl *This = (NSTC2Impl*)iface;
    FIXME("stub, %p, %p, %x, %x, %p\n",
          This, psiRoot, grfEnumFlags, grfRootStyle, pif);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTC2_fnRemoveRoot(INameSpaceTreeControl2* iface,
                                         IShellItem *psiRoot)
{
    NSTC2Impl *This = (NSTC2Impl*)iface;
    FIXME("stub, %p (%p)\n", This, psiRoot);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTC2_fnRemoveAllRoots(INameSpaceTreeControl2* iface)
{
    NSTC2Impl *This = (NSTC2Impl*)iface;
    FIXME("stub, %p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTC2_fnGetRootItems(INameSpaceTreeControl2* iface,
                                           IShellItemArray **ppsiaRootItems)
{
    NSTC2Impl *This = (NSTC2Impl*)iface;
    FIXME("stub, %p (%p)\n", This, ppsiaRootItems);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTC2_fnSetItemState(INameSpaceTreeControl2* iface,
                                           IShellItem *psi,
                                           NSTCITEMSTATE nstcisMask,
                                           NSTCITEMSTATE nstcisFlags)
{
    NSTC2Impl *This = (NSTC2Impl*)iface;
    FIXME("stub, %p (%p, %x, %x)\n", This, psi, nstcisMask, nstcisFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTC2_fnGetItemState(INameSpaceTreeControl2* iface,
                                           IShellItem *psi,
                                           NSTCITEMSTATE nstcisMask,
                                           NSTCITEMSTATE *pnstcisFlags)
{
    NSTC2Impl *This = (NSTC2Impl*)iface;
    FIXME("stub, %p (%p, %x, %p)\n", This, psi, nstcisMask, pnstcisFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTC2_fnGetSelectedItems(INameSpaceTreeControl2* iface,
                                               IShellItemArray **psiaItems)
{
    NSTC2Impl *This = (NSTC2Impl*)iface;
    FIXME("stub, %p (%p)\n", This, psiaItems);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTC2_fnGetItemCustomState(INameSpaceTreeControl2* iface,
                                                 IShellItem *psi,
                                                 int *piStateNumber)
{
    NSTC2Impl *This = (NSTC2Impl*)iface;
    FIXME("stub, %p (%p, %p)\n", This, psi, piStateNumber);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTC2_fnSetItemCustomState(INameSpaceTreeControl2* iface,
                                                 IShellItem *psi,
                                                 int iStateNumber)
{
    NSTC2Impl *This = (NSTC2Impl*)iface;
    FIXME("stub, %p (%p, %d)\n", This, psi, iStateNumber);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTC2_fnEnsureItemVisible(INameSpaceTreeControl2* iface,
                                                IShellItem *psi)
{
    NSTC2Impl *This = (NSTC2Impl*)iface;
    FIXME("stub, %p (%p)\n", This, psi);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTC2_fnSetTheme(INameSpaceTreeControl2* iface,
                                       LPCWSTR pszTheme)
{
    NSTC2Impl *This = (NSTC2Impl*)iface;
    FIXME("stub, %p (%p)\n", This, pszTheme);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTC2_fnGetNextItem(INameSpaceTreeControl2* iface,
                                          IShellItem *psi,
                                          NSTCGNI nstcgi,
                                          IShellItem **ppsiNext)
{
    NSTC2Impl *This = (NSTC2Impl*)iface;
    FIXME("stub, %p (%p, %x, %p)\n", This, psi, nstcgi, ppsiNext);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTC2_fnHitTest(INameSpaceTreeControl2* iface,
                                      POINT *ppt,
                                      IShellItem **ppsiOut)
{
    NSTC2Impl *This = (NSTC2Impl*)iface;
    FIXME("stub, %p (%p, %p)\n", This, ppsiOut, ppt);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTC2_fnGetItemRect(INameSpaceTreeControl2* iface,
                                          IShellItem *psi,
                                          RECT *prect)
{
    NSTC2Impl *This = (NSTC2Impl*)iface;
    FIXME("stub, %p (%p, %p)\n", This, psi, prect);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTC2_fnCollapseAll(INameSpaceTreeControl2* iface)
{
    NSTC2Impl *This = (NSTC2Impl*)iface;
    FIXME("stub, %p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTC2_fnSetControlStyle(INameSpaceTreeControl2* iface,
                                              NSTCSTYLE nstcsMask,
                                              NSTCSTYLE nstcsStyle)
{
    NSTC2Impl *This = (NSTC2Impl*)iface;
    FIXME("stub, %p (%x, %x)\n", This, nstcsMask, nstcsStyle);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTC2_fnGetControlStyle(INameSpaceTreeControl2* iface,
                                              NSTCSTYLE nstcsMask,
                                              NSTCSTYLE *pnstcsStyle)
{
    NSTC2Impl *This = (NSTC2Impl*)iface;
    FIXME("stub, %p (%x, %p)\n", This, nstcsMask, pnstcsStyle);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTC2_fnSetControlStyle2(INameSpaceTreeControl2* iface,
                                               NSTCSTYLE2 nstcsMask,
                                               NSTCSTYLE2 nstcsStyle)
{
    NSTC2Impl *This = (NSTC2Impl*)iface;
    FIXME("stub, %p (%x, %x)\n", This, nstcsMask, nstcsStyle);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTC2_fnGetControlStyle2(INameSpaceTreeControl2* iface,
                                               NSTCSTYLE2 nstcsMask,
                                               NSTCSTYLE2 *pnstcsStyle)
{
    NSTC2Impl *This = (NSTC2Impl*)iface;
    FIXME("stub, %p (%x, %p)\n", This, nstcsMask, pnstcsStyle);
    return E_NOTIMPL;
}

static const INameSpaceTreeControl2Vtbl vt_INameSpaceTreeControl2 = {
    NSTC2_fnQueryInterface,
    NSTC2_fnAddRef,
    NSTC2_fnRelease,
    NSTC2_fnInitialize,
    NSTC2_fnTreeAdvise,
    NSTC2_fnTreeUnadvise,
    NSTC2_fnAppendRoot,
    NSTC2_fnInsertRoot,
    NSTC2_fnRemoveRoot,
    NSTC2_fnRemoveAllRoots,
    NSTC2_fnGetRootItems,
    NSTC2_fnSetItemState,
    NSTC2_fnGetItemState,
    NSTC2_fnGetSelectedItems,
    NSTC2_fnGetItemCustomState,
    NSTC2_fnSetItemCustomState,
    NSTC2_fnEnsureItemVisible,
    NSTC2_fnSetTheme,
    NSTC2_fnGetNextItem,
    NSTC2_fnHitTest,
    NSTC2_fnGetItemRect,
    NSTC2_fnCollapseAll,
    NSTC2_fnSetControlStyle,
    NSTC2_fnGetControlStyle,
    NSTC2_fnSetControlStyle2,
    NSTC2_fnGetControlStyle2
};

/**************************************************************************
 * IOleWindow Implementation
 */

static inline NSTC2Impl *impl_from_IOleWindow(IOleWindow *iface)
{
    return (NSTC2Impl *)((char*)iface - FIELD_OFFSET(NSTC2Impl, lpowVtbl));
}

static HRESULT WINAPI IOW_fnQueryInterface(IOleWindow *iface, REFIID riid, void **ppvObject)
{
    NSTC2Impl *This = impl_from_IOleWindow(iface);
    TRACE("%p\n", This);
    return NSTC2_fnQueryInterface((INameSpaceTreeControl2*)This, riid, ppvObject);
}

static ULONG WINAPI IOW_fnAddRef(IOleWindow *iface)
{
    NSTC2Impl *This = impl_from_IOleWindow(iface);
    TRACE("%p\n", This);
    return NSTC2_fnAddRef((INameSpaceTreeControl2*)This);
}

static ULONG WINAPI IOW_fnRelease(IOleWindow *iface)
{
    NSTC2Impl *This = impl_from_IOleWindow(iface);
    TRACE("%p\n", This);
    return NSTC2_fnRelease((INameSpaceTreeControl2*)This);
}

static HRESULT WINAPI IOW_fnGetWindow(IOleWindow *iface, HWND *phwnd)
{
    NSTC2Impl *This = impl_from_IOleWindow(iface);
    TRACE("%p (%p)\n", This, phwnd);

    *phwnd = This->hwnd_main;
    return S_OK;
}

static HRESULT WINAPI IOW_fnContextSensitiveHelp(IOleWindow *iface, BOOL fEnterMode)
{
    NSTC2Impl *This = impl_from_IOleWindow(iface);
    TRACE("%p (%d)\n", This, fEnterMode);

    /* Not implemented */
    return E_NOTIMPL;
}

static const IOleWindowVtbl vt_IOleWindow = {
    IOW_fnQueryInterface,
    IOW_fnAddRef,
    IOW_fnRelease,
    IOW_fnGetWindow,
    IOW_fnContextSensitiveHelp
};

HRESULT NamespaceTreeControl_Constructor(IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    NSTC2Impl *nstc;
    HRESULT ret;

    TRACE ("%p %s %p\n", pUnkOuter, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    if(pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    EFRAME_LockModule();

    nstc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(NSTC2Impl));
    nstc->ref = 1;
    nstc->lpVtbl = &vt_INameSpaceTreeControl2;
    nstc->lpowVtbl = &vt_IOleWindow;

    ret = INameSpaceTreeControl_QueryInterface((INameSpaceTreeControl*)nstc, riid, ppv);
    INameSpaceTreeControl_Release((INameSpaceTreeControl*)nstc);

    TRACE("--(%p)\n", ppv);
    return ret;
}
