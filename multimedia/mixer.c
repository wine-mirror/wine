/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * Sample MIXER Wine Driver for Linux
 *
 * Copyright 	1997 Marcus Meissner
 * 		1999 Eric Pouech
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "windef.h"
#include "user.h"
#include "driver.h"
#include "mmddk.h"
#include "oss.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(mmaux)

#ifdef HAVE_OSS
#define MIXER_DEV "/dev/mixer"

#define	WINE_MIXER_MANUF_ID		0xAA
#define	WINE_MIXER_PRODUCT_ID		0x55
#define	WINE_MIXER_VERSION		0x0100
#define	WINE_MIXER_NAME			"WINE OSS Mixer"

#define WINE_CHN_MASK(_x)		(1L << (_x))
#define WINE_CHN_SUPPORTS(_c, _x)	((_c) & WINE_CHN_MASK(_x))
#define WINE_MIXER_MASK			(WINE_CHN_MASK(SOUND_MIXER_VOLUME) | \
                                         WINE_CHN_MASK(SOUND_MIXER_BASS)   | \
                                         WINE_CHN_MASK(SOUND_MIXER_TREBLE) | \
                                         WINE_CHN_MASK(SOUND_MIXER_SYNTH)  | \
                                         WINE_CHN_MASK(SOUND_MIXER_PCM)    | \
                                         WINE_CHN_MASK(SOUND_MIXER_LINE)   | \
                                         WINE_CHN_MASK(SOUND_MIXER_MIC)    | \
                                         WINE_CHN_MASK(SOUND_MIXER_CD))

/**************************************************************************
 * 				MIX_GetVal			[internal]
 */
static	BOOL	MIX_GetVal(int chn, int* val)
{
    int		mixer;
    BOOL	ret = FALSE;

    if ((mixer = open(MIXER_DEV, O_RDWR)) < 0) {
	/* FIXME: ENXIO => no mixer installed */
	WARN("mixer device not available !\n");
    } else {
	if (ioctl(mixer, MIXER_READ(chn), val) >= 0) {
	    TRACE("Reading %x on %d\n", *val, chn);
	    ret = TRUE;
	}
	close(mixer);
    }
    return ret;
}

/**************************************************************************
 * 				MIX_SetVal			[internal]
 */
static	BOOL	MIX_SetVal(int chn, int* val)
{
    int		mixer;
    BOOL	ret = FALSE;

    TRACE("Writing %x on %d\n", *val, chn);

    if ((mixer = open(MIXER_DEV, O_RDWR)) < 0) {
	/* FIXME: ENXIO => no mixer installed */
	WARN("mixer device not available !\n");
    } else {
	if (ioctl(mixer, MIXER_WRITE(chn), val) >= 0) {
	    ret = TRUE;
	}
	close(mixer);
    }
    return ret;
}

/**************************************************************************
 * 				MIX_GetDevCaps			[internal]
 */
static DWORD MIX_GetDevCaps(WORD wDevID, LPMIXERCAPSA lpCaps, DWORD dwSize)
{
    int 		mixer, mask;
    
    TRACE("(%04X, %p, %lu);\n", wDevID, lpCaps, dwSize);
    
    if (wDevID != 0) return MMSYSERR_BADDEVICEID;
    if (lpCaps == NULL) return MMSYSERR_INVALPARAM;
    
    if ((mixer = open(MIXER_DEV, O_RDWR)) < 0) {
	/* FIXME: ENXIO => no mixer installed */
	WARN("mixer device not available !\n");
	return MMSYSERR_NOTENABLED;
    }
    lpCaps->wMid = WINE_MIXER_MANUF_ID;
    lpCaps->wPid = WINE_MIXER_PRODUCT_ID;
    lpCaps->vDriverVersion = WINE_MIXER_VERSION;
    strcpy(lpCaps->szPname, WINE_MIXER_NAME);
    if (ioctl(mixer, SOUND_MIXER_READ_DEVMASK, &mask) == -1) {
	close(mixer);
	perror("ioctl mixer SOUND_MIXER_DEVMASK");
	return MMSYSERR_NOTENABLED;
    }	
    
    /* FIXME: can the Linux Mixer differ between multiple mixer targets ? */
    lpCaps->cDestinations = 1;
    lpCaps->fdwSupport = 0; /* No bits defined yet */
    
    close(mixer);
    return MMSYSERR_NOERROR;
}

