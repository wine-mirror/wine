/*
 * Thread safe wrappers around xf86vmode calls.
 * Always include this file instead of <X11/xf86vmode.h>.
 * This file was generated automatically by tools/make_X11wrappers
 * DO NOT EDIT!
 */

#ifndef __WINE_TS_XF86VMODE_H
#define __WINE_TS_XF86VMODE_H

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#include "windef.h"
#ifdef HAVE_LIBXXF86VM
#define XMD_H
#include "basetsd.h"

#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>

extern void (*wine_tsx11_lock)(void);
extern void (*wine_tsx11_unlock)(void);

extern Bool TSXF86VidModeQueryVersion(Display*,int*,int*);
extern Bool TSXF86VidModeQueryExtension(Display*,int*,int*);
extern Bool TSXF86VidModeGetModeLine(Display*,int,int*,XF86VidModeModeLine*);
extern Bool TSXF86VidModeGetAllModeLines(Display*,int,int*,XF86VidModeModeInfo***);
extern Bool TSXF86VidModeSwitchToMode(Display*,int,XF86VidModeModeInfo*);
extern Bool TSXF86VidModeLockModeSwitch(Display*,int,int);
extern Bool TSXF86VidModeSetViewPort(Display*,int,int,int);

#endif /* defined(HAVE_LIBXXF86VM) */

#endif /* __WINE_TS_XF86VMODE_H */
