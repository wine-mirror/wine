/*
 * Implements ACM Wrapper(CLSID_ACMWrapper).
 *
 *	FIXME - stub
 *	FIXME - no encoding
 *
 * Copyright (C) 2002 Hidenori TAKESHIMA <hidenori@a2.ctktv.ne.jp>
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
#include "msacm.h"
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


static const WCHAR ACMWrapper_FilterName[] =
{'A','C','M',' ','W','r','a','p','p','e','r',0};


typedef struct CACMWrapperImpl
{
	HACMSTREAM	has;
	WAVEFORMATEX*	pwfxIn;
	AM_MEDIA_TYPE*	pmtOuts;
	DWORD		cOuts;
	BYTE*		pConvBuf;
	DWORD		cbConvBlockSize;
	DWORD		cbConvCached;
	DWORD		cbConvAllocated;
} CACMWrapperImpl;


static
void ACMWrapper_CleanupMTypes( CACMWrapperImpl* This )
{
	DWORD	n;

	if ( This->pmtOuts == NULL ) return;
	for ( n = 0; n < This->cOuts; n++ )
	{
		QUARTZ_MediaType_Free( &This->pmtOuts[n] );
	}
	QUARTZ_FreeMem( This->pmtOuts );
	This->pmtOuts = NULL;
	This->cOuts = 0;
}

static
void ACMWrapper_CleanupConvBuf( CACMWrapperImpl* This )
{
	if ( This->pConvBuf != NULL )
	{
		QUARTZ_FreeMem( This->pConvBuf );
		This->pConvBuf = NULL;
	}
	This->cbConvBlockSize = 0;
	This->cbConvCached = 0;
	This->cbConvAllocated = 0;
}

static
const WAVEFORMATEX* ACMWrapper_GetAudioFmt( const AM_MEDIA_TYPE* pmt )
{
	const WAVEFORMATEX*	pwfx;

	if ( !IsEqualGUID( &pmt->majortype, &MEDIATYPE_Audio ) )
		return NULL;
	if ( !IsEqualGUID( &pmt->subtype, &MEDIASUBTYPE_NULL ) &&
		 !QUARTZ_MediaSubType_IsFourCC( &pmt->subtype ) )
		return NULL;
	if ( !IsEqualGUID( &pmt->formattype, &FORMAT_WaveFormatEx ) )
		return NULL;
	if ( pmt->pbFormat == NULL ||
		 pmt->cbFormat < (sizeof(WAVEFORMATEX)-sizeof(WORD)) )
		return NULL;

	pwfx = (const WAVEFORMATEX*)pmt->pbFormat;

	if ( pwfx->wFormatTag != 1 && pmt->cbFormat < sizeof(WAVEFORMATEX) )
		return NULL;

	return pwfx;
}

static
HRESULT ACMWrapper_SetupAudioFmt(
	AM_MEDIA_TYPE* pmt,
	DWORD cbFormat, WORD wFormatTag, DWORD dwBlockAlign )
{
	ZeroMemory( pmt, sizeof(AM_MEDIA_TYPE) );
	memcpy( &pmt->majortype, &MEDIATYPE_Audio, sizeof(GUID) );
	QUARTZ_MediaSubType_FromFourCC( &pmt->subtype, (DWORD)wFormatTag );
	pmt->bFixedSizeSamples = 1;
	pmt->bTemporalCompression = 1;
	pmt->lSampleSize = dwBlockAlign;
	memcpy( &pmt->formattype, &FORMAT_WaveFormatEx, sizeof(GUID) );
	pmt->pUnk = NULL;
	pmt->cbFormat = cbFormat;
	pmt->pbFormat = (BYTE*)CoTaskMemAlloc( cbFormat );
	if ( pmt->pbFormat == NULL )
		return E_OUTOFMEMORY;

	return S_OK;
}

static
void ACMWrapper_FillFmtPCM(
	WAVEFORMATEX* pwfxOut,
	const WAVEFORMATEX* pwfxIn,
	WORD wBitsPerSampOut )
{
	pwfxOut->wFormatTag = 1;
	pwfxOut->nChannels = pwfxIn->nChannels;
	pwfxOut->nSamplesPerSec = pwfxIn->nSamplesPerSec;
	pwfxOut->nAvgBytesPerSec = ((DWORD)pwfxIn->nSamplesPerSec * (DWORD)pwfxIn->nChannels * (DWORD)wBitsPerSampOut) >> 3;
	pwfxOut->nBlockAlign = (pwfxIn->nChannels * wBitsPerSampOut) >> 3;
	pwfxOut->wBitsPerSample = wBitsPerSampOut;
	pwfxOut->cbSize = 0;
}

static
BOOL ACMWrapper_IsSupported(
	WAVEFORMATEX* pwfxOut,
	WAVEFORMATEX* pwfxIn )
{
	MMRESULT	mr;

	mr = acmStreamOpen(
		NULL,(HACMDRIVER)NULL,
		pwfxIn,pwfxOut,NULL,
		0,0,ACM_STREAMOPENF_QUERY);
	if ( mr == ACMERR_NOTPOSSIBLE )
		mr = acmStreamOpen(
			NULL,(HACMDRIVER)NULL,
			pwfxIn,pwfxOut,NULL,
			0,0,ACM_STREAMOPENF_NONREALTIME|ACM_STREAMOPENF_QUERY);
	return !!(mr == MMSYSERR_NOERROR);
}

static
HRESULT ACMWrapper_StreamOpen(
	HACMSTREAM* phas,
	WAVEFORMATEX* pwfxOut,
	WAVEFORMATEX* pwfxIn )
{
	HACMSTREAM	has = (HACMSTREAM)NULL;
	MMRESULT	mr;

	mr = acmStreamOpen(
		&has,(HACMDRIVER)NULL,
		pwfxIn,pwfxOut,NULL,
		0,0,0);
	if ( mr == ACMERR_NOTPOSSIBLE )
		mr = acmStreamOpen(
			&has,(HACMDRIVER)NULL,
			pwfxIn,pwfxOut,NULL,
			0,0,ACM_STREAMOPENF_NONREALTIME);
	if ( mr != MMSYSERR_NOERROR )
	{
		if ( mr == MMSYSERR_NOMEM )
			return E_OUTOFMEMORY;
		return E_FAIL;
	}

	*phas = has;
	return S_OK;
}

/***************************************************************************
 *
 *	CACMWrapperImpl methods
 *
 */

