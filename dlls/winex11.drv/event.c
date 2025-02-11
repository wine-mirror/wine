/*
 * X11 event driver
 *
 * Copyright 1993 Alexandre Julliard
 *	     1999 Noel Borthwick
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
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#ifdef HAVE_X11_EXTENSIONS_XINPUT2_H
#include <X11/extensions/XInput2.h>
#endif

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "x11drv.h"
#include "shellapi.h"

#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(event);
WINE_DECLARE_DEBUG_CHANNEL(xdnd);

#define DndNotDnd       -1    /* OffiX drag&drop */
#define DndUnknown      0
#define DndRawData      1
#define DndFile         2
#define DndFiles        3
#define DndText         4
#define DndDir          5
#define DndLink         6
#define DndExe          7

#define DndEND          8

#define DndURL          128   /* KDE drag&drop */

#define XEMBED_EMBEDDED_NOTIFY        0
#define XEMBED_WINDOW_ACTIVATE        1
#define XEMBED_WINDOW_DEACTIVATE      2
#define XEMBED_REQUEST_FOCUS          3
#define XEMBED_FOCUS_IN               4
#define XEMBED_FOCUS_OUT              5
#define XEMBED_FOCUS_NEXT             6
#define XEMBED_FOCUS_PREV             7
#define XEMBED_MODALITY_ON            10
#define XEMBED_MODALITY_OFF           11
#define XEMBED_REGISTER_ACCELERATOR   12
#define XEMBED_UNREGISTER_ACCELERATOR 13
#define XEMBED_ACTIVATE_ACCELERATOR   14

Bool (*pXGetEventData)( Display *display, XEvent /*XGenericEventCookie*/ *event ) = NULL;
void (*pXFreeEventData)( Display *display, XEvent /*XGenericEventCookie*/ *event ) = NULL;

  /* Event handlers */
static BOOL X11DRV_FocusIn( HWND hwnd, XEvent *event );
static BOOL X11DRV_FocusOut( HWND hwnd, XEvent *event );
static BOOL X11DRV_Expose( HWND hwnd, XEvent *event );
static BOOL X11DRV_MapNotify( HWND hwnd, XEvent *event );
static BOOL X11DRV_UnmapNotify( HWND hwnd, XEvent *event );
static BOOL X11DRV_ReparentNotify( HWND hwnd, XEvent *event );
static BOOL X11DRV_GravityNotify( HWND hwnd, XEvent *event );
static BOOL X11DRV_ConfigureNotify( HWND hwnd, XEvent *event );
static BOOL X11DRV_PropertyNotify( HWND hwnd, XEvent *event );
static BOOL X11DRV_ClientMessage( HWND hwnd, XEvent *event );

#define MAX_EVENT_HANDLERS 128

static x11drv_event_handler handlers[MAX_EVENT_HANDLERS] =
{
    NULL,                     /*  0 reserved */
    NULL,                     /*  1 reserved */
    X11DRV_KeyEvent,          /*  2 KeyPress */
    X11DRV_KeyEvent,          /*  3 KeyRelease */
    X11DRV_ButtonPress,       /*  4 ButtonPress */
    X11DRV_ButtonRelease,     /*  5 ButtonRelease */
    X11DRV_MotionNotify,      /*  6 MotionNotify */
    X11DRV_EnterNotify,       /*  7 EnterNotify */
    NULL,                     /*  8 LeaveNotify */
    X11DRV_FocusIn,           /*  9 FocusIn */
    X11DRV_FocusOut,          /* 10 FocusOut */
    X11DRV_KeymapNotify,      /* 11 KeymapNotify */
    X11DRV_Expose,            /* 12 Expose */
    NULL,                     /* 13 GraphicsExpose */
    NULL,                     /* 14 NoExpose */
    NULL,                     /* 15 VisibilityNotify */
    NULL,                     /* 16 CreateNotify */
    X11DRV_DestroyNotify,     /* 17 DestroyNotify */
    X11DRV_UnmapNotify,       /* 18 UnmapNotify */
    X11DRV_MapNotify,         /* 19 MapNotify */
    NULL,                     /* 20 MapRequest */
    X11DRV_ReparentNotify,    /* 21 ReparentNotify */
    X11DRV_ConfigureNotify,   /* 22 ConfigureNotify */
    NULL,                     /* 23 ConfigureRequest */
    X11DRV_GravityNotify,     /* 24 GravityNotify */
    NULL,                     /* 25 ResizeRequest */
    NULL,                     /* 26 CirculateNotify */
    NULL,                     /* 27 CirculateRequest */
    X11DRV_PropertyNotify,    /* 28 PropertyNotify */
    X11DRV_SelectionClear,    /* 29 SelectionClear */
    X11DRV_SelectionRequest,  /* 30 SelectionRequest */
    NULL,                     /* 31 SelectionNotify */
    NULL,                     /* 32 ColormapNotify */
    X11DRV_ClientMessage,     /* 33 ClientMessage */
    X11DRV_MappingNotify,     /* 34 MappingNotify */
    X11DRV_GenericEvent       /* 35 GenericEvent */
};

static const char * event_names[MAX_EVENT_HANDLERS] =
{
    NULL, NULL, "KeyPress", "KeyRelease", "ButtonPress", "ButtonRelease",
    "MotionNotify", "EnterNotify", "LeaveNotify", "FocusIn", "FocusOut",
    "KeymapNotify", "Expose", "GraphicsExpose", "NoExpose", "VisibilityNotify",
    "CreateNotify", "DestroyNotify", "UnmapNotify", "MapNotify", "MapRequest",
    "ReparentNotify", "ConfigureNotify", "ConfigureRequest", "GravityNotify", "ResizeRequest",
    "CirculateNotify", "CirculateRequest", "PropertyNotify", "SelectionClear", "SelectionRequest",
    "SelectionNotify", "ColormapNotify", "ClientMessage", "MappingNotify", "GenericEvent"
};

/* is someone else grabbing the keyboard, for example the WM, when manipulating the window */
BOOL keyboard_grabbed = FALSE;

int xinput2_opcode = 0;

/* return the name of an X event */
static const char *dbgstr_event( int type )
{
    if (type < MAX_EVENT_HANDLERS && event_names[type]) return event_names[type];
    return wine_dbg_sprintf( "Unknown event %d", type );
}

static inline void get_event_data( XEvent *event )
{
#if defined(GenericEvent) && defined(HAVE_XEVENT_XCOOKIE)
    if (event->xany.type != GenericEvent) return;
    if (!pXGetEventData || !pXGetEventData( event->xany.display, event )) event->xcookie.data = NULL;
#endif
}

static inline void free_event_data( XEvent *event )
{
#if defined(GenericEvent) && defined(HAVE_XEVENT_XCOOKIE)
    if (event->xany.type != GenericEvent) return;
    if (event->xcookie.data) pXFreeEventData( event->xany.display, event );
#endif
}

static void host_window_send_gravity_events( struct host_window *win, Display *display, unsigned long serial, XEvent *previous )
{
    XGravityEvent event = {.type = GravityNotify, .serial = serial, .display = display};
    unsigned int i;

    for (i = 0; i < win->children_count; i++)
    {
        RECT rect = win->children[i].rect;

        event.event = win->children[i].window;
        event.window = event.event;
        event.x = rect.left;
        event.y = rect.top;
        event.send_event = 0;

        if (previous->type == ConfigureNotify && previous->xconfigure.window == event.window) continue;
        TRACE( "generating GravityNotify for window %lx, rect %s\n", event.window, wine_dbgstr_rect(&rect) );
        XPutBackEvent( event.display, (XEvent *)&event );
    }
}

