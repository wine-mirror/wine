/*
 * X events handling functions
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include <X11/Xlib.h>

#include "windows.h"
#include "win.h"
#include "class.h"
#include "message.h"


#define NB_BUTTONS      3     /* Windows can handle 3 buttons */

extern Display * display;

  /* X context to associate a hwnd to an X window */
static XContext winContext = 0;

  /* State variables */
static WORD ALTKeyState;
static HWND captureWnd = 0;
Window winHasCursor = 0;
extern HWND hWndFocus;

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


#ifdef DEBUG_EVENT
static char *event_names[] =
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
#endif

  /* Event handlers */
static void EVENT_key( HWND hwnd, XKeyEvent *event );
static void EVENT_ButtonPress( HWND hwnd, XButtonEvent *event );
static void EVENT_ButtonRelease( HWND hwnd, XButtonEvent *event );
static void EVENT_MotionNotify( HWND hwnd, XMotionEvent *event );
static void EVENT_EnterNotify( HWND hwnd, XCrossingEvent *event );
static void EVENT_FocusIn( HWND hwnd, XFocusChangeEvent *event );
static void EVENT_FocusOut( HWND hwnd, XFocusChangeEvent *event );
static void EVENT_Expose( HWND hwnd, XExposeEvent *event );
static void EVENT_ConfigureNotify( HWND hwnd, XConfigureEvent *event );


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

#ifdef DEBUG_EVENT
    printf( "Got event %s for hwnd %d\n", event_names[event->type], hwnd );
#endif

    switch(event->type)
    {
    case KeyPress:
    case KeyRelease:
	EVENT_key( hwnd, (XKeyEvent*)event );
	break;
	
    case ButtonPress:
	EVENT_ButtonPress( hwnd, (XButtonEvent*)event );
	break;

    case ButtonRelease:
	EVENT_ButtonRelease( hwnd, (XButtonEvent*)event );
	break;

    case MotionNotify:
	EVENT_MotionNotify( hwnd, (XMotionEvent*)event );
	break;

    case EnterNotify:
	EVENT_EnterNotify( hwnd, (XCrossingEvent*)event );
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

#ifdef DEBUG_EVENT
    default:    
	printf( "Unprocessed event %s for hwnd %d\n",
	        event_names[event->type], hwnd );
	break;
#endif	
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
    winHasCursor = event->window;
    
    InvalidateRect( hwnd, &rect, TRUE );
}


/***********************************************************************
 *           EVENT_key
 *
 * Handle a X key event
 */
static void EVENT_key( HWND hwnd, XKeyEvent *event )
{
    char Str[24]; 
    XComposeStatus cs; 
    KeySym keysym;
    WORD xkey, vkey, key_type, key;
    KEYLP keylp;
    BOOL extended = FALSE;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    int count = XLookupString(event, Str, 1, &keysym, &cs);
    Str[count] = '\0';
#ifdef DEBUG_KEY
    printf("WM_KEY??? : keysym=%lX, count=%u / %X / '%s'\n", 
	   keysym, count, Str[0], Str);
#endif

    xkey = LOWORD(keysym);
    key_type = HIBYTE(xkey);
    key = LOBYTE(xkey);
#ifdef DEBUG_KEY
    printf("            key_type=%X, key=%X\n", key_type, key);
#endif

      /* Position must be relative to client area */
    event->x -= wndPtr->rectClient.left - wndPtr->rectWindow.left;
    event->y -= wndPtr->rectClient.top - wndPtr->rectWindow.top;

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
	if (key >= 0x61 && key <= 0x7A)
	    vkey = key - 0x20;                 /* convert lower to uppercase */
	else
	    vkey = key;
    }

    if (event->type == KeyPress)
    {
	if (vkey == VK_MENU) ALTKeyState = TRUE;
	keylp.lp1.count = 1;
	keylp.lp1.code = LOBYTE(event->keycode);
	keylp.lp1.extended = (extended ? 1 : 0);
	keylp.lp1.context = (event->state & Mod1Mask ? 1 : 0);
	keylp.lp1.previous = (KeyDown ? 0 : 1);
	keylp.lp1.transition = 0;
#ifdef DEBUG_KEY
	printf("            wParam=%X, lParam=%lX\n", vkey, keylp.lp2 );
#endif
	hardware_event( hwnd, ALTKeyState ? WM_SYSKEYDOWN : WM_KEYDOWN, 
		        vkey, keylp.lp2,
		        event->x_root & 0xffff, event->y_root & 0xffff,
		        event->time, 0 );
	KeyDown = TRUE;

	/* The key translation ought to take place in TranslateMessage().
	 * However, there is no way of passing the required information 
	 * in a Windows message, so TranslateMessage does not currently
	 * do anything and the translation is done here.
	 */
	if (count == 1)                /* key has an ASCII representation */
	{
#ifdef DEBUG_KEY
	    printf("WM_CHAR :   wParam=%X\n", (WORD)Str[0] );
#endif
	    PostMessage( hwnd, WM_CHAR, (WORD)Str[0], keylp.lp2 );
	}
    }
    else
    {
	if (vkey == VK_MENU) ALTKeyState = FALSE;
	keylp.lp1.count = 1;
	keylp.lp1.code = LOBYTE(event->keycode);
	keylp.lp1.extended = (extended ? 1 : 0);
	keylp.lp1.context = (event->state & Mod1Mask ? 1 : 0);
	keylp.lp1.previous = 1;
	keylp.lp1.transition = 1;
#ifdef DEBUG_KEY
	printf("            wParam=%X, lParam=%lX\n", vkey, keylp.lp2 );
#endif
	hardware_event( hwnd, 
		        ((ALTKeyState || vkey == VK_MENU) ? 
			 WM_SYSKEYUP : WM_KEYUP), 
		        vkey, keylp.lp2,
		        event->x_root & 0xffff, event->y_root & 0xffff,
		        event->time, 0 );
	KeyDown = FALSE;
    }
}


