/*
 * Implements AVI Parser(Splitter).
 *
 *	FIXME - no seeking
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
#include "mmsystem.h"
#include "vfw.h"
#include "winerror.h"
#include "strmif.h"
#include "control.h"
#include "vfwmsgs.h"
#include "amvideo.h"
#include "uuids.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "parser.h"
#include "mtype.h"



static const WCHAR QUARTZ_AVIParser_Name[] =
{ 'A','V','I',' ','S','p','l','i','t','t','e','r',0 };
static const WCHAR QUARTZ_AVIParserInPin_Name[] =
{ 'I','n',0 };
static const WCHAR QUARTZ_AVIParserOutPin_Basename[] =
{ 'S','t','r','e','a','m',0 };

#define WINE_QUARTZ_AVIPINNAME_MAX	64

/****************************************************************************
 *
 *	CAVIParseImpl
 */


typedef struct CAVIParseImpl CAVIParseImpl;
typedef struct CAVIParseStream CAVIParseStream;

struct CAVIParseImpl
{
	MainAVIHeader	avih;
	CAVIParseStream*	pStreamsBuf;
	DWORD	cIndexEntries;
	AVIINDEXENTRY*	pIndexEntriesBuf;
	WCHAR	wchWork[ WINE_QUARTZ_AVIPINNAME_MAX ];
};

struct CAVIParseStream
{
	AVIStreamHeader	strh;
	DWORD	cbFmt;
	BYTE*	pFmtBuf;
	DWORD	cIndexEntries;
	AVIINDEXENTRY*	pIndexEntries;
	DWORD	cIndexCur;
	REFERENCE_TIME	rtCur;
	REFERENCE_TIME	rtInternal;
	BOOL	bDataDiscontinuity;
};


static HRESULT CAVIParseImpl_ParseStreamList(
	CParserImpl* pImpl, CAVIParseImpl* This, ULONG nStreamIndex,
	LONGLONG llOfsTop, DWORD dwListLen, CAVIParseStream* pStream )
{
	HRESULT hr;
	LONGLONG	llOfs;
	DWORD	dwChunkLength;

	TRACE("search strh\n");
	hr = RIFF_SearchChunk(
		pImpl, dwListLen,
		llOfsTop, PARSER_strh,
		&llOfs, &dwChunkLength );
	if ( hr == S_OK )
	{
		TRACE("strh has been detected\n");
		if ( dwChunkLength < sizeof(AVIStreamHeader) )
			hr = E_FAIL;
		else
			hr = IAsyncReader_SyncRead( pImpl->m_pReader,
				llOfs, sizeof(AVIStreamHeader), (BYTE*)&pStream->strh );
	}
	if ( FAILED(hr) )
		return hr;
	if ( hr != S_OK )
		return E_FAIL;

	TRACE("search strf\n");
	hr = RIFF_SearchChunk(
		pImpl, dwListLen,
		llOfsTop, PARSER_strf,
		&llOfs, &dwChunkLength );
	if ( hr == S_OK && dwChunkLength > 0 )
	{
		TRACE("strf has been detected\n");
		pStream->cbFmt = dwChunkLength;
		pStream->pFmtBuf = (BYTE*)QUARTZ_AllocMem( dwChunkLength );
		if ( pStream->pFmtBuf == NULL )
			hr = E_OUTOFMEMORY;
		else
			hr = IAsyncReader_SyncRead( pImpl->m_pReader,
				llOfs, dwChunkLength, pStream->pFmtBuf );
	}
	if ( FAILED(hr) )
		return hr;

	TRACE("search indx\n");
	hr = RIFF_SearchChunk(
		pImpl, dwListLen,
		llOfsTop, PARSER_indx,
		&llOfs, &dwChunkLength );
	if ( FAILED(hr) )
		return hr;
	if ( hr == S_OK )
	{
		FIXME( "'indx' has been detected - not implemented now!\n" );
		/*return E_FAIL;*/
	}

	return NOERROR;
}


