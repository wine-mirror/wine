/*
 *  Notepad
 *
 *  Copyright 1997,98 Marcel Baur <mbaur@g26.ethz.ch>
 *  To be distributed under the Wine License
 */

#include <stdio.h>
#include "windows.h"

#ifdef LCC
#include "lcc.h"
#endif

#include "main.h"
#include "license.h"
#include "dialog.h"
#include "language.h"

#if !defined(LCC) || defined(WINELIB)
#include "shell.h"
#endif

#ifdef WINELIB
#include "options.h"
#include "resource.h"
void LIBWINE_Register_Da();
void LIBWINE_Register_De();
void LIBWINE_Register_En();
void LIBWINE_Register_Es();
void LIBWINE_Register_Fi();
void LIBWINE_Register_Fr();
void LIBWINE_Register_Sw();
#endif

NOTEPAD_GLOBALS Globals;

/***********************************************************************
 *
 *           NOTEPAD_MenuCommand
 *
 *  All handling of main menu events
 */

int NOTEPAD_MenuCommand (WPARAM wParam)
{  
   switch (wParam) {
     case NP_FILE_NEW:          DIALOG_FileNew(); break;
     case NP_FILE_OPEN:         DIALOG_FileOpen(); break;
     case NP_FILE_SAVE:         DIALOG_FileSave(); break;
     case NP_FILE_SAVEAS:       DIALOG_FileSaveAs(); break;
     case NP_FILE_PRINT:        DIALOG_FilePrint(); break;
     case NP_FILE_PAGESETUP:    DIALOG_FilePageSetup(); break;
     case NP_FILE_PRINTSETUP:   DIALOG_FilePrinterSetup();break;
     case NP_FILE_EXIT:         DIALOG_FileExit(); break;

     case NP_EDIT_UNDO:         DIALOG_EditUndo(); break;
     case NP_EDIT_CUT:          DIALOG_EditCut(); break;
     case NP_EDIT_COPY:         DIALOG_EditCopy(); break;
     case NP_EDIT_PASTE:        DIALOG_EditPaste(); break;
     case NP_EDIT_DELETE:       DIALOG_EditDelete(); break;
     case NP_EDIT_SELECTALL:    DIALOG_EditSelectAll(); break;
     case NP_EDIT_TIMEDATE:     DIALOG_EditTimeDate();break;
     case NP_EDIT_WRAP:         DIALOG_EditWrap(); break;

     case NP_SEARCH_SEARCH:     DIALOG_Search(); break;
     case NP_SEARCH_NEXT:       DIALOG_SearchNext(); break;

     case NP_HELP_CONTENTS:     DIALOG_HelpContents(); break;
     case NP_HELP_SEARCH:       DIALOG_HelpSearch(); break;
     case NP_HELP_ON_HELP:      DIALOG_HelpHelp(); break;
     case NP_HELP_LICENSE:      DIALOG_HelpLicense(); break;
     case NP_HELP_NO_WARRANTY:  DIALOG_HelpNoWarranty(); break;
     case NP_HELP_ABOUT_WINE:   DIALOG_HelpAboutWine(); break;
     
     /* Handle languages */
     default:
      LANGUAGE_DefaultHandle(wParam);
   }
   return 0;
}



/***********************************************************************
 *
 *           NOTEPAD_WndProc
 */

LRESULT NOTEPAD_WndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hContext;
    HANDLE hDrop;                      /* drag & drop */
    CHAR szFileName[MAX_STRING_LEN];
    RECT Windowsize;

    lstrcpy(szFileName, "");

    switch (msg) {

       case WM_CREATE:
        break;

       case WM_PAINT:
          hContext = BeginPaint(hWnd, &ps);
          TextOut(hContext, 1, 1, Globals.Buffer, strlen(Globals.Buffer));
          EndPaint(hWnd, &ps);
        break;

       case WM_COMMAND:
          NOTEPAD_MenuCommand(wParam);
          break;

       case WM_DESTROYCLIPBOARD:
          MessageBox(Globals.hMainWnd, "Empty clipboard", "Debug", MB_ICONEXCLAMATION);
          break;

       case WM_CLOSE:
          if (DoCloseFile()) {
             PostQuitMessage(0);
          }
          break;

       case WM_DESTROY:
             PostQuitMessage (0);
          break;

       case WM_SIZE:
          GetClientRect(Globals.hMainWnd, &Windowsize);
          break;

       case WM_DROPFILES:
          /* User has dropped a file into main window */
          hDrop = (HANDLE) wParam;
          DragQueryFile(hDrop, 0, (CHAR *) &szFileName, sizeof(szFileName));
          DragFinish(hDrop);
          DoOpenFile(szFileName);
          break;        

       default:
          return DefWindowProc (hWnd, msg, wParam, lParam);
    }
    return 0l;
}



/***********************************************************************
 *
 *           WinMain
 */

