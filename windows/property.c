/*
 * Window properties
 *
 * Copyright 1995 Alexandre Julliard
 */

#include <string.h>
#include "win.h"
#include "user.h"
#include "callback.h"
#include "stddebug.h"
/* #define DEBUG_PROP */
#include "debug.h"


typedef struct
{
    HANDLE next;       /* Next property in window list */
    ATOM   atom;       /* Atom (or 0 if string) */
    HANDLE hData;      /* User's data */
    char   string[1];  /* Property string */
} PROPERTY;


/***********************************************************************
 *           SetProp   (USER.26)
 */
BOOL SetProp( HWND hwnd, SEGPTR str, HANDLE hData )
{
    HANDLE hProp;
    PROPERTY *prop;
    WND *wndPtr;

    dprintf_prop( stddeb, "SetProp: "NPFMT" %08lx "NPFMT"\n", hwnd, str, hData );
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;
    hProp = USER_HEAP_ALLOC( sizeof(PROPERTY) + 
                             (HIWORD(str) ? strlen(PTR_SEG_TO_LIN(str)) : 0 ));
    if (!hProp) return FALSE;
    prop = (PROPERTY *) USER_HEAP_LIN_ADDR( hProp );
    if (HIWORD(str))  /* string */
    {
        prop->atom = 0;
        strcpy( prop->string, PTR_SEG_TO_LIN(str) );
    }
    else  /* atom */
    {
        prop->atom = LOWORD(str);
        prop->string[0] = '\0';
    }
    prop->hData = hData;
    prop->next = wndPtr->hProp;
    wndPtr->hProp = hProp;
    return TRUE;
}


/***********************************************************************
 *           GetProp   (USER.25)
 */
HANDLE GetProp( HWND hwnd, SEGPTR str )
{
    HANDLE hProp;
    WND *wndPtr;

    dprintf_prop( stddeb, "GetProp: "NPFMT" %08lx\n", hwnd, str );
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
    hProp = wndPtr->hProp;
    while (hProp)
    {
        PROPERTY *prop = (PROPERTY *)USER_HEAP_LIN_ADDR(hProp);
        if (HIWORD(str))
        {
            if (!prop->atom && !strcasecmp( prop->string, PTR_SEG_TO_LIN(str)))
                return prop->hData;
        }
        else if (prop->atom && (prop->atom == LOWORD(str))) return prop->hData;
        hProp = prop->next;
    }
    return 0;
}


/***********************************************************************
 *           RemoveProp   (USER.24)
 */
HANDLE RemoveProp( HWND hwnd, SEGPTR str )
{
    HANDLE *hProp;
    WND *wndPtr;

    dprintf_prop( stddeb, "RemoveProp: "NPFMT" %08lx\n", hwnd, str );
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
    hProp = &wndPtr->hProp;
    while (*hProp)
    {
        PROPERTY *prop = (PROPERTY *)USER_HEAP_LIN_ADDR( *hProp );
        if ((HIWORD(str) && !prop->atom &&
             !strcasecmp( prop->string, PTR_SEG_TO_LIN(str))) ||
            (!HIWORD(str) && prop->atom && (prop->atom == LOWORD(str))))
        {
            HANDLE hNext = prop->next;
            HANDLE hData = prop->hData;
            USER_HEAP_FREE( *hProp );
            *hProp = hNext;
            return hData;
        }
        hProp = &prop->next;
    }
    return 0;
}


/***********************************************************************
 *           EnumProps   (USER.27)
 */
int EnumProps( HWND hwnd, FARPROC func )
{
    int ret = -1;
    HANDLE hProp;
    WND *wndPtr;

    dprintf_prop( stddeb, "EnumProps: "NPFMT" %08lx\n", hwnd, (LONG)func );
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
    hProp = wndPtr->hProp;
    while (hProp)
    {
        PROPERTY *prop = (PROPERTY *)USER_HEAP_LIN_ADDR(hProp);
        
        dprintf_prop( stddeb, "  Callback: atom=%04x data="NPFMT" str='%s'\n",
                      prop->atom, prop->hData, prop->string );

          /* Already get the next in case the callback */
          /* function removes the current property.    */
        hProp = prop->next;
        ret = CallEnumPropProc( func, hwnd,
                                prop->atom ? 
				  (LONG)MAKELONG( prop->atom, 0 )
				:
                                  (LONG)(USER_HEAP_SEG_ADDR(hProp) +
                                         ((int)prop->string - (int)prop)),
                                prop->hData );
        if (!ret) break;
    }
    return ret;
}
