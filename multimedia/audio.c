/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*				   
 * Sample Wine Driver for Open Sound System (featured in Linux and FreeBSD)
 *
 * Copyright 1994 Martin Ayotte
 */
/*
 * FIXME:
 *	- record/play should and must be done asynchronous
 *	- segmented/linear pointer problems (lpData in waveheaders,W*_DONE cbs)
 */

#define EMULATE_SB16

#define DEBUG_MCIWAVE

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "windows.h"
#include "user.h"
#include "driver.h"
#include "mmsystem.h"
#include "heap.h"
#include "ldt.h"
#include "debug.h"

#ifdef HAVE_OSS

#ifdef HAVE_MACHINE_SOUNDCARD_H
# include <machine/soundcard.h>
#endif
#ifdef HAVE_SYS_SOUNDCARD_H
# include <sys/soundcard.h>
#endif

#define SOUND_DEV "/dev/dsp"
#define MIXER_DEV "/dev/mixer"

#ifdef SOUND_VERSION
#define IOCTL(a,b,c)		((-1==ioctl(a,b,&c))&&(perror("ioctl:"#b":"#c),0))
#else
#define IOCTL(a,b,c)		(c = ioctl(a,b,c) )
#endif

#define MAX_WAVEOUTDRV 	(1)
#define MAX_WAVEINDRV 	(1)

typedef struct {
    int				unixdev;
    int				state;
    DWORD			bufsize;
    WAVEOPENDESC		waveDesc;
    WORD			wFlags;
    PCMWAVEFORMAT		Format;
    LPWAVEHDR			lpQueueHdr;
    DWORD			dwTotalPlayed;
} WINE_WAVEOUT;

typedef struct {
    int				unixdev;
    int				state;
    DWORD			bufsize;	/* OpenSound '/dev/dsp' give us that size */
    WAVEOPENDESC		waveDesc;
    WORD			wFlags;
    PCMWAVEFORMAT		Format;
    LPWAVEHDR			lpQueueHdr;
    DWORD			dwTotalRecorded;
} WINE_WAVEIN;

static WINE_WAVEOUT	WOutDev   [MAX_WAVEOUTDRV];
static WINE_WAVEIN	WInDev    [MAX_WAVEOUTDRV];

/*======================================================================*
 *                  Low level WAVE implemantation			*
 *======================================================================*/

/**************************************************************************
 * 			WAVE_NotifyClient			[internal]
 */
static DWORD WAVE_NotifyClient(UINT16 wDevID, WORD wMsg, 
			       DWORD dwParam1, DWORD dwParam2)
{
    TRACE(wave,"wDevID = %04X wMsg = %d dwParm1 = %04lX dwParam2 = %04lX\n",wDevID, wMsg, dwParam1, dwParam2);
    
    switch (wMsg) {
    case WOM_OPEN:
    case WOM_CLOSE:
    case WOM_DONE:
	if (wDevID > MAX_WAVEOUTDRV) return MCIERR_INTERNAL;
	
	if (WOutDev[wDevID].wFlags != DCB_NULL && 
	    !DriverCallback(
								  WOutDev[wDevID].waveDesc.dwCallBack, 
								  WOutDev[wDevID].wFlags, 
								  WOutDev[wDevID].waveDesc.hWave, 
								  wMsg, 
								  WOutDev[wDevID].waveDesc.dwInstance, 
								  dwParam1, 
								  dwParam2)) {
	    WARN(wave, "can't notify client !\n");
	    return MMSYSERR_NOERROR;
	}
	break;
	
    case WIM_OPEN:
    case WIM_CLOSE:
    case WIM_DATA:
	if (wDevID > MAX_WAVEINDRV) return MCIERR_INTERNAL;
	
	if (WInDev[wDevID].wFlags != DCB_NULL && 
	    !DriverCallback(
			    WInDev[wDevID].waveDesc.dwCallBack, 
			    WInDev[wDevID].wFlags, 
			    WInDev[wDevID].waveDesc.hWave, 
			    wMsg, 
			    WInDev[wDevID].waveDesc.dwInstance, 
			    dwParam1, 
			    dwParam2)) {
	    WARN(wave, "can't notify client !\n");
	    return MMSYSERR_NOERROR;
	}
	break;
    }
    return 0;
}

/**************************************************************************
 * 			wodGetDevCaps				[internal]
 */
static DWORD wodGetDevCaps(WORD wDevID, LPWAVEOUTCAPS16 lpCaps, DWORD dwSize)
{
    int 	audio;
    int		smplrate;
    int		samplesize = 16;
    int		dsp_stereo = 1;
    int		bytespersmpl;
    
    TRACE(wave, "(%u, %p, %lu);\n", wDevID, lpCaps, dwSize);
    if (lpCaps == NULL) return MMSYSERR_NOTENABLED;
    if (access(SOUND_DEV,0) != 0) return MMSYSERR_NOTENABLED;
    audio = open (SOUND_DEV, O_WRONLY, 0);
    if (audio == -1) return MMSYSERR_ALLOCATED ;
#ifdef EMULATE_SB16
    lpCaps->wMid = 0x0002;
    lpCaps->wPid = 0x0104;
    strcpy(lpCaps->szPname, "SB16 Wave Out");
#else
    lpCaps->wMid = 0x00FF; 	/* Manufac ID */
    lpCaps->wPid = 0x0001; 	/* Product ID */
    strcpy(lpCaps->szPname, "OpenSoundSystem WAVOUT Driver");
#endif
    lpCaps->vDriverVersion = 0x0100;
    lpCaps->dwFormats = 0x00000000;
    lpCaps->dwSupport = WAVECAPS_VOLUME;
    
    /* First bytespersampl, then stereo */
    bytespersmpl = (IOCTL(audio, SNDCTL_DSP_SAMPLESIZE, samplesize) != 0) ? 1 : 2;
    
    lpCaps->wChannels = (IOCTL(audio, SNDCTL_DSP_STEREO, dsp_stereo) != 0) ? 1 : 2;
    if (lpCaps->wChannels > 1) lpCaps->dwSupport |= WAVECAPS_LRVOLUME;
    
    smplrate = 44100;
    if (IOCTL(audio, SNDCTL_DSP_SPEED, smplrate) == 0) {
	lpCaps->dwFormats |= WAVE_FORMAT_4M08;
	if (lpCaps->wChannels > 1)
	    lpCaps->dwFormats |= WAVE_FORMAT_4S08;
	if (bytespersmpl > 1) {
	    lpCaps->dwFormats |= WAVE_FORMAT_4M16;
	    if (lpCaps->wChannels > 1)
		lpCaps->dwFormats |= WAVE_FORMAT_4S16;
	}
    }
    smplrate = 22050;
    if (IOCTL(audio, SNDCTL_DSP_SPEED, smplrate) == 0) {
	lpCaps->dwFormats |= WAVE_FORMAT_2M08;
	if (lpCaps->wChannels > 1)
	    lpCaps->dwFormats |= WAVE_FORMAT_2S08;
	if (bytespersmpl > 1) {
	    lpCaps->dwFormats |= WAVE_FORMAT_2M16;
	    if (lpCaps->wChannels > 1)
		lpCaps->dwFormats |= WAVE_FORMAT_2S16;
	}
    }
    smplrate = 11025;
    if (IOCTL(audio, SNDCTL_DSP_SPEED, smplrate) == 0) {
	lpCaps->dwFormats |= WAVE_FORMAT_1M08;
	if (lpCaps->wChannels > 1)
	    lpCaps->dwFormats |= WAVE_FORMAT_1S08;
	if (bytespersmpl > 1) {
	    lpCaps->dwFormats |= WAVE_FORMAT_1M16;
	    if (lpCaps->wChannels > 1)
		lpCaps->dwFormats |= WAVE_FORMAT_1S16;
	}
    }
    close(audio);
    TRACE(wave, "dwFormats = %08lX\n", lpCaps->dwFormats);
    return MMSYSERR_NOERROR;
}


