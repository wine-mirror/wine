/*
 * Implements IMediaSample2 for CMemMediaSample.
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

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "sample.h"
#include "mtype.h"


/***************************************************************************
 *
 *	CMemMediaSample::IMediaSample2
 *
 */

static HRESULT WINAPI
IMediaSample2_fnQueryInterface(IMediaSample2* iface,REFIID riid,void** ppobj)
{
	ICOM_THIS(CMemMediaSample,iface);

	TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);

	if ( ppobj == NULL )
		return E_POINTER;

	if ( IsEqualGUID( riid, &IID_IUnknown ) ||
	     IsEqualGUID( riid, &IID_IMediaSample ) ||
	     IsEqualGUID( riid, &IID_IMediaSample2 ) )
	{
		*ppobj = iface;
		IMediaSample2_AddRef(iface);
		return NOERROR;
	}

	return E_NOINTERFACE;
}

static ULONG WINAPI
IMediaSample2_fnAddRef(IMediaSample2* iface)
{
	ICOM_THIS(CMemMediaSample,iface);

	TRACE("(%p)->()\n",This);

	return InterlockedExchangeAdd(&(This->ref),1) + 1;
}

static ULONG WINAPI
IMediaSample2_fnRelease(IMediaSample2* iface)
{
	ICOM_THIS(CMemMediaSample,iface);
	LONG	ref;

	TRACE("(%p)->()\n",This);

	ref = InterlockedExchangeAdd(&(This->ref),-1) - 1;
	if ( ref > 0 )
		return (ULONG)ref;

	/* this class would be reused.. */
	if ( This->prop.pMediaType != NULL )
	{
		QUARTZ_MediaType_Destroy( This->prop.pMediaType );
		This->prop.pMediaType = NULL;
	}
	This->prop.dwTypeSpecificFlags = 0;
	This->prop.dwSampleFlags = 0;
	This->prop.lActual = This->prop.cbBuffer;

	IMemAllocator_ReleaseBuffer(This->pOwner,(IMediaSample*)iface);

	return 0;
}



static HRESULT WINAPI
IMediaSample2_fnGetPointer(IMediaSample2* iface,BYTE** ppData)
{
	ICOM_THIS(CMemMediaSample,iface);

	TRACE("(%p)->()\n",This);

	if ( ppData == NULL )
		return E_POINTER;

	*ppData = This->prop.pbBuffer;
	return NOERROR;
}

static long WINAPI
IMediaSample2_fnGetSize(IMediaSample2* iface)
{
	ICOM_THIS(CMemMediaSample,iface);

	TRACE("(%p)->()\n",This);

	return This->prop.cbBuffer;
}

static HRESULT WINAPI
IMediaSample2_fnGetTime(IMediaSample2* iface,REFERENCE_TIME* prtStart,REFERENCE_TIME* prtEnd)
{
	ICOM_THIS(CMemMediaSample,iface);

	TRACE("(%p)->(%p,%p)\n",This,prtStart,prtEnd);

	if ( prtStart == NULL || prtEnd == NULL )
		return E_POINTER;

	if ( ( This->prop.dwSampleFlags & AM_SAMPLE_TIMEVALID ) &&
		 ( This->prop.dwSampleFlags & AM_SAMPLE_STOPVALID ) )
	{
		*prtStart = This->prop.tStart;
		*prtEnd = This->prop.tStop;
		return NOERROR;
	}

	return VFW_E_MEDIA_TIME_NOT_SET;
}

static HRESULT WINAPI
IMediaSample2_fnSetTime(IMediaSample2* iface,REFERENCE_TIME* prtStart,REFERENCE_TIME* prtEnd)
{
	ICOM_THIS(CMemMediaSample,iface);

	TRACE("(%p)->(%p,%p) stub!\n",This,prtStart,prtEnd);

	This->prop.dwSampleFlags &= ~(AM_SAMPLE_TIMEVALID|AM_SAMPLE_STOPVALID);
	if ( prtStart != NULL )
	{
		This->prop.dwSampleFlags |= AM_SAMPLE_TIMEVALID;
		This->prop.tStart = *prtStart;
	}
	if ( prtEnd != NULL )
	{
		This->prop.dwSampleFlags |= AM_SAMPLE_STOPVALID;
		This->prop.tStop = *prtEnd;
	}

	return NOERROR;
}

