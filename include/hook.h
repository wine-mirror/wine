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

#ifndef WINELIB
#pragma pack(1)
#endif

  /* Hook data (pointed to by a HHOOK) */
typedef struct
{
    HANDLE   next;               /* 00 Next hook in chain */
    HOOKPROC proc WINE_PACKED;   /* 02 Hook procedure */
    short    id;                 /* 06 Hook id (WH_xxx) */
    HQUEUE   ownerQueue;         /* 08 Owner queue (0 for system hook) */
    HMODULE  ownerModule;        /* 0a Owner module */
    WORD     inHookProc;         /* 0c TRUE if in this->proc */
} HOOKDATA;

#ifndef WINELIB
#pragma pack(4)
#endif

#define HOOK_MAGIC  ((int)'H' | (int)'K' << 8)  /* 'HK' */

extern HANDLE HOOK_GetHook( short id , HQUEUE hQueue );
extern DWORD HOOK_CallHooks( short id, short code,
                             WPARAM wParam, LPARAM lParam );
extern void HOOK_FreeModuleHooks( HMODULE hModule );
extern void HOOK_FreeQueueHooks( HQUEUE hQueue );

#endif  /* __WINE_HOOK_H */
