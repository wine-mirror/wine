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
#include "winerror.h"
#include "multimedia.h"
#include "objbase.h"
#include "thread.h"
#include "debug.h"

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

static LPDIRECTSOUND	dsound = NULL;

static LPDIRECTSOUNDBUFFER	primarybuf = NULL;

static int DSOUND_setformat(LPWAVEFORMATEX wfex);
static void DSOUND_CheckEvent(IDirectSoundBuffer *dsb, int len);
static void DSOUND_CloseAudio(void);

#endif

HRESULT WINAPI DirectSoundEnumerate32A(
	LPDSENUMCALLBACK32A enumcb,
	LPVOID context)
{
	TRACE(dsound, "enumcb = %p, context = %p\n", enumcb, context);

#ifdef HAVE_OSS
	if (enumcb != NULL)
		enumcb(NULL,"WINE DirectSound using Open Sound System",
		    "sound",context);
#endif

	return 0;
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
static HRESULT WINAPI IDirectSound3DBuffer_QueryInterface(
	LPDIRECTSOUND3DBUFFER this, REFIID riid, LPVOID *ppobj)
{
	char xbuf[50];

	WINE_StringFromCLSID(riid,xbuf);
	TRACE(dsound,"(%p,%s,%p)\n",this,xbuf,ppobj);
	return E_FAIL;
}
	
static ULONG WINAPI IDirectSound3DBuffer_AddRef(LPDIRECTSOUND3DBUFFER this)
{
	this->ref++;
	return this->ref;
}

static ULONG WINAPI IDirectSound3DBuffer_Release(LPDIRECTSOUND3DBUFFER this)
{
	if(--this->ref)
		return this->ref;

	HeapFree(GetProcessHeap(),0,this->buffer);
	HeapFree(GetProcessHeap(),0,this);

	return 0;
}

/* IDirectSound3DBuffer methods */
static HRESULT WINAPI IDirectSound3DBuffer_GetAllParameters(
	LPDIRECTSOUND3DBUFFER this,
	LPDS3DBUFFER lpDs3dBuffer)
{
	FIXME(dsound,"stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBuffer_GetConeAngles(
	LPDIRECTSOUND3DBUFFER this,
	LPDWORD lpdwInsideConeAngle,
	LPDWORD lpdwOutsideConeAngle)
{
	FIXME(dsound,"stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBuffer_GetConeOrientation(
	LPDIRECTSOUND3DBUFFER this,
	LPD3DVECTOR lpvConeOrientation)
{
	FIXME(dsound,"stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBuffer_GetConeOutsideVolume(
	LPDIRECTSOUND3DBUFFER this,
	LPLONG lplConeOutsideVolume)
{
	FIXME(dsound,"stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBuffer_GetMaxDistance(
	LPDIRECTSOUND3DBUFFER this,
	LPD3DVALUE lpfMaxDistance)
{
	FIXME(dsound,"stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBuffer_GetMinDistance(
	LPDIRECTSOUND3DBUFFER this,
	LPD3DVALUE lpfMinDistance)
{
	FIXME(dsound,"stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBuffer_GetMode(
	LPDIRECTSOUND3DBUFFER this,
	LPDWORD lpdwMode)
{
	FIXME(dsound,"stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBuffer_GetPosition(
	LPDIRECTSOUND3DBUFFER this,
	LPD3DVECTOR lpvPosition)
{
	FIXME(dsound,"stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBuffer_GetVelocity(
	LPDIRECTSOUND3DBUFFER this,
	LPD3DVECTOR lpvVelocity)
{
	FIXME(dsound,"stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBuffer_SetAllParameters(
	LPDIRECTSOUND3DBUFFER this,
	LPCDS3DBUFFER lpcDs3dBuffer,
	DWORD dwApply)
{
	FIXME(dsound,"stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBuffer_SetConeAngles(
	LPDIRECTSOUND3DBUFFER this,
	DWORD dwInsideConeAngle,
	DWORD dwOutsideConeAngle,
	DWORD dwApply)
{
	FIXME(dsound,"stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBuffer_SetConeOrientation(
	LPDIRECTSOUND3DBUFFER this,
	D3DVALUE x, D3DVALUE y, D3DVALUE z,
	DWORD dwApply)
{
	FIXME(dsound,"stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBuffer_SetConeOutsideVolume(
	LPDIRECTSOUND3DBUFFER this,
	LONG lConeOutsideVolume,
	DWORD dwApply)
{
	FIXME(dsound,"stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBuffer_SetMaxDistance(
	LPDIRECTSOUND3DBUFFER this,
	D3DVALUE fMaxDistance,
	DWORD dwApply)
{
	FIXME(dsound,"stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBuffer_SetMinDistance(
	LPDIRECTSOUND3DBUFFER this,
	D3DVALUE fMinDistance,
	DWORD dwApply)
{
	FIXME(dsound,"stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBuffer_SetMode(
	LPDIRECTSOUND3DBUFFER this,
	DWORD dwMode,
	DWORD dwApply)
{
	TRACE(dsound, "mode = %lx\n", dwMode);
	this->ds3db.dwMode = dwMode;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBuffer_SetPosition(
	LPDIRECTSOUND3DBUFFER this,
	D3DVALUE x, D3DVALUE y, D3DVALUE z,
	DWORD dwApply)
{
	FIXME(dsound,"stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBuffer_SetVelocity(
	LPDIRECTSOUND3DBUFFER this,
	D3DVALUE x, D3DVALUE y, D3DVALUE z,
	DWORD dwApply)
{
	FIXME(dsound,"stub\n");
	return DS_OK;
}

IDirectSound3DBuffer_VTable ds3dbvt = {
	/* IUnknown methods */
	IDirectSound3DBuffer_QueryInterface,
	IDirectSound3DBuffer_AddRef,
	IDirectSound3DBuffer_Release,
	/* IDirectSound3DBuffer methods */
	IDirectSound3DBuffer_GetAllParameters,
	IDirectSound3DBuffer_GetConeAngles,
	IDirectSound3DBuffer_GetConeOrientation,
	IDirectSound3DBuffer_GetConeOutsideVolume,
	IDirectSound3DBuffer_GetMaxDistance,
	IDirectSound3DBuffer_GetMinDistance,
	IDirectSound3DBuffer_GetMode,
	IDirectSound3DBuffer_GetPosition,
	IDirectSound3DBuffer_GetVelocity,
	IDirectSound3DBuffer_SetAllParameters,
	IDirectSound3DBuffer_SetConeAngles,
	IDirectSound3DBuffer_SetConeOrientation,
	IDirectSound3DBuffer_SetConeOutsideVolume,
	IDirectSound3DBuffer_SetMaxDistance,
	IDirectSound3DBuffer_SetMinDistance,
	IDirectSound3DBuffer_SetMode,
	IDirectSound3DBuffer_SetPosition,
	IDirectSound3DBuffer_SetVelocity,
};

#ifdef USE_DSOUND3D
static int DSOUND_Create3DBuffer(LPDIRECTSOUNDBUFFER dsb)
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
static HRESULT WINAPI IDirectSound3DListener_QueryInterface(
	LPDIRECTSOUND3DLISTENER this, REFIID riid, LPVOID *ppobj)
{
	char xbuf[50];

	WINE_StringFromCLSID(riid,xbuf);
	TRACE(dsound,"(%p,%s,%p)\n",this,xbuf,ppobj);
	return E_FAIL;
}
	
static ULONG WINAPI IDirectSound3DListener_AddRef(LPDIRECTSOUND3DLISTENER this)
{
	this->ref++;
	return this->ref;
}

static ULONG WINAPI IDirectSound3DListener_Release(LPDIRECTSOUND3DLISTENER this)
{
	this->ref--;
	return this->ref;
}

/* IDirectSound3DListener methods */
static HRESULT WINAPI IDirectSound3DListener_GetAllParameter(
	LPDIRECTSOUND3DLISTENER this,
	LPDS3DLISTENER lpDS3DL)
{
	FIXME(dsound,"stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListener_GetDistanceFactor(
	LPDIRECTSOUND3DLISTENER this,
	LPD3DVALUE lpfDistanceFactor)
{
	FIXME(dsound,"stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListener_GetDopplerFactor(
	LPDIRECTSOUND3DLISTENER this,
	LPD3DVALUE lpfDopplerFactor)
{
	FIXME(dsound,"stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListener_GetOrientation(
	LPDIRECTSOUND3DLISTENER this,
	LPD3DVECTOR lpvOrientFront,
	LPD3DVECTOR lpvOrientTop)
{
	FIXME(dsound,"stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListener_GetPosition(
	LPDIRECTSOUND3DLISTENER this,
	LPD3DVECTOR lpvPosition)
{
	FIXME(dsound,"stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListener_GetRolloffFactor(
	LPDIRECTSOUND3DLISTENER this,
	LPD3DVALUE lpfRolloffFactor)
{
	FIXME(dsound,"stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListener_GetVelocity(
	LPDIRECTSOUND3DLISTENER this,
	LPD3DVECTOR lpvVelocity)
{
	FIXME(dsound,"stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListener_SetAllParameters(
	LPDIRECTSOUND3DLISTENER this,
	LPCDS3DLISTENER lpcDS3DL,
	DWORD dwApply)
{
	FIXME(dsound,"stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListener_SetDistanceFactor(
	LPDIRECTSOUND3DLISTENER this,
	D3DVALUE fDistanceFactor,
	DWORD dwApply)
{
	FIXME(dsound,"stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListener_SetDopplerFactor(
	LPDIRECTSOUND3DLISTENER this,
	D3DVALUE fDopplerFactor,
	DWORD dwApply)
{
	FIXME(dsound,"stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListener_SetOrientation(
	LPDIRECTSOUND3DLISTENER this,
	D3DVALUE xFront, D3DVALUE yFront, D3DVALUE zFront,
	D3DVALUE xTop, D3DVALUE yTop, D3DVALUE zTop,
	DWORD dwApply)
{
	FIXME(dsound,"stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListener_SetPosition(
	LPDIRECTSOUND3DLISTENER this,
	D3DVALUE x, D3DVALUE y, D3DVALUE z,
	DWORD dwApply)
{
	FIXME(dsound,"stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListener_SetRolloffFactor(
	LPDIRECTSOUND3DLISTENER this,
	D3DVALUE fRolloffFactor,
	DWORD dwApply)
{
	FIXME(dsound,"stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListener_SetVelocity(
	LPDIRECTSOUND3DLISTENER this,
	D3DVALUE x, D3DVALUE y, D3DVALUE z,
	DWORD dwApply)
{
	FIXME(dsound,"stub\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListener_CommitDeferredSettings(
	LPDIRECTSOUND3DLISTENER this)

{
	FIXME(dsound,"stub\n");
	return DS_OK;
}

IDirectSound3DListener_VTable ds3dlvt = {
	/* IUnknown methods */
	IDirectSound3DListener_QueryInterface,
	IDirectSound3DListener_AddRef,
	IDirectSound3DListener_Release,
	/* IDirectSound3DListener methods */
	IDirectSound3DListener_GetAllParameter,
	IDirectSound3DListener_GetDistanceFactor,
	IDirectSound3DListener_GetDopplerFactor,
	IDirectSound3DListener_GetOrientation,
	IDirectSound3DListener_GetPosition,
	IDirectSound3DListener_GetRolloffFactor,
	IDirectSound3DListener_GetVelocity,
	IDirectSound3DListener_SetAllParameters,
	IDirectSound3DListener_SetDistanceFactor,
	IDirectSound3DListener_SetDopplerFactor,
	IDirectSound3DListener_SetOrientation,
	IDirectSound3DListener_SetPosition,
	IDirectSound3DListener_SetRolloffFactor,
	IDirectSound3DListener_SetVelocity,
	IDirectSound3DListener_CommitDeferredSettings,
};

/*******************************************************************************
 *		IDirectSoundNotify
 */
static HRESULT WINAPI IDirectSoundNotify_QueryInterface(
	LPDIRECTSOUNDNOTIFY this,REFIID riid,LPVOID *ppobj
) {
	char xbuf[50];

	WINE_StringFromCLSID(riid,xbuf);
	TRACE(dsound,"(%p,%s,%p)\n",this,xbuf,ppobj);
	return E_FAIL;
}

static ULONG WINAPI IDirectSoundNotify_AddRef(LPDIRECTSOUNDNOTIFY this) {
	return ++(this->ref);
}

static ULONG WINAPI IDirectSoundNotify_Release(LPDIRECTSOUNDNOTIFY this) {
	this->ref--;
	if (!this->ref) {
		this->dsb->lpvtbl->fnRelease(this->dsb);
		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
	return this->ref;
}

static HRESULT WINAPI IDirectSoundNotify_SetNotificationPositions(
	LPDIRECTSOUNDNOTIFY this,DWORD howmuch,LPCDSBPOSITIONNOTIFY notify
) {
	int	i;

	if (TRACE_ON(dsound)) {
	    TRACE(dsound,"(%p,0x%08lx,%p)\n",this,howmuch,notify);
	    for (i=0;i<howmuch;i++)
		    TRACE(dsound,"notify at %ld to 0x%08lx\n",
                            notify[i].dwOffset,(DWORD)notify[i].hEventNotify);
	}
	this->dsb->notifies = HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,this->dsb->notifies,(this->dsb->nrofnotifies+howmuch)*sizeof(DSBPOSITIONNOTIFY));
	memcpy(	this->dsb->notifies+this->dsb->nrofnotifies,
		notify,
		howmuch*sizeof(DSBPOSITIONNOTIFY)
	);
	this->dsb->nrofnotifies+=howmuch;

	return 0;
}

IDirectSoundNotify_VTable dsnvt = {
	IDirectSoundNotify_QueryInterface,
	IDirectSoundNotify_AddRef,
	IDirectSoundNotify_Release,
	IDirectSoundNotify_SetNotificationPositions,
};

/*******************************************************************************
 *		IDirectSoundBuffer
 */

/* This sets this format for the <em>Primary Buffer Only</em> */
/* See file:///cdrom/sdk52/docs/worddoc/dsound.doc page 120 */
static HRESULT WINAPI IDirectSoundBuffer_SetFormat(
	LPDIRECTSOUNDBUFFER this,LPWAVEFORMATEX wfex
) {
	LPDIRECTSOUNDBUFFER	*dsb;
	int			i;

	/* Let's be pedantic! */
	if ((wfex == NULL) ||
	    (wfex->wFormatTag != WAVE_FORMAT_PCM) ||
	    (wfex->nChannels < 1) || (wfex->nChannels > 2) ||
	    (wfex->nSamplesPerSec < 1) ||
	    (wfex->nBlockAlign < 1) || (wfex->nChannels > 4) ||
	    ((wfex->wBitsPerSample != 8) && (wfex->wBitsPerSample != 16))) {
		TRACE(dsound, "failed pedantic check!\n");
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

	TRACE(dsound,"(formattag=0x%04x,chans=%d,samplerate=%ld"
		   "bytespersec=%ld,blockalign=%d,bitspersamp=%d,cbSize=%d)\n",
		   wfex->wFormatTag, wfex->nChannels, wfex->nSamplesPerSec,
		   wfex->nAvgBytesPerSec, wfex->nBlockAlign, 
		   wfex->wBitsPerSample, wfex->cbSize);

	primarybuf->wfx.nAvgBytesPerSec =
		this->wfx.nSamplesPerSec * this->wfx.nBlockAlign;

	DSOUND_CloseAudio();

	LeaveCriticalSection(&(primarybuf->lock));
	/* **** */

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBuffer_SetVolume(
	LPDIRECTSOUNDBUFFER this,LONG vol
) {
	double	temp;

	TRACE(dsound,"(%p,%ld)\n",this,vol);

	/* I'm not sure if we need this for primary buffer */
	if (!(this->dsbd.dwFlags & DSBCAPS_CTRLVOLUME))
		return DSERR_CONTROLUNAVAIL;

	if ((vol > DSBVOLUME_MAX) || (vol < DSBVOLUME_MIN))
		return DSERR_INVALIDPARAM;

	/* This needs to adjust the soundcard volume when */
	/* called for the primary buffer */
	if (this->dsbd.dwFlags & DSBCAPS_PRIMARYBUFFER) {
		FIXME(dsound, "Volume control of primary unimplemented.\n");
		this->volume = vol;
		return DS_OK;
	}

	/* **** */
	EnterCriticalSection(&(this->lock));

	this->volume = vol;

	temp = (double) (this->volume - (this->pan > 0 ? this->pan : 0));
	this->lVolAdjust = (ULONG) (pow(2.0, temp / 600.0) * 32768.0);
	temp = (double) (this->volume + (this->pan < 0 ? this->pan : 0));
	this->rVolAdjust = (ULONG) (pow(2.0, temp / 600.0) * 32768.0);

	LeaveCriticalSection(&(this->lock));
	/* **** */

	TRACE(dsound, "left = %lx, right = %lx\n", this->lVolAdjust, this->rVolAdjust);

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBuffer_GetVolume(
	LPDIRECTSOUNDBUFFER this,LPLONG vol
) {
	TRACE(dsound,"(%p,%p)\n",this,vol);

	if (vol == NULL)
		return DSERR_INVALIDPARAM;

	*vol = this->volume;
	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBuffer_SetFrequency(
	LPDIRECTSOUNDBUFFER this,DWORD freq
) {
	TRACE(dsound,"(%p,%ld)\n",this,freq);

	/* You cannot set the frequency of the primary buffer */
	if (!(this->dsbd.dwFlags & DSBCAPS_CTRLFREQUENCY) ||
	    (this->dsbd.dwFlags & DSBCAPS_PRIMARYBUFFER))
		return DSERR_CONTROLUNAVAIL;

	if ((freq < DSBFREQUENCY_MIN) || (freq > DSBFREQUENCY_MAX))
		return DSERR_INVALIDPARAM;

	/* **** */
	EnterCriticalSection(&(this->lock));

	this->freq = freq;
	this->freqAdjust = (freq << DSOUND_FREQSHIFT) / primarybuf->wfx.nSamplesPerSec;
	this->nAvgBytesPerSec = freq * this->wfx.nBlockAlign;

	LeaveCriticalSection(&(this->lock));
	/* **** */

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBuffer_Play(
	LPDIRECTSOUNDBUFFER this,DWORD reserved1,DWORD reserved2,DWORD flags
) {
	TRACE(dsound,"(%p,%08lx,%08lx,%08lx)\n",
		this,reserved1,reserved2,flags
	);
	this->playflags = flags;
	this->playing = 1;
	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBuffer_Stop(LPDIRECTSOUNDBUFFER this)
{
	TRACE(dsound,"(%p)\n",this);

	/* **** */
	EnterCriticalSection(&(this->lock));

	this->playing = 0;
	DSOUND_CheckEvent(this, 0);

	LeaveCriticalSection(&(this->lock));
	/* **** */

	return DS_OK;
}

static DWORD WINAPI IDirectSoundBuffer_AddRef(LPDIRECTSOUNDBUFFER this) {
/*	TRACE(dsound,"(%p) ref was %ld\n",this, this->ref); */

	return ++(this->ref);
}
static DWORD WINAPI IDirectSoundBuffer_Release(LPDIRECTSOUNDBUFFER this) {
	int	i;

/*	TRACE(dsound,"(%p) ref was %ld\n",this, this->ref); */

	if (--this->ref)
		return this->ref;

	for (i=0;i<this->dsound->nrofbuffers;i++)
		if (this->dsound->buffers[i] == this)
			break;
	if (i < this->dsound->nrofbuffers) {
		/* Put the last buffer of the list in the (now empty) position */
		this->dsound->buffers[i] = this->dsound->buffers[this->dsound->nrofbuffers - 1];
		this->dsound->buffers = HeapReAlloc(GetProcessHeap(),0,this->dsound->buffers,sizeof(LPDIRECTSOUNDBUFFER)*this->dsound->nrofbuffers);
		this->dsound->nrofbuffers--;
		this->dsound->lpvtbl->fnRelease(this->dsound);
	}

	DeleteCriticalSection(&(this->lock));

	if (this->ds3db && this->ds3db->lpvtbl)
		this->ds3db->lpvtbl->fnRelease(this->ds3db);

	if (this->parent)
		/* this is a duplicate buffer */
		this->parent->lpvtbl->fnRelease(this->parent);
	else
		/* this is a toplevel buffer */
		HeapFree(GetProcessHeap(),0,this->buffer);

	HeapFree(GetProcessHeap(),0,this);
	
	if (this == primarybuf)
		primarybuf = NULL;

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBuffer_GetCurrentPosition(
	LPDIRECTSOUNDBUFFER this,LPDWORD playpos,LPDWORD writepos
) {
	TRACE(dsound,"(%p,%p,%p)\n",this,playpos,writepos);
	if (playpos) *playpos = this->playpos;
	if (writepos) *writepos = this->writepos;
	TRACE(dsound, "playpos = %ld, writepos = %ld\n", *playpos, *writepos);
	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBuffer_GetStatus(
	LPDIRECTSOUNDBUFFER this,LPDWORD status
) {
	TRACE(dsound,"(%p,%p)\n",this,status);

	if (status == NULL)
		return DSERR_INVALIDPARAM;

	*status = 0;
	if (this->playing)
		*status |= DSBSTATUS_PLAYING;
	if (this->playflags & DSBPLAY_LOOPING)
		*status |= DSBSTATUS_LOOPING;

	return DS_OK;
}


static HRESULT WINAPI IDirectSoundBuffer_GetFormat(
	LPDIRECTSOUNDBUFFER this,LPWAVEFORMATEX lpwf,DWORD wfsize,LPDWORD wfwritten
) {
	TRACE(dsound,"(%p,%p,%ld,%p)\n",this,lpwf,wfsize,wfwritten);

	if (wfsize>sizeof(this->wfx))
		wfsize = sizeof(this->wfx);
	if (lpwf) {	/* NULL is valid */
		memcpy(lpwf,&(this->wfx),wfsize);
		if (wfwritten)
			*wfwritten = wfsize;
	} else
		if (wfwritten)
			*wfwritten = sizeof(this->wfx);
		else
			return DSERR_INVALIDPARAM;

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBuffer_Lock(
	LPDIRECTSOUNDBUFFER this,DWORD writecursor,DWORD writebytes,LPVOID lplpaudioptr1,LPDWORD audiobytes1,LPVOID lplpaudioptr2,LPDWORD audiobytes2,DWORD flags
) {

	TRACE(dsound,"(%p,%ld,%ld,%p,%p,%p,%p,0x%08lx)\n",
		this,
		writecursor,
		writebytes,
		lplpaudioptr1,
		audiobytes1,
		lplpaudioptr2,
		audiobytes2,
		flags
	);
	if (flags & DSBLOCK_FROMWRITECURSOR)
		writecursor += this->writepos;
	if (flags & DSBLOCK_ENTIREBUFFER)
		writebytes = this->buflen;
	if (writebytes > this->buflen)
		writebytes = this->buflen;

	assert(audiobytes1!=audiobytes2);
	assert(lplpaudioptr1!=lplpaudioptr2);
	if (writecursor+writebytes <= this->buflen) {
		*(LPBYTE*)lplpaudioptr1 = this->buffer+writecursor;
		*audiobytes1 = writebytes;
		if (lplpaudioptr2)
			*(LPBYTE*)lplpaudioptr2 = NULL;
		if (audiobytes2)
			*audiobytes2 = 0;
		TRACE(dsound,"->%ld.0\n",writebytes);
	} else {
		*(LPBYTE*)lplpaudioptr1 = this->buffer+writecursor;
		*audiobytes1 = this->buflen-writecursor;
		if (lplpaudioptr2)
			*(LPBYTE*)lplpaudioptr2 = this->buffer;
		if (audiobytes2)
			*audiobytes2 = writebytes-(this->buflen-writecursor);
		TRACE(dsound,"->%ld.%ld\n",*audiobytes1,audiobytes2?*audiobytes2:0);
	}
	/* No. See file:///cdrom/sdk52/docs/worddoc/dsound.doc page 21 */
	/* this->writepos=(writecursor+writebytes)%this->buflen; */
	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBuffer_SetCurrentPosition(
	LPDIRECTSOUNDBUFFER this,DWORD newpos
) {
	TRACE(dsound,"(%p,%ld)\n",this,newpos);

	/* **** */
	EnterCriticalSection(&(this->lock));

	this->playpos = newpos;

	LeaveCriticalSection(&(this->lock));
	/* **** */

	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_SetPan(
	LPDIRECTSOUNDBUFFER this,LONG pan
) {
	double	temp;

	TRACE(dsound,"(%p,%ld)\n",this,pan);

	if ((pan > DSBPAN_RIGHT) || (pan < DSBPAN_LEFT))
		return DSERR_INVALIDPARAM;

	/* You cannot set the pan of the primary buffer */
	/* and you cannot use both pan and 3D controls */
	if (!(this->dsbd.dwFlags & DSBCAPS_CTRLPAN) ||
	    (this->dsbd.dwFlags & DSBCAPS_CTRL3D) ||
	    (this->dsbd.dwFlags & DSBCAPS_PRIMARYBUFFER))
		return DSERR_CONTROLUNAVAIL;

	/* **** */
	EnterCriticalSection(&(this->lock));

	this->pan = pan;
	
	temp = (double) (this->volume - (this->pan > 0 ? this->pan : 0));
	this->lVolAdjust = (ULONG) (pow(2.0, temp / 600.0) * 32768.0);
	temp = (double) (this->volume + (this->pan < 0 ? this->pan : 0));
	this->rVolAdjust = (ULONG) (pow(2.0, temp / 600.0) * 32768.0);

	LeaveCriticalSection(&(this->lock));
	/* **** */

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBuffer_GetPan(
	LPDIRECTSOUNDBUFFER this,LPLONG pan
) {
	TRACE(dsound,"(%p,%p)\n",this,pan);

	if (pan == NULL)
		return DSERR_INVALIDPARAM;

	*pan = this->pan;

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBuffer_Unlock(
	LPDIRECTSOUNDBUFFER this,LPVOID p1,DWORD x1,LPVOID p2,DWORD x2
) {
	TRACE(dsound,"(%p,%p,%ld,%p,%ld):stub\n", this,p1,x1,p2,x2);

	/* There is really nothing to do here. Should someone */
	/* choose to implement static buffers in hardware (by */
	/* using a wave table synth, for example) this is where */
	/* you'd want to do the loading. For software buffers, */
	/* which is what we currently use, we need do nothing. */

#if 0
	/* It's also the place to pre-process 3D buffers... */
	
	/* This is highly experimental and liable to break things */
	if (this->dsbd.dwFlags & DSBCAPS_CTRL3D)
		DSOUND_Create3DBuffer(this);
#endif

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBuffer_GetFrequency(
	LPDIRECTSOUNDBUFFER this,LPDWORD freq
) {
	TRACE(dsound,"(%p,%p)\n",this,freq);

	if (freq == NULL)
		return DSERR_INVALIDPARAM;

	*freq = this->freq;

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBuffer_Initialize(
	LPDIRECTSOUNDBUFFER this,LPDIRECTSOUND dsound,LPDSBUFFERDESC dbsd
) {
	FIXME(dsound,"(%p,%p,%p):stub\n",this,dsound,dbsd);
	printf("Re-Init!!!\n");
	return DSERR_ALREADYINITIALIZED;
}

static HRESULT WINAPI IDirectSoundBuffer_GetCaps(
	LPDIRECTSOUNDBUFFER this,LPDSBCAPS caps
) {
  	TRACE(dsound,"(%p)->(%p)\n",this,caps);
  
	if (caps == NULL)
		return DSERR_INVALIDPARAM;

	/* I think we should check this value, not set it. See */
	/* Inside DirectX, p215. That should apply here, too. */
	caps->dwSize = sizeof(*caps);

	caps->dwFlags = this->dsbd.dwFlags | DSBCAPS_LOCSOFTWARE;
	caps->dwBufferBytes = this->dsbd.dwBufferBytes;
	/* This value represents the speed of the "unlock" command.
	   As unlock is quite fast (it does not do anything), I put
	   4096 ko/s = 4 Mo / s */
	caps->dwUnlockTransferRate = 4096;
	caps->dwPlayCpuOverhead = 0;
	
	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBuffer_QueryInterface(
	LPDIRECTSOUNDBUFFER this,REFIID riid,LPVOID *ppobj
) {
	char	xbuf[50];

	WINE_StringFromCLSID(riid,xbuf);
	TRACE(dsound,"(%p,%s,%p)\n",this,xbuf,ppobj);

	if (!memcmp(&IID_IDirectSoundNotify,riid,sizeof(*riid))) {
		IDirectSoundNotify	*dsn;

		dsn = (LPDIRECTSOUNDNOTIFY)HeapAlloc(GetProcessHeap(),0,sizeof(*dsn));
		dsn->ref = 1;
		dsn->dsb = this;
		this->lpvtbl->fnAddRef(this);
		dsn->lpvtbl = &dsnvt;
		*ppobj = (LPVOID)dsn;
		return 0;
	}

	if (!memcmp(&IID_IDirectSound3DBuffer,riid,sizeof(*riid))) {
		*ppobj = this->ds3db;
		if (*ppobj)
			return DS_OK;
	}

	return E_FAIL;
}

static struct tagLPDIRECTSOUNDBUFFER_VTABLE dsbvt = {
	IDirectSoundBuffer_QueryInterface,
	IDirectSoundBuffer_AddRef,
	IDirectSoundBuffer_Release,
	IDirectSoundBuffer_GetCaps,
	IDirectSoundBuffer_GetCurrentPosition,
	IDirectSoundBuffer_GetFormat,
	IDirectSoundBuffer_GetVolume,
	IDirectSoundBuffer_GetPan,
        IDirectSoundBuffer_GetFrequency,
	IDirectSoundBuffer_GetStatus,
	IDirectSoundBuffer_Initialize,
	IDirectSoundBuffer_Lock,
	IDirectSoundBuffer_Play,
	IDirectSoundBuffer_SetCurrentPosition,
	IDirectSoundBuffer_SetFormat,
	IDirectSoundBuffer_SetVolume,
	IDirectSoundBuffer_SetPan,
	IDirectSoundBuffer_SetFrequency,
	IDirectSoundBuffer_Stop,
	IDirectSoundBuffer_Unlock
};

/*******************************************************************************
 *		IDirectSound
 */

static HRESULT WINAPI IDirectSound_SetCooperativeLevel(
	LPDIRECTSOUND this,HWND32 hwnd,DWORD level
) {
	FIXME(dsound,"(%p,%08lx,%ld):stub\n",this,(DWORD)hwnd,level);
	return 0;
}

static HRESULT WINAPI IDirectSound_CreateSoundBuffer(
	LPDIRECTSOUND this,LPDSBUFFERDESC dsbd,LPLPDIRECTSOUNDBUFFER ppdsb,LPUNKNOWN lpunk
) {
	LPWAVEFORMATEX	wfex;

	TRACE(dsound,"(%p,%p,%p,%p)\n",this,dsbd,ppdsb,lpunk);
	
	if ((this == NULL) || (dsbd == NULL) || (ppdsb == NULL))
		return DSERR_INVALIDPARAM;
	
	if (TRACE_ON(dsound)) {
		TRACE(dsound,"(size=%ld)\n",dsbd->dwSize);
		TRACE(dsound,"(flags=0x%08lx\n",dsbd->dwFlags);
		_dump_DSBCAPS(dsbd->dwFlags);
		TRACE(dsound,"(bufferbytes=%ld)\n",dsbd->dwBufferBytes);
		TRACE(dsound,"(lpwfxFormat=%p)\n",dsbd->lpwfxFormat);
	}

	wfex = dsbd->lpwfxFormat;

	if (wfex)
		TRACE(dsound,"(formattag=0x%04x,chans=%d,samplerate=%ld"
		   "bytespersec=%ld,blockalign=%d,bitspersamp=%d,cbSize=%d)\n",
		   wfex->wFormatTag, wfex->nChannels, wfex->nSamplesPerSec,
		   wfex->nAvgBytesPerSec, wfex->nBlockAlign, 
		   wfex->wBitsPerSample, wfex->cbSize);

	if (dsbd->dwFlags & DSBCAPS_PRIMARYBUFFER) {
		if (primarybuf) {
			primarybuf->lpvtbl->fnAddRef(primarybuf);
			*ppdsb = primarybuf;
			return DS_OK;
		} /* Else create primarybuf */
	}

	*ppdsb = (LPDIRECTSOUNDBUFFER)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectSoundBuffer));
	if (*ppdsb == NULL)
		return DSERR_OUTOFMEMORY;
	(*ppdsb)->ref = 1;

	TRACE(dsound, "Created buffer at %p\n", *ppdsb);

	if (dsbd->dwFlags & DSBCAPS_PRIMARYBUFFER) {
		(*ppdsb)->buflen = dsound->wfx.nAvgBytesPerSec;
		(*ppdsb)->freq = dsound->wfx.nSamplesPerSec;
	} else {
		(*ppdsb)->buflen = dsbd->dwBufferBytes;
		(*ppdsb)->freq = dsbd->lpwfxFormat->nSamplesPerSec;
	}
	(*ppdsb)->buffer = (LPBYTE)HeapAlloc(GetProcessHeap(),0,(*ppdsb)->buflen);
	if ((*ppdsb)->buffer == NULL) {
		HeapFree(GetProcessHeap(),0,(*ppdsb));
		*ppdsb = NULL;
		return DSERR_OUTOFMEMORY;
	}
	/* It's not necessary to initialize values to zero since */
	/* we allocated this structure with HEAP_ZERO_MEMORY... */
	(*ppdsb)->playpos = 0;
	(*ppdsb)->writepos = 0;
	(*ppdsb)->parent = NULL;
	(*ppdsb)->lpvtbl = &dsbvt;
	(*ppdsb)->dsound = this;
	(*ppdsb)->playing = 0;
	(*ppdsb)->lVolAdjust = (1 << 15);
	(*ppdsb)->rVolAdjust = (1 << 15);

	if (!(dsbd->dwFlags & DSBCAPS_PRIMARYBUFFER)) {
		(*ppdsb)->freqAdjust = ((*ppdsb)->freq << DSOUND_FREQSHIFT) /
			primarybuf->wfx.nSamplesPerSec;
		(*ppdsb)->nAvgBytesPerSec = (*ppdsb)->freq *
			dsbd->lpwfxFormat->nBlockAlign;
	}

	memcpy(&((*ppdsb)->dsbd),dsbd,sizeof(*dsbd));

	/* register buffer */
	if (!(dsbd->dwFlags & DSBCAPS_PRIMARYBUFFER)) {
		this->buffers = (LPDIRECTSOUNDBUFFER*)HeapReAlloc(GetProcessHeap(),0,this->buffers,sizeof(LPDIRECTSOUNDBUFFER)*(this->nrofbuffers+1));
		this->buffers[this->nrofbuffers] = *ppdsb;
		this->nrofbuffers++;
	}
	this->lpvtbl->fnAddRef(this);

	if (dsbd->lpwfxFormat)
		memcpy(&((*ppdsb)->wfx), dsbd->lpwfxFormat, sizeof((*ppdsb)->wfx));

	InitializeCriticalSection(&((*ppdsb)->lock));
	
#if USE_DSOUND3D
	if (dsbd->dwFlags & DSBCAPS_CTRL3D) {
		IDirectSound3DBuffer	*ds3db;

		ds3db = (LPDIRECTSOUND3DBUFFER)HeapAlloc(GetProcessHeap(),
			0,sizeof(*ds3db));
		ds3db->ref = 1;
		ds3db->dsb = (*ppdsb);
		ds3db->lpvtbl = &ds3dbvt;
		(*ppdsb)->ds3db = ds3db;
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
		ds3db->buflen = ((*ppdsb)->buflen * primarybuf->wfx.nBlockAlign) /
			(*ppdsb)->wfx.nBlockAlign;
		ds3db->buffer = HeapAlloc(GetProcessHeap(), 0, ds3db->buflen);
		if (ds3db->buffer == NULL) {
			ds3db->buflen = 0;
			ds3db->ds3db.dwMode = DS3DMODE_DISABLE;
		}
	}
#endif
	return DS_OK;
}

static HRESULT WINAPI IDirectSound_DuplicateSoundBuffer(
	LPDIRECTSOUND this,LPDIRECTSOUNDBUFFER pdsb,LPLPDIRECTSOUNDBUFFER ppdsb
) {
	TRACE(dsound,"(%p,%p,%p)\n",this,pdsb,ppdsb);

	*ppdsb = (LPDIRECTSOUNDBUFFER)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectSoundBuffer));

	pdsb->lpvtbl->fnAddRef(pdsb);
	memcpy(*ppdsb, pdsb, sizeof(IDirectSoundBuffer));
	(*ppdsb)->ref = 1;
	(*ppdsb)->playpos = 0;
	(*ppdsb)->writepos = 0;
	(*ppdsb)->dsound = this;
	(*ppdsb)->parent = pdsb;
	memcpy(&((*ppdsb)->wfx), &(pdsb->wfx), sizeof((*ppdsb)->wfx));
	/* register buffer */
	this->buffers = (LPDIRECTSOUNDBUFFER*)HeapReAlloc(GetProcessHeap(),0,this->buffers,sizeof(LPDIRECTSOUNDBUFFER)*(this->nrofbuffers+1));
	this->buffers[this->nrofbuffers] = *ppdsb;
	this->nrofbuffers++;
	this->lpvtbl->fnAddRef(this);
	return 0;
}


static HRESULT WINAPI IDirectSound_GetCaps(LPDIRECTSOUND this,LPDSCAPS caps) {
	TRACE(dsound,"(%p,%p)\n",this,caps);
	TRACE(dsound,"(flags=0x%08lx)\n",caps->dwFlags);

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

	return 0;
}

static ULONG WINAPI IDirectSound_AddRef(LPDIRECTSOUND this) {
	return ++(this->ref);
}

static ULONG WINAPI IDirectSound_Release(LPDIRECTSOUND this) {
	TRACE(dsound,"(%p), ref was %ld\n",this,this->ref);
	if (!--(this->ref)) {
		DSOUND_CloseAudio();
		while(IDirectSoundBuffer_Release(primarybuf)); /* Deallocate */
		FIXME(dsound, "need to release all buffers!\n");
		HeapFree(GetProcessHeap(),0,this);
		dsound = NULL;
		return 0;
	}
	return this->ref;
}

static HRESULT WINAPI IDirectSound_SetSpeakerConfig(
	LPDIRECTSOUND this,DWORD config
) {
	FIXME(dsound,"(%p,0x%08lx):stub\n",this,config);
	return 0;
}

static HRESULT WINAPI IDirectSound_QueryInterface(
	LPDIRECTSOUND this,REFIID riid,LPVOID *ppobj
) {
	char xbuf[50];

	if (!memcmp(&IID_IDirectSound3DListener,riid,sizeof(*riid))) {

		if (this->listener) {
			*ppobj = this->listener;
			return DS_OK;
		}
		this->listener = (LPDIRECTSOUND3DLISTENER)HeapAlloc(
			GetProcessHeap(), 0, sizeof(*(this->listener)));
		this->listener->ref = 1;
		this->listener->lpvtbl = &ds3dlvt;
		this->lpvtbl->fnAddRef(this);
		this->listener->ds3dl.dwSize = sizeof(DS3DLISTENER);
		this->listener->ds3dl.vPosition.x.x = 0.0;
		this->listener->ds3dl.vPosition.y.y = 0.0;
		this->listener->ds3dl.vPosition.z.z = 0.0;
		this->listener->ds3dl.vVelocity.x.x = 0.0;
		this->listener->ds3dl.vVelocity.y.y = 0.0;
		this->listener->ds3dl.vVelocity.z.z = 0.0;
		this->listener->ds3dl.vOrientFront.x.x = 0.0;
		this->listener->ds3dl.vOrientFront.y.y = 0.0;
		this->listener->ds3dl.vOrientFront.z.z = 1.0;
		this->listener->ds3dl.vOrientTop.x.x = 0.0;
		this->listener->ds3dl.vOrientTop.y.y = 1.0;
		this->listener->ds3dl.vOrientTop.z.z = 0.0;
		this->listener->ds3dl.flDistanceFactor = DS3D_DEFAULTDISTANCEFACTOR;
		this->listener->ds3dl.flRolloffFactor = DS3D_DEFAULTROLLOFFFACTOR;
		this->listener->ds3dl.flDopplerFactor = DS3D_DEFAULTDOPPLERFACTOR;
		*ppobj = (LPVOID)this->listener;
		return DS_OK;
	}

	WINE_StringFromCLSID(riid,xbuf);
	TRACE(dsound,"(%p,%s,%p)\n",this,xbuf,ppobj);
	return E_FAIL;
}

static HRESULT WINAPI IDirectSound_Compact(
	LPDIRECTSOUND this)
{
	TRACE(dsound, "(%p)\n", this);
	return DS_OK;
}

static HRESULT WINAPI IDirectSound_GetSpeakerConfig(
	LPDIRECTSOUND this,
	LPDWORD lpdwSpeakerConfig)
{
	TRACE(dsound, "(%p, %p)\n", this, lpdwSpeakerConfig);
	*lpdwSpeakerConfig = DSSPEAKER_STEREO | (DSSPEAKER_GEOMETRY_NARROW << 16);
	return DS_OK;
}

static HRESULT WINAPI IDirectSound_Initialize(
	LPDIRECTSOUND this,
	LPGUID lpGuid)
{
	TRACE(dsound, "(%p, %p)\n", this, lpGuid);
	return DS_OK;
}

static struct tagLPDIRECTSOUND_VTABLE dsvt = {
	IDirectSound_QueryInterface,
	IDirectSound_AddRef,
	IDirectSound_Release,
	IDirectSound_CreateSoundBuffer,
	IDirectSound_GetCaps,
	IDirectSound_DuplicateSoundBuffer,
	IDirectSound_SetCooperativeLevel,
	IDirectSound_Compact,
	IDirectSound_GetSpeakerConfig,
	IDirectSound_SetSpeakerConfig,
	IDirectSound_Initialize
};


/* See http://www.opensound.com/pguide/audio.html for more details */

static int
DSOUND_setformat(LPWAVEFORMATEX wfex) {
	int	xx,channels,speed,format,nformat;

	if (!audioOK) {
		TRACE(dsound, "(%p) deferred\n", wfex);
		return 0;
	}
	switch (wfex->wFormatTag) {
	default:
		WARN(dsound,"unknown WAVE_FORMAT tag %d\n",wfex->wFormatTag);
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
		FIXME(dsound,"SNDCTL_DSP_GETFMTS: format not supported\n"); 
		return -1;
	}
	nformat = format;
	if (-1==ioctl(audiofd,SNDCTL_DSP_SETFMT,&nformat)) {
		perror("ioctl SNDCTL_DSP_SETFMT");
		return -1;
	}
	if (nformat!=format) {/* didn't work */
		FIXME(dsound,"SNDCTL_DSP_GETFMTS: format not set\n"); 
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
	TRACE(dsound,"(freq=%ld,channels=%d,bits=%d)\n",
		wfex->nSamplesPerSec,wfex->nChannels,wfex->wBitsPerSample
	);
	return 0;
}

static void DSOUND_CheckEvent(IDirectSoundBuffer *dsb, int len)
{
	int			i;
	DWORD			offset;
	LPDSBPOSITIONNOTIFY	event;

	if (dsb->nrofnotifies == 0)
		return;

	TRACE(dsound,"(%p) buflen = %ld, playpos = %ld, len = %d\n",
		dsb, dsb->buflen, dsb->playpos, len);
	for (i = 0; i < dsb->nrofnotifies ; i++) {
		event = dsb->notifies + i;
		offset = event->dwOffset;
		TRACE(dsound, "checking %d, position %ld, event = %d\n",
			i, offset, event->hEventNotify);
		/* DSBPN_OFFSETSTOP has to be the last element. So this is */
		/* OK. [Inside DirectX, p274] */
		/*  */
		/* This also means we can't sort the entries by offset, */
		/* because DSBPN_OFFSETSTOP == -1 */
		if (offset == DSBPN_OFFSETSTOP) {
			if (dsb->playing == 0) {
				SetEvent(event->hEventNotify);
				TRACE(dsound,"signalled event %d (%d)\n", event->hEventNotify, i);
				return;
			} else
				return;
		}
		if ((dsb->playpos + len) >= dsb->buflen) {
			if ((offset < ((dsb->playpos + len) % dsb->buflen)) ||
			    (offset >= dsb->playpos)) {
				TRACE(dsound,"signalled event %d (%d)\n", event->hEventNotify, i);
				SetEvent(event->hEventNotify);
			}
		} else {
			if ((offset >= dsb->playpos) && (offset < (dsb->playpos + len))) {
				TRACE(dsound,"signalled event %d (%d)\n", event->hEventNotify, i);
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
static inline void get_fields(const IDirectSoundBuffer *dsb, BYTE *buf, INT32 *fl, INT32 *fr)
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

	FIXME(dsound, "get_fields found an unsupported configuration\n");
	return;
}

static inline void set_fields(BYTE *buf, INT32 fl, INT32 fr)
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
	FIXME(dsound, "set_fields found an unsupported configuration\n");
	return;
}

/* Now with PerfectPitch (tm) technology */
static INT32 DSOUND_MixerNorm(IDirectSoundBuffer *dsb, BYTE *buf, INT32 len)
{
	INT32	i, size, ipos, ilen, fieldL, fieldR;
	BYTE	*ibp, *obp;
	INT32	iAdvance = dsb->wfx.nBlockAlign;
	INT32	oAdvance = primarybuf->wfx.nBlockAlign;

	ibp = dsb->buffer + dsb->playpos;
	obp = buf;

	TRACE(dsound, "(%p, %p, %p), playpos=%8.8lx\n", dsb, ibp, obp, dsb->playpos);
	/* Check for the best case */
	if ((dsb->freq == primarybuf->wfx.nSamplesPerSec) &&
	    (dsb->wfx.wBitsPerSample == primarybuf->wfx.wBitsPerSample) &&
	    (dsb->wfx.nChannels == primarybuf->wfx.nChannels)) {
		TRACE(dsound, "(%p) Best case\n", dsb);
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
		TRACE(dsound, "(%p) Same sample rate %ld = primary %ld\n", dsb,
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

	TRACE(dsound, "(%p) Adjusting frequency: %ld -> %ld\n",
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

static void DSOUND_MixerVol(IDirectSoundBuffer *dsb, BYTE *buf, INT32 len)
{
	INT32	i, inc = primarybuf->wfx.wBitsPerSample >> 3;
	BYTE	*bpc = buf;
	INT16	*bps = (INT16 *) buf;
	
	TRACE(dsound, "(%p) left = %lx, right = %lx\n", dsb,
		dsb->lVolAdjust, dsb->rVolAdjust);
	if ((!(dsb->dsbd.dwFlags & DSBCAPS_CTRLPAN) || (dsb->pan == 0)) &&
	    (!(dsb->dsbd.dwFlags & DSBCAPS_CTRLVOLUME) || (dsb->volume == 0)) &&
	    !(dsb->dsbd.dwFlags & DSBCAPS_CTRL3D))
		return;		/* Nothing to do */

	/* If we end up with some bozo coder using panning or 3D sound */
	/* with a mono primary buffer, it could sound very weird using */
	/* this method. Oh well, tough patooties. */

	for (i = 0; i < len; i += inc) {
		INT32	val;

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
			FIXME(dsound, "MixerVol had a nasty error\n");
		}
	}		
}

#ifdef USE_DSOUND3D
static void DSOUND_Mixer3D(IDirectSoundBuffer *dsb, BYTE *buf, INT32 len)
{
	BYTE	*ibp, *obp;
	DWORD	buflen, playpos;

	buflen = dsb->ds3db->buflen;
	playpos = (dsb->playpos * primarybuf->wfx.nBlockAlign) / dsb->wfx.nBlockAlign;
	ibp = dsb->ds3db->buffer + playpos;
	obp = buf;

	if (playpos > buflen) {
		FIXME(dsound, "Major breakage");
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

static DWORD DSOUND_MixInBuffer(IDirectSoundBuffer *dsb)
{
	INT32	i, len, ilen, temp, field;
	INT32	advance = primarybuf->wfx.wBitsPerSample >> 3;
	BYTE	*buf, *ibuf, *obuf;
	INT16	*ibufs, *obufs;

	len = DSOUND_FRAGLEN;			/* The most we will use */
	if (!(dsb->playflags & DSBPLAY_LOOPING)) {
		temp = MulDiv32(primarybuf->wfx.nAvgBytesPerSec, dsb->buflen,
			dsb->nAvgBytesPerSec) -
		       MulDiv32(primarybuf->wfx.nAvgBytesPerSec, dsb->playpos,
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
	TRACE(dsound, "allocating buffer (size = %d)\n", len);
	if ((buf = ibuf = (BYTE *) malloc(len)) == NULL)
		return 0;

	TRACE(dsound, "MixInBuffer (%p) len = %d\n", dsb, len);

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
	free(buf);

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
	INT32			i, len, maxlen = 0;
	IDirectSoundBuffer	*dsb;

	for (i = dsound->nrofbuffers - 1; i >= 0; i--) {
		dsb = dsound->buffers[i];

		if (!dsb || !(dsb->lpvtbl))
			continue;
		dsb->lpvtbl->fnAddRef(dsb);
 		if (dsb->buflen && dsb->playing) {
			EnterCriticalSection(&(dsb->lock));
			len = DSOUND_MixInBuffer(dsb);
			maxlen = len > maxlen ? len : maxlen;
			LeaveCriticalSection(&(dsb->lock));
		}
		dsb->lpvtbl->fnRelease(dsb);
	}
	
	return maxlen;
}

static int DSOUND_OpenAudio(void)
{
	int	audioFragment;

	if (primarybuf == NULL)
		return DSERR_OUTOFMEMORY;

	while (audiofd != -1)
		sleep(5);
	audiofd = open("/dev/audio",O_WRONLY);
	if (audiofd==-1) {
		/* Don't worry if sound is busy at the moment */
		if (errno != EBUSY)
			perror("open /dev/audio");
		return audiofd; /* -1 */
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
	if (audiofd != -1)
		close(audiofd);
	primarybuf->playpos = 0;
	primarybuf->writepos = DSOUND_FRAGLEN;
	memset(primarybuf->buffer, neutral, primarybuf->buflen);
	audiofd = -1;
	TRACE(dsound, "Audio stopped\n");
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

	TRACE(dsound,"dsound is at pid %d\n",getpid());
	while (1) {
		if (!dsound) {
			WARN(dsound,"DSOUND thread giving up.\n");
			ExitThread(0);
		}
		if (getppid()==1) {
			WARN(dsound,"DSOUND father died? Giving up.\n");
			ExitThread(0);
		}
		/* RACE: dsound could be deleted */
		dsound->lpvtbl->fnAddRef(dsound);
		if (primarybuf == NULL) {
			/* Should never happen */
			WARN(dsound, "Lost the primary buffer!\n");
			dsound->lpvtbl->fnRelease(dsound);
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
		dsound->lpvtbl->fnRelease(dsound);
	}
	ExitThread(0);
}

#endif /* HAVE_OSS */

HRESULT WINAPI DirectSoundCreate(REFGUID lpGUID,LPDIRECTSOUND *ppDS,IUnknown *pUnkOuter )
{
	if (lpGUID)
		TRACE(dsound,"(%p,%p,%p)\n",lpGUID,ppDS,pUnkOuter);
	else
		TRACE(dsound,"DirectSoundCreate (%p)\n", ppDS);

#ifdef HAVE_OSS

	if (ppDS == NULL)
		return DSERR_INVALIDPARAM;

	if (primarybuf) {
		dsound->lpvtbl->fnAddRef(dsound);
		*ppDS = dsound;
		return DS_OK;
	}

	/* Check that we actually have audio capabilities */
	/* If we do, whether it's busy or not, we continue */
	/* otherwise we return with DSERR_NODRIVER */

	audiofd = open("/dev/audio",O_WRONLY);
	if (audiofd == -1) {
		if (errno == ENODEV) {
			MSG("No sound hardware found.\n");
			return DSERR_NODRIVER;
		} else if (errno == EBUSY) {
			MSG("Sound device busy, will keep trying.\n");
		} else {
			MSG("Unexpected error while checking for sound support.\n");
			return DSERR_GENERIC;
		}
	} else {
		close(audiofd);
		audiofd = -1;
	}

	*ppDS = (LPDIRECTSOUND)HeapAlloc(GetProcessHeap(),0,sizeof(IDirectSound));
	if (*ppDS == NULL)
		return DSERR_OUTOFMEMORY;

	(*ppDS)->ref		= 1;
	(*ppDS)->lpvtbl		= &dsvt;
	(*ppDS)->buffers	= NULL;
	(*ppDS)->nrofbuffers	= 0;

	(*ppDS)->wfx.wFormatTag		= 1;
	(*ppDS)->wfx.nChannels		= 2;
	(*ppDS)->wfx.nSamplesPerSec	= 22050;
	(*ppDS)->wfx.nAvgBytesPerSec	= 44100;
	(*ppDS)->wfx.nBlockAlign	= 2;
	(*ppDS)->wfx.wBitsPerSample	= 8;

	if (!dsound) {
		HANDLE32	hnd;
		DWORD		xid;

		dsound = (*ppDS);
		if (primarybuf == NULL) {
			DSBUFFERDESC	dsbd;
			HRESULT		hr;

			dsbd.dwSize = sizeof(DSBUFFERDESC);
			dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
			dsbd.dwBufferBytes = 0;
			dsbd.lpwfxFormat = &(dsound->wfx);
			hr = IDirectSound_CreateSoundBuffer(*ppDS, &dsbd, &primarybuf, NULL);
			if (hr != DS_OK)
				return hr;
			dsound->primary = primarybuf;
		}
		memset(primarybuf->buffer, 128, primarybuf->buflen);
		hnd = CreateThread(NULL,0,DSOUND_thread,0,0,&xid);
	}
	return DS_OK;
#else
	MessageBox32A(0,"DirectSound needs the Open Sound System Driver, which has not been found by ./configure.","WINE DirectSound",MB_OK|MB_ICONSTOP);
	return DSERR_NODRIVER;
#endif
}

/*******************************************************************************
 * DirectSound ClassFactory
 */
typedef struct
{
    /* IUnknown fields */
    ICOM_VTABLE(IClassFactory)* lpvtbl;
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
	FIXME(dsound,"(%p)->(%s,%p),stub!\n",This,buf,ppobj);
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
	TRACE(dsound,"(%p)->(%p,%s,%p)\n",This,pOuter,buf,ppobj);
	if (!memcmp(riid,&IID_IDirectSound,sizeof(IID_IDirectSound))) {
		/* FIXME: reuse already created dsound if present? */
		return DirectSoundCreate(riid,(LPDIRECTSOUND*)ppobj,pOuter);
	}
	return E_NOINTERFACE;
}

static HRESULT WINAPI DSCF_LockServer(LPCLASSFACTORY iface,BOOL32 dolock) {
	ICOM_THIS(IClassFactoryImpl,iface);
	FIXME(dsound,"(%p)->(%d),stub!\n",This,dolock);
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
    TRACE(dsound, "(%p,%p,%p)\n", xbuf, buf, ppv);
    if (!memcmp(riid,&IID_IClassFactory,sizeof(IID_IClassFactory))) {
    	*ppv = (LPVOID)&DSOUND_CF;
	IClassFactory_AddRef((IClassFactory*)*ppv);
    return S_OK;
    }
    FIXME(dsound, "(%p,%p,%p): no interface found.\n", xbuf, buf, ppv);
    return E_NOINTERFACE;
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
    FIXME(dsound, "(void): stub\n");
    return S_FALSE;
}
