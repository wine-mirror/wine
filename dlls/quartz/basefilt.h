/*
 * Copyright (C) Hidenori TAKESHIMA
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

#ifndef	WINE_DSHOW_BASEFILT_H
#define	WINE_DSHOW_BASEFILT_H

/*
 * The following interfaces must be used as a part of aggregation.
 * The punkControl must not be NULL since all IUnknown methods are
 * implemented only for aggregation.
 */

/*
 * implements IBaseFilter (internal)
 *
 * a base class for implementing IBaseFilter.
 */

#include "complist.h"
#include "mtype.h"

typedef struct CBaseFilterHandlers	CBaseFilterHandlers;
typedef struct CBasePinHandlers	CBasePinHandlers;

typedef struct CBaseFilterImpl
{
	/* IPersist - IMediaFilter - IBaseFilter */
	ICOM_VFIELD(IBaseFilter);

	/* IUnknown fields */
	IUnknown*	punkControl;
	/* IBaseFilter fields */
	const CBaseFilterHandlers*	pHandlers;
	CRITICAL_SECTION	csFilter;
	const CLSID*	pclsidFilter;
	QUARTZ_CompList*	pInPins;	/* a list of IPin-s. */
	QUARTZ_CompList*	pOutPins;	/* a list of IPin-s. */
	IFilterGraph*	pfg;
	DWORD	cbNameGraph;
	WCHAR*	pwszNameGraph;
	IReferenceClock*	pClock;
	REFERENCE_TIME	rtStart;
	FILTER_STATE	fstate;
	BOOL	bIntermediateState; /* if set, fstate is ignored. */
} CBaseFilterImpl;

struct CBaseFilterHandlers
{
	HRESULT (*pOnActive)( CBaseFilterImpl* pImpl );
	HRESULT (*pOnInactive)( CBaseFilterImpl* pImpl );
	HRESULT (*pOnStop)( CBaseFilterImpl* pImpl );
};


HRESULT CBaseFilterImpl_InitIBaseFilter(
	CBaseFilterImpl* This, IUnknown* punkControl,
	const CLSID* pclsidFilter, LPCWSTR lpwszNameGraph,
	const CBaseFilterHandlers* pHandlers );
void CBaseFilterImpl_UninitIBaseFilter( CBaseFilterImpl* This );


HRESULT CBaseFilterImpl_MediaEventNotify(
	CBaseFilterImpl* This, long lEvent,LONG_PTR lParam1,LONG_PTR lParam2);


/*
 * Implements IPin, IMemInputPin, and IQualityControl. (internal)
 *
 * a base class for implementing IPin.
 */

typedef struct OutputPinAsyncImpl OutputPinAsyncImpl;

typedef struct CPinBaseImpl
{
	/* IPin */
	ICOM_VFIELD(IPin);

	/* IUnknown fields */
	IUnknown*	punkControl;
	/* IPin fields */
	const CBasePinHandlers*	pHandlers;
	DWORD	cbIdLen;
	WCHAR*	pwszId;
	BOOL	bOutput;

	/* you can change AcceptTypes while pcsPin has been hold */
	const AM_MEDIA_TYPE*	pmtAcceptTypes;
	ULONG	cAcceptTypes;

	CRITICAL_SECTION*	pcsPin;
	CRITICAL_SECTION*	pcsPinReceive;
	CBaseFilterImpl*	pFilter;
	IPin*	pPinConnectedTo;
	IMemInputPin*	pMemInputPinConnectedTo;
	AM_MEDIA_TYPE*	pmtConn;
	OutputPinAsyncImpl* pAsyncOut; /* for asynchronous output */
} CPinBaseImpl;

typedef struct CMemInputPinBaseImpl
{
	/* IMemInputPin */
	ICOM_VFIELD(IMemInputPin);

	/* IUnknown fields */
	IUnknown*	punkControl;
	/* IMemInputPin fields */
	CPinBaseImpl*	pPin;
	IMemAllocator*	pAllocator;
	BOOL	bReadonly;
} CMemInputPinBaseImpl;

typedef struct CQualityControlPassThruImpl
{
	/* IQualityControl */
	ICOM_VFIELD(IQualityControl);

	/* IUnknown fields */
	IUnknown*	punkControl;
	/* IQualityControl fields */
	CPinBaseImpl*	pPin;
	IQualityControl*	pControl;
} CQualityControlPassThruImpl;


