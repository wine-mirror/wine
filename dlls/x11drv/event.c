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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define COM_NO_WINDOWS_H
#include "config.h"

#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#ifdef HAVE_LIBXXF86DGA2
#include <X11/extensions/xf86dga.h>
#endif

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include "wine/winuser16.h"
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "shlobj.h"  /* DROPFILES */

#include "win.h"
#include "winreg.h"
#include "x11drv.h"
#include "shellapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(event);
WINE_DECLARE_DEBUG_CHANNEL(clipboard);

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

static const char * const event_names[] =
{
  "", "", "KeyPress", "KeyRelease", "ButtonPress", "ButtonRelease",
  "MotionNotify", "EnterNotify", "LeaveNotify", "FocusIn", "FocusOut",
  "KeymapNotify", "Expose", "GraphicsExpose", "NoExpose", "VisibilityNotify",
  "CreateNotify", "DestroyNotify", "UnmapNotify", "MapNotify", "MapRequest",
  "ReparentNotify", "ConfigureNotify", "ConfigureRequest", "GravityNotify",
  "ResizeRequest", "CirculateNotify", "CirculateRequest", "PropertyNotify",
  "SelectionClear", "SelectionRequest", "SelectionNotify", "ColormapNotify",
  "ClientMessage", "MappingNotify"
};


static void EVENT_ProcessEvent( XEvent *event );

  /* Event handlers */
static void EVENT_FocusIn( HWND hWnd, XFocusChangeEvent *event );
static void EVENT_FocusOut( HWND hWnd, XFocusChangeEvent *event );
static void EVENT_PropertyNotify( XPropertyEvent *event );
static void EVENT_ClientMessage( HWND hWnd, XClientMessageEvent *event );

#ifdef HAVE_LIBXXF86DGA2
static int DGAMotionEventType;
static int DGAButtonPressEventType;
static int DGAButtonReleaseEventType;
static int DGAKeyPressEventType;
static int DGAKeyReleaseEventType;

static BOOL DGAUsed = FALSE;
static HWND DGAhwnd = 0;
#endif

/* Static used for the current input method */
static INPUT_TYPE current_input_type = X11DRV_INPUT_ABSOLUTE;
static BOOL in_transition = FALSE; /* This is not used as for today */


/***********************************************************************
 *           process_events
 */
static int process_events( struct x11drv_thread_data *data )
{
    XEvent event;
    int count = 0;

    wine_tsx11_lock();
    while ( XPending( data->display ) )
    {
        Bool ignore;

        XNextEvent( data->display, &event );
        ignore = XFilterEvent( &event, None );
        wine_tsx11_unlock();
        if (!ignore) EVENT_ProcessEvent( &event );
        count++;
        wine_tsx11_lock();
    }
    wine_tsx11_unlock();
    return count;
}


/***********************************************************************
 *           MsgWaitForMultipleObjectsEx   (X11DRV.@)
 */
