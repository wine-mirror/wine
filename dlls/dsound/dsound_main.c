/*  			DirectSound
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998 Rob Riggs
 * Copyright 2000-2002 TransGaming Technologies, Inc.
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
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <math.h>	/* Insomnia - pow() function */

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "mmsystem.h"
#include "winternl.h"
#include "mmddk.h"
#include "wine/windef16.h"
#include "wine/winbase16.h"
#include "wine/debug.h"
#include "dsound.h"
#include "dsdriver.h"
#include "dsound_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);

/* these are eligible for tuning... they must be high on slow machines... */
/* some stuff may get more responsive with lower values though... */
#define DS_EMULDRIVER 0 /* some games (Quake 2, UT) refuse to accept
				emulated dsound devices. set to 0 ! */
#define DS_HEL_MARGIN 5 /* HEL only: number of waveOut fragments ahead to mix in new buffers
			 * (keep this close or equal to DS_HEL_QUEUE for best results) */
#define DS_HEL_QUEUE  5 /* HEL only: number of waveOut fragments ahead to queue to driver
			 * (this will affect HEL sound reliability and latency) */

#define DS_SND_QUEUE_MAX 28 /* max number of fragments to prebuffer */
#define DS_SND_QUEUE_MIN 12 /* min number of fragments to prebuffer */

IDirectSoundImpl*	dsound = NULL;

HRESULT mmErr(UINT err)
{
	switch(err) {
	case MMSYSERR_NOERROR:
		return DS_OK;
	case MMSYSERR_ALLOCATED:
		return DSERR_ALLOCATED;
	case MMSYSERR_ERROR:
	case MMSYSERR_INVALHANDLE:
	case WAVERR_STILLPLAYING:
		return DSERR_GENERIC; /* FIXME */
	case MMSYSERR_NODRIVER:
		return DSERR_NODRIVER;
	case MMSYSERR_NOMEM:
		return DSERR_OUTOFMEMORY;
	case MMSYSERR_INVALPARAM:
	case WAVERR_BADFORMAT:
	case WAVERR_UNPREPARED:
		return DSERR_INVALIDPARAM;
	case MMSYSERR_NOTSUPPORTED:
		return DSERR_UNSUPPORTED;
	default:
		FIXME("Unknown MMSYS error %d\n",err);
		return DSERR_GENERIC;
	}
}

int ds_emuldriver = DS_EMULDRIVER;
int ds_hel_margin = DS_HEL_MARGIN;
int ds_hel_queue = DS_HEL_QUEUE;
int ds_snd_queue_max = DS_SND_QUEUE_MAX;
int ds_snd_queue_min = DS_SND_QUEUE_MIN;
int ds_hw_accel = DS_HW_ACCEL_FULL;
int ds_default_playback = 0;
int ds_default_capture = 0;

/*
 * Get a config key from either the app-specific or the default config
 */

inline static DWORD get_config_key( HKEY defkey, HKEY appkey, const char *name,
                                    char *buffer, DWORD size )
{
    if (appkey && !RegQueryValueExA( appkey, name, 0, NULL, buffer, &size )) return 0;
    return RegQueryValueExA( defkey, name, 0, NULL, buffer, &size );
}


/*
 * Setup the dsound options.
 */

