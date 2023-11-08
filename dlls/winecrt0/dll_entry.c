/*
 * Default entry point for a .so dll
 *
 * Copyright 2022 Alexandre Julliard
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#if 0
#pragma makedep unix
#endif

/* this is actually part of a static lib linked into a .dll.so module, not a real Unix library */
#undef WINE_UNIX_LIB

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"

extern void __wine_init_so_dll(void);

BOOL WINAPI __wine_spec_dll_entry( HINSTANCE inst, DWORD reason, LPVOID reserved )
{
    if (reason == DLL_PROCESS_ATTACH) __wine_init_so_dll();
    return DllMain( inst, reason, reserved );
}
