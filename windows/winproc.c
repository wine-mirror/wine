/*
 * Window procedure callbacks
 *
 * Copyright 1995 Martin von Loewis
 * Copyright 1996 Alexandre Julliard
 */

#include <stdio.h>
#include "windows.h"
#include "callback.h"
#include "heap.h"
#include "ldt.h"
#include "stackframe.h"
#include "string32.h"
#include "struct32.h"
#include "win.h"
#include "winproc.h"


typedef struct
{
    WNDPROC32       func;        /* 32-bit function, or 0 if free */
    unsigned int    count : 30;  /* Reference count, or next free if func==0 */
    WINDOWPROCTYPE  type : 2;    /* Function type */
} WINDOWPROC;

#define NB_WINPROCS    1024  /* Must be < 64K; 1024 should be enough for now */

static WINDOWPROC winProcs[NB_WINPROCS];
static int lastWinProc = 0;
static int freeWinProc = NB_WINPROCS;

/* Check if a win proc was created by WINPROC_AllocWinProc */
#define IS_ALLOCATED_WINPROC(func)  (HIWORD(func) == 0xffff)

/**********************************************************************
 *	     WINPROC_AllocWinProc
 *
 * Allocate a new window procedure.
 */
WNDPROC16 WINPROC_AllocWinProc( WNDPROC32 func, WINDOWPROCTYPE type )
{
    WINDOWPROC *proc;
    if (!func) return (WNDPROC16)0;  /* Null win proc remains null */
    if (IS_ALLOCATED_WINPROC(func))  /* Already allocated? */
    {
        if (LOWORD(func) >= NB_WINPROCS) return (WNDPROC16)0;
        proc = &winProcs[LOWORD(func)];
        if (!proc->func) return (WNDPROC16)0;
        proc->count++;
        return (WNDPROC16)func;
    }
    if (freeWinProc < NB_WINPROCS)  /* There is a free entry */
    {
        proc = &winProcs[freeWinProc];
        proc->func = func;
        func = (WNDPROC32)MAKELONG( freeWinProc, 0xffff );
        freeWinProc = proc->count;  /* Next free entry */
        proc->count = 1;
        proc->type  = type;
        return (WNDPROC16)func;
    }
    if (lastWinProc < NB_WINPROCS)  /* There's a free entry at the end */
    {
        proc = &winProcs[lastWinProc];
        proc->func = func;
        func = (WNDPROC32)MAKELONG( lastWinProc, 0xffff );
        lastWinProc++;
        proc->count = 1;
        proc->type  = type;
        return (WNDPROC16)func;
    }
    fprintf( stderr, "WINPROC_AllocWinProc: out of window procedures.\n"
                     "Please augment NB_WINPROCS in winproc.c\n" );
    return (WNDPROC16)0;
}


/**********************************************************************
 *	     WINPROC_GetWinProcType
 *
 * Return the type of a window procedure.
 */
WINDOWPROCTYPE WINPROC_GetWinProcType( WNDPROC16 func )
{
    WORD id = LOWORD(func);
    if (!IS_ALLOCATED_WINPROC(func)) return WIN_PROC_16;
    if ((id >= NB_WINPROCS) || !winProcs[id].func) return WIN_PROC_INVALID;
    return winProcs[id].type;
}


/**********************************************************************
 *	     WINPROC_GetWinProcFunc
 *
 * Return the 32-bit window procedure for a winproc.
 */
WNDPROC32 WINPROC_GetWinProcFunc( WNDPROC16 func )
{
    WORD id = LOWORD(func);
    if (!IS_ALLOCATED_WINPROC(func)) return NULL;
    if (id >= NB_WINPROCS) return NULL;
    return winProcs[id].func;
}


/**********************************************************************
 *	     WINPROC_FreeWinProc
 *
 * Free a window procedure.
 */
void WINPROC_FreeWinProc( WNDPROC16 func )
{
    WORD id = LOWORD(func);
    if (!IS_ALLOCATED_WINPROC(func)) return;
    if ((id >= NB_WINPROCS) || !winProcs[id].func)
    {
        fprintf( stderr, "WINPROC_FreeWinProc: invalid proc %08x\n",
                 (UINT32)func );
        return;
    }
    if (--winProcs[id].count == 0)
    {
        winProcs[id].func = 0;
        winProcs[id].count = freeWinProc;
        freeWinProc = id;
    }
}


