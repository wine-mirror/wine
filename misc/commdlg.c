/*
 * COMMDLG functions
 *
 * Copyright 1994 Martin Ayotte
 */

/*
#define DEBUG_OPENDLG
#define DEBUG_OPENDLG_DRAW
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dialog.h"
#include "win.h"
#include "user.h"
#include "message.h"
#include "library.h"
#include "commdlg.h"
#include "dlgs.h"

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

BOOL FileDlg_Init(HWND hWnd, DWORD lParam);
BOOL OpenDlg_ScanFiles(HWND hWnd, WORD nDrive, LPSTR newPath, LPSTR fileSpec);
BOOL OpenDlg_ScanDir(HWND hWnd, WORD nDrive, LPSTR newPath, LPSTR fileSpec);
LPSTR OpenDlg_GetFileType(LPCSTR types, WORD index);
LPSTR OpenDlg_ExtractCurDir(LPSTR FullPath, short MaxLen);
BOOL FileOpenDlgProc(HWND hWnd, WORD wMsg, WORD wParam, LONG lParam);
BOOL FileSaveDlgProc(HWND hWnd, WORD wMsg, WORD wParam, LONG lParam);
BOOL ColorDlgProc(HWND hWnd, WORD wMsg, WORD wParam, LONG lParam);
BOOL PrintDlgProc(HWND hWnd, WORD wMsg, WORD wParam, LONG lParam);
BOOL PrintSetupDlgProc(HWND hWnd, WORD wMsg, WORD wParam, LONG lParam);
BOOL ReplaceTextDlgProc(HWND hWnd, WORD wMsg, WORD wParam, LONG lParam);
BOOL FindTextDlgProc(HWND hWnd, WORD wMsg, WORD wParam, LONG lParam);

/***********************************************************************
 * 				GetOpenFileName			[COMMDLG.1]
 */
BOOL GetOpenFileName(LPOPENFILENAME lpofn)
{
	HANDLE		hDlgTmpl;
	HANDLE		hResInfo;
	HINSTANCE	hInst;
	WND 		*wndPtr;
	BOOL		bRet;
	printf("GetOpenFileName(%p); !\n", lpofn);
	if (lpofn == NULL) return FALSE;
	printf("GetOpenFileName // Flags=%08lX !\n", lpofn->Flags);
	printf("GetOpenFileName // nMaxFile=%ld lpstrFile='%s' !\n", 
						lpofn->nMaxFile, PTR_SEG_TO_LIN(lpofn->lpstrFile));
	printf("GetOpenFileName // lpstrInitialDir='%s' !\n", PTR_SEG_TO_LIN(lpofn->lpstrInitialDir));
	printf("GetOpenFileName // lpstrFilter=%p !\n", lpofn->lpstrFilter);
	printf("GetOpenFileName // nFilterIndex=%ld !\n", lpofn->nFilterIndex);
	if (lpofn->Flags & OFN_ENABLETEMPLATEHANDLE) {
		hDlgTmpl = lpofn->hInstance;
		}
	else {
		if (lpofn->Flags & OFN_ENABLETEMPLATE) {
			printf("GetOpenFileName // avant FindResource hInstance=%04X lpTemplateName='%s' !\n", 
								lpofn->hInstance, PTR_SEG_TO_LIN(lpofn->lpTemplateName));
			hInst = lpofn->hInstance;
			hResInfo = FindResource(hInst, 
				(LPSTR)lpofn->lpTemplateName, RT_DIALOG);
			}
		else {
			printf("GetOpenFileName // avant FindResource hSysRes=%04X !\n", hSysRes);
			hInst = hSysRes;
			hResInfo = FindResource(hInst, MAKEINTRESOURCE(OPENFILEDLG2), RT_DIALOG);
			}
		if (hResInfo == 0) {
			CommDlgLastError = CDERR_FINDRESFAILURE;
			return FALSE;
			}
		printf("GetOpenFileName // apres FindResource hResInfo=%04X!\n", hResInfo);
		hDlgTmpl = LoadResource(hInst, hResInfo);
		}
	if (hDlgTmpl == 0) {
		CommDlgLastError = CDERR_LOADRESFAILURE;
		return FALSE;
		}
	printf("GetOpenFileName // apres LoadResource hDlgTmpl=%04X!\n", hDlgTmpl);
    wndPtr = WIN_FindWndPtr(lpofn->hwndOwner);
	bRet = DialogBoxIndirectParam(wndPtr->hInstance, hDlgTmpl, 
		lpofn->hwndOwner, (WNDPROC)FileOpenDlgProc, (DWORD)lpofn); 

/*	strcpy(lpofn->lpstrFile, "SETUP.TXT"); */
/*	strcpy(lpofn->lpstrFileTitle, "SETUP.TXT");*/
/*
	lpofn->nFileOffset = 0;
	lpofn->nFileExtension = strlen(lpofn->lpstrFile) - 3;
	bRet = TRUE;
*/
	printf("GetOpenFileName // return lpstrFile='%s' !\n", PTR_SEG_TO_LIN(lpofn->lpstrFile));
	return bRet;
}


/***********************************************************************
 * 				GetSaveFileName			[COMMDLG.2]
 */
