/*
 * Functions to use the XRender extension
 *
 * Copyright 2001 Huw D M Davies for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "winnt.h"
#include "wownt32.h"
#include "x11drv.h"
#include "bitmap.h"
#include "wine/unicode.h"
#include "wine/debug.h"

BOOL X11DRV_XRender_Installed = FALSE;
WINE_DEFAULT_DEBUG_CHANNEL(xrender);

#ifdef HAVE_X11_EXTENSIONS_XRENDER_H

#include "ts_xlib.h"
#include <X11/extensions/Xrender.h>

static XRenderPictFormat *screen_format; /* format of screen */
static XRenderPictFormat *mono_format; /* format of mono bitmap */

typedef struct
{
  LOGFONTW lf;
  XFORM  xform; /* this is dum as we don't care about offsets */
  DWORD hash;
} LFANDSIZE;

#define INITIAL_REALIZED_BUF_SIZE 128


typedef struct
{
  LFANDSIZE lfsz;
  GlyphSet glyphset;
  XRenderPictFormat *font_format;
  int nrealized;
  BOOL *realized;
  UINT count;
  INT next;
} gsCacheEntry;

struct tagXRENDERINFO
{
    int                cache_index;
    Picture            pict;
    Picture            tile_pict;
    Pixmap             tile_xpm;
    COLORREF           lastTextColor;
};


static gsCacheEntry *glyphsetCache = NULL;
static DWORD glyphsetCacheSize = 0;
static INT lastfree = -1;
static INT mru = -1;

#define INIT_CACHE_SIZE 10

static int antialias = 1;

/* some default values just in case */
#ifndef SONAME_LIBX11
#define SONAME_LIBX11 "libX11.so"
#endif
#ifndef SONAME_LIBXEXT
#define SONAME_LIBXEXT "libXext.so"
#endif
#ifndef SONAME_LIBXRENDER
#define SONAME_LIBXRENDER "libXrender.so"
#endif

static void *xrender_handle;

#define MAKE_FUNCPTR(f) static typeof(f) * p##f;
MAKE_FUNCPTR(XRenderAddGlyphs)
MAKE_FUNCPTR(XRenderCompositeString8)
MAKE_FUNCPTR(XRenderCompositeString16)
MAKE_FUNCPTR(XRenderCompositeString32)
MAKE_FUNCPTR(XRenderCreateGlyphSet)
MAKE_FUNCPTR(XRenderCreatePicture)
MAKE_FUNCPTR(XRenderFillRectangle)
MAKE_FUNCPTR(XRenderFindFormat)
MAKE_FUNCPTR(XRenderFindVisualFormat)
MAKE_FUNCPTR(XRenderFreeGlyphSet)
MAKE_FUNCPTR(XRenderFreePicture)
MAKE_FUNCPTR(XRenderSetPictureClipRectangles)
MAKE_FUNCPTR(XRenderQueryExtension)
#undef MAKE_FUNCPTR

static CRITICAL_SECTION xrender_cs = CRITICAL_SECTION_INIT("xrender_cs");


/***********************************************************************
 *   X11DRV_XRender_Init
 *
 * Let's see if our XServer has the extension available
 *
 */
