/*
 * Debugger register handling
 *
 * Copyright 1995 Alexandre Julliard
 */

#include "config.h"
#include <stdio.h>
#include <string.h>
#include "selectors.h"
#include "debugger.h"

CONTEXT DEBUG_context;

/***********************************************************************
 *           DEBUG_SetRegister
 *
 * Set a register value.
 */
void DEBUG_SetRegister( enum debug_regs reg, int val )
{
    switch(reg)
    {
        case REG_EAX: EAX_reg(&DEBUG_context) = val; break;
        case REG_EBX: EBX_reg(&DEBUG_context) = val; break;
        case REG_ECX: ECX_reg(&DEBUG_context) = val; break;
        case REG_EDX: EDX_reg(&DEBUG_context) = val; break;
        case REG_ESI: ESI_reg(&DEBUG_context) = val; break;
        case REG_EDI: EDI_reg(&DEBUG_context) = val; break;
        case REG_EBP: EBP_reg(&DEBUG_context) = val; break;
        case REG_EFL: EFL_reg(&DEBUG_context) = val; break;
        case REG_EIP: EIP_reg(&DEBUG_context) = val; break;
        case REG_ESP: ESP_reg(&DEBUG_context) = val; break;
        case REG_AX:  AX_reg(&DEBUG_context)  = val; break;
        case REG_BX:  BX_reg(&DEBUG_context)  = val; break;
        case REG_CX:  CX_reg(&DEBUG_context)  = val; break;
        case REG_DX:  DX_reg(&DEBUG_context)  = val; break;
        case REG_SI:  SI_reg(&DEBUG_context)  = val; break;
        case REG_DI:  DI_reg(&DEBUG_context)  = val; break;
        case REG_BP:  BP_reg(&DEBUG_context)  = val; break;
        case REG_FL:  FL_reg(&DEBUG_context)  = val; break;
        case REG_IP:  IP_reg(&DEBUG_context)  = val; break;
        case REG_SP:  SP_reg(&DEBUG_context)  = val; break;
        case REG_CS:  CS_reg(&DEBUG_context)  = val; break;
        case REG_DS:  DS_reg(&DEBUG_context)  = val; break;
        case REG_ES:  ES_reg(&DEBUG_context)  = val; break;
        case REG_SS:  SS_reg(&DEBUG_context)  = val; break;
        case REG_FS:  FS_reg(&DEBUG_context)  = val; break;
        case REG_GS:  GS_reg(&DEBUG_context)  = val; break;
    }
}


int
DEBUG_PrintRegister(enum debug_regs reg)
{
    switch(reg)
    {
        case REG_EAX: fprintf(stderr, "%%eax"); break;
        case REG_EBX: fprintf(stderr, "%%ebx"); break;
        case REG_ECX: fprintf(stderr, "%%ecx"); break;
        case REG_EDX: fprintf(stderr, "%%edx"); break;
        case REG_ESI: fprintf(stderr, "%%esi"); break;
        case REG_EDI: fprintf(stderr, "%%edi"); break;
        case REG_EBP: fprintf(stderr, "%%ebp"); break;
        case REG_EFL: fprintf(stderr, "%%efl"); break;
        case REG_EIP: fprintf(stderr, "%%eip"); break;
        case REG_ESP: fprintf(stderr, "%%esp"); break;
        case REG_AX:  fprintf(stderr, "%%ax"); break;
        case REG_BX:  fprintf(stderr, "%%bx"); break;
        case REG_CX:  fprintf(stderr, "%%cx"); break;
        case REG_DX:  fprintf(stderr, "%%dx"); break;
        case REG_SI:  fprintf(stderr, "%%si"); break;
        case REG_DI:  fprintf(stderr, "%%di"); break;
        case REG_BP:  fprintf(stderr, "%%bp"); break;
        case REG_FL:  fprintf(stderr, "%%fl"); break;
        case REG_IP:  fprintf(stderr, "%%ip"); break;
        case REG_SP:  fprintf(stderr, "%%sp"); break;
        case REG_CS:  fprintf(stderr, "%%cs"); break;
        case REG_DS:  fprintf(stderr, "%%ds"); break;
        case REG_ES:  fprintf(stderr, "%%es"); break;
        case REG_SS:  fprintf(stderr, "%%ss"); break;
        case REG_FS:  fprintf(stderr, "%%fs"); break;
        case REG_GS:  fprintf(stderr, "%%gs"); break;
    }
    return TRUE;
}

/***********************************************************************
 *           DEBUG_GetRegister
 *
 * Get a register value.
 */
