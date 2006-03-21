/*		DirectDraw Base Functions
 *
 * Copyright 1997-1999 Marcus Meissner
 * Copyright 1998 Lionel Ulmer (most of Direct3D stuff)
 * Copyright 2000-2001 TransGaming Technologies Inc.
 *
 * This file contains the (internal) driver registration functions,
 * driver enumeration APIs and DirectDraw creation functions.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define GLPRIVATE_NO_REDEFINE

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winerror.h"
#include "wingdi.h"
#include "wine/exception.h"
#include "excpt.h"

#include "ddraw.h"
#include "d3d.h"

/* This for all the enumeration and creation of D3D-related objects */
#include "ddraw_private.h"
#include "wine/debug.h"
#include "wine/library.h"

#include "gl_private.h"

#undef GLPRIVATE_NO_REDEFINE

#define MAX_DDRAW_DRIVERS 3
static const ddraw_driver* DDRAW_drivers[MAX_DDRAW_DRIVERS];
static int DDRAW_num_drivers; /* = 0 */
static int DDRAW_default_driver;

void (*wine_tsx11_lock_ptr)(void) = NULL;
void (*wine_tsx11_unlock_ptr)(void) = NULL;

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

/**********************************************************************/

typedef struct {
    LPVOID lpCallback;
    LPVOID lpContext;
} DirectDrawEnumerateProcData;

BOOL opengl_initialized = 0;

#ifdef HAVE_OPENGL

#include "opengl_private.h"

static void *gl_handle = NULL;

#define GL_API_FUNCTION(f) typeof(f) * p##f;
#include "gl_api.h"
#undef GL_API_FUNCTION

#ifndef SONAME_LIBGL
#define SONAME_LIBGL "libGL.so"
#endif

