/*
 * XMS v2+ emulation
 *
 * Copyright 1998 Ove Kåven
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Note: This XMS emulation is hooked through the DPMI interrupt.
 */

#include "config.h"

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <string.h>
#include "winbase.h"
#include "wine/winbase16.h"
#include "module.h"
#include "miscemu.h"
#include "toolhelp.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(int31);

typedef struct {
 WORD Handle;
 DWORD Offset;
} WINE_PACKED MOVEOFS;

typedef struct {
 DWORD Length;
 MOVEOFS Source;
 MOVEOFS Dest;
} WINE_PACKED MOVESTRUCT;

static BYTE * XMS_Offset( MOVEOFS *ofs )
{
    if (ofs->Handle) return (BYTE*)GlobalLock16(ofs->Handle)+ofs->Offset;
        else return (BYTE*)DOSMEM_MapRealToLinear(ofs->Offset);
}

/**********************************************************************
 *	    XMS_Handler
 */

void WINAPI XMS_Handler( CONTEXT86 *context )
{
    switch(AH_reg(context))
    {
    case 0x00:   /* Get XMS version number */
        TRACE("get XMS version number\n");
        AX_reg(context) = 0x0200; /* 2.0 */
        BX_reg(context) = 0x0000; /* internal revision */
        DX_reg(context) = 0x0001; /* HMA exists */
        break;
    case 0x08:   /* Query Free Extended Memory */
    {
	MEMMANINFO mmi;

        TRACE("query free extended memory\n");
	mmi.dwSize = sizeof(mmi);
	MemManInfo16(&mmi);
        AX_reg(context) = mmi.dwLargestFreeBlock >> 10;
        DX_reg(context) = (mmi.dwFreePages * mmi.wPageSize) >> 10;
        TRACE("returning largest %dK, total %dK\n", AX_reg(context), DX_reg(context));
    }
    break;
    case 0x09:   /* Allocate Extended Memory Block */
        TRACE("allocate extended memory block (%dK)\n",
            DX_reg(context));
	DX_reg(context) = GlobalAlloc16(GMEM_MOVEABLE,
	    (DWORD)DX_reg(context)<<10);
	AX_reg(context) = DX_reg(context) ? 1 : 0;
	if (!DX_reg(context)) BL_reg(context) = 0xA0; /* out of memory */
	break;
    case 0x0a:   /* Free Extended Memory Block */
	TRACE("free extended memory block %04x\n",DX_reg(context));
       if(!DX_reg(context) || GlobalFree16(DX_reg(context))) {
         AX_reg(context) = 0;    /* failure */
         BL_reg(context) = 0xa2; /* invalid handle */
       } else
         AX_reg(context) = 1;    /* success */
	break;
    case 0x0b:   /* Move Extended Memory Block */
    {
	MOVESTRUCT*move=CTX_SEG_OFF_TO_LIN(context,
	    context->SegDs,context->Esi);
        BYTE*src,*dst;
        TRACE("move extended memory block\n");
        src=XMS_Offset(&move->Source);
        dst=XMS_Offset(&move->Dest);
	memcpy(dst,src,move->Length);
	if (move->Source.Handle) GlobalUnlock16(move->Source.Handle);
	if (move->Dest.Handle) GlobalUnlock16(move->Dest.Handle);
	break;
    }
    default:
        INT_BARF( context, 0x31 );
        AX_reg(context) = 0x0000; /* failure */
        BL_reg(context) = 0x80;   /* function not implemented */
        break;
    }
}
