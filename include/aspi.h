/* ASPI definitions used for both WINASPI and WNASPI32 */

#ifndef __WINE_ASPI_H
#define __WINE_ASPI_H

#include "windef.h"

#include "pshpack1.h"
#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */


/* Stuff in BOTH spec */

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





/*********** OLD ****************/

/* Target status codes */
#define STATUS_GOOD             0x00
#define STATUS_CHKCOND          0x02
#define STATUS_BUSY             0x08
#define STATUS_RESCONF          0x18

#define ASPI_POSTING(prb) (prb->SRB_Flags & 0x1)

/* WNASPI32/WINASPI defs */
#define HOST_TO_TARGET(prb) (((prb->SRB_Flags>>3) & 0x3) == 0x2)
#define TARGET_TO_HOST(prb) (((prb->SRB_Flags>>3) & 0x3) == 0x1)
#define NO_DATA_TRANSFERED(prb) (((prb->SRB_Flags>>3) & 0x3) == 0x3)


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

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */
#include "poppack.h"

#endif
