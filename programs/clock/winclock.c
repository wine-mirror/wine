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
#include "winclock.h"

static COLORREF FaceColor = RGB(192,192,192);
static COLORREF HandColor = RGB(0,0,0);
static COLORREF EtchColor = RGB(0,0,0);
static COLORREF GRAY = RGB(128,128,128);
static const int ETCH_DEPTH = 2;
 
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

static void DrawHands(HDC dc, BOOL bSeconds)
{
    SelectObject(dc,CreatePen(PS_SOLID,1,HandColor));
    if (bSeconds)
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

static void PositionHands(const POINT* centre, int radius, BOOL bSeconds)
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
    if (bSeconds)
        PositionHand(centre, radius * 0.79, second/60 * 2*M_PI, &SecondHand);  
}

void AnalogClock(HDC dc, int x, int y, BOOL bSeconds)
{
    POINT centre;
    int radius;
    radius = min(x, y)/2;

    centre.x = x/2;
    centre.y = y/2;

    DrawFace(dc, &centre, radius);
    PositionHands(&centre, radius, bSeconds);
    DrawHands(dc, bSeconds);
}

void DigitalClock(HDC dc, int x, int y, BOOL bSeconds)
{
    CHAR szTime[255];
    SIZE extent;
    LOGFONT lf;
    double xscale, yscale;
    HFONT oldFont;

    GetTimeFormat(LOCALE_USER_DEFAULT, bSeconds ? 0 : TIME_NOSECONDS, NULL,
		  NULL, szTime, sizeof (szTime));

    memset(&lf, 0, sizeof (lf));
    lf.lfHeight = -20;

    x -= 2 * ETCH_DEPTH;
    y -= 2 * ETCH_DEPTH;

    oldFont = SelectObject(dc, CreateFontIndirect(&lf));
    GetTextExtentPoint(dc, szTime, strlen(szTime), &extent);
    xscale = (double)x/extent.cx;
    yscale = (double)y/extent.cy;
    lf.lfHeight *= min(xscale, yscale);

    DeleteObject(SelectObject(dc, CreateFontIndirect(&lf)));
    GetTextExtentPoint(dc, szTime, strlen(szTime), &extent);

    SetBkColor(dc, GRAY); /* to match the background brush */
    SetTextColor(dc, EtchColor);
    TextOut(dc, (x - extent.cx)/2 + ETCH_DEPTH, (y - extent.cy)/2 + ETCH_DEPTH,
	    szTime, strlen(szTime));
    SetBkMode(dc, TRANSPARENT);

    SetTextColor(dc, FaceColor);
    TextOut(dc, (x - extent.cx)/2, (y - extent.cy)/2, szTime, strlen(szTime));
    DeleteObject(SelectObject(dc, oldFont));
}
