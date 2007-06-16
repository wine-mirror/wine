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
#include "winuser.h"
#include "winnls.h"
#include "mmddk.h"
#include "mmsystem.h"
#include "alsa.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mixer);

#ifdef HAVE_ALSA

#define	WINE_MIXER_MANUF_ID      0xAA
#define	WINE_MIXER_PRODUCT_ID    0x55
#define	WINE_MIXER_VERSION       0x0100

/* Generic notes:
 * In windows it seems to be required for all controls to have a volume switch
 * In alsa that's optional
 *
 * I assume for playback controls, that there is always a playback volume switch available
 * Mute is optional
 *
 * For capture controls, it is needed that there is a capture switch and a volume switch,
 * It doesn't matter whether it is a playback volume switch or a capture volume switch.
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

static const char * getControlType(DWORD dwControlType)
{
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
    return wine_dbg_sprintf("UNKNOWN(%08x)", dwControlType);
}

/* A simple declaration of a line control
 * These are each of the channels that show up
 */
typedef struct line {
    /* Name we present to outside world */
    WCHAR name[MAXPNAMELEN];

    DWORD component;
    DWORD dst;
    DWORD capt;
    DWORD chans;
    snd_mixer_elem_t *elem;
} line;

/* A control structure, with toggle enabled switch
 * Control structures control volume, muted, which capture source
 */
typedef struct control {
    BOOL enabled;
    MIXERCONTROLW c;
} control;

/* Mixer device */
typedef struct mixer
{
    snd_mixer_t *mix;
    WCHAR mixername[MAXPNAMELEN];

    int chans, dests;
    LPDRVCALLBACK callback;
    DWORD_PTR callbackpriv;
    HDRVR hmx;

    line *lines;
    control *controls;
} mixer;

#define MAX_MIXERS 32
#define CONTROLSPERLINE 3
#define OFS_MUTE 2
#define OFS_MUX 1

static int cards = 0;
static mixer mixdev[MAX_MIXERS];
static HANDLE thread;
static int elem_callback(snd_mixer_elem_t *elem, unsigned int mask);
static DWORD WINAPI ALSA_MixerPollThread(LPVOID lParam);
static CRITICAL_SECTION elem_crst;
static int msg_pipe[2];
static LONG refcnt;

/* found channel names in alsa lib, alsa api doesn't have another way for this
 * map name -> componenttype, worst case we get a wrong componenttype which is
 * mostly harmless
 */

static const struct mixerlinetype {
    const char *name;  DWORD cmpt;
} converttable[] = {
    { "Master",     MIXERLINE_COMPONENTTYPE_DST_SPEAKERS,    },
    { "Capture",    MIXERLINE_COMPONENTTYPE_DST_WAVEIN,      },
    { "PCM",        MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT,     },
    { "PC Speaker", MIXERLINE_COMPONENTTYPE_SRC_PCSPEAKER,   },
    { "Synth",      MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER, },
    { "Headphone",  MIXERLINE_COMPONENTTYPE_DST_HEADPHONES,  },
    { "Mic",        MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE,  },
    { "Aux",        MIXERLINE_COMPONENTTYPE_SRC_UNDEFINED,   },
    { "CD",         MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC, },
    { "Line",       MIXERLINE_COMPONENTTYPE_SRC_LINE,        },
    { "Phone",      MIXERLINE_COMPONENTTYPE_SRC_TELEPHONE,   },
};

/* Map name to MIXERLINE_COMPONENTTYPE_XXX */
static int getcomponenttype(const char *name)
{
    int x;
    for (x=0; x< sizeof(converttable)/sizeof(converttable[0]); ++x)
        if (!strcasecmp(name, converttable[x].name))
        {
            TRACE("%d -> %s\n", x, name);
            return converttable[x].cmpt;
        }
    WARN("Unknown mixer name %s, probably harmless\n", name);
    return MIXERLINE_COMPONENTTYPE_SRC_UNDEFINED;
}

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

static void fillcontrols(mixer *mmixer)
{
    int id;
    for (id = 0; id < mmixer->chans; ++id)
    {
        line *mline = &mmixer->lines[id];
        int ofs = CONTROLSPERLINE * id;
        int x;
        long min, max;

        TRACE("Filling control %d\n", id);
        if (id == 1 && !mline->elem)
            continue;

        if (mline->capt && snd_mixer_selem_has_capture_volume(mline->elem))
            snd_mixer_selem_get_capture_volume_range(mline->elem, &min, &max);
        else
            snd_mixer_selem_get_playback_volume_range(mline->elem, &min, &max);

        /* (!snd_mixer_selem_has_playback_volume(elem) || snd_mixer_selem_has_capture_volume(elem)) */
        /* Volume, always enabled by definition of blacklisted channels */
        mmixer->controls[ofs].enabled = 1;
        mmixer->controls[ofs].c.cbStruct = sizeof(mmixer->controls[ofs].c);
        mmixer->controls[ofs].c.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
        mmixer->controls[ofs].c.dwControlID = ofs;
        mmixer->controls[ofs].c.Bounds.s1.dwMinimum = 0;
        mmixer->controls[ofs].c.Bounds.s1.dwMaximum = 65535;
        mmixer->controls[ofs].c.Metrics.cSteps = 65536/(max-min);

        if ((id == 1 && snd_mixer_selem_has_capture_switch(mline->elem)) ||
            (!mline->capt && snd_mixer_selem_has_playback_switch(mline->elem)))
        { /* MUTE button optional, main capture channel should have one too */
            mmixer->controls[ofs+OFS_MUTE].enabled = 1;
            mmixer->controls[ofs+OFS_MUTE].c.cbStruct = sizeof(mmixer->controls[ofs].c);
            mmixer->controls[ofs+OFS_MUTE].c.dwControlType = MIXERCONTROL_CONTROLTYPE_MUTE;
            mmixer->controls[ofs+OFS_MUTE].c.dwControlID = ofs+OFS_MUTE;
            mmixer->controls[ofs+OFS_MUTE].c.Bounds.s1.dwMaximum = 1;
        }

        if (mline->capt && snd_mixer_selem_has_capture_switch_exclusive(mline->elem))
            mmixer->controls[CONTROLSPERLINE+OFS_MUX].c.dwControlType = MIXERCONTROL_CONTROLTYPE_MUX;

        if (id == 1)
        { /* Capture select, in case cMultipleItems is 0, it means capture is disabled anyway */
            mmixer->controls[ofs+OFS_MUX].enabled = 1;
            mmixer->controls[ofs+OFS_MUX].c.cbStruct = sizeof(mmixer->controls[ofs].c);
            mmixer->controls[ofs+OFS_MUX].c.dwControlType = MIXERCONTROL_CONTROLTYPE_MIXER;
            mmixer->controls[ofs+OFS_MUX].c.dwControlID = ofs+OFS_MUX;
            mmixer->controls[ofs+OFS_MUX].c.fdwControl = MIXERCONTROL_CONTROLF_MULTIPLE;

            for (x = 0; x<mmixer->chans; ++x)
                if (x != id && mmixer->lines[x].dst == id)
                    ++(mmixer->controls[ofs+OFS_MUX].c.cMultipleItems);
            if (!mmixer->controls[ofs+OFS_MUX].c.cMultipleItems)
                mmixer->controls[ofs+OFS_MUX].enabled = 0;

            mmixer->controls[ofs+OFS_MUX].c.Bounds.s1.dwMaximum = mmixer->controls[ofs+OFS_MUX].c.cMultipleItems - 1;
            mmixer->controls[ofs+OFS_MUX].c.Metrics.cSteps = mmixer->controls[ofs+OFS_MUX].c.cMultipleItems;
        }
        for (x=0; x<CONTROLSPERLINE; ++x)
        {
            lstrcpynW(mmixer->controls[ofs+x].c.szShortName, mline->name, sizeof(mmixer->controls[ofs+x].c.szShortName)/sizeof(WCHAR));
            lstrcpynW(mmixer->controls[ofs+x].c.szName, mline->name, sizeof(mmixer->controls[ofs+x].c.szName)/sizeof(WCHAR));
        }
    }
}

