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

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "aspi.h"
#include "wnaspi32.h"
#include "winescsi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(aspi);

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
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &ASPI_CritSection,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": ASPI_CritSection") }
};
static CRITICAL_SECTION ASPI_CritSection = { &critsect_debug, -1, 0, 0, 0, 0 };


BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
	switch( fdwReason )
	{
	case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstDLL);
            SCSI_Init();
            break;
	case DLL_PROCESS_DETACH:
            if (fImpLoad) break;
            DeleteCriticalSection( &ASPI_CritSection );
            break;
	}
	return TRUE;
}


static int
ASPI_OpenDevice(SRB_ExecSCSICmd *prb)
{
    int	fd;
    DWORD	hc;
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

    if (prb->SRB_HaId >= ASPI_GetNumControllers())
	return -1;

    hc = ASPI_GetHCforController( prb->SRB_HaId );
    fd = SCSI_OpenDevice( HIWORD(hc), LOWORD(hc), prb->SRB_Target, prb->SRB_Lun);

    if (fd == -1)
	return -1;

    /* device is now open */
    /* FIXME: Let users specify SCSI timeout in registry */
    SCSI_LinuxSetTimeout( fd, SCSI_DEFAULT_TIMEOUT );

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
  int	i;
  BYTE *cdb;

  switch (prb->CDBByte[0]) {
  case CMD_INQUIRY:
    TRACE("INQUIRY {\n");
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
    WARN("\tTransfer by scsi cmd. Length not checked.\n");
  }

  TRACE("\tResidual byte length reporting %s\n", prb->SRB_Flags & 0x4 ? "enabled" : "disabled");
  TRACE("\tLinking %s\n", prb->SRB_Flags & 0x2 ? "enabled" : "disabled");
  TRACE("\tPosting %s\n", prb->SRB_Flags & 0x1 ? "enabled" : "disabled");
  TRACE("Target: %d\n", prb->SRB_Target);
  TRACE("Lun: %d\n", prb->SRB_Lun);
  TRACE("BufLen: %d\n", prb->SRB_BufLen);
  TRACE("SenseLen: %d\n", prb->SRB_SenseLen);
  TRACE("BufPtr: %p\n", prb->SRB_BufPointer);
  TRACE("CDB Length: %d\n", prb->SRB_CDBLen);
  TRACE("POST Proc: %p\n", prb->SRB_PostProc);
  cdb = &prb->CDBByte[0];
  if (TRACE_ON(aspi)) {
      TRACE("CDB buffer[");
      for (i = 0; i < prb->SRB_CDBLen; i++) {
          if (i != 0) TRACE(",");
          TRACE("%02x", *cdb++);
      }
      TRACE("]\n");
  }
}

static void
ASPI_PrintCDBArea(SRB_ExecSCSICmd *prb)
{
    if (TRACE_ON(aspi))
    {
	int i;
        TRACE("CDB[");
        for (i = 0; i < prb->SRB_CDBLen; i++) {
            if (i) TRACE(",");
            TRACE("%02x", prb->CDBByte[i]);
        }
        TRACE("]\n");
    }
}

