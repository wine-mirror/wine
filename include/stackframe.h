/*
 * 16-bit mode stack frame layout
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_STACKFRAME_H
#define __WINE_STACKFRAME_H

#include <windows.h>
#include "ldt.h"

#pragma pack(1)

  /* 16-bit stack layout after CallFrom16() */
typedef struct
{
    DWORD   saved_ss_sp;    /* 00 saved previous 16-bit stack */
    DWORD   ebp;            /* 04 full 32-bit content of ebp */
    WORD    entry_ip;       /* 08 ip of entry point */
    WORD    ds;             /* 0a ds */
    WORD    entry_cs;       /* 0c cs of entry point */
    WORD    es;             /* 0e es */
    DWORD   entry_point;    /* 10 32-bit entry point to call */
    WORD    bp;             /* 14 16-bit bp */
    WORD    ip;             /* 16 return address */
    WORD    cs;             /* 18 */
    WORD    args[1];        /* 1a arguments to API function */
} STACK16FRAME;

  /* 32-bit stack layout after CallTo16() */
typedef struct
{
    DWORD   edi;            /* 00 saved registers */
    DWORD   esi;            /* 04 */
    DWORD   edx;            /* 08 */
    DWORD   ecx;            /* 0c */
    DWORD   ebx;            /* 10 */
    DWORD   restore_addr;   /* 14 return address for restoring code selector */
    DWORD   codeselector;   /* 18 code selector to restore */
    DWORD   ebp;            /* 1c saved 32-bit frame pointer */
    DWORD   retaddr;        /* 20 actual return address */
    DWORD   args[1];        /* 24 arguments to 16-bit function */
} STACK32FRAME;

#pragma pack(4)

  /* Saved 16-bit stack for current process (Win16 only) */
extern DWORD IF1632_Saved16_ss_sp;

  /* Saved 32-bit stack for current process (Win16 only) */
extern DWORD IF1632_Saved32_esp;

  /* Original Unix stack */
extern DWORD IF1632_Original32_esp;

#define CURRENT_STACK16  ((STACK16FRAME *)PTR_SEG_TO_LIN(IF1632_Saved16_ss_sp))

#define CURRENT_DS   (CURRENT_STACK16->ds)

#endif /* __WINE_STACKFRAME_H */
