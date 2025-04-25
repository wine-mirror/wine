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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#include <stdio.h>
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlwapi.h>

#include "main.h"
#include "dialog.h"
#include "notepad_res.h"

NOTEPAD_GLOBALS Globals;
static ATOM aFINDMSGSTRING;
static RECT main_rect;

/***********************************************************************
 *
 *           SetFileNameAndEncoding
 *
 *  Sets global file name and encoding (which is used to preselect original
 *  encoding in Save As dialog, and when saving without using the Save As
 *  dialog).
 */
VOID SetFileNameAndEncoding(LPCWSTR szFileName, ENCODING enc)
{
    lstrcpyW(Globals.szFileName, szFileName);
    Globals.szFileTitle[0] = 0;
    GetFileTitleW(szFileName, Globals.szFileTitle, ARRAY_SIZE(Globals.szFileTitle));
    Globals.encFile = enc;
}

void UpdateStatusBar(void)
{
    int currentLine;
    int currentCol = -1;
    WCHAR statusTxt[256];
    int lineIndex;
    DWORD selStart;
    DWORD selEnd;

    SendMessageW(Globals.hEdit, EM_GETSEL, (WPARAM)&selStart, (LPARAM)&selEnd);
    if(selStart == selEnd)
        Globals.trackedSel = selStart;
    if(selStart < Globals.trackedSel)
        currentCol = selStart;
    else
        currentCol = selEnd;
    currentLine = SendMessageW(Globals.hEdit, EM_LINEFROMCHAR, currentCol, 0);
    lineIndex = SendMessageW(Globals.hEdit, EM_LINEINDEX, currentLine, 0);
    if(Globals.lastLn != currentLine || Globals.lastCol != currentCol)
    {
        swprintf(statusTxt, ARRAY_SIZE(statusTxt), Globals.szStatusString, currentLine + 1, (currentCol - lineIndex) + 1);
        SendMessageW(Globals.hStatusBar, SB_SETTEXTW, 0, (LPARAM)statusTxt);
        Globals.lastLn = currentLine;
        Globals.lastCol = currentCol;
    }
}

static void ToggleStatusBar(void)
{
    RECT rc;

    Globals.bStatusBar = !Globals.bStatusBar;
    CheckMenuItem(GetMenu(Globals.hMainWnd), CMD_SBAR,
            MF_BYCOMMAND | (Globals.bStatusBar ? MF_CHECKED : MF_UNCHECKED));
    GetClientRect(Globals.hMainWnd, &rc);
    ShowWindow(Globals.hStatusBar, Globals.bStatusBar ? SW_SHOW : SW_HIDE);
    updateWindowSize(rc.right, rc.bottom);
    UpdateStatusBar();
}

void updateWindowSize(int width, int height)
{
    int StatusBarHeight = 0;

    if(Globals.bStatusBar)
    {
        RECT SBarRect;

        SendMessageW(Globals.hStatusBar, WM_SIZE, 0, 0);
        GetWindowRect(Globals.hStatusBar, &SBarRect);
        StatusBarHeight = (SBarRect.bottom - SBarRect.top);
    }
    SetWindowPos(Globals.hEdit, NULL, 0, 0, width, height - StatusBarHeight,
                SWP_NOOWNERZORDER | SWP_NOZORDER);
}

/***********************************************************************
 *
 *           NOTEPAD_SaveSettingToRegistry
 *
 *  Save setting to registry HKCU\Software\Microsoft\Notepad.
 */