static char *sdlabels[SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_LABELS;
static char *sdnames[SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_NAMES;

/**************************************************************************
 * 				MIX_GetLineInfoFromIndex	[internal]
 */
static	void	MIX_GetLineInfoFromIndex(LPMIXERLINEA lpMl, int devmask, DWORD idx)
{
    strcpy(lpMl->szShortName, sdlabels[idx]);
    strcpy(lpMl->szName, sdnames[idx]);
    lpMl->dwLineID = idx;
    lpMl->dwDestination = 0; /* index for speakers */
    lpMl->cConnections = 1;
    lpMl->cControls = 1;
    switch (idx) {
    case SOUND_MIXER_SYNTH:
	lpMl->dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER;
	lpMl->fdwLine	 |= MIXERLINE_LINEF_SOURCE;
	break;
    case SOUND_MIXER_CD:
	lpMl->dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC;
	lpMl->fdwLine	 |= MIXERLINE_LINEF_SOURCE;
	break;
    case SOUND_MIXER_LINE:
	lpMl->dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_LINE;
	lpMl->fdwLine	 |= MIXERLINE_LINEF_SOURCE;
	break;
    case SOUND_MIXER_MIC:
	lpMl->dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE;
	lpMl->fdwLine	 |= MIXERLINE_LINEF_SOURCE;
	break;
    case SOUND_MIXER_PCM:
	lpMl->dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT;
	lpMl->fdwLine	 |= MIXERLINE_LINEF_SOURCE;
	break;
    default:
	ERR("Index %ld not handled.\n", idx);
	break;
    }
}

/**************************************************************************
 * 				MIX_GetLineInfo			[internal]
 */
static DWORD MIX_GetLineInfo(WORD wDevID, LPMIXERLINEA lpMl, DWORD fdwInfo)
{
    int 		mixer, i, j;
    int			devmask, stereomask;
    BOOL		isDst = FALSE;
    DWORD		ret = MMSYSERR_NOERROR;
    
    TRACE("(%04X, %p, %lu);\n", wDevID, lpMl, fdwInfo);
    if (lpMl == NULL || lpMl->cbStruct != sizeof(*lpMl)) 
	return MMSYSERR_INVALPARAM;
    if ((mixer = open(MIXER_DEV, O_RDWR)) < 0)
	return MMSYSERR_NOTENABLED;
    
    if (ioctl(mixer, SOUND_MIXER_READ_DEVMASK, &devmask) == -1) {
	close(mixer);
	perror("ioctl mixer SOUND_MIXER_DEVMASK");
	return MMSYSERR_NOTENABLED;
    }
    devmask &= WINE_MIXER_MASK;
    if (ioctl(mixer, SOUND_MIXER_READ_STEREODEVS, &stereomask) == -1) {
	close(mixer);
	perror("ioctl mixer SOUND_MIXER_STEREODEVS");
	return MMSYSERR_NOTENABLED;
    }
    stereomask &= WINE_MIXER_MASK;

#if 0
    int			recsrc, recmask;

    if (ioctl(mixer, SOUND_MIXER_READ_RECSRC, &recsrc) == -1) {
	close(mixer);
	perror("ioctl mixer SOUND_MIXER_RECSRC");
	return MMSYSERR_NOTENABLED;
    }

    if (ioctl(mixer, SOUND_MIXER_READ_RECMASK, &recmask) == -1) {
	close(mixer);
	perror("ioctl mixer SOUND_MIXER_RECMASK");
	return MMSYSERR_NOTENABLED;
    }
#endif

    /* FIXME: set all the variables correctly... the lines below
     * are very wrong...
     */
    lpMl->fdwLine	= MIXERLINE_LINEF_ACTIVE;
    lpMl->cChannels	= 1;
    lpMl->dwUser	= 0;
    lpMl->cControls	= 1;
    
    switch (fdwInfo & MIXER_GETLINEINFOF_QUERYMASK) {
    case MIXER_GETLINEINFOF_DESTINATION:
	TRACE("DESTINATION (%08lx)\n", lpMl->dwDestination);
	/* FIXME: Linux doesn't seem to support multiple outputs? 
	 * So we have only one output type: Speaker.
	 */
	lpMl->dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
	lpMl->dwSource = 0xFFFFFFFF;
	lpMl->dwLineID = SOUND_MIXER_VOLUME;
	strncpy(lpMl->szShortName, sdlabels[SOUND_MIXER_VOLUME], MIXER_SHORT_NAME_CHARS);
	strncpy(lpMl->szName, sdnames[SOUND_MIXER_VOLUME], MIXER_LONG_NAME_CHARS);
	
	/* we have all connections found in the devmask */
	lpMl->cConnections = 0;
	for (j = 1; j < SOUND_MIXER_NRDEVICES; j++)
	    if (WINE_CHN_SUPPORTS(devmask, j))
		lpMl->cConnections++;
	if (stereomask & WINE_CHN_MASK(SOUND_MIXER_VOLUME))
	    lpMl->cChannels++;
	break;
    case MIXER_GETLINEINFOF_SOURCE:
	TRACE("SOURCE (%08lx)\n", lpMl->dwSource);
	i = lpMl->dwSource;
	for (j = 1; j < SOUND_MIXER_NRDEVICES; j++) {
	    if (WINE_CHN_SUPPORTS(devmask, j) && (i-- == 0)) 
		break;
	}
	if (j >= SOUND_MIXER_NRDEVICES)
	    return MIXERR_INVALLINE;
	if (WINE_CHN_SUPPORTS(stereomask, j))
	    lpMl->cChannels++;
	MIX_GetLineInfoFromIndex(lpMl, devmask, j);
	break;
    case MIXER_GETLINEINFOF_LINEID:
	TRACE("LINEID (%08lx)\n", lpMl->dwLineID);
	if (lpMl->dwLineID >= SOUND_MIXER_NRDEVICES)
	    return MIXERR_INVALLINE;
	if (WINE_CHN_SUPPORTS(stereomask, lpMl->dwLineID))
	    lpMl->cChannels++;
	MIX_GetLineInfoFromIndex(lpMl, devmask, lpMl->dwLineID);
	break;
    case MIXER_GETLINEINFOF_COMPONENTTYPE:
	TRACE("COMPONENT TYPE (%08lx)\n", lpMl->dwComponentType);
	
	switch (lpMl->dwComponentType) {
	case MIXERLINE_COMPONENTTYPE_DST_SPEAKERS:
	    i = SOUND_MIXER_VOLUME;
	    lpMl->dwDestination = 0;
	    lpMl->dwSource = 0xFFFFFFFF;
	    isDst = TRUE;
	    break;
	case MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER:
	    i = SOUND_MIXER_SYNTH;
	    lpMl->fdwLine |= MIXERLINE_LINEF_SOURCE;
	    break;
	case MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC:
	    i = SOUND_MIXER_CD;
	    lpMl->fdwLine |= MIXERLINE_LINEF_SOURCE;
	    break;
	case MIXERLINE_COMPONENTTYPE_SRC_LINE:
	    i = SOUND_MIXER_LINE;
	    lpMl->fdwLine |= MIXERLINE_LINEF_SOURCE;
	    break;
	case MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE:
	    i = SOUND_MIXER_MIC;
	    lpMl->fdwLine |= MIXERLINE_LINEF_SOURCE;
	    break;
	case MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT:
	    i = SOUND_MIXER_PCM;
	    lpMl->fdwLine |= MIXERLINE_LINEF_SOURCE;
	    break;
	default:
	    FIXME("Unhandled component type (%08lx)\n", lpMl->dwComponentType);
	    return MMSYSERR_INVALPARAM;
	}
	
	if (WINE_CHN_SUPPORTS(devmask, i)) {
	    strcpy(lpMl->szShortName, sdlabels[i]);
	    strcpy(lpMl->szName, sdnames[i]);
	    lpMl->dwLineID = i;
	}
	if (WINE_CHN_SUPPORTS(stereomask, i))
	    lpMl->cChannels++;
	lpMl->cConnections = 0;
	if (isDst) {
	    for (j = 1; j < SOUND_MIXER_NRDEVICES; j++) {
		if (WINE_CHN_SUPPORTS(devmask, j)) {
		    lpMl->cConnections++;
		}
	    }
	}
	break;
    case MIXER_GETLINEINFOF_TARGETTYPE:
	FIXME("_TARGETTYPE not implemented yet.\n");
	break;
    default:
	WARN("Unknown flag (%08lx)\n", fdwInfo & MIXER_GETLINEINFOF_QUERYMASK);
	break;
    }
    
    lpMl->Target.dwType = MIXERLINE_TARGETTYPE_AUX;
    lpMl->Target.dwDeviceID = 0xFFFFFFFF;
    lpMl->Target.wMid = WINE_MIXER_MANUF_ID;
    lpMl->Target.wPid = WINE_MIXER_PRODUCT_ID;
    lpMl->Target.vDriverVersion = WINE_MIXER_VERSION;
    strcpy(lpMl->Target.szPname, WINE_MIXER_NAME);
    
    close(mixer);
    return ret;
}

/**************************************************************************
 * 				MIX_GetLineInfo			[internal]
 */
static DWORD MIX_Open(WORD wDevID, LPMIXEROPENDESC lpMod, DWORD flags)
{
    TRACE("(%04X, %p, %lu);\n", wDevID, lpMod, flags);
    if (lpMod == NULL) return MMSYSERR_INVALPARAM;
    /* hmm. We don't keep the mixer device open. So just pretend it works */
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				MIX_GetLineControls		[internal]
 */
static	DWORD	MIX_GetLineControls(WORD wDevID, LPMIXERLINECONTROLSA lpMlc, DWORD flags)
{
    LPMIXERCONTROLA	mc;

    TRACE("(%04X, %p, %lu): stub!\n", wDevID, lpMlc, flags);
    
    if (lpMlc == NULL) return MMSYSERR_INVALPARAM;
    if (lpMlc->cbStruct < sizeof(*lpMlc) ||
	lpMlc->cbmxctrl < sizeof(MIXERCONTROLA))
	return MMSYSERR_INVALPARAM;

    switch (flags & MIXER_GETLINECONTROLSF_QUERYMASK) {
    case MIXER_GETLINECONTROLSF_ALL:
	TRACE("line=%08lx GLCF_ALL (%ld)\n", lpMlc->dwLineID, lpMlc->cControls);
	if (lpMlc->cControls != 1) 
	    return MMSYSERR_INVALPARAM;
	break;
    case MIXER_GETLINECONTROLSF_ONEBYID:
	TRACE("line=%08lx GLCF_ONEBYID (%lx)\n", lpMlc->dwLineID, lpMlc->u.dwControlID);
	if (lpMlc->u.dwControlID != 0) 
	    return MMSYSERR_INVALPARAM;
	break;
    case MIXER_GETLINECONTROLSF_ONEBYTYPE:
	TRACE("line=%08lx GLCF_ONEBYTYPE (%lx)\n", lpMlc->dwLineID, lpMlc->u.dwControlType);
	if ((lpMlc->u.dwControlType & MIXERCONTROL_CT_CLASS_MASK) != MIXERCONTROL_CT_CLASS_FADER)
	    return MMSYSERR_INVALPARAM;
	break;
    default:
	ERR("Unknown flag %08lx\n", flags & MIXER_GETLINECONTROLSF_QUERYMASK);
	return MMSYSERR_INVALPARAM;
    }
    TRACE("Returning volume control\n");
    /* currently, OSS only provides 1 control per line
     * so one by id == one by type == all
     */
    mc = lpMlc->pamxctrl;
    mc->cbStruct = sizeof(MIXERCONTROLA);
    /* since we always have a single control per line, we'll use for controlID the lineID */
    mc->dwControlID = lpMlc->dwLineID;
    mc->dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
    mc->fdwControl = 0;
    mc->cMultipleItems = 0;
    strncpy(mc->szShortName, "Vol", MIXER_SHORT_NAME_CHARS);
    strncpy(mc->szName, "Volume", MIXER_LONG_NAME_CHARS);
    memset(&mc->Bounds, 0, sizeof(mc->Bounds));
    /* CONTROLTYPE_VOLUME uses the MIXER_CONTROLDETAILS_UNSIGNED struct, 
     * [0, 100] is the range supported by OSS
     * FIXME: sounds like MIXERCONTROL_CONTROLTYPE_VOLUME is always between 0 and 65536...
     * look at conversions done in (Get|Set)ControlDetails to stay in [0, 100] range
     */
    mc->Bounds.dw.dwMinimum = 0;
    mc->Bounds.dw.dwMaximum = 100;
    memset(&mc->Metrics, 0, sizeof(mc->Metrics));
    mc->Metrics.cSteps = 0;
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				MIX_GetControlDetails		[internal]
 */
static	DWORD	MIX_GetControlDetails(WORD wDevID, LPMIXERCONTROLDETAILS lpmcd, DWORD fdwDetails)
{
    DWORD	ret = MMSYSERR_NOTSUPPORTED;

    TRACE("(%04X, %p, %lu)\n", wDevID, lpmcd, fdwDetails);
    
    if (lpmcd == NULL) return MMSYSERR_INVALPARAM;

    switch (fdwDetails & MIXER_GETCONTROLDETAILSF_QUERYMASK) {
    case MIXER_GETCONTROLDETAILSF_VALUE:
	TRACE("GCD VALUE (%08lx)\n", lpmcd->dwControlID);
	{
	    LPMIXERCONTROLDETAILS_UNSIGNED	mcdu;
	    int					val;

	    /* ControlID == LineID == OSS mixer channel */
	    /* return value is 00RL (4 bytes)... */
	    if (!MIX_GetVal(lpmcd->dwControlID, &val))
		return MMSYSERR_INVALPARAM;
	    
	    switch (lpmcd->cChannels) {
	    case 1:
		/* mono... so R = L */
		mcdu = (LPMIXERCONTROLDETAILS_UNSIGNED)lpmcd->paDetails;
		mcdu->dwValue = (LOBYTE(LOWORD(val)) * 65536L) / 100;
		break;
	    case 2:
		/* stereo, left is paDetails[0] */
		mcdu = (LPMIXERCONTROLDETAILS_UNSIGNED)((char*)lpmcd->paDetails + 0 * lpmcd->cbDetails);
		mcdu->dwValue = (LOBYTE(LOWORD(val)) * 65536L) / 100;
		mcdu = (LPMIXERCONTROLDETAILS_UNSIGNED)((char*)lpmcd->paDetails + 1 * lpmcd->cbDetails);
		mcdu->dwValue = (HIBYTE(LOWORD(val)) * 65536L) / 100;
		break;
	    default:
		WARN("Unknown cChannels (%ld)\n", lpmcd->cChannels);
		return MMSYSERR_INVALPARAM;
	    }
	    TRACE("=> %08lx\n", mcdu->dwValue);
	}
	ret = MMSYSERR_NOERROR;
	break;
    case MIXER_GETCONTROLDETAILSF_LISTTEXT:
	FIXME("NIY\n");
	break;
    default:
	WARN("Unknown flag (%08lx)\n", fdwDetails & MIXER_GETCONTROLDETAILSF_QUERYMASK);
    }
    return ret;
}

/**************************************************************************
 * 				MIX_SetControlDetails		[internal]
 */
static	DWORD	MIX_SetControlDetails(WORD wDevID, LPMIXERCONTROLDETAILS lpmcd, DWORD fdwDetails)
{
    DWORD	ret = MMSYSERR_NOTSUPPORTED;
    
    TRACE("(%04X, %p, %lu)\n", wDevID, lpmcd, fdwDetails);
    
    if (lpmcd == NULL) return MMSYSERR_INVALPARAM;
    
    switch (fdwDetails & MIXER_GETCONTROLDETAILSF_QUERYMASK) {
    case MIXER_GETCONTROLDETAILSF_VALUE:
	TRACE("GCD VALUE (%08lx)\n", lpmcd->dwControlID);
	{
	    LPMIXERCONTROLDETAILS_UNSIGNED	mcdu;
	    int					val;

	    /* val should contain 00RL */
	    switch (lpmcd->cChannels) {
	    case 1:
		/* mono... so R = L */
		mcdu = (LPMIXERCONTROLDETAILS_UNSIGNED)lpmcd->paDetails;
		TRACE("Setting RL to %08ld\n", mcdu->dwValue);
		val = 0x101 * ((mcdu->dwValue * 100) >> 16);
		break;
	    case 2:
		/* stereo, left is paDetails[0] */
		mcdu = (LPMIXERCONTROLDETAILS_UNSIGNED)((char*)lpmcd->paDetails + 0 * lpmcd->cbDetails);
		TRACE("Setting L to %08ld\n", mcdu->dwValue);
		val = ((mcdu->dwValue * 100) >> 16);
		mcdu = (LPMIXERCONTROLDETAILS_UNSIGNED)((char*)lpmcd->paDetails + 1 * lpmcd->cbDetails);
		TRACE("Setting R to %08ld\n", mcdu->dwValue);
		val += ((mcdu->dwValue * 100) >> 16) << 8;
		break;
	    default:
		WARN("Unknown cChannels (%ld)\n", lpmcd->cChannels);
		return MMSYSERR_INVALPARAM;
	    }

	    /* ControlID == LineID == OSS mixer channel */
	    if (!MIX_SetVal(lpmcd->dwControlID, &val))
		return MMSYSERR_INVALPARAM;
	}
	ret = MMSYSERR_NOERROR;
	break;
    case MIXER_GETCONTROLDETAILSF_LISTTEXT:
	FIXME("NIY\n");
	break;
    default:
	WARN("Unknown GetControlDetails flag (%08lx)\n", fdwDetails & MIXER_GETCONTROLDETAILSF_QUERYMASK);
    }
    return MMSYSERR_NOTSUPPORTED;
}

#endif /* HAVE_OSS */

/**************************************************************************
 * 				mixMessage			[sample driver]
 */
DWORD WINAPI mixMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
			DWORD dwParam1, DWORD dwParam2)
{
    TRACE("(%04X, %04X, %08lX, %08lX, %08lX);\n", 
	  wDevID, wMsg, dwUser, dwParam1, dwParam2);
    
#ifdef HAVE_OSS
    switch(wMsg) {
    case MXDM_GETDEVCAPS:	
	return MIX_GetDevCaps(wDevID, (LPMIXERCAPSA)dwParam1, dwParam2);
    case MXDM_GETLINEINFO:
	return MIX_GetLineInfo(wDevID, (LPMIXERLINEA)dwParam1, dwParam2);
    case MXDM_GETNUMDEVS:
	TRACE("return 1;\n");
	return 1;
    case MXDM_OPEN:
	return MIX_Open(wDevID, (LPMIXEROPENDESC)dwParam1, dwParam2);
    case MXDM_CLOSE:
	return MMSYSERR_NOERROR;
    case MXDM_GETLINECONTROLS:
	return MIX_GetLineControls(wDevID, (LPMIXERLINECONTROLSA)dwParam1, dwParam2);
    case MXDM_GETCONTROLDETAILS:
	return MIX_GetControlDetails(wDevID, (LPMIXERCONTROLDETAILS)dwParam1, dwParam2);
    case MXDM_SETCONTROLDETAILS:
	return MIX_SetControlDetails(wDevID, (LPMIXERCONTROLDETAILS)dwParam1, dwParam2);
    default:
	WARN("unknown message %d!\n", wMsg);
    }
    return MMSYSERR_NOTSUPPORTED;
#else
    return MMSYSERR_NOTENABLED;
#endif
}


