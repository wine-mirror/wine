/*
 * Implements MPEG Audio Decoder(CLSID_CMpegAudioCodec)
 *
 *	FIXME - what library can we use? SMPEG??
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
#include "mmsystem.h"
#include "mmreg.h"
#include "strmif.h"
#include "control.h"
#include "amvideo.h"
#include "vfwmsgs.h"
#include "uuids.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "xform.h"
#include "mtype.h"

static const WCHAR CMPEGAudioDecoderImpl_FilterName[] =
{'M','P','E','G',' ','A','u','d','i','o',' ','D','e','c','o','d','e','r',0};


typedef struct CMPEGAudioDecoderImpl
{
	AM_MEDIA_TYPE*	pmt;
	DWORD		cmt;
	WAVEFORMATEX	wfxOut;

	/* codec stuffs */

} CMPEGAudioDecoderImpl;


/*****************************************************************************
 *
 *	codec-dependent stuffs	- no codec
 *
 */

#define	NO_CODEC_IMPL

static void Codec_OnConstruct(CMPEGAudioDecoderImpl* This)
{
}

static void Codec_OnCleanup(CMPEGAudioDecoderImpl* This)
{
}

static HRESULT Codec_BeginTransform(CTransformBaseImpl* pImpl,CMPEGAudioDecoderImpl* This)
{
	FIXME("no codec\n");
	return E_NOTIMPL;
}

static HRESULT Codec_ProcessReceive(CTransformBaseImpl* pImpl,CMPEGAudioDecoderImpl* This,IMediaSample* pSampIn)
{
	FIXME("no codec\n");
	return E_NOTIMPL;
}

static HRESULT Codec_EndTransform(CTransformBaseImpl* pImpl,CMPEGAudioDecoderImpl* This)
{
	FIXME("no codec\n");
	return E_NOTIMPL;
}



/***************************************************************************
 *
 *	CMPEGAudioDecoderImpl methods
 *
 */

static void CMPEGAudioDecoderImpl_CleanupOutTypes(CMPEGAudioDecoderImpl* This)
{
	DWORD	i;

	if ( This->pmt != NULL )
	{
		for ( i = 0; i < This->cmt; i++ )
		{
			QUARTZ_MediaType_Free(&This->pmt[i]);
		}
		QUARTZ_FreeMem(This->pmt);
		This->pmt = NULL;
	}
	This->cmt = 0;
}

static HRESULT CMPEGAudioDecoderImpl_Init( CTransformBaseImpl* pImpl )
{
	CMPEGAudioDecoderImpl*	This = (CMPEGAudioDecoderImpl*)pImpl->m_pUserData;

	TRACE("(%p)\n",This);

	if ( This != NULL )
		return NOERROR;

	This = (CMPEGAudioDecoderImpl*)QUARTZ_AllocMem( sizeof(CMPEGAudioDecoderImpl) );
	if ( This == NULL )
		return E_OUTOFMEMORY;
	ZeroMemory( This, sizeof(CMPEGAudioDecoderImpl) );
	pImpl->m_pUserData = This;

	/* construct */
	This->pmt = NULL;
	This->cmt = 0;
	Codec_OnConstruct(This);

	return S_OK;
}

static HRESULT CMPEGAudioDecoderImpl_Cleanup( CTransformBaseImpl* pImpl )
{
	CMPEGAudioDecoderImpl*	This = (CMPEGAudioDecoderImpl*)pImpl->m_pUserData;

	TRACE("(%p)\n",This);

	if ( This == NULL )
		return S_OK;

	/* destruct */
	Codec_OnCleanup(This);
	CMPEGAudioDecoderImpl_CleanupOutTypes(This);

	QUARTZ_FreeMem( This );
	pImpl->m_pUserData = NULL;

	return S_OK;
}

