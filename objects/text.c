/*
 * text functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
 * Copyright 2003 Shachar Shemesh
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

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "winerror.h"
#include "winnls.h"
#include "gdi.h"
#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(text);

/***********************************************************************
 *           FONT_mbtowc
 *
 * Returns a '\0' terminated Unicode translation of str using the
 * charset of the currently selected font in hdc.  If count is -1 then
 * str is assumed to be '\0' terminated, otherwise it contains the
 * number of bytes to convert.  If plenW is non-NULL, on return it
 * will point to the number of WCHARs (excluding the '\0') that have
 * been written.  If pCP is non-NULL, on return it will point to the
 * codepage used in the conversion (NB, this may be CP_SYMBOL so watch
 * out).  The caller should free the returned LPWSTR from the process
 * heap itself.
 */
LPWSTR FONT_mbtowc(HDC hdc, LPCSTR str, INT count, INT *plenW, UINT *pCP)
{
    UINT cp = CP_ACP;
    INT lenW, i;
    LPWSTR strW;
    CHARSETINFO csi;
    int charset = GetTextCharset(hdc);

    /* Hmm, nicely designed api this one! */
    if(TranslateCharsetInfo((DWORD*)charset, &csi, TCI_SRCCHARSET))
        cp = csi.ciACP;
    else {
        switch(charset) {
	case OEM_CHARSET:
	    cp = GetOEMCP();
	    break;
	case DEFAULT_CHARSET:
	    cp = GetACP();
	    break;

	case VISCII_CHARSET:
	case TCVN_CHARSET:
	case KOI8_CHARSET:
	case ISO3_CHARSET:
	case ISO4_CHARSET:
	case ISO10_CHARSET:
	case CELTIC_CHARSET:
	  /* FIXME: These have no place here, but because x11drv
	     enumerates fonts with these (made up) charsets some apps
	     might use them and then the FIXME below would become
	     annoying.  Now we could pick the intended codepage for
	     each of these, but since it's broken anyway we'll just
	     use CP_ACP and hope it'll go away...
	  */
	    cp = CP_ACP;
	    break;


	default:
	    FIXME("Can't find codepage for charset %d\n", charset);
	    break;
	}
    }

    TRACE("cp == %d\n", cp);

    if(count == -1) count = strlen(str);
    if(cp != CP_SYMBOL) {
        lenW = MultiByteToWideChar(cp, 0, str, count, NULL, 0);
	strW = HeapAlloc(GetProcessHeap(), 0, (lenW + 1) * sizeof(WCHAR));
	MultiByteToWideChar(cp, 0, str, count, strW, lenW);
    } else {
        lenW = count;
	strW = HeapAlloc(GetProcessHeap(), 0, (lenW + 1) * sizeof(WCHAR));
	for(i = 0; i < count; i++) strW[i] = (BYTE)str[i];
    }
    strW[lenW] = '\0';
    TRACE("mapped %s -> %s\n", debugstr_an(str, count), debugstr_wn(strW, lenW));
    if(plenW) *plenW = lenW;
    if(pCP) *pCP = cp;
    return strW;
}


/***********************************************************************
 *           ExtTextOutA    (GDI32.@)
 */
BOOL WINAPI ExtTextOutA( HDC hdc, INT x, INT y, UINT flags,
                         const RECT *lprect, LPCSTR str, UINT count, const INT *lpDx )
{
    INT wlen;
    UINT codepage;
    LPWSTR p = FONT_mbtowc(hdc, str, count, &wlen, &codepage);
    BOOL ret;
    LPINT lpDxW = NULL;

    if (lpDx) {
        unsigned int i = 0, j = 0;

	lpDxW = (LPINT)HeapAlloc( GetProcessHeap(), 0, wlen*sizeof(INT));
	while(i < count) {
	    if(IsDBCSLeadByteEx(codepage, str[i])) {
	        lpDxW[j++] = lpDx[i] + lpDx[i+1];
		i = i + 2;
	    } else {
	        lpDxW[j++] = lpDx[i];
		i = i + 1;
	    }
	}
    }

    ret = ExtTextOutW( hdc, x, y, flags, lprect, p, wlen, lpDxW );

    HeapFree( GetProcessHeap(), 0, p );
    if (lpDxW) HeapFree( GetProcessHeap(), 0, lpDxW );
    return ret;
}


