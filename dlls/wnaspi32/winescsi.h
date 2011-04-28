/*
 * Copyright (C) 2000 David Elliott <dfe@infinite-internet.net>
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

#ifndef __WINESCSI_H__
#define __WINESCSI_H__

#ifdef linux
/* Copy of info from 2.2.x kernel */
#define SG_MAX_SENSE 16   /* too little, unlikely to change in 2.2.x */

#define SG_NEXT_CMD_LEN 0x2283  /* override SCSI command length with given
		   number on the next write() on this file descriptor */

struct sg_header
{
    int pack_len;    /* [o] reply_len (ie useless), ignored as input */
    int reply_len;   /* [i] max length of expected reply (inc. sg_header) */
    int pack_id;     /* [io] id number of packet (use ints >= 0) */
    int result;      /* [o] 0==ok, else (+ve) Unix errno (best ignored) */
    unsigned int twelve_byte:1;
        /* [i] Force 12 byte command length for group 6 & 7 commands  */
    unsigned int target_status:5;   /* [o] scsi status from target */
    unsigned int host_status:8;     /* [o] host status (see "DID" codes) */
    unsigned int driver_status:8;   /* [o] driver status+suggestion */
    unsigned int other_flags:10;    /* unused */
    unsigned char sense_buffer[SG_MAX_SENSE]; /* [o] Output in 3 cases:
           when target_status is CHECK_CONDITION or
           when target_status is COMMAND_TERMINATED or
           when (driver_status & DRIVER_SENSE) is true. */
};      /* This structure is 36 bytes long on i386 */

#define SCSI_OFF sizeof(struct sg_header)

#define SG_SET_TIMEOUT	0x2201
#define SG_GET_TIMEOUT	0x2202
#define SCSI_DEFAULT_TIMEOUT 6000*5 /* 5 minutes */
#endif


/* RegKey used for SCSI info under HKLM */
#define KEYNAME_SCSI "HARDWARE\\DEVICEMAP\\Scsi\\Scsi Port %d\\Scsi Bus %d\\Target Id %d\\Logical Unit Id %d"

/* Function prototypes from dlls/wnaspi32/aspi.c */
void
SCSI_Init(void) DECLSPEC_HIDDEN;

int
ASPI_GetNumControllers(void) DECLSPEC_HIDDEN;

int
SCSI_OpenDevice( int h, int c, int t, int d ) DECLSPEC_HIDDEN;

int
SCSI_LinuxSetTimeout( int fd, int timeout ) DECLSPEC_HIDDEN;

#ifdef linux
BOOL
SCSI_LinuxDeviceIo( int fd,
		struct sg_header * lpvInBuffer, DWORD cbInBuffer,
		struct sg_header * lpvOutBuffer, DWORD cbOutBuffer,
		LPDWORD lpcbBytesReturned ) DECLSPEC_HIDDEN;

void
SCSI_Fix_CMD_LEN( int fd, int cmd, int len ) DECLSPEC_HIDDEN;
#endif

DWORD
ASPI_GetHCforController( int controller ) DECLSPEC_HIDDEN;

/*** This is where we throw some miscellaneous crap ***/

#define ASPI_POSTING(prb) (prb->SRB_Flags & 0x1)

/* WNASPI32/WINASPI defs */
#define HOST_TO_TARGET(prb) (((prb->SRB_Flags>>3) & 0x3) == 0x2)
#define TARGET_TO_HOST(prb) (((prb->SRB_Flags>>3) & 0x3) == 0x1)
#define NO_DATA_TRANSFERRED(prb) (((prb->SRB_Flags>>3) & 0x3) == 0x3)


#define INQUIRY_VENDOR          8

#define MUSTEK_SCSI_AREA_AND_WINDOWS 0x04
#define MUSTEK_SCSI_READ_SCANNED_DATA 0x08
#define MUSTEK_SCSI_GET_IMAGE_STATUS 0x0f
#define MUSTEK_SCSI_ADF_AND_BACKTRACE 0x10
#define MUSTEK_SCSI_CCD_DISTANCE 0x11
#define MUSTEK_SCSI_START_STOP 0x1b


#define INQURIY_CMDLEN 6
#define INQURIY_REPLY_LEN 96
#define INQUIRY_VENDOR 8

#define SENSE_BUFFER(prb) (&prb->CDBByte[prb->SRB_CDBLen])

/* Just a container for seeing what devices are open */
struct ASPI_DEVICE_INFO {
    struct ASPI_DEVICE_INFO *   next;
    int                         fd;
    int                         hostId;
    int                         target;
    int                         lun;
};

typedef struct ASPI_DEVICE_INFO ASPI_DEVICE_INFO;

/*** End Miscellaneous crap ***/

#endif /* #ifndef __WINESCSI_H */