/**********************************************************************
 *	     WINPROC_CallProc32ATo32W
 *
 * Call a window procedure, translating args from Ansi to Unicode.
 */
static LRESULT WINPROC_CallProc32ATo32W( WNDPROC32 func, HWND32 hwnd,
                                         UINT32 msg, WPARAM32 wParam,
                                         LPARAM lParam )
{
    LRESULT result;

    switch(msg)
    {
    case WM_GETTEXT:
        {
            LPWSTR str = (LPWSTR)HeapAlloc(SystemHeap,0,wParam*sizeof(WCHAR));
            if (!str) return 0;
            result = CallWndProc32( func, hwnd, msg, wParam, (LPARAM)str );
            STRING32_UniToAnsi( (LPSTR)lParam, str );
            HeapFree( SystemHeap, 0, str );
            return strlen( (LPSTR)lParam ) + 1;
        }

    case WM_SETTEXT:
        {
            LPWSTR str = STRING32_DupAnsiToUni( (LPCSTR)lParam );
            if (!str) return 0;
            result = CallWndProc32( func, hwnd, msg, wParam, (LPARAM)str );
            free( str );
        }
        return result;

    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCT32W cs = *(CREATESTRUCT32W *)lParam;
            cs.lpszName  = STRING32_DupAnsiToUni( (LPCSTR)cs.lpszName );
            cs.lpszClass = STRING32_DupAnsiToUni( (LPCSTR)cs.lpszName );
            result = CallWndProc32( func, hwnd, msg, wParam, (LPARAM)&cs );
            free( (LPVOID)cs.lpszName );
            free( (LPVOID)cs.lpszClass );
        }
        return result;
	
    default:  /* No translation needed */
        return CallWndProc32( func, hwnd, msg, wParam, lParam );
    }
}


/**********************************************************************
 *	     WINPROC_CallProc32WTo32A
 *
 * Call a window procedure, translating args from Unicode to Ansi.
 */
static LRESULT WINPROC_CallProc32WTo32A( WNDPROC32 func, HWND32 hwnd,
                                         UINT32 msg, WPARAM32 wParam,
                                         LPARAM lParam )
{
    LRESULT result;

    switch(msg)
    {
    case WM_GETTEXT:
        {
            LPSTR str = (LPSTR)HeapAlloc( SystemHeap, 0, wParam );
            if (!str) return 0;
            result = CallWndProc32( func, hwnd, msg, wParam, (LPARAM)str );
            STRING32_AnsiToUni( (LPWSTR)lParam, str );
            HeapFree( SystemHeap, 0, str );
            return STRING32_lstrlenW( (LPWSTR)lParam ) + 1;  /* FIXME? */
        }

    case WM_SETTEXT:
        {
            LPSTR str = STRING32_DupUniToAnsi( (LPCWSTR)lParam );
            if (!str) return 0;
            result = CallWndProc32( func, hwnd, msg, wParam, (LPARAM)str );
            free( str );
        }
        return result;

    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCT32A cs = *(CREATESTRUCT32A *)lParam;
            cs.lpszName  = STRING32_DupUniToAnsi( (LPCWSTR)cs.lpszName );
            cs.lpszClass = STRING32_DupUniToAnsi( (LPCWSTR)cs.lpszName );
            result = CallWndProc32( func, hwnd, msg, wParam, (LPARAM)&cs );
            free( (LPVOID)cs.lpszName );
            free( (LPVOID)cs.lpszClass );
        }
        return result;
	
    default:  /* No translation needed */
        return CallWndProc32( func, hwnd, msg, wParam, lParam );
    }
}


/**********************************************************************
 *	     WINPROC_CallProc16To32A
 *
 * Call a 32-bit window procedure, translating the 16-bit args.
 */
static LRESULT WINPROC_CallProc16To32A(WNDPROC32 func, HWND16 hwnd, UINT16 msg,
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
    case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT16*dis16 = (DRAWITEMSTRUCT16*)PTR_SEG_TO_LIN(lParam);
            DRAWITEMSTRUCT32 dis;
            dis.CtlType    = dis16->CtlType;
            dis.CtlID      = dis16->CtlID;
            dis.itemID     = dis16->itemID;
            dis.itemAction = dis16->itemAction;
            dis.itemState  = dis16->itemState;
            dis.hwndItem   = dis16->hwndItem;
            dis.hDC        = dis16->hDC;
            dis.itemData   = dis16->itemData;
            CONV_RECT16TO32( &dis16->rcItem, &dis.rcItem );
            result = CallWndProc32( func, hwnd, msg, wParam, (LPARAM)&dis );
            /* We don't bother to translate it back */
        }
        return result;

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
 *	     WINPROC_CallProc16To32W
 *
 * Call a 32-bit window procedure, translating the 16-bit args.
 */
