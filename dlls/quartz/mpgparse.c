/*
 * Implements MPEG-1 / MPEG-2 Parser(Splitter).
 *
 *	FIXME - no splitter implementation.
 *	FIXME - no packet support (implemented payload only)
 *	FIXME - no seeking
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
#include "mmsystem.h"
#include "vfw.h"
#include "winerror.h"
#include "strmif.h"
#include "control.h"
#include "vfwmsgs.h"
#include "amvideo.h"
#include "mmreg.h"
#include "uuids.h"
#include "dvdmedia.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "parser.h"
#include "mtype.h"

static const WCHAR QUARTZ_MPEG1Parser_Name[] =
{ 'M','P','E','G','-','I',' ','S','p','l','i','t','t','e','r',0 };
static const WCHAR QUARTZ_MPEG2Parser_Name[] =
{ 'M','P','E','G','-','2',' ','S','p','l','i','t','t','e','r',0 };
static const WCHAR QUARTZ_MPGParserInPin_Name[] =
{ 'I','n',0 };
static const WCHAR QUARTZ_MPGParserOutPin_VideoPinName[] =
{ 'V','i','d','e','o',0 };
static const WCHAR QUARTZ_MPGParserOutPin_AudioPinName[] =
{ 'A','u','d','i','o',0 };
	/* FIXME */
static const WCHAR QUARTZ_MPGParserOutPin_UnknownTypePinName[] =
{ 'O','u','t',0 };



static DWORD bitratesl1[16] =
	{  0, 32, 64, 96,128,160,192,224,256,288,320,352,384,416,448,  0};
static DWORD bitratesl2[16] =
	{  0, 32, 48, 56, 64, 80, 96,112,128,160,192,224,256,320,384,  0};
static DWORD bitratesl3[16] =
	{  0, 32, 40, 48, 56, 64, 80, 96,112,128,160,192,224,256,320,  0};


/****************************************************************************
 *
 *	CMPGParseImpl
 */


typedef struct CMPGParseImpl CMPGParseImpl;
typedef struct CMPGParsePayload CMPGParsePayload;

enum MPGPayloadType
{
	MPGPayload_Video = 0xe0,
	MPGPayload_Audio = 0xc0,
};

struct CMPGParseImpl
{
	LONGLONG	llPosNext;
	BOOL	bRawPayload;
	DWORD	dwPayloadBlockSizeMax;
	DWORD	cPayloads;
	CMPGParsePayload*	pPayloads;
};

struct CMPGParsePayload
{
	enum MPGPayloadType	payloadtype;
	BOOL	bDataDiscontinuity;
};


static HRESULT CMPGParseImpl_SyncReadPayload(
	CParserImpl* pImpl, CMPGParseImpl* This,
	enum MPGPayloadType payloadtype,
	LONGLONG llPosStart, LONG lLength, BYTE* pbBuf )
{
	if ( This == NULL || This->pPayloads == NULL )
		return E_UNEXPECTED;

	if ( This->bRawPayload )
	{
		if ( payloadtype != This->pPayloads[0].payloadtype )
			return E_UNEXPECTED;
		return IAsyncReader_SyncRead( pImpl->m_pReader, llPosStart, lLength, pbBuf );
	}
	else
	{
		FIXME( "not implemented\n" );
	}

	return E_NOTIMPL;
}



