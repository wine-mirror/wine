/* DirectMusicInteractiveEngine Main
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
 * Copyright (C) 2003-2004 Raphael Junqueira
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

#include "dmime_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmime);

LONG DMIME_refCount = 0;

typedef struct {
    const IClassFactoryVtbl *lpVtbl;
} IClassFactoryImpl;

/******************************************************************
 *		DirectMusicPerformance ClassFactory
 */
static HRESULT WINAPI PerformanceCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	FIXME("- no interface\n\tIID:\t%s\n", debugstr_guid(riid));

	if (ppobj == NULL) return E_POINTER;

	return E_NOINTERFACE;
}

static ULONG WINAPI PerformanceCF_AddRef(LPCLASSFACTORY iface) {
	DMIME_LockModule();

	return 2; /* non-heap based object */
}

static ULONG WINAPI PerformanceCF_Release(LPCLASSFACTORY iface) {
	DMIME_UnlockModule();

	return 1; /* non-heap based object */
}

static HRESULT WINAPI PerformanceCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj) {
	TRACE ("(%p, %s, %p)\n", pOuter, debugstr_dmguid(riid), ppobj);
	
	return DMUSIC_CreateDirectMusicPerformanceImpl (riid, ppobj, pOuter);
}

static HRESULT WINAPI PerformanceCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	TRACE("(%d)\n", dolock);

	if (dolock)
		DMIME_LockModule();
	else
		DMIME_UnlockModule();
	
	return S_OK;
}

static const IClassFactoryVtbl PerformanceCF_Vtbl = {
	PerformanceCF_QueryInterface,
	PerformanceCF_AddRef,
	PerformanceCF_Release,
	PerformanceCF_CreateInstance,
	PerformanceCF_LockServer
};

static IClassFactoryImpl Performance_CF = {&PerformanceCF_Vtbl};

/******************************************************************
 *		DirectMusicSegment ClassFactory
 */
static HRESULT WINAPI SegmentCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	FIXME("- no interface\n\tIID:\t%s\n", debugstr_guid(riid));

	if (ppobj == NULL) return E_POINTER;
	
	return E_NOINTERFACE;
}

static ULONG WINAPI SegmentCF_AddRef(LPCLASSFACTORY iface) {
	DMIME_LockModule();

	return 2; /* non-heap based object */
}

static ULONG WINAPI SegmentCF_Release(LPCLASSFACTORY iface) {
	DMIME_UnlockModule();

	return 1; /* non-heap based object */
}

static HRESULT WINAPI SegmentCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj) {
	TRACE ("(%p, %s, %p)\n", pOuter, debugstr_dmguid(riid), ppobj);
	
	return DMUSIC_CreateDirectMusicSegmentImpl (riid, ppobj, pOuter);
}

static HRESULT WINAPI SegmentCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	TRACE("(%d)\n", dolock);

	if (dolock)
		DMIME_LockModule();
	else
		DMIME_UnlockModule();
	
	return S_OK;
}

static const IClassFactoryVtbl SegmentCF_Vtbl = {
	SegmentCF_QueryInterface,
	SegmentCF_AddRef,
	SegmentCF_Release,
	SegmentCF_CreateInstance,
	SegmentCF_LockServer
};

static IClassFactoryImpl Segment_CF = {&SegmentCF_Vtbl};

/******************************************************************
 *		DirectMusicSegmentState ClassFactory
 */
static HRESULT WINAPI SegmentStateCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	FIXME("- no interface\n\tIID:\t%s\n", debugstr_guid(riid));

	if (ppobj == NULL) return E_POINTER;
	
	return E_NOINTERFACE;
}

static ULONG WINAPI SegmentStateCF_AddRef(LPCLASSFACTORY iface) {
	DMIME_LockModule();

	return 2; /* non-heap based objects */
}

static ULONG WINAPI SegmentStateCF_Release(LPCLASSFACTORY iface) {
	DMIME_UnlockModule();
	
	return 1; /* non-heap based object */
}

static HRESULT WINAPI SegmentStateCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj) {
	TRACE ("(%p, %s, %p)\n", pOuter, debugstr_dmguid(riid), ppobj);

	return DMUSIC_CreateDirectMusicSegmentStateImpl (riid, ppobj, pOuter);
}