struct CBasePinHandlers
{
	HRESULT (*pOnPreConnect)( CPinBaseImpl* pImpl, IPin* pPin );
	HRESULT (*pOnPostConnect)( CPinBaseImpl* pImpl, IPin* pPin );
	HRESULT (*pOnDisconnect)( CPinBaseImpl* pImpl );
	HRESULT (*pCheckMediaType)( CPinBaseImpl* pImpl, const AM_MEDIA_TYPE* pmt );
	HRESULT (*pQualityNotify)( CPinBaseImpl* pImpl, IBaseFilter* pFilter, Quality q );
	HRESULT (*pReceive)( CPinBaseImpl* pImpl, IMediaSample* pSample );
	HRESULT (*pReceiveCanBlock)( CPinBaseImpl* pImpl );
	HRESULT (*pEndOfStream)( CPinBaseImpl* pImpl );
	HRESULT (*pBeginFlush)( CPinBaseImpl* pImpl );
	HRESULT (*pEndFlush)( CPinBaseImpl* pImpl );
	HRESULT (*pNewSegment)( CPinBaseImpl* pImpl, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, double rate );
};



HRESULT CPinBaseImpl_InitIPin(
	CPinBaseImpl* This, IUnknown* punkControl,
	CRITICAL_SECTION* pcsPin,
	CRITICAL_SECTION* pcsPinReceive,
	CBaseFilterImpl* pFilter, LPCWSTR pwszId,
	BOOL bOutput,
	const CBasePinHandlers*	pHandlers );
void CPinBaseImpl_UninitIPin( CPinBaseImpl* This );


HRESULT CMemInputPinBaseImpl_InitIMemInputPin(
	CMemInputPinBaseImpl* This, IUnknown* punkControl,
	CPinBaseImpl* pPin );
void CMemInputPinBaseImpl_UninitIMemInputPin(
	CMemInputPinBaseImpl* This );


HRESULT CQualityControlPassThruImpl_InitIQualityControl(
	CQualityControlPassThruImpl* This, IUnknown* punkControl,
	CPinBaseImpl* pPin );
void CQualityControlPassThruImpl_UninitIQualityControl(
	CQualityControlPassThruImpl* This );


HRESULT CPinBaseImpl_SendSample( CPinBaseImpl* This, IMediaSample* pSample );
HRESULT CPinBaseImpl_SendReceiveCanBlock( CPinBaseImpl* This );
HRESULT CPinBaseImpl_SendEndOfStream( CPinBaseImpl* This );
HRESULT CPinBaseImpl_SendBeginFlush( CPinBaseImpl* This );
HRESULT CPinBaseImpl_SendEndFlush( CPinBaseImpl* This );
HRESULT CPinBaseImpl_SendNewSegment( CPinBaseImpl* This, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, double rate );


/***************************************************************************
 *
 *	handlers for output pins.
 *
 */

HRESULT OutputPinSync_Receive( CPinBaseImpl* pImpl, IMediaSample* pSample );
HRESULT OutputPinSync_ReceiveCanBlock( CPinBaseImpl* pImpl );
HRESULT OutputPinSync_EndOfStream( CPinBaseImpl* pImpl );
HRESULT OutputPinSync_BeginFlush( CPinBaseImpl* pImpl );
HRESULT OutputPinSync_EndFlush( CPinBaseImpl* pImpl );
HRESULT OutputPinSync_NewSegment( CPinBaseImpl* pImpl, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, double rate );

/***************************************************************************
 *
 *	handlers for output pins (async).
 *
 */

HRESULT OutputPinAsync_OnActive( CPinBaseImpl* pImpl );
HRESULT OutputPinAsync_OnInactive( CPinBaseImpl* pImpl );
HRESULT OutputPinAsync_Receive( CPinBaseImpl* pImpl, IMediaSample* pSample );
HRESULT OutputPinAsync_ReceiveCanBlock( CPinBaseImpl* pImpl );
HRESULT OutputPinAsync_EndOfStream( CPinBaseImpl* pImpl );
HRESULT OutputPinAsync_BeginFlush( CPinBaseImpl* pImpl );
HRESULT OutputPinAsync_EndFlush( CPinBaseImpl* pImpl );
HRESULT OutputPinAsync_NewSegment( CPinBaseImpl* pImpl, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, double rate );



#endif	/* WINE_DSHOW_BASEFILT_H */
