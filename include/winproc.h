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

typedef void *HWINDOWPROC;  /* Really a pointer to a WINDOWPROC */

extern BOOL32 WINPROC_Init(void);
extern WNDPROC16 WINPROC_GetProc( HWINDOWPROC proc, WINDOWPROCTYPE type );
extern BOOL32 WINPROC_SetProc( HWINDOWPROC *pFirst, WNDPROC16 func,
                               WINDOWPROCTYPE type );
extern void WINPROC_FreeProc( HWINDOWPROC proc );
extern WINDOWPROCTYPE WINPROC_GetProcType( HWINDOWPROC proc );

#endif  /* __WINE_WINPROC_H */
