/*
 * Implements IBaseFilter for transform filters. (internal)
 *
 * hidenori@a2.ctktv.ne.jp
 */

#ifndef	WINE_DSHOW_XFORM_H
#define	WINE_DSHOW_XFORM_H

#include "iunk.h"
#include "basefilt.h"
#include "seekpass.h"


typedef struct CTransformBaseImpl CTransformBaseImpl;
typedef struct CTransformBaseInPinImpl CTransformBaseInPinImpl;
typedef struct CTransformBaseOutPinImpl CTransformBaseOutPinImpl;
typedef struct TransformBaseHandlers	TransformBaseHandlers;

struct CTransformBaseImpl
{
	QUARTZ_IUnkImpl	unk;
	CBaseFilterImpl	basefilter;

	CTransformBaseInPinImpl*	pInPin;
	CTransformBaseOutPinImpl*	pOutPin;
	CSeekingPassThru*	pSeekPass;

	CRITICAL_SECTION	csFilter;
	IMemAllocator*	m_pOutPinAllocator;
	BOOL	m_bPreCopy; /* sample must be copied */
	BOOL	m_bReuseSample; /* sample must be reused */
	BOOL	m_bInFlush;
	IMediaSample*	m_pSample;

	BOOL	m_bFiltering;
	const TransformBaseHandlers*	m_pHandler;
	void*	m_pUserData;
};

struct CTransformBaseInPinImpl
{
	QUARTZ_IUnkImpl	unk;
	CPinBaseImpl	pin;
	CMemInputPinBaseImpl	meminput;

	CTransformBaseImpl*	pFilter;
};

struct CTransformBaseOutPinImpl
{
	QUARTZ_IUnkImpl	unk;
	CPinBaseImpl	pin;
	CQualityControlPassThruImpl	qcontrol;
	QUARTZ_IFDelegation	qiext;

	CTransformBaseImpl*	pFilter;
};

struct TransformBaseHandlers
{
	/* all methods must be implemented */

	HRESULT (*pInit)( CTransformBaseImpl* pImpl );
	HRESULT (*pCleanup)( CTransformBaseImpl* pImpl );

	/* pmtOut may be NULL */
	HRESULT (*pCheckMediaType)( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE* pmtOut );
	/* get output types */
	HRESULT (*pGetOutputTypes)( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE** ppmtAcceptTypes, ULONG* pcAcceptTypes );
	/* get allocator properties */
	HRESULT (*pGetAllocProp)( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE* pmtOut, ALLOCATOR_PROPERTIES* pProp, BOOL* pbTransInPlace, BOOL* pbTryToReuseSample );

	/* prepare the filter */
	HRESULT (*pBeginTransform)( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE* pmtOut, BOOL bReuseSample );
	/* process a sample */
	HRESULT (*pTransform)( CTransformBaseImpl* pImpl, IMediaSample* pSampIn, IMediaSample* pSampOut );
	/* unprepare the filter */
	HRESULT (*pEndTransform)( CTransformBaseImpl* pImpl );
};

#define	CTransformBaseImpl_THIS(iface,member)	CTransformBaseImpl*	This = ((CTransformBaseImpl*)(((char*)iface)-offsetof(CTransformBaseImpl,member)))
#define	CTransformBaseInPinImpl_THIS(iface,member)	CTransformBaseInPinImpl*	This = ((CTransformBaseInPinImpl*)(((char*)iface)-offsetof(CTransformBaseInPinImpl,member)))
#define	CTransformBaseOutPinImpl_THIS(iface,member)	CTransformBaseOutPinImpl*	This = ((CTransformBaseOutPinImpl*)(((char*)iface)-offsetof(CTransformBaseOutPinImpl,member)))


HRESULT QUARTZ_CreateTransformBase(
	IUnknown* punkOuter,void** ppobj,
	const CLSID* pclsidTransformBase,
	LPCWSTR pwszTransformBaseName,
	LPCWSTR pwszInPinName,
	LPCWSTR pwszOutPinName,
	const TransformBaseHandlers* pHandler );
HRESULT QUARTZ_CreateTransformBaseInPin(
	CTransformBaseImpl* pFilter,
	CRITICAL_SECTION* pcsPin,
	CTransformBaseInPinImpl** ppPin,
	LPCWSTR pwszPinName );
HRESULT QUARTZ_CreateTransformBaseOutPin(
	CTransformBaseImpl* pFilter,
	CRITICAL_SECTION* pcsPin,
	CTransformBaseOutPinImpl** ppPin,
	LPCWSTR pwszPinName );


HRESULT QUARTZ_CreateAVIDec(IUnknown* punkOuter,void** ppobj);
HRESULT QUARTZ_CreateColour(IUnknown* punkOuter,void** ppobj);

#endif	/* WINE_DSHOW_XFORM_H */
