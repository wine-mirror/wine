/*
 * CD-ROM support
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
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
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
    void* buffer;
    uint64_t bufferSize;
    void* sense;
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "winioctl.h"
#include "ntddstor.h"
#include "ntddcdrm.h"
#include "ddk/ntddcdvd.h"
#include "ntddscsi.h"
#include "wine/server.h"
#include "unix_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cdrom);

/* Non-Linux systems do not have linux/cdrom.h and the like, and thus
   lack the following constants. */

#ifndef CD_SECS
# define CD_SECS              60 /* seconds per minute */
#endif
#ifndef CD_FRAMES
# define CD_FRAMES            75 /* frames per second */
#endif

#ifdef WORDS_BIGENDIAN
#define GET_BE_DWORD(x) (x)
#else
#define GET_BE_DWORD(x) RtlUlongByteSwap(x)
#endif

static const struct iocodexs
{
  DWORD code;
  const char *codex;
} iocodextable[] = {
#define X(x) { x, #x },
X(IOCTL_CDROM_CHECK_VERIFY)
X(IOCTL_CDROM_CURRENT_POSITION)
X(IOCTL_CDROM_DISK_TYPE)
X(IOCTL_CDROM_GET_CONTROL)
X(IOCTL_CDROM_GET_DRIVE_GEOMETRY)
X(IOCTL_CDROM_GET_VOLUME)
X(IOCTL_CDROM_LOAD_MEDIA)
X(IOCTL_CDROM_MEDIA_CATALOG)
X(IOCTL_CDROM_MEDIA_REMOVAL)
X(IOCTL_CDROM_PAUSE_AUDIO)
X(IOCTL_CDROM_PLAY_AUDIO_MSF)
X(IOCTL_CDROM_RAW_READ)
X(IOCTL_CDROM_READ_Q_CHANNEL)
X(IOCTL_CDROM_READ_TOC)
X(IOCTL_CDROM_RESUME_AUDIO)
X(IOCTL_CDROM_SEEK_AUDIO_MSF)
X(IOCTL_CDROM_SET_VOLUME)
X(IOCTL_CDROM_STOP_AUDIO)
X(IOCTL_CDROM_TRACK_ISRC)
X(IOCTL_DISK_GET_MEDIA_TYPES)
X(IOCTL_DISK_MEDIA_REMOVAL)
X(IOCTL_DVD_END_SESSION)
X(IOCTL_DVD_GET_REGION)
X(IOCTL_DVD_READ_KEY)
X(IOCTL_DVD_READ_STRUCTURE)
X(IOCTL_DVD_SEND_KEY)
X(IOCTL_DVD_START_SESSION)
X(IOCTL_SCSI_GET_ADDRESS)
X(IOCTL_SCSI_GET_CAPABILITIES)
X(IOCTL_SCSI_GET_INQUIRY_DATA)
X(IOCTL_SCSI_PASS_THROUGH)
X(IOCTL_SCSI_PASS_THROUGH_DIRECT)
X(IOCTL_STORAGE_CHECK_VERIFY)
X(IOCTL_STORAGE_CHECK_VERIFY2)
X(IOCTL_STORAGE_EJECTION_CONTROL)
X(IOCTL_STORAGE_EJECT_MEDIA)
X(IOCTL_STORAGE_GET_DEVICE_NUMBER)
X(IOCTL_STORAGE_GET_MEDIA_TYPES)
X(IOCTL_STORAGE_GET_MEDIA_TYPES_EX)
X(IOCTL_STORAGE_LOAD_MEDIA)
X(IOCTL_STORAGE_MEDIA_REMOVAL)
X(IOCTL_STORAGE_RESET_DEVICE)
#undef X
};
static const char *iocodex(DWORD code)
{
   unsigned int i;
   static char buffer[25];
   for(i=0; i<ARRAY_SIZE(iocodextable); i++)
      if (code==iocodextable[i].code)
	 return iocodextable[i].codex;
   snprintf(buffer, sizeof(buffer), "IOCTL_CODE_%x", code);
   return buffer;
}

#define INQ_REPLY_LEN 36
#define INQ_CMD_LEN 6

#define FRAME_OF_ADDR(a) (((int)(a)[1] * CD_SECS + (a)[2]) * CD_FRAMES + (a)[3])
#define FRAME_OF_MSF(a) (((int)(a).M * CD_SECS + (a).S) * CD_FRAMES + (a).F)
#define FRAME_OF_TOC(toc, idx)  FRAME_OF_ADDR((toc).TrackData[idx - (toc).FirstTrack].Address)
#define MSF_OF_FRAME(m,fr) {int f=(fr); ((UCHAR *)&(m))[2]=f%CD_FRAMES;f/=CD_FRAMES;((UCHAR *)&(m))[1]=f%CD_SECS;((UCHAR *)&(m))[0]=f/CD_SECS;}

typedef struct _SCSI_PASS_THROUGH32
 {
    USHORT       Length;
    UCHAR        ScsiStatus;
    UCHAR        PathId;
    UCHAR        TargetId;
    UCHAR        Lun;
    UCHAR        CdbLength;
    UCHAR        SenseInfoLength;
    UCHAR        DataIn;
    ULONG        DataTransferLength;
    ULONG        TimeOutValue;
    ULONG        DataBufferOffset;
    ULONG        SenseInfoOffset;
    UCHAR        Cdb[16];
} SCSI_PASS_THROUGH32, *PSCSI_PASS_THROUGH32;

/* The documented format of DVD_LAYER_DESCRIPTOR is wrong. Even the format in the
 * DDK's header is wrong. There are four bytes at the start  defined by
 * MMC-5. The first two are the size of the structure in big-endian order as
 * defined by MMC-5. The other two are reserved.
 */
typedef struct
{
    DVD_DESCRIPTOR_HEADER Header;
    DVD_LAYER_DESCRIPTOR Descriptor;
    UCHAR Padding;
} internal_dvd_layer_descriptor;
C_ASSERT(sizeof(internal_dvd_layer_descriptor) == 22);

typedef struct
{
    DVD_DESCRIPTOR_HEADER Header;
    DVD_MANUFACTURER_DESCRIPTOR Descriptor;
    UCHAR Padding;
} internal_dvd_manufacturer_descriptor;
C_ASSERT(sizeof(internal_dvd_manufacturer_descriptor) == 2053);

static NTSTATUS CDROM_ReadTOC(int, int, CDROM_TOC*);
static NTSTATUS CDROM_GetStatusCode(int);

/* FIXME: this is needed because we can't open simultaneously several times /dev/cdrom
 * this should be removed when a proper device interface is implemented
 *
 * (WS) We need this to keep track of current position and to safely
 * detect media changes. Besides this should provide a great speed up
 * for toc inquiries.
 */
struct cdrom_cache {
    dev_t       device;
    ino_t       inode;
    char        toc_good; /* if false, will reread TOC from disk */
    CDROM_TOC   toc;
    SUB_Q_CURRENT_POSITION CurrentPosition;
};
/* who has more than 5 cdroms on his/her machine ?? */
/* FIXME: this should grow depending on the number of cdroms we install/configure
 * at startup
 */
#define MAX_CACHE_ENTRIES       5
static struct cdrom_cache cdrom_cache[MAX_CACHE_ENTRIES];

