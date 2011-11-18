/*
 * The dialog that displays after a crash
 *
 * Copyright 2008 Mikolaj Zalewski
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

#include "debugger.h"
#include "wingdi.h"
#include "winuser.h"
#include "commctrl.h"
#include "shellapi.h"
#include "psapi.h"

#include "wine/debug.h"
#include "wine/unicode.h"

#include "resource.h"

#define MAX_PROGRAM_NAME_LENGTH 80

int msgbox_res_id(HWND hwnd, UINT textId, UINT captionId, UINT uType)
{
    WCHAR caption[256];
    WCHAR text[256];
    LoadStringW(GetModuleHandleW(NULL), captionId, caption, sizeof(caption)/sizeof(caption[0]));
    LoadStringW(GetModuleHandleW(NULL), textId, text, sizeof(text)/sizeof(text[0]));
    return MessageBoxW(hwnd, text, caption, uType);
}

static WCHAR *get_program_name(HANDLE hProcess)
{
    WCHAR image_name[MAX_PATH];
    WCHAR *programname;
    WCHAR *output;

    /* GetProcessImageFileNameW gives no way to query the correct buffer size,
     * but programs with a path longer than MAX_PATH can't be started by the
     * shell, so we expect they don't happen often */
    if (!GetProcessImageFileNameW(hProcess, image_name, MAX_PATH))
    {
        static WCHAR unidentified[MAX_PROGRAM_NAME_LENGTH];
        LoadStringW(GetModuleHandleW(NULL), IDS_UNIDENTIFIED,
                unidentified, MAX_PROGRAM_NAME_LENGTH);
        return unidentified;
    }

    programname = strrchrW(image_name, '\\');
    if (programname != NULL)
        programname++;
    else
        programname = image_name;

    /* TODO: if the image has a VERSIONINFO, we could try to find there a more
     * user-friendly program name */

    /* don't display a too long string to the user */
    if (strlenW(programname) >= MAX_PROGRAM_NAME_LENGTH)
    {
        programname[MAX_PROGRAM_NAME_LENGTH - 4] = '.';
        programname[MAX_PROGRAM_NAME_LENGTH - 3] = '.';
        programname[MAX_PROGRAM_NAME_LENGTH - 2] = '.';
        programname[MAX_PROGRAM_NAME_LENGTH - 1] = 0;
    }

    output = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR)*(lstrlenW(programname) + 1));
    lstrcpyW(output, programname);

    return output;
}

static LPWSTR g_ProgramName;
static HFONT g_hBoldFont;
static HMENU g_hDebugMenu = NULL;

static void set_bold_font(HWND hDlg)
{
    HFONT hNormalFont = (HFONT)SendDlgItemMessageW(hDlg, IDC_STATIC_TXT1,
            WM_GETFONT, 0, 0);
    LOGFONTW font;
    GetObjectW(hNormalFont, sizeof(LOGFONTW), &font);
    font.lfWeight = FW_BOLD;
    g_hBoldFont = CreateFontIndirectW(&font);
    SendDlgItemMessageW(hDlg, IDC_STATIC_TXT1, WM_SETFONT, (WPARAM)g_hBoldFont, TRUE);
}

static void set_message_with_filename(HWND hDlg)
{
    WCHAR originalText[1000];
    WCHAR newText[1000 + MAX_PROGRAM_NAME_LENGTH];

    GetDlgItemTextW(hDlg, IDC_STATIC_TXT1, originalText,
            sizeof(originalText)/sizeof(originalText[0]));
    wsprintfW(newText, originalText, g_ProgramName);
    SetDlgItemTextW(hDlg, IDC_STATIC_TXT1, newText);
}

static INT_PTR WINAPI DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static const WCHAR openW[] = {'o','p','e','n',0};
    switch (msg)
    {
    case WM_INITDIALOG:
    {
        set_bold_font(hwnd);
        set_message_with_filename(hwnd);
        return TRUE;
    }
    case WM_CTLCOLORSTATIC:
    {
        /* WM_CTLCOLOR* don't use DWLP_MSGRESULT */
        INT_PTR id = GetDlgCtrlID((HWND)lParam);
        if (id == IDC_STATIC_BG || id == IDC_STATIC_TXT1)
            return (LONG_PTR)GetSysColorBrush(COLOR_WINDOW);

        return FALSE;
    }
    case WM_RBUTTONDOWN:
    {
        POINT mousePos;
        if (!(wParam & MK_SHIFT))
            return FALSE;
        if (g_hDebugMenu == NULL)
            g_hDebugMenu = LoadMenuW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDM_DEBUG_POPUP));
        GetCursorPos(&mousePos);
        TrackPopupMenu(GetSubMenu(g_hDebugMenu, 0), TPM_RIGHTBUTTON, mousePos.x, mousePos.y,
                0, hwnd, NULL);
        return TRUE;
    }
    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code)
        {
        case NM_CLICK:
        case NM_RETURN:
            if (wParam == IDC_STATIC_TXT2)
                ShellExecuteW( NULL, openW, ((NMLINK *)lParam)->item.szUrl, NULL, NULL, SW_SHOW );
            break;
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
            case IDOK:
            case IDCANCEL:
            case ID_DEBUG:
                EndDialog(hwnd, LOWORD(wParam));
                return TRUE;
        }
        return TRUE;
    }
    return FALSE;
}

BOOL display_crash_dialog(void)
{
    static const WCHAR winedeviceW[] = {'w','i','n','e','d','e','v','i','c','e','.','e','x','e',0};
    static const INITCOMMONCONTROLSEX init = { sizeof(init), ICC_LINK_CLASS };

    INT_PTR result;
    /* dbg_curr_process->handle is not set */
    HANDLE hProcess;

    if (!DBG_IVAR(ShowCrashDialog))
        return TRUE;

    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dbg_curr_pid);
    g_ProgramName = get_program_name(hProcess);
    CloseHandle(hProcess);
    if (!strcmpW( g_ProgramName, winedeviceW )) return TRUE;
    InitCommonControlsEx( &init );
    result = DialogBoxW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDD_CRASH_DLG), NULL, DlgProc);
    if (result == ID_DEBUG) {
        AllocConsole();
        return FALSE;
    }
    return TRUE;
}
