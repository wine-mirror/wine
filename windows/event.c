/*
 * X events handling functions
 * 
 * Copyright 1993 Alexandre Julliard
 * 
 */

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "windows.h"
#include "winnt.h"
#include "gdi.h"
#include "heap.h"
#include "queue.h"
#include "win.h"
#include "class.h"
#include "clipboard.h"
#include "message.h"
#include "module.h"
#include "options.h"
#include "queue.h"
#include "winpos.h"
#include "drive.h"
#include "shell.h"
#include "xmalloc.h"
#include "keyboard.h"
#include "stddebug.h"
#include "debug.h"
#include "dde_proc.h"


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

  /* X context to associate a hwnd to an X window */
static XContext winContext = 0;

  /* State variables */
BOOL MouseButtonsStates[NB_BUTTONS];
BOOL AsyncMouseButtonsStates[NB_BUTTONS];
BYTE InputKeyStateTable[256];

static INT16  captureHT = HTCLIENT;
static HWND32 captureWnd = 0;
static BOOL32 InputEnabled = TRUE;

static Atom wmProtocols = None;
static Atom wmDeleteWindow = None;
static Atom dndProtocol = None;
static Atom dndSelection = None;

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

  /* Event handlers */
static void EVENT_key( XKeyEvent *event );
static void EVENT_ButtonPress( XButtonEvent *event );
static void EVENT_ButtonRelease( XButtonEvent *event );
static void EVENT_MotionNotify( XMotionEvent *event );
static void EVENT_FocusIn( HWND hwnd, XFocusChangeEvent *event );
static void EVENT_FocusOut( HWND hwnd, XFocusChangeEvent *event );
static void EVENT_Expose( WND *pWnd, XExposeEvent *event );
static void EVENT_GraphicsExpose( WND *pWnd, XGraphicsExposeEvent *event );
static void EVENT_ConfigureNotify( HWND hwnd, XConfigureEvent *event );
static void EVENT_SelectionRequest( WND *pWnd, XSelectionRequestEvent *event);
static void EVENT_SelectionNotify( XSelectionEvent *event);
static void EVENT_SelectionClear( WND *pWnd, XSelectionClearEvent *event);
static void EVENT_ClientMessage( WND *pWnd, XClientMessageEvent *event );
static void EVENT_MapNotify( HWND hwnd, XMapEvent *event );

/* Usable only with OLVWM - compile option perhaps?
static void EVENT_EnterNotify( WND *pWnd, XCrossingEvent *event );
*/

extern void FOCUS_SetXFocus( HWND32 );
extern BOOL16 DRAG_QueryUpdate( HWND, SEGPTR, BOOL32 );

/***********************************************************************
 *           EVENT_ProcessEvent
 *
 * Process an X event.
 */
void EVENT_ProcessEvent( XEvent *event )
{
    WND *pWnd;
    
    if (XFindContext( display, event->xany.window, winContext,
                      (char **)&pWnd ) != 0)
        return;  /* Not for a registered window */

    dprintf_event( stddeb, "Got event %s for hwnd %04x\n",
                   event_names[event->type], pWnd->hwndSelf );

    switch(event->type)
    {
    case KeyPress:
    case KeyRelease:
        if (InputEnabled)
            EVENT_key( (XKeyEvent*)event );
	break;
	
    case ButtonPress:
        if (InputEnabled)
            EVENT_ButtonPress( (XButtonEvent*)event );
	break;

    case ButtonRelease:
        if (InputEnabled)
            EVENT_ButtonRelease( (XButtonEvent*)event );
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
        if (InputEnabled)
	{
            while (XCheckTypedWindowEvent(display,((XAnyEvent *)event)->window,
                                          MotionNotify, event));    
            EVENT_MotionNotify( (XMotionEvent*)event );
	}
	break;

    case FocusIn:
        EVENT_FocusIn( pWnd->hwndSelf, (XFocusChangeEvent*)event );
	break;

    case FocusOut:
	EVENT_FocusOut( pWnd->hwndSelf, (XFocusChangeEvent*)event );
	break;

    case Expose:
	EVENT_Expose( pWnd, (XExposeEvent *)event );
	break;

    case GraphicsExpose:
	EVENT_GraphicsExpose( pWnd, (XGraphicsExposeEvent *)event );
        break;

    case ConfigureNotify:
	EVENT_ConfigureNotify( pWnd->hwndSelf, (XConfigureEvent*)event );
	break;

    case SelectionRequest:
	EVENT_SelectionRequest( pWnd, (XSelectionRequestEvent *)event );
	break;

    case SelectionNotify:
	EVENT_SelectionNotify( (XSelectionEvent *)event );
	break;

    case SelectionClear:
	EVENT_SelectionClear( pWnd, (XSelectionClearEvent*) event );
	break;

    case ClientMessage:
	EVENT_ClientMessage( pWnd, (XClientMessageEvent *) event );
	break;

/*  case EnterNotify:
 *       EVENT_EnterNotify( pWnd, (XCrossingEvent *) event );
 *       break;
 */
    case NoExpose:
	break;

    /* We get all these because of StructureNotifyMask. */
    case UnmapNotify:
    case CirculateNotify:
    case CreateNotify:
    case DestroyNotify:
    case GravityNotify:
    case ReparentNotify:
	break;

    case MapNotify:
	EVENT_MapNotify( pWnd->hwndSelf, (XMapEvent *)event );
	break;

    default:    
	dprintf_event(stddeb, "Unprocessed event %s for hwnd %04x\n",
	        event_names[event->type], pWnd->hwndSelf );
	break;
    }
}


/***********************************************************************
 *           EVENT_RegisterWindow
 *
 * Associate an X window to a HWND.
 */
