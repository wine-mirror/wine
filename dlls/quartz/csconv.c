/*
 * Implements Color Space Converter(CLSID_Colour).
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
#include "winerror.h"
#include "vfw.h"
#include "strmif.h"
#include "control.h"
#include "amvideo.h"
#include "vfwmsgs.h"
#include "uuids.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "xform.h"
#include "videoblt.h"

static const WCHAR ColorConv_FilterName[] =
{'C','o','l','o','r',' ','S','p','a','c','e',' ','C','o','n','v','e','r','t','e','r',0};

struct BltHandler
{
	const GUID*	psubtypeIn;
	const GUID*	psubtypeOut;
	pVIDEOBLT_Blt	pBlt;
};

static const struct BltHandler conv_handlers[] =
{
	{ &MEDIASUBTYPE_RGB24, &MEDIASUBTYPE_RGB32, VIDEOBLT_Blt_888_to_8888 },
	{ &MEDIASUBTYPE_RGB24, &MEDIASUBTYPE_RGB565, VIDEOBLT_Blt_888_to_565 },
	{ &MEDIASUBTYPE_RGB24, &MEDIASUBTYPE_RGB555, VIDEOBLT_Blt_888_to_555 },
	{ &MEDIASUBTYPE_RGB24, &MEDIASUBTYPE_RGB8, VIDEOBLT_Blt_888_to_332 },
	{ NULL, NULL, NULL },
};

typedef struct CColorConvImpl
{
	pVIDEOBLT_Blt	m_pBlt;
	AM_MEDIA_TYPE*	m_pmtConv;
	DWORD	m_cConv;
	LONG	pitchIn;
	LONG	pitchOut;
} CColorConvImpl;

/***************************************************************************
 *
 *	CColorConvImpl methods
 *
 */

static void ColorConv_FreeOutTypes(CColorConvImpl* This)
{
	DWORD	i;

	if ( This->m_pmtConv == NULL )
		return;

	TRACE("cConv = %lu\n",This->m_cConv);
	for ( i = 0; i < This->m_cConv; i++ )
	{
		QUARTZ_MediaType_Free(&This->m_pmtConv[i]);
	}
	QUARTZ_FreeMem(This->m_pmtConv);
	This->m_pmtConv = NULL;
	This->m_cConv = 0;
}

static HRESULT ColorConv_FillBitmapInfo( BITMAPINFO* pbiOut, LONG biWidth, LONG biHeight, const GUID* psubtype )
{
	int i;
	DWORD* pdwBitf;
	HRESULT hr = E_FAIL;

	pbiOut->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pbiOut->bmiHeader.biWidth = biWidth;
	pbiOut->bmiHeader.biHeight = biHeight;
	pbiOut->bmiHeader.biPlanes = 1;

	if ( IsEqualGUID( psubtype, &MEDIASUBTYPE_RGB8 ) )
	{
		pbiOut->bmiHeader.biBitCount = 8;
		for ( i = 0; i < 256; i++ )
		{
			pbiOut->bmiColors[i].rgbRed = ((i>>5)&7)*255/7;
			pbiOut->bmiColors[i].rgbGreen = ((i>>2)&7)*255/7;
			pbiOut->bmiColors[i].rgbBlue = (i&3)*255/3;
		}
		hr = S_OK;
	}
	if ( IsEqualGUID( psubtype, &MEDIASUBTYPE_RGB555 ) )
	{
		pbiOut->bmiHeader.biBitCount = 16;
		hr = S_OK;
	}
	if ( IsEqualGUID( psubtype, &MEDIASUBTYPE_RGB565 ) )
	{
		pbiOut->bmiHeader.biBitCount = 16;
		pbiOut->bmiHeader.biCompression = 3;
		pdwBitf = (DWORD*)(&pbiOut->bmiColors[0]);
		pdwBitf[0] = 0xf800;
		pdwBitf[1] = 0x07e0;
		pdwBitf[2] = 0x001f;
		hr = S_OK;
	}
	if ( IsEqualGUID( psubtype, &MEDIASUBTYPE_RGB24 ) )
	{
		pbiOut->bmiHeader.biBitCount = 24;
		hr = S_OK;
	}
	if ( IsEqualGUID( psubtype, &MEDIASUBTYPE_RGB32 ) )
	{
		pbiOut->bmiHeader.biBitCount = 32;
		hr = S_OK;
	}

	pbiOut->bmiHeader.biSizeImage = DIBSIZE(pbiOut->bmiHeader);

	return hr;
}


