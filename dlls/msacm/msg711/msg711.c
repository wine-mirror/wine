/*
 * msg711.drv - G711 codec driver
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

WINE_DEFAULT_DEBUG_CHANNEL(msg711);


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
	CodecType_EncMuLaw,
	CodecType_EncALaw,
	CodecType_DecMuLaw,
	CodecType_DecALaw,
};

typedef struct CodecImpl
{
	int	dummy;
} CodecImpl;

/***********************************************************************/

static const WORD dec_mulaw[256] =
{
	0x8284, 0x8684, 0x8A84, 0x8E84, 0x9284, 0x9684, 0x9A84, 0x9E84,
	0xA284, 0xA684, 0xAA84, 0xAE84, 0xB284, 0xB684, 0xBA84, 0xBE84,
	0xC184, 0xC384, 0xC584, 0xC784, 0xC984, 0xCB84, 0xCD84, 0xCF84,
	0xD184, 0xD384, 0xD584, 0xD784, 0xD984, 0xDB84, 0xDD84, 0xDF84,
	0xE104, 0xE204, 0xE304, 0xE404, 0xE504, 0xE604, 0xE704, 0xE804,
	0xE904, 0xEA04, 0xEB04, 0xEC04, 0xED04, 0xEE04, 0xEF04, 0xF004,
	0xF0C4, 0xF144, 0xF1C4, 0xF244, 0xF2C4, 0xF344, 0xF3C4, 0xF444,
	0xF4C4, 0xF544, 0xF5C4, 0xF644, 0xF6C4, 0xF744, 0xF7C4, 0xF844,
	0xF8A4, 0xF8E4, 0xF924, 0xF964, 0xF9A4, 0xF9E4, 0xFA24, 0xFA64,
	0xFAA4, 0xFAE4, 0xFB24, 0xFB64, 0xFBA4, 0xFBE4, 0xFC24, 0xFC64,
	0xFC94, 0xFCB4, 0xFCD4, 0xFCF4, 0xFD14, 0xFD34, 0xFD54, 0xFD74,
	0xFD94, 0xFDB4, 0xFDD4, 0xFDF4, 0xFE14, 0xFE34, 0xFE54, 0xFE74,
	0xFE8C, 0xFE9C, 0xFEAC, 0xFEBC, 0xFECC, 0xFEDC, 0xFEEC, 0xFEFC,
	0xFF0C, 0xFF1C, 0xFF2C, 0xFF3C, 0xFF4C, 0xFF5C, 0xFF6C, 0xFF7C,
	0xFF88, 0xFF90, 0xFF98, 0xFFA0, 0xFFA8, 0xFFB0, 0xFFB8, 0xFFC0,
	0xFFC8, 0xFFD0, 0xFFD8, 0xFFE0, 0xFFE8, 0xFFF0, 0xFFF8, 0x0000,
	0x7D7C, 0x797C, 0x757C, 0x717C, 0x6D7C, 0x697C, 0x657C, 0x617C,
	0x5D7C, 0x597C, 0x557C, 0x517C, 0x4D7C, 0x497C, 0x457C, 0x417C,
	0x3E7C, 0x3C7C, 0x3A7C, 0x387C, 0x367C, 0x347C, 0x327C, 0x307C,
	0x2E7C, 0x2C7C, 0x2A7C, 0x287C, 0x267C, 0x247C, 0x227C, 0x207C,
	0x1EFC, 0x1DFC, 0x1CFC, 0x1BFC, 0x1AFC, 0x19FC, 0x18FC, 0x17FC,
	0x16FC, 0x15FC, 0x14FC, 0x13FC, 0x12FC, 0x11FC, 0x10FC, 0x0FFC,
	0x0F3C, 0x0EBC, 0x0E3C, 0x0DBC, 0x0D3C, 0x0CBC, 0x0C3C, 0x0BBC,
	0x0B3C, 0x0ABC, 0x0A3C, 0x09BC, 0x093C, 0x08BC, 0x083C, 0x07BC,
	0x075C, 0x071C, 0x06DC, 0x069C, 0x065C, 0x061C, 0x05DC, 0x059C,
	0x055C, 0x051C, 0x04DC, 0x049C, 0x045C, 0x041C, 0x03DC, 0x039C,
	0x036C, 0x034C, 0x032C, 0x030C, 0x02EC, 0x02CC, 0x02AC, 0x028C,
	0x026C, 0x024C, 0x022C, 0x020C, 0x01EC, 0x01CC, 0x01AC, 0x018C,
	0x0174, 0x0164, 0x0154, 0x0144, 0x0134, 0x0124, 0x0114, 0x0104,
	0x00F4, 0x00E4, 0x00D4, 0x00C4, 0x00B4, 0x00A4, 0x0094, 0x0084,
	0x0078, 0x0070, 0x0068, 0x0060, 0x0058, 0x0050, 0x0048, 0x0040,
	0x0038, 0x0030, 0x0028, 0x0020, 0x0018, 0x0010, 0x0008, 0x0000,
};

