/*
 * Copyright 2000 David Elliott
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

#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wine/windef16.h"
#include "wine/winaspi.h"
#include "wine/debug.h"
#include "dosexe.h"
#include "winerror.h"

WINE_DEFAULT_DEBUG_CHANNEL(aspi);

static HINSTANCE hWNASPI32 = INVALID_HANDLE_VALUE;
static DWORD (__cdecl *pSendASPI32Command) (LPSRB) = NULL;

static void
DOSASPI_PostProc( SRB_ExecSCSICmd *lpPRB )
{
	DWORD ptrSRB;
	LPSRB16 lpSRB16;


	memcpy(&ptrSRB,lpPRB->SenseArea + lpPRB->SRB_SenseLen,sizeof(DWORD));
	TRACE("Copying data back to DOS client at 0x%8x\n",ptrSRB);
	lpSRB16 = PTR_REAL_TO_LIN(SELECTOROF(ptrSRB),OFFSETOF(ptrSRB));
	lpSRB16->cmd.SRB_TargStat = lpPRB->SRB_TargStat;
	lpSRB16->cmd.SRB_HaStat = lpPRB->SRB_HaStat;
	memcpy(lpSRB16->cmd.CDBByte + lpSRB16->cmd.SRB_CDBLen,lpPRB->SenseArea,lpSRB16->cmd.SRB_SenseLen);

	/* Now do posting */
	if( lpPRB->SRB_Status == SS_SECURITY_VIOLATION )
	{
		/* SS_SECURITY_VIOLATION isn't defined in DOS ASPI */
		TRACE("Returning SS_NO_DEVICE for SS_SECURITY_VIOLATION\n");
		lpPRB->SRB_Status = SS_NO_DEVICE;
	}

	lpSRB16->cmd.SRB_Status = lpPRB->SRB_Status;
	TRACE("SRB_Status = 0x%x\n", lpPRB->SRB_Status);

	HeapFree(GetProcessHeap(),0,lpPRB);

	if( (lpSRB16->cmd.SRB_Flags & SRB_POSTING) && lpSRB16->cmd.SRB_PostProc )
	{
		CONTEXT ctx;
/* The stack should look like this on entry to proc
 * NOTE: the SDK draws the following diagram bass awkwardly, use this one
 * to avoid being confused.  Remember, the act of pushing something on
 * an intel stack involves decreasing the stack pointer by the size of
 * the data, and then copying the data at the new SP.
 */
/***************************
 * ... Other crap that is already on the stack ...
 * Segment of SRB Pointer		<- SP+6
 * Offset of SRB Pointer		<- SP+4
 * Segment of return address		<- SP+2
 * Offset of return address		<- SP+0
 */
		/* FIXME: I am about 99% sure what is here is correct,
		 * but this code has never been tested (and probably
		 * won't be either until someone finds a DOS program
		 * that actually uses a Post Routine) */

		/* Zero everything */
		memset(&ctx, 0, sizeof(ctx));
                ctx.EFlags |= V86_FLAG;

		/* CS:IP is routine to call */
		ctx.SegCs = SELECTOROF(lpSRB16->cmd.SRB_PostProc);
		ctx.Eip   = OFFSETOF(lpSRB16->cmd.SRB_PostProc);
		/* DPMI_CallRMProc will push the pointer to the stack
		 * it is given (in this case &ptrSRB) with length
		 * 2*sizeof(WORD), that is, it copies the contents
		 * of ptrSRB onto the stack, and decs sp by 2*sizeof(WORD).
		 * After doing that, it pushes the return address
		 * onto the stack (so we don't need to worry about that)
		 * So the stack should be okay for the PostProc
		 */
		if(DPMI_CallRMProc(&ctx, (LPWORD)&ptrSRB, 2, FALSE))
		{
			TRACE("DPMI_CallRMProc returned nonzero (error) status\n");
		}
	} /* if ((SRB_Flags&SRB_POSTING) && SRB_PostProc) */
}

