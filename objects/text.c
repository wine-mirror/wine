/*
 * text functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
 *
 */

#include <stdlib.h>
#include <X11/Xatom.h>
#include "windows.h"
#include "dc.h"
#include "gdi.h"
#include "callback.h"
#include "metafile.h"
#include "string32.h"
#include "stddebug.h"
/* #define DEBUG_TEXT */
#include "debug.h"
#include "xmalloc.h"

#define TAB     9
#define LF     10
#define CR     13
#define SPACE  32
#define PREFIX 38

#define SWAP_INT(a,b)  { int t = a; a = b; b = t; }

static int tabstop = 8;
static int tabwidth;
static int spacewidth;
static int prefix_offset;


static const char *TEXT_NextLine( HDC hdc, const char *str, int *count,
                                  char *dest, int *len, int width, WORD format)
{
    /* Return next line of text from a string.
     * 
     * hdc - handle to DC.
     * str - string to parse into lines.
     * count - length of str.
     * dest - destination in which to return line.
     * len - length of resultant line in dest in chars.
     * width - maximum width of line in pixels.
     * format - format type passed to DrawText.
     *
     * Returns pointer to next char in str after end of the line
     * or NULL if end of str reached.
     */

    int i = 0, j = 0, k;
    int plen = 0;
    int numspaces;
    SIZE16 size;
    int lasttab = 0;
    int wb_i = 0, wb_j = 0, wb_count = 0;

    while (*count)
    {
	switch (str[i])
	{
	case CR:
	case LF:
	    if (!(format & DT_SINGLELINE))
	    {
		if (str[i] == CR && str[i+1] == LF)
		    i++;
		i++;
		*len = j;
		(*count)--;
		return (&str[i]);
	    }
	    dest[j++] = str[i++];
	    if (!(format & DT_NOCLIP) || !(format & DT_NOPREFIX) ||
		(format & DT_WORDBREAK))
	    {
		if (!GetTextExtentPoint16(hdc, &dest[j-1], 1, &size))
		    return NULL;
		plen += size.cx;
	    }
	    break;
	    
	case PREFIX:
	    if (!(format & DT_NOPREFIX))
	    {
		prefix_offset = j;
		i++;
	    }
	    else
	    {
		dest[j++] = str[i++];
		if (!(format & DT_NOCLIP) || (format & DT_WORDBREAK))
		{
		    if (!GetTextExtentPoint16(hdc, &dest[j-1], 1, &size))
			return NULL;
		    plen += size.cx;
		}
	    }
	    break;
	    
	case TAB:
	    if (format & DT_EXPANDTABS)
	    {
		wb_i = ++i;
		wb_j = j;
		wb_count = *count;

		if (!GetTextExtentPoint16(hdc, &dest[lasttab], j - lasttab,
					                         &size))
		    return NULL;

		numspaces = (tabwidth - size.cx) / spacewidth;
		for (k = 0; k < numspaces; k++)
		    dest[j++] = SPACE;
		plen += tabwidth - size.cx;
		lasttab = wb_j + numspaces;
	    }
	    else
	    {
		dest[j++] = str[i++];
		if (!(format & DT_NOCLIP) || !(format & DT_NOPREFIX) ||
		    (format & DT_WORDBREAK))
		{
		    if (!GetTextExtentPoint16(hdc, &dest[j-1], 1, &size))
			return NULL;
		    plen += size.cx;
		}
	    }
	    break;

	case SPACE:
	    dest[j++] = str[i++];
	    if (!(format & DT_NOCLIP) || !(format & DT_NOPREFIX) ||
		(format & DT_WORDBREAK))
	    {
		wb_i = i;
		wb_j = j - 1;
		wb_count = *count;
		if (!GetTextExtentPoint16(hdc, &dest[j-1], 1, &size))
		    return NULL;
		plen += size.cx;
	    }
	    break;

	default:
	    dest[j++] = str[i++];
	    if (!(format & DT_NOCLIP) || !(format & DT_NOPREFIX) ||
		(format & DT_WORDBREAK))
	    {
		if (!GetTextExtentPoint16(hdc, &dest[j-1], 1, &size))
		    return NULL;
		plen += size.cx;
	    }
	}

	(*count)--;
	if (!(format & DT_NOCLIP) || (format & DT_WORDBREAK))
	{
	    if (plen > width)
	    {
		if (format & DT_WORDBREAK)
		{
		    if (wb_j)
		    {
			*len = wb_j;
			*count = wb_count - 1;
			return (&str[wb_i]);
		    }
		}
		else
		{
		    *len = j;
		    return (&str[i]);
		}
	    }
	}
    }
    
    *len = j;
    return NULL;
}


