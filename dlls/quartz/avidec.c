/*
 * Implements AVI Decompressor(CLSID_AVIDec).
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


static const WCHAR AVIDec_FilterName[] =
{'A','V','I',' ','D','e','c','o','m','p','r','e','s','s','o','r',0};

typedef struct CAVIDecImpl
{
	HIC hicCached;
	HIC hicTrans;
	AM_MEDIA_TYPE m_mtOut;
	BITMAPINFO* m_pbiIn;
	BITMAPINFO* m_pbiOut;
	BYTE* m_pOutBuf;
} CAVIDecImpl;

/***************************************************************************
 *
 *	CAVIDecImpl methods
 *
 */

static void AVIDec_ReleaseDIBBuffers(CAVIDecImpl* This)
{
	TRACE("(%p)\n",This);

	if ( This->m_pbiIn != NULL )
	{
		QUARTZ_FreeMem(This->m_pbiIn); This->m_pbiIn = NULL;
	}
	if ( This->m_pbiOut != NULL )
	{
		QUARTZ_FreeMem(This->m_pbiOut); This->m_pbiOut = NULL;
	}
	if ( This->m_pOutBuf != NULL )
	{
		QUARTZ_FreeMem(This->m_pOutBuf); This->m_pOutBuf = NULL;
	}
}

static BITMAPINFO* AVIDec_DuplicateBitmapInfo(const BITMAPINFO* pbi)
{
	DWORD dwSize;
	BITMAPINFO*	pbiRet;

	dwSize = pbi->bmiHeader.biSize;
	if ( dwSize < sizeof(BITMAPINFOHEADER) )
		return NULL;
	if ( pbi->bmiHeader.biBitCount <= 8 )
	{
		if ( pbi->bmiHeader.biClrUsed == 0 )
			dwSize += sizeof(RGBQUAD)*(1<<pbi->bmiHeader.biBitCount);
		else
			dwSize += sizeof(RGBQUAD)*pbi->bmiHeader.biClrUsed;
	}
	if ( pbi->bmiHeader.biCompression == 3 &&
		 dwSize == sizeof(BITMAPINFOHEADER) )
		dwSize += sizeof(DWORD)*3;

	pbiRet = (BITMAPINFO*)QUARTZ_AllocMem(dwSize);
	if ( pbiRet != NULL )
		memcpy( pbiRet, pbi, dwSize );

	return pbiRet;
}

static HRESULT AVIDec_Init( CTransformBaseImpl* pImpl )
{
	CAVIDecImpl*	This = pImpl->m_pUserData;

	TRACE("(%p)\n",This);

	if ( This != NULL )
		return NOERROR;

	This = (CAVIDecImpl*)QUARTZ_AllocMem( sizeof(CAVIDecImpl) );
	if ( This == NULL )
		return E_OUTOFMEMORY;
	ZeroMemory( This, sizeof(CAVIDecImpl) );
	pImpl->m_pUserData = This;
	/* construct */
	This->hicCached = (HIC)NULL;
	This->hicTrans = (HIC)NULL;
	ZeroMemory( &This->m_mtOut, sizeof(AM_MEDIA_TYPE) );
	This->m_pbiIn = NULL;
	This->m_pbiOut = NULL;
	This->m_pOutBuf = NULL;

	return NOERROR;
}

static HRESULT AVIDec_Cleanup( CTransformBaseImpl* pImpl )
{
	CAVIDecImpl*	This = pImpl->m_pUserData;

	TRACE("(%p)\n",This);

	if ( This == NULL )
		return NOERROR;

	/* destruct */
	QUARTZ_MediaType_Free( &This->m_mtOut );

	AVIDec_ReleaseDIBBuffers(This);

	if ( This->hicCached != (HIC)NULL )
		ICClose(This->hicCached);
	if ( This->hicTrans != (HIC)NULL )
		ICClose(This->hicTrans);

	QUARTZ_FreeMem( This );
	pImpl->m_pUserData = NULL;

	return NOERROR;
}

