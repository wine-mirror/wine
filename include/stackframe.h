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
    DWORD   relay;          /* 24 return address to relay stub */
    DWORD   retaddr;        /* 28 actual return address */
    DWORD   args[1];        /* 2c arguments to 16-bit function */
} STACK32FRAME;

  /* 16-bit stack layout after CallFrom16() */
typedef struct
{
    STACK32FRAME *frame32;        /* 00 32-bit frame from last CallTo16() */
    DWORD         edx;            /* 04 saved registers */
    DWORD         ecx;            /* 08 */
    DWORD         ebp;            /* 0c */
    WORD          ds;             /* 10 */
    WORD          es;             /* 12 */
    WORD          fs;             /* 14 */
    WORD          gs;             /* 16 */
    DWORD         relay;          /* 18 address of argument relay stub */
    DWORD         entry_ip;       /* 1c ip of entry point */
    DWORD         entry_cs;       /* 20 cs of entry point */
    DWORD         entry_point;    /* 24 API entry point to call, reused as mutex count */
    WORD          bp;             /* 28 16-bit stack frame chain */
    WORD          ip;             /* 2a return address */
    WORD          cs;             /* 2c */
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
