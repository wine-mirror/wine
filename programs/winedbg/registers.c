/*
 * Debugger register handling
 *
 * Copyright 1995 Alexandre Julliard
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
 */

#include "config.h"
#include <string.h>
#include "debugger.h"

/***********************************************************************
 *           DEBUG_Flags
 *
 * Return Flag String.
 */
static char *DEBUG_Flags( DWORD flag, char *buf )
{
#ifdef __i386__
    char *pt;

    strcpy( buf, "   - 00      - - - " );
    pt = buf + strlen( buf );
    if ( buf >= pt-- ) return( buf );
    if ( flag & 0x00000001 ) *pt = 'C'; /* Carry Falg */
    if ( buf >= pt-- ) return( buf );
    if ( flag & 0x00000002 ) *pt = '1';
    if ( buf >= pt-- ) return( buf );
    if ( flag & 0x00000004 ) *pt = 'P'; /* Parity Flag */
    if ( buf >= pt-- ) return( buf );
    if ( flag & 0x00000008 ) *pt = '-';
    if ( buf >= pt-- ) return( buf );
    if ( flag & 0x00000010 ) *pt = 'A'; /* Auxiliary Carry Flag */
    if ( buf >= pt-- ) return( buf );
    if ( flag & 0x00000020 ) *pt = '-';
    if ( buf >= pt-- ) return( buf );
    if ( flag & 0x00000040 ) *pt = 'Z'; /* Zero Flag */
    if ( buf >= pt-- ) return( buf );
    if ( flag & 0x00000080 ) *pt = 'S'; /* Sign Flag */
    if ( buf >= pt-- ) return( buf );
    if ( flag & 0x00000100 ) *pt = 'T'; /* Trap/Trace Flag */
    if ( buf >= pt-- ) return( buf );
    if ( flag & 0x00000200 ) *pt = 'I'; /* Interupt Enable Flag */
    if ( buf >= pt-- ) return( buf );
    if ( flag & 0x00000400 ) *pt = 'D'; /* Direction Indicator */
    if ( buf >= pt-- ) return( buf );
    if ( flag & 0x00000800 ) *pt = 'O'; /* Overflow Flag */
    if ( buf >= pt-- ) return( buf );
    if ( flag & 0x00001000 ) *pt = '1'; /* I/O Privilege Level */
    if ( buf >= pt-- ) return( buf );
    if ( flag & 0x00002000 ) *pt = '1'; /* I/O Privilege Level */
    if ( buf >= pt-- ) return( buf );
    if ( flag & 0x00004000 ) *pt = 'N'; /* Nested Task Flag */
    if ( buf >= pt-- ) return( buf );
    if ( flag & 0x00008000 ) *pt = '-';
    if ( buf >= pt-- ) return( buf );
    if ( flag & 0x00010000 ) *pt = 'R'; /* Resume Flag */
    if ( buf >= pt-- ) return( buf );
    if ( flag & 0x00020000 ) *pt = 'V'; /* Vritual Mode Flag */
    if ( buf >= pt-- ) return( buf );
    if ( flag & 0x00040000 ) *pt = 'a'; /* Alignment Check Flag */
    if ( buf >= pt-- ) return( buf );
    return( buf );
#else
    strcpy(buf, "unknown CPU");
    return buf;
#endif
}


/***********************************************************************
 *           DEBUG_InfoRegisters
 *
 * Display registers information.
 */
