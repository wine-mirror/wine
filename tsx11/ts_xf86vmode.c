/*
 * Thread safe wrappers around xf86vmode calls.
 * This file was generated automatically by tools/make_X11wrappers
 * DO NOT EDIT!
 */

#include "config.h"

#ifndef X_DISPLAY_MISSING

#include "windef.h"
#ifdef HAVE_LIBXXF86VM
#define XMD_H
#define INT32 INT

#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>
#include "debugtools.h"
#include "x11drv.h"

DEFAULT_DEBUG_CHANNEL(x11)

Bool TSXF86VidModeQueryVersion(Display*a0,int*a1,int*a2)
{
  Bool r;
  TRACE("Call XF86VidModeQueryVersion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XF86VidModeQueryVersion(a0,a1,a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XF86VidModeQueryVersion\n");
  return r;
}

Bool TSXF86VidModeQueryExtension(Display*a0,int*a1,int*a2)
{
  Bool r;
  TRACE("Call XF86VidModeQueryExtension\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XF86VidModeQueryExtension(a0,a2,a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XF86VidModeQueryExtension\n");
  return r;
}

Bool TSXF86VidModeGetModeLine(Display*a0,int a1,int*a2,XF86VidModeModeLine*a3)
{
  Bool r;
  TRACE("Call XF86VidModeGetModeLine\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XF86VidModeGetModeLine(a0,a1,a2,a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XF86VidModeGetModeLine\n");
  return r;
}

Bool TSXF86VidModeGetAllModeLines(Display*a0,int a1,int*a2,XF86VidModeModeInfo***a3)
{
  Bool r;
  TRACE("Call XF86VidModeGetAllModeLines\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XF86VidModeGetAllModeLines(a0,a1,a2,a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XF86VidModeGetAllModeLines\n");
  return r;
}

Bool TSXF86VidModeAddModeLine(Display*a0,int a1,XF86VidModeModeInfo*a2,XF86VidModeModeInfo*a3)
{
  Bool r;
  TRACE("Call XF86VidModeAddModeLine\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XF86VidModeAddModeLine(a0,a1,a2,a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XF86VidModeAddModeLine\n");
  return r;
}

Bool TSXF86VidModeDeleteModeLine(Display*a0,int a1,XF86VidModeModeInfo*a2)
{
  Bool r;
  TRACE("Call XF86VidModeDeleteModeLine\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XF86VidModeDeleteModeLine(a0,a1,a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XF86VidModeDeleteModeLine\n");
  return r;
}

Bool TSXF86VidModeModModeLine(Display*a0,int a1,XF86VidModeModeLine*a2)
{
  Bool r;
  TRACE("Call XF86VidModeModModeLine\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XF86VidModeModModeLine(a0,a1,a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XF86VidModeModModeLine\n");
  return r;
}

Status TSXF86VidModeValidateModeLine(Display*a0,int a1,XF86VidModeModeInfo*a2)
{
  Status r;
  TRACE("Call XF86VidModeValidateModeLine\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XF86VidModeValidateModeLine(a0,a1,a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XF86VidModeValidateModeLine\n");
  return r;
}

Bool TSXF86VidModeSwitchMode(Display*a0,int a1,int a2)
{
  Bool r;
  TRACE("Call XF86VidModeSwitchMode\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XF86VidModeSwitchMode(a0,a1,a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XF86VidModeSwitchMode\n");
  return r;
}

Bool TSXF86VidModeSwitchToMode(Display*a0,int a1,XF86VidModeModeInfo*a2)
{
  Bool r;
  TRACE("Call XF86VidModeSwitchToMode\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XF86VidModeSwitchToMode(a0,a1,a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XF86VidModeSwitchToMode\n");
  return r;
}

Bool TSXF86VidModeLockModeSwitch(Display*a0,int a1,int a2)
{
  Bool r;
  TRACE("Call XF86VidModeLockModeSwitch\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XF86VidModeLockModeSwitch(a0,a1,a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XF86VidModeLockModeSwitch\n");
  return r;
}

Bool TSXF86VidModeGetMonitor(Display*a0,int a1,XF86VidModeMonitor*a2)
{
  Bool r;
  TRACE("Call XF86VidModeGetMonitor\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XF86VidModeGetMonitor(a0,a1,a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XF86VidModeGetMonitor\n");
  return r;
}

Bool TSXF86VidModeGetViewPort(Display*a0,int a1,int*a2,int*a3)
{
  Bool r;
  TRACE("Call XF86VidModeGetViewPort\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XF86VidModeGetViewPort(a0,a1,a2,a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XF86VidModeGetViewPort\n");
  return r;
}

Bool TSXF86VidModeSetViewPort(Display*a0,int a1,int a2,int a3)
{
  Bool r;
  TRACE("Call XF86VidModeSetViewPort\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XF86VidModeSetViewPort(a0,a1,a2,a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XF86VidModeSetViewPort\n");
  return r;
}

#endif /* defined(HAVE_LIBXXF86VM) */

#endif /* !defined(X_DISPLAY_MISSING) */