static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Proposed media change function: not really needed at this time */
/* This is a 1 or 0 type of function */
#if 0
static int CDROM_MediaChanged(int dev)
{
   int i;

   struct cdrom_tochdr	hdr;
   struct cdrom_tocentry entry;

   if (dev < 0 || dev >= MAX_CACHE_ENTRIES)
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
 *		get_parent_device
 *
 * On Mac OS, get the device for the whole disk from a fd that points to a partition.
 * This is ugly and inefficient, but we have no choice since the partition fd doesn't
 * support the eject ioctl.
 */
#ifdef __APPLE__
static NTSTATUS get_parent_device( int fd, char *name, size_t len )
{
    NTSTATUS status = STATUS_NO_SUCH_FILE;
    struct stat st;
    int i;
    io_service_t service;
    CFMutableDictionaryRef dict;
    CFTypeRef val;

    if (fstat( fd, &st ) == -1) return errno_to_status( errno );
    if (!S_ISCHR( st.st_mode )) return STATUS_OBJECT_TYPE_MISMATCH;

    /* create a dictionary with the right major/minor numbers */

    if (!(dict = IOServiceMatching( kIOMediaClass ))) return STATUS_NO_MEMORY;

    i = major( st.st_rdev );
    val = CFNumberCreate( NULL, kCFNumberIntType, &i );
    CFDictionaryAddValue( dict, CFSTR( "BSD Major" ), val );
    CFRelease( val );

    i = minor( st.st_rdev );
    val = CFNumberCreate( NULL, kCFNumberIntType, &i );
    CFDictionaryAddValue( dict, CFSTR( "BSD Minor" ), val );
    CFRelease( val );

    CFDictionaryAddValue( dict, CFSTR("Removable"), kCFBooleanTrue );

    service = IOServiceGetMatchingService( 0, dict );

    /* now look for the parent that has the "Whole" attribute set to TRUE */

    while (service)
    {
        io_service_t parent = 0;
        CFBooleanRef whole;
        CFStringRef str;
        int ok;

        if (!IOObjectConformsTo( service, kIOMediaClass ))
            goto next;
        if (!(whole = IORegistryEntryCreateCFProperty( service, CFSTR("Whole"), NULL, 0 )))
            goto next;
        ok = (whole == kCFBooleanTrue);
        CFRelease( whole );
        if (!ok) goto next;

        if ((str = IORegistryEntryCreateCFProperty( service, CFSTR("BSD Name"), NULL, 0 )))
        {
            strcpy( name, "/dev/r" );
            CFStringGetCString( str, name + 6, len - 6, kCFStringEncodingUTF8 );
            CFRelease( str );
            status = STATUS_SUCCESS;
        }
        IOObjectRelease( service );
        break;

next:
        IORegistryEntryGetParentEntry( service, kIOServicePlane, &parent );
        IOObjectRelease( service );
        service = parent;
    }
    return status;
}
#endif


/******************************************************************
 *		CDROM_SyncCache                          [internal]
 *
 * Read the TOC in and store it in the cdrom_cache structure.
 * Further requests for the TOC will be copied from the cache
 * unless certain events like disk ejection is detected, in which
 * case the cache will be cleared, causing it to be resynced.
 * The cache section must be held by caller.
 */
static NTSTATUS CDROM_SyncCache(int dev, int fd)
{
#ifdef linux
   int i, tsz;
   struct cdrom_tochdr		hdr;
   struct cdrom_tocentry	entry;

   CDROM_TOC *toc = &cdrom_cache[dev].toc;
   cdrom_cache[dev].toc_good = 0;

   if (ioctl(fd, CDROMREADTOCHDR, &hdr) == -1)
   {
      WARN("(%d) -- Error occurred (%s)!\n", dev, strerror(errno));
      return errno_to_status( errno );
   }

   toc->FirstTrack = hdr.cdth_trk0;
   toc->LastTrack  = hdr.cdth_trk1;
   tsz = sizeof(toc->FirstTrack) + sizeof(toc->LastTrack)
       + sizeof(TRACK_DATA) * (toc->LastTrack-toc->FirstTrack+2);
   toc->Length[0] = tsz >> 8;
   toc->Length[1] = tsz;

   TRACE("caching toc from=%d to=%d\n", toc->FirstTrack, toc->LastTrack );

   for (i = toc->FirstTrack; i <= toc->LastTrack + 1; i++)
   {
     if (i == toc->LastTrack + 1)
       entry.cdte_track = CDROM_LEADOUT;
     else
       entry.cdte_track = i;
     entry.cdte_format = CDROM_MSF;
     if (ioctl(fd, CDROMREADTOCENTRY, &entry) == -1)
     {
       WARN("error read entry (%s)\n", strerror(errno));
       return errno_to_status( errno );
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
   return STATUS_SUCCESS;

#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__DragonFly__)

   int i, tsz;
   struct ioc_toc_header hdr;
   struct ioc_read_toc_entry entry;
   struct cd_toc_entry toc_buffer;

   CDROM_TOC *toc = &cdrom_cache[dev].toc;
   cdrom_cache[dev].toc_good = 0;

    if (ioctl(fd, CDIOREADTOCHEADER, &hdr) == -1)
    {
        WARN("(%d) -- Error occurred (%s)!\n", dev, strerror(errno));
        return errno_to_status( errno );
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
        memset(&toc_buffer, 0, sizeof(toc_buffer));
	entry.address_format = CD_MSF_FORMAT;
	entry.data_len = sizeof(toc_buffer);
	entry.data = &toc_buffer;
        if (ioctl(fd, CDIOREADTOCENTRYS, &entry) == -1)
        {
	    WARN("error read entry (%s)\n", strerror(errno));
            return errno_to_status( errno );
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
    return STATUS_SUCCESS;

#elif defined(__APPLE__)
    int i;
    dk_cd_read_toc_t hdr;
    CDROM_TOC *toc = &cdrom_cache[dev].toc;
    cdrom_cache[dev].toc_good = 0;

    memset( &hdr, 0, sizeof(hdr) );
    hdr.buffer = toc;
    hdr.bufferLength = sizeof(*toc);
    if (ioctl(fd, DKIOCCDREADTOC, &hdr) == -1)
    {
        WARN("(%d) -- Error occurred (%s)!\n", dev, strerror(errno));
        return errno_to_status( errno );
    }
    for (i = toc->FirstTrack; i <= toc->LastTrack + 1; i++)
    {
        /* convert address format */
        TRACK_DATA *data = &toc->TrackData[i - toc->FirstTrack];
        DWORD frame = (((DWORD)data->Address[0] << 24) | ((DWORD)data->Address[1] << 16) |
                       ((DWORD)data->Address[2] << 8) | data->Address[3]);
        MSF_OF_FRAME( data->Address[1], frame );
        data->Address[0] = 0;
    }

    cdrom_cache[dev].toc_good = 1;
    return STATUS_SUCCESS;
#else
    FIXME("not supported on this O/S\n");
    return STATUS_NOT_SUPPORTED;
#endif
}

static void CDROM_ClearCacheEntry(int dev)
{
    mutex_lock( &cache_mutex );
    cdrom_cache[dev].toc_good = 0;
    mutex_unlock( &cache_mutex );
}



/******************************************************************
 *		CDROM_GetInterfaceInfo
 *
 * Determines the ide interface (the number after the ide), and the
 * number of the device on that interface for ide cdroms (*iface <= 1).
 * Determines the scsi information for scsi cdroms (*iface >= 2).
 * Returns FALSE if the info cannot not be obtained.
 */
static BOOL CDROM_GetInterfaceInfo(int fd, UCHAR* iface, UCHAR* port, UCHAR* device, UCHAR* lun)
{
#if defined(linux)
    struct stat st;
    if ( fstat(fd, &st) == -1 || ! S_ISBLK(st.st_mode)) return FALSE;
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
        if (ioctl(fd, SCSI_IOCTL_GET_IDLUN, idlun) != -1)
        {
            *port = (idlun[0] >> 24) & 0xff;
            *iface = ((idlun[0] >> 16) & 0xff) + 2;
            *device = idlun[0] & 0xff;
            *lun = (idlun[0] >> 8) & 0xff;
        }
        else
#endif
        {
            WARN("CD-ROM device (%d, %d) not supported\n", major(st.st_rdev), minor(st.st_rdev));
            return FALSE;
        }
    }
    return TRUE;
#elif defined(__NetBSD__)
    struct scsi_addr addr;
    if (ioctl(fd, SCIOCIDENTIFY, &addr) != -1)
    {
        switch (addr.type)
        {
        case TYPE_SCSI:
            *port = 1;
            *iface = addr.addr.scsi.scbus;
            *device = addr.addr.scsi.target;
            *lun = addr.addr.scsi.lun;
            return TRUE;
        case TYPE_ATAPI:
            *port = 0;
            *iface = addr.addr.atapi.atbus;
            *device = addr.addr.atapi.drive;
            *lun = 0;
            return TRUE;
        }
    }
    return FALSE;
#elif defined(__APPLE__)
    dk_scsi_identify_t addr;
    if (ioctl(fd, DKIOCSCSIIDENTIFY, &addr) != -1)
    {
       *port = addr.bus;
       *iface = addr.port;
       *device = addr.target;
       *lun = addr.lun;
       return TRUE;
    }
    return FALSE;
#else
    FIXME("not implemented on this O/S\n");
    return FALSE;
#endif
}


/******************************************************************
 *		CDROM_Open
 *
 */
static NTSTATUS CDROM_Open(int fd, int* dev)
{
    struct stat st;
    NTSTATUS ret = STATUS_SUCCESS;
    int         empty = -1;

    if (fstat(fd, &st) == -1) return errno_to_status( errno );

    mutex_lock( &cache_mutex );
    for (*dev = 0; *dev < MAX_CACHE_ENTRIES; (*dev)++)
    {
        if (empty == -1 &&
            cdrom_cache[*dev].device == 0 &&
            cdrom_cache[*dev].inode == 0)
            empty = *dev;
        else if (cdrom_cache[*dev].device == st.st_dev &&
                 cdrom_cache[*dev].inode == st.st_ino)
            break;
    }
    if (*dev == MAX_CACHE_ENTRIES)
    {
        if (empty == -1) ret = STATUS_NOT_IMPLEMENTED;
        else
        {
            *dev = empty;
            cdrom_cache[*dev].device  = st.st_dev;
            cdrom_cache[*dev].inode   = st.st_ino;
        }
    }
    mutex_unlock( &cache_mutex );

    TRACE("%d, %d\n", *dev, fd);
    return ret;
}

/******************************************************************
 *		CDROM_GetStatusCode
 *
 *
 */
static NTSTATUS CDROM_GetStatusCode(int io)
{
    if (io == 0) return STATUS_SUCCESS;
    return errno_to_status( errno );
}

/******************************************************************
 *		CDROM_GetControl
 *
 */
static NTSTATUS CDROM_GetControl(int dev, int fd, CDROM_AUDIO_CONTROL* cac)
{
#ifdef __APPLE__
    uint16_t speed;
    int io = ioctl( fd, DKIOCCDGETSPEED, &speed );
    if (io != 0) return CDROM_GetStatusCode( io );
    /* DKIOCCDGETSPEED returns the speed in kilobytes per second,
     * so convert to logical blocks (assumed to be ~2 KB).
     */
    cac->LogicalBlocksPerSecond = speed/2;
#else
    cac->LogicalBlocksPerSecond = 1; /* FIXME */
#endif
    cac->LbaFormat = 0; /* FIXME */
    return  STATUS_NOT_SUPPORTED;
}

/******************************************************************
 *		CDROM_GetDeviceNumber
 *
 */
static NTSTATUS CDROM_GetDeviceNumber(int dev, STORAGE_DEVICE_NUMBER* devnum)
{
    FIXME( "stub\n" );
    devnum->DeviceType = FILE_DEVICE_DISK;
    devnum->DeviceNumber = 1;
    devnum->PartitionNumber = 1;
    return STATUS_SUCCESS;
}

/******************************************************************
 *		CDROM_GetDriveGeometry
 *
 */
static NTSTATUS CDROM_GetDriveGeometry(int dev, int fd, DISK_GEOMETRY* dg)
{
  CDROM_TOC     toc;
  NTSTATUS      ret;
  int           fsize;

  if ((ret = CDROM_ReadTOC(dev, fd, &toc)) != 0) return ret;

  fsize = FRAME_OF_TOC(toc, toc.LastTrack+1)
        - FRAME_OF_TOC(toc, 1); /* Total size in frames */

  dg->Cylinders.QuadPart = fsize / (64 * 32);
  dg->MediaType = RemovableMedia;
  dg->TracksPerCylinder = 64;
  dg->SectorsPerTrack = 32;
  dg->BytesPerSector= 2048;
  return ret;
}

/******************************************************************
 *		CDROM_GetMediaType
 *
 */
static NTSTATUS CDROM_GetMediaType(int dev, int fd, GET_MEDIA_TYPES* medtype)
{
    FIXME("semi-stub\n");
    medtype->DeviceType = FILE_DEVICE_CD_ROM;
    medtype->MediaInfoCount = 0;

#if defined(HAVE_SG_IO_HDR_T_INTERFACE_ID) && defined(HAVE_LINUX_CDROM_H)
    {
        unsigned char drive_config[8];
        unsigned char get_config_cmd[10] = { GPCMD_GET_CONFIGURATION, 0, 0xff, 0xff,
                                             0, 0, 0, 0, sizeof(drive_config), 0 };
        sg_io_hdr_t iocmd =
        {
            .interface_id = 'S',
            .dxfer_direction = SG_DXFER_FROM_DEV,
            .cmd_len = sizeof(get_config_cmd),
            .dxfer_len = sizeof(drive_config),
            .dxferp = drive_config,
            .cmdp = get_config_cmd,
            .timeout = 1000,
        };
        int err;

        if ((err = ioctl(fd, SG_IO, &iocmd)))
            return CDROM_GetStatusCode(err);

        if (iocmd.status == 0 && (drive_config[6] || drive_config[7] >= 0x10))
            medtype->DeviceType = FILE_DEVICE_DVD;
    }
#endif

    return STATUS_SUCCESS;
}

/**************************************************************************
 *                              CDROM_Reset                     [internal]
 */
static NTSTATUS CDROM_ResetAudio(int fd)
{
#if defined(linux)
    return CDROM_GetStatusCode(ioctl(fd, CDROMRESET));
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__DragonFly__)
    return CDROM_GetStatusCode(ioctl(fd, CDIOCRESET, NULL));
#else
    FIXME("not supported on this O/S\n");
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *		CDROM_SetTray
 *
 *
 */
static NTSTATUS CDROM_SetTray(int fd, BOOL doEject)
{
#if defined(linux)
    return CDROM_GetStatusCode(ioctl(fd, doEject ? CDROMEJECT : CDROMCLOSETRAY));
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__DragonFly__)
    return CDROM_GetStatusCode((ioctl(fd, CDIOCALLOW, NULL)) ||
                               (ioctl(fd, doEject ? CDIOCEJECT : CDIOCCLOSE, NULL)) ||
                               (ioctl(fd, CDIOCPREVENT, NULL)));
#elif defined(__APPLE__)
    if (doEject) return CDROM_GetStatusCode( ioctl( fd, DKIOCEJECT, NULL ) );
    else return STATUS_NOT_SUPPORTED;
#else
    FIXME("not supported on this O/S\n");
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *		CDROM_ControlEjection
 *
 *
 */
static NTSTATUS CDROM_ControlEjection(int fd, const PREVENT_MEDIA_REMOVAL* rmv)
{
#if defined(linux)
    return CDROM_GetStatusCode(ioctl(fd, CDROM_LOCKDOOR, rmv->PreventMediaRemoval));
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__DragonFly__)
    return CDROM_GetStatusCode(ioctl(fd, (rmv->PreventMediaRemoval) ? CDIOCPREVENT : CDIOCALLOW, NULL));
#else
    FIXME("not supported on this O/S\n");
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *		CDROM_ReadTOC
 *
 *
 */
static NTSTATUS CDROM_ReadTOC(int dev, int fd, CDROM_TOC* toc)
{
    NTSTATUS       ret = STATUS_NOT_SUPPORTED;

    if (dev < 0 || dev >= MAX_CACHE_ENTRIES)
        return STATUS_INVALID_PARAMETER;

    mutex_lock( &cache_mutex );
    if (cdrom_cache[dev].toc_good || !(ret = CDROM_SyncCache(dev, fd)))
    {
        *toc = cdrom_cache[dev].toc;
        ret = STATUS_SUCCESS;
    }
    mutex_unlock( &cache_mutex );
    return ret;
}

/******************************************************************
 *		CDROM_GetDiskData
 *
 *
 */
static NTSTATUS CDROM_GetDiskData(int dev, int fd, CDROM_DISK_DATA* data)
{
    CDROM_TOC   toc;
    NTSTATUS    ret;
    int         i;

    if ((ret = CDROM_ReadTOC(dev, fd, &toc)) != 0) return ret;
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
static NTSTATUS CDROM_ReadQChannel(int dev, int fd, const CDROM_SUB_Q_DATA_FORMAT* fmt,
                                   SUB_Q_CHANNEL_DATA* data)
{
    NTSTATUS            ret = STATUS_NOT_SUPPORTED;
#ifdef linux
    SUB_Q_HEADER*       hdr = (SUB_Q_HEADER*)data;
    int                 io;
    struct cdrom_subchnl	sc;
    sc.cdsc_format = CDROM_MSF;

    io = ioctl(fd, CDROMSUBCHNL, &sc);
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
        hdr->AudioStatus = AUDIO_STATUS_NO_STATUS;
    }
    switch (fmt->Format)
    {
    case IOCTL_CDROM_CURRENT_POSITION:
        mutex_lock( &cache_mutex );
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
        mutex_unlock( &cache_mutex );
        break;
    case IOCTL_CDROM_MEDIA_CATALOG:
        data->MediaCatalog.FormatCode = IOCTL_CDROM_MEDIA_CATALOG;
        {
            struct cdrom_mcn mcn;
            if ((io = ioctl(fd, CDROM_GET_MCN, &mcn)) == -1) goto end;

            data->MediaCatalog.FormatCode = IOCTL_CDROM_MEDIA_CATALOG;
            data->MediaCatalog.Mcval = 0; /* FIXME */
            memcpy(data->MediaCatalog.MediaCatalog, mcn.medium_catalog_number, 14);
            data->MediaCatalog.MediaCatalog[14] = 0;
        }
        break;
    case IOCTL_CDROM_TRACK_ISRC:
        FIXME("TrackIsrc: NIY on linux\n");
        data->TrackIsrc.FormatCode = IOCTL_CDROM_TRACK_ISRC;
        data->TrackIsrc.Tcval = 0;
        io = 0;
        break;
    }

 end:
    ret = CDROM_GetStatusCode(io);
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__DragonFly__)
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
    io = ioctl(fd, CDIOCREADSUBCHANNEL, &read_sc);
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
        hdr->AudioStatus = AUDIO_STATUS_NO_STATUS;
    }
    switch (fmt->Format)
    {
    case IOCTL_CDROM_CURRENT_POSITION:
        mutex_lock( &cache_mutex );
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
        mutex_unlock( &cache_mutex );
        break;
    case IOCTL_CDROM_MEDIA_CATALOG:
        data->MediaCatalog.FormatCode = IOCTL_CDROM_MEDIA_CATALOG;
        data->MediaCatalog.Mcval = sc.what.media_catalog.mc_valid;
        memcpy(data->MediaCatalog.MediaCatalog, sc.what.media_catalog.mc_number, 15);
        break;
    case IOCTL_CDROM_TRACK_ISRC:
        data->TrackIsrc.FormatCode = IOCTL_CDROM_TRACK_ISRC;
        data->TrackIsrc.Tcval = sc.what.track_info.ti_valid;
        memcpy(data->TrackIsrc.TrackIsrc, sc.what.track_info.ti_number, 15);
        break;
    }

 end:
    ret = CDROM_GetStatusCode(io);
#elif defined(__APPLE__)
    SUB_Q_HEADER* hdr = (SUB_Q_HEADER*)data;
    int io = 0;
    union
    {
        dk_cd_read_mcn_t mcn;
        dk_cd_read_isrc_t isrc;
    } ioc;
    /* We need IOCDAudioControl for IOCTL_CDROM_CURRENT_POSITION */
    if (fmt->Format == IOCTL_CDROM_CURRENT_POSITION)
    {
        FIXME("NIY\n");
        return STATUS_NOT_SUPPORTED;
    }
    /* No IOCDAudioControl support; just set the audio status to none */
    hdr->AudioStatus = AUDIO_STATUS_NO_STATUS;
    switch(fmt->Format)
    {
    case IOCTL_CDROM_MEDIA_CATALOG:
        if ((io = ioctl(fd, DKIOCCDREADMCN, &ioc.mcn)) == -1) break;
        memcpy(data->MediaCatalog.MediaCatalog, ioc.mcn.mcn, kCDMCNMaxLength);
        data->MediaCatalog.Mcval = 1;
        break;
    case IOCTL_CDROM_TRACK_ISRC:
        ioc.isrc.track = fmt->Track;
        if ((io = ioctl(fd, DKIOCCDREADISRC, &ioc.isrc)) == -1) break;
        memcpy(data->TrackIsrc.TrackIsrc, ioc.isrc.isrc, kCDISRCMaxLength);
        data->TrackIsrc.Tcval = 1;
        data->TrackIsrc.Track = ioc.isrc.track;
    }
    ret = CDROM_GetStatusCode(io);
#endif
    return ret;
}

/******************************************************************
 *		CDROM_Verify
 *  Implements: IOCTL_STORAGE_CHECK_VERIFY
 *              IOCTL_CDROM_CHECK_VERIFY
 *              IOCTL_DISK_CHECK_VERIFY
 *
 */
static NTSTATUS CDROM_Verify(int dev, int fd)
{
#if defined(linux)
    int ret;

    ret = ioctl(fd,  CDROM_DRIVE_STATUS, NULL);
    if(ret == -1) {
        TRACE("ioctl CDROM_DRIVE_STATUS failed(%s)!\n", strerror(errno));
        return CDROM_GetStatusCode(ret);
    }

    if(ret == CDS_DISC_OK)
        return STATUS_SUCCESS;
    else
        return STATUS_NO_MEDIA_IN_DEVICE;
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__DragonFly__)
    int ret;
    ret = ioctl(fd, CDIOCSTART, NULL);
    if(ret == 0)
        return STATUS_SUCCESS;
    else
        return STATUS_NO_MEDIA_IN_DEVICE;
#elif defined(__APPLE__)
	/* At this point, we know that we have media, because in Mac OS X, the
	 * device file is only created when media is present. */
	return STATUS_SUCCESS;
#else
    FIXME("not supported on this O/S\n");
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *		CDROM_PlayAudioMSF
 *
 *
 */
static NTSTATUS CDROM_PlayAudioMSF(int fd, const CDROM_PLAY_AUDIO_MSF* audio_msf)
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

    io = ioctl(fd, CDROMSTART);
    if (io == -1)
    {
	WARN("motor doesn't start !\n");
	goto end;
    }
    io = ioctl(fd, CDROMPLAYMSF, &msf);
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
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__DragonFly__)
    struct	ioc_play_msf	msf;
    int         io;

    msf.start_m      = audio_msf->StartingM;
    msf.start_s      = audio_msf->StartingS;
    msf.start_f      = audio_msf->StartingF;
    msf.end_m        = audio_msf->EndingM;
    msf.end_s        = audio_msf->EndingS;
    msf.end_f        = audio_msf->EndingF;

    io = ioctl(fd, CDIOCSTART, NULL);
    if (io == -1)
    {
	WARN("motor doesn't start !\n");
	goto end;
    }
    io = ioctl(fd, CDIOCPLAYMSF, &msf);
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
static NTSTATUS CDROM_SeekAudioMSF(int dev, int fd, const CDROM_SEEK_AUDIO_MSF* audio_msf)
{
    CDROM_TOC toc;
    int i, io, frame;
    SUB_Q_CURRENT_POSITION *cp;
#if defined(linux)
    struct cdrom_msf0	msf;
    struct cdrom_subchnl sc;
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__DragonFly__)
    struct ioc_play_msf	msf;
    struct ioc_read_subchannel	read_sc;
    struct cd_sub_channel_info	sc;
    int final_frame;
#endif

    /* Use the information on the TOC to compute the new current
     * position, which is shadowed on the cache. [Portable]. */
    frame = FRAME_OF_MSF(*audio_msf);

    if ((io = CDROM_ReadTOC(dev, fd, &toc)) != 0) return io;

    for(i=toc.FirstTrack;i<=toc.LastTrack+1;i++)
      if (FRAME_OF_TOC(toc,i)>frame) break;
    if (i <= toc.FirstTrack || i > toc.LastTrack+1)
      return STATUS_INVALID_PARAMETER;
    i--;
    mutex_lock( &cache_mutex );
    cp = &cdrom_cache[dev].CurrentPosition;
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
    mutex_unlock( &cache_mutex );

    /* If playing, then issue a seek command, otherwise do nothing */
#ifdef linux
    sc.cdsc_format = CDROM_MSF;

    io = ioctl(fd, CDROMSUBCHNL, &sc);
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
      return CDROM_GetStatusCode(ioctl(fd, CDROMSEEK, &msf));
    }
    return STATUS_SUCCESS;
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__DragonFly__)
    read_sc.address_format = CD_MSF_FORMAT;
    read_sc.track          = 0;
    read_sc.data_len       = sizeof(sc);
    read_sc.data           = &sc;
    read_sc.data_format    = CD_CURRENT_POSITION;

    io = ioctl(fd, CDIOCREADSUBCHANNEL, &read_sc);
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

      return CDROM_GetStatusCode(ioctl(fd, CDIOCPLAYMSF, &msf));
    }
    return STATUS_SUCCESS;
