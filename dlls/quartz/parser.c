/*
 * Implements IBaseFilter for parsers. (internal)
 *
 * hidenori@a2.ctktv.ne.jp
 */

#include "config.h"

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "mmsystem.h"
#include "winerror.h"
#include "wine/obj_base.h"
#include "strmif.h"
#include "vfwmsgs.h"
#include "uuids.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "parser.h"
#include "mtype.h"
#include "memalloc.h"

#define	QUARTZ_MSG_BEGINFLUSH	(WM_APP+1)
#define	QUARTZ_MSG_ENDFLUSH	(WM_APP+2)
#define	QUARTZ_MSG_EXITTHREAD		(WM_APP+3)
#define	QUARTZ_MSG_SEEK		(WM_APP+0)

/***************************************************************************
 *
 *	CParserImpl internal methods
 *
 */

static
void CParserImpl_SetAsyncReader( CParserImpl* This, IAsyncReader* pReader )
{
	if ( This->m_pReader != NULL )
	{
		IAsyncReader_Release( This->m_pReader );
		This->m_pReader = NULL;
	}
	if ( pReader != NULL )
	{
		This->m_pReader = pReader;
		IAsyncReader_AddRef(This->m_pReader);
	}
}

static
void CParserImpl_ReleaseOutPins( CParserImpl* This )
{
	ULONG nIndex;

	if ( This->m_ppOutPins != NULL )
	{
		for ( nIndex = 0; nIndex < This->m_cOutStreams; nIndex++ )
		{
			if ( This->m_ppOutPins[nIndex] != NULL )
			{
				IUnknown_Release(This->m_ppOutPins[nIndex]->unk.punkControl);
				This->m_ppOutPins[nIndex] = NULL;
			}
		}
		QUARTZ_FreeMem(This->m_ppOutPins);
		This->m_ppOutPins = NULL;
	}
	This->m_cOutStreams = 0;
}

static
void CParserImpl_ClearAllRequests( CParserImpl* This )
{
	ULONG	nIndex;

	for ( nIndex = 0; nIndex < This->m_cOutStreams; nIndex++ )
		This->m_ppOutPins[nIndex]->m_bReqUsed = FALSE;
}


static
HRESULT CParserImpl_ReleaseAllPendingSamples( CParserImpl* This )
{
	HRESULT hr;
	IMediaSample*	pSample;
	DWORD_PTR	dwContext;

	IAsyncReader_BeginFlush(This->m_pReader);
	while ( 1 )
	{
		hr = IAsyncReader_WaitForNext(This->m_pReader,0,&pSample,&dwContext);
		if ( hr != S_OK )
			break;
		IMediaSample_Release(pSample);
	}
	IAsyncReader_EndFlush(This->m_pReader);

	if ( hr == VFW_E_TIMEOUT )
		hr = NOERROR;

	return hr;
}

static
HRESULT CParserImpl_ProcessNextSample( CParserImpl* This )
{
	IMediaSample*	pSample;
	DWORD_PTR	dwContext;
	ULONG	nIndex;
	HRESULT hr;
	CParserOutPinImpl*	pOutPin;
	MSG	msg;

	while ( 1 )
	{
		if ( PeekMessageA( &msg, (HWND)NULL, 0, 0, PM_REMOVE ) )
		{
			hr = NOERROR;
			switch ( msg.message )
			{
			case QUARTZ_MSG_BEGINFLUSH:
				FIXME("BeginFlush\n");
				hr = IAsyncReader_BeginFlush(This->m_pReader);
				/* send to all output pins */
				break;
			case QUARTZ_MSG_ENDFLUSH:
				FIXME("EndFlush\n");
				hr = IAsyncReader_EndFlush(This->m_pReader);
				/* send to all output pins */
				break;
			case QUARTZ_MSG_EXITTHREAD:
				FIXME("EndThread\n");
				CParserImpl_ReleaseAllPendingSamples(This);
				CParserImpl_ClearAllRequests(This);
				return S_FALSE;
			case QUARTZ_MSG_SEEK:
				FIXME("Seek\n");
				break;
			default:
				FIXME( "invalid message %04u\n", (unsigned)msg.message );
				/* Notify (ABORT) */
				hr = E_FAIL;
			}

			return hr;
		}

		hr = IAsyncReader_WaitForNext(This->m_pReader,PARSER_POLL_INTERVAL,&pSample,&dwContext);
		nIndex = (ULONG)dwContext;
		if ( hr != VFW_E_TIMEOUT )
			break;
	}
	if ( FAILED(hr) )
		return hr;

	pOutPin = This->m_ppOutPins[nIndex];
	if ( pOutPin != NULL && pOutPin->m_bReqUsed )
	{
		if ( This->m_pHandler->pProcessSample != NULL )
			hr = This->m_pHandler->pProcessSample(This,nIndex,pOutPin->m_llReqStart,pOutPin->m_lReqLength,pSample);

		if ( FAILED(hr) )
		{
			/* Notify (ABORT) */
		}
		else
		{
			/* FIXME - if pin has its own allocator, sample must be copied */
			hr = CPinBaseImpl_SendSample(&pOutPin->pin,pSample);
		}
		pOutPin->m_bReqUsed = FALSE;
	}

	if ( SUCCEEDED(hr) )
		hr = NOERROR;

	IMediaSample_Release(pSample);
	return hr;
}