static BOOL DDRAW_bind_to_opengl( void )
{
    const char *glname = SONAME_LIBGL;

    gl_handle = wine_dlopen(glname, RTLD_NOW, NULL, 0);
    if (!gl_handle) {
        WARN("Wine cannot find the OpenGL graphics library (%s).\n",glname);
	return FALSE;
    }

#define GL_API_FUNCTION(f)  \
    if((p##f = wine_dlsym(gl_handle, #f, NULL, 0)) == NULL) \
    { \
        WARN("Can't find symbol %s\n", #f); \
        goto sym_not_found; \
    }
#include "gl_api.h"
#undef GL_API_FUNCTION

    /* And now calls the function to initialize the various fields for the rendering devices */
    return d3ddevice_init_at_startup(gl_handle);
    
sym_not_found:
    WARN("Wine cannot find certain functions that it needs inside the OpenGL\n"
	 "graphics library.  To enable Wine to use OpenGL please upgrade\n"
	 "your OpenGL libraries\n");
    wine_dlclose(gl_handle, NULL, 0);
    gl_handle = NULL;
    return FALSE;
}

#endif /* HAVE_OPENGL */

BOOL s3tc_initialized = 0;

static void *s3tc_handle = NULL;

FUNC_FETCH_2D_TEXEL_RGBA_DXT1 fetch_2d_texel_rgba_dxt1;
FUNC_FETCH_2D_TEXEL_RGBA_DXT3 fetch_2d_texel_rgba_dxt3;
FUNC_FETCH_2D_TEXEL_RGBA_DXT5 fetch_2d_texel_rgba_dxt5;

#ifndef SONAME_LIBTXC_DXTN
#define SONAME_LIBTXC_DXTN "libtxc_dxtn.so"
#endif

static BOOL DDRAW_bind_to_s3tc( void )
{
    const char * const s3tcname = SONAME_LIBTXC_DXTN;

    s3tc_handle = wine_dlopen(s3tcname, RTLD_NOW, NULL, 0);
    if (!s3tc_handle) {
        TRACE("No S3TC software decompression library seems to be present (%s).\n",s3tcname);
	return FALSE;
    }
    TRACE("Found S3TC software decompression library (%s).\n",s3tcname);

#define API_FUNCTION(f)  \
    if((f = wine_dlsym(s3tc_handle, #f, NULL, 0)) == NULL) \
    { \
        WARN("Can't find symbol %s\n", #f); \
        goto sym_not_found; \
    }
    API_FUNCTION(fetch_2d_texel_rgba_dxt1);
    API_FUNCTION(fetch_2d_texel_rgba_dxt3);
    API_FUNCTION(fetch_2d_texel_rgba_dxt5);
#undef API_FUNCTION

    return TRUE;
    
sym_not_found:
    WARN("Wine cannot find functions that are necessary for S3TC software decompression\n");
    wine_dlclose(s3tc_handle, NULL, 0);
    s3tc_handle = NULL;
    return FALSE;
}

/*******************************************************************************
 * DirectDrawEnumerateExA (DDRAW.@)
 *
 * Enumerates all DirectDraw devices installed on the system.
 *
 * PARAMS
 *  lpCallback [I] DDEnumCallbackEx function to be called with a description of 
 *		   each enumerated HAL.
 *  lpContext  [I] application-defined value to be passed to the callback.
 *  dwFlags    [I] Specifies the enumeration scope. see msdn.
 *
 * RETURNS
 *  Success: DD_OK.
 *  Failure: DDERR_INVALIDPARAMS
 */
HRESULT WINAPI DirectDrawEnumerateExA(
    LPDDENUMCALLBACKEXA lpCallback, LPVOID lpContext, DWORD dwFlags)
{
    int i;
    BOOL stop = FALSE;
    TRACE("(%p,%p, %08lx)\n", lpCallback, lpContext, dwFlags);

    if (TRACE_ON(ddraw)) {
	TRACE("  Flags : ");
	if (dwFlags & DDENUM_ATTACHEDSECONDARYDEVICES)
	    TRACE("DDENUM_ATTACHEDSECONDARYDEVICES ");
	if (dwFlags & DDENUM_DETACHEDSECONDARYDEVICES)
	    TRACE("DDENUM_DETACHEDSECONDARYDEVICES ");
	if (dwFlags & DDENUM_NONDISPLAYDEVICES)
	    TRACE("DDENUM_NONDISPLAYDEVICES ");
	TRACE("\n");
    }

    for (i=0; i<DDRAW_num_drivers; i++)
    {
        TRACE("Enumerating %s/%s interface\n",
	      DDRAW_drivers[i]->info->szDriver,
	      DDRAW_drivers[i]->info->szDescription);

	/* We have to pass NULL from the primary display device.
	 * RoadRage chapter 6's enumeration routine expects it. */
        __TRY
        {
            if (!lpCallback((DDRAW_default_driver == i) ? NULL
                            :(LPGUID)&DDRAW_drivers[i]->info->guidDeviceIdentifier,
                            (LPSTR)DDRAW_drivers[i]->info->szDescription,
                            (LPSTR)DDRAW_drivers[i]->info->szDriver,
                            lpContext, 0))
                stop = TRUE;
        }
        __EXCEPT_PAGE_FAULT
        {
            return E_INVALIDARG;
        }
        __ENDTRY
        if (stop) return DD_OK;
    }

    /* Unsupported flags */
    if (dwFlags & DDENUM_NONDISPLAYDEVICES) {
	FIXME("no non-display devices supported.\n");
    }
    if (dwFlags & DDENUM_DETACHEDSECONDARYDEVICES) {
	FIXME("no detached secondary devices supported.\n");
    }

    return DD_OK;
}

/*******************************************************************************
 * DirectDrawEnumerateExW (DDRAW.@)
 */

static BOOL CALLBACK DirectDrawEnumerateExProcW(
    GUID *lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName,
    LPVOID lpContext, HMONITOR hm)
{
    INT len;
    BOOL bResult;
    LPWSTR lpDriverDescriptionW, lpDriverNameW;
    DirectDrawEnumerateProcData *pEPD = (DirectDrawEnumerateProcData*)lpContext;

    len = MultiByteToWideChar( CP_ACP, 0, lpDriverDescription, -1, NULL, 0 );
    lpDriverDescriptionW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, lpDriverDescription, -1, lpDriverDescriptionW, len );

    len = MultiByteToWideChar( CP_ACP, 0, lpDriverName, -1, NULL, 0 );
    lpDriverNameW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, lpDriverName, -1, lpDriverNameW, len );

    bResult = (*(LPDDENUMCALLBACKEXW *) pEPD->lpCallback)(lpGUID, lpDriverDescriptionW,
                                                          lpDriverNameW, pEPD->lpContext, hm);

    HeapFree(GetProcessHeap(), 0, lpDriverDescriptionW);
    HeapFree(GetProcessHeap(), 0, lpDriverNameW);
    return bResult;
}

HRESULT WINAPI DirectDrawEnumerateExW(
  LPDDENUMCALLBACKEXW lpCallback, LPVOID lpContext, DWORD dwFlags)
{
    DirectDrawEnumerateProcData epd;
    epd.lpCallback = (LPVOID) lpCallback;
    epd.lpContext = lpContext;

    return DirectDrawEnumerateExA(DirectDrawEnumerateExProcW, (LPVOID) &epd, 0);
}

/***********************************************************************
 *		DirectDrawEnumerateA (DDRAW.@)
 */

static BOOL CALLBACK DirectDrawEnumerateProcA(
	GUID *lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName,
	LPVOID lpContext, HMONITOR hm)
{
    DirectDrawEnumerateProcData *pEPD = (DirectDrawEnumerateProcData*)lpContext;

    return ((LPDDENUMCALLBACKA) pEPD->lpCallback)(
	lpGUID, lpDriverDescription, lpDriverName, pEPD->lpContext);
}

HRESULT WINAPI DirectDrawEnumerateA(
  LPDDENUMCALLBACKA lpCallback, LPVOID lpContext)
{
    DirectDrawEnumerateProcData epd;
    epd.lpCallback = (LPVOID) lpCallback;
    epd.lpContext = lpContext;

    return DirectDrawEnumerateExA(DirectDrawEnumerateProcA, (LPVOID) &epd, 0);
}

/***********************************************************************
 *		DirectDrawEnumerateW (DDRAW.@)
 */

static BOOL WINAPI DirectDrawEnumerateProcW(
  GUID *lpGUID, LPWSTR lpDriverDescription, LPWSTR lpDriverName,
  LPVOID lpContext, HMONITOR hm)
{
    DirectDrawEnumerateProcData *pEPD = (DirectDrawEnumerateProcData*)lpContext;

    return ((LPDDENUMCALLBACKW) pEPD->lpCallback)(
	lpGUID, lpDriverDescription, lpDriverName, pEPD->lpContext);
}

HRESULT WINAPI DirectDrawEnumerateW(
  LPDDENUMCALLBACKW lpCallback, LPVOID lpContext)
{
    DirectDrawEnumerateProcData epd;
    epd.lpCallback = (LPVOID) lpCallback;
    epd.lpContext = lpContext;

    return DirectDrawEnumerateExW(DirectDrawEnumerateProcW, (LPVOID) &epd, 0);
}

/***********************************************************************
 *		DirectDrawCreate (DDRAW.@)
 */

const ddraw_driver* DDRAW_FindDriver(const GUID* pGUID)
{
    static const GUID zeroGUID; /* gets zero-inited */

    TRACE("(%s)\n", pGUID ? debugstr_guid(pGUID) : "(null)");

    if (DDRAW_num_drivers == 0) return NULL;

    if (pGUID == (LPGUID)DDCREATE_EMULATIONONLY
	|| pGUID == (LPGUID)DDCREATE_HARDWAREONLY)
	pGUID = NULL;

    if (pGUID == NULL || memcmp(pGUID, &zeroGUID, sizeof(GUID)) == 0)
    {
	/* Use the default driver. */
	return DDRAW_drivers[DDRAW_default_driver];
    }
    else
    {
	/* Look for a matching GUID. */

	int i;
	for (i=0; i < DDRAW_num_drivers; i++)
	{
	    if (IsEqualGUID(pGUID,
			    &DDRAW_drivers[i]->info->guidDeviceIdentifier))
		break;
	}

	if (i < DDRAW_num_drivers)
	{
	    return DDRAW_drivers[i];
	}
	else
	{
	    ERR("(%s): did not recognize requested GUID.\n",debugstr_guid(pGUID));
	    return NULL;
	}
    }
}

static HRESULT DDRAW_Create(
	LPGUID lpGUID, LPVOID *lplpDD, LPUNKNOWN pUnkOuter, REFIID iid, BOOL ex
) {
    const ddraw_driver* driver;
    LPDIRECTDRAW7 pDD;
    HRESULT hr;

    TRACE("(%s,%p,%p,%d)\n", debugstr_guid(lpGUID), lplpDD, pUnkOuter, ex);

    if (DDRAW_num_drivers == 0)
    {
	WARN("no DirectDraw drivers registered\n");
	return DDERR_INVALIDDIRECTDRAWGUID;
    }

    if (lpGUID == (LPGUID)DDCREATE_EMULATIONONLY
	|| lpGUID == (LPGUID)DDCREATE_HARDWAREONLY)
	lpGUID = NULL;

    if (pUnkOuter != NULL)
	return DDERR_INVALIDPARAMS; /* CLASS_E_NOAGGREGATION? */

    driver = DDRAW_FindDriver(lpGUID);
    if (driver == NULL) return DDERR_INVALIDDIRECTDRAWGUID;

    hr = driver->create(lpGUID, &pDD, pUnkOuter, ex);
    if (FAILED(hr)) return hr;

    hr = IDirectDraw7_QueryInterface(pDD, iid, lplpDD);
    IDirectDraw7_Release(pDD);
    return hr;
}

/***********************************************************************
 *		DirectDrawCreate (DDRAW.@)
 *
 * Only creates legacy IDirectDraw interfaces.
 * Cannot create IDirectDraw7 interfaces.
 * In theory.
 */
HRESULT WINAPI DirectDrawCreate(
	LPGUID lpGUID, LPDIRECTDRAW* lplpDD, LPUNKNOWN pUnkOuter
) {
    TRACE("(%s,%p,%p)\n", debugstr_guid(lpGUID), lplpDD, pUnkOuter);
    return DDRAW_Create(lpGUID, (LPVOID*) lplpDD, pUnkOuter, &IID_IDirectDraw, FALSE);
}

/***********************************************************************
 *		DirectDrawCreateEx (DDRAW.@)
 *
 * Only creates new IDirectDraw7 interfaces.
 * Supposed to fail if legacy interfaces are requested.
 * In theory.
 */
HRESULT WINAPI DirectDrawCreateEx(
	LPGUID lpGUID, LPVOID* lplpDD, REFIID iid, LPUNKNOWN pUnkOuter
) {
    TRACE("(%s,%p,%s,%p)\n", debugstr_guid(lpGUID), lplpDD, debugstr_guid(iid), pUnkOuter);

    if (!IsEqualGUID(iid, &IID_IDirectDraw7))
	return DDERR_INVALIDPARAMS;

    return DDRAW_Create(lpGUID, lplpDD, pUnkOuter, iid, TRUE);
}

extern HRESULT Uninit_DirectDraw_Create(const GUID*, LPDIRECTDRAW7*,
					LPUNKNOWN, BOOL);

/* This is for the class factory. */
static HRESULT DDRAW_CreateDirectDraw(IUnknown* pUnkOuter, REFIID iid,
				      LPVOID* ppObj)
{
    LPDIRECTDRAW7 pDD;
    HRESULT hr;
    BOOL ex;

    TRACE("(%p,%s,%p)\n", pUnkOuter, debugstr_guid(iid), ppObj);
    
    /* This is a mighty hack :-) */
    if (IsEqualGUID(iid, &IID_IDirectDraw7))
	ex = TRUE;
    else
        ex = FALSE;
    
    hr = Uninit_DirectDraw_Create(NULL, &pDD, pUnkOuter, ex);
    if (FAILED(hr)) return hr;

    hr = IDirectDraw7_QueryInterface(pDD, iid, ppObj);
    IDirectDraw_Release(pDD);
    return hr;
}

/******************************************************************************
 * DirectDraw ClassFactory
 */
typedef struct {
    ICOM_VFIELD_MULTI(IClassFactory);

    LONG ref;
    HRESULT (*pfnCreateInstance)(IUnknown *pUnkOuter, REFIID iid,
				 LPVOID *ppObj);
} IClassFactoryImpl;

struct object_creation_info
{
    const CLSID *clsid;
    HRESULT (*pfnCreateInstance)(IUnknown *pUnkOuter, REFIID riid,
				 LPVOID *ppObj);
};

/* There should be more, but these are the only ones listed in the header
 * file. */
extern HRESULT DDRAW_CreateDirectDrawClipper(IUnknown *pUnkOuter, REFIID riid,
					     LPVOID *ppObj);

static const struct object_creation_info object_creation[] =
{
    { &CLSID_DirectDraw,       	DDRAW_CreateDirectDraw },
    { &CLSID_DirectDraw7,	DDRAW_CreateDirectDraw },
    { &CLSID_DirectDrawClipper,	DDRAW_CreateDirectDrawClipper }
};

static HRESULT WINAPI
DDCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;

    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(riid), ppobj);
    
    if (IsEqualGUID(riid, &IID_IUnknown)
	|| IsEqualGUID(riid, &IID_IClassFactory))
    {
	IClassFactory_AddRef(iface);
	*ppobj = This;
	return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI DDCF_AddRef(LPCLASSFACTORY iface)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->() incrementing from %ld.\n", This, ref - 1);
    
    return ref;
}

static ULONG WINAPI DDCF_Release(LPCLASSFACTORY iface)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p)->() decrementing from %ld.\n", This, ref+1);

    if (ref == 0)
	HeapFree(GetProcessHeap(), 0, This);

    return ref;
}


static HRESULT WINAPI DDCF_CreateInstance(
	LPCLASSFACTORY iface,LPUNKNOWN pOuter,REFIID riid,LPVOID *ppobj
)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;

    TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);

    return This->pfnCreateInstance(pOuter, riid, ppobj);
}

