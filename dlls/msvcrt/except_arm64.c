/*
 * msvcrt C++ exception handling
 *
 * Copyright 2011 Alexandre Julliard
 * Copyright 2013 André Hentschel
 * Copyright 2017 Martin Storsjo
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
#include "wine/port.h"

#ifdef __aarch64__

#include <stdarg.h>

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

struct _DISPATCHER_CONTEXT;

typedef LONG (WINAPI *PC_LANGUAGE_EXCEPTION_HANDLER)( EXCEPTION_POINTERS *ptrs, ULONG64 frame );
typedef EXCEPTION_DISPOSITION (WINAPI *PEXCEPTION_ROUTINE)( EXCEPTION_RECORD *rec,
                                                            ULONG64 frame,
                                                            CONTEXT *context,
                                                            struct _DISPATCHER_CONTEXT *dispatch );

typedef struct _DISPATCHER_CONTEXT
{
    DWORD64               ControlPc;
    DWORD64               ImageBase;
    PRUNTIME_FUNCTION     FunctionEntry;
    DWORD64               EstablisherFrame;
    DWORD64               TargetPc;
    PCONTEXT              ContextRecord;
    PEXCEPTION_ROUTINE    LanguageHandler;
    PVOID                 HandlerData;
    PUNWIND_HISTORY_TABLE HistoryTable;
    DWORD                 ScopeIndex;
    BOOLEAN               ControlPcIsUnwound;
    PBYTE                 NonVolatileRegisters;
} DISPATCHER_CONTEXT;

/*********************************************************************
 *		__CxxExceptionFilter (MSVCRT.@)
 */
int CDECL __CxxExceptionFilter( PEXCEPTION_POINTERS ptrs,
                                const type_info *ti, int flags, void **copy )
{
    FIXME( "%p %p %x %p: not implemented\n", ptrs, ti, flags, copy );
    return EXCEPTION_CONTINUE_SEARCH;
}

/*********************************************************************
 *		__CxxFrameHandler (MSVCRT.@)
 */
EXCEPTION_DISPOSITION CDECL __CxxFrameHandler(EXCEPTION_RECORD *rec, DWORD frame, CONTEXT *context,
                                              DISPATCHER_CONTEXT *dispatch)
{
    FIXME("%p %x %p %p: not implemented\n", rec, frame, context, dispatch);
    return ExceptionContinueSearch;
}


/*********************************************************************
 *		__CppXcptFilter (MSVCRT.@)
 */
int CDECL __CppXcptFilter(NTSTATUS ex, PEXCEPTION_POINTERS ptr)
{
    /* only filter c++ exceptions */
    if (ex != CXX_EXCEPTION) return EXCEPTION_CONTINUE_SEARCH;
    return _XcptFilter(ex, ptr);
}


/*********************************************************************
 *		__CxxDetectRethrow (MSVCRT.@)
 */
BOOL CDECL __CxxDetectRethrow(PEXCEPTION_POINTERS ptrs)
{
    PEXCEPTION_RECORD rec;

    if (!ptrs)
        return FALSE;

    rec = ptrs->ExceptionRecord;

    if (rec->ExceptionCode == CXX_EXCEPTION &&
        rec->NumberParameters == 3 &&
        rec->ExceptionInformation[0] == CXX_FRAME_MAGIC_VC6 &&
        rec->ExceptionInformation[2])
    {
        ptrs->ExceptionRecord = msvcrt_get_thread_data()->exc_record;
        return TRUE;
    }
    return (msvcrt_get_thread_data()->exc_record == rec);
}


/*********************************************************************
 *		__CxxQueryExceptionSize (MSVCRT.@)
 */
unsigned int CDECL __CxxQueryExceptionSize(void)
{
    return sizeof(cxx_exception_type);
}


/*******************************************************************
 *		_setjmp (MSVCRT.@)
 */
__ASM_GLOBAL_FUNC(MSVCRT__setjmp,
                  "mov x1, #0\n\t"  /* frame */
                  "b " __ASM_NAME("MSVCRT__setjmpex"));

/*******************************************************************
 *		_setjmpex (MSVCRT.@)
 */
