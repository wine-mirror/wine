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
typedef struct IDirectMusicPatternTrackImpl IDirectMusicPatternTrackImpl;

typedef struct IDirectMusicSegmentObject IDirectMusicSegmentObject;
typedef struct IDirectMusicSegmentObjectStream IDirectMusicSegmentObjectStream;
typedef struct IDirectMusicSongObject IDirectMusicSongObject;
typedef struct IDirectMusicSongObjectStream IDirectMusicSongObjectStream;
typedef struct IDirectMusicAudioPathObject IDirectMusicAudioPathObject;
typedef struct IDirectMusicAudioPathObjectStream IDirectMusicAudioPathObjectStream;
typedef struct IDirectMusicGraphObject IDirectMusicGraphObject;
typedef struct IDirectMusicGraphObjectStream IDirectMusicGraphObjectStream;

typedef struct IDirectMusicLyricsTrack IDirectMusicLyricsTrack;
typedef struct IDirectMusicLyricsTrackStream IDirectMusicLyricsTrackStream;
typedef struct IDirectMusicMarkerTrack IDirectMusicMarkerTrack;
typedef struct IDirectMusicMarkerTrackStream IDirectMusicMarkerTrackStream;
typedef struct IDirectMusicParamControlTrack IDirectMusicParamControlTrack;
typedef struct IDirectMusicParamControlTrackStream IDirectMusicParamControlTrackStream;
typedef struct IDirectMusicSegTriggerTrack IDirectMusicSegTriggerTrack;
typedef struct IDirectMusicSegTriggerTrackStream IDirectMusicSegTriggerTrackStream;
typedef struct IDirectMusicSeqTrack IDirectMusicSeqTrack;
typedef struct IDirectMusicSeqTrackStream IDirectMusicSeqTrackStream;
typedef struct IDirectMusicSysExTrack IDirectMusicSysExTrack;
typedef struct IDirectMusicSysExTrackStream IDirectMusicSysExTrackStream;
typedef struct IDirectMusicTempoTrack IDirectMusicTempoTrack;
typedef struct IDirectMusicTempoTrackStream IDirectMusicTempoTrackStream;
typedef struct IDirectMusicTimeSigTrack IDirectMusicTimeSigTrack;
typedef struct IDirectMusicTimeSigTrackStream IDirectMusicTimeSigTrackStream;
typedef struct IDirectMusicWaveTrack IDirectMusicWaveTrack;
typedef struct IDirectMusicWaveTrackStream IDirectMusicWaveTrackStream;
	
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
extern ICOM_VTABLE(IDirectMusicPatternTrack) DirectMusicPatternTrack_Vtbl;

extern ICOM_VTABLE(IDirectMusicObject) DirectMusicSegmentObject_Vtbl;
extern ICOM_VTABLE(IPersistStream) DirectMusicSegmentObjectStream_Vtbl;
extern ICOM_VTABLE(IDirectMusicObject) DirectMusicSongObject_Vtbl;
extern ICOM_VTABLE(IPersistStream) DirectMusicSongObjectStream_Vtbl;
extern ICOM_VTABLE(IDirectMusicObject) DirectMusicAudioPathObject_Vtbl;
extern ICOM_VTABLE(IPersistStream) DirectMusicAudioPathObjectStream_Vtbl;
extern ICOM_VTABLE(IDirectMusicObject) DirectMusicGraphObject_Vtbl;
extern ICOM_VTABLE(IPersistStream) DirectMusicGraphObjectStream_Vtbl;

