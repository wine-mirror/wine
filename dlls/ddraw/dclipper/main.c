/*		DirectDrawClipper implementation
 *
 * Copyright 2000 Marcus Meissner
 * Copyright 2000 TransGaming Technologies Inc.
 */

#include "config.h"
#include "winerror.h"
#include "debugtools.h"

#include <stdlib.h>

#include "ddraw_private.h"
#include "dclipper/main.h"
#include "ddraw/main.h"

DEFAULT_DEBUG_CHANNEL(ddraw);

/******************************************************************************
 *			DirectDrawCreateClipper (DDRAW.7)
 */

static ICOM_VTABLE(IDirectDrawClipper) DDRAW_Clipper_VTable;

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
    ICOM_THIS(IDirectDrawClipperImpl,iface);

    TRACE("(%p)->SetHwnd(0x%08lx,0x%08lx)\n",This,dwFlags,(DWORD)hWnd);
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
    ICOM_THIS(IDirectDrawClipperImpl,iface);
    TRACE("(%p)->() decrementing from %lu.\n", This, This->ref );

    if (--This->ref == 0)
    {
	Main_DirectDrawClipper_Destroy(This);
	return 0;
    }
    else return This->ref;
}

HRESULT WINAPI Main_DirectDrawClipper_GetClipList(
    LPDIRECTDRAWCLIPPER iface,LPRECT prcClip,LPRGNDATA lprgn,LPDWORD pdwSize
) {
    ICOM_THIS(IDirectDrawClipperImpl,iface);
    FIXME("(%p,%p,%p,%p),stub!\n",This,prcClip,lprgn,pdwSize);
    abort();
    if (pdwSize) *pdwSize=0;
    return DD_OK;
}

HRESULT WINAPI Main_DirectDrawClipper_SetClipList(
    LPDIRECTDRAWCLIPPER iface,LPRGNDATA lprgn,DWORD pdwSize
) {
    ICOM_THIS(IDirectDrawClipperImpl,iface);
    FIXME("(%p,%p,%ld),stub!\n",This,lprgn,pdwSize);
    abort();
    return DD_OK;
}

HRESULT WINAPI Main_DirectDrawClipper_QueryInterface(
    LPDIRECTDRAWCLIPPER iface, REFIID riid, LPVOID* ppvObj
) {
    ICOM_THIS(IDirectDrawClipperImpl,iface);

    if (IsEqualGUID(&IID_IUnknown, riid)
	|| IsEqualGUID(&IID_IDirectDrawClipper, riid))
    {
	*ppvObj = ICOM_INTERFACE(This, IDirectDrawClipper);
	++This->ref;
	return S_OK;
    }
    else
    {
	return E_NOINTERFACE;
    }
}

ULONG WINAPI Main_DirectDrawClipper_AddRef( LPDIRECTDRAWCLIPPER iface )
{
    ICOM_THIS(IDirectDrawClipperImpl,iface);
    TRACE("(%p)->() incrementing from %lu.\n", This, This->ref );
    return ++This->ref;
}

HRESULT WINAPI Main_DirectDrawClipper_GetHWnd(
    LPDIRECTDRAWCLIPPER iface, HWND* hWndPtr
) {
    ICOM_THIS(IDirectDrawClipperImpl,iface);
    FIXME("(%p)->(%p),stub!\n",This,hWndPtr);

    *hWndPtr = This->hWnd; 

    return DD_OK;
}

HRESULT WINAPI Main_DirectDrawClipper_Initialize(
     LPDIRECTDRAWCLIPPER iface, LPDIRECTDRAW lpDD, DWORD dwFlags
) {
    IDirectDrawImpl* pOwner;
    ICOM_THIS(IDirectDrawClipperImpl,iface);
    FIXME("(%p)->(%p,0x%08lx),stub!\n",This,lpDD,dwFlags);

    if (This->ddraw_owner != NULL) return DDERR_ALREADYINITIALIZED;

    pOwner = ICOM_OBJECT(IDirectDrawImpl, IDirectDraw, lpDD);
    This->ddraw_owner = pOwner;
    Main_DirectDraw_AddClipper(pOwner, This);

    return DD_OK;
}

HRESULT WINAPI Main_DirectDrawClipper_IsClipListChanged(
    LPDIRECTDRAWCLIPPER iface, BOOL* lpbChanged
) {
    ICOM_THIS(IDirectDrawClipperImpl,iface);
    FIXME("(%p)->(%p),stub!\n",This,lpbChanged);

    /* XXX What is safest? */
    *lpbChanged = FALSE;

    return DD_OK;
}

static ICOM_VTABLE(IDirectDrawClipper) DDRAW_Clipper_VTable =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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
