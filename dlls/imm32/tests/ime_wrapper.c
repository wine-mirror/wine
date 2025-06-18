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

#if 0
#pragma makedep testdll
#endif

#include "ime_test.h"

struct ime_functions ime_functions = {0};

BOOL WINAPI ImeConfigure( HKL hkl, HWND hwnd, DWORD mode, void *data )
{
    if (!ime_functions.pImeConfigure) return FALSE;
    return ime_functions.pImeConfigure( hkl, hwnd, mode, data );
}

DWORD WINAPI ImeConversionList( HIMC himc, const WCHAR *source, CANDIDATELIST *dest, DWORD dest_len, UINT flag )
{
    if (!ime_functions.pImeConversionList) return 0;
    return ime_functions.pImeConversionList( himc, source, dest, dest_len, flag );
}

BOOL WINAPI ImeDestroy( UINT force )
{
    if (!ime_functions.pImeDestroy) return FALSE;
    return ime_functions.pImeDestroy( force );
}

UINT WINAPI ImeEnumRegisterWord( REGISTERWORDENUMPROCW proc, const WCHAR *reading, DWORD style,
                                 const WCHAR *string, void *data )
{
    if (!ime_functions.pImeEnumRegisterWord) return 0;
    return ime_functions.pImeEnumRegisterWord( proc, reading, style, string, data );
}

LRESULT WINAPI ImeEscape( HIMC himc, UINT escape, void *data )
{
    if (!ime_functions.pImeEscape) return 0;
    return ime_functions.pImeEscape( himc, escape, data );
}

DWORD WINAPI ImeGetImeMenuItems( HIMC himc, DWORD flags, DWORD type, IMEMENUITEMINFOW *parent,
                                 IMEMENUITEMINFOW *menu, DWORD size )
{
    if (!ime_functions.pImeGetImeMenuItems) return 0;
    return ime_functions.pImeGetImeMenuItems( himc, flags, type, parent, menu, size );
}

UINT WINAPI ImeGetRegisterWordStyle( UINT item, STYLEBUFW *style_buf )
{
    if (!ime_functions.pImeGetRegisterWordStyle) return 0;
    return ime_functions.pImeGetRegisterWordStyle( item, style_buf );
}

BOOL WINAPI ImeInquire( IMEINFO *info, WCHAR *ui_class, DWORD flags )
{
    if (!ime_functions.pImeInquire) return FALSE;
    return ime_functions.pImeInquire( info, ui_class, flags );
}

BOOL WINAPI ImeProcessKey( HIMC himc, UINT vkey, LPARAM key_data, BYTE *key_state )
{
    if (!ime_functions.pImeProcessKey) return FALSE;
    return ime_functions.pImeProcessKey( himc, vkey, key_data, key_state );
}

BOOL WINAPI ImeRegisterWord( const WCHAR *reading, DWORD style, const WCHAR *string )
{
    if (!ime_functions.pImeRegisterWord) return FALSE;
    return ime_functions.pImeRegisterWord( reading, style, string );
}

BOOL WINAPI ImeSelect( HIMC himc, BOOL select )
{
    if (!ime_functions.pImeSelect) return FALSE;
    return ime_functions.pImeSelect( himc, select );
}

BOOL WINAPI ImeSetActiveContext( HIMC himc, BOOL flag )
{
    if (!ime_functions.pImeSetActiveContext) return FALSE;
    return ime_functions.pImeSetActiveContext( himc, flag );
}

BOOL WINAPI ImeSetCompositionString( HIMC himc, DWORD index, const void *comp, DWORD comp_len,
                                     const void *read, DWORD read_len )
{
    if (!ime_functions.pImeSetCompositionString) return FALSE;
    return ime_functions.pImeSetCompositionString( himc, index, comp, comp_len, read, read_len );
}

UINT WINAPI ImeToAsciiEx( UINT vkey, UINT scan_code, BYTE *key_state, TRANSMSGLIST *msgs, UINT state, HIMC himc )
{
    if (!ime_functions.pImeToAsciiEx) return 0;
    return ime_functions.pImeToAsciiEx( vkey, scan_code, key_state, msgs, state, himc );
}

BOOL WINAPI ImeUnregisterWord( const WCHAR *reading, DWORD style, const WCHAR *string )
{
    if (!ime_functions.pImeUnregisterWord) return FALSE;
    return ime_functions.pImeUnregisterWord( reading, style, string );
}

BOOL WINAPI NotifyIME( HIMC himc, DWORD action, DWORD index, DWORD value )
{
    if (!ime_functions.pNotifyIME) return FALSE;
    return ime_functions.pNotifyIME( himc, action, index, value );
}

BOOL WINAPI DllMain( HINSTANCE instance, DWORD reason, LPVOID reserved )
{
    static HMODULE module;

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        if (!(module = GetModuleHandleW( L"winetest_ime.dll" ))) return TRUE;
        ime_functions = *(struct ime_functions *)GetProcAddress( module, "ime_functions" );
        if (!ime_functions.pDllMain) return TRUE;
        return ime_functions.pDllMain( instance, reason, reserved );

    case DLL_PROCESS_DETACH:
        if (module == instance) return TRUE;
        if (!ime_functions.pDllMain) return TRUE;
        ime_functions.pDllMain( instance, reason, reserved );
        memset( &ime_functions, 0, sizeof(ime_functions) );
    }

    return TRUE;
}
