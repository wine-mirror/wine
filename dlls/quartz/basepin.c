/*
 * Implements IPin and IMemInputPin. (internal)
 *
 * hidenori@a2.ctktv.ne.jp
 */

#include "config.h"

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "wine/obj_base.h"
#include "strmif.h"
#include "vfwmsgs.h"
#include "wine/unicode.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "basefilt.h"
#include "mtype.h"
#include "memalloc.h"


/***************************************************************************
 *
 *	CPinBaseImpl
 *
 */

static HRESULT WINAPI
CPinBaseImpl_fnQueryInterface(IPin* iface,REFIID riid,void** ppobj)
{
	ICOM_THIS(CPinBaseImpl,iface);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->punkControl,riid,ppobj);
}

static ULONG WINAPI
CPinBaseImpl_fnAddRef(IPin* iface)
{
	ICOM_THIS(CPinBaseImpl,iface);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->punkControl);
}

static ULONG WINAPI
CPinBaseImpl_fnRelease(IPin* iface)
{
	ICOM_THIS(CPinBaseImpl,iface);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->punkControl);
}

static HRESULT WINAPI
CPinBaseImpl_fnConnect(IPin* iface,IPin* pPin,const AM_MEDIA_TYPE* pmt)
{
	ICOM_THIS(CPinBaseImpl,iface);
	HRESULT	hr = E_NOTIMPL;

	FIXME("(%p)->(%p,%p) stub!\n",This,pPin,pmt);

	if ( !This->bOutput )
		return E_UNEXPECTED;
	if ( pPin == NULL || pmt == NULL )
		return E_POINTER;

	EnterCriticalSection( &This->csPin );

	if ( This->pPinConnectedTo != NULL )
	{
		hr = VFW_E_ALREADY_CONNECTED;
		goto err;
	}

	/* FIXME - return fail if running */

	/* FIXME */

	hr = E_NOTIMPL;
err:
	LeaveCriticalSection( &This->csPin );

	return hr;
}

static HRESULT WINAPI
CPinBaseImpl_fnReceiveConnection(IPin* iface,IPin* pPin,const AM_MEDIA_TYPE* pmt)
{
	ICOM_THIS(CPinBaseImpl,iface);
	HRESULT	hr = E_NOTIMPL;

	FIXME("(%p)->(%p,%p) stub!\n",This,pPin,pmt);

	if ( This->bOutput )
		return E_UNEXPECTED;
	if ( pPin == NULL || pmt == NULL )
		return E_POINTER;

	EnterCriticalSection( &This->csPin );

	if ( This->pPinConnectedTo != NULL )
	{
		hr = VFW_E_ALREADY_CONNECTED;
		goto err;
	}

	/* FIXME - return fail if running */

	/* FIXME */

	hr = E_NOTIMPL;
err:
	LeaveCriticalSection( &This->csPin );

	return E_NOTIMPL;
}

static HRESULT WINAPI
CPinBaseImpl_fnDisconnect(IPin* iface)
{
	ICOM_THIS(CPinBaseImpl,iface);
	HRESULT hr = NOERROR;

	FIXME("(%p)->() stub!\n",This);

	EnterCriticalSection( &This->csPin );

	/* FIXME - return fail if running */

	if ( This->pPinConnectedTo != NULL )
	{
		/* FIXME - cleanup */

		if ( This->pmtConn != NULL )
		{
			QUARTZ_MediaType_Destroy( This->pmtConn );
			This->pmtConn = NULL;
		}

		IPin_Release(This->pPinConnectedTo);
		This->pPinConnectedTo = NULL;
		hr = NOERROR;
	}
	else
	{
		hr = S_FALSE; /* FIXME - is this correct??? */
	}

	LeaveCriticalSection( &This->csPin );

	return hr;
}

static HRESULT WINAPI
CPinBaseImpl_fnConnectedTo(IPin* iface,IPin** ppPin)
{
	ICOM_THIS(CPinBaseImpl,iface);
	HRESULT	hr = VFW_E_NOT_CONNECTED;

	TRACE("(%p)->(%p)\n",This,ppPin);

	if ( ppPin == NULL )
		return E_POINTER;

	EnterCriticalSection( &This->csPin );

	*ppPin = This->pPinConnectedTo;
	if ( This->pPinConnectedTo != NULL )
	{
		IPin_AddRef(This->pPinConnectedTo);
		hr = NOERROR;
	}

	LeaveCriticalSection( &This->csPin );

	return hr;
}