static HRESULT CMPGParseImpl_InitParser( CParserImpl* pImpl, ULONG* pcStreams )
{
	CMPGParseImpl*	This = NULL;
	HRESULT hr;
	BYTE	hdrbuf[8];

	TRACE("(%p,%p)\n",pImpl,pcStreams);

	This = (CMPGParseImpl*)QUARTZ_AllocMem( sizeof(CMPGParseImpl) );
	if ( This == NULL )
		return E_OUTOFMEMORY;
	pImpl->m_pUserData = This;
	ZeroMemory( This, sizeof(CMPGParseImpl) );

	hr = IAsyncReader_SyncRead( pImpl->m_pReader, 0, 8, hdrbuf );
	if ( FAILED(hr) )
		return hr;
	if ( hr != S_OK )
		return E_FAIL;

	if ( hdrbuf[0] == 0x00 && hdrbuf[1] == 0x00 &&
		 hdrbuf[2] == 0x01 && hdrbuf[3] == 0xba )
	{
		This->bRawPayload = FALSE;
		This->dwPayloadBlockSizeMax = 0;

		FIXME( "no mpeg/system support\n" );
		return E_FAIL;
	}
	else
	if ( hdrbuf[0] == 0x00 && hdrbuf[1] == 0x00 &&
		 hdrbuf[2] == 0x01 && hdrbuf[3] == 0xb3 )
	{
		TRACE( "mpeg/video payload\n" );
		This->llPosNext = 0;
		This->bRawPayload = TRUE;
		This->dwPayloadBlockSizeMax = 0x4000;
		This->cPayloads = 1;
		This->pPayloads = (CMPGParsePayload*)QUARTZ_AllocMem( sizeof(CMPGParsePayload) );
		if ( This->pPayloads == NULL )
			return E_OUTOFMEMORY;
		*pcStreams = 1;
		This->pPayloads[0].payloadtype = MPGPayload_Video;
		This->pPayloads[0].bDataDiscontinuity = TRUE;
	}
	else
	if ( hdrbuf[0] == 0xff && (hdrbuf[1]&0xf0) == 0xf0 )
	{
		TRACE( "mpeg/audio payload\n" );
		This->llPosNext = 0;
		This->bRawPayload = TRUE;
		This->dwPayloadBlockSizeMax = 0;
		This->cPayloads = 1;
		This->pPayloads = (CMPGParsePayload*)QUARTZ_AllocMem( sizeof(CMPGParsePayload) );
		if ( This->pPayloads == NULL )
			return E_OUTOFMEMORY;
		*pcStreams = 1;
		This->pPayloads[0].payloadtype = MPGPayload_Audio;
		This->pPayloads[0].bDataDiscontinuity = TRUE;
	}
	else
	{
		return E_FAIL;
	}

	return S_OK;
}

static HRESULT CMPGParseImpl_UninitParser( CParserImpl* pImpl )
{
	CMPGParseImpl*	This = (CMPGParseImpl*)pImpl->m_pUserData;
	ULONG	nIndex;

	TRACE("(%p)\n",This);

	if ( This == NULL )
		return NOERROR;

	/* destruct */

	if ( This->pPayloads != NULL )
	{
		for ( nIndex = 0; nIndex < This->cPayloads; nIndex++ )
		{
			/* release this stream */

			
		}
		QUARTZ_FreeMem( This->pPayloads );
		This->pPayloads = NULL;
	}

	QUARTZ_FreeMem( This );
	pImpl->m_pUserData = NULL;

	return NOERROR;
}

static LPCWSTR CMPGParseImpl_GetOutPinName( CParserImpl* pImpl, ULONG nStreamIndex )
{
	CMPGParseImpl*	This = (CMPGParseImpl*)pImpl->m_pUserData;

	TRACE("(%p,%lu)\n",This,nStreamIndex);

	if ( This == NULL || nStreamIndex >= This->cPayloads )
		return NULL;

	switch ( This->pPayloads[nStreamIndex].payloadtype )
	{
	case MPGPayload_Video:
		return QUARTZ_MPGParserOutPin_VideoPinName;
	case MPGPayload_Audio:
		return QUARTZ_MPGParserOutPin_AudioPinName;
	default:
		FIXME("mpeg - unknown stream type %02x\n",This->pPayloads[nStreamIndex].payloadtype);
	}

	/* FIXME */
	return QUARTZ_MPGParserOutPin_UnknownTypePinName;
}