static HRESULT WINAPI SegmentStateCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	TRACE("(%d)\n", dolock);

	if (dolock)
		DMIME_LockModule();
	else
		DMIME_UnlockModule();
	
	return S_OK;
}

static const IClassFactoryVtbl SegmentStateCF_Vtbl = {
	SegmentStateCF_QueryInterface,
	SegmentStateCF_AddRef,
	SegmentStateCF_Release,
	SegmentStateCF_CreateInstance,
	SegmentStateCF_LockServer
};

static IClassFactoryImpl SegmentState_CF = {&SegmentStateCF_Vtbl};

/******************************************************************
 *		DirectMusicGraph ClassFactory
 */
static HRESULT WINAPI GraphCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	FIXME("- no interface\n\tIID:\t%s\n", debugstr_guid(riid));

	if (ppobj == NULL) return E_POINTER;
	
	return E_NOINTERFACE;
}

static ULONG WINAPI GraphCF_AddRef(LPCLASSFACTORY iface) {
	DMIME_LockModule();

	return 2; /* non-heap based object */
}

static ULONG WINAPI GraphCF_Release(LPCLASSFACTORY iface) {
	DMIME_UnlockModule();

	return 1; /* non-heap based object */
}

static HRESULT WINAPI GraphCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj) {
	TRACE ("(%p, %s, %p)\n", pOuter, debugstr_dmguid(riid), ppobj);
	
	return DMUSIC_CreateDirectMusicGraphImpl (riid, ppobj, pOuter);
}

static HRESULT WINAPI GraphCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	TRACE("(%d)\n", dolock);

	if (dolock)
		DMIME_LockModule();
	else
		DMIME_UnlockModule();
	
	return S_OK;
}

static const IClassFactoryVtbl GraphCF_Vtbl = {
	GraphCF_QueryInterface,
	GraphCF_AddRef,
	GraphCF_Release,
	GraphCF_CreateInstance,
	GraphCF_LockServer
};

static IClassFactoryImpl Graph_CF = {&GraphCF_Vtbl};

/******************************************************************
 *		DirectMusicTempoTrack ClassFactory
 */
static HRESULT WINAPI TempoTrackCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	FIXME("- no interface\n\tIID:\t%s\n", debugstr_guid(riid));

	if (ppobj == NULL) return E_POINTER;
	
	return E_NOINTERFACE;
}

static ULONG WINAPI TempoTrackCF_AddRef(LPCLASSFACTORY iface) {
	DMIME_LockModule();

	return 2; /* non-heap based object */
}

static ULONG WINAPI TempoTrackCF_Release(LPCLASSFACTORY iface) {
	DMIME_UnlockModule();

	return 1; /* non-heap based object */
}

static HRESULT WINAPI TempoTrackCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj) {
	TRACE ("(%p, %s, %p)\n", pOuter, debugstr_dmguid(riid), ppobj);
	
	return DMUSIC_CreateDirectMusicTempoTrack (riid, ppobj, pOuter);
}

static HRESULT WINAPI TempoTrackCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	TRACE("(%d)\n", dolock);

	if (dolock)
		DMIME_LockModule();
	else
		DMIME_UnlockModule();
	
	return S_OK;
}

static const IClassFactoryVtbl TempoTrackCF_Vtbl = {
	TempoTrackCF_QueryInterface,
	TempoTrackCF_AddRef,
	TempoTrackCF_Release,
	TempoTrackCF_CreateInstance,
	TempoTrackCF_LockServer
};

static IClassFactoryImpl TempoTrack_CF = {&TempoTrackCF_Vtbl};

/******************************************************************
 *		DirectMusicSeqTrack ClassFactory
 */
static HRESULT WINAPI SeqTrackCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	FIXME("- no interface\n\tIID:\t%s\n", debugstr_guid(riid));

	if (ppobj == NULL) return E_POINTER;
	
	return E_NOINTERFACE;
}

static ULONG WINAPI SeqTrackCF_AddRef(LPCLASSFACTORY iface) {
	DMIME_LockModule();
	
	return 2; /* non-heap based object */
}

