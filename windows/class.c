/*
 * Window classes functions
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "class.h"
#include "user.h"
#include "win.h"
#include "dce.h"

/* #define DEBUG_CLASS /* */


static HCLASS firstClass = 0;


/***********************************************************************
 *           CLASS_FindClassByName
 *
 * Return a handle and a pointer to the class.
 * 'ptr' can be NULL if the pointer is not needed.
 */
HCLASS CLASS_FindClassByName( char * name, CLASS **ptr )
{
    ATOM atom;
    HCLASS class;
    CLASS * classPtr;

      /* First search task-specific classes */

    if ((atom = FindAtom( name )) != 0)
    {
	for (class = firstClass; (class); class = classPtr->hNext)
	{
	    classPtr = (CLASS *) USER_HEAP_ADDR(class);
	    if (classPtr->wc.style & CS_GLOBALCLASS) continue;
	    if (classPtr->atomName == atom)
	    {
		if (ptr) *ptr = classPtr;
		return class;
	    }
	}
    }
    
      /* Then search global classes */

    if ((atom = GlobalFindAtom( name )) != 0)
    {
	for (class = firstClass; (class); class = classPtr->hNext)
	{
	    classPtr = (CLASS *) USER_HEAP_ADDR(class);
	    if (!(classPtr->wc.style & CS_GLOBALCLASS)) continue;
	    if (classPtr->atomName == atom)
	    {
		if (ptr) *ptr = classPtr;
		return class;
	    }
	}
    }

    return 0;
}


/***********************************************************************
 *           CLASS_FindClassPtr
 *
 * Return a pointer to the CLASS structure corresponding to a HCLASS.
 */
CLASS * CLASS_FindClassPtr( HCLASS hclass )
{
    CLASS * ptr;
    
    if (!hclass) return NULL;
    ptr = (CLASS *) USER_HEAP_ADDR( hclass );
    if (ptr->wMagic != CLASS_MAGIC) return NULL;
    return ptr;
}


/***********************************************************************
 *           RegisterClass    (USER.57)
 */
ATOM RegisterClass( LPWNDCLASS class )
{
    CLASS * newClass, * prevClassPtr;
    HCLASS handle, prevClass;
    
#ifdef DEBUG_CLASS
    printf( "RegisterClass: wndproc=%08x hinst=%d name='%s' background %x\n", 
	    class->lpfnWndProc, class->hInstance, class->lpszClassName,
	    class->hbrBackground );
#endif

      /* Check if a class with this name already exists */

    prevClass = CLASS_FindClassByName( class->lpszClassName, &prevClassPtr );
    if (prevClass)
    {
	  /* Class can be created only if it is local and */
	  /* if the class with the same name is global.   */

	if (class->style & CS_GLOBALCLASS) return 0;
	if (!(prevClassPtr->wc.style & CS_GLOBALCLASS)) return 0;
    }

      /* Create class */

    handle = USER_HEAP_ALLOC( GMEM_MOVEABLE, sizeof(CLASS)+class->cbClsExtra );
    if (!handle) return 0;
    newClass = (CLASS *) USER_HEAP_ADDR( handle );
    newClass->hNext      = firstClass;
    newClass->wMagic     = CLASS_MAGIC;
    newClass->cWindows   = 0;  
    newClass->wc         = *class;

    if (newClass->wc.style & CS_GLOBALCLASS)
	newClass->atomName = GlobalAddAtom( class->lpszClassName );
    else newClass->atomName = AddAtom( class->lpszClassName );
    newClass->wc.lpszClassName = NULL; 

    if (newClass->wc.style & CS_CLASSDC)
	newClass->hdce = DCE_AllocDCE( DCE_CLASS_DC );
    else newClass->hdce = 0;

      /* Make a copy of the menu name (only if it is a string) */

    if ((int)class->lpszMenuName & 0xffff0000)
    {
	HANDLE hname;
	hname = USER_HEAP_ALLOC( GMEM_MOVEABLE, strlen(class->lpszMenuName)+1);
	if (hname)
	{
	    newClass->wc.lpszMenuName = (char *)USER_HEAP_ADDR( hname );
	    strcpy( newClass->wc.lpszMenuName, class->lpszMenuName );
	}
    }

    if (class->cbClsExtra) memset( newClass->wExtra, 0, class->cbClsExtra );
    firstClass = handle;
    return newClass->atomName;
}


