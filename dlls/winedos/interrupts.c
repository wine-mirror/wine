/*
 * Interrupt emulation
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
#include "wine/debug.h"
#include "wine/winbase16.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);

static FARPROC16     DOSVM_Vectors16[256];
static FARPROC48     DOSVM_Vectors48[256];
static const INTPROC DOSVM_VectorsBuiltin[] =
{
  /* 00 */ 0,                  0,                  0,                  0,
  /* 04 */ 0,                  0,                  0,                  0,
  /* 08 */ 0,                  DOSVM_Int09Handler, 0,                  0,
  /* 0C */ 0,                  0,                  0,                  0,
  /* 10 */ DOSVM_Int10Handler, DOSVM_Int11Handler, DOSVM_Int12Handler, DOSVM_Int13Handler,
  /* 14 */ 0,                  DOSVM_Int15Handler, DOSVM_Int16Handler, DOSVM_Int17Handler,
  /* 18 */ 0,                  0,                  DOSVM_Int1aHandler, 0,
  /* 1C */ 0,                  0,                  0,                  0,
  /* 20 */ DOSVM_Int20Handler, DOSVM_Int21Handler, 0,                  0,
  /* 24 */ 0,                  DOSVM_Int25Handler, DOSVM_Int26Handler, 0,
  /* 28 */ 0,                  DOSVM_Int29Handler, DOSVM_Int2aHandler, 0,
  /* 2C */ 0,                  0,                  0,                  DOSVM_Int2fHandler,
  /* 30 */ 0,                  DOSVM_Int31Handler, 0,                  DOSVM_Int33Handler,
  /* 34 */ DOSVM_Int34Handler, DOSVM_Int35Handler, DOSVM_Int36Handler, DOSVM_Int37Handler,
  /* 38 */ DOSVM_Int38Handler, DOSVM_Int39Handler, DOSVM_Int3aHandler, DOSVM_Int3bHandler,
  /* 3C */ DOSVM_Int3cHandler, DOSVM_Int3dHandler, DOSVM_Int3eHandler, 0,
  /* 40 */ 0,                  DOSVM_Int41Handler, 0,                  0,
  /* 44 */ 0,                  0,                  0,                  0,
  /* 48 */ 0,                  0,                  0,                  DOSVM_Int4bHandler,
  /* 4C */ 0,                  0,                  0,                  0,
  /* 50 */ 0,                  0,                  0,                  0,
  /* 54 */ 0,                  0,                  0,                  0,
  /* 58 */ 0,                  0,                  0,                  0,
  /* 5C */ DOSVM_Int5cHandler, 0,                  0,                  0,
  /* 60 */ 0,                  0,                  0,                  0,
  /* 64 */ 0,                  0,                  0,                  DOSVM_Int67Handler
};


/**********************************************************************
 *         DOSVM_DefaultHandler
 *
 * Default interrupt handler. This will be used to emulate all
 * interrupts that don't have their own interrupt handler.
 */
void WINAPI DOSVM_DefaultHandler( CONTEXT86 *context )
{
}


/**********************************************************************
 *          DOSVM_IntProcRelay
 *
 * Simple DOSRELAY that interprets its argument as INTPROC and calls it.
 */
static void DOSVM_IntProcRelay( CONTEXT86 *context, LPVOID data )
{
    INTPROC proc = (INTPROC)data;
    proc(context);
}


/**********************************************************************
 *          DOSVM_PushFlags
 *
 * This routine is used to make default int25 and int26 handlers leave the 
 * original eflags into stack. In order to do this, stack is manipulated
 * so that it actually contains two copies of eflags, one of which is
 * popped during return from interrupt handler.
 */