static HRESULT CAVIParseImpl_InitParser( CParserImpl* pImpl, ULONG* pcStreams )
{
	CAVIParseImpl*	This = NULL;
	BYTE	riffhdr[12];
	ULONG	i;
	ULONG	nIndex;
	HRESULT hr;
	LONGLONG	llOfs_hdrl;
	DWORD	dwLen_hdrl;
	LONGLONG	llOfs;
	DWORD	dwChunkId;
	DWORD	dwChunkLength;
	AVIINDEXENTRY*	pEntriesBuf = NULL;
	ULONG	cEntries;
	ULONG	cEntriesCur;

	TRACE("(%p,%p)\n",pImpl,pcStreams);

	hr = IAsyncReader_SyncRead( pImpl->m_pReader, 0, 12, riffhdr );
	if ( FAILED(hr) )
		return hr;
	if ( hr != S_OK )
		return E_FAIL;
	if ( memcmp( &riffhdr[0], "RIFF", 4 ) != 0 ||
		 memcmp( &riffhdr[8], "AVI ", 4 ) != 0 )
		return E_FAIL;

	TRACE("it's AVI\n");

	This = (CAVIParseImpl*)QUARTZ_AllocMem( sizeof(CAVIParseImpl) );
	if ( This == NULL )
		return E_OUTOFMEMORY;
	pImpl->m_pUserData = This;
	ZeroMemory( This, sizeof(CAVIParseImpl) );
	This->pStreamsBuf = NULL;
	This->cIndexEntries = 0;
	This->pIndexEntriesBuf = 0;

	hr = RIFF_SearchList(
		pImpl, (DWORD)0xffffffff,
		PARSER_RIFF_OfsFirst, PARSER_hdrl,
		&llOfs_hdrl, &dwLen_hdrl );
	if ( FAILED(hr) )
		return hr;
	if ( hr != S_OK )
		return E_FAIL;

	/* read 'avih' */
	TRACE("read avih\n");
	hr = RIFF_SearchChunk(
		pImpl, dwLen_hdrl,
		llOfs_hdrl, PARSER_avih,
		&llOfs, &dwChunkLength );
	if ( FAILED(hr) )
		return hr;
	if ( hr != S_OK )
		return E_FAIL;

	if ( dwChunkLength > sizeof(MainAVIHeader) )
		dwChunkLength = sizeof(MainAVIHeader);
	hr = IAsyncReader_SyncRead( pImpl->m_pReader, llOfs, dwChunkLength, (BYTE*)&(This->avih) );
	if ( FAILED(hr) )
		return hr;
	if ( hr != S_OK )
		return E_FAIL;
	if ( This->avih.dwStreams == 0 )
		return E_FAIL;

	/* initialize streams. */
	This->pStreamsBuf = (CAVIParseStream*)QUARTZ_AllocMem(
		sizeof(CAVIParseStream) * This->avih.dwStreams );
	if ( This->pStreamsBuf == NULL )
		return E_OUTOFMEMORY;
	ZeroMemory( This->pStreamsBuf,
		sizeof(CAVIParseStream) * This->avih.dwStreams );

	llOfs = llOfs_hdrl;
	for ( nIndex = 0; nIndex < This->avih.dwStreams; nIndex++ )
	{
		TRACE("search strl for stream %lu\n",nIndex);
		hr = RIFF_SearchList(
			pImpl,
			dwLen_hdrl, llOfs, PARSER_strl,
			&llOfs, &dwChunkLength );
		if ( FAILED(hr) )
			return hr;
		if ( hr != S_OK )
			return E_FAIL;

		/* read 'strl'. */
		hr = CAVIParseImpl_ParseStreamList(
			pImpl, This, nIndex,
			llOfs, dwChunkLength, &This->pStreamsBuf[nIndex] );

		if ( FAILED(hr) )
			return hr;
		if ( hr != S_OK )
			return E_FAIL;
		llOfs += dwChunkLength;
	}

	/* initialize idx1. */
	TRACE("search idx1\n");
	hr = RIFF_SearchChunk(
		pImpl, (DWORD)0xffffffff,
		PARSER_RIFF_OfsFirst, PARSER_idx1,
		&llOfs, &dwChunkLength );
	if ( FAILED(hr) )
		return hr;
	if ( hr == S_OK )
	{
		/* read idx1. */
		This->cIndexEntries = dwChunkLength / sizeof(AVIINDEXENTRY);
		This->pIndexEntriesBuf = (AVIINDEXENTRY*)QUARTZ_AllocMem(
			sizeof(AVIINDEXENTRY) * This->cIndexEntries );
		if ( This->pIndexEntriesBuf == NULL )
			return E_OUTOFMEMORY;
		hr = IAsyncReader_SyncRead( pImpl->m_pReader, llOfs, sizeof(AVIINDEXENTRY) * This->cIndexEntries, (BYTE*)This->pIndexEntriesBuf );
		if ( FAILED(hr) )
			return hr;
		if ( hr != S_OK )
			return E_FAIL;

		pEntriesBuf = (AVIINDEXENTRY*)QUARTZ_AllocMem(
			sizeof(AVIINDEXENTRY) * This->cIndexEntries );
		if ( pEntriesBuf == NULL )
			return E_OUTOFMEMORY;
		cEntries = 0;
		for ( nIndex = 0; nIndex < This->avih.dwStreams; nIndex++ )
		{
			cEntriesCur = cEntries;
			dwChunkId = (((nIndex%10)+'0')<<8) | ((nIndex/10)+'0');
			for ( i = 0; i < This->cIndexEntries; i++ )
			{
				if ( (This->pIndexEntriesBuf[i].ckid & 0xffff) == dwChunkId )
					memcpy( &pEntriesBuf[cEntries++], &This->pIndexEntriesBuf[i], sizeof(AVIINDEXENTRY) );
			}
			This->pStreamsBuf[nIndex].pIndexEntries = &pEntriesBuf[cEntriesCur];
			This->pStreamsBuf[nIndex].cIndexEntries = cEntries - cEntriesCur;
			This->pStreamsBuf[nIndex].cIndexCur = 0;
			This->pStreamsBuf[nIndex].rtCur = 0;
			This->pStreamsBuf[nIndex].rtInternal = 0;
			TRACE("stream %lu - %lu entries\n",nIndex,This->pStreamsBuf[nIndex].cIndexEntries);
			This->pStreamsBuf[nIndex].bDataDiscontinuity = TRUE;
		}
		QUARTZ_FreeMem(This->pIndexEntriesBuf);
		This->pIndexEntriesBuf = pEntriesBuf;

		This->avih.dwSuggestedBufferSize = 0;
		for ( i = 0; i < This->cIndexEntries; i++ )
		{
			if ( This->avih.dwSuggestedBufferSize < This->pIndexEntriesBuf[i].dwChunkLength )
				This->avih.dwSuggestedBufferSize = This->pIndexEntriesBuf[i].dwChunkLength;
		}
	}
	else
	{
		return E_FAIL;
	}

	if ( This->avih.dwStreams > 100 )
		return E_FAIL;

	*pcStreams = This->avih.dwStreams;

	return NOERROR;
}

