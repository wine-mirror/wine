/*
 * Windows hook definitions
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_HOOK_H
#define __WINE_HOOK_H

#include "windows.h"

#define HOOK_WIN16   0x0
#define HOOK_WIN32   0x1
#define HOOK_UNICODE 0x2

#define HOOK_MAPTYPE (HOOK_WIN32 | HOOK_UNICODE)

extern HOOKPROC16 HOOK_GetProc16( HHOOK hhook );
extern BOOL32 HOOK_IsHooked( INT16 id );
extern LRESULT HOOK_CallHooks16( INT16 id, INT16 code, WPARAM16 wParam,
				 LPARAM lParam );
extern LRESULT HOOK_CallHooks32A( INT32 id, INT32 code, WPARAM32 wParam,
				  LPARAM lParam );
extern LRESULT HOOK_CallHooks32W( INT32 id, INT32 code, WPARAM32 wParam,
				  LPARAM lParam );
extern void HOOK_FreeModuleHooks( HMODULE16 hModule );
extern void HOOK_FreeQueueHooks( HQUEUE16 hQueue );
extern void HOOK_ResetQueueHooks( HQUEUE16 hQueue );
extern HOOKPROC32 HOOK_GetProc( HHOOK hook );

#endif  /* __WINE_HOOK_H */
