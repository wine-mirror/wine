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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * NOTES:
 *
 * See http://www.microsoft.com/msj/0197/exception/exception.htm,
 * but don't believe all of it.
 *
 * FIXME: Incomplete support for nested exceptions/try block cleanup.
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"
#include "wine/exception.h"
#include "msvcrt.h"
#include "excpt.h"
#include "wincon.h"
#include "msvcrt/float.h"
#include "msvcrt/signal.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/* VC++ extensions to Win32 SEH */
typedef struct _SCOPETABLE
{
  int previousTryLevel;
  int (*lpfnFilter)(PEXCEPTION_POINTERS);
  int (*lpfnHandler)(void);
} SCOPETABLE, *PSCOPETABLE;

typedef struct _MSVCRT_EXCEPTION_FRAME
{
  EXCEPTION_REGISTRATION_RECORD *prev;
  void (*handler)(PEXCEPTION_RECORD, EXCEPTION_REGISTRATION_RECORD*,
                  PCONTEXT, PEXCEPTION_RECORD);
  PSCOPETABLE scopetable;
  int trylevel;
  int _ebp;
  PEXCEPTION_POINTERS xpointers;
} MSVCRT_EXCEPTION_FRAME;

#define TRYLEVEL_END (-1) /* End of trylevel list */

#if defined(__GNUC__) && defined(__i386__)
inline static void call_finally_block( void *code_block, void *base_ptr )
{
    __asm__ __volatile__ ("movl %1,%%ebp; call *%%eax" \
                          : : "a" (code_block), "g" (base_ptr));
}

inline static DWORD call_filter( void *func, void *arg, void *ebp )
{
    DWORD ret;
    __asm__ __volatile__ ("pushl %%ebp; pushl %3; movl %2,%%ebp; call *%%eax; popl %%ebp; popl %%ebp"
                          : "=a" (ret)
                          : "0" (func), "r" (ebp), "r" (arg)
                          : "ecx", "edx", "memory" );
    return ret;
}
#endif

static DWORD MSVCRT_nested_handler(PEXCEPTION_RECORD rec,
                                   EXCEPTION_REGISTRATION_RECORD* frame,
                                   PCONTEXT context,
                                   EXCEPTION_REGISTRATION_RECORD** dispatch)
{
  if (rec->ExceptionFlags & 0x6)
    return ExceptionContinueSearch;
  *dispatch = frame;
  return ExceptionCollidedUnwind;
}


/*********************************************************************
 *		_XcptFilter (MSVCRT.@)
 */
int _XcptFilter(int ex, PEXCEPTION_POINTERS ptr)
{
  FIXME("(%d,%p)semi-stub\n", ex, ptr);
  return UnhandledExceptionFilter(ptr);
}

/*********************************************************************
 *		_EH_prolog (MSVCRT.@)
 */
#ifdef __i386__
/* Provided for VC++ binary compatibility only */
__ASM_GLOBAL_FUNC(_EH_prolog,
                  "pushl $-1\n\t"
                  "pushl %eax\n\t"
                  "pushl %fs:0\n\t"
                  "movl  %esp, %fs:0\n\t"
                  "movl  12(%esp), %eax\n\t"
                  "movl  %ebp, 12(%esp)\n\t"
                  "leal  12(%esp), %ebp\n\t"
                  "pushl %eax\n\t"
                  "ret");
#endif

/*******************************************************************
 *		_global_unwind2 (MSVCRT.@)
 */
void _global_unwind2(EXCEPTION_REGISTRATION_RECORD* frame)
{
    TRACE("(%p)\n",frame);
    RtlUnwind( frame, 0, 0, 0 );
}

/*******************************************************************
 *		_local_unwind2 (MSVCRT.@)
 */
void _local_unwind2(MSVCRT_EXCEPTION_FRAME* frame, int trylevel)
{
  MSVCRT_EXCEPTION_FRAME *curframe = frame;
  EXCEPTION_REGISTRATION_RECORD reg;

  TRACE("(%p,%d,%d)\n",frame, frame->trylevel, trylevel);

  /* Register a handler in case of a nested exception */
  reg.Handler = (PEXCEPTION_HANDLER)MSVCRT_nested_handler;
  reg.Prev = NtCurrentTeb()->Tib.ExceptionList;
  __wine_push_frame(&reg);

  while (frame->trylevel != TRYLEVEL_END && frame->trylevel != trylevel)
  {
    int curtrylevel = frame->scopetable[frame->trylevel].previousTryLevel;
    curframe = frame;
    curframe->trylevel = curtrylevel;
    if (!frame->scopetable[curtrylevel].lpfnFilter)
    {
      ERR("__try block cleanup not implemented - expect crash!\n");
      /* FIXME: Remove current frame, set ebp, call
       * frame->scopetable[curtrylevel].lpfnHandler()
       */
    }
  }
  __wine_pop_frame(&reg);
  TRACE("unwound OK\n");
}

