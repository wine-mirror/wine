/*
 * X11 windows driver
 *
 * Copyright 1993, 1994, 1995, 1996 Alexandre Julliard
 *                             1993 David Metcalfe
 *                       1995, 1996 Alex Korobka
 */

#include "config.h"

#include <X11/Xatom.h>

#include "ts_xlib.h"
#include "ts_xutil.h"
#include "ts_shape.h"

#include <stdlib.h>
#include <string.h>

#include "bitmap.h"
#include "debugtools.h"
#include "gdi.h"
#include "options.h"
#include "message.h"
#include "win.h"
#include "windef.h"
#include "x11drv.h"
#include "wingdi.h"
#include "winnls.h"
#include "wine/winuser16.h"

DEFAULT_DEBUG_CHANNEL(win);

extern Atom wmChangeState;

#define HAS_DLGFRAME(style,exStyle) \
((!((style) & WS_THICKFRAME)) && (((style) & WS_DLGFRAME) || ((exStyle) & WS_EX_DLGMODALFRAME)))

/**********************************************************************/

WND_DRIVER X11DRV_WND_Driver =
{
  X11DRV_WND_ForceWindowRaise
};


/***********************************************************************
 *		X11DRV_WND_IsZeroSizeWnd
 *
 * Return TRUE if the window has a height or widht less or equal to 0
 */
static BOOL X11DRV_WND_IsZeroSizeWnd(WND *wndPtr)
{
    if ( (wndPtr->rectWindow.left >= wndPtr->rectWindow.right) ||
         (wndPtr->rectWindow.top >= wndPtr->rectWindow.bottom) )
        return TRUE;
    else
        return FALSE;
}

/***********************************************************************
 *		X11DRV_WND_ForceWindowRaise
 *
 * Raise a window on top of the X stacking order, while preserving 
 * the correct Windows Z order.
 */
void X11DRV_WND_ForceWindowRaise(WND *wndPtr)
{
  XWindowChanges winChanges;
  WND *wndPrev,*pDesktop = WIN_GetDesktop();

  if (X11DRV_WND_IsZeroSizeWnd(wndPtr))
  {
    WIN_ReleaseDesktop();
    return;
  }
  
  if( !wndPtr || !get_whole_window(wndPtr) || (wndPtr->dwExStyle & WS_EX_MANAGED) )
  {
      WIN_ReleaseDesktop();
    return;
  }
  
  /* Raise all windows up to wndPtr according to their Z order.
   * (it would be easier with sibling-related Below but it doesn't
   * work very well with SGI mwm for instance)
   */
  winChanges.stack_mode = Above;
  while (wndPtr)
    {
      if ( !X11DRV_WND_IsZeroSizeWnd(wndPtr) && get_whole_window(wndPtr) )
         TSXReconfigureWMWindow( thread_display(), get_whole_window(wndPtr), 0,
                                 CWStackMode, &winChanges );

      wndPrev = pDesktop->child;
      if (wndPrev == wndPtr) break;
      while (wndPrev && (wndPrev->next != wndPtr)) wndPrev = wndPrev->next;

      wndPtr = wndPrev;
    }
  WIN_ReleaseDesktop();
}
