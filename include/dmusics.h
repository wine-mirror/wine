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
#undef  INTERFACE
#define INTERFACE IDirectMusicSynth
#define IDirectMusicSynth_METHODS \
    /*** IDirectMusicSynth methods ***/ \
    STDMETHOD(Open)(THIS_ LPDMUS_PORTPARAMS pPortParams) PURE; \
    STDMETHOD(Close)(THIS) PURE; \
    STDMETHOD(SetNumChannelGroups)(THIS_ DWORD dwGroups) PURE; \
    STDMETHOD(Download)(THIS_ LPHANDLE phDownload, LPVOID pvData, LPBOOL pbFree) PURE; \
    STDMETHOD(Unload)(THIS_ HANDLE hDownload, HRESULT (CALLBACK* lpFreeHandle)(HANDLE,HANDLE), HANDLE hUserData) PURE; \
    STDMETHOD(PlayBuffer)(THIS_ REFERENCE_TIME rt, LPBYTE pbBuffer, DWORD cbBuffer) PURE; \
    STDMETHOD(GetRunningStats)(THIS_ LPDMUS_SYNTHSTATS pStats) PURE; \
    STDMETHOD(GetPortCaps)(THIS_ LPDMUS_PORTCAPS pCaps) PURE; \
    STDMETHOD(SetMasterClock)(THIS_ IReferenceClock *pClock) PURE; \
    STDMETHOD(GetLatencyClock)(THIS_ IReferenceClock **ppClock) PURE; \
    STDMETHOD(Activate)(THIS_ BOOL fEnable) PURE; \
    STDMETHOD(SetSynthSink)(THIS_ IDirectMusicSynthSink *pSynthSink) PURE; \
    STDMETHOD(Render)(THIS_ short *pBuffer, DWORD dwLength, LONGLONG llPosition) PURE; \
    STDMETHOD(SetChannelPriority)(THIS_ DWORD dwChannelGroup, DWORD dwChannel, DWORD dwPriority) PURE; \
    STDMETHOD(GetChannelPriority)(THIS_ DWORD dwChannelGroup, DWORD dwChannel, LPDWORD pdwPriority) PURE; \
    STDMETHOD(GetFormat)(THIS_ LPWAVEFORMATEX pWaveFormatEx, LPDWORD pdwWaveFormatExSiz) PURE; \
    STDMETHOD(GetAppend)(THIS_ DWORD *pdwAppend) PURE;

/*** IDirectMusicSynth methods ***/
#define IDirectMusicSynth_IMETHODS \
    IUnknown_IMETHODS \
    IDirectMusicSynth_METHODS
ICOM_DEFINE(IDirectMusicSynth,IUnknown)
#undef INTERFACE

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
#undef  INTERFACE
#define INTERFACE IDirectMusicSynth8
#define IDirectMusicSynth8_METHODS \
    /*** IDirectMusicSynth8 methods ***/ \
    STDMETHOD(PlayVoice)(THIS_ REFERENCE_TIME rt, DWORD dwVoiceId, DWORD dwChannelGroup, DWORD dwChannel, DWORD dwDLId, long prPitch, long vrVolume, SAMPLE_TIME stVoiceStart, SAMPLE_TIME stLoopStart, SAMPLE_TIME stLoopEnd) PURE; \
    STDMETHOD(StopVoice)(THIS_ REFERENCE_TIME rt, DWORD dwVoiceId) PURE; \
    STDMETHOD(GetVoiceState)(THIS_ DWORD dwVoice[], DWORD cbVoice, DMUS_VOICE_STATE dwVoiceState[]) PURE; \
    STDMETHOD(Refresh)(THIS_ DWORD dwDownloadID, DWORD dwFlags) PURE; \
    STDMETHOD(AssignChannelToBuses)(THIS_ DWORD dwChannelGroup, DWORD dwChannel, LPDWORD pdwBuses, DWORD cBuses) PURE;

/*** IDirectMusicSynth methods ***/
#define IDirectMusicSynth8_IMETHODS \
    IUnknown_IMETHODS \
    IDirectMusicSynth_METHODS \
    IDirectMusicSynth8_METHODS
ICOM_DEFINE(IDirectMusicSynth8,IDirectMusicSynth)
#undef INTERFACE

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
#undef  INTERFACE
#define INTERFACE IDirectMusicSynthSink
#define IDirectMusicSynthSink_METHODS \
    /*** IDirectMusicSynthSink methods ***/ \
    STDMETHOD(Init)(THIS_ IDirectMusicSynth *pSynth) PURE; \
    STDMETHOD(SetMasterClock)(THIS_ IReferenceClock *pClock) PURE; \
    STDMETHOD(GetLatencyClock)(THIS_ IReferenceClock **ppClock) PURE; \
    STDMETHOD(Activate)(THIS_ BOOL fEnable) PURE; \
    STDMETHOD(SampleToRefTime)(THIS_ LONGLONG llSampleTime, REFERENCE_TIME *prfTime) PURE; \
    STDMETHOD(RefTimeToSample)(THIS_ REFERENCE_TIME rfTime, LONGLONG *pllSampleTime) PURE; \
    STDMETHOD(SetDirectSound)(THIS_ LPDIRECTSOUND pDirectSound, LPDIRECTSOUNDBUFFER pDirectSoundBuffer) PURE; \
    STDMETHOD(GetDesiredBufferSize)(THIS_ LPDWORD pdwBufferSizeInSamples) PURE;

/*** IDirectMusicSynthSink methods ***/
#define IDirectMusicSynthSink_IMETHODS \
    IUnknown_IMETHODS \
    IDirectMusicSynthSink_METHODS
ICOM_DEFINE(IDirectMusicSynthSink,IUnknown)
#undef INTERFACE

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
