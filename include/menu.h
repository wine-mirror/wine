/* $Id$
 *
 * Menu definitions
 */

#ifndef MENU_H
#define MENU_H

#define MENU_MAGIC   0x554d  /* 'MU' */

extern BOOL MENU_Init(void);
extern UINT MENU_GetMenuBarHeight( HWND hwnd, UINT menubarWidth,
				   int orgX, int orgY );         /* menu.c */
extern void MENU_TrackMouseMenuBar( HWND hwnd, POINT pt );       /* menu.c */
extern void MENU_TrackKbdMenuBar( HWND hwnd, UINT wParam );      /* menu.c */
extern UINT MENU_DrawMenuBar( HDC hDC, LPRECT lprect,
			      HWND hwnd, BOOL suppress_draw );   /* menu.c */
extern HMENU CopySysMenu(); /* menu.c */

extern LRESULT PopupMenuWndProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam );

typedef struct tagMENUITEM
{
    WORD	item_flags;    /* Item flags */
    UINT	item_id;       /* Item or popup id */
    RECT	rect;          /* Item area (relative to menu window) */
    WORD        xTab;          /* X position of text after Tab */
    HBITMAP	hCheckBit;     /* Bitmap for checked item */
    HBITMAP	hUnCheckBit;   /* Bitmap for unchecked item */
    HANDLE      hText;	       /* Handle to item string or bitmap */
    char	*item_text;
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
    UINT	FocusedItem;  /* Currently focused item */
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
