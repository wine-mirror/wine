/*
 * Window properties
 *
 * Copyright 1995, 1996 Alexandre Julliard
 */

#include <string.h>

#include "windef.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "win.h"
#include "heap.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(prop)


typedef struct tagPROPERTY
{
    struct tagPROPERTY *next;     /* Next property in window list */
    HANDLE            handle;   /* User's data */
    LPSTR               string;   /* Property string (or atom) */
} PROPERTY;


/***********************************************************************
 *           PROP_FindProp
 */
static PROPERTY *PROP_FindProp( HWND hwnd, LPCSTR str )
{
    ATOM atom;
    PROPERTY *prop;
    WND *pWnd = WIN_FindWndPtr( hwnd );

    if (!pWnd) return NULL;
    if (HIWORD(str))
    {
        atom = GlobalFindAtomA( str );
        for (prop = pWnd->pProp; prop; prop = prop->next)
        {
            if (HIWORD(prop->string))
            {
                if (!lstrcmpiA( prop->string, str )) goto END;
            }
            else if (LOWORD(prop->string) == atom) goto END;
        }
    }
    else  /* atom */
    {
        atom = LOWORD(str);
        for (prop = pWnd->pProp; (prop); prop = prop->next)
        {
            if (HIWORD(prop->string))
            {
                if (GlobalFindAtomA( prop->string ) == atom) goto END;
            }
            else if (LOWORD(prop->string) == atom) goto END;
        }
    }
    prop = NULL;
END:
    WIN_ReleaseWndPtr(pWnd);
    return prop;
}


/***********************************************************************
 *           GetProp16   (USER.25)
 */
HANDLE16 WINAPI GetProp16( HWND16 hwnd, LPCSTR str )
{
    return (HANDLE16)GetPropA( hwnd, str );
}


/***********************************************************************
 *           GetProp32A   (USER32.281)
 */
HANDLE WINAPI GetPropA( HWND hwnd, LPCSTR str )
{
    PROPERTY *prop = PROP_FindProp( hwnd, str );

    if (HIWORD(str))
        TRACE("(%08x,'%s'): returning %08x\n",
                      hwnd, str, prop ? prop->handle : 0 );
    else
        TRACE("(%08x,#%04x): returning %08x\n",
                      hwnd, LOWORD(str), prop ? prop->handle : 0 );

    return prop ? prop->handle : 0;
}


/***********************************************************************
 *           GetProp32W   (USER32.282)
 */
HANDLE WINAPI GetPropW( HWND hwnd, LPCWSTR str )
{
    LPSTR strA;
    HANDLE ret;

    if (!HIWORD(str)) return GetPropA( hwnd, (LPCSTR)(UINT)LOWORD(str) );
    strA = HEAP_strdupWtoA( GetProcessHeap(), 0, str );
    ret = GetPropA( hwnd, strA );
    HeapFree( GetProcessHeap(), 0, strA );
    return ret;
}


/***********************************************************************
 *           SetProp16   (USER.26)
 */
BOOL16 WINAPI SetProp16( HWND16 hwnd, LPCSTR str, HANDLE16 handle )
{
    return (BOOL16)SetPropA( hwnd, str, handle );
}


/***********************************************************************
 *           SetProp32A   (USER32.497)
 */
BOOL WINAPI SetPropA( HWND hwnd, LPCSTR str, HANDLE handle )
{
    PROPERTY *prop;

    if (HIWORD(str))
        TRACE("%04x '%s' %08x\n", hwnd, str, handle );
    else
        TRACE("%04x #%04x %08x\n",
                      hwnd, LOWORD(str), handle );

    if (!(prop = PROP_FindProp( hwnd, str )))
    {
        /* We need to create it */
        WND *pWnd = WIN_FindWndPtr( hwnd );
        if (!pWnd) return FALSE;
        if (!(prop = HeapAlloc( SystemHeap, 0, sizeof(*prop) )))
        {
            WIN_ReleaseWndPtr(pWnd);
            return FALSE;
        }
        if (!(prop->string = SEGPTR_STRDUP(str)))
        {
            HeapFree( SystemHeap, 0, prop );
            WIN_ReleaseWndPtr(pWnd);
            return FALSE;

        }
        prop->next  = pWnd->pProp;
        pWnd->pProp = prop;
        WIN_ReleaseWndPtr(pWnd);
    }
    prop->handle = handle;
    return TRUE;
}


/***********************************************************************
 *           SetProp32W   (USER32.498)
 */
BOOL WINAPI SetPropW( HWND hwnd, LPCWSTR str, HANDLE handle )
{
    BOOL ret;
    LPSTR strA;

    if (!HIWORD(str))
        return SetPropA( hwnd, (LPCSTR)(UINT)LOWORD(str), handle );
    strA = HEAP_strdupWtoA( GetProcessHeap(), 0, str );
    ret = SetPropA( hwnd, strA, handle );
    HeapFree( GetProcessHeap(), 0, strA );
    return ret;
}


/***********************************************************************
 *           RemoveProp16   (USER.24)
 */
