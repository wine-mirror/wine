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
#include <sys/stat.h>
#ifdef MAJOR_IN_MKDEV
# include <sys/mkdev.h>
#elif defined(MAJOR_IN_SYSMACROS)
# include <sys/sysmacros.h>
#endif
#include <sys/types.h>
#include <sys/ioctl.h>
#ifdef HAVE_SCSI_SG_H
# include <scsi/sg.h>
#endif
#ifdef HAVE_SCSI_SCSI_H
# include <scsi/scsi.h>
# undef REASSIGN_BLOCKS  /* avoid conflict with winioctl.h */
# undef FAILED           /* avoid conflict with winerror.h */
#endif
#ifdef HAVE_SCSI_SCSI_IOCTL_H
# include <scsi/scsi_ioctl.h>
#endif
#ifdef HAVE_LINUX_CDROM_H
# include <linux/cdrom.h>
#endif
#ifdef HAVE_LINUX_MAJOR_H
# include <linux/major.h>
#endif
#ifdef HAVE_SYS_CDIO_H
# include <sys/cdio.h>
#endif
#ifdef HAVE_SYS_SCSIIO_H
# include <sys/scsiio.h>
#endif
#ifdef __APPLE__
# include <libkern/OSByteOrder.h>
# include <sys/disk.h>
# include <IOKit/IOKitLib.h>
# include <IOKit/storage/IOMedia.h>
# include <IOKit/storage/IOCDMediaBSDClient.h>
# include <IOKit/storage/IODVDMediaBSDClient.h>
# include <IOKit/scsi/SCSITask.h>
# include <IOKit/scsi/SCSICmds_REQUEST_SENSE_Defs.h>
# define SENSEBUFLEN kSenseDefaultSize

typedef struct
{
    uint32_t attribute;
    uint32_t timeout;
    uint32_t response;
    uint32_t status;
    uint8_t direction;
    uint8_t cdbSize;
    uint8_t reserved0144[2];
    uint8_t cdb[16];
    void *buffer;
    uint64_t bufferSize;
    void *sense;
    uint64_t senseLen;
} dk_scsi_command_t;

typedef struct
{
    uint64_t bus;
    uint64_t port;
    uint64_t target;
    uint64_t lun;
} dk_scsi_identify_t;

#define DKIOCSCSICOMMAND _IOWR('d', 253, dk_scsi_command_t)
#define DKIOCSCSIIDENTIFY _IOR('d', 254, dk_scsi_identify_t)

#endif

/* Linux defines these; other systems do not. */
#ifndef CD_SECS
# define CD_SECS    60 /* seconds per minute */
#endif
#ifndef CD_FRAMES
# define CD_FRAMES  75 /* frames per second */
#endif

#include "mountmgr.h"
#include "ddk/ntddcdvd.h"
#include "unixlib.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cdrom);

/* The documented format of DVD_LAYER_DESCRIPTOR is wrong. Even the format in
 * the DDK's header is wrong. There are four bytes at the start defined by
 * MMC-5. The first two are the size of the structure in big-endian order as
 * defined by MMC-5. The other two are reserved.
 */
struct dvd_layer_descriptor
{
    DVD_DESCRIPTOR_HEADER Header;
    DVD_LAYER_DESCRIPTOR Descriptor;
    UCHAR Padding;
};
C_ASSERT(sizeof(struct dvd_layer_descriptor) == 22);

struct dvd_manufacturer_descriptor
{
    DVD_DESCRIPTOR_HEADER Header;
    DVD_MANUFACTURER_DESCRIPTOR Descriptor;
    UCHAR Padding;
};
C_ASSERT(sizeof(struct dvd_manufacturer_descriptor) == 2053);

typedef struct _SCSI_PASS_THROUGH32
{
    USHORT Length;
    UCHAR ScsiStatus;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    UCHAR CdbLength;
    UCHAR SenseInfoLength;
    UCHAR DataIn;
    ULONG DataTransferLength;
    ULONG TimeOutValue;
    ULONG DataBufferOffset;
    ULONG SenseInfoOffset;
    UCHAR Cdb[16];
} SCSI_PASS_THROUGH32;

typedef struct _SCSI_PASS_THROUGH_DIRECT32
{
    USHORT Length;
    UCHAR ScsiStatus;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    UCHAR CdbLength;
    UCHAR SenseInfoLength;
    UCHAR DataIn;
    ULONG DataTransferLength;
    ULONG TimeOutValue;
    ULONG DataBuffer;
    ULONG SenseInfoOffset;
    UCHAR Cdb[16];
} SCSI_PASS_THROUGH_DIRECT32;

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
        frame_to_msf( &msf.end_m, final_frame );
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
    if (ioctl( cdrom->fd, CDIOCPLAYMSF, &msf ) == -1)
    {
        WARN( "failed to play: %s\n", strerror( errno ));
        return errno_to_status( errno );
    }
    TRACE( "playing %d:%d:%d to %d:%d:%d\n",
            msf.start_m, msf.start_s, msf.start_f,
            msf.end_m, msf.end_s, msf.end_f );
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

static NTSTATUS read_q_channel( struct cdrom *cdrom, const CDROM_SUB_Q_DATA_FORMAT *fmt,
        unsigned int data_size, SUB_Q_CHANNEL_DATA *data, unsigned int *ret_size )
{
#ifdef linux
    SUB_Q_HEADER *hdr = &data->CurrentPosition.Header;
    struct cdrom_subchnl sc;

    sc.cdsc_format = CDROM_MSF;
    if (ioctl( cdrom->fd, CDROMSUBCHNL, &sc ) == -1)
    {
        hdr->AudioStatus = AUDIO_STATUS_NO_STATUS;
        TRACE( "failed to read subchannel data: %s\n", strerror( errno ));
        cdrom->toc_valid = false;
        return errno_to_status( errno );
    }

    hdr->AudioStatus = AUDIO_STATUS_NOT_SUPPORTED;

    switch (sc.cdsc_audiostatus)
    {
        case CDROM_AUDIO_INVALID:
            cdrom->toc_valid = false;
            hdr->AudioStatus = AUDIO_STATUS_NOT_SUPPORTED;
            break;
        case CDROM_AUDIO_NO_STATUS:
            cdrom->toc_valid = false;
            hdr->AudioStatus = AUDIO_STATUS_NO_STATUS;
            break;
        case CDROM_AUDIO_PLAY:
            hdr->AudioStatus = AUDIO_STATUS_IN_PROGRESS;
            break;
        case CDROM_AUDIO_PAUSED:
            hdr->AudioStatus = AUDIO_STATUS_PAUSED;
            break;
        case CDROM_AUDIO_COMPLETED:
            hdr->AudioStatus = AUDIO_STATUS_PLAY_COMPLETE;
            break;
        case CDROM_AUDIO_ERROR:
            hdr->AudioStatus = AUDIO_STATUS_PLAY_ERROR;
            break;
        default:
            FIXME( "unhandled status %#x\n", sc.cdsc_audiostatus );
            hdr->AudioStatus = AUDIO_STATUS_NO_STATUS;
    }
    switch (fmt->Format)
    {
        case IOCTL_CDROM_CURRENT_POSITION:
            if (data_size < sizeof(SUB_Q_CURRENT_POSITION))
                return STATUS_BUFFER_TOO_SMALL;

            if (hdr->AudioStatus == AUDIO_STATUS_IN_PROGRESS)
            {
                data->CurrentPosition.FormatCode = IOCTL_CDROM_CURRENT_POSITION;
                data->CurrentPosition.Control = sc.cdsc_ctrl;
                data->CurrentPosition.ADR = sc.cdsc_adr;
                data->CurrentPosition.TrackNumber = sc.cdsc_trk;
                data->CurrentPosition.IndexNumber = sc.cdsc_ind;

                data->CurrentPosition.AbsoluteAddress[0] = 0;
                data->CurrentPosition.AbsoluteAddress[1] = sc.cdsc_absaddr.msf.minute;
                data->CurrentPosition.AbsoluteAddress[2] = sc.cdsc_absaddr.msf.second;
                data->CurrentPosition.AbsoluteAddress[3] = sc.cdsc_absaddr.msf.frame;

                data->CurrentPosition.TrackRelativeAddress[0] = 0;
                data->CurrentPosition.TrackRelativeAddress[1] = sc.cdsc_reladdr.msf.minute;
                data->CurrentPosition.TrackRelativeAddress[2] = sc.cdsc_reladdr.msf.second;
                data->CurrentPosition.TrackRelativeAddress[3] = sc.cdsc_reladdr.msf.frame;

                cdrom->pos = data->CurrentPosition;
            }
            else
            {
                cdrom->pos.Header = *hdr;
                data->CurrentPosition = cdrom->pos;
            }
            *ret_size = sizeof(SUB_Q_CURRENT_POSITION);
            return STATUS_SUCCESS;

        case IOCTL_CDROM_MEDIA_CATALOG:
        {
            struct cdrom_mcn mcn;

            if (data_size < sizeof(SUB_Q_MEDIA_CATALOG_NUMBER))
                return STATUS_BUFFER_TOO_SMALL;

            data->MediaCatalog.FormatCode = IOCTL_CDROM_MEDIA_CATALOG;

            if (ioctl( cdrom->fd, CDROM_GET_MCN, &mcn ) == -1)
                return errno_to_status( errno );

            data->MediaCatalog.FormatCode = IOCTL_CDROM_MEDIA_CATALOG;
            data->MediaCatalog.Mcval = 0; /* FIXME */
            memcpy( data->MediaCatalog.MediaCatalog, mcn.medium_catalog_number, 14 );
            data->MediaCatalog.MediaCatalog[14] = 0;
            *ret_size = sizeof(SUB_Q_MEDIA_CATALOG_NUMBER);
            return STATUS_SUCCESS;
        }

        default:
            FIXME( "unhandled format %#x\n", fmt->Format );
            return STATUS_NOT_IMPLEMENTED;
    }
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__DragonFly__)
    SUB_Q_HEADER *hdr = &data->CurrentPosition.Header;
    struct ioc_read_subchannel read_sc;
    struct cd_sub_channel_info sc;

    read_sc.address_format = CD_MSF_FORMAT;
    read_sc.track = 0;
    read_sc.data_len = sizeof(sc);
    read_sc.data = &sc;
    switch (fmt->Format)
    {
        case IOCTL_CDROM_CURRENT_POSITION:
            if (data_size < sizeof(SUB_Q_CURRENT_POSITION))
                return STATUS_BUFFER_TOO_SMALL;
            read_sc.data_format = CD_CURRENT_POSITION;
            break;
        case IOCTL_CDROM_MEDIA_CATALOG:
            if (data_size < sizeof(SUB_Q_MEDIA_CATALOG_NUMBER))
                return STATUS_BUFFER_TOO_SMALL;
            read_sc.data_format = CD_MEDIA_CATALOG;
            break;
        case IOCTL_CDROM_TRACK_ISRC:
            if (data_size < sizeof(SUB_Q_TRACK_ISRC))
                return STATUS_BUFFER_TOO_SMALL;
            read_sc.data_format = CD_TRACK_INFO;
            sc.what.track_info.track_number = data->TrackIsrc.Track;
            break;
        default:
            FIXME( "unhandled format %#x\n", fmt->Format );
            return STATUS_NOT_IMPLEMENTED;
    }
    if (ioctl( cdrom->fd, CDIOCREADSUBCHANNEL, &read_sc ) == -1)
    {
        hdr->AudioStatus = AUDIO_STATUS_NO_STATUS;
        TRACE( "failed to read subchannel data: %s\n", strerror( errno ));
        cdrom->toc_valid = false;
        return errno_to_status( errno );
    }

    hdr->AudioStatus = AUDIO_STATUS_NOT_SUPPORTED;

    switch (sc.header.audio_status)
    {
        case CD_AS_AUDIO_INVALID:
            cdrom->toc_valid = false;
            hdr->AudioStatus = AUDIO_STATUS_NOT_SUPPORTED;
            break;
        case CD_AS_NO_STATUS:
            cdrom->toc_valid = false;
            hdr->AudioStatus = AUDIO_STATUS_NO_STATUS;
            break;
        case CD_AS_PLAY_IN_PROGRESS:
            hdr->AudioStatus = AUDIO_STATUS_IN_PROGRESS;
            break;
        case CD_AS_PLAY_PAUSED:
            hdr->AudioStatus = AUDIO_STATUS_PAUSED;
            break;
        case CD_AS_PLAY_COMPLETED:
            hdr->AudioStatus = AUDIO_STATUS_PLAY_COMPLETE;
            break;
        case CD_AS_PLAY_ERROR:
            hdr->AudioStatus = AUDIO_STATUS_PLAY_ERROR;
            break;
        default:
            FIXME( "unhandled status %#x\n", sc.header.audio_status );
            hdr->AudioStatus = AUDIO_STATUS_NO_STATUS;
    }

    switch (fmt->Format)
    {
        case IOCTL_CDROM_CURRENT_POSITION:
            if (hdr->AudioStatus==AUDIO_STATUS_IN_PROGRESS)
            {
                data->CurrentPosition.FormatCode = IOCTL_CDROM_CURRENT_POSITION;
                data->CurrentPosition.Control = sc.what.position.control;
                data->CurrentPosition.ADR = sc.what.position.addr_type;
                data->CurrentPosition.TrackNumber = sc.what.position.track_number;
                data->CurrentPosition.IndexNumber = sc.what.position.index_number;

                data->CurrentPosition.AbsoluteAddress[0] = 0;
                data->CurrentPosition.AbsoluteAddress[1] = sc.what.position.absaddr.msf.minute;
                data->CurrentPosition.AbsoluteAddress[2] = sc.what.position.absaddr.msf.second;
                data->CurrentPosition.AbsoluteAddress[3] = sc.what.position.absaddr.msf.frame;
                data->CurrentPosition.TrackRelativeAddress[0] = 0;
                data->CurrentPosition.TrackRelativeAddress[1] = sc.what.position.reladdr.msf.minute;
                data->CurrentPosition.TrackRelativeAddress[2] = sc.what.position.reladdr.msf.second;
                data->CurrentPosition.TrackRelativeAddress[3] = sc.what.position.reladdr.msf.frame;
                cdrom->pos = data->CurrentPosition;
            }
            else
            {
                cdrom->pos.Header = *hdr;
                data->CurrentPosition = cdrom->pos;
            }
            *ret_size = sizeof(SUB_Q_CURRENT_POSITION);
            return STATUS_SUCCESS;

        case IOCTL_CDROM_MEDIA_CATALOG:
            data->MediaCatalog.FormatCode = IOCTL_CDROM_MEDIA_CATALOG;
            data->MediaCatalog.Mcval = sc.what.media_catalog.mc_valid;
            memcpy( data->MediaCatalog.MediaCatalog, sc.what.media_catalog.mc_number, 15 );
            *ret_size = sizeof(SUB_Q_MEDIA_CATALOG_NUMBER);
            return STATUS_SUCCESS;

        case IOCTL_CDROM_TRACK_ISRC:
            data->TrackIsrc.FormatCode = IOCTL_CDROM_TRACK_ISRC;
            data->TrackIsrc.Tcval = sc.what.track_info.ti_valid;
            memcpy( data->TrackIsrc.TrackIsrc, sc.what.track_info.ti_number, 15 );
            *ret_size = sizeof(SUB_Q_TRACK_ISRC);
            break;
    }
