/*
 * Scroll-bar class extra info
 *
 * Copyright 1993 Martin Ayotte
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_SCROLL_H
#define __WINE_SCROLL_H

#include "wintypes.h"

typedef struct
{
    INT32   CurVal;   /* Current scroll-bar value */
    INT32   MinVal;   /* Minimum scroll-bar value */
    INT32   MaxVal;   /* Maximum scroll-bar value */
    INT32   Page;     /* Page size of scroll bar (Win32) */
    UINT32  flags;    /* EnableScrollBar flags */
} SCROLLBAR_INFO;

extern LRESULT WINAPI ScrollBarWndProc( HWND32 hwnd, UINT32 uMsg,
                                        WPARAM32 wParam, LPARAM lParam );
extern void SCROLL_DrawScrollBar( HWND32 hwnd, HDC32 hdc, INT32 nBar,
                                  BOOL32 arrows, BOOL32 interior );
extern void SCROLL_HandleScrollEvent( HWND32 hwnd, INT32 nBar,
                                      UINT32 msg, POINT32 pt );

#endif  /* __WINE_SCROLL_H */
