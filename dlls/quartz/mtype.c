/*
 * Implements IEnumMediaTypes and helper functions. (internal)
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
#include "mtype.h"
#include "iunk.h"


/****************************************************************************/



HRESULT QUARTZ_MediaType_Copy(
	AM_MEDIA_TYPE* pmtDst,
	const AM_MEDIA_TYPE* pmtSrc )
{
	memcpy( &pmtDst->majortype, &pmtSrc->majortype, sizeof(GUID) );
	memcpy( &pmtDst->subtype, &pmtSrc->subtype, sizeof(GUID) );
	pmtDst->bFixedSizeSamples = pmtSrc->bFixedSizeSamples;
	pmtDst->bTemporalCompression = pmtSrc->bTemporalCompression;
	pmtDst->lSampleSize = pmtSrc->lSampleSize;
	memcpy( &pmtDst->formattype, &pmtSrc->formattype, sizeof(GUID) );
	pmtDst->pUnk = NULL;
	pmtDst->cbFormat = pmtSrc->cbFormat;
	pmtDst->pbFormat = NULL;

	if ( pmtSrc->pbFormat != NULL && pmtSrc->cbFormat != 0 )
	{
		pmtDst->pbFormat = (BYTE*)CoTaskMemAlloc( pmtSrc->cbFormat );
		if ( pmtDst->pbFormat == NULL )
		{
			CoTaskMemFree( pmtDst );
			return E_OUTOFMEMORY;
		}
		memcpy( pmtDst->pbFormat, pmtSrc->pbFormat, pmtSrc->cbFormat );
	}

	if ( pmtSrc->pUnk != NULL )
	{
		pmtDst->pUnk = pmtSrc->pUnk;
		IUnknown_AddRef( pmtSrc->pUnk );
	}

	return S_OK;
}

void QUARTZ_MediaType_Free(
	AM_MEDIA_TYPE* pmt )
{
	if ( pmt->pUnk != NULL )
	{
		IUnknown_Release( pmt->pUnk );
		pmt->pUnk = NULL;
	}
	if ( pmt->pbFormat != NULL )
	{
		CoTaskMemFree( pmt->pbFormat );
		pmt->cbFormat = 0;
		pmt->pbFormat = NULL;
	}
}

AM_MEDIA_TYPE* QUARTZ_MediaType_Duplicate(
	const AM_MEDIA_TYPE* pmtSrc )
{
	AM_MEDIA_TYPE*	pmtDup;

	pmtDup = (AM_MEDIA_TYPE*)CoTaskMemAlloc( sizeof(AM_MEDIA_TYPE) );
	if ( pmtDup == NULL )
		return NULL;
	if ( QUARTZ_MediaType_Copy( pmtDup, pmtSrc ) != S_OK )
	{
		CoTaskMemFree( pmtDup );
		return NULL;
	}

	return pmtDup;
}

void QUARTZ_MediaType_Destroy(
	AM_MEDIA_TYPE* pmt )
{
	QUARTZ_MediaType_Free( pmt );
	CoTaskMemFree( pmt );
}


/****************************************************************************/

typedef struct IEnumMediaTypesImpl
{
	ICOM_VFIELD(IEnumMediaTypes);
} IEnumMediaTypesImpl;

typedef struct
{
	QUARTZ_IUnkImpl	unk;
	IEnumMediaTypesImpl	enummtype;
	struct QUARTZ_IFEntry	IFEntries[1];
	CRITICAL_SECTION	cs;
	AM_MEDIA_TYPE*	pTypes;
	ULONG	cTypes;
	ULONG	cCur;
} CEnumMediaTypes;

#define	CEnumMediaTypes_THIS(iface,member)		CEnumMediaTypes*	This = ((CEnumMediaTypes*)(((char*)iface)-offsetof(CEnumMediaTypes,member)))



