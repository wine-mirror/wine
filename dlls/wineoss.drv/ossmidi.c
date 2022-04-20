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

NTSTATUS midi_out_fm_reset(void *args)
{
    WORD dev_id = (WORD)(UINT_PTR)args;
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

    return STATUS_SUCCESS;
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
        midi_out_fm_reset((void *)(UINT_PTR)dev_id);
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
    default:
        TRACE("Unsupported message\n");
        *params->err = MMSYSERR_NOTSUPPORTED;
    }

    return STATUS_SUCCESS;
}