static HRESULT WINAPI
IMediaSample2_fnIsSyncPoint(IMediaSample2* iface)
{
	ICOM_THIS(CMemMediaSample,iface);

	TRACE("(%p)->()\n",This);

	return ( This->prop.dwSampleFlags & AM_SAMPLE_SPLICEPOINT ) ?
						S_OK : S_FALSE;
}

static HRESULT WINAPI
IMediaSample2_fnSetSyncPoint(IMediaSample2* iface,BOOL bSync)
{
	ICOM_THIS(CMemMediaSample,iface);

	TRACE("(%p)->(%d)\n",This,bSync);

	if ( bSync )
		This->prop.dwSampleFlags |= AM_SAMPLE_SPLICEPOINT;
	else
		This->prop.dwSampleFlags &= ~AM_SAMPLE_SPLICEPOINT;

	return NOERROR;
}

static HRESULT WINAPI
IMediaSample2_fnIsPreroll(IMediaSample2* iface)
{
	ICOM_THIS(CMemMediaSample,iface);

	TRACE("(%p)->()\n",This);

	return ( This->prop.dwSampleFlags & AM_SAMPLE_PREROLL ) ?
						S_OK : S_FALSE;
}

static HRESULT WINAPI
IMediaSample2_fnSetPreroll(IMediaSample2* iface,BOOL bPreroll)
{
	ICOM_THIS(CMemMediaSample,iface);

	TRACE("(%p)->(%d)\n",This,bPreroll);

	if ( bPreroll )
		This->prop.dwSampleFlags |= AM_SAMPLE_PREROLL;
	else
		This->prop.dwSampleFlags &= ~AM_SAMPLE_PREROLL;

	return NOERROR;
}

static long WINAPI
IMediaSample2_fnGetActualDataLength(IMediaSample2* iface)
{
	ICOM_THIS(CMemMediaSample,iface);

	TRACE("(%p)->()\n",This);

	return This->prop.lActual;
}

static HRESULT WINAPI
IMediaSample2_fnSetActualDataLength(IMediaSample2* iface,long lLength)
{
	ICOM_THIS(CMemMediaSample,iface);

	TRACE("(%p)->(%ld)\n",This,lLength);

	if ( This->prop.cbBuffer > lLength )
		return E_INVALIDARG;

	This->prop.lActual = lLength;
	return NOERROR;
}

static HRESULT WINAPI
IMediaSample2_fnGetMediaType(IMediaSample2* iface,AM_MEDIA_TYPE** ppmt)
{
	ICOM_THIS(CMemMediaSample,iface);

	TRACE("(%p)->(%p)\n",This,ppmt);

	if ( ppmt == NULL )
		return E_POINTER;
	*ppmt = NULL;
	if ( !(This->prop.dwSampleFlags & AM_SAMPLE_TYPECHANGED) )
		return S_FALSE;

	*ppmt = QUARTZ_MediaType_Duplicate( This->prop.pMediaType );
	if ( *ppmt == NULL )
		return E_OUTOFMEMORY;

	return NOERROR;
}

static HRESULT WINAPI
IMediaSample2_fnSetMediaType(IMediaSample2* iface,AM_MEDIA_TYPE* pmt)
{
	ICOM_THIS(CMemMediaSample,iface);
	AM_MEDIA_TYPE* pmtDup;

	TRACE("(%p)->(%p)\n",This,pmt);

	if ( pmt == NULL )
	{
		/* FIXME? */
		if ( This->prop.pMediaType != NULL )
		{
			QUARTZ_MediaType_Destroy( This->prop.pMediaType );
			This->prop.pMediaType = NULL;
		}
		This->prop.dwSampleFlags &= ~AM_SAMPLE_TYPECHANGED;
		return NOERROR;
	}

	pmtDup = QUARTZ_MediaType_Duplicate( pmt );
	if ( pmtDup == NULL )
		return E_OUTOFMEMORY;

	if ( This->prop.pMediaType != NULL )
		QUARTZ_MediaType_Destroy( This->prop.pMediaType );
	This->prop.dwSampleFlags |= AM_SAMPLE_TYPECHANGED;
	This->prop.pMediaType = pmtDup;

	return NOERROR;
}

