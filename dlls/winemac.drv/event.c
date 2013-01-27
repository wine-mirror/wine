/*
 * MACDRV event driver
 *
 * Copyright 1993 Alexandre Julliard
 *           1999 Noel Borthwick
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

WINE_DEFAULT_DEBUG_CHANNEL(event);


/* return the name of an Mac event */
static const char *dbgstr_event(int type)
{
    static const char * const event_names[] = {
        "APP_DEACTIVATED",
        "MOUSE_BUTTON",
        "WINDOW_CLOSE_REQUESTED",
        "WINDOW_FRAME_CHANGED",
        "WINDOW_GOT_FOCUS",
        "WINDOW_LOST_FOCUS",
    };

    if (0 <= type && type < NUM_EVENT_TYPES) return event_names[type];
    return wine_dbg_sprintf("Unknown event %d", type);
}


/***********************************************************************
 *              get_event_mask
 */
static macdrv_event_mask get_event_mask(DWORD mask)
{
    macdrv_event_mask event_mask = 0;

    if ((mask & QS_ALLINPUT) == QS_ALLINPUT) return -1;

    if (mask & QS_MOUSEBUTTON)
        event_mask |= event_mask_for_type(MOUSE_BUTTON);

    if (mask & QS_POSTMESSAGE)
    {
        event_mask |= event_mask_for_type(APP_DEACTIVATED);
        event_mask |= event_mask_for_type(WINDOW_CLOSE_REQUESTED);
        event_mask |= event_mask_for_type(WINDOW_FRAME_CHANGED);
        event_mask |= event_mask_for_type(WINDOW_GOT_FOCUS);
        event_mask |= event_mask_for_type(WINDOW_LOST_FOCUS);
    }

    return event_mask;
}


/***********************************************************************
 *              macdrv_handle_event
 */
void macdrv_handle_event(macdrv_event *event)
{
    HWND hwnd = macdrv_get_window_hwnd(event->window);
    const macdrv_event *prev;
    struct macdrv_thread_data *thread_data = macdrv_thread_data();

    TRACE("%s for hwnd/window %p/%p\n", dbgstr_event(event->type), hwnd,
          event->window);

    prev = thread_data->current_event;
    thread_data->current_event = event;

    switch (event->type)
    {
    case APP_DEACTIVATED:
        macdrv_app_deactivated();
        break;
    case MOUSE_BUTTON:
        macdrv_mouse_button(hwnd, event);
        break;
    case WINDOW_CLOSE_REQUESTED:
        macdrv_window_close_requested(hwnd);
        break;
    case WINDOW_FRAME_CHANGED:
        macdrv_window_frame_changed(hwnd, event->window_frame_changed.frame);
        break;
    case WINDOW_GOT_FOCUS:
        macdrv_window_got_focus(hwnd, event);
        break;
    case WINDOW_LOST_FOCUS:
        macdrv_window_lost_focus(hwnd, event);
        break;
    default:
        TRACE("    ignoring\n");
        break;
    }

    thread_data->current_event = prev;
}


/***********************************************************************
 *              process_events
 */
static int process_events(macdrv_event_queue queue, macdrv_event_mask mask)
{
    macdrv_event event;
    int count = 0;

    while (macdrv_get_event_from_queue(queue, mask, &event))
    {
        count++;
        macdrv_handle_event(&event);
        macdrv_cleanup_event(&event);
    }
    if (count) TRACE("processed %d events\n", count);
    return count;
}


/***********************************************************************
 *              MsgWaitForMultipleObjectsEx   (MACDRV.@)
 */
DWORD CDECL macdrv_MsgWaitForMultipleObjectsEx(DWORD count, const HANDLE *handles,
                                               DWORD timeout, DWORD mask, DWORD flags)
{
    DWORD ret;
    struct macdrv_thread_data *data = macdrv_thread_data();
    macdrv_event_mask event_mask = get_event_mask(mask);

    TRACE("count %d, handles %p, timeout %u, mask %x, flags %x\n", count,
          handles, timeout, mask, flags);

    if (!data)
    {
        if (!count && !timeout) return WAIT_TIMEOUT;
        return WaitForMultipleObjectsEx(count, handles, flags & MWMO_WAITALL,
                                        timeout, flags & MWMO_ALERTABLE);
    }

    if (data->current_event) event_mask = 0;  /* don't process nested events */

    if (process_events(data->queue, event_mask)) ret = count - 1;
    else if (count || timeout)
    {
        ret = WaitForMultipleObjectsEx(count, handles, flags & MWMO_WAITALL,
                                       timeout, flags & MWMO_ALERTABLE);
        if (ret == count - 1) process_events(data->queue, event_mask);
    }
    else ret = WAIT_TIMEOUT;

    return ret;
}