static BOOL host_window_filter_event( XEvent *event, XEvent *previous )
{
    struct host_window *win;

    if (!(win = get_host_window( event->xany.window, FALSE ))) return FALSE;

    switch (event->type)
    {
    case DestroyNotify:
        TRACE( "host window %p/%lx DestroyNotify\n", win, win->window );
        win->destroyed = TRUE;
        break;
    case ReparentNotify:
    {
        XReparentEvent *reparent = (XReparentEvent *)event;
        TRACE( "host window %p/%lx ReparentNotify, parent %lx\n", win, win->window, reparent->parent );
        host_window_set_parent( win, reparent->parent );
        host_window_send_gravity_events( win, event->xany.display, event->xany.serial, previous );
        break;
    }
    case GravityNotify:
    {
        XGravityEvent *gravity = (XGravityEvent *)event;
        OffsetRect( &win->rect, gravity->x - win->rect.left, gravity->y - win->rect.top );
        if (win->parent) win->rect = host_window_configure_child( win->parent, win->window, win->rect, FALSE );
        TRACE( "host window %p/%lx GravityNotify, rect %s\n", win, win->window, wine_dbgstr_rect(&win->rect) );
        host_window_send_gravity_events( win, event->xany.display, event->xany.serial, previous );
        break;
    }
    case ConfigureNotify:
    {
        XConfigureEvent *configure = (XConfigureEvent *)event;
        SetRect( &win->rect, configure->x, configure->y, configure->x + configure->width, configure->y + configure->height );
        if (win->parent) win->rect = host_window_configure_child( win->parent, win->window, win->rect, configure->send_event );
        TRACE( "host window %p/%lx ConfigureNotify, rect %s\n", win, win->window, wine_dbgstr_rect(&win->rect) );
        host_window_send_gravity_events( win, event->xany.display, event->xany.serial, previous );
        break;
    }
    }

    return TRUE;
}

/***********************************************************************
 *           xembed_request_focus
 */
static void xembed_request_focus( Display *display, Window window, DWORD timestamp )
{
    XEvent xev;

    xev.xclient.type = ClientMessage;
    xev.xclient.window = window;
    xev.xclient.message_type = x11drv_atom(_XEMBED);
    xev.xclient.serial = 0;
    xev.xclient.display = display;
    xev.xclient.send_event = True;
    xev.xclient.format = 32;

    xev.xclient.data.l[0] = timestamp;
    xev.xclient.data.l[1] = XEMBED_REQUEST_FOCUS;
    xev.xclient.data.l[2] = 0;
    xev.xclient.data.l[3] = 0;
    xev.xclient.data.l[4] = 0;

    XSendEvent(display, window, False, NoEventMask, &xev);
    XFlush( display );
}

/***********************************************************************
 *           X11DRV_register_event_handler
 *
 * Register a handler for a given event type.
 * If already registered, overwrite the previous handler.
 */
void X11DRV_register_event_handler( int type, x11drv_event_handler handler, const char *name )
{
    assert( type < MAX_EVENT_HANDLERS );
    assert( !handlers[type] || handlers[type] == handler );
    handlers[type] = handler;
    event_names[type] = name;
    TRACE("registered handler %p for event %d %s\n", handler, type, debugstr_a(name) );
}


/***********************************************************************
 *           filter_event
 */
static Bool filter_event( Display *display, XEvent *event, char *arg )
{
    ULONG_PTR mask = (ULONG_PTR)arg;

    if ((mask & QS_ALLINPUT) == QS_ALLINPUT) return 1;

    switch(event->type)
    {
    case KeyPress:
    case KeyRelease:
    case KeymapNotify:
    case MappingNotify:
        return (mask & (QS_KEY | QS_HOTKEY | QS_RAWINPUT)) != 0;
    case ButtonPress:
    case ButtonRelease:
        return (mask & (QS_MOUSEBUTTON | QS_RAWINPUT)) != 0;
    case MotionNotify:
    case EnterNotify:
    case LeaveNotify:
        return (mask & (QS_MOUSEMOVE | QS_RAWINPUT)) != 0;
    case Expose:
        return (mask & QS_PAINT) != 0;
    case FocusIn:
    case FocusOut:
    case MapNotify:
    case UnmapNotify:
    case ConfigureNotify:
    case PropertyNotify:
    case ClientMessage:
        return (mask & (QS_POSTMESSAGE | QS_SENDMESSAGE)) != 0;
#ifdef GenericEvent
    case GenericEvent:
#ifdef HAVE_X11_EXTENSIONS_XINPUT2_H
        if (event->xcookie.extension == xinput2_opcode) return (mask & QS_INPUT) != 0;
#endif
        /* fallthrough */
#endif
    default:
        return (mask & QS_SENDMESSAGE) != 0;
    }
}


enum event_merge_action
{
    MERGE_DISCARD,  /* discard the old event */
    MERGE_HANDLE,   /* handle the old event */
    MERGE_KEEP,     /* keep the old event for future merging */
    MERGE_IGNORE    /* ignore the new event, keep the old one */
};

/***********************************************************************
 *           merge_raw_motion_events
 */
#ifdef HAVE_X11_EXTENSIONS_XINPUT2_H
static enum event_merge_action merge_raw_motion_events( XIRawEvent *prev, XIRawEvent *next )
{
    int i, j, k;
    unsigned char mask;

    if (!prev->valuators.mask_len) return MERGE_HANDLE;
    if (!next->valuators.mask_len) return MERGE_HANDLE;

    mask = prev->valuators.mask[0] | next->valuators.mask[0];
    if (mask == next->valuators.mask[0])  /* keep next */
    {
        for (i = j = k = 0; i < 8; i++)
        {
            if (XIMaskIsSet( prev->valuators.mask, i ))
                next->valuators.values[j] += prev->valuators.values[k++];
            if (XIMaskIsSet( next->valuators.mask, i )) j++;
        }
        TRACE( "merging duplicate GenericEvent\n" );
        return MERGE_DISCARD;
    }
    if (mask == prev->valuators.mask[0])  /* keep prev */
    {
        for (i = j = k = 0; i < 8; i++)
        {
            if (XIMaskIsSet( next->valuators.mask, i ))
                prev->valuators.values[j] += next->valuators.values[k++];
            if (XIMaskIsSet( prev->valuators.mask, i )) j++;
        }
        TRACE( "merging duplicate GenericEvent\n" );
        return MERGE_IGNORE;
    }
    /* can't merge events with disjoint masks */
    return MERGE_HANDLE;
}
#endif

/***********************************************************************
 *           merge_events
 *
 * Try to merge 2 consecutive events.
 */