extern ICOM_VTABLE(IDirectMusicTrack8) DirectMusicLyricsTrack_Vtbl;
extern ICOM_VTABLE(IPersistStream) DirectMusicLyricsTrackStream_Vtbl;
extern ICOM_VTABLE(IDirectMusicTrack8) DirectMusicMarkerTrack_Vtbl;
extern ICOM_VTABLE(IPersistStream) DirectMusicMarkerTrackStream_Vtbl;
extern ICOM_VTABLE(IDirectMusicTrack8) DirectMusicParamControlTrack_Vtbl;
extern ICOM_VTABLE(IPersistStream) DirectMusicParamControlTrackStream_Vtbl;
extern ICOM_VTABLE(IDirectMusicTrack8) DirectMusicSegTriggerTrack_Vtbl;
extern ICOM_VTABLE(IPersistStream) DirectMusicSegTriggerTrackStream_Vtbl;
extern ICOM_VTABLE(IDirectMusicTrack8) DirectMusicSeqTrack_Vtbl;
extern ICOM_VTABLE(IPersistStream) DirectMusicSeqTrackStream_Vtbl;
extern ICOM_VTABLE(IDirectMusicTrack8) DirectMusicSysExTrack_Vtbl;
extern ICOM_VTABLE(IPersistStream) DirectMusicSysExTrackStream_Vtbl;
extern ICOM_VTABLE(IDirectMusicTrack8) DirectMusicTempoTrack_Vtbl;
extern ICOM_VTABLE(IPersistStream) DirectMusicTempoTrackStream_Vtbl;
extern ICOM_VTABLE(IDirectMusicTrack8) DirectMusicTimeSigTrack_Vtbl;
extern ICOM_VTABLE(IPersistStream) DirectMusicTimeSigTrackStream_Vtbl;
extern ICOM_VTABLE(IDirectMusicTrack8) DirectMusicWaveTrack_Vtbl;
extern ICOM_VTABLE(IPersistStream) DirectMusicWaveTrackStream_Vtbl;

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
/* can support IID_IDirectMusicPatternTrack
 * return always an IDirectMusicPatternTrackImpl
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicPatternTrack (LPCGUID lpcGUID, LPDIRECTMUSICPATTERNTRACK *ppDMPtrnTrack, LPUNKNOWN pUnkOuter);

extern HRESULT WINAPI DMUSIC_CreateDirectMusicSegmentObject (LPCGUID lpcGUID, LPDIRECTMUSICOBJECT *ppObject, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicSongObject (LPCGUID lpcGUID, LPDIRECTMUSICOBJECT *ppObject, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicAudioPathObject (LPCGUID lpcGUID, LPDIRECTMUSICOBJECT *ppObject, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicGraphObject (LPCGUID lpcGUID, LPDIRECTMUSICOBJECT *ppObject, LPUNKNOWN pUnkOuter);

extern HRESULT WINAPI DMUSIC_CreateDirectMusicLyricsTrack (LPCGUID lpcGUID, LPDIRECTMUSICTRACK8* ppTrack, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicMarkerTrack (LPCGUID lpcGUID, LPDIRECTMUSICTRACK8* ppTrack, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicParamControlTrack (LPCGUID lpcGUID, LPDIRECTMUSICTRACK8* ppTrack, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicSegTriggerTrack (LPCGUID lpcGUID, LPDIRECTMUSICTRACK8* ppTrack, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicSeqTrack (LPCGUID lpcGUID, LPDIRECTMUSICTRACK8* ppTrack, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicSysExTrack (LPCGUID lpcGUID, LPDIRECTMUSICTRACK8* ppTrack, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicTempoTrack (LPCGUID lpcGUID, LPDIRECTMUSICTRACK8* ppTrack, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicTimeSigTrack (LPCGUID lpcGUID, LPDIRECTMUSICTRACK8* ppTrack, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicWaveTrack (LPCGUID lpcGUID, LPDIRECTMUSICTRACK8* ppTrack, LPUNKNOWN pUnkOuter);

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
  IDirectMusicSegmentObject* pObject;
	
  DMUS_IO_SEGMENT_HEADER segHeader; /* header */
  GUID guidID; /* unique id */
  DMUS_IO_VERSION vVersion; /* version */
  /* info from UNFO list */
  WCHAR* wszName;
  WCHAR* wszArtist;
  WCHAR* wszCopyright;
  WCHAR* wszSubject;
  WCHAR* wszComment;
  /* tracks */
  DWORD dwTracks;
  LPDIRECTMUSICTRACK8 ppTracks[255];
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
  IDirectMusicGraphObject* pObject;
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
  IDirectMusicSongObject* pObject;
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
  IDirectMusicAudioPathObject* pObject;
	
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
 * IDirectMusicPatternTrackImpl implementation structure
 */
struct IDirectMusicPatternTrackImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicPatternTrack);
  DWORD          ref;

  /* IDirectMusicPatternTrackImpl fields */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicPatternTrackImpl_QueryInterface (LPDIRECTMUSICPATTERNTRACK iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicPatternTrackImpl_AddRef (LPDIRECTMUSICPATTERNTRACK iface);
extern ULONG WINAPI   IDirectMusicPatternTrackImpl_Release (LPDIRECTMUSICPATTERNTRACK iface);
/* IDirectMusicPatternTrack: */
extern HRESULT WINAPI IDirectMusicPatternTrackImpl_CreateSegment (LPDIRECTMUSICPATTERNTRACK iface, IDirectMusicStyle* pStyle, IDirectMusicSegment** ppSegment);
extern HRESULT WINAPI IDirectMusicPatternTrackImpl_SetVariation (LPDIRECTMUSICPATTERNTRACK iface, IDirectMusicSegmentState* pSegState, DWORD dwVariationFlags, DWORD dwPart);
extern HRESULT WINAPI IDirectMusicPatternTrackImpl_SetPatternByName (LPDIRECTMUSICPATTERNTRACK iface, IDirectMusicSegmentState* pSegState, WCHAR* wszName, IDirectMusicStyle* pStyle, DWORD dwPatternType, DWORD* pdwLength);


/*****************************************************************************
 * IDirectMusicSegmentObject implementation structure
 */
struct IDirectMusicSegmentObject
{
  /* IUnknown fields */
  ICOM_VFIELD (IDirectMusicObject);
  DWORD          ref;

  /* IDirectMusicObjectImpl fields */
  LPDMUS_OBJECTDESC pDesc;
  IDirectMusicSegmentObjectStream* pStream;
  IDirectMusicSegment8Impl* pSegment;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicSegmentObject_QueryInterface (LPDIRECTMUSICOBJECT iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicSegmentObject_AddRef (LPDIRECTMUSICOBJECT iface);
extern ULONG WINAPI   IDirectMusicSegmentObject_Release (LPDIRECTMUSICOBJECT iface);
/* IDirectMusicObject: */
extern HRESULT WINAPI IDirectMusicSegmentObject_GetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicSegmentObject_SetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicSegmentObject_ParseDescriptor (LPDIRECTMUSICOBJECT iface, LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc);

/*****************************************************************************
 * IDirectMusicSegmentObjectStream implementation structure
 */
struct IDirectMusicSegmentObjectStream
{
  /* IUnknown fields */
  ICOM_VFIELD (IPersistStream);
  DWORD          ref;

  /* IPersistStreamImpl fields */
  IDirectMusicSegmentObject* pParentObject;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicSegmentObjectStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI   IDirectMusicSegmentObjectStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI   IDirectMusicSegmentObjectStream_Release (LPPERSISTSTREAM iface);
/* IPersist: */
extern HRESULT WINAPI IDirectMusicSegmentObjectStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicSegmentObjectStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicSegmentObjectStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicSegmentObjectStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicSegmentObjectStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);


/*****************************************************************************
 * IDirectMusicSongObject implementation structure
 */
struct IDirectMusicSongObject
{
  /* IUnknown fields */
  ICOM_VFIELD (IDirectMusicObject);
  DWORD          ref;

  /* IDirectMusicObjectImpl fields */
  LPDMUS_OBJECTDESC pDesc;
  IDirectMusicSongObjectStream* pStream;
  IDirectMusicSongImpl* pSong;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicSongObject_QueryInterface (LPDIRECTMUSICOBJECT iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicSongObject_AddRef (LPDIRECTMUSICOBJECT iface);
extern ULONG WINAPI   IDirectMusicSongObject_Release (LPDIRECTMUSICOBJECT iface);
/* IDirectMusicObject: */
extern HRESULT WINAPI IDirectMusicSongObject_GetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicSongObject_SetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicSongObject_ParseDescriptor (LPDIRECTMUSICOBJECT iface, LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc);

/*****************************************************************************
 * IDirectMusicSongObjectStream implementation structure
 */
struct IDirectMusicSongObjectStream
{
  /* IUnknown fields */
  ICOM_VFIELD (IPersistStream);
  DWORD          ref;

  /* IPersistStreamImpl fields */
  IDirectMusicSongObject* pParentObject;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicSongObjectStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI   IDirectMusicSongObjectStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI   IDirectMusicSongObjectStream_Release (LPPERSISTSTREAM iface);
/* IPersist: */
extern HRESULT WINAPI IDirectMusicSongObjectStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicSongObjectStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicSongObjectStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicSongObjectStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicSongObjectStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);

/*****************************************************************************
 * IDirectMusicAudioPathObject implementation structure
 */
struct IDirectMusicAudioPathObject
{
  /* IUnknown fields */
  ICOM_VFIELD (IDirectMusicObject);
  DWORD          ref;

  /* IDirectMusicObjectImpl fields */
  LPDMUS_OBJECTDESC pDesc;
  IDirectMusicAudioPathObjectStream* pStream;
  IDirectMusicAudioPathImpl* pAudioPath;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicAudioPathObject_QueryInterface (LPDIRECTMUSICOBJECT iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicAudioPathObject_AddRef (LPDIRECTMUSICOBJECT iface);
extern ULONG WINAPI   IDirectMusicAudioPathObject_Release (LPDIRECTMUSICOBJECT iface);
/* IDirectMusicObject: */
extern HRESULT WINAPI IDirectMusicAudioPathObject_GetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicAudioPathObject_SetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicAudioPathObject_ParseDescriptor (LPDIRECTMUSICOBJECT iface, LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc);

/*****************************************************************************
 * IDirectMusicAudioPathObjectStream implementation structure
 */
struct IDirectMusicAudioPathObjectStream
{
  /* IUnknown fields */
  ICOM_VFIELD (IPersistStream);
  DWORD          ref;

  /* IPersistStreamImpl fields */
  IDirectMusicAudioPathObject* pParentObject;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicAudioPathObjectStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI   IDirectMusicAudioPathObjectStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI   IDirectMusicAudioPathObjectStream_Release (LPPERSISTSTREAM iface);
/* IPersist: */
extern HRESULT WINAPI IDirectMusicAudioPathObjectStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicAudioPathObjectStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicAudioPathObjectStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicAudioPathObjectStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicAudioPathObjectStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);

/*****************************************************************************
 * IDirectMusicGraphObject implementation structure
 */
struct IDirectMusicGraphObject
{
  /* IUnknown fields */
  ICOM_VFIELD (IDirectMusicObject);
  DWORD          ref;

  /* IDirectMusicObjectImpl fields */
  LPDMUS_OBJECTDESC pDesc;
  IDirectMusicGraphObjectStream* pStream;
  IDirectMusicGraphImpl* pGraph;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicGraphObject_QueryInterface (LPDIRECTMUSICOBJECT iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicGraphObject_AddRef (LPDIRECTMUSICOBJECT iface);
extern ULONG WINAPI   IDirectMusicGraphObject_Release (LPDIRECTMUSICOBJECT iface);
/* IDirectMusicObject: */
extern HRESULT WINAPI IDirectMusicGraphObject_GetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicGraphObject_SetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicGraphObject_ParseDescriptor (LPDIRECTMUSICOBJECT iface, LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc);

/*****************************************************************************
 * IDirectMusicGraphObjectStream implementation structure
 */
struct IDirectMusicGraphObjectStream
{
  /* IUnknown fields */
  ICOM_VFIELD (IPersistStream);
  DWORD          ref;

  /* IPersistStreamImpl fields */
  IDirectMusicGraphObject* pParentObject;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicGraphObjectStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI   IDirectMusicGraphObjectStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI   IDirectMusicGraphObjectStream_Release (LPPERSISTSTREAM iface);
/* IPersist: */
extern HRESULT WINAPI IDirectMusicGraphObjectStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicGraphObjectStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicGraphObjectStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicGraphObjectStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicGraphObjectStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);


/*****************************************************************************
 * IDirectMusicLyricsTrack implementation structure
 */
struct IDirectMusicLyricsTrack
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicTrack8);
  DWORD          ref;

  /* IDirectMusicLyricsTrack fields */
  IDirectMusicLyricsTrackStream* pStream;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicLyricsTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicLyricsTrack_AddRef (LPDIRECTMUSICTRACK8 iface);
extern ULONG WINAPI   IDirectMusicLyricsTrack_Release (LPDIRECTMUSICTRACK8 iface);
/* IDirectMusicTrack: */
extern HRESULT WINAPI IDirectMusicLyricsTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment);
extern HRESULT WINAPI IDirectMusicLyricsTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrackID, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicLyricsTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData);
extern HRESULT WINAPI IDirectMusicLyricsTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicLyricsTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicLyricsTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicLyricsTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType);
extern HRESULT WINAPI IDirectMusicLyricsTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicLyricsTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicLyricsTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack);
/* IDirectMusicTrack8: */
extern HRESULT WINAPI IDirectMusicLyricsTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicLyricsTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicLyricsTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicLyricsTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
extern HRESULT WINAPI IDirectMusicLyricsTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);

/*****************************************************************************
 * IDirectMusicLyricsTrackStream implementation structure
 */
struct IDirectMusicLyricsTrackStream
{
  /* IUnknown fields */
  ICOM_VFIELD (IPersistStream);
  DWORD          ref;

  /* IPersistStreamImpl fields */
  IDirectMusicLyricsTrack* pParentTrack;
};

/* IUnknown: */
extern HRESULT WINAPI  IDirectMusicLyricsTrackStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI IDirectMusicLyricsTrackStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI IDirectMusicLyricsTrackStream_Release (LPPERSISTSTREAM iface);
/* IPersist: */
extern HRESULT WINAPI IDirectMusicLyricsTrackStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicLyricsTrackStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicLyricsTrackStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicLyricsTrackStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicLyricsTrackStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);

/*****************************************************************************
 * IDirectMusicMarkerTrack implementation structure
 */
struct IDirectMusicMarkerTrack
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicTrack8);
  DWORD          ref;

  /* IDirectMusicMarkerTrack fields */
  IDirectMusicMarkerTrackStream* pStream;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicMarkerTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicMarkerTrack_AddRef (LPDIRECTMUSICTRACK8 iface);
extern ULONG WINAPI   IDirectMusicMarkerTrack_Release (LPDIRECTMUSICTRACK8 iface);
/* IDirectMusicTrack: */
extern HRESULT WINAPI IDirectMusicMarkerTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment);
extern HRESULT WINAPI IDirectMusicMarkerTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrackID, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicMarkerTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData);
extern HRESULT WINAPI IDirectMusicMarkerTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicMarkerTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicMarkerTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicMarkerTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType);
extern HRESULT WINAPI IDirectMusicMarkerTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicMarkerTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicMarkerTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack);
/* IDirectMusicTrack8: */
extern HRESULT WINAPI IDirectMusicMarkerTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicMarkerTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicMarkerTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicMarkerTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
extern HRESULT WINAPI IDirectMusicMarkerTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);

