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
 *   classes are getting registred with wrong hInstance.
 */

#include <stdlib.h>
#include <string.h>
#include "class.h"
#include "heap.h"
#include "win.h"
#include "dce.h"
#include "atom.h"
#include "ldt.h"
#include "toolhelp.h"
#include "winproc.h"
#include "debug.h"
#include "winerror.h"


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
        DUMP("%p is not a class\n", ptr );
        return;
    }

    GlobalGetAtomName32A( ptr->atomName, className, sizeof(className) );

    DUMP( "Class %p:\n", ptr );
    DUMP( "next=%p  name=%04x '%s'  style=%08x  wndProc=%08x\n"
             "inst=%04x  dce=%08x  icon=%04x  cursor=%04x  bkgnd=%04x\n"
             "clsExtra=%d  winExtra=%d  #windows=%d\n",
             ptr->next, ptr->atomName, className, ptr->style,
             (UINT32)ptr->winproc, ptr->hInstance, (UINT32)ptr->dce,
             ptr->hIcon, ptr->hCursor, ptr->hbrBackground,
             ptr->cbClsExtra, ptr->cbWndExtra, ptr->cWindows );
    if (ptr->cbClsExtra)
    {
        DUMP( "extra bytes:" );
        for (i = 0; i < ptr->cbClsExtra; i++)
            DUMP( " %02x", *((BYTE *)ptr->wExtra+i) );
        DUMP( "\n" );
    }
    DUMP( "\n" );
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

    DUMP( " Class   Name                  Style   WndProc\n" );
    for (ptr = firstClass; ptr; ptr = ptr->next)
    {
        GlobalGetAtomName32A( ptr->atomName, className, sizeof(className) );
        DUMP( "%08x %-20.20s %08x %08x\n", (UINT32)ptr, className,
                 ptr->style, (UINT32)ptr->winproc );
    }
    DUMP( "\n" );
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
static BOOL32 CLASS_FreeClass( CLASS *classPtr )
{
    CLASS **ppClass;
    TRACE(class,"%p \n", classPtr);  

    /* Check if we can remove this class */

    if (classPtr->cWindows > 0) return FALSE;

    /* Remove the class from the linked list */

    for (ppClass = &firstClass; *ppClass; ppClass = &(*ppClass)->next)
        if (*ppClass == classPtr) break;
    if (!*ppClass)
    {
        ERR( class, "Class list corrupted\n" );
        return FALSE;
    }
    *ppClass = classPtr->next;

    /* Delete the class */

    if (classPtr->dce) DCE_FreeDCE( classPtr->dce );
    if (classPtr->hbrBackground) DeleteObject32( classPtr->hbrBackground );
    GlobalDeleteAtom( classPtr->atomName );
    CLASS_SetMenuNameA( classPtr, NULL );
    WINPROC_FreeProc( classPtr->winproc, WIN_PROC_CLASS );
    HeapFree( SystemHeap, 0, classPtr );
    return TRUE;
}


/***********************************************************************
 *           CLASS_FreeModuleClasses
 */
