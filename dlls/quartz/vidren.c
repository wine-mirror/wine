/*
 * Implements CLSID_VideoRenderer.
 *
 * hidenori@a2.ctktv.ne.jp
 *
 * FIXME - use clock
 */

#include "config.h"

#include <stdlib.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "mmsystem.h"
#include "strmif.h"
#include "control.h"
#include "vfwmsgs.h"
#include "uuids.h"
#include "amvideo.h"
#include "evcode.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "vidren.h"
#include "seekpass.h"


static const WCHAR QUARTZ_VideoRenderer_Name[] =
{ 'V','i','d','e','o',' ','R','e','n','d','e','r','e','r',0 };
static const WCHAR QUARTZ_VideoRendererPin_Name[] =
{ 'I','n',0 };

#define VIDRENMSG_UPDATE	(WM_APP+0)
#define VIDRENMSG_ENDTHREAD	(WM_APP+1)

static const CHAR VIDREN_szWndClass[] = "Wine_VideoRenderer";
static const CHAR VIDREN_szWndName[] = "Wine Video Renderer";




static void VIDREN_OnPaint( CVideoRendererImpl* This, HWND hwnd )
{
	PAINTSTRUCT ps;
	const VIDEOINFOHEADER* pinfo;
	const AM_MEDIA_TYPE* pmt;

	TRACE("(%p,%08x)\n",This,hwnd);

	if ( !BeginPaint( hwnd, &ps ) )
		return;

	pmt = This->pPin->pin.pmtConn;
	if ( (!This->m_bSampleIsValid) || pmt == NULL )
		goto err;

	pinfo = (const VIDEOINFOHEADER*)pmt->pbFormat;

	StretchDIBits(
		ps.hdc,
		0, 0,
		abs(pinfo->bmiHeader.biWidth), abs(pinfo->bmiHeader.biHeight),
		0, 0,
		abs(pinfo->bmiHeader.biWidth), abs(pinfo->bmiHeader.biHeight),
		This->m_pSampleData, (BITMAPINFO*)(&pinfo->bmiHeader),
		DIB_RGB_COLORS, SRCCOPY );

err:
	EndPaint( hwnd, &ps );
}

static void VIDREN_OnQueryNewPalette( CVideoRendererImpl* This, HWND hwnd )
{
	FIXME("(%p,%08x)\n",This,hwnd);
}

static void VIDREN_OnUpdate( CVideoRendererImpl* This, HWND hwnd )
{
	MSG	msg;

	TRACE("(%p,%08x)\n",This,hwnd);

	InvalidateRect(hwnd,NULL,FALSE);
	UpdateWindow(hwnd);

	/* FIXME */
	while ( PeekMessageA(&msg,hwnd,
		VIDRENMSG_UPDATE,VIDRENMSG_UPDATE,
		PM_REMOVE) != FALSE )
	{
		/* discard this message. */
	}
}


static LRESULT CALLBACK
VIDREN_WndProc(
	HWND hwnd, UINT message,
	WPARAM wParam, LPARAM lParam )
{
	CVideoRendererImpl*	This = (CVideoRendererImpl*)
		GetWindowLongA( hwnd, 0L );

	TRACE("(%p) - %u/%u/%ld\n",This,message,wParam,lParam);

	if ( message == WM_NCCREATE )
	{
		This = (CVideoRendererImpl*)(((CREATESTRUCTA*)lParam)->lpCreateParams);
		SetWindowLongA( hwnd, 0L, (LONG)This );
		This->m_hwnd = hwnd;
	}

	if ( message == WM_NCDESTROY )
	{
		PostQuitMessage(0);
		This->m_hwnd = (HWND)NULL;
		SetWindowLongA( hwnd, 0L, (LONG)NULL );
		This = NULL;
	}

	if ( This != NULL )
	{
		switch ( message )
		{
		case WM_PAINT:
			TRACE("WM_PAINT begin\n");
			EnterCriticalSection( &This->m_csSample );
			VIDREN_OnPaint( This, hwnd );
			LeaveCriticalSection( &This->m_csSample );
			TRACE("WM_PAINT end\n");
			return 0;
		case WM_CLOSE:
			ShowWindow( hwnd, SW_HIDE );
			return 0;
		case WM_PALETTECHANGED:
			if ( hwnd == (HWND)wParam )
				break;
			/* fall through */
		case WM_QUERYNEWPALETTE:
			VIDREN_OnQueryNewPalette( This, hwnd );
			break;
		case VIDRENMSG_UPDATE:
			VIDREN_OnUpdate( This, hwnd );
			return 0;
		case VIDRENMSG_ENDTHREAD:
			DestroyWindow(hwnd);
			return 0;
		default:
			break;
		}
	}

	return DefWindowProcA( hwnd, message, wParam, lParam );
}

