/*
 * Implements IPin and IMemInputPin. (internal)
 *
 * Copyright (C) Hidenori TAKESHIMA <hidenori@a2.ctktv.ne.jp>
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

#include "config.h"

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "strmif.h"
#include "vfwmsgs.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "basefilt.h"
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
	ULONG	i;
	FILTER_STATE	fs;

	TRACE("(%p)->(%p,%p)\n",This,pPin,pmt);

	if ( !This->bOutput )
	{
		TRACE("Connect() should not be sent to input pins\n");
		return E_UNEXPECTED;
	}
	if ( pPin == NULL )
		return E_POINTER;

	TRACE("try to connect to %p\n",pPin);

	EnterCriticalSection( This->pcsPin );

	if ( This->pPinConnectedTo != NULL )
	{
		hr = VFW_E_ALREADY_CONNECTED;
		goto err;
	}

	/* return fail if running */
	hr = IBaseFilter_GetState((IBaseFilter*)(This->pFilter),0,&fs);
	if ( hr != S_OK || fs != State_Stopped )
	{
		TRACE("not stopped\n");
		hr = VFW_E_NOT_STOPPED;
		goto err;
	}

	if ( This->pHandlers->pOnPreConnect != NULL )
	{
		hr = This->pHandlers->pOnPreConnect(This,pPin);
		if ( FAILED(hr) )
		{
			TRACE("OnPreconnect() failed hr = %08lx\n",hr);
			goto err;
		}
	}

	if ( pmt != NULL )
	{
		hr = IPin_QueryAccept(iface,pmt);
		if ( FAILED(hr) )
			goto err;
		hr = IPin_ReceiveConnection(pPin,iface,pmt);
		if ( FAILED(hr) )
			goto err;
	}
	else
	{
		for ( i = 0; i < This->cAcceptTypes; i++ )
		{
			pmt = &This->pmtAcceptTypes[i];
			hr = IPin_QueryAccept(iface,pmt);
			if ( SUCCEEDED(hr) )
			{
				hr = IPin_ReceiveConnection(pPin,iface,pmt);
				TRACE("ReceiveConnection - %08lx\n",hr);
				if ( SUCCEEDED(hr) )
				{
					goto connected;
				}
			}
		}

		hr = VFW_E_TYPE_NOT_ACCEPTED;
		goto err;
	}

connected:;
	This->pmtConn = QUARTZ_MediaType_Duplicate( pmt );
	if ( This->pmtConn == NULL )
	{
		hr = E_OUTOFMEMORY;
		IPin_Disconnect(pPin);
		goto err;
	}

	This->pPinConnectedTo = pPin; IPin_AddRef(pPin);
	hr = IPin_QueryInterface(pPin,&IID_IMemInputPin,(void**)&This->pMemInputPinConnectedTo);
	if ( FAILED(hr) )
	{
		TRACE("no IMemInputPin\n");
		IPin_Disconnect(pPin);
		goto err;
	}

	if ( This->pHandlers->pOnPostConnect != NULL )
	{
		hr = This->pHandlers->pOnPostConnect(This,pPin);
		if ( FAILED(hr) )
		{
			TRACE("OnPostConnect() failed hr = %08lx\n",hr);
			IPin_Disconnect(pPin);
			goto err;
		}
	}

	hr = S_OK;

err:
	if ( FAILED(hr) )
	{
		IPin_Disconnect(iface);
	}
	LeaveCriticalSection( This->pcsPin );

	TRACE("return %08lx\n",hr);

	return hr;
}

