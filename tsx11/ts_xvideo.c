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

#include "ts_xvideo.h"


int TSXvQueryExtension(Display* a0, unsigned int* a1, unsigned int* a2, unsigned int* a3, unsigned int* a4, unsigned int* a5)
{
  int r;
  wine_tsx11_lock();
  r = XvQueryExtension( a0,  a1,  a2,  a3,  a4,  a5);
  wine_tsx11_unlock();
  return r;
}

int TSXvQueryAdaptors(Display* a0, Window a1, unsigned int* a2, XvAdaptorInfo** a3)
{
  int r;
  wine_tsx11_lock();
  r = XvQueryAdaptors( a0,  a1,  a2,  a3);
  wine_tsx11_unlock();
  return r;
}

int TSXvQueryEncodings(Display* a0, XvPortID a1, unsigned int* a2, XvEncodingInfo** a3)
{
  int r;
  wine_tsx11_lock();
  r = XvQueryEncodings( a0,  a1,  a2,  a3);
  wine_tsx11_unlock();
  return r;
}

int TSXvPutVideo(Display* a0, XvPortID a1, Drawable a2, GC a3, int a4, int a5, unsigned int a6, unsigned int a7, int a8, int a9, unsigned int a10, unsigned int a11)
{
  int r;
  wine_tsx11_lock();
  r = XvPutVideo( a0,  a1,  a2,  a3,  a4,  a5,  a6,  a7,  a8,  a9,  a10,  a11);
  wine_tsx11_unlock();
  return r;
}

int TSXvPutStill(Display* a0, XvPortID a1, Drawable a2, GC a3, int a4, int a5, unsigned int a6, unsigned int a7, int a8, int a9, unsigned int a10, unsigned int a11)
{
  int r;
  wine_tsx11_lock();
  r = XvPutStill( a0,  a1,  a2,  a3,  a4,  a5,  a6,  a7,  a8,  a9,  a10,  a11);
  wine_tsx11_unlock();
  return r;
}

int TSXvGetVideo(Display* a0, XvPortID a1, Drawable a2, GC a3, int a4, int a5, unsigned int a6, unsigned int a7, int a8, int a9, unsigned int a10, unsigned int a11)
{
  int r;
  wine_tsx11_lock();
  r = XvGetVideo( a0,  a1,  a2,  a3,  a4,  a5,  a6,  a7,  a8,  a9,  a10,  a11);
  wine_tsx11_unlock();
  return r;
}

int TSXvGetStill(Display* a0, XvPortID a1, Drawable a2, GC a3, int a4, int a5, unsigned int a6, unsigned int a7, int a8, int a9, unsigned int a10, unsigned int a11)
{
  int r;
  wine_tsx11_lock();
  r = XvGetStill( a0,  a1,  a2,  a3,  a4,  a5,  a6,  a7,  a8,  a9,  a10,  a11);
  wine_tsx11_unlock();
  return r;
}

int TSXvStopVideo(Display* a0, XvPortID a1, Drawable a2)
{
  int r;
  wine_tsx11_lock();
  r = XvStopVideo( a0,  a1,  a2);
  wine_tsx11_unlock();
  return r;
}

int TSXvGrabPort(Display* a0, XvPortID a1, Time a2)
{
  int r;
  wine_tsx11_lock();
  r = XvGrabPort( a0,  a1,  a2);
  wine_tsx11_unlock();
  return r;
}

int TSXvUngrabPort(Display* a0, XvPortID a1, Time a2)
{
  int r;
  wine_tsx11_lock();
  r = XvUngrabPort( a0,  a1,  a2);
  wine_tsx11_unlock();
  return r;
}

int TSXvSelectVideoNotify(Display* a0, Drawable a1, Bool a2)
{
  int r;
  wine_tsx11_lock();
  r = XvSelectVideoNotify( a0,  a1,  a2);
  wine_tsx11_unlock();
  return r;
}

