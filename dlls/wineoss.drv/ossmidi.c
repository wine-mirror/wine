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
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <pthread.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"
#include "audioclient.h"
#include "mmddk.h"

#include "wine/debug.h"
#include "wine/unixlib.h"

#include "unixlib.h"

struct midi_dest
{
    BOOL                bEnabled;
    MIDIOPENDESC        midiDesc;
    BYTE                runningStatus;
    WORD                wFlags;
    MIDIHDR            *lpQueueHdr;
    void               *lpExtra; /* according to port type (MIDI, FM...), extra data when needed */
    MIDIOUTCAPSW        caps;
    int                 fd;
};

struct midi_src
{
    int                 state; /* -1 disabled, 0 is no recording started, 1 in recording, bit 2 set if in sys exclusive recording */
    MIDIOPENDESC        midiDesc;
    WORD                wFlags;
    MIDIHDR            *lpQueueHdr;
    unsigned char       incoming[3];
    unsigned char       incPrev;
    char                incLen;
    UINT                startTime;
    MIDIINCAPSW         caps;
    int                 fd;
};

static pthread_mutex_t in_buffer_mutex = PTHREAD_MUTEX_INITIALIZER;

static unsigned int num_dests, num_srcs, num_synths, seq_refs;
static struct midi_dest *dests;
static struct midi_src *srcs;
static int load_count;

static unsigned int num_midi_in_started;
static int rec_cancel_pipe[2];
static pthread_t rec_thread_id;

static pthread_mutex_t notify_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t notify_read_cond = PTHREAD_COND_INITIALIZER;
static pthread_cond_t notify_write_cond = PTHREAD_COND_INITIALIZER;
static BOOL notify_quit;
#define NOTIFY_BUFFER_SIZE 64 + 1 /* + 1 for the sentinel */
static struct notify_context notify_buffer[NOTIFY_BUFFER_SIZE];
static struct notify_context *notify_read = notify_buffer, *notify_write = notify_buffer;

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

static void in_buffer_lock(void)
{
    pthread_mutex_lock(&in_buffer_mutex);
}

static void in_buffer_unlock(void)
{
    pthread_mutex_unlock(&in_buffer_mutex);
}

static uint64_t get_time_msec(void)
{
    struct timespec now = {0, 0};

#ifdef CLOCK_MONOTONIC_RAW
    if (!clock_gettime(CLOCK_MONOTONIC_RAW, &now))
        return (uint64_t)now.tv_sec * 1000 + now.tv_nsec / 1000000;
#endif
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (uint64_t)now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

/*
 * notify buffer: The notification ring buffer is implemented so that
 * there is always at least one unused sentinel before the current
 * read position in order to allow detection of the full vs empty
 * state.
 */
static struct notify_context *notify_buffer_next(struct notify_context *notify)
{
    if (++notify >= notify_buffer + ARRAY_SIZE(notify_buffer))
        notify = notify_buffer;

    return notify;
}

static BOOL notify_buffer_empty(void)
{
    return notify_read == notify_write;
}

static BOOL notify_buffer_full(void)
{
    return notify_buffer_next(notify_write) == notify_read;
}

static BOOL notify_buffer_add(struct notify_context *notify)
{
    if (notify_buffer_full()) return FALSE;

    *notify_write = *notify;
    notify_write = notify_buffer_next(notify_write);
    return TRUE;
}

static BOOL notify_buffer_remove(struct notify_context *notify)
{
    if (notify_buffer_empty()) return FALSE;

    *notify = *notify_read;
    notify_read = notify_buffer_next(notify_read);
    return TRUE;
}

static void notify_post(struct notify_context *notify)
{
    pthread_mutex_lock(&notify_mutex);

    if (notify)
    {
        while (notify_buffer_full())
            pthread_cond_wait(&notify_write_cond, &notify_mutex);

        notify_buffer_add(notify);
    }
    else notify_quit = TRUE;
    pthread_cond_signal(&notify_read_cond);

    pthread_mutex_unlock(&notify_mutex);
}

static void set_in_notify(struct notify_context *notify, struct midi_src *src, WORD dev_id, WORD msg,
                          UINT_PTR param_1, UINT_PTR param_2)
{
    notify->send_notify = TRUE;
    notify->dev_id = dev_id;
    notify->msg = msg;
    notify->param_1 = param_1;
    notify->param_2 = param_2;
    notify->callback = src->midiDesc.dwCallback;
    notify->flags = src->wFlags;
    notify->device = src->midiDesc.hMidi;
    notify->instance = src->midiDesc.dwInstance;
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

static UINT oss_midi_init(void)
{
    int i, status, synth_devs = 255, midi_devs = 255, fd, len;
    struct synth_info sinfo;
    struct midi_info minfo;
    struct midi_dest *dest;
    struct midi_src *src;

    TRACE("(%i)\n", load_count);

    if (load_count++)
        return 1;

    /* try to open device */
    fd = seq_open();
    if (fd == -1)
        return -1;

    /* find how many Synth devices are there in the system */
    status = ioctl(fd, SNDCTL_SEQ_NRSYNTHS, &synth_devs);
    if (status == -1)
    {
        ERR("ioctl for nr synth failed.\n");
        seq_close(fd);
        return -1;
    }
    /* find how many MIDI devices are there in the system */
    status = ioctl(fd, SNDCTL_SEQ_NRMIDIS, &midi_devs);
    if (status == -1)
    {
        ERR("ioctl on nr midi failed.\n");
        midi_devs = 0;
    }

    /* windows does not seem to differentiate Synth from MIDI devices */
    num_synths = synth_devs;
    num_dests = synth_devs + midi_devs;
    num_srcs = midi_devs;

    srcs = calloc( num_srcs, sizeof(*srcs) );
    dests = calloc( num_dests, sizeof(*dests) );

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

            TRACE("SynthOut[%d]\tOSS info: synth type=%d/%d capa=%x\n",
                  i, sinfo.synth_type, sinfo.synth_subtype, (unsigned)sinfo.capabilities);
        }

        TRACE("SynthOut[%d]\tname='%s' techn=%d voices=%d notes=%d chnMsk=%04x support=%d\n",
              i, wine_dbgstr_w(dest->caps.szPname), dest->caps.wTechnology,
              dest->caps.wVoices, dest->caps.wNotes, dest->caps.wChannelMask,
              (unsigned)dest->caps.dwSupport);
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

        TRACE("OSS info: midi[%d] dev-type=%d capa=%x\n"
              "\tMidiOut[%d] name='%s' techn=%d voices=%d notes=%d chnMsk=%04x support=%d\n"
              "\tMidiIn [%d] name='%s' support=%d\n",
              i, minfo.dev_type, (unsigned)minfo.capabilities,
              synth_devs + i, wine_dbgstr_w(dest->caps.szPname), dest->caps.wTechnology,
              dest->caps.wVoices, dest->caps.wNotes, dest->caps.wChannelMask, (unsigned)dest->caps.dwSupport,
              i, wine_dbgstr_w(src->caps.szPname), (unsigned)src->caps.dwSupport);
    }

    /* close file and exit */
    seq_close(fd);

    return 0;
}

