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

#define OPENFILEDLG2			11
#define SAVEFILEDLG2			12

static	DWORD 		CommDlgLastError = 0;

static	HBITMAP		hFolder = 0;
static	HBITMAP		hFolder2 = 0;
static	HBITMAP		hFloppy = 0;
static	HBITMAP		hHDisk = 0;
static	HBITMAP		hCDRom = 0;

int DOS_GetDefaultDrive(void);
void DOS_SetDefaultDrive(int drive);
char *DOS_GetCurrentDir(int drive);
int DOS_ChangeDir(int drive, char *dirname);

BOOL FileOpenDlgProc(HWND hWnd, WORD wMsg, WORD wParam, LONG lParam);
BOOL FileSaveDlgProc(HWND hWnd, WORD wMsg, WORD wParam, LONG lParam);
BOOL ColorDlgProc(HWND hWnd, WORD wMsg, WORD wParam, LONG lParam);
BOOL PrintDlgProc(HWND hWnd, WORD wMsg, WORD wParam, LONG lParam);
BOOL PrintSetupDlgProc(HWND hWnd, WORD wMsg, WORD wParam, LONG lParam);
BOOL ReplaceTextDlgProc(HWND hWnd, WORD wMsg, WORD wParam, LONG lParam);
BOOL FindTextDlgProc(HWND hWnd, WORD wMsg, WORD wParam, LONG lParam);

/***********************************************************************
 * COMMDLG_IsPathName [internal]
 */

static BOOL COMMDLG_IsPathName(LPSTR str)
{
  if (str[strlen(str)-1] == ':' && strlen(str) == 2) return TRUE;
  if (str[strlen(str)-1] == '\\') return TRUE;
  if (strchr(str,'*') != NULL) return TRUE;
  return FALSE;
}

/***********************************************************************
 * 				FileDlg_Init			[internal]
 */
static BOOL FileDlg_Init()
{
  if (!hFolder) hFolder = LoadBitmap(0, MAKEINTRESOURCE(OBM_FOLDER));
  if (!hFolder2) hFolder2 = LoadBitmap(0, MAKEINTRESOURCE(OBM_FOLDER2));
  if (!hFloppy) hFloppy = LoadBitmap(0, MAKEINTRESOURCE(OBM_FLOPPY));
  if (!hHDisk) hHDisk = LoadBitmap(0, MAKEINTRESOURCE(OBM_HDISK));
  if (!hCDRom) hCDRom = LoadBitmap(0, MAKEINTRESOURCE(OBM_CDROM));
  if (hFolder == 0 || hFolder2 == 0 || hFloppy == 0 || 
      hHDisk == 0 || hCDRom == 0)
  fprintf(stderr, "FileDlg_Init // Error loading bitmaps !");
  return TRUE;
}

/***********************************************************************
 *                              OpenDlg_FixDirName              [internal]
 */
void OpenDlg_FixDirName(LPSTR dirname)
{
  char temp[512];
  char* strp1;
  char* strp2;

  strp1=dirname;
  if( dirname[1] != ':'){
    temp[0]=(char)((char)DOS_GetDefaultDrive()+'A');
    temp[1]=':';
    temp[2]='\\';
    temp[3]= '\0';
    strcat(temp, DOS_GetCurrentDir(DOS_GetDefaultDrive()));
    if(dirname[0]=='.' && dirname[1]=='.') {
      strp2 = strrchr(temp, '\\');
      if (strp2 != NULL){
	*strp2='\0';
	strp1+=2;
      }
    }
    strcat(temp, "\\");
    strcat(temp, strp1);
    strcpy(dirname, temp);
  } 
}
  

/***********************************************************************
 * 				OpenDlg_ScanDir			[internal]
 */
