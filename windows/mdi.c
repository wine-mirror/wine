/* MDI.C
 *
 * Copyright 1994, Bob Amstadt
 *
 * This file contains routines to support MDI features.
 */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "windows.h"
#include "win.h"
#include "mdi.h"
#include "user.h"
#include "sysmetrics.h"

#define DEBUG_MDI /* */

extern WORD MENU_DrawMenuBar( HDC hDC, LPRECT lprect,
			      HWND hwnd, BOOL suppress_draw );  /* menu.c */

/**********************************************************************
 *					MDIRecreateMenuList
 */
void
MDIRecreateMenuList(MDICLIENTINFO *ci)
{
    MDICHILDINFO *chi;
    char buffer[128];
    int id, n, index;

#ifdef DEBUG_MDI
    fprintf(stderr, "MDIRecreateMenuList: hWindowMenu %04.4x\n", 
	    ci->hWindowMenu);
#endif
    
    id = ci->idFirstChild; 
    while (DeleteMenu(ci->hWindowMenu, id, MF_BYCOMMAND))
	id++;

#ifdef DEBUG_MDI
    fprintf(stderr, "MDIRecreateMenuList: id %04.4x, idFirstChild %04.4x\n", 
	    id, ci->idFirstChild);
#endif

    if (!ci->flagMenuAltered)
    {
	ci->flagMenuAltered = TRUE;
	AppendMenu(ci->hWindowMenu, MF_SEPARATOR, 0, NULL);
    }
    
    id = ci->idFirstChild;
    index = 1;
    for (chi = ci->infoActiveChildren; chi != NULL; chi = chi->next)
    {
	n = sprintf(buffer, "%d ", index++);
	GetWindowText(chi->hwnd, buffer + n, sizeof(buffer) - n - 1);

#ifdef DEBUG_MDI
	fprintf(stderr, "MDIRecreateMenuList: id %04.4x, '%s'\n", 
		id, buffer);
#endif

	AppendMenu(ci->hWindowMenu, MF_STRING, id++, buffer);
    }
}

/**********************************************************************
 *					MDICreateChild
 */
HWND 
MDICreateChild(WND *w, MDICLIENTINFO *ci, HWND parent, LPMDICREATESTRUCT cs)
{
    HWND hwnd;

    /*
     * Create child window
     */
    cs->style &= (WS_MINIMIZE | WS_MAXIMIZE | WS_HSCROLL | WS_VSCROLL);
    
    hwnd = CreateWindowEx(0, cs->szClass, cs->szTitle, 
			  WS_CHILD | WS_BORDER | WS_CAPTION | WS_CLIPSIBLINGS |
			  WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU |
			  WS_THICKFRAME | WS_VISIBLE | cs->style,
			  cs->x, cs->y, cs->cx, cs->cy, parent, (HMENU) 0,
			  w->hInstance, (LPSTR) cs);

    if (hwnd)
    {
	HANDLE h = USER_HEAP_ALLOC(GMEM_MOVEABLE, sizeof(MDICHILDINFO));
	MDICHILDINFO *child_info = USER_HEAP_ADDR(h);
	if (!h)
	{
	    DestroyWindow(hwnd);
	    return 0;
	}

	ci->nActiveChildren++;
	
	child_info->next = ci->infoActiveChildren;
	child_info->prev = NULL;
	child_info->hwnd = hwnd;

	if (ci->infoActiveChildren)
	    ci->infoActiveChildren->prev = child_info;

	ci->infoActiveChildren = child_info;

	SendMessage(parent, WM_CHILDACTIVATE, 0, 0);
    }
	
    return hwnd;
}

/**********************************************************************
 *					MDIDestroyChild
 */
HWND 
MDIDestroyChild(WND *w_parent, MDICLIENTINFO *ci, HWND parent, HWND child,
		 BOOL flagDestroy)
{
    MDICHILDINFO  *chi;
    
    chi = ci->infoActiveChildren;
    while (chi && chi->hwnd != child)
	chi = chi->next;

    if (chi)
    {
	if (chi->prev)
	    chi->prev->next = chi->next;
	if (chi->next)
	    chi->next->prev = chi->prev;
	if (ci->infoActiveChildren == chi)
	    ci->infoActiveChildren = chi->next;

	ci->nActiveChildren--;
	
	if (chi->hwnd == ci->hwndActiveChild)
	    SendMessage(parent, WM_CHILDACTIVATE, 0, 0);

	USER_HEAP_FREE((HANDLE) (LONG) chi);
	
	if (flagDestroy)
	    DestroyWindow(child);
    }
    
    return 0;
}

