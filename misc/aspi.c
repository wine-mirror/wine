#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <ldt.h>
#include <memory.h>
#include <unistd.h>
#include <callback.h>
#include "windows.h"
#include "aspi.h"
#include "options.h"
#include "heap.h"
#include "debug.h"


/* FIXME!
 * 1) Residual byte length reporting not handled
 * 2) Make this code re-entrant for multithreading
 * 3) Only linux supported so far
 */

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
  unsigned int twelve_byte:1; /* Force 12 byte command length for group 6 & 7 commands  */
  unsigned int other_flags:31;			/* for future use */
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

#define INQUIRY_VENDOR		8

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
    struct ASPI_DEVICE_INFO *	next;
    int				fd;
    int				hostId;
    int				target;
    int				lun;
};

typedef struct ASPI_DEVICE_INFO ASPI_DEVICE_INFO;
static ASPI_DEVICE_INFO *ASPI_open_devices = NULL;

#ifdef linux
static int
ASPI_OpenDevice16(SRB_ExecSCSICmd16 *prb)
{
    int	fd;
    char	idstr[20];
    char	device_str[50];
    ASPI_DEVICE_INFO *curr;

    /* search list of devices to see if we've opened it already.
     * There is not an explicit open/close in ASPI land, so hopefully
     * keeping a device open won't be a problem.
     */

    for (curr = ASPI_open_devices; curr; curr = curr->next) {
	if (curr->hostId == prb->SRB_HaId &&
	    curr->target == prb->SRB_Target &&
	    curr->lun == prb->SRB_Lun) {
	    return curr->fd;
	}
    }

    /* device wasn't cached, go ahead and open it */
    sprintf(idstr, "scsi c%1dt%1dd%1d", prb->SRB_HaId, prb->SRB_Target, prb->SRB_Lun);

    if (!PROFILE_GetWineIniString(idstr, "Device", "", device_str, sizeof(device_str))) {
	TRACE(aspi, "Trying to open unlisted scsi device %s\n", idstr);
	return -1;
    }

    TRACE(aspi, "Opening device %s=%s\n", idstr, device_str);

    fd = open(device_str, O_RDWR);
    if (fd == -1) {
	int save_error = errno;
	WARN(aspi, "Error opening device errno=%d\n", save_error);
	return -1;
    }

    /* device is now open */
    curr = HeapAlloc( SystemHeap, 0, sizeof(ASPI_DEVICE_INFO) );
    curr->fd = fd;
    curr->hostId = prb->SRB_HaId;
    curr->target = prb->SRB_Target;
    curr->lun = prb->SRB_Lun;

    /* insert new record at beginning of open device list */
    curr->next = ASPI_open_devices;
    ASPI_open_devices = curr;
    return fd;
}


static void
ASPI_DebugPrintCmd16(SRB_ExecSCSICmd16 *prb)
{
  BYTE	cmd;
  int	i;
  BYTE *cdb;
  BYTE *lpBuf;
  dbg_decl_str(aspi, 512);

  lpBuf = PTR_SEG_TO_LIN(prb->SRB_BufPointer);

  switch (prb->CDBByte[0]) {
  case CMD_INQUIRY:
    TRACE(aspi, "{\n");
    TRACE(aspi, "\tEVPD: %d\n", prb->CDBByte[1] & 1);
    TRACE(aspi, "\tLUN: %d\n", (prb->CDBByte[1] & 0xc) >> 1);
    TRACE(aspi, "\tPAGE CODE: %d\n", prb->CDBByte[2]);
    TRACE(aspi, "\tALLOCATION LENGTH: %d\n", prb->CDBByte[4]);
    TRACE(aspi, "\tCONTROL: %d\n", prb->CDBByte[5]);
    TRACE(aspi, "}\n");
    break;
  case CMD_SCAN_SCAN:
    TRACE(aspi, "Transfer Length: %d\n", prb->CDBByte[4]);
    break;
  }

  TRACE(aspi, "Host Adapter: %d\n", prb->SRB_HaId);
  TRACE(aspi, "Flags: %d\n", prb->SRB_Flags);
  if (TARGET_TO_HOST(prb)) {
    TRACE(aspi, "\tData transfer: Target to host. Length checked.\n");
  }
  else if (HOST_TO_TARGET(prb)) {
    TRACE(aspi, "\tData transfer: Host to target. Length checked.\n");
  }
  else if (NO_DATA_TRANSFERED(prb)) {
    TRACE(aspi, "\tData transfer: none\n");
  }
  else {
    WARN(aspi, "\tTransfer by scsi cmd. Length not checked\n");
  }

  TRACE(aspi, "\tResidual byte length reporting %s\n", prb->SRB_Flags & 0x4 ? "enabled" : "disabled");
  TRACE(aspi, "\tLinking %s\n", prb->SRB_Flags & 0x2 ? "enabled" : "disabled");
  TRACE(aspi, "\tPosting %s\n", prb->SRB_Flags & 0x1 ? "enabled" : "disabled");
  TRACE(aspi, "Target: %d\n", prb->SRB_Target);
  TRACE(aspi, "Lun: %d\n", prb->SRB_Lun);
  TRACE(aspi, "BufLen: %ld\n", prb->SRB_BufLen);
  TRACE(aspi, "SenseLen: %d\n", prb->SRB_SenseLen);
  TRACE(aspi, "BufPtr: %lx (%p)\n", prb->SRB_BufPointer, lpBuf);
  TRACE(aspi, "LinkPointer %lx\n", prb->SRB_Rsvd1);
  TRACE(aspi, "CDB Length: %d\n", prb->SRB_CDBLen);
  TRACE(aspi, "POST Proc: %lx\n", (DWORD) prb->SRB_PostProc);
  cdb = &prb->CDBByte[0];
  cmd = prb->CDBByte[0];
  for (i = 0; i < prb->SRB_CDBLen; i++) {
    if (i != 0) dsprintf(aspi, ",");
    dsprintf(aspi, "%02x", *cdb++);
  }
  TRACE(aspi, "CDB buffer[%s]\n", dbg_str(aspi));
}

