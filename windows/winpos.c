/*
 * Window position related functions.
 *
 * Copyright 1993, 1994, 1995 Alexandre Julliard
 *                       1995,1996 Alex Korobka
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include "sysmetrics.h"
#include "heap.h"
#include "module.h"
#include "user.h"
#include "win.h"
#include "hook.h"
#include "message.h"
#include "queue.h"
#include "options.h"
#include "winpos.h"
#include "dce.h"
#include "nonclient.h"
#include "stddebug.h"
/* #define DEBUG_WIN */
#include "debug.h"

#define  SWP_AGG_NOGEOMETRYCHANGE \
    (SWP_NOSIZE | SWP_NOMOVE | SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE)
#define  SWP_AGG_NOPOSCHANGE \
    (SWP_AGG_NOGEOMETRYCHANGE | SWP_NOZORDER)
#define  SWP_AGG_STATUSFLAGS \
    (SWP_AGG_NOPOSCHANGE | SWP_FRAMECHANGED | SWP_HIDEWINDOW | SWP_SHOWWINDOW)

#define SMC_NOCOPY		0x0001
#define SMC_NOPARENTERASE	0x0002
#define SMC_DRAWFRAME		0x0004
#define SMC_SETXPOS		0x0008

/* ----- external functions ----- */

extern void 	FOCUS_SwitchFocus( HWND32 , HWND32 );
extern HRGN32 	DCE_GetVisRgn( HWND32, WORD );
extern HWND32	CARET_GetHwnd();
extern BOOL32   DCE_InvalidateDCE(WND*, RECT16* );

/* ----- internal variables ----- */

static HWND32 hwndActive      = 0;  /* Currently active window */
static HWND32 hwndPrevActive  = 0;  /* Previously active window */

extern MESSAGEQUEUE* pActiveQueue;

/***********************************************************************
 *           WINPOS_CheckActive
 */
void WINPOS_CheckActive( HWND32 hwnd )
{
  if( hwnd == hwndPrevActive ) hwndPrevActive = 0;
  if( hwnd == hwndActive )
  {
      hwndActive = 0; 
      dprintf_win(stddeb,"\tattempt to activate destroyed window!\n");
  }
}

/***********************************************************************
 *           WINPOS_FindIconPos
 *
 * Find a suitable place for an iconic window.
 * The new position is stored into wndPtr->ptIconPos.
 */
void WINPOS_FindIconPos( HWND32 hwnd )
{
    RECT16 rectParent;
    short x, y, xspacing, yspacing;
    WND * wndPtr = WIN_FindWndPtr( hwnd );

    if (!wndPtr || !wndPtr->parent) return;
    GetClientRect16( wndPtr->parent->hwndSelf, &rectParent );
    if ((wndPtr->ptIconPos.x >= rectParent.left) &&
        (wndPtr->ptIconPos.x + SYSMETRICS_CXICON < rectParent.right) &&
        (wndPtr->ptIconPos.y >= rectParent.top) &&
        (wndPtr->ptIconPos.y + SYSMETRICS_CYICON < rectParent.bottom))
        return;  /* The icon already has a suitable position */

    xspacing = yspacing = 70;  /* FIXME: This should come from WIN.INI */
    y = rectParent.bottom;
    for (;;)
    {
        for (x = rectParent.left; x<=rectParent.right-xspacing; x += xspacing)
        {
              /* Check if another icon already occupies this spot */
            WND *childPtr = wndPtr->parent->child;
            while (childPtr)
            {
                if ((childPtr->dwStyle & WS_MINIMIZE) && (childPtr != wndPtr))
                {
                    if ((childPtr->rectWindow.left < x + xspacing) &&
                        (childPtr->rectWindow.right >= x) &&
                        (childPtr->rectWindow.top <= y) &&
                        (childPtr->rectWindow.bottom > y - yspacing))
                        break;  /* There's a window in there */
                }
                childPtr = childPtr->next;
            }
            if (!childPtr)
            {
                  /* No window was found, so it's OK for us */
                wndPtr->ptIconPos.x = x + (xspacing - SYSMETRICS_CXICON) / 2;
                wndPtr->ptIconPos.y = y - (yspacing + SYSMETRICS_CYICON) / 2;
                return;
            }
        }
        y -= yspacing;
    }
}


/***********************************************************************
 *           ArrangeIconicWindows   (USER32.6)
 */
UINT16 ArrangeIconicWindows16( HWND16 parent) {
    return ArrangeIconicWindows32(parent);
}
/***********************************************************************
 *           ArrangeIconicWindows   (USER.170)
 */
UINT32 ArrangeIconicWindows32( HWND32 parent )
{
    RECT32 rectParent;
    HWND32 hwndChild;
    INT32 x, y, xspacing, yspacing;

    GetClientRect32( parent, &rectParent );
    x = rectParent.left;
    y = rectParent.bottom;
    xspacing = yspacing = 70;  /* FIXME: This should come from WIN.INI */
    hwndChild = GetWindow32( parent, GW_CHILD );
    while (hwndChild)
    {
        if (IsIconic32( hwndChild ))
        {
            SetWindowPos32( hwndChild, 0, x + (xspacing - SYSMETRICS_CXICON) / 2,
                            y - (yspacing + SYSMETRICS_CYICON) / 2, 0, 0,
                            SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE );
            if (x <= rectParent.right - xspacing) x += xspacing;
            else
            {
                x = rectParent.left;
                y -= yspacing;
            }
        }
        hwndChild = GetWindow32( hwndChild, GW_HWNDNEXT );
    }
    return yspacing;
}


/***********************************************************************
 *           GetWindowRect16   (USER.32)
 */
void GetWindowRect16( HWND16 hwnd, LPRECT16 rect ) 
{
    WND * wndPtr = WIN_FindWndPtr( hwnd ); 
    if (!wndPtr) return;
    
    *rect = wndPtr->rectWindow;
    if (wndPtr->dwStyle & WS_CHILD)
	MapWindowPoints16( wndPtr->parent->hwndSelf, 0, (POINT16 *)rect, 2 );
}


/***********************************************************************
 *           GetWindowRect32   (USER.32)
 */
void GetWindowRect32( HWND32 hwnd, LPRECT32 rect ) 
{
    WND * wndPtr = WIN_FindWndPtr( hwnd ); 
    if (!wndPtr) return;
    
    CONV_RECT16TO32( &wndPtr->rectWindow, rect );
    if (wndPtr->dwStyle & WS_CHILD)
	MapWindowPoints32( wndPtr->parent->hwndSelf, 0, (POINT32 *)rect, 2 );
}


/***********************************************************************
 *           GetClientRect16   (USER.33)
 */
void GetClientRect16( HWND16 hwnd, LPRECT16 rect ) 
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );

    rect->left = rect->top = rect->right = rect->bottom = 0;
    if (wndPtr) 
    {
	rect->right  = wndPtr->rectClient.right - wndPtr->rectClient.left;
	rect->bottom = wndPtr->rectClient.bottom - wndPtr->rectClient.top;
    }
}


/***********************************************************************
 *           GetClientRect32   (USER32.219)
 */
void GetClientRect32( HWND32 hwnd, LPRECT32 rect ) 
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );

    rect->left = rect->top = rect->right = rect->bottom = 0;
    if (wndPtr) 
    {
	rect->right  = wndPtr->rectClient.right - wndPtr->rectClient.left;
	rect->bottom = wndPtr->rectClient.bottom - wndPtr->rectClient.top;
    }
}


/*******************************************************************
 *         ClientToScreen16   (USER.28)
 */
BOOL16 ClientToScreen16( HWND16 hwnd, LPPOINT16 lppnt )
{
    MapWindowPoints16( hwnd, 0, lppnt, 1 );
    return TRUE;
}


/*******************************************************************
 *         ClientToScreen32   (USER32.51)
 */
BOOL32 ClientToScreen32( HWND32 hwnd, LPPOINT32 lppnt )
{
    MapWindowPoints32( hwnd, 0, lppnt, 1 );
    return TRUE;
}


/*******************************************************************
 *         ScreenToClient16   (USER.29)
 */
void ScreenToClient16( HWND16 hwnd, LPPOINT16 lppnt )
{
    MapWindowPoints16( 0, hwnd, lppnt, 1 );
}


/*******************************************************************
 *         ScreenToClient32   (USER32.446)
 */
void ScreenToClient32( HWND32 hwnd, LPPOINT32 lppnt )
{
    MapWindowPoints32( 0, hwnd, lppnt, 1 );
}


/***********************************************************************
 *           WINPOS_WindowFromPoint
 *
 * Find the window and hittest for a given point.
 */
INT16 WINPOS_WindowFromPoint( WND* wndScope, POINT16 pt, WND **ppWnd )
{
    WND *wndPtr;
    INT16 hittest = HTERROR;
    INT16 x, y;

    *ppWnd = NULL;
    x = pt.x;
    y = pt.y;
    wndPtr = wndScope->child;
    for (;;)
    {
        while (wndPtr)
        {
            /* If point is in window, and window is visible, and it  */
            /* is enabled (or it's a top-level window), then explore */
            /* its children. Otherwise, go to the next window.       */

            if ((wndPtr->dwStyle & WS_VISIBLE) &&
                (!(wndPtr->dwStyle & WS_DISABLED) ||
                 ((wndPtr->dwStyle & (WS_POPUP | WS_CHILD)) != WS_CHILD)) &&
                (x >= wndPtr->rectWindow.left) &&
                (x < wndPtr->rectWindow.right) &&
                (y >= wndPtr->rectWindow.top) &&
                (y < wndPtr->rectWindow.bottom))
            {
                *ppWnd = wndPtr;  /* Got a suitable window */

                /* If window is minimized or disabled, return at once */
                if (wndPtr->dwStyle & WS_MINIMIZE) return HTCAPTION;
                if (wndPtr->dwStyle & WS_DISABLED) return HTERROR;

                /* If point is not in client area, ignore the children */
                if ((x < wndPtr->rectClient.left) ||
                    (x >= wndPtr->rectClient.right) ||
                    (y < wndPtr->rectClient.top) ||
                    (y >= wndPtr->rectClient.bottom)) break;

                x -= wndPtr->rectClient.left;
                y -= wndPtr->rectClient.top;
                wndPtr = wndPtr->child;
            }
            else wndPtr = wndPtr->next;
        }

        /* If nothing found, return the scope window */
        if (!*ppWnd)
        {
            *ppWnd = wndScope;
            if( pt.x >= (wndScope->rectClient.left - wndScope->rectWindow.left) &&
                pt.x >= (wndScope->rectClient.top - wndScope->rectWindow.top ) &&
                pt.x <= (wndScope->rectClient.right - wndScope->rectWindow.left) &&
                pt.x <= (wndScope->rectClient.bottom - wndScope->rectWindow.top ) )
                return HTCLIENT;
	    if( pt.x < 0 || pt.y < 0 || 
		pt.x > (wndScope->rectWindow.right - wndScope->rectWindow.left) ||
		pt.y > (wndScope->rectWindow.bottom - wndScope->rectWindow.top ) )
		return HTNOWHERE;
	    return HTCAPTION;	/* doesn't matter in this case */
        }

        /* Send the WM_NCHITTEST message (only if to the same task) */
        if ((*ppWnd)->hmemTaskQ != GetTaskQueue(0)) return HTCLIENT;
        hittest = (INT16)SendMessage16( (*ppWnd)->hwndSelf, WM_NCHITTEST, 0,
                                    MAKELONG( pt.x, pt.y ) );
        if (hittest != HTTRANSPARENT) return hittest;  /* Found the window */

        /* If no children found in last search, make point relative to parent*/
        if (!wndPtr)
        {
            x += (*ppWnd)->rectClient.left;
            y += (*ppWnd)->rectClient.top;
        }

        /* Restart the search from the next sibling */
        wndPtr = (*ppWnd)->next;
        *ppWnd = (*ppWnd)->parent;
    }
}


