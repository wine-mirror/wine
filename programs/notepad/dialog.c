/*
 * Notepad
 *
 * Copyright 1998 Marcel Baur <mbaur@g26.ethz.ch>
 */

#include <stdio.h>
#include "windows.h"
#include "commdlg.h"
#include "winnls.h"
#include "winerror.h"
#ifdef WINELIB
#include "shell.h"
#include "options.h"
#endif
#include "main.h"
#include "license.h"
#include "language.h"
#include "dialog.h"
#include "version.h"
#include "debug.h"


static LRESULT DIALOG_PAGESETUP_DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);


int AlertIDS(UINT ids_message, UINT ids_caption, WORD type) {
/*
 * Given some ids strings, this acts as a language-aware wrapper for 
 * "MessageBox"
 */
   CHAR szMessage[MAX_STRING_LEN];
   CHAR szCaption[MAX_STRING_LEN];
   
   LoadString(Globals.hInstance, ids_message, szMessage, sizeof(szMessage));
   LoadString(Globals.hInstance, ids_caption, szCaption, sizeof(szCaption));
   
   return (MessageBox(Globals.hMainWnd, szMessage, szCaption, type));
}

void AlertFileNotFound(LPCSTR szFilename) {

   int nResult;
   
   nResult = AlertIDS(IDS_NOTFOUND, IDS_ERROR, 0);
}


VOID AlertOutOfMemory(void) {

   int nResult;
   
   nResult = AlertIDS(IDS_OUT_OF_MEMORY, IDS_ERROR, 0);
   PostQuitMessage(1);
}


BOOL ExistFile(LPCSTR szFilename) {
/*
 *  Returns: TRUE  - if "szFileName" exists
 *           FALSE - if it does not
 */
   WIN32_FIND_DATA32A entry;
   HANDLE32 handle;
   
   handle = FindFirstFile32A(szFilename, &entry);
   
   return (handle!=INVALID_HANDLE_VALUE32);
}

VOID DoSaveFile(VOID) {
   // Really Save the file 
   
   // ... (Globals.szFileName);
}


BOOL DoCloseFile(void) {
// Return value: TRUE  - User agreed to close (both save/don't save)
//               FALSE - User cancelled close by selecting "Cancel"

   CHAR szMessage[MAX_STRING_LEN];
   CHAR szCaption[MAX_STRING_LEN];

   INT nResult;
   
   if (strlen(Globals.szFileName)>0) {

   // prompt user to save changes
   
   // FIXME: The following resources are not yet in the .rc files
   // szMessage, szCaption show up random values. Please keep these lines!
   
//  LoadString(Globals.hInstance, ids_savechanges, szMessage, sizeof(szMessage));
//  LoadString(Globals.hInstance, ids_savetitle, szCaption, sizeof(szCaption));
   
   nResult = MessageBox(Globals.hMainWnd, szMessage, szCaption, MB_YESNOCANCEL);

   switch (nResult) {
          case IDYES:     DoSaveFile();
                     break;
          case IDNO:      break;
          case IDCANCEL:  return(FALSE);
                     break;
                      
         default:    return(FALSE);
                     break;
      }
      
    
  }
  
  // Forget file name
  
  lstrcpyn(Globals.szFileName, "\0", 1);
  LANGUAGE_UpdateWindowCaption();

  return(TRUE);
  
}


void DoOpenFile(LPCSTR szFileName) {

    // Close any files and prompt to save changes
    if (DoCloseFile) {

        // Open file
        lstrcpyn(Globals.szFileName, szFileName, strlen(szFileName)+1); 
        LANGUAGE_UpdateWindowCaption();
    
    }
}


VOID DIALOG_FileNew(VOID)
{
    // Close any files and promt to save changes
    if (DoCloseFile()) {
    
        // do nothing yet
    
    }
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

        openfilename.lStructSize       = sizeof(openfilename);
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
                
                if (ExistFile(openfilename.lpstrFile))
                    DoOpenFile(openfilename.lpstrFile);
                else 
                    AlertFileNotFound(openfilename.lpstrFile);
                    
        }
          
}