static HRESULT CAVIParseImpl_UninitParser( CParserImpl* pImpl )
{
	CAVIParseImpl*	This = (CAVIParseImpl*)pImpl->m_pUserData;
	ULONG	nIndex;

	TRACE("(%p)\n",This);

	if ( This == NULL )
		return NOERROR;

	/* destruct */
	if ( This->pStreamsBuf != NULL )
	{
		for ( nIndex = 0; nIndex < This->avih.dwStreams; nIndex++ )
		{
			/* release this stream */
			if ( This->pStreamsBuf[nIndex].pFmtBuf != NULL )
				QUARTZ_FreeMem(This->pStreamsBuf[nIndex].pFmtBuf);
		}
		QUARTZ_FreeMem( This->pStreamsBuf );
		This->pStreamsBuf = NULL;
	}

	if ( This->pIndexEntriesBuf != NULL )
	{
		QUARTZ_FreeMem( This->pIndexEntriesBuf );
		This->pIndexEntriesBuf = NULL;
	}

	QUARTZ_FreeMem( This );
	pImpl->m_pUserData = NULL;

	return NOERROR;
}

static LPCWSTR CAVIParseImpl_GetOutPinName( CParserImpl* pImpl, ULONG nStreamIndex )
{
	CAVIParseImpl*	This = (CAVIParseImpl*)pImpl->m_pUserData;
	int wlen;

	TRACE("(%p,%lu)\n",This,nStreamIndex);

	if ( This == NULL || nStreamIndex >= This->avih.dwStreams )
		return NULL;

	wlen = lstrlenW(QUARTZ_AVIParserOutPin_Basename);
	memcpy( This->wchWork, QUARTZ_AVIParserOutPin_Basename, sizeof(WCHAR)*wlen );
	This->wchWork[ wlen ] = (nStreamIndex/10) + '0';
	This->wchWork[ wlen+1 ] = (nStreamIndex%10) + '0';
	This->wchWork[ wlen+2 ] = 0;

	return This->wchWork;
}