/*******************************************************************
 *         WindowFromPoint16   (USER.30)
 */
HWND16  WindowFromPoint16( POINT16 pt )
{
    WND *pWnd;
    WINPOS_WindowFromPoint( WIN_GetDesktop(), pt, &pWnd );
    return pWnd->hwndSelf;
}


/*******************************************************************
 *         WindowFromPoint32   (USER32.581)
 */
HWND32 WindowFromPoint32( POINT32 pt )
{
    WND *pWnd;
    POINT16 pt16;
    CONV_POINT32TO16( &pt, &pt16 );
    WINPOS_WindowFromPoint( WIN_GetDesktop(), pt16, &pWnd );
    return (HWND32)pWnd->hwndSelf;
}


/*******************************************************************
 *         ChildWindowFromPoint16   (USER.191)
 */
HWND16 ChildWindowFromPoint16( HWND16 hwndParent, POINT16 pt )
{
    /* pt is in the client coordinates */

    WND* wnd = WIN_FindWndPtr(hwndParent);
    RECT16 rect;

    if( !wnd ) return 0;

    /* get client rect fast */
    rect.top = rect.left = 0;
    rect.right = wnd->rectClient.right - wnd->rectClient.left;
    rect.bottom = wnd->rectClient.bottom - wnd->rectClient.top;

    if (!PtInRect16( &rect, pt )) return 0;

    wnd = wnd->child;
    while ( wnd )
    {
        if (PtInRect16( &wnd->rectWindow, pt )) return wnd->hwndSelf;
        wnd = wnd->next;
    }
    return hwndParent;
}


/*******************************************************************
 *         ChildWindowFromPoint32   (USER32.)
 */
HWND32 ChildWindowFromPoint32( HWND32 hwndParent, POINT32 pt )
{
    POINT16 pt16;
    CONV_POINT32TO16( &pt, &pt16 );
    return (HWND32)ChildWindowFromPoint16( hwndParent, pt16 );
}


/*******************************************************************
 *         WINPOS_GetWinOffset
 *
 * Calculate the offset between the origin of the two windows. Used
 * to implement MapWindowPoints.
 */
static void WINPOS_GetWinOffset( HWND32 hwndFrom, HWND32 hwndTo,
                                 POINT32 *offset )
{
    WND * wndPtr;

    offset->x = offset->y = 0;
    if (hwndFrom == hwndTo ) return;

      /* Translate source window origin to screen coords */
    if (hwndFrom)
    {
        if (!(wndPtr = WIN_FindWndPtr( hwndFrom )))
        {
            fprintf(stderr,"MapWindowPoints: bad hwndFrom = %04x\n",hwndFrom);
            return;
        }
        while (wndPtr->parent)
        {
            offset->x += wndPtr->rectClient.left;
            offset->y += wndPtr->rectClient.top;
            wndPtr = wndPtr->parent;
        }
    }

      /* Translate origin to destination window coords */
    if (hwndTo)
    {
        if (!(wndPtr = WIN_FindWndPtr( hwndTo )))
        {
            fprintf(stderr,"MapWindowPoints: bad hwndTo = %04x\n", hwndTo );
            return;
        }
        while (wndPtr->parent)
        {
            offset->x -= wndPtr->rectClient.left;
            offset->y -= wndPtr->rectClient.top;
            wndPtr = wndPtr->parent;
        }    
    }
}


/*******************************************************************
 *         MapWindowPoints16   (USER.258)
 */
void MapWindowPoints16( HWND16 hwndFrom, HWND16 hwndTo,
                        LPPOINT16 lppt, UINT16 count )
{
    POINT32 offset;

    WINPOS_GetWinOffset( hwndFrom, hwndTo, &offset );
    while (count--)
    {
	lppt->x += offset.x;
	lppt->y += offset.y;
        lppt++;
    }
}


/*******************************************************************
 *         MapWindowPoints32   (USER32.385)
 */
void MapWindowPoints32( HWND32 hwndFrom, HWND32 hwndTo,
                        LPPOINT32 lppt, UINT32 count )
{
    POINT32 offset;

    WINPOS_GetWinOffset( hwndFrom, hwndTo, &offset );
    while (count--)
    {
	lppt->x += offset.x;
	lppt->y += offset.y;
        lppt++;
    }
}


/***********************************************************************
 *           IsIconic   (USER.31)
 */
BOOL16 IsIconic16(HWND16 hWnd)
{
    return IsIconic32(hWnd);
}
/***********************************************************************
 *           IsIconic   (USER32.344)
 */
BOOL32 IsIconic32(HWND32 hWnd)
{
    WND * wndPtr = WIN_FindWndPtr(hWnd);
    if (wndPtr == NULL) return FALSE;
    return (wndPtr->dwStyle & WS_MINIMIZE) != 0;
}
 
 
/***********************************************************************
 *           IsZoomed   (USER.272)
 */
BOOL16 IsZoomed16(HWND16 hWnd)
{
    return IsZoomed32(hWnd);
}
/***********************************************************************
 *           IsZoomed   (USER32.351)
 */
BOOL32 IsZoomed32(HWND32 hWnd)
{
    WND * wndPtr = WIN_FindWndPtr(hWnd);
    if (wndPtr == NULL) return FALSE;
    return (wndPtr->dwStyle & WS_MAXIMIZE) != 0;
}


/*******************************************************************
 *         GetActiveWindow    (USER.60)
 */
HWND16 GetActiveWindow16(void)
{
    return GetActiveWindow32();
}
/*******************************************************************
 *         GetActiveWindow    (USER32.204)
 */
HWND32 GetActiveWindow32(void)
{
    return hwndActive;
}


/*******************************************************************
 *         WINPOS_IsGoodEnough
 */
static BOOL32 WINPOS_IsGoodEnough(WND* pWnd)
{
 return (pWnd) ? ((!(pWnd->dwStyle & WS_DISABLED) &&
                     pWnd->dwStyle & WS_VISIBLE ) ? TRUE : FALSE) : FALSE;
}


/*******************************************************************
 *         SetActiveWindow    (USER.59)
 */
HWND16 SetActiveWindow16( HWND16 hwnd )
{
    return SetActiveWindow32(hwnd);
}
/*******************************************************************
 *         SetActiveWindow    (USER.59)
 */
HWND32 SetActiveWindow32( HWND32 hwnd )
{
    HWND32 prev = hwndActive;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    if ( !WINPOS_IsGoodEnough(wndPtr) ) return 0;

    WINPOS_SetActiveWindow( hwnd, 0, 0 );
    return prev;
}


/***********************************************************************
 *           BringWindowToTop   (USER.45)
 */
BOOL16 BringWindowToTop16( HWND16 hwnd )
{
    return BringWindowToTop32(hwnd);
}
/***********************************************************************
 *           BringWindowToTop   (USER32.10)
 */
BOOL32 BringWindowToTop32( HWND32 hwnd )
{
    return SetWindowPos32( hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE );
}


/***********************************************************************
 *           MoveWindow   (USER.56)
 */
BOOL16 MoveWindow16(
	HWND16 hwnd, INT16 x, INT16 y, INT16 cx, INT16 cy, BOOL16 repaint
) {
    return MoveWindow32(hwnd,x,y,cx,cy,repaint);
}
/***********************************************************************
 *           MoveWindow   (USER32.398)
 */
BOOL32 MoveWindow32(
	HWND32 hwnd, INT32 x, INT32 y, INT32 cx, INT32 cy, BOOL32 repaint
) {    
    int flags = SWP_NOZORDER | SWP_NOACTIVATE;
    if (!repaint) flags |= SWP_NOREDRAW;
    dprintf_win(stddeb, "MoveWindow: %04x %d,%d %dx%d %d\n", 
	    hwnd, x, y, cx, cy, repaint );
    return SetWindowPos32( hwnd, 0, x, y, cx, cy, flags );
}

/***********************************************************************
 *           ShowWindow   (USER.42)
 */
BOOL16 ShowWindow16( HWND16 hwnd, INT16 cmd ) 
{    
    return ShowWindow32(hwnd,cmd);
}
/***********************************************************************
 *           ShowWindow   (USER.42)
 */
