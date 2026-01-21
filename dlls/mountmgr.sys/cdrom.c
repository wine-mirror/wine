/*
 * CD-ROM ioctls implementation
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1999, 2001, 2003 Eric Pouech
 * Copyright 2000 Andreas Mohr
 * Copyright 2005 Ivan Leo Puoti
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
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#ifdef HAVE_LINUX_CDROM_H
# include <linux/cdrom.h>
#endif
#ifdef HAVE_SYS_CDIO_H
# include <sys/cdio.h>
#endif
#ifdef __APPLE__
# include <libkern/OSByteOrder.h>
# include <sys/disk.h>
# include <IOKit/IOKitLib.h>
# include <IOKit/storage/IOMedia.h>
# include <IOKit/storage/IOCDMediaBSDClient.h>
# include <IOKit/storage/IODVDMediaBSDClient.h>
#endif

/* Linux defines these; other systems do not. */
#ifndef CD_SECS
# define CD_SECS    60 /* seconds per minute */
#endif
#ifndef CD_FRAMES
# define CD_FRAMES  75 /* frames per second */
#endif

#include "mountmgr.h"
#include "unixlib.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cdrom);

struct cdrom
{
    int fd;
    SUB_Q_CURRENT_POSITION pos;
    /* Some systems are very slow to read the TOC, so we cache it. */
    bool toc_valid;
    CDROM_TOC toc;
};

NTSTATUS cdrom_open( void *args )
{
    struct cdrom_open_params *params = args;
    struct cdrom *cdrom;

    if (!(cdrom = malloc( sizeof( *cdrom ))))
        return STATUS_NO_MEMORY;

    if ((cdrom->fd = open( params->unix_device, O_RDONLY )) < 0)
    {
        WARN( "failed to open %s: %s\n", params->unix_device, strerror(errno) );
        return errno_to_status( errno );
    }

    params->cdrom = cdrom;
    return STATUS_SUCCESS;
}

NTSTATUS cdrom_close( void *args )
{
    struct cdrom *cdrom = args;

    close( cdrom->fd );
    free( cdrom );
    return STATUS_SUCCESS;
}

static void frame_to_msf( unsigned char *m, unsigned int frame )
{
    m[2] = frame % CD_FRAMES;
    frame /= CD_FRAMES;
    m[1] = frame % CD_SECS;
    m[0] = frame / CD_SECS;
}

static unsigned int track_to_frame( const CDROM_TOC *toc, unsigned int track_idx )
{
    const TRACK_DATA *track = &toc->TrackData[track_idx - toc->FirstTrack];
    return ((unsigned int)track->Address[1] * CD_SECS + track->Address[2]) * CD_FRAMES + track->Address[3];
}