static HRESULT WINAPI
CPinBaseImpl_fnReceiveConnection(IPin* iface,IPin* pPin,const AM_MEDIA_TYPE* pmt)
{
	ICOM_THIS(CPinBaseImpl,iface);
	HRESULT	hr = E_NOTIMPL;
	FILTER_STATE fs;

	TRACE("(%p)->(%p,%p)\n",This,pPin,pmt);

	if ( This->bOutput )
	{
		TRACE("ReceiveConnection() should not be sent to output pins\n");
		return E_UNEXPECTED;
	}
	if ( pPin == NULL || pmt == NULL )
		return E_POINTER;

	EnterCriticalSection( This->pcsPin );

	if ( This->pPinConnectedTo != NULL )
	{
		hr = VFW_E_ALREADY_CONNECTED;
		goto err;
	}

	/* return fail if running */
	hr = IBaseFilter_GetState((IBaseFilter*)(This->pFilter),0,&fs);
	if ( hr != S_OK || fs != State_Stopped )
	{
		TRACE("not stopped\n");
		hr = VFW_E_NOT_STOPPED;
		goto err;
	}

	if ( This->pHandlers->pOnPreConnect != NULL )
	{
		hr = This->pHandlers->pOnPreConnect(This,pPin);
		if ( FAILED(hr) )
		{
			TRACE("OnPreConnect() failed hr = %08lx\n",hr);
			goto err;
		}
	}

	hr = IPin_QueryAccept(iface,pmt);
	if ( FAILED(hr) )
		goto err;

	This->pmtConn = QUARTZ_MediaType_Duplicate( pmt );
	if ( This->pmtConn == NULL )
	{
		hr = E_OUTOFMEMORY;
		goto err;
	}

	if ( This->pHandlers->pOnPostConnect != NULL )
	{
		hr = This->pHandlers->pOnPostConnect(This,pPin);
		if ( FAILED(hr) )
		{
			TRACE("OnPostConnect() failed hr = %08lx\n",hr);
			goto err;
		}
	}

	hr = S_OK;
	This->pPinConnectedTo = pPin; IPin_AddRef(pPin);

err:
	if ( FAILED(hr) )
		IPin_Disconnect(iface);
	LeaveCriticalSection( This->pcsPin );

	return hr;
}

static HRESULT WINAPI
CPinBaseImpl_fnDisconnect(IPin* iface)
{
	ICOM_THIS(CPinBaseImpl,iface);
	HRESULT hr = NOERROR;
	FILTER_STATE fs;

	TRACE("(%p)->()\n",This);

	EnterCriticalSection( This->pcsPin );

	/* return fail if running */
	hr = IBaseFilter_GetState((IBaseFilter*)(This->pFilter),0,&fs);
	if ( hr != S_OK || fs != State_Stopped )
	{
		TRACE("not stopped\n");
		hr = VFW_E_NOT_STOPPED;
		goto err;
	}

	if ( This->pHandlers->pOnDisconnect != NULL )
		hr = This->pHandlers->pOnDisconnect(This);

	if ( This->pmtConn != NULL )
	{
		QUARTZ_MediaType_Destroy( This->pmtConn );
		This->pmtConn = NULL;
	}
	if ( This->pMemInputPinConnectedTo != NULL )
	{
		IMemInputPin_Release(This->pMemInputPinConnectedTo);
		This->pMemInputPinConnectedTo = NULL;
	}
	if ( This->pPinConnectedTo != NULL )
	{
		/* FIXME - cleanup */

		IPin_Release(This->pPinConnectedTo);
		This->pPinConnectedTo = NULL;
		hr = NOERROR;
	}
	else
	{
		hr = S_FALSE; /* FIXME - is this correct??? */
	}

err:
	LeaveCriticalSection( This->pcsPin );

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

	EnterCriticalSection( This->pcsPin );

	*ppPin = This->pPinConnectedTo;
	if ( This->pPinConnectedTo != NULL )
	{
		IPin_AddRef(This->pPinConnectedTo);
		hr = NOERROR;
	}

	LeaveCriticalSection( This->pcsPin );

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

	EnterCriticalSection( This->pcsPin );

	if ( This->pmtConn != NULL )
	{
		hr = QUARTZ_MediaType_Copy( pmt, This->pmtConn );
	}
	else
	{
		ZeroMemory( pmt, sizeof(AM_MEDIA_TYPE) );
		pmt->bFixedSizeSamples = TRUE;
		pmt->lSampleSize = 1;
		hr = VFW_E_NOT_CONNECTED;
	}

	LeaveCriticalSection( This->pcsPin );

	return hr;
}