static
DWORD WINAPI CParserImpl_ThreadEntry( LPVOID pv )
{
	CParserImpl*	This = (CParserImpl*)pv;
	BOOL	bReqNext;
	ULONG	nIndex = 0;
	HRESULT hr;
	REFERENCE_TIME	rtSampleTimeStart, rtSampleTimeEnd;
	LONGLONG	llReqStart;
	LONG	lReqLength;
	REFERENCE_TIME	rtReqStart, rtReqStop;
	IMediaSample*	pSample;
	MSG	msg;

	/* initialize the message queue. */
	PeekMessageA( &msg, (HWND)NULL, 0, 0, PM_NOREMOVE );

	CParserImpl_ClearAllRequests(This);

	/* resume the owner thread. */
	SetEvent( This->m_hEventInit );

	TRACE( "Enter message loop.\n" );

	bReqNext = TRUE;
	while ( 1 )
	{
		if ( bReqNext )
		{
			/* Get the next request.  */
			hr = This->m_pHandler->pGetNextRequest( This, &nIndex, &llReqStart, &lReqLength, &rtReqStart, &rtReqStop );
			if ( FAILED(hr) )
			{
				/* Notify (ABORT) */
				break;
			}
			if ( hr != S_OK )
			{
				/* Flush */
				/* Notify (COMPLETE) */

				/* Waiting... */
				hr = CParserImpl_ProcessNextSample(This);
				if ( hr != S_OK )
				{
					/* notification is already sent */
					break;
				}
				continue;
			}
			rtSampleTimeStart = llReqStart * QUARTZ_TIMEUNITS;
			rtSampleTimeEnd = (llReqStart + lReqLength) * QUARTZ_TIMEUNITS;
			bReqNext = FALSE;
		}

		if ( !This->m_ppOutPins[nIndex]->m_bReqUsed )
		{
			hr = IMemAllocator_GetBuffer( This->m_pAllocator, &pSample, NULL, NULL, 0 );
			if ( FAILED(hr) )
			{
				/* Notify (ABORT) */
				break;
			}
			hr = IMediaSample_SetTime(pSample,&rtSampleTimeStart,&rtSampleTimeEnd);
			if ( SUCCEEDED(hr) )
				hr = IAsyncReader_Request(This->m_pReader,pSample,nIndex);
			if ( FAILED(hr) )
			{
				/* Notify (ABORT) */
				break;
			}

			This->m_ppOutPins[nIndex]->m_bReqUsed = TRUE;
			This->m_ppOutPins[nIndex]->m_llReqStart = llReqStart;
			This->m_ppOutPins[nIndex]->m_lReqLength = lReqLength;
			This->m_ppOutPins[nIndex]->m_rtReqStart = rtSampleTimeStart;
			This->m_ppOutPins[nIndex]->m_rtReqStop = rtSampleTimeEnd;
			bReqNext = TRUE;
			continue;
		}

		hr = CParserImpl_ProcessNextSample(This);
		if ( hr != S_OK )
		{
			/* notification is already sent */
			break;
		}
	}

	return 0;
}

