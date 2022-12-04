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

#include "ime_test.h"

BOOL WINAPI ImeConfigure( HKL hkl, HWND hwnd, DWORD mode, void *data )
{
    return FALSE;
}

DWORD WINAPI ImeConversionList( HIMC himc, const WCHAR *source, CANDIDATELIST *dest, DWORD dest_len, UINT flag )
{
    return 0;
}

BOOL WINAPI ImeDestroy( UINT force )
{
    return FALSE;
}

UINT WINAPI ImeEnumRegisterWord( REGISTERWORDENUMPROCW proc, const WCHAR *reading, DWORD style,
                                 const WCHAR *string, void *data )
{
    return 0;
}

LRESULT WINAPI ImeEscape( HIMC himc, UINT escape, void *data )
{
    return 0;
}

DWORD WINAPI ImeGetImeMenuItems( HIMC himc, DWORD flags, DWORD type, IMEMENUITEMINFOW *parent,
                                 IMEMENUITEMINFOW *menu, DWORD size )
{
    return 0;
}

UINT WINAPI ImeGetRegisterWordStyle( UINT item, STYLEBUFW *style_buf )
{
    return 0;
}

BOOL WINAPI ImeInquire( IMEINFO *info, WCHAR *ui_class, DWORD flags )
{
    return FALSE;
}

BOOL WINAPI ImeProcessKey( HIMC himc, UINT vkey, LPARAM key_data, BYTE *key_state )
{
    return FALSE;
}

BOOL WINAPI ImeRegisterWord( const WCHAR *reading, DWORD style, const WCHAR *string )
{
    return FALSE;
}

BOOL WINAPI ImeSelect( HIMC himc, BOOL select )
{
    return FALSE;
}

BOOL WINAPI ImeSetActiveContext( HIMC himc, BOOL flag )
{
    return FALSE;
}

BOOL WINAPI ImeSetCompositionString( HIMC himc, DWORD index, const void *comp, DWORD comp_len,
                                     const void *read, DWORD read_len )
{
    return FALSE;
}

UINT WINAPI ImeToAsciiEx( UINT vkey, UINT scan_code, BYTE *key_state, TRANSMSGLIST *msgs, UINT state, HIMC himc )
{
    return 0;
}

BOOL WINAPI ImeUnregisterWord( const WCHAR *reading, DWORD style, const WCHAR *string )
{
    return FALSE;
}

BOOL WINAPI NotifyIME( HIMC himc, DWORD action, DWORD index, DWORD value )
{
    return FALSE;
}

BOOL WINAPI DllMain( HINSTANCE instance, DWORD reason, LPVOID reserved )
{
    return TRUE;
}
