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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* TODO:
 * + implement notification mechanism when state of mixer's controls
 */

#include "config.h"
#include "wine/port.h"

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
#include "winnls.h"
#include "mmddk.h"
#include "oss.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mixer);

#ifdef HAVE_OSS

#define MAX_MIXERDRV     (6)

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
static const char * const MIX_Labels[SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_LABELS;
static const char * const MIX_Names [SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_NAMES;

struct mixerCtrl
{
    DWORD		dwLineID;
    MIXERCONTROLW	ctrl;
};

struct mixer
{
    char*		name;
    char*		dev_name;
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
static struct mixer	MIX_Mixers[MAX_MIXERDRV];

/**************************************************************************
 */

static const char * getMessage(UINT uMsg)
{
    static char str[64];
#define MSG_TO_STR(x) case x: return #x;
    switch (uMsg) {
    MSG_TO_STR(DRVM_INIT);
    MSG_TO_STR(DRVM_EXIT);
    MSG_TO_STR(DRVM_ENABLE);
    MSG_TO_STR(DRVM_DISABLE);
    MSG_TO_STR(MXDM_GETDEVCAPS);
    MSG_TO_STR(MXDM_GETLINEINFO);
    MSG_TO_STR(MXDM_GETNUMDEVS);
    MSG_TO_STR(MXDM_OPEN);
    MSG_TO_STR(MXDM_CLOSE);
    MSG_TO_STR(MXDM_GETLINECONTROLS);
    MSG_TO_STR(MXDM_GETCONTROLDETAILS);
    MSG_TO_STR(MXDM_SETCONTROLDETAILS);
    }
#undef MSG_TO_STR
    sprintf(str, "UNKNOWN(%08x)", uMsg);
    return str;
}

static const char * getIoctlCommand(int command)
{
    static char str[64];
#define IOCTL_TO_STR(x) case x: return #x;
    switch (command) {
    IOCTL_TO_STR(SOUND_MIXER_VOLUME);
    IOCTL_TO_STR(SOUND_MIXER_BASS);
    IOCTL_TO_STR(SOUND_MIXER_TREBLE);
    IOCTL_TO_STR(SOUND_MIXER_SYNTH);
    IOCTL_TO_STR(SOUND_MIXER_PCM);
    IOCTL_TO_STR(SOUND_MIXER_SPEAKER);
    IOCTL_TO_STR(SOUND_MIXER_LINE);
    IOCTL_TO_STR(SOUND_MIXER_MIC);
    IOCTL_TO_STR(SOUND_MIXER_CD);
    IOCTL_TO_STR(SOUND_MIXER_IMIX);
    IOCTL_TO_STR(SOUND_MIXER_ALTPCM);
    IOCTL_TO_STR(SOUND_MIXER_RECLEV);
    IOCTL_TO_STR(SOUND_MIXER_IGAIN);
    IOCTL_TO_STR(SOUND_MIXER_OGAIN);
    IOCTL_TO_STR(SOUND_MIXER_LINE1);
    IOCTL_TO_STR(SOUND_MIXER_LINE2);
    IOCTL_TO_STR(SOUND_MIXER_LINE3);
    IOCTL_TO_STR(SOUND_MIXER_DIGITAL1);
    IOCTL_TO_STR(SOUND_MIXER_DIGITAL2);
    IOCTL_TO_STR(SOUND_MIXER_DIGITAL3);
#ifdef SOUND_MIXER_PHONEIN
    IOCTL_TO_STR(SOUND_MIXER_PHONEIN);
#endif
#ifdef SOUND_MIXER_PHONEOUT
    IOCTL_TO_STR(SOUND_MIXER_PHONEOUT);
#endif
    IOCTL_TO_STR(SOUND_MIXER_VIDEO);
    IOCTL_TO_STR(SOUND_MIXER_RADIO);
#ifdef SOUND_MIXER_MONITOR
    IOCTL_TO_STR(SOUND_MIXER_MONITOR);
#endif
    }
#undef IOCTL_TO_STR
    sprintf(str, "UNKNOWN(%08x)", command);
    return str;
}

static const char * getControlType(DWORD dwControlType)
{
    static char str[64];
#define TYPE_TO_STR(x) case x: return #x;
    switch (dwControlType) {
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_CUSTOM);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_BOOLEANMETER);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_SIGNEDMETER);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_PEAKMETER);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_UNSIGNEDMETER);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_BOOLEAN);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_ONOFF);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_MUTE);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_MONO);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_LOUDNESS);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_STEREOENH);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_BASS_BOOST);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_BUTTON);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_DECIBELS);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_SIGNED);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_UNSIGNED);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_PERCENT);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_SLIDER);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_PAN);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_QSOUNDPAN);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_FADER);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_VOLUME);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_BASS);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_TREBLE);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_EQUALIZER);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_SINGLESELECT);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_MUX);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_MULTIPLESELECT);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_MIXER);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_MICROTIME);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_MILLITIME);
    }
#undef TYPE_TO_STR
    sprintf(str, "UNKNOWN(%08x)", dwControlType);
    return str;
}

static const char * getComponentType(DWORD dwComponentType)
{
    static char str[64];
#define TYPE_TO_STR(x) case x: return #x;
    switch (dwComponentType) {
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_DST_UNDEFINED);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_DST_DIGITAL);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_DST_LINE);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_DST_MONITOR);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_DST_SPEAKERS);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_DST_HEADPHONES);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_DST_TELEPHONE);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_DST_WAVEIN);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_DST_VOICEIN);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_SRC_UNDEFINED);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_SRC_DIGITAL);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_SRC_LINE);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_SRC_TELEPHONE);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_SRC_PCSPEAKER);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_SRC_AUXILIARY);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_SRC_ANALOG);
    }
#undef TYPE_TO_STR
    sprintf(str, "UNKNOWN(%08x)", dwComponentType);
    return str;
}

static const char * getTargetType(DWORD dwType)
{
    static char str[64];
#define TYPE_TO_STR(x) case x: return #x;
    switch (dwType) {
    TYPE_TO_STR(MIXERLINE_TARGETTYPE_UNDEFINED);
    TYPE_TO_STR(MIXERLINE_TARGETTYPE_WAVEOUT);
    TYPE_TO_STR(MIXERLINE_TARGETTYPE_WAVEIN);
    TYPE_TO_STR(MIXERLINE_TARGETTYPE_MIDIOUT);
    TYPE_TO_STR(MIXERLINE_TARGETTYPE_MIDIIN);
    TYPE_TO_STR(MIXERLINE_TARGETTYPE_AUX);
    }
#undef TYPE_TO_STR
    sprintf(str, "UNKNOWN(%08x)", dwType);
    return str;
}