void X11DRV_XRender_Init(void)
{
    int error_base, event_base, i;
    XRenderPictFormat pf;

    if (!wine_dlopen(SONAME_LIBX11, RTLD_NOW|RTLD_GLOBAL, NULL, 0)) return;
    if (!wine_dlopen(SONAME_LIBXEXT, RTLD_NOW|RTLD_GLOBAL, NULL, 0)) return;
    xrender_handle = wine_dlopen(SONAME_LIBXRENDER, RTLD_NOW, NULL, 0);
    if(!xrender_handle) return;

#define LOAD_FUNCPTR(f) if((p##f = wine_dlsym(xrender_handle, #f, NULL, 0)) == NULL) goto sym_not_found;
LOAD_FUNCPTR(XRenderAddGlyphs)
LOAD_FUNCPTR(XRenderCompositeString8)
LOAD_FUNCPTR(XRenderCompositeString16)
LOAD_FUNCPTR(XRenderCompositeString32)
LOAD_FUNCPTR(XRenderCreateGlyphSet)
LOAD_FUNCPTR(XRenderCreatePicture)
LOAD_FUNCPTR(XRenderFillRectangle)
LOAD_FUNCPTR(XRenderFindFormat)
LOAD_FUNCPTR(XRenderFindVisualFormat)
LOAD_FUNCPTR(XRenderFreeGlyphSet)
LOAD_FUNCPTR(XRenderFreePicture)
LOAD_FUNCPTR(XRenderSetPictureClipRectangles)
LOAD_FUNCPTR(XRenderQueryExtension)
#undef LOAD_FUNCPTR

    wine_tsx11_lock();
    if(pXRenderQueryExtension(gdi_display, &event_base, &error_base)) {
        X11DRV_XRender_Installed = TRUE;
	TRACE("Xrender is up and running error_base = %d\n", error_base);
	screen_format = pXRenderFindVisualFormat(gdi_display, visual);
	if(!screen_format) { /* This fails in buggy versions of libXrender.so */
            wine_tsx11_unlock();
	    WINE_MESSAGE(
	     "Wine has detected that you probably have a buggy version\n"
	     "of libXrender.so .  Because of this client side font rendering\n"
	     "will be disabled.  Please upgrade this library.\n");
	    X11DRV_XRender_Installed = FALSE;
	    return;
	}
	pf.type = PictTypeDirect;
	pf.depth = 1;
	pf.direct.alpha = 0;
	pf.direct.alphaMask = 1;
	mono_format = pXRenderFindFormat(gdi_display, PictFormatType |
					  PictFormatDepth | PictFormatAlpha |
					  PictFormatAlphaMask, &pf, 0);
	if(!mono_format) {
            wine_tsx11_unlock();
	    ERR("mono_format == NULL?\n");
	    X11DRV_XRender_Installed = FALSE;
	    return;
	}
	glyphsetCache = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
				  sizeof(*glyphsetCache) * INIT_CACHE_SIZE);

	glyphsetCacheSize = INIT_CACHE_SIZE;
	lastfree = 0;
	for(i = 0; i < INIT_CACHE_SIZE; i++) {
	  glyphsetCache[i].next = i + 1;
	  glyphsetCache[i].count = -1;
	}
	glyphsetCache[i-1].next = -1;
    } else {
        TRACE("Xrender is not available on this server\n");
    }
    wine_tsx11_unlock();
    return;

sym_not_found:
    wine_dlclose(xrender_handle, NULL, 0);
    xrender_handle = NULL;
}

static BOOL fontcmp(LFANDSIZE *p1, LFANDSIZE *p2)
{
  if(p1->hash != p2->hash) return TRUE;
  if(memcmp(&p1->xform, &p2->xform, sizeof(p1->xform))) return TRUE;
  if(memcmp(&p1->lf, &p2->lf, offsetof(LOGFONTW, lfFaceName))) return TRUE;
  return strcmpW(p1->lf.lfFaceName, p2->lf.lfFaceName);
}

#if 0
static void walk_cache(void)
{
  int i;

  EnterCriticalSection(&xrender_cs);
  for(i=mru; i >= 0; i = glyphsetCache[i].next)
    TRACE("item %d\n", i);
  LeaveCriticalSection(&xrender_cs);
}
#endif

static int LookupEntry(LFANDSIZE *plfsz)
{
  int i, prev_i = -1;

  for(i = mru; i >= 0; i = glyphsetCache[i].next) {
    TRACE("%d\n", i);
    if(glyphsetCache[i].count == -1) { /* reached free list so stop */
      i = -1;
      break;
    }

    if(!fontcmp(&glyphsetCache[i].lfsz, plfsz)) {
      glyphsetCache[i].count++;
      if(prev_i >= 0) {
	glyphsetCache[prev_i].next = glyphsetCache[i].next;
	glyphsetCache[i].next = mru;
	mru = i;
      }
      TRACE("found font in cache %d\n", i);
      return i;
    }
    prev_i = i;
  }
  TRACE("font not in cache\n");
  return -1;
}

