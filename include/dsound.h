/*
 * Copyright (C) the Wine project
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

#ifndef __WINE_DSOUND_H
#define __WINE_DSOUND_H

#ifndef DIRECTSOUND_VERSION
#define DIRECTSOUND_VERSION 0x0800
#endif

#include "mmsystem.h"
#include "d3dtypes.h"

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_GUID(CLSID_DirectSound,		0x47d4d946, 0x62e8, 0x11cf, 0x93, 0xbc, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);
DEFINE_GUID(CLSID_DirectSound8,		0x3901cc3f, 0x84b5, 0x4fa4, 0xba, 0x35, 0xaa, 0x81, 0x72, 0xb8, 0xa0, 0x9b);
DEFINE_GUID(CLSID_DirectSoundCapture,	0xb0210780, 0x89cd, 0x11d0, 0xaf, 0x08, 0x00, 0xa0, 0xc9, 0x25, 0xcd, 0x16);
DEFINE_GUID(CLSID_DirectSoundCapture8,	0xe4bcac13, 0x7f99, 0x4908, 0x9a, 0x8e, 0x74, 0xe3, 0xbf, 0x24, 0xb6, 0xe1);
DEFINE_GUID(CLSID_DirectSoundFullDuplex,0xfea4300c, 0x7959, 0x4147, 0xb2, 0x6a, 0x23, 0x77, 0xb9, 0xe7, 0xa9, 0x1d);

DEFINE_GUID(IID_IDirectSound,		0x279AFA83,0x4981,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60);
typedef struct IDirectSound IDirectSound,*LPDIRECTSOUND;

DEFINE_GUID(IID_IDirectSound8,		0xC50A7E93,0xF395,0x4834,0x9E,0xF6,0x7F,0xA9,0x9D,0xE5,0x09,0x66);
typedef struct IDirectSound8 IDirectSound8,*LPDIRECTSOUND8;

DEFINE_GUID(IID_IDirectSoundBuffer,	0x279AFA85,0x4981,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60);
typedef struct IDirectSoundBuffer IDirectSoundBuffer,*LPDIRECTSOUNDBUFFER,**LPLPDIRECTSOUNDBUFFER;

DEFINE_GUID(IID_IDirectSoundBuffer8,	0x6825A449,0x7524,0x4D82,0x92,0x0F,0x50,0xE3,0x6A,0xB3,0xAB,0x1E);
typedef struct IDirectSoundBuffer8 IDirectSoundBuffer8,*LPDIRECTSOUNDBUFFER8,**LPLPDIRECTSOUNDBUFFER8;

DEFINE_GUID(IID_IDirectSoundNotify,	0xB0210783,0x89cd,0x11d0,0xAF,0x08,0x00,0xA0,0xC9,0x25,0xCD,0x16);
typedef struct IDirectSoundNotify IDirectSoundNotify,*LPDIRECTSOUNDNOTIFY;

DEFINE_GUID(IID_IDirectSound3DListener,	0x279AFA84,0x4981,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60);
typedef struct IDirectSound3DListener IDirectSound3DListener,*LPDIRECTSOUND3DLISTENER;

DEFINE_GUID(IID_IDirectSound3DBuffer,	0x279AFA86,0x4981,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60);
typedef struct IDirectSound3DBuffer IDirectSound3DBuffer,*LPDIRECTSOUND3DBUFFER;

DEFINE_GUID(IID_IDirectSoundCapture,	0xB0210781,0x89CD,0x11D0,0xAF,0x08,0x00,0xA0,0xC9,0x25,0xCD,0x16);
typedef struct IDirectSoundCapture IDirectSoundCapture,*LPDIRECTSOUNDCAPTURE;

DEFINE_GUID(IID_IDirectSoundCaptureBuffer,0xB0210782,0x89CD,0x11D0,0xAF,0x08,0x00,0xA0,0xC9,0x25,0xCD,0x16);
typedef struct IDirectSoundCaptureBuffer IDirectSoundCaptureBuffer,*LPDIRECTSOUNDCAPTUREBUFFER;

DEFINE_GUID(IID_IDirectSoundCaptureBuffer8,0x00990DF4,0x0DBB,0x4872,0x83,0x3E,0x6D,0x30,0x3E,0x80,0xAE,0xB6);
typedef struct IDirectSoundCaptureBuffer8 IDirectSoundCaptureBuffer8,*LPDIRECTSOUNDCAPTUREBUFFER8,**LPLPDIRECTSOUNDCAPTUREBUFFER8;

DEFINE_GUID(IID_IDirectSoundFullDuplex,	0xEDCB4C7A,0xDAAB,0x4216,0xA4,0x2E,0x6C,0x50,0x59,0x6D,0xDC,0x1D);
typedef struct IDirectSoundFullDuplex IDirectSoundFullDuplex,*LPDIRECTSOUNDFULLDUPLEX;

DEFINE_GUID(IID_IKsPropertySet,		0x31EFAC30,0x515C,0x11D0,0xA9,0xAA,0x00,0xAA,0x00,0x61,0xBE,0x93);
typedef struct IKsPropertySet IKsPropertySet,*LPKSPROPERTYSET;

DEFINE_GUID(DSDEVID_DefaultPlayback,     0xDEF00000,0x9C6D,0x47Ed,0xAA,0xF1,0x4D,0xDA,0x8F,0x2B,0x5C,0x03);
DEFINE_GUID(DSDEVID_DefaultCapture,      0xDEF00001,0x9C6D,0x47Ed,0xAA,0xF1,0x4D,0xDA,0x8F,0x2B,0x5C,0x03);
DEFINE_GUID(DSDEVID_DefaultVoicePlayback,0xDEF00002,0x9C6D,0x47Ed,0xAA,0xF1,0x4D,0xDA,0x8F,0x2B,0x5C,0x03);
DEFINE_GUID(DSDEVID_DefaultVoiceCapture, 0xDEF00003,0x9C6D,0x47ED,0xAA,0xF1,0x4D,0xDA,0x8F,0x2B,0x5C,0x03);

DEFINE_GUID(DSDEVID_WinePlayback,        0x40316A1D,0x605B,0xD611,0x87,0xC6,0x00,0x80,0xAD,0x00,0x02,0xFE);

#define	_FACDS		0x878
#define	MAKE_DSHRESULT(code)		MAKE_HRESULT(1,_FACDS,code)

#define DS_OK				0
#define DSERR_ALLOCATED			MAKE_DSHRESULT(10)
#define DSERR_CONTROLUNAVAIL		MAKE_DSHRESULT(30)
#define DSERR_INVALIDPARAM		E_INVALIDARG
#define DSERR_INVALIDCALL		MAKE_DSHRESULT(50)
#define DSERR_GENERIC			E_FAIL
#define DSERR_PRIOLEVELNEEDED		MAKE_DSHRESULT(70)
#define DSERR_OUTOFMEMORY		E_OUTOFMEMORY
#define DSERR_BADFORMAT			MAKE_DSHRESULT(100)
#define DSERR_UNSUPPORTED		E_NOTIMPL
#define DSERR_NODRIVER			MAKE_DSHRESULT(120)
#define DSERR_ALREADYINITIALIZED	MAKE_DSHRESULT(130)
#define DSERR_NOAGGREGATION		CLASS_E_NOAGGREGATION
#define DSERR_BUFFERLOST		MAKE_DSHRESULT(150)
#define DSERR_OTHERAPPHASPRIO		MAKE_DSHRESULT(160)
#define DSERR_UNINITIALIZED		MAKE_DSHRESULT(170)

#define DSCAPS_PRIMARYMONO          0x00000001
#define DSCAPS_PRIMARYSTEREO        0x00000002
#define DSCAPS_PRIMARY8BIT          0x00000004
#define DSCAPS_PRIMARY16BIT         0x00000008
#define DSCAPS_CONTINUOUSRATE       0x00000010
#define DSCAPS_EMULDRIVER           0x00000020
#define DSCAPS_CERTIFIED            0x00000040
#define DSCAPS_SECONDARYMONO        0x00000100
#define DSCAPS_SECONDARYSTEREO      0x00000200
#define DSCAPS_SECONDARY8BIT        0x00000400
#define DSCAPS_SECONDARY16BIT       0x00000800

#define	DSSCL_NORMAL		1
#define	DSSCL_PRIORITY		2
#define	DSSCL_EXCLUSIVE		3
#define	DSSCL_WRITEPRIMARY	4

typedef struct _DSCAPS
{
    DWORD	dwSize;
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
    DWORD	dwUnlockTransferRateHwBuffers;
    DWORD	dwPlayCpuOverheadSwBuffers;
    DWORD	dwReserved1;
    DWORD	dwReserved2;
} DSCAPS,*LPDSCAPS;

#define DSBPLAY_LOOPING         0x00000001

#define DSBSTATUS_PLAYING           0x00000001
#define DSBSTATUS_BUFFERLOST        0x00000002
#define DSBSTATUS_LOOPING           0x00000004


#define DSBLOCK_FROMWRITECURSOR     0x00000001
#define DSBLOCK_ENTIREBUFFER        0x00000002

#define DSBCAPS_PRIMARYBUFFER       0x00000001
#define DSBCAPS_STATIC              0x00000002
#define DSBCAPS_LOCHARDWARE         0x00000004
#define DSBCAPS_LOCSOFTWARE         0x00000008
#define DSBCAPS_CTRL3D              0x00000010
#define DSBCAPS_CTRLFREQUENCY       0x00000020
#define DSBCAPS_CTRLPAN             0x00000040
#define DSBCAPS_CTRLVOLUME          0x00000080
#define DSBCAPS_CTRLPOSITIONNOTIFY  0x00000100
#define DSBCAPS_CTRLDEFAULT         0x000000E0  /* Pan + volume + frequency. */
#define DSBCAPS_CTRLALL             0x000001F0  /* All control capabilities */
#define DSBCAPS_STICKYFOCUS         0x00004000
#define DSBCAPS_GLOBALFOCUS         0x00008000
#define DSBCAPS_GETCURRENTPOSITION2 0x00010000  /* More accurate play cursor under emulation*/
#define DSBCAPS_MUTE3DATMAXDISTANCE 0x00020000