/* get amount of channels for elem */
/* Officially we should keep capture/playback separated,
 * but that's not going to work in the alsa api */
static int chans(mixer *mmixer, snd_mixer_elem_t * elem, DWORD capt)
{
    int ret=0, chn;

    if (capt && snd_mixer_selem_has_capture_volume(elem)) {
        for (chn = 0; chn <= SND_MIXER_SCHN_LAST; ++chn)
            if (snd_mixer_selem_has_capture_channel(elem, chn))
                ++ret;
    } else {
        for (chn = 0; chn <= SND_MIXER_SCHN_LAST; ++chn)
            if (snd_mixer_selem_has_playback_channel(elem, chn))
                ++ret;
    }
    if (!ret)
        FIXME("Mixer channel %s was found for %s, but no channels were found? Wrong selection!\n", snd_mixer_selem_get_name(elem), (snd_mixer_selem_has_playback_volume(elem) ? "playback" : "capture"));
    return ret;
}

static void filllines(mixer *mmixer, snd_mixer_elem_t *mastelem, snd_mixer_elem_t *captelem, int capt)
{
    snd_mixer_elem_t *elem;
    line *mline = mmixer->lines;

    /* Master control */
    MultiByteToWideChar(CP_UNIXCP, 0, snd_mixer_selem_get_name(mastelem), -1, mline->name, sizeof(mline->name)/sizeof(WCHAR));
    mline->component = getcomponenttype(snd_mixer_selem_get_name(mastelem));
    mline->dst = 0;
    mline->capt = 0;
    mline->elem = mastelem;
    mline->chans = chans(mmixer, mastelem, 0);

    snd_mixer_elem_set_callback(mastelem, &elem_callback);
    snd_mixer_elem_set_callback_private(mastelem, mmixer);

    /* Capture control
     * Note: since mmixer->dests = 1, it means only playback control is visible
     * This makes sense, because if there are no capture sources capture control
     * can't do anything and should be invisible */

    /* Control 1 is reserved for capture even when not enabled */
    ++mline;
    if (capt)
    {
        MultiByteToWideChar(CP_UNIXCP, 0, snd_mixer_selem_get_name(captelem), -1, mline->name, sizeof(mline->name)/sizeof(WCHAR));
        mline->component = getcomponenttype(snd_mixer_selem_get_name(captelem));
        mline->dst = 1;
        mline->capt = 1;
        mline->elem = captelem;
        mline->chans = chans(mmixer, captelem, 1);

        snd_mixer_elem_set_callback(captelem, &elem_callback);
        snd_mixer_elem_set_callback_private(captelem, mmixer);
    }

    for (elem = snd_mixer_first_elem(mmixer->mix); elem; elem = snd_mixer_elem_next(elem))
        if (elem != mastelem && elem != captelem && !blacklisted(elem))
        {
            const char * name = snd_mixer_selem_get_name(elem);
            DWORD comp = getcomponenttype(name);

            if (snd_mixer_selem_has_playback_volume(elem))
            {
                (++mline)->component = comp;
                MultiByteToWideChar(CP_UNIXCP, 0, name, -1, mline->name, MAXPNAMELEN);
                mline->capt = mline->dst = 0;
                mline->elem = elem;
                mline->chans = chans(mmixer, elem, 0);
            }
            else if (!capt)
                continue;

            if (capt && snd_mixer_selem_has_capture_switch(elem))
            {
                (++mline)->component = comp;
                MultiByteToWideChar(CP_UNIXCP, 0, name, -1, mline->name, MAXPNAMELEN);
                mline->capt = mline->dst = 1;
                mline->elem = elem;
                mline->chans = chans(mmixer, elem, 1);
            }

            snd_mixer_elem_set_callback(elem, &elem_callback);
            snd_mixer_elem_set_callback_private(elem, mmixer);
        }
}

/* Windows api wants to have a 'master' device to which all slaves are attached
 * There are 2 ones in this code:
 * - 'Master', fall back to 'Headphone' if unavailable, and if that's not available 'PCM'
 * - 'Capture'
 * Capture might not always be available, so should be prepared to be without if needed
 */

