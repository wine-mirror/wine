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

static FARPROC16 DOSVM_Vectors16[256];
static FARPROC48 DOSVM_Vectors48[256];
static INTPROC   DOSVM_VectorsBuiltin[256];

/* Ordinal number for interrupt 0 handler in winedos.dll and winedos16.dll */
#define FIRST_INTERRUPT 100

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

  if(islong)
  {
    FARPROC48 addr = DOSVM_GetPMHandler48( intnum );
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
    FARPROC16 addr = INT_GetPMHandler( intnum ); /* FIXME: DOSVM_GetPMHandler16 */
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
  static HMODULE16 procs;
  FARPROC16 handler = DOSVM_Vectors16[intnum];

  if (!handler)
  {
    if (!procs &&
        (procs = GetModuleHandle16( "winedos16" )) < 32 &&
        (procs = LoadLibrary16( "winedos16" )) < 32)
    {
      ERR("could not load winedos16.dll\n");
      procs = 0;
      return 0;
    }

    handler = GetProcAddress16( procs, (LPCSTR)(FIRST_INTERRUPT + intnum));
    if (!handler) 
    {
      WARN("int%x not implemented, returning dummy handler\n", intnum );
      handler = GetProcAddress16( procs, (LPCSTR)(FIRST_INTERRUPT + 256));
    }

    DOSVM_Vectors16[intnum] = handler;
  }

  return handler;
}


/**********************************************************************
 *          DOSVM_SetPMHandler
 *
 * Set the protected mode interrupt handler for a given interrupt.
 */
void DOSVM_SetPMHandler( BYTE intnum, FARPROC16 handler )
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
    DOSVM_Vectors48[intnum].offset = 4 * intnum;
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
  static HMODULE procs;
  INTPROC handler = DOSVM_VectorsBuiltin[intnum];

  if (!handler)
  {
    if (!procs)
      procs = LoadLibraryA( "winedos.dll" );

    if (!procs) 
    {
      ERR("could not load winedos.dll\n");
      return 0;
    }

    handler = (INTPROC)GetProcAddress( procs, 
                                      (LPCSTR)(FIRST_INTERRUPT + intnum));
    if (!handler) 
    {
      WARN("int%x not implemented, returning dummy handler\n", intnum );
      handler = (INTPROC)GetProcAddress( procs, 
                                        (LPCSTR)(FIRST_INTERRUPT + 256));
    }

    DOSVM_VectorsBuiltin[intnum] = handler;
  }

  return handler;
}

/**********************************************************************
 *         DOSVM_DefaultHandler
 *
 * Default interrupt handler. This will be used to emulate all
 * interrupts that don't have their own interrupt handler.
 */
void WINAPI DOSVM_DefaultHandler( CONTEXT86 *context )
{
}