static int AllocEntry(void)
{
  int best = -1, prev_best = -1, i, prev_i = -1;

  if(lastfree >= 0) {
    assert(glyphsetCache[lastfree].count == -1);
    glyphsetCache[lastfree].count = 1;
    best = lastfree;
    lastfree = glyphsetCache[lastfree].next;
    assert(best != mru);
    glyphsetCache[best].next = mru;
    mru = best;

    TRACE("empty space at %d, next lastfree = %d\n", mru, lastfree);
    return mru;
  }

  for(i = mru; i >= 0; i = glyphsetCache[i].next) {
    if(glyphsetCache[i].count == 0) {
      best = i;
      prev_best = prev_i;
    }
    prev_i = i;
  }

  if(best >= 0) {
    TRACE("freeing unused glyphset at cache %d\n", best);
    wine_tsx11_lock();
    pXRenderFreeGlyphSet(gdi_display, glyphsetCache[best].glyphset);
    wine_tsx11_unlock();
    glyphsetCache[best].glyphset = 0;
    if(glyphsetCache[best].nrealized) { /* do we really want to do this? */
      HeapFree(GetProcessHeap(), 0, glyphsetCache[best].realized);
      glyphsetCache[best].realized = NULL;
      glyphsetCache[best].nrealized = 0;
    }
    glyphsetCache[best].count = 1;
    if(prev_best >= 0) {
      glyphsetCache[prev_best].next = glyphsetCache[best].next;
      glyphsetCache[best].next = mru;
      mru = best;
    } else {
      assert(mru == best);
    }
    return mru;
  }

  TRACE("Growing cache\n");
  glyphsetCache = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			      glyphsetCache,
			      (glyphsetCacheSize + INIT_CACHE_SIZE)
			      * sizeof(*glyphsetCache));
  for(best = i = glyphsetCacheSize; i < glyphsetCacheSize + INIT_CACHE_SIZE;
      i++) {
    glyphsetCache[i].next = i + 1;
    glyphsetCache[i].count = -1;
  }
  glyphsetCache[i-1].next = -1;
  glyphsetCacheSize += INIT_CACHE_SIZE;

  lastfree = glyphsetCache[best].next;
  glyphsetCache[best].count = 1;
  glyphsetCache[best].next = mru;
  mru = best;
  TRACE("new free cache slot at %d\n", mru);
  return mru;
}

static int GetCacheEntry(LFANDSIZE *plfsz)
{
  XRenderPictFormat pf;
  int ret;
  gsCacheEntry *entry;

  if((ret = LookupEntry(plfsz)) != -1) return ret;

  ret = AllocEntry();
  entry = glyphsetCache + ret;
  entry->lfsz = *plfsz;
  assert(entry->nrealized == 0);


  if(antialias) {
    pf.depth = 8;
    pf.direct.alphaMask = 0xff;
  } else {
    pf.depth = 1;
    pf.direct.alphaMask = 1;
  }
  pf.type = PictTypeDirect;
  pf.direct.alpha = 0;

  wine_tsx11_lock();
  entry->font_format = pXRenderFindFormat(gdi_display,
					 PictFormatType |
					 PictFormatDepth |
					 PictFormatAlpha |
					 PictFormatAlphaMask,
					 &pf, 0);

  entry->glyphset = pXRenderCreateGlyphSet(gdi_display, entry->font_format);
  wine_tsx11_unlock();
  return ret;
}

static void dec_ref_cache(int index)
{
  assert(index >= 0);
  TRACE("dec'ing entry %d to %d\n", index, glyphsetCache[index].count - 1);
  assert(glyphsetCache[index].count > 0);
  glyphsetCache[index].count--;
}

static void lfsz_calc_hash(LFANDSIZE *plfsz)
{
  DWORD hash = 0, *ptr;
  int i;

  for(ptr = (DWORD*)&plfsz->xform; ptr < (DWORD*)(&plfsz->xform + 1); ptr++)
    hash ^= *ptr;
  for(i = 0, ptr = (DWORD*)&plfsz->lf; i < 7; i++, ptr++)
    hash ^= *ptr;
  for(i = 0, ptr = (DWORD*)&plfsz->lf.lfFaceName; i < LF_FACESIZE/2; i++, ptr++) {
    WCHAR *pwc = (WCHAR *)ptr;
    if(!*pwc) break;
    hash ^= *ptr;
    pwc++;
    if(!*pwc) break;
  }
  plfsz->hash = hash;
  return;
}

