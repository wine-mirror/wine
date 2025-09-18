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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <poll.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "macdrv.h"
#include "oleidl.h"

WINE_DEFAULT_DEBUG_CHANNEL(event);
WINE_DECLARE_DEBUG_CHANNEL(imm);

static pthread_mutex_t ime_mutex = PTHREAD_MUTEX_INITIALIZER;
static RECT ime_composition_rect;

/* return the name of an Mac event */
static const char *dbgstr_event(int type)
{
    static const char * const event_names[] = {
        "APP_ACTIVATED",
        "APP_DEACTIVATED",
        "APP_QUIT_REQUESTED",
        "DISPLAYS_CHANGED",
        "HOTKEY_PRESS",
        "IM_SET_TEXT",
        "KEY_PRESS",
        "KEY_RELEASE",
        "KEYBOARD_CHANGED",
        "LOST_PASTEBOARD_OWNERSHIP",
        "MOUSE_BUTTON",
        "MOUSE_MOVED_RELATIVE",
        "MOUSE_MOVED_ABSOLUTE",
        "MOUSE_SCROLL",
        "QUERY_EVENT",
        "QUERY_EVENT_NO_PREEMPT_WAIT",
        "REASSERT_WINDOW_POSITION",
        "RELEASE_CAPTURE",
        "SENT_TEXT_INPUT",
        "STATUS_ITEM_MOUSE_BUTTON",
        "STATUS_ITEM_MOUSE_MOVE",
        "WINDOW_BROUGHT_FORWARD",
        "WINDOW_CLOSE_REQUESTED",
        "WINDOW_DID_MINIMIZE",
        "WINDOW_DID_UNMINIMIZE",
        "WINDOW_DRAG_BEGIN",
        "WINDOW_DRAG_END",
        "WINDOW_FRAME_CHANGED",
        "WINDOW_GOT_FOCUS",
        "WINDOW_LOST_FOCUS",
        "WINDOW_MAXIMIZE_REQUESTED",
        "WINDOW_MINIMIZE_REQUESTED",
        "WINDOW_RESIZE_ENDED",
        "WINDOW_RESTORE_REQUESTED",
    };
    C_ASSERT(ARRAYSIZE(event_names) == NUM_EVENT_TYPES);

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

    if (mask & QS_HOTKEY)
        event_mask |= event_mask_for_type(HOTKEY_PRESS);

    if (mask & QS_KEY)
    {
        event_mask |= event_mask_for_type(KEY_PRESS);
        event_mask |= event_mask_for_type(KEY_RELEASE);
        event_mask |= event_mask_for_type(KEYBOARD_CHANGED);
    }

    if (mask & QS_MOUSEBUTTON)
    {
        event_mask |= event_mask_for_type(MOUSE_BUTTON);
        event_mask |= event_mask_for_type(MOUSE_SCROLL);
    }

    if (mask & QS_MOUSEMOVE)
    {
        event_mask |= event_mask_for_type(MOUSE_MOVED_RELATIVE);
        event_mask |= event_mask_for_type(MOUSE_MOVED_ABSOLUTE);
    }

    if (mask & QS_POSTMESSAGE)
    {
        event_mask |= event_mask_for_type(APP_ACTIVATED);
        event_mask |= event_mask_for_type(APP_DEACTIVATED);
        event_mask |= event_mask_for_type(APP_QUIT_REQUESTED);
        event_mask |= event_mask_for_type(DISPLAYS_CHANGED);
        event_mask |= event_mask_for_type(IM_SET_TEXT);
        event_mask |= event_mask_for_type(LOST_PASTEBOARD_OWNERSHIP);
        event_mask |= event_mask_for_type(STATUS_ITEM_MOUSE_BUTTON);
        event_mask |= event_mask_for_type(STATUS_ITEM_MOUSE_MOVE);
        event_mask |= event_mask_for_type(WINDOW_DID_UNMINIMIZE);
        event_mask |= event_mask_for_type(WINDOW_FRAME_CHANGED);
        event_mask |= event_mask_for_type(WINDOW_GOT_FOCUS);
        event_mask |= event_mask_for_type(WINDOW_LOST_FOCUS);
    }

    if (mask & QS_SENDMESSAGE)
    {
        event_mask |= event_mask_for_type(QUERY_EVENT);
        event_mask |= event_mask_for_type(QUERY_EVENT_NO_PREEMPT_WAIT);
        event_mask |= event_mask_for_type(REASSERT_WINDOW_POSITION);
        event_mask |= event_mask_for_type(RELEASE_CAPTURE);
        event_mask |= event_mask_for_type(SENT_TEXT_INPUT);
        event_mask |= event_mask_for_type(WINDOW_BROUGHT_FORWARD);
        event_mask |= event_mask_for_type(WINDOW_CLOSE_REQUESTED);
        event_mask |= event_mask_for_type(WINDOW_DRAG_BEGIN);
        event_mask |= event_mask_for_type(WINDOW_DRAG_END);
        event_mask |= event_mask_for_type(WINDOW_MAXIMIZE_REQUESTED);
        event_mask |= event_mask_for_type(WINDOW_MINIMIZE_REQUESTED);
        event_mask |= event_mask_for_type(WINDOW_RESIZE_ENDED);
        event_mask |= event_mask_for_type(WINDOW_RESTORE_REQUESTED);
    }

    return event_mask;
}

