/* MDI.C
 *
 * Copyright 1994, Bob Amstadt
 *           1995, Alex Korobka
 *
 * This file contains routines to support MDI features.
 *
 * Notes: Windows keeps ID of MDI menu item in the wIDenu field
 *        of corresponding MDI child.
 *
 *	  Basic child activation routine is MDI_ChildActivate and 
 *        SetWindowPos(childHwnd,...) implicitly calls it if SWP_NOACTIVATE i
 *        is not used.
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
#include "sysmetrics.h"
#include "stddebug.h"
#include "debug.h"

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
 HWND	hWnd   = mdiClient->hwndChild;
 WND* 	wndPtr = WIN_FindWndPtr( hWnd );
    
 if( !wndPtr ) return 0;

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
                       wndPtr->wIDmenu,(LPSTR)buffer);
}

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

 if( lpWndText )
     strncpy(buffer + n, lpWndText, sizeof(buffer) - n - 1);

 n    = GetMenuState(clientInfo->hWindowMenu,wndPtr->wIDmenu ,MF_BYCOMMAND); 
 bRet = ModifyMenu(clientInfo->hWindowMenu , wndPtr->wIDmenu , 
                   MF_BYCOMMAND | MF_STRING, wndPtr->wIDmenu ,(LPSTR)buffer );
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
		   index - 1 ,(LPSTR)buffer ); 
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
    AppendMenu(ci->hWindowMenu ,MF_STRING ,wIDmenu, (LPSTR)&chDef);

    hwnd = CreateWindow( cs->szClass, cs->szTitle,
			  WS_CHILD | WS_BORDER | WS_CAPTION | WS_CLIPSIBLINGS |
			  WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU |
			  WS_THICKFRAME | WS_VISIBLE | cs->style,
			  cs->x, cs->y, cs->cx, cs->cy, parent, 
                         (HMENU) wIDmenu, w->hInstance, (SEGPTR)lParam);

    if (hwnd)
    {
	ci->nActiveChildren++;
	MDI_MenuModifyItem(w ,hwnd); 

	/* FIXME: at this point NC area of hwnd stays inactive */
    }
    else
	DeleteMenu(ci->hWindowMenu,wIDmenu,MF_BYCOMMAND);
	
    return hwnd;
}

/**********************************************************************
 *			MDI_SwitchActiveChild
 * 
 * Notes: SetWindowPos sends WM_CHILDACTIVATE to the child window that is
 *        being activated 
 *
 *        Ideally consecutive SetWindowPos should be replaced by 
 *        BeginDeferWindowPos/EndDeferWindowPos but currently it doesn't
 *        matter.
 *
 *	  wTo is basically lParam of WM_MDINEXT message
 */