/***********************************************************************
 *   X11DRV_XRender_Finalize
 */
void X11DRV_XRender_Finalize(void)
{
    FIXME("Free cached glyphsets\n");
#if 0
    if (xrender_handle) wine_dlclose(xrender_handle, NULL, 0);
    xrender_handle = NULL;
#endif
}


/***********************************************************************
 *   X11DRV_XRender_SelectFont
 */
BOOL X11DRV_XRender_SelectFont(X11DRV_PDEVICE *physDev, HFONT hfont)
{
    LFANDSIZE lfsz;

    GetObjectW(hfont, sizeof(lfsz.lf), &lfsz.lf);
    TRACE("h=%ld w=%ld weight=%ld it=%d charset=%d name=%s\n",
	  lfsz.lf.lfHeight, lfsz.lf.lfWidth, lfsz.lf.lfWeight,
	  lfsz.lf.lfItalic, lfsz.lf.lfCharSet, debugstr_w(lfsz.lf.lfFaceName));
    lfsz.xform = physDev->dc->xformWorld2Vport;
    lfsz_calc_hash(&lfsz);

    EnterCriticalSection(&xrender_cs);
    if(!physDev->xrender) {
        physDev->xrender = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
				     sizeof(*physDev->xrender));
	physDev->xrender->cache_index = -1;
    }
    else if(physDev->xrender->cache_index != -1)
        dec_ref_cache(physDev->xrender->cache_index);
    physDev->xrender->cache_index = GetCacheEntry(&lfsz);
    LeaveCriticalSection(&xrender_cs);
    return 0;
}

/***********************************************************************
 *   X11DRV_XRender_DeleteDC
 */
void X11DRV_XRender_DeleteDC(X11DRV_PDEVICE *physDev)
{
    wine_tsx11_lock();
    if(physDev->xrender->tile_pict)
        pXRenderFreePicture(gdi_display, physDev->xrender->tile_pict);

    if(physDev->xrender->tile_xpm)
        XFreePixmap(gdi_display, physDev->xrender->tile_xpm);

    if(physDev->xrender->pict) {
	TRACE("freeing pict = %lx dc = %p\n", physDev->xrender->pict, physDev->dc);
        pXRenderFreePicture(gdi_display, physDev->xrender->pict);
    }
    wine_tsx11_unlock();

    EnterCriticalSection(&xrender_cs);
    if(physDev->xrender->cache_index != -1)
        dec_ref_cache(physDev->xrender->cache_index);
    LeaveCriticalSection(&xrender_cs);

    HeapFree(GetProcessHeap(), 0, physDev->xrender);
    physDev->xrender = NULL;
    return;
}

/***********************************************************************
 *   X11DRV_XRender_UpdateDrawable
 *
 * This gets called from X11DRV_SetDrawable and deletes the pict when the
 * drawable changes.  However at the moment we delete the pict at the end of
 * every ExtTextOut so this is basically a NOP.
 */
void X11DRV_XRender_UpdateDrawable(X11DRV_PDEVICE *physDev)
{
    if(physDev->xrender->pict) {
        TRACE("freeing pict %08lx from dc %p\n", physDev->xrender->pict, physDev->dc);
        wine_tsx11_lock();
        pXRenderFreePicture(gdi_display, physDev->xrender->pict);
        wine_tsx11_unlock();
    }
    physDev->xrender->pict = 0;
    return;
}

/************************************************************************
 *   UploadGlyph
 *
 * Helper to ExtTextOut.  Must be called inside xrender_cs
 */
