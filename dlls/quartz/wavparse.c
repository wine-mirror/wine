/*
 * Implements WAVE/AU/AIFF Parser.
 *
 * hidenori@a2.ctktv.ne.jp
 */

#include "config.h"

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "mmsystem.h"
#include "winerror.h"
#include "wine/obj_base.h"
#include "strmif.h"
#include "vfwmsgs.h"
#include "uuids.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"

/* for CLSID_quartzWaveParser. */
#include "initguid.h"
#include "parser.h"


static const WCHAR QUARTZ_WaveParser_Name[] =
{ 'W','a','v','e',' ','P','a','r','s','e','r',0 };
static const WCHAR QUARTZ_WaveParserInPin_Name[] =
{ 'I','n',0 };
static const WCHAR QUARTZ_WaveParserOutPin_Name[] =
{ 'O','u','t',0 };


/****************************************************************************/

/* S_OK = found, S_FALSE = not found */
HRESULT RIFF_GetNext(
	CParserImpl* pImpl, LONGLONG llOfs,
	DWORD* pdwCode, DWORD* pdwLength )
{
	BYTE bTemp[8];
	HRESULT hr;

	hr = IAsyncReader_SyncRead( pImpl->m_pReader, llOfs, 8, bTemp );
	if ( hr == S_OK )
	{
		*pdwCode = mmioFOURCC(bTemp[0],bTemp[1],bTemp[2],bTemp[3]);
		*pdwLength = PARSER_LE_UINT32(&bTemp[4]);
	}
	else
	{
		*pdwCode = 0;
		*pdwLength = 0;
	}

	return hr;
}

/* S_OK = found, S_FALSE = not found */
HRESULT RIFF_SearchChunk(
	CParserImpl* pImpl,
	LONGLONG llOfs, DWORD dwChunk,
	LONGLONG* pllOfs, DWORD* pdwChunkLength )
{
	HRESULT hr;
	DWORD dwCurCode;
	DWORD dwCurLen;

	while ( 1 )
	{
		hr = RIFF_GetNext( pImpl, llOfs, &dwCurCode, &dwCurLen );
		if ( hr != S_OK )
			break;
		if ( dwChunk == dwCurCode )
			break;
		llOfs += 8 + (LONGLONG)((dwCurLen+1)&(~1));
	}

	*pllOfs = llOfs;
	*pdwChunkLength = dwCurLen;

	return hr;
}




/****************************************************************************
 *
 *	CWavParseImpl
 */

typedef enum WavParseFmtType
{
	WaveParse_Native,
	WaveParse_Signed8,
	WaveParse_Signed16BE,
	WaveParse_Unsigned16LE,
	WaveParse_Unsigned16BE,
} WavParseFmtType;

typedef struct CWavParseImpl
{
	DWORD	cbFmt;
	WAVEFORMATEX*	pFmt;
	DWORD	dwBlockSize;
	LONGLONG	llDataStart;
	LONGLONG	llBytesTotal;
	LONGLONG	llBytesProcessed;
	WavParseFmtType	iFmtType;
} CWavParseImpl;


static HRESULT CWavParseImpl_InitWAV( CParserImpl* pImpl, CWavParseImpl* This )
{
	HRESULT hr;
	LONGLONG	llOfs;
	DWORD	dwChunkLength;

	hr = RIFF_SearchChunk(
		pImpl, PARSER_RIFF_OfsFirst,
		PARSER_fmt, &llOfs, &dwChunkLength );
	if ( FAILED(hr) )
		return hr;
	if ( hr != S_OK || ( dwChunkLength < (sizeof(WAVEFORMATEX)-2) ) )
		return E_FAIL;
	llOfs += 8;

	This->cbFmt = dwChunkLength;
	if ( dwChunkLength < sizeof(WAVEFORMATEX) )
		This->cbFmt = sizeof(WAVEFORMATEX);
	This->pFmt = (WAVEFORMATEX*)QUARTZ_AllocMem( dwChunkLength );
	if ( This->pFmt == NULL )
		return E_OUTOFMEMORY;
	ZeroMemory( This->pFmt, This->cbFmt );

	hr = IAsyncReader_SyncRead(
		pImpl->m_pReader, llOfs, dwChunkLength, (BYTE*)This->pFmt );
	if ( hr != S_OK )
	{
		if ( SUCCEEDED(hr) )
			hr = E_FAIL;
		return hr;
	}


	hr = RIFF_SearchChunk(
		pImpl, PARSER_RIFF_OfsFirst,
		PARSER_data, &llOfs, &dwChunkLength );
	if ( FAILED(hr) )
		return hr;
	if ( hr != S_OK || dwChunkLength == 0 )
		return E_FAIL;

	This->llDataStart = llOfs;
	This->llBytesTotal = (LONGLONG)dwChunkLength;

	return NOERROR;
}

