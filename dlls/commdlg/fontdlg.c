/*
 * COMMDLG - Font Dialog
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Albrecht Kleine
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

#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "heap.h"
#include "commdlg.h"
#include "dlgs.h"
#include "wine/debug.h"
#include "cderr.h"

WINE_DEFAULT_DEBUG_CHANNEL(commdlg);

#include "cdlg.h"
#include "fontdlg.h"

static HBITMAP hBitmapTT = 0;


INT_PTR CALLBACK FormatCharDlgProcA(HWND hDlg, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam);
INT_PTR CALLBACK FormatCharDlgProcW(HWND hDlg, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam);
BOOL16 CALLBACK FormatCharDlgProc16(HWND16 hDlg, UINT16 message, WPARAM16 wParam,
                                    LPARAM lParam);

/* There is a table here of all charsets, and the sample text for each.
 * There is a second table that translates a charset into an index into
 * the first table.
 */

#define CI(cs) ((IDS_CHARSET_##cs)-IDS_CHARSET_ANSI)
#define SAMPLE_EXTLEN 10

static const WCHAR SAMPLE_LANG_TEXT[][SAMPLE_EXTLEN]={
    {'Y','y','Z','z',0}, /* Western and default */
    {0}, /* Symbol */
    {0}, /* Shift JIS */
    {0}, /* Hangul */
    {0}, /* GB2312 */
    {0}, /* BIG5 */
    {0}, /* Greek */
    {0}, /* Turkish */
    {0x05e0, 0x05e1, 0x05e9, 0x05ea, 0}, /* Hebrew */
    {0}, /* Arabic */
    {0}, /* Baltic */
    {0}, /* Vietnamese */
    {0}, /* Russian */
    {0}, /* East European */
    {0}, /* Thai */
    {0}, /* Johab */
    {0}, /* Mac */
    {0}, /* OEM */
    {0}, /* VISCII */
    {0}, /* TCVN */
    {0}, /* KOI-8 */
    {0}, /* ISO-8859-3 */
    {0}, /* ISO-8859-4 */
    {0}, /* ISO-8859-10 */
    {0} /* Celtic */
};

static const int CHARSET_ORDER[256]={
    CI(ANSI), 0, CI(SYMBOL), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, CI(MAC), 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    CI(JIS), CI(HANGUL), CI(JOHAB), 0, 0, 0, CI(GB2312), 0, CI(BIG5), 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, CI(GREEK), CI(TURKISH), CI(VIETNAMESE), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, CI(HEBREW), CI(ARABIC), 0, 0, 0, 0, 0, 0, 0, CI(BALTIC), 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, CI(RUSSIAN), 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, CI(THAI), 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, CI(EE), 0,
    CI(VISCII), CI(TCVN), CI(KOI8), CI(ISO3), CI(ISO4), CI(ISO10), CI(CELTIC), 0, 0, 0, 0, 0, 0, 0, 0, CI(OEM),
};

struct {
    int         mask;
    char        *name;
} cfflags[] = {
#define XX(x) { x, #x },
    XX(CF_SCREENFONTS)
    XX(CF_PRINTERFONTS)
    XX(CF_SHOWHELP)
    XX(CF_ENABLEHOOK)
    XX(CF_ENABLETEMPLATE)
    XX(CF_ENABLETEMPLATEHANDLE)
    XX(CF_INITTOLOGFONTSTRUCT)
    XX(CF_USESTYLE)
    XX(CF_EFFECTS)
    XX(CF_APPLY)
    XX(CF_ANSIONLY)
    XX(CF_NOVECTORFONTS)
    XX(CF_NOSIMULATIONS)
    XX(CF_LIMITSIZE)
    XX(CF_FIXEDPITCHONLY)
    XX(CF_WYSIWYG)
    XX(CF_FORCEFONTEXIST)
    XX(CF_SCALABLEONLY)
    XX(CF_TTONLY)
    XX(CF_NOFACESEL)
    XX(CF_NOSTYLESEL)
    XX(CF_NOSIZESEL)
    XX(CF_SELECTSCRIPT)
    XX(CF_NOSCRIPTSEL)
    XX(CF_NOVERTFONTS)
#undef XX
    {0,NULL},
};

void _dump_cf_flags(DWORD cflags)
{
    int i;

    for (i=0;cfflags[i].name;i++)
        if (cfflags[i].mask & cflags)
            MESSAGE("%s|",cfflags[i].name);
    MESSAGE("\n");
}

/***********************************************************************
 *           ChooseFontA   (COMDLG32.@)
 */
BOOL WINAPI ChooseFontA(LPCHOOSEFONTA lpChFont)
{
    LPCVOID template;
    HRSRC hResInfo;
    HINSTANCE hDlginst;
    HGLOBAL hDlgTmpl;

    if ( (lpChFont->Flags&CF_ENABLETEMPLATEHANDLE)!=0 )
    {
        template=(LPCVOID)lpChFont->hInstance;
    } else
    {
        if ( (lpChFont->Flags&CF_ENABLETEMPLATE)!=0 )
        {
            hDlginst=lpChFont->hInstance;
            if( !(hResInfo = FindResourceA(hDlginst, lpChFont->lpTemplateName,
                            (LPSTR)RT_DIALOG)))
            {
                COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
                return FALSE;
            }
        } else
        {
            hDlginst=COMDLG32_hInstance;
            if (!(hResInfo = FindResourceA(hDlginst, "CHOOSE_FONT", (LPSTR)RT_DIALOG)))
            {
                COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
                return FALSE;
            }
        }
        if (!(hDlgTmpl = LoadResource(hDlginst, hResInfo )) ||
                !(template = LockResource( hDlgTmpl )))
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
            return FALSE;
        }
    }
    if (TRACE_ON(commdlg))
        _dump_cf_flags(lpChFont->Flags);

    if (lpChFont->Flags & (CF_SELECTSCRIPT | CF_NOVERTFONTS ))
        FIXME(": unimplemented flag (ignored)\n");

    return DialogBoxIndirectParamA(COMDLG32_hInstance, template,
            lpChFont->hwndOwner, FormatCharDlgProcA, (LPARAM)lpChFont );
}