static HRESULT ColorConv_Init( CTransformBaseImpl* pImpl )
{
	CColorConvImpl*	This = pImpl->m_pUserData;

	if ( This != NULL )
		return NOERROR;

	This = (CColorConvImpl*)QUARTZ_AllocMem( sizeof(CColorConvImpl) );
	if ( This == NULL )
		return E_OUTOFMEMORY;
	ZeroMemory( This, sizeof(CColorConvImpl) );
	pImpl->m_pUserData = This;
	/* construct */
	This->m_pBlt = NULL;
	This->m_pmtConv = NULL;
	This->m_cConv = 0;

	return NOERROR;
}

static HRESULT ColorConv_Cleanup( CTransformBaseImpl* pImpl )
{
	CColorConvImpl*	This = pImpl->m_pUserData;

	if ( This == NULL )
		return NOERROR;
	/* destruct */
	ColorConv_FreeOutTypes(This);

	QUARTZ_FreeMem( This );
	pImpl->m_pUserData = NULL;

	return NOERROR;
}

static HRESULT ColorConv_CheckMediaType( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE* pmtOut )
{
	CColorConvImpl*	This = pImpl->m_pUserData;
	BITMAPINFOHEADER*	pbiIn = NULL;
	BITMAPINFOHEADER*	pbiOut = NULL;
	HRESULT hr;
	GUID stIn, stOut;
	const struct BltHandler*	phandler;

	TRACE("(%p)\n",This);

	if ( This == NULL )
		return E_UNEXPECTED;

	if ( !IsEqualGUID( &pmtIn->majortype, &MEDIATYPE_Video ) )
		return E_FAIL;
	if ( !IsEqualGUID( &pmtIn->formattype, &FORMAT_VideoInfo ) )
		return E_FAIL;
	pbiIn = (&((VIDEOINFOHEADER*)pmtIn->pbFormat)->bmiHeader);
	if ( pbiIn->biCompression != 0 &&
		 pbiIn->biCompression != 3 )
		return E_FAIL;

	hr = QUARTZ_MediaSubType_FromBitmap( &stIn, pbiIn );
	if ( hr != S_OK || !IsEqualGUID( &pmtIn->subtype, &stIn ) )
		return E_FAIL;

	if ( pmtOut != NULL )
	{
		if ( !IsEqualGUID( &pmtOut->majortype, &MEDIATYPE_Video ) )
			return E_FAIL;
		if ( !IsEqualGUID( &pmtOut->formattype, &FORMAT_VideoInfo ) )
			return E_FAIL;
		pbiOut = (&((VIDEOINFOHEADER*)pmtOut->pbFormat)->bmiHeader);
		if ( pbiOut->biCompression != 0 &&
			 pbiOut->biCompression != 3 )
			return E_FAIL;
		hr = QUARTZ_MediaSubType_FromBitmap( &stOut, pbiOut );
		if ( hr != S_OK || !IsEqualGUID( &pmtOut->subtype, &stOut ) )
			return E_FAIL;
		if ( pbiIn->biWidth != pbiOut->biWidth ||
			 pbiIn->biHeight != pbiOut->biHeight ||
			 pbiIn->biPlanes != 1 || pbiOut->biPlanes != 1 )
			return E_FAIL;
	}

	phandler = conv_handlers;
	while ( phandler->psubtypeIn != NULL )
	{
		if ( IsEqualGUID( &pmtIn->subtype, phandler->psubtypeIn ) )
		{
			if ( pmtOut == NULL )
				return S_OK;
			if ( IsEqualGUID( &pmtOut->subtype, phandler->psubtypeOut ) )
				return S_OK;
		}
		phandler ++;
	}

	return E_FAIL;
}

