/*
 * Thread safe wrappers around xvideo calls.
 * Always include this file instead of <X11/xvideo.h>.
 * This file was generated automatically by tools/make_X11wrappers
 * DO NOT EDIT!
 */

#ifndef __WINE_TS_XVIDEO_H
#define __WINE_TS_XVIDEO_H

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#ifdef HAVE_XVIDEO

#include <X11/Xlib.h>
#include <X11/extensions/Xv.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xvlib.h>

extern void (*wine_tsx11_lock)(void);
extern void (*wine_tsx11_unlock)(void);

extern int TSXvQueryExtension(Display*, unsigned int*, unsigned int*, unsigned int*, unsigned int*, unsigned int*);
extern int TSXvQueryAdaptors(Display*, Window, unsigned int*, XvAdaptorInfo**);
extern int TSXvQueryEncodings(Display*, XvPortID, unsigned int*, XvEncodingInfo**);
extern int TSXvPutVideo(Display*, XvPortID, Drawable, GC, int, int, unsigned int, unsigned int, int, int, unsigned int, unsigned int);
extern int TSXvPutStill(Display*, XvPortID, Drawable, GC, int, int, unsigned int, unsigned int, int, int, unsigned int, unsigned int);
extern int TSXvGetVideo(Display*, XvPortID, Drawable, GC, int, int, unsigned int, unsigned int, int, int, unsigned int, unsigned int);
extern int TSXvGetStill(Display*, XvPortID, Drawable, GC, int, int, unsigned int, unsigned int, int, int, unsigned int, unsigned int);
extern int TSXvStopVideo(Display*, XvPortID, Drawable);
extern int TSXvGrabPort(Display*, XvPortID, Time);
extern int TSXvUngrabPort(Display*, XvPortID, Time);
extern int TSXvSelectVideoNotify(Display*, Drawable, Bool);
extern int TSXvSelectPortNotify(Display*, XvPortID, Bool);
extern int TSXvSetPortAttribute(Display*, XvPortID, Atom, int);
extern int TSXvGetPortAttribute(Display*, XvPortID, Atom, int*);
extern int TSXvQueryBestSize(Display*, XvPortID, Bool, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int*, unsigned int*);
extern XvAttribute* TSXvQueryPortAttributes(Display*, XvPortID, int*);
extern void TSXvFreeAdaptorInfo(XvAdaptorInfo*);
extern void TSXvFreeEncodingInfo(XvEncodingInfo*);
extern XvImageFormatValues * TSXvListImageFormats(Display*, XvPortID, int*);
extern XvImage * TSXvCreateImage(Display*, XvPortID, int, char*, int, int);
extern int TSXvPutImage(Display*, XvPortID, Drawable, GC, XvImage*, int, int, unsigned int, unsigned int, int, int, unsigned int, unsigned int);
extern int TSXvShmPutImage(Display*, XvPortID, Drawable, GC, XvImage*, int, int, unsigned int, unsigned int, int, int, unsigned int, unsigned int, Bool);
extern XvImage * TSXvShmCreateImage(Display*, XvPortID, int, char*, int, int, XShmSegmentInfo*);

#endif /* defined(HAVE_XVIDEO) */

#endif /* __WINE_TS_XVIDEO_H */
