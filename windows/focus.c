/*
 * Focus functions
 *
 * Copyright 1993 David Metcalfe
 *           1994 Alexandre Julliard
 * 	     1995 Alex Korobka
 *
 */

#include "win.h"
#include "winpos.h"
#include "hook.h"
#include "color.h"
#include "message.h"
#include "options.h"

static HWND hwndFocus = 0;

/*****************************************************************
 *               FOCUS_SetXFocus
 *
 * Set the X focus.
 * Explicit colormap management seems to work only with OLVWM.
 */
void FOCUS_SetXFocus( HWND hwnd )
{
    XWindowAttributes win_attr;
    Window win;

    /* Only mess with the X focus if there's */
    /* no desktop window and no window manager. */
    if ((rootWindow != DefaultRootWindow(display)) || Options.managed) return;

    if (!hwnd)	/* If setting the focus to 0, uninstall the colormap */
    {
	if (COLOR_GetSystemPaletteFlags() & COLOR_PRIVATE)
	    XUninstallColormap( display, COLOR_GetColormap() );
	return;
    }

      /* Set X focus and install colormap */

    if (!(win = WIN_GetXWindow( hwnd ))) return;
    if (!XGetWindowAttributes( display, win, &win_attr ) ||
        (win_attr.map_state != IsViewable))
        return;  /* If window is not viewable, don't change anything */

    XSetInputFocus( display, win, RevertToParent, CurrentTime );
    if (COLOR_GetSystemPaletteFlags() & COLOR_PRIVATE)
        XInstallColormap( display, COLOR_GetColormap() );

    EVENT_Synchronize();
}

/*****************************************************************
 *	         FOCUS_SwitchFocus 
 */
void FOCUS_SwitchFocus(HWND hFocusFrom, HWND hFocusTo)
{
    hwndFocus = hFocusTo;

    if (hFocusFrom) SendMessage16( hFocusFrom, WM_KILLFOCUS, (WPARAM)hFocusTo, 0L);
    if( !hFocusTo || hFocusTo != hwndFocus )
	return;

    SendMessage16( hFocusTo, WM_SETFOCUS, (WPARAM)hFocusFrom, 0L);
    FOCUS_SetXFocus( hFocusTo );
}


/*****************************************************************
 *               SetFocus            (USER.22)
 */
HWND SetFocus(HWND hwnd)
{
    HWND hWndPrevFocus, hwndTop = hwnd;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    if (wndPtr)
    {
	  /* Check if we can set the focus to this window */

	while ( (wndPtr->dwStyle & (WS_CHILD | WS_POPUP)) == WS_CHILD  )
	{
	    if ( wndPtr->dwStyle & ( WS_MINIMIZE | WS_DISABLED) )
		 return 0;
            if (!(wndPtr = wndPtr->parent)) return 0;
	    hwndTop = wndPtr->hwndSelf;
	}

	if( hwnd == hwndFocus ) return hwnd;

	/* call hooks */
	if( HOOK_CallHooks( WH_CBT, HCBT_SETFOCUS, (WPARAM)hwnd, (LPARAM)hwndFocus) )
	    return 0;

        /* activate hwndTop if needed. */
	if (hwndTop != GetActiveWindow())
	{
	    if (!WINPOS_SetActiveWindow(hwndTop, 0, 0)) return 0;

	    if (!IsWindow( hwnd )) return 0;  /* Abort if window destroyed */
	}
    }
    else if( HOOK_CallHooks( WH_CBT, HCBT_SETFOCUS, 0, (LPARAM)hwndFocus ) )
             return 0;

      /* Change focus and send messages */
    hWndPrevFocus = hwndFocus;

    FOCUS_SwitchFocus( hwndFocus , hwnd );

    return hWndPrevFocus;
}


/*****************************************************************
 *               GetFocus            (USER.23)
 */
HWND GetFocus(void)
{
    return hwndFocus;
}


