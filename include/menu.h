/* $Id$
 *
 * Menu definitions
 */

#ifndef MENU_H
#define MENU_H

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Core.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Box.h>

#include "windows.h"

typedef struct tagMENUITEM
{
    struct tagMENUITEM *next;
    struct tagMENUITEM *prev;
    struct tagMENUITEM *child;
    struct tagMENUITEM *parent;
    WORD	item_flags;
    WORD	item_id;
    char       *item_text;
    Widget	w;
    Widget	menu_w;
    char	menu_name[10];
    RECT	rect;
    HBITMAP	hCheckBit;
    HBITMAP	hUnCheckBit;
} MENUITEM, *LPMENUITEM;

typedef struct tagMENUBAR
{
    struct tagMENUBAR *next;
    HANDLE	menuDescription;	/* Memory containing menu desc.   */
    HWND	ownerWnd;		/* Owner window			  */
    int		nItems;    		/* Number of items on menu	  */
    Widget	parentWidget;		/* Parent of menu widget	  */
    Widget	menuBarWidget;		/* Widget to contain menu options */
    MENUITEM   *firstItem;
} MENUBAR, *LPMENUBAR;

typedef struct tagPOPUPMENU
{
    HWND	hWnd;			/* PopupMenu window handle	  */
    HWND	ownerWnd;		/* Owner window			  */
    WORD	nItems;    		/* Number of items on menu	  */
    MENUITEM   *firstItem;
    WORD	FocusedItem;
    WORD	MouseFlags;
} POPUPMENU, *LPPOPUPMENU;

typedef struct
{
    WORD	version;		/* Should be zero		  */
    WORD	reserved;		/* Must be zero			  */
} MENU_HEADER;

typedef struct
{
    WORD	item_flags;		/* See windows.h		  */
    char	item_text[1];		/* Text for menu item		  */
} MENU_POPUPITEM;

typedef struct
{
    WORD	item_flags;		/* See windows.h		  */
    WORD	item_id;		/* Control Id for menu item	  */
    char	item_text[1];		/* Text for menu item		  */
} MENU_NORMALITEM;

extern LPMENUBAR MENU_CreateMenuBar(Widget parent, HANDLE instance, 
				    HWND wnd, char *menu_name, int width);
extern LPMENUBAR MENU_UseMenu(Widget parent, HANDLE instance, 
			      HWND wnd, HMENU hmenu, int width);

#endif /* MENU_H */
