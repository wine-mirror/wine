/*
 * Implements Asynchronous File/URL Source.
 *
 * FIXME - not work yet.
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
#include "uuids.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "asyncsrc.h"
#include "memalloc.h"


/***************************************************************************
 *
 *	CAsyncReaderImpl internal methods
 *
 */

static DWORD WINAPI
CAsyncReaderImpl_ThreadEntry( LPVOID pv )
{
	CAsyncReaderImpl*	This = (CAsyncReaderImpl*)pv;
	HANDLE hWaitEvents[2];
	HANDLE hReadEvents[2];
	DWORD	dwRes;

	SetEvent( This->m_hEventInit );

	hWaitEvents[0] = This->m_hEventReqQueued;
	hWaitEvents[1] = This->m_hEventAbort;

	hReadEvents[0] = This->m_hEventSampQueued;
	hReadEvents[1] = This->m_hEventAbort;

	while ( 1 )
	{
		dwRes = WaitForMultipleObjects(2,hWaitEvents,FALSE,INFINITE);
		if ( dwRes != WAIT_OBJECT_0 )
			break;

		/* FIXME - process a queued request */

		dwRes = WaitForMultipleObjects(2,hReadEvents,FALSE,INFINITE);
		if ( dwRes != WAIT_OBJECT_0 )
			break;
	}

	return 0;
}

static HRESULT
CAsyncReaderImpl_BeginThread( CAsyncReaderImpl* This )
{
	DWORD dwRes;
	DWORD dwThreadId;
	HANDLE hEvents[2];

	if ( This->m_hEventInit != (HANDLE)NULL ||
		 This->m_hEventAbort != (HANDLE)NULL ||
		 This->m_hEventReqQueued != (HANDLE)NULL ||
		 This->m_hEventSampQueued != (HANDLE)NULL ||
		 This->m_hEventCompletion != (HANDLE)NULL ||
		 This->m_hThread != (HANDLE)NULL )
		return E_UNEXPECTED;

	This->m_hEventInit = CreateEventA(NULL,TRUE,FALSE,NULL);
	if ( This->m_hEventInit == (HANDLE)NULL )
		return E_OUTOFMEMORY;
	This->m_hEventAbort = CreateEventA(NULL,TRUE,FALSE,NULL);
	if ( This->m_hEventAbort == (HANDLE)NULL )
		return E_OUTOFMEMORY;
	This->m_hEventReqQueued = CreateEventA(NULL,TRUE,FALSE,NULL);
	if ( This->m_hEventReqQueued == (HANDLE)NULL )
		return E_OUTOFMEMORY;
	This->m_hEventSampQueued = CreateEventA(NULL,TRUE,FALSE,NULL);
	if ( This->m_hEventSampQueued == (HANDLE)NULL )
		return E_OUTOFMEMORY;
	This->m_hEventCompletion = CreateEventA(NULL,TRUE,FALSE,NULL);
	if ( This->m_hEventCompletion == (HANDLE)NULL )
		return E_OUTOFMEMORY;

	/* create the processing thread. */
	This->m_hThread = CreateThread(
		NULL, 0,
		CAsyncReaderImpl_ThreadEntry,
		(LPVOID)This,
		0, &dwThreadId );
	if ( This->m_hThread == (HANDLE)NULL )
		return E_FAIL;

	hEvents[0] = This->m_hEventInit;
	hEvents[1] = This->m_hThread;

	dwRes = WaitForMultipleObjects(2,hEvents,FALSE,INFINITE);
	if ( dwRes != WAIT_OBJECT_0 )
		return E_FAIL;

	return NOERROR;
}

static void
CAsyncReaderImpl_EndThread( CAsyncReaderImpl* This )
{
	if ( This->m_hThread != (HANDLE)NULL )
	{
		SetEvent( This->m_hEventAbort );

		WaitForSingleObject( This->m_hThread, INFINITE );
		CloseHandle( This->m_hThread );
		This->m_hThread = (HANDLE)NULL;
	}
	if ( This->m_hEventInit != (HANDLE)NULL )
	{
		CloseHandle( This->m_hEventInit );
		This->m_hEventInit = (HANDLE)NULL;
	}
	if ( This->m_hEventAbort != (HANDLE)NULL )
	{
		CloseHandle( This->m_hEventAbort );
		This->m_hEventAbort = (HANDLE)NULL;
	}
	if ( This->m_hEventReqQueued != (HANDLE)NULL )
	{
		CloseHandle( This->m_hEventReqQueued );
		This->m_hEventReqQueued = (HANDLE)NULL;
	}
	if ( This->m_hEventSampQueued != (HANDLE)NULL )
	{
		CloseHandle( This->m_hEventSampQueued );
		This->m_hEventSampQueued = (HANDLE)NULL;
	}
	if ( This->m_hEventCompletion != (HANDLE)NULL )
	{
		CloseHandle( This->m_hEventCompletion );
		This->m_hEventCompletion = (HANDLE)NULL;
	}
}

