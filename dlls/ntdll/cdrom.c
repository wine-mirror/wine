/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/* Main file for CD-ROM support
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1999, 2001 Eric Pouech
 * Copyright 2000 Andreas Mohr
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <errno.h>
#include <string.h>
#include <stdio.h>
#ifdef HAVE_IO_H
# include <io.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SCSI_SG_H
# include <scsi/sg.h>
#endif
#ifdef HAVE_LINUX_MAJOR_H
# include <linux/major.h>
#endif
#ifdef HAVE_LINUX_HDREG_H
# include <linux/hdreg.h>
#endif
#ifdef HAVE_LINUX_PARAM_H
# include <linux/param.h>
#endif
#ifdef HAVE_LINUX_CDROM_H
# include <linux/cdrom.h>
#endif
#ifdef HAVE_LINUX_UCDROM_H
# include <linux/ucdrom.h>
#endif
#ifdef HAVE_SYS_CDIO_H
# include <sys/cdio.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "winternl.h"
#include "winioctl.h"
#include "ntddstor.h"
#include "ntddcdrm.h"
#include "ntddscsi.h"
#include "drive.h"
#include "file.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cdrom);

#ifdef linux

# ifndef IDE6_MAJOR
#  define IDE6_MAJOR 88
# endif
# ifndef IDE7_MAJOR
#  define IDE7_MAJOR 89
# endif

# ifdef CDROM_SEND_PACKET
/* structure for CDROM_PACKET_COMMAND ioctl */
/* not all Linux versions have all the fields, so we define the
 * structure ourselves to make sure */
struct linux_cdrom_generic_command
{
    unsigned char          cmd[CDROM_PACKET_SIZE];
    unsigned char         *buffer;
    unsigned int           buflen;
    int                    stat;
    struct request_sense  *sense;
    unsigned char          data_direction;
    int                    quiet;
    int                    timeout;
    void                  *reserved[1];
};
# endif  /* CDROM_SEND_PACKET */

#endif  /* linux */

/* FIXME: this is needed because we can't open simultaneously several times /dev/cdrom
 * this should be removed when a proper device interface is implemented
 */
struct cdrom_cache {
    int fd;
    int count;
};
static struct cdrom_cache cdrom_cache[26];

/******************************************************************
 *		CDROM_GetIdeInterface
 *
 * Determines the ide interface (the number after the ide), and the
 * number of the device on that interface for ide cdroms.
 * Returns false if the info could not be get
 *
 * NOTE: this function is used in CDROM_InitRegistry and CDROM_GetAddress
 */
static int CDROM_GetIdeInterface(int dev, int* iface, int* device)
{
#if defined(linux)
    {
        struct stat st;
#ifdef SG_EMULATED_HOST
        if (ioctl(dev, SG_EMULATED_HOST) != -1) {
            FIXME("not implemented for true scsi drives\n");
            return 0;
        }
#endif
        if ( fstat(dev, &st) == -1 || ! S_ISBLK(st.st_mode)) {
            FIXME("cdrom not a block device!!!\n");
            return 0;
        }
        switch (major(st.st_rdev)) {
            case IDE0_MAJOR: *iface = 0; break;
            case IDE1_MAJOR: *iface = 1; break;
            case IDE2_MAJOR: *iface = 2; break;
            case IDE3_MAJOR: *iface = 3; break;
            case IDE4_MAJOR: *iface = 4; break;
            case IDE5_MAJOR: *iface = 5; break;
            case IDE6_MAJOR: *iface = 6; break;
            case IDE7_MAJOR: *iface = 7; break;
            case SCSI_CDROM_MAJOR: *iface = 11; break;
            default:
                FIXME("CD-ROM device with major ID %d not supported\n", major(st.st_rdev));
                break;
        }
        *device = (minor(st.st_rdev) == 63 ? 1 : 0);
        return 1;
    }
#elif defined(__FreeBSD__) || defined(__NetBSD__)
    FIXME("not implemented for BSD\n");
    return 0;
#else
    FIXME("not implemented for nonlinux\n");
    return 0;
#endif
}


/******************************************************************
 *		CDROM_InitRegistry
 *
 * Initializes registry to contain scsi info about the cdrom in NT.
 * All devices (even not real scsi ones) have this info in NT.
 * TODO: for now it only works for non scsi devices
 * NOTE: programs usually read these registry entries after sending the
 *       IOCTL_SCSI_GET_ADDRESS ioctl to the cdrom
 */
