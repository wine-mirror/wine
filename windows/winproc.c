/*
 * Window procedure callbacks
 *
 * Copyright 1995 Martin von Loewis
 * Copyright 1996 Alexandre Julliard
 */

#include <stdio.h>
#include "windows.h"
#include "alias.h"
#include "callback.h"
#include "heap.h"
#include "ldt.h"
#include "stackframe.h"
#include "struct32.h"
#include "win.h"


/**********************************************************************
 *	     WINPROC_CallProc16To32
 *
 * Call a 32-bit window procedure, translating the 16-bit args.
 */
static LRESULT WINPROC_CallProc16To32( WNDPROC32 func, HWND16 hwnd, UINT16 msg,
                                       WPARAM16 wParam, LPARAM lParam )
{
    LRESULT result;

    switch(msg)
    {
    case WM_ACTIVATE:
    case WM_CHARTOITEM:
    case WM_COMMAND:
    case WM_HSCROLL:
    case WM_VKEYTOITEM:
    case WM_VSCROLL:
        return CallWndProc32( func, hwnd, msg,
                              MAKEWPARAM( wParam, HIWORD(lParam) ),
                              (LPARAM)(HWND32)LOWORD(lParam) );
    case WM_CTLCOLOR:
        return CallWndProc32( func, hwnd, WM_CTLCOLORMSGBOX + HIWORD(lParam),
                              (WPARAM32)(HDC32)wParam,
                              (LPARAM)(HWND32)LOWORD(lParam) );
    case WM_GETMINMAXINFO:
        {
            MINMAXINFO32 mmi;
            STRUCT32_MINMAXINFO16to32( (MINMAXINFO16 *)PTR_SEG_TO_LIN(lParam),
                                       &mmi );
            result = CallWndProc32( func, hwnd, msg, wParam, (LPARAM)&mmi);
            STRUCT32_MINMAXINFO32to16( &mmi,
                                       (MINMAXINFO16 *)PTR_SEG_TO_LIN(lParam));
        }
        return result;

    case WM_GETTEXT:
        /* FIXME: Unicode */
        return CallWndProc32( func, hwnd, msg, wParam,
                              (LPARAM)PTR_SEG_TO_LIN(lParam) );
    case WM_MDISETMENU:
        return CallWndProc32( func, hwnd, msg,
                              (WPARAM32)(HMENU32)LOWORD(lParam),
                              (LPARAM)(HMENU32)HIWORD(lParam) );
    case WM_MENUCHAR:
    case WM_MENUSELECT:
        return CallWndProc32( func, hwnd, msg,
                              MAKEWPARAM( wParam, LOWORD(lParam) ),
                              (LPARAM)(HMENU32)HIWORD(lParam) );
    case WM_NCCALCSIZE:
        {
            NCCALCSIZE_PARAMS16 *pnc16;
            NCCALCSIZE_PARAMS32 nc;
            WINDOWPOS32 wp;

            pnc16 = (NCCALCSIZE_PARAMS16 *)PTR_SEG_TO_LIN(lParam);
            CONV_RECT16TO32( &pnc16->rgrc[0], &nc.rgrc[0] );
            if (wParam)
            {
                CONV_RECT16TO32( &pnc16->rgrc[1], &nc.rgrc[1] );
                CONV_RECT16TO32( &pnc16->rgrc[2], &nc.rgrc[2] );
                STRUCT32_WINDOWPOS16to32( (WINDOWPOS16 *)PTR_SEG_TO_LIN(pnc16->lppos),
                                          &wp );
                nc.lppos = &wp;
            }

            result = CallWndProc32( func, hwnd, msg, wParam, (LPARAM)&nc );

            pnc16 = (NCCALCSIZE_PARAMS16 *)PTR_SEG_TO_LIN(lParam);
            CONV_RECT32TO16( &nc.rgrc[0], &pnc16->rgrc[0] );
            if (wParam)
            {
                CONV_RECT32TO16( &nc.rgrc[1], &pnc16->rgrc[1] );
                CONV_RECT32TO16( &nc.rgrc[2], &pnc16->rgrc[2] );
                STRUCT32_WINDOWPOS32to16( nc.lppos,
                                  (WINDOWPOS16 *)PTR_SEG_TO_LIN(pnc16->lppos));
            }
        }
        return result;
	
    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCT16 *pcs16 = (CREATESTRUCT16 *)PTR_SEG_TO_LIN(lParam);
            CREATESTRUCT32A cs;

            STRUCT32_CREATESTRUCT16to32A( pcs16, &cs );
            /* FIXME: Unicode */
            cs.lpszName       = (LPCSTR)PTR_SEG_TO_LIN(pcs16->lpszName);
            cs.lpszClass      = (LPCSTR)PTR_SEG_TO_LIN(pcs16->lpszClass);
            result = CallWndProc32( func, hwnd, msg, wParam, (LPARAM)&cs );
            pcs16 = (CREATESTRUCT16 *)PTR_SEG_TO_LIN(lParam);
            STRUCT32_CREATESTRUCT32Ato16( &cs, pcs16 );

            if (cs.lpszName != (LPCSTR)PTR_SEG_TO_LIN(pcs16->lpszName))
                fprintf( stderr, "CallWindowProc16: WM_NCCREATE(%04x) changed lpszName (%p->%p), please report.\n",
                    msg, (LPCSTR)PTR_SEG_TO_LIN(pcs16->lpszName), cs.lpszName);
            if (cs.lpszClass != (LPCSTR)PTR_SEG_TO_LIN(pcs16->lpszClass))
                fprintf( stderr, "CallWindowProc16: WM_NCCREATE(%04x) changed lpszClass (%p->%p), please report.\n",
                  msg, (LPCSTR)PTR_SEG_TO_LIN(pcs16->lpszClass), cs.lpszClass);
        }
        return result;

    case WM_PARENTNOTIFY:
        if ((wParam == WM_CREATE) || (wParam == WM_DESTROY))
            return CallWndProc32( func, hwnd, msg,
                                  MAKEWPARAM( wParam, HIWORD(lParam) ),
                                  (LPARAM)(HWND32)LOWORD(lParam) );
        else return CallWndProc32( func, hwnd, msg,
                                   MAKEWPARAM( wParam, 0  /* FIXME? */ ),
                                   lParam );
    case WM_SETTEXT:
        /* FIXME: Unicode */
        return CallWndProc32( func, hwnd, msg, wParam,
                              (LPARAM)PTR_SEG_TO_LIN(lParam) );

    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        {
            WINDOWPOS32 wp;
            STRUCT32_WINDOWPOS16to32( (WINDOWPOS16 *)PTR_SEG_TO_LIN(lParam),
                                      &wp );
            result = CallWndProc32( func, hwnd, msg, wParam, (LPARAM)&wp );
            STRUCT32_WINDOWPOS32to16( &wp,
                                      (WINDOWPOS16 *)PTR_SEG_TO_LIN(lParam) );
        }
        return result;

    case WM_ASKCBFORMATNAME:
    case WM_COMPAREITEM:
    case WM_DELETEITEM:
    case WM_DEVMODECHANGE:
    case WM_DRAWITEM:
    case WM_MDIACTIVATE:
    case WM_MDICREATE:
    case WM_MEASUREITEM:
    case WM_PAINTCLIPBOARD:
    case WM_SIZECLIPBOARD:
    case WM_WININICHANGE:
        fprintf( stderr, "CallWindowProc16To32: message %04x needs translation\n", msg );

    default:  /* No translation needed */
        return CallWndProc32( func, hwnd, msg, (WPARAM32)wParam, lParam );
    }
}


