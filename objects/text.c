/*
 * text functions
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include <X11/Xatom.h>
#include "windows.h"
#include "gdi.h"
#include "metafile.h"

#define TAB     9
#define LF     10
#define CR     13
#define SPACE  32
#define PREFIX 38

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
    int wb_i = 0, wb_j = 0, wb_count;

    while (*count)
    {
	switch (str[i])
	{
	case CR:
	case LF:
	    if (!(format & DT_SINGLELINE))
	    {
		i++;
		if (str[i] == CR || str[i] == LF)
		    i++;
		*len = j;
		return (&str[i]);
	    }
	    dest[j++] = str[i++];
	    if (!(format & DT_NOCLIP) || !(format & DT_NOPREFIX))
	    {
		if (!GetTextExtentPoint(hdc, &dest[j-1], 1, &size))
		    return NULL;
		plen += size.cx;
	    }
	    break;

	case PREFIX:
	    if (!(format & DT_NOPREFIX))
	    {
		prefix_offset = j + 1;
		i++;
	    }
	    else
	    {
		dest[j++] = str[i++];
		if (!(format & DT_NOCLIP))
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
		if (!(format & DT_NOCLIP) || !(format & DT_NOPREFIX))
		{
		    if (!GetTextExtentPoint(hdc, &dest[j-1], 1, &size))
			return NULL;
		    plen += size.cx;
		}
	    }
	    break;

	case SPACE:
	    dest[j++] = str[i++];
	    if (!(format & DT_NOCLIP) || !(format & DT_NOPREFIX))
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
	    if (!(format & DT_NOCLIP) || !(format & DT_NOPREFIX))
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
		    *len = wb_j;
		    *count = wb_count;
		    return (&str[wb_i]);
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
    int len, lh, prefix_x, prefix_len;
    TEXTMETRIC tm;
    int x = rect->left, y = rect->top;
    int width = rect->right - rect->left;

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
	    GetTextExtentPoint(hdc, line, prefix_offset - 1, &size);
	    prefix_x = size.cx;
	    GetTextExtentPoint(hdc, line + prefix_offset, 1, &size);
	    prefix_len = size.cx;
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
	    if (!TextOut(hdc, x, y, line, len)) return 0;
	if (prefix_offset != -1)
	{
	    MoveTo(hdc, x + prefix_x, y + size.cy);
	    LineTo(hdc, x + prefix_x + prefix_len, y + size.cy);
	}

	if (strPtr)
	{
	    y += lh;
	    if (!(flags & DT_NOCLIP))
	    {
		if (y > rect->bottom - lh)
		    break;
	    }
	}
    }
    while (strPtr);
    if (flags & DT_CALCRECT) rect->bottom = y;
    return 1;
}


/***********************************************************************
 *           TextOut    (GDI.33)
 */
BOOL TextOut( HDC hdc, short x, short y, LPSTR str, short count )
{
    int dir, ascent, descent, i;
    XCharStruct info;
    XFontStruct *font;

    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return FALSE;
	MF_TextOut(dc, x, y, str, count);
	return TRUE;
    }

    if (!DC_SetupGCForText( dc )) return TRUE;
    font = dc->u.x.font.fstruct;

    if (dc->w.textAlign & TA_UPDATECP)
    {
	x = dc->w.CursPosX;
	y = dc->w.CursPosY;
    }
#ifdef DEBUG_TEXT
    printf( "TextOut: %d,%d '%s', %d\n", x, y, str, count );
#endif
    x = XLPTODP( dc, x );
    y = YLPTODP( dc, y );

    XTextExtents( font, str, count, &dir, &ascent, &descent, &info );
    info.width += count*dc->w.charExtra + dc->w.breakExtra*dc->w.breakCount;

      /* Compute starting position */

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

      /* Draw text */

    if (!dc->w.charExtra && !dc->w.breakExtra)
    {
	if (dc->w.backgroundMode == TRANSPARENT)
	    XDrawString( XT_display, dc->u.x.drawable, dc->u.x.gc, 
			 dc->w.DCOrgX + x, dc->w.DCOrgY + y, str, count );
	else
	    XDrawImageString( XT_display, dc->u.x.drawable, dc->u.x.gc,
			      dc->w.DCOrgX + x, dc->w.DCOrgY + y, str, count );
    }
    else
    {
	char * p = str;
	int xchar = x;
	for (i = 0; i < count; i++, p++)
	{
	    XCharStruct * charStr;
	    unsigned char ch = *p;
	    int extraWidth;
	    
	    if ((ch < font->min_char_or_byte2)||(ch > font->max_char_or_byte2))
		ch = font->default_char;
	    if (!font->per_char) charStr = &font->min_bounds;
	    else charStr = font->per_char + ch - font->min_char_or_byte2;

	    extraWidth = dc->w.charExtra;
	    if (ch == dc->u.x.font.metrics.tmBreakChar)
		extraWidth += dc->w.breakExtra;

	    if (dc->w.backgroundMode == TRANSPARENT)
		XDrawString( XT_display, dc->u.x.drawable, dc->u.x.gc,
			     dc->w.DCOrgX + xchar, dc->w.DCOrgY + y, p, 1 );
	    else
	    {
		XDrawImageString( XT_display, dc->u.x.drawable, dc->u.x.gc,
				  dc->w.DCOrgX + xchar, dc->w.DCOrgY + y, p, 1 );
		XSetForeground( XT_display, dc->u.x.gc, dc->w.backgroundPixel);
		XFillRectangle( XT_display, dc->u.x.drawable, dc->u.x.gc,
			        dc->w.DCOrgX + xchar + charStr->width,
			        dc->w.DCOrgY + y - font->ascent,
			        extraWidth, font->ascent + font->descent );
		XSetForeground( XT_display, dc->u.x.gc, dc->w.textPixel );
	    }
	    xchar += charStr->width + extraWidth;
	}
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
	XSetLineAttributes( XT_display, dc->u.x.gc, lineWidth,
			    LineSolid, CapRound, JoinBevel ); 
	XDrawLine( XT_display, dc->u.x.drawable, dc->u.x.gc,
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
	XSetLineAttributes( XT_display, dc->u.x.gc, lineAscent + lineDescent,
			    LineSolid, CapRound, JoinBevel ); 
	XDrawLine( XT_display, dc->u.x.drawable, dc->u.x.gc,
		   dc->w.DCOrgX + x, dc->w.DCOrgY + y - lineAscent,
		   dc->w.DCOrgX + x + info.width, dc->w.DCOrgY + y - lineAscent );
    }
    
    return TRUE;
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
 *			ExtTextOut			[GDI.351]
 */
BOOL ExtTextOut(HDC hDC, short x, short y, WORD wOptions, LPRECT lprect,
			LPSTR str, WORD count, LPINT lpDx)
{
	printf("EMPTY STUB !!! ExtTextOut(); ! call TextOut() for now !\n");
	TextOut(hDC, x, y, str, count);
	return FALSE;
}



