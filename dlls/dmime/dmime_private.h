/* DirectMusicInteractiveEngine Private Include
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

#ifndef __WINE_DMIME_PRIVATE_H
#define __WINE_DMIME_PRIVATE_H

#include "windef.h"
#include "wine/debug.h"
#include "winbase.h"
#include "winnt.h"
#include "dmusicc.h"
#include "dmusici.h"
#include "dmusics.h"
#include "dmplugin.h"
#include "dmusicf.h"
#include "dsound.h"

#include "../dmusic/dmusic_private.h"

/*****************************************************************************
 * Interfaces
 */
typedef struct IDirectMusicPerformance8Impl IDirectMusicPerformance8Impl;
typedef struct IDirectMusicSegment8Impl IDirectMusicSegment8Impl;
typedef struct IDirectMusicSegmentState8Impl IDirectMusicSegmentState8Impl;
typedef struct IDirectMusicGraphImpl IDirectMusicGraphImpl;
typedef struct IDirectMusicSongImpl IDirectMusicSongImpl;
typedef struct IDirectMusicAudioPathImpl IDirectMusicAudioPathImpl;
typedef struct IDirectMusicTool8Impl IDirectMusicTool8Impl;
typedef struct IDirectMusicTrack8Impl IDirectMusicTrack8Impl;
typedef struct IDirectMusicPatternTrackImpl IDirectMusicPatternTrackImpl;

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern ICOM_VTABLE(IDirectMusicPerformance8) DirectMusicPerformance8_Vtbl;
extern ICOM_VTABLE(IDirectMusicSegment8) DirectMusicSegment8_Vtbl;
extern ICOM_VTABLE(IDirectMusicSegmentState8) DirectMusicSegmentState8_Vtbl;
extern ICOM_VTABLE(IDirectMusicGraph) DirectMusicGraph_Vtbl;
extern ICOM_VTABLE(IDirectMusicSong) DirectMusicSong_Vtbl;
extern ICOM_VTABLE(IDirectMusicAudioPath) DirectMusicAudioPath_Vtbl;
extern ICOM_VTABLE(IDirectMusicTool8) DirectMusicTool8_Vtbl;
extern ICOM_VTABLE(IDirectMusicTrack8) DirectMusicTrack8_Vtbl;
extern ICOM_VTABLE(IDirectMusicPatternTrack) DirectMusicPatternTrack_Vtbl;

/*****************************************************************************
 * ClassFactory
 */
/* can support IID_IDirectMusicPerformance and IID_IDirectMusicPerformance8
 * return always an IDirectMusicPerformance8Impl
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicPerformance (LPCGUID lpcGUID, LPDIRECTMUSICPERFORMANCE8 *ppDMPerf, LPUNKNOWN pUnkOuter);
/* can support IID_IDirectMusicSegment and IID_IDirectMusicSegment8
 * return always an IDirectMusicSegment8Impl
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicSegment (LPCGUID lpcGUID, LPDIRECTMUSICSEGMENT8 *ppDMSeg, LPUNKNOWN pUnkOuter);
/* can support IID_IDirectMusicSegmentState and IID_IDirectMusicSegmentState8
 * return always an IDirectMusicSegmentState8Impl
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicSegmentState (LPCGUID lpcGUID, LPDIRECTMUSICSEGMENTSTATE8 *ppDMSeg, LPUNKNOWN pUnkOuter);
/* can support IID_IDirectMusicSegmentgraph
 * return always an IDirectMusicGraphImpl
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicGraph (LPCGUID lpcGUID, LPDIRECTMUSICGRAPH *ppDMGrph, LPUNKNOWN pUnkOuter);
/* can support IID_IDirectMusicSong
 * return always an IDirectMusicSong
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicSong (LPCGUID lpcGUID, LPDIRECTMUSICSONG *ppDMSng, LPUNKNOWN pUnkOuter);
/* can support IID_IDirectMusicAudioPath
 * return always an IDirectMusicAudioPathImpl
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicAudioPath (LPCGUID lpcGUID, LPDIRECTMUSICAUDIOPATH *ppDMApath, LPUNKNOWN pUnkOuter);
/* can support IID_IDirectMusicTool and IID_IDirectMusicTool8
 * return always an IDirectMusicTool8Impl
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicTool (LPCGUID lpcGUID, LPDIRECTMUSICTOOL8 *ppDMTool, LPUNKNOWN pUnkOuter);
/* can support IID_IDirectMusicTrack and IID_IDirectMusicTrack8
 * return always an IDirectMusicTrack8Impl
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicTrack (LPCGUID lpcGUID, LPDIRECTMUSICTRACK8 *ppDMTrack, LPUNKNOWN pUnkOuter);
/* can support IID_IDirectMusicPatternTrack
 * return always an IDirectMusicPatternTrackImpl
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicPatternTrack (LPCGUID lpcGUID, LPDIRECTMUSICPATTERNTRACK *ppDMPtrnTrack, LPUNKNOWN pUnkOuter);

/*****************************************************************************
 * IDirectMusicPerformance8Impl implementation structure
 */