static BOOL VIDREN_Register( HINSTANCE hInst )
{
	WNDCLASSA	wc;
	ATOM	atom;

	wc.style = 0;
	wc.lpfnWndProc = VIDREN_WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = sizeof(LONG);
	wc.hInstance = hInst;
	wc.hIcon = LoadIconA((HINSTANCE)NULL,IDI_WINLOGOA);
	wc.hCursor = LoadCursorA((HINSTANCE)NULL,IDC_ARROWA);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = VIDREN_szWndClass;

	atom = RegisterClassA( &wc );
	if ( atom != (ATOM)0 )
		return TRUE;

	/* FIXME */
	return FALSE;
}

static HWND VIDREN_Create( HWND hwndOwner, CVideoRendererImpl* This )
{
	HINSTANCE hInst = (HINSTANCE)GetModuleHandleA(NULL);
	const VIDEOINFOHEADER* pinfo;
	DWORD	dwExStyle = 0;
	DWORD	dwStyle = WS_OVERLAPPED|WS_CAPTION|WS_MINIMIZEBOX|WS_MAXIMIZEBOX;
	RECT	rcWnd;

	if ( !VIDREN_Register( hInst ) )
		return (HWND)NULL;

	pinfo = (const VIDEOINFOHEADER*)This->pPin->pin.pmtConn->pbFormat;

	TRACE("width %ld, height %ld\n", pinfo->bmiHeader.biWidth, pinfo->bmiHeader.biHeight);

	rcWnd.left = 0;
	rcWnd.top = 0;
	rcWnd.right = pinfo->bmiHeader.biWidth;
	rcWnd.bottom = abs(pinfo->bmiHeader.biHeight);
	AdjustWindowRectEx( &rcWnd, dwStyle, FALSE, dwExStyle );

	TRACE("window width %d,height %d\n",
		rcWnd.right-rcWnd.left,rcWnd.bottom-rcWnd.top);

	return CreateWindowExA(
		dwExStyle,
		VIDREN_szWndClass, VIDREN_szWndName,
		dwStyle | WS_VISIBLE,
		100,100, /* FIXME */
		rcWnd.right-rcWnd.left, rcWnd.bottom-rcWnd.top,
		hwndOwner, (HMENU)NULL,
		hInst, (LPVOID)This );
}

static DWORD WINAPI VIDREN_ThreadEntry( LPVOID pv )
{
	CVideoRendererImpl*	This = (CVideoRendererImpl*)pv;
	MSG	msg;

	TRACE("(%p)\n",This);
	if ( !VIDREN_Create( (HWND)NULL, This ) )
		return 0;
	TRACE("VIDREN_Create succeeded\n");

	SetEvent( This->m_hEventInit );
	TRACE("Enter message loop\n");

	while ( GetMessageA(&msg,(HWND)NULL,0,0) )
	{
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}

	return 0;
}

static HRESULT VIDREN_StartThread( CVideoRendererImpl* This )
{
	DWORD dwRes;
	DWORD dwThreadId;
	HANDLE	hEvents[2];

	if ( This->m_hEventInit != (HANDLE)NULL ||
		 This->m_hwnd != (HWND)NULL ||
		 This->m_hThread != (HANDLE)NULL ||
		 This->pPin->pin.pmtConn == NULL )
		return E_UNEXPECTED;

	This->m_hEventInit = CreateEventA(NULL,TRUE,FALSE,NULL);
	if ( This->m_hEventInit == (HANDLE)NULL )
		return E_OUTOFMEMORY;

	This->m_hThread = CreateThread(
		NULL, 0,
		VIDREN_ThreadEntry,
		(LPVOID)This,
		0, &dwThreadId );
	if ( This->m_hThread == (HANDLE)NULL )
		return E_FAIL;

	hEvents[0] = This->m_hEventInit;
	hEvents[1] = This->m_hThread;

	dwRes = WaitForMultipleObjects(2,hEvents,FALSE,INFINITE);
	if ( dwRes != WAIT_OBJECT_0 )
		return E_FAIL;

	return S_OK;
}

static void VIDREN_EndThread( CVideoRendererImpl* This )
{
	if ( This->m_hwnd != (HWND)NULL )
		PostMessageA( This->m_hwnd, VIDRENMSG_ENDTHREAD, 0, 0 );

	if ( This->m_hThread != (HANDLE)NULL )
	{
		WaitForSingleObject( This->m_hThread, INFINITE );
		CloseHandle( This->m_hThread );
		This->m_hThread = (HANDLE)NULL;
	}
	if ( This->m_hEventInit != (HANDLE)NULL )
	{
		CloseHandle( This->m_hEventInit );
		This->m_hEventInit = (HANDLE)NULL;
	}
}



/***************************************************************************
 *
 *	CVideoRendererImpl methods
 *
 */

static HRESULT CVideoRendererImpl_OnActive( CBaseFilterImpl* pImpl )
{
	CVideoRendererImpl_THIS(pImpl,basefilter);

	FIXME( "(%p)\n", This );

	This->m_bSampleIsValid = FALSE;

	return NOERROR;
}

static HRESULT CVideoRendererImpl_OnInactive( CBaseFilterImpl* pImpl )
{
	CVideoRendererImpl_THIS(pImpl,basefilter);

	FIXME( "(%p)\n", This );

	EnterCriticalSection( &This->m_csSample );
	This->m_bSampleIsValid = FALSE;
	LeaveCriticalSection( &This->m_csSample );

	return NOERROR;
}

