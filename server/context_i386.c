/*
 * i386 register context support
 *
 * Copyright (C) 1999 Alexandre Julliard
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

#ifdef __i386__

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>

#include "windef.h"

#include "file.h"
#include "thread.h"
#include "request.h"

/* copy a context structure according to the flags */
void copy_context( CONTEXT *to, const CONTEXT *from, unsigned int flags )
{
    flags &= ~CONTEXT_i386;  /* get rid of CPU id */
    if (flags & CONTEXT_CONTROL)
    {
        to->Ebp    = from->Ebp;
        to->Eip    = from->Eip;
        to->Esp    = from->Esp;
        to->SegCs  = from->SegCs;
        to->SegSs  = from->SegSs;
        to->EFlags = from->EFlags;
    }
    if (flags & CONTEXT_INTEGER)
    {
        to->Eax = from->Eax;
        to->Ebx = from->Ebx;
        to->Ecx = from->Ecx;
        to->Edx = from->Edx;
        to->Esi = from->Esi;
        to->Edi = from->Edi;
    }
    if (flags & CONTEXT_SEGMENTS)
    {
        to->SegDs = from->SegDs;
        to->SegEs = from->SegEs;
        to->SegFs = from->SegFs;
        to->SegGs = from->SegGs;
    }
    if (flags & CONTEXT_FLOATING_POINT)
    {
        to->FloatSave = from->FloatSave;
    }
    if (flags & CONTEXT_EXTENDED_REGISTERS)
    {
        memcpy( to->ExtendedRegisters, from->ExtendedRegisters, sizeof(to->ExtendedRegisters) );
    }
    if (flags & CONTEXT_DEBUG_REGISTERS)
    {
        to->Dr0 = from->Dr0;
        to->Dr1 = from->Dr1;
        to->Dr2 = from->Dr2;
        to->Dr3 = from->Dr3;
        to->Dr6 = from->Dr6;
        to->Dr7 = from->Dr7;
    }
    to->ContextFlags |= flags;
}

/* retrieve the current instruction pointer of a context */
void *get_context_ip( const CONTEXT *context )
{
    return (void *)context->Eip;
}

/* return the context flag that contains the CPU id */
unsigned int get_context_cpu_flag(void)
{
    return CONTEXT_i386;
}

/* return only the context flags that correspond to system regs */
/* (system regs are the ones we can't access on the client side) */
unsigned int get_context_system_regs( unsigned int flags )
{
    return flags & (CONTEXT_DEBUG_REGISTERS & ~CONTEXT_i386);
}

#endif  /* __i386__ */
