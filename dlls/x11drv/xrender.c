/*
 * Functions to use the XRender extension
 *
 * Copyright 2001 Huw D M Davies for CodeWeavers
 */
#include "config.h"

#include "winnt.h"
#include "x11drv.h"
#include "bitmap.h"
#include "debugtools.h"
#include "region.h"
#include <string.h>
#include <stdlib.h>
#include "wine/unicode.h"
#include <assert.h>

BOOL X11DRV_XRender_Installed = FALSE;
DEFAULT_DEBUG_CHANNEL(xrender);

#ifdef HAVE_LIBXRENDER

#include "ts_xlib.h"
#include "ts_xrender.h"

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
    gsCacheEntry       *cacheEntry;
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

    if(TSXRenderQueryExtension(gdi_display, &event_base, &error_base)) {
        X11DRV_XRender_Installed = TRUE;
	TRACE("Xrender is up and running error_base = %d\n", error_base);
	screen_format = TSXRenderFindVisualFormat(gdi_display, visual);
	pf.type = PictTypeDirect;
	pf.depth = 1;
	pf.direct.alpha = 0;
	pf.direct.alphaMask = 1;
	mono_format = TSXRenderFindFormat(gdi_display, PictFormatType |
					  PictFormatDepth | PictFormatAlpha |
					  PictFormatAlphaMask, &pf, 0);

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
    return;
}

static BOOL fontcmp(LFANDSIZE *p1, LFANDSIZE *p2)
{
  if(p1->hash != p2->hash) return TRUE;
  if(memcmp(&p1->xform, &p2->xform, sizeof(p1->xform))) return TRUE;
  if(memcmp(&p1->lf, &p2->lf, offsetof(LOGFONTW, lfFaceName))) return TRUE;
  return strcmpW(p1->lf.lfFaceName, p2->lf.lfFaceName);
}

static void walk_cache(void)
{
  int i;

  for(i=mru; i >= 0; i = glyphsetCache[i].next)
    TRACE("item %d\n", i);
}

static gsCacheEntry *LookupEntry(LFANDSIZE *plfsz)
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
      return glyphsetCache + i;
    }
    prev_i = i;
  }
  TRACE("font not in cache\n");
  return NULL;
}

static gsCacheEntry *AllocEntry(void)
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
    return glyphsetCache + mru;
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
    TSXRenderFreeGlyphSet(gdi_display, glyphsetCache[best].glyphset);
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
    return glyphsetCache + mru;
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
  return glyphsetCache + mru;
}    

static gsCacheEntry *GetCacheEntry(LFANDSIZE *plfsz)
{
  XRenderPictFormat pf;
  gsCacheEntry *ret;

  if((ret = LookupEntry(plfsz)) != NULL) return ret;

  ret = AllocEntry();
  ret->lfsz = *plfsz;
  assert(ret->nrealized == 0);


  if(antialias && abs(plfsz->lf.lfHeight) > 16) {
    pf.depth = 8;
    pf.direct.alphaMask = 0xff;
  } else {
    pf.depth = 1;
    pf.direct.alphaMask = 1;
  }
  pf.type = PictTypeDirect;
  pf.direct.alpha = 0;

  ret->font_format = TSXRenderFindFormat(gdi_display,
					 PictFormatType |
					 PictFormatDepth |
					 PictFormatAlpha |
					 PictFormatAlphaMask,
					 &pf, 0);

  ret->glyphset = TSXRenderCreateGlyphSet(gdi_display, ret->font_format);
  return ret;
}

static void dec_ref_cache(gsCacheEntry *entry)
{
  TRACE("dec'ing entry %d to %d\n", entry - glyphsetCache, entry->count - 1);
  assert(entry->count > 0);
  entry->count--;
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
    return;
}


/***********************************************************************
 *   X11DRV_XRender_SelectFont
 */
BOOL X11DRV_XRender_SelectFont(DC *dc, HFONT hfont)
{
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;
    LFANDSIZE lfsz;

    GetObjectW(hfont, sizeof(lfsz.lf), &lfsz.lf);
    TRACE("h=%ld w=%ld weight=%ld it=%d charset=%d name=%s\n",
	  lfsz.lf.lfHeight, lfsz.lf.lfWidth, lfsz.lf.lfWeight,
	  lfsz.lf.lfItalic, lfsz.lf.lfCharSet, debugstr_w(lfsz.lf.lfFaceName));
    lfsz.xform = dc->xformWorld2Vport;
    lfsz_calc_hash(&lfsz);

    if(!physDev->xrender)
        physDev->xrender = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
				     sizeof(*physDev->xrender));

    else if(physDev->xrender->cacheEntry)
        dec_ref_cache(physDev->xrender->cacheEntry);
    physDev->xrender->cacheEntry = GetCacheEntry(&lfsz);
    return 0;
}

