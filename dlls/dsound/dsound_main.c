/*  			DirectSound
 * 
 * Copyright 1998 Marcus Meissner
 * Copyright 1998 Rob Riggs
 * Copyright 2000-2001 TransGaming Technologies, Inc.
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
/*
 * Most thread locking is complete. There may be a few race
 * conditions still lurking.
 *
 * Tested with a Soundblaster clone, a Gravis UltraSound Classic,
 * and a Turtle Beach Tropez+.
 *
 * TODO:
 *	Implement DirectSoundCapture API
 *	Implement SetCooperativeLevel properly (need to address focus issues)
 *	Implement DirectSound3DBuffers (stubs in place)
 *	Use hardware 3D support if available
 *      Add critical section locking inside Release and AddRef methods
 *      Handle static buffers - put those in hardware, non-static not in hardware
 *      Hardware DuplicateSoundBuffer
 *      Proper volume calculation, and setting volume in HEL primary buffer
 *      Optimize WINMM and negotiate fragment size, decrease DS_HEL_MARGIN
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>	/* Insomnia - pow() function */

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "mmsystem.h"
#include "mmddk.h"
#include "wine/windef16.h"
#include "wine/debug.h"
#include "dsound.h"
#include "dsdriver.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);

/* these are eligible for tuning... they must be high on slow machines... */
/* some stuff may get more responsive with lower values though... */
#define DS_EMULDRIVER 1 /* some games (Quake 2, UT) refuse to accept
				emulated dsound devices. set to 0 ! */ 
#define DS_HEL_FRAGS 48 /* HEL only: number of waveOut fragments in primary buffer
			 * (changing this won't help you) */
#define DS_HEL_MARGIN 5 /* HEL only: number of waveOut fragments ahead to mix in new buffers
			 * (keep this close or equal to DS_HEL_QUEUE for best results) */
#define DS_HEL_QUEUE  5 /* HEL only: number of waveOut fragments ahead to queue to driver
			 * (this will affect HEL sound reliability and latency) */

#define DS_SND_QUEUE_MAX 28 /* max number of fragments to prebuffer */
#define DS_SND_QUEUE_MIN 12 /* min number of fragments to prebuffer */

/* Linux does not support better timing than 10ms */
#define DS_TIME_RES 10  /* Resolution of multimedia timer */
#define DS_TIME_DEL 10  /* Delay of multimedia timer callback, and duration of HEL fragment */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
typedef struct IDirectSoundImpl IDirectSoundImpl;
typedef struct IDirectSoundBufferImpl IDirectSoundBufferImpl;
typedef struct IDirectSoundNotifyImpl IDirectSoundNotifyImpl;
typedef struct IDirectSound3DListenerImpl IDirectSound3DListenerImpl;
typedef struct IDirectSound3DBufferImpl IDirectSound3DBufferImpl;
typedef struct IDirectSoundCaptureImpl IDirectSoundCaptureImpl;
typedef struct IDirectSoundCaptureBufferImpl IDirectSoundCaptureBufferImpl;
typedef struct IKsPropertySetImpl IKsPropertySetImpl;

/*****************************************************************************
 * IDirectSound implementation structure
 */
struct IDirectSoundImpl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirectSound);
    DWORD                      ref;
    /* IDirectSoundImpl fields */
    PIDSDRIVER                  driver;
    DSDRIVERDESC                drvdesc;
    DSDRIVERCAPS                drvcaps;
    HWAVEOUT                    hwo;
    LPWAVEHDR                   pwave[DS_HEL_FRAGS];
    UINT                        timerID, pwplay, pwwrite, pwqueue, prebuf;
    DWORD                       fraglen;
    DWORD                       priolevel;
    int                         nrofbuffers;
    IDirectSoundBufferImpl**    buffers;
    IDirectSoundBufferImpl*     primary;
    IDirectSound3DListenerImpl* listener;
    WAVEFORMATEX                wfx; /* current main waveformat */
    CRITICAL_SECTION		lock;
};

/*****************************************************************************
 * IDirectSoundBuffer implementation structure
 */
struct IDirectSoundBufferImpl
{
    /* FIXME: document */
    /* IUnknown fields */
    ICOM_VFIELD(IDirectSoundBuffer);
    DWORD                            ref;
    /* IDirectSoundBufferImpl fields */
    PIDSDRIVERBUFFER          hwbuf;
    WAVEFORMATEX              wfx;
    LPBYTE                    buffer;
    IDirectSound3DBufferImpl* ds3db;
    DWORD                     playflags,state,leadin;
    DWORD                     playpos,startpos,writelead,buflen;
    DWORD                     nAvgBytesPerSec;
    DWORD                     freq;
    DSVOLUMEPAN               volpan;
    IDirectSoundBufferImpl*   parent;         /* for duplicates */
    IDirectSoundImpl*         dsound;
    DSBUFFERDESC              dsbd;
    LPDSBPOSITIONNOTIFY       notifies;
    int                       nrofnotifies;
    CRITICAL_SECTION          lock;
    /* used for frequency conversion (PerfectPitch) */
    ULONG                     freqAdjust, freqAcc;
    /* used for intelligent (well, sort of) prebuffering */
    DWORD                     probably_valid_to;
    DWORD                     primary_mixpos, buf_mixpos;
    BOOL                      need_remix;
};

#define STATE_STOPPED  0
#define STATE_STARTING 1
#define STATE_PLAYING  2
#define STATE_STOPPING 3

/*****************************************************************************
 * IDirectSoundNotify implementation structure
 */
struct IDirectSoundNotifyImpl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirectSoundNotify);
    DWORD                            ref;
    /* IDirectSoundNotifyImpl fields */
    IDirectSoundBufferImpl* dsb;
};

/*****************************************************************************
 *  IDirectSound3DListener implementation structure
 */
struct IDirectSound3DListenerImpl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirectSound3DListener);
    DWORD                                ref;
    /* IDirectSound3DListenerImpl fields */
    IDirectSoundBufferImpl* dsb;
    DS3DLISTENER            ds3dl;
    CRITICAL_SECTION        lock;   
};

struct IKsPropertySetImpl 
{
    /* IUnknown fields */
    ICOM_VFIELD(IKsPropertySet);
    DWORD			ref;
    /* IKsPropertySetImpl fields */
    IDirectSound3DBufferImpl	*ds3db;	/* backptr, no ref */
};

/*****************************************************************************
 * IDirectSound3DBuffer implementation structure
 */
struct IDirectSound3DBufferImpl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirectSound3DBuffer);
    DWORD                              ref;
    /* IDirectSound3DBufferImpl fields */
    IDirectSoundBufferImpl* dsb;
    DS3DBUFFER              ds3db;
    LPBYTE                  buffer;
    DWORD                   buflen;
    CRITICAL_SECTION        lock;
    IKsPropertySetImpl*     iks;
};


/*****************************************************************************
 * IDirectSoundCapture implementation structure
 */
struct IDirectSoundCaptureImpl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirectSoundCapture);
    DWORD                              ref;

    /* IDirectSoundCaptureImpl fields */
    CRITICAL_SECTION        lock;
};

/*****************************************************************************
 * IDirectSoundCapture implementation structure
 */
struct IDirectSoundCaptureBufferImpl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirectSoundCaptureBuffer);
    DWORD                              ref;

    /* IDirectSoundCaptureBufferImpl fields */
    CRITICAL_SECTION        lock;
};


/* #define USE_DSOUND3D 1 */

#define DSOUND_FREQSHIFT (14)

static IDirectSoundImpl*	dsound = NULL;

static IDirectSoundBufferImpl*	primarybuf = NULL;

static void DSOUND_CheckEvent(IDirectSoundBufferImpl *dsb, int len);
static void DSOUND_MixCancelAt(IDirectSoundBufferImpl *dsb, DWORD buf_writepos);

static void DSOUND_WaveQueue(IDirectSoundImpl *dsound, DWORD mixq);
static void DSOUND_PerformMix(void);
static void CALLBACK DSOUND_callback(HWAVEOUT hwo, UINT msg, DWORD dwUser, DWORD dw1, DWORD dw2);

static HRESULT DSOUND_CreateDirectSoundCapture( LPVOID* ppobj );
static HRESULT DSOUND_CreateDirectSoundCaptureBuffer( LPCDSCBUFFERDESC lpcDSCBufferDesc, LPVOID* ppobj );

static ICOM_VTABLE(IDirectSoundCapture) dscvt;
static ICOM_VTABLE(IDirectSoundCaptureBuffer) dscbvt;

static HRESULT mmErr(UINT err)
{
	switch(err) {
	case MMSYSERR_NOERROR:
		return DS_OK;
	case MMSYSERR_ALLOCATED:
		return DSERR_ALLOCATED;
	case MMSYSERR_INVALHANDLE:
		return DSERR_GENERIC; /* FIXME */
	case MMSYSERR_NODRIVER:
		return DSERR_NODRIVER;
	case MMSYSERR_NOMEM:
		return DSERR_OUTOFMEMORY;
	case MMSYSERR_INVALPARAM:
		return DSERR_INVALIDPARAM;
	default:
		FIXME("Unknown MMSYS error %d\n",err);
		return DSERR_GENERIC;
	}
}

/***************************************************************************
 * DirectSoundEnumerateA [DSOUND.2]  
 *
 * Enumerate all DirectSound drivers installed in the system
 *
 * RETURNS
 *    Success: DS_OK
 *    Failure: DSERR_INVALIDPARAM
 */
HRESULT WINAPI DirectSoundEnumerateA(
	LPDSENUMCALLBACKA lpDSEnumCallback,
	LPVOID lpContext)
{
	TRACE("lpDSEnumCallback = %p, lpContext = %p\n", 
	      lpDSEnumCallback, lpContext);

#ifdef HAVE_OSS
	if (lpDSEnumCallback != NULL)
		if (lpDSEnumCallback(NULL, "Primary DirectSound Driver",
				     "sound", lpContext))
			lpDSEnumCallback((LPGUID)&DSDEVID_WinePlayback,
					 "WINE DirectSound", "sound",
					 lpContext);
#endif

	return DS_OK;
}

/***************************************************************************
 * DirectSoundEnumerateW [DSOUND.3]  
 *
 * Enumerate all DirectSound drivers installed in the system
 *
 * RETURNS
 *    Success: DS_OK
 *    Failure: DSERR_INVALIDPARAM
 */
HRESULT WINAPI DirectSoundEnumerateW(
	LPDSENUMCALLBACKW lpDSEnumCallback, 
	LPVOID lpContext )
{
        FIXME("lpDSEnumCallback = %p, lpContext = %p: stub\n", 
                lpDSEnumCallback, lpContext);

        return DS_OK;
}


static void _dump_DSBCAPS(DWORD xmask) {
	struct {
		DWORD	mask;
		char	*name;
	} flags[] = {
#define FE(x) { x, #x },
		FE(DSBCAPS_PRIMARYBUFFER)
		FE(DSBCAPS_STATIC)
		FE(DSBCAPS_LOCHARDWARE)
		FE(DSBCAPS_LOCSOFTWARE)
		FE(DSBCAPS_CTRL3D)
		FE(DSBCAPS_CTRLFREQUENCY)
		FE(DSBCAPS_CTRLPAN)
		FE(DSBCAPS_CTRLVOLUME)
		FE(DSBCAPS_CTRLPOSITIONNOTIFY)
		FE(DSBCAPS_CTRLDEFAULT)
		FE(DSBCAPS_CTRLALL)
		FE(DSBCAPS_STICKYFOCUS)
		FE(DSBCAPS_GLOBALFOCUS)
		FE(DSBCAPS_GETCURRENTPOSITION2)
		FE(DSBCAPS_MUTE3DATMAXDISTANCE)
#undef FE
	};
	int	i;

	for (i=0;i<sizeof(flags)/sizeof(flags[0]);i++)
		if ((flags[i].mask & xmask) == flags[i].mask)
			DPRINTF("%s ",flags[i].name);
}

/*******************************************************************************
 *              IKsPropertySet
 */

/* IUnknown methods */
#ifdef USE_DSOUND3D
static HRESULT WINAPI IKsPropertySetImpl_QueryInterface(
	LPKSPROPERTYSET iface, REFIID riid, LPVOID *ppobj
) {
	ICOM_THIS(IKsPropertySetImpl,iface);

	FIXME("(%p,%s,%p), stub!\n",This,debugstr_guid(riid),ppobj);
	return E_FAIL;
}
#endif

#ifdef USE_DSOUND3D
static ULONG WINAPI IKsPropertySetImpl_AddRef(LPKSPROPERTYSET iface) {
	ICOM_THIS(IKsPropertySetImpl,iface);

	This->ref++;
	return This->ref;
}
#endif

#ifdef USE_DSOUND3D
static ULONG WINAPI IKsPropertySetImpl_Release(LPKSPROPERTYSET iface) {
	ICOM_THIS(IKsPropertySetImpl,iface);

	This->ref--;
	return This->ref;
}
#endif

#ifdef USE_DSOUND3D
static HRESULT WINAPI IKsPropertySetImpl_Get(LPKSPROPERTYSET iface,
	REFGUID guidPropSet, ULONG dwPropID,
	LPVOID pInstanceData, ULONG cbInstanceData,
	LPVOID pPropData, ULONG cbPropData,
	PULONG pcbReturned
) {
	ICOM_THIS(IKsPropertySetImpl,iface);

	FIXME("(%p,%s,%ld,%p,%ld,%p,%ld,%p), stub!\n",This,debugstr_guid(guidPropSet),dwPropID,pInstanceData,cbInstanceData,pPropData,cbPropData,pcbReturned);
	return E_PROP_ID_UNSUPPORTED;
}
#endif

#ifdef USE_DSOUND3D
static HRESULT WINAPI IKsPropertySetImpl_Set(LPKSPROPERTYSET iface,
	REFGUID guidPropSet, ULONG dwPropID,
	LPVOID pInstanceData, ULONG cbInstanceData,
	LPVOID pPropData, ULONG cbPropData
) {
	ICOM_THIS(IKsPropertySetImpl,iface);

	FIXME("(%p,%s,%ld,%p,%ld,%p,%ld), stub!\n",This,debugstr_guid(guidPropSet),dwPropID,pInstanceData,cbInstanceData,pPropData,cbPropData);
	return E_PROP_ID_UNSUPPORTED;
}
#endif

#ifdef USE_DSOUND3D
static HRESULT WINAPI IKsPropertySetImpl_QuerySupport(LPKSPROPERTYSET iface,
	REFGUID guidPropSet, ULONG dwPropID, PULONG pTypeSupport
) {
	ICOM_THIS(IKsPropertySetImpl,iface);

	FIXME("(%p,%s,%ld,%p), stub!\n",This,debugstr_guid(guidPropSet),dwPropID,pTypeSupport);
	return E_PROP_ID_UNSUPPORTED;
}
#endif

#ifdef USE_DSOUND3D
static ICOM_VTABLE(IKsPropertySet) iksvt = {
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IKsPropertySetImpl_QueryInterface,
	IKsPropertySetImpl_AddRef,
	IKsPropertySetImpl_Release,
	IKsPropertySetImpl_Get,
	IKsPropertySetImpl_Set,
	IKsPropertySetImpl_QuerySupport
};
#endif

/*******************************************************************************
 *              IDirectSound3DBuffer
 */

/* IUnknown methods */
#ifdef USE_DSOUND3D
static HRESULT WINAPI IDirectSound3DBufferImpl_QueryInterface(
	LPDIRECTSOUND3DBUFFER iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectSound3DBufferImpl,iface);

	if ( IsEqualGUID( &IID_IKsPropertySet, riid ) ) {
	    IDirectSound3DBuffer_AddRef(iface);
	    *ppobj = This->iks;
	    return S_OK;
	}

	FIXME("(%p,%s,%p), no such interface.\n",This,debugstr_guid(riid),ppobj);
	return E_FAIL;
}
#endif

#ifdef USE_DSOUND3D
static ULONG WINAPI IDirectSound3DBufferImpl_AddRef(LPDIRECTSOUND3DBUFFER iface)
{
	ICOM_THIS(IDirectSound3DBufferImpl,iface);
	This->ref++;
	return This->ref;
}
#endif

#ifdef USE_DSOUND3D
static ULONG WINAPI IDirectSound3DBufferImpl_Release(LPDIRECTSOUND3DBUFFER iface)
{
	ICOM_THIS(IDirectSound3DBufferImpl,iface);

	TRACE("(%p) ref was %ld\n", This, This->ref);

	if(--This->ref)
		return This->ref;

	if (This->dsb)
		IDirectSoundBuffer_Release((LPDIRECTSOUNDBUFFER)This->dsb);

	DeleteCriticalSection(&This->lock);

	HeapFree(GetProcessHeap(),0,This->buffer);
	HeapFree(GetProcessHeap(),0,This);

	return 0;
}
#endif

/* IDirectSound3DBuffer methods */
#ifdef USE_DSOUND3D
static HRESULT WINAPI IDirectSound3DBufferImpl_GetAllParameters(
	LPDIRECTSOUND3DBUFFER iface,
	LPDS3DBUFFER lpDs3dBuffer)
{
	FIXME("stub\n");
	return DS_OK;
}
#endif

#ifdef USE_DSOUND3D
static HRESULT WINAPI IDirectSound3DBufferImpl_GetConeAngles(
	LPDIRECTSOUND3DBUFFER iface,
	LPDWORD lpdwInsideConeAngle,
	LPDWORD lpdwOutsideConeAngle)
{
	FIXME("stub\n");
	return DS_OK;
}
#endif

#ifdef USE_DSOUND3D
static HRESULT WINAPI IDirectSound3DBufferImpl_GetConeOrientation(
	LPDIRECTSOUND3DBUFFER iface,
	LPD3DVECTOR lpvConeOrientation)
{
	FIXME("stub\n");
	return DS_OK;
}
#endif

#ifdef USE_DSOUND3D
static HRESULT WINAPI IDirectSound3DBufferImpl_GetConeOutsideVolume(
	LPDIRECTSOUND3DBUFFER iface,
	LPLONG lplConeOutsideVolume)
{
	FIXME("stub\n");
	return DS_OK;
}
#endif

#ifdef USE_DSOUND3D
static HRESULT WINAPI IDirectSound3DBufferImpl_GetMaxDistance(
	LPDIRECTSOUND3DBUFFER iface,
	LPD3DVALUE lpfMaxDistance)
{
	FIXME("stub\n");
	return DS_OK;
}
#endif

#ifdef USE_DSOUND3D
static HRESULT WINAPI IDirectSound3DBufferImpl_GetMinDistance(
	LPDIRECTSOUND3DBUFFER iface,
	LPD3DVALUE lpfMinDistance)
{
	FIXME("stub\n");
	return DS_OK;
}
#endif

#ifdef USE_DSOUND3D
static HRESULT WINAPI IDirectSound3DBufferImpl_GetMode(
	LPDIRECTSOUND3DBUFFER iface,
	LPDWORD lpdwMode)
{
	FIXME("stub\n");
	return DS_OK;
}
#endif

#ifdef USE_DSOUND3D
static HRESULT WINAPI IDirectSound3DBufferImpl_GetPosition(
	LPDIRECTSOUND3DBUFFER iface,
	LPD3DVECTOR lpvPosition)
{
	FIXME("stub\n");
	return DS_OK;
}
#endif

#ifdef USE_DSOUND3D
static HRESULT WINAPI IDirectSound3DBufferImpl_GetVelocity(
	LPDIRECTSOUND3DBUFFER iface,
	LPD3DVECTOR lpvVelocity)
{
	FIXME("stub\n");
	return DS_OK;
}
#endif

#ifdef USE_DSOUND3D
static HRESULT WINAPI IDirectSound3DBufferImpl_SetAllParameters(
	LPDIRECTSOUND3DBUFFER iface,
	LPCDS3DBUFFER lpcDs3dBuffer,
	DWORD dwApply)
{
	FIXME("stub\n");
	return DS_OK;
}
#endif