static const WCHAR sz_short_volume [] = {'V','o','l',0};
static const WCHAR sz_long_volume  [] = {'V','o','l','u','m','e',0};
static const WCHAR sz_shrtlng_mute [] = {'M','u','t','e',0};
static const WCHAR sz_shrtlng_mixer[] = {'M','i','x','e','r',0};

/**************************************************************************
 * 				MIX_FillLineControls		[internal]
 */
static void MIX_FillLineControls(struct mixer* mix, int c, DWORD lineID,
                                 DWORD dwControlType)
{
    struct mixerCtrl* 	mc = &mix->ctrl[c];
    int			j;

    TRACE("(%p, %d, %08x, %s)\n", mix, c, lineID,
          getControlType(dwControlType));

    mc->dwLineID = lineID;
    mc->ctrl.cbStruct = sizeof(MIXERCONTROLW);
    mc->ctrl.dwControlID = c + 1;
    mc->ctrl.dwControlType = dwControlType;

    switch (dwControlType)
    {
    case MIXERCONTROL_CONTROLTYPE_VOLUME:
	mc->ctrl.fdwControl = 0;
	mc->ctrl.cMultipleItems = 0;
	strcpyW(mc->ctrl.szShortName, sz_short_volume);
	strcpyW(mc->ctrl.szName, sz_long_volume);
	memset(&mc->ctrl.Bounds, 0, sizeof(mc->ctrl.Bounds));
	/* CONTROLTYPE_VOLUME uses the MIXER_CONTROLDETAILS_UNSIGNED struct,
	 * [0, 100] is the range supported by OSS
	 * whatever the min and max values are they must match
	 * conversions done in (Get|Set)ControlDetails to stay in [0, 100] range
	 */
	mc->ctrl.Bounds.s1.dwMinimum = 0;
	mc->ctrl.Bounds.s1.dwMaximum = 65535;
	memset(&mc->ctrl.Metrics, 0, sizeof(mc->ctrl.Metrics));
        mc->ctrl.Metrics.cSteps = 656;
	break;
    case MIXERCONTROL_CONTROLTYPE_MUTE:
    case MIXERCONTROL_CONTROLTYPE_ONOFF:
	mc->ctrl.fdwControl = 0;
	mc->ctrl.cMultipleItems = 0;
	strcpyW(mc->ctrl.szShortName, sz_shrtlng_mute);
	strcpyW(mc->ctrl.szName, sz_shrtlng_mute);
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
	strcpyW(mc->ctrl.szShortName, sz_shrtlng_mixer);
	strcpyW(mc->ctrl.szName, sz_shrtlng_mixer);
	memset(&mc->ctrl.Bounds, 0, sizeof(mc->ctrl.Bounds));
        mc->ctrl.Bounds.s1.dwMaximum = mc->ctrl.cMultipleItems - 1;
	memset(&mc->ctrl.Metrics, 0, sizeof(mc->ctrl.Metrics));
        mc->ctrl.Metrics.cSteps = mc->ctrl.cMultipleItems;
	break;

    default:
	FIXME("Internal error: unknown type: %08x\n", dwControlType);
    }
    TRACE("ctrl[%2d]: typ=%08x lin=%08x\n", c + 1, dwControlType, lineID);
}

/******************************************************************
 *		MIX_GetMixer
 *
 *
 */
static struct mixer*	MIX_Get(WORD wDevID)
{
    TRACE("(%04x)\n", wDevID);

    if (wDevID >= MIX_NumMixers || MIX_Mixers[wDevID].dev_name == NULL)
        return NULL;

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

    TRACE("(%04X, %p, %u);\n", wDevID, lpMod, flags);

    /* as we partly init the mixer with MIX_Open, we can allow null open decs
     * EPP     if (lpMod == NULL) return MMSYSERR_INVALPARAM;
     * anyway, it seems that WINMM/MMSYSTEM doesn't always open the mixer
     * device before sending messages to it... it seems to be linked to all
     * the equivalent of mixer identification
     * (with a reference to a wave, midi.. handle
     */
    if ((mix = MIX_Get(wDevID)) == NULL) {
        WARN("bad device ID: %04X\n", wDevID);
        return MMSYSERR_BADDEVICEID;
    }

    if ((mixer = open(mix->dev_name, O_RDWR)) < 0)
    {
	ERR("open(%s, O_RDWR) failed (%s)\n",
            mix->dev_name, strerror(errno));

	if (errno == ENODEV || errno == ENXIO)
	{
	    /* no driver present */
            WARN("no driver\n");
	    return MMSYSERR_NODRIVER;
	}
	return MMSYSERR_ERROR;
    }

    if (ioctl(mixer, SOUND_MIXER_READ_DEVMASK, &mix->devMask) == -1)
    {
	ERR("ioctl(%s, SOUND_MIXER_DEVMASK) failed (%s)\n",
            mix->dev_name, strerror(errno));
	ret = MMSYSERR_ERROR;
	goto error;
    }

    mix->devMask &= WINE_MIXER_MASK_SPEAKER;
    if (mix->devMask == 0)
    {
        WARN("no driver\n");
	ret = MMSYSERR_NODRIVER;
	goto error;
    }

    if (ioctl(mixer, SOUND_MIXER_READ_STEREODEVS, &mix->stereoMask) == -1)
    {
	ERR("ioctl(%s, SOUND_MIXER_STEREODEVS) failed (%s)\n",
            mix->dev_name, strerror(errno));
	ret = MMSYSERR_ERROR;
	goto error;
    }
    mix->stereoMask &= WINE_MIXER_MASK_SPEAKER;

    if (ioctl(mixer, SOUND_MIXER_READ_RECMASK, &mix->recMask) == -1)
    {
	ERR("ioctl(%s, SOUND_MIXER_RECMASK) failed (%s)\n",
            mix->dev_name, strerror(errno));
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
	ERR("ioctl(%s, SOUND_MIXER_READ_CAPS) failed (%s)\n",
            mix->dev_name, strerror(errno));
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
    if (!(mix->ctrl = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                sizeof(mix->ctrl[0]) * mix->numCtrl)))
    {
	ret = MMSYSERR_NOMEM;
	goto error;
    }

