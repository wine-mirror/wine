/*
 * 16-bit and 32-bit mode stack frame layout
 *
 * Copyright 1995, 1998 Alexandre Julliard
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
 */

#ifndef __WINE_STACKFRAME_H
#define __WINE_STACKFRAME_H

#include <string.h>

#include "thread.h"
#include "winnt.h"
#include "wine/winbase16.h"

#include "pshpack1.h"

  /* 32-bit stack layout after CallTo16() */
typedef struct _STACK32FRAME
{
    DWORD   restore_addr;   /* 00 return address for restoring code selector */
    DWORD   codeselector;   /* 04 code selector to restore */
    EXCEPTION_FRAME frame;  /* 08 Exception frame */
    SEGPTR  frame16;        /* 10 16-bit frame from last CallFrom16() */
    DWORD   edi;            /* 14 saved registers */
    DWORD   esi;            /* 18 */
    DWORD   ebx;            /* 1c */
    DWORD   ebp;            /* 20 saved 32-bit frame pointer */
    DWORD   retaddr;        /* 24 return address */
    DWORD   target;         /* 28 target address / CONTEXT86 pointer */
    DWORD   nb_args;        /* 2c number of 16-bit argument bytes */
} STACK32FRAME;

  /* 16-bit stack layout after CallFrom16() */
typedef struct _STACK16FRAME
{
    STACK32FRAME *frame32;        /* 00 32-bit frame from last CallTo16() */
    DWORD         edx;            /* 04 saved registers */
    DWORD         ecx;            /* 08 */
    DWORD         ebp;            /* 0c */
    WORD          ds;             /* 10 */
    WORD          es;             /* 12 */
    WORD          fs;             /* 14 */
    WORD          gs;             /* 16 */
    DWORD         callfrom_ip;    /* 18 callfrom tail IP */
    DWORD         module_cs;      /* 1c module code segment */
    DWORD         relay;          /* 20 relay function address */
    WORD          entry_ip;       /* 22 entry point IP */
    DWORD         entry_point;    /* 26 API entry point to call, reused as mutex count */
    WORD          bp;             /* 2a 16-bit stack frame chain */
    WORD          ip;             /* 2c return address */
    WORD          cs;             /* 2e */
} STACK16FRAME;

#include "poppack.h"

#define THREAD_STACK16(teb)  ((STACK16FRAME*)MapSL((teb)->cur_stack))
#define CURRENT_STACK16      (THREAD_STACK16(NtCurrentTeb()))
#define CURRENT_DS           (CURRENT_STACK16->ds)

/* varargs lists on the 16-bit stack */
#define VA_START16(list) ((list) = (VA_LIST16)(CURRENT_STACK16 + 1))
#define VA_END16(list) ((void)0)


/* Push bytes on the 16-bit stack of a thread;
 * return a segptr to the first pushed byte
 */
static inline SEGPTR WINE_UNUSED stack16_push( int size )
{
    TEB *teb = NtCurrentTeb();
    STACK16FRAME *frame = THREAD_STACK16(teb);
    memmove( (char*)frame - size, frame, sizeof(*frame) );
    teb->cur_stack -= size;
    return (SEGPTR)(teb->cur_stack + sizeof(*frame));
}

/* Pop bytes from the 16-bit stack of a thread */
static inline void WINE_UNUSED stack16_pop( int size )
{
    TEB *teb = NtCurrentTeb();
    STACK16FRAME *frame = THREAD_STACK16(teb);
    memmove( (char*)frame + size, frame, sizeof(*frame) );
    teb->cur_stack += size;
}

/* Push a DWORD on the 32-bit stack */
static inline void WINE_UNUSED stack32_push( CONTEXT86 *context, DWORD val )
{
    context->Esp -= sizeof(DWORD);
    *(DWORD *)context->Esp = val;
}

/* Pop a DWORD from the 32-bit stack */
static inline DWORD WINE_UNUSED stack32_pop( CONTEXT86 *context )
{
    DWORD ret = *(DWORD *)context->Esp;
    context->Esp += sizeof(DWORD);
    return ret;
}

#endif /* __WINE_STACKFRAME_H */
