/*
 * Implements IBaseFilter for transform filters. (internal)
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
#include "control.h"
#include "vfwmsgs.h"
#include "uuids.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "xform.h"
#include "sample.h"


static const WCHAR XFORM_DefInName[] =
{'X','F','o','r','m',' ','I','n',0};
static const WCHAR XFORM_DefOutName[] =
{'X','F','o','r','m',' ','O','u','t',0};

/***************************************************************************
 *
 *	CTransformBaseImpl methods
 *
 */

static HRESULT CTransformBaseImpl_OnActive( CBaseFilterImpl* pImpl )
{
	CTransformBaseImpl_THIS(pImpl,basefilter);

	TRACE( "(%p)\n", This );

	return NOERROR;
}

static HRESULT CTransformBaseImpl_OnInactive( CBaseFilterImpl* pImpl )
{
	CTransformBaseImpl_THIS(pImpl,basefilter);
	HRESULT hr;
	IMemAllocator* pAllocator;

	TRACE( "(%p)\n", This );

	if ( This->pInPin->pin.pPinConnectedTo == NULL ||
		 This->pOutPin->pin.pPinConnectedTo == NULL )
		return NOERROR;

	EnterCriticalSection( &This->basefilter.csFilter );

	pAllocator = This->m_pOutPinAllocator;
	if ( pAllocator != NULL &&
		 This->pInPin->meminput.pAllocator != pAllocator )
	{
		hr = IMemAllocator_Commit( pAllocator );
		if ( FAILED(hr) )
			goto end;
	}

	if ( !This->m_bFiltering )
	{
		hr = This->m_pHandler->pBeginTransform( This, This->pInPin->pin.pmtConn, This->pOutPin->pin.pmtConn, This->m_bReuseSample );
		if ( FAILED(hr) )
			goto end;
		This->m_bFiltering = TRUE;
	}

	hr = NOERROR;
end:
	LeaveCriticalSection( &This->basefilter.csFilter );

	return hr;
}

static HRESULT CTransformBaseImpl_OnStop( CBaseFilterImpl* pImpl )
{
	CTransformBaseImpl_THIS(pImpl,basefilter);
	IMemAllocator* pAllocator;

	TRACE( "(%p)\n", This );

	EnterCriticalSection( &This->basefilter.csFilter );

	if ( This->m_bFiltering )
	{
		This->m_pHandler->pEndTransform( This );
		This->m_bFiltering = FALSE;
	}
	if ( This->m_pSample != NULL )
	{
		IMediaSample_Release( This->m_pSample );
		This->m_pSample = NULL;
	}

	pAllocator = This->m_pOutPinAllocator;
	if ( pAllocator != NULL &&
		 This->pInPin->meminput.pAllocator != pAllocator )
	{
		IMemAllocator_Decommit( pAllocator );
	}

	LeaveCriticalSection( &This->basefilter.csFilter );

	return NOERROR;
}

static const CBaseFilterHandlers filterhandlers =
{
	CTransformBaseImpl_OnActive, /* pOnActive */
	CTransformBaseImpl_OnInactive, /* pOnInactive */
	CTransformBaseImpl_OnStop, /* pOnStop */
};

/***************************************************************************
 *
 *	CTransformBaseInPinImpl methods
 *
 */

static HRESULT CTransformBaseInPinImpl_OnPostConnect( CPinBaseImpl* pImpl, IPin* pPin )
{
	CTransformBaseInPinImpl_THIS(pImpl,pin);
	HRESULT hr;

	TRACE( "(%p,%p)\n", This, pPin );

	EnterCriticalSection( &This->pFilter->basefilter.csFilter );
	hr = This->pFilter->m_pHandler->pGetOutputTypes( This->pFilter, This->pFilter->pInPin->pin.pmtConn, &This->pFilter->pOutPin->pin.pmtAcceptTypes, &This->pFilter->pOutPin->pin.cAcceptTypes );
	if ( FAILED(hr) )
		goto end;

	hr = NOERROR;
end:
	LeaveCriticalSection( &This->pFilter->basefilter.csFilter );

	return hr;
}

