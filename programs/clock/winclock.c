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
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "windows.h"
#include "winnls.h"
#include "winclock.h"

#define MIN(a,b) (((a)<(b))?(a):(b))

COLORREF FaceColor = RGB(192,192,192);
COLORREF HandColor = RGB(0,0,0);
COLORREF EtchColor = RGB(0,0,0);

typedef struct
{
    POINT Start;
    POINT End;
} HandData;

HandData HourHand, MinuteHand, SecondHand;

static void DrawFace(HDC dc, const POINT* centre, int radius)
{
    int t;

    SelectObject(dc,CreateSolidBrush(FaceColor));
    SelectObject(dc,CreatePen(PS_SOLID,1,EtchColor));
    Ellipse(dc,
            centre->x - radius, centre->y - radius,
            centre->x + radius, centre->y + radius);

    for(t=0; t<12; t++) {
        MoveToEx(dc,
                 centre->x + sin(t*M_PI/6)*0.9*radius,
                 centre->y - cos(t*M_PI/6)*0.9*radius,
                 NULL);
        LineTo(dc,
               centre->x + sin(t*M_PI/6)*0.8*radius,
               centre->y - cos(t*M_PI/6)*0.8*radius);
    }
    if (radius>64)
        for(t=0; t<60; t++)
            SetPixel(dc,
                     centre->x + sin(t*M_PI/30)*0.9*radius,
                     centre->y - cos(t*M_PI/30)*0.9*radius,
                     EtchColor);
    DeleteObject(SelectObject(dc,GetStockObject(NULL_BRUSH)));
    DeleteObject(SelectObject(dc,GetStockObject(NULL_PEN)));
}

static void DrawHand(HDC dc,HandData* hand)
{
    MoveToEx(dc, hand->Start.x, hand->Start.y, NULL);
    LineTo(dc, hand->End.x, hand->End.y);
}

static void DrawHands(HDC dc)
{
    SelectObject(dc,CreatePen(PS_SOLID,1,HandColor));
    DrawHand(dc, &SecondHand);
    DrawHand(dc, &MinuteHand);
    DrawHand(dc, &HourHand);
    DeleteObject(SelectObject(dc,GetStockObject(NULL_PEN)));
}

static void PositionHand(const POINT* centre, double length, double angle, HandData* hand)
{
    hand->Start = *centre;
    hand->End.x = centre->x + sin(angle)*length;
    hand->End.y = centre->y - cos(angle)*length;
}

static void PositionHands(const POINT* centre, int radius)
{
    SYSTEMTIME st;
    double hour, minute, second;

    /* 0 <= hour,minute,second < 2pi */
    /* Adding the millisecond count makes the second hand move more smoothly */

    GetLocalTime(&st);
    second = st.wSecond + st.wMilliseconds/1000.0;
    minute = st.wMinute + second/60.0;
    hour   = st.wHour % 12 + minute/60.0;

    PositionHand(centre, radius * 0.5,  hour/12   * 2*M_PI, &HourHand);
    PositionHand(centre, radius * 0.65, minute/60 * 2*M_PI, &MinuteHand);
    PositionHand(centre, radius * 0.79, second/60 * 2*M_PI, &SecondHand);  
}

void AnalogClock(HDC dc, int x, int y)
{
    POINT centre;
    int radius;
    radius = MIN(x, y)/2;

    centre.x = x/2;
    centre.y = y/2;

    DrawFace(dc, &centre, radius);
    PositionHands(&centre, radius);
    DrawHands(dc);
}

void DigitalClock(HDC dc, int X, int Y)
{
    /* FIXME - this doesn't work very well */
    CHAR szTime[255];
    static short xChar, yChar;
    TEXTMETRIC tm;
    SYSTEMTIME st;
    
    GetLocalTime(&st);
    GetTimeFormat(LOCALE_USER_DEFAULT, LOCALE_STIMEFORMAT, &st, NULL,
                  szTime, sizeof szTime);    
    xChar = tm.tmAveCharWidth;
    yChar = tm.tmHeight;
    xChar = 100;
    yChar = 100;

    SelectObject(dc,CreatePen(PS_SOLID,1,FaceColor));
    TextOut (dc, xChar, yChar, szTime, strlen (szTime));
    DeleteObject(SelectObject(dc,GetStockObject(NULL_PEN)));   
}
