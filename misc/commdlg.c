/*
 * COMMDLG functions
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Albrecht Kleine
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "win.h"
#include "user.h"
#include "message.h"
#include "commdlg.h"
#include "dlgs.h"
#include "module.h"
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
static  HBITMAP 	hBitmapTT = 0;

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
                                   MODULE_GetWndProcEntry16("FileOpenDlgProc"),
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
                                   MODULE_GetWndProcEntry16("FileSaveDlgProc"),
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
 *                              FILEDLG_HookCallChk             [internal]
 */
static int FILEDLG_HookCallChk(LPOPENFILENAME lpofn)
{
 if (lpofn)
  if (lpofn->Flags & OFN_ENABLEHOOK)
   if (lpofn->lpfnHook)
    return 1;
 return 0;   
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
  if (lpofn->lpstrTitle)
      SendMessage( hWnd, WM_SETTEXT, 0, (LPARAM)lpofn->lpstrTitle );
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
  if (FILEDLG_HookCallChk(lpofn))
     return (BOOL)CallWindowProc(lpofn->lpfnHook, 
               hWnd,  WM_INITDIALOG, wParam,(LPARAM)MAKE_SEGPTR(lpofn));
  else  
     return TRUE;
}

/***********************************************************************
 *                              FILEDLG_WMCommand               [internal]
 */
