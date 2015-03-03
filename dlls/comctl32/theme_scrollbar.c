/*
 * Theming - Scrollbar control
 *
 * Copyright (c) 2015 Mark Harmstone
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
 *
 */

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "uxtheme.h"
#include "vssym32.h"
#include "comctl32.h"
#include "wine/debug.h"

/* Minimum size of the thumb in pixels */
#define SCROLL_MIN_THUMB 6

/* Minimum size of the rectangle between the arrows */
#define SCROLL_MIN_RECT  4

WINE_DEFAULT_DEBUG_CHANNEL(theme_scroll);

static void calc_thumb_dimensions(unsigned int size, SCROLLINFO *si, unsigned int *thumbpos, unsigned int *thumbsize)
{
    if (size <= SCROLL_MIN_RECT)
        *thumbpos = *thumbsize = 0;
    else if (si->nPage > si->nMax - si->nMin)
        *thumbpos = *thumbsize = 0;
    else {
        if (si->nPage > 0) {
            *thumbsize = MulDiv(size, si->nPage, si->nMax - si->nMin + 1);
            if (*thumbsize < SCROLL_MIN_THUMB) *thumbsize = SCROLL_MIN_THUMB;
        }
        else *thumbsize = GetSystemMetrics(SM_CXVSCROLL);

        if (size < *thumbsize)
            *thumbpos = *thumbsize = 0;
        else {
            int max = si->nMax - max(si->nPage - 1, 0);
            size -= *thumbsize;
            if (si->nMin >= max)
                *thumbpos = 0;
            else
                *thumbpos = MulDiv(size, si->nTrackPos - si->nMin, max - si->nMin);
        }
    }
}

