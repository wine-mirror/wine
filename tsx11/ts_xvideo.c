/*
 * Thread safe wrappers around xvideo calls.
 * This file was generated automatically by tools/make_X11wrappers
 * DO NOT EDIT!
 */

#include "config.h"

#ifdef HAVE_XVIDEO

#include <X11/Xlib.h>
#include <X11/extensions/Xv.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xvlib.h>

#include "debugtools.h"
#include "ts_xvideo.h"
#include "x11drv.h"

DEFAULT_DEBUG_CHANNEL(x11);

int TSXvQueryExtension(Display* a0, unsigned int* a1, unsigned int* a2, unsigned int* a3, unsigned int* a4, unsigned int* a5)
{
  int r;
  TRACE("Call XvQueryExtension\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XvQueryExtension( a0,  a1,  a2,  a3,  a4,  a5);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XvQueryExtension\n");
  return r;
}

int TSXvQueryAdaptors(Display* a0, Window a1, unsigned int* a2, XvAdaptorInfo** a3)
{
  int r;
  TRACE("Call XvQueryAdaptors\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XvQueryAdaptors( a0,  a1,  a2,  a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XvQueryAdaptors\n");
  return r;
}

int TSXvQueryEncodings(Display* a0, XvPortID a1, unsigned int* a2, XvEncodingInfo** a3)
{
  int r;
  TRACE("Call XvQueryEncodings\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XvQueryEncodings( a0,  a1,  a2,  a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XvQueryEncodings\n");
  return r;
}

int TSXvPutVideo(Display* a0, XvPortID a1, Drawable a2, GC a3, int a4, int a5, unsigned int a6, unsigned int a7, int a8, int a9, unsigned int a10, unsigned int a11)
{
  int r;
  TRACE("Call XvPutVideo\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XvPutVideo( a0,  a1,  a2,  a3,  a4,  a5,  a6,  a7,  a8,  a9,  a10,  a11);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XvPutVideo\n");
  return r;
}

int TSXvPutStill(Display* a0, XvPortID a1, Drawable a2, GC a3, int a4, int a5, unsigned int a6, unsigned int a7, int a8, int a9, unsigned int a10, unsigned int a11)
{
  int r;
  TRACE("Call XvPutStill\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XvPutStill( a0,  a1,  a2,  a3,  a4,  a5,  a6,  a7,  a8,  a9,  a10,  a11);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XvPutStill\n");
  return r;
}

int TSXvGetVideo(Display* a0, XvPortID a1, Drawable a2, GC a3, int a4, int a5, unsigned int a6, unsigned int a7, int a8, int a9, unsigned int a10, unsigned int a11)
{
  int r;
  TRACE("Call XvGetVideo\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XvGetVideo( a0,  a1,  a2,  a3,  a4,  a5,  a6,  a7,  a8,  a9,  a10,  a11);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XvGetVideo\n");
  return r;
}

int TSXvGetStill(Display* a0, XvPortID a1, Drawable a2, GC a3, int a4, int a5, unsigned int a6, unsigned int a7, int a8, int a9, unsigned int a10, unsigned int a11)
{
  int r;
  TRACE("Call XvGetStill\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XvGetStill( a0,  a1,  a2,  a3,  a4,  a5,  a6,  a7,  a8,  a9,  a10,  a11);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XvGetStill\n");
  return r;
}

int TSXvStopVideo(Display* a0, XvPortID a1, Drawable a2)
{
  int r;
  TRACE("Call XvStopVideo\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XvStopVideo( a0,  a1,  a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XvStopVideo\n");
  return r;
}

int TSXvGrabPort(Display* a0, XvPortID a1, Time a2)
{
  int r;
  TRACE("Call XvGrabPort\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XvGrabPort( a0,  a1,  a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XvGrabPort\n");
  return r;
}

int TSXvUngrabPort(Display* a0, XvPortID a1, Time a2)
{
  int r;
  TRACE("Call XvUngrabPort\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XvUngrabPort( a0,  a1,  a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XvUngrabPort\n");
  return r;
}

int TSXvSelectVideoNotify(Display* a0, Drawable a1, Bool a2)
{
  int r;
  TRACE("Call XvSelectVideoNotify\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XvSelectVideoNotify( a0,  a1,  a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XvSelectVideoNotify\n");
  return r;
}