static enum event_merge_action merge_events( XEvent *prev, XEvent *next )
{
    switch (prev->type)
    {
    case ConfigureNotify:
        switch (next->type)
        {
        case ConfigureNotify:
            if (prev->xany.window == next->xany.window)
            {
                TRACE( "discarding duplicate ConfigureNotify for window %lx\n", prev->xany.window );
                return MERGE_DISCARD;
            }
            break;
        case Expose:
        case PropertyNotify:
            return MERGE_KEEP;
        }
        break;
    case MotionNotify:
        switch (next->type)
        {
        case MotionNotify:
            if (prev->xany.window == next->xany.window)
            {
                TRACE( "discarding duplicate MotionNotify for window %lx\n", prev->xany.window );
                return MERGE_DISCARD;
            }
            break;
#ifdef HAVE_X11_EXTENSIONS_XINPUT2_H
        case GenericEvent:
            if (next->xcookie.extension != xinput2_opcode) break;
            if (next->xcookie.evtype != XI_RawMotion) break;
            if (x11drv_thread_data()->warp_serial) break;
            return MERGE_KEEP;
        }
        break;
    case GenericEvent:
        if (prev->xcookie.extension != xinput2_opcode) break;
        if (prev->xcookie.evtype != XI_RawMotion) break;
        switch (next->type)
        {
        case GenericEvent:
            if (next->xcookie.extension != xinput2_opcode) break;
            if (next->xcookie.evtype != XI_RawMotion) break;
            if (x11drv_thread_data()->warp_serial) break;
            return merge_raw_motion_events( prev->xcookie.data, next->xcookie.data );
#endif
        }
        break;
    }
    return MERGE_HANDLE;
}


/***********************************************************************
 *           call_event_handler
 */
static inline BOOL call_event_handler( Display *display, XEvent *event )
{
    HWND hwnd;
    XEvent *prev;
    struct x11drv_thread_data *thread_data;
    BOOL ret;

    if (!handlers[event->type])
    {
        TRACE( "%s for win %lx, ignoring\n", dbgstr_event( event->type ), event->xany.window );
        return FALSE;  /* no handler, ignore it */
    }

#ifdef GenericEvent
    if (event->type == GenericEvent) hwnd = 0; else
#endif
    if (XFindContext( display, event->xany.window, winContext, (char **)&hwnd ) != 0)
        hwnd = 0;  /* not for a registered window */
    if (!hwnd && event->xany.window == root_window) hwnd = NtUserGetDesktopWindow();

    TRACE( "%lu %s for hwnd/window %p/%lx\n",
           event->xany.serial, dbgstr_event( event->type ), hwnd, event->xany.window );
    thread_data = x11drv_thread_data();
    prev = thread_data->current_event;
    thread_data->current_event = event;
    ret = handlers[event->type]( hwnd, event );
    thread_data->current_event = prev;
    return ret;
}


/***********************************************************************
 *           process_events
 */
static BOOL process_events( Display *display, Bool (*filter)(Display*, XEvent*,XPointer), ULONG_PTR arg )
{
    XEvent event, prev_event;
    int count = 0;
    BOOL queued = FALSE;
    enum event_merge_action action = MERGE_DISCARD;

    prev_event.type = 0;
    while (XCheckIfEvent( display, &event, filter, (char *)arg ))
    {
        count++;
        if (XFilterEvent( &event, None ))
        {
            /*
             * SCIM on linux filters key events strangely. It does not filter the
             * KeyPress events for these keys however it does filter the
             * KeyRelease events. This causes wine to become very confused as
             * to the keyboard state.
             *
             * We need to let those KeyRelease events be processed so that the
             * keyboard state is correct.
             */
            if (event.type == KeyRelease)
            {
                KeySym keysym = 0;
                XKeyEvent *keyevent = &event.xkey;

                XLookupString(keyevent, NULL, 0, &keysym, NULL);
                if (!(keysym == XK_Shift_L ||
                    keysym == XK_Shift_R ||
                    keysym == XK_Control_L ||
                    keysym == XK_Control_R ||
                    keysym == XK_Alt_R ||
                    keysym == XK_Alt_L ||
                    keysym == XK_Meta_R ||
                    keysym == XK_Meta_L))
                        continue; /* not a key we care about, ignore it */
            }
            else
                continue;  /* filtered, ignore it */
        }

        if (host_window_filter_event( &event, &prev_event )) continue;

        get_event_data( &event );
        if (prev_event.type) action = merge_events( &prev_event, &event );
        switch( action )
        {
        case MERGE_HANDLE:  /* handle prev, keep new */
            queued |= call_event_handler( display, &prev_event );
            /* fall through */
        case MERGE_DISCARD:  /* discard prev, keep new */
            free_event_data( &prev_event );
            prev_event = event;
            break;
        case MERGE_KEEP:  /* handle new, keep prev for future merging */
            queued |= call_event_handler( display, &event );
            /* fall through */
        case MERGE_IGNORE: /* ignore new, keep prev for future merging */
            free_event_data( &event );
            break;
        }
    }
    if (prev_event.type) queued |= call_event_handler( display, &prev_event );
    free_event_data( &prev_event );
    XFlush( gdi_display );
    if (count) TRACE( "processed %d events, returning %d\n", count, queued );
    return queued;
}


/***********************************************************************
 *           ProcessEvents   (X11DRV.@)
 */
BOOL X11DRV_ProcessEvents( DWORD mask )
{
    struct x11drv_thread_data *data = x11drv_thread_data();

    if (!data) return FALSE;
    if (data->current_event) mask = 0;  /* don't process nested events */

    return process_events( data->display, filter_event, mask );
}

/***********************************************************************
 *           EVENT_x11_time_to_win32_time
 *
 * Make our timer and the X timer line up as best we can
 *  Pass 0 to retrieve the current adjustment value (times -1)
 */
DWORD EVENT_x11_time_to_win32_time(Time time)
{
  static DWORD adjust = 0;
  DWORD now = NtGetTickCount();
  DWORD ret;

  if (! adjust && time != 0)
  {
    ret = now;
    adjust = time - now;
  }
  else
  {
      /* If we got an event in the 'future', then our clock is clearly wrong. 
         If we got it more than 10000 ms in the future, then it's most likely
         that the clock has wrapped.  */

      ret = time - adjust;
      if (ret > now && ((ret - now) < 10000) && time != 0)
      {
        adjust += ret - now;
        ret    -= ret - now;
      }
  }

  return ret;

}

/*******************************************************************
 *         can_activate_window
 *
 * Check if we can activate the specified window.
 */
static inline BOOL can_activate_window( HWND hwnd )
{
    LONG style = NtUserGetWindowLongW( hwnd, GWL_STYLE );
    struct x11drv_win_data *data;
    RECT rect;

    if ((data = get_win_data( hwnd )))
    {
        style = style & ~(WS_VISIBLE | WS_MINIMIZE | WS_MAXIMIZE);
        if (data->current_state.wm_state != WithdrawnState) style |= WS_VISIBLE;
        if (data->current_state.wm_state == IconicState) style |= WS_MINIMIZE;
        if (data->current_state.net_wm_state & (1 << NET_WM_STATE_MAXIMIZED)) style |= WS_MAXIMIZE;
        release_win_data( data );
    }

    if (!(style & WS_VISIBLE)) return FALSE;
    if ((style & (WS_POPUP|WS_CHILD)) == WS_CHILD) return FALSE;
    if (style & WS_MINIMIZE) return FALSE;
    if (NtUserGetWindowLongW( hwnd, GWL_EXSTYLE ) & WS_EX_NOACTIVATE) return FALSE;
    if (hwnd == NtUserGetDesktopWindow()) return FALSE;
    if (NtUserGetWindowRect( hwnd, &rect, NtUserGetDpiForWindow( hwnd ) ) && IsRectEmpty( &rect )) return FALSE;
    return !(style & WS_DISABLED);
}


/**********************************************************************
 *              set_input_focus
 *
 * Try to force focus for embedded or non-managed windows.
 */