static HRESULT CTransformBaseInPinImpl_OnDisconnect( CPinBaseImpl* pImpl )
{
	CTransformBaseInPinImpl_THIS(pImpl,pin);

	TRACE( "(%p)\n", This );

	if ( This->meminput.pAllocator != NULL )
	{
		IMemAllocator_Decommit(This->meminput.pAllocator);
		IMemAllocator_Release(This->meminput.pAllocator);
		This->meminput.pAllocator = NULL;
	}

	return NOERROR;
}

static HRESULT CTransformBaseInPinImpl_CheckMediaType( CPinBaseImpl* pImpl, const AM_MEDIA_TYPE* pmt )
{
	CTransformBaseInPinImpl_THIS(pImpl,pin);
	HRESULT hr;

	TRACE( "(%p,%p)\n", This, pmt );

	EnterCriticalSection( &This->pFilter->basefilter.csFilter );
	hr = This->pFilter->m_pHandler->pCheckMediaType( This->pFilter, pmt, (This->pFilter->pOutPin->pin.pPinConnectedTo != NULL) ? This->pFilter->pOutPin->pin.pmtConn : NULL );
	LeaveCriticalSection( &This->pFilter->basefilter.csFilter );

	return hr;
}

static HRESULT CTransformBaseInPinImpl_Receive( CPinBaseImpl* pImpl, IMediaSample* pSample )
{
	CTransformBaseInPinImpl_THIS(pImpl,pin);
	HRESULT hr;

	TRACE( "(%p,%p)\n", This, pSample );

	if ( This->pin.pPinConnectedTo == NULL ||
		 This->pFilter->pOutPin->pin.pPinConnectedTo == NULL )
		return NOERROR;

	if ( !This->pFilter->m_bFiltering )
		return E_UNEXPECTED;

	if ( This->pFilter->m_bInFlush )
		return S_FALSE;

	if ( This->pFilter->m_pHandler->pProcessReceive != NULL )
	{
		hr = This->pFilter->m_pHandler->pProcessReceive( This->pFilter, pSample );
	}
	else
	{
		if ( This->meminput.pAllocator != This->pFilter->m_pOutPinAllocator )
		{
			if ( This->pFilter->m_pSample == NULL )
			{
				hr = IMemAllocator_GetBuffer( This->pFilter->m_pOutPinAllocator, &This->pFilter->m_pSample, NULL, NULL, 0 );
				if ( FAILED(hr) )
					goto end;
			}
			hr = QUARTZ_IMediaSample_Copy(
				This->pFilter->m_pSample, pSample, This->pFilter->m_bPreCopy );
			if ( FAILED(hr) )
				goto end;
		}

		if ( This->pFilter->m_bPreCopy )
			hr = This->pFilter->m_pHandler->pTransform( This->pFilter, This->pFilter->m_pSample, NULL );
		else
			hr = This->pFilter->m_pHandler->pTransform( This->pFilter, pSample, This->pFilter->m_pSample );

		if ( FAILED(hr) )
			goto end;

		if ( hr == NOERROR )
		{
			hr = CPinBaseImpl_SendSample(&This->pFilter->pOutPin->pin,This->pFilter->m_pSample);
			if ( FAILED(hr) )
				goto end;
		}

		hr = NOERROR;
	end:
		if ( !This->pFilter->m_bReuseSample )
		{
			if ( This->pFilter->m_pSample != NULL )
			{
				IMediaSample_Release( This->pFilter->m_pSample );
				This->pFilter->m_pSample = NULL;
			}
		}

		if ( FAILED(hr) )
		{
			/* Notify(ABORT) */
		}
	}

	return hr;
}

static HRESULT CTransformBaseInPinImpl_ReceiveCanBlock( CPinBaseImpl* pImpl )
{
	CTransformBaseInPinImpl_THIS(pImpl,pin);

	TRACE( "(%p)\n", This );

	if ( This->pin.pPinConnectedTo == NULL ||
		 This->pFilter->pOutPin->pin.pPinConnectedTo == NULL )
		return S_FALSE;

	return CPinBaseImpl_SendReceiveCanBlock( &This->pFilter->pOutPin->pin );
}