static NTSTATUS update_toc_cache( struct cdrom *cdrom )
{
#ifdef linux
    CDROM_TOC *toc = &cdrom->toc;
    struct cdrom_tochdr hdr;
    struct cdrom_tocentry entry;
    unsigned int size;

    cdrom->toc_valid = false;

    if (ioctl( cdrom->fd, CDROMREADTOCHDR, &hdr ) == -1)
    {
        WARN( "Failed to read TOC header: %s\n", strerror( errno ));
        return errno_to_status( errno );
    }

    toc->FirstTrack = hdr.cdth_trk0;
    toc->LastTrack = hdr.cdth_trk1;
    size = sizeof(toc->FirstTrack) + sizeof(toc->LastTrack)
            + (toc->LastTrack - toc->FirstTrack + 2) * sizeof(TRACK_DATA);
    toc->Length[0] = size >> 8;
    toc->Length[1] = size;

    TRACE( "first track %u, last track %u\n", toc->FirstTrack, toc->LastTrack );

    for (unsigned int i = toc->FirstTrack; i <= toc->LastTrack + 1; ++i)
    {
        TRACK_DATA *track = &toc->TrackData[i - toc->FirstTrack];

        if (i == toc->LastTrack + 1)
            entry.cdte_track = CDROM_LEADOUT;
        else
            entry.cdte_track = i;
        entry.cdte_format = CDROM_MSF;
        if (ioctl( cdrom->fd, CDROMREADTOCENTRY, &entry ) == -1)
        {
            WARN( "Failed to read TOC entry %u: %s\n", entry.cdte_track, strerror( errno ));
            return errno_to_status( errno );
        }
        track->Control = entry.cdte_ctrl;
        track->Adr = entry.cdte_adr;
        /* marking last track with leadout value as index */
        track->TrackNumber = entry.cdte_track;
        track->Address[0] = 0;
        track->Address[1] = entry.cdte_addr.msf.minute;
        track->Address[2] = entry.cdte_addr.msf.second;
        track->Address[3] = entry.cdte_addr.msf.frame;
    }
    cdrom->toc_valid = true;
    return STATUS_SUCCESS;
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__DragonFly__)
    CDROM_TOC *toc = &cdrom->toc;
    struct ioc_toc_header hdr;
    struct ioc_read_toc_entry entry;
    struct cd_toc_entry toc_buffer;
    unsigned int size;

    cdrom->toc_valid = 0;

    if (ioctl( cdrom->fd, CDIOREADTOCHEADER, &hdr ) == -1)
    {
        WARN( "Failed to read TOC header: %s\n", strerror( errno ));
        return errno_to_status( errno );
    }
    toc->FirstTrack = hdr.starting_track;
    toc->LastTrack  = hdr.ending_track;
    size = sizeof(toc->FirstTrack) + sizeof(toc->LastTrack)
            + (toc->LastTrack - toc->FirstTrack + 2) * sizeof(TRACK_DATA);
    toc->Length[0] = size >> 8;
    toc->Length[1] = size;

    TRACE( "first track %u, last track %u\n", toc->FirstTrack, toc->LastTrack );

    for (unsigned int i = toc->FirstTrack; i <= toc->LastTrack + 1; i++)
    {
        TRACK_DATA *track = &toc->TrackData[i - toc->FirstTrack];

#define LEADOUT 0xaa
        if (i == toc->LastTrack + 1)
            entry.starting_track = LEADOUT;
        else
            entry.starting_track = i;
        memset( &toc_buffer, 0, sizeof(toc_buffer) );
        entry.address_format = CD_MSF_FORMAT;
        entry.data_len = sizeof(toc_buffer);
        entry.data = &toc_buffer;
        if (ioctl( cdrom->fd, CDIOREADTOCENTRYS, &entry ) == -1)
        {
            WARN( "Failed to read TOC entry %u: %s\n", entry.starting_track, strerror( errno ));
            return errno_to_status( errno );
        }
        track->Control = toc_buffer.control;
        track->Adr = toc_buffer.addr_type;
        /* marking last track with leadout value as index */
        track->TrackNumber = entry.starting_track;
        track->Address[0] = 0;
        track->Address[1] = toc_buffer.addr.msf.minute;
        track->Address[2] = toc_buffer.addr.msf.second;
        track->Address[3] = toc_buffer.addr.msf.frame;
    }
    cdrom->toc_valid = true;
    return STATUS_SUCCESS;
#elif defined(__APPLE__)
    CDROM_TOC *toc = &cdrom->toc;
    dk_cd_read_toc_t hdr;

    cdrom->toc_valid = false;

    memset( &hdr, 0, sizeof(hdr) );
    hdr.buffer = toc;
    hdr.bufferLength = sizeof(*toc);
    if (ioctl( cdrom->fd, DKIOCCDREADTOC, &hdr ) == -1)
    {
        WARN( "Failed to read TOC: %s\n", strerror( errno ));
        return errno_to_status( errno );
    }
    for (unsigned int i = toc->FirstTrack; i <= toc->LastTrack + 1; i++)
    {
        /* convert address format */
        TRACK_DATA *track = &toc->TrackData[i - toc->FirstTrack];
        unsigned int frame = MAKELONG( MAKEWORD( track->Address[3], track->Address[2] ),
                                       MAKEWORD( track->Address[1], track->Address[0] ));
        frame_to_msf( &track->Address[1], frame );
        track->Address[0] = 0;
    }

    cdrom->toc_valid = true;
    return STATUS_SUCCESS;
#else
    FIXME( "not implemented for this platform\n" );
    return STATUS_NOT_SUPPORTED;
#endif
}

static NTSTATUS read_toc( struct cdrom *cdrom, CDROM_TOC *toc )
{
    NTSTATUS status;

    if (!cdrom->toc_valid && (status = update_toc_cache( cdrom )))
        return status;
    *toc = cdrom->toc;
    return STATUS_SUCCESS;
}

