/*
 * Thread safe wrappers around Xlib calls.
 * This file was generated automatically by tools/make_X11wrappers
 * DO NOT EDIT!
 */

#include "config.h"

#ifdef HAVE_X11_XLIB_H

#include <X11/Xlib.h>

#include "ts_xlib.h"


XFontStruct * TSXLoadQueryFont(Display* a0, const  char* a1)
{
  XFontStruct * r;
  wine_tsx11_lock();
  r = XLoadQueryFont(a0, a1);
  wine_tsx11_unlock();
  return r;
}

XImage * TSXGetImage(Display* a0, Drawable a1, int a2, int a3, unsigned int a4, unsigned int a5, unsigned long a6, int a7)
{
  XImage * r;
  wine_tsx11_lock();
  r = XGetImage(a0, a1, a2, a3, a4, a5, a6, a7);
  wine_tsx11_unlock();
  return r;
}

Display * TSXOpenDisplay(const  char* a0)
{
  Display * r;
  wine_tsx11_lock();
  r = XOpenDisplay(a0);
  wine_tsx11_unlock();
  return r;
}

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

Atom  TSXInternAtom(Display* a0, const  char* a1, int a2)
{
  Atom  r;
  wine_tsx11_lock();
  r = XInternAtom(a0, a1, a2);
  wine_tsx11_unlock();
  return r;
}

Colormap  TSXCreateColormap(Display* a0, Window a1, Visual* a2, int a3)
{
  Colormap  r;
  wine_tsx11_lock();
  r = XCreateColormap(a0, a1, a2, a3);
  wine_tsx11_unlock();
  return r;
}

Pixmap  TSXCreatePixmap(Display* a0, Drawable a1, unsigned int a2, unsigned int a3, unsigned int a4)
{
  Pixmap  r;
  wine_tsx11_lock();
  r = XCreatePixmap(a0, a1, a2, a3, a4);
  wine_tsx11_unlock();
  return r;
}

