/*
 * text functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
 *
 */

#include <stdlib.h>
#include "windows.h"
#include "dc.h"
#include "gdi.h"
#include "heap.h"
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

static const char *TEXT_NextLine( HDC16 hdc, const char *str, int *count,
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
                if (str[++i] != PREFIX)
                {
                    prefix_offset = j;
                    break;
                }
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

    if (flags & DT_CALCRECT) flags |= DT_NOCLIP;

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
                HPEN32 hpen = CreatePen32( PS_SOLID, 1, GetTextColor32(hdc) );
                HPEN32 oldPen = SelectObject32( hdc, hpen );
                MoveTo(hdc, x + prefix_x, y + tm.tmAscent + 1 );
                LineTo32(hdc, x + prefix_end, y + tm.tmAscent + 1 );
                SelectObject32( hdc, oldPen );
                DeleteObject32( hpen );
            }
	}
	else if (size.cx > max_width)
	    max_width = size.cx;

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
    return y - rect->top;
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
    LPSTR p = HEAP_strdupWtoA( GetProcessHeap(), 0, str );
    INT32 ret = DrawText32A( hdc, p, count, rect, flags );
    HeapFree( GetProcessHeap(), 0, p );
    return ret;
}


/***********************************************************************
 *           ExtTextOut16    (GDI.351)
 */
BOOL16 ExtTextOut16( HDC16 hdc, INT16 x, INT16 y, UINT16 flags,
                     const RECT16 *lprect, LPCSTR str, UINT16 count,
                     const INT16 *lpDx )
{
    BOOL32	ret;
    int		i;
    RECT32	rect32;
    LPINT32	lpdx32 = lpDx?(LPINT32)xmalloc(sizeof(INT32)*count):NULL;

    if (lprect)	CONV_RECT16TO32(lprect,&rect32);
    if (lpdx32)	for (i=count;i--;) lpdx32[i]=lpDx[i];
    ret = ExtTextOut32A(hdc,x,y,flags,lprect?&rect32:NULL,str,count,lpdx32);
    if (lpdx32) free(lpdx32);
    return ret;


}


/***********************************************************************
 *           ExtTextOut32A    (GDI32.98)
 */
BOOL32 ExtTextOut32A( HDC32 hdc, INT32 x, INT32 y, UINT32 flags,
                      const RECT32 *lprect, LPCSTR str, UINT32 count,
                      const INT32 *lpDx )
{
    DC * dc = DC_GetDCPtr( hdc );
    return dc && dc->funcs->pExtTextOut && 
    	   dc->funcs->pExtTextOut(dc,x,y,flags,lprect,str,count,lpDx);
}


/***********************************************************************
 *           ExtTextOut32W    (GDI32.99)
 */
BOOL32 ExtTextOut32W( HDC32 hdc, INT32 x, INT32 y, UINT32 flags,
                      const RECT32 *lprect, LPCWSTR str, UINT32 count,
                      const INT32 *lpDx )
{
    LPSTR p = HEAP_strdupWtoA( GetProcessHeap(), 0, str );
    INT32 ret = ExtTextOut32A( hdc, x, y, flags, lprect, p, count, lpDx );
    HeapFree( GetProcessHeap(), 0, p );
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
 *           GrayString   (USER.185)
 */
BOOL GrayString(HDC16 hdc, HBRUSH16 hbr, GRAYSTRINGPROC16 gsprc, LPARAM lParam, 
		INT cch, INT x, INT y, INT cx, INT cy)
{
    BOOL ret;
    COLORREF current_color;

    if (!cch) cch = lstrlen16( (LPCSTR)PTR_SEG_TO_LIN(lParam) );
    if (gsprc) return gsprc( hdc, lParam, cch );
    current_color = GetTextColor32( hdc );
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
LONG TEXT_TabbedTextOut( HDC16 hdc, int x, int y, LPSTR lpstr, int count, 
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
                          GetBkMode32(hdc) == OPAQUE ? ETO_OPAQUE : 0,
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
LONG TabbedTextOut( HDC16 hdc, short x, short y, LPSTR lpstr, short count, 
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
DWORD GetTabbedTextExtent( HDC16 hdc, LPSTR lpstr, int count, 
                          int cTabStops, LPINT16 lpTabPos )
{
    dprintf_text( stddeb, "GetTabbedTextExtent: %04x '%*.*s' %d\n",
                  hdc, count, count, lpstr, count );
    return TEXT_TabbedTextOut( hdc, 0, 0, lpstr, count, cTabStops,
                               lpTabPos, 0, FALSE );
}
