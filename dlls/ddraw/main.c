/*		DirectDraw Base Functions
 *
 * Copyright 1997-1999 Marcus Meissner
 * Copyright 1998 Lionel Ulmer (most of Direct3D stuff)
 */

#include "config.h"

#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "winerror.h"
#include "heap.h"
#include "dc.h"
#include "win.h"
#include "wine/exception.h"
#include "ddraw.h"
#include "d3d.h"
#include "debugtools.h"
#include "message.h"
#include "monitor.h"

/* This for all the enumeration and creation of D3D-related objects */
#include "ddraw_private.h"

#define MAX_DDRAW_DRIVERS	3
static ddraw_driver * ddraw_drivers[MAX_DDRAW_DRIVERS];
static int nrof_ddraw_drivers			= 0;

DEFAULT_DEBUG_CHANNEL(ddraw);

/* register a direct draw driver. We better not use malloc for we are in 
 * the ELF startup initialisation at this point.
 */
void ddraw_register_driver(ddraw_driver *driver) {
    ddraw_drivers[nrof_ddraw_drivers++] = driver;

    /* increase MAX_DDRAW_DRIVERS if the line below triggers */
    assert(nrof_ddraw_drivers <= MAX_DDRAW_DRIVERS);
}

/**********************************************************************/

typedef struct {
    LPVOID lpCallback;
    LPVOID lpContext; 
} DirectDrawEnumerateProcData;

/***********************************************************************
 *		DirectDrawEnumerateExA (DDRAW.*)
 */
HRESULT WINAPI DirectDrawEnumerateExA(
    LPDDENUMCALLBACKEXA lpCallback, LPVOID lpContext, DWORD dwFlags)
{
    int i;
    TRACE("(%p,%p, %08lx)\n", lpCallback, lpContext, dwFlags);

    if (TRACE_ON(ddraw)) {
	DPRINTF("  Flags : ");
	if (dwFlags & DDENUM_ATTACHEDSECONDARYDEVICES)
	    DPRINTF("DDENUM_ATTACHEDSECONDARYDEVICES ");
	if (dwFlags & DDENUM_DETACHEDSECONDARYDEVICES)
	    DPRINTF("DDENUM_DETACHEDSECONDARYDEVICES ");
	if (dwFlags & DDENUM_NONDISPLAYDEVICES)
	    DPRINTF("DDENUM_NONDISPLAYDEVICES ");
	DPRINTF("\n");
    }

    if (dwFlags & DDENUM_NONDISPLAYDEVICES) {
	FIXME("no non-display devices supported.\n");
	return DD_OK;
    }
/* Hmm. Leave this out.
    if (dwFlags & DDENUM_ATTACHEDSECONDARYDEVICES) {
	FIXME("no attached secondary devices supported.\n");
	return DD_OK;
    }
 */
    if (dwFlags & DDENUM_DETACHEDSECONDARYDEVICES) {
	FIXME("no detached secondary devices supported.\n");
	return DD_OK;
    }

    for (i=0;i<MAX_DDRAW_DRIVERS;i++) {
	if (!ddraw_drivers[i])
	    continue;
	if (ddraw_drivers[i]->createDDRAW(NULL)) /* !0 is failing */
	    continue;
        TRACE("Enumerating %s/%s interface\n",ddraw_drivers[i]->name,ddraw_drivers[i]->type);
	if (!lpCallback(
	    ddraw_drivers[i]->guid,
	    (LPSTR)ddraw_drivers[i]->name,
	    (LPSTR)ddraw_drivers[i]->type,
	    lpContext,
	    0		/* FIXME: flags not supported here */
	))
	    return DD_OK;
    }
    if (nrof_ddraw_drivers) {
	TRACE("Enumerating the default interface\n");
	if (!lpCallback(NULL,"WINE (default)", "display", lpContext, 0))
	    return DD_OK;
    }
    return DD_OK;
}

/***********************************************************************
 *		DirectDrawEnumerateExW (DDRAW.*)
 */

