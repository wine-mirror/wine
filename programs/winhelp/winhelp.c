/*
 * Help Viewer
 *
 * Copyright    1996 Ulrich Schmid <uschmid@mail.hh.provi.de>
 *              2002 Sylvain Petreolle <spetreolle@yahoo.fr>
 *              2002 Eric Pouech <eric.pouech@wanadoo.fr>
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

#include <stdio.h>
#include <string.h>
#include "winbase.h"
#include "wingdi.h"
#include "windowsx.h"
#include "winhelp.h"
#include "winhelp_res.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winhelp);

static BOOL    WINHELP_RegisterWinClasses(void);
static LRESULT CALLBACK WINHELP_MainWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK WINHELP_TextWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK WINHELP_ButtonBoxWndProc(HWND, UINT, WPARAM, LPARAM);
static void    WINHELP_CheckPopup(UINT);
static BOOL    WINHELP_SplitLines(HWND hWnd, LPSIZE);
static void    WINHELP_InitFonts(HWND hWnd);
static void    WINHELP_DeleteLines(WINHELP_WINDOW*);
static void    WINHELP_DeleteWindow(WINHELP_WINDOW*);
static void    WINHELP_SetupText(HWND hWnd);
static WINHELP_LINE_PART* WINHELP_IsOverLink(HWND hWnd, WPARAM wParam, LPARAM lParam);

WINHELP_GLOBALS Globals = {3, 0, 0, 0, 0, 0};

static BOOL MacroTest = FALSE;

/***********************************************************************
 *
 *           WinMain
 */
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE prev, LPSTR cmdline, int show)
{
    MSG         msg;
    LONG        lHash = 0;

    Globals.hInstance = hInstance;
    
    /* Get options */
    while (*cmdline && (*cmdline == ' ' || *cmdline == '-'))
    {
        CHAR   option;
        LPCSTR topic_id;
        if (*cmdline++ == ' ') continue;

        option = *cmdline;
        if (option) cmdline++;
        while (*cmdline && *cmdline == ' ') cmdline++;
        switch (option)
	{
	case 'i':
	case 'I':
            topic_id = cmdline;
            while (*cmdline && *cmdline != ' ') cmdline++;
            if (*cmdline) *cmdline++ = '\0';
            lHash = HLPFILE_Hash(topic_id);
            break;

	case '3':
	case '4':
            Globals.wVersion = option - '0';
            break;

	case 't':
            MacroTest = TRUE;
            break;

        case 'x':
            show = SW_HIDE; 
            break;

        default:
            WINE_FIXME("Unsupported cmd line: %s\n", cmdline);
            break;
	}
    }

    /* Create primary window */
    WINHELP_RegisterWinClasses();
    WINHELP_CreateHelpWindowByHash(cmdline, lHash, "main", FALSE, NULL, NULL, show);

    /* Message loop */
    while (GetMessage(&msg, 0, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

/***********************************************************************
 *
 *           RegisterWinClasses
 */
static BOOL WINHELP_RegisterWinClasses(void)
{
    WNDCLASS class_main, class_button_box, class_text, class_shadow;

    class_main.style               = CS_HREDRAW | CS_VREDRAW;
    class_main.lpfnWndProc         = WINHELP_MainWndProc;
    class_main.cbClsExtra          = 0;
    class_main.cbWndExtra          = sizeof(LONG);
    class_main.hInstance           = Globals.hInstance;
    class_main.hIcon               = LoadIcon(0, IDI_APPLICATION);
    class_main.hCursor             = LoadCursor(0, IDC_ARROW);
    class_main.hbrBackground       = GetStockObject(WHITE_BRUSH);
    class_main.lpszMenuName        = 0;
    class_main.lpszClassName       = MAIN_WIN_CLASS_NAME;

    class_button_box               = class_main;
    class_button_box.lpfnWndProc   = WINHELP_ButtonBoxWndProc;
    class_button_box.hbrBackground = GetStockObject(GRAY_BRUSH);
    class_button_box.lpszClassName = BUTTON_BOX_WIN_CLASS_NAME;

    class_text = class_main;
    class_text.lpfnWndProc         = WINHELP_TextWndProc;
    class_text.lpszClassName       = TEXT_WIN_CLASS_NAME;

    class_shadow = class_main;
    class_shadow.lpfnWndProc       = DefWindowProc;
    class_shadow.hbrBackground     = GetStockObject(GRAY_BRUSH);
    class_shadow.lpszClassName     = SHADOW_WIN_CLASS_NAME;

    return (RegisterClass(&class_main) &&
            RegisterClass(&class_button_box) &&
            RegisterClass(&class_text) &&
            RegisterClass(&class_shadow));
}

typedef struct
{
    WORD size;
    WORD command;
    LONG data;
    LONG reserved;
    WORD ofsFilename;
    WORD ofsData;
} WINHELP,*LPWINHELP;

/******************************************************************
 *		WINHELP_HandleCommand
 *
 *
 */
static LRESULT  WINHELP_HandleCommand(HWND hSrcWnd, LPARAM lParam)
{
    COPYDATASTRUCT*     cds = (COPYDATASTRUCT*)lParam;
    WINHELP*            wh;

    if (cds->dwData != 0xA1DE505)
    {
        WINE_FIXME("Wrong magic number (%08lx)\n", cds->dwData);
        return 0;
    }

    wh = (WINHELP*)cds->lpData;

    if (wh)
    {
        WINE_TRACE("Got[%u]: cmd=%u data=%08lx fn=%s\n", 
                   wh->size, wh->command, wh->data,
                   wh->ofsFilename ? (LPSTR)wh + wh->ofsFilename : "");
        switch (wh->command)
        {
        case HELP_QUIT:
            MACRO_Exit();
            break;
        case HELP_FINDER:
            /* in fact, should be the topic dialog box */
            if (wh->ofsFilename)
            {
                MACRO_JumpHash((LPSTR)wh + wh->ofsFilename, "main", 0);
            }
            break;
        case HELP_CONTEXT:
        case HELP_SETCONTENTS:
        case HELP_CONTENTS:
        case HELP_CONTEXTPOPUP:
        case HELP_FORCEFILE:
        case HELP_HELPONHELP:
        case HELP_KEY:
        case HELP_PARTIALKEY:
        case HELP_COMMAND:
        case HELP_MULTIKEY:
        case HELP_SETWINPOS:
            WINE_FIXME("NIY\n");
            break;
        }
    }
    return 0L;
}

/***********************************************************************
 *
 *           WINHELP_CreateHelpWindow
 */

static BOOL WINHELP_CreateHelpWindow(HLPFILE_PAGE* page, LPCSTR lpszWindow,
                                     BOOL bPopup, HWND hParentWnd, LPPOINT mouse, INT nCmdShow)
{
    CHAR    szCaption[MAX_STRING_LEN];
    SIZE    size   = {CW_USEDEFAULT, 0/*CW_USEDEFAULT*/};
    POINT   origin = {240, 0};
    LPSTR   ptr;
    WINHELP_WINDOW *win, *oldwin;
    HLPFILE_MACRO  *macro;
    HWND hWnd;
    BOOL bPrimary;

    if (bPopup)
        lpszWindow = NULL;
    else if (!lpszWindow || !lpszWindow[0])
        lpszWindow = Globals.active_win->lpszName;
    bPrimary = lpszWindow && !lstrcmpi(lpszWindow, "main");

    /* Calculate horizontal size and position of a popup window */
    if (bPopup)
    {
        RECT parent_rect;
        GetWindowRect(hParentWnd, &parent_rect);
        size.cx = (parent_rect.right  - parent_rect.left) / 2;
        size.cy = 10; /* need a non null value, so that border are taken into account while computing */

        origin = *mouse;
        ClientToScreen(hParentWnd, &origin);
        origin.x -= size.cx / 2;
        origin.x  = min(origin.x, GetSystemMetrics(SM_CXSCREEN) - size.cx);
        origin.x  = max(origin.x, 0);
    }

    /* Initialize WINHELP_WINDOW struct */
    win = HeapAlloc(GetProcessHeap(), 0,
                    sizeof(WINHELP_WINDOW) + (lpszWindow ? strlen(lpszWindow) + 1 : 0));
    if (!win) return FALSE;

    win->next  = Globals.win_list;
    Globals.win_list = win;
    if (lpszWindow)
    {
        ptr = (char*)win + sizeof(WINHELP_WINDOW);
        lstrcpy(ptr, (LPSTR) lpszWindow);
        win->lpszName = ptr;
    }
    else win->lpszName = NULL;

    win->page = page;
    win->first_button = 0;
    win->first_line = 0;
    win->hMainWnd = 0;
    win->hButtonBoxWnd = 0;
    win->hTextWnd = 0;
    win->hShadowWnd = 0;

    win->hArrowCur = LoadCursorA(0, IDC_ARROWA);
    win->hHandCur = LoadCursorA(0, IDC_HANDA);

    Globals.active_win = win;

    /* Initialize default pushbuttons */
    if (MacroTest && !bPopup)
        MACRO_CreateButton("BTN_TEST", "&Test", "MacroTest");
    if (bPrimary && page)
    {
        CHAR    buffer[MAX_STRING_LEN];

        LoadString(Globals.hInstance, STID_CONTENTS, buffer, sizeof(buffer));
        MACRO_CreateButton("BTN_CONTENTS", buffer, "Contents()");
        LoadString(Globals.hInstance, STID_SEARCH,buffer, sizeof(buffer));
        MACRO_CreateButton("BTN_SEARCH", buffer, "Search()");
        LoadString(Globals.hInstance, STID_BACK, buffer, sizeof(buffer));
        MACRO_CreateButton("BTN_BACK", buffer, "Back()");
        LoadString(Globals.hInstance, STID_HISTORY, buffer, sizeof(buffer));
        MACRO_CreateButton("BTN_HISTORY", buffer, "History()");
        LoadString(Globals.hInstance, STID_TOPICS, buffer, sizeof(buffer));
        MACRO_CreateButton("BTN_TOPICS", buffer, "Finder()");
    }

    /* Initialize file specific pushbuttons */
    if (!bPopup && page)
        for (macro = page->file->first_macro; macro; macro = macro->next)
            MACRO_ExecuteMacro(macro->lpszMacro);

    /* Reuse existing window */
    if (lpszWindow)
        for (oldwin = win->next; oldwin; oldwin = oldwin->next)
            if (oldwin->lpszName && !lstrcmpi(oldwin->lpszName, lpszWindow))
            {
                WINHELP_BUTTON *button;

                win->hMainWnd      = oldwin->hMainWnd;
                win->hButtonBoxWnd = oldwin->hButtonBoxWnd;
                win->hTextWnd      = oldwin->hTextWnd;
                oldwin->hMainWnd = oldwin->hButtonBoxWnd = oldwin->hTextWnd = 0;

                SetWindowLong(win->hMainWnd,      0, (LONG) win);
                SetWindowLong(win->hButtonBoxWnd, 0, (LONG) win);
                SetWindowLong(win->hTextWnd,      0, (LONG) win);

                WINHELP_InitFonts(win->hMainWnd);

                if (page) {
                    SetWindowText(win->hMainWnd, page->file->lpszTitle);
                }

                WINHELP_SetupText(win->hTextWnd);
                InvalidateRect(win->hTextWnd, NULL, TRUE);
                SendMessage(win->hMainWnd, WM_USER, 0, 0);
                ShowWindow(win->hMainWnd, nCmdShow);
                UpdateWindow(win->hTextWnd);


                for (button = oldwin->first_button; button; button = button->next)
                    DestroyWindow(button->hWnd);

                WINHELP_DeleteWindow(oldwin);
                return TRUE;
            }

    /* Create main Window */
    if (!page) LoadString(Globals.hInstance, STID_WINE_HELP, szCaption, sizeof(szCaption));
    hWnd = CreateWindow(bPopup ? TEXT_WIN_CLASS_NAME : MAIN_WIN_CLASS_NAME,
                        page ? page->file->lpszTitle : szCaption,
                        bPopup ? WS_POPUPWINDOW | WS_BORDER : WS_OVERLAPPEDWINDOW,
                        origin.x, origin.y, size.cx, size.cy,
                        0, bPrimary ? LoadMenu(Globals.hInstance, MAKEINTRESOURCE(MAIN_MENU)) : 0,
                        Globals.hInstance, win);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

/***********************************************************************
 *
 *           WINHELP_CreateHelpWindowByPage
 */
BOOL WINHELP_CreateHelpWindowByPage(HLPFILE_PAGE* page, LPCSTR lpszWindow,
                                    BOOL bPopup, HWND hParentWnd, LPPOINT mouse, INT nCmdShow)
{
    if (page) page->file->wRefCount++;
    return WINHELP_CreateHelpWindow(page, lpszWindow, bPopup, hParentWnd, mouse, nCmdShow);
}

/***********************************************************************
 *
 *           WINHELP_CreateHelpWindowByHash
 */
BOOL WINHELP_CreateHelpWindowByHash(LPCSTR lpszFile, LONG lHash, LPCSTR lpszWindow,
                                    BOOL bPopup, HWND hParentWnd, LPPOINT mouse, INT nCmdShow)
{
    HLPFILE_PAGE*       page;

    /* Read help file */
    if (lpszFile[0])
    {
        page = lHash ? HLPFILE_PageByHash(lpszFile, lHash) : HLPFILE_Contents(lpszFile);

        /* Add Suffix `.hlp' */
        if (!page && lstrcmpi(lpszFile + strlen(lpszFile) - 4, ".hlp"))
	{
            CHAR      szFile_hlp[MAX_PATHNAME_LEN];

            lstrcpyn(szFile_hlp, lpszFile, sizeof(szFile_hlp) - 4);
            szFile_hlp[sizeof(szFile_hlp) - 5] = '\0';
            lstrcat(szFile_hlp, ".hlp");

            page = lHash ? HLPFILE_PageByHash(szFile_hlp, lHash) : HLPFILE_Contents(szFile_hlp);
            if (!page)
	    {
                WINHELP_MessageBoxIDS_s(STID_HLPFILE_ERROR_s, lpszFile, STID_WHERROR, MB_OK);
                if (Globals.win_list) return FALSE;
	    }
	}
    }
    else page = NULL;
    return WINHELP_CreateHelpWindowByPage(page, lpszWindow, bPopup, hParentWnd, mouse, nCmdShow);
}

/***********************************************************************
 *
 *           WINHELP_MainWndProc
 */
static LRESULT CALLBACK WINHELP_MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WINHELP_WINDOW *win;
    WINHELP_BUTTON *button;
    RECT rect, button_box_rect;
    INT  text_top;

    WINHELP_CheckPopup(msg);

    switch (msg)
    {
    case WM_NCCREATE:
        win = (WINHELP_WINDOW*) ((LPCREATESTRUCT) lParam)->lpCreateParams;
        SetWindowLong(hWnd, 0, (LONG) win);
        win->hMainWnd = hWnd;
        break;

    case WM_CREATE:
        win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);

        /* Create button box and text Window */
        CreateWindow(BUTTON_BOX_WIN_CLASS_NAME, "", WS_CHILD | WS_VISIBLE,
                     0, 0, 0, 0, hWnd, 0, Globals.hInstance, win);

        CreateWindow(TEXT_WIN_CLASS_NAME, "", WS_CHILD | WS_VISIBLE,
                     0, 0, 0, 0, hWnd, 0, Globals.hInstance, win);

        /* Fall through */
    case WM_USER:
    case WM_WINDOWPOSCHANGED:
        win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);
        GetClientRect(hWnd, &rect);

        /* Update button box and text Window */
        SetWindowPos(win->hButtonBoxWnd, HWND_TOP,
                     rect.left, rect.top,
                     rect.right - rect.left,
                     rect.bottom - rect.top, 0);

        GetWindowRect(win->hButtonBoxWnd, &button_box_rect);
        text_top = rect.top + button_box_rect.bottom - button_box_rect.top;

        SetWindowPos(win->hTextWnd, HWND_TOP,
                     rect.left, text_top,
                     rect.right - rect.left,
                     rect.bottom - text_top, 0);

        break;

    case WM_COMMAND:
        Globals.active_win = win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);
        switch (wParam)
	{
            /* Menu FILE */
	case MNID_FILE_OPEN:    MACRO_FileOpen();       break;
	case MNID_FILE_PRINT:	MACRO_Print();          break;
	case MNID_FILE_SETUP:	MACRO_PrinterSetup();   break;
	case MNID_FILE_EXIT:	MACRO_Exit();           break;

            /* Menu EDIT */
	case MNID_EDIT_COPYDLG: MACRO_CopyDialog();     break;
	case MNID_EDIT_ANNOTATE:MACRO_Annotate();       break;

            /* Menu Bookmark */
	case MNID_BKMK_DEFINE:  MACRO_BookmarkDefine(); break;

            /* Menu Help */
	case MNID_HELP_HELPON:	MACRO_HelpOn();         break;
	case MNID_HELP_HELPTOP: MACRO_HelpOnTop();      break;
	case MNID_HELP_ABOUT:	MACRO_About();          break;
	case MNID_HELP_WINE:    ShellAbout(hWnd, "WINE", "Help", 0); break;

	default:
            /* Buttons */
            for (button = win->first_button; button; button = button->next)
                if (wParam == button->wParam) break;
            if (button)
                MACRO_ExecuteMacro(button->lpszMacro);
            else
                WINHELP_MessageBoxIDS(STID_NOT_IMPLEMENTED, 0x121, MB_OK);
            break;
	}
        break;
    case WM_DESTROY:
        if (Globals.hPopupWnd) DestroyWindow(Globals.hPopupWnd);
        break;
    case WM_COPYDATA:
        return WINHELP_HandleCommand((HWND)wParam, lParam);
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

/***********************************************************************
 *
 *           WINHELP_ButtonBoxWndProc
 */
static LRESULT CALLBACK WINHELP_ButtonBoxWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WINDOWPOS      *winpos;
    WINHELP_WINDOW *win;
    WINHELP_BUTTON *button;
    SIZE button_size;
    INT  x, y;

    WINHELP_CheckPopup(msg);

    switch (msg)
    {
    case WM_NCCREATE:
        win = (WINHELP_WINDOW*) ((LPCREATESTRUCT) lParam)->lpCreateParams;
        SetWindowLong(hWnd, 0, (LONG) win);
        win->hButtonBoxWnd = hWnd;
        break;

    case WM_WINDOWPOSCHANGING:
        winpos = (WINDOWPOS*) lParam;
        win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);

        /* Update buttons */
        button_size.cx = 0;
        button_size.cy = 0;
        for (button = win->first_button; button; button = button->next)
	{
            HDC  hDc;
            SIZE textsize;
            if (!button->hWnd)
                button->hWnd = CreateWindow(STRING_BUTTON, (LPSTR) button->lpszName,
                                            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                            0, 0, 0, 0,
                                            hWnd, (HMENU) button->wParam,
                                            Globals.hInstance, 0);
            hDc = GetDC(button->hWnd);
            GetTextExtentPoint(hDc, button->lpszName,
                               lstrlen(button->lpszName), &textsize);
            ReleaseDC(button->hWnd, hDc);

            button_size.cx = max(button_size.cx, textsize.cx + BUTTON_CX);
            button_size.cy = max(button_size.cy, textsize.cy + BUTTON_CY);
	}

        x = 0;
        y = 0;
        for (button = win->first_button; button; button = button->next)
	{
            SetWindowPos(button->hWnd, HWND_TOP, x, y, button_size.cx, button_size.cy, 0);

            if (x + 2 * button_size.cx <= winpos->cx)
                x += button_size.cx;
            else
                x = 0, y += button_size.cy;
	}
        winpos->cy = y + (x ? button_size.cy : 0);
        break;

    case WM_COMMAND:
        SendMessage(GetParent(hWnd), msg, wParam, lParam);
        break;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

/***********************************************************************
 *
 *           WINHELP_TextWndProc
 */
static LRESULT CALLBACK WINHELP_TextWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WINHELP_WINDOW    *win;
    WINHELP_LINE      *line;
    WINHELP_LINE_PART *part;
    WINDOWPOS         *winpos;
    PAINTSTRUCT        ps;
    HDC   hDc;
    POINT mouse;
    INT   scroll_pos;
    HWND  hPopupWnd;
    BOOL  bExit;

    if (msg != WM_LBUTTONDOWN)
        WINHELP_CheckPopup(msg);

    switch (msg)
    {
    case WM_NCCREATE:
        win = (WINHELP_WINDOW*) ((LPCREATESTRUCT) lParam)->lpCreateParams;
        SetWindowLong(hWnd, 0, (LONG) win);
        win->hTextWnd = hWnd;
        if (!win->lpszName) Globals.hPopupWnd = win->hMainWnd = hWnd;
        WINHELP_InitFonts(hWnd);
        break;

    case WM_CREATE:
        win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);

        /* Calculate vertical size and position of a popup window */
        if (!win->lpszName)
	{
            POINT origin;
            RECT old_window_rect;
            RECT old_client_rect;
            SIZE old_window_size;
            SIZE old_client_size;
            SIZE new_client_size;
            SIZE new_window_size;

            GetWindowRect(hWnd, &old_window_rect);
            origin.x = old_window_rect.left;
            origin.y = old_window_rect.top;
            old_window_size.cx = old_window_rect.right  - old_window_rect.left;
            old_window_size.cy = old_window_rect.bottom - old_window_rect.top;

            GetClientRect(hWnd, &old_client_rect);
            old_client_size.cx = old_client_rect.right  - old_client_rect.left;
            old_client_size.cy = old_client_rect.bottom - old_client_rect.top;

            new_client_size = old_client_size;
            WINHELP_SplitLines(hWnd, &new_client_size);

            if (origin.y + POPUP_YDISTANCE + new_client_size.cy <= GetSystemMetrics(SM_CYSCREEN))
                origin.y += POPUP_YDISTANCE;
            else
                origin.y -= POPUP_YDISTANCE + new_client_size.cy;

            new_window_size.cx = old_window_size.cx - old_client_size.cx + new_client_size.cx;
            new_window_size.cy = old_window_size.cy - old_client_size.cy + new_client_size.cy;

            win->hShadowWnd =
                CreateWindow(SHADOW_WIN_CLASS_NAME, "", WS_POPUP,
                             origin.x + SHADOW_DX, origin.y + SHADOW_DY,
                             new_window_size.cx, new_window_size.cy,
                             0, 0, Globals.hInstance, 0);

            SetWindowPos(hWnd, HWND_TOP, origin.x, origin.y,
                         new_window_size.cx, new_window_size.cy,
                         0);
            SetWindowPos(win->hShadowWnd, hWnd, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
            ShowWindow(win->hShadowWnd, SW_NORMAL);
            SetActiveWindow(hWnd);
	}
        break;

    case WM_WINDOWPOSCHANGED:
        winpos = (WINDOWPOS*) lParam;

        if (!(winpos->flags & SWP_NOSIZE)) WINHELP_SetupText(hWnd);
        break;

    case WM_VSCROLL:
    {
	BOOL  update = TRUE;
	RECT  rect;
	INT   Min, Max;
	INT   CurPos = GetScrollPos(hWnd, SB_VERT);
	GetScrollRange(hWnd, SB_VERT, &Min, &Max);
	GetClientRect(hWnd, &rect);

	switch (wParam & 0xffff)
        {
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION: CurPos  = wParam >> 16;                   break;
        case SB_TOP:           CurPos  = Min;                            break;
        case SB_BOTTOM:        CurPos  = Max;                            break;
        case SB_PAGEUP:        CurPos -= (rect.bottom - rect.top) / 2;   break;
        case SB_PAGEDOWN:      CurPos += (rect.bottom - rect.top) / 2;   break;
        case SB_LINEUP:        CurPos -= GetSystemMetrics(SM_CXVSCROLL); break;
        case SB_LINEDOWN:      CurPos += GetSystemMetrics(SM_CXVSCROLL); break;
        default: update = FALSE;
        }
	if (update)
        {
	    INT dy = GetScrollPos(hWnd, SB_VERT) - CurPos;
	    SetScrollPos(hWnd, SB_VERT, CurPos, TRUE);
	    ScrollWindow(hWnd, 0, dy, NULL, NULL);
	    UpdateWindow(hWnd);
        }
    }
    break;

    case WM_PAINT:
        hDc = BeginPaint(hWnd, &ps);
        win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);
        scroll_pos = GetScrollPos(hWnd, SB_VERT);

        for (line = win->first_line; line; line = line->next)
        {
            for (part = &line->first_part; part; part = part->next)
            {
                switch (part->cookie)
                {
                case hlp_line_part_text:
                    SelectObject(hDc, part->u.text.hFont);
                    SetTextColor(hDc, part->u.text.color);
                    TextOut(hDc, part->rect.left, part->rect.top - scroll_pos,
                            part->u.text.lpsText, part->u.text.wTextLen);
                    if (part->u.text.wUnderline)
                    {
                        HPEN    hPen;

                        switch (part->u.text.wUnderline)
                        {
                        case 1: /* simple */
                        case 2: /* double */
                            hPen = CreatePen(PS_SOLID, 1, part->u.text.color);
                            break;
                        case 3: /* dotted */
                            hPen = CreatePen(PS_DOT, 1, part->u.text.color);
                            break;
                        default:
                            WINE_FIXME("Unknow underline type\n");
                            continue;
                        }

                        SelectObject(hDc, hPen);
                        MoveToEx(hDc, part->rect.left, part->rect.bottom - scroll_pos - 1, NULL);
                        LineTo(hDc, part->rect.right, part->rect.bottom - scroll_pos - 1);
                        if (part->u.text.wUnderline == 2)
                        {
                            MoveToEx(hDc, part->rect.left, part->rect.bottom - scroll_pos + 1, NULL);
                            LineTo(hDc, part->rect.right, part->rect.bottom - scroll_pos + 1);
                        }
                        DeleteObject(hPen);
                    }
                    break;
                case hlp_line_part_image:
                    {
                        HDC hMemDC;

                        hMemDC = CreateCompatibleDC(hDc);
                        SelectObject(hMemDC, part->u.image.hBitmap);
                        BitBlt(hDc, part->rect.left, part->rect.top - scroll_pos,
                               part->rect.right - part->rect.left, part->rect.bottom - part->rect.top,
                               hMemDC, 0, 0, SRCCOPY);
                        DeleteDC(hMemDC);
                    }
                    break;
                }
            }
        }

        EndPaint(hWnd, &ps);
        break;

    case WM_MOUSEMOVE:
        win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);

        if (WINHELP_IsOverLink(hWnd, wParam, lParam))
            SetCursor(win->hHandCur); /* set to hand pointer cursor to indicate a link */
        else
            SetCursor(win->hArrowCur); /* set to hand pointer cursor to indicate a link */

        break;

    case WM_LBUTTONDOWN:
        win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);

        hPopupWnd = Globals.hPopupWnd;
        Globals.hPopupWnd = 0;

        part = WINHELP_IsOverLink(hWnd, wParam, lParam);
        if (part)
        {
            mouse.x = LOWORD(lParam);
            mouse.y = HIWORD(lParam);

            switch (part->link.cookie)
            {
            case hlp_link_none:
                break;
            case hlp_link_link:
                WINHELP_CreateHelpWindowByHash(part->link.lpszString, part->link.lHash, NULL,
                                               FALSE, hWnd, &mouse, SW_NORMAL);
                break;
            case hlp_link_popup:
                WINHELP_CreateHelpWindowByHash(part->link.lpszString, part->link.lHash, NULL,
                                               TRUE, hWnd, &mouse, SW_NORMAL);
                break;
            case hlp_link_macro:
                MACRO_ExecuteMacro(part->link.lpszString);
                break;
            default:
                WINE_FIXME("Unknown link cookie %d\n", part->link.cookie);
            }
        }

        if (hPopupWnd)
            DestroyWindow(hPopupWnd);
        break;

    case WM_NCDESTROY:
        win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);

        if (hWnd == Globals.hPopupWnd) Globals.hPopupWnd = 0;
        if (win->hShadowWnd) DestroyWindow(win->hShadowWnd);

        bExit = (Globals.wVersion >= 4 && !lstrcmpi(win->lpszName, "main"));

        WINHELP_DeleteWindow(win);

        if (bExit) MACRO_Exit();

        if (!Globals.win_list)
            PostQuitMessage(0);
        break;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