static HRESULT WINAPI DDCF_LockServer(LPCLASSFACTORY iface,BOOL dolock)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    FIXME("(%p)->(%d),stub!\n",This,dolock);
    return S_OK;
}

static const IClassFactoryVtbl DDCF_Vtbl =
{
    DDCF_QueryInterface,
    DDCF_AddRef,
    DDCF_Release,
    DDCF_CreateInstance,
    DDCF_LockServer
};

/*******************************************************************************
 * DllGetClassObject [DDRAW.@]
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
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    unsigned int i;
    IClassFactoryImpl *factory;

    TRACE("(%s,%s,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if ( !IsEqualGUID( &IID_IClassFactory, riid )
	 && ! IsEqualGUID( &IID_IUnknown, riid) )
	return E_NOINTERFACE;

    for (i=0; i < sizeof(object_creation)/sizeof(object_creation[0]); i++)
    {
	if (IsEqualGUID(object_creation[i].clsid, rclsid))
	    break;
    }

    if (i == sizeof(object_creation)/sizeof(object_creation[0]))
    {
	FIXME("%s: no class found.\n", debugstr_guid(rclsid));
	return CLASS_E_CLASSNOTAVAILABLE;
    }

    factory = HeapAlloc(GetProcessHeap(), 0, sizeof(*factory));
    if (factory == NULL) return E_OUTOFMEMORY;

    ICOM_INIT_INTERFACE(factory, IClassFactory, DDCF_Vtbl);
    factory->ref = 1;

    factory->pfnCreateInstance = object_creation[i].pfnCreateInstance;

    *ppv = ICOM_INTERFACE(factory, IClassFactory);
    return S_OK;
}


/*******************************************************************************
 * DllCanUnloadNow [DDRAW.@]  Determines whether the DLL is in use.
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: S_FALSE
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    FIXME("(void): stub\n");
    return S_FALSE;
}

/******************************************************************************
 * Initialisation
 */