void EVENT_RegisterWindow( WND *pWnd )
{
    if (wmProtocols == None)
        wmProtocols = XInternAtom( display, "WM_PROTOCOLS", True );
    if (wmDeleteWindow == None)
        wmDeleteWindow = XInternAtom( display, "WM_DELETE_WINDOW", True );
    if( dndProtocol == None )
	dndProtocol = XInternAtom( display, "DndProtocol" , False );
    if( dndSelection == None )
	dndSelection = XInternAtom( display, "DndSelection" , False );

    XSetWMProtocols( display, pWnd->window, &wmDeleteWindow, 1 );

    if (!winContext) winContext = XUniqueContext();
    XSaveContext( display, pWnd->window, winContext, (char *)pWnd );
}

/***********************************************************************
 *           EVENT_DestroyWindow
 */
void EVENT_DestroyWindow( WND *pWnd )
{
   XEvent xe;

   XDeleteContext( display, pWnd->window, winContext );
   XDestroyWindow( display, pWnd->window );
   while( XCheckWindowEvent(display, pWnd->window, NoEventMask, &xe) );
}

/***********************************************************************
 *           EVENT_WaitXEvent
 *
 * Wait for an X event, optionally sleeping until one arrives.
 * Return TRUE if an event is pending, FALSE on timeout or error
 * (for instance lost connection with the server).
 */
BOOL32 EVENT_WaitXEvent( BOOL32 sleep, BOOL32 peek )
{
    XEvent event;
    LONG maxWait = sleep ? TIMER_GetNextExpiration() : 0;

    /* Wait for an event or a timeout. If maxWait is -1, we have no timeout;
     * in this case, we fall through directly to the XNextEvent loop.
     */

    if ((maxWait != -1) && !XPending(display))
    {
        fd_set read_set;
        struct timeval timeout;
        int fd = ConnectionNumber(display);

        FD_ZERO( &read_set );
        FD_SET( fd, &read_set );

	timeout.tv_usec = (maxWait % 1000) * 1000;
	timeout.tv_sec = maxWait / 1000;

#ifdef CONFIG_IPC
	sigsetjmp(env_wait_x, 1);
	stop_wait_op= CONT;
	    
	if (DDE_GetRemoteMessage()) {
	    while(DDE_GetRemoteMessage())
		;
	    return TRUE;
	}
	stop_wait_op = STOP_WAIT_X;
	/* The code up to the next "stop_wait_op = CONT" must be reentrant */
	if (select( fd+1, &read_set, NULL, NULL, &timeout ) != 1 &&
	    !XPending(display))
        {
	    stop_wait_op = CONT;
            TIMER_ExpireTimers();
	    return FALSE;
	}
        else stop_wait_op = CONT;
#else  /* CONFIG_IPC */
	if (select( fd+1, &read_set, NULL, NULL, &timeout ) != 1)
        {
            /* Timeout or error */
            TIMER_ExpireTimers();
            return FALSE;
        }
#endif  /* CONFIG_IPC */

    }

    /* Process the event (and possibly others that occurred in the meantime) */

    do
    {

#ifdef CONFIG_IPC
        if (DDE_GetRemoteMessage())
        {
            while(DDE_GetRemoteMessage()) ;
            return TRUE;
        }
#endif  /* CONFIG_IPC */

	XNextEvent( display, &event );

        if( peek )
        {
	  WND*		pWnd;
	  MESSAGEQUEUE* pQ;

          if (XFindContext( display, ((XAnyEvent *)&event)->window, winContext,
                            (char **)&pWnd ) || (event.type == NoExpose))
              continue;

	  /* check for the "safe" hardware events */

	  if( event.type == MotionNotify ||
	      event.type == ButtonPress || event.type == ButtonRelease ||
	      event.type == KeyPress || event.type == KeyRelease ||
	      event.type == SelectionRequest || event.type == SelectionClear )
	  {
	       EVENT_ProcessEvent( &event );
               continue;
	  }

	  if( pWnd )
            if( (pQ = (MESSAGEQUEUE*)GlobalLock16(pWnd->hmemTaskQ)) )
            {
              pQ->flags |= QUEUE_FLAG_XEVENT;
              PostEvent(pQ->hTask);
	      XPutBackEvent(display, &event);
              break;
	    }
        }
        else
          EVENT_ProcessEvent( &event );
    }
    while (XPending( display ));
    return TRUE;
}


/***********************************************************************
 *           EVENT_Synchronize
 *
 * Synchronize with the X server. Should not be used too often.
 */