static
HRESULT CParserImpl_BeginThread( CParserImpl* This )
{
	DWORD dwRes;
	HANDLE hEvents[2];

	if ( This->m_hEventInit != (HANDLE)NULL ||
		 This->m_hThread != (HANDLE)NULL )
		return E_UNEXPECTED;

	This->m_hEventInit = CreateEventA(NULL,TRUE,FALSE,NULL);
	if ( This->m_hEventInit == (HANDLE)NULL )
		return E_OUTOFMEMORY;

	/* create the processing thread. */
	This->m_hThread = CreateThread(
		NULL, 0,
		CParserImpl_ThreadEntry,
		(LPVOID)This,
		0, &This->m_dwThreadId );
	if ( This->m_hThread == (HANDLE)NULL )
		return E_FAIL;

	hEvents[0] = This->m_hEventInit;
	hEvents[1] = This->m_hThread;

	dwRes = WaitForMultipleObjects(2,hEvents,FALSE,INFINITE);
	if ( dwRes != WAIT_OBJECT_0 )
		return E_FAIL;

	return NOERROR;
}

static
void CParserImpl_EndThread( CParserImpl* This )
{
	if ( This->m_hThread != (HANDLE)NULL )
	{
		if ( PostThreadMessageA(
			This->m_dwThreadId, QUARTZ_MSG_EXITTHREAD, 0, 0 ) )
		{
			WaitForSingleObject( This->m_hThread, INFINITE );
		}
		CloseHandle( This->m_hThread );
		This->m_hThread = (HANDLE)NULL;
		This->m_dwThreadId = 0;
	}
	if ( This->m_hEventInit != (HANDLE)NULL )
	{
		CloseHandle( This->m_hEventInit );
		This->m_hEventInit = (HANDLE)NULL;
	}
}

static
HRESULT CParserImpl_MemCommit( CParserImpl* This )
{
	HRESULT hr;
	ULONG	nIndex;
	IMemAllocator*	pAlloc;

	if ( This->m_pAllocator == NULL )
		return E_UNEXPECTED;

	hr = IMemAllocator_Commit( This->m_pAllocator );
	if ( FAILED(hr) )
		return hr;

	if ( This->m_ppOutPins != NULL && This->m_cOutStreams > 0 )
	{
		for ( nIndex = 0; nIndex < This->m_cOutStreams; nIndex++ )
		{
			pAlloc = This->m_ppOutPins[nIndex]->m_pOutPinAllocator;
			if ( pAlloc != NULL && pAlloc != This->m_pAllocator )
			{
				hr = IMemAllocator_Commit( pAlloc );
				if ( FAILED(hr) )
					return hr;
			}
		}
	}

	return NOERROR;
}

static
void CParserImpl_MemDecommit( CParserImpl* This )
{
	ULONG	nIndex;
	IMemAllocator*	pAlloc;

	if ( This->m_pAllocator != NULL )
		IMemAllocator_Decommit( This->m_pAllocator );

	if ( This->m_ppOutPins != NULL && This->m_cOutStreams > 0 )
	{
		for ( nIndex = 0; nIndex < This->m_cOutStreams; nIndex++ )
		{
			pAlloc = This->m_ppOutPins[nIndex]->m_pOutPinAllocator;
			if ( pAlloc != NULL )
				IMemAllocator_Decommit( pAlloc );
		}
	}
}



/***************************************************************************
 *
 *	CParserImpl methods
 *
 */

static HRESULT CParserImpl_OnActive( CBaseFilterImpl* pImpl )
{
	CParserImpl_THIS(pImpl,basefilter);
	HRESULT hr;

	TRACE( "(%p)\n", This );

	hr = CParserImpl_MemCommit(This);
	if ( FAILED(hr) )
		return hr;
	hr = CParserImpl_BeginThread(This);
	if ( FAILED(hr) )
		return hr;

	return NOERROR;
}

static HRESULT CParserImpl_OnInactive( CBaseFilterImpl* pImpl )
{
	CParserImpl_THIS(pImpl,basefilter);

	TRACE( "(%p)\n", This );

	CParserImpl_EndThread(This);
	CParserImpl_MemDecommit(This);

	return NOERROR;
}

static HRESULT CParserImpl_OnStop( CBaseFilterImpl* pImpl )
{
	CParserImpl_THIS(pImpl,basefilter);

	FIXME( "(%p)\n", This );

	/* FIXME - reset streams. */

	return NOERROR;
}


static const CBaseFilterHandlers filterhandlers =
{
	CParserImpl_OnActive, /* pOnActive */
	CParserImpl_OnInactive, /* pOnInactive */
	CParserImpl_OnStop, /* pOnStop */
};