static HRESULT AVIDec_CheckMediaType( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE* pmtOut )
{
	CAVIDecImpl*	This = pImpl->m_pUserData;
	BITMAPINFO*	pbiIn = NULL;
	BITMAPINFO*	pbiOut = NULL;
	HIC	hic;

	TRACE("(%p)\n",This);
	if ( This == NULL )
		return E_UNEXPECTED;

	if ( !IsEqualGUID( &pmtIn->majortype, &MEDIATYPE_Video ) )
		return E_FAIL;
	if ( !IsEqualGUID( &pmtIn->formattype, &FORMAT_VideoInfo ) )
		return E_FAIL;
	pbiIn = (BITMAPINFO*)(&((VIDEOINFOHEADER*)pmtIn->pbFormat)->bmiHeader);
	if ( pmtOut != NULL )
	{
		if ( !IsEqualGUID( &pmtOut->majortype, &MEDIATYPE_Video ) )
			return E_FAIL;
		if ( !IsEqualGUID( &pmtOut->formattype, &FORMAT_VideoInfo ) )
			return E_FAIL;
		pbiOut = (BITMAPINFO*)(&((VIDEOINFOHEADER*)pmtOut->pbFormat)->bmiHeader);
	}

	if ( This->hicCached != (HIC)NULL &&
		 ICDecompressQuery( This->hicCached, pbiIn, pbiOut ) == ICERR_OK )
	{
		TRACE("supported format\n");
		return NOERROR;
	}

	TRACE("try to find a decoder...\n");
	hic = ICLocate(
		mmioFOURCC('V','I','D','C'), 0,
		&pbiIn->bmiHeader, &pbiOut->bmiHeader, ICMODE_DECOMPRESS );
	if ( hic == (HIC)NULL )
	{
		WARN("no decoder for %c%c%c%c\n",
			(int)(( pbiIn->bmiHeader.biCompression >>  0 ) & 0xff),
			(int)(( pbiIn->bmiHeader.biCompression >>  8 ) & 0xff),
			(int)(( pbiIn->bmiHeader.biCompression >> 16 ) & 0xff),
			(int)(( pbiIn->bmiHeader.biCompression >> 24 ) & 0xff) );
		return E_FAIL;
	}
	TRACE("found\n");

	if ( This->hicCached != (HIC)NULL )
		ICClose(This->hicCached);
	This->hicCached = hic;

	return NOERROR;
}

static HRESULT AVIDec_GetOutputTypes( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE** ppmtAcceptTypes, ULONG* pcAcceptTypes )
{
	CAVIDecImpl*	This = pImpl->m_pUserData;
	HRESULT hr;
	LONG cbFmt;
	BITMAPINFO*	pbiIn = NULL;
	BITMAPINFO*	pbiOut = NULL;

	TRACE("(%p)\n",This);
	hr = AVIDec_CheckMediaType( pImpl, pmtIn, NULL );
	if ( FAILED(hr) )
		return hr;

	TRACE("(%p) - get size of format\n",This);
	pbiIn = (BITMAPINFO*)(&((VIDEOINFOHEADER*)pmtIn->pbFormat)->bmiHeader);
	cbFmt = (LONG)ICDecompressGetFormatSize( This->hicCached, pbiIn );
	if ( cbFmt < sizeof(BITMAPINFOHEADER) )
		return E_FAIL;

	QUARTZ_MediaType_Free( &This->m_mtOut );
	ZeroMemory( &This->m_mtOut, sizeof(AM_MEDIA_TYPE) );

	memcpy( &This->m_mtOut.majortype, &MEDIATYPE_Video, sizeof(GUID) );
	memcpy( &This->m_mtOut.formattype, &FORMAT_VideoInfo, sizeof(GUID) );
	This->m_mtOut.cbFormat = sizeof(VIDEOINFOHEADER) + cbFmt + sizeof(RGBQUAD)*256;
	This->m_mtOut.pbFormat = (BYTE*)CoTaskMemAlloc(This->m_mtOut.cbFormat);
	if ( This->m_mtOut.pbFormat == NULL )
		return E_OUTOFMEMORY;
	ZeroMemory( This->m_mtOut.pbFormat, This->m_mtOut.cbFormat );

	pbiOut = (BITMAPINFO*)(&((VIDEOINFOHEADER*)This->m_mtOut.pbFormat)->bmiHeader);

	TRACE("(%p) - get format\n",This);
	if ( ICDecompressGetFormat( This->hicCached, pbiIn, pbiOut ) != ICERR_OK )
		return E_FAIL;

	hr = QUARTZ_MediaSubType_FromBitmap( &This->m_mtOut.subtype, &pbiOut->bmiHeader );
	if ( FAILED(hr) )
		return hr;
	if ( hr != S_OK )
		QUARTZ_MediaSubType_FromFourCC( &This->m_mtOut.subtype, pbiOut->bmiHeader.biCompression );

	This->m_mtOut.bFixedSizeSamples = (pbiOut->bmiHeader.biCompression == 0) ? 1 : 0;
	This->m_mtOut.lSampleSize = (pbiOut->bmiHeader.biCompression == 0) ? DIBSIZE(pbiOut->bmiHeader) : pbiOut->bmiHeader.biSizeImage;

	/* get palette */
	if ( pbiOut->bmiHeader.biBitCount <= 8 )
	{
		TRACE("(%p) - get palette\n",This);
		if ( ICDecompressGetPalette( This->hicCached, pbiIn, pbiOut ) != ICERR_OK )
		{
			TRACE("(%p) - use the input palette\n",This);
			if ( pbiIn->bmiHeader.biBitCount != pbiOut->bmiHeader.biBitCount )
			{
				FIXME( "no palette...FIXME?\n" );
				return E_FAIL;
			}
			if ( pbiOut->bmiHeader.biClrUsed == 0 )
				pbiOut->bmiHeader.biClrUsed = 1<<pbiOut->bmiHeader.biBitCount;
			if ( pbiOut->bmiHeader.biClrUsed > (1<<pbiOut->bmiHeader.biBitCount) )
			{
				ERR( "biClrUsed=%ld\n", pbiOut->bmiHeader.biClrUsed );
				return E_FAIL;
			}

			memcpy( pbiOut->bmiColors, pbiIn->bmiColors,
				sizeof(RGBQUAD) * pbiOut->bmiHeader.biClrUsed );
		}
	}

	TRACE("(%p) - return format\n",This);
	*ppmtAcceptTypes = &This->m_mtOut;
	*pcAcceptTypes = 1;

	return NOERROR;
}

