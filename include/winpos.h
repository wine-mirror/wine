/*
 * *DeferWindowPos() structure and definitions
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_WINPOS_H
#define __WINE_WINPOS_H

#include "win.h"

#define DWP_MAGIC  0x5057  /* 'WP' */

/* undocumented SWP flags - from SDK 3.1 */
#define SWP_NOCLIENTSIZE	0x0800
#define SWP_NOCLIENTMOVE	0x1000

typedef struct
{
    WORD        actualCount;
    WORD        suggestedCount;
    WORD        valid;
    WORD        wMagic;
    HWND        hwndParent;
    WINDOWPOS16 winPos[1];
} DWP;

extern void WINPOS_FindIconPos( HWND hwnd );
extern BOOL WINPOS_SetActiveWindow( HWND hWnd, BOOL fMouse, BOOL fChangeFocus);
extern BOOL WINPOS_ChangeActiveWindow( HWND hwnd, BOOL mouseMsg );
extern LONG WINPOS_SendNCCalcSize( HWND hwnd, BOOL calcValidRect,
                                  RECT16 *newWindowRect, RECT16 *oldWindowRect,
				  RECT16 *oldClientRect, SEGPTR winpos,
				  RECT16 *newClientRect );
extern LONG WINPOS_HandleWindowPosChanging16(WND *wndPtr, WINDOWPOS16 *winpos);
extern LONG WINPOS_HandleWindowPosChanging32(WND *wndPtr, WINDOWPOS32 *winpos);
extern INT16 WINPOS_WindowFromPoint( POINT16 pt, WND **ppWnd );

#endif  /* __WINE_WINPOS_H */
