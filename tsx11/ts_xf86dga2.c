/*
 * Thread safe wrappers around xf86dga2 calls.
 * This file was generated automatically by tools/make_X11wrappers
 * DO NOT EDIT!
 */

#include "config.h"

#ifdef HAVE_LIBXXF86DGA2

#include <X11/Xlib.h>
#include <X11/extensions/xf86dga.h>

#include "ts_xf86dga2.h"


Bool TSXDGAQueryVersion(Display* a0, int* a1, int* a2)
{
  Bool r;
  wine_tsx11_lock();
  r = XDGAQueryVersion( a0,  a1,  a2);
  wine_tsx11_unlock();
  return r;
}

Bool TSXDGAQueryExtension(Display* a0, int* a1, int* a2)
{
  Bool r;
  wine_tsx11_lock();
  r = XDGAQueryExtension( a0,  a1,  a2);
  wine_tsx11_unlock();
  return r;
}

XDGAMode* TSXDGAQueryModes(Display* a0, int a1, int* a2)
{
  XDGAMode* r;
  wine_tsx11_lock();
  r = XDGAQueryModes( a0,  a1,  a2);
  wine_tsx11_unlock();
  return r;
}

XDGADevice* TSXDGASetMode(Display* a0, int a1, int a2)
{
  XDGADevice* r;
  wine_tsx11_lock();
  r = XDGASetMode( a0,  a1,  a2);
  wine_tsx11_unlock();
  return r;
}

Bool TSXDGAOpenFramebuffer(Display* a0, int a1)
{
  Bool r;
  wine_tsx11_lock();
  r = XDGAOpenFramebuffer( a0,  a1);
  wine_tsx11_unlock();
  return r;
}

void TSXDGACloseFramebuffer(Display* a0, int a1)
{
  wine_tsx11_lock();
  XDGACloseFramebuffer( a0,  a1);
  wine_tsx11_unlock();
}

void TSXDGASetViewport(Display* a0, int a1, int a2, int a3, int a4)
{
  wine_tsx11_lock();
  XDGASetViewport( a0,  a1,  a2,  a3,  a4);
  wine_tsx11_unlock();
}

void TSXDGAInstallColormap(Display* a0, int a1, Colormap a2)
{
  wine_tsx11_lock();
  XDGAInstallColormap( a0,  a1,  a2);
  wine_tsx11_unlock();
}

Colormap TSXDGACreateColormap(Display* a0, int a1, XDGADevice* a2, int a3)
{
  Colormap r;
  wine_tsx11_lock();
  r = XDGACreateColormap( a0,  a1,  a2,  a3);
  wine_tsx11_unlock();
  return r;
}

void TSXDGASelectInput(Display* a0, int a1, long a2)
{
  wine_tsx11_lock();
  XDGASelectInput( a0,  a1,  a2);
  wine_tsx11_unlock();
}

#endif /* defined(HAVE_LIBXXF86DGA2) */

