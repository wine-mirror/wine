/*
 * GSM 06.10 codec handling
 * Copyright (C) 2009 Maarten Lankhorst
 *
 * Based on msg711.acm
 * Copyright (C) 2002 Eric Pouech
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include <wine/port.h>

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#ifdef HAVE_GSM_H
#include <gsm.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "mmsystem.h"
#include "mmreg.h"
#include "msacm.h"
#include "msacmdrv.h"
#include "wine/library.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gsm);

#ifdef SONAME_LIBGSM

static void *libgsm_handle;
#define FUNCPTR(f) static typeof(f) * p##f
FUNCPTR(gsm_create);
FUNCPTR(gsm_destroy);
FUNCPTR(gsm_option);
FUNCPTR(gsm_encode);
FUNCPTR(gsm_decode);

#define LOAD_FUNCPTR(f) \
    if((p##f = wine_dlsym(libgsm_handle, #f, NULL, 0)) == NULL) { \
        wine_dlclose(libgsm_handle, NULL, 0); \
        libgsm_handle = NULL; \
        return 0; \
    }

/***********************************************************************
 *           GSM_drvLoad
 */
static LRESULT GSM_drvLoad(void)
{
    char error[128];

    libgsm_handle = wine_dlopen(SONAME_LIBGSM, RTLD_NOW, error, sizeof(error));
    if (libgsm_handle)
    {
        LOAD_FUNCPTR(gsm_create);
        LOAD_FUNCPTR(gsm_destroy);
        LOAD_FUNCPTR(gsm_option);
        LOAD_FUNCPTR(gsm_encode);
        LOAD_FUNCPTR(gsm_decode);
        return 1;
    }
    else
    {
        ERR("Couldn't load " SONAME_LIBGSM ": %s\n", error);
        return 0;
    }
}

/***********************************************************************
 *           GSM_drvFree
 */
static LRESULT GSM_drvFree(void)
{
    if (libgsm_handle)
        wine_dlclose(libgsm_handle, NULL, 0);
    return 1;
}

#else

static LRESULT GSM_drvLoad(void)
{
    return 1;
}

static LRESULT GSM_drvFree(void)
{
    return 1;
}

#endif

/***********************************************************************
 *           GSM_DriverDetails
 *
 */
static	LRESULT GSM_DriverDetails(PACMDRIVERDETAILSW add)
{
    add->fccType = ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC;
    add->fccComp = ACMDRIVERDETAILS_FCCCOMP_UNDEFINED;
    /* Details found from probing native msgsm32.acm */
    add->wMid = MM_MICROSOFT;
    add->wPid = 36;
    add->vdwACM = 0x3320000;
    add->vdwDriver = 0x4000000;
    add->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CODEC;
    add->cFormatTags = 2;
    add->cFilterTags = 0;
    add->hicon = NULL;
    MultiByteToWideChar( CP_ACP, 0, "Wine GSM 6.10", -1,
                         add->szShortName, sizeof(add->szShortName)/sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, "Wine GSM 6.10 libgsm codec", -1,
                         add->szLongName, sizeof(add->szLongName)/sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, "Brought to you by the Wine team...", -1,
                         add->szCopyright, sizeof(add->szCopyright)/sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, "Refer to LICENSE file", -1,
                         add->szLicensing, sizeof(add->szLicensing)/sizeof(WCHAR) );
    add->szFeatures[0] = 0;
    return MMSYSERR_NOERROR;
}