#elif defined(__APPLE__)
    SUB_Q_HEADER *hdr = &data->CurrentPosition.Header;

    /* We need IOCDAudioControl for IOCTL_CDROM_CURRENT_POSITION */
    if (fmt->Format == IOCTL_CDROM_CURRENT_POSITION)
    {
        FIXME( "IOCTL_CDROM_CURRENT_POSITION is not implemented\n" );
        return STATUS_NOT_SUPPORTED;
    }
    /* No IOCDAudioControl support; just set the audio status to none */
    hdr->AudioStatus = AUDIO_STATUS_NO_STATUS;
    switch (fmt->Format)
    {
        case IOCTL_CDROM_MEDIA_CATALOG:
        {
            dk_cd_read_mcn_t mcn;

            if (data_size < sizeof(SUB_Q_MEDIA_CATALOG_NUMBER))
                return STATUS_BUFFER_TOO_SMALL;

            if (ioctl( cdrom->fd, DKIOCCDREADMCN, &mcn ) == -1)
                return errno_to_status( errno );
            memcpy( data->MediaCatalog.MediaCatalog, mcn.mcn, kCDMCNMaxLength );
            data->MediaCatalog.Mcval = 1;
            *ret_size = sizeof(SUB_Q_MEDIA_CATALOG_NUMBER);
            return STATUS_SUCCESS;
        }
        case IOCTL_CDROM_TRACK_ISRC:
        {
            dk_cd_read_isrc_t isrc;

            if (data_size < sizeof(SUB_Q_TRACK_ISRC))
                return STATUS_BUFFER_TOO_SMALL;

            isrc.track = fmt->Track;
            if (ioctl( cdrom->fd, DKIOCCDREADISRC, &isrc ) == -1)
                return errno_to_status( errno );
            memcpy( data->TrackIsrc.TrackIsrc, isrc.isrc, kCDISRCMaxLength );
            data->TrackIsrc.Tcval = 1;
            data->TrackIsrc.Track = isrc.track;
            *ret_size = sizeof(SUB_Q_TRACK_ISRC);
            return STATUS_SUCCESS;
        }
        default:
            FIXME( "unhandled format %#x\n", fmt->Format );
            return STATUS_NOT_IMPLEMENTED;
    }
#else
    FIXME( "not implemented for this platform\n" );
    return STATUS_NOT_SUPPORTED;
#endif
}

/* Some features of this IOCTL are rather poorly documented and
 * not really intuitive either:
 *
 *   1. Although the DiskOffset parameter is meant to be a
 *      byte offset into the disk, it is in fact the sector
 *      number multiplied by 2048 regardless of the actual
 *      sector size.
 *
 *   2. The least significant 11 bits of DiskOffset are ignored.
 *
 *   3. The TrackMode parameter has no effect on the sector
 *      size. The entire info sector (i.e. 2352 bytes of data)
 *      is always returned. IMO the TrackMode is only used
 *      to check the correct sector type.
 */
static NTSTATUS raw_read( struct cdrom *cdrom, const RAW_READ_INFO *info,
        unsigned int size, void *buffer, unsigned int *ret_size )
{
#ifdef __APPLE__
    dk_cd_read_t cdrd;
#endif

    TRACE( "DiskOffset=%s SectorCount=%u TrackMode=%#x\n",
            wine_dbgstr_longlong(info->DiskOffset.QuadPart), info->SectorCount, info->TrackMode );

    if (size < info->SectorCount * 2352)
        return STATUS_BUFFER_TOO_SMALL;

#if defined(linux)
    if (info->DiskOffset.u.HighPart & ~2047)
    {
        WARN( "DiskOffset points to a sector >= 2**32\n" );
        return STATUS_NOT_SUPPORTED;
    }

    switch (info->TrackMode)
    {
        case YellowMode2:
        case XAForm2:
        {
            unsigned int lba = info->DiskOffset.QuadPart >> 11;
            struct cdrom_msf *msf;
            BYTE **bp = buffer;

            if ((lba + info->SectorCount) >
                ((1 << 8 * sizeof(msf->cdmsf_min0)) * CD_SECS * CD_FRAMES - CD_MSF_OFFSET))
            {
                FIXME( "DiskOffset not accessible with MSF\n" );
                return STATUS_NOT_SUPPORTED;
            }

            /* Linux reads only one sector at a time.
             * ioctl CDROMREADRAW takes struct cdrom_msf as an argument
             * on the contrary to what header comments state.
             */
            lba += CD_MSF_OFFSET;
            for (unsigned int i = 0; i < info->SectorCount; i++, lba++, bp += 2352)
            {
                msf = (struct cdrom_msf *)bp;
                msf->cdmsf_min0 = lba / CD_FRAMES / CD_SECS;
                msf->cdmsf_sec0 = lba / CD_FRAMES % CD_SECS;
                msf->cdmsf_frame0 = lba % CD_FRAMES;
                if (ioctl( cdrom->fd, CDROMREADRAW, msf ))
                {
                    *ret_size = 2352 * i;
                    return errno_to_status( errno );
                }
            }
            break;
        }

        case CDDA:
        {
            struct cdrom_read_audio cdra;

            cdra.addr.lba = info->DiskOffset.QuadPart >> 11;
            TRACE("reading at %u\n", cdra.addr.lba);
            cdra.addr_format = CDROM_LBA;
            cdra.nframes = info->SectorCount;
            cdra.buf = buffer;
            if (ioctl( cdrom->fd, CDROMREADAUDIO, &cdra ))
                return errno_to_status( errno );
            break;
        }

        default:
            FIXME( "unhandled mode %#x\n", info->TrackMode );
            return STATUS_NOT_IMPLEMENTED;
    }
#elif defined(__APPLE__)
    /* Mac OS lets us read multiple parts of the sector at a time.
     * We can read all the sectors in at once, unlike Linux. */
    memset( &cdrd, 0, sizeof(cdrd) );
    cdrd.offset = (info->DiskOffset.QuadPart >> 11) * kCDSectorSizeWhole;
    cdrd.buffer = buffer;
    cdrd.bufferLength = info->SectorCount * kCDSectorSizeWhole;
    switch (info->TrackMode)
    {
        case YellowMode2:
            cdrd.sectorType = kCDSectorTypeMode2;
            cdrd.sectorArea = kCDSectorAreaSync | kCDSectorAreaHeader | kCDSectorAreaUser;
            break;

        case XAForm2:
            cdrd.sectorType = kCDSectorTypeMode2Form2;
            cdrd.sectorArea = kCDSectorAreaSync | kCDSectorAreaHeader | kCDSectorAreaSubHeader | kCDSectorAreaUser;
            break;

        case CDDA:
            cdrd.sectorType = kCDSectorTypeCDDA;
            cdrd.sectorArea = kCDSectorAreaUser;
            break;

        default:
            FIXME( "unhandled mode %#x\n", info->TrackMode );
            return STATUS_NOT_IMPLEMENTED;
    }
    if (ioctl( cdrom->fd, DKIOCCDREAD, &cdrd ))
    {
        *ret_size = cdrd.bufferLength;
        return errno_to_status( errno );
    }
#else
    FIXME( "not implemented for this platform\n" );
    return STATUS_NOT_SUPPORTED;
#endif

    *ret_size = 2352 * info->SectorCount;
    return STATUS_SUCCESS;
}