static HRESULT ColorConv_GetOutputTypes( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE** ppmtAcceptTypes, ULONG* pcAcceptTypes )
{
	CColorConvImpl*	This = pImpl->m_pUserData;
	HRESULT hr;
	const struct BltHandler*	phandler;
	DWORD	cConv;
	BITMAPINFOHEADER*	pbiIn = NULL;
	BITMAPINFOHEADER*	pbiOut = NULL;

	TRACE("(%p)\n",This);

	if ( This == NULL )
		return E_UNEXPECTED;

	hr = ColorConv_CheckMediaType( pImpl, pmtIn, NULL );
	if ( FAILED(hr) )
		return hr;
	pbiIn = (&((VIDEOINFOHEADER*)pmtIn->pbFormat)->bmiHeader);

	ColorConv_FreeOutTypes(This);

	cConv = 0;
	phandler = conv_handlers;
	while ( phandler->psubtypeIn != NULL )
	{
		if ( IsEqualGUID( &pmtIn->subtype, phandler->psubtypeIn ) )
			cConv ++;
		phandler ++;
	}

	This->m_cConv = cConv;
	This->m_pmtConv = (AM_MEDIA_TYPE*)QUARTZ_AllocMem(
		sizeof(AM_MEDIA_TYPE) * cConv );
	if ( This->m_pmtConv == NULL )
		return E_OUTOFMEMORY;
	ZeroMemory( This->m_pmtConv, sizeof(AM_MEDIA_TYPE) * cConv );

	cConv = 0;
	phandler = conv_handlers;
	while ( phandler->psubtypeIn != NULL )
	{
		if ( IsEqualGUID( &pmtIn->subtype, phandler->psubtypeIn ) )
		{
			memcpy( &This->m_pmtConv[cConv].majortype, &MEDIATYPE_Video, sizeof(GUID) );
			memcpy( &This->m_pmtConv[cConv].subtype, phandler->psubtypeOut, sizeof(GUID) );
			This->m_pmtConv[cConv].bFixedSizeSamples = 1;
			This->m_pmtConv[cConv].bTemporalCompression = 0;
			This->m_pmtConv[cConv].lSampleSize = DIBSIZE(*pbiIn);
			memcpy( &This->m_pmtConv[cConv].formattype, &FORMAT_VideoInfo, sizeof(GUID) );
			This->m_pmtConv[cConv].cbFormat = sizeof(VIDEOINFO);
			This->m_pmtConv[cConv].pbFormat = (BYTE*)CoTaskMemAlloc( This->m_pmtConv[cConv].cbFormat );
			if ( This->m_pmtConv[cConv].pbFormat == NULL )
				return E_OUTOFMEMORY;
			ZeroMemory( This->m_pmtConv[cConv].pbFormat, This->m_pmtConv[cConv].cbFormat );
			pbiOut = &(((VIDEOINFOHEADER*)(This->m_pmtConv[cConv].pbFormat))->bmiHeader);
			hr = ColorConv_FillBitmapInfo( (BITMAPINFO*)pbiOut, pbiIn->biWidth, pbiIn->biHeight, phandler->psubtypeOut );
			if ( FAILED(hr) )
				return hr;

			cConv ++;
		}
		phandler ++;
	}

	*ppmtAcceptTypes = This->m_pmtConv;
	*pcAcceptTypes = This->m_cConv;

	return NOERROR;
}

static HRESULT ColorConv_GetAllocProp( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE* pmtOut, ALLOCATOR_PROPERTIES* pProp, BOOL* pbTransInPlace, BOOL* pbTryToReuseSample )
{
	CColorConvImpl*	This = pImpl->m_pUserData;
	HRESULT hr;
	BITMAPINFOHEADER*	pbiOut = NULL;

	TRACE("(%p)\n",This);

	if ( This == NULL )
		return E_UNEXPECTED;

	hr = ColorConv_CheckMediaType( pImpl, pmtIn, pmtOut );
	if ( FAILED(hr) )
		return hr;

	pbiOut = (&((VIDEOINFOHEADER*)pmtOut->pbFormat)->bmiHeader);

	pProp->cBuffers = 1;
	pProp->cbBuffer = DIBSIZE(*pbiOut);
	TRACE("%ldx%ldx%u cbBuffer = %ld\n",pbiOut->biWidth,pbiOut->biHeight,(unsigned)pbiOut->biBitCount,pProp->cbBuffer);

	return NOERROR;
}