static const CBaseFilterHandlers filterhandlers =
{
	CVideoRendererImpl_OnActive, /* pOnActive */
	CVideoRendererImpl_OnInactive, /* pOnInactive */
	NULL, /* pOnStop */
};

/***************************************************************************
 *
 *	CVideoRendererPinImpl methods
 *
 */

static HRESULT CVideoRendererPinImpl_OnPreConnect( CPinBaseImpl* pImpl, IPin* pPin )
{
	CVideoRendererPinImpl_THIS(pImpl,pin);

	TRACE("(%p,%p)\n",This,pPin);

	return NOERROR;
}

static HRESULT CVideoRendererPinImpl_OnPostConnect( CPinBaseImpl* pImpl, IPin* pPin )
{
	CVideoRendererPinImpl_THIS(pImpl,pin);
	const VIDEOINFOHEADER* pinfo;
	HRESULT hr;

	TRACE("(%p,%p)\n",This,pPin);

	if ( This->pRender->m_pSampleData != NULL )
	{
		QUARTZ_FreeMem(This->pRender->m_pSampleData);
		This->pRender->m_pSampleData = NULL;
	}
	This->pRender->m_cbSampleData = 0;
	This->pRender->m_bSampleIsValid = FALSE;

	pinfo = (const VIDEOINFOHEADER*)This->pin.pmtConn->pbFormat;
	if ( pinfo == NULL )
		return E_FAIL;

	This->pRender->m_bSampleIsValid = FALSE;
	This->pRender->m_cbSampleData = DIBSIZE(pinfo->bmiHeader);
	This->pRender->m_pSampleData = (BYTE*)QUARTZ_AllocMem(This->pRender->m_cbSampleData);
	if ( This->pRender->m_pSampleData == NULL )
		return E_OUTOFMEMORY;

	hr = VIDREN_StartThread(This->pRender);
	if ( FAILED(hr) )
		return hr;

	return NOERROR;
}

static HRESULT CVideoRendererPinImpl_OnDisconnect( CPinBaseImpl* pImpl )
{
	CVideoRendererPinImpl_THIS(pImpl,pin);

	TRACE("(%p)\n",This);

	VIDREN_EndThread(This->pRender);

	if ( This->pRender->m_pSampleData != NULL )
	{
		QUARTZ_FreeMem(This->pRender->m_pSampleData);
		This->pRender->m_pSampleData = NULL;
	}
	This->pRender->m_cbSampleData = 0;
	This->pRender->m_bSampleIsValid = FALSE;

	if ( This->meminput.pAllocator != NULL )
	{
		IMemAllocator_Decommit(This->meminput.pAllocator);
		IMemAllocator_Release(This->meminput.pAllocator);
		This->meminput.pAllocator = NULL;
	}

	return NOERROR;
}

static HRESULT CVideoRendererPinImpl_CheckMediaType( CPinBaseImpl* pImpl, const AM_MEDIA_TYPE* pmt )
{
	CVideoRendererPinImpl_THIS(pImpl,pin);
	const VIDEOINFOHEADER* pinfo;

	TRACE("(%p,%p)\n",This,pmt);

	if ( !IsEqualGUID( &pmt->majortype, &MEDIATYPE_Video ) )
		return E_FAIL;
	if ( !IsEqualGUID( &pmt->formattype, &FORMAT_VideoInfo ) )
		return E_FAIL;
	/*
	 * check subtype.
	 */
	if ( !IsEqualGUID( &pmt->subtype, &MEDIASUBTYPE_RGB555 ) &&
	     !IsEqualGUID( &pmt->subtype, &MEDIASUBTYPE_RGB565 ) &&
	     !IsEqualGUID( &pmt->subtype, &MEDIASUBTYPE_RGB24 ) &&
	     !IsEqualGUID( &pmt->subtype, &MEDIASUBTYPE_RGB32 ) )
		return E_FAIL;

	/****
	 *
	 *
	if ( !IsEqualGUID( &pmt->subtype, &MEDIASUBTYPE_RGB8 ) &&
	     !IsEqualGUID( &pmt->subtype, &MEDIASUBTYPE_RGB555 ) &&
	     !IsEqualGUID( &pmt->subtype, &MEDIASUBTYPE_RGB565 ) &&
	     !IsEqualGUID( &pmt->subtype, &MEDIASUBTYPE_RGB24 ) &&
	     !IsEqualGUID( &pmt->subtype, &MEDIASUBTYPE_RGB32 ) )
		return E_FAIL;
	 *
	 ****/

	pinfo = (const VIDEOINFOHEADER*)pmt->pbFormat;
	if ( pinfo == NULL ||
		 pinfo->bmiHeader.biSize < sizeof(BITMAPINFOHEADER) ||
		 pinfo->bmiHeader.biWidth <= 0 ||
		 pinfo->bmiHeader.biHeight == 0 ||
		 pinfo->bmiHeader.biPlanes != 1 ||
		 pinfo->bmiHeader.biCompression != 0 )
		return E_FAIL;

	return NOERROR;
}