/* Validate a WAVEFORMATEX structure */
static DWORD GSM_FormatValidate(const WAVEFORMATEX *wfx)
{
    if (wfx->nChannels != 1)
        return 0;

    switch (wfx->wFormatTag)
    {
    case WAVE_FORMAT_PCM:
        if (wfx->wBitsPerSample != 16)
        {
            WARN("PCM wBitsPerSample %u\n", wfx->wBitsPerSample);
            return 0;
        }
        if (wfx->nBlockAlign != 2)
        {
            WARN("PCM nBlockAlign %u\n", wfx->nBlockAlign);
            return 0;
        }
        if (wfx->nAvgBytesPerSec != wfx->nBlockAlign * wfx->nSamplesPerSec)
        {
            WARN("PCM nAvgBytesPerSec %u/%u\n",
                 wfx->nAvgBytesPerSec,
                 wfx->nBlockAlign * wfx->nSamplesPerSec);
            return 0;
        }
        return 1;
    case WAVE_FORMAT_GSM610:
        if (wfx->cbSize < sizeof(WORD))
        {
            WARN("GSM cbSize %u\n", wfx->cbSize);
            return 0;
        }
        if (wfx->wBitsPerSample != 0)
        {
            WARN("GSM wBitsPerSample %u\n", wfx->wBitsPerSample);
            return 0;
        }
        if (wfx->nBlockAlign != 65)
        {
            WARN("GSM nBlockAlign %u\n", wfx->nBlockAlign);
            return 0;
        }
        if (((GSM610WAVEFORMAT*)wfx)->wSamplesPerBlock != 320)
        {
            WARN("GSM wSamplesPerBlock %u\n",
                 ((GSM610WAVEFORMAT*)wfx)->wSamplesPerBlock);
            return 0;
        }
        if (wfx->nAvgBytesPerSec != wfx->nSamplesPerSec * 65 / 320)
        {
            WARN("GSM nAvgBytesPerSec %d / %d\n",
                 wfx->nAvgBytesPerSec, wfx->nSamplesPerSec * 65 / 320);
            return 0;
        }
        return 1;
    default:
        return 0;
    }
    return 0;
}

static const DWORD gsm_rates[] = { 8000, 11025, 22050, 44100, 48000, 96000 };
#define NUM_RATES (sizeof(gsm_rates)/sizeof(*gsm_rates))

/***********************************************************************
 *           GSM_FormatTagDetails
 *
 */
