/* DirectMusicScript Main
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

#include "dmscript_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmscript);

typedef struct
{
    /* IUnknown fields */
    ICOM_VFIELD(IClassFactory);
    DWORD                       ref;
} IClassFactoryImpl;

/******************************************************************
 *		DirectMusicScriptAutoImplSegment ClassFactory
 */
static HRESULT WINAPI ScriptAutoImplSegmentCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI ScriptAutoImplSegmentCF_AddRef(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	return ++(This->ref);
}

static ULONG WINAPI ScriptAutoImplSegmentCF_Release(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	/* static class, won't be  freed */
	return --(This->ref);
}

static HRESULT WINAPI ScriptAutoImplSegmentCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	TRACE ("(%p)->(%p,%s,%p)\n", This, pOuter, debugstr_guid(riid), ppobj);
	
	/* nothing here yet */
	
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

static HRESULT WINAPI ScriptAutoImplSegmentCF_LockServer(LPCLASSFACTORY iface,BOOL dolock)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	FIXME("(%p)->(%d),stub!\n", This, dolock);
	return S_OK;
}

static ICOM_VTABLE(IClassFactory) ScriptAutoImplSegmentCF_Vtbl = {
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	ScriptAutoImplSegmentCF_QueryInterface,
	ScriptAutoImplSegmentCF_AddRef,
	ScriptAutoImplSegmentCF_Release,
	ScriptAutoImplSegmentCF_CreateInstance,
	ScriptAutoImplSegmentCF_LockServer
};

static IClassFactoryImpl ScriptAutoImplSegment_CF = {&ScriptAutoImplSegmentCF_Vtbl, 1 };

/******************************************************************
 *		DirectMusicScriptTrack ClassFactory
 */
static HRESULT WINAPI ScriptTrackCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI ScriptTrackCF_AddRef(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	return ++(This->ref);
}

static ULONG WINAPI ScriptTrackCF_Release(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	/* static class, won't be  freed */
	return --(This->ref);
}

static HRESULT WINAPI ScriptTrackCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	TRACE ("(%p)->(%p,%s,%p)\n", This, pOuter, debugstr_guid(riid), ppobj);
	
	if (IsEqualIID (riid, &IID_IDirectMusicTrack) 
		|| IsEqualIID (riid, &IID_IDirectMusicTrack8)) {
		return DMUSIC_CreateDirectMusicScriptTrack (riid, (LPDIRECTMUSICTRACK8*)ppobj, pOuter);
	}
	
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

static HRESULT WINAPI ScriptTrackCF_LockServer(LPCLASSFACTORY iface,BOOL dolock)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	FIXME("(%p)->(%d),stub!\n", This, dolock);
	return S_OK;
}

static ICOM_VTABLE(IClassFactory) ScriptTrackCF_Vtbl = {
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	ScriptTrackCF_QueryInterface,
	ScriptTrackCF_AddRef,
	ScriptTrackCF_Release,
	ScriptTrackCF_CreateInstance,
	ScriptTrackCF_LockServer
};

static IClassFactoryImpl ScriptTrack_CF = {&ScriptTrackCF_Vtbl, 1 };

/******************************************************************
 *		DirectMusicAudioVBScript ClassFactory
 */
static HRESULT WINAPI AudioVBScriptCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI AudioVBScriptCF_AddRef(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	return ++(This->ref);
}

static ULONG WINAPI AudioVBScriptCF_Release(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	/* static class, won't be  freed */
	return --(This->ref);
}

static HRESULT WINAPI AudioVBScriptCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	TRACE ("(%p)->(%p,%s,%p)\n", This, pOuter, debugstr_guid(riid), ppobj);
	
	/* nothing here yet */
	
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

static HRESULT WINAPI AudioVBScriptCF_LockServer(LPCLASSFACTORY iface,BOOL dolock)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	FIXME("(%p)->(%d),stub!\n", This, dolock);
	return S_OK;
}

static ICOM_VTABLE(IClassFactory) AudioVBScriptCF_Vtbl = {
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	AudioVBScriptCF_QueryInterface,
	AudioVBScriptCF_AddRef,
	AudioVBScriptCF_Release,
	AudioVBScriptCF_CreateInstance,
	AudioVBScriptCF_LockServer
};

static IClassFactoryImpl AudioVBScript_CF = {&AudioVBScriptCF_Vtbl, 1 };

/******************************************************************
 *		DirectMusicScript ClassFactory
 */