BOOL32 ShowWindow32( HWND32 hwnd, INT32 cmd ) 
{    
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    BOOL32 wasVisible, showFlag;
    POINT16 maxSize;
    int swpflags = 0;
    short x = 0, y = 0, cx = 0, cy = 0;

    if (!wndPtr) return FALSE;

    dprintf_win(stddeb,"ShowWindow: hwnd=%04x, cmd=%d\n", hwnd, cmd);

    wasVisible = (wndPtr->dwStyle & WS_VISIBLE) != 0;

    switch(cmd)
    {
        case SW_HIDE:
            if (!wasVisible) return FALSE;
	    swpflags |= SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOMOVE | 
		        SWP_NOACTIVATE | SWP_NOZORDER;
	    break;

	case SW_SHOWMINNOACTIVE:
            swpflags |= SWP_NOACTIVATE | SWP_NOZORDER;
            /* fall through */
	case SW_SHOWMINIMIZED:
            swpflags |= SWP_SHOWWINDOW;
            /* fall through */
	case SW_MINIMIZE:
            swpflags |= SWP_FRAMECHANGED;
            if (!(wndPtr->dwStyle & WS_MINIMIZE))
            {
		if( HOOK_CallHooks16( WH_CBT, HCBT_MINMAX, hwnd, cmd) )
		    return 0;

                if (wndPtr->dwStyle & WS_MAXIMIZE)
                {
                    wndPtr->flags |= WIN_RESTORE_MAX;
                    wndPtr->dwStyle &= ~WS_MAXIMIZE;
                }
                else
                {
                    wndPtr->flags &= ~WIN_RESTORE_MAX;
                    wndPtr->rectNormal = wndPtr->rectWindow;
                }
                wndPtr->dwStyle |= WS_MINIMIZE;
                WINPOS_FindIconPos( hwnd );
                x  = wndPtr->ptIconPos.x;
                y  = wndPtr->ptIconPos.y;
                cx = SYSMETRICS_CXICON;
                cy = SYSMETRICS_CYICON;
		swpflags |= SWP_NOCOPYBITS;
            }
            else swpflags |= SWP_NOSIZE | SWP_NOMOVE;
	    break;

	case SW_SHOWMAXIMIZED: /* same as SW_MAXIMIZE: */
            swpflags |= SWP_SHOWWINDOW | SWP_FRAMECHANGED;
            if (!(wndPtr->dwStyle & WS_MAXIMIZE))
            {
		if( HOOK_CallHooks16( WH_CBT, HCBT_MINMAX, hwnd, cmd) )
		    return 0;

                  /* Store the current position and find the maximized size */
                if (!(wndPtr->dwStyle & WS_MINIMIZE))
                    wndPtr->rectNormal = wndPtr->rectWindow; 

                NC_GetMinMaxInfo( wndPtr, &maxSize,
                                  &wndPtr->ptMaxPos, NULL, NULL );
                x  = wndPtr->ptMaxPos.x;
                y  = wndPtr->ptMaxPos.y;

		if( wndPtr->dwStyle & WS_MINIMIZE )
		    if( !SendMessage32A( hwnd, WM_QUERYOPEN, 0, 0L ) )
			{
		         swpflags |= SWP_NOSIZE | SWP_NOMOVE;
			 break;
			}
		    else swpflags |= SWP_NOCOPYBITS;

                cx = maxSize.x;
                cy = maxSize.y;
                wndPtr->dwStyle &= ~WS_MINIMIZE;
                wndPtr->dwStyle |= WS_MAXIMIZE;
            }
            else swpflags |= SWP_NOSIZE | SWP_NOMOVE;
            break;

	case SW_SHOWNA:
            swpflags |= SWP_NOACTIVATE | SWP_NOZORDER;
            /* fall through */
	case SW_SHOW:
	    swpflags |= SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE;
	    break;

	case SW_SHOWNOACTIVATE:
            swpflags |= SWP_NOZORDER;
            if (GetActiveWindow32()) swpflags |= SWP_NOACTIVATE;
            /* fall through */
	case SW_SHOWNORMAL:  /* same as SW_NORMAL: */
	case SW_SHOWDEFAULT: /* FIXME: should have its own handler */
	case SW_RESTORE:
	    swpflags |= SWP_SHOWWINDOW | SWP_FRAMECHANGED;

            if (wndPtr->dwStyle & WS_MINIMIZE)
            {
		if( HOOK_CallHooks16( WH_CBT, HCBT_MINMAX, hwnd, cmd) )
		    return 0;

                if( !SendMessage16( hwnd, WM_QUERYOPEN, 0, 0L) )
                  {
                    swpflags |= SWP_NOSIZE | SWP_NOMOVE;
                    break;
                  }
                wndPtr->ptIconPos.x = wndPtr->rectWindow.left;
                wndPtr->ptIconPos.y = wndPtr->rectWindow.top;
                wndPtr->dwStyle &= ~WS_MINIMIZE;
                if (wndPtr->flags & WIN_RESTORE_MAX)
                {
                    /* Restore to maximized position */
                    NC_GetMinMaxInfo( wndPtr, &maxSize, &wndPtr->ptMaxPos,
                                      NULL, NULL );
                    x  = wndPtr->ptMaxPos.x;
                    y  = wndPtr->ptMaxPos.y;
                    cx = maxSize.x;
                    cy = maxSize.y;
                   wndPtr->dwStyle |= WS_MAXIMIZE;
                }
                else  /* Restore to normal position */
                {
                    x  = wndPtr->rectNormal.left;
                    y  = wndPtr->rectNormal.top;
                    cx = wndPtr->rectNormal.right - wndPtr->rectNormal.left;
                    cy = wndPtr->rectNormal.bottom - wndPtr->rectNormal.top;
                }
		swpflags |= SWP_NOCOPYBITS;
            }
            else if (wndPtr->dwStyle & WS_MAXIMIZE)
            {
		if( HOOK_CallHooks16( WH_CBT, HCBT_MINMAX, hwnd, cmd) )
		    return 0;

                wndPtr->ptMaxPos.x = wndPtr->rectWindow.left;
                wndPtr->ptMaxPos.y = wndPtr->rectWindow.top;
                wndPtr->dwStyle &= ~WS_MAXIMIZE;
                x  = wndPtr->rectNormal.left;
                y  = wndPtr->rectNormal.top;
                cx = wndPtr->rectNormal.right - wndPtr->rectNormal.left;
                cy = wndPtr->rectNormal.bottom - wndPtr->rectNormal.top;
            }
            else swpflags |= SWP_NOSIZE | SWP_NOMOVE;
	    break;
    }

    showFlag = (cmd != SW_HIDE);
    if (showFlag != wasVisible)
    {
        SendMessage16( hwnd, WM_SHOWWINDOW, showFlag, 0 );
        if (!IsWindow32( hwnd )) return wasVisible;
    }

    if ((wndPtr->dwStyle & WS_CHILD) &&
        !IsWindowVisible32( wndPtr->parent->hwndSelf ) &&
        (swpflags & SWP_NOSIZE) && (swpflags & SWP_NOMOVE))
    {
        /* Don't call SetWindowPos32() on invisible child windows */
        if (cmd == SW_HIDE) wndPtr->dwStyle &= ~WS_VISIBLE;
        else wndPtr->dwStyle |= WS_VISIBLE;
    }
    else
    {
        /* We can't activate a child window */
        if (wndPtr->dwStyle & WS_CHILD)
            swpflags |= SWP_NOACTIVATE | SWP_NOZORDER;
        SetWindowPos32( hwnd, HWND_TOP, x, y, cx, cy, swpflags );
        if (!IsWindow32( hwnd )) return wasVisible;
    }

    if (wndPtr->flags & WIN_NEED_SIZE)
    {
        /* should happen only in CreateWindowEx() */
	int wParam = SIZE_RESTORED;

	wndPtr->flags &= ~WIN_NEED_SIZE;
	if (wndPtr->dwStyle & WS_MAXIMIZE) wParam = SIZE_MAXIMIZED;
	else if (wndPtr->dwStyle & WS_MINIMIZE) wParam = SIZE_MINIMIZED;
	SendMessage16( hwnd, WM_SIZE, wParam,
		     MAKELONG(wndPtr->rectClient.right-wndPtr->rectClient.left,
			    wndPtr->rectClient.bottom-wndPtr->rectClient.top));
	SendMessage16( hwnd, WM_MOVE, 0,
		   MAKELONG(wndPtr->rectClient.left, wndPtr->rectClient.top) );
    }

    return wasVisible;
}


/***********************************************************************
 *           GetInternalWindowPos16   (USER.460)
 */
UINT16 GetInternalWindowPos16( HWND16 hwnd, LPRECT16 rectWnd, LPPOINT16 ptIcon)
{
    WINDOWPLACEMENT16 wndpl;
    if (!GetWindowPlacement16( hwnd, &wndpl )) return 0;
    if (rectWnd) *rectWnd = wndpl.rcNormalPosition;
    if (ptIcon)  *ptIcon = wndpl.ptMinPosition;
    return wndpl.showCmd;
}


/***********************************************************************
 *           GetInternalWindowPos32   (USER32.244)
 */
UINT32 GetInternalWindowPos32( HWND32 hwnd, LPRECT32 rectWnd, LPPOINT32 ptIcon)
{
    WINDOWPLACEMENT32 wndpl;
    if (!GetWindowPlacement32( hwnd, &wndpl )) return 0;
    if (rectWnd) *rectWnd = wndpl.rcNormalPosition;
    if (ptIcon)  *ptIcon = wndpl.ptMinPosition;
    return wndpl.showCmd;
}


/***********************************************************************
 *           SetInternalWindowPos16   (USER.461)
 */
void SetInternalWindowPos16( HWND16 hwnd, UINT16 showCmd,
                             LPRECT16 rect, LPPOINT16 pt )
{
    WINDOWPLACEMENT16 wndpl;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    wndpl.length  = sizeof(wndpl);
    wndpl.flags   = (pt != NULL) ? WPF_SETMINPOSITION : 0;
    wndpl.showCmd = showCmd;
    if (pt) wndpl.ptMinPosition = *pt;
    wndpl.rcNormalPosition = (rect != NULL) ? *rect : wndPtr->rectNormal;
    wndpl.ptMaxPosition = wndPtr->ptMaxPos;
    SetWindowPlacement16( hwnd, &wndpl );
}


/***********************************************************************
 *           SetInternalWindowPos32   (USER32.482)
 */
void SetInternalWindowPos32( HWND32 hwnd, UINT32 showCmd,
                             LPRECT32 rect, LPPOINT32 pt )
{
    WINDOWPLACEMENT32 wndpl;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    wndpl.length  = sizeof(wndpl);
    wndpl.flags   = (pt != NULL) ? WPF_SETMINPOSITION : 0;
    wndpl.showCmd = showCmd;
    if (pt) wndpl.ptMinPosition = *pt;
    if (rect) wndpl.rcNormalPosition = *rect;
    else CONV_RECT16TO32( &wndPtr->rectNormal, &wndpl.rcNormalPosition );
    CONV_POINT16TO32( &wndPtr->ptMaxPos, &wndpl.ptMaxPosition );
    SetWindowPlacement32( hwnd, &wndpl );
}


/***********************************************************************
 *           GetWindowPlacement16   (USER.370)
 */
BOOL16 GetWindowPlacement16( HWND16 hwnd, WINDOWPLACEMENT16 *wndpl )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return FALSE;

    wndpl->length  = sizeof(*wndpl);
    wndpl->flags   = 0;
    wndpl->showCmd = IsZoomed16(hwnd) ? SW_SHOWMAXIMIZED : 
	             (IsIconic16(hwnd) ? SW_SHOWMINIMIZED : SW_SHOWNORMAL);
    wndpl->ptMinPosition = wndPtr->ptIconPos;
    wndpl->ptMaxPosition = wndPtr->ptMaxPos;
    wndpl->rcNormalPosition = wndPtr->rectNormal;
    return TRUE;
}


/***********************************************************************
 *           GetWindowPlacement32   (USER32.306)
 */
