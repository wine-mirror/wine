/*
 * Window classes functions
 *
 * Copyright 1993, 1996, 2003 Alexandre Julliard
 * Copyright 1998 Juergen Schmied (jsch)
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "user_private.h"
#include "controls.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(class);

#define MAX_ATOM_LEN 255 /* from dlls/kernel32/atom.c */

static inline const char *debugstr_us( const UNICODE_STRING *us )
{
    if (!us) return "<null>";
    return debugstr_wn( us->Buffer, us->Length / sizeof(WCHAR) );
}


/***********************************************************************
 *           get_int_atom_value
 */
ATOM get_int_atom_value( UNICODE_STRING *name )
{
    const WCHAR *ptr = name->Buffer;
    const WCHAR *end = ptr + name->Length / sizeof(WCHAR);
    UINT ret = 0;

    if (IS_INTRESOURCE(ptr)) return LOWORD(ptr);

    if (*ptr++ != '#') return 0;
    while (ptr < end)
    {
        if (*ptr < '0' || *ptr > '9') return 0;
        ret = ret * 10 + *ptr++ - '0';
        if (ret >= MAXINTATOM) return 0;
    }
    return ret;
}


/***********************************************************************
 *           is_comctl32_class
 */
static BOOL is_comctl32_class( const WCHAR *name )
{
    static const WCHAR *classesW[] =
    {
        L"ComboBoxEx32",
        L"msctls_hotkey32",
        L"msctls_progress32",
        L"msctls_statusbar32",
        L"msctls_trackbar32",
        L"msctls_updown32",
        L"NativeFontCtl",
        L"ReBarWindow32",
        L"SysAnimate32",
        L"SysDateTimePick32",
        L"SysHeader32",
        L"SysIPAddress32",
        L"SysLink",
        L"SysListView32",
        L"SysMonthCal32",
        L"SysPager",
        L"SysTabControl32",
        L"SysTreeView32",
        L"ToolbarWindow32",
        L"tooltips_class32",
    };

    int min = 0, max = ARRAY_SIZE( classesW ) - 1;

    while (min <= max)
    {
        int res, pos = (min + max) / 2;
        if (!(res = wcsicmp( name, classesW[pos] ))) return TRUE;
        if (res < 0) max = pos - 1;
        else min = pos + 1;
    }
    return FALSE;
}

static BOOL is_builtin_class( const WCHAR *name )
{
    static const WCHAR *classesW[] =
    {
        L"IME",
        L"MDIClient",
        L"Scrollbar",
    };

    int min = 0, max = ARRAY_SIZE( classesW ) - 1;

    while (min <= max)
    {
        int res, pos = (min + max) / 2;
        if (!(res = wcsicmp( name, classesW[pos] ))) return TRUE;
        if (res < 0) max = pos - 1;
        else min = pos + 1;
    }
    return FALSE;
}

void init_class_name_ansi( UNICODE_STRING *str, const char *name )
{
    if (IS_INTRESOURCE( name ))
    {
        str->Buffer = (WCHAR *)name;
        str->Length = str->MaximumLength = 0;
    }
    else
    {
        UINT len = MultiByteToWideChar( CP_ACP, 0, name, -1, str->Buffer, str->MaximumLength / sizeof(WCHAR) );
        str->Length = (len - 1) * sizeof(WCHAR);
    }
}

void init_class_name( UNICODE_STRING *str, const WCHAR *name )
{
    if (IS_INTRESOURCE( name ))
    {
        str->Buffer = (WCHAR *)name;
        str->Length = str->MaximumLength = 0;
    }
    else
    {
        str->Length = min( str->MaximumLength, wcslen( name ) * sizeof(WCHAR) );
        memcpy( str->Buffer, name, str->Length + sizeof(WCHAR) );
    }
}

static BOOL alloc_menu_nameA( struct client_menu_name *ret, const char *menu_name )
{
    if (!IS_INTRESOURCE(menu_name))
    {
        DWORD lenA = strlen( menu_name ) + 1;
        DWORD lenW = MultiByteToWideChar( CP_ACP, 0, menu_name, lenA, NULL, 0 );
        ret->nameW = HeapAlloc( GetProcessHeap(), 0, lenA + lenW * sizeof(WCHAR) );
        if (!ret->nameW) return FALSE;
        ret->nameA = (char *)(ret->nameW + lenW);
        MultiByteToWideChar( CP_ACP, 0, menu_name, lenA, ret->nameW, lenW );
        memcpy( ret->nameA, menu_name, lenA );
    }
    else
    {
        ret->nameW = (WCHAR *)menu_name;
        ret->nameA = (char *)menu_name;
    }
    return TRUE;
}