static BOOL UploadGlyph(X11DRV_PDEVICE *physDev, int glyph)
{
    int buflen;
    char *buf;
    Glyph gid;
    GLYPHMETRICS gm;
    XGlyphInfo gi;
    gsCacheEntry *entry = glyphsetCache + physDev->xrender->cache_index;
    UINT ggo_format = GGO_GLYPH_INDEX;
    BOOL aa;

    if(entry->nrealized <= glyph) {
        entry->nrealized = (glyph / 128 + 1) * 128;
	entry->realized = HeapReAlloc(GetProcessHeap(),
				      HEAP_ZERO_MEMORY,
				      entry->realized,
				      entry->nrealized * sizeof(BOOL));
    }

    if(entry->font_format->depth == 8) {
        aa = TRUE;
	ggo_format |= WINE_GGO_GRAY16_BITMAP;
    } else {
        aa = FALSE;
	ggo_format |= GGO_BITMAP;
    }

    buflen = GetGlyphOutlineW(physDev->hdc, glyph, ggo_format, &gm, 0, NULL,
			      NULL);
    if(buflen == GDI_ERROR) {
        LeaveCriticalSection(&xrender_cs);
	return FALSE;
    }

    entry->realized[glyph] = TRUE;

    buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, buflen);
    GetGlyphOutlineW(physDev->hdc, glyph, ggo_format, &gm, buflen, buf, NULL);

    TRACE("buflen = %d. Got metrics: %dx%d adv=%d,%d origin=%ld,%ld\n",
	  buflen,
	  gm.gmBlackBoxX, gm.gmBlackBoxY, gm.gmCellIncX, gm.gmCellIncY,
	  gm.gmptGlyphOrigin.x, gm.gmptGlyphOrigin.y);

    gi.width = gm.gmBlackBoxX;
    gi.height = gm.gmBlackBoxY;
    gi.x = -gm.gmptGlyphOrigin.x;
    gi.y = gm.gmptGlyphOrigin.y;
    gi.xOff = gm.gmCellIncX;
    gi.yOff = gm.gmCellIncY;

    if(TRACE_ON(xrender)) {
        int pitch, i, j;
	char output[300];
	unsigned char *line;

	if(!aa) {
	    pitch = ((gi.width + 31) / 32) * 4;
	    for(i = 0; i < gi.height; i++) {
	        line = buf + i * pitch;
		output[0] = '\0';
		for(j = 0; j < pitch * 8; j++) {
	            strcat(output, (line[j / 8] & (1 << (7 - (j % 8)))) ? "#" : " ");
		}
		strcat(output, "\n");
		TRACE(output);
	    }
	} else {
	    char blks[] = " .:;!o*#";
	    char str[2];

	    str[1] = '\0';
	    pitch = ((gi.width + 3) / 4) * 4;
	    for(i = 0; i < gi.height; i++) {
	        line = buf + i * pitch;
		output[0] = '\0';
		for(j = 0; j < pitch; j++) {
		    str[0] = blks[line[j] >> 5];
		    strcat(output, str);
		}
		strcat(output, "\n");
		TRACE(output);
	    }
	}
    }

    if(!aa && BitmapBitOrder(gdi_display) != MSBFirst) {
        unsigned char *byte = buf, c;
	int i = buflen;

	while(i--) {
	    c = *byte;

	    /* magic to flip bit order */
	    c = ((c << 1) & 0xaa) | ((c >> 1) & 0x55);
	    c = ((c << 2) & 0xcc) | ((c >> 2) & 0x33);
	    c = ((c << 4) & 0xf0) | ((c >> 4) & 0x0f);

	    *byte++ = c;
	}
    }
    gid = glyph;
    wine_tsx11_lock();
    pXRenderAddGlyphs(gdi_display, entry->glyphset, &gid, &gi, 1,
		       buf, buflen);
    wine_tsx11_unlock();
    HeapFree(GetProcessHeap(), 0, buf);
    return TRUE;
}

/***********************************************************************
 *   X11DRV_XRender_ExtTextOut
 */
