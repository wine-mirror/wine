/*
 * X11 graphics driver text functions
 *
 * Copyright 1993,1994 Alexandre Julliard
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

#include <X11/Xatom.h>

#include "ts_xlib.h"

#include <stdarg.h>
#include <stdlib.h>
#include <math.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wownt32.h"
#include "gdi.h"
#include "x11font.h"
#include "bitmap.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(text);

#define SWAP_INT(a,b)  { int t = a; a = b; b = t; }
#define IROUND(x) (int)((x)>0? (x)+0.5 : (x) - 0.5)


/***********************************************************************
 *           X11DRV_ExtTextOut
 */
BOOL
X11DRV_ExtTextOut( X11DRV_PDEVICE *physDev, INT x, INT y, UINT flags,
                   const RECT *lprect, LPCWSTR wstr, UINT count,
                   const INT *lpDx )
{
    int 	        i;
    fontObject*		pfo;
    INT	 	        width, ascent, descent, xwidth, ywidth;
    XFontStruct*	font;
    RECT 		rect;
    char		dfBreakChar, lfUnderline, lfStrikeOut;
    BOOL		rotated = FALSE;
    XChar2b		*str2b = NULL;
    BOOL		dibUpdateFlag = FALSE;
    BOOL                result = TRUE;
    POINT               pt;
    DC *dc = physDev->dc;

    if(dc->gdiFont)
        return X11DRV_XRender_ExtTextOut(physDev, x, y, flags, lprect, wstr, count,
					 lpDx);


    if (!X11DRV_SetupGCForText( physDev )) return TRUE;

    pfo = XFONT_GetFontObject( physDev->font );
    font = pfo->fs;

    if (pfo->lf.lfEscapement && pfo->lpX11Trans)
        rotated = TRUE;
    dfBreakChar = (char)pfo->fi->df.dfBreakChar;
    lfUnderline = (pfo->fo_flags & FO_SYNTH_UNDERLINE) ? 1 : 0;
    lfStrikeOut = (pfo->fo_flags & FO_SYNTH_STRIKEOUT) ? 1 : 0;

    TRACE("hdc=%p df=%04x %d,%d %s, %d  flags=%d lpDx=%p\n",
	  dc->hSelf, (UINT16)(physDev->font), x, y,
	  debugstr_wn (wstr, count), count, flags, lpDx);

    /* some strings sent here end in a newline for whatever reason.  I have no
       clue what the right treatment should be in general, but ignoring
       terminating newlines seems ok.  MW, April 1998.  */
    if (count > 0 && wstr[count - 1] == '\n') count--;

    if (lprect != NULL) TRACE("\trect=(%ld,%ld - %ld,%ld)\n",
                                     lprect->left, lprect->top,
                                     lprect->right, lprect->bottom );
      /* Setup coordinates */

    if (dc->textAlign & TA_UPDATECP)
    {
	x = dc->CursPosX;
	y = dc->CursPosY;
    }

    if (flags & (ETO_OPAQUE | ETO_CLIPPED))  /* there's a rectangle */
    {
        if (!lprect)  /* not always */
        {
            SIZE sz;
            if (flags & ETO_CLIPPED)  /* Can't clip with no rectangle */
	      return FALSE;
	    if (!X11DRV_GetTextExtentPoint( physDev, wstr, count, &sz ))
	      return FALSE;
	    rect.left   = x;
	    rect.right  = x + sz.cx;
	    rect.top    = y;
	    rect.bottom = y + sz.cy;
	}
	else
	{
	    rect = *lprect;
	}
	LPtoDP(physDev->hdc, (POINT*)&rect, 2);

	if (rect.right < rect.left) SWAP_INT( rect.left, rect.right );
	if (rect.bottom < rect.top) SWAP_INT( rect.top, rect.bottom );
    }

    pt.x = x;
    pt.y = y;
    LPtoDP(physDev->hdc, &pt, 1);
    x = pt.x;
    y = pt.y;

    TRACE("\treal coord: x=%i, y=%i, rect=(%ld,%ld - %ld,%ld)\n",
			  x, y, rect.left, rect.top, rect.right, rect.bottom);

      /* Draw the rectangle */

    if (flags & ETO_OPAQUE)
    {
        X11DRV_LockDIBSection( physDev, DIB_Status_GdiMod, FALSE );
        dibUpdateFlag = TRUE;
        TSXSetForeground( gdi_display, physDev->gc, physDev->backgroundPixel );
        TSXFillRectangle( gdi_display, physDev->drawable, physDev->gc,
                          physDev->org.x + rect.left, physDev->org.y + rect.top,
                          rect.right-rect.left, rect.bottom-rect.top );
    }
    if (!count) goto END;  /* Nothing more to do */

      /* Compute text starting position */

    if (lpDx) /* have explicit character cell x offsets in logical coordinates */
    {
        for (i = width = 0; i < count; i++) width += lpDx[i];
        width = INTERNAL_XWSTODS(dc, width);
    }
    else
    {
        SIZE sz;
        if (!X11DRV_GetTextExtentPoint( physDev, wstr, count, &sz ))
        {
            result = FALSE;
            goto END;
        }
	width = INTERNAL_XWSTODS(dc, sz.cx);
    }
    ascent = pfo->lpX11Trans ? pfo->lpX11Trans->ascent : font->ascent;
    descent = pfo->lpX11Trans ? pfo->lpX11Trans->descent : font->descent;
    xwidth = pfo->lpX11Trans ? width * pfo->lpX11Trans->a /
      pfo->lpX11Trans->pixelsize : width;
    ywidth = pfo->lpX11Trans ? width * pfo->lpX11Trans->b /
      pfo->lpX11Trans->pixelsize : 0;

    switch( dc->textAlign & (TA_LEFT | TA_RIGHT | TA_CENTER) )
    {
      case TA_LEFT:
	  if (dc->textAlign & TA_UPDATECP) {
	      pt.x = x + xwidth;
	      pt.y = y - ywidth;
	      DPtoLP(physDev->hdc, &pt, 1);
	      dc->CursPosX = pt.x;
	      dc->CursPosY = pt.y;
	  }
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
      case TA_CENTER:
	  x -= xwidth / 2;
	  y += ywidth / 2;
	  break;
    }

    switch( dc->textAlign & (TA_TOP | TA_BOTTOM | TA_BASELINE) )
    {
      case TA_TOP:
	  x -= pfo->lpX11Trans ? ascent * pfo->lpX11Trans->c /
	    pfo->lpX11Trans->pixelsize : 0;
	  y += pfo->lpX11Trans ? ascent * pfo->lpX11Trans->d /
	    pfo->lpX11Trans->pixelsize : ascent;
	  break;
      case TA_BOTTOM:
	  x += pfo->lpX11Trans ? descent * pfo->lpX11Trans->c /
	    pfo->lpX11Trans->pixelsize : 0;
	  y -= pfo->lpX11Trans ? descent * pfo->lpX11Trans->d /
	    pfo->lpX11Trans->pixelsize : descent;
	  break;
      case TA_BASELINE:
	  break;
    }

      /* Set the clip region */

    if (flags & ETO_CLIPPED)
    {
        SaveVisRgn16( HDC_16(dc->hSelf) );
        IntersectVisRect16( HDC_16(dc->hSelf), lprect->left, lprect->top, lprect->right, lprect->bottom );
    }

      /* Draw the text background if necessary */

    if (!dibUpdateFlag)
    {
        X11DRV_LockDIBSection( physDev, DIB_Status_GdiMod, FALSE );
        dibUpdateFlag = TRUE;
    }

    if (GetBkMode( physDev->hdc ) != TRANSPARENT)
    {
          /* If rectangle is opaque and clipped, do nothing */
        if (!(flags & ETO_CLIPPED) || !(flags & ETO_OPAQUE))
        {
              /* Only draw if rectangle is not opaque or if some */
              /* text is outside the rectangle */
            if (!(flags & ETO_OPAQUE) ||
                (x < rect.left) ||
                (x + width >= rect.right) ||
                (y - ascent < rect.top) ||
                (y + descent >= rect.bottom))
            {
                TSXSetForeground( gdi_display, physDev->gc, physDev->backgroundPixel );
                TSXFillRectangle( gdi_display, physDev->drawable, physDev->gc,
                                  physDev->org.x + x, physDev->org.y + y - ascent,
                                  width, ascent + descent );
            }
        }
    }

    /* Draw the text (count > 0 verified) */
    if (!(str2b = X11DRV_cptable[pfo->fi->cptable].punicode_to_char2b( pfo, wstr, count )))
        goto FAIL;

    TSXSetForeground( gdi_display, physDev->gc, physDev->textPixel );
    if(!rotated)
    {
      if (!dc->charExtra && !dc->breakExtra && !lpDx)
      {
        X11DRV_cptable[pfo->fi->cptable].pDrawString(
		pfo, gdi_display, physDev->drawable, physDev->gc,
		physDev->org.x + x, physDev->org.y + y, str2b, count );
      }
      else  /* Now the fun begins... */
      {
        XTextItem16 *items, *pitem;
	int delta;

	/* allocate max items */

        pitem = items = HeapAlloc( GetProcessHeap(), 0,
                                   count * sizeof(XTextItem16) );
	if(items == NULL) goto FAIL;
        delta = i = 0;
	if( lpDx ) /* explicit character widths */
	{
	    long ve_we;
	    unsigned short err = 0;

	    ve_we = (LONG)(dc->xformWorld2Vport.eM11 * 0x10000);

	    while (i < count)
	    {
		/* initialize text item with accumulated delta */

		long sum;
		long fSum;
		sum = 0;
		pitem->chars  = str2b + i;
		pitem->delta  = delta;
		pitem->nchars = 0;
		pitem->font   = None;
		delta = 0;

		/* add characters to the same XTextItem
		 * until new delta becomes non-zero */

		do
		{
		    sum += lpDx[i];
		    fSum = sum*ve_we+err;
		    delta = SHIWORD(fSum)
		      - X11DRV_cptable[pfo->fi->cptable].pTextWidth(
		                                pfo, pitem->chars, pitem->nchars+1);
		    pitem->nchars++;
		} while ((++i < count) && !delta);
		pitem++;
		err = LOWORD(fSum);
	   }
	}
	else /* charExtra or breakExtra */
	{
            while (i < count)
            {
		pitem->chars  = str2b + i;
		pitem->delta  = delta;
		pitem->nchars = 0;
		pitem->font   = None;
		delta = 0;

		do
                {
                    delta += dc->charExtra;
                    if (str2b[i].byte2 == (char)dfBreakChar)
		      delta += dc->breakExtra;
		    pitem->nchars++;
                } while ((++i < count) && !delta);
		pitem++;
            }
        }

	X11DRV_cptable[pfo->fi->cptable].pDrawText( pfo, gdi_display,
		physDev->drawable, physDev->gc,
		physDev->org.x + x, physDev->org.y + y, items, pitem - items );
        HeapFree( GetProcessHeap(), 0, items );
      }
    }
    else /* rotated */
    {
      /* have to render character by character. */
      double offset = 0.0;
      int i;

      for (i=0; i<count; i++)
      {
	int char_metric_offset = str2b[i].byte2 + (str2b[i].byte1 << 8)
	  - font->min_char_or_byte2;
	int x_i = IROUND((double) (physDev->org.x + x) + offset *
			 pfo->lpX11Trans->a / pfo->lpX11Trans->pixelsize );
	int y_i = IROUND((double) (physDev->org.y + y) - offset *
			 pfo->lpX11Trans->b / pfo->lpX11Trans->pixelsize );

	X11DRV_cptable[pfo->fi->cptable].pDrawString(
		pfo, gdi_display, physDev->drawable, physDev->gc,
		x_i, y_i, &str2b[i], 1);
	if (lpDx)
	{
	  offset += INTERNAL_XWSTODS(dc, lpDx[i]);
	}
	else
	{
	  offset += (double) (font->per_char ?
			      font->per_char[char_metric_offset].attributes:
			      font->min_bounds.attributes)
	                  * pfo->lpX11Trans->pixelsize / 1000.0;
	  offset += dc->charExtra;
	  if (str2b[i].byte2 == (char)dfBreakChar)
	    offset += dc->breakExtra;
	}
      }
    }
    HeapFree( GetProcessHeap(), 0, str2b );

      /* Draw underline and strike-out if needed */

    if (lfUnderline)
    {
	long linePos, lineWidth;

	if (!TSXGetFontProperty( font, XA_UNDERLINE_POSITION, &linePos ))
	    linePos = descent - 1;
	if (!TSXGetFontProperty( font, XA_UNDERLINE_THICKNESS, &lineWidth ))
	    lineWidth = 0;
	else if (lineWidth == 1) lineWidth = 0;
	TSXSetLineAttributes( gdi_display, physDev->gc, lineWidth,
			      LineSolid, CapRound, JoinBevel );
        TSXDrawLine( gdi_display, physDev->drawable, physDev->gc,
		     physDev->org.x + x, physDev->org.y + y + linePos,
		     physDev->org.x + x + width, physDev->org.y + y + linePos );
    }
    if (lfStrikeOut)
    {
	long lineAscent, lineDescent;
	if (!TSXGetFontProperty( font, XA_STRIKEOUT_ASCENT, &lineAscent ))
	    lineAscent = ascent / 2;
	if (!TSXGetFontProperty( font, XA_STRIKEOUT_DESCENT, &lineDescent ))
	    lineDescent = -lineAscent * 2 / 3;
	TSXSetLineAttributes( gdi_display, physDev->gc, lineAscent + lineDescent,
			    LineSolid, CapRound, JoinBevel );
	TSXDrawLine( gdi_display, physDev->drawable, physDev->gc,
                     physDev->org.x + x, physDev->org.y + y - lineAscent,
                     physDev->org.x + x + width, physDev->org.y + y - lineAscent );
    }

    if (flags & ETO_CLIPPED) RestoreVisRgn16( HDC_16(dc->hSelf) );
    goto END;

FAIL:
    if(str2b != NULL) HeapFree( GetProcessHeap(), 0, str2b );
    result = FALSE;

END:
    if (dibUpdateFlag) X11DRV_UnlockDIBSection( physDev, TRUE );
    return result;
}