/***************************************************************************
 *
 *	CAsyncReaderImpl methods
 *
 */

static HRESULT WINAPI
CAsyncReaderImpl_fnQueryInterface(IAsyncReader* iface,REFIID riid,void** ppobj)
{
	ICOM_THIS(CAsyncReaderImpl,iface);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->punkControl,riid,ppobj);
}

static ULONG WINAPI
CAsyncReaderImpl_fnAddRef(IAsyncReader* iface)
{
	ICOM_THIS(CAsyncReaderImpl,iface);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->punkControl);
}

static ULONG WINAPI
CAsyncReaderImpl_fnRelease(IAsyncReader* iface)
{
	ICOM_THIS(CAsyncReaderImpl,iface);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->punkControl);
}

static HRESULT WINAPI
CAsyncReaderImpl_fnRequestAllocator(IAsyncReader* iface,IMemAllocator* pAlloc,ALLOCATOR_PROPERTIES* pProp,IMemAllocator** ppAllocActual)
{
	ICOM_THIS(CAsyncReaderImpl,iface);
	HRESULT hr;
	ALLOCATOR_PROPERTIES	propActual;
	IUnknown* punk = NULL;

	TRACE("(%p)->(%p,%p,%p)\n",This,pAlloc,pProp,ppAllocActual);

	if ( pAlloc == NULL || pProp == NULL || ppAllocActual == NULL )
		return E_POINTER;

	IMemAllocator_AddRef(pAlloc);
	hr = IMemAllocator_SetProperties( pAlloc, pProp, &propActual );
	if ( SUCCEEDED(hr) )
	{
		*ppAllocActual = pAlloc;
		return S_OK;
	}
	IMemAllocator_Release(pAlloc);

	hr = QUARTZ_CreateMemoryAllocator(NULL,(void**)&punk);
	if ( FAILED(hr) )
		return hr;
	hr = IUnknown_QueryInterface( punk, &IID_IMemAllocator, (void**)&pAlloc );
	IUnknown_Release(punk);
	if ( FAILED(hr) )
		return hr;

	hr = IMemAllocator_SetProperties( pAlloc, pProp, &propActual );
	if ( SUCCEEDED(hr) )
	{
		*ppAllocActual = pAlloc;
		return S_OK;
	}
	IMemAllocator_Release(pAlloc);

	return hr;
}

static HRESULT WINAPI
CAsyncReaderImpl_fnRequest(IAsyncReader* iface,IMediaSample* pSample,DWORD_PTR dwContext)
{
	ICOM_THIS(CAsyncReaderImpl,iface);

	FIXME("(%p)->() stub!\n",This);

	EnterCriticalSection( This->pcsReader );
	LeaveCriticalSection( This->pcsReader );

	return E_NOTIMPL;
}

static HRESULT WINAPI
CAsyncReaderImpl_fnWaitForNext(IAsyncReader* iface,DWORD dwTimeout,IMediaSample** pSample,DWORD_PTR* pdwContext)
{
	ICOM_THIS(CAsyncReaderImpl,iface);

	FIXME("(%p)->() stub!\n",This);

	EnterCriticalSection( This->pcsReader );
	LeaveCriticalSection( This->pcsReader );

	return E_NOTIMPL;
}

static HRESULT WINAPI
CAsyncReaderImpl_fnSyncReadAligned(IAsyncReader* iface,IMediaSample* pSample)
{
	ICOM_THIS(CAsyncReaderImpl,iface);

	FIXME("(%p)->() stub!\n",This);

	EnterCriticalSection( This->pcsReader );
	LeaveCriticalSection( This->pcsReader );

	return E_NOTIMPL;
}

static HRESULT WINAPI
CAsyncReaderImpl_fnSyncRead(IAsyncReader* iface,LONGLONG llPosStart,LONG lLength,BYTE* pbBuf)
{
	ICOM_THIS(CAsyncReaderImpl,iface);

	FIXME("(%p)->() stub!\n",This);

	EnterCriticalSection( This->pcsReader );
	LeaveCriticalSection( This->pcsReader );

	return E_NOTIMPL;
}