static UINT midi_exit(void)
{
    TRACE("(%i)\n", load_count);

    if (--load_count)
        return 1;

    free( srcs );
    free( dests );
    return 0;
}

NTSTATUS oss_midi_release(void *args)
{
    /* stop the notify_wait thread */
    notify_post(NULL);

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
    if (dev_id >= num_dests) return MMSYSERR_BADDEVICEID;
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

    dest->runningStatus = 0;
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

    if (dev_id >= num_dests) return MMSYSERR_BADDEVICEID;
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
    BYTE evt = LOBYTE(LOWORD(data)), d1, d2;
    sFMextra *extra = dest->lpExtra;
    sVoice *voice = extra->voice;
    sChannel *channel = extra->channel;
    int chn = (evt & 0x0F), i, nv;

    if (evt & 0x80)
    {
        d1 = HIBYTE(LOWORD(data));
        d2 = LOBYTE(HIWORD(data));
        if (evt < 0xF0)
            dest->runningStatus = evt;
    }
    else if (dest->runningStatus)
    {
        evt = dest->runningStatus;
        d1 = LOBYTE(LOWORD(data));
        d2 = HIBYTE(LOWORD(data));
    }
    else
    {
        FIXME("ooch %x\n", data);
        return MMSYSERR_NOERROR;
    }

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
            dest->runningStatus = 0;
            break;
        default:
            WARN("Unsupported (yet) system event %02x\n", evt & 0x0F);
        }
        if (evt <= 0xF7)
            dest->runningStatus = 0;
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
    struct midi_dest *dest = dests + dev_id;
    BYTE evt = LOBYTE(LOWORD(data)), d1, d2;
    int dev = dev_id - num_synths;

    if (dev < 0)
    {
        WARN("Internal error on devID (%u) !\n", dev_id);
        return MIDIERR_NODEVICE;
    }

    if (evt & 0x80)
    {
        d1 = HIBYTE(LOWORD(data));
        d2 = LOBYTE(HIWORD(data));
    }
    else if (dest->runningStatus)
    {
        evt = dest->runningStatus;
        d1 = LOBYTE(LOWORD(data));
        d2 = HIBYTE(LOWORD(data));
    }
    else
    {
        FIXME("ooch %x\n", data);
        return MMSYSERR_NOERROR;
    }

    switch (evt & 0xF0)
    {
    case MIDI_NOTEOFF:
    case MIDI_NOTEON:
    case MIDI_KEY_PRESSURE:
    case MIDI_CTL_CHANGE:
    case MIDI_PITCH_BEND:
        if (LOBYTE(LOWORD(data)) >= 0x80)
        {
            SEQ_MIDIOUT(dev, evt);
            dest->runningStatus = evt;
        }
        SEQ_MIDIOUT(dev, d1);
        SEQ_MIDIOUT(dev, d2);
        break;
    case MIDI_PGM_CHANGE:
    case MIDI_CHN_PRESSURE:
        if (LOBYTE(LOWORD(data)) >= 0x80)
        {
            SEQ_MIDIOUT(dev, evt);
            dest->runningStatus = evt;
        }
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
            dest->runningStatus = 0;
            break;
        case 0x01: /* MTC Quarter frame */
        case 0x03: /* Song Select. */
            SEQ_MIDIOUT(dev, evt);
            SEQ_MIDIOUT(dev, d1);
            break;
        case 0x02: /* Song Position Pointer. */
            SEQ_MIDIOUT(dev, evt);
            SEQ_MIDIOUT(dev, d1);
            SEQ_MIDIOUT(dev, d2);
        }
        if (evt <= 0xF7) /* System Exclusive, System Common Message */
            dest->runningStatus = 0;
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

    TRACE("dwBufferLength=%u !\n", (unsigned)hdr->dwBufferLength);
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

    dest->runningStatus = 0;
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

