/* MDI.C
 *
 * Copyright 1994, Bob Amstadt
 *           1995,1996 Alex Korobka
 *
 * This file contains routines to support MDI features.
 *
 * Notes: Fairly complete implementation. Any volunteers for 
 *	  "More windows..." stuff?
 *
 *        Also, Excel and WinWord do _not_ use MDI so if you're trying
 *	  to fix them look elsewhere. 
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "winuser.h"
#include "win.h"
#include "heap.h"
#include "nonclient.h"
#include "mdi.h"
#include "user.h"
#include "menu.h"
#include "resource.h"
#include "struct32.h"
#include "tweak.h"
#include "debug.h"

DEFAULT_DEBUG_CHANNEL(mdi)

#define MDIF_NEEDUPDATE		0x0001

static HBITMAP16 hBmpClose   = 0;
static HBITMAP16 hBmpRestore = 0;

INT SCROLL_SetNCSbState(WND*,int,int,int,int,int,int);

/* ----------------- declarations ----------------- */
static void MDI_UpdateFrameText(WND *, HWND, BOOL, LPCSTR);
static BOOL MDI_AugmentFrameMenu(MDICLIENTINFO*, WND *, HWND);
static BOOL MDI_RestoreFrameMenu(WND *, HWND);

static LONG MDI_ChildActivate( WND*, HWND );

/* -------- Miscellaneous service functions ----------
 *
 *			MDI_GetChildByID
 */

static HWND MDI_GetChildByID(WND* wndPtr, INT id)
{
    for (wndPtr = wndPtr->child; wndPtr; wndPtr = wndPtr->next)
        if (wndPtr->wIDmenu == id) return wndPtr->hwndSelf;
    return 0;
}

static void MDI_PostUpdate(HWND hwnd, MDICLIENTINFO* ci, WORD recalc)
{
    if( !(ci->mdiFlags & MDIF_NEEDUPDATE) )
    {
	ci->mdiFlags |= MDIF_NEEDUPDATE;
	PostMessageA( hwnd, WM_MDICALCCHILDSCROLL, 0, 0);
    }
    ci->sbRecalc = recalc;
}

/**********************************************************************
 *			MDI_MenuModifyItem
 */
static BOOL MDI_MenuModifyItem(WND* clientWnd, HWND hWndChild )
{
    char            buffer[128];
    MDICLIENTINFO  *clientInfo = (MDICLIENTINFO*)clientWnd->wExtra;
    WND            *wndPtr     = WIN_FindWndPtr(hWndChild);
    UINT	    n          = sprintf(buffer, "%d ",
				 wndPtr->wIDmenu - clientInfo->idFirstChild + 1);
    BOOL	    bRet	    = 0;

    if( !clientInfo->hWindowMenu )
    {
        bRet =  FALSE;
        goto END;
    }

    if (wndPtr->text) lstrcpynA(buffer + n, wndPtr->text, sizeof(buffer) - n );

    n    = GetMenuState(clientInfo->hWindowMenu,wndPtr->wIDmenu ,MF_BYCOMMAND); 
    bRet = ModifyMenuA(clientInfo->hWindowMenu , wndPtr->wIDmenu, 
                      MF_BYCOMMAND | MF_STRING, wndPtr->wIDmenu, buffer );
    CheckMenuItem(clientInfo->hWindowMenu ,wndPtr->wIDmenu , n & MF_CHECKED);
END:
    WIN_ReleaseWndPtr(wndPtr);
    return bRet;
}

/**********************************************************************
 *			MDI_MenuDeleteItem
 */
static BOOL MDI_MenuDeleteItem(WND* clientWnd, HWND hWndChild )
{
    char    	 buffer[128];
    MDICLIENTINFO *clientInfo = (MDICLIENTINFO*)clientWnd->wExtra;
    WND    	*wndPtr     = WIN_FindWndPtr(hWndChild);
    UINT	 index      = 0,id,n;
    BOOL         retvalue;

    if( !clientInfo->nActiveChildren ||
        !clientInfo->hWindowMenu )
    {
        retvalue = FALSE;
        goto END;
    }

    id = wndPtr->wIDmenu;
    DeleteMenu(clientInfo->hWindowMenu,id,MF_BYCOMMAND);

 /* walk the rest of MDI children to prevent gaps in the id 
  * sequence and in the menu child list */

    for( index = id+1; index <= clientInfo->nActiveChildren + 
				clientInfo->idFirstChild; index++ )
    {
        WND *tmpWnd = WIN_FindWndPtr(MDI_GetChildByID(clientWnd,index));
	if( !tmpWnd )
	{
	      TRACE(mdi,"no window for id=%i\n",index);
            WIN_ReleaseWndPtr(tmpWnd);
	      continue;
	}
    
	/* set correct id */
	tmpWnd->wIDmenu--;

	n = sprintf(buffer, "%d ",index - clientInfo->idFirstChild);
	if (tmpWnd->text)
            lstrcpynA(buffer + n, tmpWnd->text, sizeof(buffer) - n );	

	/* change menu */
	ModifyMenuA(clientInfo->hWindowMenu ,index ,MF_BYCOMMAND | MF_STRING,
                      index - 1 , buffer ); 
        WIN_ReleaseWndPtr(tmpWnd);
    }
    retvalue = TRUE;
END:
    WIN_ReleaseWndPtr(wndPtr);
    return retvalue;
}

/**********************************************************************
 * 			MDI_GetWindow
 *
 * returns "activateable" child different from the current or zero
 */
static HWND MDI_GetWindow(WND *clientWnd, HWND hWnd, BOOL bNext,
                            DWORD dwStyleMask )
{
    MDICLIENTINFO *clientInfo = (MDICLIENTINFO*)clientWnd->wExtra;
    WND *wndPtr, *pWnd, *pWndLast = NULL;
    
    dwStyleMask |= WS_DISABLED | WS_VISIBLE;
    if( !hWnd ) hWnd = clientInfo->hwndActiveChild;

    if( !(wndPtr = WIN_FindWndPtr(hWnd)) ) return 0;

    for ( pWnd = WIN_LockWndPtr(wndPtr->next); ; WIN_UpdateWndPtr(&pWnd,pWnd->next))
    {
        if (!pWnd ) WIN_UpdateWndPtr(&pWnd,wndPtr->parent->child);

        if ( pWnd == wndPtr ) break; /* went full circle */

        if (!pWnd->owner && (pWnd->dwStyle & dwStyleMask) == WS_VISIBLE )
        {
	    pWndLast = pWnd;
	    if ( bNext ) break;
        }
    }
    WIN_ReleaseWndPtr(wndPtr);
    WIN_ReleaseWndPtr(pWnd);
    return pWndLast ? pWndLast->hwndSelf : 0;
}

/**********************************************************************
 *			MDI_CalcDefaultChildPos
 *
 *  It seems that the default height is about 2/3 of the client rect
 */
static void MDI_CalcDefaultChildPos( WND* w, WORD n, LPPOINT lpPos,
                                     INT delta)
{
    INT  nstagger;
    RECT rect = w->rectClient;
    INT  spacing = GetSystemMetrics(SM_CYCAPTION) +
		     GetSystemMetrics(SM_CYFRAME) - 1; 

    if( rect.bottom - rect.top - delta >= spacing ) 
	rect.bottom -= delta;

    nstagger = (rect.bottom - rect.top)/(3 * spacing);
    lpPos[1].x = (rect.right - rect.left - nstagger * spacing);
    lpPos[1].y = (rect.bottom - rect.top - nstagger * spacing);
    lpPos[0].x = lpPos[0].y = spacing * (n%(nstagger+1));
}

/**********************************************************************
 *            MDISetMenu
 */
static LRESULT MDISetMenu( HWND hwnd, HMENU hmenuFrame,
                           HMENU hmenuWindow)
{
    WND *w = WIN_FindWndPtr(hwnd);
    MDICLIENTINFO *ci;
    HWND hwndFrame = GetParent(hwnd);
    HMENU oldFrameMenu = GetMenu(hwndFrame);

    TRACE(mdi, "%04x %04x %04x\n",
                hwnd, hmenuFrame, hmenuWindow);

    ci = (MDICLIENTINFO *) w->wExtra;

    if( ci->hwndChildMaximized && hmenuFrame && hmenuFrame!=oldFrameMenu )
        MDI_RestoreFrameMenu(w->parent, ci->hwndChildMaximized );

    if( hmenuWindow && hmenuWindow!=ci->hWindowMenu )
    {
        /* delete menu items from ci->hWindowMenu 
         * and add them to hmenuWindow */

        INT i = GetMenuItemCount(ci->hWindowMenu) - 1;
        INT pos = GetMenuItemCount(hmenuWindow) + 1;

        AppendMenuA( hmenuWindow, MF_SEPARATOR, 0, NULL);

        if( ci->nActiveChildren )
        {
            INT j = i - ci->nActiveChildren + 1;
            char buffer[100];
            UINT id,state;

            for( ; i >= j ; i-- )
            {
                id = GetMenuItemID(ci->hWindowMenu,i );
                state = GetMenuState(ci->hWindowMenu,i,MF_BYPOSITION); 

                GetMenuStringA(ci->hWindowMenu, i, buffer, 100, MF_BYPOSITION);

                DeleteMenu(ci->hWindowMenu, i , MF_BYPOSITION);
                InsertMenuA(hmenuWindow, pos, MF_BYPOSITION | MF_STRING,
                              id, buffer);
                CheckMenuItem(hmenuWindow ,pos , MF_BYPOSITION | (state & MF_CHECKED));
            }
        }

        /* remove separator */
        DeleteMenu(ci->hWindowMenu, i, MF_BYPOSITION); 

        ci->hWindowMenu = hmenuWindow;
    } 

    if( hmenuFrame && hmenuFrame!=oldFrameMenu)
    {
        SetMenu(hwndFrame, hmenuFrame);
        if( ci->hwndChildMaximized )
            MDI_AugmentFrameMenu(ci, w->parent, ci->hwndChildMaximized );
        WIN_ReleaseWndPtr(w);
        return oldFrameMenu;
    }
    WIN_ReleaseWndPtr(w);
    return 0;
}

