/* DirectMusic Core API Stuff
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

#ifndef __WINE_DMUSIC_CORE_H
#define __WINE_DMUSIC_CORE_H

#include "objbase.h"

#include "mmsystem.h"
#include "dsound.h"

#include "dls1.h"
#include "dmerror.h"
#include "dmdls.h"
#include "dmusbuff.h"

/*#include "pshpack8.h" */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_GUID(CLSID_DirectMusic,						0x636b9f10, 0x0c7d, 0x11d1, 0x95, 0xb2, 0x00, 0x20, 0xaf, 0xdc, 0x74, 0x21);
DEFINE_GUID(CLSID_DirectMusicCollection,			0x480ff4b0, 0x28b2, 0x11d1, 0xbe, 0xf7, 0x0,  0xc0, 0x4f, 0xbf, 0x8f, 0xef);
DEFINE_GUID(CLSID_DirectMusicSynth,					0x58C2B4D0, 0x46E7, 0x11D1, 0x89, 0xAC, 0x00, 0xA0, 0xC9, 0x05, 0x41, 0x29);

#ifndef __IReferenceClock_FWD_DEFINED__
#define __IReferenceClock_FWD_DEFINED__
typedef struct IReferenceClock IReferenceClock;
#endif

DEFINE_GUID(IID_IDirectMusic,						0x6536115a,0x7b2d,0x11d2,0xba,0x18,0x00,0x00,0xf8,0x75,0xac,0x12);
typedef struct IDirectMusic IDirectMusic, *LPDIRECTMUSIC;
DEFINE_GUID(IID_IDirectMusicBuffer,					0xd2ac2878,0xb39b,0x11d1,0x87,0x4,0x0,0x60,0x8,0x93,0xb1,0xbd);
typedef struct IDirectMusicBuffer IDirectMusicBuffer, *LPDIRECTMUSICBUFFER, IDirectMusicBuffer8, *LPDIRECTMUSICBUFFER8;
DEFINE_GUID(IID_IDirectMusicPort, 					0x08f2d8c9,0x37c2,0x11d2,0xb9,0xf9,0x00,0x00,0xf8,0x75,0xac,0x12);
#define IID_IDirectMusicPort8 IID_IDirectMusicPort
typedef struct IDirectMusicPort IDirectMusicPort, *LPDIRECTMUSICPORT, IDirectMusicPort8, *LPDIRECTMUSICPORT8;
DEFINE_GUID(IID_IDirectMusicThru, 					0xced153e7,0x3606,0x11d2,0xb9,0xf9,0x00,0x00,0xf8,0x75,0xac,0x12);
#define IID_IDirectMusicThru8 IID_IDirectMusicThru
typedef struct IDirectMusicThru IDirectMusicThru, *LPDIRECTMUSICTHRU, IDirectMusicThru8, *LPDIRECTMUSICTHRU8;
DEFINE_GUID(IID_IDirectMusicPortDownload,			0xd2ac287a,0xb39b,0x11d1,0x87,0x4,0x0,0x60,0x8,0x93,0xb1,0xbd);
typedef struct IDirectMusicPortDownload IDirectMusicPortDownload, *LPDIRECTMUSICPORTDOWNLOAD, IDirectMusicPortDownload8, *LPDIRECTMUSICPORTDOWNLOAD8;
#define IID_IDirectMusicPortDownload8 IID_IDirectMusicPortDownload
DEFINE_GUID(IID_IDirectMusicDownload,				0xd2ac287b,0xb39b,0x11d1,0x87,0x4,0x0,0x60,0x8,0x93,0xb1,0xbd);
#define IID_IDirectMusicDownload8 IID_IDirectMusicDownload
typedef struct IDirectMusicDownload IDirectMusicDownload, *LPDIRECTMUSICDOWNLOAD, IDirectMusicDownload8, *LPDIRECTMUSICDOWNLOAD8;
DEFINE_GUID(IID_IDirectMusicCollection,				0xd2ac287c,0xb39b,0x11d1,0x87,0x4,0x0,0x60,0x8,0x93,0xb1,0xbd);
#define IID_IDirectMusicCollection8 IID_IDirectMusicCollection
typedef struct IDirectMusicCollection IDirectMusicCollection, *LPDIRECTMUSICCOLLECTION, IDirectMusicCollection8, *LPDIRECTMUSICCOLLECTION8;
DEFINE_GUID(IID_IDirectMusicInstrument,				0xd2ac287d,0xb39b,0x11d1,0x87,0x4,0x0,0x60,0x8,0x93,0xb1,0xbd);
#define IID_IDirectMusicInstrument8 IID_IDirectMusicInstrument
typedef struct IDirectMusicInstrument IDirectMusicInstrument, *LPDIRECTMUSICINSTRUMENT, IDirectMusicInstrument8, *LPDIRECTMUSICINSTRUMENT8;
DEFINE_GUID(IID_IDirectMusicDownloadedInstrument,	0xd2ac287e,0xb39b,0x11d1,0x87,0x4,0x0,0x60,0x8,0x93,0xb1,0xbd);
#define IID_IDirectMusicDownloadedInstrument8 IID_IDirectMusicDownloadedInstrument
typedef struct IDirectMusicDownloadedInstrument IDirectMusicDownloadedInstrument, *LPDIRECTMUSICDOWNLOADEDINSTRUMENT, IDirectMusicDownloadedInstrument8, *LPDIRECTMUSICDOWNLOADEDINSTRUMENT8;
DEFINE_GUID(IID_IDirectMusic2,						0x6fc2cae1,0xbc78,0x11d2,0xaf,0xa6,0x0,0xaa,0x0,0x24,0xd8,0xb6);
DEFINE_GUID(IID_IDirectMusic8,						0x2d3629f7,0x813d,0x4939,0x85,0x08,0xf0,0x5c,0x6b,0x75,0xfd,0x97);
typedef struct IDirectMusic8 IDirectMusic8, *LPDIRECTMUSIC8;

