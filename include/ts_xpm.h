/*
 * Thread safe wrappers around xpm calls.
 * Always include this file instead of <X11/xpm.h>.
 * This file was generated automatically by tools/make_X11wrappers
 *
 * Copyright 1998 Kristian Nielsen
 */

#ifndef __WINE_TS_XPM_H
#define __WINE_TS_XPM_H

#include "config.h"

#ifndef X_DISPLAY_MISSING

#ifdef HAVE_LIBXXPM

#include <X11/xpm.h>

extern int TSXpmCreatePixmapFromData(Display *, Drawable, char **, Pixmap *, Pixmap *, XpmAttributes *);
extern int TSXpmAttributesSize(void);

#endif /* defined(HAVE_LIBXXPM) */

#endif /* !defined(X_DISPLAY_MISSING) */

#endif /* __WINE_TS_XPM_H */
