/*
 * Trackbar class extra info
 *
 * Copyright 1998 Eric Kohl
 */

#ifndef __WINE_TRACKBAR_H
#define __WINE_TRACKBAR_H

typedef struct tagTRACKBAR_INFO
{
    INT32  nRangeMin;
    INT32  nRangeMax;
    INT32  nLineSize;
    INT32  nPageSize;
    INT32  nSelMin;
    INT32  nSelMax;
    INT32  nPos;
    UINT32 uThumbLen;
    UINT32 uNumTics;
	UINT32  uTicFreq;
    HWND32 hwndToolTip;
    HWND32 hwndBuddyLA;
    HWND32 hwndBuddyRB;
    INT32  fLocation;
	COLORREF clrBk;
	
	INT32  flags;
    BOOL32 bFocus;
    RECT32 rcChannel;
    RECT32 rcSelection;
    RECT32 rcThumb;
    LPLONG tics;
} TRACKBAR_INFO;


extern void TRACKBAR_Register (void);

#endif  /* __WINE_TRACKBAR_H */
