static char RCSId[] = "$Id$";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";

#define DEBUG_MENU

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/SmeLine.h>
#include <X11/Xaw/SimpleMenu.h>
#include "WinMenuButto.h"
#include "SmeMenuButto.h"
#include "windows.h"
#include "menu.h"
#include "heap.h"
#include "win.h"
#include "bitmaps/check_bitmap"
#include "bitmaps/nocheck_bitmap"

static LPMENUBAR firstMenu = NULL;
static MENUITEM *parentItem;
static MENUITEM *siblingItem;
static int       lastLevel;
static int       menuId = 0;
static Pixmap    checkBitmap = XtUnspecifiedPixmap;
static Pixmap    nocheckBitmap = XtUnspecifiedPixmap;

LPPOPUPMENU PopupMenuGetStorageHeader(HWND hwnd);
LPPOPUPMENU PopupMenuGetWindowAndStorage(HWND hwnd, WND **wndPtr);
void StdDrawPopupMenu(HWND hwnd);
LPMENUITEM PopupMenuFindItem(HWND hwnd, int x, int y, WORD *lpRet);
LPMENUITEM GetMenuItemPtr(LPPOPUPMENU menu, WORD nPos);

/***********************************************************************
 *           PopupMenuWndProc
 */
LONG PopupMenuWndProc( HWND hwnd, WORD message, WORD wParam, LONG lParam )
{    
    CREATESTRUCT *createStruct;
    WORD	wRet;
    short	x, y;
    WND  	*wndPtr;
    LPPOPUPMENU lppop;
    LPMENUITEM	lpitem, lpitem2;
    HMENU	hSubMenu;
    RECT	rect;
    HDC		hDC;
    switch(message) 
    {
    case WM_CREATE:
#ifdef DEBUG_MENU
        printf("PopupMenu WM_CREATE lParam=%08X !\n", lParam);
#endif
	createStruct = (CREATESTRUCT *)lParam;
     	lppop = (LPPOPUPMENU)createStruct->lpCreateParams;
     	if (lppop == 0) break;
	wndPtr = WIN_FindWndPtr(hwnd);
	*((LPPOPUPMENU *)&wndPtr->wExtra[1]) = lppop;
#ifdef DEBUG_MENU
        printf("PopupMenu WM_CREATE lppop=%08X !\n", lppop);
#endif
	return 0;
    case WM_DESTROY:
	lppop = PopupMenuGetWindowAndStorage(hwnd, &wndPtr);
#ifdef DEBUG_MENU
        printf("PopupMenu WM_DESTROY %lX !\n", lppop);
#endif
	return 0;
	
    case WM_LBUTTONDOWN:
	lppop = PopupMenuGetWindowAndStorage(hwnd, &wndPtr);
	SetCapture(hwnd);
	lpitem = PopupMenuFindItem(hwnd, LOWORD(lParam), HIWORD(lParam), &wRet);
	printf("PopupMenu WM_LBUTTONDOWN wRet=%d lpitem=%08X !\n", wRet, lpitem);
	if (lpitem != NULL) {
	    lppop->FocusedItem = wRet;
	    if (((lpitem->item_flags & MF_SEPARATOR) != MF_SEPARATOR) &&
		((lpitem->item_flags & MF_MENUBREAK) != MF_MENUBREAK)) {
	        hDC = GetDC(hwnd);
	        InvertRect(hDC, &lpitem->rect);
		ReleaseDC(hwnd, hDC);
		}
	    }
	break;
    case WM_LBUTTONUP:
	lppop = PopupMenuGetStorageHeader(hwnd);
	ReleaseCapture();
	lpitem = PopupMenuFindItem(hwnd, LOWORD(lParam), HIWORD(lParam), &wRet);
	printf("PopupMenu WM_LBUTTONUP wRet=%d lpitem=%08X !\n", wRet, lpitem);
	if (lpitem != NULL) {
	    if (lppop->FocusedItem != (WORD)-1) {
		lpitem2 = GetMenuItemPtr(lppop, lppop->FocusedItem);
		if (((lpitem2->item_flags & MF_SEPARATOR) != MF_SEPARATOR) &&
		    ((lpitem2->item_flags & MF_MENUBREAK) != MF_MENUBREAK)) {
	            hDC = GetDC(hwnd);
	            InvertRect(hDC, &lpitem2->rect);
		    ReleaseDC(hwnd, hDC);
		    }
		}
	    if ((lpitem->item_flags & MF_POPUP) == MF_POPUP) {
		hSubMenu = (HMENU)lpitem->item_id;
		printf("ShowSubmenu hSubMenu=%04X !\n", hSubMenu);
		GetClientRect(hwnd, &rect);
		x = rect.right;
		GetWindowRect(hwnd, &rect);
		x += rect.left;
		TrackPopupMenu(hSubMenu, TPM_LEFTBUTTON, 
			x, rect.top + HIWORD(lParam), 
			0, lppop->ownerWnd, (LPRECT)NULL);
		break;
		}
	    if (((lpitem->item_flags & MF_SEPARATOR) != MF_SEPARATOR) &&
		((lpitem->item_flags & MF_POPUP) != MF_POPUP)) {
		SendMessage(lppop->ownerWnd, WM_COMMAND, lpitem->item_id, 0L);
		printf("PopupMenu // SendMessage WM_COMMAND wParam=%d !\n", 
			lpitem->item_id);
	    	ShowWindow(lppop->hWnd, SW_HIDE);
	    	break;
		}
	    }
	break;
    case WM_MOUSEMOVE:
        if ((wParam & MK_LBUTTON) != 0) {
	    lppop = PopupMenuGetStorageHeader(hwnd);
	    lpitem = PopupMenuFindItem(hwnd, LOWORD(lParam), HIWORD(lParam), &wRet);
	    if ((lpitem != NULL) && (lppop->FocusedItem != wRet)) {
		lpitem2 = GetMenuItemPtr(lppop, lppop->FocusedItem);
		hDC = GetDC(hwnd);
		if (((lpitem2->item_flags & MF_POPUP) == MF_POPUP) ||
		    ((lpitem2->item_flags & MF_STRING) == MF_STRING)) {
		    InvertRect(hDC, &lpitem2->rect);
		    }
		lppop->FocusedItem = wRet;
		printf("PopupMenu WM_MOUSEMOVE wRet=%d lpitem=%08X !\n", wRet, lpitem);
		InvertRect(hDC, &lpitem->rect);
		ReleaseDC(hwnd, hDC);
		}
            }
	break;

    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_CHAR:
	lppop = PopupMenuGetWindowAndStorage(hwnd, &wndPtr);
	return(SendMessage(wndPtr->hwndParent, message, wParam, lParam));

    case WM_PAINT:
        printf("PopupMenu WM_PAINT !\n");
	StdDrawPopupMenu(hwnd);
	break;
    default:
	return DefWindowProc( hwnd, message, wParam, lParam );
    }
return(0);
}