static ULONG WINAPI SeqTrackCF_Release(LPCLASSFACTORY iface) {
	DMIME_UnlockModule();

	return 1; /* non-heap based object */
}

static HRESULT WINAPI SeqTrackCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj) {
	TRACE ("(%p, %s, %p)\n", pOuter, debugstr_dmguid(riid), ppobj);
	
	return DMUSIC_CreateDirectMusicSeqTrack (riid, ppobj, pOuter);
}

static HRESULT WINAPI SeqTrackCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	TRACE("(%d)\n", dolock);

	if (dolock)
		DMIME_LockModule();
	else
		DMIME_UnlockModule();
	
	return S_OK;
}

static const IClassFactoryVtbl SeqTrackCF_Vtbl = {
	SeqTrackCF_QueryInterface,
	SeqTrackCF_AddRef,
	SeqTrackCF_Release,
	SeqTrackCF_CreateInstance,
	SeqTrackCF_LockServer
};

static IClassFactoryImpl SeqTrack_CF = {&SeqTrackCF_Vtbl};

/******************************************************************
 *		DirectMusicSysExTrack ClassFactory
 */
static HRESULT WINAPI SysExTrackCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	FIXME("- no interface\n\tIID:\t%s\n", debugstr_guid(riid));

	if (ppobj == NULL) return E_POINTER;
	
	return E_NOINTERFACE;
}

static ULONG WINAPI SysExTrackCF_AddRef(LPCLASSFACTORY iface) {
	DMIME_LockModule();

	return 2; /* non-heap based object */
}

static ULONG WINAPI SysExTrackCF_Release(LPCLASSFACTORY iface) {
	DMIME_UnlockModule();

	return 1; /* non-heap based object */
}

static HRESULT WINAPI SysExTrackCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj) {
	TRACE ("(%p, %s, %p)\n", pOuter, debugstr_dmguid(riid), ppobj);
	return DMUSIC_CreateDirectMusicSysExTrack (riid, ppobj, pOuter);
}

static HRESULT WINAPI SysExTrackCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	TRACE("(%d)\n", dolock);

	if (dolock)
		DMIME_LockModule();
	else
		DMIME_UnlockModule();
	
	return S_OK;
}

static const IClassFactoryVtbl SysExTrackCF_Vtbl = {
	SysExTrackCF_QueryInterface,
	SysExTrackCF_AddRef,
	SysExTrackCF_Release,
	SysExTrackCF_CreateInstance,
	SysExTrackCF_LockServer
};

static IClassFactoryImpl SysExTrack_CF = {&SysExTrackCF_Vtbl};

/******************************************************************
 *		DirectMusicTimeSigTrack ClassFactory
 */
static HRESULT WINAPI TimeSigTrackCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	FIXME("- no interface\n\tIID:\t%s\n", debugstr_guid(riid));

	if (ppobj == NULL) return E_POINTER;
	
	return E_NOINTERFACE;
}

static ULONG WINAPI TimeSigTrackCF_AddRef(LPCLASSFACTORY iface) {
	DMIME_LockModule();

	return 2; /* non-heap based object */
}

static ULONG WINAPI TimeSigTrackCF_Release(LPCLASSFACTORY iface) {
	DMIME_UnlockModule();

	return 1; /* non-heap based object */
}

static HRESULT WINAPI TimeSigTrackCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj) {
	TRACE ("(%p, %s, %p)\n", pOuter, debugstr_dmguid(riid), ppobj);
	
	return DMUSIC_CreateDirectMusicTimeSigTrack (riid, ppobj, pOuter);
}

static HRESULT WINAPI TimeSigTrackCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	TRACE("(%d)\n", dolock);

	if (dolock)
		DMIME_LockModule();
	else
		DMIME_UnlockModule();
	
	return S_OK;
}

static const IClassFactoryVtbl TimeSigTrackCF_Vtbl = {
	TimeSigTrackCF_QueryInterface,
	TimeSigTrackCF_AddRef,
	TimeSigTrackCF_Release,
	TimeSigTrackCF_CreateInstance,
	TimeSigTrackCF_LockServer
};

static IClassFactoryImpl TimeSigTrack_CF = {&TimeSigTrackCF_Vtbl};