/*****************************************************************************
 * IDirectMusicMarkerTrackStream implementation structure
 */
struct IDirectMusicMarkerTrackStream
{
  /* IUnknown fields */
  ICOM_VFIELD (IPersistStream);
  DWORD          ref;

  /* IPersistStreamImpl fields */
  IDirectMusicMarkerTrack* pParentTrack;
};

/* IUnknown: */
extern HRESULT WINAPI  IDirectMusicMarkerTrackStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI IDirectMusicMarkerTrackStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI IDirectMusicMarkerTrackStream_Release (LPPERSISTSTREAM iface);
/* IPersist: */
extern HRESULT WINAPI IDirectMusicMarkerTrackStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicMarkerTrackStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicMarkerTrackStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicMarkerTrackStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicMarkerTrackStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);

/*****************************************************************************
 * IDirectMusicParamControlTrack implementation structure
 */
struct IDirectMusicParamControlTrack
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicTrack8);
  DWORD          ref;

  /* IDirectMusicParamControlTrack fields */
  IDirectMusicParamControlTrackStream* pStream;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicParamControlTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicParamControlTrack_AddRef (LPDIRECTMUSICTRACK8 iface);
extern ULONG WINAPI   IDirectMusicParamControlTrack_Release (LPDIRECTMUSICTRACK8 iface);
/* IDirectMusicTrack: */
extern HRESULT WINAPI IDirectMusicParamControlTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment);
extern HRESULT WINAPI IDirectMusicParamControlTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrackID, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicParamControlTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData);
extern HRESULT WINAPI IDirectMusicParamControlTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicParamControlTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicParamControlTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicParamControlTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType);
extern HRESULT WINAPI IDirectMusicParamControlTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicParamControlTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicParamControlTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack);
/* IDirectMusicTrack8: */
extern HRESULT WINAPI IDirectMusicParamControlTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicParamControlTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicParamControlTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicParamControlTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
extern HRESULT WINAPI IDirectMusicParamControlTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);

