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

#include "config.h"

#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "shlobj.h"  /* DROPFILES */

#include "win.h"
#include "x11drv.h"
#include "shellapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(event);

extern BOOL ximInComposeMode;

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

  /* Event handlers */
static void EVENT_FocusIn( HWND hwnd, XEvent *event );
static void EVENT_FocusOut( HWND hwnd, XEvent *event );
static void EVENT_PropertyNotify( HWND hwnd, XEvent *event );
static void EVENT_ClientMessage( HWND hwnd, XEvent *event );

struct event_handler
{
    int                  type;    /* event type */
    x11drv_event_handler handler; /* corresponding handler function */
};

#define MAX_EVENT_HANDLERS 64

static struct event_handler handlers[MAX_EVENT_HANDLERS] =
{
    /* list must be sorted by event type */
    { KeyPress,         X11DRV_KeyEvent },
    { KeyRelease,       X11DRV_KeyEvent },
    { ButtonPress,      X11DRV_ButtonPress },
    { ButtonRelease,    X11DRV_ButtonRelease },
    { MotionNotify,     X11DRV_MotionNotify },
    { EnterNotify,      X11DRV_EnterNotify },
    /* LeaveNotify */
    { FocusIn,          EVENT_FocusIn },
    { FocusOut,         EVENT_FocusOut },
    { KeymapNotify,     X11DRV_KeymapNotify },
    { Expose,           X11DRV_Expose },
    /* GraphicsExpose */
    /* NoExpose */
    /* VisibilityNotify */
    /* CreateNotify */
    /* DestroyNotify */
    { UnmapNotify,      X11DRV_UnmapNotify },
    { MapNotify,        X11DRV_MapNotify },
    /* MapRequest */
    /* ReparentNotify */
    { ConfigureNotify,  X11DRV_ConfigureNotify },
    /* ConfigureRequest */
    /* GravityNotify */
    /* ResizeRequest */
    /* CirculateNotify */
    /* CirculateRequest */
    { PropertyNotify,   EVENT_PropertyNotify },
    { SelectionClear,   X11DRV_SelectionClear },
    { SelectionRequest, X11DRV_SelectionRequest },
    /* SelectionNotify */
    /* ColormapNotify */
    { ClientMessage,    EVENT_ClientMessage },
    { MappingNotify,    X11DRV_MappingNotify },
};

static int nb_event_handlers = 18;  /* change this if you add handlers above */


/* return the name of an X event */
static const char *dbgstr_event( int type )
{
    static const char * const event_names[] =
    {
        "KeyPress", "KeyRelease", "ButtonPress", "ButtonRelease",
        "MotionNotify", "EnterNotify", "LeaveNotify", "FocusIn", "FocusOut",
        "KeymapNotify", "Expose", "GraphicsExpose", "NoExpose", "VisibilityNotify",
        "CreateNotify", "DestroyNotify", "UnmapNotify", "MapNotify", "MapRequest",
        "ReparentNotify", "ConfigureNotify", "ConfigureRequest", "GravityNotify",
        "ResizeRequest", "CirculateNotify", "CirculateRequest", "PropertyNotify",
        "SelectionClear", "SelectionRequest", "SelectionNotify", "ColormapNotify",
        "ClientMessage", "MappingNotify"
    };

    if (type >= KeyPress && type <= MappingNotify) return event_names[type - KeyPress];
    return wine_dbg_sprintf( "Extension event %d", type );
}


/***********************************************************************
 *           find_handler
 *
 * Find the handler for a given event type. Caller must hold the x11 lock.
 */
static inline x11drv_event_handler find_handler( int type )
{
    int min = 0, max = nb_event_handlers - 1;

    while (min <= max)
    {
        int pos = (min + max) / 2;
        if (handlers[pos].type == type) return handlers[pos].handler;
        if (handlers[pos].type > type) max = pos - 1;
        else min = pos + 1;
    }
    return NULL;
}


/***********************************************************************
 *           X11DRV_register_event_handler
 *
 * Register a handler for a given event type.
 * If already registered, overwrite the previous handler.
 */
