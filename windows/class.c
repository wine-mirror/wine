/*
 * Window classes functions
 *
 * Copyright 1993, 1996 Alexandre Julliard
 *           1998 Juergen Schmied (jsch)
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * FIXME: In win32 all classes are local. They are registered at
 *	  program start. Processes CANNOT share classes. (Source: some
 *	  win31->NT migration book)
 *
 * FIXME: There seems to be a general problem with hInstance in WINE
 *   classes are getting registered with wrong hInstance.
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "wine/winbase16.h"
#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "wownt32.h"
#include "wine/unicode.h"
#include "win.h"
#include "user.h"
#include "controls.h"
#include "dce.h"
#include "winproc.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(class);

typedef struct tagCLASS
{
    struct tagCLASS *next;          /* Next class */
    struct tagCLASS *prev;          /* Prev class */
    UINT             cWindows;      /* Count of existing windows */
    UINT             style;         /* Class style */
    WNDPROC          winprocA;      /* Window procedure (ASCII) */
    WNDPROC          winprocW;      /* Window procedure (Unicode) */
    INT              cbClsExtra;    /* Class extra bytes */
    INT              cbWndExtra;    /* Window extra bytes */
    LPWSTR           menuName;      /* Default menu name (Unicode followed by ASCII) */
    SEGPTR           segMenuName;   /* Default menu name as SEGPTR */
    struct tagDCE   *dce;           /* Class DCE (if CS_CLASSDC) */
    HINSTANCE        hInstance;     /* Module that created the task */
    HICON            hIcon;         /* Default icon */
    HICON            hIconSm;       /* Default small icon */
    HCURSOR          hCursor;       /* Default cursor */
    HBRUSH           hbrBackground; /* Default background */
    ATOM             atomName;      /* Name of the class */
} CLASS;

static CLASS *firstClass;

/***********************************************************************
 *           get_class_ptr
 */
static CLASS *get_class_ptr( HWND hwnd, BOOL write_access )
{
    WND *ptr = WIN_GetPtr( hwnd );

    if (ptr)
    {
        if (ptr != WND_OTHER_PROCESS) return ptr->class;
        if (IsWindow( hwnd )) /* check other processes */
        {
            if (write_access)
            {
                /* modifying classes in other processes is not allowed */
                SetLastError( ERROR_ACCESS_DENIED );
                return NULL;
            }
            FIXME( "reading from class of other process window %p\n", hwnd );
            /* DbgBreakPoint(); */
        }
    }
    SetLastError( ERROR_INVALID_WINDOW_HANDLE );
    return NULL;
}


/***********************************************************************
 *           release_class_ptr
 */
inline static void release_class_ptr( CLASS *ptr )
{
    USER_Unlock();
}


/***********************************************************************
 *           CLASS_GetProc
 *
 * Get the class winproc for a given proc type
 */
static WNDPROC16 CLASS_GetProc( CLASS *classPtr, WINDOWPROCTYPE type )
{
    WNDPROC proc = classPtr->winprocA;

    if (classPtr->winprocW)
    {
        /* if we have a Unicode proc, use it if we have no ASCII proc
         * or if we have both and Unicode was requested
         */
        if (!proc || type == WIN_PROC_32W) proc = classPtr->winprocW;
    }
    return WINPROC_GetProc( proc, type );
}


/***********************************************************************
 *           CLASS_SetProc
 *
 * Set the class winproc for a given proc type.
 * Returns the previous window proc.
 */
static WNDPROC16 CLASS_SetProc( CLASS *classPtr, WNDPROC newproc, WINDOWPROCTYPE type )
{
    WNDPROC *proc = &classPtr->winprocA;
    WNDPROC16 ret;

    if (classPtr->winprocW)
    {
        /* if we have a Unicode proc, use it if we have no ASCII proc
         * or if we have both and Unicode was requested
         */
        if (!*proc || type == WIN_PROC_32W) proc = &classPtr->winprocW;
    }
    ret = WINPROC_GetProc( *proc, type );
    WINPROC_SetProc( proc, newproc, type, WIN_PROC_CLASS );
    /* now free the one that we didn't set */
    if (classPtr->winprocA && classPtr->winprocW)
    {
        if (proc == &classPtr->winprocA)
        {
            WINPROC_FreeProc( classPtr->winprocW, WIN_PROC_CLASS );
            classPtr->winprocW = 0;
        }
        else
        {
            WINPROC_FreeProc( classPtr->winprocA, WIN_PROC_CLASS );
            classPtr->winprocA = 0;
        }
    }
    return ret;
}


/***********************************************************************
 *           CLASS_GetMenuNameA
 *
 * Get the menu name as a ASCII string.
 */
inline static LPSTR CLASS_GetMenuNameA( CLASS *classPtr )
{
    if (!HIWORD(classPtr->menuName)) return (LPSTR)classPtr->menuName;
    return (LPSTR)(classPtr->menuName + strlenW(classPtr->menuName) + 1);
}


/***********************************************************************
 *           CLASS_GetMenuName16
 *
 * Get the menu name as a SEGPTR.
 */
inline static SEGPTR CLASS_GetMenuName16( CLASS *classPtr )
{
    if (!HIWORD(classPtr->menuName)) return (SEGPTR)classPtr->menuName;
    if (!classPtr->segMenuName)
        classPtr->segMenuName = MapLS( CLASS_GetMenuNameA(classPtr) );
    return classPtr->segMenuName;
}


