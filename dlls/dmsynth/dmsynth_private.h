/* DirectMusicSynthesizer Private Include
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

#ifndef __WINE_DMSYNTH_PRIVATE_H
#define __WINE_DMSYNTH_PRIVATE_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "wingdi.h"
#include "winuser.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "wine/unicode.h"
#include "winreg.h"

#include "dmusici.h"
#include "dmusicf.h"
#include "dmusics.h"

/*****************************************************************************
 * Interfaces
 */
typedef struct IDirectMusicSynth8Impl IDirectMusicSynth8Impl;
typedef struct IDirectMusicSynthSinkImpl IDirectMusicSynthSinkImpl;

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern ICOM_VTABLE(IDirectMusicSynth8) DirectMusicSynth8_Vtbl;
extern ICOM_VTABLE(IDirectMusicSynthSink) DirectMusicSynthSink_Vtbl;

/*****************************************************************************
 * ClassFactory
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicSynthImpl (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicSynthSinkImpl (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter);

/*****************************************************************************
 * IDirectMusicSynth8Impl implementation structure
 */
struct IDirectMusicSynth8Impl {
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicSynth8);
  DWORD          ref;

  /* IDirectMusicSynth8 fields */
  DMUS_PORTCAPS pCaps;
  BOOL fActive;
  IReferenceClock* pLatencyClock;
  IDirectMusicSynthSinkImpl* pSynthSink;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicSynth8Impl_QueryInterface (LPDIRECTMUSICSYNTH8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicSynth8Impl_AddRef (LPDIRECTMUSICSYNTH8 iface);
extern ULONG WINAPI   IDirectMusicSynth8Impl_Release (LPDIRECTMUSICSYNTH8 iface);
/* IDirectMusicSynth: */
extern HRESULT WINAPI IDirectMusicSynth8Impl_Open (LPDIRECTMUSICSYNTH8 iface, LPDMUS_PORTPARAMS pPortParams);
extern HRESULT WINAPI IDirectMusicSynth8Impl_Close (LPDIRECTMUSICSYNTH8 iface);
extern HRESULT WINAPI IDirectMusicSynth8Impl_SetNumChannelGroups (LPDIRECTMUSICSYNTH8 iface, DWORD dwGroups);
extern HRESULT WINAPI IDirectMusicSynth8Impl_Download (LPDIRECTMUSICSYNTH8 iface, LPHANDLE phDownload, LPVOID pvData, LPBOOL pbFree);
extern HRESULT WINAPI IDirectMusicSynth8Impl_Unload (LPDIRECTMUSICSYNTH8 iface, HANDLE hDownload, HRESULT (CALLBACK* lpFreeHandle)(HANDLE,HANDLE), HANDLE hUserData);
extern HRESULT WINAPI IDirectMusicSynth8Impl_PlayBuffer (LPDIRECTMUSICSYNTH8 iface, REFERENCE_TIME rt, LPBYTE pbBuffer, DWORD cbBuffer);
extern HRESULT WINAPI IDirectMusicSynth8Impl_GetRunningStats (LPDIRECTMUSICSYNTH8 iface, LPDMUS_SYNTHSTATS pStats);
extern HRESULT WINAPI IDirectMusicSynth8Impl_GetPortCaps (LPDIRECTMUSICSYNTH8 iface, LPDMUS_PORTCAPS pCaps);
extern HRESULT WINAPI IDirectMusicSynth8Impl_SetMasterClock (LPDIRECTMUSICSYNTH8 iface, IReferenceClock* pClock);
extern HRESULT WINAPI IDirectMusicSynth8Impl_GetLatencyClock (LPDIRECTMUSICSYNTH8 iface, IReferenceClock** ppClock);
extern HRESULT WINAPI IDirectMusicSynth8Impl_Activate (LPDIRECTMUSICSYNTH8 iface, BOOL fEnable);
extern HRESULT WINAPI IDirectMusicSynth8Impl_SetSynthSink (LPDIRECTMUSICSYNTH8 iface, IDirectMusicSynthSink* pSynthSink);
extern HRESULT WINAPI IDirectMusicSynth8Impl_Render (LPDIRECTMUSICSYNTH8 iface, short* pBuffer, DWORD dwLength, LONGLONG llPosition);
extern HRESULT WINAPI IDirectMusicSynth8Impl_SetChannelPriority (LPDIRECTMUSICSYNTH8 iface, DWORD dwChannelGroup, DWORD dwChannel, DWORD dwPriority);
extern HRESULT WINAPI IDirectMusicSynth8Impl_GetChannelPriority (LPDIRECTMUSICSYNTH8 iface, DWORD dwChannelGroup, DWORD dwChannel, LPDWORD pdwPriority);
extern HRESULT WINAPI IDirectMusicSynth8Impl_GetFormat (LPDIRECTMUSICSYNTH8 iface, LPWAVEFORMATEX pWaveFormatEx, LPDWORD pdwWaveFormatExSiz);
extern HRESULT WINAPI IDirectMusicSynth8Impl_GetAppend (LPDIRECTMUSICSYNTH8 iface, DWORD* pdwAppend);
/* IDirectMusicSynth8: */
extern HRESULT WINAPI IDirectMusicSynth8Impl_PlayVoice (LPDIRECTMUSICSYNTH8 iface, REFERENCE_TIME rt, DWORD dwVoiceId, DWORD dwChannelGroup, DWORD dwChannel, DWORD dwDLId, long prPitch, long vrVolume, SAMPLE_TIME stVoiceStart, SAMPLE_TIME stLoopStart, SAMPLE_TIME stLoopEnd);
extern HRESULT WINAPI IDirectMusicSynth8Impl_StopVoice (LPDIRECTMUSICSYNTH8 iface, REFERENCE_TIME rt, DWORD dwVoiceId);
extern HRESULT WINAPI IDirectMusicSynth8Impl_GetVoiceState (LPDIRECTMUSICSYNTH8 iface, DWORD dwVoice[], DWORD cbVoice, DMUS_VOICE_STATE dwVoiceState[]);
extern HRESULT WINAPI IDirectMusicSynth8Impl_Refresh (LPDIRECTMUSICSYNTH8 iface, DWORD dwDownloadID, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicSynth8Impl_AssignChannelToBuses (LPDIRECTMUSICSYNTH8 iface, DWORD dwChannelGroup, DWORD dwChannel, LPDWORD pdwBuses, DWORD cBuses);

/*****************************************************************************
 * IDirectMusicSynthSinkImpl implementation structure
 */
struct IDirectMusicSynthSinkImpl {
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicSynthSink);
  DWORD          ref;

  /* IDirectMusicSynthSinkImpl fields */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicSynthSinkImpl_QueryInterface (LPDIRECTMUSICSYNTHSINK iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicSynthSinkImpl_AddRef (LPDIRECTMUSICSYNTHSINK iface);
extern ULONG WINAPI   IDirectMusicSynthSinkImpl_Release (LPDIRECTMUSICSYNTHSINK iface);
/* IDirectMusicSynthSinkImpl: */
extern HRESULT WINAPI IDirectMusicSynthSinkImpl_Init (LPDIRECTMUSICSYNTHSINK iface, IDirectMusicSynth* pSynth);
extern HRESULT WINAPI IDirectMusicSynthSinkImpl_SetMasterClock (LPDIRECTMUSICSYNTHSINK iface, IReferenceClock* pClock);
extern HRESULT WINAPI IDirectMusicSynthSinkImpl_GetLatencyClock (LPDIRECTMUSICSYNTHSINK iface, IReferenceClock** ppClock);
extern HRESULT WINAPI IDirectMusicSynthSinkImpl_Activate (LPDIRECTMUSICSYNTHSINK iface, BOOL fEnable);
extern HRESULT WINAPI IDirectMusicSynthSinkImpl_SampleToRefTime (LPDIRECTMUSICSYNTHSINK iface, LONGLONG llSampleTime, REFERENCE_TIME* prfTime);
extern HRESULT WINAPI IDirectMusicSynthSinkImpl_RefTimeToSample (LPDIRECTMUSICSYNTHSINK iface, REFERENCE_TIME rfTime, LONGLONG* pllSampleTime);
extern HRESULT WINAPI IDirectMusicSynthSinkImpl_SetDirectSound (LPDIRECTMUSICSYNTHSINK iface, LPDIRECTSOUND pDirectSound, LPDIRECTSOUNDBUFFER pDirectSoundBuffer);
extern HRESULT WINAPI IDirectMusicSynthSinkImpl_GetDesiredBufferSize (LPDIRECTMUSICSYNTHSINK iface, LPDWORD pdwBufferSizeInSamples);

#endif	/* __WINE_DMSYNTH_PRIVATE_H */