DEFINE_GUID(GUID_DMUS_PROP_GM_Hardware, 			0x178f2f24,0xc364,0x11d1,0xa7,0x60,0x00,0x00,0xf8,0x75,0xac,0x12);
DEFINE_GUID(GUID_DMUS_PROP_GS_Hardware, 			0x178f2f25,0xc364,0x11d1,0xa7,0x60,0x00,0x00,0xf8,0x75,0xac,0x12);
DEFINE_GUID(GUID_DMUS_PROP_XG_Hardware, 			0x178f2f26,0xc364,0x11d1,0xa7,0x60,0x00,0x00,0xf8,0x75,0xac,0x12);
DEFINE_GUID(GUID_DMUS_PROP_XG_Capable,  			0x6496aba1,0x61b0,0x11d2,0xaf,0xa6,0x0,0xaa,0x0,0x24,0xd8,0xb6);
DEFINE_GUID(GUID_DMUS_PROP_GS_Capable,  			0x6496aba2,0x61b0,0x11d2,0xaf,0xa6,0x0,0xaa,0x0,0x24,0xd8,0xb6);
DEFINE_GUID(GUID_DMUS_PROP_DLS1,        			0x178f2f27,0xc364,0x11d1,0xa7,0x60,0x00,0x00,0xf8,0x75,0xac,0x12);
DEFINE_GUID(GUID_DMUS_PROP_DLS2,        			0xf14599e5,0x4689,0x11d2,0xaf,0xa6,0x0,0xaa,0x0,0x24,0xd8,0xb6);
DEFINE_GUID(GUID_DMUS_PROP_INSTRUMENT2, 			0x865fd372,0x9f67,0x11d2,0x87,0x2a,0x0,0x60,0x8,0x93,0xb1,0xbd);
DEFINE_GUID(GUID_DMUS_PROP_SynthSink_DSOUND,		0xaa97844,0xc877,0x11d1,0x87,0xc,0x0,0x60,0x8,0x93,0xb1,0xbd);
DEFINE_GUID(GUID_DMUS_PROP_SynthSink_WAVE,			0xaa97845,0xc877,0x11d1,0x87,0xc,0x0,0x60,0x8,0x93,0xb1,0xbd);
DEFINE_GUID(GUID_DMUS_PROP_SampleMemorySize, 		0x178f2f28,0xc364,0x11d1,0xa7,0x60,0x00,0x00,0xf8,0x75,0xac, 0x12);
DEFINE_GUID(GUID_DMUS_PROP_SamplePlaybackRate, 		0x2a91f713,0xa4bf,0x11d2,0xbb,0xdf,0x0,0x60,0x8,0x33,0xdb,0xd8);
DEFINE_GUID(GUID_DMUS_PROP_WriteLatency,			0x268a0fa0,0x60f2,0x11d2,0xaf,0xa6,0x0,0xaa,0x0,0x24,0xd8,0xb6);
DEFINE_GUID(GUID_DMUS_PROP_WritePeriod,				0x268a0fa1,0x60f2,0x11d2,0xaf,0xa6,0x0,0xaa,0x0,0x24,0xd8,0xb6);
DEFINE_GUID(GUID_DMUS_PROP_MemorySize,  			0x178f2f28,0xc364,0x11d1,0xa7,0x60,0x00,0x00,0xf8, 0x75, 0xac, 0x12);
DEFINE_GUID(GUID_DMUS_PROP_WavesReverb,				0x4cb5622,0x32e5,0x11d2,0xaf,0xa6,0x0,0xaa,0x0,0x24,0xd8,0xb6);
DEFINE_GUID(GUID_DMUS_PROP_Effects, 				0xcda8d611,0x684a,0x11d2,0x87,0x1e,0x0,0x60,0x8,0x93,0xb1,0xbd);
DEFINE_GUID(GUID_DMUS_PROP_LegacyCaps,				0xcfa7cdc2,0x00a1,0x11d2,0xaa,0xd5,0x00,0x00,0xf8,0x75,0xac,0x12);
DEFINE_GUID(GUID_DMUS_PROP_Volume, 					0xfedfae25L,0xe46e,0x11d1,0xaa,0xce,0x00,0x00,0xf8,0x75,0xac,0x12);

typedef ULONGLONG    SAMPLE_TIME, *LPSAMPLE_TIME;
typedef ULONGLONG    SAMPLE_POSITION;

#define DMUS_MAX_DESCRIPTION 128
#define DMUS_MAX_DRIVER 128

typedef struct _DMUS_BUFFERDESC
{
    DWORD dwSize;
    DWORD dwFlags;
    GUID guidBufferFormat;
    DWORD cbBuffer;
} DMUS_BUFFERDESC, *LPDMUS_BUFFERDESC;

#define DMUS_EFFECT_NONE             0x00000000
#define DMUS_EFFECT_REVERB           0x00000001
#define DMUS_EFFECT_CHORUS           0x00000002
#define DMUS_EFFECT_DELAY            0x00000004
 
#define DMUS_PC_INPUTCLASS       (0)
#define DMUS_PC_OUTPUTCLASS      (1)

#define DMUS_PC_DLS              (0x00000001)
#define DMUS_PC_EXTERNAL         (0x00000002)
#define DMUS_PC_SOFTWARESYNTH    (0x00000004)
#define DMUS_PC_MEMORYSIZEFIXED  (0x00000008)
#define DMUS_PC_GMINHARDWARE     (0x00000010)
#define DMUS_PC_GSINHARDWARE     (0x00000020)
#define DMUS_PC_XGINHARDWARE     (0x00000040)
#define DMUS_PC_DIRECTSOUND      (0x00000080)
#define DMUS_PC_SHAREABLE        (0x00000100)
#define DMUS_PC_DLS2             (0x00000200)
#define DMUS_PC_AUDIOPATH        (0x00000400)
#define DMUS_PC_WAVE             (0x00000800)

#define DMUS_PC_SYSTEMMEMORY     (0x7FFFFFFF)

