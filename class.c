/*
 * Window classes functions
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include "class.h"


static HCLASS firstClass = 0;


/***********************************************************************
 *           CLASS_FindClassByName
 *
 * Return a handle and a pointer to the class.
 * The caller must GlobalUnlock the pointer.
 */
HCLASS CLASS_FindClassByName( char * name, CLASS **ptr )
{
    HCLASS class, next;
    
    class = firstClass;
    while(class)
    {
	*ptr = (CLASS *) GlobalLock(class);
	if (!strcmp( (*ptr)->wc.lpszClassName, name )) return class;
	next = (*ptr)->hNext;
	GlobalUnlock(class);
	class = next;
    }
    return 0;
}

/***********************************************************************
 *           CLASS_FindClassPtr
 *
 * Return a pointer to the CLASS structure corresponding to a HCLASS.
 * The caller must GlobalUnlock the pointer.
 */
CLASS * CLASS_FindClassPtr( HCLASS hclass )
{
    CLASS * ptr;
    
    if (!hclass) return NULL;
    ptr = (CLASS *) GlobalLock( hclass );
    if (ptr->wMagic != CLASS_MAGIC) 
    {
	GlobalUnlock( hclass );
	return NULL;
    }	
    return ptr;
}


/***********************************************************************
 *           RegisterClass    (USER.57)
 */
ATOM RegisterClass( LPWNDCLASS class )
{
    CLASS * newClass;
    HCLASS handle;
    int i;
    
#ifdef DEBUG_CLASS
    printf( "RegisterClass: wndproc=%08x hinst=%d name='%s'\n", 
	    class->lpfnWndProc, class->hInstance, class->lpszClassName );
#endif

    handle = GlobalAlloc( GMEM_MOVEABLE, sizeof(CLASS)+class->cbClsExtra );
    if (!handle) return 0;
    newClass = (CLASS *) GlobalLock( handle );
    newClass->hNext      = firstClass;
    newClass->wMagic     = CLASS_MAGIC;
    newClass->atomName   = handle;  /* Should be an atom */
    newClass->hDCE       = 0;  /* Should allocate a DCE if needed */
    newClass->cWindows   = 0;  
    newClass->wc         = *class;
        
    newClass->wc.lpszMenuName = 0;

      /* Class name should also be set to zero. For now we need the
       * name because we don't have atoms.
       */
    newClass->wc.lpszClassName = (char *)malloc(strlen(class->lpszClassName)+1);
    strcpy( newClass->wc.lpszClassName, class->lpszClassName );
    
    if (class->cbClsExtra) memset( newClass->wExtra, 0, class->cbClsExtra );

    GlobalUnlock( handle );
    
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
    {
	GlobalUnlock( class );
	return FALSE;
    }
    
      /* Remove the class from the linked list */
    if (firstClass == class) firstClass = classPtr->hNext;
    else
    {
	prevClass = firstClass; 
	while (prevClass)
	{
	    prevClassPtr = (CLASS *) GlobalLock(prevClass);
	    next == prevClassPtr->hNext;
	    if (next == class) break;
	    GlobalUnlock(prevClass);
	    prevClass = next;
	}
	if (!prevClass)
	{
	    printf( "ERROR: Class list corrupted\n" );
	    return FALSE;
	}
	prevClassPtr->hNext = classPtr->hNext;
	GlobalUnlock( prevClass );
    }
    
    GlobalUnlock( class );
    GlobalFree( class );
    return TRUE;
}