/**********************************************************************
 *	     WINPROC_CallProc32To16
 *
 * Call a 16-bit window procedure, translating the 32-bit args.
 */
static LRESULT WINPROC_CallProc32To16( WNDPROC16 func, WORD ds, HWND32 hwnd,
                                       UINT32 msg, WPARAM32 wParam,
                                       LPARAM lParam )
{
    LRESULT result;

    switch(msg)
    {
    case WM_ACTIVATE:
    case WM_CHARTOITEM:
    case WM_COMMAND:
    case WM_HSCROLL:
    case WM_VKEYTOITEM:
    case WM_VSCROLL:
        return CallWndProc16( func, ds, hwnd, msg, (WPARAM16)wParam,
                              MAKELPARAM( (HWND16)lParam, HIWORD(wParam) ) );
        
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        return CallWndProc16( func, ds, hwnd, WM_CTLCOLOR, (WPARAM16)wParam,
                              MAKELPARAM( (HWND16)lParam,
                                          (WORD)msg - WM_CTLCOLORMSGBOX ) );
    case WM_GETMINMAXINFO:
        {
            MINMAXINFO16 *mmi = SEGPTR_NEW(MINMAXINFO16);
            if (!mmi) return 0;
            STRUCT32_MINMAXINFO32to16( (MINMAXINFO32 *)lParam, mmi );
            result = CallWndProc16( func, ds, hwnd, msg, (WPARAM16)wParam,
                                    (LPARAM)SEGPTR_GET(mmi) );
            STRUCT32_MINMAXINFO16to32( mmi, (MINMAXINFO32 *)lParam );
            SEGPTR_FREE(mmi);
        }
        return result;

    case WM_GETTEXT:
        {
            LPSTR str;
            wParam = MIN( wParam, 0xff80 );  /* Size must be < 64K */
            if (!(str = SEGPTR_ALLOC( wParam ))) return 0;
            result = CallWndProc16( func, ds, hwnd, msg, (WPARAM16)wParam,
                                    (LPARAM)SEGPTR_GET(str) );
            if (result > 0) memcpy( (LPSTR)lParam, str, result );
            SEGPTR_FREE(str);
        }
        return result;

    case WM_MDISETMENU:
        return CallWndProc16( func, ds, hwnd, msg, TRUE /* FIXME? */,
                              MAKELPARAM( (HMENU16)LOWORD(lParam),
                                          (HMENU16)HIWORD(lParam) ) );
    case WM_MENUCHAR:
    case WM_MENUSELECT:
        return CallWndProc16( func, ds, hwnd, msg, (WPARAM16)LOWORD(wParam),
                              MAKELPARAM( HIWORD(wParam), (HMENU16)lParam ) );
    case WM_NCCALCSIZE:
        {
            NCCALCSIZE_PARAMS32 *nc32 = (NCCALCSIZE_PARAMS32 *)lParam;
            NCCALCSIZE_PARAMS16 *nc;
            WINDOWPOS16 *wp = NULL;

            if (!(nc = SEGPTR_NEW(NCCALCSIZE_PARAMS16))) return 0;
            CONV_RECT32TO16( &nc32->rgrc[0], &nc->rgrc[0] );
            if (wParam)
            {
                CONV_RECT32TO16( &nc32->rgrc[1], &nc->rgrc[1] );
                CONV_RECT32TO16( &nc32->rgrc[2], &nc->rgrc[2] );
                if (!(wp = SEGPTR_NEW(WINDOWPOS16)))
                {
                    SEGPTR_FREE(nc);
                    return 0;
                }
                STRUCT32_WINDOWPOS32to16( nc32->lppos, wp );
                nc->lppos = SEGPTR_GET(wp);
            }
            result = CallWndProc16( func, ds, hwnd, msg, (WPARAM16)wParam,
                                    (LPARAM)SEGPTR_GET(nc) );
            CONV_RECT16TO32( &nc->rgrc[0], &nc32->rgrc[0] );
            if (wParam)
            {
                CONV_RECT16TO32( &nc->rgrc[1], &nc32->rgrc[1] );
                CONV_RECT16TO32( &nc->rgrc[2], &nc32->rgrc[2] );
                STRUCT32_WINDOWPOS16to32( (WINDOWPOS16 *)PTR_SEG_TO_LIN(nc->lppos),
                                          nc32->lppos );
                SEGPTR_FREE(wp);
            }
            SEGPTR_FREE(nc);
        }
        return result;

    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCT16 *cs;
            CREATESTRUCT32A *cs32 = (CREATESTRUCT32A *)lParam;
            LPSTR name, cls;

            if (!(cs = SEGPTR_NEW(CREATESTRUCT16))) return 0;
            STRUCT32_CREATESTRUCT32Ato16( cs32, cs );
            name = SEGPTR_STRDUP( cs32->lpszName );
            cls  = SEGPTR_STRDUP( cs32->lpszClass );
            cs->lpszName  = SEGPTR_GET(name);
            cs->lpszClass = SEGPTR_GET(cls);
            result = CallWndProc16( func, ds, hwnd, msg, (WPARAM16)wParam,
                                    (LPARAM)SEGPTR_GET(cs) );
            STRUCT32_CREATESTRUCT16to32A( cs, cs32 );
            if (PTR_SEG_TO_LIN(cs->lpszName) != name)
                cs32->lpszName  = (LPCSTR)PTR_SEG_TO_LIN( cs->lpszName );
            if (PTR_SEG_TO_LIN(cs->lpszClass) != cls)
                cs32->lpszClass = (LPCSTR)PTR_SEG_TO_LIN( cs->lpszClass );
            SEGPTR_FREE(name);
            SEGPTR_FREE(cls);
            SEGPTR_FREE(cs);
        }
        return result;
	
    case WM_PARENTNOTIFY:
        if ((wParam == WM_CREATE) || (wParam == WM_DESTROY))
            return CallWndProc16( func, ds, hwnd, msg, (WPARAM16)wParam,
                                  MAKELPARAM( (HWND16)lParam, LOWORD(wParam)));
    case WM_SETTEXT:
        {
            LPSTR str = SEGPTR_STRDUP( (LPSTR)lParam );
            if (!str) return 0;
            result = CallWndProc16( func, ds, hwnd, msg, (WPARAM16)wParam,
                                    (LPARAM)SEGPTR_GET(str) );
            SEGPTR_FREE(str);
        }
        return result;

    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        {
            WINDOWPOS16 *wp;
            if (!(wp = SEGPTR_NEW(WINDOWPOS16))) return 0;
            STRUCT32_WINDOWPOS32to16( (WINDOWPOS32 *)lParam, wp );
            result = CallWndProc16( func, ds, hwnd, msg, (WPARAM16)wParam,
                                    (LPARAM)SEGPTR_GET(wp) );
            STRUCT32_WINDOWPOS16to32( wp, (WINDOWPOS32 *)lParam );
            SEGPTR_FREE(wp);
        }
        return result;

    case WM_ASKCBFORMATNAME:
    case WM_COMPAREITEM:
    case WM_DELETEITEM:
    case WM_DEVMODECHANGE:
    case WM_DRAWITEM:
    case WM_MDIACTIVATE:
    case WM_MDICREATE:
    case WM_MEASUREITEM:
    case WM_PAINTCLIPBOARD:
    case WM_SIZECLIPBOARD:
    case WM_WININICHANGE:
        fprintf( stderr, "CallWindowProc32To16: message %04x needs translation\n", msg );

    default:  /* No translation needed */
        return CallWndProc16( func, ds, hwnd, msg, (WPARAM16)wParam, lParam );
    }
}