static HRESULT CVideoRendererPinImpl_Receive( CPinBaseImpl* pImpl, IMediaSample* pSample )
{
	CVideoRendererPinImpl_THIS(pImpl,pin);
	HWND hwnd;
	BYTE*	pData = NULL;
	LONG	lLength;
	HRESULT hr;

	TRACE( "(%p,%p)\n",This,pSample );

	hwnd = This->pRender->m_hwnd;
	if ( hwnd == (HWND)NULL ||
		 This->pRender->m_hThread == (HWND)NULL )
		return E_UNEXPECTED;
	if ( This->pRender->m_fInFlush )
		return S_FALSE;
	if ( pSample == NULL )
		return E_POINTER;

	/* FIXME - wait/skip/qualitycontrol */
	

	/* duplicate this sample. */
	hr = IMediaSample_GetPointer(pSample,&pData);
	if ( FAILED(hr) )
		return hr;
	lLength = (LONG)IMediaSample_GetActualDataLength(pSample);
	if ( lLength <= 0 || (lLength < (LONG)This->pRender->m_cbSampleData) )
	{
		ERR( "invalid length: %ld\n", lLength );
		return NOERROR;
	}

	EnterCriticalSection( &This->pRender->m_csSample );
	memcpy(This->pRender->m_pSampleData,pData,lLength);
	This->pRender->m_bSampleIsValid = TRUE;
	PostMessageA( hwnd, VIDRENMSG_UPDATE, 0, 0 );
	LeaveCriticalSection( &This->pRender->m_csSample );

	return NOERROR;
}

static HRESULT CVideoRendererPinImpl_ReceiveCanBlock( CPinBaseImpl* pImpl )
{
	CVideoRendererPinImpl_THIS(pImpl,pin);

	TRACE( "(%p)\n", This );

	/* might block. */
	return S_OK;
}

static HRESULT CVideoRendererPinImpl_EndOfStream( CPinBaseImpl* pImpl )
{
	CVideoRendererPinImpl_THIS(pImpl,pin);

	FIXME( "(%p)\n", This );

	This->pRender->m_fInFlush = FALSE;

	/* FIXME - don't notify twice until stopped or seeked. */
	return CBaseFilterImpl_MediaEventNotify(
		&This->pRender->basefilter, EC_COMPLETE,
		(LONG_PTR)S_OK, (LONG_PTR)(IBaseFilter*)(This->pRender) );
}

static HRESULT CVideoRendererPinImpl_BeginFlush( CPinBaseImpl* pImpl )
{
	CVideoRendererPinImpl_THIS(pImpl,pin);

	FIXME( "(%p)\n", This );

	This->pRender->m_fInFlush = TRUE;
	EnterCriticalSection( &This->pRender->m_csSample );
	This->pRender->m_bSampleIsValid = FALSE;
	LeaveCriticalSection( &This->pRender->m_csSample );

	return NOERROR;
}

static HRESULT CVideoRendererPinImpl_EndFlush( CPinBaseImpl* pImpl )
{
	CVideoRendererPinImpl_THIS(pImpl,pin);

	FIXME( "(%p)\n", This );

	This->pRender->m_fInFlush = FALSE;

	return NOERROR;
}

static HRESULT CVideoRendererPinImpl_NewSegment( CPinBaseImpl* pImpl, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, double rate )
{
	CVideoRendererPinImpl_THIS(pImpl,pin);

	FIXME( "(%p)\n", This );

	This->pRender->m_fInFlush = FALSE;

	return NOERROR;
}




static const CBasePinHandlers pinhandlers =
{
	CVideoRendererPinImpl_OnPreConnect, /* pOnPreConnect */
	CVideoRendererPinImpl_OnPostConnect, /* pOnPostConnect */
	CVideoRendererPinImpl_OnDisconnect, /* pOnDisconnect */
	CVideoRendererPinImpl_CheckMediaType, /* pCheckMediaType */
	NULL, /* pQualityNotify */
	CVideoRendererPinImpl_Receive, /* pReceive */
	CVideoRendererPinImpl_ReceiveCanBlock, /* pReceiveCanBlock */
	CVideoRendererPinImpl_EndOfStream, /* pEndOfStream */
	CVideoRendererPinImpl_BeginFlush, /* pBeginFlush */
	CVideoRendererPinImpl_EndFlush, /* pEndFlush */
	CVideoRendererPinImpl_NewSegment, /* pNewSegment */
};


/***************************************************************************
 *
 *	new/delete CVideoRendererImpl
 *
 */

/* can I use offsetof safely? - FIXME? */
static QUARTZ_IFEntry FilterIFEntries[] =
{
  { &IID_IPersist, offsetof(CVideoRendererImpl,basefilter)-offsetof(CVideoRendererImpl,unk) },
  { &IID_IMediaFilter, offsetof(CVideoRendererImpl,basefilter)-offsetof(CVideoRendererImpl,unk) },
  { &IID_IBaseFilter, offsetof(CVideoRendererImpl,basefilter)-offsetof(CVideoRendererImpl,unk) },
  { &IID_IBasicVideo, offsetof(CVideoRendererImpl,basvid)-offsetof(CVideoRendererImpl,unk) },
  { &IID_IBasicVideo2, offsetof(CVideoRendererImpl,basvid)-offsetof(CVideoRendererImpl,unk) },
  { &IID_IVideoWindow, offsetof(CVideoRendererImpl,vidwin)-offsetof(CVideoRendererImpl,unk) },
};