static LRESULT FILEDLG_WMCommand(HWND hWnd, WPARAM wParam, LPARAM lParam) 
{
  LONG lRet;
  LPOPENFILENAME lpofn;
  OPENFILENAME ofn2;
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

      if (FILEDLG_HookCallChk(lpofn))
       CallWindowProc (lpofn->lpfnHook, hWnd,
                  RegisterWindowMessage(MAKE_SEGPTR(LBSELCHSTRING)), 
                  control, MAKELONG(lRet,CD_LBSELCHANGE));       
      /* FIXME: for OFN_ALLOWMULTISELECT we need CD_LBSELSUB, CD_SELADD, CD_LBSELNOITEMS */                  
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
      ofn2=*lpofn; /* for later restoring */
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
#if 0
      ShowWindow(hWnd, SW_HIDE);   /* this should not be necessary ?! (%%%) */
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
      if (FILEDLG_HookCallChk(lpofn))
      {
       lRet= (BOOL)CallWindowProc (lpofn->lpfnHook,
               hWnd, RegisterWindowMessage(MAKE_SEGPTR(FILEOKSTRING)), 
               0, (LPARAM)MAKE_SEGPTR(lpofn));                        
       if (lRet)       
       {
         *lpofn=ofn2; /* restore old state */
#if 0
         ShowWindow(hWnd, SW_SHOW);               /* only if above (%%%) SW_HIDE used */
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
LRESULT FileOpenDlgProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{  
 LPOPENFILENAME lpofn = (LPOPENFILENAME)GetWindowLong(hWnd, DWL_USER);
 
 if (wMsg!=WM_INITDIALOG)
  if (FILEDLG_HookCallChk(lpofn))
  {
   LRESULT  lRet=(BOOL)CallWindowProc(lpofn->lpfnHook, hWnd, wMsg, wParam, lParam);
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
 LPOPENFILENAME lpofn = (LPOPENFILENAME)GetWindowLong(hWnd, DWL_USER);
 
 if (wMsg!=WM_INITDIALOG)
  if (FILEDLG_HookCallChk(lpofn))
  {
   LRESULT  lRet=(BOOL)CallWindowProc(lpofn->lpfnHook, hWnd, wMsg, wParam, lParam);
   if (lRet)   
    return lRet;         /* else continue message processing */
  }             
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
                                   MODULE_GetWndProcEntry16("ColorDlgProc"), 
                                   (DWORD)lpChCol );
    SYSRES_FreeResource( hDlgTmpl );
    return bRet;
}


/***********************************************************************
 *           FindTextDlg   (COMMDLG.11)
 */
BOOL FindText(LPFINDREPLACE lpFind)
{
    HANDLE hInst, hDlgTmpl;
    BOOL bRet;
    SEGPTR ptr;

    /*
     * FIXME : Should respond to FR_ENABLETEMPLATE and FR_ENABLEHOOK here
     * For now, only the standard dialog works.
     */
    /*
     * FIXME : We should do error checking on the lpFind structure here
     * and make CommDlgExtendedError() return the error condition.
     */
    hDlgTmpl = SYSRES_LoadResource( SYSRES_DIALOG_FIND_TEXT );
    hInst = WIN_GetWindowInstance( lpFind->hwndOwner );
    if (!(ptr = (SEGPTR)WIN16_GlobalLock16( hDlgTmpl ))) return -1;
    bRet = CreateDialogIndirectParam( hInst, ptr, lpFind->hwndOwner,
                                      MODULE_GetWndProcEntry16("FindTextDlgProc"),
                                      (DWORD)lpFind );
    GlobalUnlock16( hDlgTmpl );
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
    SEGPTR ptr;

    /*
     * FIXME : Should respond to FR_ENABLETEMPLATE and FR_ENABLEHOOK here
     * For now, only the standard dialog works.
     */
    /*
     * FIXME : We should do error checking on the lpFind structure here
     * and make CommDlgExtendedError() return the error condition.
     */
    hDlgTmpl = SYSRES_LoadResource( SYSRES_DIALOG_REPLACE_TEXT );
    hInst = WIN_GetWindowInstance( lpFind->hwndOwner );
    if (!(ptr = (SEGPTR)WIN16_GlobalLock16( hDlgTmpl ))) return -1;
    bRet = CreateDialogIndirectParam( hInst, ptr, lpFind->hwndOwner,
                                      MODULE_GetWndProcEntry16("ReplaceTextDlgProc"),
                                      (DWORD)lpFind );
    GlobalUnlock16( hDlgTmpl );
    SYSRES_FreeResource( hDlgTmpl );
    return bRet;
}


/***********************************************************************
 *                              FINDDLG_WMInitDialog            [internal]
 */
static LRESULT FINDDLG_WMInitDialog(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    LPFINDREPLACE lpfr;

    SetWindowLong(hWnd, DWL_USER, lParam);
    lpfr = (LPFINDREPLACE)lParam;
    lpfr->Flags &= ~(FR_FINDNEXT | FR_REPLACE | FR_REPLACEALL | FR_DIALOGTERM);
    /*
     * FIXME : If the initial FindWhat string is empty, we should disable the
     * FindNext (IDOK) button.  Only after typing some text, the button should be
     * enabled.
     */
    SetDlgItemText(hWnd, edt1, lpfr->lpstrFindWhat);
    CheckRadioButton(hWnd, rad1, rad2, (lpfr->Flags & FR_DOWN) ? rad2 : rad1);
    if (lpfr->Flags & (FR_HIDEUPDOWN | FR_NOUPDOWN)) {
	EnableWindow(GetDlgItem(hWnd, rad1), FALSE);
	EnableWindow(GetDlgItem(hWnd, rad2), FALSE);
    }
    if (lpfr->Flags & FR_HIDEUPDOWN) {
	ShowWindow(GetDlgItem(hWnd, rad1), SW_HIDE);
	ShowWindow(GetDlgItem(hWnd, rad2), SW_HIDE);
	ShowWindow(GetDlgItem(hWnd, grp1), SW_HIDE);
    }
    CheckDlgButton(hWnd, chx1, (lpfr->Flags & FR_WHOLEWORD) ? 1 : 0);
    if (lpfr->Flags & (FR_HIDEWHOLEWORD | FR_NOWHOLEWORD))
	EnableWindow(GetDlgItem(hWnd, chx1), FALSE);
    if (lpfr->Flags & FR_HIDEWHOLEWORD)
	ShowWindow(GetDlgItem(hWnd, chx1), SW_HIDE);
    CheckDlgButton(hWnd, chx2, (lpfr->Flags & FR_MATCHCASE) ? 1 : 0);
    if (lpfr->Flags & (FR_HIDEMATCHCASE | FR_NOMATCHCASE))
	EnableWindow(GetDlgItem(hWnd, chx2), FALSE);
    if (lpfr->Flags & FR_HIDEMATCHCASE)
	ShowWindow(GetDlgItem(hWnd, chx2), SW_HIDE);
    if (!(lpfr->Flags & FR_SHOWHELP)) {
	EnableWindow(GetDlgItem(hWnd, pshHelp), FALSE);
	ShowWindow(GetDlgItem(hWnd, pshHelp), SW_HIDE);
    }
    ShowWindow(hWnd, SW_SHOWNORMAL);
    return TRUE;
}    


/***********************************************************************
 *                              FINDDLG_WMCommand               [internal]
 */
static LRESULT FINDDLG_WMCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    LPFINDREPLACE lpfr;
    int uFindReplaceMessage = RegisterWindowMessage(MAKE_SEGPTR(FINDMSGSTRING));
    int uHelpMessage = RegisterWindowMessage(MAKE_SEGPTR(HELPMSGSTRING));

    lpfr = (LPFINDREPLACE)GetWindowLong(hWnd, DWL_USER);
    switch (wParam) {
	case IDOK:
	    GetDlgItemText(hWnd, edt1, lpfr->lpstrFindWhat, lpfr->wFindWhatLen);
	    if (IsDlgButtonChecked(hWnd, rad2))
		lpfr->Flags |= FR_DOWN;
		else lpfr->Flags &= ~FR_DOWN;
	    if (IsDlgButtonChecked(hWnd, chx1))
		lpfr->Flags |= FR_WHOLEWORD; 
		else lpfr->Flags &= ~FR_WHOLEWORD;
	    if (IsDlgButtonChecked(hWnd, chx2))
		lpfr->Flags |= FR_MATCHCASE; 
		else lpfr->Flags &= ~FR_MATCHCASE;
            lpfr->Flags &= ~(FR_REPLACE | FR_REPLACEALL | FR_DIALOGTERM);
	    lpfr->Flags |= FR_FINDNEXT;
	    SendMessage(lpfr->hwndOwner, uFindReplaceMessage, 0, (LPARAM)MAKE_SEGPTR(lpfr));
	    return TRUE;
	case IDCANCEL:
            lpfr->Flags &= ~(FR_FINDNEXT | FR_REPLACE | FR_REPLACEALL);
	    lpfr->Flags |= FR_DIALOGTERM;
	    SendMessage(lpfr->hwndOwner, uFindReplaceMessage, 0, (LPARAM)MAKE_SEGPTR(lpfr));
	    DestroyWindow(hWnd);
	    return TRUE;
	case pshHelp:
	    /* FIXME : should lpfr structure be passed as an argument ??? */
	    SendMessage(lpfr->hwndOwner, uHelpMessage, 0, 0);
	    return TRUE;
    }
    return FALSE;
}    


/***********************************************************************
 *           FindTextDlgProc   (COMMDLG.13)
 */
LRESULT FindTextDlgProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    switch (wMsg) {
	case WM_INITDIALOG:
	    return FINDDLG_WMInitDialog(hWnd, wParam, lParam);
	case WM_COMMAND:
	    return FINDDLG_WMCommand(hWnd, wParam, lParam);
    }
    return FALSE;
}


/***********************************************************************
 *                              REPLACEDLG_WMInitDialog         [internal]
 */
static LRESULT REPLACEDLG_WMInitDialog(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    LPFINDREPLACE lpfr;

    SetWindowLong(hWnd, DWL_USER, lParam);
    lpfr = (LPFINDREPLACE)lParam;
    lpfr->Flags &= ~(FR_FINDNEXT | FR_REPLACE | FR_REPLACEALL | FR_DIALOGTERM);
    /*
     * FIXME : If the initial FindWhat string is empty, we should disable the FinNext /
     * Replace / ReplaceAll buttons.  Only after typing some text, the buttons should be
     * enabled.
     */
    SetDlgItemText(hWnd, edt1, lpfr->lpstrFindWhat);
    SetDlgItemText(hWnd, edt2, lpfr->lpstrReplaceWith);
    CheckDlgButton(hWnd, chx1, (lpfr->Flags & FR_WHOLEWORD) ? 1 : 0);
    if (lpfr->Flags & (FR_HIDEWHOLEWORD | FR_NOWHOLEWORD))
	EnableWindow(GetDlgItem(hWnd, chx1), FALSE);
    if (lpfr->Flags & FR_HIDEWHOLEWORD)
	ShowWindow(GetDlgItem(hWnd, chx1), SW_HIDE);
    CheckDlgButton(hWnd, chx2, (lpfr->Flags & FR_MATCHCASE) ? 1 : 0);
    if (lpfr->Flags & (FR_HIDEMATCHCASE | FR_NOMATCHCASE))
	EnableWindow(GetDlgItem(hWnd, chx2), FALSE);
    if (lpfr->Flags & FR_HIDEMATCHCASE)
	ShowWindow(GetDlgItem(hWnd, chx2), SW_HIDE);
    if (!(lpfr->Flags & FR_SHOWHELP)) {
	EnableWindow(GetDlgItem(hWnd, pshHelp), FALSE);
	ShowWindow(GetDlgItem(hWnd, pshHelp), SW_HIDE);
    }
    ShowWindow(hWnd, SW_SHOWNORMAL);
    return TRUE;
}    


/***********************************************************************
 *                              REPLACEDLG_WMCommand            [internal]
 */
static LRESULT REPLACEDLG_WMCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    LPFINDREPLACE lpfr;
    int uFindReplaceMessage = RegisterWindowMessage(MAKE_SEGPTR(FINDMSGSTRING));
    int uHelpMessage = RegisterWindowMessage(MAKE_SEGPTR(HELPMSGSTRING));

    lpfr = (LPFINDREPLACE)GetWindowLong(hWnd, DWL_USER);
    switch (wParam) {
	case IDOK:
	    GetDlgItemText(hWnd, edt1, lpfr->lpstrFindWhat, lpfr->wFindWhatLen);
	    GetDlgItemText(hWnd, edt2, lpfr->lpstrReplaceWith, lpfr->wReplaceWithLen);
	    if (IsDlgButtonChecked(hWnd, chx1))
		lpfr->Flags |= FR_WHOLEWORD; 
		else lpfr->Flags &= ~FR_WHOLEWORD;
	    if (IsDlgButtonChecked(hWnd, chx2))
		lpfr->Flags |= FR_MATCHCASE; 
		else lpfr->Flags &= ~FR_MATCHCASE;
            lpfr->Flags &= ~(FR_REPLACE | FR_REPLACEALL | FR_DIALOGTERM);
	    lpfr->Flags |= FR_FINDNEXT;
	    SendMessage(lpfr->hwndOwner, uFindReplaceMessage, 0, (LPARAM)MAKE_SEGPTR(lpfr));
	    return TRUE;
	case IDCANCEL:
            lpfr->Flags &= ~(FR_FINDNEXT | FR_REPLACE | FR_REPLACEALL);
	    lpfr->Flags |= FR_DIALOGTERM;
	    SendMessage(lpfr->hwndOwner, uFindReplaceMessage, 0, (LPARAM)MAKE_SEGPTR(lpfr));
	    DestroyWindow(hWnd);
	    return TRUE;
	case psh1:
	    GetDlgItemText(hWnd, edt1, lpfr->lpstrFindWhat, lpfr->wFindWhatLen);
	    GetDlgItemText(hWnd, edt2, lpfr->lpstrReplaceWith, lpfr->wReplaceWithLen);
	    if (IsDlgButtonChecked(hWnd, chx1))
		lpfr->Flags |= FR_WHOLEWORD; 
		else lpfr->Flags &= ~FR_WHOLEWORD;
	    if (IsDlgButtonChecked(hWnd, chx2))
		lpfr->Flags |= FR_MATCHCASE; 
		else lpfr->Flags &= ~FR_MATCHCASE;
            lpfr->Flags &= ~(FR_FINDNEXT | FR_REPLACEALL | FR_DIALOGTERM);
	    lpfr->Flags |= FR_REPLACE;
	    SendMessage(lpfr->hwndOwner, uFindReplaceMessage, 0, (LPARAM)MAKE_SEGPTR(lpfr));
	    return TRUE;
	case psh2:
	    GetDlgItemText(hWnd, edt1, lpfr->lpstrFindWhat, lpfr->wFindWhatLen);
	    GetDlgItemText(hWnd, edt2, lpfr->lpstrReplaceWith, lpfr->wReplaceWithLen);
	    if (IsDlgButtonChecked(hWnd, chx1))
		lpfr->Flags |= FR_WHOLEWORD; 
		else lpfr->Flags &= ~FR_WHOLEWORD;
	    if (IsDlgButtonChecked(hWnd, chx2))
		lpfr->Flags |= FR_MATCHCASE; 
		else lpfr->Flags &= ~FR_MATCHCASE;
            lpfr->Flags &= ~(FR_FINDNEXT | FR_REPLACE | FR_DIALOGTERM);
	    lpfr->Flags |= FR_REPLACEALL;
	    SendMessage(lpfr->hwndOwner, uFindReplaceMessage, 0, (LPARAM)MAKE_SEGPTR(lpfr));
	    return TRUE;
	case pshHelp:
	    /* FIXME : should lpfr structure be passed as an argument ??? */
	    SendMessage(lpfr->hwndOwner, uHelpMessage, 0, 0);
	    return TRUE;
    }
    return FALSE;
}    