static HRESULT WINAPI
CAsyncReaderImpl_fnLength(IAsyncReader* iface,LONGLONG* pllTotal,LONGLONG* pllAvailable)
{
	ICOM_THIS(CAsyncReaderImpl,iface);
	HRESULT hr;

	TRACE("(%p)->()\n",This);

	EnterCriticalSection( This->pcsReader );
	hr = This->pSource->m_pHandler->pGetLength( This->pSource, pllTotal, pllAvailable );
	LeaveCriticalSection( This->pcsReader );

	return hr;
}

static HRESULT WINAPI
CAsyncReaderImpl_fnBeginFlush(IAsyncReader* iface)
{
	ICOM_THIS(CAsyncReaderImpl,iface);

	FIXME("(%p)->() stub!\n",This);

	EnterCriticalSection( This->pcsReader );
	LeaveCriticalSection( This->pcsReader );

	return E_NOTIMPL;
}

static HRESULT WINAPI
CAsyncReaderImpl_fnEndFlush(IAsyncReader* iface)
{
	ICOM_THIS(CAsyncReaderImpl,iface);

	FIXME("(%p)->() stub!\n",This);

	EnterCriticalSection( This->pcsReader );
	LeaveCriticalSection( This->pcsReader );

	return E_NOTIMPL;
}


static ICOM_VTABLE(IAsyncReader) iasyncreader =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	CAsyncReaderImpl_fnQueryInterface,
	CAsyncReaderImpl_fnAddRef,
	CAsyncReaderImpl_fnRelease,

	/* IAsyncReader fields */
	CAsyncReaderImpl_fnRequestAllocator,
	CAsyncReaderImpl_fnRequest,
	CAsyncReaderImpl_fnWaitForNext,
	CAsyncReaderImpl_fnSyncReadAligned,
	CAsyncReaderImpl_fnSyncRead,
	CAsyncReaderImpl_fnLength,
	CAsyncReaderImpl_fnBeginFlush,
	CAsyncReaderImpl_fnEndFlush,
};

HRESULT CAsyncReaderImpl_InitIAsyncReader(
	CAsyncReaderImpl* This, IUnknown* punkControl,
	CAsyncSourceImpl* pSource,
	CRITICAL_SECTION* pcsReader )
{
	TRACE("(%p,%p)\n",This,punkControl);

	if ( punkControl == NULL )
	{
		ERR( "punkControl must not be NULL\n" );
		return E_INVALIDARG;
	}

	ICOM_VTBL(This) = &iasyncreader;
	This->punkControl = punkControl;
	This->pSource = pSource;
	This->pcsReader = pcsReader;
	This->m_hEventInit = (HANDLE)NULL;
	This->m_hEventAbort = (HANDLE)NULL;
	This->m_hEventReqQueued = (HANDLE)NULL;
	This->m_hEventSampQueued = (HANDLE)NULL;
	This->m_hEventCompletion = (HANDLE)NULL;
	This->m_hThread = (HANDLE)NULL;

	return NOERROR;
}

void CAsyncReaderImpl_UninitIAsyncReader(
	CAsyncReaderImpl* This )
{
	TRACE("(%p)\n",This);
}

/***************************************************************************
 *
 *	CFileSourceFilterImpl
 *
 */

static HRESULT WINAPI
CFileSourceFilterImpl_fnQueryInterface(IFileSourceFilter* iface,REFIID riid,void** ppobj)
{
	ICOM_THIS(CFileSourceFilterImpl,iface);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->punkControl,riid,ppobj);
}

static ULONG WINAPI
CFileSourceFilterImpl_fnAddRef(IFileSourceFilter* iface)
{
	ICOM_THIS(CFileSourceFilterImpl,iface);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->punkControl);
}

static ULONG WINAPI
CFileSourceFilterImpl_fnRelease(IFileSourceFilter* iface)
{
	ICOM_THIS(CFileSourceFilterImpl,iface);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->punkControl);
}

