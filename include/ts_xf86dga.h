/*
 * Thread safe wrappers around xf86dga calls.
 * Always include this file instead of <X11/xf86dga.h>.
 * This file was generated automatically by tools/make_X11wrappers
 *
 * Copyright 1998 Kristian Nielsen
 */

#ifndef __WINE_TSXF86DGA_H
#define __WINE_TSXF86DGA_H

#include <X11/Xlib.h>
#include <X11/extensions/xf86dga.h>

extern Bool TSXF86DGAQueryVersion(Display*,int*,int*);
extern Bool TSXF86DGAQueryExtension(Display*,int*,int*);
extern Status TSXF86DGAGetVideo(Display*,int,char**,int*,int*,int*);
extern Status TSXF86DGADirectVideo(Display*,int,int);
extern Status TSXF86DGAGetViewPortSize(Display*,int,int*,int*);
extern Status TSXF86DGASetViewPort(Display*,int,int,int);
extern Status TSXF86DGAInstallColormap(Display*,int,Colormap);
extern Status TSXF86DGAQueryDirectVideo(Display*,int,int*);
extern Status TSXF86DGAViewPortChanged(Display*,int,int);

#endif /* __WINE_TSXF86DGA_H */