/***********************************************************************
 *           CLASS_GetMenuNameW
 *
 * Get the menu name as a Unicode string.
 */
inline static LPWSTR CLASS_GetMenuNameW( CLASS *classPtr )
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
    UnMapLS( classPtr->segMenuName );
    classPtr->segMenuName = 0;
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
    UnMapLS( classPtr->segMenuName );
    classPtr->segMenuName = 0;
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
static BOOL CLASS_FreeClass( CLASS *classPtr )
{
    TRACE("%p\n", classPtr);

    /* Check if we can remove this class */

    if (classPtr->cWindows > 0)
    {
        SetLastError( ERROR_CLASS_HAS_WINDOWS );
        return FALSE;
    }

    /* Remove the class from the linked list */

    if (classPtr->next) classPtr->next->prev = classPtr->prev;
    if (classPtr->prev) classPtr->prev->next = classPtr->next;
    else firstClass = classPtr->next;

    /* Delete the class */

    if (classPtr->dce) DCE_FreeDCE( classPtr->dce );
    if (classPtr->hbrBackground > (HBRUSH)(COLOR_GRADIENTINACTIVECAPTION + 1))
        DeleteObject( classPtr->hbrBackground );
    GlobalDeleteAtom( classPtr->atomName );
    WINPROC_FreeProc( classPtr->winprocA, WIN_PROC_CLASS );
    WINPROC_FreeProc( classPtr->winprocW, WIN_PROC_CLASS );
    UnMapLS( classPtr->segMenuName );
    HeapFree( GetProcessHeap(), 0, classPtr->menuName );
    HeapFree( GetProcessHeap(), 0, classPtr );
    return TRUE;
}


/***********************************************************************
 *           CLASS_FreeModuleClasses
 */
void CLASS_FreeModuleClasses( HMODULE16 hModule )
{
    CLASS *ptr, *next;

    TRACE("0x%08x\n", hModule);

    USER_Lock();
    for (ptr = firstClass; ptr; ptr = next)
    {
        next = ptr->next;
	if (ptr->hInstance == HINSTANCE_32(hModule)) CLASS_FreeClass( ptr );
    }
    USER_Unlock();
}


/***********************************************************************
 *           CLASS_FindClassByAtom
 *
 * Return a pointer to the class.
 * hinstance has been normalized by the caller.
 *
 * NOTES
 *  980805 a local class will be found now if registred with hInst=0
 *  and looed up with a hInst!=0. msmoney does it (jsch)
 *
 *  Local class registered with a USER instance handle are found as if
 *  they were global classes.
 */
static CLASS *CLASS_FindClassByAtom( ATOM atom, HINSTANCE hinstance )
{
    CLASS * class, *tclass = 0, *user_class = 0;
    HINSTANCE16 hUser = GetModuleHandle16("USER");

    TRACE("0x%08x %p\n", atom, hinstance);

    /* First search task-specific classes */

    for (class = firstClass; (class); class = class->next)
    {
        if (class->style & CS_GLOBALCLASS) continue;
        if (class->atomName == atom)
        {
            if (hinstance==class->hInstance || hinstance == (HINSTANCE)0xffff)
            {
                TRACE("-- found local %p\n", class);
                return class;
            }
            if (class->hInstance == 0) tclass = class;
            else if(class->hInstance == HINSTANCE_32(hUser))
            {
                user_class = class;
            }
        }
    }

    /* Then search global classes */

    for (class = firstClass; (class); class = class->next)
    {
        if (!(class->style & CS_GLOBALCLASS)) continue;
        if (class->atomName == atom)
        {
            TRACE("-- found global %p\n", class);
            return class;
        }
    }

    /* Check if there was a local class registered with USER */
    if( user_class )
    {
        TRACE("--found local USER class %p\n", user_class);
        return user_class;
    }

    /* Then check if there was a local class with hInst=0*/
    if ( tclass )
    {
        WARN("-- found local Class registred with hInst=0\n");
        return tclass;
    }

    TRACE("-- not found\n");
    return 0;
}


/***********************************************************************
 *           CLASS_RegisterClass
 *
 * The real RegisterClass() functionality.
 */
static CLASS *CLASS_RegisterClass( ATOM atom, HINSTANCE hInstance,
                                   DWORD style, INT classExtra, INT winExtra )
{
    CLASS *classPtr;

    TRACE("atom=0x%x hinst=%p style=0x%lx clExtr=0x%x winExtr=0x%x\n",
          atom, hInstance, style, classExtra, winExtra );

   /* Check if a class with this name already exists */
    classPtr = CLASS_FindClassByAtom( atom, hInstance );
    if (classPtr)
    {
        /* Class can be created only if it is local and */
        /* if the class with the same name is global.   */

        if ((style & CS_GLOBALCLASS) || !(classPtr->style & CS_GLOBALCLASS))
        {
            SetLastError( ERROR_CLASS_ALREADY_EXISTS );
            return NULL;
        }
    }

    /* Fix the extra bytes value */

    if (classExtra < 0) classExtra = 0;
    else if (classExtra > 40)  /* Extra bytes are limited to 40 in Win32 */
        WARN("Class extra bytes %d is > 40\n", classExtra);
    if (winExtra < 0) winExtra = 0;
    else if (winExtra > 40)    /* Extra bytes are limited to 40 in Win32 */
        WARN("Win extra bytes %d is > 40\n", winExtra );

    /* Create the class */

    classPtr = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CLASS) + classExtra );
    if (!classPtr) return NULL;
    classPtr->style       = style;
    classPtr->cbWndExtra  = winExtra;
    classPtr->cbClsExtra  = classExtra;
    classPtr->hInstance   = hInstance;
    classPtr->atomName    = atom;
    classPtr->dce         = (style & CS_CLASSDC) ? DCE_AllocDCE( 0, DCE_CLASS_DC ) : NULL;

    /* Other non-null values must be set by caller */

    if ((classPtr->next = firstClass)) firstClass->prev = classPtr;
    firstClass = classPtr;
    return classPtr;
}