int DEBUG_GetRegister( enum debug_regs reg )
{
    switch(reg)
    {
        case REG_EAX: return EAX_reg(&DEBUG_context);
        case REG_EBX: return EBX_reg(&DEBUG_context);
        case REG_ECX: return ECX_reg(&DEBUG_context);
        case REG_EDX: return EDX_reg(&DEBUG_context);
        case REG_ESI: return ESI_reg(&DEBUG_context);
        case REG_EDI: return EDI_reg(&DEBUG_context);
        case REG_EBP: return EBP_reg(&DEBUG_context);
        case REG_EFL: return EFL_reg(&DEBUG_context);
        case REG_EIP: return EIP_reg(&DEBUG_context);
        case REG_ESP: return ESP_reg(&DEBUG_context);
        case REG_AX:  return AX_reg(&DEBUG_context);
        case REG_BX:  return BX_reg(&DEBUG_context);
        case REG_CX:  return CX_reg(&DEBUG_context);
        case REG_DX:  return DX_reg(&DEBUG_context);
        case REG_SI:  return SI_reg(&DEBUG_context);
        case REG_DI:  return DI_reg(&DEBUG_context);
        case REG_BP:  return BP_reg(&DEBUG_context);
        case REG_FL:  return FL_reg(&DEBUG_context);
        case REG_IP:  return IP_reg(&DEBUG_context);
        case REG_SP:  return SP_reg(&DEBUG_context);
        case REG_CS:  return CS_reg(&DEBUG_context);
        case REG_DS:  return DS_reg(&DEBUG_context);
        case REG_ES:  return ES_reg(&DEBUG_context);
        case REG_SS:  return SS_reg(&DEBUG_context);
        case REG_FS:  return FS_reg(&DEBUG_context);
        case REG_GS:  return GS_reg(&DEBUG_context);
    }
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
    char flag[33];

    fprintf(stderr,"Register dump:\n");

    /* First get the segment registers out of the way */
    fprintf( stderr," CS:%04x SS:%04x DS:%04x ES:%04x FS:%04x GS:%04x",
             (WORD)CS_reg(&DEBUG_context), (WORD)SS_reg(&DEBUG_context),
             (WORD)DS_reg(&DEBUG_context), (WORD)ES_reg(&DEBUG_context),
             (WORD)FS_reg(&DEBUG_context), (WORD)GS_reg(&DEBUG_context) );
    if (dbg_mode == 16)
    {
        fprintf( stderr,"\n IP:%04x SP:%04x BP:%04x FLAGS:%04x(%s)\n",
                 IP_reg(&DEBUG_context), SP_reg(&DEBUG_context),
                 BP_reg(&DEBUG_context), FL_reg(&DEBUG_context),
		 DEBUG_Flags(FL_reg(&DEBUG_context), flag));
	fprintf( stderr," AX:%04x BX:%04x CX:%04x DX:%04x SI:%04x DI:%04x\n",
                 AX_reg(&DEBUG_context), BX_reg(&DEBUG_context),
                 CX_reg(&DEBUG_context), DX_reg(&DEBUG_context),
                 SI_reg(&DEBUG_context), DI_reg(&DEBUG_context) );
    }
    else  /* 32-bit mode */
    {
        fprintf( stderr, "\n EIP:%08lx ESP:%08lx EBP:%08lx EFLAGS:%08lx(%s)\n", 
                 EIP_reg(&DEBUG_context), ESP_reg(&DEBUG_context),
                 EBP_reg(&DEBUG_context), EFL_reg(&DEBUG_context),
		 DEBUG_Flags(EFL_reg(&DEBUG_context), flag));
	fprintf( stderr, " EAX:%08lx EBX:%08lx ECX:%08lx EDX:%08lx\n", 
		 EAX_reg(&DEBUG_context), EBX_reg(&DEBUG_context),
                 ECX_reg(&DEBUG_context), EDX_reg(&DEBUG_context) );
	fprintf( stderr, " ESI:%08lx EDI:%08lx\n",
                 ESI_reg(&DEBUG_context), EDI_reg(&DEBUG_context) );
    }
}


/***********************************************************************
 *           DEBUG_ValidateRegisters
 *
 * Make sure all registers have a correct value for returning from
 * the signal handler.
 */
BOOL DEBUG_ValidateRegisters(void)
{
    WORD cs, ds;

    if (ISV86(&DEBUG_context)) return TRUE;

/* Check that a selector is a valid ring-3 LDT selector, or a NULL selector */
#define CHECK_SEG(seg,name) \
    if (((seg) & ~3) && \
        ((((seg) & 7) != 7) || IS_LDT_ENTRY_FREE(SELECTOR_TO_ENTRY(seg)))) \
    { \
        fprintf( stderr, "*** Invalid value for %s register: %04x\n", \
                 (name), (WORD)(seg) ); \
        return FALSE; \
    }

    GET_CS(cs);
    GET_DS(ds);
    if (CS_reg(&DEBUG_context) != cs) CHECK_SEG(CS_reg(&DEBUG_context), "CS");
    if (SS_reg(&DEBUG_context) != ds) CHECK_SEG(SS_reg(&DEBUG_context), "SS");
    if (DS_reg(&DEBUG_context) != ds) CHECK_SEG(DS_reg(&DEBUG_context), "DS");
    if (ES_reg(&DEBUG_context) != ds) CHECK_SEG(ES_reg(&DEBUG_context), "ES");
    if (FS_reg(&DEBUG_context) != ds) CHECK_SEG(FS_reg(&DEBUG_context), "FS");
    if (GS_reg(&DEBUG_context) != ds) CHECK_SEG(GS_reg(&DEBUG_context), "GS");

    /* Check that CS and SS are not NULL */

    if (!ISV86(&DEBUG_context))
    if (!(CS_reg(&DEBUG_context) & ~3))
    {
        fprintf( stderr, "*** Invalid value for CS register: %04x\n",
                 (WORD)CS_reg(&DEBUG_context) );
        return FALSE;
    }
    if (!(SS_reg(&DEBUG_context) & ~3))
    {
        fprintf( stderr, "*** Invalid value for SS register: %04x\n",
                 (WORD)SS_reg(&DEBUG_context) );
        return FALSE;
    }
    return TRUE;
#undef CHECK_SEG
}
