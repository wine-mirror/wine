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

/*
 * the function pointer to a exception handler
 */

/* forward definition */
struct __EXCEPTION_FRAME;

typedef DWORD (CALLBACK *PEXCEPTION_HANDLER)( PEXCEPTION_RECORD pexcrec,
                                      struct __EXCEPTION_FRAME  *pestframe,
                                      PCONTEXT                   pcontext,
                                      struct __EXCEPTION_FRAME **pdispatcher);

/*
 * The exception frame, used for registering exception handlers 
 * Win32 cares only about this, but compilers generally emit 
 * larger exception frames for their own use.
 */

typedef struct __EXCEPTION_FRAME
{
  struct __EXCEPTION_FRAME *Prev;
  PEXCEPTION_HANDLER       Handler;
} EXCEPTION_FRAME, *PEXCEPTION_FRAME;

                        
void WINAPI RtlUnwind(PEXCEPTION_FRAME,LPVOID,PEXCEPTION_RECORD,DWORD);

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