BOOL32 GetWindowPlacement32( HWND32 hwnd, WINDOWPLACEMENT32 *wndpl )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return FALSE;

    wndpl->length  = sizeof(*wndpl);
    wndpl->flags   = 0;
    wndpl->showCmd = IsZoomed32(hwnd) ? SW_SHOWMAXIMIZED : 
	             (IsIconic32(hwnd) ? SW_SHOWMINIMIZED : SW_SHOWNORMAL);
    CONV_POINT16TO32( &wndPtr->ptIconPos, &wndpl->ptMinPosition );
    CONV_POINT16TO32( &wndPtr->ptMaxPos, &wndpl->ptMaxPosition );
    CONV_RECT16TO32( &wndPtr->rectNormal, &wndpl->rcNormalPosition );
    return TRUE;
}


/***********************************************************************
 *           SetWindowPlacement16   (USER.371)
 */
BOOL16 SetWindowPlacement16( HWND16 hwnd, const WINDOWPLACEMENT16 *wndpl )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return FALSE;

    if (wndpl->flags & WPF_SETMINPOSITION)
	wndPtr->ptIconPos = wndpl->ptMinPosition;
    if ((wndpl->flags & WPF_RESTORETOMAXIMIZED) &&
	(wndpl->showCmd == SW_SHOWMINIMIZED)) wndPtr->flags |= WIN_RESTORE_MAX;
    wndPtr->ptMaxPos   = wndpl->ptMaxPosition;
    wndPtr->rectNormal = wndpl->rcNormalPosition;
    ShowWindow16( hwnd, wndpl->showCmd );
    return TRUE;
}


/***********************************************************************
 *           SetWindowPlacement32   (USER32.518)
 */
BOOL32 SetWindowPlacement32( HWND32 hwnd, const WINDOWPLACEMENT32 *wndpl )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return FALSE;

    if (wndpl->flags & WPF_SETMINPOSITION)
	CONV_POINT32TO16( &wndpl->ptMinPosition, &wndPtr->ptIconPos );
    if ((wndpl->flags & WPF_RESTORETOMAXIMIZED) &&
	(wndpl->showCmd == SW_SHOWMINIMIZED)) wndPtr->flags |= WIN_RESTORE_MAX;
    CONV_POINT32TO16( &wndpl->ptMaxPosition, &wndPtr->ptMaxPos );
    CONV_RECT32TO16( &wndpl->rcNormalPosition, &wndPtr->rectNormal );
    ShowWindow32( hwnd, wndpl->showCmd );
    return TRUE;
}


/***********************************************************************
 *           WINPOS_ForceXWindowRaise
 *
 * Raise a window on top of the X stacking order, while preserving 
 * the correct Windows Z order.
 */
static void WINPOS_ForceXWindowRaise( WND* pWnd )
{
    XWindowChanges winChanges;
    WND *wndPrev;

    /* Raise all windows up to pWnd according to their Z order.
     * (it would be easier with sibling-related Below but it doesn't
     * work very well with SGI mwm for instance)
     */
    winChanges.stack_mode = Above;
    while (pWnd)
    {
        if (pWnd->window) XReconfigureWMWindow( display, pWnd->window, 0,
                                                CWStackMode, &winChanges );
        wndPrev = WIN_GetDesktop()->child;
        if (wndPrev == pWnd) break;
        while (wndPrev && (wndPrev->next != pWnd)) wndPrev = wndPrev->next;
        pWnd = wndPrev;
    }
}


/*******************************************************************
 *	   WINPOS_SetActiveWindow
 *
 * back-end to SetActiveWindow
 */
BOOL32 WINPOS_SetActiveWindow( HWND32 hWnd, BOOL32 fMouse, BOOL32 fChangeFocus)
{
    WND                   *wndPtr          = WIN_FindWndPtr(hWnd);
    WND                   *wndTemp         = WIN_FindWndPtr(hwndActive);
    CBTACTIVATESTRUCT16   *cbtStruct;
    WORD                   wIconized=0;
    HQUEUE16 hOldActiveQueue = (pActiveQueue)?pActiveQueue->self:0;
    HQUEUE16 hNewActiveQueue;

    /* paranoid checks */
    if( hWnd == GetDesktopWindow32() || hWnd == hwndActive )
	return 0;

/* if (wndPtr && (GetTaskQueue(0) != wndPtr->hmemTaskQ))
	return 0;
        */

    if( wndTemp )
	wIconized = HIWORD(wndTemp->dwStyle & WS_MINIMIZE);
    else
	dprintf_win(stddeb,"WINPOS_ActivateWindow: no current active window.\n");

    /* call CBT hook chain */
    if ((cbtStruct = SEGPTR_NEW(CBTACTIVATESTRUCT16)))
    {
        LRESULT wRet;
        cbtStruct->fMouse     = fMouse;
        cbtStruct->hWndActive = hwndActive;
        wRet = HOOK_CallHooks16( WH_CBT, HCBT_ACTIVATE, (WPARAM16)hWnd,
                                 (LPARAM)SEGPTR_GET(cbtStruct) );
        SEGPTR_FREE(cbtStruct);
        if (wRet) return wRet;
    }

    /* set prev active wnd to current active wnd and send notification */
    if ((hwndPrevActive = hwndActive) && IsWindow32(hwndPrevActive))
    {
        if (!SendMessage16( hwndPrevActive, WM_NCACTIVATE, FALSE, 0 ))
        {
	    if (GetSysModalWindow16() != hWnd) return 0;
	    /* disregard refusal if hWnd is sysmodal */
        }

#if 0
	SendMessage32A( hwndPrevActive, WM_ACTIVATE,
                        MAKEWPARAM( WA_INACTIVE, wIconized ),
                        (LPARAM)hWnd );
#else
	/* FIXME: must be SendMessage16() because 32A doesn't do
	 * intertask at this time */
	SendMessage16( hwndPrevActive, WM_ACTIVATE, WA_INACTIVE,
				MAKELPARAM( (HWND16)hWnd, wIconized ) );
#endif

	/* check if something happened during message processing */
	if( hwndPrevActive != hwndActive ) return 0;
    }

    /* set active wnd */
    hwndActive = hWnd;

    /* send palette messages */
    if (hWnd && SendMessage16( hWnd, WM_QUERYNEWPALETTE, 0, 0L))
	SendMessage16((HWND16)-1, WM_PALETTEISCHANGING, (WPARAM16)hWnd, 0L );

    /* if prev wnd is minimized redraw icon title 
  if( hwndPrevActive )
    {
        wndTemp = WIN_FindWndPtr( WIN_GetTopParent( hwndPrevActive ) );
        if(wndTemp)
          if(wndTemp->dwStyle & WS_MINIMIZE)
            RedrawIconTitle(hwndPrevActive); 
      } 
  */

    /* managed windows will get ConfigureNotify event */  
    if (wndPtr && !(wndPtr->dwStyle & WS_CHILD) && !(wndPtr->flags & WIN_MANAGED))
    {
	/* check Z-order and bring hWnd to the top */
	for (wndTemp = WIN_GetDesktop()->child; wndTemp; wndTemp = wndTemp->next)
	    if (wndTemp->dwStyle & WS_VISIBLE) break;

	if( wndTemp != wndPtr )
	    SetWindowPos32(hWnd, HWND_TOP, 0,0,0,0, 
			   SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE );
        if (!IsWindow32(hWnd)) return 0;
    }

    hNewActiveQueue = wndPtr ? wndPtr->hmemTaskQ : 0;

    /* send WM_ACTIVATEAPP if necessary */
    if (hOldActiveQueue != hNewActiveQueue)
    {
        WND **list, **ppWnd;

        if ((list = WIN_BuildWinArray( WIN_GetDesktop() )))
        {
            for (ppWnd = list; *ppWnd; ppWnd++)
            {
                if (!IsWindow32( (*ppWnd)->hwndSelf )) continue;

                if ((*ppWnd)->hmemTaskQ == hOldActiveQueue)
                   SendMessage16( (*ppWnd)->hwndSelf, WM_ACTIVATEAPP,
                                   0, QUEUE_GetQueueTask(hNewActiveQueue) );
            }
            HeapFree( SystemHeap, 0, list );
        }

	pActiveQueue = (hNewActiveQueue)
		       ? (MESSAGEQUEUE*) GlobalLock16(hNewActiveQueue) : NULL;

        if ((list = WIN_BuildWinArray( WIN_GetDesktop() )))
        {
            for (ppWnd = list; *ppWnd; ppWnd++)
            {
                if (!IsWindow32( (*ppWnd)->hwndSelf )) continue;

                if ((*ppWnd)->hmemTaskQ == hNewActiveQueue)
                   SendMessage16( (*ppWnd)->hwndSelf, WM_ACTIVATEAPP,
                                  1, QUEUE_GetQueueTask( hOldActiveQueue ) );
            }
            HeapFree( SystemHeap, 0, list );
        }
	if (!IsWindow32(hWnd)) return 0;
    }

    if (hWnd)
    {
        /* walk up to the first unowned window */
        wndTemp = wndPtr;
        while (wndTemp->owner) wndTemp = wndTemp->owner;
        /* and set last active owned popup */
        wndTemp->hwndLastActive = hWnd;

        wIconized = HIWORD(wndTemp->dwStyle & WS_MINIMIZE);
        SendMessage16( hWnd, WM_NCACTIVATE, TRUE, 0 );
#if 0
        SendMessage32A( hWnd, WM_ACTIVATE,
		 MAKEWPARAM( (fMouse) ? WA_CLICKACTIVE : WA_ACTIVE, wIconized),
		 (LPARAM)hwndPrevActive );
#else
        SendMessage16(hWnd, WM_ACTIVATE, (fMouse) ? WA_CLICKACTIVE : WA_ACTIVE,
                      MAKELPARAM( (HWND16)hwndPrevActive, wIconized) );
#endif

        if( !IsWindow32(hWnd) ) return 0;
    }

    /* change focus if possible */
    if( fChangeFocus && GetFocus32() )
	if( WIN_GetTopParent(GetFocus32()) != hwndActive )
	    FOCUS_SwitchFocus( GetFocus32(),
			       (wndPtr->dwStyle & WS_MINIMIZE)? 0: hwndActive);

    if( !hwndPrevActive && wndPtr && 
	 wndPtr->window && !(wndPtr->flags & WIN_MANAGED) )
	WINPOS_ForceXWindowRaise(wndPtr);

    /* if active wnd is minimized redraw icon title 
  if( hwndActive )
      {
        wndPtr = WIN_FindWndPtr(hwndActive);
        if(wndPtr->dwStyle & WS_MINIMIZE)
           RedrawIconTitle(hwndActive);
    }
  */
    return (hWnd == hwndActive);
}

/*******************************************************************
 *         WINPOS_ActivateOtherWindow
 *
 * DestroyWindow() helper. pWnd must be a top-level window.
 */
