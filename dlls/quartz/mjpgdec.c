/*
 * Implements AVI MJPG Decompressor.
 *
 *	FIXME - stub
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
#include "ijgdec.h"

static const WCHAR MJPGDec_FilterName[] =
{'M','J','P','G',' ','D','e','c','o','m','p','r','e','s','s','o','r',0};

typedef struct CMJPGDecImpl
{
	AM_MEDIA_TYPE*	m_pmtConv;
	DWORD	m_cConv;
	BITMAPINFOHEADER	m_biOut;
} CMJPGDecImpl;

static const BYTE jpeg_standard_dht_data[0x1f+0xb5+0x1f+0xb5+2*4] =
{
	 0xff, 0xc4, 0x00, 0x1f, 0x00,
	 0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01,
	 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	 0x08, 0x09, 0x0a, 0x0b,

	 0xff, 0xc4, 0x00, 0xb5, 0x10,
	 0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03,
	 0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7d,
	 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
	 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
	 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
	 0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
	 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
	 0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
	 0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
	 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
	 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
	 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
	 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
	 0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
	 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
	 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
	 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
	 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
	 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
	 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
	 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	 0xf9, 0xfa,

	 0xff, 0xc4, 0x00, 0x1f, 0x01,
	 0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	 0x08, 0x09, 0x0a, 0x0b,

	 0xff, 0xc4, 0x00, 0xb5, 0x11,
	 0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04,
	 0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02, 0x77,
	 0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
	 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
	 0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
	 0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
	 0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
	 0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
	 0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
	 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
	 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
	 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
	 0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
	 0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
	 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
	 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
	 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
	 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
	 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
	 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
	 0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	 0xf9, 0xfa,
};

/***************************************************************************
 *
 *	CMJPGDecImpl methods
 *
 */

static void MJPGDec_FreeOutTypes(CMJPGDecImpl* This)
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


static HRESULT MJPGDec_Init( CTransformBaseImpl* pImpl )
{
	CMJPGDecImpl*	This = pImpl->m_pUserData;

	if ( This != NULL )
		return NOERROR;

	This = (CMJPGDecImpl*)QUARTZ_AllocMem( sizeof(CMJPGDecImpl) );
	if ( This == NULL )
		return E_OUTOFMEMORY;
	ZeroMemory( This, sizeof(CMJPGDecImpl) );
	pImpl->m_pUserData = This;
	/* construct */
	This->m_pmtConv = NULL;
	This->m_cConv = 0;

	return NOERROR;
}

static HRESULT MJPGDec_Cleanup( CTransformBaseImpl* pImpl )
{
	CMJPGDecImpl*	This = pImpl->m_pUserData;

	if ( This == NULL )
		return NOERROR;
	/* destruct */
	MJPGDec_FreeOutTypes(This);

	QUARTZ_FreeMem( This );
	pImpl->m_pUserData = NULL;

	return NOERROR;
}

static HRESULT MJPGDec_CheckMediaType( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE* pmtOut )
{
	CMJPGDecImpl*	This = pImpl->m_pUserData;
	const BITMAPINFOHEADER*	pbiIn = NULL;
	const BITMAPINFOHEADER*	pbiOut = NULL;

	TRACE("(%p)\n",This);

	if ( This == NULL )
		return E_UNEXPECTED;

	if ( !IsEqualGUID( &pmtIn->majortype, &MEDIATYPE_Video ) )
		return E_FAIL;
	if ( !IsEqualGUID( &pmtIn->formattype, &FORMAT_VideoInfo ) )
		return E_FAIL;
	pbiIn = (&((const VIDEOINFOHEADER*)pmtIn->pbFormat)->bmiHeader);
	if ( pbiIn->biCompression != mmioFOURCC('M','J','P','G') &&
		 pbiIn->biCompression != mmioFOURCC('m','j','p','g') )
		return E_FAIL;
	if ( pbiIn->biBitCount != 24 )
		return E_FAIL;

	if ( pmtOut != NULL )
	{
		if ( !IsEqualGUID( &pmtOut->majortype, &MEDIATYPE_Video ) )
			return E_FAIL;
		if ( !IsEqualGUID( &pmtOut->formattype, &FORMAT_VideoInfo ) )
			return E_FAIL;
		if ( !IsEqualGUID( &pmtOut->subtype, &MEDIASUBTYPE_RGB24 ) )
			return E_FAIL;
		pbiOut = (&((const VIDEOINFOHEADER*)pmtOut->pbFormat)->bmiHeader);
		if ( pbiOut->biCompression != 0 )
			return E_FAIL;
		if ( pbiIn->biWidth != pbiOut->biWidth ||
			 pbiIn->biHeight != pbiOut->biHeight ||
			 pbiIn->biPlanes != 1 || pbiOut->biPlanes != 1 ||
			 pbiOut->biBitCount != pbiIn->biBitCount )
			return E_FAIL;
	}

	return S_OK;
}

