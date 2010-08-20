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

#include "wine/debug.h"
#include "debughlp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

typedef struct _ExplorerBrowserImpl {
    const IExplorerBrowserVtbl *lpVtbl;
    const IShellBrowserVtbl *lpsbVtbl;
    LONG ref;
    BOOL destroyed;
} ExplorerBrowserImpl;

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
    TRACE("%p (%p, %p, %p)\n", This, hwndParent, prc, pfs);

    return S_OK;
}

static HRESULT WINAPI IExplorerBrowser_fnDestroy(IExplorerBrowser *iface)
{
    ExplorerBrowserImpl *This = (ExplorerBrowserImpl*)iface;
    TRACE("%p\n", This);

    This->destroyed = TRUE;

    return S_OK;
}

static HRESULT WINAPI IExplorerBrowser_fnSetRect(IExplorerBrowser *iface,
                                                 HDWP *phdwp, RECT rcBrowser)
{
    ExplorerBrowserImpl *This = (ExplorerBrowserImpl*)iface;
    FIXME("stub, %p (%p, %s)\n", This, phdwp, wine_dbgstr_rect(&rcBrowser));

    return E_NOTIMPL;
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
    FIXME("stub, %p (%p)\n", This, pfs);

    return E_NOTIMPL;
}

static HRESULT WINAPI IExplorerBrowser_fnAdvise(IExplorerBrowser *iface,
                                                IExplorerBrowserEvents *psbe,
                                                DWORD *pdwCookie)
{
    ExplorerBrowserImpl *This = (ExplorerBrowserImpl*)iface;
    FIXME("stub, %p (%p, %p)\n", This, psbe, pdwCookie);

    return E_NOTIMPL;
}

static HRESULT WINAPI IExplorerBrowser_fnUnadvise(IExplorerBrowser *iface,
                                                  DWORD dwCookie)
{
    ExplorerBrowserImpl *This = (ExplorerBrowserImpl*)iface;
    FIXME("stub, %p (0x%x)\n", This, dwCookie);

    return E_NOTIMPL;
}

static HRESULT WINAPI IExplorerBrowser_fnSetOptions(IExplorerBrowser *iface,
                                                    EXPLORER_BROWSER_OPTIONS dwFlag)
{
    ExplorerBrowserImpl *This = (ExplorerBrowserImpl*)iface;
    FIXME("stub, %p (0x%x)\n", This, dwFlag);

    return E_NOTIMPL;
}

static HRESULT WINAPI IExplorerBrowser_fnGetOptions(IExplorerBrowser *iface,
                                                    EXPLORER_BROWSER_OPTIONS *pdwFlag)
{
    ExplorerBrowserImpl *This = (ExplorerBrowserImpl*)iface;
    FIXME("stub, %p (%p)\n", This, pdwFlag);

    return E_NOTIMPL;
}

static HRESULT WINAPI IExplorerBrowser_fnBrowseToIDList(IExplorerBrowser *iface,
                                                        PCUIDLIST_RELATIVE pidl,
                                                        UINT uFlags)
{
    ExplorerBrowserImpl *This = (ExplorerBrowserImpl*)iface;
    FIXME("stub, %p (%p, 0x%x)\n", This, pidl, uFlags);

    return E_NOTIMPL;
}

static HRESULT WINAPI IExplorerBrowser_fnBrowseToObject(IExplorerBrowser *iface,
                                                        IUnknown *punk, UINT uFlags)
{
    ExplorerBrowserImpl *This = (ExplorerBrowserImpl*)iface;
    FIXME("stub, %p (%p, 0x%x)\n", This, punk, uFlags);

    return E_NOTIMPL;
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
    FIXME("stub, %p (%s, %p)\n", This, shdebugstr_guid(riid), ppv);

    *ppv = NULL;
    return E_FAIL;
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
    FIXME("stub, %p (%p)\n", This, phwnd);

    return E_NOTIMPL;
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
    FIXME("stub, %p\n", This);

    return E_NOTIMPL;
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
    FIXME("stub, %p (%p)\n", This, ppshv);

    return E_NOTIMPL;
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

    ret = IExplorerBrowser_QueryInterface((IExplorerBrowser*)eb, riid, ppv);
    IExplorerBrowser_Release((IExplorerBrowser*)eb);

    TRACE("--(%p)\n", ppv);
    return ret;
}
