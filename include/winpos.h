/*
 * *DeferWindowPos() structure and definitions
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef WINPOS_H
#define WINPOS_H

#define DWP_MAGIC  0x5057  /* 'WP' */

extern HWND WINPOS_ChangeActiveWindow( HWND hwnd, BOOL mouseMsg ); /*winpos.c*/
extern void WINPOS_GetMinMaxInfo( HWND hwnd, POINT *maxSize, POINT *maxPos,
			    POINT *minTrack, POINT *maxTrack );  /* winpos.c */
extern LONG WINPOS_SendNCCalcSize( HWND hwnd, BOOL calcValidRect,
				   RECT *newWindowRect, RECT *oldWindowRect,
				   RECT *oldClientRect, WINDOWPOS *winpos,
				   RECT *newClientRect );  /* winpos.c */
extern LONG WINPOS_HandleWindowPosChanging( WINDOWPOS *winpos ); /* winpos.c */

typedef struct
{
    WORD        actualCount;
    WORD        suggestedCount;
    WORD        valid;
    WORD        wMagic;
    HWND        hwndParent;
    WINDOWPOS   winPos[1];
} DWP;

#endif  /* WINPOS_H */
