#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <memory.h>
#include <unistd.h>

#include "winbase.h"
#include "aspi.h"
#include "wnaspi32.h"
#include "winescsi.h"
#include "options.h"
#include "heap.h"
#include "debugtools.h"
#include "ldt.h"
#include "callback.h"

DEFAULT_DEBUG_CHANNEL(aspi);

/* FIXME!
 * 1) Residual byte length reporting not handled
 * 2) Make this code re-entrant for multithreading
 *    -- Added CriticalSection to OpenDevices function
 * 3) Only linux supported so far
 * 4) Leaves sg devices open. This may or may not be okay.  A better solution
 *    would be to close the file descriptors when the thread/process using
 *    them no longer needs them.
 */

#ifdef linux

static ASPI_DEVICE_INFO *ASPI_open_devices = NULL;
static CRITICAL_SECTION ASPI_CritSection;

#endif /* defined(linux) */


BOOL WINAPI WNASPI32_LibMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
#ifdef linux
	static BOOL	bInitDone=FALSE;
#if 0
	TRACE("0x%x 0x%1x %p\n", hInstDLL, fdwReason, fImpLoad);
#endif
	switch( fdwReason )
	{
	case DLL_PROCESS_ATTACH:
		/* Create instance data */
		if(!bInitDone)
		{
			/* Initialize global stuff just once */
			InitializeCriticalSection(&ASPI_CritSection);
			bInitDone=TRUE;
		}
		break;
	case DLL_PROCESS_DETACH:
		/* Destroy instance data */
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
#else /* defined(linux) */
	return TRUE;
#endif /* defined(linux) */
}

#ifdef linux

static int
ASPI_OpenDevice(SRB_ExecSCSICmd *prb)
{
    int	fd;
    char	idstr[20];
    char	device_str[50];
    ASPI_DEVICE_INFO *curr;

    /* search list of devices to see if we've opened it already.
     * There is not an explicit open/close in ASPI land, so hopefully
     * keeping a device open won't be a problem.
     */

    EnterCriticalSection(&ASPI_CritSection);
    for (curr = ASPI_open_devices; curr; curr = curr->next) {
	if (curr->hostId == prb->SRB_HaId &&
	    curr->target == prb->SRB_Target &&
	    curr->lun == prb->SRB_Lun) {
            LeaveCriticalSection(&ASPI_CritSection);
	    return curr->fd;
	}
    }
    LeaveCriticalSection(&ASPI_CritSection);

    /* device wasn't cached, go ahead and open it */
    sprintf(idstr, "scsi c%1dt%1dd%1d", prb->SRB_HaId, prb->SRB_Target, prb->SRB_Lun);

    if (!PROFILE_GetWineIniString(idstr, "Device", "", device_str, sizeof(device_str))) {
	TRACE("Trying to open unlisted scsi device %s\n", idstr);
	return -1;
    }

    TRACE("Opening device %s=%s\n", idstr, device_str);

    fd = open(device_str, O_RDWR);
    if (fd == -1) {
	int save_error = errno;
#ifdef HAVE_STRERROR
    ERR("Error opening device %s, error '%s'\n", device_str, strerror(save_error));
#else
    ERR("Error opening device %s, error %d\n", device_str, save_error);
#endif
	return -1;
    }

    /* device is now open */
    curr = HeapAlloc( GetProcessHeap(), 0, sizeof(ASPI_DEVICE_INFO) );
    curr->fd = fd;
    curr->hostId = prb->SRB_HaId;
    curr->target = prb->SRB_Target;
    curr->lun = prb->SRB_Lun;

    /* insert new record at beginning of open device list */
    EnterCriticalSection(&ASPI_CritSection);
    curr->next = ASPI_open_devices;
    ASPI_open_devices = curr;
    LeaveCriticalSection(&ASPI_CritSection);
    return fd;
}