BOOL32 WINPOS_ActivateOtherWindow(WND* pWnd)
{
  BOOL32	bRet = 0;
  WND*  	pWndTo = NULL;

  if( pWnd->hwndSelf == hwndPrevActive )
      hwndPrevActive = 0;

  if( hwndActive != pWnd->hwndSelf && 
    ( hwndActive || QUEUE_IsDoomedQueue(pWnd->hmemTaskQ)) )
      return 0;

  if( pWnd->dwStyle & WS_POPUP &&
      WINPOS_IsGoodEnough( pWnd->owner ) ) pWndTo = pWnd->owner;
  else
  {
    WND* pWndPtr = pWnd;

    pWndTo = WIN_FindWndPtr(hwndPrevActive);

    while( !WINPOS_IsGoodEnough(pWndTo) ) 
    {
      /* by now owned windows should've been taken care of */

      pWndTo = pWndPtr->next;
      pWndPtr = pWndTo;
      if( !pWndTo ) return 0;
    }
  }

  bRet = WINPOS_SetActiveWindow( pWndTo->hwndSelf, FALSE, TRUE );

  /* switch desktop queue to current active */
  if( pWndTo->parent == WIN_GetDesktop())
      WIN_GetDesktop()->hmemTaskQ = pWndTo->hmemTaskQ;

  hwndPrevActive = 0;
  return bRet;  
}

/*******************************************************************
 *	   WINPOS_ChangeActiveWindow
 *
 */
BOOL32 WINPOS_ChangeActiveWindow( HWND32 hWnd, BOOL32 mouseMsg )
{
    WND *wndPtr = WIN_FindWndPtr(hWnd);

    if (!hWnd) return WINPOS_SetActiveWindow( 0, mouseMsg, TRUE );

    if( !wndPtr ) return FALSE;

    /* child windows get WM_CHILDACTIVATE message */
    if( (wndPtr->dwStyle & (WS_CHILD | WS_POPUP)) == WS_CHILD )
	return SendMessage16(hWnd, WM_CHILDACTIVATE, 0, 0L);

        /* owned popups imply owner activation - not sure */
    if ((wndPtr->dwStyle & WS_POPUP) && wndPtr->owner &&
        !(wndPtr->owner->dwStyle & WS_DISABLED ))
    {
        if (!(wndPtr = wndPtr->owner)) return FALSE;
	hWnd = wndPtr->hwndSelf;
    }

    if( hWnd == hwndActive ) return FALSE;

    if( !WINPOS_SetActiveWindow(hWnd ,mouseMsg ,TRUE) )
	return FALSE;

    /* switch desktop queue to current active */
    if( wndPtr->parent == WIN_GetDesktop())
        WIN_GetDesktop()->hmemTaskQ = wndPtr->hmemTaskQ;

    return TRUE;
}


/***********************************************************************
 *           WINPOS_SendNCCalcSize
 *
 * Send a WM_NCCALCSIZE message to a window.
 * All parameters are read-only except newClientRect.
 * oldWindowRect, oldClientRect and winpos must be non-NULL only
 * when calcValidRect is TRUE.
 */
LONG WINPOS_SendNCCalcSize( HWND32 hwnd, BOOL32 calcValidRect,
                            RECT16 *newWindowRect, RECT16 *oldWindowRect,
                            RECT16 *oldClientRect, SEGPTR winpos,
                            RECT16 *newClientRect )
{
    NCCALCSIZE_PARAMS16 *params;
    LONG result;

    if (!(params = SEGPTR_NEW(NCCALCSIZE_PARAMS16))) return 0;
    params->rgrc[0] = *newWindowRect;
    if (calcValidRect)
    {
	params->rgrc[1] = *oldWindowRect;
	params->rgrc[2] = *oldClientRect;
	params->lppos = winpos;
    }
    result = SendMessage16( hwnd, WM_NCCALCSIZE, calcValidRect,
                          (LPARAM)SEGPTR_GET( params ) );
    dprintf_win( stddeb, "WINPOS_SendNCCalcSize: %d,%d-%d,%d\n",
                 params->rgrc[0].left, params->rgrc[0].top,
                 params->rgrc[0].right, params->rgrc[0].bottom );
    *newClientRect = params->rgrc[0];
    SEGPTR_FREE(params);
    return result;
}


/***********************************************************************
 *           WINPOS_HandleWindowPosChanging16
 *
 * Default handling for a WM_WINDOWPOSCHANGING. Called from DefWindowProc().
 */
LONG WINPOS_HandleWindowPosChanging16( WND *wndPtr, WINDOWPOS16 *winpos )
{
    POINT16 maxSize;
    if (winpos->flags & SWP_NOSIZE) return 0;
    if ((wndPtr->dwStyle & WS_THICKFRAME) ||
	((wndPtr->dwStyle & (WS_POPUP | WS_CHILD)) == 0))
    {
	NC_GetMinMaxInfo( wndPtr, &maxSize, NULL, NULL, NULL );
	winpos->cx = MIN( winpos->cx, maxSize.x );
	winpos->cy = MIN( winpos->cy, maxSize.y );
    }
    return 0;
}


/***********************************************************************
 *           WINPOS_HandleWindowPosChanging32
 *
 * Default handling for a WM_WINDOWPOSCHANGING. Called from DefWindowProc().
 */
LONG WINPOS_HandleWindowPosChanging32( WND *wndPtr, WINDOWPOS32 *winpos )
{
    POINT16 maxSize;
    if (winpos->flags & SWP_NOSIZE) return 0;
    if ((wndPtr->dwStyle & WS_THICKFRAME) ||
	((wndPtr->dwStyle & (WS_POPUP | WS_CHILD)) == 0))
    {
	NC_GetMinMaxInfo( wndPtr, &maxSize, NULL, NULL, NULL );
	winpos->cx = MIN( winpos->cx, maxSize.x );
	winpos->cy = MIN( winpos->cy, maxSize.y );
    }
    return 0;
}


/***********************************************************************
 *           WINPOS_MoveWindowZOrder
 *
 * Move a window in Z order, invalidating everything that needs it.
 * Only necessary for windows without associated X window.
 */
static void WINPOS_MoveWindowZOrder( HWND32 hwnd, HWND32 hwndAfter )
{
    BOOL32 movingUp;
    WND *pWndAfter, *pWndCur, *wndPtr = WIN_FindWndPtr( hwnd );

    /* We have two possible cases:
     * - The window is moving up: we have to invalidate all areas
     *   of the window that were covered by other windows
     * - The window is moving down: we have to invalidate areas
     *   of other windows covered by this one.
     */

    if (hwndAfter == HWND_TOP)
    {
        movingUp = TRUE;
    }
    else if (hwndAfter == HWND_BOTTOM)
    {
        if (!wndPtr->next) return;  /* Already at the bottom */
        movingUp = FALSE;
    }
    else
    {
        if (!(pWndAfter = WIN_FindWndPtr( hwndAfter ))) return;
        if (wndPtr->next == pWndAfter) return;  /* Already placed right */

          /* Determine which window we encounter first in Z-order */
        pWndCur = wndPtr->parent->child;
        while ((pWndCur != wndPtr) && (pWndCur != pWndAfter))
            pWndCur = pWndCur->next;
        movingUp = (pWndCur == pWndAfter);
    }

    if (movingUp)
    {
        WND *pWndPrevAfter = wndPtr->next;
        WIN_UnlinkWindow( hwnd );
        WIN_LinkWindow( hwnd, hwndAfter );
        pWndCur = wndPtr->next;
        while (pWndCur != pWndPrevAfter)
        {
            RECT32 rect = { pWndCur->rectWindow.left,
			    pWndCur->rectWindow.top,
			    pWndCur->rectWindow.right,
			    pWndCur->rectWindow.bottom };
            OffsetRect32( &rect, -wndPtr->rectClient.left,
                          -wndPtr->rectClient.top );
            PAINT_RedrawWindow( hwnd, &rect, 0, RDW_INVALIDATE | RDW_ALLCHILDREN |
                              RDW_FRAME | RDW_ERASE, 0 );
            pWndCur = pWndCur->next;
        }
    }
    else  /* Moving down */
    {
        pWndCur = wndPtr->next;
        WIN_UnlinkWindow( hwnd );
        WIN_LinkWindow( hwnd, hwndAfter );
        while (pWndCur != wndPtr)
        {
            RECT32 rect = { pWndCur->rectWindow.left,
                            pWndCur->rectWindow.top,
                            pWndCur->rectWindow.right,
                            pWndCur->rectWindow.bottom };
            OffsetRect32( &rect, -pWndCur->rectClient.left,
                          -pWndCur->rectClient.top );
            PAINT_RedrawWindow( pWndCur->hwndSelf, &rect, 0, RDW_INVALIDATE |
                              RDW_ALLCHILDREN | RDW_FRAME | RDW_ERASE, 0 );
            pWndCur = pWndCur->next;
        }
    }
}

/***********************************************************************
 *           WINPOS_ReorderOwnedPopups
 *
 * fix Z order taking into account owned popups -
 * basically we need to maintain them above owner window
 */
HWND32 WINPOS_ReorderOwnedPopups(HWND32 hwndInsertAfter,WND* wndPtr,WORD flags)
{
 WND* 	w = WIN_GetDesktop()->child;

  if( wndPtr->dwStyle & WS_POPUP && wndPtr->owner )
  {
   /* implement "local z-order" between the top and owner window */

     HWND32 hwndLocalPrev = HWND_TOP;

     if( hwndInsertAfter != HWND_TOP )
     {
	while( w != wndPtr->owner )
	{
          if (w != wndPtr) hwndLocalPrev = w->hwndSelf;
	  if( hwndLocalPrev == hwndInsertAfter ) break;
	  w = w->next;
	}
	hwndInsertAfter = hwndLocalPrev;
     }

  }
  else if( wndPtr->dwStyle & WS_CHILD ) return hwndInsertAfter;

  w = WIN_GetDesktop()->child;
  while( w )
  {
    if( w == wndPtr ) break;

    if( w->dwStyle & WS_POPUP && w->owner == wndPtr )
    {
      SetWindowPos32(w->hwndSelf, hwndInsertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE |
                     SWP_NOACTIVATE | SWP_NOSENDCHANGING | SWP_DEFERERASE);
      hwndInsertAfter = w->hwndSelf;
    }
    w = w->next;
  }

  return hwndInsertAfter;
}

/***********************************************************************
 *	     WINPOS_SizeMoveClean
 *
 * Make window look nice without excessive repainting
 *
 * the pain:
 *
 * visible regions are in window coordinates
 * update regions are in window client coordinates
 * client and window rectangles are in parent client coordinates
 */
