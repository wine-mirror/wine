/*		DirectDrawClipper implementation
 *
 * Copyright 2000 Marcus Meissner
 */

#include "config.h"
#include "winerror.h"

#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "debugtools.h"
#include "ddraw_private.h"

DEFAULT_DEBUG_CHANNEL(ddraw);

/******************************************************************************
 *			DirectDrawCreateClipper (DDRAW.7)
 */
HRESULT WINAPI DirectDrawCreateClipper(
    DWORD dwFlags, LPDIRECTDRAWCLIPPER *lplpDDClipper, LPUNKNOWN pUnkOuter
) {
    IDirectDrawClipperImpl** ilplpDDClipper=(IDirectDrawClipperImpl**)lplpDDClipper;
    TRACE("(%08lx,%p,%p)\n", dwFlags, ilplpDDClipper, pUnkOuter);

    *ilplpDDClipper = (IDirectDrawClipperImpl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawClipperImpl));
    ICOM_VTBL(*ilplpDDClipper) = &ddclipvt;
    (*ilplpDDClipper)->ref = 1;
    (*ilplpDDClipper)->hWnd = 0; 
    return DD_OK;
}

/******************************************************************************
 *			IDirectDrawClipper
 */
static HRESULT WINAPI IDirectDrawClipperImpl_SetHwnd(
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

static ULONG WINAPI IDirectDrawClipperImpl_Release(LPDIRECTDRAWCLIPPER iface) {
    ICOM_THIS(IDirectDrawClipperImpl,iface);
    TRACE("(%p)->() decrementing from %lu.\n", This, This->ref );

    This->ref--;
    if (This->ref)
	return This->ref;
    HeapFree(GetProcessHeap(),0,This);
    return S_OK;
}

static HRESULT WINAPI IDirectDrawClipperImpl_GetClipList(
    LPDIRECTDRAWCLIPPER iface,LPRECT rects,LPRGNDATA lprgn,LPDWORD hmm
) {
    ICOM_THIS(IDirectDrawClipperImpl,iface);
    FIXME("(%p,%p,%p,%p),stub!\n",This,rects,lprgn,hmm);
    if (hmm) *hmm=0;
    return DD_OK;
}

static HRESULT WINAPI IDirectDrawClipperImpl_SetClipList(
    LPDIRECTDRAWCLIPPER iface,LPRGNDATA lprgn,DWORD hmm
) {
    ICOM_THIS(IDirectDrawClipperImpl,iface);
    FIXME("(%p,%p,%ld),stub!\n",This,lprgn,hmm);
    return DD_OK;
}

static HRESULT WINAPI IDirectDrawClipperImpl_QueryInterface(
    LPDIRECTDRAWCLIPPER iface, REFIID riid, LPVOID* ppvObj
) {
    ICOM_THIS(IDirectDrawClipperImpl,iface);
    FIXME("(%p)->(%p,%p),stub!\n",This,riid,ppvObj);
    return OLE_E_ENUM_NOMORE;
}

static ULONG WINAPI IDirectDrawClipperImpl_AddRef( LPDIRECTDRAWCLIPPER iface )
{
    ICOM_THIS(IDirectDrawClipperImpl,iface);
    TRACE("(%p)->() incrementing from %lu.\n", This, This->ref );
    return ++(This->ref);
}

static HRESULT WINAPI IDirectDrawClipperImpl_GetHWnd(
    LPDIRECTDRAWCLIPPER iface, HWND* hWndPtr
) {
    ICOM_THIS(IDirectDrawClipperImpl,iface);
    FIXME("(%p)->(%p),stub!\n",This,hWndPtr);

    *hWndPtr = This->hWnd; 

    return DD_OK;
}

static HRESULT WINAPI IDirectDrawClipperImpl_Initialize(
     LPDIRECTDRAWCLIPPER iface, LPDIRECTDRAW lpDD, DWORD dwFlags
) {
    ICOM_THIS(IDirectDrawClipperImpl,iface);
    FIXME("(%p)->(%p,0x%08lx),stub!\n",This,lpDD,dwFlags);
    return DD_OK;
}

static HRESULT WINAPI IDirectDrawClipperImpl_IsClipListChanged(
    LPDIRECTDRAWCLIPPER iface, BOOL* lpbChanged
) {
    ICOM_THIS(IDirectDrawClipperImpl,iface);
    FIXME("(%p)->(%p),stub!\n",This,lpbChanged);
    return DD_OK;
}

ICOM_VTABLE(IDirectDrawClipper) ddclipvt = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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