void DEBUG_InfoRegisters(const CONTEXT* ctx)
{
    DEBUG_Printf(DBG_CHN_MESG,"Register dump:\n");

#ifdef __i386__
    /* First get the segment registers out of the way */
    DEBUG_Printf( DBG_CHN_MESG," CS:%04x SS:%04x DS:%04x ES:%04x FS:%04x GS:%04x",
		  (WORD)ctx->SegCs, (WORD)ctx->SegSs,
		  (WORD)ctx->SegDs, (WORD)ctx->SegEs,
		  (WORD)ctx->SegFs, (WORD)ctx->SegGs );
    if (DEBUG_CurrThread->dbg_mode != MODE_32)
    {
        char flag[33];

        DEBUG_Printf( DBG_CHN_MESG,"\n IP:%04x SP:%04x BP:%04x FLAGS:%04x(%s)\n",
		      LOWORD(ctx->Eip), LOWORD(ctx->Esp),
		      LOWORD(ctx->Ebp), LOWORD(ctx->EFlags),
		      DEBUG_Flags(LOWORD(ctx->EFlags), flag));
	DEBUG_Printf( DBG_CHN_MESG," AX:%04x BX:%04x CX:%04x DX:%04x SI:%04x DI:%04x\n",
		      LOWORD(ctx->Eax), LOWORD(ctx->Ebx),
		      LOWORD(ctx->Ecx), LOWORD(ctx->Edx),
		      LOWORD(ctx->Esi), LOWORD(ctx->Edi) );
    }
    else  /* 32-bit mode */
    {
        char flag[33];

        DEBUG_Printf( DBG_CHN_MESG, "\n EIP:%08lx ESP:%08lx EBP:%08lx EFLAGS:%08lx(%s)\n",
		      ctx->Eip, ctx->Esp,
		      ctx->Ebp, ctx->EFlags,
		      DEBUG_Flags(ctx->EFlags, flag));
	DEBUG_Printf( DBG_CHN_MESG, " EAX:%08lx EBX:%08lx ECX:%08lx EDX:%08lx\n",
		      ctx->Eax, ctx->Ebx,
		      ctx->Ecx, ctx->Edx );
	DEBUG_Printf( DBG_CHN_MESG, " ESI:%08lx EDI:%08lx\n",
		      ctx->Esi, ctx->Edi );
    }
#endif
}


/***********************************************************************
 *           DEBUG_ValidateRegisters
 *
 * Make sure all registers have a correct value for returning from
 * the signal handler.
 */
BOOL DEBUG_ValidateRegisters(void)
{
   DBG_ADDR	addr;
   char		ch;

#ifdef __i386__
    if (DEBUG_context.EFlags & V86_FLAG) return TRUE;

#if 0
/* Check that a selector is a valid ring-3 LDT selector, or a NULL selector */
#define CHECK_SEG(seg,name) \
    if (((seg) & ~3) && ((((seg) & 7) != 7) || !DEBUG_IsSelector(seg))) { \
        DEBUG_Printf( DBG_CHN_MESG, "*** Invalid value for %s register: %04x\n", \
                      (name), (WORD)(seg) ); \
        return FALSE; \
    }

    cs = __get_cs();
    ds = __get_ds();
    if (DEBUG_context.SegCs != cs) CHECK_SEG(DEBUG_context.SegCs, "CS");
    if (DEBUG_context.SegSs != ds) CHECK_SEG(DEBUG_context.SegSs, "SS");
    if (DEBUG_context.SegDs != ds) CHECK_SEG(DEBUG_context.SegDs, "DS");
    if (DEBUG_context.SegEs != ds) CHECK_SEG(DEBUG_context.SegEs, "ES");
    if (DEBUG_context.SegFs != ds) CHECK_SEG(DEBUG_context.SegFs, "FS");
    if (DEBUG_context.SegGs != ds) CHECK_SEG(DEBUG_context.SegGs, "GS");
#endif

    /* Check that CS and SS are not NULL */

    if (!(DEBUG_context.SegCs & ~3))
    {
        DEBUG_Printf( DBG_CHN_MESG, "*** Invalid value for CS register: %04x\n",
		      (WORD)DEBUG_context.SegCs );
        return FALSE;
    }
    if (!(DEBUG_context.SegSs & ~3))
    {
        DEBUG_Printf( DBG_CHN_MESG, "*** Invalid value for SS register: %04x\n",
		      (WORD)DEBUG_context.SegSs );
        return FALSE;
    }

#undef CHECK_SEG
#else
    /* to be written */
#endif

    /* check if PC is correct */
    DEBUG_GetCurrentAddress(&addr);
    return DEBUG_READ_MEM_VERBOSE((void*)DEBUG_ToLinear(&addr), &ch, 1);
}
