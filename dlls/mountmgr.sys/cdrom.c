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

#ifdef __APPLE__
static void frame_to_msf( unsigned char *m, unsigned int frame )
{
    m[2] = frame % CD_FRAMES;
    frame /= CD_FRAMES;
    m[1] = frame % CD_SECS;
    m[0] = frame / CD_SECS;
}
#endif

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

NTSTATUS cdrom_ioctl( void *args )
{
    struct cdrom_ioctl_params *params = args;
    unsigned int code = params->code;

    TRACE( "ioctl %#x\n", code );

    params->ret_size = 0;

    switch (code)
    {
        case IOCTL_CDROM_READ_TOC:
            if (params->output_size < sizeof(CDROM_TOC))
                return STATUS_BUFFER_TOO_SMALL;
            params->ret_size = sizeof(CDROM_TOC);
            return read_toc( params->cdrom, params->output );

        default:
            FIXME("Unsupported ioctl %#x (device=%#x access=%#x func=%#x method=%#x)\n",
                  code, code >> 16, (code >> 14) & 3, (code >> 2) & 0xfff, code & 3);
            return STATUS_NOT_SUPPORTED;
    }
}
