/*
 * Thread safe wrappers around Xrender calls.
 * This file was generated automatically by tools/make_X11wrappers
 * DO NOT EDIT!
 */

#include "config.h"

#ifdef HAVE_LIBXRENDER

#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>

#include "ts_xrender.h"


void TSXRenderAddGlyphs(Display*a0,GlyphSet a1,Glyph*a2,XGlyphInfo*a3,int a4,char*a5,int a6)
{
  wine_tsx11_lock();
  XRenderAddGlyphs(a0,a1,a2,a3,a4,a5,a6);
  wine_tsx11_unlock();
}

void TSXRenderCompositeString8(Display*a0,int a1,Picture a2,Picture a3,XRenderPictFormat*a4,GlyphSet a5,int a6,int a7,int a8,int a9,char*a10,int a11)
{
  wine_tsx11_lock();
  XRenderCompositeString8(a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11);
  wine_tsx11_unlock();
}

void TSXRenderCompositeString16(Display*a0,int a1,Picture a2,Picture a3,XRenderPictFormat*a4,GlyphSet a5,int a6,int a7,int a8,int a9,unsigned short*a10,int a11)
{
  wine_tsx11_lock();
  XRenderCompositeString16(a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11);
  wine_tsx11_unlock();
}

void TSXRenderCompositeString32(Display*a0,int a1,Picture a2,Picture a3,XRenderPictFormat*a4,GlyphSet a5,int a6,int a7,int a8,int a9,unsigned int*a10,int a11)
{
  wine_tsx11_lock();
  XRenderCompositeString32(a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11);
  wine_tsx11_unlock();
}

GlyphSet TSXRenderCreateGlyphSet(Display*a0,XRenderPictFormat*a1)
{
  GlyphSet r;
  wine_tsx11_lock();
  r = XRenderCreateGlyphSet(a0,a1);
  wine_tsx11_unlock();
  return r;
}

Picture TSXRenderCreatePicture(Display*a0,Drawable a1,XRenderPictFormat*a2,unsigned long a3,XRenderPictureAttributes*a4)
{
  Picture r;
  wine_tsx11_lock();
  r = XRenderCreatePicture(a0,a1,a2,a3,a4);
  wine_tsx11_unlock();
  return r;
}

void TSXRenderFillRectangle(Display*a0,int a1,Picture a2,XRenderColor*a3,int a4,int a5,unsigned int a6,unsigned int a7)
{
  wine_tsx11_lock();
  XRenderFillRectangle(a0,a1,a2,a3,a4,a5,a6,a7);
  wine_tsx11_unlock();
}

XRenderPictFormat* TSXRenderFindFormat(Display*a0,unsigned long a1,XRenderPictFormat*a2,int a3)
{
  XRenderPictFormat* r;
  wine_tsx11_lock();
  r = XRenderFindFormat(a0,a1,a2,a3);
  wine_tsx11_unlock();
  return r;
}

XRenderPictFormat* TSXRenderFindVisualFormat(Display*a0,Visual*a1)
{
  XRenderPictFormat* r;
  wine_tsx11_lock();
  r = XRenderFindVisualFormat(a0,a1);
  wine_tsx11_unlock();
  return r;
}

void TSXRenderFreeGlyphSet(Display*a0,GlyphSet a1)
{
  wine_tsx11_lock();
  XRenderFreeGlyphSet(a0,a1);
  wine_tsx11_unlock();
}

void TSXRenderFreePicture(Display*a0,Picture a1)
{
  wine_tsx11_lock();
  XRenderFreePicture(a0,a1);
  wine_tsx11_unlock();
}

void TSXRenderSetPictureClipRectangles(Display*a0,Picture a1,int a2,int a3,XRectangle* a4,int a5)
{
  wine_tsx11_lock();
  XRenderSetPictureClipRectangles(a0,a1,a2,a3,a4,a5);
  wine_tsx11_unlock();
}

Bool TSXRenderQueryExtension(Display*a0,int*a1,int*a2)
{
  Bool r;
  wine_tsx11_lock();
  r = XRenderQueryExtension(a0,a1,a2);
  wine_tsx11_unlock();
  return r;
}

#endif /* defined(HAVE_LIBXRENDER) */

