/*
 * text functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
 *
static char Copyright[] = "Copyright  Alexandre Julliard, 1993, 1994";
*/
#include <stdlib.h>
#include <X11/Xatom.h>
#include "windows.h"
#include "dc.h"
#include "gdi.h"
#include "callback.h"
#include "metafile.h"
#include "stddebug.h"
/* #define DEBUG_TEXT */
#include "debug.h"

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


static char *TEXT_NextLine(HDC hdc, char *str, int *count, char *dest, 
			   int *len, int width, WORD format)
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
    SIZE size;
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
		if (!GetTextExtentPoint(hdc, &dest[j-1], 1, &size))
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
		    if (!GetTextExtentPoint(hdc, &dest[j-1], 1, &size))
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

		if (!GetTextExtentPoint(hdc, &dest[lasttab], j - lasttab,
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
		    if (!GetTextExtentPoint(hdc, &dest[j-1], 1, &size))
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
		if (!GetTextExtentPoint(hdc, &dest[j-1], 1, &size))
		    return NULL;
		plen += size.cx;
	    }
	    break;

	default:
	    dest[j++] = str[i++];
	    if (!(format & DT_NOCLIP) || !(format & DT_NOPREFIX) ||
		(format & DT_WORDBREAK))
	    {
		if (!GetTextExtentPoint(hdc, &dest[j-1], 1, &size))
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
 *           DrawText    (USER.85)
 */
int DrawText( HDC hdc, LPSTR str, int count, LPRECT rect, WORD flags )
{
    SIZE size;
    char *strPtr;
    static char line[1024];
    int len, lh, prefix_x, prefix_end;
    TEXTMETRIC tm;
    int x = rect->left, y = rect->top;
    int width = rect->right - rect->left;
    int max_width = 0;

    dprintf_text(stddeb,"DrawText: '%s', %d , [(%d,%d),(%d,%d)]\n", str, count,
	   rect->left, rect->top, rect->right, rect->bottom);
    
    if (count == -1) count = strlen(str);
    strPtr = str;

    GetTextMetrics(hdc, &tm);
    if (flags & DT_EXTERNALLEADING)
	lh = tm.tmHeight + tm.tmExternalLeading;
    else
	lh = tm.tmHeight;

    if (flags & DT_TABSTOP)
	tabstop = flags >> 8;

    if (flags & DT_EXPANDTABS)
    {
	GetTextExtentPoint(hdc, " ", 1, &size);
	spacewidth = size.cx;
	GetTextExtentPoint(hdc, "o", 1, &size);
	tabwidth = size.cx * tabstop;
    }

    do
    {
	prefix_offset = -1;
	strPtr = TEXT_NextLine(hdc, strPtr, &count, line, &len, width, flags);

	if (prefix_offset != -1)
	{
	    GetTextExtentPoint(hdc, line, prefix_offset, &size);
	    prefix_x = size.cx;
	    GetTextExtentPoint(hdc, line, prefix_offset + 1, &size);
	    prefix_end = size.cx - 1;
	}

	if (!GetTextExtentPoint(hdc, line, len, &size)) return 0;
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
	    if (!ExtTextOut( hdc, x, y, (flags & DT_NOCLIP) ? 0 : ETO_CLIPPED,
                             rect, line, len, NULL )) return 0;
	}
	else if (size.cx > max_width)
	    max_width = size.cx;

	if (prefix_offset != -1)
	{
	    HPEN hpen = CreatePen( PS_SOLID, 1, GetTextColor(hdc) );
	    HPEN oldPen = SelectObject( hdc, hpen );
	    MoveTo(hdc, x + prefix_x, y + tm.tmAscent + 1 );
	    LineTo(hdc, x + prefix_end, y + tm.tmAscent + 1 );
	    SelectObject( hdc, oldPen );
	    DeleteObject( hpen );
	}

	y += lh;
	if (strPtr)
	{
	    if (!(flags & DT_NOCLIP))
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
 *           ExtTextOut    (GDI.351)
 */
BOOL ExtTextOut( HDC hdc, short x, short y, WORD flags, LPRECT lprect,
                 LPSTR str, WORD count, LPINT lpDx )
{
    int dir, ascent, descent, i;
    XCharStruct info;
    XFontStruct *font;
    RECT rect;

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

    dprintf_text(stddeb,"ExtTextOut: %d,%d '%*.*s', %d  flags=%d rect=%d,%d,%d,%d\n",
            x, y, count, count, str, count, flags,
            lprect->left, lprect->top, lprect->right, lprect->bottom );

      /* Setup coordinates */

    if (dc->w.textAlign & TA_UPDATECP)
    {
	x = dc->w.CursPosX;
	y = dc->w.CursPosY;
    }
    x = XLPTODP( dc, x );
    y = YLPTODP( dc, y );
    if (flags & (ETO_OPAQUE | ETO_CLIPPED))  /* There's a rectangle */
    {
        rect.left   = XLPTODP( dc, lprect->left );
        rect.right  = XLPTODP( dc, lprect->right );
        rect.top    = YLPTODP( dc, lprect->top );
        rect.bottom = YLPTODP( dc, lprect->bottom );
        if (rect.right < rect.left) SWAP_INT( rect.left, rect.right );
        if (rect.bottom < rect.top) SWAP_INT( rect.top, rect.bottom );
    }

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
    info.width += count*dc->w.charExtra + dc->w.breakExtra*dc->w.breakCount;
    if (lpDx) for (i = 0; i < count; i++) info.width += lpDx[i];

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
    
      /* Draw the text */

    XSetForeground( display, dc->u.x.gc, dc->w.textPixel );
    if (!dc->w.charExtra && !dc->w.breakExtra && !lpDx)
    {
        XDrawString( display, dc->u.x.drawable, dc->u.x.gc, 
                     dc->w.DCOrgX + x, dc->w.DCOrgY + y, str, count );
    }
    else  /* Now the fun begins... */
    {
        XTextItem *items, *pitem;

        items = malloc( count * sizeof(XTextItem) );
        for (i = 0, pitem = items; i < count; i++, pitem++)
        {
            pitem->chars  = str + i;
            pitem->nchars = 1;
            pitem->font   = None;
            if (i == 0)
            {
                pitem->delta = 0;
                continue;  /* First iteration -> no delta */
            }
            pitem->delta = dc->w.charExtra;
            if (str[i] == (char)dc->u.x.font.metrics.tmBreakChar)
                pitem->delta += dc->w.breakExtra;
            if (lpDx)
            {
                INT width;
                GetCharWidth( hdc, str[i], str[i], &width );
                pitem->delta += lpDx[i-1] - width;
            }
        }
        XDrawText( display, dc->u.x.drawable, dc->u.x.gc,
                   dc->w.DCOrgX + x, dc->w.DCOrgY + y, items, count );
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
 *           TextOut    (GDI.33)
 */
BOOL TextOut( HDC hdc, short x, short y, LPSTR str, short count )
{
    return ExtTextOut( hdc, x, y, 0, NULL, str, count, NULL );
}


/***********************************************************************
 *		GrayString (USER.185)
 */
BOOL GrayString(HDC hdc, HBRUSH hbr, FARPROC gsprc, LPARAM lParam, 
		INT cch, INT x, INT y, INT cx, INT cy)
{
	int s, current_color;

	if (gsprc) {
		return CallGrayStringProc(gsprc, hdc, lParam, 
					cch ? cch : lstrlen((LPCSTR) lParam) );
	} else {
		current_color = GetTextColor(hdc);
		SetTextColor(hdc, GetSysColor(COLOR_GRAYTEXT) );
		s = TextOut(hdc, x, y, (LPSTR) lParam, 
				cch ? cch : lstrlen((LPCSTR) lParam) );
		SetTextColor(hdc, current_color);
		
		return s;
	}
}


/***********************************************************************
 *           TEXT_TabbedTextOut
 *
 * Helper function for TabbedTextOut() and GetTabbedTextExtent().
 * Note: this doesn't work too well for text-alignment modes other
 *       than TA_LEFT|TA_TOP. But we want bug-for-bug compatibility :-)
 */
LONG TEXT_TabbedTextOut( HDC hdc, int x, int y, LPSTR lpstr, int count, 
                         int cTabStops, LPINT lpTabPos, int nTabOrg,
                         BOOL fDisplayText)
{
    WORD defWidth;
    DWORD extent;
    int i, tabPos = 0;

    if (cTabStops == 1) defWidth = *lpTabPos;
    else
    {
        TEXTMETRIC tm;
        GetTextMetrics( hdc, &tm );
        defWidth = 8 * tm.tmAveCharWidth;
    }
    
    while (count > 0)
    {
        for (i = 0; i < count; i++)
            if (lpstr[i] == '\t') break;
        extent = GetTextExtent( hdc, lpstr, i );
        while ((cTabStops > 0) && (nTabOrg + *lpTabPos < x + LOWORD(extent)))
        {
            lpTabPos++;
            cTabStops--;
        }
        if (lpstr[i] != '\t')
            tabPos = x + LOWORD(extent);
        else if (cTabStops > 0)
            tabPos = nTabOrg + *lpTabPos;
        else
            tabPos = (x + LOWORD(extent) + defWidth - 1) / defWidth * defWidth;
        if (fDisplayText)
        {
            RECT r;
            SetRect( &r, x, y, tabPos, y+HIWORD(extent) );
            ExtTextOut( hdc, x, y,
                        GetBkMode(hdc) == OPAQUE ? ETO_OPAQUE : 0,
                        &r, lpstr, i, NULL );
        }
        x = tabPos;
        count -= i+1;
        lpstr += i+1;
    }
    return tabPos;
}


/***********************************************************************
 *           TabbedTextOut    (USER.196)
 */
LONG TabbedTextOut( HDC hdc, short x, short y, LPSTR lpstr, short count, 
                    short cTabStops, LPINT lpTabPos, short nTabOrg )
{
    dprintf_text( stddeb, "TabbedTextOut: %x %d,%d '%*.*s' %d\n",
                  hdc, x, y, count, count, lpstr, count );
    return TEXT_TabbedTextOut( hdc, x, y, lpstr, count, cTabStops,
                               lpTabPos, nTabOrg, TRUE );
}


/***********************************************************************
 *           GetTabbedTextExtent    (USER.197)
 */
DWORD GetTabbedTextExtent( HDC hdc, LPSTR lpstr, int count, 
                          int cTabStops, LPINT lpTabPos )
{
    dprintf_text( stddeb, "GetTabbedTextExtent: %x '%*.*s' %d\n",
                  hdc, count, count, lpstr, count );
    return TEXT_TabbedTextOut( hdc, 0, 0, lpstr, count, cTabStops,
                               lpTabPos, 0, FALSE );
}
