/*
 * COMMDLG functions
 *
 * Copyright 1994 Martin Ayotte
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "win.h"
#include "user.h"
#include "message.h"
#include "commdlg.h"
#include "dlgs.h"
#include "selectors.h"
#include "resource.h"
#include "dos_fs.h"
#include "drive.h"
#include "stackframe.h"
#include "stddebug.h"
#include "debug.h"

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
 *           GetOpenFileName   (COMMDLG.1)
 */
BOOL GetOpenFileName(LPOPENFILENAME lpofn)
{
    HINSTANCE hInst;
    HANDLE hDlgTmpl, hResInfo;
    BOOL bRet;
  
    if (!lpofn || !FileDlg_Init()) return FALSE;

    if (lpofn->Flags & OFN_ENABLETEMPLATEHANDLE) hDlgTmpl = lpofn->hInstance;
    else if (lpofn->Flags & OFN_ENABLETEMPLATE)
    {
        if (!(hResInfo = FindResource( lpofn->hInstance,
                                       lpofn->lpTemplateName, RT_DIALOG)))
        {
            CommDlgLastError = CDERR_FINDRESFAILURE;
            return FALSE;
        }
        hDlgTmpl = LoadResource( lpofn->hInstance, hResInfo );
    }
    else hDlgTmpl = SYSRES_LoadResource( SYSRES_DIALOG_OPEN_FILE );
    if (!hDlgTmpl)
    {
        CommDlgLastError = CDERR_LOADRESFAILURE;
        return FALSE;
    }

    hInst = WIN_GetWindowInstance( lpofn->hwndOwner );
    bRet = DialogBoxIndirectParam( hInst, hDlgTmpl, lpofn->hwndOwner,
                                   GetWndProcEntry16("FileOpenDlgProc"),
                                   (DWORD)lpofn );

    if (!(lpofn->Flags & OFN_ENABLETEMPLATEHANDLE))
    {
        if (lpofn->Flags & OFN_ENABLETEMPLATE) FreeResource( hDlgTmpl );
        else SYSRES_FreeResource( hDlgTmpl );
    }

    dprintf_commdlg(stddeb,"GetOpenFileName // return lpstrFile='%s' !\n", 
           (LPSTR)PTR_SEG_TO_LIN(lpofn->lpstrFile));
    return bRet;
}


/***********************************************************************
 *           GetSaveFileName   (COMMDLG.2)
 */
