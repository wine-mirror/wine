/* 
 *  Clock (winclock.h)
 *
 *  Copyright 1998 by Marcel Baur <mbaur@g26.ethz.ch>
 *  This file is essentially rolex.c by Jim Peterson.
 *  Please see my winclock.c and/or his rolex.c for references.
 *
 */

 #include <windows.h>
 
typedef struct
{
  int StartX,StartY,EndX,EndY;
  BOOL DontRedraw;
} HandData;

extern HandData OldMinute, OldHour, OldSecond;

/* function prototypes */


void DrawFace(HDC dc);
void DrawHourHand(HDC dc);
void DrawMinuteHand(HDC dc);
void DrawSecondHand(HDC dc);
BOOL UpdateHourHand(HDC dc,int MidX,int MidY,int XExt,int YExt,WORD Pos);
BOOL UpdateMinuteHand(HDC dc,int MidX,int MidY,int XExt,int YExt,WORD Pos);
BOOL UpdateSecondHand(HDC dc,int MidX,int MidY,int XExt,int YExt,WORD Pos);
void Idle(HDC idc);