void setup_dsound_options(void)
{
    char buffer[MAX_PATH+1];
    HKEY hkey, appkey = 0;

    buffer[MAX_PATH]='\0';

    if (RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Wine\\Wine\\Config\\dsound", 0, NULL,
                         REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL ))
    {
        ERR("Cannot create config registry key\n" );
        ExitProcess(1);
    }

    if (GetModuleFileNameA( 0, buffer, MAX_PATH ))
    {
        HKEY tmpkey;

        if (!RegOpenKeyA( HKEY_LOCAL_MACHINE, "Software\\Wine\\Wine\\Config\\AppDefaults", &tmpkey ))
        {
           char appname[MAX_PATH+16];
           char *p = strrchr( buffer, '\\' );
           if (p!=NULL) {
                   appname[MAX_PATH]='\0';
                   strncpy(appname,p+1,MAX_PATH);
                   strcat(appname,"\\dsound");
                   TRACE("appname = [%s] \n",appname);
                   if (RegOpenKeyA( tmpkey, appname, &appkey )) appkey = 0;
                       RegCloseKey( tmpkey );
           }
        }
    }

    /* get options */

    if (!get_config_key( hkey, appkey, "EmulDriver", buffer, MAX_PATH ))
        ds_emuldriver = strcmp(buffer, "N");

    if (!get_config_key( hkey, appkey, "HELmargin", buffer, MAX_PATH ))
        ds_hel_margin = atoi(buffer);

    if (!get_config_key( hkey, appkey, "HELqueue", buffer, MAX_PATH ))
        ds_hel_queue = atoi(buffer);

    if (!get_config_key( hkey, appkey, "SndQueueMax", buffer, MAX_PATH ))
        ds_snd_queue_max = atoi(buffer);

    if (!get_config_key( hkey, appkey, "SndQueueMin", buffer, MAX_PATH ))
        ds_snd_queue_min = atoi(buffer);

    if (!get_config_key( hkey, appkey, "HardwareAcceleration", buffer, MAX_PATH )) {
	if (strcmp(buffer, "Full") == 0)
	    ds_hw_accel = DS_HW_ACCEL_FULL;
	else if (strcmp(buffer, "Standard") == 0)
	    ds_hw_accel = DS_HW_ACCEL_STANDARD;
	else if (strcmp(buffer, "Basic") == 0)
	    ds_hw_accel = DS_HW_ACCEL_BASIC;
	else if (strcmp(buffer, "Emulation") == 0)
	    ds_hw_accel = DS_HW_ACCEL_EMULATION;
    }

    if (!get_config_key( hkey, appkey, "DefaultPlayback", buffer, MAX_PATH ))
	    ds_default_playback = atoi(buffer);

    if (!get_config_key( hkey, appkey, "DefaultCapture", buffer, MAX_PATH ))
	    ds_default_capture = atoi(buffer);

    if (appkey) RegCloseKey( appkey );
    RegCloseKey( hkey );

    if (ds_emuldriver != DS_EMULDRIVER )
       WARN("ds_emuldriver = %d (default=%d)\n",ds_emuldriver, DS_EMULDRIVER);
    if (ds_hel_margin != DS_HEL_MARGIN )
       WARN("ds_hel_margin = %d (default=%d)\n",ds_hel_margin, DS_HEL_MARGIN );
    if (ds_hel_queue != DS_HEL_QUEUE )
       WARN("ds_hel_queue = %d (default=%d)\n",ds_hel_queue, DS_HEL_QUEUE );
    if (ds_snd_queue_max != DS_SND_QUEUE_MAX)
       WARN("ds_snd_queue_max = %d (default=%d)\n",ds_snd_queue_max ,DS_SND_QUEUE_MAX);
    if (ds_snd_queue_min != DS_SND_QUEUE_MIN)
       WARN("ds_snd_queue_min = %d (default=%d)\n",ds_snd_queue_min ,DS_SND_QUEUE_MIN);
    if (ds_hw_accel != DS_HW_ACCEL_FULL)
	WARN("ds_hw_accel = %s (default=Full)\n", 
	    ds_hw_accel==DS_HW_ACCEL_FULL ? "Full" :
	    ds_hw_accel==DS_HW_ACCEL_STANDARD ? "Standard" :
	    ds_hw_accel==DS_HW_ACCEL_BASIC ? "Basic" :
	    ds_hw_accel==DS_HW_ACCEL_EMULATION ? "Emulation" :
	    "Unknown");
    if (ds_default_playback != 0)
	WARN("ds_default_playback = %d (default=0)\n",ds_default_playback);
    if (ds_default_capture != 0)
	WARN("ds_default_capture = %d (default=0)\n",ds_default_playback);
}