static HRESULT CTransformBaseInPinImpl_EndOfStream( CPinBaseImpl* pImpl )
{
	CTransformBaseInPinImpl_THIS(pImpl,pin);

	TRACE( "(%p)\n", This );

	if ( This->pin.pPinConnectedTo == NULL ||
		 This->pFilter->pOutPin->pin.pPinConnectedTo == NULL )
		return NOERROR;

	return CPinBaseImpl_SendEndOfStream( &This->pFilter->pOutPin->pin );
}

static HRESULT CTransformBaseInPinImpl_BeginFlush( CPinBaseImpl* pImpl )
{
	CTransformBaseInPinImpl_THIS(pImpl,pin);

	TRACE( "(%p)\n", This );

	if ( This->pin.pPinConnectedTo == NULL ||
		 This->pFilter->pOutPin->pin.pPinConnectedTo == NULL )
		return NOERROR;

	This->pFilter->m_bInFlush = TRUE;

	return CPinBaseImpl_SendBeginFlush( &This->pFilter->pOutPin->pin );
}

static HRESULT CTransformBaseInPinImpl_EndFlush( CPinBaseImpl* pImpl )
{
	CTransformBaseInPinImpl_THIS(pImpl,pin);

	TRACE( "(%p)\n", This );

	if ( This->pin.pPinConnectedTo == NULL ||
		 This->pFilter->pOutPin->pin.pPinConnectedTo == NULL )
		return NOERROR;

	This->pFilter->m_bInFlush = FALSE;

	return CPinBaseImpl_SendEndFlush( &This->pFilter->pOutPin->pin );
}

static HRESULT CTransformBaseInPinImpl_NewSegment( CPinBaseImpl* pImpl, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, double rate )
{
	CTransformBaseInPinImpl_THIS(pImpl,pin);

	FIXME( "(%p)\n", This );

	if ( This->pin.pPinConnectedTo == NULL ||
		 This->pFilter->pOutPin->pin.pPinConnectedTo == NULL )
		return NOERROR;

	return CPinBaseImpl_SendNewSegment( &This->pFilter->pOutPin->pin,
		rtStart, rtStop, rate );
}

static const CBasePinHandlers inputpinhandlers =
{
	NULL, /* pOnPreConnect */
	CTransformBaseInPinImpl_OnPostConnect, /* pOnPostConnect */
	CTransformBaseInPinImpl_OnDisconnect, /* pOnDisconnect */
	CTransformBaseInPinImpl_CheckMediaType, /* pCheckMediaType */
	NULL, /* pQualityNotify */
	CTransformBaseInPinImpl_Receive, /* pReceive */
	CTransformBaseInPinImpl_ReceiveCanBlock, /* pReceiveCanBlock */
	CTransformBaseInPinImpl_EndOfStream, /* pEndOfStream */
	CTransformBaseInPinImpl_BeginFlush, /* pBeginFlush */
	CTransformBaseInPinImpl_EndFlush, /* pEndFlush */
	CTransformBaseInPinImpl_NewSegment, /* pNewSegment */
};

/***************************************************************************
 *
 *	CTransformBaseOutPinImpl methods
 *
 */

