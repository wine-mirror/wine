/*
 * Window properties
 *
 * Copyright 1995, 1996 Alexandre Julliard
 */

#define NO_TRANSITION_TYPES  /* This file is Win32-clean */
#include <string.h>
#include "win.h"
#include "heap.h"
#include "callback.h"
#include "string32.h"
#include "stddebug.h"
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
    PROPERTY *prop;
    WND *pWnd = WIN_FindWndPtr( hwnd );

    if (!pWnd) return NULL;
    if (HIWORD(str))
    {
        for (prop = pWnd->pProp; prop; prop = prop->next)
            if (HIWORD(prop->string) && !lstrcmpi32A( prop->string, str ))
                return prop;
    }
    else  /* atom */
    {
        for (prop = pWnd->pProp; (prop); prop = prop->next)
            if (!HIWORD(prop->string) && (LOWORD(prop->string) == LOWORD(str)))
                return prop;
    }
    return NULL;
}


/***********************************************************************
 *           GetProp16   (USER.25)
 */
HANDLE16 GetProp16( HWND16 hwnd, LPCSTR str )
{
    return (HANDLE16)GetProp32A( hwnd, str );
}


/***********************************************************************
 *           GetProp32A   (USER32.280)
 */
HANDLE32 GetProp32A( HWND32 hwnd, LPCSTR str )
{
    PROPERTY *prop = PROP_FindProp( hwnd, str );

    dprintf_prop( stddeb, "GetProp(%08x,'%s'): returning %08x\n",
                  hwnd, str, prop ? prop->handle : 0 );
    return prop ? prop->handle : 0;
}


/***********************************************************************
 *           GetProp32W   (USER32.281)
 */
HANDLE32 GetProp32W( HWND32 hwnd, LPCWSTR str )
{
    LPSTR strA;
    HANDLE32 ret;

    if (!HIWORD(str)) return GetProp32A( hwnd, (LPCSTR)(UINT32)LOWORD(str) );
    strA = STRING32_DupUniToAnsi( str );
    ret = GetProp32A( hwnd, strA );
    free( strA );
    return ret;
}


/***********************************************************************
 *           SetProp16   (USER.26)
 */
BOOL16 SetProp16( HWND16 hwnd, LPCSTR str, HANDLE16 handle )
{
    return (BOOL16)SetProp32A( hwnd, str, handle );
}


/***********************************************************************
 *           SetProp32A   (USER32.496)
 */