/***********************************************************************
 *           UnregisterClass    (USER.403)
 */
BOOL UnregisterClass( LPSTR className, HANDLE instance )
{
    HANDLE class, prevClass;
    CLASS * classPtr, * prevClassPtr;
    
      /* Check if we can remove this class */
    class = CLASS_FindClassByName( className, &classPtr );
    if (!class) return FALSE;
    if ((classPtr->wc.hInstance != instance) || (classPtr->cWindows > 0))
	return FALSE;
    
      /* Remove the class from the linked list */
    if (firstClass == class) firstClass = classPtr->hNext;
    else
    {
	for (prevClass = firstClass; prevClass; prevClass=prevClassPtr->hNext)
	{
	    prevClassPtr = (CLASS *) USER_HEAP_ADDR(prevClass);
	    if (prevClassPtr->hNext == class) break;
	}
	if (!prevClass)
	{
	    printf( "ERROR: Class list corrupted\n" );
	    return FALSE;
	}
	prevClassPtr->hNext = classPtr->hNext;
    }

      /* Delete the class */
    if (classPtr->hdce) DCE_FreeDCE( classPtr->hdce );
    if (classPtr->wc.hbrBackground) DeleteObject( classPtr->wc.hbrBackground );
    if (classPtr->wc.style & CS_GLOBALCLASS) GlobalDeleteAtom( classPtr->atomName );
    else DeleteAtom( classPtr->atomName );
    if ((int)classPtr->wc.lpszMenuName & 0xffff0000)
	USER_HEAP_FREE( (int)classPtr->wc.lpszMenuName & 0xffff );
    USER_HEAP_FREE( class );
    return TRUE;
}


/***********************************************************************
 *           GetClassWord    (USER.129)
 */
WORD GetClassWord( HWND hwnd, short offset )
{
    return (WORD)GetClassLong( hwnd, offset );
}


/***********************************************************************
 *           SetClassWord    (USER.130)
 */
WORD SetClassWord( HWND hwnd, short offset, WORD newval )
{
    CLASS * classPtr;
    WND * wndPtr;
    WORD *ptr, retval = 0;
    
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
    if (!(classPtr = CLASS_FindClassPtr( wndPtr->hClass ))) return 0;
    ptr = (WORD *)(((char *)classPtr->wExtra) + offset);
    retval = *ptr;
    *ptr = newval;
    return retval;
}


/***********************************************************************
 *           GetClassLong    (USER.131)
 */
LONG GetClassLong( HWND hwnd, short offset )
{
    CLASS * classPtr;
    WND * wndPtr;
    
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
    if (!(classPtr = CLASS_FindClassPtr( wndPtr->hClass ))) return 0;
    return *(LONG *)(((char *)classPtr->wExtra) + offset);
}


/***********************************************************************
 *           SetClassLong    (USER.132)
 */
LONG SetClassLong( HWND hwnd, short offset, LONG newval )
{
    CLASS * classPtr;
    WND * wndPtr;
    LONG *ptr, retval = 0;
    
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
    if (!(classPtr = CLASS_FindClassPtr( wndPtr->hClass ))) return 0;
    ptr = (LONG *)(((char *)classPtr->wExtra) + offset);
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
    CLASS *classPtr;

    if (!(wndPtr = WIN_FindWndPtr(hwnd))) return 0;
    if (!(classPtr = CLASS_FindClassPtr(wndPtr->hClass))) return 0;

    return (GetAtomName(classPtr->atomName, lpClassName, maxCount));
}


/***********************************************************************
 *           GetClassInfo      (USER.404)
 */
BOOL GetClassInfo(HANDLE hInstance, LPSTR lpClassName, 
		                    LPWNDCLASS lpWndClass)
{
    CLASS *classPtr;

    if (!(CLASS_FindClassByName(lpClassName, &classPtr))) return FALSE;
    if (hInstance && (hInstance != classPtr->wc.hInstance)) return FALSE;

    memcpy(lpWndClass, &(classPtr->wc), sizeof(WNDCLASS));
    return TRUE;
}
