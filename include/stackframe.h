/*
 * 16-bit and 32-bit mode stack frame layout
 *
 * Copyright 1995, 1998 Alexandre Julliard
 */

#ifndef __WINE_STACKFRAME_H
#define __WINE_STACKFRAME_H

#include "ldt.h"
#include "thread.h"

#include "pshpack1.h"

  /* 32-bit stack layout after CallTo16() */
typedef struct _STACK32FRAME
{
    SEGPTR  frame16;        /* 00 16-bit frame from last CallFrom16() */
    DWORD   restore_addr;   /* 04 return address for restoring code selector */
    DWORD   codeselector;   /* 08 code selector to restore */
    DWORD   edi;            /* 0c saved registers */
    DWORD   esi;            /* 10 */
    DWORD   edx;            /* 14 */
    DWORD   ecx;            /* 18 */
    DWORD   ebx;            /* 1c */
    DWORD   ebp;            /* 20 saved 32-bit frame pointer */
    DWORD   retaddr;        /* 24 actual return address */
    DWORD   args[1];        /* 28 arguments to 16-bit function */
} STACK32FRAME;

  /* 16-bit stack layout after CallFrom16() */
typedef struct
{
    STACK32FRAME *frame32;        /* 00 32-bit frame from last CallTo16() */
    DWORD         ebp;            /* 04 full 32-bit content of ebp */
    WORD          mutex_count;    /* 08 Win16Mutex recursion count */
    WORD          fs;             /* 0a fs */
    WORD          entry_ip;       /* 0c ip of entry point */
    WORD          ds;             /* 0e ds */
    WORD          entry_cs;       /* 10 cs of entry point */
    WORD          es;             /* 12 es */
    DWORD         entry_point;    /* 14 32-bit entry point to call */
    WORD          bp;             /* 18 16-bit bp */
    WORD          ip;             /* 1a return address */
    WORD          cs;             /* 1c */
} STACK16FRAME;

#include "poppack.h"

#define THREAD_STACK16(teb)  ((STACK16FRAME*)PTR_SEG_TO_LIN((teb)->cur_stack))
#define CURRENT_STACK16      (THREAD_STACK16(NtCurrentTeb()))
#define CURRENT_DS           (CURRENT_STACK16->ds)

/* varargs lists on the 16-bit stack */

typedef void *VA_LIST16;

#define __VA_ROUNDED16(type) \
    ((sizeof(type) + sizeof(WORD) - 1) / sizeof(WORD) * sizeof(WORD))
#define VA_START16(list) ((list) = (VA_LIST16)(CURRENT_STACK16 + 1))
#define VA_ARG16(list,type) \
    (((list) = (VA_LIST16)((char *)(list) + __VA_ROUNDED16(type))), \
     *((type *)(void *)((char *)(list) - __VA_ROUNDED16(type))))
#define VA_END16(list) ((void)0)

/* Push bytes on the 16-bit stack of a thread;
 * return a segptr to the first pushed byte
 */
#define STACK16_PUSH(teb,size) \
 (memmove((char*)THREAD_STACK16(teb)-(size),THREAD_STACK16(teb), \
          sizeof(STACK16FRAME)), \
  (teb)->cur_stack -= (size), \
  (SEGPTR)((teb)->cur_stack + sizeof(STACK16FRAME)))

/* Pop bytes from the 16-bit stack of a thread */
#define STACK16_POP(teb,size) \
 (memmove((char*)THREAD_STACK16(teb)+(size),THREAD_STACK16(teb), \
          sizeof(STACK16FRAME)), \
  (teb)->cur_stack += (size))

/* Push a DWORD on the 32-bit stack */
#define STACK32_PUSH(context,val) (*--(*(DWORD **)&ESP_reg(context)) = (val))

/* Pop a DWORD from the 32-bit stack */
#define STACK32_POP(context) (*(*(DWORD **)&ESP_reg(context))++)

/* Win32 register functions */
#define REGS_FUNC(name) __regs_##name

#endif /* __WINE_STACKFRAME_H */
