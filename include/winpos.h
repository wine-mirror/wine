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


extern HWND WINPOS_NextWindowFromPoint( HWND hwnd, POINT pt );
extern HWND WINPOS_ChangeActiveWindow( HWND hwnd, BOOL mouseMsg );
extern void WINPOS_GetMinMaxInfo( HWND hwnd, POINT *maxSize, POINT *maxPos,
			    POINT *minTrack, POINT *maxTrack );
extern LONG WINPOS_SendNCCalcSize( HWND hwnd, BOOL calcValidRect,
				   RECT *newWindowRect, RECT *oldWindowRect,
				   RECT *oldClientRect, WINDOWPOS *winpos,
				   RECT *newClientRect );
extern LONG WINPOS_HandleWindowPosChanging( WINDOWPOS *winpos );

#endif  /* WINPOS_H */