__ASM_GLOBAL_FUNC(MSVCRT__setjmpex,
                  "str x1,       [x0]\n\t"        /* jmp_buf->Frame */
                  "stp x19, x20, [x0, #0x10]\n\t" /* jmp_buf->X19, X20 */
                  "stp x21, x22, [x0, #0x20]\n\t" /* jmp_buf->X21, X22 */
                  "stp x23, x24, [x0, #0x30]\n\t" /* jmp_buf->X23, X24 */
                  "stp x25, x26, [x0, #0x40]\n\t" /* jmp_buf->X25, X26 */
                  "stp x27, x28, [x0, #0x50]\n\t" /* jmp_buf->X27, X28 */
                  "stp x29, x30, [x0, #0x60]\n\t" /* jmp_buf->Fp,  Lr  */
                  "mov x2,  sp\n\t"
                  "str x2,       [x0, #0x70]\n\t" /* jmp_buf->Sp */
                  "mrs x2,  fpcr\n\t"
                  "str w2,       [x0, #0x78]\n\t" /* jmp_buf->Fpcr */
                  "mrs x2,  fpsr\n\t"
                  "str w2,       [x0, #0x7c]\n\t" /* jmp_buf->Fpsr */
                  "stp d8,  d9,  [x0, #0x80]\n\t" /* jmp_buf->D[0-1] */
                  "stp d10, d11, [x0, #0x90]\n\t" /* jmp_buf->D[2-3] */
                  "stp d12, d13, [x0, #0xa0]\n\t" /* jmp_buf->D[4-5] */
                  "stp d14, d15, [x0, #0xb0]\n\t" /* jmp_buf->D[6-7] */
                  "mov x0, #0\n\t"
                  "ret");


extern void DECLSPEC_NORETURN CDECL longjmp_set_regs(struct MSVCRT___JUMP_BUFFER *jmp, int retval);
__ASM_GLOBAL_FUNC(longjmp_set_regs,
                  "ldp x19, x20, [x0, #0x10]\n\t" /* jmp_buf->X19, X20 */
                  "ldp x21, x22, [x0, #0x20]\n\t" /* jmp_buf->X21, X22 */
                  "ldp x23, x24, [x0, #0x30]\n\t" /* jmp_buf->X23, X24 */
                  "ldp x25, x26, [x0, #0x40]\n\t" /* jmp_buf->X25, X26 */
                  "ldp x27, x28, [x0, #0x50]\n\t" /* jmp_buf->X27, X28 */
                  "ldp x29, x30, [x0, #0x60]\n\t" /* jmp_buf->Fp,  Lr  */
                  "ldr x2,       [x0, #0x70]\n\t" /* jmp_buf->Sp */
                  "mov sp,  x2\n\t"
                  "ldr w2,       [x0, #0x78]\n\t" /* jmp_buf->Fpcr */
                  "msr fpcr, x2\n\t"
                  "ldr w2,       [x0, #0x7c]\n\t" /* jmp_buf->Fpsr */
                  "msr fpsr, x2\n\t"
                  "ldp d8,  d9,  [x0, #0x80]\n\t" /* jmp_buf->D[0-1] */
                  "ldp d10, d11, [x0, #0x90]\n\t" /* jmp_buf->D[2-3] */
                  "ldp d12, d13, [x0, #0xa0]\n\t" /* jmp_buf->D[4-5] */
                  "ldp d14, d15, [x0, #0xb0]\n\t" /* jmp_buf->D[6-7] */
                  "mov x0, x1\n\t"                /* retval */
                  "ret");

/*******************************************************************
 *		longjmp (MSVCRT.@)
 */
void __cdecl MSVCRT_longjmp(struct MSVCRT___JUMP_BUFFER *jmp, int retval)
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
        RtlUnwind((void *)jmp->Frame, (void *)jmp->Lr, &rec, IntToPtr(retval));
    }
    longjmp_set_regs(jmp, retval);
}

/*********************************************************************
 *              _fpieee_flt (MSVCRT.@)
 */
int __cdecl _fpieee_flt(ULONG exception_code, EXCEPTION_POINTERS *ep,
        int (__cdecl *handler)(_FPIEEE_RECORD*))
{
    FIXME("(%x %p %p)\n", exception_code, ep, handler);
    return EXCEPTION_CONTINUE_SEARCH;
}

#endif  /* __aarch64__ */
