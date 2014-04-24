/* DirectMusicScript Main
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "objbase.h"
#include "rpcproxy.h"
#include "initguid.h"
#include "dmusici.h"

#include "dmscript_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmscript);

static HINSTANCE instance;
LONG DMSCRIPT_refCount = 0;

typedef struct {
        IClassFactory IClassFactory_iface;
        HRESULT WINAPI (*fnCreateInstance)(REFIID riid, void **ppv, IUnknown *pUnkOuter);
} IClassFactoryImpl;

static HRESULT WINAPI create_unimpl_instance(REFIID riid, void **ppv, IUnknown *pUnkOuter)
{
        FIXME("(%p, %s, %p) stub\n", pUnkOuter, debugstr_dmguid(riid), ppv);

        return CLASS_E_CLASSNOTAVAILABLE;
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
        DMSCRIPT_LockModule();

        return 2; /* non-heap based object */
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
        DMSCRIPT_UnlockModule();

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
                DMSCRIPT_LockModule();
        else
                DMSCRIPT_UnlockModule();

        return S_OK;
}

static const IClassFactoryVtbl classfactory_vtbl = {
        ClassFactory_QueryInterface,
        ClassFactory_AddRef,
        ClassFactory_Release,
        ClassFactory_CreateInstance,
        ClassFactory_LockServer
};

static IClassFactoryImpl ScriptAutoImplSegment_CF = {{&classfactory_vtbl}, create_unimpl_instance};
static IClassFactoryImpl ScriptTrack_CF = {{&classfactory_vtbl},
                                           DMUSIC_CreateDirectMusicScriptTrack};
static IClassFactoryImpl AudioVBScript_CF = {{&classfactory_vtbl}, create_unimpl_instance};
static IClassFactoryImpl Script_CF = {{&classfactory_vtbl}, DMUSIC_CreateDirectMusicScriptImpl};
static IClassFactoryImpl ScriptAutoImplPerformance_CF = {{&classfactory_vtbl},
                                                         create_unimpl_instance};
static IClassFactoryImpl ScriptSourceCodeLoader_CF = {{&classfactory_vtbl}, create_unimpl_instance};
static IClassFactoryImpl ScriptAutoImplSegmentState_CF = {{&classfactory_vtbl},
                                                          create_unimpl_instance};
static IClassFactoryImpl ScriptAutoImplAudioPathConfig_CF = {{&classfactory_vtbl},
                                                             create_unimpl_instance};
static IClassFactoryImpl ScriptAutoImplAudioPath_CF = {{&classfactory_vtbl},
                                                       create_unimpl_instance};
static IClassFactoryImpl ScriptAutoImplSong_CF = {{&classfactory_vtbl}, create_unimpl_instance};

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
 *		DllCanUnloadNow (DMSCRIPT.@)
 *
 *
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
	return DMSCRIPT_refCount != 0 ? S_FALSE : S_OK;
}


