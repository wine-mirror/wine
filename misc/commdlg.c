/*
 * COMMDLG functions
 *
 * Copyright 1994 Martin Ayotte
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dialog.h"
#include "win.h"
#include "user.h"
#include "message.h"
#include "commdlg.h"
#include "dlgs.h"
#include "selectors.h"
#include "../rc/sysres.h"
#include "dos_fs.h"
#include "stackframe.h"

#define OPENFILEDLG2			11
#define SAVEFILEDLG2			12

static	DWORD 		CommDlgLastError = 0;

static	HBITMAP		hFolder = 0;
static	HBITMAP		hFolder2 = 0;
static	HBITMAP		hFloppy = 0;
static	HBITMAP		hHDisk = 0;
static	HBITMAP		hCDRom = 0;

/***********************************************************************
 * 				FileDlg_Init			[internal]
 */
static BOOL FileDlg_Init()
{
    static BOOL initialized = 0;
    
    if (!initialized) {
	if (!hFolder) hFolder = LoadBitmap(0, MAKEINTRESOURCE(OBM_FOLDER));
	if (!hFolder2) hFolder2 = LoadBitmap(0, MAKEINTRESOURCE(OBM_FOLDER2));
	if (!hFloppy) hFloppy = LoadBitmap(0, MAKEINTRESOURCE(OBM_FLOPPY));
	if (!hHDisk) hHDisk = LoadBitmap(0, MAKEINTRESOURCE(OBM_HDISK));
	if (!hCDRom) hCDRom = LoadBitmap(0, MAKEINTRESOURCE(OBM_CDROM));
	if (hFolder == 0 || hFolder2 == 0 || hFloppy == 0 || 
	    hHDisk == 0 || hCDRom == 0)
	{	
	    fprintf(stderr, "FileDlg_Init // Error loading bitmaps !");
	    return FALSE;
	}
	initialized = TRUE;
    }
    return TRUE;
}

/***********************************************************************
 * 				GetOpenFileName			[COMMDLG.1]
 */
BOOL GetOpenFileName(LPOPENFILENAME lpofn)
{
  HANDLE    hDlgTmpl;
  HANDLE    hResInfo;
  HINSTANCE hInst;
  BOOL 	    bRet;
  LPCSTR    dlgTemplate;
  
  if (lpofn == NULL)
    return FALSE;
  if (!FileDlg_Init())
    return FALSE;
  if (lpofn->Flags & OFN_ENABLETEMPLATEHANDLE)
    {
      dlgTemplate = GlobalLock(lpofn->hInstance);
      if (!dlgTemplate)
	{
	  CommDlgLastError = CDERR_LOADRESFAILURE;
	  return FALSE;
	}
    }
  else
    {
      if (lpofn->Flags & OFN_ENABLETEMPLATE)
	{
	  hInst = lpofn->hInstance;
	  hResInfo = FindResource(hInst, lpofn->lpTemplateName, RT_DIALOG);
	  if (hResInfo == 0)
	    {
	      CommDlgLastError = CDERR_FINDRESFAILURE;
	      return FALSE;
	    }
	  hDlgTmpl = LoadResource(hInst, hResInfo);
	  if (hDlgTmpl == 0)
	    {
	      CommDlgLastError = CDERR_LOADRESFAILURE;
	      return FALSE;
	    }
	  dlgTemplate = GlobalLock(hDlgTmpl);
	}
      else
	dlgTemplate = sysres_DIALOG_3;
    }
  hInst = GetWindowWord(lpofn->hwndOwner, GWW_HINSTANCE);
  bRet = DialogBoxIndirectParamPtr(hInst, dlgTemplate, lpofn->hwndOwner,
				   GetWndProcEntry16("FileOpenDlgProc"),
				   (DWORD)lpofn);
  printf("GetOpenFileName // return lpstrFile='%s' !\n", 
	 (LPSTR)PTR_SEG_TO_LIN(lpofn->lpstrFile));
  return bRet;
}


/***********************************************************************
 * 				GetSaveFileName			[COMMDLG.2]
 */
