/*
 * X events handling functions
 * 
 * Copyright 1993 Alexandre Julliard
 * 
 */

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include "gdi.h"
#include "windows.h"
#include "win.h"
#include "class.h"
#include "message.h"
#include "clipboard.h"
#include "options.h"
#include "winpos.h"
#include "registers.h"
#include "stackframe.h"
#include "stddebug.h"
/* #define DEBUG_EVENT */
/* #define DEBUG_KEY   */
#include "debug.h"


#ifdef ndef
#ifndef FamilyAmoeba
typedef char *XPointer;
#endif
#endif

#ifdef WHO_NEEDS_DIRTY_HACKS
#ifdef sparc
/* Dirty hack to compile with Sun's OpenWindows */
typedef char *XPointer;
#endif
#endif

#define NB_BUTTONS      3     /* Windows can handle 3 buttons */

  /* X context to associate a hwnd to an X window */
static XContext winContext = 0;

  /* State variables */
BOOL MouseButtonsStates[NB_BUTTONS] = { FALSE, FALSE, FALSE };
BOOL AsyncMouseButtonsStates[NB_BUTTONS] = { FALSE, FALSE, FALSE };
BYTE KeyStateTable[256];
BYTE AsyncKeyStateTable[256];
static WORD ALTKeyState;
static HWND captureWnd = 0;
static BOOL	InputEnabled = TRUE;

/* Keyboard translation tables */
static int special_key[] =
{
    VK_BACK, VK_TAB, 0, VK_CLEAR, 0, VK_RETURN, 0, 0,           /* FF08 */
    0, 0, 0, VK_PAUSE, VK_SCROLL, 0, 0, 0,                      /* FF10 */
    0, 0, 0, VK_ESCAPE                                          /* FF18 */
};

static cursor_key[] =
{
    VK_HOME, VK_LEFT, VK_UP, VK_RIGHT, VK_DOWN, VK_PRIOR, 
                                       VK_NEXT, VK_END          /* FF50 */
};

static misc_key[] =
{
    VK_SELECT, VK_SNAPSHOT, VK_EXECUTE, VK_INSERT, 0, 0, 0, 0,  /* FF60 */
    VK_CANCEL, VK_HELP, VK_CANCEL, VK_MENU                      /* FF68 */
};

static keypad_key[] =
{
    VK_MENU, VK_NUMLOCK,                                        /* FF7E */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* FF80 */
    0, 0, 0, 0, 0, VK_RETURN, 0, 0,                             /* FF88 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* FF90 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* FF98 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* FFA0 */
    0, 0, VK_MULTIPLY, VK_ADD, VK_SEPARATOR, VK_SUBTRACT, 
                               VK_DECIMAL, VK_DIVIDE,           /* FFA8 */
    VK_NUMPAD0, VK_NUMPAD1, VK_NUMPAD2, VK_NUMPAD3, VK_NUMPAD4,
                            VK_NUMPAD5, VK_NUMPAD6, VK_NUMPAD7, /* FFB0 */
    VK_NUMPAD8, VK_NUMPAD9                                      /* FFB8 */
};
    
static function_key[] =
{
    VK_F1, VK_F2,                                               /* FFBE */
    VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8, VK_F9, VK_F10,    /* FFC0 */
    VK_F11, VK_F12, VK_F13, VK_F14, VK_F15, VK_F16              /* FFC8 */
};

static modifier_key[] =
{
    VK_SHIFT, VK_SHIFT, VK_CONTROL, VK_CONTROL, VK_CAPITAL,
                                                0, 0,           /* FFE1 */
    0, VK_MENU, VK_MENU                                         /* FFE8 */
};

typedef union
{
    struct
    {
	unsigned long count : 16;
	unsigned long code : 8;
	unsigned long extended : 1;
	unsigned long : 4;
	unsigned long context : 1;
	unsigned long previous : 1;
	unsigned long transition : 1;
    } lp1;
    unsigned long lp2;
} KEYLP;

static BOOL KeyDown = FALSE;

static const char *event_names[] =
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
static void EVENT_Expose( HWND hwnd, XExposeEvent *event );
static void EVENT_ConfigureNotify( HWND hwnd, XConfigureEvent *event );
static void EVENT_SelectionRequest( HWND hwnd, XSelectionRequestEvent *event);
static void EVENT_SelectionNotify( HWND hwnd, XSelectionEvent *event);
static void EVENT_SelectionClear( HWND hwnd, XSelectionClearEvent *event);
static void EVENT_ClientMessage( HWND hwnd, XClientMessageEvent *event );


