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
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define NONAMELESSSTRUCT
#define NONAMELESSUNION
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
#include "dsconf.h"

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
    TRACE("(%p,%p)\n",pGuidSrc,pGuidDest);

    if ( pGuidSrc == NULL) {
	WARN("invalid parameter: pGuidSrc == NULL\n");
	return DSERR_INVALIDPARAM;
    }

    if ( pGuidDest == NULL ) {
	WARN("invalid parameter: pGuidDest == NULL\n");
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
			    if (lpDSEnumCallback(NULL, "Primary Sound Driver", desc.szDrvName, lpContext) == FALSE)
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
			    if (lpDSEnumCallback(NULL, wDesc, wName, lpContext) == FALSE)
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
	TRACE("(%p,%08lx,%ld)\n",This,(DWORD)hwnd,level);

	if (level==DSSCL_PRIORITY || level==DSSCL_EXCLUSIVE) {
		FIXME("level=%s not fully supported\n",
			level==DSSCL_PRIORITY ? "DSSCL_PRIORITY" : "DSSCL_EXCLUSIVE");
	}

	This->priolevel = level;

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundImpl_CreateSoundBuffer(
	LPDIRECTSOUND8 iface,LPDSBUFFERDESC dsbd,LPLPDIRECTSOUNDBUFFER8 ppdsb,LPUNKNOWN lpunk
) {
	ICOM_THIS(IDirectSoundImpl,iface);
	LPWAVEFORMATEX	wfex;
	HRESULT hres = DS_OK;

	TRACE("(%p,%p,%p,%p)\n",This,dsbd,ppdsb,lpunk);

	if (This == NULL) {
		WARN("invalid parameter: This == NULL\n");
		return DSERR_INVALIDPARAM;
	}

	if (dsbd == NULL) {
		WARN("invalid parameter: dsbd == NULL\n");
		return DSERR_INVALIDPARAM;
	}

	if (ppdsb == NULL) {
		WARN("invalid parameter: ppdsb == NULL\n");
		return DSERR_INVALIDPARAM;
	}

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
		if (This->primary) {
			WARN("Primary Buffer already created\n");
			IDirectSoundBuffer_AddRef((LPDIRECTSOUNDBUFFER8)(This->primary));
			*ppdsb = (LPDIRECTSOUNDBUFFER8)(This->primary);
		} else {
			This->dsbd = *dsbd;
			hres = PrimaryBufferImpl_Create(This, (PrimaryBufferImpl**)&(This->primary), &(This->dsbd));
			if (This->primary) {
				IDirectSoundBuffer_AddRef((LPDIRECTSOUNDBUFFER8)(This->primary));
				*ppdsb = (LPDIRECTSOUNDBUFFER8)(This->primary);
			} else 
				WARN("PrimaryBufferImpl_Create failed\n");
		}
	} else {
		IDirectSoundBufferImpl * dsb;
		hres = IDirectSoundBufferImpl_Create(This, (IDirectSoundBufferImpl**)&dsb, dsbd);
		if (dsb) {
			hres = SecondaryBufferImpl_Create(dsb, (SecondaryBufferImpl**)ppdsb);
			if (*ppdsb) {
				dsb->dsb = (SecondaryBufferImpl*)*ppdsb;
				IDirectSoundBuffer_AddRef((LPDIRECTSOUNDBUFFER8)*ppdsb);
			} else
				WARN("SecondaryBufferImpl_Create failed\n");
		} else 
			WARN("IDirectSoundBufferImpl_Create failed\n");
	}

	return hres;
}

