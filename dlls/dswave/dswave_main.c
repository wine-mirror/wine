/* DirectMusic Wave Main
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

#include "dswave_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dswave);

typedef struct {
    /* IUnknown fields */
    ICOM_VFIELD(IClassFactory);
    DWORD                       ref;
} IClassFactoryImpl;

/******************************************************************
 *		DirectMusicWave ClassFactory
 */
static HRESULT WINAPI WaveCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	ICOM_THIS(IClassFactoryImpl,iface);
	FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI WaveCF_AddRef(LPCLASSFACTORY iface) {
	ICOM_THIS(IClassFactoryImpl,iface);
	return ++(This->ref);
}

static ULONG WINAPI WaveCF_Release(LPCLASSFACTORY iface) {
	ICOM_THIS(IClassFactoryImpl,iface);
	/* static class, won't be  freed */
	return --(This->ref);
}

static HRESULT WINAPI WaveCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS(IClassFactoryImpl,iface);
	TRACE ("(%p)->(%p,%s,%p)\n", This, pOuter, debugstr_guid(riid), ppobj);
	return DMUSIC_CreateDirectMusicWaveImpl (riid, ppobj, pOuter);
}

static HRESULT WINAPI WaveCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	ICOM_THIS(IClassFactoryImpl,iface);
	FIXME("(%p)->(%d),stub!\n", This, dolock);
	return S_OK;
}

static ICOM_VTABLE(IClassFactory) WaveCF_Vtbl = {
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	WaveCF_QueryInterface,
	WaveCF_AddRef,
	WaveCF_Release,
	WaveCF_CreateInstance,
	WaveCF_LockServer
};

static IClassFactoryImpl Wave_CF = {&WaveCF_Vtbl, 1 };

/******************************************************************
 *		DllMain
 *
 *
 */
BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	if (fdwReason == DLL_PROCESS_ATTACH) {
            DisableThreadLibraryCalls(hinstDLL);
		/* FIXME: Initialisation */
	} else if (fdwReason == DLL_PROCESS_DETACH) {
		/* FIXME: Cleanup */
	}

	return TRUE;
}


/******************************************************************
 *		DllCanUnloadNow (DSWAVE.1)
 *
 *
 */
HRESULT WINAPI DSWAVE_DllCanUnloadNow(void) {
    FIXME("(void): stub\n");
    return S_FALSE;
}


/******************************************************************
 *		DllGetClassObject (DSWAVE.2)
 *
 *
 */
HRESULT WINAPI DSWAVE_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv) {
	TRACE("(%p,%p,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
	if (IsEqualCLSID (rclsid, &CLSID_DirectSoundWave) && IsEqualIID (riid, &IID_IClassFactory)) {
		*ppv = (LPVOID) &Wave_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);
		return S_OK;
	}
	
    WARN("(%p,%p,%p): no interface found.\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}
