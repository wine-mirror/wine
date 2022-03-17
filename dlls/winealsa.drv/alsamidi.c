/*
 * MIDI driver for ALSA (unixlib)
 *
 * Copyright 1994       Martin Ayotte
 * Copyright 1998       Luiz Otavio L. Zorzella
 * Copyright 1998, 1999 Eric POUECH
 * Copyright 2003       Christian Costa
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
#include <stdio.h>
#include <pthread.h>
#include <alsa/asoundlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "mmdeviceapi.h"
#include "mmddk.h"

#include "wine/debug.h"
#include "wine/unixlib.h"

#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(midi);

struct midi_dest
{
    BOOL                bEnabled;
    MIDIOPENDESC        midiDesc;
    WORD                wFlags;
    MIDIOUTCAPSW        caps;
    snd_seq_t          *seq;
    snd_seq_addr_t      addr;
    int                 port_out;
};

static pthread_mutex_t seq_mutex = PTHREAD_MUTEX_INITIALIZER;

static unsigned int num_dests, num_srcs;
static struct midi_dest dests[MAX_MIDIOUTDRV];
static struct midi_src srcs[MAX_MIDIINDRV];
static snd_seq_t *midi_seq;
static unsigned int seq_refs;
static int port_in = -1;

static void seq_lock(void)
{
    pthread_mutex_lock(&seq_mutex);
}

static void seq_unlock(void)
{
    pthread_mutex_unlock(&seq_mutex);
}

NTSTATUS midi_seq_lock(void *args)
{
    if (args) seq_lock();
    else seq_unlock();

    return STATUS_SUCCESS;
}

static snd_seq_t *seq_open(int *port_in_ret)
{
    static int midi_warn;

    seq_lock();
    if (seq_refs == 0)
    {
        if (snd_seq_open(&midi_seq, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0)
        {
            if (!midi_warn)
                WARN("Error opening ALSA sequencer.\n");
            midi_warn = 1;
            seq_unlock();
            return NULL;
        }
        snd_seq_set_client_name(midi_seq, "WINE midi driver");
    }
    seq_refs++;

    if (port_in_ret)
    {
        if (port_in < 0)
        {
            port_in = snd_seq_create_simple_port(midi_seq, "WINE ALSA Input",
                                                 SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_READ | SND_SEQ_PORT_CAP_SUBS_WRITE,
                                                 SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION);
            if (port_in < 0)
                TRACE("Unable to create input port\n");
            else
                TRACE("Input port %d created successfully\n", port_in);
        }
        *port_in_ret = port_in;
    }
    seq_unlock();
    return midi_seq;
}

static void seq_close(void)
{
    seq_lock();
    if (--seq_refs == 0)
    {
        if (port_in >= 0)
        {
            snd_seq_delete_simple_port(midi_seq, port_in);
            port_in = -1;
        }
        snd_seq_close(midi_seq);
        midi_seq = NULL;
    }
    seq_unlock();
}

NTSTATUS midi_seq_open(void *args)
{
    struct midi_seq_open_params *params = args;

    if (!params->close)
        params->seq = seq_open(params->port_in);
    else
        seq_close();

    return STATUS_SUCCESS;
}

static int alsa_to_win_device_type(unsigned int type)
{
    /* MOD_MIDIPORT     output port
     * MOD_SYNTH        generic internal synth
     * MOD_SQSYNTH      square wave internal synth
     * MOD_FMSYNTH      FM internal synth
     * MOD_MAPPER       MIDI mapper
     * MOD_WAVETABLE    hardware wavetable internal synth
     * MOD_SWSYNTH      software internal synth
     */

    /* FIXME Is this really the correct equivalence from ALSA to
       Windows Sound type? */

    if (type & SND_SEQ_PORT_TYPE_SYNTH)
        return MOD_FMSYNTH;

    if (type & (SND_SEQ_PORT_TYPE_DIRECT_SAMPLE|SND_SEQ_PORT_TYPE_SAMPLE))
        return MOD_SYNTH;

    if (type & (SND_SEQ_PORT_TYPE_MIDI_GENERIC|SND_SEQ_PORT_TYPE_APPLICATION))
        return MOD_MIDIPORT;

    ERR("Cannot determine the type (alsa type is %x) of this midi device. Assuming FM Synth\n", type);
    return MOD_FMSYNTH;
}

