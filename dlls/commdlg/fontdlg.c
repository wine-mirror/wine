/*
 * COMMDLG - Font Dialog
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Albrecht Kleine
 */

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "ldt.h"
#include "heap.h"
#include "commdlg.h"
#include "dialog.h"
#include "dlgs.h"
#include "module.h"
#include "debugtools.h"
#include "font.h"
#include "winproc.h"
#include "cderr.h"

DEFAULT_DEBUG_CHANNEL(commdlg);

#include "cdlg.h"

static HBITMAP16 hBitmapTT = 0; 


LRESULT WINAPI FormatCharDlgProcA(HWND hDlg, UINT uMsg, WPARAM wParam,
				  LPARAM lParam);
LRESULT WINAPI FormatCharDlgProcW(HWND hDlg, UINT uMsg, WPARAM wParam,
				  LPARAM lParam);


static void CFn_CHOOSEFONT16to32A(LPCHOOSEFONT16 chf16, LPCHOOSEFONTA chf32a)
{
  chf32a->lStructSize=sizeof(CHOOSEFONTA);
  chf32a->hwndOwner=chf16->hwndOwner;
  chf32a->hDC=chf16->hDC;
  chf32a->iPointSize=chf16->iPointSize;
  chf32a->Flags=chf16->Flags;
  chf32a->rgbColors=chf16->rgbColors;
  chf32a->lCustData=chf16->lCustData;
  chf32a->lpfnHook=NULL;
  chf32a->lpTemplateName=PTR_SEG_TO_LIN(chf16->lpTemplateName);
  chf32a->hInstance=chf16->hInstance;
  chf32a->lpszStyle=PTR_SEG_TO_LIN(chf16->lpszStyle);
  chf32a->nFontType=chf16->nFontType;
  chf32a->nSizeMax=chf16->nSizeMax;
  chf32a->nSizeMin=chf16->nSizeMin;
  FONT_LogFont16To32A(PTR_SEG_TO_LIN(chf16->lpLogFont), chf32a->lpLogFont);
}


/***********************************************************************
 *                        ChooseFont16   (COMMDLG.15)     
 */
BOOL16 WINAPI ChooseFont16(LPCHOOSEFONT16 lpChFont)
{
    HINSTANCE16 hInst;
    HANDLE16 hDlgTmpl = 0;
    BOOL16 bRet = FALSE, win32Format = FALSE;
    LPCVOID template;
    HWND hwndDialog;
    CHOOSEFONTA cf32a;
    LOGFONTA lf32a;
    SEGPTR lpTemplateName;
    
    cf32a.lpLogFont=&lf32a;
    CFn_CHOOSEFONT16to32A(lpChFont, &cf32a);

    TRACE("ChooseFont\n");
    if (!lpChFont) return FALSE;    

    if (lpChFont->Flags & CF_ENABLETEMPLATEHANDLE)
    {
        if (!(template = LockResource16( lpChFont->hInstance )))
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
            return FALSE;
        }
    }
    else if (lpChFont->Flags & CF_ENABLETEMPLATE)
    {
        HANDLE16 hResInfo;
        if (!(hResInfo = FindResource16( lpChFont->hInstance,
                                         lpChFont->lpTemplateName,
                                         RT_DIALOG16)))
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
            return FALSE;
        }
        if (!(hDlgTmpl = LoadResource16( lpChFont->hInstance, hResInfo )) ||
            !(template = LockResource16( hDlgTmpl )))
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
            return FALSE;
        }
    }
    else
    {
	HANDLE hResInfo;
	if (!(hResInfo = FindResourceA(COMMDLG_hInstance32, "CHOOSE_FONT", RT_DIALOGA)))
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
	    return FALSE;
	}
	if (!(hDlgTmpl = LoadResource(COMMDLG_hInstance32, hResInfo )) ||
	    !(template = LockResource( hDlgTmpl )))
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	    return FALSE;
	}
        win32Format = TRUE;
    }

    hInst = GetWindowLongA( lpChFont->hwndOwner, GWL_HINSTANCE );
    
    /* lpTemplateName is not used in the dialog */
    lpTemplateName=lpChFont->lpTemplateName;
    lpChFont->lpTemplateName=(SEGPTR)&cf32a;
    
    hwndDialog = DIALOG_CreateIndirect( hInst, template, win32Format,
                                        lpChFont->hwndOwner,
                                        (DLGPROC16)FormatCharDlgProcA,
                                        (DWORD)lpChFont, WIN_PROC_32A );
    if (hwndDialog) bRet = DIALOG_DoDialogBox(hwndDialog, lpChFont->hwndOwner);
    if (hDlgTmpl) FreeResource16( hDlgTmpl );
    lpChFont->lpTemplateName=lpTemplateName;
    FONT_LogFont32ATo16(cf32a.lpLogFont, 
        (LPLOGFONT16)(PTR_SEG_TO_LIN(lpChFont->lpLogFont)));
    return bRet;
}