/***********************************************************************
 *
 *           SetupText
 */
static void WINHELP_SetupText(HWND hWnd)
{
    HDC  hDc = GetDC(hWnd);
    RECT rect;
    SIZE newsize;

    ShowScrollBar(hWnd, SB_VERT, FALSE);
    if (!WINHELP_SplitLines(hWnd, NULL))
    {
        ShowScrollBar(hWnd, SB_VERT, TRUE);
        GetClientRect(hWnd, &rect);

        WINHELP_SplitLines(hWnd, &newsize);
        SetScrollRange(hWnd, SB_VERT, 0, rect.top + newsize.cy - rect.bottom, TRUE);
    }
    else SetScrollPos(hWnd, SB_VERT, 0, FALSE);

    ReleaseDC(hWnd, hDc);
}

/***********************************************************************
 *
 *           WINHELP_AppendText
 */
static BOOL WINHELP_AppendText(WINHELP_LINE ***linep, WINHELP_LINE_PART ***partp,
			       LPSIZE space, LPSIZE textsize,
			       INT *line_ascent, INT ascent,
			       LPCSTR text, UINT textlen,
			       HFONT font, COLORREF color, HLPFILE_LINK *link,
                               unsigned underline)
{
    WINHELP_LINE      *line;
    WINHELP_LINE_PART *part;
    LPSTR ptr;

    if (!*partp) /* New line */
    {
        *line_ascent  = ascent;

        line = HeapAlloc(GetProcessHeap(), 0,
                         sizeof(WINHELP_LINE) + textlen + (link ? lstrlen(link->lpszString) + 1 : 0));
        if (!line) return FALSE;

        line->next    = 0;
        part          = &line->first_part;
        ptr           = (char*)line + sizeof(WINHELP_LINE);

        line->rect.top    = (**linep ? (**linep)->rect.bottom : 0) + space->cy;
        line->rect.bottom = line->rect.top;
        line->rect.left   = space->cx;
        line->rect.right  = space->cx;

        if (**linep) *linep = &(**linep)->next;
        **linep = line;
        space->cy = 0;
    }
    else /* Same line */
    {
        line = **linep;

        if (*line_ascent < ascent)
	{
            WINHELP_LINE_PART *p;
            for (p = &line->first_part; p; p = p->next)
	    {
                p->rect.top    += ascent - *line_ascent;
                p->rect.bottom += ascent - *line_ascent;
	    }
            line->rect.bottom += ascent - *line_ascent;
            *line_ascent = ascent;
	}

        part = HeapAlloc(GetProcessHeap(), 0,
                         sizeof(WINHELP_LINE_PART) + textlen +
                         (link ? lstrlen(link->lpszString) + 1 : 0));
        if (!part) return FALSE;
        **partp = part;
        ptr     = (char*)part + sizeof(WINHELP_LINE_PART);
    }

    memcpy(ptr, text, textlen);
    part->cookie            = hlp_line_part_text;
    part->rect.left         = line->rect.right + (*partp ? space->cx : 0);
    part->rect.right        = part->rect.left + textsize->cx;
    line->rect.right        = part->rect.right;
    part->rect.top          =
        ((*partp) ? line->rect.top : line->rect.bottom) + *line_ascent - ascent;
    part->rect.bottom       = part->rect.top + textsize->cy;
    line->rect.bottom       = max(line->rect.bottom, part->rect.bottom);
    part->u.text.lpsText    = ptr;
    part->u.text.wTextLen   = textlen;
    part->u.text.hFont      = font;
    part->u.text.color      = color;
    part->u.text.wUnderline = underline;

    WINE_TRACE("Appended text '%*.*s'[%d] @ (%d,%d-%d,%d)\n",
               part->u.text.wTextLen,
               part->u.text.wTextLen,
               part->u.text.lpsText,
               part->u.text.wTextLen,
               part->rect.left, part->rect.top, part->rect.right, part->rect.bottom);
    if (link)
    {
        strcpy(ptr + textlen, link->lpszString);
        part->link.lpszString = ptr + textlen;
        part->link.cookie     = link->cookie;
        part->link.lHash      = link->lHash;
        part->link.bClrChange = link->bClrChange;
    }
    else part->link.cookie = hlp_link_none;

    part->next          = 0;
    *partp              = &part->next;

    space->cx = 0;

    return TRUE;
}

