#if !defined(WINASPI_H)
#define WINASPI_H

#include "pshpack1.h"

#define ASPI_DOS        1
#define ASPI_WIN16      2

typedef union SRB16 * LPSRB16;

typedef struct tagSRB_HaInquiry16 {
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
} SRB_HaInquiry16 WINE_PACKED;

typedef struct tagSRB_ExecSCSICmd16 {
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
} SRB_ExecSCSICmd16 WINE_PACKED;

typedef struct tagSRB_Abort16 {
  BYTE        SRB_Cmd;            /* ASPI command code = SC_ABORT_SRB */
  BYTE        SRB_Status;         /* ASPI command status byte */
  BYTE        SRB_HaId;           /* ASPI host adapter number */
  BYTE        SRB_Flags;          /* ASPI request flags */
  DWORD       SRB_Hdr_Rsvd;       /* Reserved, MUST = 0 */
  LPSRB16     SRB_ToAbort;        /* Pointer to SRB to abort */
} SRB_Abort16 WINE_PACKED;

typedef struct tagSRB_BusDeviceReset16 {
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
  SEGPTR      SRB_PostProc;       /* Post routine */
  BYTE        SRB_ResetRsvd2[34]; /* Reserved, MUST = 0 */
} SRB_BusDeviceReset16 WINE_PACKED;

typedef struct tagSRB_GDEVBlock16 {
  BYTE        SRB_Cmd;            /* ASPI command code = SC_GET_DEV_TYPE */
  BYTE        SRB_Status;         /* ASPI command status byte */
  BYTE        SRB_HaId;           /* ASPI host adapter number */
  BYTE        SRB_Flags;          /* ASPI request flags */
  DWORD       SRB_Hdr_Rsvd;       /* Reserved, MUST = 0 */
  BYTE        SRB_Target;         /* Target's SCSI ID */
  BYTE        SRB_Lun;            /* Target's LUN number */
  BYTE        SRB_DeviceType;     /* Target's peripheral device type */
} SRB_GDEVBlock16 WINE_PACKED;

typedef struct tagSRB_Common16 {
  BYTE	SRB_Cmd;
} SRB_Common16;

union SRB16 {
  SRB_Common16		common;
  SRB_HaInquiry16	inquiry;
  SRB_ExecSCSICmd16	cmd;
  SRB_Abort16		abort;
  SRB_BusDeviceReset16	reset;
  SRB_GDEVBlock16	devtype;
};

typedef union SRB16 SRB16;

#include "poppack.h"

#endif
