/*
 * X11 event driver
 *
 * Copyright 1993 Alexandre Julliard
 *	     1999 Noel Borthwick
 */

#include "config.h"

#include <X11/Xatom.h>
#include <X11/keysym.h>

#include "ts_xlib.h"
#include "ts_xresource.h"
#include "ts_xutil.h"
#ifdef HAVE_LIBXXF86DGA2
#include "ts_xf86dga2.h"
#endif

#include <assert.h>
#include <string.h>
#include "wine/winuser16.h"
#include "shlobj.h"  /* DROPFILES */

#include "clipboard.h"
#include "dce.h"
#include "debugtools.h"
#include "heap.h"
#include "input.h"
#include "keyboard.h"
#include "message.h"
#include "mouse.h"
#include "options.h"
#include "queue.h"
#include "win.h"
#include "winpos.h"
#include "file.h"
#include "windef.h"
#include "x11drv.h"
#include "shellapi.h"

DEFAULT_DEBUG_CHANNEL(event);
DECLARE_DEBUG_CHANNEL(win);
  
/* X context to associate a hwnd to an X window */
extern XContext winContext;

extern Atom wmProtocols;
extern Atom wmDeleteWindow;
extern Atom dndProtocol;
extern Atom dndSelection;

extern void X11DRV_KEYBOARD_UpdateState(void);
extern void X11DRV_KEYBOARD_HandleEvent( XKeyEvent *event, int x, int y );

#define NB_BUTTONS      5     /* Windows can handle 3 buttons and the wheel too */


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

/* The last X window which had the focus */
static Window glastXFocusWin = 0;

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
static BOOL X11DRV_CheckFocus(void);

  /* Event handlers */
static void EVENT_Key( HWND hWnd, XKeyEvent *event );
static void EVENT_ButtonPress( HWND hWnd, XButtonEvent *event );
static void EVENT_ButtonRelease( HWND hWnd, XButtonEvent *event );
static void EVENT_MotionNotify( HWND hWnd, XMotionEvent *event );
static void EVENT_FocusIn( HWND hWnd, XFocusChangeEvent *event );
static void EVENT_FocusOut( HWND hWnd, XFocusChangeEvent *event );
static void EVENT_SelectionRequest( HWND hWnd, XSelectionRequestEvent *event, BOOL bIsMultiple );
static void EVENT_SelectionClear( HWND hWnd, XSelectionClearEvent *event);
static void EVENT_PropertyNotify( XPropertyEvent *event );
static void EVENT_ClientMessage( HWND hWnd, XClientMessageEvent *event );
static void EVENT_MappingNotify( XMappingEvent *event );

extern void X11DRV_Expose( HWND hwnd, XExposeEvent *event );
extern void X11DRV_MapNotify( HWND hwnd, XMapEvent *event );
extern void X11DRV_UnmapNotify( HWND hwnd, XUnmapEvent *event );
extern void X11DRV_ConfigureNotify( HWND hwnd, XConfigureEvent *event );

#ifdef HAVE_LIBXXF86DGA2
static int DGAMotionEventType;
static int DGAButtonPressEventType;
static int DGAButtonReleaseEventType;
static int DGAKeyPressEventType;
static int DGAKeyReleaseEventType;

static BOOL DGAUsed = FALSE;
static HWND DGAhwnd = 0;

static void EVENT_DGAMotionEvent( XDGAMotionEvent *event );
static void EVENT_DGAButtonPressEvent( XDGAButtonEvent *event );
static void EVENT_DGAButtonReleaseEvent( XDGAButtonEvent *event );
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
      EVENT_DGAMotionEvent((XDGAMotionEvent *) event);
      return;
    }
    if (event->type == DGAButtonPressEventType) {
      TRACE("DGAButtonPressEvent received.\n");
      EVENT_DGAButtonPressEvent((XDGAButtonEvent *) event);
      return;
    }
    if (event->type == DGAButtonReleaseEventType) {
      TRACE("DGAButtonReleaseEvent received.\n");
      EVENT_DGAButtonReleaseEvent((XDGAButtonEvent *) event);
      return;
    }
    if ((event->type == DGAKeyPressEventType) ||
	(event->type == DGAKeyReleaseEventType)) {
      /* Fill a XKeyEvent to send to EVENT_Key */
      POINT pt;
      XKeyEvent ke;
      XDGAKeyEvent *evt = (XDGAKeyEvent *) event;

      TRACE("DGAKeyPress/ReleaseEvent received.\n");
      
      GetCursorPos( &pt );
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
      ke.x = pt.x;
      ke.y = pt.y;
      ke.x_root = -1;
      ke.y_root = -1;
      ke.state = evt->state;
      ke.keycode = evt->keycode;
      ke.same_screen = TRUE;
      
      X11DRV_KEYBOARD_HandleEvent(&ke,pt.x,pt.y);
      return;
    }
  }