/******************************************************************
 *		DllGetClassObject (DMSCRIPT.@)
 *
 *
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    TRACE("(%s, %s, %p)\n", debugstr_dmguid(rclsid), debugstr_dmguid(riid), ppv);
    if (IsEqualCLSID (rclsid, &CLSID_DirectMusicScriptAutoImpSegment) && IsEqualIID (riid, &IID_IClassFactory)) {
      *ppv = &ScriptAutoImplSegment_CF;
      IClassFactory_AddRef((IClassFactory*)*ppv);
      return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicScriptTrack) && IsEqualIID (riid, &IID_IClassFactory)) {
      *ppv = &ScriptTrack_CF;
      IClassFactory_AddRef((IClassFactory*)*ppv);
      return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_AudioVBScript) && IsEqualIID (riid, &IID_IClassFactory)) {
      *ppv = &AudioVBScript_CF;
      IClassFactory_AddRef((IClassFactory*)*ppv);
      return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicScript) && IsEqualIID (riid, &IID_IClassFactory)) {
      *ppv = &Script_CF;
      IClassFactory_AddRef((IClassFactory*)*ppv);
      return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicScriptAutoImpPerformance) && IsEqualIID (riid, &IID_IClassFactory)) {
      *ppv = &ScriptAutoImplPerformance_CF;
      IClassFactory_AddRef((IClassFactory*)*ppv);
      return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicScriptSourceCodeLoader) && IsEqualIID (riid, &IID_IClassFactory)) {
      *ppv = &ScriptSourceCodeLoader_CF;
      IClassFactory_AddRef((IClassFactory*)*ppv);
      return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicScriptAutoImpSegmentState) && IsEqualIID (riid, &IID_IClassFactory)) {
      *ppv = &ScriptAutoImplSegmentState_CF;
      IClassFactory_AddRef((IClassFactory*)*ppv);
      return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicScriptAutoImpAudioPathConfig) && IsEqualIID (riid, &IID_IClassFactory)) {
      *ppv = &ScriptAutoImplAudioPathConfig_CF;
      IClassFactory_AddRef((IClassFactory*)*ppv);
      return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicScriptAutoImpAudioPath) && IsEqualIID (riid, &IID_IClassFactory)) {
      *ppv = &ScriptAutoImplAudioPath_CF;
      IClassFactory_AddRef((IClassFactory*)*ppv);
      return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicScriptAutoImpSong) && IsEqualIID (riid, &IID_IClassFactory)) {
      *ppv = &ScriptAutoImplSong_CF;
      IClassFactory_AddRef((IClassFactory*)*ppv);
      return S_OK;
	}		

    WARN("(%s, %s, %p): no interface found.\n", debugstr_dmguid(rclsid), debugstr_dmguid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}


/***********************************************************************
 *		DllRegisterServer (DMSCRIPT.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources( instance );
}

/***********************************************************************
 *		DllUnregisterServer (DMSCRIPT.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources( instance );
}

/******************************************************************
 *		Helper functions
 *
 *
 */

/* FOURCC to string conversion for debug messages */
const char *debugstr_fourcc (DWORD fourcc) {
    if (!fourcc) return "'null'";
    return wine_dbg_sprintf ("\'%c%c%c%c\'",
		(char)(fourcc), (char)(fourcc >> 8),
        (char)(fourcc >> 16), (char)(fourcc >> 24));
}

/* DMUS_VERSION struct to string conversion for debug messages */
static const char *debugstr_dmversion (const DMUS_VERSION *version) {
	if (!version) return "'null'";
	return wine_dbg_sprintf ("\'%i,%i,%i,%i\'",
		(int)((version->dwVersionMS & 0xFFFF0000) >> 8), (int)(version->dwVersionMS & 0x0000FFFF), 
		(int)((version->dwVersionLS & 0xFFFF0000) >> 8), (int)(version->dwVersionLS & 0x0000FFFF));
}