static HRESULT WINAPI
CPinBaseImpl_fnQueryPinInfo(IPin* iface,PIN_INFO* pinfo)
{
	ICOM_THIS(CPinBaseImpl,iface);

	TRACE("(%p)->(%p)\n",This,pinfo);

	if ( pinfo == NULL )
		return E_POINTER;

	EnterCriticalSection( This->pcsPin );

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

	LeaveCriticalSection( This->pcsPin );

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
	HRESULT hr;

	TRACE("(%p)->(%p)\n",This,pmt);

	if ( pmt == NULL )
		return E_POINTER;

	hr = NOERROR;
	EnterCriticalSection( This->pcsPin );
	if ( This->pHandlers->pCheckMediaType != NULL )
		hr = This->pHandlers->pCheckMediaType(This,pmt);
	LeaveCriticalSection( This->pcsPin );

	return hr;
}

static HRESULT WINAPI
CPinBaseImpl_fnEnumMediaTypes(IPin* iface,IEnumMediaTypes** ppenum)
{
	ICOM_THIS(CPinBaseImpl,iface);
	HRESULT hr;

	TRACE("(%p)->(%p)\n",This,ppenum);

	if ( ppenum == NULL )
		return E_POINTER;

	hr = E_NOTIMPL;

	EnterCriticalSection( This->pcsPin );
	if ( This->cAcceptTypes > 0 )
		hr = QUARTZ_CreateEnumMediaTypes(
			ppenum, This->pmtAcceptTypes, This->cAcceptTypes );
	LeaveCriticalSection( This->pcsPin );

	return hr;
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
	HRESULT hr = E_NOTIMPL;

	TRACE("(%p)->()\n",This);

	if ( This->bOutput )
		return E_UNEXPECTED;

	EnterCriticalSection( This->pcsPinReceive );
	if ( This->pHandlers->pEndOfStream != NULL )
		hr = This->pHandlers->pEndOfStream(This);
	LeaveCriticalSection( This->pcsPinReceive );

	return hr;
}

static HRESULT WINAPI
CPinBaseImpl_fnBeginFlush(IPin* iface)
{
	ICOM_THIS(CPinBaseImpl,iface);
	HRESULT hr = E_NOTIMPL;

	TRACE("(%p)->()\n",This);

	if ( This->bOutput )
		return E_UNEXPECTED;

	EnterCriticalSection( This->pcsPin );
	if ( This->pHandlers->pBeginFlush != NULL )
		hr = This->pHandlers->pBeginFlush(This);
	LeaveCriticalSection( This->pcsPin );

	EnterCriticalSection( This->pcsPinReceive );
	LeaveCriticalSection( This->pcsPinReceive );

	return hr;
}

static HRESULT WINAPI
CPinBaseImpl_fnEndFlush(IPin* iface)
{
	ICOM_THIS(CPinBaseImpl,iface);
	HRESULT hr = E_NOTIMPL;

	TRACE("(%p)->()\n",This);

	if ( This->bOutput )
		return E_UNEXPECTED;

	EnterCriticalSection( This->pcsPin );
	if ( This->pHandlers->pEndFlush != NULL )
		hr = This->pHandlers->pEndFlush(This);
	LeaveCriticalSection( This->pcsPin );

	return hr;
}

static HRESULT WINAPI
CPinBaseImpl_fnNewSegment(IPin* iface,REFERENCE_TIME rtStart,REFERENCE_TIME rtStop,double rate)
{
	ICOM_THIS(CPinBaseImpl,iface);
	HRESULT hr = E_NOTIMPL;

	TRACE("(%p)->()\n",This);

	if ( This->bOutput )
		return E_UNEXPECTED;

	EnterCriticalSection( This->pcsPin );
	if ( This->pHandlers->pNewSegment != NULL )
		hr = This->pHandlers->pNewSegment(This,rtStart,rtStop,rate);
	LeaveCriticalSection( This->pcsPin );

	return hr;
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
	CRITICAL_SECTION* pcsPin,
	CRITICAL_SECTION* pcsPinReceive,
	CBaseFilterImpl* pFilter, LPCWSTR pwszId,
	BOOL bOutput,
	const CBasePinHandlers*	pHandlers )
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
	This->pHandlers = pHandlers;
	This->cbIdLen = sizeof(WCHAR)*(lstrlenW(pwszId)+1);
	This->pwszId = NULL;
	This->bOutput = bOutput;
	This->pmtAcceptTypes = NULL;
	This->cAcceptTypes = 0;
	This->pcsPin = pcsPin;
	This->pcsPinReceive = pcsPinReceive;
	This->pFilter = pFilter;
	This->pPinConnectedTo = NULL;
	This->pMemInputPinConnectedTo = NULL;
	This->pmtConn = NULL;
	This->pAsyncOut = NULL;

	This->pwszId = (WCHAR*)QUARTZ_AllocMem( This->cbIdLen );
	if ( This->pwszId == NULL )
	{
		hr = E_OUTOFMEMORY;
		goto err;
	}
	memcpy( This->pwszId, pwszId, This->cbIdLen );

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

	EnterCriticalSection( This->pPin->pcsPin );

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

	LeaveCriticalSection( This->pPin->pcsPin );

	return hr;
}

