/*
 * Thread safe wrappers around xf86dga2 calls.
 * Always include this file instead of <X11/xf86dga2.h>.
 * This file was generated automatically by tools/make_X11wrappers
 *
 * Copyright 1998 Kristian Nielsen
 */

#ifndef __WINE_TS_XF86DGA2_H
#define __WINE_TS_XF86DGA2_H

#include "config.h"

#ifdef HAVE_LIBXXF86DGA2

#include <X11/Xlib.h>
#include <X11/extensions/xf86dga.h>

extern Bool TSXDGAQueryVersion(Display*, int*, int*);
extern Bool TSXDGAQueryExtension(Display*, int*, int*);
extern XDGAMode* TSXDGAQueryModes(Display*, int, int*);
extern XDGADevice* TSXDGASetMode(Display*, int, int);
extern Bool TSXDGAOpenFramebuffer(Display*, int);
extern void TSXDGACloseFramebuffer(Display*, int);
extern void TSXDGASetViewport(Display*, int, int, int, int);
extern void TSXDGAInstallColormap(Display*, int, Colormap);
extern Colormap TSXDGACreateColormap(Display*, int, XDGADevice*, int);
extern void TSXDGASelectInput(Display*, int, long);
extern void TSXDGAFillRectangle(Display*, int, int, int, unsigned int, unsigned int, unsigned long);
extern void TSXDGACopyArea(Display*, int, int, int, unsigned int, unsigned int, int, int);
extern void TSXDGACopyTransparentArea(Display*, int, int, int, unsigned int, unsigned int, int, int, unsigned long);
extern int TSXDGAGetViewportStatus(Display*, int);
extern void TSXDGASync(Display*, int);
extern Bool TSXDGASetClientVersion(Display*);
extern void TSXDGAChangePixmapMode(Display*, int, int*, int*, int);
extern void TSXDGAKeyEventToXKeyEvent(XDGAKeyEvent*, XKeyEvent*);

#endif /* defined(HAVE_LIBXXF86DGA2) */

#endif /* __WINE_TS_XF86DGA2_H */
