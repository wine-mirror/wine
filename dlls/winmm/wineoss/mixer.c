/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * Sample MIXER Wine Driver for Linux
 *
 * Copyright 	1997 Marcus Meissner
 * 		1999,2001 Eric Pouech
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

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "mmddk.h"
#include "oss.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mmaux);

#ifdef HAVE_OSS

#define	WINE_MIXER_MANUF_ID		0xAA
#define	WINE_MIXER_PRODUCT_ID		0x55
#define	WINE_MIXER_VERSION		0x0100
#define	WINE_MIXER_NAME			"WINE OSS Mixer"

#define WINE_CHN_MASK(_x)		(1L << (_x))
#define WINE_CHN_SUPPORTS(_c, _x)	((_c) & WINE_CHN_MASK(_x))
/* Bass and Treble are no longer in the mask as Windows does not handle them */
#define WINE_MIXER_MASK_SPEAKER		(WINE_CHN_MASK(SOUND_MIXER_SYNTH)  | \
                                         WINE_CHN_MASK(SOUND_MIXER_PCM)    | \
                                         WINE_CHN_MASK(SOUND_MIXER_LINE)   | \
                                         WINE_CHN_MASK(SOUND_MIXER_MIC)    | \
                                         WINE_CHN_MASK(SOUND_MIXER_CD)     )

#define WINE_MIXER_MASK_RECORD		(WINE_CHN_MASK(SOUND_MIXER_SYNTH)  | \
                                         WINE_CHN_MASK(SOUND_MIXER_LINE)   | \
                                         WINE_CHN_MASK(SOUND_MIXER_MIC)    | \
                                         WINE_CHN_MASK(SOUND_MIXER_IMIX)   )