static
DWORD ASPI_SendASPIDOSCommand(DWORD ptrSRB)
{
	PSRB_ExecSCSICmd lpPRB;
	DWORD retval;
	union tagSRB16 * lpSRB16;

	lpSRB16 = PTR_REAL_TO_LIN(SELECTOROF(ptrSRB),OFFSETOF(ptrSRB));

	retval = SS_ERR;
	switch( lpSRB16->common.SRB_Cmd )
	{
	case SC_HA_INQUIRY:
		TRACE("SC_HA_INQUIRY\n");
		/* Format is identical in this case */
		retval = (*pSendASPI32Command)((LPSRB)lpSRB16);
		break;
	case SC_GET_DEV_TYPE:
		TRACE("SC_GET_DEV_TYPE\n");
		/* Format is identical in this case */
		retval = (*pSendASPI32Command)((LPSRB)lpSRB16);
		break;
	case SC_EXEC_SCSI_CMD:
		TRACE("SC_EXEC_SCSI_CMD\n");
		TRACE("Copying data from DOS client at 0x%8x\n",ptrSRB);
		lpPRB = HeapAlloc(GetProcessHeap(),0,sizeof(SRB)+lpSRB16->cmd.SRB_SenseLen+sizeof(DWORD));
#define srb_dos_to_w32(name) \
		lpPRB->SRB_##name = lpSRB16->cmd.SRB_##name

		srb_dos_to_w32(Cmd);
		srb_dos_to_w32(Status);
		srb_dos_to_w32(HaId);
		srb_dos_to_w32(BufLen);
		srb_dos_to_w32(SenseLen);
		srb_dos_to_w32(CDBLen);
		srb_dos_to_w32(Target);
		srb_dos_to_w32(Lun);
#undef srb_dos_to_w32

		/* Allow certain flags to go on to WNASPI32, we also need
		 * to make sure SRB_POSTING is enabled */
		lpPRB->SRB_Flags = SRB_POSTING | (lpSRB16->cmd.SRB_Flags&(SRB_DIR_IN|SRB_DIR_OUT|SRB_ENABLE_RESIDUAL_COUNT));

		/* Pointer to data buffer */
		lpPRB->SRB_BufPointer = PTR_REAL_TO_LIN(SELECTOROF(lpSRB16->cmd.SRB_BufPointer),
                                                        OFFSETOF(lpSRB16->cmd.SRB_BufPointer));
		/* Copy CDB in */
		memcpy(&lpPRB->CDBByte[0],&lpSRB16->cmd.CDBByte[0],lpSRB16->cmd.SRB_CDBLen);

		/* Set post proc to our post proc */
		lpPRB->SRB_PostProc = DOSASPI_PostProc;

		/* Stick the DWORD after all the sense info */
		memcpy(lpPRB->SenseArea + lpPRB->SRB_SenseLen,&ptrSRB,sizeof(DWORD));
		retval = (*pSendASPI32Command)((LPSRB)lpPRB);
		break;
	case SC_ABORT_SRB:
		TRACE("SC_ABORT_SRB\n");
		/* Would need some sort of table of active shit */
		break;
	case SC_RESET_DEV:
		TRACE("SC_RESET_DEV\n");
		break;
	default:
		TRACE("Unknown command code\n");
		break;
	}

	TRACE("Returning %x\n", retval );
	return retval;
}

static void WINAPI ASPI_DOS_func(CONTEXT *context)
{
	WORD *stack = CTX_SEG_OFF_TO_LIN(context, context->SegSs, context->Esp);
	DWORD ptrSRB = *(DWORD *)&stack[2];

	ASPI_SendASPIDOSCommand(ptrSRB);

	/* simulate a normal RETF sequence as required by DPMI CallRMProcFar */
	context->Eip = *(stack++);
	context->SegCs  = *(stack++);
	context->Esp += 2*sizeof(WORD);
}


/**********************************************************************
 *	    ASPIHandler
 *
 * returns the address of a real mode callback to ASPI_DOS_func()
 */
void DOSVM_ASPIHandler( CONTEXT *context )
{
	FARPROC16 *p = CTX_SEG_OFF_TO_LIN(context, context->SegDs, context->Edx);
	TRACE("DOS ASPI opening\n");
	if ((CX_reg(context) == 4) || (CX_reg(context) == 5))
	{
		if( hWNASPI32 == INVALID_HANDLE_VALUE )
		{
			TRACE("Loading WNASPI32\n");
			hWNASPI32 = LoadLibraryExA("WNASPI32", 0, 0);
		}

		if( hWNASPI32 == INVALID_HANDLE_VALUE )
		{
			ERR("Error loading WNASPI32\n");
			goto error_exit;
		}

		/* Get SendASPI32Command by Ordinal 2 */
		/* Cast to correct argument/return types */
		pSendASPI32Command = (DWORD (*)(LPSRB))GetProcAddress(hWNASPI32, (LPCSTR)2);
		if( !pSendASPI32Command )
		{
			ERR("Error getting ordinal 2 from WNASPI32\n");
			goto error_exit;
		}

		*p = DPMI_AllocInternalRMCB(ASPI_DOS_func);
		TRACE("allocated real mode proc %p\n", *p);
		SET_AX( context, CX_reg(context) );

		return;
	}
error_exit:
	/* Return some error... General Failure sounds okay */
	SET_AX( context, ERROR_GEN_FAILURE );
	SET_CFLAG(context);
}