/***********************************************************************
 *           ChooseFontA   (COMDLG32.3)
 */
BOOL WINAPI ChooseFontA(LPCHOOSEFONTA lpChFont)
{
  BOOL bRet=FALSE;
  HWND hwndDialog;
  HINSTANCE hInst=GetWindowLongA( lpChFont->hwndOwner, GWL_HINSTANCE );
  LPCVOID template;
  HANDLE hResInfo, hDlgTmpl;

  if (!(hResInfo = FindResourceA(COMMDLG_hInstance32, "CHOOSE_FONT", RT_DIALOGA)))
  {
    COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
    return FALSE;
  }
  if (!(hDlgTmpl = LoadResource(COMMDLG_hInstance32, hResInfo )) ||
      !(template = LockResource( hDlgTmpl )))
  {
    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
    return FALSE;
  }

  if (lpChFont->Flags & (CF_SELECTSCRIPT | CF_NOVERTFONTS | CF_ENABLETEMPLATE |
    CF_ENABLETEMPLATEHANDLE)) FIXME(": unimplemented flag (ignored)\n");
  hwndDialog = DIALOG_CreateIndirect(hInst, template, TRUE, lpChFont->hwndOwner,
            (DLGPROC16)FormatCharDlgProcA, (LPARAM)lpChFont, WIN_PROC_32A );
  if (hwndDialog) bRet = DIALOG_DoDialogBox(hwndDialog, lpChFont->hwndOwner);  
  return bRet;
}

/***********************************************************************
 *           ChooseFontW   (COMDLG32.4)
 */
BOOL WINAPI ChooseFontW(LPCHOOSEFONTW lpChFont)
{
  BOOL bRet=FALSE;
  HWND hwndDialog;
  HINSTANCE hInst=GetWindowLongA( lpChFont->hwndOwner, GWL_HINSTANCE );
  CHOOSEFONTA cf32a;
  LOGFONTA lf32a;
  LPCVOID template;
  HANDLE hResInfo, hDlgTmpl;

  if (!(hResInfo = FindResourceA(COMMDLG_hInstance32, "CHOOSE_FONT", RT_DIALOGA)))
  {
    COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
    return FALSE;
  }
  if (!(hDlgTmpl = LoadResource(COMMDLG_hInstance32, hResInfo )) ||
      !(template = LockResource( hDlgTmpl )))
  {
    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
    return FALSE;
  }

  if (lpChFont->Flags & (CF_SELECTSCRIPT | CF_NOVERTFONTS | CF_ENABLETEMPLATE |
    CF_ENABLETEMPLATEHANDLE)) FIXME(": unimplemented flag (ignored)\n");
  memcpy(&cf32a, lpChFont, sizeof(cf32a));
  memcpy(&lf32a, lpChFont->lpLogFont, sizeof(LOGFONTA));
  lstrcpynWtoA(lf32a.lfFaceName, lpChFont->lpLogFont->lfFaceName, LF_FACESIZE);
  cf32a.lpLogFont=&lf32a;
  cf32a.lpszStyle=HEAP_strdupWtoA(GetProcessHeap(), 0, lpChFont->lpszStyle);
  lpChFont->lpTemplateName=(LPWSTR)&cf32a;
  hwndDialog=DIALOG_CreateIndirect(hInst, template, TRUE, lpChFont->hwndOwner,
            (DLGPROC16)FormatCharDlgProcW, (LPARAM)lpChFont, WIN_PROC_32W );
  if (hwndDialog)bRet=DIALOG_DoDialogBox(hwndDialog, lpChFont->hwndOwner);  
  HeapFree(GetProcessHeap(), 0, cf32a.lpszStyle);
  lpChFont->lpTemplateName=(LPWSTR)cf32a.lpTemplateName;
  memcpy(lpChFont->lpLogFont, &lf32a, sizeof(CHOOSEFONTA));
  lstrcpynAtoW(lpChFont->lpLogFont->lfFaceName, lf32a.lfFaceName, LF_FACESIZE);
  return bRet;
}


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
 *                          CFn_HookCallChk                 [internal]
 */
