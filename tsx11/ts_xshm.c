/*
 * Thread safe wrappers around XShm calls.
 * This file was generated automatically by tools/make_X11wrappers
 *
 * Copyright 1998 Kristian Nielsen
 */

#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#include "tsx11defs.h"
#include "stddebug.h"
#include "debug.h"

Bool TSXShmQueryExtension(Display *a0)
{
  Bool r;
  dprintf_x11(stddeb, "Call XShmQueryExtension\n");
  X11_LOCK();
  r = XShmQueryExtension(a0);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XShmQueryExtension\n");
  return r;
}

int TSXShmPixmapFormat(Display *a0)
{
  int r;
  dprintf_x11(stddeb, "Call XShmPixmapFormat\n");
  X11_LOCK();
  r = XShmPixmapFormat(a0);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XShmPixmapFormat\n");
  return r;
}

Status TSXShmDetach(Display *a0, XShmSegmentInfo *a1)
{
  Status r;
  dprintf_x11(stddeb, "Call XShmDetach\n");
  X11_LOCK();
  r = XShmDetach(a0, a1);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XShmDetach\n");
  return r;
}

Status TSXShmAttach(Display *a0, XShmSegmentInfo *a1)
{
  Status r;
  dprintf_x11(stddeb, "Call XShmAttach\n");
  X11_LOCK();
  r = XShmAttach(a0, a1);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XShmAttach\n");
  return r;
}
