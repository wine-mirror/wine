/*
 * COMMDLG - File Dialogs
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Albrecht Kleine
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "winbase.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "ldt.h"
#include "heap.h"
#include "commdlg.h"
#include "resource.h"
#include "dialog.h"
#include "dlgs.h"
#include "module.h"
#include "drive.h"
#include "debugtools.h"
#include "winproc.h"
#include "cderr.h"

DEFAULT_DEBUG_CHANNEL(commdlg)

#include "cdlg.h"

static HICON16 hFolder = 0;
static HICON16 hFolder2 = 0;
static HICON16 hFloppy = 0;
static HICON16 hHDisk = 0;
static HICON16 hCDRom = 0;
static HICON16 hNet = 0;
static int fldrHeight = 0;
static int fldrWidth = 0;

static const char defaultfilter[]=" \0\0";

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
 *           GetOpenFileName16   (COMMDLG.1)
 *
 * Creates a dialog box for the user to select a file to open.
 *
 * RETURNS
 *    TRUE on succes: user selected a valid file
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
		    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
		    return FALSE;
		}
	    }
	    else if (lpofn->Flags & OFN_ENABLETEMPLATE)
	    {
		if (!(hResInfo = FindResourceA(MapHModuleSL(lpofn->hInstance),
						PTR_SEG_TO_LIN(lpofn->lpTemplateName), RT_DIALOGA)))
		{
		    COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
		    return FALSE;
		}
		if (!(hDlgTmpl = LoadResource( MapHModuleSL(lpofn->hInstance),
						 hResInfo )) ||
		    !(template = LockResource( hDlgTmpl )))
		{
		    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
		    return FALSE;
		}
	    } else {
		if (!(hResInfo = FindResourceA(COMMDLG_hInstance32, "OPEN_FILE", RT_DIALOGA)))
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
	    }
	    win32Format = TRUE;
    } else {
	    if (lpofn->Flags & OFN_ENABLETEMPLATEHANDLE)
	    {
		if (!(template = LockResource16( lpofn->hInstance )))
		{
		    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
		    return FALSE;
		}
	    }
	    else if (lpofn->Flags & OFN_ENABLETEMPLATE)
	    {
		if (!(hResInfo = FindResource16(lpofn->hInstance,
						lpofn->lpTemplateName,
                                                RT_DIALOG16)))
		{
		    COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
		    return FALSE;
		}
		if (!(hDlgTmpl = LoadResource16( lpofn->hInstance, hResInfo )) ||
		    !(template = LockResource16( hDlgTmpl )))
		{
		    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
		    return FALSE;
		}
	    } else {
		if (!(hResInfo = FindResourceA(COMMDLG_hInstance32, "OPEN_FILE", RT_DIALOGA)))
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
    }

    hInst = GetWindowLongA( lpofn->hwndOwner, GWL_HINSTANCE );

    if (!(lpofn->lpstrFilter))
      {
       str = SEGPTR_ALLOC(sizeof(defaultfilter));
       TRACE("Alloc %p default for Filetype in GetOpenFileName\n",str);
       memcpy(str,defaultfilter,sizeof(defaultfilter));
       lpofn->lpstrFilter=SEGPTR_GET(str);
      }

    if (!(lpofn->lpstrTitle))
      {
       str1 = SEGPTR_ALLOC(strlen(defaultopen)+1);
       TRACE("Alloc %p default for Title in GetOpenFileName\n",str1);
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
       TRACE("Freeing %p default for Title in GetOpenFileName\n",str1);
        SEGPTR_FREE(str1);
       lpofn->lpstrTitle=0;
      }

    if (str)
      {
       TRACE("Freeing %p default for Filetype in GetOpenFileName\n",str);
        SEGPTR_FREE(str);
       lpofn->lpstrFilter=0;
      }

    if (hDlgTmpl) {
	    if (lpofn->Flags & OFN_WINE)
		    FreeResource( hDlgTmpl );
	    else
		    FreeResource16( hDlgTmpl );
    }

    TRACE("return lpstrFile='%s' !\n", 
           (LPSTR)PTR_SEG_TO_LIN(lpofn->lpstrFile));
    return bRet;
}


/***********************************************************************
 *           GetSaveFileName   (COMMDLG.2)
 *
 * Creates a dialog box for the user to select a file to save.
 *
 * RETURNS
 *    TRUE on succes: user enters a valid file
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
		    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
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
		    COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
		    return FALSE;
		}
		if (!(hDlgTmpl = LoadResource(MapHModuleSL(lpofn->hInstance),
						hResInfo)) ||
		    !(template = LockResource(hDlgTmpl)))
		{
		    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
		    return FALSE;
		}
		win32Format= TRUE;
	    } else {
		HANDLE hResInfo;
		if (!(hResInfo = FindResourceA(COMMDLG_hInstance32, "SAVE_FILE", RT_DIALOGA)))
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
    } else {
	    if (lpofn->Flags & OFN_ENABLETEMPLATEHANDLE)
	    {
		if (!(template = LockResource16( lpofn->hInstance )))
		{
		    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
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
		    COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
		    return FALSE;
		}
		if (!(hDlgTmpl = LoadResource16( lpofn->hInstance, hResInfo )) ||
		    !(template = LockResource16( hDlgTmpl )))
		{
		    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
		    return FALSE;
		}
	} else {
		HANDLE hResInfo;
		if (!(hResInfo = FindResourceA(COMMDLG_hInstance32, "SAVE_FILE", RT_DIALOGA)))
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
    }

    hInst = GetWindowLongA( lpofn->hwndOwner, GWL_HINSTANCE );

    if (!(lpofn->lpstrFilter))
      {
       str = SEGPTR_ALLOC(sizeof(defaultfilter));
       TRACE("Alloc default for Filetype in GetSaveFileName\n");
       memcpy(str,defaultfilter,sizeof(defaultfilter));
       lpofn->lpstrFilter=SEGPTR_GET(str);
      }

    if (!(lpofn->lpstrTitle))
      {
       str1 = SEGPTR_ALLOC(sizeof(defaultsave)+1);
       TRACE("Alloc default for Title in GetSaveFileName\n");
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
       TRACE("Freeing %p default for Title in GetSaveFileName\n",str1);
        SEGPTR_FREE(str1);
       lpofn->lpstrTitle=0;
      }
 
    if (str)
      {
       TRACE("Freeing %p default for Filetype in GetSaveFileName\n",str);
        SEGPTR_FREE(str);
       lpofn->lpstrFilter=0;
      }
    
    if (hDlgTmpl) {
	    if (lpofn->Flags & OFN_WINE)
		    FreeResource( hDlgTmpl );
	    else
		    FreeResource16( hDlgTmpl );
    }

    TRACE("return lpstrFile='%s' !\n", 
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

		 TRACE("Using filter %s\n", filter);
		 SendMessageA(hlb, LB_RESETCONTENT, 0, 0);
		 while (filter) {
			 scptr = strchr(filter, ';');
			 if (scptr)	*scptr = 0;
			 TRACE("Using file spec %s\n", filter);
			 if (SendMessageA(hlb, LB_DIR, 0, (LPARAM)filter) == LB_ERR)
				 return FALSE;
			 if (scptr) *scptr = ';';
			 filter = (scptr) ? (scptr + 1) : 0;
		 }
	 }

    strcpy(buffer, "*.*");
    return DlgDirListA(hWnd, buffer, lst2, stc1, DDL_EXCLUSIVE | DDL_DIRECTORY);
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
    HICON16 hIcon;
    COLORREF oldText = 0, oldBk = 0;

    if (lpdis->CtlType == ODT_LISTBOX && lpdis->CtlID == lst1)
    {
        if (!(str = SEGPTR_ALLOC(512))) return FALSE;
	SendMessage16(lpdis->hwndItem, LB_GETTEXT16, lpdis->itemID, 
                      (LPARAM)SEGPTR_GET(str));

	if ((lpdis->itemState & ODS_SELECTED) && !savedlg)
	{
	    oldBk = SetBkColor( lpdis->hDC, GetSysColor( COLOR_HIGHLIGHT ) );
	    oldText = SetTextColor( lpdis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
	}
	if (savedlg)
	    SetTextColor(lpdis->hDC,GetSysColor(COLOR_GRAYTEXT) );

	ExtTextOut16(lpdis->hDC, lpdis->rcItem.left + 1,
                  lpdis->rcItem.top + 1, ETO_OPAQUE | ETO_CLIPPED,
                  &(lpdis->rcItem), str, strlen(str), NULL);

	if (lpdis->itemState & ODS_SELECTED)
	    DrawFocusRect16( lpdis->hDC, &(lpdis->rcItem) );

	if ((lpdis->itemState & ODS_SELECTED) && !savedlg)
	{
	    SetBkColor( lpdis->hDC, oldBk );
	    SetTextColor( lpdis->hDC, oldText );
	}
        SEGPTR_FREE(str);
	return TRUE;
    }

    if (lpdis->CtlType == ODT_LISTBOX && lpdis->CtlID == lst2)
    {
        if (!(str = SEGPTR_ALLOC(512))) return FALSE;
	SendMessage16(lpdis->hwndItem, LB_GETTEXT16, lpdis->itemID, 
                      (LPARAM)SEGPTR_GET(str));

	if (lpdis->itemState & ODS_SELECTED)
	{
	    oldBk = SetBkColor( lpdis->hDC, GetSysColor( COLOR_HIGHLIGHT ) );
	    oldText = SetTextColor( lpdis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
	}
	ExtTextOut16(lpdis->hDC, lpdis->rcItem.left + fldrWidth,
                  lpdis->rcItem.top + 1, ETO_OPAQUE | ETO_CLIPPED,
                  &(lpdis->rcItem), str, strlen(str), NULL);

	if (lpdis->itemState & ODS_SELECTED)
	    DrawFocusRect16( lpdis->hDC, &(lpdis->rcItem) );

	if (lpdis->itemState & ODS_SELECTED)
	{
	    SetBkColor( lpdis->hDC, oldBk );
	    SetTextColor( lpdis->hDC, oldText );
	}
	DrawIcon(lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top, hFolder);
        SEGPTR_FREE(str);
	return TRUE;
    }
    if (lpdis->CtlType == ODT_COMBOBOX && lpdis->CtlID == cmb2)
    {
        if (!(str = SEGPTR_ALLOC(512))) return FALSE;
	SendMessage16(lpdis->hwndItem, CB_GETLBTEXT16, lpdis->itemID, 
                      (LPARAM)SEGPTR_GET(str));
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
	ExtTextOut16(lpdis->hDC, lpdis->rcItem.left + fldrWidth,
                  lpdis->rcItem.top + 1, ETO_OPAQUE | ETO_CLIPPED,
                  &(lpdis->rcItem), str, strlen(str), NULL);

	if (lpdis->itemState & ODS_SELECTED)
	{
	    SetBkColor( lpdis->hDC, oldBk );
	    SetTextColor( lpdis->hDC, oldText );
	}
	DrawIcon(lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top, hIcon);
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
    LPMEASUREITEMSTRUCT16 lpmeasure;
    
    lpmeasure = (LPMEASUREITEMSTRUCT16)PTR_SEG_TO_LIN(lParam);
    lpmeasure->itemHeight = fldrHeight;
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
			(WNDPROC16)lpofn->lpfnHook,hwnd,(UINT16)wMsg,(WPARAM16)wParam,lParam
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

                if (WINPROC_SetProc(&hWindowProc, (WNDPROC16)lpofn->lpfnHook, ProcType, WIN_PROC_WINDOW))
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
      TRACE("lpstrCustomFilter = %p\n", pstr);
      while(*pstr)
	{
	  old_pstr = pstr;
          i = SendDlgItemMessage16(hWnd, cmb1, CB_ADDSTRING16, 0,
                                   (LPARAM)lpofn->lpstrCustomFilter + n );
          n += strlen(pstr) + 1;
	  pstr += strlen(pstr) + 1;
	  TRACE("add str='%s' "
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
	  TRACE("add str='%s' "
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
  TRACE("nFilterIndex = %ld, SetText of edt1 to '%s'\n", 
  			lpofn->nFilterIndex, tmpstr);
  SetDlgItemTextA( hWnd, edt1, tmpstr );
  /* get drive list */
  *tmpstr = 0;
  DlgDirListComboBoxA(hWnd, tmpstr, cmb2, 0, DDL_DRIVES | DDL_EXCLUSIVE);
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
      WARN("Couldn't read initial directory %s!\n",tmpstr);
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
      TRACE("Selected filter : %s\n", pstr);
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
	  TRACE("tmpstr=%s, tmpstr2=%s\n", tmpstr, tmpstr2);
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
      TRACE("lpofn->nFilterIndex=%ld\n", lpofn->nFilterIndex);
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
	ofn16->lpfnHook = (LPOFNHOOKPROC16)ofn->lpfnHook;

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
	    if (ofn->lpstrFileTitle)
		strcpy(ofn->lpstrFileTitle,
			PTR_SEG_TO_LIN(ofn16->lpstrFileTitle));
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
	ofn16->lpfnHook = (LPOFNHOOKPROC16)ofn->lpfnHook;
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
	    if (ofn->lpstrFileTitle)
                lstrcpyAtoW(ofn->lpstrFileTitle,
			PTR_SEG_TO_LIN(ofn16->lpstrFileTitle));
	    SEGPTR_FREE(PTR_SEG_TO_LIN(ofn16->lpstrFileTitle));
	}
	SEGPTR_FREE(ofn16);
	return ret;
}