static UINT midi_out_get_devcaps(WORD dev_id, MIDIOUTCAPSW *caps, UINT size)
{
    TRACE("(%04X, %p, %08X);\n", dev_id, caps, size);

    if (dev_id >= num_dests) return MMSYSERR_BADDEVICEID;
    if (!caps) return MMSYSERR_INVALPARAM;

    memcpy(caps, &dests[dev_id].caps, min(size, sizeof(*caps)));

    return MMSYSERR_NOERROR;
}

static UINT midi_out_get_volume(WORD dev_id, UINT *volume)
{
    if (!volume) return MMSYSERR_INVALPARAM;
    if (dev_id >= num_dests) return MMSYSERR_BADDEVICEID;

    *volume = 0xFFFFFFFF;
    return (dests[dev_id].caps.dwSupport & MIDICAPS_VOLUME) ? 0 : MMSYSERR_NOTSUPPORTED;
}

static UINT midi_out_reset(WORD dev_id)
{
    struct midi_dest *dest;
    unsigned chn;

    TRACE("(%04X);\n", dev_id);

    if (dev_id >= num_dests) return MMSYSERR_BADDEVICEID;
    dest = dests + dev_id;
    if (!dest->bEnabled) return MIDIERR_NODEVICE;

    /* stop all notes */
    for (chn = 0; chn < 16; chn++)
    {
        /* turn off every note */
        midi_out_data(dev_id, 0x7800 | MIDI_CTL_CHANGE | chn);
        /* remove sustain on all channels */
        midi_out_data(dev_id, (CTL_SUSTAIN << 8) | MIDI_CTL_CHANGE | chn);
    }
    dest->runningStatus = 0;
    /* FIXME: the LongData buffers must also be returned to the app */
    return MMSYSERR_NOERROR;
}

static void handle_sysex_data(struct midi_src *src, unsigned char value, UINT time)
{
    struct notify_context notify;
    MIDIHDR *hdr;
    BOOL done = FALSE;

    src->state |= 2;
    src->incLen = 0;

    in_buffer_lock();

    hdr = src->lpQueueHdr;
    if (hdr)
    {
        BYTE *data = (BYTE *)hdr->lpData;

        data[hdr->dwBytesRecorded++] = value;
        if (hdr->dwBytesRecorded == hdr->dwBufferLength)
            done = TRUE;
    }

    if (value == 0xf7) /* end */
    {
        src->state &= ~2;
        done = TRUE;
    }

    if (done && hdr)
    {
        src->lpQueueHdr = hdr->lpNext;
        hdr->dwFlags &= ~MHDR_INQUEUE;
        hdr->dwFlags |= MHDR_DONE;
        set_in_notify(&notify, src, src - srcs, MIM_LONGDATA, (UINT_PTR)hdr, time);
        notify_post(&notify);
    }

    in_buffer_unlock();
}

static void handle_regular_data(struct midi_src *src, unsigned char value, UINT time)
{
    struct notify_context notify;
    UINT to_send = 0;

#define IS_CMD(_x)     (((_x) & 0x80) == 0x80)
#define IS_SYS_CMD(_x) (((_x) & 0xF0) == 0xF0)

    if (!IS_CMD(value) && src->incLen == 0) /* try to reuse old cmd */
    {
        if (IS_CMD(src->incPrev) && !IS_SYS_CMD(src->incPrev))
        {
            src->incoming[0] = src->incPrev;
            src->incLen = 1;
        }
        else
        {
            /* FIXME: should generate MIM_ERROR notification */
            return;
        }
    }
    src->incoming[(int)src->incLen++] = value;
    if (src->incLen == 1 && !IS_SYS_CMD(src->incoming[0]))
        /* store new cmd, just in case */
        src->incPrev = src->incoming[0];

#undef IS_CMD
#undef IS_SYS_CMD

    switch (src->incoming[0] & 0xF0)
    {
    case MIDI_NOTEOFF:
    case MIDI_NOTEON:
    case MIDI_KEY_PRESSURE:
    case MIDI_CTL_CHANGE:
    case MIDI_PITCH_BEND:
        if (src->incLen == 3)
            to_send = (src->incoming[2] << 16) | (src->incoming[1] << 8) |
                src->incoming[0];
        break;
    case MIDI_PGM_CHANGE:
    case MIDI_CHN_PRESSURE:
        if (src->incLen == 2)
            to_send = (src->incoming[1] << 8) | src->incoming[0];
        break;
    case MIDI_SYSTEM_PREFIX:
        if (src->incLen == 1)
            to_send = src->incoming[0];
        break;
    }

    if (to_send)
    {
        src->incLen = 0;
        set_in_notify(&notify, src, src - srcs, MIM_DATA, to_send, time);
        notify_post(&notify);
    }
}

