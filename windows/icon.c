
/*
 * Iconification functions
 *
 * Copyright 1994 John Richardson
 */

static char Copyright[] = "Copyright  John Richardson, 1994";

#include "win.h"
#include "class.h"
#include "message.h"
#include "sysmetrics.h"
#include "user.h"
#include "scroll.h"
#include "menu.h"
#include "icon.h"

RECT myrect;


ICON_Iconify(HWND hwnd)
{
	WND *wndPtr = WIN_FindWndPtr( hwnd );
	GC testgc;
	WND *parwPtr;
	
#ifdef DEBUG_ICON
#endif
	printf("ICON_Iconify %d\n", hwnd);

	parwPtr = WIN_FindWndPtr(wndPtr->hwndParent);
	if (parwPtr == NULL) {
		printf("argh, the parent is NULL, what to do?\n");
		exit(1);
	}
	wndPtr->dwStyle |= WS_MINIMIZE; 
 	XUnmapWindow(display, wndPtr->window); 
	if (wndPtr->icon == NULL) {
		printf("argh, couldn't find icon\n");
		exit(1);
	}
	printf("parent edge values are %d, %d\n",  parwPtr->rectWindow.left,
				parwPtr->rectWindow.bottom);
	wndPtr->ptIconPos.x = parwPtr->rectWindow.left + 10;
	wndPtr->ptIconPos.y = parwPtr->rectWindow.bottom - 80;

#ifdef NOT
        wndPtr->rectWindow.right = 100;
        wndPtr->rectWindow.bottom = 32;
        wndPtr->rectNormal.right = 1000;
        wndPtr->rectNormal.bottom = 32;
        wndPtr->rectClient.top= wndPtr->ptIconPos.y;
	wndPtr->rectClient.left= wndPtr->ptIconPos.x;

        wndPtr->rectClient.right = 100;
	wndPtr->rectClient.bottom = 64;
#endif
	wndPtr->rectClientSave = wndPtr->rectNormal;
	myrect = wndPtr->rectClient;

	MoveWindow(hwnd, wndPtr->ptIconPos.x, wndPtr->ptIconPos.y,
			100, 64+20, FALSE);

	XMoveWindow(display, wndPtr->icon, 
		wndPtr->ptIconPos.x, wndPtr->ptIconPos.y);

	XMapWindow(display, wndPtr->icon);

	SendMessage(hwnd, WM_PAINTICON, 0, 0);
        printf("done with iconify\n");
}


BOOL ICON_isAtPoint(HWND hwnd, POINT pt)
{
       	WND *wndPtr = WIN_FindWndPtr( hwnd );
	int iconWidth, iconHeight;

/****************
	if (wndPtr->hwndParent != GetDesktopWindow()) { 
        	pt.x -= wndPtr->rectClient.left;
        	pt.y -= wndPtr->rectClient.top;
	} 
*****************/
 
	
	if (wndPtr->hIcon != (HICON)NULL) {
		ICONALLOC   *lpico;
      		lpico = (ICONALLOC *)GlobalLock(wndPtr->hIcon);
      		iconWidth = (int)lpico->descriptor.Width;
      		iconHeight = (int)lpico->descriptor.Height;
    	} else {
      		iconWidth = 64;
      		iconHeight = 64;
    	}	
#define DEBUG_ICON 1
#ifdef DEBUG_ICON
        printf("icon x,y is %d,%d\n",
		wndPtr->ptIconPos.x, wndPtr->ptIconPos.y);

        printf("icon end x,y is %d,%d\n",
		wndPtr->ptIconPos.x + 100, 
		wndPtr->ptIconPos.y + iconHeight + 20);

        printf("mouse pt x,y is %d,%d\n", pt.x, pt.y);

	printf("%d\n", IsIconic(hwnd));
	printf("%d\n", (pt.x >= wndPtr->ptIconPos.x));
	printf("%d\n", (pt.x < wndPtr->ptIconPos.x + 100));
	printf("%d\n", (pt.y >= wndPtr->ptIconPos.y));
	printf("%d\n", (pt.y < wndPtr->ptIconPos.y + iconHeight + 20));
	printf("%d\n", !(wndPtr->dwStyle & WS_DISABLED));
	printf("%d\n",  (wndPtr->dwStyle & WS_VISIBLE));
	printf("%d\n", !(wndPtr->dwExStyle & WS_EX_TRANSPARENT));
#endif
	
        if ( IsIconic(hwnd) &&
            (pt.x >= wndPtr->ptIconPos.x) &&
            (pt.x < wndPtr->ptIconPos.x + 100) &&
            (pt.y >= wndPtr->ptIconPos.y) &&
            (pt.y < wndPtr->ptIconPos.y + iconHeight + 20) &&
            !(wndPtr->dwStyle & WS_DISABLED) &&
            (wndPtr->dwStyle & WS_VISIBLE) &&
            !(wndPtr->dwExStyle & WS_EX_TRANSPARENT))
        {
		printf("got a winner!\n");
		return 1;
        } 
 
	return 0;
	
}

HWND ICON_findIconFromPoint(POINT pt)
{
    	HWND hwnd = GetTopWindow( GetDesktopWindow() );
    	WND *wndPtr;
	HWND hwndRet = 0;

    	while (hwnd) {

        	if ( !(wndPtr=WIN_FindWndPtr(hwnd)))  return 0;
      		if (ICON_isAtPoint(hwnd, pt))  {
			printf("returning\n");
			return hwndRet = hwnd;
		} else {
			printf("checking child\n");
      			hwnd = wndPtr->hwndChild;
		}
    	}
	return hwndRet;
}


ICON_Deiconify(HWND hwnd)
{
	WND *wndPtr = WIN_FindWndPtr( hwnd );

	printf("deiconifying\n");
	XUnmapWindow(display, wndPtr->icon); 
	wndPtr->dwStyle &= ~WS_MINIMIZE;
/*	wndPtr->rectNormal = myrect;
*/
	MoveWindow(hwnd, 
		wndPtr->rectClientSave.left, 
		wndPtr->rectClientSave.top,
		wndPtr->rectClientSave.right - wndPtr->rectClientSave.left, 
		wndPtr->rectClientSave.bottom - wndPtr->rectClientSave.top, 
		FALSE);

	XMapWindow(display, wndPtr->window);
}