static void post_ime_update( HWND hwnd, UINT cursor_pos, WCHAR *comp_str, WCHAR *result_str )
{
    NtUserMessageCall( hwnd, WINE_IME_POST_UPDATE, cursor_pos, (LPARAM)comp_str,
                       result_str, NtUserImeDriverCall, FALSE );
}

/***********************************************************************
 *              macdrv_im_set_text
 */
static void macdrv_im_set_text(const macdrv_event *event)
{
    HWND hwnd = macdrv_get_window_hwnd(event->window);
    WCHAR *text = NULL;

    TRACE_(imm)("win %p/%p himc %p text %s complete %u\n", hwnd, event->window, event->im_set_text.himc,
                debugstr_cf(event->im_set_text.text), event->im_set_text.complete);

    if (event->im_set_text.text)
    {
        CFIndex length = CFStringGetLength(event->im_set_text.text);
        if (!(text = malloc((length + 1) * sizeof(WCHAR)))) return;
        if (length) CFStringGetCharacters(event->im_set_text.text, CFRangeMake(0, length), text);
        text[length] = 0;
    }

    if (event->im_set_text.complete) post_ime_update(hwnd, -1, NULL, text);
    else post_ime_update(hwnd,
                         MAKELONG(event->im_set_text.cursor_begin, event->im_set_text.cursor_end),
                         text, NULL);

    free(text);
}

/***********************************************************************
 *              macdrv_sent_text_input
 */
static void macdrv_sent_text_input(const macdrv_event *event)
{
    TRACE_(imm)("handled: %s\n", event->sent_text_input.handled ? "TRUE" : "FALSE");
    *event->sent_text_input.done = event->sent_text_input.handled ? 1 : -1;
}


/**************************************************************************
 *              drag_operations_to_dropeffects
 */
static DWORD drag_operations_to_dropeffects(uint32_t ops)
{
    DWORD effects = 0;
    if (ops & (DRAG_OP_COPY | DRAG_OP_GENERIC))
        effects |= DROPEFFECT_COPY;
    if (ops & DRAG_OP_MOVE)
        effects |= DROPEFFECT_MOVE;
    if (ops & (DRAG_OP_LINK | DRAG_OP_GENERIC))
        effects |= DROPEFFECT_LINK;
    return effects;
}


/**************************************************************************
 *              dropeffect_to_drag_operation
 */