#endif

  if (TSXFindContext( display, event->xany.window, winContext, (char **)&hWnd ) != 0)
      hWnd = 0;  /* Not for a registered window */

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
      EVENT_Key( hWnd, (XKeyEvent*)event );
      break;
      
    case ButtonPress:
      EVENT_ButtonPress( hWnd, (XButtonEvent*)event );
      break;
      
    case ButtonRelease:
      EVENT_ButtonRelease( hWnd, (XButtonEvent*)event );
      break;
      
    case MotionNotify:
      /* Wine between two fast machines across the overloaded campus
	 ethernet gets very boged down in MotionEvents. The following
	 simply finds the last motion event in the queue and drops
	 the rest. On a good link events are servered before they build
	 up so this doesn't take place. On a slow link this may cause
	 problems if the event order is important. I'm not yet seen
	 of any problems. Jon 7/6/96.
      */
      if ((current_input_type == X11DRV_INPUT_ABSOLUTE) &&
	  (in_transition == FALSE))
	/* Only cumulate events if in absolute mode */
	while (TSXCheckTypedWindowEvent(display,((XAnyEvent *)event)->window,
					MotionNotify, event));    
      EVENT_MotionNotify( hWnd, (XMotionEvent*)event );
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

    case MappingNotify:
      EVENT_MappingNotify( (XMappingEvent *) event );
      break;

    default:    
      WARN("Unprocessed event %s for hwnd %04x\n",
	   event_names[event->type], hWnd );
      break;
    }
    TRACE( "returns.\n" );
}

/***********************************************************************
 *           X11DRV_EVENT_XStateToKeyState
 *
 * Translate a X event state (Button1Mask, ShiftMask, etc...) to
 * a Windows key state (MK_SHIFT, MK_CONTROL, etc...)
 */
WORD X11DRV_EVENT_XStateToKeyState( int state )
{
  int kstate = 0;
  
  if (state & Button1Mask) kstate |= MK_LBUTTON;
  if (state & Button2Mask) kstate |= MK_MBUTTON;
  if (state & Button3Mask) kstate |= MK_RBUTTON;
  if (state & ShiftMask)   kstate |= MK_SHIFT;
  if (state & ControlMask) kstate |= MK_CONTROL;
  return kstate;
}


/* get the coordinates of a mouse event */
static void get_coords( HWND *hwnd, Window window, int x, int y, POINT *pt )
{
    struct x11drv_win_data *data;
    WND *win;

    if (!(win = WIN_FindWndPtr( *hwnd ))) return;
    data = win->pDriverData;

    if (window == data->whole_window)
    {
        x -= data->client_rect.left;
        y -= data->client_rect.top;
    }
    while (win->parent && win->parent->hwndSelf != GetDesktopWindow())
    {
        x += win->rectClient.left;
        y += win->rectClient.top;
        WIN_UpdateWndPtr( &win, win->parent );
    }
    pt->x = x + win->rectClient.left;
    pt->y = y + win->rectClient.top;
    *hwnd = win->hwndSelf;
    WIN_ReleaseWndPtr( win );
}


/***********************************************************************
 *           EVENT_Key
 *
 * Handle a X key event
 */
static void EVENT_Key( HWND hWnd, XKeyEvent *event )
{
    POINT pt;
    get_coords( &hWnd, event->window, event->x, event->y, &pt );
    X11DRV_KEYBOARD_HandleEvent( event, pt.x, pt.y );
}


