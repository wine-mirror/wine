/*
 * Thread safe wrappers around Xlib calls.
 * Always include this file instead of <X11/Xlib.h>.
 * This file was generated automatically by tools/make_X11wrappers
 * DO NOT EDIT!
 */

#ifndef __WINE_TS_XLIB_H
#define __WINE_TS_XLIB_H

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#ifdef HAVE_X11_XLIB_H

#include <X11/Xlib.h>

extern void (*wine_tsx11_lock)(void);
extern void (*wine_tsx11_unlock)(void);

extern XFontStruct * TSXLoadQueryFont(Display*, const  char*);
extern XImage * TSXGetImage(Display*, Drawable, int, int, unsigned int, unsigned int, unsigned long, int);
extern Display * TSXOpenDisplay(const  char*);
extern char * TSXGetAtomName(Display*, Atom);
extern char * TSXKeysymToString(KeySym);
extern Atom  TSXInternAtom(Display*, const  char*, int);
extern Colormap  TSXCreateColormap(Display*, Window, Visual*, int);
extern Pixmap  TSXCreatePixmap(Display*, Drawable, unsigned int, unsigned int, unsigned int);
extern Pixmap  TSXCreateBitmapFromData(Display*, Drawable, const  char*, unsigned int, unsigned int);
extern Window  TSXGetSelectionOwner(Display*, Atom);
extern char ** TSXListFonts(Display*, const  char*, int, int*);
extern KeySym  TSXKeycodeToKeysym(Display*, unsigned int, int);
extern int * TSXListDepths(Display*, int, int*);
extern int   TSXReconfigureWMWindow(Display*, Window, int, unsigned int, XWindowChanges*);
extern int   TSXAllocColor(Display*, Colormap, XColor*);
extern int   TSXAllocColorCells(Display*, Colormap, int, unsigned long*, unsigned int, unsigned long*, unsigned int);
extern int  TSXBell(Display*, int);
extern int  TSXChangeGC(Display*, GC, unsigned long, XGCValues*);
extern int  TSXChangeProperty(Display*, Window, Atom, Atom, int, int, const  unsigned char*, int);
extern int  TSXChangeWindowAttributes(Display*, Window, unsigned long, XSetWindowAttributes*);
extern int  TSXCopyArea(Display*, Drawable, Drawable, GC, int, int, unsigned int, unsigned int, int, int);
extern int  TSXCopyPlane(Display*, Drawable, Drawable, GC, int, int, unsigned int, unsigned int, int, int, unsigned long);
extern int  TSXDefineCursor(Display*, Window, Cursor);
extern int  TSXDeleteProperty(Display*, Window, Atom);
extern int  TSXDrawArc(Display*, Drawable, GC, int, int, unsigned int, unsigned int, int, int);
extern int  TSXDrawLine(Display*, Drawable, GC, int, int, int, int);
extern int  TSXDrawLines(Display*, Drawable, GC, XPoint*, int, int);
extern int  TSXDrawRectangle(Display*, Drawable, GC, int, int, unsigned int, unsigned int);
extern int  TSXDrawString16(Display*, Drawable, GC, int, int, const  XChar2b*, int);
extern int  TSXDrawText16(Display*, Drawable, GC, int, int, XTextItem16*, int);
extern int  TSXFillArc(Display*, Drawable, GC, int, int, unsigned int, unsigned int, int, int);
extern int  TSXFillPolygon(Display*, Drawable, GC, XPoint*, int, int, int);
extern int  TSXFillRectangle(Display*, Drawable, GC, int, int, unsigned int, unsigned int);
extern int  TSXFlush(Display*);
extern int  TSXFree(void*);
extern int  TSXFreeColormap(Display*, Colormap);
extern int  TSXFreeColors(Display*, Colormap, unsigned long*, int, unsigned long);
extern int  TSXFreeFont(Display*, XFontStruct*);
extern int  TSXFreeFontNames(char**);
extern int  TSXFreePixmap(Display*, Pixmap);
extern int   TSXGetFontProperty(XFontStruct*, Atom, unsigned long*);
extern int   TSXGetGeometry(Display*, Drawable, Window*, int*, int*, unsigned int*, unsigned int*, unsigned int*, unsigned int*);
extern int  TSXGetScreenSaver(Display*, int*, int*, int*, int*);
extern int  TSXGetWindowProperty(Display*, Window, Atom, long, long, int, Atom, Atom*, int*, unsigned long*, unsigned long*, unsigned char**);
extern int   TSXGetWindowAttributes(Display*, Window, XWindowAttributes*);
extern int  TSXGrabPointer(Display*, Window, int, unsigned int, int, int, Window, Cursor, Time);
extern int  TSXGrabServer(Display*);
extern KeyCode  TSXKeysymToKeycode(Display*, KeySym);
extern int  TSXMapWindow(Display*, Window);
extern int  TSXQueryColor(Display*, Colormap, XColor*);
extern int  TSXQueryColors(Display*, Colormap, XColor*, int);
extern int   TSXQueryPointer(Display*, Window, Window*, Window*, int*, int*, int*, int*, unsigned int*);
extern int   TSXQueryTree(Display*, Window, Window*, Window*, Window**, unsigned int*);
extern int  TSXRefreshKeyboardMapping(XMappingEvent*);
extern int   TSXSendEvent(Display*, Window, int, long, XEvent*);
extern int  TSXSetArcMode(Display*, GC, int);
extern int  TSXSetClipRectangles(Display*, GC, int, int, XRectangle*, int, int);
extern int  TSXSetDashes(Display*, GC, int, const  char*, int);
extern int  TSXSetForeground(Display*, GC, unsigned long);
extern int  TSXSetFunction(Display*, GC, int);
extern int  TSXSetGraphicsExposures(Display*, GC, int);
extern int  TSXSetLineAttributes(Display*, GC, unsigned int, int, int, int);
extern int  TSXSetScreenSaver(Display*, int, int, int, int);
extern int  TSXSetSelectionOwner(Display*, Atom, Window, Time);
extern int  TSXSetSubwindowMode(Display*, GC, int);
extern int  TSXStoreColor(Display*, Colormap, XColor*);
extern int  TSXSync(Display*, int);
extern int  TSXTextExtents16(XFontStruct*, const  XChar2b*, int, int*, int*, int*, XCharStruct*);
extern int  TSXTextWidth16(XFontStruct*, const  XChar2b*, int);
extern int  TSXUngrabPointer(Display*, Time);
extern int  TSXUngrabServer(Display*);
extern int  TSXUninstallColormap(Display*, Colormap);
extern int  TSXUnmapWindow(Display*, Window);
extern XIM  TSXOpenIM(Display*, struct _XrmHashBucketRec*, char*, char*);

#endif /* defined(HAVE_X11_XLIB_H) */

#endif /* __WINE_TS_XLIB_H */
