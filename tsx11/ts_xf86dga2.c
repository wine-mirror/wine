/*
 * Thread safe wrappers around xf86dga2 calls.
 * This file was generated automatically by tools/make_X11wrappers
 * DO NOT EDIT!
 */

#include "config.h"

#ifdef HAVE_LIBXXF86DGA2

#include <X11/Xlib.h>
#include <X11/extensions/xf86dga.h>

#include "debugtools.h"
#include "ts_xf86dga2.h"
#include "x11drv.h"

DEFAULT_DEBUG_CHANNEL(x11)

Bool TSXDGAQueryVersion(Display* a0, int* a1, int* a2)
{
  Bool r;
  TRACE("Call XDGAQueryVersion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XDGAQueryVersion( a0,  a1,  a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XDGAQueryVersion\n");
  return r;
}

Bool TSXDGAQueryExtension(Display* a0, int* a1, int* a2)
{
  Bool r;
  TRACE("Call XDGAQueryExtension\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XDGAQueryExtension( a0,  a1,  a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XDGAQueryExtension\n");
  return r;
}

XDGAMode* TSXDGAQueryModes(Display* a0, int a1, int* a2)
{
  XDGAMode* r;
  TRACE("Call XDGAQueryModes\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XDGAQueryModes( a0,  a1,  a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XDGAQueryModes\n");
  return r;
}

XDGADevice* TSXDGASetMode(Display* a0, int a1, int a2)
{
  XDGADevice* r;
  TRACE("Call XDGASetMode\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XDGASetMode( a0,  a1,  a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XDGASetMode\n");
  return r;
}

Bool TSXDGAOpenFramebuffer(Display* a0, int a1)
{
  Bool r;
  TRACE("Call XDGAOpenFramebuffer\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XDGAOpenFramebuffer( a0,  a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XDGAOpenFramebuffer\n");
  return r;
}

void TSXDGACloseFramebuffer(Display* a0, int a1)
{
  TRACE("Call XDGACloseFramebuffer\n");
  EnterCriticalSection( &X11DRV_CritSection );
  XDGACloseFramebuffer( a0,  a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XDGACloseFramebuffer\n");
}

void TSXDGASetViewport(Display* a0, int a1, int a2, int a3, int a4)
{
  TRACE("Call XDGASetViewport\n");
  EnterCriticalSection( &X11DRV_CritSection );
  XDGASetViewport( a0,  a1,  a2,  a3,  a4);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XDGASetViewport\n");
}

void TSXDGAInstallColormap(Display* a0, int a1, Colormap a2)
{
  TRACE("Call XDGAInstallColormap\n");
  EnterCriticalSection( &X11DRV_CritSection );
  XDGAInstallColormap( a0,  a1,  a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XDGAInstallColormap\n");
}

Colormap TSXDGACreateColormap(Display* a0, int a1, XDGADevice* a2, int a3)
{
  Colormap r;
  TRACE("Call XDGACreateColormap\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XDGACreateColormap( a0,  a1,  a2,  a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XDGACreateColormap\n");
  return r;
}

void TSXDGASelectInput(Display* a0, int a1, long a2)
{
  TRACE("Call XDGASelectInput\n");
  EnterCriticalSection( &X11DRV_CritSection );
  XDGASelectInput( a0,  a1,  a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XDGASelectInput\n");
}

void TSXDGAFillRectangle(Display* a0, int a1, int a2, int a3, unsigned int a4, unsigned int a5, unsigned long a6)
{
  TRACE("Call XDGAFillRectangle\n");
  EnterCriticalSection( &X11DRV_CritSection );
  XDGAFillRectangle( a0,  a1,  a2,  a3,  a4,  a5,  a6);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XDGAFillRectangle\n");
}

void TSXDGACopyArea(Display* a0, int a1, int a2, int a3, unsigned int a4, unsigned int a5, int a6, int a7)
{
  TRACE("Call XDGACopyArea\n");
  EnterCriticalSection( &X11DRV_CritSection );
  XDGACopyArea( a0,  a1,  a2,  a3,  a4,  a5,  a6,  a7);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XDGACopyArea\n");
}

void TSXDGACopyTransparentArea(Display* a0, int a1, int a2, int a3, unsigned int a4, unsigned int a5, int a6, int a7, unsigned long a8)
{
  TRACE("Call XDGACopyTransparentArea\n");
  EnterCriticalSection( &X11DRV_CritSection );
  XDGACopyTransparentArea( a0,  a1,  a2,  a3,  a4,  a5,  a6,  a7,  a8);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XDGACopyTransparentArea\n");
}

int TSXDGAGetViewportStatus(Display* a0, int a1)
{
  int r;
  TRACE("Call XDGAGetViewportStatus\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XDGAGetViewportStatus( a0,  a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XDGAGetViewportStatus\n");
  return r;
}

void TSXDGASync(Display* a0, int a1)
{
  TRACE("Call XDGASync\n");
  EnterCriticalSection( &X11DRV_CritSection );
  XDGASync( a0,  a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XDGASync\n");
}

Bool TSXDGASetClientVersion(Display* a0)
{
  Bool r;
  TRACE("Call XDGASetClientVersion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XDGASetClientVersion( a0);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XDGASetClientVersion\n");
  return r;
}

void TSXDGAChangePixmapMode(Display* a0, int a1, int* a2, int* a3, int a4)
{
  TRACE("Call XDGAChangePixmapMode\n");
  EnterCriticalSection( &X11DRV_CritSection );
  XDGAChangePixmapMode( a0,  a1,  a2,  a3,  a4);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XDGAChangePixmapMode\n");
}

void TSXDGAKeyEventToXKeyEvent(XDGAKeyEvent* a0, XKeyEvent* a1)
{
  TRACE("Call XDGAKeyEventToXKeyEvent\n");
  EnterCriticalSection( &X11DRV_CritSection );
  XDGAKeyEventToXKeyEvent( a0,  a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XDGAKeyEventToXKeyEvent\n");
}

#endif /* defined(HAVE_LIBXXF86DGA2) */

