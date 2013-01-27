/*
 * MACDRV mouse driver
 *
 * Copyright 1998 Ulrich Weigand
 * Copyright 2007 Henri Verbeet
 * Copyright 2011, 2012, 2013 Ken Thomases for CodeWeavers Inc.
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

#include "config.h"

#include "macdrv.h"
#include "winuser.h"
#include "wine/server.h"

WINE_DEFAULT_DEBUG_CHANNEL(cursor);


/***********************************************************************
 *              send_mouse_input
 *
 * Update the various window states on a mouse event.
 */
static void send_mouse_input(HWND hwnd, UINT flags, int x, int y,
                             DWORD mouse_data, unsigned long time)
{
    INPUT input;
    HWND top_level_hwnd;

    top_level_hwnd = GetAncestor(hwnd, GA_ROOT);

    if ((flags & MOUSEEVENTF_MOVE) && (flags & MOUSEEVENTF_ABSOLUTE))
    {
        RECT rect;

        /* update the wine server Z-order */
        SetRect(&rect, x, y, x + 1, y + 1);
        MapWindowPoints(0, top_level_hwnd, (POINT *)&rect, 2);

        SERVER_START_REQ(update_window_zorder)
        {
            req->window      = wine_server_user_handle(top_level_hwnd);
            req->rect.left   = rect.left;
            req->rect.top    = rect.top;
            req->rect.right  = rect.right;
            req->rect.bottom = rect.bottom;
            wine_server_call(req);
        }
        SERVER_END_REQ;
    }

    input.type              = INPUT_MOUSE;
    input.mi.dx             = x;
    input.mi.dy             = y;
    input.mi.mouseData      = mouse_data;
    input.mi.dwFlags        = flags;
    input.mi.time           = time;
    input.mi.dwExtraInfo    = 0;

    __wine_send_input(top_level_hwnd, &input);
}


/***********************************************************************
 *              macdrv_mouse_button
 *
 * Handler for MOUSE_BUTTON events.
 */
void macdrv_mouse_button(HWND hwnd, const macdrv_event *event)
{
    UINT flags = 0;
    WORD data = 0;

    TRACE("win %p button %d %s at (%d,%d) time %lu (%lu ticks ago)\n", hwnd, event->mouse_button.button,
          (event->mouse_button.pressed ? "pressed" : "released"),
          event->mouse_button.x, event->mouse_button.y,
          event->mouse_button.time_ms, (GetTickCount() - event->mouse_button.time_ms));

    if (event->mouse_button.pressed)
    {
        switch (event->mouse_button.button)
        {
        case 0: flags |= MOUSEEVENTF_LEFTDOWN; break;
        case 1: flags |= MOUSEEVENTF_RIGHTDOWN; break;
        case 2: flags |= MOUSEEVENTF_MIDDLEDOWN; break;
        default:
            flags |= MOUSEEVENTF_XDOWN;
            data = 1 << (event->mouse_button.button - 3);
            break;
        }
    }
    else
    {
        switch (event->mouse_button.button)
        {
        case 0: flags |= MOUSEEVENTF_LEFTUP; break;
        case 1: flags |= MOUSEEVENTF_RIGHTUP; break;
        case 2: flags |= MOUSEEVENTF_MIDDLEUP; break;
        default:
            flags |= MOUSEEVENTF_XUP;
            data = 1 << (event->mouse_button.button - 3);
            break;
        }
    }

    send_mouse_input(hwnd, flags | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE,
                     event->mouse_button.x, event->mouse_button.y,
                     data, event->mouse_button.time_ms);
}