#define DSBSIZE_MIN                 4
#define DSBSIZE_MAX                 0xFFFFFFF
#define DSBPAN_LEFT                 -10000
#define DSBPAN_RIGHT                 10000
#define DSBVOLUME_MAX                    0
#define DSBVOLUME_MIN               -10000
#define DSBFREQUENCY_MIN            100
#define DSBFREQUENCY_MAX            100000
#define DSBFREQUENCY_ORIGINAL       0

typedef struct _DSBCAPS
{
    DWORD	dwSize;
    DWORD	dwFlags;
    DWORD	dwBufferBytes;
    DWORD	dwUnlockTransferRate;
    DWORD	dwPlayCpuOverhead;
} DSBCAPS,*LPDSBCAPS;

#define DSSCL_NORMAL                1
#define DSSCL_PRIORITY              2
#define DSSCL_EXCLUSIVE             3
#define DSSCL_WRITEPRIMARY          4

typedef struct _DSEFFECTDESC
{
    DWORD	dwSize;
    DWORD	dwFlags;
    GUID	guidDSFXClass;
    DWORD_PTR	dwReserved1;
    DWORD_PTR	dwReserved2;
} DSEFFECTDESC,*LPDSEFFECTDESC;
typedef const DSEFFECTDESC *LPCDSEFFECTDESC;

#define DSFX_LOCHARDWARE    0x00000001
#define DSFX_LOCSOFTWARE    0x00000002

enum
{
    DSFXR_PRESENT,
    DSFXR_LOCHARDWARE,
    DSFXR_LOCSOFTWARE,
    DSFXR_UNALLOCATED,
    DSFXR_FAILED,
    DSFXR_UNKNOWN,
    DSFXR_SENDLOOP
};

typedef struct _DSBUFFERDESC
{
    DWORD		dwSize;
    DWORD		dwFlags;
    DWORD		dwBufferBytes;
    DWORD		dwReserved;
    LPWAVEFORMATEX	lpwfxFormat;
#if DIRECTSOUND_VERSION >= 0x0700
    GUID		guid3DAlgorithm;
#endif
} DSBUFFERDESC,*LPDSBUFFERDESC;
typedef const DSBUFFERDESC *LPCDSBUFFERDESC;

typedef struct _DSBPOSITIONNOTIFY
{
    DWORD	dwOffset;
    HANDLE	hEventNotify;
} DSBPOSITIONNOTIFY,*LPDSBPOSITIONNOTIFY;
typedef const DSBPOSITIONNOTIFY *LPCDSBPOSITIONNOTIFY;

#define DSSPEAKER_HEADPHONE     1
#define DSSPEAKER_MONO          2
#define DSSPEAKER_QUAD          3
#define DSSPEAKER_STEREO        4
#define DSSPEAKER_SURROUND      5

#define DSSPEAKER_GEOMETRY_MIN      0x00000005  /* 5 degrees */
#define DSSPEAKER_GEOMETRY_NARROW   0x0000000A  /* 10 degrees */
#define DSSPEAKER_GEOMETRY_WIDE     0x00000014  /* 20 degrees */
#define DSSPEAKER_GEOMETRY_MAX      0x000000B4  /* 180 degrees */

#define DS_CERTIFIED                0x00000000
#define DS_UNCERTIFIED              0x00000001

typedef struct _DSCBUFFERDESC
{
  DWORD          dwSize;
  DWORD          dwFlags;
  DWORD          dwBufferBytes;
  DWORD          dwReserved;
  LPWAVEFORMATEX lpwfxFormat;
} DSCBUFFERDESC, *LPDSCBUFFERDESC;
typedef const DSCBUFFERDESC *LPCDSCBUFFERDESC;

typedef struct _DSCCAPS
{
  DWORD dwSize;
  DWORD dwFlags;
  DWORD dwFormats;
  DWORD dwChannels;
} DSCCAPS, *LPDSCCAPS;
typedef const DSCCAPS *LPCDSCCAPS;

typedef struct _DSCBCAPS
{
  DWORD dwSize;
  DWORD dwFlags;
  DWORD dwBufferBytes;
  DWORD dwReserved;
} DSCBCAPS, *LPDSCBCAPS;
typedef const DSCBCAPS *LPCDSCBCAPS;

#define DSCCAPS_EMULDRIVER          DSCAPS_EMULDRIVER
#define DSCCAPS_CERTIFIED           DSCAPS_CERTIFIED

#define DSCBCAPS_WAVEMAPPED         0x80000000
#define DSCBCAPS_CTRLFX             0x00000200

