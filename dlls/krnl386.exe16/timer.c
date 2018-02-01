/*
 * 8253/8254 Programmable Interval Timer (PIT) emulation
 *
 * Copyright 2003 Jukka Heinonen
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

#include "config.h"

#include "dosexe.h"
#include "wingdi.h"
#include "winuser.h"


/***********************************************************************
 *              DOSVM_SetTimer
 */
void DOSVM_SetTimer( UINT ticks )
{
}


/***********************************************************************
 *              DOSVM_Int08Handler
 *
 * DOS interrupt 08h handler (IRQ0 - TIMER).
 */
void WINAPI DOSVM_Int08Handler( CONTEXT *context )
{
    BIOSDATA *bios_data      = DOSVM_BiosData();
    CONTEXT nested_context = *context;
    FARPROC16 int1c_proc     = DOSVM_GetRMHandler( 0x1c );
    
    nested_context.SegCs = SELECTOROF(int1c_proc);
    nested_context.Eip   = OFFSETOF(int1c_proc);

    /*
     * Update BIOS ticks since midnight.
     *
     * FIXME: What to do when number of ticks exceeds ticks per day?
     */
    bios_data->Ticks++;

    /*
     * If IRQ is called from protected mode, convert
     * context into VM86 context. Stack is invalidated so
     * that DPMI_CallRMProc allocates a new stack.
     */
    if (!ISV86(&nested_context))
    {
        nested_context.EFlags |= V86_FLAG;
        nested_context.SegSs = 0;
    }

    /*
     * Call interrupt 0x1c.
     */
    DPMI_CallRMProc( &nested_context, NULL, 0, TRUE );

    DOSVM_AcknowledgeIRQ( context );
}