void CLASS_FreeModuleClasses( HMODULE16 hModule )
{
    CLASS *ptr, *next;
  
    TRACE(class,"0x%08x \n", hModule);  

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
CLASS *CLASS_FindClassByAtom( ATOM atom, HINSTANCE32 hinstance )
{   CLASS * class, *tclass=0;

    TRACE(class,"0x%08x 0x%08x\n", atom, hinstance);

    /* First search task-specific classes */

    for (class = firstClass; (class); class = class->next)
    {
        if (class->style & CS_GLOBALCLASS) continue;
        if (class->atomName == atom)
        {
            if (hinstance==class->hInstance || hinstance==0xffff )
            {
                TRACE(class,"-- found local %p\n", class);
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
            TRACE(class,"-- found global %p\n", class);
            return class;
        }
    }

    /* Then check if there was a local class with hInst=0*/
    if ( tclass )
    {
        WARN(class,"-- found local Class registred with hInst=0\n");
        return tclass;
    }
    
    TRACE(class,"-- not found\n");
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

    TRACE(class,"atom=0x%x hinst=0x%x style=0x%lx clExtr=0x%x winExtr=0x%x wndProc=0x%p ProcType=0x%x\n",
     atom, hInstance, style, classExtra, winExtra, wndProc, wndProcType);

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
        WARN(class, "Class extra bytes %d is > 40\n", classExtra);
    if (winExtra < 0) winExtra = 0;
    else if (winExtra > 40)    /* Extra bytes are limited to 40 in Win32 */
        WARN(class, "Win extra bytes %d is > 40\n", winExtra );

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

    WINPROC_SetProc( &classPtr->winproc, wndProc, wndProcType, WIN_PROC_CLASS);

    /* Other values must be set by caller */

    if (classExtra) memset( classPtr->wExtra, 0, classExtra );
    firstClass = classPtr;
    return classPtr;
}


/***********************************************************************
 *           RegisterClass16    (USER.57)
 */
ATOM WINAPI RegisterClass16( const WNDCLASS16 *wc )
{
    ATOM atom;
    CLASS *classPtr;
    HINSTANCE16 hInstance=GetExePtr(wc->hInstance);

    if (!(atom = GlobalAddAtom16( wc->lpszClassName ))) return 0;
    if (!(classPtr = CLASS_RegisterClass( atom, hInstance, wc->style,
                                          wc->cbClsExtra, wc->cbWndExtra,
                                          wc->lpfnWndProc, WIN_PROC_16 )))
    {
        GlobalDeleteAtom( atom );
        return 0;
    }

    TRACE(class, "atom=%04x wndproc=%08lx hinst=%04x
bg=%04x style=%08x clsExt=%d winExt=%d class=%p name='%s'\n",
                   atom, (DWORD)wc->lpfnWndProc, hInstance,
                   wc->hbrBackground, wc->style, wc->cbClsExtra,
                   wc->cbWndExtra, classPtr,
		   HIWORD(wc->lpszClassName) ?
                       (char *)PTR_SEG_TO_LIN(wc->lpszClassName) : "" );

    classPtr->hIcon         = wc->hIcon;
    classPtr->hIconSm       = 0;
    classPtr->hCursor       = wc->hCursor;
    classPtr->hbrBackground = wc->hbrBackground;

    CLASS_SetMenuNameA( classPtr, HIWORD(wc->lpszMenuName) ?
                 PTR_SEG_TO_LIN(wc->lpszMenuName) : (LPCSTR)wc->lpszMenuName );
    return atom;
}


/***********************************************************************
 *           RegisterClass32A      (USER32.427)
 * RETURNS
 *	>0: Unique identifier
 *	0: Failure
 */
ATOM WINAPI RegisterClass32A(
	    const WNDCLASS32A* wc /* Address of structure with class data */
) {
    ATOM atom;
    CLASS *classPtr;

    if (!(atom = GlobalAddAtom32A( wc->lpszClassName ))) 
    {
        SetLastError(ERROR_CLASS_ALREADY_EXISTS);
        return FALSE;
    }
    if (!(classPtr = CLASS_RegisterClass( atom, wc->hInstance, wc->style,
                                          wc->cbClsExtra, wc->cbWndExtra,
                                          (WNDPROC16)wc->lpfnWndProc,
                                          WIN_PROC_32A )))
    {   GlobalDeleteAtom( atom );
        SetLastError(ERROR_CLASS_ALREADY_EXISTS);
        return FALSE;
    }

    TRACE(class, "atom=%04x wndproc=%08lx hinst=%04x bg=%04x style=%08x clsExt=%d winExt=%d class=%p name='%s'\n",
                   atom, (DWORD)wc->lpfnWndProc, wc->hInstance,
                   wc->hbrBackground, wc->style, wc->cbClsExtra,
                   wc->cbWndExtra, classPtr,
                   HIWORD(wc->lpszClassName) ? wc->lpszClassName : "" );
    
    classPtr->hIcon         = (HICON16)wc->hIcon;
    classPtr->hIconSm       = 0;
    classPtr->hCursor       = (HCURSOR16)wc->hCursor;
    classPtr->hbrBackground = (HBRUSH16)wc->hbrBackground;
    CLASS_SetMenuNameA( classPtr, wc->lpszMenuName );
    return atom;
}


/***********************************************************************
 *           RegisterClass32W      (USER32.430)
 */
ATOM WINAPI RegisterClass32W( const WNDCLASS32W* wc )
{
    ATOM atom;
    CLASS *classPtr;

    if (!(atom = GlobalAddAtom32W( wc->lpszClassName )))
    {
        SetLastError(ERROR_CLASS_ALREADY_EXISTS);
        return FALSE;
    }
    if (!(classPtr = CLASS_RegisterClass( atom, wc->hInstance, wc->style,
                                          wc->cbClsExtra, wc->cbWndExtra,
                                          (WNDPROC16)wc->lpfnWndProc,
                                          WIN_PROC_32W )))
    {
        SetLastError(ERROR_CLASS_ALREADY_EXISTS);
        GlobalDeleteAtom( atom );
        return 0;
    }

    TRACE(class, "atom=%04x wndproc=%08lx hinst=%04x bg=%04x style=%08x clsExt=%d winExt=%d class=%p\n",
                   atom, (DWORD)wc->lpfnWndProc, wc->hInstance,
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
ATOM WINAPI RegisterClassEx16( const WNDCLASSEX16 *wc )
{
    ATOM atom;
    CLASS *classPtr;
    HINSTANCE16 hInstance = GetExePtr( wc->hInstance );

    if (!(atom = GlobalAddAtom16( wc->lpszClassName ))) return 0;
    if (!(classPtr = CLASS_RegisterClass( atom, hInstance, wc->style,
                                          wc->cbClsExtra, wc->cbWndExtra,
                                          wc->lpfnWndProc, WIN_PROC_16 )))
    {
        GlobalDeleteAtom( atom );
        return 0;
    }

    TRACE(class, "atom=%04x wndproc=%08lx hinst=%04x bg=%04x style=%08x clsExt=%d winExt=%d class=%p\n",
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
 *           RegisterClassEx32A      (USER32.428)
 */
ATOM WINAPI RegisterClassEx32A( const WNDCLASSEX32A* wc )
{
    ATOM atom;
    CLASS *classPtr;

    if (!(atom = GlobalAddAtom32A( wc->lpszClassName )))
    {
        SetLastError(ERROR_CLASS_ALREADY_EXISTS);
        return FALSE;
    }
    if (!(classPtr = CLASS_RegisterClass( atom, wc->hInstance, wc->style,
                                          wc->cbClsExtra, wc->cbWndExtra,
                                          (WNDPROC16)wc->lpfnWndProc,
                                          WIN_PROC_32A )))
    {
        SetLastError(ERROR_CLASS_ALREADY_EXISTS);
        GlobalDeleteAtom( atom );
        return FALSE;
    }

    TRACE(class, "atom=%04x wndproc=%08lx hinst=%04x bg=%04x style=%08x clsExt=%d winExt=%d class=%p\n",
                   atom, (DWORD)wc->lpfnWndProc, wc->hInstance,
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
 *           RegisterClassEx32W      (USER32.429)
 */
ATOM WINAPI RegisterClassEx32W( const WNDCLASSEX32W* wc )
{
    ATOM atom;
    CLASS *classPtr;

    if (!(atom = GlobalAddAtom32W( wc->lpszClassName )))
    {
        SetLastError(ERROR_CLASS_ALREADY_EXISTS);
        return 0;
    }
    if (!(classPtr = CLASS_RegisterClass( atom, wc->hInstance, wc->style,
                                          wc->cbClsExtra, wc->cbWndExtra,
                                          (WNDPROC16)wc->lpfnWndProc,
                                          WIN_PROC_32W )))
    {
        SetLastError(ERROR_CLASS_ALREADY_EXISTS);
        GlobalDeleteAtom( atom );
        return 0;
    }

    TRACE(class, "atom=%04x wndproc=%08lx hinst=%04x bg=%04x style=%08x clsExt=%d winExt=%d class=%p\n",
                   atom, (DWORD)wc->lpfnWndProc, wc->hInstance,
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
BOOL16 WINAPI UnregisterClass16( SEGPTR className, HINSTANCE16 hInstance )
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
 *           UnregisterClass32A    (USER32.563)
 *
 */
BOOL32 WINAPI UnregisterClass32A( LPCSTR className, HINSTANCE32 hInstance )
{   CLASS *classPtr;
    ATOM atom;
    BOOL32 ret;

    TRACE(class,"%s %x\n",className, hInstance);

    if (!(atom = GlobalFindAtom32A( className )))
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
    if (!(ret = CLASS_FreeClass( classPtr )))
        SetLastError(ERROR_CLASS_HAS_WINDOWS);
    return ret;
}

/***********************************************************************
 *           UnregisterClass32W    (USER32.564)
 */
BOOL32 WINAPI UnregisterClass32W( LPCWSTR className, HINSTANCE32 hInstance )
{   CLASS *classPtr;
    ATOM atom;
    BOOL32 ret;

    TRACE(class,"%s %x\n",debugstr_w(className), hInstance);

    if (!(atom = GlobalFindAtom32W( className )))
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
    if (!(ret = CLASS_FreeClass( classPtr )))
        SetLastError(ERROR_CLASS_HAS_WINDOWS);
    return ret;
}

/***********************************************************************
 *           GetClassWord16    (USER.129)
 */
WORD WINAPI GetClassWord16( HWND16 hwnd, INT16 offset )
{
    return GetClassWord32( hwnd, offset );
}


/***********************************************************************
 *           GetClassWord32    (USER32.219)
 */
WORD WINAPI GetClassWord32( HWND32 hwnd, INT32 offset )
{
    WND * wndPtr;
    
    TRACE(class,"%x %x\n",hwnd, offset);
    
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

    WARN(class, "Invalid offset %d\n", offset);
    return 0;
}


/***********************************************************************
 *           GetClassLong16    (USER.131)
 */
LONG WINAPI GetClassLong16( HWND16 hwnd, INT16 offset )
{
    WND *wndPtr;
    LONG ret;

    TRACE(class,"%x %x\n",hwnd, offset);

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
 *           GetClassLong32A    (USER32.215)
 */
LONG WINAPI GetClassLong32A( HWND32 hwnd, INT32 offset )
{
    WND * wndPtr;
    
    TRACE(class,"%x %x\n",hwnd, offset);
    
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
            return GetClassWord32( hwnd, offset );
    }
    WARN(class, "Invalid offset %d\n", offset);
    return 0;
}


/***********************************************************************
 *           GetClassLong32W    (USER32.216)
 */
LONG WINAPI GetClassLong32W( HWND32 hwnd, INT32 offset )
{
    WND * wndPtr;

    TRACE(class,"%x %x\n",hwnd, offset);

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
 *           SetClassWord16    (USER.130)
 */
WORD WINAPI SetClassWord16( HWND16 hwnd, INT16 offset, WORD newval )
{
    return SetClassWord32( hwnd, offset, newval );
}


/***********************************************************************
 *           SetClassWord32    (USER32.469)
 */
WORD WINAPI SetClassWord32( HWND32 hwnd, INT32 offset, WORD newval )
{
    WND * wndPtr;
    WORD retval = 0;
    void *ptr;
    
    TRACE(class,"%x %x %x\n",hwnd, offset, newval);
    
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
    if (offset >= 0)
    {
        if (offset + sizeof(WORD) <= wndPtr->class->cbClsExtra)
            ptr = ((char *)wndPtr->class->wExtra) + offset;
        else
        {
            WARN( class, "Invalid offset %d\n", offset );
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
            WARN( class, "Invalid offset %d\n", offset);
            return 0;
    }
    retval = GET_WORD(ptr);
    PUT_WORD( ptr, newval );
    return retval;
}


/***********************************************************************
 *           SetClassLong16    (USER.132)
 */
LONG WINAPI SetClassLong16( HWND16 hwnd, INT16 offset, LONG newval )
{
    WND *wndPtr;
    LONG retval;

    TRACE(class,"%x %x %lx\n",hwnd, offset, newval);

    switch(offset)
    {
    case GCL_WNDPROC:
        if (!(wndPtr = WIN_FindWndPtr(hwnd))) return 0;
        retval = (LONG)WINPROC_GetProc( wndPtr->class->winproc, WIN_PROC_16 );
        WINPROC_SetProc( &wndPtr->class->winproc, (WNDPROC16)newval,
                         WIN_PROC_16, WIN_PROC_CLASS );
        return retval;
    case GCL_MENUNAME:
        return SetClassLong32A( hwnd, offset, (LONG)PTR_SEG_TO_LIN(newval) );
    default:
        return SetClassLong32A( hwnd, offset, newval );
    }
}


/***********************************************************************
 *           SetClassLong32A    (USER32.467)
 */
LONG WINAPI SetClassLong32A( HWND32 hwnd, INT32 offset, LONG newval )
{
    WND * wndPtr;
    LONG retval = 0;
    void *ptr;
    
    TRACE(class,"%x %x %lx\n",hwnd, offset, newval);
        
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
    if (offset >= 0)
    {
        if (offset + sizeof(LONG) <= wndPtr->class->cbClsExtra)
            ptr = ((char *)wndPtr->class->wExtra) + offset;
        else
        {
            WARN( class, "Invalid offset %d\n", offset );
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
                             WIN_PROC_32A, WIN_PROC_CLASS );
            return retval;
        case GCL_HBRBACKGROUND:
        case GCL_HCURSOR:
        case GCL_HICON:
        case GCL_HICONSM:
            return SetClassWord32( hwnd, offset, (WORD)newval );
        case GCL_STYLE:      ptr = &wndPtr->class->style; break;
        case GCL_CBWNDEXTRA: ptr = &wndPtr->class->cbWndExtra; break;
        case GCL_CBCLSEXTRA: ptr = &wndPtr->class->cbClsExtra; break;
        case GCL_HMODULE:    ptr = &wndPtr->class->hInstance; break;
        default:
            WARN( class, "Invalid offset %d\n", offset );
            return 0;
    }
    retval = GET_DWORD(ptr);
    PUT_DWORD( ptr, newval );
    return retval;
}


/***********************************************************************
 *           SetClassLong32W    (USER32.468)
 */
LONG WINAPI SetClassLong32W( HWND32 hwnd, INT32 offset, LONG newval )
{
    WND *wndPtr;
    LONG retval;

    TRACE(class,"%x %x %lx\n",hwnd, offset, newval);
    
    switch(offset)
    {
    case GCL_WNDPROC:
        if (!(wndPtr = WIN_FindWndPtr(hwnd))) return 0;
        retval = (LONG)WINPROC_GetProc( wndPtr->class->winproc, WIN_PROC_32W );
        WINPROC_SetProc( &wndPtr->class->winproc, (WNDPROC16)newval,
                         WIN_PROC_32W, WIN_PROC_CLASS );
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
INT16 WINAPI GetClassName16( HWND16 hwnd, LPSTR buffer, INT16 count )
{
    WND *wndPtr;
    if (!(wndPtr = WIN_FindWndPtr(hwnd))) return 0;
    return GlobalGetAtomName16( wndPtr->class->atomName, buffer, count );
}


/***********************************************************************
 *           GetClassName32A      (USER32.217)
 */
INT32 WINAPI GetClassName32A( HWND32 hwnd, LPSTR buffer, INT32 count )
{   INT32 ret;
    WND *wndPtr;
            
    if (!(wndPtr = WIN_FindWndPtr(hwnd))) return 0;
    ret = GlobalGetAtomName32A( wndPtr->class->atomName, buffer, count );

    TRACE(class,"%x %s %x\n",hwnd, buffer, count);
    return ret;
}


/***********************************************************************
 *           GetClassName32W      (USER32.218)
 */
INT32 WINAPI GetClassName32W( HWND32 hwnd, LPWSTR buffer, INT32 count )
{   INT32 ret;
    WND *wndPtr;

    if (!(wndPtr = WIN_FindWndPtr(hwnd))) return 0;
    ret = GlobalGetAtomName32W( wndPtr->class->atomName, buffer, count );

    TRACE(class,"%x %s %x\n",hwnd, debugstr_w(buffer), count);
    
    return ret;
}


/***********************************************************************
 *           GetClassInfo16      (USER.404)
 */
BOOL16 WINAPI GetClassInfo16( HINSTANCE16 hInstance, SEGPTR name,
                              WNDCLASS16 *wc )
{
    ATOM atom;
    CLASS *classPtr;

    TRACE(class,"%x %p %p\n",hInstance, PTR_SEG_TO_LIN (name), wc);
    
    hInstance = GetExePtr( hInstance );
    if (!(atom = GlobalFindAtom16( name )) ||
        !(classPtr = CLASS_FindClassByAtom( atom, hInstance )))
        return FALSE;
    if ((hInstance != classPtr->hInstance) &&
        !(classPtr->style & CS_GLOBALCLASS)) /*BWCC likes to pass hInstance=0*/
        return FALSE;
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
 *           GetClassInfo32A      (USER32.211)
 */
BOOL32 WINAPI GetClassInfo32A( HINSTANCE32 hInstance, LPCSTR name,
                               WNDCLASS32A *wc )
{
    ATOM atom;
    CLASS *classPtr;

    TRACE(class,"%x %p %p\n",hInstance, name, wc);

    /* workaround: if hInstance=NULL you expect to get the system classes
    but this classes (as example from comctl32.dll SysListView) won't be
    registred with hInstance=NULL in WINE because of the late loading
    of this dll. fixes file dialogs in WinWord95 (jsch)*/

    if (!(atom=GlobalFindAtom32A(name)) || !(classPtr=CLASS_FindClassByAtom(atom,hInstance)))
        return FALSE;

    if  (classPtr->hInstance && (hInstance != classPtr->hInstance))
    {
        if (hInstance) return FALSE;
        else    
            WARN(class,"systemclass %s (hInst=0) demanded but only class with hInst!=0 found\n",name);
    }

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
 *           GetClassInfo32W      (USER32.214)
 */
BOOL32 WINAPI GetClassInfo32W( HINSTANCE32 hInstance, LPCWSTR name,
                               WNDCLASS32W *wc )
{
    ATOM atom;
    CLASS *classPtr;

    TRACE(class,"%x %p %p\n",hInstance, name, wc);

    if (!(atom = GlobalFindAtom32W( name )) ||
        !(classPtr = CLASS_FindClassByAtom( atom, hInstance )) ||
	(classPtr->hInstance && (hInstance != classPtr->hInstance)))
        return FALSE;

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
BOOL16 WINAPI GetClassInfoEx16( HINSTANCE16 hInstance, SEGPTR name,
                                WNDCLASSEX16 *wc )
{
    ATOM atom;
    CLASS *classPtr;

    TRACE(class,"%x %p %p\n",hInstance,PTR_SEG_TO_LIN( name ), wc);
    
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
 *           GetClassInfoEx32A      (USER32.212)
 */
BOOL32 WINAPI GetClassInfoEx32A( HINSTANCE32 hInstance, LPCSTR name,
                                 WNDCLASSEX32A *wc )
{
    ATOM atom;
    CLASS *classPtr;

    TRACE(class,"%x %p %p\n",hInstance, name, wc);
    
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
 *           GetClassInfoEx32W      (USER32.213)
 */
BOOL32 WINAPI GetClassInfoEx32W( HINSTANCE32 hInstance, LPCWSTR name,
                                 WNDCLASSEX32W *wc )
{
    ATOM atom;
    CLASS *classPtr;

    TRACE(class,"%x %p %p\n",hInstance, name, wc);
    
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
BOOL16 WINAPI ClassFirst( CLASSENTRY *pClassEntry )
{
    TRACE(class,"%p\n",pClassEntry);
    pClassEntry->wNext = 1;
    return ClassNext( pClassEntry );
}


/***********************************************************************
 *           ClassNext      (TOOLHELP.70)
 */
BOOL16 WINAPI ClassNext( CLASSENTRY *pClassEntry )
{
    int i;
    CLASS *class = firstClass;

    TRACE(class,"%p\n",pClassEntry);
   
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
