/* DirectMusic Private Include
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

#ifndef __WINE_DMUSIC_PRIVATE_H
#define __WINE_DMUSIC_PRIVATE_H

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
typedef struct IDirectMusic8Impl IDirectMusic8Impl;
typedef struct IDirectMusicBufferImpl IDirectMusicBufferImpl;
typedef struct IDirectMusicDownloadedInstrumentImpl IDirectMusicDownloadedInstrumentImpl;
typedef struct IDirectMusicDownloadImpl IDirectMusicDownloadImpl;
typedef struct IDirectMusicPortDownloadImpl IDirectMusicPortDownloadImpl;
typedef struct IDirectMusicPortImpl IDirectMusicPortImpl;
typedef struct IDirectMusicThruImpl IDirectMusicThruImpl;
typedef struct IReferenceClockImpl IReferenceClockImpl;

typedef struct IDirectMusicCollectionImpl IDirectMusicCollectionImpl;
typedef struct IDirectMusicInstrumentImpl IDirectMusicInstrumentImpl;

	
/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern ICOM_VTABLE(IDirectMusic8) DirectMusic8_Vtbl;
extern ICOM_VTABLE(IDirectMusicBuffer) DirectMusicBuffer_Vtbl;
extern ICOM_VTABLE(IDirectMusicDownloadedInstrument) DirectMusicDownloadedInstrument_Vtbl;
extern ICOM_VTABLE(IDirectMusicDownload) DirectMusicDownload_Vtbl;
extern ICOM_VTABLE(IDirectMusicPortDownload) DirectMusicPortDownload_Vtbl;
extern ICOM_VTABLE(IDirectMusicPort) DirectMusicPort_Vtbl;
extern ICOM_VTABLE(IDirectMusicThru) DirectMusicThru_Vtbl;
extern ICOM_VTABLE(IReferenceClock) ReferenceClock_Vtbl;

extern ICOM_VTABLE(IUnknown)               DirectMusicCollection_Unknown_Vtbl;
extern ICOM_VTABLE(IDirectMusicCollection) DirectMusicCollection_Collection_Vtbl;
extern ICOM_VTABLE(IDirectMusicObject)     DirectMusicCollection_Object_Vtbl;
extern ICOM_VTABLE(IPersistStream)         DirectMusicCollection_PersistStream_Vtbl;

extern ICOM_VTABLE(IUnknown)               DirectMusicInstrument_Unknown_Vtbl;
extern ICOM_VTABLE(IDirectMusicInstrument) DirectMusicInstrument_Instrument_Vtbl;

/*****************************************************************************
 * Some stuff to make my life easier :=)
 */
 
/* some sort of aux. midi channel: big fake at the moment; accepts only priority
   changes... more coming soon */
typedef struct DMUSIC_PRIVATE_MCHANNEL_ {
	DWORD priority;
} DMUSIC_PRIVATE_MCHANNEL, *LPDMUSIC_PRIVATE_MCHANNEL;

/* some sort of aux. channel group: collection of 16 midi channels */
typedef struct DMUSIC_PRIVATE_CHANNEL_GROUP_ {
	DMUSIC_PRIVATE_MCHANNEL channel[16]; /* 16 channels in a group */
} DMUSIC_PRIVATE_CHANNEL_GROUP, *LPDMUSIC_PRIVATE_CHANNEL_GROUP;


/*****************************************************************************
 * ClassFactory
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicImpl (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicBufferImpl (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicDownloadedInstrumentImpl (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicDownloadImpl (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateReferenceClockImpl (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter);

extern HRESULT WINAPI DMUSIC_CreateDirectMusicCollectionImpl (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicInstrumentImpl (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter);

/*****************************************************************************
 * IDirectMusic8Impl implementation structure
 */