/***************************************************************************
 *
 *	CParserInPinImpl methods
 *
 */

static HRESULT CParserInPinImpl_OnPreConnect( CPinBaseImpl* pImpl, IPin* pPin )
{
	CParserInPinImpl_THIS(pImpl,pin);
	HRESULT hr;
	ULONG	nIndex;
	IUnknown*	punk;
	IAsyncReader* pReader = NULL;
	LPCWSTR pwszOutPinName;
	IMemAllocator*	pAllocActual;
	AM_MEDIA_TYPE*	pmt;

	TRACE("(%p,%p)\n",This,pPin);

	if ( This->pParser->m_pHandler->pInitParser == NULL ||
		 This->pParser->m_pHandler->pUninitParser == NULL ||
		 This->pParser->m_pHandler->pGetOutPinName == NULL ||
		 This->pParser->m_pHandler->pGetStreamType == NULL ||
		 This->pParser->m_pHandler->pCheckStreamType == NULL ||
		 This->pParser->m_pHandler->pGetAllocProp == NULL ||
		 This->pParser->m_pHandler->pGetNextRequest == NULL )
	{
		FIXME("this parser is not implemented.\n");
		return E_NOTIMPL;
	}

	CParserImpl_SetAsyncReader( This->pParser, NULL );
	hr = IPin_QueryInterface( pPin, &IID_IAsyncReader, (void**)&pReader );
	if ( FAILED(hr) )
		return hr;
	CParserImpl_SetAsyncReader( This->pParser, pReader );
	IAsyncReader_Release(pReader);

	/* initialize parser. */
	hr = This->pParser->m_pHandler->pInitParser(This->pParser,&This->pParser->m_cOutStreams);
	if ( FAILED(hr) )
		return hr;
	This->pParser->m_ppOutPins = (CParserOutPinImpl**)QUARTZ_AllocMem(
		sizeof(CParserOutPinImpl*) * This->pParser->m_cOutStreams );
	if ( This->pParser->m_ppOutPins == NULL )
		return E_OUTOFMEMORY;
	for ( nIndex = 0; nIndex < This->pParser->m_cOutStreams; nIndex++ )
		This->pParser->m_ppOutPins[nIndex] = NULL;

	/* create and initialize an allocator. */
	hr = This->pParser->m_pHandler->pGetAllocProp(This->pParser,&This->pParser->m_propAlloc);
	if ( FAILED(hr) )
		return hr;
	if ( This->pParser->m_pAllocator == NULL )
	{
		hr = QUARTZ_CreateMemoryAllocator(NULL,(void**)&punk);
		if ( FAILED(hr) )
			return hr;
		hr = IUnknown_QueryInterface( punk, &IID_IMemAllocator, (void**)&This->pParser->m_pAllocator );
		IUnknown_Release(punk);
		if ( FAILED(hr) )
			return hr;
	}
	pAllocActual = NULL;
	hr = IAsyncReader_RequestAllocator(pReader,This->pParser->m_pAllocator,&This->pParser->m_propAlloc,&pAllocActual);
	if ( FAILED(hr) )
		return hr;
	IMemAllocator_Release(This->pParser->m_pAllocator);
	This->pParser->m_pAllocator = pAllocActual;

	/* create output pins. */
	for ( nIndex = 0; nIndex < This->pParser->m_cOutStreams; nIndex++ )
	{
		pwszOutPinName = This->pParser->m_pHandler->pGetOutPinName(This->pParser,nIndex);
		if ( pwszOutPinName == NULL )
			return E_FAIL;
		hr = QUARTZ_CreateParserOutPin(
			This->pParser,
			&This->pParser->m_csParser,
			&This->pParser->m_ppOutPins[nIndex],
			nIndex, pwszOutPinName );
		if ( SUCCEEDED(hr) )
			hr = QUARTZ_CompList_AddComp(
				This->pParser->basefilter.pOutPins,
				(IUnknown*)&(This->pParser->m_ppOutPins[nIndex]->pin),
				NULL, 0 );
		if ( FAILED(hr) )
			return hr;
		pmt = &This->pParser->m_ppOutPins[nIndex]->m_mtOut;
		QUARTZ_MediaType_Free( pmt );
		ZeroMemory( pmt, sizeof(AM_MEDIA_TYPE) );
		hr = This->pParser->m_pHandler->pGetStreamType(This->pParser,nIndex,pmt);
		if ( FAILED(hr) )
		{
			ZeroMemory( pmt, sizeof(AM_MEDIA_TYPE) );
			return hr;
		}
		This->pParser->m_ppOutPins[nIndex]->pin.cAcceptTypes = 1;
		This->pParser->m_ppOutPins[nIndex]->pin.pmtAcceptTypes = pmt;
	}

	return NOERROR;
}