static HRESULT MJPGDec_GetOutputTypes( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE** ppmtAcceptTypes, ULONG* pcAcceptTypes )
{
	CMJPGDecImpl*	This = pImpl->m_pUserData;
	HRESULT hr;
	DWORD dwIndex;
	const BITMAPINFOHEADER*	pbiIn = NULL;
	BITMAPINFOHEADER*	pbiOut = NULL;

	TRACE("(%p)\n",This);

	if ( This == NULL )
		return E_UNEXPECTED;

	hr = MJPGDec_CheckMediaType( pImpl, pmtIn, NULL );
	if ( FAILED(hr) )
		return hr;
	pbiIn = (&((const VIDEOINFOHEADER*)pmtIn->pbFormat)->bmiHeader);

	MJPGDec_FreeOutTypes(This);

	This->m_cConv = 1;
	This->m_pmtConv = (AM_MEDIA_TYPE*)QUARTZ_AllocMem( sizeof(AM_MEDIA_TYPE) );
	if ( This->m_pmtConv == NULL )
		return E_OUTOFMEMORY;
	ZeroMemory( This->m_pmtConv, sizeof(AM_MEDIA_TYPE) * 1 );

	dwIndex = 0;

	memcpy( &This->m_pmtConv[dwIndex].majortype, &MEDIATYPE_Video, sizeof(GUID) );
	memcpy( &This->m_pmtConv[dwIndex].subtype, &MEDIASUBTYPE_RGB24, sizeof(GUID) );
	This->m_pmtConv[dwIndex].bFixedSizeSamples = 1;
	This->m_pmtConv[dwIndex].bTemporalCompression = 0;
	This->m_pmtConv[dwIndex].lSampleSize = DIBSIZE(*pbiIn);
	memcpy( &This->m_pmtConv[dwIndex].formattype, &FORMAT_VideoInfo, sizeof(GUID) );
	This->m_pmtConv[dwIndex].cbFormat = sizeof(VIDEOINFO);
	This->m_pmtConv[dwIndex].pbFormat = (BYTE*)CoTaskMemAlloc( This->m_pmtConv[dwIndex].cbFormat );
	if ( This->m_pmtConv[dwIndex].pbFormat == NULL )
		return E_OUTOFMEMORY;
	ZeroMemory( This->m_pmtConv[dwIndex].pbFormat, This->m_pmtConv[dwIndex].cbFormat );
	pbiOut = &(((VIDEOINFOHEADER*)(This->m_pmtConv[dwIndex].pbFormat))->bmiHeader);
	pbiOut->biSize = sizeof(BITMAPINFOHEADER);
	pbiOut->biWidth = pbiIn->biWidth;
	pbiOut->biHeight = pbiIn->biHeight;
	pbiOut->biPlanes = 1;
	pbiOut->biBitCount = pbiIn->biBitCount;

	*ppmtAcceptTypes = This->m_pmtConv;
	*pcAcceptTypes = This->m_cConv;

	return S_OK;
}

static HRESULT MJPGDec_GetAllocProp( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE* pmtOut, ALLOCATOR_PROPERTIES* pProp, BOOL* pbTransInPlace, BOOL* pbTryToReuseSample )
{
	CMJPGDecImpl*	This = pImpl->m_pUserData;
	HRESULT hr;
	const BITMAPINFOHEADER*	pbiOut = NULL;

	TRACE("(%p)\n",This);

	if ( This == NULL )
		return E_UNEXPECTED;

	hr = MJPGDec_CheckMediaType( pImpl, pmtIn, NULL );
	if ( FAILED(hr) )
		return hr;
	pbiOut = (&((const VIDEOINFOHEADER*)pmtOut->pbFormat)->bmiHeader);

	pProp->cBuffers = 1;
	pProp->cbBuffer = DIBSIZE(*pbiOut);

	return S_OK;
}

