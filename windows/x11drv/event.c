/*
 * X11 event driver
 *
 * Copyright 1993 Alexandre Julliard
 */

#include "config.h"

#ifndef X_DISPLAY_MISSING

#include <X11/Xatom.h>
#include <X11/keysym.h>
#include "ts_xlib.h"
#include "ts_xresource.h"
#include "ts_xutil.h"

#include <assert.h>
#include <string.h>
#include <unistd.h>
#include "callback.h"
#include "class.h"
#include "clipboard.h"
#include "dce.h"
#include "dde_proc.h"
#include "debug.h"
#include "drive.h"
#include "heap.h"
#include "keyboard.h"
#include "message.h"
#include "mouse.h"
#include "options.h"
#include "queue.h"
#include "shell.h"
#include "winpos.h"
#include "winsock.h"
#include "windef.h"
#include "x11drv.h"

DECLARE_DEBUG_CHANNEL(event)
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

/* EVENT_WaitNetEvent() master fd sets */

static fd_set __event_io_set[3];
static int    __event_max_fd = 0;
static int    __event_x_connection = 0;
static int    __wakeup_pipe[2];

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
static void EVENT_Key( HWND hWnd, XKeyEvent *event );
static void EVENT_ButtonPress( HWND hWnd, XButtonEvent *event );
static void EVENT_ButtonRelease( HWND hWnd, XButtonEvent *event );
static void EVENT_MotionNotify( HWND hWnd, XMotionEvent *event );
static void EVENT_FocusIn( HWND hWnd, XFocusChangeEvent *event );
static void EVENT_FocusOut( HWND hWnd, XFocusChangeEvent *event );
static void EVENT_Expose( HWND hWnd, XExposeEvent *event );
static void EVENT_GraphicsExpose( HWND hWnd, XGraphicsExposeEvent *event );
static void EVENT_ConfigureNotify( HWND hWnd, XConfigureEvent *event );
static void EVENT_SelectionRequest( HWND hWnd, XSelectionRequestEvent *event);
static void EVENT_SelectionClear( HWND hWnd, XSelectionClearEvent *event);
static void EVENT_ClientMessage( HWND hWnd, XClientMessageEvent *event );
static void EVENT_MapNotify( HWND pWnd, XMapEvent *event );
static void EVENT_UnmapNotify( HWND pWnd, XUnmapEvent *event );

/* Usable only with OLVWM - compile option perhaps?
static void EVENT_EnterNotify( HWND hWnd, XCrossingEvent *event );
*/

static void EVENT_GetGeometry( Window win, int *px, int *py,
                               unsigned int *pwidth, unsigned int *pheight );


/***********************************************************************
 *           EVENT_Init
 *
 * Initialize network IO.
 */
BOOL X11DRV_EVENT_Init(void)
{
  int  i;
  for( i = 0; i < 3; i++ )
    FD_ZERO( __event_io_set + i );
  
  __event_max_fd = __event_x_connection = ConnectionNumber(display);
  FD_SET( __event_x_connection, &__event_io_set[EVENT_IO_READ] );

  /* this pipe is used to be able to wake-up the scheduler(WaitNetEvent) by
   a 32 bit thread, this will become obsolete when the input thread will be
   implemented */
  pipe(__wakeup_pipe);

  /* make the pipe non-blocking */
  fcntl(__wakeup_pipe[0], F_SETFL, O_NONBLOCK);
  fcntl(__wakeup_pipe[1], F_SETFL, O_NONBLOCK);
  
  FD_SET( __wakeup_pipe[0], &__event_io_set[EVENT_IO_READ] );
  if (__wakeup_pipe[0] > __event_max_fd)
      __event_max_fd = __wakeup_pipe[0];
      
  __event_max_fd++;
  return TRUE;
}

/***********************************************************************
 *		X11DRV_EVENT_AddIO 
 */
void X11DRV_EVENT_AddIO( int fd, unsigned io_type )
{
  FD_SET( fd, &__event_io_set[io_type] );
  if( __event_max_fd <= fd ) __event_max_fd = fd + 1;
}