static void
PrintSenseArea16(SRB_ExecSCSICmd16 *prb)
{
  int	i;
  BYTE *cdb;
  dbg_decl_str(aspi, 512);

  cdb = &prb->CDBByte[0];
  for (i = 0; i < prb->SRB_SenseLen; i++) {
    if (i) dsprintf(aspi, ",");
    dsprintf(aspi, "%02x", *cdb++);
  }
  TRACE(aspi, "SenseArea[%s]\n", dbg_str(aspi));
}

static void
ASPI_DebugPrintResult16(SRB_ExecSCSICmd16 *prb)
{
  BYTE *lpBuf;

  lpBuf = PTR_SEG_TO_LIN(prb->SRB_BufPointer);

  switch (prb->CDBByte[0]) {
  case CMD_INQUIRY:
    TRACE(aspi, "Vendor: %s\n", lpBuf + INQUIRY_VENDOR);
    break;
  case CMD_TEST_UNIT_READY:
    PrintSenseArea16(prb);
    break;
  }
}

static WORD
ASPI_ExecScsiCmd16(SRB_ExecSCSICmd16 *prb, SEGPTR segptr_prb)
{
  struct sg_header *sg_hd, *sg_reply_hdr;
  int	status;
  BYTE *lpBuf;
  int	in_len, out_len;
  int	error_code = 0;
  int	fd;

  ASPI_DebugPrintCmd16(prb);

  fd = ASPI_OpenDevice16(prb);
  if (fd == -1) {
      prb->SRB_Status = SS_ERR;
      return SS_ERR;
  }

  sg_hd = NULL;
  sg_reply_hdr = NULL;

  prb->SRB_Status = SS_PENDING;
  lpBuf = PTR_SEG_TO_LIN(prb->SRB_BufPointer);

  if (!prb->SRB_CDBLen) {
      prb->SRB_Status = SS_ERR;
      return SS_ERR;
  }

  /* build up sg_header + scsi cmd */
  if (HOST_TO_TARGET(prb)) {
    /* send header, command, and then data */
    in_len = SCSI_OFF + prb->SRB_CDBLen + prb->SRB_BufLen;
    sg_hd = (struct sg_header *) malloc(in_len);
    memset(sg_hd, 0, SCSI_OFF);
    memcpy(sg_hd + 1, &prb->CDBByte[0], prb->SRB_CDBLen);
    if (prb->SRB_BufLen) {
      memcpy(((BYTE *) sg_hd) + SCSI_OFF + prb->SRB_CDBLen, lpBuf, prb->SRB_BufLen);
    }
  }
  else {
    /* send header and command - no data */
    in_len = SCSI_OFF + prb->SRB_CDBLen;
    sg_hd = (struct sg_header *) malloc(in_len);
    memset(sg_hd, 0, SCSI_OFF);
    memcpy(sg_hd + 1, &prb->CDBByte[0], prb->SRB_CDBLen);
  }

  if (TARGET_TO_HOST(prb)) {
    out_len = SCSI_OFF + prb->SRB_BufLen;
    sg_reply_hdr = (struct sg_header *) malloc(out_len);
    memset(sg_reply_hdr, 0, SCSI_OFF);
    sg_hd->reply_len = out_len;
  }
  else {
    out_len = SCSI_OFF;
    sg_reply_hdr = (struct sg_header *) malloc(out_len);
    memset(sg_reply_hdr, 0, SCSI_OFF);
    sg_hd->reply_len = out_len;
  }

  status = write(fd, sg_hd, in_len);
  if (status < 0 || status != in_len) {
      int myerror = errno;

    WARN(aspi, "Not enough bytes written to scsi device bytes=%d .. %d\n", in_len, status);
    if (status < 0) {
	if (myerror == ENOMEM) {
	    fprintf(stderr, "ASPI: Linux generic scsi driver\n  You probably need to re-compile your kernel with a larger SG_BIG_BUFF value (sg.h)\n  Suggest 130560\n");
	}
	WARN(aspi, "errno: = %d\n", myerror);
    }
    goto error_exit;
  }

  status = read(fd, sg_reply_hdr, out_len);
  if (status < 0 || status != out_len) {
    WARN(aspi, "not enough bytes read from scsi device%d\n", status);
    goto error_exit;
  }

  if (sg_reply_hdr->result != 0) {
    error_code = sg_reply_hdr->result;
    WARN(aspi, "reply header error (%d)\n", sg_reply_hdr->result);
    goto error_exit;
  }

  if (TARGET_TO_HOST(prb) && prb->SRB_BufLen) {
    memcpy(lpBuf, sg_reply_hdr + 1, prb->SRB_BufLen);
  }

  /* copy in sense buffer to amount that is available in client */
  if (prb->SRB_SenseLen) {
    int sense_len = prb->SRB_SenseLen;
    if (prb->SRB_SenseLen > 16)
      sense_len = 16;
    memcpy(SENSE_BUFFER(prb), &sg_reply_hdr->sense_buffer[0], sense_len);
  }


  prb->SRB_Status = SS_COMP;
  prb->SRB_HaStat = HASTAT_OK;
  prb->SRB_TargStat = STATUS_GOOD;

  /* now do  posting */

  if (ASPI_POSTING(prb) && prb->SRB_PostProc) {
    TRACE(aspi, "Post Routine (%lx) called\n", (DWORD) prb->SRB_PostProc);
    Callbacks->CallASPIPostProc(prb->SRB_PostProc, segptr_prb);
  }

  free(sg_reply_hdr);
  free(sg_hd);
  ASPI_DebugPrintResult16(prb);
  return SS_COMP;
  
error_exit:
  if (error_code == EBUSY) {
      prb->SRB_Status = SS_ASPI_IS_BUSY;
      TRACE(aspi, "Device busy\n");
  }
  else {
      WARN(aspi, "ASPI_GenericHandleScsiCmd failed\n");
      prb->SRB_Status = SS_ERR;
  }

  /* I'm not sure exactly error codes work here
   * We probably should set prb->SRB_TargStat, SRB_HaStat ?
   */
  WARN(aspi, "ASPI_GenericHandleScsiCmd: error_exit\n");
  free(sg_reply_hdr);
  free(sg_hd);
  return prb->SRB_Status;
}
#endif