static void ACMWrapper_Close( CACMWrapperImpl* This )
{
	if ( This->has != (HACMSTREAM)NULL )
	{
		acmStreamReset( This->has, 0 );
		acmStreamClose( This->has, 0 );
		This->has = (HACMSTREAM)NULL;
	}
}

static HRESULT ACMWrapper_Init( CTransformBaseImpl* pImpl )
{
	CACMWrapperImpl*	This = pImpl->m_pUserData;

	TRACE("(%p)\n",This);

	if ( This != NULL )
		return NOERROR;

	This = (CACMWrapperImpl*)QUARTZ_AllocMem( sizeof(CACMWrapperImpl) );
	if ( This == NULL )
		return E_OUTOFMEMORY;
	ZeroMemory( This, sizeof(CACMWrapperImpl) );
	pImpl->m_pUserData = This;

	/* construct */
	This->has = (HACMSTREAM)NULL;
	This->pwfxIn = NULL;
	This->pmtOuts = NULL;
	This->cOuts = 0;
	This->pConvBuf = NULL;

	return S_OK;
}

static HRESULT ACMWrapper_Cleanup( CTransformBaseImpl* pImpl )
{
	CACMWrapperImpl*	This = pImpl->m_pUserData;

	TRACE("(%p)\n",This);

	if ( This == NULL )
		return NOERROR;

	/* destruct */
	ACMWrapper_Close( This );
	QUARTZ_FreeMem( This->pwfxIn );
	ACMWrapper_CleanupMTypes( This );
	ACMWrapper_CleanupConvBuf( This );

	QUARTZ_FreeMem( This );
	pImpl->m_pUserData = NULL;

	return S_OK;
}