static HRESULT WINAPI IDirectSoundImpl_DuplicateSoundBuffer(
	LPDIRECTSOUND8 iface,LPDIRECTSOUNDBUFFER8 psb,LPLPDIRECTSOUNDBUFFER8 ppdsb
) {
	ICOM_THIS(IDirectSoundImpl,iface);
	IDirectSoundBufferImpl* pdsb;
	IDirectSoundBufferImpl* dsb;
	HRESULT hres = DS_OK;
	TRACE("(%p,%p,%p)\n",This,psb,ppdsb);

	if (This == NULL) {
		WARN("invalid parameter: This == NULL\n");
		return DSERR_INVALIDPARAM;
	}

	if (psb == NULL) {
		WARN("invalid parameter: psb == NULL\n");
		return DSERR_INVALIDPARAM;
	}

	if (ppdsb == NULL) {
		WARN("invalid parameter: ppdsb == NULL\n");
		return DSERR_INVALIDPARAM;
	}

	/* FIXME: hack to make sure we have a secondary buffer */
	if ((DWORD)((SecondaryBufferImpl *)psb)->dsb == (DWORD)This) {
		ERR("trying to duplicate primary buffer\n");
		*ppdsb = NULL;
		return DSERR_INVALIDCALL;
	}

	pdsb = ((SecondaryBufferImpl *)psb)->dsb;

	if (pdsb->hwbuf) {
		FIXME("need to duplicate hardware buffer\n");
		*ppdsb = NULL;
		return DSERR_INVALIDCALL;
	}

	dsb = (IDirectSoundBufferImpl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*dsb));

	if (dsb == NULL) {
		WARN("out of memory\n");
		*ppdsb = NULL;
		return DSERR_OUTOFMEMORY;
	}

	memcpy(dsb, pdsb, sizeof(IDirectSoundBufferImpl));
	dsb->ref = 0;
	dsb->state = STATE_STOPPED;
	dsb->playpos = 0;
	dsb->buf_mixpos = 0;
	dsb->dsound = This;
	dsb->buffer->ref++;
	dsb->hwbuf = NULL;
	dsb->ds3db = NULL;
	dsb->iks = NULL; /* FIXME? */
	dsb->dsb = NULL;
	memcpy(&(dsb->wfx), &(pdsb->wfx), sizeof(dsb->wfx));
	InitializeCriticalSection(&(dsb->lock));
	/* register buffer */
	RtlAcquireResourceExclusive(&(This->lock), TRUE);
	{
		IDirectSoundBufferImpl **newbuffers = (IDirectSoundBufferImpl**)HeapReAlloc(GetProcessHeap(),0,This->buffers,sizeof(IDirectSoundBufferImpl**)*(This->nrofbuffers+1));
		if (newbuffers) {
			This->buffers = newbuffers;
			This->buffers[This->nrofbuffers] = dsb;
			This->nrofbuffers++;
			TRACE("buffer count is now %d\n", This->nrofbuffers);
		} else {
			ERR("out of memory for buffer list! Current buffer count is %d\n", This->nrofbuffers);
			IDirectSoundBuffer8_Release(psb);
			DeleteCriticalSection(&(dsb->lock));
			RtlReleaseResource(&(This->lock));
			HeapFree(GetProcessHeap(),0,dsb);
			*ppdsb = 0;
			return DSERR_OUTOFMEMORY;
		}
	}
	RtlReleaseResource(&(This->lock));
	IDirectSound_AddRef(iface);
	hres = SecondaryBufferImpl_Create(dsb, (SecondaryBufferImpl**)ppdsb);
	if (*ppdsb) {
		dsb->dsb = (SecondaryBufferImpl*)*ppdsb;
		IDirectSoundBuffer_AddRef((LPDIRECTSOUNDBUFFER8)*ppdsb);
	} else
		WARN("SecondaryBufferImpl_Create failed\n");

	return hres;
}

