/*
 * COMMDLG - File Dialogs
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
#include "wine/unicode.h"
#include "ldt.h"
#include "heap.h"
#include "commdlg.h"
#include "dialog.h"
#include "dlgs.h"
#include "module.h"
#include "drive.h"
#include "debugtools.h"
#include "winproc.h"
#include "cderr.h"
#include "tweak.h"

DEFAULT_DEBUG_CHANNEL(commdlg);

#include "cdlg.h"

#define BUFFILE 512
#define BUFFILEALLOC 512 * sizeof(WCHAR)   

struct FSPRIVATE 
{
    HWND hwnd; /* file dialog window handle */
    BOOL hook; /* TRUE if the dialog is hooked */
    UINT lbselchstring; /* registered message id */
    UINT fileokstring; /* registered message id */
    LPARAM lParam; /* save original lparam */
    HANDLE16 hDlgTmpl16; /* handle for resource 16 */
    HANDLE16 hResource16; /* handle for allocated resource 16 */
    HANDLE16 hGlobal16; /* 16 bits mem block (resources) */
    LPCVOID template; /* template for 32 bits resource */
    BOOL open; /* TRUE if open dialog, FALSE if save dialog */
    OPENFILENAMEW *ofnW; /* original structure or work struct */
    OPENFILENAMEA *ofnA; /* original structure if 32bits ansi dialog */
    OPENFILENAME16 *ofn16; /* original structure if 16 bits dialog */
};


#define LFSPRIVATE struct FSPRIVATE *

#define LFS16 1
#define LFS32A 2
#define LFS32W 3
 
static const WCHAR FILE_star[] = {'*','.','*', 0};
static const WCHAR FILE_bslash[] = {'\\', 0};
static const WCHAR FILE_specc[] = {'%','c',':', 0};

static HICON16 hFolder = 0;
static HICON16 hFolder2 = 0;
static HICON16 hFloppy = 0;
static HICON16 hHDisk = 0;
static HICON16 hCDRom = 0;
static HICON16 hNet = 0;
static int fldrHeight = 0;
static int fldrWidth = 0;

#define OFN_PROP "FILEDLG_OFN"

static const char defaultfilter[]=" \0\0";
static char defaultopen[]="Open File";
static char defaultsave[]="Save as";

/***********************************************************************
 *
 * Windows 3.1 style OpenFileName/SaveFileName dialog
 *
 */

LRESULT WINAPI FileOpenDlgProc16(HWND16 hWnd, UINT16 wMsg, WPARAM16 wParam,
				 LPARAM lParam);
LRESULT WINAPI FileSaveDlgProc16(HWND16 hWnd, UINT16 wMsg, WPARAM16 wParam,
				 LPARAM lParam);

static LRESULT WINAPI FileOpenDlgProc(HWND hDlg, UINT msg,
                                      WPARAM wParam, LPARAM lParam);

/***********************************************************************
 * 				FileDlg_Init			[internal]
 */
static BOOL FileDlg_Init(void)
{
    static BOOL initialized = 0;
    CURSORICONINFO *fldrInfo;
    
    if (!initialized) {
	if (!hFolder) hFolder = LoadIcon16(0, MAKEINTRESOURCE16(OIC_FOLDER));
	if (!hFolder2) hFolder2 = LoadIcon16(0, MAKEINTRESOURCE16(OIC_FOLDER2));
	if (!hFloppy) hFloppy = LoadIcon16(0, MAKEINTRESOURCE16(OIC_FLOPPY));
	if (!hHDisk) hHDisk = LoadIcon16(0, MAKEINTRESOURCE16(OIC_HDISK));
	if (!hCDRom) hCDRom = LoadIcon16(0, MAKEINTRESOURCE16(OIC_CDROM));
	if (!hNet) hNet = LoadIcon16(0, MAKEINTRESOURCE16(OIC_NETWORK));
	if (hFolder == 0 || hFolder2 == 0 || hFloppy == 0 || 
	    hHDisk == 0 || hCDRom == 0 || hNet == 0)
	{
	    ERR("Error loading icons !\n");
	    return FALSE;
	}
	fldrInfo = (CURSORICONINFO *) GlobalLock16( hFolder2 );
	if (!fldrInfo)
	{	
	    ERR("Error measuring icons !\n");
	    return FALSE;
	}
	fldrHeight = fldrInfo -> nHeight;
	fldrWidth = fldrInfo -> nWidth;
	GlobalUnlock16( hFolder2 );
	initialized = TRUE;
    }
    return TRUE;
}


/***********************************************************************
 *           Get32BitsTemplate                                  [internal]
 *
 * Get a template (or FALSE if failure) when 16 bits dialogs are used
 * by a 32 bits application
 *
 */
BOOL Get32BitsTemplate(LFSPRIVATE lfs)
{
    LPOPENFILENAMEW ofnW = lfs->ofnW;
    HANDLE hDlgTmpl;

    if (ofnW->Flags & OFN_ENABLETEMPLATEHANDLE)
    {
	if (!(lfs->template = LockResource( ofnW->hInstance )))
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	    return FALSE;
	}
    }
    else if (ofnW->Flags & OFN_ENABLETEMPLATE)
    {
	HANDLE hResInfo;
        if (lfs->ofnA)
	    hResInfo = FindResourceA(lfs->ofnA->hInstance,
				 lfs->ofnA->lpTemplateName,
                                 RT_DIALOGA);
        else
	    hResInfo = FindResourceW(ofnW->hInstance,
				 ofnW->lpTemplateName,
                                 RT_DIALOGW);
        if (!hResInfo)
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
	    return FALSE;
	}
	if (!(hDlgTmpl = LoadResource(ofnW->hInstance,
				hResInfo)) ||
		    !(lfs->template = LockResource(hDlgTmpl)))
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	    return FALSE;
	}
    } else { /* get it from internal Wine resource */
	HANDLE hResInfo;
	if (!(hResInfo = FindResourceA(COMMDLG_hInstance32, 
             lfs->open? "OPEN_FILE":"SAVE_FILE", RT_DIALOGA)))
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
	    return FALSE;
        }
        if (!(hDlgTmpl = LoadResource(COMMDLG_hInstance32, hResInfo )) ||
                !(lfs->template = LockResource( hDlgTmpl )))
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
            return FALSE;
        }
    }
    return TRUE;
}


/***********************************************************************
 *           Get16BitsTemplate                                [internal]
 *
 * Get a template (FALSE if failure) when 16 bits dialogs are used
 * by a 16 bits application
 *
 */
BOOL Get16BitsTemplate(LFSPRIVATE lfs)
{
    LPOPENFILENAME16 ofn16 = lfs->ofn16;
    LPCVOID template;
    HGLOBAL16 hGlobal16 = 0;

    if (ofn16->Flags & OFN_ENABLETEMPLATEHANDLE)
        lfs->hDlgTmpl16 = ofn16->hInstance;
    else if (ofn16->Flags & OFN_ENABLETEMPLATE)
    {
	HANDLE16 hResInfo;
	if (!(hResInfo = FindResource16(ofn16->hInstance,
					ofn16->lpTemplateName,
                                        RT_DIALOG16)))
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
	    return FALSE;
	}
	if (!(lfs->hDlgTmpl16 = LoadResource16( ofn16->hInstance, hResInfo )))
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	    return FALSE;
	}
        lfs->hResource16 = lfs->hDlgTmpl16;
    }
    else
    { /* get resource from (32 bits) own Wine resource; convert it to 16 */
	HANDLE hResInfo, hDlgTmpl32;
        LPCVOID template32;
        DWORD size;

	if (!(hResInfo = FindResourceA(COMMDLG_hInstance32, 
               lfs->open ? "OPEN_FILE":"SAVE_FILE", RT_DIALOGA)))
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
	    return FALSE;
	}
	if (!(hDlgTmpl32 = LoadResource(COMMDLG_hInstance32, hResInfo )) ||
	    !(template32 = LockResource( hDlgTmpl32 )))
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	    return FALSE;
	}
        size = SizeofResource(GetModuleHandleA("COMDLG32"), hResInfo);
        hGlobal16 = GlobalAlloc16(0, size);
        if (!hGlobal16)
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_MEMALLOCFAILURE);
            ERR("alloc failure for %ld bytes\n", size);
            return FALSE;
        }
        template = GlobalLock16(hGlobal16);
        if (!template)
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_MEMLOCKFAILURE);
            ERR("global lock failure for %x handle\n", hGlobal16);
            GlobalFree16(hGlobal16);
            return FALSE;
        }
        ConvertDialog32To16((LPVOID)template32, size, (LPVOID)template);
        lfs->hDlgTmpl16 = hGlobal16;
        lfs->hGlobal16 = hGlobal16;
    }
    return TRUE;
}

