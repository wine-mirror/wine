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
#include <stdarg.h>
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
#ifdef HAVE_SCSI_SCSI_H
# include <scsi/scsi.h>
# undef REASSIGN_BLOCKS  /* avoid conflict with winioctl.h */
#endif
#ifdef HAVE_SCSI_SCSI_IOCTL_H
# include <scsi/scsi_ioctl.h>
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
#ifdef HAVE_SYS_SCSIIO_H
# include <sys/scsiio.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "ntstatus.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"
#include "winioctl.h"
#include "ntddstor.h"
#include "ntddcdrm.h"
#include "ntddscsi.h"
#include "wine/debug.h"

/* Non-Linux systems do not have linux/cdrom.h and the like, and thus
   lack the following constants. */

#ifndef CD_SECS
  #define CD_SECS              60 /* seconds per minute */
#endif
#ifndef CD_FRAMES
  #define CD_FRAMES            75 /* frames per second */
#endif

static const struct iocodexs
{
  DWORD code;
  const char *codex;
} iocodextable[] = {
{IOCTL_CDROM_UNLOAD_DRIVER, "IOCTL_CDROM_UNLOAD_DRIVER"},
{IOCTL_CDROM_READ_TOC, "IOCTL_CDROM_READ_TOC"},
{IOCTL_CDROM_GET_CONTROL, "IOCTL_CDROM_GET_CONTROL"},
{IOCTL_CDROM_PLAY_AUDIO_MSF, "IOCTL_CDROM_PLAY_AUDIO_MSF"},
{IOCTL_CDROM_SEEK_AUDIO_MSF, "IOCTL_CDROM_SEEK_AUDIO_MSF"},
{IOCTL_CDROM_STOP_AUDIO, "IOCTL_CDROM_STOP_AUDIO"},
{IOCTL_CDROM_PAUSE_AUDIO, "IOCTL_CDROM_PAUSE_AUDIO"},
{IOCTL_CDROM_RESUME_AUDIO, "IOCTL_CDROM_RESUME_AUDIO"},
{IOCTL_CDROM_GET_VOLUME, "IOCTL_CDROM_GET_VOLUME"},
{IOCTL_CDROM_SET_VOLUME, "IOCTL_CDROM_SET_VOLUME"},
{IOCTL_CDROM_READ_Q_CHANNEL, "IOCTL_CDROM_READ_Q_CHANNEL"},
{IOCTL_CDROM_GET_LAST_SESSION, "IOCTL_CDROM_GET_LAST_SESSION"},
{IOCTL_CDROM_RAW_READ, "IOCTL_CDROM_RAW_READ"},
{IOCTL_CDROM_DISK_TYPE, "IOCTL_CDROM_DISK_TYPE"},
{IOCTL_CDROM_GET_DRIVE_GEOMETRY, "IOCTL_CDROM_GET_DRIVE_GEOMETRY"},
{IOCTL_CDROM_CHECK_VERIFY, "IOCTL_CDROM_CHECK_VERIFY"},
{IOCTL_CDROM_MEDIA_REMOVAL, "IOCTL_CDROM_MEDIA_REMOVAL"},
{IOCTL_CDROM_EJECT_MEDIA, "IOCTL_CDROM_EJECT_MEDIA"},
{IOCTL_CDROM_LOAD_MEDIA, "IOCTL_CDROM_LOAD_MEDIA"},
{IOCTL_CDROM_RESERVE, "IOCTL_CDROM_RESERVE"},
{IOCTL_CDROM_RELEASE, "IOCTL_CDROM_RELEASE"},
{IOCTL_CDROM_FIND_NEW_DEVICES, "IOCTL_CDROM_FIND_NEW_DEVICES"}
};
static const char *iocodex(DWORD code)
{
   int i;
   static char buffer[25];
   for(i=0; i<sizeof(iocodextable)/sizeof(struct iocodexs); i++)
      if (code==iocodextable[i].code)
	 return iocodextable[i].codex;
   sprintf(buffer, "IOCTL_CODE_%x", (int)code);
   return buffer;
}

WINE_DEFAULT_DEBUG_CHANNEL(cdrom);

#define FRAME_OF_ADDR(a) (((int)(a)[1] * CD_SECS + (a)[2]) * CD_FRAMES + (a)[3])
#define FRAME_OF_MSF(a) (((int)(a).M * CD_SECS + (a).S) * CD_FRAMES + (a).F)
#define FRAME_OF_TOC(toc, idx)  FRAME_OF_ADDR((toc).TrackData[idx - (toc).FirstTrack].Address)
#define MSF_OF_FRAME(m,fr) {int f=(fr); ((UCHAR *)&(m))[2]=f%CD_FRAMES;f/=CD_FRAMES;((UCHAR *)&(m))[1]=f%CD_SECS;((UCHAR *)&(m))[0]=f/CD_SECS;}

static NTSTATUS CDROM_ReadTOC(int, CDROM_TOC*);
static NTSTATUS CDROM_GetStatusCode(int);


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
 * 
 * (WS) We need this to keep track of current position and to safely
 * detect media changes. Besides this should provide a great speed up
 * for toc inquiries.
 */
struct cdrom_cache {
    int fd;
    int count;
    char toc_good; /* if false, will reread TOC from disk */
    CDROM_TOC toc;
    SUB_Q_CURRENT_POSITION CurrentPosition;
    const char *device;
};
static struct cdrom_cache cdrom_cache[26];

/* Proposed media change function: not really needed at this time */
/* This is a 1 or 0 type of function */
#if 0
static int CDROM_MediaChanged(int dev)
{
   int i;

   struct cdrom_tochdr	hdr;
   struct cdrom_tocentry entry;

   if (dev < 0 || dev >= 26)
      return 0;
   if ( ioctl(cdrom_cache[dev].fd, CDROMREADTOCHDR, &hdr) == -1 )
      return 0;

   if ( memcmp(&hdr, &cdrom_cache[dev].hdr, sizeof(struct cdrom_tochdr)) )
      return 1;

   for (i=hdr.cdth_trk0; i<=hdr.cdth_trk1+1; i++)
   {
      if (i == hdr.cdth_trk1 + 1)
      {
	 entry.cdte_track = CDROM_LEADOUT;
      } else {
         entry.cdte_track = i;
      }
      entry.cdte_format = CDROM_MSF;
      if ( ioctl(cdrom_cache[dev].fd, CDROMREADTOCENTRY, &entry) == -1)
	 return 0;
      if ( memcmp(&entry, cdrom_cache[dev].entry+i-hdr.cdth_trk0,
			      sizeof(struct cdrom_tocentry)) )
	 return 1;
   }
   return 0;
}
#endif

/******************************************************************
 *		CDROM_SyncCache                          [internal]
 *
 * Read the TOC in and store it in the cdrom_cache structure.
 * Further requests for the TOC will be copied from the cache
 * unless certain events like disk ejection is detected, in which
 * case the cache will be cleared, causing it to be resynced.
 *
 */
