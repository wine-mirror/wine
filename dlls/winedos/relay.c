/*
 * Routines for dynamically building calls to Wine from 
 * protected mode applications.
 *
 * Copyright 2002 Jukka Heinonen
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
#include "dosexe.h"
#include "stackframe.h"
#include "wine/debug.h"
#include "builtin16.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);

/*
 * Magic DWORD used to check stack integrity.
 */
#define RELAY_MAGIC 0xabcdef00

/*
 * Memory block for temporary 16-bit stacks used when
 * 32-bit code calls relay.
 */
typedef struct {
    DWORD inuse;          /* non-zero if stack block is in use */
    DWORD eip;            /* saved ip */
    DWORD seg_cs;         /* saved cs */
    DWORD esp;            /* saved sp */
    DWORD seg_ss;         /* saved ss */
    DWORD stack_bottom;   /* guard dword */
    BYTE  stack[256-7*4]; /* 16-bit stack */
    DWORD stack_top;      /* guard dword */
} RELAY_Stack16;


/**********************************************************************
 *          RELAY_GetPointer
 *
 * Get pointer to stack block when given esp pointing to 16-bit stack
 * inside relay data segment.
 */
static RELAY_Stack16 *RELAY_GetPointer( DWORD offset )
{
    offset = offset / sizeof(RELAY_Stack16) * sizeof(RELAY_Stack16);
    return MapSL(MAKESEGPTR(DOSVM_dpmi_segments->relay_data_sel, offset));
}


/**********************************************************************
 *          RELAY_MakeShortContext
 *
 * If context is using 32-bit stack or code segment, allocate
 * 16-bit stack, make stack pointer point to this stack and
 * make code pointer point to stub that restores everything.
 * So, after this routine, SS and CS are guaranteed to be 16-bit.
 *
 * Note: This might be called from signal handler, so the stack
 *       allocation algorithm must be signal safe.
 */
static void RELAY_MakeShortContext( CONTEXT86 *context )
{
    if (IS_SELECTOR_32BIT(context->SegCs) || IS_SELECTOR_32BIT(context->SegSs))
    {
        DWORD offset = offsetof(RELAY_Stack16, stack_top);
        RELAY_Stack16 *stack = RELAY_GetPointer( 0 );

        while (stack->inuse && offset < DOSVM_RELAY_DATA_SIZE) {
            stack++;
            offset += sizeof(RELAY_Stack16);
        }

        if (offset >= DOSVM_RELAY_DATA_SIZE)
            ERR( "Too many nested interrupts!\n" );
        
        stack->inuse = 1;
        stack->eip = context->Eip;
        stack->seg_cs = context->SegCs;
        stack->esp = context->Esp;
        stack->seg_ss = context->SegSs;

        stack->stack_bottom = RELAY_MAGIC;
        stack->stack_top = RELAY_MAGIC;

        context->SegSs = DOSVM_dpmi_segments->relay_data_sel;
        context->Esp = offset;
        context->SegCs = DOSVM_dpmi_segments->relay_code_sel;
        context->Eip = 3;
    }
}


/**********************************************************************
 *          RELAY_RelayStub
 *
 * This stub is called by __wine_call_from_16_regs in order to marshall
 * relay parameters.
 */
static void __stdcall RELAY_RelayStub( DOSRELAY proc, 
                                       unsigned char *args, 
                                       void *context )
{
    proc( (CONTEXT86*)context, *(LPVOID *)args );
}


/**********************************************************************
 *          DOSVM_RelayHandler
 *
 * Restore saved code and stack pointers and release stack block.
 */
void DOSVM_RelayHandler( CONTEXT86 *context )
{
    RELAY_Stack16 *stack = RELAY_GetPointer( context->Esp );

    context->SegSs = stack->seg_ss;
    context->Esp = stack->esp;
    context->SegCs = stack->seg_cs;
    context->Eip = stack->eip;

    if (!stack->inuse || 
        stack->stack_bottom != RELAY_MAGIC ||
        stack->stack_top != RELAY_MAGIC)
        ERR( "Stack corrupted!\n" );

    stack->inuse = 0;
}


/**********************************************************************
 *          DOSVM_SaveCallFrame
 *
 * Save current call frame. This routine must be called from DOSRELAY
 * called using DOSVM_BuildCallFrame before the relay modifies stack 
 * pointer. This routine makes sure that the relay can return safely
 * to application context and that no memory is leaked.
 *
 * Note: If DOSVM_BuildCallFrame was called using 32-bit CS or SS,
 *       old values of CS and SS will be lost. This does not matter
 *       since this routine is only used by Raw Mode Switch.
 */
