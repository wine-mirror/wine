/*
 * Thread safe wrappers around XShm calls.
 * This file was generated automatically by tools/make_X11wrappers
 * DO NOT EDIT!
 */

#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#include "x11drv.h"
#include "debug.h"

Bool TSXShmQueryExtension(Display *a0)
{
  Bool r;
  TRACE(x11, "Call XShmQueryExtension\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XShmQueryExtension(a0);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XShmQueryExtension\n");
  return r;
}

int TSXShmPixmapFormat(Display *a0)
{
  int r;
  TRACE(x11, "Call XShmPixmapFormat\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XShmPixmapFormat(a0);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XShmPixmapFormat\n");
  return r;
}

Status TSXShmDetach(Display *a0, XShmSegmentInfo *a1)
{
  Status r;
  TRACE(x11, "Call XShmDetach\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XShmDetach(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XShmDetach\n");
  return r;
}

Status TSXShmAttach(Display *a0, XShmSegmentInfo *a1)
{
  Status r;
  TRACE(x11, "Call XShmAttach\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XShmAttach(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XShmAttach\n");
  return r;
}

Status TSXShmPutImage(Display *a0, Drawable a1, GC a2, XImage *a3, int a4, int a5, int a6, int a7, unsigned int a8, unsigned int a9, Bool a10)
{
  Status r;
  TRACE(x11, "Call XShmPutImage\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XShmPutImage(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XShmPutImage\n");
  return r;
}