static void
ASPI_DebugPrintCmd(SRB_ExecSCSICmd *prb)
{
  BYTE	cmd;
  int	i;
  BYTE *cdb;

  switch (prb->CDBByte[0]) {
  case CMD_INQUIRY:
    TRACE("{\n");
    TRACE("\tEVPD: %d\n", prb->CDBByte[1] & 1);
    TRACE("\tLUN: %d\n", (prb->CDBByte[1] & 0xc) >> 1);
    TRACE("\tPAGE CODE: %d\n", prb->CDBByte[2]);
    TRACE("\tALLOCATION LENGTH: %d\n", prb->CDBByte[4]);
    TRACE("\tCONTROL: %d\n", prb->CDBByte[5]);
    TRACE("}\n");
    break;
  case CMD_SCAN_SCAN:
    TRACE("Transfer Length: %d\n", prb->CDBByte[4]);
    break;
  }

  TRACE("Host Adapter: %d\n", prb->SRB_HaId);
  TRACE("Flags: %d\n", prb->SRB_Flags);
  if (TARGET_TO_HOST(prb)) {
    TRACE("\tData transfer: Target to host. Length checked.\n");
  }
  else if (HOST_TO_TARGET(prb)) {
    TRACE("\tData transfer: Host to target. Length checked.\n");
  }
  else if (NO_DATA_TRANSFERED(prb)) {
    TRACE("\tData transfer: none\n");
  }
  else {
    WARN("\tTransfer by scsi cmd. Length not checked.\n");
  }

  TRACE("\tResidual byte length reporting %s\n", prb->SRB_Flags & 0x4 ? "enabled" : "disabled");
  TRACE("\tLinking %s\n", prb->SRB_Flags & 0x2 ? "enabled" : "disabled");
  TRACE("\tPosting %s\n", prb->SRB_Flags & 0x1 ? "enabled" : "disabled");
  TRACE("Target: %d\n", prb->SRB_Target);
  TRACE("Lun: %d\n", prb->SRB_Lun);
  TRACE("BufLen: %ld\n", prb->SRB_BufLen);
  TRACE("SenseLen: %d\n", prb->SRB_SenseLen);
  TRACE("BufPtr: %p\n", prb->SRB_BufPointer);
  TRACE("CDB Length: %d\n", prb->SRB_CDBLen);
  TRACE("POST Proc: %lx\n", (DWORD) prb->SRB_PostProc);
  cdb = &prb->CDBByte[0];
  cmd = prb->CDBByte[0];
  if (TRACE_ON(aspi))
  {
      DPRINTF("CDB buffer[");
      for (i = 0; i < prb->SRB_CDBLen; i++) {
          if (i != 0) DPRINTF(",");
          DPRINTF("%02x", *cdb++);
      }
      DPRINTF("]\n");
  }
}

static void
ASPI_PrintSenseArea(SRB_ExecSCSICmd *prb)
{
  int	i;
  BYTE *cdb;

  if (TRACE_ON(aspi))
  {
      cdb = &prb->CDBByte[16];
      DPRINTF("SenseArea[");
      for (i = 0; i < prb->SRB_SenseLen; i++) {
          if (i) DPRINTF(",");
          DPRINTF("%02x", *cdb++);
      }
      DPRINTF("]\n");
  }
}

static void
ASPI_DebugPrintResult(SRB_ExecSCSICmd *prb)
{

  TRACE("SRB_Status: %x\n", prb->SRB_Status);
  TRACE("SRB_HaStat: %x\n", prb->SRB_HaStat);
  TRACE("SRB_TargStat: %x\n", prb->SRB_TargStat);
  switch (prb->CDBByte[0]) {
  case CMD_INQUIRY:
    TRACE("Vendor: '%s'\n", prb->SRB_BufPointer + INQUIRY_VENDOR);
    break;
  case CMD_TEST_UNIT_READY:
    ASPI_PrintSenseArea(prb);
    break;
  }
}

