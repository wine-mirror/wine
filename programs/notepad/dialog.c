/*
 * Notepad
 *
 * Copyright 1998 Marcel Baur <mbaur@g26.ethz.ch>
 */

#include <stdio.h>
#include "windows.h"
#include "commdlg.h"
#ifdef WINELIB
#include "shell.h"
#include "options.h"
#endif
#include "main.h"
#include "license.h"
#include "dialog.h"
#include "version.h"

static LRESULT DIALOG_PAGESETUP_DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

VOID DIALOG_FileNew(VOID)
{
  fprintf(stderr, "FileNew()\n");
}

VOID DIALOG_FileOpen(VOID)
{
	  OPENFILENAME openfilename;
	  CHAR szPath[MAX_PATHNAME_LEN];
	  CHAR szDir[MAX_PATHNAME_LEN];
	  CHAR szzFilter[2 * MAX_STRING_LEN + 100];
	  LPSTR p = szzFilter;

	  LoadString(Globals.hInstance, IDS_TEXT_FILES_TXT, p, MAX_STRING_LEN);
	  p += strlen(p) + 1;
	  lstrcpy(p, "*.txt");
	  p += strlen(p) + 1;
	  LoadString(Globals.hInstance, IDS_ALL_FILES, p, MAX_STRING_LEN);
	  p += strlen(p) + 1;
	  lstrcpy(p, "*.*");
	  p += strlen(p) + 1;
	  *p = '\0';

	  GetWindowsDirectory(szDir, sizeof(szDir));

	  openfilename.lStructSize       = 0;
	  openfilename.hwndOwner         = Globals.hMainWnd;
	  openfilename.hInstance         = Globals.hInstance;
	  openfilename.lpstrFilter       = szzFilter;
	  openfilename.lpstrCustomFilter = 0;
	  openfilename.nMaxCustFilter    = 0;
	  openfilename.nFilterIndex      = 0;
	  openfilename.lpstrFile         = szPath;
	  openfilename.nMaxFile          = sizeof(szPath);
	  openfilename.lpstrFileTitle    = 0;
	  openfilename.nMaxFileTitle     = 0;
	  openfilename.lpstrInitialDir   = szDir;
	  openfilename.lpstrTitle        = 0;
	  openfilename.Flags             = 0;
	  openfilename.nFileOffset       = 0;
	  openfilename.nFileExtension    = 0;
	  openfilename.lpstrDefExt       = 0;
	  openfilename.lCustData         = 0;
	  openfilename.lpfnHook          = 0;
 	  openfilename.lpTemplateName    = 0;

  	  if (GetOpenFileName(&openfilename)) {
	  printf("Openfile: %s\n", openfilename.lpstrFile);
	  }
}

VOID DIALOG_FileSave(VOID)
{
  fprintf(stderr, "FileSave()\n");
}

VOID DIALOG_FileSaveAs(VOID)
{
          OPENFILENAME savefilename;
          CHAR szPath[MAX_PATHNAME_LEN];
          CHAR szDir[MAX_PATHNAME_LEN];
          CHAR szzFilter[2 * MAX_STRING_LEN + 100];
          LPSTR p = szzFilter;

          LoadString(Globals.hInstance, IDS_TEXT_FILES_TXT, p, MAX_STRING_LEN);
          p += strlen(p) + 1;
          lstrcpy(p, "*.txt");
          p += strlen(p) + 1;
          LoadString(Globals.hInstance, IDS_ALL_FILES, p, MAX_STRING_LEN);
          p += strlen(p) + 1;
          lstrcpy(p, "*.*");
          p += strlen(p) + 1;
          *p = '\0';

          GetWindowsDirectory(szDir, sizeof(szDir));

          savefilename.lStructSize       = 0;
          savefilename.hwndOwner         = Globals.hMainWnd;
          savefilename.hInstance         = Globals.hInstance;
          savefilename.lpstrFilter       = szzFilter;
          savefilename.lpstrCustomFilter = 0;
          savefilename.nMaxCustFilter    = 0;
          savefilename.nFilterIndex      = 0;
          savefilename.lpstrFile         = szPath;
          savefilename.nMaxFile          = sizeof(szPath);
          savefilename.lpstrFileTitle    = 0;
          savefilename.nMaxFileTitle     = 0;
          savefilename.lpstrInitialDir   = szDir;
          savefilename.lpstrTitle        = 0;
          savefilename.Flags             = 0;
          savefilename.nFileOffset       = 0;
          savefilename.nFileExtension    = 0;
          savefilename.lpstrDefExt       = 0;
          savefilename.lCustData         = 0;
          savefilename.lpfnHook          = 0;
          savefilename.lpTemplateName    = 0;

          if (GetSaveFileName(&savefilename));
}

VOID DIALOG_FilePrint(VOID)
{
	PRINTDLG printer;
	printer.lStructSize           = 0;
        printer.hwndOwner             = Globals.hMainWnd;
        printer.hInstance             = Globals.hInstance;
        printer.hDevMode              = 0;
	printer.hDevNames             = 0;
	printer.hDC                   = 0;
        printer.Flags                 = 0;
	printer.nFromPage             = 0;
        printer.nToPage               = 0;
	printer.nMinPage              = 0;
	printer.nMaxPage              = 0;
	printer.nCopies               = 0;
	printer.lCustData             = 0;
	printer.lpfnPrintHook       = 0;
	printer.lpfnSetupHook         = 0;
	printer.lpPrintTemplateName   = 0;
	printer.lpSetupTemplateName   = 0;
	printer.hPrintTemplate        = 0;
	printer.hSetupTemplate        = 0;
	
	if(PrintDlg16(&printer));
}

