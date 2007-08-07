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

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "wine/unicode.h"
#include "win.h"
#include "user_private.h"
#include "controls.h"
#include "wine/server.h"
#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(class);

typedef struct tagCLASS
{
    struct list      entry;         /* Entry in class list */
    UINT             style;         /* Class style */
    BOOL             local;         /* Local class? */
    WNDPROC          winproc;       /* Window procedure */
    INT              cbClsExtra;    /* Class extra bytes */
    INT              cbWndExtra;    /* Window extra bytes */
    LPWSTR           menuName;      /* Default menu name (Unicode followed by ASCII) */
    HINSTANCE        hInstance;     /* Module that created the task */
    HICON            hIcon;         /* Default icon */
    HICON            hIconSm;       /* Default small icon */
    HCURSOR          hCursor;       /* Default cursor */
    HBRUSH           hbrBackground; /* Default background */
    ATOM             atomName;      /* Name of the class */
} CLASS;

static struct list class_list = LIST_INIT( class_list );

#define CLASS_OTHER_PROCESS ((CLASS *)1)
#define MAX_ATOM_LEN 255 /* from dlls/kernel32/atom.c */

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
 *           set_server_info
 *
 * Set class info with the wine server.
 */
static BOOL set_server_info( HWND hwnd, INT offset, LONG_PTR newval, UINT size )
{
    BOOL ret;

    SERVER_START_REQ( set_class_info )
    {
        req->window = hwnd;
        req->extra_offset = -1;
        switch(offset)
        {
        case GCW_ATOM:
            req->flags = SET_CLASS_ATOM;
            req->atom = newval;
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
            req->instance = (void *)newval;
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


/***********************************************************************
 *           CLASS_GetMenuNameA
 *
 * Get the menu name as a ASCII string.
 */
static inline LPSTR CLASS_GetMenuNameA( CLASS *classPtr )
{
    if (!HIWORD(classPtr->menuName)) return (LPSTR)classPtr->menuName;
    return (LPSTR)(classPtr->menuName + strlenW(classPtr->menuName) + 1);
}


/***********************************************************************
 *           CLASS_GetMenuNameW
 *
 * Get the menu name as a Unicode string.
 */
static inline LPWSTR CLASS_GetMenuNameW( CLASS *classPtr )
{
    return classPtr->menuName;
}


/***********************************************************************
 *           CLASS_SetMenuNameA
 *
 * Set the menu name in a class structure by copying the string.
 */
static void CLASS_SetMenuNameA( CLASS *classPtr, LPCSTR name )
{
    if (HIWORD(classPtr->menuName)) HeapFree( GetProcessHeap(), 0, classPtr->menuName );
    if (HIWORD(name))
    {
        DWORD lenA = strlen(name) + 1;
        DWORD lenW = MultiByteToWideChar( CP_ACP, 0, name, lenA, NULL, 0 );
        classPtr->menuName = HeapAlloc( GetProcessHeap(), 0, lenA + lenW*sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, name, lenA, classPtr->menuName, lenW );
        memcpy( classPtr->menuName + lenW, name, lenA );
    }
    else classPtr->menuName = (LPWSTR)name;
}


/***********************************************************************
 *           CLASS_SetMenuNameW
 *
 * Set the menu name in a class structure by copying the string.
 */
static void CLASS_SetMenuNameW( CLASS *classPtr, LPCWSTR name )
{
    if (HIWORD(classPtr->menuName)) HeapFree( GetProcessHeap(), 0, classPtr->menuName );
    if (HIWORD(name))
    {
        DWORD lenW = strlenW(name) + 1;
        DWORD lenA = WideCharToMultiByte( CP_ACP, 0, name, lenW, NULL, 0, NULL, NULL );
        classPtr->menuName = HeapAlloc( GetProcessHeap(), 0, lenA + lenW*sizeof(WCHAR) );
        memcpy( classPtr->menuName, name, lenW*sizeof(WCHAR) );
        WideCharToMultiByte( CP_ACP, 0, name, lenW,
                             (char *)(classPtr->menuName + lenW), lenA, NULL, NULL );
    }
    else classPtr->menuName = (LPWSTR)name;
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

    list_remove( &classPtr->entry );
    if (classPtr->hbrBackground > (HBRUSH)(COLOR_GRADIENTINACTIVECAPTION + 1))
        DeleteObject( classPtr->hbrBackground );
    HeapFree( GetProcessHeap(), 0, classPtr->menuName );
    HeapFree( GetProcessHeap(), 0, classPtr );
    USER_Unlock();
}


/***********************************************************************
 *           CLASS_FreeModuleClasses
 */
void CLASS_FreeModuleClasses( HMODULE16 hModule )
{
    struct list *ptr, *next;

    TRACE("0x%08x\n", hModule);

    USER_Lock();
    for (ptr = list_head( &class_list ); ptr; ptr = next)
    {
        CLASS *class = LIST_ENTRY( ptr, CLASS, entry );
        next = list_next( &class_list, ptr );
        if (class->hInstance == HINSTANCE_32(hModule))
        {
            BOOL ret;

            SERVER_START_REQ( destroy_class )
            {
                req->atom = class->atomName;
                req->instance = class->hInstance;
                ret = !wine_server_call_err( req );
            }
            SERVER_END_REQ;
            if (ret) CLASS_FreeClass( class );
        }
    }
    USER_Unlock();
}


/***********************************************************************
 *           CLASS_FindClassByAtom
 *
 * Return a pointer to the class.
 * hinstance has been normalized by the caller.
 */
static CLASS *CLASS_FindClassByAtom( ATOM atom, HINSTANCE hinstance )
{
    struct list *ptr;

    USER_Lock();

    LIST_FOR_EACH( ptr, &class_list )
    {
        CLASS *class = LIST_ENTRY( ptr, CLASS, entry );
        if (class->atomName != atom) continue;
        if (!hinstance || !class->local || class->hInstance == hinstance)
        {
            TRACE("0x%04x %p -> %p\n", atom, hinstance, class);
            return class;
        }
    }
    USER_Unlock();
    TRACE("0x%04x %p -> not found\n", atom, hinstance);
    return NULL;
}


/***********************************************************************
 *           CLASS_RegisterClass
 *
 * The real RegisterClass() functionality.
 * The atom is deleted no matter what.
 */
static CLASS *CLASS_RegisterClass( ATOM atom, HINSTANCE hInstance, BOOL local,
                                   DWORD style, INT classExtra, INT winExtra )
{
    CLASS *classPtr;
    BOOL ret;

    TRACE("atom=0x%x hinst=%p style=0x%x clExtr=0x%x winExtr=0x%x\n",
          atom, hInstance, style, classExtra, winExtra );

    /* Fix the extra bytes value */

    if (classExtra < 0 || winExtra < 0)
    {
         SetLastError( ERROR_INVALID_PARAMETER );
         return NULL;
    }
    if (classExtra > 40)  /* Extra bytes are limited to 40 in Win32 */
        WARN("Class extra bytes %d is > 40\n", classExtra);
    if (winExtra > 40)    /* Extra bytes are limited to 40 in Win32 */
        WARN("Win extra bytes %d is > 40\n", winExtra );

    classPtr = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CLASS) + classExtra );
    if (!classPtr)
    {
        GlobalDeleteAtom( atom );
        return NULL;
    }

    SERVER_START_REQ( create_class )
    {
        req->local      = local;
        req->atom       = atom;
        req->style      = style;
        req->instance   = hInstance;
        req->extra      = classExtra;
        req->win_extra  = winExtra;
        req->client_ptr = classPtr;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    GlobalDeleteAtom( atom );  /* the server increased the atom ref count */
    if (!ret)
    {
        HeapFree( GetProcessHeap(), 0, classPtr );
        return NULL;
    }

    classPtr->style       = style;
    classPtr->local       = local;
    classPtr->cbWndExtra  = winExtra;
    classPtr->cbClsExtra  = classExtra;
    classPtr->hInstance   = hInstance;
    classPtr->atomName    = atom;

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
 * This allows having both ASCII and Unicode winprocs for the same class.
 */
static CLASS *register_builtin( const struct builtin_class_descr *descr )
{
    ATOM atom;
    CLASS *classPtr;

    if (!(atom = GlobalAddAtomA( descr->name ))) return 0;

    if (!(classPtr = CLASS_RegisterClass( atom, user32_module, FALSE,
                                          descr->style, 0, descr->extra ))) return 0;

    classPtr->hCursor       = LoadCursorA( 0, (LPSTR)descr->cursor );
    classPtr->hbrBackground = descr->brush;
    classPtr->winproc       = WINPROC_AllocProc( descr->procA, descr->procW );
    release_class_ptr( classPtr );
    return classPtr;
}


/***********************************************************************
 *           CLASS_RegisterBuiltinClasses
 */
void CLASS_RegisterBuiltinClasses(void)
{
    extern const struct builtin_class_descr BUTTON_builtin_class;
    extern const struct builtin_class_descr COMBO_builtin_class;
    extern const struct builtin_class_descr COMBOLBOX_builtin_class;
    extern const struct builtin_class_descr DIALOG_builtin_class;
    extern const struct builtin_class_descr DESKTOP_builtin_class;
    extern const struct builtin_class_descr EDIT_builtin_class;
    extern const struct builtin_class_descr ICONTITLE_builtin_class;
    extern const struct builtin_class_descr LISTBOX_builtin_class;
    extern const struct builtin_class_descr MDICLIENT_builtin_class;
    extern const struct builtin_class_descr MENU_builtin_class;
    extern const struct builtin_class_descr SCROLL_builtin_class;
    extern const struct builtin_class_descr STATIC_builtin_class;

    register_builtin( &DESKTOP_builtin_class );
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

    /* the DefWindowProc winprocs are magic too */
    WINPROC_AllocProc( DefWindowProcA, DefWindowProcW );
}


/***********************************************************************
 *           CLASS_AddWindow
 *
 * Add a new window using this class, and set the necessary
 * information inside the window structure.
 */
void CLASS_AddWindow( CLASS *class, WND *win, BOOL unicode )
{
    win->class    = class;
    win->clsStyle = class->style;
    win->winproc  = class->winproc;
    if (WINPROC_IsUnicode( win->winproc, unicode )) win->flags |= WIN_ISUNICODE;
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
    ATOM atom;
    CLASS *classPtr;
    HINSTANCE instance;

    if (wc->hInstance == user32_module)
    {
        /* we can't register a class for user32 */
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (!(instance = wc->hInstance)) instance = GetModuleHandleW( NULL );

    if (!(atom = GlobalAddAtomA( wc->lpszClassName ))) return 0;

    if (!(classPtr = CLASS_RegisterClass( atom, instance, !(wc->style & CS_GLOBALCLASS),
                                          wc->style, wc->cbClsExtra, wc->cbWndExtra )))
        return 0;

    TRACE("atom=%04x wndproc=%p hinst=%p bg=%p style=%08x clsExt=%d winExt=%d class=%p\n",
          atom, wc->lpfnWndProc, instance, wc->hbrBackground,
          wc->style, wc->cbClsExtra, wc->cbWndExtra, classPtr );

    classPtr->hIcon         = wc->hIcon;
    classPtr->hIconSm       = wc->hIconSm;
    classPtr->hCursor       = wc->hCursor;
    classPtr->hbrBackground = wc->hbrBackground;
    classPtr->winproc       = WINPROC_AllocProc( wc->lpfnWndProc, NULL );
    CLASS_SetMenuNameA( classPtr, wc->lpszMenuName );
    release_class_ptr( classPtr );
    return atom;
}


/***********************************************************************
 *		RegisterClassExW (USER32.@)
 */
ATOM WINAPI RegisterClassExW( const WNDCLASSEXW* wc )
{
    ATOM atom;
    CLASS *classPtr;
    HINSTANCE instance;

    if (wc->hInstance == user32_module)
    {
        /* we can't register a class for user32 */
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (!(instance = wc->hInstance)) instance = GetModuleHandleW( NULL );

    if (!(atom = GlobalAddAtomW( wc->lpszClassName ))) return 0;

    if (!(classPtr = CLASS_RegisterClass( atom, instance, !(wc->style & CS_GLOBALCLASS),
                                          wc->style, wc->cbClsExtra, wc->cbWndExtra )))
        return 0;

    TRACE("atom=%04x wndproc=%p hinst=%p bg=%p style=%08x clsExt=%d winExt=%d class=%p\n",
          atom, wc->lpfnWndProc, instance, wc->hbrBackground,
          wc->style, wc->cbClsExtra, wc->cbWndExtra, classPtr );

    classPtr->hIcon         = wc->hIcon;
    classPtr->hIconSm       = wc->hIconSm;
    classPtr->hCursor       = wc->hCursor;
    classPtr->hbrBackground = wc->hbrBackground;
    classPtr->winproc       = WINPROC_AllocProc( NULL, wc->lpfnWndProc );
    CLASS_SetMenuNameW( classPtr, wc->lpszMenuName );
    release_class_ptr( classPtr );
    return atom;
}


/***********************************************************************
 *		UnregisterClassA (USER32.@)
 */
BOOL WINAPI UnregisterClassA( LPCSTR className, HINSTANCE hInstance )
{
    ATOM atom = HIWORD(className) ? GlobalFindAtomA( className ) : LOWORD(className);
    return UnregisterClassW( (LPCWSTR)MAKEINTATOM(atom), hInstance );
}

/***********************************************************************
 *		UnregisterClassW (USER32.@)
 */
BOOL WINAPI UnregisterClassW( LPCWSTR className, HINSTANCE hInstance )
{
    CLASS *classPtr = NULL;
    ATOM atom = HIWORD(className) ? GlobalFindAtomW( className ) : LOWORD(className);

    TRACE("%s %p %x\n",debugstr_w(className), hInstance, atom);

    if (!atom)
    {
        SetLastError( ERROR_CLASS_DOES_NOT_EXIST );
        return FALSE;
    }

    SERVER_START_REQ( destroy_class )
    {
        req->atom = atom;
        req->instance = hInstance;
        if (!wine_server_call_err( req )) classPtr = reply->client_ptr;
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

    TRACE("%p %x\n",hwnd, offset);

    if (!(class = get_class_ptr( hwnd, FALSE ))) return 0;

    if (class == CLASS_OTHER_PROCESS)
    {
        SERVER_START_REQ( set_class_info )
        {
            req->window = hwnd;
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

    TRACE("%p %d\n", hwnd, offset);

    if (!(class = get_class_ptr( hwnd, FALSE ))) return 0;

    if (class == CLASS_OTHER_PROCESS)
    {
        SERVER_START_REQ( set_class_info )
        {
            req->window = hwnd;
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
                    retvalue = (ULONG_PTR)reply->old_instance;
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
        retvalue = (ULONG_PTR)class->hIconSm;
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
        retvalue = (ULONG_PTR)class->hInstance;
        break;
    case GCLP_WNDPROC:
        retvalue = (ULONG_PTR)WINPROC_GetProc( class->winproc, unicode );
        break;
    case GCLP_MENUNAME:
        retvalue = (ULONG_PTR)CLASS_GetMenuNameW( class );
        if (unicode)
            retvalue = (ULONG_PTR)CLASS_GetMenuNameW( class );
        else
            retvalue = (ULONG_PTR)CLASS_GetMenuNameA( class );
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

    TRACE("%p %d %x\n", hwnd, offset, newval);

    if (!(class = get_class_ptr( hwnd, TRUE ))) return 0;

    SERVER_START_REQ( set_class_info )
    {
        req->window = hwnd;
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

    TRACE("%p %d %lx\n", hwnd, offset, newval);

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
        class->winproc = WINPROC_AllocProc( unicode ? NULL : (WNDPROC)newval,
                                            unicode ? (WNDPROC)newval : NULL );
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
        class->hIcon = (HICON)newval;
        break;
    case GCLP_HICONSM:
        retval = (ULONG_PTR)class->hIconSm;
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
        retval = (ULONG_PTR)class->hInstance;
        class->hInstance = (HINSTANCE)newval;
        break;
    case GCW_ATOM:
        if (!set_server_info( hwnd, offset, newval, size )) break;
        retval = class->atomName;
        class->atomName = newval;
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
    TRACE("%p %d %x\n", hwnd, offset, newval);

    return CLASS_SetClassLong( hwnd, offset, newval, sizeof(LONG), TRUE );
}


/***********************************************************************
 *		SetClassLongA (USER32.@)
 */
DWORD WINAPI SetClassLongA( HWND hwnd, INT offset, LONG newval )
{
    TRACE("%p %d %x\n", hwnd, offset, newval);

    return CLASS_SetClassLong( hwnd, offset, newval, sizeof(LONG), FALSE );
}


/***********************************************************************
 *		GetClassNameA (USER32.@)
 */
INT WINAPI GetClassNameA( HWND hwnd, LPSTR buffer, INT count )
{
    char tmpbuf[MAX_ATOM_LEN + 1];
    INT ret;

    TRACE("%p %p %d\n", hwnd, buffer, count);

    if (count <= 0) return 0;

    ret = GlobalGetAtomNameA( GetClassLongW( hwnd, GCW_ATOM ), tmpbuf, MAX_ATOM_LEN + 1 );
    if (ret)
    {
        ret = min(count - 1, ret);
        memcpy(buffer, tmpbuf, ret);
        buffer[ret] = 0;
    }
    return ret;
}


/***********************************************************************
 *		GetClassNameW (USER32.@)
 */
INT WINAPI GetClassNameW( HWND hwnd, LPWSTR buffer, INT count )
{
    WCHAR tmpbuf[MAX_ATOM_LEN + 1];
    INT ret;

    TRACE("%p %p %d\n", hwnd, buffer, count);

    if (count <= 0) return 0;

    ret = GlobalGetAtomNameW( GetClassLongW( hwnd, GCW_ATOM ), tmpbuf, MAX_ATOM_LEN + 1 );
    if (ret)
    {
        ret = min(count - 1, ret);
        memcpy(buffer, tmpbuf, ret * sizeof(WCHAR));
        buffer[ret] = 0;
    }
    return ret;
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
    ATOM atom = HIWORD(name) ? GlobalFindAtomA( name ) : LOWORD(name);
    CLASS *classPtr;

    TRACE("%p %s %x %p\n", hInstance, debugstr_a(name), atom, wc);

    if (!hInstance) hInstance = user32_module;

    if (!atom || !(classPtr = CLASS_FindClassByAtom( atom, hInstance )))
    {
        SetLastError( ERROR_CLASS_DOES_NOT_EXIST );
        return FALSE;
    }
    wc->style         = classPtr->style;
    wc->lpfnWndProc   = WINPROC_GetProc( classPtr->winproc, FALSE );
    wc->cbClsExtra    = classPtr->cbClsExtra;
    wc->cbWndExtra    = classPtr->cbWndExtra;
    wc->hInstance     = (hInstance == user32_module) ? 0 : hInstance;
    wc->hIcon         = (HICON)classPtr->hIcon;
    wc->hIconSm       = (HICON)classPtr->hIconSm;
    wc->hCursor       = (HCURSOR)classPtr->hCursor;
    wc->hbrBackground = (HBRUSH)classPtr->hbrBackground;
    wc->lpszMenuName  = CLASS_GetMenuNameA( classPtr );
    wc->lpszClassName = name;
    release_class_ptr( classPtr );

    /* We must return the atom of the class here instead of just TRUE. */
    return atom;
}


/***********************************************************************
 *		GetClassInfoExW (USER32.@)
 */
BOOL WINAPI GetClassInfoExW( HINSTANCE hInstance, LPCWSTR name, WNDCLASSEXW *wc )
{
    ATOM atom = HIWORD(name) ? GlobalFindAtomW( name ) : LOWORD(name);
    CLASS *classPtr;

    TRACE("%p %s %x %p\n", hInstance, debugstr_w(name), atom, wc);

    if (!hInstance) hInstance = user32_module;

    if (!atom || !(classPtr = CLASS_FindClassByAtom( atom, hInstance )))
    {
        SetLastError( ERROR_CLASS_DOES_NOT_EXIST );
        return FALSE;
    }
    wc->style         = classPtr->style;
    wc->lpfnWndProc   = WINPROC_GetProc( classPtr->winproc, TRUE );
    wc->cbClsExtra    = classPtr->cbClsExtra;
    wc->cbWndExtra    = classPtr->cbWndExtra;
    wc->hInstance     = (hInstance == user32_module) ? 0 : hInstance;
    wc->hIcon         = (HICON)classPtr->hIcon;
    wc->hIconSm       = (HICON)classPtr->hIconSm;
    wc->hCursor       = (HCURSOR)classPtr->hCursor;
    wc->hbrBackground = (HBRUSH)classPtr->hbrBackground;
    wc->lpszMenuName  = CLASS_GetMenuNameW( classPtr );
    wc->lpszClassName = name;
    release_class_ptr( classPtr );

    /* We must return the atom of the class here instead of just TRUE. */
    return atom;
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