static HRESULT WINAPI
IEnumMediaTypes_fnQueryInterface(IEnumMediaTypes* iface,REFIID riid,void** ppobj)
{
	CEnumMediaTypes_THIS(iface,enummtype);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IEnumMediaTypes_fnAddRef(IEnumMediaTypes* iface)
{
	CEnumMediaTypes_THIS(iface,enummtype);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IEnumMediaTypes_fnRelease(IEnumMediaTypes* iface)
{
	CEnumMediaTypes_THIS(iface,enummtype);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}

static HRESULT WINAPI
IEnumMediaTypes_fnNext(IEnumMediaTypes* iface,ULONG cReq,AM_MEDIA_TYPE** ppmtype,ULONG* pcFetched)
{
	CEnumMediaTypes_THIS(iface,enummtype);
	HRESULT	hr;
	ULONG	cFetched;

	TRACE("(%p)->(%lu,%p,%p)\n",This,cReq,ppmtype,pcFetched);

	if ( pcFetched == NULL && cReq > 1 )
		return E_INVALIDARG;
	if ( ppmtype == NULL )
		return E_POINTER;

	EnterCriticalSection( &This->cs );

	hr = NOERROR;
	cFetched = 0;
	while ( cReq > 0 )
	{
		if ( This->cCur >= This->cTypes )
		{
			hr = S_FALSE;
			break;
		}
		ppmtype[ cFetched ] =
			QUARTZ_MediaType_Duplicate( &This->pTypes[ This->cCur ] );
		if ( ppmtype[ cFetched ] == NULL )
		{
			hr = E_OUTOFMEMORY;
			while ( cFetched > 0 )
			{
				cFetched --;
				QUARTZ_MediaType_Destroy( ppmtype[ cFetched ] );
			}
			break;
		}

		cFetched ++;

		This->cCur ++;
		cReq --;
	}

	LeaveCriticalSection( &This->cs );

	if ( pcFetched != NULL )
		*pcFetched = cFetched;

	return hr;
}

static HRESULT WINAPI
IEnumMediaTypes_fnSkip(IEnumMediaTypes* iface,ULONG cSkip)
{
	CEnumMediaTypes_THIS(iface,enummtype);
	HRESULT	hr;

	TRACE("(%p)->()\n",This);

	EnterCriticalSection( &This->cs );

	hr = NOERROR;
	while ( cSkip > 0 )
	{
		if ( This->cCur >= This->cTypes )
		{
			hr = S_FALSE;
			break;
		}
		This->cCur ++;
		cSkip --;
	}

	LeaveCriticalSection( &This->cs );

	return hr;
}

static HRESULT WINAPI
IEnumMediaTypes_fnReset(IEnumMediaTypes* iface)
{
	CEnumMediaTypes_THIS(iface,enummtype);

	TRACE("(%p)->()\n",This);

	EnterCriticalSection( &This->cs );

	This->cCur = 0;

	LeaveCriticalSection( &This->cs );

	return NOERROR;
}

static HRESULT WINAPI
IEnumMediaTypes_fnClone(IEnumMediaTypes* iface,IEnumMediaTypes** ppobj)
{
	CEnumMediaTypes_THIS(iface,enummtype);
	HRESULT	hr;

	TRACE("(%p)->()\n",This);

	if ( ppobj == NULL )
		return E_POINTER;

	EnterCriticalSection( &This->cs );

	hr = QUARTZ_CreateEnumMediaTypes(
		ppobj,
		This->pTypes, This->cTypes );
	if ( SUCCEEDED(hr) )
		IEnumMediaTypes_Skip( *ppobj, This->cCur );

	LeaveCriticalSection( &This->cs );

	return hr;
}


static ICOM_VTABLE(IEnumMediaTypes) ienummtype =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IEnumMediaTypes_fnQueryInterface,
	IEnumMediaTypes_fnAddRef,
	IEnumMediaTypes_fnRelease,
	/* IEnumMediaTypes fields */
	IEnumMediaTypes_fnNext,
	IEnumMediaTypes_fnSkip,
	IEnumMediaTypes_fnReset,
	IEnumMediaTypes_fnClone,
};


/* can I use offsetof safely? - FIXME? */
static QUARTZ_IFEntry IFEntries[] =
{
  { &IID_IEnumMediaTypes, offsetof(CEnumMediaTypes,enummtype)-offsetof(CEnumMediaTypes,unk) },
};


void QUARTZ_DestroyEnumMediaTypes(IUnknown* punk)
{
	CEnumMediaTypes_THIS(punk,unk);
	ULONG	i;

	if ( This->pTypes != NULL )
	{
		for ( i = 0; i < This->cTypes; i++ )
			QUARTZ_MediaType_Free( &This->pTypes[i] );
		QUARTZ_FreeMem( This->pTypes );
	}

	DeleteCriticalSection( &This->cs );
}

HRESULT QUARTZ_CreateEnumMediaTypes(
	IEnumMediaTypes** ppobj,
	const AM_MEDIA_TYPE* pTypes, ULONG cTypes )
{
	CEnumMediaTypes*	penum;
	AM_MEDIA_TYPE*	pTypesDup = NULL;
	ULONG	i;
	HRESULT	hr;

	TRACE("(%p,%p,%lu)\n",ppobj,pTypes,cTypes);

	if ( cTypes > 0 )
	{
		pTypesDup = (AM_MEDIA_TYPE*)QUARTZ_AllocMem(
			sizeof( AM_MEDIA_TYPE ) * cTypes );
		if ( pTypesDup == NULL )
			return E_OUTOFMEMORY;

		i = 0;
		while ( i < cTypes )
		{
			hr = QUARTZ_MediaType_Copy( &pTypesDup[i], &pTypes[i] );
			if ( FAILED(hr) )
			{
				while ( i > 0 )
				{
					i --;
					QUARTZ_MediaType_Free( &pTypesDup[i] );
				}
				QUARTZ_FreeMem( pTypesDup );
				return hr;
			}

			i ++;
		}
	}

	penum = (CEnumMediaTypes*)QUARTZ_AllocObj( sizeof(CEnumMediaTypes) );
	if ( penum == NULL )
	{
		return E_OUTOFMEMORY;
	}
	penum->pTypes = pTypesDup;
	penum->cTypes = cTypes;
	penum->cCur = 0;

	QUARTZ_IUnkInit( &penum->unk, NULL );
	ICOM_VTBL(&penum->enummtype) = &ienummtype;

	penum->unk.pEntries = IFEntries;
	penum->unk.dwEntries = sizeof(IFEntries)/sizeof(IFEntries[0]);
	penum->unk.pOnFinalRelease = QUARTZ_DestroyEnumMediaTypes;

	InitializeCriticalSection( &penum->cs );

	*ppobj = (IEnumMediaTypes*)(&penum->enummtype);

	return S_OK;
}