/***********************************************************************
 *           DrawText16    (USER.85)
 */
INT16 DrawText16( HDC16 hdc, LPCSTR str, INT16 i_count,
                  LPRECT16 rect, UINT16 flags )
{
    SIZE16 size;
    const char *strPtr;
    static char line[1024];
    int len, lh, count=i_count;
    int prefix_x = 0;
    int prefix_end = 0;
    TEXTMETRIC16 tm;
    int x = rect->left, y = rect->top;
    int width = rect->right - rect->left;
    int max_width = 0;

    dprintf_text(stddeb,"DrawText: '%s', %d , [(%d,%d),(%d,%d)]\n", str,
		 count, rect->left, rect->top, rect->right, rect->bottom);
    
    if (count == -1) count = strlen(str);
    strPtr = str;

    GetTextMetrics16(hdc, &tm);
    if (flags & DT_EXTERNALLEADING)
	lh = tm.tmHeight + tm.tmExternalLeading;
    else
	lh = tm.tmHeight;

    if (flags & DT_TABSTOP)
	tabstop = flags >> 8;

    if (flags & DT_EXPANDTABS)
    {
	GetTextExtentPoint16(hdc, " ", 1, &size);
	spacewidth = size.cx;
	GetTextExtentPoint16(hdc, "o", 1, &size);
	tabwidth = size.cx * tabstop;
    }

    do
    {
	prefix_offset = -1;
	strPtr = TEXT_NextLine(hdc, strPtr, &count, line, &len, width, flags);

	if (prefix_offset != -1)
	{
	    GetTextExtentPoint16(hdc, line, prefix_offset, &size);
	    prefix_x = size.cx;
	    GetTextExtentPoint16(hdc, line, prefix_offset + 1, &size);
	    prefix_end = size.cx - 1;
	}

	if (!GetTextExtentPoint16(hdc, line, len, &size)) return 0;
	if (flags & DT_CENTER) x = (rect->left + rect->right -
				    size.cx) / 2;
	else if (flags & DT_RIGHT) x = rect->right - size.cx;

	if (flags & DT_SINGLELINE)
	{
	    if (flags & DT_VCENTER) y = rect->top + 
	    	(rect->bottom - rect->top) / 2 - size.cy / 2;
	    else if (flags & DT_BOTTOM) y = rect->bottom - size.cy;
	}
	if (!(flags & DT_CALCRECT))
	{
	    if (!ExtTextOut16(hdc, x, y, (flags & DT_NOCLIP) ? 0 : ETO_CLIPPED,
                              rect, line, len, NULL )) return 0;
            if (prefix_offset != -1)
            {
                HPEN16 hpen = CreatePen( PS_SOLID, 1, GetTextColor(hdc) );
                HPEN16 oldPen = SelectObject( hdc, hpen );
                MoveTo(hdc, x + prefix_x, y + tm.tmAscent + 1 );
                LineTo(hdc, x + prefix_end, y + tm.tmAscent + 1 );
                SelectObject( hdc, oldPen );
                DeleteObject( hpen );
            }
	}
	else if (size.cx > max_width)
	    max_width = size.cx;

	y += lh;
	if (strPtr)
	{
	    if (!(flags & DT_NOCLIP) && !(flags & DT_CALCRECT))
	    {
		if (y > rect->bottom - lh)
		    break;
	    }
	}
    }
    while (strPtr);
    if (flags & DT_CALCRECT)
    {
	rect->right = rect->left + max_width;
	rect->bottom = y;
    }
    return 1;
}


/***********************************************************************
 *           DrawText32A    (USER32.163)
 */
INT32 DrawText32A( HDC32 hdc, LPCSTR str, INT32 count,
                   LPRECT32 rect, UINT32 flags )
{
    RECT16 rect16;
    INT16 ret;

    if (!rect)
        return DrawText16( (HDC16)hdc, str, (INT16)count, NULL, (UINT16)flags);
    CONV_RECT32TO16( rect, &rect16 );
    ret = DrawText16( (HDC16)hdc, str, (INT16)count, &rect16, (UINT16)flags );
    CONV_RECT16TO32( &rect16, rect );
    return ret;
}


/***********************************************************************
 *           DrawText32W    (USER32.166)
 */
INT32 DrawText32W( HDC32 hdc, LPCWSTR str, INT32 count,
                   LPRECT32 rect, UINT32 flags )
{
    char *p = STRING32_DupUniToAnsi( str );
    INT32 ret = DrawText32A( hdc, p, count, rect, flags );
    free(p);
    return ret;
}