/**********************************************************************
 *            MDIRefreshMenu
 */
static LRESULT MDIRefreshMenu( HWND hwnd, HMENU hmenuFrame,
                           HMENU hmenuWindow)
{
    HWND hwndFrame = GetParent(hwnd);
    HMENU oldFrameMenu = GetMenu(hwndFrame);

    TRACE(mdi, "%04x %04x %04x\n",
                hwnd, hmenuFrame, hmenuWindow);

    FIXME(mdi,"partially function stub\n");

    return oldFrameMenu;
}


/* ------------------ MDI child window functions ---------------------- */


/**********************************************************************
 *					MDICreateChild
 */
static HWND MDICreateChild( WND *w, MDICLIENTINFO *ci, HWND parent, 
                              LPMDICREATESTRUCTA cs )
{
    POINT          pos[2]; 
    DWORD	     style = cs->style | (WS_CHILD | WS_CLIPSIBLINGS);
    HWND 	     hwnd, hwndMax = 0;
    WORD	     wIDmenu = ci->idFirstChild + ci->nActiveChildren;
    char	     lpstrDef[]="junk!";

    TRACE(mdi, "origin %i,%i - dim %i,%i, style %08x\n", 
                cs->x, cs->y, cs->cx, cs->cy, (unsigned)cs->style);    
    /* calculate placement */
    MDI_CalcDefaultChildPos(w, ci->nTotalCreated++, pos, 0);

    if (cs->cx == CW_USEDEFAULT || !cs->cx) cs->cx = pos[1].x;
    if (cs->cy == CW_USEDEFAULT || !cs->cy) cs->cy = pos[1].y;

    if( cs->x == CW_USEDEFAULT )
    {
 	cs->x = pos[0].x;
	cs->y = pos[0].y;
    }

    /* restore current maximized child */
    if( style & WS_VISIBLE && ci->hwndChildMaximized )
    {
	if( style & WS_MAXIMIZE )
	    SendMessageA(w->hwndSelf, WM_SETREDRAW, FALSE, 0L );
	hwndMax = ci->hwndChildMaximized;
	ShowWindow( hwndMax, SW_SHOWNOACTIVATE );
	if( style & WS_MAXIMIZE )
	    SendMessageA(w->hwndSelf, WM_SETREDRAW, TRUE, 0L );
    }

    /* this menu is needed to set a check mark in MDI_ChildActivate */
    AppendMenuA(ci->hWindowMenu ,MF_STRING ,wIDmenu, lpstrDef );

    ci->nActiveChildren++;

    /* fix window style */
    if( !(w->dwStyle & MDIS_ALLCHILDSTYLES) )
    {
        style &= (WS_CHILD | WS_CLIPSIBLINGS | WS_MINIMIZE | WS_MAXIMIZE |
                  WS_CLIPCHILDREN | WS_DISABLED | WS_VSCROLL | WS_HSCROLL );
        style |= (WS_VISIBLE | WS_OVERLAPPEDWINDOW);
    }

    if( w->flags & WIN_ISWIN32 )
    {
	hwnd = CreateWindowA( cs->szClass, cs->szTitle, style, 
                                cs->x, cs->y, cs->cx, cs->cy, parent, 
                                (HMENU16)wIDmenu, cs->hOwner, cs );
    }
    else
    {
    	MDICREATESTRUCT16 *cs16;
        LPSTR title, cls;

        cs16 = SEGPTR_NEW(MDICREATESTRUCT16);
        STRUCT32_MDICREATESTRUCT32Ato16( cs, cs16 );
        title = SEGPTR_STRDUP( cs->szTitle );
        cls   = SEGPTR_STRDUP( cs->szClass );
        cs16->szTitle = SEGPTR_GET(title);
        cs16->szClass = SEGPTR_GET(cls);

	hwnd = CreateWindow16( cs->szClass, cs->szTitle, style, 
			       cs16->x, cs16->y, cs16->cx, cs16->cy, parent, 
			       (HMENU)wIDmenu, cs16->hOwner,
                               (LPVOID)SEGPTR_GET(cs16) );
        SEGPTR_FREE( title );
        SEGPTR_FREE( cls );
        SEGPTR_FREE( cs16 );
    }

    /* MDI windows are WS_CHILD so they won't be activated by CreateWindow */

    if (hwnd)
    {
	WND* wnd = WIN_FindWndPtr( hwnd );

	MDI_MenuModifyItem(w ,hwnd); 
	if( wnd->dwStyle & WS_MINIMIZE && ci->hwndActiveChild )
	    ShowWindow( hwnd, SW_SHOWMINNOACTIVE );
	else
	{
            /* WS_VISIBLE is clear if a) the MDI client has
             * MDIS_ALLCHILDSTYLES style and 2) the flag is cleared in the
             * MDICreateStruct. If so the created window is not shown nor 
             * activated.
             */
            int showflag=wnd->dwStyle & WS_VISIBLE;
            /* clear visible flag, otherwise SetWindoPos32 ignores
             * the SWP_SHOWWINDOW command.
             */
            wnd->dwStyle &= ~WS_VISIBLE;
            if(showflag){
                SetWindowPos( hwnd, 0, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE );

                /* Set maximized state here in case hwnd didn't receive WM_SIZE
                 * during CreateWindow - bad!
                 */

                if((wnd->dwStyle & WS_MAXIMIZE) && !ci->hwndChildMaximized )
                {
                    ci->hwndChildMaximized = wnd->hwndSelf;
                    MDI_AugmentFrameMenu( ci, w->parent, hwnd );
                    MDI_UpdateFrameText( w->parent, ci->self, MDI_REPAINTFRAME, NULL ); 
                }
            }else
                /* needed, harmless ? */
                SetWindowPos( hwnd, 0, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE );

	}
        WIN_ReleaseWndPtr(wnd);
        TRACE(mdi, "created child - %04x\n",hwnd);
    }
    else
    {
	ci->nActiveChildren--;
	DeleteMenu(ci->hWindowMenu,wIDmenu,MF_BYCOMMAND);
	if( IsWindow(hwndMax) )
	    ShowWindow(hwndMax, SW_SHOWMAXIMIZED);
    }
	
    return hwnd;
}

/**********************************************************************
 *			MDI_ChildGetMinMaxInfo
 *
 * Note: The rule here is that client rect of the maximized MDI child 
 *	 is equal to the client rect of the MDI client window.
 */
static void MDI_ChildGetMinMaxInfo( WND* clientWnd, HWND hwnd,
                                    MINMAXINFO16* lpMinMax )
{
    WND*	childWnd = WIN_FindWndPtr(hwnd);
    RECT	rect 	 = clientWnd->rectClient;

    MapWindowPoints( clientWnd->parent->hwndSelf, 
		     ((MDICLIENTINFO*)clientWnd->wExtra)->self, (LPPOINT)&rect, 2);
    AdjustWindowRectEx( &rect, childWnd->dwStyle, 0, childWnd->dwExStyle );

    lpMinMax->ptMaxSize.x = rect.right -= rect.left;
    lpMinMax->ptMaxSize.y = rect.bottom -= rect.top;

    lpMinMax->ptMaxPosition.x = rect.left;
    lpMinMax->ptMaxPosition.y = rect.top; 

    WIN_ReleaseWndPtr(childWnd);
    
    TRACE(mdi,"max rect (%i,%i - %i, %i)\n", 
                        rect.left,rect.top,rect.right,rect.bottom);
    
}

/**********************************************************************
 *			MDI_SwitchActiveChild
 * 
 * Note: SetWindowPos sends WM_CHILDACTIVATE to the child window that is
 *       being activated 
 */
static void MDI_SwitchActiveChild( HWND clientHwnd, HWND childHwnd,
                                   BOOL bNextWindow )
{
    WND		  *w	     = WIN_FindWndPtr(clientHwnd);
    HWND	   hwndTo    = 0;
    HWND	   hwndPrev  = 0;
    MDICLIENTINFO *ci;

    hwndTo = MDI_GetWindow(w, childHwnd, bNextWindow, 0);
 
    ci = (MDICLIENTINFO *) w->wExtra;

    TRACE(mdi, "from %04x, to %04x\n",childHwnd,hwndTo);

    if ( !hwndTo ) goto END; /* no window to switch to */

    hwndPrev = ci->hwndActiveChild;

    if ( hwndTo != hwndPrev )
    {
	BOOL bOptimize = 0;

	if( ci->hwndChildMaximized )
	{
	    bOptimize = 1; 
	    w->dwStyle &= ~WS_VISIBLE;
	}

	SetWindowPos( hwndTo, HWND_TOP, 0, 0, 0, 0, 
			SWP_NOMOVE | SWP_NOSIZE );

	if( bNextWindow && hwndPrev )
	    SetWindowPos( hwndPrev, HWND_BOTTOM, 0, 0, 0, 0, 
		  	    SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE );
	if( bOptimize )
	    ShowWindow( clientHwnd, SW_SHOW );
    }
END:
    WIN_ReleaseWndPtr(w);
}

	    
/**********************************************************************
 *                                      MDIDestroyChild
 */
