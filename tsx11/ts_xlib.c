/*
 * Thread safe wrappers around Xlib calls.
 * This file was generated automatically by tools/make_X11wrappers
 * DO NOT EDIT!
 */

#include "config.h"

#ifndef X_DISPLAY_MISSING


#include <X11/Xlib.h>
#include "debug.h"
#include "x11drv.h"

XFontStruct * TSXLoadQueryFont(Display* a0, const  char* a1)
{
  XFontStruct * r;
  TRACE(x11, "Call XLoadQueryFont\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XLoadQueryFont(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XLoadQueryFont\n");
  return r;
}

XModifierKeymap	* TSXGetModifierMapping(Display* a0)
{
  XModifierKeymap	* r;
  TRACE(x11, "Call XGetModifierMapping\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XGetModifierMapping(a0);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XGetModifierMapping\n");
  return r;
}

XImage * TSXCreateImage(Display* a0, Visual* a1, unsigned int a2, int a3, int a4, char* a5, unsigned int a6, unsigned int a7, int a8, int a9)
{
  XImage * r;
  TRACE(x11, "Call XCreateImage\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XCreateImage(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XCreateImage\n");
  return r;
}

XImage * TSXGetImage(Display* a0, Drawable a1, int a2, int a3, unsigned int a4, unsigned int a5, unsigned long a6, int a7)
{
  XImage * r;
  TRACE(x11, "Call XGetImage\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XGetImage(a0, a1, a2, a3, a4, a5, a6, a7);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XGetImage\n");
  return r;
}

Display * TSXOpenDisplay(const  char* a0)
{
  Display * r;
  TRACE(x11, "Call XOpenDisplay\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XOpenDisplay(a0);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XOpenDisplay\n");
  return r;
}

void  TSXrmInitialize(void)
{
  TRACE(x11, "Call XrmInitialize\n");
  EnterCriticalSection( &X11DRV_CritSection );
  XrmInitialize();
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XrmInitialize\n");
}

char * TSXGetAtomName(Display* a0, Atom a1)
{
  char * r;
  TRACE(x11, "Call XGetAtomName\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XGetAtomName(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XGetAtomName\n");
  return r;
}

char * TSXKeysymToString(KeySym a0)
{
  char * r;
  TRACE(x11, "Call XKeysymToString\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XKeysymToString(a0);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XKeysymToString\n");
  return r;
}

Atom  TSXInternAtom(Display* a0, const  char* a1, int a2)
{
  Atom  r;
  TRACE(x11, "Call XInternAtom\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XInternAtom(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XInternAtom\n");
  return r;
}

Colormap  TSXCopyColormapAndFree(Display* a0, Colormap a1)
{
  Colormap  r;
  TRACE(x11, "Call XCopyColormapAndFree\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XCopyColormapAndFree(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XCopyColormapAndFree\n");
  return r;
}

Colormap  TSXCreateColormap(Display* a0, Window a1, Visual* a2, int a3)
{
  Colormap  r;
  TRACE(x11, "Call XCreateColormap\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XCreateColormap(a0, a1, a2, a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XCreateColormap\n");
  return r;
}

Cursor  TSXCreatePixmapCursor(Display* a0, Pixmap a1, Pixmap a2, XColor* a3, XColor* a4, unsigned int a5, unsigned int a6)
{
  Cursor  r;
  TRACE(x11, "Call XCreatePixmapCursor\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XCreatePixmapCursor(a0, a1, a2, a3, a4, a5, a6);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XCreatePixmapCursor\n");
  return r;
}

Cursor  TSXCreateFontCursor(Display* a0, unsigned int a1)
{
  Cursor  r;
  TRACE(x11, "Call XCreateFontCursor\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XCreateFontCursor(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XCreateFontCursor\n");
  return r;
}

