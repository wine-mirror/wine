#include "config.h"

#include "windef.h"
#include "winerror.h"
#include "ole2.h"
#include "guiddef.h"
/*#include "dshow.h"*/ /* not yet */

#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(quartz);

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
} IClassFactoryImpl;

static IClassFactoryImpl QUARTZ_GlobalCF = {&iclassfact, 0 };

static DWORD dwClassObjRef;


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
	if ( (This->ref) == 0 )
		dwClassObjRef ++;

	return ++(This->ref);
}

static ULONG WINAPI IClassFactory_fnRelease(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	TRACE("(%p)->()\n",This);
	if ( (--(This->ref)) > 0 )
		return This->ref;

	dwClassObjRef --;
	return 0;
}

static HRESULT WINAPI IClassFactory_fnCreateInstance(LPCLASSFACTORY iface,LPUNKNOWN pOuter,REFIID riid,LPVOID *ppobj)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	FIXME("(%p)->(%p,%s,%p),stub!\n",This,pOuter,debugstr_guid(riid),ppobj);

	*ppobj = NULL;
	if ( pOuter != NULL )
		return E_FAIL;

	return E_NOINTERFACE;
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




/***********************************************************************
 *		QUARTZ_InitProcess (internal)
 */
static BOOL QUARTZ_InitProcess( void )
{
	TRACE("()\n");

	dwClassObjRef = 0;

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
DWORD WINAPI QUARTZ_DllCanUnloadNow(void)
{
	return ( dwClassObjRef == 0 ) ? S_OK : S_FALSE;
}

/***********************************************************************
 *		DllGetClassObject (QUARTZ.@)
 */
DWORD WINAPI QUARTZ_DllGetClassObject(
		const CLSID* pclsid,const IID* piid,void** ppv)
{
	*ppv = NULL;
	if ( IsEqualCLSID( &IID_IClassFactory, piid ) )
	{
		*ppv = (LPVOID)&QUARTZ_GlobalCF;
		IClassFactory_AddRef((IClassFactory*)*ppv);
		return S_OK;
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