#define DSCBLOCL_ENTIREBUFFER       0x00000001
#define DSCBSTART_LOOPING           0x00000001
#define DSCBPN_OFFSET_STOP          0xffffffff

#define DSCBSTATUS_CAPTURING        0x00000001
#define DSCBSTATUS_LOOPING          0x00000002

#ifndef __LPCGUID_DEFINED__
#define __LPCGUID_DEFINED__
typedef const GUID *LPCGUID;
#endif

typedef LPVOID* LPLPVOID;

typedef BOOL (CALLBACK *LPDSENUMCALLBACKW)(LPGUID,LPCWSTR,LPCWSTR,LPVOID);
typedef BOOL (CALLBACK *LPDSENUMCALLBACKA)(LPGUID,LPCSTR,LPCSTR,LPVOID);

extern HRESULT WINAPI DirectSoundCreate(LPCGUID lpGUID,LPDIRECTSOUND *ppDS,LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DirectSoundEnumerateA(LPDSENUMCALLBACKA, LPVOID);
extern HRESULT WINAPI DirectSoundEnumerateW(LPDSENUMCALLBACKW, LPVOID);

extern HRESULT WINAPI DirectSoundCaptureCreate(LPCGUID lpGUID, LPDIRECTSOUNDCAPTURE *ppDSC, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DirectSoundCaptureEnumerateA(LPDSENUMCALLBACKA, LPVOID);
extern HRESULT WINAPI DirectSoundCaptureEnumerateW(LPDSENUMCALLBACKW, LPVOID);

extern HRESULT WINAPI DirectSoundCreate8(LPCGUID lpGUID,LPDIRECTSOUND8 *ppDS8,LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DirectSoundCaptureCreate8(LPCGUID lpGUID, LPDIRECTSOUNDCAPTURE *ppDSC8, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DirectSoundFullDuplexCreate(LPCGUID pcGuidCaptureDevice, LPCGUID pcGuidRenderDevice, 
    LPCDSCBUFFERDESC pcDSCBufferDesc, LPCDSBUFFERDESC pcDSBufferDesc, HWND hWnd, DWORD dwLevel, 
    LPDIRECTSOUNDFULLDUPLEX *ppDSFD, LPDIRECTSOUNDCAPTUREBUFFER8 *ppDSCBuffer8, LPDIRECTSOUNDBUFFER8 *ppDSBuffer8, LPUNKNOWN pUnkOuter); 
#define DirectSoundFullDuplexCreate8 DirectSoundFullDuplexCreate
extern HRESULT WINAPI GetDeviceID(LPCGUID lpGuidSrc, LPGUID lpGuidDest);


/*****************************************************************************
 * IDirectSound interface
 */
#define ICOM_INTERFACE IDirectSound
#define IDirectSound_METHODS \
    ICOM_METHOD3(HRESULT,CreateSoundBuffer,    LPDSBUFFERDESC,lpcDSBufferDesc, LPLPDIRECTSOUNDBUFFER,lplpDirectSoundBuffer, IUnknown*,pUnkOuter) \
    ICOM_METHOD1(HRESULT,GetCaps,              LPDSCAPS,lpDSCaps) \
    ICOM_METHOD2(HRESULT,DuplicateSoundBuffer, LPDIRECTSOUNDBUFFER,lpDsbOriginal, LPLPDIRECTSOUNDBUFFER,lplpDsbDuplicate) \
    ICOM_METHOD2(HRESULT,SetCooperativeLevel,  HWND,hwnd, DWORD,dwLevel) \
    ICOM_METHOD (HRESULT,Compact) \
    ICOM_METHOD1(HRESULT,GetSpeakerConfig,     LPDWORD,lpdwSpeakerConfig) \
    ICOM_METHOD1(HRESULT,SetSpeakerConfig,     DWORD,dwSpeakerConfig) \
    ICOM_METHOD1(HRESULT,Initialize,           LPCGUID,lpcGuid)
#define IDirectSound_IMETHODS \
    IUnknown_IMETHODS \
    IDirectSound_METHODS
ICOM_DEFINE(IDirectSound,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IDirectSound_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirectSound_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirectSound_Release(p)            ICOM_CALL (Release,p)
    /*** IDirectSound methods ***/
#define IDirectSound_CreateSoundBuffer(p,a,b,c)  ICOM_CALL3(CreateSoundBuffer,p,a,b,c)
#define IDirectSound_GetCaps(p,a)                ICOM_CALL1(GetCaps,p,a)
#define IDirectSound_DuplicateSoundBuffer(p,a,b) ICOM_CALL2(DuplicateSoundBuffer,p,a,b)
#define IDirectSound_SetCooperativeLevel(p,a,b)  ICOM_CALL2(SetCooperativeLevel,p,a,b)
#define IDirectSound_Compact(p)                  ICOM_CALL (Compact,p)
#define IDirectSound_GetSpeakerConfig(p,a)       ICOM_CALL1(GetSpeakerConfig,p,a)
#define IDirectSound_SetSpeakerConfig(p,a)       ICOM_CALL1(SetSpeakerConfig,p,a)
#define IDirectSound_Initialize(p,a)             ICOM_CALL1(Initialize,p,a)


/*****************************************************************************
 * IDirectSound8 interface
 */
#define ICOM_INTERFACE IDirectSound8
#define IDirectSound8_METHODS \
    ICOM_METHOD3(HRESULT,CreateSoundBuffer,    LPDSBUFFERDESC,lpcDSBufferDesc, LPLPDIRECTSOUNDBUFFER8,lplpDirectSoundBuffer, IUnknown*,pUnkOuter) \
    ICOM_METHOD1(HRESULT,GetCaps,              LPDSCAPS,lpDSCaps) \
    ICOM_METHOD2(HRESULT,DuplicateSoundBuffer, LPDIRECTSOUNDBUFFER8,lpDsbOriginal, LPLPDIRECTSOUNDBUFFER8,lplpDsbDuplicate) \
    ICOM_METHOD2(HRESULT,SetCooperativeLevel,  HWND,hwnd, DWORD,dwLevel) \
    ICOM_METHOD (HRESULT,Compact) \
    ICOM_METHOD1(HRESULT,GetSpeakerConfig,     LPDWORD,lpdwSpeakerConfig) \
    ICOM_METHOD1(HRESULT,SetSpeakerConfig,     DWORD,dwSpeakerConfig) \
    ICOM_METHOD1(HRESULT,Initialize,           LPCGUID,lpcGuid) \
    ICOM_METHOD1(HRESULT,VerifyCertification,  LPDWORD,pdwCertified)
#define IDirectSound8_IMETHODS \
    IUnknown_IMETHODS \
    IDirectSound8_METHODS
ICOM_DEFINE(IDirectSound8,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IDirectSound8_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirectSound8_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirectSound8_Release(p)            ICOM_CALL (Release,p)
    /*** IDirectSound methods ***/
#define IDirectSound8_CreateSoundBuffer(p,a,b,c)  ICOM_CALL3(CreateSoundBuffer,p,a,b,c)
#define IDirectSound8_GetCaps(p,a)                ICOM_CALL1(GetCaps,p,a)
#define IDirectSound8_DuplicateSoundBuffer(p,a,b) ICOM_CALL2(DuplicateSoundBuffer,p,a,b)
#define IDirectSound8_SetCooperativeLevel(p,a,b)  ICOM_CALL2(SetCooperativeLevel,p,a,b)
#define IDirectSound8_Compact(p)                  ICOM_CALL (Compact,p)
#define IDirectSound8_GetSpeakerConfig(p,a)       ICOM_CALL1(GetSpeakerConfig,p,a)
#define IDirectSound8_SetSpeakerConfig(p,a)       ICOM_CALL1(SetSpeakerConfig,p,a)
#define IDirectSound8_Initialize(p,a)             ICOM_CALL1(Initialize,p,a)
    /*** IDirectSound8 methods ***/
#define IDirectSound8_VerifyCertification(p,a)    ICOM_CALL1(VerifyCertification,p,a)


/*****************************************************************************
 * IDirectSoundBuffer interface
 */
#define ICOM_INTERFACE IDirectSoundBuffer
#define IDirectSoundBuffer_METHODS \
    ICOM_METHOD1(HRESULT,GetCaps,              LPDSBCAPS,lpDSBufferCaps) \
    ICOM_METHOD2(HRESULT,GetCurrentPosition,   LPDWORD,lpdwCurrentPlayCursor, LPDWORD,lpdwCurrentWriteCursor) \
    ICOM_METHOD3(HRESULT,GetFormat,            LPWAVEFORMATEX,lpwfxFormat, DWORD,dwSizeAllocated, LPDWORD,lpdwSizeWritten) \
    ICOM_METHOD1(HRESULT,GetVolume,            LPLONG,lplVolume) \
    ICOM_METHOD1(HRESULT,GetPan,               LPLONG,lplpan) \
    ICOM_METHOD1(HRESULT,GetFrequency,         LPDWORD,lpdwFrequency) \
    ICOM_METHOD1(HRESULT,GetStatus,            LPDWORD,lpdwStatus) \
    ICOM_METHOD2(HRESULT,Initialize,           LPDIRECTSOUND,lpDirectSound, LPDSBUFFERDESC,lpcDSBufferDesc) \
    ICOM_METHOD7(HRESULT,Lock,                 DWORD,dwWriteCursor, DWORD,dwWriteBytes, LPVOID,lplpvAudioPtr1, LPDWORD,lpdwAudioBytes1, LPVOID,lplpvAudioPtr2, LPDWORD,lpdwAudioBytes2, DWORD,dwFlags) \
    ICOM_METHOD3(HRESULT,Play,                 DWORD,dwReserved1, DWORD,dwReserved2, DWORD,dwFlags) \
    ICOM_METHOD1(HRESULT,SetCurrentPosition,   DWORD,dwNewPosition) \
    ICOM_METHOD1(HRESULT,SetFormat,            LPWAVEFORMATEX,lpcfxFormat) \
    ICOM_METHOD1(HRESULT,SetVolume,            LONG,lVolume) \
    ICOM_METHOD1(HRESULT,SetPan,               LONG,lPan) \
    ICOM_METHOD1(HRESULT,SetFrequency,         DWORD,dwFrequency) \
    ICOM_METHOD (HRESULT,Stop) \
    ICOM_METHOD4(HRESULT,Unlock,               LPVOID,lpvAudioPtr1, DWORD,dwAudioBytes1, LPVOID,lpvAudioPtr2, DWORD,dwAudioPtr2) \
    ICOM_METHOD (HRESULT,Restore)
#define IDirectSoundBuffer_IMETHODS \
    IUnknown_IMETHODS \
    IDirectSoundBuffer_METHODS
ICOM_DEFINE(IDirectSoundBuffer,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IDirectSoundBuffer_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirectSoundBuffer_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirectSoundBuffer_Release(p)            ICOM_CALL (Release,p)
    /*** IDirectSoundBuffer methods ***/
#define IDirectSoundBuffer_GetCaps(p,a)                ICOM_CALL1(GetCaps,p,a)
#define IDirectSoundBuffer_GetCurrentPosition(p,a,b)   ICOM_CALL2(GetCurrentPosition,p,a,b)
#define IDirectSoundBuffer_GetFormat(p,a,b,c)          ICOM_CALL3(GetFormat,p,a,b,c)
#define IDirectSoundBuffer_GetVolume(p,a)              ICOM_CALL1(GetVolume,p,a)
#define IDirectSoundBuffer_GetPan(p,a)                 ICOM_CALL1(GetPan,p,a)
#define IDirectSoundBuffer_GetFrequency(p,a)           ICOM_CALL1(GetFrequency,p,a)
#define IDirectSoundBuffer_GetStatus(p,a)              ICOM_CALL1(GetStatus,p,a)
#define IDirectSoundBuffer_Initialize(p,a,b)           ICOM_CALL2(Initialize,p,a,b)
#define IDirectSoundBuffer_Lock(p,a,b,c,d,e,f,g)       ICOM_CALL7(Lock,p,a,b,c,d,e,f,g)
#define IDirectSoundBuffer_Play(p,a,b,c)               ICOM_CALL3(Play,p,a,b,c)
#define IDirectSoundBuffer_SetCurrentPosition(p,a)     ICOM_CALL1(SetCurrentPosition,p,a)
#define IDirectSoundBuffer_SetFormat(p,a)              ICOM_CALL1(SetFormat,p,a)
#define IDirectSoundBuffer_SetVolume(p,a)              ICOM_CALL1(SetVolume,p,a)
#define IDirectSoundBuffer_SetPan(p,a)                 ICOM_CALL1(SetPan,p,a)
#define IDirectSoundBuffer_SetFrequency(p,a)           ICOM_CALL1(SetFrequency,p,a)
#define IDirectSoundBuffer_Stop(p)                     ICOM_CALL (Stop,p)
#define IDirectSoundBuffer_Unlock(p,a,b,c,d)           ICOM_CALL4(Unlock,p,a,b,c,d)
#define IDirectSoundBuffer_Restore(p)                  ICOM_CALL (Restore,p)


/*****************************************************************************
 * IDirectSoundBuffer8 interface
 */
#define ICOM_INTERFACE IDirectSoundBuffer8
#define IDirectSoundBuffer8_METHODS \
    ICOM_METHOD1(HRESULT,GetCaps,              LPDSBCAPS,lpDSBufferCaps) \
    ICOM_METHOD2(HRESULT,GetCurrentPosition,   LPDWORD,lpdwCurrentPlayCursor, LPDWORD,lpdwCurrentWriteCursor) \
    ICOM_METHOD3(HRESULT,GetFormat,            LPWAVEFORMATEX,lpwfxFormat, DWORD,dwSizeAllocated, LPDWORD,lpdwSizeWritten) \
    ICOM_METHOD1(HRESULT,GetVolume,            LPLONG,lplVolume) \
    ICOM_METHOD1(HRESULT,GetPan,               LPLONG,lplpan) \
    ICOM_METHOD1(HRESULT,GetFrequency,         LPDWORD,lpdwFrequency) \
    ICOM_METHOD1(HRESULT,GetStatus,            LPDWORD,lpdwStatus) \
    ICOM_METHOD2(HRESULT,Initialize,           LPDIRECTSOUND8,lpDirectSound, LPDSBUFFERDESC,lpcDSBufferDesc) \
    ICOM_METHOD7(HRESULT,Lock,                 DWORD,dwWriteCursor, DWORD,dwWriteBytes, LPVOID,lplpvAudioPtr1, LPDWORD,lpdwAudioBytes1, LPVOID,lplpvAudioPtr2, LPDWORD,lpdwAudioBytes2, DWORD,dwFlags) \
    ICOM_METHOD3(HRESULT,Play,                 DWORD,dwReserved1, DWORD,dwReserved2, DWORD,dwFlags) \
    ICOM_METHOD1(HRESULT,SetCurrentPosition,   DWORD,dwNewPosition) \
    ICOM_METHOD1(HRESULT,SetFormat,            LPWAVEFORMATEX,lpcfxFormat) \
    ICOM_METHOD1(HRESULT,SetVolume,            LONG,lVolume) \
    ICOM_METHOD1(HRESULT,SetPan,               LONG,lPan) \
    ICOM_METHOD1(HRESULT,SetFrequency,         DWORD,dwFrequency) \
    ICOM_METHOD (HRESULT,Stop) \
    ICOM_METHOD4(HRESULT,Unlock,               LPVOID,lpvAudioPtr1, DWORD,dwAudioBytes1, LPVOID,lpvAudioPtr2, DWORD,dwAudioPtr2) \
    ICOM_METHOD (HRESULT,Restore) \
    ICOM_METHOD3(HRESULT,SetFX,                DWORD,dwEffectsCount, LPDSEFFECTDESC,pDSFXDesc, LPDWORD,pdwResultCodes) \
    ICOM_METHOD3(HRESULT,AcquireResources,     DWORD,dwFlags, DWORD,dwEffectsCount, LPDWORD,pdwResultCodes) \
    ICOM_METHOD4(HRESULT,GetObjectInPath,      REFGUID,rguidObject, DWORD,dwIndex, REFGUID,rguidInterface, LPVOID*,ppObject)
#define IDirectSoundBuffer8_IMETHODS \
    IUnknown_IMETHODS \
    IDirectSoundBuffer8_METHODS
ICOM_DEFINE(IDirectSoundBuffer8,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IDirectSoundBuffer8_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirectSoundBuffer8_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirectSoundBuffer8_Release(p)            ICOM_CALL (Release,p)
    /*** IDirectSoundBuffer methods ***/
#define IDirectSoundBuffer8_GetCaps(p,a)                ICOM_CALL1(GetCaps,p,a)
#define IDirectSoundBuffer8_GetCurrentPosition(p,a,b)   ICOM_CALL2(GetCurrentPosition,p,a,b)
#define IDirectSoundBuffer8_GetFormat(p,a,b,c)          ICOM_CALL3(GetFormat,p,a,b,c)
#define IDirectSoundBuffer8_GetVolume(p,a)              ICOM_CALL1(GetVolume,p,a)
#define IDirectSoundBuffer8_GetPan(p,a)                 ICOM_CALL1(GetPan,p,a)
#define IDirectSoundBuffer8_GetFrequency(p,a)           ICOM_CALL1(GetFrequency,p,a)
#define IDirectSoundBuffer8_GetStatus(p,a)              ICOM_CALL1(GetStatus,p,a)
#define IDirectSoundBuffer8_Initialize(p,a,b)           ICOM_CALL2(Initialize,p,a,b)
#define IDirectSoundBuffer8_Lock(p,a,b,c,d,e,f,g)       ICOM_CALL7(Lock,p,a,b,c,d,e,f,g)
#define IDirectSoundBuffer8_Play(p,a,b,c)               ICOM_CALL3(Play,p,a,b,c)
#define IDirectSoundBuffer8_SetCurrentPosition(p,a)     ICOM_CALL1(SetCurrentPosition,p,a)
#define IDirectSoundBuffer8_SetFormat(p,a)              ICOM_CALL1(SetFormat,p,a)
#define IDirectSoundBuffer8_SetVolume(p,a)              ICOM_CALL1(SetVolume,p,a)
#define IDirectSoundBuffer8_SetPan(p,a)                 ICOM_CALL1(SetPan,p,a)
#define IDirectSoundBuffer8_SetFrequency(p,a)           ICOM_CALL1(SetFrequency,p,a)
#define IDirectSoundBuffer8_Stop(p)                     ICOM_CALL (Stop,p)
#define IDirectSoundBuffer8_Unlock(p,a,b,c,d)           ICOM_CALL4(Unlock,p,a,b,c,d)
#define IDirectSoundBuffer8_Restore(p)                  ICOM_CALL (Restore,p)
    /*** IDirectSoundBuffer8 methods ***/
#define IDirectSoundBuffer8_SetFX(p,a,b,c)              ICOM_CALL3(SetFX,p,a,b,c)
#define IDirectSoundBuffer8_AcquireResources(p,a,b,c)   ICOM_CALL3(AcquireResources,p,a,b,c)
#define IDirectSoundBuffer8_GetObjectInPath(p,a,b,c)    ICOM_CALL3(GetObjectInPath,p,a,b,c)


/*****************************************************************************
 * IDirectSoundCapture interface
 */
#define ICOM_INTERFACE IDirectSoundCapture
#define IDirectSoundCapture_METHODS \
    ICOM_METHOD3(HRESULT,CreateCaptureBuffer, LPCDSCBUFFERDESC,lpcDSCBufferDesc,LPDIRECTSOUNDCAPTUREBUFFER*,lplpDSCaptureBuffer, LPUNKNOWN,pUnk) \
    ICOM_METHOD1(HRESULT,GetCaps,             LPDSCCAPS,lpDSCCaps) \
    ICOM_METHOD1(HRESULT,Initialize,          LPCGUID,lpcGUID)

#define IDirectSoundCapture_IMETHODS \
    IUnknown_IMETHODS \
    IDirectSoundCapture_METHODS
ICOM_DEFINE(IDirectSoundCapture,IUnknown)
#undef ICOM_INTERFACE

#define IDirectSoundCapture_QueryInterface(p,a,b)        ICOM_CALL2(QueryInterface,p,a,b)
#define IDirectSoundCapture_AddRef(p)                    ICOM_CALL (AddRef,p)
#define IDirectSoundCapture_Release(p)                   ICOM_CALL (Release,p)
#define IDirectSoundCapture_CreateCaptureBuffer(p,a,b,c) ICOM_CALL3(CreateCaptureBuffer,p,a,b,c)
#define IDirectSoundCapture_GetCaps(p,a)                 ICOM_CALL1(GetCaps,p,a)
#define IDirectSoundCapture_Initialize(p,a)              ICOM_CALL1(Initialize,p,a)

/*****************************************************************************
 * IDirectSoundCaptureBuffer interface
 */
#define ICOM_INTERFACE IDirectSoundCaptureBuffer
#define IDirectSoundCaptureBuffer_METHODS \
    ICOM_METHOD1(HRESULT,GetCaps,             LPDSCBCAPS,lpDSCBCaps) \
    ICOM_METHOD2(HRESULT,GetCurrentPosition,  LPDWORD,lpdwCapturePosition,LPDWORD,lpdwReadPosition) \
    ICOM_METHOD3(HRESULT,GetFormat,           LPWAVEFORMATEX,lpwfxFormat, DWORD,dwSizeAllocated, LPDWORD,lpdwSizeWritten) \
    ICOM_METHOD1(HRESULT,GetStatus,           LPDWORD,lpdwStatus) \
    ICOM_METHOD2(HRESULT,Initialize,          LPDIRECTSOUNDCAPTURE,lpDSC, LPCDSCBUFFERDESC,lpcDSCBDesc) \
    ICOM_METHOD7(HRESULT,Lock,                DWORD,dwReadCusor, DWORD,dwReadBytes, LPVOID*,lplpvAudioPtr1, LPDWORD,lpdwAudioBytes1, LPVOID*,lplpvAudioPtr2, LPDWORD,lpdwAudioBytes2, DWORD,dwFlags) \
    ICOM_METHOD1(HRESULT,Start,               DWORD,dwFlags) \
    ICOM_METHOD (HRESULT,Stop) \
    ICOM_METHOD4(HRESULT,Unlock,              LPVOID,lpvAudioPtr1, DWORD,dwAudioBytes1, LPVOID,lpvAudioPtr2, DWORD,dwAudioBytes2)

#define IDirectSoundCaptureBuffer_IMETHODS \
    IUnknown_IMETHODS \
    IDirectSoundCaptureBuffer_METHODS
ICOM_DEFINE(IDirectSoundCaptureBuffer,IUnknown)
#undef ICOM_INTERFACE

#define IDirectSoundCaptureBuffer_QueryInterface(p,a,b)     ICOM_CALL2(QueryInterface,p,a,b)
#define IDirectSoundCaptureBuffer_AddRef(p)                 ICOM_CALL (AddRef,p)
#define IDirectSoundCaptureBuffer_Release(p)                ICOM_CALL (Release,p)
#define IDirectSoundCaptureBuffer_GetCaps(p,a)              ICOM_CALL1(GetCaps,p,a)
#define IDirectSoundCaptureBuffer_GetCurrentPosition(p,a,b) ICOM_CALL2(GetCurrentPosition,p,a,b)
#define IDirectSoundCaptureBuffer_GetFormat(p,a,b,c)        ICOM_CALL3(GetFormat,p,a,b,c)
#define IDirectSoundCaptureBuffer_GetStatus(p,a)            ICOM_CALL1(GetStatus,p,a)
#define IDirectSoundCaptureBuffer_Initialize(p,a,b)         ICOM_CALL2(Initialize,p,a,b)
#define IDirectSoundCaptureBuffer_Lock(p,a,b,c,d,e,f,g)     ICOM_CALL7(Lock,p,a,b,c,d,e,f,g)
#define IDirectSoundCaptureBuffer_Start(p,a)                ICOM_CALL1(Start,p,a)
#define IDirectSoundCaptureBuffer_Stop(p)                   ICOM_CALL (Stop,p)
#define IDirectSoundCaptureBuffer_Unlock(p,a,b,c,d)         ICOM_CALL4(Unlock,p,a,b,c,d)

/*****************************************************************************
 * IDirectSoundCaptureBuffer8 interface
 */
#define ICOM_INTERFACE IDirectSoundCaptureBuffer8
#define IDirectSoundCaptureBuffer8_METHODS \
    ICOM_METHOD4(HRESULT,GetObjectInPath, REFGUID,rguidObject, DWORD,dwIndex, REFGUID,rguidInterface, LPVOID*,ppObject) \
    ICOM_METHOD2(HRESULT,GetFXStatus,     DWORD,dwFXCount, LPDWORD,pdwFXStatus)

#define IDirectSoundCaptureBuffer8_IMETHODS \
    IDirectSoundCaptureBuffer_IMETHODS \
    IDirectSoundCaptureBuffer8_METHODS
ICOM_DEFINE(IDirectSoundCaptureBuffer8,IDirectSoundCaptureBuffer)
#undef ICOM_INTERFACE

#define IDirectSoundCaptureBuffer8_QueryInterface(p,a,b)      ICOM_CALL2(QueryInterface,p,a,b)
#define IDirectSoundCaptureBuffer8_AddRef(p)                  ICOM_CALL (AddRef,p)
#define IDirectSoundCaptureBuffer8_Release(p)                 ICOM_CALL (Release,p)
#define IDirectSoundCaptureBuffer8_GetCaps(p,a)               ICOM_CALL1(GetCaps,p,a)
#define IDirectSoundCaptureBuffer8_GetCurrentPosition(p,a,b)  ICOM_CALL2(GetCurrentPosition,p,a,b)
#define IDirectSoundCaptureBuffer8_GetFormat(p,a,b,c)         ICOM_CALL3(GetFormat,p,a,b,c)
#define IDirectSoundCaptureBuffer8_GetStatus(p,a)             ICOM_CALL1(GetStatus,p,a)
#define IDirectSoundCaptureBuffer8_Initialize(p,a,b)          ICOM_CALL2(Initialize,p,a,b)
#define IDirectSoundCaptureBuffer8_Lock(p,a,b,c,d,e,f,g)      ICOM_CALL7(Lock,p,a,b,c,d,e,f,g)
#define IDirectSoundCaptureBuffer8_Start(p,a)                 ICOM_CALL1(Start,p,a)
#define IDirectSoundCaptureBuffer8_Stop(p)                    ICOM_CALL (Stop,p)
#define IDirectSoundCaptureBuffer8_Unlock(p,a,b,c,d)          ICOM_CALL4(Unlock,p,a,b,c,d)
#define IDirectSoundCaptureBuffer8_GetObjectInPath(p,a,b,c,d) ICOM_CALL4(GetObjectInPath,p,a,b,c,d)
#define IDirectSoundCaptureBuffer8_GetFXStatus(p,a,b)         ICOM_CALL2(GetFXStatus,p,a,b)

/*****************************************************************************
 * IDirectSoundNotify interface
 */
#define WINE_NOBUFFER                   0x80000000

#define DSBPN_OFFSETSTOP		-1

#define ICOM_INTERFACE IDirectSoundNotify
#define IDirectSoundNotify_METHODS \
    ICOM_METHOD2(HRESULT,SetNotificationPositions, DWORD,cPositionNotifies, LPCDSBPOSITIONNOTIFY,lpcPositionNotifies)
#define IDirectSoundNotify_IMETHODS \
    IUnknown_IMETHODS \
    IDirectSoundNotify_METHODS
ICOM_DEFINE(IDirectSoundNotify,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IDirectSoundNotify_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirectSoundNotify_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirectSoundNotify_Release(p)            ICOM_CALL (Release,p)
/*** IDirectSoundNotify methods ***/
#define IDirectSoundNotify_SetNotificationPositions(p,a,b) ICOM_CALL2(SetNotificationPositions,p,a,b)


/*****************************************************************************
 * IDirectSound3DListener interface
 */
#define DS3DMODE_NORMAL             0x00000000
#define DS3DMODE_HEADRELATIVE       0x00000001
#define DS3DMODE_DISABLE            0x00000002

#define DS3D_IMMEDIATE              0x00000000
#define DS3D_DEFERRED               0x00000001

#define DS3D_MINDISTANCEFACTOR      0.0f
#define DS3D_MAXDISTANCEFACTOR      10.0f
#define DS3D_DEFAULTDISTANCEFACTOR  1.0f

#define DS3D_MINROLLOFFFACTOR       0.0f
#define DS3D_MAXROLLOFFFACTOR       10.0f
#define DS3D_DEFAULTROLLOFFFACTOR   1.0f

#define DS3D_MINDOPPLERFACTOR       0.0f
#define DS3D_MAXDOPPLERFACTOR       10.0f
#define DS3D_DEFAULTDOPPLERFACTOR   1.0f

#define DS3D_DEFAULTMINDISTANCE     1.0f
#define DS3D_DEFAULTMAXDISTANCE     1000000000.0f

#define DS3D_MINCONEANGLE           0
#define DS3D_MAXCONEANGLE           360
#define DS3D_DEFAULTCONEANGLE       360

#define DS3D_DEFAULTCONEOUTSIDEVOLUME   0

typedef struct _DS3DLISTENER {
	DWORD				dwSize;
	D3DVECTOR			vPosition;
	D3DVECTOR			vVelocity;
	D3DVECTOR			vOrientFront;
	D3DVECTOR			vOrientTop;
	D3DVALUE			flDistanceFactor;
	D3DVALUE			flRolloffFactor;
	D3DVALUE			flDopplerFactor;
} DS3DLISTENER, *LPDS3DLISTENER;

typedef const DS3DLISTENER *LPCDS3DLISTENER;

#define ICOM_INTERFACE IDirectSound3DListener
#define IDirectSound3DListener_METHODS \
    ICOM_METHOD1(HRESULT,GetAllParameters,  LPDS3DLISTENER,lpListener) \
    ICOM_METHOD1(HRESULT,GetDistanceFactor, LPD3DVALUE,lpflDistanceFactor) \
    ICOM_METHOD1(HRESULT,GetDopplerFactor,  LPD3DVALUE,lpflDopplerFactor) \
    ICOM_METHOD2(HRESULT,GetOrientation,    LPD3DVECTOR,lpvOrientFront, LPD3DVECTOR,lpvOrientTop) \
    ICOM_METHOD1(HRESULT,GetPosition,       LPD3DVECTOR,lpvPosition) \
    ICOM_METHOD1(HRESULT,GetRolloffFactor,  LPD3DVALUE,lpflRolloffFactor) \
    ICOM_METHOD1(HRESULT,GetVelocity,       LPD3DVECTOR,lpvVelocity) \
    ICOM_METHOD2(HRESULT,SetAllParameters,  LPCDS3DLISTENER,lpcListener, DWORD,dwApply) \
    ICOM_METHOD2(HRESULT,SetDistanceFactor, D3DVALUE,flDistanceFactor, DWORD,dwApply) \
    ICOM_METHOD2(HRESULT,SetDopplerFactor,  D3DVALUE,flDopplerFactor, DWORD,dwApply) \
    ICOM_METHOD7(HRESULT,SetOrientation,    D3DVALUE,xFront, D3DVALUE,yFront, D3DVALUE,zFront, D3DVALUE,xTop, D3DVALUE,yTop, D3DVALUE,zTop, DWORD,dwApply) \
    ICOM_METHOD4(HRESULT,SetPosition,       D3DVALUE,x, D3DVALUE,y, D3DVALUE,z, DWORD,dwApply) \
    ICOM_METHOD2(HRESULT,SetRolloffFactor,  D3DVALUE,flRolloffFactor, DWORD,dwApply) \
    ICOM_METHOD4(HRESULT,SetVelocity,       D3DVALUE,x, D3DVALUE,y, D3DVALUE,z, DWORD,dwApply) \
    ICOM_METHOD (HRESULT,CommitDeferredSettings)
#define IDirectSound3DListener_IMETHODS \
    IUnknown_IMETHODS \
    IDirectSound3DListener_METHODS
ICOM_DEFINE(IDirectSound3DListener,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IDirectSound3DListener_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirectSound3DListener_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirectSound3DListener_Release(p)            ICOM_CALL (Release,p)
/*** IDirectSound3DListener methods ***/
#define IDirectSound3DListener_GetAllParameters(p,a)           ICOM_CALL1(GetAllParameters,p,a)
#define IDirectSound3DListener_GetDistanceFactor(p,a)          ICOM_CALL1(GetDistanceFactor,p,a)
#define IDirectSound3DListener_GetDopplerFactor(p,a)           ICOM_CALL1(GetDopplerFactor,p,a)
#define IDirectSound3DListener_GetOrientation(p,a,b)           ICOM_CALL2(GetOrientation,p,a,b)
#define IDirectSound3DListener_GetPosition(p,a)                ICOM_CALL1(GetPosition,p,a)
#define IDirectSound3DListener_GetRolloffFactor(p,a)           ICOM_CALL1(GetRolloffFactor,p,a)
#define IDirectSound3DListener_GetVelocity(p,a)                ICOM_CALL1(GetVelocity,p,a)
#define IDirectSound3DListener_SetAllParameters(p,a,b)         ICOM_CALL2(SetAllParameters,p,a,b)
#define IDirectSound3DListener_SetDistanceFactor(p,a,b)        ICOM_CALL2(SetDistanceFactor,p,a,b)
#define IDirectSound3DListener_SetDopplerFactor(p,a,b)         ICOM_CALL2(SetDopplerFactor,p,a,b)
#define IDirectSound3DListener_SetOrientation(p,a,b,c,d,e,f,g) ICOM_CALL7(SetOrientation,p,a,b,c,d,e,f,g)
#define IDirectSound3DListener_SetPosition(p,a,b,c,d)          ICOM_CALL4(SetPosition,p,a,b,c,d)
#define IDirectSound3DListener_SetRolloffFactor(p,a,b)         ICOM_CALL2(SetRolloffFactor,p,a,b)
#define IDirectSound3DListener_SetVelocity(p,a,b,c,d)          ICOM_CALL4(SetVelocity,p,a,b,c,d)
#define IDirectSound3DListener_CommitDeferredSettings(p)       ICOM_CALL (CommitDeferredSettings,p)


/*****************************************************************************
 * IDirectSound3DBuffer interface
 */
typedef struct  _DS3DBUFFER {
	DWORD				dwSize;
	D3DVECTOR			vPosition;
	D3DVECTOR			vVelocity;
	DWORD				dwInsideConeAngle;
	DWORD				dwOutsideConeAngle;
	D3DVECTOR			vConeOrientation;
	LONG				lConeOutsideVolume;
	D3DVALUE			flMinDistance;
	D3DVALUE			flMaxDistance;
	DWORD				dwMode;
} DS3DBUFFER, *LPDS3DBUFFER;

typedef const DS3DBUFFER *LPCDS3DBUFFER;

#define ICOM_INTERFACE IDirectSound3DBuffer
#define IDirectSound3DBuffer_METHODS \
    ICOM_METHOD1(HRESULT,GetAllParameters,     LPDS3DBUFFER,lpDs3dBuffer) \
    ICOM_METHOD2(HRESULT,GetConeAngles,        LPDWORD,lpdwInsideConeAngle, LPDWORD,lpdwOutsideConeAngle) \
    ICOM_METHOD1(HRESULT,GetConeOrientation,   LPD3DVECTOR,lpvOrientation) \
    ICOM_METHOD1(HRESULT,GetConeOutsideVolume, LPLONG,lplConeOutsideVolume) \
    ICOM_METHOD1(HRESULT,GetMaxDistance,       LPD3DVALUE,lpflMaxDistance) \
    ICOM_METHOD1(HRESULT,GetMinDistance,       LPD3DVALUE,lpflMinDistance) \
    ICOM_METHOD1(HRESULT,GetMode,              LPDWORD,lpwdMode) \
    ICOM_METHOD1(HRESULT,GetPosition,          LPD3DVECTOR,lpvPosition) \
    ICOM_METHOD1(HRESULT,GetVelocity,          LPD3DVECTOR,lpvVelocity) \
    ICOM_METHOD2(HRESULT,SetAllParameters,     LPCDS3DBUFFER,lpcDs3dBuffer, DWORD,dwApply) \
    ICOM_METHOD3(HRESULT,SetConeAngles,        DWORD,dwInsideConeAngle, DWORD,dwOutsideConeAngle, DWORD,dwApply) \
    ICOM_METHOD4(HRESULT,SetConeOrientation,   D3DVALUE,x, D3DVALUE,y, D3DVALUE,z, DWORD,dwApply) \
    ICOM_METHOD2(HRESULT,SetConeOutsideVolume, LONG,lConeOutsideVolume, DWORD,dwApply) \
    ICOM_METHOD2(HRESULT,SetMaxDistance,       D3DVALUE,flMaxDistance, DWORD,dwApply) \
    ICOM_METHOD2(HRESULT,SetMinDistance,       D3DVALUE,flMinDistance, DWORD,dwApply) \
    ICOM_METHOD2(HRESULT,SetMode,              DWORD,dwMode, DWORD,dwApply) \
    ICOM_METHOD4(HRESULT,SetPosition,          D3DVALUE,x, D3DVALUE,y, D3DVALUE,z, DWORD,dwApply) \
    ICOM_METHOD4(HRESULT,SetVelocity,          D3DVALUE,x, D3DVALUE,y, D3DVALUE,z, DWORD,dwApply)
#define IDirectSound3DBuffer_IMETHODS \
    IUnknown_IMETHODS \
    IDirectSound3DBuffer_METHODS
ICOM_DEFINE(IDirectSound3DBuffer,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IDirectSound3DBuffer_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirectSound3DBuffer_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirectSound3DBuffer_Release(p)            ICOM_CALL (Release,p)
/*** IDirectSound3DBuffer methods ***/
#define IDirectSound3DBuffer_GetAllParameters(p,a)         ICOM_CALL1(GetAllParameters,p,a)
#define IDirectSound3DBuffer_GetConeAngles(p,a,b)          ICOM_CALL2(GetConeAngles,p,a,b)
#define IDirectSound3DBuffer_GetConeOrientation(p,a)       ICOM_CALL1(GetConeOrientation,p,a)
#define IDirectSound3DBuffer_GetConeOutsideVolume(p,a)     ICOM_CALL1(GetConeOutsideVolume,p,a)
#define IDirectSound3DBuffer_GetMaxDistance(p,a)           ICOM_CALL1(GetMaxDistance,p,a)
#define IDirectSound3DBuffer_GetMinDistance(p,a)           ICOM_CALL1(GetMinDistance,p,a)
#define IDirectSound3DBuffer_GetMode(p,a)                  ICOM_CALL1(GetMode,p,a)
#define IDirectSound3DBuffer_GetPosition(p,a)              ICOM_CALL1(GetPosition,p,a)
#define IDirectSound3DBuffer_GetVelocity(p,a)              ICOM_CALL1(GetVelocity,p,a)
#define IDirectSound3DBuffer_SetAllParameters(p,a,b)       ICOM_CALL2(SetAllParameters,p,a,b)
#define IDirectSound3DBuffer_SetConeAngles(p,a,b)          ICOM_CALL3(SetConeAngles,p,a,b)
#define IDirectSound3DBuffer_SetConeOrientation(p,a,b,c,d) ICOM_CALL4(SetConeOrientation,p,a,b,c,d)
#define IDirectSound3DBuffer_SetConeOutsideVolume(p,a,b)   ICOM_CALL2(SetConeOutsideVolume,p,a,b)
#define IDirectSound3DBuffer_SetMaxDistance(p,a,b)         ICOM_CALL2(SetMaxDistance,p,a,b)
#define IDirectSound3DBuffer_SetMinDistance(p,a,b)         ICOM_CALL2(SetMinDistance,p,a,b)
#define IDirectSound3DBuffer_SetMode(p,a,b)                ICOM_CALL2(SetMode,p,a,b)
#define IDirectSound3DBuffer_SetPosition(p,a,b,c,d)        ICOM_CALL4(SetPosition,p,a,b,c,d)
#define IDirectSound3DBuffer_SetVelocity(p,a,b,c,d)        ICOM_CALL4(SetVelocity,p,a,b,c,d)

/*****************************************************************************
 * IKsPropertySet interface
 */
#define KSPROPERTY_SUPPORT_GET	1
#define KSPROPERTY_SUPPORT_SET	2

#define ICOM_INTERFACE IKsPropertySet
#define IKsPropertySet_METHODS \
    ICOM_METHOD7(HRESULT,Get,REFGUID,rgid,ULONG,x1,LPVOID,p1,ULONG,x2,LPVOID,p2,ULONG,x3,ULONG*,px4)\
    ICOM_METHOD6(HRESULT,Set,REFGUID,rgid,ULONG,x1,LPVOID,p1,ULONG,x2,LPVOID,p2,ULONG,x3)\
    ICOM_METHOD3(HRESULT,QuerySupport,REFGUID,rgid,ULONG,x1,ULONG*,px2)
#define IKsPropertySet_IMETHODS \
        IUnknown_IMETHODS \
	IKsPropertySet_METHODS
ICOM_DEFINE(IKsPropertySet,IUnknown)
#undef ICOM_INTERFACE

#define IKsPropertySet_QueryInterface(p,a,b)	ICOM_CALL2(QueryInterface,p,a,b)
#define IKsPropertySet_AddRef(p)		ICOM_CALL (AddRef,p)
#define IKsPropertySet_Release(p)		ICOM_CALL (Release,p)
#define IKsPropertySet_Get(p,a,b,c,d,e,f,g)	ICOM_CALL7(Get,p,a,b,c,d,e,f,g)
#define IKsPropertySet_Set(p,a,b,c,d,e,f)	ICOM_CALL6(Set,p,a,b,c,d,e,f)
#define IKsPropertySet_QuerySupport(p,a,b,c)	ICOM_CALL3(QuerySupport,p,a,b,c)


/*****************************************************************************
 * IDirectSoundFullDuplex interface
 */
#define ICOM_INTERFACE IDirectSoundFullDuplex
#define IDirectSoundFullDuplex_METHODS \
    ICOM_METHOD8(HRESULT,Initialize,LPCGUID,pCaptureGuid,LPCGUID,pRendererGuid,LPCDSCBUFFERDESC,lpDscBufferDesc,LPCDSBUFFERDESC,lpDsBufferDesc,HWND,hWnd,DWORD,dwLevel,LPLPDIRECTSOUNDCAPTUREBUFFER8,lplpDirectSoundCaptureBuffer8,LPLPDIRECTSOUNDBUFFER8,lplpDirectSoundBuffer8)
#define IDirectSoundFullDuplex_IMETHODS \
        IUnknown_IMETHODS \
	IDirectSoundFullDuplex_METHODS
ICOM_DEFINE(IDirectSoundFullDuplex,IUnknown)
#undef ICOM_INTERFACE

#define IDirectSoundFullDuplex_QueryInterface(p,a,b)	ICOM_CALL2(QueryInterface,p,a,b)
#define IDirectSoundFullDuplex_AddRef(p)		ICOM_CALL (AddRef,p)
#define IDirectSoundFullDuplex_Release(p)		ICOM_CALL (Release,p)
#define IDirectSoundFullDuplex_Initialize(p,a,b,c,d,e,f,g,h)	ICOM_CALL8(Initialize,p,a,b,c,d,e,f,g,h)

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_DSOUND_H */