static HRESULT CTransformBaseOutPinImpl_OnPostConnect( CPinBaseImpl* pImpl, IPin* pPin )
{
	CTransformBaseOutPinImpl_THIS(pImpl,pin);
	HRESULT hr;
	ALLOCATOR_PROPERTIES propReqThis;
	ALLOCATOR_PROPERTIES propReqPeer;
	ALLOCATOR_PROPERTIES propActual;
	BOOL bTransInPlace = FALSE;
	BOOL bTryToReUseSample = FALSE;
	BOOL bOutReadonly = FALSE;
	IMemAllocator*	pAllocator;

	FIXME( "(%p,%p)\n", This, pPin );

	if ( This->pFilter->pInPin->pin.pPinConnectedTo == NULL )
		return E_FAIL;
	if ( This->pin.pMemInputPinConnectedTo == NULL )
		return E_UNEXPECTED;

	ZeroMemory( &propReqThis, sizeof(ALLOCATOR_PROPERTIES) );
	ZeroMemory( &propReqPeer, sizeof(ALLOCATOR_PROPERTIES) );
	ZeroMemory( &propActual, sizeof(ALLOCATOR_PROPERTIES) );

	hr = This->pFilter->m_pHandler->pGetAllocProp( This->pFilter, This->pFilter->pInPin->pin.pmtConn, This->pin.pmtConn, &propReqThis, &bTransInPlace, &bTryToReUseSample );
	if ( FAILED(hr) )
		goto end;

	if ( propReqThis.cbAlign == 0 )
		propReqThis.cbAlign = 1;

	if ( bTransInPlace )
	{
		ZeroMemory( &propReqPeer, sizeof(ALLOCATOR_PROPERTIES) );
		hr = IMemInputPin_GetAllocatorRequirements(
			This->pin.pMemInputPinConnectedTo, &propReqPeer );
		if ( propReqPeer.cbAlign != 0 && propReqPeer.cbAlign != 1 )
			bTransInPlace = FALSE;
		if ( propReqPeer.cbPrefix != 0 )
			bTransInPlace = FALSE;

		bOutReadonly = FALSE;
		if ( bTransInPlace && This->pFilter->pInPin->meminput.bReadonly )
			bOutReadonly = TRUE;

		pAllocator = This->pFilter->pInPin->meminput.pAllocator;

		hr = IMemInputPin_NotifyAllocator(
			This->pin.pMemInputPinConnectedTo,
			pAllocator, bOutReadonly );
		if ( hr == NOERROR )
		{
			This->pFilter->m_pOutPinAllocator = pAllocator;
			IMemAllocator_AddRef(pAllocator);
			bTryToReUseSample = FALSE;
			goto end;
		}
	}

	hr = IMemInputPin_GetAllocator(
			This->pin.pMemInputPinConnectedTo, &pAllocator );
	if ( FAILED(hr) )
		goto end;
	hr = IMemAllocator_SetProperties( pAllocator, &propReqThis, &propActual );
	if ( SUCCEEDED(hr) )
	{
		TRACE("cBuffers = %ld / cbBuffer = %ld\n",propActual.cBuffers,propActual.cbBuffer);
		hr = IMemInputPin_NotifyAllocator(
			This->pin.pMemInputPinConnectedTo, pAllocator,
			bTryToReUseSample );
	}
	if ( FAILED(hr) )
	{
		IMemAllocator_Release(pAllocator);
		goto end;
	}
	This->pFilter->m_pOutPinAllocator = pAllocator;

	hr = NOERROR;
end:
	This->pFilter->m_bPreCopy = FALSE;
	This->pFilter->m_bReuseSample = FALSE;
	if ( hr == NOERROR )
	{
		This->pFilter->m_bPreCopy = bTransInPlace && (This->pFilter->pInPin->meminput.pAllocator != This->pFilter->m_pOutPinAllocator);
		This->pFilter->m_bReuseSample = bTryToReUseSample;
	}

	return hr;
}

static HRESULT CTransformBaseOutPinImpl_OnDisconnect( CPinBaseImpl* pImpl )
{
	CTransformBaseOutPinImpl_THIS(pImpl,pin);

	TRACE( "(%p)\n", This );

	if ( This->pFilter->m_pOutPinAllocator != NULL )
	{
		IMemAllocator_Decommit(This->pFilter->m_pOutPinAllocator);
		IMemAllocator_Release(This->pFilter->m_pOutPinAllocator);
		This->pFilter->m_pOutPinAllocator = NULL;
	}

	return NOERROR;
}

static HRESULT CTransformBaseOutPinImpl_CheckMediaType( CPinBaseImpl* pImpl, const AM_MEDIA_TYPE* pmt )
{
	CTransformBaseOutPinImpl_THIS(pImpl,pin);
	HRESULT hr;

	TRACE( "(%p,%p)\n", This, pmt );

	if ( This->pFilter->pInPin->pin.pPinConnectedTo == NULL )
		return E_FAIL;

	EnterCriticalSection( &This->pFilter->basefilter.csFilter );
	hr = This->pFilter->m_pHandler->pCheckMediaType( This->pFilter, This->pFilter->pInPin->pin.pmtConn, pmt );
	LeaveCriticalSection( &This->pFilter->basefilter.csFilter );

	return hr;
}