/***********************************************************************
 *           EVENT_MotionNotify
 */
static void EVENT_MotionNotify( HWND hWnd, XMotionEvent *event )
{
    POINT pt;

    if (current_input_type == X11DRV_INPUT_ABSOLUTE)
    {
        get_coords( &hWnd, event->window, event->x, event->y, &pt );
        X11DRV_SendEvent( MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE, pt.x, pt.y,
                          X11DRV_EVENT_XStateToKeyState( event->state ),
                          event->time - X11DRV_server_startticks, hWnd);
    }
    else
    {
        X11DRV_SendEvent( MOUSEEVENTF_MOVE,
                          event->x_root, event->y_root,
                          X11DRV_EVENT_XStateToKeyState( event->state ),
                          event->time - X11DRV_server_startticks, hWnd);
    }
}


/***********************************************************************
 *           EVENT_ButtonPress
 */
static void EVENT_ButtonPress( HWND hWnd, XButtonEvent *event )
{
    static const WORD statusCodes[NB_BUTTONS] = { MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_MIDDLEDOWN,
                                                  MOUSEEVENTF_RIGHTDOWN, MOUSEEVENTF_WHEEL,
                                                  MOUSEEVENTF_WHEEL};
    int buttonNum = event->button - 1;
    WORD keystate, wData = 0;
    POINT pt;

    if (buttonNum >= NB_BUTTONS) return;

    get_coords( &hWnd, event->window, event->x, event->y, &pt );

    /* Get the compatible keystate */
    keystate = X11DRV_EVENT_XStateToKeyState( event->state );

    /*
     * Make sure that the state of the button that was just 
     * pressed is "down".
     */
    switch (buttonNum)
    {
    case 0:
        keystate |= MK_LBUTTON;
        break;
    case 1:
        keystate |= MK_MBUTTON;
        break;
    case 2:
        keystate |= MK_RBUTTON;
        break;
    case 3:
        wData = WHEEL_DELTA;
        break;
    case 4:
        wData = -WHEEL_DELTA;
        break;
    }

    X11DRV_SendEvent( statusCodes[buttonNum], pt.x, pt.y,
                      MAKEWPARAM(keystate,wData),
                      event->time - X11DRV_server_startticks, hWnd);
}


/***********************************************************************
 *           EVENT_ButtonRelease
 */
static void EVENT_ButtonRelease( HWND hWnd, XButtonEvent *event )
{
    static const WORD statusCodes[NB_BUTTONS] = { MOUSEEVENTF_LEFTUP, MOUSEEVENTF_MIDDLEUP,
                                                  MOUSEEVENTF_RIGHTUP, 0, 0 };
    int buttonNum = event->button - 1;
    WORD keystate;
    POINT pt;

    if (buttonNum >= NB_BUTTONS) return;

    get_coords( &hWnd, event->window, event->x, event->y, &pt );

    /* Get the compatible keystate */
    keystate = X11DRV_EVENT_XStateToKeyState( event->state );

    /*
     * Make sure that the state of the button that was just 
     * released is "up".
     */
    switch (buttonNum)
    {
    case 0:
        keystate &= ~MK_LBUTTON;
        break;
    case 1:
        keystate &= ~MK_MBUTTON;
        break;
    case 2:
        keystate &= ~MK_RBUTTON;
        break;
    default:
        return;
    }
    X11DRV_SendEvent( statusCodes[buttonNum], pt.x, pt.y,
                      keystate, event->time - X11DRV_server_startticks, hWnd);
}


/**********************************************************************
 *              EVENT_FocusIn
 */
