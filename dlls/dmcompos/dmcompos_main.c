/* DirectMusicComposer Main
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

#include "dmcompos_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmcompos);

typedef struct {
    /* IUnknown fields */
    ICOM_VFIELD(IClassFactory);
    DWORD                       ref;
} IClassFactoryImpl;

/******************************************************************
 *		DirectMusicChordMap ClassFactory
 */
static HRESULT WINAPI ChordMapCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	ICOM_THIS(IClassFactoryImpl,iface);
	FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI ChordMapCF_AddRef(LPCLASSFACTORY iface) {
	ICOM_THIS(IClassFactoryImpl,iface);
	return ++(This->ref);
}

static ULONG WINAPI ChordMapCF_Release(LPCLASSFACTORY iface) {
	ICOM_THIS(IClassFactoryImpl,iface);
	/* static class, won't be  freed */
	return --(This->ref);
}

static HRESULT WINAPI ChordMapCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS(IClassFactoryImpl,iface);
	TRACE ("(%p)->(%p,%s,%p)\n", This, pOuter, debugstr_guid(riid), ppobj);
	return DMUSIC_CreateDirectMusicChordMapImpl (riid, ppobj, pOuter);
}

static HRESULT WINAPI ChordMapCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	ICOM_THIS(IClassFactoryImpl,iface);
	FIXME("(%p)->(%d),stub!\n", This, dolock);
	return S_OK;
}

static ICOM_VTABLE(IClassFactory) ChordMapCF_Vtbl = {
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	ChordMapCF_QueryInterface,
	ChordMapCF_AddRef,
	ChordMapCF_Release,
	ChordMapCF_CreateInstance,
	ChordMapCF_LockServer
};

static IClassFactoryImpl ChordMap_CF = {&ChordMapCF_Vtbl, 1 };

/******************************************************************
 *		DirectMusicComposer ClassFactory
 */
static HRESULT WINAPI ComposerCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	ICOM_THIS(IClassFactoryImpl,iface);
	FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI ComposerCF_AddRef(LPCLASSFACTORY iface) {
	ICOM_THIS(IClassFactoryImpl,iface);
	return ++(This->ref);
}

static ULONG WINAPI ComposerCF_Release(LPCLASSFACTORY iface) {
	ICOM_THIS(IClassFactoryImpl,iface);
	/* static class, won't be  freed */
	return --(This->ref);
}

static HRESULT WINAPI ComposerCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS(IClassFactoryImpl,iface);
	TRACE ("(%p)->(%p,%s,%p)\n", This, pOuter, debugstr_guid(riid), ppobj);
	return DMUSIC_CreateDirectMusicComposerImpl (riid, ppobj, pOuter);
}

static HRESULT WINAPI ComposerCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	ICOM_THIS(IClassFactoryImpl,iface);
	FIXME("(%p)->(%d),stub!\n", This, dolock);
	return S_OK;
}

static ICOM_VTABLE(IClassFactory) ComposerCF_Vtbl = {
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	ComposerCF_QueryInterface,
	ComposerCF_AddRef,
	ComposerCF_Release,
	ComposerCF_CreateInstance,
	ComposerCF_LockServer
};

static IClassFactoryImpl Composer_CF = {&ComposerCF_Vtbl, 1 };

/******************************************************************
 *		DirectMusicChordMapTrack ClassFactory
 */
static HRESULT WINAPI ChordMapTrackCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	ICOM_THIS(IClassFactoryImpl,iface);
	FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI ChordMapTrackCF_AddRef(LPCLASSFACTORY iface) {
	ICOM_THIS(IClassFactoryImpl,iface);
	return ++(This->ref);
}

static ULONG WINAPI ChordMapTrackCF_Release(LPCLASSFACTORY iface) {
	ICOM_THIS(IClassFactoryImpl,iface);
	/* static class, won't be  freed */
	return --(This->ref);
}

static HRESULT WINAPI ChordMapTrackCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS(IClassFactoryImpl,iface);
	TRACE ("(%p)->(%p,%s,%p)\n", This, pOuter, debugstr_guid(riid), ppobj);
	return DMUSIC_CreateDirectMusicChordMapTrack (riid, ppobj, pOuter);
}

static HRESULT WINAPI ChordMapTrackCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	ICOM_THIS(IClassFactoryImpl,iface);
	FIXME("(%p)->(%d),stub!\n", This, dolock);
	return S_OK;
}

static ICOM_VTABLE(IClassFactory) ChordMapTrackCF_Vtbl = {
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	ChordMapTrackCF_QueryInterface,
	ChordMapTrackCF_AddRef,
	ChordMapTrackCF_Release,
	ChordMapTrackCF_CreateInstance,
	ChordMapTrackCF_LockServer
};

