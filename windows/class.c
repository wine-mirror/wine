/*
 * Window classes functions
 *
 * Copyright 1993, 1996 Alexandre Julliard
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "class.h"
#include "heap.h"
#include "win.h"
#include "dce.h"
#include "atom.h"
#include "ldt.h"
#include "toolhelp.h"
#include "winproc.h"
#include "stddebug.h"
#include "debug.h"


static CLASS *firstClass = NULL;


/***********************************************************************
 *           CLASS_DumpClass
 *
 * Dump the content of a class structure to stderr.
 */
void CLASS_DumpClass( CLASS *ptr )
{
    char className[80];
    int i;

    if (ptr->magic != CLASS_MAGIC)
    {
        fprintf( stderr, "%p is not a class\n", ptr );
        return;
    }

    GlobalGetAtomName32A( ptr->atomName, className, sizeof(className) );

    fprintf( stderr, "Class %p:\n", ptr );
    fprintf( stderr,
             "next=%p  name=%04x '%s'  style=%08x  wndProc=%08x\n"
             "inst=%04x  dce=%08x  icon=%04x  cursor=%04x  bkgnd=%04x\n"
             "clsExtra=%d  winExtra=%d  #windows=%d\n",
             ptr->next, ptr->atomName, className, ptr->style,
             (UINT32)ptr->winproc, ptr->hInstance, (UINT32)ptr->dce,
             ptr->hIcon, ptr->hCursor, ptr->hbrBackground,
             ptr->cbClsExtra, ptr->cbWndExtra, ptr->cWindows );
    if (ptr->cbClsExtra)
    {
        fprintf( stderr, "extra bytes:" );
        for (i = 0; i < ptr->cbClsExtra; i++)
            fprintf( stderr, " %02x", *((BYTE *)ptr->wExtra+i) );
        fprintf( stderr, "\n" );
    }
    fprintf( stderr, "\n" );
}


/***********************************************************************
 *           CLASS_WalkClasses
 *
 * Walk the class list and print each class on stderr.
 */
void CLASS_WalkClasses(void)
{
    CLASS *ptr;
    char className[80];

    fprintf( stderr, " Class   Name                  Style   WndProc\n" );
    for (ptr = firstClass; ptr; ptr = ptr->next)
    {
        GlobalGetAtomName32A( ptr->atomName, className, sizeof(className) );
        fprintf( stderr, "%08x %-20.20s %08x %08x\n", (UINT32)ptr, className,
                 ptr->style, (UINT32)ptr->winproc );
    }
    fprintf( stderr, "\n" );
}


/***********************************************************************
 *           CLASS_GetMenuNameA
 *
 * Get the menu name as a ASCII string.
 */
static LPSTR CLASS_GetMenuNameA( CLASS *classPtr )
{
    if (!classPtr->menuNameA && classPtr->menuNameW)
    {
        /* We need to copy the Unicode string */
        classPtr->menuNameA = SEGPTR_STRDUP_WtoA( classPtr->menuNameW );
    }
    return classPtr->menuNameA;
}


/***********************************************************************
 *           CLASS_GetMenuNameW
 *
 * Get the menu name as a Unicode string.
 */
static LPWSTR CLASS_GetMenuNameW( CLASS *classPtr )
{
    if (!classPtr->menuNameW && classPtr->menuNameA)
    {
        if (!HIWORD(classPtr->menuNameA))
            return (LPWSTR)classPtr->menuNameA;
        /* Now we need to copy the ASCII string */
        classPtr->menuNameW = HEAP_strdupAtoW( SystemHeap, 0,
                                               classPtr->menuNameA );
    }
    return classPtr->menuNameW;
}


/***********************************************************************
 *           CLASS_SetMenuNameA
 *
 * Set the menu name in a class structure by copying the string.
 */
static void CLASS_SetMenuNameA( CLASS *classPtr, LPCSTR name )
{
    if (HIWORD(classPtr->menuNameA)) SEGPTR_FREE( classPtr->menuNameA );
    if (classPtr->menuNameW) HeapFree( SystemHeap, 0, classPtr->menuNameW );
    classPtr->menuNameA = SEGPTR_STRDUP( name );
    classPtr->menuNameW = 0;
}


/***********************************************************************
 *           CLASS_SetMenuNameW
 *
 * Set the menu name in a class structure by copying the string.
 */
static void CLASS_SetMenuNameW( CLASS *classPtr, LPCWSTR name )
{
    if (!HIWORD(name))
    {
        CLASS_SetMenuNameA( classPtr, (LPCSTR)name );
        return;
    }
    if (HIWORD(classPtr->menuNameA)) SEGPTR_FREE( classPtr->menuNameA );
    if (classPtr->menuNameW) HeapFree( SystemHeap, 0, classPtr->menuNameW );
    if ((classPtr->menuNameW = HeapAlloc( SystemHeap, 0,
                                         (lstrlen32W(name)+1)*sizeof(WCHAR) )))
        lstrcpy32W( classPtr->menuNameW, name );
    classPtr->menuNameA = 0;
}