static HRESULT CAVIParseImpl_GetStreamType( CParserImpl* pImpl, ULONG nStreamIndex, AM_MEDIA_TYPE* pmt )
{
	CAVIParseImpl*	This = (CAVIParseImpl*)pImpl->m_pUserData;
	VIDEOINFOHEADER*	pvi;
	BITMAPINFOHEADER*	pbi;
	WAVEFORMATEX*	pwfx;
	DWORD	cbFmt;
	DWORD	cb;
	HRESULT hr;

	TRACE("(%p,%lu,%p)\n",This,nStreamIndex,pmt);

	if ( This == NULL )
		return E_UNEXPECTED;
	if ( nStreamIndex >= This->avih.dwStreams )
		return E_INVALIDARG;

	cbFmt = This->pStreamsBuf[nStreamIndex].cbFmt;

	ZeroMemory( pmt, sizeof(AM_MEDIA_TYPE) );
	switch ( This->pStreamsBuf[nStreamIndex].strh.fccType )
	{
	case PARSER_vids:
		pbi = (BITMAPINFOHEADER*)This->pStreamsBuf[nStreamIndex].pFmtBuf;
		if ( pbi == NULL || cbFmt < sizeof(BITMAPINFOHEADER) )
			goto unknown_format;

		memcpy( &pmt->majortype, &MEDIATYPE_Video, sizeof(GUID) );
		hr = QUARTZ_MediaSubType_FromBitmap( &pmt->subtype, pbi );
		if ( FAILED(hr) )
			goto unknown_format;
		if ( hr != S_OK )
			QUARTZ_MediaSubType_FromFourCC( &pmt->subtype, (DWORD)pbi->biCompression );

		pmt->bFixedSizeSamples = QUARTZ_BitmapHasFixedSample( pbi ) ? 1 : 0;
		pmt->bTemporalCompression = 0; /* FIXME - 1 if inter-frame compression is used */
		pmt->lSampleSize = ( pbi->biCompression == 0 ) ? DIBSIZE(*pbi) : pbi->biSizeImage;
		memcpy( &pmt->formattype, &FORMAT_VideoInfo, sizeof(GUID) );

		cb = sizeof(VIDEOINFOHEADER) + cbFmt;
		pmt->pbFormat = (BYTE*)CoTaskMemAlloc( cb );
		if ( pmt->pbFormat == NULL )
			return E_OUTOFMEMORY;
		ZeroMemory( pmt->pbFormat, cb );
		pvi = (VIDEOINFOHEADER*)pmt->pbFormat;
		pmt->cbFormat = cb;
		memcpy( &pvi->bmiHeader, pbi, cbFmt );
		break;
	case PARSER_auds:
		pwfx = (WAVEFORMATEX*)This->pStreamsBuf[nStreamIndex].pFmtBuf;
		if ( pwfx == NULL || cbFmt < (sizeof(WAVEFORMATEX)-2) )
			goto unknown_format;

		memcpy( &pmt->majortype, &MEDIATYPE_Audio, sizeof(GUID) );
		QUARTZ_MediaSubType_FromFourCC( &pmt->subtype, (DWORD)pwfx->wFormatTag );
		pmt->bFixedSizeSamples = 1;
		pmt->bTemporalCompression = 0;
		pmt->lSampleSize = pwfx->nBlockAlign;
		memcpy( &pmt->formattype, &FORMAT_WaveFormatEx, sizeof(GUID) );
		pmt->pUnk = NULL;

		cb = ( cbFmt < sizeof(WAVEFORMATEX) ) ? sizeof(WAVEFORMATEX) : cbFmt;
		pmt->pbFormat = (BYTE*)CoTaskMemAlloc( cb );
		if ( pmt->pbFormat == NULL )
			return E_OUTOFMEMORY;
		ZeroMemory( pmt->pbFormat, cb );
		pmt->cbFormat = cbFmt;
		memcpy( pmt->pbFormat, pwfx, cbFmt );
		break;
	case PARSER_mids:
		/* FIXME? */
		memcpy( &pmt->majortype, &MEDIATYPE_Midi, sizeof(GUID) );
		memcpy( &pmt->subtype, &MEDIASUBTYPE_NULL, sizeof(GUID) );
		pmt->bFixedSizeSamples = 0;
		pmt->bTemporalCompression = 0;
		pmt->lSampleSize = 1;
		memcpy( &pmt->formattype, &FORMAT_None, sizeof(GUID) );
		pmt->pUnk = NULL;
		pmt->cbFormat = 0;
		pmt->pbFormat = NULL;
		break;
	case PARSER_txts:
		/* FIXME? */
		memcpy( &pmt->majortype, &MEDIATYPE_Text, sizeof(GUID) );
		memcpy( &pmt->subtype, &MEDIASUBTYPE_NULL, sizeof(GUID) );
		pmt->bFixedSizeSamples = 0;
		pmt->bTemporalCompression = 0;
		pmt->lSampleSize = 1;
		memcpy( &pmt->formattype, &FORMAT_None, sizeof(GUID) );
		pmt->pUnk = NULL;
		pmt->cbFormat = 0;
		pmt->pbFormat = NULL;
		break;
	default:
		goto unknown_format;
	}

	return NOERROR;

unknown_format:;
	FIXME( "(%p) unsupported stream type %c%c%c%c\n",This,
			(int)((This->pStreamsBuf[nStreamIndex].strh.fccType>> 0)&0xff),
			(int)((This->pStreamsBuf[nStreamIndex].strh.fccType>> 8)&0xff),
			(int)((This->pStreamsBuf[nStreamIndex].strh.fccType>>16)&0xff),
			(int)((This->pStreamsBuf[nStreamIndex].strh.fccType>>24)&0xff) );

	memcpy( &pmt->majortype, &MEDIATYPE_NULL, sizeof(GUID) );
	memcpy( &pmt->subtype, &MEDIASUBTYPE_NULL, sizeof(GUID) );
	pmt->bFixedSizeSamples = 0;
	pmt->bTemporalCompression = 0;
	pmt->lSampleSize = 1;
	memcpy( &pmt->formattype, &FORMAT_None, sizeof(GUID) );
	pmt->pUnk = NULL;
	pmt->cbFormat = 0;
	pmt->pbFormat = NULL;

	return NOERROR;
}

