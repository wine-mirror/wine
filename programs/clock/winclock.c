/* 
 *  Clock (winclock.c)
 *
 *  Copyright 1998 by Marcel Baur <mbaur@g26.ethz.ch>
 *
 *  This file is based on  rolex.c  by Jim Peterson.
 *
 *  I just managed to move the relevant parts into the Clock application
 *  and made it look like the original Windows one. You can find the original
 *  rolex.c in the wine /libtest directory.
 *
 *  Original file header:
 *  >
 *  > rolex.c: Windows clock application for WINE (by Jim Peterson)    
 *  >                                                                 
 *  > This is a translation of a Turbo Pascal OWL application I made   
 *  > once, so it's a little flaky (tons of globals, functions that    
 *  > could have been in-lined, etc.).  The source code should easily  
 *  > compile with a standard Win32 C compiler.                        
 *  >                                                                 
 *  > To try it out, type 'make rolex'.                               
 *  >
 *
 */

#include <math.h>
#include <string.h>
#include "winclock.h"
#include "windows.h"
#include "main.h"

COLORREF FaceColor = RGB(192,192,192);
COLORREF HandColor = RGB(0,0,0);
COLORREF EtchColor = RGB(0,0,0);

float Pi=3.1415926;

HandData OldSecond,OldHour,OldMinute;

int MiddleX(void) {
  int X, diff;

  X    = (Globals.MaxX/2);  
  diff = (Globals.MaxX-Globals.MaxY);
  if (diff>0) { X = (X-(diff/2)); }
  return X;
}

int MiddleY(void) {
  int Y, diff;
  
  Y    = (Globals.MaxY/2);
  diff = (Globals.MaxX-Globals.MaxY);
  if (diff<0) { Y = Y+(diff/2); }
  return Y;
}

void DrawFace(HDC dc)
{
  int MidX, MidY, t, DiffX, DiffY;
  
  MidX = MiddleX();
  MidY = MiddleY();
  DiffX = (Globals.MaxX-MidX*2)/2;
  DiffY = (Globals.MaxY-MidY*2)/2;

  SelectObject(dc,CreateSolidBrush(FaceColor));
  SelectObject(dc,CreatePen(PS_SOLID,1,EtchColor));
  Ellipse(dc,DiffX,DiffY,2*MidX+DiffX,2*MidY+DiffY);

  for(t=0; t<12; t++)
  {
    MoveToEx(dc,(MidX+DiffX)+sin(t*Pi/6)*0.9*MidX,(MidY+DiffY)-cos(t*Pi/6)*0.9*MidY,NULL);
    LineTo(dc,(MidY+DiffX)+sin(t*Pi/6)*0.8*MidX,(MidY+DiffY)-cos(t*Pi/6)*0.8*MidY);
  }
  if(Globals.MaxX>64 && Globals.MaxY>64)
    for(t=0; t<60; t++)
      SetPixel(dc,(MidX+DiffX)+sin(t*Pi/30)*0.9*MidX,(MidY+DiffY)-cos(t*Pi/30)*0.9*MidY
	       ,EtchColor);
  DeleteObject(SelectObject(dc,GetStockObject(NULL_BRUSH)));
  DeleteObject(SelectObject(dc,GetStockObject(NULL_PEN)));
  memset(&OldSecond,0,sizeof(OldSecond));
  memset(&OldMinute,0,sizeof(OldMinute));
  memset(&OldHour,0,sizeof(OldHour));
}

void DrawHourHand(HDC dc)
{
  MoveToEx(dc, OldHour.StartX, OldHour.StartY, NULL);
  LineTo(dc, OldHour.EndX, OldHour.EndY);
}

void DrawMinuteHand(HDC dc)
{
  MoveToEx(dc, OldMinute.StartX, OldMinute.StartY, NULL);
  LineTo(dc, OldMinute.EndX, OldMinute.EndY);
}

void DrawSecondHand(HDC dc)
{
    MoveToEx(dc, OldSecond.StartX, OldSecond.StartY, NULL);
    LineTo(dc, OldSecond.EndX, OldSecond.EndY);
}