static void set_input_focus( struct x11drv_win_data *data )
{
    XWindowChanges changes;
    DWORD timestamp;

    if (!data->whole_window) return;

    if (EVENT_x11_time_to_win32_time(0))
        /* ICCCM says don't use CurrentTime, so try to use last message time if possible */
        /* FIXME: this is not entirely correct */
        timestamp = NtUserGetThreadInfo()->message_time - EVENT_x11_time_to_win32_time(0);
    else
        timestamp = CurrentTime;

    /* Set X focus and install colormap */
    changes.stack_mode = Above;
    XConfigureWindow( data->display, data->whole_window, CWStackMode, &changes );

    if (data->embedder)
        xembed_request_focus( data->display, data->embedder, timestamp );
    else
        XSetInputFocus( data->display, data->whole_window, RevertToParent, timestamp );

}

/**********************************************************************
 *              set_focus
 */
static void set_focus( Display *display, HWND hwnd, Time time )
{
    HWND focus;
    Window win;
    GUITHREADINFO threadinfo;

    TRACE( "setting foreground window to %p\n", hwnd );
    NtUserSetForegroundWindow( hwnd );

    threadinfo.cbSize = sizeof(threadinfo);
    NtUserGetGUIThreadInfo( 0, &threadinfo );
    focus = threadinfo.hwndFocus;
    if (!focus) focus = threadinfo.hwndActive;
    if (focus) focus = NtUserGetAncestor( focus, GA_ROOT );
    win = X11DRV_get_whole_window(focus);

    if (win)
    {
        TRACE( "setting focus to %p (%lx) time=%ld\n", focus, win, time );
        XSetInputFocus( display, win, RevertToParent, time );
    }
}


/**********************************************************************
 *              handle_manager_message
 */
static void handle_manager_message( HWND hwnd, XClientMessageEvent *event )
{
    if (hwnd != NtUserGetDesktopWindow()) return;

    if (systray_atom && event->data.l[1] == systray_atom)
    {
        TRACE( "new owner %lx\n", event->data.l[2] );
        NtUserPostMessage( systray_hwnd, WM_USER + 1, 0, 0 );
    }
}


/**********************************************************************
 *              handle_wm_protocols
 */
static void handle_wm_protocols( HWND hwnd, XClientMessageEvent *event )
{
    Atom protocol = (Atom)event->data.l[0];
    Time event_time = (Time)event->data.l[1];

    if (!protocol) return;

    if (protocol == x11drv_atom(WM_DELETE_WINDOW))
    {
        update_user_time( event_time );

        if (hwnd == NtUserGetDesktopWindow())
        {
            /* The desktop window does not have a close button that we can
             * pretend to click. Therefore, we simply send it a close command. */
            send_message( hwnd, WM_SYSCOMMAND, SC_CLOSE, 0 );
            return;
        }

        /* Ignore the delete window request if the window has been disabled
         * and we are in managed mode. This is to disallow applications from
         * being closed by the window manager while in a modal state.
         */
        if (NtUserIsWindowEnabled( hwnd ))
        {
            HMENU hSysMenu;

            if (NtUserGetClassLongW( hwnd, GCL_STYLE ) & CS_NOCLOSE) return;
            hSysMenu = NtUserGetSystemMenu( hwnd, FALSE );
            if (hSysMenu)
            {
                UINT state = NtUserThunkedMenuItemInfo( hSysMenu, SC_CLOSE, MF_BYCOMMAND,
                                                        NtUserGetMenuState, NULL, NULL );
                if (state == 0xFFFFFFFF || (state & (MF_DISABLED | MF_GRAYED)))
                    return;
            }
            if (get_active_window() != hwnd)
            {
                LRESULT ma = send_message( hwnd, WM_MOUSEACTIVATE,
                                           (WPARAM)NtUserGetAncestor( hwnd, GA_ROOT ),
                                           MAKELPARAM( HTCLOSE, WM_NCLBUTTONDOWN ) );
                switch(ma)
                {
                    case MA_NOACTIVATEANDEAT:
                    case MA_ACTIVATEANDEAT:
                        return;
                    case MA_NOACTIVATE:
                        break;
                    case MA_ACTIVATE:
                    case 0:
                        NtUserSetActiveWindow( hwnd );
                        break;
                    default:
                        WARN( "unknown WM_MOUSEACTIVATE code %d\n", (int) ma );
                        break;
                }
            }

            NtUserPostMessage( hwnd, WM_SYSCOMMAND, SC_CLOSE, 0 );
        }
    }
    else if (protocol == x11drv_atom(WM_TAKE_FOCUS))
    {
        HWND last_focus = x11drv_thread_data()->last_focus, foreground = NtUserGetForegroundWindow();

        if (window_has_pending_wm_state( hwnd, -1 ))
        {
            WARN( "Ignoring window %p/%lx WM_TAKE_FOCUS serial %lu, event_time %ld, foreground %p during WM_STATE change\n",
                  hwnd, event->window, event->serial, event_time, foreground );
            return;
        }

        TRACE( "window %p/%lx WM_TAKE_FOCUS serial %lu, event_time %ld, foreground %p\n", hwnd, event->window,
               event->serial, event_time, foreground );
        TRACE( "  enabled %u, visible %u, style %#x, focus %p, active %p, last %p\n",
                NtUserIsWindowEnabled( hwnd ), NtUserIsWindowVisible( hwnd ), (int)NtUserGetWindowLongW( hwnd, GWL_STYLE ),
                get_focus(), get_active_window(), last_focus );

        if (can_activate_window(hwnd))
        {
            /* simulate a mouse click on the menu to find out
             * whether the window wants to be activated */
            LRESULT ma = send_message( hwnd, WM_MOUSEACTIVATE,
                                       (WPARAM)NtUserGetAncestor( hwnd, GA_ROOT ),
                                       MAKELONG( HTMENU, WM_LBUTTONDOWN ) );
            if (ma != MA_NOACTIVATEANDEAT && ma != MA_NOACTIVATE)
            {
                set_focus( event->display, hwnd, event_time );
                return;
            }
        }
        else if (hwnd == NtUserGetDesktopWindow())
        {
            hwnd = foreground;
            if (!hwnd) hwnd = last_focus;
            if (!hwnd) hwnd = NtUserGetDesktopWindow();
            set_focus( event->display, hwnd, event_time );
            return;
        }
        /* try to find some other window to give the focus to */
        hwnd = get_focus();
        if (hwnd) hwnd = NtUserGetAncestor( hwnd, GA_ROOT );
        if (!hwnd) hwnd = get_active_window();
        if (!hwnd) hwnd = last_focus;
        if (hwnd && can_activate_window(hwnd)) set_focus( event->display, hwnd, event_time );
    }
    else if (protocol == x11drv_atom(_NET_WM_PING))
    {
      XClientMessageEvent xev;
      xev = *event;
      
      TRACE("NET_WM Ping\n");
      xev.window = DefaultRootWindow(xev.display);
      XSendEvent(xev.display, xev.window, False, SubstructureRedirectMask | SubstructureNotifyMask, (XEvent*)&xev);
    }
}


static const char * const focus_details[] =
{
    "NotifyAncestor",
    "NotifyVirtual",
    "NotifyInferior",
    "NotifyNonlinear",
    "NotifyNonlinearVirtual",
    "NotifyPointer",
    "NotifyPointerRoot",
    "NotifyDetailNone"
};

static const char * const focus_modes[] =
{
    "NotifyNormal",
    "NotifyGrab",
    "NotifyUngrab",
    "NotifyWhileGrabbed"
};

