/*
 * Debugger register handling
 *
 * Copyright 1995 Alexandre Julliard
 */

#include "config.h"
#include <stdio.h>
#include <string.h>
#include "debugger.h"

/***********************************************************************
 *           DEBUG_SetRegister
 *
 * Set a register value.
 */
void DEBUG_SetRegister( enum debug_regs reg, int val )
{
#ifdef __i386__
    switch(reg)
    {
        case REG_EAX: DEBUG_context.Eax = val; break;
        case REG_EBX: DEBUG_context.Ebx = val; break;
        case REG_ECX: DEBUG_context.Ecx = val; break;
        case REG_EDX: DEBUG_context.Edx = val; break;
        case REG_ESI: DEBUG_context.Esi = val; break;
        case REG_EDI: DEBUG_context.Edi = val; break;
        case REG_EBP: DEBUG_context.Ebp = val; break;
        case REG_EFL: DEBUG_context.EFlags = val; break;
        case REG_EIP: DEBUG_context.Eip = val; break;
        case REG_ESP: DEBUG_context.Esp = val; break;
        case REG_CS:  DEBUG_context.SegCs = val; break;
        case REG_DS:  DEBUG_context.SegDs = val; break;
        case REG_ES:  DEBUG_context.SegEs = val; break;
        case REG_SS:  DEBUG_context.SegSs = val; break;
        case REG_FS:  DEBUG_context.SegFs = val; break;
        case REG_GS:  DEBUG_context.SegGs = val; break;
#define SET_LOW_WORD(dw,lw)	((dw) = ((dw) & 0xFFFF0000) | LOWORD(lw))
        case REG_AX:  SET_LOW_WORD(DEBUG_context.Eax,val); break;
        case REG_BX:  SET_LOW_WORD(DEBUG_context.Ebx,val); break;
        case REG_CX:  SET_LOW_WORD(DEBUG_context.Ecx,val); break;
        case REG_DX:  SET_LOW_WORD(DEBUG_context.Edx,val); break;
        case REG_SI:  SET_LOW_WORD(DEBUG_context.Esi,val); break;
        case REG_DI:  SET_LOW_WORD(DEBUG_context.Edi,val); break;
        case REG_BP:  SET_LOW_WORD(DEBUG_context.Ebp,val); break;
        case REG_FL:  SET_LOW_WORD(DEBUG_context.EFlags,val); break;
        case REG_IP:  SET_LOW_WORD(DEBUG_context.Eip,val); break;
        case REG_SP:  SET_LOW_WORD(DEBUG_context.Esp,val); break;
#undef SET_LOWORD
    }
#endif
}


int DEBUG_PrintRegister(enum debug_regs reg)
{
#ifdef __i386__
    char* 	val = NULL;
    switch(reg)
    {
        case REG_EAX: val = "%%eax"; break;
        case REG_EBX: val = "%%ebx"; break;
        case REG_ECX: val = "%%ecx"; break;
        case REG_EDX: val = "%%edx"; break;
        case REG_ESI: val = "%%esi"; break;
        case REG_EDI: val = "%%edi"; break;
        case REG_EBP: val = "%%ebp"; break;
        case REG_EFL: val = "%%efl"; break;
        case REG_EIP: val = "%%eip"; break;
        case REG_ESP: val = "%%esp"; break;
        case REG_AX:  val = "%%ax";  break;
        case REG_BX:  val = "%%bx";  break;
        case REG_CX:  val = "%%cx";  break;
        case REG_DX:  val = "%%dx";  break;
        case REG_SI:  val = "%%si";  break;
        case REG_DI:  val = "%%di";  break;
        case REG_BP:  val = "%%bp";  break;
        case REG_FL:  val = "%%fl";  break;
        case REG_IP:  val = "%%ip";  break;
        case REG_SP:  val = "%%sp";  break;
        case REG_CS:  val = "%%cs";  break;
        case REG_DS:  val = "%%ds";  break;
        case REG_ES:  val = "%%es";  break;
        case REG_SS:  val = "%%ss";  break;
        case REG_FS:  val = "%%fs";  break;
        case REG_GS:  val = "%%gs";  break;
    }
    if (val) fprintf(stderr, val);
    return TRUE;
#else
    return FALSE;
#endif
}

/***********************************************************************
 *           DEBUG_GetRegister
 *
 * Get a register value.
 */
