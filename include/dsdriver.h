/*
 * DirectSound driver
 * (DirectX 5 version)
 *
 * Copyright (C) 2000 Ove Kaaven
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_DSDRIVER_H
#define __WINE_DSDRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_GUID(IID_IDsDriver,		0x8C4233C0l, 0xB4CC, 0x11CE, 0x92, 0x94, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);
typedef struct IDsDriver IDsDriver,*PIDSDRIVER;

DEFINE_GUID(IID_IDsDriverBuffer,	0x8C4233C1l, 0xB4CC, 0x11CE, 0x92, 0x94, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);
typedef struct IDsDriverBuffer IDsDriverBuffer,*PIDSDRIVERBUFFER;

DEFINE_GUID(IID_IDsDriverPropertySet,	0x0F6F2E8E0, 0xD842, 0x11D0, 0x8F, 0x75, 0x00, 0xC0, 0x4F, 0xC2, 0x8A, 0xCA);
typedef struct IDsDriverPropertySet IDsDriverPropertySet,*PIDSDRIVERPROPERTYSET;

DEFINE_GUID(IID_IDsDriverNotify,	0x00363EF44, 0x3B57, 0x11D3, 0xAC, 0x79, 0x00, 0x10, 0x5A, 0x01, 0x7f, 0xe1);
typedef struct IDsDriverNotify IDsDriverNotify,*PIDSDRIVERNOTIFY;

DEFINE_GUID(IID_IDsCaptureDriver,	0x03DD10C47, 0x74FB, 0x11D3, 0x90, 0x49, 0xCB, 0xB4, 0xB3, 0x2E, 0xAA, 0x08);
typedef struct IDsCaptureDriver IDsCaptureDriver,*PIDSCDRIVER;

DEFINE_GUID(IID_IDsCaptureDriverBuffer,	0x03DD10C48, 0x74FB, 0x11D3, 0x90, 0x49, 0xCB, 0xB4, 0xB3, 0x2E, 0xAA, 0x08);
typedef struct IDsCaptureDriverBuffer IDsCaptureDriverBuffer,*PIDSCDRIVERBUFFER;

#define DSDDESC_DOMMSYSTEMOPEN		0x00000001
#define DSDDESC_DOMMSYSTEMSETFORMAT	0x00000002
#define DSDDESC_USESYSTEMMEMORY		0x00000004
#define DSDDESC_DONTNEEDPRIMARYLOCK	0x00000008
#define DSDDESC_DONTNEEDSECONDARYLOCK	0x00000010

#define DSDHEAP_NOHEAP			0
#define DSDHEAP_CREATEHEAP		1
#define DSDHEAP_USEDIRECTDRAWHEAP	2
#define DSDHEAP_PRIVATEHEAP		3

typedef struct _DSDRIVERDESC
{
    DWORD      	dwFlags;
    CHAR	szDesc[256];
    CHAR	szDrvName[256];
    DWORD	dnDevNode;
    WORD	wVxdId;
    WORD	wReserved;
    ULONG	ulDeviceNum;
    DWORD	dwHeapType;
    LPVOID	pvDirectDrawHeap;
    DWORD	dwMemStartAddress;
    DWORD	dwMemEndAddress;
    DWORD	dwMemAllocExtra;
    LPVOID	pvReserved1;
    LPVOID	pvReserved2;
} DSDRIVERDESC,*PDSDRIVERDESC;

typedef struct _DSDRIVERCAPS
{
    DWORD	dwFlags;
    DWORD	dwMinSecondarySampleRate;
    DWORD	dwMaxSecondarySampleRate;
    DWORD	dwPrimaryBuffers;
    DWORD	dwMaxHwMixingAllBuffers;
    DWORD	dwMaxHwMixingStaticBuffers;
    DWORD	dwMaxHwMixingStreamingBuffers;
    DWORD	dwFreeHwMixingAllBuffers;
    DWORD	dwFreeHwMixingStaticBuffers;
    DWORD	dwFreeHwMixingStreamingBuffers;
    DWORD	dwMaxHw3DAllBuffers;
    DWORD	dwMaxHw3DStaticBuffers;
    DWORD	dwMaxHw3DStreamingBuffers;
    DWORD	dwFreeHw3DAllBuffers;
    DWORD	dwFreeHw3DStaticBuffers;
    DWORD	dwFreeHw3DStreamingBuffers;
    DWORD	dwTotalHwMemBytes;
    DWORD	dwFreeHwMemBytes;
    DWORD	dwMaxContigFreeHwMemBytes;
} DSDRIVERCAPS,*PDSDRIVERCAPS;

typedef struct _DSVOLUMEPAN
{
    DWORD	dwTotalLeftAmpFactor;
    DWORD	dwTotalRightAmpFactor;
    LONG	lVolume;
    DWORD	dwVolAmpFactor;
    LONG	lPan;
    DWORD	dwPanLeftAmpFactor;
    DWORD	dwPanRightAmpFactor;
} DSVOLUMEPAN,*PDSVOLUMEPAN;

typedef union _DSPROPERTY
{
    struct {
	GUID	Set;
	ULONG	Id;
	ULONG	Flags;
	ULONG	InstanceId;
    } DUMMYSTRUCTNAME;
    ULONGLONG	Alignment;
} DSPROPERTY,*PDSPROPERTY;

typedef struct _DSCDRIVERCAPS
{
    DWORD	dwSize;
    DWORD	dwFlags;
    DWORD	dwFormats;
    DWORD	dwChannels;
} DSCDRIVERCAPS,*PDSCDRIVERCAPS;

/*****************************************************************************
 * IDsDriver interface
 */
