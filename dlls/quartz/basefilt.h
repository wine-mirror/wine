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
} CBaseFilterImpl;

struct CBaseFilterHandlers
{
	HRESULT (*pOnActive)( CBaseFilterImpl* pImpl );
	HRESULT (*pOnInactive)( CBaseFilterImpl* pImpl );
};


HRESULT CBaseFilterImpl_InitIBaseFilter(
	CBaseFilterImpl* This, IUnknown* punkControl,
	const CLSID* pclsidFilter, LPCWSTR lpwszNameGraph,
	const CBaseFilterHandlers* pHandlers );
void CBaseFilterImpl_UninitIBaseFilter( CBaseFilterImpl* This );


/*
 * Implements IPin, IMemInputPin, and IQualityControl. (internal)
 *
 * a base class for implementing IPin.
 */

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
	CBaseFilterImpl*	pFilter;
	IPin*	pPinConnectedTo;
	AM_MEDIA_TYPE*	pmtConn;
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



#endif	/* WINE_DSHOW_BASEFILT_H */
