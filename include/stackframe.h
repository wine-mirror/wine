/*
 * 16-bit mode stack frame layout
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef WINE_STACKFRAME_H
#define WINE_STACKFRAME_H

#include <windows.h>

typedef struct
{
    WORD    saved_ss;
    WORD    saved_bp;
    WORD    saved_sp;
    WORD    es;
    WORD    ds;
    WORD    bp;
    WORD    arg_length;
    WORD    ip;
    WORD    cs;
    WORD    args[1];
} STACK16FRAME;


extern STACK16FRAME *pStack16Frame;

#endif /* WINE_STACKFRAME_H */