static BOOL alloc_menu_nameW( struct client_menu_name *ret, const WCHAR *menu_name )
{
    if (!IS_INTRESOURCE(menu_name))
    {
        DWORD lenW = lstrlenW( menu_name ) + 1;
        DWORD lenA = WideCharToMultiByte( CP_ACP, 0, menu_name, lenW, NULL, 0, NULL, NULL );
        ret->nameW = HeapAlloc( GetProcessHeap(), 0, lenA + lenW * sizeof(WCHAR) );
        if (!ret->nameW) return FALSE;
        ret->nameA = (char *)(ret->nameW + lenW);
        memcpy( ret->nameW, menu_name, lenW * sizeof(WCHAR) );
        WideCharToMultiByte( CP_ACP, 0, menu_name, lenW, ret->nameA, lenA, NULL, NULL );
    }
    else
    {
        ret->nameW = (WCHAR *)menu_name;
        ret->nameA = (char *)menu_name;
    }
    return TRUE;
}

static void free_menu_name( struct client_menu_name *name )
{
    if (!IS_INTRESOURCE(name->nameW)) HeapFree( GetProcessHeap(), 0, name->nameW );
}

static ULONG_PTR set_menu_nameW( HWND hwnd, INT offset, ULONG_PTR newval )
{
    struct client_menu_name menu_name;
    if (!alloc_menu_nameW( &menu_name, (const WCHAR *)newval )) return 0;
    NtUserSetClassLongPtr( hwnd, offset, (ULONG_PTR)&menu_name, FALSE );
    free_menu_name( &menu_name );
    return 0;
}

static ULONG_PTR set_menu_nameA( HWND hwnd, INT offset, ULONG_PTR newval )
{
    struct client_menu_name menu_name;
    if (!alloc_menu_nameA( &menu_name, (const char *)newval )) return 0;
    NtUserSetClassLongPtr( hwnd, offset, (ULONG_PTR)&menu_name, TRUE );
    free_menu_name( &menu_name );
    return 0;
}

void get_class_version( UNICODE_STRING *name, UNICODE_STRING *version, BOOL load )
{
    ACTCTX_SECTION_KEYED_DATA data = {.cbSize = sizeof(data)};
    const WCHAR *class_name = name->Buffer;
    HMODULE hmod = NULL;

    memset( version, 0, sizeof(*version) );

    if (IS_INTRESOURCE( name->Buffer ) || is_builtin_class( name->Buffer )) return;

    if (is_comctl32_class( name->Buffer ))
    {
        if (load && !(hmod = GetModuleHandleW( L"comctl32" ))) hmod = LoadLibraryW( L"comctl32" );
    }
    else if (!RtlFindActivationContextSectionString( 0, NULL, ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION, name, &data ))
    {
        struct wndclass_redirect_data
        {
            ULONG size;
            DWORD res;
            ULONG name_len;
            ULONG name_offset;
            ULONG module_len;
            ULONG module_offset;
        } *wndclass = (struct wndclass_redirect_data *)data.lpData;
        const WCHAR *module, *ptr;

        module = (const WCHAR *)((BYTE *)data.lpSectionBase + wndclass->module_offset);
        if (load && !(hmod = GetModuleHandleW( module ))) hmod = LoadLibraryW( module );

        *version = *name;
        version->Length = wndclass->name_len - name->Length;
        class_name += version->Length / sizeof(WCHAR);

        ptr = (const WCHAR *)((BYTE *)wndclass + wndclass->name_offset);
        memcpy( name->Buffer, ptr, wndclass->name_len );
        name->Length = wndclass->name_len;
        name->Buffer[name->Length / sizeof(WCHAR)] = 0;
    }

    if (load && hmod)
    {
        BOOL (WINAPI *pRegisterClassNameW)( const WCHAR *class );
        if ((pRegisterClassNameW = (void *)GetProcAddress( hmod, "RegisterClassNameW" )))
        {
            TRACE( "registering %s version %s\n", debugstr_us(name), debugstr_us(version) );
            pRegisterClassNameW( class_name );
        }
    }
}


static void load_uxtheme(void)
{
    BOOL (WINAPI * pIsThemeActive)(void);
    HMODULE uxtheme;

    uxtheme = LoadLibraryA("uxtheme.dll");
    if (uxtheme)
    {
        pIsThemeActive = (void *)GetProcAddress(uxtheme, "IsThemeActive");
        if (!pIsThemeActive || !pIsThemeActive())
            FreeLibrary(uxtheme);
    }
}

/***********************************************************************
 *           User32InitBuiltinClasses
 */