static HRESULT CVideoRendererImpl_OnQueryInterface(
	IUnknown* punk, const IID* piid, void** ppobj )
{
	CVideoRendererImpl_THIS(punk,unk);

	if ( This->pSeekPass == NULL )
		return E_NOINTERFACE;

	if ( IsEqualGUID( &IID_IMediaPosition, piid ) ||
		 IsEqualGUID( &IID_IMediaSeeking, piid ) )
	{
		TRACE( "IMediaSeeking(or IMediaPosition) is queried\n" );
		return IUnknown_QueryInterface( (IUnknown*)(&This->pSeekPass->unk), piid, ppobj );
	}

	return E_NOINTERFACE;
}

static void QUARTZ_DestroyVideoRenderer(IUnknown* punk)
{
	CVideoRendererImpl_THIS(punk,unk);

	TRACE( "(%p)\n", This );
	CVideoRendererImpl_OnInactive(&This->basefilter);
	VIDREN_EndThread(This);

	if ( This->pPin != NULL )
	{
		IUnknown_Release(This->pPin->unk.punkControl);
		This->pPin = NULL;
	}
	if ( This->pSeekPass != NULL )
	{
		IUnknown_Release((IUnknown*)&This->pSeekPass->unk);
		This->pSeekPass = NULL;
	}

	CVideoRendererImpl_UninitIBasicVideo2(This);
	CVideoRendererImpl_UninitIVideoWindow(This);
	CBaseFilterImpl_UninitIBaseFilter(&This->basefilter);

	DeleteCriticalSection( &This->m_csSample );
}