/***********************************************************************
 *
 *           WINHELP_AppendBitmap
 */
static BOOL WINHELP_AppendBitmap(WINHELP_LINE ***linep, WINHELP_LINE_PART ***partp,
                                 LPSIZE space,
                                 HBITMAP hBmp, LPSIZE bmpSize,
                                 HLPFILE_LINK *link, unsigned pos)
{
    WINHELP_LINE      *line;
    WINHELP_LINE_PART *part;
    LPSTR              ptr;

    if (!*partp || pos == 1) /* New line */
    {
        line = HeapAlloc(GetProcessHeap(), 0,
                         sizeof(WINHELP_LINE) + (link ? lstrlen(link->lpszString) + 1 : 0));
        if (!line) return FALSE;

        line->next    = NULL;
        part          = &line->first_part;

        line->rect.top    = (**linep ? (**linep)->rect.bottom : 0) + space->cy;
        line->rect.bottom = line->rect.top;
        line->rect.left   = space->cx;
        line->rect.right  = space->cx;

        if (**linep) *linep = &(**linep)->next;
        **linep = line;
        space->cy = 0;
        ptr = (char*)line + sizeof(WINHELP_LINE);
    }
    else /* Same line */
    {
        if (pos == 2) WINE_FIXME("Left alignment not handled\n");
        line = **linep;

        part = HeapAlloc(GetProcessHeap(), 0,
                         sizeof(WINHELP_LINE_PART) +
                         (link ? lstrlen(link->lpszString) + 1 : 0));
        if (!part) return FALSE;
        **partp = part;
        ptr = (char*)part + sizeof(WINHELP_LINE_PART);
    }

    part->cookie          = hlp_line_part_image;
    part->rect.left       = line->rect.right + (*partp ? space->cx : 0);
    part->rect.right      = part->rect.left + bmpSize->cx;
    line->rect.right      = part->rect.right;
    part->rect.top        =
        ((*partp) ? line->rect.top : line->rect.bottom);
    part->rect.bottom     = part->rect.top + bmpSize->cy;
    line->rect.bottom     = max(line->rect.bottom, part->rect.bottom);
    part->u.image.hBitmap = hBmp;

    WINE_TRACE("Appended bitmap '%d' @ (%d,%d-%d,%d)\n",
               (unsigned)part->u.image.hBitmap,
               part->rect.left, part->rect.top, part->rect.right, part->rect.bottom);

    if (link)
    {
        strcpy(ptr, link->lpszString);
        part->link.lpszString = ptr;
        part->link.cookie     = link->cookie;
        part->link.lHash      = link->lHash;
        part->link.bClrChange = link->bClrChange;
    }
    else part->link.cookie = hlp_link_none;

    part->next            = NULL;
    *partp                = &part->next;

    space->cx = 0;

    return TRUE;
}


