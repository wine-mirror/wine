/*
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Alexandre Julliard
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "wine/winbase16.h"
#include "winnt.h"
#include "heap.h"
#include "module.h"
#include "stackframe.h"
#include "selectors.h"
#include "builtin16.h"
#include "task.h"
#include "syslevel.h"
#include "debugtools.h"
#include "main.h"
#include "callback.h"

DEFAULT_DEBUG_CHANNEL(relay);

/***********************************************************************
 *           RELAY_Init
 */
BOOL RELAY_Init(void)
{
#ifdef __i386__
    WORD codesel;

      /* Allocate the code selector for CallTo16 routines */

    extern void Call16_Ret_Start(), Call16_Ret_End();
    extern void CallTo16_Ret();
    extern void CALL32_CBClient_Ret();
    extern void CALL32_CBClientEx_Ret();
    extern SEGPTR CallTo16_RetAddr;
    extern SEGPTR CALL32_CBClient_RetAddr;
    extern SEGPTR CALL32_CBClientEx_RetAddr;

    codesel = SELECTOR_AllocBlock( (void *)Call16_Ret_Start,
                                   (char *)Call16_Ret_End - (char *)Call16_Ret_Start,
                                   SEGMENT_CODE, TRUE, FALSE );
    if (!codesel) return FALSE;

      /* Patch the return addresses for CallTo16 routines */

    CallTo16_RetAddr = 
        PTR_SEG_OFF_TO_SEGPTR( codesel, (char*)CallTo16_Ret - (char*)Call16_Ret_Start );
    CALL32_CBClient_RetAddr = 
        PTR_SEG_OFF_TO_SEGPTR( codesel, (char*)CALL32_CBClient_Ret - (char*)Call16_Ret_Start );
    CALL32_CBClientEx_RetAddr = 
        PTR_SEG_OFF_TO_SEGPTR( codesel, (char*)CALL32_CBClientEx_Ret - (char*)Call16_Ret_Start );
#endif

    /* Initialize thunking */
    return THUNK_Init();
}

/*
 * Stubs for the CallTo16/CallFrom16 routines on non-Intel architectures
 * (these will never be called but need to be present to satisfy the linker ...)
 */
#ifndef __i386__
WORD CALLBACK CallTo16Word( FARPROC16 target, INT nArgs )
{ assert( FALSE ); }

LONG CALLBACK CallTo16Long( FARPROC16 target, INT nArgs )
{ assert( FALSE ); }

LONG CALLBACK CallTo16RegisterShort( const CONTEXT86 *context, INT nArgs )
{ assert( FALSE ); }

LONG CALLBACK CallTo16RegisterLong ( const CONTEXT86 *context, INT nArgs )
{ assert( FALSE ); }

WORD CallFrom16Word( void )       
{ assert( FALSE ); }

LONG CallFrom16Long( void )       
{ assert( FALSE ); }

void CallFrom16Register( void )   
{ assert( FALSE ); }

void CallFrom16Thunk( void )      
{ assert( FALSE ); }

DWORD WINAPI CALL32_CBClient( FARPROC proc, LPWORD args, DWORD *esi )
{ assert( FALSE ); }

DWORD WINAPI CALL32_CBClientEx( FARPROC proc, LPWORD args, DWORD *esi, INT *nArgs )
{ assert( FALSE ); }
#endif


/* from relay32/relay386.c */
extern char **debug_relay_excludelist,**debug_relay_includelist;

/***********************************************************************
 *           RELAY_DebugCallFrom16
 */
