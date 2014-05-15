/*
 * Copyright 2014 Hans Leidekker for CodeWeavers
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wininet.h"
#include "wine/debug.h"

static HINSTANCE instance;

WINE_DEFAULT_DEBUG_CHANNEL(jsproxy);

static CRITICAL_SECTION cs_jsproxy;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &cs_jsproxy,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": cs_jsproxy") }
};
static CRITICAL_SECTION cs_jsproxy = { &critsect_debug, -1, 0, 0, 0, 0 };

/******************************************************************
 *      DllMain (jsproxy.@)
 */
BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        instance = hinst;
        DisableThreadLibraryCalls( hinst );
        break;

    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

static inline void *heap_alloc( SIZE_T size )
{
    return HeapAlloc( GetProcessHeap(), 0, size );
}

static inline BOOL heap_free( LPVOID mem )
{
    return HeapFree( GetProcessHeap(), 0, mem );
}

static inline WCHAR *strdupAW( const char *src, DWORD len )
{
    WCHAR *dst = NULL;
    if (src)
    {
        int dst_len = MultiByteToWideChar( CP_ACP, 0, src, len, NULL, 0 );
        if ((dst = heap_alloc( (dst_len + 1) * sizeof(WCHAR) ))) len = MultiByteToWideChar( CP_ACP, 0, src, len, dst, dst_len );
        dst[dst_len] = 0;
    }
    return dst;
}

static struct pac_script
{
    WCHAR *text;
} pac_script;
static struct pac_script *global_script = &pac_script;

/******************************************************************
 *      InternetDeInitializeAutoProxyDll (jsproxy.@)
 */
BOOL WINAPI InternetDeInitializeAutoProxyDll( LPSTR mime, DWORD reserved )
{
    TRACE( "%s, %u\n", debugstr_a(mime), reserved );

    EnterCriticalSection( &cs_jsproxy );

    heap_free( global_script->text );
    global_script->text = NULL;

    LeaveCriticalSection( &cs_jsproxy );
    return TRUE;
}

static WCHAR *load_script( const char *filename )
{
    HANDLE handle;
    DWORD size, bytes_read;
    char *buffer;
    int len;
    WCHAR *script = NULL;

    handle = CreateFileA( filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );
    if (handle == INVALID_HANDLE_VALUE) return NULL;

    size = GetFileSize( handle, NULL );
    if (!(buffer = heap_alloc( size ))) goto done;
    if (!ReadFile( handle, buffer, size, &bytes_read, NULL ) || bytes_read != size) goto done;

    len = MultiByteToWideChar( CP_ACP, 0, buffer, size, NULL, 0 );
    if (!(script = heap_alloc( (len + 1) * sizeof(WCHAR) ))) goto done;
    MultiByteToWideChar( CP_ACP, 0, buffer, size, script, len );
    script[len] = 0;

done:
    CloseHandle( handle );
    heap_free( buffer );
    return script;
}

/******************************************************************
 *      InternetInitializeAutoProxyDll (jsproxy.@)
 */
BOOL WINAPI JSPROXY_InternetInitializeAutoProxyDll( DWORD version, LPSTR tmpfile, LPSTR mime,
                                                    AutoProxyHelperFunctions *callbacks,
                                                    LPAUTO_PROXY_SCRIPT_BUFFER buffer )
{
    BOOL ret = FALSE;

    TRACE( "%u, %s, %s, %p, %p\n", version, debugstr_a(tmpfile), debugstr_a(mime), callbacks, buffer );

    if (callbacks) FIXME( "callbacks not supported\n" );

    EnterCriticalSection( &cs_jsproxy );

    if (global_script->text)
    {
        LeaveCriticalSection( &cs_jsproxy );
        return FALSE;
    }
    if (buffer && buffer->dwStructSize == sizeof(*buffer) && buffer->lpszScriptBuffer &&
        (global_script->text = strdupAW( buffer->lpszScriptBuffer, buffer->dwScriptBufferSize ))) ret = TRUE;
    else if ((global_script->text = load_script( tmpfile ))) ret = TRUE;

    LeaveCriticalSection( &cs_jsproxy );
    return ret;
}
