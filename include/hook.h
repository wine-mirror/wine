/*
 * Windows hook definitions
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef HOOK_H
#define HOOK_H

#include "windows.h"
#include "ldt.h"

  /* Hook data (pointed to by a HHOOK) */
typedef struct
{
    HHOOK    next;   /* Next hook in chain */
    HOOKPROC proc;   /* Hook procedure */
    short    id;     /* Hook id (WH_xxx) */
    HTASK    htask;  /* Task owning this hook */
} HOOKDATA;


#define FIRST_HOOK  WH_MSGFILTER
#define LAST_HOOK   WH_SHELL

#define SYSTEM_HOOK(id)  (systemHooks[(id)-FIRST_HOOK])
#define TASK_HOOK(id)    (taskHooks[(id)-FIRST_HOOK])
#define INTERNAL_CALL_HOOK(hhook,code,wparam,lparam) \
    ((hhook) ? CallHookProc(((HOOKDATA*)PTR_SEG_TO_LIN(hhook))->proc,\
                            code, wparam, lparam) : 0)

#define CALL_SYSTEM_HOOK(id,code,wparam,lparam) \
    INTERNAL_CALL_HOOK(SYSTEM_HOOK(id),code,wparam,lparam)
#define CALL_TASK_HOOK(id,code,wparam,lparam) \
    INTERNAL_CALL_HOOK(TASK_HOOK(id),code,wparam,lparam)

extern DWORD CallHookProc( HOOKPROC func, short code,
			   WPARAM wParam, LPARAM lParam );  /* callback.c */

extern HHOOK systemHooks[];
extern HHOOK taskHooks[];

#endif  /* HOOK_H */