/***********************************************************************
 *
 *           WINHELP_SplitLines
 */
static BOOL WINHELP_SplitLines(HWND hWnd, LPSIZE newsize)
{
    WINHELP_WINDOW     *win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);
    HLPFILE_PARAGRAPH  *p;
    WINHELP_LINE      **line = &win->first_line;
    WINHELP_LINE_PART **part = 0;
    INT                 line_ascent = 0;
    SIZE                space;
    RECT                rect;
    HDC                 hDc;

    if (newsize) newsize->cx = newsize->cy = 0;

    if (!win->page) return TRUE;

    WINHELP_DeleteLines(win);

    GetClientRect(hWnd, &rect);

    rect.top    += INTERNAL_BORDER_WIDTH;
    rect.left   += INTERNAL_BORDER_WIDTH;
    rect.right  -= INTERNAL_BORDER_WIDTH;
    rect.bottom -= INTERNAL_BORDER_WIDTH;

    space.cy = rect.top;
    space.cx = rect.left;

    hDc = GetDC(hWnd);

    for (p = win->page->first_paragraph; p; p = p->next)
    {
        switch (p->cookie)
        {
        case para_normal_text:
        case para_debug_text:
            {
                TEXTMETRIC tm;
                SIZE textsize    = {0, 0};
                LPCSTR text      = p->u.text.lpszText;
                UINT indent      = 0;
                UINT len         = strlen(text);
                unsigned underline = 0;

                HFONT hFont = 0;
                COLORREF color = RGB(0, 0, 0);

                if (p->u.text.wFont < win->page->file->numFonts)
                {
                    HLPFILE*    hlpfile = win->page->file;

                    if (!hlpfile->fonts[p->u.text.wFont].hFont)
                        hlpfile->fonts[p->u.text.wFont].hFont = CreateFontIndirect(&hlpfile->fonts[p->u.text.wFont].LogFont);
                    hFont = hlpfile->fonts[p->u.text.wFont].hFont;
                    color = hlpfile->fonts[p->u.text.wFont].color;
                }
                else
                {
                    UINT  wFont = (p->u.text.wFont < win->fonts_len) ? p->u.text.wFont : 0;

                    hFont = win->fonts[wFont];
                }

                if (p->link)
                {
                    underline = (p->link->cookie == hlp_link_popup) ? 3 : 1;
                    color = RGB(0, 0x80, 0);
                }
                if (p->cookie == para_debug_text) color = RGB(0xff, 0, 0);

                SelectObject(hDc, hFont);

                GetTextMetrics(hDc, &tm);

                if (p->u.text.wIndent)
                {
                    indent = p->u.text.wIndent * 5 * tm.tmAveCharWidth;
                    if (!part)
                        space.cx = rect.left + indent - 2 * tm.tmAveCharWidth;
                }

                if (p->u.text.wVSpace)
                {
                    part = 0;
                    space.cx = rect.left + indent;
                    space.cy += (p->u.text.wVSpace - 1) * tm.tmHeight;
                }

                if (p->u.text.wHSpace)
                {
                    space.cx += p->u.text.wHSpace * 2 * tm.tmAveCharWidth;
                }

                WINE_TRACE("splitting text %s\n", text);

                while (len)
                {
                    INT free_width = rect.right - (part ? (*line)->rect.right : rect.left) - space.cx;
                    UINT low = 0, curr = len, high = len, textlen = 0;

                    if (free_width > 0)
                    {
                        while (1)
                        {
                            GetTextExtentPoint(hDc, text, curr, &textsize);

                            if (textsize.cx <= free_width) low = curr;
                            else high = curr;

                            if (high <= low + 1) break;

                            if (textsize.cx) curr = (curr * free_width) / textsize.cx;
                            if (curr <= low) curr = low + 1;
                            else if (curr >= high) curr = high - 1;
                        }
                        textlen = low;
                        while (textlen && text[textlen] && text[textlen] != ' ') textlen--;
                    }
                    if (!part && !textlen) textlen = max(low, 1);

                    if (free_width <= 0 || !textlen)
                    {
                        part = 0;
                        space.cx = rect.left + indent;
                        space.cx = min(space.cx, rect.right - rect.left - 1);
                        continue;
                    }

                    WINE_TRACE("\t => %d %*s\n", textlen, textlen, text);

                    if (!WINHELP_AppendText(&line, &part, &space, &textsize,
                                            &line_ascent, tm.tmAscent,
                                            text, textlen, hFont, color, p->link, underline) ||
                        (!newsize && (*line)->rect.bottom > rect.bottom))
                    {
                        ReleaseDC(hWnd, hDc);
                        return FALSE;
                    }

                    if (newsize)
                        newsize->cx = max(newsize->cx, (*line)->rect.right + INTERNAL_BORDER_WIDTH);

                    len -= textlen;
                    text += textlen;
                    if (text[0] == ' ') text++, len--;
                }
            }
            break;
        case para_image:
            {
                DIBSECTION      dibs;
                SIZE            bmpSize;
                INT             free_width;

                if (p->u.image.pos & 0x8000)
                {
                    space.cx = rect.left;
                    if (*line)
                        space.cy += (*line)->rect.bottom - (*line)->rect.top;
                    part = 0;
                }

                GetObject(p->u.image.hBitmap, sizeof(dibs), &dibs);
                bmpSize.cx = dibs.dsBm.bmWidth;
                bmpSize.cy = dibs.dsBm.bmHeight;

                free_width = rect.right - ((part && *line) ? (*line)->rect.right : rect.left) - space.cx;
                if (free_width <= 0)
                {
                    part = NULL;
                    space.cx = rect.left;
                    space.cx = min(space.cx, rect.right - rect.left - 1);
                }
                if (!WINHELP_AppendBitmap(&line, &part, &space,
                                          p->u.image.hBitmap, &bmpSize,
                                          p->link, p->u.image.pos) ||
                    (!newsize && (*line)->rect.bottom > rect.bottom))
                {
                    return FALSE;
                }
            }
            break;
        }

    }

    if (newsize)
        newsize->cy = (*line)->rect.bottom + INTERNAL_BORDER_WIDTH;

    ReleaseDC(hWnd, hDc);
    return TRUE;
}