/**********************************************************************
 *					MDIBringChildToTop
 */
void MDIBringChildToTop(HWND parent, WORD id, WORD by_id, BOOL send_to_bottom)
{
    MDICHILDINFO  *chi;
    MDICLIENTINFO *ci;
    WND           *w;
    int            i;

    w  = WIN_FindWndPtr(parent);
    ci = (MDICLIENTINFO *) w->wExtra;
    
#ifdef DEBUG_MDI
    fprintf(stderr, "MDIBringToTop: id %04.4x, by_id %d\n", id, by_id);
#endif

    if (by_id)
	id -= ci->idFirstChild;
    if (!by_id || id < ci->nActiveChildren)
    {
	chi = ci->infoActiveChildren;

	if (by_id)
	{
	    for (i = 0; i < id; i++)
		chi = chi->next;
	}
	else
	{
	    while (chi && chi->hwnd != id)
		chi = chi->next;
	}

	if (!chi)
	    return;

#ifdef DEBUG_MDI
	fprintf(stderr, "MDIBringToTop: child %04.4x\n", chi->hwnd);
#endif
	if (chi != ci->infoActiveChildren)
	{
	    if (ci->flagChildMaximized)
	    {
		RECT rectOldRestore, rect;

		w = WIN_FindWndPtr(chi->hwnd);
		
		rectOldRestore = ci->rectRestore;
		GetWindowRect(chi->hwnd, &ci->rectRestore);

		rect.top    = (ci->rectMaximize.top -
			       (w->rectClient.top - w->rectWindow.top));
		rect.bottom = (ci->rectMaximize.bottom + 
			       (w->rectWindow.bottom - w->rectClient.bottom));
		rect.left   = (ci->rectMaximize.left - 
			       (w->rectClient.left - w->rectWindow.left));
		rect.right  = (ci->rectMaximize.right +
			       (w->rectWindow.right - w->rectClient.right));
		w->dwStyle |= WS_MAXIMIZE;
		SetWindowPos(chi->hwnd, HWND_TOP, rect.left, rect.top, 
			     rect.right - rect.left + 1, 
			     rect.bottom - rect.top + 1, 0);
		SendMessage(chi->hwnd, WM_SIZE, SIZE_MAXIMIZED,
			    MAKELONG(w->rectClient.right-w->rectClient.left,
				     w->rectClient.bottom-w->rectClient.top));

		w = WIN_FindWndPtr(ci->hwndActiveChild);
		w->dwStyle &= ~WS_MAXIMIZE;
		SetWindowPos(ci->hwndActiveChild, HWND_BOTTOM, 
			     rectOldRestore.left, rectOldRestore.top, 
			     rectOldRestore.right - rectOldRestore.left + 1, 
			     rectOldRestore.bottom - rectOldRestore.top + 1,
			     SWP_NOACTIVATE | 
			     (send_to_bottom ? 0 : SWP_NOZORDER));
	    }
	    else
	    {
		SetWindowPos(chi->hwnd, HWND_TOP, 0, 0, 0, 0, 
			     SWP_NOMOVE | SWP_NOSIZE );
		if (send_to_bottom)
		{
		    SetWindowPos(ci->hwndActiveChild, HWND_BOTTOM, 0, 0, 0, 0, 
				 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		}
	    }
		
	    if (chi->next)
		chi->next->prev    = chi->prev;

	    if (chi->prev)
		chi->prev->next    = chi->next;
	    
	    chi->prev              = NULL;
	    chi->next              = ci->infoActiveChildren;
	    chi->next->prev        = chi;
	    ci->infoActiveChildren = chi;

	    SendMessage(parent, WM_CHILDACTIVATE, 0, 0);
	}
	
#ifdef DEBUG_MDI
	fprintf(stderr, "MDIBringToTop: pos %04.4x, hwnd %04.4x\n", 
		id, chi->hwnd);
#endif
    }
}

/**********************************************************************
 *					MDIMaximizeChild
 */
LONG MDIMaximizeChild(HWND parent, HWND child, MDICLIENTINFO *ci)
{
    WND *w = WIN_FindWndPtr(child);
    RECT rect;
    
    MDIBringChildToTop(parent, child, FALSE, FALSE);
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
		 rect.right - rect.left + 1, rect.bottom - rect.top + 1,
		 SWP_NOACTIVATE | SWP_NOZORDER);
    
    ci->flagChildMaximized = TRUE;
    
    SendMessage(child, WM_SIZE, SIZE_MAXIMIZED,
		MAKELONG(w->rectClient.right-w->rectClient.left,
			 w->rectClient.bottom-w->rectClient.top));
    SendMessage(GetParent(parent), WM_NCPAINT, 1, 0);

    return 0;
}

