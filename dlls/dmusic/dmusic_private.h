/* DirectMusic Private Include
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

#ifndef __WINE_DMUSIC_PRIVATE_H
#define __WINE_DMUSIC_PRIVATE_H

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

/*****************************************************************************
 * Interfaces
 */
typedef struct IDirectMusicImpl IDirectMusicImpl;
typedef struct IDirectMusic8Impl IDirectMusic8Impl;
typedef struct IDirectMusicBufferImpl IDirectMusicBufferImpl;
typedef struct IDirectMusicInstrumentImpl IDirectMusicInstrumentImpl;
typedef struct IDirectMusicDownloadedInstrumentImpl IDirectMusicDownloadedInstrumentImpl;
typedef struct IDirectMusicCollectionImpl IDirectMusicCollectionImpl;
typedef struct IDirectMusicDownloadImpl IDirectMusicDownloadImpl;
typedef struct IDirectMusicPortDownloadImpl IDirectMusicPortDownloadImpl;
typedef struct IDirectMusicPortImpl IDirectMusicPortImpl;
typedef struct IDirectMusicThruImpl IDirectMusicThruImpl;
typedef struct IReferenceClockImpl IReferenceClockImpl;

typedef struct IDirectMusicSynthImpl IDirectMusicSynthImpl;
typedef struct IDirectMusicSynth8Impl IDirectMusicSynth8Impl;
typedef struct IDirectMusicSynthSinkImpl IDirectMusicSynthSinkImpl;

typedef struct IDirectMusicToolImpl IDirectMusicToolImpl;
typedef struct IDirectMusicTool8Impl IDirectMusicTool8Impl;
typedef struct IDirectMusicTrackImpl IDirectMusicTrackImpl;
typedef struct IDirectMusicTrack8Impl IDirectMusicTrack8Impl;

typedef struct IDirectMusicBandImpl IDirectMusicBandImpl;
typedef struct IDirectMusicObjectImpl IDirectMusicObjectImpl;
typedef struct IDirectMusicLoaderImpl IDirectMusicLoaderImpl;
typedef struct IDirectMusicLoader8Impl IDirectMusicLoader8Impl;
typedef struct IDirectMusicGetLoaderImpl IDirectMusicGetLoaderImpl;
typedef struct IDirectMusicSegmentImpl IDirectMusicSegmentImpl;
typedef struct IDirectMusicSegment8Impl IDirectMusicSegment8Impl;
typedef struct IDirectMusicSegmentStateImpl IDirectMusicSegmentStateImpl;
typedef struct IDirectMusicSegmentState8Impl IDirectMusicSegmentState8Impl;
typedef struct IDirectMusicAudioPathImpl IDirectMusicAudioPathImpl;
typedef struct IDirectMusicPerformanceImpl IDirectMusicPerformanceImpl;
typedef struct IDirectMusicPerformance8Impl IDirectMusicPerformance8Impl;
typedef struct IDirectMusicGraphImpl IDirectMusicGraphImpl;
typedef struct IDirectMusicStyleImpl IDirectMusicStyleImpl;
typedef struct IDirectMusicStyle8Impl IDirectMusicStyle8Impl;
typedef struct IDirectMusicChordMapImpl IDirectMusicChordMapImpl;
typedef struct IDirectMusicComposerImpl IDirectMusicComposerImpl;
typedef struct IDirectMusicPatternTrackImpl IDirectMusicPatternTrackImpl;
typedef struct IDirectMusicScriptImpl IDirectMusicScriptImpl;
typedef struct IDirectMusicContainerImpl IDirectMusicContainerImpl;
typedef struct IDirectMusicSongImpl IDirectMusicSongImpl;

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern ICOM_VTABLE(IDirectMusic) DirectMusic_Vtbl;
extern ICOM_VTABLE(IDirectMusic8) DirectMusic8_Vtbl;
extern ICOM_VTABLE(IDirectMusicBuffer) DirectMusicBuffer_Vtbl;
extern ICOM_VTABLE(IDirectMusicInstrument) DirectMusicInstrument_Vtbl;
extern ICOM_VTABLE(IDirectMusicDownloadedInstrument) DirectMusicDownloadedInstrument_Vtbl;
extern ICOM_VTABLE(IDirectMusicCollection) DirectMusicCollection_Vtbl;
extern ICOM_VTABLE(IDirectMusicDownload) DirectMusicDownload_Vtbl;
extern ICOM_VTABLE(IDirectMusicPortDownload) DirectMusicPortDownload_Vtbl;
extern ICOM_VTABLE(IDirectMusicPort) DirectMusicPort_Vtbl;
extern ICOM_VTABLE(IDirectMusicThru) DirectMusicThru_Vtbl;
extern ICOM_VTABLE(IReferenceClock) ReferenceClock_Vtbl;

extern ICOM_VTABLE(IDirectMusicSynth) DirectMusicSynth;
extern ICOM_VTABLE(IDirectMusicSynth8) DirectMusicSynth8;
extern ICOM_VTABLE(IDirectMusicSynthSink) DirectMusicSynthSink;

extern ICOM_VTABLE(IDirectMusicTool) DirectMusicTool;
extern ICOM_VTABLE(IDirectMusicTool8) DirectMusicTool8;
extern ICOM_VTABLE(IDirectMusicTrack) DirectMusicTrack;
extern ICOM_VTABLE(IDirectMusicTrack8) DirectMusicTrack8;

extern ICOM_VTABLE(IDirectMusicBand) DirectMusicBand;
extern ICOM_VTABLE(IDirectMusicObject) DirectMusicObject;
extern ICOM_VTABLE(IDirectMusicLoader) DirectMusicLoader;
extern ICOM_VTABLE(IDirectMusicLoader8) DirectMusicLoader8;
extern ICOM_VTABLE(IDirectMusicGetLoader) DirectMusicGetLoader;
extern ICOM_VTABLE(IDirectMusicSegment) DirectMusicSegment;
extern ICOM_VTABLE(IDirectMusicSegment8) DirectMusicSegment8;
extern ICOM_VTABLE(IDirectMusicSegmentState) DirectMusicSegmentState;
extern ICOM_VTABLE(IDirectMusicSegmentState8) DirectMusicSegmentState8;
extern ICOM_VTABLE(IDirectMusicAudioPath) DirectMusicAudioPath_Vtbl;
extern ICOM_VTABLE(IDirectMusicPerformance) DirectMusicPerformance_Vtbl;
extern ICOM_VTABLE(IDirectMusicPerformance8) DirectMusicPerformance8_Vtbl;
extern ICOM_VTABLE(IDirectMusicGraph) DirectMusicGraph_Vtbl;
extern ICOM_VTABLE(IDirectMusicStyle) DirectMusicStyle_Vtbl;
extern ICOM_VTABLE(IDirectMusicStyle8) DirectMusicStyle8_Vtbl;
extern ICOM_VTABLE(IDirectMusicChordMap) DirectMusicChordMap_Vtbl;
extern ICOM_VTABLE(IDirectMusicComposer) DirectMusicComposer_Vtbl;
extern ICOM_VTABLE(IDirectMusicPatternTrack) DirectMusicPatternTrack_Vtbl;
extern ICOM_VTABLE(IDirectMusicScript) DirectMusicScript_Vtbl;
extern ICOM_VTABLE(IDirectMusicContainer) DirectMusicContainer_Vtbl;
extern ICOM_VTABLE(IDirectMusicSong) DirectMusicSong_Vtbl;


/*****************************************************************************
 * Some stuff to make my life easier :=)
 */
 
/* some sort of aux. performance channel: as far as i can understand, these are 
   used to represent a particular midi channel in particular group at particular
   group; so all we need to do is to fill it with parent port, group and midi 
   channel ? */
typedef struct DMUSIC_PRIVATE_PCHANNEL_
{
	DWORD channel; /* map to this channel... */
	DWORD group; /* ... in this group ... */
	IDirectMusicPort *port; /* ... at this port */
} DMUSIC_PRIVATE_PCHANNEL, *LPDMUSIC_PRIVATE_PCHANNEL;

/* some sort of aux. midi channel: big fake at the moment; accepts only priority
   changes... more coming soon */
typedef struct DMUSIC_PRIVATE_MCHANNEL_
{
	DWORD priority;
} DMUSIC_PRIVATE_MCHANNEL, *LPDMUSIC_PRIVATE_MCHANNEL;

/* some sort of aux. channel group: collection of 16 midi channels */
typedef struct DMUSIC_PRIVATE_CHANNEL_GROUP_
{
	DMUSIC_PRIVATE_MCHANNEL channel[16]; /* 16 channels in a group */
} DMUSIC_PRIVATE_CHANNEL_GROUP, *LPDMUSIC_PRIVATE_CHANNEL_GROUP;

/* used for loading chunks of data from files */
typedef struct _rawChunk
{
	FOURCC id; /* FOURCC */
	DWORD size; /* size of chunk_riff */
	/* BYTE* data; */ /* chunk_riff data */
} rawChunk;