int TSXvSelectPortNotify(Display* a0, XvPortID a1, Bool a2)
{
  int r;
  wine_tsx11_lock();
  r = XvSelectPortNotify( a0,  a1,  a2);
  wine_tsx11_unlock();
  return r;
}

int TSXvSetPortAttribute(Display* a0, XvPortID a1, Atom a2, int a3)
{
  int r;
  wine_tsx11_lock();
  r = XvSetPortAttribute( a0,  a1,  a2,  a3);
  wine_tsx11_unlock();
  return r;
}

int TSXvGetPortAttribute(Display* a0, XvPortID a1, Atom a2, int* a3)
{
  int r;
  wine_tsx11_lock();
  r = XvGetPortAttribute( a0,  a1,  a2,  a3);
  wine_tsx11_unlock();
  return r;
}

int TSXvQueryBestSize(Display* a0, XvPortID a1, Bool a2, unsigned int a3, unsigned int a4, unsigned int a5, unsigned int a6, unsigned int* a7, unsigned int* a8)
{
  int r;
  wine_tsx11_lock();
  r = XvQueryBestSize( a0,  a1,  a2,  a3,  a4,  a5,  a6,  a7,  a8);
  wine_tsx11_unlock();
  return r;
}

XvAttribute* TSXvQueryPortAttributes(Display* a0, XvPortID a1, int* a2)
{
  XvAttribute* r;
  wine_tsx11_lock();
  r = XvQueryPortAttributes( a0,  a1,  a2);
  wine_tsx11_unlock();
  return r;
}

void TSXvFreeAdaptorInfo(XvAdaptorInfo* a0)
{
  wine_tsx11_lock();
  XvFreeAdaptorInfo( a0);
  wine_tsx11_unlock();
}

void TSXvFreeEncodingInfo(XvEncodingInfo* a0)
{
  wine_tsx11_lock();
  XvFreeEncodingInfo( a0);
  wine_tsx11_unlock();
}

XvImageFormatValues * TSXvListImageFormats(Display* a0, XvPortID a1, int* a2)
{
  XvImageFormatValues * r;
  wine_tsx11_lock();
  r = XvListImageFormats( a0,  a1,  a2);
  wine_tsx11_unlock();
  return r;
}

XvImage * TSXvCreateImage(Display* a0, XvPortID a1, int a2, char* a3, int a4, int a5)
{
  XvImage * r;
  wine_tsx11_lock();
  r = XvCreateImage( a0,  a1,  a2,  a3,  a4,  a5);
  wine_tsx11_unlock();
  return r;
}

int TSXvPutImage(Display* a0, XvPortID a1, Drawable a2, GC a3, XvImage* a4, int a5, int a6, unsigned int a7, unsigned int a8, int a9, int a10, unsigned int a11, unsigned int a12)
{
  int r;
  wine_tsx11_lock();
  r = XvPutImage( a0,  a1,  a2,  a3,  a4,  a5,  a6,  a7,  a8,  a9,  a10,  a11,  a12);
  wine_tsx11_unlock();
  return r;
}

int TSXvShmPutImage(Display* a0, XvPortID a1, Drawable a2, GC a3, XvImage* a4, int a5, int a6, unsigned int a7, unsigned int a8, int a9, int a10, unsigned int a11, unsigned int a12, Bool a13)
{
  int r;
  wine_tsx11_lock();
  r = XvShmPutImage( a0,  a1,  a2,  a3,  a4,  a5,  a6,  a7,  a8,  a9,  a10,  a11,  a12,  a13);
  wine_tsx11_unlock();
  return r;
}

XvImage * TSXvShmCreateImage(Display* a0, XvPortID a1, int a2, char* a3, int a4, int a5, XShmSegmentInfo* a6)
{
  XvImage * r;
  wine_tsx11_lock();
  r = XvShmCreateImage( a0,  a1,  a2,  a3,  a4,  a5,  a6);
  wine_tsx11_unlock();
  return r;
}

#endif /* defined(HAVE_XVIDEO) */

