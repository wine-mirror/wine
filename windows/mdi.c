/* MDI.C
 *
 * Copyright 1994, Bob Amstadt
 *           1995,1996 Alex Korobka
 *
 * This file contains routines to support MDI features.
 *
 * Notes: Windows keeps ID of MDI menu item in the wIDenu field
 *        of corresponding MDI child.
 *
 *	  Basic child activation routine is MDI_ChildActivate and 
 *        SetWindowPos(childHwnd,...) implicitly calls it if SWP_NOACTIVATE
 *        is not used.
 *
 * To fix:
 *	  Sticky client crollbars 
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "windows.h"
#include "win.h"
#include "nonclient.h"
#include "mdi.h"
#include "user.h"
#include "menu.h"
#include "stackframe.h"
#include "sysmetrics.h"
#include "stddebug.h"
#include "debug.h"

void MDI_UpdateFrameText(HWND, HWND, BOOL, LPCSTR);
BOOL MDI_AugmentFrameMenu(MDICLIENTINFO*, HWND, HWND);
BOOL MDI_RestoreFrameMenu(HWND, HWND);

void ScrollChildren(HWND , UINT , WPARAM , LPARAM );
void CalcChildScroll(HWND, WORD);

/* ----------------- declarations ----------------- */

static LONG MDI_ChildActivate(WND* ,HWND );

/* -------- Miscellaneous service functions ----------
 *
 *			MDI_GetChildByID
 */

static HWND MDI_GetChildByID(WND* mdiClient,int id)
{
 HWND	hWnd   	    = mdiClient->hwndChild;
 WND* 	wndPtr 	    = WIN_FindWndPtr( hWnd );

 while( wndPtr )
  {
	if( wndPtr->wIDmenu == id ) return hWnd;
 	wndPtr = WIN_FindWndPtr(hWnd = wndPtr->hwndNext);
  }

 return 0;
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
 LPSTR		 lpWndText  = (LPSTR) USER_HEAP_LIN_ADDR(wndPtr->hText);
 int 		 n          = sprintf(buffer, "%d ", 
				      clientInfo->nActiveChildren);

 if( !clientInfo->hWindowMenu ) return 0; 
    
 if( lpWndText )
     strncpy(buffer + n, lpWndText, sizeof(buffer) - n - 1);
 return AppendMenu(clientInfo->hWindowMenu,MF_STRING,
                       wndPtr->wIDmenu, MAKE_SEGPTR(buffer) );
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
 LPSTR           lpWndText  = (LPSTR) USER_HEAP_LIN_ADDR(wndPtr->hText);
 UINT		 n          = sprintf(buffer, "%d ",
                              wndPtr->wIDmenu - clientInfo->idFirstChild + 1);
 BOOL		 bRet	    = 0;

 if( !clientInfo->hWindowMenu ) return 0;

 if( lpWndText ) lstrcpyn(buffer + n, lpWndText, sizeof(buffer) - n );

 n    = GetMenuState(clientInfo->hWindowMenu,wndPtr->wIDmenu ,MF_BYCOMMAND); 
 bRet = ModifyMenu(clientInfo->hWindowMenu , wndPtr->wIDmenu , 
                   MF_BYCOMMAND | MF_STRING, wndPtr->wIDmenu ,
                   MAKE_SEGPTR(buffer) );
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
 LPSTR		 lpWndText;
 INT		 index      = 0,id,n;

 if( !clientInfo->nActiveChildren ||
     !clientInfo->hWindowMenu ) return 0;

 id = wndPtr->wIDmenu;
 DeleteMenu(clientInfo->hWindowMenu,id,MF_BYCOMMAND);

 /* walk the rest of MDI children to prevent gaps in the id 
  * sequence and in the menu child list 
  */

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

	n          = sprintf(buffer, "%d ",index - clientInfo->idFirstChild);
	lpWndText  = (LPSTR) USER_HEAP_LIN_ADDR(wndPtr->hText);

	if( lpWndText )
	    strncpy(buffer + n, lpWndText, sizeof(buffer) - n - 1);	

	/* change menu */
	ModifyMenu(clientInfo->hWindowMenu ,index ,MF_BYCOMMAND | MF_STRING,
		   index - 1 , MAKE_SEGPTR(buffer) ); 
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
 HWND            hWndNext;
 MDICLIENTINFO  *clientInfo = (MDICLIENTINFO*)clientWnd->wExtra;
 WND            *wndPtr;

 if( !hWnd ) hWnd = clientInfo->hwndActiveChild;

 if( !(wndPtr = WIN_FindWndPtr(hWnd)) ) return 0;

 hWndNext = hWnd;
 wTo      = wTo ? GW_HWNDPREV : GW_HWNDNEXT;

 while( hWndNext )
    {
        if( clientWnd->hwndChild == hWndNext && wTo == GW_HWNDPREV )
             hWndNext = GetWindow( hWndNext, GW_HWNDLAST);
        else if( wndPtr->hwndNext == 0 && wTo == GW_HWNDNEXT )
                 hWndNext = clientWnd->hwndChild;
             else
                 hWndNext = GetWindow( hWndNext, wTo );
	
        wndPtr = WIN_FindWndPtr( hWndNext );

        if( (wndPtr->dwStyle & WS_VISIBLE) &&
           !(wndPtr->dwStyle & WS_DISABLED) )
             break;

        /* check if all windows were iterated through */
        if( hWndNext == hWnd ) break;
    }

 return ( hWnd == hWndNext )? 0 : hWndNext;
}


