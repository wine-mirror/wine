/*
 * Thread safe wrappers around XShm calls.
 * Always include this file instead of <X11/XShm.h>.
 * This file was generated automatically by tools/make_X11wrappers
 * DO NOT EDIT!
 */

#ifndef __WINE_TS_XSHM_H
#define __WINE_TS_XSHM_H

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#ifdef HAVE_LIBXXSHM

#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>

extern void (*wine_tsx11_lock)(void);
extern void (*wine_tsx11_unlock)(void);

extern Bool TSXShmQueryExtension(Display *);
extern Bool TSXShmQueryVersion(Display *, int *, int *, Bool *);
extern int TSXShmPixmapFormat(Display *);
extern Status TSXShmAttach(Display *, XShmSegmentInfo *);
extern Status TSXShmDetach(Display *, XShmSegmentInfo *);
extern Status TSXShmPutImage(Display *, Drawable, GC, XImage *, int, int, int, int, unsigned int, unsigned int, Bool);
extern Status TSXShmGetImage(Display *, Drawable, XImage *, int, int, unsigned long);
extern XImage * TSXShmCreateImage(Display *, Visual *, unsigned int, int, char *, XShmSegmentInfo *, unsigned int, unsigned int);
extern Pixmap TSXShmCreatePixmap(Display *, Drawable, char *, XShmSegmentInfo *, unsigned int, unsigned int, unsigned int);

#endif /* defined(HAVE_LIBXXSHM) */

#endif /* __WINE_TS_XSHM_H */