static HRESULT WINAPI
CPinBaseImpl_fnConnectionMediaType(IPin* iface,AM_MEDIA_TYPE* pmt)
{
	ICOM_THIS(CPinBaseImpl,iface);
	HRESULT hr = E_FAIL;

	TRACE("(%p)->(%p)\n",This,pmt);

	if ( pmt == NULL )
		return E_POINTER;

	EnterCriticalSection( &This->csPin );

	if ( This->pmtConn != NULL )
	{
		hr = QUARTZ_MediaType_Copy( pmt, This->pmtConn );
	}
	else
	{
		ZeroMemory( pmt, sizeof(AM_MEDIA_TYPE) );
		pmt->bFixedSizeSamples = TRUE;
		pmt->lSampleSize = 1;
		hr = NOERROR;
	}

	LeaveCriticalSection( &This->csPin );

	return hr;
}

static HRESULT WINAPI
CPinBaseImpl_fnQueryPinInfo(IPin* iface,PIN_INFO* pinfo)
{
	ICOM_THIS(CPinBaseImpl,iface);

	TRACE("(%p)->(%p)\n",This,pinfo);

	if ( pinfo == NULL )
		return E_POINTER;

	EnterCriticalSection( &This->csPin );

	ZeroMemory( pinfo, sizeof(PIN_INFO) );
	pinfo->pFilter = (IBaseFilter*)(This->pFilter);
	if ( pinfo->pFilter != NULL )
		IBaseFilter_AddRef( pinfo->pFilter );
	pinfo->dir = This->bOutput ? PINDIR_OUTPUT : PINDIR_INPUT;
	if ( This->cbIdLen <= sizeof(pinfo->achName) )
		memcpy( pinfo->achName, This->pwszId, This->cbIdLen );
	else
	{
		memcpy( pinfo->achName, This->pwszId, sizeof(pinfo->achName) );
		pinfo->achName[sizeof(pinfo->achName)/sizeof(pinfo->achName[0])-1] = 0;
	}

	LeaveCriticalSection( &This->csPin );

	return NOERROR;
}

static HRESULT WINAPI
CPinBaseImpl_fnQueryDirection(IPin* iface,PIN_DIRECTION* pdir)
{
	ICOM_THIS(CPinBaseImpl,iface);

	TRACE("(%p)->(%p)\n",This,pdir);

	if ( pdir == NULL )
		return E_POINTER;

	*pdir = This->bOutput ? PINDIR_OUTPUT : PINDIR_INPUT;

	return NOERROR;
}

static HRESULT WINAPI
CPinBaseImpl_fnQueryId(IPin* iface,LPWSTR* lpwszId)
{
	ICOM_THIS(CPinBaseImpl,iface);

	TRACE("(%p)->(%p)\n",This,lpwszId);

	if ( lpwszId == NULL )
		return E_POINTER;

	*lpwszId = (WCHAR*)CoTaskMemAlloc( This->cbIdLen );
	if ( *lpwszId == NULL )
		return E_OUTOFMEMORY;
	memcpy( *lpwszId, This->pwszId, This->cbIdLen );

	return NOERROR;
}