    j = 0;
    MIX_FillLineControls(mix, j++, MAKELONG(0, LINEID_DST),
                         MIXERCONTROL_CONTROLTYPE_VOLUME);
    MIX_FillLineControls(mix, j++, MAKELONG(0, LINEID_DST),
                         MIXERCONTROL_CONTROLTYPE_MUTE);
    MIX_FillLineControls(mix, j++, MAKELONG(1, LINEID_DST),
			 mix->singleRecChannel ?
			     MIXERCONTROL_CONTROLTYPE_MUX :
                             MIXERCONTROL_CONTROLTYPE_MIXER);
    MIX_FillLineControls(mix, j++, MAKELONG(1, LINEID_DST),
                         MIXERCONTROL_CONTROLTYPE_MUTE/*EPP*/);
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

    TRACE("(%p, %s, %p\n", mix, getIoctlCommand(chn), val);

    if ((mixer = open(mix->dev_name, O_RDWR)) < 0) {
	/* FIXME: ENXIO => no mixer installed */
	WARN("mixer device not available !\n");
    } else {
	if (ioctl(mixer, MIXER_READ(chn), val) >= 0) {
	    TRACE("Reading %04x for %s\n", *val, getIoctlCommand(chn));
	    ret = TRUE;
	} else {
            ERR("ioctl(%s, MIXER_READ(%s)) failed (%s)\n",
                mix->dev_name, getIoctlCommand(chn), strerror(errno));
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

    TRACE("(%p, %s, %x)\n", mix, getIoctlCommand(chn), val);

    if ((mixer = open(mix->dev_name, O_RDWR)) < 0) {
	/* FIXME: ENXIO => no mixer installed */
	WARN("mixer device not available !\n");
    } else {
	if (ioctl(mixer, MIXER_WRITE(chn), &val) >= 0) {
	    TRACE("Set %s to %04x\n", getIoctlCommand(chn), val);
	    ret = TRUE;
	} else {
            ERR("ioctl(%s, MIXER_WRITE(%s)) failed (%s)\n",
                mix->dev_name, getIoctlCommand(chn), strerror(errno));
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

    TRACE("(%p, %p)\n", mix, mask);

    if ((mixer = open(mix->dev_name, O_RDWR)) >= 0) {
	if (ioctl(mixer, SOUND_MIXER_READ_RECSRC, &mask) >= 0) {
            ret = TRUE;
	} else {
            ERR("ioctl(%s, SOUND_MIXER_READ_RECSRC) failed (%s)\n",
                mix->dev_name, strerror(errno));
        }
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

    TRACE("(%p, %08x)\n", mix, mask);

    if ((mixer = open(mix->dev_name, O_RDWR)) >= 0) {
	if (ioctl(mixer, SOUND_MIXER_WRITE_RECSRC, &mask) >= 0) {
	    ret = TRUE;
	} else {
            ERR("ioctl(%s, SOUND_MIXER_WRITE_RECSRC) failed (%s)\n",
                mix->dev_name, strerror(errno));
        }
	close(mixer);
    }
    return ret;
}

/**************************************************************************
 * 				MIX_GetDevCaps			[internal]
 */
static DWORD MIX_GetDevCaps(WORD wDevID, LPMIXERCAPSW lpCaps, DWORD dwSize)
{
    struct mixer*	mix;
    MIXERCAPSW		capsW;
    const char*         name;

    TRACE("(%04X, %p, %u);\n", wDevID, lpCaps, dwSize);

    if (lpCaps == NULL) {
        WARN("invalid parameter: lpCaps == NULL\n");
        return MMSYSERR_INVALPARAM;
    }

    if ((mix = MIX_Get(wDevID)) == NULL) {
        WARN("bad device ID: %04X\n", wDevID);
        return MMSYSERR_BADDEVICEID;
    }

    capsW.wMid = WINE_MIXER_MANUF_ID;
    capsW.wPid = WINE_MIXER_PRODUCT_ID;
    capsW.vDriverVersion = WINE_MIXER_VERSION;
    if (!(name = mix->name)) name = WINE_MIXER_NAME;
    MultiByteToWideChar(CP_ACP, 0, name, -1, capsW.szPname, sizeof(capsW.szPname) / sizeof(WCHAR));
    capsW.cDestinations = 2; /* speakers & record */
    capsW.fdwSupport = 0; /* No bits defined yet */

    memcpy(lpCaps, &capsW, min(dwSize, sizeof(capsW)));

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				MIX_GetLineInfoDst	[internal]
 */
static	DWORD	MIX_GetLineInfoDst(struct mixer* mix, LPMIXERLINEW lpMl,
                                   DWORD dst)
{
    unsigned mask;
    int	j;

    TRACE("(%p, %p, %08x)\n", mix, lpMl, dst);

    lpMl->dwDestination = dst;
    switch (dst)
    {
    case LINEID_SPEAKER:
	lpMl->dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
	mask = mix->devMask;
	j = SOUND_MIXER_VOLUME;
        lpMl->Target.dwType = MIXERLINE_TARGETTYPE_WAVEOUT;
	break;
    case LINEID_RECORD:
	lpMl->dwComponentType = MIXERLINE_COMPONENTTYPE_DST_WAVEIN;
	mask = mix->recMask;
	j = SOUND_MIXER_RECLEV;
        lpMl->Target.dwType = MIXERLINE_TARGETTYPE_WAVEIN;
	break;
    default:
	FIXME("shouldn't happen\n");
	return MMSYSERR_ERROR;
    }
    lpMl->dwSource = 0xFFFFFFFF;
    MultiByteToWideChar(CP_ACP, 0, MIX_Labels[j], -1, lpMl->szShortName, sizeof(lpMl->szShortName) / sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, MIX_Names[j],  -1, lpMl->szName,      sizeof(lpMl->szName) / sizeof(WCHAR));

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
static	DWORD	MIX_GetLineInfoSrc(struct mixer* mix, LPMIXERLINEW lpMl,
                                   DWORD idx, DWORD dst)
{
    int		i, j;
    unsigned	mask = (dst) ? mix->recMask : mix->devMask;

    TRACE("(%p, %p, %d, %08x)\n", mix, lpMl, idx, dst);

    MultiByteToWideChar(CP_ACP, 0, MIX_Labels[idx], -1, lpMl->szShortName, sizeof(lpMl->szShortName) / sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, MIX_Names[idx],  -1, lpMl->szName,      sizeof(lpMl->szName) / sizeof(WCHAR));
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
        lpMl->Target.dwType = MIXERLINE_TARGETTYPE_MIDIOUT;
	break;
    case SOUND_MIXER_CD:
	lpMl->dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC;
	lpMl->fdwLine	 |= MIXERLINE_LINEF_SOURCE;
        lpMl->Target.dwType = MIXERLINE_TARGETTYPE_UNDEFINED;
	break;
    case SOUND_MIXER_LINE:
	lpMl->dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_LINE;
	lpMl->fdwLine	 |= MIXERLINE_LINEF_SOURCE;
        lpMl->Target.dwType = MIXERLINE_TARGETTYPE_UNDEFINED;
	break;
    case SOUND_MIXER_MIC:
	lpMl->dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE;
	lpMl->fdwLine	 |= MIXERLINE_LINEF_SOURCE;
        lpMl->Target.dwType = MIXERLINE_TARGETTYPE_WAVEIN;
	break;
    case SOUND_MIXER_PCM:
	lpMl->dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT;
	lpMl->fdwLine	 |= MIXERLINE_LINEF_SOURCE;
        lpMl->Target.dwType = MIXERLINE_TARGETTYPE_WAVEOUT;
	break;
    case SOUND_MIXER_IMIX:
	lpMl->dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_UNDEFINED;
	lpMl->fdwLine	 |= MIXERLINE_LINEF_SOURCE;
        lpMl->Target.dwType = MIXERLINE_TARGETTYPE_UNDEFINED;
	break;
    default:
	WARN("Index %d not handled.\n", idx);
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
    TRACE("(%08x)\n",lineID);

    return ((HIWORD(lineID) < SOUND_MIXER_NRDEVICES && LOWORD(lineID) < 2) ||
	    (HIWORD(lineID) == LINEID_DST &&
             LOWORD(lineID) < SOUND_MIXER_NRDEVICES));
}

/**************************************************************************
 * 				MIX_GetLineInfo			[internal]
 */
static DWORD MIX_GetLineInfo(WORD wDevID, LPMIXERLINEW lpMl, DWORD fdwInfo)
{
    int 		i, j;
    DWORD		ret = MMSYSERR_NOERROR;
    unsigned		mask;
    struct mixer*	mix;

    TRACE("(%04X, %p, %u);\n", wDevID, lpMl, fdwInfo);

    if (lpMl == NULL) {
        WARN("invalid parameter: lpMl = NULL\n");
	return MMSYSERR_INVALPARAM;
    }

    if (lpMl->cbStruct != sizeof(*lpMl)) {
        WARN("invalid parameter: lpMl->cbStruct = %d != %d\n",
             lpMl->cbStruct, sizeof(*lpMl));
	return MMSYSERR_INVALPARAM;
    }

    if ((mix = MIX_Get(wDevID)) == NULL) {
        WARN("bad device ID: %04X\n", wDevID);
        return MMSYSERR_BADDEVICEID;
    }

    /* FIXME: set all the variables correctly... the lines below
     * are very wrong...
     */
    lpMl->fdwLine	= MIXERLINE_LINEF_ACTIVE;
    lpMl->dwUser	= 0;

    switch (fdwInfo & MIXER_GETLINEINFOF_QUERYMASK)
    {
    case MIXER_GETLINEINFOF_DESTINATION:
	TRACE("MIXER_GETLINEINFOF_DESTINATION (%08x)\n", lpMl->dwDestination);
	if (lpMl->dwDestination >= 2) {
            WARN("invalid parameter: lpMl->dwDestination = %d >= 2\n",
                 lpMl->dwDestination);
	    return MMSYSERR_INVALPARAM;
        }
	ret = MIX_GetLineInfoDst(mix, lpMl, lpMl->dwDestination);
	if (ret != MMSYSERR_NOERROR) {
            WARN("error\n");
	    return ret;
        }
	break;
    case MIXER_GETLINEINFOF_SOURCE:
	TRACE("MIXER_GETLINEINFOF_SOURCE (%08x), dst=%08x\n", lpMl->dwSource,
              lpMl->dwDestination);
	switch (lpMl->dwDestination)
	{
	case LINEID_SPEAKER: mask = mix->devMask; break;
	case LINEID_RECORD: mask = mix->recMask; break;
	default:
            WARN("invalid parameter\n");
            return MMSYSERR_INVALPARAM;
	}
	i = lpMl->dwSource;
	for (j = 0; j < SOUND_MIXER_NRDEVICES; j++)
	{
	    if (WINE_CHN_SUPPORTS(mask, j) && (i-- == 0))
		break;
	}
	if (j >= SOUND_MIXER_NRDEVICES) {
            WARN("invalid line\n");
	    return MIXERR_INVALLINE;
        }
	ret = MIX_GetLineInfoSrc(mix, lpMl, j, lpMl->dwDestination);
	if (ret != MMSYSERR_NOERROR) {
            WARN("error\n");
	    return ret;
        }
	break;
    case MIXER_GETLINEINFOF_LINEID:
	TRACE("MIXER_GETLINEINFOF_LINEID (%08x)\n", lpMl->dwLineID);

	if (!MIX_CheckLine(lpMl->dwLineID)) {
            WARN("invalid line\n");
	    return MIXERR_INVALLINE;
        }
	if (HIWORD(lpMl->dwLineID) == LINEID_DST)
	    ret = MIX_GetLineInfoDst(mix, lpMl, LOWORD(lpMl->dwLineID));
	else
	    ret = MIX_GetLineInfoSrc(mix, lpMl, HIWORD(lpMl->dwLineID),
                                     LOWORD(lpMl->dwLineID));
	if (ret != MMSYSERR_NOERROR) {
            WARN("error\n");
	    return ret;
        }
	break;
    case MIXER_GETLINEINFOF_COMPONENTTYPE:
	TRACE("MIXER_GETLINEINFOF_COMPONENTTYPE (%s)\n",
              getComponentType(lpMl->dwComponentType));
	switch (lpMl->dwComponentType)
	{
	case MIXERLINE_COMPONENTTYPE_DST_HEADPHONES:
	case MIXERLINE_COMPONENTTYPE_DST_SPEAKERS:
	    ret = MIX_GetLineInfoDst(mix, lpMl, LINEID_SPEAKER);
	    break;
	case MIXERLINE_COMPONENTTYPE_DST_LINE:
	case MIXERLINE_COMPONENTTYPE_DST_VOICEIN:
	case MIXERLINE_COMPONENTTYPE_DST_WAVEIN:
	    ret = MIX_GetLineInfoDst(mix, lpMl, LINEID_RECORD);
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
	    FIXME("Unhandled component type (%s)\n",
                  getComponentType(lpMl->dwComponentType));
	    return MMSYSERR_INVALPARAM;
	}
	break;
    case MIXER_GETLINEINFOF_TARGETTYPE:
	FIXME("MIXER_GETLINEINFOF_TARGETTYPE not implemented yet.\n");
        TRACE("MIXER_GETLINEINFOF_TARGETTYPE (%s)\n",
              getTargetType(lpMl->Target.dwType));
        switch (lpMl->Target.dwType) {
        case MIXERLINE_TARGETTYPE_UNDEFINED:
        case MIXERLINE_TARGETTYPE_WAVEOUT:
        case MIXERLINE_TARGETTYPE_WAVEIN:
        case MIXERLINE_TARGETTYPE_MIDIOUT:
        case MIXERLINE_TARGETTYPE_MIDIIN:
        case MIXERLINE_TARGETTYPE_AUX:
	default:
	    FIXME("Unhandled target type (%s)\n",
                  getTargetType(lpMl->Target.dwType));
	    return MMSYSERR_INVALPARAM;
        }
	break;
    default:
	WARN("Unknown flag (%08lx)\n", fdwInfo & MIXER_GETLINEINFOF_QUERYMASK);
	break;
    }

    if ((fdwInfo & MIXER_GETLINEINFOF_QUERYMASK) != MIXER_GETLINEINFOF_TARGETTYPE) {
        const char* name;
        lpMl->Target.dwDeviceID = 0xFFFFFFFF;
        lpMl->Target.wMid = WINE_MIXER_MANUF_ID;
        lpMl->Target.wPid = WINE_MIXER_PRODUCT_ID;
        lpMl->Target.vDriverVersion = WINE_MIXER_VERSION;
        if (!(name = mix->name)) name = WINE_MIXER_NAME;
        MultiByteToWideChar(CP_ACP, 0, name, -1, lpMl->Target.szPname, sizeof(lpMl->Target.szPname) / sizeof(WCHAR));
    }

    return ret;
}

/******************************************************************
 *		MIX_CheckControl
 *
 */
static BOOL	MIX_CheckControl(struct mixer* mix, DWORD ctrlID)
{
    TRACE("(%p, %08x)\n", mix, ctrlID);

    return (ctrlID >= 1 && ctrlID <= mix->numCtrl);
}

/**************************************************************************
 * 				MIX_GetLineControls		[internal]
 */
static	DWORD	MIX_GetLineControls(WORD wDevID, LPMIXERLINECONTROLSW lpMlc,
                                    DWORD flags)
{
    DWORD		dwRet = MMSYSERR_NOERROR;
    struct mixer*	mix;

    TRACE("(%04X, %p, %u);\n", wDevID, lpMlc, flags);

    if (lpMlc == NULL) {
        WARN("invalid parameter: lpMlc == NULL\n");
        return MMSYSERR_INVALPARAM;
    }

    if (lpMlc->cbStruct < sizeof(*lpMlc)) {
        WARN("invalid parameter: lpMlc->cbStruct = %d < %d\n",
             lpMlc->cbStruct, sizeof(*lpMlc));
	return MMSYSERR_INVALPARAM;
    }

    if (lpMlc->cbmxctrl < sizeof(MIXERCONTROLW)) {
        WARN("invalid parameter: lpMlc->cbmxctrl = %d < %d\n",
             lpMlc->cbmxctrl, sizeof(MIXERCONTROLW));
	return MMSYSERR_INVALPARAM;
    }

    if ((mix = MIX_Get(wDevID)) == NULL) {
        WARN("bad device ID: %04X\n", wDevID);
        return MMSYSERR_BADDEVICEID;
    }

    switch (flags & MIXER_GETLINECONTROLSF_QUERYMASK)
    {
    case MIXER_GETLINECONTROLSF_ALL:
        {
	    int		i, j;

            TRACE("line=%08x MIXER_GETLINECONTROLSF_ALL (%d)\n",
                  lpMlc->dwLineID, lpMlc->cControls);

	    for (i = j = 0; i < mix->numCtrl; i++)
	    {
		if (mix->ctrl[i].dwLineID == lpMlc->dwLineID)
		    j++;
	    }

	    if (!j || lpMlc->cControls != j) {
                WARN("invalid parameter\n");
                dwRet = MMSYSERR_INVALPARAM;
	    } else if (!MIX_CheckLine(lpMlc->dwLineID)) {
                WARN("invalid line\n");
                dwRet = MIXERR_INVALLINE;
	    } else {
		for (i = j = 0; i < mix->numCtrl; i++)
		{
		    if (mix->ctrl[i].dwLineID == lpMlc->dwLineID)
		    {
			TRACE("[%d] => [%2d]: typ=%08x\n", j, i + 1,
                              mix->ctrl[i].ctrl.dwControlType);
			lpMlc->pamxctrl[j++] = mix->ctrl[i].ctrl;
                    }
		}
	    }
	}
	break;
    case MIXER_GETLINECONTROLSF_ONEBYID:
	TRACE("line=%08x MIXER_GETLINECONTROLSF_ONEBYID (%x)\n",
              lpMlc->dwLineID, lpMlc->u.dwControlID);

	if (!MIX_CheckControl(mix, lpMlc->u.dwControlID) ||
	    mix->ctrl[lpMlc->u.dwControlID - 1].dwLineID != lpMlc->dwLineID) {
            WARN("invalid parameter\n");
	    dwRet = MMSYSERR_INVALPARAM;
	} else
	    lpMlc->pamxctrl[0] = mix->ctrl[lpMlc->u.dwControlID - 1].ctrl;
	break;
    case MIXER_GETLINECONTROLSF_ONEBYTYPE:
	TRACE("line=%08x MIXER_GETLINECONTROLSF_ONEBYTYPE (%s)\n",
              lpMlc->dwLineID, getControlType(lpMlc->u.dwControlType));
	if (!MIX_CheckLine(lpMlc->dwLineID)) {
            WARN("invalid line\n");
            dwRet = MIXERR_INVALLINE;
	} else {
	    int	  i;
	    DWORD ct = lpMlc->u.dwControlType & MIXERCONTROL_CT_CLASS_MASK;
	    for (i = 0; i < mix->numCtrl; i++) {
		if (mix->ctrl[i].dwLineID == lpMlc->dwLineID &&
		    ct == (mix->ctrl[i].ctrl.dwControlType &
                    MIXERCONTROL_CT_CLASS_MASK)) {
		    lpMlc->pamxctrl[0] = mix->ctrl[i].ctrl;
		    break;
		}
	    }

	    if (i == mix->numCtrl) {
                WARN("invalid parameter: control not found\n");
                dwRet = MMSYSERR_INVALPARAM;
            }
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
static	DWORD	MIX_GetControlDetails(WORD wDevID, LPMIXERCONTROLDETAILS lpmcd,
                                      DWORD fdwDetails)
{
    DWORD		ret = MMSYSERR_NOTSUPPORTED;
    DWORD		c, chnl;
    struct mixer*	mix;

    TRACE("(%04X, %p, %u);\n", wDevID, lpmcd, fdwDetails);

    if (lpmcd == NULL) {
        WARN("invalid parameter: lpmcd == NULL\n");
        return MMSYSERR_INVALPARAM;
    }

    if ((mix = MIX_Get(wDevID)) == NULL) {
        WARN("bad device ID: %04X\n", wDevID);
        return MMSYSERR_BADDEVICEID;
    }

    switch (fdwDetails & MIXER_GETCONTROLDETAILSF_QUERYMASK)
    {
    case MIXER_GETCONTROLDETAILSF_VALUE:
	TRACE("MIXER_GETCONTROLDETAILSF_VALUE (%08x)\n", lpmcd->dwControlID);
	if (MIX_CheckControl(mix, lpmcd->dwControlID))
	{
	    c = lpmcd->dwControlID - 1;
	    chnl = HIWORD(mix->ctrl[c].dwLineID);
	    if (chnl == LINEID_DST)
		chnl = LOWORD(mix->ctrl[c].dwLineID) ? SOUND_MIXER_RECLEV :
                    SOUND_MIXER_VOLUME;
	    switch (mix->ctrl[c].ctrl.dwControlType)
	    {
	    case MIXERCONTROL_CONTROLTYPE_VOLUME:
		{
		    LPMIXERCONTROLDETAILS_UNSIGNED	mcdu;
		    int					val;

                    if (lpmcd->cbDetails !=
                        sizeof(MIXERCONTROLDETAILS_UNSIGNED)) {
                        WARN("invalid parameter: cbDetails != %d\n",
                             sizeof(MIXERCONTROLDETAILS_UNSIGNED));
                        return MMSYSERR_INVALPARAM;
                    }

                    TRACE("%s MIXERCONTROLDETAILS_UNSIGNED[%u]\n",
                          getControlType(mix->ctrl[c].ctrl.dwControlType),
                          lpmcd->cChannels);

                    mcdu = (LPMIXERCONTROLDETAILS_UNSIGNED)lpmcd->paDetails;

		    /* return value is 00RL (4 bytes)... */
		    if ((val = mix->volume[chnl]) == -1 &&
                        !MIX_GetVal(mix, chnl, &val)) {
                        WARN("invalid parameter\n");
			return MMSYSERR_INVALPARAM;
                    }

		    switch (lpmcd->cChannels)
		    {
		    case 1:
			/* mono... so R = L */
			mcdu->dwValue = ((LOBYTE(LOWORD(val)) * 65536.0) / 100.0) + 0.5;
			TRACE("Reading RL = %d\n", mcdu->dwValue);
			break;
		    case 2:
			/* stereo, left is paDetails[0] */
			mcdu->dwValue = ((LOBYTE(LOWORD(val)) * 65536.0) / 100.0) + 0.5;
			TRACE("Reading L = %d\n", mcdu->dwValue);
                        mcdu++;
			mcdu->dwValue = ((HIBYTE(LOWORD(val)) * 65536.0) / 100.0) + 0.5;
			TRACE("Reading R = %d\n", mcdu->dwValue);
			break;
		    default:
			WARN("Unsupported cChannels (%d)\n", lpmcd->cChannels);
			return MMSYSERR_INVALPARAM;
		    }
                    TRACE("=> %08x\n", mcdu->dwValue);
		}
		break;
	    case MIXERCONTROL_CONTROLTYPE_MUTE:
	    case MIXERCONTROL_CONTROLTYPE_ONOFF:
		{
		    LPMIXERCONTROLDETAILS_BOOLEAN	mcdb;

                    if (lpmcd->cbDetails !=
                        sizeof(MIXERCONTROLDETAILS_BOOLEAN)) {
                        WARN("invalid parameter: cbDetails != %d\n",
                             sizeof(MIXERCONTROLDETAILS_BOOLEAN));
                        return MMSYSERR_INVALPARAM;
                    }

                    TRACE("%s MIXERCONTROLDETAILS_BOOLEAN[%u]\n",
                          getControlType(mix->ctrl[c].ctrl.dwControlType),
                          lpmcd->cChannels);

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

                    if (lpmcd->cbDetails !=
                        sizeof(MIXERCONTROLDETAILS_BOOLEAN)) {
                        WARN("invalid parameter: cbDetails != %d\n",
                             sizeof(MIXERCONTROLDETAILS_BOOLEAN));
                        return MMSYSERR_INVALPARAM;
                    }

                    TRACE("%s MIXERCONTROLDETAILS_BOOLEAN[%u]\n",
                          getControlType(mix->ctrl[c].ctrl.dwControlType),
                          lpmcd->cChannels);

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
		WARN("%s Unsupported\n",
                     getControlType(mix->ctrl[c].ctrl.dwControlType));
	    }
	    ret = MMSYSERR_NOERROR;
	}
	else
	{
            WARN("invalid parameter\n");
	    ret = MMSYSERR_INVALPARAM;
	}
	break;
    case MIXER_GETCONTROLDETAILSF_LISTTEXT:
	TRACE("MIXER_GETCONTROLDETAILSF_LISTTEXT (%08x)\n",
              lpmcd->dwControlID);

	ret = MMSYSERR_INVALPARAM;
	if (MIX_CheckControl(mix, lpmcd->dwControlID))
	{
	    int	c = lpmcd->dwControlID - 1;

	    if (mix->ctrl[c].ctrl.dwControlType == MIXERCONTROL_CONTROLTYPE_MUX ||
		mix->ctrl[c].ctrl.dwControlType == MIXERCONTROL_CONTROLTYPE_MIXER)
	    {
		LPMIXERCONTROLDETAILS_LISTTEXTW	mcdlt;
		int i, j;

		mcdlt = (LPMIXERCONTROLDETAILS_LISTTEXTW)lpmcd->paDetails;
		for (i = j = 0; j < SOUND_MIXER_NRDEVICES; j++)
		{
		    if (WINE_CHN_SUPPORTS(mix->recMask, j))
		    {
			mcdlt[i].dwParam1 = MAKELONG(LINEID_RECORD, j);
			mcdlt[i].dwParam2 = 0;
			MultiByteToWideChar(CP_ACP, 0, MIX_Names[j], -1, 
                                            mcdlt[i].szName, sizeof(mcdlt[i]) / sizeof(WCHAR));
			i++;
		    }
		}
		if (i != lpmcd->u.cMultipleItems) FIXME("bad count\n");
		ret = MMSYSERR_NOERROR;
	    }
	}
	break;
    default:
	WARN("Unknown flag (%08lx)\n",
             fdwDetails & MIXER_GETCONTROLDETAILSF_QUERYMASK);
    }
    return ret;
}

/**************************************************************************
 * 				MIX_SetControlDetails		[internal]
 */
static	DWORD	MIX_SetControlDetails(WORD wDevID, LPMIXERCONTROLDETAILS lpmcd,
                                      DWORD fdwDetails)
{
    DWORD		ret = MMSYSERR_NOTSUPPORTED;
    DWORD		c, chnl;
    int			val;
    struct mixer*	mix;

    TRACE("(%04X, %p, %u);\n", wDevID, lpmcd, fdwDetails);

    if (lpmcd == NULL) {
        TRACE("invalid parameter: lpmcd == NULL\n");
        return MMSYSERR_INVALPARAM;
    }

    if ((mix = MIX_Get(wDevID)) == NULL) {
        WARN("bad device ID: %04X\n", wDevID);
        return MMSYSERR_BADDEVICEID;
    }

    switch (fdwDetails & MIXER_GETCONTROLDETAILSF_QUERYMASK)
    {
    case MIXER_GETCONTROLDETAILSF_VALUE:
	TRACE("MIXER_GETCONTROLDETAILSF_VALUE (%08x)\n", lpmcd->dwControlID);
	if (MIX_CheckControl(mix, lpmcd->dwControlID))
	{
	    c = lpmcd->dwControlID - 1;

            TRACE("dwLineID=%08x\n",mix->ctrl[c].dwLineID);

	    chnl = HIWORD(mix->ctrl[c].dwLineID);
	    if (chnl == LINEID_DST)
		chnl = LOWORD(mix->ctrl[c].dwLineID) ?
                    SOUND_MIXER_RECLEV : SOUND_MIXER_VOLUME;

	    switch (mix->ctrl[c].ctrl.dwControlType)
	    {
	    case MIXERCONTROL_CONTROLTYPE_VOLUME:
		{
		    LPMIXERCONTROLDETAILS_UNSIGNED	mcdu;

                    if (lpmcd->cbDetails !=
                        sizeof(MIXERCONTROLDETAILS_UNSIGNED)) {
                        WARN("invalid parameter: cbDetails != %d\n",
                             sizeof(MIXERCONTROLDETAILS_UNSIGNED));
                        return MMSYSERR_INVALPARAM;
                    }

                    TRACE("%s MIXERCONTROLDETAILS_UNSIGNED[%u]\n",
                          getControlType(mix->ctrl[c].ctrl.dwControlType),
                          lpmcd->cChannels);

                    mcdu = (LPMIXERCONTROLDETAILS_UNSIGNED)lpmcd->paDetails;
		    /* val should contain 00RL */
		    switch (lpmcd->cChannels)
		    {
		    case 1:
			/* mono... so R = L */
			TRACE("Setting RL to %d\n", mcdu->dwValue);
			val = 0x101 * ((mcdu->dwValue * 100) >> 16);
			break;
		    case 2:
			/* stereo, left is paDetails[0] */
			TRACE("Setting L to %d\n", mcdu->dwValue);
			val = ((mcdu->dwValue * 100.0) / 65536.0) + 0.5;
                        mcdu++;
			TRACE("Setting R to %d\n", mcdu->dwValue);
			val += (int)(((mcdu->dwValue * 100) / 65536.0) + 0.5) << 8;
			break;
		    default:
			WARN("Unsupported cChannels (%d)\n", lpmcd->cChannels);
			return MMSYSERR_INVALPARAM;
		    }

		    if (mix->volume[chnl] == -1)
		    {
			if (!MIX_SetVal(mix, chnl, val)) {
                            WARN("invalid parameter\n");
			    return MMSYSERR_INVALPARAM;
                        }
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

                    if (lpmcd->cbDetails !=
                        sizeof(MIXERCONTROLDETAILS_BOOLEAN)) {
                        WARN("invalid parameter: cbDetails != %d\n",
                             sizeof(MIXERCONTROLDETAILS_BOOLEAN));
                        return MMSYSERR_INVALPARAM;
                    }

                    TRACE("%s MIXERCONTROLDETAILS_BOOLEAN[%u]\n",
                          getControlType(mix->ctrl[c].ctrl.dwControlType),
                          lpmcd->cChannels);

		    mcdb = (LPMIXERCONTROLDETAILS_BOOLEAN)lpmcd->paDetails;
		    if (mcdb->fValue)
		    {
                        /* save the volume and then set it to 0 */
			if (!MIX_GetVal(mix, chnl, &mix->volume[chnl]) ||
                            !MIX_SetVal(mix, chnl, 0)) {
                            WARN("invalid parameter\n");
			    return MMSYSERR_INVALPARAM;
                        }
		    }
		    else
		    {
			if (mix->volume[chnl] == -1)
			{
			    ret = MMSYSERR_NOERROR;
			    break;
			}
			if (!MIX_SetVal(mix, chnl, mix->volume[chnl])) {
                            WARN("invalid parameter\n");
			    return MMSYSERR_INVALPARAM;
                        }
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

                    if (lpmcd->cbDetails !=
                        sizeof(MIXERCONTROLDETAILS_BOOLEAN)) {
                        WARN("invalid parameter: cbDetails != %d\n",
                             sizeof(MIXERCONTROLDETAILS_BOOLEAN));
                        return MMSYSERR_INVALPARAM;
                    }

                    TRACE("%s MIXERCONTROLDETAILS_BOOLEAN[%u]\n",
                          getControlType(mix->ctrl[c].ctrl.dwControlType),
                          lpmcd->cChannels);

		    /* we mute both channels at the same time */
		    mcdb = (LPMIXERCONTROLDETAILS_BOOLEAN)lpmcd->paDetails;
		    mask = 0;
		    for (i = j = 0; j < SOUND_MIXER_NRDEVICES; j++)
		    {
			if (WINE_CHN_SUPPORTS(mix->recMask, j) &&
                            mcdb[i++].fValue)
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
		    if (i != lpmcd->u.cMultipleItems)
                        FIXME("bad count\n");
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
	WARN("Unknown SetControlDetails flag (%08lx)\n",
             fdwDetails & MIXER_GETCONTROLDETAILSF_QUERYMASK);
    }
    return ret;
}

/**************************************************************************
 * 				MIX_Init			[internal]
 */
LRESULT OSS_MixerInit(void)
{
    int	i, mixer;

    TRACE("()\n");

    MIX_NumMixers = 0;

    for (i = 0; i < MAX_MIXERDRV; i++) {
        char name[32];

        if (i == 0)
            sprintf(name, "/dev/mixer");
        else
            sprintf(name, "/dev/mixer%d", i);

        if ((mixer = open(name, O_RDWR)) >= 0) {
#ifdef SOUND_MIXER_INFO
            mixer_info info;
            if (ioctl(mixer, SOUND_MIXER_INFO, &info) >= 0) {
                MIX_Mixers[MIX_NumMixers].name = HeapAlloc(GetProcessHeap(),0,strlen(info.name) + 1);
                strcpy(MIX_Mixers[MIX_NumMixers].name, info.name);
            } else {
                /* FreeBSD up to at least 5.2 provides this ioctl, but does not
                 * implement it properly, and there are probably similar issues
                 * on other platforms, so we warn but try to go ahead.
                 */
                WARN("%s: cannot read SOUND_MIXER_INFO!\n", name);
            }
#endif
            close(mixer);

            MIX_Mixers[MIX_NumMixers].dev_name = HeapAlloc(GetProcessHeap(),0,strlen(name) + 1);
            strcpy(MIX_Mixers[MIX_NumMixers].dev_name, name);
            MIX_NumMixers++;
            MIX_Open(MIX_NumMixers - 1, NULL, 0); /* FIXME */
        } else {
            WARN("couldn't open %s\n", name);
        }
    }

    if (MIX_NumMixers == 0) {
        WARN("no driver\n");
        return MMSYSERR_NODRIVER;
    }

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				MIX_Exit			[internal]
 */
LRESULT OSS_MixerExit(void)
{
    int	i;

    TRACE("()\n");

    for (i = 0; i < MIX_NumMixers; i++) {
        HeapFree(GetProcessHeap(),0,MIX_Mixers[i].name);
        HeapFree(GetProcessHeap(),0,MIX_Mixers[i].dev_name);
    }

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				MIX_GetNumDevs			[internal]
 */
static	DWORD	MIX_GetNumDevs(void)
{
    TRACE("()\n");

    return MIX_NumMixers;
}

#endif /* HAVE_OSS */

/**************************************************************************
 * 				mxdMessage (WINEOSS.3)
 */
DWORD WINAPI OSS_mxdMessage(UINT wDevID, UINT wMsg, DWORD dwUser,
			    DWORD dwParam1, DWORD dwParam2)
{
#ifdef HAVE_OSS
    TRACE("(%04X, %s, %08X, %08X, %08X);\n", wDevID, getMessage(wMsg),
          dwUser, dwParam1, dwParam2);

    switch (wMsg)
    {
    case DRVM_INIT:
    case DRVM_EXIT:
    case DRVM_ENABLE:
    case DRVM_DISABLE:
	/* FIXME: Pretend this is supported */
	return 0;
    case MXDM_GETDEVCAPS:
	return MIX_GetDevCaps(wDevID, (LPMIXERCAPSW)dwParam1, dwParam2);
    case MXDM_GETLINEINFO:
	return MIX_GetLineInfo(wDevID, (LPMIXERLINEW)dwParam1, dwParam2);
    case MXDM_GETNUMDEVS:
	return MIX_GetNumDevs();
    case MXDM_OPEN:
	return MMSYSERR_NOERROR;
	/* MIX_Open(wDevID, (LPMIXEROPENDESC)dwParam1, dwParam2); */
    case MXDM_CLOSE:
	return MMSYSERR_NOERROR;
    case MXDM_GETLINECONTROLS:
	return MIX_GetLineControls(wDevID, (LPMIXERLINECONTROLSW)dwParam1, dwParam2);
    case MXDM_GETCONTROLDETAILS:
	return MIX_GetControlDetails(wDevID, (LPMIXERCONTROLDETAILS)dwParam1, dwParam2);
    case MXDM_SETCONTROLDETAILS:
	return MIX_SetControlDetails(wDevID, (LPMIXERCONTROLDETAILS)dwParam1, dwParam2);
    default:
	WARN("unknown message %d!\n", wMsg);
	return MMSYSERR_NOTSUPPORTED;
    }
#else
    TRACE("(%04X, %04X, %08X, %08X, %08X);\n", wDevID, wMsg,
          dwUser, dwParam1, dwParam2);

    return MMSYSERR_NOTENABLED;
#endif
}