/***********************************************************************
 *           ExtTextOut16    (GDI.351)
 */
BOOL16 ExtTextOut16( HDC16 hdc, INT16 x, INT16 y, UINT16 flags,
                     const RECT16 *lprect, LPCSTR str, UINT16 count,
                     const INT16 *lpDx )
{
    int dir, ascent, descent, i;
    XCharStruct info;
    XFontStruct *font;
    RECT16 rect;

    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr( hdc, METAFILE_DC_MAGIC );
	if (!dc) return FALSE;
	MF_TextOut( dc, x, y, str, count );
	return TRUE;
    }

    if (!DC_SetupGCForText( dc )) return TRUE;
    font = dc->u.x.font.fstruct;

    dprintf_text(stddeb,"ExtTextOut: hdc=%04x %d,%d '%*.*s', %d  flags=%d\n",
            hdc, x, y, count, count, str, count, flags);
    if (lprect != NULL) dprintf_text(stddeb, "\trect=(%d,%d- %d,%d)\n",
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
            SIZE16 sz;
            if (flags & ETO_CLIPPED)  /* Can't clip with no rectangle */
	      return FALSE;
	    if (!GetTextExtentPoint16( hdc, str, count, &sz ))
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

    dprintf_text(stddeb,"\treal coord: x=%i, y=%i, rect=(%d,%d-%d,%d)\n",
			  x, y, rect.left, rect.top, rect.right, rect.bottom);

      /* Draw the rectangle */

    if (flags & ETO_OPAQUE)
    {
        XSetForeground( display, dc->u.x.gc, dc->w.backgroundPixel );
        XFillRectangle( display, dc->u.x.drawable, dc->u.x.gc,
                        dc->w.DCOrgX + rect.left, dc->w.DCOrgY + rect.top,
                        rect.right-rect.left, rect.bottom-rect.top );
    }
    if (!count) return TRUE;  /* Nothing more to do */

      /* Compute text starting position */

    XTextExtents( font, str, count, &dir, &ascent, &descent, &info );

    if (lpDx) /* have explicit character cell x offsets */
    {
	/* sum lpDx array and add the width of last character */

        info.width = XTextWidth( font, str + count - 1, 1) + dc->w.charExtra;
        if (str[count-1] == (char)dc->u.x.font.metrics.tmBreakChar)
            info.width += dc->w.breakExtra;

        for (i = 0; i < count; i++) info.width += lpDx[i];
    }
    else
       info.width += count*dc->w.charExtra + dc->w.breakExtra*dc->w.breakCount;

    switch( dc->w.textAlign & (TA_LEFT | TA_RIGHT | TA_CENTER) )
    {
      case TA_LEFT:
 	  if (dc->w.textAlign & TA_UPDATECP)
	      dc->w.CursPosX = XDPTOLP( dc, x + info.width );
	  break;
      case TA_RIGHT:
	  x -= info.width;
	  if (dc->w.textAlign & TA_UPDATECP) dc->w.CursPosX = XDPTOLP( dc, x );
	  break;
      case TA_CENTER:
	  x -= info.width / 2;
	  break;
    }

    switch( dc->w.textAlign & (TA_TOP | TA_BOTTOM | TA_BASELINE) )
    {
      case TA_TOP:
	  y += font->ascent;
	  break;
      case TA_BOTTOM:
	  y -= font->descent;
	  break;
      case TA_BASELINE:
	  break;
    }

      /* Set the clip region */

    if (flags & ETO_CLIPPED)
    {
        SaveVisRgn( hdc );
        IntersectVisRect( hdc, rect.left, rect.top, rect.right, rect.bottom );
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
                (x + info.width >= rect.right) ||
                (y-font->ascent < rect.top) ||
                (y+font->descent >= rect.bottom))
            {
                XSetForeground( display, dc->u.x.gc, dc->w.backgroundPixel );
                XFillRectangle( display, dc->u.x.drawable, dc->u.x.gc,
                                dc->w.DCOrgX + x,
                                dc->w.DCOrgY + y - font->ascent,
                                info.width,
                                font->ascent + font->descent );
            }
        }
    }
    
    /* Draw the text (count > 0 verified) */

    XSetForeground( display, dc->u.x.gc, dc->w.textPixel );
    if (!dc->w.charExtra && !dc->w.breakExtra && !lpDx)
    {
        XDrawString( display, dc->u.x.drawable, dc->u.x.gc, 
                     dc->w.DCOrgX + x, dc->w.DCOrgY + y, str, count );
    }
    else  /* Now the fun begins... */
    {
        XTextItem *items, *pitem;
	int delta;

	/* allocate max items */

        pitem = items = xmalloc( count * sizeof(XTextItem) );
        delta = i = 0;
        while (i < count)
        {
	    /* initialize text item with accumulated delta */

            pitem->chars  = (char *)str + i;
	    pitem->delta  = delta; 
            pitem->nchars = 0;
            pitem->font   = None;
            delta = 0;

	    /* stuff characters into the same XTextItem until new delta 
	     * becomes  non-zero */

	    do
            {
                if (lpDx) delta += lpDx[i] - XTextWidth( font, str + i, 1);
                else
                {
                    delta += dc->w.charExtra;
                    if (str[i] == (char)dc->u.x.font.metrics.tmBreakChar)
                        delta += dc->w.breakExtra;
                }
                pitem->nchars++;
            } 
	    while ((++i < count) && !delta);
            pitem++;
        }

        XDrawText( display, dc->u.x.drawable, dc->u.x.gc,
                   dc->w.DCOrgX + x, dc->w.DCOrgY + y, items, pitem - items );
        free( items );
    }

      /* Draw underline and strike-out if needed */

    if (dc->u.x.font.metrics.tmUnderlined)
    {
	long linePos, lineWidth;       
	if (!XGetFontProperty( font, XA_UNDERLINE_POSITION, &linePos ))
	    linePos = font->descent-1;
	if (!XGetFontProperty( font, XA_UNDERLINE_THICKNESS, &lineWidth ))
	    lineWidth = 0;
	else if (lineWidth == 1) lineWidth = 0;
	XSetLineAttributes( display, dc->u.x.gc, lineWidth,
			    LineSolid, CapRound, JoinBevel ); 
        XDrawLine( display, dc->u.x.drawable, dc->u.x.gc,
		   dc->w.DCOrgX + x, dc->w.DCOrgY + y + linePos,
		   dc->w.DCOrgX + x + info.width, dc->w.DCOrgY + y + linePos );
    }
    if (dc->u.x.font.metrics.tmStruckOut)
    {
	long lineAscent, lineDescent;
	if (!XGetFontProperty( font, XA_STRIKEOUT_ASCENT, &lineAscent ))
	    lineAscent = font->ascent / 3;
	if (!XGetFontProperty( font, XA_STRIKEOUT_DESCENT, &lineDescent ))
	    lineDescent = -lineAscent;
	XSetLineAttributes( display, dc->u.x.gc, lineAscent + lineDescent,
			    LineSolid, CapRound, JoinBevel ); 
	XDrawLine( display, dc->u.x.drawable, dc->u.x.gc,
		   dc->w.DCOrgX + x, dc->w.DCOrgY + y - lineAscent,
		   dc->w.DCOrgX + x + info.width, dc->w.DCOrgY + y - lineAscent );
    }
    if (flags & ETO_CLIPPED) RestoreVisRgn( hdc );
    return TRUE;
}


