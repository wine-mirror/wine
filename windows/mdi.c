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
#include <stdio.h>
#include <math.h>
#include "xmalloc.h"
#include "windows.h"
#include "win.h"
#include "heap.h"
#include "nonclient.h"
#include "mdi.h"
#include "user.h"
#include "menu.h"
#include "resource.h"
#include "struct32.h"
#include "sysmetrics.h"
#include "stddebug.h"
#include "debug.h"


static HBITMAP16 hBmpClose   = 0;
static HBITMAP16 hBmpRestore = 0;

DWORD SCROLL_SetNCSbState(WND*,int,int,int,int,int,int);

/* ----------------- declarations ----------------- */
void MDI_UpdateFrameText(WND *, HWND, BOOL, LPCSTR);
BOOL MDI_AugmentFrameMenu(MDICLIENTINFO*, WND *, HWND);
BOOL MDI_RestoreFrameMenu(WND *, HWND);

static LONG MDI_ChildActivate(WND* ,HWND );

/* -------- Miscellaneous service functions ----------
 *
 *			MDI_GetChildByID
 */

static HWND MDI_GetChildByID(WND* wndPtr,int id)
{
    for (wndPtr = wndPtr->child; wndPtr; wndPtr = wndPtr->next)
        if (wndPtr->wIDmenu == id) return wndPtr->hwndSelf;
    return 0;
}

static void MDI_PostUpdate(HWND hwnd, MDICLIENTINFO* ci, WORD recalc)
{
 if( !ci->sbNeedUpdate )
   {
      ci->sbNeedUpdate = TRUE;
      PostMessage( hwnd, WM_MDICALCCHILDSCROLL, 0, 0);
   }
 ci->sbRecalc = recalc;
}

/**********************************************************************
 *			MDI_MenuAppendItem
 */
#ifdef SUPERFLUOUS_FUNCTIONS
static BOOL MDI_MenuAppendItem(WND *clientWnd, HWND hWndChild)
{
 char buffer[128];
 MDICLIENTINFO  *clientInfo = (MDICLIENTINFO*)clientWnd->wExtra;
 WND		*wndPtr     = WIN_FindWndPtr(hWndChild);
 int 		 n          = sprintf(buffer, "%d ", 
				      clientInfo->nActiveChildren);

 if( !clientInfo->hWindowMenu ) return 0; 
    
 if (wndPtr->text) strncpy(buffer + n, wndPtr->text, sizeof(buffer) - n - 1);
 return AppendMenu32A( clientInfo->hWindowMenu, MF_STRING,
                       wndPtr->wIDmenu, buffer );
}
#endif

/**********************************************************************
 *			MDI_MenuModifyItem
 */
static BOOL MDI_MenuModifyItem(WND* clientWnd, HWND hWndChild )
{
 char            buffer[128];
 MDICLIENTINFO  *clientInfo = (MDICLIENTINFO*)clientWnd->wExtra;
 WND            *wndPtr     = WIN_FindWndPtr(hWndChild);
 UINT		 n          = sprintf(buffer, "%d ",
                              wndPtr->wIDmenu - clientInfo->idFirstChild + 1);
 BOOL		 bRet	    = 0;

 if( !clientInfo->hWindowMenu ) return 0;

 if (wndPtr->text) lstrcpyn32A(buffer + n, wndPtr->text, sizeof(buffer) - n );

 n    = GetMenuState(clientInfo->hWindowMenu,wndPtr->wIDmenu ,MF_BYCOMMAND); 
 bRet = ModifyMenu32A(clientInfo->hWindowMenu , wndPtr->wIDmenu, 
                      MF_BYCOMMAND | MF_STRING, wndPtr->wIDmenu, buffer );
 CheckMenuItem(clientInfo->hWindowMenu ,wndPtr->wIDmenu , n & MF_CHECKED);
 return bRet;
}

/**********************************************************************
 *			MDI_MenuDeleteItem
 */
static BOOL MDI_MenuDeleteItem(WND* clientWnd, HWND hWndChild )
{
 char    	 buffer[128];
 MDICLIENTINFO  *clientInfo = (MDICLIENTINFO*)clientWnd->wExtra;
 WND    	*wndPtr     = WIN_FindWndPtr(hWndChild);
 UINT		 index      = 0,id,n;

 if( !clientInfo->nActiveChildren ||
     !clientInfo->hWindowMenu ) return 0;

 id = wndPtr->wIDmenu;
 DeleteMenu(clientInfo->hWindowMenu,id,MF_BYCOMMAND);

 /* walk the rest of MDI children to prevent gaps in the id 
  * sequence and in the menu child list */

 for( index = id+1; index <= clientInfo->nActiveChildren + 
                             clientInfo->idFirstChild; index++ )
    {
	wndPtr = WIN_FindWndPtr(MDI_GetChildByID(clientWnd,index));
	if( !wndPtr )
	     {
	      dprintf_mdi(stddeb,"MDIMenuDeleteItem: no window for id=%i\n",index);
	      continue;
    }
    
	/* set correct id */
	wndPtr->wIDmenu--;

	n = sprintf(buffer, "%d ",index - clientInfo->idFirstChild);
	if (wndPtr->text)
            lstrcpyn32A(buffer + n, wndPtr->text, sizeof(buffer) - n );	

	/* change menu */
	ModifyMenu32A(clientInfo->hWindowMenu ,index ,MF_BYCOMMAND | MF_STRING,
                      index - 1 , buffer ); 
    }
 return 1;
}

/**********************************************************************
 * 			MDI_GetWindow
 *
 * returns "activateable" child  or zero
 */
HWND MDI_GetWindow(WND  *clientWnd, HWND hWnd, WORD wTo )
{
    MDICLIENTINFO *clientInfo = (MDICLIENTINFO*)clientWnd->wExtra;
    WND *wndPtr, *pWnd, *pWndLast;
    
    if( !hWnd ) hWnd = clientInfo->hwndActiveChild;

    if( !(wndPtr = WIN_FindWndPtr(hWnd)) ) return 0;

    pWnd = wndPtr;
    pWndLast = NULL;
    for (;;)
    {
        pWnd = pWnd->next;
        if (!pWnd) pWnd = wndPtr->parent->child;
        if (pWnd == wndPtr)  /* not found */
        {
            if (!wTo || !pWndLast) return 0;
            break;
        }

	/* we are not interested in owned popups */
        if ( !pWnd->owner &&
	     (pWnd->dwStyle & WS_VISIBLE) &&
            !(pWnd->dwStyle & WS_DISABLED))  /* found one */
        {
            pWndLast = pWnd;
            if (!wTo) break;
        }
    }
    return pWndLast ? pWndLast->hwndSelf : 0;
}

/**********************************************************************
 *			MDI_CalcDefaultChildPos
 *
 *  It seems that default height is 2/3 of client rect
 */
