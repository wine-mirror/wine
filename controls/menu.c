static char RCSId[] = "$Id$";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";

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
	wndPtr = (WND *) GlobalLock(menu->ownerWnd);
	if (wndPtr == NULL)
	    return;

#ifdef DEBUG_MENU
	printf("Selected '%s' (%d).\n", 
	       this_item->item_text, this_item->item_id);
#endif
	
	CallWindowProc(wndPtr->lpfnWndProc, menu->ownerWnd, WM_COMMAND,
		       this_item->item_id, 0);
	
	GlobalUnlock(menu->ownerWnd);
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