static BOOL CALLBACK DirectDrawEnumerateExProcW(
    GUID *lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName, 
    LPVOID lpContext, HMONITOR hm)
{
    DirectDrawEnumerateProcData *pEPD = (DirectDrawEnumerateProcData*)lpContext;
    LPWSTR lpDriverDescriptionW =
	HEAP_strdupAtoW(GetProcessHeap(), 0, lpDriverDescription);
    LPWSTR lpDriverNameW =
	HEAP_strdupAtoW(GetProcessHeap(), 0, lpDriverName);

    BOOL bResult = (*(LPDDENUMCALLBACKEXW *) pEPD->lpCallback)(
	lpGUID, lpDriverDescriptionW, lpDriverNameW, pEPD->lpContext, hm);

    HeapFree(GetProcessHeap(), 0, lpDriverDescriptionW);
    HeapFree(GetProcessHeap(), 0, lpDriverNameW);
    return bResult;
}

/**********************************************************************/

HRESULT WINAPI DirectDrawEnumerateExW(
  LPDDENUMCALLBACKEXW lpCallback, LPVOID lpContext, DWORD dwFlags)
{
    DirectDrawEnumerateProcData epd;
    epd.lpCallback = (LPVOID) lpCallback;
    epd.lpContext = lpContext;

    return DirectDrawEnumerateExA(DirectDrawEnumerateExProcW, (LPVOID) &epd, 0);
}

/***********************************************************************
 *		DirectDrawEnumerateA (DDRAW.*)
 */

static BOOL CALLBACK DirectDrawEnumerateProcA(
	GUID *lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName, 
	LPVOID lpContext, HMONITOR hm)
{
    DirectDrawEnumerateProcData *pEPD = (DirectDrawEnumerateProcData*)lpContext;

    return ((LPDDENUMCALLBACKA) pEPD->lpCallback)(
	lpGUID, lpDriverDescription, lpDriverName, pEPD->lpContext);
}

/**********************************************************************/

HRESULT WINAPI DirectDrawEnumerateA(
  LPDDENUMCALLBACKA lpCallback, LPVOID lpContext) 
{
    DirectDrawEnumerateProcData epd;  
    epd.lpCallback = (LPVOID) lpCallback;
    epd.lpContext = lpContext;

    return DirectDrawEnumerateExA(DirectDrawEnumerateProcA, (LPVOID) &epd, 0);
}

/***********************************************************************
 *		DirectDrawEnumerateW (DDRAW.*)
 */

static BOOL WINAPI DirectDrawEnumerateProcW(
  GUID *lpGUID, LPWSTR lpDriverDescription, LPWSTR lpDriverName, 
  LPVOID lpContext, HMONITOR hm)
{
    DirectDrawEnumerateProcData *pEPD = (DirectDrawEnumerateProcData*)lpContext;
  
    return ((LPDDENUMCALLBACKW) pEPD->lpCallback)(
	lpGUID, lpDriverDescription, lpDriverName, pEPD->lpContext);
}

/**********************************************************************/

HRESULT WINAPI DirectDrawEnumerateW(
  LPDDENUMCALLBACKW lpCallback, LPVOID lpContext) 
{
    DirectDrawEnumerateProcData epd;
    epd.lpCallback = (LPVOID) lpCallback;
    epd.lpContext = lpContext;

    return DirectDrawEnumerateExW(DirectDrawEnumerateProcW, (LPVOID) &epd, 0);
}

/******************************************************************************
 * 				DirectDraw Window Procedure
 */
