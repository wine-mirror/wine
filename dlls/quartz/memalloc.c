/*
 * Implementation of CLSID_MemoryAllocator.
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
#include "uuids.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "memalloc.h"


/***************************************************************************
 *
 *	new/delete for CLSID_MemoryAllocator.
 *
 */

/* can I use offsetof safely? - FIXME? */
static QUARTZ_IFEntry IFEntries[] =
{
  { &IID_IMemAllocator, offsetof(CMemoryAllocator,memalloc)-offsetof(CMemoryAllocator,unk) },
};

static void QUARTZ_DestroyMemoryAllocator(IUnknown* punk)
{
	CMemoryAllocator_THIS(punk,unk);

	CMemoryAllocator_UninitIMemAllocator( This );
}

HRESULT QUARTZ_CreateMemoryAllocator(IUnknown* punkOuter,void** ppobj)
{
	CMemoryAllocator*	pma;
	HRESULT	hr;

	TRACE("(%p,%p)\n",punkOuter,ppobj);

	pma = (CMemoryAllocator*)QUARTZ_AllocObj( sizeof(CMemoryAllocator) );
	if ( pma == NULL )
		return E_OUTOFMEMORY;

	QUARTZ_IUnkInit( &pma->unk, punkOuter );
	hr = CMemoryAllocator_InitIMemAllocator( pma );
	if ( FAILED(hr) )
	{
		QUARTZ_FreeObj( pma );
		return hr;
	}

	pma->unk.pEntries = IFEntries;
	pma->unk.dwEntries = sizeof(IFEntries)/sizeof(IFEntries[0]);
	pma->unk.pOnFinalRelease = QUARTZ_DestroyMemoryAllocator;

	*ppobj = (void*)(&pma->unk);

	return S_OK;
}


/***************************************************************************
 *
 *	CMemoryAllocator::IMemAllocator
 *
 */