static void paint_scrollbar(HWND hwnd, HTHEME theme)
{
    HDC dc;
    PAINTSTRUCT ps;
    RECT r;
    DWORD style = GetWindowLongW(hwnd, GWL_STYLE);
    BOOL vertical = style & SBS_VERT;
    BOOL disabled = !IsWindowEnabled(hwnd);

    GetWindowRect(hwnd, &r);
    OffsetRect(&r, -r.left, -r.top);

    dc = BeginPaint(hwnd, &ps);

    if (style & SBS_SIZEBOX || style & SBS_SIZEGRIP) {
        int state;

        if (style & SBS_SIZEBOXTOPLEFTALIGN)
            state = SZB_TOPLEFTALIGN;
        else
            state = SZB_RIGHTALIGN;

        DrawThemeBackground(theme, dc, SBP_SIZEBOX, state, &r, NULL);
    } else {
        SCROLLBARINFO sbi;
        SCROLLINFO si;
        unsigned int thumbpos, thumbsize;
        int uppertrackstate, lowertrackstate, thumbstate;
        RECT partrect, trackrect;
        SIZE grippersize;

        sbi.cbSize = sizeof(sbi);
        GetScrollBarInfo(hwnd, OBJID_CLIENT, &sbi);

        si.cbSize = sizeof(si);
        si.fMask = SIF_ALL;
        GetScrollInfo(hwnd, SB_CTL, &si);

        trackrect = r;

        if (disabled) {
            uppertrackstate = SCRBS_DISABLED;
            lowertrackstate = SCRBS_DISABLED;
            thumbstate = SCRBS_DISABLED;
        } else {
            uppertrackstate = SCRBS_NORMAL;
            lowertrackstate = SCRBS_NORMAL;
            thumbstate = SCRBS_NORMAL;
        }

        if (vertical) {
            SIZE upsize, downsize;
            int uparrowstate, downarrowstate;

            if (disabled) {
                uparrowstate = ABS_UPDISABLED;
                downarrowstate = ABS_DOWNDISABLED;
            } else {
                uparrowstate = ABS_UPNORMAL;
                downarrowstate = ABS_DOWNNORMAL;
            }

            if (FAILED(GetThemePartSize(theme, dc, SBP_ARROWBTN, uparrowstate, NULL, TS_DRAW, &upsize))) {
                WARN("Could not get up arrow size.\n");
                return;
            }

            if (FAILED(GetThemePartSize(theme, dc, SBP_ARROWBTN, downarrowstate, NULL, TS_DRAW, &downsize))) {
                WARN("Could not get down arrow size.\n");
                return;
            }

            if (r.bottom - r.top - upsize.cy - downsize.cy < SCROLL_MIN_RECT)
                upsize.cy = downsize.cy = (r.bottom - r.top - SCROLL_MIN_RECT)/2;

            partrect = r;
            partrect.bottom = partrect.top + upsize.cy;
            DrawThemeBackground(theme, dc, SBP_ARROWBTN, uparrowstate, &partrect, NULL);

            trackrect.top = partrect.bottom;

            partrect.bottom = r.bottom;
            partrect.top = partrect.bottom - downsize.cy;
            DrawThemeBackground(theme, dc, SBP_ARROWBTN, downarrowstate, &partrect, NULL);

            trackrect.bottom = partrect.top;

            calc_thumb_dimensions(trackrect.bottom - trackrect.top, &si, &thumbpos, &thumbsize);

            if (thumbpos > 0) {
                partrect.top = trackrect.top;
                partrect.bottom = partrect.top + thumbpos;

                DrawThemeBackground(theme, dc, SBP_UPPERTRACKVERT, uppertrackstate, &partrect, NULL);
            }

            if (thumbsize > 0) {
                partrect.top = trackrect.top + thumbpos;
                partrect.bottom = partrect.top + thumbsize;

                DrawThemeBackground(theme, dc, SBP_THUMBBTNVERT, thumbstate, &partrect, NULL);

                if (SUCCEEDED(GetThemePartSize(theme, dc, SBP_GRIPPERVERT, thumbstate, NULL, TS_DRAW, &grippersize))) {
                    MARGINS margins;

                    if (SUCCEEDED(GetThemeMargins(theme, dc, SBP_THUMBBTNVERT, thumbstate, TMT_CONTENTMARGINS, &partrect, &margins))) {
                        if (grippersize.cy <= (thumbsize - margins.cyTopHeight - margins.cyBottomHeight))
                            DrawThemeBackground(theme, dc, SBP_GRIPPERVERT, thumbstate, &partrect, NULL);
                    }
                }
            }

            if (thumbpos + thumbsize < trackrect.bottom - trackrect.top) {
                partrect.bottom = trackrect.bottom;
                partrect.top = trackrect.top + thumbsize + thumbpos;

                DrawThemeBackground(theme, dc, SBP_LOWERTRACKVERT, lowertrackstate, &partrect, NULL);
            }
        } else {
            SIZE leftsize, rightsize;
            int leftarrowstate, rightarrowstate;

            if (disabled) {
                leftarrowstate = ABS_LEFTDISABLED;
                rightarrowstate = ABS_RIGHTDISABLED;
            } else {
                leftarrowstate = ABS_LEFTNORMAL;
                rightarrowstate = ABS_RIGHTNORMAL;
            }

            if (FAILED(GetThemePartSize(theme, dc, SBP_ARROWBTN, leftarrowstate, NULL, TS_DRAW, &leftsize))) {
                WARN("Could not get left arrow size.\n");
                return;
            }

            if (FAILED(GetThemePartSize(theme, dc, SBP_ARROWBTN, rightarrowstate, NULL, TS_DRAW, &rightsize))) {
                WARN("Could not get right arrow size.\n");
                return;
            }

            if (r.right - r.left - leftsize.cx - rightsize.cx < SCROLL_MIN_RECT)
                leftsize.cx = rightsize.cx = (r.right - r.left - SCROLL_MIN_RECT)/2;

            partrect = r;
            partrect.right = partrect.left + leftsize.cx;
            DrawThemeBackground(theme, dc, SBP_ARROWBTN, leftarrowstate, &partrect, NULL);

            trackrect.left = partrect.right;

            partrect.right = r.right;
            partrect.left = partrect.right - rightsize.cx;
            DrawThemeBackground(theme, dc, SBP_ARROWBTN, rightarrowstate, &partrect, NULL);

            trackrect.right = partrect.left;

            calc_thumb_dimensions(trackrect.right - trackrect.left, &si, &thumbpos, &thumbsize);

            if (thumbpos > 0) {
                partrect.left = trackrect.left;
                partrect.right = partrect.left + thumbpos;

                DrawThemeBackground(theme, dc, SBP_UPPERTRACKHORZ, uppertrackstate, &partrect, NULL);
            }

            if (thumbsize > 0) {
                partrect.left = trackrect.left + thumbpos;
                partrect.right = partrect.left + thumbsize;

                DrawThemeBackground(theme, dc, SBP_THUMBBTNHORZ, thumbstate, &partrect, NULL);

                if (SUCCEEDED(GetThemePartSize(theme, dc, SBP_GRIPPERHORZ, thumbstate, NULL, TS_DRAW, &grippersize))) {
                    MARGINS margins;

                    if (SUCCEEDED(GetThemeMargins(theme, dc, SBP_THUMBBTNHORZ, thumbstate, TMT_CONTENTMARGINS, &partrect, &margins))) {
                        if (grippersize.cx <= (thumbsize - margins.cxLeftWidth - margins.cxRightWidth))
                            DrawThemeBackground(theme, dc, SBP_GRIPPERHORZ, thumbstate, &partrect, NULL);
                    }
                }
            }

            if (thumbpos + thumbsize < trackrect.right - trackrect.left) {
                partrect.right = trackrect.right;
                partrect.left = trackrect.left + thumbsize + thumbpos;

                DrawThemeBackground(theme, dc, SBP_LOWERTRACKHORZ, lowertrackstate, &partrect, NULL);
            }
        }
    }

    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK THEMING_ScrollbarSubclassProc (HWND hwnd, UINT msg,
                                                WPARAM wParam, LPARAM lParam,
                                                ULONG_PTR dwRefData)
{
    const WCHAR* themeClass = WC_SCROLLBARW;
    HTHEME theme;
    LRESULT result;

    TRACE("(%p, 0x%x, %lu, %lu, %lu)\n", hwnd, msg, wParam, lParam, dwRefData);

    switch (msg) {
        case WM_CREATE:
            result = THEMING_CallOriginalClass(hwnd, msg, wParam, lParam);
            OpenThemeData(hwnd, themeClass);
            return result;

        case WM_DESTROY:
            theme = GetWindowTheme(hwnd);
            CloseThemeData(theme);
            return THEMING_CallOriginalClass(hwnd, msg, wParam, lParam);

        case WM_THEMECHANGED:
            theme = GetWindowTheme(hwnd);
            CloseThemeData(theme);
            OpenThemeData(hwnd, themeClass);
            break;

        case WM_SYSCOLORCHANGE:
            theme = GetWindowTheme(hwnd);
            if (!theme) return THEMING_CallOriginalClass(hwnd, msg, wParam, lParam);
            /* Do nothing. When themed, a WM_THEMECHANGED will be received, too,
             * which will do the repaint. */
            break;

        case WM_PAINT:
            theme = GetWindowTheme(hwnd);
            if (!theme) return THEMING_CallOriginalClass(hwnd, msg, wParam, lParam);

            paint_scrollbar(hwnd, theme);
            break;

        default:
            return THEMING_CallOriginalClass(hwnd, msg, wParam, lParam);
    }

    return 0;
}
