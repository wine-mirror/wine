/*
 * Thread safe wrappers around Xlib calls.
 * Always include this file instead of <X11/Xlib.h>.
 * This file was generated automatically by tools/make_X11wrappers
 *
 * Copyright 1998 Kristian Nielsen
 */

#ifndef __WINE_TSXLIB_H
#define __WINE_TSXLIB_H

#include <X11/Xlib.h>

extern XFontStruct * TSXLoadQueryFont(Display*, const  char*);
extern XModifierKeymap	* TSXGetModifierMapping(Display*);
extern XImage * TSXCreateImage(Display*, Visual*, unsigned int, int, int, char*, unsigned int, unsigned int, int, int);
extern XImage * TSXGetImage(Display*, Drawable, int, int, unsigned int, unsigned int, unsigned long, int);
extern Display * TSXOpenDisplay(const  char*);
extern void  TSXrmInitialize(void);
extern char * TSXGetAtomName(Display*, Atom);
extern char * TSXKeysymToString(KeySym);
extern Atom  TSXInternAtom(Display*, const  char*, int);
extern Colormap  TSXCopyColormapAndFree(Display*, Colormap);
extern Colormap  TSXCreateColormap(Display*, Window, Visual*, int);
extern Cursor  TSXCreatePixmapCursor(Display*, Pixmap, Pixmap, XColor*, XColor*, unsigned int, unsigned int);
extern Cursor  TSXCreateFontCursor(Display*, unsigned int);
extern GC  TSXCreateGC(Display*, Drawable, unsigned long, XGCValues*);
extern Pixmap  TSXCreatePixmap(Display*, Drawable, unsigned int, unsigned int, unsigned int);
extern Pixmap  TSXCreateBitmapFromData(Display*, Drawable, const  char*, unsigned int, unsigned int);
extern Window  TSXGetSelectionOwner(Display*, Atom);
extern Window  TSXCreateWindow(Display*, Window, int, int, unsigned int, unsigned int, unsigned int, int, unsigned int, Visual*, unsigned long, XSetWindowAttributes*);
extern char ** TSXListFonts(Display*, const  char*, int, int*);
extern KeySym  TSXKeycodeToKeysym(Display*, unsigned int, int);
extern KeySym  TSXLookupKeysym(XKeyEvent*, int);
extern KeySym * TSXGetKeyboardMapping(Display*, unsigned int, int, int*);
extern char * TSXResourceManagerString(Display*);
extern int   TSXInitThreads(void);
extern int * TSXListDepths(Display*, int, int*);
extern int   TSXReconfigureWMWindow(Display*, Window, int, unsigned int, XWindowChanges*);
extern int   TSXSetWMProtocols(Display*, Window, Atom*, int);
extern int  TSXSetTransientForHint(Display*, Window, Window);
extern int  TSXActivateScreenSaver(Display*);
extern int   TSXAllocColor(Display*, Colormap, XColor*);
extern int   TSXAllocColorCells(Display*, Colormap, int, unsigned long*, unsigned int, unsigned long*, unsigned int);
extern int  TSXBell(Display*, int);
extern int  TSXChangeGC(Display*, GC, unsigned long, XGCValues*);
extern int  TSXChangeKeyboardControl(Display*, unsigned long, XKeyboardControl*);
extern int  TSXChangeProperty(Display*, Window, Atom, Atom, int, int, const  unsigned char*, int);
extern int  TSXChangeWindowAttributes(Display*, Window, unsigned long, XSetWindowAttributes*);
extern int   TSXCheckTypedWindowEvent(Display*, Window, int, XEvent*);
extern int   TSXCheckWindowEvent(Display*, Window, long, XEvent*);
extern int  TSXConvertSelection(Display*, Atom, Atom, Atom, Window, Time);
extern int  TSXCopyArea(Display*, Drawable, Drawable, GC, int, int, unsigned int, unsigned int, int, int);
extern int  TSXCopyPlane(Display*, Drawable, Drawable, GC, int, int, unsigned int, unsigned int, int, int, unsigned long);
extern int  TSXDefineCursor(Display*, Window, Cursor);
extern int  TSXDestroyWindow(Display*, Window);
extern int  TSXDisplayKeycodes(Display*, int*, int*);
extern int  TSXDrawArc(Display*, Drawable, GC, int, int, unsigned int, unsigned int, int, int);
extern int  TSXDrawLine(Display*, Drawable, GC, int, int, int, int);
extern int  TSXDrawLines(Display*, Drawable, GC, XPoint*, int, int);
extern int  TSXDrawPoint(Display*, Drawable, GC, int, int);
extern int  TSXDrawRectangle(Display*, Drawable, GC, int, int, unsigned int, unsigned int);
extern int  TSXDrawSegments(Display*, Drawable, GC, XSegment*, int);
extern int  TSXDrawString(Display*, Drawable, GC, int, int, const  char*, int);
extern int  TSXDrawText(Display*, Drawable, GC, int, int, XTextItem*, int);
extern int  TSXFillArc(Display*, Drawable, GC, int, int, unsigned int, unsigned int, int, int);
extern int  TSXFillPolygon(Display*, Drawable, GC, XPoint*, int, int, int);
extern int  TSXFillRectangle(Display*, Drawable, GC, int, int, unsigned int, unsigned int);
extern int  TSXFlush(Display*);
extern int  TSXFree(void*);
extern int  TSXFreeColormap(Display*, Colormap);
extern int  TSXFreeColors(Display*, Colormap, unsigned long*, int, unsigned long);
extern int  TSXFreeCursor(Display*, Cursor);
extern int  TSXFreeFont(Display*, XFontStruct*);
extern int  TSXFreeFontNames(char**);
extern int  TSXFreeGC(Display*, GC);
extern int  TSXFreeModifiermap(XModifierKeymap*);
extern int  TSXFreePixmap(Display*, Pixmap);
extern int   TSXGetFontProperty(XFontStruct*, Atom, unsigned long*);
extern int   TSXGetGeometry(Display*, Drawable, Window*, int*, int*, unsigned int*, unsigned int*, unsigned int*, unsigned int*);
extern int  TSXGetInputFocus(Display*, Window*, int*);
extern int  TSXGetKeyboardControl(Display*, XKeyboardState*);
extern int  TSXGetScreenSaver(Display*, int*, int*, int*, int*);
extern int  TSXGetWindowProperty(Display*, Window, Atom, long, long, int, Atom, Atom*, int*, unsigned long*, unsigned long*, unsigned char**);
extern int   TSXGetWindowAttributes(Display*, Window, XWindowAttributes*);
extern int  TSXGrabServer(Display*);
extern int  TSXInstallColormap(Display*, Colormap);
extern KeyCode  TSXKeysymToKeycode(Display*, KeySym);
extern int  TSXMapWindow(Display*, Window);
extern int  TSXNextEvent(Display*, XEvent*);
extern int  TSXParseGeometry(const  char*, int*, int*, unsigned int*, unsigned int*);
extern int  TSXPending(Display*);
extern int  TSXPutBackEvent(Display*, XEvent*);
extern int  TSXPutImage(Display*, Drawable, GC, XImage*, int, int, int, int, unsigned int, unsigned int);
extern int  TSXQueryColor(Display*, Colormap, XColor*);
extern int   TSXQueryPointer(Display*, Window, Window*, Window*, int*, int*, int*, int*, unsigned int*);
extern int   TSXQueryTree(Display*, Window, Window*, Window*, Window**, unsigned int*);
extern int  TSXResetScreenSaver(Display*);
extern int  TSXRestackWindows(Display*, Window*, int);
extern int   TSXSendEvent(Display*, Window, int, long, XEvent*);
extern int  TSXSetArcMode(Display*, GC, int);
extern int  TSXSetBackground(Display*, GC, unsigned long);
extern int  TSXSetClipMask(Display*, GC, Pixmap);
extern int  TSXSetClipOrigin(Display*, GC, int, int);
extern int  TSXSetClipRectangles(Display*, GC, int, int, XRectangle*, int, int);
extern int  TSXSetDashes(Display*, GC, int, const  char*, int);
extern int  TSXSetFillStyle(Display*, GC, int);
extern int  TSXSetForeground(Display*, GC, unsigned long);
extern int  TSXSetFunction(Display*, GC, int);
extern int  TSXSetGraphicsExposures(Display*, GC, int);
extern int  TSXSetIconName(Display*, Window, const  char*);
extern int  TSXSetInputFocus(Display*, Window, int, Time);
extern int  TSXSetLineAttributes(Display*, GC, unsigned int, int, int, int);
extern int  TSXSetScreenSaver(Display*, int, int, int, int);
extern int  TSXSetSelectionOwner(Display*, Atom, Window, Time);
extern int  TSXSetSubwindowMode(Display*, GC, int);
extern int  TSXSetWindowColormap(Display*, Window, Colormap);
extern int  TSXStoreColor(Display*, Colormap, XColor*);
extern int  TSXStoreName(Display*, Window, const  char*);
extern int  TSXSync(Display*, int);
extern int  TSXTextExtents(XFontStruct*, const  char*, int, int*, int*, int*, XCharStruct*);
extern int  TSXTextWidth(XFontStruct*, const  char*, int);
extern int  TSXUngrabServer(Display*);
extern int  TSXUninstallColormap(Display*, Colormap);
extern int  TSXUnmapWindow(Display*, Window);
extern int  TSXWarpPointer(Display*, Window, Window, int, int, unsigned int, unsigned int, int, int);
extern XIM  TSXOpenIM(Display*, struct _XrmHashBucketRec*, char*, char*);
extern int (*TSXSynchronize(Display *, Bool))(Display *);
extern void TS_XInitImageFuncPtrs(XImage *);

#endif /* __WINE_TSXLIB_H */
