/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * Sample MIXER Wine Driver for Linux
 *
 * Copyright 	1997 Marcus Meissner
 * 				1999 Eric Pouech
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "windef.h"
#include "user.h"
#include "driver.h"
#include "multimedia.h"
#include "debug.h"

DEFAULT_DEBUG_CHANNEL(mmaux)

#ifdef HAVE_OSS
#define MIXER_DEV "/dev/mixer"

#define	WINE_MIXER_MANUF_ID		0xAA
#define	WINE_MIXER_PRODUCT_ID	0x55
#define	WINE_MIXER_VERSION		0x0100
#define	WINE_MIXER_NAME			"WINE OSS Mixer"

/**************************************************************************
 * 				MIX_GetDevCaps												[internal]
 */
static DWORD MIX_GetDevCaps(WORD wDevID, LPMIXERCAPSA lpCaps, DWORD dwSize)
{
	int 		mixer, mask;

	TRACE(mmaux, "(%04X, %p, %lu);\n", wDevID, lpCaps, dwSize);

	if (wDevID != 0) return MMSYSERR_BADDEVICEID;
	if (lpCaps == NULL) return MMSYSERR_INVALPARAM;

	if ((mixer = open(MIXER_DEV, O_RDWR)) < 0) {
		WARN(mmaux, "mixer device not available !\n");
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
		lpMl->fdwLine	|= MIXERLINE_LINEF_SOURCE;
		break;
	case SOUND_MIXER_CD:
		lpMl->dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC;
		lpMl->fdwLine	|= MIXERLINE_LINEF_SOURCE;
		break;
	case SOUND_MIXER_LINE:
		lpMl->dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_LINE;
		lpMl->fdwLine	|= MIXERLINE_LINEF_SOURCE;
		break;
	case SOUND_MIXER_MIC:
		lpMl->dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE;
		lpMl->fdwLine	|= MIXERLINE_LINEF_SOURCE;
		break;
	case SOUND_MIXER_PCM:
		lpMl->dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT;
		lpMl->fdwLine	|= MIXERLINE_LINEF_SOURCE;
		break;
	default:
		ERR(mmaux, "Index %ld not handled.\n", idx);
		break;
	}
}

/**************************************************************************
 * 				MIX_GetLineInfo											[internal]
 */