void EVENT_Synchronize()
{
    XEvent event;

    XSync( display, False );
    while (XPending( display ))
    {
	XNextEvent( display, &event );
	EVENT_ProcessEvent( &event );
    }    
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
 *           EVENT_Expose
 */
static void EVENT_Expose( WND *pWnd, XExposeEvent *event )
{
    RECT32 rect;

    /* Make position relative to client area instead of window */
    rect.left   = event->x - (pWnd->rectClient.left - pWnd->rectWindow.left);
    rect.top    = event->y - (pWnd->rectClient.top - pWnd->rectWindow.top);
    rect.right  = rect.left + event->width;
    rect.bottom = rect.top + event->height;

    PAINT_RedrawWindow( pWnd->hwndSelf, &rect, 0,
                        RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN | RDW_ERASE |
                        (event->count ? 0 : RDW_ERASENOW), 0 );
}


/***********************************************************************
 *           EVENT_GraphicsExpose
 *
 * This is needed when scrolling area is partially obscured
 * by non-Wine X window.
 */
static void EVENT_GraphicsExpose( WND *pWnd, XGraphicsExposeEvent *event )
{
    RECT32 rect;

    /* Make position relative to client area instead of window */
    rect.left   = event->x - (pWnd->rectClient.left - pWnd->rectWindow.left);
    rect.top    = event->y - (pWnd->rectClient.top - pWnd->rectWindow.top);
    rect.right  = rect.left + event->width;
    rect.bottom = rect.top + event->height;

    PAINT_RedrawWindow( pWnd->hwndSelf, &rect, 0,
                        RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE |
                        (event->count ? 0 : RDW_ERASENOW), 0 );
}


/***********************************************************************
 *           EVENT_key
 *
 * Handle a X key event
 */

/* Keyboard translation tables */
static const int special_key[] =
{
    VK_BACK, VK_TAB, 0, VK_CLEAR, 0, VK_RETURN, 0, 0,           /* FF08 */
    0, 0, 0, VK_PAUSE, VK_SCROLL, 0, 0, 0,                      /* FF10 */
    0, 0, 0, VK_ESCAPE                                          /* FF18 */
};

static const int cursor_key[] =
{
    VK_HOME, VK_LEFT, VK_UP, VK_RIGHT, VK_DOWN, VK_PRIOR, 
                                       VK_NEXT, VK_END          /* FF50 */
};

static const int misc_key[] =
{
    VK_SELECT, VK_SNAPSHOT, VK_EXECUTE, VK_INSERT, 0, 0, 0, 0,  /* FF60 */
    VK_CANCEL, VK_HELP, VK_CANCEL, VK_MENU                      /* FF68 */
};

static const int keypad_key[] =
{
    0, VK_NUMLOCK,                                        	/* FF7E */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* FF80 */
    0, 0, 0, 0, 0, VK_RETURN, 0, 0,                             /* FF88 */
    0, 0, 0, 0, 0, VK_HOME, VK_LEFT, VK_UP,                     /* FF90 */
    VK_RIGHT, VK_DOWN, VK_PRIOR, VK_NEXT, VK_END, 0,
				 VK_INSERT, VK_DELETE,          /* FF98 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* FFA0 */
    0, 0, VK_MULTIPLY, VK_ADD, VK_SEPARATOR, VK_SUBTRACT, 
                               VK_DECIMAL, VK_DIVIDE,           /* FFA8 */
    VK_NUMPAD0, VK_NUMPAD1, VK_NUMPAD2, VK_NUMPAD3, VK_NUMPAD4,
                            VK_NUMPAD5, VK_NUMPAD6, VK_NUMPAD7, /* FFB0 */
    VK_NUMPAD8, VK_NUMPAD9                                      /* FFB8 */
};
    
static const int function_key[] =
{
    VK_F1, VK_F2,                                               /* FFBE */
    VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8, VK_F9, VK_F10,    /* FFC0 */
    VK_F11, VK_F12, VK_F13, VK_F14, VK_F15, VK_F16              /* FFC8 */
};

static const int modifier_key[] =
{
    VK_SHIFT, VK_SHIFT, VK_CONTROL, VK_CONTROL, VK_CAPITAL, 0, /* FFE1 */
    VK_MENU, VK_MENU, VK_MENU, VK_MENU                         /* FFE7 */
};

static int AltGrMask;
static int min_keycode, max_keycode;
static int keyc2vkey [256];
/* For example : keyc2vkey[10] is the VK_* associated with keycode10 */

static WORD EVENT_event_to_vkey( XKeyEvent *e)
{
  static int keysyms_per_keycode;
  int i;
  KeySym keysym;

  if (!keysyms_per_keycode) /* First time : Initialization */
    {
      KeySym *ksp;
      XModifierKeymap *mmp;
      KeyCode *kcp;
      XKeyEvent e2;
      WORD vkey, OEMvkey;

      XDisplayKeycodes(e->display, &min_keycode, &max_keycode);
      ksp = XGetKeyboardMapping(e->display, min_keycode,
				max_keycode + 1 - min_keycode, &keysyms_per_keycode);
      /* We are only interested in keysyms_per_keycode.
	 There is no need to hold a local copy of the keysyms table */
      XFree(ksp);
      mmp = XGetModifierMapping(e->display);
      kcp = mmp->modifiermap;
      for (i = 0; i < 8; i += 1) /* There are 8 modifier keys */
	{
	  int j;

	  for (j = 0; j < mmp->max_keypermod; j += 1, kcp += 1)
	    if (*kcp)
	      {
		int k;

		for (k = 0; k < keysyms_per_keycode; k += 1)
		  if (XKeycodeToKeysym(e->display, *kcp, k)
		      == XK_Mode_switch)
		    {
		      AltGrMask = 1 << i;
		      dprintf_key(stddeb, "AltGrMask is %x\n", AltGrMask);
		    }
	      }
	}
      XFreeModifiermap(mmp);
	
	/* Now build two conversion arrays :
	 * keycode -> vkey + extended
	 * vkey + extended -> keycode */

      e2.display = e->display;
      e2.state = 0;

      OEMvkey = 0xb9; /* first OEM virtual key available is ba */
      for (e2.keycode=min_keycode; e2.keycode<=max_keycode; e2.keycode++)
	{
	  XLookupString(&e2, NULL, 0, &keysym, NULL);
	  vkey = 0;
	  if (keysym)  /* otherwise, keycode not used */
	    {
	      if ((keysym >> 8) == 0xFF)         /* non-character key */
		{
		  int key = keysym & 0xff;
		
		  if (key >= 0x08 && key <= 0x1B)         /* special key */
		    vkey = special_key[key - 0x08];
		  else if (key >= 0x50 && key <= 0x57)    /* cursor key */
		    vkey = cursor_key[key - 0x50];
		  else if (key >= 0x60 && key <= 0x6B)    /* miscellaneous key */
		    vkey = misc_key[key - 0x60];
		  else if (key >= 0x7E && key <= 0xB9)    /* keypad key */
		    vkey = keypad_key[key - 0x7E];
		  else if (key >= 0xBE && key <= 0xCD)    /* function key */
		    {
		      vkey = function_key[key - 0xBE];
		      vkey |= 0x100; /* set extended bit */
		    }
		  else if (key >= 0xE1 && key <= 0xEA)    /* modifier key */
		    vkey = modifier_key[key - 0xE1];
		  else if (key == 0xFF)                   /* DEL key */
		    vkey = VK_DELETE;
		  /* extended must also be set for ALT_R, CTRL_R,
		     INS, DEL, HOME, END, PAGE_UP, PAGE_DOWN, ARROW keys,
		     keypad / and keypad ENTER (SDK 3.1 Vol.3 p 138) */
		  switch (keysym)
		    {
		    case XK_Control_R :
		    case XK_Alt_R :
		    case XK_Insert :
		    case XK_Delete :
		    case XK_Home :
		    case XK_End :
		    case XK_Prior :
		    case XK_Next :
		    case XK_Left :
		    case XK_Up :
		    case XK_Right :
		    case XK_Down :
		    case XK_KP_Divide :
		    case XK_KP_Enter :
		      vkey |= 0x100;
		    }
		}
	      for (i = 0; (i < keysyms_per_keycode) && (!vkey); i++)
		{
		  keysym = XLookupKeysym(&e2, i);
		  if ((keysym >= VK_0 && keysym <= VK_9)
		      || (keysym >= VK_A && keysym <= VK_Z)
		      || keysym == VK_SPACE)
		    vkey = keysym;
		}

	      if (!vkey)
		{
		  /* Others keys: let's assign OEM virtual key codes in the allowed range,
		   * that is ([0xba,0xc0], [0xdb,0xe4], 0xe6 (given up) et [0xe9,0xf5]) */
		  switch (++OEMvkey)
		    {
		    case 0xc1 : OEMvkey=0xdb; break;
		    case 0xe5 : OEMvkey=0xe9; break;
		    case 0xf6 : OEMvkey=0xf5; fprintf(stderr,"No more OEM vkey available!\n");
		    }
		  
		  vkey = OEMvkey;
		  
		  if (debugging_keyboard)
		    {
		      fprintf(stddeb,"OEM specific virtual key %X assigned to keycode %X :\n ("
			      ,OEMvkey,e2.keycode);
		      for (i = 0; i < keysyms_per_keycode; i += 1)
			{
			  char	*ksname;
			  
			  keysym = XLookupKeysym(&e2, i);
			  ksname = XKeysymToString(keysym);
			  if (!ksname)
			    ksname = "NoSymbol";
			  fprintf(stddeb, "%lX (%s) ", keysym, ksname);
			}
		      fprintf(stddeb, ")\n");
		    }
		}
	    }
	  keyc2vkey[e2.keycode] = vkey;
	} /* for */
    } /* Initialization */
  
  return keyc2vkey[e->keycode];
  
}

int EVENT_ToAscii(WORD wVirtKey, WORD wScanCode, LPSTR lpKeyState, 
	LPVOID lpChar, WORD wFlags) 
{
    XKeyEvent e;
    KeySym keysym;
    static XComposeStatus cs;
    int ret;
    WORD keyc;

    e.display = display;
    e.keycode = 0;
    for (keyc=min_keycode; keyc<=max_keycode; keyc++)
      { /* this could be speed up by making another table, an array of struct vkey,keycode
	 * (vkey -> keycode) with vkeys sorted .... but it takes memory (512*3 bytes)!  DF */
	if ((keyc2vkey[keyc] & 0xFF)== wVirtKey) /* no need to make a more precise test (with the extended bit correctly set above wVirtKey ... VK* are different enough... */
	  {
	    if ((e.keycode) && ((wVirtKey<0x10) || (wVirtKey>0x12))) 
		/* it's normal to have 2 shift, control, and alt ! */
		dprintf_keyboard(stddeb,"Strange ... the keycodes %X and %X are matching!\n",
				 e.keycode,keyc);
	    e.keycode = keyc;
	  }
      }
    if (!e.keycode) 
      {
	fprintf(stderr,"Unknown virtual key %X !!! \n",wVirtKey);
	return wVirtKey; /* whatever */
      }
    e.state = 0;
    if (lpKeyState[VK_SHIFT] & 0x80)
	e.state |= ShiftMask;
    if (lpKeyState[VK_CAPITAL] & 0x80)
	e.state |= LockMask;
    if (lpKeyState[VK_CONTROL] & 0x80)
	if (lpKeyState[VK_MENU] & 0x80)
	    e.state |= AltGrMask;
	else
	    e.state |= ControlMask;
    if (lpKeyState[VK_NUMLOCK] & 0x80)
	e.state |= Mod2Mask;
    dprintf_key(stddeb, "EVENT_ToAscii(%04X, %04X) : faked state = %X\n",
		wVirtKey, wScanCode, e.state);
    ret = XLookupString(&e, lpChar, 2, &keysym, &cs);
    if (ret == 0)
	{
	BYTE dead_char = 0;

	((char*)lpChar)[1] = '\0';
	switch (keysym)
	    {
	    case XK_dead_tilde :
	    case 0x1000FE7E : /* Xfree's XK_Dtilde */
		dead_char = '~';
		break;
	    case XK_dead_acute :
	    case 0x1000FE27 : /* Xfree's XK_Dacute_accent */
		dead_char = 0xb4;
		break;
	    case XK_dead_circumflex :
	    case 0x1000FE5E : /* Xfree's XK_Dcircumflex_accent */
		dead_char = '^';
		break;
	    case XK_dead_grave :
	    case 0x1000FE60 : /* Xfree's XK_Dgrave_accent */
		dead_char = '`';
		break;
	    case XK_dead_diaeresis :
	    case 0x1000FE22 : /* Xfree's XK_Ddiaeresis */
		dead_char = 0xa8;
		break;
	    }
	if (dead_char)
	    {
	    *(char*)lpChar = dead_char;
	    ret = -1;
	    }
	else
	    {
	    char	*ksname;

	    ksname = XKeysymToString(keysym);
	    if (!ksname)
		ksname = "No Name";
	    if ((keysym >> 8) != 0xff)
		{
		fprintf(stderr, "Please report : no char for keysym %04lX (%s) :\n",
			keysym, ksname);
		fprintf(stderr, "  wVirtKey = %X, wScanCode = %X, keycode = %X, state = %X\n",
			wVirtKey, wScanCode, e.keycode, e.state);
		}
	    }
	}
    dprintf_key(stddeb, "EVENT_ToAscii about to return %d with char %x\n",
		ret, *(char*)lpChar);
    return ret;
}

typedef union
{
    struct
    {
	unsigned long count : 16;
	unsigned long code : 8;
	unsigned long extended : 1;
	unsigned long unused : 2;
	unsigned long win_internal : 2;
	unsigned long context : 1;
	unsigned long previous : 1;
	unsigned long transition : 1;
    } lp1;
    unsigned long lp2;
} KEYLP;

static void EVENT_key( XKeyEvent *event )
{
    char Str[24]; 
    XComposeStatus cs; 
    KeySym keysym;
    WORD vkey = 0;
    KEYLP keylp;
    WORD message;
    static BOOL force_extended = FALSE; /* hack for AltGr translation */

    int ascii_chars = XLookupString(event, Str, 1, &keysym, &cs);

    dprintf_key(stddeb, "EVENT_key : state = %X\n", event->state);
    if (keysym == XK_Mode_switch)
	{
	dprintf_key(stddeb, "Alt Gr key event received\n");
	event->keycode = XKeysymToKeycode(event->display, XK_Control_L);
	dprintf_key(stddeb, "Control_L is keycode 0x%x\n", event->keycode);
	EVENT_key(event);
	event->keycode = XKeysymToKeycode(event->display, XK_Alt_L);
	dprintf_key(stddeb, "Alt_L is keycode 0x%x\n", event->keycode);
	force_extended = TRUE;
	EVENT_key(event);
	force_extended = FALSE;
	return;
	}

    Str[ascii_chars] = '\0';
    if (debugging_key)
	{
	char	*ksname;

	ksname = XKeysymToString(keysym);
	if (!ksname)
	    ksname = "No Name";
	fprintf(stddeb, "%s : keysym=%lX (%s), ascii chars=%u / %X / '%s'\n", 
	    event_names[event->type], keysym, ksname,
	    ascii_chars, Str[0] & 0xff, Str);
	}

#if 0
    /* Ctrl-Alt-Return enters the debugger */
    if ((keysym == XK_Return) && (event->type == KeyPress) &&
        (event->state & ControlMask) && (event->state & Mod1Mask))
        DEBUG_EnterDebugger();
#endif

    vkey = EVENT_event_to_vkey(event);
    if (force_extended) vkey |= 0x100;

    dprintf_key(stddeb, "keycode 0x%x converted to vkey 0x%x\n",
		    event->keycode, vkey);

    keylp.lp1.count = 1;
    keylp.lp1.code = LOBYTE(event->keycode) - 8;
    keylp.lp1.extended = (vkey & 0x100 ? 1 : 0);
    keylp.lp1.win_internal = 0; /* this has something to do with dialogs, 
				* don't remember where I read it - AK */
				/* it's '1' under windows, when a dialog box appears
				 * and you press one of the underlined keys - DF*/
    vkey &= 0xff;
    if (event->type == KeyPress)
    {
	keylp.lp1.previous = (InputKeyStateTable[vkey] & 0x80) != 0;
        if (!(InputKeyStateTable[vkey] & 0x80))
            InputKeyStateTable[vkey] ^= 0x01;
	InputKeyStateTable[vkey] |= 0x80;
	keylp.lp1.transition = 0;
	message = (InputKeyStateTable[VK_MENU] & 0x80)
		    && !(InputKeyStateTable[VK_CONTROL] & 0x80)
			 ? WM_SYSKEYDOWN : WM_KEYDOWN;
    }
    else
    {
	UINT sysKey = (InputKeyStateTable[VK_MENU] & 0x80)
			&& !(InputKeyStateTable[VK_CONTROL] & 0x80)
			&& (force_extended == FALSE); /* for Alt from AltGr */

	InputKeyStateTable[vkey] &= ~0x80; 
	keylp.lp1.previous = 1;
	keylp.lp1.transition = 1;
	message = sysKey ? WM_SYSKEYUP : WM_KEYUP;
    }
    keylp.lp1.context = ( (event->state & Mod1Mask)  || 
			  (InputKeyStateTable[VK_MENU] & 0x80)) ? 1 : 0;
    dprintf_key(stddeb,"            wParam=%04X, lParam=%08lX\n", 
		vkey, keylp.lp2 );
    dprintf_key(stddeb,"            InputKeyState=%X\n",
		InputKeyStateTable[vkey]);

    hardware_event( message, vkey, keylp.lp2, event->x_root - desktopX,
	    event->y_root - desktopY, event->time - MSG_WineStartTicks, 0 );
}


/***********************************************************************
 *           EVENT_MotionNotify
 */
static void EVENT_MotionNotify( XMotionEvent *event )
{
    hardware_event( WM_MOUSEMOVE, EVENT_XStateToKeyState( event->state ), 0L,
		    event->x_root - desktopX, event->y_root - desktopY,
		    event->time - MSG_WineStartTicks, 0 );
}


/***********************************************************************
 *           EVENT_DummyMotionNotify
 *
 * Generate a dummy MotionNotify event. Used to force a WM_SETCURSOR message.
 */
void EVENT_DummyMotionNotify(void)
{
    Window root, child;
    int rootX, rootY, childX, childY;
    unsigned int state;

    if (XQueryPointer( display, rootWindow, &root, &child,
                       &rootX, &rootY, &childX, &childY, &state ))
    {
        hardware_event(WM_MOUSEMOVE, EVENT_XStateToKeyState( state ), 0L,
                       rootX - desktopX, rootY - desktopY, GetTickCount(), 0 );
    }
}


/***********************************************************************
 *           EVENT_ButtonPress
 */
static void EVENT_ButtonPress( XButtonEvent *event )
{
    static WORD messages[NB_BUTTONS] = 
        { WM_LBUTTONDOWN, WM_MBUTTONDOWN, WM_RBUTTONDOWN };
    int buttonNum = event->button - 1;

    if (buttonNum >= NB_BUTTONS) return;
    MouseButtonsStates[buttonNum] = 0x8000;
    AsyncMouseButtonsStates[buttonNum] = 0x8000;
    hardware_event( messages[buttonNum],
		    EVENT_XStateToKeyState( event->state ), 0L,
		    event->x_root - desktopX, event->y_root - desktopY,
		    event->time - MSG_WineStartTicks, 0 );
}


/***********************************************************************
 *           EVENT_ButtonRelease
 */
static void EVENT_ButtonRelease( XButtonEvent *event )
{
    static const WORD messages[NB_BUTTONS] = 
        { WM_LBUTTONUP, WM_MBUTTONUP, WM_RBUTTONUP };
    int buttonNum = event->button - 1;

    if (buttonNum >= NB_BUTTONS) return;    
    MouseButtonsStates[buttonNum] = FALSE;
    hardware_event( messages[buttonNum],
		    EVENT_XStateToKeyState( event->state ), 0L,
		    event->x_root - desktopX, event->y_root - desktopY,
		    event->time - MSG_WineStartTicks, 0 );
}


/**********************************************************************
 *              EVENT_FocusIn
 */
static void EVENT_FocusIn (HWND hwnd, XFocusChangeEvent *event )
{
    if (event->detail == NotifyPointer) return;
    if (hwnd != GetActiveWindow()) WINPOS_ChangeActiveWindow( hwnd, FALSE );
    if ((hwnd != GetFocus32()) && !IsChild( hwnd, GetFocus32()))
        SetFocus32( hwnd );
}


/**********************************************************************
 *              EVENT_FocusOut
 *
 * Note: only top-level override-redirect windows get FocusOut events.
 */
static void EVENT_FocusOut( HWND hwnd, XFocusChangeEvent *event )
{
    if (event->detail == NotifyPointer) return;
    if (hwnd == GetActiveWindow()) WINPOS_ChangeActiveWindow( 0, FALSE );
    if ((hwnd == GetFocus32()) || IsChild( hwnd, GetFocus32()))
        SetFocus32( 0 );
}


/**********************************************************************
 *              EVENT_ConfigureNotify
 *
 * The ConfigureNotify event is only selected on the desktop window
 * and on top-level windows when the -managed flag is used.
 */
static void EVENT_ConfigureNotify( HWND hwnd, XConfigureEvent *event )
{
    /* FIXME: with -desktop xxx we get this event _before_ desktop 
     * window structure is created. WIN_GetDesktop() check is a hack.
     */

    if ( !WIN_GetDesktop() || hwnd == GetDesktopWindow32())
    {
        desktopX = event->x;
	desktopY = event->y;
    }
    else
    {
        WND *wndPtr;
	WINDOWPOS16 *winpos;
	RECT16 newWindowRect, newClientRect;
	HRGN32 hrgnOldPos, hrgnNewPos;

	if (!(wndPtr = WIN_FindWndPtr( hwnd )) ||
	    !(wndPtr->flags & WIN_MANAGED) )
	    return;
	
        if (!(winpos = SEGPTR_NEW(WINDOWPOS16))) return;

/*	XTranslateCoordinates(display, event->window, rootWindow,
	    event->x, event->y, &event->x, &event->y, &child);
            */

	/* Fill WINDOWPOS struct */
	winpos->flags = SWP_NOACTIVATE | SWP_NOZORDER;
	winpos->hwnd = hwnd;
	winpos->x = event->x;
	winpos->y = event->y;
	winpos->cx = event->width;
	winpos->cy = event->height;

	/* Check for unchanged attributes */
	if(winpos->x == wndPtr->rectWindow.left &&
	   winpos->y == wndPtr->rectWindow.top)
	    winpos->flags |= SWP_NOMOVE;
	if(winpos->cx == wndPtr->rectWindow.right - wndPtr->rectWindow.left &&
	   winpos->cy == wndPtr->rectWindow.bottom - wndPtr->rectWindow.top)
	    winpos->flags |= SWP_NOSIZE;

	/* Send WM_WINDOWPOSCHANGING */
	SendMessage16(hwnd, WM_WINDOWPOSCHANGING, 0, (LPARAM)SEGPTR_GET(winpos));

	/* Calculate new position and size */
	newWindowRect.left = event->x;
	newWindowRect.right = event->x + event->width;
	newWindowRect.top = event->y;
	newWindowRect.bottom = event->y + event->height;

	WINPOS_SendNCCalcSize( winpos->hwnd, TRUE, &newWindowRect,
			       &wndPtr->rectWindow, &wndPtr->rectClient,
			       SEGPTR_GET(winpos), &newClientRect );

        hrgnOldPos = CreateRectRgnIndirect16( &wndPtr->rectWindow );
        hrgnNewPos = CreateRectRgnIndirect16( &newWindowRect );
        CombineRgn32( hrgnOldPos, hrgnOldPos, hrgnNewPos, RGN_DIFF );
 
	/* Set new size and position */
	wndPtr->rectWindow = newWindowRect;
	wndPtr->rectClient = newClientRect;
	SendMessage16( hwnd, WM_WINDOWPOSCHANGED, 0, (LPARAM)SEGPTR_GET(winpos));
        SEGPTR_FREE(winpos);

        /* full window drag leaves unrepainted garbage without this */
        PAINT_RedrawWindow( 0, NULL, hrgnOldPos, RDW_INVALIDATE |
                            RDW_ALLCHILDREN | RDW_ERASE | RDW_ERASENOW,
                            RDW_C_USEHRGN );
        DeleteObject32(hrgnOldPos);
        DeleteObject32(hrgnNewPos);
    }
}


/***********************************************************************
 *           EVENT_SelectionRequest
 */
static void EVENT_SelectionRequest( WND *pWnd, XSelectionRequestEvent *event )
{
    XSelectionEvent result;
    Atom 	    rprop = None;
    Window 	    request = event->requestor;

    if(event->target == XA_STRING)
    {
	HANDLE16 hText;
	LPSTR  text;
	int    size,i,j;

        rprop = event->property;

	if(rprop == None) rprop = event->target;

        if(event->selection!=XA_PRIMARY) rprop = None;
        else if(!CLIPBOARD_IsPresent(CF_OEMTEXT)) rprop = None;
	else
	{
            /* open to make sure that clipboard is available */

	    BOOL 	couldOpen = OpenClipboard( pWnd->hwndSelf );
	    char*	lpstr = 0;

	    hText = GetClipboardData(CF_TEXT);
	    text = GlobalLock16(hText);
	    size = GlobalSize16(hText);

	    /* remove carriage returns */

	    lpstr = (char*)xmalloc(size--);
	    for(i=0,j=0; i < size && text[i]; i++ )
	    {
	       if( text[i] == '\r' && 
		  (text[i+1] == '\n' || text[i+1] == '\0') ) continue;
	       lpstr[j++] = text[i];
	    }
	    lpstr[j]='\0';

	    XChangeProperty(display, request, rprop, 
			    XA_STRING, 8, PropModeReplace, 
			    lpstr, j);
	    free(lpstr);

	    /* close only if we opened before */

	    if(couldOpen) CloseClipboard();
	}
    }

    if(rprop==None) 
       dprintf_event(stddeb,"Request for %s ignored\n", XGetAtomName(display,event->target));

    result.type=SelectionNotify;
    result.display=display;
    result.requestor=request;
    result.selection=event->selection;
    result.property=rprop;
    result.target=event->target;
    result.time=event->time;
    XSendEvent(display,event->requestor,False,NoEventMask,(XEvent*)&result);
}


/***********************************************************************
 *           EVENT_SelectionNotify
 */
static void EVENT_SelectionNotify( XSelectionEvent *event )
{
    if (event->selection != XA_PRIMARY) return;

    if (event->target != XA_STRING) CLIPBOARD_ReadSelection( 0, None );
    else CLIPBOARD_ReadSelection( event->requestor, event->property );

    dprintf_clipboard(stddeb,"\tSelectionNotify done!\n");
}


/***********************************************************************
 *           EVENT_SelectionClear
 */
static void EVENT_SelectionClear( WND *pWnd, XSelectionClearEvent *event )
{
    if (event->selection != XA_PRIMARY) return;
    CLIPBOARD_ReleaseSelection( event->window, pWnd->hwndSelf ); 
}


/**********************************************************************
 *           EVENT_ClientMessage
 */
static void EVENT_ClientMessage( WND *pWnd, XClientMessageEvent *event )
{
    if (event->message_type != None && event->format == 32)
    {
       if ((event->message_type == wmProtocols) && 
           (((Atom) event->data.l[0]) == wmDeleteWindow))
	  SendMessage16( pWnd->hwndSelf, WM_SYSCOMMAND, SC_CLOSE, 0 );
       else if ( event->message_type == dndProtocol &&
		(event->data.l[0] == DndFile || event->data.l[0] == DndFiles) )
       {
	  unsigned long		data_length;
	  unsigned long		aux_long;
	  unsigned char*	p_data = NULL;
	  union {
		   Atom		atom_aux;
		   POINT32	pt_aux;
		   int		i;
		}		u;
	  int			x, y;
          BOOL16	        bAccept;
	  HGLOBAL16		hDragInfo = GlobalAlloc16( GMEM_SHARE | GMEM_ZEROINIT, sizeof(DRAGINFO));
	  LPDRAGINFO            lpDragInfo = (LPDRAGINFO) GlobalLock16(hDragInfo);
	  SEGPTR		spDragInfo = (SEGPTR) WIN16_GlobalLock16(hDragInfo);
	  Window		w_aux_root, w_aux_child;
	  WND*			pDropWnd;
	  
          if( !lpDragInfo || !spDragInfo ) return;

	  XQueryPointer( display, pWnd->window, &w_aux_root, &w_aux_child, 
		 &x, &y, &u.pt_aux.x, &u.pt_aux.y, (unsigned int*)&aux_long);

          lpDragInfo->hScope = pWnd->hwndSelf;
          lpDragInfo->pt.x = (INT16)x; lpDragInfo->pt.y = (INT16)y;

	  /* find out drop point and drop window */
	  if( x < 0 || y < 0 ||
	      x > (pWnd->rectWindow.right - pWnd->rectWindow.left) ||
	      y > (pWnd->rectWindow.bottom - pWnd->rectWindow.top) )
	  {   bAccept = pWnd->dwExStyle & WS_EX_ACCEPTFILES; x = y = 0; }
	  else
	  {
	      bAccept = DRAG_QueryUpdate( pWnd->hwndSelf, spDragInfo, TRUE );
	      x = lpDragInfo->pt.x; y = lpDragInfo->pt.y;
	  }
	  pDropWnd = WIN_FindWndPtr( lpDragInfo->hScope );
	  GlobalFree16( hDragInfo );

	  if( bAccept )
	  {
	      XGetWindowProperty( display, DefaultRootWindow(display),
			      dndSelection, 0, 65535, FALSE,
                              AnyPropertyType, &u.atom_aux, &u.pt_aux.y,
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
                      INT32 len = GetShortPathName32A( p, NULL, 0 );
                      if (len) aux_long += len + 1;
		      else *p = -1;
		  }
		  p += strlen(p) + 1;
		}
		if( aux_long && aux_long < 65535 )
		{
		  HDROP16                 hDrop;
		  LPDROPFILESTRUCT        lpDrop;

		  aux_long += sizeof(DROPFILESTRUCT) + 1; 
		  hDrop = (HDROP16)GlobalAlloc16( GMEM_SHARE, aux_long );
		  lpDrop = (LPDROPFILESTRUCT) GlobalLock16( hDrop );

		  if( lpDrop )
		  {
		    lpDrop->wSize = sizeof(DROPFILESTRUCT);
		    lpDrop->ptMousePos.x = (INT16)x;
		    lpDrop->ptMousePos.y = (INT16)y;
		    lpDrop->fInNonClientArea = (BOOL16) 
			( x < (pDropWnd->rectClient.left - pDropWnd->rectWindow.left)  ||
			  y < (pDropWnd->rectClient.top - pDropWnd->rectWindow.top)    ||
			  x > (pDropWnd->rectClient.right - pDropWnd->rectWindow.left) ||
			  y > (pDropWnd->rectClient.bottom - pDropWnd->rectWindow.top) );
		    p_drop = ((char*)lpDrop) + sizeof(DROPFILESTRUCT);
		    p = p_data;
		    while(*p)
		    {
		      if( *p != -1 )	/* use only "good" entries */
		      {
                          GetShortPathName32A( p, p_drop, 65535 );
                          p_drop += strlen( p_drop ) + 1;
		      }
		      p += strlen(p) + 1;
		    }
		    *p_drop = '\0';
		    PostMessage( pWnd->hwndSelf, WM_DROPFILES, (WPARAM16)hDrop, 0L );
		  }
	        }
	      }
	      if( p_data ) XFree(p_data);  

	  } /* WS_EX_ACCEPTFILES */
       } /* dndProtocol */
       else
	  dprintf_event( stddeb, "unrecognized ClientMessage\n" );
    }
}