static void ALSA_MixerInit(void)
{
    int x, mixnum = 0;

    for (x = 0; x < MAX_MIXERS; ++x)
    {
        int card, err, capcontrols = 0;
        char cardind[6], cardname[10];

        snd_ctl_t *ctl;
        snd_mixer_elem_t *elem, *mastelem = NULL, *headelem = NULL, *captelem = NULL, *pcmelem = NULL;
        snd_ctl_card_info_t *info = NULL;
        snd_ctl_card_info_alloca(&info);

        memset(&mixdev[mixnum], 0, sizeof(*mixdev));
        snprintf(cardind, sizeof(cardind), "%d", x);
        card = snd_card_get_index(cardind);
        if (card < 0)
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

        err = snd_mixer_open(&mixdev[mixnum].mix, 0);
        if (err < 0)
        {
            WARN("Error occurred opening mixer: %s\n", snd_strerror(err));
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

        /* First, lets see what's available..
         * If there are multiple Master or Captures, all except 1 will be added as slaves
         */
        for (elem = snd_mixer_first_elem(mixdev[mixnum].mix); elem; elem = snd_mixer_elem_next(elem))
            if (!strcasecmp(snd_mixer_selem_get_name(elem), "Master") && !mastelem)
                mastelem = elem;
            else if (!strcasecmp(snd_mixer_selem_get_name(elem), "Capture") && !captelem)
                captelem = elem;
            else if (!blacklisted(elem))
            {
                if (snd_mixer_selem_has_capture_switch(elem))
                    ++capcontrols;
                if (snd_mixer_selem_has_playback_volume(elem))
                {
                    if (!strcasecmp(snd_mixer_selem_get_name(elem), "Headphone") && !headelem)
                        headelem = elem;
                    else if (!strcasecmp(snd_mixer_selem_get_name(elem), "PCM") && !pcmelem)
                        pcmelem = elem;
                    else
                        ++(mixdev[mixnum].chans);
                }
            }

        /* Add master channel, uncounted channels and an extra for capture  */
        mixdev[mixnum].chans += !!mastelem + !!headelem + !!pcmelem + 1;

        /* If there is only 'Capture' and 'Master', this device is not worth it */
        if (mixdev[mixnum].chans == 2)
        {
            WARN("No channels found, skipping device!\n");
            goto close;
        }

        /* Master element can't have a capture control in this code, so
         * if Headphone or PCM is promoted to master, unset its capture control */
        if (headelem && !mastelem)
        {
            /* Using 'Headphone' as master device */
            mastelem = headelem;
            capcontrols -= !!snd_mixer_selem_has_capture_switch(mastelem);
        }
        else if (pcmelem && !mastelem)
        {
            /* Use 'PCM' as master device */
            mastelem = pcmelem;
            capcontrols -= !!snd_mixer_selem_has_capture_switch(mastelem);
        }
        else if (!mastelem)
        {
            /* If there is nothing sensible that can act as 'Master' control, something is wrong */
            FIXME("No master control found, disabling mixer\n");
            goto close;
        }

        if (!captelem || !capcontrols)
        {
            /* Can't enable capture, so disabling it
             * Note: capture control will still exist because
             * dwLineID 0 and 1 are reserved for Master and Capture
             */
            WARN("No use enabling capture part of mixer, capture control found: %s, amount of capture controls: %d\n",
                 (!captelem ? "no" : "yes"), capcontrols);
            capcontrols = 0;
            mixdev[mixnum].dests = 1;
        }
        else
        {
            mixdev[mixnum].chans += capcontrols;
            mixdev[mixnum].dests = 2;
        }

        mixdev[mixnum].lines = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(line) * mixdev[mixnum].chans);
        mixdev[mixnum].controls = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(control) * CONTROLSPERLINE*mixdev[mixnum].chans);
        err = -ENOMEM;
        if (!mixdev[mixnum].lines || !mixdev[mixnum].controls)
            goto close;

        filllines(&mixdev[mixnum], mastelem, captelem, capcontrols);
        fillcontrols(&mixdev[mixnum]);

        TRACE("%s: Amount of controls: %i/%i, name: %s\n", cardname, mixdev[mixnum].dests, mixdev[mixnum].chans, debugstr_w(mixdev[mixnum].mixername));
        mixnum++;
        continue;

        eclose:
        WARN("Error occurred initialising mixer: %s\n", snd_strerror(err));
        close:
        HeapFree(GetProcessHeap(), 0, mixdev[mixnum].lines);
        HeapFree(GetProcessHeap(), 0, mixdev[mixnum].controls);
        snd_mixer_close(mixdev[mixnum].mix);
    }
    cards = mixnum;

    /* There is no trouble with already assigning callbacks without initialising critsect:
     * Callbacks only occur when snd_mixer_handle_events is called (only happens in thread)
     */
    InitializeCriticalSection(&elem_crst);
    elem_crst.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": ALSA_MIXER.elem_crst");
    TRACE("\n");
}

static void ALSA_MixerExit(void)
{
    int x;

    if (refcnt)
    {
        WARN("Callback thread still alive, terminating uncleanly, refcnt: %d\n", refcnt);
        /* Least we can do is making sure we're not in 'foreign' code */
        EnterCriticalSection(&elem_crst);
        TerminateThread(thread, 1);
        refcnt = 0;
    }

    TRACE("Cleaning up\n");

    elem_crst.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&elem_crst);
    for (x = 0; x < cards; ++x)
    {
        snd_mixer_close(mixdev[x].mix);
        HeapFree(GetProcessHeap(), 0, mixdev[x].lines);
        HeapFree(GetProcessHeap(), 0, mixdev[x].controls);
    }
    cards = 0;
}

static mixer* MIX_GetMix(UINT wDevID)
{
    mixer *mmixer;

    if (wDevID < 0 || wDevID >= cards)
    {
        WARN("Invalid mixer id: %d\n", wDevID);
        return NULL;
    }

    mmixer = &mixdev[wDevID];
    return mmixer;
}

/* Since alsa doesn't tell what exactly changed, just assume all affected controls changed */
static int elem_callback(snd_mixer_elem_t *elem, unsigned int type)
{
    mixer *mmixer = snd_mixer_elem_get_callback_private(elem);
    int x;
    BOOL captchanged = 0;

    if (type != SND_CTL_EVENT_MASK_VALUE)
        return 0;

    assert(mmixer);

    EnterCriticalSection(&elem_crst);

    if (!mmixer->callback)
        goto out;

    for (x=0; x<mmixer->chans; ++x)
    {
        const int ofs = CONTROLSPERLINE*x;
        if (elem != mmixer->lines[x].elem)
            continue;

        if (mmixer->lines[x].capt)
            ++captchanged;

        TRACE("Found changed control %s\n", debugstr_w(mmixer->lines[x].name));
        mmixer->callback(mmixer->hmx, MM_MIXM_LINE_CHANGE, mmixer->callbackpriv, x, 0);
        mmixer->callback(mmixer->hmx, MM_MIXM_CONTROL_CHANGE, mmixer->callbackpriv, ofs, 0);

        if (mmixer->controls[ofs+OFS_MUTE].enabled)
            mmixer->callback(mmixer->hmx, MM_MIXM_CONTROL_CHANGE, mmixer->callbackpriv, ofs+OFS_MUTE, 0);
    }
    if (captchanged)
        mmixer->callback(mmixer->hmx, MM_MIXM_CONTROL_CHANGE, mmixer->callbackpriv, CONTROLSPERLINE+OFS_MUX, 0);

    out:
    LeaveCriticalSection(&elem_crst);

    return 0;
}