/**********************************************************************
 *					MDISetMenu
 * FIXME: This is not complete.
 */
HMENU MDISetMenu(HWND hwnd, BOOL fRefresh, HMENU hmenuFrame, HMENU hmenuWindow)
{
    dprintf_mdi(stddeb, "WM_MDISETMENU: "NPFMT" %04x "NPFMT" "NPFMT"\n", hwnd, fRefresh, hmenuFrame, hmenuWindow);
    if (!fRefresh) {
	HWND hwndFrame = GetParent(hwnd);
	HMENU oldFrameMenu = GetMenu(hwndFrame);
	SetMenu(hwndFrame, hmenuFrame);
	return oldFrameMenu;
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
    MDICREATESTRUCT *cs = (MDICREATESTRUCT *)PTR_SEG_TO_LIN(lParam);
    HWND hwnd;
    WORD	     wIDmenu = ci->idFirstChild + ci->nActiveChildren;
    int spacing;
    char	     chDef = '\0';

    /*
     * Create child window
     */
    cs->style &= (WS_MINIMIZE | WS_MAXIMIZE | WS_HSCROLL | WS_VSCROLL);

				/* The child windows should probably  */
				/* stagger, shouldn't they? -DRP      */
    spacing = GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFRAME);
    cs->x = ci->nActiveChildren * spacing;  
    cs->y = ci->nActiveChildren * spacing;

    /* this menu is needed to set a check mark in MDI_ChildActivate */
    AppendMenu(ci->hWindowMenu ,MF_STRING ,wIDmenu, MAKE_SEGPTR(&chDef) );

    hwnd = CreateWindow( cs->szClass, cs->szTitle,
			  WS_CHILD | WS_BORDER | WS_CAPTION | WS_CLIPSIBLINGS |
			  WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU |
			  WS_THICKFRAME | WS_VISIBLE | cs->style,
			  cs->x, cs->y, cs->cx, cs->cy, parent, 
                         (HMENU)(DWORD)(WORD)wIDmenu, w->hInstance, 
			 (SEGPTR)lParam);

    if (hwnd)
    {
	ci->nActiveChildren++;
	MDI_MenuModifyItem(w ,hwnd); 

    }
    else
	DeleteMenu(ci->hWindowMenu,wIDmenu,MF_BYCOMMAND);
	
    return hwnd;
}

/**********************************************************************
 *			MDI_ChildGetMinMaxInfo
 *
 */
void MDI_ChildGetMinMaxInfo(WND* clientWnd, HWND hwnd, MINMAXINFO* lpMinMax )
{
 WND*	childWnd = WIN_FindWndPtr(hwnd);
 RECT 	rect 	 = clientWnd->rectClient;

 MapWindowPoints(clientWnd->hwndParent, 
	       ((MDICLIENTINFO*)clientWnd->wExtra)->self, (LPPOINT)&rect, 2);
 AdjustWindowRectEx(&rect, childWnd->dwStyle, 0, childWnd->dwExStyle);

 lpMinMax->ptMaxSize.x = rect.right -= rect.left;
 lpMinMax->ptMaxSize.y = rect.bottom -= rect.top;

 lpMinMax->ptMaxPosition.x = rect.left;
 lpMinMax->ptMaxPosition.y = rect.top; 
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

    dprintf_mdi(stddeb, "MDI_SwitchActiveChild: from "NPFMT", to "NPFMT"\n",childHwnd,hwndTo);

    if ( !hwndTo ) return; 

    hwndPrev = ci->hwndActiveChild;

    if ( hwndTo != hwndPrev )
	{
	  BOOL bSA = 0;

	  if( ci->flagChildMaximized )
	    {
	      bSA = 1; 
	      w->dwStyle &= ~WS_VISIBLE;
	    }

	  SetWindowPos( hwndTo, HWND_TOP, 0, 0, 0, 0, 
			SWP_NOMOVE | SWP_NOSIZE );
	  if( !wTo && hwndPrev )
	    {
	       SetWindowPos( hwndPrev, HWND_BOTTOM, 0, 0, 0, 0, 
		  	     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	    }

	  if( bSA )
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
		if( child == ci->flagChildMaximized )
		  {
		    MDI_RestoreFrameMenu(w_parent->hwndParent, child);
		    ci->flagChildMaximized = 0;
		    MDI_UpdateFrameText(w_parent->hwndParent,parent,TRUE,NULL);
		  }

                MDI_ChildActivate(w_parent,0);
	      }
	    MDI_MenuDeleteItem(w_parent, child);
	}
	
        ci->nActiveChildren--;

	/* WM_MDISETMENU ? */

        if (flagDestroy)
	   {
            DestroyWindow(child);
	    PostMessage(parent,WM_MDICALCCHILDSCROLL,0,0L);
	    ci->sbRecalc |= (SB_BOTH+1);
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

    dprintf_mdi(stddeb,"MDI_ChildActivate: "NPFMT"\n", hWndChild);

    if( GetActiveWindow() == clientPtr->hwndParent )
        isActiveFrameWnd = TRUE;
	
    /* deactivate prev. active child */
    if( wndPrev )
    {
	SendMessage( prevActiveWnd, WM_NCACTIVATE, FALSE, 0L );
#ifdef WINELIB32
        SendMessage( prevActiveWnd, WM_MDIACTIVATE, (WPARAM)prevActiveWnd, 
		     (LPARAM)hWndChild);
#else
        SendMessage( prevActiveWnd, WM_MDIACTIVATE, FALSE,
					    MAKELONG(hWndChild,prevActiveWnd));
#endif
        /* uncheck menu item */
       	if( clientInfo->hWindowMenu )
       	        CheckMenuItem( clientInfo->hWindowMenu,
       	                       wndPrev->wIDmenu, 0);
      }

    /* set appearance */
    if( clientInfo->flagChildMaximized )
      if( clientInfo->flagChildMaximized != hWndChild )
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
		SetFocus( clientInfo->self );
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
	    SendMessage( hWndChild, WM_NCACTIVATE, TRUE, 0L);
	    if( GetFocus() == clientInfo->self )
		SendMessage( clientInfo->self, WM_SETFOCUS, 
			    (WPARAM)clientInfo->self, 0L );
	    else
		SetFocus( clientInfo->self );
    }

#ifdef WINELIB32
    SendMessage( hWndChild, WM_MDIACTIVATE, (WPARAM)hWndChild,
		 (LPARAM)prevActiveWnd );
#else
    SendMessage( hWndChild, WM_MDIACTIVATE, TRUE,
				       MAKELONG(prevActiveWnd,hWndChild) );
#endif

    return 1;
}