static HRESULT WINAPI
CFileSourceFilterImpl_fnLoad(IFileSourceFilter* iface,LPCOLESTR pFileName,const AM_MEDIA_TYPE* pmt)
{
	ICOM_THIS(CFileSourceFilterImpl,iface);
	HRESULT hr;

	TRACE("(%p)->(%s,%p)\n",This,debugstr_w(pFileName),pmt);

	if ( pFileName == NULL )
		return E_POINTER;

	if ( This->m_pwszFileName != NULL )
		return E_UNEXPECTED;

	This->m_cbFileName = sizeof(WCHAR)*(lstrlenW(pFileName)+1);
	This->m_pwszFileName = (WCHAR*)QUARTZ_AllocMem( This->m_cbFileName );
	if ( This->m_pwszFileName == NULL )
		return E_OUTOFMEMORY;
	memcpy( This->m_pwszFileName, pFileName, This->m_cbFileName );

	if ( pmt != NULL )
	{
		hr = QUARTZ_MediaType_Copy( &This->m_mt, pmt );
		if ( FAILED(hr) )
			goto err;
	}
	else
	{
		ZeroMemory( &This->m_mt, sizeof(AM_MEDIA_TYPE) );
		memcpy( &This->m_mt.majortype, &MEDIATYPE_Stream, sizeof(GUID) );
		memcpy( &This->m_mt.subtype, &MEDIASUBTYPE_NULL, sizeof(GUID) );
		This->m_mt.lSampleSize = 1;
		memcpy( &This->m_mt.formattype, &FORMAT_None, sizeof(GUID) );
	}

	hr = This->pSource->m_pHandler->pLoad( This->pSource, pFileName );
	if ( FAILED(hr) )
		goto err;

	return NOERROR;
err:;
	return hr;
}

static HRESULT WINAPI
CFileSourceFilterImpl_fnGetCurFile(IFileSourceFilter* iface,LPOLESTR* ppFileName,AM_MEDIA_TYPE* pmt)
{
	ICOM_THIS(CFileSourceFilterImpl,iface);
	HRESULT hr = E_NOTIMPL;

	TRACE("(%p)->(%p,%p)\n",This,ppFileName,pmt);

	if ( ppFileName == NULL || pmt == NULL )
		return E_POINTER;

	if ( This->m_pwszFileName == NULL )
		return E_FAIL;

	hr = QUARTZ_MediaType_Copy( pmt, &This->m_mt );
	if ( FAILED(hr) )
		return hr;

	*ppFileName = (WCHAR*)CoTaskMemAlloc( This->m_cbFileName );
	if ( *ppFileName == NULL )
	{
		QUARTZ_MediaType_Free(pmt);
		ZeroMemory( pmt, sizeof(AM_MEDIA_TYPE) );
		return E_OUTOFMEMORY;
	}

	memcpy( *ppFileName, This->m_pwszFileName, This->m_cbFileName );

	return NOERROR;
}

static ICOM_VTABLE(IFileSourceFilter) ifilesource =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	CFileSourceFilterImpl_fnQueryInterface,
	CFileSourceFilterImpl_fnAddRef,
	CFileSourceFilterImpl_fnRelease,
	/* IFileSourceFilter fields */
	CFileSourceFilterImpl_fnLoad,
	CFileSourceFilterImpl_fnGetCurFile,
};

HRESULT CFileSourceFilterImpl_InitIFileSourceFilter(
	CFileSourceFilterImpl* This, IUnknown* punkControl,
	CAsyncSourceImpl* pSource,
	CRITICAL_SECTION* pcsFileSource )
{
	TRACE("(%p,%p)\n",This,punkControl);

	if ( punkControl == NULL )
	{
		ERR( "punkControl must not be NULL\n" );
		return E_INVALIDARG;
	}

	ICOM_VTBL(This) = &ifilesource;
	This->punkControl = punkControl;
	This->pSource = pSource;
	This->pcsFileSource = pcsFileSource;
	This->m_pwszFileName = NULL;
	This->m_cbFileName = 0;
	ZeroMemory( &This->m_mt, sizeof(AM_MEDIA_TYPE) );

	return NOERROR;
}

void CFileSourceFilterImpl_UninitIFileSourceFilter(
	CFileSourceFilterImpl* This )
{
	TRACE("(%p)\n",This);

	This->pSource->m_pHandler->pCleanup( This->pSource );
	if ( This->m_pwszFileName != NULL )
		QUARTZ_FreeMem( This->m_pwszFileName );
	QUARTZ_MediaType_Free( &This->m_mt );
}

/***************************************************************************
 *
 *	CAsyncSourcePinImpl methods
 *
 */


