/* DirectMusicBand Main
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
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

#include "dmband_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmband);

typedef struct {
    /* IUnknown fields */
    IClassFactoryVtbl          *lpVtbl;
    DWORD                       ref;
} IClassFactoryImpl;

/******************************************************************
 *		DirectMusicBand ClassFactory
 */
 
static HRESULT WINAPI BandCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
	FIXME("(%p, %s, %p): stub\n",This, debugstr_dmguid(riid), ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI BandCF_AddRef(LPCLASSFACTORY iface) {
	IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
	return ++(This->ref);
}

static ULONG WINAPI BandCF_Release(LPCLASSFACTORY iface) {
	IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
	/* static class, won't be  freed */
	return --(This->ref);
}

static HRESULT WINAPI BandCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj) {
	IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
	TRACE ("(%p, %p, %s, %p)\n", This, pOuter, debugstr_dmguid(riid), ppobj);
	return DMUSIC_CreateDirectMusicBandImpl (riid, ppobj, pOuter);
}

static HRESULT WINAPI BandCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
	FIXME("(%p, %d): stub\n", This, dolock);
	return S_OK;
}

static IClassFactoryVtbl BandCF_Vtbl = {
	BandCF_QueryInterface,
	BandCF_AddRef,
	BandCF_Release,
	BandCF_CreateInstance,
	BandCF_LockServer
};

static IClassFactoryImpl Band_CF = {&BandCF_Vtbl, 1 };


/******************************************************************
 *		DirectMusicBandTrack ClassFactory
 */
 
static HRESULT WINAPI BandTrackCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	IClassFactoryImpl *This = (IClassFactoryImpl *)iface;

	FIXME("(%p, %s, %p): stub\n", This, debugstr_dmguid(riid), ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI BandTrackCF_AddRef(LPCLASSFACTORY iface) {
	IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
	return ++(This->ref);
}

static ULONG WINAPI BandTrackCF_Release(LPCLASSFACTORY iface) {
	IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
	/* static class, won't be  freed */
	return --(This->ref);
}

static HRESULT WINAPI BandTrackCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj) {
	IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
	TRACE ("(%p, %p, %s, %p)\n", This, pOuter, debugstr_dmguid(riid), ppobj);
	return DMUSIC_CreateDirectMusicBandTrack (riid, ppobj, pOuter);
}

static HRESULT WINAPI BandTrackCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
	FIXME("(%p, %d): stub\n", This, dolock);
	return S_OK;
}

static IClassFactoryVtbl BandTrackCF_Vtbl = {
	BandTrackCF_QueryInterface,
	BandTrackCF_AddRef,
	BandTrackCF_Release,
	BandTrackCF_CreateInstance,
	BandTrackCF_LockServer
};

static IClassFactoryImpl BandTrack_CF = {&BandTrackCF_Vtbl, 1 };

/******************************************************************
 *		DllMain
 *
 *
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	if (fdwReason == DLL_PROCESS_ATTACH) {
            DisableThreadLibraryCalls(hinstDLL);
		/* FIXME: Initialisation */
	} else if (fdwReason == DLL_PROCESS_DETACH) {
		/* FIXME: Cleanup */
	}

	return TRUE;
}


/******************************************************************
 *		DllCanUnloadNow (DMBAND.1)
 *
 *
 */
HRESULT WINAPI DMBAND_DllCanUnloadNow(void) {
    FIXME("(void): stub\n");
    return S_FALSE;
}


/******************************************************************
 *		DllGetClassObject (DMBAND.2)
 *
 *
 */
HRESULT WINAPI DMBAND_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv) {
    TRACE("(%s, %s, %p)\n", debugstr_dmguid(rclsid), debugstr_dmguid(riid), ppv);

	if (IsEqualCLSID (rclsid, &CLSID_DirectMusicBand) && IsEqualIID (riid, &IID_IClassFactory)) {
		*ppv = (LPVOID) &Band_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);
		return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicBandTrack) && IsEqualIID (riid, &IID_IClassFactory)) {
		*ppv = (LPVOID) &BandTrack_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);
		return S_OK;	
	}
	
    WARN("(%s, %s, %p): no interface found.\n", debugstr_dmguid(rclsid), debugstr_dmguid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}