void MDI_CalcDefaultChildPos(WND* w, WORD n, LPPOINT16 lpPos, INT delta)
{
 RECT16 rect = w->rectClient;
 INT  spacing = GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFRAME) - 1; 
 INT  nstagger;

 if( rect.bottom - rect.top - delta >= spacing ) 
     rect.bottom -= delta;

 nstagger = (rect.bottom - rect.top)/(3*spacing);
 lpPos[1].x = (rect.right - rect.left - nstagger*spacing);
 lpPos[1].y = (rect.bottom - rect.top - nstagger*spacing);
 lpPos[0].x = lpPos[0].y = spacing*(n%(nstagger+1));
}

/**********************************************************************
 *					MDISetMenu
 */
HMENU16 MDISetMenu(HWND hwnd, BOOL fRefresh, HMENU16 hmenuFrame,
                   HMENU16 hmenuWindow)
{
    WND           *w         = WIN_FindWndPtr(hwnd);
    MDICLIENTINFO *ci;

    dprintf_mdi(stddeb, "WM_MDISETMENU: %04x %04x %04x %04x\n",
                hwnd, fRefresh, hmenuFrame, hmenuWindow);

    ci = (MDICLIENTINFO *) w->wExtra;

    if (!fRefresh) 
       {
	HWND hwndFrame = GetParent16(hwnd);
	HMENU16 oldFrameMenu = GetMenu(hwndFrame);
        
	if( ci->hwndChildMaximized && hmenuFrame && hmenuFrame!=oldFrameMenu )
	    MDI_RestoreFrameMenu(w->parent, ci->hwndChildMaximized );

	if( hmenuWindow && hmenuWindow!=ci->hWindowMenu )
	  {
	    /* delete menu items from ci->hWindowMenu 
	     * and add them to hmenuWindow */

            INT		i = GetMenuItemCount(ci->hWindowMenu) - 1;
	    INT 	pos = GetMenuItemCount(hmenuWindow) + 1;

            AppendMenu32A( hmenuWindow, MF_SEPARATOR, 0, NULL);

	    if( ci->nActiveChildren )
	      {
	        INT  j = i - ci->nActiveChildren + 1;
		char buffer[100];
		UINT id,state;

		for( ; i >= j ; i-- )
		   {
		     id = GetMenuItemID(ci->hWindowMenu,i );
		     state = GetMenuState(ci->hWindowMenu,i,MF_BYPOSITION); 

		     GetMenuString(ci->hWindowMenu, i, buffer, 100, MF_BYPOSITION);

		     DeleteMenu(ci->hWindowMenu, i , MF_BYPOSITION);
		     InsertMenu32A(hmenuWindow, pos, MF_BYPOSITION | MF_STRING,
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
	    return oldFrameMenu;
	  }

       }
    return 0;
}

/**********************************************************************
 *					MDIIconArrange
 */
WORD MDIIconArrange(HWND parent)
{
  return ArrangeIconicWindows(parent);		/* Any reason why the    */
						/* existing icon arrange */
						/* can't be used here?	 */
						/* -DRP			 */
}


/* ------------------ MDI child window functions ---------------------- */


/**********************************************************************
 *					MDICreateChild
 */
HWND MDICreateChild(WND *w, MDICLIENTINFO *ci, HWND parent, LPARAM lParam )
{
    POINT16          pos[2]; 
    MDICREATESTRUCT16 *cs = (MDICREATESTRUCT16 *)PTR_SEG_TO_LIN(lParam);
    DWORD	     style = cs->style | (WS_CHILD | WS_CLIPSIBLINGS);
    HWND 	     hwnd, hwndMax = 0;
    WORD	     wIDmenu = ci->idFirstChild + ci->nActiveChildren;
    char	     lpstrDef[]="junk!";

    /*
     * Create child window
     *
     */

    dprintf_mdi(stdnimp,"MDICreateChild: origin %i,%i - dim %i,%i, style %08x\n", 
					 cs->x, cs->y, cs->cx, cs->cy, (unsigned)cs->style);    
    /* calculate placement */
    MDI_CalcDefaultChildPos(w, ci->nTotalCreated++, pos, 0);

    if( cs->cx == CW_USEDEFAULT16 || !cs->cx )
        cs->cx = pos[1].x;
    if( cs->cy == CW_USEDEFAULT16 || !cs->cy )
        cs->cy = pos[1].y;

    if( cs->x == CW_USEDEFAULT16 )
      {
 	cs->x = pos[0].x;
	cs->y = pos[0].y;
      }

    /* restore current maximized child */
    if( style & WS_VISIBLE && ci->hwndChildMaximized )
      {
	if( style & WS_MAXIMIZE )
	  SendMessage16(w->hwndSelf, WM_SETREDRAW, FALSE, 0L );
	hwndMax = ci->hwndChildMaximized;
	ShowWindow( hwndMax, SW_SHOWNOACTIVATE );
	if( style & WS_MAXIMIZE )
	  SendMessage16(w->hwndSelf, WM_SETREDRAW, TRUE, 0L );
      }

    /* this menu is needed to set a check mark in MDI_ChildActivate */
    AppendMenu32A(ci->hWindowMenu ,MF_STRING ,wIDmenu, lpstrDef );

    ci->nActiveChildren++;

    /* fix window style */
    if( !(w->dwStyle & MDIS_ALLCHILDSTYLES) )
      {
        style &= (WS_CHILD | WS_CLIPSIBLINGS | WS_MINIMIZE | WS_MAXIMIZE |
                  WS_CLIPCHILDREN | WS_DISABLED | WS_VSCROLL | WS_HSCROLL );
        style |= (WS_VISIBLE | WS_OVERLAPPEDWINDOW);
      }

    hwnd = CreateWindow16( (LPCSTR)PTR_SEG_TO_LIN(cs->szClass),
                           (LPCSTR)PTR_SEG_TO_LIN(cs->szTitle), style, 
			  cs->x, cs->y, cs->cx, cs->cy, parent, 
                         (HMENU16)wIDmenu, w->hInstance, 
			 (LPVOID)lParam);

    /* MDI windows are WS_CHILD so they won't be activated by CreateWindow */

    if (hwnd)
    {
	WND* wnd = WIN_FindWndPtr( hwnd );

	MDI_MenuModifyItem(w ,hwnd); 
	if( wnd->dwStyle & WS_MINIMIZE && ci->hwndActiveChild )
	    ShowWindow( hwnd, SW_SHOWMINNOACTIVE );
	else
	  {
	    SetWindowPos( hwnd, 0, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE );

	    /* Set maximized state here in case hwnd didn't receive WM_SIZE
	     * during CreateWindow - bad!
	     */

            if( wnd->dwStyle & WS_MAXIMIZE && !ci->hwndChildMaximized )
              {
                ci->hwndChildMaximized = wnd->hwndSelf;
                MDI_AugmentFrameMenu( ci, w->parent, hwnd );
                MDI_UpdateFrameText( w->parent, ci->self, MDI_REPAINTFRAME, NULL ); 
	      }
	  }
        dprintf_mdi(stddeb, "MDICreateChild: created child - %04x\n",hwnd);
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
 */
void MDI_ChildGetMinMaxInfo(WND* clientWnd, HWND hwnd, MINMAXINFO16* lpMinMax )
{
 WND*	childWnd = WIN_FindWndPtr(hwnd);
 RECT16	rect 	 = clientWnd->rectClient;

 MapWindowPoints16(clientWnd->parent->hwndSelf, 
	       ((MDICLIENTINFO*)clientWnd->wExtra)->self, (LPPOINT16)&rect, 2);
 AdjustWindowRectEx16( &rect, childWnd->dwStyle, 0, childWnd->dwExStyle );

 lpMinMax->ptMaxSize.x = rect.right -= rect.left;
 lpMinMax->ptMaxSize.y = rect.bottom -= rect.top;

 lpMinMax->ptMaxPosition.x = rect.left;
 lpMinMax->ptMaxPosition.y = rect.top; 

 dprintf_mdi(stddeb,"\tChildMinMaxInfo: max rect (%i,%i - %i, %i)\n", 
                        rect.left,rect.top,rect.right,rect.bottom);

}

/**********************************************************************
 *			MDI_SwitchActiveChild
 * 
 * Notes: SetWindowPos sends WM_CHILDACTIVATE to the child window that is
 *        being activated 
 *
 *	  wTo is basically lParam of WM_MDINEXT message or explicit 
 *        window handle
 */
void MDI_SwitchActiveChild(HWND clientHwnd, HWND childHwnd, BOOL wTo )
{
    WND		  *w	     = WIN_FindWndPtr(clientHwnd);
    HWND	   hwndTo    = 0;
    HWND	   hwndPrev  = 0;
    MDICLIENTINFO *ci;

    hwndTo = MDI_GetWindow(w,childHwnd,(WORD)wTo);
 
    ci = (MDICLIENTINFO *) w->wExtra;

    dprintf_mdi(stddeb, "MDI_SwitchActiveChild: from %04x, to %04x\n",childHwnd,hwndTo);

    if ( !hwndTo ) return; 

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
	  if( !wTo && hwndPrev )
	    {
	       SetWindowPos( hwndPrev, HWND_BOTTOM, 0, 0, 0, 0, 
		  	     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	    }

	  if( bOptimize )
	       ShowWindow( clientHwnd, SW_SHOW );
	}
}

	    
/**********************************************************************
 *                                      MDIDestroyChild
 */
HWND MDIDestroyChild(WND *w_parent, MDICLIENTINFO *ci, HWND parent,
                     HWND child, BOOL flagDestroy)
{
    WND         *childPtr = WIN_FindWndPtr(child);

    if( childPtr )
    {
        if( child == ci->hwndActiveChild )
          {
	    MDI_SwitchActiveChild(parent,child,0);

	    if( child == ci->hwndActiveChild )
	      {
		ShowWindow( child, SW_HIDE);
		if( child == ci->hwndChildMaximized )
		  {
		    MDI_RestoreFrameMenu(w_parent->parent, child);
		    ci->hwndChildMaximized = 0;
		    MDI_UpdateFrameText(w_parent->parent,parent,TRUE,NULL);
		  }

                MDI_ChildActivate(w_parent,0);
	      }
	    MDI_MenuDeleteItem(w_parent, child);
	}
	
        ci->nActiveChildren--;

        dprintf_mdi(stddeb,"MDIDestroyChild: child destroyed - %04x\n",child);

        if (flagDestroy)
	   {
	     MDI_PostUpdate(GetParent16(child), ci, SB_BOTH+1);
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
LONG MDI_ChildActivate(WND *clientPtr, HWND hWndChild)
{
    MDICLIENTINFO       *clientInfo = (MDICLIENTINFO*)clientPtr->wExtra; 
    HWND                 prevActiveWnd = clientInfo->hwndActiveChild;
    WND                 *wndPtr = WIN_FindWndPtr( hWndChild );
    WND			*wndPrev = WIN_FindWndPtr( prevActiveWnd );
    BOOL		 isActiveFrameWnd = 0;	 

    if( hWndChild == prevActiveWnd ) return 0L;

    if( wndPtr )
        if( wndPtr->dwStyle & WS_DISABLED ) return 0L;

    dprintf_mdi(stddeb,"MDI_ChildActivate: %04x\n", hWndChild);

    if( GetActiveWindow() == clientPtr->parent->hwndSelf )
        isActiveFrameWnd = TRUE;
	
    /* deactivate prev. active child */
    if( wndPrev )
    {
	wndPrev->dwStyle |= WS_SYSMENU;
	SendMessage16( prevActiveWnd, WM_NCACTIVATE, FALSE, 0L );

#ifdef WINELIB32
        SendMessage32A( prevActiveWnd, WM_MDIACTIVATE, (WPARAM32)prevActiveWnd, 
                        (LPARAM)hWndChild);
#else 

        SendMessage16( prevActiveWnd, WM_MDIACTIVATE, FALSE,
                       MAKELONG(hWndChild,prevActiveWnd));
#endif 
        /* uncheck menu item */
       	if( clientInfo->hWindowMenu )
       	        CheckMenuItem( clientInfo->hWindowMenu,
       	                       wndPrev->wIDmenu, 0);
      }

    /* set appearance */
    if( clientInfo->hwndChildMaximized )
      if( clientInfo->hwndChildMaximized != hWndChild )
        if( hWndChild )
	        {
		  clientInfo->hwndActiveChild = hWndChild;
		  ShowWindow( hWndChild, SW_SHOWMAXIMIZED);
	        }
	else
		ShowWindow( clientInfo->hwndActiveChild, 
			    SW_SHOWNORMAL );

    clientInfo->hwndActiveChild = hWndChild;

    /* check if we have any children left */
    if( !hWndChild )
	{
	    if( isActiveFrameWnd )
		SetFocus32( clientInfo->self );
	    return 0;
	}
	
    /* check menu item */
    if( clientInfo->hWindowMenu )
              CheckMenuItem( clientInfo->hWindowMenu,
                             wndPtr->wIDmenu, MF_CHECKED);

    /* bring active child to the top */
    SetWindowPos( hWndChild, 0,0,0,0,0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);

    if( isActiveFrameWnd )
	  {
	    SendMessage16( hWndChild, WM_NCACTIVATE, TRUE, 0L);
	    if( GetFocus32() == clientInfo->self )
		SendMessage16( clientInfo->self, WM_SETFOCUS, 
			    (WPARAM16)clientInfo->self, 0L );
	    else
		SetFocus32( clientInfo->self );
    }

#ifdef WINELIB32
    SendMessage32A( hWndChild, WM_MDIACTIVATE, (WPARAM32)hWndChild,
                    (LPARAM)prevActiveWnd );
#else
    SendMessage16( hWndChild, WM_MDIACTIVATE, TRUE,
                   MAKELONG(hWndChild,prevActiveWnd));
#endif

    return 1;
}

/**********************************************************************
 *			MDI_BuildWCL
 *
 *  iTotal returns number of children available for tiling or cascading
 */
MDIWCL* MDI_BuildWCL(WND* clientWnd, INT16* iTotal)
{
    MDIWCL *listTop,*listNext;
    WND    *childWnd;

    if (!(listTop = (MDIWCL*)malloc( sizeof(MDIWCL) ))) return NULL;

    listTop->hChild = clientWnd->child ? clientWnd->child->hwndSelf : 0;
    listTop->prev   = NULL;
    *iTotal 	    = 1;

    /* build linked list from top child to bottom */

    childWnd  =  WIN_FindWndPtr( listTop->hChild );
    while( childWnd && childWnd->next )
    {
	listNext = (MDIWCL*)xmalloc(sizeof(MDIWCL));
	
	if( (childWnd->dwStyle & WS_DISABLED) ||
	    (childWnd->dwStyle & WS_MINIMIZE) ||
	    !(childWnd->dwStyle & WS_VISIBLE)   )
	{
	    listTop->hChild = 0;
	    (*iTotal)--;
	}

	listNext->hChild = childWnd->next->hwndSelf;
	listNext->prev   = listTop;
	listTop          = listNext;
	(*iTotal)++;

	childWnd  =  childWnd->next;
    }

    if( (childWnd->dwStyle & WS_DISABLED) ||
	(childWnd->dwStyle & WS_MINIMIZE) ||
	!(childWnd->dwStyle & WS_VISIBLE)   )
    {
	listTop->hChild = 0;
	(*iTotal)--;
    }
 
    return listTop;
}


/* -------------------- MDI client window functions ------------------- */

/**********************************************************************
 *				CreateMDIMenuBitmap
 */
HBITMAP16 CreateMDIMenuBitmap(void)
{
 HDC32 		hDCSrc  = CreateCompatibleDC32(0);
 HDC32		hDCDest	= CreateCompatibleDC32(hDCSrc);
 HBITMAP16	hbClose = LoadBitmap16(0, MAKEINTRESOURCE(OBM_CLOSE) );
 HBITMAP16	hbCopy,hb_src,hb_dest;

 hb_src = SelectObject32(hDCSrc,hbClose);
 hbCopy = CreateCompatibleBitmap(hDCSrc,SYSMETRICS_CXSIZE, SYSMETRICS_CYSIZE);
 hb_dest = SelectObject32(hDCDest,hbCopy);

 BitBlt32(hDCDest, 0, 0, SYSMETRICS_CXSIZE, SYSMETRICS_CYSIZE,
          hDCSrc, SYSMETRICS_CXSIZE, 0, SRCCOPY);
 
 SelectObject32(hDCSrc,hb_src);
 SelectObject32(hDCDest,hb_dest);

 DeleteObject32(hbClose);

 DeleteDC32(hDCDest);
 DeleteDC32(hDCSrc);

 return hbCopy;
}

/**********************************************************************
 *				MDICascade
 */
LONG MDICascade(WND* clientWnd, MDICLIENTINFO *ci)
{
    MDIWCL	 *listTop,*listPrev;
    INT16	  delta = 0,iToPosition = 0, n = 0;
    POINT16       pos[2];
  
    if (ci->hwndChildMaximized)
        ShowWindow( ci->hwndChildMaximized, SW_NORMAL);

    if (ci->nActiveChildren == 0) return 0;

    if (!(listTop = MDI_BuildWCL(clientWnd,&iToPosition))) return 0;

    if( iToPosition < ci->nActiveChildren ) 
        delta = 2 * SYSMETRICS_CYICONSPACING + SYSMETRICS_CYICON;

    /* walk list and move windows */
    while ( listTop )
    {
	dprintf_mdi(stddeb, "MDICascade: move %04x to (%d,%d) size [%d,%d]\n", 
                    listTop->hChild, pos[0].x, pos[0].y, pos[1].x, pos[1].y);

	if( listTop->hChild )
        {
            MDI_CalcDefaultChildPos(clientWnd, n++, pos, delta);
            SetWindowPos(listTop->hChild, 0, pos[0].x, pos[0].y, pos[1].x, pos[1].y,
                         SWP_DRAWFRAME | SWP_NOACTIVATE | SWP_NOZORDER);
        }

	listPrev = listTop->prev;
	free(listTop);
	listTop = listPrev;
    }

    if( iToPosition < ci->nActiveChildren )
        ArrangeIconicWindows( clientWnd->hwndSelf );

    return 0;
}

/**********************************************************************
 *					MDITile
 *
 */
LONG MDITile(WND* wndClient, MDICLIENTINFO *ci,WORD wParam)
{
    MDIWCL       *listTop,*listPrev;
    RECT16        rect;
    int           xsize, ysize;
    int		  x, y;
    int		  rows, columns;
    int           r, c;
    int           i;
    INT16	  iToPosition = 0;

    if (ci->hwndChildMaximized)
	ShowWindow(ci->hwndChildMaximized, SW_NORMAL);

    if (ci->nActiveChildren == 0) return 0;

    listTop = MDI_BuildWCL(wndClient, &iToPosition);

    dprintf_mdi(stddeb,"MDITile: %i windows to tile\n",iToPosition);

    if( !listTop ) return 0;

    /* tile children */
    if ( iToPosition )
    {
        rect = wndClient->rectClient;
    	rows    = (int) sqrt((double) iToPosition);
    	columns = iToPosition / rows;

        if (wParam == MDITILE_HORIZONTAL)  /* version >= 3.1 */
        {
            i=rows;
            rows=columns;  /* exchange r and c */
            columns=i;
        }

    	/* hack */
    	if( iToPosition != ci->nActiveChildren)
        {
            y = rect.bottom - 2 * SYSMETRICS_CYICONSPACING - SYSMETRICS_CYICON;
            rect.bottom = ( y - SYSMETRICS_CYICON < rect.top )? rect.bottom: y;
        }

    	ysize   = rect.bottom / rows;
    	xsize   = rect.right  / columns;
    
    	x       = 0;
    	i       = 0;

    	for (c = 1; c <= columns; c++)
    	{
            if (c == columns)
            {
                rows  = iToPosition - i;
                ysize = rect.bottom / rows;
            }

            y = 0;
            for (r = 1; r <= rows; r++, i++)
            {
                /* shouldn't happen but... */
                if( !listTop )
                    break;
                
                /* skip iconized childs from tiling */
                while (!listTop->hChild)
                {
    	            listPrev = listTop->prev;
    	            free(listTop);
    	            listTop = listPrev;
    	        }                
                SetWindowPos(listTop->hChild, 0, x, y, xsize, ysize, 
                             SWP_DRAWFRAME | SWP_NOACTIVATE | SWP_NOZORDER);
                y += ysize;
    	        listPrev = listTop->prev;
    	        free(listTop);
    	        listTop = listPrev;
            }
            x += xsize;
    	}
    }
  
    /* free the rest if any */
    while( listTop )
    {
        listPrev = listTop->prev;
        free(listTop);
        listTop = listPrev;
    }
    
    if (iToPosition < ci->nActiveChildren )
        ArrangeIconicWindows( wndClient->hwndSelf );

    return 0;
}

/* ----------------------- Frame window ---------------------------- */


/**********************************************************************
 *					MDI_AugmentFrameMenu
 */
BOOL MDI_AugmentFrameMenu(MDICLIENTINFO* ci, WND *frame, HWND hChild)
{
 WND*		child = WIN_FindWndPtr(hChild);
 HMENU16  	hSysPopup = 0;

 dprintf_mdi(stddeb,"MDI_AugmentFrameMenu: frame %p,child %04x\n",frame,hChild);

 if( !frame->wIDmenu || !child->hSysMenu ) return 0; 

 /* create a copy of sysmenu popup and insert it into frame menu bar */

 if (!(hSysPopup = LoadMenuIndirect32A(SYSRES_GetResPtr(SYSRES_MENU_SYSMENU))))
     return 0;
 
 dprintf_mdi(stddeb,"\t\tgot popup %04x\n in sysmenu %04x",hSysPopup,child->hSysMenu);
 
 if( !InsertMenu32A(frame->wIDmenu,0,MF_BYPOSITION | MF_BITMAP | MF_POPUP,
                    hSysPopup, (LPSTR)(DWORD)hBmpClose ))
   {  DestroyMenu(hSysPopup); return 0; }

 if( !AppendMenu32A(frame->wIDmenu,MF_HELP | MF_BITMAP,
                    SC_RESTORE, (LPSTR)(DWORD)hBmpRestore ))
   {
      RemoveMenu(frame->wIDmenu,0,MF_BYPOSITION);
      return 0;
   }

 EnableMenuItem(hSysPopup, SC_SIZE, MF_BYCOMMAND | MF_GRAYED);
 EnableMenuItem(hSysPopup, SC_MOVE, MF_BYCOMMAND | MF_GRAYED);
 EnableMenuItem(hSysPopup, SC_MAXIMIZE, MF_BYCOMMAND | MF_GRAYED);

 /* redraw menu */
 DrawMenuBar(frame->hwndSelf);

 return 1;
}

/**********************************************************************
 *					MDI_RestoreFrameMenu
 */
BOOL MDI_RestoreFrameMenu( WND *frameWnd, HWND hChild)
{
 INT	nItems   = GetMenuItemCount(frameWnd->wIDmenu) - 1;

 dprintf_mdi(stddeb,"MDI_RestoreFrameMenu: for child %04x\n",hChild);

 if( GetMenuItemID(frameWnd->wIDmenu,nItems) != SC_RESTORE )
     return 0; 


 RemoveMenu(frameWnd->wIDmenu,0,MF_BYPOSITION);
 DeleteMenu(frameWnd->wIDmenu,nItems-1,MF_BYPOSITION);

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
void MDI_UpdateFrameText(WND *frameWnd, HWND hClient, BOOL repaint, LPCSTR lpTitle)
{
 char   lpBuffer[MDI_MAXTITLELENGTH+1];
 WND* 	clientWnd = WIN_FindWndPtr(hClient);

 MDICLIENTINFO *ci = (MDICLIENTINFO *) clientWnd->wExtra;

 dprintf_mdi(stddeb, "MDI: repaint %i, frameText %s\n", repaint, (lpTitle)?lpTitle:"NULL");

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
	 int    i_child_text_length = strlen(childWnd->text);

	 lstrcpyn32A( lpBuffer, ci->frameTitle, MDI_MAXTITLELENGTH);

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
                 lstrcpyn32A( lpBuffer + i_frame_text_length + 4, 
                              childWnd->text,
                              MDI_MAXTITLELENGTH - i_frame_text_length - 5 );
                 strcat( lpBuffer, "]" );
		}
	   }
       }
     else
       {
         strncpy(lpBuffer, ci->frameTitle, MDI_MAXTITLELENGTH );
	 lpBuffer[MDI_MAXTITLELENGTH]='\0';
       }
   }
 else
   lpBuffer[0] = '\0';

 DEFWND_SetText( frameWnd, lpBuffer );
 if( repaint == MDI_REPAINTFRAME)
     SetWindowPos(frameWnd->hwndSelf, 0,0,0,0,0, SWP_FRAMECHANGED |
                  SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER );
}


/* ----------------------------- Interface ---------------------------- */


/**********************************************************************
 *					MDIClientWndProc
 *
 * This function is the handler for all MDI requests.
 */
LRESULT MDIClientWndProc(HWND hwnd, UINT message, WPARAM16 wParam, LPARAM lParam)
{
    LPCREATESTRUCT16     cs;
    LPCLIENTCREATESTRUCT16 ccs;
    MDICLIENTINFO       *ci;
    RECT16		 rect;
    WND                 *w 	  = WIN_FindWndPtr(hwnd);
    WND			*frameWnd = w->parent;
    INT			nItems;
    
    ci = (MDICLIENTINFO *) w->wExtra;
    
    switch (message)
    {
      case WM_CREATE:
	cs                      = (LPCREATESTRUCT16) PTR_SEG_TO_LIN(lParam);
	ccs                     = (LPCLIENTCREATESTRUCT16) PTR_SEG_TO_LIN(cs->lpCreateParams);

	ci->hWindowMenu         = ccs->hWindowMenu;
	ci->idFirstChild        = ccs->idFirstChild;
	ci->hwndChildMaximized  = 0;
	ci->nActiveChildren	= 0;
	ci->nTotalCreated	= 0;
	ci->frameTitle		= NULL;
	ci->sbNeedUpdate	= 0;
	ci->self		= hwnd;
	w->dwStyle             |= WS_CLIPCHILDREN;

	if (!hBmpClose)
        {
            hBmpClose = CreateMDIMenuBitmap();
            hBmpRestore = LoadBitmap16( 0, MAKEINTRESOURCE(OBM_RESTORE) );
        }
	MDI_UpdateFrameText(frameWnd, hwnd, MDI_NOFRAMEREPAINT,frameWnd->text);

	AppendMenu32A( ccs->hWindowMenu, MF_SEPARATOR, 0, NULL );

	GetClientRect16(frameWnd->hwndSelf, &rect);
	NC_HandleNCCalcSize( w, &rect );
	w->rectClient = rect;

	dprintf_mdi(stddeb,"MDI: Client created - hwnd = %04x, idFirst = %u\n",hwnd,ci->idFirstChild);

	return 0;
      
      case WM_DESTROY:
	if( ci->hwndChildMaximized ) MDI_RestoreFrameMenu(w, frameWnd->hwndSelf);
	if((nItems = GetMenuItemCount(ci->hWindowMenu)) > 0) {
    	    ci->idFirstChild = nItems - 1;
	    ci->nActiveChildren++; 		/* to delete a separator */
	    while( ci->nActiveChildren-- )
	        DeleteMenu(ci->hWindowMenu,MF_BYPOSITION,ci->idFirstChild--);
	}
	return 0;

      case WM_MDIACTIVATE:
        if( ci->hwndActiveChild != (HWND)wParam )
	    SetWindowPos((HWND)wParam, 0,0,0,0,0, SWP_NOSIZE | SWP_NOMOVE ); 
	return 0;

      case WM_MDICASCADE:
	return MDICascade(w, ci);

      case WM_MDICREATE:
	return (LONG)MDICreateChild(w, ci, hwnd, lParam );

      case WM_MDIDESTROY:
	return (LONG)MDIDestroyChild(w, ci, hwnd, (HWND)wParam, TRUE);

      case WM_MDIGETACTIVE:
	return ((LONG) ci->hwndActiveChild | 
		((LONG) (ci->hwndChildMaximized>0) << 16));

      case WM_MDIICONARRANGE:
	ci->sbNeedUpdate = TRUE;
	MDIIconArrange(hwnd);
	ci->sbRecalc = SB_BOTH+1;
	SendMessage16(hwnd,WM_MDICALCCHILDSCROLL,0,0L);
	return 0;
	
      case WM_MDIMAXIMIZE:
	ShowWindow((HWND)wParam, SW_MAXIMIZE);
	return 0;

      case WM_MDINEXT:
	MDI_SwitchActiveChild(hwnd, (HWND)wParam, (lParam)?1:0);
	break;
	
      case WM_MDIRESTORE:
	ShowWindow( (HWND)wParam, SW_NORMAL);
	return 0;

      case WM_MDISETMENU:
#ifdef WINELIB32
	return (LRESULT)MDISetMenu(hwnd, FALSE, (HMENU16)wParam, (HMENU16)lParam);
#else
	return (LRESULT)MDISetMenu(hwnd, wParam, LOWORD(lParam), HIWORD(lParam));
#endif
	
      case WM_MDITILE:
	ci->sbNeedUpdate = TRUE;
	ShowScrollBar32(hwnd,SB_BOTH,FALSE);
	MDITile(w, ci,wParam);
        ci->sbNeedUpdate = FALSE;
        return 0;

      case WM_VSCROLL:
      case WM_HSCROLL:
	ci->sbNeedUpdate = TRUE;
        ScrollChildren(hwnd,message,wParam,lParam);
	ci->sbNeedUpdate = FALSE;
        return 0;

      case WM_SETFOCUS:
	if( ci->hwndActiveChild )
	  {
	   w = WIN_FindWndPtr( ci->hwndActiveChild );
	   if( !(w->dwStyle & WS_MINIMIZE) )
	       SetFocus32( ci->hwndActiveChild );
	  } 
	return 0;
	
      case WM_NCACTIVATE:
        if( ci->hwndActiveChild )
	     SendMessage16(ci->hwndActiveChild, message, wParam, lParam);
	break;
	
      case WM_PARENTNOTIFY:
        if( wParam == WM_LBUTTONDOWN )
        {
            POINT16  pt = MAKEPOINT16(lParam);
            HWND     child = ChildWindowFromPoint16(hwnd, pt);

	    dprintf_mdi(stddeb,"MDIClient: notification from %04x (%i,%i)\n",child,pt.x,pt.y);

            if( child && child != hwnd )
              {
                WND*    wnd = WIN_FindWndPtr( child );

                /* if we got owned popup */
                if( wnd->owner ) child = wnd->owner->hwndSelf;

                if( child != ci->hwndActiveChild )
                    SetWindowPos(child, 0,0,0,0,0, SWP_NOSIZE | SWP_NOMOVE );
              }
          }
        return 0;

      case WM_SIZE:
          if( ci->hwndChildMaximized )
	  {
	     WND*	child = WIN_FindWndPtr(ci->hwndChildMaximized);
	     RECT16	rect  = { 0, 0, LOWORD(lParam), HIWORD(lParam) };

	     AdjustWindowRectEx16(&rect, child->dwStyle, 0, child->dwExStyle);
	     MoveWindow(ci->hwndChildMaximized, rect.left, rect.top,
			rect.right - rect.left, rect.bottom - rect.top, 1);
	  }
	else
	  MDI_PostUpdate(hwnd, ci, SB_BOTH+1);

	break;

      case WM_MDICALCCHILDSCROLL:
	if( ci->sbNeedUpdate )
	  if( ci->sbRecalc )
	    {
	      CalcChildScroll(hwnd, ci->sbRecalc-1);
	      ci->sbRecalc = ci->sbNeedUpdate = 0;
	    }
	return 0;
    }
    
    return DefWindowProc16(hwnd, message, wParam, lParam);
}


/***********************************************************************
 *           DefFrameProc16   (USER.445)
 */
LRESULT DefFrameProc16( HWND16 hwnd, HWND16 hwndMDIClient, UINT16 message, 
                        WPARAM16 wParam, LPARAM lParam )
{
    HWND	         childHwnd;
    MDICLIENTINFO*       ci;
    WND*                 wndPtr;

    if (hwndMDIClient)
    {
	switch (message)
	{
	  case WM_COMMAND:
	    wndPtr = WIN_FindWndPtr(hwndMDIClient);
	    ci     = (MDICLIENTINFO*)wndPtr->wExtra;

	    /* check for possible syscommands for maximized MDI child */

	    if( wParam <  ci->idFirstChild || 
		wParam >= ci->idFirstChild + ci->nActiveChildren )
	      {
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
	    	childHwnd = MDI_GetChildByID( WIN_FindWndPtr(hwndMDIClient),
                                          wParam );
 	    	if( childHwnd )
	            SendMessage16(hwndMDIClient, WM_MDIACTIVATE,
                                  (WPARAM16)childHwnd , 0L);
	      }
	    break;

	  case WM_NCACTIVATE:
	    SendMessage16(hwndMDIClient, message, wParam, lParam);
	    break;

	  case WM_SETTEXT:
	    MDI_UpdateFrameText(WIN_FindWndPtr(hwnd), hwndMDIClient, 
				      MDI_REPAINTFRAME, 
			             (LPCSTR)PTR_SEG_TO_LIN(lParam));
	    return 0;
	
	  case WM_SETFOCUS:
	    SetFocus32(hwndMDIClient);
	    break;

	  case WM_SIZE:
	    MoveWindow(hwndMDIClient, 0, 0, 
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

		if( wParam == VK_LEFT ) 
		  { if( wndPtr->parent->wIDmenu != LOWORD(lParam) ) break; }
		else if( wParam == VK_RIGHT )
		  { if( GetSystemMenu( wndPtr->parent->hwndSelf, 0) 
				       != LOWORD(lParam) ) break; }
		else break;
		
		return MAKELONG( GetSystemMenu(ci->hwndActiveChild, 0), 
				 ci->hwndActiveChild );
	      }
	    break;
	}
    }
    
    return DefWindowProc16(hwnd, message, wParam, lParam);
}


/***********************************************************************
 *           DefFrameProc32A   (USER32.121)
 */
LRESULT DefFrameProc32A( HWND32 hwnd, HWND32 hwndMDIClient, UINT32 message, 
                         WPARAM32 wParam, LPARAM lParam )
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
	    SendMessage32A(hwndMDIClient, message, wParam, lParam);
	    break;

	  case WM_SETTEXT:
              return DefFrameProc16( hwnd, hwndMDIClient, message,
                                     wParam, (LPARAM)PTR_SEG_TO_LIN(lParam) );
	
	  case WM_SETFOCUS:
	  case WM_SIZE:
              return DefFrameProc16( hwnd, hwndMDIClient, message,
                                     wParam, lParam );
	}
    }
    
    return DefWindowProc32A(hwnd, message, wParam, lParam);
}


/***********************************************************************
 *           DefFrameProc32W   (USER32.122)
 */
LRESULT DefFrameProc32W( HWND32 hwnd, HWND32 hwndMDIClient, UINT32 message, 
                         WPARAM32 wParam, LPARAM lParam )
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
	    SendMessage32W(hwndMDIClient, message, wParam, lParam);
	    break;

	  case WM_SETTEXT:
              /* FIXME: Unicode */
              return DefFrameProc32A( hwnd, hwndMDIClient, message,
                                     wParam, lParam );
	
	  case WM_SETFOCUS:
	  case WM_SIZE:
              return DefFrameProc32A( hwnd, hwndMDIClient, message,
                                      wParam, lParam );
	}
    }
    
    return DefWindowProc32W( hwnd, message, wParam, lParam );
}


