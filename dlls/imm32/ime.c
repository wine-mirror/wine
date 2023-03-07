/*
 * Copyright 2023 Rémi Bernon for CodeWeavers
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

#include <stdarg.h>
#include <stddef.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "imm.h"
#include "immdev.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

BOOL WINAPI ImeInquire( IMEINFO *info, WCHAR *ui_class, DWORD flags )
{
    FIXME( "info %p, ui_class %p, flags %#lx stub!\n", info, ui_class, flags );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

BOOL WINAPI ImeDestroy( UINT force )
{
    FIXME( "force %u stub!\n", force );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

BOOL WINAPI ImeSelect( HIMC himc, BOOL select )
{
    FIXME( "himc %p, select %d semi-stub!\n", himc, select );
    return TRUE;
}

BOOL WINAPI ImeSetActiveContext( HIMC himc, BOOL flag )
{
    static int once;
    if (!once++) FIXME( "himc %p, flag %#x stub!\n", himc, flag );
    return TRUE;
}

BOOL WINAPI ImeProcessKey( HIMC himc, UINT vkey, LPARAM key_data, BYTE *key_state )
{
    FIXME( "himc %p, vkey %u, key_data %#Ix, key_state %p stub!\n", himc, vkey, key_data, key_state );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

UINT WINAPI ImeToAsciiEx( UINT vkey, UINT scan_code, BYTE *key_state, TRANSMSGLIST *msgs, UINT state, HIMC himc )
{
    FIXME( "vkey %u, scan_code %u, key_state %p, msgs %p, state %u, himc %p stub!\n",
           vkey, scan_code, key_state, msgs, state, himc );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return 0;
}

BOOL WINAPI ImeConfigure( HKL hkl, HWND hwnd, DWORD mode, void *data )
{
    FIXME( "hkl %p, hwnd %p, mode %lu, data %p stub!\n", hkl, hwnd, mode, data );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

DWORD WINAPI ImeConversionList( HIMC himc, const WCHAR *source, CANDIDATELIST *dest, DWORD dest_len, UINT flag )
{
    FIXME( "himc %p, source %s, dest %p, dest_len %lu, flag %#x stub!\n",
           himc, debugstr_w(source), dest, dest_len, flag );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return 0;
}

BOOL WINAPI ImeSetCompositionString( HIMC himc, DWORD index, const void *comp, DWORD comp_len,
                                     const void *read, DWORD read_len )
{
    FIXME( "himc %p, index %lu, comp %p, comp_len %lu, read %p, read_len %lu stub!\n",
           himc, index, comp, comp_len, read, read_len );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

BOOL WINAPI NotifyIME( HIMC himc, DWORD action, DWORD index, DWORD value )
{
    FIXME( "himc %p, action %lu, index %lu, value %lu stub!\n", himc, action, index, value );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

LRESULT WINAPI ImeEscape( HIMC himc, UINT escape, void *data )
{
    FIXME( "himc %p, escape %#x, data %p stub!\n", himc, escape, data );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return 0;
}

DWORD WINAPI ImeGetImeMenuItems( HIMC himc, DWORD flags, DWORD type, IMEMENUITEMINFOW *parent,
                                 IMEMENUITEMINFOW *menu, DWORD size )
{
    FIXME( "himc %p, flags %#lx, type %lu, parent %p, menu %p, size %#lx stub!\n",
           himc, flags, type, parent, menu, size );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return 0;
}

BOOL WINAPI ImeRegisterWord( const WCHAR *reading, DWORD style, const WCHAR *string )
{
    FIXME( "reading %s, style %lu, string %s stub!\n", debugstr_w(reading), style, debugstr_w(string) );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

UINT WINAPI ImeGetRegisterWordStyle( UINT item, STYLEBUFW *style )
{
    FIXME( "item %u, style %p stub!\n", item, style );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return 0;
}

BOOL WINAPI ImeUnregisterWord( const WCHAR *reading, DWORD style, const WCHAR *string )
{
    FIXME( "reading %s, style %lu, string %s stub!\n", debugstr_w(reading), style, debugstr_w(string) );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

UINT WINAPI ImeEnumRegisterWord( REGISTERWORDENUMPROCW proc, const WCHAR *reading, DWORD style,
                                 const WCHAR *string, void *data )
{
    FIXME( "proc %p, reading %s, style %lu, string %s, data %p stub!\n",
           proc, debugstr_w(reading), style, debugstr_w(string), data );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return 0;
}