/***********************************************************************
 *            GetOpenFileNameA  (COMDLG32.10)
 *
 * Creates a dialog box for the user to select a file to open.
 *
 * RETURNS
 *    TRUE on succes: user enters a valid file
 *    FALSE on cancel, error, close or filename-does-not-fit-in-buffer.
 *
 * BUGS
 *    unknown, calls its 16-bit equivalent.
 */
BOOL WINAPI GetOpenFileNameA(
                             LPOPENFILENAMEA ofn /* address of init structure */
                             )
{
   BOOL16 (CALLBACK * dofunction)(SEGPTR ofn16) = GetOpenFileName16;
   return Commdlg_GetFileNameA(dofunction,ofn);
}

/***********************************************************************
 *            GetOpenFileNameW (COMDLG32.11)
 *
 * Creates a dialog box for the user to select a file to open.
 *
 * RETURNS
 *    TRUE on succes: user enters a valid file
 *    FALSE on cancel, error, close or filename-does-not-fit-in-buffer.
 *
 * BUGS
 *    unknown, calls its 16-bit equivalent.
 */
BOOL WINAPI GetOpenFileNameW(
                             LPOPENFILENAMEW ofn /* address of init structure */
                             )
{
   BOOL16 (CALLBACK * dofunction)(SEGPTR ofn16) = GetOpenFileName16;
   return Commdlg_GetFileNameW(dofunction,ofn);
}

