/*
 * msvcrt.dll exception handling
 *
 * Copyright 2000 Jon Griffiths
 *
 * See http://www.microsoft.com/msj/0197/exception/exception.htm,
 * but don't believe all of it.
 *
 * FIXME: Incomplete, no support for nested exceptions or try block cleanup.
 */
#include <setjmp.h>
#include "ntddk.h"
#include "thread.h"
#include "msvcrt.h"

DEFAULT_DEBUG_CHANNEL(msvcrt);

typedef void (*MSVCRT_sig_handler_func)(void);

/* VC++ extensions to Win32 SEH */
typedef struct _SCOPETABLE
{
  DWORD previousTryLevel;
  int (__cdecl *lpfnFilter)(int, PEXCEPTION_POINTERS);
  int (__cdecl *lpfnHandler)(PEXCEPTION_RECORD, PEXCEPTION_FRAME,
                             PCONTEXT, PEXCEPTION_FRAME *);
} SCOPETABLE, *PSCOPETABLE;

typedef struct _MSVCRT_EXCEPTION_REGISTRATION
{
  struct _EXCEPTION_REGISTRATION *prev;
  void (*handler)(PEXCEPTION_RECORD, PEXCEPTION_FRAME,
                  PCONTEXT, PEXCEPTION_RECORD);
  PSCOPETABLE scopetable;
  int trylevel;
  int _ebp;
  PEXCEPTION_POINTERS xpointers;
} MSVCRT_EXCEPTION_REGISTRATION;

typedef struct _EXCEPTION_REGISTRATION
{
  struct _EXCEPTION_REGISTRATION *prev;
  void (*handler)(PEXCEPTION_RECORD, PEXCEPTION_FRAME,
                  PCONTEXT, PEXCEPTION_RECORD);
} EXCEPTION_REGISTRATION;

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
#if defined(__GNUC__) && defined(__i386__)
  TRACE("(%p)\n",frame);
  if (0)
unwind_label: return;
  RtlUnwind( frame, &&unwind_label, 0, 0 );
#else
  FIXME("(%p) stub\n",frame);
#endif
}

/*******************************************************************
 *		_local_unwind2 (MSVCRT.@)
 */
void __cdecl MSVCRT__local_unwind2(MSVCRT_EXCEPTION_REGISTRATION *endframe, DWORD nr )
{
  FIXME("(%p,%ld) stub\n",endframe,nr);
}

/*********************************************************************
 *		_except_handler2 (MSVCRT.@)
 */
int __cdecl MSVCRT__except_handler2(PEXCEPTION_RECORD rec,
                                    PEXCEPTION_FRAME frame,
                                    PCONTEXT context, PEXCEPTION_FRAME *dispatcher)
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
                                    MSVCRT_EXCEPTION_REGISTRATION *frame,
                                    PCONTEXT context,void *dispatcher)
{
  FIXME("exception %lx flags=%lx at %p handler=%p %p %p stub\n",
        rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress,
        frame->handler, context, dispatcher);
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