static const WORD dec_alaw[256] =
{
	0xEA80, 0xEB80, 0xE880, 0xE980, 0xEE80, 0xEF80, 0xEC80, 0xED80,
	0xE280, 0xE380, 0xE080, 0xE180, 0xE680, 0xE780, 0xE480, 0xE580,
	0xF540, 0xF5C0, 0xF440, 0xF4C0, 0xF740, 0xF7C0, 0xF640, 0xF6C0,
	0xF140, 0xF1C0, 0xF040, 0xF0C0, 0xF340, 0xF3C0, 0xF240, 0xF2C0,
	0xAA00, 0xAE00, 0xA200, 0xA600, 0xBA00, 0xBE00, 0xB200, 0xB600,
	0x8A00, 0x8E00, 0x8200, 0x8600, 0x9A00, 0x9E00, 0x9200, 0x9600,
	0xD500, 0xD700, 0xD100, 0xD300, 0xDD00, 0xDF00, 0xD900, 0xDB00,
	0xC500, 0xC700, 0xC100, 0xC300, 0xCD00, 0xCF00, 0xC900, 0xCB00,
	0xFEA8, 0xFEB8, 0xFE88, 0xFE98, 0xFEE8, 0xFEF8, 0xFEC8, 0xFED8,
	0xFE28, 0xFE38, 0xFE08, 0xFE18, 0xFE68, 0xFE78, 0xFE48, 0xFE58,
	0xFFA8, 0xFFB8, 0xFF88, 0xFF98, 0xFFE8, 0xFFF8, 0xFFC8, 0xFFD8,
	0xFF28, 0xFF38, 0xFF08, 0xFF18, 0xFF68, 0xFF78, 0xFF48, 0xFF58,
	0xFAA0, 0xFAE0, 0xFA20, 0xFA60, 0xFBA0, 0xFBE0, 0xFB20, 0xFB60,
	0xF8A0, 0xF8E0, 0xF820, 0xF860, 0xF9A0, 0xF9E0, 0xF920, 0xF960,
	0xFD50, 0xFD70, 0xFD10, 0xFD30, 0xFDD0, 0xFDF0, 0xFD90, 0xFDB0,
	0xFC50, 0xFC70, 0xFC10, 0xFC30, 0xFCD0, 0xFCF0, 0xFC90, 0xFCB0,
	0x1580, 0x1480, 0x1780, 0x1680, 0x1180, 0x1080, 0x1380, 0x1280,
	0x1D80, 0x1C80, 0x1F80, 0x1E80, 0x1980, 0x1880, 0x1B80, 0x1A80,
	0x0AC0, 0x0A40, 0x0BC0, 0x0B40, 0x08C0, 0x0840, 0x09C0, 0x0940,
	0x0EC0, 0x0E40, 0x0FC0, 0x0F40, 0x0CC0, 0x0C40, 0x0DC0, 0x0D40,
	0x5600, 0x5200, 0x5E00, 0x5A00, 0x4600, 0x4200, 0x4E00, 0x4A00,
	0x7600, 0x7200, 0x7E00, 0x7A00, 0x6600, 0x6200, 0x6E00, 0x6A00,
	0x2B00, 0x2900, 0x2F00, 0x2D00, 0x2300, 0x2100, 0x2700, 0x2500,
	0x3B00, 0x3900, 0x3F00, 0x3D00, 0x3300, 0x3100, 0x3700, 0x3500,
	0x0158, 0x0148, 0x0178, 0x0168, 0x0118, 0x0108, 0x0138, 0x0128,
	0x01D8, 0x01C8, 0x01F8, 0x01E8, 0x0198, 0x0188, 0x01B8, 0x01A8,
	0x0058, 0x0048, 0x0078, 0x0068, 0x0018, 0x0008, 0x0038, 0x0028,
	0x00D8, 0x00C8, 0x00F8, 0x00E8, 0x0098, 0x0088, 0x00B8, 0x00A8,
	0x0560, 0x0520, 0x05E0, 0x05A0, 0x0460, 0x0420, 0x04E0, 0x04A0,
	0x0760, 0x0720, 0x07E0, 0x07A0, 0x0660, 0x0620, 0x06E0, 0x06A0,
	0x02B0, 0x0290, 0x02F0, 0x02D0, 0x0230, 0x0210, 0x0270, 0x0250,
	0x03B0, 0x0390, 0x03F0, 0x03D0, 0x0330, 0x0310, 0x0370, 0x0350,
};