static HRESULT CParserInPinImpl_OnDisconnect( CPinBaseImpl* pImpl )
{
	CParserInPinImpl_THIS(pImpl,pin);

	CParserImpl_OnInactive(&This->pParser->basefilter);
	CParserImpl_OnStop(&This->pParser->basefilter);
	if ( This->pParser->m_pHandler->pUninitParser != NULL )
		This->pParser->m_pHandler->pUninitParser(This->pParser);
	CParserImpl_SetAsyncReader( This->pParser, NULL );
	if ( This->pParser->m_pAllocator != NULL )
	{
		IMemAllocator_Release(This->pParser->m_pAllocator);
		This->pParser->m_pAllocator = NULL;
	}

	return NOERROR;
}

static HRESULT CParserInPinImpl_CheckMediaType( CPinBaseImpl* pImpl, const AM_MEDIA_TYPE* pmt )
{
	CParserInPinImpl_THIS(pImpl,pin);

	TRACE("(%p,%p)\n",This,pmt);

	if ( !IsEqualGUID( &pmt->majortype, &MEDIATYPE_Stream ) )
		return E_FAIL;

	return NOERROR;
}

static const CBasePinHandlers inputpinhandlers =
{
	CParserInPinImpl_OnPreConnect, /* pOnPreConnect */
	NULL, /* pOnPostConnect */
	CParserInPinImpl_OnDisconnect, /* pOnDisconnect */
	CParserInPinImpl_CheckMediaType, /* pCheckMediaType */
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
 *	CParserOutPinImpl methods
 *
 */

static HRESULT CParserOutPinImpl_OnPostConnect( CPinBaseImpl* pImpl, IPin* pPin )
{
	CParserOutPinImpl_THIS(pImpl,pin);
	ALLOCATOR_PROPERTIES	propReq;
	IMemAllocator*	pAllocator;
	HRESULT hr;
	BOOL	bNewAllocator = FALSE;

	TRACE("(%p,%p)\n",This,pPin);

	if ( This->pin.pMemInputPinConnectedTo == NULL )
		return E_UNEXPECTED;

	if ( This->m_pOutPinAllocator != NULL )
	{
		IMemAllocator_Release(This->m_pOutPinAllocator);
		This->m_pOutPinAllocator = NULL;
	}

	/* try to use This->pParser->m_pAllocator. */
	ZeroMemory( &propReq, sizeof(ALLOCATOR_PROPERTIES) );
	hr = IMemInputPin_GetAllocatorRequirements(
		This->pin.pMemInputPinConnectedTo, &propReq );
	if ( propReq.cbAlign != 0 )
	{
		if ( This->pParser->m_propAlloc.cbAlign != ( This->pParser->m_propAlloc.cbAlign / propReq.cbAlign * propReq.cbAlign ) )
			bNewAllocator = TRUE;
	}
	if ( propReq.cbPrefix != 0 )
		bNewAllocator = TRUE;
	if ( !bNewAllocator )
	{
		hr = IMemInputPin_NotifyAllocator(
			This->pin.pMemInputPinConnectedTo,
			This->pParser->m_pAllocator, FALSE );
		if ( hr == NOERROR )
		{
			This->m_pOutPinAllocator = This->pParser->m_pAllocator;
			IMemAllocator_AddRef(This->m_pOutPinAllocator);
			return NOERROR;
		}
	}

	hr = IMemInputPin_GetAllocator(
			This->pin.pMemInputPinConnectedTo, &pAllocator );
	if ( FAILED(hr) )
		return hr;
	hr = IMemInputPin_NotifyAllocator(
			This->pin.pMemInputPinConnectedTo, pAllocator, FALSE );
	if ( FAILED(hr) )
	{
		IMemAllocator_Release(pAllocator);
		return hr;
	}

	This->m_pOutPinAllocator = pAllocator;
	return NOERROR;
}

static HRESULT CParserOutPinImpl_OnDisconnect( CPinBaseImpl* pImpl )
{
	CParserOutPinImpl_THIS(pImpl,pin);

	if ( This->m_pOutPinAllocator != NULL )
	{
		IMemAllocator_Release(This->m_pOutPinAllocator);
		This->m_pOutPinAllocator = NULL;
	}

	return NOERROR;
}

static HRESULT CParserOutPinImpl_CheckMediaType( CPinBaseImpl* pImpl, const AM_MEDIA_TYPE* pmt )
{
	CParserOutPinImpl_THIS(pImpl,pin);
	HRESULT hr;

	TRACE("(%p,%p)\n",This,pmt);
	if ( pmt == NULL )
		return E_POINTER;

	if ( This->pParser->m_pHandler->pCheckStreamType == NULL )
		return E_NOTIMPL;

	hr = This->pParser->m_pHandler->pCheckStreamType( This->pParser, This->nStreamIndex, pmt );
	if ( FAILED(hr) )
		return hr;

	return NOERROR;
}


static const CBasePinHandlers outputpinhandlers =
{
	NULL, /* pOnPreConnect */
	CParserOutPinImpl_OnPostConnect, /* pOnPostConnect */
	CParserOutPinImpl_OnDisconnect, /* pOnDisconnect */
	CParserOutPinImpl_CheckMediaType, /* pCheckMediaType */
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
 *	new/delete CParserImpl
 *
 */

/* can I use offsetof safely? - FIXME? */
static QUARTZ_IFEntry FilterIFEntries[] =
{
  { &IID_IPersist, offsetof(CParserImpl,basefilter)-offsetof(CParserImpl,unk) },
  { &IID_IMediaFilter, offsetof(CParserImpl,basefilter)-offsetof(CParserImpl,unk) },
  { &IID_IBaseFilter, offsetof(CParserImpl,basefilter)-offsetof(CParserImpl,unk) },
};

static void QUARTZ_DestroyParser(IUnknown* punk)
{
	CParserImpl_THIS(punk,unk);

	TRACE( "(%p)\n", This );

	if ( This->m_pInPin != NULL )
		CParserInPinImpl_OnDisconnect(&This->m_pInPin->pin);

	CParserImpl_SetAsyncReader( This, NULL );
	if ( This->m_pAllocator != NULL )
	{
		IMemAllocator_Release(This->m_pAllocator);
		This->m_pAllocator = NULL;
	}
	if ( This->m_pInPin != NULL )
	{
		IUnknown_Release(This->m_pInPin->unk.punkControl);
		This->m_pInPin = NULL;
	}
	CParserImpl_ReleaseOutPins( This );

	DeleteCriticalSection( &This->m_csParser );

	CBaseFilterImpl_UninitIBaseFilter(&This->basefilter);
}

HRESULT QUARTZ_CreateParser(
	IUnknown* punkOuter,void** ppobj,
	const CLSID* pclsidParser,
	LPCWSTR pwszParserName,
	LPCWSTR pwszInPinName,
	const ParserHandlers* pHandler )
{
	CParserImpl*	This = NULL;
	HRESULT hr;

	TRACE("(%p,%p)\n",punkOuter,ppobj);

	This = (CParserImpl*)
		QUARTZ_AllocObj( sizeof(CParserImpl) );
	if ( This == NULL )
		return E_OUTOFMEMORY;

	This->m_pInPin = NULL;
	This->m_cOutStreams = 0;
	This->m_ppOutPins = NULL;
	This->m_pReader = NULL;
	This->m_pAllocator = NULL;
	ZeroMemory( &This->m_propAlloc, sizeof(ALLOCATOR_PROPERTIES) );
	This->m_hEventInit = (HANDLE)NULL;
	This->m_hThread = (HANDLE)NULL;
	This->m_dwThreadId = 0;
	This->m_pHandler = pHandler;
	This->m_pUserData = NULL;

	QUARTZ_IUnkInit( &This->unk, punkOuter );

	hr = CBaseFilterImpl_InitIBaseFilter(
		&This->basefilter,
		This->unk.punkControl,
		pclsidParser,
		pwszParserName,
		&filterhandlers );
	if ( SUCCEEDED(hr) )
	{
		/* construct this class. */
		hr = S_OK;

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
	This->unk.pOnFinalRelease = QUARTZ_DestroyParser;
	InitializeCriticalSection( &This->m_csParser );

	/* create the input pin. */
	hr = QUARTZ_CreateParserInPin(
		This,
		&This->m_csParser,
		&This->m_pInPin,
		pwszInPinName );
	if ( SUCCEEDED(hr) )
		hr = QUARTZ_CompList_AddComp(
			This->basefilter.pInPins,
			(IUnknown*)&(This->m_pInPin->pin),
			NULL, 0 );

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
 *	new/delete CParserInPinImpl
 *
 */

/* can I use offsetof safely? - FIXME? */
static QUARTZ_IFEntry InPinIFEntries[] =
{
  { &IID_IPin, offsetof(CParserInPinImpl,pin)-offsetof(CParserInPinImpl,unk) },
  { &IID_IMemInputPin, offsetof(CParserInPinImpl,meminput)-offsetof(CParserInPinImpl,unk) },
};

static void QUARTZ_DestroyParserInPin(IUnknown* punk)
{
	CParserInPinImpl_THIS(punk,unk);

	TRACE( "(%p)\n", This );

	CPinBaseImpl_UninitIPin( &This->pin );
	CMemInputPinBaseImpl_UninitIMemInputPin( &This->meminput );
}

HRESULT QUARTZ_CreateParserInPin(
	CParserImpl* pFilter,
	CRITICAL_SECTION* pcsPin,
	CParserInPinImpl** ppPin,
	LPCWSTR pwszPinName )
{
	CParserInPinImpl*	This = NULL;
	HRESULT hr;

	TRACE("(%p,%p,%p)\n",pFilter,pcsPin,ppPin);

	This = (CParserInPinImpl*)
		QUARTZ_AllocObj( sizeof(CParserInPinImpl) );
	if ( This == NULL )
		return E_OUTOFMEMORY;

	QUARTZ_IUnkInit( &This->unk, NULL );
	This->pParser = pFilter;

	hr = CPinBaseImpl_InitIPin(
		&This->pin,
		This->unk.punkControl,
		pcsPin,
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
	This->unk.pOnFinalRelease = QUARTZ_DestroyParserInPin;

	*ppPin = This;

	TRACE("returned successfully.\n");

	return S_OK;
}


/***************************************************************************
 *
 *	new/delete CParserOutPinImpl
 *
 */

/* can I use offsetof safely? - FIXME? */
static QUARTZ_IFEntry OutPinIFEntries[] =
{
  { &IID_IPin, offsetof(CParserOutPinImpl,pin)-offsetof(CParserOutPinImpl,unk) },
  { &IID_IQualityControl, offsetof(CParserOutPinImpl,qcontrol)-offsetof(CParserOutPinImpl,unk) },
};

static void QUARTZ_DestroyParserOutPin(IUnknown* punk)
{
	CParserOutPinImpl_THIS(punk,unk);

	TRACE( "(%p)\n", This );

	QUARTZ_MediaType_Free( &This->m_mtOut );
	if ( This->m_pOutPinAllocator != NULL )
		IMemAllocator_Release(This->m_pOutPinAllocator);

	CPinBaseImpl_UninitIPin( &This->pin );
	CQualityControlPassThruImpl_UninitIQualityControl( &This->qcontrol );
}

HRESULT QUARTZ_CreateParserOutPin(
	CParserImpl* pFilter,
	CRITICAL_SECTION* pcsPin,
	CParserOutPinImpl** ppPin,
	ULONG nStreamIndex,
	LPCWSTR pwszPinName )
{
	CParserOutPinImpl*	This = NULL;
	HRESULT hr;

	TRACE("(%p,%p,%p)\n",pFilter,pcsPin,ppPin);

	This = (CParserOutPinImpl*)
		QUARTZ_AllocObj( sizeof(CParserOutPinImpl) );
	if ( This == NULL )
		return E_OUTOFMEMORY;

	QUARTZ_IUnkInit( &This->unk, NULL );
	This->pParser = pFilter;
	This->nStreamIndex = nStreamIndex;
	ZeroMemory( &This->m_mtOut, sizeof(AM_MEDIA_TYPE) );
	This->m_pOutPinAllocator = NULL;
	This->m_pUserData = NULL;
	This->m_bReqUsed = FALSE;
	This->m_llReqStart = 0;
	This->m_lReqLength = 0;
	This->m_rtReqStart = 0;
	This->m_rtReqStop = 0;


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
	This->unk.pOnFinalRelease = QUARTZ_DestroyParserOutPin;

	*ppPin = This;

	TRACE("returned successfully.\n");

	return S_OK;
}