/***********************************************************************
 *		X11DRV_EVENT_DeleteIO 
 */
void X11DRV_EVENT_DeleteIO( int fd, unsigned io_type )
{
  FD_CLR( fd, &__event_io_set[io_type] );
}

/***********************************************************************
 *		X11DRV_EVENT_IsUserIdle 
 */
BOOL16 X11DRV_EVENT_IsUserIdle(void)
{
  struct timeval timeout = {0, 0};
  fd_set check_set;
  
  FD_ZERO(&check_set);
  FD_SET(__event_x_connection, &check_set);
  if( select(__event_x_connection + 1, &check_set, NULL, NULL, &timeout) > 0 )
    return TRUE;
  return FALSE;
}


/***********************************************************************
 *           EVENT_ReadWakeUpPipe
 *
 * Empty the wake up pipe
 */
void EVENT_ReadWakeUpPipe(void)
{
    char tmpBuf[10];
    ssize_t ret;
          
    EnterCriticalSection(&X11DRV_CritSection);
    
    /* Flush the wake-up pipe, it's just dummy data for waking-up this
     thread. This will be obsolete when the input thread will be done */
    while ( (ret = read(__wakeup_pipe[0], &tmpBuf, 10)) == 10 );

    LeaveCriticalSection(&X11DRV_CritSection);
}

/***********************************************************************
 *           X11DRV_EVENT_WaitNetEvent
 *
 * Wait for a network event, optionally sleeping until one arrives.
 * Returns TRUE if an event is pending that cannot be processed in
 * 'peek' mode, FALSE otherwise.
 */