static HRESULT CAsyncSourcePinImpl_OnPreConnect( CPinBaseImpl* pImpl, IPin* pPin )
{
	CAsyncSourcePinImpl_THIS(pImpl,pin);

	This->bAsyncReaderQueried = FALSE;

	return NOERROR;
}

static HRESULT CAsyncSourcePinImpl_OnPostConnect( CPinBaseImpl* pImpl, IPin* pPin )
{
	CAsyncSourcePinImpl_THIS(pImpl,pin);

	if ( !This->bAsyncReaderQueried )
		return E_FAIL;

	return NOERROR;
}

static HRESULT CAsyncSourcePinImpl_OnDisconnect( CPinBaseImpl* pImpl )
{
	CAsyncSourcePinImpl_THIS(pImpl,pin);

	This->bAsyncReaderQueried = FALSE;

	return NOERROR;
}

static HRESULT CAsyncSourcePinImpl_CheckMediaType( CPinBaseImpl* pImpl, const AM_MEDIA_TYPE* pmt )
{
	CAsyncSourcePinImpl_THIS(pImpl,pin);

	TRACE("(%p,%p)\n",This,pmt);
	if ( pmt == NULL )
		return E_POINTER;

	if ( !IsEqualGUID( &pmt->majortype, &MEDIATYPE_Stream ) )
		return E_FAIL;

	return NOERROR;
}

static const CBasePinHandlers outputpinhandlers =
{
	CAsyncSourcePinImpl_OnPreConnect, /* pOnPreConnect */
	CAsyncSourcePinImpl_OnPostConnect, /* pOnPostConnect */
	CAsyncSourcePinImpl_OnDisconnect, /* pOnDisconnect */
	CAsyncSourcePinImpl_CheckMediaType, /* pCheckMediaType */
	NULL, /* pQualityNotify */
	NULL, /* pReceive */
	NULL, /* pReceiveCanBlock */
	NULL, /* pEndOfStream */
	NULL, /* pBeginFlush */
	NULL, /* pEndFlush */
	NULL, /* pNewSegment */
};

/***************************************************************************
 *
 *	CAsyncSourceImpl methods
 *
 */

static HRESULT CAsyncSourceImpl_OnActive( CBaseFilterImpl* pImpl )
{
	CAsyncSourceImpl_THIS(pImpl,basefilter);
	HRESULT hr;

	TRACE( "(%p)\n", This );

	hr = CAsyncReaderImpl_BeginThread(&This->pPin->async);
	if ( FAILED(hr) )
		return hr;

	return NOERROR;
}

static HRESULT CAsyncSourceImpl_OnInactive( CBaseFilterImpl* pImpl )
{
	CAsyncSourceImpl_THIS(pImpl,basefilter);

	TRACE( "(%p)\n", This );

	CAsyncReaderImpl_EndThread(&This->pPin->async);

	return NOERROR;
}

static const CBaseFilterHandlers filterhandlers =
{
	CAsyncSourceImpl_OnActive, /* pOnActive */
	CAsyncSourceImpl_OnInactive, /* pOnInactive */
	NULL, /* pOnStop */
};

/***************************************************************************
 *
 *	new/delete CAsyncSourceImpl
 *
 */

/* can I use offsetof safely? - FIXME? */
static QUARTZ_IFEntry FilterIFEntries[] =
{
  { &IID_IPersist, offsetof(CAsyncSourceImpl,basefilter)-offsetof(CAsyncSourceImpl,unk) },
  { &IID_IMediaFilter, offsetof(CAsyncSourceImpl,basefilter)-offsetof(CAsyncSourceImpl,unk) },
  { &IID_IBaseFilter, offsetof(CAsyncSourceImpl,basefilter)-offsetof(CAsyncSourceImpl,unk) },
  { &IID_IFileSourceFilter, offsetof(CAsyncSourceImpl,filesrc)-offsetof(CAsyncSourceImpl,unk) },
};

static void QUARTZ_DestroyAsyncSource(IUnknown* punk)
{
	CAsyncSourceImpl_THIS(punk,unk);

	TRACE( "(%p)\n", This );

	if ( This->pPin != NULL )
	{
		IUnknown_Release(This->pPin->unk.punkControl);
		This->pPin = NULL;
	}

	This->m_pHandler->pCleanup( This );

	CFileSourceFilterImpl_UninitIFileSourceFilter(&This->filesrc);
	CBaseFilterImpl_UninitIBaseFilter(&This->basefilter);

	DeleteCriticalSection( &This->csFilter );
}