#else
    FIXME("not supported on this O/S\n");
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *		CDROM_PauseAudio
 *
 *
 */
static NTSTATUS CDROM_PauseAudio(int fd)
{
#if defined(linux)
    return CDROM_GetStatusCode(ioctl(fd, CDROMPAUSE));
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__DragonFly__)
    return CDROM_GetStatusCode(ioctl(fd, CDIOCPAUSE, NULL));
#else
    FIXME(": not supported on this O/S\n");
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *		CDROM_ResumeAudio
 *
 *
 */
static NTSTATUS CDROM_ResumeAudio(int fd)
{
#if defined(linux)
    return CDROM_GetStatusCode(ioctl(fd, CDROMRESUME));
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__DragonFly__)
    return CDROM_GetStatusCode(ioctl(fd, CDIOCRESUME, NULL));
#else
    FIXME("not supported on this O/S\n");
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *		CDROM_StopAudio
 *
 *
 */
static NTSTATUS CDROM_StopAudio(int fd)
{
#if defined(linux)
    return CDROM_GetStatusCode(ioctl(fd, CDROMSTOP));
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__DragonFly__)
    return CDROM_GetStatusCode(ioctl(fd, CDIOCSTOP, NULL));
#else
    FIXME("not supported on this O/S\n");
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *		CDROM_GetVolume
 *
 *
 */
static NTSTATUS CDROM_GetVolume(int fd, VOLUME_CONTROL* vc)
{
#if defined(linux)
    struct cdrom_volctrl volc;
    int io;

    io = ioctl(fd, CDROMVOLREAD, &volc);
    if (io != -1)
    {
        vc->PortVolume[0] = volc.channel0;
        vc->PortVolume[1] = volc.channel1;
        vc->PortVolume[2] = volc.channel2;
        vc->PortVolume[3] = volc.channel3;
    }
    return CDROM_GetStatusCode(io);
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__DragonFly__)
    struct  ioc_vol     volc;
    int io;

    io = ioctl(fd, CDIOCGETVOL, &volc);
    if (io != -1)
    {
        vc->PortVolume[0] = volc.vol[0];
        vc->PortVolume[1] = volc.vol[1];
        vc->PortVolume[2] = volc.vol[2];
        vc->PortVolume[3] = volc.vol[3];
    }
    return CDROM_GetStatusCode(io);
#else
    FIXME("not supported on this O/S\n");
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *		CDROM_SetVolume
 *
 *
 */
static NTSTATUS CDROM_SetVolume(int fd, const VOLUME_CONTROL* vc)
{
#if defined(linux)
    struct cdrom_volctrl volc;

    volc.channel0 = vc->PortVolume[0];
    volc.channel1 = vc->PortVolume[1];
    volc.channel2 = vc->PortVolume[2];
    volc.channel3 = vc->PortVolume[3];

    return CDROM_GetStatusCode(ioctl(fd, CDROMVOLCTRL, &volc));
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__DragonFly__)
    struct  ioc_vol     volc;

    volc.vol[0] = vc->PortVolume[0];
    volc.vol[1] = vc->PortVolume[1];
    volc.vol[2] = vc->PortVolume[2];
    volc.vol[3] = vc->PortVolume[3];

    return CDROM_GetStatusCode(ioctl(fd, CDIOCSETVOL, &volc));
#else
    FIXME(": not supported on this O/S\n");
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *		CDROM_RawRead
 *
 * Some features of this IOCTL are rather poorly documented and
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
 *      size. The entire raw sector (i.e. 2352 bytes of data)
 *      is always returned. IMO the TrackMode is only used
 *      to check the correct sector type.
 *
 */
static NTSTATUS CDROM_RawRead(int fd, const RAW_READ_INFO* raw, void* buffer, DWORD len, DWORD* sz)
{
    int         ret = STATUS_NOT_SUPPORTED;
    int         io = -1;
#ifdef __APPLE__
    dk_cd_read_t cdrd;
#endif

    TRACE("RAW_READ_INFO: DiskOffset=%s SectorCount=%i TrackMode=%i\n buffer=%p len=%i sz=%p\n",
          wine_dbgstr_longlong(raw->DiskOffset.QuadPart), raw->SectorCount, raw->TrackMode, buffer, len, sz);

    if (len < raw->SectorCount * 2352) return STATUS_BUFFER_TOO_SMALL;

#if defined(linux)
    if (raw->DiskOffset.u.HighPart & ~2047) {
        WARN("DiskOffset points to a sector >= 2**32\n");
        return ret;
    }

    switch (raw->TrackMode)
    {
    case YellowMode2:
    case XAForm2:
    {
        DWORD lba = raw->DiskOffset.QuadPart >> 11;
        struct cdrom_msf*       msf;
        PBYTE                   *bp; /* current buffer pointer */
        DWORD i;

        if ((lba + raw->SectorCount) >
            ((1 << 8*sizeof(msf->cdmsf_min0)) * CD_SECS * CD_FRAMES
             - CD_MSF_OFFSET)) {
            WARN("DiskOffset not accessible with MSF\n");
            return ret;
        }

        /* Linux reads only one sector at a time.
         * ioctl CDROMREADRAW takes struct cdrom_msf as an argument
         * on the contrary to what header comments state.
         */
        lba += CD_MSF_OFFSET;
        for (i = 0, bp = buffer; i < raw->SectorCount;
             i++, lba++, bp += 2352)
        {
            msf = (struct cdrom_msf*)bp;
            msf->cdmsf_min0   = lba / CD_FRAMES / CD_SECS;
            msf->cdmsf_sec0   = lba / CD_FRAMES % CD_SECS;
            msf->cdmsf_frame0 = lba % CD_FRAMES;
            io = ioctl(fd, CDROMREADRAW, msf);
            if (io != 0)
            {
                *sz = 2352 * i;
                return CDROM_GetStatusCode(io);
            }
        }
        break;
    }

    case CDDA:
    {
        struct cdrom_read_audio cdra;

        cdra.addr.lba = raw->DiskOffset.QuadPart >> 11;
        TRACE("reading at %u\n", cdra.addr.lba);
        cdra.addr_format = CDROM_LBA;
        cdra.nframes = raw->SectorCount;
        cdra.buf = buffer;
        io = ioctl(fd, CDROMREADAUDIO, &cdra);
        break;
    }

    default:
        FIXME("NIY: %d\n", raw->TrackMode);
        return STATUS_INVALID_PARAMETER;
    }
#elif defined(__APPLE__)
    /* Mac OS lets us read multiple parts of the sector at a time.
     * We can read all the sectors in at once, unlike Linux.
     */
    memset(&cdrd, 0, sizeof(cdrd));
    cdrd.offset = (raw->DiskOffset.QuadPart >> 11) * kCDSectorSizeWhole;
    cdrd.buffer = buffer;
    cdrd.bufferLength = raw->SectorCount * kCDSectorSizeWhole;
    switch (raw->TrackMode)
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
        FIXME("NIY: %d\n", raw->TrackMode);
        return STATUS_INVALID_PARAMETER;
    }
    io = ioctl(fd, DKIOCCDREAD, &cdrd);
    if (io != 0)
    {
        *sz = cdrd.bufferLength;
        return CDROM_GetStatusCode(io);
    }