static HRESULT WINAPI
CMemInputPinBaseImpl_fnNotifyAllocator(IMemInputPin* iface,IMemAllocator* pAllocator,BOOL bReadonly)
{
	ICOM_THIS(CMemInputPinBaseImpl,iface);

	TRACE("(%p)->()\n",This);

	if ( pAllocator == NULL )
		return E_POINTER;

	EnterCriticalSection( This->pPin->pcsPin );

	if ( This->pAllocator != NULL )
	{
		IMemAllocator_Release(This->pAllocator);
		This->pAllocator = NULL;
	}
	This->pAllocator = pAllocator;
	IMemAllocator_AddRef(This->pAllocator);

	This->bReadonly = bReadonly;

	LeaveCriticalSection( This->pPin->pcsPin );

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
	HRESULT hr = E_NOTIMPL;

	TRACE("(%p)->(%p)\n",This,pSample);

	EnterCriticalSection( This->pPin->pcsPinReceive );
	if ( This->pPin->pHandlers->pReceive != NULL )
		hr = This->pPin->pHandlers->pReceive(This->pPin,pSample);
	LeaveCriticalSection( This->pPin->pcsPinReceive );

	return hr;
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

	hr = NOERROR;
	for ( n = 0; n < nSample; n++ )
	{
		hr = IMemInputPin_Receive(iface,ppSample[n]);
		if ( FAILED(hr) )
			break;
	}

	*pnSampleProcessed = n;
	return hr;
}

static HRESULT WINAPI
CMemInputPinBaseImpl_fnReceiveCanBlock(IMemInputPin* iface)
{
	ICOM_THIS(CMemInputPinBaseImpl,iface);
	HRESULT hr = E_NOTIMPL;

	TRACE("(%p)->()\n",This);

	EnterCriticalSection( This->pPin->pcsPin );
	if ( This->pPin->pHandlers->pReceiveCanBlock != NULL )
		hr = This->pPin->pHandlers->pReceiveCanBlock(This->pPin);
	LeaveCriticalSection( This->pPin->pcsPin );

	return hr;
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
	CPinBaseImpl* pPin )
{
	TRACE("(%p,%p)\n",This,punkControl);

	if ( punkControl == NULL )
	{
		ERR( "punkControl must not be NULL\n" );
		return E_INVALIDARG;
	}

	ICOM_VTBL(This) = &imeminputpin;
	This->punkControl = punkControl;
	This->pPin = pPin;
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

/***************************************************************************
 *
 *	CQualityControlPassThruImpl
 *
 */

static HRESULT WINAPI
CQualityControlPassThruImpl_fnQueryInterface(IQualityControl* iface,REFIID riid,void** ppobj)
{
	ICOM_THIS(CQualityControlPassThruImpl,iface);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->punkControl,riid,ppobj);
}

static ULONG WINAPI
CQualityControlPassThruImpl_fnAddRef(IQualityControl* iface)
{
	ICOM_THIS(CQualityControlPassThruImpl,iface);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->punkControl);
}

static ULONG WINAPI
CQualityControlPassThruImpl_fnRelease(IQualityControl* iface)
{
	ICOM_THIS(CQualityControlPassThruImpl,iface);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->punkControl);
}


static HRESULT WINAPI
CQualityControlPassThruImpl_fnNotify(IQualityControl* iface,IBaseFilter* pFilter,Quality q)
{
	ICOM_THIS(CQualityControlPassThruImpl,iface);
	HRESULT hr = S_FALSE;

	TRACE("(%p)->()\n",This);

	if ( This->pControl != NULL )
		return IQualityControl_Notify( This->pControl, pFilter, q );

	EnterCriticalSection( This->pPin->pcsPin );
	if ( This->pPin->pHandlers->pQualityNotify != NULL )
		hr = This->pPin->pHandlers->pQualityNotify(This->pPin,pFilter,q);
	LeaveCriticalSection( This->pPin->pcsPin );

	return hr;
}