static NTSTATUS seek_audio_msf( struct cdrom *cdrom, const CDROM_SEEK_AUDIO_MSF *params )
{
    unsigned int i, frame;
    SUB_Q_CURRENT_POSITION *pos;
    NTSTATUS status;
#if defined(linux)
    struct cdrom_subchnl sc;
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__DragonFly__)
    struct ioc_play_msf msf;
    struct ioc_read_subchannel read_sc;
    struct cd_sub_channel_info sc;
    int final_frame;
#endif

    /* Use the information on the TOC to compute the new current
     * position, which is shadowed on the cache. */
    frame = (params->M * CD_SECS + params->S) * CD_FRAMES + params->F;

    if (!cdrom->toc_valid && (status = update_toc_cache( cdrom )))
        return status;

    for (i = cdrom->toc.FirstTrack; i <= cdrom->toc.LastTrack + 1; ++i)
    {
        if (track_to_frame( &cdrom->toc, i ) > frame)
            break;
    }
    if (i <= cdrom->toc.FirstTrack || i > cdrom->toc.LastTrack + 1)
        return STATUS_INVALID_PARAMETER;
    --i;

    pos = &cdrom->pos;
    pos->FormatCode = IOCTL_CDROM_CURRENT_POSITION;
    pos->Control = cdrom->toc.TrackData[i - cdrom->toc.FirstTrack].Control;
    pos->ADR = cdrom->toc.TrackData[i - cdrom->toc.FirstTrack].Adr;
    pos->TrackNumber = cdrom->toc.TrackData[i - cdrom->toc.FirstTrack].TrackNumber;
    pos->IndexNumber = 0; /* FIXME: where do they keep these? */
    pos->AbsoluteAddress[0] = 0;
    pos->AbsoluteAddress[1] = cdrom->toc.TrackData[i - cdrom->toc.FirstTrack].Address[1];
    pos->AbsoluteAddress[2] = cdrom->toc.TrackData[i - cdrom->toc.FirstTrack].Address[2];
    pos->AbsoluteAddress[3] = cdrom->toc.TrackData[i - cdrom->toc.FirstTrack].Address[3];
    frame -= track_to_frame( &cdrom->toc, i );
    pos->TrackRelativeAddress[0] = 0;
    frame_to_msf( &pos->TrackRelativeAddress[1], frame );

    /* If playing, then issue a seek command, otherwise do nothing */
#ifdef linux
    sc.cdsc_format = CDROM_MSF;

    if (ioctl( cdrom->fd, CDROMSUBCHNL, &sc ) == -1)
    {
        TRACE( "failed to read subchannel data: %s\n", strerror( errno ));
        cdrom->toc_valid = false;
        return errno_to_status( errno );
    }

    if (sc.cdsc_audiostatus == CDROM_AUDIO_PLAY)
    {
        struct cdrom_msf0 msf;
        msf.minute = params->M;
        msf.second = params->S;
        msf.frame = params->F;
        if (ioctl( cdrom->fd, CDROMSEEK, &msf ) < 0)
            return errno_to_status( errno );
    }
    return STATUS_SUCCESS;
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__DragonFly__)
    read_sc.address_format = CD_MSF_FORMAT;
    read_sc.track = 0;
    read_sc.data_len = sizeof(sc);
    read_sc.data = &sc;
    read_sc.data_format = CD_CURRENT_POSITION;

    if (ioctl( cdrom->fd, CDIOCREADSUBCHANNEL, &read_sc ) == -1)
    {
        TRACE( "failed to read subchannel data: %s\n", strerror( errno ));
        cdrom->toc_valid = false;
        return errno_to_status( errno );
    }
    if (sc.header.audio_status == CD_AS_PLAY_IN_PROGRESS)
    {
        msf.start_m = params->M;
        msf.start_s = params->S;
        msf.start_f = params->F;
        final_frame = track_to_frame( &cdrom->toc, cdrom->toc.LastTrack + 1 ) - 1;
        frame_to_msf( msf.end_m, final_frame );
        if (ioctl( cdrom->fd, CDIOCPLAYMSF, &msf ) < 0)
            return errno_to_status( errno );
    }
    return STATUS_SUCCESS;
#else
    FIXME( "not implemented for this platform\n" );
    return STATUS_NOT_SUPPORTED;
