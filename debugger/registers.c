/*
 * Debugger register handling
 *
 * Copyright 1995 Alexandre Julliard
 */

#include "config.h"
#include <string.h>
#include "debugger.h"

/***********************************************************************
 *           DEBUG_Flags
 *
 * Return Flag String.
 */
char *DEBUG_Flags( DWORD flag, char *buf )
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
    if ( flag & 0x00001000 ) *pt = '1'; /* I/O Privilage Level */
    if ( buf >= pt-- ) return( buf );
    if ( flag & 0x00002000 ) *pt = '1'; /* I/O Privilage Level */
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
void DEBUG_InfoRegisters(void)
{
    DEBUG_Printf(DBG_CHN_MESG,"Register dump:\n");

#ifdef __i386__
    /* First get the segment registers out of the way */
    DEBUG_Printf( DBG_CHN_MESG," CS:%04x SS:%04x DS:%04x ES:%04x FS:%04x GS:%04x",
		  (WORD)DEBUG_context.SegCs, (WORD)DEBUG_context.SegSs,
		  (WORD)DEBUG_context.SegDs, (WORD)DEBUG_context.SegEs,
		  (WORD)DEBUG_context.SegFs, (WORD)DEBUG_context.SegGs );
    if (DEBUG_CurrThread->dbg_mode == 16)
    {
        char flag[33];

        DEBUG_Printf( DBG_CHN_MESG,"\n IP:%04x SP:%04x BP:%04x FLAGS:%04x(%s)\n",
		      LOWORD(DEBUG_context.Eip), LOWORD(DEBUG_context.Esp),
		      LOWORD(DEBUG_context.Ebp), LOWORD(DEBUG_context.EFlags),
		      DEBUG_Flags(LOWORD(DEBUG_context.EFlags), flag));
	DEBUG_Printf( DBG_CHN_MESG," AX:%04x BX:%04x CX:%04x DX:%04x SI:%04x DI:%04x\n",
		      LOWORD(DEBUG_context.Eax), LOWORD(DEBUG_context.Ebx),
		      LOWORD(DEBUG_context.Ecx), LOWORD(DEBUG_context.Edx),
		      LOWORD(DEBUG_context.Esi), LOWORD(DEBUG_context.Edi) );
    }
    else  /* 32-bit mode */
    {
        char flag[33];

        DEBUG_Printf( DBG_CHN_MESG, "\n EIP:%08lx ESP:%08lx EBP:%08lx EFLAGS:%08lx(%s)\n", 
		      DEBUG_context.Eip, DEBUG_context.Esp,
		      DEBUG_context.Ebp, DEBUG_context.EFlags,
		      DEBUG_Flags(DEBUG_context.EFlags, flag));
	DEBUG_Printf( DBG_CHN_MESG, " EAX:%08lx EBX:%08lx ECX:%08lx EDX:%08lx\n", 
		      DEBUG_context.Eax, DEBUG_context.Ebx,
		      DEBUG_context.Ecx, DEBUG_context.Edx );
	DEBUG_Printf( DBG_CHN_MESG, " ESI:%08lx EDI:%08lx\n",
		      DEBUG_context.Esi, DEBUG_context.Edi );
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
    if (CS_reg(DEBUG_context) != cs) CHECK_SEG(CS_reg(DEBUG_context), "CS");
    if (SS_reg(DEBUG_context) != ds) CHECK_SEG(SS_reg(DEBUG_context), "SS");
    if (DS_reg(DEBUG_context) != ds) CHECK_SEG(DS_reg(DEBUG_context), "DS");
    if (ES_reg(DEBUG_context) != ds) CHECK_SEG(ES_reg(DEBUG_context), "ES");
    if (FS_reg(DEBUG_context) != ds) CHECK_SEG(FS_reg(DEBUG_context), "FS");
    if (GS_reg(DEBUG_context) != ds) CHECK_SEG(GS_reg(DEBUG_context), "GS");
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
