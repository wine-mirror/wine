#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <ldt.h>
#include <memory.h>
#include <unistd.h>
#include <callback.h>

#include "winbase.h"
#include "aspi.h"
#include "wnaspi32.h"
#include "options.h"
#include "heap.h"
#include "debug.h"


/* FIXME!
 * 1) Residual byte length reporting not handled
 * 2) Make this code re-entrant for multithreading
 * 3) Only linux supported so far
 */

#ifdef linux
static int
ASPI_OpenDevice32(SRB_ExecSCSICmd32 *prb)
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
#ifdef HAVE_STRERROR
    ERR(aspi, "Error opening device %s, error '%s'\n", device_str, strerror(save_error));
#else
    ERR(aspi, "Error opening device %s, error %d\n", device_str, save_error);
#endif
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
ASPI_DebugPrintCmd32(SRB_ExecSCSICmd32 *prb)
{
  BYTE	cmd;
  int	i;
  BYTE *cdb;
  dbg_decl_str(aspi, 512);

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
    WARN(aspi, "\tTransfer by scsi cmd. Length not checked.\n");
  }

  TRACE(aspi, "\tResidual byte length reporting %s\n", prb->SRB_Flags & 0x4 ? "enabled" : "disabled");
  TRACE(aspi, "\tLinking %s\n", prb->SRB_Flags & 0x2 ? "enabled" : "disabled");
  TRACE(aspi, "\tPosting %s\n", prb->SRB_Flags & 0x1 ? "enabled" : "disabled");
  TRACE(aspi, "Target: %d\n", prb->SRB_Target);
  TRACE(aspi, "Lun: %d\n", prb->SRB_Lun);
  TRACE(aspi, "BufLen: %ld\n", prb->SRB_BufLen);
  TRACE(aspi, "SenseLen: %d\n", prb->SRB_SenseLen);
  TRACE(aspi, "BufPtr: %p\n", prb->SRB_BufPointer);
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
ASPI_PrintSenseArea32(SRB_ExecSCSICmd32 *prb)
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
ASPI_DebugPrintResult32(SRB_ExecSCSICmd32 *prb)
{

  switch (prb->CDBByte[0]) {
  case CMD_INQUIRY:
    TRACE(aspi, "Vendor: '%s'\n", prb->SRB_BufPointer + INQUIRY_VENDOR);
    break;
  case CMD_TEST_UNIT_READY:
    ASPI_PrintSenseArea32(prb);
    break;
  }
}

