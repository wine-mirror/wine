/*
 * text functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
 *
 */

#include <string.h>

#include "windef.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "dc.h"
#include "gdi.h"
#include "heap.h"
#include "debugtools.h"
#include "cache.h"
#include "winnls.h"

DEFAULT_DEBUG_CHANNEL(text);

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

    TRACE("%s, %d , [(%d,%d),(%d,%d)]\n",
	  debugstr_an (str, count), count,
	  rect->left, rect->top, rect->right, rect->bottom);

    if (!str) return 0;
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
                HPEN hpen = CreatePen( PS_SOLID, 1, GetTextColor(hdc) );
                HPEN oldPen = SelectObject( hdc, hpen );
                MoveTo16(hdc, x + prefix_x, y + tm.tmAscent + 1 );
                LineTo(hdc, x + prefix_end + 1, y + tm.tmAscent + 1 );
                SelectObject( hdc, oldPen );
                DeleteObject( hpen );
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
 *           DrawText32A    (USER32.164)
 */
INT WINAPI DrawTextA( HDC hdc, LPCSTR str, INT count,
                          LPRECT rect, UINT flags )
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
 *           DrawText32W    (USER32.167)
 */
INT WINAPI DrawTextW( HDC hdc, LPCWSTR str, INT count,
                          LPRECT rect, UINT flags )
{
    LPSTR p = HEAP_strdupWtoA( GetProcessHeap(), 0, str );
    INT ret = DrawTextA( hdc, p, count, rect, flags );
    HeapFree( GetProcessHeap(), 0, p );
    return ret;
}

/***********************************************************************
 *           DrawTextEx32A    (USER32.165)
 */
INT WINAPI DrawTextExA( HDC hdc, LPCSTR str, INT count,
                     LPRECT rect, UINT flags, LPDRAWTEXTPARAMS dtp )
{
    TRACE("(%d,'%s',%d,%p,0x%08x,%p)\n",hdc,str,count,rect,flags,dtp);
    if(dtp) {
        FIXME("Ignores params:%d,%d,%d,%d,%d\n",dtp->cbSize,
                   dtp->iTabLength,dtp->iLeftMargin,dtp->iRightMargin,
                   dtp->uiLengthDrawn);
    }
    return DrawTextA(hdc,str,count,rect,flags);
}

/***********************************************************************
 *           DrawTextEx32W    (USER32.166)
 */
INT WINAPI DrawTextExW( HDC hdc, LPCWSTR str, INT count,
                     LPRECT rect, UINT flags, LPDRAWTEXTPARAMS dtp )
{
    TRACE("(%d,%p,%d,%p,0x%08x,%p)\n",hdc,str,count,rect,flags,dtp);
    FIXME("ignores extended functionality\n");
    return DrawTextW(hdc,str,count,rect,flags);
}

/***********************************************************************
 *           ExtTextOut16    (GDI.351)
 */
BOOL16 WINAPI ExtTextOut16( HDC16 hdc, INT16 x, INT16 y, UINT16 flags,
                            const RECT16 *lprect, LPCSTR str, UINT16 count,
                            const INT16 *lpDx )
{
    BOOL	ret;
    int		i;
    RECT	rect32;
    LPINT	lpdx32 = NULL;

    if (lpDx) lpdx32 = (LPINT)HEAP_xalloc( GetProcessHeap(), 0,
                                             sizeof(INT)*count );
    if (lprect)	CONV_RECT16TO32(lprect,&rect32);
    if (lpdx32)	for (i=count;i--;) lpdx32[i]=lpDx[i];
    ret = ExtTextOutA(hdc,x,y,flags,lprect?&rect32:NULL,str,count,lpdx32);
    if (lpdx32) HeapFree( GetProcessHeap(), 0, lpdx32 );
    return ret;


}


/***********************************************************************
 *           ExtTextOutA    (GDI32.98)
 */
BOOL WINAPI ExtTextOutA( HDC hdc, INT x, INT y, UINT flags,
                             const RECT *lprect, LPCSTR str, UINT count,
                             const INT *lpDx )
{
    LPWSTR p;
    INT ret;
    UINT codepage = CP_ACP; /* FIXME: get codepage of font charset */
    UINT wlen;

    wlen = MultiByteToWideChar(codepage,0,str,count,NULL,0);
    p = HeapAlloc( GetProcessHeap(), 0, wlen * sizeof(WCHAR) );
    wlen = MultiByteToWideChar(codepage,0,str,count,p,wlen);

    ret = ExtTextOutW( hdc, x, y, flags, lprect, p, wlen, lpDx );
    HeapFree( GetProcessHeap(), 0, p );
    return ret;
}


