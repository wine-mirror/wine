/*
 *  Notepad
 *
 *  Copyright 2000 Mike McCormack <Mike_McCormack@looksmart.com.au>
 *  Copyright 1997,98 Marcel Baur <mbaur@g26.ethz.ch>
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
 *
 */

#include <windows.h>
#include <richedit.h>
#include <stdio.h>

#include "main.h"
#include "dialog.h"
#include "notepad_res.h"

NOTEPAD_GLOBALS Globals;

/***********************************************************************
 *
 *           SetFileName
 *
 *  Sets Global File Name.
 */
VOID SetFileName(LPSTR szFileName)
{
    lstrcpy(Globals.szFileName, szFileName);
    Globals.szFileTitle[0] = 0;
    GetFileTitle(szFileName, Globals.szFileTitle, sizeof(Globals.szFileTitle));
}

/***********************************************************************
 *
 *           NOTEPAD_MenuCommand
 *
 *  All handling of main menu events
 */

int NOTEPAD_MenuCommand(WPARAM wParam)
{
    switch (wParam)
    {
    case CMD_NEW:               DIALOG_FileNew(); break;
    case CMD_OPEN:              DIALOG_FileOpen(); break;
    case CMD_SAVE:              DIALOG_FileSave(); break;
    case CMD_SAVE_AS:           DIALOG_FileSaveAs(); break;
    case CMD_PRINT:             DIALOG_FilePrint(); break;
    case CMD_PAGE_SETUP:        DIALOG_FilePageSetup(); break;
    case CMD_PRINTER_SETUP:     DIALOG_FilePrinterSetup();break;
    case CMD_EXIT:              DIALOG_FileExit(); break;

    case CMD_UNDO:             DIALOG_EditUndo(); break;
    case CMD_CUT:              DIALOG_EditCut(); break;
    case CMD_COPY:             DIALOG_EditCopy(); break;
    case CMD_PASTE:            DIALOG_EditPaste(); break;
    case CMD_DELETE:           DIALOG_EditDelete(); break;
    case CMD_SELECT_ALL:       DIALOG_EditSelectAll(); break;
    case CMD_TIME_DATE:        DIALOG_EditTimeDate();break;
    case CMD_WRAP:             DIALOG_EditWrap(); break;

    case CMD_SEARCH:           DIALOG_Search(); break;
    case CMD_SEARCH_NEXT:      DIALOG_SearchNext(); break;

    case CMD_HELP_CONTENTS:    DIALOG_HelpContents(); break;
    case CMD_HELP_SEARCH:      DIALOG_HelpSearch(); break;
    case CMD_HELP_ON_HELP:     DIALOG_HelpHelp(); break;
    case CMD_LICENSE:          DIALOG_HelpLicense(); break;
    case CMD_NO_WARRANTY:      DIALOG_HelpNoWarranty(); break;
    case CMD_ABOUT_WINE:       DIALOG_HelpAboutWine(); break;
    }
   return 0;
}

/***********************************************************************
 * Data Initialization
 */
VOID NOTEPAD_InitData(VOID)
{
    LPSTR p = Globals.szFilter;

    LoadString(Globals.hInstance, STRING_TEXT_FILES_TXT, p, MAX_STRING_LEN);
    p += strlen(p) + 1;
    lstrcpy(p, "*.txt");
    p += strlen(p) + 1;
    LoadString(Globals.hInstance, STRING_ALL_FILES, p, MAX_STRING_LEN);
    p += strlen(p) + 1;
    lstrcpy(p, "*.*");
    p += strlen(p) + 1;
    *p = '\0';
}

/***********************************************************************
 *
 *           NOTEPAD_WndProc
 */