/***********************************************************************
 *           EVENT_ProcessEvent
 *
 * Process an X event.
 */
void EVENT_ProcessEvent( XEvent *event )
{
    HWND hwnd;
    XPointer ptr;
    
    XFindContext( display, ((XAnyEvent *)event)->window, winContext, &ptr );
    hwnd = (HWND) (int)ptr;

    dprintf_event(stddeb, "Got event %s for hwnd "NPFMT"\n",
		  event_names[event->type], hwnd );

    switch(event->type)
    {
    case KeyPress:
    case KeyRelease:
	EVENT_key( (XKeyEvent*)event );
	break;
	
    case ButtonPress:
	EVENT_ButtonPress( (XButtonEvent*)event );
	break;

    case ButtonRelease:
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
        while (XCheckTypedWindowEvent(display, ((XAnyEvent *)event)->window,
			  MotionNotify, event));    
	EVENT_MotionNotify( (XMotionEvent*)event );
	break;

    case FocusIn:
        EVENT_FocusIn( hwnd, (XFocusChangeEvent*)event );
	break;

    case FocusOut:
	EVENT_FocusOut( hwnd, (XFocusChangeEvent*)event );
	break;

    case Expose:
	EVENT_Expose( hwnd, (XExposeEvent*)event );
	break;

    case ConfigureNotify:
	EVENT_ConfigureNotify( hwnd, (XConfigureEvent*)event );
	break;

    case SelectionRequest:
	EVENT_SelectionRequest( hwnd, (XSelectionRequestEvent*)event );
	break;

    case SelectionNotify:
	EVENT_SelectionNotify( hwnd, (XSelectionEvent*)event );
	break;

    case SelectionClear:
	EVENT_SelectionClear( hwnd, (XSelectionClearEvent*) event );
	break;

    case ClientMessage:
	EVENT_ClientMessage( hwnd, (XClientMessageEvent *) event );
	break;

    default:    
	dprintf_event(stddeb, "Unprocessed event %s for hwnd "NPFMT"\n",
	        event_names[event->type], hwnd );
	break;
    }
}


/***********************************************************************
 *           EVENT_RegisterWindow
 *
 * Associate an X window to a HWND.
 */
