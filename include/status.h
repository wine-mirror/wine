/*
 * Status window definitions
 *
 * Copyright 1996 Bruce Milner
 */

#ifndef __WINE_STATUS_H
#define __WINE_STATUS_H

LRESULT WINAPI StatusWindowProc( HWND32 hwnd, UINT32 msg,
                                 WPARAM32 wParam, LPARAM lParam );

typedef struct
{
    INT32	x;
    INT32	style;
    RECT32	bound;
    LPSTR	text;
} STATUSWINDOWPART;

typedef struct
{
    UINT16              numParts;
    UINT16              textHeight;
    BOOL32              simple;
    STATUSWINDOWPART	part0;	/* simple window */
    STATUSWINDOWPART   *parts;
} STATUSWINDOWINFO;

#endif  /* __WINE_STATUS_H */