static VOID NOTEPAD_SaveSettingToRegistry(void)
{
    HKEY hkey;
    DWORD disp;

    if(RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Notepad", 0, NULL,
                REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, &disp) == ERROR_SUCCESS)
    {
        DWORD data;
        WINDOWPLACEMENT wndpl;

        wndpl.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(Globals.hMainWnd, &wndpl);
        main_rect = wndpl.rcNormalPosition;

#define SET_NOTEPAD_REG(hkey, value_name, value_data) do { DWORD data = value_data; RegSetValueExW(hkey, value_name, 0, REG_DWORD, (LPBYTE)&data, sizeof(DWORD)); }while(0)
        SET_NOTEPAD_REG(hkey, L"fWrap",            Globals.bWrapLongLines);
        SET_NOTEPAD_REG(hkey, L"iWindowPosX",      main_rect.left);
        SET_NOTEPAD_REG(hkey, L"iWindowPosY",      main_rect.top);
        SET_NOTEPAD_REG(hkey, L"iWindowPosDX",     main_rect.right - main_rect.left);
        SET_NOTEPAD_REG(hkey, L"iWindowPosDY",     main_rect.bottom - main_rect.top);
        SET_NOTEPAD_REG(hkey, L"lfCharSet",        Globals.lfFont.lfCharSet);
        SET_NOTEPAD_REG(hkey, L"lfClipPrecision",  Globals.lfFont.lfClipPrecision);
        SET_NOTEPAD_REG(hkey, L"lfEscapement",     Globals.lfFont.lfEscapement);
        SET_NOTEPAD_REG(hkey, L"lfItalic",         Globals.lfFont.lfItalic);
        SET_NOTEPAD_REG(hkey, L"lfOrientation",    Globals.lfFont.lfOrientation);
        SET_NOTEPAD_REG(hkey, L"lfOutPrecision",   Globals.lfFont.lfOutPrecision);
        SET_NOTEPAD_REG(hkey, L"lfPitchAndFamily", Globals.lfFont.lfPitchAndFamily);
        SET_NOTEPAD_REG(hkey, L"lfQuality",        Globals.lfFont.lfQuality);
        SET_NOTEPAD_REG(hkey, L"lfStrikeOut",      Globals.lfFont.lfStrikeOut);
        SET_NOTEPAD_REG(hkey, L"lfUnderline",      Globals.lfFont.lfUnderline);
        SET_NOTEPAD_REG(hkey, L"lfWeight",         Globals.lfFont.lfWeight);
        SET_NOTEPAD_REG(hkey, L"iMarginTop",       Globals.iMarginTop);
        SET_NOTEPAD_REG(hkey, L"iMarginBottom",    Globals.iMarginBottom);
        SET_NOTEPAD_REG(hkey, L"iMarginLeft",      Globals.iMarginLeft);
        SET_NOTEPAD_REG(hkey, L"iMarginRight",     Globals.iMarginRight);
        SET_NOTEPAD_REG(hkey, L"bStatusBar",       Globals.bStatusBar);
#undef SET_NOTEPAD_REG

        /* Store the current value as 10 * twips */
        data = MulDiv(abs(Globals.lfFont.lfHeight), 720, GetDpiForWindow(Globals.hMainWnd));
        RegSetValueExW(hkey, L"iPointSize", 0, REG_DWORD, (LPBYTE)&data, sizeof(DWORD));

        RegSetValueExW(hkey, L"lfFaceName", 0, REG_SZ, (LPBYTE)&Globals.lfFont.lfFaceName,
                      lstrlenW(Globals.lfFont.lfFaceName) * sizeof(Globals.lfFont.lfFaceName[0]));

        RegSetValueExW(hkey, L"szHeader", 0, REG_SZ, (LPBYTE)&Globals.szHeader,
                      lstrlenW(Globals.szHeader) * sizeof(Globals.szHeader[0]));

        RegSetValueExW(hkey, L"szTrailer", 0, REG_SZ, (LPBYTE)&Globals.szFooter,
                      lstrlenW(Globals.szFooter) * sizeof(Globals.szFooter[0]));

        RegCloseKey(hkey);
    }
}

/***********************************************************************
 *
 *           NOTEPAD_LoadSettingFromRegistry
 *
 *  Load setting from registry HKCU\Software\Microsoft\Notepad.
 */