/***********************************************************************
 *           ReplaceTextDlgProc   (COMMDLG.14)
 */
LRESULT ReplaceTextDlgProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    switch (wMsg) {
	case WM_INITDIALOG:
	    return REPLACEDLG_WMInitDialog(hWnd, wParam, lParam);
	case WM_COMMAND:
	    return REPLACEDLG_WMCommand(hWnd, wParam, lParam);
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
                                     MODULE_GetWndProcEntry16("PrintSetupDlgProc") :
                                     MODULE_GetWndProcEntry16("PrintDlgProc"),
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


/* --------------------------- Choose Color Dialog ------------------------------ */

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
 LPCHOOSECOLOR lpcc;  /* points to public known data structure */
 int nextuserdef;     /* next free place in user defined color array */
 HDC hdcMem;          /* color graph used for BitBlt() */
 HBITMAP hbmMem;      /* color graph bitmap */    
 RECT fullsize;       /* original dialog window size */
 UINT msetrgb;        /* # of SETRGBSTRING message (today not used)  */
 RECT old3angle;      /* last position of l-marker */
 RECT oldcross;       /* last position of color/satuation marker */
 BOOL updating;       /* to prevent recursive WM_COMMAND/EN_UPDATE procesing */
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
static int CC_MouseCheckPredefColorArray(HWND hDlg,int dlgitem,int rows,int cols,
	    LPARAM lParam,COLORREF *cr)
{
 HWND hwnd;
 POINT point;
 RECT rect;
 int dx,dy,x,y;

 point.x=LOWORD(lParam);
 point.y=HIWORD(lParam);
 ClientToScreen(hDlg,&point);

 hwnd=GetDlgItem(hDlg,dlgitem);
 GetWindowRect(hwnd,&rect);
 if (PtInRect(&rect,point))
 {
  dx=(rect.right-rect.left)/cols;
  dy=(rect.bottom-rect.top)/rows;
  ScreenToClient(hwnd,&point);

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
static int CC_MouseCheckUserColorArray(HWND hDlg,int dlgitem,int rows,int cols,
	    LPARAM lParam,COLORREF *cr,COLORREF*crarr)
{
 HWND hwnd;
 POINT point;
 RECT rect;
 int dx,dy,x,y;

 point.x=LOWORD(lParam);
 point.y=HIWORD(lParam);
 ClientToScreen(hDlg,&point);

 hwnd=GetDlgItem(hDlg,dlgitem);
 GetWindowRect(hwnd,&rect);
 if (PtInRect(&rect,point))
 {
  dx=(rect.right-rect.left)/cols;
  dy=(rect.bottom-rect.top)/rows;
  ScreenToClient(hwnd,&point);

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
static int CC_MouseCheckColorGraph(HWND hDlg,int dlgitem,int *hori,int *vert,LPARAM lParam)
{
 HWND hwnd;
 POINT point;
 RECT rect;
 long x,y;

 point.x=LOWORD(lParam);
 point.y=HIWORD(lParam);
 ClientToScreen(hDlg,&point);
 hwnd=GetDlgItem(hDlg,dlgitem);
 GetWindowRect(hwnd,&rect);
 if (PtInRect(&rect,point))
 {
  GetClientRect(hwnd,&rect);
  ScreenToClient(hwnd,&point);

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
static int CC_MouseCheckResultWindow(HWND hDlg,LPARAM lParam)
{
 HWND hwnd;
 POINT point;
 RECT rect;
 point.x=LOWORD(lParam);
 point.y=HIWORD(lParam);
 ClientToScreen(hDlg,&point);
 hwnd=GetDlgItem(hDlg,0x2c5);
 GetWindowRect(hwnd,&rect);
 if (PtInRect(&rect,point))
 {
  PostMessage(hDlg,WM_COMMAND,0x2c9,0);
  return 1;
 }
 return 0;
}

/***********************************************************************
 *                       CC_CheckDigitsInEdit                 [internal]
 */
static int CC_CheckDigitsInEdit(HWND hwnd,int maxval)
{
 int i,k,m,result,value;
 long editpos;
 char buffer[30];
 GetWindowText(hwnd,buffer,30-1);
 m=lstrlen(buffer);
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
  editpos=SendMessage(hwnd,EM_GETSEL,0,0);
  SetWindowText(hwnd,buffer);
  SendMessage(hwnd,EM_SETSEL,0,editpos);
 }
 return value;
}



/***********************************************************************
 *                    CC_PaintSelectedColor                   [internal]
 */
static void CC_PaintSelectedColor(HWND hDlg,COLORREF cr)
{
 RECT rect;
 HDC  hdc;
 HBRUSH hBrush;
 HWND hwnd=GetDlgItem(hDlg,0x2c5);
 if (IsWindowVisible(GetDlgItem(hDlg,0x2c6)))   /* if full size */
 {
  hdc=GetDC(hwnd);
  GetClientRect (hwnd, &rect) ;
  hBrush = CreateSolidBrush(cr);
  if (hBrush)
  {
   hBrush = SelectObject (hdc, hBrush) ;
   Rectangle (hdc, rect.left,rect.top,rect.right/2,rect.bottom);
   DeleteObject (SelectObject (hdc,hBrush)) ;
   hBrush=CreateSolidBrush(GetNearestColor(hdc,cr));
   if (hBrush)
   {
    hBrush= SelectObject (hdc, hBrush) ;
    Rectangle (hdc, rect.right/2-1,rect.top,rect.right,rect.bottom);
    DeleteObject (SelectObject (hdc, hBrush)) ;
   }
  }
  ReleaseDC(hwnd,hdc);
 }
}

/***********************************************************************
 *                    CC_PaintTriangle                        [internal]
 */
static void CC_PaintTriangle(HWND hDlg,int y)
{
 HDC hDC;
 long temp;
 int w=GetDialogBaseUnits();
 POINT points[3];
 int height;
 int oben;
 RECT rect;
 HWND hwnd=GetDlgItem(hDlg,0x2be);
 struct CCPRIVATE *lpp=(struct CCPRIVATE *)GetWindowLong(hDlg, DWL_USER); 

 if (IsWindowVisible(GetDlgItem(hDlg,0x2c6)))   /* if full size */
 {
   GetClientRect(hwnd,&rect);
   height=rect.bottom;
   hDC=GetDC(hDlg);

   points[0].y=rect.top;
   points[0].x=rect.right;         /*  |  /|  */
   ClientToScreen(hwnd,points);    /*  | / |  */
   ScreenToClient(hDlg,points);    /*  |<  |  */
   oben=points[0].y;               /*  | \ |  */
				   /*  |  \|  */
   temp=(long)height*(long)y;
   points[0].y=oben+height -temp/(long)MAXVERT;
   points[1].y=points[0].y+w;
   points[2].y=points[0].y-w;
   points[2].x=points[1].x=points[0].x + w;

   if (lpp->old3angle.left)
    FillRect(hDC,&lpp->old3angle,GetStockObject(WHITE_BRUSH));
   lpp->old3angle.left  =points[0].x;
   lpp->old3angle.right =points[1].x+1;
   lpp->old3angle.top   =points[2].y-1;
   lpp->old3angle.bottom=points[1].y+1;
   Polygon(hDC,points,3);
   ReleaseDC(hDlg,hDC);
 }
}


/***********************************************************************
 *                    CC_PaintCross                           [internal]
 */
static void CC_PaintCross(HWND hDlg,int x,int y)
{
 HDC hDC;
 int w=GetDialogBaseUnits();
 HWND hwnd=GetDlgItem(hDlg,0x2c6);
 struct CCPRIVATE * lpp=(struct CCPRIVATE *)GetWindowLong(hDlg, DWL_USER); 
 RECT rect;
 POINT point;
 HPEN hPen;

 if (IsWindowVisible(GetDlgItem(hDlg,0x2c6)))   /* if full size */
 {
   GetClientRect(hwnd,&rect);
   hDC=GetDC(hwnd);
   SelectClipRgn(hDC,CreateRectRgnIndirect(&rect));   
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

   MoveTo(hDC,point.x-w,point.y); 
   LineTo(hDC,point.x+w,point.y);
   MoveTo(hDC,point.x,point.y-w); 
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
static void CC_PrepareColorGraph(HWND hDlg)    
{
 int sdif,hdif,xdif,ydif,r,g,b,hue,sat;
 HWND hwnd=GetDlgItem(hDlg,0x2c6);
 struct CCPRIVATE * lpp=(struct CCPRIVATE *)GetWindowLong(hDlg, DWL_USER);  
 HBRUSH hbrush;
 HDC hdc ;
 RECT rect,client;
 HCURSOR hcursor=SetCursor(LoadCursor(0,IDC_WAIT));

 GetClientRect(hwnd,&client);
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
   FillRect(lpp->hdcMem,&rect,hbrush);
   DeleteObject(hbrush);
   rect.bottom=rect.top;
  }
  rect.left=rect.right;
 }
 ReleaseDC(hwnd,hdc);
 SetCursor(hcursor);
}

/***********************************************************************
 *                          CC_PaintColorGraph                [internal]
 */
static void CC_PaintColorGraph(HWND hDlg)
{
 HWND hwnd=GetDlgItem(hDlg,0x2c6);
 struct CCPRIVATE * lpp=(struct CCPRIVATE *)GetWindowLong(hDlg, DWL_USER); 
 HDC  hDC;
 RECT rect;
 if (IsWindowVisible(hwnd))   /* if full size */
 {
  if (!lpp->hdcMem)
   CC_PrepareColorGraph(hDlg);   /* should not be necessary */

  hDC=GetDC(hwnd);
  GetClientRect(hwnd,&rect);
  if (lpp->hdcMem)
    BitBlt(hDC,0,0,rect.right,rect.bottom,lpp->hdcMem,0,0,SRCCOPY);
  else
    fprintf(stderr,"choose color: hdcMem is not defined\n");
  ReleaseDC(hwnd,hDC);
 }
}
/***********************************************************************
 *                           CC_PaintLumBar                   [internal]
 */
static void CC_PaintLumBar(HWND hDlg,int hue,int sat)
{
 HWND hwnd=GetDlgItem(hDlg,0x2be);
 RECT rect,client;
 int lum,ldif,ydif,r,g,b;
 HBRUSH hbrush;
 HDC hDC;

 if (IsWindowVisible(hwnd))
 {
  hDC=GetDC(hwnd);
  GetClientRect(hwnd,&client);
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
   FillRect(hDC,&rect,hbrush);
   DeleteObject(hbrush);
   rect.bottom=rect.top;
  }
  GetClientRect(hwnd,&rect);
  FrameRect(hDC,&rect,GetStockObject(BLACK_BRUSH));
  ReleaseDC(hwnd,hDC);
 }
}

/***********************************************************************
 *                             CC_EditSetRGB                  [internal]
 */
static void CC_EditSetRGB(HWND hDlg,COLORREF cr)
{
 char buffer[10];
 struct CCPRIVATE * lpp=(struct CCPRIVATE *)GetWindowLong(hDlg, DWL_USER); 
 int r=GetRValue(cr);
 int g=GetGValue(cr);
 int b=GetBValue(cr);
 if (IsWindowVisible(GetDlgItem(hDlg,0x2c6)))   /* if full size */
 {
   lpp->updating=TRUE;
   sprintf(buffer,"%d",r);
   SetWindowText(GetDlgItem(hDlg,0x2c2),buffer);
   sprintf(buffer,"%d",g);
   SetWindowText(GetDlgItem(hDlg,0x2c3),buffer);
   sprintf(buffer,"%d",b);
   SetWindowText(GetDlgItem(hDlg,0x2c4),buffer);
   lpp->updating=FALSE;
 }
}

/***********************************************************************
 *                             CC_EditSetHSL                  [internal]
 */
static void CC_EditSetHSL(HWND hDlg,int h,int s,int l)
{
 char buffer[10];
 struct CCPRIVATE * lpp=(struct CCPRIVATE *)GetWindowLong(hDlg, DWL_USER); 
 lpp->updating=TRUE;
 if (IsWindowVisible(GetDlgItem(hDlg,0x2c6)))   /* if full size */
 {
   lpp->updating=TRUE;
   sprintf(buffer,"%d",h);
   SetWindowText(GetDlgItem(hDlg,0x2bf),buffer);
   sprintf(buffer,"%d",s);
   SetWindowText(GetDlgItem(hDlg,0x2c0),buffer);
   sprintf(buffer,"%d",l);
   SetWindowText(GetDlgItem(hDlg,0x2c1),buffer);
   lpp->updating=FALSE;
 }
 CC_PaintLumBar(hDlg,h,s);
}

/***********************************************************************
 *                       CC_SwitchToFullSize                  [internal]
 */
static void CC_SwitchToFullSize(HWND hDlg,COLORREF result,LPRECT lprect)
{
 int i;
 struct CCPRIVATE * lpp=(struct CCPRIVATE *)GetWindowLong(hDlg, DWL_USER); 
 
 EnableWindow(GetDlgItem(hDlg,0x2cf),FALSE);
 CC_PrepareColorGraph(hDlg);
 for (i=0x2bf;i<0x2c5;i++)
   EnableWindow(GetDlgItem(hDlg,i),TRUE);
 for (i=0x2d3;i<0x2d9;i++)
   EnableWindow(GetDlgItem(hDlg,i),TRUE);
 EnableWindow(GetDlgItem(hDlg,0x2c9),TRUE);
 EnableWindow(GetDlgItem(hDlg,0x2c8),TRUE);

 if (lprect)
  SetWindowPos(hDlg,NULL,0,0,lprect->right-lprect->left,
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
static void CC_PaintPredefColorArray(HWND hDlg,int rows,int cols)
{
 HWND hwnd=GetDlgItem(hDlg,0x2d0);
 RECT rect;
 HDC  hdc;
 HBRUSH hBrush;
 int dx,dy,i,j,k;

 GetClientRect(hwnd,&rect);
 dx=rect.right/cols;
 dy=rect.bottom/rows;
 k=rect.left;

 hdc=GetDC(hwnd);
 GetClientRect (hwnd, &rect) ;

 for (j=0;j<rows;j++)
 {
  for (i=0;i<cols;i++)
  {
   hBrush = CreateSolidBrush(predefcolors[j][i]);
   if (hBrush)
   {
    hBrush = SelectObject (hdc, hBrush) ;
    Rectangle (hdc, rect.left,     rect.top,
		    rect.left+dx-DISTANCE,rect.top+dy-DISTANCE);
    rect.left=rect.left+dx;
    DeleteObject (SelectObject (hdc, hBrush)) ;
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
static void CC_PaintUserColorArray(HWND hDlg,int rows,int cols,COLORREF* lpcr)
{
 HWND hwnd=GetDlgItem(hDlg,0x2d1);
 RECT rect;
 HDC  hdc;
 HBRUSH hBrush;
 int dx,dy,i,j,k;

 GetClientRect(hwnd,&rect);

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
     Rectangle (hdc, rect.left,     rect.top,
		    rect.left+dx-DISTANCE,rect.top+dy-DISTANCE);
     rect.left=rect.left+dx;
     DeleteObject (SelectObject (hdc, hBrush)) ;
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
static BOOL CC_HookCallChk(LPCHOOSECOLOR lpcc)
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
static LONG CC_WMInitDialog(HWND hDlg, WPARAM wParam, LPARAM lParam) 
{
   int i,res;
   HWND hwnd;
   RECT rect;
   POINT point;
   struct CCPRIVATE * lpp; 
   
   dprintf_commdlg(stddeb,"ColorDlgProc // WM_INITDIALOG lParam=%08lX\n", lParam);
   lpp=calloc(1,sizeof(struct CCPRIVATE));
   lpp->lpcc=(LPCHOOSECOLOR)lParam;
   if (lpp->lpcc->lStructSize != sizeof(CHOOSECOLOR))
   {
      EndDialog (hDlg, 0) ;
      return FALSE;
   }
   SetWindowLong(hDlg, DWL_USER, (LONG)lpp); 

   if (!(lpp->lpcc->Flags & CC_SHOWHELP))
      ShowWindow(GetDlgItem(hDlg,0x40e),SW_HIDE);
   lpp->msetrgb=RegisterWindowMessage(MAKE_SEGPTR(SETRGBSTRING));
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
   GetWindowRect(hDlg,&lpp->fullsize);
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
      ClientToScreen(hwnd,&point);
      ScreenToClient(hDlg,&point);
      GetClientRect(hDlg,&rect);
      point.x+=GetSystemMetrics(SM_CXDLGFRAME);
      SetWindowPos(hDlg,NULL,0,0,point.x,res,SWP_NOMOVE|SWP_NOZORDER);

      ShowWindow(GetDlgItem(hDlg,0x2c6),SW_HIDE);
      ShowWindow(GetDlgItem(hDlg,0x2c5),SW_HIDE);
   }
   else
      CC_SwitchToFullSize(hDlg,lpp->lpcc->rgbResult,NULL);
   res=TRUE;
   for (i=0x2bf;i<0x2c5;i++)
     SendMessage(GetDlgItem(hDlg,i),EM_LIMITTEXT,3,0);      /* max 3 digits:  xyz  */
   if (CC_HookCallChk(lpp->lpcc))
      res=CallWindowProc((FARPROC)lpp->lpcc->lpfnHook,hDlg,WM_INITDIALOG,wParam,lParam);
   return res;
}

/***********************************************************************
 *                              CC_WMCommand                  [internal]
 */
static LRESULT CC_WMCommand(HWND hDlg, WPARAM wParam, LPARAM lParam) 
{
    int r,g,b,i,xx;
    UINT cokmsg;
    HDC hdc;
    COLORREF *cr;
    struct CCPRIVATE * lpp=(struct CCPRIVATE *)GetWindowLong(hDlg, DWL_USER); 
    dprintf_commdlg(stddeb,"CC_WMCommand wParam=%x lParam=%lx\n",wParam,lParam);
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
	       InvalidateRect(hDlg,NULL,NULL);
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
	       i=RegisterWindowMessage(MAKE_SEGPTR(HELPMSGSTRING));
	       if (lpp->lpcc->hwndOwner)
		   SendMessage(lpp->lpcc->hwndOwner,i,0,(LPARAM)lpp->lpcc);
	       if (CC_HookCallChk(lpp->lpcc))
		   CallWindowProc((FARPROC)lpp->lpcc->lpfnHook,hDlg,
		      WM_COMMAND,psh15,(LPARAM)lpp->lpcc);
	       break;

          case IDOK :
		cokmsg=RegisterWindowMessage(MAKE_SEGPTR(COLOROKSTRING));
		if (lpp->lpcc->hwndOwner)
			if (SendMessage(lpp->lpcc->hwndOwner,cokmsg,0,(LPARAM)lpp->lpcc))
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
static LRESULT CC_WMPaint(HWND hDlg, WPARAM wParam, LPARAM lParam) 
{
    struct CCPRIVATE * lpp=(struct CCPRIVATE *)GetWindowLong(hDlg, DWL_USER); 
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
 return 0;
}


/***********************************************************************
 *                              CC_WMLButtonDown              [internal]
 */
static LRESULT CC_WMLButtonDown(HWND hDlg, WPARAM wParam, LPARAM lParam) 
{
   struct CCPRIVATE * lpp=(struct CCPRIVATE *)GetWindowLong(hDlg, DWL_USER); 
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
LRESULT ColorDlgProc(HWND hDlg, UINT message,
			 UINT wParam, LONG lParam)
{
 int res;
 struct CCPRIVATE * lpp=(struct CCPRIVATE *)GetWindowLong(hDlg, DWL_USER); 
 if (message!=WM_INITDIALOG)
 {
  if (!lpp)
     return FALSE;
  res=0;
  if (CC_HookCallChk(lpp->lpcc))
     res=CallWindowProc((FARPROC)lpp->lpcc->lpfnHook,hDlg,message,wParam,lParam);
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
	                SetWindowLong(hDlg, DWL_USER, 0L); /* we don't need it anymore */
	                break;
	  case WM_COMMAND:
	                if (CC_WMCommand(hDlg, wParam, lParam))
	                   return TRUE;
	                break;     
	  case WM_PAINT:
	                CC_WMPaint(hDlg, wParam, lParam);
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



/***********************************************************************
 *                        ChooseFont   (COMMDLG.15)     
 */
BOOL ChooseFont(LPCHOOSEFONT lpChFont)
{
    HANDLE hInst, hDlgTmpl;
    BOOL bRet;
    dprintf_commdlg(stddeb,"ChoseFont\n");
    hDlgTmpl = SYSRES_LoadResource( SYSRES_DIALOG_CHOOSE_FONT );
    hInst = WIN_GetWindowInstance( lpChFont->hwndOwner );
    bRet = DialogBoxIndirectParam( hInst, hDlgTmpl, lpChFont->hwndOwner,
                                   MODULE_GetWndProcEntry16("FormatCharDlgProc"), 
                                   (DWORD)lpChFont );
    SYSRES_FreeResource( hDlgTmpl );
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
static BOOL CFn_HookCallChk(LPCHOOSEFONT lpcf)
{
 if (lpcf)
  if(lpcf->Flags & CF_ENABLEHOOK)
   if (lpcf->lpfnHook)
    return TRUE;
 return FALSE;
}

/***********************************************************************
 *                FontFamilyEnumProc                       (COMMDLG.19)
 */
int FontFamilyEnumProc(LPLOGFONT lplf ,LPTEXTMETRIC lptm, int nFontType, LPARAM lParam)
{
  int i;
  WORD w;
  HWND hwnd=LOWORD(lParam);

  dprintf_commdlg(stddeb,"FontFamilyEnumProc: font=%s (nFontType=%d)\n",
     			lplf->lfFaceName,nFontType);
  i=SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)MAKE_SEGPTR(lplf->lfFaceName));
  if (i!=CB_ERR)
  {
    w=(lplf->lfCharSet << 8) | lplf->lfPitchAndFamily;
    SendMessage(hwnd, CB_SETITEMDATA,i,MAKELONG(nFontType,w));
    return 1 ;
  }
  else
    return 0;
}

/***********************************************************************
 *                 FontStyleEnumProc                     (COMMDLG.18)
 */
int FontStyleEnumProc(LPLOGFONT lplf ,LPTEXTMETRIC lptm, int nFontType, LPARAM lParam)
{
  int j;
  char buffer[20];
/*  HWND hcmb2=LOWORD(lParam);*/
  HWND hcmb3=HIWORD(lParam);
  LPLOGFONT lf=lplf;

  dprintf_commdlg(stddeb,"FontStyleEnumProc: (nFontType=%d)\n",nFontType);
  dprintf_commdlg(stddeb,"  %s h=%d w=%d e=%d o=%d wg=%d i=%d u=%d s=%d ch=%d op=%d cp=%d q=%d pf=%xh\n",
	lf->lfFaceName,lf->lfHeight,lf->lfWidth,lf->lfEscapement,lf->lfOrientation,
	lf->lfWeight,lf->lfItalic,lf->lfUnderline,lf->lfStrikeOut,lf->lfCharSet,
	lf->lfOutPrecision,lf->lfClipPrecision,lf->lfQuality,lf->lfPitchAndFamily);

#if 1         /* VERSION A: use some predefined height values */                       
  /* FIXME: if (!(nFontType & RASTER_FONTTYPE))......... */
  {
    int sizes[]={8,9,10,11,12,14,16,18,20,22,24,26,28,36,48,72,0};  
    int i;
    if (!SendMessage(hcmb3,CB_GETCOUNT,0,0)) 
    {
      i=0;
      while (sizes[i])
      {
        sprintf(buffer,"%3d",sizes[i++]);
        j=SendMessage(hcmb3,CB_ADDSTRING,0,(LPARAM)MAKE_SEGPTR(buffer));
        SendMessage(hcmb3, CB_SETITEMDATA, j, MAKELONG(sizes[i],0));
      }
    }
  }
  return 0; 
#endif

#if 0        /* VERSION B: use only lplf->lfHeight values */
  {
    if (lplf->lfHeight)
    {
      sprintf(buffer,"%3d",lplf->lfHeight);
      j=SendMessage(hcmb3,CB_FINDSTRING,-1,(LPARAM)MAKE_SEGPTR(buffer));
      if (j==CB_ERR)
      {
       j=SendMessage(hcmb3,CB_ADDSTRING,0,(LPARAM)MAKE_SEGPTR(buffer));
       SendMessage(hcmb3, CB_SETITEMDATA, j, MAKELONG(lplf->lfHeight,lplf->lfWidth));
      } 
    } 
  }
  return 1 ;
#endif  

}


/***********************************************************************
 *           CFn_WMInitDialog                            [internal]
 */
LRESULT CFn_WMInitDialog(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
  HDC hdc;
  int i,j,res,init=0;
  long l;
  char buffer[32];
  FARPROC enumCallback = MODULE_GetWndProcEntry16("FontFamilyEnumProc");
  LPLOGFONT lpxx;
  HCURSOR hcursor=SetCursor(LoadCursor(0,IDC_WAIT));
  LPCHOOSEFONT lpcf;

  SetWindowLong(hDlg, DWL_USER, lParam); 
  lpcf=(LPCHOOSEFONT)lParam;
  lpxx=PTR_SEG_TO_LIN(lpcf->lpLogFont);
  dprintf_commdlg(stddeb,"FormatCharDlgProc // WM_INITDIALOG lParam=%08lX\n", lParam);

  if (lpcf->lStructSize != sizeof(CHOOSEFONT))
  {
    dprintf_commdlg(stddeb,"WM_INITDIALOG: structure size failure !!!\n");
    EndDialog (hDlg, 0); 
    return FALSE;
  }
  if (!hBitmapTT)
    hBitmapTT = LoadBitmap(0, MAKEINTRESOURCE(OBM_TRTYPE));
			 
  if (!(lpcf->Flags & CF_SHOWHELP) || !IsWindow(lpcf->hwndOwner))
    ShowWindow(GetDlgItem(hDlg,pshHelp),SW_HIDE);
  if (!(lpcf->Flags & CF_APPLY))
    ShowWindow(GetDlgItem(hDlg,psh3),SW_HIDE);
  if (lpcf->Flags & CF_EFFECTS)
  {
    for (res=1,i=0;res && i<TEXT_COLORS;i++)
    {
      /* FIXME: load color name from resource:  res=LoadString(...,i+....,buffer,.....); */
      j=SendDlgItemMessage(hDlg,cmb4,CB_ADDSTRING,0,(LPARAM)MAKE_SEGPTR("[color name]"));
      SendDlgItemMessage(hDlg,cmb4, CB_SETITEMDATA,j,textcolors[j]);
      /* look for a fitting value in color combobox */
      if (textcolors[j]==lpcf->rgbColors)
        SendDlgItemMessage(hDlg,cmb4, CB_SETCURSEL,j,0);
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
  
  /* perhaps this stuff should be moved to FontStyleEnumProc() ?? */
  strcpy(buffer,"Regular"); 	/* LoadString(hInst,.... ,buffer,LF_FACESIZE);*/
  j=SendDlgItemMessage(hDlg,cmb2,CB_ADDSTRING,0,(LPARAM)MAKE_SEGPTR(buffer));
  SendDlgItemMessage(hDlg,cmb2, CB_SETITEMDATA, j, MAKELONG(FW_NORMAL,0));
  strcpy(buffer,"Bold");       	/* LoadString(hInst,.... ,buffer,LF_FACESIZE);*/
  j=SendDlgItemMessage(hDlg,cmb2,CB_ADDSTRING,0,(LPARAM)MAKE_SEGPTR(buffer));
  SendDlgItemMessage(hDlg,cmb2, CB_SETITEMDATA, j, MAKELONG(FW_BOLD,0));
  strcpy(buffer,"Italic");	/* LoadString(hInst,.... ,buffer,LF_FACESIZE);*/
  j=SendDlgItemMessage(hDlg,cmb2,CB_ADDSTRING,0,(LPARAM)MAKE_SEGPTR(buffer));
  SendDlgItemMessage(hDlg,cmb2, CB_SETITEMDATA, j, MAKELONG(FW_NORMAL,1));
  strcpy(buffer,"Bold Italic");	/* LoadString(hInst,.... ,buffer,LF_FACESIZE);*/
  j=SendDlgItemMessage(hDlg,cmb2,CB_ADDSTRING,0,(LPARAM)MAKE_SEGPTR(buffer));
  SendDlgItemMessage(hDlg,cmb2, CB_SETITEMDATA, j, MAKELONG(FW_BOLD,1));
  
  hdc= (lpcf->Flags & CF_PRINTERFONTS && lpcf->hDC) ? lpcf->hDC : GetDC(hDlg);
  if (hdc)
  {
    if (!EnumFontFamilies (hdc, NULL,enumCallback,(LPARAM)GetDlgItem(hDlg,cmb1)))
      dprintf_commdlg(stddeb,"WM_INITDIALOG: EnumFontFamilies returns 0\n");
    if (lpcf->Flags & CF_INITTOLOGFONTSTRUCT)
    {
      /* look for fitting font name in combobox1 */
      j=SendDlgItemMessage(hDlg,cmb1,CB_FINDSTRING,-1,(LONG)lpxx->lfFaceName);
      if (j!=CB_ERR)
      {
        SendDlgItemMessage(hDlg,cmb1,CB_SETCURSEL,j,0);
	SendMessage(hDlg,WM_COMMAND,cmb1,MAKELONG(GetDlgItem(hDlg,cmb1),CBN_SELCHANGE));
        init=1;
        /* look for fitting font style in combobox2 */
        l=MAKELONG(lpxx->lfWeight > FW_MEDIUM ? FW_BOLD:FW_NORMAL,lpxx->lfItalic !=0);
        for (i=0;i<TEXT_EXTRAS;i++)
        {
          if (l==SendDlgItemMessage(hDlg,cmb2, CB_GETITEMDATA,i,0))
            SendDlgItemMessage(hDlg,cmb2,CB_SETCURSEL,i,0);
        }
      
        /* look for fitting font size in combobox3 */
        j=SendDlgItemMessage(hDlg,cmb3,CB_GETCOUNT,0,0);
        for (i=0;i<j;i++)
        {
          if (lpxx->lfHeight==(int)SendDlgItemMessage(hDlg,cmb3, CB_GETITEMDATA,i,0))
            SendDlgItemMessage(hDlg,cmb3,CB_SETCURSEL,i,0);
        }
      }
      if (!init)
      {
        SendDlgItemMessage(hDlg,cmb1,CB_SETCURSEL,0,0);
	SendMessage(hDlg,WM_COMMAND,cmb1,MAKELONG(GetDlgItem(hDlg,cmb1),CBN_SELCHANGE));      
      }
    }
    if (lpcf->Flags & CF_USESTYLE && lpcf->lpszStyle)
    {
      j=SendDlgItemMessage(hDlg,cmb2,CB_FINDSTRING,-1,(LONG)lpcf->lpszStyle);
      if (j!=CB_ERR)
      {
        j=SendDlgItemMessage(hDlg,cmb2,CB_SETCURSEL,j,0);
        SendMessage(hDlg,WM_COMMAND,cmb2,MAKELONG(GetDlgItem(hDlg,cmb2),CBN_SELCHANGE));
      }
    }
  }
  else
  {
    dprintf_commdlg(stddeb,"WM_INITDIALOG: HDC failure !!!\n");
    EndDialog (hDlg, 0); 
    return FALSE;
  }

  if (!(lpcf->Flags & CF_PRINTERFONTS && lpcf->hDC))
    ReleaseDC(hDlg,hdc);
  res=TRUE;
  if (CFn_HookCallChk(lpcf))
    res=CallWindowProc(lpcf->lpfnHook,hDlg,WM_INITDIALOG,wParam,lParam);
  SetCursor(hcursor);   
  return res;
}


/***********************************************************************
 *           CFn_WMMeasureItem                           [internal]
 */
LRESULT CFn_WMMeasureItem(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
  BITMAP bm;
  LPMEASUREITEMSTRUCT lpmi=PTR_SEG_TO_LIN((LPMEASUREITEMSTRUCT)lParam);
  if (!hBitmapTT)
    hBitmapTT = LoadBitmap(0, MAKEINTRESOURCE(OBM_TRTYPE));
  GetObject(hBitmapTT, sizeof(BITMAP), (LPSTR)&bm);
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
  COLORREF cr;
  RECT rect;
#if 0  
  HDC hMemDC;
  int nFontType;
  HBITMAP hBitmap; /* for later TT usage */
#endif  
  LPDRAWITEMSTRUCT lpdi = (LPDRAWITEMSTRUCT)PTR_SEG_TO_LIN(lParam);

  if (lpdi->itemID == 0xFFFF) 			/* got no items */
    DrawFocusRect(lpdi->hDC, &lpdi->rcItem);
  else
  {
   if (lpdi->CtlType == ODT_COMBOBOX)
   {
     hBrush = SelectObject(lpdi->hDC, GetStockObject(LTGRAY_BRUSH));
     SelectObject(lpdi->hDC, hBrush);
     FillRect(lpdi->hDC, &lpdi->rcItem, hBrush);
   }
   else
     return TRUE;	/* this should never happen */

   rect=lpdi->rcItem;
   switch (lpdi->CtlID)
   {
    case cmb1:	/* dprintf_commdlg(stddeb,"WM_Drawitem cmb1\n"); */
		SendMessage(lpdi->hwndItem, CB_GETLBTEXT, lpdi->itemID,
			(LPARAM)MAKE_SEGPTR(buffer));	          
		GetObject(hBitmapTT, sizeof(BITMAP), (LPSTR)&bm);
		TextOut(lpdi->hDC, lpdi->rcItem.left + bm.bmWidth + 10,
			lpdi->rcItem.top, buffer, lstrlen(buffer));
#if 0
		nFontType = SendMessage(lpdi->hwndItem, CB_GETITEMDATA, lpdi->itemID,0L);
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
    case cmb3:	/* dprintf_commdlg(stddeb,"WM_DRAWITEN cmb2,cmb3\n"); */
		SendMessage(lpdi->hwndItem, CB_GETLBTEXT, lpdi->itemID,
			(LPARAM)MAKE_SEGPTR(buffer));
		TextOut(lpdi->hDC, lpdi->rcItem.left,
			lpdi->rcItem.top, buffer, lstrlen(buffer));
		break;

    case cmb4:	/* dprintf_commdlg(stddeb,"WM_DRAWITEM cmb4 (=COLOR)\n"); */
		SendMessage(lpdi->hwndItem, CB_GETLBTEXT, lpdi->itemID,
    		    (LPARAM)MAKE_SEGPTR(buffer));
		TextOut(lpdi->hDC, lpdi->rcItem.left +  25+5,
			lpdi->rcItem.top, buffer, lstrlen(buffer));
		cr = SendMessage(lpdi->hwndItem, CB_GETITEMDATA, lpdi->itemID,0L);
		hBrush = CreateSolidBrush(cr);
		if (hBrush)
		{
		  hBrush = SelectObject (lpdi->hDC, hBrush) ;
		  rect.right=rect.left+25;
		  rect.top++;
		  rect.left+=5;
		  rect.bottom--;
		  Rectangle(lpdi->hDC,rect.left,rect.top,rect.right,rect.bottom);
		  DeleteObject (SelectObject (lpdi->hDC, hBrush)) ;
		}
		rect=lpdi->rcItem;
		rect.left+=25+5;
		break;

    default:	return TRUE;	/* this should never happen */
   }
   if (lpdi->itemState ==ODS_SELECTED)
     InvertRect(lpdi->hDC, &rect);
 }
 return TRUE;
}

/***********************************************************************
 *           CFn_WMCtlColor                              [internal]
 */
LRESULT CFn_WMCtlColor(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
  LPCHOOSEFONT lpcf=(LPCHOOSEFONT)GetWindowLong(hDlg, DWL_USER); 

  if (lpcf->Flags & CF_EFFECTS)
   if (HIWORD(lParam)==CTLCOLOR_STATIC && GetDlgCtrlID(LOWORD(lParam))==stc6)
   {
     SetTextColor(wParam,lpcf->rgbColors);
     return GetStockObject(WHITE_BRUSH);
   }
  return 0;
}

/***********************************************************************
 *           CFn_WMCommand                               [internal]
 */
LRESULT CFn_WMCommand(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
  char buffer[200];
  FARPROC enumCallback;
  HFONT hFont/*,hFontOld*/;
  int i,j;
  long l;
  HDC hdc;
  LPCHOOSEFONT lpcf=(LPCHOOSEFONT)GetWindowLong(hDlg, DWL_USER); 
  LPLOGFONT lpxx=PTR_SEG_TO_LIN(lpcf->lpLogFont);
  
  dprintf_commdlg(stddeb,"FormatCharDlgProc // WM_COMMAND lParam=%08lX\n", lParam);
  switch (wParam)
  {
	case cmb1:if (HIWORD(lParam)==CBN_SELCHANGE)
		  {
		    hdc=(lpcf->Flags & CF_PRINTERFONTS && lpcf->hDC) ? lpcf->hDC : GetDC(hDlg);
		    if (hdc)
		    {
		      /* only if cmb2 is refilled in  FontStyleEnumProc():
       		                 SendDlgItemMessage(hDlg,cmb2,CB_RESETCONTENT,0,0); 
		       */
		      SendDlgItemMessage(hDlg,cmb3,CB_RESETCONTENT,0,0);
		      i=SendDlgItemMessage(hDlg,cmb1,CB_GETCURSEL,0,0);
		      if (i!=CB_ERR)
		      {
		        HCURSOR hcursor=SetCursor(LoadCursor(0,IDC_WAIT));
                        SendDlgItemMessage(hDlg,cmb1,CB_GETLBTEXT,i,(LPARAM)MAKE_SEGPTR(buffer));
	                dprintf_commdlg(stddeb,"WM_COMMAND/cmb1 =>%s\n",buffer);
		        enumCallback = MODULE_GetWndProcEntry16("FontStyleEnumProc");
       		        EnumFontFamilies(hdc,buffer,enumCallback,
		             MAKELONG(GetDlgItem(hDlg,cmb2),GetDlgItem(hDlg,cmb3)));
		        SetCursor(hcursor);        
		      }
		      if (!(lpcf->Flags & CF_PRINTERFONTS && lpcf->hDC))
 		        ReleaseDC(hDlg,hdc);
 		    }
 		    else
                    {
                      dprintf_commdlg(stddeb,"WM_COMMAND: HDC failure !!!\n");
                      EndDialog (hDlg, 0); 
                      return TRUE;
                    }
	          }
	case chx1:
	case chx2:
	case cmb2:
	case cmb3:if (HIWORD(lParam)==CBN_SELCHANGE || HIWORD(lParam)== BN_CLICKED )
	          {
                    dprintf_commdlg(stddeb,"WM_COMMAND/cmb2,3 =%08lX\n", lParam);
		    i=SendDlgItemMessage(hDlg,cmb1,CB_GETCURSEL,0,0);
		    if (i==CB_ERR)
                      i=SendDlgItemMessage(hDlg,cmb1,WM_GETTEXT,20,(LPARAM)MAKE_SEGPTR(buffer));
                    else
                    {
		      SendDlgItemMessage(hDlg,cmb1,CB_GETLBTEXT,i,(LPARAM)MAKE_SEGPTR(buffer));
		      l=SendDlgItemMessage(hDlg,cmb1,CB_GETITEMDATA,i,0);
		      j=HIWORD(l);
		      lpcf->nFontType = LOWORD(l);
		      /* FIXME:   lpcf->nFontType |= ....  SIMULATED_FONTTYPE and so */
		      /* same value reported to the EnumFonts
		       call back with the extra FONTTYPE_...  bits added */
		      lpxx->lfPitchAndFamily=j&0xff;
		      lpxx->lfCharSet=j>>8;
                    }
                    strcpy(lpxx->lfFaceName,buffer);
		    i=SendDlgItemMessage(hDlg,cmb2,CB_GETCURSEL,0,0);
		    if (i!=CB_ERR)
		    {
		      l=SendDlgItemMessage(hDlg,cmb2,CB_GETITEMDATA,i,0);
		      if (0!=(lpxx->lfItalic=HIWORD(l)))
		        lpcf->nFontType |= ITALIC_FONTTYPE;
		      if ((lpxx->lfWeight=LOWORD(l)) > FW_MEDIUM)
		        lpcf->nFontType |= BOLD_FONTTYPE;
		    }
		    i=SendDlgItemMessage(hDlg,cmb3,CB_GETCURSEL,0,0);
		    if (i!=CB_ERR)
		    {
		      l=SendDlgItemMessage(hDlg,cmb3,CB_GETITEMDATA,i,0);
		      lpxx->lfHeight=-LOWORD(l);
		      lpxx->lfWidth = 0;  /* FYI: lfWidth is in HIWORD(l); */
		    }
		    lpxx->lfStrikeOut=IsDlgButtonChecked(hDlg,chx1);
		    lpxx->lfUnderline=IsDlgButtonChecked(hDlg,chx2);
		    lpxx->lfOrientation=lpxx->lfEscapement=0;
		    lpxx->lfOutPrecision=OUT_DEFAULT_PRECIS;
		    lpxx->lfClipPrecision=CLIP_DEFAULT_PRECIS;
		    lpxx->lfQuality=DEFAULT_QUALITY;

		    hFont=CreateFontIndirect(lpxx);
		    if (hFont)
		      SendDlgItemMessage(hDlg,stc6,WM_SETFONT,hFont,TRUE);
		    /* FIXME: Delete old font ...? */  
                  }
                  break;

	case cmb4:i=SendDlgItemMessage(hDlg,cmb4,CB_GETCURSEL,0,0);
		  if (i!=CB_ERR)
		  {
		   lpcf->rgbColors=textcolors[i];
		   InvalidateRect(GetDlgItem(hDlg,stc6),NULL,0);
		  }
		  break;
	
	case psh15:i=RegisterWindowMessage(MAKE_SEGPTR(HELPMSGSTRING));
		  if (lpcf->hwndOwner)
		    SendMessage(lpcf->hwndOwner,i,0,(LPARAM)lpcf);
		  if (CFn_HookCallChk(lpcf))
		    CallWindowProc(lpcf->lpfnHook,hDlg,WM_COMMAND,psh15,(LPARAM)lpcf);
		  break;

	case IDOK:EndDialog(hDlg, TRUE);
		  return(TRUE);
	case IDCANCEL:EndDialog(hDlg, FALSE);
		  return(TRUE);
	}
      return(FALSE);
}


/***********************************************************************
 *           FormatCharDlgProc   (COMMDLG.16)
             FIXME: 1. some strings are "hardcoded", but it's better load from sysres
                    2. some CF_.. flags are not supported
                    3. some TType extensions
 */
LRESULT FormatCharDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  LPCHOOSEFONT lpcf=(LPCHOOSEFONT)GetWindowLong(hDlg, DWL_USER);  
  if (message!=WM_INITDIALOG)
  {
   int res=0;
   if (!lpcf)
      return FALSE;
   if (CFn_HookCallChk(lpcf))
     res=CallWindowProc(lpcf->lpfnHook,hDlg,message,wParam,lParam);
   if (res)
    return res;
  }
  else
    return CFn_WMInitDialog(hDlg,wParam,lParam);
  switch (message)
    {
      case WM_MEASUREITEM:
                        return CFn_WMMeasureItem(hDlg,wParam,lParam);
      case WM_DRAWITEM:
                        return CFn_WMDrawItem(hDlg,wParam,lParam);
      case WM_CTLCOLOR:
                        return CFn_WMCtlColor(hDlg,wParam,lParam);
      case WM_COMMAND:
                        return CFn_WMCommand(hDlg,wParam,lParam);
      case WM_CHOOSEFONT_GETLOGFONT: 
                        /* FIXME:  current logfont back to caller */
                        break;
    }
  return FALSE;
}
