/*
 * Thread safe wrappers around Xutil calls.
 * Always include this file instead of <X11/Xutil.h>.
 * This file was generated automatically by tools/make_X11wrappers
 * DO NOT EDIT!
 */

#ifndef __WINE_TS_XUTIL_H
#define __WINE_TS_XUTIL_H

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#ifdef HAVE_X11_XLIB_H

#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>

extern void (*wine_tsx11_lock)(void);
extern void (*wine_tsx11_unlock)(void);

extern XWMHints * TSXAllocWMHints(void);
extern int  TSXFindContext(Display*, XID, XContext, XPointer*);
extern XWMHints * TSXGetWMHints(Display*, Window);
extern int  TSXLookupString(XKeyEvent*, char*, int, KeySym*, XComposeStatus*);
extern int  TSXSetWMHints(Display*, Window, XWMHints*);
extern int TSXDestroyImage(struct _XImage *);

#endif /* defined(HAVE_X11_XLIB_H) */

#endif /* __WINE_TS_XUTIL_H */
