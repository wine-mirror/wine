/* ASPI definitions used for both WNASPI16 and WNASPI32 */

#if !defined(ASPI_H)
#define ASPI_H

#include "pshpack1.h"

#define SS_PENDING      0x00
#define SS_COMP         0x01
#define SS_ABORTED      0x02
#define SS_ERR          0x04
#define SS_OLD_MANAGE   0xe1
#define SS_ILLEGAL_MODE 0xe2
#define SS_NO_ASPI      0xe3
#define SS_FAILED_INIT  0xe4
#define SS_INVALID_HA   0x81
#define SS_INVALID_SRB  0xe0
#define SS_ASPI_IS_BUSY 0xe5
#define SS_BUFFER_TO_BIG        0xe6

#define SC_HA_INQUIRY           0x00
#define SC_GET_DEV_TYPE         0x01
#define SC_EXEC_SCSI_CMD        0x02
#define SC_ABORT_SRB            0x03
#define SC_RESET_DEV            0x04


/* Host adapter status codes */
#define HASTAT_OK               0x00
#define HASTAT_SEL_TO           0x11
#define HASTAT_DO_DU            0x12
#define HASTAT_BUS_FREE         0x13
#define HASTAT_PHASE_ERR        0x14

/* Target status codes */
#define STATUS_GOOD             0x00
#define STATUS_CHKCOND          0x02
#define STATUS_BUSY             0x08
#define STATUS_RESCONF          0x18

#ifdef linux

/* This is a duplicate of the sg_header from /usr/src/linux/include/scsi/sg.h
 * kernel 2.0.30
 * This will probably break at some point, but for those who don't have
 * kernels installed, I think this should still work.
 *
 */

struct sg_header
 {
  int pack_len;    /* length of incoming packet <4096 (including header) */
  int reply_len;   /* maximum length <4096 of expected reply */
  int pack_id;     /* id number of packet */
  int result;      /* 0==ok, otherwise refer to errno codes */
  unsigned int twelve_byte:1; /* Force 12 byte command length for group 6 & 7
commands  */
  unsigned int other_flags:31;                  /* for future use */
  unsigned char sense_buffer[16]; /* used only by reads */
  /* command follows then data for command */
 };

#define SCSI_OFF sizeof(struct sg_header)
#endif

#define ASPI_POSTING(prb) (prb->SRB_Flags & 0x1)

#define HOST_TO_TARGET(prb) (((prb->SRB_Flags>>3) & 0x3) == 0x2)
#define TARGET_TO_HOST(prb) (((prb->SRB_Flags>>3) & 0x3) == 0x1)
#define NO_DATA_TRANSFERED(prb) (((prb->SRB_Flags>>3) & 0x3) == 0x3)

#define SRB_ENABLE_RESIDUAL_COUNT 0x4
#define SRB_EVENT_NOTIFY 0x40 /* Enable ASPI event notification */

#define INQUIRY_VENDOR          8

#define MUSTEK_SCSI_AREA_AND_WINDOWS 0x04
#define MUSTEK_SCSI_READ_SCANNED_DATA 0x08
#define MUSTEK_SCSI_GET_IMAGE_STATUS 0x0f
#define MUSTEK_SCSI_ADF_AND_BACKTRACE 0x10
#define MUSTEK_SCSI_CCD_DISTANCE 0x11
#define MUSTEK_SCSI_START_STOP 0x1b

#define CMD_TEST_UNIT_READY 0x00
#define CMD_REQUEST_SENSE 0x03
#define CMD_INQUIRY 0x12

/* scanner commands - just for debug */
#define CMD_SCAN_GET_DATA_BUFFER_STATUS 0x34
#define CMD_SCAN_GET_WINDOW 0x25
#define CMD_SCAN_OBJECT_POSITION 0x31
#define CMD_SCAN_READ 0x28
#define CMD_SCAN_RELEASE_UNIT 0x17
#define CMD_SCAN_RESERVE_UNIT 0x16
#define CMD_SCAN_SCAN 0x1b
#define CMD_SCAN_SEND 0x2a
#define CMD_SCAN_CHANGE_DEFINITION 0x40

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
static ASPI_DEVICE_INFO *ASPI_open_devices = NULL;

#include "poppack.h"

#endif
