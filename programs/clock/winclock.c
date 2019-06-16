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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "windows.h"
#include "winclock.h"

#define FaceColor (GetSysColor(COLOR_3DFACE))
#define HandColor (GetSysColor(COLOR_3DHIGHLIGHT))
#define TickColor (GetSysColor(COLOR_3DHIGHLIGHT))
#define ShadowColor (GetSysColor(COLOR_3DDKSHADOW))
#define BackgroundColor (GetSysColor(COLOR_3DFACE))

static const int SHADOW_DEPTH = 2;
 
typedef struct
{
    POINT Start;
    POINT End;
} HandData;

static HandData HourHand, MinuteHand, SecondHand;

static void DrawTicks(HDC dc, const POINT* centre, int radius)
{
    int t;

    /* Minute divisions */
    if (radius>64)
        for(t=0; t<60; t++) {
            MoveToEx(dc,
                     centre->x + sin(t*M_PI/30)*0.9*radius,
                     centre->y - cos(t*M_PI/30)*0.9*radius,
                     NULL);
	    LineTo(dc,
		   centre->x + sin(t*M_PI/30)*0.89*radius,
		   centre->y - cos(t*M_PI/30)*0.89*radius);
	}

    /* Hour divisions */
    for(t=0; t<12; t++) {

        MoveToEx(dc,
                 centre->x + sin(t*M_PI/6)*0.9*radius,
                 centre->y - cos(t*M_PI/6)*0.9*radius,
                 NULL);
        LineTo(dc,
               centre->x + sin(t*M_PI/6)*0.8*radius,
               centre->y - cos(t*M_PI/6)*0.8*radius);
    }
}

static void DrawFace(HDC dc, const POINT* centre, int radius, int border)
{
    /* Ticks */
    SelectObject(dc, CreatePen(PS_SOLID, 2, ShadowColor));
    OffsetWindowOrgEx(dc, -SHADOW_DEPTH, -SHADOW_DEPTH, NULL);
    DrawTicks(dc, centre, radius);
    DeleteObject(SelectObject(dc, CreatePen(PS_SOLID, 2, TickColor)));
    OffsetWindowOrgEx(dc, SHADOW_DEPTH, SHADOW_DEPTH, NULL);
    DrawTicks(dc, centre, radius);
    if (border)
    {
        SelectObject(dc, GetStockObject(NULL_BRUSH));
        DeleteObject(SelectObject(dc, CreatePen(PS_SOLID, 5, ShadowColor)));
        Ellipse(dc, centre->x - radius, centre->y - radius, centre->x + radius, centre->y + radius);
    }
    DeleteObject(SelectObject(dc, GetStockObject(NULL_PEN)));
}

static void DrawHand(HDC dc,HandData* hand)
{
    MoveToEx(dc, hand->Start.x, hand->Start.y, NULL);
    LineTo(dc, hand->End.x, hand->End.y);
}

static void DrawHands(HDC dc, BOOL bSeconds)
{
    if (bSeconds) {
#if 0
      	SelectObject(dc, CreatePen(PS_SOLID, 1, ShadowColor));
	OffsetWindowOrgEx(dc, -SHADOW_DEPTH, -SHADOW_DEPTH, NULL);
        DrawHand(dc, &SecondHand);
	DeleteObject(SelectObject(dc, CreatePen(PS_SOLID, 1, HandColor)));
	OffsetWindowOrgEx(dc, SHADOW_DEPTH, SHADOW_DEPTH, NULL);
#else
	SelectObject(dc, CreatePen(PS_SOLID, 1, HandColor));
#endif
        DrawHand(dc, &SecondHand);
	DeleteObject(SelectObject(dc, GetStockObject(NULL_PEN)));
    }

    SelectObject(dc, CreatePen(PS_SOLID, 4, ShadowColor));

    OffsetWindowOrgEx(dc, -SHADOW_DEPTH, -SHADOW_DEPTH, NULL);
    DrawHand(dc, &MinuteHand);
    DrawHand(dc, &HourHand);

    DeleteObject(SelectObject(dc, CreatePen(PS_SOLID, 4, HandColor)));
    OffsetWindowOrgEx(dc, SHADOW_DEPTH, SHADOW_DEPTH, NULL);
    DrawHand(dc, &MinuteHand);
    DrawHand(dc, &HourHand);

    DeleteObject(SelectObject(dc, GetStockObject(NULL_PEN)));
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

void AnalogClock(HDC dc, int x, int y, BOOL bSeconds, BOOL border)
{
    POINT centre;
    int radius;
    
    radius = min(x, y)/2 - SHADOW_DEPTH;
    if (radius < 0)
	return;

    centre.x = x/2;
    centre.y = y/2;

    DrawFace(dc, &centre, radius, border);

    PositionHands(&centre, radius, bSeconds);
    DrawHands(dc, bSeconds);
}


HFONT SizeFont(HDC dc, int x, int y, BOOL bSeconds, const LOGFONTW* font)
{
    SIZE extent;
    LOGFONTW lf;
    double xscale, yscale;
    HFONT oldFont, newFont;
    WCHAR szTime[255];
    int chars;

    chars = GetTimeFormatW(LOCALE_USER_DEFAULT, bSeconds ? 0 : TIME_NOSECONDS, NULL,
                           NULL, szTime, ARRAY_SIZE(szTime));
    if (!chars)
	return 0;

    --chars;

    lf = *font;
    lf.lfHeight = -20;

    x -= 2 * SHADOW_DEPTH;
    y -= 2 * SHADOW_DEPTH;

    oldFont = SelectObject(dc, CreateFontIndirectW(&lf));
    GetTextExtentPointW(dc, szTime, chars, &extent);
    DeleteObject(SelectObject(dc, oldFont));

    xscale = (double)x/extent.cx;
    yscale = (double)y/extent.cy;
    lf.lfHeight *= min(xscale, yscale);    
    newFont = CreateFontIndirectW(&lf);

    return newFont;
}

void DigitalClock(HDC dc, int x, int y, BOOL bSeconds, HFONT font)
{
    SIZE extent;
    HFONT oldFont;
    WCHAR szTime[255];
    int chars;

    chars = GetTimeFormatW(LOCALE_USER_DEFAULT, bSeconds ? 0 : TIME_NOSECONDS, NULL,
                           NULL, szTime, ARRAY_SIZE(szTime));
    if (!chars)
	return;
    --chars;

    oldFont = SelectObject(dc, font);
    GetTextExtentPointW(dc, szTime, chars, &extent);

    SetBkColor(dc, BackgroundColor);
    SetTextColor(dc, ShadowColor);
    TextOutW(dc, (x - extent.cx)/2 + SHADOW_DEPTH, (y - extent.cy)/2 + SHADOW_DEPTH, szTime, chars);
    SetBkMode(dc, TRANSPARENT);

    SetTextColor(dc, HandColor);
    TextOutW(dc, (x - extent.cx)/2, (y - extent.cy)/2, szTime, chars);

    SelectObject(dc, oldFont);
}