static LRESULT WINAPI DDWndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    LRESULT ret;
    IDirectDrawImpl* ddraw = NULL;
    DWORD lastError;

    /* FIXME(ddraw,"(0x%04x,%s,0x%08lx,0x%08lx),stub!\n",(int)hwnd,SPY_GetMsgName(msg),(long)wParam,(long)lParam); */

    SetLastError( ERROR_SUCCESS );
    ddraw  = (IDirectDrawImpl*)GetPropA( hwnd, ddProp );
    if( (!ddraw)  && ( ( lastError = GetLastError() ) != ERROR_SUCCESS )) 
	ERR("Unable to retrieve this ptr from window. Error %08lx\n",lastError);

    if( ddraw ) {
    /* Perform any special direct draw functions */
	if (msg==WM_PAINT)
	    ddraw->d.paintable = 1;

	/* Now let the application deal with the rest of this */
	if( ddraw->d.mainWindow ) {

	    /* Don't think that we actually need to call this but... 
	     * might as well be on the safe side of things...
	     */

	    /* I changed hwnd to ddraw->d.mainWindow as I did not see why
	     * it should be the procedures of our fake window that gets called
	     * instead of those of the window provided by the application.
	     * And with this patch, mouse clicks work with Monkey Island III
	     * - Lionel
	     */
	    ret = DefWindowProcA( ddraw->d.mainWindow, msg, wParam, lParam );

	    if( !ret ) {
		WND *tmpWnd =WIN_FindWndPtr(ddraw->d.mainWindow);
		/* We didn't handle the message - give it to the application */
		if (ddraw && ddraw->d.mainWindow && tmpWnd)
		    ret = CallWindowProcA(tmpWnd->winproc,
		ddraw->d.mainWindow, msg, wParam, lParam );
		WIN_ReleaseWndPtr(tmpWnd);
	    }
	    return ret;
	} /* else FALLTHROUGH */
    } /* else FALLTHROUGH */
    return DefWindowProcA(hwnd,msg,wParam,lParam);
}

/***********************************************************************
 *		DirectDrawCreate
 */
HRESULT WINAPI DirectDrawCreate(
	LPGUID lpGUID, LPDIRECTDRAW *lplpDD, LPUNKNOWN pUnkOuter
) {
    IDirectDrawImpl** ilplpDD=(IDirectDrawImpl**)lplpDD;
    WNDCLASSA	wc;
    HRESULT	ret = 0;
    int		i,drvindex=0;
    GUID	zeroGUID;

    struct ddraw_driver	*ddd = NULL;

    if (!HIWORD(lpGUID)) lpGUID = NULL;

    TRACE("(%s,%p,%p)\n",debugstr_guid(lpGUID),ilplpDD,pUnkOuter);

    memset(&zeroGUID,0,sizeof(zeroGUID));
    while (1) {
	ddd = NULL;
	if ( ( !lpGUID ) ||
	     ( IsEqualGUID( &zeroGUID,  lpGUID ) ) ||
	     ( IsEqualGUID( &IID_IDirectDraw,  lpGUID ) ) ||
	     ( IsEqualGUID( &IID_IDirectDraw2, lpGUID ) ) ||
	     ( IsEqualGUID( &IID_IDirectDraw4, lpGUID ) )
	) {
	    /* choose an interface out of the list */
	    for (i=0;i<nrof_ddraw_drivers;i++) {
		ddraw_driver *xddd = ddraw_drivers[i];
		if (!xddd)
		    continue;
		if (!ddd || (ddd->preference<xddd->preference)) {
		    drvindex = i;
		    ddd = xddd;
		}
	    }
	} else {
	    for (i=0;i<nrof_ddraw_drivers;i++) {
		if (!ddraw_drivers[i])
		    continue;
		if (IsEqualGUID(ddraw_drivers[i]->guid,lpGUID)) {
		    drvindex = i;
		    ddd = ddraw_drivers[i];
		    break;
		}
	    }
	}
	if (!ddd) {
	    if (!nrof_ddraw_drivers) {
		ERR("DirectDrawCreate(%s,%p,%p): no DirectDraw drivers compiled in.\n",debugstr_guid(lpGUID),lplpDD,pUnkOuter);
		return DDERR_NODIRECTDRAWHW;
	    }
	    ERR("DirectDrawCreate(%s,%p,%p): did not recognize requested GUID.\n",debugstr_guid(lpGUID),lplpDD,pUnkOuter);
	    return DDERR_INVALIDDIRECTDRAWGUID;
	}
	TRACE("using \"%s\" driver, calling %p\n",ddd->name,ddd->createDDRAW);

	ret = ddd->createDDRAW(lplpDD);
	if (!ret)
	    break;
	ddraw_drivers[drvindex] = NULL; /* mark this one as unusable */
    }
    wc.style		= CS_GLOBALCLASS;
    wc.lpfnWndProc	= DDWndProc;
    wc.cbClsExtra	= 0;
    wc.cbWndExtra	= 0;

    /* We can be a child of the desktop since we're really important */
    wc.hInstance= 0; 
    wc.hIcon	= 0;
    wc.hCursor	= (HCURSOR)IDC_ARROWA;
    wc.hbrBackground	= NULL_BRUSH;
    wc.lpszMenuName 	= 0;
    wc.lpszClassName	= "WINE_DirectDraw";
    (*ilplpDD)->d.winclass = RegisterClassA(&wc);
    return ret;
}

