/*
 * Window classes functions
 *
 * Copyright 1993, 1996 Alexandre Julliard
 *           1998 Juergen Schmied (jsch)
 *
 * FIXME: In win32 all classes are local. They are registered at 
 *	  program start. Processes CANNOT share classes. (Source: some
 *	  win31->NT migration book)
 *
 * FIXME: There seems to be a general problem with hInstance in WINE
 *   classes are getting registered with wrong hInstance.
 */

#include <stdlib.h>
#include <string.h>
#include "wine/winbase16.h"
#include "winerror.h"
#include "windef.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "wine/unicode.h"
#include "wine/port.h"
#include "heap.h"
#include "win.h"
#include "controls.h"
#include "dce.h"
#include "toolhelp.h"
#include "winproc.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(class);

typedef struct tagCLASS
{
    struct tagCLASS *next;          /* Next class */
    struct tagCLASS *prev;          /* Prev class */
    UINT             cWindows;      /* Count of existing windows */
    UINT             style;         /* Class style */
    HWINDOWPROC      winprocA;      /* Window procedure (ASCII) */
    HWINDOWPROC      winprocW;      /* Window procedure (Unicode) */
    INT              cbClsExtra;    /* Class extra bytes */
    INT              cbWndExtra;    /* Window extra bytes */
    LPWSTR           menuName;      /* Default menu name (Unicode followed by ASCII) */
    struct tagDCE   *dce;           /* Class DCE (if CS_CLASSDC) */
    HINSTANCE        hInstance;     /* Module that created the task */
    HICON            hIcon;         /* Default icon */
    HICON            hIconSm;       /* Default small icon */
    HCURSOR          hCursor;       /* Default cursor */
    HBRUSH           hbrBackground; /* Default background */
    ATOM             atomName;      /* Name of the class */
    LONG             wExtra[1];     /* Class extra bytes */
} CLASS;

static CLASS *firstClass;


/***********************************************************************
 *           CLASS_GetProc
 *
 * Get the class winproc for a given proc type
 */
