#ifndef __WNASPI32_H__
#define __WNASPI32_H__

#define FAR
/* This file should be 100% source compatible according to MSes docs and
 * Adaptecs docs */
/* Include base aspi defs */
#include "aspi.h"

#include "pshpack1.h"
#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/* Additional definitions */
/* SCSI Miscellaneous Stuff */
#define SRB_EVENT_NOTIFY		0x40
#define RESIDUAL_COUNT_SUPPORTED	0x02
#define MAX_SRB_TIMEOUT			1080001u
#define DEFAULT_SRB_TIMEOUT		1080001u

/* These are defined by MS but not adaptec */
#define SRB_DATA_SG_LIST		0x02
#define WM_ASPIPOST			0x4D42


/* ASPI Command Definitions */
#define SC_RESCAN_SCSI_BUS		0x07
#define SC_GETSET_TIMEOUTS		0x08

/* SRB Status.. MS defined */
#define SS_SECURITY_VIOLATION		0xE2 // Replaces SS_INVALID_MODE
/*** END DEFS */

/* SRB - HOST ADAPTER INQUIRY - SC_HA_INQUIRY */
struct tagSRB32_HaInquiry {
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
} WINE_PACKED;

/* SRB - GET DEVICE TYPE - SC_GET_DEV_TYPE */
struct tagSRB32_GDEVBlock {
 BYTE  SRB_Cmd;                 /* ASPI command code = SC_GET_DEV_TYPE */
 BYTE  SRB_Status;              /* ASPI command status byte */
 BYTE  SRB_HaId;                /* ASPI host adapter number */
 BYTE  SRB_Flags;               /* Reserved */
 DWORD  SRB_Hdr_Rsvd;           /* Reserved */
 BYTE  SRB_Target;              /* Target's SCSI ID */
 BYTE  SRB_Lun;                 /* Target's LUN number */
 BYTE  SRB_DeviceType;          /* Target's peripheral device type */
 BYTE  SRB_Rsvd1;
} WINE_PACKED;

/* SRB - EXECUTE SCSI COMMAND - SC_EXEC_SCSI_CMD */
struct tagSRB32_ExecSCSICmd {
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
} WINE_PACKED;

/* SRB - ABORT AN ARB - SC_ABORT_SRB */
struct tagSRB32_Abort {
  BYTE        SRB_Cmd;            /* ASPI command code = SC_ABORT_SRB */
  BYTE        SRB_Status;         /* ASPI command status byte */
  BYTE        SRB_HaId;           /* ASPI host adapter number */
  BYTE        SRB_Flags;          /* Reserved */
  DWORD       SRB_Hdr_Rsvd;       /* Reserved, MUST = 0 */
  VOID        FAR *SRB_ToAbort;   /* Pointer to SRB to abort */
} WINE_PACKED;

/* SRB - BUS DEVICE RESET - SC_RESET_DEV */
struct tagSRB32_BusDeviceReset {
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
} WINE_PACKED;

/* SRB - GET DISK INFORMATION - SC_GET_DISK_INFO */
struct tagSRB32_GetDiskInfo {
 BYTE         SRB_Cmd;                  /* ASPI command code = SC_RESET_DEV */
 BYTE         SRB_Status;               /* ASPI command status byte */
 BYTE         SRB_HaId;                 /* ASPI host adapter number */
 BYTE         SRB_Flags;                /* Reserved */
 DWORD        SRB_Hdr_Rsvd;             /* Reserved */
 BYTE         SRB_Target;               /* Target's SCSI ID */
 BYTE         SRB_Lun;                  /* Target's LUN number */
 BYTE         SRB_DriveFlags;           /* Driver flags */
 BYTE         SRB_Int13HDriveInfo;      /* Host Adapter Status */
 BYTE         SRB_Heads;                /* Preferred number of heads trans */
 BYTE         SRB_Sectors;              /* Preferred number of sectors trans */
 BYTE         SRB_Rsvd1[10];            /* Reserved */
} WINE_PACKED;

/* SRB header */
struct tagSRB32_Header {
 BYTE         SRB_Cmd;                  /* ASPI command code = SC_RESET_DEV */
 BYTE         SRB_Status;               /* ASPI command status byte */
 BYTE         SRB_HaId;                 /* ASPI host adapter number */
 BYTE         SRB_Flags;                /* Reserved */
 DWORD        SRB_Hdr_Rsvd;             /* Reserved */
} WINE_PACKED;

union tagSRB32 {
  struct tagSRB32_Header          common;
  struct tagSRB32_HaInquiry       inquiry;
  struct tagSRB32_ExecSCSICmd     cmd;
  struct tagSRB32_Abort           abort;
  struct tagSRB32_BusDeviceReset  reset;
  struct tagSRB32_GDEVBlock       devtype;
};

/* Typedefs */
#define typedefSRB(name) \
typedef struct tagSRB32_##name \
SRB_##name##, *PSRB_##name
typedefSRB(HaInquiry);
typedefSRB(GDEVBlock);
typedefSRB(ExecSCSICmd);
typedefSRB(Abort);
typedefSRB(BusDeviceReset);
typedefSRB(GetDiskInfo);
typedefSRB(Header);
#undef typedefSRB

typedef union tagSRB32 SRB, *PSRB, *LPSRB;

/* Prototypes */
extern DWORD __cdecl
SendASPI32Command (PSRB);
extern DWORD WINAPI
GetASPI32SupportInfo (void);
extern DWORD WINAPI
GetASPI32DLLVersion(void);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */
#include "poppack.h"

#endif /* __WNASPI32_H__ */
