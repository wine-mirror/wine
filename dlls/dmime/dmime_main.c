/* DirectMusicInteractiveEngine Main
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

#include "dmime_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);


/******************************************************************
 *		DirectMusicInteractiveEngine ClassFactory
 *
 *
 */
 
typedef struct
{
    /* IUnknown fields */
    ICOM_VFIELD(IClassFactory);
    DWORD                       ref;
} IClassFactoryImpl;

static HRESULT WINAPI DMIMECF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI DMIMECF_AddRef(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	return ++(This->ref);
}

static ULONG WINAPI DMIMECF_Release(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	/* static class, won't be  freed */
	return --(This->ref);
}

static HRESULT WINAPI DMIMECF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	TRACE ("(%p)->(%p,%s,%p)\n", This, pOuter, debugstr_guid(riid), ppobj);
	if (IsEqualGUID (riid, &IID_IDirectMusicPerformance) ||
	    IsEqualGUID (riid, &IID_IDirectMusicPerformance8)) {
		return DMUSIC_CreateDirectMusicPerformance (riid, (LPDIRECTMUSICPERFORMANCE8*) ppobj, pOuter);
	} else if (IsEqualGUID (riid, &IID_IDirectMusicSegment) ||
		IsEqualGUID (riid, &IID_IDirectMusicSegment8)) {
		return DMUSIC_CreateDirectMusicSegment (riid, (LPDIRECTMUSICSEGMENT8*) ppobj, pOuter);
	} else if (IsEqualGUID (riid, &IID_IDirectMusicSegmentState) ||
		IsEqualGUID (riid, &IID_IDirectMusicSegmentState8)) {
		return DMUSIC_CreateDirectMusicSegmentState (riid, (LPDIRECTMUSICSEGMENTSTATE8*) ppobj, pOuter);
	} else if (IsEqualGUID (riid, &IID_IDirectMusicGraph)) {
		return DMUSIC_CreateDirectMusicGraph (riid, (LPDIRECTMUSICGRAPH*) ppobj, pOuter);
	} else if (IsEqualGUID (riid, &IID_IDirectMusicAudioPath)) {
		return DMUSIC_CreateDirectMusicSong (riid, (LPDIRECTMUSICSONG*) ppobj, pOuter);
	} else if (IsEqualGUID (riid, &IID_IDirectMusicAudioPath)) {
		return DMUSIC_CreateDirectMusicAudioPath (riid, (LPDIRECTMUSICAUDIOPATH*) ppobj, pOuter);
	} else if (IsEqualGUID (riid, &IID_IDirectMusicTool) ||
		IsEqualGUID (riid, &IID_IDirectMusicTool8)) {
		return DMUSIC_CreateDirectMusicTool (riid, (LPDIRECTMUSICTOOL8*) ppobj, pOuter);
	} else if (IsEqualGUID (riid, &IID_IDirectMusicTrack) ||
		IsEqualGUID (riid, &IID_IDirectMusicTrack8)) {
		return DMUSIC_CreateDirectMusicTrack (riid, (LPDIRECTMUSICTRACK8*) ppobj, pOuter);
	} else if (IsEqualGUID (riid, &IID_IDirectMusicPatternTrack)) {
		return DMUSIC_CreateDirectMusicPatternTrack (riid, (LPDIRECTMUSICPATTERNTRACK*) ppobj, pOuter);
	}

	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);	
	return E_NOINTERFACE;
}

static HRESULT WINAPI DMIMECF_LockServer(LPCLASSFACTORY iface,BOOL dolock)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	FIXME("(%p)->(%d),stub!\n", This, dolock);
	return S_OK;
}

static ICOM_VTABLE(IClassFactory) DMIMECF_Vtbl = {
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	DMIMECF_QueryInterface,
	DMIMECF_AddRef,
	DMIMECF_Release,
	DMIMECF_CreateInstance,
	DMIMECF_LockServer
};

static IClassFactoryImpl DMIME_CF = {&DMIMECF_Vtbl, 1 };

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
 *		DllCanUnloadNow (DMIME.1)
 *
 *
 */
HRESULT WINAPI DMIME_DllCanUnloadNow(void)
{
    FIXME("(void): stub\n");

    return S_FALSE;
}


/******************************************************************
 *		DllGetClassObject (DMIME.2)
 *
 *
 */
HRESULT WINAPI DMIME_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    TRACE("(%p,%p,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    if (IsEqualCLSID (&IID_IClassFactory, riid)) {
      *ppv = (LPVOID) &DMIME_CF;
      IClassFactory_AddRef((IClassFactory*)*ppv);
      return S_OK;
    }
    WARN("(%p,%p,%p): no interface found.\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}
