/*
 * Exported APIs.
 *
 * hidenori@a2.ctktv.ne.jp
 */

#include "config.h"

#include "windef.h"
#include "winerror.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "mmsystem.h"
#include "ole2.h"
#include "wine/obj_oleaut.h"
#include "strmif.h"
#include "control.h"
#include "uuids.h"
#include "errors.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "fgraph.h"
#include "sysclock.h"
#include "memalloc.h"
#include "devenum.h"
#include "fmap.h"
#include "fmap2.h"
#include "seekpass.h"
#include "audren.h"


typedef struct QUARTZ_CLASSENTRY
{
	const CLSID*	pclsid;
	QUARTZ_pCreateIUnknown	pCreateIUnk;
} QUARTZ_CLASSENTRY;


static HRESULT WINAPI
IClassFactory_fnQueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj);
static ULONG WINAPI IClassFactory_fnAddRef(LPCLASSFACTORY iface);
static ULONG WINAPI IClassFactory_fnRelease(LPCLASSFACTORY iface);
static HRESULT WINAPI IClassFactory_fnCreateInstance(LPCLASSFACTORY iface,LPUNKNOWN pOuter,REFIID riid,LPVOID *ppobj);
static HRESULT WINAPI IClassFactory_fnLockServer(LPCLASSFACTORY iface,BOOL dolock);

static ICOM_VTABLE(IClassFactory) iclassfact =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IClassFactory_fnQueryInterface,
	IClassFactory_fnAddRef,
	IClassFactory_fnRelease,
	IClassFactory_fnCreateInstance,
	IClassFactory_fnLockServer
};

typedef struct
{
	/* IUnknown fields */
	ICOM_VFIELD(IClassFactory);
	LONG	ref;
	/* IClassFactory fields */
	const QUARTZ_CLASSENTRY* pEntry;
} IClassFactoryImpl;

static const QUARTZ_CLASSENTRY QUARTZ_ClassList[] =
{
	{ &CLSID_FilterGraph, &QUARTZ_CreateFilterGraph },
	{ &CLSID_SystemClock, &QUARTZ_CreateSystemClock },
	{ &CLSID_MemoryAllocator, &QUARTZ_CreateMemoryAllocator },
	{ &CLSID_SystemDeviceEnum, &QUARTZ_CreateSystemDeviceEnum },
	{ &CLSID_FilterMapper, &QUARTZ_CreateFilterMapper },
	{ &CLSID_FilterMapper2, &QUARTZ_CreateFilterMapper2 },
	{ &CLSID_SeekingPassThru, &QUARTZ_CreateSeekingPassThru },
	{ &CLSID_AudioRender, &QUARTZ_CreateAudioRenderer },
	{ NULL, NULL },
};

/* per-process variables */
static CRITICAL_SECTION csHeap;
static DWORD dwClassObjRef;
static HANDLE hDLLHeap;

void* QUARTZ_AllocObj( DWORD dwSize )
{
	void*	pv;

	EnterCriticalSection( &csHeap );
	dwClassObjRef ++;
	pv = HeapAlloc( hDLLHeap, 0, dwSize );
	if ( pv == NULL )
		dwClassObjRef --;
	LeaveCriticalSection( &csHeap );

	return pv;
}

void QUARTZ_FreeObj( void* pobj )
{
	EnterCriticalSection( &csHeap );
	HeapFree( hDLLHeap, 0, pobj );
	dwClassObjRef --;
	LeaveCriticalSection( &csHeap );
}

void* QUARTZ_AllocMem( DWORD dwSize )
{
	return HeapAlloc( hDLLHeap, 0, dwSize );
}

void QUARTZ_FreeMem( void* pMem )
{
	HeapFree( hDLLHeap, 0, pMem );
}

void* QUARTZ_ReallocMem( void* pMem, DWORD dwSize )
{
	if ( pMem == NULL )
		return QUARTZ_AllocMem( dwSize );

	return HeapReAlloc( hDLLHeap, 0, pMem, dwSize );
}

static
LPWSTR QUARTZ_strncpyAtoW( LPWSTR lpwstr, LPCSTR lpstr, INT wbuflen )
{
	INT	len;

	len = MultiByteToWideChar( CP_ACP, 0, lpstr, -1, lpwstr, wbuflen );
	if ( len == 0 )
		*lpwstr = 0;
	return lpwstr;
}


/************************************************************************/