static HRESULT ACMWrapper_CheckMediaType( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE* pmtOut )
{
	CACMWrapperImpl*	This = pImpl->m_pUserData;
	const WAVEFORMATEX*	pwfxIn;
	const WAVEFORMATEX*	pwfxOut;
	WAVEFORMATEX	wfx;

	FIXME("(%p)\n",This);
	if ( This == NULL )
		return E_UNEXPECTED;

	pwfxIn = ACMWrapper_GetAudioFmt(pmtIn);
	if ( pwfxIn == NULL ||
	     pwfxIn->wFormatTag == 0 ||
	     pwfxIn->wFormatTag == 1 )
	{
		TRACE("pwfxIn is not a compressed audio\n");
		return E_FAIL;
	}
	if ( pmtOut != NULL )
	{
		pwfxOut = ACMWrapper_GetAudioFmt(pmtOut);
		if ( pwfxOut == NULL || pwfxOut->wFormatTag != 1 )
		{
			TRACE("pwfxOut is not a linear PCM\n");
			return E_FAIL;
		}
		if ( pwfxIn->nChannels != pwfxOut->nChannels ||
		     pwfxIn->nSamplesPerSec != pwfxOut->nSamplesPerSec )
		{
			TRACE("nChannels or nSamplesPerSec is not matched\n");
			return E_FAIL;
		}
		if ( !ACMWrapper_IsSupported((WAVEFORMATEX*)pwfxOut,(WAVEFORMATEX*)pwfxIn) )
		{
			TRACE("specified formats are not supported by ACM\n");
			return E_FAIL;
		}
	}
	else
	{
		ACMWrapper_FillFmtPCM(&wfx,pwfxIn,8);
		if ( ACMWrapper_IsSupported(&wfx,(WAVEFORMATEX*)pwfxIn) )
		{
			TRACE("compressed audio - can be decoded to 8bit\n");
			return S_OK;
		}
		ACMWrapper_FillFmtPCM(&wfx,pwfxIn,16);
		if ( ACMWrapper_IsSupported(&wfx,(WAVEFORMATEX*)pwfxIn) )
		{
			TRACE("compressed audio - can be decoded to 16bit\n");
			return S_OK;
		}

		TRACE("unhandled audio %04x\n",(unsigned)pwfxIn->wFormatTag);
		return E_FAIL;
	}

	return S_OK;
}

static HRESULT ACMWrapper_GetOutputTypes( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE** ppmtAcceptTypes, ULONG* pcAcceptTypes )
{
	CACMWrapperImpl*	This = pImpl->m_pUserData;
	HRESULT hr;
	const WAVEFORMATEX*	pwfxIn;
	AM_MEDIA_TYPE*		pmtTry;
	WAVEFORMATEX*		pwfxTry;

	FIXME("(%p)\n",This);
	hr = ACMWrapper_CheckMediaType( pImpl, pmtIn, NULL );
	if ( FAILED(hr) )
		return hr;
	pwfxIn = (const WAVEFORMATEX*)pmtIn->pbFormat;

	ACMWrapper_CleanupMTypes( This );
	This->pmtOuts = QUARTZ_AllocMem( sizeof(AM_MEDIA_TYPE) * 2 );
	if ( This->pmtOuts == NULL )
		return E_OUTOFMEMORY;
	This->cOuts = 0;

	pmtTry = &This->pmtOuts[This->cOuts];
	hr = ACMWrapper_SetupAudioFmt(
		pmtTry,
		sizeof(WAVEFORMATEX), 1,
		(pwfxIn->nChannels * 8) >> 3 );
	if ( FAILED(hr) ) goto err;
	pwfxTry = (WAVEFORMATEX*)pmtTry->pbFormat;
	ACMWrapper_FillFmtPCM( pwfxTry, pwfxIn, 8 );
	if ( ACMWrapper_IsSupported( pwfxTry, (WAVEFORMATEX*)pwfxIn ) )
		This->cOuts ++;

        pmtTry = &This->pmtOuts[This->cOuts];
        hr = ACMWrapper_SetupAudioFmt(
                pmtTry,
                sizeof(WAVEFORMATEX), 1,
                (pwfxIn->nChannels * 16) >> 3 );
        if ( FAILED(hr) ) goto err;
        pwfxTry = (WAVEFORMATEX*)pmtTry->pbFormat;
        ACMWrapper_FillFmtPCM( pwfxTry, pwfxIn, 16 );
        if ( ACMWrapper_IsSupported( pwfxTry, (WAVEFORMATEX*)pwfxIn ) )
                This->cOuts ++;

	*ppmtAcceptTypes = This->pmtOuts;
	*pcAcceptTypes = This->cOuts;

	return S_OK;
err:
	ACMWrapper_CleanupMTypes( This );
	return hr;
}