static const CBasePinHandlers outputpinhandlers =
{
	NULL, /* pOnPreConnect */
	CTransformBaseOutPinImpl_OnPostConnect, /* pOnPostConnect */
	CTransformBaseOutPinImpl_OnDisconnect, /* pOnDisconnect */
	CTransformBaseOutPinImpl_CheckMediaType, /* pCheckMediaType */
	NULL, /* pQualityNotify */
	OutputPinSync_Receive, /* pReceive */
	OutputPinSync_ReceiveCanBlock, /* pReceiveCanBlock */
	OutputPinSync_EndOfStream, /* pEndOfStream */
	OutputPinSync_BeginFlush, /* pBeginFlush */
	OutputPinSync_EndFlush, /* pEndFlush */
	OutputPinSync_NewSegment, /* pNewSegment */
};


/***************************************************************************
 *
 *	new/delete CTransformBaseImpl
 *
 */

/* can I use offsetof safely? - FIXME? */
static QUARTZ_IFEntry FilterIFEntries[] =
{
  { &IID_IPersist, offsetof(CTransformBaseImpl,basefilter)-offsetof(CTransformBaseImpl,unk) },
  { &IID_IMediaFilter, offsetof(CTransformBaseImpl,basefilter)-offsetof(CTransformBaseImpl,unk) },
  { &IID_IBaseFilter, offsetof(CTransformBaseImpl,basefilter)-offsetof(CTransformBaseImpl,unk) },
};

static void QUARTZ_DestroyTransformBase(IUnknown* punk)
{
	CTransformBaseImpl_THIS(punk,unk);

	TRACE( "(%p)\n", This );

	This->m_pHandler->pCleanup(This);

	if ( This->pInPin != NULL )
	{
		IUnknown_Release(This->pInPin->unk.punkControl);
		This->pInPin = NULL;
	}
	if ( This->pOutPin != NULL )
	{
		IUnknown_Release(This->pOutPin->unk.punkControl);
		This->pOutPin = NULL;
	}
	if ( This->pSeekPass != NULL )
	{
		IUnknown_Release((IUnknown*)&This->pSeekPass->unk);
		This->pSeekPass = NULL;
	}

	CBaseFilterImpl_UninitIBaseFilter(&This->basefilter);

	DeleteCriticalSection( &This->csReceive );
}