static HRESULT WINAPI
CPinBaseImpl_fnQueryAccept(IPin* iface,const AM_MEDIA_TYPE* pmt)
{
	ICOM_THIS(CPinBaseImpl,iface);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
CPinBaseImpl_fnEnumMediaTypes(IPin* iface,IEnumMediaTypes** ppenum)
{
	ICOM_THIS(CPinBaseImpl,iface);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
CPinBaseImpl_fnQueryInternalConnections(IPin* iface,IPin** ppPin,ULONG* pul)
{
	ICOM_THIS(CPinBaseImpl,iface);

	TRACE("(%p)->(%p,%p)\n",This,ppPin,pul);

	/* E_NOTIMPL means 'no internal connections'. */
	return E_NOTIMPL;
}

static HRESULT WINAPI
CPinBaseImpl_fnEndOfStream(IPin* iface)
{
	ICOM_THIS(CPinBaseImpl,iface);

	FIXME("(%p)->() stub!\n",This);

	if ( This->bOutput )
		return E_UNEXPECTED;

	return E_NOTIMPL;
}

static HRESULT WINAPI
CPinBaseImpl_fnBeginFlush(IPin* iface)
{
	ICOM_THIS(CPinBaseImpl,iface);

	FIXME("(%p)->() stub!\n",This);

	if ( This->bOutput )
		return E_UNEXPECTED;

	return E_NOTIMPL;
}

static HRESULT WINAPI
CPinBaseImpl_fnEndFlush(IPin* iface)
{
	ICOM_THIS(CPinBaseImpl,iface);

	FIXME("(%p)->() stub!\n",This);

	if ( This->bOutput )
		return E_UNEXPECTED;

	return E_NOTIMPL;
}

static HRESULT WINAPI
CPinBaseImpl_fnNewSegment(IPin* iface,REFERENCE_TIME rtStart,REFERENCE_TIME rtStop,double rate)
{
	ICOM_THIS(CPinBaseImpl,iface);

	FIXME("(%p)->() stub!\n",This);

	if ( This->bOutput )
		return E_UNEXPECTED;

	return E_NOTIMPL;
}




static ICOM_VTABLE(IPin) ipin =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	CPinBaseImpl_fnQueryInterface,
	CPinBaseImpl_fnAddRef,
	CPinBaseImpl_fnRelease,
	/* IPin fields */
	CPinBaseImpl_fnConnect,
	CPinBaseImpl_fnReceiveConnection,
	CPinBaseImpl_fnDisconnect,
	CPinBaseImpl_fnConnectedTo,
	CPinBaseImpl_fnConnectionMediaType,
	CPinBaseImpl_fnQueryPinInfo,
	CPinBaseImpl_fnQueryDirection,
	CPinBaseImpl_fnQueryId,
	CPinBaseImpl_fnQueryAccept,
	CPinBaseImpl_fnEnumMediaTypes,
	CPinBaseImpl_fnQueryInternalConnections,
	CPinBaseImpl_fnEndOfStream,
	CPinBaseImpl_fnBeginFlush,
	CPinBaseImpl_fnEndFlush,
	CPinBaseImpl_fnNewSegment,
};


HRESULT CPinBaseImpl_InitIPin(
	CPinBaseImpl* This, IUnknown* punkControl,
	CBaseFilterImpl* pFilter, LPCWSTR pwszId,
	BOOL bOutput )
{
	HRESULT	hr = NOERROR;

	TRACE("(%p,%p,%p)\n",This,punkControl,pFilter);

	if ( punkControl == NULL )
	{
		ERR( "punkControl must not be NULL\n" );
		return E_INVALIDARG;
	}

	ICOM_VTBL(This) = &ipin;
	This->punkControl = punkControl;
	This->cbIdLen = sizeof(WCHAR)*(strlenW(pwszId)+1);
	This->pwszId = NULL;
	This->bOutput = bOutput;
	This->pFilter = pFilter;
	This->pPinConnectedTo = NULL;
	This->pmtConn = NULL;

	This->pwszId = (WCHAR*)QUARTZ_AllocMem( This->cbIdLen );
	if ( This->pwszId == NULL )
	{
		hr = E_OUTOFMEMORY;
		goto err;
	}

	InitializeCriticalSection( &This->csPin );

	return NOERROR;

err:;
	CPinBaseImpl_UninitIPin( This );
	return hr;
}

void CPinBaseImpl_UninitIPin( CPinBaseImpl* This )
{
	TRACE("(%p)\n",This);

	IPin_Disconnect( (IPin*)(This) );

	if ( This->pwszId != NULL )
	{
		QUARTZ_FreeMem( This->pwszId );
		This->pwszId = NULL;
	}

	DeleteCriticalSection( &This->csPin );
}


/***************************************************************************
 *
 *	CMemInputPinBaseImpl
 *
 */


static HRESULT WINAPI
CMemInputPinBaseImpl_fnQueryInterface(IMemInputPin* iface,REFIID riid,void** ppobj)
{
	ICOM_THIS(CMemInputPinBaseImpl,iface);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->punkControl,riid,ppobj);
}

static ULONG WINAPI
CMemInputPinBaseImpl_fnAddRef(IMemInputPin* iface)
{
	ICOM_THIS(CMemInputPinBaseImpl,iface);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->punkControl);
}

static ULONG WINAPI
CMemInputPinBaseImpl_fnRelease(IMemInputPin* iface)
{
	ICOM_THIS(CMemInputPinBaseImpl,iface);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->punkControl);
}


static HRESULT WINAPI
CMemInputPinBaseImpl_fnGetAllocator(IMemInputPin* iface,IMemAllocator** ppAllocator)
{
	ICOM_THIS(CMemInputPinBaseImpl,iface);
	HRESULT hr = NOERROR;
	IUnknown* punk;

	TRACE("(%p)->()\n",This);

	if ( ppAllocator == NULL )
		return E_POINTER;

	EnterCriticalSection( This->pcsPin );

	if ( This->pAllocator == NULL )
	{
		hr = QUARTZ_CreateMemoryAllocator(NULL,(void**)&punk);
		if ( hr == NOERROR )
		{
			hr = IUnknown_QueryInterface(punk,
				&IID_IMemAllocator,(void**)&This->pAllocator);
			IUnknown_Release(punk);
		}
	}

	if ( hr == NOERROR )
	{
		*ppAllocator = This->pAllocator;
		IMemAllocator_AddRef(This->pAllocator);
	}

	LeaveCriticalSection( This->pcsPin );

	return hr;
}