/***********************************************************************
 *           ExtTextOut32A    (GDI32.98)
 */
BOOL32 ExtTextOut32A( HDC32 hdc, INT32 x, INT32 y, UINT32 flags,
                      const RECT32 *lprect, LPCSTR str, UINT32 count,
                      const INT32 *lpDx )
{
    RECT16 rect16;

    if (lpDx) fprintf( stderr, "ExtTextOut32A: lpDx not implemented\n" );
    if (!lprect)
        return ExtTextOut16( (HDC16)hdc, (INT16)x, (INT16)y, (UINT16)flags,
                             NULL, str, (UINT16)count, NULL );
    CONV_RECT32TO16( lprect, &rect16 );
    return ExtTextOut16( (HDC16)hdc, (INT16)x, (INT16)y, (UINT16)flags,
                         &rect16, str, (UINT16)count, NULL );
}


/***********************************************************************
 *           ExtTextOut32W    (GDI32.99)
 */
BOOL32 ExtTextOut32W( HDC32 hdc, INT32 x, INT32 y, UINT32 flags,
                      const RECT32 *lprect, LPCWSTR str, UINT32 count,
                      const INT32 *lpDx )
{
    char *p = STRING32_DupUniToAnsi( str );
    INT32 ret = ExtTextOut32A( hdc, x, y, flags, lprect, p, count, lpDx );
    free(p);
    return ret;
}


/***********************************************************************
 *           TextOut16    (GDI.33)
 */
BOOL16 TextOut16( HDC16 hdc, INT16 x, INT16 y, LPCSTR str, INT16 count )
{
    return ExtTextOut16( hdc, x, y, 0, NULL, str, count, NULL );
}