static HRESULT CMPGParseImpl_GetStreamType( CParserImpl* pImpl, ULONG nStreamIndex, AM_MEDIA_TYPE* pmt )
{
	CMPGParseImpl*	This = (CMPGParseImpl*)pImpl->m_pUserData;
	HRESULT	hr;
	BYTE	hdrbuf[140+10];
	UINT	seqhdrlen;
	MPEG1VIDEOINFO*	pmpg1vi;
	MPEG2VIDEOINFO*	pmpg2vi;
	MPEG1WAVEFORMAT*	pmpg1wav;
	enum MPGPayloadType	payloadtype;
	DWORD	dwPayloadBlockSize;

	TRACE("(%p,%lu,%p)\n",This,nStreamIndex,pmt);

	if ( This == NULL )
		return E_UNEXPECTED;
	if ( nStreamIndex >= This->cPayloads )
		return E_INVALIDARG;

	ZeroMemory( pmt, sizeof(AM_MEDIA_TYPE) );

	payloadtype = This->pPayloads[nStreamIndex].payloadtype;
	switch ( payloadtype )
	{
	case MPGPayload_Video:
		hr = CMPGParseImpl_SyncReadPayload(
			pImpl, This, payloadtype, 0, 140+10, hdrbuf );
		if ( FAILED(hr) )
			return hr;
		if ( hr != S_OK )
			return E_FAIL;

		memcpy( &pmt->majortype, &MEDIATYPE_Video, sizeof(GUID) );
		seqhdrlen = 12;
		if ( hdrbuf[seqhdrlen-1] & 0x2 )
			seqhdrlen += 64;
		if ( hdrbuf[seqhdrlen-1] & 0x1 )
			seqhdrlen += 64;
		if ( hdrbuf[seqhdrlen  ] == 0x00 && hdrbuf[seqhdrlen+1] == 0x00 &&
			 hdrbuf[seqhdrlen+2] == 0x01 && hdrbuf[seqhdrlen+3] == 0xb5 )
		{
			/* video MPEG-2 */
			FIXME("video MPEG-2\n");
			if ( (hdrbuf[seqhdrlen+4]&0xf0) != 0x1 )
				return E_FAIL;
			memcpy( &pmt->subtype, &MEDIASUBTYPE_MPEG2_VIDEO, sizeof(GUID) );
			memcpy( &pmt->formattype, &FORMAT_MPEG2_VIDEO, sizeof(GUID) );
			pmt->bFixedSizeSamples = 0;
			pmt->bTemporalCompression = 1;
			pmt->lSampleSize = 0;
			pmt->cbFormat = sizeof(MPEG2VIDEOINFO);
			pmt->pbFormat = (BYTE*)CoTaskMemAlloc( sizeof(MPEG2VIDEOINFO) );
			if ( pmt->pbFormat == NULL )
				return E_OUTOFMEMORY;
			ZeroMemory( pmt->pbFormat, sizeof(MPEG2VIDEOINFO) );
			pmpg2vi = (MPEG2VIDEOINFO*)pmt->pbFormat;
			pmpg2vi->hdr.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			pmpg2vi->hdr.bmiHeader.biWidth = ((UINT)hdrbuf[4] << 4) | ((UINT)hdrbuf[5] >> 4); /* FIXME! */
			pmpg2vi->hdr.bmiHeader.biHeight = (((UINT)hdrbuf[5] & 0xf) << 8) | ((UINT)hdrbuf[6]); /* FIXME! */
			pmpg2vi->hdr.dwInterlaceFlags = AMINTERLACE_FieldPatBothRegular; /* FIXME? */
			pmpg2vi->hdr.dwCopyProtectFlags = AMCOPYPROTECT_RestrictDuplication; /* FIXME? */
			pmpg2vi->hdr.dwPictAspectRatioX = 1; /* FIXME? */
			pmpg2vi->hdr.dwPictAspectRatioY = 1; /* FIXME? */
			pmpg2vi->dwStartTimeCode = 0;
			pmpg2vi->cbSequenceHeader = seqhdrlen + 10;
			switch ( hdrbuf[seqhdrlen+4] & 0xf )
			{
			case 5: pmpg2vi->dwProfile = AM_MPEG2Profile_Simple; break;
			case 4: pmpg2vi->dwProfile = AM_MPEG2Profile_Main; break;
			case 3: pmpg2vi->dwProfile = AM_MPEG2Profile_SNRScalable; break;
			case 2: pmpg2vi->dwProfile = AM_MPEG2Profile_SpatiallyScalable; break;
			case 1: pmpg2vi->dwProfile = AM_MPEG2Profile_High; break;
			default: return E_FAIL;
			}
			switch ( hdrbuf[seqhdrlen+5] >> 4 )
			{
			case 10: pmpg2vi->dwLevel = AM_MPEG2Level_Low; break;
			case  8: pmpg2vi->dwLevel = AM_MPEG2Level_Main; break;
			case  6: pmpg2vi->dwLevel = AM_MPEG2Level_High1440; break;
			case  4: pmpg2vi->dwLevel = AM_MPEG2Level_High; break;
			default: return E_FAIL;
			}
			pmpg2vi->dwFlags = 0; /* FIXME? */
			memcpy( pmpg2vi->dwSequenceHeader, hdrbuf, seqhdrlen + 10 );

			return S_OK;
		}
		else
		{
			/* MPEG-1 */
			memcpy( &pmt->subtype, &MEDIASUBTYPE_MPEG1Payload, sizeof(GUID) );
			memcpy( &pmt->formattype, &FORMAT_MPEGVideo, sizeof(GUID) );
			pmt->bFixedSizeSamples = 0;
			pmt->bTemporalCompression = 1;
			pmt->lSampleSize = 0;
			pmt->cbFormat = sizeof(MPEG1VIDEOINFO);
			pmt->pbFormat = (BYTE*)CoTaskMemAlloc( sizeof(MPEG1VIDEOINFO) );
			if ( pmt->pbFormat == NULL )
				return E_OUTOFMEMORY;
			ZeroMemory( pmt->pbFormat, sizeof(MPEG1VIDEOINFO) );
			pmpg1vi = (MPEG1VIDEOINFO*)pmt->pbFormat;
			pmpg1vi->hdr.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			pmpg1vi->hdr.bmiHeader.biWidth = ((UINT)hdrbuf[4] << 4) | ((UINT)hdrbuf[5] >> 4);
			pmpg1vi->hdr.bmiHeader.biHeight = (((UINT)hdrbuf[5] & 0xf) << 8) | ((UINT)hdrbuf[6]);
			pmpg1vi->hdr.bmiHeader.biPlanes = 1;
			pmpg1vi->dwStartTimeCode = 0;
			pmpg1vi->cbSequenceHeader = seqhdrlen;
			memcpy( pmpg1vi->bSequenceHeader, hdrbuf, seqhdrlen );
		}

		return S_OK;
	case MPGPayload_Audio:
		hr = CMPGParseImpl_SyncReadPayload(
			pImpl, This, payloadtype, 0, 4, hdrbuf );
		if ( FAILED(hr) )
			return hr;
		if ( hr != S_OK )
			return E_FAIL;

		memcpy( &pmt->majortype, &MEDIATYPE_Audio, sizeof(GUID) );
		memcpy( &pmt->formattype, &FORMAT_WaveFormatEx, sizeof(GUID) );

		if ( !( hdrbuf[1] & 0x8 ) )
		{
			/* FIXME!!! */
			FIXME("audio not MPEG-1\n");
			return E_FAIL;
		}
		else
		{
			/* MPEG-1 */
			memcpy( &pmt->subtype, &MEDIASUBTYPE_MPEG1AudioPayload, sizeof(GUID) );
			pmt->bFixedSizeSamples = 0;
			pmt->bTemporalCompression = 1;
			pmt->lSampleSize = 0;
			pmt->cbFormat = sizeof(MPEG1WAVEFORMAT);
			pmt->pbFormat = (BYTE*)CoTaskMemAlloc( sizeof(MPEG1WAVEFORMAT) );
			if ( pmt->pbFormat == NULL )
				return E_OUTOFMEMORY;
			ZeroMemory( pmt->pbFormat, sizeof(MPEG1WAVEFORMAT) );
			pmpg1wav = (MPEG1WAVEFORMAT*)pmt->pbFormat;
			switch ( hdrbuf[1] & 0x6 )
			{
			case 0x6: pmpg1wav->fwHeadLayer = ACM_MPEG_LAYER1;
			case 0x4: pmpg1wav->fwHeadLayer = ACM_MPEG_LAYER2;
			case 0x2: pmpg1wav->fwHeadLayer = ACM_MPEG_LAYER3;
			default: return E_FAIL;
			}

			switch ( pmpg1wav->fwHeadLayer )
			{
			case ACM_MPEG_LAYER1:
				pmpg1wav->dwHeadBitrate = bitratesl1[hdrbuf[2]>>4]*1000;
				break;
			case ACM_MPEG_LAYER2:
				pmpg1wav->dwHeadBitrate = bitratesl2[hdrbuf[2]>>4]*1000;
				break;
			case ACM_MPEG_LAYER3:
				pmpg1wav->dwHeadBitrate = bitratesl3[hdrbuf[2]>>4]*1000;
				break;
			}
			if ( pmpg1wav->dwHeadBitrate == 0 )
				return E_FAIL;

			switch ( hdrbuf[3] & 0xc0 )
			{
			case 0x00: pmpg1wav->fwHeadMode = ACM_MPEG_STEREO;
			case 0x40: pmpg1wav->fwHeadMode = ACM_MPEG_JOINTSTEREO;
			case 0x80: pmpg1wav->fwHeadMode = ACM_MPEG_DUALCHANNEL;
			case 0xc0: pmpg1wav->fwHeadMode = ACM_MPEG_SINGLECHANNEL;
			}

			pmpg1wav->fwHeadModeExt = (hdrbuf[3] & 0x30) >> 4; /* FIXME?? */
			pmpg1wav->wHeadEmphasis = (hdrbuf[3] & 0x03); /* FIXME?? */
			pmpg1wav->fwHeadFlags = ACM_MPEG_ID_MPEG1;
			if ( hdrbuf[1] & 0x1 )
				pmpg1wav->fwHeadFlags |= ACM_MPEG_PROTECTIONBIT;
			if ( hdrbuf[2] & 0x1 )
				pmpg1wav->fwHeadFlags |= ACM_MPEG_PRIVATEBIT;
			if ( hdrbuf[3] & 0x8 )
				pmpg1wav->fwHeadFlags |= ACM_MPEG_COPYRIGHT;
			if ( hdrbuf[3] & 0x4 )
				pmpg1wav->fwHeadFlags |= ACM_MPEG_ORIGINALHOME;
			pmpg1wav->dwPTSLow = 0;
			pmpg1wav->dwPTSHigh = 0;

			pmpg1wav->wfx.wFormatTag = WAVE_FORMAT_MPEG;
			pmpg1wav->wfx.nChannels = (pmpg1wav->fwHeadMode != ACM_MPEG_SINGLECHANNEL) ? 2 : 1;
			switch ( hdrbuf[2] & 0x0c )
			{
			case 0x00: pmpg1wav->wfx.nSamplesPerSec = 44100;
			case 0x01: pmpg1wav->wfx.nSamplesPerSec = 48000;
			case 0x02: pmpg1wav->wfx.nSamplesPerSec = 32000;
			default: return E_FAIL;
			}
			pmpg1wav->wfx.nAvgBytesPerSec = pmpg1wav->dwHeadBitrate >> 3;
			switch ( pmpg1wav->fwHeadLayer )
			{
			case ACM_MPEG_LAYER1:
				pmpg1wav->wfx.nBlockAlign = (384>>3) * pmpg1wav->dwHeadBitrate / pmpg1wav->wfx.nSamplesPerSec;
				break;
			case ACM_MPEG_LAYER2:
				pmpg1wav->wfx.nBlockAlign = (1152>>3) * pmpg1wav->dwHeadBitrate / pmpg1wav->wfx.nSamplesPerSec;
				break;
			case ACM_MPEG_LAYER3:
				pmpg1wav->wfx.nBlockAlign = 1;
				break;
			}
			pmpg1wav->wfx.wBitsPerSample = 0;
			pmpg1wav->wfx.cbSize = sizeof(MPEG1WAVEFORMAT) - sizeof(WAVEFORMATEX);
			if ( pmpg1wav->fwHeadLayer != ACM_MPEG_LAYER3 )
			{
				pmt->bFixedSizeSamples = 1;
				pmt->lSampleSize = pmpg1wav->wfx.nBlockAlign;
			}
			dwPayloadBlockSize = (pmpg1wav->wfx.nAvgBytesPerSec + pmpg1wav->wfx.nBlockAlign - 1) / pmpg1wav->wfx.nBlockAlign;
			if ( dwPayloadBlockSize > This->dwPayloadBlockSizeMax )
				This->dwPayloadBlockSizeMax = dwPayloadBlockSize;
		}

		return S_OK;
	default:
		FIXME("mpeg - unknown stream type %02x\n",This->pPayloads[nStreamIndex].payloadtype);
		break;
	}

	FIXME("stub\n");
	return E_NOTIMPL;
}

