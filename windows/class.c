/*
 * Window classes functions
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include "class.h"
#include "user.h"
#include "win.h"


static HCLASS firstClass = 0;


/***********************************************************************
 *           CLASS_FindClassByName
 *
 * Return a handle and a pointer to the class.
 */
HCLASS CLASS_FindClassByName( char * name, CLASS **ptr )
{
    HCLASS class = firstClass;
    while(class)
    {
	*ptr = (CLASS *) USER_HEAP_ADDR(class);
	if (!strcasecmp( (*ptr)->wc.lpszClassName, name )) return class;
	class = (*ptr)->hNext;
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
    CLASS * newClass;
    HCLASS handle;
    
#ifdef DEBUG_CLASS
    printf( "RegisterClass: wndproc=%08x hinst=%d name='%s'\n", 
	    class->lpfnWndProc, class->hInstance, class->lpszClassName );
#endif

    handle = USER_HEAP_ALLOC( GMEM_MOVEABLE, sizeof(CLASS)+class->cbClsExtra );
    if (!handle) return 0;
    newClass = (CLASS *) USER_HEAP_ADDR( handle );
    newClass->hNext      = firstClass;
    newClass->wMagic     = CLASS_MAGIC;
    newClass->atomName   = handle;  /* Should be an atom */
    newClass->hDCE       = 0;  /* Should allocate a DCE if needed */
    newClass->cWindows   = 0;  
    newClass->wc         = *class;
        
      /* Class name should also be set to zero. For now we need the
       * name because we don't have atoms.
       */
    newClass->wc.lpszClassName = (char *)malloc(strlen(class->lpszClassName)+1);
    strcpy( newClass->wc.lpszClassName, class->lpszClassName );
    
    if (class->cbClsExtra) memset( newClass->wExtra, 0, class->cbClsExtra );
    
    firstClass = handle;
    return handle;  /* Should be an atom */
}


/***********************************************************************
 *           UnregisterClass    (USER.403)
 */
BOOL UnregisterClass( LPSTR className, HANDLE instance )
{
    HANDLE class, next, prevClass;
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
    WORD retval = 0;
    
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
    if ((classPtr = CLASS_FindClassPtr( wndPtr->hClass )))
    {
	WORD * ptr = (WORD *)(((char *)classPtr->wExtra) + offset);
	retval = *ptr;
	*ptr = newval;
    }
    GlobalUnlock( hwnd );
    return retval;
}


/***********************************************************************
 *           GetClassLong    (USER.131)
 */
LONG GetClassLong( HWND hwnd, short offset )
{
    CLASS * classPtr;
    WND * wndPtr;
    LONG retval = 0;
    
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
    if ((classPtr = CLASS_FindClassPtr( wndPtr->hClass )))
	retval = *(LONG *)(((char *)classPtr->wExtra) + offset);
    GlobalUnlock( hwnd );
    return retval;
}


/***********************************************************************
 *           SetClassLong    (USER.132)
 */
LONG SetClassLong( HWND hwnd, short offset, LONG newval )
{
    CLASS * classPtr;
    WND * wndPtr;
    LONG retval = 0;
    
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
    if ((classPtr = CLASS_FindClassPtr( wndPtr->hClass )))
    {
	LONG * ptr = (LONG *)(((char *)classPtr->wExtra) + offset);
	retval = *ptr;
	*ptr = newval;
    }
    GlobalUnlock( hwnd );
    return retval;
}