static HRESULT AVIDec_GetAllocProp( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE* pmtOut, ALLOCATOR_PROPERTIES* pProp, BOOL* pbTransInPlace, BOOL* pbTryToReuseSample )
{
	CAVIDecImpl*	This = pImpl->m_pUserData;
	BITMAPINFO*	pbiOut = NULL;
	HRESULT hr;

	TRACE("(%p)\n",This);

	if ( This == NULL )
		return E_UNEXPECTED;

	hr = AVIDec_CheckMediaType( pImpl, pmtIn, pmtOut );
	if ( FAILED(hr) )
		return hr;

	pbiOut = (BITMAPINFO*)(&((VIDEOINFOHEADER*)pmtOut->pbFormat)->bmiHeader);

	pProp->cBuffers = 1;
	if ( pbiOut->bmiHeader.biCompression == 0 )
		pProp->cbBuffer = DIBSIZE(pbiOut->bmiHeader);
	else
		pProp->cbBuffer = pbiOut->bmiHeader.biSizeImage;

	*pbTransInPlace = FALSE;
	*pbTryToReuseSample = TRUE;

	return NOERROR;
}

static HRESULT AVIDec_BeginTransform( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE* pmtOut, BOOL bReuseSample )
{
	CAVIDecImpl*	This = pImpl->m_pUserData;
	BITMAPINFO*	pbiIn = NULL;
	BITMAPINFO*	pbiOut = NULL;
	HRESULT hr;

	TRACE("(%p,%p,%p,%d)\n",This,pmtIn,pmtOut,bReuseSample);

	if ( This == NULL ||
		 This->hicTrans != (HIC)NULL )
		return E_UNEXPECTED;

	hr = AVIDec_CheckMediaType( pImpl, pmtIn, pmtOut );
	if ( FAILED(hr) )
		return hr;

	AVIDec_ReleaseDIBBuffers(This);

	pbiIn = (BITMAPINFO*)(&((VIDEOINFOHEADER*)pmtIn->pbFormat)->bmiHeader);
	pbiOut = (BITMAPINFO*)(&((VIDEOINFOHEADER*)pmtOut->pbFormat)->bmiHeader);
	This->m_pbiIn = AVIDec_DuplicateBitmapInfo(pbiIn);
	This->m_pbiOut = AVIDec_DuplicateBitmapInfo(pbiOut);
	if ( This->m_pbiIn == NULL || This->m_pbiOut == NULL )
		return E_OUTOFMEMORY;
	if ( This->m_pbiOut->bmiHeader.biCompression == 0 || This->m_pbiOut->bmiHeader.biCompression == 3 )
		This->m_pbiOut->bmiHeader.biSizeImage = DIBSIZE(This->m_pbiOut->bmiHeader);

	if ( !bReuseSample )
	{
		This->m_pOutBuf = QUARTZ_AllocMem(This->m_pbiOut->bmiHeader.biSizeImage);
		if ( This->m_pOutBuf == NULL )
			return E_OUTOFMEMORY;
		ZeroMemory( This->m_pOutBuf, This->m_pbiOut->bmiHeader.biSizeImage );
	}

	if ( ICERR_OK != ICDecompressBegin(
		This->hicCached, This->m_pbiIn, This->m_pbiOut ) )
		return E_FAIL;

	This->hicTrans = This->hicCached;
	This->hicCached = (HIC)NULL;

	return NOERROR;
}