void EVENT_RegisterWindow( Window w, HWND hwnd )
{
    if (!winContext) winContext = XUniqueContext();
    XSaveContext( display, w, winContext, (XPointer)(int)hwnd );
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
static void EVENT_Expose( HWND hwnd, XExposeEvent *event )
{
    RECT rect;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return;

      /* Make position relative to client area instead of window */
    rect.left = event->x - (wndPtr->rectClient.left - wndPtr->rectWindow.left);
    rect.top  = event->y - (wndPtr->rectClient.top - wndPtr->rectWindow.top);
    rect.right  = rect.left + event->width;
    rect.bottom = rect.top + event->height;

    RedrawWindow( hwnd, &rect, 0,
                  RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN | RDW_ERASE |
                  (event->count ? 0 : RDW_ERASENOW) );
}


/***********************************************************************
 *           EVENT_key
 *
 * Handle a X key event
 */
static void EVENT_key( XKeyEvent *event )
{
    char Str[24]; 
    XComposeStatus cs; 
    KeySym keysym;
    WORD vkey = 0;
    WORD xkey, key_type, key;
    KEYLP keylp;
    BOOL extended = FALSE;

    int count = XLookupString(event, Str, 1, &keysym, &cs);
    Str[count] = '\0';
    dprintf_key(stddeb,"WM_KEY??? : keysym=%lX, count=%u / %X / '%s'\n", 
	   keysym, count, Str[0], Str);

    xkey = LOWORD(keysym);
    key_type = HIBYTE(xkey);
    key = LOBYTE(xkey);
    dprintf_key(stddeb,"            key_type=%X, key=%X\n", key_type, key);

    if (key_type == 0xFF)                          /* non-character key */
    {
	if (key >= 0x08 && key <= 0x1B)            /* special key */
	    vkey = special_key[key - 0x08];
	else if (key >= 0x50 && key <= 0x57)       /* cursor key */
	    vkey = cursor_key[key - 0x50];
	else if (key >= 0x60 && key <= 0x6B)       /* miscellaneous key */
	    vkey = misc_key[key - 0x60];
	else if (key >= 0x7E && key <= 0xB9)       /* keypad key */
	{
	    vkey = keypad_key[key - 0x7E];
	    extended = TRUE;
	}
	else if (key >= 0xBE && key <= 0xCD)       /* function key */
	{
	    vkey = function_key[key - 0xBE];
	    extended = TRUE;
	}
	else if (key >= 0xE1 && key <= 0xEA)       /* modifier key */
	    vkey = modifier_key[key - 0xE1];
	else if (key == 0xFF)                      /* DEL key */
	    vkey = VK_DELETE;
    }
    else if (key_type == 0)                        /* character key */
    {
	if (isalnum(key))
	    vkey = toupper(key);                 /* convert lower to uppercase */
	else
	    vkey = 0xbe;
    }

    if (event->type == KeyPress)
    {
	if (vkey == VK_MENU) ALTKeyState = TRUE;
	if (!(KeyStateTable[vkey] & 0x0f))
	    KeyStateTable[vkey] ^= 0x80;
	KeyStateTable[vkey] |= 0x01;
	keylp.lp1.count = 1;
	keylp.lp1.code = LOBYTE(event->keycode);
	keylp.lp1.extended = (extended ? 1 : 0);
	keylp.lp1.context = (event->state & Mod1Mask ? 1 : 0);
	keylp.lp1.previous = (KeyDown ? 0 : 1);
	keylp.lp1.transition = 0;
	dprintf_key(stddeb,"            wParam=%X, lParam=%lX\n", 
		    vkey, keylp.lp2 );
	dprintf_key(stddeb,"            KeyState=%X\n", KeyStateTable[vkey]);
	hardware_event( ALTKeyState ? WM_SYSKEYDOWN : WM_KEYDOWN, 
		        vkey, keylp.lp2,
		        event->x_root - desktopX, event->y_root - desktopY,
		        event->time, 0 );
	KeyDown = TRUE;

	/* The key translation ought to take place in TranslateMessage().
	 * However, there is no way of passing the required information 
	 * in a Windows message, so TranslateMessage does not currently
	 * do anything and the translation is done here.
	 */
	if (count == 1)                /* key has an ASCII representation */
	{
	    dprintf_key(stddeb,"WM_CHAR :   wParam=%X\n", (WORD)Str[0] );
	    PostMessage( GetFocus(), WM_CHAR, (WORD)(unsigned char)(Str[0]), 
			keylp.lp2 );
	}
    }
    else
    {
	if (vkey == VK_MENU) ALTKeyState = FALSE;
	KeyStateTable[vkey] &= 0xf0;
	keylp.lp1.count = 1;
	keylp.lp1.code = LOBYTE(event->keycode);
	keylp.lp1.extended = (extended ? 1 : 0);
	keylp.lp1.context = (event->state & Mod1Mask ? 1 : 0);
	keylp.lp1.previous = 1;
	keylp.lp1.transition = 1;
	dprintf_key(stddeb,"            wParam=%X, lParam=%lX\n", 
		    vkey, keylp.lp2 );
	dprintf_key(stddeb,"            KeyState=%X\n", KeyStateTable[vkey]);
	hardware_event( ((ALTKeyState || vkey == VK_MENU) ? 
			 WM_SYSKEYUP : WM_KEYUP), 
		        vkey, keylp.lp2,
		        event->x_root - desktopX, event->y_root - desktopY,
		        event->time, 0 );
	KeyDown = FALSE;
    }
}


/***********************************************************************
 *           EVENT_MotionNotify
 */
static void EVENT_MotionNotify( XMotionEvent *event )
{
    hardware_event( WM_MOUSEMOVE, EVENT_XStateToKeyState( event->state ), 0L,
		    event->x_root - desktopX, event->y_root - desktopY,
		    event->time, 0 );
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
    MouseButtonsStates[buttonNum] = TRUE;
    AsyncMouseButtonsStates[buttonNum] = TRUE;
    hardware_event( messages[buttonNum],
		    EVENT_XStateToKeyState( event->state ), 0L,
		    event->x_root - desktopX, event->y_root - desktopY,
		    event->time, 0 );
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
		    event->time, 0 );
}


/**********************************************************************
 *              EVENT_FocusIn
 */
static void EVENT_FocusIn (HWND hwnd, XFocusChangeEvent *event )
{
    if (event->detail == NotifyPointer) return;
    if (hwnd != GetActiveWindow()) WINPOS_ChangeActiveWindow( hwnd, FALSE );
    if ((hwnd != GetFocus()) && ! IsChild( hwnd, GetFocus())) SetFocus( hwnd );
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
    if ((hwnd == GetFocus()) || IsChild( hwnd, GetFocus())) SetFocus( 0 );
}


