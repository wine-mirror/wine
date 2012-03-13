/*
 * msvcrt.dll exception handling
 *
 * Copyright 2000 Jon Griffiths
 * Copyright 2005 Juan Lang
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
 *
 * FIXME: Incomplete support for nested exceptions/try block cleanup.
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wine/exception.h"
#include "msvcrt.h"
#include "excpt.h"
#include "wincon.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);

static MSVCRT_security_error_handler security_error_handler;

#ifdef __x86_64__

/*******************************************************************
 *		_setjmp (MSVCRT.@)
 */
__ASM_GLOBAL_FUNC( MSVCRT__setjmp,
                   "xorq %rdx,%rdx\n\t"  /* frame */
                   "jmp " __ASM_NAME("MSVCRT__setjmpex") );

/*******************************************************************
 *		_setjmpex (MSVCRT.@)
 */
__ASM_GLOBAL_FUNC( MSVCRT__setjmpex,
                   "movq %rdx,(%rcx)\n\t"          /* jmp_buf->Frame */
                   "movq %rbx,0x8(%rcx)\n\t"       /* jmp_buf->Rbx */
                   "leaq 0x8(%rsp),%rax\n\t"
                   "movq %rax,0x10(%rcx)\n\t"      /* jmp_buf->Rsp */
                   "movq %rbp,0x18(%rcx)\n\t"      /* jmp_buf->Rbp */
                   "movq %rsi,0x20(%rcx)\n\t"      /* jmp_buf->Rsi */
                   "movq %rdi,0x28(%rcx)\n\t"      /* jmp_buf->Rdi */
                   "movq %r12,0x30(%rcx)\n\t"      /* jmp_buf->R12 */
                   "movq %r13,0x38(%rcx)\n\t"      /* jmp_buf->R13 */
                   "movq %r14,0x40(%rcx)\n\t"      /* jmp_buf->R14 */
                   "movq %r15,0x48(%rcx)\n\t"      /* jmp_buf->R15 */
                   "movq (%rsp),%rax\n\t"
                   "movq %rax,0x50(%rcx)\n\t"      /* jmp_buf->Rip */
                   "movdqa %xmm6,0x60(%rcx)\n\t"   /* jmp_buf->Xmm6 */
                   "movdqa %xmm7,0x70(%rcx)\n\t"   /* jmp_buf->Xmm7 */
                   "movdqa %xmm8,0x80(%rcx)\n\t"   /* jmp_buf->Xmm8 */
                   "movdqa %xmm9,0x90(%rcx)\n\t"   /* jmp_buf->Xmm9 */
                   "movdqa %xmm10,0xa0(%rcx)\n\t"  /* jmp_buf->Xmm10 */
                   "movdqa %xmm11,0xb0(%rcx)\n\t"  /* jmp_buf->Xmm11 */
                   "movdqa %xmm12,0xc0(%rcx)\n\t"  /* jmp_buf->Xmm12 */
                   "movdqa %xmm13,0xd0(%rcx)\n\t"  /* jmp_buf->Xmm13 */
                   "movdqa %xmm14,0xe0(%rcx)\n\t"  /* jmp_buf->Xmm14 */
                   "movdqa %xmm15,0xf0(%rcx)\n\t"  /* jmp_buf->Xmm15 */
                   "xorq %rax,%rax\n\t"
                   "retq" );


extern void DECLSPEC_NORETURN CDECL longjmp_set_regs( struct MSVCRT___JUMP_BUFFER *jmp, int retval );
__ASM_GLOBAL_FUNC( longjmp_set_regs,
                   "movq %rdx,%rax\n\t"            /* retval */
                   "movq 0x8(%rcx),%rbx\n\t"       /* jmp_buf->Rbx */
                   "movq 0x18(%rcx),%rbp\n\t"      /* jmp_buf->Rbp */
                   "movq 0x20(%rcx),%rsi\n\t"      /* jmp_buf->Rsi */
                   "movq 0x28(%rcx),%rdi\n\t"      /* jmp_buf->Rdi */
                   "movq 0x30(%rcx),%r12\n\t"      /* jmp_buf->R12 */
                   "movq 0x38(%rcx),%r13\n\t"      /* jmp_buf->R13 */
                   "movq 0x40(%rcx),%r14\n\t"      /* jmp_buf->R14 */
                   "movq 0x48(%rcx),%r15\n\t"      /* jmp_buf->R15 */
                   "movdqa 0x60(%rcx),%xmm6\n\t"   /* jmp_buf->Xmm6 */
                   "movdqa 0x70(%rcx),%xmm7\n\t"   /* jmp_buf->Xmm7 */
                   "movdqa 0x80(%rcx),%xmm8\n\t"   /* jmp_buf->Xmm8 */
                   "movdqa 0x90(%rcx),%xmm9\n\t"   /* jmp_buf->Xmm9 */
                   "movdqa 0xa0(%rcx),%xmm10\n\t"  /* jmp_buf->Xmm10 */
                   "movdqa 0xb0(%rcx),%xmm11\n\t"  /* jmp_buf->Xmm11 */
                   "movdqa 0xc0(%rcx),%xmm12\n\t"  /* jmp_buf->Xmm12 */
                   "movdqa 0xd0(%rcx),%xmm13\n\t"  /* jmp_buf->Xmm13 */
                   "movdqa 0xe0(%rcx),%xmm14\n\t"  /* jmp_buf->Xmm14 */
                   "movdqa 0xf0(%rcx),%xmm15\n\t"  /* jmp_buf->Xmm15 */
                   "movq 0x50(%rcx),%rdx\n\t"      /* jmp_buf->Rip */
                   "movq 0x10(%rcx),%rsp\n\t"      /* jmp_buf->Rsp */
                   "jmp *%rdx" );

