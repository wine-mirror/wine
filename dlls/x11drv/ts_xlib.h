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
extern Window  TSXGetSelectionOwner(Display*, Atom);
extern KeySym  TSXKeycodeToKeysym(Display*, unsigned int, int);
extern int  TSXChangeProperty(Display*, Window, Atom, Atom, int, int, const unsigned char*, int);
extern int  TSXFree(void*);
extern int  TSXFreeFont(Display*, XFontStruct*);
extern int  TSXGetFontProperty(XFontStruct*, Atom, unsigned long*);
extern int  TSXGetWindowProperty(Display*, Window, Atom, long, long, int, Atom, Atom*, int*, unsigned long*, unsigned long*, unsigned char**);
extern KeyCode  TSXKeysymToKeycode(Display*, KeySym);
extern int  TSXMapWindow(Display*, Window);
extern int  TSXQueryPointer(Display*, Window, Window*, Window*, int*, int*, int*, int*, unsigned int*);
extern int  TSXQueryTree(Display*, Window, Window*, Window*, Window**, unsigned int*);
extern int  TSXSetSelectionOwner(Display*, Atom, Window, Time);
extern int  TSXSync(Display*, int);
extern int  TSXUnmapWindow(Display*, Window);

#endif /* defined(HAVE_X11_XLIB_H) */

#endif /* __WINE_TS_XLIB_H */