/*****************************************************************************
 * IDirectMusicParamControlTrackStream implementation structure
 */
struct IDirectMusicParamControlTrackStream
{
  /* IUnknown fields */
  ICOM_VFIELD (IPersistStream);
  DWORD          ref;

  /* IPersistStreamImpl fields */
  IDirectMusicParamControlTrack* pParentTrack;
};

/* IUnknown: */
extern HRESULT WINAPI  IDirectMusicParamControlTrackStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI IDirectMusicParamControlTrackStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI IDirectMusicParamControlTrackStream_Release (LPPERSISTSTREAM iface);
/* IPersist: */
extern HRESULT WINAPI IDirectMusicParamControlTrackStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicParamControlTrackStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicParamControlTrackStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicParamControlTrackStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicParamControlTrackStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);

/*****************************************************************************
 * IDirectMusicSegTriggerTrack implementation structure
 */
struct IDirectMusicSegTriggerTrack
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicTrack8);
  DWORD          ref;

  /* IDirectMusicSegTriggerTrack fields */
  IDirectMusicSegTriggerTrackStream* pStream;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicSegTriggerTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicSegTriggerTrack_AddRef (LPDIRECTMUSICTRACK8 iface);
extern ULONG WINAPI   IDirectMusicSegTriggerTrack_Release (LPDIRECTMUSICTRACK8 iface);
/* IDirectMusicTrack: */
extern HRESULT WINAPI IDirectMusicSegTriggerTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment);
extern HRESULT WINAPI IDirectMusicSegTriggerTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrackID, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicSegTriggerTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData);
extern HRESULT WINAPI IDirectMusicSegTriggerTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicSegTriggerTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicSegTriggerTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicSegTriggerTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType);
extern HRESULT WINAPI IDirectMusicSegTriggerTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicSegTriggerTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicSegTriggerTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack);
/* IDirectMusicTrack8: */
extern HRESULT WINAPI IDirectMusicSegTriggerTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicSegTriggerTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicSegTriggerTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicSegTriggerTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
extern HRESULT WINAPI IDirectMusicSegTriggerTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);