static DWORD WINAPI ALSA_MixerPollThread(LPVOID lParam)
{
    struct pollfd *pfds = NULL;
    int x, y, err, mcnt, count = 1;

    TRACE("%p\n", lParam);

    for (x = 0; x < cards; ++x)
        count += snd_mixer_poll_descriptors_count(mixdev[x].mix);

    TRACE("Counted %d descriptors\n", count);
    pfds = HeapAlloc(GetProcessHeap(), 0, count * sizeof(struct pollfd));

    if (!pfds)
    {
        WARN("Out of memory\n");
        goto die;
    }

    pfds[0].fd = msg_pipe[0];
    pfds[0].events = POLLIN;

    y = 1;
    for (x = 0; x < cards; ++x)
        y += snd_mixer_poll_descriptors(mixdev[x].mix, &pfds[y], count - y);

    while ((err = poll(pfds, (unsigned int) count, -1)) >= 0 || errno == EINTR || errno == EAGAIN)
    {
        if (pfds[0].revents & POLLIN)
            break;

        mcnt = 1;
        for (x = y = 0; x < cards; ++x)
        {
            int j, max = snd_mixer_poll_descriptors_count(mixdev[x].mix);
            for (j = 0; j < max; ++j)
                if (pfds[mcnt+j].revents)
                {
                    y += snd_mixer_handle_events(mixdev[x].mix);
                    break;
                }
            mcnt += max;
        }
        if (y)
            TRACE("Handled %d events\n", y);
    }

    die:
    TRACE("Shutting down\n");
    HeapFree(GetProcessHeap(), 0, pfds);

    y = read(msg_pipe[0], &x, sizeof(x));
    close(msg_pipe[1]);
    close(msg_pipe[0]);
    return 0;
}

static DWORD MIX_Open(UINT wDevID, LPMIXEROPENDESC desc, DWORD_PTR flags)
{
    mixer *mmixer = MIX_GetMix(wDevID);
    if (!mmixer)
        return MMSYSERR_BADDEVICEID;

    flags &= CALLBACK_TYPEMASK;
    switch (flags)
    {
    case CALLBACK_NULL:
        goto done;

    case CALLBACK_FUNCTION:
        break;

    default:
        FIXME("Unhandled callback type: %08lx\n", flags & CALLBACK_TYPEMASK);
        return MIXERR_INVALVALUE;
    }

    mmixer->callback = (LPDRVCALLBACK)desc->dwCallback;
    mmixer->callbackpriv = desc->dwInstance;
    mmixer->hmx = (HDRVR)desc->hmx;

    done:
    if (InterlockedIncrement(&refcnt) == 1)
    {
        if (pipe(msg_pipe) >= 0)
        {
            thread = CreateThread(NULL, 0, ALSA_MixerPollThread, NULL, 0, NULL);
            if (!thread)
            {
                close(msg_pipe[0]);
                close(msg_pipe[1]);
                msg_pipe[0] = msg_pipe[1] = -1;
            }
        }
        else
            msg_pipe[0] = msg_pipe[1] = -1;
    }

    return MMSYSERR_NOERROR;
}

static DWORD MIX_Close(UINT wDevID)
{
    int x;
    mixer *mmixer = MIX_GetMix(wDevID);
    if (!mmixer)
        return MMSYSERR_BADDEVICEID;

    EnterCriticalSection(&elem_crst);
    mmixer->callback = 0;
    LeaveCriticalSection(&elem_crst);

    if (!InterlockedDecrement(&refcnt))
    {
        if (write(msg_pipe[1], &x, sizeof(x)) > 0)
        {
            TRACE("Shutting down thread...\n");
            WaitForSingleObject(thread, INFINITE);
            TRACE("Done\n");
        }
    }

    return MMSYSERR_NOERROR;
}

static DWORD MIX_GetDevCaps(UINT wDevID, LPMIXERCAPS2W caps, DWORD_PTR parm2)
{
    mixer *mmixer = MIX_GetMix(wDevID);
    MIXERCAPS2W capsW;

    if (!caps)
        return MMSYSERR_INVALPARAM;

    if (!mmixer)
        return MMSYSERR_BADDEVICEID;

    memset(&capsW, 0, sizeof(MIXERCAPS2W));

    capsW.wMid = WINE_MIXER_MANUF_ID;
    capsW.wPid = WINE_MIXER_PRODUCT_ID;
    capsW.vDriverVersion = WINE_MIXER_VERSION;

    lstrcpynW(capsW.szPname, mmixer->mixername, sizeof(capsW.szPname)/sizeof(WCHAR));
    capsW.cDestinations = mmixer->dests;
    memcpy(caps, &capsW, min(parm2, sizeof(capsW)));
    return MMSYSERR_NOERROR;
}

/* convert win32 volume to alsa volume, and vice versa */
static INT normalized(INT value, INT prevmax, INT nextmax)
{
    int ret = MulDiv(value, nextmax, prevmax);

    /* Have to stay in range */
    TRACE("%d/%d -> %d/%d\n", value, prevmax, ret, nextmax);
    if (ret > nextmax)
        ret = nextmax;
    else if (ret < 0)
        ret = 0;

    return ret;
}

/* get amount of sources for dest */
static int getsrccntfromchan(mixer *mmixer, int dad)
{
    int i, j=0;

    for (i=0; i<mmixer->chans; ++i)
        if (i != dad && mmixer->lines[i].dst == dad)
        {
            ++j;
        }
    if (!j)
        FIXME("No src found for %i (%s)?\n", dad, debugstr_w(mmixer->lines[dad].name));
    return j;
}

/* find lineid for source 'num' with dest 'dad' */
static int getsrclinefromchan(mixer *mmixer, int dad, int num)
{
    int i, j=0;
    for (i=0; i<mmixer->chans; ++i)
        if (i != dad && mmixer->lines[i].dst == dad)
        {
            if (num == j)
                return i;
            ++j;
        }
    WARN("No src found for src %i from dest %i\n", num, dad);
    return 0;
}

/* get the source number belonging to line */
static int getsrcfromline(mixer *mmixer, int line)
{
    int i, j=0, dad = mmixer->lines[line].dst;

    for (i=0; i<mmixer->chans; ++i)
        if (i != dad && mmixer->lines[i].dst == dad)
        {
            if (line == i)
                return j;
            ++j;
        }
    WARN("No src found for line %i with dad %i\n", line, dad);
    return 0;
}

