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

void TSXDGAFillRectangle(Display* a0, int a1, int a2, int a3, unsigned int a4, unsigned int a5, unsigned long a6)
{
  wine_tsx11_lock();
  XDGAFillRectangle( a0,  a1,  a2,  a3,  a4,  a5,  a6);
  wine_tsx11_unlock();
}

void TSXDGACopyArea(Display* a0, int a1, int a2, int a3, unsigned int a4, unsigned int a5, int a6, int a7)
{
  wine_tsx11_lock();
  XDGACopyArea( a0,  a1,  a2,  a3,  a4,  a5,  a6,  a7);
  wine_tsx11_unlock();
}

void TSXDGACopyTransparentArea(Display* a0, int a1, int a2, int a3, unsigned int a4, unsigned int a5, int a6, int a7, unsigned long a8)
{
  wine_tsx11_lock();
  XDGACopyTransparentArea( a0,  a1,  a2,  a3,  a4,  a5,  a6,  a7,  a8);
  wine_tsx11_unlock();
}

int TSXDGAGetViewportStatus(Display* a0, int a1)
{
  int r;
  wine_tsx11_lock();
  r = XDGAGetViewportStatus( a0,  a1);
  wine_tsx11_unlock();
  return r;
}

void TSXDGASync(Display* a0, int a1)
{
  wine_tsx11_lock();
  XDGASync( a0,  a1);
  wine_tsx11_unlock();
}

Bool TSXDGASetClientVersion(Display* a0)
{
  Bool r;
  wine_tsx11_lock();
  r = XDGASetClientVersion( a0);
  wine_tsx11_unlock();
  return r;
}

void TSXDGAChangePixmapMode(Display* a0, int a1, int* a2, int* a3, int a4)
{
  wine_tsx11_lock();
  XDGAChangePixmapMode( a0,  a1,  a2,  a3,  a4);
  wine_tsx11_unlock();
}

void TSXDGAKeyEventToXKeyEvent(XDGAKeyEvent* a0, XKeyEvent* a1)
{
  wine_tsx11_lock();
  XDGAKeyEventToXKeyEvent( a0,  a1);
  wine_tsx11_unlock();
}

#endif /* defined(HAVE_LIBXXF86DGA2) */