BOOL X11DRV_EVENT_WaitNetEvent( BOOL sleep, BOOL peek )
{
  XEvent event;
  LONG maxWait = sleep ? TIMER_GetNextExpiration() : 0;
  int pending = TSXPending(display);
  
  /* Wait for an event or a timeout. If maxWait is -1, we have no timeout;
   * in this case, we fall through directly to the XNextEvent loop.
   */
  
  if ((maxWait != -1) && !pending)
    {
      int num_pending;
      struct timeval timeout;
      fd_set io_set[3];
      
      memcpy( io_set, __event_io_set, sizeof(io_set) );
      
      timeout.tv_usec = (maxWait % 1000) * 1000;
      timeout.tv_sec = maxWait / 1000;
      
#ifdef CONFIG_IPC
      sigsetjmp(env_wait_x, 1);
      stop_wait_op= CONT;
      
      if (DDE_GetRemoteMessage()) {
	while(DDE_GetRemoteMessage())
	  ;
	return FALSE;
      }
      stop_wait_op = STOP_WAIT_X;
      /* The code up to the next "stop_wait_op = CONT" must be reentrant */
      num_pending = select( __event_max_fd, &io_set[EVENT_IO_READ], 
			    &io_set[EVENT_IO_WRITE], 
			    &io_set[EVENT_IO_EXCEPT], &timeout );
      if ( num_pending == 0 )
        {
	  stop_wait_op = CONT;
	  TIMER_ExpireTimers();
	  return FALSE;
	}
      else stop_wait_op = CONT;
#else  /* CONFIG_IPC */
      num_pending = select( __event_max_fd, &io_set[EVENT_IO_READ],
			    &io_set[EVENT_IO_WRITE],
			    &io_set[EVENT_IO_EXCEPT], &timeout );
      
      if ( num_pending == 0)
        {
	  /* Timeout or error */
	  TIMER_ExpireTimers();
	  return FALSE;
        }
#endif  /* CONFIG_IPC */
      
      /* Flush the wake-up pipe, it's just dummy data for waking-up this
       thread. This will be obsolete when the input thread will be done */
      if ( FD_ISSET( __wakeup_pipe[0], &io_set[EVENT_IO_READ] ) )
      {
          num_pending--;
          EVENT_ReadWakeUpPipe();
      }
       
      /*  Winsock asynchronous services */
      
      if( FD_ISSET( __event_x_connection, &io_set[EVENT_IO_READ]) ) 
	{
	  num_pending--;
	  if( num_pending )
	    WINSOCK_HandleIO( &__event_max_fd, num_pending, io_set, __event_io_set );
	}
      else /* no X events */
      {
	WINSOCK_HandleIO( &__event_max_fd, num_pending, io_set, __event_io_set );
	return FALSE;
      }
    }
  else if(!pending)
    {				/* Wait for X11 input. */
      fd_set set;
      int max_fd;
      
      FD_ZERO(&set);
      FD_SET(__event_x_connection, &set);

      /* wait on wake-up pipe also */
      FD_SET(__wakeup_pipe[0], &set);
      if (__event_x_connection > __wakeup_pipe[0])
          max_fd = __event_x_connection + 1;
      else
          max_fd = __wakeup_pipe[0] + 1;
          
      select(max_fd, &set, 0, 0, 0 );

      /* Flush the wake-up pipe, it's just dummy data for waking-up this
       thread. This will be obsolete when the input thread will be done */
      if ( FD_ISSET( __wakeup_pipe[0], &set ) )
          EVENT_ReadWakeUpPipe();
    }
  
  /* Process current X event (and possibly others that occurred in the meantime) */
  
  EnterCriticalSection(&X11DRV_CritSection);
  while (XPending( display ))
    {
      
#ifdef CONFIG_IPC
      if (DDE_GetRemoteMessage())
        {
	  LeaveCriticalSection(&X11DRV_CritSection);
	  while(DDE_GetRemoteMessage()) ;
	  return FALSE;
        }
#endif  /* CONFIG_IPC */
      
      XNextEvent( display, &event );
      
      if( peek )
      {
	  /* Check only for those events which can be processed
	   * internally. */

	  if( event.type == MotionNotify ||
	      event.type == ButtonPress || event.type == ButtonRelease ||
	      event.type == KeyPress || event.type == KeyRelease ||
	      event.type == SelectionRequest || event.type == SelectionClear ||
	      event.type == ClientMessage )
	  {
	      LeaveCriticalSection(&X11DRV_CritSection);
	      EVENT_ProcessEvent( &event );
	      EnterCriticalSection(&X11DRV_CritSection);
	      continue;
	  }

	  if ( event.type == NoExpose )
	    continue;
	  
	  XPutBackEvent(display, &event);
	  LeaveCriticalSection(&X11DRV_CritSection);
	  return TRUE;
      }
      else
      {
          LeaveCriticalSection(&X11DRV_CritSection);
          EVENT_ProcessEvent( &event );
          EnterCriticalSection(&X11DRV_CritSection);
      }
    }
  LeaveCriticalSection(&X11DRV_CritSection);

  return FALSE;
}

/***********************************************************************
 *           EVENT_Synchronize
 *
 * Synchronize with the X server. Should not be used too often.
 */
void X11DRV_EVENT_Synchronize()
{
  XEvent event;
  
  /* Use of the X critical section is needed or we have a small
   * race between XPending() and XNextEvent().
   */
  EnterCriticalSection( &X11DRV_CritSection );
  XSync( display, False );
  while (XPending( display ))
    {
      XNextEvent( display, &event );
      /* unlock X critsection for EVENT_ProcessEvent() might switch tasks */
      LeaveCriticalSection( &X11DRV_CritSection );
      EVENT_ProcessEvent( &event );
      EnterCriticalSection( &X11DRV_CritSection );
    }    
  LeaveCriticalSection( &X11DRV_CritSection );
}

/***********************************************************************
 *           EVENT_ProcessEvent
 *
 * Process an X event.
 */