/***********************************************************************
 *   X11DRV_XRender_DeleteDC
 */
void X11DRV_XRender_DeleteDC(DC *dc)
{
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;
    
    if(physDev->xrender->tile_pict)
        TSXRenderFreePicture(gdi_display, physDev->xrender->tile_pict);

    if(physDev->xrender->tile_xpm)
        TSXFreePixmap(gdi_display, physDev->xrender->tile_xpm);

    if(physDev->xrender->pict) {
	TRACE("freeing pict = %lx dc = %p\n", physDev->xrender->pict, dc);
        TSXRenderFreePicture(gdi_display, physDev->xrender->pict);
    }

    if(physDev->xrender->cacheEntry)
        dec_ref_cache(physDev->xrender->cacheEntry);
    
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
void X11DRV_XRender_UpdateDrawable(DC *dc)
{
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;

    if(physDev->xrender->pict) {
        TRACE("freeing pict %08lx from dc %p\n", physDev->xrender->pict, dc);
        TSXRenderFreePicture(gdi_display, physDev->xrender->pict);
    }
    physDev->xrender->pict = 0;
    return;
}

static BOOL UploadGlyph(DC *dc, WCHAR glyph)
{
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;
    int buflen;
    char *buf;
    Glyph gid;
    GLYPHMETRICS gm;
    XGlyphInfo gi;
    gsCacheEntry *entry = physDev->xrender->cacheEntry;
    UINT ggo_format;
    BOOL aa;

    if(entry->nrealized <= glyph) {
        entry->nrealized = (glyph / 128 + 1) * 128;
	entry->realized = HeapReAlloc(GetProcessHeap(),
				      HEAP_ZERO_MEMORY,
				      entry->realized,
				      entry->nrealized * sizeof(BOOL));
    }
    entry->realized[glyph] = TRUE;

    if(entry->font_format->depth == 8) {
        aa = TRUE;
	ggo_format = WINE_GGO_GRAY16_BITMAP;
    } else {
        aa = FALSE;
	ggo_format = GGO_BITMAP;
    }

    buflen = GetGlyphOutlineW(dc->hSelf, glyph, ggo_format, &gm, 0, NULL,
			      NULL);
    if(buflen == GDI_ERROR)
        return FALSE;

    buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, buflen);
    GetGlyphOutlineW(dc->hSelf, glyph, ggo_format, &gm, buflen, buf, NULL);

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
    TSXRenderAddGlyphs(gdi_display, entry->glyphset, &gid, &gi, 1,
		       buf, buflen);
    HeapFree(GetProcessHeap(), 0, buf);
    return TRUE;
}

/***********************************************************************
 *   X11DRV_XRender_ExtTextOut
 */