void X11DRV_register_event_handler( int type, x11drv_event_handler handler )
{
    int min, max;

    wine_tsx11_lock();
    min = 0;
    max = nb_event_handlers - 1;
    while (min <= max)
    {
        int pos = (min + max) / 2;
        if (handlers[pos].type == type)
        {
            handlers[pos].handler = handler;
            goto done;
        }
        if (handlers[pos].type > type) max = pos - 1;
        else min = pos + 1;
    }
    /* insert it between max and min */
    memmove( &handlers[min+1], &handlers[min], (nb_event_handlers - min) * sizeof(handlers[0]) );
    handlers[min].type = type;
    handlers[min].handler = handler;
    nb_event_handlers++;
    assert( nb_event_handlers <= MAX_EVENT_HANDLERS );
done:
    wine_tsx11_unlock();
    TRACE("registered handler %p for event %d count %d\n", handler, type, nb_event_handlers );
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
        return (mask & QS_KEY) != 0;
    case ButtonPress:
    case ButtonRelease:
        return (mask & QS_MOUSEBUTTON) != 0;
    case MotionNotify:
    case EnterNotify:
    case LeaveNotify:
        return (mask & QS_MOUSEMOVE) != 0;
    case Expose:
        return (mask & QS_PAINT) != 0;
    case FocusIn:
    case FocusOut:
    case MapNotify:
    case UnmapNotify:
    case ConfigureNotify:
    case PropertyNotify:
    case ClientMessage:
        return (mask & QS_POSTMESSAGE) != 0;
    default:
        return (mask & QS_SENDMESSAGE) != 0;
    }
}


/***********************************************************************
 *           process_events
 */
static int process_events( Display *display, ULONG_PTR mask )
{
    XEvent event;
    HWND hwnd;
    int count = 0;
    x11drv_event_handler handler;

    wine_tsx11_lock();
    while (XCheckIfEvent( display, &event, filter_event, (char *)mask ))
    {
        count++;
        if (XFilterEvent( &event, None )) continue;  /* filtered, ignore it */

        if (!(handler = find_handler( event.type )))
        {
            TRACE( "%s, ignoring\n", dbgstr_event( event.type ));
            continue;  /* no handler, ignore it */
        }

        if (XFindContext( display, event.xany.window, winContext, (char **)&hwnd ) != 0)
            hwnd = 0;  /* not for a registered window */
        if (!hwnd && event.xany.window == root_window) hwnd = GetDesktopWindow();

        wine_tsx11_unlock();
        TRACE( "%s for hwnd/window %p/%lx\n",
               dbgstr_event( event.type ), hwnd, event.xany.window );
        handler( hwnd, &event );
        wine_tsx11_lock();
    }
    XFlush( gdi_display );
    wine_tsx11_unlock();
    if (count) TRACE( "processed %d events\n", count );
    return count;
}


/***********************************************************************
 *           MsgWaitForMultipleObjectsEx   (X11DRV.@)
 */
DWORD X11DRV_MsgWaitForMultipleObjectsEx( DWORD count, const HANDLE *handles,
                                          DWORD timeout, DWORD mask, DWORD flags )
{
    DWORD ret;
    struct x11drv_thread_data *data = TlsGetValue( thread_data_tls_index );

    if (!data)
    {
        if (!count && !timeout) return WAIT_TIMEOUT;
        return WaitForMultipleObjectsEx( count, handles, flags & MWMO_WAITALL,
                                         timeout, flags & MWMO_ALERTABLE );
    }

    if (data->process_event_count) mask = 0;  /* don't process nested events */

    data->process_event_count++;

    if (process_events( data->display, mask )) ret = count - 1;
    else if (count || timeout)
    {
        ret = WaitForMultipleObjectsEx( count, handles, flags & MWMO_WAITALL,
                                        timeout, flags & MWMO_ALERTABLE );
        if (ret == count - 1) process_events( data->display, mask );
    }
    else ret = WAIT_TIMEOUT;

    data->process_event_count--;
    return ret;
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
  DWORD now = GetTickCount();
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
    LONG style = GetWindowLongW( hwnd, GWL_STYLE );
    if (!(style & WS_VISIBLE)) return FALSE;
    if ((style & (WS_POPUP|WS_CHILD)) == WS_CHILD) return FALSE;
    return !(style & WS_DISABLED);
}


/**********************************************************************
 *              set_focus
 */