static void EVENT_ProcessEvent( XEvent *event )
{
  WND *pWnd;
  HWND hWnd;

  switch (event->type)
  {
    case SelectionNotify: /* all of these should be caught by XCheckTypedWindowEvent() */
	 FIXME(event,"Got SelectionNotify - must not happen!\n");
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
      
  if ( TSXFindContext( display, event->xany.window, winContext,
		       (char **)&pWnd ) != 0) {
    if ( event->type == ClientMessage) {
      /* query window (drag&drop event contains only drag window) */
      Window   	root, child;
      int      	root_x, root_y, child_x, child_y;
      unsigned	u;
      TSXQueryPointer( display, X11DRV_GetXRootWindow(), &root, &child,
		       &root_x, &root_y, &child_x, &child_y, &u);
      if (TSXFindContext( display, child, winContext, (char **)&pWnd ) != 0)
	return;
    } else {
      pWnd = NULL;  /* Not for a registered window */
    }
  }

  WIN_LockWndPtr(pWnd);
  
  if(!pWnd)
      hWnd = 0;
  else
      hWnd = pWnd->hwndSelf;
  
      
  if ( !pWnd && event->xany.window != X11DRV_GetXRootWindow() )
      ERR( event, "Got event %s for unknown Window %08lx\n",
           event_names[event->type], event->xany.window );
  else
      TRACE( event, "Got event %s for hwnd %04x\n",
	     event_names[event->type], hWnd );

  WIN_ReleaseWndPtr(pWnd);

  
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
      while (TSXCheckTypedWindowEvent(display,((XAnyEvent *)event)->window,
				      MotionNotify, event));    
      EVENT_MotionNotify( hWnd, (XMotionEvent*)event );
      break;
      
    case FocusIn:
      if (!hWnd) return;
      EVENT_FocusIn( hWnd, (XFocusChangeEvent*)event );
      break;
      
    case FocusOut:
      if (!hWnd) return;
      EVENT_FocusOut( hWnd, (XFocusChangeEvent*)event );
      break;
      
    case Expose:
      EVENT_Expose( hWnd, (XExposeEvent *)event );
      break;
      
    case GraphicsExpose:
      EVENT_GraphicsExpose( hWnd, (XGraphicsExposeEvent *)event );
      break;
      
    case ConfigureNotify:
      if (!hWnd) return;
      EVENT_ConfigureNotify( hWnd, (XConfigureEvent*)event );
      break;

    case SelectionRequest:
      if (!hWnd) return;
      EVENT_SelectionRequest( hWnd, (XSelectionRequestEvent *)event );
      break;

    case SelectionClear:
      if (!hWnd) return;
      EVENT_SelectionClear( hWnd, (XSelectionClearEvent*) event );
      break;
      
    case ClientMessage:
      if (!hWnd) return;
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
      if (!hWnd) return;
      EVENT_MapNotify( hWnd, (XMapEvent *)event );
      break;

    case UnmapNotify:
      if (!hWnd) return;
      EVENT_UnmapNotify( hWnd, (XUnmapEvent *)event );
      break;

    default:    
      WARN(event, "Unprocessed event %s for hwnd %04x\n",
	   event_names[event->type], hWnd );
      break;
    }
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
      if( *children ) TSXFree( *children );
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
  TRACE(event, "\t%08x -> %08x\n", (unsigned)prev, (unsigned)w );
  return ( parent ) ? w : 0 ;
}

static unsigned __td_lookup( Window w, Window* list, unsigned max )
{
  unsigned    i;
  for( i = max - 1; i >= 0; i-- ) if( list[i] == w ) break;
  return i;
}

static BOOL EVENT_QueryZOrder( HWND hWndCheck)
{
  BOOL      bRet = FALSE;
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
      return TRUE;
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
	  /* and link pWndCheck right behind it in the local z-order */
      }
      WIN_UnlinkWindow( pWndCheck->hwndSelf );
      WIN_LinkWindow( pWndCheck->hwndSelf, hwndInsertAfter);
  }
  if( children ) TSXFree( children );
  WIN_ReleaseWndPtr(pWnd);
  WIN_ReleaseWndPtr(pWndZ);
  WIN_ReleaseWndPtr(pWndCheck);
  return bRet;
}

