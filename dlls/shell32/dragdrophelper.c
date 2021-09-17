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
    IDragSourceHelper IDragSourceHelper_iface;
    LONG ref;
} dragdrophelper;

static inline dragdrophelper *impl_from_IDropTargetHelper(IDropTargetHelper *iface)
{
    return CONTAINING_RECORD(iface, dragdrophelper, IDropTargetHelper_iface);
}

static inline dragdrophelper *impl_from_IDragSourceHelper(IDragSourceHelper *iface)
{
    return CONTAINING_RECORD(iface, dragdrophelper, IDragSourceHelper_iface);
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
    else if (IsEqualIID (riid, &IID_IDragSourceHelper))
    {
        *ppvObj = &This->IDragSourceHelper_iface;
    }

    if (*ppvObj) {
	IUnknown_AddRef ((IUnknown *) (*ppvObj));
	TRACE ("-- Interface: (%p)->(%p)\n", ppvObj, *ppvObj);
	return S_OK;
    }
    FIXME ("-- Interface: E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI IDropTargetHelper_fnAddRef (IDropTargetHelper * iface)
{
    dragdrophelper *This = impl_from_IDropTargetHelper(iface);
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE ("(%p)->(count=%u)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IDropTargetHelper_fnRelease (IDropTargetHelper * iface)
{
    dragdrophelper *This = impl_from_IDropTargetHelper(iface);
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE ("(%p)->(count=%u)\n", This, refCount + 1);

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
    FIXME ("(%p)->(%p %p %p 0x%08x)\n", This,hwndTarget, pDataObject, ppt, dwEffect);
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
    FIXME ("(%p)->(%p 0x%08x)\n", This, ppt, dwEffect);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDropTargetHelper_fnDrop (IDropTargetHelper * iface, IDataObject* pDataObject, POINT* ppt, DWORD dwEffect)
{
    dragdrophelper *This = impl_from_IDropTargetHelper(iface);
    FIXME ("(%p)->(%p %p 0x%08x)\n", This, pDataObject, ppt, dwEffect);
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

static HRESULT WINAPI DragSourceHelper_QueryInterface (IDragSourceHelper * iface, REFIID riid, LPVOID * ppv)
{
    dragdrophelper *This = impl_from_IDragSourceHelper(iface);
    return IDropTargetHelper_fnQueryInterface(&This->IDropTargetHelper_iface, riid, ppv);
}

static ULONG WINAPI DragSourceHelper_AddRef (IDragSourceHelper * iface)
{
    dragdrophelper *This = impl_from_IDragSourceHelper(iface);
    return IDropTargetHelper_fnAddRef(&This->IDropTargetHelper_iface);
}

static ULONG WINAPI DragSourceHelper_Release (IDragSourceHelper * iface)
{
    dragdrophelper *This = impl_from_IDragSourceHelper(iface);
    return IDropTargetHelper_fnRelease(&This->IDropTargetHelper_iface);
}

static HRESULT WINAPI DragSourceHelper_InitializeFromBitmap(IDragSourceHelper *iface,
    SHDRAGIMAGE *dragimage, IDataObject *object)
{
    dragdrophelper *This = impl_from_IDragSourceHelper(iface);

    FIXME("(%p)->(%p, %p): stub\n", This, dragimage, object);

    return E_NOTIMPL;
}

static HRESULT WINAPI DragSourceHelper_InitializeFromWindow(IDragSourceHelper *iface, HWND hwnd,
    POINT *pt, IDataObject *object)
{
    dragdrophelper *This = impl_from_IDragSourceHelper(iface);

    FIXME("(%p)->(%p, %s, %p): stub\n", This, hwnd, wine_dbgstr_point(pt), object);

    return E_NOTIMPL;
}

static const IDragSourceHelperVtbl DragSourceHelperVtbl =
{
    DragSourceHelper_QueryInterface,
    DragSourceHelper_AddRef,
    DragSourceHelper_Release,
    DragSourceHelper_InitializeFromBitmap,
    DragSourceHelper_InitializeFromWindow
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
    dth->IDragSourceHelper_iface.lpVtbl = &DragSourceHelperVtbl;
    dth->ref = 1;

    hr = IDropTargetHelper_QueryInterface (&dth->IDropTargetHelper_iface, riid, ppv);
    IDropTargetHelper_Release (&dth->IDropTargetHelper_iface);

    return hr;
}