/**********************************************************************
 *	     CallWindowProc16    (USER.122)
 */
LRESULT CallWindowProc16( WNDPROC16 func, HWND16 hwnd, UINT16 msg,
                          WPARAM16 wParam, LPARAM lParam )
{
    FUNCTIONALIAS *a;

    /* check if we have something better than 16 bit relays */
    if (!ALIAS_UseAliases || !(a=ALIAS_LookupAlias((DWORD)func)) ||
        (!a->wine && !a->win32))
    {
        WND *wndPtr = WIN_FindWndPtr( hwnd );
        return CallWndProc16( (FARPROC)func,
                              wndPtr ? wndPtr->hInstance : CURRENT_DS,
                              hwnd, msg, wParam, lParam );
    }

    if(a->wine)
        return ((LONG(*)(WORD,WORD,WORD,LONG))(a->wine))
                            (hwnd,msg,wParam,lParam);

    return WINPROC_CallProc16To32( (WNDPROC32)a->win32, hwnd, msg,
                                   wParam, lParam );
}


/**********************************************************************
 *	     CallWindowProc32A    (USER32.17)
 */
LRESULT CallWindowProc32A( WNDPROC32 func, HWND32 hwnd, UINT32 msg,
                           WPARAM32 wParam, LPARAM lParam )
{
    FUNCTIONALIAS *a = NULL;
    WND *wndPtr;
    WORD ds;

    /* check if we have something better than 16 bit relays */
    if (ALIAS_UseAliases && (a=ALIAS_LookupAlias((DWORD)func)) &&
        (a->wine || a->win32))
    {
        /* FIXME: Unicode */
        if (a->wine)
            return ((WNDPROC32)a->wine)(hwnd,msg,wParam,lParam);
        else return CallWndProc32( func, hwnd, msg, wParam, lParam );
    }
    wndPtr = WIN_FindWndPtr( hwnd );
    ds = wndPtr ? wndPtr->hInstance : CURRENT_DS;

    if (!a)
    {
        fprintf( stderr, "CallWindowProc32A: no alias for %p\n", func );
        return 0;
    }

    return WINPROC_CallProc32To16( (WNDPROC16)a->win16, ds, hwnd, msg,
                                   wParam, lParam );
}


/**********************************************************************
 *	     CallWindowProc32W    (USER32.18)
 */
LRESULT CallWindowProc32W( WNDPROC32 func, HWND32 hwnd, UINT32 msg,
                           WPARAM32 wParam, LPARAM lParam )
{
    /* FIXME: Unicode translation */
    return CallWindowProc32A( func, hwnd, msg, wParam, lParam );
}