static HRESULT CMPGParseImpl_CheckStreamType( CParserImpl* pImpl, ULONG nStreamIndex, const AM_MEDIA_TYPE* pmt )
{
	CMPGParseImpl*	This = (CMPGParseImpl*)pImpl->m_pUserData;
	HRESULT hr;
	AM_MEDIA_TYPE	mt;
	MPEG1VIDEOINFO*	pmpg1vi;
	MPEG1VIDEOINFO*	pmpg1viCheck;
	MPEG2VIDEOINFO*	pmpg2vi;
	MPEG2VIDEOINFO*	pmpg2viCheck;
	WAVEFORMATEX*	pwfx;
	WAVEFORMATEX*	pwfxCheck;
	enum MPGPayloadType	payloadtype;

	TRACE("(%p,%lu,%p)\n",This,nStreamIndex,pmt);

	if ( This == NULL )
		return E_UNEXPECTED;
	if ( nStreamIndex >= This->cPayloads )
		return E_INVALIDARG;

	hr = CMPGParseImpl_GetStreamType( pImpl, nStreamIndex, &mt );
	if ( FAILED(hr) )
		return hr;
	if ( !IsEqualGUID( &pmt->majortype, &mt.majortype ) ||
		 !IsEqualGUID( &pmt->subtype, &mt.subtype ) ||
		 !IsEqualGUID( &pmt->formattype, &mt.formattype ) )
	{
		hr = E_FAIL;
		goto end;
	}

	TRACE("check format\n");
	hr = S_OK;

	payloadtype = This->pPayloads[nStreamIndex].payloadtype;
	switch ( payloadtype )
	{
	case MPGPayload_Video:
		if ( IsEqualGUID( &mt.formattype, &FORMAT_MPEGVideo ) )
		{
			/* MPEG-1 Video */
			if ( pmt->cbFormat != mt.cbFormat ||
				 pmt->pbFormat == NULL )
			{
				hr = E_FAIL;
				goto end;
			}
			pmpg1vi = (MPEG1VIDEOINFO*)mt.pbFormat;
			pmpg1viCheck = (MPEG1VIDEOINFO*)pmt->pbFormat;
			if ( memcmp( pmpg1vi, pmpg1viCheck, sizeof(MPEG1VIDEOINFO) ) != 0 )
			{
				hr = E_FAIL;
				goto end;
			}
		}
		else
		if ( IsEqualGUID( &mt.formattype, &FORMAT_MPEG2_VIDEO ) )
		{
			/* MPEG-2 Video */
			if ( pmt->cbFormat != mt.cbFormat ||
				 pmt->pbFormat == NULL )
			{
				hr = E_FAIL;
				goto end;
			}
			pmpg2vi = (MPEG2VIDEOINFO*)mt.pbFormat;
			pmpg2viCheck = (MPEG2VIDEOINFO*)pmt->pbFormat;
			if ( memcmp( pmpg2vi, pmpg2viCheck, sizeof(MPEG2VIDEOINFO) ) != 0 )
			{
				hr = E_FAIL;
				goto end;
			}
		}
		else
		{
			hr = E_FAIL;
			goto end;
		}
		break;
	case MPGPayload_Audio:
		if ( IsEqualGUID( &mt.formattype, &FORMAT_WaveFormatEx ) )
		{
			if ( mt.cbFormat != pmt->cbFormat ||
				 pmt->pbFormat == NULL )
			{
				hr = E_FAIL;
				goto end;
			}
			pwfx = (WAVEFORMATEX*)mt.pbFormat;
			pwfxCheck = (WAVEFORMATEX*)pmt->pbFormat;

			if ( memcmp( pwfx, pwfxCheck, sizeof(WAVEFORMATEX) ) != 0 )
			{
				hr = E_FAIL;
				goto end;
			}
		}
		else
		{
			hr = E_FAIL;
			goto end;
		}

		break;
	default:
		FIXME( "unsupported payload type\n" );
		hr = E_FAIL;
		goto end;
	}

	hr = S_OK;
end:
	QUARTZ_MediaType_Free( &mt );

	TRACE("%08lx\n",hr);

	return hr;
}

