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
  HFONT32     hFont;        /* Handle to font (not unused) */
} PROGRESS_INFO;


extern VOID PROGRESS_Register (VOID);
extern VOID PROGRESS_Unregister (VOID);

#endif  /* __WINE_PROGRESS_H */