static void handle_midi_data(unsigned char *buffer, unsigned int len)
{
    unsigned int time = get_time_msec(), i;
    struct midi_src *src;
    unsigned char value;
    WORD dev_id;

    for (i = 0; i < len; i += (buffer[i] & 0x80) ? 8 : 4)
    {
        if (buffer[i] != SEQ_MIDIPUTC) continue;

        dev_id = buffer[i + 2];
        value = buffer[i + 1];

        if (dev_id >= num_srcs) continue;
        src = srcs + dev_id;
        if (src->state <= 0) continue;

        if (value == 0xf0 || src->state & 2) /* system exclusive */
            handle_sysex_data(src, value, time - src->startTime);
        else
            handle_regular_data(src, value, time - src->startTime);
    }
}

static void *rec_thread_proc(void *arg)
{
    int fd = PtrToLong(arg);
    unsigned char buffer[256];
    int len;
    struct pollfd pollfd[2];

    pollfd[0].fd = rec_cancel_pipe[0];
    pollfd[0].events = POLLIN;
    pollfd[1].fd = fd;
    pollfd[1].events = POLLIN;

    while (1)
    {
        /* Check if an event is present */
        if (poll(pollfd, ARRAY_SIZE(pollfd), -1) <= 0)
            continue;

        if (pollfd[0].revents & POLLIN) /* cancelled */
            break;

        len = read(fd, buffer, sizeof(buffer));

        if (len > 0 && len % 4 == 0)
            handle_midi_data(buffer, len);
    }
    return NULL;
}

static UINT midi_in_open(WORD dev_id, MIDIOPENDESC *desc, UINT flags, struct notify_context *notify)
{
    struct midi_src *src;
    int fd;

    TRACE("(%04X, %p, %08X);\n", dev_id, desc, flags);

    if (desc == NULL)
    {
        WARN("Invalid Parameter !\n");
        return MMSYSERR_INVALPARAM;
    }

    /* FIXME :
     * how to check that content of lpDesc is correct ?
     */
    if (dev_id >= num_srcs)
    {
        WARN("wDevID too large (%u) !\n", dev_id);
        return MMSYSERR_BADDEVICEID;
    }
    src = srcs + dev_id;
    if (src->state == -1)
    {
        WARN("device disabled\n");
        return MIDIERR_NODEVICE;
    }
    if (src->midiDesc.hMidi != 0)
    {
        WARN("device already open !\n");
        return MMSYSERR_ALLOCATED;
    }
    if ((flags & MIDI_IO_STATUS) != 0)
    {
        WARN("No support for MIDI_IO_STATUS in dwFlags yet, ignoring it\n");
        flags &= ~MIDI_IO_STATUS;
    }
    if ((flags & ~CALLBACK_TYPEMASK) != 0)
    {
        FIXME("Bad flags\n");
        return MMSYSERR_INVALFLAG;
    }

    fd = seq_open();
    if (fd < 0)
        return MMSYSERR_ERROR;

    if (num_midi_in_started++ == 0)
    {
        pipe(rec_cancel_pipe);
        if (pthread_create(&rec_thread_id, NULL, rec_thread_proc, LongToPtr(fd)))
        {
            close(rec_cancel_pipe[0]);
            close(rec_cancel_pipe[1]);
            num_midi_in_started = 0;
            WARN("Couldn't create thread for midi-in\n");
            seq_close(fd);
            return MMSYSERR_ERROR;
        }
        TRACE("Created thread for midi-in\n");
    }

    src->wFlags = HIWORD(flags & CALLBACK_TYPEMASK);

    src->lpQueueHdr = NULL;
    src->midiDesc = *desc;
    src->state = 0;
    src->incLen = 0;
    src->startTime = 0;
    src->fd = fd;

    set_in_notify(notify, src, dev_id, MIM_OPEN, 0, 0);
    return MMSYSERR_NOERROR;
}

static UINT midi_in_close(WORD dev_id, struct notify_context *notify)
{
    struct midi_src *src;

    TRACE("(%04X);\n", dev_id);

    if (dev_id >= num_srcs)
    {
        WARN("dev_id too big (%u) !\n", dev_id);
        return MMSYSERR_BADDEVICEID;
    }
    src = srcs + dev_id;
    if (src->midiDesc.hMidi == 0)
    {
        WARN("device not opened !\n");
        return MMSYSERR_ERROR;
    }
    if (src->lpQueueHdr != 0)
        return MIDIERR_STILLPLAYING;

    if (src->fd == -1)
    {
        WARN("ooops !\n");
        return MMSYSERR_ERROR;
    }
    if (--num_midi_in_started == 0)
    {
        TRACE("Stopping thread for midi-in\n");
        write(rec_cancel_pipe[1], "x", 1);
        pthread_join(rec_thread_id, NULL);
        close(rec_cancel_pipe[0]);
        close(rec_cancel_pipe[1]);
        TRACE("Stopped thread for midi-in\n");
    }
    seq_close(src->fd);
    src->fd = -1;

    set_in_notify(notify, src, dev_id, MIM_CLOSE, 0, 0);
    src->midiDesc.hMidi = 0;

    return MMSYSERR_NOERROR;
}

