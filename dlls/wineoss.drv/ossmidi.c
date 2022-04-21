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

typedef struct sVoice
{
    int note; /* 0 means not used */
    int channel;
    unsigned cntMark : 30,
             status : 2;
#define sVS_UNUSED    0
#define sVS_PLAYING   1
#define sVS_SUSTAINED 2
} sVoice;

typedef struct sChannel
{
    int program;

    int bender;
    int benderRange;
    /* controllers */
    int bank;       /* CTL_BANK_SELECT */
    int volume;     /* CTL_MAIN_VOLUME */
    int balance;    /* CTL_BALANCE     */
    int expression; /* CTL_EXPRESSION  */
    int sustain;    /* CTL_SUSTAIN     */

    unsigned char nrgPmtMSB; /* Non register Parameters */
    unsigned char nrgPmtLSB;
    unsigned char regPmtMSB; /* Non register Parameters */
    unsigned char regPmtLSB;
} sChannel;

typedef struct sFMextra
{
    unsigned counter;
    int drumSetMask;
    sChannel channel[16]; /* MIDI has only 16 channels */
    sVoice voice[1]; /* dyn allocated according to sound card */
    /* do not append fields below voice[1] since the size of this structure
     * depends on the number of available voices on the FM synth...
     */
} sFMextra;

#define IS_DRUM_CHANNEL(_xtra, _chn) ((_xtra)->drumSetMask & (1 << (_chn)))

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

