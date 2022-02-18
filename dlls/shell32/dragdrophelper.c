/*
 *	file system folder
 *
 *	Copyright 1997			Marcus Meissner
 *	Copyright 1998, 1999, 2002	Juergen Schmied
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
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"

#include "objbase.h"
#include "ole2.h"
#include "shlguid.h"
#include "shlobj.h"

#include "wine/debug.h"
#include "debughlp.h"

WINE_DEFAULT_DEBUG_CHANNEL (shell);

/***********************************************************************
*   IDropTargetHelper implementation
*/

typedef struct
{
    IDropTargetHelper IDropTargetHelper_iface;
    IDragSourceHelper2 IDragSourceHelper2_iface;
    LONG ref;
} dragdrophelper;

static inline dragdrophelper *impl_from_IDropTargetHelper(IDropTargetHelper *iface)
{
    return CONTAINING_RECORD(iface, dragdrophelper, IDropTargetHelper_iface);
}

static inline dragdrophelper *impl_from_IDragSourceHelper2(IDragSourceHelper2 *iface)
{
    return CONTAINING_RECORD(iface, dragdrophelper, IDragSourceHelper2_iface);
}

/**************************************************************************
 *	IDropTargetHelper_fnQueryInterface
 */
static HRESULT WINAPI IDropTargetHelper_fnQueryInterface (IDropTargetHelper * iface, REFIID riid, LPVOID * ppvObj)
{
    dragdrophelper *This = impl_from_IDropTargetHelper(iface);

    TRACE ("(%p)->(%s,%p)\n", This, shdebugstr_guid (riid), ppvObj);

    *ppvObj = NULL;

    if (IsEqualIID (riid, &IID_IUnknown) || IsEqualIID (riid, &IID_IDropTargetHelper))
    {
        *ppvObj = &This->IDropTargetHelper_iface;
    }
    else if (IsEqualIID (riid, &IID_IDragSourceHelper) || IsEqualIID (riid, &IID_IDragSourceHelper2))
    {
        *ppvObj = &This->IDragSourceHelper2_iface;
    }

    if (*ppvObj) {
	IUnknown_AddRef ((IUnknown *) (*ppvObj));
	TRACE ("-- Interface: (%p)->(%p)\n", ppvObj, *ppvObj);
	return S_OK;
    }
    FIXME ("%s: E_NOINTERFACE\n", shdebugstr_guid (riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI IDropTargetHelper_fnAddRef (IDropTargetHelper * iface)
{
    dragdrophelper *This = impl_from_IDropTargetHelper(iface);
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE ("(%p)->(count=%lu)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IDropTargetHelper_fnRelease (IDropTargetHelper * iface)
{
    dragdrophelper *This = impl_from_IDropTargetHelper(iface);
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE ("(%p)->(count=%lu)\n", This, refCount + 1);

    if (!refCount) {
        TRACE ("-- destroying (%p)\n", This);
        LocalFree (This);
        return 0;
    }
    return refCount;
}

static HRESULT WINAPI IDropTargetHelper_fnDragEnter (
        IDropTargetHelper * iface,
	HWND hwndTarget,
	IDataObject* pDataObject,
	POINT* ppt,
	DWORD dwEffect)
{
    dragdrophelper *This = impl_from_IDropTargetHelper(iface);
    FIXME ("(%p)->(%p %p %p 0x%08lx)\n", This,hwndTarget, pDataObject, ppt, dwEffect);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDropTargetHelper_fnDragLeave (IDropTargetHelper * iface)
{
    dragdrophelper *This = impl_from_IDropTargetHelper(iface);
    FIXME ("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDropTargetHelper_fnDragOver (IDropTargetHelper * iface, POINT* ppt, DWORD dwEffect)
{
    dragdrophelper *This = impl_from_IDropTargetHelper(iface);
    FIXME ("(%p)->(%p 0x%08lx)\n", This, ppt, dwEffect);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDropTargetHelper_fnDrop (IDropTargetHelper * iface, IDataObject* pDataObject, POINT* ppt, DWORD dwEffect)
{
    dragdrophelper *This = impl_from_IDropTargetHelper(iface);
    FIXME ("(%p)->(%p %p 0x%08lx)\n", This, pDataObject, ppt, dwEffect);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDropTargetHelper_fnShow (IDropTargetHelper * iface, BOOL fShow)
{
    dragdrophelper *This = impl_from_IDropTargetHelper(iface);
    FIXME ("(%p)->(%u)\n", This, fShow);
    return S_OK;
}

static const IDropTargetHelperVtbl DropTargetHelperVtbl =
{
    IDropTargetHelper_fnQueryInterface,
    IDropTargetHelper_fnAddRef,
    IDropTargetHelper_fnRelease,
    IDropTargetHelper_fnDragEnter,
    IDropTargetHelper_fnDragLeave,
    IDropTargetHelper_fnDragOver,
    IDropTargetHelper_fnDrop,
    IDropTargetHelper_fnShow
};

static HRESULT WINAPI DragSourceHelper2_QueryInterface (IDragSourceHelper2 * iface, REFIID riid, LPVOID * ppv)
{
    dragdrophelper *This = impl_from_IDragSourceHelper2(iface);
    return IDropTargetHelper_fnQueryInterface(&This->IDropTargetHelper_iface, riid, ppv);
}

static ULONG WINAPI DragSourceHelper2_AddRef (IDragSourceHelper2 * iface)
{
    dragdrophelper *This = impl_from_IDragSourceHelper2(iface);
    return IDropTargetHelper_fnAddRef(&This->IDropTargetHelper_iface);
}

static ULONG WINAPI DragSourceHelper2_Release (IDragSourceHelper2 * iface)
{
    dragdrophelper *This = impl_from_IDragSourceHelper2(iface);
    return IDropTargetHelper_fnRelease(&This->IDropTargetHelper_iface);
}

static HRESULT WINAPI DragSourceHelper2_InitializeFromBitmap(IDragSourceHelper2 *iface,
    SHDRAGIMAGE *dragimage, IDataObject *object)
{
    dragdrophelper *This = impl_from_IDragSourceHelper2(iface);

    FIXME("(%p)->(%p, %p): stub\n", This, dragimage, object);

    return E_NOTIMPL;
}

static HRESULT WINAPI DragSourceHelper2_InitializeFromWindow(IDragSourceHelper2 *iface, HWND hwnd,
    POINT *pt, IDataObject *object)
{
    dragdrophelper *This = impl_from_IDragSourceHelper2(iface);

    FIXME("(%p)->(%p, %s, %p): stub\n", This, hwnd, wine_dbgstr_point(pt), object);

    return E_NOTIMPL;
}

static HRESULT WINAPI DragSourceHelper2_SetFlags(IDragSourceHelper2 *iface, DWORD flags)
{
    dragdrophelper *This = impl_from_IDragSourceHelper2(iface);

    FIXME("(%p)->(%08lx): stub\n", This, flags);

    return S_OK;
}

static const IDragSourceHelper2Vtbl DragSourceHelper2Vtbl =
{
    DragSourceHelper2_QueryInterface,
    DragSourceHelper2_AddRef,
    DragSourceHelper2_Release,
    DragSourceHelper2_InitializeFromBitmap,
    DragSourceHelper2_InitializeFromWindow,
    DragSourceHelper2_SetFlags
};

/**************************************************************************
*	IDropTargetHelper_Constructor
*/
HRESULT WINAPI IDropTargetHelper_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv)
{
    dragdrophelper *dth;
    HRESULT hr;

    TRACE ("outer=%p %s %p\n", pUnkOuter, shdebugstr_guid (riid), ppv);

    if (!ppv)
	return E_POINTER;
    if (pUnkOuter)
	return CLASS_E_NOAGGREGATION;

    dth = LocalAlloc (LMEM_ZEROINIT, sizeof (dragdrophelper));
    if (!dth) return E_OUTOFMEMORY;

    dth->IDropTargetHelper_iface.lpVtbl = &DropTargetHelperVtbl;
    dth->IDragSourceHelper2_iface.lpVtbl = &DragSourceHelper2Vtbl;
    dth->ref = 1;

    hr = IDropTargetHelper_QueryInterface (&dth->IDropTargetHelper_iface, riid, ppv);
    IDropTargetHelper_Release (&dth->IDropTargetHelper_iface);

    return hr;
}