/***********************************************************************
 *
 *           WINHELP_CheckPopup
 */
static void WINHELP_CheckPopup(UINT msg)
{
    if (!Globals.hPopupWnd) return;

    switch (msg)
    {
    case WM_COMMAND:
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_NCLBUTTONDOWN:
    case WM_NCMBUTTONDOWN:
    case WM_NCRBUTTONDOWN:
        DestroyWindow(Globals.hPopupWnd);
        Globals.hPopupWnd = 0;
    }
}

/***********************************************************************
 *
 *           WINHELP_DeleteLines
 */
static void WINHELP_DeleteLines(WINHELP_WINDOW *win)
{
    WINHELP_LINE      *line, *next_line;
    WINHELP_LINE_PART *part, *next_part;
    for (line = win->first_line; line; line = next_line)
    {
        next_line = line->next;
        for (part = &line->first_part; part; part = next_part)
	{
            next_part = part->next;
            HeapFree(GetProcessHeap(), 0, part);
	}
    }
    win->first_line = 0;
}

/***********************************************************************
 *
 *           WINHELP_DeleteWindow
 */
static void WINHELP_DeleteWindow(WINHELP_WINDOW *win)
{
    WINHELP_WINDOW **w;

    for (w = &Globals.win_list; *w; w = &(*w)->next)
        if (*w == win)
        {
            *w = win->next;
            break;
        }

    if (win->hShadowWnd) DestroyWindow(win->hShadowWnd);
    HLPFILE_FreeHlpFilePage(win->page);
    WINHELP_DeleteLines(win);
    HeapFree(GetProcessHeap(), 0, win);
}