static HRESULT WINAPI
IClassFactory_fnQueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	TRACE("(%p)->(%p,%p)\n",This,riid,ppobj);
	if ( ( IsEqualGUID( &IID_IUnknown, riid ) ) ||
	     ( IsEqualGUID( &IID_IClassFactory, riid ) ) )
	{
		*ppobj = iface;
		IClassFactory_AddRef(iface);
		return S_OK;
	}

	return E_NOINTERFACE;
}

static ULONG WINAPI IClassFactory_fnAddRef(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	TRACE("(%p)->()\n",This);

	return InterlockedExchangeAdd(&(This->ref),1) + 1;
}

static ULONG WINAPI IClassFactory_fnRelease(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	LONG	ref;

	TRACE("(%p)->()\n",This);
	ref = InterlockedExchangeAdd(&(This->ref),-1) - 1;
	if ( ref > 0 )
		return (ULONG)ref;

	QUARTZ_FreeObj(This);
	return 0;
}

static HRESULT WINAPI IClassFactory_fnCreateInstance(LPCLASSFACTORY iface,LPUNKNOWN pOuter,REFIID riid,LPVOID *ppobj)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	HRESULT	hr;
	IUnknown*	punk;

	TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);

	if ( ppobj == NULL )
		return E_POINTER;
	if ( pOuter != NULL && !IsEqualGUID( riid, &IID_IUnknown ) )
		return CLASS_E_NOAGGREGATION;

	*ppobj = NULL;

	hr = (*This->pEntry->pCreateIUnk)(pOuter,(void**)&punk);
	if ( hr != S_OK )
		return hr;

	hr = IUnknown_QueryInterface(punk,riid,ppobj);
	IUnknown_Release(punk);

	return hr;
}

static HRESULT WINAPI IClassFactory_fnLockServer(LPCLASSFACTORY iface,BOOL dolock)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	HRESULT	hr;

	TRACE("(%p)->(%d)\n",This,dolock);
	if (dolock)
		hr = IClassFactory_AddRef(iface);
	else
		hr = IClassFactory_Release(iface);

	return hr;
}



static HRESULT IClassFactory_Alloc( const CLSID* pclsid, void** ppobj )
{
	const QUARTZ_CLASSENTRY*	pEntry;
	IClassFactoryImpl*	pImpl;

	TRACE( "(%s,%p)\n", debugstr_guid(pclsid), ppobj );

	pEntry = QUARTZ_ClassList;
	while ( pEntry->pclsid != NULL )
	{
		if ( IsEqualGUID( pclsid, pEntry->pclsid ) )
			goto found;
		pEntry ++;
	}

	return CLASS_E_CLASSNOTAVAILABLE;
found:
	pImpl = (IClassFactoryImpl*)QUARTZ_AllocObj( sizeof(IClassFactoryImpl) );
	if ( pImpl == NULL )
		return E_OUTOFMEMORY;

	TRACE( "allocated successfully.\n" );

	ICOM_VTBL(pImpl) = &iclassfact;
	pImpl->ref = 1;
	pImpl->pEntry = pEntry;

	*ppobj = (void*)pImpl;
	return S_OK;
}


/***********************************************************************
 *		QUARTZ_InitProcess (internal)
 */
static BOOL QUARTZ_InitProcess( void )
{
	TRACE("()\n");

	dwClassObjRef = 0;
	hDLLHeap = (HANDLE)NULL;
	InitializeCriticalSection( &csHeap );

	hDLLHeap = HeapCreate( 0, 0x10000, 0 );
	if ( hDLLHeap == (HANDLE)NULL )
		return FALSE;

	return TRUE;
}

/***********************************************************************
 *		QUARTZ_UninitProcess (internal)
 */
static void QUARTZ_UninitProcess( void )
{
	TRACE("()\n");

	if ( dwClassObjRef != 0 )
		ERR( "you must release some objects allocated from quartz.\n" );
	if ( hDLLHeap != (HANDLE)NULL )
	{
		HeapDestroy( hDLLHeap );
		hDLLHeap = (HANDLE)NULL;
	}
	DeleteCriticalSection( &csHeap );
}

/***********************************************************************
 *		QUARTZ_DllMain
 */
BOOL WINAPI QUARTZ_DllMain(
	HINSTANCE hInstDLL,
	DWORD fdwReason,
	LPVOID lpvReserved )
{
	TRACE("(%08x,%08lx,%p)\n",hInstDLL,fdwReason,lpvReserved);

	switch ( fdwReason )
	{
	case DLL_PROCESS_ATTACH:
		if ( !QUARTZ_InitProcess() )
			return FALSE;
		break;
	case DLL_PROCESS_DETACH:
		QUARTZ_UninitProcess();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	}

	return TRUE;
}