/***********************************************************************
 *           DefMDIChildProc16   (USER.447)
 */
LRESULT DefMDIChildProc16( HWND16 hwnd, UINT16 message,
                           WPARAM16 wParam, LPARAM lParam )
{
    MDICLIENTINFO       *ci;
    WND                 *clientWnd;

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
        return 0;

      case WM_CLOSE:
	SendMessage16(ci->self,WM_MDIDESTROY,(WPARAM16)hwnd,0L);
	return 0;

      case WM_SETFOCUS:
	if( ci->hwndActiveChild != hwnd )
	    MDI_ChildActivate(clientWnd, hwnd);
	break;

      case WM_CHILDACTIVATE:
	MDI_ChildActivate(clientWnd, hwnd);
	return 0;

      case WM_NCPAINT:
	dprintf_mdi(stddeb,"DefMDIChildProc: WM_NCPAINT for %04x, active %04x\n",
					     hwnd, ci->hwndActiveChild );
	break;

      case WM_SYSCOMMAND:
	switch( wParam )
	  {
		case SC_MOVE:
		     if( ci->hwndChildMaximized == hwnd) return 0;
		     break;
	        case SC_RESTORE:
	        case SC_MINIMIZE:
		     WIN_FindWndPtr(hwnd)->dwStyle |= WS_SYSMENU;
		     break;
		case SC_MAXIMIZE:
		     if( ci->hwndChildMaximized == hwnd) 
			 return SendMessage16( clientWnd->parent->hwndSelf,
                                             message, wParam, lParam);
		     WIN_FindWndPtr(hwnd)->dwStyle &= ~WS_SYSMENU;
		     break;
		case SC_NEXTWINDOW:
		     SendMessage16( ci->self, WM_MDINEXT, 0, 0);
		     return 0;
		case SC_PREVWINDOW:
		     SendMessage16( ci->self, WM_MDINEXT, 0, 1);
		     return 0;
	  }
	break;
	
      case WM_GETMINMAXINFO:
	MDI_ChildGetMinMaxInfo(clientWnd, hwnd, (MINMAXINFO16*) PTR_SEG_TO_LIN(lParam));
	return 0;

      case WM_SETVISIBLE:
         if( ci->hwndChildMaximized)
             ci->sbNeedUpdate = 0;
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
	    HWND hMaxChild = ci->hwndChildMaximized;

	    if( hMaxChild == hwnd ) break;

	    if( hMaxChild)
	      {	    
	       SendMessage16( hMaxChild, WM_SETREDRAW, FALSE, 0L );

	       MDI_RestoreFrameMenu( clientWnd->parent, hMaxChild);
	       ShowWindow( hMaxChild, SW_SHOWNOACTIVATE);

	       SendMessage16( hMaxChild, WM_SETREDRAW, TRUE, 0L );
	      }

	    dprintf_mdi(stddeb,"\tMDI: maximizing child %04x\n", hwnd );

	    ci->hwndChildMaximized = hwnd; /* !!! */

	    MDI_AugmentFrameMenu( ci, clientWnd->parent, hwnd);
	    MDI_UpdateFrameText( clientWnd->parent, ci->self,
				 MDI_REPAINTFRAME, NULL ); 
	  }

	if( wParam == SIZE_MINIMIZED )
	  {
	    HWND switchTo = MDI_GetWindow(clientWnd, hwnd, 0);

	    if( switchTo )
	        SendMessage16( switchTo, WM_CHILDACTIVATE, 0, 0L);
	  }
	  
	MDI_PostUpdate(clientWnd->hwndSelf, ci, SB_BOTH+1);
	break;

      case WM_MENUCHAR:

	/* MDI children don't have menu bars */
	PostMessage( clientWnd->parent->hwndSelf, WM_SYSCOMMAND, 
                     (WPARAM16)SC_KEYMENU, (LPARAM)wParam);
	return 0x00010000L;

      case WM_NEXTMENU:

	if( wParam == VK_LEFT )		/* switch to frame system menu */
	  return MAKELONG( GetSystemMenu(clientWnd->parent->hwndSelf, 0), 
			   clientWnd->parent->hwndSelf );
	if( wParam == VK_RIGHT )	/* to frame menu bar */
	  return MAKELONG( clientWnd->parent->wIDmenu,
			   clientWnd->parent->hwndSelf );

	break;	
    }
	
    return DefWindowProc16(hwnd, message, wParam, lParam);
}