static UINT32 WINPOS_SizeMoveClean(WND* Wnd, HRGN32 oldVisRgn, LPRECT16 lpOldWndRect, LPRECT16 lpOldClientRect, UINT32 uFlags )
{
 HRGN32 newVisRgn = DCE_GetVisRgn(Wnd->hwndSelf,DCX_WINDOW | DCX_CLIPSIBLINGS);
 HRGN32 dirtyRgn = CreateRectRgn32(0,0,0,0);
 int  other, my;

 dprintf_win(stddeb,"cleaning up...new wnd=(%i %i-%i %i) old wnd=(%i %i-%i %i)\n\
\t\tnew client=(%i %i-%i %i) old client=(%i %i-%i %i)\n",
		   Wnd->rectWindow.left, Wnd->rectWindow.top, Wnd->rectWindow.right, Wnd->rectWindow.bottom,
		   lpOldWndRect->left, lpOldWndRect->top, lpOldWndRect->right, lpOldWndRect->bottom,
		   Wnd->rectClient.left,Wnd->rectClient.top,Wnd->rectClient.right,Wnd->rectClient.bottom,
		   lpOldClientRect->left,lpOldClientRect->top,lpOldClientRect->right,lpOldClientRect->bottom);

 if( (lpOldWndRect->right - lpOldWndRect->left) != (Wnd->rectWindow.right - Wnd->rectWindow.left) ||
     (lpOldWndRect->bottom - lpOldWndRect->top) != (Wnd->rectWindow.bottom - Wnd->rectWindow.top) )
     uFlags |= SMC_DRAWFRAME;

 CombineRgn32( dirtyRgn, newVisRgn, 0, RGN_COPY);

 if( !(uFlags & SMC_NOCOPY) )
   CombineRgn32( newVisRgn, newVisRgn, oldVisRgn, RGN_AND ); 

 /* map regions to the parent client area */
 
 OffsetRgn32( dirtyRgn, Wnd->rectWindow.left, Wnd->rectWindow.top );
 OffsetRgn32( oldVisRgn, lpOldWndRect->left, lpOldWndRect->top );

 /* compute invalidated region outside Wnd - (in client coordinates of the parent window) */

 other = CombineRgn32(dirtyRgn, oldVisRgn, dirtyRgn, RGN_DIFF);

 /* map visible region to the Wnd client area */

 OffsetRgn32( newVisRgn, Wnd->rectWindow.left - Wnd->rectClient.left,
                         Wnd->rectWindow.top - Wnd->rectClient.top );

 /* substract previously invalidated region from the Wnd visible region */

 my =  (Wnd->hrgnUpdate > 1) ? CombineRgn32( newVisRgn, newVisRgn,
                                             Wnd->hrgnUpdate, RGN_DIFF)
                             : COMPLEXREGION;

 if( uFlags & SMC_NOCOPY )	/* invalidate Wnd visible region */
   {
     if (my != NULLREGION)
	 PAINT_RedrawWindow( Wnd->hwndSelf, NULL, newVisRgn, RDW_INVALIDATE |
	  RDW_FRAME | RDW_ALLCHILDREN | RDW_ERASE, RDW_C_USEHRGN );
     else if(uFlags & SMC_DRAWFRAME)
	 Wnd->flags |= WIN_NEEDS_NCPAINT;
   }
 else			/* bitblt old client area */
   { 
     HDC32 hDC;
     int   update;
     HRGN32 updateRgn;
     int   xfrom,yfrom,xto,yto,width,height;

     if( uFlags & SMC_DRAWFRAME )
       {
	 /* copy only client area, frame will be redrawn anyway */

         xfrom = lpOldClientRect->left; yfrom = lpOldClientRect->top;
         xto = Wnd->rectClient.left; yto = Wnd->rectClient.top;
         width = lpOldClientRect->right - xfrom; height = lpOldClientRect->bottom - yfrom;
	 updateRgn = CreateRectRgn32( 0, 0, width, height );
	 CombineRgn32( newVisRgn, newVisRgn, updateRgn, RGN_AND );
	 SetRectRgn32( updateRgn, 0, 0, Wnd->rectClient.right - xto,
                       Wnd->rectClient.bottom - yto );
       }
     else
       {
         xfrom = lpOldWndRect->left; yfrom = lpOldWndRect->top;
         xto = Wnd->rectWindow.left; yto = Wnd->rectWindow.top;
         width = lpOldWndRect->right - xfrom; height = lpOldWndRect->bottom - yfrom;
	 updateRgn = CreateRectRgn32( xto - Wnd->rectClient.left,
				      yto - Wnd->rectClient.top,
				Wnd->rectWindow.right - Wnd->rectClient.left,
			        Wnd->rectWindow.bottom - Wnd->rectClient.top );
       }

     CombineRgn32( newVisRgn, newVisRgn, updateRgn, RGN_AND );

     /* substract new visRgn from target rect to get a region that won't be copied */

     update = CombineRgn32( updateRgn, updateRgn, newVisRgn, RGN_DIFF );

     /* Blt valid bits using parent window DC */

     if( my != NULLREGION && (xfrom != xto || yfrom != yto) )
       {
	 
	 /* compute clipping region in parent client coordinates */

	 OffsetRgn32( newVisRgn, Wnd->rectClient.left, Wnd->rectClient.top );
	 CombineRgn32( oldVisRgn, oldVisRgn, newVisRgn, RGN_OR );

         hDC = GetDCEx32( Wnd->parent->hwndSelf, oldVisRgn,
                          DCX_KEEPCLIPRGN | DCX_INTERSECTRGN |
                          DCX_CACHE | DCX_CLIPSIBLINGS);

         BitBlt32( hDC, xto, yto, width, height, hDC, xfrom, yfrom, SRCCOPY );
         ReleaseDC32( Wnd->parent->hwndSelf, hDC); 
       }

     if( update != NULLREGION )
         PAINT_RedrawWindow( Wnd->hwndSelf, NULL, updateRgn, RDW_INVALIDATE |
                         RDW_FRAME | RDW_ALLCHILDREN | RDW_ERASE, RDW_C_USEHRGN );
     else if( uFlags & SMC_DRAWFRAME ) Wnd->flags |= WIN_NEEDS_NCPAINT;
     DeleteObject32( updateRgn );
   }

 /* erase uncovered areas */

 if( !(uFlags & SMC_NOPARENTERASE) && (other != NULLREGION ) )
      PAINT_RedrawWindow( Wnd->parent->hwndSelf, NULL, dirtyRgn,
                        RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE, RDW_C_USEHRGN );
 DeleteObject32(dirtyRgn);
 DeleteObject32(newVisRgn);
 return uFlags;
}


/***********************************************************************
 *           WINPOS_FindDeskTopXWindow
 *
 * Find the actual X window which needs be restacked.
 * Used by WINPOS_SetXWindowPos().
 */
static Window WINPOS_FindDeskTopXWindow( WND *wndPtr )
{
    if (!(wndPtr->flags & WIN_MANAGED))
        return wndPtr->window;
    else
    {
        Window window, root, parent, *children;
        int nchildren;
        window = wndPtr->window;
        for (;;)
        {
            XQueryTree( display, window, &root, &parent,
                        &children, &nchildren );
            XFree( children );
            if (parent == root)
                return window;
            window = parent;
        }
    }
}

/***********************************************************************
 *           WINPOS_SetXWindowPos
 *
 * SetWindowPos() for an X window. Used by the real SetWindowPos().
 */
static void WINPOS_SetXWindowPos( WINDOWPOS16 *winpos )
{
    XWindowChanges winChanges;
    int changeMask = 0;
    WND *wndPtr = WIN_FindWndPtr( winpos->hwnd );

    if (!(winpos->flags & SWP_NOSIZE))
    {
        winChanges.width     = winpos->cx;
        winChanges.height    = winpos->cy;
        changeMask |= CWWidth | CWHeight;

        /* Tweak dialog window size hints */

        if ((wndPtr->flags & WIN_MANAGED) &&
            (wndPtr->dwExStyle & WS_EX_DLGMODALFRAME))
        {
            XSizeHints *size_hints = XAllocSizeHints();

            if (size_hints)
            {
                long supplied_return;

                XGetWMSizeHints( display, wndPtr->window, size_hints,
                                 &supplied_return, XA_WM_NORMAL_HINTS);
                size_hints->min_width = size_hints->max_width = winpos->cx;
                size_hints->min_height = size_hints->max_height = winpos->cy;
                XSetWMSizeHints( display, wndPtr->window, size_hints,
                                 XA_WM_NORMAL_HINTS );
                XFree(size_hints);
            }
        }
    }
    if (!(winpos->flags & SWP_NOMOVE))
    {
        winChanges.x = winpos->x;
        winChanges.y = winpos->y;
        changeMask |= CWX | CWY;
    }
    if (!(winpos->flags & SWP_NOZORDER))
    {
	winChanges.stack_mode = Below;
	changeMask |= CWStackMode;

        if (winpos->hwndInsertAfter == HWND_TOP) winChanges.stack_mode = Above;
        else if (winpos->hwndInsertAfter != HWND_BOTTOM)
        {
            WND*   insertPtr = WIN_FindWndPtr( winpos->hwndInsertAfter );
	    Window stack[2];

	    stack[0] = WINPOS_FindDeskTopXWindow( insertPtr );
	    stack[1] = WINPOS_FindDeskTopXWindow( wndPtr );

	    /* for stupid window managers (i.e. all of them) */

	    XRestackWindows(display, stack, 2); 
	    changeMask &= ~CWStackMode;
	}
    }
    if (!changeMask) return;

    XReconfigureWMWindow( display, wndPtr->window, 0, changeMask, &winChanges );
}


/***********************************************************************
 *           SetWindowPos   (USER.232)
 */
BOOL16 SetWindowPos16( HWND16 hwnd, HWND16 hwndInsertAfter, INT16 x, INT16 y,
                       INT16 cx, INT16 cy, WORD flags )
{
    return SetWindowPos32(hwnd,(INT32)(INT16)hwndInsertAfter,x,y,cx,cy,flags);
}

/***********************************************************************
 *           SetWindowPos   (USER32.519)
 */