static void set_focus( HWND hwnd, Time time )
{
    HWND focus;
    Window win;

    TRACE( "setting foreground window to %p\n", hwnd );
    SetForegroundWindow( hwnd );

    focus = GetFocus();
    if (focus) focus = GetAncestor( focus, GA_ROOT );
    win = X11DRV_get_whole_window(focus);

    if (win)
    {
        TRACE( "setting focus to %p (%lx) time=%ld\n", focus, win, time );
        wine_tsx11_lock();
        XSetInputFocus( thread_display(), win, RevertToParent, time );
        wine_tsx11_unlock();
    }
}


/**********************************************************************
 *              handle_wm_protocols
 */
static void handle_wm_protocols( HWND hwnd, XClientMessageEvent *event )
{
    Atom protocol = (Atom)event->data.l[0];

    if (!protocol) return;

    if (protocol == x11drv_atom(WM_DELETE_WINDOW))
    {
        /* Ignore the delete window request if the window has been disabled
         * and we are in managed mode. This is to disallow applications from
         * being closed by the window manager while in a modal state.
         */
        if (IsWindowEnabled(hwnd))
        {
            HMENU hSysMenu;

            if (GetClassLongW(hwnd, GCL_STYLE) & CS_NOCLOSE) return;
            hSysMenu = GetSystemMenu(hwnd, FALSE);
            if (hSysMenu)
            {
                UINT state = GetMenuState(hSysMenu, SC_CLOSE, MF_BYCOMMAND);
                if (state == 0xFFFFFFFF || (state & (MF_DISABLED | MF_GRAYED)))
                    return;
            }
            PostMessageW( hwnd, WM_X11DRV_DELETE_WINDOW, 0, 0 );
        }
    }
    else if (protocol == x11drv_atom(WM_TAKE_FOCUS))
    {
        Time event_time = (Time)event->data.l[1];
        HWND last_focus = x11drv_thread_data()->last_focus;

        TRACE( "got take focus msg for %p, enabled=%d, visible=%d (style %08x), focus=%p, active=%p, fg=%p, last=%p\n",
               hwnd, IsWindowEnabled(hwnd), IsWindowVisible(hwnd), GetWindowLongW(hwnd, GWL_STYLE),
               GetFocus(), GetActiveWindow(), GetForegroundWindow(), last_focus );

        if (can_activate_window(hwnd))
        {
            /* simulate a mouse click on the caption to find out
             * whether the window wants to be activated */
            LRESULT ma = SendMessageW( hwnd, WM_MOUSEACTIVATE,
                                       (WPARAM)GetAncestor( hwnd, GA_ROOT ),
                                       MAKELONG(HTCAPTION,WM_LBUTTONDOWN) );
            if (ma != MA_NOACTIVATEANDEAT && ma != MA_NOACTIVATE) set_focus( hwnd, event_time );
            else TRACE( "not setting focus to %p (%lx), ma=%ld\n", hwnd, event->window, ma );
        }
        else
        {
            hwnd = GetFocus();
            if (hwnd) hwnd = GetAncestor( hwnd, GA_ROOT );
            if (!hwnd) hwnd = GetActiveWindow();
            if (!hwnd) hwnd = last_focus;
            if (hwnd && can_activate_window(hwnd)) set_focus( hwnd, event_time );
        }
    }
    else if (protocol == x11drv_atom(_NET_WM_PING))
    {
      XClientMessageEvent xev;
      xev = *event;
      
      TRACE("NET_WM Ping\n");
      wine_tsx11_lock();
      xev.window = DefaultRootWindow(xev.display);
      XSendEvent(xev.display, xev.window, False, SubstructureRedirectMask | SubstructureNotifyMask, (XEvent*)&xev);
      wine_tsx11_unlock();
      /* this line is semi-stolen from gtk2 */
      TRACE("NET_WM Pong\n");
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

/**********************************************************************
 *              EVENT_FocusIn
 */
static void EVENT_FocusIn( HWND hwnd, XEvent *xev )
{
    XFocusChangeEvent *event = &xev->xfocus;
    XIC xic;

    if (!hwnd) return;

    TRACE( "win %p xwin %lx detail=%s\n", hwnd, event->window, focus_details[event->detail] );

    if (event->detail == NotifyPointer) return;

    if ((xic = X11DRV_get_ic( hwnd )))
    {
        wine_tsx11_lock();
        XSetICFocus( xic );
        wine_tsx11_unlock();
    }
    if (use_take_focus) return;  /* ignore FocusIn if we are using take focus */

    if (!can_activate_window(hwnd))
    {
        HWND hwnd = GetFocus();
        if (hwnd) hwnd = GetAncestor( hwnd, GA_ROOT );
        if (!hwnd) hwnd = GetActiveWindow();
        if (!hwnd) hwnd = x11drv_thread_data()->last_focus;
        if (hwnd && can_activate_window(hwnd)) set_focus( hwnd, CurrentTime );
    }
    else SetForegroundWindow( hwnd );
}


/**********************************************************************
 *              EVENT_FocusOut
 *
 * Note: only top-level windows get FocusOut events.
 */
static void EVENT_FocusOut( HWND hwnd, XEvent *xev )
{
    XFocusChangeEvent *event = &xev->xfocus;
    HWND hwnd_tmp;
    Window focus_win;
    int revert;
    XIC xic;

    if (!hwnd) return;

    TRACE( "win %p xwin %lx detail=%s\n", hwnd, event->window, focus_details[event->detail] );

    if (event->detail == NotifyPointer) return;
    if (ximInComposeMode) return;

    x11drv_thread_data()->last_focus = hwnd;
    if ((xic = X11DRV_get_ic( hwnd )))
    {
        wine_tsx11_lock();
        XUnsetICFocus( xic );
        wine_tsx11_unlock();
    }
    if (hwnd != GetForegroundWindow()) return;
    SendMessageW( hwnd, WM_CANCELMODE, 0, 0 );

    /* don't reset the foreground window, if the window which is
       getting the focus is a Wine window */

    wine_tsx11_lock();
    XGetInputFocus( thread_display(), &focus_win, &revert );
    if (focus_win)
    {
        if (XFindContext( thread_display(), focus_win, winContext, (char **)&hwnd_tmp ) != 0)
            focus_win = 0;
    }
    wine_tsx11_unlock();

    if (!focus_win)
    {
        /* Abey : 6-Oct-99. Check again if the focus out window is the
           Foreground window, because in most cases the messages sent
           above must have already changed the foreground window, in which
           case we don't have to change the foreground window to 0 */
        if (hwnd == GetForegroundWindow())
        {
            TRACE( "lost focus, setting fg to 0\n" );
            SetForegroundWindow( 0 );
        }
    }
}


/***********************************************************************
 *           EVENT_PropertyNotify
 *   We use this to release resources like Pixmaps when a selection
 *   client no longer needs them.
 */
static void EVENT_PropertyNotify( HWND hwnd, XEvent *xev )
{
  XPropertyEvent *event = &xev->xproperty;
  /* Check if we have any resources to free */
  TRACE("Received PropertyNotify event:\n");

  switch(event->state)
  {
    case PropertyDelete:
    {
      TRACE("\tPropertyDelete for atom %ld on window %ld\n",
            event->atom, (long)event->window);
      break;
    }

    case PropertyNewValue:
    {
      TRACE("\tPropertyNewValue for atom %ld on window %ld\n\n",
            event->atom, (long)event->window);
      break;
    }

    default:
      break;
  }
}

static HWND find_drop_window( HWND hQueryWnd, LPPOINT lpPt )
{
    RECT tempRect;

    if (!IsWindowEnabled(hQueryWnd)) return 0;
    
    GetWindowRect(hQueryWnd, &tempRect);

    if(!PtInRect(&tempRect, *lpPt)) return 0;

    if (!IsIconic( hQueryWnd ))
    {
        POINT pt = *lpPt;
        ScreenToClient( hQueryWnd, &pt );
        GetClientRect( hQueryWnd, &tempRect );

        if (PtInRect( &tempRect, pt))
        {
            HWND ret = ChildWindowFromPointEx( hQueryWnd, pt, CWP_SKIPINVISIBLE|CWP_SKIPDISABLED );
            if (ret && ret != hQueryWnd)
            {
                ret = find_drop_window( ret, lpPt );
                if (ret) return ret;
            }
        }
    }

    if(!(GetWindowLongA( hQueryWnd, GWL_EXSTYLE ) & WS_EX_ACCEPTFILES)) return 0;
    
    ScreenToClient(hQueryWnd, lpPt);

    return hQueryWnd;
}

/**********************************************************************
 *           EVENT_DropFromOffix
 *
 * don't know if it still works (last Changlog is from 96/11/04)
 */
static void EVENT_DropFromOffiX( HWND hWnd, XClientMessageEvent *event )
{
    unsigned long	data_length;
    unsigned long	aux_long;
    unsigned char*	p_data = NULL;
    Atom atom_aux;
    int			x, y, dummy;
    BOOL	        bAccept;
    Window		win, w_aux_root, w_aux_child;
    WND*                pWnd;
    HWND		hScope = hWnd;

    win = X11DRV_get_whole_window(hWnd);
    wine_tsx11_lock();
    XQueryPointer( event->display, win, &w_aux_root, &w_aux_child,
                   &x, &y, &dummy, &dummy, (unsigned int*)&aux_long);
    x += virtual_screen_rect.left;
    y += virtual_screen_rect.top;
    wine_tsx11_unlock();

    pWnd = WIN_GetPtr(hWnd);

    /* find out drop point and drop window */
    if( x < 0 || y < 0 ||
        x > (pWnd->rectWindow.right - pWnd->rectWindow.left) ||
        y > (pWnd->rectWindow.bottom - pWnd->rectWindow.top) )
    {   
	bAccept = pWnd->dwExStyle & WS_EX_ACCEPTFILES; 
	x = 0;
	y = 0; 
    }
    else
    {
    	POINT	pt = { x, y };
        HWND    hwndDrop = find_drop_window( hWnd, &pt );
	if (hwndDrop)
	{
	    x = pt.x;
	    y = pt.y;
	    hScope = hwndDrop;
	    bAccept = TRUE;
	}
	else
	{
	    bAccept = FALSE;
	}
    }
    WIN_ReleasePtr(pWnd);

    if (!bAccept) return;

    wine_tsx11_lock();
    XGetWindowProperty( event->display, DefaultRootWindow(event->display),
                        x11drv_atom(DndSelection), 0, 65535, FALSE,
                        AnyPropertyType, &atom_aux, &dummy,
                        &data_length, &aux_long, &p_data);
    wine_tsx11_unlock();

    if( !aux_long && p_data)  /* don't bother if > 64K */
    {
        char *p = (char *)p_data;
        char *p_drop;

        aux_long = 0;
        while( *p )  /* calculate buffer size */
        {
            INT len = GetShortPathNameA( p, NULL, 0 );
            if (len) aux_long += len + 1;
            p += strlen(p) + 1;
        }
        if( aux_long && aux_long < 65535 )
        {
            HDROP                 hDrop;
            DROPFILES *lpDrop;

            aux_long += sizeof(DROPFILES) + 1;
            hDrop = GlobalAlloc( GMEM_SHARE, aux_long );
            lpDrop = (DROPFILES*)GlobalLock( hDrop );

            if( lpDrop )
            {
                WND *pDropWnd = WIN_GetPtr( hScope );
                lpDrop->pFiles = sizeof(DROPFILES);
                lpDrop->pt.x = x;
                lpDrop->pt.y = y;
                lpDrop->fNC =
                    ( x < (pDropWnd->rectClient.left - pDropWnd->rectWindow.left)  ||
                      y < (pDropWnd->rectClient.top - pDropWnd->rectWindow.top)    ||
                      x > (pDropWnd->rectClient.right - pDropWnd->rectWindow.left) ||
                      y > (pDropWnd->rectClient.bottom - pDropWnd->rectWindow.top) );
                lpDrop->fWide = FALSE;
                WIN_ReleasePtr(pDropWnd);
                p_drop = (char *)(lpDrop + 1);
                p = (char *)p_data;
                while(*p)
                {
                    if (GetShortPathNameA( p, p_drop, aux_long - (p_drop - (char *)lpDrop) ))
                        p_drop += strlen( p_drop ) + 1;
                    p += strlen(p) + 1;
                }
                *p_drop = '\0';
                PostMessageA( hWnd, WM_DROPFILES, (WPARAM)hDrop, 0L );
            }
        }
    }
    wine_tsx11_lock();
    if( p_data ) XFree(p_data);
    wine_tsx11_unlock();
}

/**********************************************************************
 *           EVENT_DropURLs
 *
 * drop items are separated by \n
 * each item is prefixed by its mime type
 *
 * event->data.l[3], event->data.l[4] contains drop x,y position
 */
static void EVENT_DropURLs( HWND hWnd, XClientMessageEvent *event )
{
  unsigned long	data_length;
  unsigned long	aux_long, drop_len = 0;
  unsigned char	*p_data = NULL; /* property data */
  char		*p_drop = NULL;
  char          *p, *next;
  int		x, y;
  DROPFILES *lpDrop;
  HDROP hDrop;
  union {
    Atom	atom_aux;
    int         i;
    Window      w_aux;
    unsigned int u;
  }		u; /* unused */

  if (!(GetWindowLongW( hWnd, GWL_EXSTYLE ) & WS_EX_ACCEPTFILES)) return;

  wine_tsx11_lock();
  XGetWindowProperty( event->display, DefaultRootWindow(event->display),
                      x11drv_atom(DndSelection), 0, 65535, FALSE,
                      AnyPropertyType, &u.atom_aux, &u.i,
                      &data_length, &aux_long, &p_data);
  wine_tsx11_unlock();
  if (aux_long)
    WARN("property too large, truncated!\n");
  TRACE("urls=%s\n", p_data);

  if( !aux_long && p_data) {	/* don't bother if > 64K */
    /* calculate length */
    p = (char*) p_data;
    next = strchr(p, '\n');
    while (p) {
      if (next) *next=0;
      if (strncmp(p,"file:",5) == 0 ) {
	INT len = GetShortPathNameA( p+5, NULL, 0 );
	if (len) drop_len += len + 1;
      }
      if (next) {
	*next = '\n';
	p = next + 1;
	next = strchr(p, '\n');
      } else {
	p = NULL;
      }
    }

    if( drop_len && drop_len < 65535 ) {
      wine_tsx11_lock();
      XQueryPointer( event->display, root_window, &u.w_aux, &u.w_aux,
                     &x, &y, &u.i, &u.i, &u.u);
      x += virtual_screen_rect.left;
      y += virtual_screen_rect.top;
      wine_tsx11_unlock();

      drop_len += sizeof(DROPFILES) + 1;
      hDrop = GlobalAlloc( GMEM_SHARE, drop_len );
      lpDrop = (DROPFILES *) GlobalLock( hDrop );

      if( lpDrop ) {
          WND *pDropWnd = WIN_GetPtr( hWnd );
	  lpDrop->pFiles = sizeof(DROPFILES);
	  lpDrop->pt.x = (INT)x;
	  lpDrop->pt.y = (INT)y;
	  lpDrop->fNC =
	    ( x < (pDropWnd->rectClient.left - pDropWnd->rectWindow.left)  ||
	      y < (pDropWnd->rectClient.top - pDropWnd->rectWindow.top)    ||
	      x > (pDropWnd->rectClient.right - pDropWnd->rectWindow.left) ||
	      y > (pDropWnd->rectClient.bottom - pDropWnd->rectWindow.top) );
	  lpDrop->fWide = FALSE;
	  p_drop = (char*)(lpDrop + 1);
          WIN_ReleasePtr(pDropWnd);
      }

      /* create message content */
      if (p_drop) {
	p = (char*) p_data;
	next = strchr(p, '\n');
	while (p) {
	  if (next) *next=0;
	  if (strncmp(p,"file:",5) == 0 ) {
	    INT len = GetShortPathNameA( p+5, p_drop, 65535 );
	    if (len) {
	      TRACE("drop file %s as %s\n", p+5, p_drop);
	      p_drop += len+1;
	    } else {
	      WARN("can't convert file %s to dos name\n", p+5);
	    }
	  } else {
	    WARN("unknown mime type %s\n", p);
	  }
	  if (next) {
	    *next = '\n';
	    p = next + 1;
	    next = strchr(p, '\n');
	  } else {
	    p = NULL;
	  }
	  *p_drop = '\0';
	}

        GlobalUnlock(hDrop);
        PostMessageA( hWnd, WM_DROPFILES, (WPARAM)hDrop, 0L );
      }
    }
    wine_tsx11_lock();
    if( p_data ) XFree(p_data);
    wine_tsx11_unlock();
  }
}

/**********************************************************************
 *              handle_dnd_protocol
 */
static void handle_dnd_protocol( HWND hwnd, XClientMessageEvent *event )
{
    Window root, child;
    int root_x, root_y, child_x, child_y;
    unsigned int u;

    /* query window (drag&drop event contains only drag window) */
    wine_tsx11_lock();
    XQueryPointer( event->display, root_window, &root, &child,
                   &root_x, &root_y, &child_x, &child_y, &u);
    if (XFindContext( event->display, child, winContext, (char **)&hwnd ) != 0) hwnd = 0;
    wine_tsx11_unlock();
    if (!hwnd) return;
    if (event->data.l[0] == DndFile || event->data.l[0] == DndFiles)
        EVENT_DropFromOffiX(hwnd, event);
    else if (event->data.l[0] == DndURL)
        EVENT_DropURLs(hwnd, event);
}


struct client_message_handler
{
    int    atom;                                  /* protocol atom */
    void (*handler)(HWND, XClientMessageEvent *); /* corresponding handler function */
};

static const struct client_message_handler client_messages[] =
{
    { XATOM_WM_PROTOCOLS, handle_wm_protocols },
    { XATOM_DndProtocol,  handle_dnd_protocol },
    { XATOM_XdndEnter,    X11DRV_XDND_EnterEvent },
    { XATOM_XdndPosition, X11DRV_XDND_PositionEvent },
    { XATOM_XdndDrop,     X11DRV_XDND_DropEvent },
    { XATOM_XdndLeave,    X11DRV_XDND_LeaveEvent }
};


/**********************************************************************
 *           EVENT_ClientMessage
 */
static void EVENT_ClientMessage( HWND hwnd, XEvent *xev )
{
    XClientMessageEvent *event = &xev->xclient;
    unsigned int i;

    if (!hwnd) return;

    if (event->format != 32)
    {
        WARN( "Don't know how to handle format %d\n", event->format );
        return;
    }

    for (i = 0; i < sizeof(client_messages)/sizeof(client_messages[0]); i++)
    {
        if (event->message_type == X11DRV_Atoms[client_messages[i].atom - FIRST_XATOM])
        {
            client_messages[i].handler( hwnd, event );
            return;
        }
    }
    TRACE( "no handler found for %ld\n", event->message_type );
}


/**********************************************************************
 *           X11DRV_WindowMessage   (X11DRV.@)
 */
LRESULT X11DRV_WindowMessage( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    switch(msg)
    {
    case WM_X11DRV_ACQUIRE_SELECTION:
        return X11DRV_AcquireClipboard( hwnd );
    case WM_X11DRV_DELETE_WINDOW:
        return SendMessageW( hwnd, WM_SYSCOMMAND, SC_CLOSE, 0 );
    default:
        FIXME( "got window msg %x hwnd %p wp %lx lp %lx\n", msg, hwnd, wp, lp );
        return 0;
    }
}


/***********************************************************************
 *		X11DRV_SendInput  (X11DRV.@)
 */
UINT X11DRV_SendInput( UINT count, LPINPUT inputs, int size )
{
    UINT i;

    for (i = 0; i < count; i++, inputs++)
    {
        switch(inputs->type)
        {
        case INPUT_MOUSE:
            X11DRV_send_mouse_input( 0, inputs->u.mi.dwFlags, inputs->u.mi.dx, inputs->u.mi.dy,
                                     inputs->u.mi.mouseData, inputs->u.mi.time,
                                     inputs->u.mi.dwExtraInfo, LLMHF_INJECTED );
            break;
        case INPUT_KEYBOARD:
            X11DRV_send_keyboard_input( inputs->u.ki.wVk, inputs->u.ki.wScan, inputs->u.ki.dwFlags,
                                        inputs->u.ki.time, inputs->u.ki.dwExtraInfo, LLKHF_INJECTED );
            break;
        case INPUT_HARDWARE:
            FIXME( "INPUT_HARDWARE not supported\n" );
            break;
        }
    }
    return count;
}