NTSTATUS WINAPI User32InitBuiltinClasses( void *args, ULONG size )
{
    DWORD layout;

    GetProcessDefaultLayout( &layout ); /* make sure that process layout is initialized */

    /* Load uxtheme.dll so that standard scrollbars and dialogs are hooked for theming support */
    load_uxtheme();
    return STATUS_SUCCESS;
}


/***********************************************************************
 *		RegisterClassA (USER32.@)
 *
 * Register a window class.
 *
 * RETURNS
 *	>0: Unique identifier
 *	0: Failure
 */
ATOM WINAPI RegisterClassA( const WNDCLASSA* wc ) /* [in] Address of structure with class data */
{
    WNDCLASSEXA wcex;

    wcex.cbSize        = sizeof(wcex);
    wcex.style         = wc->style;
    wcex.lpfnWndProc   = wc->lpfnWndProc;
    wcex.cbClsExtra    = wc->cbClsExtra;
    wcex.cbWndExtra    = wc->cbWndExtra;
    wcex.hInstance     = wc->hInstance;
    wcex.hIcon         = wc->hIcon;
    wcex.hCursor       = wc->hCursor;
    wcex.hbrBackground = wc->hbrBackground;
    wcex.lpszMenuName  = wc->lpszMenuName;
    wcex.lpszClassName = wc->lpszClassName;
    wcex.hIconSm       = 0;
    return RegisterClassExA( &wcex );
}


/***********************************************************************
 *		RegisterClassW (USER32.@)
 *
 * See RegisterClassA.
 */
ATOM WINAPI RegisterClassW( const WNDCLASSW* wc )
{
    WNDCLASSEXW wcex;

    wcex.cbSize        = sizeof(wcex);
    wcex.style         = wc->style;
    wcex.lpfnWndProc   = wc->lpfnWndProc;
    wcex.cbClsExtra    = wc->cbClsExtra;
    wcex.cbWndExtra    = wc->cbWndExtra;
    wcex.hInstance     = wc->hInstance;
    wcex.hIcon         = wc->hIcon;
    wcex.hCursor       = wc->hCursor;
    wcex.hbrBackground = wc->hbrBackground;
    wcex.lpszMenuName  = wc->lpszMenuName;
    wcex.lpszClassName = wc->lpszClassName;
    wcex.hIconSm       = 0;
    return RegisterClassExW( &wcex );
}


/***********************************************************************
 *		RegisterClassExA (USER32.@)
 */
ATOM WINAPI RegisterClassExA( const WNDCLASSEXA* wc )
{
    struct client_menu_name menu_name;
    WCHAR nameW[MAX_ATOM_LEN + 1];
    UNICODE_STRING name = RTL_CONSTANT_STRING(nameW), version;
    ATOM atom;

    init_class_name_ansi( &name, wc->lpszClassName );
    get_class_version( &name, &version, FALSE );

    if (!alloc_menu_nameA( &menu_name, wc->lpszMenuName )) return 0;

    atom = NtUserRegisterClassExWOW( (WNDCLASSEXW *)wc, &name, &version, &menu_name, 0, 1, NULL );
    if (!atom) free_menu_name( &menu_name );
    return atom;
}


/***********************************************************************
 *		RegisterClassExW (USER32.@)
 */
ATOM WINAPI RegisterClassExW( const WNDCLASSEXW* wc )
{
    struct client_menu_name menu_name;
    WCHAR nameW[MAX_ATOM_LEN + 1];
    UNICODE_STRING name = RTL_CONSTANT_STRING(nameW), version;
    ATOM atom;

    init_class_name( &name, wc->lpszClassName );
    get_class_version( &name, &version, FALSE );

    if (!alloc_menu_nameW( &menu_name, wc->lpszMenuName )) return 0;

    atom = NtUserRegisterClassExWOW( wc, &name, &version, &menu_name, 0, 0, NULL );
    if (!atom) free_menu_name( &menu_name );
    return atom;
}


/***********************************************************************
 *		UnregisterClassA (USER32.@)
 */
BOOL WINAPI UnregisterClassA( LPCSTR className, HINSTANCE hInstance )
{
    if (!IS_INTRESOURCE(className))
    {
        WCHAR name[MAX_ATOM_LEN + 1];

        if (!MultiByteToWideChar( CP_ACP, 0, className, -1, name, MAX_ATOM_LEN + 1 ))
            return FALSE;
        return UnregisterClassW( name, hInstance );
    }
    return UnregisterClassW( (LPCWSTR)className, hInstance );
}

/***********************************************************************
 *		UnregisterClassW (USER32.@)
 */