/* FIXME: the two following string arrays should be moved to a resource file in a string table */
/* if it's done, better use a struct to hold labels, name, and muted channel volume cache */
static char*	MIX_Labels[SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_LABELS;
static char*	MIX_Names [SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_NAMES;

struct mixerCtrl
{
    DWORD		dwLineID;
    MIXERCONTROLA	ctrl;
};

struct mixer
{
    const char*		name;
    int			volume[SOUND_MIXER_NRDEVICES];
    int			devMask;
    int			stereoMask;
    int			recMask;
    BOOL		singleRecChannel;
    struct mixerCtrl*	ctrl;
    int			numCtrl;
};

#define LINEID_DST	0xFFFF
#define LINEID_SPEAKER	0x0000
#define LINEID_RECORD	0x0001

static int		MIX_NumMixers;
static struct mixer	MIX_Mixers[1];

/**************************************************************************
 * 				MIX_FillLineControls		[internal]
 */
static void MIX_FillLineControls(struct mixer* mix, int c, DWORD lineID, DWORD dwType)
{
    struct mixerCtrl* 	mc = &mix->ctrl[c];
    int			j;

    mc->dwLineID = lineID;
    mc->ctrl.cbStruct = sizeof(MIXERCONTROLA);
    mc->ctrl.dwControlID = c + 1;
    mc->ctrl.dwControlType = dwType;

    switch (dwType)
    {
    case MIXERCONTROL_CONTROLTYPE_VOLUME:
	mc->ctrl.fdwControl = 0;
	mc->ctrl.cMultipleItems = 0;
	lstrcpynA(mc->ctrl.szShortName, "Vol", MIXER_SHORT_NAME_CHARS);
	lstrcpynA(mc->ctrl.szName, "Volume", MIXER_LONG_NAME_CHARS);
	memset(&mc->ctrl.Bounds, 0, sizeof(mc->ctrl.Bounds));
	/* CONTROLTYPE_VOLUME uses the MIXER_CONTROLDETAILS_UNSIGNED struct,
	 * [0, 100] is the range supported by OSS
	 * whatever the min and max values are they must match
	 * conversions done in (Get|Set)ControlDetails to stay in [0, 100] range
	 */
	mc->ctrl.Bounds.s1.dwMinimum = 0;
	mc->ctrl.Bounds.s1.dwMaximum = 65535;
	memset(&mc->ctrl.Metrics, 0, sizeof(mc->ctrl.Metrics));
	break;
    case MIXERCONTROL_CONTROLTYPE_MUTE:
    case MIXERCONTROL_CONTROLTYPE_ONOFF:
	mc->ctrl.fdwControl = 0;
	mc->ctrl.cMultipleItems = 0;
	lstrcpynA(mc->ctrl.szShortName, "Mute", MIXER_SHORT_NAME_CHARS);
	lstrcpynA(mc->ctrl.szName, "Mute", MIXER_LONG_NAME_CHARS);
	memset(&mc->ctrl.Bounds, 0, sizeof(mc->ctrl.Bounds));
	mc->ctrl.Bounds.s1.dwMinimum = 0;
	mc->ctrl.Bounds.s1.dwMaximum = 1;
	memset(&mc->ctrl.Metrics, 0, sizeof(mc->ctrl.Metrics));
	break;
    case MIXERCONTROL_CONTROLTYPE_MUX:
    case MIXERCONTROL_CONTROLTYPE_MIXER:
	mc->ctrl.fdwControl = MIXERCONTROL_CONTROLF_MULTIPLE;
	mc->ctrl.cMultipleItems = 0;
	for (j = 0; j < SOUND_MIXER_NRDEVICES; j++)
	    if (WINE_CHN_SUPPORTS(mix->recMask, j))
		mc->ctrl.cMultipleItems++;
	lstrcpynA(mc->ctrl.szShortName, "Mixer", MIXER_SHORT_NAME_CHARS);
	lstrcpynA(mc->ctrl.szName, "Mixer", MIXER_LONG_NAME_CHARS);
	memset(&mc->ctrl.Bounds, 0, sizeof(mc->ctrl.Bounds));
	memset(&mc->ctrl.Metrics, 0, sizeof(mc->ctrl.Metrics));
	break;

    default:
	FIXME("Internal error: unknown type: %08lx\n", dwType);
    }
    TRACE("ctrl[%2d]: typ=%08lx lin=%08lx\n", c + 1, dwType, lineID);
}

/******************************************************************
 *		MIX_GetMixer
 *
 *
 */
static struct mixer*	MIX_Get(WORD wDevID)
{
    if (wDevID >= MIX_NumMixers || MIX_Mixers[wDevID].name == NULL) return NULL;
    return &MIX_Mixers[wDevID];
}

/**************************************************************************
 * 				MIX_Open			[internal]
 */
static DWORD MIX_Open(WORD wDevID, LPMIXEROPENDESC lpMod, DWORD flags)
{
    int			mixer, i, j;
    unsigned 		caps;
    struct mixer*	mix;
    DWORD		ret = MMSYSERR_NOERROR;

    TRACE("(%04X, %p, %lu);\n", wDevID, lpMod, flags);

    /* as we partly init the mixer with MIX_Open, we can allow null open decs */
    /* EPP     if (lpMod == NULL) return MMSYSERR_INVALPARAM; */
    /* anyway, it seems that WINMM/MMSYSTEM doesn't always open the mixer device before sending
     * messages to it... it seems to be linked to all the equivalent of mixer identification
     * (with a reference to a wave, midi.. handle
     */
    if (!(mix = MIX_Get(wDevID))) return MMSYSERR_BADDEVICEID;

    if ((mixer = open(mix->name, O_RDWR)) < 0)
    {
	if (errno == ENODEV || errno == ENXIO)
	{
	    /* no driver present */
	    return MMSYSERR_NODRIVER;
	}
	return MMSYSERR_ERROR;
    }

    if (ioctl(mixer, SOUND_MIXER_READ_DEVMASK, &mix->devMask) == -1)
    {
	perror("ioctl mixer SOUND_MIXER_DEVMASK");
	ret = MMSYSERR_ERROR;
	goto error;
    }
    mix->devMask &= WINE_MIXER_MASK_SPEAKER;
    if (mix->devMask == 0)
    {
	ret = MMSYSERR_NODRIVER;
	goto error;
    }

    if (ioctl(mixer, SOUND_MIXER_READ_STEREODEVS, &mix->stereoMask) == -1)
    {
	perror("ioctl mixer SOUND_MIXER_STEREODEVS");
	ret = MMSYSERR_ERROR;
	goto error;
    }
    mix->stereoMask &= WINE_MIXER_MASK_SPEAKER;

    if (ioctl(mixer, SOUND_MIXER_READ_RECMASK, &mix->recMask) == -1)
    {
	perror("ioctl mixer SOUND_MIXER_RECMASK");
	ret = MMSYSERR_ERROR;
	goto error;
    }
    mix->recMask &= WINE_MIXER_MASK_RECORD;
    /* FIXME: we may need to support both rec lev & igain */
    if (!WINE_CHN_SUPPORTS(mix->recMask, SOUND_MIXER_RECLEV))
    {
	WARN("The sound card doesn't support rec level\n");
	if (WINE_CHN_SUPPORTS(mix->recMask, SOUND_MIXER_IGAIN))
	    WARN("but it does support IGain, please report\n");
    }
    if (ioctl(mixer, SOUND_MIXER_READ_CAPS, &caps) == -1)
    {
	perror("ioctl mixer SOUND_MIXER_READ_CAPS");
	ret = MMSYSERR_ERROR;
	goto error;
    }
    mix->singleRecChannel = caps & SOUND_CAP_EXCL_INPUT;
    TRACE("dev=%04x rec=%04x stereo=%04x %s\n",
	  mix->devMask, mix->recMask, mix->stereoMask,
	  mix->singleRecChannel ? "single" : "multiple");
    for (i = 0; i < SOUND_MIXER_NRDEVICES; i++)
    {
	mix->volume[i] = -1;
    }
    mix->numCtrl = 4; /* dst lines... vol&mute on speakers, vol&onoff on rec */
    /* FIXME: do we always have RECLEV on all cards ??? */
    for (i = 0; i < SOUND_MIXER_NRDEVICES; i++)
    {
	if (WINE_CHN_SUPPORTS(mix->devMask, i))
	    mix->numCtrl += 2; /* volume & mute */
	if (WINE_CHN_SUPPORTS(mix->recMask, i))
	    mix->numCtrl += 2; /* volume & onoff */

    }
    if (!(mix->ctrl = HeapAlloc(GetProcessHeap(), 0, sizeof(mix->ctrl[0]) * mix->numCtrl)))
    {
	ret = MMSYSERR_NOMEM;
	goto error;
    }

    j = 0;
    MIX_FillLineControls(mix, j++, MAKELONG(0, LINEID_DST), MIXERCONTROL_CONTROLTYPE_VOLUME);
    MIX_FillLineControls(mix, j++, MAKELONG(0, LINEID_DST), MIXERCONTROL_CONTROLTYPE_MUTE);
    MIX_FillLineControls(mix, j++, MAKELONG(1, LINEID_DST),
			 mix->singleRecChannel ?
			    MIXERCONTROL_CONTROLTYPE_MUX : MIXERCONTROL_CONTROLTYPE_MIXER);
    MIX_FillLineControls(mix, j++, MAKELONG(1, LINEID_DST), MIXERCONTROL_CONTROLTYPE_MUTE/*EPP*/);
    for (i = 0; i < SOUND_MIXER_NRDEVICES; i++)
    {
	if (WINE_CHN_SUPPORTS(mix->devMask, i))
	{
	    MIX_FillLineControls(mix, j++, MAKELONG(LINEID_SPEAKER, i),
				 MIXERCONTROL_CONTROLTYPE_VOLUME);
	    MIX_FillLineControls(mix, j++, MAKELONG(LINEID_SPEAKER, i),
				 MIXERCONTROL_CONTROLTYPE_MUTE);
	}
    }
    for (i = 0; i < SOUND_MIXER_NRDEVICES; i++)
    {
	if (WINE_CHN_SUPPORTS(mix->recMask, i))
	{
	    MIX_FillLineControls(mix, j++, MAKELONG(LINEID_RECORD, i),
				 MIXERCONTROL_CONTROLTYPE_VOLUME);
	    MIX_FillLineControls(mix, j++, MAKELONG(LINEID_RECORD, i),
				 MIXERCONTROL_CONTROLTYPE_MUTE/*EPP*/);
	}
    }
    assert(j == mix->numCtrl);
 error:
    close(mixer);
    return ret;
}

/**************************************************************************
 * 				MIX_GetVal			[internal]
 */
static	BOOL	MIX_GetVal(struct mixer* mix, int chn, int* val)
{
    int		mixer;
    BOOL	ret = FALSE;

    if ((mixer = open(mix->name, O_RDWR)) < 0)
    {
	/* FIXME: ENXIO => no mixer installed */
	WARN("mixer device not available !\n");
    }
    else
    {
	if (ioctl(mixer, MIXER_READ(chn), val) >= 0)
	{
	    TRACE("Reading volume %x on %d\n", *val, chn);
	    ret = TRUE;
	}
	close(mixer);
    }
    return ret;
}

/**************************************************************************
 * 				MIX_SetVal			[internal]
 */
static	BOOL	MIX_SetVal(struct mixer* mix, int chn, int val)
{
    int		mixer;
    BOOL	ret = FALSE;

    TRACE("Writing volume %x on %d\n", val, chn);

    if ((mixer = open(mix->name, O_RDWR)) < 0)
    {
	/* FIXME: ENXIO => no mixer installed */
	WARN("mixer device not available !\n");
    }
    else
    {
	if (ioctl(mixer, MIXER_WRITE(chn), &val) >= 0)
	{
	    ret = TRUE;
	}
	close(mixer);
    }
    return ret;
}

/******************************************************************
 *		MIX_GetRecSrc
 *
 *
 */
static BOOL	MIX_GetRecSrc(struct mixer* mix, unsigned* mask)
{
    int		mixer;
    BOOL	ret = FALSE;

    if ((mixer = open(mix->name, O_RDWR)) >= 0)
    {
	if (ioctl(mixer, SOUND_MIXER_READ_RECSRC, &mask) >= 0) ret = TRUE;
	close(mixer);
    }
    return ret;
}

/******************************************************************
 *		MIX_SetRecSrc
 *
 *
 */
static BOOL	MIX_SetRecSrc(struct mixer* mix, unsigned mask)
{
    int		mixer;
    BOOL	ret = FALSE;

    if ((mixer = open(mix->name, O_RDWR)) >= 0)
    {
	if (ioctl(mixer, SOUND_MIXER_WRITE_RECSRC, &mask) < 0)
	{
	    ERR("Can't write new mixer settings\n");
	}
	else
	    ret = TRUE;
	close(mixer);
    }
    return ret;
}

/**************************************************************************
 * 				MIX_GetDevCaps			[internal]
 */
static DWORD MIX_GetDevCaps(WORD wDevID, LPMIXERCAPSA lpCaps, DWORD dwSize)
{
    struct mixer*	mix;

    TRACE("(%04X, %p, %lu);\n", wDevID, lpCaps, dwSize);

    if (lpCaps == NULL) return MMSYSERR_INVALPARAM;
    if (!(mix = MIX_Get(wDevID))) return MMSYSERR_BADDEVICEID;

    lpCaps->wMid = WINE_MIXER_MANUF_ID;
    lpCaps->wPid = WINE_MIXER_PRODUCT_ID;
    lpCaps->vDriverVersion = WINE_MIXER_VERSION;
    strcpy(lpCaps->szPname, WINE_MIXER_NAME);

    lpCaps->cDestinations = 2; /* speakers & record */
    lpCaps->fdwSupport = 0; /* No bits defined yet */

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				MIX_GetLineInfoDst	[internal]
 */
static	DWORD	MIX_GetLineInfoDst(struct mixer* mix, LPMIXERLINEA lpMl, DWORD dst)
{
    unsigned mask;
    int	j;

    lpMl->dwDestination = dst;
    switch (dst)
    {
    case 0:
	lpMl->dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
	mask = mix->devMask;
	j = SOUND_MIXER_VOLUME;
	break;
    case 1:
	lpMl->dwComponentType = MIXERLINE_COMPONENTTYPE_DST_WAVEIN;
	mask = mix->recMask;
	j = SOUND_MIXER_RECLEV;
	break;
    default:
	FIXME("shouldn't happen\n");
	return MMSYSERR_ERROR;
    }
    lpMl->dwSource = 0xFFFFFFFF;
    lstrcpynA(lpMl->szShortName, MIX_Labels[j], MIXER_SHORT_NAME_CHARS);
    lstrcpynA(lpMl->szName, MIX_Names[j], MIXER_LONG_NAME_CHARS);

    /* we have all connections found in the MIX_DevMask */
    lpMl->cConnections = 0;
    for (j = 0; j < SOUND_MIXER_NRDEVICES; j++)
	if (WINE_CHN_SUPPORTS(mask, j))
	    lpMl->cConnections++;
    lpMl->cChannels = 1;
    if (WINE_CHN_SUPPORTS(mix->stereoMask, lpMl->dwLineID))
	lpMl->cChannels++;
    lpMl->dwLineID = MAKELONG(dst, LINEID_DST);
    lpMl->cControls = 0;
    for (j = 0; j < mix->numCtrl; j++)
	if (mix->ctrl[j].dwLineID == lpMl->dwLineID)
	    lpMl->cControls++;

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				MIX_GetLineInfoSrc	[internal]
 */
static	DWORD	MIX_GetLineInfoSrc(struct mixer* mix, LPMIXERLINEA lpMl, DWORD idx, DWORD dst)
{
    int		i, j;
    unsigned	mask = (dst) ? mix->recMask : mix->devMask;

    strcpy(lpMl->szShortName, MIX_Labels[idx]);
    strcpy(lpMl->szName, MIX_Names[idx]);
    lpMl->dwLineID = MAKELONG(dst, idx);
    lpMl->dwDestination = dst;
    lpMl->cConnections = 1;
    lpMl->cControls = 0;
    for (i = 0; i < mix->numCtrl; i++)
	if (mix->ctrl[i].dwLineID == lpMl->dwLineID)
	    lpMl->cControls++;

    switch (idx)
    {
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
    case SOUND_MIXER_IMIX:
	lpMl->dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_UNDEFINED;
	lpMl->fdwLine	 |= MIXERLINE_LINEF_SOURCE;
	break;
    default:
	WARN("Index %ld not handled.\n", idx);
	return MIXERR_INVALLINE;
    }
    lpMl->cChannels = 1;
    if (dst == 0 && WINE_CHN_SUPPORTS(mix->stereoMask, idx))
	lpMl->cChannels++;
    for (i = j = 0; j < SOUND_MIXER_NRDEVICES; j++)
    {
	if (WINE_CHN_SUPPORTS(mask, j))
	{
	    if (j == idx) break;
	    i++;
	}
    }
    lpMl->dwSource = i;
    return MMSYSERR_NOERROR;
}

/******************************************************************
 *		MIX_CheckLine
 */
static BOOL MIX_CheckLine(DWORD lineID)
{
    return ((HIWORD(lineID) < SOUND_MIXER_NRDEVICES && LOWORD(lineID) < 2) ||
	    (HIWORD(lineID) == LINEID_DST && LOWORD(lineID) < SOUND_MIXER_NRDEVICES));
}

/**************************************************************************
 * 				MIX_GetLineInfo			[internal]
 */
static DWORD MIX_GetLineInfo(WORD wDevID, LPMIXERLINEA lpMl, DWORD fdwInfo)
{
    int 		i, j;
    DWORD		ret = MMSYSERR_NOERROR;
    unsigned		mask;
    struct mixer*	mix;

    TRACE("(%04X, %p, %lu);\n", wDevID, lpMl, fdwInfo);

    if (lpMl == NULL || lpMl->cbStruct != sizeof(*lpMl))
	return MMSYSERR_INVALPARAM;
    if ((mix = MIX_Get(wDevID)) == NULL) return MMSYSERR_BADDEVICEID;

    /* FIXME: set all the variables correctly... the lines below
     * are very wrong...
     */
    lpMl->fdwLine	= MIXERLINE_LINEF_ACTIVE;
    lpMl->dwUser	= 0;

    switch (fdwInfo & MIXER_GETLINEINFOF_QUERYMASK)
    {
    case MIXER_GETLINEINFOF_DESTINATION:
	TRACE("DESTINATION (%08lx)\n", lpMl->dwDestination);
	if (lpMl->dwDestination >= 2)
	    return MMSYSERR_INVALPARAM;
	if ((ret = MIX_GetLineInfoDst(mix, lpMl, lpMl->dwDestination)) != MMSYSERR_NOERROR)
	    return ret;
	break;
    case MIXER_GETLINEINFOF_SOURCE:
	TRACE("SOURCE (%08lx), dst=%08lx\n", lpMl->dwSource, lpMl->dwDestination);
	switch (lpMl->dwDestination)
	{
	case 0: mask = mix->devMask; break;
	case 1: mask = mix->recMask; break;
	default: return MMSYSERR_INVALPARAM;
	}
	i = lpMl->dwSource;
	for (j = 0; j < SOUND_MIXER_NRDEVICES; j++)
	{
	    if (WINE_CHN_SUPPORTS(mask, j) && (i-- == 0))
		break;
	}
	if (j >= SOUND_MIXER_NRDEVICES)
	    return MIXERR_INVALLINE;
	if ((ret = MIX_GetLineInfoSrc(mix, lpMl, j, lpMl->dwDestination)) != MMSYSERR_NOERROR)
	    return ret;
	break;
    case MIXER_GETLINEINFOF_LINEID:
	TRACE("LINEID (%08lx)\n", lpMl->dwLineID);

	if (!MIX_CheckLine(lpMl->dwLineID))
	    return MIXERR_INVALLINE;
	if (HIWORD(lpMl->dwLineID) == LINEID_DST)
	    ret = MIX_GetLineInfoDst(mix, lpMl, LOWORD(lpMl->dwLineID));
	else
	    ret = MIX_GetLineInfoSrc(mix, lpMl, HIWORD(lpMl->dwLineID), LOWORD(lpMl->dwLineID));
	if (ret != MMSYSERR_NOERROR)
	    return ret;
	break;
    case MIXER_GETLINEINFOF_COMPONENTTYPE:
	TRACE("COMPONENT TYPE (%08lx)\n", lpMl->dwComponentType);
	switch (lpMl->dwComponentType)
	{
	case MIXERLINE_COMPONENTTYPE_DST_SPEAKERS:
	    ret = MIX_GetLineInfoDst(mix, lpMl, 0);
	    break;
	case MIXERLINE_COMPONENTTYPE_DST_WAVEIN:
	    ret = MIX_GetLineInfoDst(mix, lpMl, 1);
	    break;
	case MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER:
	    ret = MIX_GetLineInfoSrc(mix, lpMl, SOUND_MIXER_SYNTH, 0);
	    break;
	case MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC:
	    ret = MIX_GetLineInfoSrc(mix, lpMl, SOUND_MIXER_CD, 0);
	    break;
	case MIXERLINE_COMPONENTTYPE_SRC_LINE:
	    ret = MIX_GetLineInfoSrc(mix, lpMl, SOUND_MIXER_LINE, 0);
	    break;
	case MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE:
	    ret = MIX_GetLineInfoSrc(mix, lpMl, SOUND_MIXER_MIC, 1);
	    break;
	case MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT:
	    ret = MIX_GetLineInfoSrc(mix, lpMl, SOUND_MIXER_PCM, 0);
	    break;
	case MIXERLINE_COMPONENTTYPE_SRC_UNDEFINED:
	    ret = MIX_GetLineInfoSrc(mix, lpMl, SOUND_MIXER_IMIX, 1);
	    break;
	default:
	    FIXME("Unhandled component type (%08lx)\n", lpMl->dwComponentType);
	    return MMSYSERR_INVALPARAM;
	}
	break;
    case MIXER_GETLINEINFOF_TARGETTYPE:
	FIXME("_TARGETTYPE not implemented yet.\n");
	break;
    default:
	WARN("Unknown flag (%08lx)\n", fdwInfo & MIXER_GETLINEINFOF_QUERYMASK);
	break;
    }

    lpMl->Target.dwType = MIXERLINE_TARGETTYPE_AUX; /* FIXME */
    lpMl->Target.dwDeviceID = 0xFFFFFFFF;
    lpMl->Target.wMid = WINE_MIXER_MANUF_ID;
    lpMl->Target.wPid = WINE_MIXER_PRODUCT_ID;
    lpMl->Target.vDriverVersion = WINE_MIXER_VERSION;
    strcpy(lpMl->Target.szPname, WINE_MIXER_NAME);

    return ret;
}

/******************************************************************
 *		MIX_CheckControl
 *
 */
static BOOL	MIX_CheckControl(struct mixer* mix, DWORD ctrlID)
{
    return (ctrlID >= 1 && ctrlID <= mix->numCtrl);
}

/**************************************************************************
 * 				MIX_GetLineControls		[internal]
 */
static	DWORD	MIX_GetLineControls(WORD wDevID, LPMIXERLINECONTROLSA lpMlc, DWORD flags)
{
    DWORD		dwRet = MMSYSERR_NOERROR;
    struct mixer*	mix;

    TRACE("(%04X, %p, %lu);\n", wDevID, lpMlc, flags);

    if (lpMlc == NULL) return MMSYSERR_INVALPARAM;
    if (lpMlc->cbStruct < sizeof(*lpMlc) ||
	lpMlc->cbmxctrl < sizeof(MIXERCONTROLA))
	return MMSYSERR_INVALPARAM;
    if ((mix = MIX_Get(wDevID)) == NULL) return MMSYSERR_BADDEVICEID;

    switch (flags & MIXER_GETLINECONTROLSF_QUERYMASK)
    {
    case MIXER_GETLINECONTROLSF_ALL:
        {
	    int		i, j;

	    TRACE("line=%08lx GLCF_ALL (%ld)\n", lpMlc->dwLineID, lpMlc->cControls);

	    for (i = j = 0; i < mix->numCtrl; i++)
	    {
		if (mix->ctrl[i].dwLineID == lpMlc->dwLineID)
		    j++;
	    }
	    if (!j || lpMlc->cControls != j)		dwRet = MMSYSERR_INVALPARAM;
	    else if (!MIX_CheckLine(lpMlc->dwLineID))	dwRet = MIXERR_INVALLINE;
	    else
	    {
		for (i = j = 0; i < mix->numCtrl; i++)
		{
		    if (mix->ctrl[i].dwLineID == lpMlc->dwLineID)
		    {
			TRACE("[%d] => [%2d]: typ=%08lx\n", j, i + 1, mix->ctrl[i].ctrl.dwControlType);
			lpMlc->pamxctrl[j++] = mix->ctrl[i].ctrl;
		    }
		}
	    }
	}
	break;
    case MIXER_GETLINECONTROLSF_ONEBYID:
	TRACE("line=%08lx GLCF_ONEBYID (%lx)\n", lpMlc->dwLineID, lpMlc->u.dwControlID);

	if (!MIX_CheckControl(mix, lpMlc->u.dwControlID) ||
	    mix->ctrl[lpMlc->u.dwControlID - 1].dwLineID != lpMlc->dwLineID)
	    dwRet = MMSYSERR_INVALPARAM;
	else
	    lpMlc->pamxctrl[0] = mix->ctrl[lpMlc->u.dwControlID - 1].ctrl;
	break;
    case MIXER_GETLINECONTROLSF_ONEBYTYPE:
	TRACE("line=%08lx GLCF_ONEBYTYPE (%lx)\n", lpMlc->dwLineID, lpMlc->u.dwControlType);
	if (!MIX_CheckLine(lpMlc->dwLineID))	dwRet = MIXERR_INVALLINE;
	else
	{
	    int		i;
	    DWORD	ct = lpMlc->u.dwControlType & MIXERCONTROL_CT_CLASS_MASK;

	    for (i = 0; i < mix->numCtrl; i++)
	    {
		if (mix->ctrl[i].dwLineID == lpMlc->dwLineID &&
		    ct == (mix->ctrl[i].ctrl.dwControlType & MIXERCONTROL_CT_CLASS_MASK))
		{
		    lpMlc->pamxctrl[0] = mix->ctrl[i].ctrl;
		    break;
		}
	    }
	    if (i == mix->numCtrl) dwRet = MMSYSERR_INVALPARAM;
	}
	break;
    default:
	ERR("Unknown flag %08lx\n", flags & MIXER_GETLINECONTROLSF_QUERYMASK);
	dwRet = MMSYSERR_INVALPARAM;
    }

    return dwRet;
}

/**************************************************************************
 * 				MIX_GetControlDetails		[internal]
 */
static	DWORD	MIX_GetControlDetails(WORD wDevID, LPMIXERCONTROLDETAILS lpmcd, DWORD fdwDetails)
{
    DWORD		ret = MMSYSERR_NOTSUPPORTED;
    DWORD		c, chnl;
    struct mixer*	mix;

    TRACE("(%04X, %p, %lu);\n", wDevID, lpmcd, fdwDetails);

    if (lpmcd == NULL) return MMSYSERR_INVALPARAM;
    if ((mix = MIX_Get(wDevID)) == NULL) return MMSYSERR_BADDEVICEID;

    switch (fdwDetails & MIXER_GETCONTROLDETAILSF_QUERYMASK)
    {
    case MIXER_GETCONTROLDETAILSF_VALUE:
	TRACE("GCD VALUE (%08lx)\n", lpmcd->dwControlID);
	if (MIX_CheckControl(mix, lpmcd->dwControlID))
	{
	    c = lpmcd->dwControlID - 1;
	    chnl = HIWORD(mix->ctrl[c].dwLineID);
	    if (chnl == LINEID_DST)
		chnl = LOWORD(mix->ctrl[c].dwLineID) ? SOUND_MIXER_RECLEV : SOUND_MIXER_VOLUME;
	    switch (mix->ctrl[c].ctrl.dwControlType)
	    {
	    case MIXERCONTROL_CONTROLTYPE_VOLUME:
		{
		    LPMIXERCONTROLDETAILS_UNSIGNED	mcdu;
		    int					val;

		    TRACE(" <> %u %lu\n", sizeof(MIXERCONTROLDETAILS_UNSIGNED), lpmcd->cbDetails);
		    /* return value is 00RL (4 bytes)... */
		    if ((val = mix->volume[chnl]) == -1 && !MIX_GetVal(mix, chnl, &val))
			return MMSYSERR_INVALPARAM;

		    switch (lpmcd->cChannels)
		    {
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
		break;
	    case MIXERCONTROL_CONTROLTYPE_MUTE:
	    case MIXERCONTROL_CONTROLTYPE_ONOFF:
		{
		    LPMIXERCONTROLDETAILS_BOOLEAN	mcdb;

		    TRACE(" <> %u %lu\n", sizeof(MIXERCONTROLDETAILS_BOOLEAN), lpmcd->cbDetails);
		    /* we mute both channels at the same time */
		    mcdb = (LPMIXERCONTROLDETAILS_BOOLEAN)lpmcd->paDetails;
		    mcdb->fValue = (mix->volume[chnl] != -1);
		    TRACE("=> %s\n", mcdb->fValue ? "on" : "off");
		}
		break;
	    case MIXERCONTROL_CONTROLTYPE_MIXER:
	    case MIXERCONTROL_CONTROLTYPE_MUX:
		{
		    unsigned				mask;

		    TRACE(" <> %u %lu\n", sizeof(MIXERCONTROLDETAILS_BOOLEAN), lpmcd->cbDetails);
		    if (!MIX_GetRecSrc(mix, &mask))
		    {
			/* FIXME: ENXIO => no mixer installed */
			WARN("mixer device not available !\n");
			ret = MMSYSERR_ERROR;
		    }
		    else
		    {
			LPMIXERCONTROLDETAILS_BOOLEAN	mcdb;
			int				i, j;

			/* we mute both channels at the same time */
			mcdb = (LPMIXERCONTROLDETAILS_BOOLEAN)lpmcd->paDetails;

			for (i = j = 0; j < SOUND_MIXER_NRDEVICES; j++)
			{
			    if (WINE_CHN_SUPPORTS(mix->recMask, j))
			    {
				if (i >= lpmcd->u.cMultipleItems)
				    return MMSYSERR_INVALPARAM;
				mcdb[i++].fValue = WINE_CHN_SUPPORTS(mask, j);
			    }
			}
		    }
		}
		break;
	    default:
		WARN("Unsupported\n");
	    }
	    ret = MMSYSERR_NOERROR;
	}
	else
	{
	    ret = MMSYSERR_INVALPARAM;
	}
	break;
    case MIXER_GETCONTROLDETAILSF_LISTTEXT:
	TRACE("LIST TEXT (%08lx)\n", lpmcd->dwControlID);

	ret = MMSYSERR_INVALPARAM;
	if (MIX_CheckControl(mix, lpmcd->dwControlID))
	{
	    int	c = lpmcd->dwControlID - 1;

	    if (mix->ctrl[c].ctrl.dwControlType == MIXERCONTROL_CONTROLTYPE_MUX ||
		mix->ctrl[c].ctrl.dwControlType == MIXERCONTROL_CONTROLTYPE_MIXER)
	    {
		LPMIXERCONTROLDETAILS_LISTTEXTA	mcdlt;
		int i, j;

		mcdlt = (LPMIXERCONTROLDETAILS_LISTTEXTA)lpmcd->paDetails;
		for (i = j = 0; j < SOUND_MIXER_NRDEVICES; j++)
		{
		    if (WINE_CHN_SUPPORTS(mix->recMask, j))
		    {
			mcdlt[i].dwParam1 = MAKELONG(LINEID_RECORD, j);
			mcdlt[i].dwParam2 = 0;
			strcpy(mcdlt[i].szName, MIX_Names[j]);
			i++;
		    }
		}
		if (i != lpmcd->u.cMultipleItems) FIXME("bad count\n");
		ret = MMSYSERR_NOERROR;
	    }
	}
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
    DWORD		ret = MMSYSERR_NOTSUPPORTED;
    DWORD		c, chnl;
    int			val;
    struct mixer*	mix;

    TRACE("(%04X, %p, %lu);\n", wDevID, lpmcd, fdwDetails);

    if (lpmcd == NULL) return MMSYSERR_INVALPARAM;
    if ((mix = MIX_Get(wDevID)) == NULL) return MMSYSERR_BADDEVICEID;

    switch (fdwDetails & MIXER_GETCONTROLDETAILSF_QUERYMASK)
    {
    case MIXER_GETCONTROLDETAILSF_VALUE:
	TRACE("GCD VALUE (%08lx)\n", lpmcd->dwControlID);
	if (MIX_CheckControl(mix, lpmcd->dwControlID))
	{
	    c = lpmcd->dwControlID - 1;
	    chnl = HIWORD(mix->ctrl[c].dwLineID);
	    if (chnl == LINEID_DST)
		chnl = LOWORD(mix->ctrl[c].dwLineID) ? SOUND_MIXER_RECLEV : SOUND_MIXER_VOLUME;

	    switch (mix->ctrl[c].ctrl.dwControlType)
	    {
	    case MIXERCONTROL_CONTROLTYPE_VOLUME:
		{
		    LPMIXERCONTROLDETAILS_UNSIGNED	mcdu;

		    TRACE(" <> %u %lu\n", sizeof(MIXERCONTROLDETAILS_UNSIGNED), lpmcd->cbDetails);
		    /* val should contain 00RL */
		    switch (lpmcd->cChannels)
		    {
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

		    if (mix->volume[chnl] == -1)
		    {
			if (!MIX_SetVal(mix, chnl, val))
			    return MMSYSERR_INVALPARAM;
		    }
		    else
		    {
			mix->volume[chnl] = val;
		    }
		}
		ret = MMSYSERR_NOERROR;
		break;
	    case MIXERCONTROL_CONTROLTYPE_MUTE:
	    case MIXERCONTROL_CONTROLTYPE_ONOFF:
		{
		    LPMIXERCONTROLDETAILS_BOOLEAN	mcdb;

		    TRACE(" <> %u %lu\n", sizeof(MIXERCONTROLDETAILS_BOOLEAN), lpmcd->cbDetails);
		    mcdb = (LPMIXERCONTROLDETAILS_BOOLEAN)lpmcd->paDetails;
		    if (mcdb->fValue)
		    {
			if (!MIX_GetVal(mix, chnl, &mix->volume[chnl]) || !MIX_SetVal(mix, chnl, 0))
			    return MMSYSERR_INVALPARAM;
		    }
		    else
		    {
			if (mix->volume[chnl] == -1)
			{
			    ret = MMSYSERR_NOERROR;
			    break;
			}
			if (!MIX_SetVal(mix, chnl, mix->volume[chnl]))
			    return MMSYSERR_INVALPARAM;
			mix->volume[chnl] = -1;
		    }
		}
		ret = MMSYSERR_NOERROR;
		break;
	    case MIXERCONTROL_CONTROLTYPE_MIXER:
	    case MIXERCONTROL_CONTROLTYPE_MUX:
		{
		    LPMIXERCONTROLDETAILS_BOOLEAN	mcdb;
		    unsigned				mask;
		    int					i, j;

		    TRACE(" <> %u %lu\n", sizeof(MIXERCONTROLDETAILS_BOOLEAN), lpmcd->cbDetails);
		    /* we mute both channels at the same time */
		    mcdb = (LPMIXERCONTROLDETAILS_BOOLEAN)lpmcd->paDetails;
		    mask = 0;
		    for (i = j = 0; j < SOUND_MIXER_NRDEVICES; j++)
		    {
			if (WINE_CHN_SUPPORTS(mix->recMask, j) && mcdb[i++].fValue)
			{
			    /* a mux can only select one line at a time... */
			    if (mix->singleRecChannel && mask != 0)
			    {
				FIXME("!!!\n");
				return MMSYSERR_INVALPARAM;
			    }
			    mask |= WINE_CHN_MASK(j);
			}
		    }
		    if (i != lpmcd->u.cMultipleItems) FIXME("bad count\n");
		    TRACE("writing %04x as rec src\n", mask);
		    if (!MIX_SetRecSrc(mix, mask))
			ERR("Can't write new mixer settings\n");
		    else
			ret = MMSYSERR_NOERROR;
		}
		break;
	    }
	}
	break;
    default:
	WARN("Unknown SetControlDetails flag (%08lx)\n", fdwDetails & MIXER_GETCONTROLDETAILSF_QUERYMASK);
    }
    return ret;
}

/**************************************************************************
 * 				MIX_Init			[internal]
 */
static	DWORD	MIX_Init(void)
{
    int	mixer;

#define MIXER_DEV "/dev/mixer"
    if ((mixer = open(MIXER_DEV, O_RDWR)) < 0)
    {
	if (errno == ENODEV || errno == ENXIO)
	{
	    /* no driver present */
	    return MMSYSERR_NODRIVER;
	}
	MIX_NumMixers = 0;
	return MMSYSERR_ERROR;
    }
    close(mixer);
    MIX_NumMixers = 1;
    MIX_Mixers[0].name = MIXER_DEV;
    MIX_Open(0, NULL, 0); /* FIXME */
#undef MIXER_DEV
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				MIX_GetNumDevs			[internal]
 */
static	DWORD	MIX_GetNumDevs(void)
{
    return MIX_NumMixers;
}

#endif /* HAVE_OSS */

/**************************************************************************
 * 				mxdMessage (WINEOSS.3)
 */
DWORD WINAPI OSS_mxdMessage(UINT wDevID, UINT wMsg, DWORD dwUser,
			    DWORD dwParam1, DWORD dwParam2)
{
/* TRACE("(%04X, %04X, %08lX, %08lX, %08lX);\n", wDevID, wMsg, dwUser, dwParam1, dwParam2); */

#ifdef HAVE_OSS
    switch (wMsg)
    {
    case DRVM_INIT:
	return MIX_Init();
    case DRVM_EXIT:
    case DRVM_ENABLE:
    case DRVM_DISABLE:
	/* FIXME: Pretend this is supported */
	return 0;
    case MXDM_GETDEVCAPS:
	return MIX_GetDevCaps(wDevID, (LPMIXERCAPSA)dwParam1, dwParam2);
    case MXDM_GETLINEINFO:
	return MIX_GetLineInfo(wDevID, (LPMIXERLINEA)dwParam1, dwParam2);
    case MXDM_GETNUMDEVS:
	return MIX_GetNumDevs();
    case MXDM_OPEN:
	return MMSYSERR_NOERROR;
	/* MIX_Open(wDevID, (LPMIXEROPENDESC)dwParam1, dwParam2); */
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
	return MMSYSERR_NOTSUPPORTED;
    }
#else
    return MMSYSERR_NOTENABLED;
#endif
}