static UINT midi_in_add_buffer(WORD dev_id, MIDIHDR *hdr, UINT hdr_size)
{
    struct midi_src *src;
    MIDIHDR **next;

    TRACE("(%04X, %p, %d);\n", dev_id, hdr, hdr_size);

    if (dev_id >= num_srcs) return MMSYSERR_BADDEVICEID;
    src = srcs + dev_id;
    if (src->state == -1) return MIDIERR_NODEVICE;

    if (!hdr || hdr_size < offsetof(MIDIHDR, dwOffset) || !hdr->dwBufferLength)
        return MMSYSERR_INVALPARAM;
    if (hdr->dwFlags & MHDR_INQUEUE) return MIDIERR_STILLPLAYING;
    if (!(hdr->dwFlags & MHDR_PREPARED)) return MIDIERR_UNPREPARED;

    in_buffer_lock();

    hdr->dwFlags &= ~WHDR_DONE;
    hdr->dwFlags |= MHDR_INQUEUE;
    hdr->dwBytesRecorded = 0;
    hdr->lpNext = NULL;

    next = &src->lpQueueHdr;
    while (*next) next = &(*next)->lpNext;
    *next = hdr;

    in_buffer_unlock();

    return MMSYSERR_NOERROR;
}

static UINT midi_in_prepare(WORD dev_id, MIDIHDR *hdr, UINT hdr_size)
{
    TRACE("(%04X, %p, %d);\n", dev_id, hdr, hdr_size);

    if (hdr_size < offsetof(MIDIHDR, dwOffset) || !hdr || !hdr->lpData)
        return MMSYSERR_INVALPARAM;
    if (hdr->dwFlags & MHDR_PREPARED)
        return MMSYSERR_NOERROR;

    hdr->lpNext = NULL;
    hdr->dwFlags |= MHDR_PREPARED;
    hdr->dwFlags &= ~(MHDR_DONE | MHDR_INQUEUE);

    return MMSYSERR_NOERROR;
}

static UINT midi_in_unprepare(WORD dev_id, MIDIHDR *hdr, UINT hdr_size)
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

static UINT midi_in_get_devcaps(WORD dev_id, MIDIINCAPSW *caps, UINT size)
{
    TRACE("(%04X, %p, %08X);\n", dev_id, caps, size);

    if (dev_id >= num_srcs) return MMSYSERR_BADDEVICEID;
    if (!caps) return MMSYSERR_INVALPARAM;

    memcpy(caps, &srcs[dev_id].caps, min(size, sizeof(*caps)));

    return MMSYSERR_NOERROR;
}

static UINT midi_in_start(WORD dev_id)
{
    struct midi_src *src;

    TRACE("(%04X);\n", dev_id);

    if (dev_id >= num_srcs) return MMSYSERR_BADDEVICEID;
    src = srcs + dev_id;
    if (src->state == -1) return MIDIERR_NODEVICE;

    src->state = 1;
    src->startTime = get_time_msec();
    return MMSYSERR_NOERROR;
}

static UINT midi_in_stop(WORD dev_id)
{
    struct midi_src *src;

    TRACE("(%04X);\n", dev_id);

    if (dev_id >= num_srcs) return MMSYSERR_BADDEVICEID;
    src = srcs + dev_id;
    if (src->state == -1) return MIDIERR_NODEVICE;

    src->state = 0;
    return MMSYSERR_NOERROR;
}

static UINT midi_in_reset(WORD dev_id, struct notify_context *notify)
{
    UINT cur_time = get_time_msec();
    UINT err = MMSYSERR_NOERROR;
    struct midi_src *src;
    MIDIHDR *hdr;

    TRACE("(%04X);\n", dev_id);

    if (dev_id >= num_srcs) return MMSYSERR_BADDEVICEID;
    src = srcs + dev_id;
    if (src->state == -1) return MIDIERR_NODEVICE;

    in_buffer_lock();

    if (src->lpQueueHdr)
    {
        hdr = src->lpQueueHdr;
        src->lpQueueHdr = hdr->lpNext;
        hdr->dwFlags &= ~MHDR_INQUEUE;
        hdr->dwFlags |= MHDR_DONE;
        set_in_notify(notify, src, dev_id, MIM_LONGDATA, (UINT_PTR)hdr, cur_time - src->startTime);
        if (src->lpQueueHdr) err = ERROR_RETRY; /* ask the client to call again */
    }

    in_buffer_unlock();

    return err;
}