/***********************************************************************
 *           CLASS_UnregisterClass
 *
 * The real UnregisterClass() functionality.
 */
static BOOL CLASS_UnregisterClass( ATOM atom, HINSTANCE hInstance )
{
    CLASS *classPtr;
    BOOL ret = FALSE;

    USER_Lock();
    if (atom &&
        (classPtr = CLASS_FindClassByAtom( atom, hInstance )) &&
        (!hInstance || classPtr->hInstance == hInstance))
    {
        ret = CLASS_FreeClass( classPtr );
    }
    else SetLastError( ERROR_CLASS_DOES_NOT_EXIST );

    USER_Unlock();
    return ret;
}


/***********************************************************************
 *           CLASS_RegisterBuiltinClass
 *
 * Register a builtin control class.
 * This allows having both ASCII and Unicode winprocs for the same class.
 */
ATOM CLASS_RegisterBuiltinClass( const struct builtin_class_descr *descr )
{
    ATOM atom;
    CLASS *classPtr;

    if (!(atom = GlobalAddAtomA( descr->name ))) return 0;

    if (!(classPtr = CLASS_RegisterClass( atom, 0, descr->style, 0, descr->extra )))
    {
        GlobalDeleteAtom( atom );
        return 0;
    }

    classPtr->hCursor       = LoadCursorA( 0, (LPSTR)descr->cursor );
    classPtr->hbrBackground = descr->brush;

    if (descr->procA) WINPROC_SetProc( &classPtr->winprocA, descr->procA,
                                       WIN_PROC_32A, WIN_PROC_CLASS );
    if (descr->procW) WINPROC_SetProc( &classPtr->winprocW, descr->procW,
                                       WIN_PROC_32W, WIN_PROC_CLASS );
    return atom;
}


/***********************************************************************
 *           CLASS_AddWindow
 *
 * Add a new window using this class, and return the necessary
 * information for creating the window.
 */
CLASS *CLASS_AddWindow( ATOM atom, HINSTANCE inst, WINDOWPROCTYPE type,
                        INT *winExtra, WNDPROC *winproc, DWORD *style, struct tagDCE **dce )
{
    CLASS *class;
    if (type == WIN_PROC_16) inst = HINSTANCE_32(GetExePtr(HINSTANCE_16(inst)));

    if (!(class = CLASS_FindClassByAtom( atom, inst ))) return NULL;
    class->cWindows++;

    if (type == WIN_PROC_32W)
    {
        if (!(*winproc = class->winprocW)) *winproc = class->winprocA;
    }
    else
    {
        if (!(*winproc = class->winprocA)) *winproc = class->winprocW;
    }
    *winExtra = class->cbWndExtra;
    *style = class->style;
    *dce = class->dce;
    return class;
}


/***********************************************************************
 *           CLASS_RemoveWindow
 *
 * Remove a window from the class window count.
 */
void CLASS_RemoveWindow( CLASS *cls )
{
    if (cls && cls->cWindows) cls->cWindows--;
}


/***********************************************************************
 *		RegisterClass (USER.57)
 */
ATOM WINAPI RegisterClass16( const WNDCLASS16 *wc )
{
    ATOM atom;
    CLASS *classPtr;
    int iSmIconWidth, iSmIconHeight;
    HINSTANCE hInstance = HINSTANCE_32(GetExePtr(wc->hInstance));

    if (!(atom = GlobalAddAtomA( MapSL(wc->lpszClassName) ))) return 0;
    if (!(classPtr = CLASS_RegisterClass( atom, hInstance, wc->style,
                                          wc->cbClsExtra, wc->cbWndExtra )))
    {
        GlobalDeleteAtom( atom );
        return 0;
    }

    TRACE("atom=%04x wndproc=%08lx hinst=%p bg=%04x style=%08x clsExt=%d winExt=%d class=%p name='%s'\n",
          atom, (DWORD)wc->lpfnWndProc, hInstance,
          wc->hbrBackground, wc->style, wc->cbClsExtra,
          wc->cbWndExtra, classPtr,
          HIWORD(wc->lpszClassName) ? (char *)MapSL(wc->lpszClassName) : "" );

    iSmIconWidth  = GetSystemMetrics(SM_CXSMICON);
    iSmIconHeight = GetSystemMetrics(SM_CYSMICON);

    classPtr->hIcon         = HICON_32(wc->hIcon);
    classPtr->hIconSm       = CopyImage(classPtr->hIcon, IMAGE_ICON,
					iSmIconWidth, iSmIconHeight,
					LR_COPYFROMRESOURCE);
    classPtr->hCursor       = HCURSOR_32(wc->hCursor);
    classPtr->hbrBackground = HBRUSH_32(wc->hbrBackground);

    WINPROC_SetProc( &classPtr->winprocA, (WNDPROC)wc->lpfnWndProc,
                     WIN_PROC_16, WIN_PROC_CLASS );
    CLASS_SetMenuNameA( classPtr, MapSL(wc->lpszMenuName) );

    return atom;
}


