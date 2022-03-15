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
    params->num_dests = num_dests;
    params->num_srcs = num_srcs;
    params->dests = dests;
    params->srcs = srcs;

    TRACE("End\n");

    return STATUS_SUCCESS;
}