/***********************************************************************
 *           ChooseFontW   (COMDLG32.@)
 *
 *  NOTES:
 *
 *      The LOGFONT conversion functions will break if the structure ever
 *      grows beyond the lfFaceName element.
 *
 *      The CHOOSEFONT conversion functions assume that both versions of
 *      lpLogFont and lpszStyle (if used) point to pre-allocated objects.
 *
 *      The ASCII version of lpTemplateName is created by ChooseFontAtoW
 *      and freed by ChooseFontWtoA.
 */
inline static VOID LogFontWtoA(const LOGFONTW *lfw, LOGFONTA *lfa)
{
    memcpy(lfa, lfw, sizeof(LOGFONTA));
    WideCharToMultiByte(CP_ACP, 0, lfw->lfFaceName, -1, lfa->lfFaceName,
                        LF_FACESIZE, NULL, NULL);
    lfa->lfFaceName[LF_FACESIZE - 1] = '\0';
}

inline static VOID LogFontAtoW(const LOGFONTA *lfa, LOGFONTW *lfw)
{
    memcpy(lfw, lfa, sizeof(LOGFONTA));
    MultiByteToWideChar(CP_ACP, 0, lfa->lfFaceName, -1, lfw->lfFaceName,
                        LF_FACESIZE);
    lfw->lfFaceName[LF_FACESIZE - 1] = 0;
}

static BOOL ChooseFontWtoA(const CHOOSEFONTW *cfw, CHOOSEFONTA *cfa)
{
    LOGFONTA    *lpLogFont = cfa->lpLogFont;
    LPSTR       lpszStyle = cfa->lpszStyle;

    memcpy(cfa, cfw, sizeof(CHOOSEFONTA));
    cfa->lpLogFont = lpLogFont;
    cfa->lpszStyle = lpszStyle;

    LogFontWtoA(cfw->lpLogFont, lpLogFont);

    if ((cfw->Flags&CF_ENABLETEMPLATE)!=0 && HIWORD(cfw->lpTemplateName)!=0)
    {
        cfa->lpTemplateName = HEAP_strdupWtoA(GetProcessHeap(), 0,
                cfw->lpTemplateName);
        if (cfa->lpTemplateName == NULL)
            return FALSE;
    }

    if ((cfw->Flags & CF_USESTYLE) != 0 && cfw->lpszStyle != NULL)
    {
        WideCharToMultiByte(CP_ACP, 0, cfw->lpszStyle, -1, cfa->lpszStyle,
                LF_FACESIZE, NULL, NULL);
        cfa->lpszStyle[LF_FACESIZE - 1] = '\0';
    }

    return TRUE;
}

static VOID ChooseFontAtoW(const CHOOSEFONTA *cfa, CHOOSEFONTW *cfw)
{
    LOGFONTW    *lpLogFont = cfw->lpLogFont;
    LPWSTR      lpszStyle = cfw->lpszStyle;
    LPCWSTR     lpTemplateName = cfw->lpTemplateName;

    memcpy(cfw, cfa, sizeof(CHOOSEFONTA));
    cfw->lpLogFont = lpLogFont;
    cfw->lpszStyle = lpszStyle;
    cfw->lpTemplateName = lpTemplateName;

    LogFontAtoW(cfa->lpLogFont, lpLogFont);

    if ((cfa->Flags&CF_ENABLETEMPLATE)!=0 && HIWORD(cfa->lpTemplateName) != 0)
        HeapFree(GetProcessHeap(), 0, (LPSTR)(cfa->lpTemplateName));

    if ((cfa->Flags & CF_USESTYLE) != 0 && cfa->lpszStyle != NULL)
    {
        MultiByteToWideChar(CP_ACP, 0, cfa->lpszStyle, -1, cfw->lpszStyle,
                LF_FACESIZE);
        cfw->lpszStyle[LF_FACESIZE - 1] = 0;
    }
}

BOOL WINAPI ChooseFontW(LPCHOOSEFONTW lpChFont)
{
    CHOOSEFONTA     cf_a;
    LOGFONTA        lf_a;
    CHAR            style_a[LF_FACESIZE];

    cf_a.lpLogFont = &lf_a;
    cf_a.lpszStyle = style_a;

    if (ChooseFontWtoA(lpChFont, &cf_a) == FALSE)
    {
        COMDLG32_SetCommDlgExtendedError(CDERR_MEMALLOCFAILURE);
        return FALSE;
    }

    if (ChooseFontA(&cf_a) == FALSE)
    {
        if (cf_a.lpTemplateName != NULL)
            HeapFree(GetProcessHeap(), 0, (LPSTR)(cf_a.lpTemplateName));
        return FALSE;
    }

    ChooseFontAtoW(&cf_a, lpChFont);

    return TRUE;
}

#if 0
/***********************************************************************
 *           ChooseFontW   (COMDLG32.@)
 */