void RELAY_DebugCallFrom16( CONTEXT86 *context )
{
    STACK16FRAME *frame;
    WORD ordinal;
    char *args16, funstr[80];
    const char *args;
    int i, usecdecl, reg_func;

    if (!TRACE_ON(relay)) return;

    frame = CURRENT_STACK16;
    args = BUILTIN_GetEntryPoint16( frame, funstr, &ordinal );
    if (!args) return; /* happens for the two snoop register relays */
    if (!RELAY_ShowDebugmsgRelay(funstr)) return;
    DPRINTF( "Call %s(",funstr);
    VA_START16( args16 );

    usecdecl = ( *args == 'c' );
    args += 2;
    reg_func = (    memcmp( args, "regs_", 5 ) == 0
                 || memcmp( args, "intr_", 5 ) == 0 );
    args += 5;

    if (usecdecl)
    {
        while (*args)
        {
            switch(*args)
            {
            case 'w':
            case 's':
                DPRINTF( "0x%04x", *(WORD *)args16 );
                args16 += 2;
                break;
            case 'l':
                DPRINTF( "0x%08x", *(int *)args16 );
                args16 += 4;
                break;
            case 'p':
                DPRINTF( "%04x:%04x", *(WORD *)(args16+2), *(WORD *)args16 );
                args16 += 4;
                break;
            case 't':
            case 'T':
                DPRINTF( "%04x:%04x %s", *(WORD *)(args16+2), *(WORD *)args16,
                         debugres_a( (LPSTR)PTR_SEG_TO_LIN(*(SEGPTR *)args16 )) );
                args16 += 4;
                break;
            }
            args++;
            if (*args) DPRINTF( "," );
        }
    }
    else  /* not cdecl */
    {
        /* Start with the last arg */
        for (i = 0; args[i]; i++)
        {
            switch(args[i])
            {
            case 'w':
            case 's':
                args16 += 2;
                break;
            case 'l':
            case 'p':
            case 't':
            case 'T':
                args16 += 4;
                break;
            }
        }

        while (*args)
        {
            switch(*args)
            {
            case 'w':
            case 's':
                args16 -= 2;
                DPRINTF( "0x%04x", *(WORD *)args16 );
                break;
            case 'l':
                args16 -= 4;
                DPRINTF( "0x%08x", *(int *)args16 );
                break;
            case 't':
                args16 -= 4;
                DPRINTF( "0x%08x %s", *(int *)args16,
                         debugres_a( (LPSTR)PTR_SEG_TO_LIN(*(SEGPTR *)args16 )));
                break;
            case 'p':
                args16 -= 4;
                DPRINTF( "%04x:%04x", *(WORD *)(args16+2), *(WORD *)args16 );
                break;
            case 'T':
                args16 -= 4;
                DPRINTF( "%04x:%04x %s", *(WORD *)(args16+2), *(WORD *)args16,
                         debugres_a( (LPSTR)PTR_SEG_TO_LIN(*(SEGPTR *)args16 )));
                break;
            }
            args++;
            if (*args) DPRINTF( "," );
        }
    }

    DPRINTF( ") ret=%04x:%04x ds=%04x\n", frame->cs, frame->ip, frame->ds );
    VA_END16( args16 );

    if (reg_func)
        DPRINTF("     AX=%04x BX=%04x CX=%04x DX=%04x SI=%04x DI=%04x ES=%04x EFL=%08lx\n",
                AX_reg(context), BX_reg(context), CX_reg(context),
                DX_reg(context), SI_reg(context), DI_reg(context),
                (WORD)ES_reg(context), EFL_reg(context) );

    SYSLEVEL_CheckNotLevel( 2 );
}


/***********************************************************************
 *           RELAY_DebugCallFrom16Ret
 */
void RELAY_DebugCallFrom16Ret( CONTEXT86 *context, int ret_val )
{
    STACK16FRAME *frame;
    WORD ordinal;
    char funstr[80];
    const char *args;

    if (!TRACE_ON(relay)) return;
    frame = CURRENT_STACK16;
    args = BUILTIN_GetEntryPoint16( frame, funstr, &ordinal );
    if (!args) return;
    if (!RELAY_ShowDebugmsgRelay(funstr)) return;
    DPRINTF( "Ret  %s() ",funstr);

    if ( memcmp( args+2, "long_", 5 ) == 0 )
    {
        DPRINTF( "retval=0x%08x ret=%04x:%04x ds=%04x\n",
                 ret_val, frame->cs, frame->ip, frame->ds );
    }
    else if ( memcmp( args+2, "word_", 5 ) == 0 )
    {
        DPRINTF( "retval=0x%04x ret=%04x:%04x ds=%04x\n",
                 ret_val & 0xffff, frame->cs, frame->ip, frame->ds );
    }
    else if (    memcmp( args+2, "regs_", 5 ) == 0 
              || memcmp( args+2, "intr_", 5 ) == 0 )
    {
        DPRINTF("retval=none ret=%04x:%04x ds=%04x\n",
                (WORD)CS_reg(context), LOWORD(EIP_reg(context)), (WORD)DS_reg(context));
        DPRINTF("     AX=%04x BX=%04x CX=%04x DX=%04x SI=%04x DI=%04x ES=%04x EFL=%08lx\n",
                AX_reg(context), BX_reg(context), CX_reg(context),
                DX_reg(context), SI_reg(context), DI_reg(context),
                (WORD)ES_reg(context), EFL_reg(context) );
    }

    SYSLEVEL_CheckNotLevel( 2 );
}


/***********************************************************************
 *           RELAY_Unimplemented16
 *
 * This function is called for unimplemented 16-bit entry points (declared
 * as 'stub' in the spec file).
 */
void RELAY_Unimplemented16(void)
{
    WORD ordinal;
    char name[80];
    STACK16FRAME *frame = CURRENT_STACK16;
    BUILTIN_GetEntryPoint16( frame, name, &ordinal );
    MESSAGE("FATAL: No handler for Win16 routine %s (called from %04x:%04x)\n",
            name, frame->cs, frame->ip );
    ExitProcess(1);
}


/***********************************************************************
 *           RELAY_DebugCallTo16
 *
 * 'target' contains either the function to call (normal CallTo16)
 * or a pointer to the CONTEXT86 struct (register CallTo16).
 * 'nb_args' is the number of argument bytes on the 16-bit stack; 
 * 'reg_func' specifies whether we have a register CallTo16 or not.
 */
