#if !defined(ASPI_H)
#define ASPI_H

#pragma pack(1)

#define SS_PENDING	0x00
#define SS_COMP		0x01
#define SS_ABORTED	0x02
#define SS_ERR		0x04
#define SS_OLD_MANAGE	0xe1
#define SS_ILLEGAL_MODE	0xe2
#define SS_NO_ASPI	0xe3
#define SS_FAILED_INIT	0xe4
#define SS_INVALID_HA	0x81
#define SS_INVALID_SRB	0xe0
#define SS_ASPI_IS_BUSY	0xe5
#define SS_BUFFER_TO_BIG	0xe6

#define SC_HA_INQUIRY		0x00
#define SC_GET_DEV_TYPE 	0x01
#define SC_EXEC_SCSI_CMD	0x02
#define SC_ABORT_SRB		0x03
#define SC_RESET_DEV		0x04

/* Host adapter status codes */
#define HASTAT_OK		0x00
#define HASTAT_SEL_TO		0x11
#define HASTAT_DO_DU		0x12
#define HASTAT_BUS_FREE		0x13
#define HASTAT_PHASE_ERR	0x14

/* Target status codes */
#define STATUS_GOOD		0x00
#define STATUS_CHKCOND		0x02
#define STATUS_BUSY		0x08
#define STATUS_RESCONF		0x18


typedef union SRB16 * LPSRB16;

struct SRB_HaInquiry16 {
  BYTE	SRB_cmd;
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

typedef struct SRB_HaInquiry16 SRB_HaInquiry16;

struct SRB_ExecSCSICmd16 {
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
} WINE_PACKED ;

typedef struct SRB_ExecSCSICmd16 SRB_ExecSCSICmd16;

struct SRB_ExecSCSICmd32 {
  BYTE        SRB_Cmd;            /* ASPI command code = SC_EXEC_SCSI_CMD */
  BYTE        SRB_Status;         /* ASPI command status byte */
  BYTE        SRB_HaId;           /* ASPI host adapter number */
  BYTE        SRB_Flags;          /* ASPI request flags */
  DWORD       SRB_Hdr_Rsvd;       /* Reserved */
  BYTE        SRB_Target;         /* Target's SCSI ID */
  BYTE        SRB_Lun;            /* Target's LUN number */
  WORD        SRB_Rsvd1;          /* Reserved for Alignment */
  DWORD       SRB_BufLen;         /* Data Allocation Length */
  BYTE        *SRB_BufPointer;    /* Data Buffer Point */
  BYTE        SRB_SenseLen;       /* Sense Allocation Length */
  BYTE        SRB_CDBLen;         /* CDB Length */
  BYTE        SRB_HaStat;         /* Host Adapter Status */
  BYTE        SRB_TargStat;       /* Target Status */
  void        (*SRB_PostProc)();  /* Post routine */
  void        *SRB_Rsvd2;         /* Reserved */
  BYTE        SRB_Rsvd3[16];      /* Reserved for expansion */
  BYTE        CDBByte[16];        /* SCSI CDB */
  BYTE        SenseArea[0];       /* Request sense buffer - var length */
};

typedef struct SRB_ExecSCSICmd32 SRB_ExecSCSICmd32;

struct SRB_Abort16 {
  BYTE        SRB_Cmd;            /* ASPI command code = SC_ABORT_SRB */
  BYTE        SRB_Status;         /* ASPI command status byte */
  BYTE        SRB_HaId;           /* ASPI host adapter number */
  BYTE        SRB_Flags;          /* ASPI request flags */
  DWORD       SRB_Hdr_Rsvd;       /* Reserved, MUST = 0 */
  LPSRB16     SRB_ToAbort;        /* Pointer to SRB to abort */
} WINE_PACKED;

typedef struct SRB_Abort16 SRB_Abort16;

struct SRB_BusDeviceReset16 {
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
} WINE_PACKED;

typedef struct SRB_BusDeviceReset16 SRB_BusDeviceReset16;

struct SRB_GDEVBlock16 {
  BYTE        SRB_Cmd;            /* ASPI command code = SC_GET_DEV_TYPE */
  BYTE        SRB_Status;         /* ASPI command status byte */
  BYTE        SRB_HaId;           /* ASPI host adapter number */
  BYTE        SRB_Flags;          /* ASPI request flags */
  DWORD       SRB_Hdr_Rsvd;       /* Reserved, MUST = 0 */
  BYTE        SRB_Target;         /* Target's SCSI ID */
  BYTE        SRB_Lun;            /* Target's LUN number */
  BYTE        SRB_DeviceType;     /* Target's peripheral device type */
} WINE_PACKED;

typedef struct SRB_GDEVBlock16 SRB_GDEVBlock16;



struct SRB_Common16 {
  BYTE	SRB_cmd;
};


typedef struct SRB_Common16 SRB_Common16;


union SRB16 {
  SRB_Common16		common;
  SRB_HaInquiry16	inquiry;
  SRB_ExecSCSICmd16	cmd;
  SRB_Abort16		abort;
  SRB_BusDeviceReset16	reset;
  SRB_GDEVBlock16	devtype;
};

typedef union SRB16 SRB16;




#endif