typedef struct _DMUS_PORTCAPS
{
    DWORD   dwSize;
    DWORD   dwFlags;
    GUID    guidPort;
    DWORD   dwClass;
    DWORD   dwType;
    DWORD   dwMemorySize;
    DWORD   dwMaxChannelGroups;
    DWORD   dwMaxVoices;    
    DWORD   dwMaxAudioChannels;
    DWORD   dwEffectFlags;
    WCHAR   wszDescription[DMUS_MAX_DESCRIPTION];
} DMUS_PORTCAPS, *LPDMUS_PORTCAPS;

#define DMUS_PORT_WINMM_DRIVER      (0)
#define DMUS_PORT_USER_MODE_SYNTH   (1)
#define DMUS_PORT_KERNEL_MODE       (2)

#define DMUS_PORTPARAMS_VOICES           0x00000001
#define DMUS_PORTPARAMS_CHANNELGROUPS    0x00000002
#define DMUS_PORTPARAMS_AUDIOCHANNELS    0x00000004
#define DMUS_PORTPARAMS_SAMPLERATE       0x00000008
#define DMUS_PORTPARAMS_EFFECTS          0x00000020
#define DMUS_PORTPARAMS_SHARE            0x00000040
#define DMUS_PORTPARAMS_FEATURES         0x00000080

typedef struct _DMUS_PORTPARAMS
{
    DWORD   dwSize;
    DWORD   dwValidParams;
    DWORD   dwVoices;
    DWORD   dwChannelGroups;
    DWORD   dwAudioChannels;
    DWORD   dwSampleRate;
    DWORD   dwEffectFlags;
    BOOL    fShare;
} DMUS_PORTPARAMS7;

typedef struct _DMUS_PORTPARAMS8
{
    DWORD   dwSize;
    DWORD   dwValidParams;
    DWORD   dwVoices;
    DWORD   dwChannelGroups;
    DWORD   dwAudioChannels;
    DWORD   dwSampleRate;
    DWORD   dwEffectFlags;
    BOOL    fShare;
    DWORD   dwFeatures;
} DMUS_PORTPARAMS8, DMUS_PORTPARAMS, *LPDMUS_PORTPARAMS;

#define DMUS_PORT_FEATURE_AUDIOPATH     0x00000001
#define DMUS_PORT_FEATURE_STREAMING     0x00000002

typedef struct _DMUS_SYNTHSTATS
{
    DWORD   dwSize;
    DWORD   dwValidStats;
    DWORD   dwVoices;
    DWORD   dwTotalCPU;
    DWORD   dwCPUPerVoice;
    DWORD   dwLostNotes;
    DWORD   dwFreeMemory;
    long    lPeakVolume;
} DMUS_SYNTHSTATS, *LPDMUS_SYNTHSTATS;

typedef struct _DMUS_SYNTHSTATS8
{
    DWORD   dwSize;
    DWORD   dwValidStats;
    DWORD   dwVoices;
    DWORD   dwTotalCPU;
    DWORD   dwCPUPerVoice;
    DWORD   dwLostNotes;
    DWORD   dwFreeMemory;
    long    lPeakVolume;
	DWORD   dwSynthMemUse;
} DMUS_SYNTHSTATS8, *LPDMUS_SYNTHSTATS8;

#define DMUS_SYNTHSTATS_VOICES          1
#define DMUS_SYNTHSTATS_TOTAL_CPU       2
#define DMUS_SYNTHSTATS_CPU_PER_VOICE   4
#define DMUS_SYNTHSTATS_LOST_NOTES      8
#define DMUS_SYNTHSTATS_PEAK_VOLUME     16
#define DMUS_SYNTHSTATS_FREE_MEMORY     32

#define DMUS_SYNTHSTATS_SYSTEMMEMORY    DMUS_PC_SYSTEMMEMORY

typedef struct _DMUS_WAVES_REVERB_PARAMS
{
    float   fInGain;
    float   fReverbMix;
    float   fReverbTime;
    float   fHighFreqRTRatio;
} DMUS_WAVES_REVERB_PARAMS;

typedef enum
{
    DMUS_CLOCK_SYSTEM = 0,
    DMUS_CLOCK_WAVE = 1
} DMUS_CLOCKTYPE;

#define DMUS_CLOCKF_GLOBAL              0x00000001

typedef struct _DMUS_CLOCKINFO7
{
    DWORD           dwSize;
    DMUS_CLOCKTYPE  ctType;
    GUID            guidClock;
    WCHAR           wszDescription[DMUS_MAX_DESCRIPTION];
} DMUS_CLOCKINFO7, *LPDMUS_CLOCKINFO7;

typedef struct _DMUS_CLOCKINFO8
{
    DWORD           dwSize;
    DMUS_CLOCKTYPE  ctType;
    GUID            guidClock;
    WCHAR           wszDescription[DMUS_MAX_DESCRIPTION];
    DWORD           dwFlags;           
} DMUS_CLOCKINFO8, *LPDMUS_CLOCKINFO8, DMUS_CLOCKINFO, *LPDMUS_CLOCKINFO;

