/*
 * Emulator signal handling
 *
 * Copyright 1995 Alexandre Julliard
 */

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

#include "debugger.h"
#include "options.h"
#include "sig_context.h"
#include "miscemu.h"
#include "thread.h"
#include "debug.h"

static const char * const SIGNAL_traps[] =
{
    "Division by zero exception",      /* 0 */
    "Debug exception",                 /* 1 */
    "NMI interrupt",                   /* 2 */
    "Breakpoint exception",            /* 3 */
    "Overflow exception",              /* 4 */
    "Bound range exception",           /* 5 */
    "Invalid opcode exception",        /* 6 */
    "Device not available exception",  /* 7 */
    "Double fault exception",          /* 8 */
    "Coprocessor segment overrun",     /* 9 */
    "Invalid TSS exception",           /* 10 */
    "Segment not present exception",   /* 11 */
    "Stack fault",                     /* 12 */
    "General protection fault",        /* 13 */
    "Page fault",                      /* 14 */
    "Unknown exception",               /* 15 */
    "Floating point exception",        /* 16 */
    "Alignment check exception",       /* 17 */
    "Machine check exception"          /* 18 */
};

#define NB_TRAPS  (sizeof(SIGNAL_traps) / sizeof(SIGNAL_traps[0]))

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
    const char *fault = "Segmentation fault";

    HANDLER_INIT();
    if (INSTR_EmulateInstruction( HANDLER_CONTEXT )) return;
#ifdef TRAP_sig
    if (TRAP_sig( HANDLER_CONTEXT ) < NB_TRAPS)
        fault = SIGNAL_traps[TRAP_sig( HANDLER_CONTEXT )];
#endif
    if (IS_SELECTOR_SYSTEM(CS_sig(HANDLER_CONTEXT)))
    {
        MSG("%s in 32-bit code (0x%08lx).\n", fault, EIP_sig(HANDLER_CONTEXT));
    }
    else
    {
        MSG("%s in 16-bit code (%04x:%04lx).\n", fault,
            (WORD)CS_sig(HANDLER_CONTEXT), EIP_sig(HANDLER_CONTEXT) );
    }
#ifdef CR2_sig
    MSG("Fault address is 0x%08lx\n",CR2_sig(HANDLER_CONTEXT));
#endif
    wine_debug( signal, HANDLER_CONTEXT );
}


/***********************************************************************
 *           SIGNAL_SetContext
 *
 * Set the register values from a sigcontext. 
 */
#ifdef UNUSED_FUNCTIONS
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
#endif


/***********************************************************************
 *           SIGNAL_GetSigContext
 *
 * Build a sigcontext from the register values.
 */
#ifdef UNUSED_FUNCTIONS
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
#endif


/***********************************************************************
 *           SIGNAL_InfoRegisters
 *
 * Display registers information. 
 */
void SIGNAL_InfoRegisters( CONTEXT *context )
{
    MSG(" CS:%04x SS:%04x DS:%04x ES:%04x FS:%04x GS:%04x",
             (WORD)CS_reg(context), (WORD)SS_reg(context),
             (WORD)DS_reg(context), (WORD)ES_reg(context),
             (WORD)FS_reg(context), (WORD)GS_reg(context) );
    MSG( "\n EIP:%08lx ESP:%08lx EBP:%08lx EFLAGS:%08lx\n", 
             EIP_reg(context), ESP_reg(context),
             EBP_reg(context), EFL_reg(context) );
    MSG( " EAX:%08lx EBX:%08lx ECX:%08lx EDX:%08lx\n", 
             EAX_reg(context), EBX_reg(context),
             ECX_reg(context), EDX_reg(context) );
    MSG( " ESI:%08lx EDI:%08lx\n",
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