NTSTATUS oss_midi_out_message(void *args)
{
    struct midi_out_message_params *params = args;

    params->notify->send_notify = FALSE;

    switch (params->msg)
    {
    case DRVM_INIT:
        *params->err = oss_midi_init();
        break;
    case DRVM_EXIT:
        *params->err = midi_exit();
        break;
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
    case MODM_GETDEVCAPS:
        *params->err = midi_out_get_devcaps(params->dev_id, (MIDIOUTCAPSW *)params->param_1, params->param_2);
        break;
    case MODM_GETNUMDEVS:
        *params->err = num_dests;
        break;
    case MODM_GETVOLUME:
        *params->err = midi_out_get_volume(params->dev_id, (UINT *)params->param_1);
        break;
    case MODM_SETVOLUME:
        *params->err = 0;
        break;
    case MODM_RESET:
        *params->err = midi_out_reset(params->dev_id);
        break;
    default:
        TRACE("Unsupported message\n");
        *params->err = MMSYSERR_NOTSUPPORTED;
    }

    return STATUS_SUCCESS;
}

NTSTATUS oss_midi_in_message(void *args)
{
    struct midi_in_message_params *params = args;

    params->notify->send_notify = FALSE;

    switch (params->msg)
    {
    case DRVM_INIT:
        *params->err = oss_midi_init();
        break;
    case DRVM_EXIT:
        *params->err = midi_exit();
        break;
    case DRVM_ENABLE:
    case DRVM_DISABLE:
        /* FIXME: Pretend this is supported */
        *params->err = MMSYSERR_NOERROR;
        break;
    case MIDM_OPEN:
        *params->err = midi_in_open(params->dev_id, (MIDIOPENDESC *)params->param_1, params->param_2, params->notify);
        break;
    case MIDM_CLOSE:
        *params->err = midi_in_close(params->dev_id, params->notify);
        break;
    case MIDM_ADDBUFFER:
        *params->err = midi_in_add_buffer(params->dev_id, (MIDIHDR *)params->param_1, params->param_2);
        break;
    case MIDM_PREPARE:
        *params->err = midi_in_prepare(params->dev_id, (MIDIHDR *)params->param_1, params->param_2);
        break;
    case MIDM_UNPREPARE:
        *params->err = midi_in_unprepare(params->dev_id, (MIDIHDR *)params->param_1, params->param_2);
        break;
    case MIDM_GETDEVCAPS:
        *params->err = midi_in_get_devcaps(params->dev_id, (MIDIINCAPSW *)params->param_1, params->param_2);
        break;
    case MIDM_GETNUMDEVS:
        *params->err = num_srcs;
        break;
    case MIDM_START:
        *params->err = midi_in_start(params->dev_id);
        break;
    case MIDM_STOP:
        *params->err = midi_in_stop(params->dev_id);
        break;
    case MIDM_RESET:
        *params->err = midi_in_reset(params->dev_id, params->notify);
        break;
    default:
        TRACE("Unsupported message\n");
        *params->err = MMSYSERR_NOTSUPPORTED;
    }

    return STATUS_SUCCESS;
}

NTSTATUS oss_midi_notify_wait(void *args)
{
    struct midi_notify_wait_params *params = args;

    pthread_mutex_lock(&notify_mutex);

    while (!notify_quit && notify_buffer_empty())
        pthread_cond_wait(&notify_read_cond, &notify_mutex);

    *params->quit = notify_quit;
    if (!notify_quit)
    {
        notify_buffer_remove(params->notify);
        pthread_cond_signal(&notify_write_cond);
    }
    pthread_mutex_unlock(&notify_mutex);

    return STATUS_SUCCESS;
}

#ifdef _WIN64

typedef UINT PTR32;

struct notify_context32
{
    BOOL send_notify;
    WORD dev_id;
    WORD msg;
    UINT param_1;
    UINT param_2;
    UINT callback;
    UINT flags;
    PTR32 device;
    UINT instance;
};

static void notify_to_notify32(struct notify_context32 *notify32,
                               const struct notify_context *notify)
{
    notify32->send_notify = notify->send_notify;
    notify32->dev_id = notify->dev_id;
    notify32->msg = notify->msg;
    notify32->param_1 = notify->param_1;
    notify32->param_2 = notify->param_2;
    notify32->callback = notify->callback;
    notify32->flags = notify->flags;
    notify32->device = PtrToUlong(notify->device);
    notify32->instance = notify->instance;
}

struct midi_open_desc32
{
    PTR32 hMidi;
    UINT dwCallback;
    UINT dwInstance;
    UINT dnDevNode;
    UINT cIds;
    MIDIOPENSTRMID rgIds;
};

struct midi_hdr32
{
    PTR32 lpData;
    UINT dwBufferLength;
    UINT dwBytesRecorded;
    UINT dwUser;
    UINT dwFlags;
    PTR32 lpNext;
    UINT reserved;
    UINT dwOffset;
    UINT dwReserved[8];
};

static UINT wow64_midi_out_prepare(WORD dev_id, struct midi_hdr32 *hdr, UINT hdr_size)
{
    TRACE("(%04X, %p, %d);\n", dev_id, hdr, hdr_size);

    if (hdr_size < offsetof(struct midi_hdr32, dwOffset) || !hdr || !hdr->lpData)
        return MMSYSERR_INVALPARAM;
    if (hdr->dwFlags & MHDR_PREPARED)
        return MMSYSERR_NOERROR;

    hdr->lpNext = 0;
    hdr->dwFlags |= MHDR_PREPARED;
    hdr->dwFlags &= ~(MHDR_DONE | MHDR_INQUEUE);
    return MMSYSERR_NOERROR;
}

