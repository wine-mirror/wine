/*
 * Window properties
 *
 * Copyright 1995, 1996 Alexandre Julliard
 */

#include <string.h>
#include "win.h"
#include "heap.h"
#include "debug.h"


typedef struct tagPROPERTY
{
    struct tagPROPERTY *next;     /* Next property in window list */
    HANDLE32            handle;   /* User's data */
    LPSTR               string;   /* Property string (or atom) */
} PROPERTY;


/***********************************************************************
 *           PROP_FindProp
 */
static PROPERTY *PROP_FindProp( HWND32 hwnd, LPCSTR str )
{
    ATOM atom;
    PROPERTY *prop;
    WND *pWnd = WIN_FindWndPtr( hwnd );

    if (!pWnd) return NULL;
    if (HIWORD(str))
    {
        atom = GlobalFindAtom32A( str );
        for (prop = pWnd->pProp; prop; prop = prop->next)
        {
            if (HIWORD(prop->string))
            {
                if (!lstrcmpi32A( prop->string, str )) return prop;
            }
            else if (LOWORD(prop->string) == atom) return prop;
        }
    }
    else  /* atom */
    {
        atom = LOWORD(str);
        for (prop = pWnd->pProp; (prop); prop = prop->next)
        {
            if (HIWORD(prop->string))
            {
                if (GlobalFindAtom32A( prop->string ) == atom) return prop;
            }
            else if (LOWORD(prop->string) == atom) return prop;
        }
    }
    return NULL;
}


/***********************************************************************
 *           GetProp16   (USER.25)
 */
HANDLE16 WINAPI GetProp16( HWND16 hwnd, LPCSTR str )
{
    return (HANDLE16)GetProp32A( hwnd, str );
}


/***********************************************************************
 *           GetProp32A   (USER32.281)
 */
HANDLE32 WINAPI GetProp32A( HWND32 hwnd, LPCSTR str )
{
    PROPERTY *prop = PROP_FindProp( hwnd, str );

    if (HIWORD(str))
        TRACE(prop, "(%08x,'%s'): returning %08x\n",
                      hwnd, str, prop ? prop->handle : 0 );
    else
        TRACE(prop, "(%08x,#%04x): returning %08x\n",
                      hwnd, LOWORD(str), prop ? prop->handle : 0 );

    return prop ? prop->handle : 0;
}


/***********************************************************************
 *           GetProp32W   (USER32.282)
 */
HANDLE32 WINAPI GetProp32W( HWND32 hwnd, LPCWSTR str )
{
    LPSTR strA;
    HANDLE32 ret;

    if (!HIWORD(str)) return GetProp32A( hwnd, (LPCSTR)(UINT32)LOWORD(str) );
    strA = HEAP_strdupWtoA( GetProcessHeap(), 0, str );
    ret = GetProp32A( hwnd, strA );
    HeapFree( GetProcessHeap(), 0, strA );
    return ret;
}


/***********************************************************************
 *           SetProp16   (USER.26)
 */
BOOL16 WINAPI SetProp16( HWND16 hwnd, LPCSTR str, HANDLE16 handle )
{
    return (BOOL16)SetProp32A( hwnd, str, handle );
}


/***********************************************************************
 *           SetProp32A   (USER32.497)
 */
BOOL32 WINAPI SetProp32A( HWND32 hwnd, LPCSTR str, HANDLE32 handle )
{
    PROPERTY *prop;

    if (HIWORD(str))
        TRACE(prop, "%04x '%s' %08x\n", hwnd, str, handle );
    else
        TRACE(prop, "%04x #%04x %08x\n",
                      hwnd, LOWORD(str), handle );

    if (!(prop = PROP_FindProp( hwnd, str )))
    {
        /* We need to create it */
        WND *pWnd = WIN_FindWndPtr( hwnd );
        if (!pWnd) return FALSE;
        if (!(prop = HeapAlloc( SystemHeap, 0, sizeof(*prop) ))) return FALSE;
        if (!(prop->string = SEGPTR_STRDUP(str)))
        {
            HeapFree( SystemHeap, 0, prop );
            return FALSE;
        }
        prop->next  = pWnd->pProp;
        pWnd->pProp = prop;
    }
    prop->handle = handle;
    return TRUE;
}


/***********************************************************************
 *           SetProp32W   (USER32.498)
 */
BOOL32 WINAPI SetProp32W( HWND32 hwnd, LPCWSTR str, HANDLE32 handle )
{
    BOOL32 ret;
    LPSTR strA;

    if (!HIWORD(str))
        return SetProp32A( hwnd, (LPCSTR)(UINT32)LOWORD(str), handle );
    strA = HEAP_strdupWtoA( GetProcessHeap(), 0, str );
    ret = SetProp32A( hwnd, strA, handle );
    HeapFree( GetProcessHeap(), 0, strA );
    return ret;
}


/***********************************************************************
 *           RemoveProp16   (USER.24)
 */
HANDLE16 WINAPI RemoveProp16( HWND16 hwnd, LPCSTR str )
{
    return (HANDLE16)RemoveProp32A( hwnd, str );
}


/***********************************************************************
 *           RemoveProp32A   (USER32.442)
 */
