/* $Id$
 *
 * Menu definitions
 */

#ifndef MENU_H
#define MENU_H



typedef struct tagMENUITEM
{
    struct tagMENUITEM *next;
    struct tagMENUITEM *prev;
    struct tagMENUITEM *child;
    struct tagMENUITEM *parent;
    WORD	item_flags;
    WORD	item_id;
    WORD	sel_key;
    char	*shortcut;
    char	*item_text;
    char	menu_name[10];
    RECT	rect;
    HBITMAP	hCheckBit;
    HBITMAP	hUnCheckBit;
} MENUITEM, *LPMENUITEM;


typedef struct tagPOPUPMENU
{
    HWND	hWnd;			/* PopupMenu window handle	  */
    HWND	hWndParent;		/* Parent opupMenu window handle  */
    HWND	ownerWnd;		/* Owner window			  */
    WORD	nItems;    		/* Number of items on menu	  */
    MENUITEM   *firstItem;
    WORD	FocusedItem;
    WORD	MouseFlags;
    WORD	BarFlags;
    BOOL	SysFlag;
    WORD	Width;
    WORD	Height;
    WORD	CheckWidth;
    WORD	PopWidth;
    RECT	rect;
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

void StdDrawMenuBar(HDC hDC, LPRECT lprect, LPPOPUPMENU lppop);
void MenuButtonDown(HWND hWnd, LPPOPUPMENU lppop, int x, int y);
void MenuButtonUp(HWND hWnd, LPPOPUPMENU lppop, int x, int y);
void MenuMouseMove(HWND hWnd, LPPOPUPMENU lppop, WORD wParam, int x, int y);
extern void NC_TrackSysMenu(HWND hwnd);

#endif /* MENU_H */