static void EVENT_FocusIn( HWND hWnd, XFocusChangeEvent *event )
{
    WND *pWndLastFocus;
    XWindowAttributes win_attr;
    BOOL bIsDisabled;

    if (!hWnd) return;

    bIsDisabled = GetWindowLongA( hWnd, GWL_STYLE ) & WS_DISABLED;

    /* If the window has been disabled and we are in managed mode,
       * revert the X focus back to the last focus window. This is to disallow
       * the window manager from switching focus away while the app is
       * in a modal state.
       */
    if ( Options.managed && bIsDisabled && glastXFocusWin)
    {
        /* Change focus only if saved focus window is registered and viewable */
        wine_tsx11_lock();
        if (XFindContext( event->display, glastXFocusWin, winContext,
                           (char **)&pWndLastFocus ) == 0 )
        {
            if (XGetWindowAttributes( event->display, glastXFocusWin, &win_attr ) &&
                (win_attr.map_state == IsViewable) )
            {
                XSetInputFocus( event->display, glastXFocusWin, RevertToParent, CurrentTime );
                wine_tsx11_unlock();
                return;
            }
        }
        wine_tsx11_unlock();
    }

    if (event->detail != NotifyPointer && hWnd != GetForegroundWindow())
    {
        SetForegroundWindow( hWnd );
        X11DRV_KEYBOARD_UpdateState();
    }
}


/**********************************************************************
 *              EVENT_FocusOut
 *
 * Note: only top-level override-redirect windows get FocusOut events.
 */
static void EVENT_FocusOut( HWND hWnd, XFocusChangeEvent *event )
{
    /* Save the last window which had the focus */
    glastXFocusWin = event->window;
    if (!hWnd) return;
    if (GetWindowLongA( hWnd, GWL_STYLE ) & WS_DISABLED) glastXFocusWin = 0;

    if (event->detail != NotifyPointer && hWnd == GetForegroundWindow())
    {
        /* don't reset the foreground window, if the window which is
               getting the focus is a Wine window */
        if (!X11DRV_CheckFocus())
        {
            SendMessageA( hWnd, WM_CANCELMODE, 0, 0 );
            /* Abey : 6-Oct-99. Check again if the focus out window is the
               Foreground window, because in most cases the messages sent
               above must have already changed the foreground window, in which
               case we don't have to change the foreground window to 0 */

            if (hWnd == GetForegroundWindow())
                SetForegroundWindow( 0 );
        }
    }
}

/**********************************************************************
 *		CheckFocus (X11DRV.@)
 */