BOOL GetSaveFileName(LPOPENFILENAME lpofn)
{
	HANDLE		hDlgTmpl;
	HANDLE		hResInfo;
	HINSTANCE	hInst;
    WND 		*wndPtr;
	BOOL		bRet;
	printf("GetSaveFileName(%p); !\n", lpofn);
	if (lpofn == NULL) return FALSE;
	printf("GetSaveFileName // Flags=%08lX !\n", lpofn->Flags);
	printf("GetSaveFileName // nMaxFile=%ld lpstrFile='%s' !\n", 
						lpofn->nMaxFile, PTR_SEG_TO_LIN(lpofn->lpstrFile));
	printf("GetSaveFileName // lpstrInitialDir='%s' !\n", PTR_SEG_TO_LIN(lpofn->lpstrInitialDir));
	printf("GetSaveFileName // lpstrFilter=%p !\n", lpofn->lpstrFilter);
	if (lpofn->Flags & OFN_ENABLETEMPLATEHANDLE) {
		hDlgTmpl = lpofn->hInstance;
		}
	else {
		if (lpofn->Flags & OFN_ENABLETEMPLATE) {
			printf("GetSaveFileName // avant FindResource lpTemplateName='%s' !\n", 
                               PTR_SEG_TO_LIN(lpofn->lpTemplateName));
			hInst = lpofn->hInstance;
			hResInfo = FindResource(hInst, 
				(LPSTR)lpofn->lpTemplateName, RT_DIALOG);
			}
		else {
			printf("GetSaveFileName // avant FindResource !\n");
			hInst = hSysRes;
			hResInfo = FindResource(hInst, MAKEINTRESOURCE(SAVEFILEDLG2), RT_DIALOG);
			}
		if (hResInfo == 0) {
			CommDlgLastError = CDERR_FINDRESFAILURE;
			return FALSE;
			}
		hDlgTmpl = LoadResource(hInst, hResInfo);
		}
	if (hDlgTmpl == 0) {
		CommDlgLastError = CDERR_LOADRESFAILURE;
		return FALSE;
		}
    wndPtr = WIN_FindWndPtr(lpofn->hwndOwner);
	bRet = DialogBoxIndirectParam(wndPtr->hInstance, hDlgTmpl, 
		lpofn->hwndOwner, (WNDPROC)FileSaveDlgProc, (DWORD)lpofn);
	printf("GetSaveFileName // return lpstrFile='%s' !\n", PTR_SEG_TO_LIN(lpofn->lpstrFile));
	return bRet;
}


/***********************************************************************
 * 				ChooseColor				[COMMDLG.5]
 */
BOOL ChooseColor(LPCHOOSECOLOR lpChCol)
{
	HANDLE	hDlgTmpl;
	HANDLE	hResInfo;
    WND 	*wndPtr;
	BOOL	bRet;
	hResInfo = FindResource(hSysRes, MAKEINTRESOURCE(COLORDLG), RT_DIALOG);
	hDlgTmpl = LoadResource(hSysRes, hResInfo);
    wndPtr = WIN_FindWndPtr(lpChCol->hwndOwner);
	bRet = DialogBoxIndirectParam(wndPtr->hInstance, hDlgTmpl, 
		lpChCol->hwndOwner, (WNDPROC)ColorDlgProc, (DWORD)lpChCol);
	return bRet;
}


/***********************************************************************
 * 				FileOpenDlgProc			[COMMDLG.6]
 */
