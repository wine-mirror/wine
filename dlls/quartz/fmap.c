/*
 * Implementation of CLSID_FilterMapper and CLSID_FilterMapper2.
 *
 * FIXME - stub.
 *
 * Copyright (C) Hidenori TAKESHIMA <hidenori@a2.ctktv.ne.jp>
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
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "winerror.h"
#include "strmif.h"
#include "uuids.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "fmap.h"
#include "regsvr.h"
#include "devenum.h"
#include "complist.h"
#include "enumunk.h"

/***************************************************************************/

typedef struct QUARTZ_REGFILTERDATA
{
	DWORD	dwVersion; /* =2 */
	DWORD	dwMerit;
	DWORD	cPins; /* count of pins */
	DWORD	dwZero; /* padding??? */
} QUARTZ_REGFILTERDATA;

typedef struct QUARTZ_REGPINDATA
{
	CHAR	id[4]; /* '0pi3', '1pi3', ... */
	DWORD	dwFlags; /* flags */
	UINT	cInstances; /* FIXME - is this correct? */
	UINT	nMediaTypes; /* count of media types('0ty3') */
	UINT	nMediums; /* FIXME - is this correct? */
	UINT	nOfsClsPinCategory; /* FIXME - is this correct? */
} QUARTZ_REGPINDATA;

typedef struct QUARTZ_REGMEDIATYPE
{
	CHAR	id[4]; /* '0ty3', '1ty3', ... */
	DWORD	nZero; /* padding??? */
	UINT	nOfsMajorType;
	UINT	nOfsMinorType;
} QUARTZ_REGMEDIATYPE;



/***************************************************************************/

static
REGFILTER2* QUARTZ_RegFilterV2FromFilterData(
	const BYTE* pData, DWORD cbData )
{
	REGFILTER2*	pFilter;
	REGFILTERPINS2*	pPin;
	REGPINTYPES*	pTypes;
	BYTE* pDst;
	const QUARTZ_REGFILTERDATA*	pRegFilter;
	const QUARTZ_REGPINDATA*	pRegPin;
	const QUARTZ_REGMEDIATYPE*	pRegMediaType;
	DWORD	cPins;
	DWORD	cbBufSize;
	UINT	n;

	TRACE("(%p,%lu)\n",pData,cbData);

	if ( cbData < sizeof(QUARTZ_REGFILTERDATA) )
		return NULL;

	pRegFilter = (QUARTZ_REGFILTERDATA*)pData;

	if ( pRegFilter->dwVersion != 2 ) return NULL; /* FIXME */

	if ( cbData < (sizeof(QUARTZ_REGFILTERDATA)+sizeof(QUARTZ_REGPINDATA)*pRegFilter->cPins) )
		return NULL;

	cbBufSize = sizeof(REGFILTER2);
	cPins = pRegFilter->cPins;
	pRegPin = (const QUARTZ_REGPINDATA*)(pRegFilter+1);
	while ( cPins-- > 0 )
	{
		if ( pRegPin->nMediums != 0 ||
			 pRegPin->nOfsClsPinCategory != 0 )
			return NULL; /* FIXME */

		cbBufSize += sizeof(REGFILTERPINS2) +
			pRegPin->nMediaTypes * (sizeof(REGPINTYPES) + sizeof(GUID)*2) +
			pRegPin->nMediums * sizeof(REGPINMEDIUM) +
			sizeof(CLSID);
		pRegPin = (const QUARTZ_REGPINDATA*)( ((const BYTE*)pRegPin) +
			sizeof(QUARTZ_REGPINDATA) +
			sizeof(QUARTZ_REGMEDIATYPE) * pRegPin->nMediaTypes );
	}

	pFilter = (REGFILTER2*)QUARTZ_AllocMem( cbBufSize );
	if ( pFilter == NULL ) return NULL;
	ZeroMemory( pFilter, cbBufSize );
	pPin = (REGFILTERPINS2*)(pFilter+1);
	pDst = (BYTE*)(pPin + pRegFilter->cPins);

	pFilter->dwVersion = 2;
	pFilter->dwMerit = pRegFilter->dwMerit;
	pFilter->u.s2.cPins2 = pRegFilter->cPins;
	pFilter->u.s2.rgPins2 = pPin;

	cPins = pRegFilter->cPins;
	TRACE("cPins = %lu\n",cPins);

	pRegPin = (const QUARTZ_REGPINDATA*)(pRegFilter+1);
	while ( cPins-- > 0 )
	{
		pPin->dwFlags = pRegPin->dwFlags;
		pPin->cInstances = pRegPin->cInstances;
		pPin->nMediaTypes = pRegPin->nMediaTypes;
		pPin->lpMediaType = NULL;
		pPin->nMediums = pRegPin->nMediums;
		pPin->lpMedium = NULL;
		pPin->clsPinCategory = NULL;

		pTypes = (REGPINTYPES*)pDst;
		pPin->lpMediaType = pTypes;
		pDst += sizeof(REGPINTYPES) * pRegPin->nMediaTypes;

		pRegPin = (const QUARTZ_REGPINDATA*)( ((const BYTE*)pRegPin) +
			sizeof(QUARTZ_REGPINDATA) );

		for ( n = 0; n < pPin->nMediaTypes; n++ )
		{
			pRegMediaType = ((const QUARTZ_REGMEDIATYPE*)pRegPin);
			TRACE("ofsMajor = %u, ofsMinor = %u\n", pRegMediaType->nOfsMajorType, pRegMediaType->nOfsMinorType);
			memcpy( pDst, pData+pRegMediaType->nOfsMajorType, sizeof(GUID) );
			pTypes->clsMajorType = (const GUID*)pDst; pDst += sizeof(GUID);
			memcpy( pDst, pData+pRegMediaType->nOfsMinorType, sizeof(GUID) );
			pTypes->clsMinorType = (const GUID*)pDst; pDst += sizeof(GUID);

			pRegPin = (const QUARTZ_REGPINDATA*)( ((const BYTE*)pRegPin) +
				sizeof(QUARTZ_REGMEDIATYPE) );
			pTypes ++;
		}

		/* FIXME - pPin->lpMedium */
		/* FIXME - pPin->clsPinCategory */

		pPin ++;
	}

	return pFilter;
}

