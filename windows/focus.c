/*
 * Focus functions
 *
 * Copyright 1993 David Metcalfe
 */

static char Copyright[] = "Copyright  David Metcalfe, 1993";

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include "win.h"
#include "gdi.h"

HWND hWndFocus = 0;


/*****************************************************************
 *               SetFocus            (USER.22)
 */

HWND SetFocus(HWND hwnd)
{
    HWND hWndPrevFocus;
    WND *wndPtr;

    hWndPrevFocus = hWndFocus;

    if (hwnd == 0)
    {
	XSetInputFocus(display, None, RevertToPointerRoot, CurrentTime);
    }
    else
    {
	XWindowAttributes win_attr;
	wndPtr = WIN_FindWndPtr(hwnd);
	
	if (XGetWindowAttributes( display, wndPtr->window, &win_attr ))
	{
	    if (win_attr.map_state == IsViewable)
		XSetInputFocus(display, wndPtr->window,
			       RevertToParent, CurrentTime);
	}
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


