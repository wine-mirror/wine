/*
 * COMMDLG functions
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Albrecht Kleine
 * Copyright 1999 Klaas van Gend
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "winbase.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "win.h"
#include "heap.h"
#include "message.h"
#include "commdlg.h"
#include "resource.h"
#include "dialog.h"
#include "dlgs.h"
#include "module.h"
#include "drive.h"
#include "debug.h"
#include "font.h"
#include "winproc.h"

static	DWORD 		CommDlgLastError = 0;

static HBITMAP16 hFolder = 0;
static HBITMAP16 hFolder2 = 0;
static HBITMAP16 hFloppy = 0;
static HBITMAP16 hHDisk = 0;
static HBITMAP16 hCDRom = 0;
static HBITMAP16 hBitmapTT = 0;
static const char defaultfilter[]=" \0\0";

/***********************************************************************
 * 				FileDlg_Init			[internal]
 */
static BOOL FileDlg_Init(void)
{
    static BOOL initialized = 0;
    
    if (!initialized) {
	if (!hFolder) hFolder = LoadBitmap16(0, MAKEINTRESOURCE16(OBM_FOLDER));
	if (!hFolder2) hFolder2 = LoadBitmap16(0, MAKEINTRESOURCE16(OBM_FOLDER2));
	if (!hFloppy) hFloppy = LoadBitmap16(0, MAKEINTRESOURCE16(OBM_FLOPPY));
	if (!hHDisk) hHDisk = LoadBitmap16(0, MAKEINTRESOURCE16(OBM_HDISK));
	if (!hCDRom) hCDRom = LoadBitmap16(0, MAKEINTRESOURCE16(OBM_CDROM));
	if (hFolder == 0 || hFolder2 == 0 || hFloppy == 0 || 
	    hHDisk == 0 || hCDRom == 0)
	{	
	    WARN(commdlg, "Error loading bitmaps !\nprin");
	    return FALSE;
	}
	initialized = TRUE;
    }
    return TRUE;
}

/***********************************************************************
 *           GetOpenFileName   (COMMDLG.1)
 */
BOOL16 WINAPI GetOpenFileName16( SEGPTR ofn )
{
    HINSTANCE hInst;
    HANDLE hDlgTmpl = 0, hResInfo;
    BOOL bRet = FALSE, win32Format = FALSE;
    HWND hwndDialog;
    LPOPENFILENAME16 lpofn = (LPOPENFILENAME16)PTR_SEG_TO_LIN(ofn);
    LPCVOID template;
    char defaultopen[]="Open File";
    char *str=0,*str1=0;

    if (!lpofn || !FileDlg_Init()) return FALSE;

    if (lpofn->Flags & OFN_WINE) {
	    if (lpofn->Flags & OFN_ENABLETEMPLATEHANDLE)
	    {
		if (!(template = LockResource( MapHModuleSL(lpofn->hInstance ))))
		{
		    CommDlgLastError = CDERR_LOADRESFAILURE;
		    return FALSE;
		}
	    }
	    else if (lpofn->Flags & OFN_ENABLETEMPLATE)
	    {
		if (!(hResInfo = FindResourceA(MapHModuleSL(lpofn->hInstance),
						PTR_SEG_TO_LIN(lpofn->lpTemplateName), RT_DIALOGA)))
		{
		    CommDlgLastError = CDERR_FINDRESFAILURE;
		    return FALSE;
		}
		if (!(hDlgTmpl = LoadResource( MapHModuleSL(lpofn->hInstance),
						 hResInfo )) ||
		    !(template = LockResource( hDlgTmpl )))
		{
		    CommDlgLastError = CDERR_LOADRESFAILURE;
		    return FALSE;
		}
	    } else {
		template = SYSRES_GetResPtr( SYSRES_DIALOG_OPEN_FILE );
	    }
	    win32Format = TRUE;
    } else {
	    if (lpofn->Flags & OFN_ENABLETEMPLATEHANDLE)
	    {
		if (!(template = LockResource16( lpofn->hInstance )))
		{
		    CommDlgLastError = CDERR_LOADRESFAILURE;
		    return FALSE;
		}
	    }
	    else if (lpofn->Flags & OFN_ENABLETEMPLATE)
	    {
		if (!(hResInfo = FindResource16(lpofn->hInstance,
						lpofn->lpTemplateName,
                                                RT_DIALOG16)))
		{
		    CommDlgLastError = CDERR_FINDRESFAILURE;
		    return FALSE;
		}
		if (!(hDlgTmpl = LoadResource16( lpofn->hInstance, hResInfo )) ||
		    !(template = LockResource16( hDlgTmpl )))
		{
		    CommDlgLastError = CDERR_LOADRESFAILURE;
		    return FALSE;
		}
	    } else {
		template = SYSRES_GetResPtr( SYSRES_DIALOG_OPEN_FILE );
		win32Format = TRUE;
	    }
    }

    hInst = WIN_GetWindowInstance( lpofn->hwndOwner );

    if (!(lpofn->lpstrFilter))
      {
       str = SEGPTR_ALLOC(sizeof(defaultfilter));
       TRACE(commdlg,"Alloc %p default for Filetype in GetOpenFileName\n",str);
       memcpy(str,defaultfilter,sizeof(defaultfilter));
       lpofn->lpstrFilter=SEGPTR_GET(str);
      }

    if (!(lpofn->lpstrTitle))
      {
       str1 = SEGPTR_ALLOC(strlen(defaultopen)+1);
       TRACE(commdlg,"Alloc %p default for Title in GetOpenFileName\n",str1);
       strcpy(str1,defaultopen);
       lpofn->lpstrTitle=SEGPTR_GET(str1);
      }

    /* FIXME: doesn't handle win32 format correctly yet */
    hwndDialog = DIALOG_CreateIndirect( hInst, template, win32Format,
                                        lpofn->hwndOwner,
                                        (DLGPROC16)MODULE_GetWndProcEntry16("FileOpenDlgProc"),
                                        ofn, WIN_PROC_16 );
    if (hwndDialog) bRet = DIALOG_DoDialogBox( hwndDialog, lpofn->hwndOwner );

    if (str1)
      {
       TRACE(commdlg,"Freeing %p default for Title in GetOpenFileName\n",str1);
        SEGPTR_FREE(str1);
       lpofn->lpstrTitle=0;
      }

    if (str)
      {
       TRACE(commdlg,"Freeing %p default for Filetype in GetOpenFileName\n",str);
        SEGPTR_FREE(str);
       lpofn->lpstrFilter=0;
      }

    if (hDlgTmpl) {
	    if (lpofn->Flags & OFN_WINE)
		    FreeResource( hDlgTmpl );
	    else
		    FreeResource16( hDlgTmpl );
    }

    TRACE(commdlg,"return lpstrFile='%s' !\n", 
           (LPSTR)PTR_SEG_TO_LIN(lpofn->lpstrFile));
    return bRet;
}


/***********************************************************************
 *           GetSaveFileName   (COMMDLG.2)
 */
BOOL16 WINAPI GetSaveFileName16( SEGPTR ofn)
{
    HINSTANCE hInst;
    HANDLE hDlgTmpl = 0;
    BOOL bRet = FALSE, win32Format = FALSE;
    LPOPENFILENAME16 lpofn = (LPOPENFILENAME16)PTR_SEG_TO_LIN(ofn);
    LPCVOID template;
    HWND hwndDialog;
    char defaultsave[]="Save as";
    char *str =0,*str1=0;

    if (!lpofn || !FileDlg_Init()) return FALSE;

    if (lpofn->Flags & OFN_WINE) {
	    if (lpofn->Flags & OFN_ENABLETEMPLATEHANDLE)
	    {
		if (!(template = LockResource( MapHModuleSL(lpofn->hInstance ))))
		{
		    CommDlgLastError = CDERR_LOADRESFAILURE;
		    return FALSE;
		}
	    }
	    else if (lpofn->Flags & OFN_ENABLETEMPLATE)
	    {
		HANDLE hResInfo;
		if (!(hResInfo = FindResourceA(MapHModuleSL(lpofn->hInstance),
						 PTR_SEG_TO_LIN(lpofn->lpTemplateName),
                                                 RT_DIALOGA)))
		{
		    CommDlgLastError = CDERR_FINDRESFAILURE;
		    return FALSE;
		}
		if (!(hDlgTmpl = LoadResource(MapHModuleSL(lpofn->hInstance),
						hResInfo)) ||
		    !(template = LockResource(hDlgTmpl)))
		{
		    CommDlgLastError = CDERR_LOADRESFAILURE;
		    return FALSE;
		}
		win32Format= TRUE;
	    } else {
		template = SYSRES_GetResPtr( SYSRES_DIALOG_SAVE_FILE );
		win32Format = TRUE;
	    }
    } else {
	    if (lpofn->Flags & OFN_ENABLETEMPLATEHANDLE)
	    {
		if (!(template = LockResource16( lpofn->hInstance )))
		{
		    CommDlgLastError = CDERR_LOADRESFAILURE;
		    return FALSE;
		}
	    }
	    else if (lpofn->Flags & OFN_ENABLETEMPLATE)
	    {
		HANDLE16 hResInfo;
		if (!(hResInfo = FindResource16(lpofn->hInstance,
						lpofn->lpTemplateName,
                                                RT_DIALOG16)))
		{
		    CommDlgLastError = CDERR_FINDRESFAILURE;
		    return FALSE;
		}
		if (!(hDlgTmpl = LoadResource16( lpofn->hInstance, hResInfo )) ||
		    !(template = LockResource16( hDlgTmpl )))
		{
		    CommDlgLastError = CDERR_LOADRESFAILURE;
		    return FALSE;
		}
	} else {
		template = SYSRES_GetResPtr( SYSRES_DIALOG_SAVE_FILE );
		win32Format = TRUE;
	}
    }

    hInst = WIN_GetWindowInstance( lpofn->hwndOwner );

    if (!(lpofn->lpstrFilter))
      {
       str = SEGPTR_ALLOC(sizeof(defaultfilter));
       TRACE(commdlg,"Alloc default for Filetype in GetSaveFileName\n");
       memcpy(str,defaultfilter,sizeof(defaultfilter));
       lpofn->lpstrFilter=SEGPTR_GET(str);
      }

    if (!(lpofn->lpstrTitle))
      {
       str1 = SEGPTR_ALLOC(sizeof(defaultsave)+1);
       TRACE(commdlg,"Alloc default for Title in GetSaveFileName\n");
       strcpy(str1,defaultsave);
       lpofn->lpstrTitle=SEGPTR_GET(str1);
      }

    hwndDialog = DIALOG_CreateIndirect( hInst, template, win32Format,
                                        lpofn->hwndOwner,
                                        (DLGPROC16)MODULE_GetWndProcEntry16("FileSaveDlgProc"),
                                        ofn, WIN_PROC_16 );
    if (hwndDialog) bRet = DIALOG_DoDialogBox( hwndDialog, lpofn->hwndOwner );

    if (str1)
      {
       TRACE(commdlg,"Freeing %p default for Title in GetSaveFileName\n",str1);
        SEGPTR_FREE(str1);
       lpofn->lpstrTitle=0;
      }
 
    if (str)
      {
       TRACE(commdlg,"Freeing %p default for Filetype in GetSaveFileName\n",str);
        SEGPTR_FREE(str);
       lpofn->lpstrFilter=0;
      }
    
    if (hDlgTmpl) {
	    if (lpofn->Flags & OFN_WINE)
		    FreeResource( hDlgTmpl );
	    else
		    FreeResource16( hDlgTmpl );
    }

    TRACE(commdlg, "return lpstrFile='%s' !\n", 
            (LPSTR)PTR_SEG_TO_LIN(lpofn->lpstrFile));
    return bRet;
}

/***********************************************************************
 *                              FILEDLG_StripEditControl        [internal]
 * Strip pathnames off the contents of the edit control.
 */
static void FILEDLG_StripEditControl(HWND16 hwnd)
{
    char temp[512], *cp;

    GetDlgItemTextA( hwnd, edt1, temp, sizeof(temp) );
    cp = strrchr(temp, '\\');
    if (cp != NULL) {
	strcpy(temp, cp+1);
    }
    cp = strrchr(temp, ':');
    if (cp != NULL) {
	strcpy(temp, cp+1);
    }
    /* FIXME: shouldn't we do something with the result here? ;-) */
}

/***********************************************************************
 * 				FILEDLG_ScanDir			[internal]
 */
static BOOL FILEDLG_ScanDir(HWND16 hWnd, LPSTR newPath)
{
    char 		buffer[512];
	 char*  		str = buffer;
    int 			drive;
    HWND 		hlb;

    lstrcpynA(buffer, newPath, sizeof(buffer));

    if (str[0] && (str[1] == ':')) {
        drive = toupper(str[0]) - 'A';
        str += 2;
        if (!DRIVE_SetCurrentDrive(drive)) 
			  return FALSE;
    } else {
		 drive = DRIVE_GetCurrentDrive();
	 }

    if (str[0] && !DRIVE_Chdir(drive, str)) {
		 return FALSE;
    }

    GetDlgItemTextA(hWnd, edt1, buffer, sizeof(buffer));
    if ((hlb = GetDlgItem(hWnd, lst1)) != 0) {
		 char*	scptr; /* ptr on semi-colon */
		 char*	filter = buffer;

		 TRACE(commdlg, "Using filter %s\n", filter);
		 SendMessageA(hlb, LB_RESETCONTENT, 0, 0);
		 while (filter) {
			 scptr = strchr(filter, ';');
			 if (scptr)	*scptr = 0;
			 TRACE(commdlg, "Using file spec %s\n", filter);
			 if (SendMessageA(hlb, LB_DIR, 0, (LPARAM)filter) == LB_ERR)
				 return FALSE;
			 if (scptr) *scptr = ';';
			 filter = (scptr) ? (scptr + 1) : 0;
		 }
	 }

    strcpy(buffer, "*.*");
    return DlgDirListA(hWnd, buffer, lst2, stc1, 0x8010);
}

/***********************************************************************
 * 				FILEDLG_GetFileType		[internal]
 */

static LPSTR FILEDLG_GetFileType(LPSTR cfptr, LPSTR fptr, WORD index)
{
  int n, i;
  i = 0;
  if (cfptr)
    for ( ;(n = strlen(cfptr)) != 0; i++) 
      {
	cfptr += n + 1;
	if (i == index)
	  return cfptr;
	cfptr += strlen(cfptr) + 1;
      }
  if (fptr)
    for ( ;(n = strlen(fptr)) != 0; i++) 
      {
	fptr += n + 1;
	if (i == index)
	  return fptr;
	fptr += strlen(fptr) + 1;
    }
  return "*.*"; /* FIXME */
}

/***********************************************************************
 *                              FILEDLG_WMDrawItem              [internal]
 */
