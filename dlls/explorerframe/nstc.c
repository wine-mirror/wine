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
    LONG ref;
} NSTC2Impl;

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
    FIXME("stub, %p (%p, %p, %x)\n", This, hwndParent, prc, nstcsFlags);
    return E_NOTIMPL;
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

    ret = INameSpaceTreeControl_QueryInterface((INameSpaceTreeControl*)nstc, riid, ppv);
    INameSpaceTreeControl_Release((INameSpaceTreeControl*)nstc);

    TRACE("--(%p)\n", ppv);
    return ret;
}