static
BYTE* QUARTZ_RegFilterV2ToFilterData(
	const REGFILTER2* pFilter, DWORD* pcbData )
{
	DWORD	cbData;
	DWORD	cbPinData;
	DWORD	cPins;
	const REGFILTERPINS2*	pPin;
	const REGPINTYPES*	pTypes;
	BYTE*	pRet = NULL;
	BYTE*	pDst;
	QUARTZ_REGFILTERDATA*	pRegFilter;
	QUARTZ_REGPINDATA*	pRegPin;
	QUARTZ_REGMEDIATYPE*	pRegMediaType;
	UINT	n;

	if ( pFilter->dwVersion != 2 ) return NULL; /* FIXME */

	cbData = sizeof(QUARTZ_REGFILTERDATA);
	cPins = pFilter->u.s2.cPins2;
	pPin = pFilter->u.s2.rgPins2;
	if ( cPins > 10 ) return NULL; /* FIXME */

	cbPinData = 0;
	while ( cPins-- > 0 )
	{
		if ( pPin->cInstances != 0 ||
			 pPin->nMediaTypes > 10 ||
			 pPin->nMediums != 0 ||
			 pPin->clsPinCategory != 0 )
		{
			FIXME( "not implemented.\n" );
			return NULL; /* FIXME */
		}

		cbPinData += sizeof(QUARTZ_REGPINDATA) +
			pPin->nMediaTypes * sizeof(QUARTZ_REGMEDIATYPE);
		cbData += pPin->nMediaTypes * (sizeof(GUID)*2);
		pPin ++;
	}
	cbData += cbPinData;
	TRACE("cbData %lu, cbPinData %lu\n",cbData,cbPinData);

	pRet = (BYTE*)QUARTZ_AllocMem( cbData );
	if ( pRet == NULL ) return NULL;
	ZeroMemory( pRet, cbData );
	pDst = pRet;

	pRegFilter = (QUARTZ_REGFILTERDATA*)pDst;
	pDst += sizeof(QUARTZ_REGFILTERDATA);

	pRegFilter->dwVersion = 2;
	pRegFilter->dwMerit = pFilter->dwMerit;
	pRegFilter->cPins = pFilter->u.s2.cPins2;

	pRegPin = (QUARTZ_REGPINDATA*)pDst;
	pDst += cbPinData;

	pPin = pFilter->u.s2.rgPins2;
	for ( cPins = 0; cPins < pFilter->u.s2.cPins2; cPins++ )
	{
		pRegPin->id[0] = '0'+cPins;
		pRegPin->id[1] = 'p';
		pRegPin->id[2] = 'i';
		pRegPin->id[3] = '3';
		pRegPin->dwFlags = pPin->dwFlags; /* flags */
		pRegPin->cInstances = pPin->cInstances;
		pRegPin->nMediaTypes = pPin->nMediaTypes;
		pRegPin->nMediums = pPin->nMediums;
		pRegPin->nOfsClsPinCategory = 0; /* FIXME */

		pTypes = pPin->lpMediaType;
		pRegPin = (QUARTZ_REGPINDATA*)( ((const BYTE*)pRegPin) +
			sizeof(QUARTZ_REGPINDATA) );
		for ( n = 0; n < pPin->nMediaTypes; n++ )
		{
			pRegMediaType = ((QUARTZ_REGMEDIATYPE*)pRegPin);

			pRegMediaType->id[0] = '0'+n;
			pRegMediaType->id[1] = 't';
			pRegMediaType->id[2] = 'y';
			pRegMediaType->id[3] = '3';

			/* FIXME - CLSID should be shared. */
			pRegMediaType->nOfsMajorType = pDst - pRet;
			memcpy( pDst, pTypes->clsMajorType, sizeof(GUID) );
			pDst += sizeof(GUID);
			pRegMediaType->nOfsMinorType = pDst - pRet;
			memcpy( pDst, pTypes->clsMinorType, sizeof(GUID) );
			pDst += sizeof(GUID);

			pRegPin = (QUARTZ_REGPINDATA*)( ((const BYTE*)pRegPin) +
				sizeof(QUARTZ_REGMEDIATYPE) );
			pTypes ++;
		}
		pPin ++;
	}

	*pcbData = pDst - pRet;
	TRACE("cbData %lu/%lu\n",*pcbData,cbData);

	return pRet;
}

