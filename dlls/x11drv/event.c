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

#include "config.h"

#include <X11/Xatom.h>
#include <X11/keysym.h>

#include "ts_xlib.h"
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#ifdef HAVE_LIBXXF86DGA2
#include <X11/extensions/xf86dga.h>
#endif

#include <assert.h>
#include <string.h>
#include "wine/winuser16.h"
#include "shlobj.h"  /* DROPFILES */

#include "clipboard.h"
#include "dce.h"
#include "wine/debug.h"
#include "input.h"
#include "win.h"
#include "winpos.h"
#include "windef.h"
#include "winreg.h"
#include "x11drv.h"
#include "shellapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(event);

/* X context to associate a hwnd to an X window */
extern XContext winContext;

extern Atom wmProtocols;
extern Atom wmDeleteWindow;
extern Atom dndProtocol;
extern Atom dndSelection;

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
static void EVENT_SelectionRequest( HWND hWnd, XSelectionRequestEvent *event, BOOL bIsMultiple );
static void EVENT_SelectionClear( HWND hWnd, XSelectionClearEvent *event);
static void EVENT_PropertyNotify( XPropertyEvent *event );
static void EVENT_ClientMessage( HWND hWnd, XClientMessageEvent *event );

extern void X11DRV_ButtonPress( HWND hwnd, XButtonEvent *event );
extern void X11DRV_ButtonRelease( HWND hwnd, XButtonEvent *event );
extern void X11DRV_MotionNotify( HWND hwnd, XMotionEvent *event );
extern void X11DRV_EnterNotify( HWND hwnd, XCrossingEvent *event );
extern void X11DRV_KeyEvent( HWND hwnd, XKeyEvent *event );
extern void X11DRV_KeymapNotify( HWND hwnd, XKeymapEvent *event );
extern void X11DRV_Expose( HWND hwnd, XExposeEvent *event );
extern void X11DRV_MapNotify( HWND hwnd, XMapEvent *event );
extern void X11DRV_UnmapNotify( HWND hwnd, XUnmapEvent *event );
extern void X11DRV_ConfigureNotify( HWND hwnd, XConfigureEvent *event );
extern void X11DRV_MappingNotify( XMappingEvent *event );

#ifdef HAVE_LIBXXF86DGA2
static int DGAMotionEventType;
static int DGAButtonPressEventType;
static int DGAButtonReleaseEventType;
static int DGAKeyPressEventType;
static int DGAKeyReleaseEventType;

static BOOL DGAUsed = FALSE;
static HWND DGAhwnd = 0;

extern void X11DRV_DGAMotionEvent( HWND hwnd, XDGAMotionEvent *event );
extern void X11DRV_DGAButtonPressEvent( HWND hwnd, XDGAButtonEvent *event );
extern void X11DRV_DGAButtonReleaseEvent( HWND hwnd, XDGAButtonEvent *event );
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
        XNextEvent( data->display, &event );
        wine_tsx11_unlock();
        EVENT_ProcessEvent( &event );
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

  if ( !hWnd && event->xany.window != root_window
             && event->type != PropertyNotify
             && event->type != MappingNotify)
      WARN( "Got event %s for unknown Window %08lx\n",
            event_names[event->type], event->xany.window );
  else
      TRACE("Got event %s for hwnd %04x\n",
            event_names[event->type], hWnd );

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
      if (!hWnd) return;
      EVENT_SelectionRequest( hWnd, (XSelectionRequestEvent *)event, FALSE );
      break;

    case SelectionClear:
      if (!hWnd) return;
      EVENT_SelectionClear( hWnd, (XSelectionClearEvent*) event );
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
      WARN("Unprocessed event %s for hwnd %04x\n",
	   event_names[event->type], hWnd );
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
 *              set_focus_error_handler
 *
 * Handler for X errors happening during XSetInputFocus call.
 */
static int set_focus_error_handler( Display *display, XErrorEvent *event, void *arg )
{
    return (event->error_code == BadMatch);
}


/**********************************************************************
 *              set_focus
 */
