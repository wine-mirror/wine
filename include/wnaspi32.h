#if !defined(WNASPI32_H)
#define WNASPI32_H

#pragma pack(1)

typedef union SRB * LPSRB;

typedef struct tagSRB_HaInquiry {
 BYTE  SRB_Cmd;                 /* ASPI command code = SC_HA_INQUIRY */
 BYTE  SRB_Status;              /* ASPI command status byte */
 BYTE  SRB_HaId;                /* ASPI host adapter number */
 BYTE  SRB_Flags;               /* ASPI request flags */
 DWORD  SRB_Hdr_Rsvd;           /* Reserved, MUST = 0 */
 BYTE  HA_Count;                /* Number of host adapters present */
 BYTE  HA_SCSI_ID;              /* SCSI ID of host adapter */
 BYTE  HA_ManagerId[16];        /* String describing the manager */
 BYTE  HA_Identifier[16];       /* String describing the host adapter */
 BYTE  HA_Unique[16];           /* Host Adapter Unique parameters */
 WORD  HA_Rsvd1;
} SRB_HaInquiry WINE_PACKED;

typedef struct tagSRB_ExecSCSICmd {
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
} SRB_ExecSCSICmd WINE_PACKED;

typedef struct tagSRB_Abort {
  BYTE        SRB_Cmd;            /* ASPI command code = SC_ABORT_SRB */
  BYTE        SRB_Status;         /* ASPI command status byte */
  BYTE        SRB_HaId;           /* ASPI host adapter number */
  BYTE        SRB_Flags;          /* Reserved */
  DWORD       SRB_Hdr_Rsvd;       /* Reserved, MUST = 0 */
  LPSRB     SRB_ToAbort;        /* Pointer to SRB to abort */
} SRB_Abort WINE_PACKED;

typedef struct tagSRB_BusDeviceReset {
 BYTE         SRB_Cmd;                  /* ASPI command code = SC_RESET_DEV */
 BYTE         SRB_Status;               /* ASPI command status byte */
 BYTE         SRB_HaId;                 /* ASPI host adapter number */
 BYTE         SRB_Flags;                /* Reserved */
 DWORD        SRB_Hdr_Rsvd;             /* Reserved */
 BYTE         SRB_Target;               /* Target's SCSI ID */
 BYTE         SRB_Lun;                  /* Target's LUN number */
 BYTE         SRB_Rsvd1[12];            /* Reserved for Alignment */
 BYTE         SRB_HaStat;               /* Host Adapter Status */
 BYTE         SRB_TargStat;             /* Target Status */
 void         (*SRB_PostProc)();        /* Post routine */
 void         *SRB_Rsvd2;               /* Reserved */
 BYTE         SRB_Rsvd3[32];            /* Reserved */
} SRB_BusDeviceReset WINE_PACKED;

typedef struct tagSRB_GDEVBlock {
 BYTE  SRB_Cmd;                 /* ASPI command code = SC_GET_DEV_TYPE */
 BYTE  SRB_Status;              /* ASPI command status byte */
 BYTE  SRB_HaId;                /* ASPI host adapter number */
 BYTE  SRB_Flags;               /* Reserved */
 DWORD  SRB_Hdr_Rsvd;           /* Reserved */
 BYTE  SRB_Target;              /* Target's SCSI ID */
 BYTE  SRB_Lun;                 /* Target's LUN number */
 BYTE  SRB_DeviceType;          /* Target's peripheral device type */
 BYTE  SRB_Rsvd1;
} SRB_GDEVBlock WINE_PACKED;

typedef struct tagSRB_Common {
  BYTE  SRB_Cmd;
} SRB_Common;

union SRB {
  SRB_Common          common;
  SRB_HaInquiry       inquiry;
  SRB_ExecSCSICmd     cmd;
  SRB_Abort           abort;
  SRB_BusDeviceReset  reset;
  SRB_GDEVBlock       devtype;
};

typedef union SRB SRB;

#pragma pack(4)

#endif