/* returns name of given GUID */
const char *debugstr_dmguid (const GUID *id) {
	static const guid_info guids[] = {
		/* CLSIDs */
		GE(CLSID_AudioVBScript),
		GE(CLSID_DirectMusic),
		GE(CLSID_DirectMusicAudioPathConfig),
		GE(CLSID_DirectMusicAuditionTrack),
		GE(CLSID_DirectMusicBand),
		GE(CLSID_DirectMusicBandTrack),
		GE(CLSID_DirectMusicChordMapTrack),
		GE(CLSID_DirectMusicChordMap),
		GE(CLSID_DirectMusicChordTrack),
		GE(CLSID_DirectMusicCollection),
		GE(CLSID_DirectMusicCommandTrack),
		GE(CLSID_DirectMusicComposer),
		GE(CLSID_DirectMusicContainer),
		GE(CLSID_DirectMusicGraph),
		GE(CLSID_DirectMusicLoader),
		GE(CLSID_DirectMusicLyricsTrack),
		GE(CLSID_DirectMusicMarkerTrack),
		GE(CLSID_DirectMusicMelodyFormulationTrack),
		GE(CLSID_DirectMusicMotifTrack),
		GE(CLSID_DirectMusicMuteTrack),
		GE(CLSID_DirectMusicParamControlTrack),
		GE(CLSID_DirectMusicPatternTrack),
		GE(CLSID_DirectMusicPerformance),
		GE(CLSID_DirectMusicScript),
		GE(CLSID_DirectMusicScriptAutoImpSegment),
		GE(CLSID_DirectMusicScriptAutoImpPerformance),
		GE(CLSID_DirectMusicScriptAutoImpSegmentState),
		GE(CLSID_DirectMusicScriptAutoImpAudioPathConfig),
		GE(CLSID_DirectMusicScriptAutoImpAudioPath),
		GE(CLSID_DirectMusicScriptAutoImpSong),
		GE(CLSID_DirectMusicScriptSourceCodeLoader),
		GE(CLSID_DirectMusicScriptTrack),
		GE(CLSID_DirectMusicSection),
		GE(CLSID_DirectMusicSegment),
		GE(CLSID_DirectMusicSegmentState),
		GE(CLSID_DirectMusicSegmentTriggerTrack),
		GE(CLSID_DirectMusicSegTriggerTrack),
		GE(CLSID_DirectMusicSeqTrack),
		GE(CLSID_DirectMusicSignPostTrack),
		GE(CLSID_DirectMusicSong),
		GE(CLSID_DirectMusicStyle),
		GE(CLSID_DirectMusicStyleTrack),
		GE(CLSID_DirectMusicSynth),
		GE(CLSID_DirectMusicSynthSink),
		GE(CLSID_DirectMusicSysExTrack),
		GE(CLSID_DirectMusicTemplate),
		GE(CLSID_DirectMusicTempoTrack),
		GE(CLSID_DirectMusicTimeSigTrack),
		GE(CLSID_DirectMusicWaveTrack),
		GE(CLSID_DirectSoundWave),
		/* IIDs */
		GE(IID_IDirectMusic),
		GE(IID_IDirectMusic2),
		GE(IID_IDirectMusic8),
		GE(IID_IDirectMusicAudioPath),
		GE(IID_IDirectMusicBand),
		GE(IID_IDirectMusicBuffer),
		GE(IID_IDirectMusicChordMap),
		GE(IID_IDirectMusicCollection),
		GE(IID_IDirectMusicComposer),
		GE(IID_IDirectMusicContainer),
		GE(IID_IDirectMusicDownload),
		GE(IID_IDirectMusicDownloadedInstrument),
		GE(IID_IDirectMusicGetLoader),
		GE(IID_IDirectMusicGraph),
		GE(IID_IDirectMusicInstrument),
		GE(IID_IDirectMusicLoader),
		GE(IID_IDirectMusicLoader8),
		GE(IID_IDirectMusicObject),
		GE(IID_IDirectMusicPatternTrack),
		GE(IID_IDirectMusicPerformance),
		GE(IID_IDirectMusicPerformance2),
		GE(IID_IDirectMusicPerformance8),
		GE(IID_IDirectMusicPort),
		GE(IID_IDirectMusicPortDownload),
		GE(IID_IDirectMusicScript),
		GE(IID_IDirectMusicSegment),
		GE(IID_IDirectMusicSegment2),
		GE(IID_IDirectMusicSegment8),
		GE(IID_IDirectMusicSegmentState),
		GE(IID_IDirectMusicSegmentState8),
		GE(IID_IDirectMusicStyle),
		GE(IID_IDirectMusicStyle8),
		GE(IID_IDirectMusicSynth),
		GE(IID_IDirectMusicSynth8),
		GE(IID_IDirectMusicSynthSink),
		GE(IID_IDirectMusicThru),
		GE(IID_IDirectMusicTool),
		GE(IID_IDirectMusicTool8),
		GE(IID_IDirectMusicTrack),
		GE(IID_IDirectMusicTrack8),
		GE(IID_IUnknown),
		GE(IID_IPersistStream),
		GE(IID_IStream),
		GE(IID_IClassFactory),
		/* GUIDs */
		GE(GUID_DirectMusicAllTypes),
		GE(GUID_NOTIFICATION_CHORD),
		GE(GUID_NOTIFICATION_COMMAND),
		GE(GUID_NOTIFICATION_MEASUREANDBEAT),
		GE(GUID_NOTIFICATION_PERFORMANCE),
		GE(GUID_NOTIFICATION_RECOMPOSE),
		GE(GUID_NOTIFICATION_SEGMENT),
		GE(GUID_BandParam),
		GE(GUID_ChordParam),
		GE(GUID_CommandParam),
		GE(GUID_CommandParam2),
		GE(GUID_CommandParamNext),
		GE(GUID_IDirectMusicBand),
		GE(GUID_IDirectMusicChordMap),
		GE(GUID_IDirectMusicStyle),
		GE(GUID_MuteParam),
		GE(GUID_Play_Marker),
		GE(GUID_RhythmParam),
		GE(GUID_TempoParam),
		GE(GUID_TimeSignature),
		GE(GUID_Valid_Start_Time),
		GE(GUID_Clear_All_Bands),
		GE(GUID_ConnectToDLSCollection),
		GE(GUID_Disable_Auto_Download),
		GE(GUID_DisableTempo),
		GE(GUID_DisableTimeSig),
		GE(GUID_Download),
		GE(GUID_DownloadToAudioPath),
		GE(GUID_Enable_Auto_Download),
		GE(GUID_EnableTempo),
		GE(GUID_EnableTimeSig),
		GE(GUID_IgnoreBankSelectForGM),
		GE(GUID_SeedVariations),
		GE(GUID_StandardMIDIFile),
		GE(GUID_Unload),
		GE(GUID_UnloadFromAudioPath),
		GE(GUID_Variations),
		GE(GUID_PerfMasterTempo),
		GE(GUID_PerfMasterVolume),
		GE(GUID_PerfMasterGrooveLevel),
		GE(GUID_PerfAutoDownload),
		GE(GUID_DefaultGMCollection),
		GE(GUID_Synth_Default),
		GE(GUID_Buffer_Reverb),
		GE(GUID_Buffer_EnvReverb),
		GE(GUID_Buffer_Stereo),
		GE(GUID_Buffer_3D_Dry),
		GE(GUID_Buffer_Mono),
		GE(GUID_DMUS_PROP_GM_Hardware),
		GE(GUID_DMUS_PROP_GS_Capable),
		GE(GUID_DMUS_PROP_GS_Hardware),
		GE(GUID_DMUS_PROP_DLS1),
		GE(GUID_DMUS_PROP_DLS2),
		GE(GUID_DMUS_PROP_Effects),
		GE(GUID_DMUS_PROP_INSTRUMENT2),
		GE(GUID_DMUS_PROP_LegacyCaps),
		GE(GUID_DMUS_PROP_MemorySize),
		GE(GUID_DMUS_PROP_SampleMemorySize),
		GE(GUID_DMUS_PROP_SamplePlaybackRate),
		GE(GUID_DMUS_PROP_SetSynthSink),
		GE(GUID_DMUS_PROP_SinkUsesDSound),
		GE(GUID_DMUS_PROP_SynthSink_DSOUND),
		GE(GUID_DMUS_PROP_SynthSink_WAVE),
		GE(GUID_DMUS_PROP_Volume),
		GE(GUID_DMUS_PROP_WavesReverb),
		GE(GUID_DMUS_PROP_WriteLatency),
		GE(GUID_DMUS_PROP_WritePeriod),
		GE(GUID_DMUS_PROP_XG_Capable),
		GE(GUID_DMUS_PROP_XG_Hardware)
	};

	unsigned int i;

        if (!id) return "(null)";

	for (i = 0; i < sizeof(guids)/sizeof(guids[0]); i++) {
		if (IsEqualGUID(id, guids[i].guid))
			return guids[i].name;
	}
	/* if we didn't find it, act like standard debugstr_guid */	
	return debugstr_guid(id);
}	