static HRESULT CAVIParseImpl_CheckStreamType( CParserImpl* pImpl, ULONG nStreamIndex, const AM_MEDIA_TYPE* pmt )
{
	CAVIParseImpl*	This = (CAVIParseImpl*)pImpl->m_pUserData;
	HRESULT hr;
	AM_MEDIA_TYPE	mt;
	VIDEOINFOHEADER*	pvi;
	VIDEOINFOHEADER*	pviCheck;
	WAVEFORMATEX*	pwfx;
	WAVEFORMATEX*	pwfxCheck;

	TRACE("(%p,%lu,%p)\n",This,nStreamIndex,pmt);

	hr = CAVIParseImpl_GetStreamType( pImpl, nStreamIndex, &mt );
	if ( FAILED(hr) )
		return hr;

	TRACE("check GUIDs - %s,%s\n",debugstr_guid(&pmt->majortype),debugstr_guid(&pmt->subtype));
	if ( !IsEqualGUID( &pmt->majortype, &mt.majortype ) ||
		 !IsEqualGUID( &pmt->subtype, &mt.subtype ) ||
		 !IsEqualGUID( &pmt->formattype, &mt.formattype ) )
	{
		hr = E_FAIL;
		goto end;
	}

	TRACE("check format\n");
	hr = S_OK;
	switch ( This->pStreamsBuf[nStreamIndex].strh.fccType )
	{
	case PARSER_vids:
		TRACE("check vids\n");
		pvi = (VIDEOINFOHEADER*)mt.pbFormat;
		pviCheck = (VIDEOINFOHEADER*)pmt->pbFormat;
		if ( pvi == NULL || pviCheck == NULL || pmt->cbFormat < sizeof(VIDEOINFOHEADER) )
			hr = E_FAIL;
		if ( pvi->bmiHeader.biWidth != pviCheck->bmiHeader.biWidth ||
			 pvi->bmiHeader.biHeight != pviCheck->bmiHeader.biHeight ||
			 pvi->bmiHeader.biPlanes != pviCheck->bmiHeader.biPlanes ||
			 pvi->bmiHeader.biBitCount != pviCheck->bmiHeader.biBitCount ||
			 pvi->bmiHeader.biCompression != pviCheck->bmiHeader.biCompression ||
			 pvi->bmiHeader.biClrUsed != pviCheck->bmiHeader.biClrUsed )
			hr = E_FAIL;
		break;
	case PARSER_auds:
		TRACE("check auds\n");
		pwfx = (WAVEFORMATEX*)mt.pbFormat;
		pwfxCheck = (WAVEFORMATEX*)pmt->pbFormat;
		if ( pwfx == NULL || pwfxCheck == NULL || pmt->cbFormat < (sizeof(WAVEFORMATEX)-2) )
			hr = E_FAIL;
		if ( pwfx->wFormatTag != pwfxCheck->wFormatTag ||
			 pwfx->nBlockAlign != pwfxCheck->nBlockAlign ||
			 pwfx->wBitsPerSample != pwfxCheck->wBitsPerSample ||
			 pwfx->nChannels != pwfxCheck->nChannels ||
			 pwfx->nSamplesPerSec != pwfxCheck->nSamplesPerSec )
			hr = E_FAIL;
		break;
	case PARSER_mids:
	case PARSER_txts:
		break;
	default:
		break;
	}
end:
	QUARTZ_MediaType_Free( &mt );

	TRACE("%08lx\n",hr);

	return hr;
}