static WORD
ASPI_ExecScsiCmd32(SRB_ExecSCSICmd32 *lpPRB)
{
  struct sg_header *sg_hd, *sg_reply_hdr;
  int	status;
  int	in_len, out_len;
  int	error_code = 0;
  int	fd;

  ASPI_DebugPrintCmd32(lpPRB);

  fd = ASPI_OpenDevice32(lpPRB);
  if (fd == -1) {
      ERR(aspi, "Failed: could not open device. Device permissions !?\n");
      lpPRB->SRB_Status = SS_ERR;
      return SS_ERR;
  }

  sg_hd = NULL;
  sg_reply_hdr = NULL;

  lpPRB->SRB_Status = SS_PENDING;

  if (!lpPRB->SRB_CDBLen) {
      WARN(aspi, "Failed: lpPRB->SRB_CDBLen = 0.\n");
      lpPRB->SRB_Status = SS_ERR;
      return SS_ERR;
  }

  /* build up sg_header + scsi cmd */
  if (HOST_TO_TARGET(lpPRB)) {
    /* send header, command, and then data */
    in_len = SCSI_OFF + lpPRB->SRB_CDBLen + lpPRB->SRB_BufLen;
    sg_hd = (struct sg_header *) malloc(in_len);
    memset(sg_hd, 0, SCSI_OFF);
    memcpy(sg_hd + 1, &lpPRB->CDBByte[0], lpPRB->SRB_CDBLen);
    if (lpPRB->SRB_BufLen) {
      memcpy(((BYTE *) sg_hd) + SCSI_OFF + lpPRB->SRB_CDBLen, lpPRB->SRB_BufPointer, lpPRB->SRB_BufLen);
    }
  }
  else {
    /* send header and command - no data */
    in_len = SCSI_OFF + lpPRB->SRB_CDBLen;
    sg_hd = (struct sg_header *) malloc(in_len);
    memset(sg_hd, 0, SCSI_OFF);
    memcpy(sg_hd + 1, &lpPRB->CDBByte[0], lpPRB->SRB_CDBLen);
  }

  if (TARGET_TO_HOST(lpPRB)) {
    out_len = SCSI_OFF + lpPRB->SRB_BufLen;
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
      int save_error = errno;

    WARN(aspi, "Not enough bytes written to scsi device bytes=%d .. %d\n", in_len, status);
    if (status < 0) {
		if (save_error == ENOMEM) {
	    MSG("ASPI: Linux generic scsi driver\n  You probably need to re-compile your kernel with a larger SG_BIG_BUFF value (sg.h)\n  Suggest 130560\n");
	}
#ifdef HAVE_STRERROR
		WARN(aspi, "error:= '%s'\n", strerror(save_error));
#else
		WARN(aspi, "error:= %d\n", save_error);
#endif
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

  if (TARGET_TO_HOST(lpPRB) && lpPRB->SRB_BufLen) {
    memcpy(lpPRB->SRB_BufPointer, sg_reply_hdr + 1, lpPRB->SRB_BufLen);
  }

  /* copy in sense buffer to amount that is available in client */
  if (lpPRB->SRB_SenseLen) {
    int sense_len = lpPRB->SRB_SenseLen;
    if (lpPRB->SRB_SenseLen > 16)
      sense_len = 16;
    memcpy(SENSE_BUFFER(lpPRB), &sg_reply_hdr->sense_buffer[0], sense_len);
  }


  lpPRB->SRB_Status = SS_COMP;
  lpPRB->SRB_HaStat = HASTAT_OK;
  lpPRB->SRB_TargStat = STATUS_GOOD;

  /* now do posting */

  if (lpPRB->SRB_PostProc) {
    if (ASPI_POSTING(lpPRB)) {
      TRACE(aspi, "Post Routine (%lx) called\n", (DWORD) lpPRB->SRB_PostProc);
      (*lpPRB->SRB_PostProc)(lpPRB);
    }
    else
    if (lpPRB->SRB_Flags & SRB_EVENT_NOTIFY) {
      TRACE(aspi, "Setting event %04x\n", (HANDLE32)lpPRB->SRB_PostProc);
      SetEvent((HANDLE32)lpPRB->SRB_PostProc); /* FIXME: correct ? */
    }
  }
  free(sg_reply_hdr);
  free(sg_hd);
  ASPI_DebugPrintResult32(lpPRB);
  return SS_COMP;
  
error_exit:
  if (error_code == EBUSY) {
      lpPRB->SRB_Status = SS_ASPI_IS_BUSY;
      TRACE(aspi, "Device busy\n");
  }
  else {
      WARN(aspi, "Failed\n");
      lpPRB->SRB_Status = SS_ERR;
  }

  /* I'm not sure exactly error codes work here
   * We probably should set lpPRB->SRB_TargStat, SRB_HaStat ?
   */
  WARN(aspi, "error_exit\n");
  free(sg_reply_hdr);
  free(sg_hd);
  return lpPRB->SRB_Status;
}
#endif


/*******************************************************************
 *     GetASPI32SupportInfo32		[WNASPI32.0]
 *
 * Checks if the ASPI subsystem is initialized correctly.
 *
 * RETURNS
 *    HIWORD: 0.
 *    HIBYTE of LOWORD: status (SS_COMP or SS_FAILED_INIT)
 *    LOBYTE of LOWORD: # of host adapters.  
 */
DWORD WINAPI GetASPI32SupportInfo32()
{
    return ((SS_COMP << 8) | 1); /* FIXME: get # of host adapters installed */
}


/***********************************************************************
 *             SendASPI32Command32 (WNASPI32.1)
 */
DWORD __cdecl SendASPI32Command32(LPSRB32 lpSRB)
{
#ifdef linux
  switch (lpSRB->common.SRB_Cmd) {
  case SC_HA_INQUIRY:
    lpSRB->inquiry.SRB_Status = SS_COMP;       /* completed successfully */
    lpSRB->inquiry.HA_Count = 1;               /* not always */
    lpSRB->inquiry.HA_SCSI_ID = 7;             /* not always ID 7 */
    strcat(lpSRB->inquiry.HA_ManagerId, "ASPI for WIN32"); /* max 15 chars, don't change */
    strcat(lpSRB->inquiry.HA_Identifier, "Wine host"); /* FIXME: return host adapter name */
    memset(lpSRB->inquiry.HA_Unique, 0, 16); /* default HA_Unique content */
    lpSRB->inquiry.HA_Unique[6] = 0x02; /* Maximum Transfer Length (128K, Byte> 4-7) */
    FIXME(aspi, "ASPI: Partially implemented SC_HA_INQUIRY for adapter %d.\n", lpSRB->inquiry.SRB_HaId);
    return SS_COMP;
  case SC_GET_DEV_TYPE:
    FIXME(aspi, "Not implemented SC_GET_DEV_TYPE\n");
    break;
  case SC_EXEC_SCSI_CMD:
    return ASPI_ExecScsiCmd32(&lpSRB->cmd);
    break;
  case SC_RESET_DEV:
    FIXME(aspi, "Not implemented SC_RESET_DEV\n");
    break;
  default:
    WARN(aspi, "Unknown command %d\n", lpSRB->common.SRB_Cmd);
  }
  return SS_INVALID_SRB;
#else
  return SS_INVALID_SRB;
#endif
}


/***********************************************************************
 *             GetASPI32DLLVersion32   (WNASPI32.3)
 */

DWORD WINAPI GetASPI32DLLVersion32()
{
#ifdef linux
        return (DWORD)1;
#else
        return (DWORD)0;
#endif
}