void CDROM_InitRegistry(int dev)
{
    int portnum, targetid;
    int dma;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    WCHAR dataW[50];
    DWORD lenW;
    char buffer[40];
    DWORD value;
    const char *data;
    HKEY scsiKey;
    HKEY portKey;
    HKEY busKey;
    HKEY targetKey;
    DWORD disp;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    if ( ! CDROM_GetIdeInterface(dev, &portnum, &targetid))
        return;

    /* Ensure there is Scsi key */
    if (!RtlCreateUnicodeStringFromAsciiz( &nameW, "Machine\\HARDWARE\\DEVICEMAP\\Scsi" ) ||
        NtCreateKey( &scsiKey, KEY_ALL_ACCESS, &attr, 0,
                     NULL, REG_OPTION_VOLATILE, &disp ))
    {
        ERR("Cannot create DEVICEMAP\\Scsi registry key\n" );
        return;
    }
    RtlFreeUnicodeString( &nameW );

    snprintf(buffer,40,"Scsi Port %d",portnum);
    attr.RootDirectory = scsiKey;
    if (!RtlCreateUnicodeStringFromAsciiz( &nameW, buffer ) ||
        NtCreateKey( &portKey, KEY_ALL_ACCESS, &attr, 0,
                     NULL, REG_OPTION_VOLATILE, &disp ))
    {
        ERR("Cannot create DEVICEMAP\\Scsi Port registry key\n" );
        return;
    }
    RtlFreeUnicodeString( &nameW );

    RtlCreateUnicodeStringFromAsciiz( &nameW, "Driver" );
    data = "atapi";
    RtlMultiByteToUnicodeN( dataW, 50, &lenW, data, strlen(data));
    NtSetValueKey( portKey, &nameW, 0, REG_SZ, (BYTE*)dataW, lenW );
    RtlFreeUnicodeString( &nameW );
    value = 10;
    RtlCreateUnicodeStringFromAsciiz( &nameW, "FirstBusTimeScanInMs" );
    NtSetValueKey( portKey,&nameW, 0, REG_DWORD, (BYTE *)&value, sizeof(DWORD));
    RtlFreeUnicodeString( &nameW );
    value = 0;
#ifdef HDIO_GET_DMA
    if (ioctl(dev,HDIO_GET_DMA, &dma) != -1) {
        value = dma;
        TRACE("setting dma to %lx\n", value);
    }
#endif
    RtlCreateUnicodeStringFromAsciiz( &nameW, "DMAEnabled" );
    NtSetValueKey( portKey,&nameW, 0, REG_DWORD, (BYTE *)&value, sizeof(DWORD));
    RtlFreeUnicodeString( &nameW );

    attr.RootDirectory = portKey;
    if (!RtlCreateUnicodeStringFromAsciiz( &nameW, "Scsi Bus 0" ) ||
        NtCreateKey( &busKey, KEY_ALL_ACCESS, &attr, 0,
                     NULL, REG_OPTION_VOLATILE, &disp ))
    {
        ERR("Cannot create DEVICEMAP\\Scsi Port\\Scsi Bus registry key\n" );
        return;
    }
    RtlFreeUnicodeString( &nameW );

    attr.RootDirectory = busKey;
    if (!RtlCreateUnicodeStringFromAsciiz( &nameW, "Initiator Id 255" ) ||
        NtCreateKey( &targetKey, KEY_ALL_ACCESS, &attr, 0,
                     NULL, REG_OPTION_VOLATILE, &disp ))
    {
        ERR("Cannot create DEVICEMAP\\Scsi Port\\Scsi Bus\\Initiator Id 255 registry key\n" );
        return;
    }
    RtlFreeUnicodeString( &nameW );
    NtClose( targetKey );

    snprintf(buffer,40,"Target Id %d", targetid);
    attr.RootDirectory = busKey;
    if (!RtlCreateUnicodeStringFromAsciiz( &nameW, buffer ) ||
        NtCreateKey( &targetKey, KEY_ALL_ACCESS, &attr, 0,
                     NULL, REG_OPTION_VOLATILE, &disp ))
    {
        ERR("Cannot create DEVICEMAP\\Scsi Port\\Scsi Bus 0\\Target Id registry key\n" );
        return;
    }
    RtlFreeUnicodeString( &nameW );

    RtlCreateUnicodeStringFromAsciiz( &nameW, "Type" );
    data = "CdRomPeripheral";
    RtlMultiByteToUnicodeN( dataW, 50, &lenW, data, strlen(data));
    NtSetValueKey( targetKey, &nameW, 0, REG_SZ, (BYTE*)dataW, lenW );
    RtlFreeUnicodeString( &nameW );
    /* FIXME - maybe read the real identifier?? */
    RtlCreateUnicodeStringFromAsciiz( &nameW, "Identifier" );
    data = "Wine CDROM";
    RtlMultiByteToUnicodeN( dataW, 50, &lenW, data, strlen(data));
    NtSetValueKey( targetKey, &nameW, 0, REG_SZ, (BYTE*)dataW, lenW );
    RtlFreeUnicodeString( &nameW );
    /* FIXME - we always use Cdrom0 - do not know about the nt behaviour */
    RtlCreateUnicodeStringFromAsciiz( &nameW, "DeviceName" );
    data = "Cdrom0";
    RtlMultiByteToUnicodeN( dataW, 50, &lenW, data, strlen(data));
    NtSetValueKey( targetKey, &nameW, 0, REG_SZ, (BYTE*)dataW, lenW );
    RtlFreeUnicodeString( &nameW );

    NtClose( targetKey );
    NtClose( busKey );
    NtClose( portKey );
    NtClose( scsiKey );
}


/******************************************************************
 *		CDROM_Open
 *
 *
 */
static int CDROM_Open(HANDLE hDevice, DWORD clientID)
{
    int dev = LOWORD(clientID);

    if (dev >= 26) return -1;

    if (!cdrom_cache[dev].count)
    {
        char root[4];
        const char *device;

        strcpy(root, "A:\\");
        root[0] += dev;
        if (GetDriveTypeA(root) != DRIVE_CDROM) return -1;
        if (!(device = DRIVE_GetDevice(dev))) return -1;
        cdrom_cache[dev].fd = open(device, O_RDONLY|O_NONBLOCK);
        if (cdrom_cache[dev].fd == -1)
        {
            FIXME("Can't open configured CD-ROM drive at %s (device %s): %s\n", root, DRIVE_GetDevice(dev), strerror(errno));
            return -1;
        }
    }
    cdrom_cache[dev].count++;
    return cdrom_cache[dev].fd;
}

/******************************************************************
 *		CDROM_Close
 *
 *
 */
static void CDROM_Close(DWORD clientID, int fd)
{
    int dev = LOWORD(clientID);

    if (dev >= 26 || fd != cdrom_cache[dev].fd) FIXME("how come\n");
    if (--cdrom_cache[dev].count == 0)
        close(cdrom_cache[dev].fd);
}

/******************************************************************
 *		CDROM_GetStatusCode
 *
 *
 */
static DWORD CDROM_GetStatusCode(int io)
{
    if (io == 0) return 0;
    switch (errno)
    {
    case EIO:
#ifdef ENOMEDIUM
    case ENOMEDIUM:
#endif
	    return STATUS_NO_MEDIA_IN_DEVICE;
    case EPERM:
	    return STATUS_ACCESS_DENIED;
    }
    FIXME("Unmapped error code %d: %s\n", errno, strerror(errno));
    return STATUS_IO_DEVICE_ERROR;
}

static DWORD CDROM_GetControl(int dev, CDROM_AUDIO_CONTROL* cac)
{
    cac->LbaFormat = 0; /* FIXME */
    cac->LogicalBlocksPerSecond = 1; /* FIXME */
    return  STATUS_NOT_SUPPORTED;
}

static DWORD CDROM_GetDeviceNumber(int dev, STORAGE_DEVICE_NUMBER* devnum)
{
    return STATUS_NOT_SUPPORTED;
}

static DWORD CDROM_GetDriveGeometry(int dev, DISK_GEOMETRY* dg)
{
#if 0
    dg->Cylinders.s.LowPart = 1; /* FIXME */
    dg->Cylinders.s.HighPart = 0; /* FIXME */
    dg->MediaType = 1; /* FIXME */
    dg->TracksPerCylinder = 1; /* FIXME */
    dg->SectorsPerTrack = 1; /* FIXME */
    dg->BytesPerSector= 1; /* FIXME */
#endif
    return STATUS_NOT_SUPPORTED;
}

/**************************************************************************
 *                              CDROM_Reset                     [internal]
 */