static HRESULT WINAPI
CQualityControlPassThruImpl_fnSetSink(IQualityControl* iface,IQualityControl* pControl)
{
	ICOM_THIS(CQualityControlPassThruImpl,iface);

	TRACE("(%p)->()\n",This);

	This->pControl = pControl; /* AddRef() must not be called */

	return NOERROR;
}

static ICOM_VTABLE(IQualityControl) iqualitycontrol =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	CQualityControlPassThruImpl_fnQueryInterface,
	CQualityControlPassThruImpl_fnAddRef,
	CQualityControlPassThruImpl_fnRelease,
	/* IQualityControl fields */
	CQualityControlPassThruImpl_fnNotify,
	CQualityControlPassThruImpl_fnSetSink,
};

HRESULT CQualityControlPassThruImpl_InitIQualityControl(
	CQualityControlPassThruImpl* This, IUnknown* punkControl,
	CPinBaseImpl* pPin )
{
	TRACE("(%p,%p)\n",This,punkControl);

	if ( punkControl == NULL )
	{
		ERR( "punkControl must not be NULL\n" );
		return E_INVALIDARG;
	}

	ICOM_VTBL(This) = &iqualitycontrol;
	This->punkControl = punkControl;
	This->pPin = pPin;

	return NOERROR;
}

void CQualityControlPassThruImpl_UninitIQualityControl(
	CQualityControlPassThruImpl* This )
{
}

/***************************************************************************
 *
 *	helper methods for output pins.
 *
 */

HRESULT CPinBaseImpl_SendSample( CPinBaseImpl* This, IMediaSample* pSample )
{
	if ( This->pHandlers->pReceive == NULL )
		return E_NOTIMPL;

	return This->pHandlers->pReceive( This, pSample );
}

HRESULT CPinBaseImpl_SendReceiveCanBlock( CPinBaseImpl* This )
{
	if ( This->pHandlers->pReceiveCanBlock == NULL )
		return E_NOTIMPL;

	return This->pHandlers->pReceiveCanBlock( This );
}

HRESULT CPinBaseImpl_SendEndOfStream( CPinBaseImpl* This )
{
	if ( This->pHandlers->pEndOfStream == NULL )
		return E_NOTIMPL;

	return This->pHandlers->pEndOfStream( This );
}

HRESULT CPinBaseImpl_SendBeginFlush( CPinBaseImpl* This )
{
	if ( This->pHandlers->pBeginFlush == NULL )
		return E_NOTIMPL;

	return This->pHandlers->pBeginFlush( This );
}

HRESULT CPinBaseImpl_SendEndFlush( CPinBaseImpl* This )
{
	if ( This->pHandlers->pEndFlush == NULL )
		return E_NOTIMPL;

	return This->pHandlers->pEndFlush( This );
}

HRESULT CPinBaseImpl_SendNewSegment( CPinBaseImpl* This, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, double rate )
{
	if ( This->pHandlers->pNewSegment == NULL )
		return E_NOTIMPL;

	return This->pHandlers->pNewSegment( This, rtStart, rtStop, rate );
}



/***************************************************************************
 *
 *	handlers for output pins.
 *
 */

HRESULT OutputPinSync_Receive( CPinBaseImpl* pImpl, IMediaSample* pSample )
{
	if ( pImpl->pMemInputPinConnectedTo == NULL )
		return NOERROR;

	return IMemInputPin_Receive(pImpl->pMemInputPinConnectedTo,pSample);
}

HRESULT OutputPinSync_ReceiveCanBlock( CPinBaseImpl* pImpl )
{
	if ( pImpl->pMemInputPinConnectedTo == NULL )
		return S_FALSE;

	return IMemInputPin_ReceiveCanBlock(pImpl->pMemInputPinConnectedTo);
}

HRESULT OutputPinSync_EndOfStream( CPinBaseImpl* pImpl )
{
	if ( pImpl->pPinConnectedTo == NULL )
		return NOERROR;

	return IPin_EndOfStream(pImpl->pPinConnectedTo);
}

