/*
 *  Notepad (dialog.c)
 *
 *  Copyright 1998,99 Marcel Baur <mbaur@g26.ethz.ch>
 *  Copyright 2002 Sylvain Petreolle <spetreolle@yahoo.fr>
 *  Copyright 2002 Andriy Palamarchuk
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <assert.h>
#include <stdio.h>
#include <windows.h>
#include <commdlg.h>
#include <winerror.h>

#include "main.h"
#include "license.h"
#include "dialog.h"

static LRESULT WINAPI DIALOG_PAGESETUP_DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);



VOID ShowLastError()
{
    DWORD error = GetLastError();
    if (error != NO_ERROR)
    {
        LPVOID lpMsgBuf;
        CHAR szTitle[MAX_STRING_LEN];

        LoadString(Globals.hInstance, STRING_ERROR, szTitle, sizeof(szTitle));
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, error, 0,
            (LPTSTR) &lpMsgBuf, 0, NULL);
        MessageBox(NULL, (char*)lpMsgBuf, szTitle, MB_OK | MB_ICONERROR);
        LocalFree(lpMsgBuf);
    }
}

/**
 * Sets the caption of the main window according to Globals.szFileTitle:
 *    Notepad - (untitled)      if no file is open
 *    Notepad - [filename]      if a file is given
 */
void UpdateWindowCaption(void) {
  CHAR szCaption[MAX_STRING_LEN];
  CHAR szUntitled[MAX_STRING_LEN];

  LoadString(Globals.hInstance, STRING_NOTEPAD, szCaption, sizeof(szCaption));

  if (Globals.szFileTitle[0] != '\0') {
      lstrcat(szCaption, " - [");
      lstrcat(szCaption, Globals.szFileTitle);
      lstrcat(szCaption, "]");
  }
  else
  {
      LoadString(Globals.hInstance, STRING_UNTITLED, szUntitled, sizeof(szUntitled));
      lstrcat(szCaption, " - ");
      lstrcat(szCaption, szUntitled);
  }

  SetWindowText(Globals.hMainWnd, szCaption);
}


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

void AlertFileNotFound(LPSTR szFileName) {

   int nResult;
   CHAR szMessage[MAX_STRING_LEN];
   CHAR szRessource[MAX_STRING_LEN];

   /* Load and format szMessage */
   LoadString(Globals.hInstance, STRING_NOTFOUND, szRessource, sizeof(szRessource));
   wsprintf(szMessage, szRessource, szFileName);

   /* Load szCaption */
   LoadString(Globals.hInstance, STRING_ERROR,  szRessource, sizeof(szRessource));

   /* Display Modal Dialog */
   nResult = MessageBox(Globals.hMainWnd, szMessage, szRessource, MB_ICONEXCLAMATION);

}

int AlertFileNotSaved(LPSTR szFileName) {

   int nResult;
   CHAR szMessage[MAX_STRING_LEN];
   CHAR szRessource[MAX_STRING_LEN];

   /* Load and format Message */

   LoadString(Globals.hInstance, STRING_NOTSAVED, szRessource, sizeof(szRessource));
   wsprintf(szMessage, szRessource, szFileName);

   /* Load Caption */

   LoadString(Globals.hInstance, STRING_ERROR,  szRessource, sizeof(szRessource));

   /* Display modal */
   nResult = MessageBox(Globals.hMainWnd, szMessage, szRessource, MB_ICONEXCLAMATION|MB_YESNOCANCEL);
   return(nResult);
}


VOID AlertOutOfMemory(void) {
   int nResult;

   nResult = AlertIDS(STRING_OUT_OF_MEMORY, STRING_ERROR, MB_ICONEXCLAMATION);
   PostQuitMessage(1);
}


/**
 * Returns:
 *   TRUE  - if file exists
 *   FALSE - if file does not exist
 */
BOOL FileExists(LPSTR szFilename) {
   WIN32_FIND_DATA entry;
   HANDLE hFile;

   hFile = FindFirstFile(szFilename, &entry);
   FindClose(hFile);

   return (hFile != INVALID_HANDLE_VALUE);
}