BOOL X11DRV_XRender_ExtTextOut( DC *dc, INT x, INT y, UINT flags,
				const RECT *lprect, LPCWSTR wstr, UINT count,
				const INT *lpDx )
{
    XRenderColor col;
    int idx;
    TEXTMETRICW tm;
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;
    RGNOBJ *obj;
    XRectangle *pXrect;
    SIZE sz;
    RECT rc;
    BOOL done_extents = FALSE;
    INT width, xwidth, ywidth;
    double cosEsc, sinEsc;
    XGCValues xgcval;
    LOGFONTW lf;
    int render_op = PictOpOver;

    TRACE("%04x, %d, %d, %08x, %p, %s, %d, %p)\n", dc->hSelf, x, y, flags,
	  lprect, debugstr_wn(wstr, count), count, lpDx);
    if(lprect)
      TRACE("rect: %d,%d - %d,%d\n", lprect->left, lprect->top, lprect->right,
	    lprect->bottom);
    TRACE("align = %x bkmode = %x mapmode = %x\n", dc->textAlign,
	  dc->backgroundMode,
	  dc->MapMode);

    if(dc->textAlign & TA_UPDATECP) {
        x = dc->CursPosX;
	y = dc->CursPosY;
    }

    GetObjectW(GetCurrentObject(dc->hSelf, OBJ_FONT), sizeof(lf), &lf);
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
	        GetTextExtentPointW(dc->hSelf, wstr, count, &sz);
		done_extents = TRUE;
		rc.left = x;
		rc.top = y;
		rc.right = x + sz.cx;
		rc.bottom = y + sz.cy;
	} else {
	    rc = *lprect;
	}

	rc.left = INTERNAL_XWPTODP(dc, rc.left, rc.top);
	rc.top = INTERNAL_YWPTODP(dc, rc.left, rc.top);
	rc.right = INTERNAL_XWPTODP(dc, rc.right, rc.bottom);
	rc.bottom = INTERNAL_YWPTODP(dc, rc.right, rc.bottom);

	if(rc.left > rc.right) {INT tmp = rc.left; rc.left = rc.right; rc.right = tmp;}
	if(rc.top > rc.bottom) {INT tmp = rc.top; rc.top = rc.bottom; rc.bottom = tmp;}
    }

    xgcval.function = GXcopy;
    xgcval.background = physDev->backgroundPixel;
    xgcval.fill_style = FillSolid;
    TSXChangeGC( gdi_display, physDev->gc,
		 GCFunction | GCBackground | GCFillStyle, &xgcval );

    X11DRV_LockDIBSection( dc, DIB_Status_GdiMod, FALSE );

    if(flags & ETO_OPAQUE) {
        TSXSetForeground( gdi_display, physDev->gc, physDev->backgroundPixel );
	TSXFillRectangle( gdi_display, physDev->drawable, physDev->gc,
			  dc->DCOrgX + rc.left, dc->DCOrgY + rc.top,
			  rc.right - rc.left, rc.bottom - rc.top );
    }

    if(count == 0) {
        X11DRV_UnlockDIBSection( dc, TRUE );
	return TRUE;
    }

    x = INTERNAL_XWPTODP( dc, x, y );
    y = INTERNAL_YWPTODP( dc, x, y );

    TRACE("real x,y %d,%d\n", x, y);

    if(lpDx) {
	width = 0;
	for(idx = 0; idx < count; idx++)
	    width += lpDx[idx];
    } else {
	if(!done_extents) {
	    GetTextExtentPointW(dc->hSelf, wstr, count, &sz);
	    done_extents = TRUE;
	}
	width = sz.cx;
    }
    width = INTERNAL_XWSTODS(dc, width);
    xwidth = width * cosEsc;
    ywidth = width * sinEsc;

    GetTextMetricsW(dc->hSelf, &tm);

    switch( dc->textAlign & (TA_LEFT | TA_RIGHT | TA_CENTER) ) {
    case TA_LEFT:
        if (dc->textAlign & TA_UPDATECP) {
	    dc->CursPosX = INTERNAL_XDPTOWP( dc, x + xwidth, y - ywidth );
	    dc->CursPosY = INTERNAL_YDPTOWP( dc, x + xwidth, y - ywidth );
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
	    dc->CursPosX = INTERNAL_XDPTOWP( dc, x + xwidth, y - ywidth );
	    dc->CursPosY = INTERNAL_YDPTOWP( dc, x + xwidth, y - ywidth );
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
        SaveVisRgn16( dc->hSelf );
        CLIPPING_IntersectVisRect( dc, rc.left, rc.top, rc.right,
                                   rc.bottom, FALSE );
    }



    if(!physDev->xrender->pict) {
        XRenderPictureAttributes pa;
	pa.subwindow_mode = IncludeInferiors;

        physDev->xrender->pict =
	  TSXRenderCreatePicture(gdi_display,
				 physDev->drawable,
				 (dc->bitsPerPixel == 1) ?
				   mono_format : screen_format,
				 CPSubwindowMode, &pa);

	TRACE("allocing pict = %lx dc = %p drawable = %08lx\n", physDev->xrender->pict, dc, physDev->drawable);
    } else {
	TRACE("using existing pict = %lx dc = %p\n", physDev->xrender->pict, dc);
    }

    obj = (RGNOBJ *) GDI_GetObjPtr(dc->hGCClipRgn, REGION_MAGIC);
    if (!obj)
    {
        ERR("Rgn is 0. Please report this.\n");
	return FALSE;
    }
    
    if (obj->rgn->numRects > 0)
    {
        XRectangle *pXr;
        RECT *pRect = obj->rgn->rects;
        RECT *pEndRect = obj->rgn->rects + obj->rgn->numRects;

        pXrect = HeapAlloc( GetProcessHeap(), 0, 
			    sizeof(*pXrect) * obj->rgn->numRects );
	if(!pXrect)
	{
	    WARN("Can't alloc buffer\n");
	    GDI_ReleaseObj( dc->hGCClipRgn );
	    return FALSE;
	}

        for(pXr = pXrect; pRect < pEndRect; pRect++, pXr++)
        {
            pXr->x = pRect->left;
            pXr->y = pRect->top;
            pXr->width = pRect->right - pRect->left;
            pXr->height = pRect->bottom - pRect->top;
	    TRACE("Adding clip rect %d,%d - %d,%d\n", pRect->left, pRect->top,
		  pRect->right, pRect->bottom);
        }
    }
    else {
        TRACE("no clip rgn\n");
        pXrect = NULL;
    }

    TSXRenderSetPictureClipRectangles( gdi_display, physDev->xrender->pict,
				       0, 0, pXrect, obj->rgn->numRects );

    if(pXrect)
        HeapFree( GetProcessHeap(), 0, pXrect );

    GDI_ReleaseObj( dc->hGCClipRgn );

    if(dc->backgroundMode != TRANSPARENT) {
        if(!((flags & ETO_CLIPPED) && (flags & ETO_OPAQUE))) {
	    if(!(flags & ETO_OPAQUE) || x < rc.left || x + width >= rc.right ||
	       y - tm.tmAscent < rc.top || y + tm.tmDescent >= rc.bottom) {
	        TSXSetForeground( gdi_display, physDev->gc, physDev->backgroundPixel );
		TSXFillRectangle( gdi_display, physDev->drawable, physDev->gc,
				  dc->DCOrgX + x, dc->DCOrgY + y - tm.tmAscent,
				  width, tm.tmAscent + tm.tmDescent );
	    }
	}
    }

    /* Create a 1x1 pixmap to tile over the font mask */
    if(!physDev->xrender->tile_xpm) {
        XRenderPictureAttributes pa;

      	XRenderPictFormat *format = (dc->bitsPerPixel == 1) ? mono_format : screen_format;
	physDev->xrender->tile_xpm = TSXCreatePixmap(gdi_display,
						     physDev->drawable,
						     1, 1,
						     format->depth);
	pa.repeat = True;
	physDev->xrender->tile_pict = TSXRenderCreatePicture(gdi_display,
							     physDev->xrender->tile_xpm,
							     format,
							     CPRepeat, &pa);
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
	TSXRenderFillRectangle(gdi_display, PictOpSrc,
			       physDev->xrender->tile_pict,
			       &col, 0, 0, 1, 1);
	physDev->xrender->lastTextColor = dc->textColor;
    }

    /* FIXME the mapping of Text/BkColor onto 1 or 0 needs investigation.
     */
    if((dc->bitsPerPixel == 1) && ((dc->textColor & 0xffffff) == 0))
        render_op = PictOpOutReverse; /* This gives us 'black' text */
    
    for(idx = 0; idx < count; idx++) {
        if(wstr[idx] >= physDev->xrender->cacheEntry->nrealized ||
	   physDev->xrender->cacheEntry->realized[wstr[idx]] == FALSE) {
	    UploadGlyph(dc, wstr[idx]);
	}
    }


    TRACE("Writing %s at %d,%d\n", debugstr_wn(wstr,count), dc->DCOrgX + x,
	  dc->DCOrgY + y);
    if(!lpDx)
        TSXRenderCompositeString16(gdi_display, render_op, 
				   physDev->xrender->tile_pict,
				   physDev->xrender->pict,
				   physDev->xrender->cacheEntry->font_format,
				   physDev->xrender->cacheEntry->glyphset,
				   0, 0, dc->DCOrgX + x, dc->DCOrgY + y, wstr,
				   count);

    else {
        INT offset = 0, xoff = 0, yoff = 0;
	for(idx = 0; idx < count; idx++) {
	    TSXRenderCompositeString16(gdi_display, render_op, 
				       physDev->xrender->tile_pict,
				       physDev->xrender->pict,
				       physDev->xrender->cacheEntry->font_format,
				       physDev->xrender->cacheEntry->glyphset,
				       0, 0, dc->DCOrgX + x + xoff,
				       dc->DCOrgY + y + yoff,
				       wstr + idx, 1);
	    offset += INTERNAL_XWSTODS(dc, lpDx[idx]);
	    xoff = offset * cosEsc;
	    yoff = offset * sinEsc;
	}
    }

    if(physDev->xrender->pict) {
        TSXRenderFreePicture(gdi_display, physDev->xrender->pict);
    }
    physDev->xrender->pict = 0;


    if (flags & ETO_CLIPPED) 
        RestoreVisRgn16( dc->hSelf );

    X11DRV_UnlockDIBSection( dc, TRUE );
    return TRUE;
}

#else /* #ifdef HAVE_LIBXRENDER */

void X11DRV_XRender_Init(void)
{
    TRACE("XRender support not compiled in.\n");
    return;
}

void X11DRV_XRender_Finalize(void)
{
  assert(0);
  return;
}

BOOL X11DRV_XRender_SelectFont(DC *dc, HFONT hfont)
{
  assert(0);
  return FALSE;
}

void X11DRV_XRender_DeleteDC(DC *dc)
{
  assert(0);
  return;
}

BOOL X11DRV_XRender_ExtTextOut( DC *dc, INT x, INT y, UINT flags,
				const RECT *lprect, LPCWSTR wstr, UINT count,
				const INT *lpDx )
{
  assert(0);
  return FALSE;
}

void X11DRV_XRender_UpdateDrawable(DC *dc)
{
  assert(0);
  return;
}

#endif