/*****************************************************************************
 * IDirectMusicSegTriggerTrackStream implementation structure
 */
struct IDirectMusicSegTriggerTrackStream
{
  /* IUnknown fields */
  ICOM_VFIELD (IPersistStream);
  DWORD          ref;

  /* IPersistStreamImpl fields */
  IDirectMusicSegTriggerTrack* pParentTrack;
};

/* IUnknown: */
extern HRESULT WINAPI  IDirectMusicSegTriggerTrackStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI IDirectMusicSegTriggerTrackStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI IDirectMusicSegTriggerTrackStream_Release (LPPERSISTSTREAM iface);
/* IPersist: */
extern HRESULT WINAPI IDirectMusicSegTriggerTrackStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicSegTriggerTrackStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicSegTriggerTrackStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicSegTriggerTrackStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicSegTriggerTrackStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);

/*****************************************************************************
 * IDirectMusicSeqTrack implementation structure
 */
struct IDirectMusicSeqTrack
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicTrack8);
  DWORD          ref;

  /* IDirectMusicSeqTrack fields */
  IDirectMusicSeqTrackStream* pStream;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicSeqTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicSeqTrack_AddRef (LPDIRECTMUSICTRACK8 iface);
extern ULONG WINAPI   IDirectMusicSeqTrack_Release (LPDIRECTMUSICTRACK8 iface);
/* IDirectMusicTrack: */
extern HRESULT WINAPI IDirectMusicSeqTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment);
extern HRESULT WINAPI IDirectMusicSeqTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrackID, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicSeqTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData);
extern HRESULT WINAPI IDirectMusicSeqTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicSeqTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicSeqTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicSeqTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType);
extern HRESULT WINAPI IDirectMusicSeqTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicSeqTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicSeqTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack);
/* IDirectMusicTrack8: */
extern HRESULT WINAPI IDirectMusicSeqTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicSeqTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicSeqTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicSeqTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
extern HRESULT WINAPI IDirectMusicSeqTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);

/*****************************************************************************
 * IDirectMusicSeqTrackStream implementation structure
 */
struct IDirectMusicSeqTrackStream
{
  /* IUnknown fields */
  ICOM_VFIELD (IPersistStream);
  DWORD          ref;

  /* IPersistStreamImpl fields */
  IDirectMusicSeqTrack* pParentTrack;
};