/***********************************************************************
 *           DefMDIChildProc32A   (USER32.123)
 */
LRESULT DefMDIChildProc32A( HWND32 hwnd, UINT32 message,
                            WPARAM32 wParam, LPARAM lParam )
{
    MDICLIENTINFO       *ci;
    WND                 *clientWnd;

    clientWnd  = WIN_FindWndPtr(GetParent16(hwnd));
    ci         = (MDICLIENTINFO *) clientWnd->wExtra;

    switch (message)
    {
      case WM_SETTEXT:
	DefWindowProc32A(hwnd, message, wParam, lParam);
	MDI_MenuModifyItem(clientWnd,hwnd);
	if( ci->hwndChildMaximized == hwnd )
	    MDI_UpdateFrameText( clientWnd->parent, ci->self,
				 MDI_REPAINTFRAME, NULL );
        return 0;

      case WM_GETMINMAXINFO:
        {
            MINMAXINFO16 mmi;
            STRUCT32_MINMAXINFO32to16( (MINMAXINFO32 *)lParam, &mmi );
            MDI_ChildGetMinMaxInfo( clientWnd, hwnd, &mmi );
            STRUCT32_MINMAXINFO16to32( &mmi, (MINMAXINFO32 *)lParam );
        }
	return 0;

      case WM_MENUCHAR:

	/* MDI children don't have menu bars */
	PostMessage( clientWnd->parent->hwndSelf, WM_SYSCOMMAND, 
                     (WPARAM16)SC_KEYMENU, (LPARAM)LOWORD(wParam) );
	return 0x00010000L;

      case WM_CLOSE:
      case WM_SETFOCUS:
      case WM_CHILDACTIVATE:
      case WM_NCPAINT:
      case WM_SYSCOMMAND:
      case WM_SETVISIBLE:
      case WM_SIZE:
      case WM_NEXTMENU:
          return DefMDIChildProc16( hwnd, message, (WPARAM16)wParam, lParam );
    }
    return DefWindowProc32A(hwnd, message, wParam, lParam);
}


