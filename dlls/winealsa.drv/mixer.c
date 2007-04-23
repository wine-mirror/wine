/*
 * Alsa MIXER Wine Driver for Linux
 * Very loosely based on wineoss mixer driver
 *
 * Copyright 2007 Maarten Lankhorst
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
#include "wingdi.h"
#include "winerror.h"
#include "winuser.h"
#include "winnls.h"
#include "mmddk.h"
#include "mmsystem.h"
#include "alsa.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mixer);

#ifdef HAVE_ALSA

/* Generic notes:
 * In windows it seems to be required for all controls to have a volume switch
 * In alsa that's optional
 *
 * I assume for playback controls, that there is always a playback volume switch available
 * Mute is optional
 *
 * For capture controls, it is needed that there is a capture switch and a volume switch,
 * It doesn't matter wether it is a playback volume switch or a capture volume switch.
 * The code will first try to get/adjust capture volume, if that fails it tries playback volume
 * It is not pretty, but under my 3 test cards it seems that there is no other choice:
 * Most capture controls don't have a capture volume setting
 *
 * MUX means that only capture source can be exclusively selected,
 * MIXER means that multiple sources can be selected simultaneously.
 */

static const char * getMessage(UINT uMsg)
{
    static char str[64];
#define MSG_TO_STR(x) case x: return #x;
    switch (uMsg){
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
    default: break;
    }
#undef MSG_TO_STR
    sprintf(str, "UNKNOWN(%08x)", uMsg);
    return str;
}

/* Mixer device */
typedef struct mixer
{
    snd_mixer_t *mix;
    WCHAR mixername[MAXPNAMELEN];

    int chans, dests;
} mixer;

#define MAX_MIXERS 32

static int cards = 0;
static mixer mixdev[MAX_MIXERS];

/* Is this control suited for showing up? */
static int blacklisted(snd_mixer_elem_t *elem)
{
    const char *name = snd_mixer_selem_get_name(elem);
    BOOL blisted = 0;

    if (!snd_mixer_selem_has_playback_volume(elem) &&
        (!snd_mixer_selem_has_capture_volume(elem) ||
         !snd_mixer_selem_has_capture_switch(elem)))
        blisted = 1;

    TRACE("%s: %x\n", name, blisted);
    return blisted;
}

