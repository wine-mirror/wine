/*
 * imaadp32.drv - IMA4 codec driver
 *
 * Copyright 2001 Hidenori Takeshima
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
 *
 *
 * FIXME - no encoding.
 */

#include "config.h"

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"
#include "winuser.h"
#include "mmsystem.h"
#include "msacm.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(imaadp32);


/***********************************************************************/

#define ACMDM_DRIVER_NOTIFY             (ACMDM_BASE + 1)
#define ACMDM_DRIVER_DETAILS            (ACMDM_BASE + 10)

#define ACMDM_HARDWARE_WAVE_CAPS_INPUT  (ACMDM_BASE + 20)
#define ACMDM_HARDWARE_WAVE_CAPS_OUTPUT (ACMDM_BASE + 21)

#define ACMDM_FORMATTAG_DETAILS         (ACMDM_BASE + 25)
#define ACMDM_FORMAT_DETAILS            (ACMDM_BASE + 26)
#define ACMDM_FORMAT_SUGGEST            (ACMDM_BASE + 27)

#define ACMDM_FILTERTAG_DETAILS         (ACMDM_BASE + 50)
#define ACMDM_FILTER_DETAILS            (ACMDM_BASE + 51)

#define ACMDM_STREAM_OPEN               (ACMDM_BASE + 76)
#define ACMDM_STREAM_CLOSE              (ACMDM_BASE + 77)
#define ACMDM_STREAM_SIZE               (ACMDM_BASE + 78)
#define ACMDM_STREAM_CONVERT            (ACMDM_BASE + 79)
#define ACMDM_STREAM_RESET              (ACMDM_BASE + 80)
#define ACMDM_STREAM_PREPARE            (ACMDM_BASE + 81)
#define ACMDM_STREAM_UNPREPARE          (ACMDM_BASE + 82)
#define ACMDM_STREAM_UPDATE             (ACMDM_BASE + 83)

typedef struct _ACMDRVSTREAMINSTANCE
{
  DWORD           cbStruct;
  PWAVEFORMATEX   pwfxSrc;
  PWAVEFORMATEX   pwfxDst;
  PWAVEFILTER     pwfltr;
  DWORD           dwCallback;
  DWORD           dwInstance;
  DWORD           fdwOpen;
  DWORD           fdwDriver;
  DWORD           dwDriver;
  HACMSTREAM    has;
} ACMDRVSTREAMINSTANCE, *PACMDRVSTREAMINSTANCE;

typedef struct _ACMDRVSTREAMHEADER *PACMDRVSTREAMHEADER;
typedef struct _ACMDRVSTREAMHEADER {
  DWORD  cbStruct;
  DWORD  fdwStatus;
  DWORD  dwUser;
  LPBYTE pbSrc;
  DWORD  cbSrcLength;
  DWORD  cbSrcLengthUsed;
  DWORD  dwSrcUser;
  LPBYTE pbDst;
  DWORD  cbDstLength;
  DWORD  cbDstLengthUsed;
  DWORD  dwDstUser;

  DWORD fdwConvert;
  PACMDRVSTREAMHEADER *padshNext;
  DWORD fdwDriver;
  DWORD dwDriver;

  /* Internal fields for ACM */
  DWORD  fdwPrepared;
  DWORD  dwPrepared;
  LPBYTE pbPreparedSrc;
  DWORD  cbPreparedSrcLength;
  LPBYTE pbPreparedDst;
  DWORD  cbPreparedDstLength;
} ACMDRVSTREAMHEADER;

typedef struct _ACMDRVSTREAMSIZE
{
  DWORD cbStruct;
  DWORD fdwSize;
  DWORD cbSrcLength;
  DWORD cbDstLength;
} ACMDRVSTREAMSIZE, *PACMDRVSTREAMSIZE;

typedef struct _ACMDRVFORMATSUGGEST
{
  DWORD           cbStruct;
  DWORD           fdwSuggest;
  PWAVEFORMATEX   pwfxSrc;
  DWORD           cbwfxSrc;
  PWAVEFORMATEX   pwfxDst;
  DWORD           cbwfxDst;
} ACMDRVFORMATSUGGEST, *PACMDRVFORMATSUGGEST;




/***********************************************************************/