static HRESULT WINAPI ScriptCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI ScriptCF_AddRef(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	return ++(This->ref);
}

static ULONG WINAPI ScriptCF_Release(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	/* static class, won't be  freed */
	return --(This->ref);
}

static HRESULT WINAPI ScriptCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	TRACE ("(%p)->(%p,%s,%p)\n", This, pOuter, debugstr_guid(riid), ppobj);
	if (IsEqualIID (riid, &IID_IDirectMusicScript)) {
		return DMUSIC_CreateDirectMusicScript (riid, (LPDIRECTMUSICSCRIPT*)ppobj, pOuter);
	} else if (IsEqualIID (riid, &IID_IDirectMusicObject)) {
		return DMUSIC_CreateDirectMusicScriptObject (riid, (LPDIRECTMUSICOBJECT*)ppobj, pOuter);
	}
	
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

static HRESULT WINAPI ScriptCF_LockServer(LPCLASSFACTORY iface,BOOL dolock)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	FIXME("(%p)->(%d),stub!\n", This, dolock);
	return S_OK;
}

static ICOM_VTABLE(IClassFactory) ScriptCF_Vtbl = {
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	ScriptCF_QueryInterface,
	ScriptCF_AddRef,
	ScriptCF_Release,
	ScriptCF_CreateInstance,
	ScriptCF_LockServer
};

static IClassFactoryImpl Script_CF = {&ScriptCF_Vtbl, 1 };

/******************************************************************
 *		DirectMusicScriptAutoImplPerformance ClassFactory
 */
static HRESULT WINAPI ScriptAutoImplPerformanceCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI ScriptAutoImplPerformanceCF_AddRef(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	return ++(This->ref);
}

static ULONG WINAPI ScriptAutoImplPerformanceCF_Release(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	/* static class, won't be  freed */
	return --(This->ref);
}

static HRESULT WINAPI ScriptAutoImplPerformanceCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	TRACE ("(%p)->(%p,%s,%p)\n", This, pOuter, debugstr_guid(riid), ppobj);
	
	/* nothing here yet */
	
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

static HRESULT WINAPI ScriptAutoImplPerformanceCF_LockServer(LPCLASSFACTORY iface,BOOL dolock)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	FIXME("(%p)->(%d),stub!\n", This, dolock);
	return S_OK;
}

static ICOM_VTABLE(IClassFactory) ScriptAutoImplPerformanceCF_Vtbl = {
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	ScriptAutoImplPerformanceCF_QueryInterface,
	ScriptAutoImplPerformanceCF_AddRef,
	ScriptAutoImplPerformanceCF_Release,
	ScriptAutoImplPerformanceCF_CreateInstance,
	ScriptAutoImplPerformanceCF_LockServer
};

static IClassFactoryImpl ScriptAutoImplPerformance_CF = {&ScriptAutoImplPerformanceCF_Vtbl, 1 };

/******************************************************************
 *		DirectMusicScriptSourceCodeLoader ClassFactory
 */
static HRESULT WINAPI ScriptSourceCodeLoaderCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI ScriptSourceCodeLoaderCF_AddRef(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	return ++(This->ref);
}

static ULONG WINAPI ScriptSourceCodeLoaderCF_Release(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	/* static class, won't be  freed */
	return --(This->ref);
}

static HRESULT WINAPI ScriptSourceCodeLoaderCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	TRACE ("(%p)->(%p,%s,%p)\n", This, pOuter, debugstr_guid(riid), ppobj);
	
	/* nothing here yet */
	
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

static HRESULT WINAPI ScriptSourceCodeLoaderCF_LockServer(LPCLASSFACTORY iface,BOOL dolock)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	FIXME("(%p)->(%d),stub!\n", This, dolock);
	return S_OK;
}

static ICOM_VTABLE(IClassFactory) ScriptSourceCodeLoaderCF_Vtbl = {
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	ScriptSourceCodeLoaderCF_QueryInterface,
	ScriptSourceCodeLoaderCF_AddRef,
	ScriptSourceCodeLoaderCF_Release,
	ScriptSourceCodeLoaderCF_CreateInstance,
	ScriptSourceCodeLoaderCF_LockServer
};

static IClassFactoryImpl ScriptSourceCodeLoader_CF = {&ScriptSourceCodeLoaderCF_Vtbl, 1 };

/******************************************************************
 *		DirectMusicScriptAutoImplSegmentState ClassFactory
 */
