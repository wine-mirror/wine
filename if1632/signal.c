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
#include "sigcontext.h"
#include "miscemu.h"


/* Signal handler declaration */

#ifdef linux
#define HANDLER_DEF(name) void name (int signal, SIGCONTEXT context_struct)
#define HANDLER_PROLOG SIGCONTEXT *context = &context_struct; {
#define HANDLER_EPILOG }
#elif defined(__svr4__) || defined(_SCO_DS)
#define HANDLER_DEF(name) void name (int signal, void *siginfo, SIGCONTEXT *context)
#define HANDLER_PROLOG  /* nothing */
#define HANDLER_EPILOG  /* nothing */
#else
#define HANDLER_DEF(name) void name (int signal, int code, SIGCONTEXT *context)
#define HANDLER_PROLOG  /* nothing */
#define HANDLER_EPILOG  /* nothing */
#endif

extern void SIGNAL_SetHandler( int sig, void (*func)(), int flags );
extern BOOL32 INSTR_EmulateInstruction( SIGCONTEXT *context );


/**********************************************************************
 *              SIGNAL_break
 * 
 * Handle Ctrl-C and such
 */
static HANDLER_DEF(SIGNAL_break)
{
    HANDLER_PROLOG;
    if (Options.debug) wine_debug( signal, context );  /* Enter our debugger */
    else exit(0);
    HANDLER_EPILOG;
}


/**********************************************************************
 *		SIGNAL_trap
 *
 * SIGTRAP handler.
 */
static HANDLER_DEF(SIGNAL_trap)
{
  HANDLER_PROLOG;
  wine_debug( signal, context );  /* Enter our debugger */
  HANDLER_EPILOG;
}


/**********************************************************************
 *		SIGNAL_fault
 *
 * Segfault handler.
 */
static HANDLER_DEF(SIGNAL_fault)
{
    HANDLER_PROLOG;
    if (CS_sig(context) == WINE_CODE_SELECTOR)
    {
        fprintf( stderr, "Segmentation fault in Wine program (%04x:%08lx)."
                         "  Please debug.\n",
                 (unsigned short) CS_sig(context), EIP_sig(context));
    }
    else
    {
        if (INSTR_EmulateInstruction( context )) return;
        fprintf( stderr, "Segmentation fault in Windows program %04x:%08lx.\n",
                 (unsigned short) CS_sig(context), EIP_sig(context) );
    }
    wine_debug( signal, context );
    HANDLER_EPILOG;
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
