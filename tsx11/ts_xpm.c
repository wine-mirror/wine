/*
 * Thread safe wrappers around xpm calls.
 * This file was generated automatically by tools/make_X11wrappers
 * DO NOT EDIT!
 */

#include <X11/xpm.h>
#include "x11drv.h"
#include "stddebug.h"
#include "debug.h"

int TSXpmCreatePixmapFromData(Display *a0, Drawable a1, char **a2, Pixmap *a3, Pixmap *a4, XpmAttributes *a5)
{
  int r;
  dprintf_x11(stddeb, "Call XpmCreatePixmapFromData\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XpmCreatePixmapFromData(a0, a1, a2, a3, a4, a5);
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XpmCreatePixmapFromData\n");
  return r;
}

int TSXpmAttributesSize(void)
{
  int r;
  dprintf_x11(stddeb, "Call XpmAttributesSize\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XpmAttributesSize();
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XpmAttributesSize\n");
  return r;
}