BOOL FileOpenDlgProc(HWND hWnd, WORD wMsg, WORD wParam, LONG lParam)
{
	int		n;
	LPSTR	ptr;
	LPSTR	fspec;
	char	str[512];
	char	C2[128];
	WORD	wRet;
	HBRUSH	hBrush;
	HDC 	hMemDC;
	HBITMAP	hBitmap;
	BITMAP	bm;
	LPMEASUREITEMSTRUCT lpmeasure;
	LPDRAWITEMSTRUCT lpdis;
	static	int		nDrive;
	static 	char	CurPath[512];
	static LPOPENFILENAME lpofn;

	switch (wMsg) {
		case WM_INITDIALOG:
#ifdef DEBUG_OPENDLG
			printf("FileOpenDlgProc // WM_INITDIALOG lParam=%08X\n", lParam);
#endif
			printf("FileOpenDlgProc // WM_INITDIALOG lParam=%08lX\n", lParam);
			if (!FileDlg_Init(hWnd, lParam)) return TRUE;
			SendDlgItemMessage(hWnd, cmb1, CB_RESETCONTENT, 0, 0L);
			lpofn = (LPOPENFILENAME)lParam;
			ptr = (LPSTR)PTR_SEG_TO_LIN(lpofn->lpstrFilter);
			strcpy(CurPath, PTR_SEG_TO_LIN(lpofn->lpstrInitialDir));
#ifdef DEBUG_OPENDLG
			printf("FileOpenDlgProc // lpstrInitialDir='%s' !\n", CurPath);
#endif
			while((n = strlen(ptr)) != 0) {
#ifdef DEBUG_OPENDLG
				printf("FileOpenDlgProc // file type '%s' !\n", ptr);
#endif
				SendDlgItemMessage(hWnd, cmb1, CB_ADDSTRING, 0, (DWORD)ptr);
				ptr += ++n;
#ifdef DEBUG_OPENDLG
				printf("FileOpenDlgProc // file spec '%s' !\n", ptr);
#endif
				n = strlen(ptr);
				ptr += ++n;
				}
			SendDlgItemMessage(hWnd, edt1, WM_SETTEXT, 0, (DWORD)str);
			SendDlgItemMessage(hWnd, cmb1, CB_SETCURSEL, 
						lpofn->nFilterIndex - 1, 0L);
			DlgDirListComboBox(hWnd, "", cmb2, 0, 0xC000);
			nDrive = 2; 		/* Drive 'C:' */
			SendDlgItemMessage(hWnd, cmb2, CB_SETCURSEL, nDrive, 0L);
			sprintf(str, "%c:\\%s", nDrive + 'A', DOS_GetCurrentDir(nDrive));
			fspec = OpenDlg_GetFileType(PTR_SEG_TO_LIN(lpofn->lpstrFilter), 
									lpofn->nFilterIndex);
#ifdef DEBUG_OPENDLG
			printf("FileOpenDlgProc // WM_INITDIALOG fspec #%d = '%s' !\n", 
											lpofn->nFilterIndex, fspec);
#endif
			if (!OpenDlg_ScanDir(hWnd, nDrive, str, fspec)) {
				printf("OpenDlg_ScanDir // ChangeDir Error !\n");
				}
			else {
				strcpy(CurPath, str);
				}
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
#ifdef DEBUG_OPENDLG_DRAW
		printf("FileOpenDlgProc WM_MEASUREITEM Height=%d !\n", bm.bmHeight);
#endif
		return TRUE;

	case WM_DRAWITEM:
#ifdef DEBUG_OPENDLG_DRAW
		printf("FileOpenDlgProc // WM_DRAWITEM w=%04X l=%08X\n", wParam, lParam);
#endif
		if (lParam == 0L) break;
		lpdis = (LPDRAWITEMSTRUCT)PTR_SEG_TO_LIN(lParam);
#ifdef DEBUG_OPENDLG_DRAW
		printf("FileOpenDlgProc // WM_DRAWITEM CtlType=%04X CtlID=%04X \n", 
									lpdis->CtlType, lpdis->CtlID);
#endif
		if ((lpdis->CtlType == ODT_LISTBOX) && (lpdis->CtlID == lst1)) {
			hBrush = SelectObject(lpdis->hDC, GetStockObject(LTGRAY_BRUSH));
			SelectObject(lpdis->hDC, hBrush);
			FillRect(lpdis->hDC, &lpdis->rcItem, hBrush);
			ptr = (LPSTR) PTR_SEG_TO_LIN(lpdis->itemData);
			if (ptr == NULL) break;
			TextOut(lpdis->hDC, lpdis->rcItem.left,	lpdis->rcItem.top, 
											ptr, strlen(ptr));
			}
		if ((lpdis->CtlType == ODT_LISTBOX) && (lpdis->CtlID == lst2)) {
			hBrush = SelectObject(lpdis->hDC, GetStockObject(LTGRAY_BRUSH));
			SelectObject(lpdis->hDC, hBrush);
			FillRect(lpdis->hDC, &lpdis->rcItem, hBrush);
			ptr = (LPSTR) PTR_SEG_TO_LIN(lpdis->itemData);
			if (ptr == NULL) break;
			if (strcmp(ptr, "[.]") == 0) {
				hBitmap = hFolder2;
/*				ptr = OpenDlg_ExtractCurDir(CurPath, -1); */
				ptr = CurPath;
				}
			else
				hBitmap = hFolder;
			GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
			TextOut(lpdis->hDC, lpdis->rcItem.left + bm.bmWidth, 
							lpdis->rcItem.top, ptr, strlen(ptr));
			hMemDC = CreateCompatibleDC(lpdis->hDC);
			SelectObject(hMemDC, hBitmap);
			BitBlt(lpdis->hDC, lpdis->rcItem.left,	lpdis->rcItem.top,
						bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
			DeleteDC(hMemDC);
			}
		if ((lpdis->CtlType == ODT_COMBOBOX) && (lpdis->CtlID == cmb2)) {
			hBrush = SelectObject(lpdis->hDC, GetStockObject(LTGRAY_BRUSH));
			SelectObject(lpdis->hDC, hBrush);
			FillRect(lpdis->hDC, &lpdis->rcItem, hBrush);
			ptr = (LPSTR) PTR_SEG_TO_LIN(lpdis->itemData);
			if (ptr == NULL) break;
			switch(ptr[2]) {
				case 'a':
				case 'b':
					hBitmap = hFloppy;
					break;
				default:
					hBitmap = hHDisk;
				}
			GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
			TextOut(lpdis->hDC, lpdis->rcItem.left + bm.bmWidth, 
							lpdis->rcItem.top, ptr, strlen(ptr));
			hMemDC = CreateCompatibleDC(lpdis->hDC);
			SelectObject(hMemDC, hBitmap);
			BitBlt(lpdis->hDC, lpdis->rcItem.left,	lpdis->rcItem.top,
						bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
			DeleteDC(hMemDC);
			}
		if (lpdis->itemState != 0) {
		    InvertRect(lpdis->hDC, &lpdis->rcItem);
		    }
        break;

	case WM_COMMAND:
		switch (wParam) {
			case lst1:
				if (HIWORD(lParam) == LBN_DBLCLK ||
					HIWORD(lParam) == LBN_SELCHANGE) {
#ifdef DEBUG_OPENDLG
					printf("FileOpenDlgProc // LBN_SELCHANGE on lst1 !\n");
#endif
					wRet = SendDlgItemMessage(hWnd, lst1, LB_GETCURSEL, 0, 0L);
					SendDlgItemMessage(hWnd, lst1, LB_GETTEXT, wRet, (DWORD)C2);
					}
				if (HIWORD(lParam) == LBN_DBLCLK) {
#ifdef DEBUG_OPENDLG
					printf("FileOpenDlgProc // LBN_DBLCLK on lst1 !\n");
#endif
					return SendMessage(hWnd, WM_COMMAND, IDOK, 0L);
					}
				break;
			case lst2:
				if (HIWORD(lParam) == LBN_DBLCLK) {
#ifdef DEBUG_OPENDLG
					printf("FileOpenDlgProc // LBN_DBLCLK on lst2 !\n");
#endif
					wRet = SendDlgItemMessage(hWnd, cmb1, CB_GETCURSEL, 0, 0L);
					if (wRet == (WORD)LB_ERR) return 0;
					fspec = OpenDlg_GetFileType(lpofn->lpstrFilter, wRet + 1);
					C2[0] = '\0';
					wRet = SendDlgItemMessage(hWnd, lst2, LB_GETCURSEL, 0, 0L);
					if (wRet == (WORD)LB_ERR) return 0;
					SendDlgItemMessage(hWnd, lst2, LB_GETTEXT, wRet, (DWORD)C2);
					if (C2[0] == '[') {
						C2[strlen(C2) - 1] = '\0';
						sprintf(str, "%s\\%s", CurPath, &C2[1]);
						if (!OpenDlg_ScanDir(hWnd, nDrive, str, fspec)) {
							printf("OpenDlg_ScanDir // ChangeDir Error !\n");
							}
						else {
							strcpy(CurPath, str);
							}
						}
					}
				break;
			case cmb1:
				if (HIWORD(lParam) == CBN_SELCHANGE) {
					wRet = SendDlgItemMessage(hWnd, cmb1, CB_GETCURSEL, 0, 0L);
					if (wRet == (WORD)LB_ERR) return 0;
					fspec = OpenDlg_GetFileType(lpofn->lpstrFilter, wRet + 1);
					printf("FileOpenDlgProc // new fspec #%d = '%s' !\n", wRet, fspec);
					if (!OpenDlg_ScanFiles(hWnd, nDrive, CurPath, fspec)) {
						printf("OpenDlg_ScanFiles // Change FileType Error !\n");
						}
					}
				break;
			case cmb2:
#ifdef DEBUG_OPENDLG
				printf("FileOpenDlgProc // combo #2 changed !\n");
#endif
				wRet = SendDlgItemMessage(hWnd, cmb1, CB_GETCURSEL, 0, 0L);
				if (wRet == (WORD)LB_ERR) return 0;
				fspec = OpenDlg_GetFileType(lpofn->lpstrFilter, wRet + 1);
				wRet = SendDlgItemMessage(hWnd, cmb2, CB_GETCURSEL, 0, 0L);
				if (wRet == (WORD)LB_ERR) return 0;
				printf("FileOpenDlgProc // combo #2 CB_GETCURSEL=%d !\n", wRet);
				SendDlgItemMessage(hWnd, cmb2, CB_GETLBTEXT, wRet, (DWORD)C2);
				nDrive = C2[2] - 'a';
#ifdef DEBUG_OPENDLG
				printf("FileOpenDlgProc // new drive selected=%d !\n", nDrive);
#endif
				sprintf(str, "%c:\\%s", nDrive + 'A', DOS_GetCurrentDir(nDrive));
#ifdef DEBUG_OPENDLG
				printf("FileOpenDlgProc // new drive , curPath='%s' !\n", str);
#endif
				if (!OpenDlg_ScanDir(hWnd, nDrive, str, fspec)) {
					printf("OpenDlg_ScanDir // ChangeDir Error !\n");
					}
				else {
					strcpy(CurPath, str);
					}
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
				ShowWindow(hWnd, SW_HIDE); 
				SendDlgItemMessage(hWnd, edt1, WM_GETTEXT, 0, (DWORD)str);
				wRet = SendDlgItemMessage(hWnd, lst1, LB_GETCURSEL, 0, 0L);
				SendDlgItemMessage(hWnd, lst1, LB_GETTEXT, wRet, (DWORD)str);
				printf("FileOpenDlgProc // IDOK str='%s'\n", str);
				strcpy(PTR_SEG_TO_LIN(lpofn->lpstrFile), str);
				lpofn->nFileOffset = 0;
				lpofn->nFileExtension = strlen(PTR_SEG_TO_LIN(lpofn->lpstrFile)) - 3;
				if (lpofn->lpstrFileTitle != NULL) {
					wRet = SendDlgItemMessage(hWnd, lst1, LB_GETCURSEL, 0, 0L);
					SendDlgItemMessage(hWnd, lst1, LB_GETTEXT, wRet, (DWORD)str);
					strcpy(PTR_SEG_TO_LIN(lpofn->lpstrFileTitle), str);
					}
				EndDialog(hWnd, TRUE);
				return(TRUE);
			case IDCANCEL:
				EndDialog(hWnd, FALSE);
				return(TRUE);
			}
		return(FALSE);
	}


/*
    case WM_CTLCOLOR:
        SetBkColor((HDC)wParam, 0x00C0C0C0);
        switch (HIWORD(lParam))
            {
            case CTLCOLOR_BTN:
                SetTextColor((HDC)wParam, 0x00000000);
                return(hGRAYBrush);
            case CTLCOLOR_STATIC:
                SetTextColor((HDC)wParam, 0x00000000);
                return(hGRAYBrush);
            }
        return(FALSE);

*/
	return FALSE;
}


/***********************************************************************
 * 				FileDlg_Init			[internal]
 */
BOOL FileDlg_Init(HWND hWnd, DWORD lParam)
{
	LPOPENFILENAME	lpofn;
	lpofn = (LPOPENFILENAME)lParam;
	if (lpofn == NULL) {
		fprintf(stderr, "FileDlg_Init // Bad LPOPENFILENAME pointer !");
		return FALSE;
		}
	if (hFolder == (HBITMAP)NULL) 
		hFolder = LoadBitmap((HINSTANCE)NULL, MAKEINTRESOURCE(OBM_FOLDER));
	if (hFolder2 == (HBITMAP)NULL) 
		hFolder2 = LoadBitmap((HINSTANCE)NULL, MAKEINTRESOURCE(OBM_FOLDER2));
	if (hFloppy == (HBITMAP)NULL) 
		hFloppy = LoadBitmap((HINSTANCE)NULL, MAKEINTRESOURCE(OBM_FLOPPY));
	if (hHDisk == (HBITMAP)NULL) 
		hHDisk = LoadBitmap((HINSTANCE)NULL, MAKEINTRESOURCE(OBM_HDISK));
	if (hCDRom == (HBITMAP)NULL) 
		hCDRom = LoadBitmap((HINSTANCE)NULL, MAKEINTRESOURCE(OBM_CDROM));
	if (hFolder == 0 || hFolder2 == 0 || hFloppy == 0 || 
		hHDisk == 0 || hCDRom == 0)
		fprintf(stderr, "FileDlg_Init // Error loading bitmaps !");
	return TRUE;
}


/***********************************************************************
 * 				OpenDlg_ScanFiles		[internal]
 */
BOOL OpenDlg_ScanFiles(HWND hWnd, WORD nDrive, LPSTR newPath, LPSTR fileSpec)
{
	int		OldDrive;
	char	OldPath[512];
	char	str[512];
	OldDrive = DOS_GetDefaultDrive();
	DOS_SetDefaultDrive(nDrive) ;
	strcpy(OldPath, DOS_GetCurrentDir(nDrive));
#ifdef DEBUG_OPENDLG
	printf("OpenDlg_ScanFiles // OldDrive=%d OldPath='%s'\n", OldDrive, OldPath);
	printf("OpenDlg_ScanFiles // wanted newPath='%s'\n", newPath);
#endif
	if (newPath[1] == ':')
		DOS_ChangeDir(nDrive, &newPath[2]);
	else
		DOS_ChangeDir(nDrive, newPath);
	sprintf(newPath, "%c:\\%s", nDrive + 'A', DOS_GetCurrentDir(nDrive));
#ifdef DEBUG_OPENDLG
	printf("OpenDlg_ScanFiles // actual newPath='%s'\n", newPath);
#endif
	if (strlen(newPath) == 3) newPath[2] = '\0';
	sprintf(str, "%s\\%s", newPath, fileSpec);
	DlgDirList(hWnd, str, lst1, 0, 0x0000);
	SendDlgItemMessage(hWnd, edt1, WM_SETTEXT, 0, (DWORD)str);
	DOS_ChangeDir(nDrive, OldPath);
	DOS_SetDefaultDrive(OldDrive);
	return TRUE;
}



/***********************************************************************
 * 				OpenDlg_ScanDir			[internal]
 */
BOOL OpenDlg_ScanDir(HWND hWnd, WORD nDrive, LPSTR newPath, LPSTR fileSpec)
{
	int		OldDrive;
	char	OldPath[512];
	char	str[512];
	OldDrive = DOS_GetDefaultDrive();
	DOS_SetDefaultDrive(nDrive) ;
	strcpy(OldPath, DOS_GetCurrentDir(nDrive));
#ifdef DEBUG_OPENDLG
	printf("OpenDlg_ScanDir // OldDrive=%d OldPath='%s'\n", OldDrive, OldPath);
	printf("OpenDlg_ScanDir // wanted newPath='%s'\n", newPath);
#endif
	if (newPath[1] == ':')
		DOS_ChangeDir(nDrive, &newPath[2]);
	else
		DOS_ChangeDir(nDrive, newPath);
	sprintf(newPath, "%c:\\%s", nDrive + 'A', DOS_GetCurrentDir(nDrive));
#ifdef DEBUG_OPENDLG
	printf("OpenDlg_ScanDir // actual newPath='%s'\n", newPath);
#endif
	if (strlen(newPath) == 3) newPath[2] = '\0';
	sprintf(str, "%s\\%s", newPath, fileSpec);
	DlgDirList(hWnd, str, lst1, 0, 0x0000);
	SendDlgItemMessage(hWnd, edt1, WM_SETTEXT, 0, (DWORD)str);
	sprintf(str, "%s\\*.*", newPath);
	DlgDirList(hWnd, str, lst2, 0, 0x8010);
	if (strlen(newPath) == 2) strcat(newPath, "\\");
	SendDlgItemMessage(hWnd, stc1, WM_SETTEXT, 0, (DWORD)newPath);
	DOS_ChangeDir(nDrive, OldPath);
	DOS_SetDefaultDrive(OldDrive);
	return TRUE;
}



/***********************************************************************
 * 				OpenDlg_GetFileType		[internal]
 */
LPSTR OpenDlg_GetFileType(LPCSTR types, WORD index)
{
	int		n;
	int		i = 1;
	LPSTR 	ptr = (LPSTR) types;
	if	(ptr == NULL) return NULL;
	while((n = strlen(ptr)) != 0) {
#ifdef DEBUG_OPENDLG
		printf("OpenDlg_GetFileType // file type '%s' !\n", ptr);
#endif
		ptr += ++n;
#ifdef DEBUG_OPENDLG
		printf("OpenDlg_GetFileType // file spec '%s' !\n", ptr);
#endif
		if (i++ == index) return ptr;
		n = strlen(ptr);
		ptr += ++n;
		}
	return NULL;
}


/***********************************************************************
 * 				OpenDlg_ExtractCurDir	[internal]
 */
LPSTR OpenDlg_ExtractCurDir(LPSTR FullPath, short MaxLen)
{
	LPSTR 	ptr;
	if (MaxLen < 0) MaxLen = strlen(FullPath);
	ptr = FullPath + MaxLen - 1;
	if (*ptr == '\\') return NULL;
	while (ptr > FullPath) {
		if (*ptr == '\\') return (ptr + 1);
		ptr--;
		}
	return NULL;
}


/***********************************************************************
 * 				FileSaveDlgProc			[COMMDLG.7]
 */
BOOL FileSaveDlgProc(HWND hWnd, WORD wMsg, WORD wParam, LONG lParam)
{
	int		n;
	LPSTR	ptr;
	LPSTR	fspec;
	char	str[512];
	char	C2[128];
	WORD	wRet;
	HBRUSH	hBrush;
	HDC 	hMemDC;
	HBITMAP	hBitmap;
	BITMAP	bm;
	LPMEASUREITEMSTRUCT lpmeasure;
	LPDRAWITEMSTRUCT lpdis;
	static	int		nDrive;
	static 	char	CurPath[512];
	static LPOPENFILENAME lpofn;

	switch (wMsg) {
		case WM_INITDIALOG:
#ifdef DEBUG_OPENDLG
			printf("FileSaveDlgProc // WM_INITDIALOG lParam=%08X\n", lParam);
#endif
			if (!FileDlg_Init(hWnd, lParam)) return TRUE;
			SendDlgItemMessage(hWnd, cmb1, CB_RESETCONTENT, 0, 0L);
			lpofn = (LPOPENFILENAME)lParam;
			ptr = (LPSTR)lpofn->lpstrFilter;
			strcpy(CurPath, lpofn->lpstrInitialDir);
			while((n = strlen(ptr)) != 0) {
#ifdef DEBUG_OPENDLG
				printf("FileSaveDlgProc // file type '%s' !\n", ptr);
#endif
				SendDlgItemMessage(hWnd, cmb1, CB_ADDSTRING, 0, (DWORD)ptr);
				ptr += ++n;
#ifdef DEBUG_OPENDLG
				printf("FileSaveDlgProc // file spec '%s' !\n", ptr);
#endif
				n = strlen(ptr);
				ptr += ++n;
				}
			SendDlgItemMessage(hWnd, edt1, WM_SETTEXT, 0, (DWORD)str);
			SendDlgItemMessage(hWnd, cmb1, CB_SETCURSEL, 
						lpofn->nFilterIndex - 1, 0L);
			DlgDirListComboBox(hWnd, "", cmb2, 0, 0xC000);
			nDrive = 2; 		/* Drive 'C:' */
			SendDlgItemMessage(hWnd, cmb2, CB_SETCURSEL, nDrive, 0L);
			sprintf(str, "%c:\\%s", nDrive + 'A', DOS_GetCurrentDir(nDrive));
			fspec = OpenDlg_GetFileType(lpofn->lpstrFilter, 
									lpofn->nFilterIndex);
#ifdef DEBUG_OPENDLG
			printf("FileSaveDlgProc // WM_INITDIALOG fspec #%d = '%s' !\n", 
											lpofn->nFilterIndex, fspec);
#endif
			if (!OpenDlg_ScanDir(hWnd, nDrive, str, fspec)) {
				printf("OpenDlg_ScanDir // ChangeDir Error !\n");
				}
			else {
				strcpy(CurPath, str);
				}
			ShowWindow(hWnd, SW_SHOWNORMAL);
			return (TRUE);

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
		lpmeasure = (LPMEASUREITEMSTRUCT)lParam;
		lpmeasure->itemHeight = bm.bmHeight;
#ifdef DEBUG_OPENDLG_DRAW
		printf("FileSaveDlgProc WM_MEASUREITEM Height=%d !\n", bm.bmHeight);
#endif
		return TRUE;

		case WM_DRAWITEM:
#ifdef DEBUG_OPENDLG_DRAW
			printf("FileSaveDlgProc // WM_DRAWITEM w=%04X l=%08X\n", wParam, lParam);
#endif
			if (lParam == 0L) break;
			lpdis = (LPDRAWITEMSTRUCT)lParam;
#ifdef DEBUG_OPENDLG_DRAW
			printf("FileSaveDlgProc // WM_DRAWITEM lpdis->CtlID=%04X\n", lpdis->CtlID);
#endif
			if ((lpdis->CtlType == ODT_LISTBOX) && (lpdis->CtlID == lst1)) {
				hBrush = SelectObject(lpdis->hDC, GetStockObject(LTGRAY_BRUSH));
				SelectObject(lpdis->hDC, hBrush);
				FillRect(lpdis->hDC, &lpdis->rcItem, hBrush);
				ptr = (LPSTR) lpdis->itemData;
				if (ptr == NULL) break;
				TextOut(lpdis->hDC, lpdis->rcItem.left,	lpdis->rcItem.top, 
														ptr, strlen(ptr));
				}
			if ((lpdis->CtlType == ODT_LISTBOX) && (lpdis->CtlID == lst2)) {
				hBrush = SelectObject(lpdis->hDC, GetStockObject(LTGRAY_BRUSH));
				SelectObject(lpdis->hDC, hBrush);
				FillRect(lpdis->hDC, &lpdis->rcItem, hBrush);
				ptr = (LPSTR) lpdis->itemData;
				if (ptr == NULL) break;
				if (strcmp(ptr, "[.]") == 0) {
					hBitmap = hFolder2;
					ptr = CurPath;
					}
				else
					hBitmap = hFolder;
				GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
				TextOut(lpdis->hDC, lpdis->rcItem.left + bm.bmWidth, 
								lpdis->rcItem.top, ptr, strlen(ptr));
				hMemDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(hMemDC, hBitmap);
				BitBlt(lpdis->hDC, lpdis->rcItem.left,	lpdis->rcItem.top,
							bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
				DeleteDC(hMemDC);
				}
			if ((lpdis->CtlType == ODT_COMBOBOX) && (lpdis->CtlID == cmb2)) {
				hBrush = SelectObject(lpdis->hDC, GetStockObject(LTGRAY_BRUSH));
				SelectObject(lpdis->hDC, hBrush);
				FillRect(lpdis->hDC, &lpdis->rcItem, hBrush);
				ptr = (LPSTR) lpdis->itemData;
				if (ptr == NULL) break;
				switch(ptr[2]) {
					case 'a':
					case 'b':
						hBitmap = hFloppy;
						break;
					default:
						hBitmap = hHDisk;
					}
				GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
				TextOut(lpdis->hDC, lpdis->rcItem.left + bm.bmWidth, 
								lpdis->rcItem.top, ptr, strlen(ptr));
				hMemDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(hMemDC, hBitmap);
				BitBlt(lpdis->hDC, lpdis->rcItem.left,	lpdis->rcItem.top,
							bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
				DeleteDC(hMemDC);
				}
	        break;

		case WM_COMMAND:
			switch (wParam) {
				case lst1:
					if (HIWORD(lParam) == LBN_DBLCLK ||
						HIWORD(lParam) == LBN_SELCHANGE) {
#ifdef DEBUG_OPENDLG
						printf("FileSaveDlgProc // LBN_SELCHANGE on lst1 !\n");
#endif
						wRet = SendDlgItemMessage(hWnd, lst1, LB_GETCURSEL, 0, 0L);
						SendDlgItemMessage(hWnd, lst1, LB_GETTEXT, wRet, (DWORD)C2);
						}
					if (HIWORD(lParam) == LBN_DBLCLK) {
#ifdef DEBUG_OPENDLG
						printf("FileSaveDlgProc // LBN_DBLCLK on lst1 !\n");
#endif
						return SendMessage(hWnd, WM_COMMAND, IDOK, 0L);
						}
					break;
				case lst2:
				if (HIWORD(lParam) == LBN_DBLCLK) {
#ifdef DEBUG_OPENDLG
					printf("FileSaveDlgProc // LBN_DBLCLK on lst2 !\n");
#endif
					wRet = SendDlgItemMessage(hWnd, cmb1, CB_GETCURSEL, 0, 0L);
					fspec = OpenDlg_GetFileType(lpofn->lpstrFilter, wRet + 1);
					C2[0] = '\0';
					wRet = SendDlgItemMessage(hWnd, lst2, LB_GETCURSEL, 0, 0L);
					SendDlgItemMessage(hWnd, lst2, LB_GETTEXT, wRet, (DWORD)C2);
					if (C2[0] == '[') {
						C2[strlen(C2) - 1] = '\0';
						sprintf(str, "%s\\%s", CurPath, &C2[1]);
						if (!OpenDlg_ScanDir(hWnd, nDrive, str, fspec)) {
							printf("FileSaveDlgProc // ChangeDir Error !\n");
							}
						else {
							strcpy(CurPath, str);
							}
						}
					}
				break;
			case cmb1:
				if (HIWORD(lParam) == CBN_SELCHANGE) {
					wRet = SendDlgItemMessage(hWnd, cmb1, CB_GETCURSEL, 0, 0L);
					fspec = OpenDlg_GetFileType(lpofn->lpstrFilter, wRet + 1);
					printf("FileSaveDlgProc // new fspec #%d = '%s' !\n", wRet, fspec);
					if (!OpenDlg_ScanFiles(hWnd, nDrive, CurPath, fspec)) {
						printf("OpenDlg_ScanFile // Change FileType Error !\n");
						}
					}
				break;
			case cmb2:
#ifdef DEBUG_OPENDLG
				printf("FileSaveDlgProc // combo #2 changed !\n");
#endif
				wRet = SendDlgItemMessage(hWnd, cmb1, CB_GETCURSEL, 0, 0L);
				if (wRet == (WORD)LB_ERR) return 0;
				fspec = OpenDlg_GetFileType(lpofn->lpstrFilter, wRet + 1);
				wRet = SendDlgItemMessage(hWnd, cmb2, CB_GETCURSEL, 0, 0L);
				if (wRet == (WORD)LB_ERR) return 0;
				printf("FileSaveDlgProc // combo #2 CB_GETCURSEL=%d !\n", wRet);
				SendDlgItemMessage(hWnd, cmb2, CB_GETLBTEXT, wRet, (DWORD)C2);
				nDrive = C2[2] - 'a';
#ifdef DEBUG_OPENDLG
				printf("FileSaveDlgProc // new drive selected=%d !\n", nDrive);
#endif
				sprintf(str, "%c:\\%s", nDrive + 'A', DOS_GetCurrentDir(nDrive));
#ifdef DEBUG_OPENDLG
				printf("FileSaveDlgProc // new drive , curPath='%s' !\n", str);
#endif
				if (!OpenDlg_ScanDir(hWnd, nDrive, str, fspec)) {
					printf("FileSaveDlgProc // Change Drive Error !\n");
					}
				else {
					strcpy(CurPath, str);
					}
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
					strcpy(lpofn->lpstrFile, "titi.txt");
					if (lpofn->lpstrFileTitle != NULL) {
						strcpy(lpofn->lpstrFileTitle, "titi.txt");
						}
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
	HANDLE	hDlgTmpl;
	HANDLE	hResInfo;
    WND 	*wndPtr;
	BOOL	bRet;
	hResInfo = FindResource(hSysRes, MAKEINTRESOURCE(FINDDLG), RT_DIALOG);
	if (hResInfo == 0) {
		CommDlgLastError = CDERR_FINDRESFAILURE;
		return FALSE;
		}
	hDlgTmpl = LoadResource(hSysRes, hResInfo);
	if (hDlgTmpl == 0) {
		CommDlgLastError = CDERR_LOADRESFAILURE;
		return FALSE;
		}
    wndPtr = WIN_FindWndPtr(lpFind->hwndOwner);
	bRet = DialogBoxIndirectParam(wndPtr->hInstance, hDlgTmpl, 
		lpFind->hwndOwner, (WNDPROC)FindTextDlgProc, (DWORD)lpFind);
	return bRet;
}


/***********************************************************************
 * 				ReplaceTextDlg			[COMMDLG.12]
 */
BOOL ReplaceText(LPFINDREPLACE lpFind)
{
	HANDLE	hDlgTmpl;
	HANDLE	hResInfo;
    WND 	*wndPtr;
	BOOL	bRet;
	hResInfo = FindResource(hSysRes, MAKEINTRESOURCE(REPLACEDLG), RT_DIALOG);
	if (hResInfo == 0) {
		CommDlgLastError = CDERR_FINDRESFAILURE;
		return FALSE;
		}
	hDlgTmpl = LoadResource(hSysRes, hResInfo);
	if (hDlgTmpl == 0) {
		CommDlgLastError = CDERR_LOADRESFAILURE;
		return FALSE;
		}
    wndPtr = WIN_FindWndPtr(lpFind->hwndOwner);
	bRet = DialogBoxIndirectParam(wndPtr->hInstance, hDlgTmpl, 
		lpFind->hwndOwner, (WNDPROC)ReplaceTextDlgProc, (DWORD)lpFind);
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
	HANDLE	hDlgTmpl;
	HANDLE	hResInfo;
    WND 	*wndPtr;
	BOOL	bRet;
	printf("PrintDlg(%p) // Flags=%08lX\n", lpPrint, lpPrint->Flags);
	if (lpPrint->Flags & PD_PRINTSETUP)
		hResInfo = FindResource(hSysRes, MAKEINTRESOURCE(PRINTSETUPDLG), RT_DIALOG);
	else
		hResInfo = FindResource(hSysRes, MAKEINTRESOURCE(PRINTDLG), RT_DIALOG);
	if (hResInfo == 0) {
		CommDlgLastError = CDERR_FINDRESFAILURE;
		return FALSE;
		}
	hDlgTmpl = LoadResource(hSysRes, hResInfo);
	if (hDlgTmpl == 0) {
		CommDlgLastError = CDERR_LOADRESFAILURE;
		return FALSE;
		}
    wndPtr = WIN_FindWndPtr(lpPrint->hwndOwner);
	if (lpPrint->Flags & PD_PRINTSETUP)
		bRet = DialogBoxIndirectParam(wndPtr->hInstance, hDlgTmpl, 
			lpPrint->hwndOwner, (WNDPROC)PrintSetupDlgProc, (DWORD)lpPrint);
	else
		bRet = DialogBoxIndirectParam(wndPtr->hInstance, hDlgTmpl, 
			lpPrint->hwndOwner, (WNDPROC)PrintDlgProc, (DWORD)lpPrint);
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
	int		i, len;
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
		if (lpFile[i] == '/' || 
			lpFile[i] == '\\' || 
			lpFile[i] == ':') {
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



