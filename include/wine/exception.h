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
 *          break out of the current block. 'return' causes disaster and must
 *          never be used.
 *
 * -- AJ
 */

/* Define this if you want to use your compiler built-in __try/__except support.
 * This is only useful when compiling to a native Windows binary, as the built-in
 * compiler exceptions will most certainly not work under Winelib.
 */
#ifdef USE_COMPILER_EXCEPTIONS

#define __TRY __try
#define __EXCEPT(func,arg) __except((func)(GetExceptionInformation(),GetExceptionCode(),(arg)))
#define __FINALLY(func,arg) __finally { (func)(!AbnormalTermination(),(arg)); }
#define __ENDTRY /*nothing*/

#else  /* USE_COMPILER_EXCEPTIONS */

#define __TRY \
    do { __WINE_FRAME __f; \
         int __first = 1; \
         for (;;) if (!__first) \
         { \
             do {

#define __EXCEPT(func,arg) \
             } while(0); \
             EXC_pop_frame( &__f.frame ); \
             break; \
         } else { \
             __f.frame.Handler = (PEXCEPTION_HANDLER)WINE_exception_handler; \
             __f.u.e.filter = (func); \
             __f.u.e.param = (LPVOID)(arg); \
             EXC_push_frame( &__f.frame ); \
             if (setjmp( __f.u.e.jmp)) { \
                 EXC_pop_frame( &__f.frame ); \
                 do {

#define __ENDTRY \
                 } while (0); \
                 break; \
             } \
             __first = 0; \
         } \
    } while (0);

#define __FINALLY(func,arg) \
             } while(0); \
             EXC_pop_frame( &__f.frame ); \
             (func)( TRUE, (LPVOID)(arg) ); \
             break; \
         } else { \
             __f.frame.Handler = (PEXCEPTION_HANDLER)WINE_finally_handler; \
             __f.u.f.finally_func = (func); \
             __f.u.f.param = (LPVOID)(arg); \
             EXC_push_frame( &__f.frame ); \
             __first = 0; \
         } \
    } while (0);


/* Dummy structure for using GetExceptionCode in filter as well as in handler */
typedef struct
{
     union { struct { DWORD code; } e; } u;
} __WINE_DUMMY_FRAME;

typedef DWORD (*CALLBACK __WINE_FILTER)(PEXCEPTION_POINTERS,__WINE_DUMMY_FRAME,LPVOID);
typedef void (*CALLBACK __WINE_FINALLY)(BOOL,LPVOID);

#define WINE_EXCEPTION_FILTER(func,arg) \
    DWORD WINAPI func( EXCEPTION_POINTERS *__eptr, __WINE_DUMMY_FRAME __f, LPVOID arg )
#define WINE_FINALLY_FUNC(func,arg) \
    void WINAPI func( BOOL __normal, LPVOID arg )

#define GetExceptionInformation() (__eptr)
#define GetExceptionCode()        (__f.u.e.code)
#define AbnormalTermination()     (!__normal)

typedef struct
{
    EXCEPTION_FRAME frame;
    union
    {
        struct  /* exception data */
        {
            __WINE_FILTER   filter;
            LPVOID          param;
            DWORD           code;
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

#endif /* USE_COMPILER_EXCEPTIONS */

static inline EXCEPTION_FRAME *EXC_push_frame( EXCEPTION_FRAME *frame )
{
#if defined(__GNUC__) && defined(__i386__)
    EXCEPTION_FRAME *prev;
    __asm__ __volatile__(".byte 0x64\n\tmovl (0),%0"
                         "\n\tmovl %0,(%1)"
                         "\n\t.byte 0x64\n\tmovl %1,(0)"
                         : "=&r" (prev) : "r" (frame) : "memory" );
    return prev;
#else
    TEB * teb = NtCurrentTeb();
    frame->Prev = teb->except;
    teb->except = frame;
    return frame->Prev;
#endif
}

static inline EXCEPTION_FRAME *EXC_pop_frame( EXCEPTION_FRAME *frame )
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

#endif  /* __WINE_WINE_EXCEPTION_H */