struct IDirectMusicPerformance8Impl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicPerformance8);
  DWORD                  ref;

  /* IDirectMusicPerformanceImpl fields */
  IDirectMusic8*         pDirectMusic;
  IDirectSound*          pDirectSound;
  IDirectMusicGraph*     pToolGraph;
  DMUS_AUDIOPARAMS       pParams;

  /* global parameters */
  BOOL  fAutoDownload;
  char  cMasterGrooveLevel;
  float fMasterTempo;
  long  lMasterVolume;
	
  /* performance channels */
  DMUSIC_PRIVATE_PCHANNEL PChannel[1];

   /* IDirectMusicPerformance8Impl fields */
  IDirectMusicAudioPath* pDefaultPath;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicPerformance8Impl_QueryInterface (LPDIRECTMUSICPERFORMANCE8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicPerformance8Impl_AddRef (LPDIRECTMUSICPERFORMANCE8 iface);
extern ULONG WINAPI   IDirectMusicPerformance8Impl_Release (LPDIRECTMUSICPERFORMANCE8 iface);
/* IDirectMusicPerformance: */
extern HRESULT WINAPI IDirectMusicPerformance8Impl_Init (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusic** ppDirectMusic, LPDIRECTSOUND pDirectSound, HWND hWnd);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_PlaySegment (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusicSegment* pSegment, DWORD dwFlags, __int64 i64StartTime, IDirectMusicSegmentState** ppSegmentState);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_Stop (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusicSegment* pSegment, IDirectMusicSegmentState* pSegmentState, MUSIC_TIME mtTime, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_GetSegmentState (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusicSegmentState** ppSegmentState, MUSIC_TIME mtTime);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_SetPrepareTime (LPDIRECTMUSICPERFORMANCE8 iface, DWORD dwMilliSeconds);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_GetPrepareTime (LPDIRECTMUSICPERFORMANCE8 iface, DWORD* pdwMilliSeconds);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_SetBumperLength (LPDIRECTMUSICPERFORMANCE8 iface, DWORD dwMilliSeconds);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_GetBumperLength (LPDIRECTMUSICPERFORMANCE8 iface, DWORD* pdwMilliSeconds);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_SendPMsg (LPDIRECTMUSICPERFORMANCE8 iface, DMUS_PMSG* pPMSG);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_MusicToReferenceTime (LPDIRECTMUSICPERFORMANCE8 iface, MUSIC_TIME mtTime, REFERENCE_TIME* prtTime);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_ReferenceToMusicTime (LPDIRECTMUSICPERFORMANCE8 iface, REFERENCE_TIME rtTime, MUSIC_TIME* pmtTime);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_IsPlaying (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusicSegment* pSegment, IDirectMusicSegmentState* pSegState);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_GetTime (LPDIRECTMUSICPERFORMANCE8 iface, REFERENCE_TIME* prtNow, MUSIC_TIME* pmtNow);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_AllocPMsg (LPDIRECTMUSICPERFORMANCE8 iface, ULONG cb, DMUS_PMSG** ppPMSG);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_FreePMsg (LPDIRECTMUSICPERFORMANCE8 iface, DMUS_PMSG* pPMSG);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_GetGraph (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusicGraph** ppGraph);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_SetGraph (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusicGraph* pGraph);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_SetNotificationHandle (LPDIRECTMUSICPERFORMANCE8 iface, HANDLE hNotification, REFERENCE_TIME rtMinimum);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_GetNotificationPMsg (LPDIRECTMUSICPERFORMANCE8 iface, DMUS_NOTIFICATION_PMSG** ppNotificationPMsg);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_AddNotificationType (LPDIRECTMUSICPERFORMANCE8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_RemoveNotificationType (LPDIRECTMUSICPERFORMANCE8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_AddPort (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusicPort* pPort);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_RemovePort (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusicPort* pPort);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_AssignPChannelBlock (LPDIRECTMUSICPERFORMANCE8 iface, DWORD dwBlockNum, IDirectMusicPort* pPort, DWORD dwGroup);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_AssignPChannel (LPDIRECTMUSICPERFORMANCE8 iface, DWORD dwPChannel, IDirectMusicPort* pPort, DWORD dwGroup, DWORD dwMChannel);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_PChannelInfo (LPDIRECTMUSICPERFORMANCE8 iface, DWORD dwPChannel, IDirectMusicPort** ppPort, DWORD* pdwGroup, DWORD* pdwMChannel);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_DownloadInstrument (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusicInstrument* pInst, DWORD dwPChannel, IDirectMusicDownloadedInstrument** ppDownInst, DMUS_NOTERANGE* pNoteRanges, DWORD dwNumNoteRanges, IDirectMusicPort** ppPort, DWORD* pdwGroup, DWORD* pdwMChannel);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_Invalidate (LPDIRECTMUSICPERFORMANCE8 iface, MUSIC_TIME mtTime, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_GetParam (LPDIRECTMUSICPERFORMANCE8 iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_SetParam (LPDIRECTMUSICPERFORMANCE8 iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_GetGlobalParam (LPDIRECTMUSICPERFORMANCE8 iface, REFGUID rguidType, void* pParam, DWORD dwSize);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_SetGlobalParam (LPDIRECTMUSICPERFORMANCE8 iface, REFGUID rguidType, void* pParam, DWORD dwSize);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_GetLatencyTime (LPDIRECTMUSICPERFORMANCE8 iface, REFERENCE_TIME* prtTime);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_GetQueueTime (LPDIRECTMUSICPERFORMANCE8 iface, REFERENCE_TIME* prtTime);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_AdjustTime (LPDIRECTMUSICPERFORMANCE8 iface, REFERENCE_TIME rtAmount);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_CloseDown (LPDIRECTMUSICPERFORMANCE8 iface);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_GetResolvedTime (LPDIRECTMUSICPERFORMANCE8 iface, REFERENCE_TIME rtTime, REFERENCE_TIME* prtResolved, DWORD dwTimeResolveFlags);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_MIDIToMusic (LPDIRECTMUSICPERFORMANCE8 iface, BYTE bMIDIValue, DMUS_CHORD_KEY* pChord, BYTE bPlayMode, BYTE bChordLevel, WORD* pwMusicValue);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_MusicToMIDI (LPDIRECTMUSICPERFORMANCE8 iface, WORD wMusicValue, DMUS_CHORD_KEY* pChord, BYTE bPlayMode, BYTE bChordLevel, BYTE* pbMIDIValue);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_TimeToRhythm (LPDIRECTMUSICPERFORMANCE8 iface, MUSIC_TIME mtTime, DMUS_TIMESIGNATURE* pTimeSig, WORD* pwMeasure, BYTE* pbBeat, BYTE* pbGrid, short* pnOffset);
extern HRESULT WINAPI IDirectMusicPerformance8Impl_RhythmToTime (LPDIRECTMUSICPERFORMANCE8 iface, WORD wMeasure, BYTE bBeat, BYTE bGrid, short nOffset, DMUS_TIMESIGNATURE* pTimeSig, MUSIC_TIME* pmtTime);
/* IDirectMusicPerformance8: */
extern HRESULT WINAPI IDirectMusicPerformance8ImplInitAudio (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusic** ppDirectMusic, IDirectSound** ppDirectSound, HWND hWnd, DWORD dwDefaultPathType, DWORD dwPChannelCount, DWORD dwFlags, DMUS_AUDIOPARAMS* pParams);
extern HRESULT WINAPI IDirectMusicPerformance8ImplPlaySegmentEx (LPDIRECTMUSICPERFORMANCE8 iface, IUnknown* pSource, WCHAR* pwzSegmentName, IUnknown* pTransition, DWORD dwFlags, __int64 i64StartTime, IDirectMusicSegmentState** ppSegmentState, IUnknown* pFrom, IUnknown* pAudioPath);
extern HRESULT WINAPI IDirectMusicPerformance8ImplStopEx (LPDIRECTMUSICPERFORMANCE8 iface, IUnknown* pObjectToStop, __int64 i64StopTime, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicPerformance8ImplClonePMsg (LPDIRECTMUSICPERFORMANCE8 iface, DMUS_PMSG* pSourcePMSG, DMUS_PMSG** ppCopyPMSG);
extern HRESULT WINAPI IDirectMusicPerformance8ImplCreateAudioPath (LPDIRECTMUSICPERFORMANCE8 iface, IUnknown* pSourceConfig, BOOL fActivate, IDirectMusicAudioPath** ppNewPath);
extern HRESULT WINAPI IDirectMusicPerformance8ImplCreateStandardAudioPath (LPDIRECTMUSICPERFORMANCE8 iface, DWORD dwType, DWORD dwPChannelCount, BOOL fActivate, IDirectMusicAudioPath** ppNewPath);
extern HRESULT WINAPI IDirectMusicPerformance8ImplSetDefaultAudioPath (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusicAudioPath* pAudioPath);
extern HRESULT WINAPI IDirectMusicPerformance8ImplGetDefaultAudioPath (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusicAudioPath** ppAudioPath);
extern HRESULT WINAPI IDirectMusicPerformance8ImplGetParamEx (LPDIRECTMUSICPERFORMANCE8 iface, REFGUID rguidType, DWORD dwTrackID, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
/* ClassFactory */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicPerformance8 (LPCGUID lpcGUID, LPDIRECTMUSICPERFORMANCE8 *ppDMPerf8, LPUNKNOWN pUnkOuter);

/*****************************************************************************
 * IDirectMusicSegment8Impl implementation structure
 */
struct IDirectMusicSegment8Impl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicSegment8);
  DWORD          ref;

  /* IDirectMusicSegment8Impl fields */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicSegment8Impl_QueryInterface (LPDIRECTMUSICSEGMENT8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicSegment8Impl_AddRef (LPDIRECTMUSICSEGMENT8 iface);
extern ULONG WINAPI   IDirectMusicSegment8Impl_Release (LPDIRECTMUSICSEGMENT8 iface);
/* IDirectMusicSegment: */
extern HRESULT WINAPI IDirectMusicSegment8Impl_GetLength (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME* pmtLength);
extern HRESULT WINAPI IDirectMusicSegment8Impl_SetLength (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME mtLength);
extern HRESULT WINAPI IDirectMusicSegment8Impl_GetRepeats (LPDIRECTMUSICSEGMENT8 iface, DWORD* pdwRepeats);
extern HRESULT WINAPI IDirectMusicSegment8Impl_SetRepeats (LPDIRECTMUSICSEGMENT8 iface, DWORD dwRepeats);
extern HRESULT WINAPI IDirectMusicSegment8Impl_GetDefaultResolution (LPDIRECTMUSICSEGMENT8 iface, DWORD* pdwResolution);
extern HRESULT WINAPI IDirectMusicSegment8Impl_SetDefaultResolution (LPDIRECTMUSICSEGMENT8 iface, DWORD dwResolution);
extern HRESULT WINAPI IDirectMusicSegment8Impl_GetTrack (LPDIRECTMUSICSEGMENT8 iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, IDirectMusicTrack** ppTrack);
extern HRESULT WINAPI IDirectMusicSegment8Impl_GetTrackGroup (LPDIRECTMUSICSEGMENT8 iface, IDirectMusicTrack* pTrack, DWORD* pdwGroupBits);
extern HRESULT WINAPI IDirectMusicSegment8Impl_InsertTrack (LPDIRECTMUSICSEGMENT8 iface, IDirectMusicTrack* pTrack, DWORD dwGroupBits);
extern HRESULT WINAPI IDirectMusicSegment8Impl_RemoveTrack (LPDIRECTMUSICSEGMENT8 iface, IDirectMusicTrack* pTrack);
extern HRESULT WINAPI IDirectMusicSegment8Impl_InitPlay (LPDIRECTMUSICSEGMENT8 iface, IDirectMusicSegmentState** ppSegState, IDirectMusicPerformance* pPerformance, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicSegment8Impl_GetGraph (LPDIRECTMUSICSEGMENT8 iface, IDirectMusicGraph** ppGraph);
extern HRESULT WINAPI IDirectMusicSegment8Impl_SetGraph (LPDIRECTMUSICSEGMENT8 iface, IDirectMusicGraph* pGraph);
extern HRESULT WINAPI IDirectMusicSegment8Impl_AddNotificationType (LPDIRECTMUSICSEGMENT8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicSegment8Impl_RemoveNotificationType (LPDIRECTMUSICSEGMENT8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicSegment8Impl_GetParam (LPDIRECTMUSICSEGMENT8 iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicSegment8Impl_SetParam (LPDIRECTMUSICSEGMENT8 iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicSegment8Impl_Clone (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicSegment** ppSegment);
extern HRESULT WINAPI IDirectMusicSegment8Impl_SetStartPoint (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME mtStart);
extern HRESULT WINAPI IDirectMusicSegment8Impl_GetStartPoint (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME* pmtStart);
extern HRESULT WINAPI IDirectMusicSegment8Impl_SetLoopPoints (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd);
extern HRESULT WINAPI IDirectMusicSegment8Impl_GetLoopPoints (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME* pmtStart, MUSIC_TIME* pmtEnd);
extern HRESULT WINAPI IDirectMusicSegment8Impl_SetPChannelsUsed (LPDIRECTMUSICSEGMENT8 iface, DWORD dwNumPChannels, DWORD* paPChannels);
/* IDirectMusicSegment8: */
extern HRESULT WINAPI IDirectMusicSegment8Impl_SetTrackConfig (LPDIRECTMUSICSEGMENT8 iface, REFGUID rguidTrackClassID, DWORD dwGroupBits, DWORD dwIndex, DWORD dwFlagsOn, DWORD dwFlagsOff);
extern HRESULT WINAPI IDirectMusicSegment8Impl_GetAudioPathConfig (LPDIRECTMUSICSEGMENT8 iface, IUnknown** ppAudioPathConfig);
extern HRESULT WINAPI IDirectMusicSegment8Impl_Compose (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME mtTime, IDirectMusicSegment* pFromSegment, IDirectMusicSegment* pToSegment, IDirectMusicSegment** ppComposedSegment);
extern HRESULT WINAPI IDirectMusicSegment8Impl_Download (LPDIRECTMUSICSEGMENT8 iface, IUnknown *pAudioPath);
extern HRESULT WINAPI IDirectMusicSegment8Impl_Unload (LPDIRECTMUSICSEGMENT8 iface, IUnknown *pAudioPath);
	
/*****************************************************************************
 * IDirectMusicSegmentState8Impl implementation structure
 */
struct IDirectMusicSegmentState8Impl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicSegmentState8);
  DWORD          ref;

  /* IDirectMusicSegmentState8Impl fields */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicSegmentState8Impl_QueryInterface (LPDIRECTMUSICSEGMENTSTATE8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicSegmentState8Impl_AddRef (LPDIRECTMUSICSEGMENTSTATE8 iface);
extern ULONG WINAPI   IDirectMusicSegmentState8Impl_Release (LPDIRECTMUSICSEGMENTSTATE8 iface);
/* IDirectMusicSegmentState: */
extern HRESULT WINAPI IDirectMusicSegmentState8Impl_GetRepeats (LPDIRECTMUSICSEGMENTSTATE8 iface,  DWORD* pdwRepeats);
extern HRESULT WINAPI IDirectMusicSegmentState8Impl_GetSegment (LPDIRECTMUSICSEGMENTSTATE8 iface, IDirectMusicSegment** ppSegment);
extern HRESULT WINAPI IDirectMusicSegmentState8Impl_GetStartTime (LPDIRECTMUSICSEGMENTSTATE8 iface, MUSIC_TIME* pmtStart);
extern HRESULT WINAPI IDirectMusicSegmentState8Impl_GetSeek (LPDIRECTMUSICSEGMENTSTATE8 iface, MUSIC_TIME* pmtSeek);
extern HRESULT WINAPI IDirectMusicSegmentState8Impl_GetStartPoint (LPDIRECTMUSICSEGMENTSTATE8 iface, MUSIC_TIME* pmtStart);
/* IDirectMusicSegmentState8: */
extern HRESULT WINAPI IDirectMusicSegmentState8Impl_SetTrackConfig (LPDIRECTMUSICSEGMENTSTATE8 iface, REFGUID rguidTrackClassID, DWORD dwGroupBits, DWORD dwIndex, DWORD dwFlagsOn, DWORD dwFlagsOff);
extern HRESULT WINAPI IDirectMusicSegmentState8Impl_GetObjectInPath (LPDIRECTMUSICSEGMENTSTATE8 iface, DWORD dwPChannel, DWORD dwStage, DWORD dwBuffer, REFGUID guidObject, DWORD dwIndex, REFGUID iidInterface, void** ppObject);

/*****************************************************************************
 * IDirectMusicGraphImpl implementation structure
 */
struct IDirectMusicGraphImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicGraph);
  DWORD          ref;

  /* IDirectMusicGraphImpl fields */
  IDirectMusicTool8Impl* pFirst;
  IDirectMusicTool8Impl* pLast;
  WORD                  num_tools;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicGraphImpl_QueryInterface (LPDIRECTMUSICGRAPH iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicGraphImpl_AddRef (LPDIRECTMUSICGRAPH iface);
extern ULONG WINAPI   IDirectMusicGraphImpl_Release (LPDIRECTMUSICGRAPH iface);
/* IDirectMusicGraph: */
extern HRESULT WINAPI IDirectMusicGraphImpl_StampPMsg (LPDIRECTMUSICGRAPH iface, DMUS_PMSG* pPMSG);
extern HRESULT WINAPI IDirectMusicGraphImpl_InsertTool (LPDIRECTMUSICGRAPH iface, IDirectMusicTool* pTool, DWORD* pdwPChannels, DWORD cPChannels, LONG lIndex);
extern HRESULT WINAPI IDirectMusicGraphImpl_GetTool (LPDIRECTMUSICGRAPH iface, DWORD dwIndex, IDirectMusicTool** ppTool);
extern HRESULT WINAPI IDirectMusicGraphImpl_RemoveTool (LPDIRECTMUSICGRAPH iface, IDirectMusicTool* pTool);

/*****************************************************************************
 * IDirectMusicSongImpl implementation structure
 */
struct IDirectMusicSongImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicSong);
  DWORD          ref;

  /* IDirectMusicSongImpl fields */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicSongImpl_QueryInterface (LPDIRECTMUSICSONG iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicSongImpl_AddRef (LPDIRECTMUSICSONG iface);
extern ULONG WINAPI   IDirectMusicSongImpll_Release (LPDIRECTMUSICSONG iface);
/* IDirectMusicContainer: */
extern HRESULT WINAPI IDirectMusicSongImpl_Compose (LPDIRECTMUSICSONG iface);
extern HRESULT WINAPI IDirectMusicSongImpl_GetParam (LPDIRECTMUSICSONG iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicSongImpl_GetSegment (LPDIRECTMUSICSONG iface, WCHAR* pwzName, IDirectMusicSegment** ppSegment);
extern HRESULT WINAPI IDirectMusicSongImpl_GetAudioPathConfig (LPDIRECTMUSICSONG iface, IUnknown** ppAudioPathConfig);
extern HRESULT WINAPI IDirectMusicSongImpl_Download (LPDIRECTMUSICSONG iface, IUnknown* pAudioPath);
extern HRESULT WINAPI IDirectMusicSongImpl_Unload (LPDIRECTMUSICSONG iface, IUnknown* pAudioPath);
extern HRESULT WINAPI IDirectMusicSongImpl_EnumSegment (LPDIRECTMUSICSONG iface, DWORD dwIndex, IDirectMusicSegment** ppSegment);

/*****************************************************************************
 * IDirectMusicAudioPathImpl implementation structure
 */
struct IDirectMusicAudioPathImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicAudioPath);
  DWORD          ref;

  /* IDirectMusicAudioPathImpl fields */
  IDirectMusicPerformance8* pPerf;
  IDirectMusicGraph*        pToolGraph;
  IDirectSoundBuffer*       pDSBuffer;
  IDirectSoundBuffer*       pPrimary;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicAudioPathImpl_QueryInterface (LPDIRECTMUSICAUDIOPATH iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicAudioPathImpl_AddRef (LPDIRECTMUSICAUDIOPATH iface);
extern ULONG WINAPI   IDirectMusicAudioPathImpl_Release (LPDIRECTMUSICAUDIOPATH iface);
/* IDirectMusicAudioPath: */
extern HRESULT WINAPI IDirectMusicAudioPathImpl_GetObjectInPath (LPDIRECTMUSICAUDIOPATH iface, DWORD dwPChannel, DWORD dwStage, DWORD dwBuffer, REFGUID guidObject, WORD dwIndex, REFGUID iidInterface, void** ppObject);
extern HRESULT WINAPI IDirectMusicAudioPathImpl_Activate (LPDIRECTMUSICAUDIOPATH iface, BOOL fActivate);
extern HRESULT WINAPI IDirectMusicAudioPathImpl_SetVolume (LPDIRECTMUSICAUDIOPATH iface, long lVolume, DWORD dwDuration);
extern HRESULT WINAPI IDirectMusicAudioPathImpl_ConvertPChannel (LPDIRECTMUSICAUDIOPATH iface, DWORD dwPChannelIn, DWORD* pdwPChannelOut);

/*****************************************************************************
 * IDirectMusicTool8Impl implementation structure
 */
struct IDirectMusicTool8Impl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicTool8);
  DWORD          ref;

  /* IDirectMusicTool8Impl fields */
  IDirectMusicTool8Impl* pPrev;
  IDirectMusicTool8Impl* pNext;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicTool8Impl_QueryInterface (LPDIRECTMUSICTOOL8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicTool8Impl_AddRef (LPDIRECTMUSICTOOL8 iface);
extern ULONG WINAPI   IDirectMusicTool8Impl_Release (LPDIRECTMUSICTOOL8 iface);
/* IDirectMusicTool8Impl: */
extern HRESULT WINAPI IDirectMusicTool8Impl_Init (LPDIRECTMUSICTOOL8 iface, IDirectMusicGraph* pGraph);
extern HRESULT WINAPI IDirectMusicTool8Impl_GetMsgDeliveryType (LPDIRECTMUSICTOOL8 iface, DWORD* pdwDeliveryType);
extern HRESULT WINAPI IDirectMusicTool8Impl_GetMediaTypeArraySize (LPDIRECTMUSICTOOL8 iface, DWORD* pdwNumElements);
extern HRESULT WINAPI IDirectMusicTool8Impl_GetMediaTypes (LPDIRECTMUSICTOOL8 iface, DWORD** padwMediaTypes, DWORD dwNumElements);
extern HRESULT WINAPI IDirectMusicTool8Impl_ProcessPMsg (LPDIRECTMUSICTOOL8 iface, IDirectMusicPerformance* pPerf, DMUS_PMSG* pPMSG);
extern HRESULT WINAPI IDirectMusicTool8Impl_Flush (LPDIRECTMUSICTOOL8 iface, IDirectMusicPerformance* pPerf, DMUS_PMSG* pPMSG, REFERENCE_TIME rtTime);
/* IDirectMusicToolImpl8: */
extern HRESULT WINAPI IDirectMusicTool8Impl_Clone (LPDIRECTMUSICTOOL8 iface, IDirectMusicTool** ppTool);

/*****************************************************************************
 * IDirectMusicTrack8Impl implementation structure
 */
struct IDirectMusicTrack8Impl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicTrack8);
  DWORD          ref;

  /* IDirectMusicTrack8Impl fields */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicTrack8Impl_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicTrack8Impl_AddRef (LPDIRECTMUSICTRACK8 iface);
extern ULONG WINAPI   IDirectMusicTrack8Impl_Release (LPDIRECTMUSICTRACK8 iface);
/* IDirectMusicTrack: */
extern HRESULT WINAPI IDirectMusicTrack8Impl_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment);
extern HRESULT WINAPI IDirectMusicTrack8Impl_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrackID, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicTrack8Impl_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData);
extern HRESULT WINAPI IDirectMusicTrack8Impl_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicTrack8Impl_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicTrack8Impl_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicTrack8Impl_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType);
extern HRESULT WINAPI IDirectMusicTrack8Impl_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicTrack8Impl_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicTrack8Impl_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack);
/* IDirectMusicTrack8: */
extern HRESULT WINAPI IDirectMusicTrack8Impl_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicTrack8Impl_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicTrack8Impl_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicTrack8Impl_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
extern HRESULT WINAPI IDirectMusicTrack8Impl_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);

/*****************************************************************************
 * IDirectMusicPatternTrackImpl implementation structure
 */
struct IDirectMusicPatternTrackImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicPatternTrack);
  DWORD          ref;

  /* IDirectMusicComposerImpl fields */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicPatternTrackImpl_QueryInterface (LPDIRECTMUSICPATTERNTRACK iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicPatternTrackImpl_AddRef (LPDIRECTMUSICPATTERNTRACK iface);
extern ULONG WINAPI   IDirectMusicPatternTrackImpl_Release (LPDIRECTMUSICPATTERNTRACK iface);
/* IDirectMusicPatternTrack: */
extern HRESULT WINAPI IDirectMusicPatternTrackImpl_CreateSegment (LPDIRECTMUSICPATTERNTRACK iface, IDirectMusicStyle* pStyle, IDirectMusicSegment** ppSegment);
extern HRESULT WINAPI IDirectMusicPatternTrackImpl_SetVariation (LPDIRECTMUSICPATTERNTRACK iface, IDirectMusicSegmentState* pSegState, DWORD dwVariationFlags, DWORD dwPart);
extern HRESULT WINAPI IDirectMusicPatternTrackImpl_SetPatternByName (LPDIRECTMUSICPATTERNTRACK iface, IDirectMusicSegmentState* pSegState, WCHAR* wszName, IDirectMusicStyle* pStyle, DWORD dwPatternType, DWORD* pdwLength);

#endif	/* __WINE_DMIME_PRIVATE_H */