/**********************************************************************
 *					MDIRestoreChild
 */
LONG MDIRestoreChild(HWND parent, MDICLIENTINFO *ci)
{
    HWND    child;
    WND    *w      = WIN_FindWndPtr(child);
    LPRECT  lprect = &ci->rectRestore;

    printf("restoring mdi child\n");

    child = ci->hwndActiveChild;
    
    w->dwStyle &= ~WS_MAXIMIZE;
    SetWindowPos(child, 0, lprect->left, lprect->top, 
		 lprect->right - lprect->left + 1, 
		 lprect->bottom - lprect->top + 1,
		 SWP_NOACTIVATE | SWP_NOZORDER);
    
    ci->flagChildMaximized = FALSE;

    ShowWindow(child, SW_RESTORE);		/* display the window */
    SendMessage(GetParent(parent), WM_NCPAINT, 1, 0);
    MDIBringChildToTop(parent, child, FALSE, FALSE);

    return 0;
}

/**********************************************************************
 *					MDIChildActivated
 */
LONG MDIChildActivated(WND *w, MDICLIENTINFO *ci, HWND parent)
{
    MDICHILDINFO *chi;
    HWND          deact_hwnd;
    HWND          act_hwnd;
    LONG          lParam;

#ifdef DEBUG_MDI
    fprintf(stderr, "MDIChildActivate: top %04.4x\n", w->hwndChild);
#endif

    chi = ci->infoActiveChildren;
    if (chi)
    {
	deact_hwnd = ci->hwndActiveChild;
	act_hwnd   = chi->hwnd;
	lParam     = ((LONG) deact_hwnd << 16) | act_hwnd;

#ifdef DEBUG_MDI
	fprintf(stderr, "MDIChildActivate: deact %04.4x, act %04.4x\n",
	       deact_hwnd, act_hwnd);
#endif

	ci->hwndActiveChild = act_hwnd;

	if (deact_hwnd != act_hwnd)
	{
	    MDIRecreateMenuList(ci);
	    SendMessage(deact_hwnd,  WM_NCACTIVATE, FALSE, 0);
	    SendMessage(deact_hwnd, WM_MDIACTIVATE, FALSE, lParam);
	}
	
	SendMessage(act_hwnd,  WM_NCACTIVATE, TRUE, 0);
	SendMessage(act_hwnd, WM_MDIACTIVATE, TRUE, lParam);
    }

    if (chi || ci->nActiveChildren == 0)
    {
	MDIRecreateMenuList(ci);
	SendMessage(GetParent(parent), WM_NCPAINT, 0, 0);
    }
    
    return 0;
}

/**********************************************************************
 *					MDICascade
 */
LONG MDICascade(HWND parent, MDICLIENTINFO *ci)
{
    MDICHILDINFO *chi;
    RECT          rect;
    int           spacing, xsize, ysize;
    int		  x, y;

    if (ci->flagChildMaximized)
	MDIRestoreChild(parent, ci);

    GetClientRect(parent, &rect);
    spacing = GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFRAME);
    ysize   = rect.bottom - 8 * spacing;
    xsize   = rect.right  - 8 * spacing;
    
#ifdef DEBUG_MDI
    fprintf(stderr, 
	    "MDICascade: Client wnd at (%d,%d) - (%d,%d), spacing %d\n", 
	    rect.left, rect.top, rect.right, rect.bottom, spacing);
    fprintf(stderr, "MDICascade: searching for last child\n");
#endif
    for (chi = ci->infoActiveChildren; chi->next != NULL; chi = chi->next)
	;
    
#ifdef DEBUG_MDI
    fprintf(stderr, "MDICascade: last child is %04.4x\n", chi->hwnd);
#endif
    x = 0;
    y = 0;
    for ( ; chi != NULL; chi = chi->prev)
    {
#ifdef DEBUG_MDI
	fprintf(stderr, "MDICascade: move %04.4x to (%d,%d) size [%d,%d]\n", 
		chi->hwnd, x, y, xsize, ysize);
#endif
	SetWindowPos(chi->hwnd, 0, x, y, xsize, ysize, 
		     SWP_DRAWFRAME | SWP_NOACTIVATE | SWP_NOZORDER);

	x += spacing;
	y += spacing;
    }

    return 0;
}

/**********************************************************************
 *					MDITile
 */