static void
ASPI_PrintSenseArea(SRB_ExecSCSICmd *prb)
{
  int	i;
  BYTE	*rqbuf = prb->SenseArea;

  if (TRACE_ON(aspi))
  {
      TRACE("Request Sense reports:\n");
      if ((rqbuf[0]&0x7f)!=0x70) {
	      TRACE("\tInvalid sense header: 0x%02x instead of 0x70\n", rqbuf[0]&0x7f);
	      return;
      }
      TRACE("\tCurrent command read filemark: %s\n",(rqbuf[2]&0x80)?"yes":"no");
      TRACE("\tEarly warning passed: %s\n",(rqbuf[2]&0x40)?"yes":"no");
      TRACE("\tIncorrect blocklength: %s\n",(rqbuf[2]&0x20)?"yes":"no");
      TRACE("\tSense Key: %d\n",rqbuf[2]&0xf);
      if (rqbuf[0]&0x80)
	TRACE("\tResidual Length: %d\n",rqbuf[3]*0x1000000+rqbuf[4]*0x10000+rqbuf[5]*0x100+rqbuf[6]);
      TRACE("\tAdditional Sense Length: %d\n",rqbuf[7]);
      TRACE("\tAdditional Sense Code: %d\n",rqbuf[12]);
      TRACE("\tAdditional Sense Code Qualifier: %d\n",rqbuf[13]);
      if (rqbuf[15]&0x80) {
	TRACE("\tIllegal Param is in %s\n",(rqbuf[15]&0x40)?"the CDB":"the Data Out Phase");
	if (rqbuf[15]&0x8) {
	  TRACE("Pointer at %d, bit %d\n",rqbuf[16]*256+rqbuf[17],rqbuf[15]&0x7);
	}
      }
      TRACE("SenseArea[");
      for (i = 0; i < prb->SRB_SenseLen; i++) {
	if (i) TRACE(",");
	TRACE("%02x", *rqbuf++);
      }
      TRACE("]\n");
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

/* Posting must be done in such a way that as soon as the SRB_Status is set
 * we don't touch the SRB anymore because it could possibly be freed
 * if the app is doing ASPI polling
 */
static DWORD
WNASPI32_DoPosting( SRB_ExecSCSICmd *lpPRB, DWORD status )
{
	void (*SRB_PostProc)(SRB_ExecSCSICmd *) = lpPRB->SRB_PostProc;
	BYTE SRB_Flags = lpPRB->SRB_Flags;
	if( status == SS_PENDING )
	{
		WARN("Tried posting SS_PENDING\n");
		return SS_PENDING;
	}
	lpPRB->SRB_Status = status;
	/* lpPRB is NOT safe, it could be freed in another thread */

	if (SRB_PostProc)
	{
		if (SRB_Flags & 0x1)
		{
			TRACE("Post Routine (%p) called\n", SRB_PostProc);
			/* Even though lpPRB could have been freed by
			 * the program.. that's unlikely if it planned
			 * to use it in the PostProc
			 */
			(*SRB_PostProc)(lpPRB);
		}
		else if (SRB_Flags & SRB_EVENT_NOTIFY) {
			TRACE("Setting event %p\n", SRB_PostProc);
			SetEvent(SRB_PostProc);
		}
	}
	return SS_PENDING;
}

static WORD
ASPI_ExecScsiCmd(SRB_ExecSCSICmd *lpPRB)
{
  struct sg_header *sg_hd, *sg_reply_hdr;
  WORD ret;
  DWORD	status;
  int	in_len, out_len;
  int   num_controllers = 0;
  int	error_code = 0;
  int	fd;
  DWORD SRB_Status;

  num_controllers = ASPI_GetNumControllers();
  if (lpPRB->SRB_HaId >= num_controllers) {
      WARN("Failed: Wanted hostadapter with index %d, but we have only %d.\n",
	  lpPRB->SRB_HaId, num_controllers
      );
      return WNASPI32_DoPosting( lpPRB, SS_INVALID_HA );
  }
  fd = ASPI_OpenDevice(lpPRB);
  if (fd == -1) {
      return WNASPI32_DoPosting( lpPRB, SS_NO_DEVICE );
  }
    
  /* FIXME: hackmode */
#define MAKE_TARGET_TO_HOST(lpPRB) \
  	if (!TARGET_TO_HOST(lpPRB)) { \
	    WARN("program was not sending target_to_host for cmd %x (flags=%x),correcting.\n",lpPRB->CDBByte[0],lpPRB->SRB_Flags); \
	    lpPRB->SRB_Flags |= 0x08; \
	}
#define MAKE_HOST_TO_TARGET(lpPRB) \
  	if (!HOST_TO_TARGET(lpPRB)) { \
	    WARN("program was not sending host_to_target for cmd %x (flags=%x),correcting.\n",lpPRB->CDBByte[0],lpPRB->SRB_Flags); \
	    lpPRB->SRB_Flags |= 0x10; \
	}
  switch (lpPRB->CDBByte[0]) {
  case 0x12: /* INQUIRY */
  case 0x5a: /* MODE_SENSE_10 */
  case 0xa4: /* REPORT_KEY (DVD) MMC-2 */
  case 0xad: /* READ DVD STRUCTURE MMC-2 */
        MAKE_TARGET_TO_HOST(lpPRB)
	break;
  case 0xa3: /* SEND KEY (DVD) MMC-2 */
        MAKE_HOST_TO_TARGET(lpPRB)
	break;
  default:
	if ((((lpPRB->SRB_Flags & 0x18) == 0x00) ||
	     ((lpPRB->SRB_Flags & 0x18) == 0x18)
	    ) && lpPRB->SRB_BufLen
	) {
            FIXME("command 0x%02x, no data transfer specified, but buflen is %d!!!\n",lpPRB->CDBByte[0],lpPRB->SRB_BufLen);
	}
	break;
  }
  ASPI_DebugPrintCmd(lpPRB);

  sg_hd = NULL;
  sg_reply_hdr = NULL;

  lpPRB->SRB_Status = SS_PENDING;

  if (!lpPRB->SRB_CDBLen) {
      ERR("Failed: lpPRB->SRB_CDBLen = 0.\n");
      return WNASPI32_DoPosting( lpPRB, SS_INVALID_SRB );
  }

  /* build up sg_header + scsi cmd */
  if (HOST_TO_TARGET(lpPRB)) {
    /* send header, command, and then data */
    in_len = SCSI_OFF + lpPRB->SRB_CDBLen + lpPRB->SRB_BufLen;
    sg_hd = HeapAlloc(GetProcessHeap(), 0, in_len);
    memset(sg_hd, 0, SCSI_OFF);
    memcpy(sg_hd + 1, &lpPRB->CDBByte[0], lpPRB->SRB_CDBLen);
    if (lpPRB->SRB_BufLen) {
      memcpy(((BYTE *) sg_hd) + SCSI_OFF + lpPRB->SRB_CDBLen, lpPRB->SRB_BufPointer, lpPRB->SRB_BufLen);
    }
  }
  else {
    /* send header and command - no data */
    in_len = SCSI_OFF + lpPRB->SRB_CDBLen;
    sg_hd = HeapAlloc(GetProcessHeap(), 0, in_len);
    memset(sg_hd, 0, SCSI_OFF);
    memcpy(sg_hd + 1, &lpPRB->CDBByte[0], lpPRB->SRB_CDBLen);
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

  SCSI_Fix_CMD_LEN(fd, lpPRB->CDBByte[0], lpPRB->SRB_CDBLen);

  if(!SCSI_LinuxDeviceIo( fd,
			  sg_hd, in_len,
			  sg_reply_hdr, out_len,
			  &status) )
  {
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
    memcpy(lpPRB->SenseArea, &sg_reply_hdr->sense_buffer[0], sense_len);

    ASPI_PrintCDBArea(lpPRB);
    ASPI_PrintSenseArea(lpPRB);
  }

  SRB_Status = SS_COMP;
  lpPRB->SRB_HaStat = HASTAT_OK;
  lpPRB->SRB_TargStat = sg_reply_hdr->target_status << 1;

  HeapFree(GetProcessHeap(), 0, sg_reply_hdr);
  HeapFree(GetProcessHeap(), 0, sg_hd);

  /* FIXME: Should this be != 0 maybe? */
  if( lpPRB->SRB_TargStat == 2 ) {
    SRB_Status = SS_ERR;
    switch (lpPRB->CDBByte[0]) {
    case 0xa4: /* REPORT_KEY (DVD) MMC-2 */
    case 0xa3: /* SEND KEY (DVD) MMC-2 */
          SRB_Status = SS_COMP;
	  lpPRB->SRB_TargStat = 0;
	  FIXME("Program wants to do DVD Region switching, but fails (non compliant DVD drive). Ignoring....\n");
	  break;
    }
  }

  ASPI_DebugPrintResult(lpPRB);
  /* now do posting */
  ret = WNASPI32_DoPosting( lpPRB, SRB_Status );

  switch (lpPRB->CDBByte[0]) {
  case CMD_INQUIRY:
      if (SRB_Status == SS_COMP)
	  return SS_COMP; /* some junk expects ss_comp here. */
      /*FALLTHROUGH*/
  default:
      break;
  }

  /* In real WNASPI32 stuff really is always pending because ASPI does things
     in the background, but we are not doing that (yet) */

  return ret;

error_exit:
  SRB_Status = SS_ERR;
  if (error_code == EBUSY) {
      WNASPI32_DoPosting( lpPRB, SS_ASPI_IS_BUSY );
      TRACE("Device busy\n");
  } else
      FIXME("Failed\n");

  /* I'm not sure exactly error codes work here
   * We probably should set lpPRB->SRB_TargStat, SRB_HaStat ?
   */
  WARN("error_exit\n");
  HeapFree(GetProcessHeap(), 0, sg_reply_hdr);
  HeapFree(GetProcessHeap(), 0, sg_hd);
  WNASPI32_DoPosting( lpPRB, SRB_Status );
  return SS_PENDING;
}

#endif /* defined(linux) */


/*******************************************************************
 *     GetASPI32SupportInfo		[WNASPI32.1]
 *
 * Checks if the ASPI subsystem is initialized correctly.
 *
 * RETURNS
 *    HIWORD: 0.
 *    HIBYTE of LOWORD: status (SS_COMP or SS_FAILED_INIT)
 *    LOBYTE of LOWORD: # of host adapters.
 */
DWORD __cdecl GetASPI32SupportInfo(void)
{
    DWORD controllers = ASPI_GetNumControllers();

    if (!controllers)
	return SS_NO_ADAPTERS << 8;
    return (SS_COMP << 8) | controllers ;
}

/***********************************************************************
 *             SendASPI32Command (WNASPI32.2)
 */
DWORD __cdecl SendASPI32Command(LPSRB lpSRB)
{
#ifdef linux
  static const char szId[] = "ASPI for WIN32";
  static const char szWh[] = "Wine host";
  switch (lpSRB->common.SRB_Cmd) {
  case SC_HA_INQUIRY:
    lpSRB->inquiry.SRB_Status = SS_COMP;       /* completed successfully */
    lpSRB->inquiry.HA_Count = ASPI_GetNumControllers();
    lpSRB->inquiry.HA_SCSI_ID = 7;             /* not always ID 7 */
    memcpy(lpSRB->inquiry.HA_ManagerId, szId, sizeof szId); /* max 15 chars, don't change */
    memcpy(lpSRB->inquiry.HA_Identifier, szWh, sizeof szWh); /* FIXME: return host adapter name */
    memset(lpSRB->inquiry.HA_Unique, 0, 16); /* default HA_Unique content */
    lpSRB->inquiry.HA_Unique[6] = 0x02; /* Maximum Transfer Length (128K, Byte> 4-7) */
    lpSRB->inquiry.HA_Unique[3] = 0x08; /* Maximum number of SCSI targets */
    FIXME("ASPI: Partially implemented SC_HA_INQUIRY for adapter %d.\n", lpSRB->inquiry.SRB_HaId);
    return SS_COMP;

  case SC_GET_DEV_TYPE: {
    /* FIXME: We should return SS_NO_DEVICE if the device is not configured */
    /* FIXME: We should return SS_INVALID_HA if HostAdapter!=0 */
    SRB		tmpsrb;
    unsigned char inqbuf[200];
    DWORD	ret;

    memset(&tmpsrb,0,sizeof(tmpsrb));

    /* Copy header */
    tmpsrb.common = lpSRB->common;

    tmpsrb.cmd.SRB_Flags	|= 8; /* target to host */
    tmpsrb.cmd.SRB_Cmd 		= SC_EXEC_SCSI_CMD;
    tmpsrb.cmd.SRB_Target	= lpSRB->devtype.SRB_Target;
    tmpsrb.cmd.SRB_Lun		= lpSRB->devtype.SRB_Lun;
    tmpsrb.cmd.SRB_BufLen	= sizeof(inqbuf);
    tmpsrb.cmd.SRB_BufPointer	= inqbuf;
    tmpsrb.cmd.CDBByte[0]	= 0x12; /* INQUIRY  */
    				  /* FIXME: handle lun */
    tmpsrb.cmd.CDBByte[4]	= sizeof(inqbuf);
    tmpsrb.cmd.SRB_CDBLen	= 6;

    ret = ASPI_ExecScsiCmd(&tmpsrb.cmd);

    lpSRB->devtype.SRB_Status	= tmpsrb.cmd.SRB_Status;
    lpSRB->devtype.SRB_DeviceType = inqbuf[0]&0x1f;

    TRACE("returning devicetype %d for target %d\n",inqbuf[0]&0x1f,tmpsrb.cmd.SRB_Target);
    if (ret!=SS_PENDING) /* Any error is passed down directly */
	return ret;
    /* FIXME: knows that the command is finished already, pass final Status */
    return tmpsrb.cmd.SRB_Status;
  }
  case SC_EXEC_SCSI_CMD:
    return ASPI_ExecScsiCmd(&lpSRB->cmd);
  case SC_ABORT_SRB:
    FIXME("Not implemented SC_ABORT_SRB\n");
    break;
  case SC_RESET_DEV:
    FIXME("Not implemented SC_RESET_DEV\n");
    break;
  case SC_GET_DISK_INFO:
    /* here we have to find out the int13 / bios association.
     * We just say we do not have any.
     */
    FIXME("SC_GET_DISK_INFO always return 'int13 unassociated disk'.\n");
    lpSRB->diskinfo.SRB_DriveFlags = 0; /* disk is not int13 served */
    return SS_COMP;
  default:
    FIXME("Unknown command %d\n", lpSRB->common.SRB_Cmd);
  }
  return SS_INVALID_SRB;
#else
  return SS_INVALID_SRB;
#endif
}


/***********************************************************************
 *             GetASPI32DLLVersion   (WNASPI32.4)
 */
DWORD __cdecl GetASPI32DLLVersion(void)
{
#ifdef linux
	TRACE("Returning version 1\n");
        return 1;
#else
	FIXME("Please add SCSI support for your operating system, returning 0\n");
        return 0;
#endif
}

/***********************************************************************
 *             GetASPI32Buffer   (WNASPI32.8)
 * Supposed to return a DMA capable large SCSI buffer.
 * Our implementation does not use those at all, all buffer stuff is
 * done in the kernel SG device layer. So we just heapalloc the buffer.
 */
BOOL __cdecl GetASPI32Buffer(PASPI32BUFF pab)
{
    pab->AB_BufPointer = HeapAlloc(GetProcessHeap(),
	    	pab->AB_ZeroFill?HEAP_ZERO_MEMORY:0,
		pab->AB_BufLen
    );
    if (!pab->AB_BufPointer) return FALSE;
    return TRUE;
}

/***********************************************************************
 *             FreeASPI32Buffer   (WNASPI32.14)
 */
BOOL __cdecl FreeASPI32Buffer(PASPI32BUFF pab)
{
    HeapFree(GetProcessHeap(),0,pab->AB_BufPointer);
    return TRUE;
}

/***********************************************************************
 *             TranslateASPI32Address   (WNASPI32.7)
 */
BOOL __cdecl TranslateASPI32Address(LPDWORD pdwPath, LPDWORD pdwDEVNODE)
{
    FIXME("(%p, %p), stub !\n", pdwPath, pdwDEVNODE);
    return TRUE;
}