/***********************************************************************
 *		RegisterClassA (USER32.@)
 * RETURNS
 *	>0: Unique identifier
 *	0: Failure
 */
ATOM WINAPI RegisterClassA( const WNDCLASSA* wc ) /* [in] Address of structure with class data */
{
    ATOM atom;
    int iSmIconWidth, iSmIconHeight;
    CLASS *classPtr;

    if (!(atom = GlobalAddAtomA( wc->lpszClassName ))) return 0;

    if (!(classPtr = CLASS_RegisterClass( atom, wc->hInstance, wc->style,
                                          wc->cbClsExtra, wc->cbWndExtra )))
    {
        GlobalDeleteAtom( atom );
        return 0;
    }

    TRACE("atom=%04x wndproc=%p hinst=%p bg=%p style=%08x clsExt=%d winExt=%d class=%p name='%s'\n",
          atom, wc->lpfnWndProc, wc->hInstance,
          wc->hbrBackground, wc->style, wc->cbClsExtra,
          wc->cbWndExtra, classPtr,
          HIWORD(wc->lpszClassName) ? wc->lpszClassName : "" );

    iSmIconWidth  = GetSystemMetrics(SM_CXSMICON);
    iSmIconHeight = GetSystemMetrics(SM_CYSMICON);

    classPtr->hIcon         = wc->hIcon;
    classPtr->hIconSm       = CopyImage(wc->hIcon, IMAGE_ICON,
					iSmIconWidth, iSmIconHeight,
					LR_COPYFROMRESOURCE);
    classPtr->hCursor       = wc->hCursor;
    classPtr->hbrBackground = wc->hbrBackground;

    WINPROC_SetProc( &classPtr->winprocA, wc->lpfnWndProc,
                     WIN_PROC_32A, WIN_PROC_CLASS );
    CLASS_SetMenuNameA( classPtr, wc->lpszMenuName );
    return atom;
}


/***********************************************************************
 *		RegisterClassW (USER32.@)
 */
ATOM WINAPI RegisterClassW( const WNDCLASSW* wc )
{
    ATOM atom;
    int iSmIconWidth, iSmIconHeight;
    CLASS *classPtr;

    if (!(atom = GlobalAddAtomW( wc->lpszClassName ))) return 0;

    if (!(classPtr = CLASS_RegisterClass( atom, wc->hInstance, wc->style,
                                          wc->cbClsExtra, wc->cbWndExtra )))
    {
        GlobalDeleteAtom( atom );
        return 0;
    }

    TRACE("atom=%04x wndproc=%p hinst=%p bg=%p style=%08x clsExt=%d winExt=%d class=%p\n",
          atom, wc->lpfnWndProc, wc->hInstance,
          wc->hbrBackground, wc->style, wc->cbClsExtra,
          wc->cbWndExtra, classPtr );

    iSmIconWidth  = GetSystemMetrics(SM_CXSMICON);
    iSmIconHeight = GetSystemMetrics(SM_CYSMICON);

    classPtr->hIcon         = wc->hIcon;
    classPtr->hIconSm       = CopyImage(wc->hIcon, IMAGE_ICON,
					iSmIconWidth, iSmIconHeight,
					LR_COPYFROMRESOURCE);
    classPtr->hCursor       = wc->hCursor;
    classPtr->hbrBackground = wc->hbrBackground;

    WINPROC_SetProc( &classPtr->winprocW, wc->lpfnWndProc,
                     WIN_PROC_32W, WIN_PROC_CLASS );
    CLASS_SetMenuNameW( classPtr, wc->lpszMenuName );
    return atom;
}


/***********************************************************************
 *		RegisterClassEx (USER.397)
 */
ATOM WINAPI RegisterClassEx16( const WNDCLASSEX16 *wc )
{
    ATOM atom;
    CLASS *classPtr;
    HINSTANCE hInstance = HINSTANCE_32(GetExePtr( wc->hInstance ));

    if (!(atom = GlobalAddAtomA( MapSL(wc->lpszClassName) ))) return 0;
    if (!(classPtr = CLASS_RegisterClass( atom, hInstance, wc->style,
                                          wc->cbClsExtra, wc->cbWndExtra )))
    {
        GlobalDeleteAtom( atom );
        return 0;
    }

    TRACE("atom=%04x wndproc=%p hinst=%p bg=%04x style=%08x clsExt=%d winExt=%d class=%p\n",
          atom, wc->lpfnWndProc, hInstance,
          wc->hbrBackground, wc->style, wc->cbClsExtra,
          wc->cbWndExtra, classPtr );

    classPtr->hIcon         = HICON_32(wc->hIcon);
    classPtr->hIconSm       = HICON_32(wc->hIconSm);
    classPtr->hCursor       = HCURSOR_32(wc->hCursor);
    classPtr->hbrBackground = HBRUSH_32(wc->hbrBackground);

    WINPROC_SetProc( &classPtr->winprocA, (WNDPROC)wc->lpfnWndProc,
                     WIN_PROC_16, WIN_PROC_CLASS );
    CLASS_SetMenuNameA( classPtr, MapSL(wc->lpszMenuName) );
    return atom;
}


/***********************************************************************
 *		RegisterClassExA (USER32.@)
 */
