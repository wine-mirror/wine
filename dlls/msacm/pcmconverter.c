/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 *      MSACM32 library
 *
 *      Copyright 2000		Eric Pouech
 */

#include <assert.h>
#include "wine/winestring.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "msacm.h"
#include "msacmdrv.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(msacm);

static	DWORD	PCM_drvOpen(LPCSTR str)
{
    return 1;
}

static	DWORD	PCM_drvClose(DWORD dwDevID)
{
    return 1;
}

static	struct {
    int		nChannels;
    int		nBits;
    int		rate;
} PCM_Formats[] = {
    {1,  8,  8000},
    {2,  8,  8000},
    {1, 16,  8000},
    {2, 16,  8000},
    {1,  8, 11025},
    {2,  8, 11025},
    {1, 16, 11025},
    {2, 16, 11025},
    {1,  8, 22050},
    {2,  8, 22050},
    {1, 16, 22050},
    {2, 16, 22050},
    {1,  8, 44100},
    {2,  8, 44100},
    {1, 16, 44100},
    {2, 16, 44100},
};

#define	NUM_PCM_FORMATS	(sizeof(PCM_Formats) / sizeof(PCM_Formats[0]))

static	DWORD	PCM_GetFormatIndex(LPWAVEFORMATEX wfx)
{
    int i;
    
    for (i = 0; i < NUM_PCM_FORMATS; i++) {
	if (wfx->nChannels == PCM_Formats[i].nChannels &&
	    wfx->nSamplesPerSec == PCM_Formats[i].rate &&
	    wfx->wBitsPerSample == PCM_Formats[i].nBits)
	    return i;
    }
    return 0xFFFFFFFF;
}

static	LRESULT PCM_DriverDetails(PACMDRIVERDETAILSW add)
{
    add->fccType = ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC;
    add->fccComp = ACMDRIVERDETAILS_FCCCOMP_UNDEFINED;
    add->wMid = 0xFF;
    add->wPid = 0x00;
    add->vdwACM = 0x01000000;
    add->vdwDriver = 0x01000000;
    add->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CONVERTER;
    add->cFormatTags = 1;
    add->cFilterTags = 0;
    add->hicon = (HICON)0;
    lstrcpyAtoW(add->szShortName, "WINE-PCM");
    lstrcpyAtoW(add->szLongName, "Wine PCM converter");
    lstrcpyAtoW(add->szCopyright, "Brought to you by the Wine team...");
    lstrcpyAtoW(add->szLicensing, "Refer to LICENSE file");
    add->szFeatures[0] = 0;
    
    return MMSYSERR_NOERROR;
}

static	LRESULT	PCM_FormatTagDetails(PACMFORMATTAGDETAILSW aftd, DWORD dwQuery)
{
    switch (dwQuery) {
    case ACM_FORMATTAGDETAILSF_INDEX:
	if (aftd->dwFormatTagIndex != 0) return ACMERR_NOTPOSSIBLE;
	break;
    case ACM_FORMATTAGDETAILSF_FORMATTAG: 
	if (aftd->dwFormatTag != WAVE_FORMAT_PCM) return ACMERR_NOTPOSSIBLE;
	break;
    case ACM_FORMATTAGDETAILSF_LARGESTSIZE:
	if (aftd->dwFormatTag != WAVE_FORMAT_UNKNOWN && aftd->dwFormatTag != WAVE_FORMAT_UNKNOWN)
	    return ACMERR_NOTPOSSIBLE;
	break;
    default:
	WARN("Unsupported query %08lx\n", dwQuery);
	return MMSYSERR_NOTSUPPORTED;
    }
    
    aftd->dwFormatTagIndex = 0;
    aftd->dwFormatTag = WAVE_FORMAT_PCM;
    aftd->cbFormatSize = sizeof(PCMWAVEFORMAT);
    aftd->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CONVERTER;
    aftd->cStandardFormats = NUM_PCM_FORMATS;
    aftd->szFormatTag[0] = 0;
    
    return MMSYSERR_NOERROR;
}