/***********************************************************************
 *           EVENT_MotionNotify
 */
static void EVENT_MotionNotify( HWND hwnd, XMotionEvent *event )
{
    hardware_event( hwnd, WM_MOUSEMOVE,
		    EVENT_XStateToKeyState( event->state ), 0L,
		    event->x_root & 0xffff, event->y_root & 0xffff,
		    event->time, 0 );		    
}


/***********************************************************************
 *           EVENT_ButtonPress
 */
static void EVENT_ButtonPress( HWND hwnd, XButtonEvent *event )
{
    static WORD messages[NB_BUTTONS] = 
        { WM_LBUTTONDOWN, WM_MBUTTONDOWN, WM_RBUTTONDOWN };
    int buttonNum = event->button - 1;

    if (buttonNum >= NB_BUTTONS) return;    
    winHasCursor = event->window;
    hardware_event( hwnd, messages[buttonNum],
		    EVENT_XStateToKeyState( event->state ), 0L,
		    event->x_root & 0xffff, event->y_root & 0xffff,
		    event->time, 0 );		    
}


/***********************************************************************
 *           EVENT_ButtonRelease
 */
static void EVENT_ButtonRelease( HWND hwnd, XButtonEvent *event )
{
    static const WORD messages[NB_BUTTONS] = 
        { WM_LBUTTONUP, WM_MBUTTONUP, WM_RBUTTONUP };
    int buttonNum = event->button - 1;

    if (buttonNum >= NB_BUTTONS) return;    
    winHasCursor = event->window;
    hardware_event( hwnd, messages[buttonNum],
		    EVENT_XStateToKeyState( event->state ), 0L,
		    event->x_root & 0xffff, event->y_root & 0xffff,
		    event->time, 0 );		    
}


/***********************************************************************
 *           EVENT_ConfigureNotify
 */
static void EVENT_ConfigureNotify( HWND hwnd, XConfigureEvent *event )
{
    HANDLE handle;
    NCCALCSIZE_PARAMS *params;	    
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return;
    wndPtr->rectWindow.left   = event->x;
    wndPtr->rectWindow.top    = event->y;
    wndPtr->rectWindow.right  = event->x + event->width;
    wndPtr->rectWindow.bottom = event->y + event->height;

      /* Send WM_NCCALCSIZE message */
    handle = GlobalAlloc( GMEM_MOVEABLE, sizeof(*params) );
    params = (NCCALCSIZE_PARAMS *)GlobalLock( handle );
    params->rgrc[0] = wndPtr->rectWindow;
    params->lppos   = NULL;  /* Should be WINDOWPOS struct */
    SendMessage( hwnd, WM_NCCALCSIZE, FALSE, (LPARAM)params );
    wndPtr->rectClient = params->rgrc[0];
    PostMessage( hwnd, WM_MOVE, 0,
		 MAKELONG( wndPtr->rectClient.left, wndPtr->rectClient.top ));
    PostMessage( hwnd, WM_SIZE, SIZE_RESTORED,
		 MAKELONG( wndPtr->rectClient.right-wndPtr->rectClient.left,
			   wndPtr->rectClient.bottom-wndPtr->rectClient.top) );
    GlobalUnlock( handle );
    GlobalFree( handle );
}


/**********************************************************************
 *              EVENT_FocusIn
 */
static void EVENT_FocusIn( HWND hwnd, XFocusChangeEvent *event )
{
    PostMessage( hwnd, WM_SETFOCUS, hwnd, 0 );
    hWndFocus = hwnd;
}


/**********************************************************************
 *              EVENT_FocusOut
 */
static void EVENT_FocusOut( HWND hwnd, XFocusChangeEvent *event )
{
    if (hWndFocus)
    {
	PostMessage( hwnd, WM_KILLFOCUS, hwnd, 0 );
	hWndFocus = 0;
    }
}


/**********************************************************************
 *              EVENT_EnterNotify
 */
static void EVENT_EnterNotify( HWND hwnd, XCrossingEvent *event )
{
    if (captureWnd != 0) return;
    winHasCursor = event->window;
    PostMessage( hwnd, WM_SETCURSOR, hwnd, 0 );
}


/**********************************************************************
 *		SetCapture 	(USER.18)
 */
HWND SetCapture(HWND wnd)
{
    int rv;
    HWND old_capture_wnd = captureWnd;
    WND *wnd_p = WIN_FindWndPtr(wnd);
    if (wnd_p == NULL)
	return 0;
    
    rv = XGrabPointer(display, wnd_p->window, False, 
		      ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
		      GrabModeAsync, GrabModeSync, None, None, CurrentTime);

    if (rv == GrabSuccess)
    {
	captureWnd = wnd;
	return old_capture_wnd;
    }
    else
	return 0;
}

/**********************************************************************
 *		ReleaseCapture	(USER.19)
 */
void ReleaseCapture()
{
    WND *wnd_p;
    
    if (captureWnd == 0)
	return;
    
    wnd_p = WIN_FindWndPtr(captureWnd);
    if (wnd_p == NULL)
	return;
    
    XUngrabPointer( display, CurrentTime );
    captureWnd = 0;
}

/**********************************************************************
 *		GetCapture 	(USER.236)
 */
HWND GetCapture()
{
    return captureWnd;
}