BOOL X11DRV_XRender_ExtTextOut( X11DRV_PDEVICE *physDev, INT x, INT y, UINT flags,
				const RECT *lprect, LPCWSTR wstr, UINT count,
				const INT *lpDx )
{
    XRenderColor col;
    int idx;
    TEXTMETRICW tm;
    RGNDATA *data;
    SIZE sz;
    RECT rc;
    BOOL done_extents = FALSE;
    INT width, xwidth, ywidth;
    double cosEsc, sinEsc;
    XGCValues xgcval;
    LOGFONTW lf;
    int render_op = PictOpOver;
    WORD *glyphs;
    POINT pt;
    gsCacheEntry *entry;
    BOOL retv = FALSE;
    HDC hdc = physDev->hdc;
    DC *dc = physDev->dc;

    TRACE("%04x, %d, %d, %08x, %p, %s, %d, %p)\n", hdc, x, y, flags,
	  lprect, debugstr_wn(wstr, count), count, lpDx);

    if(flags & ETO_GLYPH_INDEX)
        glyphs = (LPWORD)wstr;
    else {
        glyphs = HeapAlloc(GetProcessHeap(), 0, count * sizeof(WCHAR));
        GetGlyphIndicesW(hdc, wstr, count, glyphs, 0);
    }

    if(lprect)
      TRACE("rect: %d,%d - %d,%d\n", lprect->left, lprect->top, lprect->right,
	    lprect->bottom);
    TRACE("align = %x bkmode = %x mapmode = %x\n", dc->textAlign, GetBkMode(hdc), dc->MapMode);

    if(dc->textAlign & TA_UPDATECP) {
        x = dc->CursPosX;
	y = dc->CursPosY;
    }

    GetObjectW(GetCurrentObject(hdc, OBJ_FONT), sizeof(lf), &lf);
    if(lf.lfEscapement != 0) {
        cosEsc = cos(lf.lfEscapement * M_PI / 1800);
	sinEsc = sin(lf.lfEscapement * M_PI / 1800);
    } else {
        cosEsc = 1;
	sinEsc = 0;
    }

    if(flags & (ETO_CLIPPED | ETO_OPAQUE)) {
        if(!lprect) {
	    if(flags & ETO_CLIPPED) return FALSE;
	        GetTextExtentPointI(hdc, glyphs, count, &sz);
		done_extents = TRUE;
		rc.left = x;
		rc.top = y;
		rc.right = x + sz.cx;
		rc.bottom = y + sz.cy;
	} else {
	    rc = *lprect;
	}

	LPtoDP(physDev->hdc, (POINT*)&rc, 2);

	if(rc.left > rc.right) {INT tmp = rc.left; rc.left = rc.right; rc.right = tmp;}
	if(rc.top > rc.bottom) {INT tmp = rc.top; rc.top = rc.bottom; rc.bottom = tmp;}
    }

    xgcval.function = GXcopy;
    xgcval.background = physDev->backgroundPixel;
    xgcval.fill_style = FillSolid;
    TSXChangeGC( gdi_display, physDev->gc,
		 GCFunction | GCBackground | GCFillStyle, &xgcval );

    X11DRV_LockDIBSection( physDev, DIB_Status_GdiMod, FALSE );

    if(flags & ETO_OPAQUE) {
        TSXSetForeground( gdi_display, physDev->gc, physDev->backgroundPixel );
	TSXFillRectangle( gdi_display, physDev->drawable, physDev->gc,
			  physDev->org.x + rc.left, physDev->org.y + rc.top,
			  rc.right - rc.left, rc.bottom - rc.top );
    }

    if(count == 0) {
	retv =  TRUE;
        goto done;
    }

    pt.x = x;
    pt.y = y;
    LPtoDP(physDev->hdc, &pt, 1);
    x = pt.x;
    y = pt.y;

    TRACE("real x,y %d,%d\n", x, y);

    if(lpDx) {
	width = 0;
	for(idx = 0; idx < count; idx++)
	    width += lpDx[idx];
    } else {
	if(!done_extents) {
	    GetTextExtentPointI(hdc, glyphs, count, &sz);
	    done_extents = TRUE;
	}
	width = sz.cx;
    }
    width = INTERNAL_XWSTODS(dc, width);
    xwidth = width * cosEsc;
    ywidth = width * sinEsc;

    GetTextMetricsW(hdc, &tm);

    tm.tmAscent = INTERNAL_YWSTODS(dc, tm.tmAscent);
    tm.tmDescent = INTERNAL_YWSTODS(dc, tm.tmDescent);
    switch( dc->textAlign & (TA_LEFT | TA_RIGHT | TA_CENTER) ) {
    case TA_LEFT:
        if (dc->textAlign & TA_UPDATECP) {
	    pt.x = x + xwidth;
	    pt.y = y - ywidth;
	    DPtoLP(physDev->hdc, &pt, 1);
	    dc->CursPosX = pt.x;
	    dc->CursPosY = pt.y;
	}
	break;

    case TA_CENTER:
        x -= xwidth / 2;
	y += ywidth / 2;
	break;

    case TA_RIGHT:
        x -= xwidth;
	y += ywidth;
	if (dc->textAlign & TA_UPDATECP) {
	    pt.x = x;
	    pt.y = y;
	    DPtoLP(physDev->hdc, &pt, 1);
	    dc->CursPosX = pt.x;
	    dc->CursPosY = pt.y;
	}
	break;
    }

    switch( dc->textAlign & (TA_TOP | TA_BOTTOM | TA_BASELINE) ) {
    case TA_TOP:
        y += tm.tmAscent * cosEsc;
	x += tm.tmAscent * sinEsc;
	break;

    case TA_BOTTOM:
        y -= tm.tmDescent * cosEsc;
	x -= tm.tmDescent * sinEsc;
	break;

    case TA_BASELINE:
        break;
    }

    if (flags & ETO_CLIPPED)
    {
        SaveVisRgn16( HDC_16(hdc) );
        IntersectVisRect16( HDC_16(dc->hSelf), lprect->left, lprect->top, lprect->right, lprect->bottom );
    }

    if(!physDev->xrender->pict) {
        XRenderPictureAttributes pa;
	pa.subwindow_mode = IncludeInferiors;

        wine_tsx11_lock();
        physDev->xrender->pict = pXRenderCreatePicture(gdi_display,
                                                       physDev->drawable,
                                                       (dc->bitsPerPixel == 1) ?
                                                       mono_format : screen_format,
                                                       CPSubwindowMode, &pa);
        wine_tsx11_unlock();

	TRACE("allocing pict = %lx dc = %p drawable = %08lx\n", physDev->xrender->pict, dc, physDev->drawable);
    } else {
	TRACE("using existing pict = %lx dc = %p\n", physDev->xrender->pict, dc);
    }

    if ((data = X11DRV_GetRegionData( dc->hGCClipRgn, 0 )))
    {
        wine_tsx11_lock();
        pXRenderSetPictureClipRectangles( gdi_display, physDev->xrender->pict,
                                          physDev->org.x, physDev->org.y,
                                          (XRectangle *)data->Buffer, data->rdh.nCount );
        wine_tsx11_unlock();
        HeapFree( GetProcessHeap(), 0, data );
    }

    if(GetBkMode(hdc) != TRANSPARENT) {
        if(!((flags & ETO_CLIPPED) && (flags & ETO_OPAQUE))) {
	    if(!(flags & ETO_OPAQUE) || x < rc.left || x + width >= rc.right ||
	       y - tm.tmAscent < rc.top || y + tm.tmDescent >= rc.bottom) {
	        TSXSetForeground( gdi_display, physDev->gc, physDev->backgroundPixel );
		TSXFillRectangle( gdi_display, physDev->drawable, physDev->gc,
				  physDev->org.x + x, physDev->org.y + y - tm.tmAscent,
				  width, tm.tmAscent + tm.tmDescent );
	    }
	}
    }

    /* Create a 1x1 pixmap to tile over the font mask */
    if(!physDev->xrender->tile_xpm) {
        XRenderPictureAttributes pa;

      	XRenderPictFormat *format = (dc->bitsPerPixel == 1) ? mono_format : screen_format;
        wine_tsx11_lock();
	physDev->xrender->tile_xpm = XCreatePixmap(gdi_display,
						     physDev->drawable,
						     1, 1,
						     format->depth);
	pa.repeat = True;
	physDev->xrender->tile_pict = pXRenderCreatePicture(gdi_display,
							     physDev->xrender->tile_xpm,
							     format,
							     CPRepeat, &pa);
        wine_tsx11_unlock();
	TRACE("Created pixmap of depth %d\n", format->depth);
	/* init lastTextColor to something different from dc->textColor */
	physDev->xrender->lastTextColor = ~dc->textColor;

    }

    if(dc->textColor != physDev->xrender->lastTextColor) {
        if(dc->bitsPerPixel != 1) {
	    /* Map 0 -- 0xff onto 0 -- 0xffff */
	    col.red = GetRValue(dc->textColor);
	    col.red |= col.red << 8;
	    col.green = GetGValue(dc->textColor);
	    col.green |= col.green << 8;
	    col.blue = GetBValue(dc->textColor);
	    col.blue |= col.blue << 8;
	    col.alpha = 0x0;
	} else { /* for a 1bpp bitmap we always need a 1 in the tile */
	    col.red = col.green = col.blue = 0;
	    col.alpha = 0xffff;
	}
        wine_tsx11_lock();
	pXRenderFillRectangle(gdi_display, PictOpSrc,
			       physDev->xrender->tile_pict,
			       &col, 0, 0, 1, 1);
        wine_tsx11_unlock();
	physDev->xrender->lastTextColor = dc->textColor;
    }

    /* FIXME the mapping of Text/BkColor onto 1 or 0 needs investigation.
     */
    if((dc->bitsPerPixel == 1) && ((dc->textColor & 0xffffff) == 0))
        render_op = PictOpOutReverse; /* This gives us 'black' text */

    EnterCriticalSection(&xrender_cs);
    entry = glyphsetCache + physDev->xrender->cache_index;

    for(idx = 0; idx < count; idx++) {
        if(glyphs[idx] >= entry->nrealized || entry->realized[glyphs[idx]] == FALSE) {
	    UploadGlyph(physDev, glyphs[idx]);
	}
    }


    TRACE("Writing %s at %ld,%ld\n", debugstr_wn(wstr,count),
          physDev->org.x + x, physDev->org.y + y);

    wine_tsx11_lock();
    if(!lpDx)
        pXRenderCompositeString16(gdi_display, render_op,
				   physDev->xrender->tile_pict,
				   physDev->xrender->pict,
				   entry->font_format, entry->glyphset,
				   0, 0, physDev->org.x + x, physDev->org.y + y,
				   glyphs, count);

    else {
        INT offset = 0, xoff = 0, yoff = 0;
	for(idx = 0; idx < count; idx++) {
	    pXRenderCompositeString16(gdi_display, render_op,
				       physDev->xrender->tile_pict,
				       physDev->xrender->pict,
				       entry->font_format, entry->glyphset,
				       0, 0, physDev->org.x + x + xoff,
				       physDev->org.y + y + yoff,
				       glyphs + idx, 1);
	    offset += INTERNAL_XWSTODS(dc, lpDx[idx]);
	    xoff = offset * cosEsc;
	    yoff = offset * -sinEsc;
	}
    }

    LeaveCriticalSection(&xrender_cs);

    if(physDev->xrender->pict) {
        pXRenderFreePicture(gdi_display, physDev->xrender->pict);
    }
    physDev->xrender->pict = 0;
    wine_tsx11_unlock();

    if (flags & ETO_CLIPPED)
        RestoreVisRgn16( HDC_16(hdc) );

    retv = TRUE;

done:
    X11DRV_UnlockDIBSection( physDev, TRUE );
    if(glyphs != wstr) HeapFree(GetProcessHeap(), 0, glyphs);
    return retv;
}

#else /* HAVE_X11_EXTENSIONS_XRENDER_H */

void X11DRV_XRender_Init(void)
{
    TRACE("XRender support not compiled in.\n");
    return;
}

void X11DRV_XRender_Finalize(void)
{
}

BOOL X11DRV_XRender_SelectFont(X11DRV_PDEVICE *physDev, HFONT hfont)
{
  assert(0);
  return FALSE;
}

void X11DRV_XRender_DeleteDC(X11DRV_PDEVICE *physDev)
{
  assert(0);
  return;
}

BOOL X11DRV_XRender_ExtTextOut( X11DRV_PDEVICE *physDev, INT x, INT y, UINT flags,
				const RECT *lprect, LPCWSTR wstr, UINT count,
				const INT *lpDx )
{
  assert(0);
  return FALSE;
}

void X11DRV_XRender_UpdateDrawable(X11DRV_PDEVICE *physDev)
{
  assert(0);
  return;
}

#endif /* HAVE_X11_EXTENSIONS_XRENDER_H */