/**********************************************************************
 *			MDI_BuildWCL
 *
 *  iTotal returns number of children available for tiling or cascading
 */
MDIWCL* MDI_BuildWCL(WND* clientWnd, int* iTotal)
{
    MDIWCL *listTop,*listNext;
    WND    *childWnd;

    if (!(listTop = (MDIWCL*)malloc( sizeof(MDIWCL) ))) return NULL;

    listTop->hChild = clientWnd->hwndChild;
    listTop->prev   = NULL;
    *iTotal 	    = 1;

    /* build linked list from top child to bottom */

    childWnd  =  WIN_FindWndPtr( listTop->hChild );
    while( childWnd && childWnd->hwndNext )
    {
	listNext = (MDIWCL*)malloc(sizeof(MDIWCL));
	
	if( !listNext )
	{
	    /* quit gracefully */
	    listNext = listTop->prev;
	    while( listTop )
	    {
                listNext = listTop->prev;
                free(listTop);
                listTop  = listNext;
	    }
	    dprintf_mdi(stddeb,"MDICascade: allocation failed\n");
	    return NULL;
	}
    
	if( (childWnd->dwStyle & WS_DISABLED) ||
	    (childWnd->dwStyle & WS_MINIMIZE) ||
	    !(childWnd->dwStyle & WS_VISIBLE)   )
	{
	    listTop->hChild = 0;
	    (*iTotal)--;
	}

	listNext->hChild = childWnd->hwndNext;
	listNext->prev   = listTop;
	listTop          = listNext;
	(*iTotal)++;

	childWnd  =  WIN_FindWndPtr( childWnd->hwndNext );
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
HBITMAP CreateMDIMenuBitmap(void)
{
 HDC 		hDCSrc  = CreateCompatibleDC(0);
 HDC		hDCDest	= CreateCompatibleDC(hDCSrc);
 HBITMAP	hbClose = LoadBitmap(0, MAKEINTRESOURCE(OBM_CLOSE) );
 HBITMAP	hbCopy,hb_src,hb_dest;

 hb_src = SelectObject(hDCSrc,hbClose);
 hbCopy = CreateCompatibleBitmap(hDCSrc,SYSMETRICS_CXSIZE, SYSMETRICS_CYSIZE);
 hb_dest = SelectObject(hDCDest,hbCopy);

 BitBlt(hDCDest, 0, 0, SYSMETRICS_CXSIZE, SYSMETRICS_CYSIZE,
	hDCSrc, SYSMETRICS_CXSIZE, 0, SRCCOPY);
 
 SelectObject(hDCSrc,hb_src);
 SelectObject(hDCDest,hb_dest);

 DeleteObject(hbClose);

 DeleteDC(hDCDest);
 DeleteDC(hDCSrc);

 return hbCopy;
}

/**********************************************************************
 *				MDICascade
 */
LONG MDICascade(HWND parent, MDICLIENTINFO *ci)
{
    WND		 *clientWnd;
    MDIWCL	 *listTop,*listPrev;
    RECT          rect;
    int           spacing, xsize, ysize;
    int		  x, y;
    int		  iToPosition = 0;

    if (ci->flagChildMaximized)
	ShowWindow( ci->flagChildMaximized, SW_NORMAL);

    if (ci->nActiveChildren == 0) return 0;

    GetClientRect(parent, &rect);
    spacing = GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFRAME);
    ysize   = rect.bottom - 8 * spacing;
    xsize   = rect.right  - 8 * spacing;
    
    dprintf_mdi(stddeb, 
      "MDICascade: Client wnd at (%ld,%ld) - (%ld,%ld), spacing %d\n", 
      (LONG)rect.left, (LONG)rect.top, (LONG)rect.right, (LONG)rect.bottom,
      spacing);
    
    clientWnd =  WIN_FindWndPtr( parent );

    listTop   =  MDI_BuildWCL(clientWnd,&iToPosition); 

    if( !listTop ) return 0;

    x = 0;
    y = 0;

    /* walk list and move windows */
    while ( listTop )
    {
	dprintf_mdi(stddeb, "MDICascade: move "NPFMT" to (%d,%d) size [%d,%d]\n", 
		listTop->hChild, x, y, xsize, ysize);

	if( listTop->hChild )
          {
	   SetWindowPos(listTop->hChild, 0, x, y, xsize, ysize, 
		     SWP_DRAWFRAME | SWP_NOACTIVATE | SWP_NOZORDER);

	x += spacing;
	y += spacing;
	  }
	
	listPrev = listTop->prev;
	free(listTop);
	listTop = listPrev;
    }

    if( iToPosition < ci->nActiveChildren )
	ArrangeIconicWindows( parent );

    return 0;
}

/**********************************************************************
 *					MDITile
 *
 */
LONG MDITile(HWND parent, MDICLIENTINFO *ci,WORD wParam)
{
    WND		 *wndClient = WIN_FindWndPtr(parent);
    MDIWCL       *listTop,*listPrev;
    RECT          rect;
    int           xsize, ysize;
    int		  x, y;
    int		  rows, columns;
    int           r, c;
    int           i;
    int		  iToPosition = 0;

    if (ci->flagChildMaximized)
	ShowWindow(ci->flagChildMaximized, SW_NORMAL);

    if (ci->nActiveChildren == 0) return 0;

    listTop = MDI_BuildWCL(wndClient, &iToPosition);

    dprintf_mdi(stddeb,"MDITile: %i windows to tile\n",iToPosition);

    if( !listTop ) return 0;

    /* tile children */
    if ( iToPosition )
    {

    	GetClientRect(parent, &rect);

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
                
                if( listTop->hChild )
                {
                    SetWindowPos(listTop->hChild, 0, x, y, xsize, ysize, 
				 SWP_DRAWFRAME | SWP_NOACTIVATE | SWP_NOZORDER);
		    y += ysize;
                }
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
        ArrangeIconicWindows( parent );

    return 0;
}

/* ----------------------- Frame window ---------------------------- */


/**********************************************************************
 *					MDI_AugmentFrameMenu
 */
BOOL MDI_AugmentFrameMenu(MDICLIENTINFO* ci, HWND hFrame, HWND hChild)
{
 WND*	frame = WIN_FindWndPtr(hFrame);
 WND*	child = WIN_FindWndPtr(hChild);
 HMENU  hSysPopup = 0;

 dprintf_mdi(stddeb,"MDI_AugmentFrameMenu: frame "NPFMT",child "NPFMT"\n",hFrame,hChild);

 if( !frame->wIDmenu || !child->hSysMenu ) return 0; 
 
 hSysPopup = GetSystemMenu(hChild,FALSE);

 dprintf_mdi(stddeb,"got popup "NPFMT"\n in sysmenu "NPFMT"",hSysPopup,child->hSysMenu);
 
 if( !InsertMenu(frame->wIDmenu,0,MF_BYPOSITION | MF_BITMAP | MF_POPUP,
                 hSysPopup, (SEGPTR)(DWORD)ci->obmClose) )
      return 0;

 if( !AppendMenu(frame->wIDmenu,MF_HELP | MF_BITMAP,
                 SC_RESTORE, (SEGPTR)(DWORD)ci->obmRestore) )
   {
      RemoveMenu(frame->wIDmenu,0,MF_BYPOSITION);
      return 0;
   }

 EnableMenuItem(hSysPopup, SC_SIZE, MF_BYCOMMAND | MF_GRAYED);
 EnableMenuItem(hSysPopup, SC_MOVE, MF_BYCOMMAND | MF_GRAYED);
 EnableMenuItem(hSysPopup, SC_MAXIMIZE, MF_BYCOMMAND | MF_GRAYED);

 child->dwStyle &= ~WS_SYSMENU;

 /* redraw frame */
 SetWindowPos(hFrame, 0,0,0,0,0, SWP_FRAMECHANGED | SWP_NOSIZE | 
                                 SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER );

 return 1;
}

/**********************************************************************
 *					MDI_RestoreFrameMenu
 */
BOOL MDI_RestoreFrameMenu(HWND hFrame, HWND hChild)
{
 WND*	frameWnd = WIN_FindWndPtr(hFrame);
 WND*   child    = WIN_FindWndPtr(hChild);
 INT	nItems   = GetMenuItemCount(frameWnd->wIDmenu) - 1;

 dprintf_mdi(stddeb,"MDI_RestoreFrameMenu: for child "NPFMT"\n",hChild);

 if( GetMenuItemID(frameWnd->wIDmenu,nItems) != SC_RESTORE )
     return 0; 

 child->dwStyle |= WS_SYSMENU;

 RemoveMenu(frameWnd->wIDmenu,0,MF_BYPOSITION);
 DeleteMenu(frameWnd->wIDmenu,nItems-1,MF_BYPOSITION);

  /* redraw frame */
 SetWindowPos(hChild, 0,0,0,0,0, SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE |
                                 SWP_NOACTIVATE | SWP_NOZORDER );
 return 1;
}

/**********************************************************************
 *				        MDI_UpdateFrameText
 *
 * used when child window is maximized/restored 
 *
 * Note: lpTitle can be NULL
 */
void MDI_UpdateFrameText(HWND hFrame, HWND hClient, BOOL repaint, LPCSTR lpTitle)
{
 char   lpBuffer[MDI_MAXTITLELENGTH+1];
 LPSTR	lpText    = NULL;
 WND* 	clientWnd = WIN_FindWndPtr(hClient);
 WND* 	frameWnd  = WIN_FindWndPtr(hFrame);

 MDICLIENTINFO *ci = (MDICLIENTINFO *) clientWnd->wExtra;

 dprintf_mdi(stddeb, "MDI: repaint %i, frameText %s\n", repaint, (lpTitle)?lpTitle:"NULL");

 /* store new "default" title if lpTitle is not NULL */
 if( lpTitle )
   {
	if( ci->hFrameTitle )
	    USER_HEAP_FREE( ci->hFrameTitle );
	ci->hFrameTitle = USER_HEAP_ALLOC( strlen(lpTitle) + 1 );
	lpText = (LPSTR) USER_HEAP_LIN_ADDR( ci->hFrameTitle );
	strcpy( lpText, lpTitle );
   }
 else
  lpText = (LPSTR) USER_HEAP_LIN_ADDR(ci->hFrameTitle);

 if( ci->hFrameTitle )
   {
     WND* childWnd = WIN_FindWndPtr( ci->flagChildMaximized );     

     if( childWnd && childWnd->hText )
       {
	 /* combine frame title and child title if possible */

	 LPCSTR lpBracket  = " - [";
	 LPCSTR lpChildText = (LPCSTR) USER_HEAP_LIN_ADDR( childWnd->hText );
 
	 int	i_frame_text_length = strlen(lpText);
	 int    i_child_text_length = strlen(lpChildText);

	 strncpy( lpBuffer, lpText, MDI_MAXTITLELENGTH);
	 lpBuffer[MDI_MAXTITLELENGTH] = '\0';

	 if( i_frame_text_length + 5 < MDI_MAXTITLELENGTH )
	   {
	     strcat( lpBuffer, lpBracket );

	     if( i_frame_text_length + i_child_text_length + 5 < MDI_MAXTITLELENGTH )
		{
		   strcat( lpBuffer, lpChildText );
	         *(short*)(lpBuffer + i_frame_text_length + i_child_text_length + 4) = (short)']';
		}
	     else
		{
		   memcpy( lpBuffer + i_frame_text_length + 4, 
		           lpChildText, 
			   MDI_MAXTITLELENGTH - i_frame_text_length - 4);
		 *(short*)(lpBuffer + MDI_MAXTITLELENGTH - 1) = (short)']';
		}
	   }
       }
     else
       {
         strncpy(lpBuffer, lpText, MDI_MAXTITLELENGTH );
	 lpBuffer[MDI_MAXTITLELENGTH]='\0';
       }
   }
 else
   lpBuffer[0] = '\0';

 if( frameWnd->hText )
     USER_HEAP_FREE( frameWnd->hText );
 
 frameWnd->hText = USER_HEAP_ALLOC( strlen(lpBuffer) + 1 );
 lpText = (LPSTR) USER_HEAP_LIN_ADDR( frameWnd->hText );
 strcpy( lpText, lpBuffer );

 if( frameWnd->window )
     XStoreName( display, frameWnd->window, lpBuffer );

 if( repaint == MDI_REPAINTFRAME)
     SetWindowPos(hFrame, 0,0,0,0,0, SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE |
                                     SWP_NOACTIVATE | SWP_NOZORDER );
}


/* ----------------------------- Interface ---------------------------- */


/**********************************************************************
 *					MDIClientWndProc
 *
 * This function is the handler for all MDI requests.
 */
LRESULT MDIClientWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPCREATESTRUCT       cs;
    LPCLIENTCREATESTRUCT ccs;
    MDICLIENTINFO       *ci;
    RECT		 rect;
    WND                 *w 	  = WIN_FindWndPtr(hwnd);
    WND			*frameWnd = WIN_FindWndPtr( w->hwndParent );

    ci = (MDICLIENTINFO *) w->wExtra;
    
    switch (message)
    {
      case WM_CREATE:
	cs                      = (LPCREATESTRUCT) PTR_SEG_TO_LIN(lParam);
	ccs                     = (LPCLIENTCREATESTRUCT) PTR_SEG_TO_LIN(cs->lpCreateParams);

	ci->hWindowMenu         = ccs->hWindowMenu;
	ci->idFirstChild        = ccs->idFirstChild;
	ci->flagChildMaximized  = 0;
	ci->hFrameTitle		= frameWnd->hText;
	ci->sbStop		= 0;
	ci->self		= hwnd;
	ci->obmClose		= CreateMDIMenuBitmap();
	ci->obmRestore		= LoadBitmap(0, MAKEINTRESOURCE(OBM_RESTORE));
	w->dwStyle             |= WS_CLIPCHILDREN;
	frameWnd->hText		= 0;	/* will be restored in UpdateFrameText */

	MDI_UpdateFrameText( w->hwndParent, hwnd, MDI_NOFRAMEREPAINT, NULL);

	AppendMenu(ccs->hWindowMenu,MF_SEPARATOR,0,(SEGPTR)0);

	GetClientRect(w->hwndParent, &rect);
	NC_HandleNCCalcSize(hwnd, (NCCALCSIZE_PARAMS*) &rect);
	w->rectClient = rect;

	return 0;
      
      case WM_DESTROY:
	if( ci->flagChildMaximized )
	  MDI_RestoreFrameMenu(hwnd, w->hwndParent);

	if(ci->obmClose)   DeleteObject(ci->obmClose);
	if(ci->obmRestore) DeleteObject(ci->obmRestore);

	ci->idFirstChild = GetMenuItemCount(ci->hWindowMenu) - 1;
	ci->nActiveChildren++; 			/* to delete a separator */

	while( ci->nActiveChildren-- )
	     DeleteMenu(ci->hWindowMenu,MF_BYPOSITION,ci->idFirstChild--);

	return 0;

      case WM_MDIACTIVATE:
        if( ci->hwndActiveChild != (HWND)wParam )
	    SetWindowPos((HWND)wParam, 0,0,0,0,0, SWP_NOSIZE | SWP_NOMOVE ); 
	return 0;

      case WM_MDICASCADE:
	return MDICascade(hwnd, ci);

      case WM_MDICREATE:
	return (LONG)MDICreateChild(w, ci, hwnd, lParam );

      case WM_MDIDESTROY:
	return (LONG)MDIDestroyChild(w, ci, hwnd, (HWND)wParam, TRUE);

      case WM_MDIGETACTIVE:
	return ((LONG) ci->hwndActiveChild | 
		((LONG) (ci->flagChildMaximized>0) << 16));

      case WM_MDIICONARRANGE:
	       ci->sbStop = TRUE;
	       MDIIconArrange(hwnd);
	       ci->sbStop = FALSE;
	       SendMessage(hwnd,WM_MDICALCCHILDSCROLL,0,0L);
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
	return (LRESULT)MDISetMenu(hwnd, FALSE, (HMENU)wParam, (HMENU)lParam);
#else
	return (LRESULT)MDISetMenu(hwnd, wParam, LOWORD(lParam), HIWORD(lParam));
#endif
	
      case WM_MDITILE:
	ci->sbStop = TRUE;
	ShowScrollBar(hwnd,SB_BOTH,FALSE);
	MDITile(hwnd, ci,wParam);
        ci->sbStop = FALSE;
        return 0;

      case WM_VSCROLL:
      case WM_HSCROLL:
	ci->sbStop = TRUE;
        ScrollChildren(hwnd,message,wParam,lParam);
	ci->sbStop = FALSE;
        return 0;

      case WM_SETFOCUS:
	if( ci->hwndActiveChild )
	  {
	   w = WIN_FindWndPtr( ci->hwndActiveChild );
	   if( !(w->dwStyle & WS_MINIMIZE) )
	       SetFocus( ci->hwndActiveChild );
	  } 
	return 0;
	
      case WM_NCACTIVATE:
        if( ci->hwndActiveChild )
	     SendMessage(ci->hwndActiveChild, message, wParam, lParam);
	break;
	
      case WM_PARENTNOTIFY:
	if (wParam == WM_LBUTTONDOWN && (ci->hwndHitTest != ci->hwndActiveChild) )
	     SetWindowPos(ci->hwndHitTest, 0,0,0,0,0, SWP_NOSIZE | SWP_NOMOVE );
	break;

      case WM_SIZE:
	if( ci->flagChildMaximized )
	  {
	     WND*	child = WIN_FindWndPtr(ci->flagChildMaximized);
	     RECT	rect  = { 0, 0, LOWORD(lParam), HIWORD(lParam) };

	     AdjustWindowRectEx(&rect, child->dwStyle, 0, child->dwExStyle);
	     MoveWindow(ci->flagChildMaximized, rect.left, rect.top,
			rect.right - rect.left, rect.bottom - rect.top, 1);
	  }
	else
	  {
	     PostMessage(hwnd,WM_MDICALCCHILDSCROLL,0,0L);
	     ci->sbRecalc |= (SB_BOTH+1);
	  }
	break;

      case WM_MDICALCCHILDSCROLL:
	if( !ci->sbStop )
	  if( ci->sbRecalc )
	    {
	      CalcChildScroll(hwnd, ci->sbRecalc-1);
	      ci->sbRecalc = 0;
	    }
	return 0;
    }
    
    return DefWindowProc(hwnd, message, wParam, lParam);
}

/**********************************************************************
 *					DefFrameProc (USER.445)
 *
 */
LRESULT DefFrameProc(HWND hwnd, HWND hwndMDIClient, UINT message, 
		     WPARAM wParam, LPARAM lParam)
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

	    /* check for possible system command codes */

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
		       if( ci->flagChildMaximized )
			   return SendMessage( ci->flagChildMaximized, WM_SYSCOMMAND,
					       wParam, lParam);
		  }
	      }
	    else
	      {
	    	childHwnd = MDI_GetChildByID( WIN_FindWndPtr(hwndMDIClient),
                                          wParam );
 	    	if( childHwnd )
	            SendMessage(hwndMDIClient, WM_MDIACTIVATE, (WPARAM)childHwnd , 0L);
	      }
	    break;

	  case WM_NCACTIVATE:
	    SendMessage(hwndMDIClient, message, wParam, lParam);
	    break;

	  case WM_SETTEXT:
	    MDI_UpdateFrameText(hwnd, hwndMDIClient, 
				      MDI_REPAINTFRAME, 
			             (LPCSTR)PTR_SEG_TO_LIN(lParam));
	    return 0;
	
	  case WM_SETFOCUS:
	    SetFocus(hwndMDIClient);
	    break;

	  case WM_SIZE:
	    MoveWindow(hwndMDIClient, 0, 0, 
		       LOWORD(lParam), HIWORD(lParam), TRUE);
	    break;
	}
    }
    
    return DefWindowProc(hwnd, message, wParam, lParam);
}