int DEBUG_GetRegister( enum debug_regs reg )
{
#ifdef __i386__
    switch(reg)
    {
        case REG_EAX: return DEBUG_context.Eax;
        case REG_EBX: return DEBUG_context.Ebx;
        case REG_ECX: return DEBUG_context.Ecx;
        case REG_EDX: return DEBUG_context.Edx;
        case REG_ESI: return DEBUG_context.Esi;
        case REG_EDI: return DEBUG_context.Edi;
        case REG_EBP: return DEBUG_context.Ebp;
        case REG_EFL: return DEBUG_context.EFlags;
        case REG_EIP: return DEBUG_context.Eip;
        case REG_ESP: return DEBUG_context.Esp;
        case REG_CS:  return DEBUG_context.SegCs;
        case REG_DS:  return DEBUG_context.SegDs;
        case REG_ES:  return DEBUG_context.SegEs;
        case REG_SS:  return DEBUG_context.SegSs;
        case REG_FS:  return DEBUG_context.SegFs;
        case REG_GS:  return DEBUG_context.SegGs;
        case REG_AX:  return LOWORD(DEBUG_context.Eax);
        case REG_BX:  return LOWORD(DEBUG_context.Ebx);
        case REG_CX:  return LOWORD(DEBUG_context.Ecx);
        case REG_DX:  return LOWORD(DEBUG_context.Edx);
        case REG_SI:  return LOWORD(DEBUG_context.Esi);
        case REG_DI:  return LOWORD(DEBUG_context.Edi);
        case REG_BP:  return LOWORD(DEBUG_context.Ebp);
        case REG_FL:  return LOWORD(DEBUG_context.EFlags);
        case REG_IP:  return LOWORD(DEBUG_context.Eip);
        case REG_SP:  return LOWORD(DEBUG_context.Esp);
    }
#endif
    return 0;  /* should not happen */
}


/***********************************************************************
 *           DEBUG_Flags
 *
 * Return Flag String.
 */
char *DEBUG_Flags( DWORD flag, char *buf )
{
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
}


/***********************************************************************
 *           DEBUG_InfoRegisters
 *
 * Display registers information. 
 */
void DEBUG_InfoRegisters(void)
{
    fprintf(stderr,"Register dump:\n");

#ifdef __i386__
    /* First get the segment registers out of the way */
    fprintf( stderr," CS:%04x SS:%04x DS:%04x ES:%04x FS:%04x GS:%04x",
             (WORD)DEBUG_context.SegCs, (WORD)DEBUG_context.SegSs,
             (WORD)DEBUG_context.SegDs, (WORD)DEBUG_context.SegEs,
             (WORD)DEBUG_context.SegFs, (WORD)DEBUG_context.SegGs );
    if (DEBUG_CurrThread->dbg_mode == 16)
    {
        char flag[33];

        fprintf( stderr,"\n IP:%04x SP:%04x BP:%04x FLAGS:%04x(%s)\n",
                 LOWORD(DEBUG_context.Eip), LOWORD(DEBUG_context.Esp),
                 LOWORD(DEBUG_context.Ebp), LOWORD(DEBUG_context.EFlags),
		 DEBUG_Flags(LOWORD(DEBUG_context.EFlags), flag));
	fprintf( stderr," AX:%04x BX:%04x CX:%04x DX:%04x SI:%04x DI:%04x\n",
                 LOWORD(DEBUG_context.Eax), LOWORD(DEBUG_context.Ebx),
                 LOWORD(DEBUG_context.Ecx), LOWORD(DEBUG_context.Edx),
                 LOWORD(DEBUG_context.Esi), LOWORD(DEBUG_context.Edi) );
    }
    else  /* 32-bit mode */
    {
        char flag[33];

        fprintf( stderr, "\n EIP:%08lx ESP:%08lx EBP:%08lx EFLAGS:%08lx(%s)\n", 
                 DEBUG_context.Eip, DEBUG_context.Esp,
                 DEBUG_context.Ebp, DEBUG_context.EFlags,
		 DEBUG_Flags(DEBUG_context.EFlags, flag));
	fprintf( stderr, " EAX:%08lx EBX:%08lx ECX:%08lx EDX:%08lx\n", 
		 DEBUG_context.Eax, DEBUG_context.Ebx,
                 DEBUG_context.Ecx, DEBUG_context.Edx );
	fprintf( stderr, " ESI:%08lx EDI:%08lx\n",
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
#ifdef __i386__
    if (DEBUG_context.EFlags & V86_FLAG) return TRUE;

#if 0
/* Check that a selector is a valid ring-3 LDT selector, or a NULL selector */
#define CHECK_SEG(seg,name) \
    if (((seg) & ~3) && ((((seg) & 7) != 7) || !DEBUG_IsSelector(seg))) { \
        fprintf( stderr, "*** Invalid value for %s register: %04x\n", \
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
        fprintf( stderr, "*** Invalid value for CS register: %04x\n",
                 (WORD)DEBUG_context.SegCs );
        return FALSE;
    }
    if (!(DEBUG_context.SegSs & ~3))
    {
        fprintf( stderr, "*** Invalid value for SS register: %04x\n",
                 (WORD)DEBUG_context.SegSs );
        return FALSE;
    }
    return TRUE;
#undef CHECK_SEG
#else
    return TRUE;
#endif
}
