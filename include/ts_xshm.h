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
extern int TSXShmPixmapFormat(Display *);
extern Status TSXShmDetach(Display *, XShmSegmentInfo *);
extern Status TSXShmAttach(Display *, XShmSegmentInfo *);

#endif /* __WINE_TSXSHM_H */