static BOOL CFn_HookCallChk(LPCHOOSEFONT16 lpcf)
{
 if (lpcf)
  if(lpcf->Flags & CF_ENABLEHOOK)
   if (lpcf->lpfnHook)
    return TRUE;
 return FALSE;
}

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
static INT AddFontFamily(LPLOGFONTA lplf, UINT nFontType, 
                           LPCHOOSEFONTA lpcf, HWND hwnd)
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

typedef struct
{
  HWND hWnd1;
  HWND hWnd2;
  LPCHOOSEFONTA lpcf32a;
} CFn_ENUMSTRUCT, *LPCFn_ENUMSTRUCT;

/*************************************************************************
 *              FontFamilyEnumProc32                           [internal]
 */
static INT WINAPI FontFamilyEnumProc(LPENUMLOGFONTA lpEnumLogFont, 
	  LPNEWTEXTMETRICA metrics, UINT nFontType, LPARAM lParam)
{
  LPCFn_ENUMSTRUCT e;
  e=(LPCFn_ENUMSTRUCT)lParam;
  return AddFontFamily(&lpEnumLogFont->elfLogFont, nFontType, e->lpcf32a, e->hWnd1);
}

/***********************************************************************
 *                FontFamilyEnumProc16                     (COMMDLG.19)
 */
INT16 WINAPI FontFamilyEnumProc16( SEGPTR logfont, SEGPTR metrics,
                                   UINT16 nFontType, LPARAM lParam )
{
  HWND16 hwnd=LOWORD(lParam);
  HWND16 hDlg=GetParent16(hwnd);
  LPCHOOSEFONT16 lpcf=(LPCHOOSEFONT16)GetWindowLongA(hDlg, DWL_USER); 
  LOGFONT16 *lplf = (LOGFONT16 *)PTR_SEG_TO_LIN( logfont );
  LOGFONTA lf32a;
  FONT_LogFont16To32A(lplf, &lf32a);
  return AddFontFamily(&lf32a, nFontType, (LPCHOOSEFONTA)lpcf->lpTemplateName,
                       hwnd);
}

/*************************************************************************
 *              SetFontStylesToCombo2                           [internal]
 *
 * Fill font style information into combobox  (without using font.c directly)
 */