/***********************************************************************
 *           CLASS_FreeClass
 *
 * Free a class structure.
 */
static BOOL CLASS_FreeClass( CLASS *classPtr )
{
    CLASS **ppClass;

    /* Check if we can remove this class */

    if (classPtr->cWindows > 0) return FALSE;

    /* Remove the class from the linked list */

    for (ppClass = &firstClass; *ppClass; ppClass = &(*ppClass)->next)
        if (*ppClass == classPtr) break;
    if (!*ppClass)
    {
        fprintf(stderr, "ERROR: Class list corrupted\n" );
        return FALSE;
    }
    *ppClass = classPtr->next;

    /* Delete the class */

    if (classPtr->dce) DCE_FreeDCE( classPtr->dce );
    if (classPtr->hbrBackground) DeleteObject32( classPtr->hbrBackground );
    GlobalDeleteAtom( classPtr->atomName );
    CLASS_SetMenuNameA( classPtr, NULL );
    WINPROC_FreeProc( classPtr->winproc );
    HeapFree( SystemHeap, 0, classPtr );
    return TRUE;
}


/***********************************************************************
 *           CLASS_FreeModuleClasses
 */
void CLASS_FreeModuleClasses( HMODULE16 hModule )
{
    CLASS *ptr, *next;
  
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
 */
CLASS *CLASS_FindClassByAtom( ATOM atom, HINSTANCE16 hinstance )
{
    CLASS * class;

    if (hinstance != 0xffff) hinstance = GetExePtr(hinstance);

      /* First search task-specific classes */

    for (class = firstClass; (class); class = class->next)
    {
        if (class->style & CS_GLOBALCLASS) continue;
        if ((class->atomName == atom) && 
            ((hinstance == 0xffff) ||
             (hinstance == class->hInstance))) return class;
    }
    
      /* Then search global classes */

    for (class = firstClass; (class); class = class->next)
    {
        if (!(class->style & CS_GLOBALCLASS)) continue;
        if (class->atomName == atom) return class;
    }

    return 0;
}


/***********************************************************************
 *           CLASS_RegisterClass
 *
 * The real RegisterClass() functionality.
 */
static CLASS *CLASS_RegisterClass( ATOM atom, HINSTANCE32 hInstance,
                                   DWORD style, INT32 classExtra,
                                   INT32 winExtra, WNDPROC16 wndProc,
                                   WINDOWPROCTYPE wndProcType )
{
    CLASS *classPtr;

    /* Check if a class with this name already exists */

    classPtr = CLASS_FindClassByAtom( atom, hInstance );
    if (classPtr)
    {
        /* Class can be created only if it is local and */
        /* if the class with the same name is global.   */

	if (style & CS_GLOBALCLASS) return NULL;
        if (!(classPtr->style & CS_GLOBALCLASS)) return NULL;
    }

    /* Fix the extra bytes value */

    if (classExtra < 0) classExtra = 0;
    else if (classExtra > 40)  /* Extra bytes are limited to 40 in Win32 */
        fprintf(stderr, "Warning: class extra bytes %d is > 40\n", classExtra);
    if (winExtra < 0) winExtra = 0;
    else if (winExtra > 40)    /* Extra bytes are limited to 40 in Win32 */
        fprintf( stderr, "Warning: win extra bytes %d is > 40\n", winExtra );

    /* Create the class */

    classPtr = (CLASS *)HeapAlloc( SystemHeap, 0, sizeof(CLASS) +
                                       classExtra - sizeof(classPtr->wExtra) );
    if (!classPtr) return NULL;
    classPtr->next        = firstClass;
    classPtr->magic       = CLASS_MAGIC;
    classPtr->cWindows    = 0;  
    classPtr->style       = style;
    classPtr->winproc     = (HWINDOWPROC)0;
    classPtr->cbWndExtra  = winExtra;
    classPtr->cbClsExtra  = classExtra;
    classPtr->hInstance   = hInstance;
    classPtr->atomName    = atom;
    classPtr->menuNameA   = 0;
    classPtr->menuNameW   = 0;
    classPtr->dce         = (style & CS_CLASSDC) ?
                                 DCE_AllocDCE( 0, DCE_CLASS_DC ) : NULL;

    WINPROC_SetProc( &classPtr->winproc, wndProc, wndProcType );

    /* Other values must be set by caller */

    if (classExtra) memset( classPtr->wExtra, 0, classExtra );
    firstClass = classPtr;
    return classPtr;
}


/***********************************************************************
 *           RegisterClass16    (USER.57)
 */
ATOM RegisterClass16( const WNDCLASS16 *wc )
{
    ATOM atom;
    CLASS *classPtr;

    HINSTANCE32 hInstance = (HINSTANCE32)GetExePtr( wc->hInstance );

    if (!(atom = GlobalAddAtom16( wc->lpszClassName ))) return 0;
    if (!(classPtr = CLASS_RegisterClass( atom, hInstance, wc->style,
                                          wc->cbClsExtra, wc->cbWndExtra,
                                          wc->lpfnWndProc, WIN_PROC_16 )))
    {
        GlobalDeleteAtom( atom );
        return 0;
    }

    dprintf_class( stddeb, "RegisterClass16: atom=%04x wndproc=%08lx hinst=%04x bg=%04x style=%08x clsExt=%d winExt=%d class=%p\n",
                   atom, (DWORD)wc->lpfnWndProc, hInstance,
                   wc->hbrBackground, wc->style, wc->cbClsExtra,
                   wc->cbWndExtra, classPtr );

    classPtr->hIcon         = wc->hIcon;
    classPtr->hIconSm       = 0;
    classPtr->hCursor       = wc->hCursor;
    classPtr->hbrBackground = wc->hbrBackground;

    CLASS_SetMenuNameA( classPtr, HIWORD(wc->lpszMenuName) ?
                 PTR_SEG_TO_LIN(wc->lpszMenuName) : (LPCSTR)wc->lpszMenuName );
    return atom;
}


/***********************************************************************
 *           RegisterClass32A      (USER32.426)
 */
ATOM RegisterClass32A( const WNDCLASS32A* wc )
{
    ATOM atom;
    CLASS *classPtr;

    /* FIXME: this should not be necessary for Win32 */
    HINSTANCE32 hInstance = (HINSTANCE32)GetExePtr( wc->hInstance );

    if (!(atom = GlobalAddAtom32A( wc->lpszClassName ))) return 0;
    if (!(classPtr = CLASS_RegisterClass( atom, hInstance, wc->style,
                                          wc->cbClsExtra, wc->cbWndExtra,
                                          (WNDPROC16)wc->lpfnWndProc,
                                          WIN_PROC_32A )))
    {
        GlobalDeleteAtom( atom );
        return 0;
    }

    dprintf_class( stddeb, "RegisterClass32A: atom=%04x wndproc=%08lx hinst=%04x bg=%04x style=%08x clsExt=%d winExt=%d class=%p\n",
                   atom, (DWORD)wc->lpfnWndProc, hInstance,
                   wc->hbrBackground, wc->style, wc->cbClsExtra,
                   wc->cbWndExtra, classPtr );
    
    classPtr->hIcon         = (HICON16)wc->hIcon;
    classPtr->hIconSm       = 0;
    classPtr->hCursor       = (HCURSOR16)wc->hCursor;
    classPtr->hbrBackground = (HBRUSH16)wc->hbrBackground;
    CLASS_SetMenuNameA( classPtr, wc->lpszMenuName );
    return atom;
}


/***********************************************************************
 *           RegisterClass32W      (USER32.429)
 */
ATOM RegisterClass32W( const WNDCLASS32W* wc )
{
    ATOM atom;
    CLASS *classPtr;

    /* FIXME: this should not be necessary for Win32 */
    HINSTANCE32 hInstance = (HINSTANCE32)GetExePtr( wc->hInstance );

    if (!(atom = GlobalAddAtom32W( wc->lpszClassName ))) return 0;
    if (!(classPtr = CLASS_RegisterClass( atom, hInstance, wc->style,
                                          wc->cbClsExtra, wc->cbWndExtra,
                                          (WNDPROC16)wc->lpfnWndProc,
                                          WIN_PROC_32W )))
    {
        GlobalDeleteAtom( atom );
        return 0;
    }

    dprintf_class( stddeb, "RegisterClass32W: atom=%04x wndproc=%08lx hinst=%04x bg=%04x style=%08x clsExt=%d winExt=%d class=%p\n",
                   atom, (DWORD)wc->lpfnWndProc, hInstance,
                   wc->hbrBackground, wc->style, wc->cbClsExtra,
                   wc->cbWndExtra, classPtr );
    
    classPtr->hIcon         = (HICON16)wc->hIcon;
    classPtr->hIconSm       = 0;
    classPtr->hCursor       = (HCURSOR16)wc->hCursor;
    classPtr->hbrBackground = (HBRUSH16)wc->hbrBackground;
    CLASS_SetMenuNameW( classPtr, wc->lpszMenuName );
    return atom;
}


/***********************************************************************
 *           RegisterClassEx16    (USER.397)
 */
ATOM RegisterClassEx16( const WNDCLASSEX16 *wc )
{
    ATOM atom;
    CLASS *classPtr;

    HINSTANCE32 hInstance = (HINSTANCE32)GetExePtr( wc->hInstance );

    if (!(atom = GlobalAddAtom16( wc->lpszClassName ))) return 0;
    if (!(classPtr = CLASS_RegisterClass( atom, hInstance, wc->style,
                                          wc->cbClsExtra, wc->cbWndExtra,
                                          wc->lpfnWndProc, WIN_PROC_16 )))
    {
        GlobalDeleteAtom( atom );
        return 0;
    }

    dprintf_class( stddeb, "RegisterClassEx16: atom=%04x wndproc=%08lx hinst=%04x bg=%04x style=%08x clsExt=%d winExt=%d class=%p\n",
                   atom, (DWORD)wc->lpfnWndProc, hInstance,
                   wc->hbrBackground, wc->style, wc->cbClsExtra,
                   wc->cbWndExtra, classPtr );
    
    classPtr->hIcon         = wc->hIcon;
    classPtr->hIconSm       = wc->hIconSm;
    classPtr->hCursor       = wc->hCursor;
    classPtr->hbrBackground = wc->hbrBackground;

    CLASS_SetMenuNameA( classPtr, HIWORD(wc->lpszMenuName) ?
                 PTR_SEG_TO_LIN(wc->lpszMenuName) : (LPCSTR)wc->lpszMenuName );
    return atom;
}


/***********************************************************************
 *           RegisterClassEx32A      (USER32.427)
 */
ATOM RegisterClassEx32A( const WNDCLASSEX32A* wc )
{
    ATOM atom;
    CLASS *classPtr;

    /* FIXME: this should not be necessary for Win32 */
    HINSTANCE32 hInstance = (HINSTANCE32)GetExePtr( wc->hInstance );

    if (!(atom = GlobalAddAtom32A( wc->lpszClassName ))) return 0;
    if (!(classPtr = CLASS_RegisterClass( atom, hInstance, wc->style,
                                          wc->cbClsExtra, wc->cbWndExtra,
                                          (WNDPROC16)wc->lpfnWndProc,
                                          WIN_PROC_32A )))
    {
        GlobalDeleteAtom( atom );
        return 0;
    }

    dprintf_class( stddeb, "RegisterClassEx32A: atom=%04x wndproc=%08lx hinst=%04x bg=%04x style=%08x clsExt=%d winExt=%d class=%p\n",
                   atom, (DWORD)wc->lpfnWndProc, hInstance,
                   wc->hbrBackground, wc->style, wc->cbClsExtra,
                   wc->cbWndExtra, classPtr );
    
    classPtr->hIcon         = (HICON16)wc->hIcon;
    classPtr->hIconSm       = (HICON16)wc->hIconSm;
    classPtr->hCursor       = (HCURSOR16)wc->hCursor;
    classPtr->hbrBackground = (HBRUSH16)wc->hbrBackground;
    CLASS_SetMenuNameA( classPtr, wc->lpszMenuName );
    return atom;
}


/***********************************************************************
 *           RegisterClassEx32W      (USER32.428)
 */
ATOM RegisterClassEx32W( const WNDCLASSEX32W* wc )
{
    ATOM atom;
    CLASS *classPtr;

    /* FIXME: this should not be necessary for Win32 */
    HINSTANCE32 hInstance = (HINSTANCE32)GetExePtr( wc->hInstance );

    if (!(atom = GlobalAddAtom32W( wc->lpszClassName ))) return 0;
    if (!(classPtr = CLASS_RegisterClass( atom, hInstance, wc->style,
                                          wc->cbClsExtra, wc->cbWndExtra,
                                          (WNDPROC16)wc->lpfnWndProc,
                                          WIN_PROC_32W )))
    {
        GlobalDeleteAtom( atom );
        return 0;
    }

    dprintf_class( stddeb, "RegisterClassEx32W: atom=%04x wndproc=%08lx hinst=%04x bg=%04x style=%08x clsExt=%d winExt=%d class=%p\n",
                   atom, (DWORD)wc->lpfnWndProc, hInstance,
                   wc->hbrBackground, wc->style, wc->cbClsExtra,
                   wc->cbWndExtra, classPtr );
    
    classPtr->hIcon         = (HICON16)wc->hIcon;
    classPtr->hIconSm       = (HICON16)wc->hIconSm;
    classPtr->hCursor       = (HCURSOR16)wc->hCursor;
    classPtr->hbrBackground = (HBRUSH16)wc->hbrBackground;
    CLASS_SetMenuNameW( classPtr, wc->lpszMenuName );
    return atom;
}


/***********************************************************************
 *           UnregisterClass16    (USER.403)
 */
BOOL16 UnregisterClass16( SEGPTR className, HINSTANCE16 hInstance )
{
    CLASS *classPtr;
    ATOM atom;

    hInstance = GetExePtr( hInstance );
    if (!(atom = GlobalFindAtom16( className ))) return FALSE;
    if (!(classPtr = CLASS_FindClassByAtom( atom, hInstance )) ||
        (classPtr->hInstance != hInstance)) return FALSE;
    return CLASS_FreeClass( classPtr );
}


/***********************************************************************
 *           UnregisterClass32A    (USER32.562)
 */
BOOL32 UnregisterClass32A( LPCSTR className, HINSTANCE32 hInstance )
{
    CLASS *classPtr;
    ATOM atom;

    hInstance = GetExePtr( hInstance );  /* FIXME: not needed in Win32 */
    if (!(atom = GlobalFindAtom32A( className ))) return FALSE;
    if (!(classPtr = CLASS_FindClassByAtom( atom, hInstance )) ||
        (classPtr->hInstance != hInstance)) return FALSE;
    return CLASS_FreeClass( classPtr );
}


/***********************************************************************
 *           UnregisterClass32W    (USER32.563)
 */
BOOL32 UnregisterClass32W( LPCWSTR className, HINSTANCE32 hInstance )
{
    CLASS *classPtr;
    ATOM atom;

    hInstance = GetExePtr( hInstance );  /* FIXME: not needed in Win32 */
    if (!(atom = GlobalFindAtom32W( className ))) return FALSE;
    if (!(classPtr = CLASS_FindClassByAtom( atom, hInstance )) ||
        (classPtr->hInstance != hInstance)) return FALSE;
    return CLASS_FreeClass( classPtr );
}


/***********************************************************************
 *           GetClassWord    (USER.129) (USER32.218)
 */
WORD GetClassWord( HWND32 hwnd, INT32 offset )
{
    WND * wndPtr;
    
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
    if (offset >= 0)
    {
        if (offset <= wndPtr->class->cbClsExtra - sizeof(WORD))
            return GET_WORD(((char *)wndPtr->class->wExtra) + offset);
    }
    else switch(offset)
    {
        case GCW_HBRBACKGROUND: return wndPtr->class->hbrBackground;
        case GCW_HCURSOR:       return wndPtr->class->hCursor;
        case GCW_HICON:         return wndPtr->class->hIcon;
        case GCW_HICONSM:       return wndPtr->class->hIconSm;
        case GCW_ATOM:          return wndPtr->class->atomName;
        case GCW_STYLE:
        case GCW_CBWNDEXTRA:
        case GCW_CBCLSEXTRA:
        case GCW_HMODULE:
            return (WORD)GetClassLong32A( hwnd, offset );
    }
    fprintf(stderr, "Warning: invalid offset %d for GetClassWord()\n", offset);
    return 0;
}


/***********************************************************************
 *           GetClassLong16    (USER.131)
 */
LONG GetClassLong16( HWND16 hwnd, INT16 offset )
{
    WND *wndPtr;
    LONG ret;

    switch( offset )
    {
    case GCL_WNDPROC:
        if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
        return (LONG)WINPROC_GetProc( wndPtr->class->winproc, WIN_PROC_16 );
    case GCL_MENUNAME:
        ret = GetClassLong32A( hwnd, offset );
        return (LONG)SEGPTR_GET( (void *)ret );
    default:
        return GetClassLong32A( hwnd, offset );
    }
}


/***********************************************************************
 *           GetClassLong32A    (USER32.214)
 */
LONG GetClassLong32A( HWND32 hwnd, INT32 offset )
{
    WND * wndPtr;
    
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
    if (offset >= 0)
    {
        if (offset <= wndPtr->class->cbClsExtra - sizeof(LONG))
            return GET_DWORD(((char *)wndPtr->class->wExtra) + offset);
    }
    switch(offset)
    {
        case GCL_STYLE:      return (LONG)wndPtr->class->style;
        case GCL_CBWNDEXTRA: return (LONG)wndPtr->class->cbWndExtra;
        case GCL_CBCLSEXTRA: return (LONG)wndPtr->class->cbClsExtra;
        case GCL_HMODULE:    return (LONG)wndPtr->class->hInstance;
        case GCL_WNDPROC:
            return (LONG)WINPROC_GetProc(wndPtr->class->winproc, WIN_PROC_32A);
        case GCL_MENUNAME:
            return (LONG)CLASS_GetMenuNameA( wndPtr->class );
        case GCL_HBRBACKGROUND:
        case GCL_HCURSOR:
        case GCL_HICON:
        case GCL_HICONSM:
            return GetClassWord( hwnd, offset );
    }
    fprintf(stderr, "Warning: invalid offset %d for GetClassLong()\n", offset);
    return 0;
}


/***********************************************************************
 *           GetClassLong32W    (USER32.215)
 */
LONG GetClassLong32W( HWND32 hwnd, INT32 offset )
{
    WND * wndPtr;

    switch(offset)
    {
    case GCL_WNDPROC:
        if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
        return (LONG)WINPROC_GetProc( wndPtr->class->winproc, WIN_PROC_32W );
    case GCL_MENUNAME:
        if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
        return (LONG)CLASS_GetMenuNameW( wndPtr->class );
    default:
        return GetClassLong32A( hwnd, offset );
    }
}


/***********************************************************************
 *           SetClassWord    (USER.130) (USER32.468)
 */
WORD SetClassWord( HWND32 hwnd, INT32 offset, WORD newval )
{
    WND * wndPtr;
    WORD retval = 0;
    void *ptr;
    
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
    if (offset >= 0)
    {
        if (offset + sizeof(WORD) <= wndPtr->class->cbClsExtra)
            ptr = ((char *)wndPtr->class->wExtra) + offset;
        else
        {
            fprintf( stderr, "Warning: invalid offset %d for SetClassWord()\n",
                     offset );
            return 0;
        }
    }
    else switch(offset)
    {
        case GCW_STYLE:
        case GCW_CBWNDEXTRA:
        case GCW_CBCLSEXTRA:
        case GCW_HMODULE:
            return (WORD)SetClassLong32A( hwnd, offset, (LONG)newval );
        case GCW_HBRBACKGROUND: ptr = &wndPtr->class->hbrBackground; break;
        case GCW_HCURSOR:       ptr = &wndPtr->class->hCursor; break;
        case GCW_HICON:         ptr = &wndPtr->class->hIcon; break;
        case GCW_HICONSM:       ptr = &wndPtr->class->hIconSm; break;
        case GCW_ATOM:          ptr = &wndPtr->class->atomName; break;
        default:
            fprintf( stderr, "Warning: invalid offset %d for SetClassWord()\n",
                     offset);
            return 0;
    }
    retval = GET_WORD(ptr);
    PUT_WORD( ptr, newval );
    return retval;
}


/***********************************************************************
 *           SetClassLong16    (USER.132)
 */
LONG SetClassLong16( HWND16 hwnd, INT16 offset, LONG newval )
{
    WND *wndPtr;
    LONG retval;

    switch(offset)
    {
    case GCL_WNDPROC:
        if (!(wndPtr = WIN_FindWndPtr(hwnd))) return 0;
        retval = (LONG)WINPROC_GetProc( wndPtr->class->winproc, WIN_PROC_16 );
        WINPROC_SetProc( &wndPtr->class->winproc, (WNDPROC16)newval,
                         WIN_PROC_16 );
        return retval;
    case GCL_MENUNAME:
        return SetClassLong32A( hwnd, offset, (LONG)PTR_SEG_TO_LIN(newval) );
    default:
        return SetClassLong32A( hwnd, offset, newval );
    }
}


/***********************************************************************
 *           SetClassLong32A    (USER32.466)
 */
LONG SetClassLong32A( HWND32 hwnd, INT32 offset, LONG newval )
{
    WND * wndPtr;
    LONG retval = 0;
    void *ptr;
    
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
    if (offset >= 0)
    {
        if (offset + sizeof(LONG) <= wndPtr->class->cbClsExtra)
            ptr = ((char *)wndPtr->class->wExtra) + offset;
        else
        {
            fprintf( stderr, "Warning: invalid offset %d for SetClassLong()\n",
                     offset );
            return 0;
        }
    }
    else switch(offset)
    {
        case GCL_MENUNAME:
            CLASS_SetMenuNameA( wndPtr->class, (LPCSTR)newval );
            return 0;  /* Old value is now meaningless anyway */
        case GCL_WNDPROC:
            retval = (LONG)WINPROC_GetProc( wndPtr->class->winproc,
                                            WIN_PROC_32A );
            WINPROC_SetProc( &wndPtr->class->winproc, (WNDPROC16)newval,
                             WIN_PROC_32A );
            return retval;
        case GCL_HBRBACKGROUND:
        case GCL_HCURSOR:
        case GCL_HICON:
        case GCL_HICONSM:
            return SetClassWord( hwnd, offset, (WORD)newval );
        case GCL_STYLE:      ptr = &wndPtr->class->style; break;
        case GCL_CBWNDEXTRA: ptr = &wndPtr->class->cbWndExtra; break;
        case GCL_CBCLSEXTRA: ptr = &wndPtr->class->cbClsExtra; break;
        case GCL_HMODULE:    ptr = &wndPtr->class->hInstance; break;
        default:
            fprintf( stderr, "Warning: invalid offset %d for SetClassLong()\n",
                     offset);
            return 0;
    }
    retval = GET_DWORD(ptr);
    PUT_DWORD( ptr, newval );
    return retval;
}


/***********************************************************************
 *           SetClassLong32W    (USER32.467)
 */
LONG SetClassLong32W( HWND32 hwnd, INT32 offset, LONG newval )
{
    WND *wndPtr;
    LONG retval;

    switch(offset)
    {
    case GCL_WNDPROC:
        if (!(wndPtr = WIN_FindWndPtr(hwnd))) return 0;
        retval = (LONG)WINPROC_GetProc( wndPtr->class->winproc, WIN_PROC_32W );
        WINPROC_SetProc( &wndPtr->class->winproc, (WNDPROC16)newval,
                         WIN_PROC_32W );
        return retval;
    case GCL_MENUNAME:
        if (!(wndPtr = WIN_FindWndPtr(hwnd))) return 0;
        CLASS_SetMenuNameW( wndPtr->class, (LPCWSTR)newval );
        return 0;  /* Old value is now meaningless anyway */
    default:
        return SetClassLong32A( hwnd, offset, newval );
    }
}


/***********************************************************************
 *           GetClassName16      (USER.58)
 */
INT16 GetClassName16( HWND16 hwnd, LPSTR buffer, INT16 count )
{
    WND *wndPtr;
    if (!(wndPtr = WIN_FindWndPtr(hwnd))) return 0;
    return GlobalGetAtomName16( wndPtr->class->atomName, buffer, count );
}


/***********************************************************************
 *           GetClassName32A      (USER32.216)
 */
INT32 GetClassName32A( HWND32 hwnd, LPSTR buffer, INT32 count )
{
    WND *wndPtr;
    if (!(wndPtr = WIN_FindWndPtr(hwnd))) return 0;
    return GlobalGetAtomName32A( wndPtr->class->atomName, buffer, count );
}


/***********************************************************************
 *           GetClassName32W      (USER32.217)
 */
INT32 GetClassName32W( HWND32 hwnd, LPWSTR buffer, INT32 count )
{
    WND *wndPtr;
    if (!(wndPtr = WIN_FindWndPtr(hwnd))) return 0;
    return GlobalGetAtomName32W( wndPtr->class->atomName, buffer, count );
}


/***********************************************************************
 *           GetClassInfo16      (USER.404)
 */
BOOL16 GetClassInfo16( HINSTANCE16 hInstance, SEGPTR name, WNDCLASS16 *wc )
{
    ATOM atom;
    CLASS *classPtr;

    hInstance = GetExePtr( hInstance );
    if (!(atom = GlobalFindAtom16( name )) ||
        !(classPtr = CLASS_FindClassByAtom( atom, hInstance )) ||
        (hInstance != classPtr->hInstance)) return FALSE;
    wc->style         = (UINT16)classPtr->style;
    wc->lpfnWndProc   = WINPROC_GetProc( classPtr->winproc, WIN_PROC_16 );
    wc->cbClsExtra    = (INT16)classPtr->cbClsExtra;
    wc->cbWndExtra    = (INT16)classPtr->cbWndExtra;
    wc->hInstance     = (HINSTANCE16)classPtr->hInstance;
    wc->hIcon         = classPtr->hIcon;
    wc->hCursor       = classPtr->hCursor;
    wc->hbrBackground = classPtr->hbrBackground;
    wc->lpszClassName = (SEGPTR)0;
    wc->lpszMenuName  = (SEGPTR)CLASS_GetMenuNameA( classPtr );
    if (HIWORD(wc->lpszMenuName))  /* Make it a SEGPTR */
        wc->lpszMenuName = SEGPTR_GET( (LPSTR)wc->lpszMenuName );
    return TRUE;
}


/***********************************************************************
 *           GetClassInfo32A      (USER32.210)
 */
BOOL32 GetClassInfo32A( HINSTANCE32 hInstance, LPCSTR name, WNDCLASS32A *wc )
{
    ATOM atom;
    CLASS *classPtr;

    hInstance = GetExePtr( hInstance );  /* FIXME: not needed in Win32 */
    if (!(atom = GlobalFindAtom32A( name )) ||
        !(classPtr = CLASS_FindClassByAtom( atom, hInstance )) ||
        (hInstance != classPtr->hInstance)) return FALSE;
    wc->style         = classPtr->style;
    wc->lpfnWndProc   = (WNDPROC32)WINPROC_GetProc( classPtr->winproc,
                                                    WIN_PROC_32A );
    wc->cbClsExtra    = classPtr->cbClsExtra;
    wc->cbWndExtra    = classPtr->cbWndExtra;
    wc->hInstance     = classPtr->hInstance;
    wc->hIcon         = (HICON32)classPtr->hIcon;
    wc->hCursor       = (HCURSOR32)classPtr->hCursor;
    wc->hbrBackground = (HBRUSH32)classPtr->hbrBackground;
    wc->lpszMenuName  = CLASS_GetMenuNameA( classPtr );
    wc->lpszClassName = NULL;
    return TRUE;
}


/***********************************************************************
 *           GetClassInfo32W      (USER32.213)
 */
BOOL32 GetClassInfo32W( HINSTANCE32 hInstance, LPCWSTR name, WNDCLASS32W *wc )
{
    ATOM atom;
    CLASS *classPtr;

    hInstance = GetExePtr( hInstance );  /* FIXME: not needed in Win32 */
    if (!(atom = GlobalFindAtom32W( name )) ||
        !(classPtr = CLASS_FindClassByAtom( atom, hInstance )) ||
        (hInstance != classPtr->hInstance)) return FALSE;
    wc->style         = classPtr->style;
    wc->lpfnWndProc   = (WNDPROC32)WINPROC_GetProc( classPtr->winproc,
                                                    WIN_PROC_32W );
    wc->cbClsExtra    = classPtr->cbClsExtra;
    wc->cbWndExtra    = classPtr->cbWndExtra;
    wc->hInstance     = classPtr->hInstance;
    wc->hIcon         = (HICON32)classPtr->hIcon;
    wc->hCursor       = (HCURSOR32)classPtr->hCursor;
    wc->hbrBackground = (HBRUSH32)classPtr->hbrBackground;
    wc->lpszMenuName  = CLASS_GetMenuNameW( classPtr );
    wc->lpszClassName = NULL;
    return TRUE;
}


/***********************************************************************
 *           GetClassInfoEx16      (USER.398)
 *
 * FIXME: this is just a guess, I have no idea if GetClassInfoEx() is the
 * same in Win16 as in Win32. --AJ
 */
BOOL16 GetClassInfoEx16( HINSTANCE16 hInstance, SEGPTR name, WNDCLASSEX16 *wc )
{
    ATOM atom;
    CLASS *classPtr;

    hInstance = GetExePtr( hInstance );
    if (!(atom = GlobalFindAtom16( name )) ||
        !(classPtr = CLASS_FindClassByAtom( atom, hInstance )) ||
        (hInstance != classPtr->hInstance)) return FALSE;
    wc->style         = classPtr->style;
    wc->lpfnWndProc   = WINPROC_GetProc( classPtr->winproc, WIN_PROC_16 );
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
    return TRUE;
}


/***********************************************************************
 *           GetClassInfoEx32A      (USER32.211)
 */
BOOL32 GetClassInfoEx32A( HINSTANCE32 hInstance, LPCSTR name,
                          WNDCLASSEX32A *wc )
{
    ATOM atom;
    CLASS *classPtr;

    hInstance = GetExePtr( hInstance );  /* FIXME: not needed in Win32 */
    if (!(atom = GlobalFindAtom32A( name )) ||
        !(classPtr = CLASS_FindClassByAtom( atom, hInstance )) ||
        (hInstance != classPtr->hInstance)) return FALSE;
    wc->style         = classPtr->style;
    wc->lpfnWndProc   = (WNDPROC32)WINPROC_GetProc( classPtr->winproc,
                                                    WIN_PROC_32A );
    wc->cbClsExtra    = classPtr->cbClsExtra;
    wc->cbWndExtra    = classPtr->cbWndExtra;
    wc->hInstance     = classPtr->hInstance;
    wc->hIcon         = (HICON32)classPtr->hIcon;
    wc->hIconSm       = (HICON32)classPtr->hIconSm;
    wc->hCursor       = (HCURSOR32)classPtr->hCursor;
    wc->hbrBackground = (HBRUSH32)classPtr->hbrBackground;
    wc->lpszMenuName  = CLASS_GetMenuNameA( classPtr );
    wc->lpszClassName = NULL;
    return TRUE;
}


/***********************************************************************
 *           GetClassInfoEx32W      (USER32.212)
 */
BOOL32 GetClassInfoEx32W( HINSTANCE32 hInstance, LPCWSTR name,
                          WNDCLASSEX32W *wc )
{
    ATOM atom;
    CLASS *classPtr;

    hInstance = GetExePtr( hInstance );  /* FIXME: not needed in Win32 */
    if (!(atom = GlobalFindAtom32W( name )) ||
        !(classPtr = CLASS_FindClassByAtom( atom, hInstance )) ||
        (hInstance != classPtr->hInstance)) return FALSE;
    wc->style         = classPtr->style;
    wc->lpfnWndProc   = (WNDPROC32)WINPROC_GetProc( classPtr->winproc,
                                                    WIN_PROC_32W );
    wc->cbClsExtra    = classPtr->cbClsExtra;
    wc->cbWndExtra    = classPtr->cbWndExtra;
    wc->hInstance     = classPtr->hInstance;
    wc->hIcon         = (HICON32)classPtr->hIcon;
    wc->hIconSm       = (HICON32)classPtr->hIconSm;
    wc->hCursor       = (HCURSOR32)classPtr->hCursor;
    wc->hbrBackground = (HBRUSH32)classPtr->hbrBackground;
    wc->lpszMenuName  = CLASS_GetMenuNameW( classPtr );
    wc->lpszClassName = NULL;
    return TRUE;
}


/***********************************************************************
 *           ClassFirst      (TOOLHELP.69)
 */
BOOL16 ClassFirst( CLASSENTRY *pClassEntry )
{
    pClassEntry->wNext = 1;
    return ClassNext( pClassEntry );
}


/***********************************************************************
 *           ClassNext      (TOOLHELP.70)
 */
BOOL16 ClassNext( CLASSENTRY *pClassEntry )
{
    int i;
    CLASS *class = firstClass;

    if (!pClassEntry->wNext) return FALSE;
    for (i = 1; (i < pClassEntry->wNext) && class; i++) class = class->next;
    if (!class)
    {
        pClassEntry->wNext = 0;
        return FALSE;
    }
    pClassEntry->hInst = class->hInstance;
    pClassEntry->wNext++;
    GlobalGetAtomName32A( class->atomName, pClassEntry->szClassName,
                          sizeof(pClassEntry->szClassName) );
    return TRUE;
}
