/*
 * Sample MIXER Wine Driver for Linux
 *
 * Copyright 1997 Marcus Meissner
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

#define MIXER_DEV "/dev/mixer"

/**************************************************************************
 * 				MIX_GetDevCaps			[internal]
 */
static DWORD MIX_GetDevCaps(WORD wDevID, LPMIXERCAPS16 lpCaps, DWORD dwSize)
{
#ifdef HAVE_OSS
	int 		mixer,mask;

	TRACE(mmaux, "(%04X, %p, %lu);\n", wDevID, lpCaps, dwSize);
	if (lpCaps == NULL) return MMSYSERR_INVALPARAM;
	if ((mixer = open(MIXER_DEV, O_RDWR)) < 0) {
		WARN(mmaux, "mixer device not available !\n");
		return MMSYSERR_NOTENABLED;
	}
	lpCaps->wMid = 0xAA;
	lpCaps->wPid = 0x55;
	lpCaps->vDriverVersion = 0x0100;
	strcpy(lpCaps->szPname, "WINE Generic Mixer");
	if (ioctl(mixer, SOUND_MIXER_READ_DEVMASK, &mask) == -1) {
		close(mixer);
		perror("ioctl mixer SOUND_MIXER_DEVMASK");
		return MMSYSERR_NOTENABLED;
	}
	/* FIXME: can the Linux Mixer differ between multiple mixertargets? */
	lpCaps->cDestinations = 1;
	lpCaps->fdwSupport = 0; /* No bits defined yet */

	close(mixer);
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}