static HRESULT AVIDec_Transform( CTransformBaseImpl* pImpl, IMediaSample* pSampIn, IMediaSample* pSampOut )
{
	CAVIDecImpl*	This = pImpl->m_pUserData;
	DWORD	dwFlags;
	BYTE*	pDataIn = NULL;
	BYTE*	pDataOut = NULL;
	HRESULT hr;

	TRACE("(%p)\n",This);

	if ( This == NULL || pSampOut == NULL ||
		 This->hicTrans == (HIC)NULL ||
		 This->m_pbiIn == NULL ||
		 This->m_pbiOut == NULL )
		return E_UNEXPECTED;

	hr = IMediaSample_GetPointer( pSampIn, &pDataIn );
	if ( FAILED(hr) )
		return hr;
	hr = IMediaSample_GetPointer( pSampOut, &pDataOut );
	if ( FAILED(hr) )
		return hr;

	dwFlags = 0;
	/*** FIXME!!!
	 *
	 * if ( IMediaSample_IsSyncPoint(pSampIn) != S_OK )
	 *	dwFlags |= ICDECOMPRESS_NOTKEYFRAME;
	 ****/

	if ( IMediaSample_IsPreroll(pSampIn) == S_OK )
		dwFlags |= ICDECOMPRESS_PREROLL;

	if ( ICERR_OK != ICDecompress(
		This->hicTrans,
		dwFlags,
		&This->m_pbiIn->bmiHeader,
		pDataIn,
		&This->m_pbiOut->bmiHeader,
		( This->m_pOutBuf != NULL ) ? This->m_pOutBuf : pDataOut ) )
		return E_FAIL;

	if ( This->m_pOutBuf != NULL )
		memcpy( pDataOut, This->m_pOutBuf,
				This->m_pbiOut->bmiHeader.biSizeImage );

	return NOERROR;
}

static HRESULT AVIDec_EndTransform( CTransformBaseImpl* pImpl )
{
	CAVIDecImpl*	This = pImpl->m_pUserData;

	TRACE("(%p)\n",This);

	if ( This == NULL )
		return E_UNEXPECTED;
	if ( This->hicTrans == (HIC)NULL )
		return NOERROR;

	ICDecompressEnd(This->hicTrans);

	if ( This->hicCached != (HIC)NULL )
		ICClose(This->hicCached);
	This->hicCached = This->hicTrans;
	This->hicTrans = (HIC)NULL;

	AVIDec_ReleaseDIBBuffers(This);

	return NOERROR;
}


static const TransformBaseHandlers transhandlers =
{
	AVIDec_Init,
	AVIDec_Cleanup,
	AVIDec_CheckMediaType,
	AVIDec_GetOutputTypes,
	AVIDec_GetAllocProp,
	AVIDec_BeginTransform,
	AVIDec_Transform,
	AVIDec_EndTransform,
};


HRESULT QUARTZ_CreateAVIDec(IUnknown* punkOuter,void** ppobj)
{
	return QUARTZ_CreateTransformBase(
		punkOuter,ppobj,
		&CLSID_AVIDec,
		AVIDec_FilterName,
		NULL, NULL,
		&transhandlers );
}