static	LRESULT	PCM_FormatDetails(PACMFORMATDETAILSW afd, DWORD dwQuery)
{
    switch (dwQuery) {
    case ACM_FORMATDETAILSF_FORMAT:
	afd->dwFormatIndex = PCM_GetFormatIndex(afd->pwfx);
	if (afd->dwFormatIndex == 0xFFFFFFFF) return ACMERR_NOTPOSSIBLE;
	break;
    case ACM_FORMATDETAILSF_INDEX:
	assert(afd->dwFormatIndex < NUM_PCM_FORMATS);
	afd->pwfx->wFormatTag = WAVE_FORMAT_PCM;
	afd->pwfx->nChannels = PCM_Formats[afd->dwFormatIndex].nChannels;
	afd->pwfx->nSamplesPerSec = PCM_Formats[afd->dwFormatIndex].rate;
	afd->pwfx->wBitsPerSample = PCM_Formats[afd->dwFormatIndex].nBits;
	/* native MSACM uses a PCMWAVEFORMAT structure, so cbSize is not accessible
	   afd->pwfx->cbSize = 0; 
	*/
	afd->pwfx->nBlockAlign = (afd->pwfx->nChannels * afd->pwfx->wBitsPerSample) / 8;
	afd->pwfx->nAvgBytesPerSec = afd->pwfx->nSamplesPerSec * afd->pwfx->nBlockAlign;
	break;
    default:
	WARN("Unsupported query %08lx\n", dwQuery);
	return MMSYSERR_NOTSUPPORTED;	
    }
    
    afd->dwFormatTag = WAVE_FORMAT_PCM;
    afd->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CONVERTER;
    afd->szFormat[0] = 0; /* let MSACM format this for us... */
    
    return MMSYSERR_NOERROR;
}

static	LRESULT	PCM_FormatSuggest(PACMDRVFORMATSUGGEST adfs)
{
    FIXME("(%p);\n", adfs);
    return MMSYSERR_NOTSUPPORTED;
}

static	LRESULT	PCM_StreamOpen(PACMDRVSTREAMINSTANCE adsi)
{
    assert(!(adsi->fdwOpen & ACM_STREAMOPENF_ASYNC));
    
    if (PCM_GetFormatIndex(adsi->pwfxSrc) == 0xFFFFFFFF ||
	PCM_GetFormatIndex(adsi->pwfxDst) == 0xFFFFFFFF)
	return ACMERR_NOTPOSSIBLE;
    return MMSYSERR_NOERROR;
}

static	LRESULT	PCM_StreamClose(PACMDRVSTREAMINSTANCE adsi)
{
    return MMSYSERR_NOERROR;
}

static	inline DWORD	PCM_round(DWORD a, DWORD b, DWORD c)
{
    assert(a && b && c);
    /* to be sure, always return an entire number of c... */
    return (a * b + c - 1) / c;
}

static	LRESULT PCM_StreamSize(PACMDRVSTREAMINSTANCE adsi, PACMDRVSTREAMSIZE adss)
{
    switch (adss->fdwSize) {
    case ACM_STREAMSIZEF_DESTINATION:
	/* cbDstLength => cbSrcLength */
	adss->cbSrcLength = PCM_round(adss->cbDstLength, adsi->pwfxSrc->nAvgBytesPerSec, adsi->pwfxDst->nAvgBytesPerSec);
	break;
    case ACM_STREAMSIZEF_SOURCE:
	/* cbSrcLength => cbDstLength */
	adss->cbDstLength =  PCM_round(adss->cbSrcLength, adsi->pwfxDst->nAvgBytesPerSec, adsi->pwfxSrc->nAvgBytesPerSec);
	break;
    default:
	WARN("Unsupported query %08lx\n", adss->fdwSize);
	return MMSYSERR_NOTSUPPORTED;	
    }
    return MMSYSERR_NOERROR;
}

/*
  parameters :
  8 bit unsigned vs 16 bit signed (-32 / +32k ???)
  mono vs stereo
  sampling rate (8.0, 11.025, 22.05, 44.1 kHz)
*/

static	void cvtMS88K(const unsigned char* src, int ns, unsigned char* dst)
{
    while (ns--) {
	*dst++ = *src;
	*dst++ = *src++;
    }
}

static	void cvtMS816K(const unsigned char* src, int ns, short* dst)
{
    int	v;
    
    while (ns--) {
	v = ((short)(*src++) ^ 0x80) * 256;
	*dst++ = LOBYTE(v);
	*dst++ = HIBYTE(v);
	*dst++ = LOBYTE(v);
	*dst++ = HIBYTE(v);
    }
}

static	void cvtMS168K(const short* src, int ns, unsigned char* dst)
{
    unsigned char v;
    
    while (ns--) {
	v = HIBYTE(*src++) ^ 0x80;
	*dst++ = v;
	*dst++ = v;
    }
}

static	void cvtMS1616K(const short* src, int ns, short* dst)
{
    while (ns--) {
	*dst++ = *src;
	*dst++ = *src++;
    }
}

static	void cvtSM88K(const unsigned char* src, int ns, unsigned char* dst)
{
    while (ns--) {
	*dst++ = (src[0] + src[1]) / 2;
	src += 2;
    }
}

static	void cvtSM816K(const unsigned char* src, int ns, short* dst)
{
    int	v;
    
    while (ns--) {
	v = (((short)(src[0]) ^ 0x80) * 256 + ((short)(src[1]) ^ 0x80) * 256) / 2;
	src += 2;
	*dst++ = LOBYTE(v);
	*dst++ = HIBYTE(v);
    }
}