static NTSTATUS disk_type( struct cdrom *cdrom, CDROM_DISK_DATA *data )
{
    NTSTATUS status;

    if (!cdrom->toc_valid && (status = update_toc_cache( cdrom )))
        return status;

    data->DiskData = 0;
    for (unsigned int i = cdrom->toc.FirstTrack; i <= cdrom->toc.LastTrack; ++i)
    {
        if (cdrom->toc.TrackData[i - cdrom->toc.FirstTrack].Control & 0x04)
            data->DiskData |= CDROM_DISK_DATA_TRACK;
        else
            data->DiskData |= CDROM_DISK_AUDIO_TRACK;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS get_drive_geometry( struct cdrom *cdrom, DISK_GEOMETRY *geometry )
{
    unsigned int frame_size;
    NTSTATUS status;

    if (!cdrom->toc_valid && (status = update_toc_cache( cdrom )))
        return status;

    frame_size = track_to_frame( &cdrom->toc, cdrom->toc.LastTrack + 1 ) - track_to_frame( &cdrom->toc, 1 );
    geometry->Cylinders.QuadPart = frame_size / (64 * 32);
    geometry->MediaType = RemovableMedia;
    geometry->TracksPerCylinder = 64;
    geometry->SectorsPerTrack = 32;
    geometry->BytesPerSector= 2048;
    return STATUS_SUCCESS;
}

static NTSTATUS media_removal( struct cdrom *cdrom, const PREVENT_MEDIA_REMOVAL *prevent_removal )
{
#if defined(linux)
    if (ioctl( cdrom->fd, CDROM_LOCKDOOR, prevent_removal->PreventMediaRemoval ))
        return errno_to_status( errno );
    return STATUS_SUCCESS;
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__DragonFly__)
    if (ioctl( cdrom->fd, (prevent_removal->PreventMediaRemoval ? CDIOCPREVENT : CDIOCALLOW), NULL ))
        return errno_to_status( errno );
    return STATUS_SUCCESS;
#else
    FIXME( "not implemented for this platform\n" );
    return STATUS_NOT_SUPPORTED;
#endif
}

static NTSTATUS eject_media( struct cdrom *cdrom )
{
#if defined(linux)
    if (ioctl( cdrom->fd, CDROMEJECT ))
        return errno_to_status( errno );
    return STATUS_SUCCESS;
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__DragonFly__)
    if (ioctl( cdrom->fd, CDIOCEJECT, NULL ))
        return errno_to_status( errno );
    return STATUS_SUCCESS;
#elif defined(__APPLE__)
    if (ioctl( cdrom->fd, DKIOCEJECT, NULL ))
        return errno_to_status( errno );
    return STATUS_SUCCESS;
#else
    FIXME( "not implemented for this platform\n" );
    return STATUS_NOT_SUPPORTED;
#endif
}

static NTSTATUS load_media( struct cdrom *cdrom )
{
#if defined(linux)
    if (ioctl( cdrom->fd, CDROMCLOSETRAY ))
        return errno_to_status( errno );
    return STATUS_SUCCESS;
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__DragonFly__)
    if (ioctl( cdrom->fd, CDIOCCLOSE, NULL ))
        return errno_to_status( errno );
    return STATUS_SUCCESS;
#else
    FIXME( "not implemented for this platform\n" );
    return STATUS_NOT_SUPPORTED;
#endif
}

static NTSTATUS reset_device( struct cdrom *cdrom )
{
#if defined(linux)
    if (ioctl( cdrom->fd, CDROMRESET ))
        return errno_to_status( errno );
    return STATUS_SUCCESS;
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__DragonFly__)
    if (ioctl( cdrom->fd, CDIOCRESET, NULL ))
        return errno_to_status( errno );
    return STATUS_SUCCESS;
#else
    FIXME( "not implemented for this platform\n" );
    return STATUS_NOT_SUPPORTED;
#endif
}

static NTSTATUS check_verify( struct cdrom *cdrom )
{
#if defined(linux)
    int ret;

    if ((ret = ioctl( cdrom->fd, CDROM_DRIVE_STATUS, NULL )) == -1)
        return errno_to_status( errno );
    else if (ret == CDS_DISC_OK)
        return STATUS_SUCCESS;
    else
        return STATUS_NO_MEDIA_IN_DEVICE;
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__DragonFly__)
    if (!ioctl( cdrom->fd, CDIOCSTART, NULL ))
        return STATUS_SUCCESS;
    else
        return STATUS_NO_MEDIA_IN_DEVICE;
#elif defined(__APPLE__)
    /* Mac OS X only creates the device file when media is present. */
    return STATUS_SUCCESS;
#else
    FIXME( "not implemented for this platform\n" );
    return STATUS_NOT_SUPPORTED;
#endif
}

static NTSTATUS dvd_start_session( struct cdrom *cdrom, DVD_SESSION_ID *id )
{
#if defined(linux)
    dvd_authinfo auth_info;

    memset( &auth_info, 0, sizeof( auth_info ) );
    auth_info.type = DVD_LU_SEND_AGID;
    if (ioctl( cdrom->fd, DVD_AUTH, &auth_info ))
        return errno_to_status( errno );
    *id = auth_info.lsa.agid;
    return STATUS_SUCCESS;
#elif defined(__APPLE__)
    DVDAuthenticationGrantIDInfo agid_info;
    dk_dvd_report_key_t dvdrk;

    dvdrk.format = kDVDKeyFormatAGID_CSS;
    dvdrk.keyClass = kDVDKeyClassCSS_CPPM_CPRM;
    dvdrk.bufferLength = sizeof(DVDAuthenticationGrantIDInfo);
    dvdrk.buffer = &agid_info;

    if (ioctl( cdrom->fd, DKIOCDVDREPORTKEY, &dvdrk ))
        return errno_to_status( errno );
    *id = agid_info.grantID;
    return STATUS_SUCCESS;
#else
    FIXME( "not implemented for this platform\n" );
    return STATUS_NOT_SUPPORTED;
#endif
}

static NTSTATUS dvd_end_session( struct cdrom *cdrom, const DVD_SESSION_ID *id )
{
#if defined(linux)
    dvd_authinfo auth_info;

    memset( &auth_info, 0, sizeof( auth_info ) );
    auth_info.type = DVD_INVALIDATE_AGID;
    auth_info.lsa.agid = *id;
    if (ioctl( cdrom->fd, DVD_AUTH, &auth_info ))
        return errno_to_status( errno );
    return STATUS_SUCCESS;
#elif defined(__APPLE__)
    dk_dvd_send_key_t dvdsk;

    dvdsk.format = kDVDKeyFormatAGID_Invalidate;
    dvdsk.keyClass = kDVDKeyClassCSS_CPPM_CPRM;
    dvdsk.grantID = *id;
    if (ioctl( cdrom->fd, DKIOCDVDSENDKEY, &dvdsk ))
        return errno_to_status( errno );
    return STATUS_SUCCESS;
#else
    FIXME( "not implemented for this platform\n" );
    return STATUS_NOT_SUPPORTED;
#endif
}

