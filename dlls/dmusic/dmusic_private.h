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

#include "config.h"
#include "windef.h"
#include "wine/debug.h"
#include "winbase.h"
#include "winnt.h"
#include "dmusicc.h"
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

/*****************************************************************************
 * IDirectMusicImpl implementation structure
 */
struct IDirectMusicImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusic);
  DWORD          ref;

  /* IDirectMusicImpl fields */
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

/*****************************************************************************
 * IDirectMusic8Impl implementation structure
 */
struct IDirectMusic8Impl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusic8);
  DWORD          ref;

  /* IDirectMusic8Impl fields */
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

#endif	/* __WINE_DMUSIC_PRIVATE_H */
