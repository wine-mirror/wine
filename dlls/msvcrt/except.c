/*
 * msvcrt.dll exception handling
 *
 * Copyright 2000 Jon Griffiths
 *
 * See http://www.microsoft.com/msj/0197/exception/exception.htm,
 * but don't believe all of it.
 *
 * FIXME: Incomplete support for nested exceptions/try block cleanup.
 */
#include "ntddk.h"
#include "wine/exception.h"
#include "thread.h"
#include "msvcrt.h"

DEFAULT_DEBUG_CHANNEL(msvcrt);

typedef void (*MSVCRT_sig_handler_func)(void);

/* VC++ extensions to Win32 SEH */
typedef struct _SCOPETABLE
{
  DWORD previousTryLevel;
  int (__cdecl *lpfnFilter)(PEXCEPTION_POINTERS);
  int (__cdecl *lpfnHandler)(void);
} SCOPETABLE, *PSCOPETABLE;

typedef struct _MSVCRT_EXCEPTION_FRAME
{
  EXCEPTION_FRAME *prev;
  void (*handler)(PEXCEPTION_RECORD, PEXCEPTION_FRAME,
                  PCONTEXT, PEXCEPTION_RECORD);
  PSCOPETABLE scopetable;
  DWORD trylevel;
  int _ebp;
  PEXCEPTION_POINTERS xpointers;
} MSVCRT_EXCEPTION_FRAME;

#define TRYLEVEL_END 0xff /* End of trylevel list */

#if defined(__GNUC__) && defined(__i386__)

#define CALL_FINALLY_BLOCK(code_block, base_ptr) \
  __asm__ __volatile__ ("movl %0,%%eax; movl %1,%%ebp; call *%%eax" \
                        : : "g" (code_block), "g" (base_ptr))

static DWORD __cdecl MSVCRT_nested_handler(PEXCEPTION_RECORD rec,
                                           struct __EXCEPTION_FRAME *frame,
                                           PCONTEXT context WINE_UNUSED,
                                           struct __EXCEPTION_FRAME **dispatch)
{
  if (rec->ExceptionFlags & 0x6)
    return ExceptionContinueSearch;
  *dispatch = frame;
  return ExceptionCollidedUnwind;
}
#endif


/*********************************************************************
 *		_XcptFilter (MSVCRT.@)
 */
int __cdecl MSVCRT__XcptFilter(int ex, PEXCEPTION_POINTERS ptr)
{
  FIXME("(%d,%p)semi-stub\n", ex, ptr);
  return UnhandledExceptionFilter(ptr);
}

/*********************************************************************
 *		_EH_prolog (MSVCRT.@)
 */
#ifdef __i386__
/* Provided for VC++ binary compatability only */
__ASM_GLOBAL_FUNC(MSVCRT__EH_prolog,
                  "pushl $0xff\n\t"
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
void __cdecl MSVCRT__global_unwind2(PEXCEPTION_FRAME frame)
{
    TRACE("(%p)\n",frame);
    RtlUnwind( frame, 0, 0, 0 );
}

/*******************************************************************
 *		_local_unwind2 (MSVCRT.@)
 */
void __cdecl MSVCRT__local_unwind2(MSVCRT_EXCEPTION_FRAME *frame,
                                   DWORD trylevel)
{
  MSVCRT_EXCEPTION_FRAME *curframe = frame;
  DWORD curtrylevel = 0xfe;
  EXCEPTION_FRAME reg;

  TRACE("(%p,%ld,%ld)\n",frame, frame->trylevel, trylevel);

  /* Register a handler in case of a nested exception */
  reg.Handler = (PEXCEPTION_HANDLER)MSVCRT_nested_handler;
  reg.Prev = NtCurrentTeb()->except;
  __wine_push_frame(&reg);

  while (frame->trylevel != TRYLEVEL_END && frame->trylevel != trylevel)
  {
    curtrylevel = frame->scopetable[frame->trylevel].previousTryLevel;
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
int __cdecl MSVCRT__except_handler2(PEXCEPTION_RECORD rec,
                                    PEXCEPTION_FRAME frame,
                                    PCONTEXT context,
                                    PEXCEPTION_FRAME *dispatcher)
{
  FIXME("exception %lx flags=%lx at %p handler=%p %p %p stub\n",
        rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress,
        frame->Handler, context, dispatcher);
  return ExceptionContinueSearch;
}

/*********************************************************************
 *		_except_handler3 (MSVCRT.@)
 */
int __cdecl MSVCRT__except_handler3(PEXCEPTION_RECORD rec,
                                    MSVCRT_EXCEPTION_FRAME *frame,
                                    PCONTEXT context,void *dispatcher)
{
#if defined(__GNUC__) && defined(__i386__)
  long retval, trylevel;
  EXCEPTION_POINTERS exceptPtrs;
  PSCOPETABLE pScopeTable;

  TRACE("exception %lx flags=%lx at %p handler=%p %p %p semi-stub\n",
        rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress,
        frame->handler, context, dispatcher);

  __asm__ __volatile__ ("cld");

  if (rec->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
  {
    /* Unwinding the current frame */
     MSVCRT__local_unwind2(frame, TRYLEVEL_END);
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

        retval = pScopeTable[trylevel].lpfnFilter(&exceptPtrs);

        TRACE("filter returned %s\n", retval == EXCEPTION_CONTINUE_EXECUTION ?
              "CONTINUE_EXECUTION" : retval == EXCEPTION_EXECUTE_HANDLER ?
              "EXECUTE_HANDLER" : "CONTINUE_SEARCH");

        if (retval == EXCEPTION_CONTINUE_EXECUTION)
          return ExceptionContinueExecution;

        if (retval == EXCEPTION_EXECUTE_HANDLER)
        {
          /* Unwind all higher frames, this one will handle the exception */
          MSVCRT__global_unwind2((PEXCEPTION_FRAME)frame);
          MSVCRT__local_unwind2(frame, trylevel);

          /* Set our trylevel to the enclosing block, and call the __finally
           * code, which won't return
           */
          frame->trylevel = pScopeTable->previousTryLevel;
          TRACE("__finally block %p\n",pScopeTable[trylevel].lpfnHandler);
          CALL_FINALLY_BLOCK(pScopeTable[trylevel].lpfnHandler, frame->_ebp);
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
int __cdecl MSVCRT__abnormal_termination(void)
{
  FIXME("(void)stub\n");
  return 0;
}

/*******************************************************************
 *		_setjmp (MSVCRT.@)
 */
int __cdecl MSVCRT__setjmp(LPDWORD *jmpbuf)
{
  FIXME(":(%p): stub\n",jmpbuf);
  return 0;
}

/*******************************************************************
 *		_setjmp3 (MSVCRT.@)
 */
int __cdecl MSVCRT__setjmp3(LPDWORD *jmpbuf, int x)
{
  FIXME(":(%p %x): stub\n",jmpbuf,x);
  return 0;
}

/*********************************************************************
 *		longjmp (MSVCRT.@)
 */
void __cdecl MSVCRT_longjmp(jmp_buf env, int val)
{
  FIXME("MSVCRT_longjmp semistub, expect crash\n");
  longjmp(env, val);
}

/*********************************************************************
 *		signal (MSVCRT.@)
 */
void * __cdecl MSVCRT_signal(int sig, MSVCRT_sig_handler_func func)
{
  FIXME("(%d %p):stub\n", sig, func);
  return (void*)-1;
}