/***********************************************************************
 *
 *           WINHELP_InitFonts
 */
static void WINHELP_InitFonts(HWND hWnd)
{
    WINHELP_WINDOW *win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);
    LOGFONT logfontlist[] = {
        {-10, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, 0, 32, "Helv"},
        {-12, 0, 0, 0, 700, 0, 0, 0, 0, 0, 0, 0, 32, "Helv"},
        {-12, 0, 0, 0, 700, 0, 0, 0, 0, 0, 0, 0, 32, "Helv"},
        {-12, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, 0, 32, "Helv"},
        {-12, 0, 0, 0, 700, 0, 0, 0, 0, 0, 0, 0, 32, "Helv"},
        {-10, 0, 0, 0, 700, 0, 0, 0, 0, 0, 0, 0, 32, "Helv"},
        { -8, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, 0, 32, "Helv"}};
#define FONTS_LEN (sizeof(logfontlist)/sizeof(*logfontlist))

    static HFONT fonts[FONTS_LEN];
    static BOOL init = 0;

    win->fonts_len = FONTS_LEN;
    win->fonts = fonts;

    if (!init)
    {
        INT i;

        for(i = 0; i < FONTS_LEN; i++)
	{
            fonts[i] = CreateFontIndirect(&logfontlist[i]);
	}

        init = 1;
    }
}

