/*  			DirectSound
 * 
 * Copyright 1998 Marcus Meissner
 * Copyright 1998 Rob Riggs
 */
/*
 * Note: This file requires multithread ability. It is not possible to
 * implement the stuff in a single thread anyway. And most DirectX apps
 * require threading themselves.
 *
 * Most thread locking is complete. There may be a few race
 * conditions still lurking.
 *
 * Tested with a Soundblaster clone, a Gravis UltraSound Classic,
 * and a Turtle Beach Tropez+.
 *
 * TODO:
 *	Implement DirectSoundCapture API
 *	Implement SetCooperativeLevel properly (need to address focus issues)
 *	Use wavetable synth for static buffers if available
 *	Implement DirectSound3DBuffers (stubs in place)
 *	Use hardware 3D support if available (OSS support may be needed first)
 *	Add support for APIs other than OSS: ALSA (http://alsa.jcu.cz/)
 *	and esound (http://www.gnome.org), for instance
 *
 * FIXME: Status needs updating.
 *
 * Status:
 * - Wing Commander 4/W95:
 *   The intromovie plays without problems. Nearly lipsynchron.
 * - DiscWorld 2
 *   The sound works, but noticeable chunks are left out (from the sound and
 *   the animation). Don't know why yet.
 * - Diablo:
 *   Sound works, but slows down the movieplayer.
 * - XvT: 
 *   Doesn't sound yet.
 * - Monkey Island 3:
 *   The background sound of the startscreen works ;)
 * - WingCommander Prophecy Demo:
 *   Sound works for the intromovie.
 * - Total Annihilation (1998/12/04):
 *   Sound plays perfectly in the game, but the Smacker movies
 *   (http://www.smacker.com/) play silently.
 * - A-10 Cuba! Demo (1998/12/04):
 *   Sound works properly (for some people).
 * - dsstream.exe, from DirectX 5.2 SDK (1998/12/04):
 *   Works properly, but requires "-dll -winmm".
 * - dsshow.exe, from DirectX 5.2 SDK (1998/12/04):
 *   Initializes the DLL properly with CoCreateInstance(), but the
 *   FileOpen dialog box is broken - could not test properly
 */

#include "config.h"
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>	/* Insomnia - pow() function */
#include "dsound.h"
#include "winuser.h"
#include "winerror.h"
#include "wine/obj_base.h"
#include "thread.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(dsound)


/*****************************************************************************
 * Predeclare the interface implementation structures
 */
typedef struct IDirectSoundImpl IDirectSoundImpl;
typedef struct IDirectSoundBufferImpl IDirectSoundBufferImpl;
typedef struct IDirectSoundNotifyImpl IDirectSoundNotifyImpl;
typedef struct IDirectSound3DListenerImpl IDirectSound3DListenerImpl;
typedef struct IDirectSound3DBufferImpl IDirectSound3DBufferImpl;

/*****************************************************************************
 * IDirectSound implementation structure
 */
struct IDirectSoundImpl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirectSound);
    DWORD                      ref;
    /* IDirectSoundImpl fields */
    DWORD                       priolevel;
    int                         nrofbuffers;
    IDirectSoundBufferImpl**    buffers;
    IDirectSoundBufferImpl*     primary;
    IDirectSound3DListenerImpl* listener;
    WAVEFORMATEX                wfx; /* current main waveformat */
};

/*****************************************************************************
 * IDirectSoundBuffer implementation structure
 */
struct IDirectSoundBufferImpl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirectSoundBuffer);
    DWORD                            ref;
    /* IDirectSoundBufferImpl fields */
    WAVEFORMATEX              wfx;
    LPBYTE                    buffer;
    IDirectSound3DBufferImpl* ds3db;
    DWORD                     playflags,playing;
    DWORD                     playpos,writepos,buflen;
    DWORD                     nAvgBytesPerSec;
    DWORD                     freq;
    ULONG                     freqAdjust;
    LONG                      volume,pan;
    LONG                      lVolAdjust,rVolAdjust;
    IDirectSoundBufferImpl*   parent;         /* for duplicates */
    IDirectSoundImpl*         dsound;
    DSBUFFERDESC              dsbd;
    LPDSBPOSITIONNOTIFY       notifies;
    int                       nrofnotifies;
    CRITICAL_SECTION          lock;
};

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
};


#ifdef HAVE_OSS
# include <sys/ioctl.h>
# ifdef HAVE_MACHINE_SOUNDCARD_H
#  include <machine/soundcard.h>
# endif
# ifdef HAVE_SYS_SOUNDCARD_H
#  include <sys/soundcard.h>
# endif

/* #define USE_DSOUND3D 1 */

#define DSOUND_FRAGLEN (primarybuf->wfx.nAvgBytesPerSec >> 4)
#define DSOUND_FREQSHIFT (14)

static int audiofd = -1;
static int audioOK = 0;

static IDirectSoundImpl*	dsound = NULL;

static IDirectSoundBufferImpl*	primarybuf = NULL;

static int DSOUND_setformat(LPWAVEFORMATEX wfex);
static void DSOUND_CheckEvent(IDirectSoundBufferImpl *dsb, int len);
static void DSOUND_CloseAudio(void);

#endif


HRESULT WINAPI DirectSoundEnumerateA(
	LPDSENUMCALLBACKA enumcb,
	LPVOID context)
{
	TRACE("enumcb = %p, context = %p\n", enumcb, context);

#ifdef HAVE_OSS
	if (enumcb != NULL)
		enumcb(NULL,"WINE DirectSound using Open Sound System",
		    "sound",context);
#endif

	return DS_OK;
}

#ifdef HAVE_OSS
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
		FE(DSBCAPS_CTRLFREQUENCY)
		FE(DSBCAPS_CTRLPAN)
		FE(DSBCAPS_CTRLVOLUME)
		FE(DSBCAPS_CTRLDEFAULT)
		FE(DSBCAPS_CTRLALL)
		FE(DSBCAPS_STICKYFOCUS)
		FE(DSBCAPS_GETCURRENTPOSITION2)
	};
	int	i;

	for (i=0;i<sizeof(flags)/sizeof(flags[0]);i++)
		if (flags[i].mask & xmask)
			fprintf(stderr,"%s ",flags[i].name);
}

/*******************************************************************************
 *              IDirectSound3DBuffer
 */