/**********************************************************************
 *					DefMDIChildProc (USER.447)
 *
 */
#ifdef WINELIB32
LONG DefMDIChildProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
#else
LONG DefMDIChildProc(HWND hwnd, WORD message, WORD wParam, LONG lParam)
#endif
{
    MDICLIENTINFO       *ci;
    WND                 *clientWnd;

    clientWnd  = WIN_FindWndPtr(GetParent(hwnd));
    ci         = (MDICLIENTINFO *) clientWnd->wExtra;

    switch (message)
    {
      case WM_NCHITTEST:
	ci->hwndHitTest = hwnd;
	break;
	
      case WM_SETTEXT:
	DefWindowProc(hwnd, message, wParam, lParam);
	MDI_MenuModifyItem(clientWnd,hwnd);
	if( ci->flagChildMaximized == hwnd )
	    MDI_UpdateFrameText( clientWnd->hwndParent, ci->self,
				 MDI_REPAINTFRAME, NULL );
        return 0;

      case WM_CLOSE:
	SendMessage(ci->self,WM_MDIDESTROY,(WPARAM)hwnd,0L);
	return 0;

      case WM_SETFOCUS:
	if( ci->hwndActiveChild != hwnd )
	    MDI_ChildActivate(clientWnd, hwnd);
	break;

      case WM_CHILDACTIVATE:
	MDI_ChildActivate(clientWnd, hwnd);
	return 0;

      case WM_NCPAINT:
	dprintf_mdi(stddeb,"DefMDIChildProc: WM_NCPAINT for "NPFMT", active "NPFMT"\n",
					     hwnd, ci->hwndActiveChild );
	break;

      case WM_SYSCOMMAND:
	switch( wParam )
	  {
		case SC_MOVE:
		     if( ci->flagChildMaximized == hwnd) return 0;
		     break;
		case SC_MAXIMIZE:
		     if( ci->flagChildMaximized == hwnd) 
			 return SendMessage( clientWnd->hwndParent, message, wParam, lParam);
		     break;
		case SC_NEXTWINDOW:
		     SendMessage( ci->self, WM_MDINEXT, 0, 0);
		     return 0;
		case SC_PREVWINDOW:
		     SendMessage( ci->self, WM_MDINEXT, 0, 1);
		     return 0;
	  }
	break;
	
      case WM_GETMINMAXINFO:
	MDI_ChildGetMinMaxInfo(clientWnd, hwnd, (MINMAXINFO*) PTR_SEG_TO_LIN(lParam));
	return 0;

      case WM_SETVISIBLE:
         if( !ci->sbStop && !ci->flagChildMaximized)
          {
            PostMessage(ci->self,WM_MDICALCCHILDSCROLL,0,0L);
            ci->sbRecalc |= (SB_BOTH+1);
          }
	break;

      case WM_SIZE:
	/* do not change */

	if( ci->hwndActiveChild == hwnd && wParam != SIZE_MAXIMIZED )
	  {
  	    ci->flagChildMaximized = 0;
	    
	    MDI_RestoreFrameMenu( clientWnd->hwndParent, hwnd);
            MDI_UpdateFrameText( clientWnd->hwndParent, ci->self,
                                 MDI_REPAINTFRAME, NULL );
	  }

	if( wParam == SIZE_MAXIMIZED )
	  {
	    HWND hMaxChild = ci->flagChildMaximized;

	    if( hMaxChild == hwnd ) break;

	    if( hMaxChild)
	      {	    
	       SendMessage( hMaxChild, WM_SETREDRAW, FALSE, 0L );

	       MDI_RestoreFrameMenu( clientWnd->hwndParent, hMaxChild);
	       ShowWindow( hMaxChild, SW_SHOWNOACTIVATE);

	       SendMessage( hMaxChild, WM_SETREDRAW, TRUE, 0L );
	      }

	    ci->flagChildMaximized = hwnd; /* !!! */

	    MDI_AugmentFrameMenu( ci, clientWnd->hwndParent, hwnd);
	    MDI_UpdateFrameText( clientWnd->hwndParent, ci->self,
				 MDI_REPAINTFRAME, NULL ); 
	  }

	if( wParam == SIZE_MINIMIZED )
	  {
	    HWND switchTo = MDI_GetWindow(clientWnd, hwnd, 0);

	    if( switchTo )
	        SendMessage( switchTo, WM_CHILDACTIVATE, 0, 0L);
	  }
	    
        if( !ci->sbStop )
          {
            PostMessage(ci->self,WM_MDICALCCHILDSCROLL,0,0L);
            ci->sbRecalc |= (SB_BOTH+1);
          }
	break;

      case WM_MENUCHAR:
      case WM_NEXTMENU:
	   /* set current menu to child system menu */

	break;	
    }
	
    return DefWindowProc(hwnd, message, wParam, lParam);
}