HRESULT QUARTZ_CreateTransformBase(
	IUnknown* punkOuter,void** ppobj,
	const CLSID* pclsidTransformBase,
	LPCWSTR pwszTransformBaseName,
	LPCWSTR pwszInPinName,
	LPCWSTR pwszOutPinName,
	const TransformBaseHandlers* pHandler )
{
	CTransformBaseImpl*	This = NULL;
	HRESULT hr;

	TRACE("(%p,%p)\n",punkOuter,ppobj);

	if ( pwszInPinName == NULL )
		pwszInPinName = XFORM_DefInName;
	if ( pwszOutPinName == NULL )
		pwszOutPinName = XFORM_DefOutName;

	This = (CTransformBaseImpl*)
		QUARTZ_AllocObj( sizeof(CTransformBaseImpl) );
	if ( This == NULL )
		return E_OUTOFMEMORY;

	This->pInPin = NULL;
	This->pOutPin = NULL;
	This->pSeekPass = NULL;
	This->m_pOutPinAllocator = NULL;
	This->m_bPreCopy = FALSE; /* sample must be copied */
	This->m_bReuseSample = FALSE; /* sample must be reused */
	This->m_bInFlush = FALSE;
	This->m_pSample = NULL;
	This->m_bFiltering = FALSE;
	This->m_pHandler = pHandler;
	This->m_pUserData = NULL;

	QUARTZ_IUnkInit( &This->unk, punkOuter );

	hr = CBaseFilterImpl_InitIBaseFilter(
		&This->basefilter,
		This->unk.punkControl,
		pclsidTransformBase,
		pwszTransformBaseName,
		&filterhandlers );
	if ( SUCCEEDED(hr) )
	{
		/* construct this class. */
		hr = This->m_pHandler->pInit( This );
		if ( FAILED(hr) )
		{
			CBaseFilterImpl_UninitIBaseFilter(&This->basefilter);
		}
	}

	if ( FAILED(hr) )
	{
		QUARTZ_FreeObj(This);
		return hr;
	}

	This->unk.pEntries = FilterIFEntries;
	This->unk.dwEntries = sizeof(FilterIFEntries)/sizeof(FilterIFEntries[0]);
	This->unk.pOnFinalRelease = QUARTZ_DestroyTransformBase;
	InitializeCriticalSection( &This->csReceive );

	/* create pins. */
	hr = QUARTZ_CreateTransformBaseInPin(
		This, &This->basefilter.csFilter, &This->csReceive,
		&This->pInPin, pwszInPinName );
	if ( SUCCEEDED(hr) )
		hr = QUARTZ_CompList_AddComp(
			This->basefilter.pInPins,
			(IUnknown*)&(This->pInPin->pin),
			NULL, 0 );
	if ( SUCCEEDED(hr) )
		hr = QUARTZ_CreateTransformBaseOutPin(
			This, &This->basefilter.csFilter,
			&This->pOutPin, pwszOutPinName );
	if ( SUCCEEDED(hr) )
		hr = QUARTZ_CompList_AddComp(
			This->basefilter.pOutPins,
			(IUnknown*)&(This->pOutPin->pin),
			NULL, 0 );

	if ( SUCCEEDED(hr) )
	{
		hr = QUARTZ_CreateSeekingPassThruInternal(
			(IUnknown*)&(This->pOutPin->unk), &This->pSeekPass,
			FALSE, (IPin*)&(This->pInPin->pin) );
	}

	if ( FAILED(hr) )
	{
		IUnknown_Release( This->unk.punkControl );
		return hr;
	}

	*ppobj = (void*)&(This->unk);

	return S_OK;
}

/***************************************************************************
 *
 *	new/delete CTransformBaseInPinImpl
 *
 */

/* can I use offsetof safely? - FIXME? */
static QUARTZ_IFEntry InPinIFEntries[] =
{
  { &IID_IPin, offsetof(CTransformBaseInPinImpl,pin)-offsetof(CTransformBaseInPinImpl,unk) },
  { &IID_IMemInputPin, offsetof(CTransformBaseInPinImpl,meminput)-offsetof(CTransformBaseInPinImpl,unk) },
};

static void QUARTZ_DestroyTransformBaseInPin(IUnknown* punk)
{
	CTransformBaseInPinImpl_THIS(punk,unk);

	TRACE( "(%p)\n", This );

	CPinBaseImpl_UninitIPin( &This->pin );
	CMemInputPinBaseImpl_UninitIMemInputPin( &This->meminput );
}

HRESULT QUARTZ_CreateTransformBaseInPin(
	CTransformBaseImpl* pFilter,
	CRITICAL_SECTION* pcsPin,
	CRITICAL_SECTION* pcsPinReceive,
	CTransformBaseInPinImpl** ppPin,
	LPCWSTR pwszPinName )
{
	CTransformBaseInPinImpl*	This = NULL;
	HRESULT hr;

	TRACE("(%p,%p,%p)\n",pFilter,pcsPin,ppPin);

	This = (CTransformBaseInPinImpl*)
		QUARTZ_AllocObj( sizeof(CTransformBaseInPinImpl) );
	if ( This == NULL )
		return E_OUTOFMEMORY;

	QUARTZ_IUnkInit( &This->unk, NULL );
	This->pFilter = pFilter;

	hr = CPinBaseImpl_InitIPin(
		&This->pin,
		This->unk.punkControl,
		pcsPin, pcsPinReceive,
		&pFilter->basefilter,
		pwszPinName,
		FALSE,
		&inputpinhandlers );

	if ( SUCCEEDED(hr) )
	{
		hr = CMemInputPinBaseImpl_InitIMemInputPin(
			&This->meminput,
			This->unk.punkControl,
			&This->pin );
		if ( FAILED(hr) )
		{
			CPinBaseImpl_UninitIPin( &This->pin );
		}
	}

	if ( FAILED(hr) )
	{
		QUARTZ_FreeObj(This);
		return hr;
	}

	This->unk.pEntries = InPinIFEntries;
	This->unk.dwEntries = sizeof(InPinIFEntries)/sizeof(InPinIFEntries[0]);
	This->unk.pOnFinalRelease = QUARTZ_DestroyTransformBaseInPin;

	*ppPin = This;

	TRACE("returned successfully.\n");

	return S_OK;
}