static HRESULT CWavParseImpl_InitAU( CParserImpl* pImpl, CWavParseImpl* This )
{
	BYTE	au_hdr[24];
	DWORD	dataofs;
	DWORD	datalen;
	DWORD	datafmt;
	DWORD	datarate;
	DWORD	datachannels;
	HRESULT hr;
	WAVEFORMATEX	wfx;

	hr = IAsyncReader_SyncRead( pImpl->m_pReader, 0, 24, au_hdr );
	if ( FAILED(hr) )
		return hr;

	dataofs = PARSER_BE_UINT32(&au_hdr[4]);
	datalen = PARSER_BE_UINT32(&au_hdr[8]);
	datafmt = PARSER_BE_UINT32(&au_hdr[12]);
	datarate = PARSER_BE_UINT32(&au_hdr[16]);
	datachannels = PARSER_BE_UINT32(&au_hdr[20]);

	if ( dataofs < 24U || datalen == 0U )
		return E_FAIL;
	if ( datachannels != 1 && datachannels != 2 )
		return E_FAIL;

	ZeroMemory( &wfx, sizeof(WAVEFORMATEX) );
	wfx.nChannels = datachannels;
	wfx.nSamplesPerSec = datarate;

	switch ( datafmt )
	{
	case 1:
		wfx.wFormatTag = 7;
		wfx.nBlockAlign = datachannels;
		wfx.wBitsPerSample = 8;
		break;
	case 2:
		wfx.wFormatTag = 1;
		wfx.nBlockAlign = datachannels;
		wfx.wBitsPerSample = 8;
		This->iFmtType = WaveParse_Signed8;
		break;
	case 3:
		wfx.wFormatTag = 1;
		wfx.nBlockAlign = datachannels;
		wfx.wBitsPerSample = 16;
		This->iFmtType = WaveParse_Signed16BE;
		break;
	default:
		FIXME("audio/basic - unknown format %lu\n", datafmt );
		return E_FAIL;
	}
	wfx.nAvgBytesPerSec = (datarate * datachannels * (DWORD)wfx.wBitsPerSample) >> 3;

	This->cbFmt = sizeof(WAVEFORMATEX);
	This->pFmt = (WAVEFORMATEX*)QUARTZ_AllocMem( sizeof(WAVEFORMATEX) );
	if ( This->pFmt == NULL )
		return E_OUTOFMEMORY;
	memcpy( This->pFmt, &wfx, sizeof(WAVEFORMATEX) );

	This->llDataStart = dataofs;
	This->llBytesTotal = datalen;

	return NOERROR;
}

static HRESULT CWavParseImpl_InitAIFF( CParserImpl* pImpl, CWavParseImpl* This )
{
	FIXME( "AIFF is not supported now.\n" );
	return E_FAIL;
}