void RELAY_DebugCallTo16( LPVOID target, int nb_args, BOOL reg_func )
{
    WORD *stack16;
    TEB *teb;

    if (!TRACE_ON(relay)) return;
    teb = NtCurrentTeb();
    stack16 = (WORD *)THREAD_STACK16(teb);

    nb_args /= sizeof(WORD);

    if ( reg_func )
    {
        CONTEXT86 *context = (CONTEXT86 *)target;

        DPRINTF("CallTo16(func=%04lx:%04x,ds=%04lx",
                CS_reg(context), LOWORD(EIP_reg(context)), DS_reg(context) );
        while (nb_args--) DPRINTF( ",0x%04x", *--stack16 );
        DPRINTF(") ss:sp=%04x:%04x\n", SELECTOROF(teb->cur_stack),
                OFFSETOF(teb->cur_stack) );
        DPRINTF("     AX=%04x BX=%04x CX=%04x DX=%04x SI=%04x DI=%04x BP=%04x ES=%04x FS=%04x\n",
                AX_reg(context), BX_reg(context), CX_reg(context),
                DX_reg(context), SI_reg(context), DI_reg(context),
                BP_reg(context), (WORD)ES_reg(context), (WORD)FS_reg(context) );
    }
    else
    {
        DPRINTF("CallTo16(func=%04x:%04x,ds=%04x",
                HIWORD(target), LOWORD(target), SELECTOROF(teb->cur_stack) );
        while (nb_args--) DPRINTF( ",0x%04x", *--stack16 );
        DPRINTF(") ss:sp=%04x:%04x\n", SELECTOROF(teb->cur_stack),
                OFFSETOF(teb->cur_stack) );
    }

    SYSLEVEL_CheckNotLevel( 2 );
}


/***********************************************************************
 *           RELAY_DebugCallTo16Ret
 */
void RELAY_DebugCallTo16Ret( int ret_val )
{
    if (!TRACE_ON(relay)) return;

    DPRINTF("CallTo16() ss:sp=%04x:%04x retval=0x%08x\n", 
            SELECTOROF(NtCurrentTeb()->cur_stack),
            OFFSETOF(NtCurrentTeb()->cur_stack), ret_val);
    SYSLEVEL_CheckNotLevel( 2 );
}


/**********************************************************************
 *	     Catch    (KERNEL.55)
 *
 * Real prototype is:
 *   INT16 WINAPI Catch( LPCATCHBUF lpbuf );
 */
void WINAPI Catch16( LPCATCHBUF lpbuf, CONTEXT86 *context )
{
    /* Note: we don't save the current ss, as the catch buffer is */
    /* only 9 words long. Hopefully no one will have the silly    */
    /* idea to change the current stack before calling Throw()... */

    /* Windows uses:
     * lpbuf[0] = ip
     * lpbuf[1] = cs
     * lpbuf[2] = sp
     * lpbuf[3] = bp
     * lpbuf[4] = si
     * lpbuf[5] = di
     * lpbuf[6] = ds
     * lpbuf[7] = unused
     * lpbuf[8] = ss
     */

    lpbuf[0] = LOWORD(EIP_reg(context));
    lpbuf[1] = CS_reg(context);
    /* Windows pushes 4 more words before saving sp */
    lpbuf[2] = LOWORD(ESP_reg(context)) - 4 * sizeof(WORD);
    lpbuf[3] = LOWORD(EBP_reg(context));
    lpbuf[4] = LOWORD(ESI_reg(context));
    lpbuf[5] = LOWORD(EDI_reg(context));
    lpbuf[6] = DS_reg(context);
    lpbuf[7] = 0;
    lpbuf[8] = SS_reg(context);
    AX_reg(context) = 0;  /* Return 0 */
}


/**********************************************************************
 *	     Throw    (KERNEL.56)
 *
 * Real prototype is:
 *   INT16 WINAPI Throw( LPCATCHBUF lpbuf, INT16 retval );
 */
void WINAPI Throw16( LPCATCHBUF lpbuf, INT16 retval, CONTEXT86 *context )
{
    STACK16FRAME *pFrame;
    STACK32FRAME *frame32;
    TEB *teb = NtCurrentTeb();

    AX_reg(context) = retval;

    /* Find the frame32 corresponding to the frame16 we are jumping to */
    pFrame = THREAD_STACK16(teb);
    frame32 = pFrame->frame32;
    while (frame32 && frame32->frame16)
    {
        if (OFFSETOF(frame32->frame16) < OFFSETOF(teb->cur_stack))
            break;  /* Something strange is going on */
        if (OFFSETOF(frame32->frame16) > lpbuf[2])
        {
            /* We found the right frame */
            pFrame->frame32 = frame32;
            break;
        }
        frame32 = ((STACK16FRAME *)PTR_SEG_TO_LIN(frame32->frame16))->frame32;
    }

    EIP_reg(context) = lpbuf[0];
    CS_reg(context)  = lpbuf[1];
    ESP_reg(context) = lpbuf[2] + 4 * sizeof(WORD) - sizeof(WORD) /*extra arg*/;
    EBP_reg(context) = lpbuf[3];
    ESI_reg(context) = lpbuf[4];
    EDI_reg(context) = lpbuf[5];
    DS_reg(context)  = lpbuf[6];

    if (lpbuf[8] != SS_reg(context))
        ERR("Switching stack segment with Throw() not supported; expect crash now\n" );
}

