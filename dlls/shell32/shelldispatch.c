/*
 * IShellDispatch implementation
 *
 * Copyright 2010 Alexander Morozov for Etersoft
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"

#include "shlobj.h"
#include "shldisp.h"
#include "debughlp.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

typedef struct {
    IShellDispatch IShellDispatch_iface;
    LONG ref;
} ShellDispatch;

static inline ShellDispatch *impl_from_IShellDispatch(IShellDispatch *iface)
{
    return CONTAINING_RECORD(iface, ShellDispatch, IShellDispatch_iface);
}

static HRESULT WINAPI ShellDispatch_QueryInterface(IShellDispatch *iface,
        REFIID riid, LPVOID *ppv)
{
    ShellDispatch *This = impl_from_IShellDispatch(iface);

    TRACE("(%p,%p,%p)\n", iface, riid, ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, riid) ||
        IsEqualIID(&IID_IDispatch, riid) ||
        IsEqualIID(&IID_IShellDispatch, riid))
        *ppv = This;
    else
    {
        FIXME("not implemented for %s\n", shdebugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ShellDispatch_AddRef(IShellDispatch *iface)
{
    ShellDispatch *This = impl_from_IShellDispatch(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p), new refcount=%i\n", iface, ref);

    return ref;
}

static ULONG WINAPI ShellDispatch_Release(IShellDispatch *iface)
{
    ShellDispatch *This = impl_from_IShellDispatch(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p), new refcount=%i\n", iface, ref);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);
    return ref;
}

static HRESULT WINAPI ShellDispatch_GetTypeInfoCount(IShellDispatch *iface,
        UINT *pctinfo)
{
    FIXME("(%p,%p)\n", iface, pctinfo);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_GetTypeInfo(IShellDispatch *iface,
        UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    FIXME("(%p,%u,%d,%p)\n", iface, iTInfo, lcid, ppTInfo);

    *ppTInfo = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_GetIDsOfNames(IShellDispatch *iface,
        REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    FIXME("(%p,%p,%p,%u,%d,%p)\n", iface, riid, rgszNames, cNames, lcid,
            rgDispId);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_Invoke(IShellDispatch *iface,
        DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
        DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo,
        UINT *puArgErr)
{
    FIXME("(%p,%d,%p,%d,%u,%p,%p,%p,%p)\n", iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_get_Application(IShellDispatch *iface,
        IDispatch **ppid)
{
    FIXME("(%p,%p)\n", iface, ppid);

    *ppid = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_get_Parent(IShellDispatch *iface,
        IDispatch **ppid)
{
    FIXME("(%p,%p)\n", iface, ppid);

    *ppid = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_NameSpace(IShellDispatch *iface,
        VARIANT vDir, Folder **ppsdf)
{
    FIXME("(%p,%p)\n", iface, ppsdf);

    *ppsdf = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_BrowseForFolder(IShellDispatch *iface,
        LONG Hwnd, BSTR Title, LONG Options, VARIANT RootFolder, Folder **ppsdf)
{
    FIXME("(%p,%x,%s,%x,%p)\n", iface, Hwnd, debugstr_w(Title), Options, ppsdf);

    *ppsdf = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_Windows(IShellDispatch *iface,
        IDispatch **ppid)
{
    FIXME("(%p,%p)\n", iface, ppid);

    *ppid = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_Open(IShellDispatch *iface, VARIANT vDir)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_Explore(IShellDispatch *iface, VARIANT vDir)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_MinimizeAll(IShellDispatch *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_UndoMinimizeALL(IShellDispatch *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_FileRun(IShellDispatch *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_CascadeWindows(IShellDispatch *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_TileVertically(IShellDispatch *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_TileHorizontally(IShellDispatch *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_ShutdownWindows(IShellDispatch *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_Suspend(IShellDispatch *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_EjectPC(IShellDispatch *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_SetTime(IShellDispatch *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_TrayProperties(IShellDispatch *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_Help(IShellDispatch *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_FindFiles(IShellDispatch *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_FindComputer(IShellDispatch *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_RefreshMenu(IShellDispatch *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellDispatch_ControlPanelItem(IShellDispatch *iface,
        BSTR szDir)
{
    FIXME("(%p,%s)\n", iface, debugstr_w(szDir));

    return E_NOTIMPL;
}

static const IShellDispatchVtbl ShellDispatch_Vtbl = {
    ShellDispatch_QueryInterface,
    ShellDispatch_AddRef,
    ShellDispatch_Release,
    ShellDispatch_GetTypeInfoCount,
    ShellDispatch_GetTypeInfo,
    ShellDispatch_GetIDsOfNames,
    ShellDispatch_Invoke,
    ShellDispatch_get_Application,
    ShellDispatch_get_Parent,
    ShellDispatch_NameSpace,
    ShellDispatch_BrowseForFolder,
    ShellDispatch_Windows,
    ShellDispatch_Open,
    ShellDispatch_Explore,
    ShellDispatch_MinimizeAll,
    ShellDispatch_UndoMinimizeALL,
    ShellDispatch_FileRun,
    ShellDispatch_CascadeWindows,
    ShellDispatch_TileVertically,
    ShellDispatch_TileHorizontally,
    ShellDispatch_ShutdownWindows,
    ShellDispatch_Suspend,
    ShellDispatch_EjectPC,
    ShellDispatch_SetTime,
    ShellDispatch_TrayProperties,
    ShellDispatch_Help,
    ShellDispatch_FindFiles,
    ShellDispatch_FindComputer,
    ShellDispatch_RefreshMenu,
    ShellDispatch_ControlPanelItem
};

HRESULT WINAPI IShellDispatch_Constructor(IUnknown *pUnkOuter, REFIID riid,
        LPVOID *ppv)
{
    ShellDispatch *This;
    HRESULT ret;

    TRACE("(%p,%s)\n", pUnkOuter, debugstr_guid(riid));

    *ppv = NULL;

    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(ShellDispatch));
    if (!This) return E_OUTOFMEMORY;
    This->IShellDispatch_iface.lpVtbl = &ShellDispatch_Vtbl;
    This->ref = 1;

    ret = ShellDispatch_QueryInterface(&This->IShellDispatch_iface, riid, ppv);
    ShellDispatch_Release(&This->IShellDispatch_iface);
    return ret;
}