BOOL GetSaveFileName(LPOPENFILENAME lpofn)
{
  HANDLE    hDlgTmpl;
  HANDLE    hResInfo;
  HINSTANCE hInst;
  BOOL	    bRet;
  LPCSTR    dlgTemplate;
  
  if (!FileDlg_Init()) return FALSE;

  if (lpofn == NULL) return FALSE;
  if (lpofn->Flags & OFN_ENABLETEMPLATEHANDLE) {
    dlgTemplate = GlobalLock(lpofn->hInstance);
    if (!dlgTemplate) {
      CommDlgLastError = CDERR_LOADRESFAILURE;
      return FALSE;
    }
  } else {
    if (lpofn->Flags & OFN_ENABLETEMPLATE) {
      hInst = lpofn->hInstance;
      hResInfo = FindResource(hInst, lpofn->lpTemplateName, RT_DIALOG);
      if (hResInfo == 0) {
	CommDlgLastError = CDERR_FINDRESFAILURE;
	return FALSE;
      }
      hDlgTmpl = LoadResource(hInst, hResInfo);
      if (hDlgTmpl == 0) {
	CommDlgLastError = CDERR_LOADRESFAILURE;
	return FALSE;
      }
      dlgTemplate = GlobalLock(hDlgTmpl);
    } else {
      dlgTemplate = sysres_DIALOG_4; /* SAVEFILEDIALOG */
    }
  }
  hInst = GetWindowWord(lpofn->hwndOwner, GWW_HINSTANCE);
  bRet = DialogBoxIndirectParamPtr(hInst, dlgTemplate, lpofn->hwndOwner,
				   GetWndProcEntry16("FileSaveDlgProc"),
				   (DWORD)lpofn); 
  printf("GetSaveFileName // return lpstrFile='%s' !\n", 
	 (LPSTR)PTR_SEG_TO_LIN(lpofn->lpstrFile));
  return bRet;
}

/***********************************************************************
 *                              FILEDLG_StripEditControl        [internal]
 * Strip pathnames off the contents of the edit control.
 */
static void FILEDLG_StripEditControl(HWND hwnd)
{
    char temp[512], *cp;

    SendDlgItemMessage(hwnd, edt1, WM_GETTEXT, 511, MAKE_SEGPTR(temp));
    cp = strrchr(temp, '\\');
    if (cp != NULL) {
	strcpy(temp, cp+1);
    }
    cp = strrchr(temp, ':');
    if (cp != NULL) {
	strcpy(temp, cp+1);
    }
}

/***********************************************************************
 * 				FILEDLG_ScanDir			[internal]
 */
static BOOL FILEDLG_ScanDir(HWND hWnd, LPSTR newPath)
{
  char str[512],str2[512];

  strcpy(str,newPath);
  SendDlgItemMessage(hWnd, edt1, WM_GETTEXT, 511, MAKE_SEGPTR(str2));
  strcat(str, str2);
  if (!DlgDirList(hWnd, str, lst1, 0, 0x0000)) return FALSE;
  DlgDirList(hWnd, "*.*", lst2, stc1, 0x8010);
  
  return TRUE;
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
  return NULL;
}

/***********************************************************************
 *                              FILEDLG_WMDrawItem              [internal]
 */