static IClassFactoryImpl ChordMapTrack_CF = {&ChordMapTrackCF_Vtbl, 1 };

/******************************************************************
 *		DirectMusicTemplate ClassFactory
 */
static HRESULT WINAPI TemplateCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	ICOM_THIS(IClassFactoryImpl,iface);
	FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI TemplateCF_AddRef(LPCLASSFACTORY iface) {
	ICOM_THIS(IClassFactoryImpl,iface);
	return ++(This->ref);
}

static ULONG WINAPI TemplateCF_Release(LPCLASSFACTORY iface) {
	ICOM_THIS(IClassFactoryImpl,iface);
	/* static class, won't be  freed */
	return --(This->ref);
}

static HRESULT WINAPI TemplateCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS(IClassFactoryImpl,iface);
	TRACE ("(%p)->(%p,%s,%p)\n", This, pOuter, debugstr_guid(riid), ppobj);
	/* nothing yet */
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

static HRESULT WINAPI TemplateCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	ICOM_THIS(IClassFactoryImpl,iface);
	FIXME("(%p)->(%d),stub!\n", This, dolock);
	return S_OK;
}

static ICOM_VTABLE(IClassFactory) TemplateCF_Vtbl = {
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	TemplateCF_QueryInterface,
	TemplateCF_AddRef,
	TemplateCF_Release,
	TemplateCF_CreateInstance,
	TemplateCF_LockServer
};

static IClassFactoryImpl Template_CF = {&TemplateCF_Vtbl, 1 };

/******************************************************************
 *		DirectMusicSignPostTrack ClassFactory
 */
static HRESULT WINAPI SignPostTrackCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	ICOM_THIS(IClassFactoryImpl,iface);
	FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI SignPostTrackCF_AddRef(LPCLASSFACTORY iface) {
	ICOM_THIS(IClassFactoryImpl,iface);
	return ++(This->ref);
}

static ULONG WINAPI SignPostTrackCF_Release(LPCLASSFACTORY iface) {
	ICOM_THIS(IClassFactoryImpl,iface);
	/* static class, won't be  freed */
	return --(This->ref);
}

static HRESULT WINAPI SignPostTrackCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS(IClassFactoryImpl,iface);
	TRACE ("(%p)->(%p,%s,%p)\n", This, pOuter, debugstr_guid(riid), ppobj);
	return DMUSIC_CreateDirectMusicSignPostTrack (riid, ppobj, pOuter);
}

static HRESULT WINAPI SignPostTrackCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	ICOM_THIS(IClassFactoryImpl,iface);
	FIXME("(%p)->(%d),stub!\n", This, dolock);
	return S_OK;
}

static ICOM_VTABLE(IClassFactory) SignPostTrackCF_Vtbl = {
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	SignPostTrackCF_QueryInterface,
	SignPostTrackCF_AddRef,
	SignPostTrackCF_Release,
	SignPostTrackCF_CreateInstance,
	SignPostTrackCF_LockServer
};

static IClassFactoryImpl SignPostTrack_CF = {&SignPostTrackCF_Vtbl, 1 };

/******************************************************************
 *		DllMain
 *
 *
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	if (fdwReason == DLL_PROCESS_ATTACH) 	{
            DisableThreadLibraryCalls(hinstDLL);
		/* FIXME: Initialisation */
	}
	else if (fdwReason == DLL_PROCESS_DETACH) 	{
		/* FIXME: Cleanup */
	}

	return TRUE;
}


/******************************************************************
 *		DllCanUnloadNow (DMCOMPOS.1)
 *
 *
 */
HRESULT WINAPI DMCOMPOS_DllCanUnloadNow(void) {
    FIXME("(void): stub\n");
    return S_FALSE;
}


/******************************************************************
 *		DllGetClassObject (DMCOMPOS.2)
 *
 *
 */
HRESULT WINAPI DMCOMPOS_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv) {
    TRACE("(%p,%p,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    if (IsEqualCLSID (rclsid, &CLSID_DirectMusicChordMap) && IsEqualIID (riid, &IID_IClassFactory)) {
		*ppv = (LPVOID) &ChordMap_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);
		return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicComposer) && IsEqualIID (riid, &IID_IClassFactory)) { 
		*ppv = (LPVOID) &Composer_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);		
		return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicChordMapTrack) && IsEqualIID (riid, &IID_IClassFactory)) {
		*ppv = (LPVOID) &ChordMapTrack_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);		
		return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicTemplate) && IsEqualIID (riid, &IID_IClassFactory)) {
		*ppv = (LPVOID) &Template_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);		
		return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicSignPostTrack) && IsEqualIID (riid, &IID_IClassFactory)) {
		*ppv = (LPVOID) &SignPostTrack_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);		
		return S_OK;
	}

    WARN("(%p,%p,%p): no interface found.\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}
