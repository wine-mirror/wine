/*
 * X11 graphics driver text functions
 *
 * Copyright 1993,1994 Alexandre Julliard
 */

#include <stdlib.h>
#include "ts_xlib.h"
#include <X11/Xatom.h>
#include "windows.h"
#include <math.h>
#include "dc.h"
#include "gdi.h"
/*#include "callback.h"*/
#include "heap.h"
#include "x11font.h"
#include "debugstr.h"
#include "debug.h"

#define SWAP_INT(a,b)  { int t = a; a = b; b = t; }
#define IROUND(x) (int)((x)>0? (x)+0.5 : (x) - 0.5)

/***********************************************************************
 *           X11DRV_ExtTextOut
 */
BOOL32
X11DRV_ExtTextOut( DC *dc, INT32 x, INT32 y, UINT32 flags,
                   const RECT32 *lprect, LPCSTR str, UINT32 count,
                   const INT32 *lpDx )
{
    int 	        i;
    fontObject*		pfo;
    INT32	 	width, ascent, descent, xwidth, ywidth;
    XFontStruct*	font;
    RECT32 		rect;
    char		dfBreakChar, lfUnderline, lfStrikeOut;
    BOOL32		rotated = FALSE;

    if (!X11DRV_SetupGCForText( dc )) return TRUE;

    pfo = XFONT_GetFontObject( dc->u.x.font );
    font = pfo->fs;
     
    if (pfo->lf.lfEscapement && pfo->lpX11Trans)
        rotated = TRUE;
    dfBreakChar = (char)pfo->fi->df.dfBreakChar;
    lfUnderline = (pfo->fo_flags & FO_SYNTH_UNDERLINE) ? 1 : 0;
    lfStrikeOut = (pfo->fo_flags & FO_SYNTH_STRIKEOUT) ? 1 : 0;

    TRACE(text,"hdc=%04x df=%04x %d,%d %s, %d  flags=%d lpDx=%p\n",
	  dc->hSelf, (UINT16)(dc->u.x.font), x, y,
	  debugstr_an (str, count), count, flags, lpDx);

    /* some strings sent here end in a newline for whatever reason.  I have no
       clue what the right treatment should be in general, but ignoring
       terminating newlines seems ok.  MW, April 1998.  */
    if (count > 0 && str[count - 1] == '\n') count--;

    if (lprect != NULL) TRACE(text, "\trect=(%d,%d - %d,%d)\n",
                                     lprect->left, lprect->top,
                                     lprect->right, lprect->bottom );
      /* Setup coordinates */

    if (dc->w.textAlign & TA_UPDATECP)
    {
	x = dc->w.CursPosX;
	y = dc->w.CursPosY;
    }

    if (flags & (ETO_OPAQUE | ETO_CLIPPED))  /* there's a rectangle */
    {
        if (!lprect)  /* not always */
        {
            SIZE32 sz;
            if (flags & ETO_CLIPPED)  /* Can't clip with no rectangle */
	      return FALSE;
	    if (!X11DRV_GetTextExtentPoint( dc, str, count, &sz ))
	      return FALSE;
	    rect.left   = XLPTODP( dc, x );
	    rect.right  = XLPTODP( dc, x+sz.cx );
	    rect.top    = YLPTODP( dc, y );
	    rect.bottom = YLPTODP( dc, y+sz.cy );
	}
	else
	{
	    rect.left   = XLPTODP( dc, lprect->left );
	    rect.right  = XLPTODP( dc, lprect->right );
	    rect.top    = YLPTODP( dc, lprect->top );
	    rect.bottom = YLPTODP( dc, lprect->bottom );
	}
	if (rect.right < rect.left) SWAP_INT( rect.left, rect.right );
	if (rect.bottom < rect.top) SWAP_INT( rect.top, rect.bottom );
    }

    x = XLPTODP( dc, x );
    y = YLPTODP( dc, y );

    TRACE(text,"\treal coord: x=%i, y=%i, rect=(%d,%d - %d,%d)\n",
			  x, y, rect.left, rect.top, rect.right, rect.bottom);

      /* Draw the rectangle */

    if (flags & ETO_OPAQUE)
    {
        TSXSetForeground( display, dc->u.x.gc, dc->u.x.backgroundPixel );
        TSXFillRectangle( display, dc->u.x.drawable, dc->u.x.gc,
                        dc->w.DCOrgX + rect.left, dc->w.DCOrgY + rect.top,
                        rect.right-rect.left, rect.bottom-rect.top );
    }
    if (!count) return TRUE;  /* Nothing more to do */

      /* Compute text starting position */

    if (lpDx) /* have explicit character cell x offsets in logical coordinates */
    {
	int extra = dc->wndExtX / 2;
        for (i = width = 0; i < count; i++) width += lpDx[i];
	width = (width * dc->vportExtX + extra ) / dc->wndExtX;
    }
    else
    {
        SIZE32 sz;
        if (!X11DRV_GetTextExtentPoint( dc, str, count, &sz ))
	    return FALSE;
	width = XLSTODS(dc, sz.cx);
    }
    ascent = pfo->lpX11Trans ? pfo->lpX11Trans->ascent : font->ascent;
    descent = pfo->lpX11Trans ? pfo->lpX11Trans->descent : font->descent;
    xwidth = pfo->lpX11Trans ? width * pfo->lpX11Trans->a /
      pfo->lpX11Trans->pixelsize : width;
    ywidth = pfo->lpX11Trans ? width * pfo->lpX11Trans->b /
      pfo->lpX11Trans->pixelsize : 0;

    switch( dc->w.textAlign & (TA_LEFT | TA_RIGHT | TA_CENTER) )
    {
      case TA_LEFT:
	  if (dc->w.textAlign & TA_UPDATECP) {
	      dc->w.CursPosX = XDPTOLP( dc, x + xwidth );
	      dc->w.CursPosY = YDPTOLP( dc, y - ywidth );
	  }
	  break;
      case TA_RIGHT:
	  x -= xwidth;
	  y += ywidth;
	  if (dc->w.textAlign & TA_UPDATECP) {
	      dc->w.CursPosX = XDPTOLP( dc, x );
	      dc->w.CursPosY = YDPTOLP( dc, y );
	  }
	  break;
      case TA_CENTER:
	  x -= xwidth / 2;
	  y += ywidth / 2;
	  break;
    }

    switch( dc->w.textAlign & (TA_TOP | TA_BOTTOM | TA_BASELINE) )
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
        SaveVisRgn( dc->hSelf );
        CLIPPING_IntersectVisRect( dc, rect.left, rect.top, rect.right,
                                   rect.bottom, FALSE );
    }

      /* Draw the text background if necessary */

    if (dc->w.backgroundMode != TRANSPARENT)
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
                TSXSetForeground( display, dc->u.x.gc, dc->u.x.backgroundPixel );
                TSXFillRectangle( display, dc->u.x.drawable, dc->u.x.gc,
                                dc->w.DCOrgX + x,
                                dc->w.DCOrgY + y - ascent,
                                width,
                                ascent + descent );
            }
        }
    }
    
    /* Draw the text (count > 0 verified) */

    TSXSetForeground( display, dc->u.x.gc, dc->u.x.textPixel );
    if (!dc->w.charExtra && !dc->w.breakExtra && !lpDx)
    {
      if(!rotated)
      {
        TSXDrawString( display, dc->u.x.drawable, dc->u.x.gc, 
                     dc->w.DCOrgX + x, dc->w.DCOrgY + y, str, count );
      }
      else 
      {  
	/* have to render character by character. */
	double offset = 0.0;
	int i;

	for(i=0; i<count; i++) {
	  int char_metric_offset = (unsigned char) str[i] 
	    - font->min_char_or_byte2;
	  int x_i = IROUND((double) (dc->w.DCOrgX + x) + offset *
			   pfo->lpX11Trans->a / 1000.0 );
	  int y_i = IROUND((double) (dc->w.DCOrgY + y) - offset *
			   pfo->lpX11Trans->b / 1000.0 );

	  TSXDrawString( display, dc->u.x.drawable, dc->u.x.gc,
			 x_i, y_i, &str[i], 1);
	  offset += (double) (font->per_char ?
		     font->per_char[char_metric_offset].attributes:
		     font->min_bounds.attributes);
	}
      }
    }
    else  /* Now the fun begins... */
    {
        XTextItem *items, *pitem;
	int delta;

	/* allocate max items */

        pitem = items = HEAP_xalloc( GetProcessHeap(), 0,
                                     count * sizeof(XTextItem) );
        delta = i = 0;
	if( lpDx ) /* explicit character widths */
	{
	    int extra = dc->wndExtX / 2;

	    while (i < count)
	    {
		/* initialize text item with accumulated delta */

		pitem->chars  = (char *)str + i;
		pitem->delta  = delta;
		pitem->nchars = 0;
		pitem->font   = None;
		delta = 0;

		/* add characters  to the same XTextItem until new delta
		 * becomes  non-zero */

		do
		{
		    delta += (lpDx[i] * dc->vportExtX + extra) / dc->wndExtX
					    - TSXTextWidth( font, str + i, 1);
		    pitem->nchars++;
		} while ((++i < count) && !delta);
		pitem++;
	   }
	}
	else /* charExtra or breakExtra */
	{
            while (i < count)
            {
		pitem->chars  = (char *)str + i;
		pitem->delta  = delta;
		pitem->nchars = 0;
		pitem->font   = None;
		delta = 0;

		do
                {
                    delta += dc->w.charExtra;
                    if (str[i] == (char)dfBreakChar) delta += dc->w.breakExtra;
		    pitem->nchars++;
                } while ((++i < count) && !delta);
		pitem++;
            } 
        }

        TSXDrawText( display, dc->u.x.drawable, dc->u.x.gc,
                   dc->w.DCOrgX + x, dc->w.DCOrgY + y, items, pitem - items );
        HeapFree( GetProcessHeap(), 0, items );
    }

      /* Draw underline and strike-out if needed */

    if (lfUnderline)
    {
	long linePos, lineWidth;       

	if (!TSXGetFontProperty( font, XA_UNDERLINE_POSITION, &linePos ))
	    linePos = descent - 1;
	if (!TSXGetFontProperty( font, XA_UNDERLINE_THICKNESS, &lineWidth ))
	    lineWidth = 0;
	else if (lineWidth == 1) lineWidth = 0;
	TSXSetLineAttributes( display, dc->u.x.gc, lineWidth,
			    LineSolid, CapRound, JoinBevel ); 
        TSXDrawLine( display, dc->u.x.drawable, dc->u.x.gc,
		   dc->w.DCOrgX + x, dc->w.DCOrgY + y + linePos,
		   dc->w.DCOrgX + x + width, dc->w.DCOrgY + y + linePos );
    }
    if (lfStrikeOut)
    {
	long lineAscent, lineDescent;
	if (!TSXGetFontProperty( font, XA_STRIKEOUT_ASCENT, &lineAscent ))
	    lineAscent = ascent / 2;
	if (!TSXGetFontProperty( font, XA_STRIKEOUT_DESCENT, &lineDescent ))
	    lineDescent = -lineAscent * 2 / 3;
	TSXSetLineAttributes( display, dc->u.x.gc, lineAscent + lineDescent,
			    LineSolid, CapRound, JoinBevel ); 
	TSXDrawLine( display, dc->u.x.drawable, dc->u.x.gc,
		   dc->w.DCOrgX + x, dc->w.DCOrgY + y - lineAscent,
		   dc->w.DCOrgX + x + width, dc->w.DCOrgY + y - lineAscent );
    }

    if (flags & ETO_CLIPPED) 
        RestoreVisRgn( dc->hSelf );

    return TRUE;
}