static HRESULT
ACMWrapper_GetConvBufSize(
	CTransformBaseImpl* pImpl,
	CACMWrapperImpl* This,
	DWORD* pcbInput, DWORD* pcbOutput,
	const AM_MEDIA_TYPE* pmtOut, const AM_MEDIA_TYPE* pmtIn )
{
        HRESULT hr;
        const WAVEFORMATEX* pwfxIn;
        const WAVEFORMATEX* pwfxOut;
        HACMSTREAM      has;
        MMRESULT        mr;
        DWORD   cbInput;
        DWORD   cbOutput;

        if ( This == NULL )
                return E_UNEXPECTED;

        hr = ACMWrapper_CheckMediaType( pImpl, pmtIn, pmtOut );
        if ( FAILED(hr) )
                return hr;
        pwfxIn = (const WAVEFORMATEX*)pmtIn->pbFormat;
        pwfxOut = (const WAVEFORMATEX*)pmtOut->pbFormat;

        hr = ACMWrapper_StreamOpen(
                &has, (WAVEFORMATEX*)pwfxOut, (WAVEFORMATEX*)pwfxIn );
        if ( FAILED(hr) )
                return hr;

        cbInput = (pwfxIn->nAvgBytesPerSec + pwfxIn->nBlockAlign - 1) / pwfxIn->nBlockAlign * pwfxIn->nBlockAlign;
        cbOutput = 0;

        mr = acmStreamSize( has, cbInput, &cbOutput, ACM_STREAMSIZEF_SOURCE );
        acmStreamClose( has, 0 );

        if ( mr != MMSYSERR_NOERROR || cbOutput == 0 )
                return E_FAIL;
        TRACE("size %lu -> %lu\n", cbInput, cbOutput);

	if ( pcbInput != NULL ) *pcbInput = cbInput;
	if ( pcbOutput != NULL ) *pcbOutput = cbOutput;

	return S_OK;
}


static HRESULT ACMWrapper_GetAllocProp( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE* pmtOut, ALLOCATOR_PROPERTIES* pProp, BOOL* pbTransInPlace, BOOL* pbTryToReuseSample )
{
	CACMWrapperImpl*	This = pImpl->m_pUserData;
	HRESULT hr;
	DWORD	cbOutput;

	TRACE("(%p)\n",This);

	if ( This == NULL )
		return E_UNEXPECTED;

	hr = ACMWrapper_GetConvBufSize(
		pImpl, This, NULL, &cbOutput, pmtOut, pmtIn );
	if ( FAILED(hr) )
		return hr;

	pProp->cBuffers = 1;
	pProp->cbBuffer = cbOutput;

	*pbTransInPlace = FALSE;
	*pbTryToReuseSample = FALSE;

	return S_OK;
}