/**********************************************************************
 *           EVENT_EnterNotify
 *
 * Install colormap when Wine window is focused in
 * self-managed mode with private colormap
 */
/*
  void EVENT_EnterNotify( WND *pWnd, XCrossingEvent *event )
  {
   if( !Options.managed && rootWindow == DefaultRootWindow(display) &&
     (COLOR_GetSystemPaletteFlags() & COLOR_PRIVATE) && GetFocus32() )
      XInstallColormap( display, COLOR_GetColormap() );
  }
 */ 

/**********************************************************************
 *		EVENT_MapNotify
 */
void EVENT_MapNotify( HWND hWnd, XMapEvent *event )
{
    HWND32 hwndFocus = GetFocus32();

    if (hwndFocus && IsChild( hWnd, hwndFocus ))
      FOCUS_SetXFocus( (HWND32)hwndFocus );

    return;
}

/**********************************************************************
 *              EVENT_Capture
 * 
 * We need this to be able to generate double click messages
 * when menu code captures mouse in the window without CS_DBLCLK style.
 */
HWND32 EVENT_Capture(HWND32 hwnd, INT16 ht)
{
    Window win;
    HWND32 old_capture_wnd = captureWnd;

    if (!hwnd)
    {
        ReleaseCapture();
        return old_capture_wnd;
    }
    if ((win = WIN_GetXWindow( hwnd )))
    {
        if (XGrabPointer(display, win, False,
                     ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                     GrabModeAsync, GrabModeAsync,
                     None, None, CurrentTime ) == GrabSuccess)
	{
            dprintf_win(stddeb, "SetCapture: %04x\n", hwnd);
            captureWnd   = hwnd;
	    captureHT    = ht;
            return old_capture_wnd;
        }
    }
    return 0;
}

