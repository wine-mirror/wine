/*
 * Implementation of IFilterMapper for CLSID_FilterMapper.
 *
 * FIXME - stub.
 *
 * hidenori@a2.ctktv.ne.jp
 */

#include "config.h"

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winreg.h"
#include "winerror.h"
#include "wine/obj_base.h"
#include "wine/obj_oleaut.h"
#include "strmif.h"
#include "control.h"
#include "uuids.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "fmap.h"
#include "regsvr.h"


static HRESULT WINAPI
IFilterMapper_fnQueryInterface(IFilterMapper* iface,REFIID riid,void** ppobj)
{
	CFilterMapper_THIS(iface,fmap);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IFilterMapper_fnAddRef(IFilterMapper* iface)
{
	CFilterMapper_THIS(iface,fmap);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IFilterMapper_fnRelease(IFilterMapper* iface)
{
	CFilterMapper_THIS(iface,fmap);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}


static HRESULT WINAPI
IFilterMapper_fnRegisterFilter(IFilterMapper* iface,CLSID clsid,LPCWSTR lpwszName,DWORD dwMerit)
{
	CFilterMapper_THIS(iface,fmap);

	FIXME("(%p)->(%s,%s,%08lx)\n",This,
		debugstr_guid(&clsid),debugstr_w(lpwszName),dwMerit);

	/* FIXME */
	/* FIXME - handle dwMerit! */
	return QUARTZ_RegisterAMovieFilter(
		&CLSID_LegacyAmFilterCategory,
		&clsid,
		NULL, 0,
		lpwszName, NULL, TRUE );
}

static HRESULT WINAPI
IFilterMapper_fnRegisterFilterInstance(IFilterMapper* iface,CLSID clsid,LPCWSTR lpwszName,CLSID* pclsidMedia)
{
	CFilterMapper_THIS(iface,fmap);
	HRESULT	hr;

	FIXME("(%p)->()\n",This);

	if ( pclsidMedia == NULL )
		return E_POINTER;
	hr = CoCreateGuid(pclsidMedia);
	if ( FAILED(hr) )
		return hr;

	/* FIXME */
	/* this doesn't work. */
	/* return IFilterMapper_RegisterFilter(iface,
		*pclsidMedia,lpwszName,0x60000000); */

	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterMapper_fnRegisterPin(IFilterMapper* iface,CLSID clsidFilter,LPCWSTR lpwszName,BOOL bRendered,BOOL bOutput,BOOL bZero,BOOL bMany,CLSID clsidReserved,LPCWSTR lpwszReserved)
{
	CFilterMapper_THIS(iface,fmap);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterMapper_fnRegisterPinType(IFilterMapper* iface,CLSID clsidFilter,LPCWSTR lpwszName,CLSID clsidMajorType,CLSID clsidSubType)
{
	CFilterMapper_THIS(iface,fmap);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterMapper_fnUnregisterFilter(IFilterMapper* iface,CLSID clsidFilter)
{
	CFilterMapper_THIS(iface,fmap);

	FIXME("(%p)->(%s)\n",This,debugstr_guid(&clsidFilter));

	/* FIXME */
	return QUARTZ_RegisterAMovieFilter(
		&CLSID_LegacyAmFilterCategory,
		&clsidFilter,
		NULL, 0, NULL, NULL, FALSE );
}

static HRESULT WINAPI
IFilterMapper_fnUnregisterFilterInstance(IFilterMapper* iface,CLSID clsidMedia)
{
	CFilterMapper_THIS(iface,fmap);

	FIXME("(%p)->(%s)\n",This,debugstr_guid(&clsidMedia));

	/* FIXME */
	/* this doesn't work. */
	/* return IFilterMapper_UnregisterFilter(iface,clsidMedia); */

	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterMapper_fnUnregisterPin(IFilterMapper* iface,CLSID clsidPin,LPCWSTR lpwszName)
{
	CFilterMapper_THIS(iface,fmap);

	FIXME("(%p)->(%s,%s) stub!\n",This,
		debugstr_guid(&clsidPin),debugstr_w(lpwszName));

	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterMapper_fnEnumMatchingFilters(IFilterMapper* iface,IEnumRegFilters** ppobj,DWORD dwMerit,BOOL bInputNeeded,CLSID clsInMajorType,CLSID clsidSubType,BOOL bRender,BOOL bOutputNeeded,CLSID clsOutMajorType,CLSID clsOutSubType)
{
	CFilterMapper_THIS(iface,fmap);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}



static ICOM_VTABLE(IFilterMapper) ifmap =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IFilterMapper_fnQueryInterface,
	IFilterMapper_fnAddRef,
	IFilterMapper_fnRelease,
	/* IFilterMapper fields */
	IFilterMapper_fnRegisterFilter,
	IFilterMapper_fnRegisterFilterInstance,
	IFilterMapper_fnRegisterPin,
	IFilterMapper_fnRegisterPinType,
	IFilterMapper_fnUnregisterFilter,
	IFilterMapper_fnUnregisterFilterInstance,
	IFilterMapper_fnUnregisterPin,
	IFilterMapper_fnEnumMatchingFilters,
};


HRESULT CFilterMapper_InitIFilterMapper( CFilterMapper* pfm )
{
	TRACE("(%p)\n",pfm);
	ICOM_VTBL(&pfm->fmap) = &ifmap;

	return NOERROR;
}

void CFilterMapper_UninitIFilterMapper( CFilterMapper* pfm )
{
	TRACE("(%p)\n",pfm);
}
