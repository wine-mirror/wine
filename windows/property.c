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

DEFAULT_DEBUG_CHANNEL(prop);


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
 *		GetProp (USER.25)
 */
HANDLE16 WINAPI GetProp16( HWND16 hwnd, LPCSTR str )
{
    return (HANDLE16)GetPropA( hwnd, str );
}


/***********************************************************************
 *		GetPropA (USER32.@)
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
 *		GetPropW (USER32.@)
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
 *		SetProp (USER.26)
 */
BOOL16 WINAPI SetProp16( HWND16 hwnd, LPCSTR str, HANDLE16 handle )
{
    return (BOOL16)SetPropA( hwnd, str, handle );
}


/***********************************************************************
 *		SetPropA (USER32.@)
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
        if (!(prop = HeapAlloc( GetProcessHeap(), 0, sizeof(*prop) )))
        {
            WIN_ReleaseWndPtr(pWnd);
            return FALSE;
        }
        if (!(prop->string = SEGPTR_STRDUP(str)))
        {
            HeapFree( GetProcessHeap(), 0, prop );
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
 *		SetPropW (USER32.@)
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
 *		RemoveProp (USER.24)
 */
HANDLE16 WINAPI RemoveProp16( HWND16 hwnd, LPCSTR str )
{
    return (HANDLE16)RemovePropA( hwnd, str );
}


/***********************************************************************
 *		RemovePropA (USER32.@)
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
    HeapFree( GetProcessHeap(), 0, prop );
    return handle;
}


/***********************************************************************
 *		RemovePropW (USER32.@)
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
        HeapFree( GetProcessHeap(), 0, prop );
    }
    pWnd->pProp = NULL;
}


/***********************************************************************
 *		EnumProps (USER.27)
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

        /* SDK docu seems to suggest that EnumProps16 does not retrieve
	 * the string in case of an atom handle, in contrast to Win32 */

        TRACE("  Callback: handle=%08x str=%s\n",
              prop->handle, debugstr_a(prop->string) );
        ret = func( hwnd, SEGPTR_GET(prop->string), prop->handle );
        if (!ret) break;
    }
    WIN_ReleaseWndPtr(pWnd);
    return ret;
}


#define MAX_ATOM_LEN              255
static INT PROPS_EnumPropsA( HWND hwnd, PROPENUMPROCA func, LPARAM lParam, BOOL ex )
{
    PROPERTY *prop, *next;
    WND *pWnd;
    INT ret = -1;
    char atomStr[MAX_ATOM_LEN+1];
    char *pStr;

    TRACE("%04x %08x %08lx%s\n",
                  hwnd, (UINT)func, lParam, ex ? " [Ex]" : "" );
    if (!(pWnd = WIN_FindWndPtr( hwnd ))) return -1;
    for (prop = pWnd->pProp; (prop); prop = next)
    {
        /* Already get the next in case the callback */
        /* function removes the current property.    */
        next = prop->next;

        if (!HIWORD(prop->string))
        { /* get "real" string the atom points to.
	   * This seems to be done for Win32 only */
            if (!(GlobalGetAtomNameA((ATOM)LOWORD(prop->string), atomStr, MAX_ATOM_LEN+1)))
            {
                ERR("huh ? Atom %04x not an atom ??\n",
                    (ATOM)LOWORD(prop->string));
                atomStr[0] = '\0';
            }
            pStr = atomStr;
        }
        else
            pStr = prop->string;

        TRACE("  Callback: handle=%08x str='%s'\n",
            prop->handle, prop->string );

        if (ex) /* EnumPropsEx has an additional lParam !! */
            ret = func( hwnd, pStr, prop->handle, lParam );
        else
            ret = func( hwnd, pStr, prop->handle );
        if (!ret) break;
    }
    WIN_ReleaseWndPtr(pWnd);
    return ret;
}


/***********************************************************************
 *		EnumPropsA (USER32.@)
 */
INT WINAPI EnumPropsA( HWND hwnd, PROPENUMPROCA func )
{
    return PROPS_EnumPropsA(hwnd, func, 0, FALSE);
}


/***********************************************************************
 *		EnumPropsExA (USER32.@)
 */
INT WINAPI EnumPropsExA(HWND hwnd, PROPENUMPROCEXA func, LPARAM lParam)
{
    return PROPS_EnumPropsA(hwnd, (PROPENUMPROCA)func, lParam, TRUE);
}


static INT PROPS_EnumPropsW( HWND hwnd, PROPENUMPROCW func, LPARAM lParam, BOOL ex )
{
    PROPERTY *prop, *next;
    WND *pWnd;
    INT ret = -1;
    char atomStr[MAX_ATOM_LEN+1];
    char *pStr;
    LPWSTR strW;

    TRACE("%04x %08x %08lx%s\n",
                  hwnd, (UINT)func, lParam, ex ? " [Ex]" : "" );
    if (!(pWnd = WIN_FindWndPtr( hwnd ))) return -1;
    for (prop = pWnd->pProp; (prop); prop = next)
    {
        /* Already get the next in case the callback */
        /* function removes the current property.    */
        next = prop->next;

        if (!HIWORD(prop->string))
        { /* get "real" string the atom points to.
	   * This seems to be done for Win32 only */
            if (!(GlobalGetAtomNameA((ATOM)LOWORD(prop->string), atomStr, MAX_ATOM_LEN+1)))
            {
                ERR("huh ? Atom %04x not an atom ??\n",
                    (ATOM)LOWORD(prop->string));
                atomStr[0] = '\0';
            }
            pStr = atomStr;
        }
        else
            pStr = prop->string;

        TRACE("  Callback: handle=%08x str='%s'\n",
            prop->handle, prop->string );

        strW = HEAP_strdupAtoW( GetProcessHeap(), 0, pStr );

        if (ex) /* EnumPropsEx has an additional lParam !! */
            ret = func( hwnd, strW, prop->handle, lParam );
        else
            ret = func( hwnd, strW, prop->handle );
        HeapFree( GetProcessHeap(), 0, strW );
        if (!ret) break;
    }
    WIN_ReleaseWndPtr(pWnd);
    return ret;
}


/***********************************************************************
 *		EnumPropsW (USER32.@)
 */
INT WINAPI EnumPropsW( HWND hwnd, PROPENUMPROCW func )
{
    return PROPS_EnumPropsW(hwnd, func, 0, FALSE);
}

/***********************************************************************
 *		EnumPropsExW (USER32.@)
 */
INT WINAPI EnumPropsExW(HWND hwnd, PROPENUMPROCEXW func, LPARAM lParam)
{
    return PROPS_EnumPropsW(hwnd, (PROPENUMPROCW)func, 0, TRUE);
}