static	LRESULT	GSM_FormatTagDetails(PACMFORMATTAGDETAILSW aftd, DWORD dwQuery)
{
    static const WCHAR szPcm[]={'P','C','M',0};
    static const WCHAR szGsm[]={'G','S','M',' ','6','.','1','0',0};

    switch (dwQuery)
    {
    case ACM_FORMATTAGDETAILSF_INDEX:
	if (aftd->dwFormatTagIndex > 1) return ACMERR_NOTPOSSIBLE;
	break;
    case ACM_FORMATTAGDETAILSF_LARGESTSIZE:
	if (aftd->dwFormatTag == WAVE_FORMAT_UNKNOWN)
        {
            aftd->dwFormatTagIndex = 1;
	    break;
	}
	/* fall thru */
    case ACM_FORMATTAGDETAILSF_FORMATTAG:
	switch (aftd->dwFormatTag)
        {
	case WAVE_FORMAT_PCM: aftd->dwFormatTagIndex = 0; break;
	case WAVE_FORMAT_GSM610: aftd->dwFormatTagIndex = 1; break;
	default: return ACMERR_NOTPOSSIBLE;
	}
	break;
    default:
	WARN("Unsupported query %08x\n", dwQuery);
	return MMSYSERR_NOTSUPPORTED;
    }

    aftd->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CODEC;
    switch (aftd->dwFormatTagIndex)
    {
    case 0:
	aftd->dwFormatTag = WAVE_FORMAT_PCM;
	aftd->cbFormatSize = sizeof(PCMWAVEFORMAT);
	aftd->cStandardFormats = NUM_RATES;
        lstrcpyW(aftd->szFormatTag, szPcm);
        break;
    case 1:
	aftd->dwFormatTag = WAVE_FORMAT_GSM610;
	aftd->cbFormatSize = sizeof(GSM610WAVEFORMAT);
	aftd->cStandardFormats = NUM_RATES;
        lstrcpyW(aftd->szFormatTag, szGsm);
	break;
    }
    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           GSM_FormatDetails
 *
 */
static	LRESULT	GSM_FormatDetails(PACMFORMATDETAILSW afd, DWORD dwQuery)
{
    switch (dwQuery)
    {
    case ACM_FORMATDETAILSF_FORMAT:
	if (!GSM_FormatValidate(afd->pwfx)) return ACMERR_NOTPOSSIBLE;
	break;
    case ACM_FORMATDETAILSF_INDEX:
	afd->pwfx->wFormatTag = afd->dwFormatTag;
	switch (afd->dwFormatTag)
        {
	case WAVE_FORMAT_PCM:
	    if (afd->dwFormatIndex >= NUM_RATES) return ACMERR_NOTPOSSIBLE;
	    afd->pwfx->nChannels = 1;
	    afd->pwfx->nSamplesPerSec = gsm_rates[afd->dwFormatIndex];
	    afd->pwfx->wBitsPerSample = 16;
	    afd->pwfx->nBlockAlign = 2;
	    afd->pwfx->nAvgBytesPerSec = afd->pwfx->nSamplesPerSec * afd->pwfx->nBlockAlign;
	    break;
	case WAVE_FORMAT_GSM610:
            if (afd->dwFormatIndex >= NUM_RATES) return ACMERR_NOTPOSSIBLE;
	    afd->pwfx->nChannels = 1;
	    afd->pwfx->nSamplesPerSec = gsm_rates[afd->dwFormatIndex];
	    afd->pwfx->wBitsPerSample = 0;
	    afd->pwfx->nBlockAlign = 65;
            afd->pwfx->nAvgBytesPerSec = afd->pwfx->nSamplesPerSec * 65 / 320;
            afd->pwfx->cbSize = sizeof(WORD);
            ((GSM610WAVEFORMAT*)afd->pwfx)->wSamplesPerBlock = 320;
	    break;
	default:
            WARN("Unsupported tag %08x\n", afd->dwFormatTag);
	    return MMSYSERR_INVALPARAM;
	}
	break;
    default:
	WARN("Unsupported query %08x\n", dwQuery);
	return MMSYSERR_NOTSUPPORTED;
    }
    afd->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CODEC;
    afd->szFormat[0] = 0; /* let MSACM format this for us... */

    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           GSM_FormatSuggest
 *
 */
static	LRESULT	GSM_FormatSuggest(PACMDRVFORMATSUGGEST adfs)
{
    /* some tests ... */
    if (adfs->cbwfxSrc < sizeof(PCMWAVEFORMAT) ||
	adfs->cbwfxDst < sizeof(PCMWAVEFORMAT) ||
	!GSM_FormatValidate(adfs->pwfxSrc)) return ACMERR_NOTPOSSIBLE;
    /* FIXME: should do those tests against the real size (according to format tag */

    /* If no suggestion for destination, then copy source value */
    if (!(adfs->fdwSuggest & ACM_FORMATSUGGESTF_NCHANNELS))
	adfs->pwfxDst->nChannels = adfs->pwfxSrc->nChannels;
    if (!(adfs->fdwSuggest & ACM_FORMATSUGGESTF_NSAMPLESPERSEC))
        adfs->pwfxDst->nSamplesPerSec = adfs->pwfxSrc->nSamplesPerSec;

    if (!(adfs->fdwSuggest & ACM_FORMATSUGGESTF_WBITSPERSAMPLE))
    {
	if (adfs->pwfxSrc->wFormatTag == WAVE_FORMAT_PCM)
            adfs->pwfxDst->wBitsPerSample = 0;
        else
            adfs->pwfxDst->wBitsPerSample = 16;
    }
    if (!(adfs->fdwSuggest & ACM_FORMATSUGGESTF_WFORMATTAG))
    {
	switch (adfs->pwfxSrc->wFormatTag)
        {
        case WAVE_FORMAT_PCM: adfs->pwfxDst->wFormatTag = WAVE_FORMAT_GSM610; break;
        case WAVE_FORMAT_GSM610: adfs->pwfxDst->wFormatTag = WAVE_FORMAT_PCM; break;
        }
    }

    /* recompute other values */
    switch (adfs->pwfxDst->wFormatTag)
    {
    case WAVE_FORMAT_PCM:
        adfs->pwfxDst->nBlockAlign = 2;
        adfs->pwfxDst->nAvgBytesPerSec = adfs->pwfxDst->nSamplesPerSec * 2;
        break;
    case WAVE_FORMAT_GSM610:
        if (adfs->pwfxDst->cbSize < sizeof(WORD))
            return ACMERR_NOTPOSSIBLE;
        adfs->pwfxDst->nBlockAlign = 65;
        adfs->pwfxDst->nAvgBytesPerSec = adfs->pwfxDst->nSamplesPerSec * 65 / 320;
        ((GSM610WAVEFORMAT*)adfs->pwfxDst)->wSamplesPerBlock = 320;
        break;
    default:
        return ACMERR_NOTPOSSIBLE;
    }

    /* check if result is ok */
    if (!GSM_FormatValidate(adfs->pwfxDst)) return ACMERR_NOTPOSSIBLE;
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			GSM_DriverProc			[exported]
 */
LRESULT CALLBACK GSM_DriverProc(DWORD_PTR dwDevID, HDRVR hDriv, UINT wMsg,
					 LPARAM dwParam1, LPARAM dwParam2)
{
    TRACE("(%08lx %p %04x %08lx %08lx);\n",
          dwDevID, hDriv, wMsg, dwParam1, dwParam2);

    switch (wMsg)
    {
    case DRV_LOAD:		return GSM_drvLoad();
    case DRV_FREE:		return GSM_drvFree();
    case DRV_OPEN:		return 1;
    case DRV_CLOSE:		return 1;
    case DRV_ENABLE:		return 1;
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBoxA(0, "GSM 06.10 codec", "Wine Driver", MB_OK); return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;

    case ACMDM_DRIVER_NOTIFY:
	/* no caching from other ACM drivers is done so far */
	return MMSYSERR_NOERROR;

    case ACMDM_DRIVER_DETAILS:
	return GSM_DriverDetails((PACMDRIVERDETAILSW)dwParam1);

    case ACMDM_FORMATTAG_DETAILS:
	return GSM_FormatTagDetails((PACMFORMATTAGDETAILSW)dwParam1, dwParam2);

    case ACMDM_FORMAT_DETAILS:
	return GSM_FormatDetails((PACMFORMATDETAILSW)dwParam1, dwParam2);

    case ACMDM_FORMAT_SUGGEST:
	return GSM_FormatSuggest((PACMDRVFORMATSUGGEST)dwParam1);

    case ACMDM_STREAM_OPEN:
    case ACMDM_STREAM_CLOSE:
    case ACMDM_STREAM_SIZE:
    case ACMDM_STREAM_CONVERT:
        return MMSYSERR_NOTSUPPORTED;

    case ACMDM_HARDWARE_WAVE_CAPS_INPUT:
    case ACMDM_HARDWARE_WAVE_CAPS_OUTPUT:
	/* this converter is not a hardware driver */
    case ACMDM_FILTERTAG_DETAILS:
    case ACMDM_FILTER_DETAILS:
	/* this converter is not a filter */
    case ACMDM_STREAM_RESET:
	/* only needed for asynchronous driver... we aren't, so just say it */
	return MMSYSERR_NOTSUPPORTED;
    case ACMDM_STREAM_PREPARE:
    case ACMDM_STREAM_UNPREPARE:
	/* nothing special to do here... so don't do anything */
	return MMSYSERR_NOERROR;

    default:
	return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
}