static WORD
ASPI_ExecScsiCmd(SRB_ExecSCSICmd *lpPRB)
{
  struct sg_header *sg_hd, *sg_reply_hdr;
  int	status;
  int	in_len, out_len;
  int	error_code = 0;
  int	fd;

  ASPI_DebugPrintCmd(lpPRB);

  fd = ASPI_OpenDevice(lpPRB);
  if (fd == -1) {
      ERR("Failed: could not open device c%01dt%01dd%01d. Device permissions !?\n",
	  lpPRB->SRB_HaId,lpPRB->SRB_Target,lpPRB->SRB_Lun);
      lpPRB->SRB_Status = SS_NO_DEVICE;
      return SS_NO_DEVICE;
  }

  sg_hd = NULL;
  sg_reply_hdr = NULL;

  lpPRB->SRB_Status = SS_PENDING;

  if (!lpPRB->SRB_CDBLen) {
      WARN("Failed: lpPRB->SRB_CDBLen = 0.\n");
      lpPRB->SRB_Status = SS_ERR;
      return SS_ERR;
  }

  /* build up sg_header + scsi cmd */
  if (HOST_TO_TARGET(lpPRB)) {
    /* send header, command, and then data */
    in_len = SCSI_OFF + lpPRB->SRB_CDBLen + lpPRB->SRB_BufLen;
    sg_hd = (struct sg_header *) HeapAlloc(GetProcessHeap(), 0, in_len);
    memset(sg_hd, 0, SCSI_OFF);
    memcpy(sg_hd + 1, &lpPRB->CDBByte[0], lpPRB->SRB_CDBLen);
    if (lpPRB->SRB_BufLen) {
      memcpy(((BYTE *) sg_hd) + SCSI_OFF + lpPRB->SRB_CDBLen, lpPRB->SRB_BufPointer, lpPRB->SRB_BufLen);
    }
  }
  else {
    /* send header and command - no data */
    in_len = SCSI_OFF + lpPRB->SRB_CDBLen;
    sg_hd = (struct sg_header *) HeapAlloc(GetProcessHeap(), 0, in_len);
    memset(sg_hd, 0, SCSI_OFF);
    memcpy(sg_hd + 1, &lpPRB->CDBByte[0], lpPRB->SRB_CDBLen);
  }

  if (TARGET_TO_HOST(lpPRB)) {
    out_len = SCSI_OFF + lpPRB->SRB_BufLen;
    sg_reply_hdr = (struct sg_header *) HeapAlloc(GetProcessHeap(), 0, out_len);
    memset(sg_reply_hdr, 0, SCSI_OFF);
    sg_hd->reply_len = out_len;
  }
  else {
    out_len = SCSI_OFF;
    sg_reply_hdr = (struct sg_header *) HeapAlloc(GetProcessHeap(), 0, out_len);
    memset(sg_reply_hdr, 0, SCSI_OFF);
    sg_hd->reply_len = out_len;
  }

  status = write(fd, sg_hd, in_len);
  if (status < 0 || status != in_len) {
      int save_error = errno;

    WARN("Not enough bytes written to scsi device bytes=%d .. %d\n", in_len, status);
    if (status < 0) {
		if (save_error == ENOMEM) {
	    MESSAGE("ASPI: Linux generic scsi driver\n  You probably need to re-compile your kernel with a larger SG_BIG_BUFF value (sg.h)\n  Suggest 130560\n");
	}
#ifdef HAVE_STRERROR
		WARN("error:= '%s'\n", strerror(save_error));
#else
		WARN("error:= %d\n", save_error);
#endif
    }
    goto error_exit;
  }

  status = read(fd, sg_reply_hdr, out_len);
  if (status < 0 || status != out_len) {
    WARN("not enough bytes read from scsi device%d\n", status);
    goto error_exit;
  }

  if (sg_reply_hdr->result != 0) {
    error_code = sg_reply_hdr->result;
    WARN("reply header error (%d)\n", sg_reply_hdr->result);
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

    /* CDB is fixed in WNASPI32 */
    memcpy(&lpPRB->CDBByte[16], &sg_reply_hdr->sense_buffer[0], sense_len);

    TRACE("CDB is %d bytes long\n", lpPRB->SRB_CDBLen );
    ASPI_PrintSenseArea(lpPRB);
  }

  lpPRB->SRB_Status = SS_COMP;
  lpPRB->SRB_HaStat = HASTAT_OK;
  lpPRB->SRB_TargStat = sg_reply_hdr->target_status << 1;

  /* FIXME: Should this be != 0 maybe? */
  if( lpPRB->SRB_TargStat == 2 )
    lpPRB->SRB_Status = SS_ERR;

  ASPI_DebugPrintResult(lpPRB);
  /* now do posting */

  if (lpPRB->SRB_PostProc) {
    if (ASPI_POSTING(lpPRB)) {
      TRACE("Post Routine (%lx) called\n", (DWORD) lpPRB->SRB_PostProc);
      (*lpPRB->SRB_PostProc)(lpPRB);
    }
    else
    if (lpPRB->SRB_Flags & SRB_EVENT_NOTIFY) {
      TRACE("Setting event %04x\n", (HANDLE)lpPRB->SRB_PostProc);
      SetEvent((HANDLE)lpPRB->SRB_PostProc); /* FIXME: correct ? */
    }
  }
  HeapFree(GetProcessHeap(), 0, sg_reply_hdr);
  HeapFree(GetProcessHeap(), 0, sg_hd);
  return SS_PENDING;
  /* In real WNASPI32 stuff really is always pending because ASPI does things
     in the background, but we are not doing that (yet) */
  
error_exit:
  if (error_code == EBUSY) {
      lpPRB->SRB_Status = SS_ASPI_IS_BUSY;
      TRACE("Device busy\n");
  }
  else {
      WARN("Failed\n");
      lpPRB->SRB_Status = SS_ERR;
  }

  /* I'm not sure exactly error codes work here
   * We probably should set lpPRB->SRB_TargStat, SRB_HaStat ?
   */
  WARN("error_exit\n");
  HeapFree(GetProcessHeap(), 0, sg_reply_hdr);
  HeapFree(GetProcessHeap(), 0, sg_hd);
  return lpPRB->SRB_Status;
}