struct IDirectMusic8Impl {
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
struct IDirectMusicBufferImpl {
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
 * IDirectMusicDownloadedInstrumentImpl implementation structure
 */
struct IDirectMusicDownloadedInstrumentImpl {
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
 * IDirectMusicDownloadImpl implementation structure
 */
struct IDirectMusicDownloadImpl {
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
struct IDirectMusicPortDownloadImpl {
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
struct IDirectMusicPortImpl {
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
extern HRESULT WINAPI IDirectMusicPortImpl_Compact (LPDIRECTMUSICPORT iface);
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
struct IDirectMusicThruImpl {
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
struct IReferenceClockImpl {
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


typedef struct _DMUS_PRIVATE_INSTRUMENT_ENTRY {
	struct list entry; /* for listing elements */
	IDirectMusicInstrument* pInstrument;
} DMUS_PRIVATE_INSTRUMENTENTRY, *LPDMUS_PRIVATE_INSTRUMENTENTRY;

typedef struct _DMUS_PRIVATE_POOLCUE {
	struct list entry; /* for listing elements */
} DMUS_PRIVATE_POOLCUE, *LPDMUS_PRIVATE_POOLCUE;

/*****************************************************************************
 * IDirectMusicCollectionImpl implementation structure
 */
struct IDirectMusicCollectionImpl {
  /* IUnknown fields */
  ICOM_VTABLE(IUnknown) *UnknownVtbl;
  ICOM_VTABLE(IDirectMusicCollection) *CollectionVtbl;
  ICOM_VTABLE(IDirectMusicObject) *ObjectVtbl;
  ICOM_VTABLE(IPersistStream) *PersistStreamVtbl;
  DWORD          ref;