static HRESULT MJPGDec_BeginTransform( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE* pmtOut, BOOL bReuseSample )
{
	CMJPGDecImpl*	This = pImpl->m_pUserData;
	HRESULT hr;
	const BITMAPINFOHEADER*	pbiOut = NULL;

	TRACE("(%p)\n",This);

	if ( This == NULL )
		return E_UNEXPECTED;

	hr = MJPGDec_CheckMediaType( pImpl, pmtIn, NULL );
	if ( FAILED(hr) )
		return hr;

	ZeroMemory( &This->m_biOut, sizeof(BITMAPINFOHEADER) );

	pbiOut = (&((const VIDEOINFOHEADER*)pmtOut->pbFormat)->bmiHeader);
	memcpy( &This->m_biOut, pbiOut, sizeof(BITMAPINFOHEADER) );

	return S_OK;
}

static HRESULT MJPGDec_Transform( CTransformBaseImpl* pImpl, IMediaSample* pSampIn, IMediaSample* pSampOut )
{
	CMJPGDecImpl*	This = pImpl->m_pUserData;
	BYTE*	pDataIn = NULL;
	BYTE*	pDataOut = NULL;
	LONG	lDataInLen;
	LONG	outpitch;
	HRESULT hr;
	const char*	psrcs[3];
	int	lenofsrcs[3];

	TRACE("(%p)\n",This);

	if ( This == NULL )
		return E_UNEXPECTED;

	hr = IMediaSample_GetPointer( pSampIn, &pDataIn );
	if ( FAILED(hr) )
		return hr;
	lDataInLen = IMediaSample_GetActualDataLength( pSampIn );
	if ( lDataInLen < 4 )
		return E_FAIL;
	hr = IMediaSample_GetPointer( pSampOut, &pDataOut );
	if ( FAILED(hr) )
		return hr;
	IMediaSample_SetActualDataLength( pSampOut, DIBSIZE(This->m_biOut) );

	if ( pDataIn[0] != 0xff || pDataIn[1] != 0xd8 )
		return E_FAIL;

	psrcs[0] = (const char*)&pDataIn[0];
	psrcs[1] = (const char*)jpeg_standard_dht_data;
	psrcs[2] = (const char*)&pDataIn[2];
	lenofsrcs[0] = 2;
	lenofsrcs[1] = sizeof(jpeg_standard_dht_data);
	lenofsrcs[2] = lDataInLen - 2;

	outpitch = DIBWIDTHBYTES(This->m_biOut);
	if ( 0 > IJGDEC_Decode(
		pDataOut + (This->m_biOut.biHeight-1) * outpitch, - outpitch,
		This->m_biOut.biWidth, This->m_biOut.biHeight,
		This->m_biOut.biBitCount,
		&psrcs[0], &lenofsrcs[0], 3 ) )
	{
		return E_FAIL;
	}

	return S_OK;
}

static HRESULT MJPGDec_EndTransform( CTransformBaseImpl* pImpl )
{
	CMJPGDecImpl*	This = pImpl->m_pUserData;

	TRACE("(%p)\n",This);

	if ( This == NULL )
		return E_UNEXPECTED;

	ZeroMemory( &This->m_biOut, sizeof(BITMAPINFOHEADER) );

	return S_OK;
}


static const TransformBaseHandlers transhandlers =
{
	MJPGDec_Init,
	MJPGDec_Cleanup,
	MJPGDec_CheckMediaType,
	MJPGDec_GetOutputTypes,
	MJPGDec_GetAllocProp,
	MJPGDec_BeginTransform,
	NULL,
	MJPGDec_Transform,
	MJPGDec_EndTransform,
};

HRESULT QUARTZ_CreateMJPGDecompressor(IUnknown* punkOuter,void** ppobj)
{
	return QUARTZ_CreateTransformBase(
		punkOuter,ppobj,
		&CLSID_quartzMJPGDecompressor,
		MJPGDec_FilterName,
		NULL, NULL,
		&transhandlers );
}


