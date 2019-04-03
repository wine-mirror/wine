/*
 * Fallbacks for debugging functions when running on Windows
 *
 * Copyright 2019 Alexandre Julliard
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

#ifdef _WIN32

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

static const char * (__cdecl *p__wine_dbg_strdup)( const char *str );
static int (__cdecl *p__wine_dbg_output)( const char *str );

static void load_func( void **func, const char *name, void *def )
{
    if (!*func)
    {
        HMODULE module = GetModuleHandleA( "ntdll.dll" );
        void *proc = GetProcAddress( module, name );
        interlocked_xchg_ptr( func, proc ? proc : def );
    }
}
#define LOAD_FUNC(name) load_func( (void **)&p ## name, #name, fallback ## name )

/* FIXME: this is not 100% thread-safe */
static const char * __cdecl fallback__wine_dbg_strdup( const char *str )
{
    static char *list[32];
    static int pos;
    char *ret = _strdup( str );
    int idx;

    idx = interlocked_xchg_add( &pos, 1 ) % ARRAY_SIZE(list);
    free( interlocked_xchg_ptr( (void **)&list[idx], ret ));
    return ret;
}

static int __cdecl fallback__wine_dbg_output( const char *str )
{
    return fwrite( str, 1, strlen(str), stderr );
}

const char * __cdecl __wine_dbg_strdup( const char *str )
{
    LOAD_FUNC( __wine_dbg_strdup );
    return p__wine_dbg_strdup( str );
}

int __cdecl __wine_dbg_output( const char *str )
{
    LOAD_FUNC( __wine_dbg_output );
    return p__wine_dbg_output( str );
}

#endif  /* _WIN32 */