/***********************************************************************
 *
 *           WINHELP_MessageBoxIDS
 */
INT WINHELP_MessageBoxIDS(UINT ids_text, UINT ids_title, WORD type)
{
    CHAR text[MAX_STRING_LEN];
    CHAR title[MAX_STRING_LEN];

    LoadString(Globals.hInstance, ids_text, text, sizeof(text));
    LoadString(Globals.hInstance, ids_title, title, sizeof(title));

    return MessageBox(0, text, title, type);
}

/***********************************************************************
 *
 *           MAIN_MessageBoxIDS_s
 */
INT WINHELP_MessageBoxIDS_s(UINT ids_text, LPCSTR str, UINT ids_title, WORD type)
{
    CHAR text[MAX_STRING_LEN];
    CHAR title[MAX_STRING_LEN];
    CHAR newtext[MAX_STRING_LEN + MAX_PATHNAME_LEN];

    LoadString(Globals.hInstance, ids_text, text, sizeof(text));
    LoadString(Globals.hInstance, ids_title, title, sizeof(title));
    wsprintf(newtext, text, str);

    return MessageBox(0, newtext, title, type);
}

/******************************************************************
 *		WINHELP_IsOverLink
 *
 *
 */
WINHELP_LINE_PART* WINHELP_IsOverLink(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    WINHELP_WINDOW* win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);
    POINT mouse;
    WINHELP_LINE      *line;
    WINHELP_LINE_PART *part;
    int scroll_pos = GetScrollPos(hWnd, SB_VERT);

    mouse.x = LOWORD(lParam);
    mouse.y = HIWORD(lParam);
    for (line = win->first_line; line; line = line->next)
    {
        for (part = &line->first_part; part; part = part->next)
        {
            if (part->link.cookie != hlp_link_none &&
                part->link.lpszString &&
                part->rect.left   <= mouse.x &&
                part->rect.right  >= mouse.x &&
                part->rect.top    <= mouse.y + scroll_pos &&
                part->rect.bottom >= mouse.y + scroll_pos)
            {
                return part;
            }
        }
    }

    return NULL;
}