/***********************************************************************
 *             GetASPISupportInfo16   (WINASPI.1)
 */

WORD WINAPI GetASPISupportInfo16()
{
#ifdef linux
    TRACE(aspi, "GETASPISupportInfo\n");
    /* high byte SS_COMP - low byte number of host adapters.
     * FIXME!!! The number of host adapters is incorrect.
     * I'm not sure how to determine this under linux etc.
     */
    return ((SS_COMP << 8) | 0x1);
#else
    return ((SS_COMP << 8) | 0x0);
#endif
}

/***********************************************************************
 *             SendASPICommand16   (WINASPI.2)
 */

WORD WINAPI SendASPICommand16(SEGPTR segptr_srb)
{
#ifdef linux
  LPSRB16 lpSRB = PTR_SEG_TO_LIN(segptr_srb);

  switch (lpSRB->common.SRB_cmd) {
  case SC_HA_INQUIRY:
    FIXME(aspi, "Not implemented SC_HA_INQUIRY\n");
    break;
  case SC_GET_DEV_TYPE:
    FIXME(aspi, "Not implemented SC_GET_DEV_TYPE\n");
    break;
  case SC_EXEC_SCSI_CMD:
    return ASPI_ExecScsiCmd16(&lpSRB->cmd, segptr_srb);
    break;
  case SC_RESET_DEV:
    FIXME(aspi, "Not implemented SC_RESET_DEV\n");
    break;
  default:
    WARN(aspi, "Unknown command %d\n", lpSRB->common.SRB_cmd);
  }
  return SS_INVALID_SRB;
#else
  return SS_INVALID_SRB;
#endif
}

/***********************************************************************
 *             GetASPIDLLVersion   (WINASPI.4)
 */

DWORD WINAPI GetASPIDLLVersion()
{
#ifdef linux
	return (DWORD)2;
#else
	return (DWORD)0;
#endif
}