static uint32_t dropeffect_to_drag_operation(DWORD effect, uint32_t ops)
{
    if (effect & DROPEFFECT_LINK && ops & DRAG_OP_LINK) return DRAG_OP_LINK;
    if (effect & DROPEFFECT_COPY && ops & DRAG_OP_COPY) return DRAG_OP_COPY;
    if (effect & DROPEFFECT_MOVE && ops & DRAG_OP_MOVE) return DRAG_OP_MOVE;
    if (effect & DROPEFFECT_LINK && ops & DRAG_OP_GENERIC) return DRAG_OP_GENERIC;
    if (effect & DROPEFFECT_COPY && ops & DRAG_OP_GENERIC) return DRAG_OP_GENERIC;

    return DRAG_OP_NONE;
}


/**************************************************************************
 *              query_drag_drop_drop
 */
static BOOL query_drag_drop_drop(macdrv_query *query)
{
    HWND hwnd = macdrv_get_window_hwnd(query->window);
    struct macdrv_win_data *data;

    if (!(data = get_win_data(hwnd)))
    {
        WARN("no win_data for win %p/%p\n", hwnd, query->window);
        return FALSE;
    }

    release_win_data(data);

    NtUserMessageCall(hwnd, WINE_DRAG_DROP_DROP, 0, 0, NULL, NtUserDragDropCall, FALSE);
    return TRUE;
}

/**************************************************************************
 *              query_drag_drop_enter
 */
static BOOL query_drag_drop_enter(macdrv_query *query)
{
    CFTypeRef pasteboard = query->drag_drop.pasteboard;
    struct format_entry *entries;
    UINT entries_size;

    if (!(entries = get_format_entries(pasteboard, &entries_size))) return FALSE;
    NtUserMessageCall(0, WINE_DRAG_DROP_ENTER, entries_size, (LPARAM)entries, NULL, NtUserDragDropCall, FALSE);
    free(entries);

    return TRUE;
}

/**************************************************************************
 *              query_drag_drop_leave
 */
static BOOL query_drag_drop_leave(macdrv_query *query)
{
    NtUserMessageCall(0, WINE_DRAG_DROP_LEAVE, 0, 0, NULL, NtUserDragDropCall, FALSE);
    return TRUE;
}


/**************************************************************************
 *              query_drag_drop_drag
 */
static BOOL query_drag_drop_drag(macdrv_query *query)
{
    HWND hwnd = macdrv_get_window_hwnd(query->window);
    struct macdrv_win_data *data = get_win_data(hwnd);
    DWORD effect;
    POINT point;

    if (!data)
    {
        WARN("no win_data for win %p/%p\n", hwnd, query->window);
        return FALSE;
    }

    effect = drag_operations_to_dropeffects(query->drag_drop.ops);
    point.x = query->drag_drop.x + data->rects.visible.left;
    point.y = query->drag_drop.y + data->rects.visible.top;
    release_win_data(data);

    effect = NtUserMessageCall(hwnd, WINE_DRAG_DROP_DRAG, MAKELONG(point.x, point.y), effect, NULL, NtUserDragDropCall, FALSE);
    if (!effect) return FALSE;

    query->drag_drop.ops = dropeffect_to_drag_operation(effect, query->drag_drop.ops);
    return TRUE;
}


/**************************************************************************
 *              query_ime_char_rect
 */
BOOL query_ime_char_rect(macdrv_query* query)
{
    HWND hwnd = macdrv_get_window_hwnd(query->window);
    void *himc = query->ime_char_rect.himc;
    CFRange *range = &query->ime_char_rect.range;

    TRACE_(imm)("win %p/%p himc %p range %ld-%ld\n", hwnd, query->window, himc, range->location,
                range->length);

    pthread_mutex_lock(&ime_mutex);
    query->ime_char_rect.rect = cgrect_from_rect(ime_composition_rect);
    pthread_mutex_unlock(&ime_mutex);

    TRACE_(imm)(" -> range %ld-%ld rect %s\n", range->location,
                range->length, wine_dbgstr_cgrect(query->ime_char_rect.rect));

    return TRUE;
}