BOOL is_current_process_focused(void)
{
    Display *display = x11drv_thread_data()->display;
    Window focus;
    int revert;
    HWND hwnd;

    XGetInputFocus( display, &focus, &revert );
    if (focus && !XFindContext( display, focus, winContext, (char **)&hwnd )) return TRUE;
    return FALSE;
}

/**********************************************************************
 *              X11DRV_FocusIn
 */
static BOOL X11DRV_FocusIn( HWND hwnd, XEvent *xev )
{
    HWND foreground = NtUserGetForegroundWindow();
    XFocusChangeEvent *event = &xev->xfocus;
    BOOL was_grabbed;

    if (event->detail == NotifyPointer) return FALSE;
    if (!hwnd) return FALSE;

    if (window_has_pending_wm_state( hwnd, -1 ))
    {
        WARN( "Ignoring window %p/%lx FocusIn serial %lu, detail %s, mode %s, foreground %p during WM_STATE change\n",
              hwnd, event->window, event->serial, focus_details[event->detail], focus_modes[event->mode], foreground );
        return FALSE;
    }

    TRACE( "window %p/%lx FocusIn serial %lu, detail %s, mode %s, foreground %p\n", hwnd, event->window,
           event->serial, focus_details[event->detail], focus_modes[event->mode], foreground );

    /* when focusing in the virtual desktop window, re-apply the cursor clipping rect */
    if (is_virtual_desktop() && hwnd == NtUserGetDesktopWindow()) reapply_cursor_clipping();
    if (hwnd == NtUserGetDesktopWindow()) return FALSE;

    x11drv_thread_data()->keymapnotify_hwnd = hwnd;

    /* when keyboard grab is released, re-apply the cursor clipping rect */
    was_grabbed = keyboard_grabbed;
    keyboard_grabbed = event->mode == NotifyGrab || event->mode == NotifyWhileGrabbed;
    if (was_grabbed > keyboard_grabbed) reapply_cursor_clipping();
    /* ignore wm specific NotifyUngrab / NotifyGrab events w.r.t focus */
    if (event->mode == NotifyGrab || event->mode == NotifyUngrab) return FALSE;

    xim_set_focus( hwnd, TRUE );

    if (use_take_focus) return TRUE;

    if (!can_activate_window(hwnd))
    {
        HWND hwnd = get_focus();
        if (hwnd) hwnd = NtUserGetAncestor( hwnd, GA_ROOT );
        if (!hwnd) hwnd = get_active_window();
        if (!hwnd) hwnd = x11drv_thread_data()->last_focus;
        if (hwnd && can_activate_window(hwnd)) set_focus( event->display, hwnd, CurrentTime );
    }
    else NtUserSetForegroundWindow( hwnd );
    return TRUE;
}

/**********************************************************************
 *              focus_out
 */
static void focus_out( Display *display , HWND hwnd )
 {
    if (xim_in_compose_mode()) return;

    x11drv_thread_data()->last_focus = hwnd;
    xim_set_focus( hwnd, FALSE );

    if (is_virtual_desktop()) return;
    if (hwnd != NtUserGetForegroundWindow()) return;
    if (!(NtUserGetWindowLongW( hwnd, GWL_STYLE ) & WS_MINIMIZE))
        send_message( hwnd, WM_CANCELMODE, 0, 0 );

    /* don't reset the foreground window, if the window which is
       getting the focus is a Wine window */

    if (!is_current_process_focused())
    {
        /* Abey : 6-Oct-99. Check again if the focus out window is the
           Foreground window, because in most cases the messages sent
           above must have already changed the foreground window, in which
           case we don't have to change the foreground window to 0 */
        if (hwnd == NtUserGetForegroundWindow())
        {
            TRACE( "lost focus, setting fg to desktop\n" );
            NtUserSetForegroundWindow( NtUserGetDesktopWindow() );
        }
    }
 }

/**********************************************************************
 *              X11DRV_FocusOut
 *
 * Note: only top-level windows get FocusOut events.
 */
static BOOL X11DRV_FocusOut( HWND hwnd, XEvent *xev )
{
    HWND foreground = NtUserGetForegroundWindow();
    XFocusChangeEvent *event = &xev->xfocus;

    if (event->detail == NotifyPointer)
    {
        if (!hwnd && event->window == x11drv_thread_data()->clip_window)
        {
            NtUserClipCursor( NULL );
            /* NtUserClipCursor will ask the foreground window to ungrab the cursor, but
             * it might not be responsive, so unmap the clipping window ourselves too */
            XUnmapWindow( event->display, event->window );
        }
        return TRUE;
    }
    if (!hwnd) return FALSE;

    if (window_has_pending_wm_state( hwnd, NormalState )) /* ignore FocusOut only if the window is being shown */
    {
        WARN( "Ignoring window %p/%lx FocusOut serial %lu, detail %s, mode %s, foreground %p during WM_STATE change\n",
              hwnd, event->window, event->serial, focus_details[event->detail], focus_modes[event->mode], foreground );
        return FALSE;
    }

    TRACE( "window %p/%lx FocusOut serial %lu, detail %s, mode %s, foreground %p\n", hwnd, event->window,
           event->serial, focus_details[event->detail], focus_modes[event->mode], foreground );

    /* in virtual desktop mode or when keyboard is grabbed, release any cursor grab but keep the clipping rect */
    keyboard_grabbed = event->mode == NotifyGrab || event->mode == NotifyWhileGrabbed;
    if (is_virtual_desktop() || keyboard_grabbed) ungrab_clipping_window();
    /* ignore wm specific NotifyUngrab / NotifyGrab events w.r.t focus */
    if (event->mode == NotifyGrab || event->mode == NotifyUngrab) return FALSE;

    focus_out( event->display, hwnd );
    return TRUE;
}


/***********************************************************************
 *           X11DRV_Expose
 */
static BOOL X11DRV_Expose( HWND hwnd, XEvent *xev )
{
    XExposeEvent *event = &xev->xexpose;
    RECT rect, abs_rect;
    POINT pos;
    struct x11drv_win_data *data;
    UINT flags = RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN;

    TRACE( "win %p (%lx) %d,%d %dx%d\n",
           hwnd, event->window, event->x, event->y, event->width, event->height );

    if (event->window != root_window)
    {
        pos.x = event->x;
        pos.y = event->y;
    }
    else pos = root_to_virtual_screen( event->x, event->y );

    if (!(data = get_win_data( hwnd ))) return FALSE;

    rect.left   = pos.x;
    rect.top    = pos.y;
    rect.right  = pos.x + event->width;
    rect.bottom = pos.y + event->height;

    if (event->window != data->client_window)
        OffsetRect( &rect, data->rects.visible.left - data->rects.client.left,
                    data->rects.visible.top - data->rects.client.top );

    if (event->window != root_window)
    {
        abs_rect = rect;
        OffsetRect( &abs_rect, data->rects.client.left, data->rects.client.top );

        SERVER_START_REQ( update_window_zorder )
        {
            req->window      = wine_server_user_handle( hwnd );
            req->rect        = wine_server_rectangle( abs_rect );
            wine_server_call( req );
        }
        SERVER_END_REQ;
    }
    else flags &= ~RDW_ALLCHILDREN;

    release_win_data( data );

    NtUserExposeWindowSurface( hwnd, flags, &rect, NtUserGetWinMonitorDpi( hwnd, MDT_RAW_DPI ) );
    return TRUE;
}