BOOL WINAPI ChooseFontW(LPCHOOSEFONTW lpChFont)
{
    BOOL bRet=FALSE;
    CHOOSEFONTA cf32a;
    LOGFONTA lf32a;
    LPCVOID template;
    HANDLE hResInfo, hDlgTmpl;

    if (TRACE_ON(commdlg))
        _dump_cf_flags(lpChFont->Flags);

    if (!(hResInfo = FindResourceA(COMDLG32_hInstance, "CHOOSE_FONT", (LPSTR)RT_DIALOG)))
    {
        COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
        return FALSE;
    }
    if (!(hDlgTmpl = LoadResource(COMDLG32_hInstance, hResInfo )) ||
            !(template = LockResource( hDlgTmpl )))
    {
        COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
        return FALSE;
    }

    if (lpChFont->Flags & (CF_SELECTSCRIPT | CF_NOVERTFONTS | CF_ENABLETEMPLATE |
                CF_ENABLETEMPLATEHANDLE)) FIXME(": unimplemented flag (ignored)\n");
    memcpy(&cf32a, lpChFont, sizeof(cf32a));
    memcpy(&lf32a, lpChFont->lpLogFont, sizeof(LOGFONTA));

    WideCharToMultiByte( CP_ACP, 0, lpChFont->lpLogFont->lfFaceName, -1,
            lf32a.lfFaceName, LF_FACESIZE, NULL, NULL );
    lf32a.lfFaceName[LF_FACESIZE-1] = 0;
    cf32a.lpLogFont=&lf32a;
    cf32a.lpszStyle=HEAP_strdupWtoA(GetProcessHeap(), 0, lpChFont->lpszStyle);
    lpChFont->lpTemplateName=(LPWSTR)&cf32a;
    bRet = DialogBoxIndirectParamW(COMDLG32_hInstance, template,
            lpChFont->hwndOwner, FormatCharDlgProcW, (LPARAM)lpChFont );
    HeapFree(GetProcessHeap(), 0, cf32a.lpszStyle);
    lpChFont->lpTemplateName=(LPWSTR)cf32a.lpTemplateName;
    memcpy(lpChFont->lpLogFont, &lf32a, sizeof(CHOOSEFONTA));
    MultiByteToWideChar( CP_ACP, 0, lf32a.lfFaceName, -1,
            lpChFont->lpLogFont->lfFaceName, LF_FACESIZE );
    lpChFont->lpLogFont->lfFaceName[LF_FACESIZE-1] = 0;
    return bRet;
}
#endif

#define TEXT_EXTRAS 4
#define TEXT_COLORS 16

static const COLORREF textcolors[TEXT_COLORS]=
{
    0x00000000L,0x00000080L,0x00008000L,0x00008080L,
    0x00800000L,0x00800080L,0x00808000L,0x00808080L,
    0x00c0c0c0L,0x000000ffL,0x0000ff00L,0x0000ffffL,
    0x00ff0000L,0x00ff00ffL,0x00ffff00L,0x00FFFFFFL
};

/***********************************************************************
 *                          CFn_HookCallChk32                 [internal]
 */
static BOOL CFn_HookCallChk32(LPCHOOSEFONTA lpcf)
{
    if (lpcf)
        if(lpcf->Flags & CF_ENABLEHOOK)
            if (lpcf->lpfnHook)
                return TRUE;
    return FALSE;
}

/*************************************************************************
 *              AddFontFamily                               [internal]
 */
INT AddFontFamily(const LOGFONTA *lplf, UINT nFontType,
        LPCHOOSEFONTA lpcf, HWND hwnd, LPCFn_ENUMSTRUCT e)
{
    int i;
    WORD w;

    TRACE("font=%s (nFontType=%d)\n", lplf->lfFaceName,nFontType);

    if (lpcf->Flags & CF_FIXEDPITCHONLY)
        if (!(lplf->lfPitchAndFamily & FIXED_PITCH))
            return 1;
    if (lpcf->Flags & CF_ANSIONLY)
        if (lplf->lfCharSet != ANSI_CHARSET)
            return 1;
    if (lpcf->Flags & CF_TTONLY)
        if (!(nFontType & TRUETYPE_FONTTYPE))
            return 1;

    if (e) e->added++;

    i=SendMessageA(hwnd, CB_ADDSTRING, 0, (LPARAM)lplf->lfFaceName);
    if (i!=CB_ERR)
    {
        w=(lplf->lfCharSet << 8) | lplf->lfPitchAndFamily;
        SendMessageA(hwnd, CB_SETITEMDATA, i, MAKELONG(nFontType,w));
        return 1 ;        /* store some important font information */
    }
    else
        return 0;
}

/*************************************************************************
 *              FontFamilyEnumProc32                           [internal]
 */
static INT WINAPI FontFamilyEnumProc(const LOGFONTA *lpLogFont,
        const TEXTMETRICA *metrics, DWORD dwFontType, LPARAM lParam)
{
    LPCFn_ENUMSTRUCT e;
    e=(LPCFn_ENUMSTRUCT)lParam;
    return AddFontFamily(lpLogFont, dwFontType, e->lpcf32a, e->hWnd1, e);
}

/*************************************************************************
 *              SetFontStylesToCombo2                           [internal]
 *
 * Fill font style information into combobox  (without using font.c directly)
 */
static int SetFontStylesToCombo2(HWND hwnd, HDC hdc, const LOGFONTA *lplf)
{
#define FSTYLES 4
    struct FONTSTYLE
    {
        int italic;
        int weight;
        char stname[20];
    };
    static struct FONTSTYLE fontstyles[FSTYLES]={
        { 0,FW_NORMAL,"Regular"}, { 1,FW_NORMAL,"Italic"},
        { 0,FW_BOLD,"Bold"}, { 1,FW_BOLD,"Bold Italic"}
    };
    HFONT hf;
    TEXTMETRICA tm;
    int i,j;
    LOGFONTA lf;

    memcpy(&lf, lplf, sizeof(LOGFONTA));

    for (i=0;i<FSTYLES;i++)
    {
        lf.lfItalic=fontstyles[i].italic;
        lf.lfWeight=fontstyles[i].weight;
        hf=CreateFontIndirectA(&lf);
        hf=SelectObject(hdc,hf);
        GetTextMetricsA(hdc,&tm);
        hf=SelectObject(hdc,hf);
        DeleteObject(hf);
                /* font successful created ? */
        if (tm.tmWeight==fontstyles[i].weight &&
            ((tm.tmItalic != 0)==fontstyles[i].italic))
        {
            j=SendMessageA(hwnd,CB_ADDSTRING,0,(LPARAM)fontstyles[i].stname );
            if (j==CB_ERR) return 1;
            j=SendMessageA(hwnd, CB_SETITEMDATA, j,
                           MAKELONG(fontstyles[i].weight,fontstyles[i].italic));
            if (j==CB_ERR) return 1;
        }
    }
    return 0;
}

