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
    extern DWORD CallTo16_DataSelector;
    extern SEGPTR CALL32_CBClient_RetAddr;
    extern SEGPTR CALL32_CBClientEx_RetAddr;

    codesel = SELECTOR_AllocBlock( (void *)Call16_Ret_Start,
                                   (char *)Call16_Ret_End - (char *)Call16_Ret_Start,
                                   WINE_LDT_FLAGS_CODE | WINE_LDT_FLAGS_32BIT );
    if (!codesel) return FALSE;

      /* Patch the return addresses for CallTo16 routines */

    CallTo16_DataSelector = __get_ds();
    CallTo16_RetAddr = 
        PTR_SEG_OFF_TO_SEGPTR( codesel, (char*)CallTo16_Ret - (char*)Call16_Ret_Start );
    CALL32_CBClient_RetAddr = 
        PTR_SEG_OFF_TO_SEGPTR( codesel, (char*)CALL32_CBClient_Ret - (char*)Call16_Ret_Start );
    CALL32_CBClientEx_RetAddr = 
        PTR_SEG_OFF_TO_SEGPTR( codesel, (char*)CALL32_CBClientEx_Ret - (char*)Call16_Ret_Start );
#endif
    return TRUE;
}

/*
 * Stubs for the CallTo16/CallFrom16 routines on non-Intel architectures
 * (these will never be called but need to be present to satisfy the linker ...)
 */
#ifndef __i386__
/***********************************************************************
 *		wine_call_to_16_word
 */
WORD WINAPI wine_call_to_16_word( FARPROC16 target, INT nArgs )
{
    assert( FALSE );
}

/***********************************************************************
 *		wine_call_to_16_long
 */
LONG WINAPI wine_call_to_16_long( FARPROC16 target, INT nArgs )
{
    assert( FALSE );
}

/***********************************************************************
 *		wine_call_to_16_regs_short
 */
void WINAPI wine_call_to_16_regs_short( CONTEXT86 *context, INT nArgs )
{
    assert( FALSE );
}

/***********************************************************************
 *		wine_call_to_16_regs_long
 */
void WINAPI wine_call_to_16_regs_long ( CONTEXT86 *context, INT nArgs )
{
    assert( FALSE );
}

/***********************************************************************
 *		__wine_call_from_16_word
 */
WORD __cdecl __wine_call_from_16_word(...)
{
    assert( FALSE );
}

/***********************************************************************
 *		__wine_call_from_16_long
 */
LONG __cdecl __wine_call_from_16_long(...)
{
    assert( FALSE );
}

/***********************************************************************
 *		__wine_call_from_16_regs
 */
void __cdecl __wine_call_from_16_regs(...)
{
    assert( FALSE );
}

/***********************************************************************
 *		__wine_call_from_16_thunk
 */
void __cdecl __wine_call_from_16_thunk(...)
{
    assert( FALSE );
}

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
                (WORD)context->SegEs, context->EFlags );

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
                (WORD)context->SegCs, LOWORD(context->Eip), (WORD)context->SegDs);
        DPRINTF("     AX=%04x BX=%04x CX=%04x DX=%04x SI=%04x DI=%04x ES=%04x EFL=%08lx\n",
                AX_reg(context), BX_reg(context), CX_reg(context),
                DX_reg(context), SI_reg(context), DI_reg(context),
                (WORD)context->SegEs, context->EFlags );
    }

    SYSLEVEL_CheckNotLevel( 2 );
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
                context->SegCs, LOWORD(context->Eip), context->SegDs );
        while (nb_args--) DPRINTF( ",0x%04x", *--stack16 );
        DPRINTF(") ss:sp=%04x:%04x\n", SELECTOROF(teb->cur_stack),
                OFFSETOF(teb->cur_stack) );
        DPRINTF("     AX=%04x BX=%04x CX=%04x DX=%04x SI=%04x DI=%04x BP=%04x ES=%04x FS=%04x\n",
                AX_reg(context), BX_reg(context), CX_reg(context),
                DX_reg(context), SI_reg(context), DI_reg(context),
                BP_reg(context), (WORD)context->SegEs, (WORD)context->SegFs );
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
void RELAY_DebugCallTo16Ret( BOOL reg_func, int ret_val )
{
    if (!TRACE_ON(relay)) return;

    if (!reg_func)
    {
        DPRINTF("CallTo16() ss:sp=%04x:%04x retval=0x%08x\n", 
                SELECTOROF(NtCurrentTeb()->cur_stack),
                OFFSETOF(NtCurrentTeb()->cur_stack), ret_val);
    }
    else
    {
        CONTEXT86 *context = (CONTEXT86 *)ret_val;

        DPRINTF("CallTo16() ss:sp=%04x:%04x\n", 
                SELECTOROF(NtCurrentTeb()->cur_stack),
                OFFSETOF(NtCurrentTeb()->cur_stack));
        DPRINTF("     AX=%04x BX=%04x CX=%04x DX=%04x BP=%04x SP=%04x\n",
                AX_reg(context), BX_reg(context), CX_reg(context),
                DX_reg(context), BP_reg(context), LOWORD(context->Esp));
    }

    SYSLEVEL_CheckNotLevel( 2 );
}
