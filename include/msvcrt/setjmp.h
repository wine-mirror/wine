/*
 * Setjmp/Longjmp definitions
 *
 * Copyright 2001 Francois Gouget.
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
 */
#ifndef __WINE_SETJMP_H
#define __WINE_SETJMP_H
#ifndef __WINE_USE_MSVCRT
#define __WINE_USE_MSVCRT
#endif

#include <pshpack8.h>

#ifdef __i386__

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

#define _JBLEN                     16
#define _JBTYPE                    int
typedef _JBTYPE                    jmp_buf[_JBLEN];


#ifdef __cplusplus
extern "C" {
#endif

int         _setjmp(jmp_buf);
int         longjmp(jmp_buf,int);

#ifdef __cplusplus
}
#endif

#define setjmp _setjmp

#include <poppack.h>

#endif /* __WINE_SETJMP_H */