/******************************************************************
 *		DirectMusicParamControlTrack ClassFactory
 */
static HRESULT WINAPI ParamControlTrackCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	FIXME("- no interface\n\tIID:\t%s\n", debugstr_guid(riid));

	if (ppobj == NULL) return E_POINTER;
	
	return E_NOINTERFACE;
}

static ULONG WINAPI ParamControlTrackCF_AddRef(LPCLASSFACTORY iface) {
	DMIME_LockModule();

	return 2; /* non-heap based object */
}

static ULONG WINAPI ParamControlTrackCF_Release(LPCLASSFACTORY iface) {
	DMIME_UnlockModule();

	return 1; /* non-heap based object */
}

static HRESULT WINAPI ParamControlTrackCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj) {
	TRACE ("(%p, %s, %p)\n", pOuter, debugstr_dmguid(riid), ppobj);
	
	return DMUSIC_CreateDirectMusicParamControlTrack (riid, ppobj, pOuter);
}

static HRESULT WINAPI ParamControlTrackCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	TRACE("(%d)\n", dolock);

	if (dolock)
		DMIME_LockModule();
	else
		DMIME_UnlockModule();
	
	return S_OK;
}

static const IClassFactoryVtbl ParamControlTrackCF_Vtbl = {
	ParamControlTrackCF_QueryInterface,
	ParamControlTrackCF_AddRef,
	ParamControlTrackCF_Release,
	ParamControlTrackCF_CreateInstance,
	ParamControlTrackCF_LockServer
};

static IClassFactoryImpl ParamControlTrack_CF = {&ParamControlTrackCF_Vtbl};

/******************************************************************
 *		DirectMusicMarkerTrack ClassFactory
 */
static HRESULT WINAPI MarkerTrackCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	FIXME("- no interface\n\tIID:\t%s\n", debugstr_guid(riid));

	if (ppobj == NULL) return E_POINTER;
	
	return E_NOINTERFACE;
}

static ULONG WINAPI MarkerTrackCF_AddRef(LPCLASSFACTORY iface) {
	DMIME_LockModule();

	return 2; /* non-heap based object */
}

static ULONG WINAPI MarkerTrackCF_Release(LPCLASSFACTORY iface) {
	DMIME_UnlockModule();

	return 1; /* non-heap based object */
}

static HRESULT WINAPI MarkerTrackCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj) {
	TRACE ("(%p, %s, %p)\n", pOuter, debugstr_dmguid(riid), ppobj);

	return DMUSIC_CreateDirectMusicMarkerTrack (riid, ppobj, pOuter);
}

static HRESULT WINAPI MarkerTrackCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	TRACE("(%d)\n", dolock);

	if (dolock)
		DMIME_LockModule();
	else
		DMIME_UnlockModule();
	
	return S_OK;
}

static const IClassFactoryVtbl MarkerTrackCF_Vtbl = {
	MarkerTrackCF_QueryInterface,
	MarkerTrackCF_AddRef,
	MarkerTrackCF_Release,
	MarkerTrackCF_CreateInstance,
	MarkerTrackCF_LockServer
};

static IClassFactoryImpl MarkerTrack_CF = {&MarkerTrackCF_Vtbl};

/******************************************************************
 *		DirectMusicLyricsTrack ClassFactory
 */
static HRESULT WINAPI LyricsTrackCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	FIXME("- no interface\n\tIID:\t%s\n", debugstr_guid(riid));

	if (ppobj == NULL) return E_POINTER;
	
	return E_NOINTERFACE;
}

static ULONG WINAPI LyricsTrackCF_AddRef(LPCLASSFACTORY iface) {
	DMIME_LockModule();

	return 2; /* non-heap based object */
}

static ULONG WINAPI LyricsTrackCF_Release(LPCLASSFACTORY iface) {
	DMIME_UnlockModule();

	return 1; /* non-heap based object */
}

static HRESULT WINAPI LyricsTrackCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj) {
	TRACE ("(%p, %s, %p)\n", pOuter, debugstr_dmguid(riid), ppobj);
	
	return DMUSIC_CreateDirectMusicLyricsTrack (riid, ppobj, pOuter);
}