/* generic flag-dumping function */
static const char* debugstr_flags (DWORD flags, const flag_info* names, size_t num_names){
	char buffer[128] = "", *ptr = &buffer[0];
	unsigned int i;
	int size = sizeof(buffer);
	
	for (i=0; i < num_names; i++)
	{
		if ((flags & names[i].val) ||	/* standard flag*/
			((!flags) && (!names[i].val))) { /* zero value only */
				int cnt = snprintf(ptr, size, "%s ", names[i].name);
				if (cnt < 0 || cnt >= size) break;
				size -= cnt;
				ptr += cnt;
		}
	}
	
	return wine_dbg_sprintf("%s", buffer);
}

/* dump DMUS_OBJ flags */
static const char *debugstr_DMUS_OBJ_FLAGS (DWORD flagmask) {
    static const flag_info flags[] = {
	    FE(DMUS_OBJ_OBJECT),
	    FE(DMUS_OBJ_CLASS),
	    FE(DMUS_OBJ_NAME),
	    FE(DMUS_OBJ_CATEGORY),
	    FE(DMUS_OBJ_FILENAME),
	    FE(DMUS_OBJ_FULLPATH),
	    FE(DMUS_OBJ_URL),
	    FE(DMUS_OBJ_VERSION),
	    FE(DMUS_OBJ_DATE),
	    FE(DMUS_OBJ_LOADED),
	    FE(DMUS_OBJ_MEMORY),
	    FE(DMUS_OBJ_STREAM)
	};
    return debugstr_flags (flagmask, flags, sizeof(flags)/sizeof(flags[0]));
}

