/*
 *  DirectMusic Software Synth Definitions
 *
 *  Copyright (C) 2003 Rok Mandeljc
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __WINE_DMUSIC_SOFTWARESYNTH_H
#define __WINE_DMUSIC_SOFTWARESYNTH_H

#include "dmusicc.h"


/*****************************************************************************
 * Definitions
 */
#define REGSTR_PATH_SOFTWARESYNTHS  "Software\\Microsoft\\DirectMusic\\SoftwareSynths"
#define REFRESH_F_LASTBUFFER        0x00000001


/*****************************************************************************
 * Predeclare the interfaces
 */

DEFINE_GUID(IID_IDirectMusicSynth, 			0x9823661,0x5c85,0x11d2,0xaf,0xa6,0x0,0xaa,0x0,0x24,0xd8,0xb6);
typedef struct IDirectMusicSynth IDirectMusicSynth, *LPDIRECTMUSICSYNTH;
DEFINE_GUID(IID_IDirectMusicSynth8,			0x53cab625,0x2711,0x4c9f,0x9d,0xe7,0x1b,0x7f,0x92,0x5f,0x6f,0xc8);
typedef struct IDirectMusicSynth8 IDirectMusicSynth8, *LPDIRECTMUSICSYNTH8;
DEFINE_GUID(IID_IDirectMusicSynthSink,		0x9823663,0x5c85,0x11d2,0xaf,0xa6,0x0,0xaa, 0x0,0x24,0xd8,0xb6);
typedef struct IDirectMusicSynthSink IDirectMusicSynthSink, *LPDIRECTMUSICSYNTHSINK;
DEFINE_GUID(GUID_DMUS_PROP_SetSynthSink,	0x0a3a5ba5,0x37b6,0x11d2,0xb9,0xf9,0x00,0x00,0xf8,0x75,0xac,0x12);
DEFINE_GUID(GUID_DMUS_PROP_SinkUsesDSound,	0xbe208857,0x8952,0x11d2,0xba,0x1c,0x00,0x00,0xf8,0x75,0xac,0x12);


/*****************************************************************************
 * Structures
 */
#ifndef _DMUS_VOICE_STATE_DEFINED
#define _DMUS_VOICE_STATE_DEFINED

typedef struct _DMUS_VOICE_STATE
{
    BOOL                bExists;
    SAMPLE_POSITION     spPosition;
} DMUS_VOICE_STATE;

#endif /* _DMUS_VOICE_STATE_DEFINED */


/*****************************************************************************
 * IDirectMusicSynth interface
 */
#undef  ICOM_INTERFACE
#define ICOM_INTERFACE IDirectMusicSynth
#define IDirectMusicSynth_METHODS \
    /*** IDirectMusicSynth methods ***/ \
    ICOM_METHOD1(HRESULT, Open, LPDMUS_PORTPARAMS,pPortParams) \
    ICOM_METHOD (HRESULT, Close) \
    ICOM_METHOD1(HRESULT, SetNumChannelGroups, DWORD,dwGroups) \
    ICOM_METHOD3(HRESULT, Download, LPHANDLE,phDownload, LPVOID,pvData, LPBOOL,pbFree) \
    ICOM_METHOD3(HRESULT, Unload, HANDLE,hDownload, HRESULT,(CALLBACK* lpFreeHandle)(HANDLE,HANDLE), HANDLE,hUserData) \
    ICOM_METHOD3(HRESULT, PlayBuffer, REFERENCE_TIME,rt, LPBYTE,pbBuffer, DWORD,cbBuffer) \
    ICOM_METHOD1(HRESULT, GetRunningStats, LPDMUS_SYNTHSTATS,pStats) \
    ICOM_METHOD1(HRESULT, GetPortCaps, LPDMUS_PORTCAPS,pCaps) \
    ICOM_METHOD1(HRESULT, SetMasterClock, IReferenceClock*,pClock) \
	ICOM_METHOD1(HRESULT, GetLatencyClock, IReferenceClock**,ppClock) \
	ICOM_METHOD1(HRESULT, Activate, BOOL,fEnable) \
	ICOM_METHOD1(HRESULT, SetSynthSink, IDirectMusicSynthSink*,pSynthSink) \
	ICOM_METHOD3(HRESULT, Render, short*,pBuffer, DWORD,dwLength, LONGLONG,llPosition) \
	ICOM_METHOD3(HRESULT, SetChannelPriority, DWORD,dwChannelGroup, DWORD,dwChannel, DWORD,dwPriority) \
	ICOM_METHOD3(HRESULT, GetChannelPriority, DWORD,dwChannelGroup, DWORD,dwChannel, LPDWORD,pdwPriority) \
	ICOM_METHOD2(HRESULT, GetFormat, LPWAVEFORMATEX,pWaveFormatEx, LPDWORD,pdwWaveFormatExSiz) \
	ICOM_METHOD1(HRESULT, GetAppend, DWORD*,pdwAppend)