static HRESULT ColorConv_BeginTransform( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE* pmtOut, BOOL bReuseSample )
{
	CColorConvImpl*	This = pImpl->m_pUserData;
	HRESULT hr;
	BITMAPINFOHEADER*	pbiIn = NULL;
	BITMAPINFOHEADER*	pbiOut = NULL;
	const struct BltHandler*	phandler;

	TRACE("(%p)\n",This);

	if ( This == NULL )
		return E_UNEXPECTED;

	hr = ColorConv_CheckMediaType( pImpl, pmtIn, pmtOut );
	if ( FAILED(hr) )
		return hr;

	pbiIn = (&((VIDEOINFOHEADER*)pmtIn->pbFormat)->bmiHeader);
	pbiOut = (&((VIDEOINFOHEADER*)pmtOut->pbFormat)->bmiHeader);

	This->pitchIn = DIBWIDTHBYTES(*pbiIn);
	This->pitchOut = DIBWIDTHBYTES(*pbiOut);

	This->m_pBlt = NULL;
	phandler = conv_handlers;
	while ( phandler->psubtypeIn != NULL )
	{
		if ( IsEqualGUID( &pmtIn->subtype, phandler->psubtypeIn ) )
		{
			if ( IsEqualGUID( &pmtOut->subtype, phandler->psubtypeOut ) )
			{
				This->m_pBlt = phandler->pBlt;
				return S_OK;
			}
		}
		phandler ++;
	}

	return E_FAIL;
}

static HRESULT ColorConv_Transform( CTransformBaseImpl* pImpl, IMediaSample* pSampIn, IMediaSample* pSampOut )
{
	CColorConvImpl*	This = pImpl->m_pUserData;
	BYTE*	pDataIn = NULL;
	BYTE*	pDataOut = NULL;
	BITMAPINFO*	pbiIn;
	BITMAPINFO*	pbiOut;
	HRESULT hr;

	if ( This == NULL )
		return E_UNEXPECTED;

	hr = IMediaSample_GetPointer( pSampIn, &pDataIn );
	if ( FAILED(hr) )
		return hr;
	hr = IMediaSample_GetPointer( pSampOut, &pDataOut );
	if ( FAILED(hr) )
		return hr;

	if ( This->m_pBlt != NULL )
	{
		pbiIn = (BITMAPINFO*)&(((VIDEOINFOHEADER*)pImpl->pInPin->pin.pmtConn->pbFormat)->bmiHeader);
		pbiOut = (BITMAPINFO*)&(((VIDEOINFOHEADER*)pImpl->pOutPin->pin.pmtConn->pbFormat)->bmiHeader);
		This->m_pBlt(
			pDataOut, This->pitchOut,
			pDataIn, This->pitchIn,
			pbiIn->bmiHeader.biWidth,
			abs(pbiIn->bmiHeader.biHeight),
			&pbiIn->bmiColors[0], pbiIn->bmiHeader.biClrUsed );
		hr = IMediaSample_SetActualDataLength(pSampOut,DIBSIZE(pbiOut->bmiHeader));
		if ( FAILED(hr) )
			return hr;
	}

	return NOERROR;
}

static HRESULT ColorConv_EndTransform( CTransformBaseImpl* pImpl )
{
	CColorConvImpl*	This = pImpl->m_pUserData;

	if ( This == NULL )
		return E_UNEXPECTED;

	This->m_pBlt = NULL;

	return NOERROR;
}


static const TransformBaseHandlers transhandlers =
{
	ColorConv_Init,
	ColorConv_Cleanup,
	ColorConv_CheckMediaType,
	ColorConv_GetOutputTypes,
	ColorConv_GetAllocProp,
	ColorConv_BeginTransform,
	NULL,
	ColorConv_Transform,
	ColorConv_EndTransform,
};

HRESULT QUARTZ_CreateColour(IUnknown* punkOuter,void** ppobj)
{
	return QUARTZ_CreateTransformBase(
		punkOuter,ppobj,
		&CLSID_Colour,
		ColorConv_FilterName,
		NULL, NULL,
		&transhandlers );
}

