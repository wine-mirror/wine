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
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);

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
void CDECL _global_unwind2(EXCEPTION_REGISTRATION_RECORD* frame)
{
    TRACE("(%p)\n",frame);
    RtlUnwind( frame, 0, 0, 0 );
}

/*******************************************************************
 *		_local_unwind2 (MSVCRT.@)
 */
void CDECL _local_unwind2(MSVCRT_EXCEPTION_FRAME* frame, int trylevel)
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
int CDECL _except_handler2(PEXCEPTION_RECORD rec,
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
int CDECL _except_handler3(PEXCEPTION_RECORD rec,
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
    TRACE("unwound current frame, returning ExceptionContinueSearch\n");
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
  TRACE("reached TRYLEVEL_END, returning ExceptionContinueSearch\n");
  return ExceptionContinueSearch;
}

/*********************************************************************
 *		_abnormal_termination (MSVCRT.@)
 */
int CDECL _abnormal_termination(void)
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

/* define an entrypoint for setjmp/setjmp3 that stores the registers in the jmp buf */
/* and then jumps to the C backend function */
#define DEFINE_SETJMP_ENTRYPOINT(name) \
    __ASM_GLOBAL_FUNC( name, \
                       "movl 4(%esp),%ecx\n\t"   /* jmp_buf */      \
                       "movl %ebp,0(%ecx)\n\t"   /* jmp_buf.Ebp */  \
                       "movl %ebx,4(%ecx)\n\t"   /* jmp_buf.Ebx */  \
                       "movl %edi,8(%ecx)\n\t"   /* jmp_buf.Edi */  \
                       "movl %esi,12(%ecx)\n\t"  /* jmp_buf.Esi */  \
                       "movl %esp,16(%ecx)\n\t"  /* jmp_buf.Esp */  \
                       "movl 0(%esp),%eax\n\t"                      \
                       "movl %eax,20(%ecx)\n\t"  /* jmp_buf.Eip */  \
                       "jmp " __ASM_NAME("__regs_") # name )

/* restore the registers from the jmp buf upon longjmp */
extern void DECLSPEC_NORETURN longjmp_set_regs( struct MSVCRT___JUMP_BUFFER *jmp, int retval );
__ASM_GLOBAL_FUNC( longjmp_set_regs,
                   "movl 4(%esp),%ecx\n\t"   /* jmp_buf */
                   "movl 8(%esp),%eax\n\t"   /* retval */
                   "movl 0(%ecx),%ebp\n\t"   /* jmp_buf.Ebp */
                   "movl 4(%ecx),%ebx\n\t"   /* jmp_buf.Ebx */
                   "movl 8(%ecx),%edi\n\t"   /* jmp_buf.Edi */
                   "movl 12(%ecx),%esi\n\t"  /* jmp_buf.Esi */
                   "movl 16(%ecx),%esp\n\t"  /* jmp_buf.Esp */
                   "addl $4,%esp\n\t"        /* get rid of return address */
                   "jmp *20(%ecx)\n\t"       /* jmp_buf.Eip */ );

/*
 * The signatures of the setjmp/longjmp functions do not match that
 * declared in the setjmp header so they don't follow the regular naming
 * convention to avoid conflicts.
 */

/*******************************************************************
 *		_setjmp (MSVCRT.@)
 */
DEFINE_SETJMP_ENTRYPOINT(MSVCRT__setjmp);
int CDECL __regs_MSVCRT__setjmp(struct MSVCRT___JUMP_BUFFER *jmp)
{
    jmp->Registration = (unsigned long)NtCurrentTeb()->Tib.ExceptionList;
    if (jmp->Registration == TRYLEVEL_END)
        jmp->TryLevel = TRYLEVEL_END;
    else
        jmp->TryLevel = ((MSVCRT_EXCEPTION_FRAME*)jmp->Registration)->trylevel;

    TRACE("buf=%p ebx=%08lx esi=%08lx edi=%08lx ebp=%08lx esp=%08lx eip=%08lx frame=%08lx\n",
          jmp, jmp->Ebx, jmp->Esi, jmp->Edi, jmp->Ebp, jmp->Esp, jmp->Eip, jmp->Registration );
    return 0;
}

/*******************************************************************
 *		_setjmp3 (MSVCRT.@)
 */
DEFINE_SETJMP_ENTRYPOINT( MSVCRT__setjmp3 );
int CDECL __regs_MSVCRT__setjmp3(struct MSVCRT___JUMP_BUFFER *jmp, int nb_args)
{
    jmp->Cookie = MSVCRT_JMP_MAGIC;
    jmp->UnwindFunc = 0;
    jmp->Registration = (unsigned long)NtCurrentTeb()->Tib.ExceptionList;
    if (jmp->Registration == TRYLEVEL_END)
    {
        jmp->TryLevel = TRYLEVEL_END;
    }
    else
    {
        void **args = ((void**)jmp->Esp)+2;

        if (nb_args > 0) jmp->UnwindFunc = (unsigned long)*args++;
        if (nb_args > 1) jmp->TryLevel = (unsigned long)*args++;
        else jmp->TryLevel = ((MSVCRT_EXCEPTION_FRAME*)jmp->Registration)->trylevel;
        if (nb_args > 2)
        {
            size_t size = (nb_args - 2) * sizeof(DWORD);
            memcpy( jmp->UnwindData, args, min( size, sizeof(jmp->UnwindData) ));
        }
    }

    TRACE("buf=%p ebx=%08lx esi=%08lx edi=%08lx ebp=%08lx esp=%08lx eip=%08lx frame=%08lx\n",
          jmp, jmp->Ebx, jmp->Esi, jmp->Edi, jmp->Ebp, jmp->Esp, jmp->Eip, jmp->Registration );
    return 0;
}

/*********************************************************************
 *		longjmp (MSVCRT.@)
 */
int CDECL MSVCRT_longjmp(struct MSVCRT___JUMP_BUFFER *jmp, int retval)
{
    unsigned long cur_frame = 0;

    TRACE("buf=%p ebx=%08lx esi=%08lx edi=%08lx ebp=%08lx esp=%08lx eip=%08lx frame=%08lx retval=%08x\n",
          jmp, jmp->Ebx, jmp->Esi, jmp->Edi, jmp->Ebp, jmp->Esp, jmp->Eip, jmp->Registration, retval );

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

    longjmp_set_regs( jmp, retval );
}

/*********************************************************************
 *		_seh_longjmp_unwind (MSVCRT.@)
 */
void __stdcall _seh_longjmp_unwind(struct MSVCRT___JUMP_BUFFER *jmp)
{
    _local_unwind2( (MSVCRT_EXCEPTION_FRAME *)jmp->Registration, jmp->TryLevel );
}
#endif /* i386 */

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

    if (!except || !except->ExceptionRecord)
        return EXCEPTION_CONTINUE_SEARCH;

    switch (except->ExceptionRecord->ExceptionCode)
    {
    case EXCEPTION_ACCESS_VIOLATION:
        if (sighandlers[MSVCRT_SIGSEGV])
        {
            if (sighandlers[MSVCRT_SIGSEGV] != MSVCRT_SIG_IGN)
                sighandlers[MSVCRT_SIGSEGV](MSVCRT_SIGSEGV);
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
        if (sighandlers[MSVCRT_SIGFPE])
        {
            if (sighandlers[MSVCRT_SIGFPE] != MSVCRT_SIG_IGN)
            {
                int i, float_signal = _FPE_INVALID;

                float_handler handler = (float_handler)sighandlers[MSVCRT_SIGFPE];
                for (i = 0; i < sizeof(float_exception_map) /
                 sizeof(float_exception_map[0]); i++)
                    if (float_exception_map[i].status ==
                     except->ExceptionRecord->ExceptionCode)
                    {
                        float_signal = float_exception_map[i].signal;
                        break;
                    }
                handler(MSVCRT_SIGFPE, float_signal);
            }
            ret = EXCEPTION_CONTINUE_EXECUTION;
        }
        break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        if (sighandlers[MSVCRT_SIGILL])
        {
            if (sighandlers[MSVCRT_SIGILL] != MSVCRT_SIG_IGN)
                sighandlers[MSVCRT_SIGILL](MSVCRT_SIGILL);
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
        ret = sighandlers[sig];
        sighandlers[sig] = func;
        break;
    default:
        ret = MSVCRT_SIG_ERR;
    }
    return ret;
}

/*********************************************************************
 *		_XcptFilter (MSVCRT.@)
 */
int CDECL _XcptFilter(NTSTATUS ex, PEXCEPTION_POINTERS ptr)
{
    TRACE("(%08lx,%p)\n", ex, ptr);
    /* I assume ptr->ExceptionRecord->ExceptionCode is the same as ex */
    return msvcrt_exception_filter(ptr);
}