GC  TSXCreateGC(Display* a0, Drawable a1, unsigned long a2, XGCValues* a3)
{
  GC  r;
  TRACE(x11, "Call XCreateGC\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XCreateGC(a0, a1, a2, a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XCreateGC\n");
  return r;
}

Pixmap  TSXCreatePixmap(Display* a0, Drawable a1, unsigned int a2, unsigned int a3, unsigned int a4)
{
  Pixmap  r;
  TRACE(x11, "Call XCreatePixmap\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XCreatePixmap(a0, a1, a2, a3, a4);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XCreatePixmap\n");
  return r;
}

Pixmap  TSXCreateBitmapFromData(Display* a0, Drawable a1, const  char* a2, unsigned int a3, unsigned int a4)
{
  Pixmap  r;
  TRACE(x11, "Call XCreateBitmapFromData\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XCreateBitmapFromData(a0, a1, a2, a3, a4);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XCreateBitmapFromData\n");
  return r;
}

Window  TSXGetSelectionOwner(Display* a0, Atom a1)
{
  Window  r;
  TRACE(x11, "Call XGetSelectionOwner\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XGetSelectionOwner(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XGetSelectionOwner\n");
  return r;
}

Window  TSXCreateWindow(Display* a0, Window a1, int a2, int a3, unsigned int a4, unsigned int a5, unsigned int a6, int a7, unsigned int a8, Visual* a9, unsigned long a10, XSetWindowAttributes* a11)
{
  Window  r;
  TRACE(x11, "Call XCreateWindow\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XCreateWindow(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XCreateWindow\n");
  return r;
}

char ** TSXListFonts(Display* a0, const  char* a1, int a2, int* a3)
{
  char ** r;
  TRACE(x11, "Call XListFonts\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XListFonts(a0, a1, a2, a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XListFonts\n");
  return r;
}

KeySym  TSXKeycodeToKeysym(Display* a0, unsigned int a1, int a2)
{
  KeySym  r;
  TRACE(x11, "Call XKeycodeToKeysym\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XKeycodeToKeysym(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XKeycodeToKeysym\n");
  return r;
}

KeySym  TSXLookupKeysym(XKeyEvent* a0, int a1)
{
  KeySym  r;
  TRACE(x11, "Call XLookupKeysym\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XLookupKeysym(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XLookupKeysym\n");
  return r;
}

KeySym * TSXGetKeyboardMapping(Display* a0, unsigned int a1, int a2, int* a3)
{
  KeySym * r;
  TRACE(x11, "Call XGetKeyboardMapping\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XGetKeyboardMapping(a0, a1, a2, a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XGetKeyboardMapping\n");
  return r;
}

char * TSXResourceManagerString(Display* a0)
{
  char * r;
  TRACE(x11, "Call XResourceManagerString\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XResourceManagerString(a0);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XResourceManagerString\n");
  return r;
}

