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
    INT32  nThumbLen;

    BOOL32 bFocus;
    RECT32 rcChannel;
} TRACKBAR_INFO;


extern void TRACKBAR_Register (void);

#endif  /* __WINE_TRACKBAR_H */