static DWORD MIX_GetLineInfo(WORD wDevID, LPMIXERLINEA lpMl, DWORD fdwInfo)
{
	int 		mixer, i, j, devmask, recsrc, recmask;
	BOOL		isDst = FALSE;
	DWORD		ret = MMSYSERR_NOERROR;

	TRACE(mmaux, "(%04X, %p, %lu);\n", wDevID, lpMl, fdwInfo);
	if (lpMl == NULL || lpMl->cbStruct != sizeof(*lpMl)) 
		return MMSYSERR_INVALPARAM;
	if ((mixer = open(MIXER_DEV, O_RDWR)) < 0)
		return MMSYSERR_NOTENABLED;

	if (ioctl(mixer, SOUND_MIXER_READ_DEVMASK, &devmask) == -1) {
		close(mixer);
		perror("ioctl mixer SOUND_MIXER_DEVMASK");
		return MMSYSERR_NOTENABLED;
	}
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

	/* FIXME: set all the variables correctly... the lines below
	 * are very wrong...
	 */
	lpMl->fdwLine	    = MIXERLINE_LINEF_ACTIVE;
	lpMl->cChannels    = 2;
	lpMl->dwUser       = 0;
	lpMl->cControls    = 1;

	switch (fdwInfo & MIXER_GETLINEINFOF_QUERYMASK) {
	case MIXER_GETLINEINFOF_DESTINATION:
		/* FIXME: Linux doesn't seem to support multiple outputs? 
		 * So we have only one output type: Speaker.
		 */
		lpMl->dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
		lpMl->dwDestination = 0;
		lpMl->dwSource = 0xFFFFFFFF;
		lpMl->dwLineID = SOUND_MIXER_SPEAKER;
		strcpy(lpMl->szShortName, sdlabels[SOUND_MIXER_SPEAKER]);
		strcpy(lpMl->szName, sdnames[SOUND_MIXER_SPEAKER]);

		/* we have all connections found in the devmask */
		lpMl->cConnections = 0;
		for (j = 0; j < 31; j++)
			if (devmask & (1 << j))
				lpMl->cConnections++;
		break;
	case MIXER_GETLINEINFOF_SOURCE:
		for (i = j = 0; j < 31; j++) {
			if (devmask & (1 << j)) {
				if (lpMl->dwSource == i)
					break;
				i++;
			}
		}
		MIX_GetLineInfoFromIndex(lpMl, devmask, i);
		break;
	case MIXER_GETLINEINFOF_LINEID:
		MIX_GetLineInfoFromIndex(lpMl, devmask, lpMl->dwLineID);
		break;
	case MIXER_GETLINEINFOF_COMPONENTTYPE:
		TRACE(mmaux, "Getting component type (%08lx)\n", lpMl->dwComponentType);

		switch (lpMl->dwComponentType) {
		case MIXERLINE_COMPONENTTYPE_DST_SPEAKERS:
			i = SOUND_MIXER_SPEAKER;
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
			FIXME(mmaux, "Unhandled component type (%08lx)\n", lpMl->dwComponentType);
			return MMSYSERR_INVALPARAM;
		}

		if (devmask & (1 << i)) {
			strcpy(lpMl->szShortName, sdlabels[i]);
			strcpy(lpMl->szName, sdnames[i]);
			lpMl->dwLineID = i;
		}
		lpMl->cConnections = 0;
		if (isDst) {
			for (j = 0; j < 31; j++)
				if (devmask & (1 << j))
					lpMl->cConnections++;
			/*			lpMl->dwLineID = 32;*/
		}
		break;
	case MIXER_GETLINEINFOF_TARGETTYPE:
		FIXME(mmaux, "_TARGETTYPE not implemented yet.\n");
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
	TRACE(mmaux, "(%04X, %p, %lu);\n", wDevID, lpMod, flags);
	if (lpMod == NULL) return MMSYSERR_INVALPARAM;
	/* hmm. We don't keep the mixer device open. So just pretend it works */
	return MMSYSERR_NOERROR;
}

static	DWORD	MIX_GetLineControls(WORD wDevID, LPMIXERLINECONTROLSA lpMlc, DWORD flags)
{
	TRACE(mmaux, "(%04X, %p, %lu);\n", wDevID, lpMlc, flags);

	if (lpMlc == NULL) return MMSYSERR_INVALPARAM;

	return MMSYSERR_NOERROR;
}
#endif

/**************************************************************************
 * 				mixMessage			[sample driver]
 */
DWORD WINAPI mixMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
								DWORD dwParam1, DWORD dwParam2)
{
	TRACE(mmaux, "(%04X, %04X, %08lX, %08lX, %08lX);\n", 
			wDevID, wMsg, dwUser, dwParam1, dwParam2);

#ifdef HAVE_OSS
	switch(wMsg) {
	case MXDM_GETDEVCAPS:	
		return MIX_GetDevCaps(wDevID, (LPMIXERCAPSA)dwParam1, dwParam2);
	case MXDM_GETLINEINFO:
		return MIX_GetLineInfo(wDevID, (LPMIXERLINEA)dwParam1, dwParam2);
	case MXDM_GETNUMDEVS:
		TRACE(mmaux, "return 1;\n");
		return 1;
	case MXDM_OPEN:
		return MIX_Open(wDevID, (LPMIXEROPENDESC)dwParam1, dwParam2);
	case MXDM_CLOSE:
		return MMSYSERR_NOERROR;
	case MXDM_GETLINECONTROLS:
		return MIX_GetLineControls(wDevID, (LPMIXERLINECONTROLSA)dwParam1, dwParam2);
	default:
		WARN(mmaux, "unknown message %d!\n", wMsg);
	}
	return MMSYSERR_NOTSUPPORTED;
#endif
	return MMSYSERR_NOTENABLED;
}


