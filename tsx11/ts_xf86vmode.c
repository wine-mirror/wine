/*
 * Thread safe wrappers around xf86vmode calls.
 * This file was generated automatically by tools/make_X11wrappers
 * DO NOT EDIT!
 */

#include "config.h"

#include "windef.h"
#ifdef HAVE_LIBXXF86VM
#define XMD_H
#include "basetsd.h"

#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>

#include "ts_xf86vmode.h"


Bool TSXF86VidModeQueryVersion(Display*a0,int*a1,int*a2)
{
  Bool r;
  wine_tsx11_lock();
  r = XF86VidModeQueryVersion(a0,a1,a2);
  wine_tsx11_unlock();
  return r;
}

Bool TSXF86VidModeQueryExtension(Display*a0,int*a1,int*a2)
{
  Bool r;
  wine_tsx11_lock();
  r = XF86VidModeQueryExtension(a0,a2,a2);
  wine_tsx11_unlock();
  return r;
}

Bool TSXF86VidModeGetModeLine(Display*a0,int a1,int*a2,XF86VidModeModeLine*a3)
{
  Bool r;
  wine_tsx11_lock();
  r = XF86VidModeGetModeLine(a0,a1,a2,a3);
  wine_tsx11_unlock();
  return r;
}

Bool TSXF86VidModeGetAllModeLines(Display*a0,int a1,int*a2,XF86VidModeModeInfo***a3)
{
  Bool r;
  wine_tsx11_lock();
  r = XF86VidModeGetAllModeLines(a0,a1,a2,a3);
  wine_tsx11_unlock();
  return r;
}

Bool TSXF86VidModeAddModeLine(Display*a0,int a1,XF86VidModeModeInfo*a2,XF86VidModeModeInfo*a3)
{
  Bool r;
  wine_tsx11_lock();
  r = XF86VidModeAddModeLine(a0,a1,a2,a3);
  wine_tsx11_unlock();
  return r;
}

Bool TSXF86VidModeDeleteModeLine(Display*a0,int a1,XF86VidModeModeInfo*a2)
{
  Bool r;
  wine_tsx11_lock();
  r = XF86VidModeDeleteModeLine(a0,a1,a2);
  wine_tsx11_unlock();
  return r;
}

Bool TSXF86VidModeModModeLine(Display*a0,int a1,XF86VidModeModeLine*a2)
{
  Bool r;
  wine_tsx11_lock();
  r = XF86VidModeModModeLine(a0,a1,a2);
  wine_tsx11_unlock();
  return r;
}

Status TSXF86VidModeValidateModeLine(Display*a0,int a1,XF86VidModeModeInfo*a2)
{
  Status r;
  wine_tsx11_lock();
  r = XF86VidModeValidateModeLine(a0,a1,a2);
  wine_tsx11_unlock();
  return r;
}

Bool TSXF86VidModeSwitchMode(Display*a0,int a1,int a2)
{
  Bool r;
  wine_tsx11_lock();
  r = XF86VidModeSwitchMode(a0,a1,a2);
  wine_tsx11_unlock();
  return r;
}

Bool TSXF86VidModeSwitchToMode(Display*a0,int a1,XF86VidModeModeInfo*a2)
{
  Bool r;
  wine_tsx11_lock();
  r = XF86VidModeSwitchToMode(a0,a1,a2);
  wine_tsx11_unlock();
  return r;
}

Bool TSXF86VidModeLockModeSwitch(Display*a0,int a1,int a2)
{
  Bool r;
  wine_tsx11_lock();
  r = XF86VidModeLockModeSwitch(a0,a1,a2);
  wine_tsx11_unlock();
  return r;
}

Bool TSXF86VidModeGetMonitor(Display*a0,int a1,XF86VidModeMonitor*a2)
{
  Bool r;
  wine_tsx11_lock();
  r = XF86VidModeGetMonitor(a0,a1,a2);
  wine_tsx11_unlock();
  return r;
}

Bool TSXF86VidModeGetViewPort(Display*a0,int a1,int*a2,int*a3)
{
  Bool r;
  wine_tsx11_lock();
  r = XF86VidModeGetViewPort(a0,a1,a2,a3);
  wine_tsx11_unlock();
  return r;
}

Bool TSXF86VidModeSetViewPort(Display*a0,int a1,int a2,int a3)
{
  Bool r;
  wine_tsx11_lock();
  r = XF86VidModeSetViewPort(a0,a1,a2,a3);
  wine_tsx11_unlock();
  return r;
}

#endif /* defined(HAVE_LIBXXF86VM) */

