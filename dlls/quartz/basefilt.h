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
typedef struct CBaseFilterImpl
{
	ICOM_VFIELD(IBaseFilter);

	/* IUnknown fields */
	IUnknown*	punkControl;
	/* IBaseFilter fields */
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

HRESULT CBaseFilterImpl_InitIBaseFilter(
	CBaseFilterImpl* This, IUnknown* punkControl,
	const CLSID* pclsidFilter, LPCWSTR lpwszNameGraph );
void CBaseFilterImpl_UninitIBaseFilter( CBaseFilterImpl* This );


/*
 * Implements IPin and IMemInputPin. (internal)
 *
 * a base class for implementing IPin.
 */

typedef struct CPinBaseImpl
{
	ICOM_VFIELD(IPin);

	/* IUnknown fields */
	IUnknown*	punkControl;
	/* IPin fields */
	DWORD	cbIdLen;
	WCHAR*	pwszId;
	BOOL	bOutput;

	CRITICAL_SECTION	csPin;
	CBaseFilterImpl*	pFilter;
	IPin*	pPinConnectedTo;
	AM_MEDIA_TYPE*	pmtConn;
} CPinBaseImpl;

typedef struct CMemInputPinBaseImpl
{
	ICOM_VFIELD(IMemInputPin);

	/* IUnknown fields */
	IUnknown*	punkControl;
	/* IMemInputPin fields */
	CRITICAL_SECTION*	pcsPin;
	IMemAllocator*	pAllocator;
	BOOL	bReadonly;
} CMemInputPinBaseImpl;


HRESULT CPinBaseImpl_InitIPin(
	CPinBaseImpl* This, IUnknown* punkControl,
	CBaseFilterImpl* pFilter, LPCWSTR pwszId,
	BOOL bOutput );
void CPinBaseImpl_UninitIPin( CPinBaseImpl* This );


HRESULT CMemInputPinBaseImpl_InitIMemInputPin(
	CMemInputPinBaseImpl* This, IUnknown* punkControl,
	CRITICAL_SECTION* pcsPin
	);
void CMemInputPinBaseImpl_UninitIMemInputPin(
	CMemInputPinBaseImpl* This );



#endif	/* WINE_DSHOW_BASEFILT_H */