static HRESULT WINAPI IDirectSoundImpl_GetCaps(LPDIRECTSOUND8 iface,LPDSCAPS lpDSCaps) {
	ICOM_THIS(IDirectSoundImpl,iface);
	TRACE("(%p,%p)\n",This,lpDSCaps);

	if (This == NULL) {
		WARN("invalid parameter: This == NULL\n");
		return DSERR_INVALIDPARAM;
	}

	if (lpDSCaps == NULL) {
		WARN("invalid parameter: lpDSCaps = NULL\n");
		return DSERR_INVALIDPARAM;
	}

	/* check is there is enough room */
	if (lpDSCaps->dwSize < sizeof(*lpDSCaps)) {
		WARN("invalid parameter: lpDSCaps->dwSize = %ld < %d\n",
			lpDSCaps->dwSize, sizeof(*lpDSCaps));
		return DSERR_INVALIDPARAM;
	}

	lpDSCaps->dwFlags                               = This->drvcaps.dwFlags;
	TRACE("(flags=0x%08lx)\n",lpDSCaps->dwFlags);

	lpDSCaps->dwMinSecondarySampleRate		= This->drvcaps.dwMinSecondarySampleRate;
	lpDSCaps->dwMaxSecondarySampleRate		= This->drvcaps.dwMaxSecondarySampleRate;

	lpDSCaps->dwPrimaryBuffers			= This->drvcaps.dwPrimaryBuffers;

	lpDSCaps->dwMaxHwMixingAllBuffers		= This->drvcaps.dwMaxHwMixingAllBuffers;
	lpDSCaps->dwMaxHwMixingStaticBuffers		= This->drvcaps.dwMaxHwMixingStaticBuffers;
	lpDSCaps->dwMaxHwMixingStreamingBuffers		= This->drvcaps.dwMaxHwMixingStreamingBuffers;

	lpDSCaps->dwFreeHwMixingAllBuffers		= This->drvcaps.dwFreeHwMixingAllBuffers;
	lpDSCaps->dwFreeHwMixingStaticBuffers		= This->drvcaps.dwFreeHwMixingStaticBuffers;
	lpDSCaps->dwFreeHwMixingStreamingBuffers	= This->drvcaps.dwFreeHwMixingStreamingBuffers;

	lpDSCaps->dwMaxHw3DAllBuffers			= This->drvcaps.dwMaxHw3DAllBuffers;
	lpDSCaps->dwMaxHw3DStaticBuffers		= This->drvcaps.dwMaxHw3DStaticBuffers;
	lpDSCaps->dwMaxHw3DStreamingBuffers		= This->drvcaps.dwMaxHw3DStreamingBuffers;

	lpDSCaps->dwFreeHw3DAllBuffers			= This->drvcaps.dwFreeHw3DAllBuffers;
	lpDSCaps->dwFreeHw3DStaticBuffers		= This->drvcaps.dwFreeHw3DStaticBuffers;
	lpDSCaps->dwFreeHw3DStreamingBuffers		= This->drvcaps.dwFreeHw3DStreamingBuffers;

	lpDSCaps->dwTotalHwMemBytes			= This->drvcaps.dwTotalHwMemBytes;

	lpDSCaps->dwFreeHwMemBytes			= This->drvcaps.dwFreeHwMemBytes;

	lpDSCaps->dwMaxContigFreeHwMemBytes		= This->drvcaps.dwMaxContigFreeHwMemBytes;

	/* driver doesn't have these */
	lpDSCaps->dwUnlockTransferRateHwBuffers		= 4096;	/* But we have none... */

	lpDSCaps->dwPlayCpuOverheadSwBuffers		= 1;	/* 1% */

	return DS_OK;
}

