/*
 * Window classes functions
 *
 * Copyright 1993 Alexandre Julliard
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "class.h"
#include "user.h"
#include "win.h"
#include "dce.h"
#include "atom.h"
#include "ldt.h"
#include "toolhelp.h"
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

    if (((CLASS *)USER_HEAP_LIN_ADDR(ptr->self) != ptr) ||
        (ptr->wMagic != CLASS_MAGIC))
    {
        fprintf( stderr, "%p is not a class\n", ptr );
        return;
    }

    GlobalGetAtomName( ptr->atomName, className, sizeof(className) );

    fprintf( stderr, "Class %p:\n", ptr );
    fprintf( stderr,
             "next=%p  name=%04x '%s'  style=%04x  wndProc=%08lx\n"
             "inst=%04x  hdce=%04x  icon=%04x  cursor=%04x  bkgnd=%04x\n"
             "clsExtra=%d  winExtra=%d  #windows=%d\n",
             ptr->next, ptr->atomName, className, ptr->wc.style,
             (DWORD)ptr->wc.lpfnWndProc, ptr->wc.hInstance, ptr->hdce,
             ptr->wc.hIcon, ptr->wc.hCursor, ptr->wc.hbrBackground,
             ptr->wc.cbClsExtra, ptr->wc.cbWndExtra, ptr->cWindows );
    if (ptr->wc.cbClsExtra)
    {
        fprintf( stderr, "extra bytes:" );
        for (i = 0; i < ptr->wc.cbClsExtra; i++)
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

    fprintf( stderr, " Class   Name                Style WndProc\n" );
    for (ptr = firstClass; ptr; ptr = ptr->next)
    {
        GlobalGetAtomName( ptr->atomName, className, sizeof(className) );
        fprintf( stderr, "%08lx %-20.20s %04x %08lx\n", (DWORD)ptr, className,
                 ptr->wc.style, (DWORD)ptr->wc.lpfnWndProc );
    }
    fprintf( stderr, "\n" );
}


/***********************************************************************
 *           CLASS_FreeClass
 *
 * Free a class structure.
 */
static void CLASS_FreeClass( CLASS *classPtr )
{
    CLASS **ppClass;

    /* Remove the class from the linked list */

    for (ppClass = &firstClass; *ppClass; ppClass = &(*ppClass)->next)
        if (*ppClass == classPtr) break;
    if (!*ppClass)
    {
        fprintf(stderr, "ERROR: Class list corrupted\n" );
        return;
    }
    *ppClass = classPtr->next;

    /* Delete the class */

    if (classPtr->hdce) DCE_FreeDCE( classPtr->hdce );
    if (classPtr->wc.hbrBackground) DeleteObject( classPtr->wc.hbrBackground );
    GlobalDeleteAtom( classPtr->atomName );
    if (HIWORD(classPtr->wc.lpszMenuName))
	USER_HEAP_FREE( (HANDLE)classPtr->wc.lpszMenuName );
    USER_HEAP_FREE( classPtr->self );
}


/***********************************************************************
 *           CLASS_FreeModuleClasses
 */
void CLASS_FreeModuleClasses( HMODULE hModule )
{
    CLASS *ptr, *next;
  
    for (ptr = firstClass; ptr; ptr = next)
    {
        next = ptr->next;
	if (ptr->wc.hInstance == hModule) CLASS_FreeClass( ptr );
    }
}


/***********************************************************************
 *           CLASS_FindClassByName
 *
 * Return a pointer to the class.
 */
CLASS *CLASS_FindClassByName( SEGPTR name, HINSTANCE hinstance )
{
    ATOM atom;
    CLASS * class;

    if (!(atom = GlobalFindAtom( name ))) return 0;

      /* First search task-specific classes */

    for (class = firstClass; (class); class = class->next)
    {
        if (class->wc.style & CS_GLOBALCLASS) continue;
        if ((class->atomName == atom) && 
            ((hinstance==(HINSTANCE)0xffff) ||
             (hinstance == class->wc.hInstance))) return class;
    }
    
      /* Then search global classes */

    for (class = firstClass; (class); class = class->next)
    {
        if (!(class->wc.style & CS_GLOBALCLASS)) continue;
        if (class->atomName == atom) return class;
    }

    return 0;
}


/***********************************************************************
 *           RegisterClass    (USER.57)
 */