static
REGFILTER2* QUARTZ_RegFilterV1ToV2( const REGFILTER2* prfV1 )
{
	REGFILTER2*	prfV2;
	const REGFILTERPINS*	pPinV1;
	REGFILTERPINS2*	pPinV2;
	DWORD	cPins;

	if ( prfV1->dwVersion != 1 ) return NULL;

	prfV2 = (REGFILTER2*)QUARTZ_AllocMem(
		sizeof(REGFILTER2) + sizeof(REGFILTERPINS2) * prfV1->u.s1.cPins );
	if ( prfV2 == NULL ) return NULL;
	ZeroMemory( prfV2, sizeof(REGFILTER2) + sizeof(REGFILTERPINS2) * prfV1->u.s1.cPins );
	pPinV1 = prfV1->u.s1.rgPins;
	pPinV2 = (REGFILTERPINS2*)(prfV2+1);
	prfV2->dwVersion = 2;
	prfV2->dwMerit = prfV1->dwMerit;
	prfV2->u.s2.cPins2 = prfV1->u.s1.cPins;
	prfV2->u.s2.rgPins2 = pPinV2;

	cPins = prfV1->u.s1.cPins;
	while ( cPins-- > 0 )
	{
		pPinV2->dwFlags = 0;
		pPinV2->cInstances = 0;
		pPinV2->nMediaTypes = pPinV1->nMediaTypes;
		pPinV2->lpMediaType = pPinV1->lpMediaType;
		pPinV2->nMediums = 0;
		pPinV2->lpMedium = NULL;
		pPinV2->clsPinCategory = NULL;

		if ( pPinV1->bRendered )
			pPinV2->dwFlags |= REG_PINFLAG_B_RENDERER;
		if ( pPinV1->bOutput )
			pPinV2->dwFlags |= REG_PINFLAG_B_OUTPUT;
		if ( pPinV1->bZero )
			pPinV2->dwFlags |= REG_PINFLAG_B_ZERO;
		if ( pPinV1->bMany )
			pPinV2->dwFlags |= REG_PINFLAG_B_MANY;

		pPinV1 ++;
		pPinV2 ++;
	}

	return prfV2;
}

static
BYTE* QUARTZ_RegFilterToFilterData(
	const REGFILTER2* pFilter, DWORD* pcbData )
{
	REGFILTER2*	prfV2;
	BYTE*	pRet = NULL;

	*pcbData = 0;
	switch ( pFilter->dwVersion )
	{
	case 1:
		prfV2 = QUARTZ_RegFilterV1ToV2( pFilter );
		if ( prfV2 != NULL )
		{
			pRet = QUARTZ_RegFilterV2ToFilterData( prfV2, pcbData );
			QUARTZ_FreeMem( prfV2 );
		}
		break;
	case 2:
		pRet = QUARTZ_RegFilterV2ToFilterData( pFilter, pcbData );
		break;
	default:
		FIXME( "unknown REGFILTER2 version - %08lu\n", pFilter->dwVersion );
		break;
	}

	return pRet;
}