static HRESULT WINAPI ScriptAutoImplSegmentStateCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI ScriptAutoImplSegmentStateCF_AddRef(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	return ++(This->ref);
}

static ULONG WINAPI ScriptAutoImplSegmentStateCF_Release(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	/* static class, won't be  freed */
	return --(This->ref);
}

static HRESULT WINAPI ScriptAutoImplSegmentStateCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	TRACE ("(%p)->(%p,%s,%p)\n", This, pOuter, debugstr_guid(riid), ppobj);
	
	/* nothing here yet */
	
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

static HRESULT WINAPI ScriptAutoImplSegmentStateCF_LockServer(LPCLASSFACTORY iface,BOOL dolock)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	FIXME("(%p)->(%d),stub!\n", This, dolock);
	return S_OK;
}

static ICOM_VTABLE(IClassFactory) ScriptAutoImplSegmentStateCF_Vtbl = {
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	ScriptAutoImplSegmentStateCF_QueryInterface,
	ScriptAutoImplSegmentStateCF_AddRef,
	ScriptAutoImplSegmentStateCF_Release,
	ScriptAutoImplSegmentStateCF_CreateInstance,
	ScriptAutoImplSegmentStateCF_LockServer
};

static IClassFactoryImpl ScriptAutoImplSegmentState_CF = {&ScriptAutoImplSegmentStateCF_Vtbl, 1 };

/******************************************************************
 *		DirectMusicScriptAutoImplAudioPathConfig ClassFactory
 */
static HRESULT WINAPI ScriptAutoImplAudioPathConfigCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI ScriptAutoImplAudioPathConfigCF_AddRef(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	return ++(This->ref);
}

static ULONG WINAPI ScriptAutoImplAudioPathConfigCF_Release(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	/* static class, won't be  freed */
	return --(This->ref);
}

static HRESULT WINAPI ScriptAutoImplAudioPathConfigCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	TRACE ("(%p)->(%p,%s,%p)\n", This, pOuter, debugstr_guid(riid), ppobj);
	
	/* nothing here yet */
	
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

static HRESULT WINAPI ScriptAutoImplAudioPathConfigCF_LockServer(LPCLASSFACTORY iface,BOOL dolock)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	FIXME("(%p)->(%d),stub!\n", This, dolock);
	return S_OK;
}

static ICOM_VTABLE(IClassFactory) ScriptAutoImplAudioPathConfigCF_Vtbl = {
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	ScriptAutoImplAudioPathConfigCF_QueryInterface,
	ScriptAutoImplAudioPathConfigCF_AddRef,
	ScriptAutoImplAudioPathConfigCF_Release,
	ScriptAutoImplAudioPathConfigCF_CreateInstance,
	ScriptAutoImplAudioPathConfigCF_LockServer
};

static IClassFactoryImpl ScriptAutoImplAudioPathConfig_CF = {&ScriptAutoImplAudioPathConfigCF_Vtbl, 1 };

/******************************************************************
 *		DirectMusicScriptAutoImplAudioPath ClassFactory
 */
static HRESULT WINAPI ScriptAutoImplAudioPathCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI ScriptAutoImplAudioPathCF_AddRef(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	return ++(This->ref);
}

static ULONG WINAPI ScriptAutoImplAudioPathCF_Release(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	/* static class, won't be  freed */
	return --(This->ref);
}

static HRESULT WINAPI ScriptAutoImplAudioPathCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	TRACE ("(%p)->(%p,%s,%p)\n", This, pOuter, debugstr_guid(riid), ppobj);
	
	/* nothing here yet */
	
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

static HRESULT WINAPI ScriptAutoImplAudioPathCF_LockServer(LPCLASSFACTORY iface,BOOL dolock)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	FIXME("(%p)->(%d),stub!\n", This, dolock);
	return S_OK;
}

static ICOM_VTABLE(IClassFactory) ScriptAutoImplAudioPathCF_Vtbl = {
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	ScriptAutoImplAudioPathCF_QueryInterface,
	ScriptAutoImplAudioPathCF_AddRef,
	ScriptAutoImplAudioPathCF_Release,
	ScriptAutoImplAudioPathCF_CreateInstance,
	ScriptAutoImplAudioPathCF_LockServer
};

static IClassFactoryImpl ScriptAutoImplAudioPath_CF = {&ScriptAutoImplAudioPathCF_Vtbl, 1 };