/* Get volume/muted/capture channel */
static DWORD MIX_GetControlDetails(UINT wDevID, LPMIXERCONTROLDETAILS mctrld, DWORD_PTR flags)
{
    mixer *mmixer = MIX_GetMix(wDevID);
    DWORD ctrl;
    DWORD line;
    control *ct;

    if (!mctrld)
        return MMSYSERR_INVALPARAM;

    ctrl = mctrld->dwControlID;
    line = ctrl/CONTROLSPERLINE;

    if (mctrld->cbStruct != sizeof(*mctrld))
        return MMSYSERR_INVALPARAM;

    if (!mmixer)
        return MMSYSERR_BADDEVICEID;

    if (line < 0 || line >= mmixer->chans || !mmixer->controls[ctrl].enabled)
        return MIXERR_INVALCONTROL;

    ct = &mmixer->controls[ctrl];

    flags &= MIXER_GETCONTROLDETAILSF_QUERYMASK;

    switch (flags) {
    case MIXER_GETCONTROLDETAILSF_VALUE:
        TRACE("MIXER_GETCONTROLDETAILSF_VALUE (%d/%d)\n", ctrl, line);
        switch (ct->c.dwControlType)
        {
        case MIXERCONTROL_CONTROLTYPE_VOLUME:
        {
            long min = 0, max = 0, vol = 0;
            int chn;
            LPMIXERCONTROLDETAILS_UNSIGNED mcdu;
            snd_mixer_elem_t * elem = mmixer->lines[line].elem;

            if (mctrld->cbDetails != sizeof(MIXERCONTROLDETAILS_UNSIGNED))
            {
                WARN("invalid parameter: cbDetails %d\n", mctrld->cbDetails);
                return MMSYSERR_INVALPARAM;
            }

            TRACE("%s MIXERCONTROLDETAILS_UNSIGNED[%u]\n", getControlType(ct->c.dwControlType), mctrld->cChannels);

            mcdu = (LPMIXERCONTROLDETAILS_UNSIGNED)mctrld->paDetails;

            if (mctrld->cChannels != 1 && mmixer->lines[line].chans != mctrld->cChannels)
            {
                WARN("Unsupported cChannels (%d instead of %d)\n", mctrld->cChannels, mmixer->lines[line].chans);
                return MMSYSERR_INVALPARAM;
            }

            if (mmixer->lines[line].capt && snd_mixer_selem_has_capture_volume(elem)) {
                snd_mixer_selem_get_capture_volume_range(elem, &min, &max);
                for (chn = 0; chn <= SND_MIXER_SCHN_LAST; ++chn)
                    if (snd_mixer_selem_has_capture_channel(elem, chn))
                    {
                        snd_mixer_selem_get_capture_volume(elem, chn, &vol);
                        mcdu->dwValue = normalized(vol - min, max, 65535);
                        if (mctrld->cChannels == 1)
                            break;
                        ++mcdu;
                    }
            } else {
                snd_mixer_selem_get_playback_volume_range(elem, &min, &max);

                for (chn = 0; chn <= SND_MIXER_SCHN_LAST; ++chn)
                    if (snd_mixer_selem_has_playback_channel(elem, chn))
                    {
                        snd_mixer_selem_get_playback_volume(elem, chn, &vol);
                        mcdu->dwValue = normalized(vol - min, max, 65535);
                        if (mctrld->cChannels == 1)
                            break;
                        ++mcdu;
                    }
            }

            return MMSYSERR_NOERROR;
        }

        case MIXERCONTROL_CONTROLTYPE_ONOFF:
        case MIXERCONTROL_CONTROLTYPE_MUTE:
        {
            LPMIXERCONTROLDETAILS_BOOLEAN mcdb;
            int chn, ival;
            snd_mixer_elem_t * elem = mmixer->lines[line].elem;

            if (mctrld->cbDetails != sizeof(MIXERCONTROLDETAILS_BOOLEAN))
            {
                WARN("invalid parameter: cbDetails %d\n", mctrld->cbDetails);
                return MMSYSERR_INVALPARAM;
            }

            TRACE("%s MIXERCONTROLDETAILS_BOOLEAN[%u]\n", getControlType(ct->c.dwControlType), mctrld->cChannels);

            mcdb = (LPMIXERCONTROLDETAILS_BOOLEAN)mctrld->paDetails;

            if (line == 1)
                for (chn = 0; chn <= SND_MIXER_SCHN_LAST; ++chn)
                {
                    if (!snd_mixer_selem_has_capture_channel(elem, chn))
                        continue;
                    snd_mixer_selem_get_capture_switch(elem, chn, &ival);
                    break;
                }
            else
                for (chn = 0; chn <= SND_MIXER_SCHN_LAST; ++chn)
                {
                    if (!snd_mixer_selem_has_playback_channel(elem, chn))
                        continue;
                    snd_mixer_selem_get_playback_switch(elem, chn, &ival);
                    break;
                }

            mcdb->fValue = !ival;
            TRACE("=> %s\n", mcdb->fValue ? "on" : "off");
            return MMSYSERR_NOERROR;
        }
        case MIXERCONTROL_CONTROLTYPE_MIXER:
        case MIXERCONTROL_CONTROLTYPE_MUX:
        {
            LPMIXERCONTROLDETAILS_BOOLEAN mcdb;
            int x, i=0, ival = 0, chn;

            if (mctrld->cbDetails != sizeof(MIXERCONTROLDETAILS_BOOLEAN))
            {
                WARN("invalid parameter: cbDetails %d\n", mctrld->cbDetails);
                return MMSYSERR_INVALPARAM;
            }

            TRACE("%s MIXERCONTROLDETAILS_BOOLEAN[%u]\n", getControlType(ct->c.dwControlType), mctrld->cChannels);

            mcdb = (LPMIXERCONTROLDETAILS_BOOLEAN)mctrld->paDetails;

            for (x = 0; x<mmixer->chans; ++x)
                if (line != x && mmixer->lines[x].dst == line)
                {
                    ival = 0;
                    for (chn = 0; chn <= SND_MIXER_SCHN_LAST; ++chn)
                    {
                        if (!snd_mixer_selem_has_capture_channel(mmixer->lines[x].elem, chn))
                            continue;
                        snd_mixer_selem_get_capture_switch(mmixer->lines[x].elem, chn, &ival);
                        if (ival)
                            break;
                    }
                    if (i >= mctrld->u.cMultipleItems)
                    {
                        TRACE("overflow\n");
                        return MMSYSERR_INVALPARAM;
                    }
                    TRACE("fVal[%i] = %sselected\n", i, (!ival ? "un" : ""));
                    mcdb[i++].fValue = ival;
                }
            break;
        }
        default:

            FIXME("Unhandled controltype %s\n", getControlType(ct->c.dwControlType));
            return MMSYSERR_INVALPARAM;
        }
        return MMSYSERR_NOERROR;

    case MIXER_GETCONTROLDETAILSF_LISTTEXT:
        TRACE("MIXER_GETCONTROLDETAILSF_LISTTEXT (%d)\n", ctrl);

        if (ct->c.dwControlType == MIXERCONTROL_CONTROLTYPE_MUX || ct->c.dwControlType == MIXERCONTROL_CONTROLTYPE_MIXER)
        {
            LPMIXERCONTROLDETAILS_LISTTEXTW mcdlt = (LPMIXERCONTROLDETAILS_LISTTEXTW)mctrld->paDetails;
            int i, j;

            for (i = j = 0; j < mmixer->chans; ++j)
                if (j != line && mmixer->lines[j].dst == line)
                {
                    if (i > mctrld->u.cMultipleItems)
                        return MMSYSERR_INVALPARAM;
                    mcdlt->dwParam1 = j;
                    mcdlt->dwParam2 = mmixer->lines[j].component;
                    lstrcpynW(mcdlt->szName, mmixer->lines[j].name, sizeof(mcdlt->szName) / sizeof(WCHAR));
                    TRACE("Adding %i as %s\n", j, debugstr_w(mcdlt->szName));
                    ++i; ++mcdlt;
                }
            if (i < mctrld->u.cMultipleItems)
                return MMSYSERR_INVALPARAM;
            return MMSYSERR_NOERROR;
        }
        FIXME ("Imagine this code being horribly broken and incomplete, introducing: reality\n");
        return MMSYSERR_INVALPARAM;

    default:
        WARN("Unknown flag (%08lx)\n", flags);
        return MMSYSERR_INVALPARAM;
    }
}