/***********************************************************************
 *                              FILEDLG_StripEditControl        [internal]
 * Strip pathnames off the contents of the edit control.
 */
static void FILEDLG_StripEditControl(HWND hwnd)
{
    WCHAR temp[BUFFILE], *cp;

    GetDlgItemTextW( hwnd, edt1, temp, sizeof(temp) );
    cp = strrchrW(temp, '\\');
    if (cp != NULL) {
	lstrcpyW(temp, cp+1);
    }
    cp = strrchrW(temp, ':');
    if (cp != NULL) {
	lstrcpyW(temp, cp+1);
    }
    /* FIXME: shouldn't we do something with the result here? ;-) */
}



/***********************************************************************
 *                              FILEDLG_CallWindowProc          [internal]
 *
 *      Call the appropriate hook
 */
static BOOL FILEDLG_CallWindowProc(LFSPRIVATE lfs, UINT wMsg, WPARAM wParam,
                                   LPARAM lParam)
{
    if (lfs->ofn16)
    {
        return (BOOL) CallWindowProc16(
          (WNDPROC16)lfs->ofn16->lpfnHook, lfs->hwnd,
          (UINT16)wMsg, (WPARAM16)wParam, lParam);
    }
    if (lfs->ofnA)
    {
        return (BOOL) CallWindowProcA(
          (WNDPROC)lfs->ofnA->lpfnHook, lfs->hwnd,
          wMsg, wParam, lParam);
    }

    if (lfs->ofnW)
    {
        return (BOOL) CallWindowProcW(
          (WNDPROC)lfs->ofnW->lpfnHook, lfs->hwnd,
          wMsg, wParam, lParam);
    }
    return FALSE;
}


/***********************************************************************
 * 				FILEDLG_ScanDir			[internal]
 */
static BOOL FILEDLG_ScanDir(HWND hWnd, LPWSTR newPath)
{
    WCHAR		buffer[BUFFILE];
    HWND 		hdlg;

    lstrcpynW(buffer, newPath, sizeof(buffer));

    if ( !SetCurrentDirectoryW( newPath ))
        return FALSE;
    /* get the list of spec files */
    GetDlgItemTextW(hWnd, edt1, buffer, sizeof(buffer));
    /* list of files */
    if ((hdlg = GetDlgItem(hWnd, lst1)) != 0) {
        WCHAR*	scptr; /* ptr on semi-colon */
	WCHAR*	filter = buffer;

	TRACE("Using filter %s\n", debugstr_w(filter));
	SendMessageW(hdlg, LB_RESETCONTENT, 0, 0);
	while (filter) {
	    scptr = strchrW(filter, ';');
	    if (scptr)	*scptr = 0;
	    TRACE("Using file spec %s\n", debugstr_w(filter));
	    if (SendMessageW(hdlg, LB_DIR, 0, (LPARAM)filter) == LB_ERR)
	        return FALSE;
	    if (scptr) *scptr = ';';
	        filter = (scptr) ? (scptr + 1) : 0;
	 }
    }
    /* list of directories */
    lstrcpyW(buffer, FILE_star);
    return DlgDirListW(hWnd, buffer, lst2, stc1, DDL_EXCLUSIVE | DDL_DIRECTORY);
}

/***********************************************************************
 * 				FILEDLG_GetFileType		[internal]
 */

static LPWSTR FILEDLG_GetFileType(LPWSTR cfptr, LPWSTR fptr, WORD index)
{
  int n, i;
  i = 0;
  if (cfptr)
    for ( ;(n = lstrlenW(cfptr)) != 0; i++) 
      {
	cfptr += n + 1;
	if (i == index)
	  return cfptr;
	cfptr += lstrlenW(cfptr) + 1;
      }
  if (fptr)
    for ( ;(n = lstrlenW(fptr)) != 0; i++) 
      {
	fptr += n + 1;
	if (i == index)
	  return fptr;
	fptr += lstrlenW(fptr) + 1;
    }
  return (LPWSTR) FILE_star; /* FIXME */
}

/***********************************************************************
 *                              FILEDLG_WMDrawItem              [internal]
 */
