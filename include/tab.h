/*
 * Tab control class extra info
 *
 * Copyright 1998 Anders Carlsson
 */

#ifndef __WINE_TAB_H  
#define __WINE_TAB_H

#include "commctrl.h"
#include "windef.h"

typedef struct tagTAB_ITEM
{
  UINT   mask;
  DWORD  dwState;
  LPSTR  pszText;
  INT    cchTextMax;
  INT    iImage;
  LPARAM lParam;
  RECT   rect;    /* bounding rectangle of the item relative to the
		   * leftmost item (the leftmost item, 0, would have a 
		   * "left" member of 0 in this rectangle) */
} TAB_ITEM;

typedef struct tagTAB_INFO
{
  UINT       uNumItem;        /* number of tab items */
  INT        tabHeight;       /* height of the tab row */
  INT        tabWidth;        /* width of tabs */
  HFONT      hFont;           /* handle to the current font */
  HCURSOR    hcurArrow;       /* handle to the current cursor */
  HIMAGELIST himl;            /* handle to a image list (may be 0) */
  HWND       hwndToolTip;     /* handle to tab's tooltip */
  UINT       cchTextMax;
  INT        leftmostVisible; /* Used for scrolling, this member contains
			       * the index of the first visible item */
  INT        iSelected;       /* the currently selected item */
  INT        uFocus;          /* item which has the focus */
  TAB_ITEM*  items;           /* pointer to an array of TAB_ITEM's */
  BOOL       DoRedraw;        /* flag for redrawing when tab contents is changed*/
  BOOL       needsScrolling;  /* TRUE if the size of the tabs is greater than 
			       * the size of the control */
  HWND       hwndUpDown;      /* Updown control used for scrolling */
} TAB_INFO;


extern VOID TAB_Register (VOID);
extern VOID TAB_Unregister (VOID);

#endif  /* __WINE_TAB_H */
