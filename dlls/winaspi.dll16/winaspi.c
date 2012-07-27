/*
 * Copyright 1997 Bruce Milner
 * Copyright 1998 Andreas Mohr
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wine/windef16.h"
#include "wine/winbase16.h"
#include "winreg.h"
#include "wownt32.h"
#include "aspi.h"
#include "wine/winaspi.h"
#include "wine/debug.h"


/* FIXME!
 * 1) Residual byte length reporting not handled
 * 2) Make this code re-entrant for multithreading
 * 3) Only linux supported so far
 */

#ifdef linux

/* Copy of info from 2.2.x kernel */
#define SG_MAX_SENSE 16   /* too little, unlikely to change in 2.2.x */

struct sg_header
{
    int pack_len;    /* [o] reply_len (ie useless), ignored as input */
    int reply_len;   /* [i] max length of expected reply (inc. sg_header) */
    int pack_id;     /* [io] id number of packet (use ints >= 0) */
    int result;      /* [o] 0==ok, else (+ve) Unix errno (best ignored) */
    unsigned int twelve_byte:1;
        /* [i] Force 12 byte command length for group 6 & 7 commands  */
    unsigned int target_status:5;   /* [o] scsi status from target */
    unsigned int host_status:8;     /* [o] host status (see "DID" codes) */
    unsigned int driver_status:8;   /* [o] driver status+suggestion */
    unsigned int other_flags:10;    /* unused */
    unsigned char sense_buffer[SG_MAX_SENSE]; /* [o] Output in 3 cases:
           when target_status is CHECK_CONDITION or
           when target_status is COMMAND_TERMINATED or
           when (driver_status & DRIVER_SENSE) is true. */
};      /* This structure is 36 bytes long on i386 */

#define SCSI_OFF sizeof(struct sg_header)

#define PTR_TO_LIN(ptr,mode) \
  ((mode) == ASPI_DOS ? ((void*)(((unsigned int)SELECTOROF(ptr) << 4) + OFFSETOF(ptr))) : MapSL(ptr))

WINE_DEFAULT_DEBUG_CHANNEL(aspi);

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

static FARPROC16 ASPIChainFunc = NULL;
static WORD HA_Count = 1; /* host adapter count; FIXME: detect it */