#define DSBUSID_FIRST_SPKR_LOC              0
#define DSBUSID_FRONT_LEFT                  0
#define DSBUSID_LEFT                        0
#define DSBUSID_FRONT_RIGHT                 1
#define DSBUSID_RIGHT                       1
#define DSBUSID_FRONT_CENTER                2
#define DSBUSID_LOW_FREQUENCY               3
#define DSBUSID_BACK_LEFT                   4
#define DSBUSID_BACK_RIGHT                  5
#define DSBUSID_FRONT_LEFT_OF_CENTER        6 
#define DSBUSID_FRONT_RIGHT_OF_CENTER       7
#define DSBUSID_BACK_CENTER                 8
#define DSBUSID_SIDE_LEFT                   9
#define DSBUSID_SIDE_RIGHT                 10
#define DSBUSID_TOP_CENTER                 11
#define DSBUSID_TOP_FRONT_LEFT             12
#define DSBUSID_TOP_FRONT_CENTER           13
#define DSBUSID_TOP_FRONT_RIGHT            14
#define DSBUSID_TOP_BACK_LEFT              15
#define DSBUSID_TOP_BACK_CENTER            16
#define DSBUSID_TOP_BACK_RIGHT             17
#define DSBUSID_LAST_SPKR_LOC              17
#define DSBUSID_IS_SPKR_LOC(id) ( ((id) >= DSBUSID_FIRST_SPKR_LOC) && ((id) <= DSBUSID_LAST_SPKR_LOC) )
#define DSBUSID_REVERB_SEND                64
#define DSBUSID_CHORUS_SEND                65
#define DSBUSID_DYNAMIC_0                 512 
#define DSBUSID_NULL			   0xFFFFFFFF


/*****************************************************************************
 * IDirectMusic interface
 */
#undef INTERFACE
#define INTERFACE IDirectMusic
#define IDirectMusic_METHODS \
    IUnknown_METHODS \
    STDMETHOD(EnumPort)(THIS_ DWORD dwIndex, LPDMUS_PORTCAPS pPortCaps) PURE; \
    STDMETHOD(CreateMusicBuffer)(THIS_ LPDMUS_BUFFERDESC pBufferDesc, LPDIRECTMUSICBUFFER **ppBuffer, LPUNKNOWN pUnkOuter) PURE; \
    STDMETHOD(CreatePort)(THIS_ REFCLSID rclsidPort, LPDMUS_PORTPARAMS pPortParams, LPDIRECTMUSICPORT *ppPort, LPUNKNOWN pUnkOuter) PURE; \
    STDMETHOD(EnumMasterClock)(THIS_ DWORD dwIndex, LPDMUS_CLOCKINFO lpClockInfo) PURE; \
    STDMETHOD(GetMasterClock)(THIS_ LPGUID pguidClock, IReferenceClock **ppReferenceClock) PURE; \
    STDMETHOD(SetMasterClock)(THIS_ REFGUID rguidClock) PURE; \
    STDMETHOD(Activate)(THIS_ BOOL fEnable) PURE; \
    STDMETHOD(GetDefaultPort)(THIS_ LPGUID pguidPort) PURE; \
    STDMETHOD(SetDirectSound)(THIS_ LPDIRECTSOUND pDirectSound, HWND hWnd) PURE;
