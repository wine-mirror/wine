/*
 * Windows hook definitions
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_HOOK_H
#define __WINE_HOOK_H

#include "windows.h"
#include "ldt.h"
#include "callback.h"

#pragma pack(1)

  /* Hook data (pointed to by a HHOOK) */
typedef struct
{
    HANDLE16   next;               /* 00 Next hook in chain */
    HOOKPROC   proc WINE_PACKED;   /* 02 Hook procedure */
    short      id;                 /* 06 Hook id (WH_xxx) */
    HQUEUE16   ownerQueue;         /* 08 Owner queue (0 for system hook) */
    HMODULE16  ownerModule;        /* 0a Owner module */
    WORD       inHookProc;         /* 0c TRUE if in this->proc */
} HOOKDATA;

#pragma pack(4)

#define HOOK_MAGIC  ((int)'H' | (int)'K' << 8)  /* 'HK' */

extern HANDLE HOOK_GetHook( short id , HQUEUE hQueue );
extern DWORD HOOK_CallHooks( short id, short code,
                             WPARAM wParam, LPARAM lParam );
extern void HOOK_FreeModuleHooks( HMODULE16 hModule );
extern void HOOK_FreeQueueHooks( HQUEUE16 hQueue );

#endif  /* __WINE_HOOK_H */