/*** IDirectMusicSynth methods ***/
#define IDirectMusicSynth_IMETHODS \
    IUnknown_IMETHODS \
    IDirectMusicSynth_METHODS
ICOM_DEFINE(IDirectMusicSynth,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IDirectMusicSynth_QueryInterface(p,a,b)					ICOM_CALL2(QueryInterface,p,a,b)
#define IDirectMusicSynth_AddRef(p)								ICOM_CALL (AddRef,p)
#define IDirectMusicSynth_Release(p)							ICOM_CALL (Release,p)
/*** IDirectMusicSynth methods ***/
#define IDirectMusicSynth_Open(p,a)								ICOM_CALL1(Open,p,a)
#define IDirectMusicSynth_Close(p)								ICOM_CALL (Close,p)
#define IDirectMusicSynth_SetNumChannelGroups(p,a)				ICOM_CALL1(SetNumChannelGroups,p,a)
#define IDirectMusicSynth_Download(p,a,b,c)						ICOM_CALL3(Download,p,a,b,c)
#define IDirectMusicSynth_Unload(p,a,b,c)						ICOM_CALL3(Unload,p,a,b,c)
#define IDirectMusicSynth_PlayBuffer(p,a,b,c)					ICOM_CALL3(PlayBuffer,p,a,b,c)
#define IDirectMusicSynth_GetRunningStats(p,a)					ICOM_CALL1(GetRunningStats,p,a)
#define IDirectMusicSynth_GetPortCaps(p,a)						ICOM_CALL1(GetPortCaps,p,a)
#define IDirectMusicSynth_SetMasterClock(p,a)					ICOM_CALL1(SetMasterClock,p,a)
#define IDirectMusicSynth_GetLatencyClock(p,a)					ICOM_CALL1(GetLatencyClock,p,a)
#define IDirectMusicSynth_Activate(p,a)							ICOM_CALL1(Activate,p,a)
#define IDirectMusicSynth_SetSynthSink(p,a)						ICOM_CALL1(SetSynthSink,p,a)
#define IDirectMusicSynth_Render(p,a,b,c)						ICOM_CALL3(Render,p,a,b,c)
#define IDirectMusicSynth_SetChannelPriority(p,a,b,c)			ICOM_CALL3(SetChannelPriority,p,a,b,c)
#define IDirectMusicSynth_GetChannelPriority(p,a,b,c)			ICOM_CALL3(GetChannelPriority,p,a,b,c)
#define IDirectMusicSynth_GetFormat(p,a,b)						ICOM_CALL2(GetFormat,p,a,b)
#define IDirectMusicSynth_GetAppend(p,a)						ICOM_CALL1(GetAppend,p,a)


/*****************************************************************************
 * IDirectMusicSynth8 interface
 */
#undef  ICOM_INTERFACE
#define ICOM_INTERFACE IDirectMusicSynth8
#define IDirectMusicSynth8_METHODS \
    /*** IDirectMusicSynth8 methods ***/ \
    ICOM_METHOD10(HRESULT, PlayVoice, REFERENCE_TIME,rt, DWORD,dwVoiceId, DWORD,dwChannelGroup, DWORD,dwChannel, DWORD,dwDLId, long,prPitch, long,vrVolume, SAMPLE_TIME,stVoiceStart, SAMPLE_TIME,stLoopStart, SAMPLE_TIME,stLoopEnd) \
    ICOM_METHOD2(HRESULT, StopVoice, REFERENCE_TIME,rt, DWORD,dwVoiceId) \
    ICOM_METHOD3(HRESULT, GetVoiceState, DWORD,dwVoice[], DWORD,cbVoice, DMUS_VOICE_STATE,dwVoiceState[]) \
    ICOM_METHOD2(HRESULT, Refresh, DWORD,dwDownloadID, DWORD,dwFlags) \
    ICOM_METHOD4(HRESULT, AssignChannelToBuses, DWORD,dwChannelGroup, DWORD,dwChannel, LPDWORD,pdwBuses, DWORD,cBuses)

/*** IDirectMusicSynth methods ***/
#define IDirectMusicSynth8_IMETHODS \
    IUnknown_IMETHODS \
	IDirectMusicSynth_METHODS \
    IDirectMusicSynth8_METHODS
ICOM_DEFINE(IDirectMusicSynth8,IDirectMusicSynth)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IDirectMusicSynth8_QueryInterface(p,a,b)					ICOM_CALL2(QueryInterface,p,a,b)
#define IDirectMusicSynth8_AddRef(p)								ICOM_CALL (AddRef,p)
#define IDirectMusicSynth8_Release(p)								ICOM_CALL (Release,p)
/*** IDirectMusicSynth methods ***/
#define IDirectMusicSynth8_Open(p,a)								ICOM_CALL1(Open,p,a)
#define IDirectMusicSynth8_Close(p)									ICOM_CALL (Close,p)
#define IDirectMusicSynth8_SetNumChannelGroups(p,a)					ICOM_CALL1(SetNumChannelGroups,p,a)
#define IDirectMusicSynth8_Download(p,a,b,c)						ICOM_CALL3(Download,p,a,b,c)
#define IDirectMusicSynth8_Unload(p,a,b,c)							ICOM_CALL3(Unload,p,a,b,c)
#define IDirectMusicSynth8_PlayBuffer(p,a,b,c)						ICOM_CALL3(PlayBuffer,p,a,b,c)
#define IDirectMusicSynth8_GetRunningStats(p,a)						ICOM_CALL1(GetRunningStats,p,a)
#define IDirectMusicSynth8_GetPortCaps(p,a)							ICOM_CALL1(GetPortCaps,p,a)
#define IDirectMusicSynth8_SetMasterClock(p,a)						ICOM_CALL1(SetMasterClock,p,a)
#define IDirectMusicSynth8_GetLatencyClock(p,a)						ICOM_CALL1(GetLatencyClock,p,a)
#define IDirectMusicSynth8_Activate(p,a)							ICOM_CALL1(Activate,p,a)
#define IDirectMusicSynth8_SetSynthSink(p,a)						ICOM_CALL1(SetSynthSink,p,a)
#define IDirectMusicSynth8_Render(p,a,b,c)							ICOM_CALL3(Render,p,a,b,c)
#define IDirectMusicSynth8_SetChannelPriority(p,a,b,c)				ICOM_CALL3(SetChannelPriority,p,a,b,c)
#define IDirectMusicSynth8_GetChannelPriority(p,a,b,c)				ICOM_CALL3(GetChannelPriority,p,a,b,c)
#define IDirectMusicSynth8_GetFormat(p,a,b)							ICOM_CALL2(GetFormat,p,a,b)
#define IDirectMusicSynth8_GetAppend(p,a)							ICOM_CALL1(GetAppend,p,a)
/*** IDirectMusicSynth8 methods ***/
#define IDirectMusicSynth8_PlayVoice(p,a,b,c,d,e,f,g,h,i,j)			ICOM_CALL10(PlayVoice,p,a,b,c,d,e,f,g,h,i,j)
#define IDirectMusicSynth8_StopVoice(p,a,b)							ICOM_CALL2(StopVoice,p,a,b)
#define IDirectMusicSynth8_GetVoiceState(p,a,b,c)					ICOM_CALL3(GetVoiceState,p,a,b,c)
#define IDirectMusicSynth8_Refresh(p,a,b)							ICOM_CALL2(Refresh,p,a,b)
#define IDirectMusicSynth8_AssignChannelToBuses(p,a,b,c,d)			ICOM_CALL4(AssignChannelToBuses,p,a,b,c,d)


/*****************************************************************************
 * IDirectMusicSynthSink interface
 */
#undef  ICOM_INTERFACE
#define ICOM_INTERFACE IDirectMusicSynthSink
#define IDirectMusicSynthSink_METHODS \
    /*** IDirectMusicSynthSink methods ***/ \
    ICOM_METHOD1(HRESULT, Init, IDirectMusicSynth*,pSynth) \
    ICOM_METHOD1(HRESULT, SetMasterClock, IReferenceClock*,pClock) \
    ICOM_METHOD1(HRESULT, GetLatencyClock, IReferenceClock**,ppClock) \
    ICOM_METHOD1(HRESULT, Activate, BOOL,fEnable) \
    ICOM_METHOD2(HRESULT, SampleToRefTime, LONGLONG,llSampleTime, REFERENCE_TIME*,prfTime) \
    ICOM_METHOD2(HRESULT, RefTimeToSample, REFERENCE_TIME,rfTime, LONGLONG*,pllSampleTime) \
    ICOM_METHOD2(HRESULT, SetDirectSound, LPDIRECTSOUND,pDirectSound, LPDIRECTSOUNDBUFFER,pDirectSoundBuffer) \
    ICOM_METHOD1(HRESULT, GetDesiredBufferSize, LPDWORD,pdwBufferSizeInSamples)

/*** IDirectMusicSynthSink methods ***/
#define IDirectMusicSynthSink_IMETHODS \
    IUnknown_IMETHODS \
    IDirectMusicSynthSink_METHODS
ICOM_DEFINE(IDirectMusicSynthSink,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IDirectMusicSynthSink_QueryInterface(p,a,b)					ICOM_CALL2(QueryInterface,p,a,b)
#define IDirectMusicSynthSink_AddRef(p)								ICOM_CALL (AddRef,p)
#define IDirectMusicSynthSink_Release(p)							ICOM_CALL (Release,p)
/*** IDirectMusicSynth methods ***/
#define IDirectMusicSynthSink_Init(p,a)							ICOM_CALL1(Init,p,a)
#define IDirectMusicSynthSink_SetMasterClock(p,a)				ICOM_CALL1(SetMasterClock,p,a)
#define IDirectMusicSynthSink_GetLatencyClock(p,a)				ICOM_CALL1(GetLatencyClock,p,a)
#define IDirectMusicSynthSink_Activate(p,a)						ICOM_CALL1(Activate,p,a)
#define IDirectMusicSynthSink_SampleToRefTime(p,a,b)			ICOM_CALL2(SampleToRefTime,p,a,b)
#define IDirectMusicSynthSink_RefTimeToSample(p,a,b)			ICOM_CALL2(RefTimeToSample,p,a,b)
#define IDirectMusicSynthSink_SetDirectSound(p,a,b)				ICOM_CALL2(SetDirectSound,p,a,b)
#define IDirectMusicSynthSink_GetDesiredBufferSize(p,a)			ICOM_CALL1(GetDesiredBufferSize,p,a)

#endif /* __WINE_DMUSIC_SOFTWARESYNTH_H */