ATOM WINAPI RegisterClassExA( const WNDCLASSEXA* wc )
{
    ATOM atom;
    CLASS *classPtr;

    if (!(atom = GlobalAddAtomA( wc->lpszClassName ))) return 0;

    if (!(classPtr = CLASS_RegisterClass( atom, wc->hInstance, wc->style,
                                          wc->cbClsExtra, wc->cbWndExtra )))
    {
        GlobalDeleteAtom( atom );
        return 0;
    }

    TRACE("atom=%04x wndproc=%p hinst=%p bg=%p style=%08x clsExt=%d winExt=%d class=%p\n",
          atom, wc->lpfnWndProc, wc->hInstance,
          wc->hbrBackground, wc->style, wc->cbClsExtra,
          wc->cbWndExtra, classPtr );

    classPtr->hIcon         = wc->hIcon;
    classPtr->hIconSm       = wc->hIconSm;
    classPtr->hCursor       = wc->hCursor;
    classPtr->hbrBackground = wc->hbrBackground;
    WINPROC_SetProc( &classPtr->winprocA, wc->lpfnWndProc, WIN_PROC_32A, WIN_PROC_CLASS );
    CLASS_SetMenuNameA( classPtr, wc->lpszMenuName );
    return atom;
}


/***********************************************************************
 *		RegisterClassExW (USER32.@)
 */
ATOM WINAPI RegisterClassExW( const WNDCLASSEXW* wc )
{
    ATOM atom;
    CLASS *classPtr;

    if (!(atom = GlobalAddAtomW( wc->lpszClassName ))) return 0;

    if (!(classPtr = CLASS_RegisterClass( atom, wc->hInstance, wc->style,
                                          wc->cbClsExtra, wc->cbWndExtra )))
    {
        GlobalDeleteAtom( atom );
        return 0;
    }

    TRACE("atom=%04x wndproc=%p hinst=%p bg=%p style=%08x clsExt=%d winExt=%d class=%p\n",
          atom, wc->lpfnWndProc, wc->hInstance,
          wc->hbrBackground, wc->style, wc->cbClsExtra,
          wc->cbWndExtra, classPtr );

    classPtr->hIcon         = wc->hIcon;
    classPtr->hIconSm       = wc->hIconSm;
    classPtr->hCursor       = wc->hCursor;
    classPtr->hbrBackground = wc->hbrBackground;
    WINPROC_SetProc( &classPtr->winprocW, wc->lpfnWndProc, WIN_PROC_32W, WIN_PROC_CLASS );
    CLASS_SetMenuNameW( classPtr, wc->lpszMenuName );
    return atom;
}


/***********************************************************************
 *		UnregisterClass (USER.403)
 */
BOOL16 WINAPI UnregisterClass16( LPCSTR className, HINSTANCE16 hInstance )
{
    return UnregisterClassA( className, HINSTANCE_32(GetExePtr( hInstance )) );
}

/***********************************************************************
 *		UnregisterClassA (USER32.@)
 *
 */
BOOL WINAPI UnregisterClassA( LPCSTR className, HINSTANCE hInstance )
{
    TRACE("%s %p\n",debugstr_a(className), hInstance);
    return CLASS_UnregisterClass( GlobalFindAtomA( className ), hInstance );
}

/***********************************************************************
 *		UnregisterClassW (USER32.@)
 */
BOOL WINAPI UnregisterClassW( LPCWSTR className, HINSTANCE hInstance )
{
    TRACE("%s %p\n",debugstr_w(className), hInstance);
    return CLASS_UnregisterClass( GlobalFindAtomW( className ), hInstance );
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

    if (offset <= class->cbClsExtra - sizeof(WORD))
        memcpy( &retvalue, (char *)(class + 1) + offset, sizeof(retvalue) );
    else
        SetLastError( ERROR_INVALID_INDEX );
    release_class_ptr( class );
    return retvalue;
}


/***********************************************************************
 *		GetClassLong (USER.131)
 */
LONG WINAPI GetClassLong16( HWND16 hwnd16, INT16 offset )
{
    CLASS *class;
    LONG ret;
    HWND hwnd = (HWND)(ULONG_PTR)hwnd16;  /* no need for full handle */

    TRACE("%p %d\n",hwnd, offset);

    switch( offset )
    {
    case GCL_WNDPROC:
        if (!(class = get_class_ptr( hwnd, FALSE ))) return 0;
        ret = (LONG)CLASS_GetProc( class, WIN_PROC_16 );
        release_class_ptr( class );
        return ret;
    case GCL_MENUNAME:
        if (!(class = get_class_ptr( hwnd, FALSE ))) return 0;
        ret = (LONG)CLASS_GetMenuName16( class );
        release_class_ptr( class );
        return ret;
    default:
        return GetClassLongA( hwnd, offset );
    }
}


/***********************************************************************
 *		GetClassLongW (USER32.@)
 */
