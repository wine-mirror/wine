/*
 * X11 event driver
 *
 * Copyright 1993 Alexandre Julliard
 *	     1999 Noel Borthwick
 */

#include "config.h"

#ifndef X_DISPLAY_MISSING

#include <X11/Xatom.h>
#include <X11/keysym.h>
#include "ts_xlib.h"
#include "ts_xresource.h"
#include "ts_xutil.h"
#ifdef HAVE_LIBXXSHM
#include "ts_xshm.h"
#endif

#ifdef HAVE_LIBXXF86DGA2
#include "ts_xf86dga2.h"
#endif

#include <assert.h>
#include <string.h>
#include "callback.h"
#include "clipboard.h"
#include "dce.h"
#include "debugtools.h"
#include "drive.h"
#include "heap.h"
#include "input.h"
#include "keyboard.h"
#include "message.h"
#include "mouse.h"
#include "options.h"
#include "queue.h"
#include "shell.h"
#include "winpos.h"
#include "services.h"
#include "file.h"
#include "windef.h"
#include "x11drv.h"

DEFAULT_DEBUG_CHANNEL(event)
DECLARE_DEBUG_CHANNEL(win)
  
/* X context to associate a hwnd to an X window */
extern XContext winContext;

extern Atom wmProtocols;
extern Atom wmDeleteWindow;
extern Atom dndProtocol;
extern Atom dndSelection;

extern void X11DRV_KEYBOARD_UpdateState(void);
extern void X11DRV_KEYBOARD_HandleEvent(WND *pWnd, XKeyEvent *event);

#define NB_BUTTONS      3     /* Windows can handle 3 buttons */

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


static void CALLBACK EVENT_Flush( ULONG_PTR arg );
static void CALLBACK EVENT_ProcessAllEvents( ULONG_PTR arg );
static void EVENT_ProcessEvent( XEvent *event );

  /* Event handlers */
static void EVENT_Key( HWND hWnd, XKeyEvent *event );
static void EVENT_ButtonPress( HWND hWnd, XButtonEvent *event );
static void EVENT_ButtonRelease( HWND hWnd, XButtonEvent *event );
static void EVENT_MotionNotify( HWND hWnd, XMotionEvent *event );
static void EVENT_FocusIn( HWND hWnd, XFocusChangeEvent *event );
static void EVENT_FocusOut( HWND hWnd, XFocusChangeEvent *event );
static void EVENT_Expose( HWND hWnd, XExposeEvent *event );
static void EVENT_GraphicsExpose( HWND hWnd, XGraphicsExposeEvent *event );
static void EVENT_ConfigureNotify( HWND hWnd, XConfigureEvent *event );
static void EVENT_SelectionRequest( HWND hWnd, XSelectionRequestEvent *event, BOOL bIsMultiple );
static void EVENT_SelectionClear( HWND hWnd, XSelectionClearEvent *event);
static void EVENT_PropertyNotify( XPropertyEvent *event );
static void EVENT_ClientMessage( HWND hWnd, XClientMessageEvent *event );
static void EVENT_MapNotify( HWND pWnd, XMapEvent *event );
static void EVENT_UnmapNotify( HWND pWnd, XUnmapEvent *event );

#ifdef HAVE_LIBXXSHM
static void EVENT_ShmCompletion( XShmCompletionEvent *event );
static int ShmAvailable, ShmCompletionType;
extern int XShmGetEventBase( Display * );/* Missing prototype for function in libXext. */
#endif

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

/* Usable only with OLVWM - compile option perhaps?
static void EVENT_EnterNotify( HWND hWnd, XCrossingEvent *event );
*/

static void EVENT_GetGeometry( Window win, int *px, int *py,
                               unsigned int *pwidth, unsigned int *pheight );


static BOOL bUserRepaintDisabled = TRUE;

/* Static used for the current input method */
static INPUT_TYPE current_input_type = X11DRV_INPUT_ABSOLUTE;
static BOOL in_transition = FALSE; /* This is not used as for today */

/***********************************************************************
 *           EVENT_Init
 */
BOOL X11DRV_EVENT_Init(void)
{
#ifdef HAVE_LIBXXSHM
    ShmAvailable = XShmQueryExtension( display );
    if (ShmAvailable) {
        ShmCompletionType = XShmGetEventBase( display ) + ShmCompletion;
    }
#endif

    /* Install the X event processing callback */
    SERVICE_AddObject( FILE_DupUnixHandle( ConnectionNumber(display), 
                                           GENERIC_READ | SYNCHRONIZE ),
                       EVENT_ProcessAllEvents, 0 );

    /* Install the XFlush timer callback */
    if ( Options.synchronous ) 
        TSXSynchronize( display, True );
    else
        SERVICE_AddTimer( 200000L, EVENT_Flush, 0 );

    return TRUE;
}

/***********************************************************************
 *           EVENT_Flush
 */
static void CALLBACK EVENT_Flush( ULONG_PTR arg )
{
    TSXFlush( display );
}

/***********************************************************************
 *           EVENT_ProcessAllEvents
 */
static void CALLBACK EVENT_ProcessAllEvents( ULONG_PTR arg )
{
    XEvent event;
  
    TRACE( "called (thread %lx).\n", GetCurrentThreadId() );

    EnterCriticalSection( &X11DRV_CritSection );
    while ( XPending( display ) )
    {
        XNextEvent( display, &event );
      
        LeaveCriticalSection( &X11DRV_CritSection );
        EVENT_ProcessEvent( &event );
        EnterCriticalSection( &X11DRV_CritSection );
    }
    LeaveCriticalSection( &X11DRV_CritSection );
}

/***********************************************************************
 *           EVENT_Synchronize
 *
 * Synchronize with the X server. Should not be used too often.
 */
void X11DRV_EVENT_Synchronize( void )
{
    TSXSync( display, False );
    EVENT_ProcessAllEvents( 0 );
}

