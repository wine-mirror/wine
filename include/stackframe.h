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

typedef struct
{
    WORD    saved_ss;                /* saved previous 16-bit stack */
    WORD    saved_bp;
    WORD    saved_sp;
    WORD    ds;                      /* 16-bit ds */
    DWORD   entry_point WINE_PACKED; /* entry point to call */
    WORD    ordinal_number;          /* ordinal number of entry point */
    WORD    dll_id;                  /* DLL id of entry point */
    WORD    bp;                      /* 16-bit bp */
    WORD    ip;                      /* return address */
    WORD    cs;
    WORD    args[1];                 /* arguments to API function */
} STACK16FRAME;

#ifndef WINELIB
#pragma pack(4)
#endif

extern WORD IF1632_Saved16_ss;
extern WORD IF1632_Saved16_sp;
extern WORD IF1632_Saved16_bp;

#define CURRENT_STACK16 \
    ((STACK16FRAME *)PTR_SEG_OFF_TO_LIN(IF1632_Saved16_ss,IF1632_Saved16_sp))

#define CURRENT_DS   (CURRENT_STACK16->ds)

#endif /* WINE_STACKFRAME_H */