/*******************************************************************************
 * DirectDraw ClassFactory
 *
 *  Heavily inspired (well, can you say completely copied :-) ) from DirectSound
 *
 */
typedef struct {
    /* IUnknown fields */
    ICOM_VFIELD(IClassFactory);
    DWORD		ref;
} IClassFactoryImpl;

static HRESULT WINAPI 
DDCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
    ICOM_THIS(IClassFactoryImpl,iface);

    FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI
DDCF_AddRef(LPCLASSFACTORY iface) {
    ICOM_THIS(IClassFactoryImpl,iface);
    return ++(This->ref);
}

static ULONG WINAPI DDCF_Release(LPCLASSFACTORY iface) {
    ICOM_THIS(IClassFactoryImpl,iface);
    /* static class, won't be  freed */
    return --(This->ref);
}

static HRESULT WINAPI DDCF_CreateInstance(
	LPCLASSFACTORY iface,LPUNKNOWN pOuter,REFIID riid,LPVOID *ppobj
) {
    ICOM_THIS(IClassFactoryImpl,iface);

    TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);
    if ( ( IsEqualGUID( &IID_IDirectDraw,  riid ) ) ||
	 ( IsEqualGUID( &IID_IDirectDraw2, riid ) ) ||
	 ( IsEqualGUID( &IID_IDirectDraw4, riid ) ) ) {
	    /* FIXME: reuse already created DirectDraw if present? */
	    return DirectDrawCreate((LPGUID) riid,(LPDIRECTDRAW*)ppobj,pOuter);
    }
    return CLASS_E_CLASSNOTAVAILABLE;
}

static HRESULT WINAPI DDCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
    ICOM_THIS(IClassFactoryImpl,iface);
    FIXME("(%p)->(%d),stub!\n",This,dolock);
    return S_OK;
}

static ICOM_VTABLE(IClassFactory) DDCF_Vtbl = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    DDCF_QueryInterface,
    DDCF_AddRef,
    DDCF_Release,
    DDCF_CreateInstance,
    DDCF_LockServer
};
static IClassFactoryImpl DDRAW_CF = {&DDCF_Vtbl, 1 };

/*******************************************************************************
 * DllGetClassObject [DDRAW.13]
 * Retrieves class object from a DLL object
 *
 * NOTES
 *    Docs say returns STDAPI
 *
 * PARAMS
 *    rclsid [I] CLSID for the class object
 *    riid   [I] Reference to identifier of interface for class object
 *    ppv    [O] Address of variable to receive interface pointer for riid
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: CLASS_E_CLASSNOTAVAILABLE, E_OUTOFMEMORY, E_INVALIDARG,
 *             E_UNEXPECTED
 */
DWORD WINAPI DDRAW_DllGetClassObject(REFCLSID rclsid,REFIID riid,LPVOID *ppv)
{
    TRACE("(%p,%p,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    if ( IsEqualGUID( &IID_IClassFactory, riid ) ) {
    	*ppv = (LPVOID)&DDRAW_CF;
	IClassFactory_AddRef((IClassFactory*)*ppv);
	return S_OK;
    }
    FIXME("(%p,%p,%p): no interface found.\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}


/*******************************************************************************
 * DllCanUnloadNow [DDRAW.12]  Determines whether the DLL is in use.
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: S_FALSE
 */
DWORD WINAPI DDRAW_DllCanUnloadNow(void) {
    FIXME("(void): stub\n");
    return S_FALSE;
}
