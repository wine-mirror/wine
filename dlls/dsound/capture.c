/*              DirectSoundCapture
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
 * TODO:
 *	Implement DirectSoundFullDuplex support.
 *	Implement FX support.
 */

#include "config.h"
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

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "mmsystem.h"
#include "mmddk.h"
#include "winreg.h"
#include "winternl.h"
#include "winnls.h"
#include "wine/debug.h"
#include "dsound.h"
#include "dsdriver.h"
#include "dsound_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);

static HRESULT WINAPI IDirectSoundCaptureImpl_Initialize(
    LPDIRECTSOUNDCAPTURE iface,
    LPCGUID lpcGUID );
static ULONG WINAPI IDirectSoundCaptureImpl_Release( 
    LPDIRECTSOUNDCAPTURE iface );
static ULONG WINAPI IDirectSoundCaptureBufferImpl_Release( 
    LPDIRECTSOUNDCAPTUREBUFFER8 iface );
static HRESULT DSOUND_CreateDirectSoundCaptureBuffer(
    IDirectSoundCaptureImpl *ipDSC, 
    LPCDSCBUFFERDESC lpcDSCBufferDesc, 
    LPVOID* ppobj );
static HRESULT WINAPI IDirectSoundFullDuplexImpl_Initialize(
    LPDIRECTSOUNDFULLDUPLEX iface,
    LPCGUID pCaptureGuid,
    LPCGUID pRendererGuid,
    LPCDSCBUFFERDESC lpDscBufferDesc,
    LPCDSBUFFERDESC lpDsBufferDesc,
    HWND hWnd,
    DWORD dwLevel,
    LPLPDIRECTSOUNDCAPTUREBUFFER8 lplpDirectSoundCaptureBuffer8,
    LPLPDIRECTSOUNDBUFFER8 lplpDirectSoundBuffer8 );

static ICOM_VTABLE(IDirectSoundCapture) dscvt;
static ICOM_VTABLE(IDirectSoundCaptureBuffer8) dscbvt;
static ICOM_VTABLE(IDirectSoundFullDuplex) dsfdvt;

IDirectSoundCaptureImpl*       dsound_capture = NULL;

/***************************************************************************
 * DirectSoundCaptureCreate [DSOUND.6]
 *
 * Create and initialize a DirectSoundCapture interface.
 *
 * PARAMS
 *    lpcGUID   [I] Address of the GUID that identifies the sound capture device.
 *    lplpDSC   [O] Address of a variable to receive the interface pointer.
 *    pUnkOuter [I] Must be NULL.
 *
 * RETURNS
 *    Success: DS_OK
 *    Failure: DSERR_NOAGGREGATION, DSERR_ALLOCATED, DSERR_INVALIDPARAM,
 *             DSERR_OUTOFMEMORY
 *
 * NOTES
 *    lpcGUID must be one of the values returned from DirectSoundCaptureEnumerate
 *    or NULL for the default device or DSDEVID_DefaultCapture or 
 *    DSDEVID_DefaultVoiceCapture.
 *
 *    DSERR_ALLOCATED is returned for sound devices that do not support full duplex.
 */