BOOL32 SetWindowPos32( HWND32 hwnd, HWND32 hwndInsertAfter, INT32 x, INT32 y,
                       INT32 cx, INT32 cy, WORD flags )
{
    WINDOWPOS16 *winpos;
    WND *	wndPtr;
    RECT16 	newWindowRect, newClientRect, oldWindowRect;
    HRGN32	visRgn = 0;
    HWND32	tempInsertAfter= 0;
    int 	result = 0;
    UINT32 	uFlags = 0;

    dprintf_win(stddeb,"SetWindowPos: hwnd %04x, (%i,%i)-(%i,%i) flags %08x\n", 
						 hwnd, x, y, x+cx, y+cy, flags);  
      /* Check window handle */

    if (hwnd == GetDesktopWindow32()) return FALSE;
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;

    if (wndPtr->dwStyle & WS_VISIBLE) flags &= ~SWP_SHOWWINDOW;
    else
    {
	uFlags |= SMC_NOPARENTERASE; 
	flags &= ~SWP_HIDEWINDOW;
	if (!(flags & SWP_SHOWWINDOW)) flags |= SWP_NOREDRAW;
    }

/*     Check for windows that may not be resized 
       FIXME: this should be done only for Windows 3.0 programs 
       if (flags & (SWP_SHOWWINDOW | SWP_HIDEWINDOW ) )
           flags |= SWP_NOSIZE | SWP_NOMOVE;
*/
      /* Check dimensions */

    if (cx <= 0) cx = 1;
    if (cy <= 0) cy = 1;

      /* Check flags */

    if (hwnd == hwndActive) flags |= SWP_NOACTIVATE;   /* Already active */
    if ((wndPtr->rectWindow.right - wndPtr->rectWindow.left == cx) &&
        (wndPtr->rectWindow.bottom - wndPtr->rectWindow.top == cy))
        flags |= SWP_NOSIZE;    /* Already the right size */
    if ((wndPtr->rectWindow.left == x) && (wndPtr->rectWindow.top == y))
        flags |= SWP_NOMOVE;    /* Already the right position */

      /* Check hwndInsertAfter */

    if (!(flags & (SWP_NOZORDER | SWP_NOACTIVATE)))
    {
	  /* Ignore TOPMOST flags when activating a window */
          /* _and_ moving it in Z order. */
	if ((hwndInsertAfter == HWND_TOPMOST) ||
            (hwndInsertAfter == HWND_NOTOPMOST))
	    hwndInsertAfter = HWND_TOP;	
    }
      /* TOPMOST not supported yet */
    if ((hwndInsertAfter == HWND_TOPMOST) ||
        (hwndInsertAfter == HWND_NOTOPMOST)) hwndInsertAfter = HWND_TOP;

      /* hwndInsertAfter must be a sibling of the window */
    if ((hwndInsertAfter != HWND_TOP) && (hwndInsertAfter != HWND_BOTTOM))
       {
	 WND* wnd = WIN_FindWndPtr(hwndInsertAfter);
	 if( wnd->parent != wndPtr->parent ) return FALSE;
	 if( wnd->next == wndPtr ) flags |= SWP_NOZORDER;
       }
    else if (!(wndPtr->window))
         /* FIXME: the following optimization is no good for "X-ed" windows */
       if (hwndInsertAfter == HWND_TOP)
	   flags |= ( wndPtr->parent->child == wndPtr)? SWP_NOZORDER: 0;
       else /* HWND_BOTTOM */
	   flags |= ( wndPtr->next )? 0: SWP_NOZORDER;

      /* Fill the WINDOWPOS structure */

    if (!(winpos = SEGPTR_NEW(WINDOWPOS16))) return FALSE;
    winpos->hwnd = hwnd;
    winpos->hwndInsertAfter = hwndInsertAfter;
    winpos->x = x;
    winpos->y = y;
    winpos->cx = cx;
    winpos->cy = cy;
    winpos->flags = flags;
    
      /* Send WM_WINDOWPOSCHANGING message */

    if (!(flags & SWP_NOSENDCHANGING))
	SendMessage16( hwnd, WM_WINDOWPOSCHANGING, 0,
                       (LPARAM)SEGPTR_GET(winpos) );

      /* Calculate new position and size */

    newWindowRect = wndPtr->rectWindow;
    newClientRect = (wndPtr->dwStyle & WS_MINIMIZE) ? wndPtr->rectWindow
						    : wndPtr->rectClient;

    if (!(winpos->flags & SWP_NOSIZE))
    {
        newWindowRect.right  = newWindowRect.left + winpos->cx;
        newWindowRect.bottom = newWindowRect.top + winpos->cy;
    }
    if (!(winpos->flags & SWP_NOMOVE))
    {
        newWindowRect.left    = winpos->x;
        newWindowRect.top     = winpos->y;
        newWindowRect.right  += winpos->x - wndPtr->rectWindow.left;
        newWindowRect.bottom += winpos->y - wndPtr->rectWindow.top;

	OffsetRect16( &newClientRect, winpos->x - wndPtr->rectWindow.left, 
                                      winpos->y - wndPtr->rectWindow.top );
    }

    winpos->flags |= SWP_NOCLIENTMOVE | SWP_NOCLIENTSIZE;

      /* Reposition window in Z order */

    if (!(winpos->flags & SWP_NOZORDER))
    {
	/* reorder owned popups if hwnd is top-level window 
         */
	if( wndPtr->parent == WIN_GetDesktop() )
	    hwndInsertAfter = WINPOS_ReorderOwnedPopups( hwndInsertAfter,
							 wndPtr, flags );

        if (wndPtr->window)
        {
            WIN_UnlinkWindow( winpos->hwnd );
            WIN_LinkWindow( winpos->hwnd, hwndInsertAfter );
        }
        else WINPOS_MoveWindowZOrder( winpos->hwnd, hwndInsertAfter );
    }

    if ( !wndPtr->window && !(flags & SWP_NOREDRAW) && 
        (!(flags & SWP_NOMOVE) || !(flags & SWP_NOSIZE) || (flags & SWP_FRAMECHANGED)) )
          visRgn = DCE_GetVisRgn(hwnd, DCX_WINDOW | DCX_CLIPSIBLINGS);


      /* Send WM_NCCALCSIZE message to get new client area */
    if( (flags & (SWP_FRAMECHANGED | SWP_NOSIZE)) != SWP_NOSIZE )
      {
         result = WINPOS_SendNCCalcSize( winpos->hwnd, TRUE, &newWindowRect,
				    &wndPtr->rectWindow, &wndPtr->rectClient,
				    SEGPTR_GET(winpos), &newClientRect );

         /* FIXME: WVR_ALIGNxxx */

         if( newClientRect.left != wndPtr->rectClient.left ||
             newClientRect.top != wndPtr->rectClient.top )
             winpos->flags &= ~SWP_NOCLIENTMOVE;

         if( (newClientRect.right - newClientRect.left !=
             wndPtr->rectClient.right - wndPtr->rectClient.left) ||
  	    (newClientRect.bottom - newClientRect.top !=
	     wndPtr->rectClient.bottom - wndPtr->rectClient.top) )
	     winpos->flags &= ~SWP_NOCLIENTSIZE;
      }
    else
      if( !(flags & SWP_NOMOVE) && (newClientRect.left != wndPtr->rectClient.left ||
				    newClientRect.top != wndPtr->rectClient.top) )
	    winpos->flags &= ~SWP_NOCLIENTMOVE;

    /* Update active DCEs */

    if( !(flags & SWP_NOZORDER) || (flags & SWP_HIDEWINDOW) || (flags & SWP_SHOWWINDOW)
                                || (memcmp(&newWindowRect,&wndPtr->rectWindow,sizeof(RECT16))
                                    && wndPtr->dwStyle & WS_VISIBLE ) )
      {
        RECT16 rect;

        UnionRect16(&rect,&newWindowRect,&wndPtr->rectWindow);
        DCE_InvalidateDCE(wndPtr->parent, &rect);
      }

    /* change geometry */

    oldWindowRect = wndPtr->rectWindow;

    if (wndPtr->window)
    {
        RECT16 oldClientRect = wndPtr->rectClient;

        tempInsertAfter = winpos->hwndInsertAfter;

        winpos->hwndInsertAfter = hwndInsertAfter;

	/* postpone geometry change */

	if( !(flags & (SWP_SHOWWINDOW | SWP_HIDEWINDOW)) )
	  {
              WINPOS_SetXWindowPos( winpos );
	      winpos->hwndInsertAfter = tempInsertAfter;
	  }
	else  uFlags |= SMC_SETXPOS;

        wndPtr->rectWindow = newWindowRect;
        wndPtr->rectClient = newClientRect;

	if( !(flags & (SWP_SHOWWINDOW | SWP_HIDEWINDOW)) )
	  if( (oldClientRect.left - oldWindowRect.left !=
	       newClientRect.left - newWindowRect.left) ||
	      (oldClientRect.top - oldWindowRect.top !=
	       newClientRect.top - newWindowRect.top) || winpos->flags & SWP_NOCOPYBITS )

	      PAINT_RedrawWindow( wndPtr->hwndSelf, NULL, 0, RDW_INVALIDATE |
                              RDW_ALLCHILDREN | RDW_FRAME | RDW_ERASE, 0 );
	  else
	      if( winpos->flags & SWP_FRAMECHANGED )
	      {
		WORD wErase = 0;
		RECT32 rect;

	        if( oldClientRect.right > newClientRect.right ) 
                {
		    rect.left = newClientRect.right; rect.top = newClientRect.top;
		    rect.right = oldClientRect.right; rect.bottom = newClientRect.bottom;
		    wErase = 1;
		    PAINT_RedrawWindow( wndPtr->hwndSelf, &rect, 0,
                                      RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN, 0 );
                }
		if( oldClientRect.bottom > newClientRect.bottom )
                {
		    rect.left = newClientRect.left; rect.top = newClientRect.bottom;
		    rect.right = (wErase)?oldClientRect.right:newClientRect.right;
		    rect.bottom = oldClientRect.bottom;
		    wErase = 1;
		    PAINT_RedrawWindow( wndPtr->hwndSelf, &rect, 0,
                                      RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN, 0 );
                }
		if( !wErase ) wndPtr->flags |= WIN_NEEDS_NCPAINT;
	      }
    }
    else
    {
	RECT16 oldClientRect = wndPtr->rectClient;

        wndPtr->rectWindow = newWindowRect;
        wndPtr->rectClient = newClientRect;

	if( oldClientRect.bottom - oldClientRect.top ==
	    newClientRect.bottom - newClientRect.top ) result &= ~WVR_VREDRAW;

	if( oldClientRect.right - oldClientRect.left ==
	    newClientRect.right - newClientRect.left ) result &= ~WVR_HREDRAW;

        if( !(flags & (SWP_NOREDRAW | SWP_HIDEWINDOW | SWP_SHOWWINDOW)) )
	  {
	    uFlags |=  ((winpos->flags & SWP_NOCOPYBITS) || 
			(result >= WVR_HREDRAW && result < WVR_VALIDRECTS)) ? SMC_NOCOPY : 0;
	    uFlags |=  (winpos->flags & SWP_FRAMECHANGED) ? SMC_DRAWFRAME : 0;

	    if( (winpos->flags & SWP_AGG_NOGEOMETRYCHANGE) != SWP_AGG_NOGEOMETRYCHANGE )
		uFlags = WINPOS_SizeMoveClean(wndPtr, visRgn, &oldWindowRect, 
							      &oldClientRect, uFlags);
	    else
	      { 
		/* adjust frame and do not erase parent */

		if( winpos->flags & SWP_FRAMECHANGED ) wndPtr->flags |= WIN_NEEDS_NCPAINT;
		if( winpos->flags & SWP_NOZORDER ) uFlags |= SMC_NOPARENTERASE;
	      }
	  }
        DeleteObject32(visRgn);
    }

    if (flags & SWP_SHOWWINDOW)
    {
	wndPtr->dwStyle |= WS_VISIBLE;
        if (wndPtr->window)
        {
	    if( uFlags & SMC_SETXPOS )
	    {
              WINPOS_SetXWindowPos( winpos );
              winpos->hwndInsertAfter = tempInsertAfter;
	    }
            XMapWindow( display, wndPtr->window );
        }
        else
        {
            if (!(flags & SWP_NOREDRAW))
                PAINT_RedrawWindow( winpos->hwnd, NULL, 0,
                                RDW_INVALIDATE | RDW_ALLCHILDREN |
                                RDW_FRAME | RDW_ERASENOW | RDW_ERASE, 0 );
        }
    }
    else if (flags & SWP_HIDEWINDOW)
    {
	wndPtr->dwStyle &= ~WS_VISIBLE;
        if (wndPtr->window)
        {
            XUnmapWindow( display, wndPtr->window );
	    if( uFlags & SMC_SETXPOS )
	    {
              WINPOS_SetXWindowPos( winpos );
              winpos->hwndInsertAfter = tempInsertAfter;
	    }
        }
        else
        {
            if (!(flags & SWP_NOREDRAW))
	    {
	        RECT32 rect = { oldWindowRect.left, oldWindowRect.top,
				oldWindowRect.right, oldWindowRect.bottom };
                PAINT_RedrawWindow( wndPtr->parent->hwndSelf, &rect, 0, 
			RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE | RDW_ERASENOW, 0);
	    }
	    uFlags |= SMC_NOPARENTERASE;
        }

        if ((winpos->hwnd == GetFocus32()) ||
            IsChild32( winpos->hwnd, GetFocus32()))
        {
            /* Revert focus to parent */
            SetFocus32( GetParent32(winpos->hwnd) );
        }
	if (hwnd == CARET_GetHwnd()) DestroyCaret32();

	if (winpos->hwnd == hwndActive)
	{
	      /* Activate previously active window if possible */
	    HWND32 newActive = hwndPrevActive;
	    if (!IsWindow32(newActive) || (newActive == winpos->hwnd))
	    {
		newActive = GetTopWindow32( GetDesktopWindow32() );
		if (newActive == winpos->hwnd)
                    newActive = wndPtr->next ? wndPtr->next->hwndSelf : 0;
	    }	    
	    WINPOS_ChangeActiveWindow( newActive, FALSE );
	}
    }

      /* Activate the window */

    if (!(flags & SWP_NOACTIVATE))
	    WINPOS_ChangeActiveWindow( winpos->hwnd, FALSE );
    
      /* Repaint the window */

    if (wndPtr->window) EVENT_Synchronize();  /* Wait for all expose events */

    EVENT_DummyMotionNotify(); /* Simulate a mouse event to set the cursor */

    if (!(flags & SWP_DEFERERASE) && !(uFlags & SMC_NOPARENTERASE) )
    {
	RECT32	rect;
	CONV_RECT16TO32( &oldWindowRect, &rect );
        PAINT_RedrawWindow( wndPtr->parent->hwndSelf, (wndPtr->flags & WIN_SAVEUNDER_OVERRIDE) 
			    ? &rect : NULL, 0, RDW_ALLCHILDREN | RDW_ERASENOW |
			  ((wndPtr->flags & WIN_SAVEUNDER_OVERRIDE) ? RDW_INVALIDATE : 0), 0 );
        wndPtr->flags &= ~WIN_SAVEUNDER_OVERRIDE;
    }
    else if( wndPtr->parent == WIN_GetDesktop() && wndPtr->parent->flags & WIN_NEEDS_ERASEBKGND )
	PAINT_RedrawWindow( wndPtr->parent->hwndSelf, NULL, 0, RDW_NOCHILDREN | RDW_ERASENOW, 0 );

      /* And last, send the WM_WINDOWPOSCHANGED message */

    dprintf_win(stddeb,"\tstatus flags = %04x\n", winpos->flags & SWP_AGG_STATUSFLAGS);

    if ( ((winpos->flags & SWP_AGG_STATUSFLAGS) != SWP_AGG_NOPOSCHANGE) && 
	!(winpos->flags & SWP_NOSENDCHANGING))
        SendMessage16( winpos->hwnd, WM_WINDOWPOSCHANGED,
                       0, (LPARAM)SEGPTR_GET(winpos) );

    SEGPTR_FREE(winpos);
    return TRUE;
}

					
/***********************************************************************
 *           BeginDeferWindowPos16   (USER.259)
 */