/***********************************************************************
 *           ExtTextOutW    (GDI32.@)
 */
BOOL WINAPI ExtTextOutW( HDC hdc, INT x, INT y, UINT flags,
                         const RECT *lprect, LPCWSTR str, UINT count, const INT *lpDx )
{
    BOOL ret = FALSE;
    DC * dc = DC_GetDCUpdate( hdc );
    if (dc)
    {
        if(PATH_IsPathOpen(dc->path))
            FIXME("called on an open path\n");
        else if(dc->funcs->pExtTextOut)
        {
            if( !(flags&(ETO_GLYPH_INDEX|ETO_IGNORELANGUAGE)) && BidiAvail && count>0 )
            {
                /* The caller did not specify that language processing was already done.
                 */
                LPWSTR lpReorderedString=HeapAlloc(GetProcessHeap(), 0, count*sizeof(WCHAR));

                BIDI_Reorder( str, count, GCP_REORDER,
                              ((flags&ETO_RTLREADING)!=0 || (GetTextAlign(hdc)&TA_RTLREADING)!=0)?
                              WINE_GCPW_FORCE_RTL:WINE_GCPW_FORCE_LTR,
                              lpReorderedString, count, NULL );

                ret = dc->funcs->pExtTextOut(dc->physDev,x,y,flags|ETO_IGNORELANGUAGE,
                                             lprect,lpReorderedString,count,lpDx);
                HeapFree(GetProcessHeap(), 0, lpReorderedString);
            } else
                ret = dc->funcs->pExtTextOut(dc->physDev,x,y,flags,lprect,str,count,lpDx);
        }
        GDI_ReleaseObj( hdc );
    }
    return ret;
}


/***********************************************************************
 *           TextOutA    (GDI32.@)
 */
BOOL WINAPI TextOutA( HDC hdc, INT x, INT y, LPCSTR str, INT count )
{
    return ExtTextOutA( hdc, x, y, 0, NULL, str, count, NULL );
}


/***********************************************************************
 *           TextOutW    (GDI32.@)
 */
BOOL WINAPI TextOutW(HDC hdc, INT x, INT y, LPCWSTR str, INT count)
{
    return ExtTextOutW( hdc, x, y, 0, NULL, str, count, NULL );
}


/***********************************************************************
 *		PolyTextOutA (GDI32.@)
 *
 * Draw several Strings
 */
BOOL WINAPI PolyTextOutA (
			  HDC hdc,               /* [in] Handle to device context */
			  PPOLYTEXTA pptxt,      /* [in] Array of strings */
			  INT cStrings           /* [in] Number of strings in array */
			  )
{
    for (; cStrings>0; cStrings--, pptxt++)
        if (!ExtTextOutA( hdc, pptxt->x, pptxt->y, pptxt->uiFlags, &pptxt->rcl, pptxt->lpstr, pptxt->n, pptxt->pdx ))
            return FALSE;
    return TRUE;
}



/***********************************************************************
 *		PolyTextOutW (GDI32.@)
 *
 * Draw several Strings
 */
BOOL WINAPI PolyTextOutW (
			  HDC hdc,               /* [in] Handle to device context */
			  PPOLYTEXTW pptxt,      /* [in] Array of strings */
			  INT cStrings           /* [in] Number of strings in array */
			  )
{
    for (; cStrings>0; cStrings--, pptxt++)
        if (!ExtTextOutW( hdc, pptxt->x, pptxt->y, pptxt->uiFlags, &pptxt->rcl, pptxt->lpstr, pptxt->n, pptxt->pdx ))
            return FALSE;
    return TRUE;
}