static HRESULT CMPGParseImpl_GetAllocProp( CParserImpl* pImpl, ALLOCATOR_PROPERTIES* pReqProp )
{
	CMPGParseImpl*	This = (CMPGParseImpl*)pImpl->m_pUserData;

	TRACE("(%p,%p)\n",This,pReqProp);

	if ( This == NULL )
		return E_UNEXPECTED;

	ZeroMemory( pReqProp, sizeof(ALLOCATOR_PROPERTIES) );
	pReqProp->cBuffers = This->cPayloads;
	pReqProp->cbBuffer = This->dwPayloadBlockSizeMax;

	return S_OK;
}

static HRESULT CMPGParseImpl_GetNextRequest( CParserImpl* pImpl, ULONG* pnStreamIndex, LONGLONG* pllStart, LONG* plLength, REFERENCE_TIME* prtStart, REFERENCE_TIME* prtStop, DWORD* pdwSampleFlags )
{
	CMPGParseImpl*	This = (CMPGParseImpl*)pImpl->m_pUserData;

	if ( This == NULL )
		return E_UNEXPECTED;
	*pdwSampleFlags = AM_SAMPLE_SPLICEPOINT;

	TRACE("(%p)\n",This);

	if ( This->bRawPayload )
	{
		if ( This->dwPayloadBlockSizeMax == 0 ||
			 This->cPayloads != 1 || This->pPayloads == NULL )
			return E_UNEXPECTED;
		*pnStreamIndex = 0;
		*pllStart = This->llPosNext;
		*plLength = This->dwPayloadBlockSizeMax;
		*prtStart = 0;
		*prtStop = 0;
		if ( This->pPayloads[0].bDataDiscontinuity )
		{
			*pdwSampleFlags |= AM_SAMPLE_DATADISCONTINUITY;
			This->pPayloads[0].bDataDiscontinuity = FALSE;
		}
	}
	else
	{
		FIXME("stub\n");
		return E_NOTIMPL;
	}

	return S_OK;
}

