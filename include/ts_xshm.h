/*
 * Thread safe wrappers around XShm calls.
 * Always include this file instead of <X11/XShm.h>.
 * This file was generated automatically by tools/make_X11wrappers
 *
 * Copyright 1998 Kristian Nielsen
 */

#ifndef __WINE_TSXSHM_H
#define __WINE_TSXSHM_H

#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>

extern Bool TSXShmQueryExtension(Display *);
extern Bool TSXShmQueryVersion(Display *, int *, int *, Bool *);
extern int TSXShmPixmapFormat(Display *);
extern Status TSXShmAttach(Display *, XShmSegmentInfo *);
extern Status TSXShmDetach(Display *, XShmSegmentInfo *);
extern Status TSXShmPutImage(Display *, Drawable, GC, XImage *, int, int, int, int, unsigned int, unsigned int, Bool);
extern Status TSXShmGetImage(Display *, Drawable, XImage *, int, int, unsigned long);
extern XImage * TSXShmCreateImage(Display *, Visual *, unsigned int, int, char *, XShmSegmentInfo *, unsigned int, unsigned int);
extern Pixmap TSXShmCreatePixmap(Display *, Drawable, char *, XShmSegmentInfo *, unsigned int, unsigned int, unsigned int);

#endif /* __WINE_TSXSHM_H */