LONG WINAPI GetClassLongW( HWND hwnd, INT offset )
{
    CLASS *class;
    LONG retvalue = 0;

    TRACE("%p %d\n", hwnd, offset);

    if (!(class = get_class_ptr( hwnd, FALSE ))) return 0;

    if (offset >= 0)
    {
        if (offset <= class->cbClsExtra - sizeof(LONG))
            memcpy( &retvalue, (char *)(class + 1) + offset, sizeof(retvalue) );
        else
            SetLastError( ERROR_INVALID_INDEX );
        release_class_ptr( class );
        return retvalue;
    }

    switch(offset)
    {
    case GCL_HBRBACKGROUND:
        retvalue = (LONG)class->hbrBackground;
        break;
    case GCL_HCURSOR:
        retvalue = (LONG)class->hCursor;
        break;
    case GCL_HICON:
        retvalue = (LONG)class->hIcon;
        break;
    case GCL_HICONSM:
        retvalue = (LONG)class->hIconSm;
        break;
    case GCL_STYLE:
        retvalue = (LONG)class->style;
        break;
    case GCL_CBWNDEXTRA:
        retvalue = (LONG)class->cbWndExtra;
        break;
    case GCL_CBCLSEXTRA:
        retvalue = (LONG)class->cbClsExtra;
        break;
    case GCL_HMODULE:
        retvalue = (LONG)class->hInstance;
        break;
    case GCL_WNDPROC:
        retvalue = (LONG)CLASS_GetProc( class, WIN_PROC_32W );
        break;
    case GCL_MENUNAME:
        retvalue = (LONG)CLASS_GetMenuNameW( class );
        break;
    case GCW_ATOM:
        retvalue = (DWORD)class->atomName;
        break;
    default:
        SetLastError( ERROR_INVALID_INDEX );
        break;
    }
    release_class_ptr( class );
    return retvalue;
}


/***********************************************************************
 *		GetClassLongA (USER32.@)
 */
LONG WINAPI GetClassLongA( HWND hwnd, INT offset )
{
    CLASS *class;
    LONG retvalue;

    if (offset != GCL_WNDPROC && offset != GCL_MENUNAME)
        return GetClassLongW( hwnd, offset );

    TRACE("%p %d\n", hwnd, offset);

    if (!(class = get_class_ptr( hwnd, FALSE ))) return 0;

    if (offset == GCL_WNDPROC)
        retvalue = (LONG)CLASS_GetProc( class, WIN_PROC_32A );
    else  /* GCL_MENUNAME */
        retvalue = (LONG)CLASS_GetMenuNameA( class );

    release_class_ptr( class );
    return retvalue;
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

    if (offset <= class->cbClsExtra - sizeof(WORD))
    {
        void *ptr = (char *)(class + 1) + offset;
        memcpy( &retval, ptr, sizeof(retval) );
        memcpy( ptr, &newval, sizeof(newval) );
    }
    else SetLastError( ERROR_INVALID_INDEX );

    release_class_ptr( class );
    return retval;
}


/***********************************************************************
 *		SetClassLong (USER.132)
 */
LONG WINAPI SetClassLong16( HWND16 hwnd16, INT16 offset, LONG newval )
{
    CLASS *class;
    LONG retval;
    HWND hwnd = (HWND)(ULONG_PTR)hwnd16;  /* no need for full handle */

    TRACE("%p %d %lx\n", hwnd, offset, newval);

    switch(offset)
    {
    case GCL_WNDPROC:
        if (!(class = get_class_ptr( hwnd, TRUE ))) return 0;
        retval = (LONG)CLASS_SetProc( class, (WNDPROC)newval, WIN_PROC_16 );
        release_class_ptr( class );
        return retval;
    case GCL_MENUNAME:
        newval = (LONG)MapSL( newval );
        /* fall through */
    default:
        return SetClassLongA( hwnd, offset, newval );
    }
}


/***********************************************************************
 *		SetClassLongW (USER32.@)
 */
