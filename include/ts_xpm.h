/*
 * Thread safe wrappers around xpm calls.
 * Always include this file instead of <X11/xpm.h>.
 * This file was generated automatically by tools/make_X11wrappers
 * DO NOT EDIT!
 */

#ifndef __WINE_TS_XPM_H
#define __WINE_TS_XPM_H

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#ifdef HAVE_LIBXXPM

#include <X11/xpm.h>

extern void (*wine_tsx11_lock)(void);
extern void (*wine_tsx11_unlock)(void);

extern int TSXpmCreatePixmapFromData(Display *, Drawable, char **, Pixmap *, Pixmap *, XpmAttributes *);
extern int TSXpmAttributesSize(void);

#endif /* defined(HAVE_LIBXXPM) */

#endif /* __WINE_TS_XPM_H */
