/*
 * Thread safe wrappers around xf86dga calls.
 * This file was generated automatically by tools/make_X11wrappers
 * DO NOT EDIT!
 */

#include "config.h"

#ifdef HAVE_LIBXXF86DGA

#include <X11/Xlib.h>
#include <X11/extensions/xf86dga.h>

#include "ts_xf86dga.h"


Bool TSXF86DGAQueryVersion(Display*a0,int*a1,int*a2)
{
  Bool r;
  wine_tsx11_lock();
  r = XF86DGAQueryVersion(a0,a1,a2);
  wine_tsx11_unlock();
  return r;
}

Bool TSXF86DGAQueryExtension(Display*a0,int*a1,int*a2)
{
  Bool r;
  wine_tsx11_lock();
  r = XF86DGAQueryExtension(a0,a1,a2);
  wine_tsx11_unlock();
  return r;
}

Status TSXF86DGAGetVideo(Display*a0,int a1,char**a2,int*a3,int*a4,int*a5)
{
  Status r;
  wine_tsx11_lock();
  r = XF86DGAGetVideo(a0,a1,a2,a3,a4,a5);
  wine_tsx11_unlock();
  return r;
}

Status TSXF86DGADirectVideo(Display*a0,int a1,int a2)
{
  Status r;
  wine_tsx11_lock();
  r = XF86DGADirectVideo(a0,a1,a2);
  wine_tsx11_unlock();
  return r;
}

Status TSXF86DGAGetViewPortSize(Display*a0,int a1,int *a2,int *a3)
{
  Status r;
  wine_tsx11_lock();
  r = XF86DGAGetViewPortSize(a0,a1,a2,a3);
  wine_tsx11_unlock();
  return r;
}

Status TSXF86DGASetViewPort(Display*a0,int a1,int a2,int a3)
{
  Status r;
  wine_tsx11_lock();
  r = XF86DGASetViewPort(a0,a1,a2,a3);
  wine_tsx11_unlock();
  return r;
}

Status TSXF86DGAInstallColormap(Display*a0,int a1,Colormap a2)
{
  Status r;
  wine_tsx11_lock();
  r = XF86DGAInstallColormap(a0,a1,a2);
  wine_tsx11_unlock();
  return r;
}

Status TSXF86DGAQueryDirectVideo(Display*a0,int a1,int *a2)
{
  Status r;
  wine_tsx11_lock();
  r = XF86DGAQueryDirectVideo(a0,a1,a2);
  wine_tsx11_unlock();
  return r;
}

Status TSXF86DGAViewPortChanged(Display*a0,int a1,int a2)
{
  Status r;
  wine_tsx11_lock();
  r = XF86DGAViewPortChanged(a0,a1,a2);
  wine_tsx11_unlock();
  return r;
}

#endif /* defined(HAVE_LIBXXF86DGA) */

