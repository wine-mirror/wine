/*
 * This file should be source compatible with the Adaptec winaspi.h
 * All DOS ASPI structures are the same as WINASPI
 *
 * Copyright (C) 2000 Alexandre Julliard
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

#ifndef __WINASPI_H__
#define __WINASPI_H__

/* Include base aspi defs */
#include <wnaspi32.h>

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/* WINE SCSI Stuff */
#define ASPI_DOS        1
#define ASPI_WIN16      2

#include <pshpack1.h>

/* SRB HA_INQUIRY */

typedef struct tagSRB16_HaInquiry {
  BYTE	SRB_Cmd;
  BYTE	SRB_Status;
  BYTE	SRB_HaId;
  BYTE	SRB_Flags;
  WORD	SRB_55AASignature;
  WORD	SRB_ExtBufferSize;
  BYTE	HA_Count;
  BYTE	HA_SCSI_ID;
  BYTE	HA_ManagerId[16];
  BYTE	HA_Identifier[16];
  BYTE	HA_Unique[16];
  BYTE	HA_ExtBuffer[4];
} SRB_HaInquiry16, *PSRB_HaInquiry16, *LPSRB_HaInquiry16;

typedef struct tagSRB16_GDEVBlock {
  BYTE        SRB_Cmd;            /* ASPI command code = SC_GET_DEV_TYPE */
  BYTE        SRB_Status;         /* ASPI command status byte */
  BYTE        SRB_HaId;           /* ASPI host adapter number */
  BYTE        SRB_Flags;          /* ASPI request flags */
  DWORD       SRB_Hdr_Rsvd;       /* Reserved, MUST = 0 */
  BYTE        SRB_Target;         /* Target's SCSI ID */
  BYTE        SRB_Lun;            /* Target's LUN number */
  BYTE        SRB_DeviceType;     /* Target's peripheral device type */
} SRB_GDEVBlock16, *PSRB_GDEVBlock16, *LPSRB_GDEVBlock16;


typedef struct tagSRB16_ExecSCSICmd {
  BYTE        SRB_Cmd;                /* ASPI command code	      (W)  */
  BYTE        SRB_Status;             /* ASPI command status byte     (R)  */
  BYTE        SRB_HaId;               /* ASPI host adapter number     (W)  */
  BYTE        SRB_Flags;              /* ASPI request flags	      (W)  */
  DWORD       SRB_Hdr_Rsvd;           /* Reserved, MUST = 0	      (-)  */
  BYTE        SRB_Target;             /* Target's SCSI ID	      (W)  */
  BYTE        SRB_Lun;                /* Target's LUN number	      (W)  */
  DWORD       SRB_BufLen;             /* Data Allocation LengthPG     (W/R)*/
  BYTE        SRB_SenseLen;           /* Sense Allocation Length      (W)  */
  SEGPTR      SRB_BufPointer;         /* Data Buffer Pointer	      (W)  */
  DWORD       SRB_Rsvd1;              /* Reserved, MUST = 0	      (-/W)*/
  BYTE        SRB_CDBLen;             /* CDB Length = 6		      (W)  */
  BYTE        SRB_HaStat;             /* Host Adapter Status	      (R)  */
  BYTE        SRB_TargStat;           /* Target Status		      (R)  */
  FARPROC16   SRB_PostProc;	      /* Post routine		      (W)  */
  BYTE        SRB_Rsvd2[34];          /* Reserved, MUST = 0                */
  BYTE		CDBByte[1];	      /* SCSI CBD - variable length   (W)  */
  /* variable example for 6 byte cbd
   * BYTE        CDBByte[6];             * SCSI CDB                    (W) *
   * BYTE        SenseArea6[SENSE_LEN];  * Request Sense buffer 	(R) *
   */
} SRB_ExecSCSICmd16, *PSRB_ExecSCSICmd16, *LPSRB_ExecSCSICmd16;

typedef struct tagSRB16_Abort {
  BYTE        SRB_Cmd;            /* ASPI command code = SC_ABORT_SRB */
  BYTE        SRB_Status;         /* ASPI command status byte */
  BYTE        SRB_HaId;           /* ASPI host adapter number */
  BYTE        SRB_Flags;          /* ASPI request flags */
  DWORD       SRB_Hdr_Rsvd;       /* Reserved, MUST = 0 */
  SEGPTR      SRB_ToAbort;        /* Pointer to SRB to abort */
} SRB_Abort16, *PSRB_Abort16, *LPSRB_Abort16;


typedef struct tagSRB16_BusDeviceReset {
  BYTE        SRB_Cmd;            /* ASPI command code = SC_RESET_DEV */
  BYTE        SRB_Status;         /* ASPI command status byte */
  BYTE        SRB_HaId;           /* ASPI host adapter number */
  BYTE        SRB_Flags;          /* ASPI request flags */
  DWORD       SRB_Hdr_Rsvd;       /* Reserved, MUST = 0 */
  BYTE        SRB_Target;         /* Target's SCSI ID */
  BYTE        SRB_Lun;            /* Target's LUN number */
  BYTE        SRB_ResetRsvd1[14]; /* Reserved, MUST = 0 */
  BYTE        SRB_HaStat;         /* Host Adapter Status */
  BYTE        SRB_TargStat;       /* Target Status */
  FARPROC16   SRB_PostProc;       /* Post routine */
  BYTE        SRB_ResetRsvd2[34]; /* Reserved, MUST = 0 */
} SRB_BusDeviceReset16, *PSRB_BusDeviceReset16, *LPSRB_BusDeviceReset16;


typedef struct tagSRB16_Common {
  BYTE        SRB_Cmd;            /* ASPI command code = SC_ABORT_SRB */
  BYTE        SRB_Status;         /* ASPI command status byte */
  BYTE        SRB_HaId;           /* ASPI host adapter number */
  BYTE        SRB_Flags;          /* ASPI request flags */
  DWORD       SRB_Hdr_Rsvd;       /* Reserved, MUST = 0 */
} SRB_Common16, *PSRB_Common16, *LPSRB_Common16;

typedef union tagSRB16 {
    SRB_Common16          common;
    SRB_HaInquiry16       inquiry;
    SRB_ExecSCSICmd16     cmd;
    SRB_Abort16           abort;
    SRB_BusDeviceReset16  reset;
    SRB_GDEVBlock16       devtype;
} SRB16, *LPSRB16;

#include <poppack.h>

extern WORD WINAPI SendASPICommand16(SEGPTR);
extern WORD WINAPI GetASPISupportInfo16(void);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* __WINE_WINASPI_H */