BOOL GetSaveFileName(LPOPENFILENAME lpofn)
{
    HINSTANCE hInst;
    HANDLE hDlgTmpl, hResInfo;
    BOOL bRet;
  
    if (!lpofn || !FileDlg_Init()) return FALSE;

    if (lpofn->Flags & OFN_ENABLETEMPLATEHANDLE) hDlgTmpl = lpofn->hInstance;
    else if (lpofn->Flags & OFN_ENABLETEMPLATE)
    {
        hInst = lpofn->hInstance;
        if (!(hResInfo = FindResource( lpofn->hInstance,
                                       lpofn->lpTemplateName, RT_DIALOG )))
        {
            CommDlgLastError = CDERR_FINDRESFAILURE;
            return FALSE;
        }
        hDlgTmpl = LoadResource( lpofn->hInstance, hResInfo );
    }
    else hDlgTmpl = SYSRES_LoadResource( SYSRES_DIALOG_SAVE_FILE );

    hInst = WIN_GetWindowInstance( lpofn->hwndOwner );
    bRet = DialogBoxIndirectParam( hInst, hDlgTmpl, lpofn->hwndOwner,
                                   GetWndProcEntry16("FileSaveDlgProc"),
                                   (DWORD)lpofn); 
    if (!(lpofn->Flags & OFN_ENABLETEMPLATEHANDLE))
    {
        if (lpofn->Flags & OFN_ENABLETEMPLATE) FreeResource( hDlgTmpl );
        else SYSRES_FreeResource( hDlgTmpl );
    }

    dprintf_commdlg(stddeb, "GetSaveFileName // return lpstrFile='%s' !\n", 
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

    SendDlgItemMessage(hwnd, edt1, WM_GETTEXT, 511, (LPARAM)MAKE_SEGPTR(temp));
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

  strncpy(str,newPath,511); str[511]=0;
  SendDlgItemMessage(hWnd, edt1, WM_GETTEXT, 511, (LPARAM)MAKE_SEGPTR(str2));
  strncat(str,str2,511-strlen(str)); str[511]=0;
  if (!DlgDirList(hWnd, MAKE_SEGPTR(str), lst1, 0, 0x0000)) return FALSE;
  strcpy( str, "*.*" );
  DlgDirList(hWnd, MAKE_SEGPTR(str), lst2, stc1, 0x8010);
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
static LONG FILEDLG_WMDrawItem(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)PTR_SEG_TO_LIN(lParam);
    char str[512];
    HBRUSH hBrush;
    HBITMAP hBitmap, hPrevBitmap;
    BITMAP bm;
    HDC hMemDC;

    str[0]=0;
    if (lpdis->CtlType == ODT_LISTBOX && lpdis->CtlID == lst1) {
	hBrush = SelectObject(lpdis->hDC, GetStockObject(LTGRAY_BRUSH));
	SelectObject(lpdis->hDC, hBrush);
	FillRect(lpdis->hDC, &lpdis->rcItem, hBrush);
	SendMessage(lpdis->hwndItem, LB_GETTEXT, lpdis->itemID, 
		    (LPARAM)MAKE_SEGPTR(str));
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
		    (LPARAM)MAKE_SEGPTR(str));

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
		    (LPARAM)MAKE_SEGPTR(str));
        switch(DRIVE_GetType( str[2] - 'a' ))
        {
        case TYPE_FLOPPY:  hBitmap = hFloppy; break;
        case TYPE_CDROM:   hBitmap = hCDRom; break;
        case TYPE_HD:
        case TYPE_NETWORK:
        default:           hBitmap = hHDisk; break;
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
static LONG FILEDLG_WMMeasureItem(HWND hWnd, WPARAM wParam, LPARAM lParam) 
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

static LONG FILEDLG_WMInitDialog(HWND hWnd, WPARAM wParam, LPARAM lParam) 
{
  int n;
  int i;
  LPOPENFILENAME lpofn;
  char tmpstr[512];
  LPSTR pstr;
  SetWindowLong(hWnd, DWL_USER, lParam);
  lpofn = (LPOPENFILENAME)lParam;
  /* read custom filter information */
  if (lpofn->lpstrCustomFilter)
    {
      pstr = (LPSTR)PTR_SEG_TO_LIN(lpofn->lpstrCustomFilter);
      dprintf_commdlg(stddeb,"lpstrCustomFilter = %p\n", pstr);
      while(*pstr)
	{
	  n = strlen(pstr);
	  strncpy(tmpstr, pstr, 511); tmpstr[511]=0;
	  dprintf_commdlg(stddeb,"lpstrCustomFilter // add tmpstr='%s' ", tmpstr);
          i = SendDlgItemMessage(hWnd, cmb1, CB_ADDSTRING, 0, (LPARAM)MAKE_SEGPTR(tmpstr));
	  pstr += n + 1;
	  n = strlen(pstr);
	  dprintf_commdlg(stddeb,"associated to '%s'\n", pstr);
          SendDlgItemMessage(hWnd, cmb1, CB_SETITEMDATA, i, (LPARAM)pstr);
	  pstr += n + 1;
	}
    }
  /* read filter information */
  pstr = (LPSTR)PTR_SEG_TO_LIN(lpofn->lpstrFilter);
  while(*pstr)
    {
      n = strlen(pstr);
      strncpy(tmpstr, pstr, 511); tmpstr[511]=0;
      dprintf_commdlg(stddeb,"lpstrFilter // add tmpstr='%s' ", tmpstr);
      i = SendDlgItemMessage(hWnd, cmb1, CB_ADDSTRING, 0, (LPARAM)MAKE_SEGPTR(tmpstr));
      pstr += n + 1;
      n = strlen(pstr);
      dprintf_commdlg(stddeb,"associated to '%s'\n", pstr);
      SendDlgItemMessage(hWnd, cmb1, CB_SETITEMDATA, i, (LPARAM)pstr);
      pstr += n + 1;
    }
  /* set default filter */
  if (lpofn->nFilterIndex == 0 && lpofn->lpstrCustomFilter == (SEGPTR)NULL)
  	lpofn->nFilterIndex = 1;
  SendDlgItemMessage(hWnd, cmb1, CB_SETCURSEL, lpofn->nFilterIndex - 1, 0);    
  strncpy(tmpstr, FILEDLG_GetFileType(PTR_SEG_TO_LIN(lpofn->lpstrCustomFilter),
	     PTR_SEG_TO_LIN(lpofn->lpstrFilter), lpofn->nFilterIndex - 1),511);
  tmpstr[511]=0;
  dprintf_commdlg(stddeb,"nFilterIndex = %ld // SetText of edt1 to '%s'\n", 
  			lpofn->nFilterIndex, tmpstr);
  SendDlgItemMessage(hWnd, edt1, WM_SETTEXT, 0, (LPARAM)MAKE_SEGPTR(tmpstr));
  /* get drive list */
  *tmpstr = 0;
  DlgDirListComboBox(hWnd, MAKE_SEGPTR(tmpstr), cmb2, 0, 0xC000);
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
  if (!FILEDLG_ScanDir(hWnd, tmpstr))
    fprintf(stderr, "FileDlg: couldn't read initial directory %s!\n", tmpstr);
  /* select current drive in combo 2 */
  n = DRIVE_GetCurrentDrive();
  SendDlgItemMessage(hWnd, cmb2, CB_SETCURSEL, n, 0);
  if (!(lpofn->Flags & OFN_SHOWHELP))
    ShowWindow(GetDlgItem(hWnd, pshHelp), SW_HIDE);
  if (lpofn->Flags & OFN_HIDEREADONLY)
    ShowWindow(GetDlgItem(hWnd, chx1), SW_HIDE); 
  return TRUE;
}

/***********************************************************************
 *                              FILEDLG_WMCommand               [internal]
 */
static LRESULT FILEDLG_WMCommand(HWND hWnd, WPARAM wParam, LPARAM lParam) 
{
  LONG lRet;
  LPOPENFILENAME lpofn;
  char tmpstr[512], tmpstr2[512];
  LPSTR pstr, pstr2;
  UINT control,notification;

  /* Notifications are packaged differently in Win32 */
#ifdef WINELIB32
  control = LOWORD(wParam);
  notification = HIWORD(wParam);
#else
  control = wParam;
  notification = HIWORD(lParam);
#endif
    
  lpofn = (LPOPENFILENAME)GetWindowLong(hWnd, DWL_USER);
  switch (control)
    {
    case lst1: /* file list */
      FILEDLG_StripEditControl(hWnd);
      if (notification == LBN_DBLCLK)
	goto almost_ok;
      lRet = SendDlgItemMessage(hWnd, lst1, LB_GETCURSEL, 0, 0);
      if (lRet == LB_ERR) return TRUE;
      SendDlgItemMessage(hWnd, lst1, LB_GETTEXT, lRet,
			 (LPARAM)MAKE_SEGPTR(tmpstr));
      SendDlgItemMessage(hWnd, edt1, WM_SETTEXT, 0, (LPARAM)MAKE_SEGPTR(tmpstr));
      return TRUE;
    case lst2: /* directory list */
      FILEDLG_StripEditControl(hWnd);
      if (notification == LBN_DBLCLK)
	{
	  lRet = SendDlgItemMessage(hWnd, lst2, LB_GETCURSEL, 0, 0);
	  if (lRet == LB_ERR) return TRUE;
	  SendDlgItemMessage(hWnd, lst2, LB_GETTEXT, lRet,
			     (LPARAM)MAKE_SEGPTR(tmpstr));
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
    case cmb2: /* disk drop list */
      FILEDLG_StripEditControl(hWnd);
      lRet = SendDlgItemMessage(hWnd, cmb2, CB_GETCURSEL, 0, 0L);
      if (lRet == LB_ERR) return 0;
      SendDlgItemMessage(hWnd, cmb2, CB_GETLBTEXT, lRet, (LPARAM)MAKE_SEGPTR(tmpstr));
      sprintf(tmpstr, "%c:", tmpstr[2]);
    reset_scan:
      lRet = SendDlgItemMessage(hWnd, cmb1, CB_GETCURSEL, 0, 0);
      if (lRet == LB_ERR)
	return TRUE;
      pstr = (LPSTR)SendDlgItemMessage(hWnd, cmb1, CB_GETITEMDATA, lRet, 0);
      dprintf_commdlg(stddeb,"Selected filter : %s\n", pstr);
      strncpy(tmpstr2, pstr, 511); tmpstr2[511]=0;
      SendDlgItemMessage(hWnd, edt1, WM_SETTEXT, 0, (LPARAM)MAKE_SEGPTR(tmpstr2));
      FILEDLG_ScanDir(hWnd, tmpstr);
      return TRUE;
    case chx1:
      return TRUE;
    case pshHelp:
      return TRUE;
    case IDOK:
    almost_ok:
      SendDlgItemMessage(hWnd, edt1, WM_GETTEXT, 511, (LPARAM)MAKE_SEGPTR(tmpstr));
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
	  dprintf_commdlg(stddeb,"commdlg: %s, %s\n", tmpstr, tmpstr2);
	  SendDlgItemMessage(hWnd, edt1, WM_SETTEXT, 0, (LPARAM)MAKE_SEGPTR(tmpstr2));
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
      lpofn->nFilterIndex = lRet + 1;
      dprintf_commdlg(stddeb,"commdlg: lpofn->nFilterIndex=%ld\n", lpofn->nFilterIndex);
      strncpy(tmpstr2, 
	     FILEDLG_GetFileType(PTR_SEG_TO_LIN(lpofn->lpstrCustomFilter),
				 PTR_SEG_TO_LIN(lpofn->lpstrFilter),
				 lRet), 511);
      tmpstr2[511]=0;
      SendDlgItemMessage(hWnd, edt1, WM_SETTEXT, 0, (LPARAM)MAKE_SEGPTR(tmpstr2));
      /* if ScanDir succeeds, we have changed the directory */
      if (FILEDLG_ScanDir(hWnd, tmpstr)) return TRUE;
      /* if not, this must be a filename */
      *pstr2 = 0;
      if (pstr != NULL)
	{
	  /* strip off the pathname */
	  *pstr = 0;
	  strncpy(tmpstr2, pstr+1, 511); tmpstr2[511]=0;
	  SendDlgItemMessage(hWnd, edt1, WM_SETTEXT, 0, (LPARAM)MAKE_SEGPTR(tmpstr2));
	  /* Should we MessageBox() if this fails? */
	  if (!FILEDLG_ScanDir(hWnd, tmpstr)) return TRUE;
	  strcpy(tmpstr, tmpstr2);
	}
      else 
	SendDlgItemMessage(hWnd, edt1, WM_SETTEXT, 0, (LPARAM)MAKE_SEGPTR(tmpstr));
      ShowWindow(hWnd, SW_HIDE);
      {
	int drive = DRIVE_GetCurrentDrive();
	tmpstr2[0] = 'A'+ drive;
	tmpstr2[1] = ':';
	tmpstr2[2] = '\\';
	strncpy(tmpstr2 + 3, DRIVE_GetDosCwd(drive), 507); tmpstr2[510]=0;
	if (strlen(tmpstr2) > 3)
	   strcat(tmpstr2, "\\");
	strncat(tmpstr2, tmpstr, 511-strlen(tmpstr2)); tmpstr2[511]=0;
	strcpy(PTR_SEG_TO_LIN(lpofn->lpstrFile), tmpstr2);
      }
      lpofn->nFileOffset = 0;
      lpofn->nFileExtension = 0;
      while(tmpstr2[lpofn->nFileExtension] != '.' && tmpstr2[lpofn->nFileExtension] != '\0')
        lpofn->nFileExtension++;
      if (lpofn->nFileExtension == '\0')
	lpofn->nFileExtension = 0;
      else
	lpofn->nFileExtension++;
      if (PTR_SEG_TO_LIN(lpofn->lpstrFileTitle) != NULL) 
	{
	  lRet = SendDlgItemMessage(hWnd, lst1, LB_GETCURSEL, 0, 0);
	  SendDlgItemMessage(hWnd, lst1, LB_GETTEXT, lRet,
			     (LPARAM)MAKE_SEGPTR(tmpstr));
          dprintf_commdlg(stddeb,"strcpy'ing '%s'\n",tmpstr); fflush(stdout);
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
 *           FileOpenDlgProc   (COMMDLG.6)
 */
LRESULT FileOpenDlgProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
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
 *           FileSaveDlgProc   (COMMDLG.7)
 */
LRESULT FileSaveDlgProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
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
 *           ChooseColor   (COMMDLG.5)
 */
BOOL ChooseColor(LPCHOOSECOLOR lpChCol)
{
    HANDLE hInst, hDlgTmpl;
    BOOL bRet;

    hDlgTmpl = SYSRES_LoadResource( SYSRES_DIALOG_CHOOSE_COLOR );
    hInst = WIN_GetWindowInstance( lpChCol->hwndOwner );
    bRet = DialogBoxIndirectParam( hInst, hDlgTmpl, lpChCol->hwndOwner,
                                   GetWndProcEntry16("ColorDlgProc"), 
                                   (DWORD)lpChCol );
    SYSRES_FreeResource( hDlgTmpl );
    return bRet;
}


/***********************************************************************
 *           ColorDlgProc   (COMMDLG.8)
 */
LRESULT ColorDlgProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
  switch (wMsg) 
    {
    case WM_INITDIALOG:
      dprintf_commdlg(stddeb,"ColorDlgProc // WM_INITDIALOG lParam=%08lX\n", lParam);
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
 *           FindTextDlg   (COMMDLG.11)
 */
BOOL FindText(LPFINDREPLACE lpFind)
{
    HANDLE hInst, hDlgTmpl;
    BOOL bRet;

    hDlgTmpl = SYSRES_LoadResource( SYSRES_DIALOG_FIND_TEXT );
    hInst = WIN_GetWindowInstance( lpFind->hwndOwner );
    bRet = DialogBoxIndirectParam( hInst, hDlgTmpl, lpFind->hwndOwner,
                                   GetWndProcEntry16("FindTextDlgProc"), 
                                   (DWORD)lpFind );
    SYSRES_FreeResource( hDlgTmpl );
    return bRet;
}


/***********************************************************************
 *           ReplaceTextDlg   (COMMDLG.12)
 */
BOOL ReplaceText(LPFINDREPLACE lpFind)
{
    HANDLE hInst, hDlgTmpl;
    BOOL bRet;

    hDlgTmpl = SYSRES_LoadResource( SYSRES_DIALOG_REPLACE_TEXT );
    hInst = WIN_GetWindowInstance( lpFind->hwndOwner );
    bRet = DialogBoxIndirectParam( hInst, hDlgTmpl, lpFind->hwndOwner,
                                   GetWndProcEntry16("ReplaceTextDlgProc"), 
                                   (DWORD)lpFind );
    SYSRES_FreeResource( hDlgTmpl );
    return bRet;
}


/***********************************************************************
 *           FindTextDlgProc   (COMMDLG.13)
 */
LRESULT FindTextDlgProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
  switch (wMsg)
    {
    case WM_INITDIALOG:
      dprintf_commdlg(stddeb,"FindTextDlgProc // WM_INITDIALOG lParam=%08lX\n", lParam);
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
 *           ReplaceTextDlgProc   (COMMDLG.14)
 */
LRESULT ReplaceTextDlgProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
  switch (wMsg)
    {
    case WM_INITDIALOG:
      dprintf_commdlg(stddeb,"ReplaceTextDlgProc // WM_INITDIALOG lParam=%08lX\n", lParam);
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
 *           PrintDlg   (COMMDLG.20)
 */
BOOL PrintDlg(LPPRINTDLG lpPrint)
{
    HANDLE hInst, hDlgTmpl;
    BOOL bRet;

    dprintf_commdlg(stddeb,"PrintDlg(%p) // Flags=%08lX\n", lpPrint, lpPrint->Flags );

    if (lpPrint->Flags & PD_RETURNDEFAULT)
        /* FIXME: should fill lpPrint->hDevMode and lpPrint->hDevNames here */
        return TRUE;

    if (lpPrint->Flags & PD_PRINTSETUP)
	hDlgTmpl = SYSRES_LoadResource( SYSRES_DIALOG_PRINT_SETUP );
    else
	hDlgTmpl = SYSRES_LoadResource( SYSRES_DIALOG_PRINT );

    hInst = WIN_GetWindowInstance( lpPrint->hwndOwner );
    bRet = DialogBoxIndirectParam( hInst, hDlgTmpl, lpPrint->hwndOwner,
                                   (lpPrint->Flags & PD_PRINTSETUP) ?
                                       GetWndProcEntry16("PrintSetupDlgProc") :
                                       GetWndProcEntry16("PrintDlgProc"),
                                   (DWORD)lpPrint );
    SYSRES_FreeResource( hDlgTmpl );
    return bRet;
}


/***********************************************************************
 *           PrintDlgProc   (COMMDLG.21)
 */
LRESULT PrintDlgProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
  switch (wMsg)
    {
    case WM_INITDIALOG:
      dprintf_commdlg(stddeb,"PrintDlgProc // WM_INITDIALOG lParam=%08lX\n", lParam);
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
 *           PrintSetupDlgProc   (COMMDLG.22)
 */
LRESULT PrintSetupDlgProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
  switch (wMsg)
    {
    case WM_INITDIALOG:
      dprintf_commdlg(stddeb,"PrintSetupDlgProc // WM_INITDIALOG lParam=%08lX\n", lParam);
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
 *           CommDlgExtendedError   (COMMDLG.26)
 */
DWORD CommDlgExtendedError(void)
{
  return CommDlgLastError;
}


/***********************************************************************
 *           GetFileTitle   (COMMDLG.27)
 */
short GetFileTitle(LPCSTR lpFile, LPSTR lpTitle, UINT cbBuf)
{
    int i, len;
    dprintf_commdlg(stddeb,"GetFileTitle(%p %p %d); \n", lpFile, lpTitle, cbBuf);
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
    dprintf_commdlg(stddeb,"\n---> '%s' ", &lpFile[i]);
    
    len = strlen(lpFile+i)+1;
    if (cbBuf < len)
    	return len;

    strncpy(lpTitle, &lpFile[i], len);
    return 0;
}