enum CodecType
{
	CodecType_Invalid,
	CodecType_EncIMAADPCM,
	CodecType_DecIMAADPCM,
};

typedef struct CodecImpl
{
	int	dummy;
} CodecImpl;

/***********************************************************************/


static const int ima_step[88+1] =
{
	/* from Y.Ajima's WAVFMT.TXT */
	    7,     8,     9,    10,    11,    12,    13,    14,
	   16,    17,    19,    21,    23,    25,    28,    31,
	   34,    37,    41,    45,    50,    55,    60,    66,
	   73,    80,    88,    97,   107,   118,   130,   143,
	  157,   173,   190,   209,   230,   253,   279,   307,
	  337,   371,   408,   449,   494,   544,   598,   658,
	  724,   796,   876,   963,  1060,  1166,  1282,  1411,
	 1552,  1707,  1878,  2066,  2272,  2499,  2749,  3024,
	 3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,
	 7132,  7845,  8630,  9493, 10442, 11487, 12635, 13899,
	15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

static const int ima_indexupdate[8*2] =
{
	/* from Y.Ajima's WAVFMT.TXT */
	-1,-1,-1,-1, 2, 4, 6, 8,
	-1,-1,-1,-1, 2, 4, 6, 8,
};

static int stepindex_to_diff( int stepindex, int input )
{
	/* from Y.Ajima's WAVFMT.TXT */
	int absdiff;

	absdiff = (ima_step[stepindex]*((input&0x7)*2+1)) >> 3;
	return (input&0x8) ? (-absdiff) : absdiff;
}

static int update_stepindex( int oldindex, int input )
{
	int	index;

	index = oldindex + ima_indexupdate[input];
	return (index < 0) ? 0 : (index > 88) ? 88 : index;
}

static void decode_ima_block( int channels, int samplesperblock, SHORT* pDst, BYTE* pSrc )
{
	int	samp[2];
	int	stepindex[2];
	int	inputs[8];
	int	n,k,diff;

	for ( n = 0; n < channels; n++ )
	{
		samp[n] = *(SHORT*)pSrc; pSrc += sizeof(SHORT);
		stepindex[n] = *pSrc; pSrc += sizeof(SHORT);
		*pDst++ = samp[n];
	}
	samplesperblock --;

	while ( samplesperblock >= 8 )
	{
		for ( n = 0; n < channels; n++ )
		{
			for ( k = 0; k < 4; k++ )
			{
				inputs[k*2+0] = (*pSrc) & 0xf;
				inputs[k*2+1] = (*pSrc) >> 4;
				pSrc ++;
			}
			for ( k = 0; k < 8; k++ )
			{
				diff = stepindex_to_diff( stepindex[n], inputs[k] );
				stepindex[n] = update_stepindex( stepindex[n], inputs[k] );
				diff += samp[n];
				if ( diff < -32768 ) diff = -32768;
				if ( diff >  32767 ) diff =  32767;
				samp[n] = diff;
				pDst[k*channels+n] = samp[n];
			}
		}

		pDst += channels*8;
		samplesperblock -= 8;
	}
}

static LONG IMAADPCM32_Decode( int channels, int blockalign, int samplesperblock, BYTE* pbDst, DWORD cbDstLength, DWORD* pcbDstLengthUsed, BYTE* pbSrc, DWORD cbSrcLength, DWORD* pcbSrcLengthUsed )
{
	DWORD	cbDstLengthUsed = 0;
	DWORD	cbSrcLengthUsed = 0;
	int	dstblocksize;

	dstblocksize = samplesperblock*channels*sizeof(SHORT);
	while ( cbDstLength >= dstblocksize && cbSrcLength >= blockalign )
	{
		decode_ima_block( channels, samplesperblock, (SHORT*)pbDst, pbSrc );
		pbDst += dstblocksize;
		cbDstLength -= dstblocksize;
		cbDstLengthUsed += dstblocksize;
		pbSrc += blockalign;
		cbSrcLength -= blockalign;
		cbSrcLengthUsed += blockalign;
	}

	*pcbSrcLengthUsed = cbSrcLengthUsed;
	*pcbDstLengthUsed = cbDstLengthUsed;

	return MMSYSERR_NOERROR;
}


/***********************************************************************/

static LONG Codec_DrvQueryConfigure( CodecImpl* This )
{
	return MMSYSERR_NOTSUPPORTED;
}

static LONG Codec_DrvConfigure( CodecImpl* This, HWND hwnd, DRVCONFIGINFO* pinfo )
{
	return MMSYSERR_NOTSUPPORTED;
}

static LONG Codec_DriverDetails( ACMDRIVERDETAILSW* pDrvDetails )
{
	if ( pDrvDetails->cbStruct < sizeof(ACMDRIVERDETAILSW) )
		return MMSYSERR_INVALPARAM;

	ZeroMemory( pDrvDetails, sizeof(ACMDRIVERDETAILSW) );
	pDrvDetails->cbStruct = sizeof(ACMDRIVERDETAILSW);

	pDrvDetails->fccType = ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC;
	pDrvDetails->fccComp = ACMDRIVERDETAILS_FCCCOMP_UNDEFINED;
	pDrvDetails->wMid = 0xff;	/* FIXME? */
	pDrvDetails->wPid = 0x00;	/* FIXME? */
	pDrvDetails->vdwACM = 0x01000000;	/* FIXME? */
	pDrvDetails->vdwDriver = 0x01000000;	/* FIXME? */
	pDrvDetails->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CODEC;
	pDrvDetails->cFormatTags = 2;
	pDrvDetails->cFilterTags = 0;
	pDrvDetails->hicon = (HICON)NULL;
	MultiByteToWideChar( CP_ACP, 0, "WineIMA", -1,
		pDrvDetails->szShortName,
		sizeof(pDrvDetails->szShortName)/sizeof(WCHAR) );
	MultiByteToWideChar( CP_ACP, 0, "Wine IMA codec", -1,
		pDrvDetails->szLongName,
		sizeof(pDrvDetails->szLongName)/sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, "Brought to you by the Wine team...", -1,
		pDrvDetails->szCopyright,
		sizeof(pDrvDetails->szCopyright)/sizeof(WCHAR) );
	MultiByteToWideChar( CP_ACP, 0, "Refer to LICENSE file", -1,
		pDrvDetails->szLicensing,
		sizeof(pDrvDetails->szLicensing)/sizeof(WCHAR) );
	pDrvDetails->szFeatures[0] = 0;

	return MMSYSERR_NOERROR;
}

static LONG Codec_QueryAbout( void )
{
	return MMSYSERR_NOTSUPPORTED;
}

static LONG Codec_About( HWND hwnd )
{
	return MMSYSERR_NOTSUPPORTED;
}

/***********************************************************************/

static LONG Codec_FormatTagDetails( CodecImpl* This, ACMFORMATTAGDETAILSW* pFmtTagDetails, DWORD dwFlags )
{
	FIXME( "enumerate tags\n" );

	switch ( dwFlags )
	{
	case ACM_FORMATTAGDETAILSF_INDEX:
		switch ( pFmtTagDetails->dwFormatTagIndex )
		{
		case 0:
			pFmtTagDetails->dwFormatTag = 0x11;	/* IMA ADPCM */
			break;
		case 1:
			pFmtTagDetails->dwFormatTag = 1;	/* PCM */
			break;
		default:
			return ACMERR_NOTPOSSIBLE;
		}
		break;
	case ACM_FORMATTAGDETAILSF_FORMATTAG:
		switch ( pFmtTagDetails->dwFormatTag )
		{
		case 0x11:	/* IMA ADPCM */
			pFmtTagDetails->dwFormatTagIndex = 0;
			break;
		case 1:	/* PCM */
			pFmtTagDetails->dwFormatTagIndex = 1;
			break;
		default:
			return ACMERR_NOTPOSSIBLE;
		}
		break;
	case ACM_FORMATTAGDETAILSF_LARGESTSIZE:
		if ( pFmtTagDetails->dwFormatTag != 0 &&
			 pFmtTagDetails->dwFormatTag != 1 &&
			 pFmtTagDetails->dwFormatTag != 0x11 )
			return ACMERR_NOTPOSSIBLE;
		pFmtTagDetails->dwFormatTagIndex = 0;
		break;
	default:
		return MMSYSERR_NOTSUPPORTED;
	}

	pFmtTagDetails->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CODEC;
	pFmtTagDetails->cbFormatSize = sizeof(WAVEFORMATEX);
	pFmtTagDetails->cStandardFormats = 2;	/* FIXME */
	pFmtTagDetails->szFormatTag[0] = 0;	/* FIXME */

	return MMSYSERR_NOERROR;
}

static LONG Codec_FormatDetails( CodecImpl* This, ACMFORMATDETAILSW* pFmtDetails, DWORD dwFlags )
{
	FIXME( "enumerate standard formats\n" );

	if ( pFmtDetails->cbStruct < sizeof(ACMFORMATDETAILSW) )
		return MMSYSERR_INVALPARAM;
	pFmtDetails->cbStruct = sizeof(ACMFORMATDETAILSW);

	switch ( dwFlags )
	{
	case ACM_FORMATDETAILSF_INDEX:
		switch ( pFmtDetails->dwFormatIndex )
		{
		case 0:
			pFmtDetails->dwFormatTag = 0x11;	/* IMA ADPCM */
			break;
		case 1:
			pFmtDetails->dwFormatTag = 1;	/* PCM */
			break;
		default:
			return MMSYSERR_INVALPARAM;
		}
		break;
	case ACM_FORMATDETAILSF_FORMAT:
		switch ( pFmtDetails->dwFormatTag )
		{
		case 0x11:	/* IMA ADPCM */
			pFmtDetails->dwFormatIndex = 0;
			break;
		case 1:	/* PCM */
			pFmtDetails->dwFormatIndex = 1;
			break;
		default:
			return ACMERR_NOTPOSSIBLE;
		}
		break;
	default:
		return MMSYSERR_NOTSUPPORTED;
	}

	pFmtDetails->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CODEC;
	pFmtDetails->pwfx->wFormatTag = pFmtDetails->dwFormatTag;
	pFmtDetails->pwfx->nChannels = 1;
	pFmtDetails->pwfx->nSamplesPerSec = 11025;
	pFmtDetails->pwfx->wBitsPerSample = 4;
	if ( pFmtDetails->dwFormatTag == 1 )
	{
		pFmtDetails->cbwfx = sizeof(PCMWAVEFORMAT);
	}
	else
	{
		pFmtDetails->pwfx->cbSize = sizeof(WORD);
		pFmtDetails->cbwfx = sizeof(WAVEFORMATEX) + sizeof(WORD);
	}
	pFmtDetails->szFormat[0] = 0;	/* FIXME */

    return MMSYSERR_NOERROR;
}


static LONG Codec_FormatSuggest( CodecImpl* This, ACMDRVFORMATSUGGEST* pFmtSuggest )
{
	DWORD	fdwSuggest;

	FIXME( "get suggested format\n" );

	if ( pFmtSuggest->cbStruct != sizeof(ACMDRVFORMATSUGGEST) )
		return MMSYSERR_INVALPARAM;

	if ( pFmtSuggest->cbwfxSrc < sizeof(PCMWAVEFORMAT) ||
		 pFmtSuggest->cbwfxDst < sizeof(PCMWAVEFORMAT) )
		return MMSYSERR_INVALPARAM;

	fdwSuggest = pFmtSuggest->fdwSuggest;

	if ( fdwSuggest & ACM_FORMATSUGGESTF_NCHANNELS )
	{
		if ( pFmtSuggest->pwfxSrc->nChannels != pFmtSuggest->pwfxDst->nChannels )
			return ACMERR_NOTPOSSIBLE;
		fdwSuggest &= ~ACM_FORMATSUGGESTF_NCHANNELS;
	}

	if ( fdwSuggest & ACM_FORMATSUGGESTF_NSAMPLESPERSEC )
	{
		if ( pFmtSuggest->pwfxSrc->nSamplesPerSec != pFmtSuggest->pwfxDst->nSamplesPerSec )
			return ACMERR_NOTPOSSIBLE;
		fdwSuggest &= ~ACM_FORMATSUGGESTF_NSAMPLESPERSEC;
	}

	if ( pFmtSuggest->pwfxSrc->wFormatTag == 1 )
	{
		/* Compressor */
		if ( pFmtSuggest->cbwfxDst < (sizeof(WAVEFORMATEX)+sizeof(WORD)) )
			return MMSYSERR_INVALPARAM;
		if ( pFmtSuggest->pwfxSrc->wBitsPerSample != 16 )
			return ACMERR_NOTPOSSIBLE;

		if ( fdwSuggest & ACM_FORMATSUGGESTF_WFORMATTAG )
		{
			if ( pFmtSuggest->pwfxDst->wFormatTag != 0x11 )
				return ACMERR_NOTPOSSIBLE;
			fdwSuggest &= ~ACM_FORMATSUGGESTF_WFORMATTAG;
		}

		if ( fdwSuggest & ACM_FORMATSUGGESTF_WBITSPERSAMPLE )
		{
			if ( pFmtSuggest->pwfxDst->wBitsPerSample != 4 )
				return ACMERR_NOTPOSSIBLE;
			fdwSuggest &= ~ACM_FORMATSUGGESTF_WFORMATTAG;
		}

		if ( fdwSuggest != 0 )
			return MMSYSERR_INVALFLAG;

		if ( !(fdwSuggest & ACM_FORMATSUGGESTF_WFORMATTAG) )
			pFmtSuggest->pwfxDst->wFormatTag = 0x11;
		pFmtSuggest->pwfxDst->nChannels = pFmtSuggest->pwfxSrc->nChannels;
		pFmtSuggest->pwfxDst->nSamplesPerSec = pFmtSuggest->pwfxSrc->nSamplesPerSec;
		pFmtSuggest->pwfxDst->nAvgBytesPerSec = 0; /* FIXME */
		pFmtSuggest->pwfxDst->nBlockAlign = 0; /* FIXME */
		pFmtSuggest->pwfxDst->wBitsPerSample = 4;
		pFmtSuggest->pwfxDst->cbSize = 2;

		FIXME( "no compressor" );
		return ACMERR_NOTPOSSIBLE;
	}
	else
	{
		/* Decompressor */
		if ( pFmtSuggest->cbwfxSrc < (sizeof(WAVEFORMATEX)+sizeof(WORD)) )
			return MMSYSERR_INVALPARAM;
		if ( pFmtSuggest->pwfxSrc->wFormatTag != 0x11 )
			return ACMERR_NOTPOSSIBLE;
		if ( pFmtSuggest->pwfxSrc->wBitsPerSample != 4 )
			return ACMERR_NOTPOSSIBLE;

		if ( fdwSuggest & ACM_FORMATSUGGESTF_WFORMATTAG )
		{
			if ( pFmtSuggest->pwfxDst->wFormatTag != 1 )
				return ACMERR_NOTPOSSIBLE;
			fdwSuggest &= ~ACM_FORMATSUGGESTF_WFORMATTAG;
		}

		if ( fdwSuggest & ACM_FORMATSUGGESTF_WBITSPERSAMPLE )
		{
			if ( pFmtSuggest->pwfxDst->wBitsPerSample != 16 )
				return ACMERR_NOTPOSSIBLE;
			fdwSuggest &= ~ACM_FORMATSUGGESTF_WFORMATTAG;
		}

		if ( fdwSuggest != 0 )
			return MMSYSERR_INVALFLAG;

		pFmtSuggest->pwfxDst->wFormatTag = 1;
		pFmtSuggest->pwfxDst->nChannels = pFmtSuggest->pwfxSrc->nChannels;
		pFmtSuggest->pwfxDst->nSamplesPerSec = pFmtSuggest->pwfxSrc->nSamplesPerSec;
		pFmtSuggest->pwfxDst->nAvgBytesPerSec = pFmtSuggest->pwfxSrc->nSamplesPerSec * pFmtSuggest->pwfxSrc->nChannels * 2;
		pFmtSuggest->pwfxDst->nBlockAlign = pFmtSuggest->pwfxSrc->nChannels * 2;
		pFmtSuggest->pwfxDst->wBitsPerSample = 16;
	}

	return MMSYSERR_NOERROR;
}

static LONG Codec_FilterTagDetails( CodecImpl* This, ACMFILTERTAGDETAILSW* pFilterTagDetails, DWORD dwFlags )
{
	/* This is a codec driver. */
	return MMSYSERR_NOTSUPPORTED;
}

static LONG Codec_FilterDetails( CodecImpl* This, ACMFILTERDETAILSW* pFilterDetails, DWORD dwFlags )
{
	/* This is a codec driver. */
	return MMSYSERR_NOTSUPPORTED;
}

static LONG Codec_StreamOpen( CodecImpl* This, ACMDRVSTREAMINSTANCE* pStreamInst )
{
	enum CodecType	codectype = CodecType_Invalid;

	if ( pStreamInst->cbStruct != sizeof(ACMDRVSTREAMINSTANCE) )
	{
		TRACE("invalid size of struct\n");
		return MMSYSERR_INVALPARAM;
	}

	if ( pStreamInst->fdwOpen & (~(ACM_STREAMOPENF_ASYNC|ACM_STREAMOPENF_NONREALTIME|ACM_STREAMOPENF_QUERY|CALLBACK_EVENT|CALLBACK_FUNCTION|CALLBACK_WINDOW)) )
	{
		TRACE("unknown flags\n");
		return MMSYSERR_INVALFLAG;
	}

	/* No support for async operations. */
	if ( pStreamInst->fdwOpen & ACM_STREAMOPENF_ASYNC )
		return MMSYSERR_INVALFLAG;

	/* This is a codec driver. */
	if ( pStreamInst->pwfxSrc->nChannels != pStreamInst->pwfxDst->nChannels || pStreamInst->pwfxSrc->nSamplesPerSec != pStreamInst->pwfxDst->nSamplesPerSec )
		return ACMERR_NOTPOSSIBLE;
	if ( pStreamInst->pwfltr != NULL )
		return ACMERR_NOTPOSSIBLE;

	if ( pStreamInst->pwfxSrc->wFormatTag == 1 )
	{
		if ( pStreamInst->pwfxSrc->wBitsPerSample != 16 )
			return ACMERR_NOTPOSSIBLE;
		if ( pStreamInst->pwfxDst->wBitsPerSample != 4 )
			return ACMERR_NOTPOSSIBLE;

		/* Queried as a compressor */
		FIXME( "Compressor is not implemented now\n" );
		return ACMERR_NOTPOSSIBLE;
	}
	else
	if ( pStreamInst->pwfxDst->wFormatTag == 1 )
	{
		if ( pStreamInst->pwfxDst->wBitsPerSample != 16 )
			return ACMERR_NOTPOSSIBLE;
		if ( pStreamInst->pwfxSrc->wBitsPerSample != 4 )
			return ACMERR_NOTPOSSIBLE;

		switch ( pStreamInst->pwfxSrc->wFormatTag )
		{
		case 0x11: /* IMA ADPCM */
			TRACE( "IMG ADPCM deompressor\n" );
			codectype = CodecType_DecIMAADPCM;
			break;
		default:
			return ACMERR_NOTPOSSIBLE;
		}
	}
	else
	{
		return ACMERR_NOTPOSSIBLE;
	}

	if ( pStreamInst->fdwOpen & ACM_STREAMOPENF_QUERY )
		return MMSYSERR_NOERROR;

	pStreamInst->dwDriver = (DWORD)codectype;

	return MMSYSERR_NOERROR;
}

static LONG Codec_StreamClose( CodecImpl* This, ACMDRVSTREAMINSTANCE* pStreamInst )
{
	return MMSYSERR_NOERROR;
}

static LONG Codec_StreamSize( CodecImpl* This, ACMDRVSTREAMINSTANCE* pStreamInst, ACMDRVSTREAMSIZE* pStreamSize )
{
	enum CodecType	codectype;
	LONG	res;

	if ( pStreamSize->cbStruct != sizeof(ACMDRVSTREAMSIZE) )
		return MMSYSERR_INVALPARAM;

	codectype = (enum CodecType)pStreamInst->dwDriver;

	res = MMSYSERR_NOERROR;
	FIXME("()\n");
	switch ( codectype )
	{
	case CodecType_EncIMAADPCM:
		if ( pStreamSize->fdwSize == ACM_STREAMSIZEF_SOURCE )
			pStreamSize->cbDstLength = 64 + (pStreamSize->cbSrcLength >> 2);
		else
		if ( pStreamSize->fdwSize == ACM_STREAMSIZEF_DESTINATION )
			pStreamSize->cbSrcLength = pStreamSize->cbDstLength << 2;
		else
			res = MMSYSERR_INVALFLAG;
		break;
	case CodecType_DecIMAADPCM:
		if ( pStreamSize->fdwSize == ACM_STREAMSIZEF_SOURCE )
			pStreamSize->cbDstLength = pStreamSize->cbSrcLength << 2;
		else
		if ( pStreamSize->fdwSize == ACM_STREAMSIZEF_DESTINATION )
			pStreamSize->cbSrcLength = 64 + (pStreamSize->cbDstLength >> 2);
		else
			res = MMSYSERR_INVALFLAG;
		break;
	default:
		ERR( "CodecType_Invalid\n" );
		res = MMSYSERR_NOTSUPPORTED;
		break;
	}

	return res;
}

static LONG Codec_StreamConvert( CodecImpl* This, ACMDRVSTREAMINSTANCE* pStreamInst, ACMDRVSTREAMHEADER* pStreamHdr )
{
	enum CodecType	codectype;
	LONG	res;

	codectype = (enum CodecType)pStreamInst->dwDriver;

	res = MMSYSERR_NOTSUPPORTED;
	switch ( codectype )
	{
	case CodecType_EncIMAADPCM:
		FIXME( "CodecType_EncIMAADPCM\n" );
		break;
	case CodecType_DecIMAADPCM:
		TRACE( "CodecType_DecIMAADPCM\n" );
		res = IMAADPCM32_Decode( pStreamInst->pwfxSrc->nChannels, pStreamInst->pwfxSrc->nBlockAlign, *(WORD*)(((BYTE*)pStreamInst->pwfxSrc) + sizeof(WAVEFORMATEX)), pStreamHdr->pbDst, pStreamHdr->cbDstLength, &pStreamHdr->cbDstLengthUsed, pStreamHdr->pbSrc, pStreamHdr->cbSrcLength, &pStreamHdr->cbSrcLengthUsed );
		break;
	default:
		ERR( "CodecType_Invalid\n" );
		break;
	}

	return res;
}

static LONG Codec_StreamReset( CodecImpl* This, ACMDRVSTREAMINSTANCE* pStreamInst, DWORD dwFlags )
{
	return MMSYSERR_NOTSUPPORTED;
}

static LONG Codec_StreamPrepare( CodecImpl* This, ACMDRVSTREAMINSTANCE* pStreamInst, ACMDRVSTREAMHEADER* pStreamHdr )
{
	return MMSYSERR_NOTSUPPORTED;
}

static LONG Codec_StreamUnprepare( CodecImpl* This, ACMDRVSTREAMINSTANCE* pStreamInst, ACMDRVSTREAMHEADER* pStreamHdr )
{
	return MMSYSERR_NOTSUPPORTED;
}



/***********************************************************************/

static CodecImpl* Codec_AllocDriver( void )
{
	CodecImpl*	This;

	This = HeapAlloc( GetProcessHeap(), 0, sizeof(CodecImpl) );
	if ( This == NULL )
		return NULL;
	ZeroMemory( This, sizeof(CodecImpl) );

	/* initialize members. */

	return This;
}

static void Codec_Close( CodecImpl* This )
{

	HeapFree( GetProcessHeap(), 0, This );
}



/***********************************************************************/

LONG WINAPI IMAADP32_DriverProc(
	DWORD dwDriverId, HDRVR hdrvr, UINT msg, LONG lParam1, LONG lParam2 )
{
	TRACE( "DriverProc(%08lx,%08x,%08x,%08lx,%08lx)\n",
			 dwDriverId, hdrvr, msg, lParam1, lParam2 );

	switch ( msg )
	{
	case DRV_LOAD:
		TRACE("DRV_LOAD\n");
		return TRUE;
	case DRV_FREE:
		TRACE("DRV_FREE\n");
		return TRUE;
	case DRV_OPEN:
		TRACE("DRV_OPEN\n");
		return (LONG)Codec_AllocDriver();
	case DRV_CLOSE:
		TRACE("DRV_CLOSE\n");
		Codec_Close( (CodecImpl*)dwDriverId );
		return TRUE;
	case DRV_ENABLE:
		TRACE("DRV_ENABLE\n");
		return TRUE;
	case DRV_DISABLE:
		TRACE("DRV_DISABLE\n");
		return TRUE;
	case DRV_QUERYCONFIGURE:
		TRACE("DRV_QUERYCONFIGURE\n");
		return Codec_DrvQueryConfigure( (CodecImpl*)dwDriverId );
	case DRV_CONFIGURE:
		TRACE("DRV_CONFIGURE\n");
		return Codec_DrvConfigure( (CodecImpl*)dwDriverId,
					(HWND)lParam1, (DRVCONFIGINFO*)lParam2 );
	case DRV_INSTALL:
		TRACE("DRV_INSTALL\n");
		return DRVCNF_OK;
	case DRV_REMOVE:
		TRACE("DRV_REMOVE\n");
		return 0;
	case DRV_POWER:
		TRACE("DRV_POWER\n");
		return TRUE;

	case ACMDM_DRIVER_NOTIFY:
		return MMSYSERR_NOERROR;
	case ACMDM_DRIVER_DETAILS:
		return Codec_DriverDetails((ACMDRIVERDETAILSW*)lParam1);
	case ACMDM_DRIVER_ABOUT:
		TRACE("ACMDM_DRIVER_ABOUT\n");
		return (lParam1 == -1) ? Codec_QueryAbout() : Codec_About( (HWND)lParam1 );

	case ACMDM_HARDWARE_WAVE_CAPS_INPUT:
		return MMSYSERR_NOTSUPPORTED;
	case ACMDM_HARDWARE_WAVE_CAPS_OUTPUT:
		return MMSYSERR_NOTSUPPORTED;

	case ACMDM_FORMATTAG_DETAILS:
		return Codec_FormatTagDetails( (CodecImpl*)dwDriverId, (ACMFORMATTAGDETAILSW*)lParam1, (DWORD)lParam2 );
	case ACMDM_FORMAT_DETAILS:
		return Codec_FormatDetails( (CodecImpl*)dwDriverId, (ACMFORMATDETAILSW*)lParam1, (DWORD)lParam2 );
	case ACMDM_FORMAT_SUGGEST:
		return Codec_FormatSuggest( (CodecImpl*)dwDriverId, (ACMDRVFORMATSUGGEST*)lParam1 );

	case ACMDM_FILTERTAG_DETAILS:
		return Codec_FilterTagDetails( (CodecImpl*)dwDriverId, (ACMFILTERTAGDETAILSW*)lParam1, (DWORD)lParam2 );
	case ACMDM_FILTER_DETAILS:
		return Codec_FilterDetails( (CodecImpl*)dwDriverId, (ACMFILTERDETAILSW*)lParam1, (DWORD)lParam2 );

	case ACMDM_STREAM_OPEN:
		return Codec_StreamOpen( (CodecImpl*)dwDriverId, (ACMDRVSTREAMINSTANCE*)lParam1 );
	case ACMDM_STREAM_CLOSE:
		return Codec_StreamClose( (CodecImpl*)dwDriverId, (ACMDRVSTREAMINSTANCE*)lParam1 );
	case ACMDM_STREAM_SIZE:
		return Codec_StreamSize( (CodecImpl*)dwDriverId, (ACMDRVSTREAMINSTANCE*)lParam1, (ACMDRVSTREAMSIZE*)lParam2 );
	case ACMDM_STREAM_CONVERT:
		return Codec_StreamConvert( (CodecImpl*)dwDriverId, (ACMDRVSTREAMINSTANCE*)lParam1, (ACMDRVSTREAMHEADER*)lParam2 );
	case ACMDM_STREAM_RESET:
		return Codec_StreamReset( (CodecImpl*)dwDriverId, (ACMDRVSTREAMINSTANCE*)lParam1, (DWORD)lParam2 );
	case ACMDM_STREAM_PREPARE:
		return Codec_StreamPrepare( (CodecImpl*)dwDriverId, (ACMDRVSTREAMINSTANCE*)lParam1, (ACMDRVSTREAMHEADER*)lParam2 );
	case ACMDM_STREAM_UNPREPARE:
		return Codec_StreamUnprepare( (CodecImpl*)dwDriverId, (ACMDRVSTREAMINSTANCE*)lParam1, (ACMDRVSTREAMHEADER*)lParam2 );

	}

	return DefDriverProc( dwDriverId, hdrvr, msg, lParam1, lParam2 );
}

/***********************************************************************/

BOOL WINAPI IMAADP32_DllMain( HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved )
{
	TRACE( "(%08x,%08lx,%p)\n",hInst,dwReason,lpvReserved );

	return TRUE;
}