HRESULT QUARTZ_CreateAsyncSource(
	IUnknown* punkOuter,void** ppobj,
	const CLSID* pclsidAsyncSource,
	LPCWSTR pwszAsyncSourceName,
	LPCWSTR pwszOutPinName,
	const AsyncSourceHandlers* pHandler )
{
	CAsyncSourceImpl*	This = NULL;
	HRESULT hr;

	TRACE("(%p,%p)\n",punkOuter,ppobj);

	This = (CAsyncSourceImpl*)
		QUARTZ_AllocObj( sizeof(CAsyncSourceImpl) );
	if ( This == NULL )
		return E_OUTOFMEMORY;

	This->pPin = NULL;
	This->m_pHandler = pHandler;
	This->m_pUserData = NULL;

	QUARTZ_IUnkInit( &This->unk, punkOuter );

	hr = CBaseFilterImpl_InitIBaseFilter(
		&This->basefilter,
		This->unk.punkControl,
		pclsidAsyncSource,
		pwszAsyncSourceName,
		&filterhandlers );
	if ( SUCCEEDED(hr) )
	{
		/* construct this class. */
		hr = CFileSourceFilterImpl_InitIFileSourceFilter(
			&This->filesrc, This->unk.punkControl,
			This, &This->csFilter );
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
	This->unk.pOnFinalRelease = QUARTZ_DestroyAsyncSource;
	InitializeCriticalSection( &This->csFilter );

	/* create the output pin. */
	hr = S_OK;

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
 *	new/delete CAsyncSourcePinImpl
 *
 */

/* can I use offsetof safely? - FIXME? */
static QUARTZ_IFEntry OutPinIFEntries[] =
{
  { &IID_IPin, offsetof(CAsyncSourcePinImpl,pin)-offsetof(CAsyncSourcePinImpl,unk) },
  /***{ &IID_IAsyncReader, offsetof(CAsyncSourcePinImpl,async)-offsetof(CAsyncSourcePinImpl,unk) },***/
};

static HRESULT CAsyncSourceImpl_OnQueryInterface(
	IUnknown* punk, const IID* piid, void** ppobj )
{
	CAsyncSourcePinImpl_THIS(punk,unk);

	if ( IsEqualGUID( &IID_IAsyncReader, piid ) )
	{
		*ppobj = (void*)&This->async;
		IUnknown_AddRef(punk);
		This->bAsyncReaderQueried = TRUE;
		return S_OK;
	}

	return E_NOINTERFACE;
}

static void QUARTZ_DestroyAsyncSourcePin(IUnknown* punk)
{
	CAsyncSourcePinImpl_THIS(punk,unk);

	TRACE( "(%p)\n", This );

	CAsyncReaderImpl_UninitIAsyncReader( &This->async );
	CPinBaseImpl_UninitIPin( &This->pin );
}

HRESULT QUARTZ_CreateAsyncSourcePin(
	CAsyncSourceImpl* pFilter,
	CRITICAL_SECTION* pcsPin,
	CAsyncSourcePinImpl** ppPin,
	LPCWSTR pwszPinName )
{
	CAsyncSourcePinImpl*	This = NULL;
	HRESULT hr;

	TRACE("(%p,%p,%p)\n",pFilter,pcsPin,ppPin);

	This = (CAsyncSourcePinImpl*)
		QUARTZ_AllocObj( sizeof(CAsyncSourcePinImpl) );
	if ( This == NULL )
		return E_OUTOFMEMORY;

	QUARTZ_IUnkInit( &This->unk, NULL );
	This->qiext.pNext = NULL;
	This->qiext.pOnQueryInterface = &CAsyncSourceImpl_OnQueryInterface;
	QUARTZ_IUnkAddDelegation( &This->unk, &This->qiext );

	This->bAsyncReaderQueried = FALSE;
	This->pSource = pFilter;

	hr = CPinBaseImpl_InitIPin(
		&This->pin,
		This->unk.punkControl,
		pcsPin,
		&pFilter->basefilter,
		pwszPinName,
		TRUE,
		&outputpinhandlers );

	if ( SUCCEEDED(hr) )
	{
		hr = CAsyncReaderImpl_InitIAsyncReader(
			&This->async,
			This->unk.punkControl,
			pFilter,
			pcsPin );
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
	This->unk.pOnFinalRelease = QUARTZ_DestroyAsyncSourcePin;

	*ppPin = This;

	TRACE("returned successfully.\n");

	return S_OK;
}

