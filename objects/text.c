/*
 * text functions
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Core.h>
#include <X11/Shell.h>
#include <X11/Xatom.h>

#include "message.h"
#include "callback.h"
#include "win.h"
#include "gdi.h"


/***********************************************************************
 *           DrawText    (USER.85)
 */
int DrawText( HDC hdc, LPSTR str, int count, LPRECT rect, WORD flags )
{
    int x = rect->left, y = rect->top;
    if (flags & DT_CENTER) x = (rect->left + rect->right) / 2;
    if (flags & DT_VCENTER) y = (rect->top + rect->bottom) / 2;
    if (count == -1) count = strlen(str);

    if (!TextOut( hdc, x, y, str, count )) return 0;
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
    if (!dc) return FALSE;
    if (!DC_SetupGCForText( dc )) return TRUE;
    font = dc->u.x.font.fstruct;

    if (dc->w.textAlign & TA_UPDATECP)
    {
	x = dc->w.CursPosX;
	y = dc->w.CursPosY;
    }
#ifdef DEBUG_TEXT
    printf( "TextOut: %d,%d '%s'\n", x, y, str );
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
			 x, y, str, count );
	else
	    XDrawImageString( XT_display, dc->u.x.drawable, dc->u.x.gc,
			      x, y, str, count );
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
			     xchar, y, p, 1 );
	    else
	    {
		XDrawImageString( XT_display, dc->u.x.drawable, dc->u.x.gc,
				  xchar, y, p, 1 );
		XSetForeground( XT_display, dc->u.x.gc, dc->w.backgroundPixel);
		XFillRectangle( XT_display, dc->u.x.drawable, dc->u.x.gc,
			        xchar + charStr->width, y - font->ascent,
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
		   x, y + linePos, x + info.width, y + linePos );
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
		   x, y - lineAscent, x + info.width, y - lineAscent );
    }
    
    return TRUE;
}
