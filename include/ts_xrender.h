/*
 * Thread safe wrappers around Xrender calls.
 * Always include this file instead of <X11/Xrender.h>.
 * This file was generated automatically by tools/make_X11wrappers
 *
 * Copyright 1998 Kristian Nielsen
 */

#ifndef __WINE_TS_XRENDER_H
#define __WINE_TS_XRENDER_H

#include "config.h"

#ifdef HAVE_LIBXRENDER

#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>

extern void (*wine_tsx11_lock)(void);
extern void (*wine_tsx11_unlock)(void);

extern void TSXRenderAddGlyphs(Display*,GlyphSet,Glyph*,XGlyphInfo*,int,char*,int);
extern void TSXRenderCompositeString8(Display*,int,Picture,Picture,XRenderPictFormat*,GlyphSet,int,int,int,int,char*,int);
extern void TSXRenderCompositeString16(Display*,int,Picture,Picture,XRenderPictFormat*,GlyphSet,int,int,int,int,unsigned short*,int);
extern void TSXRenderCompositeString32(Display*,int,Picture,Picture,XRenderPictFormat*,GlyphSet,int,int,int,int,unsigned int*,int);
extern GlyphSet TSXRenderCreateGlyphSet(Display*,XRenderPictFormat*);
extern Picture TSXRenderCreatePicture(Display*,Drawable,XRenderPictFormat*,unsigned long,XRenderPictureAttributes*);
extern void TSXRenderFillRectangle(Display*,int,Picture,XRenderColor*,int,int,unsigned int, unsigned int);
extern XRenderPictFormat* TSXRenderFindFormat(Display*,unsigned long,XRenderPictFormat*,int);
extern XRenderPictFormat* TSXRenderFindVisualFormat(Display*,Visual*);
extern void TSXRenderFreeGlyphSet(Display*,GlyphSet);
extern void TSXRenderFreePicture(Display*,Picture);
extern void TSXRenderSetPictureClipRectangles(Display*,Picture,int,int,XRectangle*,int);
extern Bool TSXRenderQueryExtension(Display*,int*,int*);

#endif /* defined(HAVE_LIBXRENDER) */

#endif /* __WINE_TS_XRENDER_H */