  /* IDirectMusicCollectionImpl fields */
  IStream *pStm; /* stream from which we load collection and later instruments */
  LARGE_INTEGER liCollectionPosition; /* offset in a stream where collection was loaded from */
  LARGE_INTEGER liWavePoolTablePosition; /* offset in a stream where wave pool table can be found */
  LPDMUS_OBJECTDESC pDesc;
  CHAR* szCopyright; /* FIXME: should probably placed somewhere else */
  LPDLSHEADER pHeader;
  /* pool table */
  LPPOOLTABLE pPoolTable;
  LPPOOLCUE pPoolCues;
  /* instruments */
  struct list Instruments;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicCollectionImpl_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicCollectionImpl_IUnknown_AddRef (LPUNKNOWN iface);
extern ULONG WINAPI   IDirectMusicCollectionImpl_IUnknown_Release (LPUNKNOWN iface);
/* IDirectMusicCollection: */
extern HRESULT WINAPI IDirectMusicCollectionImpl_IDirectMusicCollection_QueryInterface (LPDIRECTMUSICCOLLECTION iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicCollectionImpl_IDirectMusicCollection_AddRef (LPDIRECTMUSICCOLLECTION iface);
extern ULONG WINAPI   IDirectMusicCollectionImpl_IDirectMusicCollection_Release (LPDIRECTMUSICCOLLECTION iface);
extern HRESULT WINAPI IDirectMusicCollectionImpl_IDirectMusicCollection_GetInstrument (LPDIRECTMUSICCOLLECTION iface, DWORD dwPatch, IDirectMusicInstrument** ppInstrument);
extern HRESULT WINAPI IDirectMusicCollectionImpl_IDirectMusicCollection_EnumInstrument (LPDIRECTMUSICCOLLECTION iface, DWORD dwIndex, DWORD* pdwPatch, LPWSTR pwszName, DWORD dwNameLen);
/* IDirectMusicObject: */
extern HRESULT WINAPI IDirectMusicCollectionImpl_IDirectMusicObject_QueryInterface (LPDIRECTMUSICOBJECT iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicCollectionImpl_IDirectMusicObject_AddRef (LPDIRECTMUSICOBJECT iface);
extern ULONG WINAPI   IDirectMusicCollectionImpl_IDirectMusicObject_Release (LPDIRECTMUSICOBJECT iface);
extern HRESULT WINAPI IDirectMusicCollectionImpl_IDirectMusicObject_GetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicCollectionImpl_IDirectMusicObject_SetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicCollectionImpl_IDirectMusicObject_ParseDescriptor (LPDIRECTMUSICOBJECT iface, LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicCollectionImpl_IPersistStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI   IDirectMusicCollectionImpl_IPersistStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI   IDirectMusicCollectionImpl_IPersistStream_Release (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicCollectionImpl_IPersistStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
extern HRESULT WINAPI IDirectMusicCollectionImpl_IPersistStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicCollectionImpl_IPersistStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicCollectionImpl_IPersistStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicCollectionImpl_IPersistStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);


/*****************************************************************************
 * IDirectMusicInstrumentImpl implementation structure
 */
struct IDirectMusicInstrumentImpl {
  /* IUnknown fields */
  ICOM_VTABLE(IUnknown) *UnknownVtbl;
  ICOM_VTABLE(IDirectMusicInstrument) *InstrumentVtbl;
  DWORD          ref;

  /* IDirectMusicInstrumentImpl fields */
  LARGE_INTEGER liInstrumentPosition; /* offset in a stream where instrument chunk can be found */
  LPGUID pInstrumentID;
  LPINSTHEADER pHeader;
  WCHAR wszName[DMUS_MAX_NAME];
  /* instrument data */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicInstrumentImpl_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicInstrumentImpl_IUnknown_AddRef (LPUNKNOWN iface);
extern ULONG WINAPI   IDirectMusicInstrumentImpl_IUnknown_Release (LPUNKNOWN iface);
/* IDirectMusicInstrumentImpl: */
extern HRESULT WINAPI IDirectMusicInstrumentImpl_IDirectMusicInstrument_QueryInterface (LPDIRECTMUSICINSTRUMENT iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicInstrumentImpl_IDirectMusicInstrument_AddRef (LPDIRECTMUSICINSTRUMENT iface);
extern ULONG WINAPI   IDirectMusicInstrumentImpl_IDirectMusicInstrument_Release (LPDIRECTMUSICINSTRUMENT iface);
extern HRESULT WINAPI IDirectMusicInstrumentImpl_IDirectMusicInstrument_GetPatch (LPDIRECTMUSICINSTRUMENT iface, DWORD* pdwPatch);
extern HRESULT WINAPI IDirectMusicInstrumentImpl_IDirectMusicInstrument_SetPatch (LPDIRECTMUSICINSTRUMENT iface, DWORD dwPatch);
/* custom :) */
extern HRESULT WINAPI IDirectMusicInstrumentImpl_Custom_Load (LPDIRECTMUSICINSTRUMENT iface, LPSTREAM pStm);


/*****************************************************************************
 * Helper Functions
 */
void register_waveport (LPGUID lpGUID, LPCSTR lpszDesc, LPCSTR lpszDrvName, LPVOID lpContext);


/*****************************************************************************
 * Misc.
 */
/* my custom ICOM stuff */
#define ICOM_NAME(impl,iface,name)    impl* const name=(impl*)(iface)
#define ICOM_NAME_MULTI(impl,field,iface,name)  impl* const name=(impl*)((char*)(iface) - offsetof(impl,field))
 
/* for simpler reading */
typedef struct _DMUS_PRIVATE_CHUNK {
	FOURCC fccID; /* FOURCC ID of the chunk */
	DWORD dwSize; /* size of the chunk */
} DMUS_PRIVATE_CHUNK, *LPDMUS_PRIVATE_CHUNK;

/* check whether the given DWORD is even (return 0) or odd (return 1) */
static inline int even_or_odd (DWORD number) {
	return (number & 0x1); /* basically, check if bit 0 is set ;) */
}

/* FOURCC to string conversion for debug messages */
static inline const char *debugstr_fourcc (DWORD fourcc) {
    if (!fourcc) return "'null'";
    return wine_dbg_sprintf ("\'%c%c%c%c\'",
		(char)(fourcc), (char)(fourcc >> 8),
        (char)(fourcc >> 16), (char)(fourcc >> 24));
}

/* DMUS_VERSION struct to string conversion for debug messages */
static inline const char *debugstr_dmversion (LPDMUS_VERSION version) {
	if (!version) return "'null'";
	return wine_dbg_sprintf ("\'%i,%i,%i,%i\'",
		(int)((version->dwVersionMS && 0xFFFF0000) >> 8), (int)(version->dwVersionMS && 0x0000FFFF), 
		(int)((version->dwVersionLS && 0xFFFF0000) >> 8), (int)(version->dwVersionLS && 0x0000FFFF));
}

/* dwPatch from MIDILOCALE */
static inline DWORD MIDILOCALE2Patch (LPMIDILOCALE pLocale) {
	DWORD dwPatch = 0;
	if (!pLocale) return 0;
	dwPatch |= (pLocale->ulBank & F_INSTRUMENT_DRUMS); /* set drum bit */
	dwPatch |= ((pLocale->ulBank & 0x00007F7F) << 8); /* set MIDI bank location */
	dwPatch |= (pLocale->ulInstrument & 0x0000007F); /* set PC value */
	return dwPatch;	
}

/* MIDILOCALE from dwPatch */
static inline void Patch2MIDILOCALE (DWORD dwPatch, LPMIDILOCALE pLocale) {
	memset (pLocale, 0, sizeof(MIDILOCALE));
	
	pLocale->ulInstrument = (dwPatch & 0x7F); /* get PC value */
	pLocale->ulBank = ((dwPatch & 0x007F7F00) >> 8); /* get MIDI bank location */
	pLocale->ulBank |= (dwPatch & F_INSTRUMENT_DRUMS); /* get drum bit */
}

/* used for initialising structs (primarily for DMUS_OBJECTDESC) */
#define DM_STRUCT_INIT(x) 				\
	do {								\
		memset((x), 0, sizeof(*(x)));	\
		(x)->dwSize = sizeof(*x);		\
	} while (0)


/* used for generic dumping (copied from ddraw) */
typedef struct {
    DWORD val;
    const char* name;
} flag_info;

#define FE(x) { x, #x }
#define DMUSIC_dump_flags(flags,names,num_names) DMUSIC_dump_flags_(flags, names, num_names, 1)

/* generic dump function */
static inline void DMUSIC_dump_flags_ (DWORD flags, const flag_info* names, size_t num_names, int newline) {
	unsigned int i;
	
	for (i=0; i < num_names; i++) {
		if ((flags & names[i].val) ||      /* standard flag value */
		((!flags) && (!names[i].val))) /* zero value only */
	    	DPRINTF("%s ", names[i].name);
	}
	
    if (newline) DPRINTF("\n");
}

static inline void DMUSIC_dump_DMUS_OBJ_FLAGS (DWORD flagmask) {
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
    DMUSIC_dump_flags(flagmask, flags, sizeof(flags)/sizeof(flags[0]));
}

static inline void DMUSIC_dump_DMUS_OBJECTDESC (LPDMUS_OBJECTDESC pDesc) {
	if (pDesc) {
		DPRINTF("DMUS_OBJECTDESC (%p)\n", pDesc);
		DPRINTF("  - dwSize = %ld\n", pDesc->dwSize);
		DPRINTF("  - dwValidData = ");
		DMUSIC_dump_DMUS_OBJ_FLAGS (pDesc->dwValidData);
		if (pDesc->dwValidData & DMUS_OBJ_CLASS) DPRINTF("  - guidClass = %s\n", debugstr_guid(&pDesc->guidClass));
		if (pDesc->dwValidData & DMUS_OBJ_OBJECT) DPRINTF("  - guidObject = %s\n", debugstr_guid(&pDesc->guidObject));
		if (pDesc->dwValidData & DMUS_OBJ_DATE) DPRINTF("  - ftDate = FIXME\n");
		if (pDesc->dwValidData & DMUS_OBJ_VERSION) DPRINTF("  - vVersion = %s\n", debugstr_dmversion(&pDesc->vVersion));
		if (pDesc->dwValidData & DMUS_OBJ_NAME) DPRINTF("  - wszName = %s\n", debugstr_w(pDesc->wszName));
		if (pDesc->dwValidData & DMUS_OBJ_CATEGORY) DPRINTF("  - wszCategory = %s\n", debugstr_w(pDesc->wszCategory));
		if (pDesc->dwValidData & DMUS_OBJ_FILENAME) DPRINTF("  - wszFileName = %s\n", debugstr_w(pDesc->wszFileName));
		if (pDesc->dwValidData & DMUS_OBJ_MEMORY) DPRINTF("  - llMemLength = %lli\n  - pbMemData = %p\n", pDesc->llMemLength, pDesc->pbMemData);
		if (pDesc->dwValidData & DMUS_OBJ_STREAM) DPRINTF("  - pStream = %p\n", pDesc->pStream);		
	} else {
		DPRINTF("(NULL)\n");
	}
}

#endif	/* __WINE_DMUSIC_PRIVATE_H */
