/*
 * A basic implementation for COM DLL implementor.
 *
 * Copyright (C) 2002 Hidenori TAKESHIMA <hidenori@a2.ctktv.ne.jp>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include "windef.h"
#include "winerror.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "ole2.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(comimpl);

#include "comimpl.h"

/*
  - All threading model including Apartment and Both are supported.
  - Aggregation is supported.
  - CoFreeUnusedLibraries() is supported.
  - DisableThreadLibraryCalls() is supported.
 */

static CRITICAL_SECTION csComImpl;
static DWORD dwComImplRef;


static HRESULT WINAPI
IUnknown_fnQueryInterface(IUnknown* iface,REFIID riid,LPVOID *ppobj)
{
	ICOM_THIS(COMIMPL_IUnkImpl,iface);
	size_t	ofs;
	DWORD	dwIndex;
	COMIMPL_IFDelegation*	pDelegation;
	HRESULT	hr;

	TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);

	if ( ppobj == NULL )
		return E_POINTER;
	*ppobj = NULL;

	ofs = 0;

	if ( IsEqualGUID( &IID_IUnknown, riid ) )
	{
		TRACE("IID_IUnknown - returns inner object.\n");
	}
	else
	{
		for ( dwIndex = 0; dwIndex < This->dwEntries; dwIndex++ )
		{
			if ( IsEqualGUID( This->pEntries[dwIndex].piid, riid ) )
			{
				ofs = This->pEntries[dwIndex].ofsVTPtr;
				break;
			}
		}
		if ( dwIndex == This->dwEntries )
		{
			hr = E_NOINTERFACE;

			/* delegation */
			pDelegation = This->pDelegationFirst;
			while ( pDelegation != NULL )
			{
				hr = (*pDelegation->pOnQueryInterface)( iface, riid, ppobj );
				if ( hr != E_NOINTERFACE )
					break;
				pDelegation = pDelegation->pNext;
			}

			if ( hr == E_NOINTERFACE )
			{
				FIXME("(%p) unknown interface: %s\n",This,debugstr_guid(riid));
			}

			return hr;
		}
	}

	*ppobj = (LPVOID)(((char*)This) + ofs);
	IUnknown_AddRef((IUnknown*)(*ppobj));

	return S_OK;
}

static ULONG WINAPI
IUnknown_fnAddRef(IUnknown* iface)
{
	ICOM_THIS(COMIMPL_IUnkImpl,iface);

	TRACE("(%p)->()\n",This);

	return InterlockedExchangeAdd(&(This->ref),1) + 1;
}

static ULONG WINAPI
IUnknown_fnRelease(IUnknown* iface)
{
	ICOM_THIS(COMIMPL_IUnkImpl,iface);
	LONG	ref;

	TRACE("(%p)->()\n",This);
	ref = InterlockedExchangeAdd(&(This->ref),-1) - 1;
	if ( ref > 0 )
		return (ULONG)ref;

	This->ref ++;
	if ( This->pOnFinalRelease != NULL )
		(*(This->pOnFinalRelease))(iface);
	This->ref --;

	COMIMPL_FreeObj(This);

	return 0;
}


static ICOM_VTABLE(IUnknown) iunknown =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IUnknown_fnQueryInterface,
	IUnknown_fnAddRef,
	IUnknown_fnRelease,
};


void COMIMPL_IUnkInit( COMIMPL_IUnkImpl* pImpl, IUnknown* punkOuter )
{
	TRACE("(%p)\n",pImpl);

	ICOM_VTBL(pImpl) = &iunknown;
	pImpl->pEntries = NULL;
	pImpl->dwEntries = 0;
	pImpl->pDelegationFirst = NULL;
	pImpl->pOnFinalRelease = NULL;
	pImpl->ref = 1;
	pImpl->punkControl = (IUnknown*)pImpl;

	/* for implementing aggregation. */
	if ( punkOuter != NULL )
		pImpl->punkControl = punkOuter;
}

void COMIMPL_IUnkAddDelegationHandler(
	COMIMPL_IUnkImpl* pImpl, COMIMPL_IFDelegation* pDelegation )
{
	pDelegation->pNext = pImpl->pDelegationFirst;
	pImpl->pDelegationFirst = pDelegation;
}