/***********************************************************************
 *           DefMDIChildProc32W   (USER32.124)
 */
LRESULT DefMDIChildProc32W( HWND32 hwnd, UINT32 message,
                            WPARAM32 wParam, LPARAM lParam )
{
    MDICLIENTINFO       *ci;
    WND                 *clientWnd;

    clientWnd  = WIN_FindWndPtr(GetParent16(hwnd));
    ci         = (MDICLIENTINFO *) clientWnd->wExtra;

    switch (message)
    {
      case WM_SETTEXT:
	DefWindowProc32W(hwnd, message, wParam, lParam);
	MDI_MenuModifyItem(clientWnd,hwnd);
	if( ci->hwndChildMaximized == hwnd )
	    MDI_UpdateFrameText( clientWnd->parent, ci->self,
				 MDI_REPAINTFRAME, NULL );
        return 0;

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
          return DefMDIChildProc32A( hwnd, message, (WPARAM16)wParam, lParam );
    }
    return DefWindowProc32W(hwnd, message, wParam, lParam);
}


/**********************************************************************
 *					TranslateMDISysAccel (USER.451)
 *
 */
BOOL TranslateMDISysAccel(HWND hwndClient, LPMSG16 msg)
{
  WND* clientWnd = WIN_FindWndPtr( hwndClient);
  WND* wnd;
  MDICLIENTINFO       *ci     = NULL;
  WPARAM16	       wParam = 0;

  if( (msg->message != WM_KEYDOWN && msg->message != WM_SYSKEYDOWN) || !clientWnd )
    return 0;

  ci = (MDICLIENTINFO*) clientWnd->wExtra;
  wnd = WIN_FindWndPtr(ci->hwndActiveChild);
 
  if( !wnd ) return 0;
  
  if( wnd->dwStyle & WS_DISABLED ) return 0;
   
  if ((GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_MENU) & 0x8000))
    switch( msg->wParam )
      {
	case VK_F6:
	case VK_SEPARATOR:
	     wParam = ( GetKeyState(VK_SHIFT) & 0x8000 )? SC_NEXTWINDOW: SC_PREVWINDOW;
	     break;
	case VK_RBUTTON:
	     wParam = SC_CLOSE; 
	     break;
	default:
	     return 0;
      }
  else
      return 0;

  dprintf_mdi(stddeb,"TranslateMDISysAccel: wParam = %04x\n", wParam);

  SendMessage16(ci->hwndActiveChild,WM_SYSCOMMAND, wParam, (LPARAM)msg->wParam);
  return 1;
}