static void port_add(snd_seq_client_info_t* cinfo, snd_seq_port_info_t* pinfo, unsigned int cap, unsigned int type)
{
    char name[MAXPNAMELEN];
    unsigned int len;
    struct midi_dest *dest;
    struct midi_src *src;

    if (cap & SND_SEQ_PORT_CAP_WRITE) {
        TRACE("OUT (%d:%s:%s:%d:%s:%x)\n",snd_seq_client_info_get_client(cinfo),
              snd_seq_client_info_get_name(cinfo),
              snd_seq_client_info_get_type(cinfo) == SND_SEQ_USER_CLIENT ? "user" : "kernel",
              snd_seq_port_info_get_port(pinfo),
              snd_seq_port_info_get_name(pinfo),
              type);

        if (num_dests >= MAX_MIDIOUTDRV)
            return;
        if (!type)
            return;

        dest = dests + num_dests;
        dest->addr = *snd_seq_port_info_get_addr(pinfo);

        /* Manufac ID. We do not have access to this with soundcard.h
         * Does not seem to be a problem, because in mmsystem.h only
         * Microsoft's ID is listed.
         */
        dest->caps.wMid = 0x00FF;
        dest->caps.wPid = 0x0001;     /* FIXME Product ID  */
        /* Product Version. We simply say "1" */
        dest->caps.vDriverVersion = 0x001;
        /* The following are mandatory for MOD_MIDIPORT */
        dest->caps.wChannelMask = 0xFFFF;
        dest->caps.wVoices      = 0;
        dest->caps.wNotes       = 0;
        dest->caps.dwSupport    = 0;

        /* Try to use both client and port names, if this is too long take the port name only.
           In the second case the port name should be explicit enough due to its big size.
        */
        len = strlen(snd_seq_port_info_get_name(pinfo));
        if ( (strlen(snd_seq_client_info_get_name(cinfo)) + len + 3) < sizeof(name) ) {
            sprintf(name, "%s - %s", snd_seq_client_info_get_name(cinfo), snd_seq_port_info_get_name(pinfo));
            len = strlen(name);
        } else {
            len = min(len, sizeof(name) - 1);
            memcpy(name, snd_seq_port_info_get_name(pinfo), len);
            name[len] = '\0';
        }
        ntdll_umbstowcs( name, len + 1, dest->caps.szPname, ARRAY_SIZE(dest->caps.szPname));

        dest->caps.wTechnology = alsa_to_win_device_type(type);

        if (MOD_MIDIPORT != dest->caps.wTechnology) {
            /* FIXME Do we have this information?
             * Assuming the soundcards can handle
             * MIDICAPS_VOLUME and MIDICAPS_LRVOLUME but
             * not MIDICAPS_CACHE.
             */
            dest->caps.dwSupport = MIDICAPS_VOLUME|MIDICAPS_LRVOLUME;
            dest->caps.wVoices   = 16;

            /* FIXME Is it possible to know the maximum
             * number of simultaneous notes of a soundcard ?
             * I believe we don't have this information, but
             * it's probably equal or more than wVoices
             */
            dest->caps.wNotes     = 16;
        }
        dest->bEnabled            = TRUE;
        dest->port_out            = -1;

        TRACE("MidiOut[%d]\tname='%s' techn=%d voices=%d notes=%d chnMsk=%04x support=%d\n"
              "\tALSA info: midi dev-type=%x, capa=0\n",
              num_dests, wine_dbgstr_w(dest->caps.szPname),
              dest->caps.wTechnology,
              dest->caps.wVoices, dest->caps.wNotes,
              dest->caps.wChannelMask, dest->caps.dwSupport,
              type);

        num_dests++;
    }
    if (cap & SND_SEQ_PORT_CAP_READ) {
        TRACE("IN  (%d:%s:%s:%d:%s:%x)\n",snd_seq_client_info_get_client(cinfo),
              snd_seq_client_info_get_name(cinfo),
              snd_seq_client_info_get_type(cinfo) == SND_SEQ_USER_CLIENT ? "user" : "kernel",
              snd_seq_port_info_get_port(pinfo),
              snd_seq_port_info_get_name(pinfo),
              type);

        if (num_srcs >= MAX_MIDIINDRV)
            return;
        if (!type)
            return;

        src = srcs + num_srcs;
        src->addr = *snd_seq_port_info_get_addr(pinfo);

        /* Manufac ID. We do not have access to this with soundcard.h
         * Does not seem to be a problem, because in mmsystem.h only
         * Microsoft's ID is listed.
         */
        src->caps.wMid = 0x00FF;
        src->caps.wPid = 0x0001;      /* FIXME Product ID  */
        /* Product Version. We simply say "1" */
        src->caps.vDriverVersion = 0x001;
        src->caps.dwSupport = 0; /* mandatory with MIDIINCAPS */

        /* Try to use both client and port names, if this is too long take the port name only.
           In the second case the port name should be explicit enough due to its big size.
        */
        len = strlen(snd_seq_port_info_get_name(pinfo));
        if ( (strlen(snd_seq_client_info_get_name(cinfo)) + len + 3) < sizeof(name) ) {
            sprintf(name, "%s - %s", snd_seq_client_info_get_name(cinfo), snd_seq_port_info_get_name(pinfo));
            len = strlen(name);
        } else {
            len = min(len, sizeof(name) - 1);
            memcpy(name, snd_seq_port_info_get_name(pinfo), len);
            name[len] = '\0';
        }
        ntdll_umbstowcs( name, len + 1, src->caps.szPname, ARRAY_SIZE(src->caps.szPname));
        src->state = 0;

        TRACE("MidiIn [%d]\tname='%s' support=%d\n"
              "\tALSA info: midi dev-type=%x, capa=0\n",
              num_srcs, wine_dbgstr_w(src->caps.szPname), src->caps.dwSupport, type);

        num_srcs++;
    }
}

