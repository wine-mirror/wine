/*
 * Copyright 1997 Bruce Milner
 * Copyright 1998 Andreas Mohr
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

#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wine/windef16.h"
#include "wine/winbase16.h"
#include "wnaspi32.h"
#include "winreg.h"
#include "wownt32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(aspi);

#include <pshpack1.h>

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

static FARPROC16 ASPIChainFunc = NULL;
static FARPROC16 exec_postproc16;
static SEGPTR exec_cmd16;

static void SRB_PostProc( SRB_ExecSCSICmd *cmd32 )
{
    WOWCallback16( (DWORD)exec_postproc16, exec_cmd16 );
}


/***********************************************************************
 *             GetASPISupportInfo   (WINASPI.1)
 */
WORD WINAPI GetASPISupportInfo16(void)
{
    return GetASPI32SupportInfo();
}


/***********************************************************************
 *             SendASPICommand   (WINASPI.2)
 */
WORD WINAPI SendASPICommand16(SEGPTR segptr_srb)
{
    SRB srb32;
    LPSRB16 srb16 = MapSL( segptr_srb );
    DWORD ret;

    if (ASPIChainFunc)
    {
        /* This is not the post proc, it's the chain proc this time */
        ret = WOWCallback16((DWORD)ASPIChainFunc, segptr_srb);
        if (ret)
        {
            srb16->inquiry.SRB_Status = SS_INVALID_SRB;
            return ret;
        }
    }

    switch (srb16->common.SRB_Cmd)
    {
    case SC_HA_INQUIRY:
        srb32.common.SRB_Cmd = SC_HA_INQUIRY;
        ret = SendASPI32Command( &srb32 );
        memcpy( &srb16->inquiry, &srb32.inquiry, sizeof(srb16->inquiry) );
        return ret;

    case SC_GET_DEV_TYPE:
        FIXME("Not implemented SC_GET_DEV_TYPE\n");
        break;

    case SC_EXEC_SCSI_CMD:
        srb32.cmd.SRB_Cmd = srb16->cmd.SRB_Cmd;
        srb32.cmd.SRB_Status = srb16->cmd.SRB_Status;
        srb32.cmd.SRB_HaId = srb16->cmd.SRB_HaId;
        srb32.cmd.SRB_Flags = srb16->cmd.SRB_Flags;
        srb32.cmd.SRB_Hdr_Rsvd = srb16->cmd.SRB_Hdr_Rsvd;
        srb32.cmd.SRB_Target = srb16->cmd.SRB_Target;
        srb32.cmd.SRB_Lun = srb16->cmd.SRB_Lun;
        srb32.cmd.SRB_BufLen = srb16->cmd.SRB_BufLen;
        srb32.cmd.SRB_BufPointer = MapSL(srb16->cmd.SRB_BufPointer);
        srb32.cmd.SRB_SenseLen = srb16->cmd.SRB_SenseLen;
        srb32.cmd.SRB_CDBLen = srb16->cmd.SRB_CDBLen;
        srb32.cmd.SRB_HaStat = srb16->cmd.SRB_HaStat;
        srb32.cmd.SRB_TargStat = srb16->cmd.SRB_TargStat;
        memcpy( &srb32.cmd.CDBByte, srb16->cmd.CDBByte, srb32.cmd.SRB_CDBLen );
        srb32.cmd.SRB_PostProc = SRB_PostProc;
        exec_postproc16 = srb16->cmd.SRB_PostProc;
        exec_cmd16 = segptr_srb;
        ret = SendASPI32Command( &srb32 );
        srb16->cmd.SRB_Status = srb32.cmd.SRB_Status;
        srb16->cmd.SRB_BufLen = srb32.cmd.SRB_BufLen;
        srb16->cmd.SRB_HaStat = srb32.cmd.SRB_HaStat;
        srb16->cmd.SRB_TargStat = srb32.cmd.SRB_TargStat;
        memcpy( srb16->cmd.CDBByte + srb16->cmd.SRB_CDBLen, srb32.cmd.SenseArea,
                min( 16, srb32.cmd.SRB_SenseLen ) );
        return ret;

    case SC_RESET_DEV:
        FIXME("Not implemented SC_RESET_DEV\n");
        break;

    default:
        FIXME("Unknown command %d\n", srb16->common.SRB_Cmd);
    }
    return SS_INVALID_SRB;
}


/***********************************************************************
 *             InsertInASPIChain   (WINASPI.3)
 */
WORD WINAPI InsertInASPIChain16(BOOL16 remove, FARPROC16 pASPIChainFunc)
{
    if (remove) /* Remove */
    {
	if (ASPIChainFunc == pASPIChainFunc)
	{
	    ASPIChainFunc = NULL;
	    return SS_COMP;
	}
    }
    else /* Insert */
    {
	if (ASPIChainFunc == NULL)
	{
	    ASPIChainFunc = pASPIChainFunc;
	    return SS_COMP;
	}
    }
    return SS_ERR;
}


/***********************************************************************
 *             GetASPIDLLVersion   (WINASPI.4)
 */
DWORD WINAPI GetASPIDLLVersion16(void)
{
    return 2;
}