LRESULT WINAPI NOTEPAD_WndProc(HWND hWnd, UINT msg, WPARAM wParam,
                               LPARAM lParam)
{
    switch (msg) {

    case WM_CREATE:
    {
        RECT rc;
        GetClientRect(hWnd, &rc);
        if (LoadLibrary("RichEd32.dll"))
        {
            Globals.hEdit =
                CreateWindow("RICHEDIT", "",
                             WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
                             ES_AUTOVSCROLL | ES_MULTILINE,
                             0, 0, rc.right, rc.bottom, hWnd,
                             NULL, Globals.hInstance, NULL);
        } else {
            ShowLastError();
            return -1;
        }
        break;
    }

    case WM_COMMAND:
        NOTEPAD_MenuCommand(wParam);
        break;

    case WM_DESTROYCLIPBOARD:
        MessageBox(Globals.hMainWnd, "Empty clipboard", "Debug", MB_ICONEXCLAMATION);
        break;

    case WM_CLOSE:
        if (DoCloseFile()) {
            DestroyWindow(hWnd);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_SIZE:
        SetWindowPos(Globals.hEdit, NULL, 0, 0, LOWORD(lParam), HIWORD(lParam),
                     SWP_NOOWNERZORDER | SWP_NOZORDER);
        break;

    case WM_DROPFILES:
    {
        CHAR szFileName[MAX_PATH];
        HANDLE hDrop = (HANDLE) wParam;

        DragQueryFile(hDrop, 0, (CHAR *) &szFileName, sizeof(szFileName));
        DragFinish(hDrop);
        DoOpenFile(szFileName);
        break;
    }

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

int AlertFileDoesNotExist(LPSTR szFileName) {

   int nResult;
   CHAR szMessage[MAX_STRING_LEN];
   CHAR szRessource[MAX_STRING_LEN];

   LoadString(Globals.hInstance, STRING_DOESNOTEXIST, szRessource,
              sizeof(szRessource));
   wsprintf(szMessage, szRessource, szFileName);

   LoadString(Globals.hInstance, STRING_ERROR,  szRessource, sizeof(szRessource));

   nResult = MessageBox(Globals.hMainWnd, szMessage, szRessource,
                        MB_ICONEXCLAMATION | MB_YESNO);

   return(nResult);
}

void HandleCommandLine(LPSTR cmdline)
{

    while (*cmdline && (*cmdline == ' ' || *cmdline == '-'))
    {
        CHAR   option;

        if (*cmdline++ == ' ') continue;

        option = *cmdline;
        if (option) cmdline++;
        while (*cmdline && *cmdline == ' ') cmdline++;

        switch(option)
        {
            case 'p':
            case 'P': printf("Print file: ");
                      /* TODO - not yet able to print a file */
                      break;
        }
    }

    if (*cmdline)
    {
        /* file name is passed in the command line */
        char *file_name;
        BOOL file_exists;
        char buf[MAX_PATH];

        if (cmdline[0] == '"')
        {
            cmdline++;
            cmdline[strlen(cmdline) - 1] = 0;
        }
        if (FileExists(cmdline))
        {
            file_exists = TRUE;
            file_name = cmdline;
        }
        else
        {
            /* try to find file with ".txt" extention */
            if (!strcmp(".txt", cmdline + strlen(cmdline) - strlen(".txt")))
            {
                file_exists = FALSE;
                file_name = cmdline;
            }
            else
            {
                strncpy(buf, cmdline, MAX_PATH - strlen(".txt") - 1);
                strcat(buf, ".txt");
                file_name = buf;
                file_exists = FileExists(buf);
            }
        }

        if (file_exists)
        {
            DoOpenFile(file_name);
            InvalidateRect(Globals.hMainWnd, NULL, FALSE);
        }
        else
        {
            switch (AlertFileDoesNotExist(file_name)) {
            case IDYES:
                DoOpenFile(file_name);
                break;

            case IDNO:
                break;
            }
        }
     }
}

/***********************************************************************
 *
 *           WinMain
 */
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE prev, LPSTR cmdline, int show)
{
    MSG        msg;
    WNDCLASSEX class;
    char className[] = "NPClass";
    char winName[]   = "Notepad";

    ZeroMemory(&Globals, sizeof(Globals));
    Globals.hInstance       = hInstance;

    ZeroMemory(&class, sizeof(class));
    class.cbSize        = sizeof(class);
    class.lpfnWndProc   = NOTEPAD_WndProc;
    class.hInstance     = Globals.hInstance;
    class.hIcon         = LoadIcon(0, IDI_APPLICATION);
    class.hCursor       = LoadCursor(0, IDC_ARROW);
    class.hbrBackground = (HBRUSH)(COLOR_WINDOW);
    class.lpszMenuName  = MAKEINTRESOURCE(MAIN_MENU);
    class.lpszClassName = className;

    if (!RegisterClassEx(&class)) return FALSE;

    /* Setup windows */

    Globals.hMainWnd =
        CreateWindow(className, winName, WS_OVERLAPPEDWINDOW,
                     CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
                     NULL, NULL, Globals.hInstance, NULL);
    if (!Globals.hMainWnd)
    {
        ShowLastError();
        ExitProcess(1);
    }

    NOTEPAD_InitData();
    DIALOG_FileNew();

    ShowWindow(Globals.hMainWnd, show);
    UpdateWindow(Globals.hMainWnd);
    DragAcceptFiles(Globals.hMainWnd, TRUE);

    HandleCommandLine(cmdline);

    while (GetMessage(&msg, 0, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}