NTSTATUS midi_init(void *args)
{
    struct midi_init_params *params = args;
    static BOOL init_done;
    snd_seq_client_info_t *cinfo;
    snd_seq_port_info_t *pinfo;
    snd_seq_t *seq;

    if (init_done) {
        *params->err = ERROR_ALREADY_INITIALIZED;
        return STATUS_SUCCESS;
    }

    TRACE("Initializing the MIDI variables.\n");
    init_done = TRUE;

    /* try to open device */
    if (!(seq = seq_open(NULL))) {
        *params->err = ERROR_OPEN_FAILED;
        return STATUS_SUCCESS;
    }

    cinfo = calloc( 1, snd_seq_client_info_sizeof() );
    pinfo = calloc( 1, snd_seq_port_info_sizeof() );

    /* First, search for all internal midi devices */
    snd_seq_client_info_set_client(cinfo, -1);
    while (snd_seq_query_next_client(seq, cinfo) >= 0) {
        snd_seq_port_info_set_client(pinfo, snd_seq_client_info_get_client(cinfo));
        snd_seq_port_info_set_port(pinfo, -1);
        while (snd_seq_query_next_port(seq, pinfo) >= 0) {
            unsigned int cap = snd_seq_port_info_get_capability(pinfo);
            unsigned int type = snd_seq_port_info_get_type(pinfo);
            if (!(type & SND_SEQ_PORT_TYPE_PORT))
                port_add(cinfo, pinfo, cap, type);
        }
    }

    /* Second, search for all external ports */
    snd_seq_client_info_set_client(cinfo, -1);
    while (snd_seq_query_next_client(seq, cinfo) >= 0) {
        snd_seq_port_info_set_client(pinfo, snd_seq_client_info_get_client(cinfo));
        snd_seq_port_info_set_port(pinfo, -1);
        while (snd_seq_query_next_port(seq, pinfo) >= 0) {
            unsigned int cap = snd_seq_port_info_get_capability(pinfo);
            unsigned int type = snd_seq_port_info_get_type(pinfo);
            if (type & SND_SEQ_PORT_TYPE_PORT)
                port_add(cinfo, pinfo, cap, type);
        }
    }
    seq_close();
    free( cinfo );
    free( pinfo );

    *params->err = NOERROR;
    params->num_srcs = num_srcs;
    params->srcs = srcs;

    TRACE("End\n");

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
    int ret;
    int port_out;
    char port_out_name[32];
    snd_seq_t *midi_seq;
    struct midi_dest *dest;

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
        WARN("bad dwFlags\n");
        return MMSYSERR_INVALFLAG;
    }

    switch (dest->caps.wTechnology)
    {
    case MOD_FMSYNTH:
    case MOD_MIDIPORT:
    case MOD_SYNTH:
        if (!(midi_seq = seq_open(NULL)))
            return MMSYSERR_ALLOCATED;
        break;
    default:
        WARN("Technology not supported (yet) %d !\n", dest->caps.wTechnology);
        return MMSYSERR_NOTENABLED;
    }

    dest->wFlags = HIWORD(flags & CALLBACK_TYPEMASK);
    dest->midiDesc = *midi_desc;
    dest->seq = midi_seq;

    seq_lock();
    /* Create a port dedicated to a specific device */
    /* Keep the old name without a number for the first port */
    if (dev_id)
        sprintf(port_out_name, "WINE ALSA Output #%d", dev_id);

    port_out = snd_seq_create_simple_port(midi_seq, dev_id ? port_out_name : "WINE ALSA Output",
                                          SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ | SND_SEQ_PORT_CAP_SUBS_WRITE,
                                          SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION);

    if (port_out < 0)
    {
        TRACE("Unable to create output port\n");
        dest->port_out = -1;
    }
    else
    {
        TRACE("Output port %d created successfully\n", port_out);
        dest->port_out = port_out;

        /* Connect our app port to the device port */
        ret = snd_seq_connect_to(midi_seq, port_out, dest->addr.client, dest->addr.port);

        /* usually will happen when the port is already connected */
        /* other errors should not be fatal either */
        if (ret < 0)
            WARN("Could not connect port %d to %d:%d: %s\n", dev_id, dest->addr.client,
                 dest->addr.port, snd_strerror(ret));
    }
    seq_unlock();

    if (port_out < 0)
        return MMSYSERR_NOTENABLED;

    TRACE("Output port :%d connected %d:%d\n", port_out, dest->addr.client, dest->addr.port);

    set_out_notify(notify, dest, dev_id, MOM_OPEN, 0, 0);
    return MMSYSERR_NOERROR;
}