/***********************************************************************
 *           EVENT_XStateToKeyState
 *
 * Translate a X event state (Button1Mask, ShiftMask, etc...) to
 * a Windows key state (MK_SHIFT, MK_CONTROL, etc...)
 */
static WORD EVENT_XStateToKeyState( int state )
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
 *           X11DRV_EVENT_QueryPointer
 */
BOOL X11DRV_EVENT_QueryPointer(DWORD *posX, DWORD *posY, DWORD *state)
{
  Window root, child;
  int rootX, rootY, winX, winY;
  unsigned int xstate;
  
  if (!TSXQueryPointer( display, X11DRV_GetXRootWindow(), &root, &child,
			&rootX, &rootY, &winX, &winY, &xstate )) 
    return FALSE;
  
  if(posX)
    *posX  = (DWORD)winX;
  if(posY)
    *posY  = (DWORD)winY;
  if(state)
    *state = EVENT_XStateToKeyState( xstate );
  
  return TRUE;
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
  WND *pWnd = WIN_FindWndPtr(hWnd);
  int xOffset = pWnd? pWnd->rectWindow.left : 0;
  int yOffset = pWnd? pWnd->rectWindow.top  : 0;
  WIN_ReleaseWndPtr(pWnd);

  MOUSE_SendEvent( MOUSEEVENTF_MOVE, 
		   xOffset + event->x, yOffset + event->y,
		   EVENT_XStateToKeyState( event->state ), 
		   event->time - MSG_WineStartTicks,
		   hWnd);
}


/***********************************************************************
 *           X11DRV_EVENT_DummyMotionNotify
 *
 * Generate a dummy MotionNotify event. Used to force a WM_SETCURSOR message.
 */