/*******************************************************************
 *		longjmp (MSVCRT.@)
 */
void __cdecl MSVCRT_longjmp( struct MSVCRT___JUMP_BUFFER *jmp, int retval )
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
    longjmp_set_regs( jmp, retval );
}

/*******************************************************************
 *		_local_unwind (MSVCRT.@)
 */
void __cdecl _local_unwind( void *frame, void *target )
{
    CONTEXT context;
    RtlUnwindEx( frame, target, NULL, 0, &context, NULL );
}

#endif /* __x86_64__ */

static MSVCRT___sighandler_t sighandlers[MSVCRT_NSIG] = { MSVCRT_SIG_DFL };

static BOOL WINAPI msvcrt_console_handler(DWORD ctrlType)
{
    BOOL ret = FALSE;

    switch (ctrlType)
    {
    case CTRL_C_EVENT:
        if (sighandlers[MSVCRT_SIGINT])
        {
            if (sighandlers[MSVCRT_SIGINT] != MSVCRT_SIG_IGN)
                sighandlers[MSVCRT_SIGINT](MSVCRT_SIGINT);
            ret = TRUE;
        }
        break;
    }
    return ret;
}

typedef void (CDECL *float_handler)(int, int);

/* The exception codes are actually NTSTATUS values */
static const struct
{
    NTSTATUS status;
    int signal;
} float_exception_map[] = {
 { EXCEPTION_FLT_DENORMAL_OPERAND, MSVCRT__FPE_DENORMAL },
 { EXCEPTION_FLT_DIVIDE_BY_ZERO, MSVCRT__FPE_ZERODIVIDE },
 { EXCEPTION_FLT_INEXACT_RESULT, MSVCRT__FPE_INEXACT },
 { EXCEPTION_FLT_INVALID_OPERATION, MSVCRT__FPE_INVALID },
 { EXCEPTION_FLT_OVERFLOW, MSVCRT__FPE_OVERFLOW },
 { EXCEPTION_FLT_STACK_CHECK, MSVCRT__FPE_STACKOVERFLOW },
 { EXCEPTION_FLT_UNDERFLOW, MSVCRT__FPE_UNDERFLOW },
};

static LONG WINAPI msvcrt_exception_filter(struct _EXCEPTION_POINTERS *except)
{
    LONG ret = EXCEPTION_CONTINUE_SEARCH;
    MSVCRT___sighandler_t handler;

    if (!except || !except->ExceptionRecord)
        return EXCEPTION_CONTINUE_SEARCH;

    switch (except->ExceptionRecord->ExceptionCode)
    {
    case EXCEPTION_ACCESS_VIOLATION:
        if ((handler = sighandlers[MSVCRT_SIGSEGV]) != MSVCRT_SIG_DFL)
        {
            if (handler != MSVCRT_SIG_IGN)
            {
                sighandlers[MSVCRT_SIGSEGV] = MSVCRT_SIG_DFL;
                handler(MSVCRT_SIGSEGV);
            }
            ret = EXCEPTION_CONTINUE_EXECUTION;
        }
        break;
    /* According to msdn,
     * the FPE signal handler takes as a second argument the type of
     * floating point exception.
     */
    case EXCEPTION_FLT_DENORMAL_OPERAND:
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    case EXCEPTION_FLT_INEXACT_RESULT:
    case EXCEPTION_FLT_INVALID_OPERATION:
    case EXCEPTION_FLT_OVERFLOW:
    case EXCEPTION_FLT_STACK_CHECK:
    case EXCEPTION_FLT_UNDERFLOW:
        if ((handler = sighandlers[MSVCRT_SIGFPE]) != MSVCRT_SIG_DFL)
        {
            if (handler != MSVCRT_SIG_IGN)
            {
                unsigned int i;
                int float_signal = MSVCRT__FPE_INVALID;

                sighandlers[MSVCRT_SIGFPE] = MSVCRT_SIG_DFL;
                for (i = 0; i < sizeof(float_exception_map) /
                         sizeof(float_exception_map[0]); i++)
                {
                    if (float_exception_map[i].status ==
                        except->ExceptionRecord->ExceptionCode)
                    {
                        float_signal = float_exception_map[i].signal;
                        break;
                    }
                }
                ((float_handler)handler)(MSVCRT_SIGFPE, float_signal);
            }
            ret = EXCEPTION_CONTINUE_EXECUTION;
        }
        break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
    case EXCEPTION_PRIV_INSTRUCTION:
        if ((handler = sighandlers[MSVCRT_SIGILL]) != MSVCRT_SIG_DFL)
        {
            if (handler != MSVCRT_SIG_IGN)
            {
                sighandlers[MSVCRT_SIGILL] = MSVCRT_SIG_DFL;
                handler(MSVCRT_SIGILL);
            }
            ret = EXCEPTION_CONTINUE_EXECUTION;
        }
        break;
    }
    return ret;
}