static HRESULT CAVIParseImpl_GetAllocProp( CParserImpl* pImpl, ALLOCATOR_PROPERTIES* pReqProp )
{
	CAVIParseImpl*	This = (CAVIParseImpl*)pImpl->m_pUserData;

	TRACE("(%p,%p)\n",This,pReqProp);
	if ( This == NULL )
		return E_UNEXPECTED;

	ZeroMemory( pReqProp, sizeof(ALLOCATOR_PROPERTIES) );
	pReqProp->cBuffers = This->avih.dwStreams;
	pReqProp->cbBuffer = This->avih.dwSuggestedBufferSize;

	return NOERROR;
}

static HRESULT CAVIParseImpl_GetNextRequest( CParserImpl* pImpl, ULONG* pnStreamIndex, LONGLONG* pllStart, LONG* plLength, REFERENCE_TIME* prtStart, REFERENCE_TIME* prtStop, DWORD* pdwSampleFlags )
{
	CAVIParseImpl*	This = (CAVIParseImpl*)pImpl->m_pUserData;
	REFERENCE_TIME	rtNext;
	DWORD	nIndexNext;
	DWORD	nIndex;
	CAVIParseStream*	pStream;
	const WAVEFORMATEX*	pwfx;

	TRACE("(%p)\n",This);

	if ( This == NULL )
		return E_UNEXPECTED;
	*pdwSampleFlags = AM_SAMPLE_SPLICEPOINT;

	nIndexNext = This->avih.dwStreams;
	rtNext = ((REFERENCE_TIME)0x7fffffff<<32)|((REFERENCE_TIME)0xffffffff);
	for ( nIndex = 0; nIndex < This->avih.dwStreams; nIndex++ )
	{
		TRACE("stream %lu - %lu,%lu\n",nIndex,(unsigned long)(This->pStreamsBuf[nIndex].rtCur*1000/QUARTZ_TIMEUNITS),This->pStreamsBuf[nIndex].cIndexCur);
		if ( rtNext > This->pStreamsBuf[nIndex].rtCur &&
			 This->pStreamsBuf[nIndex].cIndexCur < This->pStreamsBuf[nIndex].cIndexEntries )
		{
			nIndexNext = nIndex;
			rtNext = This->pStreamsBuf[nIndex].rtCur;
		}
	}
	if ( nIndexNext >= This->avih.dwStreams )
		return S_FALSE;

	if ( This->pIndexEntriesBuf != NULL )
	{
		pStream = &This->pStreamsBuf[nIndexNext];
		*pnStreamIndex = nIndexNext;
		*pllStart = (LONGLONG)pStream->pIndexEntries[pStream->cIndexCur].dwChunkOffset + 8;
		*plLength = (LONG)pStream->pIndexEntries[pStream->cIndexCur].dwChunkLength;
		*prtStart = rtNext;
		*prtStop = rtNext;
		/* FIXME - is this frame keyframe?? */
		*pdwSampleFlags = AM_SAMPLE_SPLICEPOINT;
		if ( pStream->bDataDiscontinuity )
		{
			*pdwSampleFlags |= AM_SAMPLE_DATADISCONTINUITY;
			pStream->bDataDiscontinuity = FALSE;
		}

		switch ( pStream->strh.fccType )
		{
		case PARSER_vids:
			TRACE("vids\n");
			pStream->rtInternal ++;
			rtNext = pStream->rtInternal * (REFERENCE_TIME)QUARTZ_TIMEUNITS * (REFERENCE_TIME)pStream->strh.dwScale / (REFERENCE_TIME)pStream->strh.dwRate;
			/* FIXME - handle AVIPALCHANGE */
			break;
		case PARSER_auds:
			TRACE("auds\n");
			pwfx = (const WAVEFORMATEX*)pStream->pFmtBuf;
			if ( pwfx != NULL && pStream->cbFmt >= (sizeof(WAVEFORMATEX)-2) )
			{
				pStream->rtInternal += (REFERENCE_TIME)*plLength;
				rtNext = pStream->rtInternal * (REFERENCE_TIME)QUARTZ_TIMEUNITS / (REFERENCE_TIME)pwfx->nAvgBytesPerSec;
			}
			else
			{
				pStream->rtInternal += (REFERENCE_TIME)(*plLength);
				rtNext = pStream->rtInternal * (REFERENCE_TIME)QUARTZ_TIMEUNITS * (REFERENCE_TIME)pStream->strh.dwScale / ((REFERENCE_TIME)pStream->strh.dwSampleSize * (REFERENCE_TIME)pStream->strh.dwRate);
			}
			break;
		case PARSER_mids:
		case PARSER_txts:
		default:
			pStream->rtInternal += (REFERENCE_TIME)(*plLength);
			rtNext = pStream->rtInternal * (REFERENCE_TIME)QUARTZ_TIMEUNITS * (REFERENCE_TIME)pStream->strh.dwScale / ((REFERENCE_TIME)pStream->strh.dwSampleSize * (REFERENCE_TIME)pStream->strh.dwRate);
			break;
		}
		pStream->cIndexCur ++;
		pStream->rtCur = rtNext;
		*prtStop = rtNext;
	}
	else
	{
		ERR( "no idx1\n" );
		return E_NOTIMPL;
	}

	TRACE("return %lu / %ld-%ld / %lu-%lu\n",
		*pnStreamIndex,(long)*pllStart,*plLength,
		(unsigned long)((*prtStart)*1000/QUARTZ_TIMEUNITS),
		(unsigned long)((*prtStop)*1000/QUARTZ_TIMEUNITS));

	return NOERROR;
}