HRESULT WINAPI 
DirectSoundCaptureCreate8(
    LPCGUID lpcGUID,
    LPDIRECTSOUNDCAPTURE* lplpDSC,
    LPUNKNOWN pUnkOuter )
{
    IDirectSoundCaptureImpl** ippDSC=(IDirectSoundCaptureImpl**)lplpDSC;
    TRACE("(%s,%p,%p)\n", debugstr_guid(lpcGUID), lplpDSC, pUnkOuter);

    if ( pUnkOuter ) {
	WARN("invalid parameter: pUnkOuter != NULL\n");
        return DSERR_NOAGGREGATION;
    }

    if ( !lplpDSC ) {
	WARN("invalid parameter: lplpDSC == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    /* Default device? */
    if ( !lpcGUID || IsEqualGUID(lpcGUID, &GUID_NULL) ) 
	lpcGUID = &DSDEVID_DefaultCapture;

    *ippDSC = (IDirectSoundCaptureImpl*)HeapAlloc(GetProcessHeap(),
        HEAP_ZERO_MEMORY, sizeof(IDirectSoundCaptureImpl));

    if (*ippDSC == NULL) {
	WARN("out of memory\n");
        return DSERR_OUTOFMEMORY;
    } else {
        ICOM_THIS(IDirectSoundCaptureImpl, *ippDSC);

        This->ref = 1;
        This->state = STATE_STOPPED;

        InitializeCriticalSection( &(This->lock) );

        This->lpVtbl = &dscvt;
        dsound_capture = This;

        if (GetDeviceID(lpcGUID, &This->guid) == DS_OK) {
	    HRESULT	hres; 
            hres = IDirectSoundCaptureImpl_Initialize( (LPDIRECTSOUNDCAPTURE)This, &This->guid);
	    if (hres != DS_OK)
		WARN("IDirectSoundCaptureImpl_Initialize failed\n");
	    return hres;
	}
    }
    WARN("invalid GUID: %s\n", debugstr_guid(lpcGUID));
    return DSERR_INVALIDPARAM;
}

/***************************************************************************
 * DirectSoundCaptureEnumerateA [DSOUND.7]
 *
 * Enumerate all DirectSound drivers installed in the system.
 *
 * PARAMS
 *    lpDSEnumCallback  [I] Address of callback function.
 *    lpContext         [I] Address of user defined context passed to callback function.
 *
 * RETURNS
 *    Success: DS_OK
 *    Failure: DSERR_INVALIDPARAM
 */
HRESULT WINAPI 
DirectSoundCaptureEnumerateA(
    LPDSENUMCALLBACKA lpDSEnumCallback,
    LPVOID lpContext)
{
    unsigned devs, wid;
    DSDRIVERDESC desc;
    GUID guid;
    int err;

    TRACE("(%p,%p)\n", lpDSEnumCallback, lpContext );

    if (lpDSEnumCallback == NULL) {
	WARN("invalid parameter: lpDSEnumCallback == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    devs = waveInGetNumDevs();
    if (devs > 0) {
	if (GetDeviceID(&DSDEVID_DefaultCapture, &guid) == DS_OK) {
	    GUID temp;
    	    for (wid = 0; wid < devs; ++wid) {
		err = mmErr(waveInMessage((HWAVEIN)wid,DRV_QUERYDSOUNDGUID,(DWORD)&temp,0));
		if (err == DS_OK) {
		    if (IsEqualGUID( &guid, &temp ) ) {
			err = mmErr(waveInMessage((HWAVEIN)wid,DRV_QUERYDSOUNDDESC,(DWORD)&desc,0));
	    		if (err == DS_OK) {
			    TRACE("calling lpDSEnumCallback(%s,\"%s\",\"%s\",%p)\n",
		    		debugstr_guid(&DSDEVID_DefaultCapture),"Primary Sound Capture Driver",desc.szDrvName,lpContext);
			    if (lpDSEnumCallback((LPGUID)&DSDEVID_DefaultCapture, "Primary Sound Capture Driver", desc.szDrvName, lpContext) == FALSE)
				return DS_OK;
			}
		    }
		}
	    }
	}
    }

    for (wid = 0; wid < devs; ++wid) {
	err = mmErr(waveInMessage((HWAVEIN)wid,DRV_QUERYDSOUNDDESC,(DWORD)&desc,0));
	if (err == DS_OK) {
	    err = mmErr(waveInMessage((HWAVEIN)wid,DRV_QUERYDSOUNDGUID,(DWORD)&guid,0));
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
 * DirectSoundCaptureEnumerateW [DSOUND.8]
 *
 * Enumerate all DirectSound drivers installed in the system.
 *
 * PARAMS
 *    lpDSEnumCallback  [I] Address of callback function.
 *    lpContext         [I] Address of user defined context passed to callback function.
 *
 * RETURNS
 *    Success: DS_OK
 *    Failure: DSERR_INVALIDPARAM
 */
HRESULT WINAPI 
DirectSoundCaptureEnumerateW(
    LPDSENUMCALLBACKW lpDSEnumCallback,
    LPVOID lpContext)
{
    unsigned devs, wid;
    DSDRIVERDESC desc;
    GUID guid;
    int err;
    WCHAR wDesc[MAXPNAMELEN];
    WCHAR wName[MAXPNAMELEN];

    TRACE("(%p,%p)\n", lpDSEnumCallback, lpContext );

    if (lpDSEnumCallback == NULL) {
	WARN("invalid parameter: lpDSEnumCallback == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    devs = waveInGetNumDevs();
    if (devs > 0) {
	if (GetDeviceID(&DSDEVID_DefaultCapture, &guid) == DS_OK) {
	    GUID temp;
    	    for (wid = 0; wid < devs; ++wid) {
		err = mmErr(waveInMessage((HWAVEIN)wid,DRV_QUERYDSOUNDGUID,(DWORD)&temp,0));
		if (err == DS_OK) {
		    if (IsEqualGUID( &guid, &temp ) ) {
			err = mmErr(waveInMessage((HWAVEIN)wid,DRV_QUERYDSOUNDDESC,(DWORD)&desc,0));
	    		if (err == DS_OK) {
			    TRACE("calling lpDSEnumCallback(%s,\"%s\",\"%s\",%p)\n",
		    		debugstr_guid(&DSDEVID_DefaultCapture),"Primary Sound Capture Driver",desc.szDrvName,lpContext);
			    MultiByteToWideChar( CP_ACP, 0, "Primary Sound Capture Driver", -1, 
			        wDesc, sizeof(wDesc)/sizeof(WCHAR) );
			    MultiByteToWideChar( CP_ACP, 0, desc.szDrvName, -1, 
		    	        wName, sizeof(wName)/sizeof(WCHAR) );
			    if (lpDSEnumCallback((LPGUID)&DSDEVID_DefaultCapture, wDesc, wName, lpContext) == FALSE)
				return DS_OK;
			}
		    }
		}
	    }
	}
    }

    for (wid = 0; wid < devs; ++wid) {
	err = mmErr(waveInMessage((HWAVEIN)wid,DRV_QUERYDSOUNDDESC,(DWORD)&desc,0));
	if (err == DS_OK) {
	    err = mmErr(waveInMessage((HWAVEIN)wid,DRV_QUERYDSOUNDGUID,(DWORD)&guid,0));
	    if (err == DS_OK) {
		TRACE("calling lpDSEnumCallback(%s,\"%s\",\"%s\",%p)\n",
		    debugstr_guid(&DSDEVID_DefaultCapture),desc.szDesc,desc.szDrvName,lpContext);
		MultiByteToWideChar( CP_ACP, 0, desc.szDesc, -1, 
		    wDesc, sizeof(wDesc)/sizeof(WCHAR) );
		MultiByteToWideChar( CP_ACP, 0, desc.szDrvName, -1, 
		    wName, sizeof(wName)/sizeof(WCHAR) );
		if (lpDSEnumCallback((LPGUID)&DSDEVID_DefaultCapture, wDesc, wName, lpContext) == FALSE)
		    return DS_OK;
	    }
	} 
    }

    return DS_OK;
}

static void CALLBACK 
DSOUND_capture_callback(
    HWAVEIN hwi, 
    UINT msg, 
    DWORD dwUser, 
    DWORD dw1, 
    DWORD dw2 )
{
    IDirectSoundCaptureImpl* This = (IDirectSoundCaptureImpl*)dwUser;
    TRACE("(%p,%08x(%s),%08lx,%08lx,%08lx) entering at %ld\n",hwi,msg,
	msg == MM_WIM_OPEN ? "MM_WIM_OPEN" : msg == MM_WIM_CLOSE ? "MM_WIM_CLOSE" :
	msg == MM_WIM_DATA ? "MM_WIM_DATA" : "UNKNOWN",dwUser,dw1,dw2,GetTickCount());

    if (msg == MM_WIM_DATA) {
    	EnterCriticalSection( &(This->lock) );
	TRACE("DirectSoundCapture msg=MM_WIM_DATA, old This->state=%ld, old This->index=%d\n",This->state,This->index);
	if (This->state != STATE_STOPPED) {
	    if (This->state == STATE_STARTING) { 
		MMTIME mtime;
		mtime.wType = TIME_BYTES;
		waveInGetPosition(This->hwi, &mtime, sizeof(mtime));
		TRACE("mtime.u.cb=%ld,This->buflen=%ld\n", mtime.u.cb, This->buflen);
		mtime.u.cb = mtime.u.cb % This->buflen;
		This->read_position = mtime.u.cb;
		This->state = STATE_CAPTURING;
	    }
	    This->index = (This->index + 1) % This->nrofpwaves;
	    waveInUnprepareHeader(hwi,&(This->pwave[This->index]),sizeof(WAVEHDR));
	    if (This->capture_buffer->nrofnotifies)
		SetEvent(This->capture_buffer->notifies[This->index].hEventNotify);
	    if ( (This->index == 0) && !(This->capture_buffer->flags & DSCBSTART_LOOPING) ) {
		TRACE("end of buffer\n");
		This->state = STATE_STOPPED;
	    } else {
		if (This->state == STATE_CAPTURING) {
		    waveInPrepareHeader(hwi,&(This->pwave[This->index]),sizeof(WAVEHDR));
		    waveInAddBuffer(hwi, &(This->pwave[This->index]), sizeof(WAVEHDR));
	        }
	    }
	}
	TRACE("DirectSoundCapture new This->state=%ld, new This->index=%d\n",This->state,This->index);
    	LeaveCriticalSection( &(This->lock) );
    }

    TRACE("completed\n");
}

static HRESULT WINAPI
IDirectSoundCaptureImpl_QueryInterface(
    LPDIRECTSOUNDCAPTURE iface,
    REFIID riid,
    LPVOID* ppobj )
{
    ICOM_THIS(IDirectSoundCaptureImpl,iface);
    TRACE( "(%p,%s,%p)\n", This, debugstr_guid(riid), ppobj );

    if (ppobj == NULL) {
	WARN("invalid parameter\n");
	return E_INVALIDARG;
    }

    *ppobj = NULL;

    if (This->driver) {
	HRESULT	hres; 
        hres = IDsCaptureDriver_QueryInterface(This->driver, riid, ppobj);
	if (hres != DS_OK)
	    WARN("IDsCaptureDriver_QueryInterface failed\n");
	return hres;
    }

    WARN("unsupported riid: %s\n", debugstr_guid(riid));
    return E_FAIL;
}

static ULONG WINAPI
IDirectSoundCaptureImpl_AddRef( LPDIRECTSOUNDCAPTURE iface )
{
    ULONG uRef;
    ICOM_THIS(IDirectSoundCaptureImpl,iface);
    TRACE("(%p) ref was %ld, thread is %04lx\n",This, This->ref, GetCurrentThreadId());

    EnterCriticalSection( &(This->lock) );
    uRef = ++(This->ref);

    if (This->driver) 
        IDsCaptureDriver_AddRef(This->driver);

    LeaveCriticalSection( &(This->lock) );

    return uRef;
}

static ULONG WINAPI
IDirectSoundCaptureImpl_Release( LPDIRECTSOUNDCAPTURE iface )
{
    ULONG uRef;
    ICOM_THIS(IDirectSoundCaptureImpl,iface);
    TRACE("(%p) ref was %ld, thread is %04lx\n",This, This->ref, GetCurrentThreadId());

    EnterCriticalSection( &(This->lock) );

    uRef = --(This->ref);

    LeaveCriticalSection( &(This->lock) );

    if ( uRef == 0 ) {
        TRACE("deleting object\n");
        if (This->capture_buffer)
            IDirectSoundCaptureBufferImpl_Release(
		(LPDIRECTSOUNDCAPTUREBUFFER8) This->capture_buffer);

        if (This->driver) { 
            IDsCaptureDriver_Close(This->driver);
            IDsCaptureDriver_Release(This->driver);
	}

        DeleteCriticalSection( &(This->lock) );
        HeapFree( GetProcessHeap(), 0, This );
	dsound_capture = NULL;
	TRACE("(%p) released\n",This);
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

    TRACE( "(%p,%p,%p,%p)\n",This,lpcDSCBufferDesc,lplpDSCaptureBuffer,pUnk );

    if (This == NULL) {
	WARN("invalid parameter: This == NULL\n");
	return DSERR_INVALIDPARAM;
    }

    if (lpcDSCBufferDesc == NULL) {
	WARN("invalid parameter: lpcDSCBufferDesc == NULL)\n");
	return DSERR_INVALIDPARAM;
    }

    if (lplpDSCaptureBuffer == NULL) {
	WARN("invalid parameter: lplpDSCaptureBuffer == NULL\n");
	return DSERR_INVALIDPARAM;
    }

    if (pUnk != NULL) {
	WARN("invalid parameter: pUnk != NULL\n");
	return DSERR_INVALIDPARAM;
    }

    /* FIXME: We can only have one buffer so what do we do here? */
    if (This->capture_buffer) {
	WARN("lnvalid parameter: already has buffer\n");
	return DSERR_INVALIDPARAM;    /* DSERR_GENERIC ? */
    }

    hr = DSOUND_CreateDirectSoundCaptureBuffer( This, lpcDSCBufferDesc, 
        (LPVOID*)lplpDSCaptureBuffer );

    if (hr != DS_OK)
	WARN("DSOUND_CreateDirectSoundCaptureBuffer failed\n");

    return hr;
}

static HRESULT WINAPI
IDirectSoundCaptureImpl_GetCaps(
    LPDIRECTSOUNDCAPTURE iface,
    LPDSCCAPS lpDSCCaps )
{
    ICOM_THIS(IDirectSoundCaptureImpl,iface);
    TRACE("(%p,%p)\n",This,lpDSCCaps);

    if (lpDSCCaps== NULL) {
	WARN("invalid parameter: lpDSCCaps== NULL\n");
	return DSERR_INVALIDPARAM;
    }

    if (lpDSCCaps->dwSize < sizeof(*lpDSCCaps)) {
	WARN("invalid parameter: lpDSCCaps->dwSize = %ld < %d\n", 
	    lpDSCCaps->dwSize, sizeof(*lpDSCCaps));
	return DSERR_INVALIDPARAM;
    }

    if ( !(This->initialized) ) {
	WARN("not initialized\n");
	return DSERR_UNINITIALIZED;
    }

    lpDSCCaps->dwFlags = This->drvcaps.dwFlags;
    lpDSCCaps->dwFormats = This->drvcaps.dwFormats;
    lpDSCCaps->dwChannels = This->drvcaps.dwChannels;

    TRACE("(flags=0x%08lx,format=0x%08lx,channels=%ld)\n",lpDSCCaps->dwFlags,
        lpDSCCaps->dwFormats, lpDSCCaps->dwChannels);

    return DS_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureImpl_Initialize(
    LPDIRECTSOUNDCAPTURE iface,
    LPCGUID lpcGUID )
{
    HRESULT err = DSERR_INVALIDPARAM;
    unsigned wid, widn;
    ICOM_THIS(IDirectSoundCaptureImpl,iface);
    TRACE("(%p)\n", This);

    if (!This) {
	WARN("invalid parameter: This == NULL\n");
	return DSERR_INVALIDPARAM;
    }

    if (This->initialized) {
	WARN("already initialized\n");
	return DSERR_ALREADYINITIALIZED;
    }

    widn = waveInGetNumDevs();

    if (!widn) { 
	WARN("no audio devices found\n");
	return DSERR_NODRIVER;
    }

    /* Get dsound configuration */
    setup_dsound_options();

    /* enumerate WINMM audio devices and find the one we want */
    for (wid=0; wid<widn; wid++) {
	GUID guid;
	err = mmErr(waveInMessage((HWAVEIN)wid,DRV_QUERYDSOUNDGUID,(DWORD)(&guid),0));
	if (err != DS_OK) {
	    WARN("waveInMessage failed; err=%lx\n",err);
	    return err;
	}
    	if (IsEqualGUID( lpcGUID, &guid) ) {
	    err = DS_OK;
	    break;
	}
    }

    if (err != DS_OK) {
	WARN("invalid parameter\n");
	return DSERR_INVALIDPARAM;
    }

    err = mmErr(waveInMessage((HWAVEIN)wid,DRV_QUERYDSOUNDIFACE,(DWORD)&(This->driver),0));
    if ( (err != DS_OK) && (err != DSERR_UNSUPPORTED) ) {
	WARN("waveInMessage failed; err=%lx\n",err);
	return err;
    }
    err = DS_OK;

    /* Disable the direct sound driver to force emulation if requested. */
    if (ds_hw_accel == DS_HW_ACCEL_EMULATION)
	This->driver = NULL;

    /* Get driver description */
    if (This->driver) {
        TRACE("using DirectSound driver\n");
        err = IDsCaptureDriver_GetDriverDesc(This->driver, &(This->drvdesc));
	if (err != DS_OK) {
	    WARN("IDsCaptureDriver_GetDriverDesc failed\n");
	    return err;
	}
    } else {
        TRACE("using WINMM\n");
        /* if no DirectSound interface available, use WINMM API instead */
        This->drvdesc.dwFlags = DSDDESC_DOMMSYSTEMOPEN | 
            DSDDESC_DOMMSYSTEMSETFORMAT;
    }

    This->drvdesc.dnDevNode = wid;
    
    /* open the DirectSound driver if available */
    if (This->driver && (err == DS_OK))
        err = IDsCaptureDriver_Open(This->driver);

    if (err == DS_OK) {
        This->initialized = TRUE;

        /* the driver is now open, so it's now allowed to call GetCaps */
        if (This->driver) {
	    This->drvcaps.dwSize = sizeof(This->drvcaps);
            err = IDsCaptureDriver_GetCaps(This->driver,&(This->drvcaps));
	    if (err != DS_OK) {
		WARN("IDsCaptureDriver_GetCaps failed\n");
		return err;
	    }
        } else /*if (This->hwi)*/ {
            WAVEINCAPSA    wic;
            err = mmErr(waveInGetDevCapsA((UINT)This->drvdesc.dnDevNode, &wic, sizeof(wic)));

            if (err == DS_OK) {
                This->drvcaps.dwFlags = 0;
                strncpy(This->drvdesc.szDrvName, wic.szPname, 
                    sizeof(This->drvdesc.szDrvName)); 

                This->drvcaps.dwFlags |= DSCCAPS_EMULDRIVER;
                This->drvcaps.dwFormats = wic.dwFormats;
                This->drvcaps.dwChannels = wic.wChannels;
            }
        }
    }

    return err;
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
DSOUND_CreateDirectSoundCaptureBuffer(
    IDirectSoundCaptureImpl *ipDSC, 
    LPCDSCBUFFERDESC lpcDSCBufferDesc, 
    LPVOID* ppobj )
{
    LPWAVEFORMATEX  wfex;
    TRACE( "(%p,%p)\n", lpcDSCBufferDesc, ppobj );

    if (ipDSC == NULL) {
	WARN("invalid parameter: ipDSC == NULL\n");
	return DSERR_INVALIDPARAM;
    }

    if (lpcDSCBufferDesc == NULL) {
	WARN("invalid parameter: lpcDSCBufferDesc == NULL\n");
	return DSERR_INVALIDPARAM;
    }

    if (ppobj == NULL) {
	WARN("invalid parameter: ppobj == NULL\n");
	return DSERR_INVALIDPARAM;
    }

    if ( (lpcDSCBufferDesc->dwSize < sizeof(DSCBUFFERDESC)) || 
        (lpcDSCBufferDesc->dwBufferBytes == 0) ||
        (lpcDSCBufferDesc->lpwfxFormat == NULL) ) {
	WARN("invalid lpcDSCBufferDesc\n");
	*ppobj = NULL;
	return DSERR_INVALIDPARAM;
    }

    if ( !ipDSC->initialized ) {
	WARN("not initialized\n");
	*ppobj = NULL;
	return DSERR_UNINITIALIZED;
    }

    wfex = lpcDSCBufferDesc->lpwfxFormat;

    if (wfex) {
        TRACE("(formattag=0x%04x,chans=%d,samplerate=%ld,"
            "bytespersec=%ld,blockalign=%d,bitspersamp=%d,cbSize=%d)\n",
            wfex->wFormatTag, wfex->nChannels, wfex->nSamplesPerSec,
            wfex->nAvgBytesPerSec, wfex->nBlockAlign,
            wfex->wBitsPerSample, wfex->cbSize);

        if (wfex->wFormatTag == WAVE_FORMAT_PCM)
            memcpy(&(ipDSC->wfx), wfex, sizeof(WAVEFORMATEX));
        else {
            WARN("non PCM formats not supported\n");
	    *ppobj = NULL;
            return DSERR_BADFORMAT;
        }
    } else {
	WARN("lpcDSCBufferDesc->lpwfxFormat == 0\n");
	*ppobj = NULL;
	return DSERR_INVALIDPARAM; /* FIXME: DSERR_BADFORMAT ? */
    }

    *ppobj = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,
        sizeof(IDirectSoundCaptureBufferImpl));

    if ( *ppobj == NULL ) {
	WARN("out of memory\n");
	*ppobj = NULL;
	return DSERR_OUTOFMEMORY;
    } else {
    	HRESULT err = DS_OK;
        ICOM_THIS(IDirectSoundCaptureBufferImpl,*ppobj);

        This->ref = 1;
        This->dsound = ipDSC;
        This->dsound->capture_buffer = This;
	This->notify = NULL;
	This->nrofnotifies = 0;
	This->hwnotify = NULL;

        This->pdscbd = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,
            lpcDSCBufferDesc->dwSize);
        if (This->pdscbd)     
            memcpy(This->pdscbd, lpcDSCBufferDesc, lpcDSCBufferDesc->dwSize);
        else {
            WARN("no memory\n");
            This->dsound->capture_buffer = 0;
            HeapFree( GetProcessHeap(), 0, This );
            *ppobj = NULL;
            return DSERR_OUTOFMEMORY; 
        }

        This->lpVtbl = &dscbvt;

	if (ipDSC->driver) {
	    err = IDsCaptureDriver_CreateCaptureBuffer(ipDSC->driver, 
		&(ipDSC->wfx),0,0,&(ipDSC->buflen),&(ipDSC->buffer),(LPVOID*)&(ipDSC->hwbuf));
	    if (err != DS_OK) {
		WARN("IDsCaptureDriver_CreateCaptureBuffer failed\n");
		This->dsound->capture_buffer = 0;
		HeapFree( GetProcessHeap(), 0, This );
		*ppobj = NULL;
		return err;
	    }
	} else {
            LPBYTE newbuf;
            DWORD buflen;
	    DWORD flags = CALLBACK_FUNCTION;
	    if (ds_hw_accel != DS_HW_ACCEL_EMULATION)
		flags |= WAVE_DIRECTSOUND;
            err = mmErr(waveInOpen(&(ipDSC->hwi),
                ipDSC->drvdesc.dnDevNode, &(ipDSC->wfx),
                (DWORD)DSOUND_capture_callback, (DWORD)ipDSC, flags));
            if (err != DS_OK) {
                WARN("waveInOpen failed\n");
		This->dsound->capture_buffer = 0;
		HeapFree( GetProcessHeap(), 0, This );
		*ppobj = NULL;
		return err;
            }

	    buflen = lpcDSCBufferDesc->dwBufferBytes;
            TRACE("desired buflen=%ld, old buffer=%p\n", buflen, ipDSC->buffer);
            newbuf = (LPBYTE)HeapReAlloc(GetProcessHeap(),0,ipDSC->buffer,buflen);

            if (newbuf == NULL) {
                WARN("failed to allocate capture buffer\n");
                err = DSERR_OUTOFMEMORY;
                /* but the old buffer might still exist and must be re-prepared */
            } else {
                ipDSC->buffer = newbuf;
                ipDSC->buflen = buflen;
            }
	}
    }

    TRACE("returning DS_OK\n");
    return DS_OK;
}

/*******************************************************************************
 *		IDirectSoundCaptureNotify
 */
static HRESULT WINAPI IDirectSoundCaptureNotifyImpl_QueryInterface(
    LPDIRECTSOUNDNOTIFY iface,
    REFIID riid,
    LPVOID *ppobj)
{
    ICOM_THIS(IDirectSoundCaptureNotifyImpl,iface);
    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);

    if (This->dscb == NULL) {
	WARN("invalid parameter\n");
	return E_INVALIDARG;
    }

    return IDirectSoundCaptureBuffer_QueryInterface((LPDIRECTSOUNDCAPTUREBUFFER)This->dscb, riid, ppobj);
}

static ULONG WINAPI IDirectSoundCaptureNotifyImpl_AddRef(LPDIRECTSOUNDNOTIFY iface)
{
    ICOM_THIS(IDirectSoundCaptureNotifyImpl,iface);
    DWORD ref;

    TRACE("(%p) ref was %ld, thread is %04lx\n",This, This->ref, GetCurrentThreadId());

    ref = InterlockedIncrement(&(This->ref));
    return ref;
}

static ULONG WINAPI IDirectSoundCaptureNotifyImpl_Release(LPDIRECTSOUNDNOTIFY iface)
{
    ICOM_THIS(IDirectSoundCaptureNotifyImpl,iface);
    DWORD ref;

    TRACE("(%p) ref was %ld, thread is %04lx\n",This, This->ref, GetCurrentThreadId());

    ref = InterlockedDecrement(&(This->ref));
    if (ref == 0) {
	This->dscb->notify=NULL;
	IDirectSoundCaptureBuffer_Release((LPDIRECTSOUNDCAPTUREBUFFER)This->dscb);
	HeapFree(GetProcessHeap(),0,This);
	TRACE("(%p) released\n",This);
    }
    return ref;
}

static HRESULT WINAPI IDirectSoundCaptureNotifyImpl_SetNotificationPositions(
    LPDIRECTSOUNDNOTIFY iface,
    DWORD howmuch,
    LPCDSBPOSITIONNOTIFY notify)
{
    ICOM_THIS(IDirectSoundCaptureNotifyImpl,iface);
    TRACE("(%p,0x%08lx,%p)\n",This,howmuch,notify);

    if (notify == NULL) {
	WARN("invalid parameter: notify == NULL\n");
	return DSERR_INVALIDPARAM;
    }

    if (TRACE_ON(dsound)) {
	int	i;
	for (i=0;i<howmuch;i++)
	    TRACE("notify at %ld to 0x%08lx\n",
	    notify[i].dwOffset,(DWORD)notify[i].hEventNotify);
    }

    if (This->dscb->hwnotify) {
	HRESULT hres;
	hres = IDsDriverNotify_SetNotificationPositions(This->dscb->hwnotify, howmuch, notify);
	if (hres != DS_OK)
	    WARN("IDsDriverNotify_SetNotificationPositions failed\n");
	return hres;
    } else {
	/* Make an internal copy of the caller-supplied array.
	 * Replace the existing copy if one is already present. */
	This->dscb->notifies = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 
	    This->dscb->notifies, howmuch * sizeof(DSBPOSITIONNOTIFY));
	if (This->dscb->notifies == NULL) {
	    WARN("out of memory\n");
	    return DSERR_OUTOFMEMORY;
	}
	memcpy(This->dscb->notifies, notify, howmuch * sizeof(DSBPOSITIONNOTIFY));
	This->dscb->nrofnotifies = howmuch;
    }

    return S_OK;
}

ICOM_VTABLE(IDirectSoundNotify) dscnvt =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IDirectSoundCaptureNotifyImpl_QueryInterface,
    IDirectSoundCaptureNotifyImpl_AddRef,
    IDirectSoundCaptureNotifyImpl_Release,
    IDirectSoundCaptureNotifyImpl_SetNotificationPositions,
};

HRESULT WINAPI IDirectSoundCaptureNotifyImpl_Create(
    IDirectSoundCaptureBufferImpl *dscb,
    IDirectSoundCaptureNotifyImpl **pdscn)
{
    IDirectSoundCaptureNotifyImpl * dscn;
    TRACE("(%p,%p)\n",dscb,pdscn);

    dscn = (IDirectSoundCaptureNotifyImpl*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(dscn));

    if (dscn == NULL) {
	WARN("out of memory\n");
	return DSERR_OUTOFMEMORY;
    }

    dscn->ref = 0;
    dscn->lpVtbl = &dscnvt;
    dscn->dscb = dscb;
    dscb->notify = dscn;
    IDirectSoundCaptureBuffer_AddRef((LPDIRECTSOUNDCAPTUREBUFFER)dscb);

    *pdscn = dscn;
    return DS_OK;
}

/*******************************************************************************
 *		IDirectSoundCaptureBuffer
 */
static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_QueryInterface(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    REFIID riid,
    LPVOID* ppobj )
{
    ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);
    HRESULT hres;
    TRACE( "(%p,%s,%p)\n", This, debugstr_guid(riid), ppobj );

    if (ppobj == NULL) {
	WARN("invalid parameter\n");
	return E_INVALIDARG;
    }

    *ppobj = NULL;

    if ( IsEqualGUID( &IID_IDirectSoundNotify, riid ) ||
         IsEqualGUID( &IID_IDirectSoundNotify8, riid ) ) {
	if (!This->notify)
	    hres = IDirectSoundCaptureNotifyImpl_Create(This, &This->notify);
	if (This->notify) {
	    if (This->dsound->hwbuf) {
		hres = IDsCaptureDriverBuffer_QueryInterface(This->dsound->hwbuf, 
		    &IID_IDsDriverNotify, (LPVOID*)&(This->hwnotify));
		if (hres != DS_OK) {
		    WARN("IDsCaptureDriverBuffer_QueryInterface failed\n");
		    *ppobj = 0;
		    return hres;
	        }
	    }

	    IDirectSoundNotify_AddRef((LPDIRECTSOUNDNOTIFY)This->notify);
	    *ppobj = (LPVOID)This->notify;
	    return DS_OK;
	}

	WARN("IID_IDirectSoundNotify\n");
	return E_FAIL;
    }

    if ( IsEqualGUID( &IID_IDirectSoundCaptureBuffer, riid ) ||
         IsEqualGUID( &IID_IDirectSoundCaptureBuffer8, riid ) ) {
	IDirectSoundCaptureBuffer8_AddRef(iface);
	*ppobj = This;
	return NO_ERROR;
    }

    FIXME("(%p,%s,%p) unsupported GUID\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI
IDirectSoundCaptureBufferImpl_AddRef( LPDIRECTSOUNDCAPTUREBUFFER8 iface )
{
    ULONG uRef;
    ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);
    TRACE("(%p) ref was %ld, thread is %04lx\n",This, This->ref, GetCurrentThreadId());

    assert(This->dsound);

    EnterCriticalSection( &(This->dsound->lock) );

    uRef = ++(This->ref);

    LeaveCriticalSection( &(This->dsound->lock) );

    return uRef;
}

static ULONG WINAPI
IDirectSoundCaptureBufferImpl_Release( LPDIRECTSOUNDCAPTUREBUFFER8 iface )
{
    ULONG uRef;
    ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);
    TRACE("(%p) ref was %ld, thread is %04lx\n",This, This->ref, GetCurrentThreadId());

    assert(This->dsound);

    EnterCriticalSection( &(This->dsound->lock) );

    uRef = --(This->ref);

    LeaveCriticalSection( &(This->dsound->lock) );

    if ( uRef == 0 ) {
        TRACE("deleting object\n");
        if (This->pdscbd)
            HeapFree(GetProcessHeap(),0, This->pdscbd);

	if (This->dsound->hwi) {
	    waveInReset(This->dsound->hwi);
	    waveInClose(This->dsound->hwi);
	    if (This->dsound->pwave) {
	    	HeapFree(GetProcessHeap(),0, This->dsound->pwave);
		This->dsound->pwave = 0;
	    }
	    This->dsound->hwi = 0;
	}

	if (This->dsound->hwbuf) 
	    IDsCaptureDriverBuffer_Release(This->dsound->hwbuf);

        /* remove from IDirectSoundCaptureImpl */
        if (This->dsound)
            This->dsound->capture_buffer = NULL;
        else
            ERR("does not reference dsound\n");

        if (This->notify)
	    IDirectSoundNotify_Release((LPDIRECTSOUNDNOTIFY)This->notify);
        
	if (This->notifies != NULL)
		HeapFree(GetProcessHeap(), 0, This->notifies);

        HeapFree( GetProcessHeap(), 0, This );
	TRACE("(%p) released\n",This);
    }

    return uRef;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_GetCaps(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    LPDSCBCAPS lpDSCBCaps )
{
    ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);
    TRACE( "(%p,%p)\n", This, lpDSCBCaps );

    if (This == NULL) {
        WARN("invalid parameter: This == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (lpDSCBCaps == NULL) {
        WARN("invalid parameter: lpDSCBCaps == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (lpDSCBCaps->dwSize < sizeof(DSCBCAPS)) {
        WARN("invalid parameter: lpDSCBCaps->dwSize = %ld < %d\n", 
	    lpDSCBCaps->dwSize, sizeof(DSCBCAPS));
        return DSERR_INVALIDPARAM;
    }

    if (This->dsound == NULL) {
        WARN("invalid parameter: This->dsound == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    lpDSCBCaps->dwSize = sizeof(DSCBCAPS);
    lpDSCBCaps->dwFlags = This->flags;
    lpDSCBCaps->dwBufferBytes = This->pdscbd->dwBufferBytes;
    lpDSCBCaps->dwReserved = 0;

    TRACE("returning DS_OK\n");
    return DS_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_GetCurrentPosition(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    LPDWORD lpdwCapturePosition,
    LPDWORD lpdwReadPosition )
{
    ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);
    TRACE( "(%p,%p,%p)\n", This, lpdwCapturePosition, lpdwReadPosition );

    if (This == NULL) {
        WARN("invalid parameter: This == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (This->dsound == NULL) {
        WARN("invalid parameter: This->dsound == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (This->dsound->driver) {
	HRESULT	hres;
        hres = IDsCaptureDriverBuffer_GetPosition(This->dsound->hwbuf, lpdwCapturePosition, lpdwReadPosition );
	if (hres != DS_OK) {
	    WARN("IDsCaptureDriverBuffer_GetPosition failed\n");
	    return hres;
	}
    } else if (This->dsound->hwi) {
	EnterCriticalSection(&(This->dsound->lock));
	TRACE("old This->dsound->state=%ld\n",This->dsound->state);
        if (lpdwCapturePosition) {
            MMTIME mtime;
            mtime.wType = TIME_BYTES;
            waveInGetPosition(This->dsound->hwi, &mtime, sizeof(mtime));
	    TRACE("mtime.u.cb=%ld,This->dsound->buflen=%ld\n", mtime.u.cb, 
		This->dsound->buflen);
	    mtime.u.cb = mtime.u.cb % This->dsound->buflen;
            *lpdwCapturePosition = mtime.u.cb;
        }
    
	if (lpdwReadPosition) {
            if (This->dsound->state == STATE_STARTING) { 
		if (lpdwCapturePosition)
		    This->dsound->read_position = *lpdwCapturePosition;
                This->dsound->state = STATE_CAPTURING;
            } 
            *lpdwReadPosition = This->dsound->read_position;
        }
	TRACE("new This->dsound->state=%ld\n",This->dsound->state);
	LeaveCriticalSection(&(This->dsound->lock));
	if (lpdwCapturePosition) TRACE("*lpdwCapturePosition=%ld\n",*lpdwCapturePosition);
	if (lpdwReadPosition) TRACE("*lpdwReadPosition=%ld\n",*lpdwReadPosition);
    } else {
        WARN("no driver\n");
        return DSERR_NODRIVER;
    }
    
    TRACE("returning DS_OK\n");
    return DS_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_GetFormat(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    LPWAVEFORMATEX lpwfxFormat,
    DWORD dwSizeAllocated,
    LPDWORD lpdwSizeWritten )
{
    ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);
    TRACE( "(%p,%p,0x%08lx,%p)\n", This, lpwfxFormat, dwSizeAllocated, 
        lpdwSizeWritten );

    if (This == NULL) {
        WARN("invalid parameter: This == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (This->dsound == NULL) {
        WARN("invalid parameter: This->dsound == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    /* FIXME: use real size for extended formats someday */
    if (dwSizeAllocated > sizeof(This->dsound->wfx))
        dwSizeAllocated = sizeof(This->dsound->wfx);
    if (lpwfxFormat) { /* NULL is valid (just want size) */
        memcpy(lpwfxFormat,&(This->dsound->wfx),dwSizeAllocated);
        if (lpdwSizeWritten)
            *lpdwSizeWritten = dwSizeAllocated;
    } else {
        if (lpdwSizeWritten)
            *lpdwSizeWritten = sizeof(This->dsound->wfx);
        else {
            TRACE("invalid parameter: lpdwSizeWritten = NULL\n");
            return DSERR_INVALIDPARAM;
        }
    }

    TRACE("returning DS_OK\n");
    return DS_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_GetStatus(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    LPDWORD lpdwStatus )
{
    ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);
    TRACE( "(%p, %p), thread is %04lx\n", This, lpdwStatus, GetCurrentThreadId() );

    if (This == NULL) {
        WARN("invalid parameter: This == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (This->dsound == NULL) {
        WARN("invalid parameter: This->dsound == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (lpdwStatus == NULL) {
        WARN("invalid parameter: lpdwStatus == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    *lpdwStatus = 0;
    EnterCriticalSection(&(This->dsound->lock));

    TRACE("old This->dsound->state=%ld, old lpdwStatus=%08lx\n",This->dsound->state,*lpdwStatus);
    if ((This->dsound->state == STATE_STARTING) || 
        (This->dsound->state == STATE_CAPTURING)) {
        *lpdwStatus |= DSCBSTATUS_CAPTURING;
        if (This->flags & DSCBSTART_LOOPING)
            *lpdwStatus |= DSCBSTATUS_LOOPING;
    }
    TRACE("new This->dsound->state=%ld, new lpdwStatus=%08lx\n",This->dsound->state,*lpdwStatus);
    LeaveCriticalSection(&(This->dsound->lock));

    TRACE("status=%lx\n", *lpdwStatus);
    TRACE("returning DS_OK\n");
    return DS_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_Initialize(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    LPDIRECTSOUNDCAPTURE lpDSC,
    LPCDSCBUFFERDESC lpcDSCBDesc )
{
    ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);

    FIXME( "(%p,%p,%p): stub\n", This, lpDSC, lpcDSCBDesc );

    return DS_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_Lock(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    DWORD dwReadCusor,
    DWORD dwReadBytes,
    LPVOID* lplpvAudioPtr1,
    LPDWORD lpdwAudioBytes1,
    LPVOID* lplpvAudioPtr2,
    LPDWORD lpdwAudioBytes2,
    DWORD dwFlags )
{
    HRESULT err = DS_OK;
    ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);
    TRACE( "(%p,%08lu,%08lu,%p,%p,%p,%p,0x%08lx) at %ld\n", This, dwReadCusor,
        dwReadBytes, lplpvAudioPtr1, lpdwAudioBytes1, lplpvAudioPtr2,
        lpdwAudioBytes2, dwFlags, GetTickCount() );

    if (This == NULL) {
        WARN("invalid parameter: This == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (This->dsound == NULL) {
        WARN("invalid parameter: This->dsound == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (lplpvAudioPtr1 == NULL) {
        WARN("invalid parameter: lplpvAudioPtr1 == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (lpdwAudioBytes1 == NULL) {
        WARN("invalid parameter: lpdwAudioBytes1 == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    EnterCriticalSection(&(This->dsound->lock));

    if (This->dsound->driver) {
        err = IDsCaptureDriverBuffer_Lock(This->dsound->hwbuf, lplpvAudioPtr1, 
					   lpdwAudioBytes1, lplpvAudioPtr2, lpdwAudioBytes2,
					   dwReadCusor, dwReadBytes, dwFlags);
	if (err != DS_OK)
	    WARN("IDsCaptureDriverBuffer_Lock failed\n");
    } else if (This->dsound->hwi) {
        *lplpvAudioPtr1 = This->dsound->buffer + dwReadCusor;
        if ( (dwReadCusor + dwReadBytes) > This->dsound->buflen) {
            *lpdwAudioBytes1 = This->dsound->buflen - dwReadCusor;
	    if (lplpvAudioPtr2)
            	*lplpvAudioPtr2 = This->dsound->buffer;
	    if (lpdwAudioBytes2)
		*lpdwAudioBytes2 = dwReadBytes - *lpdwAudioBytes1;
        } else {
            *lpdwAudioBytes1 = dwReadBytes;
	    if (lplpvAudioPtr2)
            	*lplpvAudioPtr2 = 0;
	    if (lpdwAudioBytes2)
            	*lpdwAudioBytes2 = 0;
        }
    } else {
        TRACE("invalid call\n");
        err = DSERR_INVALIDCALL;   /* DSERR_NODRIVER ? */
    }

    LeaveCriticalSection(&(This->dsound->lock));

    return err;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_Start(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    DWORD dwFlags )
{
    HRESULT err = DS_OK;
    ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);
    TRACE( "(%p,0x%08lx)\n", This, dwFlags );

    if (This == NULL) {
        WARN("invalid parameter: This == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (This->dsound == NULL) {
        WARN("invalid parameter: This->dsound == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if ( (This->dsound->driver == 0) && (This->dsound->hwi == 0) ) {
        WARN("no driver\n");
        return DSERR_NODRIVER;
    }

    EnterCriticalSection(&(This->dsound->lock));

    This->flags = dwFlags;
    TRACE("old This->dsound->state=%ld\n",This->dsound->state);
    if (This->dsound->state == STATE_STOPPED)
        This->dsound->state = STATE_STARTING;
    else if (This->dsound->state == STATE_STOPPING)
        This->dsound->state = STATE_CAPTURING;
    TRACE("new This->dsound->state=%ld\n",This->dsound->state);

    LeaveCriticalSection(&(This->dsound->lock));

    if (This->dsound->driver) {
        err = IDsCaptureDriverBuffer_Start(This->dsound->hwbuf, dwFlags);
	if (err != DS_OK)
	    WARN("IDsCaptureDriverBuffer_Start failed\n");
        return err;
    } else {
        IDirectSoundCaptureImpl* ipDSC = This->dsound;

        if (ipDSC->buffer) {
            if (This->nrofnotifies) {
            	unsigned c;

		ipDSC->nrofpwaves = This->nrofnotifies;

                /* prepare headers */
                ipDSC->pwave = HeapReAlloc(GetProcessHeap(),0,ipDSC->pwave,
                    ipDSC->nrofpwaves*sizeof(WAVEHDR));

                for (c = 0; c < ipDSC->nrofpwaves; c++) {
                    if (c == 0) {
                        ipDSC->pwave[0].lpData = ipDSC->buffer;
                        ipDSC->pwave[0].dwBufferLength = 
                            This->notifies[0].dwOffset + 1;
                    } else {
                        ipDSC->pwave[c].lpData = ipDSC->buffer + 
                            This->notifies[c-1].dwOffset + 1;
                        ipDSC->pwave[c].dwBufferLength = 
                            This->notifies[c].dwOffset - 
                            This->notifies[c-1].dwOffset;
                    }
                    ipDSC->pwave[c].dwUser = (DWORD)ipDSC;
                    ipDSC->pwave[c].dwFlags = 0;
                    ipDSC->pwave[c].dwLoops = 0;
                    err = mmErr(waveInPrepareHeader(ipDSC->hwi,
                        &(ipDSC->pwave[c]),sizeof(WAVEHDR)));
                    if (err != DS_OK) {
                        while (c--)
                            waveInUnprepareHeader(ipDSC->hwi,
                                &(ipDSC->pwave[c]),sizeof(WAVEHDR));
                        break;
                    }
                }

                memset(ipDSC->buffer, 
                    (ipDSC->wfx.wBitsPerSample == 16) ? 0 : 128, ipDSC->buflen);
            } else {
		TRACE("no notifiers specified\n");
		/* no notifiers specified so just create a single default header */
		ipDSC->nrofpwaves = 1;
                ipDSC->pwave = HeapReAlloc(GetProcessHeap(),0,ipDSC->pwave,sizeof(WAVEHDR));
                ipDSC->pwave[0].lpData = ipDSC->buffer;
                ipDSC->pwave[0].dwBufferLength = ipDSC->buflen; 
                ipDSC->pwave[0].dwUser = (DWORD)ipDSC;
                ipDSC->pwave[0].dwFlags = 0;
                ipDSC->pwave[0].dwLoops = 0;

                err = mmErr(waveInPrepareHeader(ipDSC->hwi,
                    &(ipDSC->pwave[0]),sizeof(WAVEHDR)));
                if (err != DS_OK) {
		    WARN("waveInPrepareHeader failed\n");
                    waveInUnprepareHeader(ipDSC->hwi,
                        &(ipDSC->pwave[0]),sizeof(WAVEHDR));
		}
	    }
        }

        ipDSC->index = 0;
        ipDSC->read_position = 0;

	if (err == DS_OK) {
	    err = mmErr(waveInReset(ipDSC->hwi));
	    if (err == DS_OK) {
	        /* add the first buffer to the queue */
	        err = mmErr(waveInAddBuffer(ipDSC->hwi, &(ipDSC->pwave[0]), sizeof(WAVEHDR)));
	        if (err == DS_OK) {
	    	    /* start filling the first buffer */
	            err = mmErr(waveInStart(ipDSC->hwi));
		} else
		    WARN("waveInAddBuffer failed\n");
	    } else
		WARN("waveInReset failed\n");
        }
    }

    if (err != DS_OK) {
	WARN("calling waveInClose because of error\n");
	waveInClose(This->dsound->hwi);
	This->dsound->hwi = 0;
    }

    TRACE("returning %ld\n", err);
    return err;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_Stop( LPDIRECTSOUNDCAPTUREBUFFER8 iface )
{
    HRESULT err = DS_OK;
    ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);
    TRACE( "(%p)\n", This );

    if (This == NULL) {
        WARN("invalid parameter: This == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (This->dsound == NULL) {
        WARN("invalid parameter: This->dsound == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    EnterCriticalSection(&(This->dsound->lock));

    TRACE("old This->dsound->state=%ld\n",This->dsound->state);
    if (This->dsound->state == STATE_CAPTURING)
	This->dsound->state = STATE_STOPPING;
    else if (This->dsound->state == STATE_STARTING)
	This->dsound->state = STATE_STOPPED;
    TRACE("new This->dsound->state=%ld\n",This->dsound->state);

    LeaveCriticalSection(&(This->dsound->lock));

    if (This->dsound->driver) {
        err = IDsCaptureDriverBuffer_Stop(This->dsound->hwbuf);
	if (err == DSERR_BUFFERLOST) {
	    /* Wine-only: the driver wants us to reopen the device */
	    IDsCaptureDriverBuffer_Release(This->dsound->hwbuf);
	    err = IDsCaptureDriver_CreateCaptureBuffer(This->dsound->driver,
		&(This->dsound->wfx),0,0,&(This->dsound->buflen),&(This->dsound->buffer),
		(LPVOID*)&(This->dsound->hwbuf));
	    if (err != DS_OK) {
		WARN("IDsCaptureDriver_CreateCaptureBuffer failed\n");
		This->dsound->hwbuf = 0;
	    }
	} else if (err != DS_OK) 
	    WARN("IDsCaptureDriverBuffer_Stop failed\n");
    } else if (This->dsound->hwi) {
        err = waveInStop(This->dsound->hwi);
    } else {
	WARN("no driver\n");
        err = DSERR_NODRIVER;
    }

    TRACE( "(%p) returning 0x%08lx\n", This,err);
    return err;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_Unlock(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    LPVOID lpvAudioPtr1,
    DWORD dwAudioBytes1,
    LPVOID lpvAudioPtr2,
    DWORD dwAudioBytes2 )
{
    ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);
    TRACE( "(%p,%p,%08lu,%p,%08lu)\n", This, lpvAudioPtr1, dwAudioBytes1, 
        lpvAudioPtr2, dwAudioBytes2 );

    if (This == NULL) {
        WARN("invalid parameter: This == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (lpvAudioPtr1 == NULL) {
        WARN("invalid parameter: lpvAudioPtr1 == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (This->dsound->driver) {
	HRESULT	hres;
        hres = IDsCaptureDriverBuffer_Unlock(This->dsound->hwbuf, lpvAudioPtr1, 
					   dwAudioBytes1, lpvAudioPtr2, dwAudioBytes2);
	if (hres != DS_OK)
	    WARN("IDsCaptureDriverBuffer_Unlock failed\n");
	return hres;
    } else if (This->dsound->hwi) {
        This->dsound->read_position = (This->dsound->read_position + 
            (dwAudioBytes1 + dwAudioBytes2)) % This->dsound->buflen;
    } else {
        WARN("invalid call\n");
        return DSERR_INVALIDCALL;
    }

    return DS_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_GetObjectInPath(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    REFGUID rguidObject,
    DWORD dwIndex,
    REFGUID rguidInterface,
    LPVOID* ppObject )
{
    ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);

    FIXME( "(%p,%s,%lu,%s,%p): stub\n", This, debugstr_guid(rguidObject), 
        dwIndex, debugstr_guid(rguidInterface), ppObject );

    return DS_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_GetFXStatus(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    DWORD dwFXCount,
    LPDWORD pdwFXStatus )
{
    ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);

    FIXME( "(%p,%lu,%p): stub\n", This, dwFXCount, pdwFXStatus );

    return DS_OK;
}

static ICOM_VTABLE(IDirectSoundCaptureBuffer8) dscbvt =
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
    IDirectSoundCaptureBufferImpl_Unlock,

    /* IDirectSoundCaptureBuffer methods */
    IDirectSoundCaptureBufferImpl_GetObjectInPath,
    IDirectSoundCaptureBufferImpl_GetFXStatus
};

/*******************************************************************************
 * DirectSoundCapture ClassFactory
 */

static HRESULT WINAPI
DSCCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) 
{
    ICOM_THIS(IClassFactoryImpl,iface);

    FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI
DSCCF_AddRef(LPCLASSFACTORY iface)
{
    ICOM_THIS(IClassFactoryImpl,iface);
    TRACE("(%p) ref was %ld\n", This, This->ref);
    return ++(This->ref);
}

static ULONG WINAPI 
DSCCF_Release(LPCLASSFACTORY iface)
{
    ICOM_THIS(IClassFactoryImpl,iface);
    /* static class, won't be  freed */
    TRACE("(%p) ref was %ld\n", This, This->ref);
    return --(This->ref);
}

static HRESULT WINAPI 
DSCCF_CreateInstance(
	LPCLASSFACTORY iface,LPUNKNOWN pOuter,REFIID riid,LPVOID *ppobj )
{
    ICOM_THIS(IClassFactoryImpl,iface);
    TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);

    if (ppobj == NULL) {
	WARN("invalid parameter\n");
	return E_INVALIDARG;
    }

    *ppobj = NULL;

    if ( IsEqualGUID( &IID_IDirectSoundCapture, riid ) ||
	 IsEqualGUID( &IID_IDirectSoundCapture8, riid ) ) {
	return DirectSoundCaptureCreate8(0,(LPDIRECTSOUNDCAPTURE8*)ppobj,pOuter);
    }

    WARN("(%p,%p,%s,%p) Interface not found!\n",This,pOuter,debugstr_guid(riid),ppobj);	
    return E_NOINTERFACE;
}

static HRESULT WINAPI 
DSCCF_LockServer(LPCLASSFACTORY iface,BOOL dolock)
{
    ICOM_THIS(IClassFactoryImpl,iface);
    FIXME("(%p)->(%d),stub!\n",This,dolock);
    return S_OK;
}

static ICOM_VTABLE(IClassFactory) DSCCF_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    DSCCF_QueryInterface,
    DSCCF_AddRef,
    DSCCF_Release,
    DSCCF_CreateInstance,
    DSCCF_LockServer
};

IClassFactoryImpl DSOUND_CAPTURE_CF = { &DSCCF_Vtbl, 1 };

/***************************************************************************
 * DirectSoundFullDuplexCreate8 [DSOUND.10]
 *
 * Create and initialize a DirectSoundFullDuplex interface.
 *
 * PARAMS
 *    pcGuidCaptureDevice [I] Address of sound capture device GUID. 
 *    pcGuidRenderDevice  [I] Address of sound render device GUID.
 *    pcDSCBufferDesc     [I] Address of capture buffer description.
 *    pcDSBufferDesc      [I] Address of  render buffer description.
 *    hWnd                [I] Handle to application window.
 *    dwLevel             [I] Cooperative level.
 *    ppDSFD              [O] Address where full duplex interface returned.
 *    ppDSCBuffer8        [0] Address where capture buffer interface returned.
 *    ppDSBuffer8         [0] Address where render buffer interface returned.
 *    pUnkOuter           [I] Must be NULL.
 *
 * RETURNS
 *    Success: DS_OK
 *    Failure: DSERR_NOAGGREGATION, DSERR_ALLOCATED, DSERR_INVALIDPARAM,
 *             DSERR_OUTOFMEMORY DSERR_INVALIDCALL DSERR_NODRIVER
 */
HRESULT WINAPI 
DirectSoundFullDuplexCreate8(
    LPCGUID pcGuidCaptureDevice,
    LPCGUID pcGuidRenderDevice,
    LPCDSCBUFFERDESC pcDSCBufferDesc,
    LPCDSBUFFERDESC pcDSBufferDesc,
    HWND hWnd,
    DWORD dwLevel,
    LPDIRECTSOUNDFULLDUPLEX *ppDSFD,
    LPDIRECTSOUNDCAPTUREBUFFER8 *ppDSCBuffer8,
    LPDIRECTSOUNDBUFFER8 *ppDSBuffer8,
    LPUNKNOWN pUnkOuter)
{
    IDirectSoundFullDuplexImpl** ippDSFD=(IDirectSoundFullDuplexImpl**)ppDSFD;
    TRACE("(%s,%s,%p,%p,%lx,%lx,%p,%p,%p,%p)\n", debugstr_guid(pcGuidCaptureDevice), 
	debugstr_guid(pcGuidRenderDevice), pcDSCBufferDesc, pcDSBufferDesc,
	(DWORD)hWnd, dwLevel, ppDSFD, ppDSCBuffer8, ppDSBuffer8, pUnkOuter);

    if ( pUnkOuter ) {
	WARN("pUnkOuter != 0\n");
        return DSERR_NOAGGREGATION;
    }

    *ippDSFD = (IDirectSoundFullDuplexImpl*)HeapAlloc(GetProcessHeap(),
	HEAP_ZERO_MEMORY, sizeof(IDirectSoundFullDuplexImpl));

    if (*ippDSFD == NULL) {
	WARN("out of memory\n");
	return DSERR_OUTOFMEMORY;
    } else {
	HRESULT hres;
        ICOM_THIS(IDirectSoundFullDuplexImpl, *ippDSFD);

        This->ref = 1;
        This->lpVtbl = &dsfdvt;

        InitializeCriticalSection( &(This->lock) );

        hres = IDirectSoundFullDuplexImpl_Initialize( (LPDIRECTSOUNDFULLDUPLEX)This,
                                                      pcGuidCaptureDevice, pcGuidRenderDevice,
                                                      pcDSCBufferDesc, pcDSBufferDesc,
                                                      hWnd, dwLevel, ppDSCBuffer8, ppDSBuffer8);
	if (hres != DS_OK)
	    WARN("IDirectSoundFullDuplexImpl_Initialize failed\n");
	return hres;
    }

    return DSERR_GENERIC;
}

static HRESULT WINAPI
IDirectSoundFullDuplexImpl_QueryInterface(
    LPDIRECTSOUNDFULLDUPLEX iface,
    REFIID riid,
    LPVOID* ppobj )
{
    ICOM_THIS(IDirectSoundFullDuplexImpl,iface);
    TRACE( "(%p,%s,%p)\n", This, debugstr_guid(riid), ppobj );

    if (ppobj == NULL) {
	WARN("invalid parameter\n");
	return E_INVALIDARG;
    }

    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI
IDirectSoundFullDuplexImpl_AddRef( LPDIRECTSOUNDFULLDUPLEX iface )
{
    ULONG uRef;
    ICOM_THIS(IDirectSoundFullDuplexImpl,iface);
    TRACE("(%p) ref was %ld, thread is %04lx\n",This, This->ref, GetCurrentThreadId());

    EnterCriticalSection( &(This->lock) );

    uRef = ++(This->ref);

    LeaveCriticalSection( &(This->lock) );

    return uRef;
}

static ULONG WINAPI
IDirectSoundFullDuplexImpl_Release( LPDIRECTSOUNDFULLDUPLEX iface )
{
    ULONG uRef;
    ICOM_THIS(IDirectSoundFullDuplexImpl,iface);
    TRACE("(%p) ref was %ld, thread is %04lx\n",This, This->ref, GetCurrentThreadId());

    EnterCriticalSection( &(This->lock) );

    uRef = --(This->ref);

    LeaveCriticalSection( &(This->lock) );

    if ( uRef == 0 ) {
        DeleteCriticalSection( &(This->lock) );
        HeapFree( GetProcessHeap(), 0, This );
	TRACE("(%p) released\n",This);
    }

    return uRef;
}

static HRESULT WINAPI
IDirectSoundFullDuplexImpl_Initialize(
    LPDIRECTSOUNDFULLDUPLEX iface,
    LPCGUID pCaptureGuid,
    LPCGUID pRendererGuid,
    LPCDSCBUFFERDESC lpDscBufferDesc,
    LPCDSBUFFERDESC lpDsBufferDesc,
    HWND hWnd,
    DWORD dwLevel,
    LPLPDIRECTSOUNDCAPTUREBUFFER8 lplpDirectSoundCaptureBuffer8,
    LPLPDIRECTSOUNDBUFFER8 lplpDirectSoundBuffer8 )
{
    ICOM_THIS(IDirectSoundFullDuplexImpl,iface);
    IDirectSoundCaptureBufferImpl** ippdscb=(IDirectSoundCaptureBufferImpl**)lplpDirectSoundCaptureBuffer8;
    IDirectSoundBufferImpl** ippdsc=(IDirectSoundBufferImpl**)lplpDirectSoundBuffer8;

    FIXME( "(%p,%s,%s,%p,%p,%lx,%lx,%p,%p) stub!\n", This, debugstr_guid(pCaptureGuid), 
	debugstr_guid(pRendererGuid), lpDscBufferDesc, lpDsBufferDesc, (DWORD)hWnd, dwLevel,
	ippdscb, ippdsc);

    return E_FAIL;
}

static ICOM_VTABLE(IDirectSoundFullDuplex) dsfdvt =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    /* IUnknown methods */
    IDirectSoundFullDuplexImpl_QueryInterface,
    IDirectSoundFullDuplexImpl_AddRef,
    IDirectSoundFullDuplexImpl_Release,

    /* IDirectSoundFullDuplex methods */
    IDirectSoundFullDuplexImpl_Initialize
};

/*******************************************************************************
 * DirectSoundFullDuplex ClassFactory
 */

static HRESULT WINAPI
DSFDCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj)
{
    ICOM_THIS(IClassFactoryImpl,iface);

    FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI
DSFDCF_AddRef(LPCLASSFACTORY iface)
{
    ICOM_THIS(IClassFactoryImpl,iface);
    TRACE("(%p) ref was %ld\n", This, This->ref);
    return ++(This->ref);
}

static ULONG WINAPI 
DSFDCF_Release(LPCLASSFACTORY iface) 
{
    ICOM_THIS(IClassFactoryImpl,iface);
    /* static class, won't be  freed */
    TRACE("(%p) ref was %ld\n", This, This->ref);
    return --(This->ref);
}

static HRESULT WINAPI 
DSFDCF_CreateInstance(
    LPCLASSFACTORY iface,LPUNKNOWN pOuter,REFIID riid,LPVOID *ppobj )
{
    ICOM_THIS(IClassFactoryImpl,iface);

    TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);

    if (ppobj == NULL) {
	WARN("invalid parameter\n");
	return E_INVALIDARG;
    }

    *ppobj = NULL;

    if ( IsEqualGUID( &IID_IDirectSoundFullDuplex, riid ) ) {
	/* FIXME: how do we do this one ? */
	FIXME("not implemented\n");
	return E_NOINTERFACE;
    }

    WARN("(%p,%p,%s,%p) Interface not found!\n",This,pOuter,debugstr_guid(riid),ppobj);	
    return E_NOINTERFACE;
}

static HRESULT WINAPI 
DSFDCF_LockServer(LPCLASSFACTORY iface,BOOL dolock)
{
    ICOM_THIS(IClassFactoryImpl,iface);
    FIXME("(%p)->(%d),stub!\n",This,dolock);
    return S_OK;
}

static ICOM_VTABLE(IClassFactory) DSFDCF_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    DSFDCF_QueryInterface,
    DSFDCF_AddRef,
    DSFDCF_Release,
    DSFDCF_CreateInstance,
    DSFDCF_LockServer
};

IClassFactoryImpl DSOUND_FULLDUPLEX_CF = { &DSFDCF_Vtbl, 1 };