/***********************************************************************
 *           ExtTextOutW    (GDI32.99)
 */
BOOL WINAPI ExtTextOutW( HDC hdc, INT x, INT y, UINT flags,
                             const RECT *lprect, LPCWSTR str, UINT count,
                             const INT *lpDx )
{
    DC * dc = DC_GetDCPtr( hdc );
    return dc && dc->funcs->pExtTextOut && 
        dc->funcs->pExtTextOut(dc,x,y,flags,lprect,str,count,lpDx);
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
BOOL WINAPI TextOutA( HDC hdc, INT x, INT y, LPCSTR str, INT count )
{
    return ExtTextOutA( hdc, x, y, 0, NULL, str, count, NULL );
}


/***********************************************************************
 *           TextOut32W    (GDI32.356)
 */
BOOL WINAPI TextOutW(HDC hdc, INT x, INT y, LPCWSTR str, INT count)
{
    return ExtTextOutW( hdc, x, y, 0, NULL, str, count, NULL );
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
static BOOL TEXT_GrayString(HDC hdc, HBRUSH hb, 
                              GRAYSTRINGPROC fn, LPARAM lp, INT len,
                              INT x, INT y, INT cx, INT cy, 
                              BOOL unicode, BOOL _32bit)
{
    HBITMAP hbm, hbmsave;
    HBRUSH hbsave;
    HFONT hfsave;
    HDC memdc = CreateCompatibleDC(hdc);
    int slen = len;
    BOOL retval = TRUE;
    COLORREF fg, bg;

    if(!hdc) return FALSE;
    
    if(len == 0)
    {
        if(unicode)
    	    slen = lstrlenW((LPCWSTR)lp);
    	else if(_32bit)
    	    slen = lstrlenA((LPCSTR)lp);
    	else
    	    slen = lstrlenA((LPCSTR)PTR_SEG_TO_LIN(lp));
    }

    if((cx == 0 || cy == 0) && slen != -1)
    {
        SIZE s;
        if(unicode)
            GetTextExtentPoint32W(hdc, (LPCWSTR)lp, slen, &s);
        else if(_32bit)
            GetTextExtentPoint32A(hdc, (LPCSTR)lp, slen, &s);
        else
            GetTextExtentPoint32A(hdc, (LPCSTR)PTR_SEG_TO_LIN(lp), slen, &s);
        if(cx == 0) cx = s.cx;
        if(cy == 0) cy = s.cy;
    }

    hbm = CreateBitmap(cx, cy, 1, 1, NULL);
    hbmsave = (HBITMAP)SelectObject(memdc, hbm);
    hbsave = SelectObject( memdc, GetStockObject(BLACK_BRUSH) );
    PatBlt( memdc, 0, 0, cx, cy, PATCOPY );
    SelectObject( memdc, hbsave );
    SetTextColor(memdc, RGB(255, 255, 255));
    SetBkColor(memdc, RGB(0, 0, 0));
    hfsave = (HFONT)SelectObject(memdc, GetCurrentObject(hdc, OBJ_FONT));
            
    if(fn)
        if(_32bit)
            retval = fn(memdc, lp, slen);
        else
            retval = (BOOL)((BOOL16)((GRAYSTRINGPROC16)fn)((HDC16)memdc, lp, (INT16)slen));
    else
        if(unicode)
            TextOutW(memdc, 0, 0, (LPCWSTR)lp, slen);
        else if(_32bit)
            TextOutA(memdc, 0, 0, (LPCSTR)lp, slen);
        else
            TextOutA(memdc, 0, 0, (LPCSTR)PTR_SEG_TO_LIN(lp), slen);

    SelectObject(memdc, hfsave);

/*
 * Windows doc says that the bitmap isn't grayed when len == -1 and
 * the callback function returns FALSE. However, testing this on
 * win95 showed otherwise...
*/
#ifdef GRAYSTRING_USING_DOCUMENTED_BEHAVIOUR
    if(retval || len != -1)
#endif
    {
        hbsave = (HBRUSH)SelectObject(memdc, CACHE_GetPattern55AABrush());
        PatBlt(memdc, 0, 0, cx, cy, 0x000A0329);
        SelectObject(memdc, hbsave);
    }

    if(hb) hbsave = (HBRUSH)SelectObject(hdc, hb);
    fg = SetTextColor(hdc, RGB(0, 0, 0));
    bg = SetBkColor(hdc, RGB(255, 255, 255));
    BitBlt(hdc, x, y, cx, cy, memdc, 0, 0, 0x00E20746);
    SetTextColor(hdc, fg);
    SetBkColor(hdc, bg);
    if(hb) SelectObject(hdc, hbsave);

    SelectObject(memdc, hbmsave);
    DeleteObject(hbm);
    DeleteDC(memdc);
    return retval;
}


/***********************************************************************
 *           GrayString16   (USER.185)
 */
BOOL16 WINAPI GrayString16( HDC16 hdc, HBRUSH16 hbr, GRAYSTRINGPROC16 gsprc,
                            LPARAM lParam, INT16 cch, INT16 x, INT16 y,
                            INT16 cx, INT16 cy )
{
    return TEXT_GrayString(hdc, hbr, (GRAYSTRINGPROC)gsprc, lParam, cch, x, y, cx, cy, FALSE, FALSE);
}


/***********************************************************************
 *           GrayString32A   (USER32.315)
 */
BOOL WINAPI GrayStringA( HDC hdc, HBRUSH hbr, GRAYSTRINGPROC gsprc,
                             LPARAM lParam, INT cch, INT x, INT y,
                             INT cx, INT cy )
{
    return TEXT_GrayString(hdc, hbr, gsprc, lParam, cch, x, y, cx, cy, FALSE, TRUE);
}


/***********************************************************************
 *           GrayString32W   (USER32.316)
 */
BOOL WINAPI GrayStringW( HDC hdc, HBRUSH hbr, GRAYSTRINGPROC gsprc,
                             LPARAM lParam, INT cch, INT x, INT y,
                             INT cx, INT cy )
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
LONG TEXT_TabbedTextOut( HDC hdc, INT x, INT y, LPCSTR lpstr,
                         INT count, INT cTabStops, const INT16 *lpTabPos16,
                         const INT *lpTabPos32, INT nTabOrg,
                         BOOL fDisplayText )
{
    INT defWidth;
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
        extent = GetTextExtent16( hdc, lpstr, i );
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
            RECT r;
            r.left   = x;
            r.top    = y;
            r.right  = tabPos;
            r.bottom = y + HIWORD(extent);
            ExtTextOutA( hdc, x, y,
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
 *           TabbedTextOut16    (USER.196)
 */
LONG WINAPI TabbedTextOut16( HDC16 hdc, INT16 x, INT16 y, LPCSTR lpstr,
                             INT16 count, INT16 cTabStops,
                             const INT16 *lpTabPos, INT16 nTabOrg )
{
    TRACE("%04x %d,%d '%.*s' %d\n",
                  hdc, x, y, count, lpstr, count );
    return TEXT_TabbedTextOut( hdc, x, y, lpstr, count, cTabStops,
                               lpTabPos, NULL, nTabOrg, TRUE );
}


/***********************************************************************
 *           TabbedTextOut32A    (USER32.542)
 */
LONG WINAPI TabbedTextOutA( HDC hdc, INT x, INT y, LPCSTR lpstr,
                              INT count, INT cTabStops,
                              const INT *lpTabPos, INT nTabOrg )
{
    TRACE("%04x %d,%d '%.*s' %d\n",
                  hdc, x, y, count, lpstr, count );
    return TEXT_TabbedTextOut( hdc, x, y, lpstr, count, cTabStops,
                               NULL, lpTabPos, nTabOrg, TRUE );
}


/***********************************************************************
 *           TabbedTextOut32W    (USER32.543)
 */
LONG WINAPI TabbedTextOutW( HDC hdc, INT x, INT y, LPCWSTR str,
                              INT count, INT cTabStops,
                              const INT *lpTabPos, INT nTabOrg )
{
    LONG ret;
    LPSTR p;
    INT acount;
    UINT codepage = CP_ACP; /* FIXME: get codepage of font charset */

    acount = WideCharToMultiByte(codepage,0,str,count,NULL,0,NULL,NULL);
    p = HEAP_xalloc( GetProcessHeap(), 0, acount ); 
    acount = WideCharToMultiByte(codepage,0,str,count,p,acount,NULL,NULL);
    ret = TabbedTextOutA( hdc, x, y, p, acount, cTabStops,
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
    TRACE("%04x '%.*s' %d\n",
                  hdc, count, lpstr, count );
    return TEXT_TabbedTextOut( hdc, 0, 0, lpstr, count, cTabStops,
                               lpTabPos, NULL, 0, FALSE );
}


/***********************************************************************
 *           GetTabbedTextExtent32A    (USER32.293)
 */
DWORD WINAPI GetTabbedTextExtentA( HDC hdc, LPCSTR lpstr, INT count, 
                                     INT cTabStops, const INT *lpTabPos )
{
    TRACE("%04x '%.*s' %d\n",
                  hdc, count, lpstr, count );
    return TEXT_TabbedTextOut( hdc, 0, 0, lpstr, count, cTabStops,
                               NULL, lpTabPos, 0, FALSE );
}


/***********************************************************************
 *           GetTabbedTextExtent32W    (USER32.294)
 */
DWORD WINAPI GetTabbedTextExtentW( HDC hdc, LPCWSTR lpstr, INT count, 
                                     INT cTabStops, const INT *lpTabPos )
{
    LONG ret;
    LPSTR p;
    INT acount;
    UINT codepage = CP_ACP; /* FIXME: get codepage of font charset */

    acount = WideCharToMultiByte(codepage,0,lpstr,count,NULL,0,NULL,NULL);
    p = HEAP_xalloc( GetProcessHeap(), 0, acount );
    acount = WideCharToMultiByte(codepage,0,lpstr,count,p,acount,NULL,NULL);
    ret = GetTabbedTextExtentA( hdc, p, acount, cTabStops, lpTabPos );
    HeapFree( GetProcessHeap(), 0, p );
    return ret;
}

/***********************************************************************
 * GetTextCharset32 [GDI32.226]  Gets character set for font in DC
 *
 * NOTES
 *    Should it return a UINT32 instead of an INT32?
 *    => YES, as GetTextCharsetInfo returns UINT32
 *
 * RETURNS
 *    Success: Character set identifier
 *    Failure: DEFAULT_CHARSET
 */
UINT WINAPI GetTextCharset(
    HDC hdc) /* [in] Handle to device context */
{
    /* MSDN docs say this is equivalent */
    return GetTextCharsetInfo(hdc, NULL, 0);
}

/***********************************************************************
 * GetTextCharset16 [GDI.612]
 */
UINT16 WINAPI GetTextCharset16(HDC16 hdc)
{
    return (UINT16)GetTextCharset(hdc);
}

/***********************************************************************
 * GetTextCharsetInfo [GDI32.381]  Gets character set for font
 *
 * NOTES
 *    Should csi be an LPFONTSIGNATURE instead of an LPCHARSETINFO?
 *    Should it return a UINT32 instead of an INT32?
 *    => YES and YES, from win32.hlp from Borland
 *
 * RETURNS
 *    Success: Character set identifier
 *    Failure: DEFAULT_CHARSET
 */
UINT WINAPI GetTextCharsetInfo(
    HDC hdc,          /* [in]  Handle to device context */
    LPFONTSIGNATURE fs, /* [out] Pointer to struct to receive data */
    DWORD flags)        /* [in]  Reserved - must be 0 */
{
    HGDIOBJ hFont;
    UINT charSet = DEFAULT_CHARSET;
    LOGFONTW lf;
    CHARSETINFO csinfo;

    hFont = GetCurrentObject(hdc, OBJ_FONT);
    if (hFont == 0)
        return(DEFAULT_CHARSET);
    if ( GetObjectW(hFont, sizeof(LOGFONTW), &lf) != 0 )
        charSet = lf.lfCharSet;

    if (fs != NULL) {
      if (!TranslateCharsetInfo((LPDWORD)charSet, &csinfo, TCI_SRCCHARSET))
           return  (DEFAULT_CHARSET);
      memcpy(fs, &csinfo.fs, sizeof(FONTSIGNATURE));
    }
    return charSet;
}

/***********************************************************************
 * PolyTextOutA [GDI.402]  Draw several Strings
 */
BOOL WINAPI PolyTextOutA (
			  HDC hdc,               /* Handle to device context */			  
			  PPOLYTEXTA pptxt,      /* array of strings */
			  INT cStrings           /* Number of strings in array */
			  )
{
  FIXME("stub!\n");
  SetLastError ( ERROR_CALL_NOT_IMPLEMENTED );
  return 0;
}



/***********************************************************************
 * PolyTextOutW [GDI.403] Draw several Strings
 */
BOOL WINAPI PolyTextOutW ( 
			  HDC hdc,               /* Handle to device context */			  
			  PPOLYTEXTW pptxt,      /* array of strings */
			  INT cStrings           /* Number of strings in array */
			  )
{
  FIXME("stub!\n");
  SetLastError ( ERROR_CALL_NOT_IMPLEMENTED );
  return 0;
}