/*************************************************************************
 *              AddFontSizeToCombo3                           [internal]
 */
static int AddFontSizeToCombo3(HWND hwnd, UINT h, LPCHOOSEFONTA lpcf)
{
    int j;
    char buffer[20];

    if (  (!(lpcf->Flags & CF_LIMITSIZE))  ||
            ((lpcf->Flags & CF_LIMITSIZE) && (h >= lpcf->nSizeMin) && (h <= lpcf->nSizeMax)))
    {
        sprintf(buffer, "%2d", h);
        j=SendMessageA(hwnd, CB_FINDSTRINGEXACT, -1, (LPARAM)buffer);
        if (j==CB_ERR)
        {
            j=SendMessageA(hwnd, CB_ADDSTRING, 0, (LPARAM)buffer);
            if (j!=CB_ERR) j = SendMessageA(hwnd, CB_SETITEMDATA, j, h);
            if (j==CB_ERR) return 1;
        }
    }
    return 0;
}

/*************************************************************************
 *              SetFontSizesToCombo3                           [internal]
 */
static int SetFontSizesToCombo3(HWND hwnd, LPCHOOSEFONTA lpcf)
{
    static const int sizes[]={8,9,10,11,12,14,16,18,20,22,24,26,28,36,48,72,0};
    int i;

    for (i=0; sizes[i]; i++)
        if (AddFontSizeToCombo3(hwnd, sizes[i], lpcf)) return 1;
    return 0;
}

/***********************************************************************
 *                 AddFontStyle                          [internal]
 */
INT AddFontStyle(const LOGFONTA *lplf, UINT nFontType,
                 LPCHOOSEFONTA lpcf, HWND hcmb2, HWND hcmb3, HWND hDlg)
{
    int i;

    TRACE("(nFontType=%d)\n",nFontType);
    TRACE("  %s h=%ld w=%ld e=%ld o=%ld wg=%ld i=%d u=%d s=%d"
            " ch=%d op=%d cp=%d q=%d pf=%xh\n",
            lplf->lfFaceName,lplf->lfHeight,lplf->lfWidth,
            lplf->lfEscapement,lplf->lfOrientation,
            lplf->lfWeight,lplf->lfItalic,lplf->lfUnderline,
            lplf->lfStrikeOut,lplf->lfCharSet, lplf->lfOutPrecision,
            lplf->lfClipPrecision,lplf->lfQuality, lplf->lfPitchAndFamily);
    if (nFontType & RASTER_FONTTYPE)
    {
        if (AddFontSizeToCombo3(hcmb3, lplf->lfHeight, lpcf)) return 0;
    } else if (SetFontSizesToCombo3(hcmb3, lpcf)) return 0;

    if (!SendMessageA(hcmb2, CB_GETCOUNT, 0, 0))
    {
        HDC hdc= ((lpcf->Flags & CF_PRINTERFONTS) && lpcf->hDC) ? lpcf->hDC : GetDC(hDlg);
        i=SetFontStylesToCombo2(hcmb2,hdc,lplf);
        if (!((lpcf->Flags & CF_PRINTERFONTS) && lpcf->hDC))
            ReleaseDC(hDlg,hdc);
        if (i)
            return 0;
    }
    return 1 ;

}

/***********************************************************************
 *                 FontStyleEnumProc32                     [internal]
 */
static INT WINAPI FontStyleEnumProc( const LOGFONTA *lpFont,
        const TEXTMETRICA *metrics, DWORD dwFontType, LPARAM lParam )
{
    LPCFn_ENUMSTRUCT s=(LPCFn_ENUMSTRUCT)lParam;
    HWND hcmb2=s->hWnd1;
    HWND hcmb3=s->hWnd2;
    HWND hDlg=GetParent(hcmb3);
    return AddFontStyle(lpFont, dwFontType, s->lpcf32a, hcmb2,
            hcmb3, hDlg);
}

/***********************************************************************
 *           CFn_WMInitDialog                            [internal]
 */
