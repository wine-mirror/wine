/*  			DirectSoundCapture
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
 *	Implement DirectSoundCapture API
 */

#include "config.h"
#include <assert.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "mmsystem.h"
#include "wine/debug.h"
#include "dsound.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
typedef struct IDirectSoundCaptureImpl IDirectSoundCaptureImpl;
typedef struct IDirectSoundCaptureBufferImpl IDirectSoundCaptureBufferImpl;


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
    ICOM_VFIELD(IDirectSoundCaptureBuffer8);
    DWORD                              ref;

    /* IDirectSoundCaptureBufferImpl fields */
    CRITICAL_SECTION        lock;
};


static HRESULT DSOUND_CreateDirectSoundCapture( LPVOID* ppobj );
static HRESULT DSOUND_CreateDirectSoundCaptureBuffer( LPCDSCBUFFERDESC lpcDSCBufferDesc, LPVOID* ppobj );

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
HRESULT WINAPI DirectSoundCaptureCreate8(
	LPCGUID lpcGUID,
	LPDIRECTSOUNDCAPTURE* lplpDSC,
	LPUNKNOWN pUnkOuter )
{
	TRACE("(%s,%p,%p)\n", debugstr_guid(lpcGUID), lplpDSC, pUnkOuter);

	if( pUnkOuter ) {
		return DSERR_NOAGGREGATION;
	}

	/* Default device? */
	if ( !lpcGUID ||
	     IsEqualGUID(lpcGUID, &DSDEVID_DefaultCapture) ||
	     IsEqualGUID(lpcGUID, &DSDEVID_DefaultVoiceCapture) ) {
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
        LPDIRECTSOUNDCAPTUREBUFFER8 iface,
        REFIID riid,
        LPVOID* ppobj )
{
        ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);

        FIXME( "(%p)->(%s,%p): stub\n", This, debugstr_guid(riid), ppobj );

        return E_FAIL;
}

static ULONG WINAPI
IDirectSoundCaptureBufferImpl_AddRef( LPDIRECTSOUNDCAPTUREBUFFER8 iface )
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
IDirectSoundCaptureBufferImpl_Release( LPDIRECTSOUNDCAPTUREBUFFER8 iface )
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
        LPDIRECTSOUNDCAPTUREBUFFER8 iface,
	LPDSCBCAPS lpDSCBCaps )
{
	ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);

	FIXME( "(%p)->(%p): stub\n", This, lpDSCBCaps );

	return DS_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_GetCurrentPosition(
        LPDIRECTSOUNDCAPTUREBUFFER8 iface,
	LPDWORD lpdwCapturePosition,
	LPDWORD lpdwReadPosition )
{
	ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);

	FIXME( "(%p)->(%p,%p): stub\n", This, lpdwCapturePosition, lpdwReadPosition );

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

	FIXME( "(%p)->(%p,0x%08lx,%p): stub\n", This, lpwfxFormat, dwSizeAllocated, lpdwSizeWritten );

	return DS_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_GetStatus(
        LPDIRECTSOUNDCAPTUREBUFFER8 iface,
	LPDWORD lpdwStatus )
{
	ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);

	FIXME( "(%p)->(%p): stub\n", This, lpdwStatus );

	return DS_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_Initialize(
        LPDIRECTSOUNDCAPTUREBUFFER8 iface,
	LPDIRECTSOUNDCAPTURE lpDSC,
	LPCDSCBUFFERDESC lpcDSCBDesc )
{
	ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);

	FIXME( "(%p)->(%p,%p): stub\n", This, lpDSC, lpcDSCBDesc );

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

	FIXME( "(%p)->(%08lu,%08lu,%p,%p,%p,%p,0x%08lx): stub\n", This, dwReadCusor, dwReadBytes, lplpvAudioPtr1, lpdwAudioBytes1, lplpvAudioPtr2, lpdwAudioBytes2, dwFlags );

	return DS_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_Start(
        LPDIRECTSOUNDCAPTUREBUFFER8 iface,
	DWORD dwFlags )
{
	ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);

	FIXME( "(%p)->(0x%08lx): stub\n", This, dwFlags );

	return DS_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_Stop( LPDIRECTSOUNDCAPTUREBUFFER8 iface )
{
	ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);

	FIXME( "(%p): stub\n", This );

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

	FIXME( "(%p)->(%p,%08lu,%p,%08lu): stub\n", This, lpvAudioPtr1, dwAudioBytes1, lpvAudioPtr2, dwAudioBytes2 );

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

	FIXME( "(%p)->(%s,%lu,%s,%p): stub\n", This, debugstr_guid(rguidObject), dwIndex, debugstr_guid(rguidInterface), ppObject );

	return DS_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_GetFXStatus(
        LPDIRECTSOUNDCAPTUREBUFFER8 iface,
        DWORD dwFXCount,
        LPDWORD pdwFXStatus )
{
	ICOM_THIS(IDirectSoundCaptureBufferImpl,iface);

	FIXME( "(%p)->(%lu,%p): stub\n", This, dwFXCount, pdwFXStatus );

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
