/*
 * Thread safe wrappers around Xlib calls.
 * This file was generated automatically by tools/make_X11wrappers
 *
 * Copyright 1998 Kristian Nielsen
 */

#include <X11/Xlib.h>
#include "tsx11defs.h"
#include "stddebug.h"
#include "debug.h"

XFontStruct * TSXLoadQueryFont(Display* a0, const  char* a1)
{
  XFontStruct * r;
  dprintf_x11(stddeb, "Call XLoadQueryFont\n");
  X11_LOCK();
  r = XLoadQueryFont(a0, a1);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XLoadQueryFont\n");
  return r;
}

XModifierKeymap	* TSXGetModifierMapping(Display* a0)
{
  XModifierKeymap	* r;
  dprintf_x11(stddeb, "Call XGetModifierMapping\n");
  X11_LOCK();
  r = XGetModifierMapping(a0);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XGetModifierMapping\n");
  return r;
}

XImage * TSXCreateImage(Display* a0, Visual* a1, unsigned int a2, int a3, int a4, char* a5, unsigned int a6, unsigned int a7, int a8, int a9)
{
  XImage * r;
  dprintf_x11(stddeb, "Call XCreateImage\n");
  X11_LOCK();
  r = XCreateImage(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XCreateImage\n");
  return r;
}

XImage * TSXGetImage(Display* a0, Drawable a1, int a2, int a3, unsigned int a4, unsigned int a5, unsigned long a6, int a7)
{
  XImage * r;
  dprintf_x11(stddeb, "Call XGetImage\n");
  X11_LOCK();
  r = XGetImage(a0, a1, a2, a3, a4, a5, a6, a7);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XGetImage\n");
  return r;
}

Display * TSXOpenDisplay(const  char* a0)
{
  Display * r;
  dprintf_x11(stddeb, "Call XOpenDisplay\n");
  X11_LOCK();
  r = XOpenDisplay(a0);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XOpenDisplay\n");
  return r;
}

void  TSXrmInitialize(void)
{
  dprintf_x11(stddeb, "Call XrmInitialize\n");
  X11_LOCK();
  XrmInitialize();
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XrmInitialize\n");
}

char * TSXGetAtomName(Display* a0, Atom a1)
{
  char * r;
  dprintf_x11(stddeb, "Call XGetAtomName\n");
  X11_LOCK();
  r = XGetAtomName(a0, a1);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XGetAtomName\n");
  return r;
}

char * TSXKeysymToString(KeySym a0)
{
  char * r;
  dprintf_x11(stddeb, "Call XKeysymToString\n");
  X11_LOCK();
  r = XKeysymToString(a0);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XKeysymToString\n");
  return r;
}

Atom  TSXInternAtom(Display* a0, const  char* a1, int a2)
{
  Atom  r;
  dprintf_x11(stddeb, "Call XInternAtom\n");
  X11_LOCK();
  r = XInternAtom(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XInternAtom\n");
  return r;
}

Colormap  TSXCreateColormap(Display* a0, Window a1, Visual* a2, int a3)
{
  Colormap  r;
  dprintf_x11(stddeb, "Call XCreateColormap\n");
  X11_LOCK();
  r = XCreateColormap(a0, a1, a2, a3);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XCreateColormap\n");
  return r;
}

Cursor  TSXCreatePixmapCursor(Display* a0, Pixmap a1, Pixmap a2, XColor* a3, XColor* a4, unsigned int a5, unsigned int a6)
{
  Cursor  r;
  dprintf_x11(stddeb, "Call XCreatePixmapCursor\n");
  X11_LOCK();
  r = XCreatePixmapCursor(a0, a1, a2, a3, a4, a5, a6);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XCreatePixmapCursor\n");
  return r;
}

Cursor  TSXCreateFontCursor(Display* a0, unsigned int a1)
{
  Cursor  r;
  dprintf_x11(stddeb, "Call XCreateFontCursor\n");
  X11_LOCK();
  r = XCreateFontCursor(a0, a1);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XCreateFontCursor\n");
  return r;
}

GC  TSXCreateGC(Display* a0, Drawable a1, unsigned long a2, XGCValues* a3)
{
  GC  r;
  dprintf_x11(stddeb, "Call XCreateGC\n");
  X11_LOCK();
  r = XCreateGC(a0, a1, a2, a3);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XCreateGC\n");
  return r;
}

Pixmap  TSXCreatePixmap(Display* a0, Drawable a1, unsigned int a2, unsigned int a3, unsigned int a4)
{
  Pixmap  r;
  dprintf_x11(stddeb, "Call XCreatePixmap\n");
  X11_LOCK();
  r = XCreatePixmap(a0, a1, a2, a3, a4);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XCreatePixmap\n");
  return r;
}

Pixmap  TSXCreateBitmapFromData(Display* a0, Drawable a1, const  char* a2, unsigned int a3, unsigned int a4)
{
  Pixmap  r;
  dprintf_x11(stddeb, "Call XCreateBitmapFromData\n");
  X11_LOCK();
  r = XCreateBitmapFromData(a0, a1, a2, a3, a4);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XCreateBitmapFromData\n");
  return r;
}

Window  TSXGetSelectionOwner(Display* a0, Atom a1)
{
  Window  r;
  dprintf_x11(stddeb, "Call XGetSelectionOwner\n");
  X11_LOCK();
  r = XGetSelectionOwner(a0, a1);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XGetSelectionOwner\n");
  return r;
}

Window  TSXCreateWindow(Display* a0, Window a1, int a2, int a3, unsigned int a4, unsigned int a5, unsigned int a6, int a7, unsigned int a8, Visual* a9, unsigned long a10, XSetWindowAttributes* a11)
{
  Window  r;
  dprintf_x11(stddeb, "Call XCreateWindow\n");
  X11_LOCK();
  r = XCreateWindow(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XCreateWindow\n");
  return r;
}

char ** TSXListFonts(Display* a0, const  char* a1, int a2, int* a3)
{
  char ** r;
  dprintf_x11(stddeb, "Call XListFonts\n");
  X11_LOCK();
  r = XListFonts(a0, a1, a2, a3);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XListFonts\n");
  return r;
}

KeySym  TSXKeycodeToKeysym(Display* a0, unsigned int a1, int a2)
{
  KeySym  r;
  dprintf_x11(stddeb, "Call XKeycodeToKeysym\n");
  X11_LOCK();
  r = XKeycodeToKeysym(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XKeycodeToKeysym\n");
  return r;
}

KeySym  TSXLookupKeysym(XKeyEvent* a0, int a1)
{
  KeySym  r;
  dprintf_x11(stddeb, "Call XLookupKeysym\n");
  X11_LOCK();
  r = XLookupKeysym(a0, a1);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XLookupKeysym\n");
  return r;
}

KeySym * TSXGetKeyboardMapping(Display* a0, unsigned int a1, int a2, int* a3)
{
  KeySym * r;
  dprintf_x11(stddeb, "Call XGetKeyboardMapping\n");
  X11_LOCK();
  r = XGetKeyboardMapping(a0, a1, a2, a3);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XGetKeyboardMapping\n");
  return r;
}

char * TSXResourceManagerString(Display* a0)
{
  char * r;
  dprintf_x11(stddeb, "Call XResourceManagerString\n");
  X11_LOCK();
  r = XResourceManagerString(a0);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XResourceManagerString\n");
  return r;
}

int   TSXInitThreads(void)
{
  int   r;
  dprintf_x11(stddeb, "Call XInitThreads\n");
  X11_LOCK();
  r = XInitThreads();
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XInitThreads\n");
  return r;
}

int * TSXListDepths(Display* a0, int a1, int* a2)
{
  int * r;
  dprintf_x11(stddeb, "Call XListDepths\n");
  X11_LOCK();
  r = XListDepths(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XListDepths\n");
  return r;
}

int   TSXReconfigureWMWindow(Display* a0, Window a1, int a2, unsigned int a3, XWindowChanges* a4)
{
  int   r;
  dprintf_x11(stddeb, "Call XReconfigureWMWindow\n");
  X11_LOCK();
  r = XReconfigureWMWindow(a0, a1, a2, a3, a4);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XReconfigureWMWindow\n");
  return r;
}

int   TSXSetWMProtocols(Display* a0, Window a1, Atom* a2, int a3)
{
  int   r;
  dprintf_x11(stddeb, "Call XSetWMProtocols\n");
  X11_LOCK();
  r = XSetWMProtocols(a0, a1, a2, a3);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XSetWMProtocols\n");
  return r;
}

int  TSXSetTransientForHint(Display* a0, Window a1, Window a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XSetTransientForHint\n");
  X11_LOCK();
  r = XSetTransientForHint(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XSetTransientForHint\n");
  return r;
}

int  TSXActivateScreenSaver(Display* a0)
{
  int  r;
  dprintf_x11(stddeb, "Call XActivateScreenSaver\n");
  X11_LOCK();
  r = XActivateScreenSaver(a0);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XActivateScreenSaver\n");
  return r;
}

int   TSXAllocColor(Display* a0, Colormap a1, XColor* a2)
{
  int   r;
  dprintf_x11(stddeb, "Call XAllocColor\n");
  X11_LOCK();
  r = XAllocColor(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XAllocColor\n");
  return r;
}

int   TSXAllocColorCells(Display* a0, Colormap a1, int a2, unsigned long* a3, unsigned int a4, unsigned long* a5, unsigned int a6)
{
  int   r;
  dprintf_x11(stddeb, "Call XAllocColorCells\n");
  X11_LOCK();
  r = XAllocColorCells(a0, a1, a2, a3, a4, a5, a6);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XAllocColorCells\n");
  return r;
}

int  TSXBell(Display* a0, int a1)
{
  int  r;
  dprintf_x11(stddeb, "Call XBell\n");
  X11_LOCK();
  r = XBell(a0, a1);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XBell\n");
  return r;
}

int  TSXChangeGC(Display* a0, GC a1, unsigned long a2, XGCValues* a3)
{
  int  r;
  dprintf_x11(stddeb, "Call XChangeGC\n");
  X11_LOCK();
  r = XChangeGC(a0, a1, a2, a3);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XChangeGC\n");
  return r;
}

int  TSXChangeKeyboardControl(Display* a0, unsigned long a1, XKeyboardControl* a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XChangeKeyboardControl\n");
  X11_LOCK();
  r = XChangeKeyboardControl(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XChangeKeyboardControl\n");
  return r;
}

int  TSXChangeProperty(Display* a0, Window a1, Atom a2, Atom a3, int a4, int a5, const  unsigned char* a6, int a7)
{
  int  r;
  dprintf_x11(stddeb, "Call XChangeProperty\n");
  X11_LOCK();
  r = XChangeProperty(a0, a1, a2, a3, a4, a5, a6, a7);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XChangeProperty\n");
  return r;
}

int  TSXChangeWindowAttributes(Display* a0, Window a1, unsigned long a2, XSetWindowAttributes* a3)
{
  int  r;
  dprintf_x11(stddeb, "Call XChangeWindowAttributes\n");
  X11_LOCK();
  r = XChangeWindowAttributes(a0, a1, a2, a3);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XChangeWindowAttributes\n");
  return r;
}

int   TSXCheckTypedWindowEvent(Display* a0, Window a1, int a2, XEvent* a3)
{
  int   r;
  dprintf_x11(stddeb, "Call XCheckTypedWindowEvent\n");
  X11_LOCK();
  r = XCheckTypedWindowEvent(a0, a1, a2, a3);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XCheckTypedWindowEvent\n");
  return r;
}

int   TSXCheckWindowEvent(Display* a0, Window a1, long a2, XEvent* a3)
{
  int   r;
  dprintf_x11(stddeb, "Call XCheckWindowEvent\n");
  X11_LOCK();
  r = XCheckWindowEvent(a0, a1, a2, a3);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XCheckWindowEvent\n");
  return r;
}

int  TSXConvertSelection(Display* a0, Atom a1, Atom a2, Atom a3, Window a4, Time a5)
{
  int  r;
  dprintf_x11(stddeb, "Call XConvertSelection\n");
  X11_LOCK();
  r = XConvertSelection(a0, a1, a2, a3, a4, a5);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XConvertSelection\n");
  return r;
}

int  TSXCopyArea(Display* a0, Drawable a1, Drawable a2, GC a3, int a4, int a5, unsigned int a6, unsigned int a7, int a8, int a9)
{
  int  r;
  dprintf_x11(stddeb, "Call XCopyArea\n");
  X11_LOCK();
  r = XCopyArea(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XCopyArea\n");
  return r;
}

int  TSXCopyPlane(Display* a0, Drawable a1, Drawable a2, GC a3, int a4, int a5, unsigned int a6, unsigned int a7, int a8, int a9, unsigned long a10)
{
  int  r;
  dprintf_x11(stddeb, "Call XCopyPlane\n");
  X11_LOCK();
  r = XCopyPlane(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XCopyPlane\n");
  return r;
}

int  TSXDefineCursor(Display* a0, Window a1, Cursor a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XDefineCursor\n");
  X11_LOCK();
  r = XDefineCursor(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XDefineCursor\n");
  return r;
}

int  TSXDestroyWindow(Display* a0, Window a1)
{
  int  r;
  dprintf_x11(stddeb, "Call XDestroyWindow\n");
  X11_LOCK();
  r = XDestroyWindow(a0, a1);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XDestroyWindow\n");
  return r;
}

int  TSXDisplayKeycodes(Display* a0, int* a1, int* a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XDisplayKeycodes\n");
  X11_LOCK();
  r = XDisplayKeycodes(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XDisplayKeycodes\n");
  return r;
}

int  TSXDrawArc(Display* a0, Drawable a1, GC a2, int a3, int a4, unsigned int a5, unsigned int a6, int a7, int a8)
{
  int  r;
  dprintf_x11(stddeb, "Call XDrawArc\n");
  X11_LOCK();
  r = XDrawArc(a0, a1, a2, a3, a4, a5, a6, a7, a8);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XDrawArc\n");
  return r;
}

int  TSXDrawLine(Display* a0, Drawable a1, GC a2, int a3, int a4, int a5, int a6)
{
  int  r;
  dprintf_x11(stddeb, "Call XDrawLine\n");
  X11_LOCK();
  r = XDrawLine(a0, a1, a2, a3, a4, a5, a6);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XDrawLine\n");
  return r;
}

int  TSXDrawLines(Display* a0, Drawable a1, GC a2, XPoint* a3, int a4, int a5)
{
  int  r;
  dprintf_x11(stddeb, "Call XDrawLines\n");
  X11_LOCK();
  r = XDrawLines(a0, a1, a2, a3, a4, a5);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XDrawLines\n");
  return r;
}

int  TSXDrawPoint(Display* a0, Drawable a1, GC a2, int a3, int a4)
{
  int  r;
  dprintf_x11(stddeb, "Call XDrawPoint\n");
  X11_LOCK();
  r = XDrawPoint(a0, a1, a2, a3, a4);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XDrawPoint\n");
  return r;
}

int  TSXDrawRectangle(Display* a0, Drawable a1, GC a2, int a3, int a4, unsigned int a5, unsigned int a6)
{
  int  r;
  dprintf_x11(stddeb, "Call XDrawRectangle\n");
  X11_LOCK();
  r = XDrawRectangle(a0, a1, a2, a3, a4, a5, a6);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XDrawRectangle\n");
  return r;
}

int  TSXDrawSegments(Display* a0, Drawable a1, GC a2, XSegment* a3, int a4)
{
  int  r;
  dprintf_x11(stddeb, "Call XDrawSegments\n");
  X11_LOCK();
  r = XDrawSegments(a0, a1, a2, a3, a4);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XDrawSegments\n");
  return r;
}

int  TSXDrawString(Display* a0, Drawable a1, GC a2, int a3, int a4, const  char* a5, int a6)
{
  int  r;
  dprintf_x11(stddeb, "Call XDrawString\n");
  X11_LOCK();
  r = XDrawString(a0, a1, a2, a3, a4, a5, a6);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XDrawString\n");
  return r;
}

int  TSXDrawText(Display* a0, Drawable a1, GC a2, int a3, int a4, XTextItem* a5, int a6)
{
  int  r;
  dprintf_x11(stddeb, "Call XDrawText\n");
  X11_LOCK();
  r = XDrawText(a0, a1, a2, a3, a4, a5, a6);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XDrawText\n");
  return r;
}

int  TSXFillArc(Display* a0, Drawable a1, GC a2, int a3, int a4, unsigned int a5, unsigned int a6, int a7, int a8)
{
  int  r;
  dprintf_x11(stddeb, "Call XFillArc\n");
  X11_LOCK();
  r = XFillArc(a0, a1, a2, a3, a4, a5, a6, a7, a8);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XFillArc\n");
  return r;
}

int  TSXFillPolygon(Display* a0, Drawable a1, GC a2, XPoint* a3, int a4, int a5, int a6)
{
  int  r;
  dprintf_x11(stddeb, "Call XFillPolygon\n");
  X11_LOCK();
  r = XFillPolygon(a0, a1, a2, a3, a4, a5, a6);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XFillPolygon\n");
  return r;
}

int  TSXFillRectangle(Display* a0, Drawable a1, GC a2, int a3, int a4, unsigned int a5, unsigned int a6)
{
  int  r;
  dprintf_x11(stddeb, "Call XFillRectangle\n");
  X11_LOCK();
  r = XFillRectangle(a0, a1, a2, a3, a4, a5, a6);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XFillRectangle\n");
  return r;
}

int  TSXFlush(Display* a0)
{
  int  r;
  dprintf_x11(stddeb, "Call XFlush\n");
  X11_LOCK();
  r = XFlush(a0);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XFlush\n");
  return r;
}

int  TSXFree(void* a0)
{
  int  r;
  dprintf_x11(stddeb, "Call XFree\n");
  X11_LOCK();
  r = XFree(a0);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XFree\n");
  return r;
}

int  TSXFreeColors(Display* a0, Colormap a1, unsigned long* a2, int a3, unsigned long a4)
{
  int  r;
  dprintf_x11(stddeb, "Call XFreeColors\n");
  X11_LOCK();
  r = XFreeColors(a0, a1, a2, a3, a4);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XFreeColors\n");
  return r;
}

int  TSXFreeCursor(Display* a0, Cursor a1)
{
  int  r;
  dprintf_x11(stddeb, "Call XFreeCursor\n");
  X11_LOCK();
  r = XFreeCursor(a0, a1);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XFreeCursor\n");
  return r;
}

int  TSXFreeFont(Display* a0, XFontStruct* a1)
{
  int  r;
  dprintf_x11(stddeb, "Call XFreeFont\n");
  X11_LOCK();
  r = XFreeFont(a0, a1);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XFreeFont\n");
  return r;
}

int  TSXFreeFontNames(char** a0)
{
  int  r;
  dprintf_x11(stddeb, "Call XFreeFontNames\n");
  X11_LOCK();
  r = XFreeFontNames(a0);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XFreeFontNames\n");
  return r;
}

int  TSXFreeGC(Display* a0, GC a1)
{
  int  r;
  dprintf_x11(stddeb, "Call XFreeGC\n");
  X11_LOCK();
  r = XFreeGC(a0, a1);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XFreeGC\n");
  return r;
}

int  TSXFreeModifiermap(XModifierKeymap* a0)
{
  int  r;
  dprintf_x11(stddeb, "Call XFreeModifiermap\n");
  X11_LOCK();
  r = XFreeModifiermap(a0);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XFreeModifiermap\n");
  return r;
}

int  TSXFreePixmap(Display* a0, Pixmap a1)
{
  int  r;
  dprintf_x11(stddeb, "Call XFreePixmap\n");
  X11_LOCK();
  r = XFreePixmap(a0, a1);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XFreePixmap\n");
  return r;
}

int   TSXGetFontProperty(XFontStruct* a0, Atom a1, unsigned long* a2)
{
  int   r;
  dprintf_x11(stddeb, "Call XGetFontProperty\n");
  X11_LOCK();
  r = XGetFontProperty(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XGetFontProperty\n");
  return r;
}

int   TSXGetGeometry(Display* a0, Drawable a1, Window* a2, int* a3, int* a4, unsigned int* a5, unsigned int* a6, unsigned int* a7, unsigned int* a8)
{
  int   r;
  dprintf_x11(stddeb, "Call XGetGeometry\n");
  X11_LOCK();
  r = XGetGeometry(a0, a1, a2, a3, a4, a5, a6, a7, a8);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XGetGeometry\n");
  return r;
}

int  TSXGetInputFocus(Display* a0, Window* a1, int* a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XGetInputFocus\n");
  X11_LOCK();
  r = XGetInputFocus(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XGetInputFocus\n");
  return r;
}

int  TSXGetKeyboardControl(Display* a0, XKeyboardState* a1)
{
  int  r;
  dprintf_x11(stddeb, "Call XGetKeyboardControl\n");
  X11_LOCK();
  r = XGetKeyboardControl(a0, a1);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XGetKeyboardControl\n");
  return r;
}

int  TSXGetScreenSaver(Display* a0, int* a1, int* a2, int* a3, int* a4)
{
  int  r;
  dprintf_x11(stddeb, "Call XGetScreenSaver\n");
  X11_LOCK();
  r = XGetScreenSaver(a0, a1, a2, a3, a4);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XGetScreenSaver\n");
  return r;
}

int  TSXGetWindowProperty(Display* a0, Window a1, Atom a2, long a3, long a4, int a5, Atom a6, Atom* a7, int* a8, unsigned long* a9, unsigned long* a10, unsigned char** a11)
{
  int  r;
  dprintf_x11(stddeb, "Call XGetWindowProperty\n");
  X11_LOCK();
  r = XGetWindowProperty(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XGetWindowProperty\n");
  return r;
}

int   TSXGetWindowAttributes(Display* a0, Window a1, XWindowAttributes* a2)
{
  int   r;
  dprintf_x11(stddeb, "Call XGetWindowAttributes\n");
  X11_LOCK();
  r = XGetWindowAttributes(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XGetWindowAttributes\n");
  return r;
}

int  TSXGrabPointer(Display* a0, Window a1, int a2, unsigned int a3, int a4, int a5, Window a6, Cursor a7, Time a8)
{
  int  r;
  dprintf_x11(stddeb, "Call XGrabPointer\n");
  X11_LOCK();
  r = XGrabPointer(a0, a1, a2, a3, a4, a5, a6, a7, a8);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XGrabPointer\n");
  return r;
}

int  TSXGrabServer(Display* a0)
{
  int  r;
  dprintf_x11(stddeb, "Call XGrabServer\n");
  X11_LOCK();
  r = XGrabServer(a0);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XGrabServer\n");
  return r;
}

int  TSXInstallColormap(Display* a0, Colormap a1)
{
  int  r;
  dprintf_x11(stddeb, "Call XInstallColormap\n");
  X11_LOCK();
  r = XInstallColormap(a0, a1);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XInstallColormap\n");
  return r;
}

KeyCode  TSXKeysymToKeycode(Display* a0, KeySym a1)
{
  KeyCode  r;
  dprintf_x11(stddeb, "Call XKeysymToKeycode\n");
  X11_LOCK();
  r = XKeysymToKeycode(a0, a1);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XKeysymToKeycode\n");
  return r;
}

int  TSXMapWindow(Display* a0, Window a1)
{
  int  r;
  dprintf_x11(stddeb, "Call XMapWindow\n");
  X11_LOCK();
  r = XMapWindow(a0, a1);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XMapWindow\n");
  return r;
}

int  TSXNextEvent(Display* a0, XEvent* a1)
{
  int  r;
  dprintf_x11(stddeb, "Call XNextEvent\n");
  X11_LOCK();
  r = XNextEvent(a0, a1);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XNextEvent\n");
  return r;
}

int  TSXParseGeometry(const  char* a0, int* a1, int* a2, unsigned int* a3, unsigned int* a4)
{
  int  r;
  dprintf_x11(stddeb, "Call XParseGeometry\n");
  X11_LOCK();
  r = XParseGeometry(a0, a1, a2, a3, a4);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XParseGeometry\n");
  return r;
}

int  TSXPending(Display* a0)
{
  int  r;
  dprintf_x11(stddeb, "Call XPending\n");
  X11_LOCK();
  r = XPending(a0);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XPending\n");
  return r;
}

int  TSXPutBackEvent(Display* a0, XEvent* a1)
{
  int  r;
  dprintf_x11(stddeb, "Call XPutBackEvent\n");
  X11_LOCK();
  r = XPutBackEvent(a0, a1);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XPutBackEvent\n");
  return r;
}

int  TSXPutImage(Display* a0, Drawable a1, GC a2, XImage* a3, int a4, int a5, int a6, int a7, unsigned int a8, unsigned int a9)
{
  int  r;
  dprintf_x11(stddeb, "Call XPutImage\n");
  X11_LOCK();
  r = XPutImage(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XPutImage\n");
  return r;
}

int  TSXQueryColor(Display* a0, Colormap a1, XColor* a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XQueryColor\n");
  X11_LOCK();
  r = XQueryColor(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XQueryColor\n");
  return r;
}

int   TSXQueryPointer(Display* a0, Window a1, Window* a2, Window* a3, int* a4, int* a5, int* a6, int* a7, unsigned int* a8)
{
  int   r;
  dprintf_x11(stddeb, "Call XQueryPointer\n");
  X11_LOCK();
  r = XQueryPointer(a0, a1, a2, a3, a4, a5, a6, a7, a8);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XQueryPointer\n");
  return r;
}

int   TSXQueryTree(Display* a0, Window a1, Window* a2, Window* a3, Window** a4, unsigned int* a5)
{
  int   r;
  dprintf_x11(stddeb, "Call XQueryTree\n");
  X11_LOCK();
  r = XQueryTree(a0, a1, a2, a3, a4, a5);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XQueryTree\n");
  return r;
}

int  TSXResetScreenSaver(Display* a0)
{
  int  r;
  dprintf_x11(stddeb, "Call XResetScreenSaver\n");
  X11_LOCK();
  r = XResetScreenSaver(a0);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XResetScreenSaver\n");
  return r;
}

int  TSXRestackWindows(Display* a0, Window* a1, int a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XRestackWindows\n");
  X11_LOCK();
  r = XRestackWindows(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XRestackWindows\n");
  return r;
}

int   TSXSendEvent(Display* a0, Window a1, int a2, long a3, XEvent* a4)
{
  int   r;
  dprintf_x11(stddeb, "Call XSendEvent\n");
  X11_LOCK();
  r = XSendEvent(a0, a1, a2, a3, a4);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XSendEvent\n");
  return r;
}

int  TSXSetArcMode(Display* a0, GC a1, int a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XSetArcMode\n");
  X11_LOCK();
  r = XSetArcMode(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XSetArcMode\n");
  return r;
}

int  TSXSetBackground(Display* a0, GC a1, unsigned long a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XSetBackground\n");
  X11_LOCK();
  r = XSetBackground(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XSetBackground\n");
  return r;
}

int  TSXSetClipMask(Display* a0, GC a1, Pixmap a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XSetClipMask\n");
  X11_LOCK();
  r = XSetClipMask(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XSetClipMask\n");
  return r;
}

int  TSXSetClipOrigin(Display* a0, GC a1, int a2, int a3)
{
  int  r;
  dprintf_x11(stddeb, "Call XSetClipOrigin\n");
  X11_LOCK();
  r = XSetClipOrigin(a0, a1, a2, a3);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XSetClipOrigin\n");
  return r;
}

int  TSXSetClipRectangles(Display* a0, GC a1, int a2, int a3, XRectangle* a4, int a5, int a6)
{
  int  r;
  dprintf_x11(stddeb, "Call XSetClipRectangles\n");
  X11_LOCK();
  r = XSetClipRectangles(a0, a1, a2, a3, a4, a5, a6);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XSetClipRectangles\n");
  return r;
}

int  TSXSetDashes(Display* a0, GC a1, int a2, const  char* a3, int a4)
{
  int  r;
  dprintf_x11(stddeb, "Call XSetDashes\n");
  X11_LOCK();
  r = XSetDashes(a0, a1, a2, a3, a4);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XSetDashes\n");
  return r;
}

int  TSXSetFillStyle(Display* a0, GC a1, int a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XSetFillStyle\n");
  X11_LOCK();
  r = XSetFillStyle(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XSetFillStyle\n");
  return r;
}

int  TSXSetForeground(Display* a0, GC a1, unsigned long a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XSetForeground\n");
  X11_LOCK();
  r = XSetForeground(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XSetForeground\n");
  return r;
}

int  TSXSetFunction(Display* a0, GC a1, int a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XSetFunction\n");
  X11_LOCK();
  r = XSetFunction(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XSetFunction\n");
  return r;
}

int  TSXSetGraphicsExposures(Display* a0, GC a1, int a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XSetGraphicsExposures\n");
  X11_LOCK();
  r = XSetGraphicsExposures(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XSetGraphicsExposures\n");
  return r;
}

int  TSXSetIconName(Display* a0, Window a1, const  char* a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XSetIconName\n");
  X11_LOCK();
  r = XSetIconName(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XSetIconName\n");
  return r;
}

int  TSXSetInputFocus(Display* a0, Window a1, int a2, Time a3)
{
  int  r;
  dprintf_x11(stddeb, "Call XSetInputFocus\n");
  X11_LOCK();
  r = XSetInputFocus(a0, a1, a2, a3);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XSetInputFocus\n");
  return r;
}

int  TSXSetLineAttributes(Display* a0, GC a1, unsigned int a2, int a3, int a4, int a5)
{
  int  r;
  dprintf_x11(stddeb, "Call XSetLineAttributes\n");
  X11_LOCK();
  r = XSetLineAttributes(a0, a1, a2, a3, a4, a5);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XSetLineAttributes\n");
  return r;
}

int  TSXSetScreenSaver(Display* a0, int a1, int a2, int a3, int a4)
{
  int  r;
  dprintf_x11(stddeb, "Call XSetScreenSaver\n");
  X11_LOCK();
  r = XSetScreenSaver(a0, a1, a2, a3, a4);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XSetScreenSaver\n");
  return r;
}

int  TSXSetSelectionOwner(Display* a0, Atom a1, Window a2, Time a3)
{
  int  r;
  dprintf_x11(stddeb, "Call XSetSelectionOwner\n");
  X11_LOCK();
  r = XSetSelectionOwner(a0, a1, a2, a3);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XSetSelectionOwner\n");
  return r;
}

int  TSXSetSubwindowMode(Display* a0, GC a1, int a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XSetSubwindowMode\n");
  X11_LOCK();
  r = XSetSubwindowMode(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XSetSubwindowMode\n");
  return r;
}

int  TSXStoreColor(Display* a0, Colormap a1, XColor* a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XStoreColor\n");
  X11_LOCK();
  r = XStoreColor(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XStoreColor\n");
  return r;
}

int  TSXStoreName(Display* a0, Window a1, const  char* a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XStoreName\n");
  X11_LOCK();
  r = XStoreName(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XStoreName\n");
  return r;
}

int  TSXSync(Display* a0, int a1)
{
  int  r;
  dprintf_x11(stddeb, "Call XSync\n");
  X11_LOCK();
  r = XSync(a0, a1);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XSync\n");
  return r;
}

int  TSXTextExtents(XFontStruct* a0, const  char* a1, int a2, int* a3, int* a4, int* a5, XCharStruct* a6)
{
  int  r;
  dprintf_x11(stddeb, "Call XTextExtents\n");
  X11_LOCK();
  r = XTextExtents(a0, a1, a2, a3, a4, a5, a6);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XTextExtents\n");
  return r;
}

int  TSXTextWidth(XFontStruct* a0, const  char* a1, int a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XTextWidth\n");
  X11_LOCK();
  r = XTextWidth(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XTextWidth\n");
  return r;
}

int  TSXUngrabPointer(Display* a0, Time a1)
{
  int  r;
  dprintf_x11(stddeb, "Call XUngrabPointer\n");
  X11_LOCK();
  r = XUngrabPointer(a0, a1);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XUngrabPointer\n");
  return r;
}

int  TSXUngrabServer(Display* a0)
{
  int  r;
  dprintf_x11(stddeb, "Call XUngrabServer\n");
  X11_LOCK();
  r = XUngrabServer(a0);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XUngrabServer\n");
  return r;
}

int  TSXUninstallColormap(Display* a0, Colormap a1)
{
  int  r;
  dprintf_x11(stddeb, "Call XUninstallColormap\n");
  X11_LOCK();
  r = XUninstallColormap(a0, a1);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XUninstallColormap\n");
  return r;
}

int  TSXUnmapWindow(Display* a0, Window a1)
{
  int  r;
  dprintf_x11(stddeb, "Call XUnmapWindow\n");
  X11_LOCK();
  r = XUnmapWindow(a0, a1);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XUnmapWindow\n");
  return r;
}

int  TSXWarpPointer(Display* a0, Window a1, Window a2, int a3, int a4, unsigned int a5, unsigned int a6, int a7, int a8)
{
  int  r;
  dprintf_x11(stddeb, "Call XWarpPointer\n");
  X11_LOCK();
  r = XWarpPointer(a0, a1, a2, a3, a4, a5, a6, a7, a8);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XWarpPointer\n");
  return r;
}

int (*TSXSynchronize(Display *a0, Bool a1))(Display *)
{
  int (*r)(Display *);
  dprintf_x11(stddeb, "Call XSynchronize\n");
  X11_LOCK();
  r = XSynchronize(a0, a1);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XSynchronize\n");
  return r;
}

extern void _XInitImageFuncPtrs(XImage *);

void TS_XInitImageFuncPtrs(XImage *a0)
{
  dprintf_x11(stddeb, "Call _XInitImageFuncPtrs\n");
  X11_LOCK();
  _XInitImageFuncPtrs(a0);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret _XInitImageFuncPtrs\n");
}
