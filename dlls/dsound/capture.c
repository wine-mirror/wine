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
 *    Implement DirectSoundCapture API
 */

#include "config.h"
#include <assert.h>
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
static HRESULT DSOUND_CreateDirectSoundCaptureBuffer(
    IDirectSoundCaptureImpl *ipDSC, 
    LPCDSCBUFFERDESC lpcDSCBufferDesc, 
    LPVOID* ppobj );

static ICOM_VTABLE(IDirectSoundCapture) dscvt;
static ICOM_VTABLE(IDirectSoundCaptureBuffer8) dscbvt;


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
HRESULT WINAPI 
DirectSoundCaptureCreate8(
    LPCGUID lpcGUID,
    LPDIRECTSOUNDCAPTURE* lplpDSC,
    LPUNKNOWN pUnkOuter )
{
    TRACE("(%s,%p,%p)\n", debugstr_guid(lpcGUID), lplpDSC, pUnkOuter);

    if ( pUnkOuter ) {
        TRACE("pUnkOuter != 0\n");
        return DSERR_NOAGGREGATION;
    }

    if ( !lplpDSC ) {
        TRACE("invalid parameter: lplpDSC == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    /* Default device? */
    if ( !lpcGUID || IsEqualGUID(lpcGUID, &GUID_NULL) ||
        IsEqualGUID(lpcGUID, &DSDEVID_DefaultCapture) ||
        IsEqualGUID(lpcGUID, &DSDEVID_DefaultVoiceCapture) ) {
        IDirectSoundCaptureImpl** ippDSC=(IDirectSoundCaptureImpl**)lplpDSC;

        *ippDSC = (IDirectSoundCaptureImpl*)HeapAlloc(GetProcessHeap(),
            HEAP_ZERO_MEMORY, sizeof(IDirectSoundCaptureImpl));

        if (*ippDSC == NULL) {
            TRACE("couldn't allocate memory\n");
            return DSERR_OUTOFMEMORY;
        }
        else
        {
            ICOM_THIS(IDirectSoundCaptureImpl, *ippDSC);

            This->ref = 1;
            This->state = STATE_STOPPED;

            if (lpcGUID)
                This->guid = *lpcGUID;
            else
                This->guid = GUID_NULL;

            InitializeCriticalSection( &(This->lock) );

            ICOM_VTBL(This) = &dscvt;

            /* FIXME: should this be defered because we can't return no driver ? */
            return IDirectSoundCaptureImpl_Initialize( (LPDIRECTSOUNDCAPTURE)This, lpcGUID );
        }
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
HRESULT WINAPI 
DirectSoundCaptureEnumerateA(
    LPDSENUMCALLBACKA lpDSEnumCallback,
    LPVOID lpContext)
{
    WAVEINCAPSA wcaps;
    unsigned devs, wid;

    TRACE("(%p,%p)\n", lpDSEnumCallback, lpContext );

    if (lpDSEnumCallback == NULL) {
        TRACE("invalid parameter\n");
        return DSERR_INVALIDPARAM;
    }

    devs = waveInGetNumDevs();
    for (wid = 0; wid < devs; ++wid) {
        waveInGetDevCapsA(wid, &wcaps, sizeof(wcaps));
        if (lpDSEnumCallback) {
            lpDSEnumCallback(NULL, "WINE Sound Capture Driver",
                wcaps.szPname ,lpContext);
            return DS_OK;
        }
    }

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
HRESULT WINAPI 
DirectSoundCaptureEnumerateW(
    LPDSENUMCALLBACKW lpDSEnumCallback,
    LPVOID lpContext)
{
    WAVEINCAPSW wcaps;
    unsigned devs, wid;
    WCHAR desc[MAXPNAMELEN];

    TRACE("(%p,%p)\n", lpDSEnumCallback, lpContext );

    if (lpDSEnumCallback == NULL) {
        TRACE("invalid parameter\n");
        return DSERR_INVALIDPARAM;
    }

    devs = waveInGetNumDevs();
    for (wid = 0; wid < devs; ++wid) {
        waveInGetDevCapsW(wid, &wcaps, sizeof(wcaps));
        if (lpDSEnumCallback) {
            MultiByteToWideChar( CP_ACP, 0, "WINE Sound Capture Driver", -1, 
                desc, sizeof(desc)/sizeof(WCHAR) );
            lpDSEnumCallback(NULL, desc, wcaps.szPname ,lpContext);
            return DS_OK;
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
    TRACE("entering at %ld, msg=%08x\n", GetTickCount(), msg);

    if (msg == MM_WIM_DATA) {
        This->index = (This->index + 1) % This->capture_buffer->nrofnotifies;
        waveInUnprepareHeader(hwi,&(This->pwave[This->index]),sizeof(WAVEHDR));
        SetEvent(This->capture_buffer->notifies[This->index].hEventNotify);
        waveInPrepareHeader(hwi,&(This->pwave[This->index]),sizeof(WAVEHDR));
        waveInAddBuffer(hwi, &(This->pwave[This->index]), sizeof(WAVEHDR));
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

    if (This->driver) 
        return IDsCaptureDriver_QueryInterface(This->driver, riid, ppobj);

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

    if (This->driver) 
        IDsCaptureDriver_AddRef(This->driver);

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
        TRACE("deleting object\n");
        if (This->capture_buffer)
            IDirectSoundCaptureBuffer8_Release(
                (LPDIRECTSOUNDCAPTUREBUFFER8)This->capture_buffer);

        if (This->driver) 
            IDsDriver_Close(This->driver);
        
        if (This->drvdesc.dwFlags & DSDDESC_DOMMSYSTEMOPEN) 
            waveInClose(This->hwi);

        if (This->driver)
            IDsDriver_Release(This->driver);

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

    TRACE( "(%p,%p,%p,%p)\n", This, lpcDSCBufferDesc, lplpDSCaptureBuffer, 
        pUnk );

    if ( (This == NULL) || (lpcDSCBufferDesc== NULL) || 
        (lplpDSCaptureBuffer == NULL) || pUnk ) {
        TRACE("invalid parameters\n");
        return DSERR_INVALIDPARAM;
    }

    /* FIXME: We can only have one buffer so what do we do here? */
    if (This->capture_buffer) {
            TRACE("already has buffer\n");
            return DSERR_INVALIDPARAM;    /* DSERR_GENERIC ? */
    }

    hr = DSOUND_CreateDirectSoundCaptureBuffer( This, lpcDSCBufferDesc, 
        (LPVOID*)lplpDSCaptureBuffer );

    return hr;
}

static HRESULT WINAPI
IDirectSoundCaptureImpl_GetCaps(
    LPDIRECTSOUNDCAPTURE iface,
    LPDSCCAPS lpDSCCaps )
{
    ICOM_THIS(IDirectSoundCaptureImpl,iface);
    TRACE("(%p,%p)\n",This,lpDSCCaps);

    if ( (lpDSCCaps== NULL) || (lpDSCCaps->dwSize != sizeof(*lpDSCCaps)) ) {
        TRACE("invalid parameters\n");
        return DSERR_INVALIDPARAM;
    }

    if ( !(This->initialized) ) {
        TRACE("not initialized\n");
        return DSERR_UNINITIALIZED;
    }

    if ( (This->driver = 0) || (This->hwi == 0) ) {
        TRACE("no driver\n");
        return DSERR_NODRIVER;
    }

       lpDSCCaps->dwFlags = This->drvcaps.dwFlags;
       lpDSCCaps->dwFormats = This->formats;
       lpDSCCaps->dwChannels = This->channels;

    TRACE("(flags=0x%08lx,format=0x%08lx,channels=%ld)\n",lpDSCCaps->dwFlags,
        lpDSCCaps->dwFormats, lpDSCCaps->dwChannels);

    return DS_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureImpl_Initialize(
    LPDIRECTSOUNDCAPTURE iface,
    LPCGUID lpcGUID )
{
    HRESULT err = DS_OK;
    unsigned wid, widn;
    ICOM_THIS(IDirectSoundCaptureImpl,iface);
    TRACE("(%p)\n", This);

    if (!This) {
        TRACE("invalid parameter\n");
        return DSERR_INVALIDPARAM;
    }

    if (This->initialized) {
        TRACE("already initialized\n");
        return DSERR_ALREADYINITIALIZED;
    }

    widn = waveInGetNumDevs();

    if (!widn) { 
        TRACE("no audio devices found\n");
        return DSERR_NODRIVER;
    }

    /* FIXME: should enumerate WINMM audio devices and find the one we want */
    wid = 0;

    err = mmErr(waveInMessage((HWAVEIN)wid,DRV_QUERYDSOUNDIFACE,(DWORD)&(This->driver),0));
    if ( (err != DS_OK) && (err != DSERR_UNSUPPORTED) ) {
        TRACE("waveInMessage failed; err=%lx\n",err);
        return err;
    }
    err = DS_OK;

    /* Get driver description */
    if (This->driver) {
        TRACE("using DirectSound driver\n");
        IDsDriver_GetDriverDesc(This->driver, &(This->drvdesc));
    } else {
        TRACE("using WINMM\n");
        /* if no DirectSound interface available, use WINMM API instead */
        This->drvdesc.dwFlags = DSDDESC_DOMMSYSTEMOPEN | 
            DSDDESC_DOMMSYSTEMSETFORMAT;
        This->drvdesc.dnDevNode = wid; /* FIXME? */
    }
    
    /* If the driver requests being opened through MMSYSTEM
     * (which is recommended by the DDK), it is supposed to happen
     * before the DirectSound interface is opened */
    if (This->drvdesc.dwFlags & DSDDESC_DOMMSYSTEMOPEN)
    {
        /* FIXME: is this right? */
        This->drvdesc.dnDevNode = 0;

#if 0    /* this returns an uninitialized structure so don't use */
        /* query the wave format */
        err = mmErr(waveInOpen(NULL, This->drvdesc.dnDevNode, &(This->wfx),
            0, 0, WAVE_FORMAT_QUERY));
#else                        
        /* Set default wave format for waveInOpen */
        This->wfx.wFormatTag    = WAVE_FORMAT_PCM;
        /* We rely on the sound driver to return the actual sound format of
         * the device if it does not support 22050x8x2 and is given the
         * WAVE_DIRECTSOUND flag.
         */
        This->wfx.nSamplesPerSec = 22050;
        This->wfx.wBitsPerSample = 8;
        This->wfx.nChannels = 2;
        This->wfx.nBlockAlign = This->wfx.wBitsPerSample * 
            This->wfx.nChannels / 8;
        This->wfx.nAvgBytesPerSec = This->wfx.nSamplesPerSec * 
            This->wfx.nBlockAlign;
        This->wfx.cbSize = 0;
#endif
        err = mmErr(waveInOpen(&(This->hwi),
            This->drvdesc.dnDevNode, &(This->wfx),
            (DWORD)DSOUND_capture_callback, (DWORD)This,
            CALLBACK_FUNCTION | WAVE_DIRECTSOUND));
    }

    /* open the DirectSound driver if available */
    if (This->driver && (err == DS_OK))
        err = IDsDriver_Open(This->driver);

    if (err == DS_OK) {
        This->initialized = TRUE;

        /* the driver is now open, so it's now allowed to call GetCaps */
        if (This->driver) {
            IDsDriver_GetCaps(This->driver,&(This->drvcaps));
        } else if (This->hwi) {
            WAVEINCAPSA    wic;
            err = mmErr(waveInGetDevCapsA((UINT)This->hwi, &wic, sizeof(wic)));

            if (err == DS_OK) {
                This->drvcaps.dwFlags = 0;
                strncpy(This->drvdesc.szDrvName, wic.szPname, 
                    sizeof(This->drvdesc.szDrvName)); 

                This->formats = wic.dwFormats;
                This->channels = wic.wChannels;
    
                if (ds_emuldriver)
                    This->drvcaps.dwFlags |= DSCCAPS_EMULDRIVER;
            }
        } else {
            TRACE("no audio devices found\n");
            return DSERR_NODRIVER;
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

    if ( (ipDSC == NULL) || (lpcDSCBufferDesc == NULL) || (ppobj == NULL) ) {
        TRACE("invalid parameters\n");
        return DSERR_INVALIDPARAM;
    }

    if ( (lpcDSCBufferDesc->dwSize < sizeof(DSCBUFFERDESC)) || 
        (lpcDSCBufferDesc->dwBufferBytes == 0) ||
        (lpcDSCBufferDesc->lpwfxFormat == NULL) ) {
        TRACE("invalid lpcDSCBufferDesc\n");
        return DSERR_INVALIDPARAM;
    }

    if ( !ipDSC->initialized ) {
        TRACE("not initialized\n");

        return DSERR_UNINITIALIZED;
    }

    if ( (ipDSC->driver == 0) && (ipDSC->hwi == 0) ) {
        TRACE("no driver\n");
        return DSERR_NODRIVER;
    }

    wfex = lpcDSCBufferDesc->lpwfxFormat;

    if (wfex) {
        TRACE("(formattag=0x%04x,chans=%d,samplerate=%ld,"
            "bytespersec=%ld,blockalign=%d,bitspersamp=%d,cbSize=%d)\n",
            wfex->wFormatTag, wfex->nChannels, wfex->nSamplesPerSec,
            wfex->nAvgBytesPerSec, wfex->nBlockAlign,
            wfex->wBitsPerSample, wfex->cbSize);

        if (wfex->cbSize == 0)
            memcpy(&(ipDSC->wfx), wfex, sizeof(*wfex) + wfex->cbSize);
        else {
            ERR("non PCM formats not supported\n");
            return DSERR_BADFORMAT; /* FIXME: DSERR_INVALIDPARAM ? */
        }
    } else {
        TRACE("lpcDSCBufferDesc->lpwfxFormat == 0\n");
        return DSERR_INVALIDPARAM; /* FIXME: DSERR_BADFORMAT ? */
    }

    *ppobj = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,
        sizeof(IDirectSoundCaptureBufferImpl));

    if ( *ppobj == NULL ) {
        TRACE("out of memory\n");
        return DSERR_OUTOFMEMORY;
    } else {
        ICOM_THIS(IDirectSoundCaptureBufferImpl,*ppobj);

        This->ref = 1;
        This->dsound = ipDSC;
        This->dsound->capture_buffer = This;

        This->pdscbd = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,
            lpcDSCBufferDesc->dwSize);
        if (This->pdscbd)     
            memcpy(This->pdscbd, lpcDSCBufferDesc, lpcDSCBufferDesc->dwSize);
        else {
            TRACE("no memory\n");
            This->dsound->capture_buffer = 0;
            HeapFree( GetProcessHeap(), 0, This );
            *ppobj = NULL;
            return DSERR_OUTOFMEMORY; 
        }

        ICOM_VTBL(This) = &dscbvt;

        InitializeCriticalSection( &This->lock );
    }

    return DS_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_QueryInterface(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    REFIID riid,
    LPVOID* ppobj )
{
    ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);
    TRACE( "(%p,%s,%p)\n", This, debugstr_guid(riid), ppobj );

    if (IsEqualGUID( &IID_IDirectSoundNotify, riid ) ) {
        IDirectSoundNotifyImpl  *dsn;

        dsn = (IDirectSoundNotifyImpl*)HeapAlloc(GetProcessHeap(),0,
            sizeof(*dsn));
        dsn->ref = 1;
        dsn->dsb = 0;
        dsn->dscb = This;
        IDirectSoundCaptureBuffer8_AddRef(iface);
        ICOM_VTBL(dsn) = &dsnvt;
        *ppobj = (LPVOID)dsn;
        return DS_OK;
    }

    FIXME("(%p,%s,%p)\n", This, debugstr_guid(riid), ppobj);

    return E_FAIL;
}

static ULONG WINAPI
IDirectSoundCaptureBufferImpl_AddRef( LPDIRECTSOUNDCAPTUREBUFFER8 iface )
{
    ULONG uRef;
    ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);
    TRACE( "(%p)\n", This );

    EnterCriticalSection( &This->lock );

    TRACE( "(%p) was 0x%08lx\n", This, This->ref );
    uRef = ++(This->ref);

    LeaveCriticalSection( &This->lock );

    return uRef;
}

static ULONG WINAPI
IDirectSoundCaptureBufferImpl_Release( LPDIRECTSOUNDCAPTUREBUFFER8 iface )
{
    ULONG uRef;
    ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);
    TRACE( "(%p)\n", This );

    EnterCriticalSection( &This->lock );

    TRACE( "(%p) was 0x%08lx\n", This, This->ref );
    uRef = --(This->ref);

    LeaveCriticalSection( &This->lock );

    if ( uRef == 0 ) {
        if (This->pdscbd)
            HeapFree(GetProcessHeap(),0, This->pdscbd);

        /* remove from IDirectSoundCaptureImpl */
        if (This->dsound)
            This->dsound->capture_buffer = NULL;
        else
            ERR("does not reference dsound\n");

        if (This->notifies)
            HeapFree(GetProcessHeap(),0, This->notifies);
        
        DeleteCriticalSection( &This->lock );
        HeapFree( GetProcessHeap(), 0, This );
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

    if ( (This == NULL) || (lpDSCBCaps == NULL) ) {
        TRACE("invalid parameters\n");
        return DSERR_INVALIDPARAM;
    }

    if ( (lpDSCBCaps->dwSize < sizeof(DSCBCAPS)) || (This->dsound == NULL) ) {
        TRACE("invalid parameters\n");
        return DSERR_INVALIDPARAM;
    }

    lpDSCBCaps->dwSize = sizeof(DSCBCAPS);
    lpDSCBCaps->dwFlags = This->flags;
    lpDSCBCaps->dwBufferBytes = This->pdscbd->dwBufferBytes;
    lpDSCBCaps->dwReserved = 0;

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

    if ( (This == NULL) || (This->dsound == NULL) || 
        (lpdwReadPosition == NULL) ) {
        TRACE("invalid parameter\n");
        return DSERR_INVALIDPARAM;
    }

    if (This->dsound->driver) {
        FIXME( "(%p,%p,%p): direct sound driver not supported\n", 
            This, lpdwCapturePosition, lpdwReadPosition );
        return DSERR_NODRIVER;
    } else if (This->dsound->hwi) {
        if (lpdwCapturePosition) {
            MMTIME mtime;
            mtime.wType = TIME_BYTES;
            waveInGetPosition(This->dsound->hwi, &mtime, sizeof(mtime));
            mtime.u.cb = mtime.u.cb % This->dsound->buflen;
            *lpdwCapturePosition = mtime.u.cb;
        }
    
        if (lpdwReadPosition) {
            if (This->dsound->state == STATE_STARTING) { 
                This->dsound->read_position = *lpdwCapturePosition;
                This->dsound->state = STATE_CAPTURING;
            } 
            *lpdwReadPosition = This->dsound->read_position;
        }
    } else {
        TRACE("no driver\n");
        return DSERR_NODRIVER;
    }
    
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

    if ( (This == NULL) || (This->dsound == NULL) ) {
        TRACE("invalid parameter\n");
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
            TRACE("invalid parameter\n");
            return DSERR_INVALIDPARAM;
        }
    }

    return DS_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_GetStatus(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    LPDWORD lpdwStatus )
{
    ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);
    TRACE( "(%p, %p)\n", This, lpdwStatus );

    if ( (This == NULL ) || (This->dsound == NULL) || (lpdwStatus == NULL) ) {
        TRACE("invalid parameter\n");
        return DSERR_INVALIDPARAM;
    }

    *lpdwStatus = 0;
    if ((This->dsound->state == STATE_STARTING) || 
        (This->dsound->state == STATE_CAPTURING)) {
        *lpdwStatus |= DSCBSTATUS_CAPTURING;
        if (This->flags & DSCBSTART_LOOPING)
            *lpdwStatus |= DSCBSTATUS_LOOPING;
    }

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
    ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);
    TRACE( "(%p,%08lu,%08lu,%p,%p,%p,%p,0x%08lx)\n", This, dwReadCusor,
        dwReadBytes, lplpvAudioPtr1, lpdwAudioBytes1, lplpvAudioPtr2,
        lpdwAudioBytes2, dwFlags );

    if ( (This == NULL) || (This->dsound == NULL) || (lplpvAudioPtr1 == NULL) ||
        (lpdwAudioBytes1 == NULL) || (lplpvAudioPtr2 == NULL) || 
        (lpdwAudioBytes2 == NULL) ) {
        TRACE("invalid parameter\n");
        return DSERR_INVALIDPARAM;
    }

    if (This->dsound->driver) {
        FIXME("direct sound driver not supported\n");
        return DSERR_INVALIDCALL;    /* DSERR_NODRIVER ? */
    } else if (This->dsound->hwi) {
        *lplpvAudioPtr1 = This->dsound->buffer + dwReadCusor;
        if ( (dwReadCusor + dwReadBytes) > This->dsound->buflen) {
            *lpdwAudioBytes1 = This->dsound->buflen - dwReadCusor;
            *lplpvAudioPtr2 = This->dsound->buffer;
            *lpdwAudioBytes2 = dwReadBytes - *lpdwAudioBytes1;
        } else {
            *lpdwAudioBytes1 = dwReadBytes;
            *lplpvAudioPtr2 = 0;
            *lpdwAudioBytes2 = 0;
        }
    } else {
        TRACE("invalid call\n");
        return DSERR_INVALIDCALL;   /* DSERR_NODRIVER ? */
    }

    return DS_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_Start(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    DWORD dwFlags )
{
    HRESULT err = DS_OK;
    ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);
    TRACE( "(%p,0x%08lx)\n", This, dwFlags );

    if ( (This == NULL) || (dwFlags != DSCBSTART_LOOPING) || 
        (This->dsound == NULL) ) {
        TRACE("invalid parameter\n");
        return DSERR_INVALIDPARAM;
    }

    if ( (This->dsound->driver == 0) && (This->dsound->hwi == 0) ) {
        TRACE("no driver\n");
        return DSERR_NODRIVER;
    }

    EnterCriticalSection(&(This->lock));

    This->flags = dwFlags;
    if (This->dsound->state == STATE_STOPPED)
        This->dsound->state = STATE_STARTING;
    else if (This->dsound->state == STATE_STOPPING)
        This->dsound->state = STATE_CAPTURING;

    if (This->dsound->driver) {
        FIXME("direct sound driver not supported\n");
        LeaveCriticalSection(&(This->lock));
        return DSERR_NODRIVER;
    } else {
        LPBYTE newbuf;
        DWORD buflen;

        IDirectSoundCaptureImpl* ipDSC = This->dsound;

        /* FIXME: do this only if formats different */
        err = mmErr(waveInClose(ipDSC->hwi));
        if (err != DS_OK) {
            TRACE("waveInClose failed\n");
            LeaveCriticalSection(&(This->lock));
            return DSERR_GENERIC;
        }

        err = mmErr(waveInOpen(&(ipDSC->hwi),
            ipDSC->drvdesc.dnDevNode, &(ipDSC->wfx),
            (DWORD)DSOUND_capture_callback, (DWORD)ipDSC,
            CALLBACK_FUNCTION | WAVE_DIRECTSOUND));
        if (err != DS_OK) {
            TRACE("waveInOpen failed\n");
            LeaveCriticalSection(&(This->lock));
            return DSERR_GENERIC;
        }

        buflen = This->pdscbd->dwBufferBytes;
        TRACE("desired buflen=%ld, old buffer=%p\n", buflen, ipDSC->buffer);
        newbuf = (LPBYTE)HeapReAlloc(GetProcessHeap(),0,ipDSC->buffer,buflen);

        if (newbuf == NULL) {
            ERR("failed to allocate capture buffer\n");
            err = DSERR_OUTOFMEMORY;
            /* but the old buffer might still exist and must be re-prepared */
        } else {
            ipDSC->buffer = newbuf;
            ipDSC->buflen = buflen;
        }

        if (ipDSC->buffer) {
            unsigned c;

            if (This->nrofnotifies)
            {
                /* prepare headers */
                ipDSC->pwave = HeapReAlloc(GetProcessHeap(),0,ipDSC->pwave,
                    This->nrofnotifies*sizeof(WAVEHDR));

                for (c = 0; c < This->nrofnotifies; c++) {
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
#if 0
                    err = mmErr(waveInAddBuffer(ipDSC->hwi, &(ipDSC->pwave[c]), 
                        sizeof(WAVEHDR)));
                    if (err != DS_OK) {
                        while (c--)
                            waveInUnprepareHeader(ipDSC->hwi,&(ipDSC->pwave[c]),
                                sizeof(WAVEHDR));
                        break;
                    }
#endif
                }

                memset(ipDSC->buffer, 
                    (ipDSC->wfx.wBitsPerSample == 16) ? 0 : 128, ipDSC->buflen);
            }
        }

        ipDSC->index = 0;
        ipDSC->read_position = 0;
        err = mmErr(waveInAddBuffer(ipDSC->hwi, &(ipDSC->pwave[ipDSC->index]),
            sizeof(WAVEHDR)));
        if (err == DS_OK)
            err = mmErr(waveInStart(ipDSC->hwi));
    }

    if (err != DS_OK) {
        FIXME("cleanup\n");
        if (This->dsound->driver) {
            FIXME("direct sound driver not supported\n");
        }
        if (This->dsound->hwi) {
            waveInClose(This->dsound->hwi);
        }
        LeaveCriticalSection(&(This->lock));
        return err;
    }

    LeaveCriticalSection(&(This->lock));

    return DS_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_Stop( LPDIRECTSOUNDCAPTUREBUFFER8 iface )
{
    ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);
    TRACE( "(%p)\n", This );

    if ( (This == NULL) || (This->dsound == NULL) ) {
        TRACE("invalid parameter\n");
        return DSERR_INVALIDPARAM;
    }

    if (This->dsound->driver) {
        FIXME("direct sound driver not supported\n");
        return DSERR_NODRIVER;
    } else if (This->dsound->hwi) {
        TRACE("stopping winmm\n");
        waveInStop(This->dsound->hwi);
    } else {
        TRACE("no driver\n");
        return DSERR_NODRIVER;
    }

    return DS_OK;
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

    if ( (This == NULL) || (lpvAudioPtr1 == NULL) ) {
        TRACE("invalid parameters\n");
        return DSERR_INVALIDPARAM;
    }

    if (This->dsound->driver) {
        TRACE("direct sound driver not supported\n");
        return DSERR_INVALIDCALL;
    } else if (This->dsound->hwi) {
        This->dsound->read_position = (This->dsound->read_position + 
            (dwAudioBytes1 + dwAudioBytes2)) % This->dsound->buflen;
    } else {
        TRACE("invalid call\n");
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
