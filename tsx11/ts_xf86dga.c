/*
 * Thread safe wrappers around xf86dga calls.
 * This file was generated automatically by tools/make_X11wrappers
 * DO NOT EDIT!
 */
#include "config.h"
#ifdef HAVE_LIBXXF86DGA

#include <X11/Xlib.h>
#include <X11/extensions/xf86dga.h>
#include "x11drv.h"
#include "debug.h"

Bool TSXF86DGAQueryVersion(Display*a0,int*a1,int*a2)
{
  Bool r;
  TRACE(x11, "Call XF86DGAQueryVersion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XF86DGAQueryVersion(a0,a1,a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XF86DGAQueryVersion\n");
  return r;
}

Bool TSXF86DGAQueryExtension(Display*a0,int*a1,int*a2)
{
  Bool r;
  TRACE(x11, "Call XF86DGAQueryExtension\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XF86DGAQueryExtension(a0,a1,a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XF86DGAQueryExtension\n");
  return r;
}

Status TSXF86DGAGetVideo(Display*a0,int a1,char**a2,int*a3,int*a4,int*a5)
{
  Status r;
  TRACE(x11, "Call XF86DGAGetVideo\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XF86DGAGetVideo(a0,a1,a2,a3,a4,a5);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XF86DGAGetVideo\n");
  return r;
}

Status TSXF86DGADirectVideo(Display*a0,int a1,int a2)
{
  Status r;
  TRACE(x11, "Call XF86DGADirectVideo\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XF86DGADirectVideo(a0,a1,a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XF86DGADirectVideo\n");
  return r;
}

Status TSXF86DGAGetViewPortSize(Display*a0,int a1,int *a2,int *a3)
{
  Status r;
  TRACE(x11, "Call XF86DGAGetViewPortSize\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XF86DGAGetViewPortSize(a0,a1,a2,a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XF86DGAGetViewPortSize\n");
  return r;
}

Status TSXF86DGASetViewPort(Display*a0,int a1,int a2,int a3)
{
  Status r;
  TRACE(x11, "Call XF86DGASetViewPort\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XF86DGASetViewPort(a0,a1,a2,a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XF86DGASetViewPort\n");
  return r;
}

Status TSXF86DGAInstallColormap(Display*a0,int a1,Colormap a2)
{
  Status r;
  TRACE(x11, "Call XF86DGAInstallColormap\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XF86DGAInstallColormap(a0,a1,a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XF86DGAInstallColormap\n");
  return r;
}

Status TSXF86DGAQueryDirectVideo(Display*a0,int a1,int *a2)
{
  Status r;
  TRACE(x11, "Call XF86DGAQueryDirectVideo\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XF86DGAQueryDirectVideo(a0,a1,a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XF86DGAQueryDirectVideo\n");
  return r;
}

Status TSXF86DGAViewPortChanged(Display*a0,int a1,int a2)
{
  Status r;
  TRACE(x11, "Call XF86DGAViewPortChanged\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XF86DGAViewPortChanged(a0,a1,a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XF86DGAViewPortChanged\n");
  return r;
}
#endif