int TSXvSelectPortNotify(Display* a0, XvPortID a1, Bool a2)
{
  int r;
  TRACE("Call XvSelectPortNotify\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XvSelectPortNotify( a0,  a1,  a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XvSelectPortNotify\n");
  return r;
}

int TSXvSetPortAttribute(Display* a0, XvPortID a1, Atom a2, int a3)
{
  int r;
  TRACE("Call XvSetPortAttribute\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XvSetPortAttribute( a0,  a1,  a2,  a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XvSetPortAttribute\n");
  return r;
}

int TSXvGetPortAttribute(Display* a0, XvPortID a1, Atom a2, int* a3)
{
  int r;
  TRACE("Call XvGetPortAttribute\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XvGetPortAttribute( a0,  a1,  a2,  a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XvGetPortAttribute\n");
  return r;
}

int TSXvQueryBestSize(Display* a0, XvPortID a1, Bool a2, unsigned int a3, unsigned int a4, unsigned int a5, unsigned int a6, unsigned int* a7, unsigned int* a8)
{
  int r;
  TRACE("Call XvQueryBestSize\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XvQueryBestSize( a0,  a1,  a2,  a3,  a4,  a5,  a6,  a7,  a8);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XvQueryBestSize\n");
  return r;
}

XvAttribute* TSXvQueryPortAttributes(Display* a0, XvPortID a1, int* a2)
{
  XvAttribute* r;
  TRACE("Call XvQueryPortAttributes\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XvQueryPortAttributes( a0,  a1,  a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XvQueryPortAttributes\n");
  return r;
}

void TSXvFreeAdaptorInfo(XvAdaptorInfo* a0)
{
  TRACE("Call XvFreeAdaptorInfo\n");
  EnterCriticalSection( &X11DRV_CritSection );
  XvFreeAdaptorInfo( a0);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XvFreeAdaptorInfo\n");
}

void TSXvFreeEncodingInfo(XvEncodingInfo* a0)
{
  TRACE("Call XvFreeEncodingInfo\n");
  EnterCriticalSection( &X11DRV_CritSection );
  XvFreeEncodingInfo( a0);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XvFreeEncodingInfo\n");
}

XvImageFormatValues * TSXvListImageFormats(Display* a0, XvPortID a1, int* a2)
{
  XvImageFormatValues * r;
  TRACE("Call XvListImageFormats\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XvListImageFormats( a0,  a1,  a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XvListImageFormats\n");
  return r;
}

XvImage * TSXvCreateImage(Display* a0, XvPortID a1, int a2, char* a3, int a4, int a5)
{
  XvImage * r;
  TRACE("Call XvCreateImage\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XvCreateImage( a0,  a1,  a2,  a3,  a4,  a5);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XvCreateImage\n");
  return r;
}

int TSXvPutImage(Display* a0, XvPortID a1, Drawable a2, GC a3, XvImage* a4, int a5, int a6, unsigned int a7, unsigned int a8, int a9, int a10, unsigned int a11, unsigned int a12)
{
  int r;
  TRACE("Call XvPutImage\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XvPutImage( a0,  a1,  a2,  a3,  a4,  a5,  a6,  a7,  a8,  a9,  a10,  a11,  a12);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XvPutImage\n");
  return r;
}

int TSXvShmPutImage(Display* a0, XvPortID a1, Drawable a2, GC a3, XvImage* a4, int a5, int a6, unsigned int a7, unsigned int a8, int a9, int a10, unsigned int a11, unsigned int a12, Bool a13)
{
  int r;
  TRACE("Call XvShmPutImage\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XvShmPutImage( a0,  a1,  a2,  a3,  a4,  a5,  a6,  a7,  a8,  a9,  a10,  a11,  a12,  a13);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XvShmPutImage\n");
  return r;
}

XvImage * TSXvShmCreateImage(Display* a0, XvPortID a1, int a2, char* a3, int a4, int a5, XShmSegmentInfo* a6)
{
  XvImage * r;
  TRACE("Call XvShmCreateImage\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XvShmCreateImage( a0,  a1,  a2,  a3,  a4,  a5,  a6);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XvShmCreateImage\n");
  return r;
}

#endif /* defined(HAVE_XVIDEO) */