HANDLE16 WINAPI RemoveProp16( HWND16 hwnd, LPCSTR str )
{
    return (HANDLE16)RemovePropA( hwnd, str );
}


/***********************************************************************
 *           RemoveProp32A   (USER32.442)
 */
HANDLE WINAPI RemovePropA( HWND hwnd, LPCSTR str )
{
    ATOM atom;
    HANDLE handle;
    PROPERTY **pprop, *prop;
    WND *pWnd = WIN_FindWndPtr( hwnd );

    if (HIWORD(str))
      TRACE("%04x '%s'\n", hwnd, str );
    else
      TRACE("%04x #%04x\n", hwnd, LOWORD(str));


    if (!pWnd) return (HANDLE)0;
    if (HIWORD(str))
    {
        atom = GlobalFindAtomA( str );
        for (pprop=(PROPERTY**)&pWnd->pProp; (*pprop); pprop = &(*pprop)->next)
        {
            if (HIWORD((*pprop)->string))
            {
                if (!lstrcmpiA( (*pprop)->string, str )) break;
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
                if (GlobalFindAtomA( (*pprop)->string ) == atom) break;
            }
            else if (LOWORD((*pprop)->string) == atom) break;
        }
    }
    WIN_ReleaseWndPtr(pWnd);
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
HANDLE WINAPI RemovePropW( HWND hwnd, LPCWSTR str )
{
    LPSTR strA;
    HANDLE ret;

    if (!HIWORD(str))
        return RemovePropA( hwnd, (LPCSTR)(UINT)LOWORD(str) );
    strA = HEAP_strdupWtoA( GetProcessHeap(), 0, str );
    ret = RemovePropA( hwnd, strA );
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

    TRACE("%04x %08x\n", hwnd, (UINT)func );
    if (!(pWnd = WIN_FindWndPtr( hwnd ))) return -1;
    for (prop = pWnd->pProp; (prop); prop = next)
    {
        /* Already get the next in case the callback */
        /* function removes the current property.    */
        next = prop->next;

        TRACE("  Callback: handle=%08x str='%s'\n",
                      prop->handle, prop->string );
        ret = func( hwnd, SEGPTR_GET(prop->string), prop->handle );
        if (!ret) break;
    }
    WIN_ReleaseWndPtr(pWnd);
    return ret;
}


/***********************************************************************
 *           EnumProps32A   (USER32.186)
 */
INT WINAPI EnumPropsA( HWND hwnd, PROPENUMPROCA func )
{
    return EnumPropsExA( hwnd, (PROPENUMPROCEXA)func, 0 );
}


/***********************************************************************
 *           EnumProps32W   (USER32.189)
 */
INT WINAPI EnumPropsW( HWND hwnd, PROPENUMPROCW func )
{
    return EnumPropsExW( hwnd, (PROPENUMPROCEXW)func, 0 );
}


/***********************************************************************
 *           EnumPropsEx32A   (USER32.187)
 */
INT WINAPI EnumPropsExA(HWND hwnd, PROPENUMPROCEXA func, LPARAM lParam)
{
    PROPERTY *prop, *next;
    WND *pWnd;
    INT ret = -1;

    TRACE("%04x %08x %08lx\n",
                  hwnd, (UINT)func, lParam );
    if (!(pWnd = WIN_FindWndPtr( hwnd ))) return -1;
    for (prop = pWnd->pProp; (prop); prop = next)
    {
        /* Already get the next in case the callback */
        /* function removes the current property.    */
        next = prop->next;

        TRACE("  Callback: handle=%08x str='%s'\n",
                      prop->handle, prop->string );
        ret = func( hwnd, prop->string, prop->handle, lParam );
        if (!ret) break;
    }
    WIN_ReleaseWndPtr(pWnd);
    return ret;
}


/***********************************************************************
 *           EnumPropsEx32W   (USER32.188)
 */
INT WINAPI EnumPropsExW(HWND hwnd, PROPENUMPROCEXW func, LPARAM lParam)
{
    PROPERTY *prop, *next;
    WND *pWnd;
    INT ret = -1;

    TRACE("%04x %08x %08lx\n",
                  hwnd, (UINT)func, lParam );
    if (!(pWnd = WIN_FindWndPtr( hwnd ))) return -1;
    for (prop = pWnd->pProp; (prop); prop = next)
    {
        /* Already get the next in case the callback */
        /* function removes the current property.    */
        next = prop->next;

        TRACE("  Callback: handle=%08x str='%s'\n",
                      prop->handle, prop->string );
        if (HIWORD(prop->string))
        {
            LPWSTR str = HEAP_strdupAtoW( GetProcessHeap(), 0, prop->string );
            ret = func( hwnd, str, prop->handle, lParam );
            HeapFree( GetProcessHeap(), 0, str );
        }
        else
            ret = func( hwnd, (LPCWSTR)(UINT)LOWORD( prop->string ),
                        prop->handle, lParam );
        if (!ret) break;
    }
    WIN_ReleaseWndPtr(pWnd);
    return ret;
}
