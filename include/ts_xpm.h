/*
 * Thread safe wrappers around xpm calls.
 * Always include this file instead of <X11/xpm.h>.
 * This file was generated automatically by tools/make_X11wrappers
 *
 * Copyright 1998 Kristian Nielsen
 */

#ifndef __WINE_TSXPM_H
#define __WINE_TSXPM_H

#include <X11/xpm.h>

extern int TSXpmCreatePixmapFromData(Display *, Drawable, char **, Pixmap *, Pixmap *, XpmAttributes *);
extern int TSXpmAttributesSize(void);

#endif /* __WINE_TSXPM_H */