int PASCAL WinMain (HANDLE hInstance, HANDLE prev, LPSTR cmdline, int show)
{
    MSG      msg;
    WNDCLASS class;
    char className[] = "NPClass";  /* To make sure className >= 0x10000 */
    char winName[]   = "Notepad";

    #if defined(WINELIB) && !defined(HAVE_WINE_CONSTRUCTOR)
      /* Register resources */
      LIBWINE_Register_Da();
      LIBWINE_Register_De();
      LIBWINE_Register_En();
      LIBWINE_Register_Es();
      LIBWINE_Register_Fi();
      LIBWINE_Register_Fr();
      LIBWINE_Register_Sw();
    #endif

    /* Select Language */
    LANGUAGE_Init();


    /* Setup Globals */

    lstrcpy(Globals.lpszIniFile, "notepad.ini");
    lstrcpy(Globals.lpszIcoFile, "notepad.ico");

    Globals.hInstance       = hInstance;

#ifndef LCC
    Globals.hMainIcon       = ExtractIcon(Globals.hInstance, 
                                        Globals.lpszIcoFile, 0);
#endif
    if (!Globals.hMainIcon) {
        Globals.hMainIcon = LoadIcon(0, MAKEINTRESOURCE(DEFAULTICON));
    }

    lstrcpy(Globals.szFindText,     "");
    lstrcpy(Globals.szFileName,     "");
    lstrcpy(Globals.szMarginTop,    "25 mm");
    lstrcpy(Globals.szMarginBottom, "25 mm");
    lstrcpy(Globals.szMarginLeft,   "20 mm");
    lstrcpy(Globals.szMarginRight,  "20 mm");
    lstrcpy(Globals.szHeader,       "&n");
    lstrcpy(Globals.szFooter,       "Page &s");
    lstrcpy(Globals.Buffer, "Hello World");

    if (!prev){
        class.style         = CS_HREDRAW | CS_VREDRAW;
        class.lpfnWndProc   = NOTEPAD_WndProc;
        class.cbClsExtra    = 0;
        class.cbWndExtra    = 0;
        class.hInstance     = Globals.hInstance;
        class.hIcon         = LoadIcon (0, IDI_APPLICATION);
        class.hCursor       = LoadCursor (0, IDC_ARROW);
        class.hbrBackground = GetStockObject (WHITE_BRUSH);
        class.lpszMenuName  = 0;
        class.lpszClassName = className;
    }

    if (!RegisterClass (&class)) return FALSE;

    /* Setup windows */


    Globals.hMainWnd = CreateWindow (className, winName, 
       WS_OVERLAPPEDWINDOW + WS_HSCROLL + WS_VSCROLL,
       CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 0, 
       LoadMenu(Globals.hInstance, STRING_MENU_Xx),
       Globals.hInstance, 0);

    Globals.hFindReplaceDlg = 0;

    LANGUAGE_SelectByName(Globals.lpszLanguage);

    SetMenu(Globals.hMainWnd, Globals.hMainMenu);               
                        
    ShowWindow (Globals.hMainWnd, show);
    UpdateWindow (Globals.hMainWnd);

    /* Set up dialogs */

    /* Identify Messages originating from FindReplace */

    Globals.nCommdlgFindReplaceMsg = RegisterWindowMessage("commdlg_FindReplace");
    if (Globals.nCommdlgFindReplaceMsg==0) {
       MessageBox(Globals.hMainWnd, "Could not register commdlg_FindReplace window message", 
                  "Error", MB_ICONEXCLAMATION);
    }

    /* now handle command line */
    
    while (*cmdline && (*cmdline == ' ' || *cmdline == '-')) 
    
    {
        CHAR   option;
/*      LPCSTR topic_id; */

        if (*cmdline++ == ' ') continue;

        option = *cmdline;
        if (option) cmdline++;
        while (*cmdline && *cmdline == ' ') cmdline++;

        switch(option)
        {
            case 'p':
            case 'P': printf("Print file: ");
                      /* Not yet able to print a file */
                      break;
        }
    }

    /* Set up Drag&Drop */

    DragAcceptFiles(Globals.hMainWnd, TRUE);

    MessageBox(Globals.hMainWnd, "BEWARE!\nThis is ALPHA software that may destroy your file system.\nPlease take care.", 
    "A note from the developer...", MB_ICONEXCLAMATION);

    /* now enter mesage loop     */
    
    while (GetMessage (&msg, 0, 0, 0)) {
        if (IsDialogMessage(Globals.hFindReplaceDlg, &msg)!=0) {
          /* Message belongs to FindReplace dialog */
          /* We just let IsDialogMessage handle it */
        } 
          else
        { 
          /* Message belongs to the Notepad Main Window */
          TranslateMessage (&msg);
          DispatchMessage (&msg);
        }
    }
    return 0;
}
