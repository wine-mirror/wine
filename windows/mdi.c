/* MDI.C
 *
 * Copyright 1994, Bob Amstadt
 *
 * This file contains routines to support MDI features.
 */
#include <stdlib.h>
#include <stdio.h>
#include "windows.h"
#include "win.h"
#include "mdi.h"
#include "user.h"

/* #define DEBUG_MDI /* */

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
 *					MDICreateClient
 */
HWND 
MDICreateClient(WND *w, MDICLIENTINFO *ci, HWND parent, LPMDICREATESTRUCT cs)
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
			  w->hInstance, cs->lParam);

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
 *					MDIDestroyClient
 */
HWND 
MDIDestroyClient(WND *w_parent, MDICLIENTINFO *ci, HWND parent, HWND child,
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

	USER_HEAP_FREE((HANDLE) chi);
	
	if (flagDestroy)
	    DestroyWindow(child);
    }
    
    return 0;
}

/**********************************************************************
 *					MDIBringChildToTop
 */
void MDIBringChildToTop(HWND parent, WORD id, WORD by_id)
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
	    SetWindowPos(chi->hwnd, HWND_TOP, 0, 0, 0, 0, 
			 SWP_NOMOVE | SWP_NOSIZE );

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
	DrawMenuBar(GetParent(parent));
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

    GetClientRect(parent, &rect);
    spacing = GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFRAME);
    ysize   = abs(rect.bottom - rect.top) - 8 * spacing;
    xsize   = abs(rect.right  - rect.left) - 8 * spacing;
    
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
    RECT                 rect;

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

	GetClientRect(w->hwndParent, &rect);
	MoveWindow(hwnd, 0, 0, rect.right, rect.bottom, 1);

	return 0;

      case WM_MDIACTIVATE:
	MDIBringChildToTop(hwnd, wParam, FALSE);
	return 0;

      case WM_MDICASCADE:
	return MDICascade(hwnd, ci);

      case WM_MDICREATE:
	return MDICreateClient(w, ci, hwnd, (LPMDICREATESTRUCT) lParam);

      case WM_MDIDESTROY:
	return MDIDestroyClient(w, ci, hwnd, wParam, TRUE);

      case WM_MDIGETACTIVE:
	return ((LONG) ci->hwndActiveChild | 
		((LONG) ci->flagChildMaximized << 16));

      case WM_MDIICONARRANGE:
	/* return MDIIconArrange(...) */
	break;
	
      case WM_MDIMAXIMIZE:
	ci->flagChildMaximized = TRUE;
	MDIBringChildToTop(hwnd, wParam, FALSE);
	return 0;

      case WM_NCACTIVATE:
	SendMessage(ci->hwndActiveChild, message, wParam, lParam);
	break;
	
      case WM_PARENTNOTIFY:
	if (wParam == WM_DESTROY)
	    return MDIDestroyClient(w, ci, hwnd, LOWORD(lParam), FALSE);
	else if (wParam == WM_LBUTTONDOWN)
	    MDIBringChildToTop(hwnd, ci->hwndHitTest, FALSE);
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
    switch (message)
    {
      case WM_COMMAND:
	MDIBringChildToTop(hwndMDIClient, wParam, TRUE);
	break;

      case WM_NCACTIVATE:
	SendMessage(hwndMDIClient, message, wParam, lParam);
	break;
	
      case WM_SETFOCUS:
	SendMessage(hwndMDIClient, WM_SETFOCUS, wParam, lParam);
	break;

      case WM_SIZE:
	MoveWindow(hwndMDIClient, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
	break;

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
