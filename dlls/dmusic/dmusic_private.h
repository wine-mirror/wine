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

#include <stdarg.h>

#include "windef.h"
#include "wine/debug.h"
#include "winbase.h"
#include "winnt.h"
#include "wingdi.h"
#include "dmusicc.h"
#include "dmusici.h"
#include "dmusics.h"
#include "dmplugin.h"
#include "dmusicf.h"
#include "dsound.h"

/*****************************************************************************
 * Interfaces
 */
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

typedef struct IDirectMusicCollectionObject IDirectMusicCollectionObject;
typedef struct IDirectMusicCollectionObjectStream IDirectMusicCollectionObjectStream;
	
/*****************************************************************************
 * Predeclare the interface implementation structures
 */
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

extern ICOM_VTABLE(IDirectMusicObject) DirectMusicCollectionObject_Vtbl;
extern ICOM_VTABLE(IPersistStream) DirectMusicCollectionObjectStream_Vtbl;

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

typedef struct _Part
{
	DMUS_IO_STYLEPART header;
	UNFO_List UNFO;
	DWORD nrofnotes;
	DMUS_IO_STYLENOTE* notes;
	DWORD nrofcurves;
	DMUS_IO_STYLECURVE* curves;
	DWORD nrofmarkers;
	DMUS_IO_STYLEMARKER* markers;
	DWORD nrofresolutions;
	DMUS_IO_STYLERESOLUTION* resolutions;
	DWORD nrofanticipations;
	DMUS_IO_STYLE_ANTICIPATION* anticipations;
} Part;

typedef struct _Pattern
{
	DMUS_IO_PATTERN header;
	DWORD nrofrhytms;
	DWORD* rhytms;
	UNFO_List UNFO;
	DMUS_IO_MOTIFSETTINGS motsettings;
	/* IDirectMusicBandImpl band */
	DWORD nrofpartrefs;
	/* FIXME: only in singular form for now */
	UNFO_List partrefUNFO;
	DMUS_IO_PARTREF partref;
} Pattern;

typedef struct _WaveTrack
{
	DMUS_IO_WAVE_TRACK_HEADER header;
	/* FIXME: only in singular form now */
	DMUS_IO_WAVE_PART_HEADER partHeader;
	DMUS_IO_WAVE_ITEM_HEADER itemHeader;
	Reference reference;
} WaveTrack;

typedef struct _SegTriggerTrack
{
	DMUS_IO_SEGMENT_TRACK_HEADER header;
	/* FIXME: only in singular form now */
	DMUS_IO_SEGMENT_ITEM_HEADER itemHeader;
	Reference reference;
	WCHAR* motifName;
} SegTriggerTrack;

typedef struct _TimeSigTrack {
	DWORD nrofitems;
	DMUS_IO_TIMESIGNATURE_ITEM* items;
} TimeSigTrack;

typedef struct _ScriptEvent {
	DMUS_IO_SCRIPTTRACK_EVENTHEADER header;
	Reference reference;
	WCHAR* name;
} ScriptEvent;

/*****************************************************************************
 * ClassFactory
 */
