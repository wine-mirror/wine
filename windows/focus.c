/*
 * Focus functions
 *
 * Copyright 1993 David Metcalfe
 * Copyright 1994 Alexandre Julliard
 *
static char Copyright[] = "Copyright  David Metcalfe, 1993";
static char Copyright2[] = "Copyright  Alexandre Julliard, 1994";
*/

#include "win.h"
#include "color.h"

static HWND hWndFocus = 0;

/*****************************************************************
 *               FOCUS_SetXFocus
 *
 * Set the X focus.
 */
static void FOCUS_SetXFocus( HWND hwnd )
{
    XWindowAttributes win_attr;
    Window win;

      /* Only mess with the X focus if there's no desktop window */
    if (rootWindow != DefaultRootWindow(display)) return;

    if (!hwnd)	/* If setting the focus to 0, uninstall the colormap */
    {
	if (COLOR_WinColormap != DefaultColormapOfScreen(screen))
	    XUninstallColormap( display, COLOR_WinColormap );
	return;
    }

      /* Set X focus and install colormap */

    if (!(win = WIN_GetXWindow( hwnd ))) return;
    if (!XGetWindowAttributes( display, win, &win_attr ) ||
        (win_attr.map_state != IsViewable))
        return;  /* If window is not viewable, don't change anything */
    XSetInputFocus( display, win, RevertToParent, CurrentTime );
    if (COLOR_WinColormap != DefaultColormapOfScreen(screen))
	XInstallColormap( display, COLOR_WinColormap );
}


/*****************************************************************
 *               SetFocus            (USER.22)
 */

HWND SetFocus(HWND hwnd)
{
    HWND hWndPrevFocus, hwndParent;
    WND *wndPtr;

    if (hwnd == hWndFocus) return hWndFocus;  /* Nothing to do! */    

    if (hwnd)
    {
	  /* Check if we can set the focus to this window */

	hwndParent = hwnd;
	while ((wndPtr = WIN_FindWndPtr( hwndParent )) != NULL)
	{
	    if ((wndPtr->dwStyle & WS_MINIMIZE) ||
		(wndPtr->dwStyle & WS_DISABLED)) return 0;
	    if (!(wndPtr->dwStyle & WS_CHILD)) break;
	    hwndParent = wndPtr->hwndParent;
	}

	  /* Now hwndParent is the top-level ancestor. Activate it. */

	if (hwndParent != GetActiveWindow())
	{
	    SetWindowPos( hwndParent, HWND_TOP, 0, 0, 0, 0,
			  SWP_NOSIZE | SWP_NOMOVE );
	    if (!IsWindow( hwnd )) return 0;  /* Abort if window destroyed */
	}
    }

      /* Change focus and send messages */

    hWndPrevFocus = hWndFocus;
    hWndFocus = hwnd;    
    if (hWndPrevFocus) SendMessage( hWndPrevFocus, WM_KILLFOCUS, 
				    (WPARAM)hwnd, 0 );
    if (hwnd == hWndFocus)  /* Maybe already changed during WM_KILLFOCUS */
    {
	if (hwnd) SendMessage( hWndFocus, WM_SETFOCUS, 
			       (WPARAM)hWndPrevFocus, 0 );
	FOCUS_SetXFocus( hwnd );
    }
    return hWndPrevFocus;
}


/*****************************************************************
 *               GetFocus            (USER.23)
 */

HWND GetFocus(void)
{
    return hWndFocus;
}