static LRESULT WINPROC_CallProc16To32W(WNDPROC32 func, HWND16 hwnd, UINT16 msg,
                                       WPARAM16 wParam, LPARAM lParam )
{
    LRESULT result = 0;

    switch(msg)
    {
    case WM_GETTEXT:
    case WM_SETTEXT:
        return WINPROC_CallProc32ATo32W( func, hwnd, msg, wParam,
                                         (LPARAM)PTR_SEG_TO_LIN(lParam) );

    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCT16 *pcs16 = (CREATESTRUCT16 *)PTR_SEG_TO_LIN(lParam);
            CREATESTRUCT32A cs;

            STRUCT32_CREATESTRUCT16to32A( pcs16, &cs );
            cs.lpszName       = (LPCSTR)PTR_SEG_TO_LIN(pcs16->lpszName);
            cs.lpszClass      = (LPCSTR)PTR_SEG_TO_LIN(pcs16->lpszClass);
            result = WINPROC_CallProc32ATo32W( func, hwnd, msg, wParam,
                                               (LPARAM)&cs );
            pcs16 = (CREATESTRUCT16 *)PTR_SEG_TO_LIN(lParam);
            STRUCT32_CREATESTRUCT32Ato16( &cs, pcs16 );
        }
        return result;

    default:  /* No Unicode translation needed */
        return WINPROC_CallProc16To32A( func, hwnd, msg, wParam, lParam );
    }
}


/**********************************************************************
 *	     WINPROC_CallProc32ATo16
 *
 * Call a 16-bit window procedure, translating the 32-bit args.
 */
static LRESULT WINPROC_CallProc32ATo16( WNDPROC16 func, WORD ds, HWND32 hwnd,
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
    case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT32 *dis32 = (DRAWITEMSTRUCT32 *)lParam;
            DRAWITEMSTRUCT16 *dis = SEGPTR_NEW(DRAWITEMSTRUCT16);
            if (!dis) return 0;
            dis->CtlType    = (UINT16)dis32->CtlType;
            dis->CtlID      = (UINT16)dis32->CtlID;
            dis->itemID     = (UINT16)dis32->itemID;
            dis->itemAction = (UINT16)dis32->itemAction;
            dis->itemState  = (UINT16)dis32->itemState;
            dis->hwndItem   = (HWND16)dis32->hwndItem;
            dis->hDC        = (HDC16)dis32->hDC;
            dis->itemData   = dis32->itemData;
            CONV_RECT32TO16( &dis32->rcItem, &dis->rcItem );
            result = CallWndProc16( func, ds, hwnd, msg, (WPARAM16)wParam,
                                    (LPARAM)SEGPTR_GET(dis) );
            /* We don't bother to translate it back */
            SEGPTR_FREE(dis);
        }
        return result;

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
 *	     WINPROC_CallProc32WTo16
 *
 * Call a 16-bit window procedure, translating the 32-bit args.
 */
static LRESULT WINPROC_CallProc32WTo16( WNDPROC16 func, WORD ds, HWND32 hwnd,
                                        UINT32 msg, WPARAM32 wParam,
                                        LPARAM lParam )
{
    LRESULT result;

    switch(msg)
    {
    case WM_GETTEXT:
        {
            LPSTR str;
            wParam = MIN( wParam, 0xff80 );  /* Size must be < 64K */
            if (!(str = SEGPTR_ALLOC( wParam ))) return 0;
            result = CallWndProc16( func, ds, hwnd, msg, (WPARAM16)wParam,
                                    (LPARAM)SEGPTR_GET(str) );
            if (result > 0) STRING32_AnsiToUni( (LPWSTR)lParam, str );
            SEGPTR_FREE(str);
        }
        return result;

    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCT32A cs = *(CREATESTRUCT32A *)lParam;
            cs.lpszName  = STRING32_DupUniToAnsi( (LPCWSTR)cs.lpszName );
            cs.lpszClass = STRING32_DupUniToAnsi( (LPCWSTR)cs.lpszName );
            result = WINPROC_CallProc32ATo16( func, ds, hwnd, msg, wParam,
                                              (LPARAM)&cs );
            free( (LPVOID)cs.lpszName );
            free( (LPVOID)cs.lpszClass );
        }
        return result;
	
    case WM_SETTEXT:
        {
            LPSTR str = SEGPTR_ALLOC( STRING32_lstrlenW( (LPWSTR)lParam ) );
            if (!str) return 0;
            STRING32_UniToAnsi( str, (LPWSTR)lParam );
            result = CallWndProc16( func, ds, hwnd, msg, (WPARAM16)wParam,
                                    (LPARAM)SEGPTR_GET(str) );
            SEGPTR_FREE(str);
        }
        return result;

    default:  /* No Unicode translation needed */
        return WINPROC_CallProc32ATo16( func, ds, hwnd, msg, wParam, lParam );
    }
}


