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

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"
#include "win.h"
#include "user_private.h"
#include "controls.h"
#include "wine/server.h"
#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(class);

#define MAX_ATOM_LEN 255 /* from dlls/kernel32/atom.c */

static struct list class_list = LIST_INIT( class_list );
static INIT_ONCE init_once = INIT_ONCE_STATIC_INIT;

#define CLASS_OTHER_PROCESS ((CLASS *)1)

static inline const char *debugstr_us( const UNICODE_STRING *us )
{
    if (!us) return "<null>";
    return debugstr_wn( us->Buffer, us->Length / sizeof(WCHAR) );
}

/***********************************************************************
 *           get_class_ptr
 */
static CLASS *get_class_ptr( HWND hwnd, BOOL write_access )
{
    WND *ptr = WIN_GetPtr( hwnd );

    if (ptr)
    {
        if (ptr != WND_OTHER_PROCESS && ptr != WND_DESKTOP) return ptr->class;
        if (!write_access) return CLASS_OTHER_PROCESS;

        /* modifying classes in other processes is not allowed */
        if (ptr == WND_DESKTOP || IsWindow( hwnd ))
        {
            SetLastError( ERROR_ACCESS_DENIED );
            return NULL;
        }
    }
    SetLastError( ERROR_INVALID_WINDOW_HANDLE );
    return NULL;
}


/***********************************************************************
 *           release_class_ptr
 */