static HRESULT CAVIParseImpl_ProcessSample( CParserImpl* pImpl, ULONG nStreamIndex, LONGLONG llStart, LONG lLength, IMediaSample* pSample )
{
	CAVIParseImpl*	This = (CAVIParseImpl*)pImpl->m_pUserData;

	TRACE("(%p,%lu,%ld,%ld,%p)\n",This,nStreamIndex,(long)llStart,lLength,pSample);

	if ( This == NULL )
		return E_UNEXPECTED;

	return NOERROR;
}




static const struct ParserHandlers CAVIParseImpl_Handlers =
{
	CAVIParseImpl_InitParser,
	CAVIParseImpl_UninitParser,
	CAVIParseImpl_GetOutPinName,
	CAVIParseImpl_GetStreamType,
	CAVIParseImpl_CheckStreamType,
	CAVIParseImpl_GetAllocProp,
	CAVIParseImpl_GetNextRequest,
	CAVIParseImpl_ProcessSample,

	/* for IQualityControl */
	NULL, /* pQualityNotify */

	/* for seeking */
	NULL, /* pGetSeekingCaps */
	NULL, /* pIsTimeFormatSupported */
	NULL, /* pGetCurPos */
	NULL, /* pSetCurPos */
	NULL, /* pGetDuration */
	NULL, /* pGetStopPos */
	NULL, /* pSetStopPos */
	NULL, /* pGetPreroll */
};

HRESULT QUARTZ_CreateAVISplitter(IUnknown* punkOuter,void** ppobj)
{
	return QUARTZ_CreateParser(
		punkOuter,ppobj,
		&CLSID_AviSplitter,
		QUARTZ_AVIParser_Name,
		QUARTZ_AVIParserInPin_Name,
		&CAVIParseImpl_Handlers );
}