/***************************************************************************
 *
 *	new/delete CTransformBaseOutPinImpl
 *
 */

/* can I use offsetof safely? - FIXME? */
static QUARTZ_IFEntry OutPinIFEntries[] =
{
  { &IID_IPin, offsetof(CTransformBaseOutPinImpl,pin)-offsetof(CTransformBaseOutPinImpl,unk) },
  { &IID_IQualityControl, offsetof(CTransformBaseOutPinImpl,qcontrol)-offsetof(CTransformBaseOutPinImpl,unk) },
};

static HRESULT CTransformBaseOutPinImpl_OnQueryInterface(
	IUnknown* punk, const IID* piid, void** ppobj )
{
	CTransformBaseOutPinImpl_THIS(punk,unk);

	if ( This->pFilter == NULL || This->pFilter->pSeekPass == NULL )
		return E_NOINTERFACE;

	if ( IsEqualGUID( &IID_IMediaPosition, piid ) ||
		 IsEqualGUID( &IID_IMediaSeeking, piid ) )
	{
		TRACE( "IMediaSeeking(or IMediaPosition) is queried\n" );
		return IUnknown_QueryInterface( (IUnknown*)(&This->pFilter->pSeekPass->unk), piid, ppobj );
	}

	return E_NOINTERFACE;
}

static void QUARTZ_DestroyTransformBaseOutPin(IUnknown* punk)
{
	CTransformBaseOutPinImpl_THIS(punk,unk);

	TRACE( "(%p)\n", This );

	CPinBaseImpl_UninitIPin( &This->pin );
	CQualityControlPassThruImpl_UninitIQualityControl( &This->qcontrol );
}

HRESULT QUARTZ_CreateTransformBaseOutPin(
	CTransformBaseImpl* pFilter,
	CRITICAL_SECTION* pcsPin,
	CTransformBaseOutPinImpl** ppPin,
	LPCWSTR pwszPinName )
{
	CTransformBaseOutPinImpl*	This = NULL;
	HRESULT hr;

	TRACE("(%p,%p,%p)\n",pFilter,pcsPin,ppPin);

	This = (CTransformBaseOutPinImpl*)
		QUARTZ_AllocObj( sizeof(CTransformBaseOutPinImpl) );
	if ( This == NULL )
		return E_OUTOFMEMORY;

	QUARTZ_IUnkInit( &This->unk, NULL );
	This->qiext.pNext = NULL;
	This->qiext.pOnQueryInterface = &CTransformBaseOutPinImpl_OnQueryInterface;
	QUARTZ_IUnkAddDelegation( &This->unk, &This->qiext );

	This->pFilter = pFilter;

	hr = CPinBaseImpl_InitIPin(
		&This->pin,
		This->unk.punkControl,
		pcsPin, NULL,
		&pFilter->basefilter,
		pwszPinName,
		TRUE,
		&outputpinhandlers );

	if ( SUCCEEDED(hr) )
	{
		hr = CQualityControlPassThruImpl_InitIQualityControl(
			&This->qcontrol,
			This->unk.punkControl,
			&This->pin );
		if ( FAILED(hr) )
		{
			CPinBaseImpl_UninitIPin( &This->pin );
		}
	}

	if ( FAILED(hr) )
	{
		QUARTZ_FreeObj(This);
		return hr;
	}

	This->unk.pEntries = OutPinIFEntries;
	This->unk.dwEntries = sizeof(OutPinIFEntries)/sizeof(OutPinIFEntries[0]);
	This->unk.pOnFinalRelease = QUARTZ_DestroyTransformBaseOutPin;

	*ppPin = This;

	TRACE("returned successfully.\n");

	return S_OK;
}

