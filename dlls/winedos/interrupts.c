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

WINE_DEFAULT_DEBUG_CHANNEL(int);

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
  BOOL islong;

  if(context->SegCs == DOSVM_dpmi_segments->int48_sel) 
    islong = FALSE;
  else if(DOSVM_IsDos32())
    islong = TRUE;
  else if(IS_SELECTOR_32BIT(context->SegCs)) {
    WARN("Interrupt in 32-bit code and mode is not DPMI32\n");
    islong = TRUE;
  } else
    islong = FALSE;

  /* FIXME: Remove this check when DPMI32 support has been added */
  if(islong) {
    ERR("Interrupts not supported in 32-bit DPMI\n");
    islong = FALSE;
  }

  if(islong)
  {
    FARPROC48 addr = {0,0}; /* FIXME: INT_GetPMHandler48( intnum ); */
    DWORD *stack = CTX_SEG_OFF_TO_LIN(context, context->SegSs, context->Esp);
    /* Push the flags and return address on the stack */
    *(--stack) = context->EFlags;
    *(--stack) = context->SegCs;
    *(--stack) = context->Eip;
    /* Jump to the interrupt handler */
    context->SegCs  = addr.selector;
    context->Eip = addr.offset;
  }
  else
  {
    FARPROC16 addr = INT_GetPMHandler( intnum );
    WORD *stack = CTX_SEG_OFF_TO_LIN(context, context->SegSs, context->Esp);
    /* Push the flags and return address on the stack */
    *(--stack) = LOWORD(context->EFlags);
    *(--stack) = context->SegCs;
    *(--stack) = LOWORD(context->Eip);
    /* Jump to the interrupt handler */
    context->SegCs  = HIWORD(addr);
    context->Eip = LOWORD(addr);
  }

  if (IS_SELECTOR_32BIT(context->SegSs))
    context->Esp += islong ? -12 : -6;
  else
    ADD_LOWORD( context->Esp, islong ? -12 : -6 );
}