static void ALSA_MixerInit(void)
{
    int x, mixnum = 0;

    for (x = 0; x < MAX_MIXERS; ++x)
    {
        int card, err;
        char cardind[6], cardname[10];
        BOOL hascapt=0, hasmast=0;

        snd_ctl_t *ctl;
        snd_mixer_elem_t *elem, *mastelem = NULL, *captelem = NULL;
        snd_ctl_card_info_t *info = NULL;
        snd_ctl_card_info_alloca(&info);

        snprintf(cardind, sizeof(cardind), "%d", x);
        card = snd_card_get_index(cardind);
        if (card < 0 || card > MAX_MIXERS - 1)
            continue;
        snprintf(cardname, sizeof(cardname), "hw:%d", card);

        err = snd_ctl_open(&ctl, cardname, 0);
        if (err < 0)
        {
            WARN("Cannot open card: %s\n", snd_strerror(err));
            continue;
        }

        err = snd_ctl_card_info(ctl, info);
        if (err < 0)
        {
            WARN("Cannot get card info: %s\n", snd_strerror(err));
            snd_ctl_close(ctl);
            continue;
        }

        MultiByteToWideChar(CP_UNIXCP, 0, snd_ctl_card_info_get_name(info), -1, mixdev[mixnum].mixername, sizeof(mixdev[mixnum].mixername)/sizeof(WCHAR));
        snd_ctl_close(ctl);

        err = snd_mixer_open(&mixdev[mixnum].mix,0);
        if (err < 0)
        {
            WARN("Error occured opening mixer: %s\n", snd_strerror(err));
            continue;
        }

        err = snd_mixer_attach(mixdev[mixnum].mix, cardname);
        if (err < 0)
            goto eclose;

        err = snd_mixer_selem_register(mixdev[mixnum].mix, NULL, NULL);
        if (err < 0)
            goto eclose;

        err = snd_mixer_load(mixdev[mixnum].mix);
        if (err < 0)
            goto eclose;

        mixdev[mixnum].chans = 0;
        mixdev[mixnum].dests = 1; /* Master, Capture will be enabled if needed */

        for (elem = snd_mixer_first_elem(mixdev[mixnum].mix); elem; elem = snd_mixer_elem_next(elem))
            if (!strcasecmp(snd_mixer_selem_get_name(elem), "Master"))
            {
                mastelem = elem;
                ++hasmast;
            }
            else if (!strcasecmp(snd_mixer_selem_get_name(elem), "Capture"))
            {
                captelem = elem;
                ++hascapt;
            }
            else if (!blacklisted(elem))
            {
                if (snd_mixer_selem_has_capture_switch(elem))
                {
                    ++mixdev[mixnum].chans;
                    mixdev[mixnum].dests = 2;
                }
                if (snd_mixer_selem_has_playback_volume(elem))
                    ++mixdev[mixnum].chans;
            }

        /* If there is only 'Capture' and 'Master', this device is not worth it */
        if (!mixdev[mixnum].chans)
        {
            WARN("No channels found, skipping device!\n");
            snd_mixer_close(mixdev[mixnum].mix);
            continue;
        }

        /* If there are no 'Capture' and 'Master', something is wrong */
        if (hasmast != 1 || hascapt != 1)
        {
            if (hasmast != 1)
                FIXME("Should have found 1 channel for 'Master', but instead found %d\n", hasmast);
            if (hascapt != 1)
                FIXME("Should have found 1 channel for 'Capture', but instead found %d\n", hascapt);
            goto eclose;
        }

        mixdev[mixnum].chans += 2; /* Capture/Master */

        TRACE("%s: Amount of controls: %i/%i, name: %s\n", cardname, mixdev[mixnum].dests, mixdev[mixnum].chans, debugstr_w(mixdev[mixnum].mixername));
        mixnum++;
        continue;

        eclose:
        WARN("Error occured initialising mixer: %s\n", snd_strerror(err));
        snd_mixer_close(mixdev[mixnum].mix);
    }
    cards = mixnum;
    TRACE("\n");
}

static void ALSA_MixerExit(void)
{
    int x;
    TRACE("\n");

    for (x = 0; x < cards; ++x)
        snd_mixer_close(mixdev[x].mix);
    cards = 0;
}

#endif /*HAVE_ALSA*/

/**************************************************************************
 *                        mxdMessage (WINEALSA.3)
 */
DWORD WINAPI ALSA_mxdMessage(UINT wDevID, UINT wMsg, DWORD_PTR dwUser,
                             DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
#ifdef HAVE_ALSA
    DWORD ret;
    TRACE("(%04X, %s, %08lX, %08lX, %08lX);\n", wDevID, getMessage(wMsg),
          dwUser, dwParam1, dwParam2);

    switch (wMsg)
    {
    case DRVM_INIT: ALSA_MixerInit(); ret = MMSYSERR_NOERROR; break;
    case DRVM_EXIT: ALSA_MixerExit(); ret = MMSYSERR_NOERROR; break;
    /* All taken care of by driver initialisation */
    /* Unimplemented, and not needed */
    case DRVM_ENABLE:
    case DRVM_DISABLE:
        ret = MMSYSERR_NOERROR; break;

    case MXDM_GETNUMDEVS:
        ret = cards; break;

    default:
        WARN("unknown message %s!\n", getMessage(wMsg));
        return MMSYSERR_NOTSUPPORTED;
    }

    TRACE("Returning %08X\n", ret);
    return ret;
#else /*HAVE_ALSA*/
    TRACE("(%04X, %04X, %08lX, %08lX, %08lX);\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);

    return MMSYSERR_NOTENABLED;
#endif /*HAVE_ALSA*/
}