/***************************************************************************
 * GetDeviceID	[DSOUND.9]
 *
 * Retrieves unique identifier of default device specified
 *
 * PARAMS
 *    pGuidSrc  [I] Address of device GUID.
 *    pGuidDest [O] Address to receive unique device GUID.
 *
 * RETURNS
 *    Success: DS_OK
 *    Failure: DSERR_INVALIDPARAM
 *
 * NOTES
 *    pGuidSrc is a valid device GUID or DSDEVID_DefaultPlayback,
 *    DSDEVID_DefaultCapture, DSDEVID_DefaultVoicePlayback, or 
 *    DSDEVID_DefaultVoiceCapture.
 *    Returns pGuidSrc if pGuidSrc is a valid device or the device
 *    GUID for the specified constants.
 */
HRESULT WINAPI GetDeviceID(LPCGUID pGuidSrc, LPGUID pGuidDest)
{
    if ( ( pGuidSrc == NULL) || (pGuidDest == NULL) ) {
	WARN("invalid parameter\n");
	return DSERR_INVALIDPARAM;
    }

    if ( IsEqualGUID( &DSDEVID_DefaultPlayback, pGuidSrc ) ||
    	IsEqualGUID( &DSDEVID_DefaultVoicePlayback, pGuidSrc ) ) {
	GUID guid;
	int err = mmErr(waveOutMessage((HWAVEOUT)ds_default_playback,DRV_QUERYDSOUNDGUID,(DWORD)&guid,0));
	if (err == DS_OK) {
	    memcpy(pGuidDest, &guid, sizeof(GUID));
	    return DS_OK;
	}
    }

    if ( IsEqualGUID( &DSDEVID_DefaultCapture, pGuidSrc ) ||
    	IsEqualGUID( &DSDEVID_DefaultVoiceCapture, pGuidSrc ) ) {
	GUID guid;
	int err = mmErr(waveInMessage((HWAVEIN)ds_default_capture,DRV_QUERYDSOUNDGUID,(DWORD)&guid,0));
	if (err == DS_OK) {
	    memcpy(pGuidDest, &guid, sizeof(GUID));
	    return DS_OK;
	}
    }

    memcpy(pGuidDest, pGuidSrc, sizeof(GUID));

    return DS_OK;
}


/***************************************************************************
 * DirectSoundEnumerateA [DSOUND.2]
 *
 * Enumerate all DirectSound drivers installed in the system
 *
 * PARAMS
 *    lpDSEnumCallback  [I] Address of callback function.
 *    lpContext         [I] Address of user defined context passed to callback function.
 *
 * RETURNS
 *    Success: DS_OK
 *    Failure: DSERR_INVALIDPARAM
 */
HRESULT WINAPI DirectSoundEnumerateA(
    LPDSENUMCALLBACKA lpDSEnumCallback,
    LPVOID lpContext)
{
    unsigned devs, wod;
    DSDRIVERDESC desc;
    GUID guid;
    int err;

    TRACE("lpDSEnumCallback = %p, lpContext = %p\n",
	lpDSEnumCallback, lpContext);

    if (lpDSEnumCallback == NULL) {
	WARN("invalid parameter: lpDSEnumCallback == NULL\n");
	return DSERR_INVALIDPARAM;
    }

    devs = waveOutGetNumDevs();
    if (devs > 0) {
	if (GetDeviceID(&DSDEVID_DefaultPlayback, &guid) == DS_OK) {
	    GUID temp;
	    for (wod = 0; wod < devs; ++wod) {
		err = mmErr(waveOutMessage((HWAVEOUT)wod,DRV_QUERYDSOUNDGUID,(DWORD)&temp,0));
		if (err == DS_OK) {
		    if (IsEqualGUID( &guid, &temp ) ) {
			err = mmErr(waveOutMessage((HWAVEOUT)wod,DRV_QUERYDSOUNDDESC,(DWORD)&desc,0));
			if (err == DS_OK) {
			    TRACE("calling lpDSEnumCallback(%s,\"%s\",\"%s\",%p)\n",
				debugstr_guid(&DSDEVID_DefaultPlayback),"Primary Sound Driver",desc.szDrvName,lpContext);
			    if (lpDSEnumCallback((LPGUID)&DSDEVID_DefaultPlayback, "Primary Sound Driver", desc.szDrvName, lpContext) == FALSE)
				return DS_OK;
			}
		    }
		}
	    }
	}
    }

    for (wod = 0; wod < devs; ++wod) {
	err = mmErr(waveOutMessage((HWAVEOUT)wod,DRV_QUERYDSOUNDDESC,(DWORD)&desc,0));
	if (err == DS_OK) {
	    err = mmErr(waveOutMessage((HWAVEOUT)wod,DRV_QUERYDSOUNDGUID,(DWORD)&guid,0));
	    if (err == DS_OK) {
		TRACE("calling lpDSEnumCallback(%s,\"%s\",\"%s\",%p)\n",
		    debugstr_guid(&guid),desc.szDesc,desc.szDrvName,lpContext);
		if (lpDSEnumCallback(&guid, desc.szDesc, desc.szDrvName, lpContext) == FALSE)
		    return DS_OK;
	    }
	}
    }
    return DS_OK;
}

