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
    DWORD   saved_ss_sp;             /* saved previous 16-bit stack */
    DWORD   ebp;                     /* full 32-bit content of ebp */
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