static int
ASPI_OpenDevice16(SRB_ExecSCSICmd16 *prb)
{
    HKEY hkey;
    int	fd;
    char	idstr[50];
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
    sprintf( idstr, "Software\\Wine\\Wine\\Config\\scsi c%1dt%1dd%1d",
             prb->SRB_HaId, prb->SRB_Target, prb->SRB_Lun);

    device_str[0] = 0;
    if (!RegOpenKeyExA( HKEY_LOCAL_MACHINE, idstr, 0, KEY_ALL_ACCESS, &hkey ))
    {
        DWORD type, count = sizeof(device_str);
        if (RegQueryValueExA( hkey, "Device", 0, &type, (LPBYTE)device_str, &count )) device_str[0] = 0;
        RegCloseKey( hkey );
    }

    if (!device_str[0])
    {
	TRACE("Trying to open unlisted scsi device %s\n", idstr);
	return -1;
    }

    TRACE("Opening device %s=%s\n", idstr, device_str);

    fd = open(device_str, O_RDWR);
    if (fd == -1) {
	int save_error = errno;
	ERR("Error opening device %s, error '%s'\n", device_str, strerror(save_error));
	return -1;
    }

    /* device is now open */
    curr = HeapAlloc( GetProcessHeap(), 0, sizeof(ASPI_DEVICE_INFO) );
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
ASPI_DebugPrintCmd(SRB_ExecSCSICmd16 *prb, UINT16 mode)
{
  int	i;
  BYTE *cdb;
  BYTE *lpBuf = PTR_TO_LIN( prb->SRB_BufPointer, mode );

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
  else if (NO_DATA_TRANSFERRED(prb)) {
    TRACE("\tData transfer: none\n");
  }
  else {
    WARN("\tTransfer by scsi cmd. Length not checked\n");
  }

  TRACE("\tResidual byte length reporting %s\n", prb->SRB_Flags & 0x4 ? "enabled" : "disabled");
  TRACE("\tLinking %s\n", prb->SRB_Flags & 0x2 ? "enabled" : "disabled");
  TRACE("\tPosting %s\n", prb->SRB_Flags & 0x1 ? "enabled" : "disabled");
  TRACE("Target: %d\n", prb->SRB_Target);
  TRACE("Lun: %d\n", prb->SRB_Lun);
  TRACE("BufLen: %d\n", prb->SRB_BufLen);
  TRACE("SenseLen: %d\n", prb->SRB_SenseLen);
  TRACE("BufPtr: %x (%p)\n", prb->SRB_BufPointer, lpBuf);
  TRACE("LinkPointer %x\n", prb->SRB_Rsvd1);
  TRACE("CDB Length: %d\n", prb->SRB_CDBLen);
  TRACE("POST Proc: %x\n", (DWORD) prb->SRB_PostProc);
  cdb = prb->CDBByte;
  if (TRACE_ON(aspi))
  {
      TRACE("CDB buffer[");
      for (i = 0; i < prb->SRB_CDBLen; i++) {
          if (i != 0) TRACE(",");
          TRACE("%02x", *cdb++);
      }
      TRACE("]\n");
  }
}

static void
ASPI_PrintSenseArea16(SRB_ExecSCSICmd16 *prb)
{
  int	i;
  BYTE *cdb;

  if (TRACE_ON(aspi))
  {
      cdb = prb->CDBByte;
      TRACE("SenseArea[");
      for (i = 0; i < prb->SRB_SenseLen; i++) {
          if (i) TRACE(",");
          TRACE("%02x", *cdb++);
      }
      TRACE("]\n");
  }
}

static void
ASPI_DebugPrintResult(SRB_ExecSCSICmd16 *prb, UINT16 mode)
{
  BYTE *lpBuf = PTR_TO_LIN( prb->SRB_BufPointer, mode );

  switch (prb->CDBByte[0]) {
  case CMD_INQUIRY:
    TRACE("Vendor: '%s'\n", lpBuf + INQUIRY_VENDOR);
    break;
  case CMD_TEST_UNIT_READY:
    ASPI_PrintSenseArea16(prb);
    break;
  }
}

static WORD
ASPI_ExecScsiCmd(DWORD ptrPRB, UINT16 mode)
{
  SRB_ExecSCSICmd16 *lpPRB = PTR_TO_LIN( ptrPRB, mode );
  struct sg_header *sg_hd, *sg_reply_hdr;
  int	status;
  BYTE *lpBuf = 0;
  int	in_len, out_len;
  int	error_code = 0;
  int	fd;

  ASPI_DebugPrintCmd(lpPRB, mode);

  fd = ASPI_OpenDevice16(lpPRB);
  if (fd == -1) {
      WARN("Failed: could not open device. Device permissions !?\n");
      lpPRB->SRB_Status = SS_ERR;
      return SS_ERR;
  }

  sg_hd = NULL;
  sg_reply_hdr = NULL;

  lpPRB->SRB_Status = SS_PENDING;
  lpBuf = PTR_TO_LIN( lpPRB->SRB_BufPointer, mode );

  if (!lpPRB->SRB_CDBLen) {
      WARN("Failed: lpPRB->SRB_CDBLen = 0.\n");
      lpPRB->SRB_Status = SS_ERR;
      return SS_ERR;
  }

  /* build up sg_header + scsi cmd */
  if (HOST_TO_TARGET(lpPRB)) {
    /* send header, command, and then data */
    in_len = SCSI_OFF + lpPRB->SRB_CDBLen + lpPRB->SRB_BufLen;
    sg_hd = HeapAlloc(GetProcessHeap(), 0, in_len);
    memset(sg_hd, 0, SCSI_OFF);
    memcpy(sg_hd + 1, lpPRB->CDBByte, lpPRB->SRB_CDBLen);
    if (lpPRB->SRB_BufLen) {
      memcpy(((BYTE *) sg_hd) + SCSI_OFF + lpPRB->SRB_CDBLen, lpBuf, lpPRB->SRB_BufLen);
    }
  }
  else {
    /* send header and command - no data */
    in_len = SCSI_OFF + lpPRB->SRB_CDBLen;
    sg_hd = HeapAlloc(GetProcessHeap(), 0, in_len);
    memset(sg_hd, 0, SCSI_OFF);
    memcpy(sg_hd + 1, lpPRB->CDBByte, lpPRB->SRB_CDBLen);
  }

  if (TARGET_TO_HOST(lpPRB)) {
    out_len = SCSI_OFF + lpPRB->SRB_BufLen;
    sg_reply_hdr = HeapAlloc(GetProcessHeap(), 0, out_len);
    memset(sg_reply_hdr, 0, SCSI_OFF);
    sg_hd->reply_len = out_len;
  }
  else {
    out_len = SCSI_OFF;
    sg_reply_hdr = HeapAlloc(GetProcessHeap(), 0, out_len);
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
		WARN("error:= '%s'\n", strerror(save_error));
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
    memcpy(lpBuf, sg_reply_hdr + 1, lpPRB->SRB_BufLen);
  }

  /* copy in sense buffer to amount that is available in client */
  if (lpPRB->SRB_SenseLen) {
    int sense_len = lpPRB->SRB_SenseLen;
    if (lpPRB->SRB_SenseLen > 16)
      sense_len = 16;
    memcpy(SENSE_BUFFER(lpPRB), sg_reply_hdr->sense_buffer, sense_len);
  }


  lpPRB->SRB_Status = SS_COMP;
  lpPRB->SRB_HaStat = HASTAT_OK;
  lpPRB->SRB_TargStat = STATUS_GOOD;

  /* now do posting */

  if (ASPI_POSTING(lpPRB) && lpPRB->SRB_PostProc) {
    TRACE("Post Routine (%x) called\n", (DWORD) lpPRB->SRB_PostProc);
    switch (mode)
    {
      case ASPI_DOS:
      {
	SEGPTR spPRB = MapLS(lpPRB);

        WOWCallback16((DWORD)lpPRB->SRB_PostProc, spPRB);
	UnMapLS(spPRB);
	break;
      }
      case ASPI_WIN16:
        WOWCallback16((DWORD)lpPRB->SRB_PostProc, ptrPRB);
	break;
    }
  }

  HeapFree(GetProcessHeap(), 0, sg_reply_hdr);
  HeapFree(GetProcessHeap(), 0, sg_hd);
  ASPI_DebugPrintResult(lpPRB, mode);
  return SS_COMP;

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
#endif


/***********************************************************************
 *             GetASPISupportInfo   (WINASPI.1)
 */

WORD WINAPI GetASPISupportInfo16(void)
{
#ifdef linux
    TRACE("GETASPISupportInfo16\n");
    /* high byte SS_COMP - low byte number of host adapters */
    return ((SS_COMP << 8) | HA_Count);
#else
    return ((SS_NO_ASPI << 8) | 0);
#endif
}


static DWORD ASPI_SendASPICommand(DWORD ptrSRB, UINT16 mode)
{
#ifdef linux
  LPSRB16 lpSRB = PTR_TO_LIN( ptrSRB, mode );
  static const char szId[] = "Wine ASPI16";
  static const char szWh[] = "Wine host";

  if (mode == ASPI_WIN16 && ASPIChainFunc)
  {
      /* This is not the post proc, it's the chain proc this time */
      DWORD ret = WOWCallback16((DWORD)ASPIChainFunc, ptrSRB);
      if (ret)
      {
          lpSRB->inquiry.SRB_Status = SS_INVALID_SRB;
          return ret;
      }
  }

  switch (lpSRB->common.SRB_Cmd) {
  case SC_HA_INQUIRY:
    lpSRB->inquiry.SRB_Status = SS_COMP;       /* completed successfully */
    if (lpSRB->inquiry.SRB_55AASignature == 0x55aa) {
	TRACE("Extended request detected (Adaptec's ASPIxDOS).\nWe don't support it at the moment.\n");
    }
    lpSRB->inquiry.SRB_ExtBufferSize = 0x2000; /* bogus value */
    lpSRB->inquiry.HA_Count = HA_Count;
    lpSRB->inquiry.HA_SCSI_ID = 7;             /* not always ID 7 */
    memcpy(lpSRB->inquiry.HA_ManagerId, szId, sizeof szId); /* max 15 chars */
    memcpy(lpSRB->inquiry.HA_Identifier, szWh, sizeof szWh); /* FIXME: return host
adapter name */
    memset(lpSRB->inquiry.HA_Unique, 0, 16); /* default HA_Unique content */
    lpSRB->inquiry.HA_Unique[6] = 0x02; /* Maximum Transfer Length (128K, Byte> 4-7) */
    FIXME("ASPI: Partially implemented SC_HA_INQUIRY for adapter %d.\n", lpSRB->inquiry.SRB_HaId);
    return SS_COMP;
  case SC_GET_DEV_TYPE:
    FIXME("Not implemented SC_GET_DEV_TYPE\n");
    break;
  case SC_EXEC_SCSI_CMD:
    return ASPI_ExecScsiCmd(ptrSRB, mode);
  case SC_RESET_DEV:
    FIXME("Not implemented SC_RESET_DEV\n");
    break;
  default:
    FIXME("Unknown command %d\n", lpSRB->common.SRB_Cmd);
  }
#endif
  return SS_INVALID_SRB;
}


/***********************************************************************
 *             SendASPICommand   (WINASPI.2)
 */
WORD WINAPI SendASPICommand16(SEGPTR segptr_srb)
{
    return ASPI_SendASPICommand(segptr_srb, ASPI_WIN16);
}


/***********************************************************************
 *             InsertInASPIChain   (WINASPI.3)
 */
WORD WINAPI InsertInASPIChain16(BOOL16 remove, FARPROC16 pASPIChainFunc)
{
#ifdef linux
    if (remove) /* Remove */
    {
	if (ASPIChainFunc == pASPIChainFunc)
	{
	    ASPIChainFunc = NULL;
	    return SS_COMP;
	}
    }
    else /* Insert */
    {
	if (ASPIChainFunc == NULL)
	{
	    ASPIChainFunc = pASPIChainFunc;
	    return SS_COMP;
	}
    }
#endif
    return SS_ERR;
}


/***********************************************************************
 *             GETASPIDLLVERSION   (WINASPI.4)
 */

DWORD WINAPI GetASPIDLLVersion16(void)
{
#ifdef linux
	return 2;
#else
	return 0;
#endif
}