static DWORD CDROM_ResetAudio(int dev)
{
#if defined(linux)
    return CDROM_GetStatusCode(ioctl(dev, CDROMRESET));
#elif defined(__FreeBSD__) || defined(__NetBSD__)
    return CDROM_GetStatusCode(ioctl(dev, CDIOCRESET, NULL));
#else
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *		CDROM_SetTray
 *
 *
 */
static DWORD CDROM_SetTray(int dev, BOOL doEject)
{
#if defined(linux)
    return CDROM_GetStatusCode(ioctl(dev, doEject ? CDROMEJECT : CDROMCLOSETRAY));
#elif defined(__FreeBSD__) || defined(__NetBSD__)
    return CDROM_GetStatusCode((ioctl(dev, CDIOCALLOW, NULL)) ||
                               (ioctl(dev, doEject ? CDIOCEJECT : CDIOCCLOSE, NULL)) ||
                               (ioctl(dev, CDIOCPREVENT, NULL)));
#else
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *		CDROM_ControlEjection
 *
 *
 */
static DWORD CDROM_ControlEjection(int dev, const PREVENT_MEDIA_REMOVAL* rmv)
{
#if defined(linux)
    return CDROM_GetStatusCode(ioctl(dev, CDROM_LOCKDOOR, rmv->PreventMediaRemoval));
#elif defined(__FreeBSD__) || defined(__NetBSD__)
    return CDROM_GetStatusCode(ioctl(dev, (rmv->PreventMediaRemoval) ? CDIOCPREVENT : CDIOCALLOW, NULL));
#else
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *		CDROM_ReadTOC
 *
 *
 */
static DWORD CDROM_ReadTOC(int dev, CDROM_TOC* toc)
{
    DWORD       ret = STATUS_NOT_SUPPORTED;

#if defined(linux)
    int                         i, io = -1;
    struct cdrom_tochdr		hdr;
    struct cdrom_tocentry	entry;

    io = ioctl(dev, CDROMREADTOCHDR, &hdr);
    if (io == -1)
    {
        WARN("(%d) -- Error occurred (%s)!\n", dev, strerror(errno));
        goto end;
    }
    toc->FirstTrack = hdr.cdth_trk0;
    toc->LastTrack  = hdr.cdth_trk1;

    TRACE("from=%d to=%d\n", toc->FirstTrack, toc->LastTrack);

    for (i = toc->FirstTrack; i <= toc->LastTrack + 1; i++)
    {
	if (i == toc->LastTrack + 1)
        {
	    entry.cdte_track = CDROM_LEADOUT;
        } else {
            entry.cdte_track = i;
        }
	entry.cdte_format = CDROM_MSF;
	io = ioctl(dev, CDROMREADTOCENTRY, &entry);
	if (io == -1) {
	    WARN("error read entry (%s)\n", strerror(errno));
	    goto end;
	}
        toc->TrackData[i - toc->FirstTrack].Control = entry.cdte_ctrl;
        toc->TrackData[i - toc->FirstTrack].Adr = entry.cdte_adr;
        /* marking last track with leadout value as index */
        toc->TrackData[i - toc->FirstTrack].TrackNumber = entry.cdte_track;
        toc->TrackData[i - toc->FirstTrack].Address[0] = 0;
        toc->TrackData[i - toc->FirstTrack].Address[1] = entry.cdte_addr.msf.minute;
        toc->TrackData[i - toc->FirstTrack].Address[2] = entry.cdte_addr.msf.second;
        toc->TrackData[i - toc->FirstTrack].Address[3] = entry.cdte_addr.msf.frame;
    }
end:
    ret = CDROM_GetStatusCode(io);
#elif defined(__FreeBSD__) || defined(__NetBSD__)
    int                         i, io = -1;
    struct ioc_toc_header	hdr;
    struct ioc_read_toc_entry	entry;
    struct cd_toc_entry         toc_buffer;

    io = ioctl(dev, CDIOREADTOCHEADER, &hdr);
    if (io == -1)
    {
        WARN("(%d) -- Error occurred (%s)!\n", dev, strerror(errno));
        goto end;
    }
    toc->FirstTrack = hdr.starting_track;
    toc->LastTrack  = hdr.ending_track;

    TRACE("from=%d to=%d\n", toc->FirstTrack, toc->LastTrack);

    for (i = toc->FirstTrack; i <= toc->LastTrack + 1; i++)
    {
	if (i == toc->LastTrack + 1)
        {
#define LEADOUT 0xaa
	    entry.starting_track = LEADOUT;
        } else {
            entry.starting_track = i;
        }
	memset((char *)&toc_buffer, 0, sizeof(toc_buffer));
	entry.address_format = CD_MSF_FORMAT;
	entry.data_len = sizeof(toc_buffer);
	entry.data = &toc_buffer;
	io = ioctl(dev, CDIOREADTOCENTRYS, &entry);
	if (io == -1) {
	    WARN("error read entry (%s)\n", strerror(errno));
	    goto end;
	}
        toc->TrackData[i - toc->FirstTrack].Control = toc_buffer.control;
        toc->TrackData[i - toc->FirstTrack].Adr = toc_buffer.addr_type;
        /* marking last track with leadout value as index */
        toc->TrackData[i - toc->FirstTrack].TrackNumber = entry.starting_track;
        toc->TrackData[i - toc->FirstTrack].Address[0] = 0;
        toc->TrackData[i - toc->FirstTrack].Address[1] = toc_buffer.addr.msf.minute;
        toc->TrackData[i - toc->FirstTrack].Address[2] = toc_buffer.addr.msf.second;
        toc->TrackData[i - toc->FirstTrack].Address[3] = toc_buffer.addr.msf.frame;
    }
end:
    ret = CDROM_GetStatusCode(io);
#else
    ret = STATUS_NOT_SUPPORTED;
#endif
    return ret;
}

/******************************************************************
 *		CDROM_GetDiskData
 *
 *
 */
static DWORD CDROM_GetDiskData(int dev, CDROM_DISK_DATA* data)
{
    CDROM_TOC   toc;
    DWORD       ret;
    int         i;

    if ((ret = CDROM_ReadTOC(dev, &toc)) != 0) return ret;
    data->DiskData = 0;
    for (i = toc.FirstTrack; i <= toc.LastTrack; i++) {
        if (toc.TrackData[i].Control & 0x04)
            data->DiskData |= CDROM_DISK_DATA_TRACK;
        else
            data->DiskData |= CDROM_DISK_AUDIO_TRACK;
    }
    return 0;
}

/******************************************************************
 *		CDROM_ReadQChannel
 *
 *
 */
static DWORD CDROM_ReadQChannel(int dev, const CDROM_SUB_Q_DATA_FORMAT* fmt,
                                SUB_Q_CHANNEL_DATA* data)
{
    DWORD               ret = STATUS_NOT_SUPPORTED;
#ifdef linux
    unsigned            size;
    SUB_Q_HEADER*       hdr = (SUB_Q_HEADER*)data;
    int                 io;
    struct cdrom_subchnl	sc;
    sc.cdsc_format = CDROM_MSF;

    io = ioctl(dev, CDROMSUBCHNL, &sc);
    if (io == -1)
    {
	TRACE("opened or no_media (%s)!\n", strerror(errno));
	hdr->AudioStatus = AUDIO_STATUS_NO_STATUS;
	goto end;
    }

    hdr->AudioStatus = AUDIO_STATUS_NOT_SUPPORTED;

    switch (sc.cdsc_audiostatus) {
    case CDROM_AUDIO_INVALID:
	hdr->AudioStatus = AUDIO_STATUS_NOT_SUPPORTED;
	break;
    case CDROM_AUDIO_NO_STATUS:
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
	TRACE("status=%02X !\n", sc.cdsc_audiostatus);
        break;
    }
    switch (fmt->Format)
    {
    case IOCTL_CDROM_CURRENT_POSITION:
        size = sizeof(SUB_Q_CURRENT_POSITION);
        data->CurrentPosition.FormatCode = sc.cdsc_format;
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
        break;
    case IOCTL_CDROM_MEDIA_CATALOG:
        size = sizeof(SUB_Q_MEDIA_CATALOG_NUMBER);
        data->MediaCatalog.FormatCode = IOCTL_CDROM_MEDIA_CATALOG;
        {
            struct cdrom_mcn mcn;
            if ((io = ioctl(dev, CDROM_GET_MCN, &mcn)) == -1) goto end;

            data->MediaCatalog.FormatCode = IOCTL_CDROM_MEDIA_CATALOG;
            data->MediaCatalog.Mcval = 0; /* FIXME */
            memcpy(data->MediaCatalog.MediaCatalog, mcn.medium_catalog_number, 14);
            data->MediaCatalog.MediaCatalog[14] = 0;
        }
        break;
    case IOCTL_CDROM_TRACK_ISRC:
        size = sizeof(SUB_Q_CURRENT_POSITION);
        FIXME("TrackIsrc: NIY on linux\n");
        data->TrackIsrc.FormatCode = IOCTL_CDROM_TRACK_ISRC;
        data->TrackIsrc.Tcval = 0;
        io = 0;
        break;
    }

 end:
    ret = CDROM_GetStatusCode(io);
#elif defined(__FreeBSD__) || defined(__NetBSD__)
    unsigned            size;
    SUB_Q_HEADER*       hdr = (SUB_Q_HEADER*)data;
    int                 io;
    struct ioc_read_subchannel	read_sc;
    struct cd_sub_channel_info	sc;

    read_sc.address_format = CD_MSF_FORMAT;
    read_sc.track          = 0;
    read_sc.data_len       = sizeof(sc);
    read_sc.data           = &sc;
    switch (fmt->Format)
    {
    case IOCTL_CDROM_CURRENT_POSITION:
        read_sc.data_format    = CD_CURRENT_POSITION;
        break;
    case IOCTL_CDROM_MEDIA_CATALOG:
        read_sc.data_format    = CD_MEDIA_CATALOG;
        break;
    case IOCTL_CDROM_TRACK_ISRC:
        read_sc.data_format    = CD_TRACK_INFO;
        sc.what.track_info.track_number = data->TrackIsrc.Track;
        break;
    }
    io = ioctl(dev, CDIOCREADSUBCHANNEL, &read_sc);
    if (io == -1)
    {
	TRACE("opened or no_media (%s)!\n", strerror(errno));
	hdr->AudioStatus = AUDIO_STATUS_NO_STATUS;
	goto end;
    }

    hdr->AudioStatus = AUDIO_STATUS_NOT_SUPPORTED;

    switch (sc.header.audio_status) {
    case CD_AS_AUDIO_INVALID:
	hdr->AudioStatus = AUDIO_STATUS_NOT_SUPPORTED;
	break;
    case CD_AS_NO_STATUS:
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
	TRACE("status=%02X !\n", sc.header.audio_status);
    }
    switch (fmt->Format)
    {
    case IOCTL_CDROM_CURRENT_POSITION:
        size = sizeof(SUB_Q_CURRENT_POSITION);
        data->CurrentPosition.FormatCode = sc.what.position.data_format;
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
        break;
    case IOCTL_CDROM_MEDIA_CATALOG:
        size = sizeof(SUB_Q_MEDIA_CATALOG_NUMBER);
        data->MediaCatalog.FormatCode = IOCTL_CDROM_MEDIA_CATALOG;
        data->MediaCatalog.FormatCode = sc.what.media_catalog.data_format;
        data->MediaCatalog.Mcval = sc.what.media_catalog.mc_valid;
        memcpy(data->MediaCatalog.MediaCatalog, sc.what.media_catalog.mc_number, 15);
        break;
    case IOCTL_CDROM_TRACK_ISRC:
        size = sizeof(SUB_Q_CURRENT_POSITION);
        data->TrackIsrc.Tcval = sc.what.track_info.ti_valid;
        memcpy(data->TrackIsrc.TrackIsrc, sc.what.track_info.ti_number, 15);
        break;
    }

 end:
    ret = CDROM_GetStatusCode(io);
#endif
    return ret;
}

/******************************************************************
 *		CDROM_Verify
 *
 *
 */
static DWORD CDROM_Verify(int dev)
{
    /* quick implementation */
    CDROM_SUB_Q_DATA_FORMAT     fmt;
    SUB_Q_CHANNEL_DATA          data;

    fmt.Format = IOCTL_CDROM_CURRENT_POSITION;
    return CDROM_ReadQChannel(dev, &fmt, &data) ? 1 : 0;
}

/******************************************************************
 *		CDROM_PlayAudioMSF
 *
 *
 */
static DWORD CDROM_PlayAudioMSF(int dev, const CDROM_PLAY_AUDIO_MSF* audio_msf)
{
    DWORD       ret = STATUS_NOT_SUPPORTED;
#ifdef linux
    struct 	cdrom_msf	msf;
    int         io;

    msf.cdmsf_min0   = audio_msf->StartingM;
    msf.cdmsf_sec0   = audio_msf->StartingS;
    msf.cdmsf_frame0 = audio_msf->StartingF;
    msf.cdmsf_min1   = audio_msf->EndingM;
    msf.cdmsf_sec1   = audio_msf->EndingS;
    msf.cdmsf_frame1 = audio_msf->EndingF;

    io = ioctl(dev, CDROMSTART);
    if (io == -1)
    {
	WARN("motor doesn't start !\n");
	goto end;
    }
    io = ioctl(dev, CDROMPLAYMSF, &msf);
    if (io == -1)
    {
	WARN("device doesn't play !\n");
	goto end;
    }
    TRACE("msf = %d:%d:%d %d:%d:%d\n",
	  msf.cdmsf_min0, msf.cdmsf_sec0, msf.cdmsf_frame0,
	  msf.cdmsf_min1, msf.cdmsf_sec1, msf.cdmsf_frame1);
 end:
    ret = CDROM_GetStatusCode(io);
#elif defined(__FreeBSD__) || defined(__NetBSD__)
    struct	ioc_play_msf	msf;
    int         io;

    msf.start_m      = audio_msf->StartingM;
    msf.start_s      = audio_msf->StartingS;
    msf.start_f      = audio_msf->StartingF;
    msf.end_m        = audio_msf->EndingM;
    msf.end_s        = audio_msf->EndingS;
    msf.end_f        = audio_msf->EndingF;

    io = ioctl(dev, CDIOCSTART, NULL);
    if (io == -1)
    {
	WARN("motor doesn't start !\n");
	goto end;
    }
    io = ioctl(dev, CDIOCPLAYMSF, &msf);
    if (io == -1)
    {
	WARN("device doesn't play !\n");
	goto end;
    }
    TRACE("msf = %d:%d:%d %d:%d:%d\n",
	  msf.start_m, msf.start_s, msf.start_f,
	  msf.end_m,   msf.end_s,   msf.end_f);
end:
    ret = CDROM_GetStatusCode(io);
#endif
    return ret;
}

/******************************************************************
 *		CDROM_SeekAudioMSF
 *
 *
 */
static DWORD CDROM_SeekAudioMSF(int dev, const CDROM_SEEK_AUDIO_MSF* audio_msf)
{
#if defined(linux)
    struct cdrom_msf0	msf;
    msf.minute = audio_msf->M;
    msf.second = audio_msf->S;
    msf.frame  = audio_msf->F;

    return CDROM_GetStatusCode(ioctl(dev, CDROMSEEK, &msf));
#elif defined(__FreeBSD__) || defined(__NetBSD__)
    FIXME("Could a BSD expert implement the seek function ?\n");
    return STATUS_NOT_SUPPORTED;
#else
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *		CDROM_PauseAudio
 *
 *
 */
static DWORD CDROM_PauseAudio(int dev)
{
#if defined(linux)
    return CDROM_GetStatusCode(ioctl(dev, CDROMPAUSE));
#elif defined(__FreeBSD__) || defined(__NetBSD__)
    return CDROM_GetStatusCode(ioctl(dev, CDIOCPAUSE, NULL));
#else
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *		CDROM_ResumeAudio
 *
 *
 */
static DWORD CDROM_ResumeAudio(int dev)
{
#if defined(linux)
    return CDROM_GetStatusCode(ioctl(dev, CDROMRESUME));
#elif defined(__FreeBSD__) || defined(__NetBSD__)
    return CDROM_GetStatusCode(ioctl(dev, CDIOCRESUME, NULL));
#else
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *		CDROM_StopAudio
 *
 *
 */
static DWORD CDROM_StopAudio(int dev)
{
#if defined(linux)
    return CDROM_GetStatusCode(ioctl(dev, CDROMSTOP));
#elif defined(__FreeBSD__) || defined(__NetBSD__)
    return CDROM_GetStatusCode(ioctl(dev, CDIOCSTOP, NULL));
#else
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *		CDROM_GetVolume
 *
 *
 */
static DWORD CDROM_GetVolume(int dev, VOLUME_CONTROL* vc)
{
#if defined(linux)
    struct cdrom_volctrl volc;
    int io;

    io = ioctl(dev, CDROMVOLREAD, &volc);
    if (io != -1)
    {
        vc->PortVolume[0] = volc.channel0;
        vc->PortVolume[1] = volc.channel1;
        vc->PortVolume[2] = volc.channel2;
        vc->PortVolume[3] = volc.channel3;
    }
    return CDROM_GetStatusCode(io);
#elif defined(__FreeBSD__) || defined(__NetBSD__)
    struct  ioc_vol     volc;
    int io;

    io = ioctl(dev, CDIOCGETVOL, &volc);
    if (io != -1)
    {
        vc->PortVolume[0] = volc.vol[0];
        vc->PortVolume[1] = volc.vol[1];
        vc->PortVolume[2] = volc.vol[2];
        vc->PortVolume[3] = volc.vol[3];
    }
    return CDROM_GetStatusCode(io);
#else
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *		CDROM_SetVolume
 *
 *
 */
static DWORD CDROM_SetVolume(int dev, const VOLUME_CONTROL* vc)
{
#if defined(linux)
    struct cdrom_volctrl volc;

    volc.channel0 = vc->PortVolume[0];
    volc.channel1 = vc->PortVolume[1];
    volc.channel2 = vc->PortVolume[2];
    volc.channel3 = vc->PortVolume[3];

    return CDROM_GetStatusCode(ioctl(dev, CDROMVOLCTRL, &volc));
#elif defined(__FreeBSD__) || defined(__NetBSD__)
    struct  ioc_vol     volc;

    volc.vol[0] = vc->PortVolume[0];
    volc.vol[1] = vc->PortVolume[1];
    volc.vol[2] = vc->PortVolume[2];
    volc.vol[3] = vc->PortVolume[3];

    return CDROM_GetStatusCode(ioctl(dev, CDIOCSETVOL, &volc));
#else
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *		CDROM_RawRead
 *
 *
 */
static DWORD CDROM_RawRead(int dev, const RAW_READ_INFO* raw, void* buffer, DWORD len, DWORD* sz)
{
    int	        ret = STATUS_NOT_SUPPORTED;
    int         io = -1;
    DWORD       sectSize;

    switch (raw->TrackMode)
    {
    case YellowMode2:   sectSize = 2336;        break;
    case XAForm2:       sectSize = 2328;        break;
    case CDDA:          sectSize = 2352;        break;
    default:    return STATUS_INVALID_PARAMETER;
    }
    if (len < raw->SectorCount * sectSize) return STATUS_BUFFER_TOO_SMALL;
    /* strangely enough, it seems that sector offsets are always indicated with a size of 2048,
     * even if a larger size if read...
     */
#if defined(linux)
    {
        struct cdrom_read       cdr;
        struct cdrom_read_audio cdra;

        switch (raw->TrackMode)
        {
        case YellowMode2:
            if (raw->DiskOffset.s.HighPart) FIXME("Unsupported value\n");
            cdr.cdread_lba = raw->DiskOffset.s.LowPart; /* FIXME ? */
            cdr.cdread_bufaddr = buffer;
            cdr.cdread_buflen = raw->SectorCount * sectSize;
            io = ioctl(dev, CDROMREADMODE2, &cdr);
            break;
        case XAForm2:
            FIXME("XAForm2: NIY\n");
            return ret;
        case CDDA:
            /* FIXME: the output doesn't seem 100% correct... in fact output is shifted
             * between by NT2K box and this... should check on the same drive...
             * otherwise, I fear a 2352/2368 mismatch somewhere in one of the drivers
             * (linux/NT).
             * Anyway, that's not critical at all. We're talking of 16/32 bytes, we're
             * talking of 0.2 ms of sound
             */
            /* 2048 = 2 ** 11 */
            if (raw->DiskOffset.s.HighPart & ~2047) FIXME("Unsupported value\n");
            cdra.addr.lba = ((raw->DiskOffset.s.LowPart >> 11) |
                (raw->DiskOffset.s.HighPart << (32 - 11))) - 1;
            FIXME("reading at %u\n", cdra.addr.lba);
            cdra.addr_format = CDROM_LBA;
            cdra.nframes = raw->SectorCount;
            cdra.buf = buffer;
            io = ioctl(dev, CDROMREADAUDIO, &cdra);
            break;
        }
    }
#elif defined(__FreeBSD__)
    {
        struct ioc_read_audio   ira;

        switch (raw->TrackMode)
        {
        case YellowMode2:
            FIXME("YellowMode2: NIY\n");
            return ret;
        case XAForm2:
            FIXME("XAForm2: NIY\n");
            return ret;
        case CDDA:
            /* 2048 = 2 ** 11 */
            if (raw->DiskOffset.s.HighPart & ~2047) FIXME("Unsupported value\n");
            ira.address.lba = ((raw->DiskOffset.s.LowPart >> 11) |
                raw->DiskOffset.s.HighPart << (32 - 11)) - 1;
            ira.address_format = CD_LBA_FORMAT;
            ira.nframes = raw->SectorCount;
            ira.buffer = buffer;
            io = ioctl(dev, CDIOCREADAUDIO, &ira);
            break;
        }
    }
#elif defined(__NetBSD__)
    {
        switch (raw->TrackMode)
        {
        case YellowMode2:
            FIXME("YellowMode2: NIY\n");
            return ret;
        case XAForm2:
            FIXME("XAForm2: NIY\n");
            return ret;
        case CDDA:
	    FIXME("CDDA: NIY\n");
	    return ret;
	}
    }
#endif
    *sz = sectSize * raw->SectorCount;
    ret = CDROM_GetStatusCode(io);
    return ret;
}

/******************************************************************
 *		CDROM_ScsiPassThroughDirect
 *
 *
 */
static DWORD CDROM_ScsiPassThroughDirect(int dev, PSCSI_PASS_THROUGH_DIRECT pPacket)
{
    int ret = STATUS_NOT_SUPPORTED;
#if defined(linux) && defined(CDROM_SEND_PACKET)
    struct linux_cdrom_generic_command cmd;
    struct request_sense sense;
    int io;

    if (pPacket->Length < sizeof(SCSI_PASS_THROUGH_DIRECT))
	return STATUS_BUFFER_TOO_SMALL;

    if (pPacket->CdbLength > 12)
        return STATUS_INVALID_PARAMETER;

    if (pPacket->SenseInfoLength > sizeof(sense))
        return STATUS_INVALID_PARAMETER;

    memset(&cmd, 0, sizeof(cmd));
    memset(&sense, 0, sizeof(sense));

    memcpy(&(cmd.cmd), &(pPacket->Cdb), pPacket->CdbLength);

    cmd.buffer         = pPacket->DataBuffer;
    cmd.buflen         = pPacket->DataTransferLength;
    cmd.sense          = &sense;
    cmd.quiet          = 0;
    cmd.timeout        = pPacket->TimeOutValue*HZ;

    switch (pPacket->DataIn)
    {
    case SCSI_IOCTL_DATA_OUT:
        cmd.data_direction = CGC_DATA_WRITE;
	break;
    case SCSI_IOCTL_DATA_IN:
        cmd.data_direction = CGC_DATA_READ;
	break;
    case SCSI_IOCTL_DATA_UNSPECIFIED:
        cmd.data_direction = CGC_DATA_NONE;
	break;
    default:
       return STATUS_INVALID_PARAMETER;
    }

    io = ioctl(dev, CDROM_SEND_PACKET, &cmd);

    if (pPacket->SenseInfoLength != 0)
    {
        memcpy((char*)pPacket + pPacket->SenseInfoOffset,
	       &sense, pPacket->SenseInfoLength);
    }

    pPacket->ScsiStatus = cmd.stat;

    ret = CDROM_GetStatusCode(io);
#endif
    return ret;
}

/******************************************************************
 *		CDROM_ScsiPassThrough
 *
 *
 */
static DWORD CDROM_ScsiPassThrough(int dev, PSCSI_PASS_THROUGH pPacket)
{
    int ret = STATUS_NOT_SUPPORTED;
#if defined(linux) && defined(CDROM_SEND_PACKET)
    struct linux_cdrom_generic_command cmd;
    struct request_sense sense;
    int io;

    if (pPacket->Length < sizeof(SCSI_PASS_THROUGH))
	return STATUS_BUFFER_TOO_SMALL;

    if (pPacket->CdbLength > 12)
        return STATUS_INVALID_PARAMETER;

    if (pPacket->SenseInfoLength > sizeof(sense))
        return STATUS_INVALID_PARAMETER;

    memset(&cmd, 0, sizeof(cmd));
    memset(&sense, 0, sizeof(sense));

    memcpy(&(cmd.cmd), &(pPacket->Cdb), pPacket->CdbLength);

    if ( pPacket->DataBufferOffset > 0x1000 )
    {
        cmd.buffer     = (void*)pPacket->DataBufferOffset;
    }
    else
    {
        cmd.buffer     = ((void*)pPacket) + pPacket->DataBufferOffset;
    }
    cmd.buflen         = pPacket->DataTransferLength;
    cmd.sense          = &sense;
    cmd.quiet          = 0;
    cmd.timeout        = pPacket->TimeOutValue*HZ;

    switch (pPacket->DataIn)
    {
    case SCSI_IOCTL_DATA_OUT:
        cmd.data_direction = CGC_DATA_WRITE;
	break;
    case SCSI_IOCTL_DATA_IN:
        cmd.data_direction = CGC_DATA_READ;
	break;
    case SCSI_IOCTL_DATA_UNSPECIFIED:
        cmd.data_direction = CGC_DATA_NONE;
	break;
    default:
       return STATUS_INVALID_PARAMETER;
    }

    io = ioctl(dev, CDROM_SEND_PACKET, &cmd);

    if (pPacket->SenseInfoLength != 0)
    {
        memcpy((char*)pPacket + pPacket->SenseInfoOffset,
	       &sense, pPacket->SenseInfoLength);
    }

    pPacket->ScsiStatus = cmd.stat;

    ret = CDROM_GetStatusCode(io);
#endif
    return ret;
}

/******************************************************************
 *		CDROM_GetAddress
 *
 * implements IOCTL_SCSI_GET_ADDRESS
 */
static DWORD CDROM_GetAddress(int dev, SCSI_ADDRESS* address)
{
    int portnum, targetid;

    address->Length = sizeof(SCSI_ADDRESS);
    address->PathId = 0; /* bus number */
    address->Lun = 0;
    if ( ! CDROM_GetIdeInterface(dev, &portnum, &targetid))
        return STATUS_NOT_SUPPORTED;

    address->PortNumber = portnum;
    address->TargetId = targetid;
    return 0;
}

/******************************************************************
 *		CDROM_DeviceIoControl
 *
 *
 */
BOOL CDROM_DeviceIoControl(DWORD clientID, HANDLE hDevice, DWORD dwIoControlCode,
                           LPVOID lpInBuffer, DWORD nInBufferSize,
                           LPVOID lpOutBuffer, DWORD nOutBufferSize,
                           LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped)
{
    DWORD       sz;
    DWORD       error = 0;
    int         dev;

    TRACE("%lx[%c] %lx %lx %ld %lx %ld %lx %lx\n",
          (DWORD)hDevice, 'A' + LOWORD(clientID), dwIoControlCode, (DWORD)lpInBuffer, nInBufferSize,
          (DWORD)lpOutBuffer, nOutBufferSize, (DWORD)lpBytesReturned, (DWORD)lpOverlapped);

    if (lpBytesReturned) *lpBytesReturned = 0;
    if (lpOverlapped)
    {
        FIXME("Overlapped isn't implemented yet\n");
        SetLastError(STATUS_NOT_SUPPORTED);
        return FALSE;
    }

    SetLastError(0);
    dev = CDROM_Open(hDevice, clientID);
    if (dev == -1)
    {
        CDROM_GetStatusCode(-1);
        return FALSE;
    }

    switch (dwIoControlCode)
    {
    case IOCTL_STORAGE_CHECK_VERIFY:
    case IOCTL_CDROM_CHECK_VERIFY:
        sz = 0;
        if (lpInBuffer != NULL || nInBufferSize != 0 || lpOutBuffer != NULL || nOutBufferSize != 0)
            error = STATUS_INVALID_PARAMETER;
        else error = CDROM_Verify(dev);
        break;

/* EPP     case IOCTL_STORAGE_CHECK_VERIFY2: */

/* EPP     case IOCTL_STORAGE_FIND_NEW_DEVICES: */
/* EPP     case IOCTL_CDROM_FIND_NEW_DEVICES: */

    case IOCTL_STORAGE_LOAD_MEDIA:
    case IOCTL_CDROM_LOAD_MEDIA:
        sz = 0;
        if (lpInBuffer != NULL || nInBufferSize != 0 || lpOutBuffer != NULL || nOutBufferSize != 0)
            error = STATUS_INVALID_PARAMETER;
        else error = CDROM_SetTray(dev, FALSE);
        break;
     case IOCTL_STORAGE_EJECT_MEDIA:
        sz = 0;
        if (lpInBuffer != NULL || nInBufferSize != 0 || lpOutBuffer != NULL || nOutBufferSize != 0)
            error = STATUS_INVALID_PARAMETER;
        else error = CDROM_SetTray(dev, TRUE);
        break;

    case IOCTL_CDROM_MEDIA_REMOVAL:
    case IOCTL_DISK_MEDIA_REMOVAL:
    case IOCTL_STORAGE_MEDIA_REMOVAL:
    case IOCTL_STORAGE_EJECTION_CONTROL:
        /* FIXME the last ioctl:s is not the same as the two others...
         * lockcount/owner should be handled */
        sz = 0;
        if (lpOutBuffer != NULL || nOutBufferSize != 0) error = STATUS_INVALID_PARAMETER;
        else if (nInBufferSize < sizeof(PREVENT_MEDIA_REMOVAL)) error = STATUS_BUFFER_TOO_SMALL;
        else error = CDROM_ControlEjection(dev, (const PREVENT_MEDIA_REMOVAL*)lpInBuffer);
        break;

/* EPP     case IOCTL_STORAGE_GET_MEDIA_TYPES: */

    case IOCTL_STORAGE_GET_DEVICE_NUMBER:
        sz = sizeof(STORAGE_DEVICE_NUMBER);
        if (lpInBuffer != NULL || nInBufferSize != 0) error = STATUS_INVALID_PARAMETER;
        else if (nOutBufferSize < sz) error = STATUS_BUFFER_TOO_SMALL;
        else error = CDROM_GetDeviceNumber(dev, (STORAGE_DEVICE_NUMBER*)lpOutBuffer);
        break;

    case IOCTL_STORAGE_RESET_DEVICE:
        sz = 0;
        if (lpInBuffer != NULL || nInBufferSize != 0 || lpOutBuffer != NULL || nOutBufferSize != 0)
            error = STATUS_INVALID_PARAMETER;
        else error = CDROM_ResetAudio(dev);
        break;

    case IOCTL_CDROM_GET_CONTROL:
        sz = sizeof(CDROM_AUDIO_CONTROL);
        if (lpInBuffer != NULL || nInBufferSize != 0) error = STATUS_INVALID_PARAMETER;
        else if (nOutBufferSize < sz) error = STATUS_BUFFER_TOO_SMALL;
        else error = CDROM_GetControl(dev, (CDROM_AUDIO_CONTROL*)lpOutBuffer);
        break;

    case IOCTL_CDROM_GET_DRIVE_GEOMETRY:
        sz = sizeof(DISK_GEOMETRY);
        if (lpInBuffer != NULL || nInBufferSize != 0) error = STATUS_INVALID_PARAMETER;
        else if (nOutBufferSize < sz) error = STATUS_BUFFER_TOO_SMALL;
        else error = CDROM_GetDriveGeometry(dev, (DISK_GEOMETRY*)lpOutBuffer);
        break;

    case IOCTL_CDROM_DISK_TYPE:
        sz = sizeof(CDROM_DISK_DATA);
        if (lpInBuffer != NULL || nInBufferSize != 0) error = STATUS_INVALID_PARAMETER;
        else if (nOutBufferSize < sz) error = STATUS_BUFFER_TOO_SMALL;
        else error = CDROM_GetDiskData(dev, (CDROM_DISK_DATA*)lpOutBuffer);
        break;

/* EPP     case IOCTL_CDROM_GET_LAST_SESSION: */

    case IOCTL_CDROM_READ_Q_CHANNEL:
        sz = sizeof(SUB_Q_CHANNEL_DATA);
        if (lpInBuffer == NULL || nInBufferSize < sizeof(CDROM_SUB_Q_DATA_FORMAT))
            error = STATUS_INVALID_PARAMETER;
        else if (nOutBufferSize < sz) error = STATUS_BUFFER_TOO_SMALL;
        else error = CDROM_ReadQChannel(dev, (const CDROM_SUB_Q_DATA_FORMAT*)lpInBuffer,
                                        (SUB_Q_CHANNEL_DATA*)lpOutBuffer);
        break;

    case IOCTL_CDROM_READ_TOC:
        sz = sizeof(CDROM_TOC);
        if (lpInBuffer != NULL || nInBufferSize != 0) error = STATUS_INVALID_PARAMETER;
        else if (nOutBufferSize < sz) error = STATUS_BUFFER_TOO_SMALL;
        else error = CDROM_ReadTOC(dev, (CDROM_TOC*)lpOutBuffer);
        break;

/* EPP     case IOCTL_CDROM_READ_TOC_EX: */

    case IOCTL_CDROM_PAUSE_AUDIO:
        sz = 0;
        if (lpInBuffer != NULL || nInBufferSize != 0 || lpOutBuffer != NULL || nOutBufferSize != 0)
            error = STATUS_INVALID_PARAMETER;
        else error = CDROM_PauseAudio(dev);
        break;
    case IOCTL_CDROM_PLAY_AUDIO_MSF:
        sz = 0;
        if (lpOutBuffer != NULL || nOutBufferSize != 0) error = STATUS_INVALID_PARAMETER;
        else if (nInBufferSize < sizeof(CDROM_PLAY_AUDIO_MSF)) error = STATUS_BUFFER_TOO_SMALL;
        else error = CDROM_PlayAudioMSF(dev, (const CDROM_PLAY_AUDIO_MSF*)lpInBuffer);
        break;
    case IOCTL_CDROM_RESUME_AUDIO:
        sz = 0;
        if (lpInBuffer != NULL || nInBufferSize != 0 || lpOutBuffer != NULL || nOutBufferSize != 0)
            error = STATUS_INVALID_PARAMETER;
        else error = CDROM_ResumeAudio(dev);
        break;
    case IOCTL_CDROM_SEEK_AUDIO_MSF:
        sz = 0;
        if (lpOutBuffer != NULL || nOutBufferSize != 0) error = STATUS_INVALID_PARAMETER;
        else if (nInBufferSize < sizeof(CDROM_SEEK_AUDIO_MSF)) error = STATUS_BUFFER_TOO_SMALL;
        else error = CDROM_SeekAudioMSF(dev, (const CDROM_SEEK_AUDIO_MSF*)lpInBuffer);
        break;
    case IOCTL_CDROM_STOP_AUDIO:
        sz = 0;
        if (lpInBuffer != NULL || nInBufferSize != 0 || lpOutBuffer != NULL || nOutBufferSize != 0)
            error = STATUS_INVALID_PARAMETER;
        else error = CDROM_StopAudio(dev);
        break;
    case IOCTL_CDROM_GET_VOLUME:
        sz = sizeof(VOLUME_CONTROL);
        if (lpInBuffer != NULL || nInBufferSize != 0) error = STATUS_INVALID_PARAMETER;
        else if (nOutBufferSize < sz) error = STATUS_BUFFER_TOO_SMALL;
        else error = CDROM_GetVolume(dev, (VOLUME_CONTROL*)lpOutBuffer);
        break;
    case IOCTL_CDROM_SET_VOLUME:
        sz = 0;
        if (lpInBuffer == NULL || nInBufferSize < sizeof(VOLUME_CONTROL) || lpOutBuffer != NULL)
            error = STATUS_INVALID_PARAMETER;
        else error = CDROM_SetVolume(dev, (const VOLUME_CONTROL*)lpInBuffer);
        break;
    case IOCTL_CDROM_RAW_READ:
        sz = 0;
        if (nInBufferSize < sizeof(RAW_READ_INFO)) error = STATUS_INVALID_PARAMETER;
        else if (lpOutBuffer == NULL) error = STATUS_BUFFER_TOO_SMALL;
        else error = CDROM_RawRead(dev, (const RAW_READ_INFO*)lpInBuffer,
                                   lpOutBuffer, nOutBufferSize, &sz);
        break;
    case IOCTL_SCSI_GET_ADDRESS:
        sz = sizeof(SCSI_ADDRESS);
        if (lpInBuffer != NULL || nInBufferSize != 0) error = STATUS_INVALID_PARAMETER;
        else if (nOutBufferSize < sz) error = STATUS_BUFFER_TOO_SMALL;
        else error = CDROM_GetAddress(dev, (SCSI_ADDRESS*)lpOutBuffer);
        break;
    case IOCTL_SCSI_PASS_THROUGH_DIRECT:
        sz = sizeof(SCSI_PASS_THROUGH_DIRECT);
        if (lpOutBuffer == NULL) error = STATUS_INVALID_PARAMETER;
        else if (nOutBufferSize < sizeof(SCSI_PASS_THROUGH_DIRECT)) error = STATUS_BUFFER_TOO_SMALL;
        else error = CDROM_ScsiPassThroughDirect(dev, (PSCSI_PASS_THROUGH_DIRECT)lpOutBuffer);
        break;
    case IOCTL_SCSI_PASS_THROUGH:
        sz = sizeof(SCSI_PASS_THROUGH);
        if (lpOutBuffer == NULL) error = STATUS_INVALID_PARAMETER;
        else if (nOutBufferSize < sizeof(SCSI_PASS_THROUGH)) error = STATUS_BUFFER_TOO_SMALL;
        else error = CDROM_ScsiPassThrough(dev, (PSCSI_PASS_THROUGH)lpOutBuffer);
        break;

    default:
        FIXME("Unsupported IOCTL %lx\n", dwIoControlCode);
        sz = 0;
        error = STATUS_INVALID_PARAMETER;
        break;
    }

    if (lpBytesReturned) *lpBytesReturned = sz;
    if (error)
    {
        SetLastError(error);
        return FALSE;
    }
    CDROM_Close(clientID, dev);
    return TRUE;
}