#ifdef USE_DSOUND3D
static HRESULT WINAPI IDirectSound3DBufferImpl_SetConeAngles(
	LPDIRECTSOUND3DBUFFER iface,
	DWORD dwInsideConeAngle,
	DWORD dwOutsideConeAngle,
	DWORD dwApply)
{
	FIXME("stub\n");
	return DS_OK;
}
#endif

#ifdef USE_DSOUND3D
static HRESULT WINAPI IDirectSound3DBufferImpl_SetConeOrientation(
	LPDIRECTSOUND3DBUFFER iface,
	D3DVALUE x, D3DVALUE y, D3DVALUE z,
	DWORD dwApply)
{
	FIXME("stub\n");
	return DS_OK;
}
#endif

#ifdef USE_DSOUND3D
static HRESULT WINAPI IDirectSound3DBufferImpl_SetConeOutsideVolume(
	LPDIRECTSOUND3DBUFFER iface,
	LONG lConeOutsideVolume,
	DWORD dwApply)
{
	FIXME("stub\n");
	return DS_OK;
}
#endif

#ifdef USE_DSOUND3D
static HRESULT WINAPI IDirectSound3DBufferImpl_SetMaxDistance(
	LPDIRECTSOUND3DBUFFER iface,
	D3DVALUE fMaxDistance,
	DWORD dwApply)
{
	FIXME("stub\n");
	return DS_OK;
}
#endif

#ifdef USE_DSOUND3D
static HRESULT WINAPI IDirectSound3DBufferImpl_SetMinDistance(
	LPDIRECTSOUND3DBUFFER iface,
	D3DVALUE fMinDistance,
	DWORD dwApply)
{
	FIXME("stub\n");
	return DS_OK;
}
#endif

#ifdef USE_DSOUND3D
static HRESULT WINAPI IDirectSound3DBufferImpl_SetMode(
	LPDIRECTSOUND3DBUFFER iface,
	DWORD dwMode,
	DWORD dwApply)
{
	ICOM_THIS(IDirectSound3DBufferImpl,iface);
	TRACE("mode = %lx\n", dwMode);
	This->ds3db.dwMode = dwMode;
	return DS_OK;
}
#endif

#ifdef USE_DSOUND3D
static HRESULT WINAPI IDirectSound3DBufferImpl_SetPosition(
	LPDIRECTSOUND3DBUFFER iface,
	D3DVALUE x, D3DVALUE y, D3DVALUE z,
	DWORD dwApply)
{
	FIXME("stub\n");
	return DS_OK;
}
#endif

#ifdef USE_DSOUND3D
static HRESULT WINAPI IDirectSound3DBufferImpl_SetVelocity(
	LPDIRECTSOUND3DBUFFER iface,
	D3DVALUE x, D3DVALUE y, D3DVALUE z,
	DWORD dwApply)
{
	FIXME("stub\n");
	return DS_OK;
}
#endif

#ifdef USE_DSOUND3D
static ICOM_VTABLE(IDirectSound3DBuffer) ds3dbvt = 
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown methods */
	IDirectSound3DBufferImpl_QueryInterface,
	IDirectSound3DBufferImpl_AddRef,
	IDirectSound3DBufferImpl_Release,
	/* IDirectSound3DBuffer methods */
	IDirectSound3DBufferImpl_GetAllParameters,
	IDirectSound3DBufferImpl_GetConeAngles,
	IDirectSound3DBufferImpl_GetConeOrientation,
	IDirectSound3DBufferImpl_GetConeOutsideVolume,
	IDirectSound3DBufferImpl_GetMaxDistance,
	IDirectSound3DBufferImpl_GetMinDistance,
	IDirectSound3DBufferImpl_GetMode,
	IDirectSound3DBufferImpl_GetPosition,
	IDirectSound3DBufferImpl_GetVelocity,
	IDirectSound3DBufferImpl_SetAllParameters,
	IDirectSound3DBufferImpl_SetConeAngles,
	IDirectSound3DBufferImpl_SetConeOrientation,
	IDirectSound3DBufferImpl_SetConeOutsideVolume,
	IDirectSound3DBufferImpl_SetMaxDistance,
	IDirectSound3DBufferImpl_SetMinDistance,
	IDirectSound3DBufferImpl_SetMode,
	IDirectSound3DBufferImpl_SetPosition,
	IDirectSound3DBufferImpl_SetVelocity,
};
#endif

#ifdef USE_DSOUND3D
static int DSOUND_Create3DBuffer(IDirectSoundBufferImpl* dsb)
{
	DWORD	i, temp, iSize, oSize, offset;
	LPBYTE	bIbuf, bObuf, bTbuf = NULL;
	LPWORD	wIbuf, wObuf, wTbuf = NULL;

	/* Inside DirectX says it's stupid but allowed */
	if (dsb->wfx.nChannels == 2) {
		/* Convert to mono */
		if (dsb->wfx.wBitsPerSample == 16) {
			iSize = dsb->buflen / 4;
			wTbuf = malloc(dsb->buflen / 2);
			if (wTbuf == NULL)
				return DSERR_OUTOFMEMORY;
			for (i = 0; i < iSize; i++)
				wTbuf[i] = (dsb->buffer[i * 2] + dsb->buffer[(i * 2) + 1]) / 2;
			wIbuf = wTbuf;
		} else {
			iSize = dsb->buflen / 2;
			bTbuf = malloc(dsb->buflen / 2);
			if (bTbuf == NULL)
				return DSERR_OUTOFMEMORY;
			for (i = 0; i < iSize; i++)
				bTbuf[i] = (dsb->buffer[i * 2] + dsb->buffer[(i * 2) + 1]) / 2;
			bIbuf = bTbuf;
		}
	} else {
		if (dsb->wfx.wBitsPerSample == 16) {
			iSize = dsb->buflen / 2;
			wIbuf = (LPWORD) dsb->buffer;
		} else {
			bIbuf = (LPBYTE) dsb->buffer;
			iSize = dsb->buflen;
		}
	}

	if (primarybuf->wfx.wBitsPerSample == 16) {
		wObuf = (LPWORD) dsb->ds3db->buffer;
		oSize = dsb->ds3db->buflen / 2;
	} else {
		bObuf = (LPBYTE) dsb->ds3db->buffer;
		oSize = dsb->ds3db->buflen;
	}

	offset = primarybuf->wfx.nSamplesPerSec / 100;		/* 10ms */
	if (primarybuf->wfx.wBitsPerSample == 16 && dsb->wfx.wBitsPerSample == 16)
		for (i = 0; i < iSize; i++) {
			temp = wIbuf[i];
			if (i >= offset)
				temp += wIbuf[i - offset] >> 9;
			else
				temp += wIbuf[i + iSize - offset] >> 9;
			wObuf[i * 2] = temp;
			wObuf[(i * 2) + 1] = temp;
		}
	else if (primarybuf->wfx.wBitsPerSample == 8 && dsb->wfx.wBitsPerSample == 8)
		for (i = 0; i < iSize; i++) {
			temp = bIbuf[i];
			if (i >= offset)
				temp += bIbuf[i - offset] >> 5;
			else
				temp += bIbuf[i + iSize - offset] >> 5;
			bObuf[i * 2] = temp;
			bObuf[(i * 2) + 1] = temp;
		}
	
	if (wTbuf)
		free(wTbuf);
	if (bTbuf)
		free(bTbuf);

	return DS_OK;
}
#endif
/*******************************************************************************
 *              IDirectSound3DListener
 */

/* IUnknown methods */
static HRESULT WINAPI IDirectSound3DListenerImpl_QueryInterface(
	LPDIRECTSOUND3DLISTENER iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectSound3DListenerImpl,iface);

	TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
	return E_FAIL;
}
	
static ULONG WINAPI IDirectSound3DListenerImpl_AddRef(LPDIRECTSOUND3DLISTENER iface)
{
	ICOM_THIS(IDirectSound3DListenerImpl,iface);
	This->ref++;
	return This->ref;
}

static ULONG WINAPI IDirectSound3DListenerImpl_Release(LPDIRECTSOUND3DLISTENER iface)
{
	ULONG ulReturn;
	ICOM_THIS(IDirectSound3DListenerImpl,iface);

	TRACE("(%p) ref was %ld\n", This, This->ref);

	ulReturn = --This->ref;

	/* Free all resources */
	if( ulReturn == 0 ) {
		if(This->dsb)
			IDirectSoundBuffer_Release((LPDIRECTSOUNDBUFFER)This->dsb);
		DeleteCriticalSection(&This->lock);
		HeapFree(GetProcessHeap(),0,This);
	}

	return ulReturn;
}