VOID DIALOG_FilePageSetup(VOID)
{
  DIALOG_PageSetup();
}

VOID DIALOG_FilePrinterSetup(VOID)
{
  fprintf(stderr, "FilePrinterSetup()\n");
}

VOID DIALOG_FileExit(VOID)
{
  PostQuitMessage(0);
}

VOID DIALOG_EditUndo(VOID)
{
  fprintf(stderr, "EditUndo()\n");
}

VOID DIALOG_EditCut(VOID)
{
  fprintf(stderr, "EditCut()\n");
}

VOID DIALOG_EditCopy(VOID)
{
  fprintf(stderr, "EditCopy()\n");
}

VOID DIALOG_EditPaste(VOID)
{
  fprintf(stderr, "EditPaste()\n");
}

VOID DIALOG_EditDelete(VOID)
{
  fprintf(stderr, "EditDelete()\n");
}

VOID DIALOG_EditSelectAll(VOID)
{
   fprintf(stderr, "EditSelectAll()\n");
}

VOID DIALOG_EditTimeDate(VOID)
{
  DIALOG_TimeDate();   
}

VOID DIALOG_EditWrap(VOID)
{
  Globals.bWrapLongLines = !Globals.bWrapLongLines;
	CheckMenuItem(Globals.hEditMenu, NP_EDIT_WRAP, MF_BYCOMMAND | 
        (Globals.bWrapLongLines ? MF_CHECKED : MF_UNCHECKED));
}

VOID DIALOG_Search(VOID)
{
          FINDREPLACE find;
	  CHAR szFind[MAX_PATHNAME_LEN];
	  find.lStructSize               = 0;
          find.hwndOwner                 = Globals.hMainWnd;
          find.hInstance                 = Globals.hInstance;
          find.lpstrFindWhat             = szFind;
          find.wFindWhatLen              = sizeof(szFind);
          find.Flags                     = 0;
          find.lCustData                 = 0;
          find.lpfnHook                  = 0;
          find.lpTemplateName            = 0;
	  FindText(&find);
}

VOID DIALOG_SearchNext(VOID)
{
  fprintf(stderr, "SearchNext()\n");
}

VOID DIALOG_HelpContents(VOID)
{
  printf("NP_HELP_CONTENTS\n");
  WinHelp(Globals.hMainWnd, HELPFILE, HELP_INDEX, 0);
}

VOID DIALOG_HelpSearch(VOID)
{
  fprintf(stderr, "HelpSearch()\n");
}

VOID DIALOG_HelpHelp(VOID)
{
  printf("NP_HELP_ON_HELP\n");
  WinHelp(Globals.hMainWnd, HELPFILE, HELP_HELPONHELP, 0);
}

VOID DIALOG_HelpLicense(VOID)
{
  WineLicense(Globals.hMainWnd, Globals.lpszLanguage);
}

VOID DIALOG_HelpNoWarranty(VOID)
{
        WineWarranty(Globals.hMainWnd, Globals.lpszLanguage);
}

VOID DIALOG_HelpAboutWine(VOID)
{
  ShellAbout(Globals.hMainWnd, "Notepad", "Notepad\n" WINE_RELEASE_INFO, 0);
}

/***********************************************************************
 *
 *           DIALOG_PageSetup
 */

VOID DIALOG_PageSetup(VOID)
{
  WNDPROC lpfnDlg = MakeProcInstance(DIALOG_PAGESETUP_DlgProc, Globals.hInstance);
  DialogBox(Globals.hInstance,  STRING_PAGESETUP_Xx,
	    Globals.hMainWnd, lpfnDlg);
  FreeProcInstance(lpfnDlg);
}

/***********************************************************************
 *
 *           DIALOG_TimeDate
 */

VOID DIALOG_TimeDate(VOID)
{
  SYSTEMTIME st;
   int H, Min, Y, M, D;
   GetLocalTime(&st);
   H = st.wHour; Min = st.wMinute; Y  = st.wYear; M = st.wMonth; D = st.wDay;
   printf("TimeDate=%i:", H);
   if (Min < 10) printf("0");
   printf("%i  %i/", Min, Y);
   if (M < 10) printf("0");
   printf("%i/", M);
   if (D < 10) printf("0");
   printf("%i\n", D);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *           DIALOG_PAGESETUP_DlgProc
 */

static LRESULT DIALOG_PAGESETUP_DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
    {
    case WM_COMMAND:
      switch (wParam)
	{
	case IDOK:
	  EndDialog(hDlg, IDOK);
	  return TRUE;

	case IDCANCEL:
	  EndDialog(hDlg, IDCANCEL);
	  return TRUE;
	}
    }
  return FALSE;
}

/* Local Variables:    */
/* c-file-style: "GNU" */
/* End:                */