/* can support IID_IDirectMusic, IID_IDirectMusic2 and IID_IDirectMusic8
 * return always an IDirectMusic8Impl
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusic (LPCGUID lpcGUID, LPDIRECTMUSIC8* ppDM, LPUNKNOWN pUnkOuter);
/* can support IID_IDirectMusicBuffer
 * return always an IDirectMusicBufferImpl
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicBuffer (LPCGUID lpcGUID, LPDIRECTMUSICBUFFER* ppDMBuff, LPUNKNOWN pUnkOuter);
/* can support IID_IDirectMusicInstrument
 * return always an IDirectMusicInstrumentImpl
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicInstrument (LPCGUID lpcGUID, LPDIRECTMUSICINSTRUMENT* ppDMInstr, LPUNKNOWN pUnkOuter);
/* can support IID_IDirectMusicDownloadedInstrument
 * return always an IDirectMusicDownloadedInstrumentImpl
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicDownloadedInstrument (LPCGUID lpcGUID, LPDIRECTMUSICDOWNLOADEDINSTRUMENT* ppDMDLInstrument, LPUNKNOWN pUnkOuter);
/* can support IID_IDirectMusicCollection
 * return always an IDirectMusicCollectionImpl
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicCollection (LPCGUID lpcGUID, LPDIRECTMUSICCOLLECTION* ppDMColl, LPUNKNOWN pUnkOuter);
/* can support IID_IDirectMusicDownload
 * return always an IDirectMusicDownload
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicDownload (LPCGUID lpcGUID, LPDIRECTMUSICDOWNLOAD* ppDMDL, LPUNKNOWN pUnkOuter);
/* can support IID_IDirectMusicPortDownload
 * return always an IDirectMusicPortDownload
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicPortDownload (LPCGUID lpcGUID, LPDIRECTMUSICPORTDOWNLOAD* ppDMPortDL, LPUNKNOWN pUnkOuter);
/* can support IID_IDirectMusicPort
 * return always an IDirectMusicPortImpl
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicPort (LPCGUID lpcGUID, LPDIRECTMUSICPORT* ppDMPort, LPUNKNOWN pUnkOuter);
/* can support IID_IDirectMusicThru
 * return always an IDirectMusicThruImpl
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicThru (LPCGUID lpcGUID, LPDIRECTMUSICTHRU* ppDMThru, LPUNKNOWN pUnkOuter);
/* can support IID_IDirectMusicObject
 * return always an IDirectMusicObjectImpl
 */

extern HRESULT WINAPI DMUSIC_CreateReferenceClock (LPCGUID lpcGUID, IReferenceClock** ppDM, LPUNKNOWN pUnkOuter);

extern HRESULT WINAPI DMUSIC_CreateDirectMusicCollectionObject (LPCGUID lpcGUID, LPDIRECTMUSICOBJECT* ppObject, LPUNKNOWN pUnkOuter);

/*****************************************************************************
 * IDirectMusic8Impl implementation structure
 */