/**********************************************************************
 *              EVENT_GetCaptureInfo
 */
INT16 EVENT_GetCaptureInfo()
{
    return captureHT;
}

/**********************************************************************
 *		SetCapture16   (USER.18)
 */
HWND16 SetCapture16( HWND16 hwnd )
{
    return (HWND16)EVENT_Capture( hwnd, HTCLIENT );
}


/**********************************************************************
 *		SetCapture32   (USER32.463)
 */
HWND32 SetCapture32( HWND32 hwnd )
{
    return EVENT_Capture( hwnd, HTCLIENT );
}


/**********************************************************************
 *		ReleaseCapture   (USER.19) (USER32.438)
 */
void ReleaseCapture(void)
{
    if (captureWnd == 0) return;
    XUngrabPointer( display, CurrentTime );
    captureWnd = 0;
    dprintf_win(stddeb, "ReleaseCapture\n");
}


/**********************************************************************
 *		GetCapture16   (USER.236)
 */
HWND16 GetCapture16(void)
{
    return (HWND16)captureWnd;
}


/**********************************************************************
 *		GetCapture32   (USER32.207)
 */
HWND32 GetCapture32(void)
{
    return captureWnd;
}


/***********************************************************************
 *           GetMouseEventProc   (USER.337)
 */
FARPROC16 GetMouseEventProc(void)
{
    HMODULE16 hmodule = GetModuleHandle("USER");
    return MODULE_GetEntryPoint( hmodule,
                                 MODULE_GetOrdinal( hmodule, "Mouse_Event" ) );
}


