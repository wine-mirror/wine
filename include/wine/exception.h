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
 * __TRY
 * {
 *     do some stuff that can raise an exception
 * }
 * __EXCEPT(filter_func,param)
 * {
 *     handle the exception here
 * }
 * __ENDTRY
 *
 * or
 *
 * __TRY
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
 * Warning: inside a __TRY or __EXCEPT block, 'break' or 'continue' statements
 *          break out of the current block. You cannot use 'return', 'goto'
 *          or 'longjmp' to leave a __TRY block, as this will surely crash.
 *          You can use them to leave a __EXCEPT block though.
 *
 * -- AJ
 */

/* Define this if you want to use your compiler built-in __try/__except support.
 * This is only useful when compiling to a native Windows binary, as the built-in
 * compiler exceptions will most certainly not work under Winelib.
 */
#ifdef USE_COMPILER_EXCEPTIONS

#define __TRY __try
#define __EXCEPT(func) __except((func)(GetExceptionInformation()))
#define __FINALLY(func) __finally { (func)(!AbnormalTermination()); }
#define __ENDTRY /*nothing*/

#else  /* USE_COMPILER_EXCEPTIONS */

#define __TRY \
    do { __WINE_FRAME __f; \
         int __first = 1; \
         for (;;) if (!__first) \
         { \
             do {

#define __EXCEPT(func) \
             } while(0); \
             __wine_pop_frame( &__f.frame ); \
             break; \
         } else { \
             __f.frame.Handler = (PEXCEPTION_HANDLER)__wine_exception_handler; \
             __f.u.filter = (func); \
             __wine_push_frame( &__f.frame ); \
             if (setjmp( __f.jmp)) { \
                 const __WINE_FRAME * const __eptr WINE_UNUSED = &__f; \
                 do {

#define __ENDTRY \
                 } while (0); \
                 break; \
             } \
             __first = 0; \
         } \
    } while (0);

#define __FINALLY(func) \
             } while(0); \
             __wine_pop_frame( &__f.frame ); \
             (func)(1); \
             break; \
         } else { \
             __f.frame.Handler = (PEXCEPTION_HANDLER)WINE_finally_handler; \
             __f.u.finally_func = (func); \
             __wine_push_frame( &__f.frame ); \
             __first = 0; \
         } \
    } while (0);


typedef DWORD CALLBACK (*__WINE_FILTER)(PEXCEPTION_POINTERS);
typedef void CALLBACK (*__WINE_FINALLY)(BOOL);

#define WINE_EXCEPTION_FILTER(func) DWORD WINAPI func( EXCEPTION_POINTERS *__eptr )
#define WINE_FINALLY_FUNC(func) void WINAPI func( BOOL __normal )

#define GetExceptionInformation() (__eptr)
#define GetExceptionCode()        (__eptr->ExceptionRecord->ExceptionCode)
#define AbnormalTermination()     (!__normal)

typedef struct __tagWINE_FRAME
{
    EXCEPTION_FRAME frame;
    union
    {
        /* exception data */
        __WINE_FILTER filter;
        /* finally data */
        __WINE_FINALLY finally_func;
    } u;
    jmp_buf jmp;
    /* hack to make GetExceptionCode() work in handler */
    DWORD ExceptionCode;
    const struct __tagWINE_FRAME *ExceptionRecord;
} __WINE_FRAME;

extern DWORD __wine_exception_handler( PEXCEPTION_RECORD record, EXCEPTION_FRAME *frame,
                                       CONTEXT *context, LPVOID pdispatcher );
extern DWORD __wine_finally_handler( PEXCEPTION_RECORD record, EXCEPTION_FRAME *frame,
                                     CONTEXT *context, LPVOID pdispatcher );

#endif /* USE_COMPILER_EXCEPTIONS */

static inline EXCEPTION_FRAME * WINE_UNUSED __wine_push_frame( EXCEPTION_FRAME *frame )
{
#if defined(__GNUC__) && defined(__i386__)
    EXCEPTION_FRAME *prev;
    __asm__ __volatile__(".byte 0x64\n\tmovl (0),%0"
                         "\n\tmovl %0,(%1)"
                         "\n\t.byte 0x64\n\tmovl %1,(0)"
                         : "=&r" (prev) : "r" (frame) : "memory" );
    return prev;
#else
    TEB *teb = NtCurrentTeb();
    frame->Prev = teb->except;
    teb->except = frame;
    return frame->Prev;
#endif
}

static inline EXCEPTION_FRAME * WINE_UNUSED __wine_pop_frame( EXCEPTION_FRAME *frame )
{
#if defined(__GNUC__) && defined(__i386__)
    __asm__ __volatile__(".byte 0x64\n\tmovl %0,(0)"
                         : : "r" (frame->Prev) : "memory" );
    return frame->Prev;

#else
    NtCurrentTeb()->except = frame->Prev;
    return frame->Prev;
#endif
}

#ifdef __WINE__
extern void WINAPI EXC_RtlRaiseException( PEXCEPTION_RECORD, PCONTEXT );
extern BOOL SIGNAL_Init(void);
#endif

#endif  /* __WINE_WINE_EXCEPTION_H */
