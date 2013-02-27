/*
 * msvcrt C++ exception handling
 *
 * Copyright 2011 Alexandre Julliard
 * Copyright 2013 André Hentschel
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

#ifdef __arm__

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

typedef LONG (WINAPI *PC_LANGUAGE_EXCEPTION_HANDLER)(EXCEPTION_POINTERS *ptrs, DWORD frame);
typedef EXCEPTION_DISPOSITION (WINAPI *PEXCEPTION_ROUTINE)(EXCEPTION_RECORD *rec, DWORD frame,
                                                           CONTEXT *context,
                                                           struct _DISPATCHER_CONTEXT *dispatch);

typedef struct _DISPATCHER_CONTEXT
{
    DWORD                 ControlPc;
    DWORD                 ImageBase;
    PRUNTIME_FUNCTION     FunctionEntry;
    DWORD                 EstablisherFrame;
    DWORD                 TargetPc;
    PCONTEXT              ContextRecord;
    PEXCEPTION_ROUTINE    LanguageHandler;
    PVOID                 HandlerData;
    PUNWIND_HISTORY_TABLE HistoryTable;
    DWORD                 ScopeIndex;
    BOOLEAN               ControlPcIsUnwound;
    PBYTE                 NonVolatileRegisters;
    DWORD                 VirtualVfpHead;
} DISPATCHER_CONTEXT;


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
                  "mov r1, #0\n\t"  /* frame */
                  "b " __ASM_NAME("MSVCRT__setjmpex"));

/*******************************************************************
 *		_setjmpex (MSVCRT.@)
 */
__ASM_GLOBAL_FUNC(MSVCRT__setjmpex,
                  "str r1, [r0]\n\t"              /* jmp_buf->Frame */
                  "str r4, [r0, #0x4]\n\t"        /* jmp_buf->R4 */
                  "str r5, [r0, #0x8]\n\t"        /* jmp_buf->R5 */
                  "str r6, [r0, #0xc]\n\t"        /* jmp_buf->R6 */
                  "str r7, [r0, #0x10]\n\t"       /* jmp_buf->R7 */
                  "str r8, [r0, #0x14]\n\t"       /* jmp_buf->R8 */
                  "str r9, [r0, #0x18]\n\t"       /* jmp_buf->R9 */
                  "str r10, [r0, #0x1c]\n\t"      /* jmp_buf->R10 */
                  "str r11, [r0, #0x20]\n\t"      /* jmp_buf->R11 */
                  "str sp, [r0, #0x24]\n\t"       /* jmp_buf->Sp */
                  "str lr, [r0, #0x28]\n\t"       /* jmp_buf->Pc */
                  /* FIXME: Save floating point data */
                  "mov r0, #0\n\t"
                  "bx lr");


extern void DECLSPEC_NORETURN CDECL longjmp_set_regs(struct MSVCRT___JUMP_BUFFER *jmp, int retval);
__ASM_GLOBAL_FUNC(longjmp_set_regs,
                  "ldr r4, [r0, #0x4]\n\t"        /* jmp_buf->R4 */
                  "ldr r5, [r0, #0x8]\n\t"        /* jmp_buf->R5 */
                  "ldr r6, [r0, #0xc]\n\t"        /* jmp_buf->R6 */
                  "ldr r7, [r0, #0x10]\n\t"       /* jmp_buf->R7 */
                  "ldr r8, [r0, #0x14]\n\t"       /* jmp_buf->R8 */
                  "ldr r9, [r0, #0x18]\n\t"       /* jmp_buf->R9 */
                  "ldr r10, [r0, #0x1c]\n\t"      /* jmp_buf->R10 */
                  "ldr r11, [r0, #0x20]\n\t"      /* jmp_buf->R11 */
                  "ldr sp, [r0, #0x24]\n\t"       /* jmp_buf->Sp */
                  "ldr r2, [r0, #0x28]\n\t"       /* jmp_buf->Pc */
                  /* FIXME: Restore floating point data */
                  "mov r0, r1\n\t"                /* retval */
                  "bx r2");

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
        RtlUnwind((void *)jmp->Frame, (void *)jmp->Pc, &rec, IntToPtr(retval));
    }
    longjmp_set_regs(jmp, retval);
}

#endif  /* __arm__ */