static HRESULT WINAPI
IMemAllocator_fnQueryInterface(IMemAllocator* iface,REFIID riid,void** ppobj)
{
	CMemoryAllocator_THIS(iface,memalloc);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IMemAllocator_fnAddRef(IMemAllocator* iface)
{
	CMemoryAllocator_THIS(iface,memalloc);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IMemAllocator_fnRelease(IMemAllocator* iface)
{
	CMemoryAllocator_THIS(iface,memalloc);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}

static HRESULT WINAPI
IMemAllocator_fnSetProperties(IMemAllocator* iface,ALLOCATOR_PROPERTIES* pPropReq,ALLOCATOR_PROPERTIES* pPropActual)
{
	CMemoryAllocator_THIS(iface,memalloc);
	long	padding;
	HRESULT	hr;

	TRACE( "(%p)->(%p,%p)\n", This, pPropReq, pPropActual );

	if ( pPropReq == NULL || pPropActual == NULL )
		return E_POINTER;
	if ( pPropReq->cBuffers < 0 ||
	     pPropReq->cbBuffer < 0 ||
	     pPropReq->cbAlign < 0 ||
	     pPropReq->cbPrefix < 0 )
		return E_INVALIDARG;

	if ( ( pPropReq->cbAlign & (pPropReq->cbAlign-1) ) != 0 )
		return E_INVALIDARG;

	hr = NOERROR;

	EnterCriticalSection( &This->csMem );

	if ( This->pData != NULL || This->ppSamples != NULL )
	{
		/* if commited, properties must not be changed. */
		hr = E_UNEXPECTED;
		goto end;
	}

	This->prop.cBuffers = pPropReq->cBuffers;
	This->prop.cbBuffer = pPropReq->cbBuffer;
	This->prop.cbAlign = pPropReq->cbAlign;
	This->prop.cbPrefix = pPropReq->cbPrefix;

	if ( This->prop.cbAlign == 0 )
		This->prop.cbAlign = 1;
	padding = This->prop.cbAlign -
		( (This->prop.cbBuffer+This->prop.cbPrefix) % This->prop.cbAlign );

	This->prop.cbBuffer += padding;

	memcpy( pPropActual, &This->prop, sizeof(ALLOCATOR_PROPERTIES) );

end:
	LeaveCriticalSection( &This->csMem );

	return hr;
}

static HRESULT WINAPI
IMemAllocator_fnGetProperties(IMemAllocator* iface,ALLOCATOR_PROPERTIES* pProp)
{
	CMemoryAllocator_THIS(iface,memalloc);

	TRACE( "(%p)->(%p)\n", This, pProp );

	if ( pProp == NULL )
		return E_POINTER;

	EnterCriticalSection( &This->csMem );

	memcpy( pProp, &This->prop, sizeof(ALLOCATOR_PROPERTIES) );

	LeaveCriticalSection( &This->csMem );

	return NOERROR;
}

static HRESULT WINAPI
IMemAllocator_fnCommit(IMemAllocator* iface)
{
	CMemoryAllocator_THIS(iface,memalloc);
	HRESULT	hr;
	LONG	lBufSize;
	LONG	i;
	BYTE*	pCur;

	TRACE( "(%p)->()\n", This );

	EnterCriticalSection( &This->csMem );

	hr = NOERROR;
	if ( This->pData != NULL || This->ppSamples != NULL ||
	     This->prop.cBuffers <= 0 )
		goto end;

	lBufSize = This->prop.cBuffers *
		(This->prop.cbBuffer + This->prop.cbPrefix) +
		This->prop.cbAlign;
	if ( lBufSize <= 0 )
		lBufSize = 1;

	This->pData = (BYTE*)QUARTZ_AllocMem( lBufSize );
	if ( This->pData == NULL )
	{
		hr = E_OUTOFMEMORY;
		goto end;
	}

	This->ppSamples = (CMemMediaSample**)QUARTZ_AllocMem(
		sizeof(CMemMediaSample*) * This->prop.cBuffers );
	if ( This->ppSamples == NULL )
	{
		hr = E_OUTOFMEMORY;
		goto end;
	}

	for ( i = 0; i < This->prop.cBuffers; i++ )
		This->ppSamples[i] = NULL;

	pCur = This->pData + This->prop.cbAlign - ((This->pData-(BYTE*)NULL) & (This->prop.cbAlign-1));

	for ( i = 0; i < This->prop.cBuffers; i++ )
	{
		hr = QUARTZ_CreateMemMediaSample(
			pCur, (This->prop.cbBuffer + This->prop.cbPrefix),
			iface, &This->ppSamples[i] );
		if ( FAILED(hr) )
			goto end;
		pCur += (This->prop.cbBuffer + This->prop.cbPrefix);
	}

	hr = NOERROR;
end:
	if ( FAILED(hr) )
		IMemAllocator_Decommit(iface);

	LeaveCriticalSection( &This->csMem );

	return hr;
}

static HRESULT WINAPI
IMemAllocator_fnDecommit(IMemAllocator* iface)
{
	CMemoryAllocator_THIS(iface,memalloc);
	HRESULT	hr;
	LONG	i;
	BOOL	bBlock;

	TRACE( "(%p)->()\n", This );

	EnterCriticalSection( &This->csMem );

	hr = NOERROR;

	if ( This->pData == NULL && This->ppSamples == NULL )
		goto end;

	while ( 1 )
	{
		bBlock = FALSE;
		i = 0;

		ResetEvent( This->hEventSample );

		while ( 1 )
		{
			if ( i >= This->prop.cBuffers )
				break;

			if ( This->ppSamples[i] != NULL )
			{
				if ( This->ppSamples[i]->ref == 0 )
				{
					QUARTZ_DestroyMemMediaSample( This->ppSamples[i] );
					This->ppSamples[i] = NULL;
				}
				else
				{
					bBlock = TRUE;
				}
			}
			i++;
		}

		if ( !bBlock )
		{
			QUARTZ_FreeMem(This->ppSamples);
			This->ppSamples = NULL;
			QUARTZ_FreeMem(This->pData);
			This->pData = NULL;
			hr = NOERROR;
			break;
		}

		WaitForSingleObject( This->hEventSample, INFINITE );
	}

end:
	LeaveCriticalSection( &This->csMem );

	return hr;
}

static HRESULT WINAPI
IMemAllocator_fnGetBuffer(IMemAllocator* iface,IMediaSample** ppSample,REFERENCE_TIME* prtStart,REFERENCE_TIME* prtEnd,DWORD dwFlags)
{
	CMemoryAllocator_THIS(iface,memalloc);
	LONG	i;
	HRESULT	hr;

	TRACE( "(%p)->(%p,%p,%p,%lu)\n", This, ppSample, prtStart, prtEnd, dwFlags );

	if ( ppSample == NULL )
		return E_POINTER;

	EnterCriticalSection( &This->csMem );

	hr = NOERROR;

	if ( This->pData == NULL || This->ppSamples == NULL ||
	     This->prop.cBuffers <= 0 )
	{
		hr = E_FAIL; /* FIXME? */
		goto end;
	}

	while ( 1 )
	{
		ResetEvent( This->hEventSample );

		for ( i = 0; i < This->prop.cBuffers; i++ )
		{
			if ( This->ppSamples[i]->ref == 0 )
			{
				*ppSample = (IMediaSample*)(This->ppSamples[i]);
				IMediaSample_AddRef( *ppSample );
				hr = NOERROR;
				goto end;
			}
		}

		if ( dwFlags & AM_GBF_NOWAIT )
		{
			hr = E_FAIL; /* FIXME? */
			goto end;
		}

		WaitForSingleObject( This->hEventSample, INFINITE );
	}

end:
	LeaveCriticalSection( &This->csMem );

	return hr;
}

static HRESULT WINAPI
IMemAllocator_fnReleaseBuffer(IMemAllocator* iface,IMediaSample* pSample)
{
	CMemoryAllocator_THIS(iface,memalloc);

	TRACE( "(%p)->(%p)\n", This, pSample );
	SetEvent( This->hEventSample );

	return NOERROR;
}



static ICOM_VTABLE(IMemAllocator) imemalloc =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IMemAllocator_fnQueryInterface,
	IMemAllocator_fnAddRef,
	IMemAllocator_fnRelease,
	/* IMemAllocator fields */
	IMemAllocator_fnSetProperties,
	IMemAllocator_fnGetProperties,
	IMemAllocator_fnCommit,
	IMemAllocator_fnDecommit,
	IMemAllocator_fnGetBuffer,
	IMemAllocator_fnReleaseBuffer,
};


HRESULT CMemoryAllocator_InitIMemAllocator( CMemoryAllocator* pma )
{
	TRACE("(%p)\n",pma);

	ICOM_VTBL(&pma->memalloc) = &imemalloc;

	ZeroMemory( &pma->prop, sizeof(pma->prop) );
	pma->hEventSample = (HANDLE)NULL;
	pma->pData = NULL;
	pma->ppSamples = NULL;

	pma->hEventSample = CreateEventA( NULL, TRUE, FALSE, NULL );
	if ( pma->hEventSample == (HANDLE)NULL )
		return E_OUTOFMEMORY;

	InitializeCriticalSection( &pma->csMem );

	return NOERROR;
}

void CMemoryAllocator_UninitIMemAllocator( CMemoryAllocator* pma )
{
	TRACE("(%p)\n",pma);

	IMemAllocator_Decommit( (IMemAllocator*)(&pma->memalloc) );

	DeleteCriticalSection( &pma->csMem );

	if ( pma->hEventSample != (HANDLE)NULL )
		CloseHandle( pma->hEventSample );
}