static HRESULT ACMWrapper_BeginTransform( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE* pmtOut, BOOL bReuseSample )
{
	CACMWrapperImpl*	This = pImpl->m_pUserData;
	HRESULT	hr;
	const WAVEFORMATEX*	pwfxIn;
	const WAVEFORMATEX*	pwfxOut;
	DWORD	cbInput;

	FIXME("(%p,%p,%p,%d)\n",This,pmtIn,pmtOut,bReuseSample);

	if ( This == NULL )
		return E_UNEXPECTED;

	ACMWrapper_Close( This );
	ACMWrapper_CleanupMTypes( This );
	ACMWrapper_CleanupConvBuf( This );

	hr = ACMWrapper_GetConvBufSize(
		pImpl, This, &cbInput, NULL, pmtOut, pmtIn );
        if ( FAILED(hr) )
                return hr;
        pwfxIn = (const WAVEFORMATEX*)pmtIn->pbFormat;
        pwfxOut = (const WAVEFORMATEX*)pmtOut->pbFormat;

	This->pConvBuf = (BYTE*)QUARTZ_AllocMem( cbInput );
	if ( This->pConvBuf == NULL )
		return E_OUTOFMEMORY;
	This->cbConvBlockSize = pwfxIn->nBlockAlign;
	This->cbConvCached = 0;
	This->cbConvAllocated = cbInput;

	hr = ACMWrapper_StreamOpen(
		&This->has,
		(WAVEFORMATEX*)pmtOut, (WAVEFORMATEX*)pmtIn );
	if ( FAILED(hr) )
		return E_FAIL;

	return S_OK;
}

static HRESULT ACMWrapper_Convert(
	CTransformBaseImpl* pImpl,
	CACMWrapperImpl* This,
	BYTE* pbSrc, DWORD cbSrcLen,
	DWORD dwConvertFlags )
{
        ACMSTREAMHEADER ash;
	MMRESULT mr;
	HRESULT hr = E_FAIL;
	DWORD	dwConvCallFlags;
	DWORD	cb;
	IMediaSample*	pSampOut = NULL;
	BYTE*	pOutBuf;
	LONG	lOutBufLen;

	TRACE("()\n");

	if ( This->pConvBuf == NULL )
		return E_UNEXPECTED;

	dwConvCallFlags = ACM_STREAMCONVERTF_BLOCKALIGN;
	if ( dwConvertFlags & ACM_STREAMCONVERTF_START )
	{
		dwConvCallFlags |= ACM_STREAMCONVERTF_START;
		This->cbConvCached = 0;
	}

	while ( 1 )
	{
		cb = cbSrcLen + This->cbConvCached;
		if ( cb > This->cbConvAllocated )
			cb = This->cbConvAllocated;
		cb -= This->cbConvCached;
		if ( cb > 0 )
		{
			memcpy( This->pConvBuf+This->cbConvCached,
				pbSrc, cb );
			pbSrc += cb;
			cbSrcLen -= cb;
			This->cbConvCached += cb;
		}

		cb = This->cbConvCached / This->cbConvBlockSize * This->cbConvBlockSize;
		if ( cb == 0 )
		{
			if ( dwConvertFlags & ACM_STREAMCONVERTF_END )
			{
				dwConvCallFlags &= ~ACM_STREAMCONVERTF_BLOCKALIGN;
				dwConvCallFlags |= ACM_STREAMCONVERTF_END;
				cb = This->cbConvCached;
			}
			if ( cb == 0 )
			{
				hr = S_OK;
				break;
			}
		}

		ZeroMemory( &ash, sizeof(ash) );
		ash.cbStruct = sizeof(ash);
		ash.pbSrc = This->pConvBuf;
		ash.cbSrcLength = cb;

		hr = IMemAllocator_GetBuffer(
			pImpl->m_pOutPinAllocator,
			&pSampOut, NULL, NULL, 0 );
		if ( FAILED(hr) )
			break;
		hr = IMediaSample_SetSyncPoint( pSampOut, TRUE );
		if ( FAILED(hr) )
			break;
		if ( dwConvCallFlags & ACM_STREAMCONVERTF_START )
		{
			hr = IMediaSample_SetDiscontinuity( pSampOut, TRUE );
			if ( FAILED(hr) )
				break;
		}
		hr = IMediaSample_GetPointer( pSampOut, &pOutBuf );
		if ( FAILED(hr) )
			break;
		lOutBufLen = IMediaSample_GetSize( pSampOut );
		if ( lOutBufLen <= 0 )
		{
			hr = E_FAIL;
			break;
		}
		ash.pbDst = pOutBuf;
		ash.cbDstLength = lOutBufLen;

		mr = acmStreamPrepareHeader(
			This->has, &ash, 0 );
		if ( mr == MMSYSERR_NOERROR )
			mr = acmStreamConvert(
				This->has, &ash, dwConvCallFlags );
		if ( mr == MMSYSERR_NOERROR )
			mr = acmStreamUnprepareHeader(
				This->has, &ash, 0 );
		if ( mr != MMSYSERR_NOERROR || ash.cbSrcLengthUsed == 0 )
		{
			hr = E_FAIL;
			break;
		}

		if ( ash.cbDstLengthUsed > 0 )
		{
			hr = IMediaSample_SetActualDataLength( pSampOut, ash.cbDstLengthUsed );
			if ( FAILED(hr) )
				break;

			hr = CPinBaseImpl_SendSample(
				&pImpl->pOutPin->pin,
				pSampOut );
			if ( FAILED(hr) )
				break;
		}

		if ( This->cbConvCached == ash.cbSrcLengthUsed )
		{
			This->cbConvCached = 0;
		}
		else
		{
			This->cbConvCached -= ash.cbSrcLengthUsed;
			memmove( This->pConvBuf,
				 This->pConvBuf + ash.cbSrcLengthUsed,
				 This->cbConvCached );
		}

		IMediaSample_Release( pSampOut ); pSampOut = NULL;
		dwConvCallFlags &= ~ACM_STREAMCONVERTF_START;
	}

	if ( pSampOut != NULL )
		IMediaSample_Release( pSampOut );

	return hr;
}