/***************************************************************************/

static
BOOL QUARTZ_CheckPinType( BOOL bExactMatch, const REGFILTERPINS2* pPin, DWORD cTypes, const GUID* pTypes, const REGPINMEDIUM* pMedium, const CLSID* pCategory, BOOL bRender )
{
	DWORD	n1, n2;
	BOOL	bMatch;

	if ( cTypes > 0 && pTypes != NULL )
	{
		bMatch = FALSE;
		for ( n1 = 0; n1 < pPin->nMediaTypes; n1++ )
		{
			for ( n2 = 0; n2 < cTypes; n2++ )
			{
				if ( IsEqualGUID(pPin->lpMediaType[n1].clsMajorType,&GUID_NULL) || IsEqualGUID(pPin->lpMediaType[n1].clsMajorType, &pTypes[n2*2+0]) || (!bExactMatch && IsEqualGUID(pPin->lpMediaType[n1].clsMajorType,&GUID_NULL)) )
				{
					if ( IsEqualGUID(pPin->lpMediaType[n1].clsMinorType,&GUID_NULL) || IsEqualGUID(pPin->lpMediaType[n1].clsMinorType, &pTypes[n2*2+1]) || (!bExactMatch && IsEqualGUID(pPin->lpMediaType[n1].clsMinorType,&GUID_NULL)) )
					{
						bMatch = TRUE;
						break;
					}
				}
			}
		}
		if ( !bMatch )
			return FALSE;
	}

	if ( pMedium != NULL )
	{
		bMatch = FALSE;
		for ( n1 = 0; n1 < pPin->nMediums; n1++ )
		{
			if ( IsEqualGUID( &pPin->lpMedium[n1].clsMedium, &pMedium->clsMedium ) && pPin->lpMedium[n1].dw1 == pMedium->dw1 && pPin->lpMedium[n1].dw2 == pMedium->dw2 )
			{
				bMatch = TRUE;
				break;
			}
		}
		if ( !bMatch )
			return FALSE;
	}

	if ( pCategory != NULL )
	{
		if ( pPin->clsPinCategory == NULL )
			return FALSE;
		if ( (!bExactMatch && IsEqualGUID(pCategory,&GUID_NULL)) || IsEqualGUID(pCategory,pPin->clsPinCategory) )
			return TRUE;
		return FALSE;
	}

	if ( bRender && (!(pPin->dwFlags & REG_PINFLAG_B_RENDERER)) )
		return FALSE;

	return TRUE;
}




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
	WCHAR*	pwszPath = NULL;
	HRESULT hr;

	TRACE("(%p)->(%s,%s,%s)\n",This,
		debugstr_guid(pclsidCategory),
		debugstr_w(lpwszInst),
		debugstr_guid(rclsidFilter));

	if ( pclsidCategory == NULL )
		pclsidCategory = &CLSID_LegacyAmFilterCategory;

	hr = QUARTZ_GetFilterRegPath(
		&pwszPath, pclsidCategory, rclsidFilter, lpwszInst );
	if ( FAILED(hr) )
		return hr;

	hr = QUARTZ_RegDeleteKey(HKEY_CLASSES_ROOT,pwszPath);
	QUARTZ_FreeMem(pwszPath);

	return hr;
}