static UINT wow64_midi_out_unprepare(WORD dev_id, struct midi_hdr32 *hdr, UINT hdr_size)
{
    TRACE("(%04X, %p, %d);\n", dev_id, hdr, hdr_size);

    if (hdr_size < offsetof(struct midi_hdr32, dwOffset) || !hdr || !hdr->lpData)
        return MMSYSERR_INVALPARAM;
    if (!(hdr->dwFlags & MHDR_PREPARED))
        return MMSYSERR_NOERROR;
    if (hdr->dwFlags & MHDR_INQUEUE)
        return MIDIERR_STILLPLAYING;

    hdr->dwFlags &= ~MHDR_PREPARED;
    return MMSYSERR_NOERROR;
}

NTSTATUS oss_wow64_midi_out_message(void *args)
{
    struct
    {
        UINT dev_id;
        UINT msg;
        UINT user;
        UINT param_1;
        UINT param_2;
        PTR32 err;
        PTR32 notify;
    } *params32 = args;
    struct notify_context32 *notify32 = ULongToPtr(params32->notify);
    struct midi_open_desc32 *desc32;
    struct midi_hdr32 *hdr32;
    struct notify_context notify;
    MIDIOPENDESC open_desc;
    MIDIHDR hdr;
    struct midi_out_message_params params =
    {
        .dev_id = params32->dev_id,
        .msg = params32->msg,
        .user = params32->user,
        .param_1 = params32->param_1,
        .param_2 = params32->param_2,
        .err = ULongToPtr(params32->err),
        .notify = &notify
    };
    notify32->send_notify = FALSE;

    switch (params32->msg)
    {
    case MODM_OPEN:
        desc32 = ULongToPtr(params32->param_1);

        open_desc.hMidi = ULongToPtr(desc32->hMidi);
        open_desc.dwCallback = desc32->dwCallback;
        open_desc.dwInstance = desc32->dwInstance;
        open_desc.dnDevNode = desc32->dnDevNode;
        open_desc.cIds = desc32->cIds;
        open_desc.rgIds.dwStreamID = desc32->rgIds.dwStreamID;
        open_desc.rgIds.wDeviceID = desc32->rgIds.wDeviceID;

        params.param_1 = (UINT_PTR)&open_desc;
        break;

    case MODM_LONGDATA:
        hdr32 = ULongToPtr(params32->param_1);

        memset(&hdr, 0, sizeof(hdr));
        hdr.lpData = ULongToPtr(hdr32->lpData);
        hdr.dwBufferLength = hdr32->dwBufferLength;
        hdr.dwFlags = hdr32->dwFlags;

        params.param_1 = (UINT_PTR)&hdr;
        params.param_2 = sizeof(hdr);
        break;

    case MODM_PREPARE: /* prepare and unprepare are easier to handle explicitly */
        hdr32 = ULongToPtr(params32->param_1);

        *params.err = wow64_midi_out_prepare(params32->dev_id, hdr32, params32->param_2);
        return STATUS_SUCCESS;

    case MODM_UNPREPARE:
        hdr32 = ULongToPtr(params32->param_1);

        *params.err = wow64_midi_out_unprepare(params32->dev_id, hdr32, params32->param_2);
        return STATUS_SUCCESS;
    }

    oss_midi_out_message(&params);

    switch (params32->msg)
    {
    case MODM_LONGDATA:
        hdr32 = ULongToPtr(params32->param_1);

        hdr32->dwFlags = hdr.dwFlags;
        break;
    }

    if (notify.send_notify)
    {
        notify_to_notify32(notify32, &notify);

        if (notify.msg == MOM_DONE)
            notify32->param_1 = params32->param_1; /* restore the 32-bit hdr */
    }
    return STATUS_SUCCESS;
}

static UINT wow64_midi_in_prepare(WORD dev_id, struct midi_hdr32 *hdr, UINT hdr_size)
{
    TRACE("(%04X, %p, %d);\n", dev_id, hdr, hdr_size);

    if (hdr_size < offsetof(struct midi_hdr32, dwOffset) || !hdr || !hdr->lpData)
        return MMSYSERR_INVALPARAM;
    if (hdr->dwFlags & MHDR_PREPARED)
        return MMSYSERR_NOERROR;

    hdr->lpNext = 0;
    hdr->dwFlags |= MHDR_PREPARED;
    hdr->dwFlags &= ~(MHDR_DONE | MHDR_INQUEUE);

    return MMSYSERR_NOERROR;
}

static UINT wow64_midi_in_unprepare(WORD dev_id, struct midi_hdr32 *hdr, UINT hdr_size)
{
    TRACE("(%04X, %p, %d);\n", dev_id, hdr, hdr_size);

    if (hdr_size < offsetof(struct midi_hdr32, dwOffset) || !hdr || !hdr->lpData)
        return MMSYSERR_INVALPARAM;
    if (!(hdr->dwFlags & MHDR_PREPARED))
        return MMSYSERR_NOERROR;
    if (hdr->dwFlags & MHDR_INQUEUE)
        return MIDIERR_STILLPLAYING;

    hdr->dwFlags &= ~MHDR_PREPARED;

    return MMSYSERR_NOERROR;
}