LONG WINAPI SetClassLongW( HWND hwnd, INT offset, LONG newval )
{
    CLASS *class;
    LONG retval = 0;

    TRACE("%p %d %lx\n", hwnd, offset, newval);

    if (!(class = get_class_ptr( hwnd, TRUE ))) return 0;

    if (offset >= 0)
    {
        if (offset <= class->cbClsExtra - sizeof(LONG))
        {
            void *ptr = (char *)(class + 1) + offset;
            memcpy( &retval, ptr, sizeof(retval) );
            memcpy( ptr, &newval, sizeof(newval) );
        }
        else SetLastError( ERROR_INVALID_INDEX );
    }
    else switch(offset)
    {
    case GCL_MENUNAME:
        CLASS_SetMenuNameW( class, (LPCWSTR)newval );
        retval = 0;  /* Old value is now meaningless anyway */
        break;
    case GCL_WNDPROC:
        retval = (LONG)CLASS_SetProc( class, (WNDPROC)newval, WIN_PROC_32W );
        break;
    case GCL_HBRBACKGROUND:
        retval = (LONG)class->hbrBackground;
        class->hbrBackground = (HBRUSH)newval;
        break;
    case GCL_HCURSOR:
        retval = (LONG)class->hCursor;
        class->hCursor = (HCURSOR)newval;
        break;
    case GCL_HICON:
        retval = (LONG)class->hIcon;
        class->hIcon = (HICON)newval;
        break;
    case GCL_HICONSM:
        retval = (LONG)class->hIconSm;
        class->hIconSm = (HICON)newval;
        break;
    case GCL_STYLE:
        retval = (LONG)class->style;
        class->style = newval;
        break;
    case GCL_CBWNDEXTRA:
        retval = (LONG)class->cbWndExtra;
        class->cbWndExtra = newval;
        break;
    case GCL_HMODULE:
        retval = (LONG)class->hInstance;
        class->hInstance = (HINSTANCE)newval;
        break;
    case GCW_ATOM:
        retval = (DWORD)class->atomName;
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
 *		SetClassLongA (USER32.@)
 */
LONG WINAPI SetClassLongA( HWND hwnd, INT offset, LONG newval )
{
    CLASS *class;
    LONG retval;

    if (offset != GCL_WNDPROC && offset != GCL_MENUNAME)
        return SetClassLongW( hwnd, offset, newval );

    TRACE("%p %d %lx\n", hwnd, offset, newval);

    if (!(class = get_class_ptr( hwnd, TRUE ))) return 0;

    if (offset == GCL_WNDPROC)
        retval = (LONG)CLASS_SetProc( class, (WNDPROC)newval, WIN_PROC_32A );
    else  /* GCL_MENUNAME */
    {
        CLASS_SetMenuNameA( class, (LPCSTR)newval );
        retval = 0;  /* Old value is now meaningless anyway */
    }
    release_class_ptr( class );
    return retval;
}


/***********************************************************************
 *		GetClassNameA (USER32.@)
 */
INT WINAPI GetClassNameA( HWND hwnd, LPSTR buffer, INT count )
{
    INT ret = GlobalGetAtomNameA( GetClassLongA( hwnd, GCW_ATOM ), buffer, count );

    TRACE("%p %s %x\n",hwnd, debugstr_a(buffer), count);
    return ret;
}


/***********************************************************************
 *		GetClassNameW (USER32.@)
 */
INT WINAPI GetClassNameW( HWND hwnd, LPWSTR buffer, INT count )
{
    INT ret = GlobalGetAtomNameW( GetClassLongW( hwnd, GCW_ATOM ), buffer, count );

    TRACE("%p %s %x\n",hwnd, debugstr_w(buffer), count);
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
 *		GetClassInfo (USER.404)
 */
BOOL16 WINAPI GetClassInfo16( HINSTANCE16 hInst16, SEGPTR name, WNDCLASS16 *wc )
{
    ATOM atom;
    CLASS *classPtr;
    HINSTANCE hInstance = HINSTANCE_32(GetExePtr( hInst16 ));

    TRACE("%p %s %p\n",hInstance, debugstr_a(MapSL(name)), wc);

    if (!(atom = GlobalFindAtomA( MapSL(name) )) ||
        !(classPtr = CLASS_FindClassByAtom( atom, hInstance )))
        return FALSE;
    if ((hInstance != classPtr->hInstance) &&
        !(classPtr->style & CS_GLOBALCLASS)) /*BWCC likes to pass hInstance=0*/
        return FALSE;
    wc->style         = (UINT16)classPtr->style;
    wc->lpfnWndProc   = CLASS_GetProc( classPtr, WIN_PROC_16 );
    wc->cbClsExtra    = (INT16)classPtr->cbClsExtra;
    wc->cbWndExtra    = (INT16)classPtr->cbWndExtra;
    wc->hInstance     = classPtr->style & CS_GLOBALCLASS ? GetModuleHandle16("USER") : HINSTANCE_16(classPtr->hInstance);
    wc->hIcon         = HICON_16(classPtr->hIcon);
    wc->hCursor       = HCURSOR_16(classPtr->hCursor);
    wc->hbrBackground = HBRUSH_16(classPtr->hbrBackground);
    wc->lpszClassName = name;
    wc->lpszMenuName  = CLASS_GetMenuName16( classPtr );

    /* We must return the atom of the class here instead of just TRUE. */
    return atom;
}


/***********************************************************************
 *		GetClassInfoA (USER32.@)
 */
BOOL WINAPI GetClassInfoA( HINSTANCE hInstance, LPCSTR name,
                               WNDCLASSA *wc )
{
    ATOM atom;
    CLASS *classPtr;

    TRACE("%p %p %p\n",hInstance, name, wc);

    /* workaround: if hInstance=NULL you expect to get the system classes
    but this classes (as example from comctl32.dll SysListView) won't be
    registered with hInstance=NULL in WINE because of the late loading
    of this dll. fixes file dialogs in WinWord95 (jsch)*/

    if (!(atom=GlobalFindAtomA(name)) || !(classPtr=CLASS_FindClassByAtom(atom,hInstance)))
        return FALSE;

    if (!(classPtr->style & CS_GLOBALCLASS) &&
        classPtr->hInstance &&
        (hInstance != classPtr->hInstance))
    {
        if (hInstance) return FALSE;
        WARN("systemclass %s (hInst=0) demanded but only class with hInst!=0 found\n",name);
    }

    wc->style         = classPtr->style;
    wc->lpfnWndProc   = (WNDPROC)CLASS_GetProc( classPtr, WIN_PROC_32A );
    wc->cbClsExtra    = classPtr->cbClsExtra;
    wc->cbWndExtra    = classPtr->cbWndExtra;
    wc->hInstance     = hInstance;
    wc->hIcon         = (HICON)classPtr->hIcon;
    wc->hCursor       = (HCURSOR)classPtr->hCursor;
    wc->hbrBackground = (HBRUSH)classPtr->hbrBackground;
    wc->lpszMenuName  = CLASS_GetMenuNameA( classPtr );
    wc->lpszClassName = name;

    /* We must return the atom of the class here instead of just TRUE. */
    return atom;
}


/***********************************************************************
 *		GetClassInfoW (USER32.@)
 */
BOOL WINAPI GetClassInfoW( HINSTANCE hInstance, LPCWSTR name,
                               WNDCLASSW *wc )
{
    ATOM atom;
    CLASS *classPtr;

    TRACE("%p %p %p\n",hInstance, name, wc);

    if (	!(atom=GlobalFindAtomW(name)) ||
		!(classPtr=CLASS_FindClassByAtom(atom,hInstance))
    )
        return FALSE;

    if (!(classPtr->style & CS_GLOBALCLASS) &&
        classPtr->hInstance &&
        (hInstance != classPtr->hInstance))
    {
        if (hInstance) return FALSE;
        WARN("systemclass %s (hInst=0) demanded but only class with hInst!=0 found\n",debugstr_w(name));
    }
    wc->style         = classPtr->style;
    wc->lpfnWndProc   = (WNDPROC)CLASS_GetProc( classPtr, WIN_PROC_32W );
    wc->cbClsExtra    = classPtr->cbClsExtra;
    wc->cbWndExtra    = classPtr->cbWndExtra;
    wc->hInstance     = hInstance;
    wc->hIcon         = (HICON)classPtr->hIcon;
    wc->hCursor       = (HCURSOR)classPtr->hCursor;
    wc->hbrBackground = (HBRUSH)classPtr->hbrBackground;
    wc->lpszMenuName  = CLASS_GetMenuNameW( classPtr );
    wc->lpszClassName = name;

    /* We must return the atom of the class here instead of just TRUE. */
    return atom;
}


/***********************************************************************
 *		GetClassInfoEx (USER.398)
 *
 * FIXME: this is just a guess, I have no idea if GetClassInfoEx() is the
 * same in Win16 as in Win32. --AJ
 */
BOOL16 WINAPI GetClassInfoEx16( HINSTANCE16 hInst16, SEGPTR name, WNDCLASSEX16 *wc )
{
    ATOM atom;
    CLASS *classPtr;
    HINSTANCE hInstance = HINSTANCE_32(GetExePtr( hInst16 ));

    TRACE("%p %s %p\n",hInstance,debugstr_a( MapSL(name) ), wc);

    if (!(atom = GlobalFindAtomA( MapSL(name) )) ||
        !(classPtr = CLASS_FindClassByAtom( atom, hInstance )) ||
        (hInstance != classPtr->hInstance)) return FALSE;
    wc->style         = classPtr->style;
    wc->lpfnWndProc   = CLASS_GetProc( classPtr, WIN_PROC_16 );
    wc->cbClsExtra    = (INT16)classPtr->cbClsExtra;
    wc->cbWndExtra    = (INT16)classPtr->cbWndExtra;
    wc->hInstance     = HINSTANCE_16(classPtr->hInstance);
    wc->hIcon         = HICON_16(classPtr->hIcon);
    wc->hIconSm       = HICON_16(classPtr->hIconSm);
    wc->hCursor       = HCURSOR_16(classPtr->hCursor);
    wc->hbrBackground = HBRUSH_16(classPtr->hbrBackground);
    wc->lpszClassName = (SEGPTR)0;
    wc->lpszMenuName  = CLASS_GetMenuName16( classPtr );
    wc->lpszClassName = name;

    /* We must return the atom of the class here instead of just TRUE. */
    return atom;
}


/***********************************************************************
 *		GetClassInfoExA (USER32.@)
 */
BOOL WINAPI GetClassInfoExA( HINSTANCE hInstance, LPCSTR name,
                                 WNDCLASSEXA *wc )
{
    ATOM atom;
    CLASS *classPtr;

    TRACE("%p %p %p\n",hInstance, name, wc);

    if (!(atom = GlobalFindAtomA( name )) ||
        !(classPtr = CLASS_FindClassByAtom( atom, hInstance ))
	/*|| (hInstance != classPtr->hInstance) */ ) return FALSE;
    wc->style         = classPtr->style;
    wc->lpfnWndProc   = (WNDPROC)CLASS_GetProc( classPtr, WIN_PROC_32A );
    wc->cbClsExtra    = classPtr->cbClsExtra;
    wc->cbWndExtra    = classPtr->cbWndExtra;
    wc->hInstance     = classPtr->hInstance;
    wc->hIcon         = (HICON)classPtr->hIcon;
    wc->hIconSm       = (HICON)classPtr->hIconSm;
    wc->hCursor       = (HCURSOR)classPtr->hCursor;
    wc->hbrBackground = (HBRUSH)classPtr->hbrBackground;
    wc->lpszMenuName  = CLASS_GetMenuNameA( classPtr );
    wc->lpszClassName = name;

    /* We must return the atom of the class here instead of just TRUE. */
    return atom;
}


/***********************************************************************
 *		GetClassInfoExW (USER32.@)
 */
BOOL WINAPI GetClassInfoExW( HINSTANCE hInstance, LPCWSTR name,
                                 WNDCLASSEXW *wc )
{
    ATOM atom;
    CLASS *classPtr;

    TRACE("%p %p %p\n",hInstance, name, wc);

    if (!(atom = GlobalFindAtomW( name )) ||
        !(classPtr = CLASS_FindClassByAtom( atom, hInstance )) ||
        (hInstance != classPtr->hInstance)) return FALSE;
    wc->style         = classPtr->style;
    wc->lpfnWndProc   = (WNDPROC)CLASS_GetProc( classPtr, WIN_PROC_32W );
    wc->cbClsExtra    = classPtr->cbClsExtra;
    wc->cbWndExtra    = classPtr->cbWndExtra;
    wc->hInstance     = classPtr->hInstance;
    wc->hIcon         = (HICON)classPtr->hIcon;
    wc->hIconSm       = (HICON)classPtr->hIconSm;
    wc->hCursor       = (HCURSOR)classPtr->hCursor;
    wc->hbrBackground = (HBRUSH)classPtr->hbrBackground;
    wc->lpszMenuName  = CLASS_GetMenuNameW( classPtr );
    wc->lpszClassName = name;

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