HRESULT QUARTZ_CreateVideoRenderer(IUnknown* punkOuter,void** ppobj)
{
	CVideoRendererImpl*	This = NULL;
	HRESULT hr;

	TRACE("(%p,%p)\n",punkOuter,ppobj);

	This = (CVideoRendererImpl*)
		QUARTZ_AllocObj( sizeof(CVideoRendererImpl) );
	if ( This == NULL )
		return E_OUTOFMEMORY;
	This->pSeekPass = NULL;
	This->pPin = NULL;
	This->m_fInFlush = FALSE;

	This->m_hEventInit = (HANDLE)NULL;
	This->m_hThread = (HANDLE)NULL;
	This->m_hwnd = (HWND)NULL;
	This->m_bSampleIsValid = FALSE;
	This->m_pSampleData = NULL;
	This->m_cbSampleData = 0;

	QUARTZ_IUnkInit( &This->unk, punkOuter );
	This->qiext.pNext = NULL;
	This->qiext.pOnQueryInterface = &CVideoRendererImpl_OnQueryInterface;
	QUARTZ_IUnkAddDelegation( &This->unk, &This->qiext );

	hr = CBaseFilterImpl_InitIBaseFilter(
		&This->basefilter,
		This->unk.punkControl,
		&CLSID_VideoRenderer,
		QUARTZ_VideoRenderer_Name,
		&filterhandlers );
	if ( SUCCEEDED(hr) )
	{
		hr = CVideoRendererImpl_InitIBasicVideo2(This);
		if ( SUCCEEDED(hr) )
		{
			hr = CVideoRendererImpl_InitIVideoWindow(This);
			if ( FAILED(hr) )
			{
				CVideoRendererImpl_UninitIBasicVideo2(This);
			}
		}
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
	This->unk.pOnFinalRelease = QUARTZ_DestroyVideoRenderer;

	InitializeCriticalSection( &This->m_csSample );

	hr = QUARTZ_CreateVideoRendererPin(
		This,
		&This->basefilter.csFilter,
		&This->pPin );
	if ( SUCCEEDED(hr) )
		hr = QUARTZ_CompList_AddComp(
			This->basefilter.pInPins,
			(IUnknown*)&This->pPin->pin,
			NULL, 0 );
	if ( SUCCEEDED(hr) )
		hr = QUARTZ_CreateSeekingPassThruInternal(
			(IUnknown*)&(This->unk), &This->pSeekPass,
			TRUE, (IPin*)&(This->pPin->pin) );

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
 *	new/delete CVideoRendererPinImpl
 *
 */

/* can I use offsetof safely? - FIXME? */
static QUARTZ_IFEntry PinIFEntries[] =
{
  { &IID_IPin, offsetof(CVideoRendererPinImpl,pin)-offsetof(CVideoRendererPinImpl,unk) },
  { &IID_IMemInputPin, offsetof(CVideoRendererPinImpl,meminput)-offsetof(CVideoRendererPinImpl,unk) },
};

static void QUARTZ_DestroyVideoRendererPin(IUnknown* punk)
{
	CVideoRendererPinImpl_THIS(punk,unk);

	TRACE( "(%p)\n", This );

	CPinBaseImpl_UninitIPin( &This->pin );
	CMemInputPinBaseImpl_UninitIMemInputPin( &This->meminput );
}

HRESULT QUARTZ_CreateVideoRendererPin(
        CVideoRendererImpl* pFilter,
        CRITICAL_SECTION* pcsPin,
        CVideoRendererPinImpl** ppPin)
{
	CVideoRendererPinImpl*	This = NULL;
	HRESULT hr;

	TRACE("(%p,%p,%p)\n",pFilter,pcsPin,ppPin);

	This = (CVideoRendererPinImpl*)
		QUARTZ_AllocObj( sizeof(CVideoRendererPinImpl) );
	if ( This == NULL )
		return E_OUTOFMEMORY;

	QUARTZ_IUnkInit( &This->unk, NULL );
	This->pRender = pFilter;

	hr = CPinBaseImpl_InitIPin(
		&This->pin,
		This->unk.punkControl,
		pcsPin,
		&pFilter->basefilter,
		QUARTZ_VideoRendererPin_Name,
		FALSE,
		&pinhandlers );

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

	This->unk.pEntries = PinIFEntries;
	This->unk.dwEntries = sizeof(PinIFEntries)/sizeof(PinIFEntries[0]);
	This->unk.pOnFinalRelease = QUARTZ_DestroyVideoRendererPin;

	*ppPin = This;

	TRACE("returned successfully.\n");

	return S_OK;
}

/***************************************************************************
 *
 *	CVideoRendererImpl::IBasicVideo2
 *
 */


static HRESULT WINAPI
IBasicVideo2_fnQueryInterface(IBasicVideo2* iface,REFIID riid,void** ppobj)
{
	CVideoRendererImpl_THIS(iface,basvid);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IBasicVideo2_fnAddRef(IBasicVideo2* iface)
{
	CVideoRendererImpl_THIS(iface,basvid);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IBasicVideo2_fnRelease(IBasicVideo2* iface)
{
	CVideoRendererImpl_THIS(iface,basvid);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}

static HRESULT WINAPI
IBasicVideo2_fnGetTypeInfoCount(IBasicVideo2* iface,UINT* pcTypeInfo)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnGetTypeInfo(IBasicVideo2* iface,UINT iTypeInfo, LCID lcid, ITypeInfo** ppobj)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnGetIDsOfNames(IBasicVideo2* iface,REFIID riid, LPOLESTR* ppwszName, UINT cNames, LCID lcid, DISPID* pDispId)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnInvoke(IBasicVideo2* iface,DISPID DispId, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarRes, EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}


static HRESULT WINAPI
IBasicVideo2_fnget_AvgTimePerFrame(IBasicVideo2* iface,REFTIME* prefTime)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnget_BitRate(IBasicVideo2* iface,long* plRate)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnget_BitErrorRate(IBasicVideo2* iface,long* plRate)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnget_VideoWidth(IBasicVideo2* iface,long* plWidth)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnget_VideoHeight(IBasicVideo2* iface,long* plHeight)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnput_SourceLeft(IBasicVideo2* iface,long lLeft)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnget_SourceLeft(IBasicVideo2* iface,long* plLeft)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnput_SourceWidth(IBasicVideo2* iface,long lWidth)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnget_SourceWidth(IBasicVideo2* iface,long* plWidth)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnput_SourceTop(IBasicVideo2* iface,long lTop)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnget_SourceTop(IBasicVideo2* iface,long* plTop)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnput_SourceHeight(IBasicVideo2* iface,long lHeight)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnget_SourceHeight(IBasicVideo2* iface,long* plHeight)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnput_DestinationLeft(IBasicVideo2* iface,long lLeft)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnget_DestinationLeft(IBasicVideo2* iface,long* plLeft)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnput_DestinationWidth(IBasicVideo2* iface,long lWidth)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnget_DestinationWidth(IBasicVideo2* iface,long* plWidth)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnput_DestinationTop(IBasicVideo2* iface,long lTop)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnget_DestinationTop(IBasicVideo2* iface,long* plTop)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnput_DestinationHeight(IBasicVideo2* iface,long lHeight)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnget_DestinationHeight(IBasicVideo2* iface,long* plHeight)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnSetSourcePosition(IBasicVideo2* iface,long lLeft,long lTop,long lWidth,long lHeight)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnGetSourcePosition(IBasicVideo2* iface,long* plLeft,long* plTop,long* plWidth,long* plHeight)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnSetDefaultSourcePosition(IBasicVideo2* iface)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnSetDestinationPosition(IBasicVideo2* iface,long lLeft,long lTop,long lWidth,long lHeight)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnGetDestinationPosition(IBasicVideo2* iface,long* plLeft,long* plTop,long* plWidth,long* plHeight)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnSetDefaultDestinationPosition(IBasicVideo2* iface)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnGetVideoSize(IBasicVideo2* iface,long* plWidth,long* plHeight)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnGetVideoPaletteEntries(IBasicVideo2* iface,long lStart,long lCount,long* plRet,long* plPaletteEntry)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnGetCurrentImage(IBasicVideo2* iface,long* plBufferSize,long* plDIBBuffer)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnIsUsingDefaultSource(IBasicVideo2* iface)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnIsUsingDefaultDestination(IBasicVideo2* iface)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnGetPreferredAspectRatio(IBasicVideo2* iface,long* plRateX,long* plRateY)
{
	CVideoRendererImpl_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}




static ICOM_VTABLE(IBasicVideo2) ibasicvideo =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IBasicVideo2_fnQueryInterface,
	IBasicVideo2_fnAddRef,
	IBasicVideo2_fnRelease,
	/* IDispatch fields */
	IBasicVideo2_fnGetTypeInfoCount,
	IBasicVideo2_fnGetTypeInfo,
	IBasicVideo2_fnGetIDsOfNames,
	IBasicVideo2_fnInvoke,
	/* IBasicVideo fields */
	IBasicVideo2_fnget_AvgTimePerFrame,
	IBasicVideo2_fnget_BitRate,
	IBasicVideo2_fnget_BitErrorRate,
	IBasicVideo2_fnget_VideoWidth,
	IBasicVideo2_fnget_VideoHeight,
	IBasicVideo2_fnput_SourceLeft,
	IBasicVideo2_fnget_SourceLeft,
	IBasicVideo2_fnput_SourceWidth,
	IBasicVideo2_fnget_SourceWidth,
	IBasicVideo2_fnput_SourceTop,
	IBasicVideo2_fnget_SourceTop,
	IBasicVideo2_fnput_SourceHeight,
	IBasicVideo2_fnget_SourceHeight,
	IBasicVideo2_fnput_DestinationLeft,
	IBasicVideo2_fnget_DestinationLeft,
	IBasicVideo2_fnput_DestinationWidth,
	IBasicVideo2_fnget_DestinationWidth,
	IBasicVideo2_fnput_DestinationTop,
	IBasicVideo2_fnget_DestinationTop,
	IBasicVideo2_fnput_DestinationHeight,
	IBasicVideo2_fnget_DestinationHeight,
	IBasicVideo2_fnSetSourcePosition,
	IBasicVideo2_fnGetSourcePosition,
	IBasicVideo2_fnSetDefaultSourcePosition,
	IBasicVideo2_fnSetDestinationPosition,
	IBasicVideo2_fnGetDestinationPosition,
	IBasicVideo2_fnSetDefaultDestinationPosition,
	IBasicVideo2_fnGetVideoSize,
	IBasicVideo2_fnGetVideoPaletteEntries,
	IBasicVideo2_fnGetCurrentImage,
	IBasicVideo2_fnIsUsingDefaultSource,
	IBasicVideo2_fnIsUsingDefaultDestination,
	/* IBasicVideo2 fields */
	IBasicVideo2_fnGetPreferredAspectRatio,
};


HRESULT CVideoRendererImpl_InitIBasicVideo2( CVideoRendererImpl* This )
{
	TRACE("(%p)\n",This);
	ICOM_VTBL(&This->basvid) = &ibasicvideo;

	return NOERROR;
}

void CVideoRendererImpl_UninitIBasicVideo2( CVideoRendererImpl* This )
{
	TRACE("(%p)\n",This);
}

/***************************************************************************
 *
 *	CVideoRendererImpl::IVideoWindow
 *
 */


static HRESULT WINAPI
IVideoWindow_fnQueryInterface(IVideoWindow* iface,REFIID riid,void** ppobj)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IVideoWindow_fnAddRef(IVideoWindow* iface)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IVideoWindow_fnRelease(IVideoWindow* iface)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}

static HRESULT WINAPI
IVideoWindow_fnGetTypeInfoCount(IVideoWindow* iface,UINT* pcTypeInfo)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnGetTypeInfo(IVideoWindow* iface,UINT iTypeInfo, LCID lcid, ITypeInfo** ppobj)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnGetIDsOfNames(IVideoWindow* iface,REFIID riid, LPOLESTR* ppwszName, UINT cNames, LCID lcid, DISPID* pDispId)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnInvoke(IVideoWindow* iface,DISPID DispId, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarRes, EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}



static HRESULT WINAPI
IVideoWindow_fnput_Caption(IVideoWindow* iface,BSTR strCaption)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnget_Caption(IVideoWindow* iface,BSTR* pstrCaption)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnput_WindowStyle(IVideoWindow* iface,long lStyle)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnget_WindowStyle(IVideoWindow* iface,long* plStyle)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnput_WindowStyleEx(IVideoWindow* iface,long lExStyle)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnget_WindowStyleEx(IVideoWindow* iface,long* plExStyle)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnput_AutoShow(IVideoWindow* iface,long lAutoShow)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnget_AutoShow(IVideoWindow* iface,long* plAutoShow)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnput_WindowState(IVideoWindow* iface,long lState)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnget_WindowState(IVideoWindow* iface,long* plState)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnput_BackgroundPalette(IVideoWindow* iface,long lBackPal)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnget_BackgroundPalette(IVideoWindow* iface,long* plBackPal)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnput_Visible(IVideoWindow* iface,long lVisible)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnget_Visible(IVideoWindow* iface,long* plVisible)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnput_Left(IVideoWindow* iface,long lLeft)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnget_Left(IVideoWindow* iface,long* plLeft)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnput_Width(IVideoWindow* iface,long lWidth)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnget_Width(IVideoWindow* iface,long* plWidth)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnput_Top(IVideoWindow* iface,long lTop)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnget_Top(IVideoWindow* iface,long* plTop)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnput_Height(IVideoWindow* iface,long lHeight)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnget_Height(IVideoWindow* iface,long* plHeight)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnput_Owner(IVideoWindow* iface,OAHWND hwnd)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnget_Owner(IVideoWindow* iface,OAHWND* phwnd)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnput_MessageDrain(IVideoWindow* iface,OAHWND hwnd)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnget_MessageDrain(IVideoWindow* iface,OAHWND* phwnd)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnget_BorderColor(IVideoWindow* iface,long* plColor)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnput_BorderColor(IVideoWindow* iface,long lColor)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnget_FullScreenMode(IVideoWindow* iface,long* plMode)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnput_FullScreenMode(IVideoWindow* iface,long lMode)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnSetWindowForeground(IVideoWindow* iface,long lFocus)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnNotifyOwnerMessage(IVideoWindow* iface,OAHWND hwnd,long message,LONG_PTR wParam,LONG_PTR lParam)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnSetWindowPosition(IVideoWindow* iface,long lLeft,long lTop,long lWidth,long lHeight)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnGetWindowPosition(IVideoWindow* iface,long* plLeft,long* plTop,long* plWidth,long* plHeight)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnGetMinIdealImageSize(IVideoWindow* iface,long* plWidth,long* plHeight)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnGetMaxIdealImageSize(IVideoWindow* iface,long* plWidth,long* plHeight)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnGetRestorePosition(IVideoWindow* iface,long* plLeft,long* plTop,long* plWidth,long* plHeight)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnHideCursor(IVideoWindow* iface,long lHide)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnIsCursorHidden(IVideoWindow* iface,long* plHide)
{
	CVideoRendererImpl_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}




static ICOM_VTABLE(IVideoWindow) ivideowindow =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IVideoWindow_fnQueryInterface,
	IVideoWindow_fnAddRef,
	IVideoWindow_fnRelease,
	/* IDispatch fields */
	IVideoWindow_fnGetTypeInfoCount,
	IVideoWindow_fnGetTypeInfo,
	IVideoWindow_fnGetIDsOfNames,
	IVideoWindow_fnInvoke,
	/* IVideoWindow fields */
	IVideoWindow_fnput_Caption,
	IVideoWindow_fnget_Caption,
	IVideoWindow_fnput_WindowStyle,
	IVideoWindow_fnget_WindowStyle,
	IVideoWindow_fnput_WindowStyleEx,
	IVideoWindow_fnget_WindowStyleEx,
	IVideoWindow_fnput_AutoShow,
	IVideoWindow_fnget_AutoShow,
	IVideoWindow_fnput_WindowState,
	IVideoWindow_fnget_WindowState,
	IVideoWindow_fnput_BackgroundPalette,
	IVideoWindow_fnget_BackgroundPalette,
	IVideoWindow_fnput_Visible,
	IVideoWindow_fnget_Visible,
	IVideoWindow_fnput_Left,
	IVideoWindow_fnget_Left,
	IVideoWindow_fnput_Width,
	IVideoWindow_fnget_Width,
	IVideoWindow_fnput_Top,
	IVideoWindow_fnget_Top,
	IVideoWindow_fnput_Height,
	IVideoWindow_fnget_Height,
	IVideoWindow_fnput_Owner,
	IVideoWindow_fnget_Owner,
	IVideoWindow_fnput_MessageDrain,
	IVideoWindow_fnget_MessageDrain,
	IVideoWindow_fnget_BorderColor,
	IVideoWindow_fnput_BorderColor,
	IVideoWindow_fnget_FullScreenMode,
	IVideoWindow_fnput_FullScreenMode,
	IVideoWindow_fnSetWindowForeground,
	IVideoWindow_fnNotifyOwnerMessage,
	IVideoWindow_fnSetWindowPosition,
	IVideoWindow_fnGetWindowPosition,
	IVideoWindow_fnGetMinIdealImageSize,
	IVideoWindow_fnGetMaxIdealImageSize,
	IVideoWindow_fnGetRestorePosition,
	IVideoWindow_fnHideCursor,
	IVideoWindow_fnIsCursorHidden,

};


HRESULT CVideoRendererImpl_InitIVideoWindow( CVideoRendererImpl* This )
{
	TRACE("(%p)\n",This);
	ICOM_VTBL(&This->vidwin) = &ivideowindow;

	return NOERROR;
}

void CVideoRendererImpl_UninitIVideoWindow( CVideoRendererImpl* This )
{
	TRACE("(%p)\n",This);
}

