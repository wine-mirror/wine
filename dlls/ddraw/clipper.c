/* DirectDrawClipper implementation
 *
 * Copyright 2000 (c) Marcus Meissner
 * Copyright 2000 (c) TransGaming Technologies Inc.
 * Copyright 2006 (c) Stefan Dösinger
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

#include "ddraw_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

static inline IDirectDrawClipperImpl *impl_from_IDirectDrawClipper(IDirectDrawClipper *iface)
{
    return CONTAINING_RECORD(iface, IDirectDrawClipperImpl, IDirectDrawClipper_iface);
}

/*****************************************************************************
 * IDirectDrawClipper::QueryInterface
 *
 * Can query the IUnknown and IDirectDrawClipper interface from a
 * Clipper object. The IUnknown Interface is equal to the IDirectDrawClipper
 * interface. Can't create other interfaces.
 *
 * Arguments:
 *  riid: Interface id asked for
 *  ppvObj: Returns the pointer to the interface
 *
 * Return values:
 *  DD_OK on success
 *  E_NOINTERFACE if the requested interface wasn't found.
 *
 *****************************************************************************/
static HRESULT WINAPI IDirectDrawClipperImpl_QueryInterface(IDirectDrawClipper *iface, REFIID riid,
        void **ppvObj)
{

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), ppvObj);

    if (IsEqualGUID(&IID_IDirectDrawClipper, riid)
            || IsEqualGUID(&IID_IUnknown, riid))
    {
        IUnknown_AddRef(iface);
        *ppvObj = iface;
        return S_OK;
    }

    return E_NOINTERFACE;
}

/*****************************************************************************
 * IDirectDrawClipper::AddRef
 *
 * Increases the reference count of the interface, returns the new count
 *
 *****************************************************************************/