void DOSVM_SaveCallFrame( CONTEXT86 *context, STACK16FRAME *frame )
{
    *frame = *CURRENT_STACK16;

    /*
     * If context is using allocated stack, release it.
     */
    if (context->SegSs == DOSVM_dpmi_segments->relay_data_sel)
    {
        RELAY_Stack16 *stack = RELAY_GetPointer( context->Esp );

        if (!stack->inuse || 
            stack->stack_bottom != RELAY_MAGIC ||
            stack->stack_top != RELAY_MAGIC)
            ERR( "Stack corrupted!\n" );

        stack->inuse = 0;
    }
}


/**********************************************************************
 *          DOSVM_RestoreCallFrame
 *
 * Restore saved call frame to currect stack. This routine must always
 * be called after DOSVM_SaveCallFrame has been called and before returning
 * from DOSRELAY.
 */
void DOSVM_RestoreCallFrame( CONTEXT86 *context, STACK16FRAME *frame )
{
    /*
     * Make sure that CS and SS are 16-bit.
     */
    RELAY_MakeShortContext( context );

    /*
     * After this function returns to relay code, protected mode
     * 16 bit stack will contain STACK16FRAME and single WORD
     * (EFlags, see next comment).
     */
    NtCurrentTeb()->cur_stack =
        MAKESEGPTR( context->SegSs,
                    context->Esp - sizeof(STACK16FRAME) - sizeof(WORD) );
    
    /*
     * After relay code returns to glue function, protected
     * mode 16 bit stack will contain interrupt return record:
     * IP, CS and EFlags. Since EFlags is ignored, it won't
     * need to be initialized.
     */
    context->Esp -= 3 * sizeof(WORD);

    /*
     * Restore stack frame so that relay code won't be confused.
     * It should be noted that relay code overwrites IP and CS
     * in STACK16FRAME with values taken from current CONTEXT86.
     * These values are what is returned to glue function
     * (see previous comment).
     */
    *CURRENT_STACK16 = *frame;
}


/**********************************************************************
 *          DOSVM_BuildCallFrame
 *
 * Modifies the context so that return to context calls DOSRELAY and 
 * only after return from DOSRELAY the original context will be returned to.
 */
void DOSVM_BuildCallFrame( CONTEXT86 *context, DOSRELAY relay, LPVOID data )
{
    WORD *stack;
    WORD  code_sel = DOSVM_dpmi_segments->relay_code_sel;

    /*
     * Make sure that CS and SS are 16-bit.
     */
    RELAY_MakeShortContext( context );

    /*
     * Get stack pointer after RELAY_MakeShortContext.
     */
    stack = CTX_SEG_OFF_TO_LIN(context, context->SegSs, context->Esp);

    /*
     * Build call frame.
     */
    *(--stack) = HIWORD(data);            /* argument.hiword */ 
    *(--stack) = LOWORD(data);            /* argument.loword */
    *(--stack) = context->SegCs;          /* STACK16FRAME.cs */
    *(--stack) = LOWORD(context->Eip);    /* STACK16FRAME.ip */
    *(--stack) = LOWORD(context->Ebp);    /* STACK16FRAME.bp */
    *(--stack) = HIWORD(relay);           /* STACK16FRAME.entry_point.hiword */
    *(--stack) = LOWORD(relay);           /* STACK16FRAME.entry_point.loword */
    *(--stack) = 0;                       /* STACK16FRAME.entry_ip */
    *(--stack) = HIWORD(RELAY_RelayStub); /* STACK16FRAME.relay.hiword */
    *(--stack) = LOWORD(RELAY_RelayStub); /* STACK16FRAME.relay.loword */
    *(--stack) = 0;                       /* STACK16FRAME.module_cs.hiword */
    *(--stack) = code_sel;                /* STACK16FRAME.module_cs.loword */
    *(--stack) = 0;                       /* STACK16FRAME.callfrom_ip.hiword */
    *(--stack) = 0;                       /* STACK16FRAME.callfrom_ip.loword */

    /*
     * Adjust stack and code pointers.
     */
    ADD_LOWORD( context->Esp, -28 );
    context->SegCs = wine_get_cs();
    context->Eip = (DWORD)__wine_call_from_16_regs;
}