/******************************************************************
 *		DirectMusicScriptAutoImplSong ClassFactory
 */
static HRESULT WINAPI ScriptAutoImplSongCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI ScriptAutoImplSongCF_AddRef(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	return ++(This->ref);
}

static ULONG WINAPI ScriptAutoImplSongCF_Release(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	/* static class, won't be  freed */
	return --(This->ref);
}

static HRESULT WINAPI ScriptAutoImplSongCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	TRACE ("(%p)->(%p,%s,%p)\n", This, pOuter, debugstr_guid(riid), ppobj);
	
	/* nothing here yet */
	
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

static HRESULT WINAPI ScriptAutoImplSongCF_LockServer(LPCLASSFACTORY iface,BOOL dolock)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	FIXME("(%p)->(%d),stub!\n", This, dolock);
	return S_OK;
}

static ICOM_VTABLE(IClassFactory) ScriptAutoImplSongCF_Vtbl = {
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	ScriptAutoImplSongCF_QueryInterface,
	ScriptAutoImplSongCF_AddRef,
	ScriptAutoImplSongCF_Release,
	ScriptAutoImplSongCF_CreateInstance,
	ScriptAutoImplSongCF_LockServer
};

static IClassFactoryImpl ScriptAutoImplSong_CF = {&ScriptAutoImplSongCF_Vtbl, 1 };

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
 *		DllCanUnloadNow (DMSCRIPT.1)
 *
 *
 */
HRESULT WINAPI DMSCRIPT_DllCanUnloadNow(void)
{
    FIXME("(void): stub\n");

    return S_FALSE;
}


/******************************************************************
 *		DllGetClassObject (DMSCRIPT.2)
 *
 *
 */
HRESULT WINAPI DMSCRIPT_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    TRACE("(%p,%p,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    if (IsEqualCLSID (rclsid, &CLSID_DirectMusicScriptAutoImpSegment) && IsEqualIID (riid, &IID_IClassFactory)) {
      *ppv = (LPVOID) &ScriptAutoImplSegment_CF;
      IClassFactory_AddRef((IClassFactory*)*ppv);
      return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicScriptTrack) && IsEqualIID (riid, &IID_IClassFactory)) {
      *ppv = (LPVOID) &ScriptTrack_CF;
      IClassFactory_AddRef((IClassFactory*)*ppv);
      return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_AudioVBScript) && IsEqualIID (riid, &IID_IClassFactory)) {
      *ppv = (LPVOID) &AudioVBScript_CF;
      IClassFactory_AddRef((IClassFactory*)*ppv);
      return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicScript) && IsEqualIID (riid, &IID_IClassFactory)) {
      *ppv = (LPVOID) &Script_CF;
      IClassFactory_AddRef((IClassFactory*)*ppv);
      return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicScriptAutoImpPerformance) && IsEqualIID (riid, &IID_IClassFactory)) {
      *ppv = (LPVOID) &ScriptAutoImplPerformance_CF;
      IClassFactory_AddRef((IClassFactory*)*ppv);
      return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicScripSourceCodeLoader) && IsEqualIID (riid, &IID_IClassFactory)) {
      *ppv = (LPVOID) &ScriptSourceCodeLoader_CF;
      IClassFactory_AddRef((IClassFactory*)*ppv);
      return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicScriptAutoImpSegmentState) && IsEqualIID (riid, &IID_IClassFactory)) {
      *ppv = (LPVOID) &ScriptAutoImplSegmentState_CF;
      IClassFactory_AddRef((IClassFactory*)*ppv);
      return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicScriptAutoImpAudioPathConfig) && IsEqualIID (riid, &IID_IClassFactory)) {
      *ppv = (LPVOID) &ScriptAutoImplAudioPathConfig_CF;
      IClassFactory_AddRef((IClassFactory*)*ppv);
      return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicScriptAutoImpAudioPath) && IsEqualIID (riid, &IID_IClassFactory)) {
      *ppv = (LPVOID) &ScriptAutoImplAudioPath_CF;
      IClassFactory_AddRef((IClassFactory*)*ppv);
      return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicScriptAutoImpSong) && IsEqualIID (riid, &IID_IClassFactory)) {
      *ppv = (LPVOID) &ScriptAutoImplSong_CF;
      IClassFactory_AddRef((IClassFactory*)*ppv);
      return S_OK;
	}		

    WARN("(%p,%p,%p): no interface found.\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}