static LRESULT MDIDestroyChild( WND *w_parent, MDICLIENTINFO *ci,
                                HWND parent, HWND child,
                                BOOL flagDestroy )
{
    WND         *childPtr = WIN_FindWndPtr(child);

    if( childPtr )
    {
        if( child == ci->hwndActiveChild )
        {
	    MDI_SwitchActiveChild(parent, child, TRUE);

	    if( child == ci->hwndActiveChild )
	    {
		ShowWindow( child, SW_HIDE);
		if( child == ci->hwndChildMaximized )
		{
		    MDI_RestoreFrameMenu(w_parent->parent, child);
		    ci->hwndChildMaximized = 0;
		    MDI_UpdateFrameText(w_parent->parent,parent,TRUE,NULL);
		}

                MDI_ChildActivate(w_parent, 0);
	    }
	    MDI_MenuDeleteItem(w_parent, child);
	}
        WIN_ReleaseWndPtr(childPtr);
	
        ci->nActiveChildren--;

        TRACE(mdi,"child destroyed - %04x\n",child);

        if (flagDestroy)
	{
	    MDI_PostUpdate(GetParent(child), ci, SB_BOTH+1);
            DestroyWindow(child);
	}
    }

    return 0;
}


/**********************************************************************
 *					MDI_ChildActivate
 *
 * Note: hWndChild is NULL when last child is being destroyed
 */
static LONG MDI_ChildActivate( WND *clientPtr, HWND hWndChild )
{
    MDICLIENTINFO       *clientInfo = (MDICLIENTINFO*)clientPtr->wExtra; 
    HWND               prevActiveWnd = clientInfo->hwndActiveChild;
    WND                 *wndPtr = WIN_FindWndPtr( hWndChild );
    WND			*wndPrev = WIN_FindWndPtr( prevActiveWnd );
    BOOL		 isActiveFrameWnd = 0;	 
    LONG               retvalue;

    if( hWndChild == prevActiveWnd )
    {
        retvalue = 0L;
        goto END;
    }

    if( wndPtr )
    {
        if( wndPtr->dwStyle & WS_DISABLED )
        {
            retvalue = 0L;
            goto END;
        }
    }

    TRACE(mdi,"%04x\n", hWndChild);

    if( GetActiveWindow() == clientPtr->parent->hwndSelf )
        isActiveFrameWnd = TRUE;
	
    /* deactivate prev. active child */
    if( wndPrev )
    {
	wndPrev->dwStyle |= WS_SYSMENU;
	SendMessageA( prevActiveWnd, WM_NCACTIVATE, FALSE, 0L );
        SendMessageA( prevActiveWnd, WM_MDIACTIVATE, (WPARAM)prevActiveWnd,
                        (LPARAM)hWndChild);
        /* uncheck menu item */
       	if( clientInfo->hWindowMenu )
       	        CheckMenuItem( clientInfo->hWindowMenu,
                                 wndPrev->wIDmenu, 0);
    }

    /* set appearance */
    if( clientInfo->hwndChildMaximized )
    {
      if( clientInfo->hwndChildMaximized != hWndChild ) {
        if( hWndChild ) {
		  clientInfo->hwndActiveChild = hWndChild;
		  ShowWindow( hWndChild, SW_SHOWMAXIMIZED);
	} else
		ShowWindow( clientInfo->hwndActiveChild, SW_SHOWNORMAL );
      }
    }

    clientInfo->hwndActiveChild = hWndChild;

    /* check if we have any children left */
    if( !hWndChild )
    {
	if( isActiveFrameWnd )
	    SetFocus( clientInfo->self );
        retvalue = 0;
        goto END;
    }
	
    /* check menu item */
    if( clientInfo->hWindowMenu )
              CheckMenuItem( clientInfo->hWindowMenu,
                               wndPtr->wIDmenu, MF_CHECKED);

    /* bring active child to the top */
    SetWindowPos( hWndChild, 0,0,0,0,0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);

    if( isActiveFrameWnd )
    {
	    SendMessageA( hWndChild, WM_NCACTIVATE, TRUE, 0L);
	    if( GetFocus() == clientInfo->self )
		SendMessageA( clientInfo->self, WM_SETFOCUS, 
                                (WPARAM)clientInfo->self, 0L );
	    else
		SetFocus( clientInfo->self );
    }
    SendMessageA( hWndChild, WM_MDIACTIVATE, (WPARAM)prevActiveWnd,
                    (LPARAM)hWndChild );
    retvalue = 1;
END:
    WIN_ReleaseWndPtr(wndPtr);
    WIN_ReleaseWndPtr(wndPrev);
    return retvalue;
}

/* -------------------- MDI client window functions ------------------- */

/**********************************************************************
 *				CreateMDIMenuBitmap
 */
static HBITMAP16 CreateMDIMenuBitmap(void)
{
 HDC 		hDCSrc  = CreateCompatibleDC(0);
 HDC		hDCDest	= CreateCompatibleDC(hDCSrc);
 HBITMAP16	hbClose = LoadBitmap16(0, MAKEINTRESOURCE16(OBM_CLOSE) );
 HBITMAP16	hbCopy;
 HANDLE16	hobjSrc, hobjDest;

 hobjSrc = SelectObject(hDCSrc, hbClose);
 hbCopy = CreateCompatibleBitmap(hDCSrc,GetSystemMetrics(SM_CXSIZE),GetSystemMetrics(SM_CYSIZE));
 hobjDest = SelectObject(hDCDest, hbCopy);

 BitBlt(hDCDest, 0, 0, GetSystemMetrics(SM_CXSIZE), GetSystemMetrics(SM_CYSIZE),
          hDCSrc, GetSystemMetrics(SM_CXSIZE), 0, SRCCOPY);
  
 SelectObject(hDCSrc, hobjSrc);
 DeleteObject(hbClose);
 DeleteDC(hDCSrc);

 hobjSrc = SelectObject( hDCDest, GetStockObject(BLACK_PEN) );

 MoveToEx( hDCDest, GetSystemMetrics(SM_CXSIZE) - 1, 0, NULL );
 LineTo( hDCDest, GetSystemMetrics(SM_CXSIZE) - 1, GetSystemMetrics(SM_CYSIZE) - 1);

 SelectObject(hDCDest, hobjSrc );
 SelectObject(hDCDest, hobjDest);
 DeleteDC(hDCDest);

 return hbCopy;
}

/**********************************************************************
 *				MDICascade
 */
static LONG MDICascade(WND* clientWnd, MDICLIENTINFO *ci)
{
    WND**	ppWnd;
    UINT	total;
  
    if (ci->hwndChildMaximized)
        SendMessageA( clientWnd->hwndSelf, WM_MDIRESTORE,
                        (WPARAM)ci->hwndChildMaximized, 0);

    if (ci->nActiveChildren == 0) return 0;

    if ((ppWnd = WIN_BuildWinArray(clientWnd, BWA_SKIPHIDDEN | BWA_SKIPOWNED | 
					      BWA_SKIPICONIC, &total)))
    {
	WND**	heapPtr = ppWnd;
	if( total )
	{
	    INT	delta = 0, n = 0;
	    POINT	pos[2];
	    if( total < ci->nActiveChildren )
		delta = GetSystemMetrics(SM_CYICONSPACING) +
			GetSystemMetrics(SM_CYICON);

	    /* walk the list (backwards) and move windows */
            while (*ppWnd) ppWnd++;
	    while (ppWnd != heapPtr)
	    {
                ppWnd--;
		TRACE(mdi, "move %04x to (%ld,%ld) size [%ld,%ld]\n", 
                            (*ppWnd)->hwndSelf, pos[0].x, pos[0].y, pos[1].x, pos[1].y);

		MDI_CalcDefaultChildPos(clientWnd, n++, pos, delta);
		SetWindowPos( (*ppWnd)->hwndSelf, 0, pos[0].x, pos[0].y,
                                pos[1].x, pos[1].y,
                                SWP_DRAWFRAME | SWP_NOACTIVATE | SWP_NOZORDER);
	    }
	}
	WIN_ReleaseWinArray(heapPtr);
    }

    if( total < ci->nActiveChildren )
        ArrangeIconicWindows( clientWnd->hwndSelf );
    return 0;
}

/**********************************************************************
 *					MDITile
 */
static void MDITile( WND* wndClient, MDICLIENTINFO *ci, WPARAM wParam )
{
    WND**	ppWnd;
    UINT	total = 0;

    if (ci->hwndChildMaximized)
        SendMessageA( wndClient->hwndSelf, WM_MDIRESTORE,
                        (WPARAM)ci->hwndChildMaximized, 0);

    if (ci->nActiveChildren == 0) return;

    ppWnd = WIN_BuildWinArray(wndClient, BWA_SKIPHIDDEN | BWA_SKIPOWNED | BWA_SKIPICONIC |
	    ((wParam & MDITILE_SKIPDISABLED)? BWA_SKIPDISABLED : 0), &total );

    TRACE(mdi,"%u windows to tile\n", total);

    if( ppWnd )
    {
	WND**	heapPtr = ppWnd;

	if( total )
	{
	    RECT	rect;
	    int		x, y, xsize, ysize;
	    int		rows, columns, r, c, i;

	    GetClientRect(wndClient->hwndSelf,&rect);
	    rows    = (int) sqrt((double)total);
	    columns = total / rows;

	    if( wParam & MDITILE_HORIZONTAL )  /* version >= 3.1 */
	    {
	        i = rows;
	        rows = columns;  /* exchange r and c */
	        columns = i;
	    }

	    if( total != ci->nActiveChildren)
	    {
	        y = rect.bottom - 2 * GetSystemMetrics(SM_CYICONSPACING) - GetSystemMetrics(SM_CYICON);
	        rect.bottom = ( y - GetSystemMetrics(SM_CYICON) < rect.top )? rect.bottom: y;
	    }

	    ysize   = rect.bottom / rows;
	    xsize   = rect.right  / columns;
    
	    for (x = i = 0, c = 1; c <= columns && *ppWnd; c++)
	    {
	        if (c == columns)
		{
		    rows  = total - i;
		    ysize = rect.bottom / rows;
		}

		y = 0;
		for (r = 1; r <= rows && *ppWnd; r++, i++)
		{
		    SetWindowPos((*ppWnd)->hwndSelf, 0, x, y, xsize, ysize, 
				   SWP_DRAWFRAME | SWP_NOACTIVATE | SWP_NOZORDER);
                    y += ysize;
		    ppWnd++;
		}
		x += xsize;
	    }
	}
	WIN_ReleaseWinArray(heapPtr);
    }
  
    if( total < ci->nActiveChildren ) ArrangeIconicWindows( wndClient->hwndSelf );
}