/**************************************************************************
 * 				wodOpen				[internal]
 */
static DWORD wodOpen(WORD wDevID, LPWAVEOPENDESC lpDesc, DWORD dwFlags)
{
    int 	 	audio,abuf_size,smplrate,samplesize,dsp_stereo;
    LPWAVEFORMAT	lpFormat;
    
    TRACE(wave, "(%u, %p, %08lX);\n", wDevID, lpDesc, dwFlags);
    if (lpDesc == NULL) {
	WARN(wave, "Invalid Parameter !\n");
	return MMSYSERR_INVALPARAM;
    }
    if (wDevID >= MAX_WAVEOUTDRV) {
	TRACE(wave,"MAX_WAVOUTDRV reached !\n");
	return MMSYSERR_ALLOCATED;
    }
    WOutDev[wDevID].unixdev = 0;
    if (access(SOUND_DEV,0) != 0) return MMSYSERR_NOTENABLED;
    audio = open (SOUND_DEV, O_WRONLY, 0);
    if (audio == -1) {
	WARN(wave, "can't open !\n");
	return MMSYSERR_ALLOCATED ;
    }
    IOCTL(audio, SNDCTL_DSP_GETBLKSIZE, abuf_size);
    if (abuf_size < 1024 || abuf_size > 65536) {
	if (abuf_size == -1)
	    WARN(wave, "IOCTL can't 'SNDCTL_DSP_GETBLKSIZE' !\n");
	else
	    WARN(wave, "SNDCTL_DSP_GETBLKSIZE Invalid bufsize !\n");
	return MMSYSERR_NOTENABLED;
    }
    WOutDev[wDevID].wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);
    switch(WOutDev[wDevID].wFlags) {
    case DCB_NULL:
	TRACE(wave, "CALLBACK_NULL !\n");
	break;
    case DCB_WINDOW:
	TRACE(wave, "CALLBACK_WINDOW !\n");
	break;
    case DCB_TASK:
	TRACE(wave, "CALLBACK_TASK !\n");
	break;
    case DCB_FUNCTION:
	TRACE(wave, "CALLBACK_FUNCTION !\n");
	break;
    }
    WOutDev[wDevID].lpQueueHdr = NULL;
    WOutDev[wDevID].unixdev = audio;
    WOutDev[wDevID].dwTotalPlayed = 0;
    WOutDev[wDevID].bufsize = abuf_size;
    /* FIXME: copy lpFormat too? */
    memcpy(&WOutDev[wDevID].waveDesc, lpDesc, sizeof(WAVEOPENDESC));
    TRACE(wave,"lpDesc->lpFormat = %p\n",lpDesc->lpFormat);
    lpFormat = lpDesc->lpFormat; 
    TRACE(wave,"lpFormat = %p\n",lpFormat);
    if (lpFormat->wFormatTag != WAVE_FORMAT_PCM) {
	WARN(wave,"Bad format %04X !\n", lpFormat->wFormatTag);
	WARN(wave,"Bad nChannels %d !\n", lpFormat->nChannels);
	WARN(wave,"Bad nSamplesPerSec %ld !\n", lpFormat->nSamplesPerSec);
	return WAVERR_BADFORMAT;
    }
    memcpy(&WOutDev[wDevID].Format, lpFormat, sizeof(PCMWAVEFORMAT));
    if (WOutDev[wDevID].Format.wf.nChannels == 0) return WAVERR_BADFORMAT;
    if (WOutDev[wDevID].Format.wf.nSamplesPerSec == 0) return WAVERR_BADFORMAT;
    TRACE(wave,"wBitsPerSample=%u !\n", WOutDev[wDevID].Format.wBitsPerSample);
    if (WOutDev[wDevID].Format.wBitsPerSample == 0) {
	WOutDev[wDevID].Format.wBitsPerSample = 8 *
	    (WOutDev[wDevID].Format.wf.nAvgBytesPerSec /
	     WOutDev[wDevID].Format.wf.nSamplesPerSec) /
	    WOutDev[wDevID].Format.wf.nChannels;
    }
    samplesize = WOutDev[wDevID].Format.wBitsPerSample;
    smplrate = WOutDev[wDevID].Format.wf.nSamplesPerSec;
    dsp_stereo = (WOutDev[wDevID].Format.wf.nChannels > 1) ? TRUE : FALSE;
    
    /* First size and stereo then samplerate */
    IOCTL(audio, SNDCTL_DSP_SAMPLESIZE, samplesize);
    IOCTL(audio, SNDCTL_DSP_STEREO, dsp_stereo);
    IOCTL(audio, SNDCTL_DSP_SPEED, smplrate);
    
    TRACE(wave,"wBitsPerSample=%u !\n", WOutDev[wDevID].Format.wBitsPerSample);
    TRACE(wave,"nAvgBytesPerSec=%lu !\n", WOutDev[wDevID].Format.wf.nAvgBytesPerSec);
    TRACE(wave,"nSamplesPerSec=%lu !\n", WOutDev[wDevID].Format.wf.nSamplesPerSec);
    TRACE(wave,"nChannels=%u !\n", WOutDev[wDevID].Format.wf.nChannels);
    if (WAVE_NotifyClient(wDevID, WOM_OPEN, 0L, 0L) != MMSYSERR_NOERROR) {
	WARN(wave, "can't notify client !\n");
	return MMSYSERR_INVALPARAM;
    }
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodClose			[internal]
 */