static LONG FILEDLG_WMDrawItem(HWND hWnd, WORD wParam, LONG lParam)
{
    LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)PTR_SEG_TO_LIN(lParam);
    char str[512];
    HBRUSH hBrush;
    HBITMAP hBitmap, hPrevBitmap;
    BITMAP bm;
    HDC hMemDC;

    strcpy(str, "");
    if (lpdis->CtlType == ODT_LISTBOX && lpdis->CtlID == lst1) {
	hBrush = SelectObject(lpdis->hDC, GetStockObject(LTGRAY_BRUSH));
	SelectObject(lpdis->hDC, hBrush);
	FillRect(lpdis->hDC, &lpdis->rcItem, hBrush);
	SendMessage(lpdis->hwndItem, LB_GETTEXT, lpdis->itemID, 
		    MAKE_SEGPTR(str));
	TextOut(lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top,
		str, strlen(str));
	if (lpdis->itemState != 0) {
	    InvertRect(lpdis->hDC, &lpdis->rcItem);
	}
	return TRUE;
    }
    
    if (lpdis->CtlType == ODT_LISTBOX && lpdis->CtlID == lst2) {
	hBrush = SelectObject(lpdis->hDC, GetStockObject(LTGRAY_BRUSH));
	SelectObject(lpdis->hDC, hBrush);
	FillRect(lpdis->hDC, &lpdis->rcItem, hBrush);
	SendMessage(lpdis->hwndItem, LB_GETTEXT, lpdis->itemID, 
		    MAKE_SEGPTR(str));

	hBitmap = hFolder;
	GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
	TextOut(lpdis->hDC, lpdis->rcItem.left + bm.bmWidth, 
		lpdis->rcItem.top, str, strlen(str));
	hMemDC = CreateCompatibleDC(lpdis->hDC);
	hPrevBitmap = SelectObject(hMemDC, hBitmap);
	BitBlt(lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top,
	       bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
	SelectObject(hMemDC, hPrevBitmap);
	DeleteDC(hMemDC);
	if (lpdis->itemState != 0) {
	    InvertRect(lpdis->hDC, &lpdis->rcItem);
	}
	return TRUE;
    }
    if (lpdis->CtlType == ODT_COMBOBOX && lpdis->CtlID == cmb2) {
	hBrush = SelectObject(lpdis->hDC, GetStockObject(LTGRAY_BRUSH));
	SelectObject(lpdis->hDC, hBrush);
	FillRect(lpdis->hDC, &lpdis->rcItem, hBrush);
	SendMessage(lpdis->hwndItem, CB_GETLBTEXT, lpdis->itemID, 
		    MAKE_SEGPTR(str));
	switch(str[2]) {
	 case 'a': case 'b':
	    hBitmap = hFloppy;
	    break;
	 default:
	    hBitmap = hHDisk;
	    break;
	}
	GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
	TextOut(lpdis->hDC, lpdis->rcItem.left + bm.bmWidth, 
		lpdis->rcItem.top, str, strlen(str));
	hMemDC = CreateCompatibleDC(lpdis->hDC);
	hPrevBitmap = SelectObject(hMemDC, hBitmap);
	BitBlt(lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top,
	       bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
	SelectObject(hMemDC, hPrevBitmap);
	DeleteDC(hMemDC);
	if (lpdis->itemState != 0) {
	    InvertRect(lpdis->hDC, &lpdis->rcItem);
	}
	return TRUE;
    }
    return FALSE;
}

/***********************************************************************
 *                              FILEDLG_WMMeasureItem           [internal]
 */
static LONG FILEDLG_WMMeasureItem(HWND hWnd, WORD wParam, LONG lParam) 
{
    BITMAP bm;
    LPMEASUREITEMSTRUCT lpmeasure;
    
    GetObject(hFolder2, sizeof(BITMAP), (LPSTR)&bm);
    lpmeasure = (LPMEASUREITEMSTRUCT)PTR_SEG_TO_LIN(lParam);
    lpmeasure->itemHeight = bm.bmHeight;
    return TRUE;
}

/***********************************************************************
 *                              FILEDLG_WMInitDialog            [internal]
 */

static LONG FILEDLG_WMInitDialog(HWND hWnd, WORD wParam, LONG lParam) 
{
  int n;
  LPOPENFILENAME lpofn;
  char tmpstr[512];
  LPSTR pstr;
  SetWindowLong(hWnd, DWL_USER, lParam);
  lpofn = (LPOPENFILENAME)lParam;
  /* read custom filter information */
  if (lpofn->lpstrCustomFilter)
    {
      pstr = (LPSTR)PTR_SEG_TO_LIN(lpofn->lpstrCustomFilter);
      while(*pstr)
	{
	  n = strlen(pstr);
	  strcpy(tmpstr, pstr);
	  SendDlgItemMessage(hWnd, cmb1, CB_ADDSTRING, 0, MAKE_SEGPTR(tmpstr));
	  pstr += n + 1;
	  n = strlen(pstr);
	  pstr += n + 1;
	}
    }
  /* read filter information */
  pstr = (LPSTR)PTR_SEG_TO_LIN(lpofn->lpstrFilter);
  while(*pstr)
    {
      n = strlen(pstr);
      strcpy(tmpstr, pstr);
      SendDlgItemMessage(hWnd, cmb1, CB_ADDSTRING, 0, MAKE_SEGPTR(tmpstr));
      pstr += n + 1;
      n = strlen(pstr);
      pstr += n + 1;
    }
  /* set default filter */
  SendDlgItemMessage(hWnd, cmb1, CB_SETCURSEL, 0, 0);    
  strcpy(tmpstr, FILEDLG_GetFileType(PTR_SEG_TO_LIN(lpofn->lpstrCustomFilter),
				     PTR_SEG_TO_LIN(lpofn->lpstrFilter), 0));
  SendDlgItemMessage(hWnd, edt1, WM_SETTEXT, 0, MAKE_SEGPTR(tmpstr));
  /* get drive list */
  *tmpstr = 0;
  DlgDirListComboBox(hWnd, MAKE_SEGPTR(tmpstr), cmb2, 0, 0xC000);
  /* read initial directory */
  if (PTR_SEG_TO_LIN(lpofn->lpstrInitialDir) != NULL) 
    {
      strcpy(tmpstr, PTR_SEG_TO_LIN(lpofn->lpstrInitialDir));
      if (strlen(tmpstr) > 0 && tmpstr[strlen(tmpstr)-1] != '\\' 
	  && tmpstr[strlen(tmpstr)-1] != ':')
	strcat(tmpstr,"\\");
    }
  else
    *tmpstr = 0;
  if (!FILEDLG_ScanDir(hWnd, tmpstr))
    fprintf(stderr, "FileDlg: couldn't read initial directory %s!\n", tmpstr);
  /* select current drive in combo 2 */
  n = DOS_GetDefaultDrive();
  SendDlgItemMessage(hWnd, cmb2, CB_SETCURSEL, n - 1, 0);
  if (!(lpofn->Flags & OFN_SHOWHELP))
    ShowWindow(GetDlgItem(hWnd, pshHelp), SW_HIDE);
  if (lpofn->Flags & OFN_HIDEREADONLY)
    ShowWindow(GetDlgItem(hWnd, chx1), SW_HIDE); 
  return TRUE;
}

/***********************************************************************
 *                              FILEDLG_WMCommand               [internal]
 */
static LONG FILEDLG_WMCommand(HWND hWnd, WORD wParam, LONG lParam) 
{
  LONG lRet;
  LPOPENFILENAME lpofn;
  char tmpstr[512], tmpstr2[512];
  LPSTR pstr, pstr2;
    
  lpofn = (LPOPENFILENAME)GetWindowLong(hWnd, DWL_USER);
  switch (wParam)
    {
    case lst1: /* file list */
      FILEDLG_StripEditControl(hWnd);
      if (HIWORD(lParam) == LBN_DBLCLK)
	goto almost_ok;
      lRet = SendDlgItemMessage(hWnd, lst1, LB_GETCURSEL, 0, 0);
      if (lRet == LB_ERR) return TRUE;
      SendDlgItemMessage(hWnd, lst1, LB_GETTEXT, lRet,
			 MAKE_SEGPTR(tmpstr));
      SendDlgItemMessage(hWnd, edt1, WM_SETTEXT, 0, MAKE_SEGPTR(tmpstr));
      return TRUE;
    case lst2: /* directory list */
      FILEDLG_StripEditControl(hWnd);
      if (HIWORD(lParam) == LBN_DBLCLK)
	{
	  lRet = SendDlgItemMessage(hWnd, lst2, LB_GETCURSEL, 0, 0);
	  if (lRet == LB_ERR) return TRUE;
	  SendDlgItemMessage(hWnd, lst2, LB_GETTEXT, lRet,
			     MAKE_SEGPTR(tmpstr));
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
      if (HIWORD(lParam) == CBN_SELCHANGE) 
	{
	  *tmpstr = 0;
	  goto reset_scan;
	}
      return TRUE;
    case cmb2: /* disk drop list */
      FILEDLG_StripEditControl(hWnd);
      lRet = SendDlgItemMessage(hWnd, cmb2, CB_GETCURSEL, 0, 0L);
      if (lRet == LB_ERR) return 0;
      SendDlgItemMessage(hWnd, cmb2, CB_GETLBTEXT, lRet, MAKE_SEGPTR(tmpstr));
      sprintf(tmpstr, "%c:", tmpstr[2]);
    reset_scan:
      lRet = SendDlgItemMessage(hWnd, cmb1, CB_GETCURSEL, 0, 0);
      if (lRet == LB_ERR)
	return TRUE;
      pstr = FILEDLG_GetFileType(PTR_SEG_TO_LIN(lpofn->lpstrCustomFilter),
				 PTR_SEG_TO_LIN(lpofn->lpstrFilter),
				 lRet);
      strcpy(tmpstr2, pstr); 
      SendDlgItemMessage(hWnd, edt1, WM_SETTEXT, 0, MAKE_SEGPTR(tmpstr2));
      FILEDLG_ScanDir(hWnd, tmpstr);
      return TRUE;
    case chx1:
      return TRUE;
    case pshHelp:
      return TRUE;
    case IDOK:
    almost_ok:
      SendDlgItemMessage(hWnd, edt1, WM_GETTEXT, 511, MAKE_SEGPTR(tmpstr));
      pstr = strrchr(tmpstr, '\\');
      if (pstr == NULL)
	pstr = strrchr(tmpstr, ':');
      if (strchr(tmpstr,'*') != NULL || strchr(tmpstr,'?') != NULL)
	{
	  /* edit control contains wildcards */
	  if (pstr != NULL)
	    {
	      strcpy(tmpstr2, pstr+1);
	      *(pstr+1) = 0;
	    }
	  else
	    {
	      strcpy(tmpstr2, tmpstr);
	      strcpy(tmpstr, "");
	    }
	  printf("commdlg: %s, %s\n", tmpstr, tmpstr2);
	  SendDlgItemMessage(hWnd, edt1, WM_SETTEXT, 0, MAKE_SEGPTR(tmpstr2));
	  FILEDLG_ScanDir(hWnd, tmpstr);
	  return TRUE;
	}
      /* no wildcards, we might have a directory or a filename */
      /* try appending a wildcard and reading the directory */
      pstr2 = tmpstr + strlen(tmpstr);
      if (pstr == NULL || *(pstr+1) != 0)
	strcat(tmpstr, "\\");
      lRet = SendDlgItemMessage(hWnd, cmb1, CB_GETCURSEL, 0, 0);
      if (lRet == LB_ERR) return TRUE;
      strcpy(tmpstr2, 
	     FILEDLG_GetFileType(PTR_SEG_TO_LIN(lpofn->lpstrCustomFilter),
				 PTR_SEG_TO_LIN(lpofn->lpstrFilter),
				 lRet));
      SendDlgItemMessage(hWnd, edt1, WM_SETTEXT, 0, MAKE_SEGPTR(tmpstr2));
      /* if ScanDir succeeds, we have changed the directory */
      if (FILEDLG_ScanDir(hWnd, tmpstr)) return TRUE;
      /* if not, this must be a filename */
      *pstr2 = 0;
      if (pstr != NULL)
	{
	  /* strip off the pathname */
	  *pstr = 0;
	  strcpy(tmpstr2, pstr+1);
	  SendDlgItemMessage(hWnd, edt1, WM_SETTEXT, 0, MAKE_SEGPTR(tmpstr2));
	  /* Should we MessageBox() if this fails? */
	  if (!FILEDLG_ScanDir(hWnd, tmpstr)) return TRUE;
	  strcpy(tmpstr, tmpstr2);
	}
      else 
	SendDlgItemMessage(hWnd, edt1, WM_SETTEXT, 0, MAKE_SEGPTR(tmpstr));
      ShowWindow(hWnd, SW_HIDE);
      {
	int drive;
	drive = DOS_GetDefaultDrive();
	tmpstr2[0] = 'A'+ drive;
	tmpstr2[1] = ':';
	tmpstr2[2] = '\\';
	strcpy(tmpstr2 + 3, DOS_GetCurrentDir(drive));
	if (strlen(tmpstr2) > 3)
	  strcat(tmpstr2, "\\");
	strcat(tmpstr2, tmpstr);
	strcpy(PTR_SEG_TO_LIN(lpofn->lpstrFile), tmpstr2);
      }
      lpofn->nFileOffset = 0;
      lpofn->nFileExtension = strlen(PTR_SEG_TO_LIN(lpofn->lpstrFile)) - 3;
      if (PTR_SEG_TO_LIN(lpofn->lpstrFileTitle) != NULL) 
	{
	  lRet = SendDlgItemMessage(hWnd, lst1, LB_GETCURSEL, 0, 0);
	  SendDlgItemMessage(hWnd, lst1, LB_GETTEXT, lRet,
			     MAKE_SEGPTR(tmpstr));
	  strcpy(PTR_SEG_TO_LIN(lpofn->lpstrFileTitle), tmpstr);
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
 * 				FileOpenDlgProc			[COMMDLG.6]
 */

BOOL FileOpenDlgProc(HWND hWnd, WORD wMsg, WORD wParam, LONG lParam)
{  
  switch (wMsg)
    {
    case WM_INITDIALOG:
      return FILEDLG_WMInitDialog(hWnd, wParam, lParam);
    case WM_MEASUREITEM:
      return FILEDLG_WMMeasureItem(hWnd, wParam, lParam);
    case WM_DRAWITEM:
      return FILEDLG_WMDrawItem(hWnd, wParam, lParam);
    case WM_COMMAND:
      return FILEDLG_WMCommand(hWnd, wParam, lParam);
#if 0
    case WM_CTLCOLOR:
      SetBkColor((HDC)wParam, 0x00C0C0C0);
      switch (HIWORD(lParam))
	{
	case CTLCOLOR_BTN:
	  SetTextColor((HDC)wParam, 0x00000000);
	  return hGRAYBrush;
	case CTLCOLOR_STATIC:
	  SetTextColor((HDC)wParam, 0x00000000);
	  return hGRAYBrush;
	}
      break;
#endif
    }
  return FALSE;
}


/***********************************************************************
 * 				FileSaveDlgProc			[COMMDLG.7]
 */
BOOL FileSaveDlgProc(HWND hWnd, WORD wMsg, WORD wParam, LONG lParam)
{
  switch (wMsg) {
   case WM_INITDIALOG:
      return FILEDLG_WMInitDialog(hWnd, wParam, lParam);
      
   case WM_MEASUREITEM:
      return FILEDLG_WMMeasureItem(hWnd, wParam, lParam);
    
   case WM_DRAWITEM:
      return FILEDLG_WMDrawItem(hWnd, wParam, lParam);

   case WM_COMMAND:
      return FILEDLG_WMCommand(hWnd, wParam, lParam);
  }
  
  /*
  case WM_CTLCOLOR:
   SetBkColor((HDC)wParam, 0x00C0C0C0);
   switch (HIWORD(lParam))
   {
    case CTLCOLOR_BTN:
     SetTextColor((HDC)wParam, 0x00000000);
     return hGRAYBrush;
    case CTLCOLOR_STATIC:
     SetTextColor((HDC)wParam, 0x00000000);
     return hGRAYBrush;
   }
   return FALSE;
   
   */
  return FALSE;
}


/***********************************************************************
 * 				ChooseColor				[COMMDLG.5]
 */
BOOL ChooseColor(LPCHOOSECOLOR lpChCol)
{
        WND     *wndPtr;
	BOOL	bRet;
        wndPtr = WIN_FindWndPtr(lpChCol->hwndOwner);
	bRet = DialogBoxIndirectParamPtr(wndPtr->hInstance, sysres_DIALOG_8,
		lpChCol->hwndOwner, GetWndProcEntry16("ColorDlgProc"), 
		(DWORD)lpChCol);
	return bRet;
}


/***********************************************************************
 * 				ColorDlgProc			[COMMDLG.8]
 */
BOOL ColorDlgProc(HWND hWnd, WORD wMsg, WORD wParam, LONG lParam)
{
  switch (wMsg) 
    {
    case WM_INITDIALOG:
      printf("ColorDlgProc // WM_INITDIALOG lParam=%08lX\n", lParam);
      ShowWindow(hWnd, SW_SHOWNORMAL);
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
 * 				FindTextDlg				[COMMDLG.11]
 */
BOOL FindText(LPFINDREPLACE lpFind)
{
  WND    *wndPtr;
  BOOL   bRet;
  LPCSTR lpTemplate;
  
  lpTemplate = sysres_DIALOG_9;
  wndPtr = WIN_FindWndPtr(lpFind->hwndOwner);
  bRet = DialogBoxIndirectParamPtr(wndPtr->hInstance, lpTemplate,
				lpFind->hwndOwner, GetWndProcEntry16("FindTextDlgProc"),
				(DWORD)lpFind);
  return bRet;
}


/***********************************************************************
 * 				ReplaceTextDlg			[COMMDLG.12]
 */
BOOL ReplaceText(LPFINDREPLACE lpFind)
{
  WND    *wndPtr;
  BOOL   bRet;
  LPCSTR lpTemplate;

  lpTemplate = sysres_DIALOG_10;
  wndPtr = WIN_FindWndPtr(lpFind->hwndOwner);
  bRet = DialogBoxIndirectParamPtr(wndPtr->hInstance, lpTemplate,
				   lpFind->hwndOwner, GetWndProcEntry16("ReplaceTextDlgProc"),
				   (DWORD)lpFind);
  return bRet;
}


/***********************************************************************
 * 				FindTextDlgProc			[COMMDLG.13]
 */
BOOL FindTextDlgProc(HWND hWnd, WORD wMsg, WORD wParam, LONG lParam)
{
  switch (wMsg)
    {
    case WM_INITDIALOG:
      printf("FindTextDlgProc // WM_INITDIALOG lParam=%08lX\n", lParam);
      ShowWindow(hWnd, SW_SHOWNORMAL);
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
 * 				ReplaceTextDlgProc		[COMMDLG.14]
 */
BOOL ReplaceTextDlgProc(HWND hWnd, WORD wMsg, WORD wParam, LONG lParam)
{
  switch (wMsg)
    {
    case WM_INITDIALOG:
      printf("ReplaceTextDlgProc // WM_INITDIALOG lParam=%08lX\n", lParam);
      ShowWindow(hWnd, SW_SHOWNORMAL);
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
 * 				PrintDlg				[COMMDLG.20]
 */
BOOL PrintDlg(LPPRINTDLG lpPrint)
{
  WND    *wndPtr;
  BOOL   bRet;
  LPCSTR lpTemplate;
  
  printf("PrintDlg(%p) // Flags=%08lX\n", lpPrint, lpPrint->Flags);
  if (lpPrint->Flags & PD_PRINTSETUP)  {
    lpTemplate = sysres_DIALOG_6;
  } else  {
    lpTemplate = sysres_DIALOG_5;
  }
  wndPtr = WIN_FindWndPtr(lpPrint->hwndOwner);
  if (lpPrint->Flags & PD_PRINTSETUP)
  bRet = DialogBoxIndirectParamPtr(wndPtr->hInstance, lpTemplate,
				   lpPrint->hwndOwner, GetWndProcEntry16("PrintSetupDlgProc"),
				   (DWORD)lpPrint);
  else
  bRet = DialogBoxIndirectParamPtr(wndPtr->hInstance, lpTemplate,
				   lpPrint->hwndOwner, GetWndProcEntry16("PrintDlgProc"),
				   (DWORD)lpPrint);
  return bRet;
}


/***********************************************************************
 * 				PrintDlgProc			[COMMDLG.21]
 */
BOOL PrintDlgProc(HWND hWnd, WORD wMsg, WORD wParam, LONG lParam)
{
  switch (wMsg)
    {
    case WM_INITDIALOG:
      printf("PrintDlgProc // WM_INITDIALOG lParam=%08lX\n", lParam);
      ShowWindow(hWnd, SW_SHOWNORMAL);
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
 * 				PrintSetupDlgProc		[COMMDLG.22]
 */
BOOL PrintSetupDlgProc(HWND hWnd, WORD wMsg, WORD wParam, LONG lParam)
{
  switch (wMsg)
    {
    case WM_INITDIALOG:
      printf("PrintSetupDlgProc // WM_INITDIALOG lParam=%08lX\n", lParam);
      ShowWindow(hWnd, SW_SHOWNORMAL);
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
 * 				CommDlgExtendError		[COMMDLG.26]
 */
DWORD CommDlgExtendError(void)
{
  return CommDlgLastError;
}


/***********************************************************************
 * 				GetFileTitle			[COMMDLG.27]
 */
int GetFileTitle(LPCSTR lpFile, LPSTR lpTitle, UINT cbBuf)
{
  int i, len;
  printf("GetFileTitle(%p %p %d); \n", lpFile, lpTitle, cbBuf);
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
  printf("\n---> '%s' ", &lpFile[i]);
  len = min(cbBuf, strlen(&lpFile[i]) + 1);
  strncpy(lpTitle, &lpFile[i], len + 1);
  if (len != cbBuf)
    return len;
  else
    return 0;
}