ICOM_DEFINE(IDirectMusic,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IDirectMusic_QueryInterface(p,a,b)          (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectMusic_AddRef(p)                      (p)->lpVtbl->AddRef(p)
#define IDirectMusic_Release(p)                     (p)->lpVtbl->Release(p)
/*** IDirectMusic methods ***/
#define IDirectMusic_EnumPort(p,a,b)                (p)->lpVtbl->EnumPort(p,a,b)
#define IDirectMusic_CreateMusicBuffer(p,a,b,c)     (p)->lpVtbl->CreateMusicBuffer(p,a,b,c)
#define IDirectMusic_CreatePort(p,a,b,c,d)          (p)->lpVtbl->CreatePort(p,a,b,c,d)
#define IDirectMusic_EnumMasterClock(p,a,b)         (p)->lpVtbl->EnumMasterClock(p,a,b)
#define IDirectMusic_GetMasterClock(p,a,b)          (p)->lpVtbl->GetMasterClock(p,a,b)
#define IDirectMusic_SetMasterClock(p,a)            (p)->lpVtbl->SetMasterClock(p,a)
#define IDirectMusic_Activate(p,a)                  (p)->lpVtbl->Activate(p,a)
#define IDirectMusic_GetDefaultPort(p,a)            (p)->lpVtbl->GetDefaultPort(p,a)
#define IDirectMusic_SetDirectSound(p,a,b)          (p)->lpVtbl->SetDirectSound(p,a,b)
#endif


/*****************************************************************************
 * IDirectMusic8 interface
 */
#undef INTERFACE
#define INTERFACE IDirectMusic8
#define IDirectMusic8_METHODS \
    IDirectMusic_METHODS \
    STDMETHOD(SetExternalMasterClock)(THIS_ IReferenceClock *pClock) PURE;
ICOM_DEFINE(IDirectMusic8,IDirectMusic)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IDirectMusic8_QueryInterface(p,a,b)         (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectMusic8_AddRef(p)                     (p)->lpVtbl->AddRef(p)
#define IDirectMusic8_Release(p)                    (p)->lpVtbl->Release(p)
/*** IDirectMusic methods ***/
#define IDirectMusic8_EnumPort(p,a,b)               (p)->lpVtbl->EnumPort(p,a,b)
#define IDirectMusic8_CreateMusicBuffer(p,a,b,c)    (p)->lpVtbl->CreateMusicBuffer(p,a,b,c)
#define IDirectMusic8_CreatePort(p,a,b,c,d)         (p)->lpVtbl->CreatePort(p,a,b,c,d)
#define IDirectMusic8_EnumMasterClock(p,a,b)        (p)->lpVtbl->EnumMasterClock(p,a,b)
#define IDirectMusic8_GetMasterClock(p,a,b)         (p)->lpVtbl->GetMasterClock(p,a,b)
#define IDirectMusic8_SetMasterClock(p,a)           (p)->lpVtbl->SetMasterClock(p,a)
#define IDirectMusic8_Activate(p,a)                 (p)->lpVtbl->Activate(p,a)
#define IDirectMusic8_GetDefaultPort(p,a)           (p)->lpVtbl->GetDefaultPort(p,a)
#define IDirectMusic8_SetDirectSound(p,a,b)         (p)->lpVtbl->SetDirectSound(p,a,b)
/*** IDirectMusic8 methods ***/
#define IDirectMusic8_SetExternalMasterClock(p,a)   (p)->lpVtbl->SetExternalMasterClock(p,a)
#endif


/*****************************************************************************
 * IDirectMusicBuffer interface
 */
#undef INTERFACE
#define INTERFACE IDirectMusicBuffer
#define IDirectMusicBuffer_METHODS \
    IUnknown_METHODS \
    STDMETHOD(Flush)(THIS) PURE; \
    STDMETHOD(TotalTime)(THIS_ LPREFERENCE_TIME prtTime) PURE; \
    STDMETHOD(PackStructured)(THIS_ REFERENCE_TIME rt, DWORD dwChannelGroup, DWORD dwChannelMessage) PURE; \
    STDMETHOD(PackUnstructured)(THIS_ REFERENCE_TIME rt, DWORD dwChannelGroup, DWORD cb, LPBYTE lpb) PURE; \
    STDMETHOD(ResetReadPtr)(THIS) PURE; \
    STDMETHOD(GetNextEvent)(THIS_ LPREFERENCE_TIME prt, LPDWORD pdwChannelGroup, LPDWORD pdwLength, LPBYTE *ppData) PURE; \
    STDMETHOD(GetRawBufferPtr)(THIS_ LPBYTE *ppData) PURE; \
    STDMETHOD(GetStartTime)(THIS_ LPREFERENCE_TIME prt) PURE; \
    STDMETHOD(GetUsedBytes)(THIS_ LPDWORD pcb) PURE; \
    STDMETHOD(GetMaxBytes)(THIS_ LPDWORD pcb) PURE; \
    STDMETHOD(GetBufferFormat)(THIS_ LPGUID pGuidFormat) PURE; \
    STDMETHOD(SetStartTime)(THIS_ REFERENCE_TIME rt) PURE; \
    STDMETHOD(SetUsedBytes)(THIS_ DWORD cb) PURE;
ICOM_DEFINE(IDirectMusicBuffer,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IDirectMusicBuffer_QueryInterface(p,a,b)            (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectMusicBuffer_AddRef(p)                        (p)->lpVtbl->AddRef(p)
#define IDirectMusicBuffer_Release(p)                       (p)->lpVtbl->Release(p)
/*** IDirectMusicBuffer methods ***/
#define IDirectMusicBuffer_Flush(p)                         (p)->lpVtbl->Flush(p)
#define IDirectMusicBuffer_TotalTime(p,a)                   (p)->lpVtbl->TotalTime(p,a)
#define IDirectMusicBuffer_PackStructured(p,a,b,c)          (p)->lpVtbl->PackStructured(p,a,b,c)
#define IDirectMusicBuffer_PackUnstructured(p,a,b,c,d)      (p)->lpVtbl->PackUnstructured(p,a,b,c,d)
#define IDirectMusicBuffer_ResetReadPtr(p)                  (p)->lpVtbl->ResetReadPtr(p)
#define IDirectMusicBuffer_GetNextEvent(p,a,b,c,d)          (p)->lpVtbl->GetNextEvent(p,a,b,c,d)
#define IDirectMusicBuffer_GetRawBufferPtr(p,a)             (p)->lpVtbl->GetRawBufferPtr(p,a)
#define IDirectMusicBuffer_GetStartTime(p,a)                (p)->lpVtbl->GetStartTime(p,a)
#define IDirectMusicBuffer_GetUsedBytes(p,a)                (p)->lpVtbl->GetUsedBytes(p,a)
#define IDirectMusicBuffer_GetMaxBytes(p,a)                 (p)->lpVtbl->GetMaxBytes(p,a)
#define IDirectMusicBuffer_GetBufferFormat(p,a)             (p)->lpVtbl->GetBufferFormat(p,a)
#define IDirectMusicBuffer_SetStartTime(p,a)                (p)->lpVtbl->SetStartTime(p,a)
#define IDirectMusicBuffer_SetUsedBytes(p,a)                (p)->lpVtbl->SetUsedBytes(p,a)
#endif

/*****************************************************************************
 * IDirectMusicInstrument interface
 */
#undef  INTERFACE
#define INTERFACE IDirectMusicInstrument
#define IDirectMusicInstrument_METHODS \
    IUnknown_METHODS \
    STDMETHOD(GetPatch)(THIS_ DWORD *pdwPatch) PURE; \
    STDMETHOD(SetPatch)(THIS_ DWORD dwPatch) PURE;
ICOM_DEFINE(IDirectMusicInstrument,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IDirectMusicInstrument_QueryInterface(p,a,b)            (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectMusicInstrument_AddRef(p)                        (p)->lpVtbl->AddRef(p)
#define IDirectMusicInstrument_Release(p)                       (p)->lpVtbl->Release(p)
/*** IDirectMusicInstrument methods ***/
#define IDirectMusicInstrument_GetPatch(p,a)                    (p)->lpVtbl->GetPatch(p,a)
#define IDirectMusicInstrument_SetPatch(p,a)                    (p)->lpVtbl->SetPatch(p,a)
#endif


/*****************************************************************************
 * IDirectMusicDownloadedInstrument interface
 */
#undef  INTERFACE
#define INTERFACE IDirectMusicDownloadedInstrument
#define IDirectMusicDownloadedInstrument_METHODS \
    IUnknown_METHODS \
    /* no IDirectMusicDownloadedInstrument methods at this time */
ICOM_DEFINE(IDirectMusicDownloadedInstrument,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IDirectMusicDownloadedInstrument_QueryInterface(p,a,b)          (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectMusicDownloadedInstrument_AddRef(p)                      (p)->lpVtbl->AddRef(p)
#define IDirectMusicDownloadedInstrument_Release(p)                     (p)->lpVtbl->Release(p)
/*** IDirectMusicDownloadedInstrument methods ***/
/* none at this time */
#endif


/*****************************************************************************
 * IDirectMusicCollection interface
 */
#undef  INTERFACE
#define INTERFACE IDirectMusicCollection
#define IDirectMusicCollection_METHODS \
    IUnknown_METHODS \
    STDMETHOD(GetInstrument)(THIS_ DWORD dwPatch, IDirectMusicInstrument **ppInstrument) PURE; \
    STDMETHOD(EnumInstrument)(THIS_ DWORD dwIndex, DWORD *pdwPatch, LPWSTR pwszName, DWORD dwNameLen) PURE;
ICOM_DEFINE(IDirectMusicCollection,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IDirectMusicCollection_QueryInterface(p,a,b)            (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectMusicCollection_AddRef(p)                        (p)->lpVtbl->AddRef(p)
#define IDirectMusicCollection_Release(p)                       (p)->lpVtbl->Release(p)
/*** IDirectMusicCollection methods ***/
#define IDirectMusicCollection_GetInstrument(p,a,b)             (p)->lpVtbl->GetInstrument(p,a,b)
#define IDirectMusicCollection_EnumInstrument(p,a,b,c)          (p)->lpVtbl->EnumInstrument(p,a,b,c)
#endif


/*****************************************************************************
 * IDirectMusicDownload interface
 */
#undef  INTERFACE
#define INTERFACE IDirectMusicDownload
#define IDirectMusicDownload_METHODS \
    IUnknown_METHODS \
    STDMETHOD(GetBuffer)(THIS_ void **ppvBuffer, DWORD *pdwSize) PURE;
ICOM_DEFINE(IDirectMusicDownload,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IDirectMusicDownload_QueryInterface(p,a,b)          (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectMusicDownload_AddRef(p)                      (p)->lpVtbl->AddRef(p)
#define IDirectMusicDownload_Release(p)                     (p)->lpVtbl->Release(p)
/*** IDirectMusicDownload methods ***/
#define IDirectMusicDownload_GetBuffer(p,a,b)               (p)->lpVtbl->GetBuffer(p,a,b)
#endif


/*****************************************************************************
 * IDirectMusicPortDownload interface
 */
#undef  INTERFACE
#define INTERFACE IDirectMusicPortDownload
#define IDirectMusicPortDownload_METHODS \
    IUnknown_METHODS \
    STDMETHOD(GetBuffer)(THIS_ DWORD dwDLId, IDirectMusicDownload **ppIDMDownload) PURE; \
    STDMETHOD(AllocateBuffer)(THIS_ DWORD dwSize, IDirectMusicDownload **ppIDMDownload) PURE; \
    STDMETHOD(GetDLId)(THIS_ DWORD *pdwStartDLId, DWORD dwCount) PURE; \
    STDMETHOD(GetAppend)(THIS_ DWORD *pdwAppend) PURE; \
    STDMETHOD(Download)(THIS_ IDirectMusicDownload *pIDMDownload) PURE; \
    STDMETHOD(Unload)(THIS_ IDirectMusicDownload *pIDMDownload) PURE;
ICOM_DEFINE(IDirectMusicPortDownload,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IDirectMusicPortDownload_QueryInterface(p,a,b)          (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectMusicPortDownload_AddRef(p)                      (p)->lpVtbl->AddRef(p)
#define IDirectMusicPortDownload_Release(p)                     (p)->lpVtbl->Release(p)
/*** IDirectMusicPortDownload methods ***/
#define IDirectMusicPortDownload_GetBuffer(p,a,b)               (p)->lpVtbl->GetBuffer(p,a,b)
#define IDirectMusicPortDownload_AllocateBuffer(p,a,b)          (p)->lpVtbl->AllocateBuffer(p,a,b)
#define IDirectMusicPortDownload_GetDLId(p,a,b)                 (p)->lpVtbl->GetDLId(p,a,b)
#define IDirectMusicPortDownload_GetAppend(p,a)                 (p)->lpVtbl->GetAppend(p,a)
#define IDirectMusicPortDownload_Download(p,a)                  (p)->lpVtbl->Download(p,a)
#define IDirectMusicPortDownload_Unload(p,a)                    (p)->lpVtbl->GetBuffer(p,a)
#endif


#ifndef __WINE_DIRECTAUDIO_PRIORITIES_DEFINED
#define __WINE_DIRECTAUDIO_PRIORITIES_DEFINED

#define DAUD_CRITICAL_VOICE_PRIORITY    (0xF0000000)
#define DAUD_HIGH_VOICE_PRIORITY        (0xC0000000)
#define DAUD_STANDARD_VOICE_PRIORITY    (0x80000000)
#define DAUD_LOW_VOICE_PRIORITY         (0x40000000)
#define DAUD_PERSIST_VOICE_PRIORITY     (0x10000000) 

#define DAUD_CHAN1_VOICE_PRIORITY_OFFSET    (0x0000000E)
#define DAUD_CHAN2_VOICE_PRIORITY_OFFSET    (0x0000000D)
#define DAUD_CHAN3_VOICE_PRIORITY_OFFSET    (0x0000000C)
#define DAUD_CHAN4_VOICE_PRIORITY_OFFSET    (0x0000000B)
#define DAUD_CHAN5_VOICE_PRIORITY_OFFSET    (0x0000000A)
#define DAUD_CHAN6_VOICE_PRIORITY_OFFSET    (0x00000009)
#define DAUD_CHAN7_VOICE_PRIORITY_OFFSET    (0x00000008)
#define DAUD_CHAN8_VOICE_PRIORITY_OFFSET    (0x00000007)
#define DAUD_CHAN9_VOICE_PRIORITY_OFFSET    (0x00000006)
#define DAUD_CHAN10_VOICE_PRIORITY_OFFSET   (0x0000000F)
#define DAUD_CHAN11_VOICE_PRIORITY_OFFSET   (0x00000005)
#define DAUD_CHAN12_VOICE_PRIORITY_OFFSET   (0x00000004)
#define DAUD_CHAN13_VOICE_PRIORITY_OFFSET   (0x00000003)
#define DAUD_CHAN14_VOICE_PRIORITY_OFFSET   (0x00000002)
#define DAUD_CHAN15_VOICE_PRIORITY_OFFSET   (0x00000001)
#define DAUD_CHAN16_VOICE_PRIORITY_OFFSET   (0x00000000)
 
#define DAUD_CHAN1_DEF_VOICE_PRIORITY   (DAUD_STANDARD_VOICE_PRIORITY | DAUD_CHAN1_VOICE_PRIORITY_OFFSET)
#define DAUD_CHAN2_DEF_VOICE_PRIORITY   (DAUD_STANDARD_VOICE_PRIORITY | DAUD_CHAN2_VOICE_PRIORITY_OFFSET)
#define DAUD_CHAN3_DEF_VOICE_PRIORITY   (DAUD_STANDARD_VOICE_PRIORITY | DAUD_CHAN3_VOICE_PRIORITY_OFFSET)
#define DAUD_CHAN4_DEF_VOICE_PRIORITY   (DAUD_STANDARD_VOICE_PRIORITY | DAUD_CHAN4_VOICE_PRIORITY_OFFSET)
#define DAUD_CHAN5_DEF_VOICE_PRIORITY   (DAUD_STANDARD_VOICE_PRIORITY | DAUD_CHAN5_VOICE_PRIORITY_OFFSET)
#define DAUD_CHAN6_DEF_VOICE_PRIORITY   (DAUD_STANDARD_VOICE_PRIORITY | DAUD_CHAN6_VOICE_PRIORITY_OFFSET)
#define DAUD_CHAN7_DEF_VOICE_PRIORITY   (DAUD_STANDARD_VOICE_PRIORITY | DAUD_CHAN7_VOICE_PRIORITY_OFFSET)
#define DAUD_CHAN8_DEF_VOICE_PRIORITY   (DAUD_STANDARD_VOICE_PRIORITY | DAUD_CHAN8_VOICE_PRIORITY_OFFSET)
#define DAUD_CHAN9_DEF_VOICE_PRIORITY   (DAUD_STANDARD_VOICE_PRIORITY | DAUD_CHAN9_VOICE_PRIORITY_OFFSET)
#define DAUD_CHAN10_DEF_VOICE_PRIORITY  (DAUD_STANDARD_VOICE_PRIORITY | DAUD_CHAN10_VOICE_PRIORITY_OFFSET)
#define DAUD_CHAN11_DEF_VOICE_PRIORITY  (DAUD_STANDARD_VOICE_PRIORITY | DAUD_CHAN11_VOICE_PRIORITY_OFFSET)
#define DAUD_CHAN12_DEF_VOICE_PRIORITY  (DAUD_STANDARD_VOICE_PRIORITY | DAUD_CHAN12_VOICE_PRIORITY_OFFSET)
#define DAUD_CHAN13_DEF_VOICE_PRIORITY  (DAUD_STANDARD_VOICE_PRIORITY | DAUD_CHAN13_VOICE_PRIORITY_OFFSET)
#define DAUD_CHAN14_DEF_VOICE_PRIORITY  (DAUD_STANDARD_VOICE_PRIORITY | DAUD_CHAN14_VOICE_PRIORITY_OFFSET)
#define DAUD_CHAN15_DEF_VOICE_PRIORITY  (DAUD_STANDARD_VOICE_PRIORITY | DAUD_CHAN15_VOICE_PRIORITY_OFFSET)
#define DAUD_CHAN16_DEF_VOICE_PRIORITY  (DAUD_STANDARD_VOICE_PRIORITY | DAUD_CHAN16_VOICE_PRIORITY_OFFSET)

#endif  /* __WINE_DIRECTAUDIO_PRIORITIES_DEFINED */


/*****************************************************************************
 * IDirectMusicPort interface
 */
#undef  INTERFACE
#define INTERFACE IDirectMusicPort
#define IDirectMusicPort_METHODS \
    IUnknown_METHODS \
    STDMETHOD(PlayBuffer)(THIS_ LPDIRECTMUSICBUFFER pBuffer) PURE; \
    STDMETHOD(SetReadNotificationHandle)(THIS_ HANDLE hEvent) PURE; \
    STDMETHOD(Read)(THIS_ LPDIRECTMUSICBUFFER pBuffer) PURE; \
    STDMETHOD(DownloadInstrument)(THIS_ IDirectMusicInstrument *pInstrument, IDirectMusicDownloadedInstrument **ppDownloadedInstrument, DMUS_NOTERANGE *pNoteRanges, DWORD dwNumNoteRanges) PURE; \
    STDMETHOD(UnloadInstrument)(THIS_ IDirectMusicDownloadedInstrument *pDownloadedInstrument) PURE; \
    STDMETHOD(GetLatencyClock)(THIS_ IReferenceClock **ppClock) PURE; \
    STDMETHOD(GetRunningStats)(THIS_ LPDMUS_SYNTHSTATS pStats) PURE; \
    STDMETHOD(GetCaps)(THIS_ LPDMUS_PORTCAPS pPortCaps) PURE; \
    STDMETHOD(DeviceIoControl)(THIS_ DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) PURE; \
    STDMETHOD(SetNumChannelGroups)(THIS_ DWORD dwChannelGroups) PURE; \
    STDMETHOD(GetNumChannelGroups)(THIS_ LPDWORD pdwChannelGroups) PURE; \
    STDMETHOD(Activate)(THIS_ BOOL fActive) PURE; \
    STDMETHOD(SetChannelPriority)(THIS_ DWORD dwChannelGroup, DWORD dwChannel, DWORD dwPriority) PURE; \
    STDMETHOD(GetChannelPriority)(THIS_ DWORD dwChannelGroup, DWORD dwChannel, LPDWORD pdwPriority) PURE; \
    STDMETHOD(SetDirectSound)(THIS_ LPDIRECTSOUND pDirectSound, LPDIRECTSOUNDBUFFER pDirectSoundBuffer) PURE; \
    STDMETHOD(GetFormat)(THIS_ LPWAVEFORMATEX pWaveFormatEx, LPDWORD pdwWaveFormatExSize, LPDWORD pdwBufferSize) PURE;
ICOM_DEFINE(IDirectMusicPort,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IDirectMusicPort_QueryInterface(p,a,b)                  (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectMusicPort_AddRef(p)                              (p)->lpVtbl->AddRef(p)
#define IDirectMusicPort_Release(p)                             (p)->lpVtbl->Release(p)
/*** IDirectMusicPort methods ***/
#define IDirectMusicPort_PlayBuffer(p,a)                        (p)->lpVtbl->PlayBuffer(p,a)
#define IDirectMusicPort_SetReadNotificationHandle(p,a)         (p)->lpVtbl->SetReadNotificationHandle(p,a)
#define IDirectMusicPort_Read(p,a)                              (p)->lpVtbl->Read(p,a)
#define IDirectMusicPort_DownloadInstrument(p,a,b,c,d)          (p)->lpVtbl->DownloadInstrument(p,a,b,c,d)
#define IDirectMusicPort_UnloadInstrument(p,a)                  (p)->lpVtbl->UnloadInstrument(p,a)
#define IDirectMusicPort_GetLatencyClock(p,a)                   (p)->lpVtbl->GetLatencyClock(p,a)
#define IDirectMusicPort_GetRunningStats(p,a)                   (p)->lpVtbl->GetRunningStats(p,a)
#define IDirectMusicPort_GetCaps(p,a)                           (p)->lpVtbl->GetCaps(p,a)
#define IDirectMusicPort_DeviceIoControl(p,a,b,c,d,e,f,g)       (p)->lpVtbl->DeviceIoControl(p,a,b,c,d,e,f,g)
#define IDirectMusicPort_SetNumChannelGroups(p,a)               (p)->lpVtbl->SetNumChannelGroups(p,a)
#define IDirectMusicPort_GetNumChannelGroups(p,a)               (p)->lpVtbl->GetNumChannelGroups(p,a)
#define IDirectMusicPort_Activate(p,a)                          (p)->lpVtbl->Activate(p,a)
#define IDirectMusicPort_SetChannelPriority(p,a,b,c)            (p)->lpVtbl->SetChannelPriority(p,a,b,c)
#define IDirectMusicPort_GetChannelPriority(p,a,b,c)            (p)->lpVtbl->GetChannelPriority(p,a,b,c)
#define IDirectMusicPort_SetDirectSound(p,a,b)                  (p)->lpVtbl->SetDirectSound(p,a,b)
#define IDirectMusicPort_GetFormat(p,a,b,c)                     (p)->lpVtbl->GetFormat(p,a,b,c)
#endif


/*****************************************************************************
 * IDirectMusicThru interface
 */
#undef  INTERFACE
#define INTERFACE IDirectMusicThru
#define IDirectMusicThru_METHODS \
    IUnknown_METHODS \
    STDMETHOD(ThruChannel)(THIS_ DWORD dwSourceChannelGroup, DWORD dwSourceChannel, DWORD dwDestinationChannelGroup, DWORD dwDestinationChannel, LPDIRECTMUSICPORT pDestinationPort) PURE;
ICOM_DEFINE(IDirectMusicThru,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IDirectMusicThru_QueryInterface(p,a,b)                  (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectMusicThru_AddRef(p)                              (p)->lpVtbl->AddRef(p)
#define IDirectMusicThru_Release(p)                             (p)->lpVtbl->Release(p)
/*** IDirectMusicThru methods ***/
#define IDirectMusicThru_ThruChannel(p,a,b,c,d,e)               (p)->lpVtbl->ThruChannel(p,a,b,c,d,e)
#endif


/* this one should be defined in dsound.h too, but it's ok if it's here */
#ifndef __IReferenceClock_INTERFACE_DEFINED__
#define __IReferenceClock_INTERFACE_DEFINED__

DEFINE_GUID(IID_IReferenceClock, 0x56a86897,0x0ad4,0x11ce,0xb0,0x3a,0x00,0x20,0xaf,0x0b,0xa7,0x70);

/*****************************************************************************
 * IReferenceClock interface
 */
#undef  INTERFACE
#define INTERFACE IReferenceClock
#define IReferenceClock_METHODS \
    IUnknown_METHODS \
    STDMETHOD(GetTime)(THIS_ REFERENCE_TIME *pTime) PURE; \
    STDMETHOD(AdviseTime)(THIS_ REFERENCE_TIME baseTime, REFERENCE_TIME streamTime, HANDLE hEvent, DWORD *pdwAdviseCookie) PURE; \
    STDMETHOD(AdvisePeriodic)(THIS_ REFERENCE_TIME startTime, REFERENCE_TIME periodTime, HANDLE hSemaphore, DWORD *pdwAdviseCookie) PURE; \
    STDMETHOD(Unadvise)(THIS_ DWORD dwAdviseCookie) PURE;
ICOM_DEFINE(IReferenceClock,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IReferenceClock_QueryInterface(p,a,b)                   (p)->lpVtbl->QueryInterface(p,a,b)
#define IReferenceClock_AddRef(p)                               (p)->lpVtbl->AddRef(p)
#define IReferenceClock_Release(p)                              (p)->lpVtbl->Release(p)
/*** IReferenceClock methods ***/
#define IReferenceClock_GetTime(p,a)                            (p)->lpVtbl->GetTime(p,a)
#define IReferenceClock_AdviseTime(p,a,b,c,d)                   (p)->lpVtbl->AdviseTime(p,a,b,c,d)
#define IReferenceClock_AdvisePeriodic(p,a,b,c,d)               (p)->lpVtbl->AdvisePeriodic(p,a,b,c,d)
#define IReferenceClock_Unadvise(p,a)                           (p)->lpVtbl->Unadvise(p,a)
#endif

#endif /* __IReferenceClock_INTERFACE_DEFINED__ */

#define DMUS_VOLUME_MAX     2000
#define DMUS_VOLUME_MIN   -20000

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

/*#include "poppack.h" */

#endif /* __WINE_DMUSIC_CORE_H */