VOID DIALOG_FileSave(VOID)
{
        fprintf(stderr, "FileSave()\n");
}

VOID DIALOG_FileSaveAs(VOID)
{
        OPENFILENAME saveas;
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

        saveas.lStructSize       = 0;
        saveas.hwndOwner         = Globals.hMainWnd;
        saveas.hInstance         = Globals.hInstance;
        saveas.lpstrFilter       = szzFilter;
        saveas.lpstrCustomFilter = 0;
        saveas.nMaxCustFilter    = 0;
        saveas.nFilterIndex      = 0;
        saveas.lpstrFile         = szPath;
        saveas.nMaxFile          = sizeof(szPath);
        saveas.lpstrFileTitle    = 0;
        saveas.nMaxFileTitle     = 0;
        saveas.lpstrInitialDir   = szDir;
        saveas.lpstrTitle        = 0;
        saveas.Flags             = 0;
        saveas.nFileOffset       = 0;
        saveas.nFileExtension    = 0;
        saveas.lpstrDefExt       = 0;
        saveas.lCustData         = 0;
        saveas.lpfnHook          = 0;
        saveas.lpTemplateName    = 0;

        if (GetSaveFileName(&saveas)) {
            lstrcpyn(Globals.szFileName, saveas.lpstrFile, 
            strlen(saveas.lpstrFile)+1);
            LANGUAGE_UpdateWindowCaption();
            DIALOG_FileSave();
        }
}

VOID DIALOG_FilePrint(VOID)
{
        PRINTDLG printer;
        printer.lStructSize           = sizeof(printer);
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
        printer.lpfnPrintHook         = 0;
        printer.lpfnSetupHook         = 0;
        printer.lpPrintTemplateName   = 0;
        printer.lpSetupTemplateName   = 0;
        printer.hPrintTemplate        = 0;
        printer.hSetupTemplate        = 0;
        
        if (PrintDlg16(&printer)) {
            // do nothing
        };
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
        if (DoCloseFile()) {
               PostQuitMessage(0);
        }
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
          
        lstrcpyn(szFind, Globals.szFindText, strlen(Globals.szFindText)+1);
          
        find.lStructSize      = sizeof(find);
        find.hwndOwner        = Globals.hMainWnd;
        find.hInstance        = Globals.hInstance;
        find.lpstrFindWhat    = szFind;
        find.wFindWhatLen     = sizeof(szFind);
        find.Flags            = 0;
        find.lCustData        = 0;
        find.lpfnHook         = 0;
        find.lpTemplateName   = 0;

        if (FindText(&find)) {
             lstrcpyn(Globals.szFindText, szFind, strlen(szFind)+1);
        } 
             else 
        { 
             // do nothing yet
        };
}

VOID DIALOG_SearchNext(VOID)
{
        fprintf(stderr, "SearchNext()\n");
}

VOID DIALOG_HelpContents(VOID)
{
        WinHelp(Globals.hMainWnd, HELPFILE, HELP_INDEX, 0);
}

VOID DIALOG_HelpSearch(VOID)
{
        fprintf(stderr, "HelpSearch()\n");
}

VOID DIALOG_HelpHelp(VOID)
{
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
  DialogBox(Globals.hInstance, STRING_PAGESETUP_Xx, Globals.hMainWnd, lpfnDlg);
  FreeProcInstance(lpfnDlg);
}

/***********************************************************************
 *
 *           DIALOG_TimeDate
 */

VOID DIALOG_TimeDate(VOID)
{
  // uses [KERNEL32.310] (ole2nls.c)
  
  SYSTEMTIME   st;
  LPSYSTEMTIME lpst = &st;
  CHAR         szDate[MAX_STRING_LEN];
  LPSTR        date = szDate;
  
  GetLocalTime(&st);
  GetDateFormat(LOCALE_USER_DEFAULT, LOCALE_SLONGDATE, lpst, NULL, date, MAX_STRING_LEN);
  
  printf("Date: %s\n", date);
  
  GetTimeFormat(LOCALE_USER_DEFAULT, LOCALE_STIMEFORMAT, lpst, NULL, date, MAX_STRING_LEN);
  
  printf("Time: %s\n", date);

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