/* ----------------------- Frame window ---------------------------- */


/**********************************************************************
 *					MDI_AugmentFrameMenu
 */
static BOOL MDI_AugmentFrameMenu( MDICLIENTINFO* ci, WND *frame,
                                    HWND hChild )
{
    WND*	child = WIN_FindWndPtr(hChild);
    HMENU  	hSysPopup = 0;
  HBITMAP hSysMenuBitmap = 0;

    TRACE(mdi,"frame %p,child %04x\n",frame,hChild);

    if( !frame->wIDmenu || !child->hSysMenu )
    {
        WIN_ReleaseWndPtr(child);
        return 0;
    }
    WIN_ReleaseWndPtr(child);

    /* create a copy of sysmenu popup and insert it into frame menu bar */

    if (!(hSysPopup = LoadMenuA(GetModuleHandleA("USER32"), "SYSMENU")))
	return 0;
 
    TRACE(mdi,"\tgot popup %04x in sysmenu %04x\n", 
		hSysPopup, child->hSysMenu);
 
    AppendMenuA(frame->wIDmenu,MF_HELP | MF_BITMAP,
                   SC_MINIMIZE, (LPSTR)(DWORD)HBMMENU_MBAR_MINIMIZE ) ;
    AppendMenuA(frame->wIDmenu,MF_HELP | MF_BITMAP,
                   SC_RESTORE, (LPSTR)(DWORD)HBMMENU_MBAR_RESTORE );

  /* In Win 95 look, the system menu is replaced by the child icon */

  if(TWEAK_WineLook > WIN31_LOOK)
  {
    HICON hIcon = GetClassLongA(hChild, GCL_HICONSM);
    if (!hIcon)
      hIcon = GetClassLongA(hChild, GCL_HICON);
    if (hIcon)
    {
      HDC hMemDC;
      HBITMAP hBitmap, hOldBitmap;
      HBRUSH hBrush;
      HDC hdc = GetDC(hChild);

      if (hdc)
      {
        int cx, cy;
        cx = GetSystemMetrics(SM_CXSMICON);
        cy = GetSystemMetrics(SM_CYSMICON);
        hMemDC = CreateCompatibleDC(hdc);
        hBitmap = CreateCompatibleBitmap(hdc, cx, cy);
        hOldBitmap = SelectObject(hMemDC, hBitmap);
        SetMapMode(hMemDC, MM_TEXT);
        hBrush = CreateSolidBrush(GetSysColor(COLOR_MENU));
        DrawIconEx(hMemDC, 0, 0, hIcon, cx, cy, 0, hBrush, DI_NORMAL);
        SelectObject (hMemDC, hOldBitmap);
        DeleteObject(hBrush);
        DeleteDC(hMemDC);
        ReleaseDC(hChild, hdc);
        hSysMenuBitmap = hBitmap;
      }
    }
  }
  else
    hSysMenuBitmap = hBmpClose;

    if( !InsertMenuA(frame->wIDmenu,0,MF_BYPOSITION | MF_BITMAP | MF_POPUP,
                    hSysPopup, (LPSTR)(DWORD)hSysMenuBitmap))
    {  
        TRACE(mdi,"not inserted\n");
	DestroyMenu(hSysPopup); 
	return 0; 
    }

    /* The close button is only present in Win 95 look */
    if(TWEAK_WineLook > WIN31_LOOK)
    {
        AppendMenuA(frame->wIDmenu,MF_HELP | MF_BITMAP,
                       SC_CLOSE, (LPSTR)(DWORD)HBMMENU_MBAR_CLOSE );
    }

    EnableMenuItem(hSysPopup, SC_SIZE, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem(hSysPopup, SC_MOVE, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem(hSysPopup, SC_MAXIMIZE, MF_BYCOMMAND | MF_GRAYED);
    SetMenuDefaultItem(hSysPopup, SC_CLOSE, FALSE);

    /* redraw menu */
    DrawMenuBar(frame->hwndSelf);

    return 1;
}

/**********************************************************************
 *					MDI_RestoreFrameMenu
 */
static BOOL MDI_RestoreFrameMenu( WND *frameWnd, HWND hChild )
{
    INT nItems = GetMenuItemCount(frameWnd->wIDmenu) - 1;
    UINT iId = GetMenuItemID(frameWnd->wIDmenu,nItems) ;

    TRACE(mdi,"frameWnd %p,child %04x\n",frameWnd,hChild);

    if(!(iId == SC_RESTORE || iId == SC_CLOSE) )
	return 0; 

    /* app button */
    RemoveMenu(frameWnd->wIDmenu,0,MF_BYPOSITION);

    if(TWEAK_WineLook > WIN31_LOOK)
    {
        /* close */
        DeleteMenu(frameWnd->wIDmenu,GetMenuItemCount(frameWnd->wIDmenu) - 1,MF_BYPOSITION);
    }
    /* restore */
    DeleteMenu(frameWnd->wIDmenu,GetMenuItemCount(frameWnd->wIDmenu) - 1,MF_BYPOSITION);
    /* minimize */
    DeleteMenu(frameWnd->wIDmenu,GetMenuItemCount(frameWnd->wIDmenu) - 1,MF_BYPOSITION);

    DrawMenuBar(frameWnd->hwndSelf);

    return 1;
}


/**********************************************************************
 *				        MDI_UpdateFrameText
 *
 * used when child window is maximized/restored 
 *
 * Note: lpTitle can be NULL
 */
static void MDI_UpdateFrameText( WND *frameWnd, HWND hClient,
                                 BOOL repaint, LPCSTR lpTitle )
{
    char   lpBuffer[MDI_MAXTITLELENGTH+1];
    WND*   clientWnd = WIN_FindWndPtr(hClient);
    MDICLIENTINFO *ci = (MDICLIENTINFO *) clientWnd->wExtra;

    TRACE(mdi, "repaint %i, frameText %s\n", repaint, (lpTitle)?lpTitle:"NULL");

    if (!clientWnd)
           return;

    if (!ci)
    {
       WIN_ReleaseWndPtr(clientWnd);
           return;
    }

    /* store new "default" title if lpTitle is not NULL */
    if (lpTitle) 
    {
	if (ci->frameTitle) HeapFree( SystemHeap, 0, ci->frameTitle );
	ci->frameTitle = HEAP_strdupA( SystemHeap, 0, lpTitle );
    }

    if (ci->frameTitle)
    {
	WND* childWnd = WIN_FindWndPtr( ci->hwndChildMaximized );     

	if( childWnd && childWnd->text )
	{
	    /* combine frame title and child title if possible */

	    LPCSTR lpBracket  = " - [";
	    int	i_frame_text_length = strlen(ci->frameTitle);
	    int	i_child_text_length = strlen(childWnd->text);

	    lstrcpynA( lpBuffer, ci->frameTitle, MDI_MAXTITLELENGTH);

	    if( i_frame_text_length + 6 < MDI_MAXTITLELENGTH )
            {
		strcat( lpBuffer, lpBracket );

		if( i_frame_text_length + i_child_text_length + 6 < MDI_MAXTITLELENGTH )
		{
		    strcat( lpBuffer, childWnd->text );
		    strcat( lpBuffer, "]" );
		}
		else
		{
		    lstrcpynA( lpBuffer + i_frame_text_length + 4, 
				 childWnd->text, MDI_MAXTITLELENGTH - i_frame_text_length - 5 );
		    strcat( lpBuffer, "]" );
		}
	    }
	}
	else
	{
            strncpy(lpBuffer, ci->frameTitle, MDI_MAXTITLELENGTH );
	    lpBuffer[MDI_MAXTITLELENGTH]='\0';
	}
        WIN_ReleaseWndPtr(childWnd);

    }
    else
	lpBuffer[0] = '\0';

    DEFWND_SetText( frameWnd, lpBuffer );
    if( repaint == MDI_REPAINTFRAME)
	SetWindowPos( frameWnd->hwndSelf, 0,0,0,0,0, SWP_FRAMECHANGED |
			SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER );

    WIN_ReleaseWndPtr(clientWnd);

}


/* ----------------------------- Interface ---------------------------- */


/**********************************************************************
 *					MDIClientWndProc
 *
 * This function handles all MDI requests.
 */
LRESULT WINAPI MDIClientWndProc( HWND hwnd, UINT message, WPARAM wParam,
                                 LPARAM lParam )
{
    LPCREATESTRUCTA    cs;
    MDICLIENTINFO       *ci;
    RECT		 rect;
    WND                 *w 	  = WIN_FindWndPtr(hwnd);
    WND			*frameWnd = WIN_LockWndPtr(w->parent);
    INT nItems;
    LRESULT            retvalue;
    
    ci = (MDICLIENTINFO *) w->wExtra;
    
    switch (message)
    {
      case WM_CREATE:

	cs = (LPCREATESTRUCTA)lParam;

	/* Translation layer doesn't know what's in the cs->lpCreateParams
	 * so we have to keep track of what environment we're in. */

	if( w->flags & WIN_ISWIN32 )
	{
#define ccs ((LPCLIENTCREATESTRUCT)cs->lpCreateParams)
	    ci->hWindowMenu	= ccs->hWindowMenu;
	    ci->idFirstChild	= ccs->idFirstChild;
#undef ccs
	}
        else    
	{
	    LPCLIENTCREATESTRUCT16 ccs = (LPCLIENTCREATESTRUCT16) 
				   PTR_SEG_TO_LIN(cs->lpCreateParams);
	    ci->hWindowMenu	= ccs->hWindowMenu;
	    ci->idFirstChild	= ccs->idFirstChild;
	}

	ci->hwndChildMaximized  = 0;
	ci->nActiveChildren	= 0;
	ci->nTotalCreated	= 0;
	ci->frameTitle		= NULL;
	ci->mdiFlags		= 0;
	ci->self		= hwnd;
	w->dwStyle             |= WS_CLIPCHILDREN;

	if (!hBmpClose)
        {
            hBmpClose = CreateMDIMenuBitmap();
            hBmpRestore = LoadBitmap16( 0, MAKEINTRESOURCE16(OBM_RESTORE) );
        }
	MDI_UpdateFrameText(frameWnd, hwnd, MDI_NOFRAMEREPAINT,frameWnd->text);

	AppendMenuA( ci->hWindowMenu, MF_SEPARATOR, 0, NULL );

	GetClientRect(frameWnd->hwndSelf, &rect);
	NC_HandleNCCalcSize( w, &rect );
	w->rectClient = rect;

	TRACE(mdi,"Client created - hwnd = %04x, idFirst = %u\n",
			   hwnd, ci->idFirstChild );

        retvalue = 0;
        goto END;
      
      case WM_DESTROY:
	if( ci->hwndChildMaximized ) MDI_RestoreFrameMenu(w, frameWnd->hwndSelf);
	if((nItems = GetMenuItemCount(ci->hWindowMenu)) > 0) 
	{
    	    ci->idFirstChild = nItems - 1;
	    ci->nActiveChildren++; 		/* to delete a separator */
	    while( ci->nActiveChildren-- )
	        DeleteMenu(ci->hWindowMenu,MF_BYPOSITION,ci->idFirstChild--);
	}
        retvalue = 0;
        goto END;

      case WM_MDIACTIVATE:
        if( ci->hwndActiveChild != (HWND)wParam )
	    SetWindowPos((HWND)wParam, 0,0,0,0,0, SWP_NOSIZE | SWP_NOMOVE);
        retvalue = 0;
        goto END;

      case WM_MDICASCADE:
        retvalue = MDICascade(w, ci);
        goto END;

      case WM_MDICREATE:
        if (lParam) retvalue = MDICreateChild( w, ci, hwnd,
                                           (MDICREATESTRUCTA*)lParam );
        else retvalue = 0;
        goto END;

      case WM_MDIDESTROY:
	retvalue = MDIDestroyChild( w, ci, hwnd, (HWND)wParam, TRUE );
        goto END;

      case WM_MDIGETACTIVE:
          if (lParam) *(BOOL *)lParam = (ci->hwndChildMaximized > 0);
          retvalue = ci->hwndActiveChild;
          goto END;

      case WM_MDIICONARRANGE:
	ci->mdiFlags |= MDIF_NEEDUPDATE;
        ArrangeIconicWindows(hwnd);
	ci->sbRecalc = SB_BOTH+1;
	SendMessageA(hwnd, WM_MDICALCCHILDSCROLL, 0, 0L);
        retvalue = 0;
        goto END;
	
      case WM_MDIMAXIMIZE:
	ShowWindow( (HWND)wParam, SW_MAXIMIZE );
        retvalue = 0;
        goto END;

      case WM_MDINEXT: /* lParam != 0 means previous window */
	MDI_SwitchActiveChild(hwnd, (HWND)wParam, (lParam)? FALSE : TRUE );
	break;
	
      case WM_MDIRESTORE:
        SendMessageA( (HWND)wParam, WM_SYSCOMMAND, SC_RESTORE, 0);
        retvalue = 0;
        goto END;

      case WM_MDISETMENU:
          retvalue = MDISetMenu( hwnd, (HMENU)wParam, (HMENU)lParam );
	  goto END;
      case WM_MDIREFRESHMENU:
          retvalue = MDIRefreshMenu( hwnd, (HMENU)wParam, (HMENU)lParam );
          goto END;

      case WM_MDITILE:
	ci->mdiFlags |= MDIF_NEEDUPDATE;
	ShowScrollBar(hwnd,SB_BOTH,FALSE);
	MDITile(w, ci, wParam);
        ci->mdiFlags &= ~MDIF_NEEDUPDATE;
        retvalue = 0;
        goto END;

      case WM_VSCROLL:
      case WM_HSCROLL:
	ci->mdiFlags |= MDIF_NEEDUPDATE;
        ScrollChildren(hwnd, message, wParam, lParam);
	ci->mdiFlags &= ~MDIF_NEEDUPDATE;
        retvalue = 0;
        goto END;

      case WM_SETFOCUS:
	if( ci->hwndActiveChild )
	{
	   WND*	pw = WIN_FindWndPtr( ci->hwndActiveChild );
	   if( !(pw->dwStyle & WS_MINIMIZE) )
	       SetFocus( ci->hwndActiveChild );
	   WIN_ReleaseWndPtr(pw);
	} 
        retvalue = 0;
        goto END;
	
      case WM_NCACTIVATE:
        if( ci->hwndActiveChild )
	     SendMessageA(ci->hwndActiveChild, message, wParam, lParam);
	break;
	
      case WM_PARENTNOTIFY:
        if (LOWORD(wParam) == WM_LBUTTONDOWN)
        {
            POINT16  pt = MAKEPOINT16(lParam);
            HWND16 child = ChildWindowFromPoint16(hwnd, pt);

	    TRACE(mdi,"notification from %04x (%i,%i)\n",child,pt.x,pt.y);

            if( child && child != hwnd && child != ci->hwndActiveChild )
                SetWindowPos(child, 0,0,0,0,0, SWP_NOSIZE | SWP_NOMOVE );
        }
        retvalue = 0;
        goto END;

      case WM_SIZE:
        if( IsWindow(ci->hwndChildMaximized) )
	{
	    WND*	child = WIN_FindWndPtr(ci->hwndChildMaximized);
	    RECT	rect;

	    rect.left = 0;
	    rect.top = 0;
	    rect.right = LOWORD(lParam);
	    rect.bottom = HIWORD(lParam);

	    AdjustWindowRectEx(&rect, child->dwStyle, 0, child->dwExStyle);
	    MoveWindow(ci->hwndChildMaximized, rect.left, rect.top,
			 rect.right - rect.left, rect.bottom - rect.top, 1);
            WIN_ReleaseWndPtr(child);
	}
	else
	    MDI_PostUpdate(hwnd, ci, SB_BOTH+1);

	break;

      case WM_MDICALCCHILDSCROLL:
	if( (ci->mdiFlags & MDIF_NEEDUPDATE) && ci->sbRecalc )
	{
	    CalcChildScroll16(hwnd, ci->sbRecalc-1);
	    ci->sbRecalc = 0;
	    ci->mdiFlags &= ~MDIF_NEEDUPDATE;
	}
        retvalue = 0;
        goto END;
    }
    
    retvalue = DefWindowProcA( hwnd, message, wParam, lParam );
END:
    WIN_ReleaseWndPtr(w);
    WIN_ReleaseWndPtr(frameWnd);
    return retvalue;
}


/***********************************************************************
 *           DefFrameProc16   (USER.445)
 */
LRESULT WINAPI DefFrameProc16( HWND16 hwnd, HWND16 hwndMDIClient,
                               UINT16 message, WPARAM16 wParam, LPARAM lParam )
{
    HWND16	         childHwnd;
    MDICLIENTINFO*       ci;
    WND*                 wndPtr;

    if (hwndMDIClient)
    {
	switch (message)
	{
	  case WM_COMMAND:
	    wndPtr = WIN_FindWndPtr(hwndMDIClient);

            if (!wndPtr) {
               ERR(mdi,"null wndPtr for mdi window hwndMDIClient=%04x\n",
                             hwndMDIClient);
               return 0;
            } 

	    ci     = (MDICLIENTINFO*)wndPtr->wExtra;

	    /* check for possible syscommands for maximized MDI child */
            WIN_ReleaseWndPtr(wndPtr);

	    if( ci && (
	    	wParam <  ci->idFirstChild || 
		wParam >= ci->idFirstChild + ci->nActiveChildren
	    )){
		if( (wParam - 0xF000) & 0xF00F ) break;
		switch( wParam )
		  {
		    case SC_SIZE:
		    case SC_MOVE:
		    case SC_MINIMIZE:
		    case SC_MAXIMIZE:
		    case SC_NEXTWINDOW:
		    case SC_PREVWINDOW:
		    case SC_CLOSE:
		    case SC_RESTORE:
		       if( ci->hwndChildMaximized )
			   return SendMessage16( ci->hwndChildMaximized, WM_SYSCOMMAND,
					       wParam, lParam);
		  }
	      }
	    else
	      {
                wndPtr = WIN_FindWndPtr(hwndMDIClient);
                childHwnd = MDI_GetChildByID(wndPtr,wParam );
                WIN_ReleaseWndPtr(wndPtr);

 	    	if( childHwnd )
	            SendMessage16(hwndMDIClient, WM_MDIACTIVATE,
                                  (WPARAM16)childHwnd , 0L);
	      }
	    break;

	  case WM_NCACTIVATE:
	    SendMessage16(hwndMDIClient, message, wParam, lParam);
	    break;

	  case WM_SETTEXT:
            wndPtr = WIN_FindWndPtr(hwnd);
            MDI_UpdateFrameText(wndPtr, hwndMDIClient,
				      MDI_REPAINTFRAME, 
			             (LPCSTR)PTR_SEG_TO_LIN(lParam));
            WIN_ReleaseWndPtr(wndPtr);
	    return 0;
	
	  case WM_SETFOCUS:
	    SetFocus(hwndMDIClient);
	    break;

	  case WM_SIZE:
	    MoveWindow16(hwndMDIClient, 0, 0, 
		         LOWORD(lParam), HIWORD(lParam), TRUE);
	    break;

	  case WM_NEXTMENU:

            wndPtr = WIN_FindWndPtr(hwndMDIClient);
            ci     = (MDICLIENTINFO*)wndPtr->wExtra;

	    if( !(wndPtr->parent->dwStyle & WS_MINIMIZE) 
		&& ci->hwndActiveChild && !ci->hwndChildMaximized )
	    {
		/* control menu is between the frame system menu and 
		 * the first entry of menu bar */

		if( (wParam == VK_LEFT &&
		     wndPtr->parent->wIDmenu == LOWORD(lParam)) ||
		    (wParam == VK_RIGHT &&
		     GetSubMenu16(wndPtr->parent->hSysMenu, 0) == LOWORD(lParam)) )
		{
                    LRESULT retvalue;
                    WIN_ReleaseWndPtr(wndPtr);
		    wndPtr = WIN_FindWndPtr(ci->hwndActiveChild);
		    retvalue = MAKELONG( GetSubMenu16(wndPtr->hSysMenu, 0),
						  ci->hwndActiveChild);
                    WIN_ReleaseWndPtr(wndPtr);
                    return retvalue;
		}
	    }
            WIN_ReleaseWndPtr(wndPtr);
	    break;
	}
    }
    
    return DefWindowProc16(hwnd, message, wParam, lParam);
}


/***********************************************************************
 *           DefFrameProc32A   (USER32.122)
 */
LRESULT WINAPI DefFrameProcA( HWND hwnd, HWND hwndMDIClient,
                                UINT message, WPARAM wParam, LPARAM lParam)
{
    if (hwndMDIClient)
    {
	switch (message)
	{
	  case WM_COMMAND:
              return DefFrameProc16( hwnd, hwndMDIClient, message,
                                     (WPARAM16)wParam,
                              MAKELPARAM( (HWND16)lParam, HIWORD(wParam) ) );

	  case WM_NCACTIVATE:
	    SendMessageA(hwndMDIClient, message, wParam, lParam);
	    break;

	  case WM_SETTEXT: {
	  	LRESULT	ret;
		LPSTR	segstr = SEGPTR_STRDUP((LPSTR)lParam);

                ret = DefFrameProc16(hwnd, hwndMDIClient, message,
                                     wParam, (LPARAM)SEGPTR_GET(segstr) );
	        SEGPTR_FREE(segstr);
		return ret;
	  }
	
	  case WM_NEXTMENU:
	  case WM_SETFOCUS:
	  case WM_SIZE:
              return DefFrameProc16( hwnd, hwndMDIClient, message,
                                     wParam, lParam );
	}
    }
    
    return DefWindowProcA(hwnd, message, wParam, lParam);
}


/***********************************************************************
 *           DefFrameProc32W   (USER32.123)
 */
LRESULT WINAPI DefFrameProcW( HWND hwnd, HWND hwndMDIClient,
                                UINT message, WPARAM wParam, LPARAM lParam)
{
    if (hwndMDIClient)
    {
	switch (message)
	{
	  case WM_COMMAND:
              return DefFrameProc16( hwnd, hwndMDIClient, message,
                                     (WPARAM16)wParam,
                              MAKELPARAM( (HWND16)lParam, HIWORD(wParam) ) );

	  case WM_NCACTIVATE:
	    SendMessageW(hwndMDIClient, message, wParam, lParam);
	    break;

	  case WM_SETTEXT: 
	  {
	      LPSTR txt = HEAP_strdupWtoA(GetProcessHeap(),0,(LPWSTR)lParam);
	      LRESULT ret = DefFrameProcA( hwnd, hwndMDIClient, message,
                                     wParam, (DWORD)txt );
	      HeapFree(GetProcessHeap(),0,txt);
	      return ret;
	  }
	  case WM_NEXTMENU:
	  case WM_SETFOCUS:
	  case WM_SIZE:
              return DefFrameProcA( hwnd, hwndMDIClient, message,
                                      wParam, lParam );
	}
    }
    
    return DefWindowProcW( hwnd, message, wParam, lParam );
}


/***********************************************************************
 *           DefMDIChildProc16   (USER.447)
 */
LRESULT WINAPI DefMDIChildProc16( HWND16 hwnd, UINT16 message,
                                  WPARAM16 wParam, LPARAM lParam )
{
    MDICLIENTINFO       *ci;
    WND                 *clientWnd,*tmpWnd = 0;
    LRESULT             retvalue;

    clientWnd  = WIN_FindWndPtr(GetParent16(hwnd));
    ci         = (MDICLIENTINFO *) clientWnd->wExtra;

    switch (message)
    {
      case WM_SETTEXT:
	DefWindowProc16(hwnd, message, wParam, lParam);
	MDI_MenuModifyItem(clientWnd,hwnd);
	if( ci->hwndChildMaximized == hwnd )
	    MDI_UpdateFrameText( clientWnd->parent, ci->self,
				 MDI_REPAINTFRAME, NULL );
        retvalue = 0;
        goto END;

      case WM_CLOSE:
	SendMessage16(ci->self,WM_MDIDESTROY,(WPARAM16)hwnd,0L);
        retvalue = 0;
        goto END;

      case WM_SETFOCUS:
	if( ci->hwndActiveChild != hwnd )
	    MDI_ChildActivate(clientWnd, hwnd);
	break;

      case WM_CHILDACTIVATE:
	MDI_ChildActivate(clientWnd, hwnd);
        retvalue = 0;
        goto END;

      case WM_NCPAINT:
	TRACE(mdi,"WM_NCPAINT for %04x, active %04x\n",
					     hwnd, ci->hwndActiveChild );
	break;

      case WM_SYSCOMMAND:
	switch( wParam )
	{
		case SC_MOVE:
                     if( ci->hwndChildMaximized == hwnd)
                     {
                         retvalue = 0;
                         goto END;
                     }
		     break;
	        case SC_RESTORE:
	        case SC_MINIMIZE:
                     tmpWnd = WIN_FindWndPtr(hwnd);
                     tmpWnd->dwStyle |= WS_SYSMENU;
                     WIN_ReleaseWndPtr(tmpWnd);
		     break;
		case SC_MAXIMIZE:
		     if( ci->hwndChildMaximized == hwnd) 
                     {
		          retvalue = SendMessage16( clientWnd->parent->hwndSelf,
                                             message, wParam, lParam);
                          goto END;
                     }
                     tmpWnd = WIN_FindWndPtr(hwnd);
                     tmpWnd->dwStyle &= ~WS_SYSMENU;
                     WIN_ReleaseWndPtr(tmpWnd);
		     break;
		case SC_NEXTWINDOW:
		     SendMessage16( ci->self, WM_MDINEXT, 0, 0);
                     retvalue = 0;
                     goto END;
		case SC_PREVWINDOW:
		     SendMessage16( ci->self, WM_MDINEXT, 0, 1);
                     retvalue = 0;
                     goto END;
	}
	break;
	
      case WM_GETMINMAXINFO:
	MDI_ChildGetMinMaxInfo(clientWnd, hwnd, (MINMAXINFO16*) PTR_SEG_TO_LIN(lParam));
        retvalue = 0;
        goto END;

      case WM_SETVISIBLE:
         if( ci->hwndChildMaximized) ci->mdiFlags &= ~MDIF_NEEDUPDATE;
	 else
            MDI_PostUpdate(clientWnd->hwndSelf, ci, SB_BOTH+1);
	break;

      case WM_SIZE:
	/* do not change */

	if( ci->hwndActiveChild == hwnd && wParam != SIZE_MAXIMIZED )
	{
  	    ci->hwndChildMaximized = 0;
	    
	    MDI_RestoreFrameMenu( clientWnd->parent, hwnd);
            MDI_UpdateFrameText( clientWnd->parent, ci->self,
                                 MDI_REPAINTFRAME, NULL );
	}

	if( wParam == SIZE_MAXIMIZED )
	{
	    HWND16 hMaxChild = ci->hwndChildMaximized;

	    if( hMaxChild == hwnd ) break;

	    if( hMaxChild)
	    {	    
	        SendMessage16( hMaxChild, WM_SETREDRAW, FALSE, 0L );

	        MDI_RestoreFrameMenu( clientWnd->parent, hMaxChild);
	        ShowWindow16( hMaxChild, SW_SHOWNOACTIVATE);

	        SendMessage16( hMaxChild, WM_SETREDRAW, TRUE, 0L );
	    }

	    TRACE(mdi,"maximizing child %04x\n", hwnd );

	    ci->hwndChildMaximized = hwnd; /* !!! */
	    ci->hwndActiveChild = hwnd;

	    MDI_AugmentFrameMenu( ci, clientWnd->parent, hwnd);
	    MDI_UpdateFrameText( clientWnd->parent, ci->self,
				 MDI_REPAINTFRAME, NULL ); 
	}

	if( wParam == SIZE_MINIMIZED )
	{
	    HWND16 switchTo = MDI_GetWindow(clientWnd, hwnd, TRUE, WS_MINIMIZE);

	    if( switchTo )
	        SendMessage16( switchTo, WM_CHILDACTIVATE, 0, 0L);
	}
	 
	MDI_PostUpdate(clientWnd->hwndSelf, ci, SB_BOTH+1);
	break;

      case WM_MENUCHAR:

	/* MDI children don't have menu bars */
	PostMessage16( clientWnd->parent->hwndSelf, WM_SYSCOMMAND, 
                       (WPARAM16)SC_KEYMENU, (LPARAM)wParam);
        retvalue = 0x00010000L;
        goto END;

      case WM_NEXTMENU:

	if( wParam == VK_LEFT )		/* switch to frame system menu */
        {
            retvalue = MAKELONG( GetSubMenu16(clientWnd->parent->hSysMenu, 0),
			   clientWnd->parent->hwndSelf );
            goto END;
        }
	if( wParam == VK_RIGHT )	/* to frame menu bar */
        {
            retvalue = MAKELONG( clientWnd->parent->wIDmenu,
			   clientWnd->parent->hwndSelf );
            goto END;
        }

	break;	

      case WM_SYSCHAR:
      	   if (wParam == '-')
	   {
	   	SendMessage16(hwnd,WM_SYSCOMMAND,
			(WPARAM16)SC_KEYMENU, (LPARAM)(DWORD)VK_SPACE);
                retvalue = 0;
                goto END;
	   }
    }
	
    retvalue = DefWindowProc16(hwnd, message, wParam, lParam);
END:
    WIN_ReleaseWndPtr(clientWnd);
    return retvalue;
}


/***********************************************************************
 *           DefMDIChildProc32A   (USER32.124)
 */
LRESULT WINAPI DefMDIChildProcA( HWND hwnd, UINT message,
                                   WPARAM wParam, LPARAM lParam )
{
    MDICLIENTINFO       *ci;
    WND                 *clientWnd,*tmpWnd;
    LRESULT             retvalue;

    tmpWnd     = WIN_FindWndPtr(hwnd);
    clientWnd  = WIN_FindWndPtr(tmpWnd->parent->hwndSelf);
    ci         = (MDICLIENTINFO *) clientWnd->wExtra;
    WIN_ReleaseWndPtr(tmpWnd);

    switch (message)
    {
      case WM_SETTEXT:
	DefWindowProcA(hwnd, message, wParam, lParam);
	MDI_MenuModifyItem(clientWnd,hwnd);
	if( ci->hwndChildMaximized == hwnd )
	    MDI_UpdateFrameText( clientWnd->parent, ci->self,
				 MDI_REPAINTFRAME, NULL );
        retvalue = 0;
        goto END;

      case WM_GETMINMAXINFO:
        {
            MINMAXINFO16 mmi;
            STRUCT32_MINMAXINFO32to16( (MINMAXINFO *)lParam, &mmi );
            MDI_ChildGetMinMaxInfo( clientWnd, hwnd, &mmi );
            STRUCT32_MINMAXINFO16to32( &mmi, (MINMAXINFO *)lParam );
        }
        retvalue = 0;
        goto END;

      case WM_MENUCHAR:

	/* MDI children don't have menu bars */
	PostMessage16( clientWnd->parent->hwndSelf, WM_SYSCOMMAND, 
                       (WPARAM16)SC_KEYMENU, (LPARAM)LOWORD(wParam) );
        retvalue = 0x00010000L;
        goto END;

      case WM_CLOSE:
      case WM_SETFOCUS:
      case WM_CHILDACTIVATE:
      case WM_NCPAINT:
      case WM_SYSCOMMAND:
      case WM_SETVISIBLE:
      case WM_SIZE:
      case WM_NEXTMENU:
          retvalue = DefMDIChildProc16( hwnd, message, (WPARAM16)wParam, lParam );
          goto END;

      case WM_SYSCHAR:
      	   if (wParam == '-')
	   {
	   	SendMessageA(hwnd,WM_SYSCOMMAND,
			(WPARAM)SC_KEYMENU, (LPARAM)(DWORD)VK_SPACE);
                retvalue = 0;
                goto END;
	   }
    }
    retvalue = DefWindowProcA(hwnd, message, wParam, lParam);
END:
    WIN_ReleaseWndPtr(clientWnd);
    return retvalue;
}


/***********************************************************************
 *           DefMDIChildProc32W   (USER32.125)
 */
LRESULT WINAPI DefMDIChildProcW( HWND hwnd, UINT message,
                                   WPARAM wParam, LPARAM lParam )
{
    MDICLIENTINFO       *ci;
    WND                 *clientWnd;
    LRESULT             retvalue;

    clientWnd  = WIN_FindWndPtr(GetParent16(hwnd));
    ci         = (MDICLIENTINFO *) clientWnd->wExtra;

    switch (message)
    {
      case WM_SETTEXT:
	DefWindowProcW(hwnd, message, wParam, lParam);
	MDI_MenuModifyItem(clientWnd,hwnd);
	if( ci->hwndChildMaximized == hwnd )
	    MDI_UpdateFrameText( clientWnd->parent, ci->self,
				 MDI_REPAINTFRAME, NULL );
        retvalue = 0;
        goto END;

      case WM_GETMINMAXINFO:
      case WM_MENUCHAR:
      case WM_CLOSE:
      case WM_SETFOCUS:
      case WM_CHILDACTIVATE:
      case WM_NCPAINT:
      case WM_SYSCOMMAND:
      case WM_SETVISIBLE:
      case WM_SIZE:
      case WM_NEXTMENU:
          retvalue = DefMDIChildProcA( hwnd, message, (WPARAM16)wParam, lParam );
          goto END;

      case WM_SYSCHAR:
      	   if (wParam == '-')
	   {
	   	SendMessageW(hwnd,WM_SYSCOMMAND,
			(WPARAM)SC_KEYMENU, (LPARAM)(DWORD)VK_SPACE);
                retvalue = 0;
                goto END;
	   }
    }
    retvalue = DefWindowProcW(hwnd, message, wParam, lParam);
END:
    WIN_ReleaseWndPtr(clientWnd);
    return retvalue;
    
}


/**********************************************************************
 * CreateMDIWindowA [USER32.79] Creates a MDI child in new thread
 * FIXME: its in the same thread now
 *
 * RETURNS
 *    Success: Handle to created window
 *    Failure: NULL
 */
HWND WINAPI CreateMDIWindowA(
    LPCSTR lpClassName,    /* [in] Pointer to registered child class name */
    LPCSTR lpWindowName,   /* [in] Pointer to window name */
    DWORD dwStyle,         /* [in] Window style */
    INT X,               /* [in] Horizontal position of window */
    INT Y,               /* [in] Vertical position of window */
    INT nWidth,          /* [in] Width of window */
    INT nHeight,         /* [in] Height of window */
    HWND hWndParent,     /* [in] Handle to parent window */
    HINSTANCE hInstance, /* [in] Handle to application instance */
    LPARAM lParam)         /* [in] Application-defined value */
{
    WARN(mdi,"is only single threaded!\n");
    return MDI_CreateMDIWindowA(lpClassName, lpWindowName, dwStyle, X, Y, 
            nWidth, nHeight, hWndParent, hInstance, lParam);
}
 
/**********************************************************************
 * MDI_CreateMDIWindowA 
 * single threaded version of CreateMDIWindowA
 * called by CreateWindowEx32A
 */
HWND MDI_CreateMDIWindowA(
    LPCSTR lpClassName,
    LPCSTR lpWindowName,
    DWORD dwStyle,
    INT X,
    INT Y,
    INT nWidth,
    INT nHeight,
    HWND hWndParent,
    HINSTANCE hInstance,
    LPARAM lParam)
{
    MDICLIENTINFO* pCi;
    MDICREATESTRUCTA cs;
    WND *pWnd=WIN_FindWndPtr(hWndParent);
    HWND retvalue;

    TRACE(mdi, "(%s,%s,%ld,%d,%d,%d,%d,%x,%d,%ld)\n",
          debugstr_a(lpClassName),debugstr_a(lpWindowName),dwStyle,X,Y,
          nWidth,nHeight,hWndParent,hInstance,lParam);

    if(!pWnd){
        ERR(mdi," bad hwnd for MDI-client: %d\n",hWndParent);
        return 0;
    }
    cs.szClass=lpClassName;
    cs.szTitle=lpWindowName;
    cs.hOwner=hInstance;
    cs.x=X;
    cs.y=Y;
    cs.cx=nWidth;
    cs.cy=nHeight;
    cs.style=dwStyle;
    cs.lParam=lParam;

    pCi=(MDICLIENTINFO *)pWnd->wExtra;
    
    retvalue = MDICreateChild(pWnd,pCi,hWndParent,&cs);
    WIN_ReleaseWndPtr(pWnd);
    return retvalue;
}

/***************************************
 * CreateMDIWindow32W [USER32.80] Creates a MDI child in new thread
 *
 * RETURNS
 *    Success: Handle to created window
 *    Failure: NULL
 */
HWND WINAPI CreateMDIWindowW(
    LPCWSTR lpClassName,    /* [in] Pointer to registered child class name */
    LPCWSTR lpWindowName,   /* [in] Pointer to window name */
    DWORD dwStyle,         /* [in] Window style */
    INT X,               /* [in] Horizontal position of window */
    INT Y,               /* [in] Vertical position of window */
    INT nWidth,          /* [in] Width of window */
    INT nHeight,         /* [in] Height of window */
    HWND hWndParent,     /* [in] Handle to parent window */
    HINSTANCE hInstance, /* [in] Handle to application instance */
    LPARAM lParam)         /* [in] Application-defined value */
{
    FIXME(mdi, "(%s,%s,%ld,%d,%d,%d,%d,%x,%d,%ld): stub\n",
          debugstr_w(lpClassName),debugstr_w(lpWindowName),dwStyle,X,Y,
          nWidth,nHeight,hWndParent,hInstance,lParam);
    return (HWND)NULL;
}


/******************************************************************************
 * CreateMDIWindow32W [USER32.80]  Creates a MDI child window
 * single threaded version of CreateMDIWindow
 * called by CreateWindowEx32W(). 
 */
HWND MDI_CreateMDIWindowW(
    LPCWSTR lpClassName,   /* [in] Pointer to registered child class name */
    LPCWSTR lpWindowName,  /* [in] Pointer to window name */
    DWORD dwStyle,         /* [in] Window style */
    INT X,               /* [in] Horizontal position of window */
    INT Y,               /* [in] Vertical position of window */
    INT nWidth,          /* [in] Width of window */
    INT nHeight,         /* [in] Height of window */
    HWND hWndParent,     /* [in] Handle to parent window */
    HINSTANCE hInstance, /* [in] Handle to application instance */
    LPARAM lParam)         /* [in] Application-defined value */
{
    FIXME(mdi, "(%s,%s,%ld,%d,%d,%d,%d,%x,%d,%ld): stub\n",
          debugstr_w(lpClassName),debugstr_w(lpWindowName),dwStyle,X,Y,
          nWidth,nHeight,hWndParent,hInstance,lParam);
    return (HWND)NULL;
}


/**********************************************************************
 *             TranslateMDISysAccel32   (USER32.555)
 */
BOOL WINAPI TranslateMDISysAccel( HWND hwndClient, LPMSG msg )
{
    MSG16 msg16;
 
    STRUCT32_MSG32to16(msg,&msg16);
    /* MDICLIENTINFO is still the same for win32 and win16 ... */
    return TranslateMDISysAccel16(hwndClient,&msg16);
}


/**********************************************************************
 *             TranslateMDISysAccel16   (USER.451)
 */
BOOL16 WINAPI TranslateMDISysAccel16( HWND16 hwndClient, LPMSG16 msg )
{

    if( IsWindow(hwndClient) && (msg->message == WM_KEYDOWN || msg->message == WM_SYSKEYDOWN))
    {
	MDICLIENTINFO	*ci = NULL;
	HWND		wnd;
        WND             *clientWnd = WIN_FindWndPtr(hwndClient);

	ci = (MDICLIENTINFO*) clientWnd->wExtra;
        wnd = ci->hwndActiveChild;

        WIN_ReleaseWndPtr(clientWnd);

        if( IsWindow(wnd) && !(GetWindowLongA(wnd,GWL_STYLE) & WS_DISABLED) )
	{
	    WPARAM16	wParam = 0;

	    /* translate if the Ctrl key is down and Alt not. */
  
	    if( (GetKeyState(VK_CONTROL) & 0x8000) && 
	       !(GetKeyState(VK_MENU) & 0x8000))
	    {
		switch( msg->wParam )
		{
		    case VK_F6:
		    case VK_TAB:
			 wParam = ( GetKeyState(VK_SHIFT) & 0x8000 )
				  ? SC_NEXTWINDOW : SC_PREVWINDOW;
			 break;
		    case VK_F4:
		    case VK_RBUTTON:
			 wParam = SC_CLOSE; 
			 break;
		    default:
			 return 0;
		}
	        TRACE(mdi,"wParam = %04x\n", wParam);
	        SendMessage16( ci->hwndActiveChild, WM_SYSCOMMAND, 
					wParam, (LPARAM)msg->wParam);
	        return 1;
	    }
	}
    }
    return 0; /* failure */
}


/***********************************************************************
 *           CalcChildScroll   (USER.462)
 */
void WINAPI CalcChildScroll16( HWND16 hwnd, WORD scroll )
{
    SCROLLINFO info;
    RECT childRect, clientRect;
    INT  vmin, vmax, hmin, hmax, vpos, hpos;
    WND *pWnd, *Wnd;

    if (!(pWnd = WIN_FindWndPtr( hwnd ))) return;
    Wnd = WIN_FindWndPtr(hwnd);
    GetClientRect( hwnd, &clientRect );
    SetRectEmpty( &childRect );

    for ( WIN_UpdateWndPtr(&pWnd,pWnd->child); pWnd; WIN_UpdateWndPtr(&pWnd,pWnd->next))
    {
	  if( pWnd->dwStyle & WS_MAXIMIZE )
	  {
	      ShowScrollBar(hwnd, SB_BOTH, FALSE);
              WIN_ReleaseWndPtr(pWnd);
              WIN_ReleaseWndPtr(Wnd);
	      return;
	  }
	  UnionRect( &childRect, &pWnd->rectWindow, &childRect );
    } 
    WIN_ReleaseWndPtr(pWnd);
    UnionRect( &childRect, &clientRect, &childRect );

    hmin = childRect.left; hmax = childRect.right - clientRect.right;
    hpos = clientRect.left - childRect.left;
    vmin = childRect.top; vmax = childRect.bottom - clientRect.bottom;
    vpos = clientRect.top - childRect.top;

    switch( scroll )
    {
	case SB_HORZ:
			vpos = hpos; vmin = hmin; vmax = hmax;
	case SB_VERT:
			info.cbSize = sizeof(info);
			info.nMax = vmax; info.nMin = vmin; info.nPos = vpos;
			info.fMask = SIF_POS | SIF_RANGE;
			SetScrollInfo(hwnd, scroll, &info, TRUE);
			break;
	case SB_BOTH:
			SCROLL_SetNCSbState( Wnd, vmin, vmax, vpos,
						  hmin, hmax, hpos);
    }    
    WIN_ReleaseWndPtr(Wnd);
}


/***********************************************************************
 *           ScrollChildren16   (USER.463)
 */
void WINAPI ScrollChildren16(HWND16 hWnd, UINT16 uMsg, WPARAM16 wParam, LPARAM lParam)
{
    ScrollChildren( hWnd, uMsg, wParam, lParam );
}


/***********************************************************************
 *           ScrollChildren32   (USER32.448)
 */
void WINAPI ScrollChildren(HWND hWnd, UINT uMsg, WPARAM wParam,
                             LPARAM lParam)
{
    WND	*wndPtr = WIN_FindWndPtr(hWnd);
    INT newPos = -1;
    INT curPos, length, minPos, maxPos, shift;

    if( !wndPtr ) return;

    if( uMsg == WM_HSCROLL )
    {
	GetScrollRange(hWnd,SB_HORZ,&minPos,&maxPos);
	curPos = GetScrollPos(hWnd,SB_HORZ);
	length = (wndPtr->rectClient.right - wndPtr->rectClient.left)/2;
	shift = GetSystemMetrics(SM_CYHSCROLL);
    }
    else if( uMsg == WM_VSCROLL )
    {
	GetScrollRange(hWnd,SB_VERT,&minPos,&maxPos);
	curPos = GetScrollPos(hWnd,SB_VERT);
	length = (wndPtr->rectClient.bottom - wndPtr->rectClient.top)/2;
	shift = GetSystemMetrics(SM_CXVSCROLL);
    }
    else
    {
        WIN_ReleaseWndPtr(wndPtr);
        return;
    }

    WIN_ReleaseWndPtr(wndPtr);
    switch( wParam )
    {
	case SB_LINEUP:	
		        newPos = curPos - shift;
			break;
	case SB_LINEDOWN:    
			newPos = curPos + shift;
			break;
	case SB_PAGEUP:	
			newPos = curPos - length;
			break;
	case SB_PAGEDOWN:    
			newPos = curPos + length;
			break;

	case SB_THUMBPOSITION: 
			newPos = LOWORD(lParam);
			break;

	case SB_THUMBTRACK:  
			return;

	case SB_TOP:		
			newPos = minPos;
			break;
	case SB_BOTTOM:	
			newPos = maxPos;
			break;
	case SB_ENDSCROLL:
			CalcChildScroll16(hWnd,(uMsg == WM_VSCROLL)?SB_VERT:SB_HORZ);
			return;
    }

    if( newPos > maxPos )
	newPos = maxPos;
    else 
	if( newPos < minPos )
	    newPos = minPos;

    SetScrollPos(hWnd, (uMsg == WM_VSCROLL)?SB_VERT:SB_HORZ , newPos, TRUE);

    if( uMsg == WM_VSCROLL )
	ScrollWindowEx(hWnd ,0 ,curPos - newPos, NULL, NULL, 0, NULL, 
			SW_INVALIDATE | SW_ERASE | SW_SCROLLCHILDREN );
    else
	ScrollWindowEx(hWnd ,curPos - newPos, 0, NULL, NULL, 0, NULL,
			SW_INVALIDATE | SW_ERASE | SW_SCROLLCHILDREN );
}


/******************************************************************************
 * CascadeWindows [USER32.21] Cascades MDI child windows
 *
 * RETURNS
 *    Success: Number of cascaded windows.
 *    Failure: 0
 */
WORD WINAPI
CascadeWindows (HWND hwndParent, UINT wFlags, const LPRECT lpRect,
		UINT cKids, const HWND *lpKids)
{
    FIXME (mdi, "(0x%08x,0x%08x,...,%u,...): stub\n",
	   hwndParent, wFlags, cKids);

    return 0;
}


/******************************************************************************
 * TileWindows [USER32.545] Tiles MDI child windows
 *
 * RETURNS
 *    Success: Number of tiled windows.
 *    Failure: 0
 */
WORD WINAPI
TileWindows (HWND hwndParent, UINT wFlags, const LPRECT lpRect,
	     UINT cKids, const HWND *lpKids)
{
    FIXME (mdi, "(0x%08x,0x%08x,...,%u,...): stub\n",
	   hwndParent, wFlags, cKids);

    return 0;
}