static NTSTATUS dvd_read_key( struct cdrom *cdrom, const DVD_COPY_PROTECT_KEY *input, DVD_COPY_PROTECT_KEY *output )
{
#if defined(linux)
    dvd_struct dvd;
    dvd_authinfo auth_info;

    memcpy( output, input, input->KeyLength );

    memset( &dvd, 0, sizeof( dvd_struct ) );
    memset( &auth_info, 0, sizeof( auth_info ) );
    switch (input->KeyType)
    {
    case DvdDiskKey:
        TRACE( "DvdDiskKey\n" );
        dvd.type = DVD_STRUCT_DISCKEY;
        dvd.disckey.agid = input->SessionId;
        memset( dvd.disckey.value, 0, DVD_DISCKEY_SIZE );
        if (ioctl( cdrom->fd, DVD_READ_STRUCT, &dvd ))
            return errno_to_status( errno );
        memcpy( output->KeyData, dvd.disckey.value, DVD_DISCKEY_SIZE );
        return STATUS_SUCCESS;

    case DvdTitleKey:
        TRACE( "DvdTitleKey session %u offset %s\n",
               input->SessionId, wine_dbgstr_longlong(input->Parameters.TitleOffset.QuadPart) );
        auth_info.type = DVD_LU_SEND_TITLE_KEY;
        auth_info.lstk.agid = input->SessionId;
        auth_info.lstk.lba = input->Parameters.TitleOffset.QuadPart >> 11;
        if (ioctl( cdrom->fd, DVD_AUTH, &auth_info ))
            return errno_to_status( errno );
        memcpy( output->KeyData, auth_info.lstk.title_key, DVD_KEY_SIZE );
        return STATUS_SUCCESS;

    case DvdChallengeKey:
        TRACE( "DvdChallengeKey\n" );
        auth_info.type = DVD_LU_SEND_CHALLENGE;
        auth_info.lsc.agid = input->SessionId;
        if (ioctl( cdrom->fd, DVD_AUTH, &auth_info ))
            return errno_to_status( errno );
        memcpy( output->KeyData, auth_info.lsc.chal, DVD_CHALLENGE_SIZE );
        return STATUS_SUCCESS;

    case DvdAsf:
        TRACE( "DvdAsf\n" );
        auth_info.type = DVD_LU_SEND_ASF;
        auth_info.lsasf.asf = ((DVD_ASF *)input->KeyData)->SuccessFlag;
        if (ioctl( cdrom->fd, DVD_AUTH, &auth_info ))
            return errno_to_status( errno );
        ((DVD_ASF *)output->KeyData)->SuccessFlag = auth_info.lsasf.asf;
        return STATUS_SUCCESS;

    case DvdBusKey1:
        TRACE( "DvdBusKey1\n" );
        auth_info.type = DVD_LU_SEND_KEY1;
        auth_info.lsk.agid = input->SessionId;
        if (ioctl( cdrom->fd, DVD_AUTH, &auth_info ))
            return errno_to_status( errno );
        memcpy( output->KeyData, auth_info.lsk.key, DVD_KEY_SIZE );
        return STATUS_SUCCESS;

    case DvdGetRpcKey:
        TRACE( "DvdGetRpcKey\n" );
        auth_info.type = DVD_LU_SEND_RPC_STATE;
        if (ioctl( cdrom->fd, DVD_AUTH, &auth_info ))
            return errno_to_status( errno );
        ((DVD_RPC_KEY *)output->KeyData)->TypeCode = auth_info.lrpcs.type;
        ((DVD_RPC_KEY *)output->KeyData)->RegionMask = auth_info.lrpcs.region_mask;
        ((DVD_RPC_KEY *)output->KeyData)->RpcScheme = auth_info.lrpcs.rpc_scheme;
        ((DVD_RPC_KEY *)output->KeyData)->UserResetsAvailable = auth_info.lrpcs.ucca;
        ((DVD_RPC_KEY *)output->KeyData)->ManufacturerResetsAvailable = auth_info.lrpcs.vra;
        return STATUS_SUCCESS;

    default:
        FIXME( "unhandled key type %#x\n", input->KeyType );
        return STATUS_NOT_IMPLEMENTED;
    }
#elif defined(__APPLE__)
    dk_dvd_report_key_t key;
    dk_dvd_read_structure_t disk_key;

    memcpy( output, input, input->KeyLength );

    switch (input->KeyType)
    {
    case DvdChallengeKey:
    {
        DVDChallengeKeyInfo info;

        key.format = kDVDKeyFormatChallengeKey;
        key.grantID = input->SessionId;
        key.keyClass = kDVDKeyClassCSS_CPPM_CPRM;
        key.bufferLength = sizeof(info);
        key.buffer = &info;
        OSWriteBigInt16( info.dataLength, 0, input->KeyLength );
        if (ioctl( cdrom->fd, DKIOCDVDREPORTKEY, &key ))
            return errno_to_status( errno );
        output->KeyLength = OSReadBigInt16( info.dataLength, 0 );
        memcpy( output->KeyData, info.challengeKeyValue, output->KeyLength );
        return STATUS_SUCCESS;
    }

    case DvdBusKey1:
    {
        DVDKey1Info info;

        key.format = kDVDKeyFormatKey1;
        key.grantID = input->SessionId;
        key.keyClass = kDVDKeyClassCSS_CPPM_CPRM;
        key.bufferLength = sizeof(info);
        key.buffer = &info;
        OSWriteBigInt16( info.dataLength, 0, input->KeyLength );
        if (ioctl( cdrom->fd, DKIOCDVDREPORTKEY, &key ))
            return errno_to_status( errno );
        output->KeyLength = OSReadBigInt16( info.dataLength, 0 );
        memcpy( output->KeyData, info.key1Value, output->KeyLength );
        return STATUS_SUCCESS;
    }

    case DvdTitleKey:
    {
        DVDTitleKeyInfo info;

        key.format = kDVDKeyFormatTitleKey;
        key.grantID = input->SessionId;
        key.keyClass = kDVDKeyClassCSS_CPPM_CPRM;
        key.bufferLength = sizeof(info);
        key.buffer = &info;
        key.address = input->Parameters.TitleOffset.QuadPart >> 11;
        OSWriteBigInt16( info.dataLength, 0, input->KeyLength );
        if (ioctl( cdrom->fd, DKIOCDVDREPORTKEY, &key ))
            return errno_to_status( errno );
        output->KeyLength = OSReadBigInt16( info.dataLength, 0 );
        memcpy( output->KeyData, info.titleKeyValue, output->KeyLength );
        output->KeyFlags = 0;
        if (info.CPM)
        {
            /* output->KeyFlags |= DVD_COPYRIGHTED; */
            if (info.CP_SEC)
                output->KeyFlags |= DVD_SECTOR_PROTECTED;
#if 0
            switch (info.CGMS)
            {
            case 0:
                output->KeyFlags |= DVD_CGMS_COPY_PERMITTED;
                break;
            case 2:
                output->KeyFlags |= DVD_CGMS_COPY_ONCE;
                break;
            case 3:
                output->KeyFlags |= DVD_CGMS_NO_COPY;
                break;
            }
#endif
        }
        return STATUS_SUCCESS;
    }

    case DvdAsf:
    {
        DVDAuthenticationSuccessFlagInfo info;

        key.format = kDVDKeyFormatASF;
        key.grantID = input->SessionId;
        key.keyClass = kDVDKeyClassCSS_CPPM_CPRM;
        key.bufferLength = sizeof(info);
        key.buffer = &info;
        OSWriteBigInt16( info.dataLength, 0, input->KeyLength );
        if (ioctl( cdrom->fd, DKIOCDVDREPORTKEY, &key ))
            return errno_to_status( errno );
        output->KeyLength = OSReadBigInt16( info.dataLength, 0 );
        ((DVD_ASF *)output->KeyData)->SuccessFlag = info.successFlag;
        return STATUS_SUCCESS;
    }

    case DvdGetRpcKey:
    {
        DVDRegionPlaybackControlInfo info;

        key.format = kDVDKeyFormatRegionState;
        key.grantID = input->SessionId;
        key.keyClass = kDVDKeyClassCSS_CPPM_CPRM;
        key.bufferLength = sizeof(info);
        key.buffer = &info;
        OSWriteBigInt16( info.dataLength, 0, input->KeyLength );
        if (ioctl( cdrom->fd, DKIOCDVDREPORTKEY, &key ))
            return errno_to_status( errno );
        output->KeyLength = OSReadBigInt16( info.dataLength, 0 );
        ((DVD_RPC_KEY *)output->KeyData)->UserResetsAvailable = info.numberUserResets;
        ((DVD_RPC_KEY *)output->KeyData)->ManufacturerResetsAvailable = info.numberVendorResets;
        ((DVD_RPC_KEY *)output->KeyData)->TypeCode = info.typeCode;
        ((DVD_RPC_KEY *)output->KeyData)->RegionMask = info.driveRegion;
        ((DVD_RPC_KEY *)output->KeyData)->RpcScheme = info.rpcScheme;
        return STATUS_SUCCESS;
    }

    case DvdDiskKey:
    {
        DVDDiscKeyInfo info;

        disk_key.format = kDVDStructureFormatDiscKeyInfo;
        disk_key.grantID = input->SessionId;
        disk_key.bufferLength = sizeof(info);
        disk_key.buffer = &info;
        if (ioctl( cdrom->fd, DKIOCDVDREADSTRUCTURE, &disk_key ))
            return errno_to_status( errno );
        output->KeyLength = OSReadBigInt16( info.dataLength, 0 );
        memcpy( output->KeyData, info.discKeyStructures, output->KeyLength );
        return STATUS_SUCCESS;
    }

    case DvdInvalidateAGID:
        key.format = kDVDKeyFormatAGID_Invalidate;
        key.grantID = input->SessionId;
        key.keyClass = kDVDKeyClassCSS_CPPM_CPRM;
        if (ioctl( cdrom->fd, DKIOCDVDREPORTKEY, &key ))
            return errno_to_status( errno );
        return STATUS_SUCCESS;

    case DvdBusKey2:
    case DvdSetRpcKey:
        ERR( "attempted to read write-only key type %#x\n", input->KeyType );
        return STATUS_NOT_SUPPORTED;
    default:
        FIXME( "unhandled key type %#x\n", input->KeyType );
        return STATUS_NOT_IMPLEMENTED;
    }
#else
    FIXME( "not implemented for this platform\n" );
    return STATUS_NOT_SUPPORTED;
#endif
}

static NTSTATUS dvd_send_key( struct cdrom *cdrom, const DVD_COPY_PROTECT_KEY *key )
{
#if defined(linux)
    dvd_authinfo auth_info;

    memset( &auth_info, 0, sizeof( auth_info ) );
    switch (key->KeyType)
    {
    case DvdChallengeKey:
        TRACE( "DvdChallengeKey\n" );
        auth_info.type = DVD_HOST_SEND_CHALLENGE;
        auth_info.hsc.agid = key->SessionId;
        memcpy( auth_info.hsc.chal, key->KeyData, DVD_CHALLENGE_SIZE );
        if (ioctl( cdrom->fd, DVD_AUTH, &auth_info ))
            return errno_to_status( errno );
        return STATUS_SUCCESS;
    case DvdBusKey2:
        TRACE( "DvdBusKey2\n" );
        auth_info.type = DVD_HOST_SEND_KEY2;
        auth_info.hsk.agid = key->SessionId;
        memcpy( auth_info.hsk.key, key->KeyData, DVD_KEY_SIZE );
        if (ioctl( cdrom->fd, DVD_AUTH, &auth_info ))
            return errno_to_status( errno );
        return STATUS_SUCCESS;

    default:
        FIXME( "unhandled key type %#x\n", key->KeyType );
        return STATUS_NOT_IMPLEMENTED;
    }

#elif defined(__APPLE__)
    dk_dvd_send_key_t dvdsk;
    DVDChallengeKeyInfo chal;
    DVDKey2Info key2;

    dvdsk.keyClass = kDVDKeyClassCSS_CPPM_CPRM;
    dvdsk.grantID = key->SessionId;

    switch (key->KeyType)
    {
    case DvdChallengeKey:
        dvdsk.format = kDVDKeyFormatChallengeKey;
        dvdsk.bufferLength = sizeof(chal);
        dvdsk.buffer = &chal;
        OSWriteBigInt16( chal.dataLength, 0, key->KeyLength );
        memcpy( chal.challengeKeyValue, key->KeyData, key->KeyLength );
        break;

    case DvdBusKey2:
        dvdsk.format = kDVDKeyFormatKey2;
        dvdsk.bufferLength = sizeof(key2);
        dvdsk.buffer = &key2;
        OSWriteBigInt16( key2.dataLength, 0, key->KeyLength );
        memcpy( key2.key2Value, key->KeyData, key->KeyLength );
        break;

    case DvdInvalidateAGID:
        dvdsk.format = kDVDKeyFormatAGID_Invalidate;
        break;

    default:
        FIXME( "unhandled key type %#x\n", key->KeyType );
        return STATUS_NOT_IMPLEMENTED;
    }

    if (ioctl( cdrom->fd, DKIOCDVDSENDKEY, &dvdsk ))
        return errno_to_status( errno );
    return STATUS_SUCCESS;
#else
    FIXME( "not implemented for this platform\n" );
    return STATUS_NOT_SUPPORTED;
#endif
}

static NTSTATUS dvd_get_region( struct cdrom *cdrom, DVD_REGION *region )
{
#if defined(linux)
    dvd_struct dvd;
    dvd_authinfo auth_info;

    dvd.type = DVD_STRUCT_COPYRIGHT;
    dvd.copyright.layer_num = 0;
    auth_info.type = DVD_LU_SEND_RPC_STATE;

    if (ioctl( cdrom->fd, DVD_AUTH, &auth_info ) || ioctl( cdrom->fd, DVD_READ_STRUCT, &dvd ))
        return errno_to_status( errno );
    region->CopySystem = dvd.copyright.cpst;
    region->RegionData = dvd.copyright.rmi;
    region->SystemRegion = auth_info.lrpcs.region_mask;
    region->ResetCount = auth_info.lrpcs.ucca;
    return STATUS_SUCCESS;
#elif defined(__APPLE__)
    dk_dvd_report_key_t key;
    dk_dvd_read_structure_t dvd;
    DVDRegionPlaybackControlInfo rpc;
    DVDCopyrightInfo copy;

    key.format = kDVDKeyFormatRegionState;
    key.keyClass = kDVDKeyClassCSS_CPPM_CPRM;
    key.bufferLength = sizeof(rpc);
    key.buffer = &rpc;
    dvd.format = kDVDStructureFormatCopyrightInfo;
    dvd.bufferLength = sizeof(copy);
    dvd.buffer = &copy;

    if (ioctl( cdrom->fd, DKIOCDVDREPORTKEY, &key ) || ioctl( cdrom->fd, DKIOCDVDREADSTRUCTURE, &dvd ))
        return errno_to_status( errno );
    region->CopySystem = copy.copyrightProtectionSystemType;
    region->RegionData = copy.regionMask;
    region->SystemRegion = rpc.driveRegion;
    region->ResetCount = rpc.numberUserResets;
    return STATUS_SUCCESS;
#else
    FIXME( "not implemented for this platform\n" );
    return STATUS_NOT_SUPPORTED;
#endif
}