/**********************************************************************
 *		X11DRV_MapNotify
 */
static BOOL X11DRV_MapNotify( HWND hwnd, XEvent *event )
{
    struct x11drv_win_data *data;

    if (event->xany.window == x11drv_thread_data()->clip_window) return TRUE;

    if (!(data = get_win_data( hwnd ))) return FALSE;

    if (!data->managed && !data->embedded && data->desired_state.wm_state != WithdrawnState)
    {
        HWND hwndFocus = get_focus();
        if (hwndFocus && NtUserIsChild( hwnd, hwndFocus ))
            set_input_focus( data );
    }
    release_win_data( data );
    return TRUE;
}


/**********************************************************************
 *		X11DRV_UnmapNotify
 */
static BOOL X11DRV_UnmapNotify( HWND hwnd, XEvent *event )
{
    return TRUE;
}


/***********************************************************************
 *           X11DRV_ReparentNotify
 */
static BOOL X11DRV_ReparentNotify( HWND hwnd, XEvent *xev )
{
    XReparentEvent *event = &xev->xreparent;
    struct x11drv_win_data *data;

    if (!(data = get_win_data( hwnd ))) return FALSE;
    set_window_parent( data, event->parent );

    if (!data->embedded)
    {
        release_win_data( data );
        return FALSE;
    }

    if (data->whole_window)
    {
        if (event->parent == root_window)
        {
            TRACE( "%p/%lx reparented to root\n", hwnd, data->whole_window );
            data->embedder = 0;
            release_win_data( data );
            send_message( hwnd, WM_CLOSE, 0, 0 );
            return TRUE;
        }
        data->embedder = event->parent;
    }

    TRACE( "%p/%lx reparented to %lx\n", hwnd, data->whole_window, event->parent );
    release_win_data( data );
    return TRUE;
}

/***********************************************************************
 *		X11DRV_ConfigureNotify
 */
static BOOL X11DRV_ConfigureNotify( HWND hwnd, XEvent *xev )
{
    XConfigureEvent *event = &xev->xconfigure;
    SIZE size = {event->width, event->height};
    struct x11drv_win_data *data;
    RECT rect;
    POINT pos = {event->x, event->y};

    if (!hwnd) return FALSE;
    if (!(data = get_win_data( hwnd ))) return FALSE;

    /* update our view of the window tree for mouse event coordinate mapping */
    if (data->whole_window && data->parent && !data->parent_invalid)
    {
        SetRect( &rect, event->x, event->y, event->x + event->width, event->y + event->height );
        host_window_configure_child( data->parent, data->whole_window, rect, event->send_event );
    }

    /* synthetic events are already in absolute coordinates */
    if (!event->send_event) pos = host_window_map_point( data->parent, event->x, event->y );
    else if (is_virtual_desktop()) FIXME( "synthetic event mapping not implemented\n" );

    pos = root_to_virtual_screen( pos.x, pos.y );
    if (size.cx == 1 && size.cy == 1 && IsRectEmpty( &data->rects.window )) size.cx = size.cy = 0;
    SetRect( &rect, pos.x, pos.y, pos.x + size.cx, pos.y + size.cy );
    window_configure_notify( data, event->serial, &rect );

    release_win_data( data );

    return NtUserPostMessage( hwnd, WM_WINE_WINDOW_STATE_CHANGED, 0, 0 );
}

/***********************************************************************
 *      X11DRV_GravityNotify
 */
static BOOL X11DRV_GravityNotify( HWND hwnd, XEvent *xev )
{
    XGravityEvent *event = &xev->xgravity;
    struct x11drv_win_data *data;
    RECT rect;
    POINT pos = {event->x, event->y};

    if (!hwnd) return FALSE;
    if (!(data = get_win_data( hwnd ))) return FALSE;
    rect = data->rects.window;

    /* update our view of the window tree for mouse event coordinate mapping */
    if (data->whole_window && data->parent && !data->parent_invalid)
    {
        OffsetRect( &rect, event->x - rect.left, event->y - rect.top );
        host_window_configure_child( data->parent, data->whole_window, rect, event->send_event );
    }

    /* only handle GravityNotify events, that we generate ourselves, for embedded windows,
     * top-level windows will receive the WM synthetic ConfigureNotify events instead.
     */
    if (data->embedded)
    {
        /* synthetic events are already in absolute coordinates */
        if (!event->send_event) pos = host_window_map_point( data->parent, event->x, event->y );
        else if (is_virtual_desktop()) FIXME( "synthetic event mapping not implemented\n" );

        pos = root_to_virtual_screen( pos.x, pos.y );
        OffsetRect( &rect, pos.x - rect.left, pos.y - rect.top );
        window_configure_notify( data, event->serial, &rect );
    }

    release_win_data( data );

    return NtUserPostMessage( hwnd, WM_WINE_WINDOW_STATE_CHANGED, 0, 0 );
}

/***********************************************************************
 *           get_window_wm_state
 */
static int get_window_wm_state( Display *display, Window window )
{
    struct
    {
        CARD32 state;
        XID     icon;
    } *state;
    Atom type;
    int format, ret = -1;
    unsigned long count, remaining;

    if (!XGetWindowProperty( display, window, x11drv_atom(WM_STATE), 0,
                             sizeof(*state)/sizeof(CARD32), False, x11drv_atom(WM_STATE),
                             &type, &format, &count, &remaining, (unsigned char **)&state ))
    {
        if (type == x11drv_atom(WM_STATE) && get_property_size( format, count ) >= sizeof(*state))
            ret = state->state;
        XFree( state );
    }
    return ret;
}

/***********************************************************************
 *           get_window_xembed_info
 */
static int get_window_xembed_info( Display *display, Window window )
{
    struct
    {
        unsigned long version;
        unsigned long flags;
    } *state;
    Atom type;
    int format, ret = -1;
    unsigned long count, remaining;

    if (!XGetWindowProperty( display, window, x11drv_atom(_XEMBED_INFO), 0, 65535, False, x11drv_atom(_XEMBED_INFO),
                             &type, &format, &count, &remaining, (unsigned char **)&state ))
    {
        if (type == x11drv_atom(_XEMBED_INFO) && get_property_size( format, count ) >= sizeof(*state))
            ret = state->flags;
        XFree( state );
    }

    return ret;
}


/***********************************************************************
 *           handle_wm_state_notify
 *
 * Handle a PropertyNotify for WM_STATE.
 */
static void handle_wm_state_notify( HWND hwnd, XPropertyEvent *event )
{
    struct x11drv_win_data *data;
    UINT value = 0;

    if (!(data = get_win_data( hwnd ))) return;
    if (event->state == PropertyNewValue) value = get_window_wm_state( event->display, event->window );
    window_wm_state_notify( data, event->serial, value );
    release_win_data( data );

    NtUserPostMessage( hwnd, WM_WINE_WINDOW_STATE_CHANGED, 0, 0 );
}

static void handle_xembed_info_notify( HWND hwnd, XPropertyEvent *event )
{
    struct x11drv_win_data *data;
    UINT value = 0;

    if (!(data = get_win_data( hwnd ))) return;
    if (event->state == PropertyNewValue) value = get_window_xembed_info( event->display, event->window );
    window_wm_state_notify( data, event->serial, value ? NormalState : WithdrawnState );
    release_win_data( data );
}

