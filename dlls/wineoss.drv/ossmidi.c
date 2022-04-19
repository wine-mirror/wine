/*
 * MIDI driver for OSS (unixlib)
 *
 * Copyright 1994       Martin Ayotte
 * Copyright 1998       Luiz Otavio L. Zorzella (init procedures)
 * Copyright 1998, 1999 Eric POUECH
 * Copyright 2022       Huw Davies
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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"
#include "audioclient.h"

#include "wine/debug.h"
#include "wine/unixlib.h"

#include "unixlib.h"

static unsigned int num_dests, num_srcs, num_synths, seq_refs;
static struct midi_dest dests[MAX_MIDIOUTDRV];
static struct midi_src srcs[MAX_MIDIINDRV];

WINE_DEFAULT_DEBUG_CHANNEL(midi);

static int oss_to_win_device_type(int type)
{
    /* MOD_MIDIPORT     output port
     * MOD_SYNTH        generic internal synth
     * MOD_SQSYNTH      square wave internal synth
     * MOD_FMSYNTH      FM internal synth
     * MOD_MAPPER       MIDI mapper
     * MOD_WAVETABLE    hardware wavetable internal synth
     * MOD_SWSYNTH      software internal synth
     */

    /* FIXME Is this really the correct equivalence from UNIX to
       Windows Sound type */

    switch (type)
    {
    case SYNTH_TYPE_FM:     return MOD_FMSYNTH;
    case SYNTH_TYPE_SAMPLE: return MOD_SYNTH;
    case SYNTH_TYPE_MIDI:   return MOD_MIDIPORT;
    default:
        ERR("Cannot determine the type of this midi device. "
            "Assuming FM Synth\n");
        return MOD_FMSYNTH;
    }
}

static int seq_open(void)
{
    static int midi_warn = 1;
    static int fd = -1;

    if (seq_refs <= 0)
    {
        const char* device = getenv("MIDIDEV");

        if (!device) device = "/dev/sequencer";
        fd = open(device, O_RDWR, 0);
        if (fd == -1)
        {
            if (midi_warn)
            {
                WARN("Can't open MIDI device '%s' ! (%s). If your program needs this (probably not): %s\n",
                     device, strerror(errno),
                     errno == ENOENT ? "create it ! (\"man MAKEDEV\" ?)" :
                     errno == ENODEV ? "load MIDI sequencer kernel driver !" :
                     errno == EACCES ? "grant access ! (\"man chmod\")" : "");
            }
            midi_warn = 0;
            return -1;
        }
        fcntl(fd, F_SETFD, 1); /* set close on exec flag */
        ioctl(fd, SNDCTL_SEQ_RESET);
    }
    seq_refs++;
    return fd;
}

static int seq_close(int fd)
{
    if (--seq_refs == 0)
        close(fd);

    return 0;
}