static BOOL X11DRV_CheckFocus(void)
{
    Display *display = thread_display();
  HWND   hWnd;
  Window xW;
  int	   state;
  
  TSXGetInputFocus(display, &xW, &state);
    if( xW == None ||
        TSXFindContext(display, xW, winContext, (char **)&hWnd) ) 
      return FALSE;
    return TRUE;
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
	text_cp = PROFILE_GetWineIniInt("x11drv", "TextCP", CP_ACP);

    /*
     * Map the requested X selection property type atom name to a
     * windows clipboard format ID.
     */
    itemFmtName = TSXGetAtomName(display, target);
    TRACE("Request for %s (wFormat=%x %s)\n",
	itemFmtName, CF_UNICODETEXT, CLIPBOARD_GetFormatName(CF_UNICODETEXT));
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
                  itemFmtName, wFormat, CLIPBOARD_GetFormatName( wFormat));
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
        
        ReleaseDC(hdc, hwnd);
    }
    else if (wFormat == CF_BITMAP)
    {
        HWND hwnd = GetOpenClipboardWindow();
        HDC hdc = GetDC(hwnd);
        
        pixmap = X11DRV_BITMAP_CreatePixmapFromBitmap(hClipData, hdc);

        ReleaseDC(hdc, hwnd);
    }
    else
    {
        FIXME("%s to PIXMAP conversion not yet implemented!\n",
                      CLIPBOARD_GetFormatName(wFormat));
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
    
    /*
     * Map the requested X selection property type atom name to a
     * windows clipboard format ID.
     */
    itemFmtName = TSXGetAtomName(display, target);
    wFormat = X11DRV_CLIPBOARD_MapPropertyToFormat(itemFmtName);
    TRACE("Request for %s (wFormat=%x %s)\n",
          itemFmtName, wFormat, CLIPBOARD_GetFormatName( wFormat));
    TSXFree(itemFmtName);
    
    hClipData = GetClipboardData16(wFormat);
    
    if( hClipData && (lpClipData = GlobalLock16(hClipData)) )
    {
        cBytes = GlobalSize16(hClipData);
        
        TRACE("\tUpdating property %s, %d bytes...\n",
              TSXGetAtomName(display, rprop), cBytes);
        
        xRc = TSXChangeProperty(display, requestor, rprop,
                                target, 8, PropModeReplace,
                                (unsigned char *)lpClipData, cBytes);
        TRACE("(Rc=%d)\n", xRc);
        
        GlobalUnlock16(hClipData);
    }
    else
    {
        TRACE("\tCould not retrieve native format!\n");
        rprop = None; /* Fail the request */
    }
    
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

/**********************************************************************
 *           EVENT_DropFromOffix
 *
 * don't know if it still works (last Changlog is from 96/11/04)
 */
static void EVENT_DropFromOffiX( HWND hWnd, XClientMessageEvent *event )
{
  unsigned long		data_length;
  unsigned long		aux_long;
  unsigned char*	p_data = NULL;
  union {
    Atom		atom_aux;
    struct {
      int x;
      int y;
    } pt_aux;
    int		i;
  }		u;
  int			x, y;
  BOOL16	        bAccept;
  HGLOBAL16		hDragInfo = GlobalAlloc16( GMEM_SHARE | GMEM_ZEROINIT, sizeof(DRAGINFO16));
  LPDRAGINFO16          lpDragInfo = (LPDRAGINFO16) GlobalLock16(hDragInfo);
  SEGPTR		spDragInfo = K32WOWGlobalLock16(hDragInfo);
  Window		w_aux_root, w_aux_child;
  WND*			pDropWnd;
  WND*                  pWnd;
  
  if( !lpDragInfo || !spDragInfo ) return;
  
  pWnd = WIN_FindWndPtr(hWnd);
  
  TSXQueryPointer( event->display, get_whole_window(pWnd), &w_aux_root, &w_aux_child,
                   &x, &y, (int *) &u.pt_aux.x, (int *) &u.pt_aux.y,
                   (unsigned int*)&aux_long);
  
  lpDragInfo->hScope = hWnd;
  lpDragInfo->pt.x = (INT16)x; lpDragInfo->pt.y = (INT16)y;
  
  /* find out drop point and drop window */
  if( x < 0 || y < 0 ||
      x > (pWnd->rectWindow.right - pWnd->rectWindow.left) ||
      y > (pWnd->rectWindow.bottom - pWnd->rectWindow.top) )
    {   bAccept = pWnd->dwExStyle & WS_EX_ACCEPTFILES; x = y = 0; }
  else
    {
      bAccept = DRAG_QueryUpdate( hWnd, spDragInfo, TRUE );
      x = lpDragInfo->pt.x; y = lpDragInfo->pt.y;
    }
  pDropWnd = WIN_FindWndPtr( lpDragInfo->hScope );
  WIN_ReleaseWndPtr(pWnd);
  
  GlobalFree16( hDragInfo );
  
  if( bAccept )
    {
      TSXGetWindowProperty( event->display, DefaultRootWindow(event->display),
			    dndSelection, 0, 65535, FALSE,
			    AnyPropertyType, &u.atom_aux, (int *) &u.pt_aux.y,
			    &data_length, &aux_long, &p_data);
      
      if( !aux_long && p_data)	/* don't bother if > 64K */
	{
	  signed char *p = (signed char*) p_data;
	  char *p_drop;
	  
	  aux_long = 0; 
	  while( *p )	/* calculate buffer size */
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
		  lpDrop->pFiles = sizeof(DROPFILES);
		  lpDrop->pt.x = x;
		  lpDrop->pt.y = y;
		  lpDrop->fNC =
		    ( x < (pDropWnd->rectClient.left - pDropWnd->rectWindow.left)  ||
		      y < (pDropWnd->rectClient.top - pDropWnd->rectWindow.top)    ||
		      x > (pDropWnd->rectClient.right - pDropWnd->rectWindow.left) ||
		      y > (pDropWnd->rectClient.bottom - pDropWnd->rectWindow.top) );
                  lpDrop->fWide = FALSE;
		  p_drop = (char *)(lpDrop + 1);
		  p = p_data;
		  while(*p)
		    {
		      if( *p != -1 )	/* use only "good" entries */
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
      
    } /* WS_EX_ACCEPTFILES */

  WIN_ReleaseWndPtr(pDropWnd);
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
  WND           *pDropWnd;
  WND           *pWnd;
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

  pWnd = WIN_FindWndPtr(hWnd);

  if (!(pWnd->dwExStyle & WS_EX_ACCEPTFILES))
  {
    WIN_ReleaseWndPtr(pWnd);
    return;
  }
  WIN_ReleaseWndPtr(pWnd);

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

      pDropWnd = WIN_FindWndPtr( hWnd );
      
      drop_len += sizeof(DROPFILES) + 1;
      hDrop = GlobalAlloc( GMEM_SHARE, drop_len );
      lpDrop = (DROPFILES *) GlobalLock( hDrop );

      if( lpDrop ) {
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
      WIN_ReleaseWndPtr(pDropWnd);
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
    if ((event->message_type == wmProtocols) && 
	(((Atom) event->data.l[0]) == wmDeleteWindow))
    {
        /* Ignore the delete window request if the window has been disabled */
        if (!(GetWindowLongA( hWnd, GWL_STYLE ) & WS_DISABLED))
            PostMessageA( hWnd, WM_SYSCOMMAND, SC_CLOSE, 0 );
    }
    else if (event->message_type == dndProtocol)
    {
        /* query window (drag&drop event contains only drag window) */
        Window root, child;
        int root_x, root_y, child_x, child_y;
        unsigned int u;
        TSXQueryPointer( event->display, root_window, &root, &child,
                         &root_x, &root_y, &child_x, &child_y, &u);
        if (TSXFindContext( event->display, child, winContext, (char **)&hWnd ) != 0) return;
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


/***********************************************************************
 *           EVENT_MappingNotify
 */
static void EVENT_MappingNotify( XMappingEvent *event )
{
    TSXRefreshKeyboardMapping(event);

    /* reinitialize Wine-X11 driver keyboard table */
    X11DRV_InitKeyboard();
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

/* DGA2 event handlers */
static void EVENT_DGAMotionEvent( XDGAMotionEvent *event )
{
  X11DRV_SendEvent( MOUSEEVENTF_MOVE, 
		   event->dx, event->dy,
		   X11DRV_EVENT_XStateToKeyState( event->state ), 
		   event->time - X11DRV_server_startticks, DGAhwnd );
}

static void EVENT_DGAButtonPressEvent( XDGAButtonEvent *event )
{
  static WORD statusCodes[NB_BUTTONS] = 
    { MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_MIDDLEDOWN, MOUSEEVENTF_RIGHTDOWN };
  int buttonNum = event->button - 1;
  
  WORD keystate;

  if (buttonNum >= NB_BUTTONS) return;
  
  keystate = X11DRV_EVENT_XStateToKeyState( event->state );
  
  switch (buttonNum)
  {
    case 0:
      keystate |= MK_LBUTTON;
      break;
    case 1:
      keystate |= MK_MBUTTON;
      break;
    case 2:
      keystate |= MK_RBUTTON;
      break;
  }
  
  X11DRV_SendEvent( statusCodes[buttonNum], 0, 0, keystate, event->time - X11DRV_server_startticks, DGAhwnd );
}

static void EVENT_DGAButtonReleaseEvent( XDGAButtonEvent *event )
{
  static WORD statusCodes[NB_BUTTONS] = 
    { MOUSEEVENTF_LEFTUP, MOUSEEVENTF_MIDDLEUP, MOUSEEVENTF_RIGHTUP };
  int buttonNum = event->button - 1;
  
  WORD keystate;

  if (buttonNum >= NB_BUTTONS) return;
  
  keystate = X11DRV_EVENT_XStateToKeyState( event->state );
  
  switch (buttonNum)
  {
    case 0:
      keystate &= ~MK_LBUTTON;
      break;
    case 1:
      keystate &= ~MK_MBUTTON;
      break;
    case 2:
      keystate &= ~MK_RBUTTON;
      break;
  }
  
  X11DRV_SendEvent( statusCodes[buttonNum], 0, 0, keystate, event->time - X11DRV_server_startticks, DGAhwnd );
}

#endif
