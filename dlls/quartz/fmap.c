/*
 * Implementation of CLSID_FilterMapper and CLSID_FilterMapper2.
 *
 * FIXME - stub.
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
#include "strmif.h"
#include "uuids.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "fmap.h"
#include "regsvr.h"


/***************************************************************************
 *
 *	new/delete for CLSID_FilterMapper
 *
 */

/* can I use offsetof safely? - FIXME? */
static QUARTZ_IFEntry FMapIFEntries[] =
{
  { &IID_IFilterMapper, offsetof(CFilterMapper,fmap)-offsetof(CFilterMapper,unk) },
};


static void QUARTZ_DestroyFilterMapper(IUnknown* punk)
{
	CFilterMapper_THIS(punk,unk);

	CFilterMapper_UninitIFilterMapper( This );
}

HRESULT QUARTZ_CreateFilterMapper(IUnknown* punkOuter,void** ppobj)
{
	CFilterMapper*	pfm;
	HRESULT	hr;

	TRACE("(%p,%p)\n",punkOuter,ppobj);

	pfm = (CFilterMapper*)QUARTZ_AllocObj( sizeof(CFilterMapper) );
	if ( pfm == NULL )
		return E_OUTOFMEMORY;

	QUARTZ_IUnkInit( &pfm->unk, punkOuter );
	hr = CFilterMapper_InitIFilterMapper( pfm );
	if ( FAILED(hr) )
	{
		QUARTZ_FreeObj( pfm );
		return hr;
	}

	pfm->unk.pEntries = FMapIFEntries;
	pfm->unk.dwEntries = sizeof(FMapIFEntries)/sizeof(FMapIFEntries[0]);
	pfm->unk.pOnFinalRelease = QUARTZ_DestroyFilterMapper;

	*ppobj = (void*)(&pfm->unk);

	return S_OK;
}

/***************************************************************************
 *
 *	CLSID_FilterMapper::IFilterMapper
 *
 */

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


/***************************************************************************
 *
 *	new/delete for CLSID_FilterMapper2
 *
 */

/* can I use offsetof safely? - FIXME? */
static QUARTZ_IFEntry FMap2IFEntries[] =
{
  { &IID_IFilterMapper2, offsetof(CFilterMapper2,fmap3)-offsetof(CFilterMapper2,unk) },
  { &IID_IFilterMapper3, offsetof(CFilterMapper2,fmap3)-offsetof(CFilterMapper2,unk) },
};


static void QUARTZ_DestroyFilterMapper2(IUnknown* punk)
{
	CFilterMapper2_THIS(punk,unk);

	CFilterMapper2_UninitIFilterMapper3( This );
}

HRESULT QUARTZ_CreateFilterMapper2(IUnknown* punkOuter,void** ppobj)
{
	CFilterMapper2*	pfm;
	HRESULT	hr;

	TRACE("(%p,%p)\n",punkOuter,ppobj);

	pfm = (CFilterMapper2*)QUARTZ_AllocObj( sizeof(CFilterMapper2) );
	if ( pfm == NULL )
		return E_OUTOFMEMORY;

	QUARTZ_IUnkInit( &pfm->unk, punkOuter );
	hr = CFilterMapper2_InitIFilterMapper3( pfm );
	if ( FAILED(hr) )
	{
		QUARTZ_FreeObj( pfm );
		return hr;
	}

	pfm->unk.pEntries = FMap2IFEntries;
	pfm->unk.dwEntries = sizeof(FMap2IFEntries)/sizeof(FMap2IFEntries[0]);
	pfm->unk.pOnFinalRelease = QUARTZ_DestroyFilterMapper2;

	*ppobj = (void*)(&pfm->unk);

	return S_OK;
}

/***************************************************************************
 *
 *	CLSID_FilterMapper2::IFilterMapper3
 *
 */


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