static VOID NOTEPAD_LoadSettingFromRegistry(void)
{
    HKEY hkey;
    INT base_length, dx, dy;

    base_length = (GetSystemMetrics(SM_CXSCREEN) > GetSystemMetrics(SM_CYSCREEN))?
        GetSystemMetrics(SM_CYSCREEN) : GetSystemMetrics(SM_CXSCREEN);

    dx = base_length * .95;
    dy = dx * 3 / 4;
    SetRect( &main_rect, 0, 0, dx, dy );

    Globals.bWrapLongLines  = TRUE;
    Globals.iMarginTop = 2500;
    Globals.iMarginBottom = 2500;
    Globals.iMarginLeft = 2000;
    Globals.iMarginRight = 2000;
    Globals.bStatusBar = TRUE;
    
    Globals.lfFont.lfHeight         = -12;
    Globals.lfFont.lfWidth          = 0;
    Globals.lfFont.lfEscapement     = 0;
    Globals.lfFont.lfOrientation    = 0;
    Globals.lfFont.lfWeight         = FW_REGULAR;
    Globals.lfFont.lfItalic         = FALSE;
    Globals.lfFont.lfUnderline      = FALSE;
    Globals.lfFont.lfStrikeOut      = FALSE;
    Globals.lfFont.lfCharSet        = DEFAULT_CHARSET;
    Globals.lfFont.lfOutPrecision   = OUT_DEFAULT_PRECIS;
    Globals.lfFont.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
    Globals.lfFont.lfQuality        = DEFAULT_QUALITY;
    Globals.lfFont.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
    lstrcpyW(Globals.lfFont.lfFaceName, L"System");

    LoadStringW(Globals.hInstance, STRING_PAGESETUP_HEADERVALUE,
                Globals.szHeader, ARRAY_SIZE(Globals.szHeader));
    LoadStringW(Globals.hInstance, STRING_PAGESETUP_FOOTERVALUE,
                Globals.szFooter, ARRAY_SIZE(Globals.szFooter));

    if(RegOpenKeyW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Notepad", &hkey) == ERROR_SUCCESS)
    {
        WORD  data_helper[MAX_PATH];
        DWORD type, size;
        int point_size;

#define QUERY_NOTEPAD_REG(hkey, value_name, ret) do { DWORD type, data; DWORD size = sizeof(DWORD); if(RegQueryValueExW(hkey, value_name, 0, &type, (LPBYTE)&data, &size) == ERROR_SUCCESS) if(type == REG_DWORD) ret = data; } while(0)
        QUERY_NOTEPAD_REG(hkey, L"fWrap",            Globals.bWrapLongLines);
        QUERY_NOTEPAD_REG(hkey, L"iWindowPosX",      main_rect.left);
        QUERY_NOTEPAD_REG(hkey, L"iWindowPosY",      main_rect.top);
        QUERY_NOTEPAD_REG(hkey, L"iWindowPosDX",     dx);
        QUERY_NOTEPAD_REG(hkey, L"iWindowPosDY",     dy);
        QUERY_NOTEPAD_REG(hkey, L"lfCharSet",        Globals.lfFont.lfCharSet);
        QUERY_NOTEPAD_REG(hkey, L"lfClipPrecision",  Globals.lfFont.lfClipPrecision);
        QUERY_NOTEPAD_REG(hkey, L"lfEscapement",     Globals.lfFont.lfEscapement);
        QUERY_NOTEPAD_REG(hkey, L"lfItalic",         Globals.lfFont.lfItalic);
        QUERY_NOTEPAD_REG(hkey, L"lfOrientation",    Globals.lfFont.lfOrientation);
        QUERY_NOTEPAD_REG(hkey, L"lfOutPrecision",   Globals.lfFont.lfOutPrecision);
        QUERY_NOTEPAD_REG(hkey, L"lfPitchAndFamily", Globals.lfFont.lfPitchAndFamily);
        QUERY_NOTEPAD_REG(hkey, L"lfQuality",        Globals.lfFont.lfQuality);
        QUERY_NOTEPAD_REG(hkey, L"lfStrikeOut",      Globals.lfFont.lfStrikeOut);
        QUERY_NOTEPAD_REG(hkey, L"lfUnderline",      Globals.lfFont.lfUnderline);
        QUERY_NOTEPAD_REG(hkey, L"lfWeight",         Globals.lfFont.lfWeight);
        QUERY_NOTEPAD_REG(hkey, L"iMarginTop",       Globals.iMarginTop);
        QUERY_NOTEPAD_REG(hkey, L"iMarginBottom",    Globals.iMarginBottom);
        QUERY_NOTEPAD_REG(hkey, L"iMarginLeft",      Globals.iMarginLeft);
        QUERY_NOTEPAD_REG(hkey, L"iMarginRight",     Globals.iMarginRight);
        QUERY_NOTEPAD_REG(hkey, L"bStatusBar",       Globals.bStatusBar);
#undef QUERY_NOTEPAD_REG

        main_rect.right = main_rect.left + dx;
        main_rect.bottom = main_rect.top + dy;

        size = sizeof(DWORD);
        if(RegQueryValueExW(hkey, L"iPointSize", 0, &type, (LPBYTE)&point_size, &size) == ERROR_SUCCESS)
            if(type == REG_DWORD && point_size)
                /* The value is stored as 10 * twips */
                Globals.lfFont.lfHeight = -MulDiv(abs(point_size), GetDpiForWindow(GetDesktopWindow()), 720);

        size = sizeof(Globals.lfFont.lfFaceName);
        if(RegQueryValueExW(hkey, L"lfFaceName", 0, &type, (LPBYTE)&data_helper, &size) == ERROR_SUCCESS)
            if(type == REG_SZ)
                lstrcpyW(Globals.lfFont.lfFaceName, data_helper);

        size = sizeof(Globals.szHeader);
        if(RegQueryValueExW(hkey, L"szHeader", 0, &type, (LPBYTE)&data_helper, &size) == ERROR_SUCCESS)
            if(type == REG_SZ)
                lstrcpyW(Globals.szHeader, data_helper);

        size = sizeof(Globals.szFooter);
        if(RegQueryValueExW(hkey, L"szTrailer", 0, &type, (LPBYTE)&data_helper, &size) == ERROR_SUCCESS)
            if(type == REG_SZ)
                lstrcpyW(Globals.szFooter, data_helper);
        RegCloseKey(hkey);
    }
}

/***********************************************************************
 *
 *           NOTEPAD_MenuCommand
 *
 *  All handling of main menu events
 */
static int NOTEPAD_MenuCommand(WPARAM wParam)
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

    case CMD_SEARCH:           DIALOG_Search(); break;
    case CMD_SEARCH_NEXT:      DIALOG_SearchNext(); break;
    case CMD_REPLACE:          DIALOG_Replace(); break;
    case CMD_GO_TO:            DIALOG_EditGoTo(); break;
                               
    case CMD_WRAP:             DIALOG_EditWrap(); break;
    case CMD_FONT:             DIALOG_SelectFont(); break;
    case CMD_SBAR:             ToggleStatusBar(); break;

    case CMD_HELP_CONTENTS:    DIALOG_HelpContents(); break;
    case CMD_HELP_ABOUT_NOTEPAD: DIALOG_HelpAboutNotepad(); break;

    default:
	break;
    }
   return 0;
}

