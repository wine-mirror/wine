/*
 * Regedit main function
 *
 * Copyright (C) 2002 Robert Dickenson <robd@reactos.org>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define WIN32_LEAN_AND_MEAN     /* Exclude rarely-used stuff from Windows headers */
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <tchar.h>
#include <stdio.h>
#include <fcntl.h>

#define REGEDIT_DECLARE_FUNCTIONS
#include "main.h"

TCHAR g_pszDefaultValueName[64];

BOOL ProcessCmdLine(LPSTR lpCmdLine);


/*******************************************************************************
 * Global Variables:
 */

HINSTANCE hInst;
HWND hFrameWnd;
HWND hStatusBar;
HMENU hMenuFrame;
HMENU hPopupMenus = 0;
UINT nClipboardFormat;
LPCTSTR strClipboardFormat = _T("TODO: SET CORRECT FORMAT");


#define MAX_LOADSTRING  100
TCHAR szTitle[MAX_LOADSTRING];
const TCHAR szFrameClass[] = {'R','E','G','E','D','I','T','_','F','R','A','M','E',0};
const TCHAR szChildClass[] = {'R','E','G','E','D','I','T',0};


/*******************************************************************************
 *
 *
 *   FUNCTION: InitInstance(HANDLE, int)
 *
 *   PURPOSE: Saves instance handle and creates main window
 *
 *   COMMENTS:
 *
 *        In this function, we save the instance handle in a global variable and
 *        create and display the main program window.
 */

static BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    WNDCLASSEX wcFrame = {
                             sizeof(WNDCLASSEX),
                             CS_HREDRAW | CS_VREDRAW/*style*/,
                             FrameWndProc,
                             0/*cbClsExtra*/,
                             0/*cbWndExtra*/,
                             hInstance,
                             LoadIcon(hInstance, MAKEINTRESOURCE(IDI_REGEDIT)),
                             LoadCursor(0, IDC_ARROW),
                             0/*hbrBackground*/,
                             0/*lpszMenuName*/,
                             szFrameClass,
                             (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_REGEDIT), IMAGE_ICON,
                                              GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED)
                         };
    ATOM hFrameWndClass = RegisterClassEx(&wcFrame); /* register frame window class */

    WNDCLASSEX wcChild = {
                             sizeof(WNDCLASSEX),
                             CS_HREDRAW | CS_VREDRAW/*style*/,
                             ChildWndProc,
                             0/*cbClsExtra*/,
                             sizeof(HANDLE)/*cbWndExtra*/,
                             hInstance,
                             LoadIcon(hInstance, MAKEINTRESOURCE(IDI_REGEDIT)),
                             LoadCursor(0, IDC_ARROW),
                             0/*hbrBackground*/,
                             0/*lpszMenuName*/,
                             szChildClass,
                             (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_REGEDIT), IMAGE_ICON,
                                              GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED)

                         };
    ATOM hChildWndClass = RegisterClassEx(&wcChild); /* register child windows class */
    hChildWndClass = hChildWndClass; /* warning eater */

    hMenuFrame = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_REGEDIT_MENU));
    hPopupMenus = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_POPUP_MENUS));

    /* Initialize the Windows Common Controls DLL */
    InitCommonControls();

    /* register our hex editor control */
    HexEdit_Register();

    nClipboardFormat = RegisterClipboardFormat(strClipboardFormat);
    /* if (nClipboardFormat == 0) {
        DWORD dwError = GetLastError();
    } */

    hFrameWnd = CreateWindowEx(0, (LPCTSTR)(int)hFrameWndClass, szTitle,
                               WS_OVERLAPPEDWINDOW | WS_EX_CLIENTEDGE,
                               CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                               NULL, hMenuFrame, hInstance, NULL/*lpParam*/);

    if (!hFrameWnd) {
        return FALSE;
    }

    /* Create the status bar */
    hStatusBar = CreateStatusWindow(WS_VISIBLE|WS_CHILD|WS_CLIPSIBLINGS|SBT_NOBORDERS,
                                    _T(""), hFrameWnd, STATUS_WINDOW);
    if (hStatusBar) {
        /* Create the status bar panes */
        SetupStatusBar(hFrameWnd, FALSE);
        CheckMenuItem(GetSubMenu(hMenuFrame, ID_VIEW_MENU), ID_VIEW_STATUSBAR, MF_BYCOMMAND|MF_CHECKED);
    }
    ShowWindow(hFrameWnd, nCmdShow);
    UpdateWindow(hFrameWnd);
    return TRUE;
}

/******************************************************************************/

static void ExitInstance(void)
{
    DestroyMenu(hMenuFrame);
}

static BOOL TranslateChildTabMessage(MSG *msg)
{
    if (msg->message != WM_KEYDOWN) return FALSE;
    if (msg->wParam != VK_TAB) return FALSE;
    if (GetParent(msg->hwnd) != g_pChildWnd->hWnd) return FALSE;
    PostMessage(g_pChildWnd->hWnd, WM_COMMAND, ID_SWITCH_PANELS, 0);
    return TRUE;
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    MSG msg;
    HACCEL hAccel;

    if (ProcessCmdLine(lpCmdLine)) {
        return 0;
    }

    /* Initialize global strings */
    LoadString(hInstance, IDS_APP_TITLE, szTitle, COUNT_OF(szTitle));
    LoadString(hInstance, IDS_REGISTRY_DEFAULT_VALUE, g_pszDefaultValueName, COUNT_OF(g_pszDefaultValueName));

    /* Store instance handle in our global variable */
    hInst = hInstance;

    /* Perform application initialization */
    if (!InitInstance(hInstance, nCmdShow)) {
        return FALSE;
    }
    hAccel = LoadAccelerators(hInstance, (LPCTSTR)IDC_REGEDIT);

    /* Main message loop */
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(hFrameWnd, hAccel, &msg)
           && !TranslateChildTabMessage(&msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    ExitInstance();
    return msg.wParam;
}
