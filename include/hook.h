/*
 * Windows hook definitions
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_HOOK_H
#define __WINE_HOOK_H

#include "windows.h"

extern HANDLE16 HOOK_GetHook( INT16 id , HQUEUE16 hQueue );
extern LRESULT HOOK_CallHooks( INT16 id, INT16 code,
                               WPARAM16 wParam, LPARAM lParam );
extern HOOKPROC16 HOOK_GetProc16( HHOOK hook );
extern void HOOK_ResetQueueHooks( HQUEUE16 hQueue );
extern void HOOK_FreeModuleHooks( HMODULE16 hModule );
extern void HOOK_FreeQueueHooks( HQUEUE16 hQueue );

#endif  /* __WINE_HOOK_H */