HRESULT OutputPinSync_BeginFlush( CPinBaseImpl* pImpl )
{
	if ( pImpl->pPinConnectedTo == NULL )
		return NOERROR;

	return IPin_BeginFlush(pImpl->pPinConnectedTo);
}

HRESULT OutputPinSync_EndFlush( CPinBaseImpl* pImpl )
{
	if ( pImpl->pPinConnectedTo == NULL )
		return NOERROR;

	return IPin_EndFlush(pImpl->pPinConnectedTo);
}

HRESULT OutputPinSync_NewSegment( CPinBaseImpl* pImpl, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, double rate )
{
	if ( pImpl->pPinConnectedTo == NULL )
		return NOERROR;

	return IPin_NewSegment(pImpl->pPinConnectedTo,rtStart,rtStop,rate);
}

/***************************************************************************
 *
 *	handlers for output pins (async).
 *
 */

typedef struct OutputPinTask OutputPinTask;

enum OutputPinTaskType
{
	OutTask_ExitThread,
	OutTask_Receive,
	OutTask_EndOfStream,
	OutTask_BeginFlush,
	OutTask_EndFlush,
	OutTask_NewSegment,
};

struct OutputPinTask
{
	OutputPinTask* pNext;
	enum OutputPinTaskType tasktype;
	IMediaSample* pSample;
	REFERENCE_TIME rtStart;
	REFERENCE_TIME rtStop;
	double rate;
};

struct OutputPinAsyncImpl
{
	HANDLE m_hTaskThread;
	HANDLE m_hTaskEvent;
	IPin* m_pPin; /* connected pin */
	IMemInputPin* m_pMemInputPin; /* connected pin */
	CRITICAL_SECTION m_csTasks;
	OutputPinTask* m_pFirst;
	OutputPinTask* m_pLast;
	OutputPinTask* m_pTaskExitThread;
};

static OutputPinTask* OutputPinAsync_AllocTask( enum OutputPinTaskType tasktype )
{
	OutputPinTask* pTask;

	pTask = (OutputPinTask*)QUARTZ_AllocMem( sizeof(OutputPinTask) );
	pTask->pNext = NULL;
	pTask->tasktype = tasktype;
	pTask->pSample = NULL;

	return pTask;
}

static void OutputPinAsync_FreeTask( OutputPinTask* pTask )
{
	if ( pTask->pSample != NULL )
		IMediaSample_Release( pTask->pSample );
	QUARTZ_FreeMem( pTask );
}

static void OutputPinAsync_AddTask( OutputPinAsyncImpl* This, OutputPinTask* pTask, BOOL bFirst )
{
	EnterCriticalSection( &This->m_csTasks );

	if ( bFirst )
	{
		pTask->pNext = This->m_pFirst;
		This->m_pFirst = pTask;
		if ( This->m_pLast == NULL )
			This->m_pLast = pTask;
	}
	else
	{
		if ( This->m_pLast != NULL )
			This->m_pLast->pNext = pTask;
		else
			This->m_pFirst = pTask;
		This->m_pLast = pTask;
	}

	LeaveCriticalSection( &This->m_csTasks );

	SetEvent( This->m_hTaskEvent );
}

static OutputPinTask* OutputPinAsync_GetNextTask( OutputPinAsyncImpl* This )
{
	OutputPinTask* pTask;

	EnterCriticalSection( &This->m_csTasks );
	pTask = This->m_pFirst;
	if ( pTask != NULL )
	{
		This->m_pFirst = pTask->pNext;
		if ( This->m_pFirst == NULL )
			This->m_pLast = NULL;
		else
			SetEvent( This->m_hTaskEvent );
	}

	LeaveCriticalSection( &This->m_csTasks );

	return pTask;
}