/* Set volume/capture channel/muted for control */
static DWORD MIX_SetControlDetails(UINT wDevID, LPMIXERCONTROLDETAILS mctrld, DWORD_PTR flags)
{
    mixer *mmixer = MIX_GetMix(wDevID);
    DWORD ctrl, line, i;
    control *ct;
    snd_mixer_elem_t * elem;

    if (!mctrld)
        return MMSYSERR_INVALPARAM;

    ctrl = mctrld->dwControlID;
    line = ctrl/CONTROLSPERLINE;

    if (mctrld->cbStruct != sizeof(*mctrld))
    {
        WARN("Invalid size of mctrld %d\n", mctrld->cbStruct);
        return MMSYSERR_INVALPARAM;
    }

    if (!mmixer)
        return MMSYSERR_BADDEVICEID;

    if (line < 0 || line >= mmixer->chans)
    {
        WARN("Invalid line id: %d not in range of 0-%d\n", line, mmixer->chans-1);
        return MMSYSERR_INVALPARAM;
    }

    if (!mmixer->controls[ctrl].enabled)
    {
        WARN("Control %d not enabled\n", ctrl);
        return MIXERR_INVALCONTROL;
    }

    ct = &mmixer->controls[ctrl];
    elem = mmixer->lines[line].elem;
    flags &= MIXER_SETCONTROLDETAILSF_QUERYMASK;

    switch (flags) {
    case MIXER_SETCONTROLDETAILSF_VALUE:
        TRACE("MIXER_SETCONTROLDETAILSF_VALUE (%d)\n", ctrl);
        break;

    default:
        WARN("Unknown flag (%08lx)\n", flags);
        return MMSYSERR_INVALPARAM;
    }

    switch (ct->c.dwControlType)
    {
    case MIXERCONTROL_CONTROLTYPE_VOLUME:
    {
        long min = 0, max = 0;
        int chn;
        LPMIXERCONTROLDETAILS_UNSIGNED mcdu;
        snd_mixer_elem_t * elem = mmixer->lines[line].elem;

        if (mctrld->cbDetails != sizeof(MIXERCONTROLDETAILS_UNSIGNED))
        {
            WARN("invalid parameter: cbDetails %d\n", mctrld->cbDetails);
            return MMSYSERR_INVALPARAM;
        }

        if (mctrld->cChannels != 1 && mmixer->lines[line].chans != mctrld->cChannels)
        {
            WARN("Unsupported cChannels (%d instead of %d)\n", mctrld->cChannels, mmixer->lines[line].chans);
            return MMSYSERR_INVALPARAM;
        }

        TRACE("%s MIXERCONTROLDETAILS_UNSIGNED[%u]\n", getControlType(ct->c.dwControlType), mctrld->cChannels);
        mcdu = (LPMIXERCONTROLDETAILS_UNSIGNED)mctrld->paDetails;

        for (chn=0; chn<mctrld->cChannels;++chn)
        {
            TRACE("Chan %d value %d\n", chn, mcdu[chn].dwValue);
        }

        /* There isn't always a capture volume, so in that case change playback volume */
        if (mmixer->lines[line].capt && snd_mixer_selem_has_capture_volume(elem))
        {
            snd_mixer_selem_get_capture_volume_range(elem, &min, &max);

            for (chn = 0; chn <= SND_MIXER_SCHN_LAST; ++chn)
                if (snd_mixer_selem_has_capture_channel(elem, chn))
                {
                    snd_mixer_selem_set_capture_volume(elem, chn, min + normalized(mcdu->dwValue, 65535, max));
                    if (mctrld->cChannels != 1)
                        mcdu++;
                }
        }
        else
        {
            snd_mixer_selem_get_playback_volume_range(elem, &min, &max);

            for (chn = 0; chn <= SND_MIXER_SCHN_LAST; ++chn)
                if (snd_mixer_selem_has_playback_channel(elem, chn))
                {
                    snd_mixer_selem_set_playback_volume(elem, chn, min + normalized(mcdu->dwValue, 65535, max));
                    if (mctrld->cChannels != 1)
                        mcdu++;
                }
        }

        break;
    }
    case MIXERCONTROL_CONTROLTYPE_MUTE:
    case MIXERCONTROL_CONTROLTYPE_ONOFF:
    {
        LPMIXERCONTROLDETAILS_BOOLEAN	mcdb;

        if (mctrld->cbDetails != sizeof(MIXERCONTROLDETAILS_BOOLEAN))
        {
            WARN("invalid parameter: cbDetails %d\n", mctrld->cbDetails);
            return MMSYSERR_INVALPARAM;
        }

        TRACE("%s MIXERCONTROLDETAILS_BOOLEAN[%u]\n", getControlType(ct->c.dwControlType), mctrld->cChannels);

        mcdb = (LPMIXERCONTROLDETAILS_BOOLEAN)mctrld->paDetails;
        if (line == 1) /* Mute/unmute capturing */
            for (i = 0; i <= SND_MIXER_SCHN_LAST; ++i)
            {
                if (snd_mixer_selem_has_capture_channel(elem, i))
                    snd_mixer_selem_set_capture_switch(elem, i, !mcdb->fValue);
            }
        else
            for (i = 0; i <= SND_MIXER_SCHN_LAST; ++i)
                if (snd_mixer_selem_has_playback_channel(elem, i))
                    snd_mixer_selem_set_playback_switch(elem, i, !mcdb->fValue);
        break;
    }

    case MIXERCONTROL_CONTROLTYPE_MIXER:
    case MIXERCONTROL_CONTROLTYPE_MUX:
    {
        LPMIXERCONTROLDETAILS_BOOLEAN mcdb;
        int x, i=0, chn;
        int didone = 0, canone = (ct->c.dwControlType == MIXERCONTROL_CONTROLTYPE_MUX);

        if (mctrld->cbDetails != sizeof(MIXERCONTROLDETAILS_BOOLEAN))
        {
            WARN("invalid parameter: cbDetails %d\n", mctrld->cbDetails);
            return MMSYSERR_INVALPARAM;
        }

        TRACE("%s MIXERCONTROLDETAILS_BOOLEAN[%u]\n", getControlType(ct->c.dwControlType), mctrld->cChannels);
        mcdb = (LPMIXERCONTROLDETAILS_BOOLEAN)mctrld->paDetails;

        for (x=i=0; x < mmixer->chans; ++x)
            if (line != x && mmixer->lines[x].dst == line)
            {
                TRACE("fVal[%i] (%s) = %i\n", i, debugstr_w(mmixer->lines[x].name), mcdb[i].fValue);
                if (i >= mctrld->u.cMultipleItems)
                {
                    TRACE("Too many items to fit, overflowing\n");
                    return MIXERR_INVALVALUE;
                }
                if (mcdb[i].fValue && canone && didone)
                {
                    TRACE("Nice try, but it's not going to work\n");
                    elem_callback(mmixer->lines[1].elem, SND_CTL_EVENT_MASK_VALUE);
                    return MIXERR_INVALVALUE;
                }
                if (mcdb[i].fValue)
                    didone = 1;
                ++i;
            }

        if (canone && !didone)
        {
            TRACE("Nice try, this is not going to work either\n");
            elem_callback(mmixer->lines[1].elem, SND_CTL_EVENT_MASK_VALUE);
            return MIXERR_INVALVALUE;
        }

        for (x = i = 0; x<mmixer->chans; ++x)
            if (line != x && mmixer->lines[x].dst == line)
            {
                if (mcdb[i].fValue)
                    for (chn = 0; chn <= SND_MIXER_SCHN_LAST; ++chn)
                    {
                        if (!snd_mixer_selem_has_capture_channel(mmixer->lines[x].elem, chn))
                            continue;
                        snd_mixer_selem_set_capture_switch(mmixer->lines[x].elem, chn, mcdb[i].fValue);
                    }
                ++i;
            }

        /* If it's a MUX, it means that only 1 channel can be selected
         * and the other channels are unselected
         *
         * For MIXER multiple sources are allowed, so unselect here
         */
        if (canone)
            break;

        for (x = i = 0; x<mmixer->chans; ++x)
            if (line != x && mmixer->lines[x].dst == line)
            {
                if (!mcdb[i].fValue)
                    for (chn = 0; chn <= SND_MIXER_SCHN_LAST; ++chn)
                    {
                        if (!snd_mixer_selem_has_capture_channel(mmixer->lines[x].elem, chn))
                            continue;
                        snd_mixer_selem_set_capture_switch(mmixer->lines[x].elem, chn, mcdb[i].fValue);
                    }
                ++i;
            }
        break;
    }
    default:
        FIXME("Unhandled type %s\n", getControlType(ct->c.dwControlType));
        return MMSYSERR_INVALPARAM;
    }
    return MMSYSERR_NOERROR;
}