LPPOPUPMENU PopupMenuGetWindowAndStorage(HWND hwnd, WND **wndPtr)
{
    WND  *Ptr;
    LPPOPUPMENU lppop;
    *(wndPtr) = Ptr = WIN_FindWndPtr(hwnd);
    if (Ptr == 0) {
    	printf("Bad Window handle on PopupMenu !\n");
    	return 0;
    	}
    lppop = *((LPPOPUPMENU *)&Ptr->wExtra[1]);
    return lppop;
}


LPPOPUPMENU PopupMenuGetStorageHeader(HWND hwnd)
{
    WND  *Ptr;
    LPPOPUPMENU lppop;
    Ptr = WIN_FindWndPtr(hwnd);
    if (Ptr == 0) {
    	printf("Bad Window handle on PopupMenu !\n");
    	return 0;
    	}
    lppop = *((LPPOPUPMENU *)&Ptr->wExtra[1]);
    return lppop;
}


void StdDrawPopupMenu(HWND hwnd)
{
    WND 	*wndPtr;
    LPPOPUPMENU lppop;
    LPMENUITEM 	lpitem;
    PAINTSTRUCT ps;
    HBRUSH 	hBrush;
    HPEN	hOldPen;
    HWND	hWndParent;
    HDC 	hDC, hMemDC;
    RECT 	rect, rect2, rect3;
    HBITMAP	hBitMap;
    BITMAP	bm;
    UINT  	i;
    hDC = BeginPaint( hwnd, &ps );
    if (!IsWindowVisible(hwnd)) {
	EndPaint( hwnd, &ps );
	return;
	}
    lppop = PopupMenuGetWindowAndStorage(hwnd, &wndPtr);
    if (lppop == NULL) goto EndOfPaint;
    hBrush = SendMessage(lppop->ownerWnd, WM_CTLCOLOR, (WORD)hDC,
		    MAKELONG(hwnd, CTLCOLOR_LISTBOX));
    if (hBrush == (HBRUSH)NULL)  hBrush = GetStockObject(WHITE_BRUSH);
    GetClientRect(hwnd, &rect);
    GetClientRect(hwnd, &rect2);
    FillRect(hDC, &rect, hBrush);
    if (lppop->nItems == 0) goto EndOfPaint;
    lpitem = lppop->firstItem->next;
    if (lpitem == NULL) goto EndOfPaint;
    for(i = 0; i < lppop->nItems; i++) {
	if ((lpitem->item_flags & MF_SEPARATOR) == MF_SEPARATOR) {
	    rect2.bottom = rect2.top + 3;
	    CopyRect(&lpitem->rect, &rect2);
	    hOldPen = SelectObject(hDC, GetStockObject(BLACK_PEN));
	    MoveTo(hDC, rect2.left, rect2.top + 1);
	    LineTo(hDC, rect2.right, rect2.top + 1);
	    SelectObject(hDC, hOldPen);
	    rect2.top += 3;
	    }
	if ((lpitem->item_flags & MF_BITMAP) == MF_BITMAP) {
	    hBitMap = (HBITMAP)LOWORD(lpitem->item_text);
	    rect2.bottom = rect2.top + bm.bmHeight;
	    CopyRect(&lpitem->rect, &rect2);
	    hMemDC = CreateCompatibleDC(hDC);
	    SelectObject(hMemDC, hBitMap);
	    GetObject(hBitMap, sizeof(BITMAP), (LPSTR)&bm);
	    BitBlt(hDC, rect2.left, rect2.top,
		bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
	    DeleteDC(hMemDC);
	    rect2.top += bm.bmHeight;
	    }
	if (((lpitem->item_flags & MF_BITMAP) != MF_BITMAP) &&
	    ((lpitem->item_flags & MF_SEPARATOR) != MF_SEPARATOR) &&
	    ((lpitem->item_flags & MF_MENUBREAK) != MF_MENUBREAK)) {
	    rect2.bottom = rect2.top + 15;
	    CopyRect(&lpitem->rect, &rect2);
	    TextOut(hDC, rect2.left + 5, rect2.top + 2, 
		(char *)lpitem->item_text, strlen((char *)lpitem->item_text));
	    rect2.top += 15;
	    }
	if ((lpitem->item_flags & MF_CHECKED) == MF_CHECKED) {
	    CopyRect(&rect3, &rect2);
	    rect3.left = rect3.right - rect3.bottom + rect3.top;
	    InvertRect(hDC, &rect3);
	    }
	if ((lpitem->item_flags & MF_POPUP) == MF_POPUP) {
	    CopyRect(&rect3, &rect2);
	    rect3.left = rect3.right - rect3.bottom + rect3.top;
	    FillRect(hDC, &rect3, GetStockObject(BLACK_BRUSH));
	    }
	if (lpitem->next == NULL) goto EndOfPaint;
	lpitem = (LPMENUITEM)lpitem->next;
    }
EndOfPaint:
    EndPaint( hwnd, &ps );
}



LPMENUITEM PopupMenuFindItem(HWND hwnd, int x, int y, WORD *lpRet)
{
    WND 	*wndPtr;
    LPPOPUPMENU lppop;
    LPMENUITEM 	lpitem;
    RECT 	rect, rect2;
    HBITMAP	hBitMap;
    BITMAP	bm;
    UINT  	i;
    lppop = PopupMenuGetWindowAndStorage(hwnd, &wndPtr);
    if (lppop == NULL) return NULL;
    GetClientRect(hwnd, &rect);
    if (lppop->nItems == 0) return NULL;
    lpitem = lppop->firstItem->next;
    if (lpitem == NULL) return NULL;
    for(i = 0; i < lppop->nItems; i++) {
	if (y < rect2.top) return NULL;
	if ((lpitem->item_flags & MF_SEPARATOR) == MF_SEPARATOR) {
	    rect2.bottom = rect2.top + 3;
	    rect2.top += 3;
	    }
	if ((lpitem->item_flags & MF_BITMAP) == MF_BITMAP) {
	    hBitMap = (HBITMAP)LOWORD(lpitem->item_text);
	    GetObject(hBitMap, sizeof(BITMAP), (LPSTR)&bm);
	    rect2.bottom = rect2.top + bm.bmHeight;
	    rect2.top += bm.bmHeight;
	    }
	if (((lpitem->item_flags & MF_BITMAP) != MF_BITMAP) &&
	    ((lpitem->item_flags & MF_SEPARATOR) != MF_SEPARATOR) &&
	    ((lpitem->item_flags & MF_MENUBREAK) != MF_MENUBREAK)) {
	    rect2.bottom = rect2.top + 15;
	    rect2.top += 15;
	    }
	if (y < rect2.bottom) {
	    if (lpRet != NULL) *lpRet = i;
	    return lpitem;
	    }
	if (lpitem->next == NULL) return NULL;
	lpitem = (LPMENUITEM)lpitem->next;
    }
    return NULL;
}



LPMENUITEM GetMenuItemPtr(LPPOPUPMENU menu, WORD nPos)
{
    LPMENUITEM 	lpitem;
    int		i;
    if (menu == NULL) return NULL;
    lpitem = menu->firstItem;
    for (i = 0; i < menu->nItems; i++) {
    	if (lpitem->next == NULL) return NULL;
    	lpitem = (LPMENUITEM)lpitem->next;
    	if (i == nPos) return(lpitem);
    	}
    return NULL;
}


/**********************************************************************
 *					MENU_CheckWidget
 */
void
MENU_CheckWidget(Widget w, Boolean check)
{
    if (checkBitmap == XtUnspecifiedPixmap)
    {
	Display *display = XtDisplayOfObject(w);
	    
	checkBitmap = XCreateBitmapFromData(display,
					    DefaultRootWindow(display),
					    check_bitmap_bits,
					    check_bitmap_width,
					    check_bitmap_height);
	nocheckBitmap = XCreateBitmapFromData(display,
					    DefaultRootWindow(display),
					    nocheck_bitmap_bits,
					    nocheck_bitmap_width,
					    nocheck_bitmap_height);
    }
	    
    if (check)
	XtVaSetValues(w, XtNleftBitmap, checkBitmap, NULL);
    else
	XtVaSetValues(w, XtNleftBitmap, nocheckBitmap, NULL);
}

/**********************************************************************
 *					MENU_ParseMenu
 */
WORD *
MENU_ParseMenu(WORD *first_item, 
	       int level,
	       int limit,
	       int (*action)(WORD *item, int level, void *app_data),
	       void *app_data)
{
    WORD *item;
    WORD *next_item;
    int   i;

    level++;
    next_item = first_item;
    i = 0;
    do
    {
	i++;
	item = next_item;
	(*action)(item, level, app_data);
	if (*item & MF_POPUP)
	{
	    MENU_POPUPITEM *popup_item = (MENU_POPUPITEM *) item;

	    next_item = (WORD *) (popup_item->item_text + 
				  strlen(popup_item->item_text) + 1);
	    next_item = MENU_ParseMenu(next_item, level, 0, action, app_data);
	}
	else
	{
	    MENU_NORMALITEM *normal_item = (MENU_NORMALITEM *) item;

	    next_item = (WORD *) (normal_item->item_text + 
				  strlen(normal_item->item_text) + 1);
	}
    }
    while (!(*item & MF_END) && i != limit);

    return next_item;
}

/**********************************************************************
 *					MENU_FindMenuBar
 */
LPMENUBAR
MENU_FindMenuBar(MENUITEM *this_item)
{
    MENUITEM *root;
    LPMENUBAR menu;

    /*
     * Find root item on menu bar.
     */
    for (root = this_item; root->parent != NULL; root = root->parent)
	;
    for ( ; root->prev != NULL; root = root->prev)
	;

    /*
     * Find menu bar for the root item.
     */
    for (menu = firstMenu; 
	 menu != NULL && menu->firstItem != root; 
	 menu = menu->next)
	;
    
    return menu;
}

/**********************************************************************
 *					MENU_SelectionCallback
 */
static void
MENU_SelectionCallback(Widget w, XtPointer client_data, XtPointer call_data)
{
    MENUITEM *this_item = (MENUITEM *) client_data;
    LPMENUBAR menu;
    WND	     *wndPtr;

    if (this_item->menu_w != NULL || (this_item->item_flags & MF_DISABLED))
	return;

    /*
     * Find menu bar for the root item.
     */
    menu = MENU_FindMenuBar(this_item);
    if (menu != NULL)
    {
	wndPtr = WIN_FindWndPtr(menu->ownerWnd);
	if (wndPtr == NULL)
	    return;

#ifdef DEBUG_MENU
	printf("Selected '%s' (%d).\n", 
	       this_item->item_text, this_item->item_id);
#endif
	
	CallWindowProc(wndPtr->lpfnWndProc, menu->ownerWnd, WM_COMMAND,
		       this_item->item_id, 0);
    }
}

/**********************************************************************
 *					MENU_CreateItems
 */
int
MENU_CreateItems(WORD *item, int level, void *app_data)
{
    MENU_POPUPITEM     *popup_item;
    MENU_NORMALITEM    *normal_item;
    MENUITEM	       *this_item;
    Arg			this_args[10];
    int			n_args = 0;
    LPMENUBAR           menu = (LPMENUBAR) app_data;

    if (menu->nItems == 0)
	this_item = menu->firstItem;
    else
	this_item = (MENUITEM *) GlobalQuickAlloc(sizeof(MENUITEM));

    if (this_item == NULL)
	return 0;

    if (level > lastLevel)
    {
	parentItem  = siblingItem;
	siblingItem = NULL;
    }

    while (level < lastLevel)
    {
	siblingItem = parentItem;
	if (siblingItem != NULL)
	    parentItem = siblingItem->parent;
	else
	    parentItem = NULL;

	lastLevel--;
    }
    lastLevel = level;
    
    this_item->next = NULL;
    this_item->prev = siblingItem;
    this_item->child = NULL;
    this_item->parent = parentItem;
    
    if (siblingItem !=  NULL)
	siblingItem->next = this_item;
    if (parentItem != NULL && parentItem->child == NULL)
	parentItem->child = this_item;
    
    siblingItem = this_item;
    
    if (*item & MF_POPUP)
    {
	popup_item = (MENU_POPUPITEM *) item;
	this_item->item_flags = popup_item->item_flags;
	this_item->item_id    = -1;
	this_item->item_text  = popup_item->item_text;

#ifdef DEBUG_MENU
	printf("%d: popup %s\n", level, this_item->item_text);
#endif
    }
    else
    {
	normal_item = (MENU_NORMALITEM *) item;
	this_item->item_flags = normal_item->item_flags;
	this_item->item_id    = normal_item->item_id;
	this_item->item_text  = normal_item->item_text;

#ifdef DEBUG_MENU
	printf("%d: normal %s (%04x)\n", level, this_item->item_text,
	       this_item->item_flags);
#endif
    }

    if (level == 1)
    {
	menu->nItems++;

	if (this_item->prev != NULL)
	{
	    XtSetArg(this_args[n_args], XtNhorizDistance, 10); 
	    n_args++;
	    XtSetArg(this_args[n_args], XtNfromHoriz, this_item->prev->w); 
	    n_args++;
	}

	if (this_item->item_flags & MF_POPUP)
	{
	    sprintf(this_item->menu_name, "Menu%d", menuId++);
	    XtSetArg(this_args[n_args], XtNmenuName, this_item->menu_name);
	    n_args++;
	    
	    this_item->w = XtCreateManagedWidget(this_item->item_text,
						 winMenuButtonWidgetClass, 
						 menu->parentWidget,
						 this_args, n_args);
	    this_item->menu_w = XtCreatePopupShell(this_item->menu_name,
						 simpleMenuWidgetClass, 
						 this_item->w,
						 NULL, 0);
	}
	else
	{
	    this_item->w = XtCreateManagedWidget(this_item->item_text,
						 winCommandWidgetClass, 
						 menu->parentWidget,
						 this_args, n_args);
	    this_item->menu_w = NULL;
	    XtAddCallback(this_item->w, XtNcallback, MENU_SelectionCallback,
			  (XtPointer) this_item);
	}

	if (menu->firstItem == NULL)
	    menu->firstItem = this_item;
    }
    else
    {
	if ((this_item->item_flags & MF_MENUBREAK) ||
	    (strlen(this_item->item_text) == 0))
	{
	    XtSetArg(this_args[n_args], XtNheight, 10);
	    n_args++;
	    this_item->w = XtCreateManagedWidget("separator",
						 smeLineObjectClass,
						 this_item->parent->menu_w,
						 this_args, n_args);
	}
	else
	{
	    XtSetArg(this_args[n_args], XtNmenuName, this_item->menu_name);
	    n_args++;
	    this_item->w = XtCreateManagedWidget(this_item->item_text,
						 smeMenuButtonObjectClass,
						 this_item->parent->menu_w,
						 this_args, n_args);

	    if (this_item->item_flags & MF_POPUP)
	    {
		sprintf(this_item->menu_name, "Menu%d", menuId++);
		this_item->menu_w = XtCreatePopupShell(this_item->menu_name,
						     simpleMenuWidgetClass, 
						     this_item->parent->menu_w,
						     NULL, 0);
	    }
	    else
	    {
		this_item->menu_w = NULL;
		XtAddCallback(this_item->w, XtNcallback, 
			      MENU_SelectionCallback, (XtPointer) this_item);
	    }
	}
    }

    if (this_item->w != NULL)
    {
	if (this_item->item_flags & MF_GRAYED)
	    XtSetSensitive(this_item->w, False);
	if (this_item->item_flags & MF_DISABLED)
	    XtVaSetValues(this_item->w, XtNinactive, True, NULL);
	if (this_item->item_flags & MF_CHECKED)
	    MENU_CheckWidget(this_item->w, True);
    }

    return 1;
}

/**********************************************************************
 *					MENU_UseMenu
 */
LPMENUBAR
MENU_UseMenu(Widget parent, HANDLE instance, HWND wnd, HMENU hmenu, int width)
{
    LPMENUBAR 		menubar;
    MENUITEM           *menu;
    MENU_HEADER        *menu_desc;

    menu  = (MENUITEM *) GlobalLock(hmenu);
    if (hmenu == 0 || menu == NULL)
    {
	return NULL;
    }

    menubar = MENU_FindMenuBar(menu);
    if (menubar == NULL)
    {
	GlobalUnlock(hmenu);
	return NULL;
    }

    menubar->nItems 	     = 0;
    menubar->parentWidget    = parent;
    menubar->ownerWnd	     = wnd;

    menu_desc = (MENU_HEADER *) GlobalLock(menubar->menuDescription);
    
    parentItem  = NULL;
    siblingItem = NULL;
    lastLevel   = 0;
    MENU_ParseMenu((WORD *) (menu_desc + 1), 0, 0, MENU_CreateItems, menubar);

    menubar->menuBarWidget = menubar->firstItem->w;

    menubar->next = firstMenu;
    firstMenu = menubar;
    
    return menubar;
}

/**********************************************************************
 *					MENU_CreateMenuBar
 */
LPMENUBAR
MENU_CreateMenuBar(Widget parent, HANDLE instance, HWND wnd,
		   char *menu_name, int width)
{
    LPMENUBAR 		menubar;
    HMENU     		hmenu;
    MENUITEM           *menu;
    MENU_HEADER        *menu_desc;

#ifdef DEBUG_MENU
    printf("CreateMenuBar: instance %02x, menu '%s', width %d\n",
	   instance, menu_name, width);
#endif

    hmenu = LoadMenu(instance, menu_name);
    return MENU_UseMenu(parent, instance, wnd, hmenu, width);
}

/**********************************************************************
 *					MENU_FindItem
 */
MENUITEM *
MENU_FindItem(MENUITEM *menu, WORD item_id, WORD flags)
{
    MENUITEM *item;
    WORD position;
    
    if (flags & MF_BYPOSITION)
    {
	item = menu;
	for (position = 0; item != NULL && position != item_id; position++)
	    item = item->next;
	
	if (position == item_id)
	    return item;
    }
    else
    {
	for ( ; menu != NULL; menu = menu->next)
	{
	    if (menu->item_id == item_id && menu->child == NULL)
		return menu;
	    if (menu->child != NULL)
	    {
		item = MENU_FindItem(menu->child, item_id, flags);
		if (item != NULL)
		    return item;
	    }
	}
    }

    return NULL;
}

/**********************************************************************
 *					MENU_CollapseMenu
 */
static void
MENU_CollapseBranch(MENUITEM *item, Boolean first_flag)
{
    MENUITEM *next_item;
    
    for ( ; item != NULL; item = next_item)
    {
	next_item = item->next;
	
	if (item->child != NULL)
	    MENU_CollapseBranch(item->child, False);
	    
	if (item->w != NULL)
	    XtDestroyWidget(item->w);
	if (item->menu_w != NULL)
	    XtDestroyWidget(item->menu_w);
	
	if (first_flag)
	{
	    item->prev 	     = NULL;
	    item->child      = NULL;
	    item->next 	     = NULL;
	    item->parent     = NULL;
	    item->item_flags = 0;
	    item->item_id    = 0;
	    item->item_text  = NULL;
	    item->w 	     = NULL;
	    item->menu_w     = NULL;

	    first_flag = False;
	}
	else
	{
	    GlobalFree((unsigned int) item);
	}
    }
}

void
MENU_CollapseMenu(LPMENUBAR menubar)
{
    MENU_CollapseBranch(menubar->firstItem, True);
    
    menubar->nItems 	     = 0;
    menubar->parentWidget    = NULL;
    menubar->ownerWnd	     = 0;
    menubar->menuBarWidget   = NULL;
}


/**********************************************************************
 *					CheckMenu
 */
BOOL
CheckMenu(HMENU hmenu, WORD item_id, WORD check_flags)
{
    MENUITEM *item;
    Pixmap left_bitmap;

    if ((item = (MENUITEM *) GlobalLock(hmenu)) == NULL)
	return -1;
    
    item = MENU_FindItem(item, item_id, check_flags);
    if (item == NULL)
    {
	GlobalUnlock(hmenu);
	return -1;
    }

    XtVaGetValues(item->w, XtNleftBitmap, &left_bitmap, NULL);
    MENU_CheckWidget(item->w, (check_flags & MF_CHECKED));

    if (left_bitmap == XtUnspecifiedPixmap)
	return MF_UNCHECKED;
    else
	return MF_CHECKED;
}

/**********************************************************************
 *					LoadMenu
 */
HMENU
LoadMenu(HINSTANCE instance, char *menu_name)
{
    HANDLE		hmenubar;
    LPMENUBAR 		menu;
    HANDLE		hmenu_desc;
    HMENU     		hmenu;
    MENU_HEADER        *menu_desc;

#ifdef DEBUG_MENU
    printf("LoadMenu: instance %02x, menu '%s'\n",
	   instance, menu_name);
#endif

    if (menu_name == NULL || 
	(hmenu_desc = RSC_LoadMenu(instance, menu_name)) == 0 ||
	(menu_desc = (MENU_HEADER *) GlobalLock(hmenu_desc)) == NULL)
    {
	return 0;
    }

    hmenubar = GlobalAlloc(GMEM_MOVEABLE, sizeof(MENUBAR));
    menu = (LPMENUBAR) GlobalLock(hmenubar);
    if (menu == NULL)
    {
	GlobalFree(hmenu_desc);
	GlobalFree(hmenubar);
	return 0;
    }

    hmenu = GlobalAlloc(GMEM_MOVEABLE, sizeof(MENUITEM));
    if (hmenu == 0)
    {
	GlobalFree(hmenu_desc);
	GlobalFree(hmenubar);
	return 0;
    }

    menu->menuDescription = hmenu_desc;
    menu->nItems 	  = 0;
    menu->parentWidget    = NULL;
    menu->firstItem       = (MENUITEM *) GlobalLock(hmenu);
    menu->ownerWnd	  = 0;
    menu->menuBarWidget   = NULL;

    menu->firstItem->next 	= NULL;
    menu->firstItem->prev 	= NULL;
    menu->firstItem->child 	= NULL;
    menu->firstItem->parent 	= NULL;
    menu->firstItem->item_flags	= 0;
    menu->firstItem->item_id 	= 0;
    menu->firstItem->item_text 	= NULL;
    menu->firstItem->w 		= NULL;
    menu->firstItem->menu_w 	= NULL;

    menu->next = firstMenu;
    firstMenu  = menu;
    
    return GlobalHandleFromPointer(menu->firstItem);
}


/**********************************************************************
 *			CheckMenuItem		[USER.154]
 */
BOOL CheckMenuItem(HMENU hMenu, WORD wItemID, WORD wFlags)
{
}


/**********************************************************************
 *			EnableMenuItem		[USER.155]
 */
BOOL EnableMenuItem(HMENU hMenu, WORD wItemID, WORD wFlags)
{
}


/**********************************************************************
 *			InsertMenu		[USER.410]
 */
BOOL InsertMenu(HMENU hMenu, WORD nPos, WORD wFlags, WORD wItemID, LPSTR lpNewItem)
{
    WND		*wndPtr;
    LPPOPUPMENU	menu;
    HANDLE	hNewItem;
    LPMENUITEM 	lpitem, lpitem2;
    int		i;
#ifdef DEBUG_MENU
    printf("InsertMenu (%04X, %04X, %04X, %04X, %08X) !\n",
	hMenu, nPos, wFlags, wItemID, lpNewItem);
#endif
    menu = (LPPOPUPMENU) GlobalLock(hMenu);
    if (menu == NULL) return FALSE;
    lpitem = menu->firstItem;
    for (i = 0; i < menu->nItems; i++) {
    	if (lpitem->next == NULL) break;
    	lpitem = (LPMENUITEM)lpitem->next;
	printf("InsertMenu // during loop items !\n");
    	}
    printf("InsertMenu // after loop items !\n");
    hNewItem = GlobalAlloc(GMEM_MOVEABLE, sizeof(MENUITEM));
    if (hNewItem == 0) return FALSE;
    lpitem2 = (LPMENUITEM)GlobalLock(hNewItem);
    if (lpitem2 == 0) {
	GlobalFree(hNewItem);
	return FALSE;
	}
    lpitem2->item_flags = wFlags;
    lpitem2->item_id = wItemID;
    if ((wFlags & MF_STRING) == MF_STRING) {
	lpitem2->item_text = lpNewItem;
	}
    else
	lpitem2->item_text = lpNewItem;
    lpitem2->prev = lpitem;
    if (lpitem->next != NULL)
	lpitem2->next = lpitem->next;
    else
	lpitem2->next = NULL;
    lpitem->next = lpitem2;
    lpitem2->child = NULL;
    lpitem2->parent = NULL;
    lpitem2->w = NULL;
    lpitem2->menu_w = NULL;
    menu->nItems++;
    return TRUE;
}


/**********************************************************************
 *			AppendMenu		[USER.411]
 */
BOOL AppendMenu(HMENU hMenu, WORD wFlags, WORD wItemID, LPSTR lpNewItem)
{
    WND		*wndPtr;
    LPPOPUPMENU	menu;
    HANDLE	hNewItem;
    LPMENUITEM 	lpitem, lpitem2;
#ifdef DEBUG_MENU
    printf("AppendMenu (%04X, %04X, %04X, %08X) !\n",
	hMenu, wFlags, wItemID, lpNewItem);
#endif
    menu = (LPPOPUPMENU) GlobalLock(hMenu);
    if (menu == NULL) return FALSE;
    lpitem = menu->firstItem;
    while (lpitem->next != NULL) {
    	lpitem = (LPMENUITEM)lpitem->next;
	printf("AppendMenu // during loop items !\n");
    	}
    printf("AppendMenu // after loop items !\n");
    hNewItem = GlobalAlloc(GMEM_MOVEABLE, sizeof(MENUITEM));
    if (hNewItem == 0) return FALSE;
    lpitem2 = (LPMENUITEM)GlobalLock(hNewItem);
    if (lpitem2 == 0) {
	GlobalFree(hNewItem);
	return FALSE;
	}
    lpitem2->item_flags = wFlags;
    lpitem2->item_id = wItemID;
    if ((wFlags & MF_STRING) == MF_STRING) {
	lpitem2->item_text = lpNewItem;
	}
    else
	lpitem2->item_text = lpNewItem;
    lpitem->next = lpitem2;
    lpitem2->prev = lpitem;
    lpitem2->next = NULL;
    lpitem2->child = NULL;
    lpitem2->parent = NULL;
    lpitem2->w = NULL;
    lpitem2->menu_w = NULL;
    menu->nItems++;
    return TRUE;
}


/**********************************************************************
 *			RemoveMenu		[USER.412]
 */
BOOL RemoveMenu(HMENU hMenu, WORD nPos, WORD wFlags)
{
    WND		*wndPtr;
    LPPOPUPMENU	menu;
    LPMENUITEM 	lpitem;
    int		i;
#ifdef DEBUG_MENU
    printf("RemoveMenu (%04X, %04X, %04X) !\n", hMenu, nPos, wFlags);
#endif
    menu = (LPPOPUPMENU) GlobalLock(hMenu);
    if (menu == NULL) return FALSE;
    lpitem = menu->firstItem;
    for (i = 0; i < menu->nItems; i++) {
    	if (lpitem->next == NULL) break;
    	lpitem = (LPMENUITEM)lpitem->next;
    	if (i == nPos) {
    	    lpitem->prev->next = lpitem->next;
    	    lpitem->next->prev = lpitem->prev;
    	    GlobalFree(HIWORD(lpitem));
    	    return(TRUE);
	    }
	printf("RemoveMenu // during loop items !\n");
    	}
    printf("RemoveMenu // after loop items !\n");
    return FALSE;
}


/**********************************************************************
 *			DeleteMenu		[USER.413]
 */
BOOL DeleteMenu(HMENU hMenu, WORD nPos, WORD wFlags)
{
#ifdef DEBUG_MENU
    printf("DeleteMenu (%04X, %04X, %04X) !\n", hMenu, nPos, wFlags);
#endif
    return TRUE;
}


/**********************************************************************
 *			ModifyMenu		[USER.414]
 */
BOOL ModifyMenu(HMENU hMenu, WORD nPos, WORD wFlags, WORD wItemID, LPSTR lpNewItem)
{
#ifdef DEBUG_MENU
    printf("ModifyMenu (%04X, %04X, %04X, %04X, %08X) !\n",
	hMenu, nPos, wFlags, wItemID, lpNewItem);
#endif
    return TRUE;
}


/**********************************************************************
 *			CreatePopupMenu		[USER.415]
 */
HMENU CreatePopupMenu()
{
    HANDLE	hItem;
    HMENU	hMenu;
    LPPOPUPMENU menu;
#ifdef DEBUG_MENU
    printf("CreatePopupMenu !\n");
#endif
    hMenu = GlobalAlloc(GMEM_MOVEABLE, sizeof(POPUPMENU));
    menu = (LPPOPUPMENU) GlobalLock(hMenu);
    if (menu == NULL) {
	GlobalFree(hMenu);
	return 0;
 	}
    hItem = GlobalAlloc(GMEM_MOVEABLE, sizeof(MENUITEM));
    if (hItem == 0) {
	GlobalFree(hMenu);
	return 0;
	}
    menu->nItems 	  = 0;
    menu->firstItem       = (LPMENUITEM)GlobalLock(hItem);
    menu->ownerWnd	  = 0;
    menu->hWnd		  = 0;

    menu->firstItem->next 	= NULL;
    menu->firstItem->prev 	= NULL;
    menu->firstItem->child 	= NULL;
    menu->firstItem->parent 	= NULL;
    menu->firstItem->item_flags	= 0;
    menu->firstItem->item_id 	= 0;
    menu->firstItem->item_text 	= NULL;
    menu->firstItem->w 		= NULL;
    menu->firstItem->menu_w 	= NULL;
    return hMenu;
}


/**********************************************************************
 *			TrackPopupMenu		[USER.414]
 */
BOOL TrackPopupMenu(HMENU hMenu, WORD wFlags, short x, short y,
	short nReserved, HWND hWnd, LPRECT lpRect)
{
    WND		*wndPtr;
    LPPOPUPMENU	menu;
#ifdef DEBUG_MENU
    printf("TrackPopupMenu (%04X, %04X, %d, %d, %04X, %04X, %08X) !\n",
	hMenu, wFlags, x, y, nReserved, hWnd, lpRect);
#endif
    menu = (LPPOPUPMENU) GlobalLock(hMenu);
    if (menu == NULL) return FALSE;
    wndPtr = WIN_FindWndPtr(hWnd);
    menu->ownerWnd = hWnd;
    if (menu->hWnd == NULL) {
        menu->hWnd = CreateWindow("POPUPMENU", "", WS_CHILD | WS_VISIBLE,
        	x, y, 100, 150, hWnd, 0, wndPtr->hInstance, (LPSTR)menu);
        }
    else 
    	ShowWindow(menu->hWnd, SW_SHOW);
    return TRUE;
}


/**********************************************************************
 *			CreateMenu		[USER.151]
 */
HMENU CreateMenu()
{
    HANDLE	hItem;
    HMENU	hMenu;
    LPMENUBAR	menu;
#ifdef DEBUG_MENU
    printf("CreateMenu !\n");
#endif
    hMenu = GlobalAlloc(GMEM_MOVEABLE, sizeof(MENUBAR));
    menu = (LPMENUBAR) GlobalLock(hMenu);
    if (menu == NULL) {
	GlobalFree(hMenu);
	return 0;
 	}
    hItem = GlobalAlloc(GMEM_MOVEABLE, sizeof(MENUITEM));
    if (hItem == 0) {
	GlobalFree(hMenu);
	return 0;
	}
    menu->menuDescription = 0;
    menu->nItems 	  = 0;
    menu->parentWidget    = NULL;
    menu->firstItem       = (LPMENUITEM) GlobalLock(hItem);
    menu->ownerWnd	  = 0;
    menu->menuBarWidget   = NULL;

    menu->firstItem->next 	= NULL;
    menu->firstItem->prev 	= NULL;
    menu->firstItem->child 	= NULL;
    menu->firstItem->parent 	= NULL;
    menu->firstItem->item_flags	= 0;
    menu->firstItem->item_id 	= 0;
    menu->firstItem->item_text 	= NULL;
    menu->firstItem->w 		= NULL;
    menu->firstItem->menu_w 	= NULL;
    return hMenu;
}


/**********************************************************************
 *			DestroyMenu		[USER.152]
 */
BOOL DestroyMenu(HMENU hMenu)
{
    LPPOPUPMENU lppop;
    LPMENUITEM 	lpitem, lpitem2;
#ifdef DEBUG_MENU
    printf("DestroyMenu (%04X) !\n", hMenu);
#endif
    if (hMenu == 0) return FALSE;
    lppop = (LPPOPUPMENU) GlobalLock(hMenu);
    if (lppop == NULL) return FALSE;
    if (lppop->hWnd) DestroyWindow (lppop->hWnd);
    lpitem = lppop->firstItem;
    while (lpitem->next != NULL) {
#ifdef DEBUG_MENU
	printf("DestroyMenu (%04X) // during loop items !\n", hMenu);
#endif
	if ((lpitem->item_flags & MF_POPUP) == MF_POPUP) {
	    DestroyMenu((HMENU)lpitem->item_id);
	    }
    	lpitem = (LPMENUITEM)lpitem->next;
    	}
    GlobalFree(hMenu);
#ifdef DEBUG_MENU
    printf("DestroyMenu (%04X) // End !\n", hMenu);
#endif
    return TRUE;
}