void X11DRV_EVENT_DummyMotionNotify(void)
{
  DWORD winX, winY, state;
  
  if ( EVENT_QueryPointer( &winX, &winY, &state ) )
    {
      MOUSE_SendEvent( MOUSEEVENTF_MOVE, winX, winY, state,
		       GetTickCount(), 0 );
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
  keystate = EVENT_XStateToKeyState( event->state );
  
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
  keystate = EVENT_XStateToKeyState( event->state );

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
  if (Options.managed) EVENT_QueryZOrder( hWnd );
  
  if (event->detail != NotifyPointer)
    { 
      if (hWnd != GetActiveWindow())
        {
	  WINPOS_ChangeActiveWindow( hWnd, FALSE );
	  X11DRV_KEYBOARD_UpdateState();
        }
      if ((hWnd != GetFocus()) && !IsChild( hWnd, GetFocus()))
            SetFocus( hWnd );
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
    {
        if (hWnd == GetActiveWindow())
	{
	    SendMessageA( hWnd, WM_CANCELMODE, 0, 0 );
	    WINPOS_ChangeActiveWindow( 0, FALSE );
	}
        if ((hWnd == GetFocus()) || IsChild( hWnd, GetFocus()))
	    SetFocus( 0 );
    }
}

/**********************************************************************
 *              X11DRV_EVENT_CheckFocus
 */
BOOL X11DRV_EVENT_CheckFocus(void)
{
  WND*   pWnd;
  Window xW;
  int	   state;
  
  TSXGetInputFocus(display, &xW, &state);
    if( xW == None ||
        TSXFindContext(display, xW, winContext, (char **)&pWnd) ) 
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
  Window root, parent, *children;
  int xpos, ypos;
  unsigned int width, height, border, depth, nb_children;
  
  if (!TSXGetGeometry( display, win, &root, px, py, pwidth, pheight,
                       &border, &depth )) return;
  if (win == X11DRV_GetXRootWindow())
  {
      *px = *py = 0;
      return;
  }
  
  for (;;)
  {
      if (!TSXQueryTree(display, win, &root, &parent, &children, &nb_children))
	return;
      TSXFree( children );
      if (parent == X11DRV_GetXRootWindow()) break;
      win = parent;
      if (!TSXGetGeometry( display, win, &root, &xpos, &ypos,
                           &width, &height, &border, &depth )) return;
      *px += xpos;
      *py += ypos;
  }
}


/**********************************************************************
 *              EVENT_ConfigureNotify
 *
 * The ConfigureNotify event is only selected on top-level windows
 * when the -managed flag is used.
 */
static void EVENT_ConfigureNotify( HWND hWnd, XConfigureEvent *event )
{
  WINDOWPOS winpos;
  RECT newWindowRect, newClientRect;
  RECT oldWindowRect, oldClientRect;
  Window above = event->above;
  int x, y;
  unsigned int width, height;
  
  WND *pWnd = WIN_FindWndPtr(hWnd);
  assert (pWnd->flags & WIN_MANAGED);
  
  /* We don't rely on the event geometry info, because it is relative
   * to parent and not to root, and it may be wrong (XFree sets x,y to 0,0
   * if the window hasn't moved).
   */
  EVENT_GetGeometry( event->window, &x, &y, &width, &height );
    
TRACE(win, "%04x adjusted to (%i,%i)-(%i,%i)\n", pWnd->hwndSelf, 
	    x, y, x + width, y + height );

  /* Fill WINDOWPOS struct */
  winpos.flags = SWP_NOACTIVATE | SWP_NOZORDER;
  winpos.hwnd = hWnd;
  winpos.x = x;
  winpos.y = y;
  winpos.cx = width;
  winpos.cy = height;
    
  /* Check for unchanged attributes */
  if (winpos.x == pWnd->rectWindow.left && winpos.y == pWnd->rectWindow.top)
    winpos.flags |= SWP_NOMOVE;
  if ((winpos.cx == pWnd->rectWindow.right - pWnd->rectWindow.left) &&
      (winpos.cy == pWnd->rectWindow.bottom - pWnd->rectWindow.top))
    winpos.flags |= SWP_NOSIZE;
  else
    {
      RECT rect = { 0, 0, pWnd->rectWindow.right - pWnd->rectWindow.left,
		      pWnd->rectWindow.bottom - pWnd->rectWindow.top };
      DCE_InvalidateDCE( pWnd, &rect );
    }
  
  /* Send WM_WINDOWPOSCHANGING */
  SendMessageA( winpos.hwnd, WM_WINDOWPOSCHANGING, 0, (LPARAM)&winpos );

  if (!IsWindow( winpos.hwnd ))
  {
      WIN_ReleaseWndPtr(pWnd);
      return;
  }
  
  /* Calculate new position and size */
  newWindowRect.left = x;
  newWindowRect.right = x + width;
  newWindowRect.top = y;
  newWindowRect.bottom = y + height;

  WINPOS_SendNCCalcSize( winpos.hwnd, TRUE, &newWindowRect,
			 &pWnd->rectWindow, &pWnd->rectClient,
			 &winpos, &newClientRect );
  
  if (!IsWindow( winpos.hwnd ))
  {
      WIN_ReleaseWndPtr(pWnd);
      return;
  }
  
  oldWindowRect = pWnd->rectWindow;
  oldClientRect = pWnd->rectClient;

  /* Set new size and position */
  pWnd->rectWindow = newWindowRect;
  pWnd->rectClient = newClientRect;

  /* FIXME: Copy valid bits */

  if( oldClientRect.top - oldWindowRect.top != newClientRect.top - newWindowRect.top ||
      oldClientRect.left - oldWindowRect.left != newClientRect.left - newWindowRect.left )
      RedrawWindow( winpos.hwnd, NULL, 0, RDW_FRAME | RDW_ALLCHILDREN |
                                          RDW_INVALIDATE | RDW_ERASE | RDW_ERASENOW );

  WIN_ReleaseWndPtr(pWnd);

  SendMessageA( winpos.hwnd, WM_WINDOWPOSCHANGED, 0, (LPARAM)&winpos );
  
  if (!IsWindow( winpos.hwnd )) return;
  if( above == None )			/* absolute bottom */
    {
      WIN_UnlinkWindow( winpos.hwnd );
      WIN_LinkWindow( winpos.hwnd, HWND_BOTTOM);
    }
  else EVENT_QueryZOrder( hWnd );	/* try to outsmart window manager */
}


/***********************************************************************
 *           EVENT_SelectionRequest
 */
static void EVENT_SelectionRequest( HWND hWnd, XSelectionRequestEvent *event )
{
  XSelectionEvent result;
  Atom 	    rprop = None;
  Window    request = event->requestor;

  if(event->target == XA_STRING)
  {
      HANDLE16 hText;
      LPSTR  text;
      int    size,i,j;
      
      rprop = event->property;
      
      if( rprop == None ) 
	  rprop = event->target;
      
      if( event->selection != XA_PRIMARY ) 
	  rprop = None;
      else 
      if( !CLIPBOARD_IsPresent(CF_OEMTEXT) ) 
	  rprop = None;
      else
      {
	  /* open to make sure that clipboard is available */

	  BOOL couldOpen = OpenClipboard( hWnd );
	  char* lpstr = 0;
	  
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
	  
	  TSXChangeProperty(display, request, rprop, 
			    XA_STRING, 8, PropModeReplace, 
			    lpstr, j);
	  HeapFree( GetProcessHeap(), 0, lpstr );
	  
	  /* close only if we opened before */
	  
	  if(couldOpen) CloseClipboard();
      }
  }
  
  if( rprop == None) 
      TRACE(event,"Request for %s ignored\n", TSXGetAtomName(display,event->target));

  /* reply to sender */
  
  result.type = SelectionNotify;
  result.display = display;
  result.requestor = request;
  result.selection = event->selection;
  result.property = rprop;
  result.target = event->target;
  result.time = event->time;
  TSXSendEvent(display,event->requestor,False,NoEventMask,(XEvent*)&result);
}

/***********************************************************************
 *           EVENT_SelectionClear
 */
static void EVENT_SelectionClear( HWND hWnd, XSelectionClearEvent *event )
{
  if (event->selection != XA_PRIMARY) return;
  X11DRV_CLIPBOARD_ReleaseSelection( event->window, hWnd );
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
    WARN(event,"property too large, truncated!\n");
  TRACE(event,"urls=%s\n", p_data);

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
	      TRACE(event, "drop file %s as %s\n", p+5, p_drop);
	      p_drop += len+1;
	    } else {
	      WARN(event, "can't convert file %s to dos name \n", p+5);
	    }
	  } else {
	    WARN(event, "unknown mime type %s\n", p);
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
      SendMessage16( hWnd, WM_SYSCOMMAND, SC_CLOSE, 0 );
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
      TRACE(event, "message_type=%ld, data=%ld,%ld,%ld,%ld,%ld, msg=%s\n",
	    event->message_type, event->data.l[0], event->data.l[1], 
	    event->data.l[2], event->data.l[3], event->data.l[4],
	    p_data);
#endif
      TRACE(event, "unrecognized ClientMessage\n" );
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
      pWnd->dwStyle &= ~WS_MINIMIZE;
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
      pWnd->dwStyle |= WS_MINIMIZE;
  }
  WIN_ReleaseWndPtr(pWnd);
}


/**********************************************************************
 *		X11DRV_EVENT_Pending
 */
BOOL X11DRV_EVENT_Pending()
{
  return TSXPending(display);
}

/**********************************************************************
 *		X11DRV_EVENT_WakeUp
 */
void X11DRV_EVENT_WakeUp(void)
{
    /* wake-up EVENT_WaitNetEvent function, a 32 bit thread post an event
     for a 16 bit task */
    if (write (__wakeup_pipe[1], "A", 1) != 1)
        ERR(event, "unable to write in wakeup_pipe\n");
}


#endif /* !defined(X_DISPLAY_MISSING) */
