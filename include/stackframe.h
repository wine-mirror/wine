/*
 * 16-bit mode stack frame layout
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef WINE_STACKFRAME_H
#define WINE_STACKFRAME_H

#include <windows.h>
#include "ldt.h"

#ifndef WINELIB
#pragma pack(1)
#endif

  /* 16-bit stack layout after CallFrom16() */
typedef struct
{
    WORD    saved_ss;                /* saved previous 16-bit stack */
    WORD    saved_sp;
    WORD    entry_ip;                /* ip of entry point */
    WORD    ds;                      /* ds */
    WORD    entry_cs;                /* cs of entry point */
    WORD    es;                      /* es */
    DWORD   entry_point WINE_PACKED; /* 32-bit entry point to call */
    WORD    bp;                      /* 16-bit bp */
    WORD    ip;                      /* return address */
    WORD    cs;
    WORD    args[1];                 /* arguments to API function */
} STACK16FRAME;

  /* 32-bit stack layout after CallTo16() */
typedef struct
{
    DWORD   saved_esp;      /* saved previous 32-bit stack */
    DWORD   edi;            /* saved registers */
    DWORD   esi;
    DWORD   edx;
    DWORD   ecx;
    DWORD   ebx;
    DWORD   ebp;            /* saved 32-bit frame pointer */
    DWORD   retaddr;        /* return address */
    DWORD   codeselector;   /* code selector for return address */
    DWORD   args[1];        /* arguments to 16-bit function */
} STACK32FRAME;

#ifndef WINELIB
#pragma pack(4)
#endif

  /* Saved 16-bit stack for current process (Win16 only) */
extern WORD IF1632_Saved16_ss;
extern WORD IF1632_Saved16_sp;

  /* Saved 32-bit stack for current process (Win16 only) */
extern DWORD IF1632_Saved32_esp;
extern SEGPTR IF1632_Stack32_base;

  /* Original Unix stack */
extern DWORD IF1632_Original32_esp;

#ifndef WINELIB
#define CURRENT_STACK16 \
    ((STACK16FRAME *)PTR_SEG_OFF_TO_LIN(IF1632_Saved16_ss,IF1632_Saved16_sp))

#define CURRENT_DS   (CURRENT_STACK16->ds)

  /* Make a segmented pointer from a pointer to a variable located */
  /* on the 32-bit stack for the current task. */
#if 0
#define MAKE_SEGPTR(ptr) \
     ((SEGPTR)IF1632_Stack32_base + \
      ((DWORD)(ptr) - (DWORD)PTR_SEG_TO_LIN(IF1632_Stack32_base)))
#endif
SEGPTR MAKE_SEGPTR(void *ptr);
#else
#define CURRENT_STACK16	   error.error
#define CURRENT_DS         ((WORD)GetTaskDS())
#define MAKE_SEGPTR(ptr)   ((SEGPTR)ptr)
#endif

#endif /* WINE_STACKFRAME_H */