static HRESULT WINAPI LyricsTrackCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	TRACE("(%d)\n", dolock);

	if (dolock)
		DMIME_LockModule();
	else
		DMIME_UnlockModule();
	
	return S_OK;
}

static const IClassFactoryVtbl LyricsTrackCF_Vtbl = {
	LyricsTrackCF_QueryInterface,
	LyricsTrackCF_AddRef,
	LyricsTrackCF_Release,
	LyricsTrackCF_CreateInstance,
	LyricsTrackCF_LockServer
};

static IClassFactoryImpl LyricsTrack_CF = {&LyricsTrackCF_Vtbl};


/******************************************************************
 *		DirectMusicSegTriggerTrack ClassFactory
 */
static HRESULT WINAPI SegTriggerTrackCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	FIXME("- no interface\n\tIID:\t%s\n", debugstr_guid(riid));

	if (ppobj == NULL) return E_POINTER;
	
	return E_NOINTERFACE;
}

static ULONG WINAPI SegTriggerTrackCF_AddRef(LPCLASSFACTORY iface) {
	DMIME_LockModule();

	return 2; /* non-heap based object */
}

static ULONG WINAPI SegTriggerTrackCF_Release(LPCLASSFACTORY iface) {
	DMIME_UnlockModule();
	
	return 1; /* non-heap based object */
}

static HRESULT WINAPI SegTriggerTrackCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj) {
	TRACE ("(%p, %s, %p)\n", pOuter, debugstr_dmguid(riid), ppobj);
	
	return DMUSIC_CreateDirectMusicSegTriggerTrack (riid, ppobj, pOuter);
}

static HRESULT WINAPI SegTriggerTrackCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	TRACE("(%d)\n", dolock);

	if (dolock)
		DMIME_LockModule();
	else
		DMIME_UnlockModule();
	
	return S_OK;
}

static const IClassFactoryVtbl SegTriggerTrackCF_Vtbl = {
	SegTriggerTrackCF_QueryInterface,
	SegTriggerTrackCF_AddRef,
	SegTriggerTrackCF_Release,
	SegTriggerTrackCF_CreateInstance,
	SegTriggerTrackCF_LockServer
};

static IClassFactoryImpl SegTriggerTrack_CF = {&SegTriggerTrackCF_Vtbl};

/******************************************************************
 *		DirectMusicAudioPath ClassFactory
 */
static HRESULT WINAPI AudioPathCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	FIXME("- no interface\n\tIID:\t%s\n", debugstr_guid(riid));

	if (ppobj == NULL) return E_POINTER;
	
	return E_NOINTERFACE;
}

static ULONG WINAPI AudioPathCF_AddRef(LPCLASSFACTORY iface) {
	DMIME_LockModule();

	return 2; /* non-heap based object */
}

static ULONG WINAPI AudioPathCF_Release(LPCLASSFACTORY iface) {
	DMIME_UnlockModule();

	return 1; /* non-heap based object */
}

static HRESULT WINAPI AudioPathCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj) {
	TRACE ("(%p, %s, %p)\n", pOuter, debugstr_dmguid(riid), ppobj);
	
	return DMUSIC_CreateDirectMusicAudioPathImpl (riid, ppobj, pOuter);
}

static HRESULT WINAPI AudioPathCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	TRACE("(%d)\n", dolock);

	if (dolock)
		DMIME_LockModule();
	else
		DMIME_UnlockModule();
	
	return S_OK;
}

static const IClassFactoryVtbl AudioPathCF_Vtbl = {
	AudioPathCF_QueryInterface,
	AudioPathCF_AddRef,
	AudioPathCF_Release,
	AudioPathCF_CreateInstance,
	AudioPathCF_LockServer
};

static IClassFactoryImpl AudioPath_CF = {&AudioPathCF_Vtbl};

/******************************************************************
 *		DirectMusicWaveTrack ClassFactory
 */
static HRESULT WINAPI WaveTrackCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	FIXME("- no interface\n\tIID:\t%s\n", debugstr_guid(riid));

	if (ppobj == NULL) return E_POINTER;
	
	return E_NOINTERFACE;
}

static ULONG WINAPI WaveTrackCF_AddRef(LPCLASSFACTORY iface) {
	DMIME_LockModule();

	return 2; /* non-heap based object */
}

