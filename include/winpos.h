/*
 * *DeferWindowPos() structure and definitions
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef WINPOS_H
#define WINPOS_H

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
extern HWND WINPOS_ChangeActiveWindow( HWND hwnd, BOOL mouseMsg );
extern LONG WINPOS_SendNCCalcSize( HWND hwnd, BOOL calcValidRect,
				   RECT *newWindowRect, RECT *oldWindowRect,
				   RECT *oldClientRect, WINDOWPOS *winpos,
				   RECT *newClientRect );
extern LONG WINPOS_HandleWindowPosChanging( WINDOWPOS *winpos );

#endif  /* WINPOS_H */