LONG MDITile(HWND parent, MDICLIENTINFO *ci)
{
    MDICHILDINFO *chi;
    RECT          rect;
    int           xsize, ysize;
    int		  x, y;
    int		  rows, columns;
    int           r, c;
    int           i;

    if (ci->flagChildMaximized)
	MDIRestoreChild(parent, ci);

    GetClientRect(parent, &rect);
    rows    = (int) sqrt((double) ci->nActiveChildren);
    columns = ci->nActiveChildren / rows;
    ysize   = rect.bottom / rows;
    xsize   = rect.right  / columns;
    
    chi     = ci->infoActiveChildren;
    x       = 0;
    i       = 0;
    for (c = 1; c <= columns; c++)
    {
	if (c == columns)
	{
	    rows  = ci->nActiveChildren - i;
	    ysize = rect.bottom / rows;
	}

	y = 0;
	for (r = 1; r <= rows; r++, i++, chi = chi->next)
	{
	    SetWindowPos(chi->hwnd, 0, x, y, xsize, ysize, 
			 SWP_DRAWFRAME | SWP_NOACTIVATE | SWP_NOZORDER);

	    y += ysize;
	}

	x += xsize;
    }
    

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
    LONG           rv;
    HDC            hdc, hdcMem;
    RECT           rect;
    WND           *wndPtr = WIN_FindWndPtr(hwndFrame);

    w  = WIN_FindWndPtr(hwndClient);
    ci = (MDICLIENTINFO *) w->wExtra;

#ifdef DEBUG_MDI
	fprintf(stderr, 
		"MDIPaintMaximized: frame %04x,  client %04x"
		",  max flag %d,  menu %04x\n", 
		hwndFrame, hwndClient, 
		ci->flagChildMaximized, wndPtr ? wndPtr->wIDmenu : 0);
#endif

    if (ci->flagChildMaximized && wndPtr && wndPtr->wIDmenu != 0)
    {
	rv = NC_DoNCPaint( hwndFrame, (HRGN) 1, wParam, TRUE);
    
	hdc = GetDCEx(hwndFrame, 0, DCX_CACHE | DCX_WINDOW);
	if (!hdc)
	    return rv;

	hdcMem = CreateCompatibleDC(hdc);

	if (hbitmapClose == 0)
	{
	    hbitmapClose     = LoadBitmap(0, MAKEINTRESOURCE(OBM_OLD_CLOSE));
	    hbitmapMaximized = LoadBitmap(0, MAKEINTRESOURCE(OBM_RESTORE));
	}

#ifdef DEBUG_MDI
	fprintf(stderr, 
		"MDIPaintMaximized: hdcMem %04x, close bitmap %04x, "
		"maximized bitmap %04x\n",
		hdcMem, hbitmapClose, hbitmapMaximized);
#endif

	NC_GetInsideRect(hwndFrame, &rect);
	rect.top   += ((wndPtr->dwStyle & WS_CAPTION) ? 
		       SYSMETRICS_CYSIZE + 1 :  0);
	SelectObject(hdcMem, hbitmapClose);
	BitBlt(hdc, rect.left, rect.top + 1, 
	       SYSMETRICS_CXSIZE, SYSMETRICS_CYSIZE,
	       hdcMem, 1, 1, SRCCOPY);
	
	NC_GetInsideRect(hwndFrame, &rect);
	rect.top   += ((wndPtr->dwStyle & WS_CAPTION) ? 
		       SYSMETRICS_CYSIZE + 1 :  0);
	rect.left   = rect.right - SYSMETRICS_CXSIZE;
	SelectObject(hdcMem, hbitmapMaximized);
	BitBlt(hdc, rect.left, rect.top + 1, 
	       SYSMETRICS_CXSIZE, SYSMETRICS_CYSIZE,
	       hdcMem, 1, 1, SRCCOPY);
	
	NC_GetInsideRect(hwndFrame, &rect);
	rect.top   += ((wndPtr->dwStyle & WS_CAPTION) ? 
		       SYSMETRICS_CYSIZE + 1 :  0);
	rect.left  += SYSMETRICS_CXSIZE;
	rect.right -= SYSMETRICS_CXSIZE;
	rect.bottom = rect.top + SYSMETRICS_CYMENU;

	MENU_DrawMenuBar(hdc, &rect, hwndFrame, FALSE);
	
	DeleteDC(hdcMem);
	ReleaseDC(hwndFrame, hdc);
    }
    else
	DefWindowProc(hwndFrame, message, wParam, lParam);

    return rv;
}

/**********************************************************************
 *					MDIClientWndProc
 *
 * This function is the handler for all MDI requests.
 */