/***********************************************************************
 *           CalcChildScroll   (USER.462)
 */
void CalcChildScroll( HWND hwnd, WORD scroll )
{
    RECT16 childRect, clientRect;
    INT  vmin, vmax, hmin, hmax, vpos, hpos;
    BOOL noscroll = FALSE;
    WND *pWnd, *Wnd;

    if (!(Wnd = pWnd = WIN_FindWndPtr( hwnd ))) return;
    GetClientRect16( hwnd, &clientRect );
    SetRectEmpty16( &childRect );

    for ( pWnd = pWnd->child; pWnd; pWnd = pWnd->next )
	{
          UnionRect16( &childRect, &pWnd->rectWindow, &childRect );
	  if( pWnd->dwStyle & WS_MAXIMIZE )
	      noscroll = TRUE;
	} 
    UnionRect16( &childRect, &clientRect, &childRect );

    /* jump through the hoops to prevent excessive flashing 
     */

    hmin = childRect.left; hmax = childRect.right - clientRect.right;
    hpos = clientRect.left - childRect.left;
    vmin = childRect.top; vmax = childRect.bottom - clientRect.bottom;
    vpos = clientRect.top - childRect.top;

    if( noscroll )
	ShowScrollBar32(hwnd, SB_BOTH, FALSE);
    else
    switch( scroll )
      {
	case SB_HORZ:
			vpos = hpos; vmin = hmin; vmax = hmax;
	case SB_VERT:
			SetScrollPos32(hwnd, scroll, vpos, FALSE);
			SetScrollRange32(hwnd, scroll, vmin, vmax, TRUE);
			break;
	case SB_BOTH:
			SCROLL_SetNCSbState( Wnd, vmin, vmax, vpos,
						  hmin, hmax, hpos);
			SetWindowPos(hwnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE
                                 | SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED );
      }    
}