/* dump whole DMUS_OBJECTDESC struct */
const char *debugstr_DMUS_OBJECTDESC (LPDMUS_OBJECTDESC pDesc) {
	if (pDesc) {
		char buffer[1024] = "", *ptr = &buffer[0];
		
		ptr += sprintf(ptr, "DMUS_OBJECTDESC (%p):\n", pDesc);
		ptr += sprintf(ptr, " - dwSize = %d\n", pDesc->dwSize);
		ptr += sprintf(ptr, " - dwValidData = %s\n", debugstr_DMUS_OBJ_FLAGS (pDesc->dwValidData));
		if (pDesc->dwValidData & DMUS_OBJ_CLASS) ptr +=	sprintf(ptr, " - guidClass = %s\n", debugstr_dmguid(&pDesc->guidClass));
		if (pDesc->dwValidData & DMUS_OBJ_OBJECT) ptr += sprintf(ptr, " - guidObject = %s\n", debugstr_guid(&pDesc->guidObject));
		if (pDesc->dwValidData & DMUS_OBJ_DATE) ptr += sprintf(ptr, " - ftDate = FIXME\n");
		if (pDesc->dwValidData & DMUS_OBJ_VERSION) ptr += sprintf(ptr, " - vVersion = %s\n", debugstr_dmversion(&pDesc->vVersion));
		if (pDesc->dwValidData & DMUS_OBJ_NAME) ptr += sprintf(ptr, " - wszName = %s\n", debugstr_w(pDesc->wszName));
		if (pDesc->dwValidData & DMUS_OBJ_CATEGORY) ptr += sprintf(ptr, " - wszCategory = %s\n", debugstr_w(pDesc->wszCategory));
		if (pDesc->dwValidData & DMUS_OBJ_FILENAME) ptr += sprintf(ptr, " - wszFileName = %s\n", debugstr_w(pDesc->wszFileName));
		if (pDesc->dwValidData & DMUS_OBJ_MEMORY) ptr += sprintf(ptr, " - llMemLength = 0x%s\n  - pbMemData = %p\n", 
		                                                     wine_dbgstr_longlong(pDesc->llMemLength), pDesc->pbMemData);
		if (pDesc->dwValidData & DMUS_OBJ_STREAM) ptr += sprintf(ptr, " - pStream = %p", pDesc->pStream);

		return wine_dbg_sprintf("%s", buffer);
	} else {
		return wine_dbg_sprintf("(NULL)");
	}
}
