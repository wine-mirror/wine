/*  			DirectSound
 * 
 * Copyright 1998 Marcus Meissner
 */
/*
 * Note: This file requires multithread ability. It is not possible to
 * implement the stuff in a single thread anyway. And most DirectX apps
 * require threading themselves.
 *
 * FIXME: This file is full of race conditions and unlocked variable access
 * from two threads. But we usually don't need to bother.
 *
 * Tested with a Soundblaster clone and a Gravis UltraSound Classic.
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
 */

#include "config.h"
#include <assert.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "windows.h"
#include "winerror.h"
#include "interfaces.h"
#include "mmsystem.h"
#include "dsound.h"
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

static int audiofd = -1;
static LPDIRECTSOUND	dsound = NULL;

static short playbuf[2048];

#endif

HRESULT WINAPI DirectSoundEnumerate32A(LPDSENUMCALLBACK32A enumcb,LPVOID context) {
#ifdef HAVE_OSS
	enumcb(NULL,"WINE DirectSound using Open Sound System","sound",context);
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

static int _sort_notifies(const void *a,const void *b) {
	LPDSBPOSITIONNOTIFY	na = (LPDSBPOSITIONNOTIFY)a;
	LPDSBPOSITIONNOTIFY	nb = (LPDSBPOSITIONNOTIFY)b;

	return na->dwOffset-nb->dwOffset;
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
	qsort(this->dsb->notifies,this->dsb->nrofnotifies,sizeof(DSBPOSITIONNOTIFY),_sort_notifies);
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
static HRESULT WINAPI IDirectSoundBuffer_SetFormat(
	LPDIRECTSOUNDBUFFER this,LPWAVEFORMATEX wfex
) {

	memcpy(&(this->wfx),wfex,sizeof(this->wfx));
	TRACE(dsound,"(%p,%p)\n", this,wfex);
	TRACE(dsound,"(formattag=0x%04x,chans=%d,samplerate=%ld"
		   "bytespersec=%ld,blockalign=%d,bitspersamp=%d,cbSize=%d)\n",
		   wfex->wFormatTag, wfex->nChannels, wfex->nSamplesPerSec,
		   wfex->nAvgBytesPerSec, wfex->nBlockAlign, 
		   wfex->wBitsPerSample, wfex->cbSize);

	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_SetVolume(
	LPDIRECTSOUNDBUFFER this,LONG vol
) {
	TRACE(dsound,"(%p,%ld)\n",this,vol);
	this->volume = vol;
	this->volfac = ((double)vol+10000.0)/10000.0;
	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_GetVolume(
	LPDIRECTSOUNDBUFFER this,LPLONG vol
) {
	TRACE(dsound,"(%p,%p)\n",this,vol);
	*vol = this->volume;
	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_SetFrequency(
	LPDIRECTSOUNDBUFFER this,DWORD freq
) {
	TRACE(dsound,"(%p,%ld)\n",this,freq);
	this->wfx.nSamplesPerSec = freq;
	this->wfx.nAvgBytesPerSec = freq*this->wfx.nChannels*(this->wfx.wBitsPerSample/8);
	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_Play(
	LPDIRECTSOUNDBUFFER this,DWORD reserved1,DWORD reserved2,DWORD flags
) {
	TRACE(dsound,"(%p,%08lx,%08lx,%08lx)\n",
		this,reserved1,reserved2,flags
	);
	this->playpos = 0;
	this->playflags = flags;
	this->playing = 1;
	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_Stop(LPDIRECTSOUNDBUFFER this) {
	TRACE(dsound,"(%p)\n",this);
	this->playing = 0;
	this->writepos = 0; /* hmm */
	return 0;
}

static DWORD WINAPI IDirectSoundBuffer_AddRef(LPDIRECTSOUNDBUFFER this) {
	return ++(this->ref);
}
static DWORD WINAPI IDirectSoundBuffer_Release(LPDIRECTSOUNDBUFFER this) {
	int	i;

	if (--this->ref)
		return this->ref;
	for (i=0;i<this->dsound->nrofbuffers;i++)
		if (this->dsound->buffers[i] == this)
			break;
	if (i < this->dsound->nrofbuffers) {
		memcpy(
			this->dsound->buffers+i,
			this->dsound->buffers+i+1,
			sizeof(LPDIRECTSOUNDBUFFER)*(this->dsound->nrofbuffers-i-1)
		);
		this->dsound->buffers = HeapReAlloc(GetProcessHeap(),0,this->dsound->buffers,sizeof(LPDIRECTSOUNDBUFFER)*this->dsound->nrofbuffers);
		this->dsound->nrofbuffers--;
		this->dsound->lpvtbl->fnRelease(this->dsound);
	}
	HeapFree(GetProcessHeap(),0,this);
	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_GetCurrentPosition(
	LPDIRECTSOUNDBUFFER this,LPDWORD playpos,LPDWORD writepos
) {
	TRACE(dsound,"(%p,%p,%p)\n",this,playpos,writepos);
	if (playpos) *playpos = this->playpos;
	if (writepos) *writepos = this->writepos;
	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_GetStatus(
	LPDIRECTSOUNDBUFFER this,LPDWORD status
) {
	TRACE(dsound,"(%p,%p)\n",this,status);
	*status = 0;
	if (this->playing)
		*status |= DSBSTATUS_PLAYING;
	if (this->playflags & DSBPLAY_LOOPING)
		*status |= DSBSTATUS_LOOPING;
	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_GetFormat(
	LPDIRECTSOUNDBUFFER this,LPWAVEFORMATEX lpwf,DWORD wfsize,LPDWORD wfwritten
) {
	TRACE(dsound,"(%p,%p,%ld,%p)\n",this,lpwf,wfsize,wfwritten);
	if (wfsize>sizeof(this->wfx)) wfsize = sizeof(this->wfx);
	memcpy(lpwf,&(this->wfx),wfsize);
	if (wfwritten) *wfwritten = wfsize;
	return 0;
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
		writecursor = this->writepos;
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
	this->writepos=(writecursor+writebytes)%this->buflen;
	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_SetCurrentPosition(
	LPDIRECTSOUNDBUFFER this,DWORD newpos
) {
	TRACE(dsound,"(%p,%ld)\n",this,newpos);
	this->playpos = newpos;
	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_SetPan(
	LPDIRECTSOUNDBUFFER this,LONG newpan
) {
	TRACE(dsound,"(%p,%ld)\n",this,newpan);
	this->pan = newpan;
	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_GetPan(
	LPDIRECTSOUNDBUFFER this,LPLONG pan
) {
	TRACE(dsound,"(%p,%p)\n",this,pan);
	*pan = this->pan;
	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_Unlock(
	LPDIRECTSOUNDBUFFER this,LPVOID p1,DWORD x1,LPVOID p2,DWORD x2
) {
	FIXME(dsound,"(%p,%p,%ld,%p,%ld):stub\n", this,p1,x1,p2,x2);
	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_GetFrequency(
	LPDIRECTSOUNDBUFFER this,LPDWORD freq
) {
	TRACE(dsound,"(%p,%p)\n",this,freq);
	*freq = this->wfx.nSamplesPerSec;
	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_Initialize(
	LPDIRECTSOUNDBUFFER this,LPDIRECTSOUND dsound,LPDSBUFFERDESC dbsd
) {
	FIXME(dsound,"(%p,%p,%p):stub\n",this,dsound,dbsd);
	return DSERR_ALREADYINITIALIZED;
}

static HRESULT WINAPI IDirectSoundBuffer_GetCaps(
	LPDIRECTSOUNDBUFFER this,LPDSBCAPS caps
) {
	caps->dwSize = sizeof(*caps);
	caps->dwFlags = DSBCAPS_PRIMARYBUFFER|DSBCAPS_STATIC|DSBCAPS_CTRLALL|DSBCAPS_LOCSOFTWARE;
	caps->dwBufferBytes = 0;
	caps->dwUnlockTransferRate = 0;
	caps->dwPlayCpuOverhead = 0;
	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBuffer_QueryInterface(
	LPDIRECTSOUNDBUFFER this,REFIID riid,LPVOID *ppobj
) {
	char	xbuf[50];

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
	WINE_StringFromCLSID(riid,xbuf);
	TRACE(dsound,"(%p,%s,%p)\n",this,xbuf,ppobj);
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
	if (TRACE_ON(dsound)) {
		TRACE(dsound,"(%p,%p,%p,%p)\n",this,dsbd,ppdsb,lpunk);
		TRACE(dsound,"(size=%ld)\n",dsbd->dwSize);
		TRACE(dsound,"(flags=0x%08lx\n",dsbd->dwFlags);
		_dump_DSBCAPS(dsbd->dwFlags);
		TRACE(dsound,"(bufferbytes=%ld)\n",dsbd->dwBufferBytes);
		TRACE(dsound,"(lpwfxFormat=%p)\n",dsbd->lpwfxFormat);
	}
	*ppdsb = (LPDIRECTSOUNDBUFFER)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectSoundBuffer));
	(*ppdsb)->ref =1;
	(*ppdsb)->buffer = (LPBYTE)HeapAlloc(GetProcessHeap(),0,dsbd->dwBufferBytes);
	(*ppdsb)->buflen = dsbd->dwBufferBytes;
	(*ppdsb)->playpos = 0;
	(*ppdsb)->writepos = 0;
	(*ppdsb)->lpvtbl = &dsbvt;
	(*ppdsb)->dsound = this;
	(*ppdsb)->playing = 0;
	(*ppdsb)->volfac = 1.0;
	memcpy(&((*ppdsb)->dsbd),dsbd,sizeof(*dsbd));

	/* register buffer */
	this->buffers = (LPDIRECTSOUNDBUFFER*)HeapReAlloc(GetProcessHeap(),0,this->buffers,sizeof(LPDIRECTSOUNDBUFFER)*(this->nrofbuffers+1));
	this->buffers[this->nrofbuffers] = *ppdsb;
	this->nrofbuffers++;
	this->lpvtbl->fnAddRef(this);

	if (dsbd->lpwfxFormat) dsbvt.fnSetFormat(*ppdsb,dsbd->lpwfxFormat);
	return 0;
}

static HRESULT WINAPI IDirectSound_DuplicateSoundBuffer(
	LPDIRECTSOUND this,LPDIRECTSOUNDBUFFER pdsb,LPLPDIRECTSOUNDBUFFER ppdsb
) {
	TRACE(dsound,"(%p,%p,%p)\n",this,pdsb,ppdsb);

	*ppdsb = (LPDIRECTSOUNDBUFFER)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectSoundBuffer));
	(*ppdsb)->ref =1;
	(*ppdsb)->buffer = (LPBYTE)HeapAlloc(GetProcessHeap(),0,pdsb->buflen);
	memcpy((*ppdsb)->buffer,pdsb->buffer,pdsb->buflen);
	(*ppdsb)->buflen = pdsb->buflen;
	(*ppdsb)->playpos = 0;
	(*ppdsb)->writepos = 0;
	(*ppdsb)->lpvtbl = &dsbvt;
	(*ppdsb)->dsound = this;
	dsbvt.fnSetFormat(*ppdsb,&(pdsb->wfx));
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

	caps->dwSize = sizeof(*caps);
	caps->dwFlags = DSCAPS_PRIMARYSTEREO|DSCAPS_PRIMARY16BIT|DSCAPS_EMULDRIVER|DSCAPS_SECONDARYSTEREO|DSCAPS_SECONDARY16BIT;
	/* FIXME: query OSS */
	caps->dwMinSecondarySampleRate = 22050;
	caps->dwMaxSecondarySampleRate = 48000;
	caps->dwPrimaryBuffers = 1;
	/* FIXME: set the rest... hmm */
	return 0;
}

static ULONG WINAPI IDirectSound_AddRef(LPDIRECTSOUND this) {
	return ++(this->ref);
}

static ULONG WINAPI IDirectSound_Release(LPDIRECTSOUND this) {
	if (!--(this->ref)) {
		HeapFree(GetProcessHeap(),0,this);
		dsound = NULL;
		close(audiofd);audiofd = -1;
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

	WINE_StringFromCLSID(riid,xbuf);
	TRACE(dsound,"(%p,%s,%p)\n",this,xbuf,ppobj);
	return E_FAIL;
}

static struct tagLPDIRECTSOUND_VTABLE dsvt = {
	IDirectSound_QueryInterface,
	IDirectSound_AddRef,
	IDirectSound_Release,
	IDirectSound_CreateSoundBuffer,
	IDirectSound_GetCaps,
	IDirectSound_DuplicateSoundBuffer,
	IDirectSound_SetCooperativeLevel,
	(void *)8,
        (void *)9,
        IDirectSound_SetSpeakerConfig,
        (void *)11
};

static int
DSOUND_setformat(LPWAVEFORMATEX wfex) {
	int	xx,channels,speed,format,nformat;


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
		WARN(dsound,"SNDCTL_DSP_GETFMTS: format not supported\n"); 
		return -1;
	}
	nformat = format;
	if (-1==ioctl(audiofd,SNDCTL_DSP_SETFMT,&nformat)) {
		perror("ioctl SNDCTL_DSP_SETFMT");
		return -1;
	}
	if (nformat!=format) {/* didn't work */
		WARN(dsound,"SNDCTL_DSP_GETFMTS: format not set\n"); 
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

static LPDSBPOSITIONNOTIFY
DSOUND_nextevent(IDirectSoundBuffer *dsb) {
	int	i;

	if (dsb->nrofnotifies) {
		for (i=0;i<dsb->nrofnotifies;i++) {
			if (dsb->playpos<dsb->notifies[i].dwOffset)
				break;
		}
		if (i==dsb->nrofnotifies)
			i=0;
		return dsb->notifies+i;
	}
	return NULL;
}

#define CHECK_EVENT							\
	if (nextevent && (dsb->playpos == nextevent->dwOffset)) {	\
		SetEvent(nextevent->hEventNotify);			\
		TRACE(dsound,"signalled event %d\n",nextevent->hEventNotify);\
		nextevent = DSOUND_nextevent(dsb);			\
	}
		

static void 
DSOUND_MixInBuffer(IDirectSoundBuffer *dsb) {
	int	j,buflen = dsb->buflen;
	LPDSBPOSITIONNOTIFY	nextevent;
	int	xdiff = dsb->wfx.nSamplesPerSec-dsound->wfx.nSamplesPerSec;
	double	volfac = dsb->volfac;

	if (xdiff<0) xdiff=-xdiff;
	if (xdiff>1500) {
		WARN(dsound,"mixing in buffer of different frequency (%ld vs %ld), argh!\n",
                        dsb->wfx.nSamplesPerSec,dsound->wfx.nSamplesPerSec);
	}
	nextevent = DSOUND_nextevent(dsb);
/*	TRACE(dsound,"(%d.%d.%d.%d)\n",dsound->wfx.wBitsPerSample,dsb->wfx.wBitsPerSample,dsound->wfx.nChannels,dsb->wfx.nChannels);*/

	if (dsound->wfx.wBitsPerSample == 8) {
		char	*playbuf8 = (char*)playbuf;

		if (dsb->wfx.wBitsPerSample == 8) {
			unsigned char	*xbuf = (unsigned char*)(dsb->buffer);
			if (dsb->wfx.nChannels == 1) {
				for (j=0;j<sizeof(playbuf)/2;j++) {
			
					dsb->playpos=(dsb->playpos+1)%buflen;
					if (!dsb->playpos && !(dsb->playflags&DSBPLAY_LOOPING)) {
						dsb->playing = 0;
						dsb->playpos = buflen;
						return;
					}
					/* FIXME: pan,volume */
					playbuf8[(j<<1)  ]+=xbuf[dsb->playpos]*volfac;
					playbuf8[(j<<1)+1]+=xbuf[dsb->playpos]*volfac;
					CHECK_EVENT
				}
			} else {
				for (j=0;j<sizeof(playbuf);j++) {
					dsb->playpos=(dsb->playpos+1)%buflen;
					if (!dsb->playpos && !(dsb->playflags&DSBPLAY_LOOPING)) {
						dsb->playing = 0;
						dsb->playpos = buflen;
						return;
					}
					/* FIXME: pan,volume */
					playbuf8[j]+=xbuf[dsb->playpos]*volfac;
					CHECK_EVENT
				}
			}
		} else { /* 16 */
			short	*xbuf = (short*)(dsb->buffer);
			if (dsb->wfx.nChannels == 1) {
				for (j=0;j<sizeof(playbuf)/2;j++) {
					dsb->playpos=(dsb->playpos+2)%buflen;
					if (!dsb->playpos && !(dsb->playflags&DSBPLAY_LOOPING)) {
						dsb->playing = 0;
						dsb->playpos = buflen;
						return;
					}
					/* FIXME: pan,volume */
					playbuf8[(j<<1)  ]+=(xbuf[dsb->playpos>>1]>>8)*volfac;
					playbuf8[(j<<1)+1]+=(xbuf[dsb->playpos>>1]>>8)*volfac;
					CHECK_EVENT
				}
			} else {
				for (j=0;j<sizeof(playbuf);j++) {
					dsb->playpos=(dsb->playpos+2)%buflen;
					if (!dsb->playpos && !(dsb->playflags&DSBPLAY_LOOPING)) {
						dsb->playing = 0;
						dsb->playpos = buflen;
						return;
					}
					/* FIXME: pan,volume */
					playbuf8[j]+=(xbuf[dsb->playpos>>1]>>8)*volfac;
					CHECK_EVENT
				}
			}
		}
	} else { /* 16 bit */
		if (dsb->wfx.wBitsPerSample == 8) {
			unsigned char	*xbuf = (unsigned char*)(dsb->buffer);
			if (dsb->wfx.nChannels == 1) {
				for (j=0;j<sizeof(playbuf)/sizeof(playbuf[0])/2;j++) {
					dsb->playpos=(dsb->playpos+1)%buflen;
					if (!dsb->playpos && !(dsb->playflags&DSBPLAY_LOOPING)) {
						dsb->playing = 0;
						dsb->playpos = buflen;
						return;
					}
					/* FIXME: pan,volume */
					/* FIXME: should be *256 */
					playbuf[(j<<1)  ]+=(xbuf[dsb->playpos]<<7)*volfac;
					playbuf[(j<<1)+1]+=(xbuf[dsb->playpos]<<7)*volfac;
					CHECK_EVENT
				}
			} else {
				for (j=0;j<sizeof(playbuf)/sizeof(playbuf[0]);j++) {
					dsb->playpos=(dsb->playpos+1)%buflen;
					if (!dsb->playpos && !(dsb->playflags&DSBPLAY_LOOPING)) {
						dsb->playing = 0;
						dsb->playpos = buflen;
						return;
					}
					/* FIXME: pan,volume */
					/* FIXME: should be *256 */
					playbuf[j]+=(xbuf[dsb->playpos]<<7)*volfac;
					CHECK_EVENT
				}
			}
		} else { /* 16 */
			short	*xbuf = (short*)(dsb->buffer);
			if (dsb->wfx.nChannels == 1) {
				for (j=0;j<sizeof(playbuf)/sizeof(playbuf[0])/2;j++) {
					dsb->playpos=(dsb->playpos+2)%buflen;
					if (!dsb->playpos && !(dsb->playflags&DSBPLAY_LOOPING)) {
						dsb->playing = 0;
						dsb->playpos = buflen;
						return;
					}
					/* FIXME: pan,volume */
					playbuf[(j<<1)  ]+=(xbuf[dsb->playpos>>1]*volfac);
					playbuf[(j<<1)+1]+=(xbuf[dsb->playpos>>1]*volfac);
					CHECK_EVENT
				}
			} else {
				for (j=0;j<sizeof(playbuf)/sizeof(playbuf[0]);j++) {
					dsb->playpos=(dsb->playpos+2)%buflen;
					if (!dsb->playpos && !(dsb->playflags&DSBPLAY_LOOPING)) {
						dsb->playing = 0;
						dsb->playpos = buflen;
						return;
					}
					/* FIXME: pan,volume */
					playbuf[j]+=xbuf[dsb->playpos>>1]/**volfac*/;
					CHECK_EVENT
				}
			}
		}
	}
}

static DWORD
DSOUND_thread(LPVOID arg) {
	int	res,i,curleft,playing,haveprimary = 0;

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
		if (!dsound->nrofbuffers) {
			/* no soundbuffer yet... wait. */
			Sleep(1000);
			continue;
		}
		memset(playbuf,0,sizeof(playbuf));
		playing = 0;
		dsound->lpvtbl->fnAddRef(dsound); 
		haveprimary = 0;
		for (i=dsound->nrofbuffers;i--;) {
			IDirectSoundBuffer	*dsb = dsound->buffers[i];

			dsb->lpvtbl->fnAddRef(dsb);
			if (dsb->playing && dsb->buflen)
				playing++;
			if (dsb->dsbd.dwFlags & DSBCAPS_PRIMARYBUFFER) {
				haveprimary = 1;
				if (memcmp(&dsound->wfx,&(dsb->wfx),sizeof(dsound->wfx))) {
					DSOUND_setformat(&(dsb->wfx));
					memcpy(&dsound->wfx,&(dsb->wfx),sizeof(dsb->wfx));
				}
			}
			dsb->lpvtbl->fnRelease(dsb);
		}
		/* We have just one playbuffer, so use its format */
		if ((playing==1) && !haveprimary) {
			for (i=dsound->nrofbuffers;i--;) {
				IDirectSoundBuffer	*dsb = dsound->buffers[i];

				dsb->lpvtbl->fnAddRef(dsb);
				if (dsb->playing && dsb->buflen) {
					if (memcmp(&dsound->wfx,&(dsb->wfx),sizeof(dsound->wfx))) {
						DSOUND_setformat(&(dsb->wfx));
						memcpy(&dsound->wfx,&(dsb->wfx),sizeof(dsb->wfx));
					}
				}
				dsb->lpvtbl->fnRelease(dsb);
			}
		}
		for (i=dsound->nrofbuffers;i--;) {
			IDirectSoundBuffer	*dsb = dsound->buffers[i];

			dsb->lpvtbl->fnAddRef(dsb);
			if (dsb->buflen && dsb->playing) {
				playing++;
				DSOUND_MixInBuffer(dsb);
			}
			dsb->lpvtbl->fnRelease(dsb);
		}
		dsound->lpvtbl->fnRelease(dsound);

		/*fputc('0'+playing,stderr);*/
		curleft = 0;
		while (curleft < sizeof(playbuf)) {
			res = write(audiofd,(LPBYTE)playbuf+curleft,sizeof(playbuf)-curleft);
			if (res==-1) {
				perror("write audiofd");
				ExitThread(0);
				break;
			}
			curleft+=res;
		}
	}
	ExitThread(0);
}

#endif /* HAVE_OSS */

HRESULT WINAPI DirectSoundCreate(LPGUID lpGUID,LPDIRECTSOUND *ppDS,IUnknown *pUnkOuter ) {
	int xx;
	if (lpGUID)
		TRACE(dsound,"(%p,%p,%p)\n",lpGUID,ppDS,pUnkOuter);
#ifdef HAVE_OSS
	if (audiofd>=0)
		return DSERR_ALLOCATED;
	audiofd = open("/dev/audio",O_WRONLY);
	if (audiofd==-1) {
		perror("open /dev/audio");
		audiofd=0;
		return DSERR_NODRIVER;
	}
	xx=0x0002000c;
	if (-1==ioctl(audiofd,SNDCTL_DSP_SETFRAGMENT,&xx))
		perror("ioctl SETFRAGMENT");
/*
	TRACE(dsound,"SETFRAGMENT. count is now %d, fragsize is %d\n",
		(xx>>16)+1,xx&0xffff
	);
 */

	*ppDS = (LPDIRECTSOUND)HeapAlloc(GetProcessHeap(),0,sizeof(IDirectSound));
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

	DSOUND_setformat(&((*ppDS)->wfx));

	if (!dsound) {
		HANDLE32	hnd;
		DWORD		xid;

		dsound = (*ppDS);
		hnd = CreateThread(NULL,NULL,DSOUND_thread,0,0,&xid);
	}
	return 0;
#else
	MessageBox32A(0,"DirectSound needs the Open Sound System Driver, which has not been found by ./configure.","WINE DirectSound",MB_OK|MB_ICONSTOP);
	return DSERR_NODRIVER;
#endif
}


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
DWORD WINAPI DllGetClassObject( REFCLSID rclsid, REFIID riid, LPVOID *ppv )
{
    FIXME(dsound, "(%p,%p,%p): stub\n", rclsid, riid, ppv);
    return S_OK;
}


/*******************************************************************************
 * DllCanUnloadNow [DSOUND.3]  Determines whether the DLL is in use.
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: S_FALSE
 */
DWORD WINAPI DllCanUnloadNow(void)
{
    FIXME(dsound, "(void): stub\n");
    return S_FALSE;
}

/***************************************************************************
 *   DirectPlayLobbyCreateW (DPLAYX.5)
 *
 */
HRESULT WINAPI DirectPlayLobbyCreateW( LPGUID lpGUID, LPDIRECTPLAYLOBBY *a,
                                       IUnknown *b, LPVOID c, DWORD d )
{
  FIXME(dsound,"lpGUID=%p a=%p b=%p c=%p d=%08lx :stub\n",lpGUID,a,b,c,d);
  return E_FAIL;
}

/***************************************************************************
 *  DirectPlayLobbyCreateA   (DPLAYX.4)
 *
 */
HRESULT WINAPI DirectPlayLobbyCreateA( LPGUID lpGUID, LPDIRECTPLAYLOBBYA *a,
                                       IUnknown *b, LPVOID c, DWORD d )
{
  FIXME(dsound,"lpGUID=%p a=%p b=%p c=%p d=%08lx :stub\n",lpGUID,a,b,c,d);
  return E_FAIL;
}