/***********************************************************************
 *           X11DRV_GetTextExtentPoint
 */
BOOL X11DRV_GetTextExtentPoint( X11DRV_PDEVICE *physDev, LPCWSTR str, INT count,
                                  LPSIZE size )
{
    DC *dc = physDev->dc;
    fontObject* pfo = XFONT_GetFontObject( physDev->font );

    TRACE("%s %d\n", debugstr_wn(str,count), count);
    if( pfo ) {
	XChar2b *p = X11DRV_cptable[pfo->fi->cptable].punicode_to_char2b( pfo, str, count );
	if (!p) return FALSE;
        if( !pfo->lpX11Trans ) {
	    int dir, ascent, descent;
	    int info_width;
	    X11DRV_cptable[pfo->fi->cptable].pTextExtents( pfo, p,
				count, &dir, &ascent, &descent, &info_width );

          size->cx = fabs((FLOAT)(info_width + dc->breakRem + count *
                                  dc->charExtra) * dc->xformVport2World.eM11);
          size->cy = fabs((FLOAT)(pfo->fs->ascent + pfo->fs->descent) *
                          dc->xformVport2World.eM22);
	} else {
	    INT i;
	    float x = 0.0, y = 0.0;
	    /* FIXME: Deal with *_char_or_byte2 != 0 situations */
	    for(i = 0; i < count; i++) {
	        x += pfo->fs->per_char ?
	   pfo->fs->per_char[p[i].byte2 - pfo->fs->min_char_or_byte2].attributes :
	   pfo->fs->min_bounds.attributes;
	    }
	    y = pfo->lpX11Trans->RAW_ASCENT + pfo->lpX11Trans->RAW_DESCENT;
	    TRACE("x = %f y = %f\n", x, y);
	    x *= pfo->lpX11Trans->pixelsize / 1000.0;
	    y *= pfo->lpX11Trans->pixelsize / 1000.0;
	    size->cx = fabs((x + dc->breakRem + count * dc->charExtra) *
			    dc->xformVport2World.eM11);
	    size->cy = fabs(y * dc->xformVport2World.eM22);
	}
	size->cx *= pfo->rescale;
	size->cy *= pfo->rescale;
	HeapFree( GetProcessHeap(), 0, p );
	return TRUE;
    }
    return FALSE;
}
