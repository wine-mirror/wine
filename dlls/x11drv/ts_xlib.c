/*
 * Thread safe wrappers around Xlib calls.
 * This file was generated automatically by tools/make_X11wrappers
 * DO NOT EDIT!
 */

#include "config.h"

#ifdef HAVE_X11_XLIB_H

#include <X11/Xlib.h>

#include "ts_xlib.h"


char * TSXGetAtomName(Display* a0, Atom a1)
{
  char * r;
  wine_tsx11_lock();
  r = XGetAtomName(a0, a1);
  wine_tsx11_unlock();
  return r;
}

char * TSXKeysymToString(KeySym a0)
{
  char * r;
  wine_tsx11_lock();
  r = XKeysymToString(a0);
  wine_tsx11_unlock();
  return r;
}

Atom  TSXInternAtom(Display* a0, const char* a1, int a2)
{
  Atom  r;
  wine_tsx11_lock();
  r = XInternAtom(a0, a1, a2);
  wine_tsx11_unlock();
  return r;
}

Window  TSXGetSelectionOwner(Display* a0, Atom a1)
{
  Window  r;
  wine_tsx11_lock();
  r = XGetSelectionOwner(a0, a1);
  wine_tsx11_unlock();
  return r;
}

KeySym  TSXKeycodeToKeysym(Display* a0, unsigned int a1, int a2)
{
  KeySym  r;
  wine_tsx11_lock();
  r = XKeycodeToKeysym(a0, a1, a2);
  wine_tsx11_unlock();
  return r;
}

int  TSXChangeGC(Display* a0, GC a1, unsigned long a2, XGCValues* a3)
{
  int  r;
  wine_tsx11_lock();
  r = XChangeGC(a0, a1, a2, a3);
  wine_tsx11_unlock();
  return r;
}

int  TSXChangeProperty(Display* a0, Window a1, Atom a2, Atom a3, int a4, int a5, const unsigned char* a6, int a7)
{
  int  r;
  wine_tsx11_lock();
  r = XChangeProperty(a0, a1, a2, a3, a4, a5, a6, a7);
  wine_tsx11_unlock();
  return r;
}

int  TSXDrawArc(Display* a0, Drawable a1, GC a2, int a3, int a4, unsigned int a5, unsigned int a6, int a7, int a8)
{
  int  r;
  wine_tsx11_lock();
  r = XDrawArc(a0, a1, a2, a3, a4, a5, a6, a7, a8);
  wine_tsx11_unlock();
  return r;
}

int  TSXDrawLine(Display* a0, Drawable a1, GC a2, int a3, int a4, int a5, int a6)
{
  int  r;
  wine_tsx11_lock();
  r = XDrawLine(a0, a1, a2, a3, a4, a5, a6);
  wine_tsx11_unlock();
  return r;
}

int  TSXDrawLines(Display* a0, Drawable a1, GC a2, XPoint* a3, int a4, int a5)
{
  int  r;
  wine_tsx11_lock();
  r = XDrawLines(a0, a1, a2, a3, a4, a5);
  wine_tsx11_unlock();
  return r;
}

int  TSXDrawString16(Display* a0, Drawable a1, GC a2, int a3, int a4, const XChar2b* a5, int a6)
{
  int  r;
  wine_tsx11_lock();
  r = XDrawString16(a0, a1, a2, a3, a4, a5, a6);
  wine_tsx11_unlock();
  return r;
}

int  TSXDrawText16(Display* a0, Drawable a1, GC a2, int a3, int a4, XTextItem16* a5, int a6)
{
  int  r;
  wine_tsx11_lock();
  r = XDrawText16(a0, a1, a2, a3, a4, a5, a6);
  wine_tsx11_unlock();
  return r;
}

int  TSXFillArc(Display* a0, Drawable a1, GC a2, int a3, int a4, unsigned int a5, unsigned int a6, int a7, int a8)
{
  int  r;
  wine_tsx11_lock();
  r = XFillArc(a0, a1, a2, a3, a4, a5, a6, a7, a8);
  wine_tsx11_unlock();
  return r;
}

int  TSXFillRectangle(Display* a0, Drawable a1, GC a2, int a3, int a4, unsigned int a5, unsigned int a6)
{
  int  r;
  wine_tsx11_lock();
  r = XFillRectangle(a0, a1, a2, a3, a4, a5, a6);
  wine_tsx11_unlock();
  return r;
}

int  TSXFree(void* a0)
{
  int  r;
  wine_tsx11_lock();
  r = XFree(a0);
  wine_tsx11_unlock();
  return r;
}

int  TSXFreeFont(Display* a0, XFontStruct* a1)
{
  int  r;
  wine_tsx11_lock();
  r = XFreeFont(a0, a1);
  wine_tsx11_unlock();
  return r;
}

int  TSXFreePixmap(Display* a0, Pixmap a1)
{
  int  r;
  wine_tsx11_lock();
  r = XFreePixmap(a0, a1);
  wine_tsx11_unlock();
  return r;
}

int  TSXGetFontProperty(XFontStruct* a0, Atom a1, unsigned long* a2)
{
  int  r;
  wine_tsx11_lock();
  r = XGetFontProperty(a0, a1, a2);
  wine_tsx11_unlock();
  return r;
}