/**********************************************************************
 *              EVENT_ConfigureNotify
 *
 * The ConfigureNotify event is only selected on the desktop window
 * and on top-level windows when the -managed flag is used.
 */
static void EVENT_ConfigureNotify( HWND hwnd, XConfigureEvent *event )
{
    if (hwnd == GetDesktopWindow())
    {
        desktopX = event->x;
	desktopY = event->y;
    }
    else
    {
      /* A managed window; most of this code is shamelessly
       * stolen from SetWindowPos
       */
      
        WND *wndPtr;
	WINDOWPOS winpos;
	RECT newWindowRect, newClientRect;

	if (!(wndPtr = WIN_FindWndPtr( hwnd )))
	{
	    dprintf_event(stddeb, "ConfigureNotify: invalid HWND "NPFMT"\n", hwnd);
	    return;
	}
	
	/* Artificial messages */
	SendMessage(hwnd, WM_ENTERSIZEMOVE, 0, 0);
	SendMessage(hwnd, WM_EXITSIZEMOVE, 0, 0);

	/* Fill WINDOWPOS struct */
	winpos.flags = SWP_NOACTIVATE | SWP_NOZORDER;
	winpos.hwnd = hwnd;
	winpos.x = event->x;
	winpos.y = event->y;
	winpos.cx = event->width;
	winpos.cy = event->height;

	/* Check for unchanged attributes */
	if(winpos.x == wndPtr->rectWindow.left &&
	   winpos.y == wndPtr->rectWindow.top)
	    winpos.flags |= SWP_NOMOVE;
	if(winpos.cx == wndPtr->rectWindow.right - wndPtr->rectWindow.left &&
	   winpos.cy == wndPtr->rectWindow.bottom - wndPtr->rectWindow.top)
	    winpos.flags |= SWP_NOSIZE;

	/* Send WM_WINDOWPOSCHANGING */
	SendMessage(hwnd, WM_WINDOWPOSCHANGING, 0, (LPARAM)MAKE_SEGPTR(&winpos));

	/* Calculate new position and size */
	newWindowRect.left = event->x;
	newWindowRect.right = event->x + event->width;
	newWindowRect.top = event->y;
	newWindowRect.bottom = event->y + event->height;

	WINPOS_SendNCCalcSize( winpos.hwnd, TRUE, &newWindowRect,
			       &wndPtr->rectWindow, &wndPtr->rectClient,
			       &winpos, &newClientRect );

	/* Set new size and position */
	wndPtr->rectWindow = newWindowRect;
	wndPtr->rectClient = newClientRect;
	SendMessage(hwnd, WM_WINDOWPOSCHANGED, 0, (LPARAM)MAKE_SEGPTR(&winpos));
    }
}


/***********************************************************************
 *           EVENT_SelectionRequest
 */