/***********************************************************************
 *      SetIMECompositionRect (MACDRV.@)
 */
BOOL macdrv_SetIMECompositionRect(HWND hwnd, RECT rect)
{
    TRACE("hwnd %p, rect %s\n", hwnd, wine_dbgstr_rect(&rect));
    pthread_mutex_lock(&ime_mutex);
    ime_composition_rect = rect;
    pthread_mutex_unlock(&ime_mutex);
    return TRUE;
}


/***********************************************************************
 *      NotifyIMEStatus (MACDRV.@)
 */
void macdrv_NotifyIMEStatus( HWND hwnd, UINT status )
{
    TRACE_(imm)( "hwnd %p, status %#x\n", hwnd, status );
    if (!status) macdrv_clear_ime_text();
}


/***********************************************************************
 *              macdrv_query_event
 *
 * Handler for QUERY_EVENT and QUERY_EVENT_NO_PREEMPT_WAIT queries.
 */
static void macdrv_query_event(HWND hwnd, const macdrv_event *event)
{
    BOOL success = FALSE;
    macdrv_query *query = event->query_event.query;

    switch (query->type)
    {
        case QUERY_DRAG_DROP_ENTER:
            TRACE("QUERY_DRAG_DROP_ENTER\n");
            success = query_drag_drop_enter(query);
            break;
        case QUERY_DRAG_DROP_LEAVE:
            TRACE("QUERY_DRAG_DROP_LEAVE\n");
            success = query_drag_drop_leave(query);
            break;
        case QUERY_DRAG_DROP_DRAG:
            TRACE("QUERY_DRAG_DROP_DRAG\n");
            success = query_drag_drop_drag(query);
            break;
        case QUERY_DRAG_DROP_DROP:
            TRACE("QUERY_DRAG_DROP_DROP\n");
            success = query_drag_drop_drop(query);
            break;
        case QUERY_IME_CHAR_RECT:
            TRACE("QUERY_IME_CHAR_RECT\n");
            success = query_ime_char_rect(query);
            break;
        case QUERY_PASTEBOARD_DATA:
            TRACE("QUERY_PASTEBOARD_DATA\n");
            success = query_pasteboard_data(hwnd, query->pasteboard_data.type);
            break;
        case QUERY_RESIZE_SIZE:
            TRACE("QUERY_RESIZE_SIZE\n");
            success = query_resize_size(hwnd, query);
            break;
        case QUERY_RESIZE_START:
            TRACE("QUERY_RESIZE_START\n");
            success = query_resize_start(hwnd);
            break;
        case QUERY_MIN_MAX_INFO:
            TRACE("QUERY_MIN_MAX_INFO\n");
            success = query_min_max_info(hwnd);
            break;
        default:
            FIXME("unrecognized query type %d\n", query->type);
            break;
    }

    TRACE("success %d\n", success);
    query->status = success;
    macdrv_set_query_done(query);
}


/***********************************************************************
 *              macdrv_handle_event
 */
