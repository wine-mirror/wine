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

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "uxtheme.h"
#include "uxthemedll.h"
#include "vssym32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(theme_scroll);

void WINAPI UXTHEME_ScrollBarDraw(HWND hwnd, HDC dc, INT bar, enum SCROLL_HITTEST hit_test,
                                  const struct SCROLL_TRACKING_INFO *tracking_info,
                                  BOOL draw_arrows, BOOL draw_interior, RECT *rect, INT arrowsize,
                                  INT thumbpos, INT thumbsize, BOOL vertical)
{
    BOOL disabled = !IsWindowEnabled(hwnd);
    HTHEME theme;
    DWORD style;

    theme = bar == SB_CTL ? GetWindowTheme(hwnd) : OpenThemeData(NULL, WC_SCROLLBARW);
    if (!theme)
    {
        user_api.pScrollBarDraw(hwnd, dc, bar, hit_test, tracking_info, draw_arrows, draw_interior,
                                rect, arrowsize, thumbpos, thumbsize, vertical);
        return;
    }

    style = GetWindowLongW(hwnd, GWL_STYLE);
    if (bar == SB_CTL && (style & SBS_SIZEBOX || style & SBS_SIZEGRIP)) {
        int state;

        if (style & SBS_SIZEBOXTOPLEFTALIGN)
            state = SZB_TOPLEFTALIGN;
        else
            state = SZB_RIGHTALIGN;

        if (IsThemeBackgroundPartiallyTransparent(theme, SBP_SIZEBOX, state))
            DrawThemeParentBackground(hwnd, dc, NULL);
        DrawThemeBackground(theme, dc, SBP_SIZEBOX, state, rect, NULL);
    } else {
        int uppertrackstate, lowertrackstate, thumbstate;
        RECT partrect;
        SIZE grippersize;

        if (disabled) {
            uppertrackstate = SCRBS_DISABLED;
            lowertrackstate = SCRBS_DISABLED;
            thumbstate = SCRBS_DISABLED;
        } else {
            uppertrackstate = SCRBS_NORMAL;
            lowertrackstate = SCRBS_NORMAL;
            thumbstate = SCRBS_NORMAL;

            if (vertical == tracking_info->vertical && hit_test == tracking_info->hit_test
                && GetCapture() == hwnd)
            {
                if (hit_test == SCROLL_TOP_RECT)
                    uppertrackstate = SCRBS_PRESSED;
                else if (hit_test == SCROLL_BOTTOM_RECT)
                    lowertrackstate = SCRBS_PRESSED;
                else if (hit_test == SCROLL_THUMB)
                    thumbstate = SCRBS_PRESSED;
            }
            else
            {
                if (hit_test == SCROLL_TOP_RECT)
                    uppertrackstate = SCRBS_HOT;
                else if (hit_test == SCROLL_BOTTOM_RECT)
                    lowertrackstate = SCRBS_HOT;
                else if (hit_test == SCROLL_THUMB)
                    thumbstate = SCRBS_HOT;
            }

            /* Thumb is also shown as pressed when tracking */
            if (tracking_info->win == hwnd && tracking_info->bar == bar)
                thumbstate = SCRBS_PRESSED;
        }

        if (bar == SB_CTL)
            DrawThemeParentBackground(hwnd, dc, NULL);

        if (vertical) {
            int uparrowstate, downarrowstate;

            if (disabled) {
                uparrowstate = ABS_UPDISABLED;
                downarrowstate = ABS_DOWNDISABLED;
            } else {
                uparrowstate = ABS_UPNORMAL;
                downarrowstate = ABS_DOWNNORMAL;

                if (vertical == tracking_info->vertical && hit_test == tracking_info->hit_test
                    && GetCapture() == hwnd)
                {
                    if (hit_test == SCROLL_TOP_ARROW)
                        uparrowstate = ABS_UPPRESSED;
                    else if (hit_test == SCROLL_BOTTOM_ARROW)
                        downarrowstate = ABS_DOWNPRESSED;
                }
                else
                {
                    if (hit_test == SCROLL_TOP_ARROW)
                        uparrowstate = ABS_UPHOT;
                    else if (hit_test == SCROLL_BOTTOM_ARROW)
                        downarrowstate = ABS_DOWNHOT;
                }
            }

            partrect = *rect;
            partrect.bottom = partrect.top + arrowsize;
            DrawThemeBackground(theme, dc, SBP_ARROWBTN, uparrowstate, &partrect, NULL);

            partrect.bottom = rect->bottom;
            partrect.top = partrect.bottom - arrowsize;
            DrawThemeBackground(theme, dc, SBP_ARROWBTN, downarrowstate, &partrect, NULL);

            if (thumbpos > 0) {
                partrect.top = rect->top + arrowsize;
                partrect.bottom = rect->top + thumbpos;

                DrawThemeBackground(theme, dc, SBP_UPPERTRACKVERT, uppertrackstate, &partrect, NULL);
            }

            if (thumbsize > 0) {
                partrect.top = rect->top + thumbpos;
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

            partrect.bottom = rect->bottom - arrowsize;
            if (thumbsize > 0)
                partrect.top = rect->top + thumbpos + thumbsize;
            else
                partrect.top = rect->top + arrowsize;
            if (partrect.bottom > partrect.top)
                DrawThemeBackground(theme, dc, SBP_LOWERTRACKVERT, lowertrackstate, &partrect, NULL);
        } else {
            int leftarrowstate, rightarrowstate;

            if (disabled) {
                leftarrowstate = ABS_LEFTDISABLED;
                rightarrowstate = ABS_RIGHTDISABLED;
            } else {
                leftarrowstate = ABS_LEFTNORMAL;
                rightarrowstate = ABS_RIGHTNORMAL;

                if (vertical == tracking_info->vertical && hit_test == tracking_info->hit_test
                    && GetCapture() == hwnd)
                {
                    if (hit_test == SCROLL_TOP_ARROW)
                        leftarrowstate = ABS_LEFTPRESSED;
                    else if (hit_test == SCROLL_BOTTOM_ARROW)
                        rightarrowstate = ABS_RIGHTPRESSED;
                }
                else
                {
                    if (hit_test == SCROLL_TOP_ARROW)
                        leftarrowstate = ABS_LEFTHOT;
                    else if (hit_test == SCROLL_BOTTOM_ARROW)
                        rightarrowstate = ABS_RIGHTHOT;
                }
            }

            partrect = *rect;
            partrect.right = partrect.left + arrowsize;
            DrawThemeBackground(theme, dc, SBP_ARROWBTN, leftarrowstate, &partrect, NULL);

            partrect.right = rect->right;
            partrect.left = partrect.right - arrowsize;
            DrawThemeBackground(theme, dc, SBP_ARROWBTN, rightarrowstate, &partrect, NULL);

            if (thumbpos > 0) {
                partrect.left = rect->left + arrowsize;
                partrect.right = rect->left + thumbpos;

                DrawThemeBackground(theme, dc, SBP_UPPERTRACKHORZ, uppertrackstate, &partrect, NULL);
            }

            if (thumbsize > 0) {
                partrect.left = rect->left + thumbpos;
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

            partrect.right = rect->right - arrowsize;
            if (thumbsize > 0)
                partrect.left = rect->left + thumbpos + thumbsize;
            else
                partrect.left = rect->left + arrowsize;
            if (partrect.right > partrect.left)
                DrawThemeBackground(theme, dc, SBP_LOWERTRACKHORZ, lowertrackstate, &partrect, NULL);
        }
    }

    if (bar != SB_CTL)
        CloseThemeData(theme);
}

LRESULT WINAPI UXTHEME_ScrollbarWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                        BOOL unicode)
{
    const WCHAR* themeClass = WC_SCROLLBARW;
    HTHEME theme;
    LRESULT result;

    TRACE("(%p, 0x%x, %lu, %lu, %d)\n", hwnd, msg, wParam, lParam, unicode);

    switch (msg) {
        case WM_CREATE:
            result = user_api.pScrollBarWndProc(hwnd, msg, wParam, lParam, unicode);
            OpenThemeData(hwnd, themeClass);
            return result;

        case WM_DESTROY:
            theme = GetWindowTheme(hwnd);
            CloseThemeData(theme);
            return user_api.pScrollBarWndProc(hwnd, msg, wParam, lParam, unicode);

        case WM_THEMECHANGED:
            theme = GetWindowTheme(hwnd);
            CloseThemeData(theme);
            OpenThemeData(hwnd, themeClass);
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case WM_SYSCOLORCHANGE:
            theme = GetWindowTheme(hwnd);
            if (!theme) return user_api.pScrollBarWndProc(hwnd, msg, wParam, lParam, unicode);
            /* Do nothing. When themed, a WM_THEMECHANGED will be received, too,
             * which will do the repaint. */
            break;

        default:
            return user_api.pScrollBarWndProc(hwnd, msg, wParam, lParam, unicode);
    }

    return 0;
}