static HRESULT CWavParseImpl_InitParser( CParserImpl* pImpl, ULONG* pcStreams )
{
	CWavParseImpl*	This = NULL;
	HRESULT hr;
	BYTE	header[12];

	TRACE("(%p,%p)\n",pImpl,pcStreams);

	if ( pImpl->m_pReader == NULL )
		return E_UNEXPECTED;

	This = (CWavParseImpl*)QUARTZ_AllocMem( sizeof(CWavParseImpl) );
	if ( This == NULL )
		return E_OUTOFMEMORY;
	pImpl->m_pUserData = This;

	/* construct */
	This->cbFmt = 0;
	This->pFmt = NULL;
	This->dwBlockSize = 0;
	This->llDataStart = 0;
	This->llBytesTotal = 0;
	This->llBytesProcessed = 0;
	This->iFmtType = WaveParse_Native;

	hr = IAsyncReader_SyncRead( pImpl->m_pReader, 0, 12, header );
	if ( FAILED(hr) )
		return hr;
	if ( hr != S_OK )
		return E_FAIL;

	if ( !memcmp( &header[0], "RIFF", 4 ) &&
		 !memcmp( &header[8], "WAVE", 4 ) )
	{
		TRACE( "(%p) - it's audio/wav.\n", pImpl );
		hr = CWavParseImpl_InitWAV( pImpl, This );
	}
	else
	if ( !memcmp( &header[0], ".snd", 4 ) )
	{
		TRACE( "(%p) - it's audio/basic.\n", pImpl );
		hr = CWavParseImpl_InitAU( pImpl, This );
	}
	else
	if ( !memcmp( &header[0], "FORM", 4 ) &&
		 !memcmp( &header[8], "AIFF", 4 ) )
	{
		TRACE( "(%p) - it's audio/aiff.\n", pImpl );
		hr = CWavParseImpl_InitAIFF( pImpl, This );
	}
	else
	{
		FIXME( "(%p) - unknown format.\n", pImpl );
		hr = E_FAIL;
	}

	if ( FAILED(hr) )
	{
		return hr;
	}

	/* initialized successfully. */
	*pcStreams = 1;

	This->dwBlockSize = (This->pFmt->nAvgBytesPerSec + (DWORD)This->pFmt->nBlockAlign - 1U) / (DWORD)This->pFmt->nBlockAlign;

	TRACE( "(%p) returned successfully.\n", pImpl );

	return NOERROR;
}

static HRESULT CWavParseImpl_UninitParser( CParserImpl* pImpl )
{
	CWavParseImpl*	This = (CWavParseImpl*)pImpl->m_pUserData;

	TRACE("(%p)\n",This);

	if ( This == NULL )
		return NOERROR;

	/* destruct */
	if ( This->pFmt != NULL ) QUARTZ_FreeMem(This->pFmt);

	QUARTZ_FreeMem( This );
	pImpl->m_pUserData = NULL;

	return NOERROR;
}

static LPCWSTR CWavParseImpl_GetOutPinName( CParserImpl* pImpl, ULONG nStreamIndex )
{
	CWavParseImpl*	This = (CWavParseImpl*)pImpl->m_pUserData;

	TRACE("(%p)\n",This);

	return QUARTZ_WaveParserOutPin_Name;
}

static HRESULT CWavParseImpl_GetStreamType( CParserImpl* pImpl, ULONG nStreamIndex, AM_MEDIA_TYPE* pmt )
{
	CWavParseImpl*	This = (CWavParseImpl*)pImpl->m_pUserData;

	TRACE("(%p)\n",This);

	if ( This == NULL || This->pFmt == NULL )
		return E_UNEXPECTED;

	ZeroMemory( pmt, sizeof(AM_MEDIA_TYPE) );
	memcpy( &pmt->majortype, &MEDIATYPE_Audio, sizeof(GUID) );
	QUARTZ_MediaSubType_FromFourCC( &pmt->subtype, (DWORD)This->pFmt->wFormatTag );
	pmt->bFixedSizeSamples = 1;
	pmt->bTemporalCompression = 0;
	pmt->lSampleSize = This->pFmt->nBlockAlign;
	memcpy( &pmt->formattype, &FORMAT_WaveFormatEx, sizeof(GUID) );
	pmt->pUnk = NULL;

	pmt->pbFormat = (BYTE*)CoTaskMemAlloc( This->cbFmt );
	if ( pmt->pbFormat == NULL )
		return E_OUTOFMEMORY;
	pmt->cbFormat = This->cbFmt;
	memcpy( pmt->pbFormat, This->pFmt, This->cbFmt );

	return NOERROR;
}