VOID DoSaveFile(VOID) {
    HANDLE hFile;
    DWORD dwNumWrite;
    BOOL bTest;
    CHAR *pTemp;
    int size;

    hFile = CreateFile(Globals.szFileName, GENERIC_WRITE, FILE_SHARE_WRITE,
                       NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if(hFile == INVALID_HANDLE_VALUE)
    {
        ShowLastError();
        return;
    }

    size = GetWindowTextLength(Globals.hEdit);
    pTemp = (LPSTR) GlobalAlloc(GMEM_FIXED, size);
    if (!pTemp)
    {
        ShowLastError();
        return;
    }
    GetWindowText(Globals.hEdit, pTemp, size);

    bTest = WriteFile(hFile, pTemp, size, &dwNumWrite, NULL);
    if(bTest == FALSE)
    {
        ShowLastError();
    }
    CloseHandle(hFile);
    GlobalFree(pTemp);
}

/**
 * Returns:
 *   TRUE  - User agreed to close (both save/don't save)
 *   FALSE - User cancelled close by selecting "Cancel"
 */
BOOL DoCloseFile(void) {
    int nResult;

    if (Globals.szFileName[0] != 0) {
        /* prompt user to save changes */
        nResult = AlertFileNotSaved(Globals.szFileName);
        switch (nResult) {
            case IDYES:     DIALOG_FileSave();
                            break;

            case IDNO:      break;

            case IDCANCEL:  return(FALSE);
                            break;

            default:        return(FALSE);
                            break;
        } /* switch */
    } /* if */

    SetFileName("");

    UpdateWindowCaption();
    return(TRUE);
}


void DoOpenFile(LPSTR szFileName) {
    /* Close any files and prompt to save changes */
    if (DoCloseFile())
    {
        HANDLE hFile;
        CHAR *pTemp;
        DWORD size;
        DWORD dwNumRead;

        hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if(hFile == INVALID_HANDLE_VALUE)
        {
            ShowLastError();
            return;
        }

        size = GetFileSize(hFile, NULL);
        if (size == 0xFFFFFFFF)
        {
            ShowLastError();
            return;
        }
        size++;
        pTemp = (LPSTR) GlobalAlloc(GMEM_FIXED, size);
        if (!pTemp)
        {
            ShowLastError();
            return;
        }
        if (!ReadFile(hFile, pTemp, size, &dwNumRead, NULL))
        {
            ShowLastError();
            return;
        }
        CloseHandle(hFile);
        pTemp[dwNumRead] = '\0';
        if (!SetWindowText(Globals.hEdit, pTemp))
        {
            GlobalFree(pTemp);
            ShowLastError();
            return;
        }
        SendMessage(Globals.hEdit, EM_EMPTYUNDOBUFFER, 0, 0);
        GlobalFree(pTemp);
        SetFocus(Globals.hEdit);

        SetFileName(szFileName);
        UpdateWindowCaption();
    }
}

VOID DIALOG_FileNew(VOID)
{
    /* Close any files and promt to save changes */
    if (DoCloseFile()) {
        SetWindowText(Globals.hEdit, "");
        SendMessage(Globals.hEdit, EM_EMPTYUNDOBUFFER, 0, 0);
        SetFocus(Globals.hEdit);
    }
}

VOID DIALOG_FileOpen(VOID)
{
    OPENFILENAME openfilename;

    CHAR szPath[MAX_PATH];
    CHAR szDir[MAX_PATH];
    CHAR szDefaultExt[] = "txt";

    ZeroMemory(&openfilename, sizeof(openfilename));

    GetCurrentDirectory(sizeof(szDir), szDir);
    lstrcpy(szPath,"*.txt");

    openfilename.lStructSize       = sizeof(openfilename);
    openfilename.hwndOwner         = Globals.hMainWnd;
    openfilename.hInstance         = Globals.hInstance;
    openfilename.lpstrFilter       = Globals.szFilter;
    openfilename.lpstrFile         = szPath;
    openfilename.nMaxFile          = sizeof(szPath);
    openfilename.lpstrInitialDir   = szDir;
    openfilename.Flags             = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST |
        OFN_HIDEREADONLY;
    openfilename.lpstrDefExt       = szDefaultExt;


    if (GetOpenFileName(&openfilename)) {
        if (FileExists(openfilename.lpstrFile))
            DoOpenFile(openfilename.lpstrFile);
        else
            AlertFileNotFound(openfilename.lpstrFile);
    }
}


VOID DIALOG_FileSave(VOID)
{
    if (Globals.szFileName[0] == '\0')
        DIALOG_FileSaveAs();
    else
        DoSaveFile();
}

VOID DIALOG_FileSaveAs(VOID)
{
    OPENFILENAME saveas;
    CHAR szPath[MAX_PATH];
    CHAR szDir[MAX_PATH];
    CHAR szDefaultExt[] = "txt";

    ZeroMemory(&saveas, sizeof(saveas));

    GetCurrentDirectory(sizeof(szDir), szDir);
    lstrcpy(szPath,"*.*");

    saveas.lStructSize       = sizeof(OPENFILENAME);
    saveas.hwndOwner         = Globals.hMainWnd;
    saveas.hInstance         = Globals.hInstance;
    saveas.lpstrFilter       = Globals.szFilter;
    saveas.lpstrFile         = szPath;
    saveas.nMaxFile          = sizeof(szPath);
    saveas.lpstrInitialDir   = szDir;
    saveas.Flags             = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT |
        OFN_HIDEREADONLY;
    saveas.lpstrDefExt       = szDefaultExt;

    if (GetSaveFileName(&saveas)) {
        SetFileName(szPath);
        UpdateWindowCaption();
        DoSaveFile();
    }
}

VOID DIALOG_FilePrint(VOID)
{
    LONG bFlags;
    DOCINFO di;
    int nResult;
    HDC hContext;
    PRINTDLG printer;
    char *pDevNamesSpace;
    LPDEVNAMES lpDevNames;
    SIZE szMetric;
    int cWidthPels, cHeightPels, border;
    int xLeft, yTop, count, i, pagecount, dopage, copycount;
    LOGFONT hdrFont;
    HFONT font, old_font=0;
    CHAR *pTemp;
    int size;

    CHAR szDocumentName[MAX_STRING_LEN]; /* Name of document */
    CHAR szPrinterName[MAX_STRING_LEN];  /* Name of the printer */
    CHAR szDeviceName[MAX_STRING_LEN];   /* Name of the printer device */
    CHAR szOutput[MAX_STRING_LEN];       /* in which file/device to print */
    
    strcpy(szDocumentName, Globals.szFileTitle);
    count = strlen(szDocumentName);

    /* Get a small font and print some header info on each page */
    hdrFont.lfHeight = 100;
    hdrFont.lfWidth = 0;
    hdrFont.lfEscapement = 0;
    hdrFont.lfOrientation = 0;
    hdrFont.lfWeight = FW_BOLD;
    hdrFont.lfItalic = 0;
    hdrFont.lfUnderline = 0;
    hdrFont.lfStrikeOut = 0;
    hdrFont.lfCharSet = ANSI_CHARSET;
    hdrFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
    hdrFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    hdrFont.lfQuality = PROOF_QUALITY;
    hdrFont.lfPitchAndFamily = VARIABLE_PITCH | FF_ROMAN;
    strcpy(hdrFont.lfFaceName, "Times New Roman");
    
    font = CreateFontIndirect(&hdrFont);
    
    /* Get Current Settings */
    ZeroMemory(&printer, sizeof(printer));
    printer.lStructSize           = sizeof(printer);
    printer.hwndOwner             = Globals.hMainWnd;
    printer.hInstance             = Globals.hInstance;
    
    /* Set some default flags */
    bFlags = PD_RETURNDC + PD_SHOWHELP;
    if (TRUE) {
            /* Remove "Print Selection" if there is no selection */
            bFlags = bFlags + PD_NOSELECTION;
    }
    printer.Flags                 = bFlags;
    printer.nFromPage             = 1;
    printer.nMinPage              = 1;
    /* we really need to calculate number of pages to set nMaxPage and nToPage */
    printer.nToPage               = 20;
    printer.nMaxPage              = 20;

    /* Let commdlg manage copy settings */
    printer.nCopies               = (WORD)PD_USEDEVMODECOPIES;

    nResult = PrintDlg(&printer);
    if (printer.hDevNames==0)
        return;
    if (!nResult) {
        MessageBox(Globals.hMainWnd, "PrintDlg failed", "Print Error", MB_ICONEXCLAMATION);
        return;
    }
    hContext = printer.hDC;

    pDevNamesSpace = GlobalLock(printer.hDevNames);
    lpDevNames = (LPDEVNAMES) pDevNamesSpace;
    lstrcpy(szPrinterName, pDevNamesSpace+lpDevNames->wDriverOffset);
    lstrcpy(szDeviceName, pDevNamesSpace+lpDevNames->wDeviceOffset);
    lstrcpy(szOutput, pDevNamesSpace+lpDevNames->wOutputOffset);
    GlobalUnlock(printer.hDevNames);
/*
    MessageBox(Globals.hMainWnd, szPrinterName, "Printer Name", MB_ICONEXCLAMATION);
    MessageBox(Globals.hMainWnd, szDeviceName,  "Device Name",  MB_ICONEXCLAMATION);
    MessageBox(Globals.hMainWnd, szOutput,      "Output",       MB_ICONEXCLAMATION);
*/
    /* initialize DOCINFO */
    di.cbSize = sizeof(DOCINFO);
    di.lpszDocName = szDocumentName;
    di.lpszOutput = szOutput;
    di.lpszDatatype = (LPTSTR) NULL;
    di.fwType = 0; 
    
    /* The default resolution is pixels, ie MM_TEXT */
/*    SetMapMode(hContext, MM_TWIPS);*/
/*    SetViewPortExExt(hContext, 10, 10, 0);*/
/*    SetBkMode(hContext, OPAQUE);*/

    /* Get the page dimensions in pixels. */
    cWidthPels = GetDeviceCaps(hContext, HORZRES);
    cHeightPels = GetDeviceCaps(hContext, VERTRES);

    /* Get the file text */
    size = GetWindowTextLength(Globals.hEdit);
    pTemp = (LPSTR) GlobalAlloc(GMEM_FIXED, size);
    if (!pTemp)
    {
        ShowLastError();
        return;
    }
    GetWindowText(Globals.hEdit, pTemp, size);
    if (!size)
    {
        ShowLastError();
        return;
    }
    
    /* Okay, let's print */
    nResult = StartDoc(hContext, &di);
    if (nResult <= 0) {
        MessageBox(Globals.hMainWnd, "StartDoc failed", "Print Error", MB_ICONEXCLAMATION);
        return;
    }

    border = 150;
    for (copycount=1; copycount <= printer.nCopies; copycount++) {
        i = 0;
        pagecount = 1;
        do {
            if (pagecount >= printer.nFromPage &&
    /*          ((printer.Flags & PD_PAGENUMS) == 0 ||  pagecount <= printer.nToPage))*/
            pagecount <= printer.nToPage)
                dopage = 1;
            else
                dopage = 0;
            
            old_font = SelectObject(hContext, font);
            GetTextExtentPoint32(hContext, "M", 1, &szMetric); 
                
            if (dopage) {
                nResult = StartPage(hContext);
                if (nResult <= 0) {
                    MessageBox(Globals.hMainWnd, "StartPage failed", "Print Error", MB_ICONEXCLAMATION);
                    return;
                }
                /* Write a rectangle and header at the top of each page */
                Rectangle(hContext, border, border, cWidthPels-border, border+szMetric.cy*2);
                /* I don't know what's up with this TextOut command. This comes out
                kind of mangled.
                */
                TextOut(hContext, border*2, border+szMetric.cy*0.5, szDocumentName, count);
            }
            
            /* The starting point for the main text */
            xLeft = border*2;
            yTop = border+szMetric.cy*4;
            
            SelectObject(hContext, old_font);
            GetTextExtentPoint32(hContext, "M", 1, &szMetric); 
            
            /* Since outputting strings is giving me problems, output the main
            text one character at a time.
            */
            do {
                if (pTemp[i] == '\n') {
                    xLeft = border*2;
                    yTop += szMetric.cy;
                }
                else if (pTemp[i] != '\r') {
                    if (dopage)
                        TextOut(hContext, xLeft, yTop, &pTemp[i], 1);
                    xLeft += szMetric.cx;
                }
            } while (i++<size && yTop<(cHeightPels-border*2));
            
            if (dopage)
                EndPage(hContext);
            pagecount++;
        } while (i<size);
    }

    switch (nResult) {
        case SP_ERROR:
                MessageBox(Globals.hMainWnd, "Generic Error", "Print Engine Error", MB_ICONEXCLAMATION);
                break;
        case SP_APPABORT:
                MessageBox(Globals.hMainWnd, "The print job was aborted.", "Print Engine Error", MB_ICONEXCLAMATION);
                break;
        case SP_USERABORT:
                MessageBox(Globals.hMainWnd, "The print job was aborted using the Print Manager ", "Print Engine Error", MB_ICONEXCLAMATION);
                break;
        case SP_OUTOFDISK:
                MessageBox(Globals.hMainWnd, "Out of disk space", "Print Engine Error", MB_ICONEXCLAMATION);
                break;
        case SP_OUTOFMEMORY:
                AlertOutOfMemory();
                break;
        default:
                break;
    } /* switch */
    nResult = EndDoc(hContext);
    assert(nResult>=0);
    nResult = DeleteDC(hContext);
    assert(nResult!=0);
}

VOID DIALOG_FilePageSetup(VOID)
{
    DIALOG_PageSetup();
}

VOID DIALOG_FilePrinterSetup(VOID)
{
    PRINTDLG printer;

    ZeroMemory(&printer, sizeof(printer));
    printer.lStructSize         = sizeof(printer);
    printer.hwndOwner           = Globals.hMainWnd;
    printer.hInstance           = Globals.hInstance;
    printer.Flags               = PD_PRINTSETUP;
    printer.nCopies             = 1;

    if (PrintDlg(&printer)) {
        /* do nothing */
    };
}

VOID DIALOG_FileExit(VOID)
{
    PostMessage(Globals.hMainWnd, WM_CLOSE, 0, 0l);
}

VOID DIALOG_EditUndo(VOID)
{
    SendMessage(Globals.hEdit, EM_UNDO, 0, 0);
}

VOID DIALOG_EditCut(VOID)
{
    HANDLE hMem;

    hMem = GlobalAlloc(GMEM_ZEROINIT, 99);

    OpenClipboard(Globals.hMainWnd);
    EmptyClipboard();

    /* FIXME: Get text */
    lstrcpy((CHAR *)hMem, "Hello World");

    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();

    GlobalFree(hMem);
}

VOID DIALOG_EditCopy(VOID)
{
    HANDLE hMem;

    hMem = GlobalAlloc(GMEM_ZEROINIT, 99);

    OpenClipboard(Globals.hMainWnd);
    EmptyClipboard();

    /* FIXME: Get text */
    lstrcpy((CHAR *)hMem, "Hello World");

    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();

    GlobalFree(hMem);
}

VOID DIALOG_EditPaste(VOID)
{
    HANDLE hClipText;

    if (IsClipboardFormatAvailable(CF_TEXT)) {
        OpenClipboard(Globals.hMainWnd);
        hClipText = GetClipboardData(CF_TEXT);
        CloseClipboard();
        MessageBox(Globals.hMainWnd, (CHAR *)hClipText, "PASTE", MB_ICONEXCLAMATION);
    }
}

VOID DIALOG_EditDelete(VOID)
{
        /* Delete */
}

VOID DIALOG_EditSelectAll(VOID)
{
        /* Select all */
}


VOID DIALOG_EditTimeDate(VOID)
{
    SYSTEMTIME   st;
    LPSYSTEMTIME lpst = &st;
    CHAR         szDate[MAX_STRING_LEN];
    LPSTR        date = szDate;

    GetLocalTime(&st);
    GetDateFormat(LOCALE_USER_DEFAULT, LOCALE_SLONGDATE, lpst, NULL, date, MAX_STRING_LEN);
    GetTimeFormat(LOCALE_USER_DEFAULT, LOCALE_STIMEFORMAT, lpst, NULL, date, MAX_STRING_LEN);
}

VOID DIALOG_EditWrap(VOID)
{
    Globals.bWrapLongLines = !Globals.bWrapLongLines;
    CheckMenuItem(GetMenu(Globals.hMainWnd), CMD_WRAP,
        MF_BYCOMMAND | (Globals.bWrapLongLines ? MF_CHECKED : MF_UNCHECKED));
}

VOID DIALOG_Search(VOID)
{
        ZeroMemory(&Globals.find, sizeof(Globals.find));
        Globals.find.lStructSize      = sizeof(Globals.find);
        Globals.find.hwndOwner        = Globals.hMainWnd;
        Globals.find.hInstance        = Globals.hInstance;
        Globals.find.lpstrFindWhat    = (CHAR *) &Globals.szFindText;
        Globals.find.wFindWhatLen     = sizeof(Globals.szFindText);
        Globals.find.Flags            = FR_DOWN;

        /* We only need to create the modal FindReplace dialog which will */
        /* notify us of incoming events using hMainWnd Window Messages    */

        Globals.hFindReplaceDlg = FindText(&Globals.find);
        assert(Globals.hFindReplaceDlg !=0);
}

VOID DIALOG_SearchNext(VOID)
{
        /* Search Next */
}

VOID DIALOG_HelpContents(VOID)
{
        WinHelp(Globals.hMainWnd, HELPFILE, HELP_INDEX, 0);
}

VOID DIALOG_HelpSearch(VOID)
{
        /* Search Help */
}

VOID DIALOG_HelpHelp(VOID)
{
        WinHelp(Globals.hMainWnd, HELPFILE, HELP_HELPONHELP, 0);
}

VOID DIALOG_HelpLicense(VOID)
{
        WineLicense(Globals.hMainWnd);
}

VOID DIALOG_HelpNoWarranty(VOID)
{
        WineWarranty(Globals.hMainWnd);
}

VOID DIALOG_HelpAboutWine(VOID)
{
        CHAR szNotepad[MAX_STRING_LEN];

        LoadString(Globals.hInstance, STRING_NOTEPAD, szNotepad, sizeof(szNotepad));
        ShellAbout(Globals.hMainWnd, szNotepad, "Notepad\n" WINE_RELEASE_INFO, 0);
}


/***********************************************************************
 *
 *           DIALOG_PageSetup
 */

VOID DIALOG_PageSetup(VOID)
{
  WNDPROC lpfnDlg;

  lpfnDlg = MakeProcInstance(DIALOG_PAGESETUP_DlgProc, Globals.hInstance);
  DialogBox(Globals.hInstance, MAKEINTRESOURCE(DIALOG_PAGESETUP),
            Globals.hMainWnd, (DLGPROC)lpfnDlg);
  FreeProcInstance(lpfnDlg);
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *           DIALOG_PAGESETUP_DlgProc
 */

static LRESULT WINAPI DIALOG_PAGESETUP_DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{

   switch (msg)
    {
    case WM_COMMAND:
      switch (wParam)
        {
        case IDOK:
          /* save user input and close dialog */
          GetDlgItemText(hDlg, 0x141,   Globals.szHeader,       sizeof(Globals.szHeader));
          GetDlgItemText(hDlg, 0x143,   Globals.szFooter,       sizeof(Globals.szFooter));
          GetDlgItemText(hDlg, 0x14A,    Globals.szMarginTop,    sizeof(Globals.szMarginTop));
          GetDlgItemText(hDlg, 0x150, Globals.szMarginBottom, sizeof(Globals.szMarginBottom));
          GetDlgItemText(hDlg, 0x147,   Globals.szMarginLeft,   sizeof(Globals.szMarginLeft));
          GetDlgItemText(hDlg, 0x14D,  Globals.szMarginRight,  sizeof(Globals.szMarginRight));
          EndDialog(hDlg, IDOK);
          return TRUE;

        case IDCANCEL:
          /* discard user input and close dialog */
          EndDialog(hDlg, IDCANCEL);
          return TRUE;

        case IDHELP:
          /* FIXME: Bring this to work */
          MessageBox(Globals.hMainWnd, "Sorry, no help available", "Help", MB_ICONEXCLAMATION);
          return TRUE;
        }
      break;

    case WM_INITDIALOG:
       /* fetch last user input prior to display dialog */
       SetDlgItemText(hDlg, 0x141,   Globals.szHeader);
       SetDlgItemText(hDlg, 0x143,   Globals.szFooter);
       SetDlgItemText(hDlg, 0x14A,    Globals.szMarginTop);
       SetDlgItemText(hDlg, 0x150, Globals.szMarginBottom);
       SetDlgItemText(hDlg, 0x147,   Globals.szMarginLeft);
       SetDlgItemText(hDlg, 0x14D,  Globals.szMarginRight);
       break;
    }

  return FALSE;
}