ATOM RegisterClass( LPWNDCLASS class )
{
    CLASS * newClass, * prevClass;
    HCLASS handle;
    int classExtra;

    dprintf_class( stddeb, "RegisterClass: wndproc=%08lx hinst=%04x name='%s' background %04x\n",
                 (DWORD)class->lpfnWndProc, class->hInstance,
                 HIWORD(class->lpszClassName) ?
                  (char *)PTR_SEG_TO_LIN(class->lpszClassName) : "(int)",
                 class->hbrBackground );
    dprintf_class(stddeb,"               style=%04x clsExtra=%d winExtra=%d\n",
                  class->style, class->cbClsExtra, class->cbWndExtra );
    
      /* Window classes are owned by modules, not instances */
    class->hInstance = GetExePtr( class->hInstance );
    
      /* Check if a class with this name already exists */
    prevClass = CLASS_FindClassByName( class->lpszClassName, class->hInstance);
    if (prevClass)
    {
	  /* Class can be created only if it is local and */
	  /* if the class with the same name is global.   */

	if (class->style & CS_GLOBALCLASS) return 0;
	if (!(prevClass->wc.style & CS_GLOBALCLASS)) return 0;
    }

      /* Create class */

    classExtra = (class->cbClsExtra < 0) ? 0 : class->cbClsExtra;
    handle = USER_HEAP_ALLOC( sizeof(CLASS) + classExtra );
    if (!handle) return 0;
    newClass = (CLASS *) USER_HEAP_LIN_ADDR( handle );
    newClass->self          = handle;
    newClass->next          = firstClass;
    newClass->wMagic        = CLASS_MAGIC;
    newClass->cWindows      = 0;  
    newClass->wc            = *class;
    newClass->wc.cbWndExtra = (class->cbWndExtra < 0) ? 0 : class->cbWndExtra;
    newClass->wc.cbClsExtra = classExtra;

    newClass->atomName = GlobalAddAtom( class->lpszClassName );
    newClass->wc.lpszClassName = 0;

    if (newClass->wc.style & CS_CLASSDC)
	newClass->hdce = DCE_AllocDCE( DCE_CLASS_DC );
    else newClass->hdce = 0;

      /* Make a copy of the menu name (only if it is a string) */

    if (HIWORD(class->lpszMenuName))
    {
        char *menuname = PTR_SEG_TO_LIN( class->lpszMenuName );
	HANDLE hname = USER_HEAP_ALLOC( strlen(menuname)+1 );
	if (hname)
	{
	    newClass->wc.lpszMenuName = (SEGPTR)USER_HEAP_SEG_ADDR( hname );
	    strcpy( USER_HEAP_LIN_ADDR( hname ), menuname );
	}
    }

    if (classExtra) memset( newClass->wExtra, 0, classExtra );
    firstClass = newClass;
    return newClass->atomName;
}


/***********************************************************************
 *           UnregisterClass    (USER.403)
 */
BOOL UnregisterClass( SEGPTR className, HANDLE hinstance )
{
    CLASS *classPtr;

    hinstance = GetExePtr( hinstance );

      /* Check if we can remove this class */
    if (!(classPtr = CLASS_FindClassByName( className, hinstance )))
        return FALSE;
    if ((classPtr->wc.hInstance != hinstance) || (classPtr->cWindows > 0))
	return FALSE;
    CLASS_FreeClass( classPtr );
    return TRUE;
}


/***********************************************************************
 *           GetClassWord    (USER.129)
 */
WORD GetClassWord( HWND hwnd, short offset )
{
    WND * wndPtr;
    
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
    if (offset >= 0)
        return *(WORD *)(((char *)wndPtr->class->wExtra) + offset);
    switch(offset)
    {
        case GCW_HBRBACKGROUND: return wndPtr->class->wc.hbrBackground;
        case GCW_HCURSOR:       return wndPtr->class->wc.hCursor;
        case GCW_HICON:         return wndPtr->class->wc.hIcon;
        case GCW_HMODULE:       return wndPtr->class->wc.hInstance;
        case GCW_CBWNDEXTRA:    return wndPtr->class->wc.cbWndExtra;
        case GCW_CBCLSEXTRA:    return wndPtr->class->wc.cbClsExtra;
        case GCW_STYLE:         return wndPtr->class->wc.style;
        case GCW_ATOM:          return wndPtr->class->atomName;
    }
    fprintf(stderr, "Warning: invalid offset %d for GetClassWord()\n", offset);
    return 0;
}


/***********************************************************************
 *           SetClassWord    (USER.130)
 */