void MDI_SwitchActiveChild(HWND clientHwnd, HWND childHwnd, WORD wTo )
{
    WND		  *w	     = WIN_FindWndPtr(clientHwnd);
    HWND	   hwndTo    = MDI_GetWindow(w,childHwnd,wTo);
    HWND	   hwndPrev;
    MDICLIENTINFO *ci;

    
    ci = (MDICLIENTINFO *) w->wExtra;

    dprintf_mdi(stddeb, "MDI_SwitchActiveChild: "NPFMT", %i\n",childHwnd,wTo);

    if ( !childHwnd || !hwndTo ) return; 

    hwndPrev = ci->hwndActiveChild;

    if ( hwndTo != hwndPrev )
	{
	    if (ci->flagChildMaximized)
	    {
		RECT rectOldRestore, rect;

		w = WIN_FindWndPtr(hwndTo);
		
		/* save old window dimensions */
		rectOldRestore = ci->rectRestore;
		GetWindowRect(hwndTo, &ci->rectRestore);

		rect.top    = (ci->rectMaximize.top -
			       (w->rectClient.top - w->rectWindow.top));
		rect.bottom = (ci->rectMaximize.bottom + 
			       (w->rectWindow.bottom - w->rectClient.bottom));
		rect.left   = (ci->rectMaximize.left - 
			       (w->rectClient.left - w->rectWindow.left));
		rect.right  = (ci->rectMaximize.right +
			       (w->rectWindow.right - w->rectClient.right));
		w->dwStyle |= WS_MAXIMIZE;

		/* maximize it */
		ci->flagChildMaximized = childHwnd; /* prevent maximization
						     * in MDI_ChildActivate
						     */

		SetWindowPos( hwndTo, HWND_TOP, rect.left, rect.top, 
			     rect.right - rect.left + 1,
			     rect.bottom - rect.top + 1, 0);

		SendMessage( hwndTo, WM_SIZE, SIZE_MAXIMIZED,
			    MAKELONG(w->rectClient.right-w->rectClient.left,
				     w->rectClient.bottom-w->rectClient.top));

		w = WIN_FindWndPtr(hwndPrev);

	        if( w )
		  {
		w->dwStyle &= ~WS_MAXIMIZE;

		     /* push hwndPrev to the bottom if needed */
		     if( !wTo )
		         SetWindowPos(hwndPrev, HWND_BOTTOM, 
			     rectOldRestore.left, rectOldRestore.top, 
			     rectOldRestore.right - rectOldRestore.left + 1, 
			     rectOldRestore.bottom - rectOldRestore.top + 1,
			          SWP_NOACTIVATE );
	          }
	    }
	    else
	    {
		SetWindowPos( hwndTo, HWND_TOP, 0, 0, 0, 0, 
			     SWP_NOMOVE | SWP_NOSIZE );
		if( !wTo && hwndPrev )
		{
		    SetWindowPos( hwndPrev, HWND_BOTTOM, 0, 0, 0, 0, 
				 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		}
	    }
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
                MDI_ChildActivate(w_parent,0);

	    MDI_MenuDeleteItem(w_parent, child);
	}
	
        ci->nActiveChildren--;

        if( ci->flagChildMaximized == child )
            ci->flagChildMaximized = 1;

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
 *					MDIMaximizeChild
 */
LONG MDIMaximizeChild(HWND parent, HWND child, MDICLIENTINFO *ci)
{

    WND *w = WIN_FindWndPtr(child);
    RECT rect;
    
    if( !SendMessage( child, WM_QUERYOPEN, 0, 0L) )
	 return 0;
    
    ci->rectRestore = w->rectWindow;

    rect.top    = (ci->rectMaximize.top -
		   (w->rectClient.top - w->rectWindow.top));
    rect.bottom = (ci->rectMaximize.bottom + 
		   (w->rectWindow.bottom - w->rectClient.bottom));
    rect.left   = (ci->rectMaximize.left - 
		   (w->rectClient.left - w->rectWindow.left));
    rect.right  = (ci->rectMaximize.right +
		   (w->rectWindow.right - w->rectClient.right));
    w->dwStyle |= WS_MAXIMIZE;

    SetWindowPos(child, 0, rect.left, rect.top, 
		 rect.right - rect.left + 1, rect.bottom - rect.top + 1, 0);
    
    ci->flagChildMaximized = child;
    
    SendMessage(child, WM_SIZE, SIZE_MAXIMIZED,
		MAKELONG(w->rectClient.right-w->rectClient.left,
			 w->rectClient.bottom-w->rectClient.top));

    SendMessage(GetParent(parent), WM_NCPAINT, 0, 0);

    return 0;
}

/**********************************************************************
 *					MDIRestoreChild
 */
LONG MDIRestoreChild(HWND parent, MDICLIENTINFO *ci)
{
    HWND    hWnd;

    hWnd = ci->hwndActiveChild;

    dprintf_mdi(stddeb,"MDIRestoreChild: restore "NPFMT"\n", hWnd);

    ci->flagChildMaximized = FALSE;

    ShowWindow(hWnd, SW_RESTORE);		/* display the window */

    hWnd = GetParent(parent);

    SendMessage(hWnd,WM_NCPAINT , 0, 0);

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
        SendMessage( prevActiveWnd, WM_MDIACTIVATE, FALSE,
					    MAKELONG(hWndChild,prevActiveWnd));
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
		  MDIMaximizeChild(GetParent(hWndChild),hWndChild,clientInfo);
	        }

    clientInfo->hwndActiveChild = hWndChild;

    /* check if we have any children left */
    if( !hWndChild )
	{
	    if( isActiveFrameWnd )
		SetFocus( GetParent(hWndChild) );
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
	    if( GetFocus() == GetParent(hWndChild) )
		SendMessage( GetParent(hWndChild), WM_SETFOCUS, 
			     GetParent(hWndChild), 0L );
	    else
		SetFocus( GetParent(hWndChild) );
    }

    SendMessage( hWndChild, WM_MDIACTIVATE, TRUE,
				       MAKELONG(prevActiveWnd,hWndChild) );

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
	    fprintf(stdnimp,"MDICascade: allocation failed\n");
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

/**********************************************************************
 *					MDICascade
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
	MDIRestoreChild(parent, ci);

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

    return 0;
}

/**********************************************************************
 *					MDITile
 *
 */
LONG MDITile(HWND parent, MDICLIENTINFO *ci)
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
	MDIRestoreChild(parent, ci);

    if (ci->nActiveChildren == 0) return 0;

    listTop = MDI_BuildWCL(wndClient, &iToPosition);

    dprintf_mdi(stddeb,"MDITile: %i windows to tile\n",iToPosition);

    if( !listTop ) return 0;

    GetClientRect(parent, &rect);

    rows    = (int) sqrt((double) iToPosition);
    columns = iToPosition / rows;

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
    
    /* free the rest if any */
    while( listTop ) {
	 listPrev = listTop->prev;
	 free(listTop);
	 listTop = listPrev; }

    return 0;
}

/**********************************************************************
 *					MDIHandleLButton
 */
BOOL MDIHandleLButton(HWND hwndFrame, HWND hwndClient, 
		      WORD wParam, LONG lParam)
{
    MDICLIENTINFO *ci;
    WND           *w;
    RECT           rect;
    WORD           x;

    w  = WIN_FindWndPtr(hwndClient);
    ci = (MDICLIENTINFO *) w->wExtra;

    if (wParam == HTMENU && ci->flagChildMaximized)
    {
	x = LOWORD(lParam);
	
	NC_GetInsideRect(hwndFrame, &rect);
	if (x < rect.left + SYSMETRICS_CXSIZE)
	{
	    SendMessage(ci->hwndActiveChild, WM_SYSCOMMAND, 
			SC_CLOSE, lParam);
	    return TRUE;
	}
	else if (x >= rect.right - SYSMETRICS_CXSIZE)
	{
	    SendMessage(ci->hwndActiveChild, WM_SYSCOMMAND, 
			SC_RESTORE, lParam);
	    return TRUE;
	}
    }

    return FALSE;
}

/**********************************************************************
 *					MDIPaintMaximized
 */
LONG MDIPaintMaximized(HWND hwndFrame, HWND hwndClient, WORD message,
		       WORD wParam, LONG lParam)
{
    static HBITMAP hbitmapClose     = 0;
    static HBITMAP hbitmapMaximized = 0;
    
    MDICLIENTINFO *ci;
    WND           *w;
    HDC           hdc, hdcMem;
    RECT          rect;
    WND           *wndPtr = WIN_FindWndPtr(hwndFrame);

    w  = WIN_FindWndPtr(hwndClient);
    ci = (MDICLIENTINFO *) w->wExtra;

    dprintf_mdi(stddeb, "MDIPaintMaximized: frame "NPFMT",  client "NPFMT
		",  max flag %d,  menu %04x\n", hwndFrame, hwndClient, 
		ci->flagChildMaximized, wndPtr ? wndPtr->wIDmenu : 0);

    if (ci->flagChildMaximized && wndPtr && wndPtr->wIDmenu != 0)
    {
	NC_DoNCPaint(hwndFrame, wParam, TRUE);

	hdc = GetDCEx(hwndFrame, 0, DCX_CACHE | DCX_WINDOW);
	if (!hdc) return 0;

	hdcMem = CreateCompatibleDC(hdc);

	if (hbitmapClose == 0)
	{
	    hbitmapClose     = LoadBitmap(0, MAKEINTRESOURCE(OBM_OLD_CLOSE));
	    hbitmapMaximized = LoadBitmap(0, MAKEINTRESOURCE(OBM_RESTORE));
	}

	dprintf_mdi(stddeb, 
		    "MDIPaintMaximized: hdcMem "NPFMT", close bitmap "NPFMT", "
		    "maximized bitmap "NPFMT"\n",
		    hdcMem, hbitmapClose, hbitmapMaximized);

	NC_GetInsideRect(hwndFrame, &rect);
	rect.top += (wndPtr->dwStyle & WS_CAPTION) ? SYSMETRICS_CYSIZE + 1 : 0;
	SelectObject(hdcMem, hbitmapClose);
	BitBlt(hdc, rect.left, rect.top + 1, 
	       SYSMETRICS_CXSIZE, SYSMETRICS_CYSIZE,
	       hdcMem, 1, 1, SRCCOPY);
	
	NC_GetInsideRect(hwndFrame, &rect);
	rect.top += (wndPtr->dwStyle & WS_CAPTION) ? SYSMETRICS_CYSIZE + 1 : 0;
	rect.left   = rect.right - SYSMETRICS_CXSIZE;
	SelectObject(hdcMem, hbitmapMaximized);
	BitBlt(hdc, rect.left, rect.top + 1, 
	       SYSMETRICS_CXSIZE, SYSMETRICS_CYSIZE,
	       hdcMem, 1, 1, SRCCOPY);
	
	NC_GetInsideRect(hwndFrame, &rect);
	rect.top += (wndPtr->dwStyle & WS_CAPTION) ? SYSMETRICS_CYSIZE + 1 : 0;
	rect.left += SYSMETRICS_CXSIZE;
	rect.right -= SYSMETRICS_CXSIZE;
	rect.bottom = rect.top + SYSMETRICS_CYMENU;

	MENU_DrawMenuBar(hdc, &rect, hwndFrame, FALSE);
	
	DeleteDC(hdcMem);
	ReleaseDC(hwndFrame, hdc);
    }
    else
	return DefWindowProc(hwndFrame, message, wParam, lParam);

    return 0;
}

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
    WND                 *w;

    w  = WIN_FindWndPtr(hwnd);
    ci = (MDICLIENTINFO *) w->wExtra;
    
    switch (message)
    {
      case WM_CREATE:
	cs                      = (LPCREATESTRUCT) PTR_SEG_TO_LIN(lParam);
	ccs                     = (LPCLIENTCREATESTRUCT) PTR_SEG_TO_LIN(cs->lpCreateParams);
	ci->hWindowMenu         = ccs->hWindowMenu;
	ci->idFirstChild        = ccs->idFirstChild;
	ci->flagChildMaximized  = FALSE;
	ci->sbStop		= 0;

	w->dwStyle             |= WS_CLIPCHILDREN;

	AppendMenu(ccs->hWindowMenu,MF_SEPARATOR,0,NULL);

	GetClientRect(w->hwndParent, &ci->rectMaximize);
	MoveWindow(hwnd, 0, 0, 
		   ci->rectMaximize.right, ci->rectMaximize.bottom, 1);

	return 0;

      case WM_MDIACTIVATE:
	SetWindowPos(wParam,0,0,0,0,0, SWP_NOSIZE | SWP_NOMOVE );
	return 0;

      case WM_MDICASCADE:
	return MDICascade(hwnd, ci);

      case WM_MDICREATE:
	return (LONG)MDICreateChild(w, ci, hwnd, lParam );

      case WM_MDIDESTROY:
	return MDIDestroyChild(w, ci, hwnd, wParam, TRUE);

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
	return MDIMaximizeChild(hwnd, wParam, ci);

      case WM_MDINEXT:
	MDI_SwitchActiveChild(hwnd, (HWND)wParam, lParam);
	break;
	
      case WM_MDIRESTORE:
	return MDIRestoreChild(hwnd, ci);

      case WM_MDISETMENU:
	return MDISetMenu(hwnd, wParam, LOWORD(lParam), HIWORD(lParam));
	
      case WM_MDITILE:
	ci->sbStop = TRUE;
	ShowScrollBar(hwnd,SB_BOTH,FALSE);
	MDITile(hwnd, ci);
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
	if (wParam == WM_LBUTTONDOWN)
	     SetWindowPos(ci->hwndHitTest, 0,0,0,0,0, SWP_NOSIZE | SWP_NOMOVE );
	break;

      case WM_SIZE:
	GetClientRect(w->hwndParent, &ci->rectMaximize);
	if( !ci->hwndActiveChild )
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
    HWND	childHwnd;

    if (hwndMDIClient)
    {
	switch (message)
	{
	  case WM_COMMAND:
	    childHwnd = MDI_GetChildByID( WIN_FindWndPtr(hwndMDIClient),
                                          wParam );
 	    if( childHwnd )
	        SendMessage(hwndMDIClient, WM_MDIACTIVATE, childHwnd , 0L);
	    break;

	  case WM_NCLBUTTONDOWN:
	    if (MDIHandleLButton(hwnd, hwndMDIClient, wParam, lParam))
		return 0;
	    break;
	    
	  case WM_NCACTIVATE:
	    SendMessage(hwndMDIClient, message, wParam, lParam);
	    return MDIPaintMaximized(hwnd, hwndMDIClient, 
				     message, wParam, lParam);

	  case WM_NCPAINT:
	    return MDIPaintMaximized(hwnd, hwndMDIClient, 
				     message, wParam, lParam);
	
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
        return 0;

      case WM_CLOSE:
	SendMessage(GetParent(hwnd),WM_MDIDESTROY,hwnd,0L);
	return 0;

      case WM_SIZE:
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
	switch (wParam)
	{
	  case SC_MAXIMIZE:
	    return SendMessage(GetParent(hwnd), WM_MDIMAXIMIZE, (WPARAM)hwnd, 0);

	  case SC_RESTORE:
	    return SendMessage(GetParent(hwnd), WM_MDIRESTORE, (WPARAM)hwnd, 0);
	}
	break;
	
      /* should also handle following messages */
      case WM_GETMINMAXINFO:
	   /* should return rect of MDI client 
	    * so that normal ShowWindow will be able to handle
	    * actions that are handled by MDIMaximize and MDIRestore */

      case WM_SETVISIBLE:
         if( !ci->sbStop )
          {
            PostMessage(GetParent(hwnd),WM_MDICALCCHILDSCROLL,0,0L);
            ci->sbRecalc |= (SB_BOTH+1);
          }
	break;
      case WM_SETFOCUS:
	if( IsChild( GetActiveWindow(), GetParent(hwnd)) )
	    SendMessage(clientWnd->hwndChild,WM_CHILDACTIVATE,0,0L);
        if( !ci->sbStop )
          {
            PostMessage(GetParent(hwnd),WM_MDICALCCHILDSCROLL,0,0L);
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
    return 0;
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
 short 	 newPos;
 short 	 curPos;
 short 	 length;
 short 	 minPos;
 short 	 maxPos;
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