/***********************************************************************
 *           Mouse_Event   (USER.299)
 */
void Mouse_Event( CONTEXT *context )
{
    /* Register values:
     * AX = mouse event
     * BX = horizontal displacement if AX & ME_MOVE
     * CX = vertical displacement if AX & ME_MOVE
     * DX = button state (?)
     * SI = mouse event flags (?)
     */
    Window root, child;
    int rootX, rootY, childX, childY;
    unsigned int state;

    if (AX_reg(context) & ME_MOVE)
    {
        /* We have to actually move the cursor */
        XWarpPointer( display, rootWindow, None, 0, 0, 0, 0,
                      (short)BX_reg(context), (short)CX_reg(context) );
        return;
    }
    if (!XQueryPointer( display, rootWindow, &root, &child,
                        &rootX, &rootY, &childX, &childY, &state )) return;
    if (AX_reg(context) & ME_LDOWN)
        hardware_event( WM_LBUTTONDOWN, EVENT_XStateToKeyState( state ), 0L,
                        rootX - desktopX, rootY - desktopY, GetTickCount(), 0);
    if (AX_reg(context) & ME_LUP)
        hardware_event( WM_LBUTTONUP, EVENT_XStateToKeyState( state ), 0L,
                        rootX - desktopX, rootY - desktopY, GetTickCount(), 0);
    if (AX_reg(context) & ME_RDOWN)
        hardware_event( WM_RBUTTONDOWN, EVENT_XStateToKeyState( state ), 0L,
                        rootX - desktopX, rootY - desktopY, GetTickCount(), 0);
    if (AX_reg(context) & ME_RUP)
        hardware_event( WM_RBUTTONUP, EVENT_XStateToKeyState( state ), 0L,
                        rootX - desktopX, rootY - desktopY, GetTickCount(), 0);
}


/**********************************************************************
 *			EnableHardwareInput   (USER.331)
 */
BOOL16 EnableHardwareInput(BOOL16 bEnable)
{
  BOOL16 bOldState = InputEnabled;
  dprintf_event(stdnimp,"EnableHardwareInput(%d);\n", bEnable);
  InputEnabled = bEnable;
  return bOldState;
}

