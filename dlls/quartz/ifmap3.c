/*
 * Implementation of IFilterMapper3 for CLSID_FilterMapper2.
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
#include "fmap2.h"
#include "regsvr.h"


static HRESULT WINAPI
IFilterMapper3_fnQueryInterface(IFilterMapper3* iface,REFIID riid,void** ppobj)
{
	CFilterMapper2_THIS(iface,fmap3);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IFilterMapper3_fnAddRef(IFilterMapper3* iface)
{
	CFilterMapper2_THIS(iface,fmap3);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IFilterMapper3_fnRelease(IFilterMapper3* iface)
{
	CFilterMapper2_THIS(iface,fmap3);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}

static HRESULT WINAPI
IFilterMapper3_fnCreateCategory(IFilterMapper3* iface,REFCLSID rclsidCategory,DWORD dwMerit,LPCWSTR lpwszDesc)
{
	CFilterMapper2_THIS(iface,fmap3);

	FIXME("(%p)->(%s,%lu,%s) stub!\n",This,
		debugstr_guid(rclsidCategory),
		(unsigned long)dwMerit,debugstr_w(lpwszDesc));

	return E_NOTIMPL;
}


static HRESULT WINAPI
IFilterMapper3_fnUnregisterFilter(IFilterMapper3* iface,const CLSID* pclsidCategory,const OLECHAR* lpwszInst,REFCLSID rclsidFilter)
{
	CFilterMapper2_THIS(iface,fmap3);

	FIXME("(%p)->(%s,%s,%s) stub!\n",This,
		debugstr_guid(pclsidCategory),
		debugstr_w(lpwszInst),
		debugstr_guid(rclsidFilter));

	if ( pclsidCategory == NULL )
		pclsidCategory = &CLSID_LegacyAmFilterCategory;

	/* FIXME */
	return QUARTZ_RegisterAMovieFilter(
		pclsidCategory,
		rclsidFilter,
		NULL, 0,
		NULL, lpwszInst, FALSE );
}


static HRESULT WINAPI
IFilterMapper3_fnRegisterFilter(IFilterMapper3* iface,REFCLSID rclsidFilter,LPCWSTR lpName,IMoniker** ppMoniker,const CLSID* pclsidCategory,const OLECHAR* lpwszInst,const REGFILTER2* pRF2)
{
	CFilterMapper2_THIS(iface,fmap3);

	FIXME( "(%p)->(%s,%s,%p,%s,%s,%p) stub!\n",This,
		debugstr_guid(rclsidFilter),debugstr_w(lpName),
		ppMoniker,debugstr_guid(pclsidCategory),
		debugstr_w(lpwszInst),pRF2 );

	if ( lpName == NULL || pRF2 == NULL )
		return E_POINTER;

	if ( ppMoniker != NULL )
	{
		FIXME( "ppMoniker != NULL - not implemented!\n" );
		return E_NOTIMPL;
	}

	if ( pclsidCategory == NULL )
		pclsidCategory = &CLSID_LegacyAmFilterCategory;

	/* FIXME!! - all members in REGFILTER2 are ignored ! */

	return QUARTZ_RegisterAMovieFilter(
		pclsidCategory,
		rclsidFilter,
		NULL, 0,
		lpName, lpwszInst, TRUE );
}


static HRESULT WINAPI
IFilterMapper3_fnEnumMatchingFilters(IFilterMapper3* iface,IEnumMoniker** ppMoniker,DWORD dwFlags,BOOL bExactMatch,DWORD dwMerit,BOOL bInputNeeded,DWORD cInputTypes,const GUID* pguidInputTypes,const REGPINMEDIUM* pPinMediumIn,const CLSID* pPinCategoryIn,BOOL bRender,BOOL bOutputNeeded,DWORD cOutputTypes,const GUID* pguidOutputTypes,const REGPINMEDIUM* pPinMediumOut,const CLSID* pPinCategoryOut)
{
	CFilterMapper2_THIS(iface,fmap3);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterMapper3_fnGetICreateDevEnum(IFilterMapper3* iface,ICreateDevEnum** ppDevEnum)
{
	CFilterMapper2_THIS(iface,fmap3);

	/* undocumented */
	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}




static ICOM_VTABLE(IFilterMapper3) ifmap3 =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IFilterMapper3_fnQueryInterface,
	IFilterMapper3_fnAddRef,
	IFilterMapper3_fnRelease,
	/* IFilterMapper2 fields */
	IFilterMapper3_fnCreateCategory,
	IFilterMapper3_fnUnregisterFilter,
	IFilterMapper3_fnRegisterFilter,
	IFilterMapper3_fnEnumMatchingFilters,
	/* IFilterMapper3 fields */
	IFilterMapper3_fnGetICreateDevEnum,
};


HRESULT CFilterMapper2_InitIFilterMapper3( CFilterMapper2* pfm )
{
	TRACE("(%p)\n",pfm);
	ICOM_VTBL(&pfm->fmap3) = &ifmap3;

	return NOERROR;
}

void CFilterMapper2_UninitIFilterMapper3( CFilterMapper2* pfm )
{
	TRACE("(%p)\n",pfm);
}