/***********************************************************************
 *           TextOut32A    (GDI32.355)
 */
BOOL32 TextOut32A( HDC32 hdc, INT32 x, INT32 y, LPCSTR str, INT32 count )
{
    return ExtTextOut32A( hdc, x, y, 0, NULL, str, count, NULL );
}


/***********************************************************************
 *           TextOut32W    (GDI32.356)
 */
BOOL32 TextOut32W( HDC32 hdc, INT32 x, INT32 y, LPCWSTR str, INT32 count )
{
    return ExtTextOut32W( hdc, x, y, 0, NULL, str, count, NULL );
}


/***********************************************************************
 *		GrayString (USER.185)
 */
BOOL GrayString(HDC hdc, HBRUSH hbr, FARPROC16 gsprc, LPARAM lParam, 
		INT cch, INT x, INT y, INT cx, INT cy)
{
    BOOL ret;
    COLORREF current_color;

    if (!cch) cch = lstrlen16( (LPCSTR)PTR_SEG_TO_LIN(lParam) );
    if (gsprc) return CallGrayStringProc( gsprc, hdc, lParam, cch );
    current_color = GetTextColor( hdc );
    SetTextColor( hdc, GetSysColor(COLOR_GRAYTEXT) );
    ret = TextOut16( hdc, x, y, (LPCSTR)PTR_SEG_TO_LIN(lParam), cch );
    SetTextColor( hdc, current_color );
    return ret;
}


/***********************************************************************
 *           TEXT_TabbedTextOut
 *
 * Helper function for TabbedTextOut() and GetTabbedTextExtent().
 * Note: this doesn't work too well for text-alignment modes other
 *       than TA_LEFT|TA_TOP. But we want bug-for-bug compatibility :-)
 */
LONG TEXT_TabbedTextOut( HDC hdc, int x, int y, LPSTR lpstr, int count, 
                         int cTabStops, LPINT16 lpTabPos, int nTabOrg,
                         BOOL fDisplayText)
{
    WORD defWidth;
    DWORD extent = 0;
    int i, tabPos = x;
    int start = x;

    if (cTabStops == 1)
    {
        defWidth = *lpTabPos;
        cTabStops = 0;
    }
    else
    {
        TEXTMETRIC16 tm;
        GetTextMetrics16( hdc, &tm );
        defWidth = 8 * tm.tmAveCharWidth;
    }
    
    while (count > 0)
    {
        for (i = 0; i < count; i++)
            if (lpstr[i] == '\t') break;
        extent = GetTextExtent( hdc, lpstr, i );
        while ((cTabStops > 0) && (nTabOrg + *lpTabPos <= x + LOWORD(extent)))
        {
            lpTabPos++;
            cTabStops--;
        }
        if (i == count)
            tabPos = x + LOWORD(extent);
        else if (cTabStops > 0)
            tabPos = nTabOrg + *lpTabPos;
        else
            tabPos = nTabOrg + ((x + LOWORD(extent) - nTabOrg) / defWidth + 1) * defWidth;
        if (fDisplayText)
        {
            RECT16 r;
            SetRect16( &r, x, y, tabPos, y+HIWORD(extent) );
            ExtTextOut16( hdc, x, y,
                          GetBkMode(hdc) == OPAQUE ? ETO_OPAQUE : 0,
                          &r, lpstr, i, NULL );
        }
        x = tabPos;
        count -= i+1;
        lpstr += i+1;
    }
    return MAKELONG(tabPos - start, HIWORD(extent));
}


/***********************************************************************
 *           TabbedTextOut    (USER.196)
 */
LONG TabbedTextOut( HDC hdc, short x, short y, LPSTR lpstr, short count, 
                    short cTabStops, LPINT16 lpTabPos, short nTabOrg )
{
    dprintf_text( stddeb, "TabbedTextOut: %04x %d,%d '%*.*s' %d\n",
                  hdc, x, y, count, count, lpstr, count );
    return TEXT_TabbedTextOut( hdc, x, y, lpstr, count, cTabStops,
                               lpTabPos, nTabOrg, TRUE );
}


/***********************************************************************
 *           GetTabbedTextExtent    (USER.197)
 */
DWORD GetTabbedTextExtent( HDC hdc, LPSTR lpstr, int count, 
                          int cTabStops, LPINT16 lpTabPos )
{
    dprintf_text( stddeb, "GetTabbedTextExtent: %04x '%*.*s' %d\n",
                  hdc, count, count, lpstr, count );
    return TEXT_TabbedTextOut( hdc, 0, 0, lpstr, count, cTabStops,
                               lpTabPos, 0, FALSE );
}
