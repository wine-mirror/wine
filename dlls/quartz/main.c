#include "config.h"

#include "windef.h"
#include "winerror.h"
#include "winbase.h"
#include "wingdi.h"
#include "ole2.h"
#include "wine/obj_oleaut.h"
#include "strmif.h"
#include "control.h"
#include "uuids.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "fgraph.h"
#include "sysclock.h"
#include "memalloc.h"


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
	DWORD	ref;
	/* IClassFactory fields */
	const QUARTZ_CLASSENTRY* pEntry;
} IClassFactoryImpl;

static const QUARTZ_CLASSENTRY QUARTZ_ClassList[] =
{
	{ &CLSID_FilterGraph, &QUARTZ_CreateFilterGraph },
	{ &CLSID_SystemClock, &QUARTZ_CreateSystemClock },
	{ &CLSID_MemoryAllocator, &QUARTZ_CreateMemoryAllocator },
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

	return ++(This->ref);
}

static ULONG WINAPI IClassFactory_fnRelease(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	TRACE("(%p)->()\n",This);
	if ( (--(This->ref)) > 0 )
		return This->ref;

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

	FIXME("(%p)->(%d),stub!\n",This,dolock);
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
	}

	return CLASS_E_CLASSNOTAVAILABLE;
found:
	pImpl = (IClassFactoryImpl*)QUARTZ_AllocObj( sizeof(IClassFactoryImpl) );
	if ( pImpl == NULL )
		return E_OUTOFMEMORY;

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

