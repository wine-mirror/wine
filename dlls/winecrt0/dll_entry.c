/*
 * Default entry point for a dll
 *
 * Copyright 2005 Alexandre Julliard
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

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wine/library.h"
#include "crt0_private.h"

extern BOOL WINAPI DECLSPEC_HIDDEN DllMain( HINSTANCE inst, DWORD reason, LPVOID reserved );

BOOL WINAPI DECLSPEC_HIDDEN __wine_spec_dll_entry( HINSTANCE inst, DWORD reason, LPVOID reserved )
{
    static BOOL call_fini;
    BOOL ret;

    if (reason == DLL_PROCESS_ATTACH && __wine_spec_init_state != CONSTRUCTORS_DONE)
    {
        call_fini = TRUE;
        _init( __wine_main_argc, __wine_main_argv, NULL );
    }

    ret = DllMain( inst, reason, reserved );

    if (reason == DLL_PROCESS_DETACH && call_fini) _fini();

    return ret;
}