BOOL UpdateHourHand(HDC dc,int MidX,int MidY,int XExt,int YExt,WORD Pos)
{
  int Sx, Sy, Ex, Ey;
  BOOL rv;

  rv = FALSE;
  Sx = MidX; Sy = MidY;
  Ex = MidX+sin(Pos*Pi/6000)*XExt;
  Ey = MidY-cos(Pos*Pi/6000)*YExt;
  rv = ( Sx!=OldHour.StartX || Ex!=OldHour.EndX || 
	 Sy!=OldHour.StartY || Ey!=OldHour.EndY );
  if (Globals.bAnalog && rv)DrawHourHand(dc);
  OldHour.StartX = Sx; OldHour.EndX = Ex;
  OldHour.StartY = Sy; OldHour.EndY = Ey;
  return rv;
}

BOOL UpdateMinuteHand(HDC dc,int MidX,int MidY,int XExt,int YExt,WORD Pos)
{
  int Sx, Sy, Ex, Ey;
  BOOL rv;

  rv = FALSE;
  Sx = MidX; Sy = MidY;
  Ex = MidX+sin(Pos*Pi/30000)*XExt;
  Ey = MidY-cos(Pos*Pi/30000)*YExt;
  rv = ( Sx!=OldMinute.StartX || Ex!=OldMinute.EndX ||
	 Sy!=OldMinute.StartY || Ey!=OldMinute.EndY );
  if (Globals.bAnalog && rv)DrawMinuteHand(dc);
  OldMinute.StartX = Sx; OldMinute.EndX = Ex;
  OldMinute.StartY = Sy; OldMinute.EndY = Ey;
  return rv;
}

BOOL UpdateSecondHand(HDC dc,int MidX,int MidY,int XExt,int YExt,WORD Pos)
{
  int Sx, Sy, Ex, Ey;
  BOOL rv;

  rv = FALSE;
  
  if (Globals.bSeconds) {
    Sx = MidX; Sy = MidY;
    Ex = MidX+sin(Pos*Pi/3000)*XExt;
    Ey = MidY-cos(Pos*Pi/3000)*YExt;
    rv = ( Sx!=OldSecond.StartX || Ex!=OldSecond.EndX ||
  	 Sy!=OldSecond.StartY || Ey!=OldSecond.EndY );
    if (Globals.bAnalog && rv)DrawSecondHand(dc);
    OldSecond.StartX = Sx; OldSecond.EndX = Ex;
    OldSecond.StartY = Sy; OldSecond.EndY = Ey;
  }
  
  return rv;
}

void Idle(HDC idc)
{
  SYSTEMTIME st;
  WORD H, M, S, F;
  int MidX, MidY, DiffX, DiffY;
  HDC dc;
  BOOL Redraw;

  if(idc)
    dc=idc;
  else
    dc=GetDC(Globals.hMainWnd);
  if(!dc)return;

  GetLocalTime(&st);
  H = st.wHour;
  M = st.wMinute;
  S = st.wSecond;
  F = st.wMilliseconds / 10;
  F = F + S*100;
  M = M*1000+F/6;
  H = H*1000+M/60;
  MidX = MiddleX();
  MidY = MiddleY();
  DiffX = (Globals.MaxX-MidX*2)/2;
  DiffY = (Globals.MaxY-MidY*2)/2;

  SelectObject(dc,CreatePen(PS_SOLID,1,FaceColor));
  Redraw = FALSE;
  if(UpdateHourHand(dc,MidX+DiffX,MidY+DiffY,MidX*0.5,MidY*0.5,H)) Redraw = TRUE;
  if(UpdateMinuteHand(dc,MidX+DiffX,MidY+DiffY,MidX*0.65,MidY*0.65,M)) Redraw = TRUE;
  if(UpdateSecondHand(dc,MidX+DiffX,MidY+DiffY,MidX*0.79,MidY*0.79,F)) Redraw = TRUE;
  if (Globals.bAnalog)
  {
  DeleteObject(SelectObject(dc,CreatePen(PS_SOLID,1,HandColor)));
    if(Redraw)
    {
      DrawSecondHand(dc);
      DrawMinuteHand(dc);
      DrawHourHand(dc);
    }
  DeleteObject(SelectObject(dc,GetStockObject(NULL_PEN))); 
  }
  if(!idc) ReleaseDC(Globals.hMainWnd,dc);
}

//    class.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