static ULONG WINAPI IDirectSoundImpl_AddRef(LPDIRECTSOUND8 iface) {
	ICOM_THIS(IDirectSoundImpl,iface);
	TRACE("(%p) ref was %ld, thread is %04lx\n", This, This->ref, GetCurrentThreadId());
	return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IDirectSoundImpl_Release(LPDIRECTSOUND8 iface) {
	ICOM_THIS(IDirectSoundImpl,iface);
	ULONG ulReturn;

	TRACE("(%p) ref was %ld, thread is %04lx\n", This, This->ref, GetCurrentThreadId());
	ulReturn = InterlockedDecrement(&This->ref);
	if (ulReturn == 0) {
		HRESULT hres;
		UINT i;

		timeKillEvent(This->timerID);
		timeEndPeriod(DS_TIME_RES);
		/* wait for timer to expire */
		Sleep(DS_TIME_RES+1);

		RtlAcquireResourceShared(&(This->lock), TRUE);

		if (This->buffers) {
			for( i=0;i<This->nrofbuffers;i++)
				IDirectSoundBuffer8_Release((LPDIRECTSOUNDBUFFER8)This->buffers[i]);
		}

		RtlReleaseResource(&(This->lock));

		if (This->primary) {
			WARN("primary buffer not released\n");
			IDirectSoundBuffer8_Release((LPDIRECTSOUNDBUFFER8)This->primary);
		}

		hres = DSOUND_PrimaryDestroy(This);
		if (hres != DS_OK)
			WARN("DSOUND_PrimaryDestroy failed\n");

		if (This->driver)
			IDsDriver_Close(This->driver);

		if (This->drvdesc.dwFlags & DSDDESC_DOMMSYSTEMOPEN)
			waveOutClose(This->hwo);

		if (This->driver)
			IDsDriver_Release(This->driver);

		RtlDeleteResource(&This->lock);
		DeleteCriticalSection(&This->mixlock);
		DeleteCriticalSection(&This->ds3dl_lock);
		HeapFree(GetProcessHeap(),0,This);
		dsound = NULL;
		TRACE("(%p) released\n",This);
	}

	return ulReturn;
}

static HRESULT WINAPI IDirectSoundImpl_SetSpeakerConfig(
	LPDIRECTSOUND8 iface,DWORD config
) {
	ICOM_THIS(IDirectSoundImpl,iface);
	TRACE("(%p,0x%08lx)\n",This,config);

	This->speaker_config = config;

	WARN("not fully functional\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSoundImpl_QueryInterface(
	LPDIRECTSOUND8 iface,REFIID riid,LPVOID *ppobj
) {
	ICOM_THIS(IDirectSoundImpl,iface);
	TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);

	if (ppobj == NULL) {
		WARN("invalid parameter\n");
		return E_INVALIDARG;
	}

	*ppobj = NULL;	/* assume failure */

	if ( IsEqualGUID(riid, &IID_IUnknown) || 
	     IsEqualGUID(riid, &IID_IDirectSound) || 
	     IsEqualGUID(riid, &IID_IDirectSound8) ) {
		IDirectSound8_AddRef((LPDIRECTSOUND8)This);
		*ppobj = This;
		return S_OK;
	}

	if ( IsEqualGUID( &IID_IDirectSound3DListener, riid ) ) {
		WARN("app requested IDirectSound3DListener on dsound object\n");
		return E_NOINTERFACE;
	}

	FIXME( "Unknown IID %s\n", debugstr_guid( riid ) );
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

	if (lpdwSpeakerConfig == NULL) {
		WARN("invalid parameter\n");
		return DSERR_INVALIDPARAM;
	}

	WARN("not fully functional\n");

	*lpdwSpeakerConfig = This->speaker_config;

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundImpl_Initialize(
	LPDIRECTSOUND8 iface,
	LPCGUID lpcGuid)
{
	ICOM_THIS(IDirectSoundImpl,iface);
	TRACE("(%p, %s)\n", This, debugstr_guid(lpcGuid));
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
	BOOLEAN found = FALSE;

	TRACE("(%s,%p,%p)\n",debugstr_guid(lpcGUID),ippDS,pUnkOuter);
	
	if (ippDS == NULL) {
		WARN("invalid parameter: ippDS == NULL\n");
		return DSERR_INVALIDPARAM;
	}

        /* Get dsound configuration */
        setup_dsound_options();

	/* Default device? */
	if (!lpcGUID || IsEqualGUID(lpcGUID, &GUID_NULL))
		lpcGUID = &DSDEVID_DefaultPlayback;

	if (GetDeviceID(lpcGUID, &devGuid) != DS_OK) {
		WARN("invalid parameter: lpcGUID\n");
		*ippDS = NULL;
		return DSERR_INVALIDPARAM;
	}

	if (dsound) {
		if (IsEqualGUID(&devGuid, &dsound->guid) ) {
			/* FIXME: this is wrong, need to create a new instance */
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
	if (!wodn) {
		WARN("no driver\n");
		*ippDS = NULL;
	       	return DSERR_NODRIVER;
	}

	TRACE(" expecting GUID %s.\n", debugstr_guid(&devGuid));
	
	for (wod=0; wod<wodn; wod++) {
		GUID guid;
		err = mmErr(waveOutMessage((HWAVEOUT)wod,DRV_QUERYDSOUNDGUID,(DWORD)(&guid),0));
		if (err != DS_OK) {
			WARN("waveOutMessage failed; err=%lx\n",err);
			*ippDS = NULL;
			return err;
		}
		TRACE("got GUID %s for wod %d.\n", debugstr_guid(&guid), wod);
		if (IsEqualGUID( &devGuid, &guid) ) {
			err = DS_OK;
			found = TRUE;
			break;
		}
	}

	if (err != DS_OK) {
		WARN("invalid parameter\n");
		*ippDS = NULL;
		return DSERR_INVALIDPARAM;
	}

	if (found == FALSE) {
		WARN("No device found matching given ID - trying with default one !\n");
		wod = ds_default_playback;	
	}
	
	/* DRV_QUERYDSOUNDIFACE is a "Wine extension" to get the DSound interface */
	waveOutMessage((HWAVEOUT)wod, DRV_QUERYDSOUNDIFACE, (DWORD)&drv, 0);

	/* Disable the direct sound driver to force emulation if requested. */
	if (ds_hw_accel == DS_HW_ACCEL_EMULATION)
	    drv = NULL;
	
	/* Allocate memory */
	*ippDS = (IDirectSoundImpl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectSoundImpl));
	if (*ippDS == NULL) {
		WARN("out of memory\n");
		return DSERR_OUTOFMEMORY;
	}

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
	(*ippDS)->primary	= NULL;
	(*ippDS)->speaker_config = DSSPEAKER_STEREO | (DSSPEAKER_GEOMETRY_NARROW << 16);
	
	/* 3D listener initial parameters */
	(*ippDS)->listener	= NULL;
	(*ippDS)->ds3dl.dwSize = sizeof(DS3DLISTENER);
	(*ippDS)->ds3dl.vPosition.x = 0.0;
	(*ippDS)->ds3dl.vPosition.y = 0.0;
	(*ippDS)->ds3dl.vPosition.z = 0.0;
	(*ippDS)->ds3dl.vVelocity.x = 0.0;
	(*ippDS)->ds3dl.vVelocity.y = 0.0;
	(*ippDS)->ds3dl.vVelocity.z = 0.0;
	(*ippDS)->ds3dl.vOrientFront.x = 0.0;
	(*ippDS)->ds3dl.vOrientFront.y = 0.0;
	(*ippDS)->ds3dl.vOrientFront.z = 1.0;
	(*ippDS)->ds3dl.vOrientTop.x = 0.0;
	(*ippDS)->ds3dl.vOrientTop.y = 1.0;
	(*ippDS)->ds3dl.vOrientTop.z = 0.0;
	(*ippDS)->ds3dl.flDistanceFactor = DS3D_DEFAULTDISTANCEFACTOR;
	(*ippDS)->ds3dl.flRolloffFactor = DS3D_DEFAULTROLLOFFFACTOR;
	(*ippDS)->ds3dl.flDopplerFactor = DS3D_DEFAULTDOPPLERFACTOR;

	InitializeCriticalSection(&(*ippDS)->ds3dl_lock);

	(*ippDS)->prebuf	= ds_snd_queue_max;
	(*ippDS)->guid		= devGuid;

	/* Get driver description */
	if (drv) {
		err = IDsDriver_GetDriverDesc(drv,&((*ippDS)->drvdesc));
		if (err != DS_OK) {
			WARN("IDsDriver_GetDriverDesc failed\n");
			HeapFree(GetProcessHeap(),0,*ippDS);
			*ippDS = NULL;
			return err;
		}
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
		if (err != DS_OK) {
			WARN("waveOutOpen failed\n");
			HeapFree(GetProcessHeap(),0,*ippDS);
			*ippDS = NULL;
			return err;
		}
	}

	if (drv) {
		err = IDsDriver_Open(drv);
		if (err != DS_OK) {
			WARN("IDsDriver_Open failed\n");
			HeapFree(GetProcessHeap(),0,*ippDS);
			*ippDS = NULL;
			return err;
		}

		/* the driver is now open, so it's now allowed to call GetCaps */
		err = IDsDriver_GetCaps(drv,&((*ippDS)->drvcaps));
		if (err != DS_OK) {
			WARN("IDsDriver_GetCaps failed\n");
			HeapFree(GetProcessHeap(),0,*ippDS);
			*ippDS = NULL;
			return err;
		}
	} else {
		WAVEOUTCAPSA woc;
		err = mmErr(waveOutGetDevCapsA((*ippDS)->drvdesc.dnDevNode, &woc, sizeof(woc)));
		if (err != DS_OK) {
			WARN("waveOutGetDevCaps failed\n");
			HeapFree(GetProcessHeap(),0,*ippDS);
			*ippDS = NULL;
			return err;
		}
		ZeroMemory(&(*ippDS)->drvcaps, sizeof((*ippDS)->drvcaps));
		if ((woc.dwFormats & WAVE_FORMAT_1M08) ||
		    (woc.dwFormats & WAVE_FORMAT_2M08) ||
		    (woc.dwFormats & WAVE_FORMAT_4M08) ||
		    (woc.dwFormats & WAVE_FORMAT_48M08) ||
		    (woc.dwFormats & WAVE_FORMAT_96M08)) {
			(*ippDS)->drvcaps.dwFlags |= DSCAPS_PRIMARY8BIT;
			(*ippDS)->drvcaps.dwFlags |= DSCAPS_PRIMARYMONO;
		}
		if ((woc.dwFormats & WAVE_FORMAT_1M16) ||
		    (woc.dwFormats & WAVE_FORMAT_2M16) ||
		    (woc.dwFormats & WAVE_FORMAT_4M16) ||
		    (woc.dwFormats & WAVE_FORMAT_48M16) ||
		    (woc.dwFormats & WAVE_FORMAT_96M16)) {
			(*ippDS)->drvcaps.dwFlags |= DSCAPS_PRIMARY16BIT;
			(*ippDS)->drvcaps.dwFlags |= DSCAPS_PRIMARYMONO;
		}
		if ((woc.dwFormats & WAVE_FORMAT_1S08) ||
		    (woc.dwFormats & WAVE_FORMAT_2S08) ||
		    (woc.dwFormats & WAVE_FORMAT_4S08) ||
		    (woc.dwFormats & WAVE_FORMAT_48S08) ||
		    (woc.dwFormats & WAVE_FORMAT_96S08)) {
			(*ippDS)->drvcaps.dwFlags |= DSCAPS_PRIMARY8BIT;
			(*ippDS)->drvcaps.dwFlags |= DSCAPS_PRIMARYSTEREO;
		}
		if ((woc.dwFormats & WAVE_FORMAT_1S16) ||
		    (woc.dwFormats & WAVE_FORMAT_2S16) ||
		    (woc.dwFormats & WAVE_FORMAT_4S16) ||
		    (woc.dwFormats & WAVE_FORMAT_48S16) ||
		    (woc.dwFormats & WAVE_FORMAT_96S16)) {
			(*ippDS)->drvcaps.dwFlags |= DSCAPS_PRIMARY16BIT;
			(*ippDS)->drvcaps.dwFlags |= DSCAPS_PRIMARYSTEREO;
		}
		if (ds_emuldriver)
		    (*ippDS)->drvcaps.dwFlags |= DSCAPS_EMULDRIVER;
		(*ippDS)->drvcaps.dwMinSecondarySampleRate = DSBFREQUENCY_MIN;
		(*ippDS)->drvcaps.dwMaxSecondarySampleRate = DSBFREQUENCY_MAX;
		(*ippDS)->drvcaps.dwPrimaryBuffers = 1;
	}

	DSOUND_RecalcVolPan(&((*ippDS)->volpan));

	InitializeCriticalSection(&((*ippDS)->mixlock));
	RtlInitializeResource(&((*ippDS)->lock));

	if (!dsound) {
		HRESULT hres;
		dsound = (*ippDS);
		hres = DSOUND_PrimaryCreate(dsound);
		if (hres != DS_OK) {
			WARN("DSOUND_PrimaryCreate failed\n");
			return hres;
		}
		timeBeginPeriod(DS_TIME_RES);
		dsound->timerID = timeSetEvent(DS_TIME_DEL, DS_TIME_RES, DSOUND_timer,
					       (DWORD)dsound, TIME_PERIODIC | TIME_CALLBACK_FUNCTION);
	}

	return DS_OK;
}


/*******************************************************************************
 * DirectSound ClassFactory
 */

static HRESULT WINAPI
DSCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	ICOM_THIS(IClassFactoryImpl,iface);

	FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI
DSCF_AddRef(LPCLASSFACTORY iface) {
	ICOM_THIS(IClassFactoryImpl,iface);
	TRACE("(%p) ref was %ld\n", This, This->ref);
	return ++(This->ref);
}

static ULONG WINAPI DSCF_Release(LPCLASSFACTORY iface) {
	ICOM_THIS(IClassFactoryImpl,iface);
	/* static class, won't be  freed */
	TRACE("(%p) ref was %ld\n", This, This->ref);
	return --(This->ref);
}

static HRESULT WINAPI DSCF_CreateInstance(
	LPCLASSFACTORY iface,LPUNKNOWN pOuter,REFIID riid,LPVOID *ppobj
) {
	ICOM_THIS(IClassFactoryImpl,iface);
	TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);

	if (ppobj == NULL) {
		WARN("invalid parameter\n");
		return DSERR_INVALIDPARAM;
	}

	*ppobj = NULL;

	if ( IsEqualGUID( &IID_IDirectSound, riid ) ||
	     IsEqualGUID( &IID_IDirectSound8, riid ) ) {
		/* FIXME: reuse already created dsound if present? */
		return DirectSoundCreate8(0,(LPDIRECTSOUND8*)ppobj,pOuter);
	}

	WARN("(%p,%p,%s,%p) Interface not found!\n",This,pOuter,debugstr_guid(riid),ppobj);	
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

static IClassFactoryImpl DSOUND_CF = { &DSCF_Vtbl, 1 };

/*******************************************************************************
 * DirectSoundPrivate ClassFactory
 */

static HRESULT WINAPI
DSPCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	ICOM_THIS(IClassFactoryImpl,iface);

	FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI
DSPCF_AddRef(LPCLASSFACTORY iface) {
	ICOM_THIS(IClassFactoryImpl,iface);
	TRACE("(%p) ref was %ld\n", This, This->ref);
	return ++(This->ref);
}

static ULONG WINAPI 
DSPCF_Release(LPCLASSFACTORY iface) {
	ICOM_THIS(IClassFactoryImpl,iface);
	/* static class, won't be  freed */
	TRACE("(%p) ref was %ld\n", This, This->ref);
	return --(This->ref);
}

static HRESULT WINAPI 
DSPCF_CreateInstance(
	LPCLASSFACTORY iface,LPUNKNOWN pOuter,REFIID riid,LPVOID *ppobj
) {
	ICOM_THIS(IClassFactoryImpl,iface);
	TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);

	if (ppobj == NULL) {
		WARN("invalid parameter\n");
		return DSERR_INVALIDPARAM;
	}

	*ppobj = NULL;

	if ( IsEqualGUID( &IID_IKsPropertySet, riid ) ) {
		return IKsPrivatePropertySetImpl_Create((IKsPrivatePropertySetImpl**)ppobj);
	}

	WARN("(%p,%p,%s,%p) Interface not found!\n",This,pOuter,debugstr_guid(riid),ppobj);	
	return E_NOINTERFACE;
}

static HRESULT WINAPI 
DSPCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	ICOM_THIS(IClassFactoryImpl,iface);
	FIXME("(%p)->(%d),stub!\n",This,dolock);
	return S_OK;
}

static ICOM_VTABLE(IClassFactory) DSPCF_Vtbl = {
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	DSPCF_QueryInterface,
	DSPCF_AddRef,
	DSPCF_Release,
	DSPCF_CreateInstance,
	DSPCF_LockServer
};

static IClassFactoryImpl DSOUND_PRIVATE_CF = { &DSPCF_Vtbl, 1 };

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
    TRACE("(%s,%s,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if (ppv == NULL) {
	WARN("invalid parameter\n");
	return E_INVALIDARG;
    }

    *ppv = NULL;

    if ( IsEqualCLSID( &CLSID_DirectSound, rclsid ) ||
	 IsEqualCLSID( &CLSID_DirectSound8, rclsid ) ) {
	if ( IsEqualCLSID( &IID_IClassFactory, riid ) ) {
	    *ppv = (LPVOID)&DSOUND_CF;
	    IClassFactory_AddRef((IClassFactory*)*ppv);
	    return S_OK;
	}
    	WARN("(%s,%s,%p): no interface found.\n",
	    debugstr_guid(rclsid), debugstr_guid(riid), ppv);
	return S_FALSE;
    }
    
    if ( IsEqualCLSID( &CLSID_DirectSoundCapture, rclsid ) ||
	 IsEqualCLSID( &CLSID_DirectSoundCapture8, rclsid ) ) {
	if ( IsEqualCLSID( &IID_IClassFactory, riid ) ) {
	    *ppv = (LPVOID)&DSOUND_CAPTURE_CF;
	    IClassFactory_AddRef((IClassFactory*)*ppv);
	    return S_OK;
	}
    	WARN("(%s,%s,%p): no interface found.\n",
	    debugstr_guid(rclsid), debugstr_guid(riid), ppv);
	return S_FALSE;
    }
    
    if ( IsEqualCLSID( &CLSID_DirectSoundFullDuplex, rclsid ) ) {
	if ( IsEqualCLSID( &IID_IClassFactory, riid ) ) {
	    *ppv = (LPVOID)&DSOUND_FULLDUPLEX_CF;
	    IClassFactory_AddRef((IClassFactory*)*ppv);
	    return S_OK;
	}
    	WARN("(%s,%s,%p): no interface found.\n",
	    debugstr_guid(rclsid), debugstr_guid(riid), ppv);
	return S_FALSE;
    }
    
    if ( IsEqualCLSID( &CLSID_DirectSoundPrivate, rclsid ) ) {
	if ( IsEqualCLSID( &IID_IClassFactory, riid ) ) {
	    *ppv = (LPVOID)&DSOUND_PRIVATE_CF;
	    IClassFactory_AddRef((IClassFactory*)*ppv);
	    return S_OK;
	}
    	WARN("(%s,%s,%p): no interface found.\n",
	    debugstr_guid(rclsid), debugstr_guid(riid), ppv);
	return S_FALSE;
    }

    WARN("(%s,%s,%p): no class found.\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
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
