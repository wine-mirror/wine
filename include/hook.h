/*
 * Windows hook definitions
 *
 * Copyright 1994 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
extern void HOOK_FreeQueueHooks(void);

#endif  /* __WINE_HOOK_H */
