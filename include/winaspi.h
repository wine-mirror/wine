/**************************************************************************
 * WINE winaspi.h
 * This file should be source compatible with the Adaptec winaspi.h
 * All DOS ASPI structures are the same as WINASPI
 */

/* If __WINE__ is not defined, extra typedefs are defined to be
 * source compatible with the regular winaspi.h.
 */
#ifndef __WINASPI_H__
#define __WINASPI_H__

#define FAR
/* Include base aspi defs */
#include "aspi.h"

#include "pshpack1.h"
#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/* WINE SCSI Stuff */
#define ASPI_DOS        1
#define ASPI_WIN16      2

/* SRB HA_INQUIRY */

struct tagSRB16_HaInquiry {
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
} WINE_PACKED;

struct tagSRB16_GDEVBlock {
  BYTE        SRB_Cmd;            /* ASPI command code = SC_GET_DEV_TYPE */
  BYTE        SRB_Status;         /* ASPI command status byte */
  BYTE        SRB_HaId;           /* ASPI host adapter number */
  BYTE        SRB_Flags;          /* ASPI request flags */
  DWORD       SRB_Hdr_Rsvd;       /* Reserved, MUST = 0 */
  BYTE        SRB_Target;         /* Target's SCSI ID */
  BYTE        SRB_Lun;            /* Target's LUN number */
  BYTE        SRB_DeviceType;     /* Target's peripheral device type */
} WINE_PACKED;



struct tagSRB16_ExecSCSICmd {
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
  BYTE		CDBByte[0];	      /* SCSI CBD - variable length   (W)  */
  /* variable example for 6 byte cbd
   * BYTE        CDBByte[6];             * SCSI CDB                    (W) *
   * BYTE        SenseArea6[SENSE_LEN];  * Request Sense buffer 	(R) *
   */
} WINE_PACKED;

struct tagSRB16_Abort {
  BYTE        SRB_Cmd;            /* ASPI command code = SC_ABORT_SRB */
  BYTE        SRB_Status;         /* ASPI command status byte */
  BYTE        SRB_HaId;           /* ASPI host adapter number */
  BYTE        SRB_Flags;          /* ASPI request flags */
  DWORD       SRB_Hdr_Rsvd;       /* Reserved, MUST = 0 */
  SEGPTR      SRB_ToAbort;        /* Pointer to SRB to abort */
} WINE_PACKED;

struct tagSRB16_BusDeviceReset {
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
} WINE_PACKED;

struct tagSRB16_Common {
  BYTE        SRB_Cmd;            /* ASPI command code = SC_ABORT_SRB */
  BYTE        SRB_Status;         /* ASPI command status byte */
  BYTE        SRB_HaId;           /* ASPI host adapter number */
  BYTE        SRB_Flags;          /* ASPI request flags */
  DWORD       SRB_Hdr_Rsvd;       /* Reserved, MUST = 0 */
} WINE_PACKED;

union tagSRB16 {
  struct tagSRB16_Common common;
  struct tagSRB16_HaInquiry inquiry;
  struct tagSRB16_ExecSCSICmd cmd;
  struct tagSRB16_Abort abort;
  struct tagSRB16_BusDeviceReset reset;
  struct tagSRB16_GDEVBlock devtype;
};

#ifndef __WINE__
/* These typedefs would conflict with WNASPI32 typedefs, but
 * would make it easier to port WINASPI source to WINE */
typedef struct tagSRB16_HaInquiry
SRB_HAInquiry, *PSRB_HAInquiry, FAR *LPSRB_HAInquiry;

typedef struct tagSRB16_GDEVBlock
SRB_GDEVBlock, *PSRB_GDEVBlock, FAR *LPSRB_GDEVBlock;

typedef struct tagSRB16_ExecSCSICmd
SRB_ExecSCSICmd, *PSRB_ExecSCSICmd, FAR *LPSRB_ExecSCSICmd;

typedef struct tagSRB16_Abort
SRB_Abort, *PSRB_Abort, FAR *LPSRB_Abort;

typedef struct tagSRB16_BusDeviceReset
SRB_BusDeviceReset, *PSRB_BusDeviceReset, FAR *LPSRB_BusDeviceReset;

typedef struct tagSRB16_Common
SRB_Common, *PSRB_Common, FAR *LPSRB_Common;

typedef union tagSRB16 SRB, FAR *LPSRB;

extern WORD FAR PASCAL SendASPICommand( LPSRB );
extern WORD FAR PASCAL GetASPISupportInfo( VOID );

#endif

/* These are the typedefs for WINE */
typedef struct tagSRB16_HaInquiry
SRB_HAInquiry16, *PSRB_HAInquiry16, FAR *LPSRB_HAInquiry16;

typedef struct tagSRB16_GDEVBlock
SRB_GDEVBlock16, *PSRB_GDEVBlock16, FAR *LPSRB_GDEVBlock16;

typedef struct tagSRB16_ExecSCSICmd
SRB_ExecSCSICmd16, *PSRB_ExecSCSICmd16, FAR *LPSRB_ExecSCSICmd16;

typedef struct tagSRB16_Abort
SRB_Abort16, *PSRB_Abort16, FAR *LPSRB_Abort16;

typedef struct tagSRB16_BusDeviceReset
SRB_BusDeviceReset16, *PSRB_BusDeviceReset16, FAR *LPSRB_BusDeviceReset16;

typedef struct tagSRB16_Common
SRB_Common16, *PSRB_Common16, FAR *LPSRB_Common16;

typedef union tagSRB16 SRB16, FAR *LPSRB16;

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#include "poppack.h"

#endif /* __WINE_WINASPI_H */