LRESULT CFn_WMInitDialog(HWND hDlg, WPARAM wParam, LPARAM lParam,
                         LPCHOOSEFONTA lpcf)
{
    HDC hdc;
    int i,j,init=0;
    long l;
    LPLOGFONTA lpxx;
    HCURSOR hcursor=SetCursor(LoadCursorA(0,(LPSTR)IDC_WAIT));

    SetWindowLongA(hDlg, DWL_USER, lParam);
    lpxx=lpcf->lpLogFont;
    TRACE("WM_INITDIALOG lParam=%08lX\n", lParam);

    if (lpcf->lStructSize != sizeof(CHOOSEFONTA))
    {
        ERR("structure size failure !!!\n");
        EndDialog (hDlg, 0);
        return FALSE;
    }
    if (!hBitmapTT)
        hBitmapTT = LoadBitmapA(0, MAKEINTRESOURCEA(OBM_TRTYPE));

    if (!(lpcf->Flags & CF_SHOWHELP) || !IsWindow(lpcf->hwndOwner))
        ShowWindow(GetDlgItem(hDlg,pshHelp),SW_HIDE);
    if (!(lpcf->Flags & CF_APPLY))
        ShowWindow(GetDlgItem(hDlg,psh3),SW_HIDE);
    if (lpcf->Flags & CF_EFFECTS)
    {
        for (i=0;i<TEXT_COLORS;i++)
        {
            char name[30];

            if( LoadStringA(COMDLG32_hInstance, IDS_COLOR_BLACK+i, name,
                        sizeof(name)/sizeof(*name) )==0 )
            {
                strcpy( name, "[color name]" );
            }
            j=SendDlgItemMessageA(hDlg, cmb4, CB_ADDSTRING, 0, (LPARAM)name);
            SendDlgItemMessageA(hDlg, cmb4, CB_SETITEMDATA16, j, textcolors[j]);
            /* look for a fitting value in color combobox */
            if (textcolors[j]==lpcf->rgbColors)
                SendDlgItemMessageA(hDlg,cmb4, CB_SETCURSEL,j,0);
        }
    }
    else
    {
        ShowWindow(GetDlgItem(hDlg,cmb4),SW_HIDE);
        ShowWindow(GetDlgItem(hDlg,chx1),SW_HIDE);
        ShowWindow(GetDlgItem(hDlg,chx2),SW_HIDE);
        ShowWindow(GetDlgItem(hDlg,grp1),SW_HIDE);
        ShowWindow(GetDlgItem(hDlg,stc4),SW_HIDE);
    }
    hdc= ((lpcf->Flags & CF_PRINTERFONTS) && lpcf->hDC) ? lpcf->hDC : GetDC(hDlg);
    if (hdc)
    {
        CFn_ENUMSTRUCT s;
        s.hWnd1=GetDlgItem(hDlg,cmb1);
        s.lpcf32a=lpcf;
        do {
            s.added = 0;
            if (!EnumFontFamiliesA(hdc, NULL, FontFamilyEnumProc, (LPARAM)&s)) {
                TRACE("EnumFontFamilies returns 0\n");
                break;
            }
            if (s.added) break;
            if (lpcf->Flags & CF_FIXEDPITCHONLY) {
                FIXME("No font found with fixed pitch only, dropping flag.\n");
                lpcf->Flags &= ~CF_FIXEDPITCHONLY;
                continue;
            }
            if (lpcf->Flags & CF_TTONLY) {
                FIXME("No font found with truetype only, dropping flag.\n");
                lpcf->Flags &= ~CF_TTONLY;
                continue;
            }
            break;
        } while (1);


        if (lpcf->Flags & CF_INITTOLOGFONTSTRUCT)
        {
            /* look for fitting font name in combobox1 */
            j=SendDlgItemMessageA(hDlg,cmb1,CB_FINDSTRING,-1,(LONG)lpxx->lfFaceName);
            if (j!=CB_ERR)
            {
                SendDlgItemMessageA(hDlg, cmb1, CB_SETCURSEL, j, 0);
                SendMessageA(hDlg, WM_COMMAND, MAKEWPARAM(cmb1, CBN_SELCHANGE),
                        (LPARAM)GetDlgItem(hDlg,cmb1));
                init=1;
                /* look for fitting font style in combobox2 */
                l=MAKELONG(lpxx->lfWeight > FW_MEDIUM ? FW_BOLD:FW_NORMAL,lpxx->lfItalic !=0);
                for (i=0;i<TEXT_EXTRAS;i++)
                {
                    if (l==SendDlgItemMessageA(hDlg, cmb2, CB_GETITEMDATA, i, 0))
                        SendDlgItemMessageA(hDlg, cmb2, CB_SETCURSEL, i, 0);
                }

                /* look for fitting font size in combobox3 */
                j=SendDlgItemMessageA(hDlg, cmb3, CB_GETCOUNT, 0, 0);
                for (i=0;i<j;i++)
                {
                    if (lpxx->lfHeight==(int)SendDlgItemMessageA(hDlg,cmb3, CB_GETITEMDATA,i,0))
                        SendDlgItemMessageA(hDlg,cmb3,CB_SETCURSEL,i,0);
                }
            }
        }
        if (!init)
        {
            SendDlgItemMessageA(hDlg,cmb1,CB_SETCURSEL,0,0);
            SendMessageA(hDlg, WM_COMMAND, MAKEWPARAM(cmb1, CBN_SELCHANGE),
                    (LPARAM)GetDlgItem(hDlg,cmb1));
        }
        if (lpcf->Flags & CF_USESTYLE && lpcf->lpszStyle)
        {
            j=SendDlgItemMessageA(hDlg,cmb2,CB_FINDSTRING,-1,(LONG)lpcf->lpszStyle);
            if (j!=CB_ERR)
            {
                j=SendDlgItemMessageA(hDlg,cmb2,CB_SETCURSEL,j,0);
                SendMessageA(hDlg,WM_COMMAND,cmb2,
                        MAKELONG(HWND_16(GetDlgItem(hDlg,cmb2)),CBN_SELCHANGE));
            }
        }
    }
    else
    {
        WARN("HDC failure !!!\n");
        EndDialog (hDlg, 0);
        return FALSE;
    }

    if (!((lpcf->Flags & CF_PRINTERFONTS) && lpcf->hDC))
        ReleaseDC(hDlg,hdc);
    SetCursor(hcursor);
    return TRUE;
}


/***********************************************************************
 *           CFn_WMMeasureItem                           [internal]
 */
LRESULT CFn_WMMeasureItem(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    BITMAP bm;
    LPMEASUREITEMSTRUCT lpmi=(LPMEASUREITEMSTRUCT)lParam;
    if (!hBitmapTT)
        hBitmapTT = LoadBitmapA(0, MAKEINTRESOURCEA(OBM_TRTYPE));
    GetObjectA( hBitmapTT, sizeof(bm), &bm );
    lpmi->itemHeight=bm.bmHeight;
    /* FIXME: use MAX of bm.bmHeight and tm.tmHeight .*/
    return 0;
}


/***********************************************************************
 *           CFn_WMDrawItem                              [internal]
 */