/************************************************************************/


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
	const COMIMPL_CLASSENTRY* pEntry;
} IClassFactoryImpl;


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

	COMIMPL_FreeObj(This);
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
	const COMIMPL_CLASSENTRY*	pEntry;
	IClassFactoryImpl*	pImpl;

	TRACE( "(%s,%p)\n", debugstr_guid(pclsid), ppobj );

	pEntry = COMIMPL_ClassList;
	while ( pEntry->pclsid != NULL )
	{
		if ( IsEqualGUID( pclsid, pEntry->pclsid ) )
			goto found;
		pEntry ++;
	}

	return CLASS_E_CLASSNOTAVAILABLE;
found:
	pImpl = (IClassFactoryImpl*)COMIMPL_AllocObj( sizeof(IClassFactoryImpl) );
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
 *		COMIMPL_InitProcess (internal)
 */
static BOOL COMIMPL_InitProcess( HINSTANCE hInstDLL )
{
	TRACE("()\n");

	dwComImplRef = 0;
	InitializeCriticalSection( &csComImpl );

#ifndef COMIMPL_PERTHREAD_INIT
	DisableThreadLibraryCalls((HMODULE)hInstDLL);
#endif	/* COMIMPL_PERTHREAD_INIT */

	return TRUE;
}

/***********************************************************************
 *		COMIMPL_UninitProcess (internal)
 */
static void COMIMPL_UninitProcess( HINSTANCE hInstDLL )
{
	CHAR	szThisDLL[ MAX_PATH ];

	TRACE("()\n");

	if ( dwComImplRef != 0 )
	{
		szThisDLL[0] = '\0';
		if ( !GetModuleFileNameA( (HMODULE)hInstDLL, szThisDLL, MAX_PATH ) )
			szThisDLL[0] = '\0';
		ERR( "you must release some objects allocated from %s.\n", szThisDLL );
	}
	DeleteCriticalSection( &csComImpl );
}


/***********************************************************************
 *		COMIMPL_DllMain
 */
BOOL WINAPI COMIMPL_DllMain(
	HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpvReserved )
{
	TRACE("(%08x,%08lx,%p)\n",hInstDLL,fdwReason,lpvReserved);

	switch ( fdwReason )
	{
	case DLL_PROCESS_ATTACH:
		if ( !COMIMPL_InitProcess( hInstDLL ) )
			return FALSE;
		break;
	case DLL_PROCESS_DETACH:
		COMIMPL_UninitProcess( hInstDLL );
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	}

	return TRUE;
}

/***********************************************************************
 *		COMIMPL_DllGetClassObject
 */
HRESULT WINAPI COMIMPL_DllGetClassObject(
		const CLSID* pclsid,const IID* piid,void** ppv)
{
	*ppv = NULL;
	if ( IsEqualGUID( &IID_IUnknown, piid ) ||
	     IsEqualGUID( &IID_IClassFactory, piid ) )
	{
		return IClassFactory_Alloc( pclsid, ppv );
	}

	return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *		COMIMPL_DllCanUnloadNow
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: S_FALSE
 */
HRESULT WINAPI COMIMPL_DllCanUnloadNow(void)
{
	HRESULT	hr;

	EnterCriticalSection( &csComImpl );
	hr = ( dwComImplRef == 0 ) ? S_OK : S_FALSE;
	LeaveCriticalSection( &csComImpl );

	return hr;
}

/***********************************************************************
 *		COMIMPL_AllocObj
 */
void* COMIMPL_AllocObj( DWORD dwSize )
{
	void*	pv;

	EnterCriticalSection( &csComImpl );
	dwComImplRef ++;
	pv = HeapAlloc( COMIMPL_hHeap, 0, dwSize );
	if ( pv == NULL )
		dwComImplRef --;
	LeaveCriticalSection( &csComImpl );

	return pv;
}

/***********************************************************************
 *		COMIMPL_FreeObj
 */
void COMIMPL_FreeObj( void* pobj )
{
	EnterCriticalSection( &csComImpl );
	HeapFree( COMIMPL_hHeap, 0, pobj );
	dwComImplRef --;
	LeaveCriticalSection( &csComImpl );
}

