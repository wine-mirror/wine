/*
 * Up-down class extra info
 *
 * Copyright 1997 Dimitrie O. Paun
 */

#ifndef __WINE_UPDOWN_H
#define __WINE_UPDOWN_H

#include "windows.h"
#include "commctrl.h"

typedef struct
{
  UINT32      AccelCount;   /* Number of elements in AccelVect */
  UDACCEL*    AccelVect;    /* Vector containing AccelCount elements */
  INT32       Base;         /* Base to display nr in the buddy window */
  INT32       CurVal;       /* Current up-down value */
  INT32       MinVal;       /* Minimum up-down value */
  INT32       MaxVal;       /* Maximum up-down value */
  HWND32      Buddy;        /* Handle to the buddy window */
  INT32       Flags;        /* Internal Flags FLAG_* */
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