#ifdef HAVE_OSS
static char *sdlabels[SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_LABELS;
static char *sdnames[SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_NAMES;

static	void	MIX_GetLineInfoFromIndex(LPMIXERLINE16 lpml, int devmask, DWORD idx)
{
	strcpy(lpml->szShortName, sdlabels[idx]);
	strcpy(lpml->szName, sdnames[idx]);
	lpml->dwLineID = idx;
	lpml->dwDestination = 0; /* index for speakers */
	lpml->cConnections = 1;
	lpml->cControls = 1;
	switch (idx) {
	case SOUND_MIXER_SYNTH:
		lpml->dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER;
		lpml->fdwLine	|= MIXERLINE_LINEF_SOURCE;
		break;
	case SOUND_MIXER_CD:
		lpml->dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC;
		lpml->fdwLine	|= MIXERLINE_LINEF_SOURCE;
		break;
	case SOUND_MIXER_LINE:
		lpml->dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_LINE;
		lpml->fdwLine	|= MIXERLINE_LINEF_SOURCE;
		break;
	case SOUND_MIXER_MIC:
		lpml->dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE;
		lpml->fdwLine	|= MIXERLINE_LINEF_SOURCE;
		break;
	case SOUND_MIXER_PCM:
		lpml->dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT;
		lpml->fdwLine	|= MIXERLINE_LINEF_SOURCE;
		break;
	default:
		ERR(mmaux, "Index %ld not handled.\n", idx);
		break;
	}
}
#endif

/**************************************************************************
 * 				MIX_GetLineInfo			[internal]
 */
static DWORD MIX_GetLineInfo(WORD wDevID, LPMIXERLINE16 lpml, DWORD fdwInfo)
{
#ifdef HAVE_OSS
	int 		mixer, i, j, devmask, recsrc, recmask;
	DWORD		ret = MMSYSERR_NOERROR;

	TRACE(mmaux, "(%04X, %p, %lu);\n", wDevID, lpml, fdwInfo);
	if (lpml == NULL || lpml->cbStruct != sizeof(*lpml)) 
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
	lpml->fdwLine	 = MIXERLINE_LINEF_ACTIVE;
	lpml->cChannels = 2;

	switch (fdwInfo & MIXER_GETLINEINFOF_QUERYMASK) {
	case MIXER_GETLINEINFOF_DESTINATION:
		/* FIXME: Linux doesn't seem to support multiple outputs? 
		 * So we have only one outputtype, Speaker.
		 */
		lpml->dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
		/* we have all connections found in the devmask */
		lpml->cConnections = 0;
		for (j=0;j<31;j++)
			if (devmask & (1<<j))
				lpml->cConnections++;
		break;
	case MIXER_GETLINEINFOF_SOURCE:
		for (i = j = 0; j < 31; j++) {
			if (devmask & (1 << j)) {
				if (lpml->dwSource == i)
					break;
				i++;
			}
		}
		MIX_GetLineInfoFromIndex(lpml, devmask, i);
		break;
	case MIXER_GETLINEINFOF_LINEID:
		MIX_GetLineInfoFromIndex(lpml, devmask, lpml->dwLineID);
		break;
	case MIXER_GETLINEINFOF_COMPONENTTYPE:
		TRACE(mmaux, "Getting component type (%08lx)\n", lpml->dwComponentType);
		switch (lpml->dwComponentType) {
		case MIXERLINE_COMPONENTTYPE_DST_SPEAKERS:
			i = -1;
			break;
		case MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER:
			i = SOUND_MIXER_SYNTH;
			break;
		case MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC:
			i = SOUND_MIXER_CD;
			break;
		case MIXERLINE_COMPONENTTYPE_SRC_LINE:
			i = SOUND_MIXER_LINE;
			break;
		case MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE:
			i = SOUND_MIXER_MIC;
			break;
		case MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT:
			i = SOUND_MIXER_PCM;
			break;
		default:
			FIXME(mmaux, "Unhandled component type (%08lx)\n", lpml->dwComponentType);
			return MMSYSERR_INVALPARAM;
		}
		if (i != -1 && (devmask & (1 << i))) {
			strcpy(lpml->szShortName, sdlabels[i]);
			strcpy(lpml->szName, sdnames[i]);
			lpml->dwLineID = i;
			lpml->fdwLine = MIXERLINE_LINEF_SOURCE;
		} else {
			lpml->cConnections = 0;
			for (j=0;j<31;j++)
				if (devmask & (1<<j))
					lpml->cConnections++;
			lpml->dwLineID = 32;
		}
		break;
	case MIXER_GETLINEINFOF_TARGETTYPE:
		FIXME(mmaux, "_TARGETTYPE not implemented yet.\n");
		break;
	}
	lpml->Target.dwType = MIXERLINE_TARGETTYPE_AUX;
	close(mixer);
	return ret;
#else
	return MMSYSERR_NOTENABLED;
#endif
}

/**************************************************************************
 * 				MIX_GetLineInfo			[internal]
 */
static DWORD MIX_Open(WORD wDevID, LPMIXEROPENDESC lpmod, DWORD flags)
{
#ifdef HAVE_OSS

	TRACE(mmaux, "(%04X, %p, %lu);\n", wDevID,lpmod,flags);
	if (lpmod == NULL) return MMSYSERR_INVALPARAM;
	/* hmm. We don't keep the mixer device open. So just pretend it works */
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}

/**************************************************************************
 * 				mixMessage			[sample driver]
 */
DWORD WINAPI mixMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
								DWORD dwParam1, DWORD dwParam2)
{
	TRACE(mmaux, "(%04X, %04X, %08lX, %08lX, %08lX);\n", 
			wDevID, wMsg, dwUser, dwParam1, dwParam2);

	switch(wMsg) {
	case MXDM_GETDEVCAPS:
		return MIX_GetDevCaps(wDevID, (LPMIXERCAPS16)dwParam1, dwParam2);
	case MXDM_GETLINEINFO:
		return MIX_GetLineInfo(wDevID, (LPMIXERLINE16)dwParam1, dwParam2);
	case MXDM_GETNUMDEVS:
		TRACE(mmaux, "return 1;\n");
		return 1;
	case MXDM_OPEN:
		return MIX_Open(wDevID, (LPMIXEROPENDESC)dwParam1, dwParam2);
	case MXDM_CLOSE:
		return MMSYSERR_NOERROR;
	default:
		WARN(mmaux, "unknown message %d!\n", wMsg);
	}
	return MMSYSERR_NOTSUPPORTED;
}