#endif /* defined(linux) */


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
DWORD WINAPI GetASPI32SupportInfo()
{
    return ((SS_COMP << 8) | 1); /* FIXME: get # of host adapters installed */
}


/***********************************************************************
 *             SendASPI32Command32 (WNASPI32.1)
 */
DWORD __cdecl SendASPI32Command(LPSRB lpSRB)
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
    FIXME("ASPI: Partially implemented SC_HA_INQUIRY for adapter %d.\n", lpSRB->inquiry.SRB_HaId);
    return SS_COMP;
  case SC_GET_DEV_TYPE: {
    /* FIXME: We should return SS_NO_DEVICE if the device is not configured */
    /* FIXME: We should return SS_INVALID_HA if HostAdapter!=0 */
    SRB		tmpsrb;
    char	inqbuf[200];

    memset(&tmpsrb,0,sizeof(tmpsrb));

    tmpsrb.cmd.SRB_Cmd = SC_EXEC_SCSI_CMD;
#define X(x) tmpsrb.cmd.SRB_##x = lpSRB->devtype.SRB_##x
    X(Status);X(HaId);X(Flags);X(Target);X(Lun);
#undef X
    tmpsrb.cmd.SRB_BufLen	= sizeof(inqbuf);
    tmpsrb.cmd.SRB_Flags	= 8;/*target->host data. FIXME: anything more?*/
    tmpsrb.cmd.SRB_BufPointer	= inqbuf;
    tmpsrb.cmd.CDBByte[0]	= 0x12; /* INQUIRY  */
    tmpsrb.cmd.CDBByte[4]	= sizeof(inqbuf);
    tmpsrb.cmd.SRB_CDBLen	= 6;
    ASPI_ExecScsiCmd(&tmpsrb.cmd);
#define X(x) lpSRB->devtype.SRB_##x = tmpsrb.cmd.SRB_##x
    X(Status);
#undef X
    lpSRB->devtype.SRB_DeviceType = inqbuf[0]&0x1f;
    TRACE("returning devicetype %d for target %d\n",inqbuf[0]&0x1f,tmpsrb.cmd.SRB_Target);
    break;
  }
  case SC_EXEC_SCSI_CMD:
    return ASPI_ExecScsiCmd(&lpSRB->cmd);
    break;
  case SC_ABORT_SRB:
    FIXME("Not implemented SC_ABORT_SRB\n");
    break;
  case SC_RESET_DEV:
    FIXME("Not implemented SC_RESET_DEV\n");
    break;
#ifdef SC_GET_DISK_INFO
  case SC_GET_DISK_INFO:
    /* NT Doesn't implement this either.. so don't feel bad */
    WARN("Not implemented SC_GET_DISK_INFO\n");
    break;
#endif
  default:
    WARN("Unknown command %d\n", lpSRB->common.SRB_Cmd);
  }
  return SS_INVALID_SRB;
#else
  return SS_INVALID_SRB;
#endif
}


/***********************************************************************
 *             GetASPI32DLLVersion32   (WNASPI32.3)
 */

DWORD WINAPI GetASPI32DLLVersion()
{
#ifdef linux
        return (DWORD)1;
#else
        return (DWORD)0;
#endif
}