#define INTERFACE IDsDriver
#define IDsDriver_METHODS \
    STDMETHOD(GetDriverDesc)(THIS_ PDSDRIVERDESC pDsDriverDesc) PURE; \
    STDMETHOD(Open)(THIS) PURE; \
    STDMETHOD(Close)(THIS) PURE; \
    STDMETHOD(GetCaps)(THIS_ PDSDRIVERCAPS pDsDrvCaps) PURE; \
    STDMETHOD(CreateSoundBuffer)(THIS_ LPWAVEFORMATEX pwfx,DWORD dwFlags,DWORD dwCardAddress,LPDWORD pdwcbBufferSize,LPBYTE *ppbBuffer,LPVOID *ppvObj) PURE; \
    STDMETHOD(DuplicateSoundBuffer)(THIS_ PIDSDRIVERBUFFER pIDsDriverBuffer,LPVOID *ppvObj) PURE;
#define IDsDriver_IMETHODS \
    IUnknown_METHODS \
    IDsDriver_METHODS
ICOM_DEFINE(IDsDriver,IUnknown)
#undef INTERFACE

    /*** IUnknown methods ***/
#define IDsDriver_QueryInterface(p,a,b)		ICOM_CALL2(QueryInterface,p,a,b)
#define IDsDriver_AddRef(p)			ICOM_CALL (AddRef,p)
#define IDsDriver_Release(p)			ICOM_CALL (Release,p)
    /*** IDsDriver methods ***/
#define IDsDriver_GetDriverDesc(p,a)		ICOM_CALL1(GetDriverDesc,p,a)
#define IDsDriver_Open(p)			ICOM_CALL (Open,p)
#define IDsDriver_Close(p)			ICOM_CALL (Close,p)
#define IDsDriver_GetCaps(p,a)			ICOM_CALL1(GetCaps,p,a)
#define IDsDriver_CreateSoundBuffer(p,a,b,c,d,e,f) ICOM_CALL6(CreateSoundBuffer,p,a,b,c,d,e,f)
#define IDsDriver_DuplicateSoundBuffer(p,a,b)	ICOM_CALL2(DuplicateSoundBuffer,p,a,b)

/*****************************************************************************
 * IDsDriverBuffer interface
 */
#define INTERFACE IDsDriverBuffer
#define IDsDriverBuffer_METHODS \
    STDMETHOD(Lock)(THIS_ LPVOID *ppvAudio1,LPDWORD pdwLen1,LPVOID *pdwAudio2,LPDWORD pdwLen2,DWORD dwWritePosition,DWORD dwWriteLen,DWORD dwFlags) PURE; \
    STDMETHOD(Unlock)(THIS_ LPVOID pvAudio1,DWORD dwLen1,LPVOID pvAudio2,DWORD dwLen2) PURE; \
    STDMETHOD(SetFormat)(THIS_ LPWAVEFORMATEX pwfxToSet) PURE; \
    STDMETHOD(SetFrequency)(THIS_ DWORD dwFrequency) PURE; \
    STDMETHOD(SetVolumePan)(THIS_ PDSVOLUMEPAN pDsVolumePan) PURE; \
    STDMETHOD(SetPosition)(THIS_ DWORD dwNewPosition) PURE; \
    STDMETHOD(GetPosition)(THIS_ LPDWORD lpdwCurrentPlayCursor,LPDWORD lpdwCurrentWriteCursor) PURE; \
    STDMETHOD(Play)(THIS_ DWORD dwReserved1,DWORD dwReserved2,DWORD dwFlags) PURE; \
    STDMETHOD(Stop)(THIS) PURE;
