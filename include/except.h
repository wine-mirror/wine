/*
 * except.h
 * Copyright (c) 1996 Onno Hovers (onno@stack.urc.tue.nl)
 * Copyright (c) 1999 Alexandre Julliard
 */

#ifndef __WINE_EXCEPT_H
#define __WINE_EXCEPT_H

#include <setjmp.h>
#include "winnt.h"
#include "thread.h"

                        
static inline EXCEPTION_FRAME *EXC_push_frame( EXCEPTION_FRAME *frame )
{
    TEB * teb = NtCurrentTeb();
    frame->Prev = teb->except;
    teb->except = frame;
    return frame;
}

static inline EXCEPTION_FRAME *EXC_pop_frame( EXCEPTION_FRAME *frame )
{
    NtCurrentTeb()->except = frame->Prev;
    return frame->Prev;
}

#endif  /* __WINE_EXCEPT_H */