static HRESULT WINAPI
IFilterMapper3_fnRegisterFilter(IFilterMapper3* iface,REFCLSID rclsidFilter,LPCWSTR lpName,IMoniker** ppMoniker,const CLSID* pclsidCategory,const OLECHAR* lpwszInst,const REGFILTER2* pRF2)
{
	CFilterMapper2_THIS(iface,fmap3);
	WCHAR*  pwszPath = NULL;
	IMoniker*	pMoniker = NULL;
	BYTE*	pFilterData = NULL;
	DWORD	cbFilterData = 0;
	HRESULT hr;

	TRACE( "(%p)->(%s,%s,%p,%s,%s,%p) stub!\n",This,
		debugstr_guid(rclsidFilter),debugstr_w(lpName),
		ppMoniker,debugstr_guid(pclsidCategory),
		debugstr_w(lpwszInst),pRF2 );

	if ( lpName == NULL || pRF2 == NULL )
		return E_POINTER;

	if ( ppMoniker != NULL && *ppMoniker != NULL )
	{
		FIXME( "ppMoniker != NULL - not implemented! *ppMoniker = %p\n",*ppMoniker );
		return E_NOTIMPL;
	}

	if ( pclsidCategory == NULL )
		pclsidCategory = &CLSID_LegacyAmFilterCategory;

	if ( pMoniker == NULL )
	{
	        hr = QUARTZ_GetFilterRegPath(
		        &pwszPath, pclsidCategory, rclsidFilter, lpwszInst );
		if ( FAILED(hr) )
			return hr;
		hr = QUARTZ_CreateDeviceMoniker(
			HKEY_CLASSES_ROOT,pwszPath,&pMoniker);
		QUARTZ_FreeMem(pwszPath);
		if ( FAILED(hr) )
			return hr;
	}

	pFilterData = QUARTZ_RegFilterToFilterData( pRF2, &cbFilterData );
	if ( pFilterData == NULL || cbFilterData == 0 )
	{
		hr = E_FAIL;
		goto err;
	}

	hr = QUARTZ_RegisterFilterToMoniker(
		pMoniker, rclsidFilter, lpName, pFilterData, cbFilterData );
	if ( FAILED(hr) )
		goto err;

	if ( ppMoniker != NULL )
	{
		*ppMoniker = pMoniker;
		pMoniker = NULL;
	}
err:
	if ( pFilterData != NULL )
		QUARTZ_FreeMem(pFilterData);
	if ( pMoniker != NULL )
		IMoniker_Release(pMoniker);

	return hr;
}


