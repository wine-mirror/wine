#ifndef __WNASPI32_H__
#define __WNASPI32_H__

/* This file should be 100% source compatible according to MSes docs and
 * Adaptecs docs */

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/* SCSI Miscellaneous Stuff */
#define SENSE_LEN			14
#define SRB_DIR_SCSI			0x00
#define SRB_POSTING			0x01
#define SRB_ENABLE_RESIDUAL_COUNT	0x04
#define SRB_DIR_IN			0x08
#define SRB_DIR_OUT			0x10

/* ASPI Command Definitions */
#define SC_HA_INQUIRY			0x00
#define SC_GET_DEV_TYPE			0x01
#define SC_EXEC_SCSI_CMD		0x02
#define SC_ABORT_SRB			0x03
#define SC_RESET_DEV			0x04
#define SC_SET_HA_PARMS			0x05
#define SC_GET_DISK_INFO		0x06

/* SRB status codes */
#define SS_PENDING			0x00
#define SS_COMP				0x01
#define SS_ABORTED			0x02
#define SS_ABORT_FAIL			0x03
#define SS_ERR				0x04

#define SS_INVALID_CMD			0x80
#define SS_INVALID_HA			0x81
#define SS_NO_DEVICE			0x82

#define SS_INVALID_SRB			0xE0
#define SS_OLD_MANAGER			0xE1
#define SS_BUFFER_ALIGN			0xE1 // Win32
#define SS_ILLEGAL_MODE			0xE2
#define SS_NO_ASPI			0xE3
#define SS_FAILED_INIT			0xE4
#define SS_ASPI_IS_BUSY			0xE5
#define SS_BUFFER_TO_BIG		0xE6
#define SS_MISMATCHED_COMPONENTS	0xE7 // DLLs/EXE version mismatch
#define SS_NO_ADAPTERS			0xE8
#define SS_INSUFFICIENT_RESOURCES	0xE9
#define SS_ASPI_IS_SHUTDOWN		0xEA
#define SS_BAD_INSTALL			0xEB


/* Host status codes */
#define HASTAT_OK			0x00
#define HASTAT_SEL_TO			0x11
#define HASTAT_DO_DU			0x12
#define HASTAT_BUS_FREE			0x13
#define HASTAT_PHASE_ERR		0x14

#define HASTAT_TIMEOUT			0x09
#define HASTAT_COMMAND_TIMEOUT		0x0B
#define HASTAT_MESSAGE_REJECT		0x0D
#define HASTAT_BUS_RESET		0x0E
#define HASTAT_PARITY_ERROR		0x0F
#define HASTAT_REQUEST_SENSE_FAILED	0x10


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

#include "pshpack1.h"

/* SRB - HOST ADAPTER INQUIRY - SC_HA_INQUIRY */
typedef struct tagSRB32_HaInquiry {
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
} SRB_HaInquiry, *PSRB_HaInquiry;

/* SRB - GET DEVICE TYPE - SC_GET_DEV_TYPE */
typedef struct tagSRB32_GDEVBlock {
 BYTE  SRB_Cmd;                 /* ASPI command code = SC_GET_DEV_TYPE */
 BYTE  SRB_Status;              /* ASPI command status byte */
 BYTE  SRB_HaId;                /* ASPI host adapter number */
 BYTE  SRB_Flags;               /* Reserved */
 DWORD  SRB_Hdr_Rsvd;           /* Reserved */
 BYTE  SRB_Target;              /* Target's SCSI ID */
 BYTE  SRB_Lun;                 /* Target's LUN number */
 BYTE  SRB_DeviceType;          /* Target's peripheral device type */
 BYTE  SRB_Rsvd1;
} SRB_GDEVBlock, *PSRB_GDEVBlock;

/* SRB - EXECUTE SCSI COMMAND - SC_EXEC_SCSI_CMD */
typedef struct tagSRB32_ExecSCSICmd {
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
  BYTE        SenseArea[SENSE_LEN+2];  /* Request sense buffer - var length */
} SRB_ExecSCSICmd, *PSRB_ExecSCSICmd;

/* SRB - ABORT AN ARB - SC_ABORT_SRB */
typedef struct tagSRB32_Abort {
  BYTE        SRB_Cmd;            /* ASPI command code = SC_ABORT_SRB */
  BYTE        SRB_Status;         /* ASPI command status byte */
  BYTE        SRB_HaId;           /* ASPI host adapter number */
  BYTE        SRB_Flags;          /* Reserved */
  DWORD       SRB_Hdr_Rsvd;       /* Reserved, MUST = 0 */
  VOID       *SRB_ToAbort;        /* Pointer to SRB to abort */
} SRB_Abort, *PSRB_Abort;

/* SRB - BUS DEVICE RESET - SC_RESET_DEV */
typedef struct tagSRB32_BusDeviceReset {
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
} SRB_BusDeviceReset, *PSRB_BusDeviceReset;

/* SRB - GET DISK INFORMATION - SC_GET_DISK_INFO */
typedef struct tagSRB32_GetDiskInfo {
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
} SRB_GetDiskInfo, *PSRB_GetDiskInfo;

/* SRB header */
typedef struct tagSRB32_Header {
 BYTE         SRB_Cmd;                  /* ASPI command code = SC_RESET_DEV */
 BYTE         SRB_Status;               /* ASPI command status byte */
 BYTE         SRB_HaId;                 /* ASPI host adapter number */
 BYTE         SRB_Flags;                /* Reserved */
 DWORD        SRB_Hdr_Rsvd;             /* Reserved */
} SRB_Header, *PSRB_Header;

typedef union tagSRB32 {
    SRB_Header          common;
    SRB_HaInquiry       inquiry;
    SRB_ExecSCSICmd     cmd;
    SRB_Abort           abort;
    SRB_BusDeviceReset  reset;
    SRB_GDEVBlock       devtype;
} SRB, *PSRB, *LPSRB;

#include "poppack.h"

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

#endif /* __WNASPI32_H__ */