void macdrv_handle_event(const macdrv_event *event)
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
    case APP_ACTIVATED:
        macdrv_app_activated();
        break;
    case APP_DEACTIVATED:
        macdrv_app_deactivated();
        break;
    case APP_QUIT_REQUESTED:
        macdrv_app_quit_requested(event);
        break;
    case DISPLAYS_CHANGED:
        macdrv_displays_changed(event);
        break;
    case HOTKEY_PRESS:
        macdrv_hotkey_press(event);
        break;
    case IM_SET_TEXT:
        macdrv_im_set_text(event);
        break;
    case KEY_PRESS:
    case KEY_RELEASE:
        macdrv_key_event(hwnd, event);
        break;
    case KEYBOARD_CHANGED:
        macdrv_keyboard_changed(event);
        break;
    case LOST_PASTEBOARD_OWNERSHIP:
        macdrv_lost_pasteboard_ownership(hwnd);
        break;
    case MOUSE_BUTTON:
        macdrv_mouse_button(hwnd, event);
        break;
    case MOUSE_MOVED_RELATIVE:
    case MOUSE_MOVED_ABSOLUTE:
        macdrv_mouse_moved(hwnd, event);
        break;
    case MOUSE_SCROLL:
        macdrv_mouse_scroll(hwnd, event);
        break;
    case QUERY_EVENT:
    case QUERY_EVENT_NO_PREEMPT_WAIT:
        macdrv_query_event(hwnd, event);
        break;
    case REASSERT_WINDOW_POSITION:
        macdrv_reassert_window_position(hwnd);
        break;
    case RELEASE_CAPTURE:
        macdrv_release_capture(hwnd, event);
        break;
    case SENT_TEXT_INPUT:
        macdrv_sent_text_input(event);
        break;
    case STATUS_ITEM_MOUSE_BUTTON:
        macdrv_status_item_mouse_button(event);
        break;
    case STATUS_ITEM_MOUSE_MOVE:
        macdrv_status_item_mouse_move(event);
        break;
    case WINDOW_BROUGHT_FORWARD:
        macdrv_window_brought_forward(hwnd);
        break;
    case WINDOW_CLOSE_REQUESTED:
        macdrv_window_close_requested(hwnd);
        break;
    case WINDOW_DID_MINIMIZE:
        macdrv_window_did_minimize(hwnd);
        break;
    case WINDOW_DID_UNMINIMIZE:
        macdrv_window_did_unminimize(hwnd);
        break;
    case WINDOW_DRAG_BEGIN:
        macdrv_window_drag_begin(hwnd, event);
        break;
    case WINDOW_DRAG_END:
        macdrv_window_drag_end(hwnd);
        break;
    case WINDOW_FRAME_CHANGED:
        macdrv_window_frame_changed(hwnd, event);
        break;
    case WINDOW_GOT_FOCUS:
        macdrv_window_got_focus(hwnd, event);
        break;
    case WINDOW_LOST_FOCUS:
        macdrv_window_lost_focus(hwnd, event);
        break;
    case WINDOW_MAXIMIZE_REQUESTED:
        macdrv_window_maximize_requested(hwnd);
        break;
    case WINDOW_MINIMIZE_REQUESTED:
        macdrv_window_minimize_requested(hwnd);
        break;
    case WINDOW_RESIZE_ENDED:
        macdrv_window_resize_ended(hwnd);
        break;
    case WINDOW_RESTORE_REQUESTED:
        macdrv_window_restore_requested(hwnd, event);
        break;
    default:
        TRACE("    ignoring\n");
        break;
    }

    thread_data->current_event = prev;
}


static int check_fd_events( int fd, int events )
{
    struct pollfd pfd = {.fd = fd, .events = events};
    if (poll( &pfd, 1, 0 ) <= 0) return 0;
    return pfd.revents;
}

/***********************************************************************
 *              ProcessEvents   (MACDRV.@)
 */
BOOL macdrv_ProcessEvents(DWORD mask)
{
    struct macdrv_thread_data *data = macdrv_thread_data();
    macdrv_event_mask event_mask = get_event_mask(mask);
    macdrv_event *event;
    int count = 0;

    TRACE("mask %x\n", mask);

    if (!data) return FALSE;

    if (data->current_event && data->current_event->type != QUERY_EVENT &&
        data->current_event->type != QUERY_EVENT_NO_PREEMPT_WAIT &&
        data->current_event->type != APP_QUIT_REQUESTED &&
        data->current_event->type != WINDOW_DRAG_BEGIN)
        event_mask = 0;  /* don't process nested events */

    while (macdrv_copy_event_from_queue(data->queue, event_mask, &event))
    {
        count++;
        macdrv_handle_event(event);
        macdrv_release_event(event);
    }

    if (count) TRACE("processed %d events\n", count);
    return !check_fd_events(macdrv_get_event_queue_fd(data->queue), POLLIN);
}