BOOL WINAPI UnregisterClassW( LPCWSTR class_name, HINSTANCE instance )
{
    struct client_menu_name menu_name;
    WCHAR nameW[MAX_ATOM_LEN + 1];
    UNICODE_STRING name = RTL_CONSTANT_STRING(nameW), version;
    BOOL ret;

    init_class_name( &name, class_name );
    get_class_version( &name, &version, FALSE );

    ret = NtUserUnregisterClass( &name, instance, &menu_name );
    if (ret) free_menu_name( &menu_name );
    return ret;
}


/***********************************************************************
 *		GetClassWord (USER32.@)
 */
WORD WINAPI GetClassWord( HWND hwnd, INT offset )
{
    return NtUserGetClassWord( hwnd, offset );
}


/***********************************************************************
 *		GetClassLongW (USER32.@)
 */
DWORD WINAPI GetClassLongW( HWND hwnd, INT offset )
{
    return NtUserGetClassLongW( hwnd, offset );
}



/***********************************************************************
 *		GetClassLongA (USER32.@)
 */
DWORD WINAPI GetClassLongA( HWND hwnd, INT offset )
{
    return NtUserGetClassLongA( hwnd, offset );
}


/***********************************************************************
 *		SetClassLongW (USER32.@)
 */
DWORD WINAPI SetClassLongW( HWND hwnd, INT offset, LONG newval )
{
    if (offset == GCLP_MENUNAME) return set_menu_nameW( hwnd, offset, newval );
    return NtUserSetClassLong( hwnd, offset, newval, FALSE );
}


/***********************************************************************
 *		SetClassLongA (USER32.@)
 */
DWORD WINAPI SetClassLongA( HWND hwnd, INT offset, LONG newval )
{
    if (offset == GCLP_MENUNAME) return set_menu_nameA( hwnd, offset, newval );
    return NtUserSetClassLong( hwnd, offset, newval, TRUE );
}


/***********************************************************************
 *		GetClassNameA (USER32.@)
 */
INT WINAPI GetClassNameA( HWND hwnd, LPSTR buffer, INT count )
{
    WCHAR tmpbuf[MAX_ATOM_LEN + 1];
    DWORD len;

    if (count <= 0) return 0;
    if (!GetClassNameW( hwnd, tmpbuf, ARRAY_SIZE( tmpbuf ))) return 0;
    RtlUnicodeToMultiByteN( buffer, count - 1, &len, tmpbuf, lstrlenW(tmpbuf) * sizeof(WCHAR) );
    buffer[len] = 0;
    return len;
}


/***********************************************************************
 *		GetClassNameW (USER32.@)
 */
INT WINAPI GetClassNameW( HWND hwnd, LPWSTR buffer, INT count )
{
    UNICODE_STRING name = { .Buffer = buffer, .MaximumLength = count * sizeof(WCHAR) };
    return NtUserGetClassName( hwnd, FALSE, &name );
}


/***********************************************************************
 *		RealGetWindowClassA (USER32.@)
 */
UINT WINAPI RealGetWindowClassA( HWND hwnd, LPSTR buffer, UINT count )
{
    return GetClassNameA( hwnd, buffer, count );
}


/***********************************************************************
 *		RealGetWindowClassW (USER32.@)
 */
UINT WINAPI RealGetWindowClassW( HWND hwnd, LPWSTR buffer, UINT count )
{
    return GetClassNameW( hwnd, buffer, count );
}


/***********************************************************************
 *		GetClassInfoA (USER32.@)
 */
BOOL WINAPI GetClassInfoA( HINSTANCE hInstance, LPCSTR name, WNDCLASSA *wc )
{
    WNDCLASSEXA wcex;
    UINT ret = GetClassInfoExA( hInstance, name, &wcex );

    if (ret)
    {
        wc->style         = wcex.style;
        wc->lpfnWndProc   = wcex.lpfnWndProc;
        wc->cbClsExtra    = wcex.cbClsExtra;
        wc->cbWndExtra    = wcex.cbWndExtra;
        wc->hInstance     = wcex.hInstance;
        wc->hIcon         = wcex.hIcon;
        wc->hCursor       = wcex.hCursor;
        wc->hbrBackground = wcex.hbrBackground;
        wc->lpszMenuName  = wcex.lpszMenuName;
        wc->lpszClassName = wcex.lpszClassName;
    }
    return ret;
}


/***********************************************************************
 *		GetClassInfoW (USER32.@)
 */