static HRESULT WINAPI
IFilterMapper3_fnEnumMatchingFilters(IFilterMapper3* iface,
	IEnumMoniker** ppEnumMoniker,DWORD dwFlags,BOOL bExactMatch,DWORD dwMerit,
	BOOL bInputNeeded,DWORD cInputTypes,const GUID* pguidInputTypes,const REGPINMEDIUM* pPinMediumIn,const CLSID* pPinCategoryIn,BOOL bRender,
	BOOL bOutputNeeded,DWORD cOutputTypes,const GUID* pguidOutputTypes,const REGPINMEDIUM* pPinMediumOut,const CLSID* pPinCategoryOut)
{
	CFilterMapper2_THIS(iface,fmap3);
	ICreateDevEnum*	pEnum = NULL;
	IEnumMoniker*	pCategories = NULL;
	IMoniker*	pCat = NULL;
	DWORD	dwCatMerit;
	IEnumMoniker*	pCatFilters = NULL;
	IMoniker*	pFilter = NULL;
	CLSID	clsid;
	ULONG	cReturned;
	BYTE*	pbFilterData = NULL;
	DWORD	cbFilterData = 0;
	REGFILTER2*	prf2 = NULL;
	QUARTZ_CompList*	pList = NULL;
	const REGFILTERPINS2*	pRegFilterPin;
	DWORD	n;
	BOOL	bMatch;
	HRESULT hr;

	FIXME("(%p)->(%p,%08lx,%d,%08lx,%d,%lu,%p,%p,%p,%d,%d,%lu,%p,%p,%p)\n",This,ppEnumMoniker,dwFlags,bExactMatch,dwMerit,bInputNeeded,cInputTypes,pguidInputTypes,pPinMediumIn,pPinCategoryIn,bRender,bOutputNeeded,cOutputTypes,pguidOutputTypes,pPinMediumOut,pPinCategoryOut);

	if ( ppEnumMoniker == NULL )
		return E_POINTER;
	*ppEnumMoniker = NULL;
	if ( dwFlags != 0 )
		return E_INVALIDARG;

	hr = CoCreateInstance(
		&CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
		&IID_ICreateDevEnum, (void**)&pEnum );
	if ( FAILED(hr) )
		goto err;

	hr = ICreateDevEnum_CreateClassEnumerator(pEnum,&CLSID_ActiveMovieCategories,&pCategories,0);
	if ( hr != S_OK )
		goto err;

	while ( 1 )
	{
		if ( pCat != NULL )
		{
			IMoniker_Release(pCat);
			pCat = NULL;
		}
		hr = IEnumMoniker_Next(pCategories,1,&pCat,&cReturned);
		if ( FAILED(hr) )
			goto err;
		if ( hr != S_OK )
			break;
		hr = QUARTZ_GetMeritFromMoniker(pCat,&dwCatMerit);
		if ( hr != S_OK || dwMerit > dwCatMerit )
			continue;
		hr = QUARTZ_GetCLSIDFromMoniker(pCat,&clsid);
		if ( hr != S_OK )
			continue;

		if ( pCatFilters != NULL )
		{
			IEnumMoniker_Release(pCatFilters);
			pCatFilters = NULL;
		}
		hr = ICreateDevEnum_CreateClassEnumerator(pEnum,&clsid,&pCatFilters,0);
		if ( FAILED(hr) )
			goto err;
		if ( hr != S_OK )
			continue;

		while ( 1 )
		{
			if ( pFilter != NULL )
			{
				IMoniker_Release(pFilter);
				pFilter = NULL;
			}
			hr = IEnumMoniker_Next(pCatFilters,1,&pFilter,&cReturned);
			if ( FAILED(hr) )
				goto err;
			if ( hr != S_OK )
				break;
			if ( pbFilterData != NULL )
			{
				QUARTZ_FreeMem(pbFilterData);
				pbFilterData = NULL;
			}
			hr = QUARTZ_GetFilterDataFromMoniker(pFilter,&pbFilterData,&cbFilterData);
			if ( hr != S_OK )
				continue;

			if ( prf2 != NULL )
			{
				QUARTZ_FreeMem(prf2);
				prf2 = NULL;
			}
			prf2 = QUARTZ_RegFilterV2FromFilterData(pbFilterData,cbFilterData);
			if ( prf2 == NULL )
				continue;
			TRACE("prf2 %p, Merit %lu\n",prf2,prf2->dwMerit);
			if ( prf2->dwMerit < dwMerit || prf2->dwVersion != 2 )
				continue;

			/* check input pins. */
			if ( bInputNeeded )
			{
				bMatch = FALSE;
				for ( n = 0; n < prf2->u.s2.cPins2; n++ )
				{
					pRegFilterPin = &prf2->u.s2.rgPins2[n];
					if ( pRegFilterPin->dwFlags & REG_PINFLAG_B_OUTPUT )
						continue;
					bMatch = QUARTZ_CheckPinType( bExactMatch, pRegFilterPin, cInputTypes, pguidInputTypes, pPinMediumIn, pPinCategoryIn, bRender );
					if ( bMatch )
						break;
				}
				if ( !bMatch )
					continue;
			}

			/* check output pins. */
			if ( bOutputNeeded )
			{
				bMatch = FALSE;
				for ( n = 0; n < prf2->u.s2.cPins2; n++ )
				{
					pRegFilterPin = &prf2->u.s2.rgPins2[n];
					if ( !(pRegFilterPin->dwFlags & REG_PINFLAG_B_OUTPUT) )
						continue;
					bMatch = QUARTZ_CheckPinType( bExactMatch, pRegFilterPin, cOutputTypes, pguidOutputTypes, pPinMediumOut, pPinCategoryOut, FALSE );
					if ( bMatch )
						break;
				}
				if ( !bMatch )
					continue;
			}

			/* matched - add pFilter to the list. */
			if ( pList == NULL )
			{
				pList = QUARTZ_CompList_Alloc();
				if ( pList == NULL )
				{
					hr = E_OUTOFMEMORY;
					goto err;
				}
			}
			hr = QUARTZ_CompList_AddComp(
				pList, (IUnknown*)pFilter, NULL, 0 );
			if ( FAILED(hr) )
				goto err;
		}
	}

	if ( pList == NULL )
	{
		hr = S_FALSE;
		goto err;
	}

	FIXME("create IEnumMoniker - not sorted\n");
	/* FIXME - should be sorted?(in Merit order) */
	hr = QUARTZ_CreateEnumUnknown( &IID_IEnumMoniker, (void**)ppEnumMoniker, pList );
	if ( FAILED(hr) )
		goto err;

	hr = S_OK;
err:
	if ( pEnum != NULL )
		ICreateDevEnum_Release(pEnum);
	if ( pCategories != NULL )
		IEnumMoniker_Release(pCategories);
	if ( pCat != NULL )
		IMoniker_Release(pCat);
	if ( pCatFilters != NULL )
		IEnumMoniker_Release(pCatFilters);
	if ( pFilter != NULL )
		IMoniker_Release(pFilter);
	if ( pbFilterData != NULL )
		QUARTZ_FreeMem(pbFilterData);
	if ( prf2 != NULL )
		QUARTZ_FreeMem(prf2);
	if ( pList != NULL )
		QUARTZ_CompList_Free( pList );

	TRACE("returns %08lx\n",hr);

	return hr;
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