/*********************************************************************
 *		_except_handler2 (MSVCRT.@)
 */
int _except_handler2(PEXCEPTION_RECORD rec,
                     EXCEPTION_REGISTRATION_RECORD* frame,
                     PCONTEXT context,
                     EXCEPTION_REGISTRATION_RECORD** dispatcher)
{
  FIXME("exception %lx flags=%lx at %p handler=%p %p %p stub\n",
        rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress,
        frame->Handler, context, dispatcher);
  return ExceptionContinueSearch;
}

/*********************************************************************
 *		_except_handler3 (MSVCRT.@)
 */
int _except_handler3(PEXCEPTION_RECORD rec,
                     MSVCRT_EXCEPTION_FRAME* frame,
                     PCONTEXT context, void* dispatcher)
{
#if defined(__GNUC__) && defined(__i386__)
  long retval;
  int trylevel;
  EXCEPTION_POINTERS exceptPtrs;
  PSCOPETABLE pScopeTable;

  TRACE("exception %lx flags=%lx at %p handler=%p %p %p semi-stub\n",
        rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress,
        frame->handler, context, dispatcher);

  __asm__ __volatile__ ("cld");

  if (rec->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
  {
    /* Unwinding the current frame */
     _local_unwind2(frame, TRYLEVEL_END);
    return ExceptionContinueSearch;
  }
  else
  {
    /* Hunting for handler */
    exceptPtrs.ExceptionRecord = rec;
    exceptPtrs.ContextRecord = context;
    *((DWORD *)frame-1) = (DWORD)&exceptPtrs;
    trylevel = frame->trylevel;
    pScopeTable = frame->scopetable;

    while (trylevel != TRYLEVEL_END)
    {
      if (pScopeTable[trylevel].lpfnFilter)
      {
        TRACE("filter = %p\n", pScopeTable[trylevel].lpfnFilter);

        retval = call_filter( pScopeTable[trylevel].lpfnFilter, &exceptPtrs, &frame->_ebp );

        TRACE("filter returned %s\n", retval == EXCEPTION_CONTINUE_EXECUTION ?
              "CONTINUE_EXECUTION" : retval == EXCEPTION_EXECUTE_HANDLER ?
              "EXECUTE_HANDLER" : "CONTINUE_SEARCH");

        if (retval == EXCEPTION_CONTINUE_EXECUTION)
          return ExceptionContinueExecution;

        if (retval == EXCEPTION_EXECUTE_HANDLER)
        {
          /* Unwind all higher frames, this one will handle the exception */
          _global_unwind2((EXCEPTION_REGISTRATION_RECORD*)frame);
          _local_unwind2(frame, trylevel);

          /* Set our trylevel to the enclosing block, and call the __finally
           * code, which won't return
           */
          frame->trylevel = pScopeTable->previousTryLevel;
          TRACE("__finally block %p\n",pScopeTable[trylevel].lpfnHandler);
          call_finally_block(pScopeTable[trylevel].lpfnHandler, &frame->_ebp);
          ERR("Returned from __finally block - expect crash!\n");
       }
      }
      trylevel = pScopeTable->previousTryLevel;
    }
  }
#else
  TRACE("exception %lx flags=%lx at %p handler=%p %p %p stub\n",
        rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress,
        frame->handler, context, dispatcher);
#endif
  return ExceptionContinueSearch;
}

/*********************************************************************
 *		_abnormal_termination (MSVCRT.@)
 */
int _abnormal_termination(void)
{
  FIXME("(void)stub\n");
  return 0;
}

/*
 * setjmp/longjmp implementation
 */

#ifdef __i386__
#define MSVCRT_JMP_MAGIC 0x56433230 /* ID value for new jump structure */
typedef void (*MSVCRT_unwind_function)(const void*);

/*
 * The signatures of the setjmp/longjmp functions do not match that
 * declared in the setjmp header so they don't follow the regular naming
 * convention to avoid conflicts.
 */

/*******************************************************************
 *		_setjmp (MSVCRT.@)
 */
DEFINE_REGS_ENTRYPOINT( MSVCRT__setjmp, 4, 0 );
void WINAPI __regs_MSVCRT__setjmp(struct MSVCRT___JUMP_BUFFER *jmp, CONTEXT86* context)
{
    TRACE("(%p)\n",jmp);
    jmp->Ebp = context->Ebp;
    jmp->Ebx = context->Ebx;
    jmp->Edi = context->Edi;
    jmp->Esi = context->Esi;
    jmp->Esp = context->Esp;
    jmp->Eip = context->Eip;
    jmp->Registration = (unsigned long)NtCurrentTeb()->Tib.ExceptionList;
    if (jmp->Registration == TRYLEVEL_END)
        jmp->TryLevel = TRYLEVEL_END;
    else
        jmp->TryLevel = ((MSVCRT_EXCEPTION_FRAME*)jmp->Registration)->trylevel;
    TRACE("returning 0\n");
    context->Eax=0;
}

/*******************************************************************
 *		_setjmp3 (MSVCRT.@)
 */
DEFINE_REGS_ENTRYPOINT( MSVCRT__setjmp3, 8, 0 );
void WINAPI __regs_MSVCRT__setjmp3(struct MSVCRT___JUMP_BUFFER *jmp, int nb_args, CONTEXT86* context)
{
    TRACE("(%p,%d)\n",jmp,nb_args);
    jmp->Ebp = context->Ebp;
    jmp->Ebx = context->Ebx;
    jmp->Edi = context->Edi;
    jmp->Esi = context->Esi;
    jmp->Esp = context->Esp;
    jmp->Eip = context->Eip;
    jmp->Cookie = MSVCRT_JMP_MAGIC;
    jmp->UnwindFunc = 0;
    jmp->Registration = (unsigned long)NtCurrentTeb()->Tib.ExceptionList;
    if (jmp->Registration == TRYLEVEL_END)
    {
        jmp->TryLevel = TRYLEVEL_END;
    }
    else
    {
        void **args = ((void**)context->Esp)+2;

        if (nb_args > 0) jmp->UnwindFunc = (unsigned long)*args++;
        if (nb_args > 1) jmp->TryLevel = (unsigned long)*args++;
        else jmp->TryLevel = ((MSVCRT_EXCEPTION_FRAME*)jmp->Registration)->trylevel;
        if (nb_args > 2)
        {
            size_t size = (nb_args - 2) * sizeof(DWORD);
            memcpy( jmp->UnwindData, args, min( size, sizeof(jmp->UnwindData) ));
        }
    }
    TRACE("returning 0\n");
    context->Eax = 0;
}

/*********************************************************************
 *		longjmp (MSVCRT.@)
 */
DEFINE_REGS_ENTRYPOINT( MSVCRT_longjmp, 8, 0 );
void WINAPI __regs_MSVCRT_longjmp(struct MSVCRT___JUMP_BUFFER *jmp, int retval, CONTEXT86* context)
{
    unsigned long cur_frame = 0;

    TRACE("(%p,%d)\n", jmp, retval);

    cur_frame=(unsigned long)NtCurrentTeb()->Tib.ExceptionList;
    TRACE("cur_frame=%lx\n",cur_frame);

    if (cur_frame != jmp->Registration)
        _global_unwind2((EXCEPTION_REGISTRATION_RECORD*)jmp->Registration);

    if (jmp->Registration)
    {
        if (!IsBadReadPtr(&jmp->Cookie, sizeof(long)) &&
            jmp->Cookie == MSVCRT_JMP_MAGIC && jmp->UnwindFunc)
        {
            MSVCRT_unwind_function unwind_func;

            unwind_func=(MSVCRT_unwind_function)jmp->UnwindFunc;
            unwind_func(jmp);
        }
        else
            _local_unwind2((MSVCRT_EXCEPTION_FRAME*)jmp->Registration,
                           jmp->TryLevel);
    }

    if (!retval)
        retval = 1;

    TRACE("Jump to %lx returning %d\n",jmp->Eip,retval);
    context->Ebp = jmp->Ebp;
    context->Ebx = jmp->Ebx;
    context->Edi = jmp->Edi;
    context->Esi = jmp->Esi;
    context->Esp = jmp->Esp;
    context->Eip = jmp->Eip;
    context->Eax = retval;
}

/*********************************************************************
 *		_seh_longjmp_unwind (MSVCRT.@)
 */
void __stdcall _seh_longjmp_unwind(struct MSVCRT___JUMP_BUFFER *jmp)
{
    _local_unwind2( (MSVCRT_EXCEPTION_FRAME *)jmp->Registration, jmp->TryLevel );
}
#endif /* i386 */

static __sighandler_t sighandlers[NSIG] = { SIG_DFL };

static BOOL WINAPI msvcrt_console_handler(DWORD ctrlType)
{
    BOOL ret = FALSE;

    switch (ctrlType)
    {
    case CTRL_C_EVENT:
        if (sighandlers[SIGINT])
        {
            if (sighandlers[SIGINT] != SIG_IGN)
                sighandlers[SIGINT](SIGINT);
            ret = TRUE;
        }
        break;
    }
    return ret;
}

typedef void (*float_handler)(int, int);

/* The exception codes are actually NTSTATUS values */
struct
{
    NTSTATUS status;
    int signal;
} float_exception_map[] = {
 { EXCEPTION_FLT_DENORMAL_OPERAND, _FPE_DENORMAL },
 { EXCEPTION_FLT_DIVIDE_BY_ZERO, _FPE_ZERODIVIDE },
 { EXCEPTION_FLT_INEXACT_RESULT, _FPE_INEXACT },
 { EXCEPTION_FLT_INVALID_OPERATION, _FPE_INVALID },
 { EXCEPTION_FLT_OVERFLOW, _FPE_OVERFLOW },
 { EXCEPTION_FLT_STACK_CHECK, _FPE_STACKOVERFLOW },
 { EXCEPTION_FLT_UNDERFLOW, _FPE_UNDERFLOW },
};

static LONG WINAPI msvcrt_exception_filter(struct _EXCEPTION_POINTERS *except)
{
    LONG ret = EXCEPTION_CONTINUE_SEARCH;

    switch (except->ExceptionRecord->ExceptionCode)
    {
    case EXCEPTION_ACCESS_VIOLATION:
        if (sighandlers[SIGSEGV])
        {
            if (sighandlers[SIGSEGV] != SIG_IGN)
                sighandlers[SIGSEGV](SIGSEGV);
            ret = EXCEPTION_CONTINUE_EXECUTION;
        }
        break;
    /* According to
     * http://msdn.microsoft.com/library/en-us/vclib/html/_CRT_signal.asp
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
        if (sighandlers[SIGFPE])
        {
            if (sighandlers[SIGFPE] != SIG_IGN)
            {
                int i, float_signal = _FPE_INVALID;

                float_handler handler = (float_handler)sighandlers[SIGFPE];
                for (i = 0; i < sizeof(float_exception_map) /
                 sizeof(float_exception_map[0]); i++)
                    if (float_exception_map[i].status ==
                     except->ExceptionRecord->ExceptionCode)
                    {
                        float_signal = float_exception_map[i].signal;
                        break;
                    }
                handler(SIGFPE, float_signal);
            }
            ret = EXCEPTION_CONTINUE_EXECUTION;
        }
        break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        if (sighandlers[SIGILL])
        {
            if (sighandlers[SIGILL] != SIG_IGN)
                sighandlers[SIGILL](SIGILL);
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
 * MS signal handling is described here:
 * http://msdn.microsoft.com/library/en-us/vclib/html/_CRT_signal.asp
 * Some signals may never be generated except through an explicit call to
 * raise.
 */
__sighandler_t MSVCRT_signal(int sig, __sighandler_t func)
{
    __sighandler_t ret = SIG_ERR;

    TRACE("(%d, %p)\n", sig, func);

    if (func == SIG_ERR) return SIG_ERR;

    switch (sig)
    {
    /* Cases handled internally.  Note SIGTERM is never generated by Windows,
     * so we effectively mask it.
     */
    case SIGABRT:
    case SIGFPE:
    case SIGILL:
    case SIGSEGV:
    case SIGINT:
    case SIGTERM:
        ret = sighandlers[sig];
        sighandlers[sig] = func;
        break;
    default:
        ret = SIG_ERR;
    }
    return ret;
}