static WNDPROC16 CLASS_GetProc( CLASS *classPtr, WINDOWPROCTYPE type )
{
    HWINDOWPROC proc = classPtr->winprocA;

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
    HWINDOWPROC *proc = &classPtr->winprocA;
    WNDPROC16 ret;

    if (classPtr->winprocW)
    {
        /* if we have a Unicode proc, use it if we have no ASCII proc
         * or if we have both and Unicode was requested
         */
        if (!*proc || type == WIN_PROC_32W) proc = &classPtr->winprocW;
    }
    ret = WINPROC_GetProc( *proc, type );
    WINPROC_SetProc( proc, (HWINDOWPROC)newproc, type, WIN_PROC_CLASS );
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
    if (HIWORD(classPtr->menuName)) SEGPTR_FREE( classPtr->menuName );
    if (HIWORD(name))
    {
        DWORD lenA = strlen(name) + 1;
        DWORD lenW = MultiByteToWideChar( CP_ACP, 0, name, lenA, NULL, 0 );
        classPtr->menuName = SEGPTR_ALLOC( lenA + lenW*sizeof(WCHAR) );
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
    if (HIWORD(classPtr->menuName)) SEGPTR_FREE( classPtr->menuName );
    if (HIWORD(name))
    {
        DWORD lenW = strlenW(name) + 1;
        DWORD lenA = WideCharToMultiByte( CP_ACP, 0, name, lenW, NULL, 0, NULL, NULL );
        classPtr->menuName = SEGPTR_ALLOC( lenA + lenW*sizeof(WCHAR) );
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

    for (ptr = firstClass; ptr; ptr = next)
    {
        next = ptr->next;
	if (ptr->hInstance == hModule) CLASS_FreeClass( ptr );
    }
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
 */
static CLASS *CLASS_FindClassByAtom( ATOM atom, HINSTANCE hinstance )
{
    CLASS * class, *tclass=0;

    TRACE("0x%08x 0x%08x\n", atom, hinstance);

    /* First search task-specific classes */

    for (class = firstClass; (class); class = class->next)
    {
        if (class->style & CS_GLOBALCLASS) continue;
        if (class->atomName == atom)
        {
            if (hinstance==class->hInstance || hinstance==0xffff )
            {
                TRACE("-- found local %p\n", class);
                return class;
            }
            if (class->hInstance==0) tclass = class;
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

    TRACE("atom=0x%x hinst=0x%x style=0x%lx clExtr=0x%x winExtr=0x%x\n",
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

    classPtr = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                          sizeof(CLASS) + classExtra - sizeof(classPtr->wExtra) );
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

    classPtr->hCursor       = LoadCursorA( 0, descr->cursor );
    classPtr->hbrBackground = descr->brush;

    if (descr->procA) WINPROC_SetProc( &classPtr->winprocA, (HWINDOWPROC)descr->procA,
                                       WIN_PROC_32A, WIN_PROC_CLASS );
    if (descr->procW) WINPROC_SetProc( &classPtr->winprocW, (HWINDOWPROC)descr->procW,
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
    if (type == WIN_PROC_16) inst = GetExePtr(inst);

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
    HINSTANCE16 hInstance=GetExePtr(wc->hInstance);

    if (!(atom = GlobalAddAtomA( MapSL(wc->lpszClassName) ))) return 0;
    if (!(classPtr = CLASS_RegisterClass( atom, hInstance, wc->style,
                                          wc->cbClsExtra, wc->cbWndExtra )))
    {
        GlobalDeleteAtom( atom );
        return 0;
    }

    TRACE("atom=%04x wndproc=%08lx hinst=%04x "
                 "bg=%04x style=%08x clsExt=%d winExt=%d class=%p name='%s'\n",
                   atom, (DWORD)wc->lpfnWndProc, hInstance,
                   wc->hbrBackground, wc->style, wc->cbClsExtra,
                   wc->cbWndExtra, classPtr,
		   HIWORD(wc->lpszClassName) ?
                       (char *)MapSL(wc->lpszClassName) : "" );

    iSmIconWidth  = GetSystemMetrics(SM_CXSMICON);
    iSmIconHeight = GetSystemMetrics(SM_CYSMICON);

    classPtr->hIcon         = wc->hIcon;
    classPtr->hIconSm       = CopyImage(wc->hIcon, IMAGE_ICON,
					iSmIconWidth, iSmIconHeight,
					LR_COPYFROMRESOURCE);
    classPtr->hCursor       = wc->hCursor;
    classPtr->hbrBackground = wc->hbrBackground;

    WINPROC_SetProc( &classPtr->winprocA, (HWINDOWPROC)wc->lpfnWndProc,
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

    TRACE("atom=%04x wndproc=%08lx hinst=%04x bg=%04x style=%08x clsExt=%d winExt=%d class=%p name='%s'\n",
                   atom, (DWORD)wc->lpfnWndProc, wc->hInstance,
                   wc->hbrBackground, wc->style, wc->cbClsExtra,
                   wc->cbWndExtra, classPtr,
                   HIWORD(wc->lpszClassName) ? wc->lpszClassName : "" );
    
    iSmIconWidth  = GetSystemMetrics(SM_CXSMICON);
    iSmIconHeight = GetSystemMetrics(SM_CYSMICON);
    
    classPtr->hIcon         = wc->hIcon;
    classPtr->hIconSm       = CopyImage(wc->hIcon, IMAGE_ICON,
					iSmIconWidth, iSmIconHeight,
					LR_COPYFROMRESOURCE);
    classPtr->hCursor       = (HCURSOR16)wc->hCursor;
    classPtr->hbrBackground = (HBRUSH16)wc->hbrBackground;

    WINPROC_SetProc( &classPtr->winprocA, (HWINDOWPROC)wc->lpfnWndProc,
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

    TRACE("atom=%04x wndproc=%08lx hinst=%04x bg=%04x style=%08x clsExt=%d winExt=%d class=%p\n",
                   atom, (DWORD)wc->lpfnWndProc, wc->hInstance,
                   wc->hbrBackground, wc->style, wc->cbClsExtra,
                   wc->cbWndExtra, classPtr );
    
    iSmIconWidth  = GetSystemMetrics(SM_CXSMICON);
    iSmIconHeight = GetSystemMetrics(SM_CYSMICON);

    classPtr->hIcon         = wc->hIcon;
    classPtr->hIconSm       = CopyImage(wc->hIcon, IMAGE_ICON,
					iSmIconWidth, iSmIconHeight,
					LR_COPYFROMRESOURCE);
    classPtr->hCursor       = (HCURSOR16)wc->hCursor;
    classPtr->hbrBackground = (HBRUSH16)wc->hbrBackground;

    WINPROC_SetProc( &classPtr->winprocW, (HWINDOWPROC)wc->lpfnWndProc,
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
    HINSTANCE16 hInstance = GetExePtr( wc->hInstance );

    if (!(atom = GlobalAddAtomA( MapSL(wc->lpszClassName) ))) return 0;
    if (!(classPtr = CLASS_RegisterClass( atom, hInstance, wc->style,
                                          wc->cbClsExtra, wc->cbWndExtra )))
    {
        GlobalDeleteAtom( atom );
        return 0;
    }

    TRACE("atom=%04x wndproc=%08lx hinst=%04x bg=%04x style=%08x clsExt=%d winExt=%d class=%p\n",
                   atom, (DWORD)wc->lpfnWndProc, hInstance,
                   wc->hbrBackground, wc->style, wc->cbClsExtra,
                   wc->cbWndExtra, classPtr );
    
    classPtr->hIcon         = wc->hIcon;
    classPtr->hIconSm       = wc->hIconSm;
    classPtr->hCursor       = wc->hCursor;
    classPtr->hbrBackground = wc->hbrBackground;

    WINPROC_SetProc( &classPtr->winprocA, (HWINDOWPROC)wc->lpfnWndProc,
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

    TRACE("atom=%04x wndproc=%08lx hinst=%04x bg=%04x style=%08x clsExt=%d winExt=%d class=%p\n",
                   atom, (DWORD)wc->lpfnWndProc, wc->hInstance,
                   wc->hbrBackground, wc->style, wc->cbClsExtra,
                   wc->cbWndExtra, classPtr );
    
    classPtr->hIcon         = (HICON16)wc->hIcon;
    classPtr->hIconSm       = (HICON16)wc->hIconSm;
    classPtr->hCursor       = (HCURSOR16)wc->hCursor;
    classPtr->hbrBackground = (HBRUSH16)wc->hbrBackground;
    WINPROC_SetProc( &classPtr->winprocA, (HWINDOWPROC)wc->lpfnWndProc,
                     WIN_PROC_32A, WIN_PROC_CLASS );
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

    TRACE("atom=%04x wndproc=%08lx hinst=%04x bg=%04x style=%08x clsExt=%d winExt=%d class=%p\n",
                   atom, (DWORD)wc->lpfnWndProc, wc->hInstance,
                   wc->hbrBackground, wc->style, wc->cbClsExtra,
                   wc->cbWndExtra, classPtr );
    
    classPtr->hIcon         = (HICON16)wc->hIcon;
    classPtr->hIconSm       = (HICON16)wc->hIconSm;
    classPtr->hCursor       = (HCURSOR16)wc->hCursor;
    classPtr->hbrBackground = (HBRUSH16)wc->hbrBackground;
    WINPROC_SetProc( &classPtr->winprocW, (HWINDOWPROC)wc->lpfnWndProc,
                     WIN_PROC_32W, WIN_PROC_CLASS );
    CLASS_SetMenuNameW( classPtr, wc->lpszMenuName );
    return atom;
}


/***********************************************************************
 *		UnregisterClass (USER.403)
 */
BOOL16 WINAPI UnregisterClass16( LPCSTR className, HINSTANCE16 hInstance )
{
    return UnregisterClassA( className, GetExePtr( hInstance ) );
}


/***********************************************************************
 *		UnregisterClassA (USER32.@)
 *
 */
BOOL WINAPI UnregisterClassA( LPCSTR className, HINSTANCE hInstance )
{
    CLASS *classPtr;
    ATOM atom;

    TRACE("%s %x\n",debugres_a(className), hInstance);

    if (!(atom = GlobalFindAtomA( className )))
    {
        SetLastError(ERROR_CLASS_DOES_NOT_EXIST);
        return FALSE;
    }
    if (!(classPtr = CLASS_FindClassByAtom( atom, hInstance )) ||
        (classPtr->hInstance != hInstance))
    {
        SetLastError(ERROR_CLASS_DOES_NOT_EXIST);
        return FALSE;
    }
    return CLASS_FreeClass( classPtr );
}

/***********************************************************************
 *		UnregisterClassW (USER32.@)
 */
BOOL WINAPI UnregisterClassW( LPCWSTR className, HINSTANCE hInstance )
{
    CLASS *classPtr;
    ATOM atom;

    TRACE("%s %x\n",debugres_w(className), hInstance);

    if (!(atom = GlobalFindAtomW( className )))
    {
        SetLastError(ERROR_CLASS_DOES_NOT_EXIST);
        return FALSE;
    }
    if (!(classPtr = CLASS_FindClassByAtom( atom, hInstance )) ||
        (classPtr->hInstance != hInstance))
    {
        SetLastError(ERROR_CLASS_DOES_NOT_EXIST);
        return FALSE;
    }
    return CLASS_FreeClass( classPtr );
}

/***********************************************************************
 *		GetClassWord (USER.129)
 */
WORD WINAPI GetClassWord16( HWND16 hwnd, INT16 offset )
{
    return GetClassWord( hwnd, offset );
}


/***********************************************************************
 *		GetClassWord (USER32.@)
 */
WORD WINAPI GetClassWord( HWND hwnd, INT offset )
{
    WND * wndPtr;
    WORD retvalue = 0;
    
    TRACE("%x %x\n",hwnd, offset);
    
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
    if (offset >= 0)
    {
        if (offset <= wndPtr->class->cbClsExtra - sizeof(WORD))
        {
            retvalue = GET_WORD(((char *)wndPtr->class->wExtra) + offset);
            goto END;
        }
    }
    else switch(offset)
    {
        case GCW_HBRBACKGROUND: retvalue =  wndPtr->class->hbrBackground;
                                goto END;
        case GCW_HCURSOR:       retvalue =  wndPtr->class->hCursor;
                                goto END;
        case GCW_HICON:         retvalue = wndPtr->class->hIcon;
                                goto END;
        case GCW_HICONSM:       retvalue = wndPtr->class->hIconSm;
                                goto END;
        case GCW_ATOM:          retvalue =  wndPtr->class->atomName;
                                goto END;
        case GCW_STYLE:
        case GCW_CBWNDEXTRA:
        case GCW_CBCLSEXTRA:
        case GCW_HMODULE:
            retvalue = (WORD)GetClassLongA( hwnd, offset );
            goto END;
    }

    WARN("Invalid offset %d\n", offset);
 END:
    WIN_ReleaseWndPtr(wndPtr);
    return retvalue;
}


/***********************************************************************
 *		GetClassLong (USER.131)
 */
LONG WINAPI GetClassLong16( HWND16 hwnd, INT16 offset )
{
    WND *wndPtr;
    LONG ret;

    TRACE("%x %x\n",hwnd, offset);

    switch( offset )
    {
    case GCL_WNDPROC:
        if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
        ret = (LONG)CLASS_GetProc( wndPtr->class, WIN_PROC_16 );
        WIN_ReleaseWndPtr(wndPtr);
        return ret;
    case GCL_MENUNAME:
        ret = GetClassLongA( hwnd, offset );
        return (LONG)SEGPTR_GET( (void *)ret );
    default:
        return GetClassLongA( hwnd, offset );
    }
}


/***********************************************************************
 *		GetClassLongA (USER32.@)
 */
LONG WINAPI GetClassLongA( HWND hwnd, INT offset )
{
    WND * wndPtr;
    LONG retvalue;
    
    TRACE("%x %x\n",hwnd, offset);
    
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
    if (offset >= 0)
    {
        if (offset <= wndPtr->class->cbClsExtra - sizeof(LONG))
        {
            retvalue = GET_DWORD(((char *)wndPtr->class->wExtra) + offset);
            goto END;
        }
    }
        
    switch(offset)
    {
        case GCL_STYLE:      retvalue = (LONG)wndPtr->class->style;
                             goto END;
        case GCL_CBWNDEXTRA: retvalue = (LONG)wndPtr->class->cbWndExtra;
                             goto END;
        case GCL_CBCLSEXTRA: retvalue = (LONG)wndPtr->class->cbClsExtra;
                             goto END;
        case GCL_HMODULE:    retvalue = (LONG)wndPtr->class->hInstance;
                             goto END;
        case GCL_WNDPROC:
            retvalue = (LONG)CLASS_GetProc( wndPtr->class, WIN_PROC_32A );
            goto END;
        case GCL_MENUNAME:
            retvalue = (LONG)CLASS_GetMenuNameA( wndPtr->class );
            goto END;
        case GCW_ATOM:
        case GCL_HBRBACKGROUND:
        case GCL_HCURSOR:
        case GCL_HICON:
        case GCL_HICONSM:
            retvalue = GetClassWord( hwnd, offset );
            goto END;
    }
    WARN("Invalid offset %d\n", offset);
    retvalue = 0;
END:
    WIN_ReleaseWndPtr(wndPtr);
    return retvalue;
}


/***********************************************************************
 *		GetClassLongW (USER32.@)
 */
LONG WINAPI GetClassLongW( HWND hwnd, INT offset )
{
    WND * wndPtr;
    LONG retvalue;

    TRACE("%x %x\n",hwnd, offset);

    switch(offset)
    {
    case GCL_WNDPROC:
        if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
        retvalue = (LONG)CLASS_GetProc( wndPtr->class, WIN_PROC_32W );
        WIN_ReleaseWndPtr(wndPtr);
        return retvalue;
    case GCL_MENUNAME:
        if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
        retvalue = (LONG)CLASS_GetMenuNameW( wndPtr->class );
        WIN_ReleaseWndPtr(wndPtr);
        return retvalue;
    default:
        return GetClassLongA( hwnd, offset );
    }
}


/***********************************************************************
 *		SetClassWord (USER.130)
 */
WORD WINAPI SetClassWord16( HWND16 hwnd, INT16 offset, WORD newval )
{
    return SetClassWord( hwnd, offset, newval );
}


/***********************************************************************
 *		SetClassWord (USER32.@)
 */
WORD WINAPI SetClassWord( HWND hwnd, INT offset, WORD newval )
{
    WND * wndPtr;
    WORD retval = 0;
    void *ptr;
    
    TRACE("%x %x %x\n",hwnd, offset, newval);
    
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
    if (offset >= 0)
    {
        if (offset + sizeof(WORD) <= wndPtr->class->cbClsExtra)
            ptr = ((char *)wndPtr->class->wExtra) + offset;
        else
        {
            WARN("Invalid offset %d\n", offset );
            WIN_ReleaseWndPtr(wndPtr);
            return 0;
        }
    }
    else switch(offset)
    {
        case GCW_STYLE:
        case GCW_CBWNDEXTRA:
        case GCW_CBCLSEXTRA:
        case GCW_HMODULE:
            WIN_ReleaseWndPtr(wndPtr);
            return (WORD)SetClassLongA( hwnd, offset, (LONG)newval );
        case GCW_HBRBACKGROUND: ptr = &wndPtr->class->hbrBackground; break;
        case GCW_HCURSOR:       ptr = &wndPtr->class->hCursor; break;
        case GCW_HICON:         ptr = &wndPtr->class->hIcon; break;
        case GCW_HICONSM:       ptr = &wndPtr->class->hIconSm; break;
        case GCW_ATOM:          ptr = &wndPtr->class->atomName; break;
        default:
            WARN("Invalid offset %d\n", offset);
            WIN_ReleaseWndPtr(wndPtr);
            return 0;
    }
    retval = GET_WORD(ptr);
    PUT_WORD( ptr, newval );
    WIN_ReleaseWndPtr(wndPtr);
    return retval;
}


/***********************************************************************
 *		SetClassLong (USER.132)
 */
LONG WINAPI SetClassLong16( HWND16 hwnd, INT16 offset, LONG newval )
{
    WND *wndPtr;
    LONG retval;

    TRACE("%x %x %lx\n",hwnd, offset, newval);

    switch(offset)
    {
    case GCL_WNDPROC:
        if (!(wndPtr = WIN_FindWndPtr(hwnd))) return 0;
        retval = (LONG)CLASS_SetProc( wndPtr->class, (WNDPROC)newval, WIN_PROC_16 );
        WIN_ReleaseWndPtr(wndPtr);
        return retval;
    case GCL_MENUNAME:
        return SetClassLongA( hwnd, offset, (LONG)MapSL(newval) );
    default:
        return SetClassLongA( hwnd, offset, newval );
    }
}


/***********************************************************************
 *		SetClassLongA (USER32.@)
 */
LONG WINAPI SetClassLongA( HWND hwnd, INT offset, LONG newval )
{
    WND * wndPtr;
    LONG retval = 0;
    void *ptr;
    
    TRACE("%x %x %lx\n",hwnd, offset, newval);
        
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
    if (offset >= 0)
    {
        if (offset + sizeof(LONG) <= wndPtr->class->cbClsExtra)
            ptr = ((char *)wndPtr->class->wExtra) + offset;
        else
        {
            WARN("Invalid offset %d\n", offset );
            retval = 0;
            goto END;
        }
    }
    else switch(offset)
    {
        case GCL_MENUNAME:
            CLASS_SetMenuNameA( wndPtr->class, (LPCSTR)newval );
            retval = 0;  /* Old value is now meaningless anyway */
            goto END;
        case GCL_WNDPROC:
            retval = (LONG)CLASS_SetProc( wndPtr->class, (WNDPROC)newval, WIN_PROC_32A );
            goto END;
        case GCL_HBRBACKGROUND:
        case GCL_HCURSOR:
        case GCL_HICON:
        case GCL_HICONSM:
            retval = SetClassWord( hwnd, offset, (WORD)newval );
            goto END;
        case GCL_STYLE:      ptr = &wndPtr->class->style; break;
        case GCL_CBWNDEXTRA: ptr = &wndPtr->class->cbWndExtra; break;
        case GCL_CBCLSEXTRA: ptr = &wndPtr->class->cbClsExtra; break;
        case GCL_HMODULE:    ptr = &wndPtr->class->hInstance; break;
        default:
            WARN("Invalid offset %d\n", offset );
            retval = 0;
            goto END;
    }
    retval = GET_DWORD(ptr);
    PUT_DWORD( ptr, newval );
END:
    WIN_ReleaseWndPtr(wndPtr);
    return retval;
}


/***********************************************************************
 *		SetClassLongW (USER32.@)
 */
LONG WINAPI SetClassLongW( HWND hwnd, INT offset, LONG newval )
{
    WND *wndPtr;
    LONG retval;

    TRACE("%x %x %lx\n",hwnd, offset, newval);
    
    switch(offset)
    {
    case GCL_WNDPROC:
        if (!(wndPtr = WIN_FindWndPtr(hwnd))) return 0;
        retval = (LONG)CLASS_SetProc( wndPtr->class, (WNDPROC)newval, WIN_PROC_32W );
        WIN_ReleaseWndPtr(wndPtr);
        return retval;
    case GCL_MENUNAME:
        if (!(wndPtr = WIN_FindWndPtr(hwnd))) return 0;
        CLASS_SetMenuNameW( wndPtr->class, (LPCWSTR)newval );
        WIN_ReleaseWndPtr(wndPtr);
        return 0;  /* Old value is now meaningless anyway */
    default:
        return SetClassLongA( hwnd, offset, newval );
    }
}


/***********************************************************************
 *		GetClassName (USER.58)
 */
INT16 WINAPI GetClassName16( HWND16 hwnd, LPSTR buffer, INT16 count )
{
    return GetClassNameA( hwnd, buffer, count );
}


/***********************************************************************
 *		GetClassNameA (USER32.@)
 */
INT WINAPI GetClassNameA( HWND hwnd, LPSTR buffer, INT count )
{   INT ret;
    WND *wndPtr;
            
    if (!(wndPtr = WIN_FindWndPtr(hwnd))) return 0;
    ret = GlobalGetAtomNameA( wndPtr->class->atomName, buffer, count );

    WIN_ReleaseWndPtr(wndPtr);
    TRACE("%x %s %x\n",hwnd, buffer, count);
    return ret;
}


/***********************************************************************
 *		GetClassNameW (USER32.@)
 */
INT WINAPI GetClassNameW( HWND hwnd, LPWSTR buffer, INT count )
{   INT ret;
    WND *wndPtr;

    if (!(wndPtr = WIN_FindWndPtr(hwnd))) return 0;
    ret = GlobalGetAtomNameW( wndPtr->class->atomName, buffer, count );
    WIN_ReleaseWndPtr(wndPtr);
    TRACE("%x %s %x\n",hwnd, debugstr_w(buffer), count);
    
    return ret;
}


/***********************************************************************
 *		GetClassInfo (USER.404)
 */
BOOL16 WINAPI GetClassInfo16( HINSTANCE16 hInstance, SEGPTR name, WNDCLASS16 *wc )
{
    ATOM atom;
    CLASS *classPtr;

    TRACE("%x %s %p\n",hInstance, debugres_a(MapSL(name)), wc);

    hInstance = GetExePtr( hInstance );
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
    wc->hInstance     = (HINSTANCE16)classPtr->hInstance;
    wc->hIcon         = classPtr->hIcon;
    wc->hCursor       = classPtr->hCursor;
    wc->hbrBackground = classPtr->hbrBackground;
    wc->lpszClassName = name;
    wc->lpszMenuName  = (SEGPTR)CLASS_GetMenuNameA( classPtr );
    if (HIWORD(wc->lpszMenuName))  /* Make it a SEGPTR */
        wc->lpszMenuName = SEGPTR_GET( (LPSTR)wc->lpszMenuName );
    return TRUE;
}


/***********************************************************************
 *		GetClassInfoA (USER32.@)
 */
BOOL WINAPI GetClassInfoA( HINSTANCE hInstance, LPCSTR name,
                               WNDCLASSA *wc )
{
    ATOM atom;
    CLASS *classPtr;

    TRACE("%x %p %p\n",hInstance, name, wc);

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
    return TRUE;
}


/***********************************************************************
 *		GetClassInfoW (USER32.@)
 */
BOOL WINAPI GetClassInfoW( HINSTANCE hInstance, LPCWSTR name,
                               WNDCLASSW *wc )
{
    ATOM atom;
    CLASS *classPtr;

    TRACE("%x %p %p\n",hInstance, name, wc);

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
    return TRUE;
}


/***********************************************************************
 *		GetClassInfoEx (USER.398)
 *
 * FIXME: this is just a guess, I have no idea if GetClassInfoEx() is the
 * same in Win16 as in Win32. --AJ
 */
BOOL16 WINAPI GetClassInfoEx16( HINSTANCE16 hInstance, SEGPTR name, WNDCLASSEX16 *wc )
{
    ATOM atom;
    CLASS *classPtr;

    TRACE("%x %s %p\n",hInstance,debugres_a( MapSL(name) ), wc);

    hInstance = GetExePtr( hInstance );
    if (!(atom = GlobalFindAtomA( MapSL(name) )) ||
        !(classPtr = CLASS_FindClassByAtom( atom, hInstance )) ||
        (hInstance != classPtr->hInstance)) return FALSE;
    wc->style         = classPtr->style;
    wc->lpfnWndProc   = CLASS_GetProc( classPtr, WIN_PROC_16 );
    wc->cbClsExtra    = (INT16)classPtr->cbClsExtra;
    wc->cbWndExtra    = (INT16)classPtr->cbWndExtra;
    wc->hInstance     = (HINSTANCE16)classPtr->hInstance;
    wc->hIcon         = classPtr->hIcon;
    wc->hIconSm       = classPtr->hIconSm;
    wc->hCursor       = classPtr->hCursor;
    wc->hbrBackground = classPtr->hbrBackground;
    wc->lpszClassName = (SEGPTR)0;
    wc->lpszMenuName  = (SEGPTR)CLASS_GetMenuNameA( classPtr );
    if (HIWORD(wc->lpszMenuName))  /* Make it a SEGPTR */
        wc->lpszMenuName = SEGPTR_GET( (LPSTR)wc->lpszMenuName );
    wc->lpszClassName  = name;

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

    TRACE("%x %p %p\n",hInstance, name, wc);
    
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

    TRACE("%x %p %p\n",hInstance, name, wc);
    
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
