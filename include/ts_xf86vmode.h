/*
 * Thread safe wrappers around xf86vmode calls.
 * Always include this file instead of <X11/xf86vmode.h>.
 * This file was generated automatically by tools/make_X11wrappers
 *
 * Copyright 1998 Kristian Nielsen
 */

#ifndef __WINE_TSXF86VMODE_H
#define __WINE_TSXF86VMODE_H

#include "config.h"

#ifndef X_DISPLAY_MISSING

#include "windef.h"
#ifdef HAVE_LIBXXF86VM
#define XMD_H

#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>

extern Bool TSXF86VidModeQueryVersion(Display*,int*,int*);
extern Bool TSXF86VidModeQueryExtension(Display*,int*,int*);
extern Bool TSXF86VidModeGetModeLine(Display*,int,int*,XF86VidModeModeLine*);
extern Bool TSXF86VidModeGetAllModeLines(Display*,int,int*,XF86VidModeModeInfo***);
extern Bool TSXF86VidModeAddModeLine(Display*,int,XF86VidModeModeInfo*,XF86VidModeModeInfo*);
extern Bool TSXF86VidModeDeleteModeLine(Display*,int,XF86VidModeModeInfo*);
extern Bool TSXF86VidModeModModeLine(Display*,int,XF86VidModeModeLine*);
extern Status TSXF86VidModeValidateModeLine(Display*,int,XF86VidModeModeInfo*);
extern Bool TSXF86VidModeSwitchMode(Display*,int,int);
extern Bool TSXF86VidModeSwitchToMode(Display*,int,XF86VidModeModeInfo*);
extern Bool TSXF86VidModeLockModeSwitch(Display*,int,int);
extern Bool TSXF86VidModeGetMonitor(Display*,int,XF86VidModeMonitor*);
extern Bool TSXF86VidModeGetViewPort(Display*,int,int*,int*);
extern Bool TSXF86VidModeSetViewPort(Display*,int,int,int);

#endif /* defined(HAVE_LIBXXF86VM) */

#endif /* !defined(X_DISPLAY_MISSING) */

#endif /* __WINE_TSXF86VMODE_H */
