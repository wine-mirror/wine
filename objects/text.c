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
#include "debug.h"
#include "cache.h"

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
		if ((*count > 1) && (str[i] == CR) && (str[i+1] == LF))
                {
		    (*count)--;
                    i++;
                }
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
	    if (!(format & DT_NOPREFIX) && *count > 1)
                {
                if (str[++i] == PREFIX)
		    (*count)--;
		else {
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
INT16 WINAPI DrawText16( HDC16 hdc, LPCSTR str, INT16 i_count,
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

    dprintf_info(text,"DrawText: '%s', %d , [(%d,%d),(%d,%d)]\n", str,
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
                LineTo32(hdc, x + prefix_end + 1, y + tm.tmAscent + 1 );
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
INT32 WINAPI DrawText32A( HDC32 hdc, LPCSTR str, INT32 count,
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
INT32 WINAPI DrawText32W( HDC32 hdc, LPCWSTR str, INT32 count,
                          LPRECT32 rect, UINT32 flags )
{
    LPSTR p = HEAP_strdupWtoA( GetProcessHeap(), 0, str );
    INT32 ret = DrawText32A( hdc, p, count, rect, flags );
    HeapFree( GetProcessHeap(), 0, p );
    return ret;
}

/***********************************************************************
 *           DrawTextEx32A    (USER32.164)
 */
INT32 DrawTextEx32A( HDC32 hdc, LPCSTR str, INT32 count,
                     LPRECT32 rect, UINT32 flags, LPDRAWTEXTPARAMS dtp )
{
    fprintf(stderr,"DrawTextEx32A(%d,'%s',%d,%p,0x%08x,%p)\n",
    	hdc,str,count,rect,flags,dtp
    );
    /*FIXME: ignores extended functionality ... */
    return DrawText32A(hdc,str,count,rect,flags);
}

/***********************************************************************
 *           DrawTextEx32W    (USER32.165)
 */
INT32 DrawTextEx32W( HDC32 hdc, LPCWSTR str, INT32 count,
                     LPRECT32 rect, UINT32 flags, LPDRAWTEXTPARAMS dtp )
{
    fprintf(stderr,"DrawTextEx32A(%d,%p,%d,%p,0x%08x,%p)\n",
    	hdc,str,count,rect,flags,dtp
    );
    /*FIXME: ignores extended functionality ... */
    return DrawText32W(hdc,str,count,rect,flags);
}

/***********************************************************************
 *           ExtTextOut16    (GDI.351)
 */
BOOL16 WINAPI ExtTextOut16( HDC16 hdc, INT16 x, INT16 y, UINT16 flags,
                            const RECT16 *lprect, LPCSTR str, UINT16 count,
                            const INT16 *lpDx )
{
    BOOL32	ret;
    int		i;
    RECT32	rect32;
    LPINT32	lpdx32 = NULL;

    if (lpDx) lpdx32 = (LPINT32)HEAP_xalloc( GetProcessHeap(), 0,
                                             sizeof(INT32)*count );
    if (lprect)	CONV_RECT16TO32(lprect,&rect32);
    if (lpdx32)	for (i=count;i--;) lpdx32[i]=lpDx[i];
    ret = ExtTextOut32A(hdc,x,y,flags,lprect?&rect32:NULL,str,count,lpdx32);
    if (lpdx32) HeapFree( GetProcessHeap(), 0, lpdx32 );
    return ret;


}


/***********************************************************************
 *           ExtTextOut32A    (GDI32.98)
 */
BOOL32 WINAPI ExtTextOut32A( HDC32 hdc, INT32 x, INT32 y, UINT32 flags,
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
BOOL32 WINAPI ExtTextOut32W( HDC32 hdc, INT32 x, INT32 y, UINT32 flags,
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
BOOL16 WINAPI TextOut16( HDC16 hdc, INT16 x, INT16 y, LPCSTR str, INT16 count )
{
    return ExtTextOut16( hdc, x, y, 0, NULL, str, count, NULL );
}


/***********************************************************************
 *           TextOut32A    (GDI32.355)
 */
BOOL32 WINAPI TextOut32A( HDC32 hdc, INT32 x, INT32 y, LPCSTR str, INT32 count )
{
    return ExtTextOut32A( hdc, x, y, 0, NULL, str, count, NULL );
}


/***********************************************************************
 *           TextOut32W    (GDI32.356)
 */
BOOL32 WINAPI TextOut32W(HDC32 hdc, INT32 x, INT32 y, LPCWSTR str, INT32 count)
{
    return ExtTextOut32W( hdc, x, y, 0, NULL, str, count, NULL );
}


/***********************************************************************
 *           TEXT_GrayString
 *
 * FIXME: The call to 16-bit code only works because the wine GDI is a 16-bit
 * heap and we can guarantee that the handles fit in an INT16. We have to
 * rethink the strategy once the migration to NT handles is complete.
 * We are going to get a lot of code-duplication once this migration is
 * completed...
 * 
 */
static BOOL32 TEXT_GrayString(HDC32 hdc, HBRUSH32 hb, 
                              GRAYSTRINGPROC32 fn, LPARAM lp, INT32 len,
                              INT32 x, INT32 y, INT32 cx, INT32 cy, 
                              BOOL32 unicode, BOOL32 _32bit)
{
    HBITMAP32 hbm, hbmsave;
    HBRUSH32 hbsave;
    HFONT32 hfsave;
    HDC32 memdc = CreateCompatibleDC32(hdc);
    int slen = len;
    BOOL32 retval = TRUE;
    RECT32 r;
    COLORREF fg, bg;

    if(!hdc) return FALSE;
    
    if(len == 0)
    {
        if(unicode)
    	    slen = lstrlen32W((LPCWSTR)lp);
    	else if(_32bit)
    	    slen = lstrlen32A((LPCSTR)lp);
    	else
    	    slen = lstrlen32A((LPCSTR)PTR_SEG_TO_LIN(lp));
    }

    if((cx == 0 || cy == 0) && slen != -1)
    {
        SIZE32 s;
        if(unicode)
            GetTextExtentPoint32W(hdc, (LPCWSTR)lp, slen, &s);
        else if(_32bit)
            GetTextExtentPoint32A(hdc, (LPCSTR)lp, slen, &s);
        else
            GetTextExtentPoint32A(hdc, (LPCSTR)PTR_SEG_TO_LIN(lp), slen, &s);
        if(cx == 0) cx = s.cx;
        if(cy == 0) cy = s.cy;
    }

    r.left = r.top = 0;
    r.right = cx;
    r.bottom = cy;

    hbm = CreateBitmap32(cx, cy, 1, 1, NULL);
    hbmsave = (HBITMAP32)SelectObject32(memdc, hbm);
    FillRect32(memdc, &r, (HBRUSH32)GetStockObject32(BLACK_BRUSH));
    SetTextColor32(memdc, RGB(255, 255, 255));
    SetBkColor32(memdc, RGB(0, 0, 0));
    hfsave = (HFONT32)SelectObject32(memdc, GetCurrentObject(hdc, OBJ_FONT));
            
    if(fn)
        if(_32bit)
            retval = fn(memdc, lp, slen);
        else
            retval = (BOOL32)((BOOL16)((GRAYSTRINGPROC16)fn)((HDC16)memdc, lp, (INT16)slen));
    else
        if(unicode)
            TextOut32W(memdc, 0, 0, (LPCWSTR)lp, slen);
        else if(_32bit)
            TextOut32A(memdc, 0, 0, (LPCSTR)lp, slen);
        else
            TextOut32A(memdc, 0, 0, (LPCSTR)PTR_SEG_TO_LIN(lp), slen);

    SelectObject32(memdc, hfsave);

/*
 * Windows doc says that the bitmap isn't grayed when len == -1 and
 * the callback function returns FALSE. However, testing this on
 * win95 showed otherwise...
*/
#ifdef GRAYSTRING_USING_DOCUMENTED_BEHAVIOUR
    if(retval || len != -1)
#endif
    {
        hbsave = (HBRUSH32)SelectObject32(memdc, CACHE_GetPattern55AABrush());
        PatBlt32(memdc, 0, 0, cx, cy, 0x000A0329);
        SelectObject32(memdc, hbsave);
    }

    if(hb) hbsave = (HBRUSH32)SelectObject32(hdc, hb);
    fg = SetTextColor32(hdc, RGB(0, 0, 0));
    bg = SetBkColor32(hdc, RGB(255, 255, 255));
    BitBlt32(hdc, x, y, cx, cy, memdc, 0, 0, 0x00E20746);
    SetTextColor32(hdc, fg);
    SetBkColor32(hdc, bg);
    if(hb) SelectObject32(hdc, hbsave);

    SelectObject32(memdc, hbmsave);
    DeleteObject32(hbm);
    DeleteDC32(memdc);
    return retval;
}


/***********************************************************************
 *           GrayString16   (USER.185)
 */
BOOL16 WINAPI GrayString16( HDC16 hdc, HBRUSH16 hbr, GRAYSTRINGPROC16 gsprc,
                            LPARAM lParam, INT16 cch, INT16 x, INT16 y,
                            INT16 cx, INT16 cy )
{
    return TEXT_GrayString(hdc, hbr, (GRAYSTRINGPROC32)gsprc, lParam, cch, x, y, cx, cy, FALSE, FALSE);
}


/***********************************************************************
 *           GrayString32A   (USER32.314)
 */
BOOL32 WINAPI GrayString32A( HDC32 hdc, HBRUSH32 hbr, GRAYSTRINGPROC32 gsprc,
                             LPARAM lParam, INT32 cch, INT32 x, INT32 y,
                             INT32 cx, INT32 cy )
{
    return TEXT_GrayString(hdc, hbr, gsprc, lParam, cch, x, y, cx, cy, FALSE, TRUE);
}


/***********************************************************************
 *           GrayString32W   (USER32.315)
 */
BOOL32 WINAPI GrayString32W( HDC32 hdc, HBRUSH32 hbr, GRAYSTRINGPROC32 gsprc,
                             LPARAM lParam, INT32 cch, INT32 x, INT32 y,
                             INT32 cx, INT32 cy )
{
    return TEXT_GrayString(hdc, hbr, gsprc, lParam, cch, x, y, cx, cy, TRUE, TRUE);
}


/***********************************************************************
 *           TEXT_TabbedTextOut
 *
 * Helper function for TabbedTextOut() and GetTabbedTextExtent().
 * Note: this doesn't work too well for text-alignment modes other
 *       than TA_LEFT|TA_TOP. But we want bug-for-bug compatibility :-)
 */
LONG TEXT_TabbedTextOut( HDC32 hdc, INT32 x, INT32 y, LPCSTR lpstr,
                         INT32 count, INT32 cTabStops, const INT16 *lpTabPos16,
                         const INT32 *lpTabPos32, INT32 nTabOrg,
                         BOOL32 fDisplayText )
{
    INT32 defWidth;
    DWORD extent = 0;
    int i, tabPos = x;
    int start = x;

    if (cTabStops == 1)
    {
        defWidth = lpTabPos32 ? *lpTabPos32 : *lpTabPos16;
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
        if (lpTabPos32)
        {
            while ((cTabStops > 0) &&
                   (nTabOrg + *lpTabPos32 <= x + LOWORD(extent)))
            {
                lpTabPos32++;
                cTabStops--;
            }
        }
        else
        {
            while ((cTabStops > 0) &&
                   (nTabOrg + *lpTabPos16 <= x + LOWORD(extent)))
            {
                lpTabPos16++;
                cTabStops--;
            }
        }
        if (i == count)
            tabPos = x + LOWORD(extent);
        else if (cTabStops > 0)
            tabPos = nTabOrg + (lpTabPos32 ? *lpTabPos32 : *lpTabPos16);
        else
            tabPos = nTabOrg + ((x + LOWORD(extent) - nTabOrg) / defWidth + 1) * defWidth;
        if (fDisplayText)
        {
            RECT32 r;
            SetRect32( &r, x, y, tabPos, y+HIWORD(extent) );
            ExtTextOut32A( hdc, x, y,
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
 *           TabbedTextOut16    (USER.196)
 */
LONG WINAPI TabbedTextOut16( HDC16 hdc, INT16 x, INT16 y, LPCSTR lpstr,
                             INT16 count, INT16 cTabStops,
                             const INT16 *lpTabPos, INT16 nTabOrg )
{
    dprintf_info(text, "TabbedTextOut16: %04x %d,%d '%.*s' %d\n",
                  hdc, x, y, count, lpstr, count );
    return TEXT_TabbedTextOut( hdc, x, y, lpstr, count, cTabStops,
                               lpTabPos, NULL, nTabOrg, TRUE );
}


/***********************************************************************
 *           TabbedTextOut32A    (USER32.541)
 */
LONG WINAPI TabbedTextOut32A( HDC32 hdc, INT32 x, INT32 y, LPCSTR lpstr,
                              INT32 count, INT32 cTabStops,
                              const INT32 *lpTabPos, INT32 nTabOrg )
{
    dprintf_info(text, "TabbedTextOut32A: %04x %d,%d '%.*s' %d\n",
                  hdc, x, y, count, lpstr, count );
    return TEXT_TabbedTextOut( hdc, x, y, lpstr, count, cTabStops,
                               NULL, lpTabPos, nTabOrg, TRUE );
}


/***********************************************************************
 *           TabbedTextOut32W    (USER32.542)
 */
LONG WINAPI TabbedTextOut32W( HDC32 hdc, INT32 x, INT32 y, LPCWSTR str,
                              INT32 count, INT32 cTabStops,
                              const INT32 *lpTabPos, INT32 nTabOrg )
{
    LONG ret;
    LPSTR p = HEAP_xalloc( GetProcessHeap(), 0, count + 1 );
    lstrcpynWtoA( p, str, count + 1 );
    ret = TabbedTextOut32A( hdc, x, y, p, count, cTabStops,
                            lpTabPos, nTabOrg );
    HeapFree( GetProcessHeap(), 0, p );
    return ret;
}


/***********************************************************************
 *           GetTabbedTextExtent16    (USER.197)
 */
DWORD WINAPI GetTabbedTextExtent16( HDC16 hdc, LPCSTR lpstr, INT16 count, 
                                    INT16 cTabStops, const INT16 *lpTabPos )
{
    dprintf_info(text, "GetTabbedTextExtent: %04x '%.*s' %d\n",
                  hdc, count, lpstr, count );
    return TEXT_TabbedTextOut( hdc, 0, 0, lpstr, count, cTabStops,
                               lpTabPos, NULL, 0, FALSE );
}


/***********************************************************************
 *           GetTabbedTextExtent32A    (USER32.292)
 */
DWORD WINAPI GetTabbedTextExtent32A( HDC32 hdc, LPCSTR lpstr, INT32 count, 
                                     INT32 cTabStops, const INT32 *lpTabPos )
{
    dprintf_info(text, "GetTabbedTextExtent: %04x '%.*s' %d\n",
                  hdc, count, lpstr, count );
    return TEXT_TabbedTextOut( hdc, 0, 0, lpstr, count, cTabStops,
                               NULL, lpTabPos, 0, FALSE );
}


/***********************************************************************
 *           GetTabbedTextExtent32W    (USER32.293)
 */
DWORD WINAPI GetTabbedTextExtent32W( HDC32 hdc, LPCWSTR lpstr, INT32 count, 
                                     INT32 cTabStops, const INT32 *lpTabPos )
{
    LONG ret;
    LPSTR p = HEAP_xalloc( GetProcessHeap(), 0, count + 1 );
    lstrcpynWtoA( p, lpstr, count + 1 );
    ret = GetTabbedTextExtent32A( hdc, p, count, cTabStops, lpTabPos );
    HeapFree( GetProcessHeap(), 0, p );
    return ret;
}

/***********************************************************************
 *           GetTextCharset    (GDI32.226) (GDI.612)
 */
INT32 WINAPI GetTextCharset32(HDC32 hdc)
{
    fprintf(stdnimp,"GetTextCharset(0x%x)\n",hdc);
    return DEFAULT_CHARSET; /* FIXME */
}

INT16 WINAPI GetTextCharset16(HDC16 hdc)
{
    return GetTextCharset32(hdc);
}

/***********************************************************************
 *           GetTextCharsetInfo    (GDI32.381)
 */
INT32 WINAPI GetTextCharsetInfo(HDC32 hdc,LPCHARSETINFO csi,DWORD flags)
{
    fprintf(stdnimp,"GetTextCharsetInfo(0x%x,%p,%08lx), stub!\n",hdc,csi,flags);
    if (csi) {
	csi->ciCharset = DEFAULT_CHARSET;
	csi->ciACP = GetACP();
    }
    /* ... fill fontstruct too ... */
    return DEFAULT_CHARSET;
}