/***********************************************************************
 *            GetSaveFileNameA  (COMDLG32.12)
 *
 * Creates a dialog box for the user to select a file to save.
 *
 * RETURNS
 *    TRUE on succes: user enters a valid file
 *    FALSE on cancel, error, close or filename-does-not-fit-in-buffer.
 *
 * BUGS
 *    unknown, calls its 16-bit equivalent.
 */
BOOL WINAPI GetSaveFileNameA(
                             LPOPENFILENAMEA ofn /* address of init structure */
                             )
{
   BOOL16 (CALLBACK * dofunction)(SEGPTR ofn16) = GetSaveFileName16;
   return Commdlg_GetFileNameA(dofunction,ofn);
}

/***********************************************************************
 *            GetSaveFileNameW  (COMDLG32.13)
 *
 * Creates a dialog box for the user to select a file to save.
 *
 * RETURNS
 *    TRUE on succes: user enters a valid file
 *    FALSE on cancel, error, close or filename-does-not-fit-in-buffer.
 *
 * BUGS
 *    unknown, calls its 16-bit equivalent.
 */
BOOL WINAPI GetSaveFileNameW(
                             LPOPENFILENAMEW ofn /* address of init structure */
                             )
{
   BOOL16 (CALLBACK * dofunction)(SEGPTR ofn16) = GetSaveFileName16;
   return Commdlg_GetFileNameW(dofunction,ofn);
}

