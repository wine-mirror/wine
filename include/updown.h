/*
 * Up-down class extra info
 *
 * Copyright 1997 Dimitrie O. Paun
 */

#ifndef __WINE_UPDOWN_H
#define __WINE_UPDOWN_H

#include "wintypes.h"
#include "commctrl.h"

typedef struct
{
  UINT      AccelCount;   /* Number of elements in AccelVect */
  UDACCEL*    AccelVect;    /* Vector containing AccelCount elements */
  INT       Base;         /* Base to display nr in the buddy window */
  INT       CurVal;       /* Current up-down value */
  INT       MinVal;       /* Minimum up-down value */
  INT       MaxVal;       /* Maximum up-down value */
  HWND      Buddy;        /* Handle to the buddy window */
  INT       Flags;        /* Internal Flags FLAG_* */
} UPDOWN_INFO;

typedef struct tagNM_UPDOWN
{
  NMHDR hdr;
  int iPos;
  int iDelta;
} NM_UPDOWN;

extern VOID UPDOWN_Register (VOID);
extern VOID UPDOWN_Unregister (VOID);

#endif  /* __WINE_UPDOWN_H */
