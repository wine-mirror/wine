/*
 * Thread safe wrappers around XShm calls.
 * This file was generated automatically by tools/make_X11wrappers
 * DO NOT EDIT!
 */

#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#include "x11drv.h"
#include "stddebug.h"
#include "debug.h"

Bool TSXShmQueryExtension(Display *a0)
{
  Bool r;
  dprintf_x11(stddeb, "Call XShmQueryExtension\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XShmQueryExtension(a0);
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XShmQueryExtension\n");
  return r;
}

int TSXShmPixmapFormat(Display *a0)
{
  int r;
  dprintf_x11(stddeb, "Call XShmPixmapFormat\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XShmPixmapFormat(a0);
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XShmPixmapFormat\n");
  return r;
}

Status TSXShmDetach(Display *a0, XShmSegmentInfo *a1)
{
  Status r;
  dprintf_x11(stddeb, "Call XShmDetach\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XShmDetach(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XShmDetach\n");
  return r;
}

Status TSXShmAttach(Display *a0, XShmSegmentInfo *a1)
{
  Status r;
  dprintf_x11(stddeb, "Call XShmAttach\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XShmAttach(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XShmAttach\n");
  return r;
}