void msvcrt_init_signals(void)
{
    SetConsoleCtrlHandler(msvcrt_console_handler, TRUE);
    SetUnhandledExceptionFilter(msvcrt_exception_filter);
}

void msvcrt_free_signals(void)
{
    SetConsoleCtrlHandler(msvcrt_console_handler, FALSE);
    SetUnhandledExceptionFilter(NULL);
}

/*********************************************************************
 *		signal (MSVCRT.@)
 * Some signals may never be generated except through an explicit call to
 * raise.
 */
MSVCRT___sighandler_t CDECL MSVCRT_signal(int sig, MSVCRT___sighandler_t func)
{
    MSVCRT___sighandler_t ret = MSVCRT_SIG_ERR;

    TRACE("(%d, %p)\n", sig, func);

    if (func == MSVCRT_SIG_ERR) return MSVCRT_SIG_ERR;

    switch (sig)
    {
    /* Cases handled internally.  Note SIGTERM is never generated by Windows,
     * so we effectively mask it.
     */
    case MSVCRT_SIGABRT:
    case MSVCRT_SIGFPE:
    case MSVCRT_SIGILL:
    case MSVCRT_SIGSEGV:
    case MSVCRT_SIGINT:
    case MSVCRT_SIGTERM:
    case MSVCRT_SIGBREAK:
        ret = sighandlers[sig];
        sighandlers[sig] = func;
        break;
    default:
        ret = MSVCRT_SIG_ERR;
    }
    return ret;
}

/*********************************************************************
 *		raise (MSVCRT.@)
 */
int CDECL MSVCRT_raise(int sig)
{
    MSVCRT___sighandler_t handler;

    TRACE("(%d)\n", sig);

    switch (sig)
    {
    case MSVCRT_SIGABRT:
    case MSVCRT_SIGFPE:
    case MSVCRT_SIGILL:
    case MSVCRT_SIGSEGV:
    case MSVCRT_SIGINT:
    case MSVCRT_SIGTERM:
    case MSVCRT_SIGBREAK:
        handler = sighandlers[sig];
        if (handler == MSVCRT_SIG_DFL) MSVCRT__exit(3);
        if (handler != MSVCRT_SIG_IGN)
        {
            sighandlers[sig] = MSVCRT_SIG_DFL;
            if (sig == MSVCRT_SIGFPE)
                ((float_handler)handler)(sig, MSVCRT__FPE_EXPLICITGEN);
            else
                handler(sig);
        }
        break;
    default:
        return -1;
    }
    return 0;
}

/*********************************************************************
 *		_XcptFilter (MSVCRT.@)
 */
int CDECL _XcptFilter(NTSTATUS ex, PEXCEPTION_POINTERS ptr)
{
    TRACE("(%08x,%p)\n", ex, ptr);
    /* I assume ptr->ExceptionRecord->ExceptionCode is the same as ex */
    return msvcrt_exception_filter(ptr);
}

/*********************************************************************
 *		_abnormal_termination (MSVCRT.@)
 */
int CDECL _abnormal_termination(void)
{
  FIXME("(void)stub\n");
  return 0;
}

/******************************************************************
 *		MSVCRT___uncaught_exception
 */
BOOL CDECL MSVCRT___uncaught_exception(void)
{
    return FALSE;
}

/* _set_security_error_handler - not exported in native msvcrt, added in msvcr70 */
MSVCRT_security_error_handler CDECL _set_security_error_handler(
    MSVCRT_security_error_handler handler )
{
    MSVCRT_security_error_handler old = security_error_handler;

    TRACE("(%p)\n", handler);

    security_error_handler = handler;
    return old;
}