/* struct in which UNFO data is stored */
typedef struct _UNFO_List
{
	WCHAR* name;
	WCHAR* artist;
	WCHAR* copyright;
	WCHAR* version;
	WCHAR* subject;
	WCHAR* comment;
} UNFO_List;

typedef struct _ChordData
{
	DMUS_IO_CHORD chord;
	DWORD nrofsubchords;
	DMUS_IO_SUBCHORD *subchord;	
} ChordData;

typedef struct _Reference
{
	DMUS_IO_REFERENCE header;
	GUID guid;
	FILETIME date;
	WCHAR* name;
	WCHAR* file;
	WCHAR* category;
	DMUS_IO_VERSION version;
} Reference;

typedef struct _BandTrack
{
	DMUS_IO_BAND_TRACK_HEADER header;
	GUID guid;
	DMUS_IO_VERSION version;
	UNFO_List UNFO;
	
	DMUS_IO_BAND_ITEM_HEADER header1;
	DMUS_IO_BAND_ITEM_HEADER2 header2;
	
	/* IDirectMusicBandImpl **band; */
	
} BandTrack;

/*****************************************************************************
 * IDirectMusicImpl implementation structure
 */
struct IDirectMusicImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusic);
  DWORD          ref;

  /* IDirectMusicImpl fields */
  IDirectMusicPortImpl** ports;
  int nrofports;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicImpl_QueryInterface (LPDIRECTMUSIC iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicImpl_AddRef (LPDIRECTMUSIC iface);
extern ULONG WINAPI   IDirectMusicImpl_Release (LPDIRECTMUSIC iface);
/* IDirectMusicImpl: */
extern HRESULT WINAPI IDirectMusicImpl_EnumPort (LPDIRECTMUSIC iface, DWORD dwIndex, LPDMUS_PORTCAPS pPortCaps);
extern HRESULT WINAPI IDirectMusicImpl_CreateMusicBuffer (LPDIRECTMUSIC iface, LPDMUS_BUFFERDESC pBufferDesc, LPDIRECTMUSICBUFFER** ppBuffer, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI IDirectMusicImpl_CreatePort (LPDIRECTMUSIC iface, REFCLSID rclsidPort, LPDMUS_PORTPARAMS pPortParams, LPDIRECTMUSICPORT* ppPort, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI IDirectMusicImpl_EnumMasterClock (LPDIRECTMUSIC iface, DWORD dwIndex, LPDMUS_CLOCKINFO lpClockInfo);
extern HRESULT WINAPI IDirectMusicImpl_GetMasterClock (LPDIRECTMUSIC iface, LPGUID pguidClock, IReferenceClock** ppReferenceClock);
extern HRESULT WINAPI IDirectMusicImpl_SetMasterClock (LPDIRECTMUSIC iface, REFGUID rguidClock);
extern HRESULT WINAPI IDirectMusicImpl_Activate (LPDIRECTMUSIC iface, BOOL fEnable);
extern HRESULT WINAPI IDirectMusicImpl_GetDefaultPort (LPDIRECTMUSIC iface, LPGUID pguidPort);
extern HRESULT WINAPI IDirectMusicImpl_SetDirectSound (LPDIRECTMUSIC iface, LPDIRECTSOUND pDirectSound, HWND hWnd);

/* ClassFactory */
extern HRESULT WINAPI DMUSIC_CreateDirectMusic (LPCGUID lpcGUID, LPDIRECTMUSIC *ppDM, LPUNKNOWN pUnkOuter);

/*****************************************************************************
 * IDirectMusic8Impl implementation structure
 */
struct IDirectMusic8Impl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusic8);
  DWORD          ref;

  /* IDirectMusic8Impl fields */
  IDirectMusicPortImpl** ports;
  int nrofports;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusic8Impl_QueryInterface (LPDIRECTMUSIC8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusic8Impl_AddRef (LPDIRECTMUSIC8 iface);
extern ULONG WINAPI   IDirectMusic8Impl_Release (LPDIRECTMUSIC8 iface);
/* IDirectMusic8Impl: */
extern HRESULT WINAPI IDirectMusic8Impl_EnumPort (LPDIRECTMUSIC8 iface, DWORD dwIndex, LPDMUS_PORTCAPS pPortCaps);
extern HRESULT WINAPI IDirectMusic8Impl_CreateMusicBuffer (LPDIRECTMUSIC8 iface, LPDMUS_BUFFERDESC pBufferDesc, LPDIRECTMUSICBUFFER** ppBuffer, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI IDirectMusic8Impl_CreatePort (LPDIRECTMUSIC8 iface, REFCLSID rclsidPort, LPDMUS_PORTPARAMS pPortParams, LPDIRECTMUSICPORT* ppPort, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI IDirectMusic8Impl_EnumMasterClock (LPDIRECTMUSIC8 iface, DWORD dwIndex, LPDMUS_CLOCKINFO lpClockInfo);
extern HRESULT WINAPI IDirectMusic8Impl_GetMasterClock (LPDIRECTMUSIC8 iface, LPGUID pguidClock, IReferenceClock** ppReferenceClock);
extern HRESULT WINAPI IDirectMusic8Impl_SetMasterClock (LPDIRECTMUSIC8 iface, REFGUID rguidClock);
extern HRESULT WINAPI IDirectMusic8Impl_Activate (LPDIRECTMUSIC8 iface, BOOL fEnable);
extern HRESULT WINAPI IDirectMusic8Impl_GetDefaultPort (LPDIRECTMUSIC8 iface, LPGUID pguidPort);
extern HRESULT WINAPI IDirectMusic8Impl_SetDirectSound (LPDIRECTMUSIC8 iface, LPDIRECTSOUND pDirectSound, HWND hWnd);
extern HRESULT WINAPI IDirectMusic8Impl_SetExternalMasterClock (LPDIRECTMUSIC8 iface, IReferenceClock* pClock);

/*****************************************************************************
 * IDirectMusicBufferImpl implementation structure
 */
struct IDirectMusicBufferImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicBuffer);
  DWORD          ref;

  /* IDirectMusicBufferImpl fields */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicBufferImpl_QueryInterface (LPDIRECTMUSICBUFFER iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicBufferImpl_AddRef (LPDIRECTMUSICBUFFER iface);
extern ULONG WINAPI   IDirectMusicBufferImpl_Release (LPDIRECTMUSICBUFFER iface);
/* IDirectMusicBufferImpl: */
extern HRESULT WINAPI IDirectMusicBufferImpl_Flush (LPDIRECTMUSICBUFFER iface);
extern HRESULT WINAPI IDirectMusicBufferImpl_TotalTime (LPDIRECTMUSICBUFFER iface, LPREFERENCE_TIME prtTime);
extern HRESULT WINAPI IDirectMusicBufferImpl_PackStructured (LPDIRECTMUSICBUFFER iface, REFERENCE_TIME rt, DWORD dwChannelGroup, DWORD dwChannelMessage);
extern HRESULT WINAPI IDirectMusicBufferImpl_PackUnstructured (LPDIRECTMUSICBUFFER iface, REFERENCE_TIME rt, DWORD dwChannelGroup, DWORD cb, LPBYTE lpb);
extern HRESULT WINAPI IDirectMusicBufferImpl_ResetReadPtr (LPDIRECTMUSICBUFFER iface);
extern HRESULT WINAPI IDirectMusicBufferImpl_GetNextEvent (LPDIRECTMUSICBUFFER iface, LPREFERENCE_TIME prt, LPDWORD pdwChannelGroup, LPDWORD pdwLength, LPBYTE* ppData);
extern HRESULT WINAPI IDirectMusicBufferImpl_GetRawBufferPtr (LPDIRECTMUSICBUFFER iface, LPBYTE* ppData);
extern HRESULT WINAPI IDirectMusicBufferImpl_GetStartTime (LPDIRECTMUSICBUFFER iface, LPREFERENCE_TIME prt);
extern HRESULT WINAPI IDirectMusicBufferImpl_GetUsedBytes (LPDIRECTMUSICBUFFER iface, LPDWORD pcb);
extern HRESULT WINAPI IDirectMusicBufferImpl_GetMaxBytes (LPDIRECTMUSICBUFFER iface, LPDWORD pcb);
extern HRESULT WINAPI IDirectMusicBufferImpl_GetBufferFormat (LPDIRECTMUSICBUFFER iface, LPGUID pGuidFormat);
extern HRESULT WINAPI IDirectMusicBufferImpl_SetStartTime (LPDIRECTMUSICBUFFER iface, REFERENCE_TIME rt);
extern HRESULT WINAPI IDirectMusicBufferImpl_SetUsedBytes (LPDIRECTMUSICBUFFER iface, DWORD cb);

/*****************************************************************************
 * IDirectMusicInstrumentImpl implementation structure
 */
struct IDirectMusicInstrumentImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicInstrument);
  DWORD          ref;

  /* IDirectMusicInstrumentImpl fields */
  DWORD patch;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicInstrumentImpl_QueryInterface (LPDIRECTMUSICINSTRUMENT iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicInstrumentImpl_AddRef (LPDIRECTMUSICINSTRUMENT iface);
extern ULONG WINAPI   IDirectMusicInstrumentImpl_Release (LPDIRECTMUSICINSTRUMENT iface);
/* IDirectMusicInstrumentImpl: */
extern HRESULT WINAPI IDirectMusicInstrumentImpl_GetPatch (LPDIRECTMUSICINSTRUMENT iface, DWORD* pdwPatch);
extern HRESULT WINAPI IDirectMusicInstrumentImpl_SetPatch (LPDIRECTMUSICINSTRUMENT iface, DWORD dwPatch);

/*****************************************************************************
 * IDirectMusicDownloadedInstrumentImpl implementation structure
 */
struct IDirectMusicDownloadedInstrumentImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicDownloadedInstrument);
  DWORD          ref;

  /* IDirectMusicDownloadedInstrumentImpl fields */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicDownloadedInstrumentImpl_QueryInterface (LPDIRECTMUSICDOWNLOADEDINSTRUMENT iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicDownloadedInstrumentImpl_AddRef (LPDIRECTMUSICDOWNLOADEDINSTRUMENT iface);
extern ULONG WINAPI   IDirectMusicDownloadedInstrumentImpl_Release (LPDIRECTMUSICDOWNLOADEDINSTRUMENT iface);
/* IDirectMusicDownloadedInstrumentImpl: */
/* none yet */

/*****************************************************************************
 * IDirectMusicCollectionImpl implementation structure
 */
struct IDirectMusicCollectionImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicCollection);
  DWORD          ref;

  /* IDirectMusicCollectionImpl fields */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicCollectionImpl_QueryInterface (LPDIRECTMUSICCOLLECTION iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicCollectionImpl_AddRef (LPDIRECTMUSICCOLLECTION iface);
extern ULONG WINAPI   IDirectMusicCollectionImpl_Release (LPDIRECTMUSICCOLLECTION iface);
/* IDirectMusicImpl: */
HRESULT WINAPI IDirectMusicCollectionImpl_GetInstrument (LPDIRECTMUSICCOLLECTION iface, DWORD dwPatch, IDirectMusicInstrument** ppInstrument);
HRESULT WINAPI IDirectMusicCollectionImpl_EnumInstrument (LPDIRECTMUSICCOLLECTION iface, DWORD dwIndex, DWORD* pdwPatch, LPWSTR pwszName, DWORD dwNameLen);

/*****************************************************************************
 * IDirectMusicDownloadImpl implementation structure
 */
struct IDirectMusicDownloadImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicDownload);
  DWORD          ref;

  /* IDirectMusicDownloadImpl fields */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicDownloadImpl_QueryInterface (LPDIRECTMUSICDOWNLOAD iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicDownloadImpl_AddRef (LPDIRECTMUSICDOWNLOAD iface);
extern ULONG WINAPI   IDirectMusicDownloadImpl_Release (LPDIRECTMUSICDOWNLOAD iface);
/* IDirectMusicImpl: */
extern HRESULT WINAPI IDirectMusicDownloadImpl_GetBuffer (LPDIRECTMUSICDOWNLOAD iface, void** ppvBuffer, DWORD* pdwSize);

/*****************************************************************************
 * IDirectMusicPortDownloadImpl implementation structure
 */
struct IDirectMusicPortDownloadImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicPortDownload);
  DWORD          ref;

  /* IDirectMusicPortDownloadImpl fields */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicPortDownloadImpl_QueryInterface (LPDIRECTMUSICPORTDOWNLOAD iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicPortDownloadImpl_AddRef (LPDIRECTMUSICPORTDOWNLOAD iface);
extern ULONG WINAPI   IDirectMusicPortDownloadImpl_Release (LPDIRECTMUSICPORTDOWNLOAD iface);
/* IDirectMusicPortDownloadImpl: */
extern HRESULT WINAPI IDirectMusicPortDownloadImpl_GetBuffer (LPDIRECTMUSICPORTDOWNLOAD iface, DWORD dwDLId, IDirectMusicDownload** ppIDMDownload);
extern HRESULT WINAPI IDirectMusicPortDownloadImpl_AllocateBuffer (LPDIRECTMUSICPORTDOWNLOAD iface, DWORD dwSize, IDirectMusicDownload** ppIDMDownload);
extern HRESULT WINAPI IDirectMusicPortDownloadImpl_GetDLId (LPDIRECTMUSICPORTDOWNLOAD iface, DWORD* pdwStartDLId, DWORD dwCount);
extern HRESULT WINAPI IDirectMusicPortDownloadImpl_GetAppend (LPDIRECTMUSICPORTDOWNLOAD iface, DWORD* pdwAppend);
extern HRESULT WINAPI IDirectMusicPortDownloadImpl_Download (LPDIRECTMUSICPORTDOWNLOAD iface, IDirectMusicDownload* pIDMDownload);
extern HRESULT WINAPI IDirectMusicPortDownloadImpl_Unload (LPDIRECTMUSICPORTDOWNLOAD iface, IDirectMusicDownload* pIDMDownload);

/*****************************************************************************
 * IDirectMusicPortImpl implementation structure
 */
struct IDirectMusicPortImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicPort);
  DWORD          ref;

  /* IDirectMusicPortImpl fields */
  BOOL active;
  LPDMUS_PORTCAPS caps;
  LPDMUS_PORTPARAMS params;
  int nrofgroups;
  DMUSIC_PRIVATE_CHANNEL_GROUP group[1];
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicPortImpl_QueryInterface (LPDIRECTMUSICPORT iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicPortImpl_AddRef (LPDIRECTMUSICPORT iface);
extern ULONG WINAPI   IDirectMusicPortImpl_Release (LPDIRECTMUSICPORT iface);
/* IDirectMusicPortDownloadImpl: */
extern HRESULT WINAPI IDirectMusicPortImpl_PlayBuffer (LPDIRECTMUSICPORT iface, LPDIRECTMUSICBUFFER pBuffer);
extern HRESULT WINAPI IDirectMusicPortImpl_SetReadNotificationHandle (LPDIRECTMUSICPORT iface, HANDLE hEvent);
extern HRESULT WINAPI IDirectMusicPortImpl_Read (LPDIRECTMUSICPORT iface, LPDIRECTMUSICBUFFER pBuffer);
extern HRESULT WINAPI IDirectMusicPortImpl_DownloadInstrument (LPDIRECTMUSICPORT iface, IDirectMusicInstrument* pInstrument, IDirectMusicDownloadedInstrument** ppDownloadedInstrument, DMUS_NOTERANGE* pNoteRanges, DWORD dwNumNoteRanges);
extern HRESULT WINAPI IDirectMusicPortImpl_UnloadInstrument (LPDIRECTMUSICPORT iface, IDirectMusicDownloadedInstrument *pDownloadedInstrument);
extern HRESULT WINAPI IDirectMusicPortImpl_GetLatencyClock (LPDIRECTMUSICPORT iface, IReferenceClock** ppClock);
extern HRESULT WINAPI IDirectMusicPortImpl_GetRunningStats (LPDIRECTMUSICPORT iface, LPDMUS_SYNTHSTATS pStats);
extern HRESULT WINAPI IDirectMusicPortImpl_GetCaps (LPDIRECTMUSICPORT iface, LPDMUS_PORTCAPS pPortCaps);
extern HRESULT WINAPI IDirectMusicPortImpl_DeviceIoControl (LPDIRECTMUSICPORT iface, DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped);
extern HRESULT WINAPI IDirectMusicPortImpl_SetNumChannelGroups (LPDIRECTMUSICPORT iface, DWORD dwChannelGroups);
extern HRESULT WINAPI IDirectMusicPortImpl_GetNumChannelGroups (LPDIRECTMUSICPORT iface, LPDWORD pdwChannelGroups);
extern HRESULT WINAPI IDirectMusicPortImpl_Activate (LPDIRECTMUSICPORT iface, BOOL fActive);
extern HRESULT WINAPI IDirectMusicPortImpl_SetChannelPriority (LPDIRECTMUSICPORT iface, DWORD dwChannelGroup, DWORD dwChannel, DWORD dwPriority);
extern HRESULT WINAPI IDirectMusicPortImpl_GetChannelPriority (LPDIRECTMUSICPORT iface, DWORD dwChannelGroup, DWORD dwChannel, LPDWORD pdwPriority);
extern HRESULT WINAPI IDirectMusicPortImpl_SetDirectSound (LPDIRECTMUSICPORT iface, LPDIRECTSOUND pDirectSound, LPDIRECTSOUNDBUFFER pDirectSoundBuffer);
extern HRESULT WINAPI IDirectMusicPortImpl_GetFormat (LPDIRECTMUSICPORT iface, LPWAVEFORMATEX pWaveFormatEx, LPDWORD pdwWaveFormatExSize, LPDWORD pdwBufferSize);

/*****************************************************************************
 * IDirectMusicThruImpl implementation structure
 */
struct IDirectMusicThruImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicThru);
  DWORD          ref;

  /* IDirectMusicThruImpl fields */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicThruImpl_QueryInterface (LPDIRECTMUSICTHRU iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicThruImpl_AddRef (LPDIRECTMUSICTHRU iface);
extern ULONG WINAPI   IDirectMusicThruImpl_Release (LPDIRECTMUSICTHRU iface);
/* IDirectMusicPortDownloadImpl: */
extern HRESULT WINAPI ThruChannel (LPDIRECTMUSICTHRU iface, DWORD dwSourceChannelGroup, DWORD dwSourceChannel, DWORD dwDestinationChannelGroup, DWORD dwDestinationChannel, LPDIRECTMUSICPORT pDestinationPort);

/*****************************************************************************
 * IReferenceClockImpl implementation structure
 */
struct IReferenceClockImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IReferenceClock);
  DWORD          ref;

  /* IReferenceClockImpl fields */
};

/* IUnknown: */
extern HRESULT WINAPI IReferenceClockImpl_QueryInterface (LPREFERENCECLOCK iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IReferenceClockImpl_AddRef (LPREFERENCECLOCK iface);
extern ULONG WINAPI   IReferenceClockImpl_Release (LPREFERENCECLOCK iface);
/* IReferenceClock: */
extern HRESULT WINAPI IReferenceClockImpl_GetTime (LPREFERENCECLOCK iface, REFERENCE_TIME* pTime);
extern HRESULT WINAPI IReferenceClockImpl_AdviseTime (LPREFERENCECLOCK iface, REFERENCE_TIME baseTime, REFERENCE_TIME streamTime, HANDLE hEvent, DWORD* pdwAdviseCookie);
extern HRESULT WINAPI IReferenceClockImpl_AdvisePeriodic (LPREFERENCECLOCK iface, REFERENCE_TIME startTime, REFERENCE_TIME periodTime, HANDLE hSemaphore, DWORD* pdwAdviseCookie);
extern HRESULT WINAPI IReferenceClockImpl_Unadvise (LPREFERENCECLOCK iface, DWORD dwAdviseCookie);


/*****************************************************************************
 * IDirectMusicSynthImpl implementation structure
 */
struct IDirectMusicSynthImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicSynth);
  DWORD          ref;

  /* IDirectMusicSynth fields */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicSynthImpl_QueryInterface (LPDIRECTMUSICSYNTH iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicSynthImpl_AddRef (LPDIRECTMUSICSYNTH iface);
extern ULONG WINAPI   IDirectMusicSynthImpl_Release (LPDIRECTMUSICSYNTH iface);
/* IDirectMusicSynth: */
extern HRESULT WINAPI IDirectMusicSynthImpl_Open (LPDIRECTMUSICSYNTH iface,LPDMUS_PORTPARAMS pPortParams);
extern HRESULT WINAPI IDirectMusicSynthImpl_Close (LPDIRECTMUSICSYNTH iface);
extern HRESULT WINAPI IDirectMusicSynthImpl_SetNumChannelGroups (LPDIRECTMUSICSYNTH iface, DWORD dwGroups);
extern HRESULT WINAPI IDirectMusicSynthImpl_Download (LPDIRECTMUSICSYNTH iface, LPHANDLE phDownload, LPVOID pvData, LPBOOL pbFree);
extern HRESULT WINAPI IDirectMusicSynthImpl_Unload (LPDIRECTMUSICSYNTH iface, HANDLE hDownload, HRESULT (CALLBACK* lpFreeHandle)(HANDLE,HANDLE), HANDLE hUserData);
extern HRESULT WINAPI IDirectMusicSynthImpl_PlayBuffer (LPDIRECTMUSICSYNTH iface, REFERENCE_TIME rt, LPBYTE pbBuffer, DWORD cbBuffer);
extern HRESULT WINAPI IDirectMusicSynthImpl_GetRunningStats (LPDIRECTMUSICSYNTH iface, LPDMUS_SYNTHSTATS pStats);
extern HRESULT WINAPI IDirectMusicSynthImpl_GetPortCaps (LPDIRECTMUSICSYNTH iface, LPDMUS_PORTCAPS pCaps);
extern HRESULT WINAPI IDirectMusicSynthImpl_SetMasterClock (LPDIRECTMUSICSYNTH iface, IReferenceClock* pClock);
extern HRESULT WINAPI IDirectMusicSynthImpl_GetLatencyClock (LPDIRECTMUSICSYNTH iface, IReferenceClock** ppClock);
extern HRESULT WINAPI IDirectMusicSynthImpl_Activate (LPDIRECTMUSICSYNTH iface, BOOL fEnable);
extern HRESULT WINAPI IDirectMusicSynthImpl_SetSynthSink (LPDIRECTMUSICSYNTH iface, IDirectMusicSynthSink* pSynthSink);
extern HRESULT WINAPI IDirectMusicSynthImpl_Render (LPDIRECTMUSICSYNTH iface, short* pBuffer, DWORD dwLength, LONGLONG llPosition);
extern HRESULT WINAPI IDirectMusicSynthImpl_SetChannelPriority (LPDIRECTMUSICSYNTH iface, DWORD dwChannelGroup, DWORD dwChannel, DWORD dwPriority);
extern HRESULT WINAPI IDirectMusicSynthImpl_GetChannelPriority (LPDIRECTMUSICSYNTH iface, DWORD dwChannelGroup, DWORD dwChannel, LPDWORD pdwPriority);
extern HRESULT WINAPI IDirectMusicSynthImpl_GetFormat (LPDIRECTMUSICSYNTH iface, LPWAVEFORMATEX pWaveFormatEx, LPDWORD pdwWaveFormatExSiz);
extern HRESULT WINAPI IDirectMusicSynthImpl_GetAppend (LPDIRECTMUSICSYNTH iface, DWORD* pdwAppend);
	
/*****************************************************************************
 * IDirectMusicSynth8Impl implementation structure
 */
struct IDirectMusicSynth8Impl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicSynth8);
  DWORD          ref;

  /* IDirectMusicSynth8 fields */
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
struct IDirectMusicSynthSinkImpl
{
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


/*****************************************************************************
 * IDirectMusicToolImpl implementation structure
 */
struct IDirectMusicToolImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicTool);
  DWORD          ref;

  /* IDirectMusicToolImpl fields */
  IDirectMusicToolImpl* prev;
  IDirectMusicToolImpl* next;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicToolImpl_QueryInterface (LPDIRECTMUSICTOOL iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicToolImpl_AddRef (LPDIRECTMUSICTOOL iface);
extern ULONG WINAPI   IDirectMusicToolImpl_Release (LPDIRECTMUSICTOOL iface);
/* IDirectMusicToolImpl: */
extern HRESULT WINAPI IDirectMusicToolImpl_Init (LPDIRECTMUSICTOOL iface, IDirectMusicGraph* pGraph);
extern HRESULT WINAPI IDirectMusicToolImpl_GetMsgDeliveryType (LPDIRECTMUSICTOOL iface, DWORD* pdwDeliveryType);
extern HRESULT WINAPI IDirectMusicToolImpl_GetMediaTypeArraySize (LPDIRECTMUSICTOOL iface, DWORD* pdwNumElements);
extern HRESULT WINAPI IDirectMusicToolImpl_GetMediaTypes (LPDIRECTMUSICTOOL iface, DWORD** padwMediaTypes, DWORD dwNumElements);
extern HRESULT WINAPI IDirectMusicToolImpl_ProcessPMsg (LPDIRECTMUSICTOOL iface, IDirectMusicPerformance* pPerf, DMUS_PMSG* pPMSG);
extern HRESULT WINAPI IDirectMusicToolImpl_Flush (LPDIRECTMUSICTOOL iface, IDirectMusicPerformance* pPerf, DMUS_PMSG* pPMSG, REFERENCE_TIME rtTime);

/*****************************************************************************
 * IDirectMusicTool8Impl implementation structure
 */
struct IDirectMusicTool8Impl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicTool8);
  DWORD          ref;

  /* IDirectMusicTool8Impl fields */
  IDirectMusicToolImpl* prev;
  IDirectMusicToolImpl* next;
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
 * IDirectMusicTrackImpl implementation structure
 */
struct IDirectMusicTrackImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicTrack);
  DWORD          ref;

  /* IDirectMusicTrackImpl fields */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicTrackImpl_QueryInterface (LPDIRECTMUSICTRACK iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicTrackImpl_AddRef (LPDIRECTMUSICTRACK iface);
extern ULONG WINAPI   IDirectMusicTrackImpl_Release (LPDIRECTMUSICTRACK iface);
/* IDirectMusicTrack: */
extern HRESULT WINAPI IDirectMusicTrackImpl_Init (LPDIRECTMUSICTRACK iface, IDirectMusicSegment* pSegment);
extern HRESULT WINAPI IDirectMusicTrackImpl_InitPlay (LPDIRECTMUSICTRACK iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrackID, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicTrackImpl_EndPlay (LPDIRECTMUSICTRACK iface, void* pStateData);
extern HRESULT WINAPI IDirectMusicTrackImpl_Play (LPDIRECTMUSICTRACK iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicTrackImpl_GetParam (LPDIRECTMUSICTRACK iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicTrackImpl_SetParam (LPDIRECTMUSICTRACK iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicTrackImpl_IsParamSupported (LPDIRECTMUSICTRACK iface, REFGUID rguidType);
extern HRESULT WINAPI IDirectMusicTrackImpl_AddNotificationType (LPDIRECTMUSICTRACK iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicTrackImpl_RemoveNotificationType (LPDIRECTMUSICTRACK iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicTrackImpl_Clone (LPDIRECTMUSICTRACK iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack);

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
 * IDirectMusicBandImpl implementation structure
 */
struct IDirectMusicBandImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicBand);
  DWORD          ref;

  /* IDirectMusicBandImpl fields */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicBandImpl_QueryInterface (LPDIRECTMUSICBAND iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicBandImpl_AddRef (LPDIRECTMUSICBAND iface);
extern ULONG WINAPI   IDirectMusicBandImpl_Release (LPDIRECTMUSICBAND iface);
/* IDirectMusicBand: */
extern HRESULT WINAPI IDirectMusicBandImpl_CreateSegment (LPDIRECTMUSICBAND iface, IDirectMusicSegment** ppSegment);
extern HRESULT WINAPI IDirectMusicBandImpl_Download (LPDIRECTMUSICBAND iface, IDirectMusicPerformance* pPerformance);
extern HRESULT WINAPI IDirectMusicBandImpl_Unload (LPDIRECTMUSICBAND iface, IDirectMusicPerformance* pPerformance);

/*****************************************************************************
 * IDirectMusicObjectImpl implementation structure
 */
struct IDirectMusicObjectImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicObject);
  DWORD          ref;

  /* IDirectMusicObjectImpl fields */
  LPDMUS_OBJECTDESC desc;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicObjectImpl_QueryInterface (LPDIRECTMUSICOBJECT iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicObjectImpl_AddRef (LPDIRECTMUSICOBJECT iface);
extern ULONG WINAPI   IDirectMusicObjectImpl_Release (LPDIRECTMUSICOBJECT iface);
/* IDirectMusicObject: */
extern HRESULT WINAPI IDirectMusicObjectImpl_GetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicObjectImpl_SetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicObjectImpl_ParseDescriptor (LPDIRECTMUSICOBJECT iface, LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc);

/*****************************************************************************
 * IDirectMusicLoaderImpl implementation structure
 */
struct IDirectMusicLoaderImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicLoader);
  DWORD          ref;

  /* IDirectMusicLoaderImpl fields */
  WCHAR          searchPath[MAX_PATH];
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicLoaderImpl_QueryInterface (LPDIRECTMUSICLOADER iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicLoaderImpl_AddRef (LPDIRECTMUSICLOADER iface);
extern ULONG WINAPI   IDirectMusicLoaderImpl_Release (LPDIRECTMUSICLOADER iface);
/* IDirectMusicLoader: */
extern HRESULT WINAPI IDirectMusicLoaderImpl_GetObject (LPDIRECTMUSICLOADER iface, LPDMUS_OBJECTDESC pDesc, REFIID riid, LPVOID*ppv);
extern HRESULT WINAPI IDirectMusicLoaderImpl_SetObject (LPDIRECTMUSICLOADER iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicLoaderImpl_SetSearchDirectory (LPDIRECTMUSICLOADER iface, REFGUID rguidClass, WCHAR* pwzPath, BOOL fClear);
extern HRESULT WINAPI IDirectMusicLoaderImpl_ScanDirectory (LPDIRECTMUSICLOADER iface, REFGUID rguidClass, WCHAR* pwzFileExtension, WCHAR* pwzScanFileName);
extern HRESULT WINAPI IDirectMusicLoaderImpl_CacheObject (LPDIRECTMUSICLOADER iface, IDirectMusicObject* pObject);
extern HRESULT WINAPI IDirectMusicLoaderImpl_ReleaseObject (LPDIRECTMUSICLOADER iface, IDirectMusicObject* pObject);
extern HRESULT WINAPI IDirectMusicLoaderImpl_ClearCache (LPDIRECTMUSICLOADER iface, REFGUID rguidClass);
extern HRESULT WINAPI IDirectMusicLoaderImpl_EnableCache (LPDIRECTMUSICLOADER iface, REFGUID rguidClass, BOOL fEnable);
extern HRESULT WINAPI IDirectMusicLoaderImpl_EnumObject (LPDIRECTMUSICLOADER iface, REFGUID rguidClass, DWORD dwIndex, LPDMUS_OBJECTDESC pDesc);

/*****************************************************************************
 * IDirectMusicLoader8Impl implementation structure
 */
struct IDirectMusicLoader8Impl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicLoader8);
  DWORD          ref;

  /* IDirectMusicLoader8Impl fields */
  WCHAR          searchPath[MAX_PATH];
  /* IDirectMusicLoader8Impl fields */ 
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicLoader8Impl_QueryInterface (LPDIRECTMUSICLOADER8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicLoader8Impl_AddRef (LPDIRECTMUSICLOADER8 iface);
extern ULONG WINAPI   IDirectMusicLoader8Impl_Release (LPDIRECTMUSICLOADER8 iface);
/* IDirectMusicLoader: */
extern HRESULT WINAPI IDirectMusicLoader8Impl_GetObject (LPDIRECTMUSICLOADER8 iface, LPDMUS_OBJECTDESC pDesc, REFIID riid, LPVOID*ppv);
extern HRESULT WINAPI IDirectMusicLoader8Impl_SetObject (LPDIRECTMUSICLOADER8 iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicLoader8Impl_SetSearchDirectory (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass, WCHAR* pwzPath, BOOL fClear);
extern HRESULT WINAPI IDirectMusicLoader8Impl_ScanDirectory (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass, WCHAR* pwzFileExtension, WCHAR* pwzScanFileName);
extern HRESULT WINAPI IDirectMusicLoader8Impl_CacheObject (LPDIRECTMUSICLOADER8 iface, IDirectMusicObject* pObject);
extern HRESULT WINAPI IDirectMusicLoader8Impl_ReleaseObject (LPDIRECTMUSICLOADER8 iface, IDirectMusicObject* pObject);
extern HRESULT WINAPI IDirectMusicLoader8Impl_ClearCache (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass);
extern HRESULT WINAPI IDirectMusicLoader8Impl_EnableCache (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass, BOOL fEnable);
extern HRESULT WINAPI IDirectMusicLoader8Impl_EnumObject (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass, DWORD dwIndex, LPDMUS_OBJECTDESC pDesc);
/* IDirectMusicLoader8: */
extern void WINAPI IDirectMusicLoader8Impl_CollectGarbage (LPDIRECTMUSICLOADER8 iface);
extern HRESULT WINAPI IDirectMusicLoader8Impl_ReleaseObjectByUnknown (LPDIRECTMUSICLOADER8 iface, IUnknown* pObject);
extern HRESULT WINAPI IDirectMusicLoader8Impl_LoadObjectFromFile (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClassID, REFIID iidInterfaceID, WCHAR* pwzFilePath, void** ppObject);
/* ClassFactory */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicLoader8 (LPCGUID lpcGUID, LPDIRECTMUSICLOADER8 *ppDMLoad8, LPUNKNOWN pUnkOuter);

/*****************************************************************************
 * IDirectMusicGetLoaderImpl implementation structure
 */
struct IDirectMusicGetLoaderImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicGetLoader);
  DWORD          ref;

  /* IDirectMusicGetLoaderImpl fields */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicGetLoaderImpl_QueryInterface (LPDIRECTMUSICGETLOADER iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicGetLoaderImpl_AddRef (LPDIRECTMUSICGETLOADER iface);
extern ULONG WINAPI   IDirectMusicGetLoaderImpl_Release (LPDIRECTMUSICGETLOADER iface);
/* IDirectMusicGetLoader: */
extern HRESULT WINAPI IDirectMusicGetLoaderImpl_GetLoader (LPDIRECTMUSICGETLOADER iface, IDirectMusicLoader** ppLoader);

/*****************************************************************************
 * IDirectMusicSegmentImpl implementation structure
 */
struct IDirectMusicSegmentImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicSegment);
  DWORD          ref;

  /* IDirectMusicSegmentImpl fields */  
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicSegmentImpl_QueryInterface (LPDIRECTMUSICSEGMENT iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicSegmentImpl_AddRef (LPDIRECTMUSICSEGMENT iface);
extern ULONG WINAPI   IDirectMusicSegmentImpl_Release (LPDIRECTMUSICSEGMENT iface);
/* IDirectMusicSegment: */
extern HRESULT WINAPI IDirectMusicSegmentImpl_GetLength (LPDIRECTMUSICSEGMENT iface, MUSIC_TIME* pmtLength);
extern HRESULT WINAPI IDirectMusicSegmentImpl_SetLength (LPDIRECTMUSICSEGMENT iface, MUSIC_TIME mtLength);
extern HRESULT WINAPI IDirectMusicSegmentImpl_GetRepeats (LPDIRECTMUSICSEGMENT iface, DWORD* pdwRepeats);
extern HRESULT WINAPI IDirectMusicSegmentImpl_SetRepeats (LPDIRECTMUSICSEGMENT iface, DWORD dwRepeats);
extern HRESULT WINAPI IDirectMusicSegmentImpl_GetDefaultResolution (LPDIRECTMUSICSEGMENT iface, DWORD* pdwResolution);
extern HRESULT WINAPI IDirectMusicSegmentImpl_SetDefaultResolution (LPDIRECTMUSICSEGMENT iface, DWORD dwResolution);
extern HRESULT WINAPI IDirectMusicSegmentImpl_GetTrack (LPDIRECTMUSICSEGMENT iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, IDirectMusicTrack** ppTrack);
extern HRESULT WINAPI IDirectMusicSegmentImpl_GetTrackGroup (LPDIRECTMUSICSEGMENT iface, IDirectMusicTrack* pTrack, DWORD* pdwGroupBits);
extern HRESULT WINAPI IDirectMusicSegmentImpl_InsertTrack (LPDIRECTMUSICSEGMENT iface, IDirectMusicTrack* pTrack, DWORD dwGroupBits);
extern HRESULT WINAPI IDirectMusicSegmentImpl_RemoveTrack (LPDIRECTMUSICSEGMENT iface, IDirectMusicTrack* pTrack);
extern HRESULT WINAPI IDirectMusicSegmentImpl_InitPlay (LPDIRECTMUSICSEGMENT iface, IDirectMusicSegmentState** ppSegState, IDirectMusicPerformance* pPerformance, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicSegmentImpl_GetGraph (LPDIRECTMUSICSEGMENT iface, IDirectMusicGraph** ppGraph);
extern HRESULT WINAPI IDirectMusicSegmentImpl_SetGraph (LPDIRECTMUSICSEGMENT iface, IDirectMusicGraph* pGraph);
extern HRESULT WINAPI IDirectMusicSegmentImpl_AddNotificationType (LPDIRECTMUSICSEGMENT iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicSegmentImpl_RemoveNotificationType (LPDIRECTMUSICSEGMENT iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicSegmentImpl_GetParam (LPDIRECTMUSICSEGMENT iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicSegmentImpl_SetParam (LPDIRECTMUSICSEGMENT iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicSegmentImpl_Clone (LPDIRECTMUSICSEGMENT iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicSegment** ppSegment);
extern HRESULT WINAPI IDirectMusicSegmentImpl_SetStartPoint (LPDIRECTMUSICSEGMENT iface, MUSIC_TIME mtStart);
extern HRESULT WINAPI IDirectMusicSegmentImpl_GetStartPoint (LPDIRECTMUSICSEGMENT iface, MUSIC_TIME* pmtStart);
extern HRESULT WINAPI IDirectMusicSegmentImpl_SetLoopPoints (LPDIRECTMUSICSEGMENT iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd);
extern HRESULT WINAPI IDirectMusicSegmentImpl_GetLoopPoints (LPDIRECTMUSICSEGMENT iface, MUSIC_TIME* pmtStart, MUSIC_TIME* pmtEnd);
extern HRESULT WINAPI IDirectMusicSegmentImpl_SetPChannelsUsed (LPDIRECTMUSICSEGMENT iface, DWORD dwNumPChannels, DWORD* paPChannels);

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
 * IDirectMusicSegmentStateImpl implementation structure
 */
struct IDirectMusicSegmentStateImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicSegmentState);
  DWORD          ref;

  /* IDirectMusicSegmentStateImpl fields */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicSegmentStateImpl_QueryInterface (LPDIRECTMUSICSEGMENTSTATE iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicSegmentStateImpl_AddRef (LPDIRECTMUSICSEGMENTSTATE iface);
extern ULONG WINAPI   IDirectMusicSegmentStateImpl_Release (LPDIRECTMUSICSEGMENTSTATE iface);
/* IDirectMusicSegmentState: */
extern HRESULT WINAPI IDirectMusicSegmentStateImpl_GetRepeats (LPDIRECTMUSICSEGMENTSTATE iface,  DWORD* pdwRepeats);
extern HRESULT WINAPI IDirectMusicSegmentStateImpl_GetSegment (LPDIRECTMUSICSEGMENTSTATE iface, IDirectMusicSegment** ppSegment);
extern HRESULT WINAPI IDirectMusicSegmentStateImpl_GetStartTime (LPDIRECTMUSICSEGMENTSTATE iface, MUSIC_TIME* pmtStart);
extern HRESULT WINAPI IDirectMusicSegmentStateImpl_GetSeek (LPDIRECTMUSICSEGMENTSTATE iface, MUSIC_TIME* pmtSeek);
extern HRESULT WINAPI IDirectMusicSegmentStateImpl_GetStartPoint (LPDIRECTMUSICSEGMENTSTATE iface, MUSIC_TIME* pmtStart);

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
 * IDirectMusicAudioPathImpl implementation structure
 */
struct IDirectMusicAudioPathImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicAudioPath);
  DWORD          ref;

  /* IDirectMusicAudioPathImpl fields */
  IDirectMusicPerformance* perfo;
  IDirectMusicGraph*       toolGraph;
  IDirectSoundBuffer*      buffer;
  IDirectSoundBuffer*      primary;
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
 * IDirectMusicPerformanceImpl implementation structure
 */
struct IDirectMusicPerformanceImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicPerformance);
  DWORD              ref;

  /* IDirectMusicPerformanceImpl fields */
  IDirectMusic*      dmusic;
  IDirectSound*      dsound;
  IDirectMusicGraph* toolGraph;
  DMUS_AUDIOPARAMS   params;
	
  /* global parameters */
  BOOL  AutoDownload;
  char  MasterGrooveLevel;
  float MasterTempo;
  long  MasterVolume;
	
  /* performance channels */
  DMUSIC_PRIVATE_PCHANNEL PChannel[1];
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicPerformanceImpl_QueryInterface (LPDIRECTMUSICPERFORMANCE iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicPerformanceImpl_AddRef (LPDIRECTMUSICPERFORMANCE iface);
extern ULONG WINAPI   IDirectMusicPerformanceImpl_Release (LPDIRECTMUSICPERFORMANCE iface);
/* IDirectMusicPerformance: */
extern HRESULT WINAPI IDirectMusicPerformanceImpl_Init (LPDIRECTMUSICPERFORMANCE iface, IDirectMusic** ppDirectMusic, LPDIRECTSOUND pDirectSound, HWND hWnd);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_PlaySegment (LPDIRECTMUSICPERFORMANCE iface, IDirectMusicSegment* pSegment, DWORD dwFlags, __int64 i64StartTime, IDirectMusicSegmentState** ppSegmentState);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_Stop (LPDIRECTMUSICPERFORMANCE iface, IDirectMusicSegment* pSegment, IDirectMusicSegmentState* pSegmentState, MUSIC_TIME mtTime, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_GetSegmentState (LPDIRECTMUSICPERFORMANCE iface, IDirectMusicSegmentState** ppSegmentState, MUSIC_TIME mtTime);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_SetPrepareTime (LPDIRECTMUSICPERFORMANCE iface, DWORD dwMilliSeconds);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_GetPrepareTime (LPDIRECTMUSICPERFORMANCE iface, DWORD* pdwMilliSeconds);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_SetBumperLength (LPDIRECTMUSICPERFORMANCE iface, DWORD dwMilliSeconds);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_GetBumperLength (LPDIRECTMUSICPERFORMANCE iface, DWORD* pdwMilliSeconds);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_SendPMsg (LPDIRECTMUSICPERFORMANCE iface, DMUS_PMSG* pPMSG);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_MusicToReferenceTime (LPDIRECTMUSICPERFORMANCE iface, MUSIC_TIME mtTime, REFERENCE_TIME* prtTime);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_ReferenceToMusicTime (LPDIRECTMUSICPERFORMANCE iface, REFERENCE_TIME rtTime, MUSIC_TIME* pmtTime);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_IsPlaying (LPDIRECTMUSICPERFORMANCE iface, IDirectMusicSegment* pSegment, IDirectMusicSegmentState* pSegState);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_GetTime (LPDIRECTMUSICPERFORMANCE iface, REFERENCE_TIME* prtNow, MUSIC_TIME* pmtNow);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_AllocPMsg (LPDIRECTMUSICPERFORMANCE iface, ULONG cb, DMUS_PMSG** ppPMSG);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_FreePMsg (LPDIRECTMUSICPERFORMANCE iface, DMUS_PMSG* pPMSG);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_GetGraph (LPDIRECTMUSICPERFORMANCE iface, IDirectMusicGraph** ppGraph);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_SetGraph (LPDIRECTMUSICPERFORMANCE iface, IDirectMusicGraph* pGraph);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_SetNotificationHandle (LPDIRECTMUSICPERFORMANCE iface, HANDLE hNotification, REFERENCE_TIME rtMinimum);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_GetNotificationPMsg (LPDIRECTMUSICPERFORMANCE iface, DMUS_NOTIFICATION_PMSG** ppNotificationPMsg);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_AddNotificationType (LPDIRECTMUSICPERFORMANCE iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_RemoveNotificationType (LPDIRECTMUSICPERFORMANCE iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_AddPort (LPDIRECTMUSICPERFORMANCE iface, IDirectMusicPort* pPort);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_RemovePort (LPDIRECTMUSICPERFORMANCE iface, IDirectMusicPort* pPort);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_AssignPChannelBlock (LPDIRECTMUSICPERFORMANCE iface, DWORD dwBlockNum, IDirectMusicPort* pPort, DWORD dwGroup);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_AssignPChannel (LPDIRECTMUSICPERFORMANCE iface, DWORD dwPChannel, IDirectMusicPort* pPort, DWORD dwGroup, DWORD dwMChannel);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_PChannelInfo (LPDIRECTMUSICPERFORMANCE iface, DWORD dwPChannel, IDirectMusicPort** ppPort, DWORD* pdwGroup, DWORD* pdwMChannel);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_DownloadInstrument (LPDIRECTMUSICPERFORMANCE iface, IDirectMusicInstrument* pInst, DWORD dwPChannel, IDirectMusicDownloadedInstrument** ppDownInst, DMUS_NOTERANGE* pNoteRanges, DWORD dwNumNoteRanges, IDirectMusicPort** ppPort, DWORD* pdwGroup, DWORD* pdwMChannel);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_Invalidate (LPDIRECTMUSICPERFORMANCE iface, MUSIC_TIME mtTime, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_GetParam (LPDIRECTMUSICPERFORMANCE iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_SetParam (LPDIRECTMUSICPERFORMANCE iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_GetGlobalParam (LPDIRECTMUSICPERFORMANCE iface, REFGUID rguidType, void* pParam, DWORD dwSize);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_SetGlobalParam (LPDIRECTMUSICPERFORMANCE iface, REFGUID rguidType, void* pParam, DWORD dwSize);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_GetLatencyTime (LPDIRECTMUSICPERFORMANCE iface, REFERENCE_TIME* prtTime);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_GetQueueTime (LPDIRECTMUSICPERFORMANCE iface, REFERENCE_TIME* prtTime);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_AdjustTime (LPDIRECTMUSICPERFORMANCE iface, REFERENCE_TIME rtAmount);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_CloseDown (LPDIRECTMUSICPERFORMANCE iface);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_GetResolvedTime (LPDIRECTMUSICPERFORMANCE iface, REFERENCE_TIME rtTime, REFERENCE_TIME* prtResolved, DWORD dwTimeResolveFlags);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_MIDIToMusic (LPDIRECTMUSICPERFORMANCE iface, BYTE bMIDIValue, DMUS_CHORD_KEY* pChord, BYTE bPlayMode, BYTE bChordLevel, WORD* pwMusicValue);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_MusicToMIDI (LPDIRECTMUSICPERFORMANCE iface, WORD wMusicValue, DMUS_CHORD_KEY* pChord, BYTE bPlayMode, BYTE bChordLevel, BYTE* pbMIDIValue);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_TimeToRhythm (LPDIRECTMUSICPERFORMANCE iface, MUSIC_TIME mtTime, DMUS_TIMESIGNATURE* pTimeSig, WORD* pwMeasure, BYTE* pbBeat, BYTE* pbGrid, short* pnOffset);
extern HRESULT WINAPI IDirectMusicPerformanceImpl_RhythmToTime (LPDIRECTMUSICPERFORMANCE iface, WORD wMeasure, BYTE bBeat, BYTE bGrid, short nOffset, DMUS_TIMESIGNATURE* pTimeSig, MUSIC_TIME* pmtTime);
/* ClassFactory */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicPerformance (LPCGUID lpcGUID, LPDIRECTMUSICPERFORMANCE *ppDMPerf, LPUNKNOWN pUnkOuter);

/*****************************************************************************
 * IDirectMusicPerformance8Impl implementation structure
 */
struct IDirectMusicPerformance8Impl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicPerformance8);
  DWORD                  ref;

  /* IDirectMusicPerformanceImpl fields */
  IDirectMusic*          dmusic;
  IDirectSound*          dsound;
  IDirectMusicGraph*     toolGraph;
  DMUS_AUDIOPARAMS       params;

  /* global parameters */
  BOOL  AutoDownload;
  char  MasterGrooveLevel;
  float MasterTempo;
  long  MasterVolume;
	
  /* performance channels */
  DMUSIC_PRIVATE_PCHANNEL PChannel[1];

   /* IDirectMusicPerformance8Impl fields */
  IDirectMusicAudioPath* default_path;
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
 * IDirectMusicGraphImpl implementation structure
 */
struct IDirectMusicGraphImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicGraph);
  DWORD          ref;

  /* IDirectMusicGraphImpl fields */
  IDirectMusicToolImpl* first;
  IDirectMusicToolImpl* last;
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
 * IDirectMusicStyleImpl implementation structure
 */
struct IDirectMusicStyleImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicStyle);
  DWORD          ref;

  /* IDirectMusicStyleImpl fields */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicStyleImpl_QueryInterface (LPDIRECTMUSICSTYLE iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicStyleImpl_AddRef (LPDIRECTMUSICSTYLE iface);
extern ULONG WINAPI   IDirectMusicStyleImpl_Release (LPDIRECTMUSICSTYLE iface);
/* IDirectMusicStyle: */
extern HRESULT WINAPI IDirectMusicStyleImpl_GetBand (LPDIRECTMUSICSTYLE iface, WCHAR* pwszName, IDirectMusicBand** ppBand);
extern HRESULT WINAPI IDirectMusicStyleImpl_EnumBand (LPDIRECTMUSICSTYLE iface, DWORD dwIndex, WCHAR* pwszName);
extern HRESULT WINAPI IDirectMusicStyleImpl_GetDefaultBand (LPDIRECTMUSICSTYLE iface, IDirectMusicBand** ppBand);
extern HRESULT WINAPI IDirectMusicStyleImpl_EnumMotif (LPDIRECTMUSICSTYLE iface, DWORD dwIndex, WCHAR* pwszName);
extern HRESULT WINAPI IDirectMusicStyleImpl_GetMotif (LPDIRECTMUSICSTYLE iface, WCHAR* pwszName, IDirectMusicSegment** ppSegment);
extern HRESULT WINAPI IDirectMusicStyleImpl_GetDefaultChordMap (LPDIRECTMUSICSTYLE iface, IDirectMusicChordMap** ppChordMap);
extern HRESULT WINAPI IDirectMusicStyleImpl_EnumChordMap (LPDIRECTMUSICSTYLE iface, DWORD dwIndex, WCHAR* pwszName);
extern HRESULT WINAPI IDirectMusicStyleImpl_GetChordMap (LPDIRECTMUSICSTYLE iface, WCHAR* pwszName, IDirectMusicChordMap** ppChordMap);
extern HRESULT WINAPI IDirectMusicStyleImpl_GetTimeSignature (LPDIRECTMUSICSTYLE iface, DMUS_TIMESIGNATURE* pTimeSig);
extern HRESULT WINAPI IDirectMusicStyleImpl_GetEmbellishmentLength (LPDIRECTMUSICSTYLE iface, DWORD dwType, DWORD dwLevel, DWORD* pdwMin, DWORD* pdwMax);
extern HRESULT WINAPI IDirectMusicStyleImpl_GetTempo (LPDIRECTMUSICSTYLE iface, double* pTempo);

/*****************************************************************************
 * IDirectMusicStyle8Impl implementation structure
 */
struct IDirectMusicStyle8Impl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicStyle8);
  DWORD          ref;

  /* IDirectMusicStyle8Impl fields */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicStyle8Impl_QueryInterface (LPDIRECTMUSICSTYLE8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicStyle8Impl_AddRef (LPDIRECTMUSICSTYLE8 iface);
extern ULONG WINAPI   IDirectMusicStyle8Impl_Release (LPDIRECTMUSICSTYLE8 iface);
/* IDirectMusicStyle: */
extern HRESULT WINAPI IDirectMusicStyle8Impl_GetBand (LPDIRECTMUSICSTYLE8 iface, WCHAR* pwszName, IDirectMusicBand** ppBand);
extern HRESULT WINAPI IDirectMusicStyle8Impl_EnumBand (LPDIRECTMUSICSTYLE8 iface, DWORD dwIndex, WCHAR* pwszName);
extern HRESULT WINAPI IDirectMusicStyle8Impl_GetDefaultBand (LPDIRECTMUSICSTYLE8 iface, IDirectMusicBand** ppBand);
extern HRESULT WINAPI IDirectMusicStyle8Impl_EnumMotif (LPDIRECTMUSICSTYLE8 iface, DWORD dwIndex, WCHAR* pwszName);
extern HRESULT WINAPI IDirectMusicStyle8Impl_GetMotif (LPDIRECTMUSICSTYLE8 iface, WCHAR* pwszName, IDirectMusicSegment** ppSegment);
extern HRESULT WINAPI IDirectMusicStyle8Impl_GetDefaultChordMap (LPDIRECTMUSICSTYLE8 iface, IDirectMusicChordMap** ppChordMap);
extern HRESULT WINAPI IDirectMusicStyle8Impl_EnumChordMap (LPDIRECTMUSICSTYLE8 iface, DWORD dwIndex, WCHAR* pwszName);
extern HRESULT WINAPI IDirectMusicStyle8Impl_GetChordMap (LPDIRECTMUSICSTYLE8 iface, WCHAR* pwszName, IDirectMusicChordMap** ppChordMap);
extern HRESULT WINAPI IDirectMusicStyle8Impl_GetTimeSignature (LPDIRECTMUSICSTYLE8 iface, DMUS_TIMESIGNATURE* pTimeSig);
extern HRESULT WINAPI IDirectMusicStyle8Impl_GetEmbellishmentLength (LPDIRECTMUSICSTYLE8 iface, DWORD dwType, DWORD dwLevel, DWORD* pdwMin, DWORD* pdwMax);
extern HRESULT WINAPI IDirectMusicStyle8Impl_GetTempo (LPDIRECTMUSICSTYLE8 iface, double* pTempo);
/* IDirectMusicStyle8: */
extern HRESULT WINAPI IDirectMusicStyle8ImplEnumPattern (LPDIRECTMUSICSTYLE8 iface, DWORD dwIndex, DWORD dwPatternType, WCHAR* pwszName);

/*****************************************************************************
 * IDirectMusicChordMapImpl implementation structure
 */
struct IDirectMusicChordMapImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicChordMap);
  DWORD          ref;

  /* IDirectMusicGetLoaderImpl fields */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicChordMapImpl_QueryInterface (LPDIRECTMUSICCHORDMAP iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicChordMapImpl_AddRef (LPDIRECTMUSICCHORDMAP iface);
extern ULONG WINAPI   IDirectMusicChordMapImpl_Release (LPDIRECTMUSICCHORDMAP iface);
/* IDirectMusicChordMap: */
extern HRESULT WINAPI IDirectMusicChordMapImpl_GetScale (LPDIRECTMUSICCHORDMAP iface, DWORD* pdwScale);

/*****************************************************************************
 * IDirectMusicComposerImpl implementation structure
 */
struct IDirectMusicComposerImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicComposer);
  DWORD          ref;

  /* IDirectMusicComposerImpl fields */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicComposerImpl_QueryInterface (LPDIRECTMUSICCOMPOSER iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicComposerImpl_AddRef (LPDIRECTMUSICCOMPOSER iface);
extern ULONG WINAPI   IDirectMusicComposerImpl_Release (LPDIRECTMUSICCOMPOSER iface);
/* IDirectMusicComposer: */
extern HRESULT WINAPI IDirectMusicComposerImpl_ComposeSegmentFromTemplate (LPDIRECTMUSICCOMPOSER iface, IDirectMusicStyle* pStyle, IDirectMusicSegment* pTemplate, WORD wActivity, IDirectMusicChordMap* pChordMap, IDirectMusicSegment** ppSegment);
extern HRESULT WINAPI IDirectMusicComposerImpl_ComposeSegmentFromShape (LPDIRECTMUSICCOMPOSER iface, IDirectMusicStyle* pStyle, WORD wNumMeasures, WORD wShape, WORD wActivity, BOOL fIntro, BOOL fEnd, IDirectMusicChordMap* pChordMap, IDirectMusicSegment** ppSegment);
extern HRESULT WINAPI IDirectMusicComposerImpl_ComposeTransition (LPDIRECTMUSICCOMPOSER iface, IDirectMusicSegment* pFromSeg, IDirectMusicSegment* pToSeg, MUSIC_TIME mtTime, WORD wCommand, DWORD dwFlags, IDirectMusicChordMap* pChordMap, IDirectMusicSegment** ppTransSeg);
extern HRESULT WINAPI IDirectMusicComposerImpl_AutoTransition (LPDIRECTMUSICCOMPOSER iface, IDirectMusicPerformance* pPerformance, IDirectMusicSegment* pToSeg, WORD wCommand, DWORD dwFlags, IDirectMusicChordMap* pChordMap, IDirectMusicSegment** ppTransSeg, IDirectMusicSegmentState** ppToSegState, IDirectMusicSegmentState** ppTransSegState);
extern HRESULT WINAPI IDirectMusicComposerImpl_ComposeTemplateFromShape (LPDIRECTMUSICCOMPOSER iface, WORD wNumMeasures, WORD wShape, BOOL fIntro, BOOL fEnd, WORD wEndLength, IDirectMusicSegment** ppTemplate);
extern HRESULT WINAPI IDirectMusicComposerImpl_ChangeChordMap (LPDIRECTMUSICCOMPOSER iface, IDirectMusicSegment* pSegment, BOOL fTrackScale, IDirectMusicChordMap* pChordMap);

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

/*****************************************************************************
 * IDirectMusicScriptImpl implementation structure
 */
struct IDirectMusicScriptImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicScript);
  DWORD          ref;

  /* IDirectMusicScriptImpl fields */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicScriptImpl_QueryInterface (LPDIRECTMUSICSCRIPT iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicScriptImpl_AddRef (LPDIRECTMUSICSCRIPT iface);
extern ULONG WINAPI   IDirectMusicScriptImpl_Release (LPDIRECTMUSICSCRIPT iface);
/* IDirectMusicScript: */
extern HRESULT WINAPI IDirectMusicScriptImpl_Init (LPDIRECTMUSICSCRIPT iface, IDirectMusicPerformance* pPerformance, DMUS_SCRIPT_ERRORINFO* pErrorInfo);
extern HRESULT WINAPI IDirectMusicScriptImpl_CallRoutine (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszRoutineName, DMUS_SCRIPT_ERRORINFO* pErrorInfo);
extern HRESULT WINAPI IDirectMusicScriptImpl_SetVariableVariant (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, VARIANT varValue, BOOL fSetRef, DMUS_SCRIPT_ERRORINFO* pErrorInfo);
extern HRESULT WINAPI IDirectMusicScriptImpl_GetVariableVariant (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, VARIANT* pvarValue, DMUS_SCRIPT_ERRORINFO* pErrorInfo);
extern HRESULT WINAPI IDirectMusicScriptImpl_SetVariableNumber (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, LONG lValue, DMUS_SCRIPT_ERRORINFO* pErrorInfo);
extern HRESULT WINAPI IDirectMusicScriptImpl_GetVariableNumber (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, LONG* plValue, DMUS_SCRIPT_ERRORINFO* pErrorInfo);
extern HRESULT WINAPI IDirectMusicScriptImpl_SetVariableObject (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, IUnknown* punkValue, DMUS_SCRIPT_ERRORINFO* pErrorInfo);
extern HRESULT WINAPI IDirectMusicScriptImpl_GetVariableObject (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, REFIID riid, LPVOID* ppv, DMUS_SCRIPT_ERRORINFO* pErrorInfo);
extern HRESULT WINAPI IDirectMusicScriptImpl_EnumRoutine (LPDIRECTMUSICSCRIPT iface, DWORD dwIndex, WCHAR* pwszName);
extern HRESULT WINAPI IDirectMusicScriptImpl_EnumVariable (LPDIRECTMUSICSCRIPT iface, DWORD dwIndex, WCHAR* pwszName);

/*****************************************************************************
 * IDirectMusicContainerImpl implementation structure
 */
struct IDirectMusicContainerImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicContainer);
  DWORD          ref;

  /* IDirectMusicContainerImpl fields */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicContainerImpl_QueryInterface (LPDIRECTMUSICCONTAINER iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicContainerImpl_AddRef (LPDIRECTMUSICCONTAINER iface);
extern ULONG WINAPI   IDirectMusicContainerImpl_Release (LPDIRECTMUSICCONTAINER iface);
/* IDirectMusicContainer: */
extern HRESULT WINAPI IDirectMusicContainerImpl_EnumObject (LPDIRECTMUSICCONTAINER iface, REFGUID rguidClass, DWORD dwIndex, LPDMUS_OBJECTDESC pDesc, WCHAR* pwszAlias);

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
 * Helper Functions
 */
void register_waveport (LPGUID lpGUID, LPCSTR lpszDesc, LPCSTR lpszDrvName, LPVOID lpContext);
/* Loader Helper Functions */
HRESULT WINAPI DMUSIC_FillSegmentFromFileHandle (IDirectMusicSegmentImpl *segment, HANDLE fd);
HRESULT WINAPI DMUSIC_FillTrackFromFileHandle (IDirectMusicTrackImpl *segment, HANDLE fd);
HRESULT WINAPI DMUSIC_FillReferenceFromFileHandle (Reference reference, HANDLE fd);
HRESULT WINAPI DMUSIC_FillUNFOFromFileHandle (UNFO_List UNFO, HANDLE fd);
HRESULT WINAPI DMUSIC_FillBandFromFileHandle (IDirectMusicBandImpl *band, HANDLE fd);

#endif	/* __WINE_DMUSIC_PRIVATE_H */