/***********************************************************************
 *		DllCanUnloadNow (QUARTZ.@)
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: S_FALSE
 */
HRESULT WINAPI QUARTZ_DllCanUnloadNow(void)
{
	HRESULT	hr;

	EnterCriticalSection( &csHeap );
	hr = ( dwClassObjRef == 0 ) ? S_OK : S_FALSE;
	LeaveCriticalSection( &csHeap );

	return hr;
}

/***********************************************************************
 *		DllGetClassObject (QUARTZ.@)
 */
HRESULT WINAPI QUARTZ_DllGetClassObject(
		const CLSID* pclsid,const IID* piid,void** ppv)
{
	*ppv = NULL;
	if ( IsEqualCLSID( &IID_IUnknown, piid ) ||
	     IsEqualCLSID( &IID_IClassFactory, piid ) )
	{
		return IClassFactory_Alloc( pclsid, ppv );
	}

	return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *		DllRegisterServer (QUARTZ.@)
 */

HRESULT WINAPI QUARTZ_DllRegisterServer( void )
{
	FIXME( "(): stub\n" );
	return E_FAIL;
}

/***********************************************************************
 *		DllUnregisterServer (QUARTZ.@)
 */

HRESULT WINAPI QUARTZ_DllUnregisterServer( void )
{
	FIXME( "(): stub\n" );
	return E_FAIL;
}

/**************************************************************************/
/**************************************************************************/

/* FIXME - all string should be defined in the resource of quartz. */

static LPCSTR hresult_to_string( HRESULT hr )
{
	switch ( hr )
	{
	#define	ENTRY(x)	case x: return (const char*)#x
	/* some known codes */
	ENTRY(S_OK);
	ENTRY(S_FALSE);
	ENTRY(E_FAIL);
	ENTRY(E_POINTER);
	ENTRY(E_NOTIMPL);
	ENTRY(E_NOINTERFACE);
	ENTRY(E_OUTOFMEMORY);
	ENTRY(CLASS_E_CLASSNOTAVAILABLE);
	ENTRY(CLASS_E_NOAGGREGATION);

	/* vfwmsgs.h */
	ENTRY(VFW_S_NO_MORE_ITEMS);
	ENTRY(VFW_E_BAD_KEY);
	ENTRY(VFW_E_INVALIDMEDIATYPE);
	ENTRY(VFW_E_INVALIDSUBTYPE);
	ENTRY(VFW_E_NEED_OWNER);
	ENTRY(VFW_E_ENUM_OUT_OF_SYNC);
	ENTRY(VFW_E_ALREADY_CONNECTED);
	ENTRY(VFW_E_FILTER_ACTIVE);
	ENTRY(VFW_E_NO_TYPES);
	ENTRY(VFW_E_NO_ACCEPTABLE_TYPES);
	ENTRY(VFW_E_INVALID_DIRECTION);
	ENTRY(VFW_E_NOT_CONNECTED);
	ENTRY(VFW_E_NO_ALLOCATOR);
	ENTRY(VFW_E_RUNTIME_ERROR);
	ENTRY(VFW_E_BUFFER_NOTSET);
	ENTRY(VFW_E_BUFFER_OVERFLOW);
	ENTRY(VFW_E_BADALIGN);
	ENTRY(VFW_E_ALREADY_COMMITTED);
	ENTRY(VFW_E_BUFFERS_OUTSTANDING);
	ENTRY(VFW_E_NOT_COMMITTED);
	ENTRY(VFW_E_SIZENOTSET);
	ENTRY(VFW_E_NO_CLOCK);
	ENTRY(VFW_E_NO_SINK);
	ENTRY(VFW_E_NO_INTERFACE);
	ENTRY(VFW_E_NOT_FOUND);
	ENTRY(VFW_E_CANNOT_CONNECT);
	ENTRY(VFW_E_CANNOT_RENDER);
	ENTRY(VFW_E_CHANGING_FORMAT);
	ENTRY(VFW_E_NO_COLOR_KEY_SET);
	ENTRY(VFW_E_NOT_OVERLAY_CONNECTION);
	ENTRY(VFW_E_NOT_SAMPLE_CONNECTION);
	ENTRY(VFW_E_PALETTE_SET);
	ENTRY(VFW_E_COLOR_KEY_SET);
	ENTRY(VFW_E_NO_COLOR_KEY_FOUND);
	ENTRY(VFW_E_NO_PALETTE_AVAILABLE);
	ENTRY(VFW_E_NO_DISPLAY_PALETTE);
	ENTRY(VFW_E_TOO_MANY_COLORS);
	ENTRY(VFW_E_STATE_CHANGED);
	ENTRY(VFW_E_NOT_STOPPED);
	ENTRY(VFW_E_NOT_PAUSED);
	ENTRY(VFW_E_NOT_RUNNING);
	ENTRY(VFW_E_WRONG_STATE);
	ENTRY(VFW_E_START_TIME_AFTER_END);
	ENTRY(VFW_E_INVALID_RECT);
	ENTRY(VFW_E_TYPE_NOT_ACCEPTED);
	ENTRY(VFW_E_SAMPLE_REJECTED);
	ENTRY(VFW_E_SAMPLE_REJECTED_EOS);
	ENTRY(VFW_S_DUPLICATE_NAME);
	ENTRY(VFW_E_DUPLICATE_NAME);
	ENTRY(VFW_E_TIMEOUT);
	ENTRY(VFW_E_INVALID_FILE_FORMAT);
	ENTRY(VFW_E_ENUM_OUT_OF_RANGE);
	ENTRY(VFW_E_CIRCULAR_GRAPH);
	ENTRY(VFW_E_NOT_ALLOWED_TO_SAVE);
	ENTRY(VFW_E_TIME_ALREADY_PASSED);
	ENTRY(VFW_E_ALREADY_CANCELLED);
	ENTRY(VFW_E_CORRUPT_GRAPH_FILE);
	ENTRY(VFW_E_ADVISE_ALREADY_SET);
	ENTRY(VFW_S_STATE_INTERMEDIATE);
	ENTRY(VFW_E_NO_MODEX_AVAILABLE);
	ENTRY(VFW_E_NO_ADVISE_SET);
	ENTRY(VFW_E_NO_FULLSCREEN);
	ENTRY(VFW_E_IN_FULLSCREEN_MODE);
	ENTRY(VFW_E_UNKNOWN_FILE_TYPE);
	ENTRY(VFW_E_CANNOT_LOAD_SOURCE_FILTER);
	ENTRY(VFW_S_PARTIAL_RENDER);
	ENTRY(VFW_E_FILE_TOO_SHORT);
	ENTRY(VFW_E_INVALID_FILE_VERSION);
	ENTRY(VFW_S_SOME_DATA_IGNORED);
	ENTRY(VFW_S_CONNECTIONS_DEFERRED);
	ENTRY(VFW_E_INVALID_CLSID);
	ENTRY(VFW_E_INVALID_MEDIA_TYPE);
	ENTRY(VFW_E_SAMPLE_TIME_NOT_SET);
	ENTRY(VFW_S_RESOURCE_NOT_NEEDED);
	ENTRY(VFW_E_MEDIA_TIME_NOT_SET);
	ENTRY(VFW_E_NO_TIME_FORMAT_SET);
	ENTRY(VFW_E_MONO_AUDIO_HW);
	ENTRY(VFW_S_MEDIA_TYPE_IGNORED);
	ENTRY(VFW_E_NO_DECOMPRESSOR);
	ENTRY(VFW_E_NO_AUDIO_HARDWARE);
	ENTRY(VFW_S_VIDEO_NOT_RENDERED);
	ENTRY(VFW_S_AUDIO_NOT_RENDERED);
	ENTRY(VFW_E_RPZA);
	ENTRY(VFW_S_RPZA);
	ENTRY(VFW_E_PROCESSOR_NOT_SUITABLE);
	ENTRY(VFW_E_UNSUPPORTED_AUDIO);
	ENTRY(VFW_E_UNSUPPORTED_VIDEO);
	ENTRY(VFW_E_MPEG_NOT_CONSTRAINED);
	ENTRY(VFW_E_NOT_IN_GRAPH);
	ENTRY(VFW_S_ESTIMATED);
	ENTRY(VFW_E_NO_TIME_FORMAT);
	ENTRY(VFW_E_READ_ONLY);
	ENTRY(VFW_S_RESERVED);
	ENTRY(VFW_E_BUFFER_UNDERFLOW);
	ENTRY(VFW_E_UNSUPPORTED_STREAM);
	ENTRY(VFW_E_NO_TRANSPORT);
	ENTRY(VFW_S_STREAM_OFF);
	ENTRY(VFW_S_CANT_CUE);
	ENTRY(VFW_E_BAD_VIDEOCD);
	ENTRY(VFW_S_NO_STOP_TIME);
	ENTRY(VFW_E_OUT_OF_VIDEO_MEMORY);
	ENTRY(VFW_E_VP_NEGOTIATION_FAILED);
	ENTRY(VFW_E_DDRAW_CAPS_NOT_SUITABLE);
	ENTRY(VFW_E_NO_VP_HARDWARE);
	ENTRY(VFW_E_NO_CAPTURE_HARDWARE);
	ENTRY(VFW_E_DVD_OPERATION_INHIBITED);
	ENTRY(VFW_E_DVD_INVALIDDOMAIN);
	ENTRY(VFW_E_DVD_NO_BUTTON);
	ENTRY(VFW_E_DVD_GRAPHNOTREADY);
	ENTRY(VFW_E_DVD_RENDERFAIL);
	ENTRY(VFW_E_DVD_DECNOTENOUGH);
	ENTRY(VFW_E_DDRAW_VERSION_NOT_SUITABLE);
	ENTRY(VFW_E_COPYPROT_FAILED);
	ENTRY(VFW_S_NOPREVIEWPIN);
	ENTRY(VFW_E_TIME_EXPIRED);
	ENTRY(VFW_S_DVD_NON_ONE_SEQUENTIAL);
	ENTRY(VFW_E_DVD_WRONG_SPEED);
	ENTRY(VFW_E_DVD_MENU_DOES_NOT_EXIST);
	ENTRY(VFW_E_DVD_CMD_CANCELLED);
	ENTRY(VFW_E_DVD_STATE_WRONG_VERSION);
	ENTRY(VFW_E_DVD_STATE_CORRUPT);
	ENTRY(VFW_E_DVD_STATE_WRONG_DISC);
	ENTRY(VFW_E_DVD_INCOMPATIBLE_REGION);
	ENTRY(VFW_E_DVD_NO_ATTRIBUTES);
	ENTRY(VFW_E_DVD_NO_GOUP_PGC);
	ENTRY(VFW_E_DVD_LOW_PARENTAL_LEVEL);
	ENTRY(VFW_E_DVD_NOT_IN_KARAOKE_MODE);
	ENTRY(VFW_S_DVD_CHANNEL_CONTENTS_NOT_AVAILABLE);
	ENTRY(VFW_S_DVD_NOT_ACCURATE);
	ENTRY(VFW_E_FRAME_STEP_UNSUPPORTED);
	ENTRY(VFW_E_DVD_STREAM_DISABLED);
	ENTRY(VFW_E_DVD_TITLE_UNKNOWN);
	ENTRY(VFW_E_DVD_INVALID_DISC);
	ENTRY(VFW_E_DVD_NO_RESUME_INFORMATION);
	ENTRY(VFW_E_PIN_ALREADY_BLOCKED_ON_THIS_THREAD);
	ENTRY(VFW_E_PIN_ALREADY_BLOCKED);
	ENTRY(VFW_E_CERTIFICATION_FAILURE);
	#undef	ENTRY
	}

	return NULL;
}

/***********************************************************************
 *	AMGetErrorTextA	(quartz.@)
 */
DWORD WINAPI AMGetErrorTextA(HRESULT hr, LPSTR pszbuf, DWORD dwBufLen)
{
	LPCSTR	lpszRes;
	DWORD len;

	lpszRes = hresult_to_string( hr );
	if ( lpszRes == NULL )
		return 0;
	len = (DWORD)(strlen(lpszRes)+1);
	if ( len > dwBufLen )
		return 0;

	memcpy( pszbuf, lpszRes, len );
	return len;
}

/***********************************************************************
 *	AMGetErrorTextW	(quartz.@)
 */
DWORD WINAPI AMGetErrorTextW(HRESULT hr, LPWSTR pwszbuf, DWORD dwBufLen)
{
	CHAR	szBuf[MAX_ERROR_TEXT_LEN+1];
	DWORD	dwLen;

	dwLen = AMGetErrorTextA(hr,szBuf,MAX_ERROR_TEXT_LEN);
	if ( dwLen == 0 )
		return 0;
	szBuf[dwLen] = 0;

	QUARTZ_strncpyAtoW( pwszbuf, szBuf, dwBufLen );

	return lstrlenW( pwszbuf );
}