/**********************************************************************
 *					TranslateMDISysAccel (USER.451)
 *
 */
BOOL TranslateMDISysAccel(HWND hwndClient, LPMSG msg)
{
  WND* clientWnd = WIN_FindWndPtr( hwndClient);
  WND* wnd;
  MDICLIENTINFO       *ci     = NULL;
  WPARAM	       wParam = 0;

  if( (msg->message != WM_KEYDOWN && msg->message != WM_SYSKEYDOWN) || !clientWnd )
    return 0;

  ci = (MDICLIENTINFO*) clientWnd->wExtra;
  wnd = WIN_FindWndPtr(ci->hwndActiveChild);
 
  if( !wnd ) return 0;
  
  if( wnd->dwStyle & WS_DISABLED ) return 0;
   
  if( GetKeyState(VK_CONTROL) && !GetKeyState(VK_MENU) )
    switch( msg->wParam )
      {
	case VK_F6:
	case VK_SEPARATOR:
	     wParam = (GetKeyState(VK_SHIFT))? SC_NEXTWINDOW: SC_PREVWINDOW;
	     break;
	case VK_RBUTTON:
	     wParam = SC_CLOSE; 
	default:
	     return 0;
      }

  dprintf_mdi(stddeb,"TranslateMDISysAccel: wParam = "NPFMT"\n", wParam);

  SendMessage(ci->hwndActiveChild,WM_SYSCOMMAND, wParam, (LPARAM)msg->wParam);
  return 1;
}