WORD SetClassWord( HWND hwnd, short offset, WORD newval )
{
    WND * wndPtr;
    WORD *ptr, retval = 0;
    
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
    if (offset >= 0) ptr = (WORD *)(((char *)wndPtr->class->wExtra) + offset);
    else switch(offset)
    {
        case GCW_HBRBACKGROUND: ptr = &wndPtr->class->wc.hbrBackground; break;
        case GCW_HCURSOR:       ptr = &wndPtr->class->wc.hCursor; break;
        case GCW_HICON:         ptr = &wndPtr->class->wc.hIcon; break;
        case GCW_HMODULE:       ptr = &wndPtr->class->wc.hInstance; break;
        case GCW_CBWNDEXTRA:    ptr = &wndPtr->class->wc.cbWndExtra; break;
        case GCW_CBCLSEXTRA:    ptr = &wndPtr->class->wc.cbClsExtra; break;
        case GCW_STYLE:         ptr = &wndPtr->class->wc.style; break;
        case GCW_ATOM:          ptr = &wndPtr->class->atomName; break;
        default:
            fprintf( stderr, "Warning: invalid offset %d for SetClassWord()\n",
                     offset);
            return 0;
    }
    retval = *ptr;
    *ptr = newval;
    return retval;
}


/***********************************************************************
 *           GetClassLong    (USER.131)
 */
LONG GetClassLong( HWND hwnd, short offset )
{
    WND * wndPtr;
    
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
    if (offset >= 0)
        return *(WORD *)(((char *)wndPtr->class->wExtra) + offset);
    switch(offset)
    {
        case GCL_MENUNAME: return (LONG)wndPtr->class->wc.lpszMenuName;
        case GCL_WNDPROC:  return (LONG)wndPtr->class->wc.lpfnWndProc;
    }
    fprintf(stderr, "Warning: invalid offset %d for GetClassLong()\n", offset);
    return 0;
}


/***********************************************************************
 *           SetClassLong    (USER.132)
 */
LONG SetClassLong( HWND hwnd, short offset, LONG newval )
{
    WND * wndPtr;
    LONG *ptr, retval = 0;
    
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
    if (offset >= 0) ptr = (LONG *)(((char *)wndPtr->class->wExtra) + offset);
    else switch(offset)
    {
        case GCL_MENUNAME: ptr = (LONG*)&wndPtr->class->wc.lpszMenuName; break;
        case GCL_WNDPROC:  ptr = (LONG*)&wndPtr->class->wc.lpfnWndProc; break;
        default:
            fprintf( stderr, "Warning: invalid offset %d for SetClassLong()\n",
                     offset);
            return 0;
    }
    retval = *ptr;
    *ptr = newval;
    return retval;
}


/***********************************************************************
 *           GetClassName      (USER.58)
 */
int GetClassName(HWND hwnd, LPSTR lpClassName, short maxCount)
{
    WND *wndPtr;

    /* FIXME: We have the find the correct hInstance */
    dprintf_class(stddeb,"GetClassName(%04x,%p,%d)\n",hwnd,lpClassName,maxCount);
    if (!(wndPtr = WIN_FindWndPtr(hwnd))) return 0;
    
    return GlobalGetAtomName( wndPtr->class->atomName, lpClassName, maxCount );
}


/***********************************************************************
 *           GetClassInfo      (USER.404)
 */
BOOL GetClassInfo( HANDLE hInstance, SEGPTR name, LPWNDCLASS lpWndClass )
{
    CLASS *classPtr;

    dprintf_class( stddeb, "GetClassInfo: hInstance=%04x className=%s\n",
		   hInstance,
                   HIWORD(name) ? (char *)PTR_SEG_TO_LIN(name) : "(int)" );

    hInstance = GetExePtr( hInstance );
    
    if (!(classPtr = CLASS_FindClassByName( name, hInstance ))) return FALSE;
    if (hInstance && (hInstance != classPtr->wc.hInstance)) return FALSE;

    memcpy(lpWndClass, &(classPtr->wc), sizeof(WNDCLASS));
    return TRUE;
}


/***********************************************************************
 *           ClassFirst      (TOOLHELP.69)
 */
BOOL ClassFirst( CLASSENTRY *pClassEntry )
{
    pClassEntry->wNext = 1;
    return ClassNext( pClassEntry );
}


/***********************************************************************
 *           ClassNext      (TOOLHELP.70)
 */
BOOL ClassNext( CLASSENTRY *pClassEntry )
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
    pClassEntry->hInst = class->wc.hInstance;
    pClassEntry->wNext++;
    GlobalGetAtomName( class->atomName, pClassEntry->szClassName,
                       sizeof(pClassEntry->szClassName) );
    return TRUE;
}
