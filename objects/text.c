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
#include "winerror.h"
#include "gdi.h"
#include "heap.h"
#include "debugtools.h"
#include "winnls.h"

DEFAULT_DEBUG_CHANNEL(text);


/***********************************************************************
 *           ExtTextOut    (DISPLAY.351)
 *           ExtTextOut    (GDI.351)
 */
BOOL16 WINAPI ExtTextOut16( HDC16 hdc, INT16 x, INT16 y, UINT16 flags,
                            const RECT16 *lprect, LPCSTR str, UINT16 count,
                            const INT16 *lpDx )
{
    BOOL	ret;
    int		i;
    RECT	rect32;
    LPINT	lpdx32 = NULL;

    if (lpDx) {
	lpdx32 = (LPINT)HeapAlloc( GetProcessHeap(),0, sizeof(INT)*count );
	if(lpdx32 == NULL) return FALSE;
	for (i=count;i--;) lpdx32[i]=lpDx[i];
    }    
    if (lprect)	CONV_RECT16TO32(lprect,&rect32);
    ret = ExtTextOutA(hdc,x,y,flags,lprect?&rect32:NULL,str,count,lpdx32);
    if (lpdx32) HeapFree( GetProcessHeap(), 0, lpdx32 );
    return ret;
}


/***********************************************************************
 *           ExtTextOutA    (GDI32.@)
 */
BOOL WINAPI ExtTextOutA( HDC hdc, INT x, INT y, UINT flags,
                             const RECT *lprect, LPCSTR str, UINT count,
                             const INT *lpDx )
{
    DC * dc = DC_GetDCUpdate( hdc );
    LPWSTR p;
    UINT codepage = CP_ACP; /* FIXME: get codepage of font charset */
    BOOL ret = FALSE;
    LPINT lpDxW = NULL;

    if (!dc) return FALSE;

    if (dc->funcs->pExtTextOut)
    {
        UINT wlen = MultiByteToWideChar(codepage,0,str,count,NULL,0);
        if (lpDx)
        {
            unsigned int i = 0, j = 0;

            lpDxW = (LPINT)HeapAlloc( GetProcessHeap(), 0, wlen*sizeof(INT));
            while(i < count)
            {
                if(IsDBCSLeadByteEx(codepage, str[i]))
                {
                    lpDxW[j++] = lpDx[i] + lpDx[i+1];
                    i = i + 2;
                }
                else
                {
                    lpDxW[j++] = lpDx[i];
                    i = i + 1;
                }
            }
        }
        if ((p = HeapAlloc( GetProcessHeap(), 0, wlen * sizeof(WCHAR) )))
        {
            wlen = MultiByteToWideChar(codepage,0,str,count,p,wlen);
            ret = dc->funcs->pExtTextOut( dc, x, y, flags, lprect, p, wlen, lpDxW );
            HeapFree( GetProcessHeap(), 0, p );
        }
        if (lpDxW) HeapFree( GetProcessHeap(), 0, lpDxW );
    }
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           ExtTextOutW    (GDI32.@)
 */
BOOL WINAPI ExtTextOutW( HDC hdc, INT x, INT y, UINT flags,
                             const RECT *lprect, LPCWSTR str, UINT count,
                             const INT *lpDx )
{
    BOOL ret = FALSE;
    DC * dc = DC_GetDCUpdate( hdc );
    if (dc)
    {
	if(dc->funcs->pExtTextOut)
	    ret = dc->funcs->pExtTextOut(dc,x,y,flags,lprect,str,count,lpDx);
	GDI_ReleaseObj( hdc );
    }
    return ret;
}


/***********************************************************************
 *           TextOut    (GDI.33)
 */
BOOL16 WINAPI TextOut16( HDC16 hdc, INT16 x, INT16 y, LPCSTR str, INT16 count )
{
    return ExtTextOut16( hdc, x, y, 0, NULL, str, count, NULL );
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
 * GetTextCharset [GDI32.@]  Gets character set for font in DC
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
 * GetTextCharset [GDI.612]
 */
UINT16 WINAPI GetTextCharset16(HDC16 hdc)
{
    return (UINT16)GetTextCharset(hdc);
}

/***********************************************************************
 * GetTextCharsetInfo [GDI32.@]  Gets character set for font
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
    HDC hdc,            /* [in]  Handle to device context */
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
 *		PolyTextOutA (GDI.402)
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
  FIXME("stub!\n");
  SetLastError ( ERROR_CALL_NOT_IMPLEMENTED );
  return 0;
}



/***********************************************************************
 *		PolyTextOutW (GDI.403)
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
  FIXME("stub!\n");
  SetLastError ( ERROR_CALL_NOT_IMPLEMENTED );
  return 0;
}
