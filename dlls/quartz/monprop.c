/*
 * Implements IPropertyBag. (internal)
 *
 * hidenori@a2.ctktv.ne.jp
 */

#include "config.h"

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "winerror.h"
#include "wine/obj_base.h"
#include "objidl.h"
#include "oleidl.h"
#include "ocidl.h"
#include "oleauto.h"
#include "strmif.h"
#include "wine/unicode.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "monprop.h"


static HRESULT WINAPI
IPropertyBag_fnQueryInterface(IPropertyBag* iface,REFIID riid,void** ppobj)
{
	CRegPropertyBag_THIS(iface,propbag);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IPropertyBag_fnAddRef(IPropertyBag* iface)
{
	CRegPropertyBag_THIS(iface,propbag);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IPropertyBag_fnRelease(IPropertyBag* iface)
{
	CRegPropertyBag_THIS(iface,propbag);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}

static HRESULT WINAPI
IPropertyBag_fnRead(IPropertyBag* iface,LPCOLESTR lpszPropName,VARIANT* pVar,IErrorLog* pLog)
{
	CRegPropertyBag_THIS(iface,propbag);
	LONG	lr;
	DWORD	dwSize;
	DWORD	dwValueType;

	TRACE("(%p)->(%s,%p,%p)\n",This,
		debugstr_w(lpszPropName),pVar,pLog);

	if ( lpszPropName == NULL || pVar == NULL )
		return E_POINTER;

	dwSize = 0;
	lr = RegQueryValueExW(
		This->m_hKey, lpszPropName, NULL,
		&dwValueType, NULL, &dwSize );
	if ( lr != ERROR_SUCCESS )
		return E_INVALIDARG;

	switch ( dwValueType )
	{
	case REG_SZ:
		if ( pVar->n1.n2.vt == VT_EMPTY )
			pVar->n1.n2.vt = VT_BSTR;
		if ( pVar->n1.n2.vt != VT_BSTR )
			return E_FAIL;

		pVar->n1.n2.n3.bstrVal = SysAllocStringByteLen(
			NULL, dwSize );
		if ( pVar->n1.n2.n3.bstrVal == NULL )
			return E_OUTOFMEMORY;
		lr = RegQueryValueExW(
			This->m_hKey, lpszPropName, NULL,
			&dwValueType,
			(BYTE*)pVar->n1.n2.n3.bstrVal, &dwSize );
		if ( lr != ERROR_SUCCESS )
		{
			SysFreeString(pVar->n1.n2.n3.bstrVal);
			return E_FAIL;
		}
		break;
	default:
		FIXME("(%p)->(%s,%p,%p) - unsupported value type.\n",This,
			debugstr_w(lpszPropName),pVar,pLog);
		return E_FAIL;
	}

	return NOERROR;
}

static HRESULT WINAPI
IPropertyBag_fnWrite(IPropertyBag* iface,LPCOLESTR lpszPropName,VARIANT* pVar)
{
	CRegPropertyBag_THIS(iface,propbag);

	FIXME("(%p)->(%s,%p) stub!\n",This,
		debugstr_w(lpszPropName),pVar);

	if ( lpszPropName == NULL || pVar == NULL )
		return E_POINTER;

	return E_NOTIMPL;
}




static ICOM_VTABLE(IPropertyBag) ipropbag =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IPropertyBag_fnQueryInterface,
	IPropertyBag_fnAddRef,
	IPropertyBag_fnRelease,
	/* IPropertyBag fields */
	IPropertyBag_fnRead,
	IPropertyBag_fnWrite,
};

static HRESULT CRegPropertyBag_InitIPropertyBag(
	CRegPropertyBag* prpb, HKEY hkRoot, LPCWSTR lpKeyPath )
{
	ICOM_VTBL(&prpb->propbag) = &ipropbag;

	if ( RegOpenKeyExW(
		hkRoot, lpKeyPath, 0,
		KEY_ALL_ACCESS, &prpb->m_hKey ) != ERROR_SUCCESS )
		return E_FAIL;

	return NOERROR;
}

static void CRegPropertyBag_UninitIPropertyBag(
	CRegPropertyBag* prpb )
{
	RegCloseKey( prpb->m_hKey );
}


static void QUARTZ_DestroyRegPropertyBag(IUnknown* punk)
{
	CRegPropertyBag_THIS(punk,unk);

	CRegPropertyBag_UninitIPropertyBag(This);
}


/* can I use offsetof safely? - FIXME? */
static QUARTZ_IFEntry IFEntries[] =
{
  { &IID_IPropertyBag, offsetof(CRegPropertyBag,propbag)-offsetof(CRegPropertyBag,unk) },
};

HRESULT QUARTZ_CreateRegPropertyBag(
	HKEY hkRoot, LPCWSTR lpKeyPath,
	IPropertyBag** ppPropBag )
{
	CRegPropertyBag*	prpb;
	HRESULT	hr;

	TRACE("(%08x,%s,%p)\n",hkRoot,debugstr_w(lpKeyPath),ppPropBag );

	prpb = (CRegPropertyBag*)QUARTZ_AllocObj( sizeof(CRegPropertyBag) );
	if ( prpb == NULL )
		return E_OUTOFMEMORY;

	QUARTZ_IUnkInit( &prpb->unk, NULL );
	hr = CRegPropertyBag_InitIPropertyBag( prpb, hkRoot, lpKeyPath );
	if ( FAILED(hr) )
	{
		QUARTZ_FreeObj( prpb );
		return hr;
	}

	prpb->unk.pEntries = IFEntries;
	prpb->unk.dwEntries = sizeof(IFEntries)/sizeof(IFEntries[0]);
	prpb->unk.pOnFinalRelease = &QUARTZ_DestroyRegPropertyBag;

	*ppPropBag = (IPropertyBag*)(&prpb->propbag);

	return S_OK;
}