static HRESULT WINAPI
IMediaSample2_fnIsDiscontinuity(IMediaSample2* iface)
{
	ICOM_THIS(CMemMediaSample,iface);

	TRACE("(%p)->()\n",This);

	return ( This->prop.dwSampleFlags & AM_SAMPLE_DATADISCONTINUITY ) ?
						S_OK : S_FALSE;
}

static HRESULT WINAPI
IMediaSample2_fnSetDiscontinuity(IMediaSample2* iface,BOOL bDiscontinuity)
{
	ICOM_THIS(CMemMediaSample,iface);

	TRACE("(%p)->(%d)\n",This,bDiscontinuity);

	if ( bDiscontinuity )
		This->prop.dwSampleFlags |= AM_SAMPLE_DATADISCONTINUITY;
	else
		This->prop.dwSampleFlags &= ~AM_SAMPLE_DATADISCONTINUITY;

	return NOERROR;
}

static HRESULT WINAPI
IMediaSample2_fnGetMediaTime(IMediaSample2* iface,LONGLONG* pTimeStart,LONGLONG* pTimeEnd)
{
	ICOM_THIS(CMemMediaSample,iface);

	TRACE("(%p)->(%p,%p)\n",This,pTimeStart,pTimeEnd);

	if ( pTimeStart == NULL || pTimeEnd == NULL )
		return E_POINTER;

	if ( !This->fMediaTimeIsValid )
		return VFW_E_MEDIA_TIME_NOT_SET;

	*pTimeStart = This->llMediaTimeStart;
	*pTimeEnd = This->llMediaTimeEnd;

	return NOERROR;

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaSample2_fnSetMediaTime(IMediaSample2* iface,LONGLONG* pTimeStart,LONGLONG* pTimeEnd)
{
	ICOM_THIS(CMemMediaSample,iface);

	TRACE("(%p)->()\n",This);
	if ( pTimeStart == NULL || pTimeEnd == NULL )
	{
		This->fMediaTimeIsValid = FALSE;
	}
	else
	{
		This->fMediaTimeIsValid = TRUE;
		This->llMediaTimeStart = *pTimeStart;
		This->llMediaTimeEnd = *pTimeEnd;
	}

	return NOERROR;
}


static HRESULT WINAPI
IMediaSample2_fnGetProperties(IMediaSample2* iface,DWORD cbProp,BYTE* pbProp)
{
	ICOM_THIS(CMemMediaSample,iface);

	TRACE("(%p)->(%lu,%p)\n",This,cbProp,pbProp);

	if ( cbProp < 0 || cbProp > sizeof(AM_SAMPLE2_PROPERTIES) )
		return E_FAIL;
	memcpy( pbProp, &This->prop, cbProp );

	return NOERROR;
}

static HRESULT WINAPI
IMediaSample2_fnSetProperties(IMediaSample2* iface,DWORD cbProp,const BYTE* pbProp)
{
	ICOM_THIS(CMemMediaSample,iface);
	const AM_SAMPLE2_PROPERTIES* pProp;
	AM_SAMPLE2_PROPERTIES propNew;
	AM_MEDIA_TYPE* pmtDup = NULL;
	HRESULT hr = E_INVALIDARG;

	TRACE("(%p)->(%lu,%p)\n",This,cbProp,pbProp);

	if ( pbProp == NULL )
		return E_POINTER;
	pProp = (const AM_SAMPLE2_PROPERTIES*)pbProp;
	if ( cbProp != sizeof(AM_SAMPLE2_PROPERTIES) )
		goto err;

	CopyMemory( &propNew, pProp, sizeof(AM_SAMPLE2_PROPERTIES) );
	if ( propNew.cbData != sizeof(AM_SAMPLE2_PROPERTIES) )
		goto err;

	if ( This->prop.cbBuffer < propNew.lActual )
		goto err;

	if ( propNew.dwSampleFlags & AM_SAMPLE_TYPECHANGED )
	{
		pmtDup = QUARTZ_MediaType_Duplicate( propNew.pMediaType );
		if ( pmtDup == NULL )
		{
			hr = E_OUTOFMEMORY;
			goto err;
		}
	}

	if ( propNew.pbBuffer != NULL && propNew.pbBuffer != This->prop.pbBuffer )
		goto err;
	if ( propNew.cbBuffer != 0 && propNew.cbBuffer != This->prop.cbBuffer )
		goto err;

	if ( This->prop.pMediaType != NULL )
		QUARTZ_MediaType_Destroy( This->prop.pMediaType );
	CopyMemory( &This->prop, &propNew, sizeof(AM_SAMPLE2_PROPERTIES) );
	This->prop.pMediaType = pmtDup;
	pmtDup = NULL;

	hr= NOERROR;
err:
	if ( pmtDup != NULL )
		QUARTZ_MediaType_Destroy( pmtDup );

	return hr;
}


static ICOM_VTABLE(IMediaSample2) imediasample2 =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IMediaSample2_fnQueryInterface,
	IMediaSample2_fnAddRef,
	IMediaSample2_fnRelease,
	/* IMediaSample fields */
	IMediaSample2_fnGetPointer,
	IMediaSample2_fnGetSize,
	IMediaSample2_fnGetTime,
	IMediaSample2_fnSetTime,
	IMediaSample2_fnIsSyncPoint,
	IMediaSample2_fnSetSyncPoint,
	IMediaSample2_fnIsPreroll,
	IMediaSample2_fnSetPreroll,
	IMediaSample2_fnGetActualDataLength,
	IMediaSample2_fnSetActualDataLength,
	IMediaSample2_fnGetMediaType,
	IMediaSample2_fnSetMediaType,
	IMediaSample2_fnIsDiscontinuity,
	IMediaSample2_fnSetDiscontinuity,
	IMediaSample2_fnGetMediaTime,
	IMediaSample2_fnSetMediaTime,
	/* IMediaSample2 fields */
	IMediaSample2_fnGetProperties,
	IMediaSample2_fnSetProperties,
};


