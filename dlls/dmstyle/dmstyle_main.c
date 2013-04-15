/* DirectMusicStyle Main
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "dmstyle_private.h"
#include "rpcproxy.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmstyle);

static HINSTANCE instance;
LONG DMSTYLE_refCount = 0;

typedef struct {
        IClassFactory IClassFactory_iface;
        HRESULT WINAPI (*fnCreateInstance)(REFIID riid, void **ppv, IUnknown *pUnkOuter);
} IClassFactoryImpl;

static HRESULT WINAPI create_direct_music_section(REFIID riid, void **ppv, IUnknown *pUnkOuter)
{
        FIXME("(%p, %s, %p) stub\n", pUnkOuter, debugstr_dmguid(riid), ppv);

        return E_NOINTERFACE;
}

/******************************************************************
 *      IClassFactory implementation
 */
static inline IClassFactoryImpl *impl_from_IClassFactory(IClassFactory *iface)
{
        return CONTAINING_RECORD(iface, IClassFactoryImpl, IClassFactory_iface);
}

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
        if (ppv == NULL)
                return E_POINTER;

        if (IsEqualGUID(&IID_IUnknown, riid))
                TRACE("(%p)->(IID_IUnknown %p)\n", iface, ppv);
        else if (IsEqualGUID(&IID_IClassFactory, riid))
                TRACE("(%p)->(IID_IClassFactory %p)\n", iface, ppv);
        else {
                FIXME("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);
                *ppv = NULL;
                return E_NOINTERFACE;
        }

        *ppv = iface;
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
        DMSTYLE_LockModule();

        return 2; /* non-heap based object */
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
        DMSTYLE_UnlockModule();

        return 1; /* non-heap based object */
}

static HRESULT WINAPI ClassFactory_CreateInstance(IClassFactory *iface, IUnknown *pUnkOuter,
        REFIID riid, void **ppv)
{
        IClassFactoryImpl *This = impl_from_IClassFactory(iface);

        TRACE ("(%p, %s, %p)\n", pUnkOuter, debugstr_dmguid(riid), ppv);

        return This->fnCreateInstance(riid, ppv, pUnkOuter);
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL dolock)
{
        TRACE("(%d)\n", dolock);

        if (dolock)
                DMSTYLE_LockModule();
        else
                DMSTYLE_UnlockModule();

        return S_OK;
}

static const IClassFactoryVtbl classfactory_vtbl = {
        ClassFactory_QueryInterface,
        ClassFactory_AddRef,
        ClassFactory_Release,
        ClassFactory_CreateInstance,
        ClassFactory_LockServer
};

static IClassFactoryImpl Section_CF = {{&classfactory_vtbl}, create_direct_music_section};
static IClassFactoryImpl Style_CF = {{&classfactory_vtbl}, DMUSIC_CreateDirectMusicStyleImpl};
static IClassFactoryImpl ChordTrack_CF = {{&classfactory_vtbl}, DMUSIC_CreateDirectMusicChordTrack};
static IClassFactoryImpl CommandTrack_CF = {{&classfactory_vtbl},
                                            DMUSIC_CreateDirectMusicCommandTrack};
static IClassFactoryImpl StyleTrack_CF = {{&classfactory_vtbl}, DMUSIC_CreateDirectMusicStyleTrack};
static IClassFactoryImpl MotifTrack_CF = {{&classfactory_vtbl}, DMUSIC_CreateDirectMusicMotifTrack};
static IClassFactoryImpl AuditionTrack_CF = {{&classfactory_vtbl},
                                             DMUSIC_CreateDirectMusicAuditionTrack};
static IClassFactoryImpl MuteTrack_CF = {{&classfactory_vtbl}, DMUSIC_CreateDirectMusicMuteTrack};

/******************************************************************
 *		DllMain
 *
 *
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	if (fdwReason == DLL_PROCESS_ATTACH) {
            instance = hinstDLL;
            DisableThreadLibraryCalls(hinstDLL);
	}

	return TRUE;
}


/******************************************************************
 *		DllCanUnloadNow (DMSTYLE.1)
 *
 *
 */
HRESULT WINAPI DllCanUnloadNow(void) {
	return DMSTYLE_refCount != 0 ? S_FALSE : S_OK;
}


/******************************************************************
 *		DllGetClassObject (DMSTYLE.@)
 *
 *
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv) {
    TRACE("(%s, %s, %p)\n", debugstr_dmguid(rclsid), debugstr_dmguid(riid), ppv);
    
	if (IsEqualCLSID (rclsid, &CLSID_DirectMusicSection) && IsEqualIID (riid, &IID_IClassFactory)) {
                *ppv = &Section_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);
		return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicStyle) && IsEqualIID (riid, &IID_IClassFactory)) {
                *ppv = &Style_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);
		return S_OK;		
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicChordTrack) && IsEqualIID (riid, &IID_IClassFactory)) {
                *ppv = &ChordTrack_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);
		return S_OK;	
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicCommandTrack) && IsEqualIID (riid, &IID_IClassFactory)) {
                *ppv = &CommandTrack_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);
		return S_OK;		
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicStyleTrack) && IsEqualIID (riid, &IID_IClassFactory)) {
                *ppv = &StyleTrack_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);
		return S_OK;		
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicMotifTrack) && IsEqualIID (riid, &IID_IClassFactory)) {
                *ppv = &MotifTrack_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);
		return S_OK;		
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicAuditionTrack) && IsEqualIID (riid, &IID_IClassFactory)) {
                *ppv = &AuditionTrack_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);
		return S_OK;		
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicMuteTrack) && IsEqualIID (riid, &IID_IClassFactory)) {
                *ppv = &MuteTrack_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);
		return S_OK;		
	}

    WARN("(%s, %s, %p): no interface found.\n", debugstr_dmguid(rclsid), debugstr_dmguid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *		DllRegisterServer (DMSTYLE.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources( instance );
}

/***********************************************************************
 *		DllUnregisterServer (DMSTYLE.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources( instance );
}