LONG 
MDIClientWndProc(HWND hwnd, WORD message, WORD wParam, LONG lParam)
{
    LPCREATESTRUCT       cs;
    LPCLIENTCREATESTRUCT ccs;
    MDICLIENTINFO       *ci;
    WND                 *w;

    w  = WIN_FindWndPtr(hwnd);
    ci = (MDICLIENTINFO *) w->wExtra;
    
    switch (message)
    {
      case WM_CHILDACTIVATE:
	return MDIChildActivated(w, ci, hwnd);

      case WM_CREATE:
	cs                      = (LPCREATESTRUCT) lParam;
	ccs                     = (LPCLIENTCREATESTRUCT) cs->lpCreateParams;
	ci->hWindowMenu         = ccs->hWindowMenu;
	ci->idFirstChild        = ccs->idFirstChild;
	ci->infoActiveChildren  = NULL;
	ci->flagMenuAltered     = FALSE;
	ci->flagChildMaximized  = FALSE;
	w->dwStyle             |= WS_CLIPCHILDREN;

	GetClientRect(w->hwndParent, &ci->rectMaximize);
	MoveWindow(hwnd, 0, 0, 
		   ci->rectMaximize.right, ci->rectMaximize.bottom, 1);

	return 0;

      case WM_MDIACTIVATE:
	MDIBringChildToTop(hwnd, wParam, FALSE, FALSE);
	return 0;

      case WM_MDICASCADE:
	return MDICascade(hwnd, ci);

      case WM_MDICREATE:
	return MDICreateChild(w, ci, hwnd, (LPMDICREATESTRUCT) lParam);

      case WM_MDIDESTROY:
	return MDIDestroyChild(w, ci, hwnd, wParam, TRUE);

      case WM_MDIGETACTIVE:
	return ((LONG) ci->hwndActiveChild | 
		((LONG) ci->flagChildMaximized << 16));

      case WM_MDIICONARRANGE:
	/* return MDIIconArrange(...) */
	break;
	
      case WM_MDIMAXIMIZE:
	return MDIMaximizeChild(hwnd, wParam, ci);

      case WM_MDINEXT:
	MDIBringChildToTop(hwnd, wParam, FALSE, TRUE);
	break;
	
      case WM_MDIRESTORE:
	return MDIRestoreChild(hwnd, ci);

      case WM_MDISETMENU:
	/* return MDISetMenu(...) */
	break;
	
      case WM_MDITILE:
	return MDITile(hwnd, ci);
	
      case WM_NCACTIVATE:
	SendMessage(ci->hwndActiveChild, message, wParam, lParam);
	break;
	
      case WM_PARENTNOTIFY:
	if (wParam == WM_DESTROY)
	    return MDIDestroyChild(w, ci, hwnd, LOWORD(lParam), FALSE);
	else if (wParam == WM_LBUTTONDOWN)
	    MDIBringChildToTop(hwnd, ci->hwndHitTest, FALSE, FALSE);
	break;

      case WM_SIZE:
	GetClientRect(w->hwndParent, &ci->rectMaximize);
	break;

    }
    
    return DefWindowProc(hwnd, message, wParam, lParam);
}

/**********************************************************************
 *					DefFrameProc (USER.445)
 *
 */
LONG 
DefFrameProc(HWND hwnd, HWND hwndMDIClient, WORD message, 
	     WORD wParam, LONG lParam)
{
    if (hwndMDIClient)
    {
	switch (message)
	{
	  case WM_COMMAND:
	    MDIBringChildToTop(hwndMDIClient, wParam, TRUE, FALSE);
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
	    SendMessage(hwndMDIClient, WM_SETFOCUS, wParam, lParam);
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
LONG 
DefMDIChildProc(HWND hwnd, WORD message, WORD wParam, LONG lParam)
{
    MDICLIENTINFO       *ci;
    WND                 *w;

    w  = WIN_FindWndPtr(GetParent(hwnd));
    ci = (MDICLIENTINFO *) w->wExtra;
    
    switch (message)
    {
      case WM_NCHITTEST:
	ci->hwndHitTest = hwnd;
	break;
	
      case WM_NCPAINT:
	return NC_DoNCPaint(hwnd, (HRGN)1, 
			    hwnd == ci->hwndActiveChild);

      case WM_SYSCOMMAND:
	switch (wParam)
	{
	  case SC_MAXIMIZE:
	    return SendMessage(GetParent(hwnd), WM_MDIMAXIMIZE, hwnd, 0);

	  case SC_RESTORE:
	    return SendMessage(GetParent(hwnd), WM_MDIRESTORE, hwnd, 0);
	}
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