/***********************************************************************
 *           EVENT_UserRepaintDisable
 */
void X11DRV_EVENT_UserRepaintDisable( BOOL bDisabled )
{
    bUserRepaintDisabled = bDisabled;
}

/***********************************************************************
 *           EVENT_ProcessEvent
 *
 * Process an X event.
 */
static void EVENT_ProcessEvent( XEvent *event )
{
  HWND hWnd;

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

#ifdef HAVE_LIBXXSHM
  if (ShmAvailable && (event->type == ShmCompletionType)) {
    EVENT_ShmCompletion( (XShmCompletionEvent*)event );
    return;
  }
#endif
      
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
      ke.x = PosX;
      ke.y = PosY;
      ke.x_root = -1;
      ke.y_root = -1;
      ke.state = evt->state;
      ke.keycode = evt->keycode;
      ke.same_screen = TRUE;
      
      X11DRV_KEYBOARD_HandleEvent(NULL, &ke);
      return;
    }
  }
#endif
  
  if ( TSXFindContext( display, event->xany.window, winContext,
		       (char **)&hWnd ) != 0) {
    if ( event->type == ClientMessage) {
      /* query window (drag&drop event contains only drag window) */
      Window   	root, child;
      int      	root_x, root_y, child_x, child_y;
      unsigned	u;
      TSXQueryPointer( display, X11DRV_GetXRootWindow(), &root, &child,
		       &root_x, &root_y, &child_x, &child_y, &u);
      if (TSXFindContext( display, child, winContext, (char **)&hWnd ) != 0)
	return;
    } else {
      hWnd = 0;  /* Not for a registered window */
    }
  }

  if ( !hWnd && event->xany.window != X11DRV_GetXRootWindow()
             && event->type != PropertyNotify )
      ERR("Got event %s for unknown Window %08lx\n",
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
    {
      WND *pWndLastFocus = 0;
      XWindowAttributes win_attr;
      BOOL bIsDisabled;
      XFocusChangeEvent *xfocChange = (XFocusChangeEvent*)event;

      if (!hWnd || bUserRepaintDisabled) return;

      bIsDisabled = GetWindowLongA( hWnd, GWL_STYLE ) & WS_DISABLED;

      /* If the window has been disabled and we are in managed mode,
       * revert the X focus back to the last focus window. This is to disallow
       * the window manager from switching focus away while the app is
       * in a modal state.
       */
      if ( Options.managed && bIsDisabled && glastXFocusWin)
      {
        /* Change focus only if saved focus window is registered and viewable */
        if ( TSXFindContext( xfocChange->display, glastXFocusWin, winContext,
                             (char **)&pWndLastFocus ) == 0 )
        {
          if ( TSXGetWindowAttributes( display, glastXFocusWin, &win_attr ) &&
                 (win_attr.map_state == IsViewable) )
          {
            TSXSetInputFocus( xfocChange->display, glastXFocusWin, RevertToParent, CurrentTime );
            EVENT_Synchronize();
      break;
          }
        }
      }
       
      EVENT_FocusIn( hWnd, xfocChange );
      break;
    }
      
    case FocusOut:
    {
      /* Save the last window which had the focus */
      XFocusChangeEvent *xfocChange = (XFocusChangeEvent*)event;
      glastXFocusWin = xfocChange->window;
      if (!hWnd || bUserRepaintDisabled) return;
      if (GetWindowLongA( hWnd, GWL_STYLE ) & WS_DISABLED) glastXFocusWin = 0;
      EVENT_FocusOut( hWnd, (XFocusChangeEvent*)event );
      break;
    }
      
    case Expose:
      if (bUserRepaintDisabled) return;
      EVENT_Expose( hWnd, (XExposeEvent *)event );
      break;
      
    case GraphicsExpose:
      if (bUserRepaintDisabled) return;
      EVENT_GraphicsExpose( hWnd, (XGraphicsExposeEvent *)event );
      break;
      
    case ConfigureNotify:
      if (!hWnd || bUserRepaintDisabled) return;
      EVENT_ConfigureNotify( hWnd, (XConfigureEvent*)event );
      break;

    case SelectionRequest:
      if (!hWnd || bUserRepaintDisabled) return;
      EVENT_SelectionRequest( hWnd, (XSelectionRequestEvent *)event, FALSE );
      break;

    case SelectionClear:
      if (!hWnd || bUserRepaintDisabled) return;
      EVENT_SelectionClear( hWnd, (XSelectionClearEvent*) event );
      break;
      
    case PropertyNotify:
      EVENT_PropertyNotify( (XPropertyEvent *)event );
      break;

    case ClientMessage:
      if (!hWnd || bUserRepaintDisabled) return;
      EVENT_ClientMessage( hWnd, (XClientMessageEvent *) event );
      break;

#if 0
    case EnterNotify:
      EVENT_EnterNotify( hWnd, (XCrossingEvent *) event );
      break;
#endif

    case NoExpose:
      break;
      
    case MapNotify:
      if (!hWnd || bUserRepaintDisabled) return;
      EVENT_MapNotify( hWnd, (XMapEvent *)event );
      break;

    case UnmapNotify:
      if (!hWnd || bUserRepaintDisabled) return;
      EVENT_UnmapNotify( hWnd, (XUnmapEvent *)event );
      break;

    default:    
      WARN("Unprocessed event %s for hwnd %04x\n",
	   event_names[event->type], hWnd );
      break;
    }
    TRACE( "returns.\n" );
}

/***********************************************************************
 *           EVENT_QueryZOrder
 *
 * Synchronize internal z-order with the window manager's.
 */
static BOOL __check_query_condition( WND** pWndA, WND** pWndB )
{
  /* return TRUE if we have at least two managed windows */
  
  for( *pWndB = NULL; *pWndA; *pWndA = (*pWndA)->next )
    if( (*pWndA)->flags & WIN_MANAGED &&
	(*pWndA)->dwStyle & WS_VISIBLE ) break;
  if( *pWndA )
    for( *pWndB = (*pWndA)->next; *pWndB; *pWndB = (*pWndB)->next )
      if( (*pWndB)->flags & WIN_MANAGED &&
	  (*pWndB)->dwStyle & WS_VISIBLE ) break;
  return ((*pWndB) != NULL);
}