static	void cvtSM168K(const short* src, int ns, unsigned char* dst)
{
    unsigned char v;
    
    while (ns--) {
	v = ((HIBYTE(src[0]) ^ 0x80) + (HIBYTE(src[1]) ^ 0x80)) / 2;
	src += 2;
	*dst++ = v;
    }
}

static	void cvtSM1616K(const short* src, int ns, short* dst)
{
    while (ns--) {
	*dst++ = (src[0] + src[1]) / 2;
	src += 2;
    }
}

static	void cvtMM816K(const unsigned char* src, int ns, short* dst)
{
    int	v;
    
    while (ns--) {
	v = ((short)(*src++) ^ 0x80) * 256;
	*dst++ = LOBYTE(v);
	*dst++ = HIBYTE(v);
    }
}

static	void cvtSS816K(const unsigned char* src, int ns, short* dst)
{
    int	v;
    
    while (ns--) {
	v = ((short)(*src++) ^ 0x80) * 256;
	*dst++ = LOBYTE(v);
	*dst++ = HIBYTE(v);
	v = ((short)(*src++) ^ 0x80) * 256;
	*dst++ = LOBYTE(v);
	*dst++ = HIBYTE(v);
    }
}

static	void cvtMM168K(const short* src, int ns, unsigned char* dst)
{
    while (ns--) {
	*dst++ = HIBYTE(*src++) ^ 0x80;
    }
}

static	void cvtSS168K(const short* src, int ns, unsigned char* dst)
{
    while (ns--) {
	*dst++ = HIBYTE(*src++) ^ 0x80;
	*dst++ = HIBYTE(*src++) ^ 0x80;
    }
}

