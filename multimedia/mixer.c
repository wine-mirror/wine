/*
 * Sample MIXER Wine Driver for Linux
 *
 * Copyright 1997 Marcus Meissner
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "windows.h"
#include "user.h"
#include "driver.h"
#include "mmsystem.h"

#ifdef linux
#include <linux/soundcard.h>
#elif __FreeBSD__
#include <machine/soundcard.h>
#endif

#include "stddebug.h"
#include "debug.h"

#define MIXER_DEV "/dev/mixer"

#ifdef SOUND_VERSION
#define IOCTL(a,b,c)		ioctl(a,b,&c)
#else
#define IOCTL(a,b,c)		(c = ioctl(a,b,c) )
#endif


/**************************************************************************
 * 				MIX_GetDevCaps			[internal]
 */
static DWORD MIX_GetDevCaps(WORD wDevID, LPMIXERCAPS16 lpCaps, DWORD dwSize)
{
#ifdef linux
	int 		mixer,mask;
	struct	mixer_info	mi;

	dprintf_mmaux(stddeb,"MIX_GetDevCaps(%04X, %p, %lu);\n", wDevID, lpCaps, dwSize);
	if (lpCaps == NULL) return MMSYSERR_NOTENABLED;
	if ((mixer = open(MIXER_DEV, O_RDWR)) < 0) {
		dprintf_mmaux(stddeb,"MIX_GetDevCaps // mixer device not available !\n");
		return MMSYSERR_NOTENABLED;
	}
	if (ioctl(mixer, SOUND_MIXER_INFO, &mi) == -1) {
		close(mixer);
		perror("ioctl mixer SOUND_MIXER_INFO");
		return MMSYSERR_NOTENABLED;
	}
	fprintf(stderr,"SOUND_MIXER_INFO returns { \"%s\",\"%s\" }\n",mi.id,mi.name);
	lpCaps->wMid = 0xAA;
	lpCaps->wPid = 0x55;
	lpCaps->vDriverVersion = 0x0100;
	strcpy(lpCaps->szPname,mi.name);
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

#ifdef linux
static char *sdlabels[SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_LABELS;
static char *sdnames[SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_NAMES;
#endif

/**************************************************************************
 * 				MIX_GetLineInfo			[internal]
 */
static DWORD MIX_GetLineInfo(WORD wDevID, LPMIXERLINE16 lpml, DWORD fdwInfo)
{
#ifdef linux
	int 		mixer,i,j,devmask,recsrc,recmask;

	dprintf_mmaux(stddeb,"MIX_GetDevLineInfo(%04X, %p, %lu);\n", wDevID, lpml, fdwInfo);
	if (lpml == NULL) return MMSYSERR_NOTENABLED;
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
	lpml->cbStruct = sizeof(MIXERLINE16);
	/* FIXME: set all the variables correctly... the lines below
	 * are very wrong...
	 */
	lpml->fdwLine	= MIXERLINE_LINEF_ACTIVE;
	lpml->cChannels	= 2;

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
		for (i=j=0;j<31;j++) {
			if (devmask & (1<<j)) {
				if (lpml->dwSource == i)
					break;
				i++;
			}
		}
		strcpy(lpml->szShortName,sdlabels[i]);
		strcpy(lpml->szName,sdnames[i]);
		lpml->dwLineID = i;
		switch (i) {
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
			fprintf(stderr,"MIX_GetLineInfo:mixertype %d not handle.\n",i);
			break;
		}
		break;
	case MIXER_GETLINEINFOF_LINEID:
		fprintf(stderr,"MIX_GetLineInfo: _LINEID (%ld) not implemented yet.\n",lpml->dwLineID);
		break;
	case MIXER_GETLINEINFOF_COMPONENTTYPE:
		fprintf(stderr,"MIX_GetLineInfo: _COMPONENTTYPE not implemented yet.\n");
		break;
	case MIXER_GETLINEINFOF_TARGETTYPE:
		fprintf(stderr,"MIX_GetLineInfo: _TARGETTYPE not implemented yet.\n");
		break;
	}
	lpml->Target.dwType = MIXERLINE_TARGETTYPE_AUX;
	close(mixer);
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}

/**************************************************************************
 * 				MIX_GetLineInfo			[internal]
 */
static DWORD MIX_Open(WORD wDevID, LPMIXEROPENDESC lpmod, DWORD flags)
{
#ifdef linux

	dprintf_mmaux(stddeb,"MIX_Open(%04X, %p, %lu);\n",wDevID,lpmod,flags);
	if (lpmod == NULL) return MMSYSERR_NOTENABLED;
	/* hmm. We don't keep the mixer device open. So just pretend it works */
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}

/**************************************************************************
 * 				mixMessage			[sample driver]
 */
DWORD mixMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
					DWORD dwParam1, DWORD dwParam2)
{
	dprintf_mmaux(stddeb,"mixMessage(%04X, %04X, %08lX, %08lX, %08lX);\n", 
			wDevID, wMsg, dwUser, dwParam1, dwParam2);
	switch(wMsg) {
	case MXDM_GETDEVCAPS:
		return MIX_GetDevCaps(wDevID,(LPMIXERCAPS16)dwParam1,dwParam2);
	case MXDM_GETLINEINFO:
		return MIX_GetLineInfo(wDevID,(LPMIXERLINE16)dwParam1,dwParam2);
	case MXDM_GETNUMDEVS:
		dprintf_mmsys(stddeb,"MIX_GetNumDevs() return 1;\n");
		return 1;
	case MXDM_OPEN:
		return MIX_Open(wDevID,(LPMIXEROPENDESC)dwParam1,dwParam2);
	default:
		dprintf_mmaux(stddeb,"mixMessage // unknown message %d!\n",wMsg);
	}
	return MMSYSERR_NOTSUPPORTED;
}