/***************************************************************************
 * DirectSoundEnumerateW [DSOUND.3]
 *
 * Enumerate all DirectSound drivers installed in the system
 *
 * PARAMS
 *    lpDSEnumCallback  [I] Address of callback function.
 *    lpContext         [I] Address of user defined context passed to callback function.
 *
 * RETURNS
 *    Success: DS_OK
 *    Failure: DSERR_INVALIDPARAM
 */
HRESULT WINAPI DirectSoundEnumerateW(
	LPDSENUMCALLBACKW lpDSEnumCallback,
	LPVOID lpContext )
{
    unsigned devs, wod;
    DSDRIVERDESC desc;
    GUID guid;
    int err;
    WCHAR wDesc[MAXPNAMELEN];
    WCHAR wName[MAXPNAMELEN];

    TRACE("lpDSEnumCallback = %p, lpContext = %p\n",
	lpDSEnumCallback, lpContext);

    if (lpDSEnumCallback == NULL) {
	WARN("invalid parameter: lpDSEnumCallback == NULL\n");
	return DSERR_INVALIDPARAM;
    }

    devs = waveOutGetNumDevs();
    if (devs > 0) {
	if (GetDeviceID(&DSDEVID_DefaultPlayback, &guid) == DS_OK) {
	    GUID temp;
	    for (wod = 0; wod < devs; ++wod) {
		err = mmErr(waveOutMessage((HWAVEOUT)wod,DRV_QUERYDSOUNDGUID,(DWORD)&temp,0));
		if (err == DS_OK) {
		    if (IsEqualGUID( &guid, &temp ) ) {
			err = mmErr(waveOutMessage((HWAVEOUT)wod,DRV_QUERYDSOUNDDESC,(DWORD)&desc,0));
			if (err == DS_OK) {
			    TRACE("calling lpDSEnumCallback(%s,\"%s\",\"%s\",%p)\n",
				debugstr_guid(&DSDEVID_DefaultPlayback),"Primary Sound Driver",desc.szDrvName,lpContext);
			    MultiByteToWideChar( CP_ACP, 0, "Primary Sound Driver", -1,
				wDesc, sizeof(wDesc)/sizeof(WCHAR) );
				MultiByteToWideChar( CP_ACP, 0, desc.szDrvName, -1,
				wName, sizeof(wName)/sizeof(WCHAR) );
			    if (lpDSEnumCallback((LPGUID)&DSDEVID_DefaultPlayback, wDesc, wName, lpContext) == FALSE)
				return DS_OK;
			}
		    }
		}
	    }
	}
    }

    for (wod = 0; wod < devs; ++wod) {
	err = mmErr(waveOutMessage((HWAVEOUT)wod,DRV_QUERYDSOUNDDESC,(DWORD)&desc,0));
	if (err == DS_OK) {
	    err = mmErr(waveOutMessage((HWAVEOUT)wod,DRV_QUERYDSOUNDGUID,(DWORD)&guid,0));
	    if (err == DS_OK) {
		TRACE("calling lpDSEnumCallback(%s,\"%s\",\"%s\",%p)\n",
		    debugstr_guid(&guid),desc.szDesc,desc.szDrvName,lpContext);
		MultiByteToWideChar( CP_ACP, 0, desc.szDesc, -1,
		    wDesc, sizeof(wDesc)/sizeof(WCHAR) );
		    MultiByteToWideChar( CP_ACP, 0, desc.szDrvName, -1,
		    wName, sizeof(wName)/sizeof(WCHAR) );
		if (lpDSEnumCallback(&guid, wDesc, wName, lpContext) == FALSE)
		    return DS_OK;
	    }
	}
    }
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
 *		IDirectSound
 */

static HRESULT WINAPI IDirectSoundImpl_SetCooperativeLevel(
	LPDIRECTSOUND8 iface,HWND hwnd,DWORD level
) {
	ICOM_THIS(IDirectSoundImpl,iface);

	FIXME("(%p,%08lx,%ld):stub\n",This,(DWORD)hwnd,level);

	This->priolevel = level;

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundImpl_CreateSoundBuffer(
	LPDIRECTSOUND8 iface,LPDSBUFFERDESC dsbd,LPLPDIRECTSOUNDBUFFER8 ppdsb,LPUNKNOWN lpunk
) {
	ICOM_THIS(IDirectSoundImpl,iface);
	LPWAVEFORMATEX	wfex;

	TRACE("(%p,%p,%p,%p)\n",This,dsbd,ppdsb,lpunk);

	if ((This == NULL) || (dsbd == NULL) || (ppdsb == NULL))
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

	if (dsbd->dwFlags & DSBCAPS_PRIMARYBUFFER)
		return PrimaryBuffer_Create(This, (PrimaryBufferImpl**)ppdsb, dsbd);
	else
		return SecondaryBuffer_Create(This, (IDirectSoundBufferImpl**)ppdsb, dsbd);
}

static HRESULT WINAPI IDirectSoundImpl_DuplicateSoundBuffer(
	LPDIRECTSOUND8 iface,LPDIRECTSOUNDBUFFER8 pdsb,LPLPDIRECTSOUNDBUFFER8 ppdsb
) {
	ICOM_THIS(IDirectSoundImpl,iface);
	IDirectSoundBufferImpl* ipdsb=(IDirectSoundBufferImpl*)pdsb;
	IDirectSoundBufferImpl** ippdsb=(IDirectSoundBufferImpl**)ppdsb;
	TRACE("(%p,%p,%p)\n",This,ipdsb,ippdsb);

	if (ipdsb->dsbd.dwFlags & DSBCAPS_PRIMARYBUFFER) {
		ERR("trying to duplicate primary buffer\n");
		return DSERR_INVALIDCALL;
	}

	if (ipdsb->hwbuf) {
		FIXME("need to duplicate hardware buffer\n");
	}

	if (ipdsb->dsbd.dwFlags & DSBCAPS_CTRL3D) {
		FIXME("need to duplicate 3D buffer\n");
	}

	*ippdsb = (IDirectSoundBufferImpl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectSoundBufferImpl));

	IDirectSoundBuffer8_AddRef(pdsb);
	memcpy(*ippdsb, ipdsb, sizeof(IDirectSoundBufferImpl));
	(*ippdsb)->ref = 1;
	(*ippdsb)->state = STATE_STOPPED;
	(*ippdsb)->playpos = 0;
	(*ippdsb)->buf_mixpos = 0;
	(*ippdsb)->dsound = This;
	(*ippdsb)->parent = ipdsb;
	(*ippdsb)->hwbuf = NULL;
	(*ippdsb)->ds3db = NULL; /* FIXME? */
	(*ippdsb)->iks = NULL; /* FIXME? */
	memcpy(&((*ippdsb)->wfx), &(ipdsb->wfx), sizeof((*ippdsb)->wfx));
	InitializeCriticalSection(&(*ippdsb)->lock);
	/* register buffer */
	RtlAcquireResourceExclusive(&(This->lock), TRUE);
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
	RtlReleaseResource(&(This->lock));
	IDirectSound_AddRef(iface);
	return DS_OK;
}


static HRESULT WINAPI IDirectSoundImpl_GetCaps(LPDIRECTSOUND8 iface,LPDSCAPS caps) {
	ICOM_THIS(IDirectSoundImpl,iface);
	TRACE("(%p,%p)\n",This,caps);

	if (caps == NULL || caps->dwSize!=sizeof(*caps))
		return DSERR_INVALIDPARAM;

	caps->dwFlags = This->drvcaps.dwFlags;
	TRACE("(flags=0x%08lx)\n",caps->dwFlags);

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

static ULONG WINAPI IDirectSoundImpl_AddRef(LPDIRECTSOUND8 iface) {
	ICOM_THIS(IDirectSoundImpl,iface);
	return ++(This->ref);
}

static ULONG WINAPI IDirectSoundImpl_Release(LPDIRECTSOUND8 iface) {
	ICOM_THIS(IDirectSoundImpl,iface);
	TRACE("(%p), ref was %ld\n",This,This->ref);
	if (!--(This->ref)) {
		UINT i;

		timeKillEvent(This->timerID);
		timeEndPeriod(DS_TIME_RES);

		if (This->buffers) {
			for( i=0;i<This->nrofbuffers;i++)
				IDirectSoundBuffer8_Release((LPDIRECTSOUNDBUFFER8)This->buffers[i]);
		}

		DSOUND_PrimaryDestroy(This);

		RtlDeleteResource(&This->lock);
		DeleteCriticalSection(&This->mixlock);
		if (This->driver) {
			IDsDriver_Close(This->driver);
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
	LPDIRECTSOUND8 iface,DWORD config
) {
	ICOM_THIS(IDirectSoundImpl,iface);
	FIXME("(%p,0x%08lx):stub\n",This,config);
	return DS_OK;
}

static HRESULT WINAPI IDirectSoundImpl_QueryInterface(
	LPDIRECTSOUND8 iface,REFIID riid,LPVOID *ppobj
) {
	ICOM_THIS(IDirectSoundImpl,iface);

	if ( IsEqualGUID( &IID_IDirectSound3DListener, riid ) ) {
		ERR("app requested IDirectSound3DListener on dsound object\n");
		*ppobj = NULL;
		return E_FAIL;
	}

	FIXME("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

static HRESULT WINAPI IDirectSoundImpl_Compact(
	LPDIRECTSOUND8 iface)
{
	ICOM_THIS(IDirectSoundImpl,iface);
	TRACE("(%p)\n", This);
	return DS_OK;
}

static HRESULT WINAPI IDirectSoundImpl_GetSpeakerConfig(
	LPDIRECTSOUND8 iface,
	LPDWORD lpdwSpeakerConfig)
{
	ICOM_THIS(IDirectSoundImpl,iface);
	TRACE("(%p, %p)\n", This, lpdwSpeakerConfig);
	*lpdwSpeakerConfig = DSSPEAKER_STEREO | (DSSPEAKER_GEOMETRY_NARROW << 16);
	return DS_OK;
}

static HRESULT WINAPI IDirectSoundImpl_Initialize(
	LPDIRECTSOUND8 iface,
	LPCGUID lpcGuid)
{
	ICOM_THIS(IDirectSoundImpl,iface);
	TRACE("(%p, %p)\n", This, lpcGuid);
	return DS_OK;
}

static HRESULT WINAPI IDirectSoundImpl_VerifyCertification(
	LPDIRECTSOUND8 iface,
	LPDWORD pdwCertified)
{
	ICOM_THIS(IDirectSoundImpl,iface);
	TRACE("(%p, %p)\n", This, pdwCertified);
	*pdwCertified = DS_CERTIFIED;
	return DS_OK;
}

static ICOM_VTABLE(IDirectSound8) dsvt =
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
	IDirectSoundImpl_Initialize,
	IDirectSoundImpl_VerifyCertification
};


/*******************************************************************************
 *		DirectSoundCreate (DSOUND.1)
 *
 *  Creates and initializes a DirectSound interface.
 *
 *  PARAMS
 *     lpcGUID   [I] Address of the GUID that identifies the sound device.
 *     ppDS      [O] Address of a variable to receive the interface pointer.
 *     pUnkOuter [I] Must be NULL.
 *
 *  RETURNS
 *     Success: DS_OK
 *     Failure: DSERR_ALLOCATED, DSERR_INVALIDPARAM, DSERR_NOAGGREGATION, 
 *              DSERR_NODRIVER, DSERR_OUTOFMEMORY
 */
HRESULT WINAPI DirectSoundCreate8(LPCGUID lpcGUID,LPDIRECTSOUND8 *ppDS,IUnknown *pUnkOuter )
{
	IDirectSoundImpl** ippDS=(IDirectSoundImpl**)ppDS;
	PIDSDRIVER drv = NULL;
	unsigned wod, wodn;
	HRESULT err = DSERR_INVALIDPARAM;
	GUID devGuid;
	TRACE("(%p,%p,%p)\n",lpcGUID,ippDS,pUnkOuter);

	if (ippDS == NULL) {
		WARN("invalid parameter\n");
		return DSERR_INVALIDPARAM;
	}

        /* Get dsound configuration */
        setup_dsound_options();

	/* Default device? */
	if ( !lpcGUID || IsEqualGUID(lpcGUID, &GUID_NULL) )
		lpcGUID = &DSDEVID_DefaultPlayback;

	if (GetDeviceID(lpcGUID, &devGuid) != DS_OK) {
		WARN("invalid parameter\n");
		return DSERR_INVALIDPARAM;
	}

	if (dsound) {
		if (IsEqualGUID(&devGuid, &dsound->guid) ) {
			ERR("dsound already opened\n");
			IDirectSound_AddRef((LPDIRECTSOUND)dsound);
			*ippDS = dsound;
			return DS_OK;
		} else {
			ERR("different dsound already opened\n");
		}
	}

	/* Enumerate WINMM audio devices and find the one we want */
	wodn = waveOutGetNumDevs();
	if (!wodn) return DSERR_NODRIVER;

	for (wod=0; wod<wodn; wod++) {
		GUID guid;
		err = mmErr(waveOutMessage((HWAVEOUT)wod,DRV_QUERYDSOUNDGUID,(DWORD)(&guid),0));
		if (err != DS_OK) {
			WARN("waveOutMessage failed; err=%lx\n",err);
			return err;
		}
		if (IsEqualGUID( &devGuid, &guid) ) {
			err = DS_OK;
			break;
		}
	}

	if (err != DS_OK) {
		WARN("invalid parameter\n");
		return DSERR_INVALIDPARAM;
	}

	/* DRV_QUERYDSOUNDIFACE is a "Wine extension" to get the DSound interface */
	waveOutMessage((HWAVEOUT)wod, DRV_QUERYDSOUNDIFACE, (DWORD)&drv, 0);

	/* Disable the direct sound driver to force emulation if requested. */
	if (ds_hw_accel == DS_HW_ACCEL_EMULATION)
	    drv = NULL;
	
	/* Allocate memory */
	*ippDS = (IDirectSoundImpl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectSoundImpl));
	if (*ippDS == NULL)
		return DSERR_OUTOFMEMORY;

	(*ippDS)->lpVtbl	= &dsvt;
	(*ippDS)->ref		= 1;

	(*ippDS)->driver	= drv;
	(*ippDS)->priolevel	= DSSCL_NORMAL;
	(*ippDS)->fraglen	= 0;
	(*ippDS)->hwbuf		= NULL;
	(*ippDS)->buffer	= NULL;
	(*ippDS)->buflen	= 0;
	(*ippDS)->writelead	= 0;
	(*ippDS)->state		= STATE_STOPPED;
	(*ippDS)->nrofbuffers	= 0;
	(*ippDS)->buffers	= NULL;
	(*ippDS)->listener	= NULL;

	(*ippDS)->prebuf	= ds_snd_queue_max;
	(*ippDS)->guid		= devGuid;

	/* Get driver description */
	if (drv) {
		IDsDriver_GetDriverDesc(drv,&((*ippDS)->drvdesc));
	} else {
		/* if no DirectSound interface available, use WINMM API instead */
		(*ippDS)->drvdesc.dwFlags = DSDDESC_DOMMSYSTEMOPEN | DSDDESC_DOMMSYSTEMSETFORMAT;
	}

	(*ippDS)->drvdesc.dnDevNode = wod;

	/* Set default wave format (may need it for waveOutOpen) */
	(*ippDS)->wfx.wFormatTag	= WAVE_FORMAT_PCM;
        /* We rely on the sound driver to return the actual sound format of 
         * the device if it does not support 22050x8x2 and is given the 
         * WAVE_DIRECTSOUND flag.
         */
        (*ippDS)->wfx.nSamplesPerSec = 22050;
        (*ippDS)->wfx.wBitsPerSample = 8;
        (*ippDS)->wfx.nChannels = 2;
        (*ippDS)->wfx.nBlockAlign = (*ippDS)->wfx.wBitsPerSample * (*ippDS)->wfx.nChannels / 8;
        (*ippDS)->wfx.nAvgBytesPerSec = (*ippDS)->wfx.nSamplesPerSec * (*ippDS)->wfx.nBlockAlign;
        (*ippDS)->wfx.cbSize = 0;

	/* If the driver requests being opened through MMSYSTEM
	 * (which is recommended by the DDK), it is supposed to happen
	 * before the DirectSound interface is opened */
	if ((*ippDS)->drvdesc.dwFlags & DSDDESC_DOMMSYSTEMOPEN)
	{
		DWORD flags = CALLBACK_FUNCTION;

		/* disable direct sound if requested */
		if (ds_hw_accel != DS_HW_ACCEL_EMULATION)
		    flags |= WAVE_DIRECTSOUND;

                err = mmErr(waveOutOpen(&((*ippDS)->hwo),
					  (*ippDS)->drvdesc.dnDevNode, &((*ippDS)->wfx),
					  (DWORD)DSOUND_callback, (DWORD)(*ippDS),
					  flags));
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
		/* FIXME: We should check the device capabilities */
		(*ippDS)->drvcaps.dwFlags =
			DSCAPS_PRIMARY16BIT | DSCAPS_PRIMARYSTEREO;
		if (ds_emuldriver)
		    (*ippDS)->drvcaps.dwFlags |= DSCAPS_EMULDRIVER;
	}

	DSOUND_RecalcVolPan(&((*ippDS)->volpan));

	InitializeCriticalSection(&((*ippDS)->mixlock));
	RtlInitializeResource(&((*ippDS)->lock));

	if (!dsound) {
		dsound = (*ippDS);
		DSOUND_PrimaryCreate(dsound);
		timeBeginPeriod(DS_TIME_RES);
		dsound->timerID = timeSetEvent(DS_TIME_DEL, DS_TIME_RES, DSOUND_timer,
					       (DWORD)dsound, TIME_PERIODIC | TIME_CALLBACK_FUNCTION);
	}
	return DS_OK;
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
	if ( IsEqualGUID( &IID_IDirectSound, riid ) ||
	     IsEqualGUID( &IID_IDirectSound8, riid ) ) {
		/* FIXME: reuse already created dsound if present? */
		return DirectSoundCreate8(riid,(LPDIRECTSOUND8*)ppobj,pOuter);
	}
	if ( IsEqualGUID( &IID_IDirectSoundCapture, riid ) ||
	     IsEqualGUID( &IID_IDirectSoundCapture8, riid ) ) {
		return DirectSoundCaptureCreate8(0,(LPDIRECTSOUNDCAPTURE8*)ppobj,pOuter);
	}
	if ( IsEqualGUID( &IID_IKsPropertySet, riid ) ) {
		return IKsPropertySetImpl_Create(0,(IKsPropertySetImpl**)ppobj);
	}

	FIXME("(%p,%p,%s,%p) Interface not found!\n",This,pOuter,debugstr_guid(riid),ppobj);	
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
 * DllCanUnloadNow [DSOUND.4]  
 * Determines whether the DLL is in use.
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