/***********************************************************************
 *           ScrollChildren   (USER.463)
 */
void ScrollChildren(HWND hWnd, UINT uMsg, WPARAM16 wParam, LPARAM lParam)
{
 WND	*wndPtr = WIN_FindWndPtr(hWnd);
 short 	 newPos=-1;
 short 	 curPos;
 short 	 length;
 INT32 	 minPos;
 INT32 	 maxPos;
 short   shift;

 if( !wndPtr ) return;

 if( uMsg == WM_HSCROLL )
   {
     GetScrollRange32(hWnd,SB_HORZ,&minPos,&maxPos);
     curPos = GetScrollPos32(hWnd,SB_HORZ);
     length = (wndPtr->rectClient.right - wndPtr->rectClient.left)/2;
     shift = SYSMETRICS_CYHSCROLL;
   }
 else if( uMsg == WM_VSCROLL )
	{
	  GetScrollRange32(hWnd,SB_VERT,&minPos,&maxPos);
	  curPos = GetScrollPos32(hWnd,SB_VERT);
	  length = (wndPtr->rectClient.bottom - wndPtr->rectClient.top)/2;
	  shift = SYSMETRICS_CXVSCROLL;
	}
      else return;

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
			CalcChildScroll(hWnd,(uMsg == WM_VSCROLL)?SB_VERT:SB_HORZ);
			return;
   }

 if( newPos > maxPos )
     newPos = maxPos;
 else if( newPos < minPos )
	  newPos = minPos;

 SetScrollPos32(hWnd, (uMsg == WM_VSCROLL)?SB_VERT:SB_HORZ , newPos, TRUE);

 if( uMsg == WM_VSCROLL )
     ScrollWindow32(hWnd ,0 ,curPos - newPos, NULL, NULL);
 else
     ScrollWindow32(hWnd ,curPos - newPos, 0, NULL, NULL);
}