struct IDirectMusic8Impl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusic8);
  DWORD          ref;

  /* IDirectMusicImpl fields */
  IReferenceClockImpl* pMasterClock;
  IDirectMusicPortImpl** ppPorts;
  int nrofports;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusic8Impl_QueryInterface (LPDIRECTMUSIC8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusic8Impl_AddRef (LPDIRECTMUSIC8 iface);
extern ULONG WINAPI   IDirectMusic8Impl_Release (LPDIRECTMUSIC8 iface);
/* IDirectMusic: */
extern HRESULT WINAPI IDirectMusic8Impl_EnumPort (LPDIRECTMUSIC8 iface, DWORD dwIndex, LPDMUS_PORTCAPS pPortCaps);
extern HRESULT WINAPI IDirectMusic8Impl_CreateMusicBuffer (LPDIRECTMUSIC8 iface, LPDMUS_BUFFERDESC pBufferDesc, LPDIRECTMUSICBUFFER** ppBuffer, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI IDirectMusic8Impl_CreatePort (LPDIRECTMUSIC8 iface, REFCLSID rclsidPort, LPDMUS_PORTPARAMS pPortParams, LPDIRECTMUSICPORT* ppPort, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI IDirectMusic8Impl_EnumMasterClock (LPDIRECTMUSIC8 iface, DWORD dwIndex, LPDMUS_CLOCKINFO lpClockInfo);
extern HRESULT WINAPI IDirectMusic8Impl_GetMasterClock (LPDIRECTMUSIC8 iface, LPGUID pguidClock, IReferenceClock** ppReferenceClock);
extern HRESULT WINAPI IDirectMusic8Impl_SetMasterClock (LPDIRECTMUSIC8 iface, REFGUID rguidClock);
extern HRESULT WINAPI IDirectMusic8Impl_Activate (LPDIRECTMUSIC8 iface, BOOL fEnable);
extern HRESULT WINAPI IDirectMusic8Impl_GetDefaultPort (LPDIRECTMUSIC8 iface, LPGUID pguidPort);
extern HRESULT WINAPI IDirectMusic8Impl_SetDirectSound (LPDIRECTMUSIC8 iface, LPDIRECTSOUND pDirectSound, HWND hWnd);
/* IDirectMusic8: */
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
  DWORD dwPatch;
  LPWSTR pwszName;
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
  IDirectMusicCollectionObject* pObject;
  DWORD nrofinstruments;
  IDirectMusicInstrumentImpl** ppInstruments;
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
  IDirectSound* pDirectSound;
  IReferenceClock* pLatencyClock;
  BOOL fActive;
  LPDMUS_PORTCAPS pCaps;
  LPDMUS_PORTPARAMS pParams;
  int nrofgroups;
  DMUSIC_PRIVATE_CHANNEL_GROUP group[1];
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicPortImpl_QueryInterface (LPDIRECTMUSICPORT iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicPortImpl_AddRef (LPDIRECTMUSICPORT iface);
extern ULONG WINAPI   IDirectMusicPortImpl_Release (LPDIRECTMUSICPORT iface);
/* IDirectMusicPortImpl: */
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
  REFERENCE_TIME rtTime;
  DMUS_CLOCKINFO pClockInfo;
};

/* IUnknown: */
extern HRESULT WINAPI IReferenceClockImpl_QueryInterface (IReferenceClock *iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IReferenceClockImpl_AddRef (IReferenceClock *iface);
extern ULONG WINAPI   IReferenceClockImpl_Release (IReferenceClock *iface);
/* IReferenceClock: */
extern HRESULT WINAPI IReferenceClockImpl_GetTime (IReferenceClock *iface, REFERENCE_TIME* pTime);
extern HRESULT WINAPI IReferenceClockImpl_AdviseTime (IReferenceClock *iface, REFERENCE_TIME baseTime, REFERENCE_TIME streamTime, HANDLE hEvent, DWORD* pdwAdviseCookie);
extern HRESULT WINAPI IReferenceClockImpl_AdvisePeriodic (IReferenceClock *iface, REFERENCE_TIME startTime, REFERENCE_TIME periodTime, HANDLE hSemaphore, DWORD* pdwAdviseCookie);
extern HRESULT WINAPI IReferenceClockImpl_Unadvise (IReferenceClock *iface, DWORD dwAdviseCookie);


/*****************************************************************************
 * IDirectMusicCollectionObject implementation structure
 */
struct IDirectMusicCollectionObject
{
  /* IUnknown fields */
  ICOM_VFIELD (IDirectMusicObject);
  DWORD          ref;

  /* IDirectMusicObjectImpl fields */
  LPDMUS_OBJECTDESC pDesc;
  IDirectMusicCollectionObjectStream* pStream;
  IDirectMusicCollectionImpl* pCollection;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicCollectionObject_QueryInterface (LPDIRECTMUSICOBJECT iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicCollectionObject_AddRef (LPDIRECTMUSICOBJECT iface);
extern ULONG WINAPI   IDirectMusicCollectionObject_Release (LPDIRECTMUSICOBJECT iface);
/* IDirectMusicObject: */
extern HRESULT WINAPI IDirectMusicCollectionObject_GetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicCollectionObject_SetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicCollectionObject_ParseDescriptor (LPDIRECTMUSICOBJECT iface, LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc);

/*****************************************************************************
 * IDirectMusicCollectionObjectStream implementation structure
 */
struct IDirectMusicCollectionObjectStream
{
  /* IUnknown fields */
  ICOM_VFIELD (IPersistStream);
  DWORD          ref;

  /* IPersistStreamImpl fields */
  IDirectMusicCollectionObject* pParentObject;
};

/* IUnknown: */
extern HRESULT WINAPI  IDirectMusicCollectionObjectStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI IDirectMusicCollectionObjectStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI IDirectMusicCollectionObjectStream_Release (LPPERSISTSTREAM iface);
/* IPersist: */
extern HRESULT WINAPI IDirectMusicCollectionObjectStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicCollectionObjectStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicCollectionObjectStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicCollectionObjectStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicCollectionObjectStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);


/*****************************************************************************
 * Helper Functions
 */
void register_waveport (LPGUID lpGUID, LPCSTR lpszDesc, LPCSTR lpszDrvName, LPVOID lpContext);

#endif	/* __WINE_DMUSIC_PRIVATE_H */
