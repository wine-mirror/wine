/*
 * Windows hook definitions
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_HOOK_H
#define __WINE_HOOK_H

#include "windef.h"

#define HOOK_WIN16	0x00
#define HOOK_WIN32A	0x01
#define HOOK_WIN32W	0x02
#define HOOK_INUSE	0x80


/* hook type mask */
#define HOOK_MAPTYPE (HOOK_WIN16 | HOOK_WIN32A | HOOK_WIN32W)

extern BOOL HOOK_IsHooked( INT16 id );
extern LRESULT HOOK_CallHooks16( INT16 id, INT16 code, WPARAM16 wParam,
				 LPARAM lParam );
extern LRESULT HOOK_CallHooksA( INT id, INT code, WPARAM wParam,
				  LPARAM lParam );
extern LRESULT HOOK_CallHooksW( INT id, INT code, WPARAM wParam,
				  LPARAM lParam );
extern void HOOK_FreeModuleHooks( HMODULE16 hModule );
extern void HOOK_FreeQueueHooks( HQUEUE16 hQueue );
extern void HOOK_ResetQueueHooks( HQUEUE16 hQueue );

#endif  /* __WINE_HOOK_H */
