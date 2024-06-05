/*
 * msvcrt C++ exception handling
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

#if defined(__x86_64__) && !defined(__arm64ec__)

#include <stdarg.h>
#include <fpieee.h>
#define longjmp ms_longjmp  /* avoid prototype mismatch */
#include <setjmp.h>
#undef longjmp

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "msvcrt.h"
#include "wine/exception.h"
#include "excpt.h"
#include "wine/debug.h"

#include "cppexcept.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);


/*******************************************************************
 *		longjmp (MSVCRT.@)
 */
#ifndef __WINE_PE_BUILD
void __cdecl longjmp( _JUMP_BUFFER *jmp, int retval )
{
    EXCEPTION_RECORD rec;

    if (!retval) retval = 1;
    if (jmp->Frame)
    {
        rec.ExceptionCode = STATUS_LONGJUMP;
        rec.ExceptionFlags = 0;
        rec.ExceptionRecord = NULL;
        rec.ExceptionAddress = NULL;
        rec.NumberParameters = 1;
        rec.ExceptionInformation[0] = (DWORD_PTR)jmp;
        RtlUnwind( (void *)jmp->Frame, (void *)jmp->Rip, &rec, IntToPtr(retval) );
    }
    __wine_longjmp( (__wine_jmp_buf *)jmp, retval );
}
#endif

/*******************************************************************
 *		_local_unwind (MSVCRT.@)
 */
void __cdecl _local_unwind( void *frame, void *target )
{
    RtlUnwind( frame, target, NULL, 0 );
}

/*********************************************************************
 *              handle_fpieee_flt
 */
int handle_fpieee_flt( __msvcrt_ulong exception_code, EXCEPTION_POINTERS *ep,
                       int (__cdecl *handler)(_FPIEEE_RECORD*) )
{
    FIXME("(%lx %p %p) opcode: %#I64x\n", exception_code, ep, handler,
            *(ULONG64*)ep->ContextRecord->Rip);
    return EXCEPTION_CONTINUE_SEARCH;
}

#if _MSVCR_VER>=110 && _MSVCR_VER<=120
/*********************************************************************
 *  __crtCapturePreviousContext (MSVCR110.@)
 */
void __cdecl get_prev_context(CONTEXT *ctx, DWORD64 rip)
{
    ULONG64 frame, image_base;
    RUNTIME_FUNCTION *rf;
    void *data;

    TRACE("(%p)\n", ctx);

    rf = RtlLookupFunctionEntry(ctx->Rip, &image_base, NULL);
    if(!rf) {
        FIXME("RtlLookupFunctionEntry failed\n");
        return;
    }

    RtlVirtualUnwind(UNW_FLAG_NHANDLER, image_base, ctx->Rip,
            rf, ctx, &data, &frame, NULL);
}

__ASM_GLOBAL_FUNC( __crtCapturePreviousContext,
                   "movq %rcx,8(%rsp)\n\t"
                   "call " __ASM_NAME("RtlCaptureContext") "\n\t"
                   "movq 8(%rsp),%rcx\n\t"     /* context */
                   "leaq 8(%rsp),%rax\n\t"
                   "movq %rax,0x98(%rcx)\n\t"  /* context->Rsp */
                   "movq (%rsp),%rax\n\t"
                   "movq %rax,0xf8(%rcx)\n\t"  /* context->Rip */
                   "jmp " __ASM_NAME("get_prev_context") )
#endif

#endif  /* __x86_64__ */
