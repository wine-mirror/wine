/*
 * Window procedure callbacks definitions
 *
 * Copyright 1996 Alexandre Julliard
 */

#ifndef __WINE_WINPROC_H
#define __WINE_WINPROC_H

#include "wintypes.h"

typedef enum
{
    WIN_PROC_INVALID,
    WIN_PROC_16,
    WIN_PROC_32A,
    WIN_PROC_32W
} WINDOWPROCTYPE;

extern WNDPROC16 WINPROC_AllocWinProc( WNDPROC32 func, WINDOWPROCTYPE type );
extern WINDOWPROCTYPE WINPROC_GetWinProcType( WNDPROC16 func );
extern WNDPROC32 WINPROC_GetWinProcFunc( WNDPROC16 func );
extern void WINPROC_FreeWinProc( WNDPROC16 func );

#endif  /* __WINE_WINPROC_H */
