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
    WINDOWPOS   winPos[1];
} DWP;

typedef struct
{
  HTASK        hWindowTask;
  HTASK        hTaskSendTo;
  BOOL         wFlag;
} ACTIVATESTRUCT, *LPACTIVATESTRUCT;

extern void WINPOS_FindIconPos( HWND hwnd );
extern BOOL WINPOS_SetActiveWindow( HWND hWnd, BOOL fMouse, BOOL fChangeFocus);
extern BOOL WINPOS_ChangeActiveWindow( HWND hwnd, BOOL mouseMsg );
extern LONG WINPOS_SendNCCalcSize( HWND hwnd, BOOL calcValidRect,
				   RECT *newWindowRect, RECT *oldWindowRect,
				   RECT *oldClientRect, SEGPTR winpos,
				   RECT *newClientRect );
extern LONG WINPOS_HandleWindowPosChanging( WINDOWPOS *winpos );
extern INT WINPOS_WindowFromPoint( POINT pt, WND **ppWnd );

#endif  /* __WINE_WINPOS_H */