/* Choose which driver is considered the primary display driver. It will
 * be created when we get a NULL guid for the DirectDrawCreate(Ex). */
static int DDRAW_ChooseDefaultDriver(void)
{
    int i;
    int best = 0;
    int best_score = 0;

    assert(DDRAW_num_drivers > 0);

    /* This algorithm is really stupid. */
    for (i=0; i < DDRAW_num_drivers; i++)
    {
	if (DDRAW_drivers[i]->preference > best_score)
	{
	    best_score = DDRAW_drivers[i]->preference;
	    best = i;
	}
    }

    assert(best_score > 0);

    return best;
}

/***********************************************************************
 *		DllMain (DDRAW.0)
 */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    /* If we were sufficiently cool, DDraw drivers would just be COM
     * objects, registered with a particular component category. */

    DDRAW_HAL_Init(hInstDLL, fdwReason, lpv);
    DDRAW_User_Init(hInstDLL, fdwReason, lpv);

    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        HMODULE mod;

        DisableThreadLibraryCalls(hInstDLL);

        mod = GetModuleHandleA( "winex11.drv" );
        if (mod)
        {
            wine_tsx11_lock_ptr   = (void *)GetProcAddress( mod, "wine_tsx11_lock" );
            wine_tsx11_unlock_ptr = (void *)GetProcAddress( mod, "wine_tsx11_unlock" );
        }