static void set_focus( HWND hwnd, Time time )
{
    HWND focus;
    Window win;

    TRACE( "setting foreground window to %x\n", hwnd );
    SetForegroundWindow( hwnd );

    focus = GetFocus();
    win = X11DRV_get_whole_window(focus);

    if (win)
    {
        Display *display = thread_display();
        TRACE( "setting focus to %x (%lx) time=%ld\n", focus, win, time );
        X11DRV_expect_error( display, set_focus_error_handler, NULL );
        XSetInputFocus( display, win, RevertToParent, time );
        if (X11DRV_check_error()) TRACE("got BadMatch, ignoring\n" );
    }
}


/**********************************************************************
 *              handle_wm_protocols_message
 */
static void handle_wm_protocols_message( HWND hwnd, XClientMessageEvent *event )
{
    Atom protocol = (Atom)event->data.l[0];

    if (!protocol) return;

    if (protocol == wmDeleteWindow)
    {
        /* Ignore the delete window request if the window has been disabled
         * and we are in managed mode. This is to disallow applications from
         * being closed by the window manager while in a modal state.
         */
        if (IsWindowEnabled(hwnd)) PostMessageW( hwnd, WM_SYSCOMMAND, SC_CLOSE, 0 );
    }
    else if (protocol == wmTakeFocus)
    {
        Time event_time = (Time)event->data.l[1];
        HWND last_focus = x11drv_thread_data()->last_focus;

        TRACE( "got take focus msg for %x, enabled=%d, focus=%x, active=%x, fg=%x, last=%x\n",
               hwnd, IsWindowEnabled(hwnd), GetFocus(), GetActiveWindow(),
               GetForegroundWindow(), last_focus );

        if (can_activate_window(hwnd))
        {
            /* simulate a mouse click on the caption to find out
             * whether the window wants to be activated */
            LRESULT ma = SendMessageW( hwnd, WM_MOUSEACTIVATE,
                                       GetAncestor( hwnd, GA_ROOT ),
                                       MAKELONG(HTCAPTION,WM_LBUTTONDOWN) );
            if (ma != MA_NOACTIVATEANDEAT && ma != MA_NOACTIVATE) set_focus( hwnd, event_time );
            else TRACE( "not setting focus to %x (%lx), ma=%ld\n", hwnd, event->window, ma );
        }
        else
        {
            hwnd = GetFocus();
            if (!hwnd) hwnd = GetActiveWindow();
            if (!hwnd) hwnd = last_focus;
            if (hwnd && can_activate_window(hwnd)) set_focus( hwnd, event_time );
        }
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
    if (!hwnd) return;

    TRACE( "win %x xwin %lx detail=%s\n", hwnd, event->window, focus_details[event->detail] );

    if (wmTakeFocus) return;  /* ignore FocusIn if we are using take focus */
    if (event->detail == NotifyPointer) return;

    if (!can_activate_window(hwnd))
    {
        HWND hwnd = GetFocus();
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

    TRACE( "win %x xwin %lx detail=%s\n", hwnd, event->window, focus_details[event->detail] );

    if (event->detail == NotifyPointer) return;
    x11drv_thread_data()->last_focus = hwnd;
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
 *           EVENT_SelectionRequest_TARGETS
 *  Service a TARGETS selection request event
 */
static Atom EVENT_SelectionRequest_TARGETS( Display *display, Window requestor,
                                            Atom target, Atom rprop )
{
    Atom xaTargets = TSXInternAtom(display, "TARGETS", False);
    Atom* targets;
    Atom prop;
    UINT wFormat;
    unsigned long cTargets;
    BOOL bHavePixmap;
    int xRc;

    TRACE("Request for %s\n", TSXGetAtomName(display, target));

    /*
     * Count the number of items we wish to expose as selection targets.
     * We include the TARGETS item, and a PIXMAP if we have CF_DIB or CF_BITMAP
     */
    cTargets = CountClipboardFormats() + 1;
    if ( CLIPBOARD_IsPresent(CF_DIB) ||  CLIPBOARD_IsPresent(CF_BITMAP) )
       cTargets++;

    /* Allocate temp buffer */
    targets = (Atom*)HeapAlloc( GetProcessHeap(), 0, cTargets * sizeof(Atom));
    if(targets == NULL) return None;

    /* Create TARGETS property list (First item in list is TARGETS itself) */

    for ( targets[0] = xaTargets, cTargets = 1, wFormat = 0, bHavePixmap = FALSE;
          (wFormat = EnumClipboardFormats( wFormat )); )
    {
        if ( (prop = X11DRV_CLIPBOARD_MapFormatToProperty(wFormat)) != None )
        {
            /* Scan through what we have so far to avoid duplicates */
            int i;
            BOOL bExists;
            for (i = 0, bExists = FALSE; i < cTargets; i++)
            {
                if (targets[i] == prop)
                {
                    bExists = TRUE;
                    break;
                }
            }
            if (!bExists)
            {
                targets[cTargets++] = prop;

                /* Add PIXMAP prop for bitmaps additionally */
                if ( (wFormat == CF_DIB || wFormat == CF_BITMAP )
                     && !bHavePixmap )
                {
                    targets[cTargets++] = XA_PIXMAP;
                    bHavePixmap = TRUE;
                }
            }
        }
    }

    if (TRACE_ON(event))
    {
        int i;
        for ( i = 0; i < cTargets; i++)
        {
            if (targets[i])
            {
                char *itemFmtName = TSXGetAtomName(display, targets[i]);
                TRACE("\tAtom# %d:  Type %s\n", i, itemFmtName);
                TSXFree(itemFmtName);
            }
        }
    }

    /* Update the X property */
    TRACE("\tUpdating property %s...\n", TSXGetAtomName(display, rprop));

    /* We may want to consider setting the type to xaTargets instead,
     * in case some apps expect this instead of XA_ATOM */
    xRc = TSXChangeProperty(display, requestor, rprop,
                            XA_ATOM, 32, PropModeReplace,
                            (unsigned char *)targets, cTargets);
    TRACE("(Rc=%d)\n", xRc);

    HeapFree( GetProcessHeap(), 0, targets );

    return rprop;
}


/***********************************************************************
 *           EVENT_SelectionRequest_STRING
 *  Service a STRING selection request event
 */
static Atom EVENT_SelectionRequest_STRING( Display *display, Window requestor,
                                           Atom target, Atom rprop )
{
    static UINT text_cp = (UINT)-1;
    HANDLE hUnicodeText;
    LPWSTR uni_text;
    LPSTR  text;
    int    size,i,j;
    char* lpstr = 0;
    char *itemFmtName;
    int xRc;

    if(text_cp == (UINT)-1)
    {
	HKEY hkey;
	/* default value */
	text_cp = CP_ACP;
	if(!RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\Wine\\Wine\\Config\\x11drv", &hkey))
	{
	    char buf[20];
	    DWORD type, count = sizeof(buf);
	    if(!RegQueryValueExA(hkey, "TextCP", 0, &type, buf, &count))
		text_cp = atoi(buf);
	    RegCloseKey(hkey);
	  }
    }

    /*
     * Map the requested X selection property type atom name to a
     * windows clipboard format ID.
     */
    itemFmtName = TSXGetAtomName(display, target);
    TRACE("Request for %s (wFormat=%x %s)\n",
	itemFmtName, CF_UNICODETEXT, CLIPBOARD_GetFormatName(CF_UNICODETEXT, NULL, 0));
    TSXFree(itemFmtName);

    hUnicodeText = GetClipboardData(CF_UNICODETEXT);
    if(!hUnicodeText)
       return None;
    uni_text = GlobalLock(hUnicodeText);
    if(!uni_text)
       return None;

    size = WideCharToMultiByte(text_cp, 0, uni_text, -1, NULL, 0, NULL, NULL);
    text = HeapAlloc(GetProcessHeap(), 0, size);
    if (!text)
       return None;
    WideCharToMultiByte(text_cp, 0, uni_text, -1, text, size, NULL, NULL);

    /* remove carriage returns */

    lpstr = (char*)HeapAlloc( GetProcessHeap(), 0, size-- );
    if(lpstr == NULL) return None;
    for(i=0,j=0; i < size && text[i]; i++ )
    {
        if( text[i] == '\r' &&
            (text[i+1] == '\n' || text[i+1] == '\0') ) continue;
        lpstr[j++] = text[i];
    }
    lpstr[j]='\0';

    /* Update the X property */
    TRACE("\tUpdating property %s...\n", TSXGetAtomName(display, rprop));
    xRc = TSXChangeProperty(display, requestor, rprop,
                            XA_STRING, 8, PropModeReplace,
                            lpstr, j);
    TRACE("(Rc=%d)\n", xRc);

    GlobalUnlock(hUnicodeText);
    HeapFree(GetProcessHeap(), 0, text);
    HeapFree( GetProcessHeap(), 0, lpstr );

    return rprop;
}

/***********************************************************************
 *           EVENT_SelectionRequest_PIXMAP
 *  Service a PIXMAP selection request event
 */
static Atom EVENT_SelectionRequest_PIXMAP( Display *display, Window requestor,
                                           Atom target, Atom rprop )
{
    HANDLE hClipData = 0;
    Pixmap pixmap = 0;
    UINT   wFormat;
    char * itemFmtName;
    int xRc;
#if(0)
    XSetWindowAttributes win_attr;
    XWindowAttributes win_attr_src;
#endif

    /*
     * Map the requested X selection property type atom name to a
     * windows clipboard format ID.
     */
    itemFmtName = TSXGetAtomName(display, target);
    wFormat = X11DRV_CLIPBOARD_MapPropertyToFormat(itemFmtName);
    TRACE("Request for %s (wFormat=%x %s)\n",
                  itemFmtName, wFormat, CLIPBOARD_GetFormatName( wFormat, NULL, 0 ));
    TSXFree(itemFmtName);

    hClipData = GetClipboardData(wFormat);
    if ( !hClipData )
    {
        TRACE("Could not retrieve a Pixmap compatible format from clipboard!\n");
        rprop = None; /* Fail the request */
        goto END;
    }

    if (wFormat == CF_DIB)
    {
        HWND hwnd = GetOpenClipboardWindow();
        HDC hdc = GetDC(hwnd);

        /* For convert from packed DIB to Pixmap */
        pixmap = X11DRV_DIB_CreatePixmapFromDIB(hClipData, hdc);

        ReleaseDC(hwnd, hdc);
    }
    else if (wFormat == CF_BITMAP)
    {
        HWND hwnd = GetOpenClipboardWindow();
        HDC hdc = GetDC(hwnd);

        pixmap = X11DRV_BITMAP_CreatePixmapFromBitmap(hClipData, hdc);

        ReleaseDC(hwnd, hdc);
    }
    else
    {
        FIXME("%s to PIXMAP conversion not yet implemented!\n",
                      CLIPBOARD_GetFormatName(wFormat, NULL, 0));
        rprop = None;
        goto END;
    }

    TRACE("\tUpdating property %s on Window %ld with %s %ld...\n",
          TSXGetAtomName(display, rprop), (long)requestor,
          TSXGetAtomName(display, target), pixmap);

    /* Store the Pixmap handle in the property */
    xRc = TSXChangeProperty(display, requestor, rprop, target,
                            32, PropModeReplace,
                            (unsigned char *)&pixmap, 1);
    TRACE("(Rc=%d)\n", xRc);

    /* Enable the code below if you want to handle destroying Pixmap resources
     * in response to property notify events. Clients like XPaint don't
     * appear to be duplicating Pixmaps so they don't like us deleting,
     * the resource in response to the property being deleted.
     */
#if(0)
    /* Express interest in property notify events so that we can delete the
     * pixmap when the client deletes the property atom.
     */
    xRc = TSXGetWindowAttributes(display, requestor, &win_attr_src);
    TRACE("Turning on PropertyChangeEvent notifications from window %ld\n",
          (long)requestor);
    win_attr.event_mask = win_attr_src.your_event_mask | PropertyChangeMask;
    TSXChangeWindowAttributes(display, requestor, CWEventMask, &win_attr);

    /* Register the Pixmap we created with the request property Atom.
     * When this property is destroyed we also destroy the Pixmap in
     * response to the PropertyNotify event.
     */
    X11DRV_CLIPBOARD_RegisterPixmapResource( rprop, pixmap );
#endif

END:
    return rprop;
}


/***********************************************************************
 *           EVENT_SelectionRequest_WCF
 *  Service a Wine Clipboard Format selection request event.
 *  For <WCF>* data types we simply copy the data to X without conversion.
 */
static Atom EVENT_SelectionRequest_WCF( Display *display, Window requestor,
                                        Atom target, Atom rprop )
{
    HANDLE hClipData = 0;
    void*  lpClipData;
    UINT   wFormat;
    char * itemFmtName;
    int cBytes;
    int xRc;
    int bemf;

    /*
     * Map the requested X selection property type atom name to a
     * windows clipboard format ID.
     */
    itemFmtName = TSXGetAtomName(display, target);
    wFormat = X11DRV_CLIPBOARD_MapPropertyToFormat(itemFmtName);
    TRACE("Request for %s (wFormat=%x %s)\n",
          itemFmtName, wFormat, CLIPBOARD_GetFormatName( wFormat, NULL, 0));
    TSXFree(itemFmtName);

    hClipData = GetClipboardData(wFormat);

    bemf = wFormat == CF_METAFILEPICT || wFormat == CF_ENHMETAFILE;
    if (bemf)
        hClipData = X11DRV_CLIPBOARD_SerializeMetafile(wFormat, hClipData, sizeof(hClipData), TRUE);

    if( hClipData && (lpClipData = GlobalLock(hClipData)) )
    {
        cBytes = GlobalSize(hClipData);

        TRACE("\tUpdating property %s, %d bytes...\n",
              TSXGetAtomName(display, rprop), cBytes);

        xRc = TSXChangeProperty(display, requestor, rprop,
                                target, 8, PropModeReplace,
                                (unsigned char *)lpClipData, cBytes);
        TRACE("(Rc=%d)\n", xRc);

        GlobalUnlock(hClipData);
    }
    else
    {
        TRACE("\tCould not retrieve native format!\n");
        rprop = None; /* Fail the request */
    }

    if (bemf) /* We must free serialized metafile data */
        GlobalFree(hClipData);

    return rprop;
}


/***********************************************************************
 *           EVENT_SelectionRequest_MULTIPLE
 *  Service a MULTIPLE selection request event
 *  rprop contains a list of (target,property) atom pairs.
 *  The first atom names a target and the second names a property.
 *  The effect is as if we have received a sequence of SelectionRequest events
 *  (one for each atom pair) except that:
 *  1. We reply with a SelectionNotify only when all the requested conversions
 *  have been performed.
 *  2. If we fail to convert the target named by an atom in the MULTIPLE property,
 *  we replace the atom in the property by None.
 */
static Atom EVENT_SelectionRequest_MULTIPLE( HWND hWnd, XSelectionRequestEvent *pevent )
{
    Display *display = pevent->display;
    Atom           rprop;
    Atom           atype=AnyPropertyType;
    int		   aformat;
    unsigned long  remain;
    Atom*	   targetPropList=NULL;
    unsigned long  cTargetPropList = 0;
/*  Atom           xAtomPair = TSXInternAtom(display, "ATOM_PAIR", False); */

   /* If the specified property is None the requestor is an obsolete client.
    * We support these by using the specified target atom as the reply property.
    */
    rprop = pevent->property;
    if( rprop == None )
        rprop = pevent->target;
    if (!rprop)
        goto END;

    /* Read the MULTIPLE property contents. This should contain a list of
     * (target,property) atom pairs.
     */
    if(TSXGetWindowProperty(display, pevent->requestor, rprop,
                            0, 0x3FFF, False, AnyPropertyType, &atype,&aformat,
                            &cTargetPropList, &remain,
                            (unsigned char**)&targetPropList) != Success)
        TRACE("\tCouldn't read MULTIPLE property\n");
    else
    {
       TRACE("\tType %s,Format %d,nItems %ld, Remain %ld\n",
             TSXGetAtomName(display, atype), aformat, cTargetPropList, remain);

       /*
        * Make sure we got what we expect.
        * NOTE: According to the X-ICCCM Version 2.0 documentation the property sent
        * in a MULTIPLE selection request should be of type ATOM_PAIR.
        * However some X apps(such as XPaint) are not compliant with this and return
        * a user defined atom in atype when XGetWindowProperty is called.
        * The data *is* an atom pair but is not denoted as such.
        */
       if(aformat == 32 /* atype == xAtomPair */ )
       {
          int i;

          /* Iterate through the ATOM_PAIR list and execute a SelectionRequest
           * for each (target,property) pair */

          for (i = 0; i < cTargetPropList; i+=2)
          {
              char *targetName = TSXGetAtomName(display, targetPropList[i]);
              char *propName = TSXGetAtomName(display, targetPropList[i+1]);
              XSelectionRequestEvent event;

              TRACE("MULTIPLE(%d): Target='%s' Prop='%s'\n",
                    i/2, targetName, propName);
              TSXFree(targetName);
              TSXFree(propName);

              /* We must have a non "None" property to service a MULTIPLE target atom */
              if ( !targetPropList[i+1] )
              {
                  TRACE("\tMULTIPLE(%d): Skipping target with empty property!\n", i);
                  continue;
              }

              /* Set up an XSelectionRequestEvent for this (target,property) pair */
              memcpy( &event, pevent, sizeof(XSelectionRequestEvent) );
              event.target = targetPropList[i];
              event.property = targetPropList[i+1];

              /* Fire a SelectionRequest, informing the handler that we are processing
               * a MULTIPLE selection request event.
               */
              EVENT_SelectionRequest( hWnd, &event, TRUE );
          }
       }

       /* Free the list of targets/properties */
       TSXFree(targetPropList);
    }

END:
    return rprop;
}


/***********************************************************************
 *           EVENT_SelectionRequest
 *  Process an event selection request event.
 *  The bIsMultiple flag is used to signal when EVENT_SelectionRequest is called
 *  recursively while servicing a "MULTIPLE" selection target.
 *
 *  Note: We only receive this event when WINE owns the X selection
 */
static void EVENT_SelectionRequest( HWND hWnd, XSelectionRequestEvent *event, BOOL bIsMultiple )
{
    Display *display = event->display;
  XSelectionEvent result;
  Atom 	          rprop = None;
  Window          request = event->requestor;
  BOOL            couldOpen = FALSE;
  Atom            xaClipboard = TSXInternAtom(display, "CLIPBOARD", False);
  Atom            xaTargets = TSXInternAtom(display, "TARGETS", False);
  Atom            xaMultiple = TSXInternAtom(display, "MULTIPLE", False);

  /*
   * We can only handle the selection request if :
   * The selection is PRIMARY or CLIPBOARD, AND we can successfully open the clipboard.
   * Don't do these checks or open the clipboard while recursively processing MULTIPLE,
   * since this has been already done.
   */
  if ( !bIsMultiple )
  {
    if ( ( (event->selection != XA_PRIMARY) && (event->selection != xaClipboard) )
        || !(couldOpen = OpenClipboard(hWnd)) )
       goto END;
  }

  /* If the specified property is None the requestor is an obsolete client.
   * We support these by using the specified target atom as the reply property.
   */
  rprop = event->property;
  if( rprop == None )
      rprop = event->target;

  if(event->target == xaTargets)  /*  Return a list of all supported targets */
  {
      /* TARGETS selection request */
      rprop = EVENT_SelectionRequest_TARGETS( display, request, event->target, rprop );
  }
  else if(event->target == xaMultiple)  /*  rprop contains a list of (target, property) atom pairs */
  {
      /* MULTIPLE selection request */
      rprop = EVENT_SelectionRequest_MULTIPLE( hWnd, event );
  }
  else if(event->target == XA_STRING)  /* treat CF_TEXT as Unix text */
  {
      /* XA_STRING selection request */
      rprop = EVENT_SelectionRequest_STRING( display, request, event->target, rprop );
  }
  else if(event->target == XA_PIXMAP)  /*  Convert DIB's to Pixmaps */
  {
      /* XA_PIXMAP selection request */
      rprop = EVENT_SelectionRequest_PIXMAP( display, request, event->target, rprop );
  }
  else if(event->target == XA_BITMAP)  /*  Convert DIB's to 1-bit Pixmaps */
  {
      /* XA_BITMAP selection request - TODO: create a monochrome Pixmap */
      rprop = EVENT_SelectionRequest_PIXMAP( display, request, XA_PIXMAP, rprop );
  }
  else if(X11DRV_CLIPBOARD_IsNativeProperty(event->target)) /* <WCF>* */
  {
      /* All <WCF> selection requests */
      rprop = EVENT_SelectionRequest_WCF( display, request, event->target, rprop );
  }
  else
      rprop = None;  /* Don't support this format */

END:
  /* close clipboard only if we opened before */
  if(couldOpen) CloseClipboard();

  if( rprop == None)
      TRACE("\tRequest ignored\n");

  /* reply to sender
   * SelectionNotify should be sent only at the end of a MULTIPLE request
   */
  if ( !bIsMultiple )
  {
    result.type = SelectionNotify;
    result.display = display;
    result.requestor = request;
    result.selection = event->selection;
    result.property = rprop;
    result.target = event->target;
    result.time = event->time;
    TRACE("Sending SelectionNotify event...\n");
    TSXSendEvent(display,event->requestor,False,NoEventMask,(XEvent*)&result);
  }
}

/***********************************************************************
 *           EVENT_SelectionClear
 */
static void EVENT_SelectionClear( HWND hWnd, XSelectionClearEvent *event )
{
  Atom xaClipboard = TSXInternAtom(event->display, "CLIPBOARD", False);

  if (event->selection == XA_PRIMARY || event->selection == xaClipboard)
      X11DRV_CLIPBOARD_ReleaseSelection( event->selection, event->window, hWnd );
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
      TRACE("\tPropertyDelete for atom %s on window %ld\n",
            TSXGetAtomName(event->display, event->atom), (long)event->window);

      if (X11DRV_IsSelectionOwner())
          X11DRV_CLIPBOARD_FreeResources( event->atom );
      break;
    }

    case PropertyNewValue:
    {
      TRACE("\tPropertyNewValue for atom %s on window %ld\n\n",
            TSXGetAtomName(event->display, event->atom), (long)event->window);
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
    Window		w_aux_root, w_aux_child;
    WND*                pWnd;
    HWND		hScope = hWnd;

    pWnd = WIN_FindWndPtr(hWnd);

    TSXQueryPointer( event->display, get_whole_window(pWnd), &w_aux_root, &w_aux_child,
                     &x, &y, (int *) &u.pt_aux.x, (int *) &u.pt_aux.y,
                     (unsigned int*)&aux_long);

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
    WIN_ReleaseWndPtr(pWnd);

    if (!bAccept) return;

    TSXGetWindowProperty( event->display, DefaultRootWindow(event->display),
                          dndSelection, 0, 65535, FALSE,
                          AnyPropertyType, &u.atom_aux, (int *) &u.pt_aux.y,
                          &data_length, &aux_long, &p_data);

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
                WND *pDropWnd = WIN_FindWndPtr( hScope );
                lpDrop->pFiles = sizeof(DROPFILES);
                lpDrop->pt.x = x;
                lpDrop->pt.y = y;
                lpDrop->fNC =
                    ( x < (pDropWnd->rectClient.left - pDropWnd->rectWindow.left)  ||
                      y < (pDropWnd->rectClient.top - pDropWnd->rectWindow.top)    ||
                      x > (pDropWnd->rectClient.right - pDropWnd->rectWindow.left) ||
                      y > (pDropWnd->rectClient.bottom - pDropWnd->rectWindow.top) );
                lpDrop->fWide = FALSE;
                WIN_ReleaseWndPtr(pDropWnd);
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
    if( p_data ) TSXFree(p_data);
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

  TSXGetWindowProperty( event->display, DefaultRootWindow(event->display),
			dndSelection, 0, 65535, FALSE,
			AnyPropertyType, &u.atom_aux, &u.i,
			&data_length, &aux_long, &p_data);
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
      TSXQueryPointer( event->display, root_window, &u.w_aux, &u.w_aux,
		       &x, &y, &u.i, &u.i, &u.i);

      drop_len += sizeof(DROPFILES) + 1;
      hDrop = GlobalAlloc( GMEM_SHARE, drop_len );
      lpDrop = (DROPFILES *) GlobalLock( hDrop );

      if( lpDrop ) {
          WND *pDropWnd = WIN_FindWndPtr( hWnd );
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
          WIN_ReleaseWndPtr(pDropWnd);
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
    if( p_data ) TSXFree(p_data);
  }
}

/**********************************************************************
 *           EVENT_ClientMessage
 */
static void EVENT_ClientMessage( HWND hWnd, XClientMessageEvent *event )
{
  if (event->message_type != None && event->format == 32) {
    if (event->message_type == wmProtocols)
        handle_wm_protocols_message( hWnd, event );
    else if (event->message_type == dndProtocol)
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
    else {
#if 0
      /* enable this if you want to see the message */
      unsigned char* p_data = NULL;
      union {
	unsigned long	l;
	int            	i;
	Atom		atom;
      } u; /* unused */
      TSXGetWindowProperty( event->display, DefaultRootWindow(event->display),
			    dndSelection, 0, 65535, FALSE,
			    AnyPropertyType, &u.atom, &u.i,
			    &u.l, &u.l, &p_data);
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
