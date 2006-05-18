/*		DirectDrawClipper implementation
 *
 * Copyright 2000 Marcus Meissner
 * Copyright 2000 TransGaming Technologies Inc.
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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "ddraw.h"
#include "winerror.h"

#include "ddraw_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

/******************************************************************************
 *			DirectDrawCreateClipper (DDRAW.@)
 */

static const IDirectDrawClipperVtbl DDRAW_Clipper_VTable;

HRESULT WINAPI DirectDrawCreateClipper(
    DWORD dwFlags, LPDIRECTDRAWCLIPPER *lplpDDClipper, LPUNKNOWN pUnkOuter
) {
    IDirectDrawClipperImpl* This;
    TRACE("(%08lx,%p,%p)\n", dwFlags, lplpDDClipper, pUnkOuter);

    if (pUnkOuter != NULL) return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		     sizeof(IDirectDrawClipperImpl));
    if (This == NULL) return E_OUTOFMEMORY;

    ICOM_INIT_INTERFACE(This, IDirectDrawClipper, DDRAW_Clipper_VTable);
    This->ref = 1;
    This->hWnd = 0;
    This->ddraw_owner = NULL;

    *lplpDDClipper = ICOM_INTERFACE(This, IDirectDrawClipper);
    return DD_OK;
}

/* This is the classfactory implementation. */
HRESULT DDRAW_CreateDirectDrawClipper(IUnknown* pUnkOuter, REFIID riid,
				      LPVOID* ppObj)
{
    HRESULT hr;
    LPDIRECTDRAWCLIPPER pClip;

    hr = DirectDrawCreateClipper(0, &pClip, pUnkOuter);
    if (FAILED(hr)) return hr;

    hr = IDirectDrawClipper_QueryInterface(pClip, riid, ppObj);
    IDirectDrawClipper_Release(pClip);
    return hr;
}

/******************************************************************************
 *			IDirectDrawClipper
 */
HRESULT WINAPI Main_DirectDrawClipper_SetHwnd(
    LPDIRECTDRAWCLIPPER iface, DWORD dwFlags, HWND hWnd
) {
    IDirectDrawClipperImpl *This = (IDirectDrawClipperImpl *)iface;

    TRACE("(%p)->(0x%08lx,0x%08lx)\n", This, dwFlags, (DWORD)hWnd);
    if( dwFlags ) {
	FIXME("dwFlags = 0x%08lx, not supported.\n",dwFlags);
	return DDERR_INVALIDPARAMS;
    }

    This->hWnd = hWnd;
    return DD_OK;
}

static void Main_DirectDrawClipper_Destroy(IDirectDrawClipperImpl* This)
{
    if (This->ddraw_owner != NULL)
	Main_DirectDraw_RemoveClipper(This->ddraw_owner, This);

    HeapFree(GetProcessHeap(), 0 ,This);
}

void Main_DirectDrawClipper_ForceDestroy(IDirectDrawClipperImpl* This)
{
    WARN("deleting clipper %p with refcnt %lu\n", This, This->ref);
    Main_DirectDrawClipper_Destroy(This);
}