NTSTATUS midi_init(void *args)
{
    struct midi_init_params *params = args;
    int i, status, synth_devs = 255, midi_devs = 255, fd, len;
    struct synth_info sinfo;
    struct midi_info minfo;
    struct midi_dest *dest;
    struct midi_src *src;

    /* try to open device */
    fd = seq_open();
    if (fd == -1)
    {
        *params->err = -1;
        return STATUS_SUCCESS;
    }

    /* find how many Synth devices are there in the system */
    status = ioctl(fd, SNDCTL_SEQ_NRSYNTHS, &synth_devs);
    if (status == -1)
    {
        ERR("ioctl for nr synth failed.\n");
        seq_close(fd);
        *params->err = -1;
        return STATUS_SUCCESS;
    }

    if (synth_devs > MAX_MIDIOUTDRV)
    {
        ERR("MAX_MIDIOUTDRV (%d) was enough for the number of devices (%d). "
            "Some FM devices will not be available.\n", MAX_MIDIOUTDRV, synth_devs);
        synth_devs = MAX_MIDIOUTDRV;
    }

    for (i = 0, dest = dests; i < synth_devs; i++, dest++)
    {
        /* Manufac ID. We do not have access to this with soundcard.h
         * Does not seem to be a problem, because in mmsystem.h only
         * Microsoft's ID is listed.
         */
        dest->caps.wMid = 0x00FF;
        dest->caps.wPid = 0x0001; /* FIXME Product ID  */
        /* Product Version. We simply say "1" */
        dest->caps.vDriverVersion = 0x001;
        /* The following are mandatory for MOD_MIDIPORT */
        dest->caps.wChannelMask = 0xFFFF;
        dest->caps.wVoices = 0;
        dest->caps.wNotes = 0;
        dest->caps.dwSupport = 0;

        sinfo.device = i;
        status = ioctl(fd, SNDCTL_SYNTH_INFO, &sinfo);
        if (status == -1)
        {
            char buf[255];

            ERR("ioctl for synth info failed on %d, disabling it.\n", i);

            sprintf(buf, "Wine OSS Midi Out #%d disabled", i);
            len = ntdll_umbstowcs(buf, strlen(buf) + 1, dest->caps.szPname, ARRAY_SIZE(dest->caps.szPname));
            dest->caps.szPname[len - 1] = '\0';
            dest->caps.wTechnology = MOD_MIDIPORT;
            dest->bEnabled = FALSE;
        }
        else
        {
            len = ntdll_umbstowcs(sinfo.name, strlen(sinfo.name) + 1, dest->caps.szPname, ARRAY_SIZE(dest->caps.szPname));
            dest->caps.szPname[len - 1] = '\0';
            dest->caps.wTechnology = oss_to_win_device_type(sinfo.synth_type);

            if (dest->caps.wTechnology != MOD_MIDIPORT)
            {
                /* FIXME Do we have this information?
                 * Assuming the soundcards can handle
                 * MIDICAPS_VOLUME and MIDICAPS_LRVOLUME but
                 * not MIDICAPS_CACHE.
                 */
                dest->caps.dwSupport = MIDICAPS_VOLUME | MIDICAPS_LRVOLUME;
                dest->caps.wVoices = sinfo.nr_voices;

                /* FIXME Is it possible to know the maximum
                 * number of simultaneous notes of a soundcard ?
                 * I believe we don't have this information, but
                 * it's probably equal or more than wVoices
                 */
                dest->caps.wNotes = sinfo.nr_voices;
            }
            dest->bEnabled = TRUE;

            /* We also have the information sinfo.synth_subtype, not used here
             */
            if (sinfo.capabilities & SYNTH_CAP_INPUT)
                FIXME("Synthesizer supports MIDI in. Not yet supported.\n");

            TRACE("SynthOut[%d]\tOSS info: synth type=%d/%d capa=%lx\n",
                  i, sinfo.synth_type, sinfo.synth_subtype, (long)sinfo.capabilities);
        }

        TRACE("SynthOut[%d]\tname='%s' techn=%d voices=%d notes=%d chnMsk=%04x support=%d\n",
              i, wine_dbgstr_w(dest->caps.szPname), dest->caps.wTechnology,
              dest->caps.wVoices, dest->caps.wNotes, dest->caps.wChannelMask, dest->caps.dwSupport);
    }

    /* find how many MIDI devices are there in the system */
    status = ioctl(fd, SNDCTL_SEQ_NRMIDIS, &midi_devs);
    if (status == -1)
    {
        ERR("ioctl on nr midi failed.\n");
        midi_devs = 0;
        goto wrapup;
    }

    /* FIXME: the two restrictions below could be loosened in some cases */
    if (synth_devs + midi_devs > MAX_MIDIOUTDRV)
    {
        ERR("MAX_MIDIOUTDRV was not enough for the number of devices. "
            "Some MIDI devices will not be available.\n");
        midi_devs = MAX_MIDIOUTDRV - synth_devs;
    }

    if (midi_devs > MAX_MIDIINDRV)
    {
        ERR("MAX_MIDIINDRV (%d) was not enough for the number of devices (%d). "
            "Some MIDI devices will not be available.\n", MAX_MIDIINDRV, midi_devs);
        midi_devs = MAX_MIDIINDRV;
    }

    dest = dests + synth_devs;
    src = srcs;
    for (i = 0; i < midi_devs; i++, dest++, src++)
    {
        minfo.device = i;
        status = ioctl(fd, SNDCTL_MIDI_INFO, &minfo);
        if (status == -1) WARN("ioctl on midi info for device %d failed.\n", i);

        /* Manufacturer ID. We do not have access to this with soundcard.h
           Does not seem to be a problem, because in mmsystem.h only Microsoft's ID is listed
        */
        dest->caps.wMid = 0x00FF;
        dest->caps.wPid = 0x0001; /* FIXME Product ID */
        /* Product Version. We simply say "1" */
        dest->caps.vDriverVersion = 0x001;
        if (status == -1)
        {
            char buf[255];

            sprintf(buf, "Wine OSS Midi Out #%d disabled", synth_devs + i);
            len = ntdll_umbstowcs(buf, strlen(buf) + 1, dest->caps.szPname, ARRAY_SIZE(dest->caps.szPname));
            dest->caps.szPname[len - 1] = '\0';
            dest->bEnabled = FALSE;
        }
        else
        {
            len = ntdll_umbstowcs(minfo.name, strlen(minfo.name) + 1, dest->caps.szPname, ARRAY_SIZE(dest->caps.szPname));
            dest->caps.szPname[len - 1] = '\0';
            dest->bEnabled = TRUE;
        }
        dest->caps.wTechnology = MOD_MIDIPORT;
        dest->caps.wVoices = 0;
        dest->caps.wNotes = 0;
        dest->caps.wChannelMask = 0xFFFF;
        dest->caps.dwSupport = 0;

        /* Manufac ID. We do not have access to this with soundcard.h
           Does not seem to be a problem, because in mmsystem.h only
           Microsoft's ID is listed */
        src->caps.wMid = 0x00FF;
        src->caps.wPid = 0x0001; /* FIXME Product ID */
        /* Product Version. We simply say "1" */
        src->caps.vDriverVersion = 0x001;
        if (status == -1)
        {
            char buf[ARRAY_SIZE(dest->caps.szPname)];

            sprintf(buf, "Wine OSS Midi In #%d disabled", synth_devs + i);
            len = ntdll_umbstowcs(buf, strlen(buf) + 1, src->caps.szPname, ARRAY_SIZE(src->caps.szPname));
            src->caps.szPname[len - 1] = '\0';
            src->state = -1;
        }
        else
        {
            len = ntdll_umbstowcs(minfo.name, strlen(minfo.name) + 1, src->caps.szPname, ARRAY_SIZE(src->caps.szPname));
            src->caps.szPname[len - 1] = '\0';
            src->state = 0;
        }
        src->caps.dwSupport = 0; /* mandatory with MIDIINCAPS */

        TRACE("OSS info: midi[%d] dev-type=%d capa=%lx\n"
              "\tMidiOut[%d] name='%s' techn=%d voices=%d notes=%d chnMsk=%04x support=%d\n"
              "\tMidiIn [%d] name='%s' support=%d\n",
              i, minfo.dev_type, (long)minfo.capabilities,
              synth_devs + i, wine_dbgstr_w(dest->caps.szPname), dest->caps.wTechnology,
              dest->caps.wVoices, dest->caps.wNotes, dest->caps.wChannelMask, dest->caps.dwSupport,
              i, wine_dbgstr_w(src->caps.szPname), src->caps.dwSupport);
    }

wrapup:
    /* windows does not seem to differentiate Synth from MIDI devices */
    num_synths = synth_devs;
    num_dests = synth_devs + midi_devs;
    num_srcs = midi_devs;

    /* close file and exit */
    seq_close(fd);

    *params->err = 0;
    params->num_srcs = num_srcs;
    params->num_dests = num_dests;
    params->num_synths = num_synths;
    params->srcs = srcs;
    params->dests = dests;

    return STATUS_SUCCESS;
}
