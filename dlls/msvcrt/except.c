/*
 * msvcrt.dll exception handling
 *
 * Copyright 2000 Jon Griffiths
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

#include "winternl.h"
#include "wine/exception.h"
#include "thread.h"
#include "msvcrt.h"

#include "msvcrt/setjmp.h"
#include "msvcrt/excpt.h"


#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

typedef void (*MSVCRT_sig_handler_func)(void);

/* VC++ extensions to Win32 SEH */
typedef struct _SCOPETABLE
{
  int previousTryLevel;
  int (*lpfnFilter)(PEXCEPTION_POINTERS);
  int (*lpfnHandler)(void);
} SCOPETABLE, *PSCOPETABLE;

typedef struct _MSVCRT_EXCEPTION_FRAME
{
  EXCEPTION_FRAME *prev;
  void (*handler)(PEXCEPTION_RECORD, PEXCEPTION_FRAME,
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

static DWORD call_filter( void *func, void *arg, void *ebp )
{
    DWORD ret;
    __asm__ __volatile__ ("pushl %%ebp; pushl %3; movl %2,%%ebp; call *%%eax; popl %%ebp; popl %%ebp"
                          : "=a" (ret) : "0" (func), "g" (ebp), "g" (arg) );
    return ret;
}
#endif

static DWORD MSVCRT_nested_handler(PEXCEPTION_RECORD rec,
                                   struct __EXCEPTION_FRAME* frame,
                                   PCONTEXT context WINE_UNUSED,
                                   struct __EXCEPTION_FRAME** dispatch)
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
/* Provided for VC++ binary compatability only */
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
void _global_unwind2(PEXCEPTION_FRAME frame)
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
  EXCEPTION_FRAME reg;

  TRACE("(%p,%d,%d)\n",frame, frame->trylevel, trylevel);

  /* Register a handler in case of a nested exception */
  reg.Handler = (PEXCEPTION_HANDLER)MSVCRT_nested_handler;
  reg.Prev = NtCurrentTeb()->except;
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
                     PEXCEPTION_FRAME frame,
                     PCONTEXT context,
                     PEXCEPTION_FRAME* dispatcher)
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
          _global_unwind2((PEXCEPTION_FRAME)frame);
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
void _MSVCRT__setjmp(_JUMP_BUFFER *jmp, CONTEXT86* context)
{
    TRACE("(%p)\n",jmp);
    jmp->Ebp = context->Ebp;
    jmp->Ebx = context->Ebx;
    jmp->Edi = context->Edi;
    jmp->Esi = context->Esi;
    jmp->Esp = context->Esp;
    jmp->Eip = context->Eip;
    jmp->Registration = (unsigned long)NtCurrentTeb()->except;
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
void _MSVCRT__setjmp3(_JUMP_BUFFER *jmp, int nb_args, CONTEXT86* context)
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
    jmp->Registration = (unsigned long)NtCurrentTeb()->except;
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
void _MSVCRT_longjmp(_JUMP_BUFFER *jmp, int retval, CONTEXT86* context)
{
    unsigned long cur_frame = 0;

    TRACE("(%p,%d)\n", jmp, retval);

    cur_frame=(unsigned long)NtCurrentTeb()->except;
    TRACE("cur_frame=%lx\n",cur_frame);

    if (cur_frame != jmp->Registration)
        _global_unwind2((PEXCEPTION_FRAME)jmp->Registration);

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
#endif /* i386 */

/*********************************************************************
 *		_seh_longjmp_unwind (MSVCRT.@)
 */
void __stdcall _seh_longjmp_unwind(_JUMP_BUFFER *jmp)
{
    _local_unwind2( (MSVCRT_EXCEPTION_FRAME *)jmp->Registration, jmp->TryLevel );
}

/*********************************************************************
 *		signal (MSVCRT.@)
 */
void* MSVCRT_signal(int sig, MSVCRT_sig_handler_func func)
{
  FIXME("(%d %p):stub\n", sig, func);
  return (void*)-1;
}