/* IUnknown: */
extern HRESULT WINAPI  IDirectMusicSeqTrackStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI IDirectMusicSeqTrackStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI IDirectMusicSeqTrackStream_Release (LPPERSISTSTREAM iface);
/* IPersist: */
extern HRESULT WINAPI IDirectMusicSeqTrackStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicSeqTrackStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicSeqTrackStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicSeqTrackStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicSeqTrackStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);

/*****************************************************************************
 * IDirectMusicSysExTrack implementation structure
 */
struct IDirectMusicSysExTrack
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicTrack8);
  DWORD          ref;

  /* IDirectMusicSysExTrack fields */
  IDirectMusicSysExTrackStream* pStream;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicSysExTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicSysExTrack_AddRef (LPDIRECTMUSICTRACK8 iface);
extern ULONG WINAPI   IDirectMusicSysExTrack_Release (LPDIRECTMUSICTRACK8 iface);
/* IDirectMusicTrack: */
extern HRESULT WINAPI IDirectMusicSysExTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment);
extern HRESULT WINAPI IDirectMusicSysExTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrackID, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicSysExTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData);
extern HRESULT WINAPI IDirectMusicSysExTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicSysExTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicSysExTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicSysExTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType);
extern HRESULT WINAPI IDirectMusicSysExTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicSysExTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicSysExTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack);
/* IDirectMusicTrack8: */
extern HRESULT WINAPI IDirectMusicSysExTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicSysExTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicSysExTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicSysExTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
extern HRESULT WINAPI IDirectMusicSysExTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);

/*****************************************************************************
 * IDirectMusicSysExTrackStream implementation structure
 */
struct IDirectMusicSysExTrackStream
{
  /* IUnknown fields */
  ICOM_VFIELD (IPersistStream);
  DWORD          ref;

  /* IPersistStreamImpl fields */
  IDirectMusicSysExTrack* pParentTrack;
};

/* IUnknown: */
extern HRESULT WINAPI  IDirectMusicSysExTrackStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI IDirectMusicSysExTrackStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI IDirectMusicSysExTrackStream_Release (LPPERSISTSTREAM iface);
/* IPersist: */
extern HRESULT WINAPI IDirectMusicSysExTrackStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicSysExTrackStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicSysExTrackStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicSysExTrackStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicSysExTrackStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);

/*****************************************************************************
 * IDirectMusicTempoTrack implementation structure
 */
struct IDirectMusicTempoTrack
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicTrack8);
  DWORD          ref;

  /* IDirectMusicTempoTrack fields */
  IDirectMusicTempoTrackStream* pStream;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicTempoTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicTempoTrack_AddRef (LPDIRECTMUSICTRACK8 iface);
extern ULONG WINAPI   IDirectMusicTempoTrack_Release (LPDIRECTMUSICTRACK8 iface);
/* IDirectMusicTrack: */
extern HRESULT WINAPI IDirectMusicTempoTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment);
extern HRESULT WINAPI IDirectMusicTempoTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrackID, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicTempoTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData);
extern HRESULT WINAPI IDirectMusicTempoTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicTempoTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicTempoTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicTempoTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType);
extern HRESULT WINAPI IDirectMusicTempoTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicTempoTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicTempoTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack);
/* IDirectMusicTrack8: */
extern HRESULT WINAPI IDirectMusicTempoTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicTempoTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicTempoTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicTempoTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
extern HRESULT WINAPI IDirectMusicTempoTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);

/*****************************************************************************
 * IDirectMusicTempoTrackStream implementation structure
 */
struct IDirectMusicTempoTrackStream
{
  /* IUnknown fields */
  ICOM_VFIELD (IPersistStream);
  DWORD          ref;

  /* IPersistStreamImpl fields */
  IDirectMusicTempoTrack* pParentTrack;
};

/* IUnknown: */
extern HRESULT WINAPI  IDirectMusicTempoTrackStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI IDirectMusicTempoTrackStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI IDirectMusicTempoTrackStream_Release (LPPERSISTSTREAM iface);
/* IPersist: */
extern HRESULT WINAPI IDirectMusicTempoTrackStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicTempoTrackStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicTempoTrackStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicTempoTrackStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicTempoTrackStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);

/*****************************************************************************
 * IDirectMusicTimeSigTrack implementation structure
 */