static DWORD wodClose(WORD wDevID)
{
    TRACE(wave,"(%u);\n", wDevID);

    if (wDevID > MAX_WAVEOUTDRV) return MMSYSERR_INVALPARAM;
    if (WOutDev[wDevID].unixdev == 0) {
	WARN(wave, "can't close !\n");
	return MMSYSERR_NOTENABLED;
    }
    if (WOutDev[wDevID].lpQueueHdr != NULL) {
	WARN(wave, "still buffers open !\n");
	/* Don't care. Who needs those buffers anyway */
	/*return WAVERR_STILLPLAYING; */
    }
    close(WOutDev[wDevID].unixdev);
    WOutDev[wDevID].unixdev = 0;
    WOutDev[wDevID].bufsize = 0;
    WOutDev[wDevID].lpQueueHdr = NULL;
    if (WAVE_NotifyClient(wDevID, WOM_CLOSE, 0L, 0L) != MMSYSERR_NOERROR) {
	WARN(wave, "can't notify client !\n");
	return MMSYSERR_INVALPARAM;
    }
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodWrite			[internal]
 * FIXME: this should _APPEND_ the lpWaveHdr to the output queue of the
 * device, and initiate async playing.
 */
static DWORD wodWrite(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    int		count;
    LPSTR	        lpData;
    LPWAVEHDR	xwavehdr;
    
    TRACE(wave,"(%u, %p, %08lX);\n", wDevID, lpWaveHdr, dwSize);
    if (WOutDev[wDevID].unixdev == 0) {
        WARN(wave, "can't play !\n");
	return MMSYSERR_NOTENABLED;
    }
    if (lpWaveHdr->lpData == NULL) return WAVERR_UNPREPARED;
    if (!(lpWaveHdr->dwFlags & WHDR_PREPARED)) return WAVERR_UNPREPARED;
    if (lpWaveHdr->dwFlags & WHDR_INQUEUE) return WAVERR_STILLPLAYING;
    lpWaveHdr->dwFlags &= ~WHDR_DONE;
    lpWaveHdr->dwFlags |= WHDR_INQUEUE;
    TRACE(wave, "dwBufferLength %lu !\n", lpWaveHdr->dwBufferLength);
    TRACE(wave, "WOutDev[%u].unixdev %u !\n", wDevID, WOutDev[wDevID].unixdev);
    lpData = lpWaveHdr->lpData;
    count = write (WOutDev[wDevID].unixdev, lpData, lpWaveHdr->dwBufferLength);
    TRACE(wave,"write returned count %u !\n",count);
    if (count != lpWaveHdr->dwBufferLength) {
	WARN(wave, " error writting !\n");
	return MMSYSERR_NOTENABLED;
    }
    WOutDev[wDevID].dwTotalPlayed += count;
    lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
    lpWaveHdr->dwFlags |= WHDR_DONE;
    if ((DWORD)lpWaveHdr->lpData!=lpWaveHdr->reserved) {
	/* FIXME: what if it expects it's OWN lpwavehdr back? */
	xwavehdr = SEGPTR_NEW(WAVEHDR);
	memcpy(xwavehdr,lpWaveHdr,sizeof(WAVEHDR));
	xwavehdr->lpData = (LPBYTE)xwavehdr->reserved;
	if (WAVE_NotifyClient(wDevID, WOM_DONE, (DWORD)SEGPTR_GET(xwavehdr), count) != MMSYSERR_NOERROR) {
	    WARN(wave, "can't notify client !\n");
	    SEGPTR_FREE(xwavehdr);
	    return MMSYSERR_INVALPARAM;
	}
	SEGPTR_FREE(xwavehdr);
    } else {
	if (WAVE_NotifyClient(wDevID, WOM_DONE, (DWORD)lpWaveHdr, count) != MMSYSERR_NOERROR) {
	    WARN(wave, "can't notify client !\n");
	    return MMSYSERR_INVALPARAM;
	}
    }
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodPrepare			[internal]
 */
static DWORD wodPrepare(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    TRACE(wave, "(%u, %p, %08lX);\n", wDevID, lpWaveHdr, dwSize);
    if (WOutDev[wDevID].unixdev == 0) {
	WARN(wave, "can't prepare !\n");
	return MMSYSERR_NOTENABLED;
    }
    /* don't append to queue, wodWrite does that */
    WOutDev[wDevID].dwTotalPlayed = 0;
    if (lpWaveHdr->dwFlags & WHDR_INQUEUE)
	return WAVERR_STILLPLAYING;
    lpWaveHdr->dwFlags |= WHDR_PREPARED;
    lpWaveHdr->dwFlags &= ~WHDR_DONE;
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodUnprepare			[internal]
 */
static DWORD wodUnprepare(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    TRACE(wave, "(%u, %p, %08lX);\n", wDevID, lpWaveHdr, dwSize);
    if (WOutDev[wDevID].unixdev == 0) {
	WARN(wave, "can't unprepare !\n");
	return MMSYSERR_NOTENABLED;
    }
    if (lpWaveHdr->dwFlags & WHDR_INQUEUE)
	return WAVERR_STILLPLAYING;
    
    lpWaveHdr->dwFlags &= ~WHDR_PREPARED;
    lpWaveHdr->dwFlags |= WHDR_DONE;
    TRACE(wave, "all headers unprepared !\n");
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			wodRestart				[internal]
 */
static DWORD wodRestart(WORD wDevID)
{
    TRACE(wave,"(%u);\n", wDevID);
    if (WOutDev[wDevID].unixdev == 0) {
	WARN(wave, "can't restart !\n");
	return MMSYSERR_NOTENABLED;
    }
    /* FIXME: is NotifyClient with WOM_DONE right ? (Comet Busters 1.3.3 needs this notification) */
    /* FIXME: Myst crashes with this ... hmm -MM
       if (WAVE_NotifyClient(wDevID, WOM_DONE, 0L, 0L) != MMSYSERR_NOERROR) {
       WARN(wave, "can't notify client !\n");
       return MMSYSERR_INVALPARAM;
       }
    */
    
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			wodReset				[internal]
 */
static DWORD wodReset(WORD wDevID)
{
    TRACE(wave,"(%u);\n", wDevID);
    if (WOutDev[wDevID].unixdev == 0) {
	WARN(wave, "can't reset !\n");
	return MMSYSERR_NOTENABLED;
    }
    return MMSYSERR_NOERROR;
}


/**************************************************************************
 * 				wodGetPosition			[internal]
 */
static DWORD wodGetPosition(WORD wDevID, LPMMTIME16 lpTime, DWORD uSize)
{
    int		time;
    TRACE(wave,"(%u, %p, %lu);\n", wDevID, lpTime, uSize);
    if (WOutDev[wDevID].unixdev == 0) {
	WARN(wave, "can't get pos !\n");
	return MMSYSERR_NOTENABLED;
    }
    if (lpTime == NULL)	return MMSYSERR_INVALPARAM;
    TRACE(wave,"wType=%04X !\n", lpTime->wType);
    TRACE(wave,"wBitsPerSample=%u\n", WOutDev[wDevID].Format.wBitsPerSample); 
    TRACE(wave,"nSamplesPerSec=%lu\n", WOutDev[wDevID].Format.wf.nSamplesPerSec); 
    TRACE(wave,"nChannels=%u\n", WOutDev[wDevID].Format.wf.nChannels); 
    TRACE(wave,"nAvgBytesPerSec=%lu\n", WOutDev[wDevID].Format.wf.nAvgBytesPerSec); 
    switch(lpTime->wType) {
    case TIME_BYTES:
	lpTime->u.cb = WOutDev[wDevID].dwTotalPlayed;
	TRACE(wave,"TIME_BYTES=%lu\n", lpTime->u.cb);
	break;
    case TIME_SAMPLES:
	TRACE(wave,"dwTotalPlayed=%lu\n", WOutDev[wDevID].dwTotalPlayed);
	TRACE(wave,"wBitsPerSample=%u\n", WOutDev[wDevID].Format.wBitsPerSample);
	lpTime->u.sample = WOutDev[wDevID].dwTotalPlayed * 8 /
	    WOutDev[wDevID].Format.wBitsPerSample;
	TRACE(wave,"TIME_SAMPLES=%lu\n", lpTime->u.sample);
	break;
    case TIME_SMPTE:
	time = WOutDev[wDevID].dwTotalPlayed /
	    (WOutDev[wDevID].Format.wf.nAvgBytesPerSec / 1000);
	lpTime->u.smpte.hour = time / 108000;
	time -= lpTime->u.smpte.hour * 108000;
	lpTime->u.smpte.min = time / 1800;
	time -= lpTime->u.smpte.min * 1800;
	lpTime->u.smpte.sec = time / 30;
	time -= lpTime->u.smpte.sec * 30;
	lpTime->u.smpte.frame = time;
	lpTime->u.smpte.fps = 30;
	TRACE(wave, "wodGetPosition // TIME_SMPTE=%02u:%02u:%02u:%02u\n",
	      lpTime->u.smpte.hour, lpTime->u.smpte.min,
	      lpTime->u.smpte.sec, lpTime->u.smpte.frame);
	break;
    default:
	FIXME(wave, "wodGetPosition() format %d not supported ! use TIME_MS !\n",lpTime->wType);
	lpTime->wType = TIME_MS;
    case TIME_MS:
	lpTime->u.ms = WOutDev[wDevID].dwTotalPlayed /
	    (WOutDev[wDevID].Format.wf.nAvgBytesPerSec / 1000);
	TRACE(wave,"wodGetPosition // TIME_MS=%lu\n", lpTime->u.ms);
	break;
    }
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodGetVolume			[internal]
 */
static DWORD wodGetVolume(WORD wDevID, LPDWORD lpdwVol)
{
    int 	mixer;
    int		volume, left, right;
    TRACE(wave,"(%u, %p);\n", wDevID, lpdwVol);
    if (lpdwVol == NULL) return MMSYSERR_NOTENABLED;
    if ((mixer = open(MIXER_DEV, O_RDONLY)) < 0) {
	WARN(wave, "mixer device not available !\n");
	return MMSYSERR_NOTENABLED;
    }
    if (ioctl(mixer, SOUND_MIXER_READ_PCM, &volume) == -1) {
	WARN(wave, "unable read mixer !\n");
	return MMSYSERR_NOTENABLED;
    }
    close(mixer);
    left = volume & 0x7F;
    right = (volume >> 8) & 0x7F;
    TRACE(wave,"left=%d right=%d !\n", left, right);
    *lpdwVol = MAKELONG(left << 9, right << 9);
    return MMSYSERR_NOERROR;
}


/**************************************************************************
 * 				wodSetVolume			[internal]
 */
static DWORD wodSetVolume(WORD wDevID, DWORD dwParam)
{
    int 	mixer;
    int		volume;
    TRACE(wave,"(%u, %08lX);\n", wDevID, dwParam);
    volume = (LOWORD(dwParam) >> 9 & 0x7F) + 
	((HIWORD(dwParam) >> 9  & 0x7F) << 8);
    if ((mixer = open(MIXER_DEV, O_WRONLY)) < 0) {
	WARN(wave,	"mixer device not available !\n");
	return MMSYSERR_NOTENABLED;
    }
    if (ioctl(mixer, SOUND_MIXER_WRITE_PCM, &volume) == -1) {
	WARN(wave, "unable set mixer !\n");
	return MMSYSERR_NOTENABLED;
    }
    close(mixer);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodMessage			[sample driver]
 */
DWORD WINAPI wodMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
			DWORD dwParam1, DWORD dwParam2)
{
    int audio;
    TRACE(wave,"wodMessage(%u, %04X, %08lX, %08lX, %08lX);\n",
	  wDevID, wMsg, dwUser, dwParam1, dwParam2);
    switch(wMsg) {
    case WODM_OPEN:
	return wodOpen(wDevID, (LPWAVEOPENDESC)dwParam1, dwParam2);
    case WODM_CLOSE:
	return wodClose(wDevID);
    case WODM_WRITE:
	return wodWrite(wDevID, (LPWAVEHDR)dwParam1, dwParam2);
    case WODM_PAUSE:
	return MMSYSERR_NOTSUPPORTED;
    case WODM_STOP:
	return MMSYSERR_NOTSUPPORTED;
    case WODM_GETPOS:
	return wodGetPosition(wDevID, (LPMMTIME16)dwParam1, dwParam2);
    case WODM_BREAKLOOP:
	return MMSYSERR_NOTSUPPORTED;
    case WODM_PREPARE:
	return wodPrepare(wDevID, (LPWAVEHDR)dwParam1, dwParam2);
    case WODM_UNPREPARE:
	return wodUnprepare(wDevID, (LPWAVEHDR)dwParam1, dwParam2);
    case WODM_GETDEVCAPS:
	return wodGetDevCaps(wDevID,(LPWAVEOUTCAPS16)dwParam1,dwParam2);
    case WODM_GETNUMDEVS:
	/* FIXME: For now, only one sound device (SOUND_DEV) is allowed */
	audio = open (SOUND_DEV, O_WRONLY, 0);
	if (audio == -1)
        {
	    if (errno == EBUSY)
		return 1;
	    else
		return 0;
        }
	close (audio);
	return 1;
    case WODM_GETPITCH:
	return MMSYSERR_NOTSUPPORTED;
    case WODM_SETPITCH:
	return MMSYSERR_NOTSUPPORTED;
    case WODM_GETPLAYBACKRATE:
	return MMSYSERR_NOTSUPPORTED;
    case WODM_SETPLAYBACKRATE:
	return MMSYSERR_NOTSUPPORTED;
    case WODM_GETVOLUME:
	return wodGetVolume(wDevID, (LPDWORD)dwParam1);
    case WODM_SETVOLUME:
	return wodSetVolume(wDevID, dwParam1);
    case WODM_RESTART:
	return wodRestart(wDevID);
    case WODM_RESET:
	return wodReset(wDevID);
    default:
	WARN(wave,"unknown message !\n");
    }
    return MMSYSERR_NOTSUPPORTED;
}


/*-----------------------------------------------------------------------*/

/**************************************************************************
 * 			widGetDevCaps				[internal]
 */
static DWORD widGetDevCaps(WORD wDevID, LPWAVEINCAPS16 lpCaps, DWORD dwSize)
{
    int 	audio,smplrate,samplesize=16,dsp_stereo=1,bytespersmpl;
    
    TRACE(wave, "(%u, %p, %lu);\n", wDevID, lpCaps, dwSize);
    if (lpCaps == NULL) return MMSYSERR_NOTENABLED;
    if (access(SOUND_DEV,0) != 0) return MMSYSERR_NOTENABLED;
    audio = open (SOUND_DEV, O_RDONLY, 0);
    if (audio == -1) return MMSYSERR_ALLOCATED ;
#ifdef EMULATE_SB16
    lpCaps->wMid = 0x0002;
    lpCaps->wPid = 0x0004;
    strcpy(lpCaps->szPname, "SB16 Wave In");
#else
    lpCaps->wMid = 0x00FF; 	/* Manufac ID */
    lpCaps->wPid = 0x0001; 	/* Product ID */
    strcpy(lpCaps->szPname, "OpenSoundSystem WAVIN Driver");
#endif
    lpCaps->dwFormats = 0x00000000;
    lpCaps->wChannels = (IOCTL(audio, SNDCTL_DSP_STEREO, dsp_stereo) != 0) ? 1 : 2;
    bytespersmpl = (IOCTL(audio, SNDCTL_DSP_SAMPLESIZE, samplesize) != 0) ? 1 : 2;
    smplrate = 44100;
    if (IOCTL(audio, SNDCTL_DSP_SPEED, smplrate) == 0) {
	lpCaps->dwFormats |= WAVE_FORMAT_4M08;
	if (lpCaps->wChannels > 1)
	    lpCaps->dwFormats |= WAVE_FORMAT_4S08;
	if (bytespersmpl > 1) {
	    lpCaps->dwFormats |= WAVE_FORMAT_4M16;
	    if (lpCaps->wChannels > 1)
		lpCaps->dwFormats |= WAVE_FORMAT_4S16;
	}
    }
    smplrate = 22050;
    if (IOCTL(audio, SNDCTL_DSP_SPEED, smplrate) == 0) {
	lpCaps->dwFormats |= WAVE_FORMAT_2M08;
	if (lpCaps->wChannels > 1)
	    lpCaps->dwFormats |= WAVE_FORMAT_2S08;
	if (bytespersmpl > 1) {
	    lpCaps->dwFormats |= WAVE_FORMAT_2M16;
	    if (lpCaps->wChannels > 1)
		lpCaps->dwFormats |= WAVE_FORMAT_2S16;
	}
    }
    smplrate = 11025;
    if (IOCTL(audio, SNDCTL_DSP_SPEED, smplrate) == 0) {
	lpCaps->dwFormats |= WAVE_FORMAT_1M08;
	if (lpCaps->wChannels > 1)
	    lpCaps->dwFormats |= WAVE_FORMAT_1S08;
	if (bytespersmpl > 1) {
	    lpCaps->dwFormats |= WAVE_FORMAT_1M16;
	    if (lpCaps->wChannels > 1)
		lpCaps->dwFormats |= WAVE_FORMAT_1S16;
	}
    }
    close(audio);
    TRACE(wave, "dwFormats = %08lX\n", lpCaps->dwFormats);
    return MMSYSERR_NOERROR;
}


/**************************************************************************
 * 				widOpen				[internal]
 */
static DWORD widOpen(WORD wDevID, LPWAVEOPENDESC lpDesc, DWORD dwFlags)
{
    int 		audio,abuf_size,smplrate,samplesize,dsp_stereo;
    LPWAVEFORMAT	lpFormat;
    
    TRACE(wave, "(%u, %p, %08lX);\n", wDevID, lpDesc, dwFlags);
    if (lpDesc == NULL) {
	WARN(wave, "Invalid Parameter !\n");
	return MMSYSERR_INVALPARAM;
    }
    if (wDevID >= MAX_WAVEINDRV) {
	TRACE(wave,"MAX_WAVINDRV reached !\n");
	return MMSYSERR_ALLOCATED;
    }
    WInDev[wDevID].unixdev = 0;
    if (access(SOUND_DEV,0) != 0) return MMSYSERR_NOTENABLED;
    audio = open (SOUND_DEV, O_RDONLY, 0);
    if (audio == -1) {
	WARN(wave,"can't open !\n");
	return MMSYSERR_ALLOCATED;
    }
    IOCTL(audio, SNDCTL_DSP_GETBLKSIZE, abuf_size);
    if (abuf_size < 1024 || abuf_size > 65536) {
	if (abuf_size == -1)
	    WARN(wave, "IOCTL can't 'SNDCTL_DSP_GETBLKSIZE' !\n");
	else
	    WARN(wave, "SNDCTL_DSP_GETBLKSIZE Invalid bufsize !\n");
	return MMSYSERR_NOTENABLED;
    }
    WInDev[wDevID].wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);
    switch(WInDev[wDevID].wFlags) {
    case DCB_NULL:
	TRACE(wave,"CALLBACK_NULL!\n");
	break;
    case DCB_WINDOW:
	TRACE(wave,"CALLBACK_WINDOW!\n");
	break;
    case DCB_TASK:
	TRACE(wave,"CALLBACK_TASK!\n");
	break;
    case DCB_FUNCTION:
	TRACE(wave,"CALLBACK_FUNCTION!\n");
	break;
    }
    if (WInDev[wDevID].lpQueueHdr) {
	HeapFree(GetProcessHeap(),0,WInDev[wDevID].lpQueueHdr);
	WInDev[wDevID].lpQueueHdr = NULL;
    }
    WInDev[wDevID].unixdev = audio;
    WInDev[wDevID].bufsize = abuf_size;
    WInDev[wDevID].dwTotalRecorded = 0;
    memcpy(&WInDev[wDevID].waveDesc, lpDesc, sizeof(WAVEOPENDESC));
    lpFormat = (LPWAVEFORMAT) lpDesc->lpFormat; 
    if (lpFormat->wFormatTag != WAVE_FORMAT_PCM) {
	WARN(wave, "Bad format %04X !\n",
	     lpFormat->wFormatTag);
	return WAVERR_BADFORMAT;
    }
    memcpy(&WInDev[wDevID].Format, lpFormat, sizeof(PCMWAVEFORMAT));
    WInDev[wDevID].Format.wBitsPerSample = 8; /* <-------------- */
    if (WInDev[wDevID].Format.wf.nChannels == 0) return WAVERR_BADFORMAT;
    if (WInDev[wDevID].Format.wf.nSamplesPerSec == 0) return WAVERR_BADFORMAT;
    if (WInDev[wDevID].Format.wBitsPerSample == 0) {
	WInDev[wDevID].Format.wBitsPerSample = 8 *
	    (WInDev[wDevID].Format.wf.nAvgBytesPerSec /
	     WInDev[wDevID].Format.wf.nSamplesPerSec) /
	    WInDev[wDevID].Format.wf.nChannels;
    }
    samplesize = WInDev[wDevID].Format.wBitsPerSample;
    smplrate = WInDev[wDevID].Format.wf.nSamplesPerSec;
    dsp_stereo = (WInDev[wDevID].Format.wf.nChannels > 1) ? TRUE : FALSE;
    IOCTL(audio, SNDCTL_DSP_SPEED, smplrate);
    IOCTL(audio, SNDCTL_DSP_SAMPLESIZE, samplesize);
    IOCTL(audio, SNDCTL_DSP_STEREO, dsp_stereo);
    TRACE(wave,"wBitsPerSample=%u !\n", WInDev[wDevID].Format.wBitsPerSample);
    TRACE(wave,"nSamplesPerSec=%lu !\n", WInDev[wDevID].Format.wf.nSamplesPerSec);
    TRACE(wave,"nChannels=%u !\n", WInDev[wDevID].Format.wf.nChannels);
    TRACE(wave,"nAvgBytesPerSec=%lu\n", WInDev[wDevID].Format.wf.nAvgBytesPerSec); 
    if (WAVE_NotifyClient(wDevID, WIM_OPEN, 0L, 0L) != MMSYSERR_NOERROR) {
	WARN(wave,"can't notify client !\n");
	return MMSYSERR_INVALPARAM;
    }
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				widClose			[internal]
 */
static DWORD widClose(WORD wDevID)
{
    TRACE(wave,"(%u);\n", wDevID);
    if (wDevID > MAX_WAVEINDRV) return MMSYSERR_INVALPARAM;
    if (WInDev[wDevID].unixdev == 0) {
	WARN(wave,"can't close !\n");
	return MMSYSERR_NOTENABLED;
    }
    if (WInDev[wDevID].lpQueueHdr != NULL) {
	WARN(wave, "still buffers open !\n");
	return WAVERR_STILLPLAYING;
    }
    close(WInDev[wDevID].unixdev);
    WInDev[wDevID].unixdev = 0;
    WInDev[wDevID].bufsize = 0;
    if (WAVE_NotifyClient(wDevID, WIM_CLOSE, 0L, 0L) != MMSYSERR_NOERROR) {
	WARN(wave,"can't notify client !\n");
	return MMSYSERR_INVALPARAM;
    }
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				widAddBuffer		[internal]
 */
static DWORD widAddBuffer(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    int		count	= 1;
    LPWAVEHDR 	lpWIHdr;
    
    TRACE(wave, "(%u, %p, %08lX);\n", wDevID, lpWaveHdr, dwSize);
    if (WInDev[wDevID].unixdev == 0) {
	WARN(wave,"can't do it !\n");
	return MMSYSERR_NOTENABLED;
    }
    if (!(lpWaveHdr->dwFlags & WHDR_PREPARED)) {
	TRACE(wave, "never been prepared !\n");
	return WAVERR_UNPREPARED;
    }
    if (lpWaveHdr->dwFlags & WHDR_INQUEUE) {
	TRACE(wave,	"header already in use !\n");
	return WAVERR_STILLPLAYING;
    }
    lpWaveHdr->dwFlags |= WHDR_PREPARED;
    lpWaveHdr->dwFlags |= WHDR_INQUEUE;
    lpWaveHdr->dwFlags &= ~WHDR_DONE;
    lpWaveHdr->dwBytesRecorded = 0;
    if (WInDev[wDevID].lpQueueHdr == NULL) {
	WInDev[wDevID].lpQueueHdr = lpWaveHdr;
    } else {
	lpWIHdr = WInDev[wDevID].lpQueueHdr;
	while (lpWIHdr->lpNext != NULL) {
	    lpWIHdr = lpWIHdr->lpNext;
	    count++;
	}
	lpWIHdr->lpNext = lpWaveHdr;
	lpWaveHdr->lpNext = NULL;
	count++;
    }
    TRACE(wave, "buffer added ! (now %u in queue)\n", count);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				widPrepare			[internal]
 */
static DWORD widPrepare(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    TRACE(wave, "(%u, %p, %08lX);\n", wDevID, lpWaveHdr, dwSize);
    if (WInDev[wDevID].unixdev == 0) {
	WARN(wave,"can't prepare !\n");
	return MMSYSERR_NOTENABLED;
    }
    if (lpWaveHdr->dwFlags & WHDR_INQUEUE)
	return WAVERR_STILLPLAYING;
    lpWaveHdr->dwFlags |= WHDR_PREPARED;
    lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
    lpWaveHdr->dwFlags &= ~WHDR_DONE;
    lpWaveHdr->dwBytesRecorded = 0;
    TRACE(wave,"header prepared !\n");
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				widUnprepare			[internal]
 */
static DWORD widUnprepare(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    TRACE(wave, "(%u, %p, %08lX);\n", wDevID, lpWaveHdr, dwSize);
    if (WInDev[wDevID].unixdev == 0) {
	WARN(wave,"can't unprepare !\n");
	return MMSYSERR_NOTENABLED;
    }
    lpWaveHdr->dwFlags &= ~WHDR_PREPARED;
    lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
    lpWaveHdr->dwFlags |= WHDR_DONE;
    
    TRACE(wave, "all headers unprepared !\n");
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			widStart				[internal]
 */
static DWORD widStart(WORD wDevID)
{
    int		count	= 1;
    int            bytesRead;
    LPWAVEHDR 	lpWIHdr;
    LPWAVEHDR	*lpWaveHdr;
    
    TRACE(wave,"(%u);\n", wDevID);
    if (WInDev[wDevID].unixdev == 0) {
	WARN(wave, "can't start recording !\n");
	return MMSYSERR_NOTENABLED;
    }
    
    lpWaveHdr = &(WInDev[wDevID].lpQueueHdr);
    TRACE(wave,"lpWaveHdr = %08lx\n",(DWORD)lpWaveHdr);
    if (!*lpWaveHdr || !(*lpWaveHdr)->lpData) {
	TRACE(wave,"never been prepared !\n");
	return WAVERR_UNPREPARED;
    }
    
    while(*lpWaveHdr != NULL) {
	lpWIHdr = *lpWaveHdr;
	TRACE(wave, "recording buf#%u=%p size=%lu \n",
	      count, lpWIHdr->lpData, lpWIHdr->dwBufferLength);
	fflush(stddeb);
	bytesRead = read (WInDev[wDevID].unixdev, 
			  lpWIHdr->lpData,
			  lpWIHdr->dwBufferLength);
	if (bytesRead==-1)
	    perror("read from audio device");
	TRACE(wave,"bytesread=%d (%ld)\n", bytesRead, lpWIHdr->dwBufferLength);
	lpWIHdr->dwBytesRecorded = bytesRead;
	WInDev[wDevID].dwTotalRecorded += lpWIHdr->dwBytesRecorded;
	lpWIHdr->dwFlags &= ~WHDR_INQUEUE;
	lpWIHdr->dwFlags |= WHDR_DONE;
	
	/* FIXME: should pass segmented pointer here, do we need that?*/
	if (WAVE_NotifyClient(wDevID, WIM_DATA, (DWORD)lpWaveHdr, lpWIHdr->dwBytesRecorded) != MMSYSERR_NOERROR) {
	    WARN(wave, "can't notify client !\n");
	    return MMSYSERR_INVALPARAM;
	}
	/* removes the current block from the queue */
	*lpWaveHdr = lpWIHdr->lpNext;
	count++;
    }
    TRACE(wave,"end of recording !\n");
    fflush(stddeb);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			widStop					[internal]
 */
static DWORD widStop(WORD wDevID)
{
    TRACE(wave,"(%u);\n", wDevID);
    if (WInDev[wDevID].unixdev == 0) {
	WARN(wave,"can't stop !\n");
	return MMSYSERR_NOTENABLED;
    }
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			widReset				[internal]
 */
static DWORD widReset(WORD wDevID)
{
    TRACE(wave,"(%u);\n", wDevID);
    if (WInDev[wDevID].unixdev == 0) {
	WARN(wave,"can't reset !\n");
	return MMSYSERR_NOTENABLED;
    }
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				widGetPosition			[internal]
 */
static DWORD widGetPosition(WORD wDevID, LPMMTIME16 lpTime, DWORD uSize)
{
    int		time;
    
    TRACE(wave, "(%u, %p, %lu);\n", wDevID, lpTime, uSize);
    if (WInDev[wDevID].unixdev == 0) {
	WARN(wave,"can't get pos !\n");
	return MMSYSERR_NOTENABLED;
    }
    if (lpTime == NULL)	return MMSYSERR_INVALPARAM;
    TRACE(wave,"wType=%04X !\n", lpTime->wType);
    TRACE(wave,"wBitsPerSample=%u\n", WInDev[wDevID].Format.wBitsPerSample); 
    TRACE(wave,"nSamplesPerSec=%lu\n", WInDev[wDevID].Format.wf.nSamplesPerSec); 
    TRACE(wave,"nChannels=%u\n", WInDev[wDevID].Format.wf.nChannels); 
    TRACE(wave,"nAvgBytesPerSec=%lu\n", WInDev[wDevID].Format.wf.nAvgBytesPerSec); 
    fflush(stddeb);
    switch(lpTime->wType) {
    case TIME_BYTES:
	lpTime->u.cb = WInDev[wDevID].dwTotalRecorded;
	TRACE(wave,"TIME_BYTES=%lu\n", lpTime->u.cb);
	break;
    case TIME_SAMPLES:
	lpTime->u.sample = WInDev[wDevID].dwTotalRecorded * 8 /
	    WInDev[wDevID].Format.wBitsPerSample;
	TRACE(wave, "TIME_SAMPLES=%lu\n", lpTime->u.sample);
	break;
    case TIME_SMPTE:
	time = WInDev[wDevID].dwTotalRecorded /
	    (WInDev[wDevID].Format.wf.nAvgBytesPerSec / 1000);
	lpTime->u.smpte.hour = time / 108000;
	time -= lpTime->u.smpte.hour * 108000;
	lpTime->u.smpte.min = time / 1800;
	time -= lpTime->u.smpte.min * 1800;
	lpTime->u.smpte.sec = time / 30;
	time -= lpTime->u.smpte.sec * 30;
	lpTime->u.smpte.frame = time;
	lpTime->u.smpte.fps = 30;
	TRACE(wave,"TIME_SMPTE=%02u:%02u:%02u:%02u\n",
	      lpTime->u.smpte.hour, lpTime->u.smpte.min,
	      lpTime->u.smpte.sec, lpTime->u.smpte.frame);
	break;
    case TIME_MS:
	lpTime->u.ms = WInDev[wDevID].dwTotalRecorded /
	    (WInDev[wDevID].Format.wf.nAvgBytesPerSec / 1000);
	TRACE(wave, "TIME_MS=%lu\n", lpTime->u.ms);
	break;
    default:
	FIXME(wave, "format not supported ! use TIME_MS !\n");
	lpTime->wType = TIME_MS;
    }
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				widMessage			[sample driver]
 */
DWORD WINAPI widMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
			DWORD dwParam1, DWORD dwParam2)
{
    int audio;
    TRACE(wave,"widMessage(%u, %04X, %08lX, %08lX, %08lX);\n",
	  wDevID, wMsg, dwUser, dwParam1, dwParam2);
    switch(wMsg) {
    case WIDM_OPEN:
	return widOpen(wDevID, (LPWAVEOPENDESC)dwParam1, dwParam2);
    case WIDM_CLOSE:
	return widClose(wDevID);
    case WIDM_ADDBUFFER:
	return widAddBuffer(wDevID, (LPWAVEHDR)dwParam1, dwParam2);
    case WIDM_PREPARE:
	return widPrepare(wDevID, (LPWAVEHDR)dwParam1, dwParam2);
    case WIDM_UNPREPARE:
	return widUnprepare(wDevID, (LPWAVEHDR)dwParam1, dwParam2);
    case WIDM_GETDEVCAPS:
	return widGetDevCaps(wDevID, (LPWAVEINCAPS16)dwParam1,dwParam2);
    case WIDM_GETNUMDEVS:
	/* FIXME: For now, only one sound device (SOUND_DEV) is allowed */
	audio = open (SOUND_DEV, O_RDONLY, 0);
	if (audio == -1)
        {
	    if (errno == EBUSY)
		return 1;
	    else
		return 0;
        }
	close (audio);
	return 1;
    case WIDM_GETPOS:
	return widGetPosition(wDevID, (LPMMTIME16)dwParam1, dwParam2);
    case WIDM_RESET:
	return widReset(wDevID);
    case WIDM_START:
	return widStart(wDevID);
    case WIDM_PAUSE:
	return widStop(wDevID);
    case WIDM_STOP:
	return widStop(wDevID);
    default:
	WARN(wave,"unknown message !\n");
    }
    return MMSYSERR_NOTSUPPORTED;
}


/**************************************************************************
 * 				WAVE_DriverProc16		[sample driver]
 */
LONG WAVE_DriverProc16(DWORD dwDevID, HDRVR16 hDriv, WORD wMsg, 
		       DWORD dwParam1, DWORD dwParam2)
{
    TRACE(wave,"(%08lX, %04X, %04X, %08lX, %08lX)\n", dwDevID, hDriv, wMsg, dwParam1, dwParam2);

    switch(wMsg) {
    case DRV_LOAD:		return 1;
    case DRV_FREE:		return 1;
    case DRV_OPEN:		return 1;
    case DRV_CLOSE:		return 1;
    case DRV_ENABLE:		return 1;
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBox16(0, "Sample MultiMedia Linux Driver !", "MMLinux Driver", MB_OK);	return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;
    default:
	FIXME(wave, "is probably wrong msg=0x%04x\n", wMsg);
	return DefDriverProc16(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
    return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 				WAVE_DriverProc32		[sample driver]
 */
LONG WAVE_DriverProc32(DWORD dwDevID, HDRVR16 hDriv, DWORD wMsg, 
		       DWORD dwParam1, DWORD dwParam2)
{
    TRACE(wave,"(%08lX, %04X, %08lX, %08lX, %08lX)\n", dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    switch(wMsg) {
    case DRV_LOAD:		return 1;
    case DRV_FREE:		return 1;
    case DRV_OPEN:		return 1;
    case DRV_CLOSE:		return 1;
    case DRV_ENABLE:		return 1;
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBox16(0, "Sample MultiMedia Linux Driver !", "MMLinux Driver", MB_OK);	return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;
    default:
	FIXME(wave, "is probably wrong msg=0x%04lx\n", wMsg);
	return DefDriverProc32(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
    return MMSYSERR_NOTENABLED;
}

#else /* !HAVE_OSS */

/**************************************************************************
 * 				wodMessage			[sample driver]
 */
DWORD WINAPI wodMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
			DWORD dwParam1, DWORD dwParam2)
{
    FIXME(wave,"(%u, %04X, %08lX, %08lX, %08lX):stub\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);
    return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 				widMessage			[sample driver]
 */
DWORD WINAPI widMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
			DWORD dwParam1, DWORD dwParam2)
{
    FIXME(wave,"(%u, %04X, %08lX, %08lX, %08lX):stub\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);
    return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 				WAVE_DriverProc16		[sample driver]
 */
LONG WAVE_DriverProc16(DWORD dwDevID, HDRVR16 hDriv, WORD wMsg, 
		       DWORD dwParam1, DWORD dwParam2)
{
    return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 				WAVE_DriverProc32		[sample driver]
 */
LONG WAVE_DriverProc32(DWORD dwDevID, HDRVR16 hDriv, DWORD wMsg, 
		       DWORD dwParam1, DWORD dwParam2)
{
    return MMSYSERR_NOTENABLED;
}
#endif /* HAVE_OSS */
