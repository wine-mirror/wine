/*
 * Debugger register handling
 *
 * Copyright 1995 Alexandre Julliard
 */

#include <stdio.h>
#include "debugger.h"


struct sigcontext_struct *context;



/***********************************************************************
 *           DEBUG_SetRegister
 *
 * Set a register value.
 */
void DEBUG_SetRegister( enum debug_regs reg, int val )
{
    switch(reg)
    {
        case REG_EAX: EAX = val; break;
        case REG_EBX: EBX = val; break;
        case REG_ECX: ECX = val; break;
        case REG_EDX: EDX = val; break;
        case REG_ESI: ESI = val; break;
        case REG_EDI: EDI = val; break;
        case REG_EBP: EBP = val; break;
        case REG_EFL: EFL = val; break;
        case REG_EIP: EIP = val; break;
        case REG_ESP: ESP = val; break;
        case REG_AX:  AX  = val; break;
        case REG_BX:  BX  = val; break;
        case REG_CX:  CX  = val; break;
        case REG_DX:  DX  = val; break;
        case REG_SI:  SI  = val; break;
        case REG_DI:  DI  = val; break;
        case REG_BP:  BP  = val; break;
        case REG_FL:  FL  = val; break;
        case REG_IP:  IP  = val; break;
        case REG_SP:  SP  = val; break;
        case REG_CS:  CS  = val; break;
        case REG_DS:  DS  = val; break;
        case REG_ES:  ES  = val; break;
        case REG_SS:  SS  = val; break;
    }
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
        case REG_EAX: return EAX;
        case REG_EBX: return EBX;
        case REG_ECX: return ECX;
        case REG_EDX: return EDX;
        case REG_ESI: return ESI;
        case REG_EDI: return EDI;
        case REG_EBP: return EBP;
        case REG_EFL: return EFL;
        case REG_EIP: return EIP;
        case REG_ESP: return ESP;
        case REG_AX:  return AX;
        case REG_BX:  return BX;
        case REG_CX:  return CX;
        case REG_DX:  return DX;
        case REG_SI:  return SI;
        case REG_DI:  return DI;
        case REG_BP:  return BP;
        case REG_FL:  return FL;
        case REG_IP:  return IP;
        case REG_SP:  return SP;
        case REG_CS:  return CS;
        case REG_DS:  return DS;
        case REG_ES:  return ES;
        case REG_SS:  return SS;
    }
    return 0;  /* should not happen */
}



/***********************************************************************
 *           DEBUG_InfoRegisters
 *
 * Display registers information. 
 */
void DEBUG_InfoRegisters(void)
{
    fprintf(stderr,"Register dump:\n");

    /* First get the segment registers out of the way */
    fprintf( stderr," CS:%04x SS:%04x DS:%04x ES:%04x\n", CS, SS, DS, ES );

    if (dbg_mode == 16)
    {
        fprintf( stderr," IP:%04x SP:%04x BP:%04x FLAGS:%04x\n",
                 IP, SP, BP, FL );
	fprintf( stderr," AX:%04x BX:%04x CX:%04x DX:%04x SI:%04x DI:%04x\n",
                 AX, BX, CX, DX, SI, DI );
    }
    else  /* 32-bit mode */
    {
        fprintf( stderr, " EIP:%08lx ESP:%08lx EBP:%08lx EFLAGS:%08lx\n", 
                 EIP, ESP, EBP, EFL );
	fprintf( stderr, " EAX:%08lx EBX:%08lx ECX:%08lx EDX:%08lx\n", 
		 EAX, EBX, ECX, EDX);
	fprintf( stderr, " ESI:%08lx EDI:%08lx\n", ESI, EDI);
    }
}

