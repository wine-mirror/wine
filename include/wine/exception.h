/*
 * Wine exception handling
 *
 * Copyright (c) 1999 Alexandre Julliard
 */

#ifndef __WINE_WINE_EXCEPTION_H
#define __WINE_WINE_EXCEPTION_H

#include <setjmp.h>
#include "winnt.h"
#include "thread.h"

/* The following definitions allow using exceptions in Wine and Winelib code
 *
 * They should be used like this:
 *
 * __TRY()
 * {
 *     do some stuff that can raise an exception
 * }
 * __EXCEPT(filter_func,param)
 * {
 *     handle the exception here
 * }
 * __ENDTRY()
 *
 * or
 *
 * __TRY()
 * {
 *     do some stuff that can raise an exception
 * }
 * __FINALLY(finally_func,param)
 *
 * The filter_func must be defined with the WINE_EXCEPTION_FILTER
 * macro, and return one of the EXCEPTION_* code; it can use
 * GetExceptionInformation and GetExceptionCode to retrieve the
 * exception info.
 *
 * The finally_func must be defined with the WINE_FINALLY_FUNC macro.
 *
 * Warning: you cannot use return, break, or continue inside a __TRY
 * or __EXCEPT block.
 *
 * -- AJ
 */

#define __TRY() \
    do { __WINE_FRAME __f; int __state = -1; \
         while (__state != 2) switch(__state) \
         { case 0: /* try */

#define __EXCEPT(func,arg) \
                __state = 2; break; \
           case -1:  /* init */ \
                __f.frame.Handler = (PEXCEPTION_HANDLER)WINE_exception_handler; \
                __f.u.e.filter = (func); \
                __f.u.e.param = (LPVOID)(arg); \
                EXC_push_frame( &__f.frame ); \
                __state = setjmp( __f.u.e.jmp); \
                break; \
            case 1:  /* except */

#define __ENDTRY() \
                __state++; break; \
            } \
        EXC_pop_frame( &__f.frame ); \
    } while (0);

#define __FINALLY(func,arg) \
                __state = 2; break; \
           case -1:  /* init */ \
                __f.frame.Handler = (PEXCEPTION_HANDLER)WINE_finally_handler; \
                __f.u.f.finally_func = (func); \
                __f.u.f.param = (LPVOID)(arg); \
                EXC_push_frame( &__f.frame ); \
                __ENDTRY()

#define WINE_EXCEPTION_FILTER(func,arg) DWORD WINAPI func( EXCEPTION_POINTERS *__eptr, LPVOID arg )
#define WINE_FINALLY_FUNC(func,arg) void WINAPI func( LPVOID arg )

#define GetExceptionInformation() (__eptr)
#define GetExceptionCode()        (__eptr->ExceptionRecord->ExceptionCode)


typedef DWORD (*CALLBACK __WINE_FILTER)(PEXCEPTION_POINTERS,LPVOID);
typedef void (*CALLBACK __WINE_FINALLY)(LPVOID);

typedef struct
{
    EXCEPTION_FRAME frame;
    union
    {
        struct  /* exception data */
        {
            __WINE_FILTER   filter;
            LPVOID          param;
            jmp_buf         jmp;
        } e;
        struct  /* finally data */
        {
            __WINE_FINALLY  finally_func;
            LPVOID          param;
        } f;
    } u;
} __WINE_FRAME;

extern DWORD WINAPI WINE_exception_handler( PEXCEPTION_RECORD record, EXCEPTION_FRAME *frame,
                                            CONTEXT *context, LPVOID pdispatcher );
extern DWORD WINAPI WINE_finally_handler( PEXCEPTION_RECORD record, EXCEPTION_FRAME *frame,
                                          CONTEXT *context, LPVOID pdispatcher );

static inline EXCEPTION_FRAME *EXC_push_frame( EXCEPTION_FRAME *frame )
{
    TEB * teb = NtCurrentTeb();
    frame->Prev = teb->except;
    teb->except = frame;
    return frame;
}

static inline EXCEPTION_FRAME *EXC_pop_frame( EXCEPTION_FRAME *frame )
{
    NtCurrentTeb()->except = frame->Prev;
    return frame->Prev;
}

#endif  /* __WINE_WINE_EXCEPTION_H */
