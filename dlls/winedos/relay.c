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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#include "dosexe.h"
#include "wine/winbase16.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);

/*
 * Magic DWORD used to check stack integrity.
 */
#define RELAY_MAGIC 0xabcdef00

/*
 * Memory block for temporary 16-bit stacks used with relay calls.
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
 * Allocate separate 16-bit stack, make stack pointer point to this 
 * stack and make code pointer point to stub that restores everything.
 * So, after this routine, SS and CS are guaranteed to be 16-bit.
 *
 * Note: This might be called from signal handler, so the stack
 *       allocation algorithm must be signal safe.
 */
static void RELAY_MakeShortContext( CONTEXT86 *context )
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


/**********************************************************************
 *          RELAY_RelayStub
 *
 * This stub is called by __wine_call_from_16_regs in order to marshall
 * relay parameters.
 */
static void __stdcall RELAY_RelayStub( DOSRELAY       proc, 
                                       unsigned char *args, 
                                       void          *ctx86 )
{
    if (proc)
    {
        CONTEXT86     *context    = (CONTEXT86*)ctx86;
        RELAY_Stack16 *stack      = RELAY_GetPointer( context->Esp );

        DWORD          old_seg_cs = context->SegCs;
        DWORD          old_eip    = context->Eip;
        DWORD          old_seg_ss = context->SegSs;
        DWORD          old_esp    = context->Esp;

        context->SegCs = stack->seg_cs;
        context->Eip   = stack->eip;
        context->SegSs = stack->seg_ss;
        context->Esp   = stack->esp;

        proc( context, *(LPVOID *)args );

        stack->seg_cs = context->SegCs;
        stack->eip    = context->Eip;
        stack->seg_ss = context->SegSs;
        stack->esp    = context->Esp;

        context->SegCs = old_seg_cs;
        context->Eip   = old_eip;
        context->SegSs = old_seg_ss;
        context->Esp   = old_esp;
    }
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
 *          DOSVM_BuildCallFrame
 *
 * Modifies the context so that return to context calls DOSRELAY and 
 * only after return from DOSRELAY the original context will be returned to.
 */
void DOSVM_BuildCallFrame( CONTEXT86 *context, DOSRELAY relay, LPVOID data )
{
    static void (*__wine_call_from_16_regs_ptr)();
    WORD  code_sel = DOSVM_dpmi_segments->relay_code_sel;

    /*
     * Allocate separate stack for relay call.
     */
    RELAY_MakeShortContext( context );

    /*
     * Build call frame.
     */
    PUSH_WORD16( context, HIWORD(data) );            /* argument.hiword */ 
    PUSH_WORD16( context, LOWORD(data) );            /* argument.loword */
    PUSH_WORD16( context, context->SegCs );          /* STACK16FRAME.cs */
    PUSH_WORD16( context, LOWORD(context->Eip) );    /* STACK16FRAME.ip */
    PUSH_WORD16( context, LOWORD(context->Ebp) );    /* STACK16FRAME.bp */
    PUSH_WORD16( context, HIWORD(relay) );           /* STACK16FRAME.entry_point.hiword */
    PUSH_WORD16( context, LOWORD(relay) );           /* STACK16FRAME.entry_point.loword */
    PUSH_WORD16( context, 0 );                       /* STACK16FRAME.entry_ip */
    PUSH_WORD16( context, HIWORD(RELAY_RelayStub) ); /* STACK16FRAME.relay.hiword */
    PUSH_WORD16( context, LOWORD(RELAY_RelayStub) ); /* STACK16FRAME.relay.loword */
    PUSH_WORD16( context, 0 );                       /* STACK16FRAME.module_cs.hiword */
    PUSH_WORD16( context, code_sel );                /* STACK16FRAME.module_cs.loword */
    PUSH_WORD16( context, 0 );                       /* STACK16FRAME.callfrom_ip.hiword */
    PUSH_WORD16( context, 0 );                       /* STACK16FRAME.callfrom_ip.loword */

    /*
     * Adjust code pointer.
     */
    if (!__wine_call_from_16_regs_ptr)
        __wine_call_from_16_regs_ptr = (void *)GetProcAddress(GetModuleHandleA("kernel32.dll"),
                                                              "__wine_call_from_16_regs" );
    context->SegCs = wine_get_cs();
    context->Eip = (DWORD)__wine_call_from_16_regs_ptr;
}
