/*
 * Implementation of IShellBrowser interface
 *
 * Copyright 2011 Piotr Caban for CodeWeavers
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

#include "wine/debug.h"
#include "shdocvw.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

typedef struct {
    IShellBrowser IShellBrowser_iface;

    LONG ref;
} ShellBrowser;

static inline ShellBrowser *impl_from_IShellBrowser(IShellBrowser *iface)
{
    return CONTAINING_RECORD(iface, ShellBrowser, IShellBrowser_iface);
}

static HRESULT WINAPI ShellBrowser_QueryInterface(
        IShellBrowser* iface,
        REFIID riid,
        void **ppvObject)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    *ppvObject = NULL;

    if(IsEqualGUID(&IID_IShellBrowser, riid) || IsEqualGUID(&IID_IOleWindow, riid)
        || IsEqualGUID(&IID_IUnknown, riid))
        *ppvObject = &This->IShellBrowser_iface;

    if(*ppvObject) {
        TRACE("%p %s %p\n", This, debugstr_guid(riid), ppvObject);
        IUnknown_AddRef((IUnknown*)*ppvObject);
        return S_OK;
    }

    FIXME("%p %s %p\n", This, debugstr_guid(riid), ppvObject);
    return E_NOINTERFACE;
}

static ULONG WINAPI ShellBrowser_AddRef(
        IShellBrowser* iface)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI ShellBrowser_Release(
        IShellBrowser* iface)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
        heap_free(This);
    return ref;
}

static HRESULT WINAPI ShellBrowser_GetWindow(
        IShellBrowser* iface,
        HWND *phwnd)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    FIXME("%p %p\n", This, phwnd);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_ContextSensitiveHelp(
        IShellBrowser* iface,
        BOOL fEnterMode)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    FIXME("%p %d\n", This, fEnterMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_InsertMenusSB(
        IShellBrowser* iface,
        HMENU hmenuShared,
        LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    FIXME("%p %p %p\n", This, hmenuShared, lpMenuWidths);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_SetMenuSB(
        IShellBrowser* iface,
        HMENU hmenuShared,
        HOLEMENU holemenuReserved,
        HWND hwndActiveObject)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    FIXME("%p %p %p %p\n", This, hmenuShared, holemenuReserved, hwndActiveObject);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_RemoveMenusSB(
        IShellBrowser* iface,
        HMENU hmenuShared)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    FIXME("%p %p\n", This, hmenuShared);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_SetStatusTextSB(
        IShellBrowser* iface,
        LPCOLESTR pszStatusText)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    FIXME("%p %s\n", This, debugstr_w(pszStatusText));
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_EnableModelessSB(
        IShellBrowser* iface,
        BOOL fEnable)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    FIXME("%p %d\n", This, fEnable);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_TranslateAcceleratorSB(
        IShellBrowser* iface,
        MSG *pmsg,
        WORD wID)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    FIXME("%p %p %d\n", This, pmsg, (int)wID);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_BrowseObject(
        IShellBrowser* iface,
        LPCITEMIDLIST pidl,
        UINT wFlags)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    FIXME("%p %p %u\n", This, pidl, wFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_GetViewStateStream(
        IShellBrowser* iface,
        DWORD grfMode,
        IStream **ppStrm)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    FIXME("%p %x %p\n", This, grfMode, ppStrm);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_GetControlWindow(
        IShellBrowser* iface,
        UINT id,
        HWND *phwnd)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    FIXME("%p %u %p\n", This, id, phwnd);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_SendControlMsg(
        IShellBrowser* iface,
        UINT id,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam,
        LRESULT *pret)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    FIXME("%p %u %u %p\n", This, id, uMsg, pret);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_QueryActiveShellView(
        IShellBrowser* iface,
        IShellView **ppshv)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    FIXME("%p %p\n", This, ppshv);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_OnViewWindowActive(
        IShellBrowser* iface,
        IShellView *pshv)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    FIXME("%p %p\n", This, pshv);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_SetToolbarItems(
        IShellBrowser* iface,
        LPTBBUTTONSB lpButtons,
        UINT nButtons,
        UINT uFlags)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    FIXME("%p %p %u %u\n", This, lpButtons, nButtons, uFlags);
    return E_NOTIMPL;
}

static const IShellBrowserVtbl ShellBrowserVtbl = {
    ShellBrowser_QueryInterface,
    ShellBrowser_AddRef,
    ShellBrowser_Release,
    ShellBrowser_GetWindow,
    ShellBrowser_ContextSensitiveHelp,
    ShellBrowser_InsertMenusSB,
    ShellBrowser_SetMenuSB,
    ShellBrowser_RemoveMenusSB,
    ShellBrowser_SetStatusTextSB,
    ShellBrowser_EnableModelessSB,
    ShellBrowser_TranslateAcceleratorSB,
    ShellBrowser_BrowseObject,
    ShellBrowser_GetViewStateStream,
    ShellBrowser_GetControlWindow,
    ShellBrowser_SendControlMsg,
    ShellBrowser_QueryActiveShellView,
    ShellBrowser_OnViewWindowActive,
    ShellBrowser_SetToolbarItems
};

HRESULT ShellBrowser_Create(IShellBrowser **ppv)
{
    ShellBrowser *sb = heap_alloc(sizeof(ShellBrowser));
    if(!*ppv)
        return E_OUTOFMEMORY;

    sb->IShellBrowser_iface.lpVtbl = &ShellBrowserVtbl;

    sb->ref = 1;

    *ppv = &sb->IShellBrowser_iface;
    return S_OK;
}