/* Here we give info over the source/dest line given by dwSource+dwDest or dwDest, respectively
 * It is also possible that a line is found by componenttype or target type, latter is not implemented yet
 * Most important values returned in struct:
 * dwLineID
 * sz(Short)Name
 * line control count
 * amount of channels
 */
static DWORD MIX_GetLineInfo(UINT wDevID, LPMIXERLINEW Ml, DWORD_PTR flags)
{
    DWORD_PTR qf = flags & MIXER_GETLINEINFOF_QUERYMASK;
    mixer *mmixer = MIX_GetMix(wDevID);
    line *mline;
    int idx, i;

    if (!Ml)
    {
        WARN("No Ml\n");
        return MMSYSERR_INVALPARAM;
    }

    if (!mmixer)
    {
        WARN("Device %u not found\n", wDevID);
        return MMSYSERR_BADDEVICEID;
    }

    if (Ml->cbStruct != sizeof(*Ml))
    {
        WARN("invalid parameter: Ml->cbStruct = %d\n", Ml->cbStruct);
        return MMSYSERR_INVALPARAM;
    }

    Ml->fdwLine = MIXERLINE_LINEF_ACTIVE;
    Ml->dwUser  = 0;

    switch (qf)
    {
    case MIXER_GETLINEINFOF_COMPONENTTYPE:
    {
        Ml->dwLineID = 0xFFFF;
        for (idx = 0; idx < mmixer->chans; ++idx)
            if (mmixer->lines[idx].component == Ml->dwComponentType)
            {
                Ml->dwLineID = idx;
                break;
            }
        if (Ml->dwLineID == 0xFFFF)
            return MMSYSERR_KEYNOTFOUND;
        /* Now that we have lineid, fallback to lineid*/
    }

    case MIXER_GETLINEINFOF_LINEID:
        if (Ml->dwLineID < 0 || Ml->dwLineID >= mmixer->chans)
            return MIXERR_INVALLINE;

        TRACE("MIXER_GETLINEINFOF_LINEID %d\n", Ml->dwLineID);
        Ml->dwDestination = mmixer->lines[Ml->dwLineID].dst;

        if (Ml->dwDestination != Ml->dwLineID)
        {
            Ml->dwSource = getsrcfromline(mmixer, Ml->dwLineID);
            Ml->cConnections = 1;
        }
        else
        {
            Ml->cConnections = getsrccntfromchan(mmixer, Ml->dwLineID);
            Ml->dwSource = 0xFFFFFFFF;
        }
        TRACE("Connections %d, source %d\n", Ml->cConnections, Ml->dwSource);
        break;

    case MIXER_GETLINEINFOF_DESTINATION:
        if (Ml->dwDestination < 0 || Ml->dwDestination >= mmixer->dests)
        {
            WARN("dest %d out of bounds\n", Ml->dwDestination);
            return MIXERR_INVALLINE;
        }

        Ml->dwLineID = Ml->dwDestination;
        Ml->cConnections = getsrccntfromchan(mmixer, Ml->dwLineID);
        Ml->dwSource = 0xFFFFFFFF;
        break;

    case MIXER_GETLINEINFOF_SOURCE:
        if (Ml->dwDestination < 0 || Ml->dwDestination >= mmixer->dests)
        {
            WARN("dest %d for source out of bounds\n", Ml->dwDestination);
            return MIXERR_INVALLINE;
        }

        if (Ml->dwSource < 0 || Ml->dwSource >= getsrccntfromchan(mmixer, Ml->dwDestination))
        {
            WARN("src %d out of bounds\n", Ml->dwSource);
            return MIXERR_INVALLINE;
        }

        Ml->dwLineID = getsrclinefromchan(mmixer, Ml->dwDestination, Ml->dwSource);
        Ml->cConnections = 1;
        break;

    case MIXER_GETLINEINFOF_TARGETTYPE:
        FIXME("TODO: TARGETTYPE, stub\n");
        return MMSYSERR_INVALPARAM;

    default:
        FIXME("Unknown query flag: %08lx\n", qf);
        return MMSYSERR_INVALPARAM;
    }

    if (Ml->dwLineID >= mmixer->dests)
        Ml->fdwLine |= MIXERLINE_LINEF_SOURCE;

    mline = &mmixer->lines[Ml->dwLineID];
    Ml->dwComponentType = mline->component;
    Ml->cChannels = mmixer->lines[Ml->dwLineID].chans;
    Ml->cControls = 0;

    for (i=CONTROLSPERLINE*Ml->dwLineID;i<CONTROLSPERLINE*(Ml->dwLineID+1); ++i)
        if (mmixer->controls[i].enabled)
            ++(Ml->cControls);

    lstrcpynW(Ml->szShortName, mmixer->lines[Ml->dwLineID].name, sizeof(Ml->szShortName)/sizeof(WCHAR));
    lstrcpynW(Ml->szName, mmixer->lines[Ml->dwLineID].name, sizeof(Ml->szName)/sizeof(WCHAR));
    if (mline->capt)
        Ml->Target.dwType = MIXERLINE_TARGETTYPE_WAVEIN;
    else
        Ml->Target.dwType = MIXERLINE_TARGETTYPE_WAVEOUT;
    Ml->Target.dwDeviceID = 0xFFFFFFFF;
    Ml->Target.wMid = WINE_MIXER_MANUF_ID;
    Ml->Target.wPid = WINE_MIXER_PRODUCT_ID;
    Ml->Target.vDriverVersion = WINE_MIXER_VERSION;
    lstrcpynW(Ml->Target.szPname, mmixer->mixername, sizeof(Ml->Target.szPname)/sizeof(WCHAR));
    return MMSYSERR_NOERROR;
}

