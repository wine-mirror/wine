/*
 * Setjmp/Longjmp definitions
 *
 * Copyright 2001 Francois Gouget.
 */
#ifndef __WINE_SETJMP_H
#define __WINE_SETJMP_H
#define __WINE_USE_MSVCRT

#ifdef USE_MSVCRT_PREFIX
#define MSVCRT(x)    MSVCRT_##x
#else
#define MSVCRT(x)    x
#endif


#ifdef __i386__

#define _JBLEN                     16
#define _JBTYPE                    int

typedef struct __JUMP_BUFFER
{
    unsigned long Ebp;
    unsigned long Ebx;
    unsigned long Edi;
    unsigned long Esi;
    unsigned long Esp;
    unsigned long Eip;
    unsigned long Registration;
    unsigned long TryLevel;
    /* Start of new struct members */
    unsigned long Cookie;
    unsigned long UnwindFunc;
    unsigned long UnwindData[6];
} _JUMP_BUFFER;

#endif /* __i386__ */

typedef _JBTYPE MSVCRT(jmp_buf)[_JBLEN];


#ifdef __cplusplus
extern "C" {
#endif

int         MSVCRT(_setjmp)( MSVCRT(jmp_buf));
int         MSVCRT(longjmp)( MSVCRT(jmp_buf),int);

#ifdef __cplusplus
}
#endif

#ifndef USE_MSVCRT_PREFIX
#define setjmp _setjmp
#endif

#endif /* __WINE_SETJMP_H */