static DWORD WINAPI OutputPinAsync_ThreadEntry( LPVOID pv )
{
	OutputPinAsyncImpl* This = ((CPinBaseImpl*)pv)->pAsyncOut;
	OutputPinTask* pTask;
	BOOL bLoop = TRUE;
	BOOL bInFlush = FALSE;
	HRESULT hr;

	while ( bLoop )
	{
		WaitForSingleObject( This->m_hTaskEvent, INFINITE );
		ResetEvent( This->m_hTaskEvent );

		pTask = OutputPinAsync_GetNextTask( This );
		if ( pTask == NULL )
			continue;

		hr = S_OK;
		switch ( pTask->tasktype )
		{
		case OutTask_ExitThread:
			bLoop = FALSE;
			break;
		case OutTask_Receive:
			if ( !bInFlush )
				hr = IMemInputPin_Receive( This->m_pMemInputPin, pTask->pSample );
			break;
		case OutTask_EndOfStream:
			hr = IPin_EndOfStream( This->m_pPin );
			break;
		case OutTask_BeginFlush:
			bInFlush = TRUE;
			hr = IPin_BeginFlush( This->m_pPin );
			break;
		case OutTask_EndFlush:
			bInFlush = FALSE;
			hr = IPin_EndFlush( This->m_pPin );
			break;
		case OutTask_NewSegment:
			hr = IPin_NewSegment( This->m_pPin, pTask->rtStart, pTask->rtStop, pTask->rate );
			break;
		default:
			ERR( "unexpected task type %d.\n", pTask->tasktype );
			bLoop = FALSE;
			break;
		}

		OutputPinAsync_FreeTask( pTask );

		if ( FAILED(hr) )
		{
			ERR( "hresult %08lx\n", hr );
			bLoop = FALSE;
		}
	}

	return 0;
}

HRESULT OutputPinAsync_OnActive( CPinBaseImpl* pImpl )
{
	HRESULT hr;
	DWORD dwThreadId;

	FIXME("(%p)\n",pImpl);

	if ( pImpl->pMemInputPinConnectedTo == NULL )
		return NOERROR;

	pImpl->pAsyncOut = (OutputPinAsyncImpl*)
		QUARTZ_AllocMem( sizeof( OutputPinAsyncImpl ) );
	if ( pImpl->pAsyncOut == NULL )
		return E_OUTOFMEMORY;

	InitializeCriticalSection( &pImpl->pAsyncOut->m_csTasks );
	pImpl->pAsyncOut->m_hTaskThread = (HANDLE)NULL;
	pImpl->pAsyncOut->m_hTaskEvent = (HANDLE)NULL;
	pImpl->pAsyncOut->m_pFirst = NULL;
	pImpl->pAsyncOut->m_pLast = NULL;
	pImpl->pAsyncOut->m_pTaskExitThread = NULL;
	pImpl->pAsyncOut->m_pPin = pImpl->pPinConnectedTo;
	pImpl->pAsyncOut->m_pMemInputPin = pImpl->pMemInputPinConnectedTo;

	pImpl->pAsyncOut->m_hTaskEvent =
		CreateEventA( NULL, TRUE, FALSE, NULL );
	if ( pImpl->pAsyncOut->m_hTaskEvent == (HANDLE)NULL )
	{
		hr = E_FAIL;
		goto err;
	}

	pImpl->pAsyncOut->m_pTaskExitThread =
		OutputPinAsync_AllocTask( OutTask_ExitThread );
	if ( pImpl->pAsyncOut->m_pTaskExitThread == NULL )
	{
		hr = E_OUTOFMEMORY;
		goto err;
	}

	pImpl->pAsyncOut->m_hTaskThread = CreateThread(
		NULL, 0, OutputPinAsync_ThreadEntry, pImpl,
		0, &dwThreadId );
	if ( pImpl->pAsyncOut->m_hTaskThread == (HANDLE)NULL )
	{
		hr = E_FAIL;
		goto err;
	}

	return NOERROR;
err:
	OutputPinAsync_OnInactive( pImpl );
	return hr;
}