int  TSXGetGeometry(Display* a0, Drawable a1, Window* a2, int* a3, int* a4, unsigned int* a5, unsigned int* a6, unsigned int* a7, unsigned int* a8)
{
  int  r;
  wine_tsx11_lock();
  r = XGetGeometry(a0, a1, a2, a3, a4, a5, a6, a7, a8);
  wine_tsx11_unlock();
  return r;
}

int  TSXGetWindowProperty(Display* a0, Window a1, Atom a2, long a3, long a4, int a5, Atom a6, Atom* a7, int* a8, unsigned long* a9, unsigned long* a10, unsigned char** a11)
{
  int  r;
  wine_tsx11_lock();
  r = XGetWindowProperty(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
  wine_tsx11_unlock();
  return r;
}

KeyCode  TSXKeysymToKeycode(Display* a0, KeySym a1)
{
  KeyCode  r;
  wine_tsx11_lock();
  r = XKeysymToKeycode(a0, a1);
  wine_tsx11_unlock();
  return r;
}

int  TSXMapWindow(Display* a0, Window a1)
{
  int  r;
  wine_tsx11_lock();
  r = XMapWindow(a0, a1);
  wine_tsx11_unlock();
  return r;
}

int  TSXQueryPointer(Display* a0, Window a1, Window* a2, Window* a3, int* a4, int* a5, int* a6, int* a7, unsigned int* a8)
{
  int  r;
  wine_tsx11_lock();
  r = XQueryPointer(a0, a1, a2, a3, a4, a5, a6, a7, a8);
  wine_tsx11_unlock();
  return r;
}

int  TSXQueryTree(Display* a0, Window a1, Window* a2, Window* a3, Window** a4, unsigned int* a5)
{
  int  r;
  wine_tsx11_lock();
  r = XQueryTree(a0, a1, a2, a3, a4, a5);
  wine_tsx11_unlock();
  return r;
}

int  TSXSetArcMode(Display* a0, GC a1, int a2)
{
  int  r;
  wine_tsx11_lock();
  r = XSetArcMode(a0, a1, a2);
  wine_tsx11_unlock();
  return r;
}

int  TSXSetClipRectangles(Display* a0, GC a1, int a2, int a3, XRectangle* a4, int a5, int a6)
{
  int  r;
  wine_tsx11_lock();
  r = XSetClipRectangles(a0, a1, a2, a3, a4, a5, a6);
  wine_tsx11_unlock();
  return r;
}

int  TSXSetDashes(Display* a0, GC a1, int a2, const char* a3, int a4)
{
  int  r;
  wine_tsx11_lock();
  r = XSetDashes(a0, a1, a2, a3, a4);
  wine_tsx11_unlock();
  return r;
}

int  TSXSetForeground(Display* a0, GC a1, unsigned long a2)
{
  int  r;
  wine_tsx11_lock();
  r = XSetForeground(a0, a1, a2);
  wine_tsx11_unlock();
  return r;
}

int  TSXSetFunction(Display* a0, GC a1, int a2)
{
  int  r;
  wine_tsx11_lock();
  r = XSetFunction(a0, a1, a2);
  wine_tsx11_unlock();
  return r;
}

int  TSXSetLineAttributes(Display* a0, GC a1, unsigned int a2, int a3, int a4, int a5)
{
  int  r;
  wine_tsx11_lock();
  r = XSetLineAttributes(a0, a1, a2, a3, a4, a5);
  wine_tsx11_unlock();
  return r;
}

int  TSXSetSelectionOwner(Display* a0, Atom a1, Window a2, Time a3)
{
  int  r;
  wine_tsx11_lock();
  r = XSetSelectionOwner(a0, a1, a2, a3);
  wine_tsx11_unlock();
  return r;
}

int  TSXSetSubwindowMode(Display* a0, GC a1, int a2)
{
  int  r;
  wine_tsx11_lock();
  r = XSetSubwindowMode(a0, a1, a2);
  wine_tsx11_unlock();
  return r;
}

int  TSXStoreColor(Display* a0, Colormap a1, XColor* a2)
{
  int  r;
  wine_tsx11_lock();
  r = XStoreColor(a0, a1, a2);
  wine_tsx11_unlock();
  return r;
}

int  TSXSync(Display* a0, int a1)
{
  int  r;
  wine_tsx11_lock();
  r = XSync(a0, a1);
  wine_tsx11_unlock();
  return r;
}

int  TSXTextExtents16(XFontStruct* a0, const XChar2b* a1, int a2, int* a3, int* a4, int* a5, XCharStruct* a6)
{
  int  r;
  wine_tsx11_lock();
  r = XTextExtents16(a0, a1, a2, a3, a4, a5, a6);
  wine_tsx11_unlock();
  return r;
}

int  TSXTextWidth16(XFontStruct* a0, const XChar2b* a1, int a2)
{
  int  r;
  wine_tsx11_lock();
  r = XTextWidth16(a0, a1, a2);
  wine_tsx11_unlock();
  return r;
}

int  TSXUnmapWindow(Display* a0, Window a1)
{
  int  r;
  wine_tsx11_lock();
  r = XUnmapWindow(a0, a1);
  wine_tsx11_unlock();
  return r;
}

#endif /* defined(HAVE_X11_XLIB_H) */