#else
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
    default:
        FIXME("NIY: %d\n", raw->TrackMode);
        return STATUS_INVALID_PARAMETER;
    }
#endif

    *sz = 2352 * raw->SectorCount;
    ret = CDROM_GetStatusCode(io);
    return ret;
}

/******************************************************************
 *        CDROM_ScsiPassThroughDirect
 *        Implements IOCTL_SCSI_PASS_THROUGH_DIRECT
 *
 */
static NTSTATUS CDROM_ScsiPassThroughDirect(int fd, const SCSI_PASS_THROUGH_DIRECT *in_pkt, SCSI_PASS_THROUGH_DIRECT *out_pkt)
{
    int ret = STATUS_NOT_SUPPORTED;
#ifdef HAVE_SG_IO_HDR_T_INTERFACE_ID
    sg_io_hdr_t cmd;
    int io;
#elif defined __APPLE__
    dk_scsi_command_t cmd;
    int io;
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
    RtlZeroMemory(&cmd, sizeof(cmd));

    cmd.interface_id   = 'S';
    cmd.cmd_len        = in_pkt->CdbLength;
    cmd.mx_sb_len      = in_pkt->SenseInfoLength;
    cmd.dxfer_len      = in_pkt->DataTransferLength;
    cmd.dxferp         = in_pkt->DataBuffer;
    cmd.cmdp           = (unsigned char*)in_pkt->Cdb;
    cmd.sbp            = (unsigned char*)out_pkt + in_pkt->SenseInfoOffset;
    cmd.timeout        = in_pkt->TimeOutValue*1000;

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

    io = ioctl(fd, SG_IO, &cmd);

    out_pkt->ScsiStatus         = cmd.status;
    out_pkt->DataTransferLength = in_pkt->DataTransferLength - cmd.resid;
    out_pkt->SenseInfoLength    = cmd.sb_len_wr;

    ret = CDROM_GetStatusCode(io);

#elif defined(__APPLE__)

    memset(&cmd, 0, sizeof(cmd));
    memcpy(cmd.cdb, in_pkt->Cdb, in_pkt->CdbLength);

    cmd.cdbSize        = in_pkt->CdbLength;
    cmd.buffer         = in_pkt->DataBuffer;
    cmd.bufferSize     = in_pkt->DataTransferLength;
    cmd.sense          = (char*)out_pkt + in_pkt->SenseInfoOffset;
    cmd.senseLen       = in_pkt->SenseInfoLength;
    cmd.timeout        = in_pkt->TimeOutValue*1000; /* in milliseconds */

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

    io = ioctl(fd, DKIOCSCSICOMMAND, &cmd);

    if (cmd.response == kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE)
    {
        /* Command failed */
        switch (cmd.status)
        {
        case kSCSITaskStatus_TaskTimeoutOccurred:     return STATUS_TIMEOUT;
        case kSCSITaskStatus_ProtocolTimeoutOccurred: return STATUS_IO_TIMEOUT;
        case kSCSITaskStatus_DeviceNotResponding:     return STATUS_DEVICE_BUSY;
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

    ret = CDROM_GetStatusCode(io);

    /* FIXME: Update DataTransferLength and SenseInfoLength */
#endif
    if (ret == STATUS_SUCCESS)
    {
        out_pkt->Length = sizeof(*out_pkt);
        if (out_pkt != in_pkt)
        {
            out_pkt->CdbLength       = in_pkt->CdbLength;
            out_pkt->DataIn          = in_pkt->DataIn;
            out_pkt->TimeOutValue    = in_pkt->TimeOutValue;
            out_pkt->DataBuffer      = in_pkt->DataBuffer;
            out_pkt->SenseInfoOffset = in_pkt->SenseInfoOffset;
            memcpy(out_pkt->Cdb, in_pkt->Cdb, sizeof(in_pkt->Cdb));
        }
    }

    return ret;
}

/******************************************************************
 *              CDROM_ScsiPassThrough
 *              Implements IOCTL_SCSI_PASS_THROUGH
 *
 */
static NTSTATUS CDROM_ScsiPassThrough(int fd, const SCSI_PASS_THROUGH *in_pkt, SCSI_PASS_THROUGH *out_pkt)
{
    int ret = STATUS_NOT_SUPPORTED;
#ifdef HAVE_SG_IO_HDR_T_INTERFACE_ID
    sg_io_hdr_t cmd;
    int io;
#elif defined __APPLE__
    dk_scsi_command_t cmd;
    int io;
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
    RtlZeroMemory(&cmd, sizeof(cmd));

    cmd.interface_id   = 'S';
    cmd.dxfer_len      = in_pkt->DataTransferLength;
    cmd.cmd_len        = in_pkt->CdbLength;
    cmd.cmdp           = (unsigned char*)in_pkt->Cdb;
    cmd.mx_sb_len      = in_pkt->SenseInfoLength;
    cmd.timeout        = in_pkt->TimeOutValue*1000;

    if(cmd.mx_sb_len > 0)
        cmd.sbp = (unsigned char*)out_pkt + in_pkt->SenseInfoOffset;

    switch (in_pkt->DataIn)
    {
    case SCSI_IOCTL_DATA_IN:
        cmd.dxferp = (char*)out_pkt + in_pkt->DataBufferOffset;
        cmd.dxfer_direction = SG_DXFER_FROM_DEV;
        break;
    case SCSI_IOCTL_DATA_OUT:
        cmd.dxferp = (char*)in_pkt + in_pkt->DataBufferOffset;
        cmd.dxfer_direction = SG_DXFER_TO_DEV;
        break;
    case SCSI_IOCTL_DATA_UNSPECIFIED:
        cmd.dxfer_direction = SG_DXFER_NONE;
        break;
    default:
       return STATUS_INVALID_PARAMETER;
    }

    io = ioctl(fd, SG_IO, &cmd);

    out_pkt->ScsiStatus         = cmd.status;
    out_pkt->DataTransferLength = in_pkt->DataTransferLength - cmd.resid;
    out_pkt->SenseInfoLength    = cmd.sb_len_wr;

    ret = CDROM_GetStatusCode(io);

#elif defined(__APPLE__)

    memset(&cmd, 0, sizeof(cmd));
    memcpy(cmd.cdb, in_pkt->Cdb, in_pkt->CdbLength);

    cmd.cdbSize        = in_pkt->CdbLength;
    cmd.bufferSize     = in_pkt->DataTransferLength;
    cmd.sense          = (char*)out_pkt + in_pkt->SenseInfoOffset;
    cmd.senseLen       = in_pkt->SenseInfoLength;
    cmd.timeout        = in_pkt->TimeOutValue*1000; /* in milliseconds */

    switch (in_pkt->DataIn)
    {
    case SCSI_IOCTL_DATA_OUT:
        cmd.buffer = (char*)in_pkt + in_pkt->DataBufferOffset;
        cmd.direction = kSCSIDataTransfer_FromInitiatorToTarget;
	break;
    case SCSI_IOCTL_DATA_IN:
        cmd.buffer = (char*)out_pkt + in_pkt->DataBufferOffset;
        cmd.direction = kSCSIDataTransfer_FromTargetToInitiator;
	break;
    case SCSI_IOCTL_DATA_UNSPECIFIED:
        cmd.direction = kSCSIDataTransfer_NoDataTransfer;
	break;
    default:
       return STATUS_INVALID_PARAMETER;
    }

    io = ioctl(fd, DKIOCSCSICOMMAND, &cmd);

    if (cmd.response == kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE)
    {
        /* Command failed */
        switch (cmd.status)
        {
        case kSCSITaskStatus_TaskTimeoutOccurred:
            return STATUS_TIMEOUT;
            break;
        case kSCSITaskStatus_ProtocolTimeoutOccurred:
            return STATUS_IO_TIMEOUT;
            break;
        case kSCSITaskStatus_DeviceNotResponding:
            return STATUS_DEVICE_BUSY;
            break;
        case kSCSITaskStatus_DeviceNotPresent:
            return STATUS_NO_SUCH_DEVICE;
            break;
        case kSCSITaskStatus_DeliveryFailure:
            return STATUS_DEVICE_PROTOCOL_ERROR;
            break;
        case kSCSITaskStatus_No_Status:
        default:
            return STATUS_UNSUCCESSFUL;
            break;
        }
    }

    if (cmd.status != kSCSITaskStatus_No_Status)
        out_pkt->ScsiStatus = cmd.status;

    ret = CDROM_GetStatusCode(io);

    /* FIXME: Update DataTransferLength and SenseInfoLength */
#endif
    if (ret == STATUS_SUCCESS)
    {
        out_pkt->Length = sizeof(*out_pkt);
        if (out_pkt != in_pkt)
        {
            out_pkt->CdbLength        = in_pkt->CdbLength;
            out_pkt->DataIn           = in_pkt->DataIn;
            out_pkt->TimeOutValue     = in_pkt->TimeOutValue;
            out_pkt->DataBufferOffset = in_pkt->DataBufferOffset;
            out_pkt->SenseInfoOffset  = in_pkt->SenseInfoOffset;
            memcpy(out_pkt->Cdb, in_pkt->Cdb, sizeof(in_pkt->Cdb));
        }
    }

    return ret;
}

static NTSTATUS CDROM_ScsiPassThrough32(int fd, const SCSI_PASS_THROUGH32 *in_pkt32, SCSI_PASS_THROUGH32 *out_pkt32)
{
    SCSI_PASS_THROUGH *pkt;
    ULONG_PTR ptr;
    NTSTATUS ret;

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

    pkt = calloc(1, sizeof(SCSI_PASS_THROUGH) + in_pkt32->SenseInfoLength + in_pkt32->DataTransferLength);
    if (!pkt) return STATUS_NO_MEMORY;

    pkt->Length = sizeof(SCSI_PASS_THROUGH);
    pkt->CdbLength = in_pkt32->CdbLength;
    pkt->SenseInfoLength = in_pkt32->SenseInfoLength;
    pkt->DataIn = in_pkt32->DataIn;
    pkt->DataTransferLength = in_pkt32->DataTransferLength;
    pkt->TimeOutValue = in_pkt32->TimeOutValue;
    pkt->DataBufferOffset = sizeof(SCSI_PASS_THROUGH) + in_pkt32->SenseInfoLength;
    pkt->SenseInfoOffset = sizeof(SCSI_PASS_THROUGH);
    memcpy(pkt->Cdb, in_pkt32->Cdb, sizeof(pkt->Cdb));
    if (pkt->DataIn == SCSI_IOCTL_DATA_OUT)
        memcpy((char*)pkt + pkt->DataBufferOffset,
                (const char*)in_pkt32 + in_pkt32->DataBufferOffset,
                in_pkt32->DataTransferLength);

    ret = CDROM_ScsiPassThrough(fd, pkt, pkt);
    if (NT_ERROR(ret)) goto done;

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
    memcpy((char*)out_pkt32 + out_pkt32->SenseInfoOffset,
            (const char*)pkt + pkt->SenseInfoOffset,
            pkt->SenseInfoLength);
    if (pkt->DataIn == SCSI_IOCTL_DATA_IN)
        memcpy((char*)out_pkt32 + out_pkt32->DataBufferOffset,
                (const char*)pkt + pkt->DataBufferOffset,
                pkt->DataTransferLength);

done:
    free(pkt);
    return ret;
}

/******************************************************************
 *		CDROM_ScsiGetCaps
 *
 *
 */
static NTSTATUS CDROM_ScsiGetCaps(int fd, PIO_SCSI_CAPABILITIES caps)
{
    NTSTATUS    ret = STATUS_NOT_IMPLEMENTED;

#ifdef SG_SCATTER_SZ
    caps->Length = sizeof(*caps);
    caps->MaximumTransferLength = SG_SCATTER_SZ; /* FIXME */
    caps->MaximumPhysicalPages = SG_SCATTER_SZ / page_size;
    caps->SupportedAsynchronousEvents = TRUE;
    caps->AlignmentMask = page_size;
    caps->TaggedQueuing = FALSE; /* we could check that it works and answer TRUE */
    caps->AdapterScansDown = FALSE; /* FIXME ? */
    caps->AdapterUsesPio = FALSE; /* FIXME ? */
    ret = STATUS_SUCCESS;
#elif defined __APPLE__
    uint64_t bytesr, bytesw, align;
    int io = ioctl(fd, DKIOCGETMAXBYTECOUNTREAD, &bytesr);
    if (io != 0) return CDROM_GetStatusCode(io);
    io = ioctl(fd, DKIOCGETMAXBYTECOUNTWRITE, &bytesw);
    if (io != 0) return CDROM_GetStatusCode(io);
    io = ioctl(fd, DKIOCGETMINSEGMENTALIGNMENTBYTECOUNT, &align);
    if (io != 0) return CDROM_GetStatusCode(io);
    caps->Length = sizeof(*caps);
    caps->MaximumTransferLength = bytesr < bytesw ? bytesr : bytesw;
    caps->MaximumPhysicalPages = caps->MaximumTransferLength / page_size;
    caps->SupportedAsynchronousEvents = TRUE;
    caps->AlignmentMask = align-1;
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
static NTSTATUS CDROM_GetAddress(int fd, SCSI_ADDRESS* address)
{
    UCHAR portnum, busid, targetid, lun;

    address->Length = sizeof(SCSI_ADDRESS);
    if ( ! CDROM_GetInterfaceInfo(fd, &portnum, &busid, &targetid, &lun))
        return STATUS_NOT_SUPPORTED;

    address->PortNumber = portnum; /* primary=0 secondary=1 for ide */
    address->PathId = busid;       /* always 0 for ide */
    address->TargetId = targetid;  /* device id 0/1 for ide */
    address->Lun = lun;
    return STATUS_SUCCESS;
}

/******************************************************************
 *		DVD_StartSession
 *
 *
 */
static NTSTATUS DVD_StartSession(int fd, const DVD_SESSION_ID *sid_in, PDVD_SESSION_ID sid_out)
{
#if defined(linux)
    NTSTATUS ret = STATUS_NOT_SUPPORTED;
    dvd_authinfo auth_info;

    memset( &auth_info, 0, sizeof( auth_info ) );
    auth_info.type = DVD_LU_SEND_AGID;
    if (sid_in) auth_info.lsa.agid = *(const int*)sid_in; /* ?*/

    TRACE("fd 0x%08x\n",fd);
    ret =CDROM_GetStatusCode(ioctl(fd, DVD_AUTH, &auth_info));
    *sid_out = auth_info.lsa.agid;
    return ret;
#elif defined(__APPLE__)
    NTSTATUS ret = STATUS_NOT_SUPPORTED;
    dk_dvd_report_key_t dvdrk;
    DVDAuthenticationGrantIDInfo agid_info;

    dvdrk.format = kDVDKeyFormatAGID_CSS;
    dvdrk.keyClass = kDVDKeyClassCSS_CPPM_CPRM;
    if(sid_in) dvdrk.grantID = *(uint8_t*)sid_in; /* ? */
    dvdrk.bufferLength = sizeof(DVDAuthenticationGrantIDInfo);
    dvdrk.buffer = &agid_info;

    ret = CDROM_GetStatusCode(ioctl(fd, DKIOCDVDREPORTKEY, &dvdrk));
    *sid_out = agid_info.grantID;
    return ret;
#else
    FIXME("not supported on this O/S\n");
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *		DVD_EndSession
 *
 *
 */
static NTSTATUS DVD_EndSession(int fd, const DVD_SESSION_ID *sid)
{
#if defined(linux)
    dvd_authinfo auth_info;

    memset( &auth_info, 0, sizeof( auth_info ) );
    auth_info.type = DVD_INVALIDATE_AGID;
    auth_info.lsa.agid = *(const int*)sid;

    TRACE("\n");
    return CDROM_GetStatusCode(ioctl(fd, DVD_AUTH, &auth_info));
#elif defined(__APPLE__)
    dk_dvd_send_key_t dvdsk;

    dvdsk.format = kDVDKeyFormatAGID_Invalidate;
    dvdsk.keyClass = kDVDKeyClassCSS_CPPM_CPRM;
    dvdsk.grantID = (uint8_t)*sid;

    return CDROM_GetStatusCode(ioctl(fd, DKIOCDVDSENDKEY, &dvdsk));
#else
    FIXME("not supported on this O/S\n");
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *		DVD_SendKey
 *
 *
 */
static NTSTATUS DVD_SendKey(int fd, const DVD_COPY_PROTECT_KEY *key)
{
#if defined(linux)
    NTSTATUS ret = STATUS_NOT_SUPPORTED;
    dvd_authinfo auth_info;

    memset( &auth_info, 0, sizeof( auth_info ) );
    switch (key->KeyType)
    {
    case DvdChallengeKey:
	auth_info.type = DVD_HOST_SEND_CHALLENGE;
	auth_info.hsc.agid = (int)key->SessionId;
	TRACE("DvdChallengeKey ioc 0x%x\n", DVD_AUTH );
	memcpy( auth_info.hsc.chal, key->KeyData, DVD_CHALLENGE_SIZE );
	ret = CDROM_GetStatusCode(ioctl( fd, DVD_AUTH, &auth_info ));
	break;
    case DvdBusKey2:
	auth_info.type = DVD_HOST_SEND_KEY2;
	auth_info.hsk.agid = (int)key->SessionId;

	memcpy( auth_info.hsk.key, key->KeyData, DVD_KEY_SIZE );

	TRACE("DvdBusKey2\n");
	ret = CDROM_GetStatusCode(ioctl( fd, DVD_AUTH, &auth_info ));
	break;

    default:
	FIXME("Unknown Keytype 0x%x\n",key->KeyType);
    }
    return ret;
#elif defined(__APPLE__)
    dk_dvd_send_key_t dvdsk;
    union
    {
        DVDChallengeKeyInfo chal;
        DVDKey2Info key2;
    } key_desc;

    switch(key->KeyType)
    {
    case DvdChallengeKey:
        dvdsk.format = kDVDKeyFormatChallengeKey;
        dvdsk.bufferLength = sizeof(key_desc.chal);
        dvdsk.buffer = &key_desc.chal;
        OSWriteBigInt16(key_desc.chal.dataLength, 0, key->KeyLength);
        memcpy(key_desc.chal.challengeKeyValue, key->KeyData, key->KeyLength);
        break;
    case DvdBusKey2:
        dvdsk.format = kDVDKeyFormatKey2;
        dvdsk.bufferLength = sizeof(key_desc.key2);
        dvdsk.buffer = &key_desc.key2;
        OSWriteBigInt16(key_desc.key2.dataLength, 0, key->KeyLength);
        memcpy(key_desc.key2.key2Value, key->KeyData, key->KeyLength);
        break;
    case DvdInvalidateAGID:
        dvdsk.format = kDVDKeyFormatAGID_Invalidate;
        break;
    case DvdBusKey1:
    case DvdTitleKey:
    case DvdAsf:
    case DvdGetRpcKey:
    case DvdDiskKey:
        ERR("attempted to write read-only key type 0x%x\n", key->KeyType);
        return STATUS_NOT_SUPPORTED;
    case DvdSetRpcKey:
        FIXME("DvdSetRpcKey NIY\n");
        return STATUS_NOT_SUPPORTED;
    default:
        FIXME("got unknown key type 0x%x\n", key->KeyType);
        return STATUS_NOT_SUPPORTED;
    }
    dvdsk.keyClass = kDVDKeyClassCSS_CPPM_CPRM;
    dvdsk.grantID = (uint8_t)key->SessionId;

    return CDROM_GetStatusCode(ioctl(fd, DKIOCDVDSENDKEY, &dvdsk));
#else
    FIXME("not supported on this O/S\n");
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *		DVD_ReadKey
 *
 *
 */
static NTSTATUS DVD_ReadKey(int fd, PDVD_COPY_PROTECT_KEY key)
{
#if defined(linux)
    NTSTATUS ret = STATUS_NOT_SUPPORTED;
    dvd_struct dvd;
    dvd_authinfo auth_info;

    memset( &dvd, 0, sizeof( dvd_struct ) );
    memset( &auth_info, 0, sizeof( auth_info ) );
    switch (key->KeyType)
    {
    case DvdDiskKey:

	dvd.type = DVD_STRUCT_DISCKEY;
	dvd.disckey.agid = (int)key->SessionId;
	memset( dvd.disckey.value, 0, DVD_DISCKEY_SIZE );

	TRACE("DvdDiskKey\n");
	ret = CDROM_GetStatusCode(ioctl( fd, DVD_READ_STRUCT, &dvd ));
	if (ret == STATUS_SUCCESS)
	    memcpy(key->KeyData,dvd.disckey.value,DVD_DISCKEY_SIZE);
	break;
    case DvdTitleKey:
	auth_info.type = DVD_LU_SEND_TITLE_KEY;
	auth_info.lstk.agid = (int)key->SessionId;
	auth_info.lstk.lba = (int)(key->Parameters.TitleOffset.QuadPart>>11);
	TRACE("DvdTitleKey session %d Quadpart %s offset 0x%08x\n",
	      key->SessionId, wine_dbgstr_longlong(key->Parameters.TitleOffset.QuadPart),
	      auth_info.lstk.lba);
	ret = CDROM_GetStatusCode(ioctl( fd, DVD_AUTH, &auth_info ));
	if (ret == STATUS_SUCCESS)
	    memcpy(key->KeyData, auth_info.lstk.title_key, DVD_KEY_SIZE );
	break;
    case DvdChallengeKey:

	auth_info.type = DVD_LU_SEND_CHALLENGE;
	auth_info.lsc.agid = (int)key->SessionId;

	TRACE("DvdChallengeKey\n");
	ret = CDROM_GetStatusCode(ioctl( fd, DVD_AUTH, &auth_info ));
	if (ret == STATUS_SUCCESS)
	    memcpy( key->KeyData, auth_info.lsc.chal, DVD_CHALLENGE_SIZE );
	break;
    case DvdAsf:
	auth_info.type = DVD_LU_SEND_ASF;
	TRACE("DvdAsf\n");
	auth_info.lsasf.asf=((PDVD_ASF)key->KeyData)->SuccessFlag;
	ret = CDROM_GetStatusCode(ioctl( fd, DVD_AUTH, &auth_info ));
	((PDVD_ASF)key->KeyData)->SuccessFlag = auth_info.lsasf.asf;
	break;
    case DvdBusKey1:
	auth_info.type = DVD_LU_SEND_KEY1;
	auth_info.lsk.agid = (int)key->SessionId;

	TRACE("DvdBusKey1\n");
	ret = CDROM_GetStatusCode(ioctl( fd, DVD_AUTH, &auth_info ));

	if (ret == STATUS_SUCCESS)
	    memcpy( key->KeyData, auth_info.lsk.key, DVD_KEY_SIZE );
	break;
    case DvdGetRpcKey:
	auth_info.type = DVD_LU_SEND_RPC_STATE;

	TRACE("DvdGetRpcKey\n");
	ret = CDROM_GetStatusCode(ioctl( fd, DVD_AUTH, &auth_info ));

	if (ret == STATUS_SUCCESS)
	{
	    ((PDVD_RPC_KEY)key->KeyData)->TypeCode = auth_info.lrpcs.type;
	    ((PDVD_RPC_KEY)key->KeyData)->RegionMask = auth_info.lrpcs.region_mask;
	    ((PDVD_RPC_KEY)key->KeyData)->RpcScheme = auth_info.lrpcs.rpc_scheme;
	    ((PDVD_RPC_KEY)key->KeyData)->UserResetsAvailable = auth_info.lrpcs.ucca;
	    ((PDVD_RPC_KEY)key->KeyData)->ManufacturerResetsAvailable = auth_info.lrpcs.vra;
	}
	break;
    default:
	FIXME("Unknown keytype 0x%x\n",key->KeyType);
    }
    return ret;
#elif defined(__APPLE__)
    union
    {
        dk_dvd_report_key_t key;
        dk_dvd_read_structure_t disk_key;
    } ioc;
    union
    {
        DVDDiscKeyInfo disk_key;
        DVDChallengeKeyInfo chal;
        DVDKey1Info key1;
        DVDTitleKeyInfo title;
        DVDAuthenticationSuccessFlagInfo asf;
        DVDRegionPlaybackControlInfo rpc;
    } desc;
    NTSTATUS ret = STATUS_NOT_SUPPORTED;

    switch(key->KeyType)
    {
    case DvdChallengeKey:
        ioc.key.format = kDVDKeyFormatChallengeKey;
        ioc.key.grantID = (uint8_t)key->SessionId;
        ioc.key.keyClass = kDVDKeyClassCSS_CPPM_CPRM;
        ioc.key.bufferLength = sizeof(desc.chal);
        ioc.key.buffer = &desc.chal;
        OSWriteBigInt16(desc.chal.dataLength, 0, key->KeyLength);
        break;
    case DvdBusKey1:
        ioc.key.format = kDVDKeyFormatKey1;
        ioc.key.grantID = (uint8_t)key->SessionId;
        ioc.key.keyClass = kDVDKeyClassCSS_CPPM_CPRM;
        ioc.key.bufferLength = sizeof(desc.key1);
        ioc.key.buffer = &desc.key1;
        OSWriteBigInt16(desc.key1.dataLength, 0, key->KeyLength);
        break;
    case DvdTitleKey:
        ioc.key.format = kDVDKeyFormatTitleKey;
        ioc.key.grantID = (uint8_t)key->SessionId;
        ioc.key.keyClass = kDVDKeyClassCSS_CPPM_CPRM;
        ioc.key.bufferLength = sizeof(desc.title);
        ioc.key.buffer = &desc.title;
        ioc.key.address = (uint32_t)(key->Parameters.TitleOffset.QuadPart>>11);
        OSWriteBigInt16(desc.title.dataLength, 0, key->KeyLength);
        break;
    case DvdAsf:
        ioc.key.format = kDVDKeyFormatASF;
        ioc.key.grantID = (uint8_t)key->SessionId;
        ioc.key.keyClass = kDVDKeyClassCSS_CPPM_CPRM;
        ioc.key.bufferLength = sizeof(desc.asf);
        ioc.key.buffer = &desc.asf;
        OSWriteBigInt16(desc.asf.dataLength, 0, key->KeyLength);
        break;
    case DvdGetRpcKey:
        ioc.key.format = kDVDKeyFormatRegionState;
        ioc.key.grantID = (uint8_t)key->SessionId;
        ioc.key.keyClass = kDVDKeyClassCSS_CPPM_CPRM;
        ioc.key.bufferLength = sizeof(desc.rpc);
        ioc.key.buffer = &desc.rpc;
        OSWriteBigInt16(desc.rpc.dataLength, 0, key->KeyLength);
        break;
    case DvdDiskKey:
        ioc.disk_key.format = kDVDStructureFormatDiscKeyInfo;
        ioc.disk_key.grantID = (uint8_t)key->SessionId;
        ioc.disk_key.bufferLength = sizeof(desc.disk_key);
        ioc.disk_key.buffer = &desc.disk_key;
        break;
    case DvdInvalidateAGID:
        ioc.key.format = kDVDKeyFormatAGID_Invalidate;
        ioc.key.grantID = (uint8_t)key->SessionId;
        ioc.key.keyClass = kDVDKeyClassCSS_CPPM_CPRM;
        break;
    case DvdBusKey2:
    case DvdSetRpcKey:
        ERR("attempted to read write-only key type 0x%x\n", key->KeyType);
        return STATUS_NOT_SUPPORTED;
    default:
        FIXME("got unknown key type 0x%x\n", key->KeyType);
        return STATUS_NOT_SUPPORTED;
    }

    ret = CDROM_GetStatusCode(ioctl(fd, (key->KeyType == DvdDiskKey ? DKIOCDVDREADSTRUCTURE : DKIOCDVDREPORTKEY), &ioc));

    if (ret == STATUS_SUCCESS)
    {
        switch(key->KeyType)
        {
        case DvdChallengeKey:
            key->KeyLength = OSReadBigInt16(desc.chal.dataLength, 0);
            memcpy(key->KeyData, desc.chal.challengeKeyValue, key->KeyLength);
            break;
        case DvdBusKey1:
            key->KeyLength = OSReadBigInt16(desc.key1.dataLength, 0);
            memcpy(key->KeyData, desc.key1.key1Value, key->KeyLength);
            break;
        case DvdTitleKey:
            key->KeyLength = OSReadBigInt16(desc.title.dataLength, 0);
            memcpy(key->KeyData, desc.title.titleKeyValue, key->KeyLength);
            key->KeyFlags = 0;
            if (desc.title.CPM)
            {
                /*key->KeyFlags |= DVD_COPYRIGHTED;*/
                if (desc.title.CP_SEC) key->KeyFlags |= DVD_SECTOR_PROTECTED;
                /*else key->KeyFlags |= DVD_SECTOR_NOT_PROTECTED;*/
#if 0
                switch (desc.title.CGMS)
                {
                case 0:
                    key->KeyFlags |= DVD_CGMS_COPY_PERMITTED;
                    break;
                case 2:
                    key->KeyFlags |= DVD_CGMS_COPY_ONCE;
                    break;
                case 3:
                    key->KeyFlags |= DVD_CGMS_NO_COPY;
                    break;
                }
#endif
            } /*else key->KeyFlags |= DVD_NOT_COPYRIGHTED;*/
            break;
        case DvdAsf:
            key->KeyLength = OSReadBigInt16(desc.title.dataLength, 0);
            ((PDVD_ASF)key->KeyData)->SuccessFlag = desc.asf.successFlag;
            break;
        case DvdGetRpcKey:
            key->KeyLength = OSReadBigInt16(desc.rpc.dataLength, 0);
            ((PDVD_RPC_KEY)key->KeyData)->UserResetsAvailable =
                desc.rpc.numberUserResets;
            ((PDVD_RPC_KEY)key->KeyData)->ManufacturerResetsAvailable =
                desc.rpc.numberVendorResets;
            ((PDVD_RPC_KEY)key->KeyData)->TypeCode =
                desc.rpc.typeCode;
            ((PDVD_RPC_KEY)key->KeyData)->RegionMask =
                desc.rpc.driveRegion;
            ((PDVD_RPC_KEY)key->KeyData)->RpcScheme =
                desc.rpc.rpcScheme;
        case DvdDiskKey:
            key->KeyLength = OSReadBigInt16(desc.disk_key.dataLength, 0);
            memcpy(key->KeyData, desc.disk_key.discKeyStructures, key->KeyLength);
            break;
        case DvdBusKey2:
        case DvdSetRpcKey:
        case DvdInvalidateAGID:
        default:
            /* Silence warning */
            ;
        }
    }
    return ret;
#else
    FIXME("not supported on this O/S\n");
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *		DVD_GetRegion
 *
 * This IOCTL combines information from both IOCTL_DVD_READ_KEY
 * with key type DvdGetRpcKey and IOCTL_DVD_READ_STRUCTURE with
 * structure type DvdCopyrightInformation into one structure.
 */
static NTSTATUS DVD_GetRegion(int fd, PDVD_REGION region)
{
#if defined(linux)
    NTSTATUS ret;
    dvd_struct dvd;
    dvd_authinfo auth_info;

    dvd.type = DVD_STRUCT_COPYRIGHT;
    dvd.copyright.layer_num = 0;
    auth_info.type = DVD_LU_SEND_RPC_STATE;

    ret = CDROM_GetStatusCode(ioctl( fd, DVD_AUTH, &auth_info ));

    if (ret == STATUS_SUCCESS)
    {
        ret = CDROM_GetStatusCode(ioctl( fd, DVD_READ_STRUCT, &dvd ));

        if (ret == STATUS_SUCCESS)
        {
            region->CopySystem = dvd.copyright.cpst;
            region->RegionData = dvd.copyright.rmi;
            region->SystemRegion = auth_info.lrpcs.region_mask;
            region->ResetCount = auth_info.lrpcs.ucca;
        }
    }
    return ret;
#elif defined(__APPLE__)
    dk_dvd_report_key_t key;
    dk_dvd_read_structure_t dvd;
    DVDRegionPlaybackControlInfo rpc;
    DVDCopyrightInfo copy;
    NTSTATUS ret;

    key.format = kDVDKeyFormatRegionState;
    key.keyClass = kDVDKeyClassCSS_CPPM_CPRM;
    key.bufferLength = sizeof(rpc);
    key.buffer = &rpc;
    dvd.format = kDVDStructureFormatCopyrightInfo;
    dvd.bufferLength = sizeof(copy);
    dvd.buffer = &copy;

    ret = CDROM_GetStatusCode(ioctl( fd, DKIOCDVDREPORTKEY, &key ));

    if (ret == STATUS_SUCCESS)
    {
        ret = CDROM_GetStatusCode(ioctl( fd, DKIOCDVDREADSTRUCTURE, &dvd ));

        if (ret == STATUS_SUCCESS)
        {
            region->CopySystem = copy.copyrightProtectionSystemType;
            region->RegionData = copy.regionMask;
            region->SystemRegion = rpc.driveRegion;
            region->ResetCount = rpc.numberUserResets;
        }
    }
    return ret;
#else
    FIXME("not supported on this O/S\n");
    return STATUS_NOT_SUPPORTED;
#endif
}

static DWORD DVD_ReadStructureSize(const DVD_READ_STRUCTURE *structure, DWORD size)
{
    if (!structure || size != sizeof(DVD_READ_STRUCTURE))
        return 0;

    switch (structure->Format)
    {
    case DvdPhysicalDescriptor:
        return sizeof(DVD_LAYER_DESCRIPTOR);
    case DvdCopyrightDescriptor:
        return sizeof(DVD_COPYRIGHT_DESCRIPTOR);
    case DvdDiskKeyDescriptor:
        return sizeof(DVD_DISK_KEY_DESCRIPTOR);
    case DvdBCADescriptor:
        return sizeof(DVD_BCA_DESCRIPTOR);
    case DvdManufacturerDescriptor:
        return sizeof(DVD_MANUFACTURER_DESCRIPTOR);
    default:
        return 0;
    }
}

/******************************************************************
 *		DVD_ReadStructure
 *
 *
 */
static NTSTATUS DVD_ReadStructure(int dev, const DVD_READ_STRUCTURE *structure, PDVD_LAYER_DESCRIPTOR layer)
{
#ifdef DVD_READ_STRUCT
    /* dvd_struct is not defined consistently across platforms */
    union
    {
	struct dvd_physical	physical;
	struct dvd_copyright	copyright;
	struct dvd_disckey	disckey;
	struct dvd_bca		bca;
	struct dvd_manufact	manufact;
    } s;

    if (structure->BlockByteOffset.QuadPart) FIXME(": BlockByteOffset is not handled\n");

    switch (structure->Format)
    {
    case DvdPhysicalDescriptor:
        s.physical.type = DVD_STRUCT_PHYSICAL;
        s.physical.layer_num = structure->LayerNumber;
        break;

    case DvdCopyrightDescriptor:
        s.copyright.type = DVD_STRUCT_COPYRIGHT;
        s.copyright.layer_num = structure->LayerNumber;
        break;

    case DvdDiskKeyDescriptor:
        s.disckey.type = DVD_STRUCT_DISCKEY;
        s.disckey.agid = structure->SessionId;
        break;

    case DvdBCADescriptor:
        s.bca.type = DVD_STRUCT_BCA;
        break;

    case DvdManufacturerDescriptor:
        s.manufact.type = DVD_STRUCT_MANUFACT;
        s.manufact.layer_num = structure->LayerNumber;
        break;

    case DvdMaxDescriptor: /* This is not a real request, no matter what MSDN says! */
    default:
        return STATUS_INVALID_PARAMETER;
    }

    if (ioctl(dev, DVD_READ_STRUCT, &s) < 0)
       return STATUS_INVALID_PARAMETER;

    switch (structure->Format)
    {
    case DvdPhysicalDescriptor:
        {
            internal_dvd_layer_descriptor *p = (internal_dvd_layer_descriptor *) layer;
            struct dvd_layer *l = &s.physical.layer[s.physical.layer_num];

            p->Header.Length = 0x0802;
            p->Header.Reserved[0] = 0;
            p->Header.Reserved[1] = 0;
            p->Descriptor.BookVersion = l->book_version;
            p->Descriptor.BookType = l->book_type;
            p->Descriptor.MinimumRate = l->min_rate;
            p->Descriptor.DiskSize = l->disc_size;
            p->Descriptor.LayerType = l->layer_type;
            p->Descriptor.TrackPath = l->track_path;
            p->Descriptor.NumberOfLayers = l->nlayers;
            p->Descriptor.Reserved1 = 0;
            p->Descriptor.TrackDensity = l->track_density;
            p->Descriptor.LinearDensity = l->linear_density;
            p->Descriptor.StartingDataSector = GET_BE_DWORD(l->start_sector);
            p->Descriptor.EndDataSector = GET_BE_DWORD(l->end_sector);
            p->Descriptor.EndLayerZeroSector = GET_BE_DWORD(l->end_sector_l0);
            p->Descriptor.Reserved5 = 0;
            p->Descriptor.BCAFlag = l->bca;
        }
        break;

    case DvdCopyrightDescriptor:
        {
            PDVD_COPYRIGHT_DESCRIPTOR p = (PDVD_COPYRIGHT_DESCRIPTOR) layer;

            p->CopyrightProtectionType = s.copyright.cpst;
            p->RegionManagementInformation = s.copyright.rmi;
            p->Reserved = 0;
        }
        break;

    case DvdDiskKeyDescriptor:
        {
            PDVD_DISK_KEY_DESCRIPTOR p = (PDVD_DISK_KEY_DESCRIPTOR) layer;

            memcpy(p->DiskKeyData, s.disckey.value, 2048);
        }
        break;

    case DvdBCADescriptor:
        {
            PDVD_BCA_DESCRIPTOR p = (PDVD_BCA_DESCRIPTOR) layer;

            memcpy(p->BCAInformation, s.bca.value, s.bca.len);
        }
        break;

    case DvdManufacturerDescriptor:
        {
            internal_dvd_manufacturer_descriptor *p = (internal_dvd_manufacturer_descriptor*) layer;

            p->Header.Length = 0x0802;
            p->Header.Reserved[0] = 0;
            p->Header.Reserved[1] = 0;
            memcpy(p->Descriptor.ManufacturingInformation, s.manufact.value, 2048);
        }
        break;

    case DvdMaxDescriptor: /* Suppress warning */
	break;
    }
#elif defined(__APPLE__)
    NTSTATUS ret;
    dk_dvd_read_structure_t dvdrs;
    union
    {
        DVDPhysicalFormatInfo phys;
        DVDCopyrightInfo copy;
        DVDDiscKeyInfo disk_key;
        DVDManufacturingInfo manf;
    } desc;
    union
    {
        PDVD_LAYER_DESCRIPTOR layer;
        internal_dvd_layer_descriptor *xlayer;
        PDVD_COPYRIGHT_DESCRIPTOR copy;
        PDVD_DISK_KEY_DESCRIPTOR disk_key;
        internal_dvd_manufacturer_descriptor *manf;
    } nt_desc;

    nt_desc.layer = layer;
    RtlZeroMemory(&dvdrs, sizeof(dvdrs));
    dvdrs.address = (uint32_t)(structure->BlockByteOffset.QuadPart>>11);
    dvdrs.grantID = (uint8_t)structure->SessionId;
    dvdrs.layer = structure->LayerNumber;
    switch(structure->Format)
    {
    case DvdPhysicalDescriptor:
        dvdrs.format = kDVDStructureFormatPhysicalFormatInfo;
        dvdrs.bufferLength = sizeof(desc.phys);
        dvdrs.buffer = &desc.phys;
        break;

    case DvdCopyrightDescriptor:
        dvdrs.format = kDVDStructureFormatCopyrightInfo;
        dvdrs.bufferLength = sizeof(desc.copy);
        dvdrs.buffer = &desc.copy;
        break;

    case DvdDiskKeyDescriptor:
        dvdrs.format = kDVDStructureFormatDiscKeyInfo;
        dvdrs.bufferLength = sizeof(desc.disk_key);
        dvdrs.buffer = &desc.disk_key;
        break;

    case DvdBCADescriptor:
        FIXME("DvdBCADescriptor NIY\n");
        return STATUS_NOT_SUPPORTED;

    case DvdManufacturerDescriptor:
        dvdrs.format = kDVDStructureFormatManufacturingInfo;
        dvdrs.bufferLength = sizeof(desc.manf);
        dvdrs.buffer = &desc.manf;
        break;

    case DvdMaxDescriptor:
    default:
        FIXME("got unknown structure type 0x%x\n", structure->Format);
        return STATUS_NOT_SUPPORTED;
    }
    ret = CDROM_GetStatusCode(ioctl(dev, DKIOCDVDREADSTRUCTURE, &dvdrs));
    if(ret == STATUS_SUCCESS)
    {
        switch(structure->Format)
        {
        case DvdPhysicalDescriptor:
            nt_desc.xlayer->Header.Length = 0x0802;
            nt_desc.xlayer->Header.Reserved[0] = 0;
            nt_desc.xlayer->Header.Reserved[1] = 0;
            nt_desc.xlayer->Descriptor.BookVersion = desc.phys.partVersion;
            nt_desc.xlayer->Descriptor.BookType = desc.phys.bookType;
            nt_desc.xlayer->Descriptor.MinimumRate = desc.phys.minimumRate;
            nt_desc.xlayer->Descriptor.DiskSize = desc.phys.discSize;
            nt_desc.xlayer->Descriptor.LayerType = desc.phys.layerType;
            nt_desc.xlayer->Descriptor.TrackPath = desc.phys.trackPath;
            nt_desc.xlayer->Descriptor.NumberOfLayers = desc.phys.numberOfLayers;
            nt_desc.xlayer->Descriptor.Reserved1 = 0;
            nt_desc.xlayer->Descriptor.TrackDensity = desc.phys.trackDensity;
            nt_desc.xlayer->Descriptor.LinearDensity = desc.phys.linearDensity;
            nt_desc.xlayer->Descriptor.StartingDataSector = GET_BE_DWORD(*(DWORD *)&desc.phys.zero1);
            nt_desc.xlayer->Descriptor.EndDataSector = GET_BE_DWORD(*(DWORD *)&desc.phys.zero2);
            nt_desc.xlayer->Descriptor.EndLayerZeroSector = GET_BE_DWORD(*(DWORD *)&desc.phys.zero3);
            nt_desc.xlayer->Descriptor.Reserved5 = 0;
            nt_desc.xlayer->Descriptor.BCAFlag = desc.phys.bcaFlag;
            break;

        case DvdCopyrightDescriptor:
            nt_desc.copy->CopyrightProtectionType =
                desc.copy.copyrightProtectionSystemType;
            nt_desc.copy->RegionManagementInformation =
                desc.copy.regionMask;
            nt_desc.copy->Reserved = 0;
            break;

        case DvdDiskKeyDescriptor:
            memcpy(
                nt_desc.disk_key->DiskKeyData,
                desc.disk_key.discKeyStructures,
                2048);
            break;

        case DvdManufacturerDescriptor:
            nt_desc.manf->Header.Length = 0x0802;
            nt_desc.manf->Header.Reserved[0] = 0;
            nt_desc.manf->Header.Reserved[1] = 0;
            memcpy(
                nt_desc.manf->Descriptor.ManufacturingInformation,
                desc.manf.discManufacturingInfo,
                2048);
            break;

        case DvdBCADescriptor:
        case DvdMaxDescriptor:
        default:
            /* Silence warning */
            break;
        }
    }
    return ret;
#else
    FIXME("\n");
#endif
    return STATUS_SUCCESS;

}

/******************************************************************
 *        GetInquiryData
 *        Implements the IOCTL_GET_INQUIRY_DATA ioctl.
 *        Returns Inquiry data for all devices on the specified scsi bus
 *        Returns STATUS_BUFFER_TOO_SMALL if the output buffer is too small,
 *        STATUS_INVALID_DEVICE_REQUEST if the given handle isn't to a SCSI device,
 *        or STATUS_NOT_SUPPORTED if the OS driver is too old
 */
static NTSTATUS GetInquiryData(int fd, PSCSI_ADAPTER_BUS_INFO BufferOut, DWORD OutBufferSize)
{
#ifdef HAVE_SG_IO_HDR_T_INTERFACE_ID
    PSCSI_INQUIRY_DATA pInquiryData;
    UCHAR sense_buffer[32];
    int iochk, version;
    sg_io_hdr_t iocmd;
    UCHAR inquiry[INQ_CMD_LEN] = {INQUIRY, 0, 0, 0, INQ_REPLY_LEN, 0};

    /* Check we have a SCSI device and a supported driver */
    if(ioctl(fd, SG_GET_VERSION_NUM, &version) != 0)
    {
        WARN("IOCTL_SCSI_GET_INQUIRY_DATA sg driver is not loaded\n");
        return STATUS_INVALID_DEVICE_REQUEST;
    }
    if(version < 30000 )
        return STATUS_NOT_SUPPORTED;

    /* FIXME: Enumerate devices on the bus */
    BufferOut->NumberOfBuses = 1;
    BufferOut->BusData[0].NumberOfLogicalUnits = 1;
    BufferOut->BusData[0].InquiryDataOffset = sizeof(SCSI_ADAPTER_BUS_INFO);

    pInquiryData = (PSCSI_INQUIRY_DATA)(BufferOut + 1);

    RtlZeroMemory(&iocmd, sizeof(iocmd));
    iocmd.interface_id = 'S';
    iocmd.cmd_len = sizeof(inquiry);
    iocmd.mx_sb_len = sizeof(sense_buffer);
    iocmd.dxfer_direction = SG_DXFER_FROM_DEV;
    iocmd.dxfer_len = INQ_REPLY_LEN;
    iocmd.dxferp = pInquiryData->InquiryData;
    iocmd.cmdp = inquiry;
    iocmd.sbp = sense_buffer;
    iocmd.timeout = 1000;

    iochk = ioctl(fd, SG_IO, &iocmd);
    if(iochk != 0)
        WARN("ioctl SG_IO returned %d, error (%s)\n", iochk, strerror(errno));

    CDROM_GetInterfaceInfo(fd, &BufferOut->BusData[0].InitiatorBusId, &pInquiryData->PathId, &pInquiryData->TargetId, &pInquiryData->Lun);
    pInquiryData->DeviceClaimed = TRUE;
    pInquiryData->InquiryDataLength = INQ_REPLY_LEN;
    pInquiryData->NextInquiryDataOffset = 0;
    return STATUS_SUCCESS;
#else
    FIXME("not supported on this O/S\n");
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *		cdrom_DeviceIoControl
 */
NTSTATUS cdrom_DeviceIoControl( HANDLE device, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user,
                                IO_STATUS_BLOCK *io, UINT code, void *in_buffer,
                                UINT in_size, void *out_buffer, UINT out_size )
{
    DWORD       sz = 0;
    NTSTATUS    status = STATUS_SUCCESS;
    int fd, needs_close, dev = 0;
    unsigned int options;

    TRACE( "%p %s %p %d %p %d %p\n", device, iocodex(code), in_buffer, in_size, out_buffer, out_size, io );

    if ((status = server_get_unix_fd( device, 0, &fd, &needs_close, NULL, &options )))
        return status;

    if ((status = CDROM_Open(fd, &dev)))
    {
        if (needs_close) close( fd );
        return status;
    }

#ifdef __APPLE__
    {
        char name[100];

        /* This is ugly as hell, but Mac OS is unable to do anything from the
         * partition fd, it wants an fd for the whole device, and it sometimes
         * also requires the device fd to be closed first, so we have to close
         * the handle that the caller gave us.
         * Also for some reason it wants the fd to be closed before we even
         * open the parent if we're trying to eject the disk.
         */
        if ((status = get_parent_device( fd, name, sizeof(name) ))) return status;
        if (code == IOCTL_STORAGE_EJECT_MEDIA)
            NtClose( device );
        if (needs_close) close( fd );
        TRACE("opening parent %s\n", name );
        if ((fd = open( name, O_RDONLY )) == -1)
            return errno_to_status( errno );
        needs_close = 1;
    }
#endif

    switch (code)
    {
    case IOCTL_CDROM_CHECK_VERIFY:
    case IOCTL_DISK_CHECK_VERIFY:
    case IOCTL_STORAGE_CHECK_VERIFY:
    case IOCTL_STORAGE_CHECK_VERIFY2:
        sz = 0;
	CDROM_ClearCacheEntry(dev);
        if (in_buffer != NULL || in_size != 0 || out_buffer != NULL || out_size != 0)
            status = STATUS_INVALID_PARAMETER;
        else status = CDROM_Verify(dev, fd);
        break;

/* EPP     case IOCTL_STORAGE_CHECK_VERIFY2: */

/* EPP     case IOCTL_STORAGE_FIND_NEW_DEVICES: */
/* EPP     case IOCTL_CDROM_FIND_NEW_DEVICES: */

    case IOCTL_STORAGE_LOAD_MEDIA:
    case IOCTL_CDROM_LOAD_MEDIA:
        sz = 0;
	CDROM_ClearCacheEntry(dev);
        if (in_buffer != NULL || in_size != 0 || out_buffer != NULL || out_size != 0)
            status = STATUS_INVALID_PARAMETER;
        else status = CDROM_SetTray(fd, FALSE);
        break;
     case IOCTL_STORAGE_EJECT_MEDIA:
        sz = 0;
	CDROM_ClearCacheEntry(dev);
        if (in_buffer != NULL || in_size != 0 || out_buffer != NULL || out_size != 0)
            status = STATUS_INVALID_PARAMETER;
        else
            status = CDROM_SetTray(fd, TRUE);
        break;

    case IOCTL_CDROM_MEDIA_REMOVAL:
    case IOCTL_DISK_MEDIA_REMOVAL:
    case IOCTL_STORAGE_MEDIA_REMOVAL:
    case IOCTL_STORAGE_EJECTION_CONTROL:
        /* FIXME the last ioctl:s is not the same as the two others...
         * lockcount/owner should be handled */
        sz = 0;
	CDROM_ClearCacheEntry(dev);
        if (out_buffer != NULL || out_size != 0) status = STATUS_INVALID_PARAMETER;
        else if (in_size < sizeof(PREVENT_MEDIA_REMOVAL)) status = STATUS_BUFFER_TOO_SMALL;
        else status = CDROM_ControlEjection(fd, in_buffer);
        break;

    case IOCTL_DISK_GET_MEDIA_TYPES:
    case IOCTL_STORAGE_GET_MEDIA_TYPES:
    case IOCTL_STORAGE_GET_MEDIA_TYPES_EX:
        sz = sizeof(GET_MEDIA_TYPES);
        if (in_buffer != NULL || in_size != 0) status = STATUS_INVALID_PARAMETER;
        else if (out_size < sz) status = STATUS_BUFFER_TOO_SMALL;
        else status = CDROM_GetMediaType(dev, fd, out_buffer);
        break;

    case IOCTL_STORAGE_GET_DEVICE_NUMBER:
        sz = sizeof(STORAGE_DEVICE_NUMBER);
        if (in_buffer != NULL || in_size != 0) status = STATUS_INVALID_PARAMETER;
        else if (out_size < sz) status = STATUS_BUFFER_TOO_SMALL;
        else status = CDROM_GetDeviceNumber(dev, out_buffer);
        break;

    case IOCTL_STORAGE_RESET_DEVICE:
        sz = 0;
	CDROM_ClearCacheEntry(dev);
        if (in_buffer != NULL || in_size != 0 || out_buffer != NULL || out_size != 0)
            status = STATUS_INVALID_PARAMETER;
        else status = CDROM_ResetAudio(fd);
        break;

    case IOCTL_CDROM_GET_CONTROL:
        sz = sizeof(CDROM_AUDIO_CONTROL);
        if (in_buffer != NULL || in_size != 0) status = STATUS_INVALID_PARAMETER;
        else if (out_size < sz) status = STATUS_BUFFER_TOO_SMALL;
        else status = CDROM_GetControl(dev, fd, out_buffer);
        break;

    case IOCTL_CDROM_GET_DRIVE_GEOMETRY:
        sz = sizeof(DISK_GEOMETRY);
        if (in_buffer != NULL || in_size != 0) status = STATUS_INVALID_PARAMETER;
        else if (out_size < sz) status = STATUS_BUFFER_TOO_SMALL;
        else status = CDROM_GetDriveGeometry(dev, fd, out_buffer);
        break;

    case IOCTL_CDROM_DISK_TYPE:
        sz = sizeof(CDROM_DISK_DATA);
	/* CDROM_ClearCacheEntry(dev); */
        if (in_buffer != NULL || in_size != 0) status = STATUS_INVALID_PARAMETER;
        else if (out_size < sz) status = STATUS_BUFFER_TOO_SMALL;
        else status = CDROM_GetDiskData(dev, fd, out_buffer);
        break;

/* EPP     case IOCTL_CDROM_GET_LAST_SESSION: */

    case IOCTL_CDROM_READ_Q_CHANNEL:
        sz = sizeof(SUB_Q_CHANNEL_DATA);
        if (in_buffer == NULL || in_size < sizeof(CDROM_SUB_Q_DATA_FORMAT))
            status = STATUS_INVALID_PARAMETER;
        else if (out_size < sz) status = STATUS_BUFFER_TOO_SMALL;
        else status = CDROM_ReadQChannel(dev, fd, in_buffer, out_buffer);
        break;

    case IOCTL_CDROM_READ_TOC:
        sz = sizeof(CDROM_TOC);
        if (in_buffer != NULL || in_size != 0) status = STATUS_INVALID_PARAMETER;
        else if (out_size < sz) status = STATUS_BUFFER_TOO_SMALL;
        else status = CDROM_ReadTOC(dev, fd, out_buffer);
        break;

/* EPP     case IOCTL_CDROM_READ_TOC_EX: */

    case IOCTL_CDROM_PAUSE_AUDIO:
        sz = 0;
        if (in_buffer != NULL || in_size != 0 || out_buffer != NULL || out_size != 0)
            status = STATUS_INVALID_PARAMETER;
        else status = CDROM_PauseAudio(fd);
        break;
    case IOCTL_CDROM_PLAY_AUDIO_MSF:
        sz = 0;
        if (out_buffer != NULL || out_size != 0) status = STATUS_INVALID_PARAMETER;
        else if (in_size < sizeof(CDROM_PLAY_AUDIO_MSF)) status = STATUS_BUFFER_TOO_SMALL;
        else status = CDROM_PlayAudioMSF(fd, in_buffer);
        break;
    case IOCTL_CDROM_RESUME_AUDIO:
        sz = 0;
        if (in_buffer != NULL || in_size != 0 || out_buffer != NULL || out_size != 0)
            status = STATUS_INVALID_PARAMETER;
        else status = CDROM_ResumeAudio(fd);
        break;
    case IOCTL_CDROM_SEEK_AUDIO_MSF:
        sz = 0;
        if (out_buffer != NULL || out_size != 0) status = STATUS_INVALID_PARAMETER;
        else if (in_size < sizeof(CDROM_SEEK_AUDIO_MSF)) status = STATUS_BUFFER_TOO_SMALL;
        else status = CDROM_SeekAudioMSF(dev, fd, in_buffer);
        break;
    case IOCTL_CDROM_STOP_AUDIO:
        sz = 0;
	CDROM_ClearCacheEntry(dev); /* Maybe intention is to change media */
        if (in_buffer != NULL || in_size != 0 || out_buffer != NULL || out_size != 0)
            status = STATUS_INVALID_PARAMETER;
        else status = CDROM_StopAudio(fd);
        break;
    case IOCTL_CDROM_GET_VOLUME:
        sz = sizeof(VOLUME_CONTROL);
        if (in_buffer != NULL || in_size != 0) status = STATUS_INVALID_PARAMETER;
        else if (out_size < sz) status = STATUS_BUFFER_TOO_SMALL;
        else status = CDROM_GetVolume(fd, out_buffer);
        break;
    case IOCTL_CDROM_SET_VOLUME:
        sz = 0;
	CDROM_ClearCacheEntry(dev);
        if (in_buffer == NULL || in_size < sizeof(VOLUME_CONTROL) || out_buffer != NULL)
            status = STATUS_INVALID_PARAMETER;
        else status = CDROM_SetVolume(fd, in_buffer);
        break;
    case IOCTL_CDROM_RAW_READ:
        sz = 0;
        if (in_size < sizeof(RAW_READ_INFO)) status = STATUS_INVALID_PARAMETER;
        else if (out_buffer == NULL) status = STATUS_BUFFER_TOO_SMALL;
        else status = CDROM_RawRead(fd, in_buffer, out_buffer,
                                    out_size, &sz);
        break;
    case IOCTL_SCSI_GET_ADDRESS:
        sz = sizeof(SCSI_ADDRESS);
        if (in_buffer != NULL || in_size != 0) status = STATUS_INVALID_PARAMETER;
        else if (out_size < sz) status = STATUS_BUFFER_TOO_SMALL;
        else status = CDROM_GetAddress(fd, out_buffer);
        break;
    case IOCTL_SCSI_PASS_THROUGH_DIRECT:
        sz = sizeof(SCSI_PASS_THROUGH_DIRECT);
        if (in_buffer == NULL || out_buffer == NULL) status = STATUS_INVALID_PARAMETER;
        else if (out_size < sizeof(SCSI_PASS_THROUGH_DIRECT)) status = STATUS_BUFFER_TOO_SMALL;
        else status = CDROM_ScsiPassThroughDirect(fd, in_buffer, out_buffer);
        break;
    case IOCTL_SCSI_PASS_THROUGH:
        if (in_wow64_call())
        {
            sz = sizeof(SCSI_PASS_THROUGH32);
            if (in_buffer == NULL || out_buffer == NULL) status = STATUS_INVALID_PARAMETER;
            else if (out_size < sizeof(SCSI_PASS_THROUGH32)) status = STATUS_BUFFER_TOO_SMALL;
            else status = CDROM_ScsiPassThrough32(fd, in_buffer, out_buffer);
        }
        else
        {
            sz = sizeof(SCSI_PASS_THROUGH);
            if (in_buffer == NULL || out_buffer == NULL) status = STATUS_INVALID_PARAMETER;
            else if (out_size < sizeof(SCSI_PASS_THROUGH)) status = STATUS_BUFFER_TOO_SMALL;
            else status = CDROM_ScsiPassThrough(fd, in_buffer, out_buffer);
        }
        break;
    case IOCTL_SCSI_GET_CAPABILITIES:
        sz = sizeof(IO_SCSI_CAPABILITIES);
        if (out_buffer == NULL) status = STATUS_INVALID_PARAMETER;
        else if (out_size < sizeof(IO_SCSI_CAPABILITIES)) status = STATUS_BUFFER_TOO_SMALL;
        else status = CDROM_ScsiGetCaps(fd, out_buffer);
        break;
    case IOCTL_DVD_START_SESSION:
        sz = sizeof(DVD_SESSION_ID);
        if (out_buffer == NULL) status = STATUS_INVALID_PARAMETER;
        else if (out_size < sz) status = STATUS_BUFFER_TOO_SMALL;
        else
        {
            TRACE("before in 0x%08x out 0x%08x\n",in_buffer ? *(DVD_SESSION_ID *)in_buffer : 0,
                  *(DVD_SESSION_ID *)out_buffer);
            status = DVD_StartSession(fd, in_buffer, out_buffer);
            TRACE("before in 0x%08x out 0x%08x\n",in_buffer ? *(DVD_SESSION_ID *)in_buffer : 0,
                  *(DVD_SESSION_ID *)out_buffer);
        }
        break;
    case IOCTL_DVD_END_SESSION:
        sz = sizeof(DVD_SESSION_ID);
        if ((in_buffer == NULL) ||  (in_size < sz))status = STATUS_INVALID_PARAMETER;
        else status = DVD_EndSession(fd, in_buffer);
        break;
    case IOCTL_DVD_SEND_KEY:
        sz = 0;
        if (!in_buffer ||
            (((PDVD_COPY_PROTECT_KEY)in_buffer)->KeyLength != in_size))
            status = STATUS_INVALID_PARAMETER;
        else
        {
            TRACE("doing DVD_SendKey\n");
            status = DVD_SendKey(fd, in_buffer);
        }
        break;
    case IOCTL_DVD_READ_KEY:
        if (!in_buffer ||
            (((PDVD_COPY_PROTECT_KEY)in_buffer)->KeyLength != in_size))
            status = STATUS_INVALID_PARAMETER;
        else if (in_buffer !=out_buffer) status = STATUS_BUFFER_TOO_SMALL;
        else
        {
            TRACE("doing DVD_READ_KEY\n");
            sz = ((PDVD_COPY_PROTECT_KEY)in_buffer)->KeyLength;
            status = DVD_ReadKey(fd, in_buffer);
        }
        break;
    case IOCTL_DVD_GET_REGION:
        sz = sizeof(DVD_REGION);
        if (in_buffer != NULL || in_size != 0) status = STATUS_INVALID_PARAMETER;
        else if (out_size < sz) status = STATUS_BUFFER_TOO_SMALL;
        else
        {
            TRACE("doing DVD_Get_REGION\n");
            status = DVD_GetRegion(fd, out_buffer);
        }
        break;
    case IOCTL_DVD_READ_STRUCTURE:
        sz = DVD_ReadStructureSize(in_buffer, in_size);
        if (in_buffer == NULL || in_size != sizeof(DVD_READ_STRUCTURE)) status = STATUS_INVALID_PARAMETER;
        else if (out_size < sz || !out_buffer) status = STATUS_BUFFER_TOO_SMALL;
        else
        {
            TRACE("doing DVD_READ_STRUCTURE\n");
            status = DVD_ReadStructure(fd, in_buffer, out_buffer);
        }
        break;

    case IOCTL_SCSI_GET_INQUIRY_DATA:
        sz = INQ_REPLY_LEN;
        status = GetInquiryData(fd, out_buffer, out_size);
        break;

    case IOCTL_STORAGE_QUERY_PROPERTY:
    {
        STORAGE_PROPERTY_QUERY *query = in_buffer;

        if (in_size < sizeof(STORAGE_PROPERTY_QUERY))
        {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        switch (query->PropertyId)
        {
        case StorageDeviceProperty:
        {
            STORAGE_DEVICE_DESCRIPTOR descriptor = { .Version = sizeof(descriptor), .Size = sizeof(descriptor),
                                                     .DeviceType = FILE_DEVICE_CD_ROM, .RemovableMedia = TRUE };
            FIXME("Faking StorageDeviceProperty data\n");
            io->Information = min(sizeof(descriptor), out_size);
            memcpy(out_buffer, &descriptor, io->Information);
            status = STATUS_SUCCESS;
            break;
        }
        default:
            FIXME("Unsupported property %#x\n", query->PropertyId);
        }
        break;
    }

    default:
        status = STATUS_NOT_SUPPORTED;
    }
    if (needs_close) close( fd );
    if (!NT_ERROR(status))
        file_complete_async( device, options, event, apc, apc_user, io, status, sz );
    return status;
}