static int SetFontStylesToCombo2(HWND hwnd, HDC hdc, LPLOGFONTA lplf)
{
   #define FSTYLES 4
   struct FONTSTYLE
          { int italic; 
            int weight;
            char stname[20]; };
   static struct FONTSTYLE fontstyles[FSTYLES]={ 
          { 0,FW_NORMAL,"Regular"},{0,FW_BOLD,"Bold"},
          { 1,FW_NORMAL,"Italic"}, {1,FW_BOLD,"Bold Italic"}};
   HFONT16 hf;
   TEXTMETRIC16 tm;
   int i,j;

   for (i=0;i<FSTYLES;i++)
   {
     lplf->lfItalic=fontstyles[i].italic;
     lplf->lfWeight=fontstyles[i].weight;
     hf=CreateFontIndirectA(lplf);
     hf=SelectObject(hdc,hf);
     GetTextMetrics16(hdc,&tm);
     hf=SelectObject(hdc,hf);
     DeleteObject(hf);

     if (tm.tmWeight==fontstyles[i].weight &&
         tm.tmItalic==fontstyles[i].italic)    /* font successful created ? */
     {
       char *str = SEGPTR_STRDUP(fontstyles[i].stname);
       j=SendMessage16(hwnd,CB_ADDSTRING16,0,(LPARAM)SEGPTR_GET(str) );
       SEGPTR_FREE(str);
       if (j==CB_ERR) return 1;
       j=SendMessage16(hwnd, CB_SETITEMDATA16, j, 
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
static INT AddFontStyle(LPLOGFONTA lplf, UINT nFontType, 
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
       HDC hdc= (lpcf->Flags & CF_PRINTERFONTS && lpcf->hDC) ? lpcf->hDC : GetDC(hDlg);
       i=SetFontStylesToCombo2(hcmb2,hdc,lplf);
       if (!(lpcf->Flags & CF_PRINTERFONTS && lpcf->hDC))
         ReleaseDC(hDlg,hdc);
       if (i)
        return 0;
  }
  return 1 ;

}    

/***********************************************************************
 *                 FontStyleEnumProc16                     (COMMDLG.18)
 */
INT16 WINAPI FontStyleEnumProc16( SEGPTR logfont, SEGPTR metrics,
                                  UINT16 nFontType, LPARAM lParam )
{
  HWND16 hcmb2=LOWORD(lParam);
  HWND16 hcmb3=HIWORD(lParam);
  HWND16 hDlg=GetParent16(hcmb3);
  LPCHOOSEFONT16 lpcf=(LPCHOOSEFONT16)GetWindowLongA(hDlg, DWL_USER); 
  LOGFONT16 *lplf = (LOGFONT16 *)PTR_SEG_TO_LIN(logfont);
  LOGFONTA lf32a;
  FONT_LogFont16To32A(lplf, &lf32a);
  return AddFontStyle(&lf32a, nFontType, (LPCHOOSEFONTA)lpcf->lpTemplateName,
                      hcmb2, hcmb3, hDlg);
}

/***********************************************************************
 *                 FontStyleEnumProc32                     [internal]
 */
static INT WINAPI FontStyleEnumProc( LPENUMLOGFONTA lpFont, 
          LPNEWTEXTMETRICA metrics, UINT nFontType, LPARAM lParam )
{
  LPCFn_ENUMSTRUCT s=(LPCFn_ENUMSTRUCT)lParam;
  HWND hcmb2=s->hWnd1;
  HWND hcmb3=s->hWnd2;
  HWND hDlg=GetParent(hcmb3);
  return AddFontStyle(&lpFont->elfLogFont, nFontType, s->lpcf32a, hcmb2,
                      hcmb3, hDlg);
}

/***********************************************************************
 *           CFn_WMInitDialog                            [internal]
 */
static LRESULT CFn_WMInitDialog(HWND hDlg, WPARAM wParam, LPARAM lParam,
                         LPCHOOSEFONTA lpcf)
{
  HDC hdc;
  int i,j,res,init=0;
  long l;
  LPLOGFONTA lpxx;
  HCURSOR hcursor=SetCursor(LoadCursorA(0,IDC_WAITA));

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

  /* This font will be deleted by WM_COMMAND */
  SendDlgItemMessageA(hDlg,stc6,WM_SETFONT,
     CreateFontA(0, 0, 1, 1, 400, 0, 0, 0, 0, 0, 0, 0, 0, NULL),FALSE);
			 
  if (!(lpcf->Flags & CF_SHOWHELP) || !IsWindow(lpcf->hwndOwner))
    ShowWindow(GetDlgItem(hDlg,pshHelp),SW_HIDE);
  if (!(lpcf->Flags & CF_APPLY))
    ShowWindow(GetDlgItem(hDlg,psh3),SW_HIDE);
  if (lpcf->Flags & CF_EFFECTS)
  {
    for (res=1,i=0;res && i<TEXT_COLORS;i++)
    {
      /* FIXME: load color name from resource:  res=LoadString(...,i+....,buffer,.....); */
      char name[20];
      strcpy( name, "[color name]" );
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
  hdc= (lpcf->Flags & CF_PRINTERFONTS && lpcf->hDC) ? lpcf->hDC : GetDC(hDlg);
  if (hdc)
  {
    CFn_ENUMSTRUCT s;
    s.hWnd1=GetDlgItem(hDlg,cmb1);
    s.lpcf32a=lpcf;
    if (!EnumFontFamiliesA(hdc, NULL, FontFamilyEnumProc, (LPARAM)&s))
      TRACE("EnumFontFamilies returns 0\n");
    if (lpcf->Flags & CF_INITTOLOGFONTSTRUCT)
    {
      /* look for fitting font name in combobox1 */
      j=SendDlgItemMessageA(hDlg,cmb1,CB_FINDSTRING,-1,(LONG)lpxx->lfFaceName);
      if (j!=CB_ERR)
      {
        SendDlgItemMessageA(hDlg, cmb1, CB_SETCURSEL, j, 0);
	SendMessageA(hDlg, WM_COMMAND, MAKEWPARAM(cmb1, CBN_SELCHANGE),
                       GetDlgItem(hDlg,cmb1));
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
                       GetDlgItem(hDlg,cmb1));
    }    
    if (lpcf->Flags & CF_USESTYLE && lpcf->lpszStyle)
    {
      j=SendDlgItemMessageA(hDlg,cmb2,CB_FINDSTRING,-1,(LONG)lpcf->lpszStyle);
      if (j!=CB_ERR)
      {
        j=SendDlgItemMessageA(hDlg,cmb2,CB_SETCURSEL,j,0);
        SendMessageA(hDlg,WM_COMMAND,cmb2,
                       MAKELONG(GetDlgItem(hDlg,cmb2),CBN_SELCHANGE));
      }
    }
  }
  else
  {
    WARN("HDC failure !!!\n");
    EndDialog (hDlg, 0); 
    return FALSE;
  }

  if (!(lpcf->Flags & CF_PRINTERFONTS && lpcf->hDC))
    ReleaseDC(hDlg,hdc);
  SetCursor(hcursor);   
  return TRUE;
}


/***********************************************************************
 *           CFn_WMMeasureItem                           [internal]
 */
static LRESULT CFn_WMMeasureItem(HWND hDlg, WPARAM wParam, LPARAM lParam)
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
static LRESULT CFn_WMDrawItem(HWND hDlg, WPARAM wParam, LPARAM lParam)
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

  if (lpdi->itemID == 0xFFFF) 			/* got no items */
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
     return TRUE;	/* this should never happen */

   rect=lpdi->rcItem;
   switch (lpdi->CtlID)
   {
    case cmb1:	/* TRACE(commdlg,"WM_Drawitem cmb1\n"); */
		SendMessageA(lpdi->hwndItem, CB_GETLBTEXT, lpdi->itemID,
			       (LPARAM)buffer);	          
		GetObjectA( hBitmapTT, sizeof(bm), &bm );
		TextOutA(lpdi->hDC, lpdi->rcItem.left + bm.bmWidth + 10,
                           lpdi->rcItem.top, buffer, lstrlenA(buffer));
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
    case cmb3:	/* TRACE(commdlg,"WM_DRAWITEN cmb2,cmb3\n"); */
		SendMessageA(lpdi->hwndItem, CB_GETLBTEXT, lpdi->itemID,
		               (LPARAM)buffer);
		TextOutA(lpdi->hDC, lpdi->rcItem.left,
                           lpdi->rcItem.top, buffer, lstrlenA(buffer));
		break;

    case cmb4:	/* TRACE(commdlg,"WM_DRAWITEM cmb4 (=COLOR)\n"); */
		SendMessageA(lpdi->hwndItem, CB_GETLBTEXT, lpdi->itemID,
    		               (LPARAM)buffer);
		TextOutA(lpdi->hDC, lpdi->rcItem.left +  25+5,
                           lpdi->rcItem.top, buffer, lstrlenA(buffer));
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

    default:	return TRUE;	/* this should never happen */
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
 *           CFn_WMCtlColor                              [internal]
 */
static LRESULT CFn_WMCtlColorStatic(HWND hDlg, WPARAM wParam, LPARAM lParam,
                             LPCHOOSEFONTA lpcf)
{
  if (lpcf->Flags & CF_EFFECTS)
   if (GetDlgCtrlID(lParam)==stc6)
   {
     SetTextColor((HDC)wParam, lpcf->rgbColors);
     return GetStockObject(WHITE_BRUSH);
   }
  return 0;
}

/***********************************************************************
 *           CFn_WMCommand                               [internal]
 */
static LRESULT CFn_WMCommand(HWND hDlg, WPARAM wParam, LPARAM lParam,
                      LPCHOOSEFONTA lpcf)
{
  HFONT hFont;
  int i,j;
  long l;
  HDC hdc;
  LPLOGFONTA lpxx=lpcf->lpLogFont;
  
  TRACE("WM_COMMAND wParam=%08lX lParam=%08lX\n", (LONG)wParam, lParam);
  switch (LOWORD(wParam))
  {
	case cmb1:if (HIWORD(wParam)==CBN_SELCHANGE)
		  {
		    hdc=(lpcf->Flags & CF_PRINTERFONTS && lpcf->hDC) ? lpcf->hDC : GetDC(hDlg);
		    if (hdc)
		    {
                      SendDlgItemMessageA(hDlg, cmb2, CB_RESETCONTENT16, 0, 0); 
		      SendDlgItemMessageA(hDlg, cmb3, CB_RESETCONTENT16, 0, 0);
		      i=SendDlgItemMessageA(hDlg, cmb1, CB_GETCURSEL16, 0, 0);
		      if (i!=CB_ERR)
		      {
		        HCURSOR hcursor=SetCursor(LoadCursorA(0,IDC_WAITA));
			CFn_ENUMSTRUCT s;
                        char str[256];
                        SendDlgItemMessageA(hDlg, cmb1, CB_GETLBTEXT, i,
                                              (LPARAM)str);
	                TRACE("WM_COMMAND/cmb1 =>%s\n",str);
			s.hWnd1=GetDlgItem(hDlg, cmb2);
			s.hWnd2=GetDlgItem(hDlg, cmb3);
			s.lpcf32a=lpcf;
       		        EnumFontFamiliesA(hdc, str, FontStyleEnumProc, (LPARAM)&s);
		        SetCursor(hcursor);
		      }
		      if (!(lpcf->Flags & CF_PRINTERFONTS && lpcf->hDC))
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
	case cmb3:if (HIWORD(wParam)==CBN_SELCHANGE || HIWORD(wParam)== BN_CLICKED )
	          {
                    char str[256];
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

		    hFont=CreateFontIndirectA(lpxx);
		    if (hFont)
		    {
		      HFONT oldFont=SendDlgItemMessageA(hDlg, stc6, 
		          WM_GETFONT, 0, 0);
		      SendDlgItemMessageA(hDlg,stc6,WM_SETFONT,hFont,TRUE);
		      DeleteObject(oldFont);
		    }
                  }
                  break;

	case cmb4:i=SendDlgItemMessageA(hDlg, cmb4, CB_GETCURSEL, 0, 0);
		  if (i!=CB_ERR)
		  {
		   lpcf->rgbColors=textcolors[i];
		   InvalidateRect( GetDlgItem(hDlg,stc6), NULL, 0 );
		  }
		  break;
	
	case psh15:i=RegisterWindowMessageA( HELPMSGSTRING );
		  if (lpcf->hwndOwner)
		    SendMessageA(lpcf->hwndOwner, i, 0, (LPARAM)GetWindowLongA(hDlg, DWL_USER));
/*		  if (CFn_HookCallChk(lpcf))
		    CallWindowProc16(lpcf->lpfnHook,hDlg,WM_COMMAND,psh15,(LPARAM)lpcf);*/
		  break;

	case IDOK:if (  (!(lpcf->Flags & CF_LIMITSIZE))  ||
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
	case IDCANCEL:EndDialog(hDlg, FALSE);
		  return(TRUE);
	}
      return(FALSE);
}

static LRESULT CFn_WMDestroy(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  DeleteObject(SendDlgItemMessageA(hwnd, stc6, WM_GETFONT, 0, 0));
  return TRUE;
}


/***********************************************************************
 *           FormatCharDlgProc16   (COMMDLG.16)
             FIXME: 1. some strings are "hardcoded", but it's better load from sysres
                    2. some CF_.. flags are not supported
                    3. some TType extensions
 */
LRESULT WINAPI FormatCharDlgProc16(HWND16 hDlg, UINT16 message, WPARAM16 wParam,
                                   LPARAM lParam)
{
  LPCHOOSEFONT16 lpcf;
  LPCHOOSEFONTA lpcf32a;
  UINT uMsg32;
  WPARAM wParam32;
  LRESULT res=0;  
  if (message!=WM_INITDIALOG)
  {
   lpcf=(LPCHOOSEFONT16)GetWindowLongA(hDlg, DWL_USER);   
   if (!lpcf)
      return FALSE;
   if (CFn_HookCallChk(lpcf))
     res=CallWindowProc16((WNDPROC16)lpcf->lpfnHook,hDlg,message,wParam,lParam);
   if (res)
    return res;
  }
  else
  {
    lpcf=(LPCHOOSEFONT16)lParam;
    lpcf32a=(LPCHOOSEFONTA)lpcf->lpTemplateName;
    if (!CFn_WMInitDialog(hDlg, wParam, lParam, lpcf32a)) 
    {
      TRACE("CFn_WMInitDialog returned FALSE\n");
      return FALSE;
    }  
    if (CFn_HookCallChk(lpcf))
      return CallWindowProc16((WNDPROC16)lpcf->lpfnHook,hDlg,WM_INITDIALOG,wParam,lParam);
  }
  WINPROC_MapMsg16To32A(message, wParam, &uMsg32, &wParam32, &lParam);
  lpcf32a=(LPCHOOSEFONTA)lpcf->lpTemplateName;
  switch (uMsg32)
    {
      case WM_MEASUREITEM:
                        res=CFn_WMMeasureItem(hDlg, wParam32, lParam);
			break;
      case WM_DRAWITEM:
                        res=CFn_WMDrawItem(hDlg, wParam32, lParam);
			break;
      case WM_CTLCOLORSTATIC:
                        res=CFn_WMCtlColorStatic(hDlg, wParam32, lParam, lpcf32a);
			break;
      case WM_COMMAND:
                        res=CFn_WMCommand(hDlg, wParam32, lParam, lpcf32a);
			break;
      case WM_DESTROY:
                        res=CFn_WMDestroy(hDlg, wParam32, lParam);
			break;
      case WM_CHOOSEFONT_GETLOGFONT: 
                         TRACE("WM_CHOOSEFONT_GETLOGFONT lParam=%08lX\n",
				      lParam);
			 FIXME("current logfont back to caller\n");
                        break;
    }
  WINPROC_UnmapMsg16To32A(hDlg,uMsg32, wParam32, lParam, res);    
  return res;
}

/***********************************************************************
 *           FormatCharDlgProcA   [internal]
 */
LRESULT WINAPI FormatCharDlgProcA(HWND hDlg, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam)
{
  LPCHOOSEFONTA lpcf;
  LRESULT res=FALSE;
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
      case WM_CTLCOLORSTATIC:
                        return CFn_WMCtlColorStatic(hDlg, wParam, lParam, lpcf);
      case WM_COMMAND:
                        return CFn_WMCommand(hDlg, wParam, lParam, lpcf);
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

/***********************************************************************
 *           FormatCharDlgProcW   [internal]
 */
LRESULT WINAPI FormatCharDlgProcW(HWND hDlg, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam)
{
  LPCHOOSEFONTW lpcf32w;
  LPCHOOSEFONTA lpcf32a;
  LRESULT res=FALSE;
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
      case WM_CTLCOLORSTATIC:
                        return CFn_WMCtlColorStatic(hDlg, wParam, lParam, lpcf32a);
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