/***********************************************************************
 *           CalcChildScroll   (USER.462)
 */
void CalcChildScroll( HWND hwnd, WORD scroll )
{
    RECT childRect, clientRect;
    HWND hwndChild;

    GetClientRect( hwnd, &clientRect );
    SetRectEmpty( &childRect );
    hwndChild = GetWindow( hwnd, GW_CHILD );
    while (hwndChild)
    {
        WND *wndPtr = WIN_FindWndPtr( hwndChild );
        UnionRect( &childRect, &wndPtr->rectWindow, &childRect );
        hwndChild = wndPtr->hwndNext;
    }
    UnionRect( &childRect, &clientRect, &childRect );

    if ((scroll == SB_HORZ) || (scroll == SB_BOTH))
    {
        SetScrollRange( hwnd, SB_HORZ, childRect.left,
                        childRect.right - clientRect.right, FALSE );
        SetScrollPos( hwnd, SB_HORZ, clientRect.left - childRect.left, TRUE );
    }
    if ((scroll == SB_VERT) || (scroll == SB_BOTH))
    {
        SetScrollRange( hwnd, SB_VERT, childRect.top,
                        childRect.bottom - clientRect.bottom, FALSE );
        SetScrollPos( hwnd, SB_HORZ, clientRect.top - childRect.top, TRUE );
    }
}