static void handle_net_wm_state_notify( HWND hwnd, XPropertyEvent *event )
{
    struct x11drv_win_data *data;
    UINT value = 0;

    if (!(data = get_win_data( hwnd ))) return;
    if (event->state == PropertyNewValue) value = get_window_net_wm_state( event->display, event->window );
    window_net_wm_state_notify( data, event->serial, value );
    release_win_data( data );

    NtUserPostMessage( hwnd, WM_WINE_WINDOW_STATE_CHANGED, 0, 0 );
}

static void handle_net_supported_notify( XPropertyEvent *event )
{
    struct x11drv_thread_data *data = x11drv_thread_data();

    if (data->net_supported)
    {
        data->net_supported_count = 0;
        XFree( data->net_supported );
        data->net_supported = NULL;
        data->net_wm_state_mask = 0;
    }

    if (event->state == PropertyNewValue) net_supported_init( data );
}

static void handle_net_active_window( XPropertyEvent *event )
{
    Window window = 0;

    if (event->state == PropertyNewValue) window = get_net_active_window( event->display );
    net_active_window_notify( event->serial, window, event->time );
}

/***********************************************************************
 *           X11DRV_PropertyNotify
 */
static BOOL X11DRV_PropertyNotify( HWND hwnd, XEvent *xev )
{
    XPropertyEvent *event = &xev->xproperty;

    if (!hwnd) return FALSE;
    if (event->atom == x11drv_atom(WM_STATE)) handle_wm_state_notify( hwnd, event );
    if (event->atom == x11drv_atom(_XEMBED_INFO)) handle_xembed_info_notify( hwnd, event );
    if (event->atom == x11drv_atom(_NET_WM_STATE)) handle_net_wm_state_notify( hwnd, event );
    if (event->atom == x11drv_atom(_NET_SUPPORTED)) handle_net_supported_notify( event );
    if (event->atom == x11drv_atom(_NET_ACTIVE_WINDOW)) handle_net_active_window( event );

    return TRUE;
}


/*****************************************************************
 *		SetFocus   (X11DRV.@)
 *
 * Set the X focus.
 */
void X11DRV_SetFocus( HWND hwnd )
{
    struct x11drv_win_data *data;

    HWND parent;

    for (;;)
    {
        if (!(data = get_win_data( hwnd ))) return;
        if (data->embedded) break;
        parent = NtUserGetAncestor( hwnd, GA_PARENT );
        if (!parent || parent == NtUserGetDesktopWindow()) break;
        release_win_data( data );
        hwnd = parent;
    }
    if (!data->managed || data->embedder) set_input_focus( data );
    release_win_data( data );
}

static void drag_drop_enter( UINT entries_size, struct format_entry *entries )
{
    NtUserMessageCall( 0, WINE_DRAG_DROP_ENTER, entries_size, (LPARAM)entries, NULL,
                       NtUserDragDropCall, FALSE );
}

static void drag_drop_leave(void)
{
    NtUserMessageCall( 0, WINE_DRAG_DROP_LEAVE, 0, 0, NULL,
                       NtUserDragDropCall, FALSE );
}

static DWORD drag_drop_drag( HWND hwnd, POINT point, DWORD effect )
{
    return NtUserMessageCall( hwnd, WINE_DRAG_DROP_DRAG, MAKELONG(point.x, point.y), effect, NULL,
                              NtUserDragDropCall, FALSE );
}

static DWORD drag_drop_drop( HWND hwnd )
{
    return NtUserMessageCall( hwnd, WINE_DRAG_DROP_DROP, 0, 0, NULL,
                              NtUserDragDropCall, FALSE );
}

static void drag_drop_post( HWND hwnd, DROPFILES *drop, ULONG size )
{
    NtUserMessageCall( hwnd, WINE_DRAG_DROP_POST, size, (LPARAM)drop, NULL,
                       NtUserDragDropCall, FALSE );
}

static void drop_dnd_files( HWND hWnd, POINT pos, unsigned char *data, size_t size )
{
    size_t drop_size;
    DROPFILES *drop;

    if ((drop = file_list_to_drop_files( data, size, &drop_size )))
    {
        drop->pt = pos;
        drag_drop_post( hWnd, drop, drop_size );
        free( drop );
    }
}

static void drop_dnd_urls( HWND hWnd, POINT pos, unsigned char *data, size_t size )
{
    size_t drop_size;
    DROPFILES *drop;

    if ((drop = uri_list_to_drop_files( data, size, &drop_size )))
    {
        drop->pt = pos;
        drag_drop_post( hWnd, drop, drop_size );
        free( drop );
    }
}


/**********************************************************************
 *              handle_xembed_protocol
 */
static void handle_xembed_protocol( HWND hwnd, XClientMessageEvent *event )
{
    switch (event->data.l[1])
    {
    case XEMBED_EMBEDDED_NOTIFY:
        {
            struct x11drv_win_data *data = get_win_data( hwnd );
            if (!data) break;

            TRACE( "win %p/%lx XEMBED_EMBEDDED_NOTIFY owner %lx\n", hwnd, event->window, event->data.l[3] );
            data->embedder = event->data.l[3];

            /* window has been marked as embedded before (e.g. systray) */
            if (data->embedded || !data->embedder /* broken QX11EmbedContainer implementation */)
            {
                release_win_data( data );
                break;
            }

            set_window_parent( data, data->embedder );
            make_window_embedded( data );
            release_win_data( data );
        }
        break;

    case XEMBED_WINDOW_DEACTIVATE:
        TRACE( "win %p/%lx XEMBED_WINDOW_DEACTIVATE message\n", hwnd, event->window );
        focus_out( event->display, NtUserGetAncestor( hwnd, GA_ROOT ) );
        break;

    case XEMBED_FOCUS_OUT:
        TRACE( "win %p/%lx XEMBED_FOCUS_OUT message\n", hwnd, event->window );
        focus_out( event->display, NtUserGetAncestor( hwnd, GA_ROOT ) );
        break;

    case XEMBED_MODALITY_ON:
        TRACE( "win %p/%lx XEMBED_MODALITY_ON message\n", hwnd, event->window );
        NtUserEnableWindow( hwnd, FALSE );
        break;

    case XEMBED_MODALITY_OFF:
        TRACE( "win %p/%lx XEMBED_MODALITY_OFF message\n", hwnd, event->window );
        NtUserEnableWindow( hwnd, TRUE );
        break;

    default:
        TRACE( "win %p/%lx XEMBED message %lu(%lu)\n",
               hwnd, event->window, event->data.l[1], event->data.l[2] );
        break;
    }
}


/**********************************************************************
 *              handle_dnd_protocol
 */
static void handle_dnd_protocol( HWND hwnd, XClientMessageEvent *event )
{
    unsigned long count, remaining;
    Window root, child;
    int root_x, root_y, format;
    unsigned char *data;
    unsigned int mask;
    size_t size;
    POINT pos;
    Atom type;

    /* query window (drag&drop event contains only drag window) */
    XQueryPointer( event->display, root_window, &root, &child, &root_x, &root_y,
                   (int *)&pos.x, (int *)&pos.y, &mask );
    pos = root_to_virtual_screen( pos.x, pos.y );

    if (XFindContext( event->display, child, winContext, (char **)&hwnd ) != 0) hwnd = 0;
    if (!hwnd) return;

    if (XGetWindowProperty( event->display, DefaultRootWindow(event->display), x11drv_atom(DndSelection), 0, 65535,
                            False, AnyPropertyType, &type, &format, &count, &remaining, &data ) || !data)
        return;

    if (!remaining /* don't bother if > 64K */ && (size = get_property_size( format, count )))
    {
        if (event->data.l[0] == DndFile || event->data.l[0] == DndFiles)
            drop_dnd_files( hwnd, pos, data, size );
        else if (event->data.l[0] == DndURL)
            drop_dnd_urls( hwnd, pos, data, size );
    }

    XFree( data );
}