static inline void release_class_ptr( CLASS *ptr )
{
    USER_Unlock();
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
        if (ret > 0xffff) return 0;
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

/***********************************************************************
 *           set_server_info
 *
 * Set class info with the wine server.
 */
static BOOL set_server_info( HWND hwnd, INT offset, LONG_PTR newval, UINT size )
{
    BOOL ret;

    SERVER_START_REQ( set_class_info )
    {
        req->window = wine_server_user_handle( hwnd );
        req->extra_offset = -1;
        switch(offset)
        {
        case GCW_ATOM:
            req->flags = SET_CLASS_ATOM;
            req->atom = LOWORD(newval);
            break;
        case GCL_STYLE:
            req->flags = SET_CLASS_STYLE;
            req->style = newval;
            break;
        case GCL_CBWNDEXTRA:
            req->flags = SET_CLASS_WINEXTRA;
            req->win_extra = newval;
            break;
        case GCLP_HMODULE:
            req->flags = SET_CLASS_INSTANCE;
            req->instance = wine_server_client_ptr( (void *)newval );
            break;
        default:
            assert( offset >= 0 );
            req->flags = SET_CLASS_EXTRA;
            req->extra_offset = offset;
            req->extra_size = size;
            if ( size == sizeof(LONG) )
            {
                LONG newlong = newval;
                memcpy( &req->extra_value, &newlong, sizeof(LONG) );
            }
            else
                memcpy( &req->extra_value, &newval, sizeof(LONG_PTR) );
            break;
        }
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


static void init_class_name( UNICODE_STRING *str, const WCHAR *name )
{
    if (IS_INTRESOURCE( name ))
    {
        str->Buffer = (WCHAR *)name;
        str->Length = str->MaximumLength = 0;
    }
    else RtlInitUnicodeString( str, name );
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

/***********************************************************************
 *           CLASS_SetMenuNameA
 *
 * Set the menu name in a class structure by copying the string.
 */
static void CLASS_SetMenuNameA( CLASS *classPtr, LPCSTR name )
{
    struct client_menu_name menu_name;
    if (!alloc_menu_nameA( &menu_name, name )) return;
    free_menu_name( &classPtr->menu_name );
    classPtr->menu_name = menu_name;
}

/***********************************************************************
 *           CLASS_SetMenuNameW
 *
 * Set the menu name in a class structure by copying the string.
 */
static void CLASS_SetMenuNameW( CLASS *classPtr, LPCWSTR name )
{
    struct client_menu_name menu_name;
    if (!alloc_menu_nameW( &menu_name, name )) return;
    free_menu_name( &classPtr->menu_name );
    classPtr->menu_name = menu_name;
}


/***********************************************************************
 *           CLASS_FreeClass
 *
 * Free a class structure.
 */
static void CLASS_FreeClass( CLASS *classPtr )
{
    TRACE("%p\n", classPtr);

    USER_Lock();

    if (classPtr->dce) free_dce( classPtr->dce, 0 );
    list_remove( &classPtr->entry );
    if (classPtr->hbrBackground > (HBRUSH)(COLOR_GRADIENTINACTIVECAPTION + 1))
        DeleteObject( classPtr->hbrBackground );
    DestroyIcon( classPtr->hIconSmIntern );
    free_menu_name( &classPtr->menu_name );
    HeapFree( GetProcessHeap(), 0, classPtr );
    USER_Unlock();
}

static CLASS *find_class( HINSTANCE module, UNICODE_STRING *name )
{
    ATOM atom = get_int_atom_value( name );
    UINT_PTR instance = (UINT_PTR)module & ~0xffff;
    CLASS *class;

    USER_Lock();
    LIST_FOR_EACH_ENTRY( class, &class_list, CLASS, entry )
    {
        if (atom)
        {
            if (class->atomName != atom) continue;
        }
        else
        {
            if (wcsnicmp( class->name, name->Buffer, name->Length / sizeof(WCHAR) )) continue;
        }
        if (!class->local || !module || (class->instance & ~0xffff) == instance)
        {
            TRACE( "%s %Ix -> %p\n", debugstr_us(name), instance, class );
            return class;
        }
    }
    USER_Unlock();
    return NULL;
}

static void get_versioned_name( const WCHAR *name, UNICODE_STRING *ret, UNICODE_STRING *version, HMODULE *reg_module )
{
    ACTCTX_SECTION_KEYED_DATA data;
    struct wndclass_redirect_data
    {
        ULONG size;
        DWORD res;
        ULONG name_len;
        ULONG name_offset;
        ULONG module_len;
        ULONG module_offset;
    } *wndclass;
    const WCHAR *module, *ptr;
    UNICODE_STRING name_us;
    HMODULE hmod;
    UINT offset = 0;

    if (reg_module) *reg_module = 0;
    if (version) version->Length = 0;

    if (IS_INTRESOURCE( name ) || is_comctl32_class( name ) || is_builtin_class( name ))
    {
        init_class_name( ret, name );
        return;
    }

    data.cbSize = sizeof(data);
    RtlInitUnicodeString(&name_us, name);
    if (RtlFindActivationContextSectionString( 0, NULL, ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION,
                                               &name_us, &data ))
    {
        init_class_name( ret, name );
        return;
    }

    wndclass = (struct wndclass_redirect_data *)data.lpData;
    offset = wndclass->name_len / sizeof(WCHAR) - lstrlenW(name);

    module = (const WCHAR *)((BYTE *)data.lpSectionBase + wndclass->module_offset);
    if (!(hmod = GetModuleHandleW( module )))
        hmod = LoadLibraryW( module );

    /* Combined name is used to register versioned class name. Base name part will match exactly
       original class name and won't be reused from context data. */
    ptr = (const WCHAR *)((BYTE *)wndclass + wndclass->name_offset);
    if (version)
    {
        WCHAR *combined = version->Buffer;
        memcpy( combined, ptr, offset * sizeof(WCHAR) );
        lstrcpyW( &combined[offset], name );
        version->Length = offset * sizeof(WCHAR);
        ptr = combined;
    }

    if (reg_module) *reg_module = hmod;
    init_class_name( ret, ptr );
}

/***********************************************************************
 *           CLASS_RegisterClass
 *
 * The real RegisterClass() functionality.
 */
static CLASS *CLASS_RegisterClass( UNICODE_STRING *name, UINT basename_offset, HINSTANCE hInstance,
                                   BOOL local, DWORD style, INT classExtra, INT winExtra )
{
    CLASS *classPtr;
    BOOL ret;

    TRACE("name=%s hinst=%p style=0x%x clExtr=0x%x winExtr=0x%x\n",
          debugstr_us(name), hInstance, style, classExtra, winExtra );

    /* Fix the extra bytes value */

    if (classExtra > 40)  /* Extra bytes are limited to 40 in Win32 */
        WARN("Class extra bytes %d is > 40\n", classExtra);
    if (winExtra > 40)    /* Extra bytes are limited to 40 in Win32 */
        WARN("Win extra bytes %d is > 40\n", winExtra );

    classPtr = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CLASS) + classExtra );
    if (!classPtr) return NULL;

    classPtr->atomName = get_int_atom_value( name );
    classPtr->basename = classPtr->name;
    if (!classPtr->atomName && name)
    {
        memcpy( classPtr->name, name->Buffer, name->Length );
        classPtr->name[name->Length / sizeof(WCHAR)] = 0;
        classPtr->basename += basename_offset;
    }
    else GlobalGetAtomNameW( classPtr->atomName, classPtr->name, ARRAY_SIZE( classPtr->name ));

    SERVER_START_REQ( create_class )
    {
        req->local      = local;
        req->style      = style;
        req->instance   = wine_server_client_ptr( hInstance );
        req->extra      = classExtra;
        req->win_extra  = winExtra;
        req->client_ptr = wine_server_client_ptr( classPtr );
        req->atom       = classPtr->atomName;
        req->name_offset = basename_offset;
        if (!req->atom && name) wine_server_add_data( req, name->Buffer, name->Length );
        ret = !wine_server_call_err( req );
        classPtr->atomName = reply->atom;
    }
    SERVER_END_REQ;
    if (!ret)
    {
        HeapFree( GetProcessHeap(), 0, classPtr );
        return NULL;
    }

    classPtr->style       = style;
    classPtr->local       = local;
    classPtr->cbWndExtra  = winExtra;
    classPtr->cbClsExtra  = classExtra;
    classPtr->instance    = (UINT_PTR)hInstance;

    /* Other non-null values must be set by caller */

    USER_Lock();
    if (local) list_add_head( &class_list, &classPtr->entry );
    else list_add_tail( &class_list, &classPtr->entry );
    return classPtr;
}


/***********************************************************************
 *           register_builtin
 *
 * Register a builtin control class.
 * This allows having both ANSI and Unicode winprocs for the same class.
 */
static void register_builtin( const struct builtin_class_descr *descr )
{
    UNICODE_STRING name;
    CLASS *classPtr;
    HCURSOR cursor = 0;

    if (descr->cursor) cursor = LoadCursorA( 0, (LPSTR)descr->cursor );

    init_class_name( &name, descr->name );
    if (!(classPtr = CLASS_RegisterClass( &name, 0, user32_module, FALSE,
                                          descr->style, 0, descr->extra )))
    {
        if (cursor) DestroyCursor( cursor );
        return;
    }

    classPtr->hCursor       = cursor;
    classPtr->hbrBackground = descr->brush;
    classPtr->winproc       = BUILTIN_WINPROC( descr->proc );
    release_class_ptr( classPtr );
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
 *           register_builtins
 */
static BOOL WINAPI register_builtins( INIT_ONCE *once, void *param, void **context )
{
    register_builtin( &BUTTON_builtin_class );
    register_builtin( &COMBO_builtin_class );
    register_builtin( &COMBOLBOX_builtin_class );
    register_builtin( &DIALOG_builtin_class );
    register_builtin( &EDIT_builtin_class );
    register_builtin( &ICONTITLE_builtin_class );
    register_builtin( &LISTBOX_builtin_class );
    register_builtin( &MDICLIENT_builtin_class );
    register_builtin( &MENU_builtin_class );
    register_builtin( &SCROLL_builtin_class );
    register_builtin( &STATIC_builtin_class );
    register_builtin( &IME_builtin_class );

    /* Load uxtheme.dll so that standard scrollbars and dialogs are hooked for theming support */
    load_uxtheme();
    return TRUE;
}


/***********************************************************************
 *           register_builtin_classes
 */
void register_builtin_classes(void)
{
    InitOnceExecuteOnce( &init_once, register_builtins, NULL, NULL );
}


/***********************************************************************
 *           register_desktop_class
 */
void register_desktop_class(void)
{
    register_builtin( &DESKTOP_builtin_class );
    register_builtin( &MESSAGE_builtin_class );
}


/***********************************************************************
 *           get_class_winproc
 */
WNDPROC get_class_winproc( CLASS *class )
{
    return class->winproc;
}


/***********************************************************************
 *           get_class_dce
 */
struct dce *get_class_dce( CLASS *class )
{
    return class->dce;
}


/***********************************************************************
 *           set_class_dce
 */
struct dce *set_class_dce( CLASS *class, struct dce *dce )
{
    if (class->dce) return class->dce;  /* already set, don't change it */
    class->dce = dce;
    return dce;
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
    WCHAR nameW[MAX_ATOM_LEN + 1], combined[MAX_ATOM_LEN + 1];
    struct client_menu_name menu_name;
    UNICODE_STRING name, version;
    ATOM atom;
    CLASS *classPtr;
    HINSTANCE instance;

    GetDesktopWindow();  /* create the desktop window to trigger builtin class registration */

    if (wc->cbSize != sizeof(*wc) || wc->cbClsExtra < 0 || wc->cbWndExtra < 0 ||
        wc->hInstance == user32_module)  /* we can't register a class for user32 */
    {
         SetLastError( ERROR_INVALID_PARAMETER );
         return 0;
    }
    if (!(instance = wc->hInstance)) instance = GetModuleHandleW( NULL );

    version.Buffer = combined;
    version.MaximumLength = sizeof(combined);
    if (!IS_INTRESOURCE(wc->lpszClassName))
    {
        if (!MultiByteToWideChar( CP_ACP, 0, wc->lpszClassName, -1, nameW, MAX_ATOM_LEN + 1 )) return 0;
        get_versioned_name( nameW, &name, &version, FALSE );
    }
    else
    {
        init_class_name( &name, (const WCHAR *)wc->lpszClassName );
        version.Length = 0;
    }

    if (!alloc_menu_nameA( &menu_name, wc->lpszMenuName )) return 0;

    classPtr = CLASS_RegisterClass( &name, version.Length / sizeof(WCHAR), instance, !(wc->style & CS_GLOBALCLASS),
                                    wc->style, wc->cbClsExtra, wc->cbWndExtra );
    if (!classPtr) return 0;
    atom = classPtr->atomName;

    TRACE( "name=%s->%s atom=%04x wndproc=%p hinst=%p bg=%p style=%08x clsExt=%d winExt=%d class=%p\n",
           debugstr_a(wc->lpszClassName), debugstr_us(&name), atom, wc->lpfnWndProc,
           instance, wc->hbrBackground,  wc->style, wc->cbClsExtra, wc->cbWndExtra, classPtr );

    classPtr->hIcon         = wc->hIcon;
    classPtr->hIconSm       = wc->hIconSm;
    classPtr->hIconSmIntern = wc->hIcon && !wc->hIconSm ?
                                            CopyImage( wc->hIcon, IMAGE_ICON,
                                                GetSystemMetrics( SM_CXSMICON ),
                                                GetSystemMetrics( SM_CYSMICON ),
                                                LR_COPYFROMRESOURCE ) : NULL;
    classPtr->hCursor       = wc->hCursor;
    classPtr->hbrBackground = wc->hbrBackground;
    classPtr->winproc       = WINPROC_AllocProc( wc->lpfnWndProc, FALSE );
    classPtr->menu_name     = menu_name;
    release_class_ptr( classPtr );
    return atom;
}


/***********************************************************************
 *		RegisterClassExW (USER32.@)
 */
ATOM WINAPI RegisterClassExW( const WNDCLASSEXW* wc )
{
    WCHAR combined[MAX_ATOM_LEN + 1];
    struct client_menu_name menu_name;
    UNICODE_STRING name, version;
    ATOM atom;
    CLASS *classPtr;
    HINSTANCE instance;

    GetDesktopWindow();  /* create the desktop window to trigger builtin class registration */

    if (wc->cbSize != sizeof(*wc) || wc->cbClsExtra < 0 || wc->cbWndExtra < 0 ||
        wc->hInstance == user32_module)  /* we can't register a class for user32 */
    {
         SetLastError( ERROR_INVALID_PARAMETER );
         return 0;
    }
    if (!(instance = wc->hInstance)) instance = GetModuleHandleW( NULL );

    if (!alloc_menu_nameW( &menu_name, wc->lpszMenuName )) return 0;

    version.Buffer = combined;
    version.MaximumLength = sizeof(combined);
    get_versioned_name( wc->lpszClassName, &name, &version, FALSE );
    if (!(classPtr = CLASS_RegisterClass( &name, version.Length / sizeof(WCHAR), instance,
                                          !(wc->style & CS_GLOBALCLASS),
                                          wc->style, wc->cbClsExtra, wc->cbWndExtra )))
    {
        free_menu_name( &menu_name );
        return 0;
    }

    atom = classPtr->atomName;

    TRACE( "name=%s->%s atom=%04x wndproc=%p hinst=%p bg=%p style=%08x clsExt=%d winExt=%d class=%p\n",
           debugstr_w(wc->lpszClassName), debugstr_us(&name), atom, wc->lpfnWndProc, instance,
           wc->hbrBackground, wc->style, wc->cbClsExtra, wc->cbWndExtra, classPtr );

    classPtr->hIcon         = wc->hIcon;
    classPtr->hIconSm       = wc->hIconSm;
    classPtr->hIconSmIntern = wc->hIcon && !wc->hIconSm ?
                                            CopyImage( wc->hIcon, IMAGE_ICON,
                                                GetSystemMetrics( SM_CXSMICON ),
                                                GetSystemMetrics( SM_CYSMICON ),
                                                LR_COPYFROMRESOURCE ) : NULL;
    classPtr->hCursor       = wc->hCursor;
    classPtr->hbrBackground = wc->hbrBackground;
    classPtr->winproc       = WINPROC_AllocProc( wc->lpfnWndProc, TRUE );
    CLASS_SetMenuNameW( classPtr, wc->lpszMenuName );
    release_class_ptr( classPtr );
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
BOOL WINAPI UnregisterClassW( LPCWSTR className, HINSTANCE hInstance )
{
    CLASS *classPtr = NULL;
    UNICODE_STRING name;

    GetDesktopWindow();  /* create the desktop window to trigger builtin class registration */

    get_versioned_name( className, &name, NULL, FALSE );
    SERVER_START_REQ( destroy_class )
    {
        req->instance = wine_server_client_ptr( hInstance );
        if (!(req->atom = get_int_atom_value(&name)) && name.Length)
            wine_server_add_data( req, name.Buffer, name.Length );
        if (!wine_server_call_err( req )) classPtr = wine_server_get_ptr( reply->client_ptr );
    }
    SERVER_END_REQ;

    if (classPtr) CLASS_FreeClass( classPtr );
    return (classPtr != NULL);
}


/***********************************************************************
 *		GetClassWord (USER32.@)
 */
WORD WINAPI GetClassWord( HWND hwnd, INT offset )
{
    CLASS *class;
    WORD retvalue = 0;

    if (offset < 0) return GetClassLongA( hwnd, offset );

    if (!(class = get_class_ptr( hwnd, FALSE ))) return 0;

    if (class == CLASS_OTHER_PROCESS)
    {
        SERVER_START_REQ( set_class_info )
        {
            req->window = wine_server_user_handle( hwnd );
            req->flags = 0;
            req->extra_offset = offset;
            req->extra_size = sizeof(retvalue);
            if (!wine_server_call_err( req ))
                memcpy( &retvalue, &reply->old_extra_value, sizeof(retvalue) );
        }
        SERVER_END_REQ;
        return retvalue;
    }

    if (offset <= class->cbClsExtra - sizeof(WORD))
        memcpy( &retvalue, (char *)(class + 1) + offset, sizeof(retvalue) );
    else
        SetLastError( ERROR_INVALID_INDEX );
    release_class_ptr( class );
    return retvalue;
}


/***********************************************************************
 *             CLASS_GetClassLong
 *
 * Implementation of GetClassLong(Ptr)A/W
 */
static ULONG_PTR CLASS_GetClassLong( HWND hwnd, INT offset, UINT size,
                                     BOOL unicode )
{
    CLASS *class;
    ULONG_PTR retvalue = 0;

    if (!(class = get_class_ptr( hwnd, FALSE ))) return 0;

    if (class == CLASS_OTHER_PROCESS)
    {
        SERVER_START_REQ( set_class_info )
        {
            req->window = wine_server_user_handle( hwnd );
            req->flags = 0;
            req->extra_offset = (offset >= 0) ? offset : -1;
            req->extra_size = (offset >= 0) ? size : 0;
            if (!wine_server_call_err( req ))
            {
                switch(offset)
                {
                case GCLP_HBRBACKGROUND:
                case GCLP_HCURSOR:
                case GCLP_HICON:
                case GCLP_HICONSM:
                case GCLP_WNDPROC:
                case GCLP_MENUNAME:
                    FIXME( "offset %d (%s) not supported on other process window %p\n",
			   offset, SPY_GetClassLongOffsetName(offset), hwnd );
                    SetLastError( ERROR_INVALID_HANDLE );
                    break;
                case GCL_STYLE:
                    retvalue = reply->old_style;
                    break;
                case GCL_CBWNDEXTRA:
                    retvalue = reply->old_win_extra;
                    break;
                case GCL_CBCLSEXTRA:
                    retvalue = reply->old_extra;
                    break;
                case GCLP_HMODULE:
                    retvalue = (ULONG_PTR)wine_server_get_ptr( reply->old_instance );
                    break;
                case GCW_ATOM:
                    retvalue = reply->old_atom;
                    break;
                default:
                    if (offset >= 0)
                    {
                        if (size == sizeof(DWORD))
                        {
                            DWORD retdword;
                            memcpy( &retdword, &reply->old_extra_value, sizeof(DWORD) );
                            retvalue = retdword;
                        }
                        else
                            memcpy( &retvalue, &reply->old_extra_value,
                                    sizeof(ULONG_PTR) );
                    }
                    else SetLastError( ERROR_INVALID_INDEX );
                    break;
                }
            }
        }
        SERVER_END_REQ;
        return retvalue;
    }

    if (offset >= 0)
    {
        if (offset <= class->cbClsExtra - size)
        {
            if (size == sizeof(DWORD))
            {
                DWORD retdword;
                memcpy( &retdword, (char *)(class + 1) + offset, sizeof(DWORD) );
                retvalue = retdword;
            }
            else
                memcpy( &retvalue, (char *)(class + 1) + offset, sizeof(ULONG_PTR) );
        }
        else
            SetLastError( ERROR_INVALID_INDEX );
        release_class_ptr( class );
        return retvalue;
    }

    switch(offset)
    {
    case GCLP_HBRBACKGROUND:
        retvalue = (ULONG_PTR)class->hbrBackground;
        break;
    case GCLP_HCURSOR:
        retvalue = (ULONG_PTR)class->hCursor;
        break;
    case GCLP_HICON:
        retvalue = (ULONG_PTR)class->hIcon;
        break;
    case GCLP_HICONSM:
        retvalue = (ULONG_PTR)(class->hIconSm ? class->hIconSm : class->hIconSmIntern);
        break;
    case GCL_STYLE:
        retvalue = class->style;
        break;
    case GCL_CBWNDEXTRA:
        retvalue = class->cbWndExtra;
        break;
    case GCL_CBCLSEXTRA:
        retvalue = class->cbClsExtra;
        break;
    case GCLP_HMODULE:
        retvalue = class->instance;
        break;
    case GCLP_WNDPROC:
        retvalue = (ULONG_PTR)WINPROC_GetProc( class->winproc, unicode );
        break;
    case GCLP_MENUNAME:
        if (unicode)
            retvalue = (ULONG_PTR)class->menu_name.nameW;
        else
            retvalue = (ULONG_PTR)class->menu_name.nameA;
        break;
    case GCW_ATOM:
        retvalue = class->atomName;
        break;
    default:
        SetLastError( ERROR_INVALID_INDEX );
        break;
    }
    release_class_ptr( class );
    return retvalue;
}


/***********************************************************************
 *		GetClassLongW (USER32.@)
 */
DWORD WINAPI GetClassLongW( HWND hwnd, INT offset )
{
    return CLASS_GetClassLong( hwnd, offset, sizeof(DWORD), TRUE );
}



/***********************************************************************
 *		GetClassLongA (USER32.@)
 */
DWORD WINAPI GetClassLongA( HWND hwnd, INT offset )
{
    return CLASS_GetClassLong( hwnd, offset, sizeof(DWORD), FALSE );
}


/***********************************************************************
 *		SetClassWord (USER32.@)
 */
WORD WINAPI SetClassWord( HWND hwnd, INT offset, WORD newval )
{
    CLASS *class;
    WORD retval = 0;

    if (offset < 0) return SetClassLongA( hwnd, offset, (DWORD)newval );

    if (!(class = get_class_ptr( hwnd, TRUE ))) return 0;

    SERVER_START_REQ( set_class_info )
    {
        req->window = wine_server_user_handle( hwnd );
        req->flags = SET_CLASS_EXTRA;
        req->extra_offset = offset;
        req->extra_size = sizeof(newval);
        memcpy( &req->extra_value, &newval, sizeof(newval) );
        if (!wine_server_call_err( req ))
        {
            void *ptr = (char *)(class + 1) + offset;
            memcpy( &retval, ptr, sizeof(retval) );
            memcpy( ptr, &newval, sizeof(newval) );
        }
    }
    SERVER_END_REQ;
    release_class_ptr( class );
    return retval;
}


/***********************************************************************
 *             CLASS_SetClassLong
 *
 * Implementation of SetClassLong(Ptr)A/W
 */
static ULONG_PTR CLASS_SetClassLong( HWND hwnd, INT offset, LONG_PTR newval,
                                     UINT size, BOOL unicode )
{
    CLASS *class;
    ULONG_PTR retval = 0;

    if (!(class = get_class_ptr( hwnd, TRUE ))) return 0;

    if (offset >= 0)
    {
        if (set_server_info( hwnd, offset, newval, size ))
        {
            void *ptr = (char *)(class + 1) + offset;
            if ( size == sizeof(LONG) )
            {
                DWORD retdword;
                LONG newlong = newval;
                memcpy( &retdword, ptr, sizeof(DWORD) );
                memcpy( ptr, &newlong, sizeof(LONG) );
                retval = retdword;
            }
            else
            {
                memcpy( &retval, ptr, sizeof(ULONG_PTR) );
                memcpy( ptr, &newval, sizeof(LONG_PTR) );
            }
        }
    }
    else switch(offset)
    {
    case GCLP_MENUNAME:
        if ( unicode )
            CLASS_SetMenuNameW( class, (LPCWSTR)newval );
        else
            CLASS_SetMenuNameA( class, (LPCSTR)newval );
        retval = 0;  /* Old value is now meaningless anyway */
        break;
    case GCLP_WNDPROC:
        retval = (ULONG_PTR)WINPROC_GetProc( class->winproc, unicode );
        class->winproc = WINPROC_AllocProc( (WNDPROC)newval, unicode );
        break;
    case GCLP_HBRBACKGROUND:
        retval = (ULONG_PTR)class->hbrBackground;
        class->hbrBackground = (HBRUSH)newval;
        break;
    case GCLP_HCURSOR:
        retval = (ULONG_PTR)class->hCursor;
        class->hCursor = (HCURSOR)newval;
        break;
    case GCLP_HICON:
        retval = (ULONG_PTR)class->hIcon;
        if (class->hIconSmIntern)
        {
            DestroyIcon(class->hIconSmIntern);
            class->hIconSmIntern = NULL;
        }
        if (newval && !class->hIconSm)
            class->hIconSmIntern = CopyImage( (HICON)newval, IMAGE_ICON,
                      GetSystemMetrics( SM_CXSMICON ), GetSystemMetrics( SM_CYSMICON ),
                      LR_COPYFROMRESOURCE );
        class->hIcon = (HICON)newval;
        break;
    case GCLP_HICONSM:
        retval = (ULONG_PTR)class->hIconSm;
        if (retval && !newval && class->hIcon)
            class->hIconSmIntern = CopyImage( class->hIcon, IMAGE_ICON,
                      GetSystemMetrics( SM_CXSMICON ), GetSystemMetrics( SM_CYSMICON ),
                      LR_COPYFROMRESOURCE );
        else if (newval && class->hIconSmIntern)
        {
            DestroyIcon(class->hIconSmIntern);
            class->hIconSmIntern = NULL;
        }
        class->hIconSm = (HICON)newval;
        break;
    case GCL_STYLE:
        if (!set_server_info( hwnd, offset, newval, size )) break;
        retval = class->style;
        class->style = newval;
        break;
    case GCL_CBWNDEXTRA:
        if (!set_server_info( hwnd, offset, newval, size )) break;
        retval = class->cbWndExtra;
        class->cbWndExtra = newval;
        break;
    case GCLP_HMODULE:
        if (!set_server_info( hwnd, offset, newval, size )) break;
        retval = class->instance;
        class->instance = newval;
        break;
    case GCW_ATOM:
        if (!set_server_info( hwnd, offset, newval, size )) break;
        retval = class->atomName;
        class->atomName = newval;
        GlobalGetAtomNameW( newval, class->name, ARRAY_SIZE( class->name ));
        break;
    case GCL_CBCLSEXTRA:  /* cannot change this one */
        SetLastError( ERROR_INVALID_PARAMETER );
        break;
    default:
        SetLastError( ERROR_INVALID_INDEX );
        break;
    }
    release_class_ptr( class );
    return retval;
}


/***********************************************************************
 *		SetClassLongW (USER32.@)
 */
DWORD WINAPI SetClassLongW( HWND hwnd, INT offset, LONG newval )
{
    return CLASS_SetClassLong( hwnd, offset, newval, sizeof(LONG), TRUE );
}


/***********************************************************************
 *		SetClassLongA (USER32.@)
 */
DWORD WINAPI SetClassLongA( HWND hwnd, INT offset, LONG newval )
{
    return CLASS_SetClassLong( hwnd, offset, newval, sizeof(LONG), FALSE );
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

ATOM get_class_info( HINSTANCE instance, const WCHAR *class_name, WNDCLASSEXW *info,
                     UNICODE_STRING *name_str, BOOL ansi )
{
    UNICODE_STRING name;
    HMODULE module;
    CLASS *class;
    ATOM atom;

    get_versioned_name( class_name, &name, NULL, &module );

    if (!name_str && !instance) instance = user32_module;

    if (name.Buffer != (LPCWSTR)DESKTOP_CLASS_ATOM &&
        (IS_INTRESOURCE(name.Buffer) || name.Length != sizeof(L"Message") ||
         wcsnicmp( name.Buffer, L"Message", ARRAYSIZE(L"Message") )))
        GetDesktopWindow();  /* create the desktop window to trigger builtin class registration */

    while (class_name && !(class = find_class( instance, &name )))
    {
        if (module)
        {
            BOOL (WINAPI *pRegisterClassNameW)( const WCHAR *class );
            pRegisterClassNameW = (void *)GetProcAddress( module, "RegisterClassNameW" );
            module = NULL;
            if (pRegisterClassNameW)
            {
                TRACE( "registering %s\n", debugstr_us(&name) );
                pRegisterClassNameW( class_name );
                continue;
            }
        }
        if (IS_INTRESOURCE( class_name )) break;
        if (!is_comctl32_class( class_name )) break;
        if (GetModuleHandleW( L"comctl32.dll" )) break;
        if (!LoadLibraryW( L"comctl32.dll" )) break;
        TRACE( "%s retrying after loading comctl32\n", debugstr_w(class_name) );
    }

    if (!class)
    {
        TRACE("%s %p -> not found\n", debugstr_w(class_name), instance);
        SetLastError( ERROR_CLASS_DOES_NOT_EXIST );
        return 0;
    }

    if (info)
    {
        info->style         = class->style;
        info->lpfnWndProc   = WINPROC_GetProc( class->winproc, !ansi );
        info->cbClsExtra    = class->cbClsExtra;
        info->cbWndExtra    = class->cbWndExtra;
        info->hInstance     = (instance == user32_module) ? 0 : instance;
        info->hIcon         = class->hIcon;
        info->hIconSm       = class->hIconSm ? class->hIconSm : class->hIconSmIntern;
        info->hCursor       = class->hCursor;
        info->hbrBackground = class->hbrBackground;
        info->lpszMenuName  = ansi ? (const WCHAR *)class->menu_name.nameA : class->menu_name.nameW;
        info->lpszClassName = class_name;
    }

    if (name_str) *name_str = name;
    atom = class->atomName;
    release_class_ptr( class );
    return atom;
}

/***********************************************************************
 *		GetClassInfoExA (USER32.@)
 */
BOOL WINAPI GetClassInfoExA( HINSTANCE hInstance, LPCSTR name, WNDCLASSEXA *wc )
{
    ATOM atom;

    TRACE("%p %s %p\n", hInstance, debugstr_a(name), wc);

    if (!wc)
    {
        SetLastError( ERROR_NOACCESS );
        return FALSE;
    }

    if (!IS_INTRESOURCE(name))
    {
        WCHAR nameW[MAX_ATOM_LEN + 1];
        if (!MultiByteToWideChar( CP_ACP, 0, name, -1, nameW, ARRAY_SIZE( nameW )))
            return FALSE;
        atom = get_class_info( hInstance, nameW, (WNDCLASSEXW *)wc, NULL, TRUE );
    }
    else atom = get_class_info( hInstance, (const WCHAR *)name, (WNDCLASSEXW *)wc, NULL, TRUE );
    if (atom) wc->lpszClassName = name;

    /* We must return the atom of the class here instead of just TRUE. */
    return atom;
}


/***********************************************************************
 *		GetClassInfoExW (USER32.@)
 */
BOOL WINAPI GetClassInfoExW( HINSTANCE hInstance, LPCWSTR name, WNDCLASSEXW *wc )
{
    TRACE("%p %s %p\n", hInstance, debugstr_w(name), wc);

    if (!wc)
    {
        SetLastError( ERROR_NOACCESS );
        return FALSE;
    }

    return get_class_info( hInstance, name, wc, NULL, FALSE );
}


#if 0  /* toolhelp is in kernel, so this cannot work */

/***********************************************************************
 *		ClassFirst (TOOLHELP.69)
 */
BOOL16 WINAPI ClassFirst16( CLASSENTRY *pClassEntry )
{
    TRACE("%p\n",pClassEntry);
    pClassEntry->wNext = 1;
    return ClassNext16( pClassEntry );
}


/***********************************************************************
 *		ClassNext (TOOLHELP.70)
 */
BOOL16 WINAPI ClassNext16( CLASSENTRY *pClassEntry )
{
    int i;
    CLASS *class = firstClass;

    TRACE("%p\n",pClassEntry);

    if (!pClassEntry->wNext) return FALSE;
    for (i = 1; (i < pClassEntry->wNext) && class; i++) class = class->next;
    if (!class)
    {
        pClassEntry->wNext = 0;
        return FALSE;
    }
    pClassEntry->hInst = class->hInstance;
    pClassEntry->wNext++;
    GlobalGetAtomNameA( class->atomName, pClassEntry->szClassName,
                          sizeof(pClassEntry->szClassName) );
    return TRUE;
}
#endif

/* 64bit versions */

#ifdef GetClassLongPtrA
#undef GetClassLongPtrA
#endif

#ifdef GetClassLongPtrW
#undef GetClassLongPtrW
#endif

#ifdef SetClassLongPtrA
#undef SetClassLongPtrA
#endif

#ifdef SetClassLongPtrW
#undef SetClassLongPtrW
#endif

/***********************************************************************
 *		GetClassLongPtrA (USER32.@)
 */
ULONG_PTR WINAPI GetClassLongPtrA( HWND hwnd, INT offset )
{
    return CLASS_GetClassLong( hwnd, offset, sizeof(ULONG_PTR), FALSE );
}

/***********************************************************************
 *		GetClassLongPtrW (USER32.@)
 */
ULONG_PTR WINAPI GetClassLongPtrW( HWND hwnd, INT offset )
{
    return CLASS_GetClassLong( hwnd, offset, sizeof(ULONG_PTR), TRUE );
}

/***********************************************************************
 *		SetClassLongPtrW (USER32.@)
 */
ULONG_PTR WINAPI SetClassLongPtrW( HWND hwnd, INT offset, LONG_PTR newval )
{
    return CLASS_SetClassLong( hwnd, offset, newval, sizeof(LONG_PTR), TRUE );
}

/***********************************************************************
 *		SetClassLongPtrA (USER32.@)
 */
ULONG_PTR WINAPI SetClassLongPtrA( HWND hwnd, INT offset, LONG_PTR newval )
{
    return CLASS_SetClassLong( hwnd, offset, newval, sizeof(LONG_PTR), FALSE );
}