static Window __get_common_ancestor( Window A, Window B,
                                     Window** children, unsigned* total )
{
    /* find the real root window */
  
    Window      root, *childrenB;
    unsigned    totalB;
  
    do
    {
      TSXQueryTree( display, A, &root, &A, children, total );
      TSXQueryTree( display, B, &root, &B, &childrenB, &totalB );
      if( childrenB ) TSXFree( childrenB );
      if( *children ) TSXFree( *children ), *children = NULL;
    } while( A != B && A && B );

    if( A && B )
    {
	TSXQueryTree( display, A, &root, &B, children, total );
	return A;
    }
    return 0 ;
}

static Window __get_top_decoration( Window w, Window ancestor )
{
  Window*     children, root, prev = w, parent = w;
  unsigned    total;
  
  do
    {
      w = parent;
      TSXQueryTree( display, w, &root, &parent, &children, &total );
      if( children ) TSXFree( children );
    } while( parent && parent != ancestor );
  TRACE("\t%08x -> %08x\n", (unsigned)prev, (unsigned)w );
  return ( parent ) ? w : 0 ;
}

static unsigned __td_lookup( Window w, Window* list, unsigned max )
{
  unsigned    i;
  for( i = max - 1; i >= 0; i-- ) if( list[i] == w ) break;
  return i;
}

static HWND EVENT_QueryZOrder( HWND hWndCheck)
{
  HWND      hwndInsertAfter = HWND_TOP;
  WND      *pWndCheck = WIN_FindWndPtr(hWndCheck);
  WND      *pDesktop = WIN_GetDesktop();
  WND      *pWnd, *pWndZ = WIN_LockWndPtr(pDesktop->child);
  Window      w, parent, *children = NULL;
  unsigned    total, check, pos, best;
  
  if( !__check_query_condition(&pWndZ, &pWnd) )
  {
      WIN_ReleaseWndPtr(pWndCheck);
      WIN_ReleaseWndPtr(pDesktop->child);
      WIN_ReleaseDesktop();
      return hwndInsertAfter;
  }
  WIN_LockWndPtr(pWndZ);
  WIN_LockWndPtr(pWnd);
  WIN_ReleaseWndPtr(pDesktop->child);
  WIN_ReleaseDesktop();
  
  parent = __get_common_ancestor( X11DRV_WND_GetXWindow(pWndZ), 
				  X11DRV_WND_GetXWindow(pWnd),
				  &children, &total );
  if( parent && children )
  {
      /* w is the ancestor if pWndCheck that is a direct descendant of 'parent' */

      w = __get_top_decoration( X11DRV_WND_GetXWindow(pWndCheck), parent );

      if( w != children[total-1] ) /* check if at the top */
      {
	  /* X child at index 0 is at the bottom, at index total-1 is at the top */
	  check = __td_lookup( w, children, total );
	  best = total;

          for( WIN_UpdateWndPtr(&pWnd,pWndZ); pWnd;WIN_UpdateWndPtr(&pWnd,pWnd->next))
          {
	      /* go through all windows in Wine z-order... */

	      if( pWnd != pWndCheck )
              {
		  if( !(pWnd->flags & WIN_MANAGED) ||
		      !(w = __get_top_decoration( X11DRV_WND_GetXWindow(pWnd), parent )) )
		    continue;
		  pos = __td_lookup( w, children, total );
		  if( pos < best && pos > check )
                  {
		      /* find a nearest Wine window precedes 
		       * pWndCheck in the real z-order... */
		      best = pos;
		      hwndInsertAfter = pWnd->hwndSelf;
                  }
		  if( best - check == 1 ) break;
              }
          }
      }
  }
  if( children ) TSXFree( children );
  WIN_ReleaseWndPtr(pWnd);
  WIN_ReleaseWndPtr(pWndZ);
  WIN_ReleaseWndPtr(pWndCheck);
  return hwndInsertAfter;
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

/***********************************************************************
 *           EVENT_Expose
 */
static void EVENT_Expose( HWND hWnd, XExposeEvent *event )
{
  RECT rect;

  WND *pWnd = WIN_FindWndPtr(hWnd);
  /* Make position relative to client area instead of window */
  rect.left   = event->x - (pWnd? (pWnd->rectClient.left - pWnd->rectWindow.left) : 0);
  rect.top    = event->y - (pWnd? (pWnd->rectClient.top - pWnd->rectWindow.top) : 0);
  rect.right  = rect.left + event->width;
  rect.bottom = rect.top + event->height;
  WIN_ReleaseWndPtr(pWnd);
 
  Callout.RedrawWindow( hWnd, &rect, 0,
                          RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN | RDW_ERASE |
                          (event->count ? 0 : RDW_ERASENOW) );
}


/***********************************************************************
 *           EVENT_GraphicsExpose
 *
 * This is needed when scrolling area is partially obscured
 * by non-Wine X window.
 */
static void EVENT_GraphicsExpose( HWND hWnd, XGraphicsExposeEvent *event )
{
  RECT rect;
  WND *pWnd = WIN_FindWndPtr(hWnd);
  
  /* Make position relative to client area instead of window */
  rect.left   = event->x - (pWnd? (pWnd->rectClient.left - pWnd->rectWindow.left) : 0);
  rect.top    = event->y - (pWnd? (pWnd->rectClient.top - pWnd->rectWindow.top) : 0);
  rect.right  = rect.left + event->width;
  rect.bottom = rect.top + event->height;
  
  WIN_ReleaseWndPtr(pWnd);
  
  Callout.RedrawWindow( hWnd, &rect, 0,
                          RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE |
                          (event->count ? 0 : RDW_ERASENOW) );
}


/***********************************************************************
 *           EVENT_Key
 *
 * Handle a X key event
 */
static void EVENT_Key( HWND hWnd, XKeyEvent *event )
{
    WND *pWnd = WIN_FindWndPtr(hWnd);
  X11DRV_KEYBOARD_HandleEvent( pWnd, event );
    WIN_ReleaseWndPtr(pWnd);

}


/***********************************************************************
 *           EVENT_MotionNotify
 */
static void EVENT_MotionNotify( HWND hWnd, XMotionEvent *event )
{
  if (current_input_type == X11DRV_INPUT_ABSOLUTE) {
    WND *pWnd = WIN_FindWndPtr(hWnd);
    int xOffset = pWnd? pWnd->rectWindow.left : 0;
    int yOffset = pWnd? pWnd->rectWindow.top  : 0;
    WIN_ReleaseWndPtr(pWnd);
    
    MOUSE_SendEvent( MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE, 
		     xOffset + event->x, yOffset + event->y,
		     X11DRV_EVENT_XStateToKeyState( event->state ), 
		     event->time - MSG_WineStartTicks,
		     hWnd);
  } else {
    MOUSE_SendEvent( MOUSEEVENTF_MOVE,
                     event->x_root, event->y_root,
                     X11DRV_EVENT_XStateToKeyState( event->state ), 
                     event->time - MSG_WineStartTicks,
                     hWnd);
  }
}


/***********************************************************************
 *           EVENT_ButtonPress
 */
static void EVENT_ButtonPress( HWND hWnd, XButtonEvent *event )
{
  static WORD statusCodes[NB_BUTTONS] = 
  { MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_MIDDLEDOWN, MOUSEEVENTF_RIGHTDOWN };
  int buttonNum = event->button - 1;
  
  WND *pWnd = WIN_FindWndPtr(hWnd);
  int xOffset = pWnd? pWnd->rectWindow.left : 0;
  int yOffset = pWnd? pWnd->rectWindow.top  : 0;
  WORD keystate;
  
  WIN_ReleaseWndPtr(pWnd);

  if (buttonNum >= NB_BUTTONS) return;
  
  /*
   * Get the compatible keystate
   */
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
  }
  
  MOUSE_SendEvent( statusCodes[buttonNum], 
		   xOffset + event->x, yOffset + event->y,
		   keystate, 
		   event->time - MSG_WineStartTicks,
		   hWnd);
}