BOOL32 SetProp32A( HWND32 hwnd, LPCSTR str, HANDLE32 handle )
{
    PROPERTY *prop;

    dprintf_prop( stddeb, "SetProp: %04x '%s' %08x\n", hwnd, str, handle );
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
 *           SetProp32W   (USER32.497)
 */
BOOL32 SetProp32W( HWND32 hwnd, LPCWSTR str, HANDLE32 handle )
{
    BOOL32 ret;
    LPSTR strA;

    if (!HIWORD(str))
        return SetProp32A( hwnd, (LPCSTR)(UINT32)LOWORD(str), handle );
    strA = STRING32_DupUniToAnsi( str );
    ret = SetProp32A( hwnd, strA, handle );
    free( strA );
    return ret;
}


/***********************************************************************
 *           RemoveProp16   (USER.24)
 */
HANDLE16 RemoveProp16( HWND16 hwnd, LPCSTR str )
{
    return (HANDLE16)RemoveProp32A( hwnd, str );
}


/***********************************************************************
 *           RemoveProp32A   (USER32.441)
 */
HANDLE32 RemoveProp32A( HWND32 hwnd, LPCSTR str )
{
    HANDLE32 handle;
    PROPERTY **pprop, *prop;
    WND *pWnd = WIN_FindWndPtr( hwnd );

    dprintf_prop( stddeb, "RemoveProp: %04x '%s'\n", hwnd, str );
    if (!pWnd) return NULL;
    if (HIWORD(str))
    {
        for (pprop=(PROPERTY**)&pWnd->pProp; (*pprop); pprop = &(*pprop)->next)
            if (HIWORD((*pprop)->string) &&
                !lstrcmpi32A( (*pprop)->string, str )) break;
    }
    else  /* atom */
    {
        for (pprop=(PROPERTY**)&pWnd->pProp; (*pprop); pprop = &(*pprop)->next)
            if (!HIWORD((*pprop)->string) &&
                (LOWORD((*pprop)->string) == LOWORD(str))) break;
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
 *           RemoveProp32W   (USER32.442)
 */
HANDLE32 RemoveProp32W( HWND32 hwnd, LPCWSTR str )
{
    LPSTR strA;
    HANDLE32 ret;

    if (!HIWORD(str))
        return RemoveProp32A( hwnd, (LPCSTR)(UINT32)LOWORD(str) );
    strA = STRING32_DupUniToAnsi( str );
    ret = RemoveProp32A( hwnd, strA );
    free( strA );
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
INT16 EnumProps16( HWND16 hwnd, PROPENUMPROC16 func )
{
    PROPERTY *prop, *next;
    WND *pWnd;
    INT16 ret = -1;

    dprintf_prop( stddeb, "EnumProps: %04x %08x\n", hwnd, (UINT32)func );
    if (!(pWnd = WIN_FindWndPtr( hwnd ))) return -1;
    for (prop = pWnd->pProp; (prop); prop = next)
    {
        /* Already get the next in case the callback */
        /* function removes the current property.    */
        next = prop->next;

        dprintf_prop( stddeb, "  Callback: handle=%08x str='%s'\n",
                      prop->handle, prop->string );
        ret = CallEnumPropProc16( (FARPROC16)func, hwnd,
                                  SEGPTR_GET(prop->string), prop->handle );
        if (!ret) break;
    }
    return ret;
}


/***********************************************************************
 *           EnumProps32A   (USER32.185)
 */
INT32 EnumProps32A( HWND32 hwnd, PROPENUMPROC32A func )
{
    PROPERTY *prop, *next;
    WND *pWnd;
    INT32 ret = -1;

    dprintf_prop( stddeb, "EnumProps32A: %04x %08x\n", hwnd, (UINT32)func );
    if (!(pWnd = WIN_FindWndPtr( hwnd ))) return -1;
    for (prop = pWnd->pProp; (prop); prop = next)
    {
        /* Already get the next in case the callback */
        /* function removes the current property.    */
        next = prop->next;

        dprintf_prop( stddeb, "  Callback: handle=%08x str='%s'\n",
                      prop->handle, prop->string );
        ret = CallEnumPropProc32( func, hwnd, prop->string, prop->handle );
        if (!ret) break;
    }
    return ret;
}


/***********************************************************************
 *           EnumProps32W   (USER32.188)
 */
INT32 EnumProps32W( HWND32 hwnd, PROPENUMPROC32W func )
{
    PROPERTY *prop, *next;
    WND *pWnd;
    INT32 ret = -1;

    dprintf_prop( stddeb, "EnumProps32W: %04x %08x\n", hwnd, (UINT32)func );
    if (!(pWnd = WIN_FindWndPtr( hwnd ))) return -1;
    for (prop = pWnd->pProp; (prop); prop = next)
    {
        /* Already get the next in case the callback */
        /* function removes the current property.    */
        next = prop->next;

        dprintf_prop( stddeb, "  Callback: handle=%08x str='%s'\n",
                      prop->handle, prop->string );
        if (HIWORD(prop->string))
        {
            LPWSTR str = STRING32_DupAnsiToUni( prop->string );
            ret = CallEnumPropProc32( func, hwnd, str, prop->handle );
            free( str );
        }
        else
            ret = CallEnumPropProc32( func, hwnd,
                                      (LPCWSTR)(UINT32)LOWORD(prop->string),
                                      prop->handle );
        if (!ret) break;
    }
    return ret;
}


/***********************************************************************
 *           EnumPropsEx32A   (USER32.186)
 */
INT32 EnumPropsEx32A( HWND32 hwnd, PROPENUMPROCEX32A func, LPARAM lParam )
{
    PROPERTY *prop, *next;
    WND *pWnd;
    INT32 ret = -1;

    dprintf_prop( stddeb, "EnumPropsEx32A: %04x %08x %08lx\n",
                  hwnd, (UINT32)func, lParam );
    if (!(pWnd = WIN_FindWndPtr( hwnd ))) return -1;
    for (prop = pWnd->pProp; (prop); prop = next)
    {
        /* Already get the next in case the callback */
        /* function removes the current property.    */
        next = prop->next;

        dprintf_prop( stddeb, "  Callback: handle=%08x str='%s'\n",
                      prop->handle, prop->string );
        ret = CallEnumPropProcEx32( func, hwnd, prop->string,
                                    prop->handle, lParam );
        if (!ret) break;
    }
    return ret;
}


/***********************************************************************
 *           EnumPropsEx32W   (USER32.187)
 */
INT32 EnumPropsEx32W( HWND32 hwnd, PROPENUMPROCEX32W func, LPARAM lParam )
{
    PROPERTY *prop, *next;
    WND *pWnd;
    INT32 ret = -1;

    dprintf_prop( stddeb, "EnumPropsEx32W: %04x %08x %08lx\n",
                  hwnd, (UINT32)func, lParam );
    if (!(pWnd = WIN_FindWndPtr( hwnd ))) return -1;
    for (prop = pWnd->pProp; (prop); prop = next)
    {
        /* Already get the next in case the callback */
        /* function removes the current property.    */
        next = prop->next;

        dprintf_prop( stddeb, "  Callback: handle=%08x str='%s'\n",
                      prop->handle, prop->string );
        if (HIWORD(prop->string))
        {
            LPWSTR str = STRING32_DupAnsiToUni( prop->string );
            ret = CallEnumPropProcEx32( func, hwnd, str, prop->handle, lParam);
            free( str );
        }
        else
            ret = CallEnumPropProcEx32( func, hwnd,
                                        (LPCWSTR)(UINT32)LOWORD(prop->string),
                                        prop->handle, lParam );
        if (!ret) break;
    }
    return ret;
}
