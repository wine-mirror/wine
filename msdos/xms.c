/*
 * XMS v2+ emulation
 *
 * Copyright 1998 Ove Kåven
 *
 * This XMS emulation is hooked through the DPMI interrupt.
 */

#include <unistd.h>
#include <string.h>
#include "winbase.h"
#include "global.h"
#include "module.h"
#include "miscemu.h"
#include "toolhelp.h"
#include "debug.h"
#include "selectors.h"

DEFAULT_DEBUG_CHANNEL(int31)

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
        TRACE(int31, "get XMS version number\n");
        AX_reg(context) = 0x0200; /* 2.0 */
        BX_reg(context) = 0x0000; /* internal revision */
        DX_reg(context) = 0x0001; /* HMA exists */
        break;
    case 0x08:   /* Query Free Extended Memory */
    {
	MEMMANINFO mmi;

        TRACE(int31, "query free extended memory\n");
	mmi.dwSize = sizeof(mmi);
	MemManInfo16(&mmi);
        AX_reg(context) = mmi.dwLargestFreeBlock >> 10;
	    DX_reg(context) = (mmi.dwFreePages * VIRTUAL_GetPageSize()) >> 10;
        TRACE(int31, "returning largest %dK, total %dK\n", AX_reg(context), DX_reg(context));
    }
    break;
    case 0x09:   /* Allocate Extended Memory Block */
        TRACE(int31, "allocate extended memory block (%dK)\n",
            DX_reg(context));
	DX_reg(context) = GlobalAlloc16(GMEM_MOVEABLE,
	    (DWORD)DX_reg(context)<<10);
	AX_reg(context) = DX_reg(context) ? 1 : 0;
	if (!DX_reg(context)) BL_reg(context) = 0xA0; /* out of memory */
	break;
    case 0x0a:   /* Free Extended Memory Block */
	TRACE(int31, "free extended memory block %04x\n",DX_reg(context));
	GlobalFree16(DX_reg(context));
	break;
    case 0x0b:   /* Move Extended Memory Block */
    {
	MOVESTRUCT*move=CTX_SEG_OFF_TO_LIN(context,
	    DS_reg(context),ESI_reg(context));
        BYTE*src,*dst;
        TRACE(int31, "move extended memory block\n");
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