BOOL WINAPI GetClassInfoW( HINSTANCE hInstance, LPCWSTR name, WNDCLASSW *wc )
{
    WNDCLASSEXW wcex;
    UINT ret = GetClassInfoExW( hInstance, name, &wcex );

    if (ret)
    {
        wc->style         = wcex.style;
        wc->lpfnWndProc   = wcex.lpfnWndProc;
        wc->cbClsExtra    = wcex.cbClsExtra;
        wc->cbWndExtra    = wcex.cbWndExtra;
        wc->hInstance     = wcex.hInstance;
        wc->hIcon         = wcex.hIcon;
        wc->hCursor       = wcex.hCursor;
        wc->hbrBackground = wcex.hbrBackground;
        wc->lpszMenuName  = wcex.lpszMenuName;
        wc->lpszClassName = wcex.lpszClassName;
    }
    return ret;
}

/***********************************************************************
 *		GetClassInfoExA (USER32.@)
 */
BOOL WINAPI GetClassInfoExA( HINSTANCE hInstance, LPCSTR name, WNDCLASSEXA *wc )
{
    WCHAR nameW[MAX_ATOM_LEN + 1];
    UNICODE_STRING name_str = RTL_CONSTANT_STRING(nameW), version;
    ATOM atom;

    TRACE("%p %s %p\n", hInstance, debugstr_a(name), wc);

    if (!wc)
    {
        SetLastError( ERROR_NOACCESS );
        return FALSE;
    }

    if (!hInstance) hInstance = user32_module;
    init_class_name_ansi( &name_str, name );
    get_class_version( &name_str, &version, TRUE );

    if (!(atom = NtUserGetClassInfoEx( hInstance, &name_str, (WNDCLASSEXW *)wc, NULL, TRUE )))
    {
        TRACE( "%s %p -> not found\n", debugstr_us(&name_str), hInstance );
        SetLastError( ERROR_CLASS_DOES_NOT_EXIST );
        return 0;
    }

    wc->lpszClassName = name;
    /* We must return the atom of the class here instead of just TRUE. */
    return atom;
}


/***********************************************************************
 *		GetClassInfoExW (USER32.@)
 */
BOOL WINAPI GetClassInfoExW( HINSTANCE hInstance, LPCWSTR name, WNDCLASSEXW *wc )
{
    WCHAR nameW[MAX_ATOM_LEN + 1];
    UNICODE_STRING name_str = RTL_CONSTANT_STRING(nameW), version;
    ATOM atom;

    TRACE("%p %s %p\n", hInstance, debugstr_w(name), wc);

    if (!wc)
    {
        SetLastError( ERROR_NOACCESS );
        return FALSE;
    }

    if (!hInstance) hInstance = user32_module;
    init_class_name( &name_str, name );
    get_class_version( &name_str, &version, TRUE );

    if (!(atom = NtUserGetClassInfoEx( hInstance, &name_str, wc, NULL, FALSE )))
    {
        TRACE( "%s %p -> not found\n", debugstr_us(&name_str), hInstance );
        SetLastError( ERROR_CLASS_DOES_NOT_EXIST );
        return 0;
    }

    wc->lpszClassName = name;
    /* We must return the atom of the class here instead of just TRUE. */
    return atom;
}


#ifdef _WIN64

/* 64bit versions */

#undef GetClassLongPtrA
#undef GetClassLongPtrW
#undef SetClassLongPtrA
#undef SetClassLongPtrW

/***********************************************************************
 *		GetClassLongPtrA (USER32.@)
 */
ULONG_PTR WINAPI GetClassLongPtrA( HWND hwnd, INT offset )
{
    return NtUserGetClassLongPtrA( hwnd, offset );
}

/***********************************************************************
 *		GetClassLongPtrW (USER32.@)
 */
ULONG_PTR WINAPI GetClassLongPtrW( HWND hwnd, INT offset )
{
    return NtUserGetClassLongPtrW( hwnd, offset );
}

/***********************************************************************
 *		SetClassLongPtrW (USER32.@)
 */
ULONG_PTR WINAPI SetClassLongPtrW( HWND hwnd, INT offset, LONG_PTR newval )
{
    if (offset == GCLP_MENUNAME) return set_menu_nameW( hwnd, offset, newval );
    return NtUserSetClassLongPtr( hwnd, offset, newval, FALSE );
}

/***********************************************************************
 *		SetClassLongPtrA (USER32.@)
 */
ULONG_PTR WINAPI SetClassLongPtrA( HWND hwnd, INT offset, LONG_PTR newval )
{
    if (offset == GCLP_MENUNAME) return set_menu_nameA( hwnd, offset, newval );
    return NtUserSetClassLongPtr( hwnd, offset, newval, TRUE );
}

#endif /* _WIN64 */