static BOOL OpenDlg_ScanDir(HWND hWnd, LPSTR newPath)
{
  static HANDLE hStr = 0;
  static LPSTR  str  = NULL;
  static SEGPTR str16 = 0;
  LPSTR strp;

  OpenDlg_FixDirName(newPath);
  if (str == NULL)  {
    hStr = GlobalAlloc(0,512);
    str = GlobalLock(hStr);
    str16 = WIN16_GlobalLock(hStr);
  }

  strcpy(str,newPath);
  DlgDirList(hWnd, str, lst1, 0, 0x0000);
  strp = strrchr(str,'\\');
  if (strp == NULL)  {
    if (str[1] == ':') {
      strp = str+2;
    } else  {
      strp = str;
    }
  } else strp++;
  strcpy(str,strp);
  SendDlgItemMessage(hWnd,edt1,WM_SETTEXT, 0, str16);
  strcpy(str,"*.*");
  DlgDirList(hWnd, str, lst2, stc1, 0x8010);
  
  return TRUE;
}

/***********************************************************************
 * 				OpenDlg_GetFileType		[internal]
 */
static LPSTR OpenDlg_GetFileType(LPCSTR types, WORD index)
{
	int		n;
	int		i = 1;
	LPSTR 	ptr = (LPSTR) types;
	if	(ptr == NULL) return NULL;
	while((n = strlen(ptr)) != 0) {
		ptr += ++n;
		if (i++ == index) return ptr;
		n = strlen(ptr);
		ptr += ++n;
	}
	return NULL;
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
      dlgTemplate = sysres_DIALOG_3;
    }
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
 * 				FileOpenDlgProc			[COMMDLG.6]
 */