struct IDirectMusicTimeSigTrack
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicTrack8);
  DWORD          ref;

  /* IDirectMusicTimeSigTrack fields */
  IDirectMusicTimeSigTrackStream* pStream;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicTimeSigTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicTimeSigTrack_AddRef (LPDIRECTMUSICTRACK8 iface);
extern ULONG WINAPI   IDirectMusicTimeSigTrack_Release (LPDIRECTMUSICTRACK8 iface);
/* IDirectMusicTrack: */
extern HRESULT WINAPI IDirectMusicTimeSigTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment);
extern HRESULT WINAPI IDirectMusicTimeSigTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrackID, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicTimeSigTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData);
extern HRESULT WINAPI IDirectMusicTimeSigTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicTimeSigTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicTimeSigTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicTimeSigTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType);
extern HRESULT WINAPI IDirectMusicTimeSigTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicTimeSigTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicTimeSigTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack);
/* IDirectMusicTrack8: */
extern HRESULT WINAPI IDirectMusicTimeSigTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicTimeSigTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicTimeSigTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicTimeSigTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
extern HRESULT WINAPI IDirectMusicTimeSigTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);

/*****************************************************************************
 * IDirectMusicTimeSigTrackStream implementation structure
 */
struct IDirectMusicTimeSigTrackStream
{
  /* IUnknown fields */
  ICOM_VFIELD (IPersistStream);
  DWORD          ref;

  /* IPersistStreamImpl fields */
  IDirectMusicTimeSigTrack* pParentTrack;
};

/* IUnknown: */
extern HRESULT WINAPI  IDirectMusicTimeSigTrackStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI IDirectMusicTimeSigTrackStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI IDirectMusicTimeSigTrackStream_Release (LPPERSISTSTREAM iface);
/* IPersist: */
extern HRESULT WINAPI IDirectMusicTimeSigTrackStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicTimeSigTrackStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicTimeSigTrackStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicTimeSigTrackStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicTimeSigTrackStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);

/*****************************************************************************
 * IDirectMusicWaveTrack implementation structure
 */
struct IDirectMusicWaveTrack
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicTrack8);
  DWORD          ref;

  /* IDirectMusicWaveTrack fields */
  IDirectMusicWaveTrackStream* pStream;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicWaveTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicWaveTrack_AddRef (LPDIRECTMUSICTRACK8 iface);
extern ULONG WINAPI   IDirectMusicWaveTrack_Release (LPDIRECTMUSICTRACK8 iface);
/* IDirectMusicTrack: */
extern HRESULT WINAPI IDirectMusicWaveTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment);
extern HRESULT WINAPI IDirectMusicWaveTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrackID, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicWaveTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData);
extern HRESULT WINAPI IDirectMusicWaveTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicWaveTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicWaveTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicWaveTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType);
extern HRESULT WINAPI IDirectMusicWaveTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicWaveTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicWaveTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack);
/* IDirectMusicTrack8: */
extern HRESULT WINAPI IDirectMusicWaveTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicWaveTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicWaveTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicWaveTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
extern HRESULT WINAPI IDirectMusicWaveTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);

/*****************************************************************************
 * IDirectMusicWaveTrackStream implementation structure
 */
struct IDirectMusicWaveTrackStream
{
  /* IUnknown fields */
  ICOM_VFIELD (IPersistStream);
  DWORD          ref;

  /* IPersistStreamImpl fields */
  IDirectMusicWaveTrack* pParentTrack;
};

/* IUnknown: */
extern HRESULT WINAPI  IDirectMusicWaveTrackStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI IDirectMusicWaveTrackStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI IDirectMusicWaveTrackStream_Release (LPPERSISTSTREAM iface);
/* IPersist: */
extern HRESULT WINAPI IDirectMusicWaveTrackStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicWaveTrackStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicWaveTrackStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicWaveTrackStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicWaveTrackStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);

static inline const char *debugstr_fourcc( DWORD fourcc )
{
    if (!fourcc) return "'null'";
    return wine_dbg_sprintf( "\'%c%c%c%c\'",
                             (char)(fourcc), (char)(fourcc >> 8),
                             (char)(fourcc >> 16), (char)(fourcc >> 24) );
}

#endif	/* __WINE_DMIME_PRIVATE_H */
