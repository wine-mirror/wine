/*
 * Implements dmort APIs.
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
#include "winerror.h"
#include "wine/obj_base.h"
#include "mediaobj.h"
#include "dmort.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(msdmo);


HRESULT WINAPI MoCopyMediaType( DMO_MEDIA_TYPE* pmtDst, const DMO_MEDIA_TYPE* pmtSrc )
{
	FIXME( "() not tested\n" );

	memcpy( &pmtDst->majortype, &pmtSrc->majortype, sizeof(GUID) );
	memcpy( &pmtDst->subtype, &pmtSrc->subtype, sizeof(GUID) );
	pmtDst->bFixedSizeSamples = pmtSrc->bFixedSizeSamples;
	pmtDst->bTemporalCompression = pmtSrc->bTemporalCompression;
	pmtDst->lSampleSize = pmtSrc->lSampleSize;
	memcpy( &pmtDst->formattype, &pmtSrc->formattype, sizeof(GUID) );
	pmtDst->pUnk = NULL;
	pmtDst->cbFormat = pmtSrc->cbFormat;
	pmtDst->pbFormat = NULL;

	if ( pmtSrc->pbFormat != NULL && pmtSrc->cbFormat != 0 )
	{
		pmtDst->pbFormat = (BYTE*)CoTaskMemAlloc( pmtSrc->cbFormat );
		if ( pmtDst->pbFormat == NULL )
		{
			return E_OUTOFMEMORY;
		}
		memcpy( pmtDst->pbFormat, pmtSrc->pbFormat, pmtSrc->cbFormat );
	}

	if ( pmtSrc->pUnk != NULL )
	{
		pmtDst->pUnk = pmtSrc->pUnk;
		IUnknown_AddRef( pmtSrc->pUnk );
	}

	return S_OK;
}

HRESULT WINAPI MoCreateMediaType( DMO_MEDIA_TYPE** ppmt, DWORD cbFormat )
{
	HRESULT	hr;
	DMO_MEDIA_TYPE*	pmt;

	FIXME( "() not tested\n" );

	if ( ppmt == NULL )
		return E_POINTER;
	*ppmt = NULL;

	pmt = (DMO_MEDIA_TYPE*)CoTaskMemAlloc( sizeof(DMO_MEDIA_TYPE) );
	if ( pmt == NULL )
		return E_OUTOFMEMORY;
	hr = MoInitMediaType( pmt, cbFormat );
	if ( FAILED(hr) )
		return hr;

	*ppmt = pmt;

	return S_OK;
}

HRESULT WINAPI MoDeleteMediaType( DMO_MEDIA_TYPE* pmt )
{
	FIXME( "() not tested\n" );

	MoFreeMediaType( pmt );
	CoTaskMemFree( pmt );

	return S_OK;
}

HRESULT WINAPI MoDuplicateMediaType( DMO_MEDIA_TYPE** ppmtDest, const DMO_MEDIA_TYPE* pmtSrc )
{
	HRESULT	hr;
	DMO_MEDIA_TYPE*	pmtDup;

	FIXME( "() not tested\n" );

	if ( ppmtDest == NULL )
		return E_POINTER;
	*ppmtDest = NULL;

	pmtDup = (DMO_MEDIA_TYPE*)CoTaskMemAlloc( sizeof(DMO_MEDIA_TYPE) );
	if ( pmtDup == NULL )
		return E_OUTOFMEMORY;
	hr = MoCopyMediaType( pmtDup, pmtSrc );
	if ( FAILED(hr) )
		return hr;

	*ppmtDest = pmtDup;

	return S_OK;
}

HRESULT WINAPI MoFreeMediaType( DMO_MEDIA_TYPE* pmt )
{
	FIXME( "() not tested\n" );

	if ( pmt->pUnk != NULL )
	{
		IUnknown_Release( pmt->pUnk );
		pmt->pUnk = NULL;
	}
	if ( pmt->pbFormat != NULL )
	{
		CoTaskMemFree( pmt->pbFormat );
		pmt->cbFormat = 0;
		pmt->pbFormat = NULL;
	}

	return S_OK;
}

HRESULT WINAPI MoInitMediaType( DMO_MEDIA_TYPE* pmt, DWORD cbFormat )
{
	FIXME( "() not tested\n" );

	memset( pmt, 0, sizeof(DMO_MEDIA_TYPE) );

	pmt->pUnk = NULL;
	if ( cbFormat > 0 )
	{
		pmt->pbFormat = (BYTE*)CoTaskMemAlloc( cbFormat );
		if ( pmt->pbFormat == NULL )
			return E_OUTOFMEMORY;
		memset( pmt->pbFormat, 0, cbFormat );
	}
	pmt->cbFormat = cbFormat;

	return S_OK;
}
