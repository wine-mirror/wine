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
 */

#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <sys/signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "windows.h"
#include "winerror.h"
#include "interfaces.h"
#include "mmsystem.h"
#include "dsound.h"

#ifdef HAVE_OSS
#include <sys/ioctl.h>
#include <sys/soundcard.h>
static int audiofd = 0;
static LPDIRECTSOUND	dsound = NULL;
#endif

HRESULT WINAPI DirectSoundEnumerate32A(LPDSENUMCALLBACK32A enumcb,LPVOID context) {
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

static HRESULT WINAPI IDirectSoundBuffer_SetFormat(
	LPDIRECTSOUNDBUFFER this,LPWAVEFORMATEX wfex
) {
	int	xx,channels,speed,format,nformat;

	fprintf(stderr,"IDirectSoundBuffer(%p)->SetFormat(%p),stub!\n",this,wfex);
	memcpy(&(this->wfx),wfex,sizeof(this->wfx));
	fprintf(stderr,"	[formattag=0x%04x,",wfex->wFormatTag);
	fprintf(stderr,"chans=%d,",wfex->nChannels);
	fprintf(stderr,"samplerate=%ld,",wfex->nSamplesPerSec);
	fprintf(stderr,"bytespersec=%ld,",wfex->nAvgBytesPerSec);
	fprintf(stderr,"blockalign=%d,",wfex->nBlockAlign);
	fprintf(stderr,"bitspersamp=%d,",wfex->wBitsPerSample);
	fprintf(stderr,"cbSize=%d]\n",wfex->cbSize);

	switch (wfex->wFormatTag) {
	default:
		fprintf(stderr,"unknown WAVE_FORMAT tag %d\n",wfex->wFormatTag);
		return DSERR_BADFORMAT;
	case WAVE_FORMAT_PCM:
		format = AFMT_S16_LE;
		break;
	}
	if (-1==ioctl(audiofd,SNDCTL_DSP_GETFMTS,&xx)) {
		perror("ioctl SNDCTL_DSP_GETFMTS");
		return DSERR_BADFORMAT;
	}
	if ((xx&format)!=format) {/* format unsupported */
		fprintf(stderr,"SNDCTL_DSP_GETFMTS: format not supported\n"); 
		return DSERR_BADFORMAT;
	}
	nformat = format;
	if (-1==ioctl(audiofd,SNDCTL_DSP_SETFMT,&nformat)) {
		perror("ioctl SNDCTL_DSP_SETFMT");
		return DSERR_BADFORMAT;
	}
	if (nformat!=format) {/* didn't work */
		fprintf(stderr,"SNDCTL_DSP_GETFMTS: format not set\n"); 
		return DSERR_BADFORMAT;
	}

	channels = wfex->nChannels-1;
	if (-1==ioctl(audiofd,SNDCTL_DSP_STEREO,&channels)) {
		perror("ioctl SNDCTL_DSP_STEREO");
		return DSERR_BADFORMAT;
	}
	speed = wfex->nSamplesPerSec;
	if (-1==ioctl(audiofd,SNDCTL_DSP_SPEED,&speed)) {
		perror("ioctl SNDCTL_DSP_SPEED");
		return DSERR_BADFORMAT;
	}
	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_SetVolume(
	LPDIRECTSOUNDBUFFER this,LONG vol
) {
	fprintf(stderr,"IDirectSoundBuffer(%p)->SetVolume(%08lx),stub!\n",this,vol);
	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_GetVolume(
	LPDIRECTSOUNDBUFFER this,LPLONG vol
) {
	fprintf(stderr,"IDirectSoundBuffer(%p)->GetVolume(%p),stub!\n",this,vol);
	*vol = 100;
	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_SetFrequency(
	LPDIRECTSOUNDBUFFER this,DWORD freq
) {
	fprintf(stderr,"IDirectSoundBuffer(%p)->SetFrequency(%08lx),stub!\n",this,freq);
	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_Play(
	LPDIRECTSOUNDBUFFER this,DWORD reserved1,DWORD reserved2,DWORD flags
) {

	fprintf(stderr,"IDirectSoundBuffer(%p)->Play(%08lx,%08lx,%08lx),stub!\n",
		this,reserved1,reserved2,flags
	);
	this->playing = 1;
	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_Stop(LPDIRECTSOUNDBUFFER this) {
	/*fprintf(stderr,"IDirectSoundBuffer(%p)->Stop()\n",this);*/
	this->playing = 0;
	return 0;
}

static DWORD WINAPI IDirectSoundBuffer_AddRef(LPDIRECTSOUNDBUFFER this) {
	return ++(this->ref);
}
static DWORD WINAPI IDirectSoundBuffer_Release(LPDIRECTSOUNDBUFFER this) {
	int	i;

	if (--this->ref)
		return this->ref;
	fprintf(stderr,"	-> IDirectSoundBuffer(%p) freed.\n",this);
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
/*	fprintf(stderr,"IDirectSoundBuffer(%p)->GetCurrentPosition(%p,%p),stub!\n",this,playpos,writepos);*/

	if (playpos) *playpos = this->playpos;
	this->writepos = (this->playpos+512) % this->buflen;
	if (writepos) *writepos = this->writepos; /* hmm */
	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_GetStatus(
	LPDIRECTSOUNDBUFFER this,LPDWORD status
) {
	fprintf(stderr,"IDirectSoundBuffer(%p)->GetStatus(%p),stub!\n",this,status);
	if (this->playing)
		*status = DSBSTATUS_PLAYING;
	*status |= DSBSTATUS_LOOPING; /* FIXME */
	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_GetFormat(
	LPDIRECTSOUNDBUFFER this,LPWAVEFORMATEX lpwf,DWORD wfsize,LPDWORD wfwritten
) {
	fprintf(stderr,"IDirectSoundBuffer(%p)->GetFormat(%p,0x%08lx,%p),stub!\n",this,lpwf,wfsize,wfwritten);
	if (wfsize>sizeof(this->wfx)) wfsize = sizeof(this->wfx);
	memcpy(lpwf,&(this->wfx),wfsize);
	if (wfwritten) *wfwritten = wfsize;
	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_Lock(
	LPDIRECTSOUNDBUFFER this,DWORD writecursor,DWORD writebytes,LPVOID lplpaudioptr1,LPDWORD audiobytes1,LPVOID lplpaudioptr2,LPDWORD audiobytes2,DWORD flags
) {

/*
	fprintf(stderr,"IDirectSoundBuffer(%p)->Lock(%ld,%ld,%p,%p,%p,%p,0x%08lx),stub!\n",
		this,
		writecursor,
		writebytes,
		lplpaudioptr1,
		audiobytes1,
		lplpaudioptr2,
		audiobytes2,
		flags
	);
*/
	if (flags & DSBLOCK_FROMWRITECURSOR)
		writecursor = this->writepos;

	if (writecursor+writebytes < this->buflen) {
		*(LPBYTE*)lplpaudioptr1 = this->buffer+writecursor;
		*audiobytes1 = writebytes;
		if (lplpaudioptr2)
			*(LPBYTE*)lplpaudioptr2 = NULL;
		if (audiobytes2)
			*audiobytes2 = 0;
	} else {
		*(LPBYTE*)lplpaudioptr1 = this->buffer+writecursor;
		*audiobytes1 = this->buflen-writecursor;
		if (lplpaudioptr2)
			*(LPBYTE*)lplpaudioptr2 = this->buffer;
		if (audiobytes2)
			*audiobytes2 = writebytes-(this->buflen-writecursor);
	}
	this->writepos=(writecursor+writebytes)%this->buflen;
	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_SetCurrentPosition(
	LPDIRECTSOUNDBUFFER this,DWORD newpos
) {
	fprintf(stderr,"IDirectSoundBuffer(%p)->SetCurrentPosition(%ld)\n",this,newpos);
	this->playpos = newpos;
	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_SetPan(
	LPDIRECTSOUNDBUFFER this,LONG newpan
) {
	fprintf(stderr,"IDirectSoundBuffer(%p)->SetPan(%ld),stub!\n",this,newpan);
	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_Unlock(
	LPDIRECTSOUNDBUFFER this,LPVOID p1,DWORD x1,LPVOID p2,DWORD x2
) {
	struct count_info	ci;
/*
	fprintf(stderr,"IDirectSoundBuffer(%p)->Unlock(%p,%ld,%p,%ld),stub!\n",this,p1,x1,p2,x2);
 */
 	fprintf(stderr,"u%ld.%ld,",x1,x2);
	return 0;
}

static struct tagLPDIRECTSOUNDBUFFER_VTABLE dsbvt = {
	(void *)1,
	IDirectSoundBuffer_AddRef,
	IDirectSoundBuffer_Release,
	(void *)4,
	IDirectSoundBuffer_GetCurrentPosition,
	IDirectSoundBuffer_GetFormat,
	IDirectSoundBuffer_GetVolume,
	(void *)8,
        (void *)9,
	IDirectSoundBuffer_GetStatus,
	(void *)11,
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



static HRESULT WINAPI IDirectSound_SetCooperativeLevel(
	LPDIRECTSOUND this,HWND32 hwnd,DWORD level
) {
	fprintf(stderr,"IDirectSound(%p)->SetCooperativeLevel(%08lx,%ld),stub!\n",
		this,(DWORD)hwnd,level
	);
	return 0;
}


static HRESULT WINAPI IDirectSound_CreateSoundBuffer(
	LPDIRECTSOUND this,LPDSBUFFERDESC dsbd,LPLPDIRECTSOUNDBUFFER ppdsb,LPUNKNOWN lpunk
) {
	fprintf(stderr,"IDirectSound(%p)->CreateSoundBuffer(%p,%p,%p),stub!\n",this,dsbd,ppdsb,lpunk);
	fprintf(stderr,"[size=%ld,",dsbd->dwSize);
	fprintf(stderr,"flags = 0x%08lx,",dsbd->dwFlags);
	_dump_DSBCAPS(dsbd->dwFlags);
	fprintf(stderr,"bufferbytes = %ld,",dsbd->dwBufferBytes);
	fprintf(stderr,"lpwfxFormat = %p]\n",dsbd->lpwfxFormat);
	*ppdsb = (LPDIRECTSOUNDBUFFER)HeapAlloc(GetProcessHeap(),0,sizeof(IDirectSoundBuffer));
	(*ppdsb)->ref =1;
	(*ppdsb)->buffer = (LPBYTE)HeapAlloc(GetProcessHeap(),0,dsbd->dwBufferBytes);
	(*ppdsb)->buflen = dsbd->dwBufferBytes;
	(*ppdsb)->playpos = 0;
	(*ppdsb)->writepos = 0;
	(*ppdsb)->lpvtbl = &dsbvt;
	(*ppdsb)->dsound = this;
	(*ppdsb)->playing = 0;

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
	fprintf(stderr,"IDirectSound(%p)->DuplicateSoundBuffer(%p,%p),stub!\n",this,pdsb,ppdsb);

	*ppdsb = (LPDIRECTSOUNDBUFFER)HeapAlloc(GetProcessHeap(),0,sizeof(IDirectSoundBuffer));
	(*ppdsb)->ref =1;
	(*ppdsb)->buffer = (LPBYTE)HeapAlloc(GetProcessHeap(),0,pdsb->buflen);
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


static HRESULT WINAPI IDirectSound_GetCaps(LPDIRECTSOUND this,LPDSCAPS dscaps) {
	fprintf(stderr,"IDirectSound(%p)->GetCaps(%p),stub!\n",this,dscaps);
	fprintf(stderr,"	flags = 0x%08lx\n",dscaps->dwFlags);
	return 0;
}

static ULONG WINAPI IDirectSound_AddRef(LPDIRECTSOUND this) {
	return ++(this->ref);
}

static ULONG WINAPI IDirectSound_Release(LPDIRECTSOUND this) {
	if (!--(this->ref)) {
		HeapFree(GetProcessHeap(),0,this);
		fprintf(stderr,"	IDIrectSound(%p) freed!\n",this);
		dsound = NULL;
		return 0;
	}
	return this->ref;
}

static struct tagLPDIRECTSOUND_VTABLE dsvt = {
	(void *)1,
	IDirectSound_AddRef,
	IDirectSound_Release,
	IDirectSound_CreateSoundBuffer,
	IDirectSound_GetCaps,
	IDirectSound_DuplicateSoundBuffer,
	IDirectSound_SetCooperativeLevel,
	(void *)8,
        (void *)9,
        (void *)10,
        (void *)11
};

void
DSOUND_thread(void) {
	int	res,i,j,curleft,playing;
	short	buf[512];

	while (1) {
		if (!dsound) {
			fprintf(stderr,"DSOUND thread killed\n");
			kill(getpid(),SIGTERM);
			return;
		}
		dsound->lpvtbl->fnAddRef(dsound);
		memset(buf,0,sizeof(buf));
		/* FIXME: assumes 16 bit and same format on all buffers
		 * which must not be the case
		 */
		playing = 0;
		for (i=dsound->nrofbuffers;i--;) {

			dsound->buffers[i]->lpvtbl->fnAddRef(dsound->buffers[i]);
			if (	dsound->buffers[i]->buflen &&
				dsound->buffers[i]->playing
			) {
				int	playpos = dsound->buffers[i]->playpos/sizeof(short);
				int	buflen = dsound->buffers[i]->buflen/sizeof(short);
				short	*xbuf = (short*)(dsound->buffers[i]->buffer);

				playing++;
				dsound->buffers[i]->playpos = (sizeof(buf)+(playpos<<1))%(buflen<<1);
				for (j=0;j<sizeof(buf)/sizeof(short);j++)
					buf[j]+=xbuf[(j+playpos)%buflen];
			}
			dsound->buffers[i]->lpvtbl->fnRelease(dsound->buffers[i]);
		}
		dsound->lpvtbl->fnRelease(dsound);
		/*fputc('0'+playing,stderr);*/
		curleft = 0;
		while (curleft < sizeof(buf)) {
			res = write(audiofd,(LPBYTE)buf+curleft,sizeof(buf)-curleft);
			if (res==-1) {
				perror("write audiofd");
				fprintf(stderr,"buf is %p, curleft is %d\n",buf,curleft);
				kill(getpid(),SIGTERM);
				break;
			}
			curleft+=res;
		}
	}
}

#endif /* HAVE_OSS */

HRESULT WINAPI DirectSoundCreate(LPGUID lpGUID,LPDIRECTSOUND *ppDS,IUnknown *pUnkOuter ) {
#ifdef HAVE_OSS
	int	xx;

	fprintf(stderr,"DirectSoundCreate(%p,%p,%p)\n",lpGUID,ppDS,pUnkOuter);
	if (audiofd)
		return DSERR_ALLOCATED;
	audiofd = open("/dev/audio",O_WRONLY);
	if (audiofd==-1) {
		perror("open /dev/audio");
		audiofd=0;
		return DSERR_NODRIVER;
	}
	/* make it nonblocking */
	if (-1==ioctl(audiofd,SNDCTL_DSP_NONBLOCK,NULL)) {
		perror("ioctl SNDCTL_DSP_NONBLOCK");
		close(audiofd);
		audiofd=0;
		return DSERR_NODRIVER;
	}
	if (-1==ioctl(audiofd,SNDCTL_DSP_GETCAPS,&xx)) {
		perror("ioctl SNDCTL_DSP_GETCAPS");
		close(audiofd);
		audiofd=0;
		return DSERR_NODRIVER;
	}
	fprintf(stderr,"SNDCTL_DSP_GETCAPS returned %x\n",xx);
	*ppDS = (LPDIRECTSOUND)HeapAlloc(GetProcessHeap(),0,sizeof(IDirectSound));
	(*ppDS)->ref		= 1;
	(*ppDS)->lpvtbl		= &dsvt;
	(*ppDS)->buffers	= NULL;
	(*ppDS)->nrofbuffers	= 0;

	if (!dsound) {
		dsound = (*ppDS);
/*		THREAD_CreateSysThread(0,DSOUND_thread); FIXME */
	}
	return 0;
#else
	MessageBox32A(0,"DirectSound needs the Open Sound System Driver, which has not been found by ./configure.","WINE DirectSound",MB_OK|MB_ICONSTOP);
	return DSERR_NODRIVER; /* check */
#endif
}
