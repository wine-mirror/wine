/* DirectMusic Main
 *
 * Copyright (C) 2003 Rok Mandeljc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "dmusic_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);


/******************************************************************
 *		DirectMusic ClassFactory
 *
 *
 */
 
typedef struct
{
    /* IUnknown fields */
    ICOM_VFIELD(IClassFactory);
    DWORD                       ref;
} IClassFactoryImpl;

static HRESULT WINAPI DMCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI DMCF_AddRef(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	return ++(This->ref);
}

static ULONG WINAPI DMCF_Release(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	/* static class, won't be  freed */
	return --(This->ref);
}

static HRESULT WINAPI DMCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	TRACE ("(%p)->(%p,%s,%p)\n", This, pOuter, debugstr_guid(riid), ppobj);
	if (IsEqualGUID (&IID_IDirectMusic, riid)) {
	  return DMUSIC_CreateDirectMusic (riid, (LPDIRECTMUSIC*) ppobj, pOuter);
	}
	if (IsEqualGUID (&IID_IDirectMusicPerformance, riid) ||
	    IsEqualGUID (&IID_IDirectMusicPerformance8, riid))
	{
	  /*return DMUSIC_CreateDirectMusicPerformance (riid, (LPDIRECTMUSICPERFORMANCE*) ppobj, pOuter);*/
	  return DMUSIC_CreateDirectMusicPerformance8(riid, (LPDIRECTMUSICPERFORMANCE8*) ppobj, pOuter);

	}
	if (IsEqualGUID (&IID_IDirectMusicLoader, riid) ||
	    IsEqualGUID (&IID_IDirectMusicLoader8, riid)) {
	  return DMUSIC_CreateDirectMusicLoader8(riid, (LPDIRECTMUSICLOADER8*) ppobj, pOuter);
	}
	
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

static HRESULT WINAPI DMCF_LockServer(LPCLASSFACTORY iface,BOOL dolock)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	FIXME("(%p)->(%d),stub!\n", This, dolock);
	return S_OK;
}

static ICOM_VTABLE(IClassFactory) DMCF_Vtbl = {
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	DMCF_QueryInterface,
	DMCF_AddRef,
	DMCF_Release,
	DMCF_CreateInstance,
	DMCF_LockServer
};

static IClassFactoryImpl DMUSIC_CF = {&DMCF_Vtbl, 1 };

/******************************************************************
 *		DllMain
 *
 *
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		/* FIXME: Initialisation */
	}
	else if (fdwReason == DLL_PROCESS_DETACH)
	{
		/* FIXME: Cleanup */
	}

	return TRUE;
}


/******************************************************************
 *		DllCanUnloadNow (DMUSIC.1)
 *
 *
 */
HRESULT WINAPI DMUSIC_DllCanUnloadNow(void)
{
    FIXME("(void): stub\n");

    return S_FALSE;
}


/******************************************************************
 *		DllGetClassObject (DMUSIC.2)
 *
 *
 */
HRESULT WINAPI DMUSIC_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    TRACE("(%p,%p,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    if (IsEqualCLSID (&IID_IClassFactory, riid)) {
      *ppv = (LPVOID) &DMUSIC_CF;
      IClassFactory_AddRef((IClassFactory*)*ppv);
      return S_OK;
    }
    WARN("(%p,%p,%p): no interface found.\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}


/******************************************************************
 *		DllRegisterServer (DMUSIC.3)
 *
 *
 */
HRESULT WINAPI DMUSIC_DllRegisterServer(void)
{
    FIXME("(void): stub\n");

    return S_OK;
}


/******************************************************************
 *		DllUnregisterServer (DMUSIC.4)
 *
 *
 */
HRESULT WINAPI DMUSIC_DllUnregisterServer(void)
{
    FIXME("(void): stub\n");

    return S_OK;
}
