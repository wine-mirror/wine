/* $Id$
 *
 * Menu definitions
 */

#ifndef MENU_H
#define MENU_H

#define MENU_MAGIC   0x554d  /* 'MU' */


typedef struct tagMENUITEM
{
    WORD	item_flags;
    WORD	item_id;
    RECT	rect;
    WORD	sel_key;
    HBITMAP	hCheckBit;
    HBITMAP	hUnCheckBit;
    char	*item_text;
	HANDLE	hText;	
} MENUITEM, *LPMENUITEM;


typedef struct tagPOPUPMENU
{
    HMENU       hNext;        /* Next menu (compatibility only, always 0) */
    WORD        wFlags;       /* Menu flags (MF_POPUP, MF_SYSMENU) */
    WORD        wMagic;       /* Magic number */
    HANDLE      hTaskQ;       /* Task queue for this menu */
    WORD	Width;        /* Width of the whole menu */
    WORD	Height;       /* Height of the whole menu */
    WORD	nItems;       /* Number of items in the menu */
    HWND	hWnd;	      /* Window containing the menu */
    HANDLE      hItems;       /* Handle to the items array */
    WORD	FocusedItem;  /* Currently focused item */
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
} MENUITEMTEMPLATE;

#endif /* MENU_H */
