/*
 * Emulator signal handling
 *
 * Copyright 1995 Alexandre Julliard
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <setjmp.h>

#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/wait.h>

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__svr4__) || defined(_SCO_DS) || defined(__EMX__)
# if !defined(_SCO_DS) && !defined(__EMX__)
#  include <sys/syscall.h>
# endif
# include <sys/param.h>
#else
# include <syscall.h>
#endif

#include "debugger.h"
#include "options.h"
#include "sig_context.h"
#include "miscemu.h"
#include "thread.h"


extern void SIGNAL_SetHandler( int sig, void (*func)(), int flags );
extern BOOL32 INSTR_EmulateInstruction( SIGCONTEXT *context );


/**********************************************************************
 *              SIGNAL_break
 * 
 * Handle Ctrl-C and such
 */
static HANDLER_DEF(SIGNAL_break)
{
    HANDLER_INIT();
    if (Options.debug)
        wine_debug( signal, HANDLER_CONTEXT );  /* Enter our debugger */
    else exit(0);
}


/**********************************************************************
 *		SIGNAL_trap
 *
 * SIGTRAP handler.
 */
static HANDLER_DEF(SIGNAL_trap)
{
    HANDLER_INIT();
    wine_debug( signal, HANDLER_CONTEXT );  /* Enter our debugger */
}


/**********************************************************************
 *		SIGNAL_fault
 *
 * Segfault handler.
 */
static HANDLER_DEF(SIGNAL_fault)
{
    WORD cs;
    GET_CS(cs);
    HANDLER_INIT();
    if (CS_sig(HANDLER_CONTEXT) == cs)
    {
        fprintf( stderr, "Segmentation fault in 32-bit code (0x%08lx).\n",
                 EIP_sig(HANDLER_CONTEXT) );
    }
    else
    {
        if (INSTR_EmulateInstruction( HANDLER_CONTEXT )) return;
        fprintf( stderr, "Segmentation fault in 16-bit code (%04x:%04lx).\n",
                 (WORD)CS_sig(HANDLER_CONTEXT), EIP_sig(HANDLER_CONTEXT) );
    }
    wine_debug( signal, HANDLER_CONTEXT );
}


/***********************************************************************
 *           SIGNAL_SetContext
 *
 * Set the register values from a sigcontext. 
 */
static void SIGNAL_SetSigContext( const SIGCONTEXT *sigcontext,
                                  CONTEXT *context )
{
    EAX_reg(context) = EAX_sig(sigcontext);
    EBX_reg(context) = EBX_sig(sigcontext);
    ECX_reg(context) = ECX_sig(sigcontext);
    EDX_reg(context) = EDX_sig(sigcontext);
    ESI_reg(context) = ESI_sig(sigcontext);
    EDI_reg(context) = EDI_sig(sigcontext);
    EBP_reg(context) = EBP_sig(sigcontext);
    EFL_reg(context) = EFL_sig(sigcontext);
    EIP_reg(context) = EIP_sig(sigcontext);
    ESP_reg(context) = ESP_sig(sigcontext);
    CS_reg(context)  = LOWORD(CS_sig(sigcontext));
    DS_reg(context)  = LOWORD(DS_sig(sigcontext));
    ES_reg(context)  = LOWORD(ES_sig(sigcontext));
    SS_reg(context)  = LOWORD(SS_sig(sigcontext));
#ifdef FS_sig
    FS_reg(context)  = LOWORD(FS_sig(sigcontext));
#else
    GET_FS( FS_reg(&DEBUG_context) );
    FS_reg(context) &= 0xffff;
#endif
#ifdef GS_sig
    GS_reg(context)  = LOWORD(GS_sig(sigcontext));
#else
    GET_GS( GS_reg(&DEBUG_context) );
    GS_reg(context) &= 0xffff;
#endif
}


/***********************************************************************
 *           SIGNAL_GetSigContext
 *
 * Build a sigcontext from the register values.
 */
static void SIGNAL_GetSigContext( SIGCONTEXT *sigcontext,
                                  const CONTEXT *context )
{
    EAX_sig(sigcontext) = EAX_reg(context);
    EBX_sig(sigcontext) = EBX_reg(context);
    ECX_sig(sigcontext) = ECX_reg(context);
    EDX_sig(sigcontext) = EDX_reg(context);
    ESI_sig(sigcontext) = ESI_reg(context);
    EDI_sig(sigcontext) = EDI_reg(context);
    EBP_sig(sigcontext) = EBP_reg(context);
    EFL_sig(sigcontext) = EFL_reg(context);
    EIP_sig(sigcontext) = EIP_reg(context);
    ESP_sig(sigcontext) = ESP_reg(context);
    CS_sig(sigcontext)  = CS_reg(context);
    DS_sig(sigcontext)  = DS_reg(context);
    ES_sig(sigcontext)  = ES_reg(context);
    SS_sig(sigcontext)  = SS_reg(context);
#ifdef FS_sig
    FS_sig(sigcontext)  = FS_reg(context);
#else
    SET_FS( FS_reg(&DEBUG_context) );
#endif
#ifdef GS_sig
    GS_sig(sigcontext)  = GS_reg(context);
#else
    SET_GS( GS_reg(&DEBUG_context) );
#endif
}


/***********************************************************************
 *           SIGNAL_InfoRegisters
 *
 * Display registers information. 
 */
void SIGNAL_InfoRegisters( CONTEXT *context )
{
    fprintf( stderr," CS:%04x SS:%04x DS:%04x ES:%04x FS:%04x GS:%04x",
             (WORD)CS_reg(context), (WORD)SS_reg(context),
             (WORD)DS_reg(context), (WORD)ES_reg(context),
             (WORD)FS_reg(context), (WORD)GS_reg(context) );
    fprintf( stderr, "\n EIP:%08lx ESP:%08lx EBP:%08lx EFLAGS:%08lx\n", 
             EIP_reg(context), ESP_reg(context),
             EBP_reg(context), EFL_reg(context) );
    fprintf( stderr, " EAX:%08lx EBX:%08lx ECX:%08lx EDX:%08lx\n", 
             EAX_reg(context), EBX_reg(context),
             ECX_reg(context), EDX_reg(context) );
    fprintf( stderr, " ESI:%08lx EDI:%08lx\n",
             ESI_reg(context), EDI_reg(context) );
}


/**********************************************************************
 *		SIGNAL_InitEmulator
 *
 * Initialize emulator signals.
 */
BOOL32 SIGNAL_InitEmulator(void)
{
    SIGNAL_SetHandler( SIGINT,  (void (*)())SIGNAL_break, 1);
    SIGNAL_SetHandler( SIGSEGV, (void (*)())SIGNAL_fault, 1);
    SIGNAL_SetHandler( SIGILL,  (void (*)())SIGNAL_fault, 1);
    SIGNAL_SetHandler( SIGFPE,  (void (*)())SIGNAL_fault, 1);
    SIGNAL_SetHandler( SIGTRAP, (void (*)())SIGNAL_trap,  1); /* debugger */
    SIGNAL_SetHandler( SIGHUP,  (void (*)())SIGNAL_trap,  1); /* forced break*/
#ifdef SIGBUS
    SIGNAL_SetHandler( SIGBUS,  (void (*)())SIGNAL_fault, 1);
#endif
    return TRUE;
}