/**********************************************************************
 *	     CallWindowProc16    (USER.122)
 */
LRESULT CallWindowProc16( WNDPROC16 func, HWND16 hwnd, UINT16 msg,
                          WPARAM16 wParam, LPARAM lParam )
{
    WND *wndPtr;

    switch(WINPROC_GetWinProcType(func))
    {
    case WIN_PROC_16:
        wndPtr = WIN_FindWndPtr( hwnd );
        return CallWndProc16( (FARPROC)func,
                              wndPtr ? wndPtr->hInstance : CURRENT_DS,
                              hwnd, msg, wParam, lParam );
    case WIN_PROC_32A:
        return WINPROC_CallProc16To32A( WINPROC_GetWinProcFunc(func),
                                        hwnd, msg, wParam, lParam );
    case WIN_PROC_32W:
        return WINPROC_CallProc16To32W( WINPROC_GetWinProcFunc(func),
                                        hwnd, msg, wParam, lParam );
    default:
        fprintf(stderr, "CallWindowProc16: invalid func %08x\n", (UINT32)func);
        return 0;
    }
}


/**********************************************************************
 *	     CallWindowProc32A    (USER32.17)
 */
LRESULT CallWindowProc32A( WNDPROC32 func, HWND32 hwnd, UINT32 msg,
                           WPARAM32 wParam, LPARAM lParam )
{
    WND *wndPtr;

    switch(WINPROC_GetWinProcType( (WNDPROC16)func ))
    {
    case WIN_PROC_16:
        wndPtr = WIN_FindWndPtr( hwnd );
        return WINPROC_CallProc32ATo16( (FARPROC)func,
                                       wndPtr ? wndPtr->hInstance : CURRENT_DS,
                                       hwnd, msg, wParam, lParam );
    case WIN_PROC_32A:
        return CallWndProc32( WINPROC_GetWinProcFunc( (WNDPROC16)func ),
                              hwnd, msg, wParam, lParam );
    case WIN_PROC_32W:
        return WINPROC_CallProc32ATo32W(WINPROC_GetWinProcFunc((WNDPROC16)func),
                                        hwnd, msg, wParam, lParam );
    default:
        fprintf(stderr,"CallWindowProc32A: invalid func %08x\n",(UINT32)func);
        return 0;
    }
}


/**********************************************************************
 *	     CallWindowProc32W    (USER32.18)
 */
LRESULT CallWindowProc32W( WNDPROC32 func, HWND32 hwnd, UINT32 msg,
                           WPARAM32 wParam, LPARAM lParam )
{
    WND *wndPtr;

    switch(WINPROC_GetWinProcType( (WNDPROC16)func ))
    {
    case WIN_PROC_16:
        wndPtr = WIN_FindWndPtr( hwnd );
        return WINPROC_CallProc32WTo16( (FARPROC)func,
                                       wndPtr ? wndPtr->hInstance : CURRENT_DS,
                                       hwnd, msg, wParam, lParam );
    case WIN_PROC_32A:
        return WINPROC_CallProc32WTo32A(WINPROC_GetWinProcFunc((WNDPROC16)func),
                                        hwnd, msg, wParam, lParam );
    case WIN_PROC_32W:
        return CallWndProc32( WINPROC_GetWinProcFunc( (WNDPROC16)func ),
                              hwnd, msg, wParam, lParam );
    default:
        fprintf(stderr,"CallWindowProc32W: invalid func %08x\n",(UINT32)func);
        return 0;
    }
}