HDWP16 BeginDeferWindowPos16( INT16 count )
{
    return BeginDeferWindowPos32( count );
}


/***********************************************************************
 *           BeginDeferWindowPos32   (USER32.8)
 */
HDWP32 BeginDeferWindowPos32( INT32 count )
{
    HDWP32 handle;
    DWP *pDWP;

    if (count <= 0) return 0;
    handle = USER_HEAP_ALLOC( sizeof(DWP) + (count-1)*sizeof(WINDOWPOS32) );
    if (!handle) return 0;
    pDWP = (DWP *) USER_HEAP_LIN_ADDR( handle );
    pDWP->actualCount    = 0;
    pDWP->suggestedCount = count;
    pDWP->valid          = TRUE;
    pDWP->wMagic         = DWP_MAGIC;
    pDWP->hwndParent     = 0;
    return handle;
}


/***********************************************************************
 *           DeferWindowPos16   (USER.260)
 */
HDWP16 DeferWindowPos16( HDWP16 hdwp, HWND16 hwnd, HWND16 hwndAfter,
                         INT16 x, INT16 y, INT16 cx, INT16 cy, UINT16 flags )
{
    return DeferWindowPos32( hdwp, hwnd, (INT32)(INT16)hwndAfter,
                             x, y, cx, cy, flags );
}


/***********************************************************************
 *           DeferWindowPos32   (USER32.127)
 */
HDWP32 DeferWindowPos32( HDWP32 hdwp, HWND32 hwnd, HWND32 hwndAfter,
                         INT32 x, INT32 y, INT32 cx, INT32 cy, UINT32 flags )
{
    DWP *pDWP;
    int i;
    HDWP32 newhdwp = hdwp;
    HWND32 parent;

    pDWP = (DWP *) USER_HEAP_LIN_ADDR( hdwp );
    if (!pDWP) return 0;
    if (hwnd == GetDesktopWindow32()) return 0;

      /* All the windows of a DeferWindowPos() must have the same parent */

    parent = WIN_FindWndPtr( hwnd )->parent->hwndSelf;
    if (pDWP->actualCount == 0) pDWP->hwndParent = parent;
    else if (parent != pDWP->hwndParent)
    {
        USER_HEAP_FREE( hdwp );
        return 0;
    }

    for (i = 0; i < pDWP->actualCount; i++)
    {
        if (pDWP->winPos[i].hwnd == hwnd)
        {
              /* Merge with the other changes */
            if (!(flags & SWP_NOZORDER))
            {
                pDWP->winPos[i].hwndInsertAfter = hwndAfter;
            }
            if (!(flags & SWP_NOMOVE))
            {
                pDWP->winPos[i].x = x;
                pDWP->winPos[i].y = y;
            }                
            if (!(flags & SWP_NOSIZE))
            {
                pDWP->winPos[i].cx = cx;
                pDWP->winPos[i].cy = cy;
            }
            pDWP->winPos[i].flags &= flags & (SWP_NOSIZE | SWP_NOMOVE |
                                              SWP_NOZORDER | SWP_NOREDRAW |
                                              SWP_NOACTIVATE | SWP_NOCOPYBITS |
                                              SWP_NOOWNERZORDER);
            pDWP->winPos[i].flags |= flags & (SWP_SHOWWINDOW | SWP_HIDEWINDOW |
                                              SWP_FRAMECHANGED);
            return hdwp;
        }
    }
    if (pDWP->actualCount >= pDWP->suggestedCount)
    {
        newhdwp = USER_HEAP_REALLOC( hdwp,
                      sizeof(DWP) + pDWP->suggestedCount*sizeof(WINDOWPOS32) );
        if (!newhdwp) return 0;
        pDWP = (DWP *) USER_HEAP_LIN_ADDR( newhdwp );
        pDWP->suggestedCount++;
    }
    pDWP->winPos[pDWP->actualCount].hwnd = hwnd;
    pDWP->winPos[pDWP->actualCount].hwndInsertAfter = hwndAfter;
    pDWP->winPos[pDWP->actualCount].x = x;
    pDWP->winPos[pDWP->actualCount].y = y;
    pDWP->winPos[pDWP->actualCount].cx = cx;
    pDWP->winPos[pDWP->actualCount].cy = cy;
    pDWP->winPos[pDWP->actualCount].flags = flags;
    pDWP->actualCount++;
    return newhdwp;
}


/***********************************************************************
 *           EndDeferWindowPos16   (USER.261)
 */
BOOL16 EndDeferWindowPos16( HDWP16 hdwp )
{
    return EndDeferWindowPos32( hdwp );
}


/***********************************************************************
 *           EndDeferWindowPos32   (USER32.172)
 */
BOOL32 EndDeferWindowPos32( HDWP32 hdwp )
{
    DWP *pDWP;
    WINDOWPOS32 *winpos;
    BOOL32 res = TRUE;
    int i;

    pDWP = (DWP *) USER_HEAP_LIN_ADDR( hdwp );
    if (!pDWP) return FALSE;
    for (i = 0, winpos = pDWP->winPos; i < pDWP->actualCount; i++, winpos++)
    {
        if (!(res = SetWindowPos32( winpos->hwnd, winpos->hwndInsertAfter,
                                    winpos->x, winpos->y, winpos->cx,
                                    winpos->cy, winpos->flags ))) break;
    }
    USER_HEAP_FREE( hdwp );
    return res;
}


/***********************************************************************
 *           TileChildWindows   (USER.199)
 */
void TileChildWindows( HWND16 parent, WORD action )
{
    printf("STUB TileChildWindows(%04x, %d)\n", parent, action);
}

/***********************************************************************
 *           CascageChildWindows   (USER.198)
 */
void CascadeChildWindows( HWND16 parent, WORD action )
{
    printf("STUB CascadeChildWindows(%04x, %d)\n", parent, action);
}