static void EVENT_SelectionRequest( HWND hwnd, XSelectionRequestEvent *event )
{
    XSelectionEvent result;
    Atom rprop;
    Window request=event->requestor;
    rprop=None;
    if(event->target == XA_STRING)
    {
	HANDLE hText;
	LPSTR text;
        rprop=event->property;
	if(rprop == None)rprop=event->target;
        if(event->selection!=XA_PRIMARY)rprop=None;
        else if(!IsClipboardFormatAvailable(CF_TEXT))rprop=None;
	else{
            /* Don't worry if we can't open */
	    BOOL couldOpen=OpenClipboard(hwnd);
	    hText=GetClipboardData(CF_TEXT);
	    text=GlobalLock(hText);
	    XChangeProperty(display,request,rprop,XA_STRING,
		8,PropModeReplace,text,strlen(text));
	    GlobalUnlock(hText);
	    /* close only if we opened before */
	    if(couldOpen)CloseClipboard();
	}
    }
    if(rprop==None) dprintf_event(stddeb,"Request for %s ignored\n",
	XGetAtomName(display,event->target));
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
static void EVENT_SelectionNotify(HWND hwnd, XSelectionEvent *event)
{
    if(event->selection!=XA_PRIMARY)return;
    if(event->target!=XA_STRING)CLIPBOARD_ReadSelection(0,None);
    CLIPBOARD_ReadSelection(event->requestor,event->property);
}


/***********************************************************************
 *           EVENT_SelectionClear
 */
static void EVENT_SelectionClear(HWND hwnd, XSelectionClearEvent *event)
{
    if(event->selection!=XA_PRIMARY)return;
    CLIPBOARD_ReleaseSelection(hwnd); 
}


/**********************************************************************
 *           EVENT_ClientMessage
 */
static void EVENT_ClientMessage (HWND hwnd, XClientMessageEvent *event )
{
    static Atom wmProtocols = None;
    static Atom wmDeleteWindow = None;

    if (wmProtocols == None)
        wmProtocols = XInternAtom( display, "WM_PROTOCOLS", True );
    if (wmDeleteWindow == None)
        wmDeleteWindow = XInternAtom( display, "WM_DELETE_WINDOW", True );

    if ((event->format != 32) || (event->message_type != wmProtocols) ||
        (((Atom) event->data.l[0]) != wmDeleteWindow))
    {
	dprintf_event( stddeb, "unrecognized ClientMessage\n" );
	return;
    }
    SendMessage( hwnd, WM_SYSCOMMAND, SC_CLOSE, 0 );
}


/**********************************************************************
 *		SetCapture 	(USER.18)
 */
HWND SetCapture( HWND hwnd )
{
    Window win;
    HWND old_capture_wnd = captureWnd;

    if (!hwnd)
    {
        ReleaseCapture();
        return old_capture_wnd;
    }
    if (!(win = WIN_GetXWindow( hwnd ))) return 0;
    if (XGrabPointer(display, win, False, 
                     ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                     GrabModeAsync, GrabModeAsync,
                     None, None, CurrentTime ) == GrabSuccess)
    {
	dprintf_win(stddeb, "SetCapture: "NPFMT"\n", hwnd);
	captureWnd   = hwnd;
	return old_capture_wnd;
    }
    else return 0;
}


/**********************************************************************
 *		ReleaseCapture	(USER.19)
 */
void ReleaseCapture()
{
    if (captureWnd == 0) return;
    XUngrabPointer( display, CurrentTime );
    captureWnd = 0;
    dprintf_win(stddeb, "ReleaseCapture\n");
}

/**********************************************************************
 *		GetCapture 	(USER.236)
 */
HWND GetCapture()
{
    return captureWnd;
}


/***********************************************************************
 *           GetMouseEventProc   (USER.337)
 */
FARPROC GetMouseEventProc(void)
{
    char name[] = "Mouse_Event";
    return GetProcAddress( GetModuleHandle("USER"), MAKE_SEGPTR(name) );
}


/***********************************************************************
 *           Mouse_Event   (USER.299)
 */
#ifndef WINELIB
void Mouse_Event( struct sigcontext_struct context )
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

    if (AX_reg(&context) & ME_MOVE)
    {
        /* We have to actually move the cursor */
        XWarpPointer( display, rootWindow, None, 0, 0, 0, 0,
                      (short)BX_reg(&context), (short)CX_reg(&context) );
        return;
    }
    if (!XQueryPointer( display, rootWindow, &root, &child,
                        &rootX, &rootY, &childX, &childY, &state )) return;
    if (AX_reg(&context) & ME_LDOWN)
        hardware_event( WM_LBUTTONDOWN, EVENT_XStateToKeyState( state ), 0L,
                        rootX - desktopX, rootY - desktopY, GetTickCount(), 0);
    if (AX_reg(&context) & ME_LUP)
        hardware_event( WM_LBUTTONUP, EVENT_XStateToKeyState( state ), 0L,
                        rootX - desktopX, rootY - desktopY, GetTickCount(), 0);
    if (AX_reg(&context) & ME_RDOWN)
        hardware_event( WM_RBUTTONDOWN, EVENT_XStateToKeyState( state ), 0L,
                        rootX - desktopX, rootY - desktopY, GetTickCount(), 0);
    if (AX_reg(&context) & ME_RUP)
        hardware_event( WM_RBUTTONUP, EVENT_XStateToKeyState( state ), 0L,
                        rootX - desktopX, rootY - desktopY, GetTickCount(), 0);
}
#endif


/**********************************************************************
 *			EnableHardwareInput 		[USER.331]
 */
BOOL EnableHardwareInput(BOOL bEnable)
{
  BOOL bOldState = InputEnabled;
  dprintf_event(stdnimp,"EMPTY STUB !!! EnableHardwareInput(%d);\n", bEnable);
  InputEnabled = bEnable;
  return (bOldState && !bEnable);
}