static UINT midi_out_close(WORD dev_id, struct notify_context *notify)
{
    struct midi_dest *dest;

    TRACE("(%04X);\n", dev_id);

    if (dev_id >= num_dests)
    {
        WARN("bad device ID : %d\n", dev_id);
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

    if (dest->seq == NULL)
    {
        WARN("can't close !\n");
        return MMSYSERR_ERROR;
    }

    switch (dest->caps.wTechnology)
    {
    case MOD_FMSYNTH:
    case MOD_MIDIPORT:
    case MOD_SYNTH:
        seq_lock();
        TRACE("Deleting port :%d, connected to %d:%d\n", dest->port_out, dest->addr.client, dest->addr.port);
        snd_seq_delete_simple_port(dest->seq, dest->port_out);
        dest->port_out = -1;
        seq_unlock();
        seq_close();
        dest->seq = NULL;
        break;
    default:
        WARN("Technology not supported (yet) %d !\n", dest->caps.wTechnology);
        return MMSYSERR_NOTENABLED;
    }

    set_out_notify(notify, dest, dev_id, MOM_CLOSE, 0, 0);

    dest->midiDesc.hMidi = 0;

    return MMSYSERR_NOERROR;
}

static UINT midi_out_data(WORD dev_id, UINT data)
{
    BYTE evt = LOBYTE(LOWORD(data));
    BYTE d1  = HIBYTE(LOWORD(data));
    BYTE d2  = LOBYTE(HIWORD(data));
    struct midi_dest *dest;

    TRACE("(%04X, %08X);\n", dev_id, data);

    if (dev_id >= num_dests) return MMSYSERR_BADDEVICEID;
    dest = dests + dev_id;

    if (!dest->bEnabled) return MIDIERR_NODEVICE;

    if (dest->seq == NULL)
    {
        WARN("can't play !\n");
        return MIDIERR_NODEVICE;
    }
    switch (dest->caps.wTechnology)
    {
    case MOD_SYNTH:
    case MOD_MIDIPORT:
    {
        int handled = 1; /* Assume event is handled */
        snd_seq_event_t event;
        snd_seq_ev_clear(&event);
        snd_seq_ev_set_direct(&event);
        snd_seq_ev_set_source(&event, dest->port_out);
        snd_seq_ev_set_subs(&event);

        switch (evt & 0xF0)
        {
        case MIDI_CMD_NOTE_OFF:
            snd_seq_ev_set_noteoff(&event, evt & 0x0F, d1, d2);
            break;
        case MIDI_CMD_NOTE_ON:
            snd_seq_ev_set_noteon(&event, evt & 0x0F, d1, d2);
            break;
        case MIDI_CMD_NOTE_PRESSURE:
            snd_seq_ev_set_keypress(&event, evt & 0x0F, d1, d2);
            break;
        case MIDI_CMD_CONTROL:
            snd_seq_ev_set_controller(&event, evt & 0x0F, d1, d2);
            break;
        case MIDI_CMD_BENDER:
            snd_seq_ev_set_pitchbend(&event, evt & 0x0F, ((WORD)d2 << 7 | (WORD)d1) - 0x2000);
            break;
        case MIDI_CMD_PGM_CHANGE:
            snd_seq_ev_set_pgmchange(&event, evt & 0x0F, d1);
            break;
        case MIDI_CMD_CHANNEL_PRESSURE:
            snd_seq_ev_set_chanpress(&event, evt & 0x0F, d1);
            break;
        case MIDI_CMD_COMMON_SYSEX:
            switch (evt & 0x0F)
            {
            case 0x00:	/* System Exclusive, don't do it on modData,
                         * should require modLongData*/
            case 0x04:	/* Undefined. */
            case 0x05:	/* Undefined. */
            case 0x07:	/* End of Exclusive. */
            case 0x09:	/* Undefined. */
            case 0x0D:	/* Undefined. */
                handled = 0;
                break;
            case 0x06:	/* Tune Request */
            case 0x08:	/* Timing Clock. */
            case 0x0A:	/* Start. */
            case 0x0B:	/* Continue */
            case 0x0C:	/* Stop */
            case 0x0E: 	/* Active Sensing. */
            {
                snd_midi_event_t *midi_event;

                snd_midi_event_new(1, &midi_event);
                snd_midi_event_init(midi_event);
                snd_midi_event_encode_byte(midi_event, evt, &event);
                snd_midi_event_free(midi_event);
                break;
            }
            case 0x0F: 	/* Reset */
                /* snd_seq_ev_set_sysex(&event, 1, &evt);
                   this other way may be better */
            {
                BYTE reset_sysex_seq[] = {MIDI_CMD_COMMON_SYSEX, 0x7e, 0x7f, 0x09, 0x01, 0xf7};
                snd_seq_ev_set_sysex(&event, sizeof(reset_sysex_seq), reset_sysex_seq);
                break;
            }
            case 0x01:	/* MTC Quarter frame */
            case 0x03:	/* Song Select. */
            {
                BYTE buf[2];
                buf[0] = evt;
                buf[1] = d1;
                snd_seq_ev_set_sysex(&event, sizeof(buf), buf);
                break;
            }
            case 0x02:	/* Song Position Pointer. */
            {
                BYTE buf[3];
                buf[0] = evt;
                buf[1] = d1;
                buf[2] = d2;
                snd_seq_ev_set_sysex(&event, sizeof(buf), buf);
                break;
            }
            }
            break;
        }
        if (handled)
        {
            seq_lock();
            snd_seq_event_output_direct(dest->seq, &event);
            seq_unlock();
        }
        break;
    }
    default:
        WARN("Technology not supported (yet) %d !\n", dest->caps.wTechnology);
        return MMSYSERR_NOTENABLED;
    }

    return MMSYSERR_NOERROR;
}

static UINT midi_out_long_data(WORD dev_id, MIDIHDR *hdr, UINT hdr_size, struct notify_context *notify)
{
    struct midi_dest *dest;
    int len_add = 0;
    BYTE *data, *new_data = NULL;
    snd_seq_event_t event;

    TRACE("(%04X, %p, %08X);\n", dev_id, hdr, hdr_size);

    /* Note: MS doc does not say much about the dwBytesRecorded member of the MIDIHDR structure
     * but it seems to be used only for midi input.
     * Taking a look at the WAVEHDR structure (which is quite similar) confirms this assumption.
     */

    if (dev_id >= num_dests) return MMSYSERR_BADDEVICEID;
    dest = dests + dev_id;

    if (!dest->bEnabled) return MIDIERR_NODEVICE;

    if (dest->seq == NULL)
    {
        WARN("can't play !\n");
        return MIDIERR_NODEVICE;
    }

    data = (BYTE*)hdr->lpData;

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
     * If the latest is true, then the following WARNing will fire up
     */
    if (data[0] != 0xF0 || data[hdr->dwBufferLength - 1] != 0xF7)
    {
        WARN("Alleged system exclusive buffer is not correct\n\tPlease report with MIDI file\n");
        new_data = malloc(hdr->dwBufferLength + 2);
    }

    TRACE("dwBufferLength=%u !\n", hdr->dwBufferLength);
    TRACE("                 %02X %02X %02X ... %02X %02X %02X\n",
          data[0], data[1], data[2], data[hdr->dwBufferLength-3],
          data[hdr->dwBufferLength-2], data[hdr->dwBufferLength-1]);

    switch (dest->caps.wTechnology)
    {
    case MOD_FMSYNTH:
        /* FIXME: I don't think there is much to do here */
        free(new_data);
        break;
    case MOD_MIDIPORT:
        if (data[0] != 0xF0)
        {
            /* Send start of System Exclusive */
            len_add = 1;
            new_data[0] = 0xF0;
            memcpy(new_data + 1, data, hdr->dwBufferLength);
            WARN("Adding missing 0xF0 marker at the beginning of system exclusive byte stream\n");
        }
        if (data[hdr->dwBufferLength-1] != 0xF7)
        {
            /* Send end of System Exclusive */
            if (!len_add)
                memcpy(new_data, data, hdr->dwBufferLength);
            new_data[hdr->dwBufferLength + len_add] = 0xF7;
            len_add++;
            WARN("Adding missing 0xF7 marker at the end of system exclusive byte stream\n");
        }
        snd_seq_ev_clear(&event);
        snd_seq_ev_set_direct(&event);
        snd_seq_ev_set_source(&event, dest->port_out);
        snd_seq_ev_set_subs(&event);
        snd_seq_ev_set_sysex(&event, hdr->dwBufferLength + len_add, new_data ? new_data : data);
        seq_lock();
        snd_seq_event_output_direct(dest->seq, &event);
        seq_unlock();
        free(new_data);
        break;
    default:
        WARN("Technology not supported (yet) %d !\n", dest->caps.wTechnology);
        free(new_data);
        return MMSYSERR_NOTENABLED;
    }

    hdr->dwFlags &= ~MHDR_INQUEUE;
    hdr->dwFlags |= MHDR_DONE;
    set_out_notify(notify, dest, dev_id, MOM_DONE, (DWORD_PTR)hdr, 0);
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

static UINT midi_out_get_volume(WORD dev_id, UINT* volume)
{
    if (!volume) return MMSYSERR_INVALPARAM;
    if (dev_id >= num_dests) return MMSYSERR_BADDEVICEID;

    *volume = 0xFFFFFFFF;
    return (dests[dev_id].caps.dwSupport & MIDICAPS_VOLUME) ? 0 : MMSYSERR_NOTSUPPORTED;
}

static UINT midi_out_reset(WORD dev_id)
{
    unsigned chn;

    TRACE("(%04X);\n", dev_id);

    if (dev_id >= num_dests) return MMSYSERR_BADDEVICEID;
    if (!dests[dev_id].bEnabled) return MIDIERR_NODEVICE;

    /* stop all notes */
    /* FIXME: check if 0x78B0 is channel dependent or not. I coded it so that
     * it's channel dependent...
     */
    for (chn = 0; chn < 16; chn++)
    {
        /* turn off every note */
        midi_out_data(dev_id, 0x7800 | MIDI_CMD_CONTROL | chn);
        /* remove sustain on all channels */
        midi_out_data(dev_id, (MIDI_CTL_SUSTAIN << 8) | MIDI_CMD_CONTROL | chn);
    }
    /* FIXME: the LongData buffers must also be returned to the app */
    return MMSYSERR_NOERROR;
}


NTSTATUS midi_out_message(void *args)
{
    struct midi_out_message_params *params = args;

    params->notify->send_notify = FALSE;

    switch (params->msg)
    {
    case DRVM_EXIT:
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