NTSTATUS midi_seq_open(void *args)
{
    struct midi_seq_open_params *params = args;

    if (!params->close)
        params->fd = seq_open();
    else
        seq_close(params->fd);

    return STATUS_SUCCESS;
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

/* FIXME: this is a bad idea, it's even not static... */
SEQ_DEFINEBUF(1024);

/* FIXME: this is not reentrant, not static - because of global variable
 * _seqbuf and al.
 */
/**************************************************************************
 *                     seqbuf_dump                             [internal]
 *
 * Used by SEQ_DUMPBUF to flush the buffer.
 *
 */
void seqbuf_dump(void)
{
    int fd;

    /* The device is already open, but there's no way to pass the
       fd to this function.  Rather than rely on a global variable
       we pretend to open the seq again. */
    fd = seq_open();
    if (_seqbufptr)
    {
        if (write(fd, _seqbuf, _seqbufptr) == -1)
        {
            WARN("Can't write data to sequencer %d, errno %d (%s)!\n",
                 fd, errno, strerror(errno));
        }
        /* FIXME: In any case buffer is lost so that if many errors occur the buffer
         * will not overrun */
        _seqbufptr = 0;
    }
    seq_close(fd);
}

extern const unsigned char midiFMInstrumentPatches[16 * 128];
extern const unsigned char midiFMDrumsPatches[16 * 128];

static int midi_out_fm_load(WORD dev_id, int fd)
{
    struct sbi_instrument sbi;
    int i;

    sbi.device = dev_id;
    sbi.key = FM_PATCH;

    memset(sbi.operators + 16, 0, 16);
    for (i = 0; i < 128; i++)
    {
        sbi.channel = i;
        memcpy(sbi.operators, midiFMInstrumentPatches + i * 16, 16);

        if (write(fd, &sbi, sizeof(sbi)) == -1)
        {
            WARN("Couldn't write patch for instrument %d, errno %d (%s)!\n", sbi.channel, errno, strerror(errno));
            return -1;
        }
    }
    for (i = 0; i < 128; i++)
    {
        sbi.channel = 128 + i;
        memcpy(sbi.operators, midiFMDrumsPatches + i * 16, 16);

        if (write(fd, &sbi, sizeof(sbi)) == -1)
        {
            WARN("Couldn't write patch for drum %d, errno %d (%s)!\n", sbi.channel, errno, strerror(errno));
            return -1;
        }
    }
    return 0;
}

static void midi_out_fm_reset(WORD dev_id)
{
    struct midi_dest *dest = dests + dev_id;
    sFMextra *extra = dest->lpExtra;
    sVoice *voice = extra->voice;
    sChannel *channel = extra->channel;
    int i;

    for (i = 0; i < dest->caps.wVoices; i++)
    {
        if (voice[i].status != sVS_UNUSED)
            SEQ_STOP_NOTE(dev_id, i, voice[i].note, 64);
        SEQ_KEY_PRESSURE(dev_id, i, 127, 0);
        SEQ_CONTROL(dev_id, i, SEQ_VOLMODE, VOL_METHOD_LINEAR);
        voice[i].note = 0;
        voice[i].channel = -1;
        voice[i].cntMark = 0;
        voice[i].status = sVS_UNUSED;
    }
    for (i = 0; i < 16; i++)
    {
        channel[i].program = 0;
        channel[i].bender = 8192;
        channel[i].benderRange = 2;
        channel[i].bank = 0;
        channel[i].volume = 127;
        channel[i].balance = 64;
        channel[i].expression = 0;
        channel[i].sustain = 0;
    }
    extra->counter = 0;
    extra->drumSetMask = 1 << 9; /* channel 10 is normally drums, sometimes 16 also */
    SEQ_DUMPBUF();
}

static void set_out_notify(struct notify_context *notify, struct midi_dest *dest, WORD dev_id, WORD msg,
                           UINT_PTR param_1, UINT_PTR param_2)
{
    notify->send_notify = TRUE;
    notify->dev_id = dev_id;
    notify->msg = msg;
    notify->param_1 = param_1;
    notify->param_2 = param_2;
    notify->callback = dest->midiDesc.dwCallback;
    notify->flags = dest->wFlags;
    notify->device = dest->midiDesc.hMidi;
    notify->instance = dest->midiDesc.dwInstance;
}

static UINT midi_out_open(WORD dev_id, MIDIOPENDESC *midi_desc, UINT flags, struct notify_context *notify)
{
    struct midi_dest *dest;
    int fd = -1;

    TRACE("(%04X, %p, %08X);\n", dev_id, midi_desc, flags);
    if (midi_desc == NULL)
    {
        WARN("Invalid Parameter !\n");
        return MMSYSERR_INVALPARAM;
    }
    if (dev_id >= num_dests)
    {
        TRACE("MAX_MIDIOUTDRV reached !\n");
        return MMSYSERR_BADDEVICEID;
    }
    dest = dests + dev_id;
    if (dest->midiDesc.hMidi != 0)
    {
        WARN("device already open !\n");
        return MMSYSERR_ALLOCATED;
    }
    if (!dest->bEnabled)
    {
        WARN("device disabled !\n");
        return MIDIERR_NODEVICE;
    }
    if ((flags & ~CALLBACK_TYPEMASK) != 0)
    {
        WARN("bad flags\n");
        return MMSYSERR_INVALFLAG;
    }

    dest->lpExtra = NULL;

    switch (dest->caps.wTechnology)
    {
    case MOD_FMSYNTH:
    {
        void *extra;

        extra = malloc(offsetof(struct sFMextra, voice[dest->caps.wVoices]));
        if (!extra)
        {
            WARN("can't alloc extra data !\n");
            return MMSYSERR_NOMEM;
        }
        dest->lpExtra = extra;
        fd = seq_open();
        if (fd < 0)
        {
            dest->lpExtra = NULL;
            free(extra);
            return MMSYSERR_ERROR;
        }
        if (midi_out_fm_load(dev_id, fd) < 0)
        {
            seq_close(fd);
            dest->lpExtra = NULL;
            free(extra);
            return MMSYSERR_ERROR;
        }
        midi_out_fm_reset(dev_id);
        break;
    }
    case MOD_MIDIPORT:
    case MOD_SYNTH:
        fd = seq_open();
        if (fd < 0)
            return MMSYSERR_ALLOCATED;
        break;
    default:
        WARN("Technology not supported (yet) %d !\n", dest->caps.wTechnology);
        return MMSYSERR_NOTENABLED;
    }

    dest->wFlags = HIWORD(flags & CALLBACK_TYPEMASK);

    dest->lpQueueHdr= NULL;
    dest->midiDesc = *midi_desc;
    dest->fd = fd;

    set_out_notify(notify, dest, dev_id, MOM_OPEN, 0, 0);
    TRACE("Successful !\n");
    return MMSYSERR_NOERROR;
}

static UINT midi_out_close(WORD dev_id, struct notify_context *notify)
{
    struct midi_dest *dest;

    TRACE("(%04X);\n", dev_id);

    if (dev_id >= num_dests)
    {
        TRACE("MAX_MIDIOUTDRV reached !\n");
        return MMSYSERR_BADDEVICEID;
    }
    dest = dests + dev_id;

    if (dest->midiDesc.hMidi == 0)
    {
        WARN("device not opened !\n");
        return MMSYSERR_ERROR;
    }
    /* FIXME: should test that no pending buffer is still in the queue for
     * playing */

    if (dest->fd == -1)
    {
        WARN("can't close !\n");
        return MMSYSERR_ERROR;
    }

    switch (dest->caps.wTechnology)
    {
    case MOD_FMSYNTH:
    case MOD_SYNTH:
    case MOD_MIDIPORT:
        seq_close(dest->fd);
        break;
    default:
        WARN("Technology not supported (yet) %d !\n", dest->caps.wTechnology);
        return MMSYSERR_NOTENABLED;
    }

    free(dest->lpExtra);
    dest->lpExtra = NULL;
    dest->fd = -1;

    set_out_notify(notify, dest, dev_id, MOM_CLOSE, 0, 0);
    dest->midiDesc.hMidi = 0;
    return MMSYSERR_NOERROR;
}

static UINT midi_out_fm_data(WORD dev_id, UINT data)
{
    struct midi_dest *dest = dests + dev_id;
    WORD evt = LOBYTE(LOWORD(data));
    WORD d1  = HIBYTE(LOWORD(data));
    WORD d2  = LOBYTE(HIWORD(data));
    sFMextra *extra = dest->lpExtra;
    sVoice *voice = extra->voice;
    sChannel *channel = extra->channel;
    int chn = (evt & 0x0F), i, nv;

    /* FIXME: chorus depth controller is not used */

    switch (evt & 0xF0)
    {
    case MIDI_NOTEOFF:
        for (i = 0; i < dest->caps.wVoices; i++)
        {
            /* don't stop sustained notes */
            if (voice[i].status == sVS_PLAYING && voice[i].channel == chn && voice[i].note == d1)
            {
                voice[i].status = sVS_UNUSED;
                SEQ_STOP_NOTE(dev_id, i, d1, d2);
            }
        }
        break;
    case MIDI_NOTEON:
        if (d2 == 0) /* note off if velocity == 0 */
        {
            for (i = 0; i < dest->caps.wVoices; i++) /* don't stop sustained notes */
            {
                if (voice[i].status == sVS_PLAYING && voice[i].channel == chn && voice[i].note == d1)
                {
                    voice[i].status = sVS_UNUSED;
                    SEQ_STOP_NOTE(dev_id, i, d1, 64);
                }
            }
            break;
        }
        /* finding out in this order :
         * - an empty voice
         * - if replaying the same note on the same channel
         * - the older voice (LRU)
         */
        for (i = nv = 0; i < dest->caps.wVoices; i++)
        {
            if (voice[i].status == sVS_UNUSED || (voice[i].note == d1 && voice[i].channel == chn))
            {
                nv = i;
                break;
            }
            if (voice[i].cntMark < voice[0].cntMark)
                nv = i;
        }
        TRACE("playing on voice=%d, pgm=%d, pan=0x%02X, vol=0x%02X, bender=0x%02X, note=0x%02X, vel=0x%02X\n",
              nv, channel[chn].program, channel[chn].balance, channel[chn].volume, channel[chn].bender, d1, d2);

        SEQ_SET_PATCH(dev_id, nv, IS_DRUM_CHANNEL(extra, chn) ?
                      (128 + d1) : channel[chn].program);
        SEQ_BENDER_RANGE(dev_id, nv, channel[chn].benderRange * 100);
        SEQ_BENDER(dev_id, nv, channel[chn].bender);
        SEQ_CONTROL(dev_id, nv, CTL_PAN, channel[chn].balance);
        SEQ_CONTROL(dev_id, nv, CTL_EXPRESSION, channel[chn].expression);
        SEQ_START_NOTE(dev_id, nv, d1, d2);
        voice[nv].status = channel[chn].sustain ? sVS_SUSTAINED : sVS_PLAYING;
        voice[nv].note = d1;
        voice[nv].channel = chn;
        voice[nv].cntMark = extra->counter++;
        break;
    case MIDI_KEY_PRESSURE:
        for (i = 0; i < dest->caps.wVoices; i++)
            if (voice[i].status != sVS_UNUSED && voice[i].channel == chn && voice[i].note == d1)
                SEQ_KEY_PRESSURE(dev_id, i, d1, d2);
        break;
    case MIDI_CTL_CHANGE:
        switch (d1)
        {
        case CTL_BANK_SELECT: channel[chn].bank = d2; break;
        case CTL_MAIN_VOLUME: channel[chn].volume = d2; break;
        case CTL_PAN:         channel[chn].balance = d2; break;
        case CTL_EXPRESSION:  channel[chn].expression = d2; break;
        case CTL_SUSTAIN:     channel[chn].sustain = d2;
            if (d2)
            {
                for (i = 0; i < dest->caps.wVoices; i++)
                    if (voice[i].status == sVS_PLAYING && voice[i].channel == chn)
                        voice[i].status = sVS_SUSTAINED;
            }
            else
            {
                for (i = 0; i < dest->caps.wVoices; i++)
                {
                    if (voice[i].status == sVS_SUSTAINED && voice[i].channel == chn)
                    {
                        voice[i].status = sVS_UNUSED;
                        SEQ_STOP_NOTE(dev_id, i, voice[i].note, 64);
                    }
                }
            }
            break;
        case CTL_NONREG_PARM_NUM_LSB: channel[chn].nrgPmtLSB = d2; break;
        case CTL_NONREG_PARM_NUM_MSB: channel[chn].nrgPmtMSB = d2; break;
        case CTL_REGIST_PARM_NUM_LSB: channel[chn].regPmtLSB = d2; break;
        case CTL_REGIST_PARM_NUM_MSB: channel[chn].regPmtMSB = d2; break;
        case CTL_DATA_ENTRY:
            switch ((channel[chn].regPmtMSB << 8) | channel[chn].regPmtLSB)
            {
            case 0x0000:
                if (channel[chn].benderRange != d2)
                {
                    channel[chn].benderRange = d2;
                    for (i = 0; i < dest->caps.wVoices; i++)
                        if (voice[i].channel == chn)
                            SEQ_BENDER_RANGE(dev_id, i, channel[chn].benderRange);
                }
                break;

            case 0x7F7F:
                channel[chn].benderRange = 2;
                for (i = 0; i < dest->caps.wVoices; i++)
                    if (voice[i].channel == chn)
                        SEQ_BENDER_RANGE(dev_id, i, channel[chn].benderRange);
                break;
            default:
                TRACE("Data entry: regPmt=0x%02x%02x, nrgPmt=0x%02x%02x with %x\n",
                      channel[chn].regPmtMSB, channel[chn].regPmtLSB,
                      channel[chn].nrgPmtMSB, channel[chn].nrgPmtLSB, d2);
                break;
            }
            break;

        case 0x78: /* all sounds off */
            /* FIXME: I don't know if I have to take care of the channel for this control? */
            for (i = 0; i < dest->caps.wVoices; i++)
            {
                if (voice[i].status != sVS_UNUSED && voice[i].channel == chn)
                {
                    voice[i].status = sVS_UNUSED;
                    SEQ_STOP_NOTE(dev_id, i, voice[i].note, 64);
                }
            }
            break;
        case 0x7B: /* all notes off */
            /* FIXME: I don't know if I have to take care of the channel for this control? */
            for (i = 0; i < dest->caps.wVoices; i++)
            {
                if (voice[i].status == sVS_PLAYING && voice[i].channel == chn)
                {
                    voice[i].status = sVS_UNUSED;
                    SEQ_STOP_NOTE(dev_id, i, voice[i].note, 64);
                }
            }
            break;
        default:
            TRACE("Dropping MIDI control event 0x%02x(%02x) on channel %d\n", d1, d2, chn);
            break;
        }
        break;
    case MIDI_PGM_CHANGE:
        channel[chn].program = d1;
        break;
    case MIDI_CHN_PRESSURE:
        for (i = 0; i < dest->caps.wVoices; i++)
            if (voice[i].status != sVS_UNUSED && voice[i].channel == chn)
                SEQ_KEY_PRESSURE(dev_id, i, voice[i].note, d1);

        break;
    case MIDI_PITCH_BEND:
        channel[chn].bender = (d2 << 7) + d1;
        for (i = 0; i < dest->caps.wVoices; i++)
            if (voice[i].channel == chn)
                SEQ_BENDER(dev_id, i, channel[chn].bender);
        break;
    case MIDI_SYSTEM_PREFIX:
        switch (evt & 0x0F)
        {
        case 0x0F: /* Reset */
            midi_out_fm_reset(dev_id);
            break;
        default:
            WARN("Unsupported (yet) system event %02x\n", evt & 0x0F);
        }
        break;
    default:
        WARN("Internal error, shouldn't happen (event=%08x)\n", evt & 0xF0);
        return MMSYSERR_NOTENABLED;
    }

    SEQ_DUMPBUF();
    return MMSYSERR_NOERROR;
}

static UINT midi_out_port_data(WORD dev_id, UINT data)
{
    WORD evt = LOBYTE(LOWORD(data));
    WORD d1  = HIBYTE(LOWORD(data));
    WORD d2  = LOBYTE(HIWORD(data));
    int dev = dev_id - num_synths;

    if (dev < 0)
    {
        WARN("Internal error on devID (%u) !\n", dev_id);
        return MIDIERR_NODEVICE;
    }

    switch (evt & 0xF0)
    {
    case MIDI_NOTEOFF:
    case MIDI_NOTEON:
    case MIDI_KEY_PRESSURE:
    case MIDI_CTL_CHANGE:
    case MIDI_PITCH_BEND:
        SEQ_MIDIOUT(dev, evt);
        SEQ_MIDIOUT(dev, d1);
        SEQ_MIDIOUT(dev, d2);
        break;
    case MIDI_PGM_CHANGE:
    case MIDI_CHN_PRESSURE:
        SEQ_MIDIOUT(dev, evt);
        SEQ_MIDIOUT(dev, d1);
        break;
    case MIDI_SYSTEM_PREFIX:
        switch (evt & 0x0F)
        {
        case 0x00: /* System Exclusive, don't do it on MODM_DATA, should require MODM_LONGDATA */
        case 0x04: /* Undefined. */
        case 0x05: /* Undefined. */
        case 0x07: /* End of Exclusive. */
        case 0x09: /* Undefined. */
        case 0x0D: /* Undefined. */
            break;
        case 0x06: /* Tune Request */
        case 0x08: /* Timing Clock. */
        case 0x0A: /* Start. */
        case 0x0B: /* Continue */
        case 0x0C: /* Stop */
        case 0x0E: /* Active Sensing. */
            SEQ_MIDIOUT(dev, evt);
            break;
        case 0x0F: /* Reset */
            SEQ_MIDIOUT(dev, MIDI_SYSTEM_PREFIX);
            SEQ_MIDIOUT(dev, 0x7e);
            SEQ_MIDIOUT(dev, 0x7f);
            SEQ_MIDIOUT(dev, 0x09);
            SEQ_MIDIOUT(dev, 0x01);
            SEQ_MIDIOUT(dev, 0xf7);
            break;
        case 0x01: /* MTC Quarter frame */
        case 0x03: /* Song Select. */
            SEQ_MIDIOUT(dev, evt);
            SEQ_MIDIOUT(dev, d1);
        case 0x02: /* Song Position Pointer. */
            SEQ_MIDIOUT(dev, evt);
            SEQ_MIDIOUT(dev, d1);
            SEQ_MIDIOUT(dev, d2);
        }
        break;
    }

    SEQ_DUMPBUF();
    return MMSYSERR_NOERROR;
}

static UINT midi_out_data(WORD dev_id, UINT data)
{
    struct midi_dest *dest;

    TRACE("(%04X, %08X);\n", dev_id, data);

    if (dev_id >= num_dests) return MMSYSERR_BADDEVICEID;
    dest = dests + dev_id;
    if (!dest->bEnabled) return MIDIERR_NODEVICE;

    if (dest->fd == -1)
    {
        WARN("can't play !\n");
        return MIDIERR_NODEVICE;
    }
    switch (dest->caps.wTechnology)
    {
    case MOD_FMSYNTH:
        return midi_out_fm_data(dev_id, data);
    case MOD_MIDIPORT:
        return midi_out_port_data(dev_id, data);
    }

    WARN("Technology not supported (yet) %d !\n", dest->caps.wTechnology);
    return MMSYSERR_NOTENABLED;
}

static UINT midi_out_long_data(WORD dev_id, MIDIHDR *hdr, UINT hdr_size, struct notify_context *notify)
{
    struct midi_dest *dest;
    BYTE *data;
    unsigned int count;

    TRACE("(%04X, %p, %08X);\n", dev_id, hdr, hdr_size);

    /* Note: MS doc does not say much about the dwBytesRecorded member of the MIDIHDR structure
     * but it seems to be used only for midi input.
     * Taking a look at the WAVEHDR structure (which is quite similar) confirms this assumption.
     */

    if (dev_id >= num_dests) return MMSYSERR_BADDEVICEID;
    dest = dests + dev_id;
    if (!dest->bEnabled) return MIDIERR_NODEVICE;

    if (dest->fd == -1)
    {
        WARN("can't play !\n");
        return MIDIERR_NODEVICE;
    }

    data = (BYTE *)hdr->lpData;

    if (data == NULL)
        return MIDIERR_UNPREPARED;
    if (!(hdr->dwFlags & MHDR_PREPARED))
        return MIDIERR_UNPREPARED;
    if (hdr->dwFlags & MHDR_INQUEUE)
        return MIDIERR_STILLPLAYING;
    hdr->dwFlags &= ~MHDR_DONE;
    hdr->dwFlags |= MHDR_INQUEUE;

    /* FIXME: MS doc is not 100% clear. Will lpData only contain system exclusive
     * data, or can it also contain raw MIDI data, to be split up and sent to
     * modShortData() ?
     * If the latter is true, then the following WARNing will fire up
     */
    if (data[0] != 0xF0 || data[hdr->dwBufferLength - 1] != 0xF7)
        WARN("The allegedly system exclusive buffer is not correct\n\tPlease report with MIDI file\n");

    TRACE("dwBufferLength=%u !\n", hdr->dwBufferLength);
    TRACE("                 %02X %02X %02X ... %02X %02X %02X\n",
          data[0], data[1], data[2], data[hdr->dwBufferLength - 3],
          data[hdr->dwBufferLength - 2], data[hdr->dwBufferLength - 1]);

    switch (dest->caps.wTechnology)
    {
    case MOD_FMSYNTH:
        /* FIXME: I don't think there is much to do here */
        break;
    case MOD_MIDIPORT:
        if (data[0] != 0xF0)
        {
            /* Send end of System Exclusive */
            SEQ_MIDIOUT(dev_id - num_synths, 0xF0);
            WARN("Adding missing 0xF0 marker at the beginning of system exclusive byte stream\n");
        }
        for (count = 0; count < hdr->dwBufferLength; count++)
            SEQ_MIDIOUT(dev_id - num_synths, data[count]);
        if (data[count - 1] != 0xF7)
        {
            /* Send end of System Exclusive */
            SEQ_MIDIOUT(dev_id - num_synths, 0xF7);
            WARN("Adding missing 0xF7 marker at the end of system exclusive byte stream\n");
        }
        SEQ_DUMPBUF();
        break;
    default:
        WARN("Technology not supported (yet) %d !\n", dest->caps.wTechnology);
        return MMSYSERR_NOTENABLED;
    }

    hdr->dwFlags &= ~MHDR_INQUEUE;
    hdr->dwFlags |= MHDR_DONE;
    set_out_notify(notify, dest, dev_id, MOM_DONE, (UINT_PTR)hdr, 0);
    return MMSYSERR_NOERROR;
}

static UINT midi_out_prepare(WORD dev_id, MIDIHDR *hdr, UINT hdr_size)
{
    TRACE("(%04X, %p, %d);\n", dev_id, hdr, hdr_size);

    if (hdr_size < offsetof(MIDIHDR, dwOffset) || !hdr || !hdr->lpData)
        return MMSYSERR_INVALPARAM;
    if (hdr->dwFlags & MHDR_PREPARED)
        return MMSYSERR_NOERROR;

    hdr->lpNext = 0;
    hdr->dwFlags |= MHDR_PREPARED;
    hdr->dwFlags &= ~(MHDR_DONE | MHDR_INQUEUE);
    return MMSYSERR_NOERROR;
}

static UINT midi_out_unprepare(WORD dev_id, MIDIHDR *hdr, UINT hdr_size)
{
    TRACE("(%04X, %p, %d);\n", dev_id, hdr, hdr_size);

    if (hdr_size < offsetof(MIDIHDR, dwOffset) || !hdr || !hdr->lpData)
        return MMSYSERR_INVALPARAM;
    if (!(hdr->dwFlags & MHDR_PREPARED))
        return MMSYSERR_NOERROR;
    if (hdr->dwFlags & MHDR_INQUEUE)
        return MIDIERR_STILLPLAYING;

    hdr->dwFlags &= ~MHDR_PREPARED;
    return MMSYSERR_NOERROR;
}

NTSTATUS midi_out_message(void *args)
{
    struct midi_out_message_params *params = args;

    params->notify->send_notify = FALSE;

    switch (params->msg)
    {
    case DRVM_ENABLE:
    case DRVM_DISABLE:
        /* FIXME: Pretend this is supported */
        *params->err = MMSYSERR_NOERROR;
        break;
    case MODM_OPEN:
        *params->err = midi_out_open(params->dev_id, (MIDIOPENDESC *)params->param_1, params->param_2, params->notify);
        break;
    case MODM_CLOSE:
        *params->err = midi_out_close(params->dev_id, params->notify);
        break;
    case MODM_DATA:
        *params->err = midi_out_data(params->dev_id, params->param_1);
        break;
    case MODM_LONGDATA:
        *params->err = midi_out_long_data(params->dev_id, (MIDIHDR *)params->param_1, params->param_2, params->notify);
        break;
    case MODM_PREPARE:
        *params->err = midi_out_prepare(params->dev_id, (MIDIHDR *)params->param_1, params->param_2);
        break;
    case MODM_UNPREPARE:
        *params->err = midi_out_unprepare(params->dev_id, (MIDIHDR *)params->param_1, params->param_2);
        break;
    default:
        TRACE("Unsupported message\n");
        *params->err = MMSYSERR_NOTSUPPORTED;
    }

    return STATUS_SUCCESS;
}