static HRESULT CMPGParseImpl_ProcessSample( CParserImpl* pImpl, ULONG nStreamIndex, LONGLONG llStart, LONG lLength, IMediaSample* pSample )
{
	CMPGParseImpl*	This = (CMPGParseImpl*)pImpl->m_pUserData;
	HRESULT hr;

	TRACE("(%p,%lu,%ld,%ld,%p)\n",This,nStreamIndex,(long)llStart,lLength,pSample);

	if ( This == NULL )
		return E_UNEXPECTED;

	if ( This->bRawPayload )
	{
		hr = IMediaSample_SetTime(pSample,NULL,NULL);
		if ( FAILED(hr) )
			return hr;
	}

	return NOERROR;
}


static const struct ParserHandlers CMPGParseImpl_Handlers =
{
	CMPGParseImpl_InitParser,
	CMPGParseImpl_UninitParser,
	CMPGParseImpl_GetOutPinName,
	CMPGParseImpl_GetStreamType,
	CMPGParseImpl_CheckStreamType,
	CMPGParseImpl_GetAllocProp,
	CMPGParseImpl_GetNextRequest,
	CMPGParseImpl_ProcessSample,

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

HRESULT QUARTZ_CreateMPEG1Splitter(IUnknown* punkOuter,void** ppobj)
{
	return QUARTZ_CreateParser(
		punkOuter,ppobj,
		&CLSID_MPEG1Splitter,
		QUARTZ_MPEG1Parser_Name,
		QUARTZ_MPGParserInPin_Name,
		&CMPGParseImpl_Handlers );
}

HRESULT QUARTZ_CreateMPEG2Splitter(IUnknown* punkOuter,void** ppobj)
{
	return QUARTZ_CreateParser(
		punkOuter,ppobj,
		&CLSID_MMSPLITTER,
		QUARTZ_MPEG2Parser_Name,
		QUARTZ_MPGParserInPin_Name,
		&CMPGParseImpl_Handlers );
}