static HRESULT ACMWrapper_ProcessReceive( CTransformBaseImpl* pImpl, IMediaSample* pSampIn )
{
	CACMWrapperImpl*	This = pImpl->m_pUserData;
	BYTE*	pDataIn = NULL;
	LONG	lDataInLen;
	HRESULT hr;
	DWORD	dwConvFlags = 0;

	FIXME("(%p)\n",This);

	if ( This == NULL || This->has == (HACMSTREAM)NULL )
		return E_UNEXPECTED;

	hr = IMediaSample_GetPointer( pSampIn, &pDataIn );
	if ( FAILED(hr) )
		return hr;
	lDataInLen = IMediaSample_GetActualDataLength( pSampIn );
	if ( lDataInLen < 0 )
		return E_FAIL;
	if ( IMediaSample_IsDiscontinuity( pSampIn ) != S_OK )
		dwConvFlags |= ACM_STREAMCONVERTF_START;

	return ACMWrapper_Convert(
		pImpl, This, pDataIn, (DWORD)lDataInLen,
		dwConvFlags );
}

static HRESULT ACMWrapper_EndTransform( CTransformBaseImpl* pImpl )
{
	CACMWrapperImpl*	This = pImpl->m_pUserData;
	HRESULT hr;
	DWORD	dwConvFlags = ACM_STREAMCONVERTF_END;

	TRACE("(%p)\n",This);

	if ( This == NULL )
		return E_UNEXPECTED;

	hr = ACMWrapper_Convert(
		pImpl, This, NULL, 0,
		dwConvFlags );

	ACMWrapper_Close( This );
	ACMWrapper_CleanupMTypes( This );
	ACMWrapper_CleanupConvBuf( This );

	return hr;
}

static const TransformBaseHandlers transhandlers =
{
	ACMWrapper_Init,
	ACMWrapper_Cleanup,
	ACMWrapper_CheckMediaType,
	ACMWrapper_GetOutputTypes,
	ACMWrapper_GetAllocProp,
	ACMWrapper_BeginTransform,
	ACMWrapper_ProcessReceive,
	NULL,
	ACMWrapper_EndTransform,
};

HRESULT QUARTZ_CreateACMWrapper(IUnknown* punkOuter,void** ppobj)
{
	return QUARTZ_CreateTransformBase(
		punkOuter,ppobj,
		&CLSID_ACMWrapper,
		ACMWrapper_FilterName,
		NULL, NULL,
		&transhandlers );
}


