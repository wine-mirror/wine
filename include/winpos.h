/*
 * *DeferWindowPos() structure and definitions
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_WINPOS_H
#define __WINE_WINPOS_H

#include "win.h"

#define DWP_MAGIC  0x5057  /* 'WP' */

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
				   RECT *oldClientRect, WINDOWPOS *winpos,
				   RECT *newClientRect );
extern LONG WINPOS_HandleWindowPosChanging( WINDOWPOS *winpos );
extern INT WINPOS_WindowFromPoint( POINT pt, WND **ppWnd );

#endif  /* __WINE_WINPOS_H */