static void DOSVM_PushFlags( CONTEXT86 *context, BOOL islong, BOOL isstub )
{
    if (islong)
    {
        DWORD *stack = CTX_SEG_OFF_TO_LIN(context, 
                                          context->SegSs, 
                                          context->Esp);
        context->Esp += -4; /* One item will be added to stack. */

        if (isstub)
        {
            DWORD ip = stack[0];
            DWORD cs = stack[1];
            stack += 2; /* Pop ip and cs. */
            *(--stack) = context->EFlags;
            *(--stack) = cs;
            *(--stack) = ip;
        }
        else
            *(--stack) = context->EFlags;            
    }
    else
    {
        WORD *stack = CTX_SEG_OFF_TO_LIN(context, 
                                         context->SegSs, 
                                         context->Esp);
        ADD_LOWORD( context->Esp, -2 ); /* One item will be added to stack. */

        if (isstub)
        {
            WORD ip = stack[0];
            WORD cs = stack[1];
            stack += 2; /* Pop ip and cs. */
            *(--stack) = LOWORD(context->EFlags);
            *(--stack) = cs;
            *(--stack) = ip;
        }
        else
            *(--stack) = LOWORD(context->EFlags);
    }
}


/**********************************************************************
 *         DOSVM_EmulateInterruptPM
 *
 * Emulate software interrupt in 16-bit or 32-bit protected mode.
 * Called from signal handler when intXX opcode is executed. 
 *
 * Pushes interrupt frame to stack and changes instruction 
 * pointer to interrupt handler.
 */
void WINAPI DOSVM_EmulateInterruptPM( CONTEXT86 *context, BYTE intnum ) 
{
    if (context->SegCs == DOSVM_dpmi_segments->dpmi_sel)
    {
        DOSVM_BuildCallFrame( context, 
                              DOSVM_IntProcRelay,
                              DOSVM_RawModeSwitchHandler );
    }
    else if (context->SegCs == DOSVM_dpmi_segments->relay_code_sel)
    {
        /*
         * This must not be called using DOSVM_BuildCallFrame.
         */
        DOSVM_RelayHandler( context );
    }
    else if (context->SegCs == DOSVM_dpmi_segments->int48_sel)
    {
        if (intnum == 0x25 || intnum == 0x26)
            DOSVM_PushFlags( context, TRUE, TRUE );

        DOSVM_BuildCallFrame( context, 
                              DOSVM_IntProcRelay,
                              DOSVM_GetBuiltinHandler(intnum) );
    }
    else if (context->SegCs == DOSVM_dpmi_segments->int16_sel)
    {
        if (intnum == 0x25 || intnum == 0x26)
            DOSVM_PushFlags( context, FALSE, TRUE );

        DOSVM_BuildCallFrame( context, 
                              DOSVM_IntProcRelay, 
                              DOSVM_GetBuiltinHandler(intnum) );
    }
    else if(DOSVM_IsDos32())
    {
        FARPROC48 addr = DOSVM_GetPMHandler48( intnum );
        
        if (addr.selector == DOSVM_dpmi_segments->int48_sel)
        {
            if (intnum == 0x25 || intnum == 0x26)
                DOSVM_PushFlags( context, TRUE, FALSE );

             DOSVM_BuildCallFrame( context, 
                                   DOSVM_IntProcRelay,
                                   DOSVM_GetBuiltinHandler(intnum) );
        }
        else
        {
            DWORD *stack = CTX_SEG_OFF_TO_LIN(context, 
                                              context->SegSs, 
                                              context->Esp);
            
            /* Push the flags and return address on the stack */
            *(--stack) = context->EFlags;
            *(--stack) = context->SegCs;
            *(--stack) = context->Eip;
            context->Esp += -12;

            /* Jump to the interrupt handler */
            context->SegCs  = addr.selector;
            context->Eip = addr.offset;
        }
    }
    else
    {
        FARPROC16 addr = DOSVM_GetPMHandler16( intnum );

        if (SELECTOROF(addr) == DOSVM_dpmi_segments->int16_sel)
        {
            if (intnum == 0x25 || intnum == 0x26)
                DOSVM_PushFlags( context, FALSE, FALSE );

            DOSVM_BuildCallFrame( context, 
                                  DOSVM_IntProcRelay,
                                  DOSVM_GetBuiltinHandler(intnum) );
        }
        else
        {
            WORD *stack = CTX_SEG_OFF_TO_LIN(context, 
                                             context->SegSs, 
                                             context->Esp);
            
            /* Push the flags and return address on the stack */
            *(--stack) = LOWORD(context->EFlags);
            *(--stack) = context->SegCs;
            *(--stack) = LOWORD(context->Eip);
            ADD_LOWORD( context->Esp, -6 );

            /* Jump to the interrupt handler */
            context->SegCs  = HIWORD(addr);
            context->Eip = LOWORD(addr);
        }
    }
}

