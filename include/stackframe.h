/*
 * 16-bit mode stack frame layout
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef WINE_STACKFRAME_H
#define WINE_STACKFRAME_H

#include <windows.h>
#include "ldt.h"

typedef struct
{
    WORD    saved_ss;
    WORD    saved_bp;
    WORD    saved_sp;
    WORD    ds;
    WORD    bp;
    WORD    arg_length;
    WORD    ip;
    WORD    cs;
    WORD    args[1];
} STACK16FRAME;

extern WORD IF1632_Saved16_ss;
extern WORD IF1632_Saved16_sp;
extern WORD IF1632_Saved16_bp;

#define CURRENT_STACK16 \
    ((STACK16FRAME *)PTR_SEG_OFF_TO_LIN(IF1632_Saved16_ss,IF1632_Saved16_sp))

#define CURRENT_DS   (CURRENT_STACK16->ds)

#endif /* WINE_STACKFRAME_H */
