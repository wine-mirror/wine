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
/* #define DEBUG_CLASS */
#include "debug.h"


static HCLASS firstClass = 0;


/***********************************************************************
 *           CLASS_FindClassByName
 *
 * Return a handle and a pointer to the class.
 * 'ptr' can be NULL if the pointer is not needed.
 */
HCLASS CLASS_FindClassByName( SEGPTR name, WORD hinstance, CLASS **ptr )
{
    ATOM atom;
    HCLASS class;
    CLASS * classPtr;

    if (!(atom = GlobalFindAtom( name ))) return 0;

      /* First search task-specific classes */

    for (class = firstClass; (class); class = classPtr->hNext)
    {
        classPtr = (CLASS *) USER_HEAP_LIN_ADDR(class);
        if (classPtr->wc.style & CS_GLOBALCLASS) continue;
        if ((classPtr->atomName == atom) && 
            ((hinstance==0xffff )|| (hinstance == classPtr->wc.hInstance)))
        {
            if (ptr) *ptr = classPtr;
            return class;
        }
    }
    
      /* Then search global classes */

    for (class = firstClass; (class); class = classPtr->hNext)
    {
        classPtr = (CLASS *) USER_HEAP_LIN_ADDR(class);
        if (!(classPtr->wc.style & CS_GLOBALCLASS)) continue;
        if (classPtr->atomName == atom)
        {
            if (ptr) *ptr = classPtr;
            return class;
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
    ptr = (CLASS *) USER_HEAP_LIN_ADDR( hclass );
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
    prevClass = CLASS_FindClassByName( class->lpszClassName,
                                       class->hInstance, &prevClassPtr );
    if (prevClass)
    {
	  /* Class can be created only if it is local and */
	  /* if the class with the same name is global.   */

	if (class->style & CS_GLOBALCLASS) return 0;
	if (!(prevClassPtr->wc.style & CS_GLOBALCLASS)) return 0;
    }

      /* Create class */

    classExtra = (class->cbClsExtra < 0) ? 0 : class->cbClsExtra;
    handle = USER_HEAP_ALLOC( sizeof(CLASS) + classExtra );
    if (!handle) return 0;
    newClass = (CLASS *) USER_HEAP_LIN_ADDR( handle );
    newClass->hNext         = firstClass;
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
	    newClass->wc.lpszMenuName = USER_HEAP_SEG_ADDR( hname );
	    strcpy( USER_HEAP_LIN_ADDR( hname ), menuname );
	}
    }

    if (classExtra) memset( newClass->wExtra, 0, classExtra );
    firstClass = handle;
    return newClass->atomName;
}


/***********************************************************************
 *           UnregisterClass    (USER.403)
 */
BOOL UnregisterClass( SEGPTR className, HANDLE hinstance )
{
    HANDLE class, prevClass;
    CLASS * classPtr, * prevClassPtr;
    
    hinstance = GetExePtr( hinstance );
      /* Check if we can remove this class */
    class = CLASS_FindClassByName( className, hinstance, &classPtr );
    if (!class) return FALSE;
    if ((classPtr->wc.hInstance != hinstance) || (classPtr->cWindows > 0))
	return FALSE;
    
      /* Remove the class from the linked list */
    if (firstClass == class) firstClass = classPtr->hNext;
    else
    {
	for (prevClass = firstClass; prevClass; prevClass=prevClassPtr->hNext)
	{
	    prevClassPtr = (CLASS *) USER_HEAP_LIN_ADDR(prevClass);
	    if (prevClassPtr->hNext == class) break;
	}
	if (!prevClass)
	{
	    fprintf(stderr, "ERROR: Class list corrupted\n" );
	    return FALSE;
	}
	prevClassPtr->hNext = classPtr->hNext;
    }

      /* Delete the class */
    if (classPtr->hdce) DCE_FreeDCE( classPtr->hdce );
    if (classPtr->wc.hbrBackground) DeleteObject( classPtr->wc.hbrBackground );
    GlobalDeleteAtom( classPtr->atomName );
    if (HIWORD(classPtr->wc.lpszMenuName))
	USER_HEAP_FREE( LOWORD(classPtr->wc.lpszMenuName) );
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

    /* FIXME: We have the find the correct hInstance */
    dprintf_class(stddeb,"GetClassName(%x,%p,%d)\n",hwnd,lpClassName,maxCount);
    if (!(wndPtr = WIN_FindWndPtr(hwnd))) return 0;
    if (!(classPtr = CLASS_FindClassPtr(wndPtr->hClass))) return 0;
    
    return GlobalGetAtomName(classPtr->atomName, lpClassName, maxCount);
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
    
    if (!(CLASS_FindClassByName( name, hInstance, &classPtr))) return FALSE;
    if (hInstance && (hInstance != classPtr->wc.hInstance)) return FALSE;

    memcpy(lpWndClass, &(classPtr->wc), sizeof(WNDCLASS));
    return TRUE;
}


/***********************************************************************
 *           ClassFirst      (TOOLHELP.69)
 */
BOOL ClassFirst( CLASSENTRY *pClassEntry )
{
    pClassEntry->wNext = firstClass;
    return ClassNext( pClassEntry );
}


/***********************************************************************
 *           ClassNext      (TOOLHELP.70)
 */
BOOL ClassNext( CLASSENTRY *pClassEntry )
{
    CLASS *classPtr = (CLASS *) USER_HEAP_LIN_ADDR( pClassEntry->wNext );
    if (!classPtr) return FALSE;

    pClassEntry->hInst = classPtr->wc.hInstance;
    pClassEntry->wNext = classPtr->hNext;
    GlobalGetAtomName( classPtr->atomName, pClassEntry->szClassName,
                       sizeof(pClassEntry->szClassName) );
    return TRUE;
}