/**************************************************************************
 *           handle_xdnd_enter_event
 *
 * Handle an XdndEnter event.
 */
static void handle_xdnd_enter_event( HWND hWnd, XClientMessageEvent *event )
{
    struct format_entry *data;
    unsigned long count = 0;
    Atom *xdndtypes;
    size_t size;
    int version;

    version = (event->data.l[1] & 0xFF000000) >> 24;

    TRACE( "ver(%d) check-XdndTypeList(%ld) data=%ld,%ld,%ld,%ld,%ld\n",
           version, (event->data.l[1] & 1),
           event->data.l[0], event->data.l[1], event->data.l[2],
           event->data.l[3], event->data.l[4] );

    if (version > WINE_XDND_VERSION)
    {
        ERR("ignoring unsupported XDND version %d\n", version);
        return;
    }

    /* If the source supports more than 3 data types we retrieve
     * the entire list. */
    if (event->data.l[1] & 1)
    {
        Atom acttype;
        int actfmt;
        unsigned long bytesret;

        /* Request supported formats from source window */
        XGetWindowProperty( event->display, event->data.l[0], x11drv_atom(XdndTypeList),
                            0, 65535, FALSE, AnyPropertyType, &acttype, &actfmt, &count,
                            &bytesret, (unsigned char **)&xdndtypes );
    }
    else
    {
        count = 3;
        xdndtypes = (Atom *)&event->data.l[2];
    }

    if (TRACE_ON(xdnd))
    {
        unsigned int i;

        for (i = 0; i < count; i++)
        {
            if (xdndtypes[i] != 0)
            {
                char * pn = XGetAtomName( event->display, xdndtypes[i] );
                TRACE( "XDNDEnterAtom %ld: %s\n", xdndtypes[i], pn );
                XFree( pn );
            }
        }
    }

    data = import_xdnd_selection( event->display, event->window, x11drv_atom(XdndSelection),
                                  xdndtypes, count, &size );
    if (data) drag_drop_enter( size, data );
    free( data );

    if (event->data.l[1] & 1)
        XFree(xdndtypes);
}


static DWORD xdnd_action_to_drop_effect( long action )
{
    /* In Windows, nothing but the given effects is allowed.
     * In X the given action is just a hint, and you can always
     * XdndActionCopy and XdndActionPrivate, so be more permissive. */
    if (action == x11drv_atom(XdndActionCopy))
        return DROPEFFECT_COPY;
    else if (action == x11drv_atom(XdndActionMove))
        return DROPEFFECT_MOVE | DROPEFFECT_COPY;
    else if (action == x11drv_atom(XdndActionLink))
        return DROPEFFECT_LINK | DROPEFFECT_COPY;
    else if (action == x11drv_atom(XdndActionAsk))
        /* FIXME: should we somehow ask the user what to do here? */
        return DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK;

    FIXME( "unknown action %ld, assuming DROPEFFECT_COPY\n", action );
    return DROPEFFECT_COPY;
}


static long drop_effect_to_xdnd_action( UINT effect )
{
    if (effect == DROPEFFECT_COPY)
        return x11drv_atom(XdndActionCopy);
    else if (effect == DROPEFFECT_MOVE)
        return x11drv_atom(XdndActionMove);
    else if (effect == DROPEFFECT_LINK)
        return x11drv_atom(XdndActionLink);
    else if (effect == DROPEFFECT_NONE)
        return None;

    FIXME( "unknown drop effect %u, assuming XdndActionCopy\n", effect );
    return x11drv_atom(XdndActionCopy);
}


static void handle_xdnd_position_event( HWND hwnd, XClientMessageEvent *event )
{
    XClientMessageEvent e;
    POINT point;
    UINT effect;

    point = root_to_virtual_screen( event->data.l[2] >> 16, event->data.l[2] & 0xFFFF );
    effect = xdnd_action_to_drop_effect( event->data.l[4] );
    effect = drag_drop_drag( hwnd, point, effect );

    TRACE( "actionRequested(%ld) chosen(0x%x) at x(%d),y(%d)\n",
           event->data.l[4], effect, (int)point.x, (int)point.y );

    /*
     * Let source know if we're accepting the drop by
     * sending a status message.
     */
    e.type = ClientMessage;
    e.display = event->display;
    e.window = event->data.l[0];
    e.message_type = x11drv_atom(XdndStatus);
    e.format = 32;
    e.data.l[0] = event->window;
    e.data.l[1] = !!effect;
    e.data.l[2] = 0; /* Empty Rect */
    e.data.l[3] = 0; /* Empty Rect */
    e.data.l[4] = drop_effect_to_xdnd_action( effect );
    XSendEvent( event->display, event->data.l[0], False, NoEventMask, (XEvent *)&e );
}


static void handle_xdnd_drop_event( HWND hwnd, XClientMessageEvent *event )
{
    XClientMessageEvent e;
    UINT effect;

    effect = drag_drop_drop( hwnd );

    /* Tell the target we are finished. */
    memset( &e, 0, sizeof(e) );
    e.type = ClientMessage;
    e.display = event->display;
    e.window = event->data.l[0];
    e.message_type = x11drv_atom(XdndFinished);
    e.format = 32;
    e.data.l[0] = event->window;
    e.data.l[1] = !!effect;
    e.data.l[2] = drop_effect_to_xdnd_action( effect );
    XSendEvent( event->display, event->data.l[0], False, NoEventMask, (XEvent *)&e );
}


static void handle_xdnd_leave_event( HWND hwnd, XClientMessageEvent *event )
{
    drag_drop_leave();
}


struct client_message_handler
{
    int    atom;                                  /* protocol atom */
    void (*handler)(HWND, XClientMessageEvent *); /* corresponding handler function */
};

static const struct client_message_handler client_messages[] =
{
    { XATOM_MANAGER,      handle_manager_message },
    { XATOM_WM_PROTOCOLS, handle_wm_protocols },
    { XATOM__XEMBED,      handle_xembed_protocol },
    { XATOM_DndProtocol,  handle_dnd_protocol },
    { XATOM_XdndEnter,    handle_xdnd_enter_event },
    { XATOM_XdndPosition, handle_xdnd_position_event },
    { XATOM_XdndDrop,     handle_xdnd_drop_event },
    { XATOM_XdndLeave,    handle_xdnd_leave_event }
};


/**********************************************************************
 *           X11DRV_ClientMessage
 */
static BOOL X11DRV_ClientMessage( HWND hwnd, XEvent *xev )
{
    XClientMessageEvent *event = &xev->xclient;
    unsigned int i;

    if (!hwnd) return FALSE;

    if (event->format != 32)
    {
        WARN( "Don't know how to handle format %d\n", event->format );
        return FALSE;
    }

    for (i = 0; i < ARRAY_SIZE( client_messages ); i++)
    {
        if (event->message_type == X11DRV_Atoms[client_messages[i].atom - FIRST_XATOM])
        {
            client_messages[i].handler( hwnd, event );
            return TRUE;
        }
    }
    TRACE( "no handler found for %ld\n", event->message_type );
    return FALSE;
}