#define IDsDriverBuffer_IMETHODS \
    IUnknown_METHODS \
    IDsDriverBuffer_METHODS
ICOM_DEFINE(IDsDriverBuffer,IUnknown)
#undef INTERFACE

    /*** IUnknown methods ***/
#define IDsDriverBuffer_QueryInterface(p,a,b)	ICOM_CALL2(QueryInterface,p,a,b)
#define IDsDriverBuffer_AddRef(p)		ICOM_CALL (AddRef,p)
#define IDsDriverBuffer_Release(p)		ICOM_CALL (Release,p)
    /*** IDsDriverBuffer methods ***/
#define IDsDriverBuffer_Lock(p,a,b,c,d,e,f,g)	ICOM_CALL7(Lock,p,a,b,c,d,e,f,g)
#define IDsDriverBuffer_Unlock(p,a,b,c,d)	ICOM_CALL4(Unlock,p,a,b,c,d)
#define IDsDriverBuffer_SetFormat(p,a)		ICOM_CALL1(SetFormat,p,a)
#define IDsDriverBuffer_SetFrequency(p,a)	ICOM_CALL1(SetFrequency,p,a)
#define IDsDriverBuffer_SetVolumePan(p,a)	ICOM_CALL1(SetVolumePan,p,a)
#define IDsDriverBuffer_SetPosition(p,a)	ICOM_CALL1(SetPosition,p,a)
#define IDsDriverBuffer_GetPosition(p,a,b)	ICOM_CALL2(GetPosition,p,a,b)
#define IDsDriverBuffer_Play(p,a,b,c)		ICOM_CALL3(Play,p,a,b,c)
#define IDsDriverBuffer_Stop(p)			ICOM_CALL (Stop,p)

/*****************************************************************************
 * IDsDriverPropertySet interface
 */
#define INTERFACE IDsDriverPropertySet
#define IDsDriverPropertySet_METHODS \
    STDMETHOD(Get)(THIS_ PDSPROPERTY pDsProperty,LPVOID pPropertyParams,ULONG cbPropertyParams,LPVOID pPropertyData,ULONG cbPropertyData,PULONG pcbReturnedData) PURE; \
    STDMETHOD(Set)(THIS_ PDSPROPERTY pDsProperty,LPVOID pPropertyParams,ULONG cbPropertyParams,LPVOID pPropertyData,ULONG cbPropertyData) PURE; \
    STDMETHOD(QuerySupport)(THIS_ REFGUID PropertySetId,ULONG PropertyId,PULONG pSupport) PURE;
#define IDsDriverPropertySet_IMETHODS \
    IUnknown_METHODS \
    IDsDriverPropertySet_METHODS
ICOM_DEFINE(IDsDriverPropertySet,IUnknown)
#undef INTERFACE

    /*** IUnknown methods ***/
#define IDsDriverPropertySet_QueryInterface(p,a,b)	ICOM_CALL2(QueryInterface,p,a,b)
#define IDsDriverPropertySet_AddRef(p)			ICOM_CALL (AddRef,p)
#define IDsDriverPropertySet_Release(p)			ICOM_CALL (Release,p)
    /*** IDsDriverPropertySet methods ***/
#define IDsDriverPropertySet_Get(p,a,b,c,d,e,f)		ICOM_CALL6(Get,p,a,b,c,d,e,f)
#define IDsDriverPropertySet_Set(p,a,b,c,d,e)		ICOM_CALL5(Set,p,a,b,c,d,e)
#define IDsDriverPropertySet_QuerySupport(p,a,b,c)	ICOM_CALL3(QuerySupport,p,a,b,c)