ULONG WINAPI Main_DirectDrawClipper_Release(LPDIRECTDRAWCLIPPER iface) {
    IDirectDrawClipperImpl *This = (IDirectDrawClipperImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->() decrementing from %lu.\n", This, ref + 1);

    if (ref == 0)
    {
	Main_DirectDrawClipper_Destroy(This);
	return 0;
    }
    else return ref;
}

/***********************************************************************
*           IDirectDrawClipper::GetClipList
*
* Retrieve a copy of the clip list
*
* PARAMS
*  lpRect  Rectangle to be used to clip the clip list or NULL for the
*          entire clip list
*  lpClipList structure for the resulting copy of the clip list.
           If NULL, fills lpdwSize up to the number of bytes necessary to hold
           the entire clip.
*  lpdwSize Size of resulting clip list; size of the buffer at lpClipList
           or, if lpClipList is NULL, receives the required size of the buffer
           in bytes
* RETURNS
*  Either DD_OK or DDERR_*
*/
HRESULT WINAPI Main_DirectDrawClipper_GetClipList(
    LPDIRECTDRAWCLIPPER iface, LPRECT lpRect, LPRGNDATA lpClipList,
    LPDWORD lpdwSize)
{
    IDirectDrawClipperImpl *This = (IDirectDrawClipperImpl *)iface;

    TRACE("(%p,%p,%p,%p)\n", This, lpRect, lpClipList, lpdwSize);

    if (This->hWnd)
    {
        HDC hDC = GetDCEx(This->hWnd, NULL, DCX_WINDOW);
        if (hDC)
        {
            HRGN hRgn = CreateRectRgn(0,0,0,0);
            if (GetRandomRgn(hDC, hRgn, SYSRGN))
            {
                if (GetVersion() & 0x80000000)
                {
                    /* map region to screen coordinates */
                    POINT org;
                    GetDCOrgEx( hDC, &org );
                    OffsetRgn( hRgn, org.x, org.y );
                }
                if (lpRect)
                {
                    HRGN hRgnClip = CreateRectRgn(lpRect->left, lpRect->top,
                        lpRect->right, lpRect->bottom);
                    CombineRgn(hRgn, hRgn, hRgnClip, RGN_AND);
                    DeleteObject(hRgnClip);
                }
                *lpdwSize = GetRegionData(hRgn, *lpdwSize, lpClipList);
            }
            DeleteObject(hRgn);
            ReleaseDC(This->hWnd, hDC);
        }
        return DD_OK;
    }
    else
    {
        static int warned = 0;
        if (warned++ < 10)
            FIXME("(%p,%p,%p,%p),stub!\n",This,lpRect,lpClipList,lpdwSize);
        if (lpdwSize) *lpdwSize=0;
        return DDERR_NOCLIPLIST;
    }
}

/***********************************************************************
*           IDirectDrawClipper::SetClipList
*
* Sets or deletes (if lprgn is NULL) the clip list
*
* PARAMS
*  lprgn   Pointer to a LRGNDATA structure or NULL
*  dwFlags not used, must be 0
* RETURNS
*  Either DD_OK or DDERR_*
*/
HRESULT WINAPI Main_DirectDrawClipper_SetClipList(
    LPDIRECTDRAWCLIPPER iface,LPRGNDATA lprgn,DWORD dwFlag
) {
    IDirectDrawClipperImpl *This = (IDirectDrawClipperImpl *)iface;
    static int warned = 0;
    if (warned++ < 10 || lprgn == NULL)
        FIXME("(%p,%p,%ld),stub!\n",This,lprgn,dwFlag);
    return DD_OK;
}

HRESULT WINAPI Main_DirectDrawClipper_QueryInterface(
    LPDIRECTDRAWCLIPPER iface, REFIID riid, LPVOID* ppvObj
) {
    IDirectDrawClipperImpl *This = (IDirectDrawClipperImpl *)iface;

    if (IsEqualGUID(&IID_IUnknown, riid)
	|| IsEqualGUID(&IID_IDirectDrawClipper, riid))
    {
	*ppvObj = ICOM_INTERFACE(This, IDirectDrawClipper);
	InterlockedIncrement(&This->ref);
	return S_OK;
    }
    else
    {
	return E_NOINTERFACE;
    }
}

ULONG WINAPI Main_DirectDrawClipper_AddRef( LPDIRECTDRAWCLIPPER iface )
{
    IDirectDrawClipperImpl *This = (IDirectDrawClipperImpl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->() incrementing from %lu.\n", This, ref - 1);

    return ref;
}

HRESULT WINAPI Main_DirectDrawClipper_GetHWnd(
    LPDIRECTDRAWCLIPPER iface, HWND* hWndPtr
) {
    IDirectDrawClipperImpl *This = (IDirectDrawClipperImpl *)iface;
    TRACE("(%p)->(%p)\n", This, hWndPtr);

    *hWndPtr = This->hWnd;

    return DD_OK;
}

HRESULT WINAPI Main_DirectDrawClipper_Initialize(
     LPDIRECTDRAWCLIPPER iface, LPDIRECTDRAW lpDD, DWORD dwFlags
) {
    IDirectDrawImpl* pOwner;
    IDirectDrawClipperImpl *This = (IDirectDrawClipperImpl *)iface;
    TRACE("(%p)->(%p,0x%08lx)\n", This, lpDD, dwFlags);

    if (This->ddraw_owner != NULL) return DDERR_ALREADYINITIALIZED;

    pOwner = ICOM_OBJECT(IDirectDrawImpl, IDirectDraw, lpDD);
    This->ddraw_owner = pOwner;
    Main_DirectDraw_AddClipper(pOwner, This);

    return DD_OK;
}

HRESULT WINAPI Main_DirectDrawClipper_IsClipListChanged(
    LPDIRECTDRAWCLIPPER iface, BOOL* lpbChanged
) {
    IDirectDrawClipperImpl *This = (IDirectDrawClipperImpl *)iface;
    FIXME("(%p)->(%p),stub!\n",This,lpbChanged);

    /* XXX What is safest? */
    *lpbChanged = FALSE;

    return DD_OK;
}

static const IDirectDrawClipperVtbl DDRAW_Clipper_VTable =
{
    Main_DirectDrawClipper_QueryInterface,
    Main_DirectDrawClipper_AddRef,
    Main_DirectDrawClipper_Release,
    Main_DirectDrawClipper_GetClipList,
    Main_DirectDrawClipper_GetHWnd,
    Main_DirectDrawClipper_Initialize,
    Main_DirectDrawClipper_IsClipListChanged,
    Main_DirectDrawClipper_SetClipList,
    Main_DirectDrawClipper_SetHwnd
};
