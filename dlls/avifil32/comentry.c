/*
 * Copyright 2001 Hidenori TAKESHIMA <hidenori@a2.ctktv.ne.jp>
 */

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "winbase.h"
#include "winnls.h"
#include "mmsystem.h"
#include "winerror.h"
#include "ole2.h"
#include "vfw.h"
#include "debugtools.h"
#include "avifile_private.h"

DEFAULT_DEBUG_CHANNEL(avifile);

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

static IClassFactoryImpl AVIFILE_GlobalCF = {&iclassfact, 0 };



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
		AVIFILE_data.dwClassObjRef ++;

	return ++(This->ref);
}

static ULONG WINAPI IClassFactory_fnRelease(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);

	TRACE("(%p)->()\n",This);
	if ( (--(This->ref)) > 0 )
		return This->ref;

	AVIFILE_data.dwClassObjRef --;
	return 0;
}

static HRESULT WINAPI IClassFactory_fnCreateInstance(LPCLASSFACTORY iface,LPUNKNOWN pOuter,REFIID riid,LPVOID *ppobj)
{
    /*ICOM_THIS(IClassFactoryImpl,iface);*/

	*ppobj = NULL;
	if ( pOuter != NULL )
		return E_FAIL;

	if ( IsEqualGUID( &IID_IAVIFile, riid ) )
		return AVIFILE_CreateIAVIFile(ppobj);
	if ( IsEqualGUID( &IID_IAVIStream, riid ) )
		return AVIFILE_CreateIAVIStream(ppobj);

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
 *		DllGetClassObject (AVIFIL32.@)
 */
HRESULT WINAPI AVIFILE_DllGetClassObject(const CLSID* pclsid,const IID* piid,void** ppv)
{
	*ppv = NULL;
	if ( IsEqualCLSID( &IID_IClassFactory, piid ) )
	{
		*ppv = (LPVOID)&AVIFILE_GlobalCF;
		IClassFactory_AddRef((IClassFactory*)*ppv);
		return S_OK;
	}

	return CLASS_E_CLASSNOTAVAILABLE;
}

/*****************************************************************************
 *		DllCanUnloadNow (AVIFIL32.@)
 */
DWORD WINAPI AVIFILE_DllCanUnloadNow(void)
{
	return ( AVIFILE_data.dwClassObjRef == 0 ) ? S_OK : S_FALSE;
}