static ULONG WINAPI IDirectDrawClipperImpl_AddRef(IDirectDrawClipper *iface)
{
    IDirectDrawClipperImpl *This = impl_from_IDirectDrawClipper(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %u.\n", This, ref);

    return ref;
}

/*****************************************************************************
 * IDirectDrawClipper::Release
 *
 * Decreases the reference count of the interface, returns the new count
 * If the refcount is decreased to 0, the interface is destroyed.
 *
 *****************************************************************************/
static ULONG WINAPI IDirectDrawClipperImpl_Release(IDirectDrawClipper *iface)
{
    IDirectDrawClipperImpl *This = impl_from_IDirectDrawClipper(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %u.\n", This, ref);

    if (ref == 0)
    {
        EnterCriticalSection(&ddraw_cs);
        wined3d_clipper_decref(This->wineD3DClipper);
        HeapFree(GetProcessHeap(), 0, This);
        LeaveCriticalSection(&ddraw_cs);
        return 0;
    }
    else return ref;
}

/*****************************************************************************
 * IDirectDrawClipper::SetHwnd
 *
 * Assigns a hWnd to the clipper interface.
 *
 * Arguments:
 *  Flags: Unsupported so far
 *  hWnd: The hWnd to set
 *
 * Return values:
 *  DD_OK on success
 *  DDERR_INVALIDPARAMS if Flags was != 0
 *
 *****************************************************************************/

static HRESULT WINAPI IDirectDrawClipperImpl_SetHwnd(IDirectDrawClipper *iface, DWORD dwFlags,
        HWND hWnd)
{
    IDirectDrawClipperImpl *This = impl_from_IDirectDrawClipper(iface);
    HRESULT hr;

    TRACE("iface %p, flags %#x, window %p.\n", iface, dwFlags, hWnd);

    EnterCriticalSection(&ddraw_cs);
    hr = wined3d_clipper_set_window(This->wineD3DClipper, dwFlags, hWnd);
    LeaveCriticalSection(&ddraw_cs);
    switch(hr)
    {
        case WINED3DERR_INVALIDCALL:        return DDERR_INVALIDPARAMS;
        default:                            return hr;
    }
}

/*****************************************************************************
 * IDirectDrawClipper::GetClipList
 *
 * Retrieve a copy of the clip list
 *
 * Arguments:
 *  Rect: Rectangle to be used to clip the clip list or NULL for the
 *        entire clip list
 *  ClipList: structure for the resulting copy of the clip list.
 *            If NULL, fills Size up to the number of bytes necessary to hold
 *            the entire clip.
 *  Size: Size of resulting clip list; size of the buffer at ClipList
 *        or, if ClipList is NULL, receives the required size of the buffer
 *        in bytes
 *
 * RETURNS
 *  Either DD_OK or DDERR_*
 ************************************************************************/
static HRESULT WINAPI IDirectDrawClipperImpl_GetClipList(IDirectDrawClipper *iface, RECT *lpRect,
        RGNDATA *lpClipList, DWORD *lpdwSize)
{
    IDirectDrawClipperImpl *This = impl_from_IDirectDrawClipper(iface);
    HRESULT hr;

    TRACE("iface %p, rect %s, clip_list %p, clip_list_size %p.\n",
            iface, wine_dbgstr_rect(lpRect), lpClipList, lpdwSize);

    EnterCriticalSection(&ddraw_cs);
    hr = wined3d_clipper_get_clip_list(This->wineD3DClipper, lpRect, lpClipList, lpdwSize);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

/*****************************************************************************
 * IDirectDrawClipper::SetClipList
 *
 * Sets or deletes (if lprgn is NULL) the clip list
 *
 * This implementation is a stub and returns DD_OK always to make the app
 * happy.
 *
 * PARAMS
 *  lprgn   Pointer to a LRGNDATA structure or NULL
 *  dwFlags not used, must be 0
 * RETURNS
 *  Either DD_OK or DDERR_*
 *****************************************************************************/
static HRESULT WINAPI IDirectDrawClipperImpl_SetClipList(IDirectDrawClipper *iface, RGNDATA *lprgn,
        DWORD dwFlag)
{
    IDirectDrawClipperImpl *This = impl_from_IDirectDrawClipper(iface);
    HRESULT hr;

    TRACE("iface %p, clip_list %p, flags %#x.\n", iface, lprgn, dwFlag);

    EnterCriticalSection(&ddraw_cs);
    hr = wined3d_clipper_set_clip_list(This->wineD3DClipper, lprgn, dwFlag);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

/*****************************************************************************
 * IDirectDrawClipper::GetHwnd
 *
 * Returns the hwnd assigned with SetHwnd
 *
 * Arguments:
 *  hWndPtr: Address to store the HWND at
 *
 * Return values:
 *  Always returns DD_OK;
 *****************************************************************************/
static HRESULT WINAPI IDirectDrawClipperImpl_GetHWnd(IDirectDrawClipper *iface, HWND *hWndPtr)
{
    IDirectDrawClipperImpl *This = impl_from_IDirectDrawClipper(iface);
    HRESULT hr;

    TRACE("iface %p, window %p.\n", iface, hWndPtr);

    EnterCriticalSection(&ddraw_cs);
    hr = wined3d_clipper_get_window(This->wineD3DClipper, hWndPtr);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

/*****************************************************************************
 * IDirectDrawClipper::Initialize
 *
 * Initializes the interface. Well, there isn't much to do for this
 * implementation, but it stores the DirectDraw Interface.
 *
 * Arguments:
 *  DD: Pointer to a IDirectDraw interface
 *  Flags: Unsupported by now
 *
 * Return values:
 *  DD_OK on success
 *  DDERR_ALREADYINITIALIZED if this interface isn't initialized already
 *****************************************************************************/
static HRESULT WINAPI IDirectDrawClipperImpl_Initialize(IDirectDrawClipper *iface,
        IDirectDraw *ddraw, DWORD dwFlags)
{
    IDirectDrawClipperImpl *This = impl_from_IDirectDrawClipper(iface);

    TRACE("iface %p, ddraw %p, flags %#x.\n", iface, ddraw, dwFlags);

    EnterCriticalSection(&ddraw_cs);
    if (This->initialized)
    {
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_ALREADYINITIALIZED;
    }

    This->initialized = TRUE;

    LeaveCriticalSection(&ddraw_cs);
    return DD_OK;
}

/*****************************************************************************
 * IDirectDrawClipper::IsClipListChanged
 *
 * This function is a stub
 *
 * Arguments:
 *  Changed:
 *
 * Return values:
 *  DD_OK, because it's a stub
 *****************************************************************************/
static HRESULT WINAPI IDirectDrawClipperImpl_IsClipListChanged(IDirectDrawClipper *iface,
        BOOL *lpbChanged)
{
    FIXME("iface %p, changed %p stub!\n", iface, lpbChanged);

    /* XXX What is safest? */
    *lpbChanged = FALSE;

    return DD_OK;
}

/*****************************************************************************
 * The VTable
 *****************************************************************************/
static const struct IDirectDrawClipperVtbl ddraw_clipper_vtbl =
{
    IDirectDrawClipperImpl_QueryInterface,
    IDirectDrawClipperImpl_AddRef,
    IDirectDrawClipperImpl_Release,
    IDirectDrawClipperImpl_GetClipList,
    IDirectDrawClipperImpl_GetHWnd,
    IDirectDrawClipperImpl_Initialize,
    IDirectDrawClipperImpl_IsClipListChanged,
    IDirectDrawClipperImpl_SetClipList,
    IDirectDrawClipperImpl_SetHwnd
};

HRESULT ddraw_clipper_init(IDirectDrawClipperImpl *clipper)
{
    clipper->IDirectDrawClipper_iface.lpVtbl = &ddraw_clipper_vtbl;
    clipper->ref = 1;
    clipper->wineD3DClipper = wined3d_clipper_create();
    if (!clipper->wineD3DClipper)
    {
        WARN("Failed to create wined3d clipper.\n");
        return E_OUTOFMEMORY;
    }

    return DD_OK;
}

IDirectDrawClipperImpl *unsafe_impl_from_IDirectDrawClipper(IDirectDrawClipper *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &ddraw_clipper_vtbl);

    return impl_from_IDirectDrawClipper(iface);
}
