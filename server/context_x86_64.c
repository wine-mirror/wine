/*
 * x86-64 register context support
 *
 * Copyright (C) 1999, 2005 Alexandre Julliard
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

#ifdef __x86_64__

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>

#define NONAMELESSUNION
#include "windef.h"
#include "winbase.h"

#include "file.h"
#include "thread.h"
#include "request.h"

/* copy a context structure according to the flags */
void copy_context( CONTEXT *to, const CONTEXT *from, unsigned int flags )
{
    flags &= ~CONTEXT_AMD64;  /* get rid of CPU id */
    if (flags & CONTEXT_CONTROL)
    {
        to->Rbp    = from->Rbp;
        to->Rip    = from->Rip;
        to->Rsp    = from->Rsp;
        to->SegCs  = from->SegCs;
        to->SegSs  = from->SegSs;
        to->EFlags = from->EFlags;
        to->MxCsr  = from->MxCsr;
    }
    if (flags & CONTEXT_INTEGER)
    {
        to->Rax = from->Rax;
        to->Rcx = from->Rcx;
        to->Rdx = from->Rdx;
        to->Rbx = from->Rbx;
        to->Rsi = from->Rsi;
        to->Rdi = from->Rdi;
        to->R8  = from->R8;
        to->R9  = from->R9;
        to->R10 = from->R10;
        to->R11 = from->R11;
        to->R12 = from->R12;
        to->R13 = from->R13;
        to->R14 = from->R14;
        to->R15 = from->R15;
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
        to->u.FltSave = from->u.FltSave;
    }
    /* we don't bother copying the debug registers, since they */
    /* always need to be accessed by ptrace anyway */
    to->ContextFlags |= flags & ~CONTEXT_DEBUG_REGISTERS;
}

/* retrieve the current instruction pointer of a context */
void *get_context_ip( const CONTEXT *context )
{
    return (void *)context->Rip;
}

/* return the context flag that contains the CPU id */
unsigned int get_context_cpu_flag(void)
{
    return CONTEXT_AMD64;
}

/* return only the context flags that correspond to system regs */
/* (system regs are the ones we can't access on the client side) */
unsigned int get_context_system_regs( unsigned int flags )
{
    return flags & (CONTEXT_DEBUG_REGISTERS & ~CONTEXT_AMD64);
}

#endif  /* __x86_64__ */