#ifdef HAVE_OPENGL
        opengl_initialized = DDRAW_bind_to_opengl();
#endif /* HAVE_OPENGL */
        s3tc_initialized = DDRAW_bind_to_s3tc();

        if (DDRAW_num_drivers > 0)
            DDRAW_default_driver = DDRAW_ChooseDefaultDriver();
    }

    return TRUE;
}

/* Register a direct draw driver. This should be called from your init
 * function. (That's why there is no locking: your init func is called from
 * our DllInit, which is serialised.) */
void DDRAW_register_driver(const ddraw_driver *driver)
{
    int i;

    for (i = 0; i < DDRAW_num_drivers; i++)
    {
	if (DDRAW_drivers[i] == driver)
	{
	    ERR("Driver reregistering %p\n", driver);
	    return;
	}
    }

    if (DDRAW_num_drivers == sizeof(DDRAW_drivers)/sizeof(DDRAW_drivers[0]))
    {
	ERR("too many DDRAW drivers\n");
	return;
    }

    DDRAW_drivers[DDRAW_num_drivers++] = driver;
}

/* This totally doesn't belong here. */
LONG DDRAW_width_bpp_to_pitch(DWORD width, DWORD bpp)
{
    LONG pitch;

    assert(bpp != 0); /* keeps happening... */

    if (bpp == 15) bpp = 16;
    pitch = width * (bpp / 8);
    return pitch + (8 - (pitch % 8)) % 8;
}