static LONG FILEDLG_WMDrawItem(HWND hWnd, WPARAM wParam, LPARAM lParam,
       int savedlg, LPDRAWITEMSTRUCT lpdis)
{
    WCHAR *str;
    HICON hIcon;
    COLORREF oldText = 0, oldBk = 0;

    if (lpdis->CtlType == ODT_LISTBOX && lpdis->CtlID == lst1)
    {
        if (!(str = HeapAlloc(GetProcessHeap(), 0, BUFFILEALLOC))) return FALSE;
	SendMessageW(lpdis->hwndItem, LB_GETTEXT, lpdis->itemID, 
                      (LPARAM)str);

	if ((lpdis->itemState & ODS_SELECTED) && !savedlg)
	{
	    oldBk = SetBkColor( lpdis->hDC, GetSysColor( COLOR_HIGHLIGHT ) );
	    oldText = SetTextColor( lpdis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
	}
	if (savedlg)
	    SetTextColor(lpdis->hDC,GetSysColor(COLOR_GRAYTEXT) );

	ExtTextOutW(lpdis->hDC, lpdis->rcItem.left + 1,
                  lpdis->rcItem.top + 1, ETO_OPAQUE | ETO_CLIPPED,
                  &(lpdis->rcItem), str, lstrlenW(str), NULL);

	if (lpdis->itemState & ODS_SELECTED)
	    DrawFocusRect( lpdis->hDC, &(lpdis->rcItem) );

	if ((lpdis->itemState & ODS_SELECTED) && !savedlg)
	{
	    SetBkColor( lpdis->hDC, oldBk );
	    SetTextColor( lpdis->hDC, oldText );
	}
        HeapFree(GetProcessHeap(), 0, str);
	return TRUE;
    }

    if (lpdis->CtlType == ODT_LISTBOX && lpdis->CtlID == lst2)
    {
        if (!(str = HeapAlloc(GetProcessHeap(), 0, BUFFILEALLOC)))
            return FALSE;
	SendMessageW(lpdis->hwndItem, LB_GETTEXT, lpdis->itemID, 
                      (LPARAM)str);

	if (lpdis->itemState & ODS_SELECTED)
	{
	    oldBk = SetBkColor( lpdis->hDC, GetSysColor( COLOR_HIGHLIGHT ) );
	    oldText = SetTextColor( lpdis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
	}
	ExtTextOutW(lpdis->hDC, lpdis->rcItem.left + fldrWidth,
                  lpdis->rcItem.top + 1, ETO_OPAQUE | ETO_CLIPPED,
                  &(lpdis->rcItem), str, lstrlenW(str), NULL);

	if (lpdis->itemState & ODS_SELECTED)
	    DrawFocusRect( lpdis->hDC, &(lpdis->rcItem) );

	if (lpdis->itemState & ODS_SELECTED)
	{
	    SetBkColor( lpdis->hDC, oldBk );
	    SetTextColor( lpdis->hDC, oldText );
	}
	DrawIcon(lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top, hFolder);
        HeapFree(GetProcessHeap(), 0, str);
	return TRUE;
    }
    if (lpdis->CtlType == ODT_COMBOBOX && lpdis->CtlID == cmb2)
    {
        if (!(str = HeapAlloc(GetProcessHeap(), 0, BUFFILEALLOC)))
            return FALSE;
	SendMessageW(lpdis->hwndItem, CB_GETLBTEXT, lpdis->itemID, 
                      (LPARAM)str);
        switch(DRIVE_GetType( str[2] - 'a' ))
        {
        case TYPE_FLOPPY:  hIcon = hFloppy; break;
        case TYPE_CDROM:   hIcon = hCDRom; break;
        case TYPE_NETWORK: hIcon = hNet; break;
        case TYPE_HD:
        default:           hIcon = hHDisk; break;
        }
	if (lpdis->itemState & ODS_SELECTED)
	{
	    oldBk = SetBkColor( lpdis->hDC, GetSysColor( COLOR_HIGHLIGHT ) );
	    oldText = SetTextColor( lpdis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
	}
	ExtTextOutW(lpdis->hDC, lpdis->rcItem.left + fldrWidth,
                  lpdis->rcItem.top + 1, ETO_OPAQUE | ETO_CLIPPED,
                  &(lpdis->rcItem), str, lstrlenW(str), NULL);

	if (lpdis->itemState & ODS_SELECTED)
	{
	    SetBkColor( lpdis->hDC, oldBk );
	    SetTextColor( lpdis->hDC, oldText );
	}
	DrawIcon(lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top, hIcon);
        HeapFree(GetProcessHeap(), 0, str);
	return TRUE;
    }
    return FALSE;
}

/***********************************************************************
 *                              FILEDLG_WMMeasureItem           [internal]
 */
static LONG FILEDLG_WMMeasureItem(HWND hWnd, WPARAM wParam, LPARAM lParam) 
{
    LPMEASUREITEMSTRUCT lpmeasure;
    
    lpmeasure = (LPMEASUREITEMSTRUCT)lParam;
    lpmeasure->itemHeight = fldrHeight;
    return TRUE;
}

/***********************************************************************
 *                              FILEDLG_WMMeasureItem16         [internal]
 */
static LONG FILEDLG_WMMeasureItem16(HWND16 hWnd, WPARAM16 wParam, LPARAM lParam) 
{
    LPMEASUREITEMSTRUCT16 lpmeasure;
    
    lpmeasure = (LPMEASUREITEMSTRUCT16)PTR_SEG_TO_LIN(lParam);
    lpmeasure->itemHeight = fldrHeight;
    return TRUE;
}

/***********************************************************************
 *                              FILEDLG_WMInitDialog            [internal]
 */

static LONG FILEDLG_WMInitDialog(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  int i, n;
  WCHAR tmpstr[BUFFILE];
  LPWSTR pstr, old_pstr;
  LPOPENFILENAMEW ofn;
  LFSPRIVATE lfs = (LFSPRIVATE) lParam;

  if (!lfs) return FALSE;
  SetPropA(hWnd, OFN_PROP, (HANDLE)lfs);
  lfs->hwnd = hWnd;
  ofn = lfs->ofnW;

  TRACE("flags=%lx initialdir=%s\n", ofn->Flags, debugstr_w(ofn->lpstrInitialDir));

  SetWindowTextW( hWnd, ofn->lpstrTitle );
  /* read custom filter information */
  if (ofn->lpstrCustomFilter)
    {
      pstr = ofn->lpstrCustomFilter;
      n = 0;
      TRACE("lpstrCustomFilter = %p\n", pstr);
      while(*pstr)
	{
	  old_pstr = pstr;
          i = SendDlgItemMessageW(hWnd, cmb1, CB_ADDSTRING, 0,
                                   (LPARAM)(ofn->lpstrCustomFilter) + n );
          n += lstrlenW(pstr) + 1;
	  pstr += lstrlenW(pstr) + 1;
	  TRACE("add str='%s' "
	  "associated to '%s'\n", debugstr_w(old_pstr), debugstr_w(pstr));
          SendDlgItemMessageW(hWnd, cmb1, CB_SETITEMDATA, i, (LPARAM)pstr);
          n += lstrlenW(pstr) + 1;
	  pstr += lstrlenW(pstr) + 1;
	}
    }
  /* read filter information */
  if (ofn->lpstrFilter) {
	pstr = (LPWSTR) ofn->lpstrFilter;
	n = 0;
	while(*pstr) {
	  old_pstr = pstr;
	  i = SendDlgItemMessageW(hWnd, cmb1, CB_ADDSTRING, 0,
				       (LPARAM)(ofn->lpstrFilter + n) );
	  n += lstrlenW(pstr) + 1;
	  pstr += lstrlenW(pstr) + 1;
	  TRACE("add str='%s' "
		  "associated to '%s'\n", debugstr_w(old_pstr), debugstr_w(pstr));
	  SendDlgItemMessageW(hWnd, cmb1, CB_SETITEMDATA, i, (LPARAM)pstr);
	  n += lstrlenW(pstr) + 1;
	  pstr += lstrlenW(pstr) + 1;
	}
  }
  /* set default filter */
  if (ofn->nFilterIndex == 0 && ofn->lpstrCustomFilter == NULL)
  	ofn->nFilterIndex = 1;
  SendDlgItemMessageW(hWnd, cmb1, CB_SETCURSEL, ofn->nFilterIndex - 1, 0);    
  lstrcpynW(tmpstr, FILEDLG_GetFileType(ofn->lpstrCustomFilter,
	     (LPWSTR)ofn->lpstrFilter, ofn->nFilterIndex - 1),BUFFILE);
  TRACE("nFilterIndex = %ld, SetText of edt1 to '%s'\n", 
  			ofn->nFilterIndex, debugstr_w(tmpstr));
  SetDlgItemTextW( hWnd, edt1, tmpstr );
  /* get drive list */
  *tmpstr = 0;
  DlgDirListComboBoxW(hWnd, tmpstr, cmb2, 0, DDL_DRIVES | DDL_EXCLUSIVE);
  /* read initial directory */
  if (ofn->lpstrInitialDir != NULL) 
    {
      int len;
      lstrcpynW(tmpstr, ofn->lpstrInitialDir, 511);
      len = lstrlenW(tmpstr);
      if (len > 0 && tmpstr[len-1] != '\\'  && tmpstr[len-1] != ':') {
        tmpstr[len]='\\';
        tmpstr[len+1]='\0';
      }
    }
  else
    *tmpstr = 0;
  if (!FILEDLG_ScanDir(hWnd, tmpstr)) {
    *tmpstr = 0;
    if (!FILEDLG_ScanDir(hWnd, tmpstr))
      WARN("Couldn't read initial directory %s!\n", debugstr_w(tmpstr));
  }
  /* select current drive in combo 2, omit missing drives */
  for(i = 0, n = -1; i <= DRIVE_GetCurrentDrive(); i++)
    if (DRIVE_IsValid(i))                  n++;
  SendDlgItemMessageW(hWnd, cmb2, CB_SETCURSEL, n, 0);
  if (!(ofn->Flags & OFN_SHOWHELP))
    ShowWindow(GetDlgItem(hWnd, pshHelp), SW_HIDE);
  if (ofn->Flags & OFN_HIDEREADONLY)
    ShowWindow(GetDlgItem(hWnd, chx1), SW_HIDE);
  if (lfs->hook)
      return (BOOL) FILEDLG_CallWindowProc(lfs, WM_INITDIALOG, wParam, lfs->lParam);
  return TRUE;
}

/***********************************************************************
 *                              FILEDLG_UpdateResult            [internal]
 *      update the displayed file name (with path) 
 */
void FILEDLG_UpdateResult(LFSPRIVATE lfs, WCHAR *tmpstr, WCHAR *tmpstr2)
{
    int lenstr2;
    LPOPENFILENAMEW ofnW = lfs->ofnW;

    GetCurrentDirectoryW(BUFFILE, tmpstr2);
    lenstr2 = lstrlenW(tmpstr2);
    if (lenstr2 > 3)
        tmpstr2[lenstr2++]='\\';
    lstrcpynW(tmpstr2+lenstr2, tmpstr, BUFFILE-lenstr2);
    if (ofnW->lpstrFile)
        lstrcpynW(ofnW->lpstrFile, tmpstr2, ofnW->nMaxFile);
    ofnW->nFileOffset = strrchrW(tmpstr2,'\\') - tmpstr2 +1;
    ofnW->nFileExtension = 0;
    while(tmpstr2[ofnW->nFileExtension] != '.' && tmpstr2[ofnW->nFileExtension] != '\0')
        ofnW->nFileExtension++;
    if (tmpstr2[ofnW->nFileExtension] == '\0')
        ofnW->nFileExtension = 0;
    else
        ofnW->nFileExtension++;
    /* update the real client structures if any */
    if (lfs->ofn16)
    {
        lstrcpynWtoA(PTR_SEG_TO_LIN(lfs->ofn16->lpstrFile), ofnW->lpstrFile, ofnW->nMaxFile);
        lfs->ofn16->nFileOffset = ofnW->nFileOffset;
        lfs->ofn16->nFileExtension = ofnW->nFileExtension;
    }
    if (lfs->ofnA)
    {
        lstrcpynWtoA(lfs->ofnA->lpstrFile, ofnW->lpstrFile, ofnW->nMaxFile);
        lfs->ofnA->nFileOffset = ofnW->nFileOffset;
        lfs->ofnA->nFileExtension = ofnW->nFileExtension;
    }
}


/***********************************************************************
 *                              FILEDLG_UpdateFileTitle         [internal]
 *      update the displayed file name (without path) 
 */
void FILEDLG_UpdateFileTitle(LFSPRIVATE lfs)
{
  LONG lRet;
  LPOPENFILENAMEW ofnW = lfs->ofnW;
  if (ofnW->lpstrFileTitle != NULL) 
  {
    lRet = SendDlgItemMessageW(lfs->hwnd, lst1, LB_GETCURSEL, 0, 0);
    SendDlgItemMessageW(lfs->hwnd, lst1, LB_GETTEXT, lRet,
                             (LPARAM)ofnW->lpstrFileTitle );
    if (lfs->ofn16)
        lstrcpynWtoA(PTR_SEG_TO_LIN(lfs->ofn16->lpstrFileTitle),ofnW->lpstrFileTitle,
                    ofnW->nMaxFileTitle);
    if (lfs->ofnA)
        lstrcpynWtoA(lfs->ofnA->lpstrFileTitle,ofnW->lpstrFileTitle,
                    ofnW->nMaxFileTitle);
  }
}

/***********************************************************************
 *                              FILEDLG_WMCommand               [internal]
 */
BOOL in_update=FALSE;

static LRESULT FILEDLG_WMCommand(HWND hWnd, LPARAM lParam, UINT notification,
       UINT control, LFSPRIVATE lfs ) 
{
  LONG lRet;
  OPENFILENAMEW ofn2;
  LPOPENFILENAMEW ofnW = lfs->ofnW;
  WCHAR tmpstr[BUFFILE], tmpstr2[BUFFILE];
  LPWSTR pstr, pstr2;

  switch (control)
    {
    case lst1: /* file list */
      FILEDLG_StripEditControl(hWnd);
      if (notification == LBN_DBLCLK)
	goto almost_ok;
      /* notify the app */
      lRet = SendDlgItemMessageW(hWnd, lst1, LB_GETCURSEL16, 0, 0);
      if (lRet == LB_ERR) return TRUE;
      if ((pstr = HeapAlloc(GetProcessHeap(), 0, BUFFILEALLOC)))
      {
          SendDlgItemMessageW(hWnd, lst1, LB_GETTEXT, lRet,
                               (LPARAM)pstr);
          SetDlgItemTextW( hWnd, edt1, pstr );
          HeapFree(GetProcessHeap(), 0, pstr);
      }
      if (lfs->hook)
          FILEDLG_CallWindowProc(lfs, lfs->lbselchstring, control,
                                 MAKELONG(lRet,CD_LBSELCHANGE));
      /* FIXME: for OFN_ALLOWMULTISELECT we need CD_LBSELSUB, CD_SELADD, CD_LBSELNOITEMS */
      return TRUE;

    case lst2: /* directory list */
      FILEDLG_StripEditControl(hWnd);
      if (notification == LBN_DBLCLK)
	{
          /* get the raw string (with brackets) */
	  lRet = SendDlgItemMessageW(hWnd, lst2, LB_GETCURSEL, 0, 0);
	  if (lRet == LB_ERR) return TRUE;
          pstr = HeapAlloc(GetProcessHeap(), 0, BUFFILEALLOC);
	  SendDlgItemMessageW(hWnd, lst2, LB_GETTEXT, lRet,
			     (LPARAM)pstr);
          lstrcpyW( tmpstr, pstr );
          HeapFree(GetProcessHeap(), 0, pstr);
          /* get the selected directory in tmpstr */
	  if (tmpstr[0] == '[')
	    {
	      tmpstr[lstrlenW(tmpstr) - 1] = 0;
	      lstrcpyW(tmpstr,tmpstr+1);
	    }
	  lstrcatW(tmpstr, FILE_bslash);
          /* directory *has* to be changed before notifying the hook */
          SetCurrentDirectoryW( tmpstr );
          /* notify the app */
          if (lfs->hook)
          {
              if (FILEDLG_CallWindowProc(lfs, lfs->lbselchstring, control,
                                 MAKELONG(lRet,CD_LBSELCHANGE)))
                  return TRUE;
          }
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
      lRet = SendDlgItemMessageW(hWnd, cmb2, CB_GETCURSEL, 0, 0L);
      if (lRet == LB_ERR) return 0;
      pstr = HeapAlloc(GetProcessHeap(), 0, BUFFILEALLOC);
      SendDlgItemMessageW(hWnd, cmb2, CB_GETLBTEXT, lRet,
                           (LPARAM)pstr);
      wsprintfW(tmpstr, FILE_specc, pstr[2]);
      HeapFree(GetProcessHeap(), 0, pstr);
      if (notification != CBN_SELCHANGE) return TRUE;

    /* on entry tmpstr points to the new path to scan */
    reset_scan:
      lRet = SendDlgItemMessageW(hWnd, cmb1, CB_GETCURSEL, 0, 0);
      if (lRet == LB_ERR)
	return TRUE;
      pstr = (LPWSTR)SendDlgItemMessageW(hWnd, cmb1, CB_GETITEMDATA, lRet, 0);
      TRACE("Selected filter : %s\n", debugstr_w(pstr));
      SetDlgItemTextW( hWnd, edt1, pstr );
      FILEDLG_ScanDir(hWnd, tmpstr);
      in_update=TRUE;

    case IDOK:
    almost_ok:

      ofn2 = *ofnW; /* for later restoring */
      /* get current file name */
      GetDlgItemTextW( hWnd, edt1, tmpstr, sizeof(tmpstr) );
      pstr = strrchrW(tmpstr, '\\');
      if (pstr == NULL)
	pstr = strrchrW(tmpstr, ':');

      if (strchrW(tmpstr,'*') != NULL || strchrW(tmpstr,'?') != NULL)
	{
	  /* edit control contains wildcards */
	  if (pstr != NULL)
	    {
	      lstrcpynW(tmpstr2, pstr+1, BUFFILE);
	      *(pstr+1) = 0;
	    }
	  else
	    {
	      lstrcpyW(tmpstr2, tmpstr);
	      *tmpstr=0;
	    }

	  TRACE("tmpstr=%s, tmpstr2=%s\n", debugstr_w(tmpstr), debugstr_w(tmpstr2));
          SetDlgItemTextW( hWnd, edt1, tmpstr2 );
	  FILEDLG_ScanDir(hWnd, tmpstr);
	  return TRUE;
	}
      /* no wildcards, we might have a directory or a filename */
      /* try appending a wildcard and reading the directory */
      pstr2 = tmpstr + lstrlenW(tmpstr);
      if (pstr == NULL || *(pstr+1) != 0)
	lstrcatW(tmpstr, FILE_bslash);
      lRet = SendDlgItemMessageW(hWnd, cmb1, CB_GETCURSEL, 0, 0);
      if (lRet == LB_ERR) return TRUE;
      ofnW->nFilterIndex = lRet + 1;
      TRACE("ofn->nFilterIndex=%ld\n", ofnW->nFilterIndex);
      lstrcpynW(tmpstr2, 
	     FILEDLG_GetFileType(ofnW->lpstrCustomFilter,
				 (LPWSTR) ofnW->lpstrFilter,
				 lRet), sizeof(tmpstr2));
      SetDlgItemTextW( hWnd, edt1, tmpstr2 );

      if (!in_update)
      {
          /* if ScanDir succeeds, we have changed the directory */
          if (FILEDLG_ScanDir(hWnd, tmpstr))
              return TRUE;
      }
      /* if not, this must be a filename */
      *pstr2 = 0; /* remove the wildcard added before */

      if (pstr != NULL)
	{
	  /* strip off the pathname */
	  *pstr = 0;
          SetDlgItemTextW( hWnd, edt1, pstr + 1 );
	  lstrcpynW(tmpstr2, pstr+1, sizeof(tmpstr2) );
	  /* Should we MessageBox() if this fails? */
	  if (!FILEDLG_ScanDir(hWnd, tmpstr)) return TRUE;
	  lstrcpyW(tmpstr, tmpstr2);
	}
      else SetDlgItemTextW( hWnd, edt1, tmpstr );
      FILEDLG_UpdateResult(lfs, tmpstr, tmpstr2);
      if(in_update)
       {
         if (lfs->hook)
             FILEDLG_CallWindowProc(lfs, lfs->lbselchstring, control,
                                 MAKELONG(lRet,CD_LBSELCHANGE));
         in_update = FALSE;
         return TRUE;
       }
      FILEDLG_UpdateFileTitle(lfs);
      if (lfs->hook)
      {
        lRet = (BOOL)FILEDLG_CallWindowProc(lfs, lfs->fileokstring,
                  0, lfs->lParam );
        if (lRet)       
        {
          *ofnW = ofn2; /* restore old state */
          break;
        }
      }
      if ((ofnW->Flags & OFN_ALLOWMULTISELECT) && (ofnW->Flags & OFN_EXPLORER)) {
	 if (ofnW->lpstrFile) {
	    LPWSTR str = (LPWSTR)ofnW->lpstrFile;
	    LPWSTR ptr = strrchrW(str, '\\');
	    str[lstrlenW(str) + 1] = '\0';
	    *ptr = 0;
	 }
      }
      EndDialog(hWnd, TRUE);
      return TRUE;
    case IDCANCEL:
      EndDialog(hWnd, FALSE);
      return TRUE;
    case IDABORT: /* can be sent by the hook procedure */
      EndDialog(hWnd, TRUE);
      return TRUE;
    }
  return FALSE;
}


/***********************************************************************
 *                              FILEDLG_MapDrawItemStruct       [internal]
 *      map a 16 bits drawitem struct to 32
 */
void FILEDLG_MapDrawItemStruct(LPDRAWITEMSTRUCT16 lpdis16, LPDRAWITEMSTRUCT lpdis)
{
    lpdis->CtlType = lpdis16->CtlType;
    lpdis->CtlID = lpdis16->CtlID;
    lpdis->itemID = lpdis16->itemID;
    lpdis->itemAction = lpdis16->itemAction;
    lpdis->itemState = lpdis16->itemState;
    lpdis->hwndItem = lpdis16->hwndItem;
    lpdis->hDC = lpdis16->hDC;
    lpdis->rcItem.right = lpdis16->rcItem.right;
    lpdis->rcItem.left = lpdis16->rcItem.left;
    lpdis->rcItem.top = lpdis16->rcItem.top;
    lpdis->rcItem.bottom = lpdis16->rcItem.bottom;
    lpdis->itemData = lpdis16->itemData;
}

/************************************************************************
 *                              FILEDLG_MapStringPairsToW       [internal]
 *      map string pairs to Unicode
 */
LPWSTR FILEDLG_MapStringPairsToW(LPCSTR strA, UINT size)
{
    LPCSTR s;
    LPWSTR x, y;
    int n;

    s = strA;
    while (*s)
        s = s+strlen(s)+1;
    s++;
    n = s - strA;
    if (n < size) n = size;

    x = y = HeapAlloc(GetProcessHeap(),0, n * sizeof(WCHAR));
    s = strA;
    while (*s) {
        lstrcpyAtoW(x, s);
        x += lstrlenW(x)+1;
        s += strlen(s)+1;
	}
    *x = 0;
    return y;
}


/************************************************************************
 *                              FILEDLG_DupToW                  [internal]
 *      duplicates an Ansi string to unicode, with a buffer size
 */
LPWSTR FILEDLG_DupToW(LPCSTR str, UINT size)
{
    LPWSTR strW;
    if (str && (size > 0))
    {
        strW = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
        lstrcpynAtoW(strW, str, size);
        return strW;
    }
    else
        return NULL;
}
 

/************************************************************************
 *                              FILEDLG_MapOfnStructA          [internal]
 *      map a 32 bits Ansi structure to an Unicode one
 */
void FILEDLG_MapOfnStructA(LPOPENFILENAMEA ofnA, LPOPENFILENAMEW ofnW, BOOL open)
{
    LPCSTR str;

    ofnW->lStructSize = sizeof(OPENFILENAMEW);
    ofnW->hwndOwner = ofnA->hwndOwner;
    ofnW->hInstance = ofnA->hInstance;
    if (ofnA->lpstrFilter)
        ofnW->lpstrFilter = FILEDLG_MapStringPairsToW(ofnA->lpstrFilter, 0);
    else
        ofnW->lpstrFilter = FILEDLG_MapStringPairsToW(defaultfilter, 0);

    if ((ofnA->lpstrCustomFilter) && (*(ofnA->lpstrCustomFilter)))
        ofnW->lpstrCustomFilter = FILEDLG_MapStringPairsToW(ofnA->lpstrCustomFilter, ofnA->nMaxCustFilter);
    ofnW->nMaxCustFilter = ofnA->nMaxCustFilter;
    ofnW->nFilterIndex = ofnA->nFilterIndex;
    ofnW->lpstrFile = FILEDLG_DupToW(ofnA->lpstrFile, ofnA->nMaxFile);
    ofnW->nMaxFile = ofnA->nMaxFile;
    ofnW->lpstrFileTitle = FILEDLG_DupToW(ofnA->lpstrFileTitle, ofnA->nMaxFileTitle);
    ofnW->nMaxFileTitle = ofnA->nMaxFileTitle;
    if (ofnA->lpstrInitialDir)
        ofnW->lpstrInitialDir = HEAP_strdupAtoW(GetProcessHeap(),0,ofnA->lpstrInitialDir);
    if (ofnA->lpstrTitle)
        str = ofnA->lpstrTitle;
    else 
        /* Allocates default title (FIXME : get it from resource) */
        str = open ? defaultopen:defaultsave;
    ofnW->lpstrTitle = HEAP_strdupAtoW(GetProcessHeap(),0, str);
    ofnW->Flags = ofnA->Flags;
    ofnW->nFileOffset = ofnA->nFileOffset;
    ofnW->nFileExtension = ofnA->nFileExtension;
    ofnW->lpstrDefExt = FILEDLG_DupToW(ofnA->lpstrDefExt, 3);
    if ((ofnA->Flags & OFN_ENABLETEMPLATE) && (ofnA->lpTemplateName))
    {
        if (HIWORD(ofnA->lpTemplateName))
            ofnW->lpTemplateName = HEAP_strdupAtoW(GetProcessHeap(), 0, ofnA->lpTemplateName);
        else /* numbered resource */
            ofnW->lpTemplateName = (LPWSTR) ofnA->lpTemplateName;
    }
}


/************************************************************************
 *                              FILEDLG_MapOfnStruct16          [internal]
 *      map a 16 bits structure to an Unicode one
 */
void FILEDLG_MapOfnStruct16(LPOPENFILENAME16 ofn16, LPOPENFILENAMEW ofnW, BOOL open)
{
    OPENFILENAMEA ofnA;
    /* first convert to linear pointers */
    memset(&ofnA, 0, sizeof(OPENFILENAMEA));
    ofnA.lStructSize = sizeof(OPENFILENAMEA);
    ofnA.hwndOwner = ofn16->hwndOwner;
    ofnA.hInstance = ofn16->hInstance;
    if (ofn16->lpstrFilter)
        ofnA.lpstrFilter = PTR_SEG_TO_LIN(ofn16->lpstrFilter);
    if (ofn16->lpstrCustomFilter)
        ofnA.lpstrCustomFilter = PTR_SEG_TO_LIN(ofn16->lpstrCustomFilter);
    ofnA.nMaxCustFilter = ofn16->nMaxCustFilter;
    ofnA.nFilterIndex = ofn16->nFilterIndex;
    ofnA.lpstrFile = PTR_SEG_TO_LIN(ofn16->lpstrFile);
    ofnA.nMaxFile = ofn16->nMaxFile;
    ofnA.lpstrFileTitle = PTR_SEG_TO_LIN(ofn16->lpstrFileTitle);
    ofnA.nMaxFileTitle = ofn16->nMaxFileTitle;
    ofnA.lpstrInitialDir = PTR_SEG_TO_LIN(ofn16->lpstrInitialDir);
    ofnA.lpstrTitle = PTR_SEG_TO_LIN(ofn16->lpstrTitle);
    ofnA.Flags = ofn16->Flags;
    ofnA.nFileOffset = ofn16->nFileOffset;
    ofnA.nFileExtension = ofn16->nFileExtension;
    ofnA.lpstrDefExt = PTR_SEG_TO_LIN(ofn16->lpstrDefExt);
    if (HIWORD(ofn16->lpTemplateName))
        ofnA.lpTemplateName = PTR_SEG_TO_LIN(ofn16->lpTemplateName);
    else
        ofnA.lpTemplateName = (LPSTR) ofn16->lpTemplateName; /* ressource number */
    /* now calls the 32 bits Ansi to Unicode version to complete the job */
    FILEDLG_MapOfnStructA(&ofnA, ofnW, open);
}


/************************************************************************
 *                              FILEDLG_DestroyPrivate            [internal]
 *      destroys the private object 
 */
void FILEDLG_DestroyPrivate(LFSPRIVATE lfs)
{
    HWND hwnd;
    if (!lfs) return;
    hwnd = lfs->hwnd;
    /* free resources for a 16 bits dialog */
    if (lfs->hResource16) FreeResource16(lfs->hResource16);
    if (lfs->hGlobal16)
    {
        GlobalUnlock16(lfs->hGlobal16);
        GlobalFree16(lfs->hGlobal16);
    }
    /* if ofnW has been allocated, have to free everything in it */
    if (lfs->ofn16 || lfs->ofnA)
    {
       LPOPENFILENAMEW ofnW = lfs->ofnW;
       if (ofnW->lpstrFilter) HeapFree(GetProcessHeap(), 0, (LPWSTR) ofnW->lpstrFilter);
       if (ofnW->lpstrCustomFilter) HeapFree(GetProcessHeap(), 0, ofnW->lpstrCustomFilter);
       if (ofnW->lpstrFile) HeapFree(GetProcessHeap(), 0, ofnW->lpstrFile);
       if (ofnW->lpstrFileTitle) HeapFree(GetProcessHeap(), 0, ofnW->lpstrFileTitle);
       if (ofnW->lpstrInitialDir) HeapFree(GetProcessHeap(), 0, (LPWSTR) ofnW->lpstrInitialDir);
       if (ofnW->lpstrTitle) HeapFree(GetProcessHeap(), 0, (LPWSTR) ofnW->lpstrTitle);
       if ((ofnW->lpTemplateName) && (HIWORD(ofnW->lpTemplateName)))
           HeapFree(GetProcessHeap(), 0, (LPWSTR) ofnW->lpTemplateName);
       HeapFree(GetProcessHeap(), 0, ofnW);
    }
    TRACE("destroying private allocation %p\n", lfs);
    HeapFree(GetProcessHeap(), 0, lfs);
    RemovePropA(hwnd, OFN_PROP);
}

/************************************************************************
 *                              FILEDLG_AllocPrivate            [internal]
 *      allocate a private object to hold 32 bits Unicode 
 *      structure that will be used throughtout the calls, while
 *      keeping available the original structures and a few variables
 *      On entry : type = dialog procedure type (16,32A,32W)
 *                 dlgType = dialog type (open or save)
 */
LFSPRIVATE FILEDLG_AllocPrivate(LPARAM lParam, int type, UINT dlgType)
{
    LFSPRIVATE lfs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct FSPRIVATE));
    LFSPRIVATE ret;
    TRACE("alloc private buf %p\n", lfs);
    if (!lfs) return NULL;
    lfs->hook = FALSE;
    lfs->lParam = lParam;
    if (dlgType == OPEN_DIALOG)
        lfs->open = TRUE;
    else
        lfs->open = FALSE;
    lfs->lbselchstring = RegisterWindowMessageA(LBSELCHSTRING);
    lfs->fileokstring = RegisterWindowMessageA(FILEOKSTRING);
    switch(type)
    {
        case LFS16:
        lfs->ofn16 = (LPOPENFILENAME16) PTR_SEG_TO_LIN(lParam);
        if (lfs->ofn16->Flags & OFN_ENABLEHOOK)
            if (lfs->ofn16->lpfnHook)
                lfs->hook = TRUE;

        break;

        case LFS32A:
        lfs->ofnA = (LPOPENFILENAMEA) lParam;
        if (lfs->ofnA->Flags & OFN_ENABLEHOOK)
            if (lfs->ofnA->lpfnHook)
                lfs->hook = TRUE;
        break;

        case LFS32W:
        lfs->ofnW = (LPOPENFILENAMEW) lParam;
        if (lfs->ofnW->Flags & OFN_ENABLEHOOK)
            if (lfs->ofnW->lpfnHook)
                lfs->hook = TRUE;
        break;
    }
    ret = lfs;
    if (!lfs->ofnW)
    { /* this structure is needed internally, so create it */
        lfs->ofnW = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(OPENFILENAMEW));
        if (lfs->ofnW)
        {
            if (lfs->ofn16)
                FILEDLG_MapOfnStruct16(lfs->ofn16, lfs->ofnW, lfs->open);
            if (lfs->ofnA)
                FILEDLG_MapOfnStructA(lfs->ofnA, lfs->ofnW, lfs->open);
        }
        else 
            ret = NULL;
    }
    if (lfs->ofn16)
    {
        if (!Get16BitsTemplate(lfs)) ret = NULL;
    }
    else
        if (!Get32BitsTemplate(lfs)) ret = NULL;
    if (!ret) FILEDLG_DestroyPrivate(lfs);
    return ret;
}


/***********************************************************************
 *           GetFileName31A                                 [internal]
 *
 * Creates a win31 style dialog box for the user to select a file to open/save.
 */
BOOL WINAPI GetFileName31A( 
                           LPOPENFILENAMEA lpofn, /* addess of structure with data*/
                           UINT dlgType /* type dialogue : open/save */
                          )
{
    HINSTANCE hInst;
    BOOL bRet = FALSE;
    LFSPRIVATE lfs;

    if (!lpofn || !FileDlg_Init()) return FALSE;

    lfs = FILEDLG_AllocPrivate((LPARAM) lpofn, LFS32A, dlgType);
    if (lfs)
    {
        hInst = GetWindowLongA( lpofn->hwndOwner, GWL_HINSTANCE );
        bRet = DialogBoxIndirectParamA( hInst, lfs->template, lpofn->hwndOwner, 
             (DLGPROC) FileOpenDlgProc, (DWORD) lfs);
        FILEDLG_DestroyPrivate(lfs);
    }

    TRACE("return lpstrFile='%s' !\n", lpofn->lpstrFile);
    return bRet;
}


/***********************************************************************
 *           GetFileName31W                                 [internal]
 *
 * Creates a win31 style dialog box for the user to select a file to open/save
 */
BOOL WINAPI GetFileName31W( 
                           LPOPENFILENAMEW lpofn, /* addess of structure with data*/
                           UINT dlgType /* type dialogue : open/save */
                          )
{
    HINSTANCE hInst;
    BOOL bRet = FALSE;
    LFSPRIVATE lfs;

    if (!lpofn || !FileDlg_Init()) return FALSE;

    lfs = FILEDLG_AllocPrivate((LPARAM) lpofn, LFS32W, dlgType);
    if (lfs)
    {
        hInst = GetWindowLongA( lpofn->hwndOwner, GWL_HINSTANCE );
        bRet = DialogBoxIndirectParamW( hInst, lfs->template, lpofn->hwndOwner, 
             (DLGPROC) FileOpenDlgProc, (DWORD) lfs);
        FILEDLG_DestroyPrivate(lfs);
    }

    TRACE("return lpstrFile='%s' !\n", debugstr_w(lpofn->lpstrFile));
    return bRet;
}


/* ------------------ Dialog procedures ---------------------- */

/***********************************************************************
 *           FileOpenDlgProc16   (COMMDLG.6)
 */
LRESULT WINAPI FileOpenDlgProc16(HWND16 hWnd, UINT16 wMsg, WPARAM16 wParam,
                               LPARAM lParam)
{  
    LFSPRIVATE lfs = (LFSPRIVATE)GetPropA(hWnd,OFN_PROP);
    DRAWITEMSTRUCT dis;
 
    TRACE("msg=%x wparam=%x lParam=%lx\n", wMsg, wParam, lParam);
    if ((wMsg != WM_INITDIALOG) && lfs && lfs->hook)
        {
            LRESULT lRet = (BOOL16)FILEDLG_CallWindowProc(lfs, wMsg, wParam, lParam);
            if (lRet)   
                return lRet;         /* else continue message processing */
        }
    switch (wMsg)
    {
    case WM_INITDIALOG:
        return FILEDLG_WMInitDialog(hWnd, wParam, lParam);

    case WM_MEASUREITEM:
        return FILEDLG_WMMeasureItem16(hWnd, wParam, lParam);

    case WM_DRAWITEM:
        FILEDLG_MapDrawItemStruct((LPDRAWITEMSTRUCT16)PTR_SEG_TO_LIN(lParam), &dis);
        return FILEDLG_WMDrawItem(hWnd, wParam, lParam, FALSE, &dis);

    case WM_COMMAND:
        return FILEDLG_WMCommand(hWnd, lParam, HIWORD(lParam),wParam, lfs);
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
 *           FileSaveDlgProc16   (COMMDLG.7)
 */
LRESULT WINAPI FileSaveDlgProc16(HWND16 hWnd, UINT16 wMsg, WPARAM16 wParam,
                               LPARAM lParam)
{
 LFSPRIVATE lfs = (LFSPRIVATE)GetPropA(hWnd,OFN_PROP);
 DRAWITEMSTRUCT dis;

 TRACE("msg=%x wparam=%x lParam=%lx\n", wMsg, wParam, lParam);
 if ((wMsg != WM_INITDIALOG) && lfs && lfs->hook)
  {
   LRESULT  lRet;
   lRet = (BOOL16)FILEDLG_CallWindowProc(lfs, wMsg, wParam, lParam);
   if (lRet)   
    return lRet;         /* else continue message processing */
  }             
  switch (wMsg) {
   case WM_INITDIALOG:
      return FILEDLG_WMInitDialog(hWnd, wParam, lParam);
      
   case WM_MEASUREITEM:
      return FILEDLG_WMMeasureItem16(hWnd, wParam, lParam);
    
   case WM_DRAWITEM:
      FILEDLG_MapDrawItemStruct((LPDRAWITEMSTRUCT16)PTR_SEG_TO_LIN(lParam), &dis);
      return FILEDLG_WMDrawItem(hWnd, wParam, lParam, TRUE, &dis);

   case WM_COMMAND:
      return FILEDLG_WMCommand(hWnd, lParam, HIWORD(lParam), wParam, lfs);
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
 *           FileOpenDlgProc                                    [internal]
 *      Used for open and save, in fact.   
 */
static LRESULT WINAPI FileOpenDlgProc(HWND hWnd, UINT wMsg,
                                      WPARAM wParam, LPARAM lParam)
{
    LFSPRIVATE lfs = (LFSPRIVATE)GetPropA(hWnd,OFN_PROP);

    TRACE("msg=%x wparam=%x lParam=%lx\n", wMsg, wParam, lParam);
    if ((wMsg != WM_INITDIALOG) && lfs && lfs->hook)
        {
            LRESULT lRet;
            lRet  = (BOOL)FILEDLG_CallWindowProc(lfs, wMsg, wParam, lParam);
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
        return FILEDLG_WMDrawItem(hWnd, wParam, lParam, !lfs->open, (DRAWITEMSTRUCT *)lParam);

    case WM_COMMAND:
        return FILEDLG_WMCommand(hWnd, lParam, HIWORD(wParam), LOWORD(wParam), lfs);
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

/* ------------------ APIs ---------------------- */ 

/***********************************************************************
 *           GetOpenFileName16   (COMMDLG.1)
 *
 * Creates a dialog box for the user to select a file to open.
 *
 * RETURNS
 *    TRUE on success: user selected a valid file
 *    FALSE on cancel, error, close or filename-does-not-fit-in-buffer.
 *
 * BUGS
 *    unknown, there are some FIXME's left.
 */
BOOL16 WINAPI GetOpenFileName16( 
				SEGPTR ofn /* addess of structure with data*/
				)
{
    HINSTANCE hInst;
    BOOL bRet = FALSE;
    LPOPENFILENAME16 lpofn = (LPOPENFILENAME16)PTR_SEG_TO_LIN(ofn);
    LFSPRIVATE lfs;
    FARPROC16 ptr;

    if (!lpofn || !FileDlg_Init()) return FALSE;

    lfs = FILEDLG_AllocPrivate((LPARAM) ofn, LFS16, OPEN_DIALOG);
    if (lfs)
    {
        hInst = GetWindowLongA( lpofn->hwndOwner, GWL_HINSTANCE );
        ptr = GetProcAddress16(GetModuleHandle16("COMMDLG"), (SEGPTR) 6);
        bRet = DialogBoxIndirectParam16( hInst, lfs->hDlgTmpl16, lpofn->hwndOwner, 
             (DLGPROC16) ptr, (DWORD) lfs);
        FILEDLG_DestroyPrivate(lfs);
    }

    TRACE("return lpstrFile='%s' !\n", 
           (LPSTR)PTR_SEG_TO_LIN(lpofn->lpstrFile));
    return bRet;
}

/***********************************************************************
 *           GetSaveFileName16   (COMMDLG.2)
 *
 * Creates a dialog box for the user to select a file to save.
 *
 * RETURNS
 *    TRUE on success: user enters a valid file
 *    FALSE on cancel, error, close or filename-does-not-fit-in-buffer.
 *
 * BUGS
 *    unknown. There are some FIXME's left.
 */
BOOL16 WINAPI GetSaveFileName16( 
				SEGPTR ofn /* addess of structure with data*/
				)
{
    HINSTANCE hInst;
    BOOL bRet = FALSE;
    LPOPENFILENAME16 lpofn = (LPOPENFILENAME16)PTR_SEG_TO_LIN(ofn);
    LFSPRIVATE lfs;
    FARPROC16 ptr;

    if (!lpofn || !FileDlg_Init()) return FALSE;

    lfs = FILEDLG_AllocPrivate((LPARAM) ofn, LFS16, SAVE_DIALOG);
    if (lfs)
    {
        hInst = GetWindowLongA( lpofn->hwndOwner, GWL_HINSTANCE );
        ptr = GetProcAddress16(GetModuleHandle16("COMMDLG"), (SEGPTR) 7);
        bRet = DialogBoxIndirectParam16( hInst, lfs->hDlgTmpl16, lpofn->hwndOwner, 
             (DLGPROC16) ptr, (DWORD) lfs);
        FILEDLG_DestroyPrivate(lfs);
    } 

    TRACE("return lpstrFile='%s' !\n", 
            (LPSTR)PTR_SEG_TO_LIN(lpofn->lpstrFile));
    return bRet;
}

/***********************************************************************
 *            GetOpenFileNameA  (COMDLG32.10)
 *
 * Creates a dialog box for the user to select a file to open.
 *
 * RETURNS
 *    TRUE on success: user enters a valid file
 *    FALSE on cancel, error, close or filename-does-not-fit-in-buffer.
 *
 */
BOOL WINAPI GetOpenFileNameA(
	LPOPENFILENAMEA ofn) /* address of init structure */
{
    BOOL  newlook;
    
    /* some flags don't allow to match the TWEAK_WineLook */
    if (ofn->Flags & (OFN_ALLOWMULTISELECT|OFN_ENABLEHOOK|OFN_ENABLETEMPLATE))
    {
      newlook = (ofn->Flags & OFN_EXPLORER) ? TRUE : FALSE;
    }
    else
    {
      /* no special flags set, we can match the TWEAK_WineLook */
      newlook = (TWEAK_WineLook>WIN31_LOOK) ? TRUE : FALSE;
    }

    if (newlook)
    {
        return GetFileDialog95A(ofn, OPEN_DIALOG);
    }
    else
    {
        return GetFileName31A(ofn, OPEN_DIALOG);
    }
}

/***********************************************************************
 *            GetOpenFileNameW (COMDLG32.11)
 *
 * Creates a dialog box for the user to select a file to open.
 *
 * RETURNS
 *    TRUE on success: user enters a valid file
 *    FALSE on cancel, error, close or filename-does-not-fit-in-buffer.
 *
 */
BOOL WINAPI GetOpenFileNameW(
	LPOPENFILENAMEW ofn) /* address of init structure */
{
    BOOL  newlook;
    
    /* some flags don't allow to match the TWEAK_WineLook */
    if (ofn->Flags & (OFN_ALLOWMULTISELECT|OFN_ENABLEHOOK|OFN_ENABLETEMPLATE))
    {
      newlook = (ofn->Flags & OFN_EXPLORER) ? TRUE : FALSE;
    }
    else
    {
      /* no special flags set, we can match the TWEAK_WineLook */
      newlook = (TWEAK_WineLook>WIN31_LOOK) ? TRUE : FALSE;
    }

    if (newlook)
    {
        return GetFileDialog95W(ofn, OPEN_DIALOG);
    }
    else
    {
        return GetFileName31W(ofn, OPEN_DIALOG);
    }
}

/***********************************************************************
 *            GetSaveFileNameA  (COMDLG32.12)
 *
 * Creates a dialog box for the user to select a file to save.
 *
 * RETURNS
 *    TRUE on success: user enters a valid file
 *    FALSE on cancel, error, close or filename-does-not-fit-in-buffer.
 *
 */
BOOL WINAPI GetSaveFileNameA(
	LPOPENFILENAMEA ofn) /* address of init structure */
{
    BOOL  newlook;
    
    /* some flags don't allow to match the TWEAK_WineLook */
    if (ofn->Flags & (OFN_ALLOWMULTISELECT|OFN_ENABLEHOOK|OFN_ENABLETEMPLATE))
    {
      newlook = (ofn->Flags & OFN_EXPLORER) ? TRUE : FALSE;
    }
    else
    {
      /* no special flags set, we can match the TWEAK_WineLook */
      newlook = (TWEAK_WineLook>WIN31_LOOK) ? TRUE : FALSE;
    }

    if (newlook)
    {
        return GetFileDialog95A(ofn, SAVE_DIALOG);
    } 
    else
    {
        return GetFileName31A(ofn, SAVE_DIALOG);
    }
}

/***********************************************************************
 *            GetSaveFileNameW  (COMDLG32.13)
 *
 * Creates a dialog box for the user to select a file to save.
 *
 * RETURNS
 *    TRUE on success: user enters a valid file
 *    FALSE on cancel, error, close or filename-does-not-fit-in-buffer.
 *
 */
BOOL WINAPI GetSaveFileNameW(
	LPOPENFILENAMEW ofn) /* address of init structure */
{
    BOOL  newlook;
    
    /* some flags don't allow to match the TWEAK_WineLook */
    if (ofn->Flags & (OFN_ALLOWMULTISELECT|OFN_ENABLEHOOK|OFN_ENABLETEMPLATE))
    {
      newlook = (ofn->Flags & OFN_EXPLORER) ? TRUE : FALSE;
    }
    else
    {
      /* no special flags set, we can match the TWEAK_WineLook */
      newlook = (TWEAK_WineLook>WIN31_LOOK) ? TRUE : FALSE;
    }

    if (newlook)
    {
        return GetFileDialog95W(ofn, SAVE_DIALOG);
    } 
    else
    {
        return GetFileName31W(ofn, SAVE_DIALOG);
    }
}