static HRESULT CMPEGAudioDecoderImpl_CheckMediaType( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE* pmtOut )
{
	CMPEGAudioDecoderImpl*	This = (CMPEGAudioDecoderImpl*)pImpl->m_pUserData;
	const WAVEFORMATEX* pwfxIn;
	const WAVEFORMATEX* pwfxOut;

	TRACE("(%p)\n",This);
	if ( This == NULL )
		return E_UNEXPECTED;

	if ( !IsEqualGUID( &pmtIn->majortype, &MEDIATYPE_Audio ) )
		return E_FAIL;
	if ( !IsEqualGUID( &pmtIn->formattype, &FORMAT_WaveFormatEx ) )
		return E_FAIL;

	if ( pmtIn->pbFormat == NULL ||
	     pmtIn->cbFormat < sizeof(WAVEFORMATEX) )
		return E_FAIL;
	pwfxIn = (const WAVEFORMATEX*)pmtIn->pbFormat;
	if ( pwfxIn->wFormatTag != WAVE_FORMAT_MPEG &&
	     pwfxIn->wFormatTag != WAVE_FORMAT_MPEGLAYER3 )
		return E_FAIL;
	if ( pwfxIn->nChannels != 1 && pwfxIn->nChannels != 2 )
		return E_FAIL;
	if ( pwfxIn->nBlockAlign < 1 )
		return E_FAIL;

	if ( pmtOut != NULL )
	{
		if ( !IsEqualGUID( &pmtOut->majortype, &MEDIATYPE_Audio ) )
			return E_FAIL;
		if ( !IsEqualGUID( &pmtOut->formattype, &FORMAT_WaveFormatEx ) )
			return E_FAIL;

		if ( pmtOut->pbFormat == NULL ||
		     pmtOut->cbFormat < sizeof(WAVEFORMATEX) )
			return E_FAIL;
		pwfxOut = (const WAVEFORMATEX*)pmtOut->pbFormat;

		if ( pwfxOut->wFormatTag != WAVE_FORMAT_PCM )
			return E_FAIL;
		if ( pwfxOut->nChannels != pwfxIn->nChannels ||
		     pwfxOut->nSamplesPerSec != pwfxIn->nSamplesPerSec )
			return E_FAIL;
		if ( pwfxOut->wBitsPerSample != 16 )
			return E_FAIL;
		if ( pwfxOut->nBlockAlign != (pwfxOut->nChannels * pwfxOut->wBitsPerSample >> 3 ) )
			return E_FAIL;
	}

#ifdef	NO_CODEC_IMPL
	WARN("no codec implementation\n");
	return E_NOTIMPL;
#else
	return S_OK;
#endif
}

static HRESULT CMPEGAudioDecoderImpl_GetOutputTypes( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE** ppmtAcceptTypes, ULONG* pcAcceptTypes )
{
	CMPEGAudioDecoderImpl*	This = (CMPEGAudioDecoderImpl*)pImpl->m_pUserData;
	HRESULT hr;
	const WAVEFORMATEX*	pwfxIn;
	AM_MEDIA_TYPE*	pmtOut;
	WAVEFORMATEX*	pwfxOut;

	TRACE("(%p)\n",This);
	if ( This == NULL )
		return E_UNEXPECTED;

	hr = CMPEGAudioDecoderImpl_CheckMediaType( pImpl, pmtIn, NULL );
	if ( FAILED(hr) )
		return hr;
	pwfxIn = (const WAVEFORMATEX*)pmtIn->pbFormat;

	CMPEGAudioDecoderImpl_CleanupOutTypes(This);

	This->cmt = 1;
	This->pmt = (AM_MEDIA_TYPE*)QUARTZ_AllocMem(
		sizeof(AM_MEDIA_TYPE) * This->cmt );
	if ( This->pmt == NULL )
		return E_OUTOFMEMORY;
	ZeroMemory( This->pmt, sizeof(AM_MEDIA_TYPE) * This->cmt );

	pmtOut = &This->pmt[0];

	memcpy( &pmtOut->majortype, &MEDIATYPE_Audio, sizeof(GUID) );
	memcpy( &pmtOut->subtype, &MEDIASUBTYPE_PCM, sizeof(GUID) );
	memcpy( &pmtOut->formattype, &FORMAT_WaveFormatEx, sizeof(GUID) );
	pmtOut->bFixedSizeSamples = 1;
	pmtOut->bTemporalCompression = 0;
	pmtOut->lSampleSize = pwfxIn->nChannels * 16 >> 3;
	pmtOut->pbFormat = (BYTE*)CoTaskMemAlloc( sizeof(WAVEFORMATEX) );
	if ( pmtOut->pbFormat == NULL )
		return E_OUTOFMEMORY;
	pwfxOut = (WAVEFORMATEX*)pmtOut->pbFormat;
	pmtOut->cbFormat = sizeof(WAVEFORMATEX);
	pwfxOut->wFormatTag = WAVE_FORMAT_PCM;
	pwfxOut->nChannels = pwfxIn->nChannels;
	pwfxOut->nSamplesPerSec = pwfxIn->nSamplesPerSec;
	pwfxOut->nAvgBytesPerSec = pwfxOut->nSamplesPerSec * pmtOut->lSampleSize;
	pwfxOut->nBlockAlign = pmtOut->lSampleSize;
	pwfxOut->wBitsPerSample = 16;
	pwfxOut->cbSize = 0;

	*ppmtAcceptTypes = This->pmt;
	*pcAcceptTypes = This->cmt;

	return S_OK;
}