#endif
}

static NTSTATUS play_audio_msf( struct cdrom *cdrom, const CDROM_PLAY_AUDIO_MSF *params )
{
#ifdef linux
    struct cdrom_msf msf;

    msf.cdmsf_min0 = params->StartingM;
    msf.cdmsf_sec0 = params->StartingS;
    msf.cdmsf_frame0 = params->StartingF;
    msf.cdmsf_min1 = params->EndingM;
    msf.cdmsf_sec1 = params->EndingS;
    msf.cdmsf_frame1 = params->EndingF;

    if (ioctl( cdrom->fd, CDROMSTART ) == -1)
    {
        WARN( "failed to start: %s\n", strerror( errno ));
        return errno_to_status( errno );
    }
    if (ioctl( cdrom->fd, CDROMPLAYMSF, &msf ) == -1)
    {
        WARN( "failed to play: %s\n", strerror( errno ));
        return errno_to_status( errno );
    }
    TRACE( "playing %d:%d:%d to %d:%d:%d\n",
            msf.cdmsf_min0, msf.cdmsf_sec0, msf.cdmsf_frame0,
            msf.cdmsf_min1, msf.cdmsf_sec1, msf.cdmsf_frame1 );
    return STATUS_SUCCESS;
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__DragonFly__)
    struct ioc_play_msf msf;

    msf.start_m = params->StartingM;
    msf.start_s = params->StartingS;
    msf.start_f = params->StartingF;
    msf.end_m = params->EndingM;
    msf.end_s = params->EndingS;
    msf.end_f = params->EndingF;

    if (ioctl( cdrom->fd, CDIOCSTART, NULL ) == -1)
    {
        WARN( "failed to start: %s\n", strerror( errno ));
        return errno_to_status( errno );
    }
    if (ioctl( cdrom->fd, CDROMPLAYMSF, &msf ) == -1)
    {
        WARN( "failed to play: %s\n", strerror( errno ));
        return errno_to_status( errno );
    }
    TRACE( "playing %d:%d:%d to %d:%d:%d\n",
            msf.cdmsf_min0, msf.cdmsf_sec0, msf.cdmsf_frame0,
            msf.cdmsf_min1, msf.cdmsf_sec1, msf.cdmsf_frame1 );
    return STATUS_SUCCESS;
#else
    FIXME( "not implemented for this platform\n" );
    return STATUS_NOT_SUPPORTED;
#endif
}

static NTSTATUS pause_audio( struct cdrom *cdrom )
{
#ifdef linux
    if (ioctl( cdrom->fd, CDROMPAUSE ))
        return errno_to_status( errno );
    return STATUS_SUCCESS;
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__DragonFly__)
    if (ioctl( cdrom->fd, CDIOCPAUSE, NULL ))
        return errno_to_status( errno );
    return STATUS_SUCCESS;
#else
    FIXME( "not implemented for this platform\n" );
    return STATUS_NOT_SUPPORTED;
#endif
}

static NTSTATUS resume_audio( struct cdrom *cdrom )
{
#ifdef linux
    if (ioctl( cdrom->fd, CDROMRESUME ))
        return errno_to_status( errno );
    return STATUS_SUCCESS;
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__DragonFly__)
    if (ioctl( cdrom->fd, CDIOCRESUME, NULL ))
        return errno_to_status( errno );
    return STATUS_SUCCESS;
#else
    FIXME( "not implemented for this platform\n" );
    return STATUS_NOT_SUPPORTED;
#endif
}

static NTSTATUS stop_audio( struct cdrom *cdrom )
{
    /* We might be about to change media. */
    cdrom->toc_valid = false;
#ifdef linux
    if (ioctl( cdrom->fd, CDROMSTOP ))
        return errno_to_status( errno );
    return STATUS_SUCCESS;
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__DragonFly__)
    if (ioctl( cdrom->fd, CDIOCSTOP, NULL ))
        return errno_to_status( errno );
    return STATUS_SUCCESS;
#else
    FIXME( "not implemented for this platform\n" );
    return STATUS_NOT_SUPPORTED;
#endif
}