/***********************************************************************
 * Data Initialization
 */
static VOID NOTEPAD_InitData(VOID)
{
    LPWSTR p = Globals.szFilter;

    LoadStringW(Globals.hInstance, STRING_TEXT_FILES_TXT, p, MAX_STRING_LEN);
    p += lstrlenW(p) + 1;
    lstrcpyW(p, L"*.txt");
    p += lstrlenW(p) + 1;
    LoadStringW(Globals.hInstance, STRING_ALL_FILES, p, MAX_STRING_LEN);
    p += lstrlenW(p) + 1;
    lstrcpyW(p, L"*.*");
    p += lstrlenW(p) + 1;
    *p = '\0';
    Globals.hDevMode = NULL;
    Globals.hDevNames = NULL;

    CheckMenuItem(GetMenu(Globals.hMainWnd), CMD_WRAP,
            MF_BYCOMMAND | (Globals.bWrapLongLines ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(GetMenu(Globals.hMainWnd), CMD_SBAR,
            MF_BYCOMMAND | (Globals.bStatusBar ? MF_CHECKED : MF_UNCHECKED));
    ShowWindow(Globals.hStatusBar, Globals.bStatusBar ? SW_SHOW : SW_HIDE);
}

/***********************************************************************
 * Enable/disable items on the menu based on control state
 */
static VOID NOTEPAD_InitMenuPopup(HMENU menu, int index)
{
    int enable;

    EnableMenuItem(menu, CMD_UNDO,
        SendMessageW(Globals.hEdit, EM_CANUNDO, 0, 0) ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(menu, CMD_PASTE,
        IsClipboardFormatAvailable(CF_TEXT) ? MF_ENABLED : MF_GRAYED);
    enable = SendMessageW(Globals.hEdit, EM_GETSEL, 0, 0);
    enable = (HIWORD(enable) == LOWORD(enable)) ? MF_GRAYED : MF_ENABLED;
    EnableMenuItem(menu, CMD_CUT, enable);
    EnableMenuItem(menu, CMD_COPY, enable);
    EnableMenuItem(menu, CMD_DELETE, enable);
    
    EnableMenuItem(menu, CMD_SELECT_ALL,
        GetWindowTextLengthW(Globals.hEdit) ? MF_ENABLED : MF_GRAYED);
}

static LPWSTR NOTEPAD_StrRStr(LPWSTR pszSource, LPWSTR pszLast, LPWSTR pszSrch)
{
    int len = lstrlenW(pszSrch);
    pszLast--;
    while (pszLast >= pszSource)
    {
        if (StrCmpNW(pszLast, pszSrch, len) == 0)
            return pszLast;
        pszLast--;
    }
    return NULL;
}

/***********************************************************************
 * The user activated the Find dialog
 */
void NOTEPAD_DoFind(FINDREPLACEW *fr)
{
    LPWSTR content, found;
    int len = lstrlenW(fr->lpstrFindWhat);
    int fileLen;
    DWORD pos;

    fileLen = GetWindowTextLengthW(Globals.hEdit) + 1;
    content = HeapAlloc(GetProcessHeap(), 0, fileLen * sizeof(WCHAR));
    if (!content) return;
    GetWindowTextW(Globals.hEdit, content, fileLen);

    SendMessageW(Globals.hEdit, EM_GETSEL, 0, (LPARAM)&pos);
    switch (fr->Flags & (FR_DOWN|FR_MATCHCASE))
    {
        case 0:
            found = StrRStrIW(content, content+pos-len, fr->lpstrFindWhat);
            break;
        case FR_DOWN:
            found = StrStrIW(content+pos, fr->lpstrFindWhat);
            break;
        case FR_MATCHCASE:
            found = NOTEPAD_StrRStr(content, content+pos-len, fr->lpstrFindWhat);
            break;
        case FR_DOWN|FR_MATCHCASE:
            found = StrStrW(content+pos, fr->lpstrFindWhat);
            break;
        default:    /* shouldn't happen */
            return;
    }
    pos = found - content;
    HeapFree(GetProcessHeap(), 0, content);

    if (!found)
    {
        DIALOG_StringMsgBox(Globals.hFindReplaceDlg, STRING_NOTFOUND, fr->lpstrFindWhat,
            MB_ICONINFORMATION|MB_OK);
        return;
    }

    SendMessageW(Globals.hEdit, EM_SETSEL, pos, pos + len);
}

static void NOTEPAD_DoReplace(FINDREPLACEW *fr)
{
    LPWSTR content;
    int len = lstrlenW(fr->lpstrFindWhat);
    int fileLen;
    DWORD pos;
    DWORD pos_start;

    fileLen = GetWindowTextLengthW(Globals.hEdit) + 1;
    content = HeapAlloc(GetProcessHeap(), 0, fileLen * sizeof(WCHAR));
    if (!content) return;
    GetWindowTextW(Globals.hEdit, content, fileLen);

    SendMessageW(Globals.hEdit, EM_GETSEL, (WPARAM)&pos_start, (LPARAM)&pos);
    switch (fr->Flags & (FR_DOWN|FR_MATCHCASE))
    {
        case FR_DOWN:
            if ( pos-pos_start == len && StrCmpNIW(fr->lpstrFindWhat, content+pos_start, len) == 0)
                SendMessageW(Globals.hEdit, EM_REPLACESEL, TRUE, (LPARAM)fr->lpstrReplaceWith);
            break;
        case FR_DOWN|FR_MATCHCASE:
            if ( pos-pos_start == len && StrCmpNW(fr->lpstrFindWhat, content+pos_start, len) == 0)
                SendMessageW(Globals.hEdit, EM_REPLACESEL, TRUE, (LPARAM)fr->lpstrReplaceWith);
            break;
        default:    /* shouldn't happen */
            return;
    }
    HeapFree(GetProcessHeap(), 0, content);

    NOTEPAD_DoFind(fr);
}

static void NOTEPAD_DoReplaceAll(FINDREPLACEW *fr)
{
    LPWSTR content;
    int len = lstrlenW(fr->lpstrFindWhat);
    int fileLen;
    SIZE_T pos;

    SendMessageW(Globals.hEdit, EM_SETSEL, 0, 0);
    while(TRUE){
        fileLen = GetWindowTextLengthW(Globals.hEdit) + 1;
        content = HeapAlloc(GetProcessHeap(), 0, fileLen * sizeof(WCHAR));
        if (!content) return;
        GetWindowTextW(Globals.hEdit, content, fileLen);

        SendMessageW(Globals.hEdit, EM_GETSEL, 0, (LPARAM)&pos);
        switch (fr->Flags & (FR_DOWN|FR_MATCHCASE))
        {
            case FR_DOWN:
                pos = StrStrIW(content+pos, fr->lpstrFindWhat) - content;
                if (pos == -(SIZE_T)content) pos = ~(SIZE_T)0;
                break;
            case FR_DOWN|FR_MATCHCASE:
                pos = StrStrW(content+pos, fr->lpstrFindWhat) - content;
                if (pos == -(SIZE_T)content) pos = ~(SIZE_T)0;
                break;
            default:    /* shouldn't happen */
                return;
        }
        HeapFree(GetProcessHeap(), 0, content);

        if(pos == ~(SIZE_T)0)
        {
            SendMessageW(Globals.hEdit, EM_SETSEL, 0, 0);
            return;
        }
        SendMessageW(Globals.hEdit, EM_SETSEL, pos, pos + len);
        SendMessageW(Globals.hEdit, EM_REPLACESEL, TRUE, (LPARAM)fr->lpstrReplaceWith);
    }
}

LRESULT CALLBACK EDIT_CallBackProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                    UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch (msg)
    {
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
            UpdateStatusBar();
            break;
        case WM_MOUSEMOVE:
            if(wParam == MK_LBUTTON)
                UpdateStatusBar();
            break;

        default:
            break;
    }
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}


/***********************************************************************
 *
 *           NOTEPAD_WndProc
 */
static LRESULT WINAPI NOTEPAD_WndProc(HWND hWnd, UINT msg, WPARAM wParam,
                               LPARAM lParam)
{
    if (msg == aFINDMSGSTRING)      /* not a constant so can't be used in switch */
    {
        FINDREPLACEW *fr = (FINDREPLACEW *)lParam;

        if (fr->Flags & FR_DIALOGTERM)
            Globals.hFindReplaceDlg = NULL;
        if (fr->Flags & FR_FINDNEXT)
        {
            Globals.lastFind = *fr;
            NOTEPAD_DoFind(fr);
        }
        if (fr->Flags & FR_REPLACE)
        {
            Globals.lastFind = *fr;
            NOTEPAD_DoReplace(fr);
        }
        if (fr->Flags & FR_REPLACEALL)
        {
            Globals.lastFind = *fr;
            NOTEPAD_DoReplaceAll(fr);
        }
        return 0;
    }
    
    switch (msg) {

    case WM_CREATE:
    {
        DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
                        ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL;
        RECT rc;
        GetClientRect(hWnd, &rc);

        if (!Globals.bWrapLongLines) dwStyle |= WS_HSCROLL | ES_AUTOHSCROLL;

        Globals.hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"edit", NULL,
                             dwStyle, 0, 0, rc.right, rc.bottom, hWnd,
                             NULL, Globals.hInstance, NULL);

        SetWindowSubclass(Globals.hEdit, EDIT_CallBackProc, 0, 0);
        Globals.hFont = CreateFontIndirectW(&Globals.lfFont);
        SendMessageW(Globals.hEdit, WM_SETFONT, (WPARAM)Globals.hFont, FALSE);
        SendMessageW(Globals.hEdit, EM_LIMITTEXT, 0, 0);
        Globals.hStatusBar = CreateWindowExW(0, STATUSCLASSNAMEW, NULL,
                                 WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hWnd, NULL,
                                 Globals.hInstance, NULL);
        LoadStringW(Globals.hInstance, STRING_STATUSBAR, (LPWSTR)&Globals.szStatusString, 0);
        Globals.lastLn = -1;
        Globals.lastCol = -1;
        UpdateStatusBar();
        break;
    }

    case WM_COMMAND:
        NOTEPAD_MenuCommand(LOWORD(wParam));
        break;

    case WM_DESTROYCLIPBOARD:
        /*MessageBoxW(Globals.hMainWnd, "Empty clipboard", "Debug", MB_ICONEXCLAMATION);*/
        break;

    case WM_CLOSE:
        if (DoCloseFile()) {
            DestroyWindow(hWnd);
        }
        break;

    case WM_QUERYENDSESSION:
        if (DoCloseFile()) {
            return 1;
        }
        break;

    case WM_DESTROY:
        NOTEPAD_SaveSettingToRegistry();

        PostQuitMessage(0);
        break;

    case WM_SIZE:
        updateWindowSize(LOWORD(lParam), HIWORD(lParam));
        break;

    case WM_SETFOCUS:
        SetFocus(Globals.hEdit);
        break;

    case WM_DROPFILES:
    {
        WCHAR szFileName[MAX_PATH];
        HANDLE hDrop = (HANDLE) wParam;

        DragQueryFileW(hDrop, 0, szFileName, ARRAY_SIZE(szFileName));
        DragFinish(hDrop);
        DoOpenFile(szFileName, ENCODING_AUTO);
        break;
    }
    
    case WM_INITMENUPOPUP:
        NOTEPAD_InitMenuPopup((HMENU)wParam, lParam);
        break;

    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

static int AlertFileDoesNotExist(LPCWSTR szFileName)
{
   int nResult;
   WCHAR szMessage[MAX_STRING_LEN];
   WCHAR szResource[MAX_STRING_LEN];

   LoadStringW(Globals.hInstance, STRING_DOESNOTEXIST, szResource, ARRAY_SIZE(szResource));
   wsprintfW(szMessage, szResource, szFileName);

   LoadStringW(Globals.hInstance, STRING_ERROR, szResource, ARRAY_SIZE(szResource));

   nResult = MessageBoxW(Globals.hMainWnd, szMessage, szResource,
                         MB_ICONEXCLAMATION | MB_YESNOCANCEL);

   return(nResult);
}

static void HandleCommandLine(LPWSTR cmdline)
{
    WCHAR delimiter, *ptr;
    BOOL opt_print = FALSE;

    /* skip white space */
    while (*cmdline == ' ') cmdline++;

    /* skip executable name */
    delimiter = (*cmdline == '"' ? '"' : ' ');

    if (*cmdline == delimiter) cmdline++;

    while (*cmdline && *cmdline != delimiter) cmdline++;

    if (*cmdline == delimiter) cmdline++;

    while (*cmdline == ' ') cmdline++;

    ptr = cmdline;
    while (*ptr == ' ' || *ptr == '-' || *ptr == '/')
    {
        WCHAR option;

        if (*ptr++ == ' ') continue;

        option = *ptr;
        if (option) ptr++;
        while (*ptr == ' ') ptr++;

        switch(option)
        {
            case 'p':
            case 'P':
            {
                if (!opt_print)
                {
                    opt_print = TRUE;
                    cmdline = ptr;
                }
                break;
            }
        }
    }

    if (*cmdline)
    {
        /* file name is passed in the command line */
        LPCWSTR file_name;
        BOOL file_exists;
        WCHAR buf[MAX_PATH];

        if (cmdline[0] == '"')
        {
            WCHAR* wc;
            cmdline++;
            wc=cmdline;
            /* Note: Double-quotes are not allowed in Windows filenames */
            while (*wc && *wc != '"') wc++;
            /* On Windows notepad ignores further arguments too */
            *wc = 0;
        }

        if (FileExists(cmdline))
        {
            file_exists = TRUE;
            file_name = cmdline;
        }
        else
        {
            /* try to find file with ".txt" extension */
            if (wcschr(PathFindFileNameW(cmdline), '.'))
            {
                file_exists = FALSE;
                file_name = cmdline;
            }
            else
            {
                lstrcpynW(buf, cmdline, MAX_PATH - lstrlenW(L".txt") - 1);
                lstrcatW(buf, L".txt");
                file_name = buf;
                file_exists = FileExists(buf);
            }
        }

        if (file_exists)
        {
            DoOpenFile(file_name, ENCODING_AUTO);
            InvalidateRect(Globals.hMainWnd, NULL, FALSE);
            if (opt_print)
                DIALOG_FilePrint();
        }
        else
        {
            switch (AlertFileDoesNotExist(file_name)) {
            case IDYES:
            {

                HANDLE file;
                SetFileNameAndEncoding(file_name, ENCODING_ANSI);

                file = CreateFileW(file_name, GENERIC_WRITE, FILE_SHARE_WRITE,
                                   NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                if (file != INVALID_HANDLE_VALUE) CloseHandle(file);

                UpdateWindowCaption();
                break;
            }

            case IDNO:
                break;

            case IDCANCEL:
                DestroyWindow(Globals.hMainWnd);
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
    MSG msg;
    HACCEL hAccel;
    WNDCLASSEXW class;
    HMONITOR monitor;
    MONITORINFO info;
    INT x, y;

    InitCommonControls();

    aFINDMSGSTRING = RegisterWindowMessageW(FINDMSGSTRINGW);

    ZeroMemory(&Globals, sizeof(Globals));
    Globals.hInstance       = hInstance;
    NOTEPAD_LoadSettingFromRegistry();

    ZeroMemory(&class, sizeof(class));
    class.cbSize        = sizeof(class);
    class.lpfnWndProc   = NOTEPAD_WndProc;
    class.hInstance     = Globals.hInstance;
    class.hIcon         = LoadIconW(Globals.hInstance, MAKEINTRESOURCEW(IDI_NOTEPAD));
    class.hIconSm       = LoadImageW(Globals.hInstance, MAKEINTRESOURCEW(IDI_NOTEPAD), IMAGE_ICON,
                                     GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
                                     LR_SHARED);
    class.hCursor       = LoadCursorW(0, (LPCWSTR)IDC_ARROW);
    class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    class.lpszMenuName  = MAKEINTRESOURCEW(MAIN_MENU);
    class.lpszClassName = L"Notepad";

    if (!RegisterClassExW(&class)) return FALSE;

    /* Setup windows */

    monitor = MonitorFromRect( &main_rect, MONITOR_DEFAULTTOPRIMARY );
    info.cbSize = sizeof(info);
    GetMonitorInfoW( monitor, &info );

    x = main_rect.left;
    y = main_rect.top;
    if (main_rect.left >= info.rcWork.right ||
        main_rect.top >= info.rcWork.bottom ||
        main_rect.right < info.rcWork.left ||
        main_rect.bottom < info.rcWork.top)
        x = y = CW_USEDEFAULT;

    Globals.hMainWnd =
        CreateWindowW(L"Notepad", L"Notepad", WS_OVERLAPPEDWINDOW, x, y,
                      main_rect.right - main_rect.left, main_rect.bottom - main_rect.top,
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

    HandleCommandLine(GetCommandLineW());

    hAccel = LoadAcceleratorsW(hInstance, MAKEINTRESOURCEW(ID_ACCEL));

    while (GetMessageW(&msg, 0, 0, 0))
    {
        if (!IsDialogMessageW(Globals.hFindReplaceDlg, &msg) && !TranslateAcceleratorW(Globals.hMainWnd, hAccel, &msg))
	{
	    TranslateMessage(&msg);
            DispatchMessageW(&msg);
	}
    }
    return msg.wParam;
}
