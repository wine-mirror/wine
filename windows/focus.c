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

static HWND32 hwndFocus = 0;

/*****************************************************************
 *               FOCUS_SetXFocus
 *
 * Set the X focus.
 * Explicit colormap management seems to work only with OLVWM.
 */
void FOCUS_SetXFocus( HWND32 hwnd )
{
    XWindowAttributes win_attr;
    Window win;

    /* Only mess with the X focus if there's */
    /* no desktop window and no window manager. */
    if ((rootWindow != DefaultRootWindow(display)) || Options.managed) return;

    if (!hwnd)	/* If setting the focus to 0, uninstall the colormap */
    {
	if (COLOR_GetSystemPaletteFlags() & COLOR_PRIVATE)
	    TSXUninstallColormap( display, COLOR_GetColormap() );
	return;
    }

      /* Set X focus and install colormap */

    if (!(win = WIN_GetXWindow( hwnd ))) return;
    if (!TSXGetWindowAttributes( display, win, &win_attr ) ||
        (win_attr.map_state != IsViewable))
        return;  /* If window is not viewable, don't change anything */

    TSXSetInputFocus( display, win, RevertToParent, CurrentTime );
    if (COLOR_GetSystemPaletteFlags() & COLOR_PRIVATE)
        TSXInstallColormap( display, COLOR_GetColormap() );

    EVENT_Synchronize();
}

/*****************************************************************
 *	         FOCUS_SwitchFocus 
 */
void FOCUS_SwitchFocus( HWND32 hFocusFrom, HWND32 hFocusTo )
{
    hwndFocus = hFocusTo;

#if 0
    if (hFocusFrom) SendMessage32A( hFocusFrom, WM_KILLFOCUS, hFocusTo, 0 );
#else
    /* FIXME: must be SendMessage16() because 32A doesn't do
     * intertask at this time */
    if (hFocusFrom) SendMessage16( hFocusFrom, WM_KILLFOCUS, hFocusTo, 0 );
#endif
    if( !hFocusTo || hFocusTo != hwndFocus )
	return;

    /* According to API docs, the WM_SETFOCUS message is sent AFTER the window
       has received the keyboard focus. */
    FOCUS_SetXFocus( hFocusTo );

#if 0
    SendMessage32A( hFocusTo, WM_SETFOCUS, hFocusFrom, 0 );
#else
    SendMessage16( hFocusTo, WM_SETFOCUS, hFocusFrom, 0 );
#endif
}


/*****************************************************************
 *               SetFocus16   (USER.22)
 */
HWND16 WINAPI SetFocus16( HWND16 hwnd )
{
    return (HWND16)SetFocus32( hwnd );
}


/*****************************************************************
 *               SetFocus32   (USER32.481)
 */
HWND32 WINAPI SetFocus32( HWND32 hwnd )
{
    HWND32 hWndPrevFocus, hwndTop = hwnd;
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
	if( HOOK_CallHooks16( WH_CBT, HCBT_SETFOCUS, (WPARAM16)hwnd,
			      (LPARAM)hwndFocus) )
	    return 0;

        /* activate hwndTop if needed. */
	if (hwndTop != GetActiveWindow32())
	{
	    if (!WINPOS_SetActiveWindow(hwndTop, 0, 0)) return 0;

	    if (!IsWindow32( hwnd )) return 0;  /* Abort if window destroyed */
	}
    }
    else if( HOOK_CallHooks16( WH_CBT, HCBT_SETFOCUS, 0, (LPARAM)hwndFocus ) )
             return 0;

      /* Change focus and send messages */
    hWndPrevFocus = hwndFocus;

    FOCUS_SwitchFocus( hwndFocus , hwnd );

    return hWndPrevFocus;
}


/*****************************************************************
 *               GetFocus16   (USER.23)
 */
HWND16 WINAPI GetFocus16(void)
{
    return (HWND16)hwndFocus;
}


/*****************************************************************
 *               GetFocus32   (USER32.240)
 */
HWND32 WINAPI GetFocus32(void)
{
    return hwndFocus;
}
