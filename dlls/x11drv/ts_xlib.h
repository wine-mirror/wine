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

extern void wine_tsx11_lock(void);
extern void wine_tsx11_unlock(void);

extern char * TSXGetAtomName(Display*, Atom);
extern char * TSXKeysymToString(KeySym);
extern Atom  TSXInternAtom(Display*, const char*, int);
extern Window  TSXGetSelectionOwner(Display*, Atom);
extern KeySym  TSXKeycodeToKeysym(Display*, unsigned int, int);
extern int  TSXChangeGC(Display*, GC, unsigned long, XGCValues*);
extern int  TSXChangeProperty(Display*, Window, Atom, Atom, int, int, const unsigned char*, int);
extern int  TSXDrawArc(Display*, Drawable, GC, int, int, unsigned int, unsigned int, int, int);
extern int  TSXDrawLine(Display*, Drawable, GC, int, int, int, int);
extern int  TSXDrawLines(Display*, Drawable, GC, XPoint*, int, int);
extern int  TSXDrawString16(Display*, Drawable, GC, int, int, const XChar2b*, int);
extern int  TSXDrawText16(Display*, Drawable, GC, int, int, XTextItem16*, int);
extern int  TSXFillArc(Display*, Drawable, GC, int, int, unsigned int, unsigned int, int, int);
extern int  TSXFillRectangle(Display*, Drawable, GC, int, int, unsigned int, unsigned int);
extern int  TSXFree(void*);
extern int  TSXFreeFont(Display*, XFontStruct*);
extern int  TSXFreePixmap(Display*, Pixmap);
extern int  TSXGetFontProperty(XFontStruct*, Atom, unsigned long*);
extern int  TSXGetGeometry(Display*, Drawable, Window*, int*, int*, unsigned int*, unsigned int*, unsigned int*, unsigned int*);
extern int  TSXGetWindowProperty(Display*, Window, Atom, long, long, int, Atom, Atom*, int*, unsigned long*, unsigned long*, unsigned char**);
extern KeyCode  TSXKeysymToKeycode(Display*, KeySym);
extern int  TSXMapWindow(Display*, Window);
extern int  TSXQueryPointer(Display*, Window, Window*, Window*, int*, int*, int*, int*, unsigned int*);
extern int  TSXQueryTree(Display*, Window, Window*, Window*, Window**, unsigned int*);
extern int  TSXSetArcMode(Display*, GC, int);
extern int  TSXSetClipRectangles(Display*, GC, int, int, XRectangle*, int, int);
extern int  TSXSetDashes(Display*, GC, int, const char*, int);
extern int  TSXSetForeground(Display*, GC, unsigned long);
extern int  TSXSetFunction(Display*, GC, int);
extern int  TSXSetLineAttributes(Display*, GC, unsigned int, int, int, int);
extern int  TSXSetSelectionOwner(Display*, Atom, Window, Time);
extern int  TSXSetSubwindowMode(Display*, GC, int);
extern int  TSXStoreColor(Display*, Colormap, XColor*);
extern int  TSXSync(Display*, int);
extern int  TSXTextExtents16(XFontStruct*, const XChar2b*, int, int*, int*, int*, XCharStruct*);
extern int  TSXTextWidth16(XFontStruct*, const XChar2b*, int);
extern int  TSXUnmapWindow(Display*, Window);

#endif /* defined(HAVE_X11_XLIB_H) */

#endif /* __WINE_TS_XLIB_H */