static HRESULT CWavParseImpl_CheckStreamType( CParserImpl* pImpl, ULONG nStreamIndex, const AM_MEDIA_TYPE* pmt )
{
	if ( !IsEqualGUID( &pmt->majortype, &MEDIATYPE_Audio ) ||
		 !IsEqualGUID( &pmt->formattype, &FORMAT_WaveFormatEx ) )
		return E_FAIL;
	if ( pmt->pbFormat == NULL || pmt->cbFormat < sizeof(WAVEFORMATEX) )
		return E_FAIL;

	return NOERROR;
}

static HRESULT CWavParseImpl_GetAllocProp( CParserImpl* pImpl, ALLOCATOR_PROPERTIES* pReqProp )
{
	CWavParseImpl*	This = (CWavParseImpl*)pImpl->m_pUserData;

	TRACE("(%p)\n",This);

	if ( This == NULL || This->pFmt == NULL )
		return E_UNEXPECTED;

	ZeroMemory( pReqProp, sizeof(ALLOCATOR_PROPERTIES) );
	pReqProp->cBuffers = 1;
	pReqProp->cbBuffer = This->dwBlockSize;

	return NOERROR;
}

static HRESULT CWavParseImpl_GetNextRequest( CParserImpl* pImpl, ULONG* pnStreamIndex, LONGLONG* pllStart, LONG* plLength, REFERENCE_TIME* prtStart, REFERENCE_TIME* prtStop )
{
	CWavParseImpl*	This = (CWavParseImpl*)pImpl->m_pUserData;
	LONGLONG	llAvail;
	LONGLONG	llStart;
	LONGLONG	llEnd;

	TRACE("(%p)\n",This);

	if ( This == NULL || This->pFmt == NULL )
		return E_UNEXPECTED;

	llAvail = This->llBytesTotal - This->llBytesProcessed;
	if ( llAvail > (LONGLONG)This->dwBlockSize )
		llAvail = (LONGLONG)This->dwBlockSize;
	llStart = This->llBytesProcessed;
	llEnd = llStart + llAvail;
	This->llBytesProcessed = llEnd;

	*pllStart = This->llBytesProcessed;
	*plLength = (LONG)llAvail;
	*prtStart = llStart * QUARTZ_TIMEUNITS / (LONGLONG)This->pFmt->nAvgBytesPerSec;
	*prtStop = llEnd * QUARTZ_TIMEUNITS / (LONGLONG)This->pFmt->nAvgBytesPerSec;

	return NOERROR;
}

static HRESULT CWavParseImpl_ProcessSample( CParserImpl* pImpl, ULONG nStreamIndex, LONGLONG llStart, LONG lLength, IMediaSample* pSample )
{
	CWavParseImpl*	This = (CWavParseImpl*)pImpl->m_pUserData;

	TRACE("(%p)\n",This);

	switch ( This->iFmtType )
	{
	case WaveParse_Native:
		break;
	default:
		FIXME("(%p) - %d not implemented\n", This, This->iFmtType );
		return E_FAIL;
	}

	return NOERROR;
}


static const struct ParserHandlers CWavParseImpl_Handlers =
{
	CWavParseImpl_InitParser,
	CWavParseImpl_UninitParser,
	CWavParseImpl_GetOutPinName,
	CWavParseImpl_GetStreamType,
	CWavParseImpl_CheckStreamType,
	CWavParseImpl_GetAllocProp,
	CWavParseImpl_GetNextRequest,
	CWavParseImpl_ProcessSample,

	/* for IQualityControl */
	NULL, /* pQualityNotify */

	/* for seeking */
	NULL, /* pGetSeekingCaps */
	NULL, /* pIsTimeFormatSupported */
	NULL, /* pGetCurPos */
	NULL, /* pSetCurPos */
	NULL, /* pGetDuration */
	NULL, /* pSetDuration */
	NULL, /* pGetStopPos */
	NULL, /* pSetStopPos */
	NULL, /* pGetPreroll */
	NULL, /* pSetPreroll */
};

HRESULT QUARTZ_CreateWaveParser(IUnknown* punkOuter,void** ppobj)
{
	return QUARTZ_CreateParser(
		punkOuter,ppobj,
		&CLSID_quartzWaveParser,
		QUARTZ_WaveParser_Name,
		QUARTZ_WaveParserInPin_Name,
		&CWavParseImpl_Handlers );
}