static ULONG WINAPI WaveTrackCF_Release(LPCLASSFACTORY iface) {
	DMIME_UnlockModule();

	return 1; /* non-heap based object */
}

static HRESULT WINAPI WaveTrackCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj) {
	TRACE ("(%p, %s, %p)\n", pOuter, debugstr_dmguid(riid), ppobj);
	
	return DMUSIC_CreateDirectMusicWaveTrack (riid, ppobj, pOuter);
}

static HRESULT WINAPI WaveTrackCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	TRACE("(%d)\n", dolock);

	if (dolock)
		DMIME_LockModule();
	else
		DMIME_UnlockModule();
	
	return S_OK;
}

static const IClassFactoryVtbl WaveTrackCF_Vtbl = {
	WaveTrackCF_QueryInterface,
	WaveTrackCF_AddRef,
	WaveTrackCF_Release,
	WaveTrackCF_CreateInstance,
	WaveTrackCF_LockServer
};

static IClassFactoryImpl WaveTrack_CF = {&WaveTrackCF_Vtbl};

/******************************************************************
 *		DllMain
 *
 *
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	if (fdwReason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(hinstDLL);
		/* FIXME: Initialisation */
	}
	else if (fdwReason == DLL_PROCESS_DETACH) {
		/* FIXME: Cleanup */
	}

	return TRUE;
}


/******************************************************************
 *		DllCanUnloadNow (DMIME.1)
 *
 *
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
	return DMIME_refCount != 0 ? S_FALSE : S_OK;
}


/******************************************************************
 *		DllGetClassObject (DMIME.@)
 *
 *
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    TRACE("(%s, %s, %p)\n", debugstr_dmguid(rclsid), debugstr_dmguid(riid), ppv);
    if (IsEqualCLSID (rclsid, &CLSID_DirectMusicPerformance) && IsEqualIID (riid, &IID_IClassFactory)) {
		*ppv = (LPVOID) &Performance_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);
		return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicSegment) && IsEqualIID (riid, &IID_IClassFactory)) {
		*ppv = (LPVOID) &Segment_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);
		return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicSegmentState) && IsEqualIID (riid, &IID_IClassFactory)) {
		*ppv = (LPVOID) &SegmentState_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);
		return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicGraph) && IsEqualIID (riid, &IID_IClassFactory)) {
		*ppv = (LPVOID) &Graph_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);
		return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicTempoTrack) && IsEqualIID (riid, &IID_IClassFactory)) {
		*ppv = (LPVOID) &TempoTrack_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);
		return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicSeqTrack) && IsEqualIID (riid, &IID_IClassFactory)) {
		*ppv = (LPVOID) &SeqTrack_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);
		return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicSysExTrack) && IsEqualIID (riid, &IID_IClassFactory)) {
		*ppv = (LPVOID) &SysExTrack_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);
		return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicTimeSigTrack) && IsEqualIID (riid, &IID_IClassFactory)) {
		*ppv = (LPVOID) &TimeSigTrack_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);
		return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicParamControlTrack) && IsEqualIID (riid, &IID_IClassFactory)) {
		*ppv = (LPVOID) &ParamControlTrack_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);
		return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicMarkerTrack) && IsEqualIID (riid, &IID_IClassFactory)) {
		*ppv = (LPVOID) &MarkerTrack_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);
		return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicLyricsTrack) && IsEqualIID (riid, &IID_IClassFactory)) {
		*ppv = (LPVOID) &LyricsTrack_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);
		return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicSegTriggerTrack) && IsEqualIID (riid, &IID_IClassFactory)) {
		*ppv = (LPVOID) &SegTriggerTrack_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);
		return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicAudioPath) && IsEqualIID (riid, &IID_IClassFactory)) {
		*ppv = (LPVOID) &AudioPath_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);
		return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicWaveTrack) && IsEqualIID (riid, &IID_IClassFactory)) {
		*ppv = (LPVOID) &WaveTrack_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);
		return S_OK;
	} 
	
    WARN("(%s, %s, %p): no interface found.\n", debugstr_dmguid(rclsid), debugstr_dmguid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}
