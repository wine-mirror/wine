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

typedef struct
{
    /* IUnknown fields */
    ICOM_VFIELD(IClassFactory);
    DWORD                       ref;
} IClassFactoryImpl;

/******************************************************************
 *		DirectMusic ClassFactory
 */
static HRESULT WINAPI DirectMusicCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI DirectMusicCF_AddRef(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	return ++(This->ref);
}

static ULONG WINAPI DirectMusicCF_Release(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	/* static class, won't be  freed */
	return --(This->ref);
}

static HRESULT WINAPI DirectMusicCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	TRACE ("(%p)->(%p,%s,%p)\n", This, pOuter, debugstr_guid(riid), ppobj);
	if (IsEqualIID (riid, &IID_IDirectMusic) ||
	    IsEqualIID (riid, &IID_IDirectMusic2) ||
	    IsEqualIID (riid, &IID_IDirectMusic8)) {
	  return DMUSIC_CreateDirectMusic (riid, (LPDIRECTMUSIC8*) ppobj, pOuter);
	}
	
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

static HRESULT WINAPI DirectMusicCF_LockServer(LPCLASSFACTORY iface,BOOL dolock)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	FIXME("(%p)->(%d),stub!\n", This, dolock);
	return S_OK;
}

static ICOM_VTABLE(IClassFactory) DirectMusicCF_Vtbl = {
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	DirectMusicCF_QueryInterface,
	DirectMusicCF_AddRef,
	DirectMusicCF_Release,
	DirectMusicCF_CreateInstance,
	DirectMusicCF_LockServer
};

static IClassFactoryImpl DirectMusic_CF = {&DirectMusicCF_Vtbl, 1 };

/******************************************************************
 *		DirectMusicCollection ClassFactory
 */
static HRESULT WINAPI CollectionCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI CollectionCF_AddRef(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	return ++(This->ref);
}

static ULONG WINAPI CollectionCF_Release(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	/* static class, won't be  freed */
	return --(This->ref);
}

static HRESULT WINAPI CollectionCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	TRACE ("(%p)->(%p,%s,%p)\n", This, pOuter, debugstr_guid(riid), ppobj);
	if (IsEqualIID (riid, &IID_IDirectMusicCollection)) {
		return DMUSIC_CreateDirectMusicCollection (riid, (LPDIRECTMUSICCOLLECTION*)ppobj, pOuter);
	} else if (IsEqualIID (riid, &IID_IDirectMusicObject)) {
		return DMUSIC_CreateDirectMusicCollectionObject (riid, (LPDIRECTMUSICOBJECT*)ppobj, pOuter);
	}
	
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

static HRESULT WINAPI CollectionCF_LockServer(LPCLASSFACTORY iface,BOOL dolock)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	FIXME("(%p)->(%d),stub!\n", This, dolock);
	return S_OK;
}

static ICOM_VTABLE(IClassFactory) CollectionCF_Vtbl = {
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	CollectionCF_QueryInterface,
	CollectionCF_AddRef,
	CollectionCF_Release,
	CollectionCF_CreateInstance,
	CollectionCF_LockServer
};

static IClassFactoryImpl Collection_CF = {&CollectionCF_Vtbl, 1 };

/******************************************************************
 *		DllMain
 *
 *
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
            DisableThreadLibraryCalls(hinstDLL);
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
	if (IsEqualCLSID (rclsid, &CLSID_DirectMusic) && IsEqualIID (riid, &IID_IClassFactory)) {
		*ppv = (LPVOID) &DirectMusic_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);
		return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicCollection) && IsEqualIID (riid, &IID_IClassFactory)) {
		*ppv = (LPVOID) &Collection_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);
		return S_OK;
	}
	
    WARN("(%p,%p,%p): no interface found.\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}