static LONG FILEDLG_WMDrawItem(HWND16 hWnd, WPARAM16 wParam, LPARAM lParam,int savedlg)
{
    LPDRAWITEMSTRUCT16 lpdis = (LPDRAWITEMSTRUCT16)PTR_SEG_TO_LIN(lParam);
    char *str;
    HBRUSH hBrush;
    HBITMAP16 hBitmap, hPrevBitmap;
    BITMAP16 bm;
    HDC hMemDC;

    if (lpdis->CtlType == ODT_LISTBOX && lpdis->CtlID == lst1)
    {
        if (!(str = SEGPTR_ALLOC(512))) return FALSE;
	hBrush = SelectObject(lpdis->hDC, GetStockObject(LTGRAY_BRUSH));
	SelectObject(lpdis->hDC, hBrush);
	FillRect16(lpdis->hDC, &lpdis->rcItem, hBrush);
	SendMessage16(lpdis->hwndItem, LB_GETTEXT16, lpdis->itemID, 
                      (LPARAM)SEGPTR_GET(str));

	if (savedlg)       /* use _gray_ text in FileSaveDlg */
        {
	  if (!lpdis->itemState)
	    SetTextColor(lpdis->hDC,GetSysColor(COLOR_GRAYTEXT) );
	  else  
	    SetTextColor(lpdis->hDC,GetSysColor(COLOR_WINDOWTEXT) );
	    /* inversion of gray would be bad readable */	  	  
        }

	TextOut16(lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top,
                  str, strlen(str));
	if (lpdis->itemState != 0) {
	    InvertRect16(lpdis->hDC, &lpdis->rcItem);
	}
        SEGPTR_FREE(str);
	return TRUE;
    }
    
    if (lpdis->CtlType == ODT_LISTBOX && lpdis->CtlID == lst2)
    {
        if (!(str = SEGPTR_ALLOC(512))) return FALSE;
	hBrush = SelectObject(lpdis->hDC, GetStockObject(LTGRAY_BRUSH));
	SelectObject(lpdis->hDC, hBrush);
	FillRect16(lpdis->hDC, &lpdis->rcItem, hBrush);
	SendMessage16(lpdis->hwndItem, LB_GETTEXT16, lpdis->itemID, 
                      (LPARAM)SEGPTR_GET(str));

	hBitmap = hFolder;
	GetObject16( hBitmap, sizeof(bm), &bm );
	TextOut16(lpdis->hDC, lpdis->rcItem.left + bm.bmWidth, 
                  lpdis->rcItem.top, str, strlen(str));
	hMemDC = CreateCompatibleDC(lpdis->hDC);
	hPrevBitmap = SelectObject(hMemDC, hBitmap);
	BitBlt(lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top,
                 bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
	SelectObject(hMemDC, hPrevBitmap);
	DeleteDC(hMemDC);
	if (lpdis->itemState != 0) InvertRect16(lpdis->hDC, &lpdis->rcItem);
        SEGPTR_FREE(str);
	return TRUE;
    }
    if (lpdis->CtlType == ODT_COMBOBOX && lpdis->CtlID == cmb2)
    {
        if (!(str = SEGPTR_ALLOC(512))) return FALSE;
	hBrush = SelectObject(lpdis->hDC, GetStockObject(LTGRAY_BRUSH));
	SelectObject(lpdis->hDC, hBrush);
	FillRect16(lpdis->hDC, &lpdis->rcItem, hBrush);
	SendMessage16(lpdis->hwndItem, CB_GETLBTEXT16, lpdis->itemID, 
                      (LPARAM)SEGPTR_GET(str));
        switch(DRIVE_GetType( str[2] - 'a' ))
        {
        case TYPE_FLOPPY:  hBitmap = hFloppy; break;
        case TYPE_CDROM:   hBitmap = hCDRom; break;
        case TYPE_HD:
        case TYPE_NETWORK:
        default:           hBitmap = hHDisk; break;
        }
	GetObject16( hBitmap, sizeof(bm), &bm );
	TextOut16(lpdis->hDC, lpdis->rcItem.left + bm.bmWidth, 
                  lpdis->rcItem.top, str, strlen(str));
	hMemDC = CreateCompatibleDC(lpdis->hDC);
	hPrevBitmap = SelectObject(hMemDC, hBitmap);
	BitBlt( lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top,
                  bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY );
	SelectObject(hMemDC, hPrevBitmap);
	DeleteDC(hMemDC);
	if (lpdis->itemState != 0) InvertRect16(lpdis->hDC, &lpdis->rcItem);
        SEGPTR_FREE(str);
	return TRUE;
    }
    return FALSE;
}

/***********************************************************************
 *                              FILEDLG_WMMeasureItem           [internal]
 */
static LONG FILEDLG_WMMeasureItem(HWND16 hWnd, WPARAM16 wParam, LPARAM lParam) 
{
    BITMAP16 bm;
    LPMEASUREITEMSTRUCT16 lpmeasure;
    
    GetObject16( hFolder2, sizeof(bm), &bm );
    lpmeasure = (LPMEASUREITEMSTRUCT16)PTR_SEG_TO_LIN(lParam);
    lpmeasure->itemHeight = bm.bmHeight;
    return TRUE;
}

/***********************************************************************
 *                              FILEDLG_HookCallChk             [internal]
 */
static int FILEDLG_HookCallChk(LPOPENFILENAME16 lpofn)
{
 if (lpofn)
  if (lpofn->Flags & OFN_ENABLEHOOK)
   if (lpofn->lpfnHook)
    return 1;
 return 0;   
} 

/***********************************************************************
 *                              FILEDLG_CallWindowProc             [internal]
 *
 * Adapt the structures back for win32 calls so the callee can read lpCustData
 */
static BOOL FILEDLG_CallWindowProc(LPOPENFILENAME16 lpofn,HWND hwnd,
	UINT wMsg,WPARAM wParam,LPARAM lParam

) {
	BOOL	        needstruct;
        BOOL          result = FALSE;
        WINDOWPROCTYPE  ProcType;               /* Type of Hook Function to be called. */

        /* TRUE if lParam points to the OPENFILENAME16 Structure */
	needstruct = (PTR_SEG_TO_LIN(lParam) == lpofn);

        ProcType   = (lpofn->Flags & OFN_WINE)
                     ? (lpofn->Flags & OFN_UNICODE)             /* 32-Bit call to GetOpenFileName */
                       ? WIN_PROC_32W : WIN_PROC_32A
                     : WIN_PROC_16;                             /* 16-Bit call to GetOpenFileName */

	if (!(lpofn->Flags & OFN_WINE))
                /* Call to 16-Bit Hooking function... No Problem at all. */
		return (BOOL)CallWindowProc16(
			lpofn->lpfnHook,hwnd,(UINT16)wMsg,(WPARAM16)wParam,lParam
		);
	/* |OFN_WINE32 */
        if (needstruct)
        {
           /* Parameter lParam points to lpofn... Convert Structure Data... */
       	   if (lpofn->Flags & OFN_UNICODE)
           {
                OPENFILENAMEW ofnw;

                /* FIXME: probably needs more converted */
                ofnw.lCustData = lpofn->lCustData;
                return (BOOL)CallWindowProcW(
                         (WNDPROC)lpofn->lpfnHook,hwnd,wMsg,wParam,(LPARAM)&ofnw
                );
           }
           else /* ! |OFN_UNICODE */
           {
		OPENFILENAMEA ofna;

		/* FIXME: probably needs more converted */
		ofna.lCustData = lpofn->lCustData;
		return (BOOL)CallWindowProcA(
		        (WNDPROC)lpofn->lpfnHook,hwnd,wMsg,wParam,(LPARAM)&ofna
		);
           }
	}
        else /* ! needstruct */
        {
                HWINDOWPROC     hWindowProc=NULL;

                if (WINPROC_SetProc(&hWindowProc, lpofn->lpfnHook, ProcType, WIN_PROC_WINDOW))
                {
                    /* Call Window Procedure converting 16-Bit Type Parameters to 32-Bit Type Parameters */
                    result = CallWindowProc16( (WNDPROC16)hWindowProc,
                                                      hwnd, wMsg, wParam, lParam );

                    result = LOWORD(result);

                    WINPROC_FreeProc( hWindowProc, WIN_PROC_WINDOW );
                }

                return result;

        }
}


/***********************************************************************
 *                              FILEDLG_WMInitDialog            [internal]
 */

static LONG FILEDLG_WMInitDialog(HWND16 hWnd, WPARAM16 wParam, LPARAM lParam) 
{
  int i, n;
  LPOPENFILENAME16 lpofn;
  char tmpstr[512];
  LPSTR pstr, old_pstr;
  SetWindowLongA(hWnd, DWL_USER, lParam);
  lpofn = (LPOPENFILENAME16)PTR_SEG_TO_LIN(lParam);
  if (lpofn->lpstrTitle) SetWindowText16( hWnd, lpofn->lpstrTitle );
  /* read custom filter information */
  if (lpofn->lpstrCustomFilter)
    {
      pstr = (LPSTR)PTR_SEG_TO_LIN(lpofn->lpstrCustomFilter);
      n = 0;
      TRACE(commdlg,"lpstrCustomFilter = %p\n", pstr);
      while(*pstr)
	{
	  old_pstr = pstr;
          i = SendDlgItemMessage16(hWnd, cmb1, CB_ADDSTRING16, 0,
                                   (LPARAM)lpofn->lpstrCustomFilter + n );
          n += strlen(pstr) + 1;
	  pstr += strlen(pstr) + 1;
	  TRACE(commdlg,"add str='%s' "
			  "associated to '%s'\n", old_pstr, pstr);
          SendDlgItemMessage16(hWnd, cmb1, CB_SETITEMDATA16, i, (LPARAM)pstr);
          n += strlen(pstr) + 1;
	  pstr += strlen(pstr) + 1;
	}
    }
  /* read filter information */
  if (lpofn->lpstrFilter) {
	pstr = (LPSTR)PTR_SEG_TO_LIN(lpofn->lpstrFilter);
	n = 0;
	while(*pstr) {
	  old_pstr = pstr;
	  i = SendDlgItemMessage16(hWnd, cmb1, CB_ADDSTRING16, 0,
				       (LPARAM)lpofn->lpstrFilter + n );
	  n += strlen(pstr) + 1;
	  pstr += strlen(pstr) + 1;
	  TRACE(commdlg,"add str='%s' "
			  "associated to '%s'\n", old_pstr, pstr);
	  SendDlgItemMessage16(hWnd, cmb1, CB_SETITEMDATA16, i, (LPARAM)pstr);
	  n += strlen(pstr) + 1;
	  pstr += strlen(pstr) + 1;
	}
  }
  /* set default filter */
  if (lpofn->nFilterIndex == 0 && lpofn->lpstrCustomFilter == (SEGPTR)NULL)
  	lpofn->nFilterIndex = 1;
  SendDlgItemMessage16(hWnd, cmb1, CB_SETCURSEL16, lpofn->nFilterIndex - 1, 0);    
  strncpy(tmpstr, FILEDLG_GetFileType(PTR_SEG_TO_LIN(lpofn->lpstrCustomFilter),
	     PTR_SEG_TO_LIN(lpofn->lpstrFilter), lpofn->nFilterIndex - 1),511);
  tmpstr[511]=0;
  TRACE(commdlg,"nFilterIndex = %ld, SetText of edt1 to '%s'\n", 
  			lpofn->nFilterIndex, tmpstr);
  SetDlgItemTextA( hWnd, edt1, tmpstr );
  /* get drive list */
  *tmpstr = 0;
  DlgDirListComboBoxA(hWnd, tmpstr, cmb2, 0, 0xC000);
  /* read initial directory */
  if (PTR_SEG_TO_LIN(lpofn->lpstrInitialDir) != NULL) 
    {
      strncpy(tmpstr, PTR_SEG_TO_LIN(lpofn->lpstrInitialDir), 510);
      tmpstr[510]=0;
      if (strlen(tmpstr) > 0 && tmpstr[strlen(tmpstr)-1] != '\\' 
	  && tmpstr[strlen(tmpstr)-1] != ':')
	strcat(tmpstr,"\\");
    }
  else
    *tmpstr = 0;
  if (!FILEDLG_ScanDir(hWnd, tmpstr)) {
    *tmpstr = 0;
    if (!FILEDLG_ScanDir(hWnd, tmpstr))
      WARN(commdlg, "Couldn't read initial directory %s!\n",tmpstr);
  }
  /* select current drive in combo 2, omit missing drives */
  for(i=0, n=-1; i<=DRIVE_GetCurrentDrive(); i++)
    if (DRIVE_IsValid(i))                  n++;
  SendDlgItemMessage16(hWnd, cmb2, CB_SETCURSEL16, n, 0);
  if (!(lpofn->Flags & OFN_SHOWHELP))
    ShowWindow(GetDlgItem(hWnd, pshHelp), SW_HIDE);
  if (lpofn->Flags & OFN_HIDEREADONLY)
    ShowWindow(GetDlgItem(hWnd, chx1), SW_HIDE); 
  if (FILEDLG_HookCallChk(lpofn))
     return (BOOL16)FILEDLG_CallWindowProc(lpofn,hWnd,WM_INITDIALOG,wParam,lParam );
  else  
     return TRUE;
}

/***********************************************************************
 *                              FILEDLG_WMCommand               [internal]
 */
BOOL in_update=FALSE;

static LRESULT FILEDLG_WMCommand(HWND16 hWnd, WPARAM16 wParam, LPARAM lParam) 
{
  LONG lRet;
  LPOPENFILENAME16 lpofn;
  OPENFILENAME16 ofn2;
  char tmpstr[512], tmpstr2[512];
  LPSTR pstr, pstr2;
  UINT16 control,notification;

  /* Notifications are packaged differently in Win32 */
  control = wParam;
  notification = HIWORD(lParam);
    
  lpofn = (LPOPENFILENAME16)PTR_SEG_TO_LIN(GetWindowLongA(hWnd, DWL_USER));
  switch (control)
    {
    case lst1: /* file list */
      FILEDLG_StripEditControl(hWnd);
      if (notification == LBN_DBLCLK)
	goto almost_ok;
      lRet = SendDlgItemMessage16(hWnd, lst1, LB_GETCURSEL16, 0, 0);
      if (lRet == LB_ERR) return TRUE;
      if ((pstr = SEGPTR_ALLOC(512)))
      {
          SendDlgItemMessage16(hWnd, lst1, LB_GETTEXT16, lRet,
                               (LPARAM)SEGPTR_GET(pstr));
          SetDlgItemTextA( hWnd, edt1, pstr );
          SEGPTR_FREE(pstr);
      }
      if (FILEDLG_HookCallChk(lpofn))
       FILEDLG_CallWindowProc(lpofn,hWnd,
                  RegisterWindowMessageA( LBSELCHSTRING ),
                  control, MAKELONG(lRet,CD_LBSELCHANGE));       
      /* FIXME: for OFN_ALLOWMULTISELECT we need CD_LBSELSUB, CD_SELADD, CD_LBSELNOITEMS */                  
      return TRUE;
    case lst2: /* directory list */
      FILEDLG_StripEditControl(hWnd);
      if (notification == LBN_DBLCLK)
	{
	  lRet = SendDlgItemMessage16(hWnd, lst2, LB_GETCURSEL16, 0, 0);
	  if (lRet == LB_ERR) return TRUE;
          pstr = SEGPTR_ALLOC(512);
	  SendDlgItemMessage16(hWnd, lst2, LB_GETTEXT16, lRet,
			     (LPARAM)SEGPTR_GET(pstr));
          strcpy( tmpstr, pstr );
          SEGPTR_FREE(pstr);
	  if (tmpstr[0] == '[')
	    {
	      tmpstr[strlen(tmpstr) - 1] = 0;
	      strcpy(tmpstr,tmpstr+1);
	    }
	  strcat(tmpstr, "\\");
	  goto reset_scan;
	}
      return TRUE;
    case cmb1: /* file type drop list */
      if (notification == CBN_SELCHANGE) 
	{
	  *tmpstr = 0;
	  goto reset_scan;
	}
      return TRUE;
    case chx1:
      return TRUE;
    case pshHelp:
      return TRUE;
    case cmb2: /* disk drop list */
      FILEDLG_StripEditControl(hWnd);
      lRet = SendDlgItemMessage16(hWnd, cmb2, CB_GETCURSEL16, 0, 0L);
      if (lRet == LB_ERR) return 0;
      pstr = SEGPTR_ALLOC(512);
      SendDlgItemMessage16(hWnd, cmb2, CB_GETLBTEXT16, lRet,
                           (LPARAM)SEGPTR_GET(pstr));
      sprintf(tmpstr, "%c:", pstr[2]);
      SEGPTR_FREE(pstr);
    reset_scan:
      lRet = SendDlgItemMessage16(hWnd, cmb1, CB_GETCURSEL16, 0, 0);
      if (lRet == LB_ERR)
	return TRUE;
      pstr = (LPSTR)SendDlgItemMessage16(hWnd, cmb1, CB_GETITEMDATA16, lRet, 0);
      TRACE(commdlg,"Selected filter : %s\n", pstr);
      SetDlgItemTextA( hWnd, edt1, pstr );
      FILEDLG_ScanDir(hWnd, tmpstr);
      in_update=TRUE;
    case IDOK:
    almost_ok:
      ofn2=*lpofn; /* for later restoring */
      GetDlgItemTextA( hWnd, edt1, tmpstr, sizeof(tmpstr) );
      pstr = strrchr(tmpstr, '\\');
      if (pstr == NULL)
	pstr = strrchr(tmpstr, ':');
      if (strchr(tmpstr,'*') != NULL || strchr(tmpstr,'?') != NULL)
	{
	  /* edit control contains wildcards */
	  if (pstr != NULL)
	    {
	      strncpy(tmpstr2, pstr+1, 511); tmpstr2[511]=0;
	      *(pstr+1) = 0;
	    }
	  else
	    {
	      strcpy(tmpstr2, tmpstr);
	      *tmpstr=0;
	    }
	  TRACE(commdlg,"tmpstr=%s, tmpstr2=%s\n", tmpstr, tmpstr2);
          SetDlgItemTextA( hWnd, edt1, tmpstr2 );
	  FILEDLG_ScanDir(hWnd, tmpstr);
	  return TRUE;
	}
      /* no wildcards, we might have a directory or a filename */
      /* try appending a wildcard and reading the directory */
      pstr2 = tmpstr + strlen(tmpstr);
      if (pstr == NULL || *(pstr+1) != 0)
	strcat(tmpstr, "\\");
      lRet = SendDlgItemMessage16(hWnd, cmb1, CB_GETCURSEL16, 0, 0);
      if (lRet == LB_ERR) return TRUE;
      lpofn->nFilterIndex = lRet + 1;
      TRACE(commdlg,"lpofn->nFilterIndex=%ld\n", lpofn->nFilterIndex);
      lstrcpynA(tmpstr2, 
	     FILEDLG_GetFileType(PTR_SEG_TO_LIN(lpofn->lpstrCustomFilter),
				 PTR_SEG_TO_LIN(lpofn->lpstrFilter),
				 lRet), sizeof(tmpstr2));
      SetDlgItemTextA( hWnd, edt1, tmpstr2 );
      if (!in_update)
      /* if ScanDir succeeds, we have changed the directory */
      if (FILEDLG_ScanDir(hWnd, tmpstr)) return TRUE;
      /* if not, this must be a filename */
      *pstr2 = 0;
      if (pstr != NULL)
	{
	  /* strip off the pathname */
	  *pstr = 0;
          SetDlgItemTextA( hWnd, edt1, pstr + 1 );
	  lstrcpynA(tmpstr2, pstr+1, sizeof(tmpstr2) );
	  /* Should we MessageBox() if this fails? */
	  if (!FILEDLG_ScanDir(hWnd, tmpstr)) return TRUE;
	  strcpy(tmpstr, tmpstr2);
	}
      else SetDlgItemTextA( hWnd, edt1, tmpstr );
#if 0
      ShowWindow16(hWnd, SW_HIDE);   /* this should not be necessary ?! (%%%) */
#endif
      {
	int drive = DRIVE_GetCurrentDrive();
	tmpstr2[0] = 'A'+ drive;
	tmpstr2[1] = ':';
	tmpstr2[2] = '\\';
	strncpy(tmpstr2 + 3, DRIVE_GetDosCwd(drive), 507); tmpstr2[510]=0;
	if (strlen(tmpstr2) > 3)
	   strcat(tmpstr2, "\\");
	strncat(tmpstr2, tmpstr, 511-strlen(tmpstr2)); tmpstr2[511]=0;
	if (lpofn->lpstrFile)
	  lstrcpynA(PTR_SEG_TO_LIN(lpofn->lpstrFile), tmpstr2,lpofn->nMaxFile);
      }
      lpofn->nFileOffset = strrchr(tmpstr2,'\\') - tmpstr2 +1;
      lpofn->nFileExtension = 0;
      while(tmpstr2[lpofn->nFileExtension] != '.' && tmpstr2[lpofn->nFileExtension] != '\0')
        lpofn->nFileExtension++;
      if (tmpstr2[lpofn->nFileExtension] == '\0')
	lpofn->nFileExtension = 0;
      else
	lpofn->nFileExtension++;

      if(in_update)
       {
         if (FILEDLG_HookCallChk(lpofn))
           FILEDLG_CallWindowProc(lpofn,hWnd,
                                  RegisterWindowMessageA( LBSELCHSTRING ),
                                  control, MAKELONG(lRet,CD_LBSELCHANGE));

         in_update = FALSE;
         return TRUE;
       }
      if (PTR_SEG_TO_LIN(lpofn->lpstrFileTitle) != NULL) 
	{
	  lRet = SendDlgItemMessage16(hWnd, lst1, LB_GETCURSEL16, 0, 0);
	  SendDlgItemMessage16(hWnd, lst1, LB_GETTEXT16, lRet,
                               lpofn->lpstrFileTitle );
	}
      if (FILEDLG_HookCallChk(lpofn))
      {
       lRet= (BOOL16)FILEDLG_CallWindowProc(lpofn,
               hWnd, RegisterWindowMessageA( FILEOKSTRING ), 0, lParam );
       if (lRet)       
       {
         *lpofn=ofn2; /* restore old state */
#if 0
         ShowWindow16(hWnd, SW_SHOW);               /* only if above (%%%) SW_HIDE used */
#endif         
         break;
       }
      }
      EndDialog(hWnd, TRUE);
      return TRUE;
    case IDCANCEL:
      EndDialog(hWnd, FALSE);
      return TRUE;
    }
  return FALSE;
}


/***********************************************************************
 *           FileOpenDlgProc   (COMMDLG.6)
 */
LRESULT WINAPI FileOpenDlgProc16(HWND16 hWnd, UINT16 wMsg, WPARAM16 wParam,
                               LPARAM lParam)
{  
 LPOPENFILENAME16 lpofn = (LPOPENFILENAME16)PTR_SEG_TO_LIN(GetWindowLongA(hWnd, DWL_USER));
 
 if (wMsg!=WM_INITDIALOG)
  if (FILEDLG_HookCallChk(lpofn))
  {
   LRESULT  lRet=(BOOL16)FILEDLG_CallWindowProc(lpofn,hWnd,wMsg,wParam,lParam);
   if (lRet)   
    return lRet;         /* else continue message processing */
  }             
  switch (wMsg)
    {
    case WM_INITDIALOG:
      return FILEDLG_WMInitDialog(hWnd, wParam, lParam);
    case WM_MEASUREITEM:
      return FILEDLG_WMMeasureItem(hWnd, wParam, lParam);
    case WM_DRAWITEM:
      return FILEDLG_WMDrawItem(hWnd, wParam, lParam, FALSE);
    case WM_COMMAND:
      return FILEDLG_WMCommand(hWnd, wParam, lParam);
#if 0
    case WM_CTLCOLOR:
      SetBkColor((HDC16)wParam, 0x00C0C0C0);
      switch (HIWORD(lParam))
	{
	case CTLCOLOR_BTN:
	  SetTextColor((HDC16)wParam, 0x00000000);
	  return hGRAYBrush;
	case CTLCOLOR_STATIC:
	  SetTextColor((HDC16)wParam, 0x00000000);
	  return hGRAYBrush;
	}
      break;
#endif
    }
  return FALSE;
}


/***********************************************************************
 *           FileSaveDlgProc   (COMMDLG.7)
 */
LRESULT WINAPI FileSaveDlgProc16(HWND16 hWnd, UINT16 wMsg, WPARAM16 wParam,
                               LPARAM lParam)
{
 LPOPENFILENAME16 lpofn = (LPOPENFILENAME16)PTR_SEG_TO_LIN(GetWindowLongA(hWnd, DWL_USER));
 
 if (wMsg!=WM_INITDIALOG)
  if (FILEDLG_HookCallChk(lpofn))
  {
   LRESULT  lRet=(BOOL16)FILEDLG_CallWindowProc(lpofn,hWnd,wMsg,wParam,lParam);
   if (lRet)   
    return lRet;         /* else continue message processing */
  }             
  switch (wMsg) {
   case WM_INITDIALOG:
      return FILEDLG_WMInitDialog(hWnd, wParam, lParam);
      
   case WM_MEASUREITEM:
      return FILEDLG_WMMeasureItem(hWnd, wParam, lParam);
    
   case WM_DRAWITEM:
      return FILEDLG_WMDrawItem(hWnd, wParam, lParam, TRUE);

   case WM_COMMAND:
      return FILEDLG_WMCommand(hWnd, wParam, lParam);
  }
  
  /*
  case WM_CTLCOLOR:
   SetBkColor((HDC16)wParam, 0x00C0C0C0);
   switch (HIWORD(lParam))
   {
    case CTLCOLOR_BTN:
     SetTextColor((HDC16)wParam, 0x00000000);
     return hGRAYBrush;
    case CTLCOLOR_STATIC:
     SetTextColor((HDC16)wParam, 0x00000000);
     return hGRAYBrush;
   }
   return FALSE;
   
   */
  return FALSE;
}


/***********************************************************************
 *           FindText16   (COMMDLG.11)
 */
HWND16 WINAPI FindText16( SEGPTR find )
{
    HANDLE16 hInst;
    LPCVOID ptr;
    LPFINDREPLACE16 lpFind = (LPFINDREPLACE16)PTR_SEG_TO_LIN(find);

    /*
     * FIXME : Should respond to FR_ENABLETEMPLATE and FR_ENABLEHOOK here
     * For now, only the standard dialog works.
     */
    if (lpFind->Flags & (FR_ENABLETEMPLATE | FR_ENABLETEMPLATEHANDLE | 
	FR_ENABLEHOOK)) FIXME(commdlg, ": unimplemented flag (ignored)\n");     
    ptr = SYSRES_GetResPtr( SYSRES_DIALOG_FIND_TEXT );
    hInst = WIN_GetWindowInstance( lpFind->hwndOwner );
    return DIALOG_CreateIndirect( hInst, ptr, TRUE, lpFind->hwndOwner,
                        (DLGPROC16)MODULE_GetWndProcEntry16("FindTextDlgProc"),
                                  find, WIN_PROC_16 );
}

/***********************************************************************
 *           FindText32A   (COMMDLG.6)
 */
HWND WINAPI FindTextA( LPFINDREPLACEA lpFind )
{
    HANDLE16 hInst;
    LPCVOID ptr;

    /*
     * FIXME : Should respond to FR_ENABLETEMPLATE and FR_ENABLEHOOK here
     * For now, only the standard dialog works.
     */
    if (lpFind->Flags & (FR_ENABLETEMPLATE | FR_ENABLETEMPLATEHANDLE | 
	FR_ENABLEHOOK)) FIXME(commdlg, ": unimplemented flag (ignored)\n");     
    ptr = SYSRES_GetResPtr( SYSRES_DIALOG_FIND_TEXT );
    hInst = WIN_GetWindowInstance( lpFind->hwndOwner );
    return DIALOG_CreateIndirect( hInst, ptr, TRUE, lpFind->hwndOwner,
                (DLGPROC16)FindTextDlgProcA, (LPARAM)lpFind, WIN_PROC_32A );
}

/***********************************************************************
 *           FindText32W   (COMMDLG.7)
 */
HWND WINAPI FindTextW( LPFINDREPLACEW lpFind )
{
    HANDLE16 hInst;
    LPCVOID ptr;

    /*
     * FIXME : Should respond to FR_ENABLETEMPLATE and FR_ENABLEHOOK here
     * For now, only the standard dialog works.
     */
    if (lpFind->Flags & (FR_ENABLETEMPLATE | FR_ENABLETEMPLATEHANDLE | 
	FR_ENABLEHOOK)) FIXME(commdlg, ": unimplemented flag (ignored)\n");     
    ptr = SYSRES_GetResPtr( SYSRES_DIALOG_FIND_TEXT );
    hInst = WIN_GetWindowInstance( lpFind->hwndOwner );
    return DIALOG_CreateIndirect( hInst, ptr, TRUE, lpFind->hwndOwner,
                (DLGPROC16)FindTextDlgProcW, (LPARAM)lpFind, WIN_PROC_32W );
}

/***********************************************************************
 *           ReplaceText16   (COMMDLG.12)
 */
HWND16 WINAPI ReplaceText16( SEGPTR find )
{
    HANDLE16 hInst;
    LPCVOID ptr;
    LPFINDREPLACE16 lpFind = (LPFINDREPLACE16)PTR_SEG_TO_LIN(find);

    /*
     * FIXME : We should do error checking on the lpFind structure here
     * and make CommDlgExtendedError() return the error condition.
     */
    if (lpFind->Flags & (FR_ENABLETEMPLATE | FR_ENABLETEMPLATEHANDLE | 
	FR_ENABLEHOOK)) FIXME(commdlg, ": unimplemented flag (ignored)\n");     
    ptr = SYSRES_GetResPtr( SYSRES_DIALOG_REPLACE_TEXT );
    hInst = WIN_GetWindowInstance( lpFind->hwndOwner );
    return DIALOG_CreateIndirect( hInst, ptr, TRUE, lpFind->hwndOwner,
                     (DLGPROC16)MODULE_GetWndProcEntry16("ReplaceTextDlgProc"),
                                  find, WIN_PROC_16 );
}

/***********************************************************************
 *           ReplaceText32A   (COMDLG32.19)
 */
HWND WINAPI ReplaceTextA( LPFINDREPLACEA lpFind )
{
    HANDLE16 hInst;
    LPCVOID ptr;

    /*
     * FIXME : Should respond to FR_ENABLETEMPLATE and FR_ENABLEHOOK here
     * For now, only the standard dialog works.
     */
    if (lpFind->Flags & (FR_ENABLETEMPLATE | FR_ENABLETEMPLATEHANDLE |
	FR_ENABLEHOOK)) FIXME(commdlg, ": unimplemented flag (ignored)\n");     
    ptr = SYSRES_GetResPtr( SYSRES_DIALOG_REPLACE_TEXT );
    hInst = WIN_GetWindowInstance( lpFind->hwndOwner );
    return DIALOG_CreateIndirect( hInst, ptr, TRUE, lpFind->hwndOwner,
		(DLGPROC16)ReplaceTextDlgProcA, (LPARAM)lpFind, WIN_PROC_32A );
}

/***********************************************************************
 *           ReplaceText32W   (COMDLG32.20)
 */
HWND WINAPI ReplaceTextW( LPFINDREPLACEW lpFind )
{
    HANDLE16 hInst;
    LPCVOID ptr;

    /*
     * FIXME : We should do error checking on the lpFind structure here
     * and make CommDlgExtendedError() return the error condition.
     */
    if (lpFind->Flags & (FR_ENABLETEMPLATE | FR_ENABLETEMPLATEHANDLE | 
	FR_ENABLEHOOK)) FIXME(commdlg, ": unimplemented flag (ignored)\n");
    ptr = SYSRES_GetResPtr( SYSRES_DIALOG_REPLACE_TEXT );
    hInst = WIN_GetWindowInstance( lpFind->hwndOwner );
    return DIALOG_CreateIndirect( hInst, ptr, TRUE, lpFind->hwndOwner,
		(DLGPROC16)ReplaceTextDlgProcW, (LPARAM)lpFind, WIN_PROC_32W );
}


/***********************************************************************
 *                              FINDDLG_WMInitDialog            [internal]
 */
static LRESULT FINDDLG_WMInitDialog(HWND hWnd, LPARAM lParam, LPDWORD lpFlags,
                                    LPSTR lpstrFindWhat, BOOL fUnicode)
{
    SetWindowLongA(hWnd, DWL_USER, lParam);
    *lpFlags &= ~(FR_FINDNEXT | FR_REPLACE | FR_REPLACEALL | FR_DIALOGTERM);
    /*
     * FIXME : If the initial FindWhat string is empty, we should disable the
     * FindNext (IDOK) button.  Only after typing some text, the button should be
     * enabled.
     */
    if (fUnicode) SetDlgItemTextW(hWnd, edt1, (LPWSTR)lpstrFindWhat);
	else SetDlgItemTextA(hWnd, edt1, lpstrFindWhat);
    CheckRadioButton(hWnd, rad1, rad2, (*lpFlags & FR_DOWN) ? rad2 : rad1);
    if (*lpFlags & (FR_HIDEUPDOWN | FR_NOUPDOWN)) {
	EnableWindow(GetDlgItem(hWnd, rad1), FALSE);
	EnableWindow(GetDlgItem(hWnd, rad2), FALSE);
    }
    if (*lpFlags & FR_HIDEUPDOWN) {
	ShowWindow(GetDlgItem(hWnd, rad1), SW_HIDE);
	ShowWindow(GetDlgItem(hWnd, rad2), SW_HIDE);
	ShowWindow(GetDlgItem(hWnd, grp1), SW_HIDE);
    }
    CheckDlgButton(hWnd, chx1, (*lpFlags & FR_WHOLEWORD) ? 1 : 0);
    if (*lpFlags & (FR_HIDEWHOLEWORD | FR_NOWHOLEWORD))
	EnableWindow(GetDlgItem(hWnd, chx1), FALSE);
    if (*lpFlags & FR_HIDEWHOLEWORD)
	ShowWindow(GetDlgItem(hWnd, chx1), SW_HIDE);
    CheckDlgButton(hWnd, chx2, (*lpFlags & FR_MATCHCASE) ? 1 : 0);
    if (*lpFlags & (FR_HIDEMATCHCASE | FR_NOMATCHCASE))
	EnableWindow(GetDlgItem(hWnd, chx2), FALSE);
    if (*lpFlags & FR_HIDEMATCHCASE)
	ShowWindow(GetDlgItem(hWnd, chx2), SW_HIDE);
    if (!(*lpFlags & FR_SHOWHELP)) {
	EnableWindow(GetDlgItem(hWnd, pshHelp), FALSE);
	ShowWindow(GetDlgItem(hWnd, pshHelp), SW_HIDE);
    }
    ShowWindow(hWnd, SW_SHOWNORMAL);
    return TRUE;
}    


/***********************************************************************
 *                              FINDDLG_WMCommand               [internal]
 */
static LRESULT FINDDLG_WMCommand(HWND hWnd, WPARAM wParam, 
			HWND hwndOwner, LPDWORD lpFlags,
			LPSTR lpstrFindWhat, WORD wFindWhatLen, 
			BOOL fUnicode)
{
    int uFindReplaceMessage = RegisterWindowMessageA( FINDMSGSTRING );
    int uHelpMessage = RegisterWindowMessageA( HELPMSGSTRING );

    switch (wParam) {
	case IDOK:
	    if (fUnicode) 
	      GetDlgItemTextW(hWnd, edt1, (LPWSTR)lpstrFindWhat, wFindWhatLen/2);
	      else GetDlgItemTextA(hWnd, edt1, lpstrFindWhat, wFindWhatLen);
	    if (IsDlgButtonChecked(hWnd, rad2))
		*lpFlags |= FR_DOWN;
		else *lpFlags &= ~FR_DOWN;
	    if (IsDlgButtonChecked(hWnd, chx1))
		*lpFlags |= FR_WHOLEWORD; 
		else *lpFlags &= ~FR_WHOLEWORD;
	    if (IsDlgButtonChecked(hWnd, chx2))
		*lpFlags |= FR_MATCHCASE; 
		else *lpFlags &= ~FR_MATCHCASE;
            *lpFlags &= ~(FR_REPLACE | FR_REPLACEALL | FR_DIALOGTERM);
	    *lpFlags |= FR_FINDNEXT;
	    SendMessageA(hwndOwner, uFindReplaceMessage, 0,
                          GetWindowLongA(hWnd, DWL_USER) );
	    return TRUE;
	case IDCANCEL:
            *lpFlags &= ~(FR_FINDNEXT | FR_REPLACE | FR_REPLACEALL);
	    *lpFlags |= FR_DIALOGTERM;
	    SendMessageA(hwndOwner, uFindReplaceMessage, 0,
                          GetWindowLongA(hWnd, DWL_USER) );
	    DestroyWindow(hWnd);
	    return TRUE;
	case pshHelp:
	    /* FIXME : should lpfr structure be passed as an argument ??? */
	    SendMessageA(hwndOwner, uHelpMessage, 0, 0);
	    return TRUE;
    }
    return FALSE;
}    


/***********************************************************************
 *           FindTextDlgProc16   (COMMDLG.13)
 */
LRESULT WINAPI FindTextDlgProc16(HWND16 hWnd, UINT16 wMsg, WPARAM16 wParam,
                                 LPARAM lParam)
{
    LPFINDREPLACE16 lpfr;
    switch (wMsg) {
	case WM_INITDIALOG:
            lpfr=(LPFINDREPLACE16)PTR_SEG_TO_LIN(lParam);
	    return FINDDLG_WMInitDialog(hWnd, lParam, &(lpfr->Flags),
		PTR_SEG_TO_LIN(lpfr->lpstrFindWhat), FALSE);
	case WM_COMMAND:
	    lpfr=(LPFINDREPLACE16)PTR_SEG_TO_LIN(GetWindowLongA(hWnd, DWL_USER));
	    return FINDDLG_WMCommand(hWnd, wParam, lpfr->hwndOwner,
		&lpfr->Flags, PTR_SEG_TO_LIN(lpfr->lpstrFindWhat),
		lpfr->wFindWhatLen, FALSE);
    }
    return FALSE;
}

/***********************************************************************
 *           FindTextDlgProc32A
 */
LRESULT WINAPI FindTextDlgProcA(HWND hWnd, UINT wMsg, WPARAM wParam,
                                 LPARAM lParam)
{
    LPFINDREPLACEA lpfr;
    switch (wMsg) {
	case WM_INITDIALOG:
	    lpfr=(LPFINDREPLACEA)lParam;
	    return FINDDLG_WMInitDialog(hWnd, lParam, &(lpfr->Flags),
	      lpfr->lpstrFindWhat, FALSE);
	case WM_COMMAND:
	    lpfr=(LPFINDREPLACEA)GetWindowLongA(hWnd, DWL_USER);
	    return FINDDLG_WMCommand(hWnd, wParam, lpfr->hwndOwner,
		&lpfr->Flags, lpfr->lpstrFindWhat, lpfr->wFindWhatLen,
		FALSE);
    }
    return FALSE;
}

/***********************************************************************
 *           FindTextDlgProc32W
 */
LRESULT WINAPI FindTextDlgProcW(HWND hWnd, UINT wMsg, WPARAM wParam,
                                 LPARAM lParam)
{
    LPFINDREPLACEW lpfr;
    switch (wMsg) {
	case WM_INITDIALOG:
	    lpfr=(LPFINDREPLACEW)lParam;
	    return FINDDLG_WMInitDialog(hWnd, lParam, &(lpfr->Flags),
	      (LPSTR)lpfr->lpstrFindWhat, TRUE);
	case WM_COMMAND:
	    lpfr=(LPFINDREPLACEW)GetWindowLongA(hWnd, DWL_USER);
	    return FINDDLG_WMCommand(hWnd, wParam, lpfr->hwndOwner,
		&lpfr->Flags, (LPSTR)lpfr->lpstrFindWhat, lpfr->wFindWhatLen,
		TRUE);
    }
    return FALSE;
}


/***********************************************************************
 *                              REPLACEDLG_WMInitDialog         [internal]
 */
static LRESULT REPLACEDLG_WMInitDialog(HWND hWnd, LPARAM lParam,
		    LPDWORD lpFlags, LPSTR lpstrFindWhat, 
		    LPSTR lpstrReplaceWith, BOOL fUnicode)
{
    SetWindowLongA(hWnd, DWL_USER, lParam);
    *lpFlags &= ~(FR_FINDNEXT | FR_REPLACE | FR_REPLACEALL | FR_DIALOGTERM);
    /*
     * FIXME : If the initial FindWhat string is empty, we should disable the FinNext /
     * Replace / ReplaceAll buttons.  Only after typing some text, the buttons should be
     * enabled.
     */
    if (fUnicode)     
    {
	SetDlgItemTextW(hWnd, edt1, (LPWSTR)lpstrFindWhat);
	SetDlgItemTextW(hWnd, edt2, (LPWSTR)lpstrReplaceWith);
    } else
    {
	SetDlgItemTextA(hWnd, edt1, lpstrFindWhat);
	SetDlgItemTextA(hWnd, edt2, lpstrReplaceWith);
    }
    CheckDlgButton(hWnd, chx1, (*lpFlags & FR_WHOLEWORD) ? 1 : 0);
    if (*lpFlags & (FR_HIDEWHOLEWORD | FR_NOWHOLEWORD))
	EnableWindow(GetDlgItem(hWnd, chx1), FALSE);
    if (*lpFlags & FR_HIDEWHOLEWORD)
	ShowWindow(GetDlgItem(hWnd, chx1), SW_HIDE);
    CheckDlgButton(hWnd, chx2, (*lpFlags & FR_MATCHCASE) ? 1 : 0);
    if (*lpFlags & (FR_HIDEMATCHCASE | FR_NOMATCHCASE))
	EnableWindow(GetDlgItem(hWnd, chx2), FALSE);
    if (*lpFlags & FR_HIDEMATCHCASE)
	ShowWindow(GetDlgItem(hWnd, chx2), SW_HIDE);
    if (!(*lpFlags & FR_SHOWHELP)) {
	EnableWindow(GetDlgItem(hWnd, pshHelp), FALSE);
	ShowWindow(GetDlgItem(hWnd, pshHelp), SW_HIDE);
    }
    ShowWindow(hWnd, SW_SHOWNORMAL);
    return TRUE;
}    


/***********************************************************************
 *                              REPLACEDLG_WMCommand            [internal]
 */
static LRESULT REPLACEDLG_WMCommand(HWND hWnd, WPARAM16 wParam,
		    HWND hwndOwner, LPDWORD lpFlags,
		    LPSTR lpstrFindWhat, WORD wFindWhatLen,
		    LPSTR lpstrReplaceWith, WORD wReplaceWithLen,
		    BOOL fUnicode)
{
    int uFindReplaceMessage = RegisterWindowMessageA( FINDMSGSTRING );
    int uHelpMessage = RegisterWindowMessageA( HELPMSGSTRING );

    switch (wParam) {
	case IDOK:
	    if (fUnicode)
	    {
		GetDlgItemTextW(hWnd, edt1, (LPWSTR)lpstrFindWhat, wFindWhatLen/2);
		GetDlgItemTextW(hWnd, edt2, (LPWSTR)lpstrReplaceWith, wReplaceWithLen/2);
	    }  else
	    {
		GetDlgItemTextA(hWnd, edt1, lpstrFindWhat, wFindWhatLen);
		GetDlgItemTextA(hWnd, edt2, lpstrReplaceWith, wReplaceWithLen);
	    }
	    if (IsDlgButtonChecked(hWnd, chx1))
		*lpFlags |= FR_WHOLEWORD; 
		else *lpFlags &= ~FR_WHOLEWORD;
	    if (IsDlgButtonChecked(hWnd, chx2))
		*lpFlags |= FR_MATCHCASE; 
		else *lpFlags &= ~FR_MATCHCASE;
            *lpFlags &= ~(FR_REPLACE | FR_REPLACEALL | FR_DIALOGTERM);
	    *lpFlags |= FR_FINDNEXT;
	    SendMessageA(hwndOwner, uFindReplaceMessage, 0,
                          GetWindowLongA(hWnd, DWL_USER) );
	    return TRUE;
	case IDCANCEL:
            *lpFlags &= ~(FR_FINDNEXT | FR_REPLACE | FR_REPLACEALL);
	    *lpFlags |= FR_DIALOGTERM;
	    SendMessageA(hwndOwner, uFindReplaceMessage, 0,
                          GetWindowLongA(hWnd, DWL_USER) );
	    DestroyWindow(hWnd);
	    return TRUE;
	case psh1:
	    if (fUnicode)
	    {
		GetDlgItemTextW(hWnd, edt1, (LPWSTR)lpstrFindWhat, wFindWhatLen/2);
		GetDlgItemTextW(hWnd, edt2, (LPWSTR)lpstrReplaceWith, wReplaceWithLen/2);
	    }  else
	    {	
		GetDlgItemTextA(hWnd, edt1, lpstrFindWhat, wFindWhatLen);
		GetDlgItemTextA(hWnd, edt2, lpstrReplaceWith, wReplaceWithLen);
	    }
	    if (IsDlgButtonChecked(hWnd, chx1))
		*lpFlags |= FR_WHOLEWORD; 
		else *lpFlags &= ~FR_WHOLEWORD;
	    if (IsDlgButtonChecked(hWnd, chx2))
		*lpFlags |= FR_MATCHCASE; 
		else *lpFlags &= ~FR_MATCHCASE;
            *lpFlags &= ~(FR_FINDNEXT | FR_REPLACEALL | FR_DIALOGTERM);
	    *lpFlags |= FR_REPLACE;
	    SendMessageA(hwndOwner, uFindReplaceMessage, 0,
                          GetWindowLongA(hWnd, DWL_USER) );
	    return TRUE;
	case psh2:
	    if (fUnicode)
	    {
		GetDlgItemTextW(hWnd, edt1, (LPWSTR)lpstrFindWhat, wFindWhatLen/2);
		GetDlgItemTextW(hWnd, edt2, (LPWSTR)lpstrReplaceWith, wReplaceWithLen/2);
	    }  else
	    {
		GetDlgItemTextA(hWnd, edt1, lpstrFindWhat, wFindWhatLen);
		GetDlgItemTextA(hWnd, edt2, lpstrReplaceWith, wReplaceWithLen);
	    }
	    if (IsDlgButtonChecked(hWnd, chx1))
		*lpFlags |= FR_WHOLEWORD; 
		else *lpFlags &= ~FR_WHOLEWORD;
	    if (IsDlgButtonChecked(hWnd, chx2))
		*lpFlags |= FR_MATCHCASE; 
		else *lpFlags &= ~FR_MATCHCASE;
            *lpFlags &= ~(FR_FINDNEXT | FR_REPLACE | FR_DIALOGTERM);
	    *lpFlags |= FR_REPLACEALL;
	    SendMessageA(hwndOwner, uFindReplaceMessage, 0,
                          GetWindowLongA(hWnd, DWL_USER) );
	    return TRUE;
	case pshHelp:
	    /* FIXME : should lpfr structure be passed as an argument ??? */
	    SendMessageA(hwndOwner, uHelpMessage, 0, 0);
	    return TRUE;
    }
    return FALSE;
}    


/***********************************************************************
 *           ReplaceTextDlgProc16   (COMMDLG.14)
 */
LRESULT WINAPI ReplaceTextDlgProc16(HWND16 hWnd, UINT16 wMsg, WPARAM16 wParam,
                                    LPARAM lParam)
{
    LPFINDREPLACE16 lpfr;
    switch (wMsg) {
	case WM_INITDIALOG:
            lpfr=(LPFINDREPLACE16)PTR_SEG_TO_LIN(lParam);
	    return REPLACEDLG_WMInitDialog(hWnd, lParam, &lpfr->Flags,
		    PTR_SEG_TO_LIN(lpfr->lpstrFindWhat),
		    PTR_SEG_TO_LIN(lpfr->lpstrReplaceWith), FALSE);
	case WM_COMMAND:
	    lpfr=(LPFINDREPLACE16)PTR_SEG_TO_LIN(GetWindowLongA(hWnd, DWL_USER));
	    return REPLACEDLG_WMCommand(hWnd, wParam, lpfr->hwndOwner, 
		    &lpfr->Flags, PTR_SEG_TO_LIN(lpfr->lpstrFindWhat),
		    lpfr->wFindWhatLen, PTR_SEG_TO_LIN(lpfr->lpstrReplaceWith),
		    lpfr->wReplaceWithLen, FALSE);
    }
    return FALSE;
}

/***********************************************************************
 *           ReplaceTextDlgProc32A
 */ 
LRESULT WINAPI ReplaceTextDlgProcA(HWND hWnd, UINT wMsg, WPARAM wParam,
                                    LPARAM lParam)
{
    LPFINDREPLACEA lpfr;
    switch (wMsg) {
	case WM_INITDIALOG:
            lpfr=(LPFINDREPLACEA)lParam;
	    return REPLACEDLG_WMInitDialog(hWnd, lParam, &lpfr->Flags,
		    lpfr->lpstrFindWhat, lpfr->lpstrReplaceWith, FALSE);
	case WM_COMMAND:
	    lpfr=(LPFINDREPLACEA)GetWindowLongA(hWnd, DWL_USER);
	    return REPLACEDLG_WMCommand(hWnd, wParam, lpfr->hwndOwner, 
		    &lpfr->Flags, lpfr->lpstrFindWhat, lpfr->wFindWhatLen,
		    lpfr->lpstrReplaceWith, lpfr->wReplaceWithLen, FALSE);
    }
    return FALSE;
}

/***********************************************************************
 *           ReplaceTextDlgProc32W
 */ 
LRESULT WINAPI ReplaceTextDlgProcW(HWND hWnd, UINT wMsg, WPARAM wParam,
                                    LPARAM lParam)
{
    LPFINDREPLACEW lpfr;
    switch (wMsg) {
	case WM_INITDIALOG:
            lpfr=(LPFINDREPLACEW)lParam;
	    return REPLACEDLG_WMInitDialog(hWnd, lParam, &lpfr->Flags,
		    (LPSTR)lpfr->lpstrFindWhat, (LPSTR)lpfr->lpstrReplaceWith,
		    TRUE);
	case WM_COMMAND:
	    lpfr=(LPFINDREPLACEW)GetWindowLongA(hWnd, DWL_USER);
	    return REPLACEDLG_WMCommand(hWnd, wParam, lpfr->hwndOwner, 
		    &lpfr->Flags, (LPSTR)lpfr->lpstrFindWhat, lpfr->wFindWhatLen,
		    (LPSTR)lpfr->lpstrReplaceWith, lpfr->wReplaceWithLen, TRUE);
    }
    return FALSE;
}



/***********************************************************************
 *           PrintDlg16   (COMMDLG.20)
 */
BOOL16 WINAPI PrintDlg16( SEGPTR printdlg )
{
    HANDLE16 hInst;
    BOOL16 bRet = FALSE;
    LPCVOID template;
    HWND hwndDialog;
    LPPRINTDLG16 lpPrint = (LPPRINTDLG16)PTR_SEG_TO_LIN(printdlg);

    TRACE(commdlg,"(%p) -- Flags=%08lX\n", lpPrint, lpPrint->Flags );

    if (lpPrint->Flags & PD_RETURNDEFAULT)
        /* FIXME: should fill lpPrint->hDevMode and lpPrint->hDevNames here */
        return TRUE;

    if (lpPrint->Flags & PD_PRINTSETUP)
	template = SYSRES_GetResPtr( SYSRES_DIALOG_PRINT_SETUP );
    else
	template = SYSRES_GetResPtr( SYSRES_DIALOG_PRINT );

    hInst = WIN_GetWindowInstance( lpPrint->hwndOwner );
    hwndDialog = DIALOG_CreateIndirect( hInst, template, TRUE,
                                        lpPrint->hwndOwner,
                               (DLGPROC16)((lpPrint->Flags & PD_PRINTSETUP) ?
                                MODULE_GetWndProcEntry16("PrintSetupDlgProc") :
                                MODULE_GetWndProcEntry16("PrintDlgProc")),
                                printdlg, WIN_PROC_16 );
    if (hwndDialog) bRet = DIALOG_DoDialogBox( hwndDialog, lpPrint->hwndOwner);
    return bRet;
}



/***********************************************************************
 *           PrintDlgA   (COMDLG32.17)
 *
 *  Displays the the PRINT dialog box, which enables the user to specify
 *  specific properties of the print job.
 *
 *  (Note: according to the MS Platform SDK, this call was in the past 
 *   also used to display some PRINT SETUP dialog. As this is superseded 
 *   by PageSetupDlg, this now results in an error!)
 *
 * RETURNS
 *  nonzero if the user pressed the OK button
 *  zero    if the user cancelled the window or an error occurred
 *
 * BUGS
 *  The function is a stub only, returning TRUE to allow more programs
 *  to function.
 */
BOOL WINAPI PrintDlgA(
			 LPPRINTDLGA lppd /* ptr to PRINTDLG32 struct */
			 )
{
/* My implementing strategy:
 * 
 * step 1: display the dialog and implement the layout-flags
 * step 2: enter valid information in the fields (e.g. real printers)
 * step 3: fix the RETURN-TRUE-ALWAYS Fixme by checking lppd->Flags for
 *         PD_RETURNDEFAULT
 * step 4: implement all other specs
 * step 5: allow customisation of the dialog box
 *
 * current implementation is in step 1.
     */ 

    HWND      hwndDialog;
    BOOL      bRet = FALSE;
    LPCVOID   ptr = SYSRES_GetResPtr( SYSRES_DIALOG_PRINT32 );  
  	HINSTANCE hInst = WIN_GetWindowInstance( lppd->hwndOwner );

    FIXME(commdlg, "KVG (%p): stub\n", lppd);

    /*
     * FIXME : Should respond to TEMPLATE and HOOK flags here
     * For now, only the standard dialog works.
     */
    if (lppd->Flags & (PD_ENABLEPRINTHOOK | PD_ENABLEPRINTTEMPLATE |
			  PD_ENABLEPRINTTEMPLATEHANDLE | PD_ENABLESETUPHOOK | 
			  PD_ENABLESETUPTEMPLATE|PD_ENABLESETUPTEMPLATEHANDLE)) 
    	FIXME(commdlg, ": unimplemented flag (ignored)\n");     
	
	/*
	 * if lppd->Flags PD_RETURNDEFAULT is specified, the PrintDlg function
	 * does not display the dialog box, but returns with valid entries
	 * for hDevMode and hDevNames .
	 * 
	 * Currently the flag is recognised, but we return empty hDevMode and
	 * and hDevNames. This will be fixed when I am in step 3.
	 */
	if (lppd->Flags & PD_RETURNDEFAULT)
	   {
	    WARN(commdlg, ": PrintDlg was requested to return printer info only."
					  "\n The return value currently does NOT provide these.\n");
		CommDlgLastError=PDERR_NODEVICES; /* return TRUE, thus never checked! */
	    return(TRUE);
	   }
	   
	if (lppd->Flags & PD_PRINTSETUP)
		{
		 FIXME(commdlg, ": PrintDlg was requested to display PrintSetup box.\n");
		 CommDlgLastError=PDERR_INITFAILURE; 
		 return(FALSE);
		}
		
    hwndDialog= DIALOG_CreateIndirect(hInst, ptr, TRUE, lppd->hwndOwner,
            (DLGPROC16)PrintDlgProcA, (LPARAM)lppd, WIN_PROC_32A );
    if (hwndDialog) 
        bRet = DIALOG_DoDialogBox(hwndDialog, lppd->hwndOwner);  
  return bRet;            
}





/***********************************************************************
 *           PrintDlg32W   (COMDLG32.18)
 */
BOOL WINAPI PrintDlgW( LPPRINTDLGW printdlg )
{
    FIXME(commdlg, "A really empty stub\n" );
    return FALSE;
}



/***********************************************************************
 *           PRINTDLG_WMInitDialog                      [internal]
 */
LRESULT PRINTDLG_WMInitDialog(HWND hDlg, WPARAM wParam, LPARAM lParam,
                         LPPRINTDLGA lppd)
{
	SetWindowLongA(hDlg, DWL_USER, lParam); 
	TRACE(commdlg,"WM_INITDIALOG lParam=%08lX\n", lParam);

	if (lppd->lStructSize != sizeof(PRINTDLGA))
	{
		FIXME(commdlg,"structure size failure !!!\n");
/*		EndDialog (hDlg, 0); 
		return FALSE; 
*/	}

/* Flag processing to set the according buttons on/off and
 * Initialise the various values
 */

    /* Print range (All/Range/Selection) */
    if (lppd->nMinPage == lppd->nMaxPage) 
    	lppd->Flags &= ~PD_NOPAGENUMS;        
    /* FIXME: I allow more freedom than either Win95 or WinNT,
     *        as officially we should return error if
     *        ToPage or FromPage is out-of-range
     */
    if (lppd->nToPage < lppd->nMinPage)
        lppd->nToPage = lppd->nMinPage;
    if (lppd->nToPage > lppd->nMaxPage)
        lppd->nToPage = lppd->nMaxPage;
    if (lppd->nFromPage < lppd->nMinPage)
        lppd->nFromPage = lppd->nMinPage;
    if (lppd->nFromPage > lppd->nMaxPage)
        lppd->nFromPage = lppd->nMaxPage;
    SetDlgItemInt(hDlg, edt2, lppd->nFromPage, FALSE);
    SetDlgItemInt(hDlg, edt3, lppd->nToPage, FALSE);
    CheckRadioButton(hDlg, rad1, rad3, rad1);
    if (lppd->Flags & PD_NOSELECTION)
		EnableWindow(GetDlgItem(hDlg, rad3), FALSE);
    else
		if (lppd->Flags & PD_SELECTION)
	    	CheckRadioButton(hDlg, rad1, rad3, rad3);
    if (lppd->Flags & PD_NOPAGENUMS)
       {
		EnableWindow(GetDlgItem(hDlg, rad2), FALSE);
		EnableWindow(GetDlgItem(hDlg, stc10),FALSE);
		EnableWindow(GetDlgItem(hDlg, edt2), FALSE);
		EnableWindow(GetDlgItem(hDlg, stc11),FALSE);
		EnableWindow(GetDlgItem(hDlg, edt3), FALSE);
       }
    else
       {
		if (lppd->Flags & PD_PAGENUMS)
	    	CheckRadioButton(hDlg, rad1, rad3, rad2);
       }
	/* FIXME: in Win95, the radiobutton "All" is displayed as
	 * "Print all xxx pages"... This is not done here (yet?)
	 */
        
	/* Collate pages */
    if (lppd->Flags & PD_COLLATE)
    	FIXME(commdlg, "PD_COLLATE not implemented yet\n");
        
    /* print to file */
   	CheckDlgButton(hDlg, chx1, (lppd->Flags & PD_PRINTTOFILE) ? 1 : 0);
    if (lppd->Flags & PD_DISABLEPRINTTOFILE)
		EnableWindow(GetDlgItem(hDlg, chx1), FALSE);    
    if (lppd->Flags & PD_HIDEPRINTTOFILE)
		ShowWindow(GetDlgItem(hDlg, chx1), SW_HIDE);
    
    /* status */

TRACE(commdlg, "succesful!\n");
  return TRUE;
}


/***********************************************************************
 *             PRINTDLG_ValidateAndDuplicateSettings          [internal]
 */
BOOL PRINTDLG_ValidateAndDuplicateSettings(HWND hDlg, LPPRINTDLGA lppd)
{
 WORD nToPage;
 WORD nFromPage;
 char TempBuffer[256];
 
    /* check whether nFromPage and nToPage are within range defined by
     * nMinPage and nMaxPage
     */
    /* FIXMNo checking on rad2 is performed now, because IsDlgButtonCheck
     * currently doesn't seem to know a state BST_CHECKED
     */
/*	if (IsDlgButtonChecked(hDlg, rad2) == BST_CHECKED) */
    {
    	nFromPage = GetDlgItemInt(hDlg, edt2, NULL, FALSE);
        nToPage   = GetDlgItemInt(hDlg, edt3, NULL, FALSE);
        if (nFromPage < lppd->nMinPage || nFromPage > lppd->nMaxPage ||
            nToPage < lppd->nMinPage || nToPage > lppd->nMaxPage)
        {
         FIXME(commdlg, "The MessageBox is not internationalised.");
         sprintf(TempBuffer, "This value lies not within Page range\n"
                             "Please enter a value between %d and %d",
                             lppd->nMinPage, lppd->nMaxPage);
         MessageBoxA(hDlg, TempBuffer, "Print", MB_OK | MB_ICONWARNING);
         return(FALSE);
        }
        lppd->nFromPage = nFromPage;
        lppd->nToPage   = nToPage;
    }
     
 return(TRUE);   
}



/***********************************************************************
 *                              PRINTDLG_WMCommand               [internal]
 */
static LRESULT PRINTDLG_WMCommand(HWND hDlg, WPARAM wParam, 
			LPARAM lParam, LPPRINTDLGA lppd)
{
    switch (wParam) 
    {
	 case IDOK:
        if (PRINTDLG_ValidateAndDuplicateSettings(hDlg, lppd) != TRUE)
        	return(FALSE);
        MessageBoxA(hDlg, "OK was hit!", NULL, MB_OK);
	    DestroyWindow(hDlg);
	    return(TRUE);
	 case IDCANCEL:
        MessageBoxA(hDlg, "CANCEL was hit!", NULL, MB_OK);
        EndDialog(hDlg, FALSE);
/*	    DestroyWindow(hDlg); */
	    return(FALSE);
    }
    return FALSE;
}    



/***********************************************************************
 *           PrintDlgProcA			[internal]
 */
LRESULT WINAPI PrintDlgProcA(HWND hDlg, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam)
{
  LPPRINTDLGA lppd;
  LRESULT res=FALSE;
  if (uMsg!=WM_INITDIALOG)
  {
   lppd=(LPPRINTDLGA)GetWindowLongA(hDlg, DWL_USER);   
   if (!lppd)
    return FALSE;
}
  else
  {
    lppd=(LPPRINTDLGA)lParam;
    if (!PRINTDLG_WMInitDialog(hDlg, wParam, lParam, lppd)) 
    {
      TRACE(commdlg, "PRINTDLG_WMInitDialog returned FALSE\n");
      return FALSE;
    }  
    MessageBoxA(hDlg,"Warning: this dialog has no functionality yet!", 
    			NULL, MB_OK);
  }
  switch (uMsg)
  {
  	case WM_COMMAND:
		return PRINTDLG_WMCommand(hDlg, wParam, lParam, lppd);
    case WM_DESTROY:
	    return FALSE;
  }
  
 return res;
}







/***********************************************************************
 *           PrintDlgProc16   (COMMDLG.21)
 */
LRESULT WINAPI PrintDlgProc16(HWND16 hWnd, UINT16 wMsg, WPARAM16 wParam,
                            LPARAM lParam)
{
  switch (wMsg)
    {
    case WM_INITDIALOG:
      TRACE(commdlg,"WM_INITDIALOG lParam=%08lX\n", lParam);
      ShowWindow16(hWnd, SW_SHOWNORMAL);
      return (TRUE);
    case WM_COMMAND:
      switch (wParam)
	{
	case IDOK:
	  EndDialog(hWnd, TRUE);
	  return(TRUE);
	case IDCANCEL:
	  EndDialog(hWnd, FALSE);
	  return(TRUE);
	}
      return(FALSE);
    }
  return FALSE;
}


/***********************************************************************
 *           PrintSetupDlgProc   (COMMDLG.22)
 */
LRESULT WINAPI PrintSetupDlgProc16(HWND16 hWnd, UINT16 wMsg, WPARAM16 wParam,
                                 LPARAM lParam)
{
  switch (wMsg)
    {
    case WM_INITDIALOG:
      TRACE(commdlg,"WM_INITDIALOG lParam=%08lX\n", lParam);
      ShowWindow16(hWnd, SW_SHOWNORMAL);
      return (TRUE);
    case WM_COMMAND:
      switch (wParam) {
      case IDOK:
	EndDialog(hWnd, TRUE);
	return(TRUE);
      case IDCANCEL:
	EndDialog(hWnd, FALSE);
	return(TRUE);
      }
      return(FALSE);
    }
  return FALSE;
}



/***********************************************************************
 *           CommDlgExtendedError   (COMMDLG.26)
 */
DWORD WINAPI CommDlgExtendedError(void)
{
  return CommDlgLastError;
}

/***********************************************************************
 *           GetFileTitleA   (COMDLG32.8)
 */
short WINAPI GetFileTitleA(LPCSTR lpFile, LPSTR lpTitle, UINT cbBuf)
{
    int i, len;
    TRACE(commdlg,"(%p %p %d); \n", lpFile, lpTitle, cbBuf);
    if (lpFile == NULL || lpTitle == NULL)
    	return -1;
    len = strlen(lpFile);
    if (len == 0)
    	return -1;
    if (strpbrk(lpFile, "*[]"))
    	return -1;
    len--;
    if (lpFile[len] == '/' || lpFile[len] == '\\' || lpFile[len] == ':')
    	return -1;
    for (i = len; i >= 0; i--)
    	if (lpFile[i] == '/' ||  lpFile[i] == '\\' ||  lpFile[i] == ':')
    {
	i++;
	break;
    }
    if (i == -1)
      i++;
    TRACE(commdlg,"---> '%s' \n", &lpFile[i]);
    
    len = strlen(lpFile+i)+1;
    if (cbBuf < len)
    	return len;

    strncpy(lpTitle, &lpFile[i], len);
    return 0;
}


/***********************************************************************
 *           GetFileTitleA   (COMDLG32.8)
 */
short WINAPI GetFileTitleW(LPCWSTR lpFile, LPWSTR lpTitle, UINT cbBuf)
{
	LPSTR file = HEAP_strdupWtoA(GetProcessHeap(),0,lpFile);
	LPSTR title = HeapAlloc(GetProcessHeap(),0,cbBuf);
	short	ret;

	ret = GetFileTitleA(file,title,cbBuf);

	lstrcpynAtoW(lpTitle,title,cbBuf);
	HeapFree(GetProcessHeap(),0,file);
	HeapFree(GetProcessHeap(),0,title);
	return ret;
}
/***********************************************************************
 *           GetFileTitle   (COMMDLG.27)
 */
short WINAPI GetFileTitle16(LPCSTR lpFile, LPSTR lpTitle, UINT16 cbBuf)
{
    return GetFileTitleA(lpFile,lpTitle,cbBuf);
}


/* ------------------------ Choose Color Dialog --------------------------- */

/***********************************************************************
 *           ChooseColor   (COMMDLG.5)
 */
BOOL16 WINAPI ChooseColor16(LPCHOOSECOLOR16 lpChCol)
{
    HINSTANCE16 hInst;
    HANDLE16 hDlgTmpl = 0;
    BOOL16 bRet = FALSE, win32Format = FALSE;
    LPCVOID template;
    HWND hwndDialog;

    TRACE(commdlg,"ChooseColor\n");
    if (!lpChCol) return FALSE;    

    if (lpChCol->Flags & CC_ENABLETEMPLATEHANDLE)
    {
        if (!(template = LockResource16( lpChCol->hInstance )))
        {
            CommDlgLastError = CDERR_LOADRESFAILURE;
            return FALSE;
        }
    }
    else if (lpChCol->Flags & CC_ENABLETEMPLATE)
    {
        HANDLE16 hResInfo;
        if (!(hResInfo = FindResource16(lpChCol->hInstance,
                                        lpChCol->lpTemplateName,
                                        RT_DIALOG16)))
        {
            CommDlgLastError = CDERR_FINDRESFAILURE;
            return FALSE;
        }
        if (!(hDlgTmpl = LoadResource16( lpChCol->hInstance, hResInfo )) ||
            !(template = LockResource16( hDlgTmpl )))
        {
            CommDlgLastError = CDERR_LOADRESFAILURE;
            return FALSE;
        }
    }
    else
    {
        template = SYSRES_GetResPtr( SYSRES_DIALOG_CHOOSE_COLOR );
        win32Format = TRUE;
    }

    hInst = WIN_GetWindowInstance( lpChCol->hwndOwner );
    hwndDialog = DIALOG_CreateIndirect( hInst, template, win32Format,
                                        lpChCol->hwndOwner,
                           (DLGPROC16)MODULE_GetWndProcEntry16("ColorDlgProc"),
                                        (DWORD)lpChCol, WIN_PROC_16 );
    if (hwndDialog) bRet = DIALOG_DoDialogBox( hwndDialog, lpChCol->hwndOwner);
    if (hDlgTmpl) FreeResource16( hDlgTmpl );

    return bRet;
}


static const COLORREF predefcolors[6][8]=
{
 { 0x008080FFL, 0x0080FFFFL, 0x0080FF80L, 0x0080FF00L,
   0x00FFFF80L, 0x00FF8000L, 0x00C080FFL, 0x00FF80FFL },
 { 0x000000FFL, 0x0000FFFFL, 0x0000FF80L, 0x0040FF00L,
   0x00FFFF00L, 0x00C08000L, 0x00C08080L, 0x00FF00FFL },

 { 0x00404080L, 0x004080FFL, 0x0000FF00L, 0x00808000L,
   0x00804000L, 0x00FF8080L, 0x00400080L, 0x008000FFL },
 { 0x00000080L, 0x000080FFL, 0x00008000L, 0x00408000L,
   0x00FF0000L, 0x00A00000L, 0x00800080L, 0x00FF0080L },

 { 0x00000040L, 0x00004080L, 0x00004000L, 0x00404000L,
   0x00800000L, 0x00400000L, 0x00400040L, 0x00800040L },
 { 0x00000000L, 0x00008080L, 0x00408080L, 0x00808080L,
   0x00808040L, 0x00C0C0C0L, 0x00400040L, 0x00FFFFFFL },
};

struct CCPRIVATE
{
 LPCHOOSECOLOR16 lpcc;  /* points to public known data structure */
 int nextuserdef;     /* next free place in user defined color array */
 HDC16 hdcMem;        /* color graph used for BitBlt() */
 HBITMAP16 hbmMem;    /* color graph bitmap */    
 RECT16 fullsize;     /* original dialog window size */
 UINT16 msetrgb;        /* # of SETRGBSTRING message (today not used)  */
 RECT16 old3angle;    /* last position of l-marker */
 RECT16 oldcross;     /* last position of color/satuation marker */
 BOOL updating;     /* to prevent recursive WM_COMMAND/EN_UPDATE procesing */
 int h;
 int s;
 int l;               /* for temporary storing of hue,sat,lum */
};

/***********************************************************************
 *                             CC_HSLtoRGB                    [internal]
 */
static int CC_HSLtoRGB(char c,int hue,int sat,int lum)
{
 int res=0,maxrgb;

 /* hue */
 switch(c)
 {
  case 'R':if (hue>80)  hue-=80;  else hue+=160; break;
  case 'G':if (hue>160) hue-=160; else hue+=80;  break;
  case 'B':break;
 }

 /* l below 120 */
 maxrgb=(256*MIN(120,lum))/120;  /* 0 .. 256 */
 if (hue< 80)
  res=0;
 else
  if (hue< 120)
  {
   res=(hue-80)* maxrgb;           /* 0...10240 */
   res/=40;                        /* 0...256 */
  }
  else
   if (hue< 200)
    res=maxrgb;
   else
    {
     res=(240-hue)* maxrgb;
     res/=40;
    }
 res=res-maxrgb/2;                 /* -128...128 */

 /* saturation */
 res=maxrgb/2 + (sat*res) /240;    /* 0..256 */

 /* lum above 120 */
 if (lum>120 && res<256)
  res+=((lum-120) * (256-res))/120;

 return MIN(res,255);
}

/***********************************************************************
 *                             CC_RGBtoHSL                    [internal]
 */
static int CC_RGBtoHSL(char c,int r,int g,int b)
{
 WORD maxi,mini,mmsum,mmdif,result=0;
 int iresult=0;

 maxi=MAX(r,b);
 maxi=MAX(maxi,g);
 mini=MIN(r,b);
 mini=MIN(mini,g);

 mmsum=maxi+mini;
 mmdif=maxi-mini;

 switch(c)
 {
  /* lum */
  case 'L':mmsum*=120;              /* 0...61200=(255+255)*120 */
	   result=mmsum/255;        /* 0...240 */
	   break;
  /* saturation */
  case 'S':if (!mmsum)
	    result=0;
	   else
	    if (!mini || maxi==255)
	     result=240;
	   else
	   {
	    result=mmdif*240;       /* 0...61200=255*240 */
	    result/= (mmsum>255 ? mmsum=510-mmsum : mmsum); /* 0..255 */
	   }
	   break;
  /* hue */
  case 'H':if (!mmdif)
	    result=160;
	   else
	   {
	    if (maxi==r)
	    {
	     iresult=40*(g-b);       /* -10200 ... 10200 */
	     iresult/=(int)mmdif;    /* -40 .. 40 */
	     if (iresult<0)
	      iresult+=240;          /* 0..40 and 200..240 */
	    }
	    else
	     if (maxi==g)
	     {
	      iresult=40*(b-r);
	      iresult/=(int)mmdif;
	      iresult+=80;           /* 40 .. 120 */
	     }
	     else
	      if (maxi==b)
	      {
	       iresult=40*(r-g);
	       iresult/=(int)mmdif;
	       iresult+=160;         /* 120 .. 200 */
	      }
	    result=iresult;
	   }
	   break;
 }
 return result;    /* is this integer arithmetic precise enough ? */
}

#define DISTANCE 4

/***********************************************************************
 *                CC_MouseCheckPredefColorArray               [internal]
 */
static int CC_MouseCheckPredefColorArray(HWND16 hDlg,int dlgitem,int rows,int cols,
	    LPARAM lParam,COLORREF *cr)
{
 HWND16 hwnd;
 POINT16 point = MAKEPOINT16(lParam);
 RECT16 rect;
 int dx,dy,x,y;

 ClientToScreen16(hDlg,&point);
 hwnd=GetDlgItem(hDlg,dlgitem);
 GetWindowRect16(hwnd,&rect);
 if (PtInRect16(&rect,point))
 {
  dx=(rect.right-rect.left)/cols;
  dy=(rect.bottom-rect.top)/rows;
  ScreenToClient16(hwnd,&point);

  if (point.x % dx < (dx-DISTANCE) && point.y % dy < (dy-DISTANCE))
  {
   x=point.x/dx;
   y=point.y/dy;
   *cr=predefcolors[y][x];
   /* FIXME: Draw_a_Focus_Rect() */
   return 1;
  }
 }
 return 0;
}

/***********************************************************************
 *                  CC_MouseCheckUserColorArray               [internal]
 */
static int CC_MouseCheckUserColorArray(HWND16 hDlg,int dlgitem,int rows,int cols,
	    LPARAM lParam,COLORREF *cr,COLORREF*crarr)
{
 HWND16 hwnd;
 POINT16 point = MAKEPOINT16(lParam);
 RECT16 rect;
 int dx,dy,x,y;

 ClientToScreen16(hDlg,&point);
 hwnd=GetDlgItem(hDlg,dlgitem);
 GetWindowRect16(hwnd,&rect);
 if (PtInRect16(&rect,point))
 {
  dx=(rect.right-rect.left)/cols;
  dy=(rect.bottom-rect.top)/rows;
  ScreenToClient16(hwnd,&point);

  if (point.x % dx < (dx-DISTANCE) && point.y % dy < (dy-DISTANCE))
  {
   x=point.x/dx;
   y=point.y/dy;
   *cr=crarr[x+cols*y];
   /* FIXME: Draw_a_Focus_Rect() */
   return 1;
  }
 }
 return 0;
}

#define MAXVERT  240
#define MAXHORI  239

/*  240  ^......        ^^ 240
	 |     .        ||
    SAT  |     .        || LUM
	 |     .        ||
	 +-----> 239   ----
	   HUE
*/
/***********************************************************************
 *                  CC_MouseCheckColorGraph                   [internal]
 */
static int CC_MouseCheckColorGraph(HWND16 hDlg,int dlgitem,int *hori,int *vert,LPARAM lParam)
{
 HWND hwnd;
 POINT16 point = MAKEPOINT16(lParam);
 RECT16 rect;
 long x,y;

 ClientToScreen16(hDlg,&point);
 hwnd=GetDlgItem(hDlg,dlgitem);
 GetWindowRect16(hwnd,&rect);
 if (PtInRect16(&rect,point))
 {
  GetClientRect16(hwnd,&rect);
  ScreenToClient16(hwnd,&point);

  x=(long)point.x*MAXHORI;
  x/=rect.right;
  y=(long)(rect.bottom-point.y)*MAXVERT;
  y/=rect.bottom;

  if (hori)
   *hori=x;
  if (vert)
   *vert=y;
  return 1;
 }
 else
  return 0;
}
/***********************************************************************
 *                  CC_MouseCheckResultWindow                 [internal]
 */
static int CC_MouseCheckResultWindow(HWND16 hDlg,LPARAM lParam)
{
 HWND16 hwnd;
 POINT16 point = MAKEPOINT16(lParam);
 RECT16 rect;

 ClientToScreen16(hDlg,&point);
 hwnd=GetDlgItem(hDlg,0x2c5);
 GetWindowRect16(hwnd,&rect);
 if (PtInRect16(&rect,point))
 {
  PostMessage16(hDlg,WM_COMMAND,0x2c9,0);
  return 1;
 }
 return 0;
}

/***********************************************************************
 *                       CC_CheckDigitsInEdit                 [internal]
 */
static int CC_CheckDigitsInEdit(HWND16 hwnd,int maxval)
{
 int i,k,m,result,value;
 long editpos;
 char buffer[30];
 GetWindowTextA(hwnd,buffer,sizeof(buffer));
 m=strlen(buffer);
 result=0;

 for (i=0;i<m;i++)
  if (buffer[i]<'0' || buffer[i]>'9')
  {
   for (k=i+1;k<=m;k++)   /* delete bad character */
   {
    buffer[i]=buffer[k];
    m--;
   }
   buffer[m]=0;
   result=1;
  }

 value=atoi(buffer);
 if (value>maxval)       /* build a new string */
 {
  sprintf(buffer,"%d",maxval);
  result=2;
 }
 if (result)
 {
  editpos=SendMessage16(hwnd,EM_GETSEL16,0,0);
  SetWindowTextA(hwnd,buffer);
  SendMessage16(hwnd,EM_SETSEL16,0,editpos);
 }
 return value;
}



/***********************************************************************
 *                    CC_PaintSelectedColor                   [internal]
 */
static void CC_PaintSelectedColor(HWND16 hDlg,COLORREF cr)
{
 RECT16 rect;
 HDC  hdc;
 HBRUSH hBrush;
 HWND hwnd=GetDlgItem(hDlg,0x2c5);
 if (IsWindowVisible(GetDlgItem(hDlg,0x2c6)))   /* if full size */
 {
  hdc=GetDC(hwnd);
  GetClientRect16 (hwnd, &rect) ;
  hBrush = CreateSolidBrush(cr);
  if (hBrush)
  {
   hBrush = SelectObject (hdc, hBrush) ;
   Rectangle(hdc, rect.left,rect.top,rect.right/2,rect.bottom);
   DeleteObject (SelectObject (hdc,hBrush)) ;
   hBrush=CreateSolidBrush(GetNearestColor(hdc,cr));
   if (hBrush)
   {
    hBrush= SelectObject (hdc, hBrush) ;
    Rectangle( hdc, rect.right/2-1,rect.top,rect.right,rect.bottom);
    DeleteObject( SelectObject (hdc, hBrush)) ;
   }
  }
  ReleaseDC(hwnd,hdc);
 }
}

/***********************************************************************
 *                    CC_PaintTriangle                        [internal]
 */
static void CC_PaintTriangle(HWND16 hDlg,int y)
{
 HDC hDC;
 long temp;
 int w=GetDialogBaseUnits();
 POINT16 points[3];
 int height;
 int oben;
 RECT16 rect;
 HWND16 hwnd=GetDlgItem(hDlg,0x2be);
 struct CCPRIVATE *lpp=(struct CCPRIVATE *)GetWindowLongA(hDlg, DWL_USER); 

 if (IsWindowVisible(GetDlgItem(hDlg,0x2c6)))   /* if full size */
 {
   GetClientRect16(hwnd,&rect);
   height=rect.bottom;
   hDC=GetDC(hDlg);

   points[0].y=rect.top;
   points[0].x=rect.right;           /*  |  /|  */
   ClientToScreen16(hwnd,points);    /*  | / |  */
   ScreenToClient16(hDlg,points);    /*  |<  |  */
   oben=points[0].y;                 /*  | \ |  */
				     /*  |  \|  */
   temp=(long)height*(long)y;
   points[0].y=oben+height -temp/(long)MAXVERT;
   points[1].y=points[0].y+w;
   points[2].y=points[0].y-w;
   points[2].x=points[1].x=points[0].x + w;

   if (lpp->old3angle.left)
    FillRect16(hDC,&lpp->old3angle,GetStockObject(WHITE_BRUSH));
   lpp->old3angle.left  =points[0].x;
   lpp->old3angle.right =points[1].x+1;
   lpp->old3angle.top   =points[2].y-1;
   lpp->old3angle.bottom=points[1].y+1;
   Polygon16(hDC,points,3);
   ReleaseDC(hDlg,hDC);
 }
}


/***********************************************************************
 *                    CC_PaintCross                           [internal]
 */
static void CC_PaintCross(HWND16 hDlg,int x,int y)
{
 HDC hDC;
 int w=GetDialogBaseUnits();
 HWND16 hwnd=GetDlgItem(hDlg,0x2c6);
 struct CCPRIVATE * lpp=(struct CCPRIVATE *)GetWindowLongA(hDlg, DWL_USER); 
 RECT16 rect;
 POINT16 point;
 HPEN hPen;

 if (IsWindowVisible(GetDlgItem(hDlg,0x2c6)))   /* if full size */
 {
   GetClientRect16(hwnd,&rect);
   hDC=GetDC(hwnd);
   SelectClipRgn(hDC,CreateRectRgnIndirect16(&rect));
   hPen=CreatePen(PS_SOLID,2,0);
   hPen=SelectObject(hDC,hPen);
   point.x=((long)rect.right*(long)x)/(long)MAXHORI;
   point.y=rect.bottom-((long)rect.bottom*(long)y)/(long)MAXVERT;
   if (lpp->oldcross.left!=lpp->oldcross.right)
     BitBlt(hDC,lpp->oldcross.left,lpp->oldcross.top,
              lpp->oldcross.right-lpp->oldcross.left,
              lpp->oldcross.bottom-lpp->oldcross.top,
              lpp->hdcMem,lpp->oldcross.left,lpp->oldcross.top,SRCCOPY);
   lpp->oldcross.left  =point.x-w-1;
   lpp->oldcross.right =point.x+w+1;
   lpp->oldcross.top   =point.y-w-1;
   lpp->oldcross.bottom=point.y+w+1; 

   MoveTo16(hDC,point.x-w,point.y); 
   LineTo(hDC,point.x+w,point.y);
   MoveTo16(hDC,point.x,point.y-w); 
   LineTo(hDC,point.x,point.y+w);
   DeleteObject(SelectObject(hDC,hPen));
   ReleaseDC(hwnd,hDC);
 }
}


#define XSTEPS 48
#define YSTEPS 24


/***********************************************************************
 *                    CC_PrepareColorGraph                    [internal]
 */
static void CC_PrepareColorGraph(HWND16 hDlg)    
{
 int sdif,hdif,xdif,ydif,r,g,b,hue,sat;
 HWND hwnd=GetDlgItem(hDlg,0x2c6);
 struct CCPRIVATE * lpp=(struct CCPRIVATE *)GetWindowLongA(hDlg, DWL_USER);  
 HBRUSH hbrush;
 HDC hdc ;
 RECT16 rect,client;
 HCURSOR16 hcursor=SetCursor16(LoadCursor16(0,IDC_WAIT16));

 GetClientRect16(hwnd,&client);
 hdc=GetDC(hwnd);
 lpp->hdcMem = CreateCompatibleDC(hdc);
 lpp->hbmMem = CreateCompatibleBitmap(hdc,client.right,client.bottom);
 SelectObject(lpp->hdcMem,lpp->hbmMem);

 xdif=client.right /XSTEPS;
 ydif=client.bottom/YSTEPS+1;
 hdif=239/XSTEPS;
 sdif=240/YSTEPS;
 for(rect.left=hue=0;hue<239+hdif;hue+=hdif)
 {
  rect.right=rect.left+xdif;
  rect.bottom=client.bottom;
  for(sat=0;sat<240+sdif;sat+=sdif)
  {
   rect.top=rect.bottom-ydif;
   r=CC_HSLtoRGB('R',hue,sat,120);
   g=CC_HSLtoRGB('G',hue,sat,120);
   b=CC_HSLtoRGB('B',hue,sat,120);
   hbrush=CreateSolidBrush(RGB(r,g,b));
   FillRect16(lpp->hdcMem,&rect,hbrush);
   DeleteObject(hbrush);
   rect.bottom=rect.top;
  }
  rect.left=rect.right;
 }
 ReleaseDC(hwnd,hdc);
 SetCursor16(hcursor);
}

/***********************************************************************
 *                          CC_PaintColorGraph                [internal]
 */
static void CC_PaintColorGraph(HWND16 hDlg)
{
 HWND hwnd=GetDlgItem(hDlg,0x2c6);
 struct CCPRIVATE * lpp=(struct CCPRIVATE *)GetWindowLongA(hDlg, DWL_USER); 
 HDC  hDC;
 RECT16 rect;
 if (IsWindowVisible(hwnd))   /* if full size */
 {
  if (!lpp->hdcMem)
   CC_PrepareColorGraph(hDlg);   /* should not be necessary */

  hDC=GetDC(hwnd);
  GetClientRect16(hwnd,&rect);
  if (lpp->hdcMem)
      BitBlt(hDC,0,0,rect.right,rect.bottom,lpp->hdcMem,0,0,SRCCOPY);
  else
    WARN(commdlg,"choose color: hdcMem is not defined\n");
  ReleaseDC(hwnd,hDC);
 }
}
/***********************************************************************
 *                           CC_PaintLumBar                   [internal]
 */
static void CC_PaintLumBar(HWND16 hDlg,int hue,int sat)
{
 HWND hwnd=GetDlgItem(hDlg,0x2be);
 RECT16 rect,client;
 int lum,ldif,ydif,r,g,b;
 HBRUSH hbrush;
 HDC hDC;

 if (IsWindowVisible(hwnd))
 {
  hDC=GetDC(hwnd);
  GetClientRect16(hwnd,&client);
  rect=client;

  ldif=240/YSTEPS;
  ydif=client.bottom/YSTEPS+1;
  for(lum=0;lum<240+ldif;lum+=ldif)
  {
   rect.top=MAX(0,rect.bottom-ydif);
   r=CC_HSLtoRGB('R',hue,sat,lum);
   g=CC_HSLtoRGB('G',hue,sat,lum);
   b=CC_HSLtoRGB('B',hue,sat,lum);
   hbrush=CreateSolidBrush(RGB(r,g,b));
   FillRect16(hDC,&rect,hbrush);
   DeleteObject(hbrush);
   rect.bottom=rect.top;
  }
  GetClientRect16(hwnd,&rect);
  FrameRect16(hDC,&rect,GetStockObject(BLACK_BRUSH));
  ReleaseDC(hwnd,hDC);
 }
}

/***********************************************************************
 *                             CC_EditSetRGB                  [internal]
 */
static void CC_EditSetRGB(HWND16 hDlg,COLORREF cr)
{
 char buffer[10];
 struct CCPRIVATE * lpp=(struct CCPRIVATE *)GetWindowLongA(hDlg, DWL_USER); 
 int r=GetRValue(cr);
 int g=GetGValue(cr);
 int b=GetBValue(cr);
 if (IsWindowVisible(GetDlgItem(hDlg,0x2c6)))   /* if full size */
 {
   lpp->updating=TRUE;
   sprintf(buffer,"%d",r);
   SetWindowTextA(GetDlgItem(hDlg,0x2c2),buffer);
   sprintf(buffer,"%d",g);
   SetWindowTextA(GetDlgItem(hDlg,0x2c3),buffer);
   sprintf(buffer,"%d",b);
   SetWindowTextA(GetDlgItem(hDlg,0x2c4),buffer);
   lpp->updating=FALSE;
 }
}

/***********************************************************************
 *                             CC_EditSetHSL                  [internal]
 */
static void CC_EditSetHSL(HWND16 hDlg,int h,int s,int l)
{
 char buffer[10];
 struct CCPRIVATE * lpp=(struct CCPRIVATE *)GetWindowLongA(hDlg, DWL_USER); 
 lpp->updating=TRUE;
 if (IsWindowVisible(GetDlgItem(hDlg,0x2c6)))   /* if full size */
 {
   lpp->updating=TRUE;
   sprintf(buffer,"%d",h);
   SetWindowTextA(GetDlgItem(hDlg,0x2bf),buffer);
   sprintf(buffer,"%d",s);
   SetWindowTextA(GetDlgItem(hDlg,0x2c0),buffer);
   sprintf(buffer,"%d",l);
   SetWindowTextA(GetDlgItem(hDlg,0x2c1),buffer);
   lpp->updating=FALSE;
 }
 CC_PaintLumBar(hDlg,h,s);
}

/***********************************************************************
 *                       CC_SwitchToFullSize                  [internal]
 */
static void CC_SwitchToFullSize(HWND16 hDlg,COLORREF result,LPRECT16 lprect)
{
 int i;
 struct CCPRIVATE * lpp=(struct CCPRIVATE *)GetWindowLongA(hDlg, DWL_USER); 
 
 EnableWindow(GetDlgItem(hDlg,0x2cf),FALSE);
 CC_PrepareColorGraph(hDlg);
 for (i=0x2bf;i<0x2c5;i++)
   EnableWindow(GetDlgItem(hDlg,i),TRUE);
 for (i=0x2d3;i<0x2d9;i++)
   EnableWindow(GetDlgItem(hDlg,i),TRUE);
 EnableWindow(GetDlgItem(hDlg,0x2c9),TRUE);
 EnableWindow(GetDlgItem(hDlg,0x2c8),TRUE);

 if (lprect)
  SetWindowPos(hDlg,0,0,0,lprect->right-lprect->left,
   lprect->bottom-lprect->top, SWP_NOMOVE|SWP_NOZORDER);

 ShowWindow(GetDlgItem(hDlg,0x2c6),SW_SHOW);
 ShowWindow(GetDlgItem(hDlg,0x2be),SW_SHOW);
 ShowWindow(GetDlgItem(hDlg,0x2c5),SW_SHOW);

 CC_EditSetRGB(hDlg,result);
 CC_EditSetHSL(hDlg,lpp->h,lpp->s,lpp->l);
}

/***********************************************************************
 *                           CC_PaintPredefColorArray         [internal]
 */
static void CC_PaintPredefColorArray(HWND16 hDlg,int rows,int cols)
{
 HWND hwnd=GetDlgItem(hDlg,0x2d0);
 RECT16 rect;
 HDC  hdc;
 HBRUSH hBrush;
 int dx,dy,i,j,k;

 GetClientRect16(hwnd,&rect);
 dx=rect.right/cols;
 dy=rect.bottom/rows;
 k=rect.left;

 hdc=GetDC(hwnd);
 GetClientRect16 (hwnd, &rect) ;

 for (j=0;j<rows;j++)
 {
  for (i=0;i<cols;i++)
  {
   hBrush = CreateSolidBrush(predefcolors[j][i]);
   if (hBrush)
   {
    hBrush = SelectObject (hdc, hBrush) ;
    Rectangle(hdc, rect.left, rect.top,
                rect.left+dx-DISTANCE, rect.top+dy-DISTANCE);
    rect.left=rect.left+dx;
    DeleteObject( SelectObject (hdc, hBrush)) ;
   }
  }
  rect.top=rect.top+dy;
  rect.left=k;
 }
 ReleaseDC(hwnd,hdc);
 /* FIXME: draw_a_focus_rect */
}
/***********************************************************************
 *                             CC_PaintUserColorArray         [internal]
 */
static void CC_PaintUserColorArray(HWND16 hDlg,int rows,int cols,COLORREF* lpcr)
{
 HWND hwnd=GetDlgItem(hDlg,0x2d1);
 RECT16 rect;
 HDC  hdc;
 HBRUSH hBrush;
 int dx,dy,i,j,k;

 GetClientRect16(hwnd,&rect);

 dx=rect.right/cols;
 dy=rect.bottom/rows;
 k=rect.left;

 hdc=GetDC(hwnd);
 if (hdc)
 {
  for (j=0;j<rows;j++)
  {
   for (i=0;i<cols;i++)
   {
    hBrush = CreateSolidBrush(lpcr[i+j*cols]);
    if (hBrush)
    {
     hBrush = SelectObject (hdc, hBrush) ;
     Rectangle( hdc, rect.left, rect.top,
                  rect.left+dx-DISTANCE, rect.top+dy-DISTANCE);
     rect.left=rect.left+dx;
     DeleteObject( SelectObject (hdc, hBrush)) ;
    }
   }
   rect.top=rect.top+dy;
   rect.left=k;
  }
  ReleaseDC(hwnd,hdc);
 }
 /* FIXME: draw_a_focus_rect */
}



/***********************************************************************
 *                             CC_HookCallChk                 [internal]
 */
static BOOL CC_HookCallChk(LPCHOOSECOLOR16 lpcc)
{
 if (lpcc)
  if(lpcc->Flags & CC_ENABLEHOOK)
   if (lpcc->lpfnHook)
    return TRUE;
 return FALSE;
}

/***********************************************************************
 *                            CC_WMInitDialog                 [internal]
 */
static LONG CC_WMInitDialog(HWND16 hDlg, WPARAM16 wParam, LPARAM lParam) 
{
   int i,res;
   int r, g, b;
   HWND16 hwnd;
   RECT16 rect;
   POINT16 point;
   struct CCPRIVATE * lpp; 
   
   TRACE(commdlg,"WM_INITDIALOG lParam=%08lX\n", lParam);
   lpp=calloc(1,sizeof(struct CCPRIVATE));
   lpp->lpcc=(LPCHOOSECOLOR16)lParam;

   if (lpp->lpcc->lStructSize != sizeof(CHOOSECOLOR16))
   {
      EndDialog (hDlg, 0) ;
      return FALSE;
   }
   SetWindowLongA(hDlg, DWL_USER, (LONG)lpp); 

   if (!(lpp->lpcc->Flags & CC_SHOWHELP))
      ShowWindow(GetDlgItem(hDlg,0x40e),SW_HIDE);
   lpp->msetrgb=RegisterWindowMessageA( SETRGBSTRING );

#if 0
   cpos=MAKELONG(5,7); /* init */
   if (lpp->lpcc->Flags & CC_RGBINIT)
   {
     for (i=0;i<6;i++)
       for (j=0;j<8;j++)
        if (predefcolors[i][j]==lpp->lpcc->rgbResult)
        {
          cpos=MAKELONG(i,j);
          goto found;
        }
   }
   found:
   /* FIXME: Draw_a_focus_rect & set_init_values */
#endif

   GetWindowRect16(hDlg,&lpp->fullsize);
   if (lpp->lpcc->Flags & CC_FULLOPEN || lpp->lpcc->Flags & CC_PREVENTFULLOPEN)
   {
      hwnd=GetDlgItem(hDlg,0x2cf);
      EnableWindow(hwnd,FALSE);
   }
   if (!(lpp->lpcc->Flags & CC_FULLOPEN) || lpp->lpcc->Flags & CC_PREVENTFULLOPEN)
   {
      rect=lpp->fullsize;
      res=rect.bottom-rect.top;
      hwnd=GetDlgItem(hDlg,0x2c6); /* cut at left border */
      point.x=point.y=0;
      ClientToScreen16(hwnd,&point);
      ScreenToClient16(hDlg,&point);
      GetClientRect16(hDlg,&rect);
      point.x+=GetSystemMetrics(SM_CXDLGFRAME);
      SetWindowPos(hDlg,0,0,0,point.x,res,SWP_NOMOVE|SWP_NOZORDER);

      ShowWindow(GetDlgItem(hDlg,0x2c6),SW_HIDE);
      ShowWindow(GetDlgItem(hDlg,0x2c5),SW_HIDE);
   }
   else
      CC_SwitchToFullSize(hDlg,lpp->lpcc->rgbResult,NULL);
   res=TRUE;
   for (i=0x2bf;i<0x2c5;i++)
     SendMessage16(GetDlgItem(hDlg,i),EM_LIMITTEXT16,3,0);      /* max 3 digits:  xyz  */
   if (CC_HookCallChk(lpp->lpcc))
      res=CallWindowProc16(lpp->lpcc->lpfnHook,hDlg,WM_INITDIALOG,wParam,lParam);

   
   /* Set the initial values of the color chooser dialog */
   r = GetRValue(lpp->lpcc->rgbResult);
   g = GetGValue(lpp->lpcc->rgbResult);
   b = GetBValue(lpp->lpcc->rgbResult);

   CC_PaintSelectedColor(hDlg,lpp->lpcc->rgbResult);
   lpp->h=CC_RGBtoHSL('H',r,g,b);
   lpp->s=CC_RGBtoHSL('S',r,g,b);
   lpp->l=CC_RGBtoHSL('L',r,g,b);

   /* Doing it the long way becaus CC_EditSetRGB/HSL doesn'nt seem to work */
   SetDlgItemInt(hDlg, 703, lpp->h, TRUE);
   SetDlgItemInt(hDlg, 704, lpp->s, TRUE);
   SetDlgItemInt(hDlg, 705, lpp->l, TRUE);
   SetDlgItemInt(hDlg, 706, r, TRUE);
   SetDlgItemInt(hDlg, 707, g, TRUE);
   SetDlgItemInt(hDlg, 708, b, TRUE);

   CC_PaintCross(hDlg,lpp->h,lpp->s);
   CC_PaintTriangle(hDlg,lpp->l);

   return res;
}

/***********************************************************************
 *                              CC_WMCommand                  [internal]
 */
static LRESULT CC_WMCommand(HWND16 hDlg, WPARAM16 wParam, LPARAM lParam) 
{
    int r,g,b,i,xx;
    UINT16 cokmsg;
    HDC hdc;
    COLORREF *cr;
    struct CCPRIVATE * lpp=(struct CCPRIVATE *)GetWindowLongA(hDlg, DWL_USER); 
    TRACE(commdlg,"CC_WMCommand wParam=%x lParam=%lx\n",wParam,lParam);
    switch (wParam)
    {
          case 0x2c2:  /* edit notify RGB */
	  case 0x2c3:
	  case 0x2c4:
	       if (HIWORD(lParam)==EN_UPDATE && !lpp->updating)
			 {
			   i=CC_CheckDigitsInEdit(LOWORD(lParam),255);
			   r=GetRValue(lpp->lpcc->rgbResult);
			   g=GetGValue(lpp->lpcc->rgbResult);
			   b=GetBValue(lpp->lpcc->rgbResult);
			   xx=0;
			   switch (wParam)
			   {
			    case 0x2c2:if ((xx=(i!=r))) r=i;break;
			    case 0x2c3:if ((xx=(i!=g))) g=i;break;
			    case 0x2c4:if ((xx=(i!=b))) b=i;break;
			   }
			   if (xx) /* something has changed */
			   {
			    lpp->lpcc->rgbResult=RGB(r,g,b);
			    CC_PaintSelectedColor(hDlg,lpp->lpcc->rgbResult);
			    lpp->h=CC_RGBtoHSL('H',r,g,b);
			    lpp->s=CC_RGBtoHSL('S',r,g,b);
			    lpp->l=CC_RGBtoHSL('L',r,g,b);
			    CC_EditSetHSL(hDlg,lpp->h,lpp->s,lpp->l);
			    CC_PaintCross(hDlg,lpp->h,lpp->s);
			    CC_PaintTriangle(hDlg,lpp->l);
			   }
			 }
		 break;
		 
	  case 0x2bf:  /* edit notify HSL */
	  case 0x2c0:
	  case 0x2c1:
	       if (HIWORD(lParam)==EN_UPDATE && !lpp->updating)
			 {
			   i=CC_CheckDigitsInEdit(LOWORD(lParam),wParam==0x2bf?239:240);
			   xx=0;
			   switch (wParam)
			   {
			    case 0x2bf:if ((xx=(i!=lpp->h))) lpp->h=i;break;
			    case 0x2c0:if ((xx=(i!=lpp->s))) lpp->s=i;break;
			    case 0x2c1:if ((xx=(i!=lpp->l))) lpp->l=i;break;
			   }
			   if (xx) /* something has changed */
			   {
			    r=CC_HSLtoRGB('R',lpp->h,lpp->s,lpp->l);
			    g=CC_HSLtoRGB('G',lpp->h,lpp->s,lpp->l);
			    b=CC_HSLtoRGB('B',lpp->h,lpp->s,lpp->l);
			    lpp->lpcc->rgbResult=RGB(r,g,b);
			    CC_PaintSelectedColor(hDlg,lpp->lpcc->rgbResult);
			    CC_EditSetRGB(hDlg,lpp->lpcc->rgbResult);
			    CC_PaintCross(hDlg,lpp->h,lpp->s);
			    CC_PaintTriangle(hDlg,lpp->l);
			   }
			 }
	       break;
	       
          case 0x2cf:
               CC_SwitchToFullSize(hDlg,lpp->lpcc->rgbResult,&lpp->fullsize);
	       InvalidateRect( hDlg, NULL, TRUE );
	       SetFocus(GetDlgItem(hDlg,0x2bf));
	       break;

          case 0x2c8:    /* add colors ... column by column */
               cr=PTR_SEG_TO_LIN(lpp->lpcc->lpCustColors);
               cr[(lpp->nextuserdef%2)*8 + lpp->nextuserdef/2]=lpp->lpcc->rgbResult;
               if (++lpp->nextuserdef==16)
		   lpp->nextuserdef=0;
	       CC_PaintUserColorArray(hDlg,2,8,PTR_SEG_TO_LIN(lpp->lpcc->lpCustColors));
	       break;

          case 0x2c9:              /* resulting color */
	       hdc=GetDC(hDlg);
	       lpp->lpcc->rgbResult=GetNearestColor(hdc,lpp->lpcc->rgbResult);
	       ReleaseDC(hDlg,hdc);
	       CC_EditSetRGB(hDlg,lpp->lpcc->rgbResult);
	       CC_PaintSelectedColor(hDlg,lpp->lpcc->rgbResult);
	       r=GetRValue(lpp->lpcc->rgbResult);
	       g=GetGValue(lpp->lpcc->rgbResult);
	       b=GetBValue(lpp->lpcc->rgbResult);
	       lpp->h=CC_RGBtoHSL('H',r,g,b);
	       lpp->s=CC_RGBtoHSL('S',r,g,b);
	       lpp->l=CC_RGBtoHSL('L',r,g,b);
	       CC_EditSetHSL(hDlg,lpp->h,lpp->s,lpp->l);
	       CC_PaintCross(hDlg,lpp->h,lpp->s);
	       CC_PaintTriangle(hDlg,lpp->l);
	       break;

	  case 0x40e:           /* Help! */ /* The Beatles, 1965  ;-) */
	       i=RegisterWindowMessageA( HELPMSGSTRING );
	       if (lpp->lpcc->hwndOwner)
		   SendMessage16(lpp->lpcc->hwndOwner,i,0,(LPARAM)lpp->lpcc);
	       if (CC_HookCallChk(lpp->lpcc))
		   CallWindowProc16(lpp->lpcc->lpfnHook,hDlg,
		      WM_COMMAND,psh15,(LPARAM)lpp->lpcc);
	       break;

          case IDOK :
		cokmsg=RegisterWindowMessageA( COLOROKSTRING );
		if (lpp->lpcc->hwndOwner)
			if (SendMessage16(lpp->lpcc->hwndOwner,cokmsg,0,(LPARAM)lpp->lpcc))
			   break;    /* do NOT close */

		EndDialog (hDlg, 1) ;
		return TRUE ;
	
	  case IDCANCEL :
		EndDialog (hDlg, 0) ;
		return TRUE ;

       }
       return FALSE;
}

/***********************************************************************
 *                              CC_WMPaint                    [internal]
 */
static LRESULT CC_WMPaint(HWND16 hDlg, WPARAM16 wParam, LPARAM lParam) 
{
    HDC hdc;
    PAINTSTRUCT ps;
    struct CCPRIVATE * lpp=(struct CCPRIVATE *)GetWindowLongA(hDlg, DWL_USER); 

    hdc=BeginPaint(hDlg,&ps);
    EndPaint(hDlg,&ps);
    /* we have to paint dialog children except text and buttons */
 
    CC_PaintPredefColorArray(hDlg,6,8);
    CC_PaintUserColorArray(hDlg,2,8,PTR_SEG_TO_LIN(lpp->lpcc->lpCustColors));
    CC_PaintColorGraph(hDlg);
    CC_PaintLumBar(hDlg,lpp->h,lpp->s);
    CC_PaintCross(hDlg,lpp->h,lpp->s);
    CC_PaintTriangle(hDlg,lpp->l);
    CC_PaintSelectedColor(hDlg,lpp->lpcc->rgbResult);

    /* special necessary for Wine */
    ValidateRect(GetDlgItem(hDlg,0x2d0),NULL);
    ValidateRect(GetDlgItem(hDlg,0x2d1),NULL);
    ValidateRect(GetDlgItem(hDlg,0x2c6),NULL);
    ValidateRect(GetDlgItem(hDlg,0x2be),NULL);
    ValidateRect(GetDlgItem(hDlg,0x2c5),NULL);
    /* hope we can remove it later -->FIXME */
 return TRUE;
}


/***********************************************************************
 *                              CC_WMLButtonDown              [internal]
 */
static LRESULT CC_WMLButtonDown(HWND16 hDlg, WPARAM16 wParam, LPARAM lParam) 
{
   struct CCPRIVATE * lpp=(struct CCPRIVATE *)GetWindowLongA(hDlg, DWL_USER); 
   int r,g,b,i;
   i=0;
   if (CC_MouseCheckPredefColorArray(hDlg,0x2d0,6,8,lParam,&lpp->lpcc->rgbResult))
      i=1;
   else
      if (CC_MouseCheckUserColorArray(hDlg,0x2d1,2,8,lParam,&lpp->lpcc->rgbResult,
	      PTR_SEG_TO_LIN(lpp->lpcc->lpCustColors)))
         i=1;
      else
	 if (CC_MouseCheckColorGraph(hDlg,0x2c6,&lpp->h,&lpp->s,lParam))
	    i=2;
	 else
	    if (CC_MouseCheckColorGraph(hDlg,0x2be,NULL,&lpp->l,lParam))
	       i=2;
   if (i==2)
   {
      r=CC_HSLtoRGB('R',lpp->h,lpp->s,lpp->l);
      g=CC_HSLtoRGB('G',lpp->h,lpp->s,lpp->l);
      b=CC_HSLtoRGB('B',lpp->h,lpp->s,lpp->l);
      lpp->lpcc->rgbResult=RGB(r,g,b);
   }
   if (i==1)
   {
      r=GetRValue(lpp->lpcc->rgbResult);
      g=GetGValue(lpp->lpcc->rgbResult);
      b=GetBValue(lpp->lpcc->rgbResult);
      lpp->h=CC_RGBtoHSL('H',r,g,b);
      lpp->s=CC_RGBtoHSL('S',r,g,b);
      lpp->l=CC_RGBtoHSL('L',r,g,b);
   }
   if (i)
   {
      CC_EditSetRGB(hDlg,lpp->lpcc->rgbResult);
      CC_EditSetHSL(hDlg,lpp->h,lpp->s,lpp->l);
      CC_PaintCross(hDlg,lpp->h,lpp->s);
      CC_PaintTriangle(hDlg,lpp->l);
      CC_PaintSelectedColor(hDlg,lpp->lpcc->rgbResult);
      return TRUE;
   }
   return FALSE;
}

/***********************************************************************
 *           ColorDlgProc   (COMMDLG.8)
 */
LRESULT WINAPI ColorDlgProc16(HWND16 hDlg, UINT16 message,
                            WPARAM16 wParam, LONG lParam)
{
 int res;
 struct CCPRIVATE * lpp=(struct CCPRIVATE *)GetWindowLongA(hDlg, DWL_USER); 
 if (message!=WM_INITDIALOG)
 {
  if (!lpp)
     return FALSE;
  res=0;
  if (CC_HookCallChk(lpp->lpcc))
     res=CallWindowProc16(lpp->lpcc->lpfnHook,hDlg,message,wParam,lParam);
  if (res)
     return res;
 }

 /* FIXME: SetRGB message
 if (message && message==msetrgb)
    return HandleSetRGB(hDlg,lParam);
 */

 switch (message)
	{
	  case WM_INITDIALOG:
	                return CC_WMInitDialog(hDlg,wParam,lParam);
	  case WM_NCDESTROY:
	                DeleteDC(lpp->hdcMem); 
	                DeleteObject(lpp->hbmMem); 
	                free(lpp);
	                SetWindowLongA(hDlg, DWL_USER, 0L); /* we don't need it anymore */
	                break;
	  case WM_COMMAND:
	                if (CC_WMCommand(hDlg, wParam, lParam))
	                   return TRUE;
	                break;     
	  case WM_PAINT:
	                if (CC_WMPaint(hDlg, wParam, lParam))
	                   return TRUE;
	                break;
	  case WM_LBUTTONDBLCLK:
	                if (CC_MouseCheckResultWindow(hDlg,lParam))
			  return TRUE;
			break;
	  case WM_MOUSEMOVE:  /* FIXME: calculate new hue,sat,lum (if in color graph) */
			break;
	  case WM_LBUTTONUP:  /* FIXME: ClipCursor off (if in color graph)*/
			break;
	  case WM_LBUTTONDOWN:/* FIXME: ClipCursor on  (if in color graph)*/
	                if (CC_WMLButtonDown(hDlg, wParam, lParam))
	                   return TRUE;
	                break;     
	}
     return FALSE ;
}

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

    TRACE(commdlg,"ChooseFont\n");
    if (!lpChFont) return FALSE;    

    if (lpChFont->Flags & CF_ENABLETEMPLATEHANDLE)
    {
        if (!(template = LockResource16( lpChFont->hInstance )))
        {
            CommDlgLastError = CDERR_LOADRESFAILURE;
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
            CommDlgLastError = CDERR_FINDRESFAILURE;
            return FALSE;
        }
        if (!(hDlgTmpl = LoadResource16( lpChFont->hInstance, hResInfo )) ||
            !(template = LockResource16( hDlgTmpl )))
        {
            CommDlgLastError = CDERR_LOADRESFAILURE;
            return FALSE;
        }
    }
    else
    {
        template = SYSRES_GetResPtr( SYSRES_DIALOG_CHOOSE_FONT );
        win32Format = TRUE;
    }

    hInst = WIN_GetWindowInstance( lpChFont->hwndOwner );
    
    /* lpTemplateName is not used in the dialog */
    lpTemplateName=lpChFont->lpTemplateName;
    lpChFont->lpTemplateName=(SEGPTR)&cf32a;
    
    hwndDialog = DIALOG_CreateIndirect( hInst, template, win32Format,
                                        lpChFont->hwndOwner,
                      (DLGPROC16)MODULE_GetWndProcEntry16("FormatCharDlgProc"),
                                        (DWORD)lpChFont, WIN_PROC_16 );
    if (hwndDialog) bRet = DIALOG_DoDialogBox(hwndDialog, lpChFont->hwndOwner);
    if (hDlgTmpl) FreeResource16( hDlgTmpl );
    lpChFont->lpTemplateName=lpTemplateName;
    FONT_LogFont32ATo16(cf32a.lpLogFont, 
        (LPLOGFONT16)(PTR_SEG_TO_LIN(lpChFont->lpLogFont)));
    return bRet;
}