/* IUnknown methods */
static HRESULT WINAPI IDirectSound3DBufferImpl_QueryInterface(
	LPDIRECTSOUND3DBUFFER iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectSound3DBufferImpl,iface);
	char xbuf[50];

	WINE_StringFromCLSID(riid,xbuf);
	TRACE("(%p,%s,%p)\n",This,xbuf,ppobj);
	return E_FAIL;
}
	
static ULONG WINAPI IDirectSound3DBufferImpl_AddRef(LPDIRECTSOUND3DBUFFER iface)
{
	ICOM_THIS(IDirectSound3DBufferImpl,iface);
	This->ref++;
	return This->ref;
}

static ULONG WINAPI IDirectSound3DBufferImpl_Release(LPDIRECTSOUND3DBUFFER iface)
{
	ICOM_THIS(IDirectSound3DBufferImpl,iface);
	if(--This->ref)
		return This->ref;

	HeapFree(GetProcessHeap(),0,This->buffer);
	HeapFree(GetProcessHeap(),0,This);

	return S_OK;
}

/* IDirectSound3DBuffer methods */
static HRESULT WINAPI IDirectSound3DBufferImpl_GetAllParameters(
	LPDIRECTSOUND3DBUFFER iface,
	LPDS3DBUFFER lpDs3dBuffer)
{
	FIXME("stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_GetConeAngles(
	LPDIRECTSOUND3DBUFFER iface,
	LPDWORD lpdwInsideConeAngle,
	LPDWORD lpdwOutsideConeAngle)
{
	FIXME("stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_GetConeOrientation(
	LPDIRECTSOUND3DBUFFER iface,
	LPD3DVECTOR lpvConeOrientation)
{
	FIXME("stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_GetConeOutsideVolume(
	LPDIRECTSOUND3DBUFFER iface,
	LPLONG lplConeOutsideVolume)
{
	FIXME("stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_GetMaxDistance(
	LPDIRECTSOUND3DBUFFER iface,
	LPD3DVALUE lpfMaxDistance)
{
	FIXME("stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_GetMinDistance(
	LPDIRECTSOUND3DBUFFER iface,
	LPD3DVALUE lpfMinDistance)
{
	FIXME("stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_GetMode(
	LPDIRECTSOUND3DBUFFER iface,
	LPDWORD lpdwMode)
{
	FIXME("stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_GetPosition(
	LPDIRECTSOUND3DBUFFER iface,
	LPD3DVECTOR lpvPosition)
{
	FIXME("stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_GetVelocity(
	LPDIRECTSOUND3DBUFFER iface,
	LPD3DVECTOR lpvVelocity)
{
	FIXME("stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetAllParameters(
	LPDIRECTSOUND3DBUFFER iface,
	LPCDS3DBUFFER lpcDs3dBuffer,
	DWORD dwApply)
{
	FIXME("stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetConeAngles(
	LPDIRECTSOUND3DBUFFER iface,
	DWORD dwInsideConeAngle,
	DWORD dwOutsideConeAngle,
	DWORD dwApply)
{
	FIXME("stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetConeOrientation(
	LPDIRECTSOUND3DBUFFER iface,
	D3DVALUE x, D3DVALUE y, D3DVALUE z,
	DWORD dwApply)
{
	FIXME("stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetConeOutsideVolume(
	LPDIRECTSOUND3DBUFFER iface,
	LONG lConeOutsideVolume,
	DWORD dwApply)
{
	FIXME("stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetMaxDistance(
	LPDIRECTSOUND3DBUFFER iface,
	D3DVALUE fMaxDistance,
	DWORD dwApply)
{
	FIXME("stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetMinDistance(
	LPDIRECTSOUND3DBUFFER iface,
	D3DVALUE fMinDistance,
	DWORD dwApply)
{
	FIXME("stub\n");
	return DS_OK;
}

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

static HRESULT WINAPI IDirectSound3DBufferImpl_SetPosition(
	LPDIRECTSOUND3DBUFFER iface,
	D3DVALUE x, D3DVALUE y, D3DVALUE z,
	DWORD dwApply)
{
	FIXME("stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetVelocity(
	LPDIRECTSOUND3DBUFFER iface,
	D3DVALUE x, D3DVALUE y, D3DVALUE z,
	DWORD dwApply)
{
	FIXME("stub\n");
	return DS_OK;
}

ICOM_VTABLE(IDirectSound3DBuffer) ds3dbvt = 
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
				wTbuf[i] = (dsb->buffer[i] + dsb->buffer[(i * 2) + 1]) / 2;
			wIbuf = wTbuf;
		} else {
			iSize = dsb->buflen / 2;
			bTbuf = malloc(dsb->buflen / 2);
			if (bTbuf == NULL)
				return DSERR_OUTOFMEMORY;
			for (i = 0; i < iSize; i++)
				bTbuf[i] = (dsb->buffer[i] + dsb->buffer[(i * 2) + 1]) / 2;
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
	char xbuf[50];

	WINE_StringFromCLSID(riid,xbuf);
	TRACE("(%p,%s,%p)\n",This,xbuf,ppobj);
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
	ICOM_THIS(IDirectSound3DListenerImpl,iface);
	This->ref--;
	return This->ref;
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

ICOM_VTABLE(IDirectSound3DListener) ds3dlvt = 
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
	char xbuf[50];

	WINE_StringFromCLSID(riid,xbuf);
	TRACE("(%p,%s,%p)\n",This,xbuf,ppobj);
	return E_FAIL;
}

static ULONG WINAPI IDirectSoundNotifyImpl_AddRef(LPDIRECTSOUNDNOTIFY iface) {
	ICOM_THIS(IDirectSoundNotifyImpl,iface);
	return ++(This->ref);
}

static ULONG WINAPI IDirectSoundNotifyImpl_Release(LPDIRECTSOUNDNOTIFY iface) {
	ICOM_THIS(IDirectSoundNotifyImpl,iface);
	This->ref--;
	if (!This->ref) {
		IDirectSoundNotify_Release((LPDIRECTSOUNDBUFFER)This->dsb);
		HeapFree(GetProcessHeap(),0,This);
		return S_OK;
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

ICOM_VTABLE(IDirectSoundNotify) dsnvt = 
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

/* This sets this format for the <em>Primary Buffer Only</em> */
/* See file:///cdrom/sdk52/docs/worddoc/dsound.doc page 120 */
static HRESULT WINAPI IDirectSoundBufferImpl_SetFormat(
	LPDIRECTSOUNDBUFFER iface,LPWAVEFORMATEX wfex
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	IDirectSoundBufferImpl** dsb;
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
	EnterCriticalSection(&(primarybuf->lock));

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

	TRACE("(formattag=0x%04x,chans=%d,samplerate=%ld"
		   "bytespersec=%ld,blockalign=%d,bitspersamp=%d,cbSize=%d)\n",
		   wfex->wFormatTag, wfex->nChannels, wfex->nSamplesPerSec,
		   wfex->nAvgBytesPerSec, wfex->nBlockAlign, 
		   wfex->wBitsPerSample, wfex->cbSize);

	primarybuf->wfx.nAvgBytesPerSec =
		This->wfx.nSamplesPerSec * This->wfx.nBlockAlign;

	DSOUND_CloseAudio();

	LeaveCriticalSection(&(primarybuf->lock));
	/* **** */

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_SetVolume(
	LPDIRECTSOUNDBUFFER iface,LONG vol
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	double	temp;

	TRACE("(%p,%ld)\n",This,vol);

	/* I'm not sure if we need this for primary buffer */
	if (!(This->dsbd.dwFlags & DSBCAPS_CTRLVOLUME))
		return DSERR_CONTROLUNAVAIL;

	if ((vol > DSBVOLUME_MAX) || (vol < DSBVOLUME_MIN))
		return DSERR_INVALIDPARAM;

	/* This needs to adjust the soundcard volume when */
	/* called for the primary buffer */
	if (This->dsbd.dwFlags & DSBCAPS_PRIMARYBUFFER) {
		FIXME("Volume control of primary unimplemented.\n");
		This->volume = vol;
		return DS_OK;
	}

	/* **** */
	EnterCriticalSection(&(This->lock));

	This->volume = vol;

	temp = (double) (This->volume - (This->pan > 0 ? This->pan : 0));
	This->lVolAdjust = (ULONG) (pow(2.0, temp / 600.0) * 32768.0);
	temp = (double) (This->volume + (This->pan < 0 ? This->pan : 0));
	This->rVolAdjust = (ULONG) (pow(2.0, temp / 600.0) * 32768.0);

	LeaveCriticalSection(&(This->lock));
	/* **** */

	TRACE("left = %lx, right = %lx\n", This->lVolAdjust, This->rVolAdjust);

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_GetVolume(
	LPDIRECTSOUNDBUFFER iface,LPLONG vol
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	TRACE("(%p,%p)\n",This,vol);

	if (vol == NULL)
		return DSERR_INVALIDPARAM;

	*vol = This->volume;
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

	if ((freq < DSBFREQUENCY_MIN) || (freq > DSBFREQUENCY_MAX))
		return DSERR_INVALIDPARAM;

	/* **** */
	EnterCriticalSection(&(This->lock));

	This->freq = freq;
	This->freqAdjust = (freq << DSOUND_FREQSHIFT) / primarybuf->wfx.nSamplesPerSec;
	This->nAvgBytesPerSec = freq * This->wfx.nBlockAlign;

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
	This->playflags = flags;
	This->playing = 1;
	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_Stop(LPDIRECTSOUNDBUFFER iface)
{
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	TRACE("(%p)\n",This);

	/* **** */
	EnterCriticalSection(&(This->lock));

	This->playing = 0;
	DSOUND_CheckEvent(This, 0);

	LeaveCriticalSection(&(This->lock));
	/* **** */

	return DS_OK;
}

static DWORD WINAPI IDirectSoundBufferImpl_AddRef(LPDIRECTSOUNDBUFFER iface) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
/*	TRACE(dsound,"(%p) ref was %ld\n",This, This->ref); */

	return ++(This->ref);
}
static DWORD WINAPI IDirectSoundBufferImpl_Release(LPDIRECTSOUNDBUFFER iface) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	int	i;

/*	TRACE(dsound,"(%p) ref was %ld\n",This, This->ref); */

	if (--This->ref)
		return This->ref;

	for (i=0;i<This->dsound->nrofbuffers;i++)
		if (This->dsound->buffers[i] == This)
			break;
	if (i < This->dsound->nrofbuffers) {
		/* Put the last buffer of the list in the (now empty) position */
		This->dsound->buffers[i] = This->dsound->buffers[This->dsound->nrofbuffers - 1];
		This->dsound->buffers = HeapReAlloc(GetProcessHeap(),0,This->dsound->buffers,sizeof(LPDIRECTSOUNDBUFFER)*This->dsound->nrofbuffers);
		This->dsound->nrofbuffers--;
		IDirectSound_Release((LPDIRECTSOUND)This->dsound);
	}

	DeleteCriticalSection(&(This->lock));

	if (This->ds3db && ICOM_VTBL(This->ds3db))
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

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_GetCurrentPosition(
	LPDIRECTSOUNDBUFFER iface,LPDWORD playpos,LPDWORD writepos
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	TRACE("(%p,%p,%p)\n",This,playpos,writepos);
	if (playpos) *playpos = This->playpos;
	if (writepos) *writepos = This->writepos;
	TRACE("playpos = %ld, writepos = %ld\n", playpos?*playpos:0, writepos?*writepos:0);
	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_GetStatus(
	LPDIRECTSOUNDBUFFER iface,LPDWORD status
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	TRACE("(%p,%p)\n",This,status);

	if (status == NULL)
		return DSERR_INVALIDPARAM;

	*status = 0;
	if (This->playing)
		*status |= DSBSTATUS_PLAYING;
	if (This->playflags & DSBPLAY_LOOPING)
		*status |= DSBSTATUS_LOOPING;

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

	TRACE("(%p,%ld,%ld,%p,%p,%p,%p,0x%08lx)\n",
		This,
		writecursor,
		writebytes,
		lplpaudioptr1,
		audiobytes1,
		lplpaudioptr2,
		audiobytes2,
		flags
	);
	if (flags & DSBLOCK_FROMWRITECURSOR)
		writecursor += This->writepos;
	if (flags & DSBLOCK_ENTIREBUFFER)
		writebytes = This->buflen;
	if (writebytes > This->buflen)
		writebytes = This->buflen;

	assert(audiobytes1!=audiobytes2);
	assert(lplpaudioptr1!=lplpaudioptr2);
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
	/* No. See file:///cdrom/sdk52/docs/worddoc/dsound.doc page 21 */
	/* This->writepos=(writecursor+writebytes)%This->buflen; */
	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_SetCurrentPosition(
	LPDIRECTSOUNDBUFFER iface,DWORD newpos
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	TRACE("(%p,%ld)\n",This,newpos);

	/* **** */
	EnterCriticalSection(&(This->lock));

	This->playpos = newpos;

	LeaveCriticalSection(&(This->lock));
	/* **** */

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_SetPan(
	LPDIRECTSOUNDBUFFER iface,LONG pan
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	double	temp;

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

	This->pan = pan;
	
	temp = (double) (This->volume - (This->pan > 0 ? This->pan : 0));
	This->lVolAdjust = (ULONG) (pow(2.0, temp / 600.0) * 32768.0);
	temp = (double) (This->volume + (This->pan < 0 ? This->pan : 0));
	This->rVolAdjust = (ULONG) (pow(2.0, temp / 600.0) * 32768.0);

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

	*pan = This->pan;

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_Unlock(
	LPDIRECTSOUNDBUFFER iface,LPVOID p1,DWORD x1,LPVOID p2,DWORD x2
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	TRACE("(%p,%p,%ld,%p,%ld):stub\n", This,p1,x1,p2,x2);

	/* There is really nothing to do here. Should someone */
	/* choose to implement static buffers in hardware (by */
	/* using a wave table synth, for example) this is where */
	/* you'd want to do the loading. For software buffers, */
	/* which is what we currently use, we need do nothing. */

#if 0
	/* It's also the place to pre-process 3D buffers... */
	
	/* This is highly experimental and liable to break things */
	if (This->dsbd.dwFlags & DSBCAPS_CTRL3D)
		DSOUND_Create3DBuffer(This);
#endif

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

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_Initialize(
	LPDIRECTSOUNDBUFFER iface,LPDIRECTSOUND dsound,LPDSBUFFERDESC dbsd
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	FIXME("(%p,%p,%p):stub\n",This,dsound,dbsd);
	printf("Re-Init!!!\n");
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

	caps->dwFlags = This->dsbd.dwFlags | DSBCAPS_LOCSOFTWARE;
	caps->dwBufferBytes = This->dsbd.dwBufferBytes;
	/* This value represents the speed of the "unlock" command.
	   As unlock is quite fast (it does not do anything), I put
	   4096 ko/s = 4 Mo / s */
	caps->dwUnlockTransferRate = 4096;
	caps->dwPlayCpuOverhead = 0;
	
	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_QueryInterface(
	LPDIRECTSOUNDBUFFER iface,REFIID riid,LPVOID *ppobj
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	char	xbuf[50];

	WINE_StringFromCLSID(riid,xbuf);
	TRACE("(%p,%s,%p)\n",This,xbuf,ppobj);

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

	if ( IsEqualGUID( &IID_IDirectSound3DBuffer, riid ) ) {
		*ppobj = This->ds3db;
		if (*ppobj)
			return DS_OK;
	}

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
	IDirectSoundBufferImpl_Unlock
};

/*******************************************************************************
 *		IDirectSound
 */

static HRESULT WINAPI IDirectSoundImpl_SetCooperativeLevel(
	LPDIRECTSOUND iface,HWND hwnd,DWORD level
) {
	ICOM_THIS(IDirectSoundImpl,iface);
	FIXME("(%p,%08lx,%ld):stub\n",This,(DWORD)hwnd,level);
	return DS_OK;
}

static HRESULT WINAPI IDirectSoundImpl_CreateSoundBuffer(
	LPDIRECTSOUND iface,LPDSBUFFERDESC dsbd,LPLPDIRECTSOUNDBUFFER ppdsb,LPUNKNOWN lpunk
) {
	ICOM_THIS(IDirectSoundImpl,iface);
	IDirectSoundBufferImpl** ippdsb=(IDirectSoundBufferImpl**)ppdsb;
	LPWAVEFORMATEX	wfex;

	TRACE("(%p,%p,%p,%p)\n",This,dsbd,ippdsb,lpunk);
	
	if ((This == NULL) || (dsbd == NULL) || (ippdsb == NULL))
		return DSERR_INVALIDPARAM;
	
	if (TRACE_ON(dsound)) {
		TRACE("(size=%ld)\n",dsbd->dwSize);
		TRACE("(flags=0x%08lx\n",dsbd->dwFlags);
		_dump_DSBCAPS(dsbd->dwFlags);
		TRACE("(bufferbytes=%ld)\n",dsbd->dwBufferBytes);
		TRACE("(lpwfxFormat=%p)\n",dsbd->lpwfxFormat);
	}

	wfex = dsbd->lpwfxFormat;

	if (wfex)
		TRACE("(formattag=0x%04x,chans=%d,samplerate=%ld"
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
		} /* Else create primarybuf */
	}

	*ippdsb = (IDirectSoundBufferImpl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectSoundBufferImpl));
	if (*ippdsb == NULL)
		return DSERR_OUTOFMEMORY;
	(*ippdsb)->ref = 1;

	TRACE("Created buffer at %p\n", *ippdsb);

	if (dsbd->dwFlags & DSBCAPS_PRIMARYBUFFER) {
		(*ippdsb)->buflen = dsound->wfx.nAvgBytesPerSec;
		(*ippdsb)->freq = dsound->wfx.nSamplesPerSec;
	} else {
		(*ippdsb)->buflen = dsbd->dwBufferBytes;
		(*ippdsb)->freq = dsbd->lpwfxFormat->nSamplesPerSec;
	}
	(*ippdsb)->buffer = (LPBYTE)HeapAlloc(GetProcessHeap(),0,(*ippdsb)->buflen);
	if ((*ippdsb)->buffer == NULL) {
		HeapFree(GetProcessHeap(),0,(*ippdsb));
		*ippdsb = NULL;
		return DSERR_OUTOFMEMORY;
	}
	/* It's not necessary to initialize values to zero since */
	/* we allocated this structure with HEAP_ZERO_MEMORY... */
	(*ippdsb)->playpos = 0;
	(*ippdsb)->writepos = 0;
	(*ippdsb)->parent = NULL;
	ICOM_VTBL(*ippdsb) = &dsbvt;
	(*ippdsb)->dsound = This;
	(*ippdsb)->playing = 0;
	(*ippdsb)->lVolAdjust = (1 << 15);
	(*ippdsb)->rVolAdjust = (1 << 15);

	if (!(dsbd->dwFlags & DSBCAPS_PRIMARYBUFFER)) {
		(*ippdsb)->freqAdjust = ((*ippdsb)->freq << DSOUND_FREQSHIFT) /
			primarybuf->wfx.nSamplesPerSec;
		(*ippdsb)->nAvgBytesPerSec = (*ippdsb)->freq *
			dsbd->lpwfxFormat->nBlockAlign;
	}

	memcpy(&((*ippdsb)->dsbd),dsbd,sizeof(*dsbd));

	/* register buffer */
	if (!(dsbd->dwFlags & DSBCAPS_PRIMARYBUFFER)) {
		This->buffers = (IDirectSoundBufferImpl**)HeapReAlloc(GetProcessHeap(),0,This->buffers,sizeof(IDirectSoundBufferImpl*)*(This->nrofbuffers+1));
		This->buffers[This->nrofbuffers] = *ippdsb;
		This->nrofbuffers++;
	}
	IDirectSound_AddRef(iface);

	if (dsbd->lpwfxFormat)
		memcpy(&((*ippdsb)->wfx), dsbd->lpwfxFormat, sizeof((*ippdsb)->wfx));

	InitializeCriticalSection(&((*ippdsb)->lock));
	
#if USE_DSOUND3D
	if (dsbd->dwFlags & DSBCAPS_CTRL3D) {
		IDirectSound3DBufferImpl	*ds3db;

		ds3db = (IDirectSound3DBufferImpl*)HeapAlloc(GetProcessHeap(),
			0,sizeof(*ds3db));
		ds3db->ref = 1;
		ds3db->dsb = (*ippdsb);
		ICOM_VTBL(ds3db) = &ds3dbvt;
		(*ippdsb)->ds3db = ds3db;
		ds3db->ds3db.dwSize = sizeof(DS3DBUFFER);
		ds3db->ds3db.vPosition.x.x = 0.0;
		ds3db->ds3db.vPosition.y.y = 0.0;
		ds3db->ds3db.vPosition.z.z = 0.0;
		ds3db->ds3db.vVelocity.x.x = 0.0;
		ds3db->ds3db.vVelocity.y.y = 0.0;
		ds3db->ds3db.vVelocity.z.z = 0.0;
		ds3db->ds3db.dwInsideConeAngle = DS3D_DEFAULTCONEANGLE;
		ds3db->ds3db.dwOutsideConeAngle = DS3D_DEFAULTCONEANGLE;
		ds3db->ds3db.vConeOrientation.x.x = 0.0;
		ds3db->ds3db.vConeOrientation.y.y = 0.0;
		ds3db->ds3db.vConeOrientation.z.z = 0.0;
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

	*ippdsb = (IDirectSoundBufferImpl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectSoundBufferImpl));

	IDirectSoundBuffer_AddRef(pdsb);
	memcpy(*ippdsb, ipdsb, sizeof(IDirectSoundBufferImpl));
	(*ippdsb)->ref = 1;
	(*ippdsb)->playpos = 0;
	(*ippdsb)->writepos = 0;
	(*ippdsb)->dsound = This;
	(*ippdsb)->parent = ipdsb;
	memcpy(&((*ippdsb)->wfx), &(ipdsb->wfx), sizeof((*ippdsb)->wfx));
	/* register buffer */
	This->buffers = (IDirectSoundBufferImpl**)HeapReAlloc(GetProcessHeap(),0,This->buffers,sizeof(IDirectSoundBufferImpl**)*(This->nrofbuffers+1));
	This->buffers[This->nrofbuffers] = *ippdsb;
	This->nrofbuffers++;
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

	caps->dwFlags =
		DSCAPS_PRIMARYSTEREO |
		DSCAPS_PRIMARY16BIT |
		DSCAPS_SECONDARYSTEREO |
		DSCAPS_SECONDARY16BIT |
		DSCAPS_CONTINUOUSRATE;
	/* FIXME: query OSS */
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
		DSOUND_CloseAudio();
		while(IDirectSoundBuffer_Release((LPDIRECTSOUNDBUFFER)primarybuf)); /* Deallocate */
		FIXME("need to release all buffers!\n");
		HeapFree(GetProcessHeap(),0,This);
		dsound = NULL;
		return S_OK;
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
	char xbuf[50];

	if ( IsEqualGUID( &IID_IDirectSound3DListener, riid ) ) {

		if (This->listener) {
			*ppobj = This->listener;
			return DS_OK;
		}
		This->listener = (IDirectSound3DListenerImpl*)HeapAlloc(
			GetProcessHeap(), 0, sizeof(*(This->listener)));
		This->listener->ref = 1;
		ICOM_VTBL(This->listener) = &ds3dlvt;
		IDirectSound_AddRef(iface);
		This->listener->ds3dl.dwSize = sizeof(DS3DLISTENER);
		This->listener->ds3dl.vPosition.x.x = 0.0;
		This->listener->ds3dl.vPosition.y.y = 0.0;
		This->listener->ds3dl.vPosition.z.z = 0.0;
		This->listener->ds3dl.vVelocity.x.x = 0.0;
		This->listener->ds3dl.vVelocity.y.y = 0.0;
		This->listener->ds3dl.vVelocity.z.z = 0.0;
		This->listener->ds3dl.vOrientFront.x.x = 0.0;
		This->listener->ds3dl.vOrientFront.y.y = 0.0;
		This->listener->ds3dl.vOrientFront.z.z = 1.0;
		This->listener->ds3dl.vOrientTop.x.x = 0.0;
		This->listener->ds3dl.vOrientTop.y.y = 1.0;
		This->listener->ds3dl.vOrientTop.z.z = 0.0;
		This->listener->ds3dl.flDistanceFactor = DS3D_DEFAULTDISTANCEFACTOR;
		This->listener->ds3dl.flRolloffFactor = DS3D_DEFAULTROLLOFFFACTOR;
		This->listener->ds3dl.flDopplerFactor = DS3D_DEFAULTDOPPLERFACTOR;
		*ppobj = (LPVOID)This->listener;
		return DS_OK;
	}

	WINE_StringFromCLSID(riid,xbuf);
	TRACE("(%p,%s,%p)\n",This,xbuf,ppobj);
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
	LPGUID lpGuid)
{
	ICOM_THIS(IDirectSoundImpl,iface);
	TRACE("(%p, %p)\n", This, lpGuid);
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


/* See http://www.opensound.com/pguide/audio.html for more details */

static int
DSOUND_setformat(LPWAVEFORMATEX wfex) {
	int	xx,channels,speed,format,nformat;

	if (!audioOK) {
		TRACE("(%p) deferred\n", wfex);
		return 0;
	}
	switch (wfex->wFormatTag) {
	default:
		WARN("unknown WAVE_FORMAT tag %d\n",wfex->wFormatTag);
		return DSERR_BADFORMAT;
	case WAVE_FORMAT_PCM:
		break;
	}
	if (wfex->wBitsPerSample==8)
		format = AFMT_U8;
	else
		format = AFMT_S16_LE;

	if (-1==ioctl(audiofd,SNDCTL_DSP_GETFMTS,&xx)) {
		perror("ioctl SNDCTL_DSP_GETFMTS");
		return -1;
	}
	if ((xx&format)!=format) {/* format unsupported */
		FIXME("SNDCTL_DSP_GETFMTS: format not supported\n"); 
		return -1;
	}
	nformat = format;
	if (-1==ioctl(audiofd,SNDCTL_DSP_SETFMT,&nformat)) {
		perror("ioctl SNDCTL_DSP_SETFMT");
		return -1;
	}
	if (nformat!=format) {/* didn't work */
		FIXME("SNDCTL_DSP_GETFMTS: format not set\n"); 
		return -1;
	}

	channels = wfex->nChannels-1;
	if (-1==ioctl(audiofd,SNDCTL_DSP_STEREO,&channels)) {
		perror("ioctl SNDCTL_DSP_STEREO");
		return -1;
	}
	speed = wfex->nSamplesPerSec;
	if (-1==ioctl(audiofd,SNDCTL_DSP_SPEED,&speed)) {
		perror("ioctl SNDCTL_DSP_SPEED");
		return -1;
	}
	TRACE("(freq=%ld,channels=%d,bits=%d)\n",
		wfex->nSamplesPerSec,wfex->nChannels,wfex->wBitsPerSample
	);
	return 0;
}

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
			if (dsb->playing == 0) {
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

	/* TRACE(dsound, "(%p)", buf); */
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

	ibp = dsb->buffer + dsb->playpos;
	obp = buf;

	TRACE("(%p, %p, %p), playpos=%8.8lx\n", dsb, ibp, obp, dsb->playpos);
	/* Check for the best case */
	if ((dsb->freq == primarybuf->wfx.nSamplesPerSec) &&
	    (dsb->wfx.wBitsPerSample == primarybuf->wfx.wBitsPerSample) &&
	    (dsb->wfx.nChannels == primarybuf->wfx.nChannels)) {
		TRACE("(%p) Best case\n", dsb);
	    	if ((ibp + len) < (BYTE *)(dsb->buffer + dsb->buflen))
			memcpy(obp, ibp, len);
		else { /* wrap */
			memcpy(obp, ibp, dsb->buflen - dsb->playpos);
			memcpy(obp + (dsb->buflen - dsb->playpos),
			    dsb->buffer,
			    len - (dsb->buflen - dsb->playpos));
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

	TRACE("(%p) Adjusting frequency: %ld -> %ld\n",
		dsb, dsb->freq, primarybuf->wfx.nSamplesPerSec);

	size = len / oAdvance;
	ilen = ((size * dsb->freqAdjust) >> DSOUND_FREQSHIFT) * iAdvance;
	for (i = 0; i < size; i++) {

		ipos = (((i * dsb->freqAdjust) >> DSOUND_FREQSHIFT) * iAdvance) + dsb->playpos;

		if (ipos >= dsb->buflen)
			ipos %= dsb->buflen;	/* wrap */

		get_fields(dsb, (dsb->buffer + ipos), &fieldL, &fieldR);
		set_fields(obp, fieldL, fieldR);
		obp += oAdvance;
	}
	return ilen;
}

static void DSOUND_MixerVol(IDirectSoundBufferImpl *dsb, BYTE *buf, INT len)
{
	INT	i, inc = primarybuf->wfx.wBitsPerSample >> 3;
	BYTE	*bpc = buf;
	INT16	*bps = (INT16 *) buf;
	
	TRACE("(%p) left = %lx, right = %lx\n", dsb,
		dsb->lVolAdjust, dsb->rVolAdjust);
	if ((!(dsb->dsbd.dwFlags & DSBCAPS_CTRLPAN) || (dsb->pan == 0)) &&
	    (!(dsb->dsbd.dwFlags & DSBCAPS_CTRLVOLUME) || (dsb->volume == 0)) &&
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
			val = ((val * (i & inc ? dsb->rVolAdjust : dsb->lVolAdjust)) >> 15);
			*bpc = val + 128;
			bpc++;
			break;
		case 2:
			/* 16-bit WAV is signed -- much better */
			val = *bps;
			val = ((val * ((i & inc) ? dsb->rVolAdjust : dsb->lVolAdjust)) >> 15);
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
	DWORD	buflen, playpos;

	buflen = dsb->ds3db->buflen;
	playpos = (dsb->playpos * primarybuf->wfx.nBlockAlign) / dsb->wfx.nBlockAlign;
	ibp = dsb->ds3db->buffer + playpos;
	obp = buf;

	if (playpos > buflen) {
		FIXME("Major breakage");
		return;
	}

	if (len <= (playpos + buflen))
		memcpy(obp, ibp, len);
	else { /* wrap */
		memcpy(obp, ibp, buflen - playpos);
		memcpy(obp + (buflen - playpos),
		    dsb->buffer,
		    len - (buflen - playpos));
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

static DWORD DSOUND_MixInBuffer(IDirectSoundBufferImpl *dsb)
{
	INT	i, len, ilen, temp, field;
	INT	advance = primarybuf->wfx.wBitsPerSample >> 3;
	BYTE	*buf, *ibuf, *obuf;
	INT16	*ibufs, *obufs;

	len = DSOUND_FRAGLEN;			/* The most we will use */
	if (!(dsb->playflags & DSBPLAY_LOOPING)) {
		temp = MulDiv(primarybuf->wfx.nAvgBytesPerSec, dsb->buflen,
			dsb->nAvgBytesPerSec) -
		       MulDiv(primarybuf->wfx.nAvgBytesPerSec, dsb->playpos,
			dsb->nAvgBytesPerSec);
		len = (len > temp) ? temp : len;
	}
	len &= ~3;				/* 4 byte alignment */

	if (len == 0) {
		/* This should only happen if we aren't looping and temp < 4 */

		/* We skip the remainder, so check for possible events */
		DSOUND_CheckEvent(dsb, dsb->buflen - dsb->playpos);
		/* Stop */
		dsb->playing = 0;
		dsb->writepos = 0;
		dsb->playpos = 0;
		/* Check for DSBPN_OFFSETSTOP */
		DSOUND_CheckEvent(dsb, 0);
		return 0;
	}

	/* Been seeing segfaults in malloc() for some reason... */
	TRACE("allocating buffer (size = %d)\n", len);
	if ((buf = ibuf = (BYTE *) DSOUND_tmpbuffer(len)) == NULL)
		return 0;

	TRACE("MixInBuffer (%p) len = %d\n", dsb, len);

	ilen = DSOUND_MixerNorm(dsb, ibuf, len);
	if ((dsb->dsbd.dwFlags & DSBCAPS_CTRLPAN) ||
	    (dsb->dsbd.dwFlags & DSBCAPS_CTRLVOLUME))
		DSOUND_MixerVol(dsb, ibuf, len);

	obuf = primarybuf->buffer + primarybuf->playpos;
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

	dsb->playpos += ilen;
	dsb->writepos = dsb->playpos + ilen;
	
	if (dsb->playpos >= dsb->buflen) {
		if (!(dsb->playflags & DSBPLAY_LOOPING)) {
			dsb->playing = 0;
			dsb->writepos = 0;
			dsb->playpos = 0;
			DSOUND_CheckEvent(dsb, 0);		/* For DSBPN_OFFSETSTOP */
		} else
			dsb->playpos %= dsb->buflen;		/* wrap */
	}
	
	if (dsb->writepos >= dsb->buflen)
		dsb->writepos %= dsb->buflen;

	return len;
}

static DWORD WINAPI DSOUND_MixPrimary(void)
{
	INT			i, len, maxlen = 0;
	IDirectSoundBufferImpl	*dsb;

	for (i = dsound->nrofbuffers - 1; i >= 0; i--) {
		dsb = dsound->buffers[i];

		if (!dsb || !(ICOM_VTBL(dsb)))
			continue;
		IDirectSoundBuffer_AddRef((LPDIRECTSOUNDBUFFER)dsb);
 		if (dsb->buflen && dsb->playing) {
			EnterCriticalSection(&(dsb->lock));
			len = DSOUND_MixInBuffer(dsb);
			maxlen = len > maxlen ? len : maxlen;
			LeaveCriticalSection(&(dsb->lock));
		}
		IDirectSoundBuffer_Release((LPDIRECTSOUNDBUFFER)dsb);
	}
	
	return maxlen;
}

static int DSOUND_OpenAudio(void)
{
	int	audioFragment;

	if (primarybuf == NULL)
		return DSERR_OUTOFMEMORY;

	while (audiofd >= 0)

		sleep(5);
	/* we will most likely not get one, avoid excessive opens ... */
	if (audiofd == -ENODEV)
		return -1;
	audiofd = open("/dev/audio",O_WRONLY);
	if (audiofd==-1) {
		/* Don't worry if sound is busy at the moment */
		if ((errno != EBUSY) && (errno != ENODEV))
			perror("open /dev/audio");
		audiofd = -errno;
		return -1; /* -1 */
	}

	/* We should probably do something here if SETFRAGMENT fails... */
	audioFragment=0x0002000c;
	if (-1==ioctl(audiofd,SNDCTL_DSP_SETFRAGMENT,&audioFragment))
		perror("ioctl SETFRAGMENT");

	audioOK = 1;
	DSOUND_setformat(&(primarybuf->wfx));

	return 0;
}

static void DSOUND_CloseAudio(void)
{
	int	neutral;
	
	neutral = primarybuf->wfx.wBitsPerSample == 8 ? 128 : 0;
	audioOK = 0;	/* race condition */
	Sleep(5);
	/* It's possible we've been called with audio closed */
	/* from SetFormat()... this is just to force a call */
	/* to OpenAudio() to reset the hardware properly */
	if (audiofd >= 0)
		close(audiofd);
	primarybuf->playpos = 0;
	primarybuf->writepos = DSOUND_FRAGLEN;
	memset(primarybuf->buffer, neutral, primarybuf->buflen);
	audiofd = -1;
	TRACE("Audio stopped\n");
}
	
static int DSOUND_WriteAudio(char *buf, int len)
{
	int	result, left = 0;

	while (left < len) {
		result = write(audiofd, buf + left, len - left);
		if (result == -1) {
			if (errno == EINTR)
				continue;
			else
				return result;
		}
		left += result;
	}
	return 0;
}

static void DSOUND_OutputPrimary(int len)
{
	int	neutral, flen1, flen2;
	char	*frag1, *frag2;
	
	/* This is a bad place for this. We need to clear the */
	/* buffer with a neutral value, for unsigned 8-bit WAVE */
	/* that's 128, for signed 16-bit it's 0 */
	neutral = primarybuf->wfx.wBitsPerSample == 8 ? 128 : 0;
	
	/* **** */
	EnterCriticalSection(&(primarybuf->lock));

	/* Write out the  */
	if ((audioOK == 1) || (DSOUND_OpenAudio() == 0)) {
		if (primarybuf->playpos + len >= primarybuf->buflen) {
			frag1 = primarybuf->buffer + primarybuf->playpos;
			flen1 = primarybuf->buflen - primarybuf->playpos;
			frag2 = primarybuf->buffer;
			flen2 = len - (primarybuf->buflen - primarybuf->playpos);
			if (DSOUND_WriteAudio(frag1, flen1) != 0) {
				perror("DSOUND_WriteAudio");
				LeaveCriticalSection(&(primarybuf->lock));
				ExitThread(0);
			}
			memset(frag1, neutral, flen1);
			if (DSOUND_WriteAudio(frag2, flen2) != 0) {
				perror("DSOUND_WriteAudio");
				LeaveCriticalSection(&(primarybuf->lock));
				ExitThread(0);
			}
			memset(frag2, neutral, flen2);
		} else {
			frag1 = primarybuf->buffer + primarybuf->playpos;
			flen1 = len;
			if (DSOUND_WriteAudio(frag1, flen1) != 0) {
				perror("DSOUND_WriteAudio");
				LeaveCriticalSection(&(primarybuf->lock));
				ExitThread(0);
			}
			memset(frag1, neutral, flen1);
		}
	} else {
		/* Can't play audio at the moment -- we need to sleep */
		/* to make up for the time we'd be blocked in write() */
		/* to /dev/audio */
		Sleep(60);
	}
	primarybuf->playpos += len;
	if (primarybuf->playpos >= primarybuf->buflen)
		primarybuf->playpos %= primarybuf->buflen;
	primarybuf->writepos = primarybuf->playpos + DSOUND_FRAGLEN;
	if (primarybuf->writepos >= primarybuf->buflen)
		primarybuf->writepos %= primarybuf->buflen;

	LeaveCriticalSection(&(primarybuf->lock));
	/* **** */
}

static DWORD WINAPI DSOUND_thread(LPVOID arg)
{
	int	len;

	TRACE("dsound is at pid %d\n",getpid());
	while (1) {
		if (!dsound) {
			WARN("DSOUND thread giving up.\n");
			ExitThread(0);
		}
		if (getppid()==1) {
			WARN("DSOUND father died? Giving up.\n");
			ExitThread(0);
		}
		/* RACE: dsound could be deleted */
		IDirectSound_AddRef((LPDIRECTSOUND)dsound);
		if (primarybuf == NULL) {
			/* Should never happen */
			WARN("Lost the primary buffer!\n");
			IDirectSound_Release((LPDIRECTSOUND)dsound);
			ExitThread(0);
		}

		/* **** */
		EnterCriticalSection(&(primarybuf->lock));
		len = DSOUND_MixPrimary();
		LeaveCriticalSection(&(primarybuf->lock));
		/* **** */

		if (primarybuf->playing)
			len = DSOUND_FRAGLEN > len ? DSOUND_FRAGLEN : len;
		if (len) {
			/* This does all the work */
			DSOUND_OutputPrimary(len);
		} else {
			/* no buffers playing -- close and wait */
			if (audioOK)
				DSOUND_CloseAudio();
			Sleep(100);
		}
		IDirectSound_Release((LPDIRECTSOUND)dsound);
	}
	ExitThread(0);
}

#endif /* HAVE_OSS */

HRESULT WINAPI DirectSoundCreate(REFGUID lpGUID,LPDIRECTSOUND *ppDS,IUnknown *pUnkOuter )
{
	IDirectSoundImpl** ippDS=(IDirectSoundImpl**)ppDS;
	if (lpGUID)
		TRACE("(%p,%p,%p)\n",lpGUID,ippDS,pUnkOuter);
	else
		TRACE("DirectSoundCreate (%p)\n", ippDS);

#ifdef HAVE_OSS

	if (ippDS == NULL)
		return DSERR_INVALIDPARAM;

	if (primarybuf) {
		IDirectSound_AddRef((LPDIRECTSOUND)dsound);
		*ippDS = dsound;
		return DS_OK;
	}

	/* Check that we actually have audio capabilities */
	/* If we do, whether it's busy or not, we continue */
	/* otherwise we return with DSERR_NODRIVER */

	audiofd = open("/dev/audio",O_WRONLY);
	if (audiofd == -1) {
		audiofd = -errno;
		if (errno == ENODEV) {
			MESSAGE("No sound hardware found, but continuing anyway.\n");
		} else if (errno == EBUSY) {
			MESSAGE("Sound device busy, will keep trying.\n");
		} else {
			MESSAGE("Unexpected error (%d) while checking for sound support.\n",errno);
			return DSERR_GENERIC;
		}
	} else {
		close(audiofd);
		audiofd = -1;
	}

	*ippDS = (IDirectSoundImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(IDirectSoundImpl));
	if (*ippDS == NULL)
		return DSERR_OUTOFMEMORY;

	(*ippDS)->ref		= 1;
	ICOM_VTBL(*ippDS)	= &dsvt;
	(*ippDS)->buffers	= NULL;
	(*ippDS)->nrofbuffers	= 0;

	(*ippDS)->wfx.wFormatTag		= 1;
	(*ippDS)->wfx.nChannels		= 2;
	(*ippDS)->wfx.nSamplesPerSec	= 22050;
	(*ippDS)->wfx.nAvgBytesPerSec	= 44100;
	(*ippDS)->wfx.nBlockAlign	= 2;
	(*ippDS)->wfx.wBitsPerSample	= 8;

	if (!dsound) {
		HANDLE	hnd;
		DWORD		xid;

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
			dsound->primary = primarybuf;
		}
		memset(primarybuf->buffer, 128, primarybuf->buflen);
		hnd = CreateThread(NULL,0,DSOUND_thread,0,0,&xid);
	}
	return DS_OK;
#else
	MessageBoxA(0,"DirectSound needs the Open Sound System Driver, which has not been found by ./configure.","WINE DirectSound",MB_OK|MB_ICONSTOP);
	return DSERR_NODRIVER;
#endif
}

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
	char buf[80];

	if (HIWORD(riid))
	    WINE_StringFromCLSID(riid,buf);
	else
	    sprintf(buf,"<guid-0x%04x>",LOWORD(riid));
	FIXME("(%p)->(%s,%p),stub!\n",This,buf,ppobj);
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
	char buf[80];

	WINE_StringFromCLSID(riid,buf);
	TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,buf,ppobj);
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
	DSCF_QueryInterface,
	DSCF_AddRef,
	DSCF_Release,
	DSCF_CreateInstance,
	DSCF_LockServer
};
static IClassFactoryImpl DSOUND_CF = {&DSCF_Vtbl, 1 };

/*******************************************************************************
 * DllGetClassObject [DSOUND.4]
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
    char buf[80],xbuf[80];

    if (HIWORD(rclsid))
    	WINE_StringFromCLSID(rclsid,xbuf);
    else
    	sprintf(xbuf,"<guid-0x%04x>",LOWORD(rclsid));
    if (HIWORD(riid))
    	WINE_StringFromCLSID(riid,buf);
    else
    	sprintf(buf,"<guid-0x%04x>",LOWORD(riid));
    WINE_StringFromCLSID(riid,xbuf);
    TRACE("(%p,%p,%p)\n", xbuf, buf, ppv);
    if ( IsEqualCLSID( &IID_IClassFactory, riid ) ) {
    	*ppv = (LPVOID)&DSOUND_CF;
	IClassFactory_AddRef((IClassFactory*)*ppv);
    return S_OK;
    }

    FIXME("(%p,%p,%p): no interface found.\n", xbuf, buf, ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}


/*******************************************************************************
 * DllCanUnloadNow [DSOUND.3]  Determines whether the DLL is in use.
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