HANDLE32 WINAPI RemoveProp32A( HWND32 hwnd, LPCSTR str )
{
    ATOM atom;
    HANDLE32 handle;
    PROPERTY **pprop, *prop;
    WND *pWnd = WIN_FindWndPtr( hwnd );

    if (HIWORD(str))
      TRACE(prop, "%04x '%s'\n", hwnd, str );
    else
      TRACE(prop, "%04x #%04x\n", hwnd, LOWORD(str));


    if (!pWnd) return NULL;
    if (HIWORD(str))
    {
        atom = GlobalFindAtom32A( str );
        for (pprop=(PROPERTY**)&pWnd->pProp; (*pprop); pprop = &(*pprop)->next)
        {
            if (HIWORD((*pprop)->string))
            {
                if (!lstrcmpi32A( (*pprop)->string, str )) break;
            }
            else if (LOWORD((*pprop)->string) == atom) break;
        }
    }
    else  /* atom */
    {
        atom = LOWORD(str);
        for (pprop=(PROPERTY**)&pWnd->pProp; (*pprop); pprop = &(*pprop)->next)
        {
            if (HIWORD((*pprop)->string))
            {
                if (GlobalFindAtom32A( (*pprop)->string ) == atom) break;
            }
            else if (LOWORD((*pprop)->string) == atom) break;
        }
    }
    if (!*pprop) return 0;
    prop   = *pprop;
    handle = prop->handle;
    *pprop = prop->next;
    SEGPTR_FREE(prop->string);
    HeapFree( SystemHeap, 0, prop );
    return handle;
}


/***********************************************************************
 *           RemoveProp32W   (USER32.443)
 */
HANDLE32 WINAPI RemoveProp32W( HWND32 hwnd, LPCWSTR str )
{
    LPSTR strA;
    HANDLE32 ret;

    if (!HIWORD(str))
        return RemoveProp32A( hwnd, (LPCSTR)(UINT32)LOWORD(str) );
    strA = HEAP_strdupWtoA( GetProcessHeap(), 0, str );
    ret = RemoveProp32A( hwnd, strA );
    HeapFree( GetProcessHeap(), 0, strA );
    return ret;
}


/***********************************************************************
 *           PROPERTY_RemoveWindowProps
 *
 * Remove all properties of a window.
 */
void PROPERTY_RemoveWindowProps( WND *pWnd )
{
    PROPERTY *prop, *next;

    for (prop = pWnd->pProp; (prop); prop = next)
    {
        next = prop->next;
        SEGPTR_FREE( prop->string );
        HeapFree( SystemHeap, 0, prop );
    }
    pWnd->pProp = NULL;
}


/***********************************************************************
 *           EnumProps16   (USER.27)
 */
INT16 WINAPI EnumProps16( HWND16 hwnd, PROPENUMPROC16 func )
{
    PROPERTY *prop, *next;
    WND *pWnd;
    INT16 ret = -1;

    TRACE(prop, "%04x %08x\n", hwnd, (UINT32)func );
    if (!(pWnd = WIN_FindWndPtr( hwnd ))) return -1;
    for (prop = pWnd->pProp; (prop); prop = next)
    {
        /* Already get the next in case the callback */
        /* function removes the current property.    */
        next = prop->next;

        TRACE(prop, "  Callback: handle=%08x str='%s'\n",
                      prop->handle, prop->string );
        ret = func( hwnd, SEGPTR_GET(prop->string), prop->handle );
        if (!ret) break;
    }
    return ret;
}


/***********************************************************************
 *           EnumProps32A   (USER32.186)
 */
INT32 WINAPI EnumProps32A( HWND32 hwnd, PROPENUMPROC32A func )
{
    return EnumPropsEx32A( hwnd, (PROPENUMPROCEX32A)func, 0 );
}


/***********************************************************************
 *           EnumProps32W   (USER32.189)
 */
INT32 WINAPI EnumProps32W( HWND32 hwnd, PROPENUMPROC32W func )
{
    return EnumPropsEx32W( hwnd, (PROPENUMPROCEX32W)func, 0 );
}


/***********************************************************************
 *           EnumPropsEx32A   (USER32.187)
 */
INT32 WINAPI EnumPropsEx32A(HWND32 hwnd, PROPENUMPROCEX32A func, LPARAM lParam)
{
    PROPERTY *prop, *next;
    WND *pWnd;
    INT32 ret = -1;

    TRACE(prop, "%04x %08x %08lx\n",
                  hwnd, (UINT32)func, lParam );
    if (!(pWnd = WIN_FindWndPtr( hwnd ))) return -1;
    for (prop = pWnd->pProp; (prop); prop = next)
    {
        /* Already get the next in case the callback */
        /* function removes the current property.    */
        next = prop->next;

        TRACE(prop, "  Callback: handle=%08x str='%s'\n",
                      prop->handle, prop->string );
        ret = func( hwnd, prop->string, prop->handle, lParam );
        if (!ret) break;
    }
    return ret;
}


/***********************************************************************
 *           EnumPropsEx32W   (USER32.188)
 */
INT32 WINAPI EnumPropsEx32W(HWND32 hwnd, PROPENUMPROCEX32W func, LPARAM lParam)
{
    PROPERTY *prop, *next;
    WND *pWnd;
    INT32 ret = -1;

    TRACE(prop, "%04x %08x %08lx\n",
                  hwnd, (UINT32)func, lParam );
    if (!(pWnd = WIN_FindWndPtr( hwnd ))) return -1;
    for (prop = pWnd->pProp; (prop); prop = next)
    {
        /* Already get the next in case the callback */
        /* function removes the current property.    */
        next = prop->next;

        TRACE(prop, "  Callback: handle=%08x str='%s'\n",
                      prop->handle, prop->string );
        if (HIWORD(prop->string))
        {
            LPWSTR str = HEAP_strdupAtoW( GetProcessHeap(), 0, prop->string );
            ret = func( hwnd, str, prop->handle, lParam );
            HeapFree( GetProcessHeap(), 0, str );
        }
        else
            ret = func( hwnd, (LPCWSTR)(UINT32)LOWORD( prop->string ),
                        prop->handle, lParam );
        if (!ret) break;
    }
    return ret;
}
