/*
 * Window classes functions
 *
 * Copyright 1993 Alexandre Julliard
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "class.h"
#include "heap.h"
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

    if (ptr->magic != CLASS_MAGIC)
    {
        fprintf( stderr, "%p is not a class\n", ptr );
        return;
    }

    GlobalGetAtomName( ptr->atomName, className, sizeof(className) );

    fprintf( stderr, "Class %p:\n", ptr );
    fprintf( stderr,
             "next=%p  name=%04x '%s'  style=%08x  wndProc=%08lx\n"
             "inst=%04x  hdce=%04x  icon=%04x  cursor=%04x  bkgnd=%04x\n"
             "clsExtra=%d  winExtra=%d  #windows=%d\n",
             ptr->next, ptr->atomName, className, ptr->style,
             (DWORD)ptr->lpfnWndProc, ptr->hInstance, ptr->hdce,
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

    fprintf( stderr, " Class   Name                 Style   WndProc\n" );
    for (ptr = firstClass; ptr; ptr = ptr->next)
    {
        GlobalGetAtomName( ptr->atomName, className, sizeof(className) );
        fprintf( stderr, "%08lx %-20.20s %08x %08lx\n", (DWORD)ptr, className,
                 ptr->style, (DWORD)ptr->lpfnWndProc );
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
    if (classPtr->hbrBackground) DeleteObject( classPtr->hbrBackground );
    GlobalDeleteAtom( classPtr->atomName );
    if (HIWORD(classPtr->lpszMenuName))
	USER_HEAP_FREE( (HANDLE)classPtr->lpszMenuName );
    HeapFree( SystemHeap, 0, classPtr );
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

      /* First search task-specific classes */

    for (class = firstClass; (class); class = class->next)
    {
        if (class->style & CS_GLOBALCLASS) continue;
        if ((class->atomName == atom) && 
            ((hinstance==(HINSTANCE16)0xffff) ||
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
 *           CLASS_FindClassByName
 *
 * Return a pointer to the class.
 */
CLASS *CLASS_FindClassByName( SEGPTR name, HINSTANCE hinstance )
{
    ATOM atom;

    if (!(atom = GlobalFindAtom( name ))) return 0;
    return CLASS_FindClassByAtom( atom, hinstance );
}


/***********************************************************************
 *           RegisterClass16    (USER.57)
 */
ATOM RegisterClass16( const WNDCLASS16 *wc )
{
    CLASS * newClass, * prevClass;
    HANDLE16 hInstance;
    int classExtra;

    dprintf_class( stddeb, "RegisterClass: wndproc=%08lx hinst=%04x name='%s' background %04x\n",
                 (DWORD)wc->lpfnWndProc, wc->hInstance,
                 HIWORD(wc->lpszClassName) ?
                  (char *)PTR_SEG_TO_LIN(wc->lpszClassName) : "(int)",
                 wc->hbrBackground );
    dprintf_class(stddeb,"               style=%04x clsExtra=%d winExtra=%d\n",
                  wc->style, wc->cbClsExtra, wc->cbWndExtra );
    
      /* Window classes are owned by modules, not instances */
    hInstance = GetExePtr( wc->hInstance );
    
      /* Check if a class with this name already exists */
    prevClass = CLASS_FindClassByName( wc->lpszClassName, hInstance );
    if (prevClass)
    {
	  /* Class can be created only if it is local and */
	  /* if the class with the same name is global.   */

	if (wc->style & CS_GLOBALCLASS) return 0;
	if (!(prevClass->style & CS_GLOBALCLASS)) return 0;
    }

      /* Create class */

    classExtra = (wc->cbClsExtra < 0) ? 0 : wc->cbClsExtra;
    newClass = (CLASS *)HeapAlloc( SystemHeap, 0, sizeof(CLASS) + classExtra );
    if (!newClass) return 0;
    newClass->next          = firstClass;
    newClass->magic         = CLASS_MAGIC;
    newClass->cWindows      = 0;  
    newClass->style         = wc->style;
    newClass->lpfnWndProc   = wc->lpfnWndProc;
    newClass->cbWndExtra    = (wc->cbWndExtra < 0) ? 0 : wc->cbWndExtra;
    newClass->cbClsExtra    = classExtra;
    newClass->lpszMenuName  = wc->lpszMenuName;
    newClass->hInstance     = hInstance;
    newClass->hIcon         = wc->hIcon;
    newClass->hCursor       = wc->hCursor;
    newClass->hbrBackground = wc->hbrBackground;

    newClass->atomName = GlobalAddAtom( wc->lpszClassName );

    if (newClass->style & CS_CLASSDC)
	newClass->hdce = DCE_AllocDCE( DCE_CLASS_DC );
    else newClass->hdce = 0;

      /* Make a copy of the menu name (only if it is a string) */

    if (HIWORD(wc->lpszMenuName))
    {
        char *menuname = PTR_SEG_TO_LIN( wc->lpszMenuName );
	HANDLE hname = USER_HEAP_ALLOC( strlen(menuname)+1 );
	if (hname)
	{
	    newClass->lpszMenuName = (SEGPTR)USER_HEAP_SEG_ADDR( hname );
	    strcpy( USER_HEAP_LIN_ADDR( hname ), menuname );
	}
    }

    if (classExtra) memset( newClass->wExtra, 0, classExtra );
    firstClass = newClass;
    return newClass->atomName;
}


/***********************************************************************
 *           RegisterClass32A      (USER32.426)
 */
ATOM RegisterClass32A( const WNDCLASS32A* wc )
{
    WNDCLASS16 copy;
    HANDLE classh = 0, menuh = 0;
    SEGPTR classsegp, menusegp;
    char *classbuf, *menubuf;

    ATOM retval;
    copy.style=wc->style;
    ALIAS_RegisterAlias(0,0,(DWORD)wc->lpfnWndProc);
    copy.lpfnWndProc=wc->lpfnWndProc;
    copy.cbClsExtra=wc->cbClsExtra;
    copy.cbWndExtra=wc->cbWndExtra;
    copy.hInstance=(HINSTANCE)wc->hInstance;
    copy.hIcon=(HICON)wc->hIcon;
    copy.hCursor=(HCURSOR)wc->hCursor;
    copy.hbrBackground=(HBRUSH)wc->hbrBackground;
    
    /* FIXME: There has to be a better way of doing this - but neither
       malloc nor alloca will work */

    if(wc->lpszMenuName)
    {
        menuh = GlobalAlloc16(0, strlen(wc->lpszMenuName)+1);
        menusegp = WIN16_GlobalLock16(menuh);
        menubuf = PTR_SEG_TO_LIN(menusegp);
        strcpy( menubuf, wc->lpszMenuName);
        copy.lpszMenuName=menusegp;
    }else
        copy.lpszMenuName=0;
    if(wc->lpszClassName)
    {
        classh = GlobalAlloc16(0, strlen(wc->lpszClassName)+1);
        classsegp = WIN16_GlobalLock16(classh);
        classbuf = PTR_SEG_TO_LIN(classsegp);
        strcpy( classbuf, wc->lpszClassName);
        copy.lpszClassName=classsegp;
    }
    retval = RegisterClass16(&copy);
    GlobalFree16(menuh);
    GlobalFree16(classh);
    return retval;
}


/***********************************************************************
 *           UnregisterClass16    (USER.403)
 */
BOOL UnregisterClass16( SEGPTR className, HINSTANCE16 hinstance )
{
    CLASS *classPtr;

    hinstance = GetExePtr( hinstance );

      /* Check if we can remove this class */
    if (!(classPtr = CLASS_FindClassByName( className, hinstance )))
        return FALSE;
    if ((classPtr->hInstance != hinstance) || (classPtr->cWindows > 0))
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
        case GCW_HBRBACKGROUND: return wndPtr->class->hbrBackground;
        case GCW_HCURSOR:       return wndPtr->class->hCursor;
        case GCW_HICON:         return wndPtr->class->hIcon;
        case GCW_HMODULE:       return wndPtr->class->hInstance;
        case GCW_ATOM:          return wndPtr->class->atomName;
        case GCW_STYLE:
        case GCW_CBWNDEXTRA:
        case GCW_CBCLSEXTRA:
            return (WORD)GetClassLong( hwnd, offset );
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
        case GCW_HBRBACKGROUND: ptr = &wndPtr->class->hbrBackground; break;
        case GCW_HCURSOR:       ptr = &wndPtr->class->hCursor; break;
        case GCW_HICON:         ptr = &wndPtr->class->hIcon; break;
        case GCW_HMODULE:       ptr = &wndPtr->class->hInstance; break;
        case GCW_ATOM:          ptr = &wndPtr->class->atomName; break;
        case GCW_STYLE:
        case GCW_CBWNDEXTRA:
        case GCW_CBCLSEXTRA:
            return (WORD)SetClassLong( hwnd, offset, (LONG)newval );
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
        case GCL_STYLE:      return (LONG)wndPtr->class->style;
        case GCL_CBWNDEXTRA: return (LONG)wndPtr->class->cbWndExtra;
        case GCL_CBCLSEXTRA: return (LONG)wndPtr->class->cbClsExtra;
        case GCL_MENUNAME:   return (LONG)wndPtr->class->lpszMenuName;
        case GCL_WNDPROC:    return (LONG)wndPtr->class->lpfnWndProc;
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
        case GCL_STYLE:      ptr = (LONG*)&wndPtr->class->style; break;
        case GCL_CBWNDEXTRA: ptr = (LONG*)&wndPtr->class->cbWndExtra; break;
        case GCL_CBCLSEXTRA: ptr = (LONG*)&wndPtr->class->cbClsExtra; break;
        case GCL_MENUNAME:   ptr = (LONG*)&wndPtr->class->lpszMenuName; break;
        case GCL_WNDPROC:    ptr = (LONG*)&wndPtr->class->lpfnWndProc; break;
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
BOOL GetClassInfo( HANDLE hInstance, SEGPTR name, WNDCLASS16 *lpWndClass )
{
    CLASS *classPtr;

    dprintf_class( stddeb, "GetClassInfo: hInstance=%04x className=%s\n",
		   hInstance,
                   HIWORD(name) ? (char *)PTR_SEG_TO_LIN(name) : "(int)" );

    hInstance = GetExePtr( hInstance );
    
    if (!(classPtr = CLASS_FindClassByName( name, hInstance ))) return FALSE;
    if (hInstance && (hInstance != classPtr->hInstance)) return FALSE;

    lpWndClass->style         = (UINT16)classPtr->style;
    lpWndClass->lpfnWndProc   = classPtr->lpfnWndProc;
    lpWndClass->cbClsExtra    = (INT16)classPtr->cbClsExtra;
    lpWndClass->cbWndExtra    = (INT16)classPtr->cbWndExtra;
    lpWndClass->hInstance     = classPtr->hInstance;
    lpWndClass->hIcon         = classPtr->hIcon;
    lpWndClass->hCursor       = classPtr->hCursor;
    lpWndClass->hbrBackground = classPtr->hbrBackground;
    lpWndClass->lpszMenuName  = classPtr->lpszMenuName;
    lpWndClass->lpszClassName = 0;

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
    pClassEntry->hInst = class->hInstance;
    pClassEntry->wNext++;
    GlobalGetAtomName( class->atomName, pClassEntry->szClassName,
                       sizeof(pClassEntry->szClassName) );
    return TRUE;
}