/* Get the controls that belong to a certain line, either all or 1 */
static DWORD MIX_GetLineControls(UINT wDevID, LPMIXERLINECONTROLSW mlc, DWORD_PTR flags)
{
    mixer *mmixer = MIX_GetMix(wDevID);
    int i,j = 0;
    DWORD ct;

    if (!mlc || mlc->cbStruct != sizeof(*mlc))
    {
        WARN("Invalid mlc %p, cbStruct: %d\n", mlc, (!mlc ? -1 : mlc->cbStruct));
        return MMSYSERR_INVALPARAM;
    }

    if (mlc->cbmxctrl != sizeof(MIXERCONTROLW))
    {
        WARN("cbmxctrl %d\n", mlc->cbmxctrl);
        return MMSYSERR_INVALPARAM;
    }

    if (!mmixer)
        return MMSYSERR_BADDEVICEID;

    flags &= MIXER_GETLINECONTROLSF_QUERYMASK;

    if (flags == MIXER_GETLINECONTROLSF_ONEBYID)
        mlc->dwLineID = mlc->u.dwControlID / CONTROLSPERLINE;

    if (mlc->dwLineID < 0 || mlc->dwLineID >= mmixer->chans)
    {
        TRACE("Invalid dwLineID %d\n", mlc->dwLineID);
        return MIXERR_INVALLINE;
    }

    switch (flags)
    {
    case MIXER_GETLINECONTROLSF_ALL:
       TRACE("line=%08x MIXER_GETLINECONTROLSF_ALL (%d)\n", mlc->dwLineID, mlc->cControls);
       for (i = 0; i < CONTROLSPERLINE; ++i)
           if (mmixer->controls[i+mlc->dwLineID * CONTROLSPERLINE].enabled)
           {
               memcpy(&mlc->pamxctrl[j], &mmixer->controls[i+mlc->dwLineID * CONTROLSPERLINE].c, sizeof(MIXERCONTROLW));
               TRACE("Added %s (%s)\n", debugstr_w(mlc->pamxctrl[j].szShortName), debugstr_w(mlc->pamxctrl[j].szName));
               ++j;
               if (j > mlc->cControls)
               {
                   WARN("invalid parameter\n");
                   return MMSYSERR_INVALPARAM;
               }
           }

        if (!j || mlc->cControls > j)
        {
            WARN("invalid parameter\n");
            return MMSYSERR_INVALPARAM;
        }
        break;
    case MIXER_GETLINECONTROLSF_ONEBYID:
        TRACE("line=%08x MIXER_GETLINECONTROLSF_ONEBYID (%x)\n", mlc->dwLineID, mlc->u.dwControlID);

        if (!mmixer->controls[mlc->u.dwControlID].enabled)
           return MIXERR_INVALCONTROL;

        mlc->pamxctrl[0] = mmixer->controls[mlc->u.dwControlID].c;
        break;
    case MIXER_GETLINECONTROLSF_ONEBYTYPE:
        TRACE("line=%08x MIXER_GETLINECONTROLSF_ONEBYTYPE (%s)\n", mlc->dwLineID, getControlType(mlc->u.dwControlType));

        ct = mlc->u.dwControlType & MIXERCONTROL_CT_CLASS_MASK;
        for (i = 0; i <= CONTROLSPERLINE; ++i)
        {
            const int ofs = i+mlc->dwLineID*CONTROLSPERLINE;
            if (i == CONTROLSPERLINE)
            {
                WARN("invalid parameter: control %s not found\n", getControlType(mlc->u.dwControlType));
                return MIXERR_INVALCONTROL;
            }
            if (mmixer->controls[ofs].enabled && (mmixer->controls[ofs].c.dwControlType & MIXERCONTROL_CT_CLASS_MASK) == ct)
            {
                mlc->pamxctrl[0] = mmixer->controls[ofs].c;
                break;
            }
        }
    break;
    default:
        FIXME("Unknown flag %08lx\n", flags & MIXER_GETLINECONTROLSF_QUERYMASK);
        return MMSYSERR_INVALPARAM;
    }

    return MMSYSERR_NOERROR;
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

    case MXDM_OPEN:
        ret = MIX_Open(wDevID, (LPMIXEROPENDESC) dwParam1, dwParam2); break;

    case MXDM_CLOSE:
        ret = MIX_Close(wDevID); break;

    case MXDM_GETDEVCAPS:
        ret = MIX_GetDevCaps(wDevID, (LPMIXERCAPS2W)dwParam1, dwParam2); break;

    case MXDM_GETLINEINFO:
        ret = MIX_GetLineInfo(wDevID, (LPMIXERLINEW)dwParam1, dwParam2); break;

    case MXDM_GETLINECONTROLS:
        ret = MIX_GetLineControls(wDevID, (LPMIXERLINECONTROLSW)dwParam1, dwParam2); break;

    case MXDM_GETCONTROLDETAILS:
        ret = MIX_GetControlDetails(wDevID, (LPMIXERCONTROLDETAILS)dwParam1, dwParam2); break;

    case MXDM_SETCONTROLDETAILS:
        ret = MIX_SetControlDetails(wDevID, (LPMIXERCONTROLDETAILS)dwParam1, dwParam2); break;

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