/***********************************************************************
 *           ScrollChildren   (USER.463)
 */
void ScrollChildren(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
 WND	*wndPtr = WIN_FindWndPtr(hWnd);
 short 	 newPos=-1;
 short 	 curPos;
 short 	 length;
 INT 	 minPos;
 INT 	 maxPos;
 short   shift;

 if( !wndPtr ) return;

 if( uMsg == WM_HSCROLL )
   {
     GetScrollRange(hWnd,SB_HORZ,&minPos,&maxPos);
     curPos = GetScrollPos(hWnd,SB_HORZ);
     length = (wndPtr->rectClient.right - wndPtr->rectClient.left)/2;
     shift = SYSMETRICS_CYHSCROLL;
   }
 else if( uMsg == WM_VSCROLL )
	{
	  GetScrollRange(hWnd,SB_VERT,&minPos,&maxPos);
	  curPos = GetScrollPos(hWnd,SB_VERT);
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

 SetScrollPos(hWnd, (uMsg == WM_VSCROLL)?SB_VERT:SB_HORZ , newPos, TRUE);

 if( uMsg == WM_VSCROLL )
     ScrollWindow(hWnd ,0 ,curPos - newPos, NULL, NULL);
 else
     ScrollWindow(hWnd ,curPos - newPos, 0, NULL, NULL);
}