DWORD X11DRV_MsgWaitForMultipleObjectsEx( DWORD count, const HANDLE *handles,
                                          DWORD timeout, DWORD mask, DWORD flags )
{
    HANDLE new_handles[MAXIMUM_WAIT_OBJECTS+1];  /* FIXME! */
    DWORD i, ret;
    struct x11drv_thread_data *data = NtCurrentTeb()->driver_data;

    if (!data || data->process_event_count)
        return WaitForMultipleObjectsEx( count, handles, flags & MWMO_WAITALL,
                                         timeout, flags & MWMO_ALERTABLE );

    /* check whether only server queue handle was passed in */
    if (count < 2) flags &= ~MWMO_WAITALL;

    for (i = 0; i < count; i++) new_handles[i] = handles[i];
    new_handles[count] = data->display_fd;

    wine_tsx11_lock();
    XFlush( gdi_display );
    XFlush( data->display );
    wine_tsx11_unlock();

    data->process_event_count++;
    if (process_events( data )) ret = count;
    else
    {
        ret = WaitForMultipleObjectsEx( count+1, new_handles, flags & MWMO_WAITALL,
                                        timeout, flags & MWMO_ALERTABLE );
        if (ret == count) process_events( data );
    }
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

/***********************************************************************
 *           EVENT_ProcessEvent
 *
 * Process an X event.
 */
static void EVENT_ProcessEvent( XEvent *event )
{
  HWND hWnd;
  Display *display = event->xany.display;

  TRACE( "called.\n" );

  switch (event->type)
  {
    case SelectionNotify: /* all of these should be caught by XCheckTypedWindowEvent() */
	 FIXME("Got SelectionNotify - must not happen!\n");
	 /* fall through */

      /* We get all these because of StructureNotifyMask.
         This check is placed here to avoid getting error messages below,
         as X might send some of these even for windows that have already
         been deleted ... */
    case CirculateNotify:
    case CreateNotify:
    case DestroyNotify:
    case GravityNotify:
    case ReparentNotify:
      return;
  }

#ifdef HAVE_LIBXXF86DGA2
  if (DGAUsed) {
    if (event->type == DGAMotionEventType) {
      TRACE("DGAMotionEvent received.\n");
      X11DRV_DGAMotionEvent( DGAhwnd, (XDGAMotionEvent *)event );
      return;
    }
    if (event->type == DGAButtonPressEventType) {
      TRACE("DGAButtonPressEvent received.\n");
      X11DRV_DGAButtonPressEvent( DGAhwnd, (XDGAButtonEvent *)event );
      return;
    }
    if (event->type == DGAButtonReleaseEventType) {
      TRACE("DGAButtonReleaseEvent received.\n");
      X11DRV_DGAButtonReleaseEvent( DGAhwnd, (XDGAButtonEvent *)event );
      return;
    }
    if ((event->type == DGAKeyPressEventType) ||
	(event->type == DGAKeyReleaseEventType)) {
      /* Fill a XKeyEvent to send to EVENT_Key */
      XKeyEvent ke;
      XDGAKeyEvent *evt = (XDGAKeyEvent *) event;

      TRACE("DGAKeyPress/ReleaseEvent received.\n");

      if (evt->type == DGAKeyReleaseEventType)
	ke.type = KeyRelease;
      else
	ke.type = KeyPress;
      ke.serial = evt->serial;
      ke.send_event = FALSE;
      ke.display = evt->display;
      ke.window = 0;
      ke.root = 0;
      ke.subwindow = 0;
      ke.time = evt->time;
      ke.x = -1;
      ke.y = -1;
      ke.x_root = -1;
      ke.y_root = -1;
      ke.state = evt->state;
      ke.keycode = evt->keycode;
      ke.same_screen = TRUE;
      X11DRV_KeyEvent( 0, &ke );
      return;
    }
  }
#endif

  wine_tsx11_lock();
  if (XFindContext( display, event->xany.window, winContext, (char **)&hWnd ) != 0)
      hWnd = 0;  /* Not for a registered window */
  wine_tsx11_unlock();
  if (!hWnd && event->xany.window == root_window) hWnd = GetDesktopWindow();

  if (!hWnd && event->type != PropertyNotify &&
      event->type != MappingNotify && event->type != KeymapNotify)
      WARN( "Got event %s for unknown Window %08lx\n",
            event_names[event->type], event->xany.window );
  else if (event->type <= MappingNotify)
      TRACE("Got event %s for hwnd/window %p/%lx, GetFocus()=%p\n",
            event_names[event->type], hWnd, event->xany.window, GetFocus() );
  else
      TRACE("Got extension event for hwnd/window %p/%lx, GetFocus()=%p\n",
            hWnd, event->xany.window, GetFocus() );

  if (X11DRV_ProcessTabletEvent(hWnd, event))
  {
        TRACE("Return: filtered by tablet\n");
        return;
  }

  switch(event->type)
    {
    case KeyPress:
    case KeyRelease:
      /* FIXME: should generate a motion event if event point is different from current pos */
      X11DRV_KeyEvent( hWnd, (XKeyEvent*)event );
      break;

    case ButtonPress:
      X11DRV_ButtonPress( hWnd, (XButtonEvent*)event );
      break;

    case ButtonRelease:
      X11DRV_ButtonRelease( hWnd, (XButtonEvent*)event );
      break;

    case MotionNotify:
      X11DRV_MotionNotify( hWnd, (XMotionEvent*)event );
      break;

    case EnterNotify:
      X11DRV_EnterNotify( hWnd, (XCrossingEvent*)event );
      break;

    case FocusIn:
      EVENT_FocusIn( hWnd, (XFocusChangeEvent*)event );
      break;

    case FocusOut:
      EVENT_FocusOut( hWnd, (XFocusChangeEvent*)event );
      break;

    case Expose:
      X11DRV_Expose( hWnd, &event->xexpose );
      break;

    case ConfigureNotify:
      if (!hWnd) return;
      X11DRV_ConfigureNotify( hWnd, &event->xconfigure );
      break;

    case SelectionRequest:
      X11DRV_SelectionRequest( hWnd, (XSelectionRequestEvent *)event );
      break;

    case SelectionClear:
      X11DRV_SelectionClear( hWnd, (XSelectionClearEvent*) event );
      break;

    case PropertyNotify:
      EVENT_PropertyNotify( (XPropertyEvent *)event );
      break;

    case ClientMessage:
      if (!hWnd) return;
      EVENT_ClientMessage( hWnd, (XClientMessageEvent *) event );
      break;

    case NoExpose:
      break;

    case MapNotify:
      X11DRV_MapNotify( hWnd, (XMapEvent *)event );
      break;

    case UnmapNotify:
      X11DRV_UnmapNotify( hWnd, (XUnmapEvent *)event );
      break;

    case KeymapNotify:
      X11DRV_KeymapNotify( hWnd, (XKeymapEvent *)event );
      break;

    case MappingNotify:
      X11DRV_MappingNotify( (XMappingEvent *) event );
      break;

    default:
      WARN("Unprocessed event %s for hwnd %p\n", event_names[event->type], hWnd );
      break;
    }
    TRACE( "returns.\n" );
}


/*******************************************************************
 *         can_activate_window
 *
 * Check if we can activate the specified window.
 */
inline static BOOL can_activate_window( HWND hwnd )
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
 *              handle_wm_protocols_message
 */
static void handle_wm_protocols_message( HWND hwnd, XClientMessageEvent *event )
{
    Atom protocol = (Atom)event->data.l[0];

    if (!protocol) return;

    if (protocol == x11drv_atom(WM_DELETE_WINDOW))
    {
        /* Ignore the delete window request if the window has been disabled
         * and we are in managed mode. This is to disallow applications from
         * being closed by the window manager while in a modal state.
         */
        if (IsWindowEnabled(hwnd)) PostMessageW( hwnd, WM_SYSCOMMAND, SC_CLOSE, 0 );
    }
    else if (protocol == x11drv_atom(WM_TAKE_FOCUS))
    {
        Time event_time = (Time)event->data.l[1];
        HWND last_focus = x11drv_thread_data()->last_focus;

        TRACE( "got take focus msg for %p, enabled=%d, focus=%p, active=%p, fg=%p, last=%p\n",
               hwnd, IsWindowEnabled(hwnd), GetFocus(), GetActiveWindow(),
               GetForegroundWindow(), last_focus );

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
      xev.window = DefaultRootWindow(xev.display);
      XSendEvent(xev.display, xev.window, False, SubstructureRedirectMask | SubstructureNotifyMask, (XEvent*)&xev);
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
static void EVENT_FocusIn( HWND hwnd, XFocusChangeEvent *event )
{
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
static void EVENT_FocusOut( HWND hwnd, XFocusChangeEvent *event )
{
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
    SendMessageA( hwnd, WM_CANCELMODE, 0, 0 );

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
static void EVENT_PropertyNotify( XPropertyEvent *event )
{
  /* Check if we have any resources to free */
  TRACE("Received PropertyNotify event: \n");

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
        GetClientRect( hQueryWnd, &tempRect );
        MapWindowPoints( hQueryWnd, 0, (LPPOINT)&tempRect, 2 );

        if (PtInRect( &tempRect, *lpPt))
        {
            HWND *list = WIN_ListChildren( hQueryWnd );
    	    HWND bResult = 0;

            if (list)
            {
                int i;
		
                for (i = 0; list[i]; i++)
                {
                    if (GetWindowLongW( list[i], GWL_STYLE ) & WS_VISIBLE)
                    {
                        GetWindowRect( list[i], &tempRect );
                        if (PtInRect( &tempRect, *lpPt )) break;
                    }
                }
                if (list[i])
                {
                    if (IsWindowEnabled( list[i] ))
                        bResult = find_drop_window( list[i], lpPt );
                }
                HeapFree( GetProcessHeap(), 0, list );
            }
            if(bResult) return bResult;
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
    union {
    	Atom	atom_aux;
    	struct {
      	    int x;
      	    int y;
    	} 	pt_aux;
    	int	i;
    } 			u;
    int			x, y;
    BOOL	        bAccept;
    Window		win, w_aux_root, w_aux_child;
    WND*                pWnd;
    HWND		hScope = hWnd;

    win = X11DRV_get_whole_window(hWnd);
    wine_tsx11_lock();
    XQueryPointer( event->display, win, &w_aux_root, &w_aux_child,
                   &x, &y, (int *) &u.pt_aux.x, (int *) &u.pt_aux.y,
                   (unsigned int*)&aux_long);
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
                        AnyPropertyType, &u.atom_aux, (int *) &u.pt_aux.y,
                        &data_length, &aux_long, &p_data);
    wine_tsx11_unlock();

    if( !aux_long && p_data)  /* don't bother if > 64K */
    {
        signed char *p = (signed char*) p_data;
        char *p_drop;

        aux_long = 0;
        while( *p )  /* calculate buffer size */
        {
            p_drop = p;
            if((u.i = *p) != -1 )
            {
                INT len = GetShortPathNameA( p, NULL, 0 );
                if (len) aux_long += len + 1;
                else *p = -1;
            }
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
                p = p_data;
                while(*p)
                {
                    if( *p != -1 ) /* use only "good" entries */
                    {
                        GetShortPathNameA( p, p_drop, 65535 );
                        p_drop += strlen( p_drop ) + 1;
                    }
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
    p = p_data;
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
                     &x, &y, &u.i, &u.i, &u.i);
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
	p = p_data;
	next = strchr(p, '\n');
	while (p) {
	  if (next) *next=0;
	  if (strncmp(p,"file:",5) == 0 ) {
	    INT len = GetShortPathNameA( p+5, p_drop, 65535 );
	    if (len) {
	      TRACE("drop file %s as %s\n", p+5, p_drop);
	      p_drop += len+1;
	    } else {
	      WARN("can't convert file %s to dos name \n", p+5);
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
 *           EVENT_ClientMessage
 */
static void EVENT_ClientMessage( HWND hWnd, XClientMessageEvent *event )
{
  if (event->message_type != None && event->format == 32) {
    if (event->message_type == x11drv_atom(WM_PROTOCOLS))
        handle_wm_protocols_message( hWnd, event );
    else if (event->message_type == x11drv_atom(DndProtocol))
    {
        /* query window (drag&drop event contains only drag window) */
        Window root, child;
        int root_x, root_y, child_x, child_y;
        unsigned int u;

        wine_tsx11_lock();
        XQueryPointer( event->display, root_window, &root, &child,
                       &root_x, &root_y, &child_x, &child_y, &u);
        if (XFindContext( event->display, child, winContext, (char **)&hWnd ) != 0) hWnd = 0;
        wine_tsx11_unlock();
        if (!hWnd) return;
        if (event->data.l[0] == DndFile || event->data.l[0] == DndFiles)
            EVENT_DropFromOffiX(hWnd, event);
        else if (event->data.l[0] == DndURL)
            EVENT_DropURLs(hWnd, event);
    }
    else if (!X11DRV_XDND_Event(hWnd, event))
    {
#if 0
      /* enable this if you want to see the message */
      unsigned char* p_data = NULL;
      union {
	unsigned long	l;
	int            	i;
	Atom		atom;
      } u; /* unused */
      wine_tsx11_lock();
      XGetWindowProperty( event->display, DefaultRootWindow(event->display),
			    dndSelection, 0, 65535, FALSE,
			    AnyPropertyType, &u.atom, &u.i,
			    &u.l, &u.l, &p_data);
      wine_tsx11_unlock();
      TRACE("message_type=%ld, data=%ld,%ld,%ld,%ld,%ld, msg=%s\n",
	    event->message_type, event->data.l[0], event->data.l[1],
	    event->data.l[2], event->data.l[3], event->data.l[4],
	    p_data);
#endif
      TRACE("unrecognized ClientMessage\n" );
    }
  }
}


/**********************************************************************
 *              X11DRV_EVENT_SetInputMethod
 */
INPUT_TYPE X11DRV_EVENT_SetInputMethod(INPUT_TYPE type)
{
  INPUT_TYPE prev = current_input_type;

  /* Flag not used yet */
  in_transition = FALSE;
  current_input_type = type;

  return prev;
}

#ifdef HAVE_LIBXXF86DGA2
/**********************************************************************
 *              X11DRV_EVENT_SetDGAStatus
 */
void X11DRV_EVENT_SetDGAStatus(HWND hwnd, int event_base)
{
  if (event_base < 0) {
    DGAUsed = FALSE;
    DGAhwnd = 0;
  } else {
    DGAUsed = TRUE;
    DGAhwnd = hwnd;
    DGAMotionEventType = event_base + MotionNotify;
    DGAButtonPressEventType = event_base + ButtonPress;
    DGAButtonReleaseEventType = event_base + ButtonRelease;
    DGAKeyPressEventType = event_base + KeyPress;
    DGAKeyReleaseEventType = event_base + KeyRelease;
  }
}
#endif