BOOL FileOpenDlgProc(HWND hWnd, WORD wMsg, WORD wParam, LONG lParam)
{
  int		n;
  LPSTR	  ptr;
  LPSTR	  fspec;
  WORD	  wRet;
  LONG    lRet;
  HBRUSH  hBrush;
  HDC 	  hMemDC;
  HBITMAP hBitmap;
  BITMAP  bm;
  LPMEASUREITEMSTRUCT lpmeasure;
  LPDRAWITEMSTRUCT lpdis;
  int  nDrive;
  static LPOPENFILENAME lpofn;/* FIXME - this won't multitask */
  
  SEGPTR tempsegp;
  
  static HANDLE hStr  = 0;
  static LPSTR  str   = NULL;
  static SEGPTR str16 = 0;
  
  if (str == NULL)  {
    hStr = GlobalAlloc(0,512);
    str = GlobalLock(hStr);
    str16 = WIN16_GlobalLock(hStr);
  }
  
  switch (wMsg) {
   case WM_INITDIALOG:
    printf("FileOpenDlgProc // WM_INITDIALOG lParam=%08lX\n", lParam);
    if (!FileDlg_Init()) return TRUE;
    SendDlgItemMessage(hWnd, cmb1, CB_RESETCONTENT, 0, 0);
    lpofn = (LPOPENFILENAME)lParam;
    ptr = (LPSTR)PTR_SEG_TO_LIN(lpofn->lpstrFilter);
    tempsegp = lpofn->lpstrFilter;
    while(*ptr) {
      n = strlen(ptr);
      SendDlgItemMessage(hWnd, cmb1, CB_ADDSTRING, 0, tempsegp);
      ptr += n + 1; tempsegp += n + 1;
      n = strlen(ptr);
      ptr += n + 1; tempsegp += n + 1;
    }
    /* set default filter */
    SendDlgItemMessage(hWnd, cmb1, CB_SETCURSEL,
		       lpofn->nFilterIndex - 1, 0L);
    /* get drive information into combo 2 */
    strcpy(str,"");
    DlgDirListComboBox(hWnd, str16, cmb2, 0, 0xC000);
    
    if (PTR_SEG_TO_LIN(lpofn->lpstrInitialDir) != NULL) { 
      strcpy(str, PTR_SEG_TO_LIN(lpofn->lpstrInitialDir));
      if (str[strlen(str)-1] != '\\' && str[strlen(str)-1] != ':') {
        strcat(str,"\\");
      }
    } else  {
      strcpy(str,"");
    }
    lRet = SendDlgItemMessage(hWnd, cmb1, CB_GETCURSEL, 0, 0);
    if (lRet == LB_ERR) return FALSE;
    fspec = OpenDlg_GetFileType(PTR_SEG_TO_LIN(lpofn->lpstrFilter), lRet + 1);
    strcat(str,fspec);
    
    if (!OpenDlg_ScanDir(hWnd, str)) {
      printf("OpenDlg_ScanDir // ChangeDir Error !\n");
    }
    /* select current drive in combo */
    nDrive = DOS_GetDefaultDrive();
    SendDlgItemMessage(hWnd, cmb2, CB_SETCURSEL, nDrive, 0L);
    
    ShowWindow(hWnd, SW_SHOWNORMAL);
    return TRUE;
    
   case WM_SHOWWINDOW:
    if (wParam == 0) break;
    if (!(lpofn->Flags & OFN_SHOWHELP)) {
      ShowWindow(GetDlgItem(hWnd, pshHelp), SW_HIDE);
    }
    if (lpofn->Flags & OFN_HIDEREADONLY) {
      ShowWindow(GetDlgItem(hWnd, chx1), SW_HIDE); 
    }
    return TRUE;

   case WM_MEASUREITEM:
    GetObject(hFolder2, sizeof(BITMAP), (LPSTR)&bm);
    lpmeasure = (LPMEASUREITEMSTRUCT)PTR_SEG_TO_LIN(lParam);
    lpmeasure->itemHeight = bm.bmHeight;
    return TRUE;
    
   case WM_DRAWITEM:
    if (lParam == 0L) break;
    lpdis = (LPDRAWITEMSTRUCT)PTR_SEG_TO_LIN(lParam);
    if ((lpdis->CtlType == ODT_LISTBOX) && (lpdis->CtlID == lst1)) {
      hBrush = SelectObject(lpdis->hDC, GetStockObject(LTGRAY_BRUSH));
      SelectObject(lpdis->hDC, hBrush);
      FillRect(lpdis->hDC, &lpdis->rcItem, hBrush);
      ptr = (LPSTR) PTR_SEG_TO_LIN(lpdis->itemData);
      if (ptr == NULL) break;
      TextOut(lpdis->hDC, lpdis->rcItem.left,	lpdis->rcItem.top,
	      ptr, strlen(ptr));
      if (lpdis->itemState != 0) {
	InvertRect(lpdis->hDC, &lpdis->rcItem);
      }
      return TRUE;
    }
    if ((lpdis->CtlType == ODT_LISTBOX) && (lpdis->CtlID == lst2)) {
      hBrush = SelectObject(lpdis->hDC, GetStockObject(LTGRAY_BRUSH));
      SelectObject(lpdis->hDC, hBrush);
      FillRect(lpdis->hDC, &lpdis->rcItem, hBrush);
      ptr = (LPSTR) PTR_SEG_TO_LIN(lpdis->itemData);
      if (ptr == NULL) break;
      hBitmap = hFolder;
      GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
      TextOut(lpdis->hDC, lpdis->rcItem.left + bm.bmWidth, 
	      lpdis->rcItem.top, ptr, strlen(ptr));
      hMemDC = CreateCompatibleDC(lpdis->hDC);
      SelectObject(hMemDC, hBitmap);
      BitBlt(lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top,
	     bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
      DeleteDC(hMemDC);
      if (lpdis->itemState != 0) {
	InvertRect(lpdis->hDC, &lpdis->rcItem);
      }
      return TRUE;
    }
    if ((lpdis->CtlType == ODT_COMBOBOX) && (lpdis->CtlID == cmb2)) {
      hBrush = SelectObject(lpdis->hDC, GetStockObject(LTGRAY_BRUSH));
      SelectObject(lpdis->hDC, hBrush);
      FillRect(lpdis->hDC, &lpdis->rcItem, hBrush);
      ptr = (LPSTR) PTR_SEG_TO_LIN(lpdis->itemData);
      if (ptr == NULL) break;
      switch(ptr[2]) {
       case 'a': case 'b':
	hBitmap = hFloppy;
	break;
       default:
	hBitmap = hHDisk;
	break;
      }
      GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
      TextOut(lpdis->hDC, lpdis->rcItem.left + bm.bmWidth, 
	      lpdis->rcItem.top, ptr, strlen(ptr));
      hMemDC = CreateCompatibleDC(lpdis->hDC);
      SelectObject(hMemDC, hBitmap);
      BitBlt(lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top,
	     bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
      DeleteDC(hMemDC);
      if (lpdis->itemState != 0) {
	InvertRect(lpdis->hDC, &lpdis->rcItem);
      }
      return TRUE;
    }
    break;
    
   case WM_COMMAND:
    switch (wParam) {
     case lst1:
      if (HIWORD(lParam) == LBN_DBLCLK) {
	lRet = SendDlgItemMessage(hWnd, lst1, LB_GETCURSEL, 0, 0L);
	if (lRet == LB_ERR) return 0;
	SendDlgItemMessage(hWnd, lst1, LB_GETTEXT, lRet, str16);
	SendDlgItemMessage(hWnd, edt1, WM_SETTEXT, 0, str16);
	return SendMessage(hWnd, WM_COMMAND, IDOK, 0);
      }
      break;
     case lst2:
      if (HIWORD(lParam) == LBN_DBLCLK) {
	lRet = SendDlgItemMessage(hWnd, lst2, LB_GETCURSEL, 0, 0L);
	if (lRet == LB_ERR) return 0;
	SendDlgItemMessage(hWnd, lst2, LB_GETTEXT, lRet, str16);

	if (str[0] == '[') {
	  str[strlen(str) - 1] = 0;
	  strcpy(str,str+1);
	}
	strcat(str,"\\"); 
	lRet = SendDlgItemMessage(hWnd, cmb1, CB_GETCURSEL, 0, 0);
	if (lRet == LB_ERR) return FALSE;
	fspec = OpenDlg_GetFileType(PTR_SEG_TO_LIN(lpofn->lpstrFilter), lRet + 1);
	strcat(str,"\\"); strcat(str,fspec);
	SendDlgItemMessage(hWnd, edt1, WM_SETTEXT, 0, str16);
	return SendMessage(hWnd, WM_COMMAND, IDOK, 0);
      }
      break;
     case cmb1:
      if (HIWORD(lParam) == CBN_SELCHANGE) {
	lRet = SendDlgItemMessage(hWnd, cmb1, CB_GETCURSEL, 0, 0L);
	if (lRet == LB_ERR) return FALSE;
	fspec = OpenDlg_GetFileType(PTR_SEG_TO_LIN(lpofn->lpstrFilter), lRet + 1);
	strcpy(str,fspec);
	SendDlgItemMessage(hWnd, edt1, WM_SETTEXT, 0, str16);
	return SendMessage(hWnd, WM_COMMAND, IDOK, 0);	
      }
      break;
     case cmb2:
      wRet = SendDlgItemMessage(hWnd, cmb2, CB_GETCURSEL, 0, 0L);
      if (wRet == (WORD)LB_ERR) return 0;
      SendDlgItemMessage(hWnd, cmb2, CB_GETLBTEXT, wRet, str16);
      str[0] = str[2]; str[1] = ':'; str[2] = 0;
      lRet = SendDlgItemMessage(hWnd, cmb1, CB_GETCURSEL, 0, 0L);
      if (lRet == LB_ERR) return FALSE;
      fspec = OpenDlg_GetFileType(PTR_SEG_TO_LIN(lpofn->lpstrFilter), lRet + 1);
      strcat(str,fspec);
      SendDlgItemMessage(hWnd, edt1, WM_SETTEXT, 0, str16);
      return SendMessage(hWnd, WM_COMMAND, IDOK, 0);	
      break;
     case chx1:
#ifdef DEBUG_OPENDLG
      printf("FileOpenDlgProc // read-only toggled !\n");
#endif
      break;
     case pshHelp:
#ifdef DEBUG_OPENDLG
      printf("FileOpenDlgProc // pshHelp pressed !\n");
#endif
      break;
     case IDOK:
      SendDlgItemMessage(hWnd, edt1, WM_GETTEXT, 511, str16);
      if (COMMDLG_IsPathName(str)) {
	OpenDlg_ScanDir(hWnd, str);
      } else  {	
	ShowWindow(hWnd, SW_HIDE); 
	strcpy(PTR_SEG_TO_LIN(lpofn->lpstrFile), str);
	lpofn->nFileOffset = 0;
	lpofn->nFileExtension = strlen(PTR_SEG_TO_LIN(lpofn->lpstrFile)) - 3;
	if (PTR_SEG_TO_LIN(lpofn->lpstrFileTitle) != NULL) {
	  wRet = SendDlgItemMessage(hWnd, lst1, LB_GETCURSEL, 0, 0L);
	  SendDlgItemMessage(hWnd, lst1, LB_GETTEXT, wRet, str16);
	  strcpy(PTR_SEG_TO_LIN(lpofn->lpstrFileTitle), str);
	}
	EndDialog(hWnd, TRUE);
      }
      return TRUE;
     case IDCANCEL:
      EndDialog(hWnd, FALSE);
      return TRUE;
    }
/*    return FALSE;*/
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
 * 				FileSaveDlgProc			[COMMDLG.7]
 */
BOOL FileSaveDlgProc(HWND hWnd, WORD wMsg, WORD wParam, LONG lParam)
{
  int		n;
  LPSTR	  ptr;
  LPSTR	  fspec;
  WORD	  wRet;
  LONG    lRet;
  HBRUSH  hBrush;
  HDC 	  hMemDC;
  HBITMAP hBitmap;
  BITMAP  bm;
  LPMEASUREITEMSTRUCT lpmeasure;
  LPDRAWITEMSTRUCT lpdis;
  int  nDrive;
  static LPOPENFILENAME lpofn;/* FIXME - this won't multitask */
  
  SEGPTR tempsegp;
  
  static HANDLE hStr  = 0;
  static LPSTR  str   = NULL;
  static SEGPTR str16 = 0;
  
  if (str == NULL)  {
    hStr = GlobalAlloc(0,512);
    str = GlobalLock(hStr);
    str16 = WIN16_GlobalLock(hStr);
  }
  
  switch (wMsg) {
   case WM_INITDIALOG:
    printf("FileSaveDlgProc // WM_INITDIALOG lParam=%08lX\n", lParam);
    if (!FileDlg_Init()) return TRUE;
    SendDlgItemMessage(hWnd, cmb1, CB_RESETCONTENT, 0, 0);
    lpofn = (LPOPENFILENAME)lParam;
    ptr = (LPSTR)PTR_SEG_TO_LIN(lpofn->lpstrFilter);
    tempsegp = lpofn->lpstrFilter;
    while(*ptr) {
      n = strlen(ptr);
      SendDlgItemMessage(hWnd, cmb1, CB_ADDSTRING, 0, tempsegp);
      ptr += n + 1; tempsegp += n + 1;
      n = strlen(ptr);
      ptr += n + 1; tempsegp += n + 1;
    }
    /* set default filter */
    SendDlgItemMessage(hWnd, cmb1, CB_SETCURSEL,
		       lpofn->nFilterIndex - 1, 0L);
    /* get drive information into combo 2 */
    strcpy(str,"");
    DlgDirListComboBox(hWnd, str16, cmb2, 0, 0xC000);
    
    if (PTR_SEG_TO_LIN(lpofn->lpstrInitialDir) != NULL) { 
      strcpy(str, PTR_SEG_TO_LIN(lpofn->lpstrInitialDir));
      if (str[strlen(str)-1] != '\\' && str[strlen(str)-1] != ':') {
        strcat(str,"\\");
      }
    } else  {
      strcpy(str,"");
    }
    lRet = SendDlgItemMessage(hWnd, cmb1, CB_GETCURSEL, 0, 0);
    if (lRet == LB_ERR) return FALSE;
    fspec = OpenDlg_GetFileType(PTR_SEG_TO_LIN(lpofn->lpstrFilter), lRet + 1);
    strcat(str,fspec);
    
    if (!OpenDlg_ScanDir(hWnd, str)) {
      printf("SaveDlg_ScanDir // ChangeDir Error !\n");
    } 
    /* select current drive in combo */
    nDrive = DOS_GetDefaultDrive();
    SendDlgItemMessage(hWnd, cmb2, CB_SETCURSEL, nDrive, 0L);
    
    ShowWindow(hWnd, SW_SHOWNORMAL);
    return TRUE;
    
   case WM_SHOWWINDOW:
    if (wParam == 0) break;
    if (!(lpofn->Flags & OFN_SHOWHELP)) {
      ShowWindow(GetDlgItem(hWnd, pshHelp), SW_HIDE);
    }
    if (lpofn->Flags & OFN_HIDEREADONLY) {
      ShowWindow(GetDlgItem(hWnd, chx1), SW_HIDE); 
    }
    return TRUE;

   case WM_MEASUREITEM:
    GetObject(hFolder2, sizeof(BITMAP), (LPSTR)&bm);
    lpmeasure = (LPMEASUREITEMSTRUCT)PTR_SEG_TO_LIN(lParam);
    lpmeasure->itemHeight = bm.bmHeight;
    return TRUE;
    
   case WM_DRAWITEM:
    if (lParam == 0L) break;
    lpdis = (LPDRAWITEMSTRUCT)PTR_SEG_TO_LIN(lParam);
    if ((lpdis->CtlType == ODT_LISTBOX) && (lpdis->CtlID == lst1)) {
      hBrush = SelectObject(lpdis->hDC, GetStockObject(LTGRAY_BRUSH));
      SelectObject(lpdis->hDC, hBrush);
      FillRect(lpdis->hDC, &lpdis->rcItem, hBrush);
      ptr = (LPSTR) PTR_SEG_TO_LIN(lpdis->itemData);
      if (ptr == NULL) break;
      TextOut(lpdis->hDC, lpdis->rcItem.left,	lpdis->rcItem.top,
	      ptr, strlen(ptr));
      if (lpdis->itemState != 0) {
	InvertRect(lpdis->hDC, &lpdis->rcItem);
      }
      return TRUE;
    }
    if ((lpdis->CtlType == ODT_LISTBOX) && (lpdis->CtlID == lst2)) {
      hBrush = SelectObject(lpdis->hDC, GetStockObject(LTGRAY_BRUSH));
      SelectObject(lpdis->hDC, hBrush);
      FillRect(lpdis->hDC, &lpdis->rcItem, hBrush);
      ptr = (LPSTR) PTR_SEG_TO_LIN(lpdis->itemData);
      if (ptr == NULL) break;
      hBitmap = hFolder;
      GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
      TextOut(lpdis->hDC, lpdis->rcItem.left + bm.bmWidth, 
	      lpdis->rcItem.top, ptr, strlen(ptr));
      hMemDC = CreateCompatibleDC(lpdis->hDC);
      SelectObject(hMemDC, hBitmap);
      BitBlt(lpdis->hDC, lpdis->rcItem.left,	lpdis->rcItem.top,
	     bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
      DeleteDC(hMemDC);
      if (lpdis->itemState != 0) {
	InvertRect(lpdis->hDC, &lpdis->rcItem);
      }
      return TRUE;
    }
    if ((lpdis->CtlType == ODT_COMBOBOX) && (lpdis->CtlID == cmb2)) {
      hBrush = SelectObject(lpdis->hDC, GetStockObject(LTGRAY_BRUSH));
      SelectObject(lpdis->hDC, hBrush);
      FillRect(lpdis->hDC, &lpdis->rcItem, hBrush);
      ptr = (LPSTR) PTR_SEG_TO_LIN(lpdis->itemData);
      if (ptr == NULL) break;
      switch(ptr[2]) {
       case 'a': case 'b':
	hBitmap = hFloppy;
	break;
       default:
	hBitmap = hHDisk;
	break;
      }
      GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
      TextOut(lpdis->hDC, lpdis->rcItem.left + bm.bmWidth, 
	      lpdis->rcItem.top, ptr, strlen(ptr));
      hMemDC = CreateCompatibleDC(lpdis->hDC);
      SelectObject(hMemDC, hBitmap);
      BitBlt(lpdis->hDC, lpdis->rcItem.left,	lpdis->rcItem.top,
	     bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
      DeleteDC(hMemDC);
      if (lpdis->itemState != 0) {
	InvertRect(lpdis->hDC, &lpdis->rcItem);
      }
      return TRUE;
    }
    break;
    
   case WM_COMMAND:
    switch (wParam) {
     case lst1:
      if (HIWORD(lParam) == LBN_DBLCLK) {
	lRet = SendDlgItemMessage(hWnd, lst1, LB_GETCURSEL, 0, 0L);
	if (lRet == LB_ERR) return 0;
	SendDlgItemMessage(hWnd, lst1, LB_GETTEXT, lRet, str16);
	SendDlgItemMessage(hWnd, edt1, WM_SETTEXT, 0, str16);
	return SendMessage(hWnd, WM_COMMAND, IDOK, 0);
      }
      break;
     case lst2:
      if (HIWORD(lParam) == LBN_DBLCLK) {
	lRet = SendDlgItemMessage(hWnd, lst2, LB_GETCURSEL, 0, 0L);
	if (lRet == LB_ERR) return 0;
	SendDlgItemMessage(hWnd, lst2, LB_GETTEXT, lRet, str16);

	if (str[0] == '[') {
	  str[strlen(str) - 1] = 0;
	  strcpy(str,str+1);
	}
	strcat(str,"\\"); 
	lRet = SendDlgItemMessage(hWnd, cmb1, CB_GETCURSEL, 0, 0);
	if (lRet == LB_ERR) return FALSE;
	fspec = OpenDlg_GetFileType(PTR_SEG_TO_LIN(lpofn->lpstrFilter), lRet + 1);
	strcat(str,"\\"); strcat(str,fspec);
	SendDlgItemMessage(hWnd, edt1, WM_SETTEXT, 0, str16);
	return SendMessage(hWnd, WM_COMMAND, IDOK, 0);
      }
      break;
     case cmb1:
      if (HIWORD(lParam) == CBN_SELCHANGE) {
	lRet = SendDlgItemMessage(hWnd, cmb1, CB_GETCURSEL, 0, 0L);
	if (lRet == LB_ERR) return FALSE;
	fspec = OpenDlg_GetFileType(PTR_SEG_TO_LIN(lpofn->lpstrFilter), lRet + 1);
	strcpy(str,fspec);
	SendDlgItemMessage(hWnd, edt1, WM_SETTEXT, 0, str16);
	return SendMessage(hWnd, WM_COMMAND, IDOK, 0);	
      }
      break;
     case cmb2:
      wRet = SendDlgItemMessage(hWnd, cmb2, CB_GETCURSEL, 0, 0L);
      if (wRet == (WORD)LB_ERR) return 0;
      SendDlgItemMessage(hWnd, cmb2, CB_GETLBTEXT, wRet, str16);
      str[0] = str[2]; str[1] = ':'; str[2] = 0;
      lRet = SendDlgItemMessage(hWnd, cmb1, CB_GETCURSEL, 0, 0L);
      if (lRet == LB_ERR) return FALSE;
      fspec = OpenDlg_GetFileType(PTR_SEG_TO_LIN(lpofn->lpstrFilter), lRet + 1);
      strcat(str,fspec);
      SendDlgItemMessage(hWnd, edt1, WM_SETTEXT, 0, str16);
      return SendMessage(hWnd, WM_COMMAND, IDOK, 0);	
      break;
     case chx1:
#ifdef DEBUG_OPENDLG
      printf("FileSaveDlgProc // read-only toggled !\n");
#endif
      break;
     case pshHelp:
#ifdef DEBUG_OPENDLG
      printf("FileSaveDlgProc // pshHelp pressed !\n");
#endif
      break;
     case IDOK:
      SendDlgItemMessage(hWnd, edt1, WM_GETTEXT, 511, str16);
      if (COMMDLG_IsPathName(str)) {
	OpenDlg_ScanDir(hWnd, str);
      } else  {	
	ShowWindow(hWnd, SW_HIDE); 
	strcpy(PTR_SEG_TO_LIN(lpofn->lpstrFile), str);
	lpofn->nFileOffset = 0;
	lpofn->nFileExtension = strlen(PTR_SEG_TO_LIN(lpofn->lpstrFile)) - 3;
	if (PTR_SEG_TO_LIN(lpofn->lpstrFileTitle) != NULL) {
	  wRet = SendDlgItemMessage(hWnd, lst1, LB_GETCURSEL, 0, 0L);
	  SendDlgItemMessage(hWnd, lst1, LB_GETTEXT, wRet, str16);
	  strcpy(PTR_SEG_TO_LIN(lpofn->lpstrFileTitle), str);
	}
	EndDialog(hWnd, TRUE);
      }
      return TRUE;
     case IDCANCEL:
      EndDialog(hWnd, FALSE);
      return TRUE;
    }
/*    return FALSE;*/
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
 * 				ColorDlgProc			[COMMDLG.8]
 */
BOOL ColorDlgProc(HWND hWnd, WORD wMsg, WORD wParam, LONG lParam)
{
	switch (wMsg) {
		case WM_INITDIALOG:
			printf("ColorDlgProc // WM_INITDIALOG lParam=%08lX\n", lParam);
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
  switch (wMsg) {
   case WM_INITDIALOG:
    printf("FindTextDlgProc // WM_INITDIALOG lParam=%08lX\n", lParam);
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
 * 				ReplaceTextDlgProc		[COMMDLG.14]
 */
BOOL ReplaceTextDlgProc(HWND hWnd, WORD wMsg, WORD wParam, LONG lParam)
{
  switch (wMsg) {
   case WM_INITDIALOG:
    printf("ReplaceTextDlgProc // WM_INITDIALOG lParam=%08lX\n", lParam);
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
  switch (wMsg) {
   case WM_INITDIALOG:
    printf("PrintDlgProc // WM_INITDIALOG lParam=%08lX\n", lParam);
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
 * 				PrintSetupDlgProc		[COMMDLG.22]
 */
BOOL PrintSetupDlgProc(HWND hWnd, WORD wMsg, WORD wParam, LONG lParam)
{
  switch (wMsg) {
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
	int    	i, len;
	printf("GetFileTitle(%p %p %d); \n", lpFile, lpTitle, cbBuf);
	if (lpFile == NULL || lpTitle == NULL) return -1;
	len = strlen(lpFile);
	if (len == 0) return -1;
	if (strchr(lpFile, '*') != NULL) return -1;
	if (strchr(lpFile, '[') != NULL) return -1;
	if (strchr(lpFile, ']') != NULL) return -1;
	len--;
	if (lpFile[len] == '/' || lpFile[len] == '\\' || lpFile[len] == ':') return -1;
	for (i = len; i >= 0; i--) {
	  if (lpFile[i] == '/' ||  lpFile[i] == '\\' ||  lpFile[i] == ':') {
	    i++;
	    break;
	  }
	}
	printf("\n---> '%s' ", &lpFile[i]);
	len = min(cbBuf, strlen(&lpFile[i]) + 1);
	strncpy(lpTitle, &lpFile[i], len + 1);
	if (len != cbBuf)
		return len;
	else
		return 0;
}