static LONG MSG711_Decode( const WORD* pdec, BYTE* pbDst, DWORD cbDstLength, DWORD* pcbDstLengthUsed, BYTE* pbSrc, DWORD cbSrcLength, DWORD* pcbSrcLengthUsed )
{
	DWORD	cSample;
	WORD	w;

	cSample = cbSrcLength;
	if ( cSample > (cbDstLength>>1) )
		cSample = (cbDstLength>>1);

	*pcbSrcLengthUsed = cSample;
	*pcbDstLengthUsed = cSample << 1;

	while ( cSample-- > 0 )
	{
		w = pdec[*pbSrc++];
		*pbDst++ = LOBYTE(w);
		*pbDst++ = HIBYTE(w);
	}

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
	pDrvDetails->cFormatTags = 3;
	pDrvDetails->cFilterTags = 0;
	pDrvDetails->hicon = (HICON)NULL;
	MultiByteToWideChar( CP_ACP, 0, "WineG711", -1,
		pDrvDetails->szShortName,
		sizeof(pDrvDetails->szShortName)/sizeof(WCHAR) );
	MultiByteToWideChar( CP_ACP, 0, "Wine G711 codec", -1,
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
			pFmtTagDetails->dwFormatTag = 7;	/* Mu-Law */
			break;
		case 1:
			pFmtTagDetails->dwFormatTag = 6;	/* A-Law */
			break;
		case 2:
			pFmtTagDetails->dwFormatTag = 1;	/* PCM */
			break;
		default:
			return ACMERR_NOTPOSSIBLE;
		}
		break;
	case ACM_FORMATTAGDETAILSF_FORMATTAG:
		switch ( pFmtTagDetails->dwFormatTag )
		{
		case 7:	/* Mu-Law */
			pFmtTagDetails->dwFormatTagIndex = 0;
			break;
		case 6:	/* A-Law */
			pFmtTagDetails->dwFormatTagIndex = 1;
			break;
		case 1:	/* PCM */
			pFmtTagDetails->dwFormatTagIndex = 2;
			break;
		default:
			return ACMERR_NOTPOSSIBLE;
		}
		break;
	case ACM_FORMATTAGDETAILSF_LARGESTSIZE:
		if ( pFmtTagDetails->dwFormatTag != 0 &&
			 pFmtTagDetails->dwFormatTag != 1 &&
			 pFmtTagDetails->dwFormatTag != 6 &&
			 pFmtTagDetails->dwFormatTag != 7 )
			return ACMERR_NOTPOSSIBLE;
		pFmtTagDetails->dwFormatTagIndex = 0;
		break;
	default:
		return MMSYSERR_NOTSUPPORTED;
	}

	pFmtTagDetails->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CODEC;
	pFmtTagDetails->cbFormatSize = sizeof(WAVEFORMATEX);
	pFmtTagDetails->cStandardFormats = 3;	/* FIXME */
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
			pFmtDetails->dwFormatTag = 7;	/* Mu-Law */
			break;
		case 1:
			pFmtDetails->dwFormatTag = 6;	/* A-Law */
			break;
		case 2:
			pFmtDetails->dwFormatTag = 1;	/* PCM */
			break;
		default:
			return MMSYSERR_INVALPARAM;
		}
		break;
	case ACM_FORMATDETAILSF_FORMAT:
		switch ( pFmtDetails->dwFormatTag )
		{
		case 7:	/* Mu-Law */
			pFmtDetails->dwFormatIndex = 0;
			break;
		case 6:	/* A-Law */
			pFmtDetails->dwFormatIndex = 1;
			break;
		case 1:	/* PCM */
			pFmtDetails->dwFormatIndex = 2;
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
	pFmtDetails->pwfx->nSamplesPerSec = 8000;
	pFmtDetails->pwfx->wBitsPerSample = 8;
	if ( pFmtDetails->dwFormatTag == 1 )
	{
		pFmtDetails->cbwfx = sizeof(PCMWAVEFORMAT);
	}
	else
	{
		pFmtDetails->pwfx->cbSize = 0;
		pFmtDetails->cbwfx = sizeof(WAVEFORMATEX);
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
		if ( pFmtSuggest->cbwfxDst < sizeof(WAVEFORMATEX) )
			return MMSYSERR_INVALPARAM;
		if ( pFmtSuggest->pwfxSrc->wBitsPerSample != 16 )
			return ACMERR_NOTPOSSIBLE;

		if ( fdwSuggest & ACM_FORMATSUGGESTF_WFORMATTAG )
		{
			if ( pFmtSuggest->pwfxDst->wFormatTag != 6 &&
				 pFmtSuggest->pwfxDst->wFormatTag != 7 )
				return ACMERR_NOTPOSSIBLE;
			fdwSuggest &= ~ACM_FORMATSUGGESTF_WFORMATTAG;
		}

		if ( fdwSuggest & ACM_FORMATSUGGESTF_WBITSPERSAMPLE )
		{
			if ( pFmtSuggest->pwfxDst->wBitsPerSample != 8 )
				return ACMERR_NOTPOSSIBLE;
			fdwSuggest &= ~ACM_FORMATSUGGESTF_WFORMATTAG;
		}

		if ( fdwSuggest != 0 )
			return MMSYSERR_INVALFLAG;

		if ( !(fdwSuggest & ACM_FORMATSUGGESTF_WFORMATTAG) )
			pFmtSuggest->pwfxDst->wFormatTag = 7;
		pFmtSuggest->pwfxDst->nChannels = pFmtSuggest->pwfxSrc->nChannels;
		pFmtSuggest->pwfxDst->nSamplesPerSec = pFmtSuggest->pwfxSrc->nSamplesPerSec;
		pFmtSuggest->pwfxDst->nAvgBytesPerSec = pFmtSuggest->pwfxSrc->nSamplesPerSec * pFmtSuggest->pwfxSrc->nChannels;
		pFmtSuggest->pwfxDst->nBlockAlign = pFmtSuggest->pwfxSrc->nChannels;
		pFmtSuggest->pwfxDst->wBitsPerSample = 8;
		pFmtSuggest->pwfxDst->cbSize = 0;

		FIXME( "no compressor" );
		return ACMERR_NOTPOSSIBLE;
	}
	else
	{
		/* Decompressor */
		if ( pFmtSuggest->cbwfxSrc < sizeof(WAVEFORMATEX) )
			return MMSYSERR_INVALPARAM;
		if ( pFmtSuggest->pwfxSrc->wFormatTag != 6 &&
			 pFmtSuggest->pwfxSrc->wFormatTag != 7 )
			return ACMERR_NOTPOSSIBLE;
		if ( pFmtSuggest->pwfxSrc->wBitsPerSample != 8 )
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
		if ( pStreamInst->pwfxDst->wBitsPerSample != 8 )
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
		if ( pStreamInst->pwfxSrc->wBitsPerSample != 8 )
			return ACMERR_NOTPOSSIBLE;

		switch ( pStreamInst->pwfxSrc->wFormatTag )
		{
		case 6: /* A-Law */
			TRACE( "A-Law deompressor\n" );
			codectype = CodecType_DecALaw;
			break;
		case 7: /* Mu-Law */
			TRACE( "Mu-Law deompressor\n" );
			codectype = CodecType_DecMuLaw;
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
	switch ( codectype )
	{
	case CodecType_EncMuLaw:
	case CodecType_EncALaw:
		if ( pStreamSize->fdwSize == ACM_STREAMSIZEF_SOURCE )
			pStreamSize->cbDstLength = pStreamSize->cbSrcLength >> 1;
		else
		if ( pStreamSize->fdwSize == ACM_STREAMSIZEF_DESTINATION )
			pStreamSize->cbSrcLength = pStreamSize->cbDstLength << 1;
		else
			res = MMSYSERR_INVALFLAG;
		break;
	case CodecType_DecMuLaw:
	case CodecType_DecALaw:
		if ( pStreamSize->fdwSize == ACM_STREAMSIZEF_SOURCE )
			pStreamSize->cbDstLength = pStreamSize->cbSrcLength << 1;
		else
		if ( pStreamSize->fdwSize == ACM_STREAMSIZEF_DESTINATION )
			pStreamSize->cbSrcLength = pStreamSize->cbDstLength >> 1;
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
	case CodecType_EncMuLaw:
		FIXME( "CodecType_EncMuLaw\n" );
		break;
	case CodecType_EncALaw:
		FIXME( "CodecType_EncALaw\n" );
		break;
	case CodecType_DecMuLaw:
		TRACE( "CodecType_DecMuLaw\n" );
		res = MSG711_Decode( dec_mulaw, pStreamHdr->pbDst, pStreamHdr->cbDstLength, &pStreamHdr->cbDstLengthUsed, pStreamHdr->pbSrc, pStreamHdr->cbSrcLength, &pStreamHdr->cbSrcLengthUsed );
		break;
	case CodecType_DecALaw:
		TRACE( "CodecType_DecALaw\n" );
		res = MSG711_Decode( dec_alaw, pStreamHdr->pbDst, pStreamHdr->cbDstLength, &pStreamHdr->cbDstLengthUsed, pStreamHdr->pbSrc, pStreamHdr->cbSrcLength, &pStreamHdr->cbSrcLengthUsed );
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

LONG WINAPI MSG711_DriverProc(
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

BOOL WINAPI MSG711_DllMain( HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved )
{
	TRACE( "(%08x,%08lx,%p)\n",hInst,dwReason,lpvReserved );

	return TRUE;
}