HRESULT OutputPinAsync_OnInactive( CPinBaseImpl* pImpl )
{
	OutputPinTask* pTask;

	FIXME("(%p)\n",pImpl);

	if ( pImpl->pAsyncOut == NULL )
		return NOERROR;

	if ( pImpl->pAsyncOut->m_pTaskExitThread != NULL )
	{
		OutputPinAsync_AddTask( pImpl->pAsyncOut, pImpl->pAsyncOut->m_pTaskExitThread, TRUE );
		pImpl->pAsyncOut->m_pTaskExitThread = NULL;
	}

	if ( pImpl->pAsyncOut->m_hTaskThread != (HANDLE)NULL )
	{
		WaitForSingleObject( pImpl->pAsyncOut->m_hTaskThread, INFINITE );
		CloseHandle( pImpl->pAsyncOut->m_hTaskThread );
	}
	if ( pImpl->pAsyncOut->m_hTaskEvent != (HANDLE)NULL )
		CloseHandle( pImpl->pAsyncOut->m_hTaskEvent );

	/* release all tasks. */
	while ( 1 )
	{
		pTask = OutputPinAsync_GetNextTask( pImpl->pAsyncOut );
		if ( pTask == NULL )
			break;
		OutputPinAsync_FreeTask( pTask );
	}

	DeleteCriticalSection( &pImpl->pAsyncOut->m_csTasks );

	QUARTZ_FreeMem( pImpl->pAsyncOut );
	pImpl->pAsyncOut = NULL;

	return NOERROR;
}

HRESULT OutputPinAsync_Receive( CPinBaseImpl* pImpl, IMediaSample* pSample )
{
	OutputPinAsyncImpl* This = pImpl->pAsyncOut;
	OutputPinTask* pTask;

	TRACE("(%p,%p)\n",pImpl,pSample);

	if ( This == NULL )
		return NOERROR;

	pTask = OutputPinAsync_AllocTask( OutTask_Receive );
	if ( pTask == NULL )
		return E_OUTOFMEMORY;
	pTask->pSample = pSample; IMediaSample_AddRef( pSample );
	OutputPinAsync_AddTask( pImpl->pAsyncOut, pTask, FALSE );

	return NOERROR;
}

HRESULT OutputPinAsync_ReceiveCanBlock( CPinBaseImpl* pImpl )
{
	return S_FALSE;
}

HRESULT OutputPinAsync_EndOfStream( CPinBaseImpl* pImpl )
{
	OutputPinAsyncImpl* This = pImpl->pAsyncOut;
	OutputPinTask* pTask;

	TRACE("(%p)\n",pImpl);

	if ( This == NULL )
		return NOERROR;

	pTask = OutputPinAsync_AllocTask( OutTask_EndOfStream );
	if ( pTask == NULL )
		return E_OUTOFMEMORY;
	OutputPinAsync_AddTask( pImpl->pAsyncOut, pTask, FALSE );

	return NOERROR;
}

HRESULT OutputPinAsync_BeginFlush( CPinBaseImpl* pImpl )
{
	OutputPinAsyncImpl* This = pImpl->pAsyncOut;
	OutputPinTask* pTask;

	TRACE("(%p)\n",pImpl);

	if ( This == NULL )
		return NOERROR;

	pTask = OutputPinAsync_AllocTask( OutTask_BeginFlush );
	if ( pTask == NULL )
		return E_OUTOFMEMORY;
	OutputPinAsync_AddTask( pImpl->pAsyncOut, pTask, TRUE );

	return NOERROR;
}

HRESULT OutputPinAsync_EndFlush( CPinBaseImpl* pImpl )
{
	OutputPinAsyncImpl* This = pImpl->pAsyncOut;
	OutputPinTask* pTask;

	TRACE("(%p)\n",pImpl);

	if ( This == NULL )
		return NOERROR;

	pTask = OutputPinAsync_AllocTask( OutTask_EndFlush );
	if ( pTask == NULL )
		return E_OUTOFMEMORY;
	OutputPinAsync_AddTask( pImpl->pAsyncOut, pTask, FALSE );

	return NOERROR;
}

HRESULT OutputPinAsync_NewSegment( CPinBaseImpl* pImpl, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, double rate )
{
	OutputPinAsyncImpl* This = pImpl->pAsyncOut;
	OutputPinTask* pTask;

	TRACE("(%p)\n",pImpl);

	if ( This == NULL )
		return NOERROR;

	pTask = OutputPinAsync_AllocTask( OutTask_NewSegment );
	if ( pTask == NULL )
		return E_OUTOFMEMORY;
	pTask->rtStart = rtStart;
	pTask->rtStop = rtStop;
	pTask->rate = rate;
	OutputPinAsync_AddTask( pImpl->pAsyncOut, pTask, FALSE );

	return NOERROR;
}