int   TSXInitThreads(void)
{
  int   r;
  TRACE(x11, "Call XInitThreads\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XInitThreads();
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XInitThreads\n");
  return r;
}

int * TSXListDepths(Display* a0, int a1, int* a2)
{
  int * r;
  TRACE(x11, "Call XListDepths\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XListDepths(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XListDepths\n");
  return r;
}

int   TSXReconfigureWMWindow(Display* a0, Window a1, int a2, unsigned int a3, XWindowChanges* a4)
{
  int   r;
  TRACE(x11, "Call XReconfigureWMWindow\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XReconfigureWMWindow(a0, a1, a2, a3, a4);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XReconfigureWMWindow\n");
  return r;
}

int   TSXSetWMProtocols(Display* a0, Window a1, Atom* a2, int a3)
{
  int   r;
  TRACE(x11, "Call XSetWMProtocols\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSetWMProtocols(a0, a1, a2, a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XSetWMProtocols\n");
  return r;
}

int   TSXIconifyWindow(Display* a0, Window a1, int a2)
{
  int   r;
  TRACE(x11, "Call XIconifyWindow\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XIconifyWindow(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XIconifyWindow\n");
  return r;
}

int  TSXSetTransientForHint(Display* a0, Window a1, Window a2)
{
  int  r;
  TRACE(x11, "Call XSetTransientForHint\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSetTransientForHint(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XSetTransientForHint\n");
  return r;
}

int  TSXActivateScreenSaver(Display* a0)
{
  int  r;
  TRACE(x11, "Call XActivateScreenSaver\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XActivateScreenSaver(a0);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XActivateScreenSaver\n");
  return r;
}

int   TSXAllocColor(Display* a0, Colormap a1, XColor* a2)
{
  int   r;
  TRACE(x11, "Call XAllocColor\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XAllocColor(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XAllocColor\n");
  return r;
}

int   TSXAllocColorCells(Display* a0, Colormap a1, int a2, unsigned long* a3, unsigned int a4, unsigned long* a5, unsigned int a6)
{
  int   r;
  TRACE(x11, "Call XAllocColorCells\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XAllocColorCells(a0, a1, a2, a3, a4, a5, a6);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XAllocColorCells\n");
  return r;
}

int  TSXBell(Display* a0, int a1)
{
  int  r;
  TRACE(x11, "Call XBell\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XBell(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XBell\n");
  return r;
}

int  TSXChangeGC(Display* a0, GC a1, unsigned long a2, XGCValues* a3)
{
  int  r;
  TRACE(x11, "Call XChangeGC\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XChangeGC(a0, a1, a2, a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XChangeGC\n");
  return r;
}

int  TSXChangeKeyboardControl(Display* a0, unsigned long a1, XKeyboardControl* a2)
{
  int  r;
  TRACE(x11, "Call XChangeKeyboardControl\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XChangeKeyboardControl(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XChangeKeyboardControl\n");
  return r;
}

int  TSXChangeProperty(Display* a0, Window a1, Atom a2, Atom a3, int a4, int a5, const  unsigned char* a6, int a7)
{
  int  r;
  TRACE(x11, "Call XChangeProperty\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XChangeProperty(a0, a1, a2, a3, a4, a5, a6, a7);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XChangeProperty\n");
  return r;
}

int  TSXChangeWindowAttributes(Display* a0, Window a1, unsigned long a2, XSetWindowAttributes* a3)
{
  int  r;
  TRACE(x11, "Call XChangeWindowAttributes\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XChangeWindowAttributes(a0, a1, a2, a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XChangeWindowAttributes\n");
  return r;
}

int   TSXCheckTypedWindowEvent(Display* a0, Window a1, int a2, XEvent* a3)
{
  int   r;
  TRACE(x11, "Call XCheckTypedWindowEvent\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XCheckTypedWindowEvent(a0, a1, a2, a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XCheckTypedWindowEvent\n");
  return r;
}

int   TSXCheckWindowEvent(Display* a0, Window a1, long a2, XEvent* a3)
{
  int   r;
  TRACE(x11, "Call XCheckWindowEvent\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XCheckWindowEvent(a0, a1, a2, a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XCheckWindowEvent\n");
  return r;
}

int  TSXConvertSelection(Display* a0, Atom a1, Atom a2, Atom a3, Window a4, Time a5)
{
  int  r;
  TRACE(x11, "Call XConvertSelection\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XConvertSelection(a0, a1, a2, a3, a4, a5);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XConvertSelection\n");
  return r;
}

int  TSXCopyArea(Display* a0, Drawable a1, Drawable a2, GC a3, int a4, int a5, unsigned int a6, unsigned int a7, int a8, int a9)
{
  int  r;
  TRACE(x11, "Call XCopyArea\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XCopyArea(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XCopyArea\n");
  return r;
}

int  TSXCopyPlane(Display* a0, Drawable a1, Drawable a2, GC a3, int a4, int a5, unsigned int a6, unsigned int a7, int a8, int a9, unsigned long a10)
{
  int  r;
  TRACE(x11, "Call XCopyPlane\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XCopyPlane(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XCopyPlane\n");
  return r;
}

int  TSXDefineCursor(Display* a0, Window a1, Cursor a2)
{
  int  r;
  TRACE(x11, "Call XDefineCursor\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XDefineCursor(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XDefineCursor\n");
  return r;
}

int  TSXDestroyWindow(Display* a0, Window a1)
{
  int  r;
  TRACE(x11, "Call XDestroyWindow\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XDestroyWindow(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XDestroyWindow\n");
  return r;
}

int  TSXDisplayKeycodes(Display* a0, int* a1, int* a2)
{
  int  r;
  TRACE(x11, "Call XDisplayKeycodes\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XDisplayKeycodes(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XDisplayKeycodes\n");
  return r;
}

int  TSXDrawArc(Display* a0, Drawable a1, GC a2, int a3, int a4, unsigned int a5, unsigned int a6, int a7, int a8)
{
  int  r;
  TRACE(x11, "Call XDrawArc\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XDrawArc(a0, a1, a2, a3, a4, a5, a6, a7, a8);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XDrawArc\n");
  return r;
}

int  TSXDrawLine(Display* a0, Drawable a1, GC a2, int a3, int a4, int a5, int a6)
{
  int  r;
  TRACE(x11, "Call XDrawLine\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XDrawLine(a0, a1, a2, a3, a4, a5, a6);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XDrawLine\n");
  return r;
}

int  TSXDrawLines(Display* a0, Drawable a1, GC a2, XPoint* a3, int a4, int a5)
{
  int  r;
  TRACE(x11, "Call XDrawLines\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XDrawLines(a0, a1, a2, a3, a4, a5);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XDrawLines\n");
  return r;
}

int  TSXDrawPoint(Display* a0, Drawable a1, GC a2, int a3, int a4)
{
  int  r;
  TRACE(x11, "Call XDrawPoint\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XDrawPoint(a0, a1, a2, a3, a4);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XDrawPoint\n");
  return r;
}

int  TSXDrawRectangle(Display* a0, Drawable a1, GC a2, int a3, int a4, unsigned int a5, unsigned int a6)
{
  int  r;
  TRACE(x11, "Call XDrawRectangle\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XDrawRectangle(a0, a1, a2, a3, a4, a5, a6);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XDrawRectangle\n");
  return r;
}

int  TSXDrawSegments(Display* a0, Drawable a1, GC a2, XSegment* a3, int a4)
{
  int  r;
  TRACE(x11, "Call XDrawSegments\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XDrawSegments(a0, a1, a2, a3, a4);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XDrawSegments\n");
  return r;
}

int  TSXDrawString(Display* a0, Drawable a1, GC a2, int a3, int a4, const  char* a5, int a6)
{
  int  r;
  TRACE(x11, "Call XDrawString\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XDrawString(a0, a1, a2, a3, a4, a5, a6);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XDrawString\n");
  return r;
}

int  TSXDrawText(Display* a0, Drawable a1, GC a2, int a3, int a4, XTextItem* a5, int a6)
{
  int  r;
  TRACE(x11, "Call XDrawText\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XDrawText(a0, a1, a2, a3, a4, a5, a6);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XDrawText\n");
  return r;
}

int  TSXFillArc(Display* a0, Drawable a1, GC a2, int a3, int a4, unsigned int a5, unsigned int a6, int a7, int a8)
{
  int  r;
  TRACE(x11, "Call XFillArc\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XFillArc(a0, a1, a2, a3, a4, a5, a6, a7, a8);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XFillArc\n");
  return r;
}

int  TSXFillPolygon(Display* a0, Drawable a1, GC a2, XPoint* a3, int a4, int a5, int a6)
{
  int  r;
  TRACE(x11, "Call XFillPolygon\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XFillPolygon(a0, a1, a2, a3, a4, a5, a6);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XFillPolygon\n");
  return r;
}

int  TSXFillRectangle(Display* a0, Drawable a1, GC a2, int a3, int a4, unsigned int a5, unsigned int a6)
{
  int  r;
  TRACE(x11, "Call XFillRectangle\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XFillRectangle(a0, a1, a2, a3, a4, a5, a6);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XFillRectangle\n");
  return r;
}

int  TSXFlush(Display* a0)
{
  int  r;
  TRACE(x11, "Call XFlush\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XFlush(a0);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XFlush\n");
  return r;
}

int  TSXFree(void* a0)
{
  int  r;
  TRACE(x11, "Call XFree\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XFree(a0);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XFree\n");
  return r;
}

int  TSXFreeColormap(Display* a0, Colormap a1)
{
  int  r;
  TRACE(x11, "Call XFreeColormap\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XFreeColormap(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XFreeColormap\n");
  return r;
}

int  TSXFreeColors(Display* a0, Colormap a1, unsigned long* a2, int a3, unsigned long a4)
{
  int  r;
  TRACE(x11, "Call XFreeColors\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XFreeColors(a0, a1, a2, a3, a4);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XFreeColors\n");
  return r;
}

int  TSXFreeCursor(Display* a0, Cursor a1)
{
  int  r;
  TRACE(x11, "Call XFreeCursor\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XFreeCursor(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XFreeCursor\n");
  return r;
}

int  TSXFreeFont(Display* a0, XFontStruct* a1)
{
  int  r;
  TRACE(x11, "Call XFreeFont\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XFreeFont(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XFreeFont\n");
  return r;
}

int  TSXFreeFontNames(char** a0)
{
  int  r;
  TRACE(x11, "Call XFreeFontNames\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XFreeFontNames(a0);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XFreeFontNames\n");
  return r;
}

int  TSXFreeGC(Display* a0, GC a1)
{
  int  r;
  TRACE(x11, "Call XFreeGC\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XFreeGC(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XFreeGC\n");
  return r;
}

int  TSXFreeModifiermap(XModifierKeymap* a0)
{
  int  r;
  TRACE(x11, "Call XFreeModifiermap\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XFreeModifiermap(a0);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XFreeModifiermap\n");
  return r;
}

int  TSXFreePixmap(Display* a0, Pixmap a1)
{
  int  r;
  TRACE(x11, "Call XFreePixmap\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XFreePixmap(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XFreePixmap\n");
  return r;
}

int   TSXGetFontProperty(XFontStruct* a0, Atom a1, unsigned long* a2)
{
  int   r;
  TRACE(x11, "Call XGetFontProperty\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XGetFontProperty(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XGetFontProperty\n");
  return r;
}

int   TSXGetGeometry(Display* a0, Drawable a1, Window* a2, int* a3, int* a4, unsigned int* a5, unsigned int* a6, unsigned int* a7, unsigned int* a8)
{
  int   r;
  TRACE(x11, "Call XGetGeometry\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XGetGeometry(a0, a1, a2, a3, a4, a5, a6, a7, a8);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XGetGeometry\n");
  return r;
}

int  TSXGetInputFocus(Display* a0, Window* a1, int* a2)
{
  int  r;
  TRACE(x11, "Call XGetInputFocus\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XGetInputFocus(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XGetInputFocus\n");
  return r;
}

int  TSXGetKeyboardControl(Display* a0, XKeyboardState* a1)
{
  int  r;
  TRACE(x11, "Call XGetKeyboardControl\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XGetKeyboardControl(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XGetKeyboardControl\n");
  return r;
}

int  TSXGetScreenSaver(Display* a0, int* a1, int* a2, int* a3, int* a4)
{
  int  r;
  TRACE(x11, "Call XGetScreenSaver\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XGetScreenSaver(a0, a1, a2, a3, a4);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XGetScreenSaver\n");
  return r;
}

int  TSXGetWindowProperty(Display* a0, Window a1, Atom a2, long a3, long a4, int a5, Atom a6, Atom* a7, int* a8, unsigned long* a9, unsigned long* a10, unsigned char** a11)
{
  int  r;
  TRACE(x11, "Call XGetWindowProperty\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XGetWindowProperty(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XGetWindowProperty\n");
  return r;
}

int   TSXGetWindowAttributes(Display* a0, Window a1, XWindowAttributes* a2)
{
  int   r;
  TRACE(x11, "Call XGetWindowAttributes\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XGetWindowAttributes(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XGetWindowAttributes\n");
  return r;
}

int  TSXGrabServer(Display* a0)
{
  int  r;
  TRACE(x11, "Call XGrabServer\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XGrabServer(a0);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XGrabServer\n");
  return r;
}

int  TSXInstallColormap(Display* a0, Colormap a1)
{
  int  r;
  TRACE(x11, "Call XInstallColormap\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XInstallColormap(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XInstallColormap\n");
  return r;
}

KeyCode  TSXKeysymToKeycode(Display* a0, KeySym a1)
{
  KeyCode  r;
  TRACE(x11, "Call XKeysymToKeycode\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XKeysymToKeycode(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XKeysymToKeycode\n");
  return r;
}

int  TSXMapWindow(Display* a0, Window a1)
{
  int  r;
  TRACE(x11, "Call XMapWindow\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XMapWindow(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XMapWindow\n");
  return r;
}

int  TSXNextEvent(Display* a0, XEvent* a1)
{
  int  r;
  TRACE(x11, "Call XNextEvent\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XNextEvent(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XNextEvent\n");
  return r;
}

int  TSXParseGeometry(const  char* a0, int* a1, int* a2, unsigned int* a3, unsigned int* a4)
{
  int  r;
  TRACE(x11, "Call XParseGeometry\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XParseGeometry(a0, a1, a2, a3, a4);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XParseGeometry\n");
  return r;
}

int  TSXPending(Display* a0)
{
  int  r;
  TRACE(x11, "Call XPending\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XPending(a0);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XPending\n");
  return r;
}

int  TSXPutBackEvent(Display* a0, XEvent* a1)
{
  int  r;
  TRACE(x11, "Call XPutBackEvent\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XPutBackEvent(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XPutBackEvent\n");
  return r;
}

int  TSXPutImage(Display* a0, Drawable a1, GC a2, XImage* a3, int a4, int a5, int a6, int a7, unsigned int a8, unsigned int a9)
{
  int  r;
  TRACE(x11, "Call XPutImage\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XPutImage(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XPutImage\n");
  return r;
}

int  TSXQueryColor(Display* a0, Colormap a1, XColor* a2)
{
  int  r;
  TRACE(x11, "Call XQueryColor\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XQueryColor(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XQueryColor\n");
  return r;
}

int  TSXQueryKeymap(Display* a0, char* a1)
{
  int  r;
  TRACE(x11, "Call XQueryKeymap\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XQueryKeymap(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XQueryKeymap\n");
  return r;
}

int   TSXQueryPointer(Display* a0, Window a1, Window* a2, Window* a3, int* a4, int* a5, int* a6, int* a7, unsigned int* a8)
{
  int   r;
  TRACE(x11, "Call XQueryPointer\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XQueryPointer(a0, a1, a2, a3, a4, a5, a6, a7, a8);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XQueryPointer\n");
  return r;
}

int   TSXQueryTree(Display* a0, Window a1, Window* a2, Window* a3, Window** a4, unsigned int* a5)
{
  int   r;
  TRACE(x11, "Call XQueryTree\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XQueryTree(a0, a1, a2, a3, a4, a5);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XQueryTree\n");
  return r;
}

int  TSXResetScreenSaver(Display* a0)
{
  int  r;
  TRACE(x11, "Call XResetScreenSaver\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XResetScreenSaver(a0);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XResetScreenSaver\n");
  return r;
}

int  TSXRestackWindows(Display* a0, Window* a1, int a2)
{
  int  r;
  TRACE(x11, "Call XRestackWindows\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XRestackWindows(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XRestackWindows\n");
  return r;
}

int   TSXSendEvent(Display* a0, Window a1, int a2, long a3, XEvent* a4)
{
  int   r;
  TRACE(x11, "Call XSendEvent\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSendEvent(a0, a1, a2, a3, a4);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XSendEvent\n");
  return r;
}

int  TSXSetArcMode(Display* a0, GC a1, int a2)
{
  int  r;
  TRACE(x11, "Call XSetArcMode\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSetArcMode(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XSetArcMode\n");
  return r;
}

int  TSXSetBackground(Display* a0, GC a1, unsigned long a2)
{
  int  r;
  TRACE(x11, "Call XSetBackground\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSetBackground(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XSetBackground\n");
  return r;
}

int  TSXSetClipMask(Display* a0, GC a1, Pixmap a2)
{
  int  r;
  TRACE(x11, "Call XSetClipMask\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSetClipMask(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XSetClipMask\n");
  return r;
}

int  TSXSetClipOrigin(Display* a0, GC a1, int a2, int a3)
{
  int  r;
  TRACE(x11, "Call XSetClipOrigin\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSetClipOrigin(a0, a1, a2, a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XSetClipOrigin\n");
  return r;
}

int  TSXSetClipRectangles(Display* a0, GC a1, int a2, int a3, XRectangle* a4, int a5, int a6)
{
  int  r;
  TRACE(x11, "Call XSetClipRectangles\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSetClipRectangles(a0, a1, a2, a3, a4, a5, a6);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XSetClipRectangles\n");
  return r;
}

int  TSXSetDashes(Display* a0, GC a1, int a2, const  char* a3, int a4)
{
  int  r;
  TRACE(x11, "Call XSetDashes\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSetDashes(a0, a1, a2, a3, a4);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XSetDashes\n");
  return r;
}

int  TSXSetFillStyle(Display* a0, GC a1, int a2)
{
  int  r;
  TRACE(x11, "Call XSetFillStyle\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSetFillStyle(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XSetFillStyle\n");
  return r;
}

int  TSXSetForeground(Display* a0, GC a1, unsigned long a2)
{
  int  r;
  TRACE(x11, "Call XSetForeground\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSetForeground(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XSetForeground\n");
  return r;
}

int  TSXSetFunction(Display* a0, GC a1, int a2)
{
  int  r;
  TRACE(x11, "Call XSetFunction\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSetFunction(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XSetFunction\n");
  return r;
}

int  TSXSetGraphicsExposures(Display* a0, GC a1, int a2)
{
  int  r;
  TRACE(x11, "Call XSetGraphicsExposures\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSetGraphicsExposures(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XSetGraphicsExposures\n");
  return r;
}

int  TSXSetIconName(Display* a0, Window a1, const  char* a2)
{
  int  r;
  TRACE(x11, "Call XSetIconName\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSetIconName(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XSetIconName\n");
  return r;
}

int  TSXSetInputFocus(Display* a0, Window a1, int a2, Time a3)
{
  int  r;
  TRACE(x11, "Call XSetInputFocus\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSetInputFocus(a0, a1, a2, a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XSetInputFocus\n");
  return r;
}

int  TSXSetLineAttributes(Display* a0, GC a1, unsigned int a2, int a3, int a4, int a5)
{
  int  r;
  TRACE(x11, "Call XSetLineAttributes\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSetLineAttributes(a0, a1, a2, a3, a4, a5);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XSetLineAttributes\n");
  return r;
}

int  TSXSetScreenSaver(Display* a0, int a1, int a2, int a3, int a4)
{
  int  r;
  TRACE(x11, "Call XSetScreenSaver\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSetScreenSaver(a0, a1, a2, a3, a4);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XSetScreenSaver\n");
  return r;
}

int  TSXSetSelectionOwner(Display* a0, Atom a1, Window a2, Time a3)
{
  int  r;
  TRACE(x11, "Call XSetSelectionOwner\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSetSelectionOwner(a0, a1, a2, a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XSetSelectionOwner\n");
  return r;
}

int  TSXSetSubwindowMode(Display* a0, GC a1, int a2)
{
  int  r;
  TRACE(x11, "Call XSetSubwindowMode\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSetSubwindowMode(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XSetSubwindowMode\n");
  return r;
}

int  TSXSetWindowColormap(Display* a0, Window a1, Colormap a2)
{
  int  r;
  TRACE(x11, "Call XSetWindowColormap\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSetWindowColormap(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XSetWindowColormap\n");
  return r;
}

int  TSXStoreColor(Display* a0, Colormap a1, XColor* a2)
{
  int  r;
  TRACE(x11, "Call XStoreColor\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XStoreColor(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XStoreColor\n");
  return r;
}

int  TSXStoreName(Display* a0, Window a1, const  char* a2)
{
  int  r;
  TRACE(x11, "Call XStoreName\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XStoreName(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XStoreName\n");
  return r;
}

int  TSXSync(Display* a0, int a1)
{
  int  r;
  TRACE(x11, "Call XSync\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSync(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XSync\n");
  return r;
}

int  TSXTextExtents(XFontStruct* a0, const  char* a1, int a2, int* a3, int* a4, int* a5, XCharStruct* a6)
{
  int  r;
  TRACE(x11, "Call XTextExtents\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XTextExtents(a0, a1, a2, a3, a4, a5, a6);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XTextExtents\n");
  return r;
}

int  TSXTextWidth(XFontStruct* a0, const  char* a1, int a2)
{
  int  r;
  TRACE(x11, "Call XTextWidth\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XTextWidth(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XTextWidth\n");
  return r;
}

int  TSXUngrabServer(Display* a0)
{
  int  r;
  TRACE(x11, "Call XUngrabServer\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XUngrabServer(a0);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XUngrabServer\n");
  return r;
}

int  TSXUninstallColormap(Display* a0, Colormap a1)
{
  int  r;
  TRACE(x11, "Call XUninstallColormap\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XUninstallColormap(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XUninstallColormap\n");
  return r;
}

int  TSXUnmapWindow(Display* a0, Window a1)
{
  int  r;
  TRACE(x11, "Call XUnmapWindow\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XUnmapWindow(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XUnmapWindow\n");
  return r;
}

int  TSXWarpPointer(Display* a0, Window a1, Window a2, int a3, int a4, unsigned int a5, unsigned int a6, int a7, int a8)
{
  int  r;
  TRACE(x11, "Call XWarpPointer\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XWarpPointer(a0, a1, a2, a3, a4, a5, a6, a7, a8);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XWarpPointer\n");
  return r;
}

XIM  TSXOpenIM(Display* a0, struct _XrmHashBucketRec* a1, char* a2, char* a3)
{
  XIM  r;
  TRACE(x11, "Call XOpenIM\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XOpenIM(a0, a1, a2, a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XOpenIM\n");
  return r;
}

int (*TSXSynchronize(Display *a0, Bool a1))(Display *)
{
  int (*r)(Display *);
  TRACE(x11, "Call XSynchronize\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSynchronize(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XSynchronize\n");
  return r;
}

extern void _XInitImageFuncPtrs(XImage *);

void TS_XInitImageFuncPtrs(XImage *a0)
{
  TRACE(x11, "Call _XInitImageFuncPtrs\n");
  EnterCriticalSection( &X11DRV_CritSection );
  _XInitImageFuncPtrs(a0);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret _XInitImageFuncPtrs\n");
}


#endif /* !defined(X_DISPLAY_MISSING) */