static HRESULT CMPEGAudioDecoderImpl_GetAllocProp( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE* pmtOut, ALLOCATOR_PROPERTIES* pProp, BOOL* pbTransInPlace, BOOL* pbTryToReuseSample )
{
	CMPEGAudioDecoderImpl*	This = (CMPEGAudioDecoderImpl*)pImpl->m_pUserData;
	const WAVEFORMATEX*	pwfxIn;
	const WAVEFORMATEX*	pwfxOut;
	HRESULT hr;

	TRACE("(%p)\n",This);
	if ( This == NULL )
		return E_UNEXPECTED;

	hr = CMPEGAudioDecoderImpl_CheckMediaType( pImpl, pmtIn, pmtOut );
	if ( FAILED(hr) )
		return hr;
	pwfxIn = (const WAVEFORMATEX*)pmtIn->pbFormat;
	pwfxOut = (const WAVEFORMATEX*)pmtOut->pbFormat;

	pProp->cBuffers = 1;
	pProp->cbBuffer = pwfxOut->nAvgBytesPerSec;

	TRACE("cbBuffer %ld\n",pProp->cbBuffer);

	*pbTransInPlace = FALSE;
	*pbTryToReuseSample = FALSE;

	return S_OK;
}

static HRESULT CMPEGAudioDecoderImpl_BeginTransform( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE* pmtOut, BOOL bReuseSample )
{
	CMPEGAudioDecoderImpl*	This = (CMPEGAudioDecoderImpl*)pImpl->m_pUserData;
	HRESULT hr;

	TRACE("(%p,%p,%p,%d)\n",This,pmtIn,pmtOut,bReuseSample);
	if ( This == NULL )
		return E_UNEXPECTED;

	hr = CMPEGAudioDecoderImpl_CheckMediaType( pImpl, pmtIn, pmtOut );
	if ( FAILED(hr) )
		return hr;
	memcpy( &This->wfxOut, (const WAVEFORMATEX*)pmtOut->pbFormat, sizeof(WAVEFORMATEX) );

	return Codec_BeginTransform(pImpl,This);
}

static HRESULT CMPEGAudioDecoderImpl_ProcessReceive( CTransformBaseImpl* pImpl, IMediaSample* pSampIn )
{
	CMPEGAudioDecoderImpl*	This = (CMPEGAudioDecoderImpl*)pImpl->m_pUserData;

	TRACE("(%p,%p)\n",This,pSampIn);
	if ( This == NULL )
		return E_UNEXPECTED;

	return Codec_ProcessReceive(pImpl,This,pSampIn);
}

static HRESULT CMPEGAudioDecoderImpl_EndTransform( CTransformBaseImpl* pImpl )
{
	CMPEGAudioDecoderImpl*	This = (CMPEGAudioDecoderImpl*)pImpl->m_pUserData;
	HRESULT hr;

	TRACE("(%p)\n",This);
	if ( This == NULL )
		return E_UNEXPECTED;

	hr = Codec_EndTransform(pImpl,This);
	if ( FAILED(hr) )
		return hr;
	ZeroMemory( &This->wfxOut, sizeof(WAVEFORMATEX) );

	return S_OK;
}


static const TransformBaseHandlers transhandlers =
{
	CMPEGAudioDecoderImpl_Init,
	CMPEGAudioDecoderImpl_Cleanup,
	CMPEGAudioDecoderImpl_CheckMediaType,
	CMPEGAudioDecoderImpl_GetOutputTypes,
	CMPEGAudioDecoderImpl_GetAllocProp,
	CMPEGAudioDecoderImpl_BeginTransform,
	CMPEGAudioDecoderImpl_ProcessReceive,
	NULL,
	CMPEGAudioDecoderImpl_EndTransform,
};

HRESULT QUARTZ_CreateCMpegAudioCodec(IUnknown* punkOuter,void** ppobj)
{
	return QUARTZ_CreateTransformBase(
		punkOuter,ppobj,
		&CLSID_CMpegAudioCodec,
		CMPEGAudioDecoderImpl_FilterName,
		NULL, NULL,
		&transhandlers );
}

