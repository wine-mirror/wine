/*
 * Progress class extra info
 *
 * Copyright 1997 Dimitrie O. Paun
 */

#ifndef __WINE_PROGRESS_H
#define __WINE_PROGRESS_H

#include "windows.h"
#include "commctrl.h"

typedef struct
{
  INT32       CurVal;       /* Current progress value */
  INT32       MinVal;       /* Minimum progress value */
  INT32       MaxVal;       /* Maximum progress value */
  INT32       Step;         /* Step to use on PMB_STEPIT */
  COLORREF    ColorBar;     /* Bar color */
  COLORREF    ColorBk;      /* Background color */
} PROGRESS_INFO;

LRESULT WINAPI ProgressWindowProc(HWND32, UINT32, WPARAM32, LPARAM);

#endif  /* __WINE_PROGRESS_H */