NTSTATUS oss_wow64_midi_in_message(void *args)
{
    struct
    {
        UINT dev_id;
        UINT msg;
        UINT user;
        UINT param_1;
        UINT param_2;
        PTR32 err;
        PTR32 notify;
    } *params32 = args;
    struct notify_context32 *notify32 = ULongToPtr(params32->notify);
    struct midi_open_desc32 *desc32;
    struct midi_hdr32 *hdr32;
    struct notify_context notify;
    MIDIOPENDESC open_desc;
    MIDIHDR *hdr = NULL;
    struct midi_in_message_params params =
    {
        .dev_id = params32->dev_id,
        .msg = params32->msg,
        .user = params32->user,
        .param_1 = params32->param_1,
        .param_2 = params32->param_2,
        .err = ULongToPtr(params32->err),
        .notify = &notify
    };
    notify32->send_notify = FALSE;

    switch (params32->msg)
    {
    case MIDM_OPEN:
        desc32 = ULongToPtr(params32->param_1);

        open_desc.hMidi = ULongToPtr(desc32->hMidi);
        open_desc.dwCallback = desc32->dwCallback;
        open_desc.dwInstance = desc32->dwInstance;
        open_desc.dnDevNode = desc32->dnDevNode;
        open_desc.cIds = desc32->cIds;
        open_desc.rgIds.dwStreamID = desc32->rgIds.dwStreamID;
        open_desc.rgIds.wDeviceID = desc32->rgIds.wDeviceID;

        params.param_1 = (UINT_PTR)&open_desc;
        break;

    case MIDM_ADDBUFFER:
        hdr32 = ULongToPtr(params32->param_1);

        hdr = calloc(1, sizeof(*hdr));
        hdr->lpData = ULongToPtr(hdr32->lpData);
        hdr->dwBufferLength = hdr32->dwBufferLength;
        hdr->dwFlags = hdr32->dwFlags;
        hdr->dwReserved[7] = params32->param_1; /* keep hdr32 for MIM_LONGDATA notification */

        params.param_1 = (UINT_PTR)hdr;
        params.param_2 = sizeof(*hdr);
        break;

    case MIDM_PREPARE: /* prepare and unprepare are easier to handle explicitly */
        hdr32 = ULongToPtr(params32->param_1);

        *params.err = wow64_midi_in_prepare(params32->dev_id, hdr32, params32->param_2);
        return STATUS_SUCCESS;

    case MIDM_UNPREPARE:
        hdr32 = ULongToPtr(params32->param_1);

        *params.err = wow64_midi_in_unprepare(params32->dev_id, hdr32, params32->param_2);
        return STATUS_SUCCESS;
    }

    oss_midi_in_message(&params);

    switch (params32->msg)
    {
    case MIDM_ADDBUFFER:
        hdr32 = ULongToPtr(params32->param_1);

        if (!*params.err)
        {
            hdr32->dwFlags = hdr->dwFlags;
            hdr32->dwBytesRecorded = hdr->dwBytesRecorded;
            hdr32->lpNext = 0;
        }
        else
            free(hdr);
        break;
    }

    if (notify.send_notify)
    {
        notify_to_notify32(notify32, &notify);

        if (notify.msg == MIM_LONGDATA)
        {
            hdr = (MIDIHDR *)notify.param_1;
            notify32->param_1 = hdr->dwReserved[7];
            hdr32 = ULongToPtr(notify32->param_1);
            hdr32->dwBytesRecorded = hdr->dwBytesRecorded;
            hdr32->dwFlags = hdr->dwFlags;
            free(hdr);
        }
    }
    return STATUS_SUCCESS;
}

NTSTATUS oss_wow64_midi_notify_wait(void *args)
{
    struct
    {
        PTR32 quit;
        PTR32 notify;
    } *params32 = args;
    struct notify_context32 *notify32 = ULongToPtr(params32->notify);
    struct midi_hdr32 *hdr32;
    struct notify_context notify;
    MIDIHDR *hdr;
    struct midi_notify_wait_params params =
    {
        .quit = ULongToPtr(params32->quit),
        .notify = &notify
    };
    notify32->send_notify = FALSE;

    oss_midi_notify_wait(&params);

    if (!*params.quit && notify.send_notify)
    {
        notify_to_notify32(notify32, &notify);

        if (notify.msg == MIM_LONGDATA)
        {
            hdr = (MIDIHDR *)notify.param_1;
            notify32->param_1 = hdr->dwReserved[7];
            hdr32 = ULongToPtr(notify32->param_1);
            hdr32->dwBytesRecorded = hdr->dwBytesRecorded;
            hdr32->dwFlags = hdr->dwFlags;
            free(hdr);
        }
    }
    return STATUS_SUCCESS;
}

#endif /* _WIN64 */