Pixmap  TSXCreateBitmapFromData(Display* a0, Drawable a1, const  char* a2, unsigned int a3, unsigned int a4)
{
  Pixmap  r;
  wine_tsx11_lock();
  r = XCreateBitmapFromData(a0, a1, a2, a3, a4);
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

char ** TSXListFonts(Display* a0, const  char* a1, int a2, int* a3)
{
  char ** r;
  wine_tsx11_lock();
  r = XListFonts(a0, a1, a2, a3);
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

int * TSXListDepths(Display* a0, int a1, int* a2)
{
  int * r;
  wine_tsx11_lock();
  r = XListDepths(a0, a1, a2);
  wine_tsx11_unlock();
  return r;
}

int   TSXReconfigureWMWindow(Display* a0, Window a1, int a2, unsigned int a3, XWindowChanges* a4)
{
  int   r;
  wine_tsx11_lock();
  r = XReconfigureWMWindow(a0, a1, a2, a3, a4);
  wine_tsx11_unlock();
  return r;
}

int   TSXAllocColor(Display* a0, Colormap a1, XColor* a2)
{
  int   r;
  wine_tsx11_lock();
  r = XAllocColor(a0, a1, a2);
  wine_tsx11_unlock();
  return r;
}

int   TSXAllocColorCells(Display* a0, Colormap a1, int a2, unsigned long* a3, unsigned int a4, unsigned long* a5, unsigned int a6)
{
  int   r;
  wine_tsx11_lock();
  r = XAllocColorCells(a0, a1, a2, a3, a4, a5, a6);
  wine_tsx11_unlock();
  return r;
}

int  TSXBell(Display* a0, int a1)
{
  int  r;
  wine_tsx11_lock();
  r = XBell(a0, a1);
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

int  TSXChangeProperty(Display* a0, Window a1, Atom a2, Atom a3, int a4, int a5, const  unsigned char* a6, int a7)
{
  int  r;
  wine_tsx11_lock();
  r = XChangeProperty(a0, a1, a2, a3, a4, a5, a6, a7);
  wine_tsx11_unlock();
  return r;
}

int  TSXChangeWindowAttributes(Display* a0, Window a1, unsigned long a2, XSetWindowAttributes* a3)
{
  int  r;
  wine_tsx11_lock();
  r = XChangeWindowAttributes(a0, a1, a2, a3);
  wine_tsx11_unlock();
  return r;
}

int  TSXCopyArea(Display* a0, Drawable a1, Drawable a2, GC a3, int a4, int a5, unsigned int a6, unsigned int a7, int a8, int a9)
{
  int  r;
  wine_tsx11_lock();
  r = XCopyArea(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);
  wine_tsx11_unlock();
  return r;
}

int  TSXCopyPlane(Display* a0, Drawable a1, Drawable a2, GC a3, int a4, int a5, unsigned int a6, unsigned int a7, int a8, int a9, unsigned long a10)
{
  int  r;
  wine_tsx11_lock();
  r = XCopyPlane(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
  wine_tsx11_unlock();
  return r;
}

int  TSXDefineCursor(Display* a0, Window a1, Cursor a2)
{
  int  r;
  wine_tsx11_lock();
  r = XDefineCursor(a0, a1, a2);
  wine_tsx11_unlock();
  return r;
}

int  TSXDeleteProperty(Display* a0, Window a1, Atom a2)
{
  int  r;
  wine_tsx11_lock();
  r = XDeleteProperty(a0, a1, a2);
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

int  TSXDrawRectangle(Display* a0, Drawable a1, GC a2, int a3, int a4, unsigned int a5, unsigned int a6)
{
  int  r;
  wine_tsx11_lock();
  r = XDrawRectangle(a0, a1, a2, a3, a4, a5, a6);
  wine_tsx11_unlock();
  return r;
}

int  TSXDrawString16(Display* a0, Drawable a1, GC a2, int a3, int a4, const  XChar2b* a5, int a6)
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

int  TSXFillPolygon(Display* a0, Drawable a1, GC a2, XPoint* a3, int a4, int a5, int a6)
{
  int  r;
  wine_tsx11_lock();
  r = XFillPolygon(a0, a1, a2, a3, a4, a5, a6);
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

int  TSXFlush(Display* a0)
{
  int  r;
  wine_tsx11_lock();
  r = XFlush(a0);
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

int  TSXFreeColormap(Display* a0, Colormap a1)
{
  int  r;
  wine_tsx11_lock();
  r = XFreeColormap(a0, a1);
  wine_tsx11_unlock();
  return r;
}

int  TSXFreeColors(Display* a0, Colormap a1, unsigned long* a2, int a3, unsigned long a4)
{
  int  r;
  wine_tsx11_lock();
  r = XFreeColors(a0, a1, a2, a3, a4);
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

int  TSXFreeFontNames(char** a0)
{
  int  r;
  wine_tsx11_lock();
  r = XFreeFontNames(a0);
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

int   TSXGetFontProperty(XFontStruct* a0, Atom a1, unsigned long* a2)
{
  int   r;
  wine_tsx11_lock();
  r = XGetFontProperty(a0, a1, a2);
  wine_tsx11_unlock();
  return r;
}

int   TSXGetGeometry(Display* a0, Drawable a1, Window* a2, int* a3, int* a4, unsigned int* a5, unsigned int* a6, unsigned int* a7, unsigned int* a8)
{
  int   r;
  wine_tsx11_lock();
  r = XGetGeometry(a0, a1, a2, a3, a4, a5, a6, a7, a8);
  wine_tsx11_unlock();
  return r;
}

int  TSXGetScreenSaver(Display* a0, int* a1, int* a2, int* a3, int* a4)
{
  int  r;
  wine_tsx11_lock();
  r = XGetScreenSaver(a0, a1, a2, a3, a4);
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

int   TSXGetWindowAttributes(Display* a0, Window a1, XWindowAttributes* a2)
{
  int   r;
  wine_tsx11_lock();
  r = XGetWindowAttributes(a0, a1, a2);
  wine_tsx11_unlock();
  return r;
}

int  TSXGrabPointer(Display* a0, Window a1, int a2, unsigned int a3, int a4, int a5, Window a6, Cursor a7, Time a8)
{
  int  r;
  wine_tsx11_lock();
  r = XGrabPointer(a0, a1, a2, a3, a4, a5, a6, a7, a8);
  wine_tsx11_unlock();
  return r;
}

int  TSXGrabServer(Display* a0)
{
  int  r;
  wine_tsx11_lock();
  r = XGrabServer(a0);
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

int  TSXQueryColor(Display* a0, Colormap a1, XColor* a2)
{
  int  r;
  wine_tsx11_lock();
  r = XQueryColor(a0, a1, a2);
  wine_tsx11_unlock();
  return r;
}

int  TSXQueryColors(Display* a0, Colormap a1, XColor* a2, int a3)
{
  int  r;
  wine_tsx11_lock();
  r = XQueryColors(a0, a1, a2, a3);
  wine_tsx11_unlock();
  return r;
}

int   TSXQueryPointer(Display* a0, Window a1, Window* a2, Window* a3, int* a4, int* a5, int* a6, int* a7, unsigned int* a8)
{
  int   r;
  wine_tsx11_lock();
  r = XQueryPointer(a0, a1, a2, a3, a4, a5, a6, a7, a8);
  wine_tsx11_unlock();
  return r;
}

int   TSXQueryTree(Display* a0, Window a1, Window* a2, Window* a3, Window** a4, unsigned int* a5)
{
  int   r;
  wine_tsx11_lock();
  r = XQueryTree(a0, a1, a2, a3, a4, a5);
  wine_tsx11_unlock();
  return r;
}

int  TSXRefreshKeyboardMapping(XMappingEvent* a0)
{
  int  r;
  wine_tsx11_lock();
  r = XRefreshKeyboardMapping(a0);
  wine_tsx11_unlock();
  return r;
}

int   TSXSendEvent(Display* a0, Window a1, int a2, long a3, XEvent* a4)
{
  int   r;
  wine_tsx11_lock();
  r = XSendEvent(a0, a1, a2, a3, a4);
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

int  TSXSetDashes(Display* a0, GC a1, int a2, const  char* a3, int a4)
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

int  TSXSetGraphicsExposures(Display* a0, GC a1, int a2)
{
  int  r;
  wine_tsx11_lock();
  r = XSetGraphicsExposures(a0, a1, a2);
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

int  TSXSetScreenSaver(Display* a0, int a1, int a2, int a3, int a4)
{
  int  r;
  wine_tsx11_lock();
  r = XSetScreenSaver(a0, a1, a2, a3, a4);
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

int  TSXTextExtents16(XFontStruct* a0, const  XChar2b* a1, int a2, int* a3, int* a4, int* a5, XCharStruct* a6)
{
  int  r;
  wine_tsx11_lock();
  r = XTextExtents16(a0, a1, a2, a3, a4, a5, a6);
  wine_tsx11_unlock();
  return r;
}

int  TSXTextWidth16(XFontStruct* a0, const  XChar2b* a1, int a2)
{
  int  r;
  wine_tsx11_lock();
  r = XTextWidth16(a0, a1, a2);
  wine_tsx11_unlock();
  return r;
}

int  TSXUngrabPointer(Display* a0, Time a1)
{
  int  r;
  wine_tsx11_lock();
  r = XUngrabPointer(a0, a1);
  wine_tsx11_unlock();
  return r;
}

int  TSXUngrabServer(Display* a0)
{
  int  r;
  wine_tsx11_lock();
  r = XUngrabServer(a0);
  wine_tsx11_unlock();
  return r;
}

int  TSXUninstallColormap(Display* a0, Colormap a1)
{
  int  r;
  wine_tsx11_lock();
  r = XUninstallColormap(a0, a1);
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

XIM  TSXOpenIM(Display* a0, struct _XrmHashBucketRec* a1, char* a2, char* a3)
{
  XIM  r;
  wine_tsx11_lock();
  r = XOpenIM(a0, a1, a2, a3);
  wine_tsx11_unlock();
  return r;
}

#endif /* defined(HAVE_X11_XLIB_H) */