static NTSTATUS dvd_read_structure( struct cdrom *cdrom, const DVD_READ_STRUCTURE *structure,
                                    unsigned int size, void *buffer, unsigned int *ret_size )
{
#ifdef DVD_READ_STRUCT
    if (structure->BlockByteOffset.QuadPart)
        FIXME( "ignoring offset\n" );

    switch (structure->Format)
    {
    case DvdPhysicalDescriptor:
    {
        struct dvd_layer_descriptor *desc = buffer;
        const struct dvd_layer *layer;
        struct dvd_physical physical;

        if (size < sizeof(*desc))
            return STATUS_INVALID_PARAMETER;

        physical.type = DVD_STRUCT_PHYSICAL;
        physical.layer_num = structure->LayerNumber;
        if (ioctl( cdrom->fd, DVD_READ_STRUCT, &physical ) < 0)
            return errno_to_status( errno );
        layer = &physical.layer[physical.layer_num];
        desc->Header.Length = 0x0802;
        desc->Header.Reserved[0] = 0;
        desc->Header.Reserved[1] = 0;
        desc->Descriptor.BookVersion = layer->book_version;
        desc->Descriptor.BookType = layer->book_type;
        desc->Descriptor.MinimumRate = layer->min_rate;
        desc->Descriptor.DiskSize = layer->disc_size;
        desc->Descriptor.LayerType = layer->layer_type;
        desc->Descriptor.TrackPath = layer->track_path;
        desc->Descriptor.NumberOfLayers = layer->nlayers;
        desc->Descriptor.Reserved1 = 0;
        desc->Descriptor.TrackDensity = layer->track_density;
        desc->Descriptor.LinearDensity = layer->linear_density;
        desc->Descriptor.StartingDataSector = layer->start_sector;
        desc->Descriptor.EndDataSector = layer->end_sector;
        desc->Descriptor.EndLayerZeroSector = layer->end_sector_l0;
        desc->Descriptor.Reserved5 = 0;
        desc->Descriptor.BCAFlag = layer->bca;
        *ret_size = sizeof(*desc);
        return STATUS_SUCCESS;
    }

    case DvdCopyrightDescriptor:
    {
        DVD_COPYRIGHT_DESCRIPTOR *desc = buffer;
        struct dvd_copyright copyright;

        if (size < sizeof(*desc))
            return STATUS_INVALID_PARAMETER;

        copyright.type = DVD_STRUCT_COPYRIGHT;
        copyright.layer_num = structure->LayerNumber;
        if (ioctl( cdrom->fd, DVD_READ_STRUCT, &copyright ) < 0)
            return errno_to_status( errno );
        desc->CopyrightProtectionType = copyright.cpst;
        desc->RegionManagementInformation = copyright.rmi;
        desc->Reserved = 0;
        *ret_size = sizeof(*desc);
        return STATUS_SUCCESS;
    }

    case DvdDiskKeyDescriptor:
    {
        DVD_DISK_KEY_DESCRIPTOR *desc = buffer;
        struct dvd_disckey disckey;

        if (size < sizeof(*desc))
            return STATUS_INVALID_PARAMETER;

        disckey.type = DVD_STRUCT_DISCKEY;
        disckey.agid = structure->SessionId;
        if (ioctl( cdrom->fd, DVD_READ_STRUCT, &disckey ) < 0)
            return errno_to_status( errno );
        memcpy( desc->DiskKeyData, disckey.value, 2048 );
        *ret_size = sizeof(*desc);
        return STATUS_SUCCESS;
    }

    case DvdBCADescriptor:
    {
        DVD_BCA_DESCRIPTOR *desc = buffer;
        struct dvd_bca bca;

        if (size < sizeof(*desc))
            return STATUS_INVALID_PARAMETER;

        bca.type = DVD_STRUCT_BCA;
        if (ioctl( cdrom->fd, DVD_READ_STRUCT, &bca ) < 0)
            return errno_to_status( errno );
        memcpy( desc->BCAInformation, bca.value, bca.len );
        *ret_size = sizeof(*desc);
        return STATUS_SUCCESS;
    }

    case DvdManufacturerDescriptor:
    {
        struct dvd_manufacturer_descriptor *desc = buffer;
        struct dvd_manufact manufact;

        if (size < sizeof(*desc))
            return STATUS_INVALID_PARAMETER;

        manufact.type = DVD_STRUCT_MANUFACT;
        manufact.layer_num = structure->LayerNumber;
        if (ioctl( cdrom->fd, DVD_READ_STRUCT, &manufact ) < 0)
            return errno_to_status( errno );
        desc->Header.Length = 0x0802;
        desc->Header.Reserved[0] = 0;
        desc->Header.Reserved[1] = 0;
        memcpy( desc->Descriptor.ManufacturingInformation, manufact.value, 2048 );
        *ret_size = sizeof(*desc);
        return STATUS_SUCCESS;
    }

    default:
        FIXME( "unhandled structure %#x\n", structure->Format );
        return STATUS_NOT_IMPLEMENTED;
    }
#elif defined(__APPLE__)
    dk_dvd_read_structure_t dvdrs;

    memset( &dvdrs, 0, sizeof(dvdrs) );
    dvdrs.address = structure->BlockByteOffset.QuadPart >> 11;
    dvdrs.grantID = structure->SessionId;
    dvdrs.layer = structure->LayerNumber;

    switch (structure->Format)
    {
    case DvdPhysicalDescriptor:
    {
        struct dvd_layer_descriptor *desc = buffer;
        DVDPhysicalFormatInfo phys;

        if (size < sizeof(*desc))
            return STATUS_INVALID_PARAMETER;

        dvdrs.format = kDVDStructureFormatPhysicalFormatInfo;
        dvdrs.bufferLength = sizeof(phys);
        dvdrs.buffer = &phys;
        if (ioctl( cdrom->fd, DKIOCDVDREADSTRUCTURE, &dvdrs ))
            return errno_to_status( errno );
        desc->Header.Length = 0x0802;
        desc->Header.Reserved[0] = 0;
        desc->Header.Reserved[1] = 0;
        desc->Descriptor.BookVersion = phys.partVersion;
        desc->Descriptor.BookType = phys.bookType;
        desc->Descriptor.MinimumRate = phys.minimumRate;
        desc->Descriptor.DiskSize = phys.discSize;
        desc->Descriptor.LayerType = phys.layerType;
        desc->Descriptor.TrackPath = phys.trackPath;
        desc->Descriptor.NumberOfLayers = phys.numberOfLayers;
        desc->Descriptor.Reserved1 = 0;
        desc->Descriptor.TrackDensity = phys.trackDensity;
        desc->Descriptor.LinearDensity = phys.linearDensity;
        desc->Descriptor.StartingDataSector = *(ULONG *)&phys.zero1;
        desc->Descriptor.EndDataSector = *(ULONG *)&phys.zero2;
        desc->Descriptor.EndLayerZeroSector = *(ULONG *)&phys.zero3;
        desc->Descriptor.Reserved5 = 0;
        desc->Descriptor.BCAFlag = phys.bcaFlag;
        *ret_size = sizeof(*desc);
        return STATUS_SUCCESS;
    }

    case DvdCopyrightDescriptor:
    {
        DVD_COPYRIGHT_DESCRIPTOR *desc = buffer;
        DVDCopyrightInfo copy;

        if (size < sizeof(*desc))
            return STATUS_INVALID_PARAMETER;

        dvdrs.format = kDVDStructureFormatCopyrightInfo;
        dvdrs.bufferLength = sizeof(copy);
        dvdrs.buffer = &copy;
        if (ioctl( cdrom->fd, DKIOCDVDREADSTRUCTURE, &dvdrs ))
            return errno_to_status( errno );
        desc->CopyrightProtectionType = copy.copyrightProtectionSystemType;
        desc->RegionManagementInformation = copy.regionMask;
        desc->Reserved = 0;
        *ret_size = sizeof(*desc);
        return STATUS_SUCCESS;
    }

    case DvdDiskKeyDescriptor:
    {
        DVD_DISK_KEY_DESCRIPTOR *desc = buffer;
        DVDDiscKeyInfo disk_key;

        if (size < sizeof(*desc))
            return STATUS_INVALID_PARAMETER;

        dvdrs.format = kDVDStructureFormatDiscKeyInfo;
        dvdrs.bufferLength = sizeof(disk_key);
        dvdrs.buffer = &disk_key;
        if (ioctl( cdrom->fd, DKIOCDVDREADSTRUCTURE, &dvdrs ))
            return errno_to_status( errno );
        memcpy( desc->DiskKeyData, disk_key.discKeyStructures, 2048 );
        *ret_size = sizeof(*desc);
        return STATUS_SUCCESS;
    }

    case DvdManufacturerDescriptor:
    {
        struct dvd_manufacturer_descriptor *desc = buffer;
        DVDManufacturingInfo manf;

        if (size < sizeof(*desc))
            return STATUS_INVALID_PARAMETER;

        dvdrs.format = kDVDStructureFormatManufacturingInfo;
        dvdrs.bufferLength = sizeof(manf);
        dvdrs.buffer = &manf;
        if (ioctl( cdrom->fd, DKIOCDVDREADSTRUCTURE, &dvdrs ))
            return errno_to_status( errno );
        desc->Header.Length = 0x0802;
        desc->Header.Reserved[0] = 0;
        desc->Header.Reserved[1] = 0;
        memcpy( desc->Descriptor.ManufacturingInformation, manf.discManufacturingInfo, 2048 );
        *ret_size = sizeof(*desc);
        return STATUS_SUCCESS;
    }

    default:
        FIXME( "unhandled structure %#x\n", structure->Format );
        return STATUS_NOT_IMPLEMENTED;
    }
#else
    FIXME( "not implemented for this platform\n" );
    return STATUS_NOT_SUPPORTED;
#endif
}