static	LRESULT	PCM_StreamConvert(PACMDRVSTREAMINSTANCE adsi, PACMDRVSTREAMHEADER adsh)
{
    /* do the job */
    if (adsi->pwfxSrc->nSamplesPerSec == adsi->pwfxDst->nSamplesPerSec) {
	/* easy case */
	if (adsi->pwfxSrc->wBitsPerSample == adsi->pwfxDst->wBitsPerSample &&
	    adsi->pwfxSrc->nChannels == adsi->pwfxDst->nChannels) {
	    memcpy(adsh->pbDst, adsh->pbSrc, adsh->cbSrcLength);
	} else if (adsi->pwfxSrc->wBitsPerSample == 8 &&
	    adsi->pwfxDst->wBitsPerSample == 8) {
	    if (adsi->pwfxSrc->nChannels == 1 &&
		adsi->pwfxDst->nChannels == 2)
		cvtMS88K(adsh->pbSrc, adsh->cbSrcLength, adsh->pbDst);
	    else if (adsi->pwfxSrc->nChannels == 2 &&
		     adsi->pwfxDst->nChannels == 1)
		cvtSM88K(adsh->pbSrc, adsh->cbSrcLength / 2, adsh->pbDst);
	} else if (adsi->pwfxSrc->wBitsPerSample == 8 &&
		   adsi->pwfxDst->wBitsPerSample == 16) {
	    if (adsi->pwfxSrc->nChannels == 1 &&
		adsi->pwfxDst->nChannels == 1)
		cvtMM816K(adsh->pbSrc, adsh->cbSrcLength, (short*)adsh->pbDst);
	    else if (adsi->pwfxSrc->nChannels == 1 &&
		     adsi->pwfxDst->nChannels == 2)
		cvtMS816K(adsh->pbSrc, adsh->cbSrcLength, (short*)adsh->pbDst);
	    else if (adsi->pwfxSrc->nChannels == 2 &&
		     adsi->pwfxDst->nChannels == 1)
		cvtSM816K(adsh->pbSrc, adsh->cbSrcLength / 2, (short*)adsh->pbDst);
	    else if (adsi->pwfxSrc->nChannels == 2 &&
		     adsi->pwfxDst->nChannels == 2)
		cvtSS816K(adsh->pbSrc, adsh->cbSrcLength / 2, (short*)adsh->pbDst);
	} else if (adsi->pwfxSrc->wBitsPerSample == 16 &&
		   adsi->pwfxDst->wBitsPerSample == 8) {
	    if (adsi->pwfxSrc->nChannels == 1 &&
		adsi->pwfxDst->nChannels == 1)
		cvtMM168K((short*)adsh->pbSrc, adsh->cbSrcLength / 2, adsh->pbDst);
	    else if (adsi->pwfxSrc->nChannels == 1 &&
		     adsi->pwfxDst->nChannels == 2)
		cvtMS168K((short*)adsh->pbSrc, adsh->cbSrcLength / 2, adsh->pbDst);
	    else if (adsi->pwfxSrc->nChannels == 2 &&
		     adsi->pwfxDst->nChannels == 1)
		cvtSM168K((short*)adsh->pbSrc, adsh->cbSrcLength / 4, adsh->pbDst);
	    else if (adsi->pwfxSrc->nChannels == 2 &&
		     adsi->pwfxDst->nChannels == 2)
		cvtSS168K((short*)adsh->pbSrc, adsh->cbSrcLength / 4, adsh->pbDst);
	} else if (adsi->pwfxSrc->wBitsPerSample == 16 &&
		   adsi->pwfxDst->wBitsPerSample == 16) {
	    if (adsi->pwfxSrc->nChannels == 1 &&
		adsi->pwfxDst->nChannels == 2)
		cvtMS1616K((short*)adsh->pbSrc, adsh->cbSrcLength / 2, (short*)adsh->pbDst);
	    else if (adsi->pwfxSrc->nChannels == 2 &&
		     adsi->pwfxDst->nChannels == 1)
		cvtSM1616K((short*)adsh->pbSrc, adsh->cbSrcLength / 4, (short*)adsh->pbDst);
	} else FIXME("NIY\n");
	/* FIXME: rounding shall be taken care off... */
	adsh->cbSrcLengthUsed = adsh->cbSrcLength;
	adsh->cbDstLengthUsed = (adsh->cbSrcLength * adsi->pwfxDst->nBlockAlign) / adsi->pwfxSrc->nBlockAlign;
	
    } else {
	FIXME("NIY\n");
	return MMSYSERR_NOTSUPPORTED;
    }
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			PCM_DriverProc			[exported]
 */
LRESULT CALLBACK	PCM_DriverProc(DWORD dwDevID, HDRVR hDriv, UINT wMsg, 
				       LPARAM dwParam1, LPARAM dwParam2)
{
    TRACE("(%08lx %08lx %u %08lx %08lx);\n", 
	  dwDevID, (DWORD)hDriv, wMsg, dwParam1, dwParam2);
    
    switch (wMsg) {
    case DRV_LOAD:		return 1;
    case DRV_FREE:		return 1;
    case DRV_OPEN:		return PCM_drvOpen((LPSTR)dwParam1);
    case DRV_CLOSE:		return PCM_drvClose(dwDevID);
    case DRV_ENABLE:		return 1;	
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBoxA(0, "MSACM PCM filter !", "Wine Driver", MB_OK); return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;
	
    case ACMDM_DRIVER_NOTIFY:
	/* no caching from other ACM drivers is done so far */
	return MMSYSERR_NOERROR;
	
    case ACMDM_DRIVER_DETAILS:
	return PCM_DriverDetails((PACMDRIVERDETAILSW)dwParam1);
	
    case ACMDM_FORMATTAG_DETAILS:
	return PCM_FormatTagDetails((PACMFORMATTAGDETAILSW)dwParam1, dwParam2);
	
    case ACMDM_FORMAT_DETAILS:
	return PCM_FormatDetails((PACMFORMATDETAILSW)dwParam1, dwParam2);
	
    case ACMDM_FORMAT_SUGGEST:
	return PCM_FormatSuggest((PACMDRVFORMATSUGGEST)dwParam1);
	
    case ACMDM_STREAM_OPEN:
	return PCM_StreamOpen((PACMDRVSTREAMINSTANCE)dwParam1);
	
    case ACMDM_STREAM_CLOSE:
	return PCM_StreamClose((PACMDRVSTREAMINSTANCE)dwParam1);
	
    case ACMDM_STREAM_SIZE:
	return PCM_StreamSize((PACMDRVSTREAMINSTANCE)dwParam1, (PACMDRVSTREAMSIZE)dwParam2);
	
    case ACMDM_STREAM_CONVERT:
	return PCM_StreamConvert((PACMDRVSTREAMINSTANCE)dwParam1, (PACMDRVSTREAMHEADER)dwParam2);
	
    case ACMDM_HARDWARE_WAVE_CAPS_INPUT:
    case ACMDM_HARDWARE_WAVE_CAPS_OUTPUT:
	/* this converter is not a hardware driver */
    case ACMDM_FILTERTAG_DETAILS:
    case ACMDM_FILTER_DETAILS:
	/* this converter is not a filter */
    case ACMDM_STREAM_RESET:
	/* only needed for asynchronous driver... we aren't, so just say it */
    case ACMDM_STREAM_PREPARE:
    case ACMDM_STREAM_UNPREPARE:
	/* nothing special to do here... so don't do anything */
	return MMSYSERR_NOTSUPPORTED;
	
    default:
	return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
    return 0;
}