/* IDirectSound3DListener methods */
static HRESULT WINAPI IDirectSound3DListenerImpl_GetAllParameter(
	LPDIRECTSOUND3DLISTENER iface,
	LPDS3DLISTENER lpDS3DL)
{
	FIXME("stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_GetDistanceFactor(
	LPDIRECTSOUND3DLISTENER iface,
	LPD3DVALUE lpfDistanceFactor)
{
	FIXME("stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_GetDopplerFactor(
	LPDIRECTSOUND3DLISTENER iface,
	LPD3DVALUE lpfDopplerFactor)
{
	FIXME("stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_GetOrientation(
	LPDIRECTSOUND3DLISTENER iface,
	LPD3DVECTOR lpvOrientFront,
	LPD3DVECTOR lpvOrientTop)
{
	FIXME("stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_GetPosition(
	LPDIRECTSOUND3DLISTENER iface,
	LPD3DVECTOR lpvPosition)
{
	FIXME("stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_GetRolloffFactor(
	LPDIRECTSOUND3DLISTENER iface,
	LPD3DVALUE lpfRolloffFactor)
{
	FIXME("stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_GetVelocity(
	LPDIRECTSOUND3DLISTENER iface,
	LPD3DVECTOR lpvVelocity)
{
	FIXME("stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_SetAllParameters(
	LPDIRECTSOUND3DLISTENER iface,
	LPCDS3DLISTENER lpcDS3DL,
	DWORD dwApply)
{
	FIXME("stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_SetDistanceFactor(
	LPDIRECTSOUND3DLISTENER iface,
	D3DVALUE fDistanceFactor,
	DWORD dwApply)
{
	FIXME("stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_SetDopplerFactor(
	LPDIRECTSOUND3DLISTENER iface,
	D3DVALUE fDopplerFactor,
	DWORD dwApply)
{
	FIXME("stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_SetOrientation(
	LPDIRECTSOUND3DLISTENER iface,
	D3DVALUE xFront, D3DVALUE yFront, D3DVALUE zFront,
	D3DVALUE xTop, D3DVALUE yTop, D3DVALUE zTop,
	DWORD dwApply)
{
	FIXME("stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_SetPosition(
	LPDIRECTSOUND3DLISTENER iface,
	D3DVALUE x, D3DVALUE y, D3DVALUE z,
	DWORD dwApply)
{
	FIXME("stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_SetRolloffFactor(
	LPDIRECTSOUND3DLISTENER iface,
	D3DVALUE fRolloffFactor,
	DWORD dwApply)
{
	FIXME("stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_SetVelocity(
	LPDIRECTSOUND3DLISTENER iface,
	D3DVALUE x, D3DVALUE y, D3DVALUE z,
	DWORD dwApply)
{
	FIXME("stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_CommitDeferredSettings(
	LPDIRECTSOUND3DLISTENER iface)

{
	FIXME("stub\n");
	return DS_OK;
}

static ICOM_VTABLE(IDirectSound3DListener) ds3dlvt = 
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown methods */
	IDirectSound3DListenerImpl_QueryInterface,
	IDirectSound3DListenerImpl_AddRef,
	IDirectSound3DListenerImpl_Release,
	/* IDirectSound3DListener methods */
	IDirectSound3DListenerImpl_GetAllParameter,
	IDirectSound3DListenerImpl_GetDistanceFactor,
	IDirectSound3DListenerImpl_GetDopplerFactor,
	IDirectSound3DListenerImpl_GetOrientation,
	IDirectSound3DListenerImpl_GetPosition,
	IDirectSound3DListenerImpl_GetRolloffFactor,
	IDirectSound3DListenerImpl_GetVelocity,
	IDirectSound3DListenerImpl_SetAllParameters,
	IDirectSound3DListenerImpl_SetDistanceFactor,
	IDirectSound3DListenerImpl_SetDopplerFactor,
	IDirectSound3DListenerImpl_SetOrientation,
	IDirectSound3DListenerImpl_SetPosition,
	IDirectSound3DListenerImpl_SetRolloffFactor,
	IDirectSound3DListenerImpl_SetVelocity,
	IDirectSound3DListenerImpl_CommitDeferredSettings,
};

/*******************************************************************************
 *		IDirectSoundNotify
 */
static HRESULT WINAPI IDirectSoundNotifyImpl_QueryInterface(
	LPDIRECTSOUNDNOTIFY iface,REFIID riid,LPVOID *ppobj
) {
	ICOM_THIS(IDirectSoundNotifyImpl,iface);

	TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
	return E_FAIL;
}

static ULONG WINAPI IDirectSoundNotifyImpl_AddRef(LPDIRECTSOUNDNOTIFY iface) {
	ICOM_THIS(IDirectSoundNotifyImpl,iface);
	return ++(This->ref);
}

static ULONG WINAPI IDirectSoundNotifyImpl_Release(LPDIRECTSOUNDNOTIFY iface) {
	ICOM_THIS(IDirectSoundNotifyImpl,iface);

	TRACE("(%p) ref was %ld\n", This, This->ref);

	This->ref--;
	if (!This->ref) {
		IDirectSoundBuffer_Release((LPDIRECTSOUNDBUFFER)This->dsb);
		HeapFree(GetProcessHeap(),0,This);
		return 0;
	}
	return This->ref;
}

static HRESULT WINAPI IDirectSoundNotifyImpl_SetNotificationPositions(
	LPDIRECTSOUNDNOTIFY iface,DWORD howmuch,LPCDSBPOSITIONNOTIFY notify
) {
	ICOM_THIS(IDirectSoundNotifyImpl,iface);
	int	i;

	if (TRACE_ON(dsound)) {
	    TRACE("(%p,0x%08lx,%p)\n",This,howmuch,notify);
	    for (i=0;i<howmuch;i++)
		    TRACE("notify at %ld to 0x%08lx\n",
                            notify[i].dwOffset,(DWORD)notify[i].hEventNotify);
	}
	This->dsb->notifies = HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,This->dsb->notifies,(This->dsb->nrofnotifies+howmuch)*sizeof(DSBPOSITIONNOTIFY));
	memcpy(	This->dsb->notifies+This->dsb->nrofnotifies,
		notify,
		howmuch*sizeof(DSBPOSITIONNOTIFY)
	);
	This->dsb->nrofnotifies+=howmuch;

	return S_OK;
}

static ICOM_VTABLE(IDirectSoundNotify) dsnvt = 
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectSoundNotifyImpl_QueryInterface,
	IDirectSoundNotifyImpl_AddRef,
	IDirectSoundNotifyImpl_Release,
	IDirectSoundNotifyImpl_SetNotificationPositions,
};

/*******************************************************************************
 *		IDirectSoundBuffer
 */

static void DSOUND_RecalcVolPan(PDSVOLUMEPAN volpan)
{
	double temp;

	/* the AmpFactors are expressed in 16.16 fixed point */
	volpan->dwVolAmpFactor = (ULONG) (pow(2.0, volpan->lVolume / 600.0) * 65536);
	/* FIXME: dwPan{Left|Right}AmpFactor */

	/* FIXME: use calculated vol and pan ampfactors */
	temp = (double) (volpan->lVolume - (volpan->lPan > 0 ? volpan->lPan : 0));
	volpan->dwTotalLeftAmpFactor = (ULONG) (pow(2.0, temp / 600.0) * 65536);
	temp = (double) (volpan->lVolume + (volpan->lPan < 0 ? volpan->lPan : 0));
	volpan->dwTotalRightAmpFactor = (ULONG) (pow(2.0, temp / 600.0) * 65536);

	TRACE("left = %lx, right = %lx\n", volpan->dwTotalLeftAmpFactor, volpan->dwTotalRightAmpFactor);
}

static void DSOUND_RecalcFormat(IDirectSoundBufferImpl *dsb)
{
	DWORD sw;

	sw = dsb->wfx.nChannels * (dsb->wfx.wBitsPerSample / 8);
	if ((dsb->dsbd.dwFlags & DSBCAPS_PRIMARYBUFFER) && dsb->hwbuf) {
		DWORD fraglen;
		/* let fragment size approximate the timer delay */
		fraglen = (dsb->freq * DS_TIME_DEL / 1000) * sw;
		/* reduce fragment size until an integer number of them fits in the buffer */
		/* (FIXME: this may or may not be a good idea) */
		while (dsb->buflen % fraglen) fraglen -= sw;
		dsb->dsound->fraglen = fraglen;
		TRACE("fraglen=%ld\n", dsb->dsound->fraglen);
	}
	/* calculate the 10ms write lead */
	dsb->writelead = (dsb->freq / 100) * sw;
}

static HRESULT DSOUND_PrimaryOpen(IDirectSoundBufferImpl *dsb)
{
	HRESULT err = DS_OK;

	/* are we using waveOut stuff? */
	if (!dsb->hwbuf) {
		LPBYTE newbuf;
		DWORD buflen;
		HRESULT merr = DS_OK;
		/* Start in pause mode, to allow buffers to get filled */
		waveOutPause(dsb->dsound->hwo);
		if (dsb->state == STATE_PLAYING) dsb->state = STATE_STARTING;
		else if (dsb->state == STATE_STOPPING) dsb->state = STATE_STOPPED;
		/* use fragments of 10ms (1/100s) each (which should get us within
		 * the documented write cursor lead of 10-15ms) */
		buflen = ((dsb->wfx.nAvgBytesPerSec / 100) & ~3) * DS_HEL_FRAGS;
		TRACE("desired buflen=%ld, old buffer=%p\n", buflen, dsb->buffer);
		/* reallocate emulated primary buffer */
		newbuf = (LPBYTE)HeapReAlloc(GetProcessHeap(),0,dsb->buffer,buflen);
		if (newbuf == NULL) {
			ERR("failed to allocate primary buffer\n");
			merr = DSERR_OUTOFMEMORY;
			/* but the old buffer might still exists and must be re-prepared */
		} else {
			dsb->buffer = newbuf;
			dsb->buflen = buflen;
		}
		if (dsb->buffer) {
			unsigned c;
			IDirectSoundImpl *ds = dsb->dsound;

			ds->fraglen = dsb->buflen / DS_HEL_FRAGS;

			/* prepare fragment headers */
			for (c=0; c<DS_HEL_FRAGS; c++) {
				ds->pwave[c]->lpData = dsb->buffer + c*ds->fraglen;
				ds->pwave[c]->dwBufferLength = ds->fraglen;
				ds->pwave[c]->dwUser = (DWORD)dsb;
				ds->pwave[c]->dwFlags = 0;
				ds->pwave[c]->dwLoops = 0;
				err = mmErr(waveOutPrepareHeader(ds->hwo,ds->pwave[c],sizeof(WAVEHDR)));
				if (err != DS_OK) {
					while (c--)
						waveOutUnprepareHeader(ds->hwo,ds->pwave[c],sizeof(WAVEHDR));
					break;
				}
			}

			ds->pwplay = 0;
			ds->pwwrite = 0;
			ds->pwqueue = 0;
			memset(dsb->buffer, (dsb->wfx.wBitsPerSample == 16) ? 0 : 128, dsb->buflen);
			TRACE("fraglen=%ld\n", ds->fraglen);
			DSOUND_WaveQueue(dsb->dsound, (DWORD)-1);
		}
		if ((err == DS_OK) && (merr != DS_OK))
			err = merr;
	}
	return err;
}


static void DSOUND_PrimaryClose(IDirectSoundBufferImpl *dsb)
{
	/* are we using waveOut stuff? */
	if (!dsb->hwbuf) {
		unsigned c;
		IDirectSoundImpl *ds = dsb->dsound;

		ds->pwqueue = (DWORD)-1; /* resetting queues */
		waveOutReset(ds->hwo);
		for (c=0; c<DS_HEL_FRAGS; c++)
			waveOutUnprepareHeader(ds->hwo, ds->pwave[c], sizeof(WAVEHDR));
		ds->pwqueue = 0;
	}
}

static HRESULT DSOUND_PrimaryPlay(IDirectSoundBufferImpl *dsb)
{
	HRESULT err = DS_OK;
	if (dsb->hwbuf)
		err = IDsDriverBuffer_Play(dsb->hwbuf, 0, 0, DSBPLAY_LOOPING);
	else
		err = mmErr(waveOutRestart(dsb->dsound->hwo));
	return err;
}

static HRESULT DSOUND_PrimaryStop(IDirectSoundBufferImpl *dsb)
{
	HRESULT err = DS_OK;

	TRACE("\n");

	if (dsb->hwbuf) {
		err = IDsDriverBuffer_Stop(dsb->hwbuf);
		if (err == DSERR_BUFFERLOST) {
			/* Wine-only: the driver wants us to reopen the device */
			/* FIXME: check for errors */
			IDsDriverBuffer_Release(primarybuf->hwbuf);
			waveOutClose(dsb->dsound->hwo);
			dsb->dsound->hwo = 0;
			err = mmErr(waveOutOpen(&(dsb->dsound->hwo), dsb->dsound->drvdesc.dnDevNode,
                                                &(primarybuf->wfx), (DWORD)DSOUND_callback, (DWORD)dsb->dsound,
                                                CALLBACK_FUNCTION | WAVE_DIRECTSOUND));
                        if (err == DS_OK)
                            err = IDsDriver_CreateSoundBuffer(dsb->dsound->driver,&(dsb->wfx),dsb->dsbd.dwFlags,0,
                                                              &(dsb->buflen),&(dsb->buffer),
                                                              (LPVOID)&(dsb->hwbuf));
		}
	}
	else
		err = mmErr(waveOutPause(dsb->dsound->hwo));
	return err;
}

/* This sets this format for the <em>Primary Buffer Only</em> */
/* See file:///cdrom/sdk52/docs/worddoc/dsound.doc page 120 */
static HRESULT WINAPI IDirectSoundBufferImpl_SetFormat(
	LPDIRECTSOUNDBUFFER iface,LPWAVEFORMATEX wfex
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	IDirectSoundBufferImpl** dsb;
	HRESULT err = DS_OK;
	int			i;

	/* Let's be pedantic! */
	if ((wfex == NULL) ||
	    (wfex->wFormatTag != WAVE_FORMAT_PCM) ||
	    (wfex->nChannels < 1) || (wfex->nChannels > 2) ||
	    (wfex->nSamplesPerSec < 1) ||
	    (wfex->nBlockAlign < 1) || (wfex->nChannels > 4) ||
	    ((wfex->wBitsPerSample != 8) && (wfex->wBitsPerSample != 16))) {
		TRACE("failed pedantic check!\n");
		return DSERR_INVALIDPARAM;
	}

	/* **** */
	EnterCriticalSection(&(This->dsound->lock));

	if (primarybuf->wfx.nSamplesPerSec != wfex->nSamplesPerSec) {
		dsb = dsound->buffers;
		for (i = 0; i < dsound->nrofbuffers; i++, dsb++) {
			/* **** */
			EnterCriticalSection(&((*dsb)->lock));

			(*dsb)->freqAdjust = ((*dsb)->freq << DSOUND_FREQSHIFT) /
				wfex->nSamplesPerSec;

			LeaveCriticalSection(&((*dsb)->lock));
			/* **** */
		}
	}

	memcpy(&(primarybuf->wfx), wfex, sizeof(primarybuf->wfx));

	TRACE("(formattag=0x%04x,chans=%d,samplerate=%ld,"
		   "bytespersec=%ld,blockalign=%d,bitspersamp=%d,cbSize=%d)\n",
		   wfex->wFormatTag, wfex->nChannels, wfex->nSamplesPerSec,
		   wfex->nAvgBytesPerSec, wfex->nBlockAlign, 
		   wfex->wBitsPerSample, wfex->cbSize);

	primarybuf->wfx.nAvgBytesPerSec =
		This->wfx.nSamplesPerSec * This->wfx.nBlockAlign;

	if (primarybuf->dsound->drvdesc.dwFlags & DSDDESC_DOMMSYSTEMSETFORMAT) {
		/* FIXME: check for errors */
		DSOUND_PrimaryClose(primarybuf);
		waveOutClose(This->dsound->hwo);
		This->dsound->hwo = 0;
                err = mmErr(waveOutOpen(&(This->dsound->hwo), This->dsound->drvdesc.dnDevNode,
                                        &(primarybuf->wfx), (DWORD)DSOUND_callback, (DWORD)This->dsound,
                                        CALLBACK_FUNCTION | WAVE_DIRECTSOUND));
                if (err == DS_OK)
                    DSOUND_PrimaryOpen(primarybuf);
	}
	if (primarybuf->hwbuf) {
		err = IDsDriverBuffer_SetFormat(primarybuf->hwbuf, &(primarybuf->wfx));
		if (err == DSERR_BUFFERLOST) {
			/* Wine-only: the driver wants us to recreate the HW buffer */
			IDsDriverBuffer_Release(primarybuf->hwbuf);
			err = IDsDriver_CreateSoundBuffer(primarybuf->dsound->driver,&(primarybuf->wfx),primarybuf->dsbd.dwFlags,0,
							  &(primarybuf->buflen),&(primarybuf->buffer),
							  (LPVOID)&(primarybuf->hwbuf));
			if (primarybuf->state == STATE_PLAYING) primarybuf->state = STATE_STARTING;
			else if (primarybuf->state == STATE_STOPPING) primarybuf->state = STATE_STOPPED;
		}
                /* FIXME: should we set err back to DS_OK in all cases ? */
	}
	DSOUND_RecalcFormat(primarybuf);

	LeaveCriticalSection(&(This->dsound->lock));
	/* **** */

	return err;
}

static HRESULT WINAPI IDirectSoundBufferImpl_SetVolume(
	LPDIRECTSOUNDBUFFER iface,LONG vol
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);

	TRACE("(%p,%ld)\n",This,vol);

	/* I'm not sure if we need this for primary buffer */
	if (!(This->dsbd.dwFlags & DSBCAPS_CTRLVOLUME))
		return DSERR_CONTROLUNAVAIL;

	if ((vol > DSBVOLUME_MAX) || (vol < DSBVOLUME_MIN))
		return DSERR_INVALIDPARAM;

	/* **** */
	EnterCriticalSection(&(This->lock));

	This->volpan.lVolume = vol;

	DSOUND_RecalcVolPan(&(This->volpan));

	if (This->hwbuf) {
		IDsDriverBuffer_SetVolumePan(This->hwbuf, &(This->volpan));
	}
	else if (This->dsbd.dwFlags & DSBCAPS_PRIMARYBUFFER) {
#if 0 /* should we really do this? */
		/* the DS volume ranges from 0 (max, 0dB attenuation) to -10000 (min, 100dB attenuation) */
		/* the MM volume ranges from 0 to 0xffff in an unspecified logarithmic scale */
		WORD cvol = 0xffff + vol*6 + vol/2;
		DWORD vol = cvol | ((DWORD)cvol << 16)
		waveOutSetVolume(This->dsound->hwo, vol);
#endif
	}

	LeaveCriticalSection(&(This->lock));
	/* **** */

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_GetVolume(
	LPDIRECTSOUNDBUFFER iface,LPLONG vol
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	TRACE("(%p,%p)\n",This,vol);

	if (vol == NULL)
		return DSERR_INVALIDPARAM;

	*vol = This->volpan.lVolume;
	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_SetFrequency(
	LPDIRECTSOUNDBUFFER iface,DWORD freq
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	TRACE("(%p,%ld)\n",This,freq);

	/* You cannot set the frequency of the primary buffer */
	if (!(This->dsbd.dwFlags & DSBCAPS_CTRLFREQUENCY) ||
	    (This->dsbd.dwFlags & DSBCAPS_PRIMARYBUFFER))
		return DSERR_CONTROLUNAVAIL;

	if (!freq) freq = This->wfx.nSamplesPerSec;

	if ((freq < DSBFREQUENCY_MIN) || (freq > DSBFREQUENCY_MAX))
		return DSERR_INVALIDPARAM;

	/* **** */
	EnterCriticalSection(&(This->lock));

	This->freq = freq;
	This->freqAdjust = (freq << DSOUND_FREQSHIFT) / primarybuf->wfx.nSamplesPerSec;
	This->nAvgBytesPerSec = freq * This->wfx.nBlockAlign;
	DSOUND_RecalcFormat(This);

	LeaveCriticalSection(&(This->lock));
	/* **** */

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_Play(
	LPDIRECTSOUNDBUFFER iface,DWORD reserved1,DWORD reserved2,DWORD flags
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	TRACE("(%p,%08lx,%08lx,%08lx)\n",
		This,reserved1,reserved2,flags
	);

	/* **** */
	EnterCriticalSection(&(This->lock));

	This->playflags = flags;
	if (This->state == STATE_STOPPED) {
		This->leadin = TRUE;
		This->startpos = This->buf_mixpos;
		This->state = STATE_STARTING;
	} else if (This->state == STATE_STOPPING)
		This->state = STATE_PLAYING;
	if (!(This->dsbd.dwFlags & DSBCAPS_PRIMARYBUFFER) && This->hwbuf) {
		IDsDriverBuffer_Play(This->hwbuf, 0, 0, This->playflags);
		This->state = STATE_PLAYING;
	}

	LeaveCriticalSection(&(This->lock));
	/* **** */

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_Stop(LPDIRECTSOUNDBUFFER iface)
{
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	TRACE("(%p)\n",This);

	/* **** */
	EnterCriticalSection(&(This->lock));

	if (This->state == STATE_PLAYING)
		This->state = STATE_STOPPING;
	else if (This->state == STATE_STARTING)
		This->state = STATE_STOPPED;
	if (!(This->dsbd.dwFlags & DSBCAPS_PRIMARYBUFFER) && This->hwbuf) {
		IDsDriverBuffer_Stop(This->hwbuf);
		This->state = STATE_STOPPED;
	}
	DSOUND_CheckEvent(This, 0);

	LeaveCriticalSection(&(This->lock));
	/* **** */

	return DS_OK;
}

static DWORD WINAPI IDirectSoundBufferImpl_AddRef(LPDIRECTSOUNDBUFFER iface) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	DWORD ref;

	TRACE("(%p) ref was %ld, thread is %lx\n",This, This->ref, GetCurrentThreadId());

	ref = InterlockedIncrement(&(This->ref));
	if (!ref) {
		FIXME("thread-safety alert! AddRef-ing with a zero refcount!\n");
	}
	return ref;
}
static DWORD WINAPI IDirectSoundBufferImpl_Release(LPDIRECTSOUNDBUFFER iface) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	int	i;
	DWORD ref;

	TRACE("(%p) ref was %ld, thread is %lx\n",This, This->ref, GetCurrentThreadId());

	ref = InterlockedDecrement(&(This->ref));
	if (ref) return ref;

	EnterCriticalSection(&(This->dsound->lock));
	for (i=0;i<This->dsound->nrofbuffers;i++)
		if (This->dsound->buffers[i] == This)
			break;

	if (i < This->dsound->nrofbuffers) {
		/* Put the last buffer of the list in the (now empty) position */
		This->dsound->buffers[i] = This->dsound->buffers[This->dsound->nrofbuffers - 1];
		This->dsound->nrofbuffers--;
		This->dsound->buffers = HeapReAlloc(GetProcessHeap(),0,This->dsound->buffers,sizeof(LPDIRECTSOUNDBUFFER)*This->dsound->nrofbuffers);
		TRACE("buffer count is now %d\n", This->dsound->nrofbuffers);
		IDirectSound_Release((LPDIRECTSOUND)This->dsound);
	}
	LeaveCriticalSection(&(This->dsound->lock));

	DeleteCriticalSection(&(This->lock));
	if (This->dsbd.dwFlags & DSBCAPS_PRIMARYBUFFER)
		DSOUND_PrimaryClose(This);
	if (This->hwbuf) {
		IDsDriverBuffer_Release(This->hwbuf);
	}
	if (This->ds3db)
		IDirectSound3DBuffer_Release((LPDIRECTSOUND3DBUFFER)This->ds3db);
	if (This->parent)
		/* this is a duplicate buffer */
		IDirectSoundBuffer_Release((LPDIRECTSOUNDBUFFER)This->parent);
	else
		/* this is a toplevel buffer */
		HeapFree(GetProcessHeap(),0,This->buffer);

	HeapFree(GetProcessHeap(),0,This);
	
	if (This == primarybuf)
		primarybuf = NULL;

	return 0;
}

static DWORD DSOUND_CalcPlayPosition(IDirectSoundBufferImpl *This,
				     DWORD state, DWORD pplay, DWORD pwrite, DWORD pmix, DWORD bmix)
{
	DWORD bplay;

	TRACE("primary playpos=%ld, mixpos=%ld\n", pplay, pmix);
	TRACE("this mixpos=%ld, time=%ld\n", bmix, GetTickCount());

	/* the actual primary play position (pplay) is always behind last mixed (pmix),
	 * unless the computer is too slow or something */
	/* we need to know how far away we are from there */
#if 0 /* we'll never fill the primary entirely */
	if (pmix == pplay) {
		if ((state == STATE_PLAYING) || (state == STATE_STOPPING)) {
			/* wow, the software mixer is really doing well,
			 * seems the entire primary buffer is filled! */
			pmix += primarybuf->buflen;
		}
		/* else: the primary buffer is not playing, so probably empty */
	}
#endif
	if (pmix < pplay) pmix += primarybuf->buflen; /* wraparound */
	pmix -= pplay;
	/* detect buffer underrun */
	if (pwrite < pplay) pwrite += primarybuf->buflen; /* wraparound */
	pwrite -= pplay;
	if (pmix > (DS_SND_QUEUE_MAX * primarybuf->dsound->fraglen + pwrite + primarybuf->writelead)) {
		WARN("detected an underrun: primary queue was %ld\n",pmix);
		pmix = 0;
	}
	/* divide the offset by its sample size */
	pmix /= primarybuf->wfx.nBlockAlign;
	TRACE("primary back-samples=%ld\n",pmix);
	/* adjust for our frequency */
	pmix = (pmix * This->freqAdjust) >> DSOUND_FREQSHIFT;
	/* multiply by our own sample size */
	pmix *= This->wfx.nBlockAlign;
	TRACE("this back-offset=%ld\n", pmix);
	/* subtract from our last mixed position */
	bplay = bmix;
	while (bplay < pmix) bplay += This->buflen; /* wraparound */
	bplay -= pmix;
	if (This->leadin && ((bplay < This->startpos) || (bplay > bmix))) {
		/* seems we haven't started playing yet */
		TRACE("this still in lead-in phase\n");
		bplay = This->startpos;
	}
	/* return the result */
	return bplay;
}

static HRESULT WINAPI IDirectSoundBufferImpl_GetCurrentPosition(
	LPDIRECTSOUNDBUFFER iface,LPDWORD playpos,LPDWORD writepos
) {
	HRESULT	hres; 
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	TRACE("(%p,%p,%p)\n",This,playpos,writepos);
	if (This->hwbuf) {
		hres=IDsDriverBuffer_GetPosition(This->hwbuf,playpos,writepos);
		if (hres)
		    return hres;
	}
	else if (This->dsbd.dwFlags & DSBCAPS_PRIMARYBUFFER) {
		if (playpos) {
			MMTIME mtime;
			mtime.wType = TIME_BYTES;
			waveOutGetPosition(This->dsound->hwo, &mtime, sizeof(mtime));
			mtime.u.cb = mtime.u.cb % This->buflen;
			*playpos = mtime.u.cb;
		}
		if (writepos) {
			/* the writepos should only be used by apps with WRITEPRIMARY priority,
			 * in which case our software mixer is disabled anyway */
			*writepos = (This->dsound->pwplay + DS_HEL_MARGIN) * This->dsound->fraglen;
			while (*writepos >= This->buflen)
				*writepos -= This->buflen;
		}
	} else {
		if (playpos && (This->state != STATE_PLAYING)) {
			/* we haven't been merged into the primary buffer (yet) */
			*playpos = This->buf_mixpos;
		}
		else if (playpos) {
			DWORD pplay, pwrite, lplay, splay, pstate;
			/* let's get this exact; first, recursively call GetPosition on the primary */
			EnterCriticalSection(&(primarybuf->lock));
			IDirectSoundBufferImpl_GetCurrentPosition((LPDIRECTSOUNDBUFFER)primarybuf, &pplay, &pwrite);
			/* detect HEL mode underrun */
			pstate = primarybuf->state;
			if (!(primarybuf->hwbuf || primarybuf->dsound->pwqueue)) {
				TRACE("detected an underrun\n");
				/* pplay = ? */
				if (pstate == STATE_PLAYING)
					pstate = STATE_STARTING;
				else if (pstate == STATE_STOPPING)
					pstate = STATE_STOPPED;
			}
			/* get data for ourselves while we still have the lock */
			pstate &= This->state;
			lplay = This->primary_mixpos;
			splay = This->buf_mixpos;
			if ((This->dsbd.dwFlags & DSBCAPS_GETCURRENTPOSITION2) || primarybuf->hwbuf) {
				/* calculate play position using this */
				*playpos = DSOUND_CalcPlayPosition(This, pstate, pplay, pwrite, lplay, splay);
			} else {
				/* (unless the app isn't using GETCURRENTPOSITION2) */
				/* don't know exactly how this should be handled...
				 * the docs says that play cursor is reported as directly
				 * behind write cursor, hmm... */
				/* let's just do what might work for Half-Life */
				DWORD wp;
				wp = (This->dsound->pwplay + DS_HEL_MARGIN) * This->dsound->fraglen;
				while (wp >= primarybuf->buflen)
					wp -= primarybuf->buflen;
				*playpos = DSOUND_CalcPlayPosition(This, pstate, wp, pwrite, lplay, splay);
			}
			LeaveCriticalSection(&(primarybuf->lock));
		}
		if (writepos) *writepos = This->buf_mixpos;
	}
	if (writepos) {
		if (This->state != STATE_STOPPED)
			/* apply the documented 10ms lead to writepos */
			*writepos += This->writelead;
		while (*writepos >= This->buflen) *writepos -= This->buflen;
	}
	TRACE("playpos = %ld, writepos = %ld (%p, time=%ld)\n", playpos?*playpos:0, writepos?*writepos:0, This, GetTickCount());
	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_GetStatus(
	LPDIRECTSOUNDBUFFER iface,LPDWORD status
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	TRACE("(%p,%p), thread is %lx\n",This,status,GetCurrentThreadId());

	if (status == NULL)
		return DSERR_INVALIDPARAM;

	*status = 0;
	if ((This->state == STATE_STARTING) || (This->state == STATE_PLAYING)) {
		*status |= DSBSTATUS_PLAYING;
		if (This->playflags & DSBPLAY_LOOPING)
			*status |= DSBSTATUS_LOOPING;
	}

	TRACE("status=%lx\n", *status);
	return DS_OK;
}


static HRESULT WINAPI IDirectSoundBufferImpl_GetFormat(
	LPDIRECTSOUNDBUFFER iface,LPWAVEFORMATEX lpwf,DWORD wfsize,LPDWORD wfwritten
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	TRACE("(%p,%p,%ld,%p)\n",This,lpwf,wfsize,wfwritten);

	if (wfsize>sizeof(This->wfx))
		wfsize = sizeof(This->wfx);
	if (lpwf) {	/* NULL is valid */
		memcpy(lpwf,&(This->wfx),wfsize);
		if (wfwritten)
			*wfwritten = wfsize;
	} else
		if (wfwritten)
			*wfwritten = sizeof(This->wfx);
		else
			return DSERR_INVALIDPARAM;

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_Lock(
	LPDIRECTSOUNDBUFFER iface,DWORD writecursor,DWORD writebytes,LPVOID lplpaudioptr1,LPDWORD audiobytes1,LPVOID lplpaudioptr2,LPDWORD audiobytes2,DWORD flags
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	DWORD capf;

	TRACE("(%p,%ld,%ld,%p,%p,%p,%p,0x%08lx) at %ld\n",
		This,
		writecursor,
		writebytes,
		lplpaudioptr1,
		audiobytes1,
		lplpaudioptr2,
		audiobytes2,
		flags,
		GetTickCount()
	);

	if (flags & DSBLOCK_FROMWRITECURSOR) {
		DWORD writepos;
		/* GetCurrentPosition does too much magic to duplicate here */
		IDirectSoundBufferImpl_GetCurrentPosition(iface, NULL, &writepos);
		writecursor += writepos;
	}
	if (flags & DSBLOCK_ENTIREBUFFER)
		writebytes = This->buflen;
	if (writebytes > This->buflen)
		writebytes = This->buflen;

	assert(audiobytes1!=audiobytes2);
	assert(lplpaudioptr1!=lplpaudioptr2);

	if ((writebytes == This->buflen) &&
	    ((This->state == STATE_STARTING) ||
	     (This->state == STATE_PLAYING)))
		/* some games, like Half-Life, try to be clever (not) and
		 * keep one secondary buffer, and mix sounds into it itself,
		 * locking the entire buffer every time... so we can just forget
		 * about tracking the last-written-to-position... */
		This->probably_valid_to = (DWORD)-1;
	else
		This->probably_valid_to = writecursor;

	if (This->dsbd.dwFlags & DSBCAPS_PRIMARYBUFFER)
		capf = DSDDESC_DONTNEEDPRIMARYLOCK;
	else
		capf = DSDDESC_DONTNEEDSECONDARYLOCK;
	if (!(This->dsound->drvdesc.dwFlags & capf) && This->hwbuf) {
		IDsDriverBuffer_Lock(This->hwbuf,
				     lplpaudioptr1, audiobytes1,
				     lplpaudioptr2, audiobytes2,
				     writecursor, writebytes,
				     0);
	}
	else {
		BOOL remix = FALSE;
		if (writecursor+writebytes <= This->buflen) {
			*(LPBYTE*)lplpaudioptr1 = This->buffer+writecursor;
			*audiobytes1 = writebytes;
			if (lplpaudioptr2)
				*(LPBYTE*)lplpaudioptr2 = NULL;
			if (audiobytes2)
				*audiobytes2 = 0;
			TRACE("->%ld.0\n",writebytes);
		} else {
			*(LPBYTE*)lplpaudioptr1 = This->buffer+writecursor;
			*audiobytes1 = This->buflen-writecursor;
			if (lplpaudioptr2)
				*(LPBYTE*)lplpaudioptr2 = This->buffer;
			if (audiobytes2)
				*audiobytes2 = writebytes-(This->buflen-writecursor);
			TRACE("->%ld.%ld\n",*audiobytes1,audiobytes2?*audiobytes2:0);
		}
		/* if the segment between playpos and buf_mixpos is touched,
		 * we need to cancel some mixing */
		if (This->buf_mixpos >= This->playpos) {
			if (This->buf_mixpos > writecursor &&
			    This->playpos <= writecursor+writebytes)
				remix = TRUE;
		}
		else {
			if (This->buf_mixpos > writecursor ||
			    This->playpos <= writecursor+writebytes)
				remix = TRUE;
		}
		if (remix) {
			TRACE("locking prebuffered region, ouch\n");
			DSOUND_MixCancelAt(This, writecursor);
		}
	}
	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_SetCurrentPosition(
	LPDIRECTSOUNDBUFFER iface,DWORD newpos
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	TRACE("(%p,%ld)\n",This,newpos);

	/* **** */
	EnterCriticalSection(&(This->lock));

	This->buf_mixpos = newpos;
	if (This->hwbuf)
		IDsDriverBuffer_SetPosition(This->hwbuf, This->buf_mixpos);

	LeaveCriticalSection(&(This->lock));
	/* **** */

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_SetPan(
	LPDIRECTSOUNDBUFFER iface,LONG pan
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);

	TRACE("(%p,%ld)\n",This,pan);

	if ((pan > DSBPAN_RIGHT) || (pan < DSBPAN_LEFT))
		return DSERR_INVALIDPARAM;

	/* You cannot set the pan of the primary buffer */
	/* and you cannot use both pan and 3D controls */
	if (!(This->dsbd.dwFlags & DSBCAPS_CTRLPAN) ||
	    (This->dsbd.dwFlags & DSBCAPS_CTRL3D) ||
	    (This->dsbd.dwFlags & DSBCAPS_PRIMARYBUFFER))
		return DSERR_CONTROLUNAVAIL;

	/* **** */
	EnterCriticalSection(&(This->lock));

	This->volpan.lPan = pan;

	DSOUND_RecalcVolPan(&(This->volpan));

	if (This->hwbuf) {
		IDsDriverBuffer_SetVolumePan(This->hwbuf, &(This->volpan));
	}

	LeaveCriticalSection(&(This->lock));
	/* **** */

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_GetPan(
	LPDIRECTSOUNDBUFFER iface,LPLONG pan
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	TRACE("(%p,%p)\n",This,pan);

	if (pan == NULL)
		return DSERR_INVALIDPARAM;

	*pan = This->volpan.lPan;

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_Unlock(
	LPDIRECTSOUNDBUFFER iface,LPVOID p1,DWORD x1,LPVOID p2,DWORD x2
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	DWORD capf, probably_valid_to;

	TRACE("(%p,%p,%ld,%p,%ld):stub\n", This,p1,x1,p2,x2);

#if 0
	/* Preprocess 3D buffers... */

	/* This is highly experimental and liable to break things */
	if (This->dsbd.dwFlags & DSBCAPS_CTRL3D)
		DSOUND_Create3DBuffer(This);
#endif

	if (This->dsbd.dwFlags & DSBCAPS_PRIMARYBUFFER)
		capf = DSDDESC_DONTNEEDPRIMARYLOCK;
	else
		capf = DSDDESC_DONTNEEDSECONDARYLOCK;
	if (!(This->dsound->drvdesc.dwFlags & capf) && This->hwbuf) {
		IDsDriverBuffer_Unlock(This->hwbuf, p1, x1, p2, x2);
	}

	if (p2) probably_valid_to = (((LPBYTE)p2)-This->buffer) + x2;
	else probably_valid_to = (((LPBYTE)p1)-This->buffer) + x1;
	while (probably_valid_to >= This->buflen)
		probably_valid_to -= This->buflen;
	if ((probably_valid_to == 0) && ((x1+x2) == This->buflen) &&
	    ((This->state == STATE_STARTING) ||
	     (This->state == STATE_PLAYING)))
		/* see IDirectSoundBufferImpl_Lock */
		probably_valid_to = (DWORD)-1;
	This->probably_valid_to = probably_valid_to;

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_Restore(
	LPDIRECTSOUNDBUFFER iface
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	FIXME("(%p):stub\n",This);
	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_GetFrequency(
	LPDIRECTSOUNDBUFFER iface,LPDWORD freq
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	TRACE("(%p,%p)\n",This,freq);

	if (freq == NULL)
		return DSERR_INVALIDPARAM;

	*freq = This->freq;
	TRACE("-> %ld\n", *freq);

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_Initialize(
	LPDIRECTSOUNDBUFFER iface,LPDIRECTSOUND dsound,LPDSBUFFERDESC dbsd
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	FIXME("(%p,%p,%p):stub\n",This,dsound,dbsd);
	DPRINTF("Re-Init!!!\n");
	return DSERR_ALREADYINITIALIZED;
}

static HRESULT WINAPI IDirectSoundBufferImpl_GetCaps(
	LPDIRECTSOUNDBUFFER iface,LPDSBCAPS caps
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
  	TRACE("(%p)->(%p)\n",This,caps);
  
	if (caps == NULL)
		return DSERR_INVALIDPARAM;

	/* I think we should check this value, not set it. See */
	/* Inside DirectX, p215. That should apply here, too. */
	caps->dwSize = sizeof(*caps);

	caps->dwFlags = This->dsbd.dwFlags;
	if (This->hwbuf) caps->dwFlags |= DSBCAPS_LOCHARDWARE;
	else caps->dwFlags |= DSBCAPS_LOCSOFTWARE;

	caps->dwBufferBytes = This->buflen;

	/* This value represents the speed of the "unlock" command.
	   As unlock is quite fast (it does not do anything), I put
	   4096 ko/s = 4 Mo / s */
	/* FIXME: hwbuf speed */
	caps->dwUnlockTransferRate = 4096;
	caps->dwPlayCpuOverhead = 0;

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_QueryInterface(
	LPDIRECTSOUNDBUFFER iface,REFIID riid,LPVOID *ppobj
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);

	TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);

	if ( IsEqualGUID( &IID_IDirectSoundNotify, riid ) ) {
		IDirectSoundNotifyImpl	*dsn;

		dsn = (IDirectSoundNotifyImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(*dsn));
		dsn->ref = 1;
		dsn->dsb = This;
		IDirectSoundBuffer_AddRef(iface);
		ICOM_VTBL(dsn) = &dsnvt;
		*ppobj = (LPVOID)dsn;
		return S_OK;
	}

#ifdef USE_DSOUND3D
	if ( IsEqualGUID( &IID_IDirectSound3DBuffer, riid ) ) {
                IDirectSound3DBufferImpl        *ds3db;

		*ppobj = This->ds3db;
		if (*ppobj) {
			IDirectSound3DBuffer_AddRef((LPDIRECTSOUND3DBUFFER)This->ds3db);
			return S_OK;
		}

                ds3db = (IDirectSound3DBufferImpl*)HeapAlloc(GetProcessHeap(),
                        0,sizeof(*ds3db));
                ds3db->ref = 1;
                ds3db->dsb = This;
                ICOM_VTBL(ds3db) = &ds3dbvt;
		InitializeCriticalSection(&ds3db->lock);

		IDirectSoundBuffer_AddRef(iface);

                ds3db->ds3db.dwSize = sizeof(DS3DBUFFER);
                ds3db->ds3db.vPosition.u1.x = 0.0;
                ds3db->ds3db.vPosition.u2.y = 0.0;
                ds3db->ds3db.vPosition.u3.z = 0.0;
                ds3db->ds3db.vVelocity.u1.x = 0.0;
                ds3db->ds3db.vVelocity.u2.y = 0.0;
                ds3db->ds3db.vVelocity.u3.z = 0.0;
                ds3db->ds3db.dwInsideConeAngle = DS3D_DEFAULTCONEANGLE;
                ds3db->ds3db.dwOutsideConeAngle = DS3D_DEFAULTCONEANGLE;
                ds3db->ds3db.vConeOrientation.u1.x = 0.0;
                ds3db->ds3db.vConeOrientation.u2.y = 0.0;
                ds3db->ds3db.vConeOrientation.u3.z = 0.0;
                ds3db->ds3db.lConeOutsideVolume = DS3D_DEFAULTCONEOUTSIDEVOLUME;                ds3db->ds3db.flMinDistance = DS3D_DEFAULTMINDISTANCE;
                ds3db->ds3db.flMaxDistance = DS3D_DEFAULTMAXDISTANCE;
                ds3db->ds3db.dwMode = DS3DMODE_NORMAL;
                ds3db->buflen = (This->buflen * primarybuf->wfx.nBlockAlign) /
                        This->wfx.nBlockAlign;
                ds3db->buffer = HeapAlloc(GetProcessHeap(), 0, ds3db->buflen);
                if (ds3db->buffer == NULL) {
                        ds3db->buflen = 0;
                        ds3db->ds3db.dwMode = DS3DMODE_DISABLE;
                }

		ds3db->iks = (IKsPropertySetImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(*(ds3db->iks)));
		ds3db->iks->ref = 1;
		ds3db->iks->ds3db = ds3db;
		ICOM_VTBL(ds3db->iks) = &iksvt;

		return S_OK;
	}
#else
	if ( IsEqualGUID( &IID_IDirectSound3DBuffer, riid ) ) {
		FIXME("%s: I know about this GUID, but don't support it yet\n",
		      debugstr_guid( riid ));
		*ppobj = NULL;
		return E_FAIL;
	}
#endif

#if USE_DSOUND3D
        if ( IsEqualGUID( &IID_IDirectSound3DListener, riid ) ) {
		IDirectSound3DListenerImpl* dsl;

		if (This->dsound->listener) {
			*ppobj = This->dsound->listener;
                        IDirectSound3DListener_AddRef((LPDIRECTSOUND3DLISTENER)This->dsound->listener);
                        return DS_OK;
		}

		dsl = (IDirectSound3DListenerImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(*dsl));
		dsl->ref = 1;
		ICOM_VTBL(dsl) = &ds3dlvt;
		*ppobj = (LPVOID)dsl;

                dsl->ds3dl.dwSize = sizeof(DS3DLISTENER);
                dsl->ds3dl.vPosition.u1.x = 0.0;
                dsl->ds3dl.vPosition.u2.y = 0.0;
                dsl->ds3dl.vPosition.u3.z = 0.0;
                dsl->ds3dl.vVelocity.u1.x = 0.0;
                dsl->ds3dl.vVelocity.u2.y = 0.0;
                dsl->ds3dl.vVelocity.u3.z = 0.0;
                dsl->ds3dl.vOrientFront.u1.x = 0.0;
                dsl->ds3dl.vOrientFront.u2.y = 0.0;
                dsl->ds3dl.vOrientFront.u3.z = 1.0;
                dsl->ds3dl.vOrientTop.u1.x = 0.0;
                dsl->ds3dl.vOrientTop.u2.y = 1.0;
                dsl->ds3dl.vOrientTop.u3.z = 0.0;
                dsl->ds3dl.flDistanceFactor = DS3D_DEFAULTDISTANCEFACTOR;
                dsl->ds3dl.flRolloffFactor = DS3D_DEFAULTROLLOFFFACTOR;

		InitializeCriticalSection(&dsl->lock);

		dsl->dsb = This;
		IDirectSoundBuffer_AddRef(iface);

		This->dsound->listener = dsl;
		IDirectSound3DListener_AddRef((LPDIRECTSOUND3DLISTENER)dsl);

		return S_OK;
	}
#else
	if ( IsEqualGUID( &IID_IDirectSound3DListener, riid ) ) {
		FIXME("%s: I know about this GUID, but don't support it yet\n",
		      debugstr_guid( riid ));
		*ppobj = NULL;
		return E_FAIL;
	}
#endif

	FIXME( "Unknown GUID %s\n", debugstr_guid( riid ) );

	*ppobj = NULL;

	return E_FAIL;
}

static ICOM_VTABLE(IDirectSoundBuffer) dsbvt = 
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectSoundBufferImpl_QueryInterface,
	IDirectSoundBufferImpl_AddRef,
	IDirectSoundBufferImpl_Release,
	IDirectSoundBufferImpl_GetCaps,
	IDirectSoundBufferImpl_GetCurrentPosition,
	IDirectSoundBufferImpl_GetFormat,
	IDirectSoundBufferImpl_GetVolume,
	IDirectSoundBufferImpl_GetPan,
        IDirectSoundBufferImpl_GetFrequency,
	IDirectSoundBufferImpl_GetStatus,
	IDirectSoundBufferImpl_Initialize,
	IDirectSoundBufferImpl_Lock,
	IDirectSoundBufferImpl_Play,
	IDirectSoundBufferImpl_SetCurrentPosition,
	IDirectSoundBufferImpl_SetFormat,
	IDirectSoundBufferImpl_SetVolume,
	IDirectSoundBufferImpl_SetPan,
	IDirectSoundBufferImpl_SetFrequency,
	IDirectSoundBufferImpl_Stop,
	IDirectSoundBufferImpl_Unlock,
	IDirectSoundBufferImpl_Restore
};

/*******************************************************************************
 *		IDirectSound
 */

static HRESULT WINAPI IDirectSoundImpl_SetCooperativeLevel(
	LPDIRECTSOUND iface,HWND hwnd,DWORD level
) {
	ICOM_THIS(IDirectSoundImpl,iface);

	FIXME("(%p,%08lx,%ld):stub\n",This,(DWORD)hwnd,level);

	This->priolevel = level;

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundImpl_CreateSoundBuffer(
	LPDIRECTSOUND iface,LPDSBUFFERDESC dsbd,LPLPDIRECTSOUNDBUFFER ppdsb,LPUNKNOWN lpunk
) {
	ICOM_THIS(IDirectSoundImpl,iface);
	IDirectSoundBufferImpl** ippdsb=(IDirectSoundBufferImpl**)ppdsb;
	LPWAVEFORMATEX	wfex;
	HRESULT err = DS_OK;

	TRACE("(%p,%p,%p,%p)\n",This,dsbd,ippdsb,lpunk);
	
	if ((This == NULL) || (dsbd == NULL) || (ippdsb == NULL))
		return DSERR_INVALIDPARAM;
	
	if (TRACE_ON(dsound)) {
		TRACE("(structsize=%ld)\n",dsbd->dwSize);
		TRACE("(flags=0x%08lx:\n",dsbd->dwFlags);
		_dump_DSBCAPS(dsbd->dwFlags);
		DPRINTF(")\n");
		TRACE("(bufferbytes=%ld)\n",dsbd->dwBufferBytes);
		TRACE("(lpwfxFormat=%p)\n",dsbd->lpwfxFormat);
	}

	wfex = dsbd->lpwfxFormat;

	if (wfex)
		TRACE("(formattag=0x%04x,chans=%d,samplerate=%ld,"
		   "bytespersec=%ld,blockalign=%d,bitspersamp=%d,cbSize=%d)\n",
		   wfex->wFormatTag, wfex->nChannels, wfex->nSamplesPerSec,
		   wfex->nAvgBytesPerSec, wfex->nBlockAlign, 
		   wfex->wBitsPerSample, wfex->cbSize);

	if (dsbd->dwFlags & DSBCAPS_PRIMARYBUFFER) {
		if (primarybuf) {
			IDirectSoundBuffer_AddRef((LPDIRECTSOUNDBUFFER)primarybuf);
			*ippdsb = primarybuf;
			primarybuf->dsbd.dwFlags = dsbd->dwFlags;
			return DS_OK;
		} /* Else create primary buffer */
	} else {
		if (dsbd->dwBufferBytes < DSBSIZE_MIN || dsbd->dwBufferBytes > DSBSIZE_MAX) {
			ERR("invalid sound buffer size %ld\n", dsbd->dwBufferBytes);
			return DSERR_INVALIDPARAM; /* FIXME: which error? */
		}
	}

	*ippdsb = (IDirectSoundBufferImpl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectSoundBufferImpl));
	if (*ippdsb == NULL)
		return DSERR_OUTOFMEMORY;
	ICOM_VTBL(*ippdsb) = &dsbvt;
	(*ippdsb)->ref = 1;
	(*ippdsb)->dsound = This;
	(*ippdsb)->parent = NULL;
	(*ippdsb)->buffer = NULL;

	memcpy(&((*ippdsb)->dsbd),dsbd,sizeof(*dsbd));
	if (dsbd->lpwfxFormat)
		memcpy(&((*ippdsb)->wfx), dsbd->lpwfxFormat, sizeof((*ippdsb)->wfx));

	TRACE("Created buffer at %p\n", *ippdsb);

	if (dsbd->dwFlags & DSBCAPS_PRIMARYBUFFER) {
		(*ippdsb)->buflen = dsound->wfx.nAvgBytesPerSec;
		(*ippdsb)->freq = dsound->wfx.nSamplesPerSec;

		/* FIXME: verify that hardware capabilities (DSCAPS_PRIMARY flags) match */

		if (This->driver) {
			err = IDsDriver_CreateSoundBuffer(This->driver,wfex,dsbd->dwFlags,0,
							  &((*ippdsb)->buflen),&((*ippdsb)->buffer),
							  (LPVOID*)&((*ippdsb)->hwbuf));
		}
		if (err == DS_OK)
			err = DSOUND_PrimaryOpen(*ippdsb);
	} else {
		DWORD capf = 0;
		int use_hw;

		(*ippdsb)->buflen = dsbd->dwBufferBytes;
		(*ippdsb)->freq = dsbd->lpwfxFormat->nSamplesPerSec;

		/* Check necessary hardware mixing capabilities */
		if (wfex->nChannels==2) capf |= DSCAPS_SECONDARYSTEREO;
		else capf |= DSCAPS_SECONDARYMONO;
		if (wfex->wBitsPerSample==16) capf |= DSCAPS_SECONDARY16BIT;
		else capf |= DSCAPS_SECONDARY8BIT;
		use_hw = (This->drvcaps.dwFlags & capf) == capf;

		/* FIXME: check hardware sample rate mixing capabilities */
		/* FIXME: check app hints for software/hardware buffer (STATIC, LOCHARDWARE, etc) */
		/* FIXME: check whether any hardware buffers are left */
		/* FIXME: handle DSDHEAP_CREATEHEAP for hardware buffers */

		/* Allocate system memory if applicable */
		if ((This->drvdesc.dwFlags & DSDDESC_USESYSTEMMEMORY) || !use_hw) {
			(*ippdsb)->buffer = (LPBYTE)HeapAlloc(GetProcessHeap(),0,(*ippdsb)->buflen);
			if ((*ippdsb)->buffer == NULL)
				err = DSERR_OUTOFMEMORY;
		}

		/* Allocate the hardware buffer */
		if (use_hw && (err == DS_OK)) {
			err = IDsDriver_CreateSoundBuffer(This->driver,wfex,dsbd->dwFlags,0,
							  &((*ippdsb)->buflen),&((*ippdsb)->buffer),
							  (LPVOID*)&((*ippdsb)->hwbuf));
		}
	}

	if (err != DS_OK) {
		if ((*ippdsb)->buffer)
			HeapFree(GetProcessHeap(),0,(*ippdsb)->buffer);
		HeapFree(GetProcessHeap(),0,(*ippdsb));
		*ippdsb = NULL;
		return err;
	}
	/* calculate fragment size and write lead */
	DSOUND_RecalcFormat(*ippdsb);

	/* It's not necessary to initialize values to zero since */
	/* we allocated this structure with HEAP_ZERO_MEMORY... */
	(*ippdsb)->playpos = 0;
	(*ippdsb)->buf_mixpos = 0;
	(*ippdsb)->state = STATE_STOPPED;
	DSOUND_RecalcVolPan(&((*ippdsb)->volpan));

	if (!(dsbd->dwFlags & DSBCAPS_PRIMARYBUFFER)) {
		(*ippdsb)->freqAdjust = ((*ippdsb)->freq << DSOUND_FREQSHIFT) /
			primarybuf->wfx.nSamplesPerSec;
		(*ippdsb)->nAvgBytesPerSec = (*ippdsb)->freq *
			dsbd->lpwfxFormat->nBlockAlign;
	}

	EnterCriticalSection(&(This->lock));
	/* register buffer */
	if (!(dsbd->dwFlags & DSBCAPS_PRIMARYBUFFER)) {
		IDirectSoundBufferImpl **newbuffers = (IDirectSoundBufferImpl**)HeapReAlloc(GetProcessHeap(),0,This->buffers,sizeof(IDirectSoundBufferImpl*)*(This->nrofbuffers+1));
		if (newbuffers) {
			This->buffers = newbuffers;
			This->buffers[This->nrofbuffers] = *ippdsb;
			This->nrofbuffers++;
			TRACE("buffer count is now %d\n", This->nrofbuffers);
		} else {
			ERR("out of memory for buffer list! Current buffer count is %d\n", This->nrofbuffers);
			err = DSERR_OUTOFMEMORY;
		}
	}
	LeaveCriticalSection(&(This->lock));

	IDirectSound_AddRef(iface);

	InitializeCriticalSection(&((*ippdsb)->lock));

	if (err != DS_OK) {
		/* oops... */
		IDirectSoundBuffer_Release(*ppdsb);
		*ippdsb = NULL;
		return err;
	}
	
#ifdef USE_DSOUND3D
	if (dsbd->dwFlags & DSBCAPS_CTRL3D) {
		IDirectSound3DBufferImpl	*ds3db;

		ds3db = (IDirectSound3DBufferImpl*)HeapAlloc(GetProcessHeap(),
			0,sizeof(*ds3db));
		ICOM_VTBL(ds3db) = &ds3dbvt;
		ds3db->ref = 1;
		(*ippdsb)->ds3db = ds3db;

		ds3db->dsb = (*ippdsb);
		IDirectSoundBufferImpl_AddRef((LPDIRECTSOUNDBUFFER)(*ippdsb));

		InitializeCriticalSection(&ds3db->lock);

		ds3db->ds3db.dwSize = sizeof(DS3DBUFFER);
		ds3db->ds3db.vPosition.u1.x = 0.0;
		ds3db->ds3db.vPosition.u2.y = 0.0;
		ds3db->ds3db.vPosition.u3.z = 0.0;
		ds3db->ds3db.vVelocity.u1.x = 0.0;
		ds3db->ds3db.vVelocity.u2.y = 0.0;
		ds3db->ds3db.vVelocity.u3.z = 0.0;
		ds3db->ds3db.dwInsideConeAngle = DS3D_DEFAULTCONEANGLE;
		ds3db->ds3db.dwOutsideConeAngle = DS3D_DEFAULTCONEANGLE;
		ds3db->ds3db.vConeOrientation.u1.x = 0.0;
		ds3db->ds3db.vConeOrientation.u2.y = 0.0;
		ds3db->ds3db.vConeOrientation.u3.z = 0.0;
		ds3db->ds3db.lConeOutsideVolume = DS3D_DEFAULTCONEOUTSIDEVOLUME;
		ds3db->ds3db.flMinDistance = DS3D_DEFAULTMINDISTANCE;
		ds3db->ds3db.flMaxDistance = DS3D_DEFAULTMAXDISTANCE;
		ds3db->ds3db.dwMode = DS3DMODE_NORMAL;
		ds3db->buflen = ((*ippdsb)->buflen * primarybuf->wfx.nBlockAlign) /
			(*ippdsb)->wfx.nBlockAlign;
		ds3db->buffer = HeapAlloc(GetProcessHeap(), 0, ds3db->buflen);
		if (ds3db->buffer == NULL) {
			ds3db->buflen = 0;
			ds3db->ds3db.dwMode = DS3DMODE_DISABLE;
		}
		ds3db->iks = (IKsPropertySetImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(*(ds3db->iks)));
		ds3db->iks->ref = 1;
		ds3db->iks->ds3db = ds3db;
		ICOM_VTBL(ds3db->iks) = &iksvt;

	}
#endif
	return DS_OK;
}

static HRESULT WINAPI IDirectSoundImpl_DuplicateSoundBuffer(
	LPDIRECTSOUND iface,LPDIRECTSOUNDBUFFER pdsb,LPLPDIRECTSOUNDBUFFER ppdsb
) {
	ICOM_THIS(IDirectSoundImpl,iface);
	IDirectSoundBufferImpl* ipdsb=(IDirectSoundBufferImpl*)pdsb;
	IDirectSoundBufferImpl** ippdsb=(IDirectSoundBufferImpl**)ppdsb;
	TRACE("(%p,%p,%p)\n",This,ipdsb,ippdsb);

	if (ipdsb->hwbuf) {
		FIXME("need to duplicate hardware buffer\n");
	}

	*ippdsb = (IDirectSoundBufferImpl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectSoundBufferImpl));

	IDirectSoundBuffer_AddRef(pdsb);
	memcpy(*ippdsb, ipdsb, sizeof(IDirectSoundBufferImpl));
	(*ippdsb)->ref = 1;
	(*ippdsb)->state = STATE_STOPPED;
	(*ippdsb)->playpos = 0;
	(*ippdsb)->buf_mixpos = 0;
	(*ippdsb)->dsound = This;
	(*ippdsb)->parent = ipdsb;
	memcpy(&((*ippdsb)->wfx), &(ipdsb->wfx), sizeof((*ippdsb)->wfx));
	InitializeCriticalSection(&(*ippdsb)->lock);
	/* register buffer */
	EnterCriticalSection(&(This->lock));
	{
		IDirectSoundBufferImpl **newbuffers = (IDirectSoundBufferImpl**)HeapReAlloc(GetProcessHeap(),0,This->buffers,sizeof(IDirectSoundBufferImpl**)*(This->nrofbuffers+1));
		if (newbuffers) {
			This->buffers = newbuffers;
			This->buffers[This->nrofbuffers] = *ippdsb;
			This->nrofbuffers++;
			TRACE("buffer count is now %d\n", This->nrofbuffers);
		} else {
			ERR("out of memory for buffer list! Current buffer count is %d\n", This->nrofbuffers);
			/* FIXME: release buffer */
		}
	}
	LeaveCriticalSection(&(This->lock));
	IDirectSound_AddRef(iface);
	return DS_OK;
}


static HRESULT WINAPI IDirectSoundImpl_GetCaps(LPDIRECTSOUND iface,LPDSCAPS caps) {
	ICOM_THIS(IDirectSoundImpl,iface);
	TRACE("(%p,%p)\n",This,caps);
	TRACE("(flags=0x%08lx)\n",caps->dwFlags);

	if (caps == NULL)
		return DSERR_INVALIDPARAM;

	/* We should check this value, not set it. See Inside DirectX, p215. */
	caps->dwSize = sizeof(*caps);

	caps->dwFlags = This->drvcaps.dwFlags;

	/* FIXME: copy caps from This->drvcaps */
	caps->dwMinSecondarySampleRate		= DSBFREQUENCY_MIN;
	caps->dwMaxSecondarySampleRate		= DSBFREQUENCY_MAX;

	caps->dwPrimaryBuffers			= 1;

	caps->dwMaxHwMixingAllBuffers		= 0;
	caps->dwMaxHwMixingStaticBuffers	= 0;
	caps->dwMaxHwMixingStreamingBuffers	= 0;

	caps->dwFreeHwMixingAllBuffers		= 0;
	caps->dwFreeHwMixingStaticBuffers	= 0;
	caps->dwFreeHwMixingStreamingBuffers	= 0;

	caps->dwMaxHw3DAllBuffers		= 0;
	caps->dwMaxHw3DStaticBuffers		= 0;
	caps->dwMaxHw3DStreamingBuffers		= 0;

	caps->dwFreeHw3DAllBuffers		= 0;
	caps->dwFreeHw3DStaticBuffers		= 0;
	caps->dwFreeHw3DStreamingBuffers	= 0;

	caps->dwTotalHwMemBytes			= 0;

	caps->dwFreeHwMemBytes			= 0;

	caps->dwMaxContigFreeHwMemBytes		= 0;

	caps->dwUnlockTransferRateHwBuffers	= 4096;	/* But we have none... */

	caps->dwPlayCpuOverheadSwBuffers	= 1;	/* 1% */

	return DS_OK;
}

static ULONG WINAPI IDirectSoundImpl_AddRef(LPDIRECTSOUND iface) {
	ICOM_THIS(IDirectSoundImpl,iface);
	return ++(This->ref);
}

static ULONG WINAPI IDirectSoundImpl_Release(LPDIRECTSOUND iface) {
	ICOM_THIS(IDirectSoundImpl,iface);
	TRACE("(%p), ref was %ld\n",This,This->ref);
	if (!--(This->ref)) {
		UINT i;

		timeKillEvent(This->timerID);
		timeEndPeriod(DS_TIME_RES);

		if (primarybuf)
			IDirectSoundBuffer_Release((LPDIRECTSOUNDBUFFER)primarybuf);

		if (This->buffers) {
			for( i=0;i<This->nrofbuffers;i++)	
				IDirectSoundBuffer_Release((LPDIRECTSOUNDBUFFER)This->buffers[i]);
		}

		if (This->primary)
			IDirectSoundBuffer_Release((LPDIRECTSOUNDBUFFER)This->primary);

		DeleteCriticalSection(&This->lock);
		if (This->driver) {
			IDsDriver_Close(This->driver);
		} else {
			unsigned c;
			for (c=0; c<DS_HEL_FRAGS; c++)
				HeapFree(GetProcessHeap(),0,This->pwave[c]);
		}
		if (This->drvdesc.dwFlags & DSDDESC_DOMMSYSTEMOPEN) {
			waveOutClose(This->hwo);
		}
		if (This->driver)
			IDsDriver_Release(This->driver);

		HeapFree(GetProcessHeap(),0,This);
		dsound = NULL;
		return 0;
	}
	return This->ref;
}

static HRESULT WINAPI IDirectSoundImpl_SetSpeakerConfig(
	LPDIRECTSOUND iface,DWORD config
) {
	ICOM_THIS(IDirectSoundImpl,iface);
	FIXME("(%p,0x%08lx):stub\n",This,config);
	return DS_OK;
}

static HRESULT WINAPI IDirectSoundImpl_QueryInterface(
	LPDIRECTSOUND iface,REFIID riid,LPVOID *ppobj
) {
	ICOM_THIS(IDirectSoundImpl,iface);

	if ( IsEqualGUID( &IID_IDirectSound3DListener, riid ) ) {

		if (This->listener) {
			*ppobj = This->listener;
			IDirectSound3DListener_AddRef((LPDIRECTSOUND3DLISTENER)This->listener);
			return DS_OK;
		}

		This->listener = (IDirectSound3DListenerImpl*)HeapAlloc(
			GetProcessHeap(), 0, sizeof(*(This->listener)));
		This->listener->ref = 1;
		ICOM_VTBL(This->listener) = &ds3dlvt;
		*ppobj = (LPVOID)This->listener;
		IDirectSound3DListener_AddRef((LPDIRECTSOUND3DLISTENER)*ppobj);	

		This->listener->dsb = NULL; 

		This->listener->ds3dl.dwSize = sizeof(DS3DLISTENER);
		This->listener->ds3dl.vPosition.u1.x = 0.0;
		This->listener->ds3dl.vPosition.u2.y = 0.0;
		This->listener->ds3dl.vPosition.u3.z = 0.0;
		This->listener->ds3dl.vVelocity.u1.x = 0.0;
		This->listener->ds3dl.vVelocity.u2.y = 0.0;
		This->listener->ds3dl.vVelocity.u3.z = 0.0;
		This->listener->ds3dl.vOrientFront.u1.x = 0.0;
		This->listener->ds3dl.vOrientFront.u2.y = 0.0;
		This->listener->ds3dl.vOrientFront.u3.z = 1.0;
		This->listener->ds3dl.vOrientTop.u1.x = 0.0;
		This->listener->ds3dl.vOrientTop.u2.y = 1.0;
		This->listener->ds3dl.vOrientTop.u3.z = 0.0;
		This->listener->ds3dl.flDistanceFactor = DS3D_DEFAULTDISTANCEFACTOR;
		This->listener->ds3dl.flRolloffFactor = DS3D_DEFAULTROLLOFFFACTOR;
		This->listener->ds3dl.flDopplerFactor = DS3D_DEFAULTDOPPLERFACTOR;

		InitializeCriticalSection(&This->listener->lock);

		return DS_OK;
	}

	FIXME("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
	return E_FAIL;
}

static HRESULT WINAPI IDirectSoundImpl_Compact(
	LPDIRECTSOUND iface)
{
	ICOM_THIS(IDirectSoundImpl,iface);
	TRACE("(%p)\n", This);
	return DS_OK;
}

static HRESULT WINAPI IDirectSoundImpl_GetSpeakerConfig(
	LPDIRECTSOUND iface,
	LPDWORD lpdwSpeakerConfig)
{
	ICOM_THIS(IDirectSoundImpl,iface);
	TRACE("(%p, %p)\n", This, lpdwSpeakerConfig);
	*lpdwSpeakerConfig = DSSPEAKER_STEREO | (DSSPEAKER_GEOMETRY_NARROW << 16);
	return DS_OK;
}

static HRESULT WINAPI IDirectSoundImpl_Initialize(
	LPDIRECTSOUND iface,
	LPCGUID lpcGuid)
{
	ICOM_THIS(IDirectSoundImpl,iface);
	TRACE("(%p, %p)\n", This, lpcGuid);
	return DS_OK;
}

static ICOM_VTABLE(IDirectSound) dsvt = 
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectSoundImpl_QueryInterface,
	IDirectSoundImpl_AddRef,
	IDirectSoundImpl_Release,
	IDirectSoundImpl_CreateSoundBuffer,
	IDirectSoundImpl_GetCaps,
	IDirectSoundImpl_DuplicateSoundBuffer,
	IDirectSoundImpl_SetCooperativeLevel,
	IDirectSoundImpl_Compact,
	IDirectSoundImpl_GetSpeakerConfig,
	IDirectSoundImpl_SetSpeakerConfig,
	IDirectSoundImpl_Initialize
};


static void DSOUND_CheckEvent(IDirectSoundBufferImpl *dsb, int len)
{
	int			i;
	DWORD			offset;
	LPDSBPOSITIONNOTIFY	event;

	if (dsb->nrofnotifies == 0)
		return;

	TRACE("(%p) buflen = %ld, playpos = %ld, len = %d\n",
		dsb, dsb->buflen, dsb->playpos, len);
	for (i = 0; i < dsb->nrofnotifies ; i++) {
		event = dsb->notifies + i;
		offset = event->dwOffset;
		TRACE("checking %d, position %ld, event = %d\n",
			i, offset, event->hEventNotify);
		/* DSBPN_OFFSETSTOP has to be the last element. So this is */
		/* OK. [Inside DirectX, p274] */
		/*  */
		/* This also means we can't sort the entries by offset, */
		/* because DSBPN_OFFSETSTOP == -1 */
		if (offset == DSBPN_OFFSETSTOP) {
			if (dsb->state == STATE_STOPPED) {
				SetEvent(event->hEventNotify);
				TRACE("signalled event %d (%d)\n", event->hEventNotify, i);
				return;
			} else
				return;
		}
		if ((dsb->playpos + len) >= dsb->buflen) {
			if ((offset < ((dsb->playpos + len) % dsb->buflen)) ||
			    (offset >= dsb->playpos)) {
				TRACE("signalled event %d (%d)\n", event->hEventNotify, i);
				SetEvent(event->hEventNotify);
			}
		} else {
			if ((offset >= dsb->playpos) && (offset < (dsb->playpos + len))) {
				TRACE("signalled event %d (%d)\n", event->hEventNotify, i);
				SetEvent(event->hEventNotify);
			}
		}
	}
}

/* WAV format info can be found at: */
/* */
/*	http://www.cwi.nl/ftp/audio/AudioFormats.part2 */
/*	ftp://ftp.cwi.nl/pub/audio/RIFF-format */
/* */
/* Import points to remember: */
/* */
/*	8-bit WAV is unsigned */
/*	16-bit WAV is signed */

static inline INT16 cvtU8toS16(BYTE byte)
{
	INT16	s = (byte - 128) << 8;

	return s;
}

static inline BYTE cvtS16toU8(INT16 word)
{
	BYTE	b = (word + 32768) >> 8;
	
	return b;
}


/* We should be able to optimize these two inline functions */
/* so that we aren't doing 8->16->8 conversions when it is */
/* not necessary. But this is still a WIP. Optimize later. */
static inline void get_fields(const IDirectSoundBufferImpl *dsb, BYTE *buf, INT *fl, INT *fr)
{
	INT16	*bufs = (INT16 *) buf;

	/* TRACE("(%p)\n", buf); */
	if ((dsb->wfx.wBitsPerSample == 8) && dsb->wfx.nChannels == 2) {
		*fl = cvtU8toS16(*buf);
		*fr = cvtU8toS16(*(buf + 1));
		return;
	}

	if ((dsb->wfx.wBitsPerSample == 16) && dsb->wfx.nChannels == 2) {
		*fl = *bufs;
		*fr = *(bufs + 1);
		return;
	}

	if ((dsb->wfx.wBitsPerSample == 8) && dsb->wfx.nChannels == 1) {
		*fl = cvtU8toS16(*buf);
		*fr = *fl;
		return;
	}

	if ((dsb->wfx.wBitsPerSample == 16) && dsb->wfx.nChannels == 1) {
		*fl = *bufs;
		*fr = *bufs;
		return;
	}

	FIXME("get_fields found an unsupported configuration\n");
	return;
}

static inline void set_fields(BYTE *buf, INT fl, INT fr)
{
	INT16 *bufs = (INT16 *) buf;

	if ((primarybuf->wfx.wBitsPerSample == 8) && (primarybuf->wfx.nChannels == 2)) {
		*buf = cvtS16toU8(fl);
		*(buf + 1) = cvtS16toU8(fr);
		return;
	}

	if ((primarybuf->wfx.wBitsPerSample == 16) && (primarybuf->wfx.nChannels == 2)) {
		*bufs = fl;
		*(bufs + 1) = fr;
		return;
	}

	if ((primarybuf->wfx.wBitsPerSample == 8) && (primarybuf->wfx.nChannels == 1)) {
		*buf = cvtS16toU8((fl + fr) >> 1);
		return;
	}

	if ((primarybuf->wfx.wBitsPerSample == 16) && (primarybuf->wfx.nChannels == 1)) {
		*bufs = (fl + fr) >> 1;
		return;
	}
	FIXME("set_fields found an unsupported configuration\n");
	return;
}

/* Now with PerfectPitch (tm) technology */
static INT DSOUND_MixerNorm(IDirectSoundBufferImpl *dsb, BYTE *buf, INT len)
{
	INT	i, size, ipos, ilen, fieldL, fieldR;
	BYTE	*ibp, *obp;
	INT	iAdvance = dsb->wfx.nBlockAlign;
	INT	oAdvance = primarybuf->wfx.nBlockAlign;

	ibp = dsb->buffer + dsb->buf_mixpos;
	obp = buf;

	TRACE("(%p, %p, %p), buf_mixpos=%ld\n", dsb, ibp, obp, dsb->buf_mixpos);
	/* Check for the best case */
	if ((dsb->freq == primarybuf->wfx.nSamplesPerSec) &&
	    (dsb->wfx.wBitsPerSample == primarybuf->wfx.wBitsPerSample) &&
	    (dsb->wfx.nChannels == primarybuf->wfx.nChannels)) {
	        DWORD bytesleft = dsb->buflen - dsb->buf_mixpos;
		TRACE("(%p) Best case\n", dsb);
	    	if (len <= bytesleft )
			memcpy(obp, ibp, len);
		else { /* wrap */
			memcpy(obp, ibp, bytesleft );
			memcpy(obp + bytesleft, dsb->buffer, len - bytesleft);
		}
		return len;
	}
	
	/* Check for same sample rate */
	if (dsb->freq == primarybuf->wfx.nSamplesPerSec) {
		TRACE("(%p) Same sample rate %ld = primary %ld\n", dsb,
			dsb->freq, primarybuf->wfx.nSamplesPerSec);
		ilen = 0;
		for (i = 0; i < len; i += oAdvance) {
			get_fields(dsb, ibp, &fieldL, &fieldR);
			ibp += iAdvance;
			ilen += iAdvance;
			set_fields(obp, fieldL, fieldR);
			obp += oAdvance;
			if (ibp >= (BYTE *)(dsb->buffer + dsb->buflen))
				ibp = dsb->buffer;	/* wrap */
		}
		return (ilen);	
	}

	/* Mix in different sample rates */
	/* */
	/* New PerfectPitch(tm) Technology (c) 1998 Rob Riggs */
	/* Patent Pending :-] */

	/* Patent enhancements (c) 2000 Ove Kåven,
	 * TransGaming Technologies Inc. */

	FIXME("(%p) Adjusting frequency: %ld -> %ld (need optimization)\n",
		dsb, dsb->freq, primarybuf->wfx.nSamplesPerSec);

	size = len / oAdvance;
	ilen = 0;
	ipos = dsb->buf_mixpos;
	for (i = 0; i < size; i++) {
		get_fields(dsb, (dsb->buffer + ipos), &fieldL, &fieldR);
		set_fields(obp, fieldL, fieldR);
		obp += oAdvance;

		dsb->freqAcc += dsb->freqAdjust;
		if (dsb->freqAcc >= (1<<DSOUND_FREQSHIFT)) {
			ULONG adv = (dsb->freqAcc>>DSOUND_FREQSHIFT) * iAdvance;
			dsb->freqAcc &= (1<<DSOUND_FREQSHIFT)-1;
			ipos += adv; ilen += adv;
			while (ipos >= dsb->buflen)
				ipos -= dsb->buflen;
		}
	}
	return ilen;
}

static void DSOUND_MixerVol(IDirectSoundBufferImpl *dsb, BYTE *buf, INT len)
{
	INT	i, inc = primarybuf->wfx.wBitsPerSample >> 3;
	BYTE	*bpc = buf;
	INT16	*bps = (INT16 *) buf;
	
	TRACE("(%p) left = %lx, right = %lx\n", dsb,
		dsb->volpan.dwTotalLeftAmpFactor, dsb->volpan.dwTotalRightAmpFactor);
	if ((!(dsb->dsbd.dwFlags & DSBCAPS_CTRLPAN) || (dsb->volpan.lPan == 0)) &&
	    (!(dsb->dsbd.dwFlags & DSBCAPS_CTRLVOLUME) || (dsb->volpan.lVolume == 0)) &&
	    !(dsb->dsbd.dwFlags & DSBCAPS_CTRL3D))
		return;		/* Nothing to do */

	/* If we end up with some bozo coder using panning or 3D sound */
	/* with a mono primary buffer, it could sound very weird using */
	/* this method. Oh well, tough patooties. */

	for (i = 0; i < len; i += inc) {
		INT	val;

		switch (inc) {

		case 1:
			/* 8-bit WAV is unsigned, but we need to operate */
			/* on signed data for this to work properly */
			val = *bpc - 128;
			val = ((val * (i & inc ? dsb->volpan.dwTotalRightAmpFactor : dsb->volpan.dwTotalLeftAmpFactor)) >> 16);
			*bpc = val + 128;
			bpc++;
			break;
		case 2:
			/* 16-bit WAV is signed -- much better */
			val = *bps;
			val = ((val * ((i & inc) ? dsb->volpan.dwTotalRightAmpFactor : dsb->volpan.dwTotalLeftAmpFactor)) >> 16);
			*bps = val;
			bps++;
			break;
		default:
			/* Very ugly! */
			FIXME("MixerVol had a nasty error\n");
		}
	}		
}

#ifdef USE_DSOUND3D
static void DSOUND_Mixer3D(IDirectSoundBufferImpl *dsb, BYTE *buf, INT len)
{
	BYTE	*ibp, *obp;
	DWORD	buflen, buf_mixpos;

	buflen = dsb->ds3db->buflen;
	buf_mixpos = (dsb->buf_mixpos * primarybuf->wfx.nBlockAlign) / dsb->wfx.nBlockAlign;
	ibp = dsb->ds3db->buffer + buf_mixpos;
	obp = buf;

	if (buf_mixpos > buflen) {
		FIXME("Major breakage\n");
		return;
	}

	if (len <= (buf_mixpos + buflen))
		memcpy(obp, ibp, len);
	else { /* wrap */
		memcpy(obp, ibp, buflen - buf_mixpos);
		memcpy(obp + (buflen - buf_mixpos),
		    dsb->buffer,
		    len - (buflen - buf_mixpos));
	}
	return;
}
#endif

static void *tmp_buffer;
static size_t tmp_buffer_len = 0;

static void *DSOUND_tmpbuffer(size_t len)
{
  if (len>tmp_buffer_len) {
    void *new_buffer = realloc(tmp_buffer, len);
    if (new_buffer) {
      tmp_buffer = new_buffer;
      tmp_buffer_len = len;
    }
    return new_buffer;
  }
  return tmp_buffer;
}

static DWORD DSOUND_MixInBuffer(IDirectSoundBufferImpl *dsb, DWORD writepos, DWORD fraglen)
{
	INT	i, len, ilen, temp, field;
	INT	advance = primarybuf->wfx.wBitsPerSample >> 3;
	BYTE	*buf, *ibuf, *obuf;
	INT16	*ibufs, *obufs;

	len = fraglen;
	if (!(dsb->playflags & DSBPLAY_LOOPING)) {
		temp = MulDiv(primarybuf->wfx.nAvgBytesPerSec, dsb->buflen,
			dsb->nAvgBytesPerSec) -
		       MulDiv(primarybuf->wfx.nAvgBytesPerSec, dsb->buf_mixpos,
			dsb->nAvgBytesPerSec);
		len = (len > temp) ? temp : len;
	}
	len &= ~3;				/* 4 byte alignment */

	if (len == 0) {
		/* This should only happen if we aren't looping and temp < 4 */

		/* We skip the remainder, so check for possible events */
		DSOUND_CheckEvent(dsb, dsb->buflen - dsb->buf_mixpos);
		/* Stop */
		dsb->state = STATE_STOPPED;
		dsb->playpos = 0;
		dsb->buf_mixpos = 0;
		dsb->leadin = FALSE;
		/* Check for DSBPN_OFFSETSTOP */
		DSOUND_CheckEvent(dsb, 0);
		return 0;
	}

	/* Been seeing segfaults in malloc() for some reason... */
	TRACE("allocating buffer (size = %d)\n", len);
	if ((buf = ibuf = (BYTE *) DSOUND_tmpbuffer(len)) == NULL)
		return 0;

	TRACE("MixInBuffer (%p) len = %d, dest = %ld\n", dsb, len, writepos);

	ilen = DSOUND_MixerNorm(dsb, ibuf, len);
	if ((dsb->dsbd.dwFlags & DSBCAPS_CTRLPAN) ||
	    (dsb->dsbd.dwFlags & DSBCAPS_CTRLVOLUME))
		DSOUND_MixerVol(dsb, ibuf, len);

	obuf = primarybuf->buffer + writepos;
	for (i = 0; i < len; i += advance) {
		obufs = (INT16 *) obuf;
		ibufs = (INT16 *) ibuf;
		if (primarybuf->wfx.wBitsPerSample == 8) {
			/* 8-bit WAV is unsigned */
			field = (*ibuf - 128);
			field += (*obuf - 128);
			field = field > 127 ? 127 : field;
			field = field < -128 ? -128 : field;
			*obuf = field + 128;
		} else {
			/* 16-bit WAV is signed */
			field = *ibufs;
			field += *obufs;
			field = field > 32767 ? 32767 : field;
			field = field < -32768 ? -32768 : field;
			*obufs = field;
		}
		ibuf += advance;
		obuf += advance;
		if (obuf >= (BYTE *)(primarybuf->buffer + primarybuf->buflen))
			obuf = primarybuf->buffer;
	}
	/* free(buf); */

	if (dsb->dsbd.dwFlags & DSBCAPS_CTRLPOSITIONNOTIFY)
		DSOUND_CheckEvent(dsb, ilen);

	if (dsb->leadin && (dsb->startpos > dsb->buf_mixpos) && (dsb->startpos <= dsb->buf_mixpos + ilen)) {
		/* HACK... leadin should be reset when the PLAY position reaches the startpos,
		 * not the MIX position... but if the sound buffer is bigger than our prebuffering
		 * (which must be the case for the streaming buffers that need this hack anyway)
		 * plus DS_HEL_MARGIN or equivalent, then this ought to work anyway. */
		dsb->leadin = FALSE;
	}

	dsb->buf_mixpos += ilen;
	
	if (dsb->buf_mixpos >= dsb->buflen) {
		if (!(dsb->playflags & DSBPLAY_LOOPING)) {
			dsb->state = STATE_STOPPED;
			dsb->playpos = 0;
			dsb->buf_mixpos = 0;
			dsb->leadin = FALSE;
			DSOUND_CheckEvent(dsb, 0);		/* For DSBPN_OFFSETSTOP */
		} else {
			/* wrap */
			while (dsb->buf_mixpos >= dsb->buflen)
				dsb->buf_mixpos -= dsb->buflen;
			if (dsb->leadin && (dsb->startpos <= dsb->buf_mixpos))
				dsb->leadin = FALSE; /* HACK: see above */
		}
	}

	return len;
}

static void DSOUND_PhaseCancel(IDirectSoundBufferImpl *dsb, DWORD writepos, DWORD len)
{
	INT     i, ilen, field;
	INT     advance = primarybuf->wfx.wBitsPerSample >> 3;
	BYTE	*buf, *ibuf, *obuf;
	INT16	*ibufs, *obufs;

	len &= ~3;				/* 4 byte alignment */

	TRACE("allocating buffer (size = %ld)\n", len);
	if ((buf = ibuf = (BYTE *) DSOUND_tmpbuffer(len)) == NULL)
		return;

	TRACE("PhaseCancel (%p) len = %ld, dest = %ld\n", dsb, len, writepos);

	ilen = DSOUND_MixerNorm(dsb, ibuf, len);
	if ((dsb->dsbd.dwFlags & DSBCAPS_CTRLPAN) ||
	    (dsb->dsbd.dwFlags & DSBCAPS_CTRLVOLUME))
		DSOUND_MixerVol(dsb, ibuf, len);

	/* subtract instead of add, to phase out premixed data */
	obuf = primarybuf->buffer + writepos;
	for (i = 0; i < len; i += advance) {
		obufs = (INT16 *) obuf;
		ibufs = (INT16 *) ibuf;
		if (primarybuf->wfx.wBitsPerSample == 8) {
			/* 8-bit WAV is unsigned */
			field = (*ibuf - 128);
			field -= (*obuf - 128);
			field = field > 127 ? 127 : field;
			field = field < -128 ? -128 : field;
			*obuf = field + 128;
		} else {
			/* 16-bit WAV is signed */
			field = *ibufs;
			field -= *obufs;
			field = field > 32767 ? 32767 : field;
			field = field < -32768 ? -32768 : field;
			*obufs = field;
		}
		ibuf += advance;
		obuf += advance;
		if (obuf >= (BYTE *)(primarybuf->buffer + primarybuf->buflen))
			obuf = primarybuf->buffer;
	}
	/* free(buf); */
}

static void DSOUND_MixCancel(IDirectSoundBufferImpl *dsb, DWORD writepos, BOOL cancel)
{
	DWORD   size, flen, len, npos, nlen;
	INT	iAdvance = dsb->wfx.nBlockAlign;
	INT	oAdvance = primarybuf->wfx.nBlockAlign;
	/* determine amount of premixed data to cancel */
	DWORD primary_done =
		((dsb->primary_mixpos < writepos) ? primarybuf->buflen : 0) +
		dsb->primary_mixpos - writepos;

	TRACE("(%p, %ld), buf_mixpos=%ld\n", dsb, writepos, dsb->buf_mixpos);

	/* backtrack the mix position */
	size = primary_done / oAdvance;
	flen = size * dsb->freqAdjust;
	len = (flen >> DSOUND_FREQSHIFT) * iAdvance;
	flen &= (1<<DSOUND_FREQSHIFT)-1;
	while (dsb->freqAcc < flen) {
		len += iAdvance;
		dsb->freqAcc += 1<<DSOUND_FREQSHIFT;
	}
	len %= dsb->buflen;
	npos = ((dsb->buf_mixpos < len) ? dsb->buflen : 0) +
		dsb->buf_mixpos - len;
	if (dsb->leadin && (dsb->startpos > npos) && (dsb->startpos <= npos + len)) {
		/* stop backtracking at startpos */
		npos = dsb->startpos;
		len = ((dsb->buf_mixpos < npos) ? dsb->buflen : 0) +
			dsb->buf_mixpos - npos;
		flen = dsb->freqAcc;
		nlen = len / dsb->wfx.nBlockAlign;
		nlen = ((nlen << DSOUND_FREQSHIFT) + flen) / dsb->freqAdjust;
		nlen *= primarybuf->wfx.nBlockAlign;
		writepos =
			((dsb->primary_mixpos < nlen) ? primarybuf->buflen : 0) +
			dsb->primary_mixpos - nlen;
	}

	dsb->freqAcc -= flen;
	dsb->buf_mixpos = npos;
	dsb->primary_mixpos = writepos;

	TRACE("new buf_mixpos=%ld, primary_mixpos=%ld (len=%ld)\n",
	      dsb->buf_mixpos, dsb->primary_mixpos, len);

	if (cancel) DSOUND_PhaseCancel(dsb, writepos, len);
}

static void DSOUND_MixCancelAt(IDirectSoundBufferImpl *dsb, DWORD buf_writepos)
{
#if 0
	DWORD   i, size, flen, len, npos, nlen;
	INT	iAdvance = dsb->wfx.nBlockAlign;
	INT	oAdvance = primarybuf->wfx.nBlockAlign;
	/* determine amount of premixed data to cancel */
	DWORD buf_done =
		((dsb->buf_mixpos < buf_writepos) ? dsb->buflen : 0) +
		dsb->buf_mixpos - buf_writepos;
#endif

	WARN("(%p, %ld), buf_mixpos=%ld\n", dsb, buf_writepos, dsb->buf_mixpos);
	/* since this is not implemented yet, just cancel *ALL* prebuffering for now
	 * (which is faster anyway when there's one a single secondary buffer) */
	primarybuf->need_remix = TRUE;
}

static DWORD DSOUND_MixOne(IDirectSoundBufferImpl *dsb, DWORD playpos, DWORD writepos, DWORD mixlen)
{
	DWORD len, slen;
	/* determine this buffer's write position */
	DWORD buf_writepos = DSOUND_CalcPlayPosition(dsb, dsb->state & primarybuf->state, writepos,
						     writepos, dsb->primary_mixpos, dsb->buf_mixpos);
	/* determine how much already-mixed data exists */
	DWORD buf_done =
		((dsb->buf_mixpos < buf_writepos) ? dsb->buflen : 0) +
		dsb->buf_mixpos - buf_writepos;
	DWORD primary_done =
		((dsb->primary_mixpos < writepos) ? primarybuf->buflen : 0) +
		dsb->primary_mixpos - writepos;
	DWORD adv_done =
		((primarybuf->buf_mixpos < writepos) ? primarybuf->buflen : 0) +
		primarybuf->buf_mixpos - writepos;
	int still_behind;

	TRACE("buf_writepos=%ld, primary_writepos=%ld\n", buf_writepos, writepos);
	TRACE("buf_done=%ld, primary_done=%ld\n", buf_done, primary_done);
	TRACE("buf_mixpos=%ld, primary_mixpos=%ld, mixlen=%ld\n", dsb->buf_mixpos, dsb->primary_mixpos,
	      mixlen);
	TRACE("looping=%ld, startpos=%ld, leadin=%ld\n", dsb->playflags, dsb->startpos, dsb->leadin);

	/* save write position for non-GETCURRENTPOSITION2... */
	dsb->playpos = buf_writepos;

	/* check whether CalcPlayPosition detected a mixing underrun */
	if ((buf_done == 0) && (dsb->primary_mixpos != writepos)) {
		/* it did, but did we have more to play? */
		if ((dsb->playflags & DSBPLAY_LOOPING) ||
		    (dsb->buf_mixpos < dsb->buflen)) {
			/* yes, have to recover */
			ERR("underrun on sound buffer %p\n", dsb);
			TRACE("recovering from underrun: primary_mixpos=%ld\n", writepos);
		}
		dsb->primary_mixpos = writepos;
		primary_done = 0;
	}
	/* determine how far ahead we should mix */
	if (((dsb->playflags & DSBPLAY_LOOPING) ||
	     (dsb->leadin && (dsb->probably_valid_to != 0))) &&
	    !(dsb->dsbd.dwFlags & DSBCAPS_STATIC)) {
		/* if this is a streaming buffer, it typically means that
		 * we should defer mixing past probably_valid_to as long
		 * as we can, to avoid unnecessary remixing */
		/* the heavy-looking calculations shouldn't be that bad,
		 * as any game isn't likely to be have more than 1 or 2
		 * streaming buffers in use at any time anyway... */
		DWORD probably_valid_left =
			(dsb->probably_valid_to == (DWORD)-1) ? dsb->buflen :
			((dsb->probably_valid_to < buf_writepos) ? dsb->buflen : 0) +
			dsb->probably_valid_to - buf_writepos;
		/* check for leadin condition */
		if ((probably_valid_left == 0) &&
		    (dsb->probably_valid_to == dsb->startpos) &&
		    dsb->leadin)
			probably_valid_left = dsb->buflen;
		TRACE("streaming buffer probably_valid_to=%ld, probably_valid_left=%ld\n",
		      dsb->probably_valid_to, probably_valid_left);
		/* check whether the app's time is already up */
		if (probably_valid_left < dsb->writelead) {
			WARN("probably_valid_to now within writelead, possible streaming underrun\n");
			/* once we pass the point of no return,
			 * no reason to hold back anymore */
			dsb->probably_valid_to = (DWORD)-1;
			/* we just have to go ahead and mix what we have,
			 * there's no telling what the app is thinking anyway */
		} else {
			/* adjust for our frequency and our sample size */
			probably_valid_left = MulDiv(probably_valid_left,
						     1 << DSOUND_FREQSHIFT,
						     dsb->wfx.nBlockAlign * dsb->freqAdjust) *
				              primarybuf->wfx.nBlockAlign;
			/* check whether to clip mix_len */
			if (probably_valid_left < mixlen) {
				TRACE("clipping to probably_valid_left=%ld\n", probably_valid_left);
				mixlen = probably_valid_left;
			}
		}
	}
	/* cut mixlen with what's already been mixed */
	if (mixlen < primary_done) {
		/* huh? and still CalcPlayPosition didn't
		 * detect an underrun? */
		FIXME("problem with underrun detection (mixlen=%ld < primary_done=%ld)\n", mixlen, primary_done);
		return 0;
	}
	len = mixlen - primary_done;
	TRACE("remaining mixlen=%ld\n", len);

	if (len < primarybuf->dsound->fraglen) {
		/* smaller than a fragment, wait until it gets larger
		 * before we take the mixing overhead */
		TRACE("mixlen not worth it, deferring mixing\n");
		return 0;
	}

	/* ok, we know how much to mix, let's go */
	still_behind = (adv_done > primary_done);
	while (len) {
		slen = primarybuf->buflen - dsb->primary_mixpos;
		if (slen > len) slen = len;
		slen = DSOUND_MixInBuffer(dsb, dsb->primary_mixpos, slen);

		if ((dsb->primary_mixpos < primarybuf->buf_mixpos) &&
		    (dsb->primary_mixpos + slen >= primarybuf->buf_mixpos))
			still_behind = FALSE;

		dsb->primary_mixpos += slen; len -= slen;
		while (dsb->primary_mixpos >= primarybuf->buflen)
			dsb->primary_mixpos -= primarybuf->buflen;

		if ((dsb->state == STATE_STOPPED) || !slen) break;
	}
	TRACE("new primary_mixpos=%ld, primary_advbase=%ld\n", dsb->primary_mixpos, primarybuf->buf_mixpos);
	TRACE("mixed data len=%ld, still_behind=%d\n", mixlen-len, still_behind);
	/* return how far we think the primary buffer can
	 * advance its underrun detector...*/
	if (still_behind) return 0;
	if ((mixlen - len) < primary_done) return 0;
	slen = ((dsb->primary_mixpos < primarybuf->buf_mixpos) ?
		primarybuf->buflen : 0) + dsb->primary_mixpos -
		primarybuf->buf_mixpos;
	if (slen > mixlen) {
		/* the primary_done and still_behind checks above should have worked */
		FIXME("problem with advancement calculation (advlen=%ld > mixlen=%ld)\n", slen, mixlen);
		slen = 0;
	}
	return slen;
}

static DWORD DSOUND_MixToPrimary(DWORD playpos, DWORD writepos, DWORD mixlen, BOOL recover)
{
	INT			i, len, maxlen = 0;
	IDirectSoundBufferImpl	*dsb;

	TRACE("(%ld,%ld,%ld)\n", playpos, writepos, mixlen);
	for (i = dsound->nrofbuffers - 1; i >= 0; i--) {
		dsb = dsound->buffers[i];

		if (!dsb || !(ICOM_VTBL(dsb)))
			continue;
		if (dsb->buflen && dsb->state && !dsb->hwbuf) {
			TRACE("Checking %p, mixlen=%ld\n", dsb, mixlen);
			EnterCriticalSection(&(dsb->lock));
			if (dsb->state == STATE_STOPPING) {
				DSOUND_MixCancel(dsb, writepos, TRUE);
				dsb->state = STATE_STOPPED;
			} else {
				if ((dsb->state == STATE_STARTING) || recover)
					dsb->primary_mixpos = writepos;
				len = DSOUND_MixOne(dsb, playpos, writepos, mixlen);
				if (dsb->state == STATE_STARTING)
					dsb->state = STATE_PLAYING;
				maxlen = (len > maxlen) ? len : maxlen;
			}
			LeaveCriticalSection(&(dsb->lock));
		}
	}
	
	return maxlen;
}

static void DSOUND_MixReset(DWORD writepos)
{
	INT			i;
	IDirectSoundBufferImpl	*dsb;
	int nfiller;

	TRACE("(%ld)\n", writepos);

	/* the sound of silence */
	nfiller = primarybuf->wfx.wBitsPerSample == 8 ? 128 : 0;

	/* reset all buffer mix positions */
	for (i = dsound->nrofbuffers - 1; i >= 0; i--) {
		dsb = dsound->buffers[i];

		if (!dsb || !(ICOM_VTBL(dsb)))
			continue;
		if (dsb->buflen && dsb->state && !dsb->hwbuf) {
			TRACE("Resetting %p\n", dsb);
			EnterCriticalSection(&(dsb->lock));
			if (dsb->state == STATE_STOPPING) {
				dsb->state = STATE_STOPPED;
			}
			else if (dsb->state == STATE_STARTING) {
				/* nothing */
			} else {
				DSOUND_MixCancel(dsb, writepos, FALSE);
			}
			LeaveCriticalSection(&(dsb->lock));
		}
	}

	/* wipe out premixed data */
	if (primarybuf->buf_mixpos < writepos) {
		memset(primarybuf->buffer + writepos, nfiller, primarybuf->buflen - writepos);
		memset(primarybuf->buffer, nfiller, primarybuf->buf_mixpos);
	} else {
		memset(primarybuf->buffer + writepos, nfiller, primarybuf->buf_mixpos - writepos);
	}

	/* reset primary mix position */
	primarybuf->buf_mixpos = writepos;
}

static void DSOUND_CheckReset(IDirectSoundImpl *dsound, DWORD writepos)
{
	if (primarybuf->need_remix) {
		DSOUND_MixReset(writepos);
		primarybuf->need_remix = FALSE;
		/* maximize Half-Life performance */
		dsound->prebuf = DS_SND_QUEUE_MIN;
	} else {
		/* if (dsound->prebuf < DS_SND_QUEUE_MAX) dsound->prebuf++; */
	}
	TRACE("premix adjust: %d\n", dsound->prebuf);
}

static void DSOUND_WaveQueue(IDirectSoundImpl *dsound, DWORD mixq)
{
	if (mixq + dsound->pwqueue > DS_HEL_QUEUE) mixq = DS_HEL_QUEUE - dsound->pwqueue;
	TRACE("queueing %ld buffers, starting at %d\n", mixq, dsound->pwwrite);
	for (; mixq; mixq--) {
		waveOutWrite(dsound->hwo, dsound->pwave[dsound->pwwrite], sizeof(WAVEHDR));
		dsound->pwwrite++;
		if (dsound->pwwrite >= DS_HEL_FRAGS) dsound->pwwrite = 0;
		dsound->pwqueue++;
	}
}

/* #define SYNC_CALLBACK */

static void DSOUND_PerformMix(void)
{
	int nfiller;
	BOOL forced;
	HRESULT hres;

	EnterCriticalSection(&(dsound->lock));

	if (!primarybuf || !primarybuf->ref) {
		/* seems the primary buffer is currently being released */
		LeaveCriticalSection(&(dsound->lock));
		return;
	}

	/* the sound of silence */
	nfiller = primarybuf->wfx.wBitsPerSample == 8 ? 128 : 0;

	/* whether the primary is forced to play even without secondary buffers */
	forced = ((primarybuf->state == STATE_PLAYING) || (primarybuf->state == STATE_STARTING));

        TRACE("entering at %ld\n", GetTickCount());
	if (dsound->priolevel != DSSCL_WRITEPRIMARY) {
		BOOL paused = ((primarybuf->state == STATE_STOPPED) || (primarybuf->state == STATE_STARTING));
		/* FIXME: document variables */
 		DWORD playpos, writepos, inq, maxq, frag;
 		if (primarybuf->hwbuf) {
			hres = IDsDriverBuffer_GetPosition(primarybuf->hwbuf, &playpos, &writepos);
			if (hres) {
			    LeaveCriticalSection(&(dsound->lock));
			    return;
			}
			/* Well, we *could* do Just-In-Time mixing using the writepos,
			 * but that's a little bit ambitious and unnecessary... */
			/* rather add our safety margin to the writepos, if we're playing */
			if (!paused) {
				writepos += primarybuf->writelead;
				while (writepos >= primarybuf->buflen)
					writepos -= primarybuf->buflen;
			} else writepos = playpos;
		}
		else {
 			playpos = dsound->pwplay * dsound->fraglen;
 			writepos = playpos;
 			if (!paused) {
	 			writepos += DS_HEL_MARGIN * dsound->fraglen;
	 			while (writepos >= primarybuf->buflen)
 					writepos -= primarybuf->buflen;
	 		}
		}
		TRACE("primary playpos=%ld, writepos=%ld, clrpos=%ld, mixpos=%ld\n",
		      playpos,writepos,primarybuf->playpos,primarybuf->buf_mixpos);
		/* wipe out just-played sound data */
		if (playpos < primarybuf->playpos) {
			memset(primarybuf->buffer + primarybuf->playpos, nfiller, primarybuf->buflen - primarybuf->playpos);
			memset(primarybuf->buffer, nfiller, playpos);
		} else {
			memset(primarybuf->buffer + primarybuf->playpos, nfiller, playpos - primarybuf->playpos);
		}
		primarybuf->playpos = playpos;

		EnterCriticalSection(&(primarybuf->lock));

		/* reset mixing if necessary */
		DSOUND_CheckReset(dsound, writepos);

		/* check how much prebuffering is left */
		inq = primarybuf->buf_mixpos;
		if (inq < writepos)
			inq += primarybuf->buflen;
		inq -= writepos;

		/* find the maximum we can prebuffer */
		if (!paused) {
			maxq = playpos;
			if (maxq < writepos)
				maxq += primarybuf->buflen;
			maxq -= writepos;
		} else maxq = primarybuf->buflen;

		/* clip maxq to dsound->prebuf */
		frag = dsound->prebuf * dsound->fraglen;
		if (maxq > frag) maxq = frag;

		/* check for consistency */
		if (inq > maxq) {
			/* the playback position must have passed our last
			 * mixed position, i.e. it's an underrun, or we have
			 * nothing more to play */
			TRACE("reached end of mixed data (inq=%ld, maxq=%ld)\n", inq, maxq);
			inq = 0;
			/* stop the playback now, to allow buffers to refill */
			if (primarybuf->state == STATE_PLAYING) {
				primarybuf->state = STATE_STARTING;
			}
			else if (primarybuf->state == STATE_STOPPING) {
				primarybuf->state = STATE_STOPPED;
			}
			else {
				/* how can we have an underrun if we aren't playing? */
				WARN("unexpected primary state (%ld)\n", primarybuf->state);
			}
#ifdef SYNC_CALLBACK
			/* DSOUND_callback may need this lock */
			LeaveCriticalSection(&(primarybuf->lock));
#endif
			DSOUND_PrimaryStop(primarybuf);
#ifdef SYNC_CALLBACK
			EnterCriticalSection(&(primarybuf->lock));
#endif
			if (primarybuf->hwbuf) {
				/* the Stop is supposed to reset play position to beginning of buffer */
				/* unfortunately, OSS is not able to do so, so get current pointer */
				hres = IDsDriverBuffer_GetPosition(primarybuf->hwbuf, &playpos, NULL);
				if (hres) {
					LeaveCriticalSection(&(dsound->lock));
					LeaveCriticalSection(&(primarybuf->lock));
					return;
				}
			} else {
	 			playpos = dsound->pwplay * dsound->fraglen;
			}
			writepos = playpos;
			primarybuf->playpos = playpos;
			primarybuf->buf_mixpos = writepos;
			inq = 0;
			maxq = primarybuf->buflen;
			if (maxq > frag) maxq = frag;
			memset(primarybuf->buffer, nfiller, primarybuf->buflen);
			paused = TRUE;
		}

		/* do the mixing */
		frag = DSOUND_MixToPrimary(playpos, writepos, maxq, paused);
		if (forced) frag = maxq - inq;
		primarybuf->buf_mixpos += frag;
		while (primarybuf->buf_mixpos >= primarybuf->buflen)
			primarybuf->buf_mixpos -= primarybuf->buflen;

		if (frag) {
			/* buffers have been filled, restart playback */
			if (primarybuf->state == STATE_STARTING) {
				primarybuf->state = STATE_PLAYING;
			}
			else if (primarybuf->state == STATE_STOPPED) {
				/* the primarybuf is supposed to play if there's something to play
				 * even if it is reported as stopped, so don't let this confuse you */
				primarybuf->state = STATE_STOPPING;
			}
			LeaveCriticalSection(&(primarybuf->lock));
			if (paused) {
				DSOUND_PrimaryPlay(primarybuf);
				TRACE("starting playback\n");
			}
		}
		else
			LeaveCriticalSection(&(primarybuf->lock));
	} else {
		/* in the DSSCL_WRITEPRIMARY mode, the app is totally in charge... */
		if (primarybuf->state == STATE_STARTING) {
			DSOUND_PrimaryPlay(primarybuf);
			primarybuf->state = STATE_PLAYING;
		} 
		else if (primarybuf->state == STATE_STOPPING) {
			DSOUND_PrimaryStop(primarybuf);
			primarybuf->state = STATE_STOPPED;
		}
	}
	TRACE("completed processing at %ld\n", GetTickCount());
	LeaveCriticalSection(&(dsound->lock));
}

static void CALLBACK DSOUND_timer(UINT timerID, UINT msg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	if (!dsound || !primarybuf) {
		ERR("dsound died without killing us?\n");
		timeKillEvent(timerID);
		timeEndPeriod(DS_TIME_RES);
		return;
	}

	TRACE("entered\n");
	DSOUND_PerformMix();
}

static void CALLBACK DSOUND_callback(HWAVEOUT hwo, UINT msg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
        IDirectSoundImpl* This = (IDirectSoundImpl*)dwUser;
	TRACE("entering at %ld, msg=%08x\n", GetTickCount(), msg);
	if (msg == MM_WOM_DONE) {
		DWORD inq, mixq, fraglen, buflen, pwplay, playpos, mixpos;
		if (This->pwqueue == (DWORD)-1) {
			TRACE("completed due to reset\n");
			return;
		}
/* it could be a bad idea to enter critical section here... if there's lock contention,
 * the resulting scheduling delays might obstruct the winmm player thread */
#ifdef SYNC_CALLBACK
		EnterCriticalSection(&(primarybuf->lock));
#endif
		/* retrieve current values */
		fraglen = dsound->fraglen;
		buflen = primarybuf->buflen;
		pwplay = dsound->pwplay;
		playpos = pwplay * fraglen;
		mixpos = primarybuf->buf_mixpos;
		/* check remaining mixed data */
		inq = ((mixpos < playpos) ? buflen : 0) + mixpos - playpos;
		mixq = inq / fraglen;
		if ((inq - (mixq * fraglen)) > 0) mixq++;
		/* complete the playing buffer */
		TRACE("done playing primary pos=%ld\n", playpos);
		pwplay++;
		if (pwplay >= DS_HEL_FRAGS) pwplay = 0;
		/* write new values */
		dsound->pwplay = pwplay;
		dsound->pwqueue--;
		/* queue new buffer if we have data for it */
		if (inq>1) DSOUND_WaveQueue(This, inq-1);
#ifdef SYNC_CALLBACK
		LeaveCriticalSection(&(primarybuf->lock));
#endif
	}
	TRACE("completed\n");
}

/*******************************************************************************
 *		DirectSoundCreate (DSOUND.1)
 */
HRESULT WINAPI DirectSoundCreate(REFGUID lpGUID,LPDIRECTSOUND *ppDS,IUnknown *pUnkOuter )
{
	IDirectSoundImpl** ippDS=(IDirectSoundImpl**)ppDS;
	PIDSDRIVER drv = NULL;
	WAVEOUTCAPSA wcaps;
	unsigned wod, wodn;
	HRESULT err = DS_OK;

	if (lpGUID)
		TRACE("(%p,%p,%p)\n",lpGUID,ippDS,pUnkOuter);
	else
		TRACE("DirectSoundCreate (%p)\n", ippDS);

	if (ippDS == NULL)
		return DSERR_INVALIDPARAM;

	if (primarybuf) {
		IDirectSound_AddRef((LPDIRECTSOUND)dsound);
		*ippDS = dsound;
		return DS_OK;
	}

	/* Enumerate WINMM audio devices and find the one we want */
	wodn = waveOutGetNumDevs();
	if (!wodn) return DSERR_NODRIVER;

	/* FIXME: How do we find the GUID of an audio device? */
	wod = 0; /* start at the first audio device */

	/* Get output device caps */
	waveOutGetDevCapsA(wod, &wcaps, sizeof(wcaps));
	/* DRV_QUERYDSOUNDIFACE is a "Wine extension" to get the DSound interface */
	waveOutMessage(wod, DRV_QUERYDSOUNDIFACE, (DWORD)&drv, 0);

	/* Allocate memory */
	*ippDS = (IDirectSoundImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(IDirectSoundImpl));
	if (*ippDS == NULL)
		return DSERR_OUTOFMEMORY;

	ICOM_VTBL(*ippDS)	= &dsvt;
	(*ippDS)->ref		= 1;

	(*ippDS)->driver	= drv;
	(*ippDS)->fraglen	= 0;
	(*ippDS)->priolevel	= DSSCL_NORMAL; 
	(*ippDS)->nrofbuffers	= 0;
	(*ippDS)->buffers	= NULL;
	(*ippDS)->primary	= NULL; 
	(*ippDS)->listener	= NULL; 

	(*ippDS)->prebuf	= DS_SND_QUEUE_MAX;

	/* Get driver description */
	if (drv) {
		IDsDriver_GetDriverDesc(drv,&((*ippDS)->drvdesc));
	} else {
		/* if no DirectSound interface available, use WINMM API instead */
		(*ippDS)->drvdesc.dwFlags = DSDDESC_DOMMSYSTEMOPEN | DSDDESC_DOMMSYSTEMSETFORMAT;
		(*ippDS)->drvdesc.dnDevNode = wod; /* FIXME? */
	}

	/* Set default wave format (may need it for waveOutOpen) */
	(*ippDS)->wfx.wFormatTag	= WAVE_FORMAT_PCM;
	(*ippDS)->wfx.nChannels		= 2;
	(*ippDS)->wfx.nSamplesPerSec	= 22050;
	(*ippDS)->wfx.nAvgBytesPerSec	= 44100;
	(*ippDS)->wfx.nBlockAlign	= 2;
	(*ippDS)->wfx.wBitsPerSample	= 8;

	/* If the driver requests being opened through MMSYSTEM
	 * (which is recommended by the DDK), it is supposed to happen
	 * before the DirectSound interface is opened */
	if ((*ippDS)->drvdesc.dwFlags & DSDDESC_DOMMSYSTEMOPEN)
	{
		/* FIXME: is this right? */

	        (*ippDS)->drvdesc.dnDevNode = 0;
		err = DSERR_ALLOCATED;

		/* if this device is busy try the next one */
		while((err == DSERR_ALLOCATED) && 
			((*ippDS)->drvdesc.dnDevNode < wodn))
  		{
		  err = mmErr(waveOutOpen(&((*ippDS)->hwo), 
			(*ippDS)->drvdesc.dnDevNode, &((*ippDS)->wfx),
			(DWORD)DSOUND_callback, (DWORD)(*ippDS),
			CALLBACK_FUNCTION | WAVE_DIRECTSOUND));
                  (*ippDS)->drvdesc.dnDevNode++; /* next wave device */
		}

                (*ippDS)->drvdesc.dnDevNode--; /* take away last increment */
		
	}

	if (drv && (err == DS_OK))
		err = IDsDriver_Open(drv);

	/* FIXME: do we want to handle a temporarily busy device? */
	if (err != DS_OK) {
		HeapFree(GetProcessHeap(),0,*ippDS);
		*ippDS = NULL;
		return err;
	}

	/* the driver is now open, so it's now allowed to call GetCaps */
	if (drv) {
		IDsDriver_GetCaps(drv,&((*ippDS)->drvcaps));
	} else {
		unsigned c;

		/* FIXME: look at wcaps */
		(*ippDS)->drvcaps.dwFlags =
			DSCAPS_PRIMARY16BIT | DSCAPS_PRIMARYSTEREO;
		if (DS_EMULDRIVER)
		    (*ippDS)->drvcaps.dwFlags |= DSCAPS_EMULDRIVER;

		/* Allocate memory for HEL buffer headers */
		for (c=0; c<DS_HEL_FRAGS; c++) {
			(*ippDS)->pwave[c] = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(WAVEHDR));
			if (!(*ippDS)->pwave[c]) {
				/* Argh, out of memory */
				while (c--) {
					HeapFree(GetProcessHeap(),0,(*ippDS)->pwave[c]);
					waveOutClose((*ippDS)->hwo);
					HeapFree(GetProcessHeap(),0,*ippDS);
					*ippDS = NULL;
					return DSERR_OUTOFMEMORY;
				}
			}
		}
	}

	InitializeCriticalSection(&((*ippDS)->lock));

	if (!dsound) {
		dsound = (*ippDS);
		if (primarybuf == NULL) {
			DSBUFFERDESC	dsbd;
			HRESULT		hr;

			dsbd.dwSize = sizeof(DSBUFFERDESC);
			dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
			dsbd.dwBufferBytes = 0;
			dsbd.lpwfxFormat = &(dsound->wfx);
			hr = IDirectSound_CreateSoundBuffer(*ppDS, &dsbd, (LPDIRECTSOUNDBUFFER*)&primarybuf, NULL);
			if (hr != DS_OK)
				return hr;

			/* dsound->primary is NULL - don't need to Release */
			dsound->primary = primarybuf;
			IDirectSoundBuffer_AddRef((LPDIRECTSOUNDBUFFER)primarybuf);
		}
		timeBeginPeriod(DS_TIME_RES);
		dsound->timerID = timeSetEvent(DS_TIME_DEL, DS_TIME_RES, DSOUND_timer,
					       (DWORD)dsound, TIME_PERIODIC | TIME_CALLBACK_FUNCTION);
	}
	return DS_OK;
}

/***************************************************************************
 * DirectSoundCaptureCreate [DSOUND.6]
 *
 * Create and initialize a DirectSoundCapture interface
 *
 * RETURNS
 *    Success: DS_OK
 *    Failure: DSERR_NOAGGREGATION, DSERR_ALLOCATED, DSERR_INVALIDPARAM,
 *             DSERR_OUTOFMEMORY
 */
HRESULT WINAPI DirectSoundCaptureCreate(
	LPCGUID lpcGUID,
	LPDIRECTSOUNDCAPTURE* lplpDSC,
	LPUNKNOWN pUnkOuter )
{
	TRACE("(%s,%p,%p)\n", debugstr_guid(lpcGUID), lplpDSC, pUnkOuter);

	if( pUnkOuter ) {
		return DSERR_NOAGGREGATION;
	}

	/* Default device? */
	if ( !lpcGUID ) {
		return DSOUND_CreateDirectSoundCapture( (LPVOID*)lplpDSC );
	}

	FIXME( "Unknown GUID %s\n", debugstr_guid(lpcGUID) );
	*lplpDSC = NULL;

	return DSERR_OUTOFMEMORY;
}

/***************************************************************************
 * DirectSoundCaptureEnumerateA [DSOUND.7]
 *
 * Enumerate all DirectSound drivers installed in the system
 *
 * RETURNS
 *    Success: DS_OK
 *    Failure: DSERR_INVALIDPARAM
 */
HRESULT WINAPI DirectSoundCaptureEnumerateA(
        LPDSENUMCALLBACKA lpDSEnumCallback,
        LPVOID lpContext)
{
	TRACE("(%p,%p)\n", lpDSEnumCallback, lpContext );

	if ( lpDSEnumCallback )
		lpDSEnumCallback(NULL,"WINE Primary Sound Capture Driver",
                    "SoundCap",lpContext);


	return DS_OK;
}

/***************************************************************************
 * DirectSoundCaptureEnumerateW [DSOUND.8]
 *
 * Enumerate all DirectSound drivers installed in the system
 *
 * RETURNS
 *    Success: DS_OK
 *    Failure: DSERR_INVALIDPARAM
 */
HRESULT WINAPI DirectSoundCaptureEnumerateW(
        LPDSENUMCALLBACKW lpDSEnumCallback,
        LPVOID lpContext)
{
        FIXME("(%p,%p):stub\n", lpDSEnumCallback, lpContext );
        return DS_OK;
} 

static HRESULT
DSOUND_CreateDirectSoundCapture( LPVOID* ppobj )
{
	*ppobj = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( IDirectSoundCaptureImpl ) );

	if ( *ppobj == NULL ) {
		return DSERR_OUTOFMEMORY;
	}

	{
		ICOM_THIS(IDirectSoundCaptureImpl,*ppobj);

		This->ref = 1;
		ICOM_VTBL(This) = &dscvt;

		InitializeCriticalSection( &This->lock );
	}

	return S_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureImpl_QueryInterface(
	LPDIRECTSOUNDCAPTURE iface,
	REFIID riid,
	LPVOID* ppobj )
{
	ICOM_THIS(IDirectSoundCaptureImpl,iface);

	FIXME( "(%p)->(%s,%p): stub\n", This, debugstr_guid(riid), ppobj );

	return E_FAIL;
}

static ULONG WINAPI
IDirectSoundCaptureImpl_AddRef( LPDIRECTSOUNDCAPTURE iface )
{
	ULONG uRef;
        ICOM_THIS(IDirectSoundCaptureImpl,iface);

	EnterCriticalSection( &This->lock );

	TRACE( "(%p) was 0x%08lx\n", This, This->ref );
	uRef = ++(This->ref);

	LeaveCriticalSection( &This->lock );

        return uRef;
}

static ULONG WINAPI
IDirectSoundCaptureImpl_Release( LPDIRECTSOUNDCAPTURE iface )
{
	ULONG uRef;
	ICOM_THIS(IDirectSoundCaptureImpl,iface);

	EnterCriticalSection( &This->lock );

	TRACE( "(%p) was 0x%08lx\n", This, This->ref );
	uRef = --(This->ref);

	LeaveCriticalSection( &This->lock );

	if ( uRef == 0 ) {
		DeleteCriticalSection( &This->lock );
		HeapFree( GetProcessHeap(), 0, This );
	}

	return uRef;
}

static HRESULT WINAPI
IDirectSoundCaptureImpl_CreateCaptureBuffer(
	LPDIRECTSOUNDCAPTURE iface,
	LPCDSCBUFFERDESC lpcDSCBufferDesc,
	LPDIRECTSOUNDCAPTUREBUFFER* lplpDSCaptureBuffer, 
	LPUNKNOWN pUnk )
{
	HRESULT hr;
	ICOM_THIS(IDirectSoundCaptureImpl,iface);

	TRACE( "(%p)->(%p,%p,%p)\n", This, lpcDSCBufferDesc, lplpDSCaptureBuffer, pUnk );

	if ( pUnk ) {
		return DSERR_INVALIDPARAM;
	}

	hr = DSOUND_CreateDirectSoundCaptureBuffer( lpcDSCBufferDesc, (LPVOID*)lplpDSCaptureBuffer );

	return hr;
}

static HRESULT WINAPI
IDirectSoundCaptureImpl_GetCaps(
	LPDIRECTSOUNDCAPTURE iface,
	LPDSCCAPS lpDSCCaps )
{
        ICOM_THIS(IDirectSoundCaptureImpl,iface);

        FIXME( "(%p)->(%p): stub\n", This, lpDSCCaps );

        return DS_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureImpl_Initialize(
	LPDIRECTSOUNDCAPTURE iface,
	LPCGUID lpcGUID )
{
        ICOM_THIS(IDirectSoundCaptureImpl,iface);

        FIXME( "(%p)->(%p): stub\n", This, lpcGUID );

        return DS_OK;
}


static ICOM_VTABLE(IDirectSoundCapture) dscvt =
{
        ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
        /* IUnknown methods */
        IDirectSoundCaptureImpl_QueryInterface,
        IDirectSoundCaptureImpl_AddRef,
        IDirectSoundCaptureImpl_Release,

        /* IDirectSoundCapture methods */
        IDirectSoundCaptureImpl_CreateCaptureBuffer,
        IDirectSoundCaptureImpl_GetCaps,
        IDirectSoundCaptureImpl_Initialize 
};

static HRESULT
DSOUND_CreateDirectSoundCaptureBuffer( LPCDSCBUFFERDESC lpcDSCBufferDesc, LPVOID* ppobj )
{

	FIXME( "(%p,%p): ignoring lpcDSCBufferDesc\n", lpcDSCBufferDesc, ppobj );

	*ppobj = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( IDirectSoundCaptureBufferImpl ) );

	if ( *ppobj == NULL ) {
		return DSERR_OUTOFMEMORY;
	}

	{
		ICOM_THIS(IDirectSoundCaptureBufferImpl,*ppobj);

		This->ref = 1;
		ICOM_VTBL(This) = &dscbvt;

		InitializeCriticalSection( &This->lock );
	}

	return S_OK;
}


static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_QueryInterface(
        LPDIRECTSOUNDCAPTUREBUFFER iface,
        REFIID riid,
        LPVOID* ppobj )
{
        ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);

        FIXME( "(%p)->(%s,%p): stub\n", This, debugstr_guid(riid), ppobj );

        return E_FAIL;
}

static ULONG WINAPI
IDirectSoundCaptureBufferImpl_AddRef( LPDIRECTSOUNDCAPTUREBUFFER iface )
{
        ULONG uRef;
        ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);

        EnterCriticalSection( &This->lock );

	TRACE( "(%p) was 0x%08lx\n", This, This->ref );
        uRef = ++(This->ref);

        LeaveCriticalSection( &This->lock );

        return uRef;
}

static ULONG WINAPI
IDirectSoundCaptureBufferImpl_Release( LPDIRECTSOUNDCAPTUREBUFFER iface )
{
        ULONG uRef;
        ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);

        EnterCriticalSection( &This->lock );

	TRACE( "(%p) was 0x%08lx\n", This, This->ref );
        uRef = --(This->ref);

        LeaveCriticalSection( &This->lock );

        if ( uRef == 0 ) {
		DeleteCriticalSection( &This->lock );
                HeapFree( GetProcessHeap(), 0, This );
        }

        return uRef;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_GetCaps(
        LPDIRECTSOUNDCAPTUREBUFFER iface,
	LPDSCBCAPS lpDSCBCaps )
{
	ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);

	FIXME( "(%p)->(%p): stub\n", This, lpDSCBCaps );

	return DS_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_GetCurrentPosition(
        LPDIRECTSOUNDCAPTUREBUFFER iface,
	LPDWORD lpdwCapturePosition,
	LPDWORD lpdwReadPosition )
{
	ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);

	FIXME( "(%p)->(%p,%p): stub\n", This, lpdwCapturePosition, lpdwReadPosition );

	return DS_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_GetFormat(
        LPDIRECTSOUNDCAPTUREBUFFER iface,
	LPWAVEFORMATEX lpwfxFormat, 
	DWORD dwSizeAllocated, 
	LPDWORD lpdwSizeWritten )
{
	ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);

	FIXME( "(%p)->(%p,0x%08lx,%p): stub\n", This, lpwfxFormat, dwSizeAllocated, lpdwSizeWritten );

	return DS_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_GetStatus(
        LPDIRECTSOUNDCAPTUREBUFFER iface,
	LPDWORD lpdwStatus )
{
	ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);

	FIXME( "(%p)->(%p): stub\n", This, lpdwStatus );

	return DS_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_Initialize(
        LPDIRECTSOUNDCAPTUREBUFFER iface,
	LPDIRECTSOUNDCAPTURE lpDSC, 
	LPCDSCBUFFERDESC lpcDSCBDesc )
{
	ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);

	FIXME( "(%p)->(%p,%p): stub\n", This, lpDSC, lpcDSCBDesc );

	return DS_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_Lock(
        LPDIRECTSOUNDCAPTUREBUFFER iface,
	DWORD dwReadCusor, 
	DWORD dwReadBytes, 
	LPVOID* lplpvAudioPtr1, 
	LPDWORD lpdwAudioBytes1, 
	LPVOID* lplpvAudioPtr2, 
	LPDWORD lpdwAudioBytes2, 
	DWORD dwFlags )
{
	ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);

	FIXME( "(%p)->(%08lu,%08lu,%p,%p,%p,%p,0x%08lx): stub\n", This, dwReadCusor, dwReadBytes, lplpvAudioPtr1, lpdwAudioBytes1, lplpvAudioPtr2, lpdwAudioBytes2, dwFlags );

	return DS_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_Start(
        LPDIRECTSOUNDCAPTUREBUFFER iface,
	DWORD dwFlags )
{
	ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);

	FIXME( "(%p)->(0x%08lx): stub\n", This, dwFlags );

	return DS_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_Stop( LPDIRECTSOUNDCAPTUREBUFFER iface )
{
	ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);

	FIXME( "(%p): stub\n", This );

	return DS_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_Unlock(
        LPDIRECTSOUNDCAPTUREBUFFER iface,
	LPVOID lpvAudioPtr1, 
	DWORD dwAudioBytes1, 
	LPVOID lpvAudioPtr2, 
	DWORD dwAudioBytes2 )
{
	ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);

	FIXME( "(%p)->(%p,%08lu,%p,%08lu): stub\n", This, lpvAudioPtr1, dwAudioBytes1, lpvAudioPtr2, dwAudioBytes2 );

	return DS_OK;
}


static ICOM_VTABLE(IDirectSoundCaptureBuffer) dscbvt =
{
        ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
        /* IUnknown methods */
        IDirectSoundCaptureBufferImpl_QueryInterface,
        IDirectSoundCaptureBufferImpl_AddRef,
        IDirectSoundCaptureBufferImpl_Release,

        /* IDirectSoundCaptureBuffer methods */
        IDirectSoundCaptureBufferImpl_GetCaps,
        IDirectSoundCaptureBufferImpl_GetCurrentPosition,
        IDirectSoundCaptureBufferImpl_GetFormat,
        IDirectSoundCaptureBufferImpl_GetStatus,
        IDirectSoundCaptureBufferImpl_Initialize,
        IDirectSoundCaptureBufferImpl_Lock,
        IDirectSoundCaptureBufferImpl_Start,
        IDirectSoundCaptureBufferImpl_Stop,
        IDirectSoundCaptureBufferImpl_Unlock
};

/*******************************************************************************
 * DirectSound ClassFactory
 */
typedef struct
{
    /* IUnknown fields */
    ICOM_VFIELD(IClassFactory);
    DWORD                       ref;
} IClassFactoryImpl;

static HRESULT WINAPI 
DSCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	ICOM_THIS(IClassFactoryImpl,iface);

	FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI
DSCF_AddRef(LPCLASSFACTORY iface) {
	ICOM_THIS(IClassFactoryImpl,iface);
	return ++(This->ref);
}

static ULONG WINAPI DSCF_Release(LPCLASSFACTORY iface) {
	ICOM_THIS(IClassFactoryImpl,iface);
	/* static class, won't be  freed */
	return --(This->ref);
}

static HRESULT WINAPI DSCF_CreateInstance(
	LPCLASSFACTORY iface,LPUNKNOWN pOuter,REFIID riid,LPVOID *ppobj
) {
	ICOM_THIS(IClassFactoryImpl,iface);

	TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);
	if ( IsEqualGUID( &IID_IDirectSound, riid ) ) {
		/* FIXME: reuse already created dsound if present? */
		return DirectSoundCreate(riid,(LPDIRECTSOUND*)ppobj,pOuter);
	}
	return E_NOINTERFACE;
}

static HRESULT WINAPI DSCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	ICOM_THIS(IClassFactoryImpl,iface);
	FIXME("(%p)->(%d),stub!\n",This,dolock);
	return S_OK;
}

static ICOM_VTABLE(IClassFactory) DSCF_Vtbl = {
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	DSCF_QueryInterface,
	DSCF_AddRef,
	DSCF_Release,
	DSCF_CreateInstance,
	DSCF_LockServer
};
static IClassFactoryImpl DSOUND_CF = {&DSCF_Vtbl, 1 };

/*******************************************************************************
 * DllGetClassObject [DSOUND.5]
 * Retrieves class object from a DLL object
 *
 * NOTES
 *    Docs say returns STDAPI
 *
 * PARAMS
 *    rclsid [I] CLSID for the class object
 *    riid   [I] Reference to identifier of interface for class object
 *    ppv    [O] Address of variable to receive interface pointer for riid
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: CLASS_E_CLASSNOTAVAILABLE, E_OUTOFMEMORY, E_INVALIDARG,
 *             E_UNEXPECTED
 */
DWORD WINAPI DSOUND_DllGetClassObject(REFCLSID rclsid,REFIID riid,LPVOID *ppv)
{
    TRACE("(%p,%p,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    if ( IsEqualCLSID( &IID_IClassFactory, riid ) ) {
    	*ppv = (LPVOID)&DSOUND_CF;
	IClassFactory_AddRef((IClassFactory*)*ppv);
    return S_OK;
    }

    FIXME("(%p,%p,%p): no interface found.\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}


/*******************************************************************************
 * DllCanUnloadNow [DSOUND.4]  Determines whether the DLL is in use.
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: S_FALSE
 */
DWORD WINAPI DSOUND_DllCanUnloadNow(void)
{
    FIXME("(void): stub\n");
    return S_FALSE;
}