/* Defined property sets */
DEFINE_GUID(DSPROPSETID_DirectSound3DListener,	  0x6D047B40, 0x7AF9, 0x11D0, 0x92, 0x94, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);
typedef enum
{
    DSPROPERTY_DIRECTSOUND3DLISTENER_ALL,
    DSPROPERTY_DIRECTSOUND3DLISTENER_POSITION,
    DSPROPERTY_DIRECTSOUND3DLISTENER_VELOCITY,
    DSPROPERTY_DIRECTSOUND3DLISTENER_ORIENTATION,
    DSPROPERTY_DIRECTSOUND3DLISTENER_DISTANCEFACTOR,
    DSPROPERTY_DIRECTSOUND3DLISTENER_ROLLOFFFACTOR,
    DSPROPERTY_DIRECTSOUND3DLISTENER_DOPPLERFACTOR,
    DSPROPERTY_DIRECTSOUND3DLISTENER_BATCH,
    DSPROPERTY_DIRECTSOUND3DLISTENER_ALLOCATION
} DSPROPERTY_DIRECTSOUND3DLISTENER;

DEFINE_GUID(DSPROPSETID_DirectSound3DBuffer,	  0x6D047B41, 0x7AF9, 0x11D0, 0x92, 0x94, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);
typedef enum
{
    DSPROPERTY_DIRECTSOUND3DBUFFER_ALL,
    DSPROPERTY_DIRECTSOUND3DBUFFER_POSITION,
    DSPROPERTY_DIRECTSOUND3DBUFFER_VELOCITY,
    DSPROPERTY_DIRECTSOUND3DBUFFER_CONEANGLES,
    DSPROPERTY_DIRECTSOUND3DBUFFER_CONEORIENTATION,
    DSPROPERTY_DIRECTSOUND3DBUFFER_CONEOUTSIDEVOLUME,
    DSPROPERTY_DIRECTSOUND3DBUFFER_MINDISTANCE,
    DSPROPERTY_DIRECTSOUND3DBUFFER_MAXDISTANCE,
    DSPROPERTY_DIRECTSOUND3DBUFFER_MODE
} DSPROPERTY_DIRECTSOUND3DBUFFER;

DEFINE_GUID(DSPROPSETID_DirectSoundSpeakerConfig, 0x6D047B42, 0x7AF9, 0x11D0, 0x92, 0x94, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);
typedef enum
{
    DSPROPERTY_DIRECTSOUNDSPEAKERCONFIG_SPEAKERCONFIG
} DSPROPERTY_DIRECTSOUNDSPEAKERCONFIG;

/*****************************************************************************
 * IDsDriverNotify interface
 */
#define INTERFACE IDsDriverNotify
#define IDsDriverNotify_METHODS \
    STDMETHOD(SetNotificationPositions)(THIS_ DWORD dwPositionNotifies,LPCDSBPOSITIONNOTIFY pcPositionNotifies) PURE;
#define IDsDriverNotify_IMETHODS \
    IUnknown_METHODS \
    IDsDriverNotify_METHODS
ICOM_DEFINE(IDsDriverNotify,IUnknown)
#undef INTERFACE

    /*** IUnknown methods ***/
#define IDsDriverNotify_QueryInterface(p,a,b)		ICOM_CALL2(QueryInterface,p,a,b)
#define IDsDriverNotify_AddRef(p)			ICOM_CALL (AddRef,p)
#define IDsDriverNotify_Release(p)			ICOM_CALL (Release,p)
    /*** IDsDriverNotify methods ***/
#define IDsDriverNotify_SetNotificationPositions(p,a,b)	ICOM_CALL2(SetNotificationPositions,p,a,b)

/*****************************************************************************
 * IDsCaptureDriver interface
 */
#define INTERFACE IDsCaptureDriver
#define IDsCaptureDriver_METHODS \
    STDMETHOD(GetDriverDesc)(THIS_ PDSDRIVERDESC pDsDriverDesc) PURE; \
    STDMETHOD(Open)(THIS) PURE; \
    STDMETHOD(Close)(THIS) PURE; \
    STDMETHOD(GetCaps)(THIS_ PDSCDRIVERCAPS pDsDrvCaps) PURE; \
    STDMETHOD(CreateCaptureBuffer)(THIS_ LPWAVEFORMATEX pwfx,DWORD dwFlags,DWORD dwCardAddress,LPDWORD pdwcbBufferSize,LPBYTE *ppbBuffer,LPVOID *ppvObj) PURE;