LRESULT CFn_WMDrawItem(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    HBRUSH hBrush;
    char buffer[40];
    BITMAP bm;
    COLORREF cr, oldText=0, oldBk=0;
    RECT rect;
#if 0
    HDC hMemDC;
    int nFontType;
    HBITMAP hBitmap; /* for later TT usage */
#endif
    LPDRAWITEMSTRUCT lpdi = (LPDRAWITEMSTRUCT)lParam;

    if (lpdi->itemID == (UINT)-1)  /* got no items */
        DrawFocusRect(lpdi->hDC, &lpdi->rcItem);
    else
    {
        if (lpdi->CtlType == ODT_COMBOBOX)
        {
            if (lpdi->itemState ==ODS_SELECTED)
            {
                hBrush=GetSysColorBrush(COLOR_HIGHLIGHT);
                oldText=SetTextColor(lpdi->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
                oldBk=SetBkColor(lpdi->hDC, GetSysColor(COLOR_HIGHLIGHT));
            }  else
            {
                hBrush = SelectObject(lpdi->hDC, GetStockObject(LTGRAY_BRUSH));
                SelectObject(lpdi->hDC, hBrush);
            }
            FillRect(lpdi->hDC, &lpdi->rcItem, hBrush);
        }
        else
            return TRUE;        /* this should never happen */

        rect=lpdi->rcItem;
        switch (lpdi->CtlID)
        {
        case cmb1:
            /* TRACE(commdlg,"WM_Drawitem cmb1\n"); */
            SendMessageA(lpdi->hwndItem, CB_GETLBTEXT, lpdi->itemID,
                         (LPARAM)buffer);
            GetObjectA( hBitmapTT, sizeof(bm), &bm );
            TextOutA(lpdi->hDC, lpdi->rcItem.left + bm.bmWidth + 10,
                     lpdi->rcItem.top, buffer, strlen(buffer));
#if 0
            nFontType = SendMessageA(lpdi->hwndItem, CB_GETITEMDATA, lpdi->itemID,0L);
            /* FIXME: draw bitmap if truetype usage */
            if (nFontType&TRUETYPE_FONTTYPE)
            {
                hMemDC = CreateCompatibleDC(lpdi->hDC);
                hBitmap = SelectObject(hMemDC, hBitmapTT);
                BitBlt(lpdi->hDC, lpdi->rcItem.left, lpdi->rcItem.top,
                       bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
                SelectObject(hMemDC, hBitmap);
                DeleteDC(hMemDC);
            }
#endif
            break;
        case cmb2:
        case cmb3:
            /* TRACE(commdlg,"WM_DRAWITEN cmb2,cmb3\n"); */
            SendMessageA(lpdi->hwndItem, CB_GETLBTEXT, lpdi->itemID,
                         (LPARAM)buffer);
            TextOutA(lpdi->hDC, lpdi->rcItem.left,
                     lpdi->rcItem.top, buffer, strlen(buffer));
            break;

        case cmb4:
            /* TRACE(commdlg,"WM_DRAWITEM cmb4 (=COLOR)\n"); */
            SendMessageA(lpdi->hwndItem, CB_GETLBTEXT, lpdi->itemID,
                         (LPARAM)buffer);
            TextOutA(lpdi->hDC, lpdi->rcItem.left +  25+5,
                     lpdi->rcItem.top, buffer, strlen(buffer));
            cr = SendMessageA(lpdi->hwndItem, CB_GETITEMDATA, lpdi->itemID,0L);
            hBrush = CreateSolidBrush(cr);
            if (hBrush)
            {
                hBrush = SelectObject (lpdi->hDC, hBrush) ;
                rect.right=rect.left+25;
                rect.top++;
                rect.left+=5;
                rect.bottom--;
                Rectangle( lpdi->hDC, rect.left, rect.top,
                           rect.right, rect.bottom );
                DeleteObject( SelectObject (lpdi->hDC, hBrush)) ;
            }
            rect=lpdi->rcItem;
            rect.left+=25+5;
            break;

        default:
            return TRUE;  /* this should never happen */
        }
        if (lpdi->itemState == ODS_SELECTED)
        {
            SetTextColor(lpdi->hDC, oldText);
            SetBkColor(lpdi->hDC, oldBk);
        }
    }
    return TRUE;
}

/***********************************************************************
 *           CFn_WMCommand                               [internal]
 */
LRESULT CFn_WMCommand(HWND hDlg, WPARAM wParam, LPARAM lParam,
        LPCHOOSEFONTA lpcf)
{
    int i,j;
    long l;
    HDC hdc;
    LPLOGFONTA lpxx=lpcf->lpLogFont;

    TRACE("WM_COMMAND wParam=%08lX lParam=%08lX\n", (LONG)wParam, lParam);
    switch (LOWORD(wParam))
    {
    case cmb1:
        if (HIWORD(wParam)==CBN_SELCHANGE)
        {
            hdc=((lpcf->Flags & CF_PRINTERFONTS) && lpcf->hDC) ? lpcf->hDC : GetDC(hDlg);
            if (hdc)
            {
                SendDlgItemMessageA(hDlg, cmb2, CB_RESETCONTENT16, 0, 0);
                SendDlgItemMessageA(hDlg, cmb3, CB_RESETCONTENT16, 0, 0);
                i=SendDlgItemMessageA(hDlg, cmb1, CB_GETCURSEL16, 0, 0);
                if (i!=CB_ERR)
                {
                    HCURSOR hcursor=SetCursor(LoadCursorA(0,(LPSTR)IDC_WAIT));
                    CFn_ENUMSTRUCT s;
                    char str[256];
                    SendDlgItemMessageA(hDlg, cmb1, CB_GETLBTEXT, i,
                                        (LPARAM)str);
                    TRACE("WM_COMMAND/cmb1 =>%s\n",str);
                    s.hWnd1=GetDlgItem(hDlg, cmb2);
                    s.hWnd2=GetDlgItem(hDlg, cmb3);
                    s.lpcf32a=lpcf;
                    EnumFontFamiliesA(hdc, str, FontStyleEnumProc, (LPARAM)&s);
                    SendDlgItemMessageA(hDlg,cmb2, CB_SETCURSEL, 0, 0);
                    SendDlgItemMessageA(hDlg,cmb3, CB_SETCURSEL, 0, 0);
                    SetCursor(hcursor);
                }
                if (!((lpcf->Flags & CF_PRINTERFONTS) && lpcf->hDC))
                    ReleaseDC(hDlg,hdc);
            }
            else
            {
                WARN("HDC failure !!!\n");
                EndDialog (hDlg, 0);
                return TRUE;
            }
        }
    case chx1:
    case chx2:
    case cmb2:
    case cmb3:
        if (HIWORD(wParam)==CBN_SELCHANGE || HIWORD(wParam)== BN_CLICKED )
        {
            char str[256];
            WINDOWINFO wininfo;

            TRACE("WM_COMMAND/cmb2,3 =%08lX\n", lParam);
            i=SendDlgItemMessageA(hDlg,cmb1,CB_GETCURSEL,0,0);
            if (i==CB_ERR)
                i=GetDlgItemTextA( hDlg, cmb1, str, 256 );
            else
            {
                SendDlgItemMessageA(hDlg,cmb1,CB_GETLBTEXT,i,
                                    (LPARAM)str);
                l=SendDlgItemMessageA(hDlg,cmb1,CB_GETITEMDATA,i,0);
                j=HIWORD(l);
                lpcf->nFontType = LOWORD(l);
                /* FIXME:   lpcf->nFontType |= ....  SIMULATED_FONTTYPE and so */
                /* same value reported to the EnumFonts
                   call back with the extra FONTTYPE_...  bits added */
                lpxx->lfPitchAndFamily=j&0xff;
                lpxx->lfCharSet=j>>8;
            }
            strcpy(lpxx->lfFaceName,str);
            i=SendDlgItemMessageA(hDlg, cmb2, CB_GETCURSEL, 0, 0);
            if (i!=CB_ERR)
            {
                l=SendDlgItemMessageA(hDlg, cmb2, CB_GETITEMDATA, i, 0);
                if (0!=(lpxx->lfItalic=HIWORD(l)))
                    lpcf->nFontType |= ITALIC_FONTTYPE;
                if ((lpxx->lfWeight=LOWORD(l)) > FW_MEDIUM)
                    lpcf->nFontType |= BOLD_FONTTYPE;
            }
            i=SendDlgItemMessageA(hDlg, cmb3, CB_GETCURSEL, 0, 0);
            if (i!=CB_ERR)
                lpxx->lfHeight=-LOWORD(SendDlgItemMessageA(hDlg, cmb3, CB_GETITEMDATA, i, 0));
            else
                lpxx->lfHeight=0;
            lpxx->lfStrikeOut=IsDlgButtonChecked(hDlg,chx1);
            lpxx->lfUnderline=IsDlgButtonChecked(hDlg,chx2);
            lpxx->lfWidth=lpxx->lfOrientation=lpxx->lfEscapement=0;
            lpxx->lfOutPrecision=OUT_DEFAULT_PRECIS;
            lpxx->lfClipPrecision=CLIP_DEFAULT_PRECIS;
            lpxx->lfQuality=DEFAULT_QUALITY;
            lpcf->iPointSize= -10*lpxx->lfHeight;

            wininfo.cbSize=sizeof(wininfo);

            if( GetWindowInfo( GetDlgItem( hDlg, stc5), &wininfo ) )
            {
                MapWindowPoints( 0, hDlg, (LPPOINT) &wininfo.rcWindow, 2);
                InvalidateRect( hDlg, &wininfo.rcWindow, TRUE );
            }
        }
        break;

    case cmb4:
        i=SendDlgItemMessageA(hDlg, cmb4, CB_GETCURSEL, 0, 0);
        if (i!=CB_ERR)
        {
            WINDOWINFO wininfo;

            lpcf->rgbColors=textcolors[i];
            wininfo.cbSize=sizeof(wininfo);

            if( GetWindowInfo( GetDlgItem( hDlg, stc5), &wininfo ) )
            {
                MapWindowPoints( 0, hDlg, (LPPOINT) &wininfo.rcWindow, 2);
                InvalidateRect( hDlg, &wininfo.rcWindow, TRUE );
            }
        }
        break;

    case psh15:
        i=RegisterWindowMessageA( HELPMSGSTRINGA );
        if (lpcf->hwndOwner)
            SendMessageA(lpcf->hwndOwner, i, 0, (LPARAM)GetWindowLongA(hDlg, DWL_USER));
        /* if (CFn_HookCallChk(lpcf))
           CallWindowProc16(lpcf->lpfnHook,hDlg,WM_COMMAND,psh15,(LPARAM)lpcf);*/
        break;

    case IDOK:
        if (  (!(lpcf->Flags & CF_LIMITSIZE))  ||
              ( (lpcf->Flags & CF_LIMITSIZE) &&
                (-lpxx->lfHeight >= lpcf->nSizeMin) &&
                (-lpxx->lfHeight <= lpcf->nSizeMax)))
            EndDialog(hDlg, TRUE);
        else
        {
            char buffer[80];
            sprintf(buffer,"Select a font size between %d and %d points.",
                    lpcf->nSizeMin,lpcf->nSizeMax);
            MessageBoxA(hDlg, buffer, NULL, MB_OK);
        }
        return(TRUE);
    case IDCANCEL:
        EndDialog(hDlg, FALSE);
        return(TRUE);
    }
    return(FALSE);
}

LRESULT CFn_WMDestroy(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    return TRUE;
}

static LRESULT CFn_WMPaint(HWND hDlg, WPARAM wParam, LPARAM lParam,
        LPCHOOSEFONTA lpcf )
{
    WINDOWINFO info;

    info.cbSize=sizeof(info);

    if( GetWindowInfo( GetDlgItem( hDlg, stc5), &info ) )
    {
        PAINTSTRUCT ps;
        HDC hdc;
        HPEN hOrigPen;
        HFONT hOrigFont;
        COLORREF rgbPrev;
        WCHAR sample[SAMPLE_EXTLEN+5]={'A','a','B','b'};
        LOGFONTA lf = *(lpcf->lpLogFont);
        /* Always start with this basic sample */

        MapWindowPoints( 0, hDlg, (LPPOINT) &info.rcWindow, 2);
        hdc=BeginPaint( hDlg, &ps );

        /* Paint frame */
        MoveToEx( hdc, info.rcWindow.left, info.rcWindow.bottom, NULL );
        hOrigPen=SelectObject( hdc, CreatePen( PS_SOLID, 2,
                                               GetSysColor( COLOR_3DSHADOW ) ));
        LineTo( hdc, info.rcWindow.left, info.rcWindow.top );
        LineTo( hdc, info.rcWindow.right, info.rcWindow.top );
        DeleteObject(SelectObject( hdc, CreatePen( PS_SOLID, 2,
                                                   GetSysColor( COLOR_3DLIGHT ) )));
        LineTo( hdc, info.rcWindow.right, info.rcWindow.bottom );
        LineTo( hdc, info.rcWindow.left, info.rcWindow.bottom );
        DeleteObject(SelectObject( hdc, hOrigPen ));

        /* Draw the sample text itself */
        lstrcatW(sample, SAMPLE_LANG_TEXT[CHARSET_ORDER[lpcf->lpLogFont->lfCharSet]] );

        info.rcWindow.right--;
        info.rcWindow.bottom--;
        info.rcWindow.top++;
        info.rcWindow.left++;
        lf.lfHeight = MulDiv(lf.lfHeight, GetDeviceCaps(hdc, LOGPIXELSY), 72);
        hOrigFont = SelectObject( hdc, CreateFontIndirectA( &lf ) );
        rgbPrev=SetTextColor( hdc, lpcf->rgbColors );

        DrawTextW( hdc, sample, -1, &info.rcWindow, DT_CENTER|DT_VCENTER|DT_SINGLELINE );

        EndPaint( hDlg, &ps );
    }

    return FALSE;
}

/***********************************************************************
 *           FormatCharDlgProcA   [internal]
 */
INT_PTR CALLBACK FormatCharDlgProcA(HWND hDlg, UINT uMsg, WPARAM wParam,
        LPARAM lParam)
{
    LPCHOOSEFONTA lpcf;
    INT_PTR res = FALSE;
    if (uMsg!=WM_INITDIALOG)
    {
        lpcf=(LPCHOOSEFONTA)GetWindowLongA(hDlg, DWL_USER);
        if (!lpcf)
            return FALSE;
        if (CFn_HookCallChk32(lpcf))
            res=CallWindowProcA((WNDPROC)lpcf->lpfnHook, hDlg, uMsg, wParam, lParam);
        if (res)
            return res;
    }
    else
    {
        lpcf=(LPCHOOSEFONTA)lParam;
        if (!CFn_WMInitDialog(hDlg, wParam, lParam, lpcf))
        {
            TRACE("CFn_WMInitDialog returned FALSE\n");
            return FALSE;
        }
        if (CFn_HookCallChk32(lpcf))
            return CallWindowProcA((WNDPROC)lpcf->lpfnHook,hDlg,WM_INITDIALOG,wParam,lParam);
    }
    switch (uMsg)
    {
    case WM_MEASUREITEM:
        return CFn_WMMeasureItem(hDlg, wParam, lParam);
    case WM_DRAWITEM:
        return CFn_WMDrawItem(hDlg, wParam, lParam);
    case WM_COMMAND:
        return CFn_WMCommand(hDlg, wParam, lParam, lpcf);
    case WM_DESTROY:
        return CFn_WMDestroy(hDlg, wParam, lParam);
    case WM_CHOOSEFONT_GETLOGFONT:
        TRACE("WM_CHOOSEFONT_GETLOGFONT lParam=%08lX\n",
                lParam);
        FIXME("current logfont back to caller\n");
        break;
    case WM_PAINT:
        return CFn_WMPaint(hDlg, wParam, lParam, lpcf);
    }
    return res;
}

/***********************************************************************
 *           FormatCharDlgProcW   [internal]
 */
INT_PTR CALLBACK FormatCharDlgProcW(HWND hDlg, UINT uMsg, WPARAM wParam,
        LPARAM lParam)
{
    LPCHOOSEFONTW lpcf32w;
    LPCHOOSEFONTA lpcf32a;
    INT_PTR res = FALSE;
    if (uMsg!=WM_INITDIALOG)
    {
        lpcf32w=(LPCHOOSEFONTW)GetWindowLongA(hDlg, DWL_USER);
        if (!lpcf32w)
            return FALSE;
        if (CFn_HookCallChk32((LPCHOOSEFONTA)lpcf32w))
            res=CallWindowProcW((WNDPROC)lpcf32w->lpfnHook, hDlg, uMsg, wParam, lParam);
        if (res)
            return res;
    }
    else
    {
        lpcf32w=(LPCHOOSEFONTW)lParam;
        lpcf32a=(LPCHOOSEFONTA)lpcf32w->lpTemplateName;
        if (!CFn_WMInitDialog(hDlg, wParam, lParam, lpcf32a))
        {
            TRACE("CFn_WMInitDialog returned FALSE\n");
            return FALSE;
        }
        if (CFn_HookCallChk32((LPCHOOSEFONTA)lpcf32w))
            return CallWindowProcW((WNDPROC)lpcf32w->lpfnHook,hDlg,WM_INITDIALOG,wParam,lParam);
    }
    lpcf32a=(LPCHOOSEFONTA)lpcf32w->lpTemplateName;
    switch (uMsg)
    {
    case WM_MEASUREITEM:
        return CFn_WMMeasureItem(hDlg, wParam, lParam);
    case WM_DRAWITEM:
        return CFn_WMDrawItem(hDlg, wParam, lParam);
    case WM_COMMAND:
        return CFn_WMCommand(hDlg, wParam, lParam, lpcf32a);
    case WM_DESTROY:
        return CFn_WMDestroy(hDlg, wParam, lParam);
    case WM_CHOOSEFONT_GETLOGFONT:
        TRACE("WM_CHOOSEFONT_GETLOGFONT lParam=%08lX\n",
                lParam);
        FIXME("current logfont back to caller\n");
        break;
    }
    return res;
}
