/*
 * msvcrt C++ exception handling, PowerPC64
 *
 * Copyright 2011 Alexandre Julliard
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

#ifdef __powerpc64__

#include <setjmp.h>
#include <stdarg.h>
#include <fpieee.h>

#include "ntstatus.h"
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "msvcrt.h"
#include "excpt.h"
#include "wine/debug.h"

#include "cppexcept.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);

/*
 * ### Every entry point in this file is currently unreachable. ###
 *
 * All four are only called from __CxxFrameHandler / __CxxFrameHandler4, which
 * ntdll only ever dispatches through DISPATCHER_CONTEXT.LanguageHandler.  That
 * field is filled from a RUNTIME_FUNCTION, and on PowerPC64 there is no PE
 * unwind format and no producer for one, so RtlLookupFunctionEntry() always
 * returns NULL and the handler is never reached.  See the ppc64 arm of
 * dlls/ntdll/unwind.c and the comment on IMAGE_PPC64_RUNTIME_FUNCTION_ENTRY in
 * include/winnt.h.
 *
 * They exist so that msvcrt.dll and ucrtbase.dll link.  If real C++ EH is ever
 * built for this architecture the funclet calling convention below is the part
 * that has to be designed, not merely filled in: there is no Microsoft ABI for
 * PowerPC64 to match, and DISPATCHER_CONTEXT.NonVolatileRegisters is never
 * populated, so a funclet cannot be entered with its parent frame's registers.
 */

static void *call_exc_handler( void *handler, ULONG_PTR frame, UINT flags, BYTE *nonvol_regs )
{
    void * (*func)( ULONG_PTR frame, UINT flags ) = handler;

    if (nonvol_regs)
        FIXME( "non-volatile registers are not restored before the handler runs\n" );
    return func( frame, flags );
}


/*******************************************************************
 *		call_catch_handler
 */
void *call_catch_handler( EXCEPTION_RECORD *rec )
{
    ULONG_PTR frame = rec->ExceptionInformation[1];
    void *handler = (void *)rec->ExceptionInformation[5];
    BYTE *nonvol_regs = (BYTE *)rec->ExceptionInformation[10];

    TRACE( "calling %p frame %Ix\n", handler, frame );
    return call_exc_handler( handler, frame, 0x100, nonvol_regs );
}


/*******************************************************************
 *		call_unwind_handler
 */
void *call_unwind_handler( void *handler, ULONG_PTR frame, DISPATCHER_CONTEXT *dispatch )
{
    TRACE( "calling %p frame %Ix\n", handler, frame );
    return call_exc_handler( handler, frame, 0x100, dispatch->NonVolatileRegisters );
}


/*******************************************************************
 *		get_exception_pc
 */
ULONG_PTR get_exception_pc( DISPATCHER_CONTEXT *dispatch )
{
    ULONG_PTR pc = dispatch->ControlPc;
    if (dispatch->ControlPcIsUnwound) pc -= 4;   /* fixed-width instructions */
    return pc;
}


/*********************************************************************
 *              handle_fpieee_flt
 */
int handle_fpieee_flt( __msvcrt_ulong exception_code, EXCEPTION_POINTERS *ep,
                       int (__cdecl *handler)(_FPIEEE_RECORD*) )
{
    FIXME("(%lx %p %p)\n", exception_code, ep, handler);
    return EXCEPTION_CONTINUE_SEARCH;
}

#endif  /* __powerpc64__ */