#define IDsCaptureDriver_IMETHODS \
    IUnknown_METHODS \
    IDsCaptureDriver_METHODS
ICOM_DEFINE(IDsCaptureDriver,IUnknown)
#undef INTERFACE

    /*** IUnknown methods ***/
#define IDsCaptureDriver_QueryInterface(p,a,b)		ICOM_CALL2(QueryInterface,p,a,b)
#define IDsCaptureDriver_AddRef(p)			ICOM_CALL (AddRef,p)
#define IDsCaptureDriver_Release(p)			ICOM_CALL (Release,p)
    /*** IDsCaptureDriver methods ***/
#define IDsCaptureDriver_GetDriverDesc(p,a)		ICOM_CALL1(GetDriverDesc,p,a)
#define IDsCaptureDriver_Open(p)			ICOM_CALL (Open,p)
#define IDsCaptureDriver_Close(p)			ICOM_CALL (Close,p)
#define IDsCaptureDriver_GetCaps(p,a)			ICOM_CALL1(GetCaps,p,a)
#define IDsCaptureDriver_CreateCaptureBuffer(p,a,b,c,d,e,f) ICOM_CALL6(CreateCaptureBuffer,p,a,b,c,d,e,f)

/*****************************************************************************
 * IDsCaptureDriverBuffer interface
 */
#define INTERFACE IDsCaptureDriverBuffer
#define IDsCaptureDriverBuffer_METHODS \
    STDMETHOD(Lock)(THIS_ LPVOID *ppvAudio1,LPDWORD pdwLen1,LPVOID *ppvAudio2,LPDWORD pdwLen2,DWORD dwWritePosition,DWORD dwWriteLen,DWORD dwFlags) PURE; \
    STDMETHOD(Unlock)(THIS_ LPVOID pvAudio1,DWORD dwLen1,LPVOID pvAudio2,DWORD dwLen2) PURE; \
    STDMETHOD(SetFormat)(THIS_ LPWAVEFORMATEX pwfxToSet) PURE; \
    STDMETHOD(GetPosition)(THIS_ LPDWORD lpdwCurrentPlayCursor,LPDWORD lpdwCurrentWriteCursor) PURE; \
    STDMETHOD(GetStatus)(THIS_ LPDWORD lpdwStatus) PURE; \
    STDMETHOD(Start)(THIS_ DWORD dwFlags) PURE; \
    STDMETHOD(Stop)(THIS) PURE;
#define IDsCaptureDriverBuffer_IMETHODS \
    IUnknown_METHODS \
    IDsCaptureDriverBuffer_METHODS
ICOM_DEFINE(IDsCaptureDriverBuffer,IUnknown)
#undef INTERFACE

    /*** IUnknown methods ***/
#define IDsCaptureDriverBuffer_QueryInterface(p,a,b)	ICOM_CALL2(QueryInterface,p,a,b)
#define IDsCaptureDriverBuffer_AddRef(p)		ICOM_CALL (AddRef,p)
#define IDsCaptureDriverBuffer_Release(p)		ICOM_CALL (Release,p)
    /*** IDsCaptureDriverBuffer methods ***/
#define IDsCaptureDriverBuffer_Lock(p,a,b,c,d,e,f,g)	ICOM_CALL7(Lock,p,a,b,c,d,e,f,g)
#define IDsCaptureDriverBuffer_Unlock(p,a,b,c,d)	ICOM_CALL4(Unlock,p,a,b,c,d)
#define IDsCaptureDriverBuffer_SetFormat(p,a)		ICOM_CALL1(SetFormat,p,a)
#define IDsCaptureDriverBuffer_GetPosition(p,a,b)	ICOM_CALL2(GetPosition,p,a,b)
#define IDsCaptureDriverBuffer_GetStatus(p,a)		ICOM_CALL1(GetStatus,p,a)
#define IDsCaptureDriverBuffer_Start(p,a)		ICOM_CALL1(Start,p,a)
#define IDsCaptureDriverBuffer_Stop(p)			ICOM_CALL (Stop,p)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __WINE_DSDRIVER_H */