/***************************************************************************
 *
 *	new/delete for CMemMediaSample
 *
 */

HRESULT QUARTZ_CreateMemMediaSample(
	BYTE* pbData, DWORD dwDataLength,
	IMemAllocator* pOwner,
	CMemMediaSample** ppSample )
{
	CMemMediaSample*	pms;

	TRACE("(%p,%08lx,%p,%p)\n",pbData,dwDataLength,pOwner,ppSample);
	pms = (CMemMediaSample*)QUARTZ_AllocObj( sizeof(CMemMediaSample) );
	if ( pms == NULL )
		return E_OUTOFMEMORY;

	ICOM_VTBL(pms) = &imediasample2;
	pms->ref = 0;
	pms->pOwner = pOwner;
	pms->fMediaTimeIsValid = FALSE;
	pms->llMediaTimeStart = 0;
	pms->llMediaTimeEnd = 0;
	ZeroMemory( &(pms->prop), sizeof(pms->prop) );
	pms->prop.cbData = sizeof(pms->prop);
	pms->prop.dwTypeSpecificFlags = 0;
	pms->prop.dwSampleFlags = 0;
	pms->prop.pbBuffer = pbData;
	pms->prop.cbBuffer = (LONG)dwDataLength;
	pms->prop.lActual = (LONG)dwDataLength;

	*ppSample = pms;

	return S_OK;
}

void QUARTZ_DestroyMemMediaSample(
	CMemMediaSample* pSample )
{
	TRACE("(%p)\n",pSample);

	QUARTZ_FreeObj( pSample );
}