static NTSTATUS get_volume( struct cdrom *cdrom, VOLUME_CONTROL *vc )
{
#if defined(linux)
    struct cdrom_volctrl volc;

    if (ioctl( cdrom->fd, CDROMVOLREAD, &volc ) == -1)
        return errno_to_status( errno );
    vc->PortVolume[0] = volc.channel0;
    vc->PortVolume[1] = volc.channel1;
    vc->PortVolume[2] = volc.channel2;
    vc->PortVolume[3] = volc.channel3;
    return STATUS_SUCCESS;
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__DragonFly__)
    struct ioc_vol volc;

    if (ioctl( cdrom->fd, CDIOCGETVOL, &volc ) == -1)
        return errno_to_status( errno );
    vc->PortVolume[0] = volc.vol[0];
    vc->PortVolume[1] = volc.vol[1];
    vc->PortVolume[2] = volc.vol[2];
    vc->PortVolume[3] = volc.vol[3];
    return STATUS_SUCCESS;
#else
    FIXME( "not implemented for this platform\n" );
    return STATUS_NOT_SUPPORTED;
#endif
}

static NTSTATUS set_volume( struct cdrom *cdrom, const VOLUME_CONTROL *vc )
{
#if defined(linux)
    struct cdrom_volctrl volc;

    cdrom->toc_valid = false;
    volc.channel0 = vc->PortVolume[0];
    volc.channel1 = vc->PortVolume[1];
    volc.channel2 = vc->PortVolume[2];
    volc.channel3 = vc->PortVolume[3];
    if (ioctl( cdrom->fd, CDROMVOLCTRL, &volc ) == -1)
        return errno_to_status( errno );
    return STATUS_SUCCESS;
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__DragonFly__)
    struct ioc_vol volc;

    cdrom->toc_valid = false;
    volc.vol[0] = vc->PortVolume[0];
    volc.vol[1] = vc->PortVolume[1];
    volc.vol[2] = vc->PortVolume[2];
    volc.vol[3] = vc->PortVolume[3];
    if (ioctl( cdrom->fd, CDIOCSETVOL, &volc ) == -1)
        return errno_to_status( errno );
    return STATUS_SUCCESS;
#else
    FIXME( "not implemented for this platform\n" );
    return STATUS_NOT_SUPPORTED;
#endif
}

NTSTATUS cdrom_ioctl( void *args )
{
    struct cdrom_ioctl_params *params = args;
    unsigned int code = params->code;

    TRACE( "ioctl %#x\n", code );

    params->ret_size = 0;

    switch (code)
    {
        case IOCTL_CDROM_GET_VOLUME:
            if (params->output_size < sizeof(VOLUME_CONTROL))
                return STATUS_BUFFER_TOO_SMALL;
            params->ret_size = sizeof(VOLUME_CONTROL);
            return get_volume( params->cdrom, params->output );

        case IOCTL_CDROM_PAUSE_AUDIO:
            return pause_audio( params->cdrom );

        case IOCTL_CDROM_PLAY_AUDIO_MSF:
            if (params->input_size < sizeof(CDROM_PLAY_AUDIO_MSF))
                return STATUS_INFO_LENGTH_MISMATCH;
            return play_audio_msf( params->cdrom, params->input );

        case IOCTL_CDROM_READ_TOC:
            if (params->output_size < sizeof(CDROM_TOC))
                return STATUS_BUFFER_TOO_SMALL;
            params->ret_size = sizeof(CDROM_TOC);
            return read_toc( params->cdrom, params->output );

        case IOCTL_CDROM_RESUME_AUDIO:
            return resume_audio( params->cdrom );

        case IOCTL_CDROM_SEEK_AUDIO_MSF:
            if (params->input_size < sizeof(CDROM_SEEK_AUDIO_MSF))
                return STATUS_INFO_LENGTH_MISMATCH;
            return seek_audio_msf( params->cdrom, params->input );

        case IOCTL_CDROM_SET_VOLUME:
            if (params->input_size < sizeof(VOLUME_CONTROL))
                return STATUS_INFO_LENGTH_MISMATCH;
            return set_volume( params->cdrom, params->input );

        case IOCTL_CDROM_STOP_AUDIO:
            return stop_audio( params->cdrom );

        default:
            FIXME("Unsupported ioctl %#x (device=%#x access=%#x func=%#x method=%#x)\n",
                  code, code >> 16, (code >> 14) & 3, (code >> 2) & 0xfff, code & 3);
            return STATUS_NOT_SUPPORTED;
    }
}