/**********************************************************************
 *          DOSVM_GetRMHandler
 *
 * Return the real mode interrupt vector for a given interrupt.
 */
FARPROC16 DOSVM_GetRMHandler( BYTE intnum )
{
  return ((FARPROC16*)0)[intnum];
}

/**********************************************************************
 *          DOSVM_SetRMHandler
 *
 * Set the real mode interrupt handler for a given interrupt.
 */
void DOSVM_SetRMHandler( BYTE intnum, FARPROC16 handler )
{
  TRACE("Set real mode interrupt vector %02x <- %04x:%04x\n",
       intnum, HIWORD(handler), LOWORD(handler) );
  ((FARPROC16*)0)[intnum] = handler;
}


/**********************************************************************
 *          DOSVM_GetPMHandler16
 *
 * Return the protected mode interrupt vector for a given interrupt.
 */
FARPROC16 DOSVM_GetPMHandler16( BYTE intnum )
{
    if (!DOSVM_Vectors16[intnum])
    {
        FARPROC16 proc = (FARPROC16)MAKESEGPTR( DOSVM_dpmi_segments->int16_sel,
                                                5 * intnum );
        DOSVM_Vectors16[intnum] = proc;
    }
    return DOSVM_Vectors16[intnum];
}


/**********************************************************************
 *          DOSVM_SetPMHandler16
 *
 * Set the protected mode interrupt handler for a given interrupt.
 */
void DOSVM_SetPMHandler16( BYTE intnum, FARPROC16 handler )
{
  TRACE("Set protected mode interrupt vector %02x <- %04x:%04x\n",
       intnum, HIWORD(handler), LOWORD(handler) );
  DOSVM_Vectors16[intnum] = handler;
}

/**********************************************************************
 *         DOSVM_GetPMHandler48
 *
 * Return the protected mode interrupt vector for a given interrupt.
 * Used to get 48-bit pointer for 32-bit interrupt handlers in DPMI32.
 */
FARPROC48 DOSVM_GetPMHandler48( BYTE intnum )
{
  if (!DOSVM_Vectors48[intnum].selector)
  {
    DOSVM_Vectors48[intnum].selector = DOSVM_dpmi_segments->int48_sel;
    DOSVM_Vectors48[intnum].offset = 6 * intnum;
  }
  return DOSVM_Vectors48[intnum];
}

/**********************************************************************
 *         DOSVM_SetPMHandler48
 *
 * Set the protected mode interrupt handler for a given interrupt.
 * Used to set 48-bit pointer for 32-bit interrupt handlers in DPMI32.
 */
void DOSVM_SetPMHandler48( BYTE intnum, FARPROC48 handler )
{
  TRACE("Set 32-bit protected mode interrupt vector %02x <- %04x:%08lx\n",
       intnum, handler.selector, handler.offset );
  DOSVM_Vectors48[intnum] = handler;
}

/**********************************************************************
 *         DOSVM_GetBuiltinHandler
 *
 * Return Wine interrupt handler procedure for a given interrupt.
 */
INTPROC DOSVM_GetBuiltinHandler( BYTE intnum )
{
  if (intnum < sizeof(DOSVM_VectorsBuiltin)/sizeof(INTPROC)) {
    INTPROC proc = DOSVM_VectorsBuiltin[intnum];
    if(proc)
      return proc;
  }

  WARN("int%x not implemented, returning dummy handler\n", intnum );
  return DOSVM_DefaultHandler;
}

/**********************************************************************
 *         DOSVM_CallBuiltinHandler
 *
 * Execute Wine interrupt handler procedure.
 */
void WINAPI DOSVM_CallBuiltinHandler( CONTEXT86 *context, BYTE intnum ) 
{
  INTPROC proc = DOSVM_GetBuiltinHandler( intnum );
  proc( context );
}