static NTSTATUS scsi_pass_through( struct cdrom *cdrom, const SCSI_PASS_THROUGH *in_pkt, SCSI_PASS_THROUGH *out_pkt )
{
#ifdef HAVE_SG_IO_HDR_T_INTERFACE_ID
    sg_io_hdr_t cmd;
#elif defined __APPLE__
    dk_scsi_command_t cmd;
    int ret;
#endif

    if (in_pkt->Length < sizeof(SCSI_PASS_THROUGH))
        return STATUS_BUFFER_TOO_SMALL;

    if (in_pkt->CdbLength > 16)
        return STATUS_INVALID_PARAMETER;

#ifdef SENSEBUFLEN
    if (in_pkt->SenseInfoLength > SENSEBUFLEN)
        return STATUS_INVALID_PARAMETER;
#endif

    if (in_pkt->DataTransferLength > 0 && in_pkt->DataBufferOffset < sizeof(SCSI_PASS_THROUGH))
        return STATUS_INVALID_PARAMETER;

#ifdef HAVE_SG_IO_HDR_T_INTERFACE_ID
    memset( &cmd, 0, sizeof(cmd) );

    cmd.interface_id = 'S';
    cmd.dxfer_len = in_pkt->DataTransferLength;
    cmd.cmd_len = in_pkt->CdbLength;
    cmd.cmdp = (unsigned char *)in_pkt->Cdb;
    cmd.mx_sb_len = in_pkt->SenseInfoLength;
    cmd.timeout = in_pkt->TimeOutValue * 1000;

    if (cmd.mx_sb_len > 0)
        cmd.sbp = (unsigned char *)out_pkt + in_pkt->SenseInfoOffset;

    switch (in_pkt->DataIn)
    {
    case SCSI_IOCTL_DATA_IN:
        cmd.dxferp = (char *)out_pkt + in_pkt->DataBufferOffset;
        cmd.dxfer_direction = SG_DXFER_FROM_DEV;
        break;
    case SCSI_IOCTL_DATA_OUT:
        cmd.dxferp = (char *)in_pkt + in_pkt->DataBufferOffset;
        cmd.dxfer_direction = SG_DXFER_TO_DEV;
        break;
    case SCSI_IOCTL_DATA_UNSPECIFIED:
        cmd.dxfer_direction = SG_DXFER_NONE;
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    if (ioctl( cdrom->fd, SG_IO, &cmd ))
        return errno_to_status( errno );

    out_pkt->ScsiStatus = cmd.status;
    out_pkt->DataTransferLength = in_pkt->DataTransferLength - cmd.resid;
    out_pkt->SenseInfoLength = cmd.sb_len_wr;
#elif defined(__APPLE__)
    memset( &cmd, 0, sizeof(cmd) );
    memcpy( cmd.cdb, in_pkt->Cdb, in_pkt->CdbLength );

    cmd.cdbSize = in_pkt->CdbLength;
    cmd.bufferSize = in_pkt->DataTransferLength;
    cmd.sense = (char *)out_pkt + in_pkt->SenseInfoOffset;
    cmd.senseLen = in_pkt->SenseInfoLength;
    cmd.timeout = in_pkt->TimeOutValue * 1000; /* in milliseconds */

    switch (in_pkt->DataIn)
    {
    case SCSI_IOCTL_DATA_OUT:
        cmd.buffer = (char *)in_pkt + in_pkt->DataBufferOffset;
        cmd.direction = kSCSIDataTransfer_FromInitiatorToTarget;
        break;
    case SCSI_IOCTL_DATA_IN:
        cmd.buffer = (char *)out_pkt + in_pkt->DataBufferOffset;
        cmd.direction = kSCSIDataTransfer_FromTargetToInitiator;
        break;
    case SCSI_IOCTL_DATA_UNSPECIFIED:
        cmd.direction = kSCSIDataTransfer_NoDataTransfer;
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    ret = ioctl( cdrom->fd, DKIOCSCSICOMMAND, &cmd );

    if (cmd.response == kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE)
    {
        /* Command failed */
        switch (cmd.status)
        {
        case kSCSITaskStatus_TaskTimeoutOccurred:
            return STATUS_TIMEOUT;
        case kSCSITaskStatus_ProtocolTimeoutOccurred:
            return STATUS_IO_TIMEOUT;
        case kSCSITaskStatus_DeviceNotResponding:
            return STATUS_DEVICE_BUSY;
        case kSCSITaskStatus_DeviceNotPresent:
            return STATUS_NO_SUCH_DEVICE;
        case kSCSITaskStatus_DeliveryFailure:
            return STATUS_DEVICE_PROTOCOL_ERROR;
        case kSCSITaskStatus_No_Status:
        default:
            return STATUS_UNSUCCESSFUL;
        }
    }

    if (cmd.status != kSCSITaskStatus_No_Status)
        out_pkt->ScsiStatus = cmd.status;

    if (ret)
        return errno_to_status( errno );

    /* FIXME: Update DataTransferLength and SenseInfoLength */
#else
    FIXME( "not implemented for this platform\n" );
    return STATUS_NOT_SUPPORTED;
#endif

    out_pkt->Length = sizeof(*out_pkt);
    out_pkt->CdbLength = in_pkt->CdbLength;
    out_pkt->DataIn = in_pkt->DataIn;
    out_pkt->TimeOutValue = in_pkt->TimeOutValue;
    out_pkt->DataBufferOffset = in_pkt->DataBufferOffset;
    out_pkt->SenseInfoOffset = in_pkt->SenseInfoOffset;
    memmove( out_pkt->Cdb, in_pkt->Cdb, sizeof(in_pkt->Cdb) );
    return STATUS_SUCCESS;
}

static NTSTATUS scsi_pass_through_32( struct cdrom *cdrom, const SCSI_PASS_THROUGH32 *in_pkt32,
                                      SCSI_PASS_THROUGH32 *out_pkt32 )
{
    SCSI_PASS_THROUGH *pkt;
    NTSTATUS status;
    ULONG_PTR ptr;

    if (in_pkt32->Length < sizeof(SCSI_PASS_THROUGH32))
        return STATUS_BUFFER_TOO_SMALL;

    if (in_pkt32->CdbLength > sizeof(in_pkt32->Cdb))
        return STATUS_INVALID_PARAMETER;

    if (in_pkt32->DataTransferLength > 0)
    {
        if (in_pkt32->DataBufferOffset < sizeof(SCSI_PASS_THROUGH32))
            return STATUS_INVALID_PARAMETER;
        ptr = (ULONG_PTR)in_pkt32 + in_pkt32->DataBufferOffset;
        if (ptr < (ULONG_PTR)in_pkt32)
            return STATUS_INVALID_PARAMETER;
        if ((ptr + in_pkt32->DataTransferLength) < ptr)
            return STATUS_INVALID_PARAMETER;
    }

    if (in_pkt32->SenseInfoLength > 0)
    {
        if (in_pkt32->SenseInfoOffset < sizeof(SCSI_PASS_THROUGH32))
            return STATUS_INVALID_PARAMETER;
        ptr = (ULONG_PTR)in_pkt32 + in_pkt32->SenseInfoOffset;
        if (ptr < (ULONG_PTR)in_pkt32)
            return STATUS_INVALID_PARAMETER;
        if ((ptr + in_pkt32->SenseInfoLength) < ptr)
            return STATUS_INVALID_PARAMETER;
    }

    if (!(pkt = calloc( 1, sizeof(SCSI_PASS_THROUGH) + in_pkt32->SenseInfoLength + in_pkt32->DataTransferLength )))
        return STATUS_NO_MEMORY;

    pkt->Length = sizeof(SCSI_PASS_THROUGH);
    pkt->CdbLength = in_pkt32->CdbLength;
    pkt->SenseInfoLength = in_pkt32->SenseInfoLength;
    pkt->DataIn = in_pkt32->DataIn;
    pkt->DataTransferLength = in_pkt32->DataTransferLength;
    pkt->TimeOutValue = in_pkt32->TimeOutValue;
    pkt->DataBufferOffset = sizeof(SCSI_PASS_THROUGH) + in_pkt32->SenseInfoLength;
    pkt->SenseInfoOffset = sizeof(SCSI_PASS_THROUGH);
    memcpy( pkt->Cdb, in_pkt32->Cdb, sizeof(pkt->Cdb) );
    if (pkt->DataIn == SCSI_IOCTL_DATA_OUT)
        memcpy( (char *)pkt + pkt->DataBufferOffset,
                (const char *)in_pkt32 + in_pkt32->DataBufferOffset,
                in_pkt32->DataTransferLength );

    if ((status = scsi_pass_through( cdrom, pkt, pkt )))
        goto done;

    out_pkt32->Length = sizeof(SCSI_PASS_THROUGH32);
    out_pkt32->ScsiStatus = pkt->ScsiStatus;
    out_pkt32->PathId = pkt->PathId;
    out_pkt32->TargetId = pkt->TargetId;
    out_pkt32->Lun = pkt->Lun;
    out_pkt32->CdbLength = pkt->CdbLength;
    out_pkt32->SenseInfoLength = pkt->SenseInfoLength;
    out_pkt32->DataIn = pkt->DataIn;
    out_pkt32->DataTransferLength = pkt->DataTransferLength;
    out_pkt32->TimeOutValue = pkt->TimeOutValue;
    out_pkt32->DataBufferOffset = in_pkt32->DataBufferOffset;
    out_pkt32->SenseInfoOffset = in_pkt32->SenseInfoOffset;
    memcpy(out_pkt32->Cdb, pkt->Cdb, sizeof(out_pkt32->Cdb));
    memcpy( (char *)out_pkt32 + out_pkt32->SenseInfoOffset,
            (const char *)pkt + pkt->SenseInfoOffset, pkt->SenseInfoLength );
    if (pkt->DataIn == SCSI_IOCTL_DATA_IN)
        memcpy( (char *)out_pkt32 + out_pkt32->DataBufferOffset,
                (const char *)pkt + pkt->DataBufferOffset, pkt->DataTransferLength );

done:
    free( pkt );
    return status;
}

static NTSTATUS scsi_pass_through_direct( struct cdrom *cdrom, const SCSI_PASS_THROUGH_DIRECT *in_pkt,
                                          SCSI_PASS_THROUGH_DIRECT *out_pkt )
{
#ifdef HAVE_SG_IO_HDR_T_INTERFACE_ID
    sg_io_hdr_t cmd;
#elif defined __APPLE__
    dk_scsi_command_t cmd;
    int ret;
#endif

    if (in_pkt->Length < sizeof(SCSI_PASS_THROUGH_DIRECT))
        return STATUS_BUFFER_TOO_SMALL;

    if (in_pkt->CdbLength > 16)
        return STATUS_INVALID_PARAMETER;

#ifdef SENSEBUFLEN
    if (in_pkt->SenseInfoLength > SENSEBUFLEN)
        return STATUS_INVALID_PARAMETER;
#endif

    if (in_pkt->DataTransferLength > 0 && !in_pkt->DataBuffer)
        return STATUS_INVALID_PARAMETER;

#ifdef HAVE_SG_IO_HDR_T_INTERFACE_ID
    memset( &cmd, 0, sizeof(cmd) );

    cmd.interface_id = 'S';
    cmd.cmd_len      = in_pkt->CdbLength;
    cmd.mx_sb_len    = in_pkt->SenseInfoLength;
    cmd.dxfer_len    = in_pkt->DataTransferLength;
    cmd.dxferp       = in_pkt->DataBuffer;
    cmd.cmdp         = (unsigned char *)in_pkt->Cdb;
    cmd.sbp          = (unsigned char *)out_pkt + in_pkt->SenseInfoOffset;
    cmd.timeout      = in_pkt->TimeOutValue * 1000;

    switch (in_pkt->DataIn)
    {
    case SCSI_IOCTL_DATA_IN:
        cmd.dxfer_direction = SG_DXFER_FROM_DEV;
        break;
    case SCSI_IOCTL_DATA_OUT:
        cmd.dxfer_direction = SG_DXFER_TO_DEV;
        break;
    case SCSI_IOCTL_DATA_UNSPECIFIED:
        cmd.dxfer_direction = SG_DXFER_NONE;
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    if (ioctl( cdrom->fd, SG_IO, &cmd ))
        return errno_to_status( errno );

    out_pkt->ScsiStatus         = cmd.status;
    out_pkt->DataTransferLength = in_pkt->DataTransferLength - cmd.resid;
    out_pkt->SenseInfoLength    = cmd.sb_len_wr;
#elif defined(__APPLE__)
    memset( &cmd, 0, sizeof(cmd) );
    memcpy( cmd.cdb, in_pkt->Cdb, in_pkt->CdbLength );

    cmd.cdbSize = in_pkt->CdbLength;
    cmd.buffer = in_pkt->DataBuffer;
    cmd.bufferSize = in_pkt->DataTransferLength;
    cmd.sense = (char *)out_pkt + in_pkt->SenseInfoOffset;
    cmd.senseLen = in_pkt->SenseInfoLength;
    cmd.timeout = in_pkt->TimeOutValue * 1000; /* in milliseconds */

    switch (in_pkt->DataIn)
    {
    case SCSI_IOCTL_DATA_OUT:
        cmd.direction = kSCSIDataTransfer_FromInitiatorToTarget;
        break;
    case SCSI_IOCTL_DATA_IN:
        cmd.direction = kSCSIDataTransfer_FromTargetToInitiator;
        break;
    case SCSI_IOCTL_DATA_UNSPECIFIED:
        cmd.direction = kSCSIDataTransfer_NoDataTransfer;
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    ret = ioctl( cdrom->fd, DKIOCSCSICOMMAND, &cmd );

    if (cmd.response == kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE)
    {
        switch (cmd.status)
        {
        case kSCSITaskStatus_TaskTimeoutOccurred:
            return STATUS_TIMEOUT;
        case kSCSITaskStatus_ProtocolTimeoutOccurred:
            return STATUS_IO_TIMEOUT;
        case kSCSITaskStatus_DeviceNotResponding:
            return STATUS_DEVICE_BUSY;
        case kSCSITaskStatus_DeviceNotPresent:
            return STATUS_NO_SUCH_DEVICE;
        case kSCSITaskStatus_DeliveryFailure:
            return STATUS_DEVICE_PROTOCOL_ERROR;
        case kSCSITaskStatus_No_Status:
        default:
            return STATUS_UNSUCCESSFUL;
        }
    }

    if (ret)
        return errno_to_status( errno );

    if (cmd.status != kSCSITaskStatus_No_Status)
        out_pkt->ScsiStatus = cmd.status;

    /* FIXME: Update DataTransferLength and SenseInfoLength */
#else
    FIXME( "not implemented for this platform\n" );
    return STATUS_NOT_SUPPORTED;
#endif
    out_pkt->Length = sizeof(*out_pkt);
    out_pkt->CdbLength = in_pkt->CdbLength;
    out_pkt->DataIn = in_pkt->DataIn;
    out_pkt->TimeOutValue = in_pkt->TimeOutValue;
    out_pkt->DataBuffer = in_pkt->DataBuffer;
    out_pkt->SenseInfoOffset = in_pkt->SenseInfoOffset;
    memmove( out_pkt->Cdb, in_pkt->Cdb, sizeof(in_pkt->Cdb) );

    return STATUS_SUCCESS;
}

static NTSTATUS scsi_pass_through_direct_32( struct cdrom *cdrom, const SCSI_PASS_THROUGH_DIRECT32 *in_pkt32,
                                             SCSI_PASS_THROUGH_DIRECT32 *out_pkt32 )
{
    SCSI_PASS_THROUGH_DIRECT *pkt;
    NTSTATUS status;
    ULONG_PTR ptr;

    if (in_pkt32->Length < sizeof(SCSI_PASS_THROUGH_DIRECT32))
        return STATUS_BUFFER_TOO_SMALL;

    if (in_pkt32->CdbLength > 16)
        return STATUS_INVALID_PARAMETER;

#ifdef SENSEBUFLEN
    if (in_pkt32->SenseInfoLength > SENSEBUFLEN)
        return STATUS_INVALID_PARAMETER;
#endif

    if (in_pkt32->SenseInfoLength > 0)
    {
        if (in_pkt32->SenseInfoOffset < sizeof(SCSI_PASS_THROUGH_DIRECT32))
            return STATUS_INVALID_PARAMETER;
        ptr = (ULONG_PTR)in_pkt32 + in_pkt32->SenseInfoOffset;
        if (ptr < (ULONG_PTR)in_pkt32)
            return STATUS_INVALID_PARAMETER;
        if ((ptr + in_pkt32->SenseInfoLength) < ptr)
            return STATUS_INVALID_PARAMETER;
    }

    if (!(pkt = calloc( 1, sizeof(SCSI_PASS_THROUGH_DIRECT) + in_pkt32->SenseInfoLength )))
        return STATUS_NO_MEMORY;

    pkt->Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
    pkt->CdbLength = in_pkt32->CdbLength;
    pkt->SenseInfoLength = in_pkt32->SenseInfoLength;
    pkt->DataIn = in_pkt32->DataIn;
    pkt->DataTransferLength = in_pkt32->DataTransferLength;
    pkt->TimeOutValue = in_pkt32->TimeOutValue;
    pkt->DataBuffer = ULongToPtr(in_pkt32->DataBuffer);
    pkt->SenseInfoOffset = sizeof(SCSI_PASS_THROUGH_DIRECT);
    memcpy( pkt->Cdb, in_pkt32->Cdb, sizeof(pkt->Cdb) );

    if ((status = scsi_pass_through_direct( cdrom, pkt, pkt )))
        goto done;

    out_pkt32->Length = sizeof(SCSI_PASS_THROUGH_DIRECT32);
    out_pkt32->ScsiStatus = pkt->ScsiStatus;
    out_pkt32->PathId = pkt->PathId;
    out_pkt32->TargetId = pkt->TargetId;
    out_pkt32->Lun = pkt->Lun;
    out_pkt32->CdbLength = pkt->CdbLength;
    out_pkt32->SenseInfoLength = pkt->SenseInfoLength;
    out_pkt32->DataIn = pkt->DataIn;
    out_pkt32->DataTransferLength = pkt->DataTransferLength;
    out_pkt32->TimeOutValue = pkt->TimeOutValue;
    out_pkt32->DataBuffer = in_pkt32->DataBuffer;
    out_pkt32->SenseInfoOffset = in_pkt32->SenseInfoOffset;
    memcpy( out_pkt32->Cdb, pkt->Cdb, sizeof(out_pkt32->Cdb) );
    memcpy( (char *)out_pkt32 + out_pkt32->SenseInfoOffset,
            (const char *)pkt + pkt->SenseInfoOffset, pkt->SenseInfoLength );

done:
    free( pkt );
    return status;
}

static NTSTATUS scsi_get_address( struct cdrom *cdrom, SCSI_ADDRESS *address )
{
#if defined(linux)
    struct stat st;

    if (fstat( cdrom->fd, &st ) == -1)
        return errno_to_status( errno );
    if (!S_ISBLK(st.st_mode))
        return STATUS_INVALID_DEVICE_REQUEST;
    memset( address, 0, sizeof(SCSI_ADDRESS) );
    address->Length = sizeof(SCSI_ADDRESS);
    switch (major( st.st_rdev ))
    {
    case IDE0_MAJOR: address->PortNumber = 0; break;
    case IDE1_MAJOR: address->PortNumber = 1; break;
    case IDE2_MAJOR: address->PortNumber = 2; break;
    case IDE3_MAJOR: address->PortNumber = 3; break;
    case IDE4_MAJOR: address->PortNumber = 4; break;
    case IDE5_MAJOR: address->PortNumber = 5; break;
    case IDE6_MAJOR: address->PortNumber = 6; break;
    case IDE7_MAJOR: address->PortNumber = 7; break;
    default: address->PathId = 1; break;
    }

    if (address->PathId == 0)
        address->TargetId = (minor( st.st_rdev ) >> 6);
    else
    {
#ifdef SCSI_IOCTL_GET_IDLUN
        __u32 idlun[2];
        if (ioctl( cdrom->fd, SCSI_IOCTL_GET_IDLUN, idlun ) != -1)
        {
            address->PathId = (idlun[0] >> 24) & 0xff;
            address->PortNumber = ((idlun[0] >> 16) & 0xff) + 2;
            address->TargetId = idlun[0] & 0xff;
            address->Lun = (idlun[0] >> 8) & 0xff;
        }
        else
#endif
        {
            WARN( "CD-ROM device (%u, %u) not supported\n", major( st.st_rdev ), minor( st.st_rdev ));
            return STATUS_NOT_SUPPORTED;
        }
    }
    return STATUS_SUCCESS;
#elif defined(__NetBSD__)
    struct scsi_addr addr;

    if (ioctl( cdrom->fd, SCIOCIDENTIFY, &addr ) == -1)
        return errno_to_status( errno );
    switch (addr.type)
    {
    case TYPE_SCSI:
        address->PathId = 1;
        address->PortNumber = addr.addr.scsi.scbus;
        address->TargetId = addr.addr.scsi.target;
        address->Lun = addr.addr.scsi.lun;
        return STATUS_SUCCESS;
    case TYPE_ATAPI:
        address->PathId = 0;
        address->PortNumber = addr.addr.atapi.atbus;
        address->TargetId = addr.addr.atapi.drive;
        address->Lun = 0;
        return STATUS_SUCCESS;
    default:
        FIXME( "unhandled type %#x\n", addr.type );
        return STATUS_NOT_IMPLEMENTED;
    }
#elif defined(__APPLE__)
    dk_scsi_identify_t addr;

    if (ioctl( cdrom->fd, DKIOCSCSIIDENTIFY, &addr ) == -1)
        return errno_to_status( errno );
    address->PathId = addr.bus;
    address->PortNumber = addr.port;
    address->TargetId = addr.target;
    address->Lun = addr.lun;
    return STATUS_SUCCESS;
#else
    FIXME( "not implemented for this platform\n" );
    return STATUS_NOT_IMPLEMENTED;
#endif
}

static NTSTATUS scsi_get_capabilities( struct cdrom *cdrom, IO_SCSI_CAPABILITIES *caps )
{
#ifdef SG_SCATTER_SZ
    caps->Length = sizeof(*caps);
    caps->MaximumTransferLength = SG_SCATTER_SZ; /* FIXME */
    caps->MaximumPhysicalPages = SG_SCATTER_SZ / 0x1000;
    caps->SupportedAsynchronousEvents = TRUE;
    caps->AlignmentMask = 0x1000;
    caps->TaggedQueuing = FALSE; /* we could check that it works and answer TRUE */
    caps->AdapterScansDown = FALSE; /* FIXME ? */
    caps->AdapterUsesPio = FALSE; /* FIXME ? */
    return STATUS_SUCCESS;
#elif defined __APPLE__
    uint64_t bytesr, bytesw, align;

    if (ioctl( cdrom->fd, DKIOCGETMAXBYTECOUNTREAD, &bytesr ) ||
        ioctl( cdrom->fd, DKIOCGETMAXBYTECOUNTWRITE, &bytesw ) ||
        ioctl( cdrom->fd, DKIOCGETMINSEGMENTALIGNMENTBYTECOUNT, &align ))
        return errno_to_status( errno );
    caps->Length = sizeof(*caps);
    caps->MaximumTransferLength = min( bytesr, bytesw );
    caps->MaximumPhysicalPages = caps->MaximumTransferLength / 0x1000;
    caps->SupportedAsynchronousEvents = TRUE;
    caps->AlignmentMask = align - 1;
    caps->TaggedQueuing = FALSE; /* we could check that it works and answer TRUE */
    caps->AdapterScansDown = FALSE; /* FIXME ? */
    caps->AdapterUsesPio = FALSE; /* FIXME ? */
    return STATUS_SUCCESS;
#else
    FIXME( "not implemented for this platform\n" );
    return STATUS_NOT_IMPLEMENTED;
#endif
}

#define INQ_CMD_LEN 6
#define INQ_REPLY_LEN 36

static NTSTATUS scsi_get_inquiry_data( struct cdrom *cdrom, unsigned int size,
                                       SCSI_ADAPTER_BUS_INFO *info, unsigned int *ret_size )
{
#ifdef HAVE_SG_IO_HDR_T_INTERFACE_ID
    UCHAR inquiry[INQ_CMD_LEN] = {INQUIRY, 0, 0, 0, INQ_REPLY_LEN, 0};
    SCSI_INQUIRY_DATA *inq_data;
    unsigned int needed_size;
    UCHAR sense_buffer[32];
    SCSI_ADDRESS address;
    sg_io_hdr_t cmd;
    NTSTATUS status;
    int version;

    needed_size = offsetof( SCSI_ADAPTER_BUS_INFO, BusData[1] );
    needed_size += offsetof( SCSI_INQUIRY_DATA, InquiryData[INQ_REPLY_LEN] );
    needed_size = (needed_size + 3) & ~3;
    if (size < needed_size)
        return STATUS_INVALID_PARAMETER;

    /* Check we have a SCSI device and a supported driver */
    if (ioctl( cdrom->fd, SG_GET_VERSION_NUM, &version ))
        return errno_to_status( errno );
    if (version < 30000)
        return STATUS_NOT_SUPPORTED;

    /* FIXME: Enumerate devices on the bus */
    info->NumberOfBuses = 1;
    info->BusData[0].NumberOfLogicalUnits = 1;
    info->BusData[0].InquiryDataOffset = sizeof(SCSI_ADAPTER_BUS_INFO);

    inq_data = (SCSI_INQUIRY_DATA *)&info->BusData[1];

    memset( &cmd, 0, sizeof(cmd) );
    cmd.interface_id = 'S';
    cmd.cmd_len = sizeof(inquiry);
    cmd.mx_sb_len = sizeof(sense_buffer);
    cmd.dxfer_direction = SG_DXFER_FROM_DEV;
    cmd.dxfer_len = INQ_REPLY_LEN;
    cmd.dxferp = inq_data->InquiryData;
    cmd.cmdp = inquiry;
    cmd.sbp = sense_buffer;
    cmd.timeout = 1000;

    if (ioctl( cdrom->fd, SG_IO, &cmd ))
        WARN( "failed to send SCSI command: %s\n", strerror( errno ));

    if ((status = scsi_get_address( cdrom, &address )))
        return status;
    info->BusData[0].InitiatorBusId = address.PortNumber;
    inq_data->PathId = address.PathId;
    inq_data->TargetId = address.TargetId;
    inq_data->Lun = address.Lun;
    inq_data->DeviceClaimed = TRUE;
    inq_data->InquiryDataLength = INQ_REPLY_LEN;
    inq_data->NextInquiryDataOffset = 0;
    *ret_size = needed_size;
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
        case IOCTL_CDROM_DISK_TYPE:
            if (params->output_size < sizeof(CDROM_DISK_DATA))
                return STATUS_BUFFER_TOO_SMALL;
            params->ret_size = sizeof(CDROM_DISK_DATA);
            return disk_type( params->cdrom, params->output );

        case IOCTL_STORAGE_EJECT_MEDIA:
            return eject_media( params->cdrom );

        case IOCTL_CDROM_GET_DRIVE_GEOMETRY:
            if (params->output_size < sizeof(DISK_GEOMETRY))
                return STATUS_BUFFER_TOO_SMALL;
            params->ret_size = sizeof(DISK_GEOMETRY);
            return get_drive_geometry( params->cdrom, params->output );

        case IOCTL_CDROM_GET_VOLUME:
            if (params->output_size < sizeof(VOLUME_CONTROL))
                return STATUS_BUFFER_TOO_SMALL;
            params->ret_size = sizeof(VOLUME_CONTROL);
            return get_volume( params->cdrom, params->output );

        case IOCTL_CDROM_LOAD_MEDIA:
        case IOCTL_STORAGE_LOAD_MEDIA:
            return load_media( params->cdrom );

        case IOCTL_CDROM_MEDIA_REMOVAL:
        case IOCTL_DISK_MEDIA_REMOVAL:
        case IOCTL_STORAGE_EJECTION_CONTROL:
        case IOCTL_STORAGE_MEDIA_REMOVAL:
            /* FIXME: IOCTL_STORAGE_EJECTION_CONTROL is supposed to track the
             * file object which has requested to prevent ejection, and ignore
             * requests from other file objects. We don't handle that yet. */
            if (params->input_size < sizeof(PREVENT_MEDIA_REMOVAL))
                return STATUS_INFO_LENGTH_MISMATCH;
            return media_removal( params->cdrom, params->input );

        case IOCTL_CDROM_PAUSE_AUDIO:
            return pause_audio( params->cdrom );

        case IOCTL_CDROM_PLAY_AUDIO_MSF:
            if (params->input_size < sizeof(CDROM_PLAY_AUDIO_MSF))
                return STATUS_INFO_LENGTH_MISMATCH;
            return play_audio_msf( params->cdrom, params->input );

        case IOCTL_CDROM_RAW_READ:
            if (params->input_size < sizeof(RAW_READ_INFO))
                return STATUS_BUFFER_TOO_SMALL;
            return raw_read( params->cdrom, params->input, params->output_size, params->output, &params->ret_size );

        case IOCTL_CDROM_READ_Q_CHANNEL:
            if (params->input_size < sizeof(CDROM_SUB_Q_DATA_FORMAT))
                return STATUS_INFO_LENGTH_MISMATCH;
            return read_q_channel( params->cdrom, params->input, params->output_size, params->output, &params->ret_size );

        case IOCTL_CDROM_READ_TOC:
            if (params->output_size < sizeof(CDROM_TOC))
                return STATUS_BUFFER_TOO_SMALL;
            params->ret_size = sizeof(CDROM_TOC);
            return read_toc( params->cdrom, params->output );

        case IOCTL_STORAGE_RESET_DEVICE:
            return reset_device( params->cdrom );

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

        case IOCTL_CDROM_CHECK_VERIFY:
        case IOCTL_DISK_CHECK_VERIFY:
        case IOCTL_STORAGE_CHECK_VERIFY:
        case IOCTL_STORAGE_CHECK_VERIFY2:
            return check_verify( params->cdrom );

        case IOCTL_DVD_END_SESSION:
            if (params->input_size < sizeof(DVD_SESSION_ID))
                return STATUS_INVALID_PARAMETER;
            return dvd_end_session( params->cdrom, params->input );

        case IOCTL_DVD_GET_REGION:
            if (params->output_size < sizeof(DVD_REGION))
                return STATUS_BUFFER_TOO_SMALL;
            params->ret_size = sizeof(DVD_REGION);
            return dvd_get_region( params->cdrom, params->output );

        case IOCTL_DVD_READ_KEY:
            if (params->input_size < sizeof(DVD_COPY_PROTECT_KEY))
                return STATUS_INVALID_PARAMETER;
            if (params->output_size < sizeof(DVD_COPY_PROTECT_KEY))
                return STATUS_BUFFER_TOO_SMALL;
            params->ret_size = sizeof(DVD_COPY_PROTECT_KEY);
            return dvd_read_key( params->cdrom, params->input, params->output );

        case IOCTL_DVD_READ_STRUCTURE:
            if (params->input_size < sizeof(DVD_READ_STRUCTURE))
                return STATUS_INVALID_PARAMETER;
            return dvd_read_structure( params->cdrom, params->input, params->output_size,
                                       params->output, &params->ret_size );

        case IOCTL_DVD_SEND_KEY:
            if (params->input_size < sizeof(DVD_COPY_PROTECT_KEY))
                return STATUS_INVALID_PARAMETER;
            return dvd_send_key( params->cdrom, params->input );

        case IOCTL_DVD_START_SESSION:
            if (params->output_size < sizeof(DVD_SESSION_ID))
                return STATUS_BUFFER_TOO_SMALL;
            params->ret_size = sizeof(DVD_SESSION_ID);
            return dvd_start_session( params->cdrom, params->output );

        case IOCTL_SCSI_GET_ADDRESS:
            if (params->output_size < sizeof(SCSI_ADDRESS))
                return STATUS_BUFFER_TOO_SMALL;
            params->ret_size = sizeof(SCSI_ADDRESS);
            return scsi_get_address( params->cdrom, params->output );

        case IOCTL_SCSI_GET_CAPABILITIES:
            if (params->output_size < sizeof(IO_SCSI_CAPABILITIES))
                return STATUS_BUFFER_TOO_SMALL;
            params->ret_size = sizeof(IO_SCSI_CAPABILITIES);
            return scsi_get_capabilities( params->cdrom, params->output );

        case IOCTL_SCSI_GET_INQUIRY_DATA:
            return scsi_get_inquiry_data( params->cdrom, params->output_size,
                                          params->output, &params->ret_size );

        case IOCTL_SCSI_PASS_THROUGH:
            if (params->wow64)
            {
                if (params->input_size < sizeof(SCSI_PASS_THROUGH32))
                    return STATUS_INVALID_PARAMETER;
                if (params->output_size < sizeof(SCSI_PASS_THROUGH32))
                    return STATUS_BUFFER_TOO_SMALL;
                params->ret_size = sizeof(SCSI_PASS_THROUGH32);
                return scsi_pass_through_32( params->cdrom, params->input, params->output );
            }
            else
            {
                if (params->input_size < sizeof(SCSI_PASS_THROUGH))
                    return STATUS_INVALID_PARAMETER;
                if (params->output_size < sizeof(SCSI_PASS_THROUGH))
                    return STATUS_BUFFER_TOO_SMALL;
                params->ret_size = sizeof(SCSI_PASS_THROUGH);
                return scsi_pass_through( params->cdrom, params->input, params->output );
            }

        case IOCTL_SCSI_PASS_THROUGH_DIRECT:
            if (params->wow64)
            {
                if (params->input_size < sizeof(SCSI_PASS_THROUGH_DIRECT32))
                    return STATUS_INVALID_PARAMETER;
                if (params->output_size < sizeof(SCSI_PASS_THROUGH_DIRECT32))
                    return STATUS_BUFFER_TOO_SMALL;
                params->ret_size = sizeof(SCSI_PASS_THROUGH_DIRECT32);
                return scsi_pass_through_direct_32( params->cdrom, params->input, params->output );
            }
            else
            {
                if (params->input_size < sizeof(SCSI_PASS_THROUGH_DIRECT))
                    return STATUS_INVALID_PARAMETER;
                if (params->output_size < sizeof(SCSI_PASS_THROUGH_DIRECT))
                    return STATUS_BUFFER_TOO_SMALL;
                params->ret_size = sizeof(SCSI_PASS_THROUGH_DIRECT);
                return scsi_pass_through_direct( params->cdrom, params->input, params->output );
            }

        default:
            FIXME("Unsupported ioctl %#x (device=%#x access=%#x func=%#x method=%#x)\n",
                  code, code >> 16, (code >> 14) & 3, (code >> 2) & 0xfff, code & 3);
            return STATUS_NOT_SUPPORTED;
    }
}