static int CDROM_SyncCache(int dev)
{
   int i, io = 0, tsz;
#ifdef linux
   struct cdrom_tochdr		hdr;
   struct cdrom_tocentry	entry;
#elif defined(__FreeBSD__) || defined(__NetBSD__)
   struct ioc_toc_header	hdr;
   struct ioc_read_toc_entry	entry;
   struct cd_toc_entry         toc_buffer;
#endif
   CDROM_TOC *toc = &cdrom_cache[dev].toc;
   cdrom_cache[dev].toc_good = 0;

#ifdef linux

   io = ioctl(cdrom_cache[dev].fd, CDROMREADTOCHDR, &hdr);
   if (io == -1)
   {
      WARN("(%d) -- Error occurred (%s)!\n", dev, strerror(errno));
      goto end;
   }
   
   TRACE("caching toc from=%d to=%d\n", toc->FirstTrack, toc->LastTrack );

   toc->FirstTrack = hdr.cdth_trk0;
   toc->LastTrack  = hdr.cdth_trk1;
   tsz = sizeof(toc->FirstTrack) + sizeof(toc->LastTrack)
       + sizeof(TRACK_DATA) * (toc->LastTrack-toc->FirstTrack+2);
   toc->Length[0] = tsz >> 8;
   toc->Length[1] = tsz;

   for (i = toc->FirstTrack; i <= toc->LastTrack + 1; i++)
   {
     if (i == toc->LastTrack + 1)
       entry.cdte_track = CDROM_LEADOUT;
     else 
       entry.cdte_track = i;
     entry.cdte_format = CDROM_MSF;
     io = ioctl(cdrom_cache[dev].fd, CDROMREADTOCENTRY, &entry);
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
    cdrom_cache[dev].toc_good = 1;
    io = 0;
#elif defined(__FreeBSD__) || defined(__NetBSD__)

    io = ioctl(cdrom_cache[dev].fd, CDIOREADTOCHEADER, &hdr);
    if (io == -1)
    {
        WARN("(%d) -- Error occurred (%s)!\n", dev, strerror(errno));
        goto end;
    }
    toc->FirstTrack = hdr.starting_track;
    toc->LastTrack  = hdr.ending_track;
    tsz = sizeof(toc->FirstTrack) + sizeof(toc->LastTrack)
        + sizeof(TRACK_DATA) * (toc->LastTrack-toc->FirstTrack+2);
    toc->Length[0] = tsz >> 8;
    toc->Length[1] = tsz;

    TRACE("caching toc from=%d to=%d\n", toc->FirstTrack, toc->LastTrack );

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
	io = ioctl(cdrom_cache[dev].fd, CDIOREADTOCENTRYS, &entry);
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
    cdrom_cache[dev].toc_good = 1;
    io = 0;
#else
    return STATUS_NOT_SUPPORTED;
#endif
end:
    return CDROM_GetStatusCode(io);
}

static void CDROM_ClearCacheEntry(int dev)
{
    cdrom_cache[dev].toc_good = 0;
}



/******************************************************************
 *		CDROM_GetInterfaceInfo
 *
 * Determines the ide interface (the number after the ide), and the
 * number of the device on that interface for ide cdroms (*port == 0).
 * Determines the scsi information for scsi cdroms (*port >= 1).
 * Returns false if the info cannot not be obtained.
 *
 * NOTE: this function is used in CDROM_InitRegistry and CDROM_GetAddress
 */
static int CDROM_GetInterfaceInfo(int fd, int* port, int* iface, int* device,int* lun)
{
#if defined(linux)
    {
        struct stat st;
        if ( fstat(fd, &st) == -1 || ! S_ISBLK(st.st_mode)) {
            FIXME("cdrom not a block device!!!\n");
            return 0;
        }
        *port = 0;
        *iface = 0;
        *device = 0;
        *lun = 0;
        switch (major(st.st_rdev)) {
            case IDE0_MAJOR: *iface = 0; break;
            case IDE1_MAJOR: *iface = 1; break;
            case IDE2_MAJOR: *iface = 2; break;
            case IDE3_MAJOR: *iface = 3; break;
            case IDE4_MAJOR: *iface = 4; break;
            case IDE5_MAJOR: *iface = 5; break;
            case IDE6_MAJOR: *iface = 6; break;
            case IDE7_MAJOR: *iface = 7; break;
            default: *port = 1; break;
        }

        if (*port == 0)
                *device = (minor(st.st_rdev) >> 6);
        else
        {
#ifdef SCSI_IOCTL_GET_IDLUN
                UINT32 idlun[2];
                if (ioctl(fd, SCSI_IOCTL_GET_IDLUN, &idlun) != -1)
                {
                        *port = ((idlun[0] >> 24) & 0xff) + 1;
                        *iface = (idlun[0] >> 16) & 0xff;
                        *device = idlun[0] & 0xff;
                        *lun = (idlun[0] >> 8) & 0xff;
                }
                else
#endif
                {
                        FIXME("CD-ROM device (%d, %d) not supported\n",
                              major(st.st_rdev), minor(st.st_rdev));
                        return 0;
                }
        }
        return 1;
    }
#elif defined(__NetBSD__)
    {
       struct scsi_addr addr;
       if (ioctl(fd, SCIOCIDENTIFY, &addr) != -1) {
            switch (addr.type) {
                case TYPE_SCSI:  *port = 1;
                                 *iface = addr.addr.scsi.scbus;
                                 *device = addr.addr.scsi.target;
                                 *lun = addr.addr.scsi.lun;
                                 break;
                case TYPE_ATAPI: *port = 0;
                                 *iface = addr.addr.atapi.atbus;
                                 *device = addr.addr.atapi.drive;
                                 *lun = 0;
                                 break;
            }
            return 1;
       }
       return 0;
    }
#elif defined(__FreeBSD__)
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
void CDROM_InitRegistry(int fd, int device_id, const char *device )
{
    int portnum, busid, targetid, lun;
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

    cdrom_cache[device_id].device = device;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    if ( ! CDROM_GetInterfaceInfo(fd, &portnum, &busid, &targetid, &lun))
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

    snprintf(buffer,sizeof(buffer),"Scsi Port %d",portnum);
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
    {
        int dma;
        if (ioctl(fd,HDIO_GET_DMA, &dma) != -1) {
            value = dma;
            TRACE("setting dma to %lx\n", value);
        }
    }
#endif
    RtlCreateUnicodeStringFromAsciiz( &nameW, "DMAEnabled" );
    NtSetValueKey( portKey,&nameW, 0, REG_DWORD, (BYTE *)&value, sizeof(DWORD));
    RtlFreeUnicodeString( &nameW );

    snprintf(buffer,40,"Scsi Bus %d", busid);
    attr.RootDirectory = portKey;
    if (!RtlCreateUnicodeStringFromAsciiz( &nameW, buffer ) ||
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
 */
static NTSTATUS CDROM_Open(HANDLE hDevice, DWORD clientID, int* dev)
{
    *dev = LOWORD(clientID);

    if (*dev >= 26) return STATUS_NO_SUCH_DEVICE;

    if (!cdrom_cache[*dev].count)
    {
        const char *device;

        if (!(device = cdrom_cache[*dev].device)) return STATUS_NO_SUCH_DEVICE;
        cdrom_cache[*dev].fd = open(device, O_RDONLY|O_NONBLOCK);
        if (cdrom_cache[*dev].fd == -1)
        {
            FIXME("Can't open configured CD-ROM drive %c: (device %s): %s\n",
                  'A' + *dev, device, strerror(errno));
            return STATUS_NO_SUCH_DEVICE;
        }
    }
    cdrom_cache[*dev].count++;
    TRACE("%d, %d, %d\n", *dev, cdrom_cache[*dev].fd, cdrom_cache[*dev].count);
    return STATUS_SUCCESS;
}

/******************************************************************
 *		CDROM_Close
 *
 *
 */
static void CDROM_Close(DWORD clientID)
{
    int dev = LOWORD(clientID);

    if (dev >= 26 /*|| fd != cdrom_cache[dev].fd*/) FIXME("how come\n");
    if (--cdrom_cache[dev].count == 0) 
    {
        close(cdrom_cache[dev].fd);
        cdrom_cache[dev].fd = -1;
    }
}

/******************************************************************
 *		CDROM_GetStatusCode
 *
 *
 */
static NTSTATUS CDROM_GetStatusCode(int io)
{
    if (io == 0) return STATUS_SUCCESS;
    switch (errno)
    {
    case EIO:
#ifdef ENOMEDIUM
    case ENOMEDIUM:
#endif
        return STATUS_NO_MEDIA_IN_DEVICE;
    case EPERM:
        return STATUS_ACCESS_DENIED;
    case EINVAL:
        return STATUS_INVALID_PARAMETER;
    /* case EBADF: Bad file descriptor */
    case EOPNOTSUPP:
        return STATUS_NOT_SUPPORTED;
    }
    FIXME("Unmapped error code %d: %s\n", errno, strerror(errno));
    return STATUS_IO_DEVICE_ERROR;
}

/******************************************************************
 *		CDROM_GetControl
 *
 */
static NTSTATUS CDROM_GetControl(int dev, CDROM_AUDIO_CONTROL* cac)
{
    cac->LbaFormat = 0; /* FIXME */
    cac->LogicalBlocksPerSecond = 1; /* FIXME */
    return  STATUS_NOT_SUPPORTED;
}

/******************************************************************
 *		CDROM_GetDeviceNumber
 *
 */
static NTSTATUS CDROM_GetDeviceNumber(int dev, STORAGE_DEVICE_NUMBER* devnum)
{
    return STATUS_NOT_SUPPORTED;
}

/******************************************************************
 *		CDROM_GetDriveGeometry
 *
 */
static NTSTATUS CDROM_GetDriveGeometry(int dev, DISK_GEOMETRY* dg)
{
  CDROM_TOC     toc;
  NTSTATUS      ret = 0;
  int           fsize = 0;

  if ((ret = CDROM_ReadTOC(dev, &toc)) != 0) return ret;

  fsize = FRAME_OF_TOC(toc, toc.LastTrack+1)
        - FRAME_OF_TOC(toc, 1); /* Total size in frames */
  
  dg->Cylinders.s.LowPart = fsize / (64 * 32); 
  dg->Cylinders.s.HighPart = 0; 
  dg->MediaType = RemovableMedia;  
  dg->TracksPerCylinder = 64; 
  dg->SectorsPerTrack = 32;  
  dg->BytesPerSector= 2048; 
  return ret;
}

/**************************************************************************
 *                              CDROM_Reset                     [internal]
 */
static NTSTATUS CDROM_ResetAudio(int dev)
{
#if defined(linux)
    return CDROM_GetStatusCode(ioctl(cdrom_cache[dev].fd, CDROMRESET));
#elif defined(__FreeBSD__) || defined(__NetBSD__)
    return CDROM_GetStatusCode(ioctl(cdrom_cache[dev].fd, CDIOCRESET, NULL));
#else
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *		CDROM_SetTray
 *
 *
 */
static NTSTATUS CDROM_SetTray(int dev, BOOL doEject)
{
#if defined(linux)
    return CDROM_GetStatusCode(ioctl(cdrom_cache[dev].fd, doEject ? CDROMEJECT : CDROMCLOSETRAY));
#elif defined(__FreeBSD__) || defined(__NetBSD__)
    return CDROM_GetStatusCode((ioctl(cdrom_cache[dev].fd, CDIOCALLOW, NULL)) ||
                               (ioctl(cdrom_cache[dev].fd, doEject ? CDIOCEJECT : CDIOCCLOSE, NULL)) ||
                               (ioctl(cdrom_cache[dev].fd, CDIOCPREVENT, NULL)));
#else
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *		CDROM_ControlEjection
 *
 *
 */
static NTSTATUS CDROM_ControlEjection(int dev, const PREVENT_MEDIA_REMOVAL* rmv)
{
#if defined(linux)
    return CDROM_GetStatusCode(ioctl(cdrom_cache[dev].fd, CDROM_LOCKDOOR, rmv->PreventMediaRemoval));
#elif defined(__FreeBSD__) || defined(__NetBSD__)
    return CDROM_GetStatusCode(ioctl(cdrom_cache[dev].fd, (rmv->PreventMediaRemoval) ? CDIOCPREVENT : CDIOCALLOW, NULL));
#else
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *		CDROM_ReadTOC
 *
 *
 */
static NTSTATUS CDROM_ReadTOC(int dev, CDROM_TOC* toc)
{
    NTSTATUS       ret = STATUS_NOT_SUPPORTED;

    if (dev < 0 || dev >= 26)
       return STATUS_INVALID_PARAMETER;
    if ( !cdrom_cache[dev].toc_good ) {
       ret = CDROM_SyncCache(dev);
       if ( ret )
	  return ret;
    }
    *toc = cdrom_cache[dev].toc;
    return STATUS_SUCCESS;
}

/******************************************************************
 *		CDROM_GetDiskData
 *
 *
 */
static NTSTATUS CDROM_GetDiskData(int dev, CDROM_DISK_DATA* data)
{
    CDROM_TOC   toc;
    NTSTATUS    ret;
    int         i;

    if ((ret = CDROM_ReadTOC(dev, &toc)) != 0) return ret;
    data->DiskData = 0;
    for (i = toc.FirstTrack; i <= toc.LastTrack; i++) {
        if (toc.TrackData[i-toc.FirstTrack].Control & 0x04)
            data->DiskData |= CDROM_DISK_DATA_TRACK;
        else
            data->DiskData |= CDROM_DISK_AUDIO_TRACK;
    }
    return STATUS_SUCCESS;
}

/******************************************************************
 *		CDROM_ReadQChannel
 *
 *
 */
static NTSTATUS CDROM_ReadQChannel(int dev, const CDROM_SUB_Q_DATA_FORMAT* fmt,
                                   SUB_Q_CHANNEL_DATA* data)
{
    NTSTATUS            ret = STATUS_NOT_SUPPORTED;
#ifdef linux
    unsigned            size;
    SUB_Q_HEADER*       hdr = (SUB_Q_HEADER*)data;
    int                 io;
    struct cdrom_subchnl	sc;
    sc.cdsc_format = CDROM_MSF;

    io = ioctl(cdrom_cache[dev].fd, CDROMSUBCHNL, &sc);
    if (io == -1)
    {
	TRACE("opened or no_media (%s)!\n", strerror(errno));
	hdr->AudioStatus = AUDIO_STATUS_NO_STATUS;
	CDROM_ClearCacheEntry(dev);
	goto end;
    }

    hdr->AudioStatus = AUDIO_STATUS_NOT_SUPPORTED;

    switch (sc.cdsc_audiostatus) {
    case CDROM_AUDIO_INVALID:
	CDROM_ClearCacheEntry(dev);
	hdr->AudioStatus = AUDIO_STATUS_NOT_SUPPORTED;
	break;
    case CDROM_AUDIO_NO_STATUS:
	CDROM_ClearCacheEntry(dev);
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
	if (hdr->AudioStatus==AUDIO_STATUS_IN_PROGRESS) {
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

	  cdrom_cache[dev].CurrentPosition = data->CurrentPosition;
	}
	else /* not playing */
	{
	  cdrom_cache[dev].CurrentPosition.Header = *hdr; /* Preserve header info */
	  data->CurrentPosition = cdrom_cache[dev].CurrentPosition;
	}
        break;
    case IOCTL_CDROM_MEDIA_CATALOG:
        size = sizeof(SUB_Q_MEDIA_CATALOG_NUMBER);
        data->MediaCatalog.FormatCode = IOCTL_CDROM_MEDIA_CATALOG;
        {
            struct cdrom_mcn mcn;
            if ((io = ioctl(cdrom_cache[dev].fd, CDROM_GET_MCN, &mcn)) == -1) goto end;

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
    io = ioctl(cdrom_cache[dev].fd, CDIOCREADSUBCHANNEL, &read_sc);
    if (io == -1)
    {
	TRACE("opened or no_media (%s)!\n", strerror(errno));
	CDROM_ClearCacheEntry(dev);
	hdr->AudioStatus = AUDIO_STATUS_NO_STATUS;
	goto end;
    }

    hdr->AudioStatus = AUDIO_STATUS_NOT_SUPPORTED;

    switch (sc.header.audio_status) {
    case CD_AS_AUDIO_INVALID:
	CDROM_ClearCacheEntry(dev);
	hdr->AudioStatus = AUDIO_STATUS_NOT_SUPPORTED;
	break;
    case CD_AS_NO_STATUS:
	CDROM_ClearCacheEntry(dev);
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
	if (hdr->AudioStatus==AUDIO_STATUS_IN_PROGRESS) {
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
	  cdrom_cache[dev].CurrentPosition = data->CurrentPosition;
	}
	else { /* not playing */
	  cdrom_cache[dev].CurrentPosition.Header = *hdr; /* Preserve header info */
	  data->CurrentPosition = cdrom_cache[dev].CurrentPosition;
	}
        break;
    case IOCTL_CDROM_MEDIA_CATALOG:
        size = sizeof(SUB_Q_MEDIA_CATALOG_NUMBER);
        data->MediaCatalog.FormatCode = IOCTL_CDROM_MEDIA_CATALOG;
        data->MediaCatalog.Mcval = sc.what.media_catalog.mc_valid;
        memcpy(data->MediaCatalog.MediaCatalog, sc.what.media_catalog.mc_number, 15);
        break;
    case IOCTL_CDROM_TRACK_ISRC:
        size = sizeof(SUB_Q_CURRENT_POSITION);
        data->TrackIsrc.FormatCode = IOCTL_CDROM_TRACK_ISRC;
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
static NTSTATUS CDROM_Verify(int dev)
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
static NTSTATUS CDROM_PlayAudioMSF(int dev, const CDROM_PLAY_AUDIO_MSF* audio_msf)
{
    NTSTATUS       ret = STATUS_NOT_SUPPORTED;
#ifdef linux
    struct 	cdrom_msf	msf;
    int         io;

    msf.cdmsf_min0   = audio_msf->StartingM;
    msf.cdmsf_sec0   = audio_msf->StartingS;
    msf.cdmsf_frame0 = audio_msf->StartingF;
    msf.cdmsf_min1   = audio_msf->EndingM;
    msf.cdmsf_sec1   = audio_msf->EndingS;
    msf.cdmsf_frame1 = audio_msf->EndingF;

    io = ioctl(cdrom_cache[dev].fd, CDROMSTART);
    if (io == -1)
    {
	WARN("motor doesn't start !\n");
	goto end;
    }
    io = ioctl(cdrom_cache[dev].fd, CDROMPLAYMSF, &msf);
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

    io = ioctl(cdrom_cache[dev].fd, CDIOCSTART, NULL);
    if (io == -1)
    {
	WARN("motor doesn't start !\n");
	goto end;
    }
    io = ioctl(cdrom_cache[dev].fd, CDIOCPLAYMSF, &msf);
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
static NTSTATUS CDROM_SeekAudioMSF(int dev, const CDROM_SEEK_AUDIO_MSF* audio_msf)
{
    CDROM_TOC toc;
    int i, io, frame;
    SUB_Q_CURRENT_POSITION *cp;
#if defined(linux)
    struct cdrom_msf0	msf;
    struct cdrom_subchnl sc;
#elif defined(__FreeBSD__) || defined(__NetBSD__)
    struct ioc_play_msf	msf;
    struct ioc_read_subchannel	read_sc;
    struct cd_sub_channel_info	sc;
    int final_frame;
#endif

    /* Use the information on the TOC to compute the new current
     * position, which is shadowed on the cache. [Portable]. */
    frame = FRAME_OF_MSF(*audio_msf);
    cp = &cdrom_cache[dev].CurrentPosition;
    if ((io = CDROM_ReadTOC(dev, &toc)) != 0) return io;
     
    for(i=toc.FirstTrack;i<=toc.LastTrack+1;i++)
      if (FRAME_OF_TOC(toc,i)>frame) break;
    if (i <= toc.FirstTrack || i > toc.LastTrack+1)
      return STATUS_INVALID_PARAMETER;
    i--;
    cp->FormatCode = IOCTL_CDROM_CURRENT_POSITION; 
    cp->Control = toc.TrackData[i-toc.FirstTrack].Control; 
    cp->ADR = toc.TrackData[i-toc.FirstTrack].Adr; 
    cp->TrackNumber = toc.TrackData[i-toc.FirstTrack].TrackNumber;
    cp->IndexNumber = 0; /* FIXME: where do they keep these? */
    cp->AbsoluteAddress[0] = 0; 
    cp->AbsoluteAddress[1] = toc.TrackData[i-toc.FirstTrack].Address[1];
    cp->AbsoluteAddress[2] = toc.TrackData[i-toc.FirstTrack].Address[2];
    cp->AbsoluteAddress[3] = toc.TrackData[i-toc.FirstTrack].Address[3];
    frame -= FRAME_OF_TOC(toc,i);
    cp->TrackRelativeAddress[0] = 0;
    MSF_OF_FRAME(cp->TrackRelativeAddress[1], frame); 

    /* If playing, then issue a seek command, otherwise do nothing */
#ifdef linux
    sc.cdsc_format = CDROM_MSF;

    io = ioctl(cdrom_cache[dev].fd, CDROMSUBCHNL, &sc);
    if (io == -1)
    {
	TRACE("opened or no_media (%s)!\n", strerror(errno));
	CDROM_ClearCacheEntry(dev);
        return CDROM_GetStatusCode(io);
    }
    if (sc.cdsc_audiostatus==CDROM_AUDIO_PLAY)
    {
      msf.minute = audio_msf->M;
      msf.second = audio_msf->S;
      msf.frame  = audio_msf->F;
      return CDROM_GetStatusCode(ioctl(cdrom_cache[dev].fd, CDROMSEEK, &msf));
    }
    return STATUS_SUCCESS;
#elif defined(__FreeBSD__) || defined(__NetBSD__)
    read_sc.address_format = CD_MSF_FORMAT;
    read_sc.track          = 0;
    read_sc.data_len       = sizeof(sc);
    read_sc.data           = &sc;
    read_sc.data_format    = CD_CURRENT_POSITION;

    io = ioctl(cdrom_cache[dev].fd, CDIOCREADSUBCHANNEL, &read_sc);
    if (io == -1)
    {
	TRACE("opened or no_media (%s)!\n", strerror(errno));
	CDROM_ClearCacheEntry(dev);
        return CDROM_GetStatusCode(io);
    }
    if (sc.header.audio_status==CD_AS_PLAY_IN_PROGRESS) 
    {

      msf.start_m      = audio_msf->M;
      msf.start_s      = audio_msf->S;
      msf.start_f      = audio_msf->F;
      final_frame = FRAME_OF_TOC(toc,toc.LastTrack+1)-1;
      MSF_OF_FRAME(msf.end_m, final_frame);

      return CDROM_GetStatusCode(ioctl(cdrom_cache[dev].fd, CDIOCPLAYMSF, &msf));
    }
    return STATUS_SUCCESS;
#else
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *		CDROM_PauseAudio
 *
 *
 */
static NTSTATUS CDROM_PauseAudio(int dev)
{
#if defined(linux)
    return CDROM_GetStatusCode(ioctl(cdrom_cache[dev].fd, CDROMPAUSE));
#elif defined(__FreeBSD__) || defined(__NetBSD__)
    return CDROM_GetStatusCode(ioctl(cdrom_cache[dev].fd, CDIOCPAUSE, NULL));
#else
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *		CDROM_ResumeAudio
 *
 *
 */
static NTSTATUS CDROM_ResumeAudio(int dev)
{
#if defined(linux)
    return CDROM_GetStatusCode(ioctl(cdrom_cache[dev].fd, CDROMRESUME));
#elif defined(__FreeBSD__) || defined(__NetBSD__)
    return CDROM_GetStatusCode(ioctl(cdrom_cache[dev].fd, CDIOCRESUME, NULL));
#else
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *		CDROM_StopAudio
 *
 *
 */
static NTSTATUS CDROM_StopAudio(int dev)
{
#if defined(linux)
    return CDROM_GetStatusCode(ioctl(cdrom_cache[dev].fd, CDROMSTOP));
#elif defined(__FreeBSD__) || defined(__NetBSD__)
    return CDROM_GetStatusCode(ioctl(cdrom_cache[dev].fd, CDIOCSTOP, NULL));
#else
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *		CDROM_GetVolume
 *
 *
 */
static NTSTATUS CDROM_GetVolume(int dev, VOLUME_CONTROL* vc)
{
#if defined(linux)
    struct cdrom_volctrl volc;
    int io;

    io = ioctl(cdrom_cache[dev].fd, CDROMVOLREAD, &volc);
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

    io = ioctl(cdrom_cache[dev].fd, CDIOCGETVOL, &volc);
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
static NTSTATUS CDROM_SetVolume(int dev, const VOLUME_CONTROL* vc)
{
#if defined(linux)
    struct cdrom_volctrl volc;

    volc.channel0 = vc->PortVolume[0];
    volc.channel1 = vc->PortVolume[1];
    volc.channel2 = vc->PortVolume[2];
    volc.channel3 = vc->PortVolume[3];

    return CDROM_GetStatusCode(ioctl(cdrom_cache[dev].fd, CDROMVOLCTRL, &volc));
#elif defined(__FreeBSD__) || defined(__NetBSD__)
    struct  ioc_vol     volc;

    volc.vol[0] = vc->PortVolume[0];
    volc.vol[1] = vc->PortVolume[1];
    volc.vol[2] = vc->PortVolume[2];
    volc.vol[3] = vc->PortVolume[3];

    return CDROM_GetStatusCode(ioctl(cdrom_cache[dev].fd, CDIOCSETVOL, &volc));
#else
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *		CDROM_RawRead
 *
 *
 */
static NTSTATUS CDROM_RawRead(int dev, const RAW_READ_INFO* raw, void* buffer, DWORD len, DWORD* sz)
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
            io = ioctl(cdrom_cache[dev].fd, CDROMREADMODE2, &cdr);
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
            io = ioctl(cdrom_cache[dev].fd, CDROMREADAUDIO, &cdra);
            break;
        default:
            FIXME("NIY: %d\n", raw->TrackMode);
            return ret;
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
            io = ioctl(cdrom_cache[dev].fd, CDIOCREADAUDIO, &ira);
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
static NTSTATUS CDROM_ScsiPassThroughDirect(int dev, PSCSI_PASS_THROUGH_DIRECT pPacket)
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

    io = ioctl(cdrom_cache[dev].fd, CDROM_SEND_PACKET, &cmd);

    if (pPacket->SenseInfoLength != 0)
    {
        memcpy((char*)pPacket + pPacket->SenseInfoOffset,
	       &sense, pPacket->SenseInfoLength);
    }

    pPacket->ScsiStatus = cmd.stat;

    ret = CDROM_GetStatusCode(io);

#elif defined(__NetBSD__)
    scsireq_t cmd;
    int io;

    if (pPacket->Length < sizeof(SCSI_PASS_THROUGH_DIRECT))
	return STATUS_BUFFER_TOO_SMALL;

    if (pPacket->CdbLength > 12)
        return STATUS_INVALID_PARAMETER;

    if (pPacket->SenseInfoLength > SENSEBUFLEN)
        return STATUS_INVALID_PARAMETER;

    memset(&cmd, 0, sizeof(cmd));
    memcpy(&(cmd.cmd), &(pPacket->Cdb), pPacket->CdbLength);

    cmd.cmdlen         = pPacket->CdbLength;
    cmd.databuf        = pPacket->DataBuffer;
    cmd.datalen        = pPacket->DataTransferLength;
    cmd.senselen       = pPacket->SenseInfoLength;
    cmd.timeout        = pPacket->TimeOutValue*1000; /* in milliseconds */

    switch (pPacket->DataIn)
    {
    case SCSI_IOCTL_DATA_OUT:
        cmd.flags |= SCCMD_WRITE;
	break;
    case SCSI_IOCTL_DATA_IN:
        cmd.flags |= SCCMD_READ;
	break;
    case SCSI_IOCTL_DATA_UNSPECIFIED:
        cmd.flags = 0;
	break;
    default:
       return STATUS_INVALID_PARAMETER;
    }

    io = ioctl(cdrom_cache[dev].fd, SCIOCCOMMAND, &cmd);

    switch (cmd.retsts)
    {
    case SCCMD_OK:         break;
    case SCCMD_TIMEOUT:    return STATUS_TIMEOUT;
                           break;
    case SCCMD_BUSY:       return STATUS_DEVICE_BUSY;
                           break;
    case SCCMD_SENSE:      break;
    case SCCMD_UNKNOWN:    return STATUS_UNSUCCESSFUL;
                           break;
    }

    if (pPacket->SenseInfoLength != 0)
    {
        memcpy((char*)pPacket + pPacket->SenseInfoOffset,
	       cmd.sense, pPacket->SenseInfoLength);
    }

    pPacket->ScsiStatus = cmd.status;

    ret = CDROM_GetStatusCode(io);
#endif
    return ret;
}

/******************************************************************
 *		CDROM_ScsiPassThrough
 *
 *
 */
static NTSTATUS CDROM_ScsiPassThrough(int dev, PSCSI_PASS_THROUGH pPacket)
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
        cmd.buffer     = (char*)pPacket + pPacket->DataBufferOffset;
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

    io = ioctl(cdrom_cache[dev].fd, CDROM_SEND_PACKET, &cmd);

    if (pPacket->SenseInfoLength != 0)
    {
        memcpy((char*)pPacket + pPacket->SenseInfoOffset,
	       &sense, pPacket->SenseInfoLength);
    }

    pPacket->ScsiStatus = cmd.stat;

    ret = CDROM_GetStatusCode(io);

#elif defined(__NetBSD__)
    scsireq_t cmd;
    int io;

    if (pPacket->Length < sizeof(SCSI_PASS_THROUGH))
	return STATUS_BUFFER_TOO_SMALL;

    if (pPacket->CdbLength > 12)
        return STATUS_INVALID_PARAMETER;

    if (pPacket->SenseInfoLength > SENSEBUFLEN)
        return STATUS_INVALID_PARAMETER;

    memset(&cmd, 0, sizeof(cmd));
    memcpy(&(cmd.cmd), &(pPacket->Cdb), pPacket->CdbLength);

    if ( pPacket->DataBufferOffset > 0x1000 )
    {
        cmd.databuf     = (void*)pPacket->DataBufferOffset;
    }
    else
    {
        cmd.databuf     = (char*)pPacket + pPacket->DataBufferOffset;
    }

    cmd.cmdlen         = pPacket->CdbLength;
    cmd.datalen        = pPacket->DataTransferLength;
    cmd.senselen       = pPacket->SenseInfoLength;
    cmd.timeout        = pPacket->TimeOutValue*1000; /* in milliseconds */

    switch (pPacket->DataIn)
    {
    case SCSI_IOCTL_DATA_OUT:
        cmd.flags |= SCCMD_WRITE;
	break;
    case SCSI_IOCTL_DATA_IN:
        cmd.flags |= SCCMD_READ;
	break;
    case SCSI_IOCTL_DATA_UNSPECIFIED:
        cmd.flags = 0;
	break;
    default:
       return STATUS_INVALID_PARAMETER;
    }

    io = ioctl(cdrom_cache[dev].fd, SCIOCCOMMAND, &cmd);

    switch (cmd.retsts)
    {
    case SCCMD_OK:         break;
    case SCCMD_TIMEOUT:    return STATUS_TIMEOUT;
                           break;
    case SCCMD_BUSY:       return STATUS_DEVICE_BUSY;
                           break;
    case SCCMD_SENSE:      break;
    case SCCMD_UNKNOWN:    return STATUS_UNSUCCESSFUL;
                           break;
    }

    if (pPacket->SenseInfoLength != 0)
    {
        memcpy((char*)pPacket + pPacket->SenseInfoOffset,
	       cmd.sense, pPacket->SenseInfoLength);
    }

    pPacket->ScsiStatus = cmd.status;

    ret = CDROM_GetStatusCode(io);
#endif
    return ret;
}

/******************************************************************
 *		CDROM_ScsiGetCaps
 *
 *
 */
static NTSTATUS CDROM_ScsiGetCaps(int dev, PIO_SCSI_CAPABILITIES caps)
{
    NTSTATUS    ret = STATUS_NOT_IMPLEMENTED;

    caps->Length = sizeof(*caps);
#if defined(linux)
    caps->MaximumTransferLength = SG_SCATTER_SZ; /* FIXME */
    caps->MaximumPhysicalPages = SG_SCATTER_SZ / getpagesize();
    caps->SupportedAsynchronousEvents = TRUE;
    caps->AlignmentMask = getpagesize();
    caps->TaggedQueuing = FALSE; /* we could check that it works and answer TRUE */
    caps->AdapterScansDown = FALSE; /* FIXME ? */
    caps->AdapterUsesPio = FALSE; /* FIXME ? */
    ret = STATUS_SUCCESS;
#else
    FIXME("Unimplemented\n");
#endif
    return ret;
}

/******************************************************************
 *		CDROM_GetAddress
 *
 * implements IOCTL_SCSI_GET_ADDRESS
 */
static NTSTATUS CDROM_GetAddress(int dev, SCSI_ADDRESS* address)
{
    int portnum, busid, targetid, lun;

    address->Length = sizeof(SCSI_ADDRESS);
    if ( ! CDROM_GetInterfaceInfo(cdrom_cache[dev].fd, &portnum,
                                  &busid, &targetid, &lun))
        return STATUS_NOT_SUPPORTED;

    address->PortNumber = portnum;
    address->PathId = busid; /* bus number */
    address->TargetId = targetid;
    address->Lun = lun;
    return STATUS_SUCCESS;
}

/******************************************************************
 *		CDROM_DeviceIoControl
 *
 *
 */
NTSTATUS CDROM_DeviceIoControl(DWORD clientID, HANDLE hDevice, 
                               HANDLE hEvent, PIO_APC_ROUTINE UserApcRoutine,
                               PVOID UserApcContext, 
                               PIO_STATUS_BLOCK piosb, 
                               ULONG dwIoControlCode,
                               LPVOID lpInBuffer, DWORD nInBufferSize,
                               LPVOID lpOutBuffer, DWORD nOutBufferSize)
{
    DWORD       sz;
    NTSTATUS    status = STATUS_SUCCESS;
    int         dev;

    TRACE("%lx[%c] %s %lx %ld %lx %ld %p\n",
          (DWORD)hDevice, 'A' + LOWORD(clientID), iocodex(dwIoControlCode), (DWORD)lpInBuffer, nInBufferSize,
          (DWORD)lpOutBuffer, nOutBufferSize, piosb);

    piosb->Information = 0;

    if ((status = CDROM_Open(hDevice, clientID, &dev))) goto error;

    switch (dwIoControlCode)
    {
    case IOCTL_STORAGE_CHECK_VERIFY:
    case IOCTL_CDROM_CHECK_VERIFY:
        sz = 0;
	CDROM_ClearCacheEntry(dev);
        if (lpInBuffer != NULL || nInBufferSize != 0 || lpOutBuffer != NULL || nOutBufferSize != 0)
            status = STATUS_INVALID_PARAMETER;
        else status = CDROM_Verify(dev);
        break;

/* EPP     case IOCTL_STORAGE_CHECK_VERIFY2: */

/* EPP     case IOCTL_STORAGE_FIND_NEW_DEVICES: */
/* EPP     case IOCTL_CDROM_FIND_NEW_DEVICES: */

    case IOCTL_STORAGE_LOAD_MEDIA:
    case IOCTL_CDROM_LOAD_MEDIA:
        sz = 0;
	CDROM_ClearCacheEntry(dev);
        if (lpInBuffer != NULL || nInBufferSize != 0 || lpOutBuffer != NULL || nOutBufferSize != 0)
            status = STATUS_INVALID_PARAMETER;
        else status = CDROM_SetTray(dev, FALSE);
        break;
     case IOCTL_STORAGE_EJECT_MEDIA:
        sz = 0;
	CDROM_ClearCacheEntry(dev);
        if (lpInBuffer != NULL || nInBufferSize != 0 || lpOutBuffer != NULL || nOutBufferSize != 0)
            status = STATUS_INVALID_PARAMETER;
        else status = CDROM_SetTray(dev, TRUE);
        break;

    case IOCTL_CDROM_MEDIA_REMOVAL:
    case IOCTL_DISK_MEDIA_REMOVAL:
    case IOCTL_STORAGE_MEDIA_REMOVAL:
    case IOCTL_STORAGE_EJECTION_CONTROL:
        /* FIXME the last ioctl:s is not the same as the two others...
         * lockcount/owner should be handled */
        sz = 0;
	CDROM_ClearCacheEntry(dev);
        if (lpOutBuffer != NULL || nOutBufferSize != 0) status = STATUS_INVALID_PARAMETER;
        else if (nInBufferSize < sizeof(PREVENT_MEDIA_REMOVAL)) status = STATUS_BUFFER_TOO_SMALL;
        else status = CDROM_ControlEjection(dev, (const PREVENT_MEDIA_REMOVAL*)lpInBuffer);
        break;

/* EPP     case IOCTL_STORAGE_GET_MEDIA_TYPES: */

    case IOCTL_STORAGE_GET_DEVICE_NUMBER:
        sz = sizeof(STORAGE_DEVICE_NUMBER);
        if (lpInBuffer != NULL || nInBufferSize != 0) status = STATUS_INVALID_PARAMETER;
        else if (nOutBufferSize < sz) status = STATUS_BUFFER_TOO_SMALL;
        else status = CDROM_GetDeviceNumber(dev, (STORAGE_DEVICE_NUMBER*)lpOutBuffer);
        break;

    case IOCTL_STORAGE_RESET_DEVICE:
        sz = 0;
	CDROM_ClearCacheEntry(dev);
        if (lpInBuffer != NULL || nInBufferSize != 0 || lpOutBuffer != NULL || nOutBufferSize != 0)
            status = STATUS_INVALID_PARAMETER;
        else status = CDROM_ResetAudio(dev);
        break;

    case IOCTL_CDROM_GET_CONTROL:
        sz = sizeof(CDROM_AUDIO_CONTROL);
        if (lpInBuffer != NULL || nInBufferSize != 0) status = STATUS_INVALID_PARAMETER;
        else if (nOutBufferSize < sz) status = STATUS_BUFFER_TOO_SMALL;
        else status = CDROM_GetControl(dev, (CDROM_AUDIO_CONTROL*)lpOutBuffer);
        break;

    case IOCTL_CDROM_GET_DRIVE_GEOMETRY:
        sz = sizeof(DISK_GEOMETRY);
        if (lpInBuffer != NULL || nInBufferSize != 0) status = STATUS_INVALID_PARAMETER;
        else if (nOutBufferSize < sz) status = STATUS_BUFFER_TOO_SMALL;
        else status = CDROM_GetDriveGeometry(dev, (DISK_GEOMETRY*)lpOutBuffer);
        break;

    case IOCTL_CDROM_DISK_TYPE:
        sz = sizeof(CDROM_DISK_DATA);
	/* CDROM_ClearCacheEntry(dev); */
        if (lpInBuffer != NULL || nInBufferSize != 0) status = STATUS_INVALID_PARAMETER;
        else if (nOutBufferSize < sz) status = STATUS_BUFFER_TOO_SMALL;
        else status = CDROM_GetDiskData(dev, (CDROM_DISK_DATA*)lpOutBuffer);
        break;

/* EPP     case IOCTL_CDROM_GET_LAST_SESSION: */

    case IOCTL_CDROM_READ_Q_CHANNEL:
        sz = sizeof(SUB_Q_CHANNEL_DATA);
        if (lpInBuffer == NULL || nInBufferSize < sizeof(CDROM_SUB_Q_DATA_FORMAT))
            status = STATUS_INVALID_PARAMETER;
        else if (nOutBufferSize < sz) status = STATUS_BUFFER_TOO_SMALL;
        else status = CDROM_ReadQChannel(dev, (const CDROM_SUB_Q_DATA_FORMAT*)lpInBuffer,
                                        (SUB_Q_CHANNEL_DATA*)lpOutBuffer);
        break;

    case IOCTL_CDROM_READ_TOC:
        sz = sizeof(CDROM_TOC);
        if (lpInBuffer != NULL || nInBufferSize != 0) status = STATUS_INVALID_PARAMETER;
        else if (nOutBufferSize < sz) status = STATUS_BUFFER_TOO_SMALL;
        else status = CDROM_ReadTOC(dev, (CDROM_TOC*)lpOutBuffer);
        break;

/* EPP     case IOCTL_CDROM_READ_TOC_EX: */

    case IOCTL_CDROM_PAUSE_AUDIO:
        sz = 0;
        if (lpInBuffer != NULL || nInBufferSize != 0 || lpOutBuffer != NULL || nOutBufferSize != 0)
            status = STATUS_INVALID_PARAMETER;
        else status = CDROM_PauseAudio(dev);
        break;
    case IOCTL_CDROM_PLAY_AUDIO_MSF:
        sz = 0;
        if (lpOutBuffer != NULL || nOutBufferSize != 0) status = STATUS_INVALID_PARAMETER;
        else if (nInBufferSize < sizeof(CDROM_PLAY_AUDIO_MSF)) status = STATUS_BUFFER_TOO_SMALL;
        else status = CDROM_PlayAudioMSF(dev, (const CDROM_PLAY_AUDIO_MSF*)lpInBuffer);
        break;
    case IOCTL_CDROM_RESUME_AUDIO:
        sz = 0;
        if (lpInBuffer != NULL || nInBufferSize != 0 || lpOutBuffer != NULL || nOutBufferSize != 0)
            status = STATUS_INVALID_PARAMETER;
        else status = CDROM_ResumeAudio(dev);
        break;
    case IOCTL_CDROM_SEEK_AUDIO_MSF:
        sz = 0;
        if (lpOutBuffer != NULL || nOutBufferSize != 0) status = STATUS_INVALID_PARAMETER;
        else if (nInBufferSize < sizeof(CDROM_SEEK_AUDIO_MSF)) status = STATUS_BUFFER_TOO_SMALL;
        else status = CDROM_SeekAudioMSF(dev, (const CDROM_SEEK_AUDIO_MSF*)lpInBuffer);
        break;
    case IOCTL_CDROM_STOP_AUDIO:
        sz = 0;
	CDROM_ClearCacheEntry(dev); /* Maybe intention is to change media */
        if (lpInBuffer != NULL || nInBufferSize != 0 || lpOutBuffer != NULL || nOutBufferSize != 0)
            status = STATUS_INVALID_PARAMETER;
        else status = CDROM_StopAudio(dev);
        break;
    case IOCTL_CDROM_GET_VOLUME:
        sz = sizeof(VOLUME_CONTROL);
        if (lpInBuffer != NULL || nInBufferSize != 0) status = STATUS_INVALID_PARAMETER;
        else if (nOutBufferSize < sz) status = STATUS_BUFFER_TOO_SMALL;
        else status = CDROM_GetVolume(dev, (VOLUME_CONTROL*)lpOutBuffer);
        break;
    case IOCTL_CDROM_SET_VOLUME:
        sz = 0;
	CDROM_ClearCacheEntry(dev);
        if (lpInBuffer == NULL || nInBufferSize < sizeof(VOLUME_CONTROL) || lpOutBuffer != NULL)
            status = STATUS_INVALID_PARAMETER;
        else status = CDROM_SetVolume(dev, (const VOLUME_CONTROL*)lpInBuffer);
        break;
    case IOCTL_CDROM_RAW_READ:
        sz = 0;
        if (nInBufferSize < sizeof(RAW_READ_INFO)) status = STATUS_INVALID_PARAMETER;
        else if (lpOutBuffer == NULL) status = STATUS_BUFFER_TOO_SMALL;
        else status = CDROM_RawRead(dev, (const RAW_READ_INFO*)lpInBuffer,
                                   lpOutBuffer, nOutBufferSize, &sz);
        break;
    case IOCTL_SCSI_GET_ADDRESS:
        sz = sizeof(SCSI_ADDRESS);
        if (lpInBuffer != NULL || nInBufferSize != 0) status = STATUS_INVALID_PARAMETER;
        else if (nOutBufferSize < sz) status = STATUS_BUFFER_TOO_SMALL;
        else status = CDROM_GetAddress(dev, (SCSI_ADDRESS*)lpOutBuffer);
        break;
    case IOCTL_SCSI_PASS_THROUGH_DIRECT:
        sz = sizeof(SCSI_PASS_THROUGH_DIRECT);
        if (lpOutBuffer == NULL) status = STATUS_INVALID_PARAMETER;
        else if (nOutBufferSize < sizeof(SCSI_PASS_THROUGH_DIRECT)) status = STATUS_BUFFER_TOO_SMALL;
        else status = CDROM_ScsiPassThroughDirect(dev, (PSCSI_PASS_THROUGH_DIRECT)lpOutBuffer);
        break;
    case IOCTL_SCSI_PASS_THROUGH:
        sz = sizeof(SCSI_PASS_THROUGH);
        if (lpOutBuffer == NULL) status = STATUS_INVALID_PARAMETER;
        else if (nOutBufferSize < sizeof(SCSI_PASS_THROUGH)) status = STATUS_BUFFER_TOO_SMALL;
        else status = CDROM_ScsiPassThrough(dev, (PSCSI_PASS_THROUGH)lpOutBuffer);
        break;
    case IOCTL_SCSI_GET_CAPABILITIES:
        sz = sizeof(IO_SCSI_CAPABILITIES);
        if (lpOutBuffer == NULL) status = STATUS_INVALID_PARAMETER;
        else if (nOutBufferSize < sizeof(IO_SCSI_CAPABILITIES)) status = STATUS_BUFFER_TOO_SMALL;
        else status = CDROM_ScsiGetCaps(dev, (PIO_SCSI_CAPABILITIES)lpOutBuffer);
        break;
    default:
        FIXME("Unsupported IOCTL %lx (type=%lx access=%lx func=%lx meth=%lx)\n", 
              dwIoControlCode, dwIoControlCode >> 16, (dwIoControlCode >> 14) & 3,
              (dwIoControlCode >> 2) & 0xFFF, dwIoControlCode & 3);
        sz = 0;
        status = STATUS_INVALID_PARAMETER;
        break;
    }
    CDROM_Close(clientID);
 error:
    piosb->u.Status = status;
    piosb->Information = sz;
    if (hEvent) NtSetEvent(hEvent, NULL);
    return status;
}