static HRESULT WINAPI
CMemInputPinBaseImpl_fnNotifyAllocator(IMemInputPin* iface,IMemAllocator* pAllocator,BOOL bReadonly)
{
	ICOM_THIS(CMemInputPinBaseImpl,iface);

	TRACE("(%p)->()\n",This);

	if ( pAllocator == NULL )
		return E_POINTER;

	EnterCriticalSection( This->pcsPin );

	if ( This->pAllocator != NULL )
	{
		IMemAllocator_Release(This->pAllocator);
		This->pAllocator = NULL;
	}
	This->pAllocator = pAllocator;
	IMemAllocator_AddRef(This->pAllocator);

	This->bReadonly = bReadonly;

	LeaveCriticalSection( This->pcsPin );

	return NOERROR;
}

static HRESULT WINAPI
CMemInputPinBaseImpl_fnGetAllocatorRequirements(IMemInputPin* iface,ALLOCATOR_PROPERTIES* pProp)
{
	ICOM_THIS(CMemInputPinBaseImpl,iface);

	TRACE("(%p)->(%p)\n",This,pProp);

	if ( pProp == NULL )
		return E_POINTER;

	/* E_MOTIMPL means 'no requirements' */
	return E_NOTIMPL;
}

static HRESULT WINAPI
CMemInputPinBaseImpl_fnReceive(IMemInputPin* iface,IMediaSample* pSample)
{
	ICOM_THIS(CMemInputPinBaseImpl,iface);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
CMemInputPinBaseImpl_fnReceiveMultiple(IMemInputPin* iface,IMediaSample** ppSample,long nSample,long* pnSampleProcessed)
{
	ICOM_THIS(CMemInputPinBaseImpl,iface);
	long	n;
	HRESULT hr;

	TRACE("(%p)->()\n",This);

	if ( ppSample == NULL || pnSampleProcessed == NULL )
		return E_POINTER;

	EnterCriticalSection( This->pcsPin );

	hr = NOERROR;
	for ( n = 0; n < nSample; n++ )
	{
		hr = IMemInputPin_Receive(iface,ppSample[n]);
		if ( FAILED(hr) )
			break;
	}

	LeaveCriticalSection( This->pcsPin );

	*pnSampleProcessed = n;
	return hr;
}

static HRESULT WINAPI
CMemInputPinBaseImpl_fnReceiveCanBlock(IMemInputPin* iface)
{
	ICOM_THIS(CMemInputPinBaseImpl,iface);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}


static ICOM_VTABLE(IMemInputPin) imeminputpin =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	CMemInputPinBaseImpl_fnQueryInterface,
	CMemInputPinBaseImpl_fnAddRef,
	CMemInputPinBaseImpl_fnRelease,
	/* IMemInputPin fields */
	CMemInputPinBaseImpl_fnGetAllocator,
	CMemInputPinBaseImpl_fnNotifyAllocator,
	CMemInputPinBaseImpl_fnGetAllocatorRequirements,
	CMemInputPinBaseImpl_fnReceive,
	CMemInputPinBaseImpl_fnReceiveMultiple,
	CMemInputPinBaseImpl_fnReceiveCanBlock,
};

HRESULT CMemInputPinBaseImpl_InitIMemInputPin(
	CMemInputPinBaseImpl* This, IUnknown* punkControl,
	CRITICAL_SECTION* pcsPin
	)
{
	TRACE("(%p,%p)\n",This,punkControl);

	if ( punkControl == NULL )
	{
		ERR( "punkControl must not be NULL\n" );
		return E_INVALIDARG;
	}

	ICOM_VTBL(This) = &imeminputpin;
	This->punkControl = punkControl;
	This->pcsPin = pcsPin;
	This->pAllocator = NULL;
	This->bReadonly = FALSE;

	return NOERROR;
}

void CMemInputPinBaseImpl_UninitIMemInputPin(
	CMemInputPinBaseImpl* This )
{
	TRACE("(%p)\n",This);

	if ( This->pAllocator != NULL )
	{
		IMemAllocator_Release(This->pAllocator);
		This->pAllocator = NULL;
	}
}