/***********************************************************************
 *           EVENT_ButtonRelease
 */
static void EVENT_ButtonRelease( HWND hWnd, XButtonEvent *event )
{
  static WORD statusCodes[NB_BUTTONS] = 
  { MOUSEEVENTF_LEFTUP, MOUSEEVENTF_MIDDLEUP, MOUSEEVENTF_RIGHTUP };
  int buttonNum = event->button - 1;
  WND *pWnd = WIN_FindWndPtr(hWnd);
  int xOffset = pWnd? pWnd->rectWindow.left : 0;
  int yOffset = pWnd? pWnd->rectWindow.top  : 0;
  WORD keystate;
  
  WIN_ReleaseWndPtr(pWnd);
  
  if (buttonNum >= NB_BUTTONS) return;    
  
  /*
   * Get the compatible keystate
   */
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
  }

  MOUSE_SendEvent( statusCodes[buttonNum], 
		   xOffset + event->x, yOffset + event->y,
		   keystate, 
		   event->time - MSG_WineStartTicks,
		   hWnd);
}


/**********************************************************************
 *              EVENT_FocusIn
 */
static void EVENT_FocusIn( HWND hWnd, XFocusChangeEvent *event )
{
    if (event->detail != NotifyPointer)
        if (hWnd != GetForegroundWindow())
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
    if (event->detail != NotifyPointer)
        if (hWnd == GetForegroundWindow())
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

/**********************************************************************
 *              X11DRV_EVENT_CheckFocus
 */
BOOL X11DRV_EVENT_CheckFocus(void)
{
  HWND   hWnd;
  Window xW;
  int	   state;
  
  TSXGetInputFocus(display, &xW, &state);
    if( xW == None ||
        TSXFindContext(display, xW, winContext, (char **)&hWnd) ) 
      return FALSE;
    return TRUE;
}

/**********************************************************************
 *              EVENT_GetGeometry
 *
 * Helper function for ConfigureNotify handling.
 * Get the new geometry of a window relative to the root window.
 */
static void EVENT_GetGeometry( Window win, int *px, int *py,
                               unsigned int *pwidth, unsigned int *pheight )
{
    Window root, top;
    int x, y, width, height, border, depth;

    EnterCriticalSection( &X11DRV_CritSection );

    /* Get the geometry of the window */
    XGetGeometry( display, win, &root, &x, &y, &width, &height,
                  &border, &depth );

    /* Translate the window origin to root coordinates */
    XTranslateCoordinates( display, win, root, 0, 0, &x, &y, &top );

    LeaveCriticalSection( &X11DRV_CritSection );

    *px = x;
    *py = y;
    *pwidth = width;
    *pheight = height;
}

/**********************************************************************
 *              EVENT_ConfigureNotify
 *
 * The ConfigureNotify event is only selected on top-level windows
 * when the -managed flag is used.
 */
static void EVENT_ConfigureNotify( HWND hWnd, XConfigureEvent *event )
{
    RECT rectWindow;
    int x, y, flags = 0;
    unsigned int width, height;
    HWND newInsertAfter, oldInsertAfter;
  
    /* Get geometry and Z-order according to X */

    EVENT_GetGeometry( event->window, &x, &y, &width, &height );
    newInsertAfter = EVENT_QueryZOrder( hWnd );

    /* Get geometry and Z-order according to Wine */

    /*
     *  Needs to find the first Visible Window above the current one
     */
    oldInsertAfter = hWnd;
    for (;;)
    {
        oldInsertAfter = GetWindow( oldInsertAfter, GW_HWNDPREV );
        if (!oldInsertAfter)
        {
            oldInsertAfter = HWND_TOP;
            break;
        }
        if (GetWindowLongA( oldInsertAfter, GWL_STYLE ) & WS_VISIBLE) break;
    }

    /* Compare what has changed */

    GetWindowRect( hWnd, &rectWindow );
    if ( rectWindow.left == x && rectWindow.top == y )
        flags |= SWP_NOMOVE;
    else
        TRACE_(win)( "%04x moving from (%d,%d) to (%d,%d)\n", hWnd, 
	             rectWindow.left, rectWindow.top, x, y );

    if (    rectWindow.right - rectWindow.left == width
         && rectWindow.bottom - rectWindow.top == height )
        flags |= SWP_NOSIZE;
    else
        TRACE_(win)( "%04x resizing from (%d,%d) to (%d,%d)\n", hWnd, 
                     rectWindow.right - rectWindow.left, 
                     rectWindow.bottom - rectWindow.top, width, height );

    if ( newInsertAfter == oldInsertAfter )
        flags |= SWP_NOZORDER;
    else
        TRACE_(win)( "%04x restacking from after %04x to after %04x\n", hWnd, 
                     oldInsertAfter, newInsertAfter );

    /* If anything changed, call SetWindowPos */

    if ( flags != (SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER) )
        SetWindowPos( hWnd, newInsertAfter, x, y, width, height, 
                            flags | SWP_NOACTIVATE | SWP_WINE_NOHOSTMOVE );
}


/***********************************************************************
 *           EVENT_SelectionRequest_TARGETS
 *  Service a TARGETS selection request event
 */
static Atom EVENT_SelectionRequest_TARGETS( Window requestor, Atom target, Atom rprop )
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
    targets = (Atom*)HEAP_xalloc( GetProcessHeap(), 0, cTargets * sizeof(Atom));

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

#ifdef DEBUG_RUNTIME
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
#endif
    
    /* Update the X property */
    TRACE("\tUpdating property %s...", TSXGetAtomName(display, rprop));

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
static Atom EVENT_SelectionRequest_STRING( Window requestor, Atom target, Atom rprop )
{
    HANDLE16 hText;
    LPSTR  text;
    int    size,i,j;
    char* lpstr = 0;
    char *itemFmtName;
    int xRc;

    /*
     * Map the requested X selection property type atom name to a
     * windows clipboard format ID.
     */
    itemFmtName = TSXGetAtomName(display, target);
    TRACE("Request for %s (wFormat=%x %s)\n",
                  itemFmtName, CF_TEXT, CLIPBOARD_GetFormatName(CF_TEXT));
    TSXFree(itemFmtName);

    if ( !CLIPBOARD_IsPresent(CF_TEXT) )
    {
       rprop = None;
       goto END;
    }

    hText = GetClipboardData16(CF_TEXT);
    text = GlobalLock16(hText);
    size = GlobalSize16(hText);
    
    /* remove carriage returns */
    
    lpstr = (char*)HEAP_xalloc( GetProcessHeap(), 0, size-- );
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

    GlobalUnlock16(hText);
    HeapFree( GetProcessHeap(), 0, lpstr );

END:
    return rprop;
}

/***********************************************************************
 *           EVENT_SelectionRequest_PIXMAP
 *  Service a PIXMAP selection request event
 */
static Atom EVENT_SelectionRequest_PIXMAP( Window requestor, Atom target, Atom rprop )
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
static Atom EVENT_SelectionRequest_WCF( Window requestor, Atom target, Atom rprop )
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
                  TRACE("\tMULTIPLE(%d): Skipping target with empty property!", i);
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
      rprop = EVENT_SelectionRequest_TARGETS( request, event->target, rprop );
  }
  else if(event->target == xaMultiple)  /*  rprop contains a list of (target, property) atom pairs */
  {
      /* MULTIPLE selection request */
      rprop = EVENT_SelectionRequest_MULTIPLE(  hWnd, event );
  }
  else if(event->target == XA_STRING)  /* treat CF_TEXT as Unix text */
  {
      /* XA_STRING selection request */
      rprop = EVENT_SelectionRequest_STRING( request, event->target, rprop );
  }
  else if(event->target == XA_PIXMAP)  /*  Convert DIB's to Pixmaps */
  {
      /* XA_PIXMAP selection request */
      rprop = EVENT_SelectionRequest_PIXMAP( request, event->target, rprop );
  }
  else if(event->target == XA_BITMAP)  /*  Convert DIB's to 1-bit Pixmaps */
  {
      /* XA_BITMAP selection request - TODO: create a monochrome Pixmap */
      rprop = EVENT_SelectionRequest_PIXMAP( request, XA_PIXMAP, rprop );
  }
  else if(X11DRV_CLIPBOARD_IsNativeProperty(event->target)) /* <WCF>* */
  {
      /* All <WCF> selection requests */
      rprop = EVENT_SelectionRequest_WCF( request, event->target, rprop );
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
  Atom xaClipboard = TSXInternAtom(display, "CLIPBOARD", False);
    
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
  TRACE("Received PropertyNotify event: ");

  switch(event->state)
  {
    case PropertyDelete:
    {
      TRACE("\tPropertyDelete for atom %s on window %ld\n",
            TSXGetAtomName(event->display, event->atom), (long)event->window);
      
      if (X11DRV_CLIPBOARD_IsSelectionowner())
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
  HGLOBAL16		hDragInfo = GlobalAlloc16( GMEM_SHARE | GMEM_ZEROINIT, sizeof(DRAGINFO));
  LPDRAGINFO            lpDragInfo = (LPDRAGINFO) GlobalLock16(hDragInfo);
  SEGPTR		spDragInfo = (SEGPTR) WIN16_GlobalLock16(hDragInfo);
  Window		w_aux_root, w_aux_child;
  WND*			pDropWnd;
  WND*                  pWnd;
  
  if( !lpDragInfo || !spDragInfo ) return;
  
  pWnd = WIN_FindWndPtr(hWnd);
  
  TSXQueryPointer( display, X11DRV_WND_GetXWindow(pWnd), &w_aux_root, &w_aux_child, 
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
      TSXGetWindowProperty( display, DefaultRootWindow(display),
			    dndSelection, 0, 65535, FALSE,
			    AnyPropertyType, &u.atom_aux, (int *) &u.pt_aux.y,
			    &data_length, &aux_long, &p_data);
      
      if( !aux_long && p_data)	/* don't bother if > 64K */
	{
	  char *p = (char*) p_data;
	  char *p_drop;
	  
	  aux_long = 0; 
	  while( *p )	/* calculate buffer size */
	    {
	      p_drop = p;
	      if((u.i = *p) != -1 ) 
		u.i = DRIVE_FindDriveRoot( (const char **)&p_drop );
	      if( u.i == -1 ) *p = -1;	/* mark as "bad" */
	      else
		{
		  INT len = GetShortPathNameA( p, NULL, 0 );
		  if (len) aux_long += len + 1;
		  else *p = -1;
		}
	      p += strlen(p) + 1;
	    }
	  if( aux_long && aux_long < 65535 )
	    {
	      HDROP16                 hDrop;
	      LPDROPFILESTRUCT16        lpDrop;
	      
	      aux_long += sizeof(DROPFILESTRUCT16) + 1; 
	      hDrop = (HDROP16)GlobalAlloc16( GMEM_SHARE, aux_long );
	      lpDrop = (LPDROPFILESTRUCT16) GlobalLock16( hDrop );
	      
	      if( lpDrop )
		{
		  lpDrop->wSize = sizeof(DROPFILESTRUCT16);
		  lpDrop->ptMousePos.x = (INT16)x;
		  lpDrop->ptMousePos.y = (INT16)y;
		  lpDrop->fInNonClientArea = (BOOL16) 
		    ( x < (pDropWnd->rectClient.left - pDropWnd->rectWindow.left)  ||
		      y < (pDropWnd->rectClient.top - pDropWnd->rectWindow.top)    ||
		      x > (pDropWnd->rectClient.right - pDropWnd->rectWindow.left) ||
		      y > (pDropWnd->rectClient.bottom - pDropWnd->rectWindow.top) );
		  p_drop = ((char*)lpDrop) + sizeof(DROPFILESTRUCT16);
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
		  PostMessage16( hWnd, WM_DROPFILES,
				 (WPARAM16)hDrop, 0L );
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
  int		x, y, drop32 = FALSE ;
  union {
    Atom	atom_aux;
    int         i;
    Window      w_aux;
  }		u; /* unused */
  union {
    HDROP16     h16;
    HDROP     h32;
  } hDrop;

  pWnd = WIN_FindWndPtr(hWnd);
  drop32 = pWnd->flags & WIN_ISWIN32;

  if (!(pWnd->dwExStyle & WS_EX_ACCEPTFILES))
  {
    WIN_ReleaseWndPtr(pWnd);
    return;
  }
  WIN_ReleaseWndPtr(pWnd);

  TSXGetWindowProperty( display, DefaultRootWindow(display),
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
      TSXQueryPointer( display, X11DRV_GetXRootWindow(), &u.w_aux, &u.w_aux, 
		       &x, &y, &u.i, &u.i, &u.i);

      pDropWnd = WIN_FindWndPtr( hWnd );
      
      if (drop32) {
	LPDROPFILESTRUCT        lpDrop;
	drop_len += sizeof(DROPFILESTRUCT) + 1; 
	hDrop.h32 = (HDROP)GlobalAlloc( GMEM_SHARE, drop_len );
	lpDrop = (LPDROPFILESTRUCT) GlobalLock( hDrop.h32 );
	
	if( lpDrop ) {
	  lpDrop->lSize = sizeof(DROPFILESTRUCT);
	  lpDrop->ptMousePos.x = (INT)x;
	  lpDrop->ptMousePos.y = (INT)y;
	  lpDrop->fInNonClientArea = (BOOL) 
	    ( x < (pDropWnd->rectClient.left - pDropWnd->rectWindow.left)  ||
	      y < (pDropWnd->rectClient.top - pDropWnd->rectWindow.top)    ||
	      x > (pDropWnd->rectClient.right - pDropWnd->rectWindow.left) ||
	      y > (pDropWnd->rectClient.bottom - pDropWnd->rectWindow.top) );
	  lpDrop->fWideChar = FALSE;
	  p_drop = ((char*)lpDrop) + sizeof(DROPFILESTRUCT);
	}
      } else {
	LPDROPFILESTRUCT16        lpDrop;
	drop_len += sizeof(DROPFILESTRUCT16) + 1; 
	hDrop.h16 = (HDROP16)GlobalAlloc16( GMEM_SHARE, drop_len );
	lpDrop = (LPDROPFILESTRUCT16) GlobalLock16( hDrop.h16 );
	
	if( lpDrop ) {
	  lpDrop->wSize = sizeof(DROPFILESTRUCT16);
	  lpDrop->ptMousePos.x = (INT16)x;
	  lpDrop->ptMousePos.y = (INT16)y;
	  lpDrop->fInNonClientArea = (BOOL16) 
	    ( x < (pDropWnd->rectClient.left - pDropWnd->rectWindow.left)  ||
	      y < (pDropWnd->rectClient.top - pDropWnd->rectWindow.top)    ||
	      x > (pDropWnd->rectClient.right - pDropWnd->rectWindow.left) ||
	      y > (pDropWnd->rectClient.bottom - pDropWnd->rectWindow.top) );
	  p_drop = ((char*)lpDrop) + sizeof(DROPFILESTRUCT16);
	}
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

	if (drop32) {
	  /* can not use PostMessage32A because it is currently based on 
	   * PostMessage16 and WPARAM32 would be truncated to WPARAM16
	   */
	  GlobalUnlock(hDrop.h32);
	  SendMessageA( hWnd, WM_DROPFILES,
			  (WPARAM)hDrop.h32, 0L );
	} else {
	  GlobalUnlock16(hDrop.h16);
	  PostMessage16( hWnd, WM_DROPFILES,
			 (WPARAM16)hDrop.h16, 0L );
	}
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
      /* Ignore the delete window request if the window has been disabled
       * and we are in managed mode. This is to disallow applications from
       * being closed by the window manager while in a modal state.
       */
      BOOL bIsDisabled;
      bIsDisabled = GetWindowLongA( hWnd, GWL_STYLE ) & WS_DISABLED;

      if ( !Options.managed || !bIsDisabled )
      PostMessage16( hWnd, WM_SYSCOMMAND, SC_CLOSE, 0 );
    }
    else if ( event->message_type == dndProtocol &&
	      (event->data.l[0] == DndFile || event->data.l[0] == DndFiles) )
      EVENT_DropFromOffiX(hWnd, event);
    else if ( event->message_type == dndProtocol &&
	      event->data.l[0] == DndURL )
      EVENT_DropURLs(hWnd, event);
    else {
#if 0
      /* enable this if you want to see the message */
      unsigned char* p_data = NULL;
      union {
	unsigned long	l;
	int            	i;
	Atom		atom;
      } u; /* unused */
      TSXGetWindowProperty( display, DefaultRootWindow(display),
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
 *           EVENT_EnterNotify
 *
 * Install colormap when Wine window is focused in
 * self-managed mode with private colormap
 */
#if 0
void EVENT_EnterNotify( HWND hWnd, XCrossingEvent *event )
{
  if( !Options.managed && X11DRV_GetXRootWindow() == DefaultRootWindow(display) &&
      (COLOR_GetSystemPaletteFlags() & COLOR_PRIVATE) && GetFocus() )
    TSXInstallColormap( display, X11DRV_PALETTE_GetColormap() );
}
#endif

/**********************************************************************
 *		EVENT_MapNotify
 */
void EVENT_MapNotify( HWND hWnd, XMapEvent *event )
{
  HWND hwndFocus = GetFocus();
  WND *wndFocus = WIN_FindWndPtr(hwndFocus);
  WND *pWnd = WIN_FindWndPtr(hWnd);
  if (pWnd->flags & WIN_MANAGED)
  {
      pWnd->dwStyle &= ~WS_MINIMIZE;
      ShowOwnedPopups(hWnd,TRUE);
  }
  WIN_ReleaseWndPtr(pWnd);

  if (hwndFocus && IsChild( hWnd, hwndFocus ))
      X11DRV_WND_SetFocus(wndFocus);

  WIN_ReleaseWndPtr(wndFocus);
  
  return;
}


/**********************************************************************
 *              EVENT_MapNotify
 */
void EVENT_UnmapNotify( HWND hWnd, XUnmapEvent *event )
{
  WND *pWnd = WIN_FindWndPtr(hWnd);
  if (pWnd->flags & WIN_MANAGED)
  {
      EndMenu();
      if( pWnd->dwStyle & WS_VISIBLE )
      {
	  pWnd->dwStyle |= WS_MINIMIZE;
          ShowOwnedPopups(hWnd,FALSE);
      }
  }
  WIN_ReleaseWndPtr(pWnd);
}

/**********************************************************************
 *              X11DRV_EVENT_SetInputMehod
 */
INPUT_TYPE X11DRV_EVENT_SetInputMehod(INPUT_TYPE type)
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
  MOUSE_SendEvent( MOUSEEVENTF_MOVE, 
		   event->dx, event->dy,
		   X11DRV_EVENT_XStateToKeyState( event->state ), 
		   event->time - MSG_WineStartTicks,
		   DGAhwnd );
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
  
  MOUSE_SendEvent( statusCodes[buttonNum], 
		   0, 0, 
		   keystate, 
		   event->time - MSG_WineStartTicks,
		   DGAhwnd );
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
  
  MOUSE_SendEvent( statusCodes[buttonNum], 
		   0, 0, 
		   keystate, 
		   event->time - MSG_WineStartTicks,
		   DGAhwnd );
}

#endif

#ifdef HAVE_LIBXXSHM

/*
Normal XShm operation:

X11           service thread    app thread
------------- ----------------- ------------------------
              (idle)            ddraw calls XShmPutImage
(copies data)                   (waiting for shm_event)
ShmCompletion ->                (waiting for shm_event)
(idle)        signal shm_event ->
              (idle)            returns to app

However, this situation can occur for some reason:

X11           service thread    app thread
------------- ----------------- ------------------------
Expose ->
              WM_ERASEBKGND? ->
              (waiting for app) ddraw calls XShmPutImage
(copies data) (waiting for app) (waiting for shm_event)
ShmCompletion (waiting for app) (waiting for shm_event)
(idle)        DEADLOCK          DEADLOCK

which is why I also wait for shm_read and do XCheckTypedEvent()
calls in the wait loop. This results in:

X11           service thread    app thread
------------- ----------------- ------------------------
ShmCompletion (waiting for app) waking up on shm_read
(idle)        (waiting for app) XCheckTypedEvent() -> signal shm_event
              (waiting for app) returns
              (idle)
*/

typedef struct {
  Drawable draw;
  LONG state, waiter;
  HANDLE sema;
} shm_qs;
   
/* FIXME: this is not pretty */
static HANDLE shm_read = 0;

#define SHM_MAX_Q 4
static volatile shm_qs shm_q[SHM_MAX_Q];

static void EVENT_ShmCompletion( XShmCompletionEvent *event )
{
  int n;

  TRACE("Got ShmCompletion for drawable %ld (time %ld)\n", event->drawable, GetTickCount() );

  for (n=0; n<SHM_MAX_Q; n++)
    if ((shm_q[n].draw == event->drawable) && (shm_q[n].state == 0)) {
      HANDLE sema = shm_q[n].sema;
      if (!InterlockedCompareExchange((PVOID*)&shm_q[n].state, (PVOID)1, (PVOID)0)) {
        ReleaseSemaphore(sema, 1, NULL);
        TRACE("Signaling ShmCompletion (#%d) (semaphore %x)\n", n, sema);
      }
      return;
    }

  ERR("Got ShmCompletion for unknown drawable %ld\n", event->drawable );
}

int X11DRV_EVENT_PrepareShmCompletion( Drawable dw )
{
  int n;

  if (!shm_read)
    shm_read = ConvertToGlobalHandle( FILE_DupUnixHandle( ConnectionNumber(display), GENERIC_READ | SYNCHRONIZE ) );

  for (n=0; n<SHM_MAX_Q; n++)
    if (!shm_q[n].draw)
      if (!InterlockedCompareExchange((PVOID*)&shm_q[n].draw, (PVOID)dw, (PVOID)0))
        break;

  if (n>=SHM_MAX_Q) {
    ERR("Maximum number of outstanding ShmCompletions exceeded!\n");
    return 0;
  }

  shm_q[n].state = 0;
  if (!shm_q[n].sema) {
    shm_q[n].sema = ConvertToGlobalHandle( CreateSemaphoreA( NULL, 0, 256, NULL ) );
    TRACE("Allocated ShmCompletion slots have been increased to %d, new semaphore is %x\n", n+1, shm_q[n].sema);
  }

  TRACE("Prepared ShmCompletion (#%d) wait for drawable %ld (thread %lx) (time %ld)\n", n, dw, GetCurrentThreadId(), GetTickCount() );
  return n+1;
}

static void X11DRV_EVENT_WaitReplaceShmCompletionInternal( int *compl, Drawable dw, int creat )
{
  int n = *compl;
  LONG nn, st;
  HANDLE sema;

  if ((!n) || (creat && (!shm_q[n-1].draw))) {
    nn = X11DRV_EVENT_PrepareShmCompletion(dw);
    if (!(n=(LONG)InterlockedCompareExchange((PVOID*)compl, (PVOID)nn, (PVOID)n)))
      return;
    /* race for compl lost, clear slot */
    shm_q[nn-1].draw = 0;
    return;
  }

  if (dw && (shm_q[n-1].draw != dw)) {
    /* this shouldn't happen with the current ddraw implementation */
    FIXME("ShmCompletion replace with different drawable!\n");
    return;
  }

  sema = shm_q[n-1].sema;
  if (!sema) {
    /* nothing to wait on (PrepareShmCompletion not done yet?), so probably nothing to wait for */
    return;
  }

  nn = InterlockedExchangeAdd((PLONG)&shm_q[n-1].waiter, 1);
  if ((!shm_q[n-1].draw) || (shm_q[n-1].state == 2)) {
    /* too late, the wait was just cleared (wait complete) */
    TRACE("Wait skip for ShmCompletion (#%d) (thread %lx) (time %ld) (semaphore %x)\n", n-1, GetCurrentThreadId(), GetTickCount(), sema);
  } else {
    TRACE("Waiting for ShmCompletion (#%d) (thread %lx) (time %ld) (semaphore %x)\n", n-1, GetCurrentThreadId(), GetTickCount(), sema);
    if (nn) {
      /* another thread is already waiting, let the primary waiter do the dirty work
       * (to avoid TSX critical section contention - that could get really slow) */
      WaitForSingleObject( sema, INFINITE );
    } else
    /* we're primary waiter - first check if it's already triggered */
    if ( WaitForSingleObject( sema, 0 ) != WAIT_OBJECT_0 ) {
      /* nope, may need to poll X event queue, in case the service thread is blocked */
      XEvent event;
      HANDLE hnd[2];

      hnd[0] = sema;
      hnd[1] = shm_read;
      do {
        /* check X event queue */
        if (TSXCheckTypedEvent( display, ShmCompletionType, &event)) {
          EVENT_ProcessEvent( &event );
        }
      } while ( WaitForMultipleObjects(2, hnd, FALSE, INFINITE) > WAIT_OBJECT_0 );
    }
    TRACE("Wait complete (thread %lx) (time %ld)\n", GetCurrentThreadId(), GetTickCount() );

    /* clear wait */
    st = InterlockedExchange((LPLONG)&shm_q[n-1].state, 2);
    if (st != 2) {
      /* first waiter to return, release all other waiters */
      nn = shm_q[n-1].waiter;
      TRACE("Signaling %ld additional ShmCompletion (#%d) waiter(s), semaphore %x\n", nn-1, n-1, sema);
      ReleaseSemaphore(sema, nn-1, NULL);
    }
  }
  nn = InterlockedDecrement((LPLONG)&shm_q[n-1].waiter);
  if (!nn) {
    /* last waiter to return, replace drawable and prepare new wait */
    shm_q[n-1].draw = dw;
    shm_q[n-1].state = 0;
  }
}

void X11DRV_EVENT_WaitReplaceShmCompletion( int *compl, Drawable dw )
{
  X11DRV_EVENT_WaitReplaceShmCompletionInternal( compl, dw, 1 );
}

void X11DRV_EVENT_WaitShmCompletion( int compl )
{
  if (!compl) return;
  X11DRV_EVENT_WaitReplaceShmCompletionInternal( &compl, 0, 0 );
}

void X11DRV_EVENT_WaitShmCompletions( Drawable dw )
{
  int n;

  for (n=0; n<SHM_MAX_Q; n++)
    if (shm_q[n].draw == dw)
      X11DRV_EVENT_WaitShmCompletion( n+1 );
}

#endif /* defined(HAVE_LIBXXSHM) */

#endif /* !defined(X_DISPLAY_MISSING) */