/***********************************************************************
 *           ChooseFont32A   (COMDLG32.3)
 */
BOOL WINAPI ChooseFontA(LPCHOOSEFONTA lpChFont)
{
  BOOL bRet=FALSE;
  HWND hwndDialog;
  HINSTANCE hInst=WIN_GetWindowInstance( lpChFont->hwndOwner );
  LPCVOID template = SYSRES_GetResPtr( SYSRES_DIALOG_CHOOSE_FONT );
  if (lpChFont->Flags & (CF_SELECTSCRIPT | CF_NOVERTFONTS | CF_ENABLETEMPLATE |
    CF_ENABLETEMPLATEHANDLE)) FIXME(commdlg, ": unimplemented flag (ignored)\n");
  hwndDialog = DIALOG_CreateIndirect(hInst, template, TRUE, lpChFont->hwndOwner,
            (DLGPROC16)FormatCharDlgProcA, (LPARAM)lpChFont, WIN_PROC_32A );
  if (hwndDialog) bRet = DIALOG_DoDialogBox(hwndDialog, lpChFont->hwndOwner);  
  return bRet;
}

/***********************************************************************
 *           ChooseFont32W   (COMDLG32.4)
 */
BOOL WINAPI ChooseFontW(LPCHOOSEFONTW lpChFont)
{
  BOOL bRet=FALSE;
  HWND hwndDialog;
  HINSTANCE hInst=WIN_GetWindowInstance( lpChFont->hwndOwner );
  CHOOSEFONTA cf32a;
  LOGFONTA lf32a;
  LPCVOID template = SYSRES_GetResPtr( SYSRES_DIALOG_CHOOSE_FONT );
  if (lpChFont->Flags & (CF_SELECTSCRIPT | CF_NOVERTFONTS | CF_ENABLETEMPLATE |
    CF_ENABLETEMPLATEHANDLE)) FIXME(commdlg, ": unimplemented flag (ignored)\n");
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

  TRACE(commdlg,"font=%s (nFontType=%d)\n", lplf->lfFaceName,nFontType);

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
INT WINAPI FontFamilyEnumProc(LPENUMLOGFONTA lpEnumLogFont, 
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
INT AddFontStyle(LPLOGFONTA lplf, UINT nFontType, 
    LPCHOOSEFONTA lpcf, HWND hcmb2, HWND hcmb3, HWND hDlg)
{
  int i;
  
  TRACE(commdlg,"(nFontType=%d)\n",nFontType);
  TRACE(commdlg,"  %s h=%d w=%d e=%d o=%d wg=%d i=%d u=%d s=%d"
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
INT WINAPI FontStyleEnumProc( LPENUMLOGFONTA lpFont, 
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
LRESULT CFn_WMInitDialog(HWND hDlg, WPARAM wParam, LPARAM lParam,
                         LPCHOOSEFONTA lpcf)
{
  HDC hdc;
  int i,j,res,init=0;
  long l;
  LPLOGFONTA lpxx;
  HCURSOR hcursor=SetCursor(LoadCursorA(0,IDC_WAITA));

  SetWindowLongA(hDlg, DWL_USER, lParam); 
  lpxx=lpcf->lpLogFont;
  TRACE(commdlg,"WM_INITDIALOG lParam=%08lX\n", lParam);

  if (lpcf->lStructSize != sizeof(CHOOSEFONTA))
  {
    ERR(commdlg,"structure size failure !!!\n");
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
      TRACE(commdlg,"EnumFontFamilies returns 0\n");
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
    WARN(commdlg,"HDC failure !!!\n");
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
LRESULT CFn_WMCtlColorStatic(HWND hDlg, WPARAM wParam, LPARAM lParam,
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
LRESULT CFn_WMCommand(HWND hDlg, WPARAM wParam, LPARAM lParam,
                      LPCHOOSEFONTA lpcf)
{
  HFONT hFont;
  int i,j;
  long l;
  HDC hdc;
  LPLOGFONTA lpxx=lpcf->lpLogFont;
  
  TRACE(commdlg,"WM_COMMAND wParam=%08lX lParam=%08lX\n", (LONG)wParam, lParam);
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
	                TRACE(commdlg,"WM_COMMAND/cmb1 =>%s\n",str);
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
                      WARN(commdlg,"HDC failure !!!\n");
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
                    TRACE(commdlg,"WM_COMMAND/cmb2,3 =%08lX\n", lParam);
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
     res=CallWindowProc16(lpcf->lpfnHook,hDlg,message,wParam,lParam);
   if (res)
    return res;
  }
  else
  {
    lpcf=(LPCHOOSEFONT16)lParam;
    lpcf32a=(LPCHOOSEFONTA)lpcf->lpTemplateName;
    if (!CFn_WMInitDialog(hDlg, wParam, lParam, lpcf32a)) 
    {
      TRACE(commdlg, "CFn_WMInitDialog returned FALSE\n");
      return FALSE;
    }  
    if (CFn_HookCallChk(lpcf))
      return CallWindowProc16(lpcf->lpfnHook,hDlg,WM_INITDIALOG,wParam,lParam);
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
                         TRACE(commdlg,"WM_CHOOSEFONT_GETLOGFONT lParam=%08lX\n",
				      lParam);
			 FIXME(commdlg, "current logfont back to caller\n");
                        break;
    }
  WINPROC_UnmapMsg16To32A(hDlg,uMsg32, wParam32, lParam, res);    
  return res;
}

/***********************************************************************
 *           FormatCharDlgProc32A   [internal]
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
     res=CallWindowProcA(lpcf->lpfnHook, hDlg, uMsg, wParam, lParam);
   if (res)
     return res;
  }
  else
  {
    lpcf=(LPCHOOSEFONTA)lParam;
    if (!CFn_WMInitDialog(hDlg, wParam, lParam, lpcf)) 
    {
      TRACE(commdlg, "CFn_WMInitDialog returned FALSE\n");
      return FALSE;
    }  
    if (CFn_HookCallChk32(lpcf))
      return CallWindowProcA(lpcf->lpfnHook,hDlg,WM_INITDIALOG,wParam,lParam);
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
                         TRACE(commdlg,"WM_CHOOSEFONT_GETLOGFONT lParam=%08lX\n",
				      lParam);
			 FIXME(commdlg, "current logfont back to caller\n");
                        break;
    }
  return res;
}

/***********************************************************************
 *           FormatCharDlgProc32W   [internal]
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
     res=CallWindowProcW(lpcf32w->lpfnHook, hDlg, uMsg, wParam, lParam);
   if (res)
     return res;
  }
  else
  {
    lpcf32w=(LPCHOOSEFONTW)lParam;
    lpcf32a=(LPCHOOSEFONTA)lpcf32w->lpTemplateName;
    if (!CFn_WMInitDialog(hDlg, wParam, lParam, lpcf32a)) 
    {
      TRACE(commdlg, "CFn_WMInitDialog returned FALSE\n");
      return FALSE;
    }  
    if (CFn_HookCallChk32((LPCHOOSEFONTA)lpcf32w))
      return CallWindowProcW(lpcf32w->lpfnHook,hDlg,WM_INITDIALOG,wParam,lParam);
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
                         TRACE(commdlg,"WM_CHOOSEFONT_GETLOGFONT lParam=%08lX\n",
				      lParam);
			 FIXME(commdlg, "current logfont back to caller\n");
                        break;
    }
  return res;
}


static BOOL Commdlg_GetFileNameA( BOOL16 (CALLBACK *dofunction)(SEGPTR x),
                                      LPOPENFILENAMEA ofn )
{
	BOOL16 ret;
	LPOPENFILENAME16 ofn16 = SEGPTR_ALLOC(sizeof(OPENFILENAME16));

	memset(ofn16,'\0',sizeof(*ofn16));
	ofn16->lStructSize = sizeof(*ofn16);
	ofn16->hwndOwner = ofn->hwndOwner;
	ofn16->hInstance = MapHModuleLS(ofn->hInstance);
	if (ofn->lpstrFilter) {
		LPSTR	s,x;

		/* filter is a list...  title\0ext\0......\0\0 */
		s = (LPSTR)ofn->lpstrFilter;
		while (*s)
			s = s+strlen(s)+1;
		s++;
		x = (LPSTR)SEGPTR_ALLOC(s-ofn->lpstrFilter);
		memcpy(x,ofn->lpstrFilter,s-ofn->lpstrFilter);
		ofn16->lpstrFilter = SEGPTR_GET(x);
	}
	if (ofn->lpstrCustomFilter) {
		LPSTR	s,x;

		/* filter is a list...  title\0ext\0......\0\0 */
		s = (LPSTR)ofn->lpstrCustomFilter;
		while (*s)
			s = s+strlen(s)+1;
		s++;
		x = SEGPTR_ALLOC(s-ofn->lpstrCustomFilter);
		memcpy(x,ofn->lpstrCustomFilter,s-ofn->lpstrCustomFilter);
		ofn16->lpstrCustomFilter = SEGPTR_GET(x);
	}
	ofn16->nMaxCustFilter = ofn->nMaxCustFilter;
	ofn16->nFilterIndex = ofn->nFilterIndex;
	if (ofn->nMaxFile)
	    ofn16->lpstrFile = SEGPTR_GET(SEGPTR_ALLOC(ofn->nMaxFile));
	ofn16->nMaxFile = ofn->nMaxFile;
	ofn16->nMaxFileTitle = ofn->nMaxFileTitle;
        if (ofn16->nMaxFileTitle)
	    ofn16->lpstrFileTitle = SEGPTR_GET(SEGPTR_ALLOC(ofn->nMaxFileTitle));
	if (ofn->lpstrInitialDir)
	    ofn16->lpstrInitialDir = SEGPTR_GET(SEGPTR_STRDUP(ofn->lpstrInitialDir));
	if (ofn->lpstrTitle)
	    ofn16->lpstrTitle = SEGPTR_GET(SEGPTR_STRDUP(ofn->lpstrTitle));
	ofn16->Flags = ofn->Flags|OFN_WINE;
	ofn16->nFileOffset = ofn->nFileOffset;
	ofn16->nFileExtension = ofn->nFileExtension;
	if (ofn->lpstrDefExt)
	    ofn16->lpstrDefExt = SEGPTR_GET(SEGPTR_STRDUP(ofn->lpstrDefExt));
	ofn16->lCustData = ofn->lCustData;
	ofn16->lpfnHook = (WNDPROC16)ofn->lpfnHook;

	if (ofn->lpTemplateName)
	    ofn16->lpTemplateName = SEGPTR_GET(SEGPTR_STRDUP(ofn->lpTemplateName));

	ret = dofunction(SEGPTR_GET(ofn16));

	ofn->nFileOffset = ofn16->nFileOffset;
	ofn->nFileExtension = ofn16->nFileExtension;
	if (ofn16->lpstrFilter)
	    SEGPTR_FREE(PTR_SEG_TO_LIN(ofn16->lpstrFilter));
	if (ofn16->lpTemplateName)
	    SEGPTR_FREE(PTR_SEG_TO_LIN(ofn16->lpTemplateName));
	if (ofn16->lpstrDefExt)
	    SEGPTR_FREE(PTR_SEG_TO_LIN(ofn16->lpstrDefExt));
	if (ofn16->lpstrTitle)
	    SEGPTR_FREE(PTR_SEG_TO_LIN(ofn16->lpstrTitle));
	if (ofn16->lpstrInitialDir)
	    SEGPTR_FREE(PTR_SEG_TO_LIN(ofn16->lpstrInitialDir));
	if (ofn16->lpstrCustomFilter)
	    SEGPTR_FREE(PTR_SEG_TO_LIN(ofn16->lpstrCustomFilter));

	if (ofn16->lpstrFile) 
	  {
	    strcpy(ofn->lpstrFile,PTR_SEG_TO_LIN(ofn16->lpstrFile));
	    SEGPTR_FREE(PTR_SEG_TO_LIN(ofn16->lpstrFile));
	  }

	if (ofn16->lpstrFileTitle) 
	  {
	    strcpy(ofn->lpstrFileTitle,PTR_SEG_TO_LIN(ofn16->lpstrFileTitle));
	    SEGPTR_FREE(PTR_SEG_TO_LIN(ofn16->lpstrFileTitle));
	  }
	SEGPTR_FREE(ofn16);
	return ret;
}

static BOOL Commdlg_GetFileNameW( BOOL16 (CALLBACK *dofunction)(SEGPTR x), 
                                      LPOPENFILENAMEW ofn )
{
	BOOL16 ret;
	LPOPENFILENAME16 ofn16 = SEGPTR_ALLOC(sizeof(OPENFILENAME16));

	memset(ofn16,'\0',sizeof(*ofn16));
	ofn16->lStructSize = sizeof(*ofn16);
	ofn16->hwndOwner = ofn->hwndOwner;
	ofn16->hInstance = MapHModuleLS(ofn->hInstance);
	if (ofn->lpstrFilter) {
		LPWSTR	s;
		LPSTR	x,y;
		int	n;

		/* filter is a list...  title\0ext\0......\0\0 */
		s = (LPWSTR)ofn->lpstrFilter;
		while (*s)
			s = s+lstrlenW(s)+1;
		s++;
		n = s - ofn->lpstrFilter; /* already divides by 2. ptr magic */
		x = y = (LPSTR)SEGPTR_ALLOC(n);
		s = (LPWSTR)ofn->lpstrFilter;
		while (*s) {
			lstrcpyWtoA(x,s);
			x+=lstrlenA(x)+1;
			s+=lstrlenW(s)+1;
		}
		*x=0;
		ofn16->lpstrFilter = SEGPTR_GET(y);
}
	if (ofn->lpstrCustomFilter) {
		LPWSTR	s;
		LPSTR	x,y;
		int	n;

		/* filter is a list...  title\0ext\0......\0\0 */
		s = (LPWSTR)ofn->lpstrCustomFilter;
		while (*s)
			s = s+lstrlenW(s)+1;
		s++;
		n = s - ofn->lpstrCustomFilter;
		x = y = (LPSTR)SEGPTR_ALLOC(n);
		s = (LPWSTR)ofn->lpstrCustomFilter;
		while (*s) {
			lstrcpyWtoA(x,s);
			x+=lstrlenA(x)+1;
			s+=lstrlenW(s)+1;
		}
		*x=0;
		ofn16->lpstrCustomFilter = SEGPTR_GET(y);
	}
	ofn16->nMaxCustFilter = ofn->nMaxCustFilter;
	ofn16->nFilterIndex = ofn->nFilterIndex;
        if (ofn->nMaxFile) 
	   ofn16->lpstrFile = SEGPTR_GET(SEGPTR_ALLOC(ofn->nMaxFile));
	ofn16->nMaxFile = ofn->nMaxFile;
	ofn16->nMaxFileTitle = ofn->nMaxFileTitle;
        if (ofn->nMaxFileTitle)
		ofn16->lpstrFileTitle = SEGPTR_GET(SEGPTR_ALLOC(ofn->nMaxFileTitle));
	if (ofn->lpstrInitialDir)
		ofn16->lpstrInitialDir = SEGPTR_GET(SEGPTR_STRDUP_WtoA(ofn->lpstrInitialDir));
	if (ofn->lpstrTitle)
		ofn16->lpstrTitle = SEGPTR_GET(SEGPTR_STRDUP_WtoA(ofn->lpstrTitle));
	ofn16->Flags = ofn->Flags|OFN_WINE|OFN_UNICODE;
	ofn16->nFileOffset = ofn->nFileOffset;
	ofn16->nFileExtension = ofn->nFileExtension;
	if (ofn->lpstrDefExt)
		ofn16->lpstrDefExt = SEGPTR_GET(SEGPTR_STRDUP_WtoA(ofn->lpstrDefExt));
	ofn16->lCustData = ofn->lCustData;
	ofn16->lpfnHook = (WNDPROC16)ofn->lpfnHook;
	if (ofn->lpTemplateName) 
		ofn16->lpTemplateName = SEGPTR_GET(SEGPTR_STRDUP_WtoA(ofn->lpTemplateName));
	ret = dofunction(SEGPTR_GET(ofn16));

	ofn->nFileOffset = ofn16->nFileOffset;
	ofn->nFileExtension = ofn16->nFileExtension;
	if (ofn16->lpstrFilter)
		SEGPTR_FREE(PTR_SEG_TO_LIN(ofn16->lpstrFilter));
	if (ofn16->lpTemplateName)
		SEGPTR_FREE(PTR_SEG_TO_LIN(ofn16->lpTemplateName));
	if (ofn16->lpstrDefExt)
		SEGPTR_FREE(PTR_SEG_TO_LIN(ofn16->lpstrDefExt));
	if (ofn16->lpstrTitle)
		SEGPTR_FREE(PTR_SEG_TO_LIN(ofn16->lpstrTitle));
	if (ofn16->lpstrInitialDir)
		SEGPTR_FREE(PTR_SEG_TO_LIN(ofn16->lpstrInitialDir));
	if (ofn16->lpstrCustomFilter)
		SEGPTR_FREE(PTR_SEG_TO_LIN(ofn16->lpstrCustomFilter));

	if (ofn16->lpstrFile) {
	lstrcpyAtoW(ofn->lpstrFile,PTR_SEG_TO_LIN(ofn16->lpstrFile));
	SEGPTR_FREE(PTR_SEG_TO_LIN(ofn16->lpstrFile));
	}

	if (ofn16->lpstrFileTitle) {
                lstrcpyAtoW(ofn->lpstrFileTitle,PTR_SEG_TO_LIN(ofn16->lpstrFileTitle));
		SEGPTR_FREE(PTR_SEG_TO_LIN(ofn16->lpstrFileTitle));
	}
	SEGPTR_FREE(ofn16);
	return ret;
}

/***********************************************************************
 *            GetOpenFileName32A  (COMDLG32.10)
 */
BOOL WINAPI GetOpenFileNameA( LPOPENFILENAMEA ofn )
{
   BOOL16 (CALLBACK * dofunction)(SEGPTR ofn16) = GetOpenFileName16;
   return Commdlg_GetFileNameA(dofunction,ofn);
}

/***********************************************************************
 *            GetOpenFileName32W (COMDLG32.11)
 */
BOOL WINAPI GetOpenFileNameW( LPOPENFILENAMEW ofn )
{
   BOOL16 (CALLBACK * dofunction)(SEGPTR ofn16) = GetOpenFileName16;
   return Commdlg_GetFileNameW(dofunction,ofn);
}

/***********************************************************************
 *            GetSaveFileName32A  (COMDLG32.12)
 */
BOOL WINAPI GetSaveFileNameA( LPOPENFILENAMEA ofn )
{
   BOOL16 (CALLBACK * dofunction)(SEGPTR ofn16) = GetSaveFileName16;
   return Commdlg_GetFileNameA(dofunction,ofn);
}

/***********************************************************************
 *            GetSaveFileName32W  (COMDLG32.13)
 */
BOOL WINAPI GetSaveFileNameW( LPOPENFILENAMEW ofn )
{
   BOOL16 (CALLBACK * dofunction)(SEGPTR ofn16) = GetSaveFileName16;
   return Commdlg_GetFileNameW(dofunction,ofn);
}

/***********************************************************************
 *            ChooseColorA  (COMDLG32.1)
 */
BOOL WINAPI ChooseColorA(LPCHOOSECOLORA lpChCol )

{
  BOOL16 ret;
  char *str = NULL;
  COLORREF* ccref=SEGPTR_ALLOC(64);
  LPCHOOSECOLOR16 lpcc16=SEGPTR_ALLOC(sizeof(CHOOSECOLOR16));

  memset(lpcc16,'\0',sizeof(*lpcc16));
  lpcc16->lStructSize=sizeof(*lpcc16);
  lpcc16->hwndOwner=lpChCol->hwndOwner;
  lpcc16->hInstance=MapHModuleLS(lpChCol->hInstance);
  lpcc16->rgbResult=lpChCol->rgbResult;
  memcpy(ccref,lpChCol->lpCustColors,64);
  lpcc16->lpCustColors=(COLORREF*)SEGPTR_GET(ccref);
  lpcc16->Flags=lpChCol->Flags;
  lpcc16->lCustData=lpChCol->lCustData;
  lpcc16->lpfnHook=(WNDPROC16)lpChCol->lpfnHook;
  if (lpChCol->lpTemplateName)
    str = SEGPTR_STRDUP(lpChCol->lpTemplateName );
  lpcc16->lpTemplateName=SEGPTR_GET(str);
  
  ret = ChooseColor16(lpcc16);

  if(ret)
      lpChCol->rgbResult = lpcc16->rgbResult;

  if(str)
    SEGPTR_FREE(str);
  memcpy(lpChCol->lpCustColors,ccref,64);
  SEGPTR_FREE(ccref);
  SEGPTR_FREE(lpcc16);
  return (BOOL)ret;
}

/***********************************************************************
 *            ChooseColorW  (COMDLG32.2)
 */
BOOL WINAPI ChooseColorW(LPCHOOSECOLORW lpChCol )

{
  BOOL16 ret;
  char *str = NULL;
  COLORREF* ccref=SEGPTR_ALLOC(64);
  LPCHOOSECOLOR16 lpcc16=SEGPTR_ALLOC(sizeof(CHOOSECOLOR16));

  memset(lpcc16,'\0',sizeof(*lpcc16));
  lpcc16->lStructSize=sizeof(*lpcc16);
  lpcc16->hwndOwner=lpChCol->hwndOwner;
  lpcc16->hInstance=MapHModuleLS(lpChCol->hInstance);
  lpcc16->rgbResult=lpChCol->rgbResult;
  memcpy(ccref,lpChCol->lpCustColors,64);
  lpcc16->lpCustColors=(COLORREF*)SEGPTR_GET(ccref);
  lpcc16->Flags=lpChCol->Flags;
  lpcc16->lCustData=lpChCol->lCustData;
  lpcc16->lpfnHook=(WNDPROC16)lpChCol->lpfnHook;
  if (lpChCol->lpTemplateName)
    str = SEGPTR_STRDUP_WtoA(lpChCol->lpTemplateName );
  lpcc16->lpTemplateName=SEGPTR_GET(str);
  
  ret = ChooseColor16(lpcc16);

  if(ret)
      lpChCol->rgbResult = lpcc16->rgbResult;

  if(str)
    SEGPTR_FREE(str);
  memcpy(lpChCol->lpCustColors,ccref,64);
  SEGPTR_FREE(ccref);
  SEGPTR_FREE(lpcc16);
  return (BOOL)ret;
}

/***********************************************************************
 *            PageSetupDlgA  (COMDLG32.15)
 */
BOOL WINAPI PageSetupDlgA(LPPAGESETUPDLGA setupdlg) {
	FIXME(commdlg,"(%p), stub!\n",setupdlg);
	return FALSE;
}
