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
#include "commdlg.h"
#include "shellapi.h"
#include "psapi.h"

#include "resource.h"

#define MAX_PROGRAM_NAME_LENGTH 80

static char *crash_log;

int msgbox_res_id(HWND hwnd, UINT textid, UINT captionid, UINT type)
{
    if (DBG_IVAR(ShowCrashDialog))
    {
        WCHAR caption[256];
        WCHAR text[256];
        LoadStringW(GetModuleHandleW(NULL), captionid, caption, ARRAY_SIZE(caption));
        LoadStringW(GetModuleHandleW(NULL), textid, text, ARRAY_SIZE(text));
        return MessageBoxW(hwnd, text, caption, type);
    }

    return IDCANCEL;
}

static BOOL is_visible(void)
{
    USEROBJECTFLAGS flags;
    HWINSTA winstation;

    if (!(winstation = GetProcessWindowStation()))
        return FALSE;

    if (!(GetUserObjectInformationA(winstation, UOI_FLAGS, &flags, sizeof(flags), NULL)))
        return FALSE;

    return flags.dwFlags & WSF_VISIBLE;
}

static WCHAR *get_program_name(HANDLE hProcess)
{
    WCHAR image_name[MAX_PATH];
    WCHAR *programname;

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

    programname = wcsrchr(image_name, '\\');
    if (programname != NULL)
        programname++;
    else
        programname = image_name;

    /* TODO: if the image has a VERSIONINFO, we could try to find there a more
     * user-friendly program name */

    /* don't display a too long string to the user */
    if (lstrlenW(programname) >= MAX_PROGRAM_NAME_LENGTH)
    {
        programname[MAX_PROGRAM_NAME_LENGTH - 4] = '.';
        programname[MAX_PROGRAM_NAME_LENGTH - 3] = '.';
        programname[MAX_PROGRAM_NAME_LENGTH - 2] = '.';
        programname[MAX_PROGRAM_NAME_LENGTH - 1] = 0;
    }

    return wcsdup(programname);
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

static void set_fixed_font( HWND dlg, UINT id )
{
    HFONT hfont = (HFONT)SendDlgItemMessageW( dlg, id, WM_GETFONT, 0, 0);
    LOGFONTW font;

    GetObjectW(hfont, sizeof(LOGFONTW), &font);
    font.lfPitchAndFamily = FIXED_PITCH;
    font.lfFaceName[0] = 0;
    hfont = CreateFontIndirectW(&font);
    SendDlgItemMessageW( dlg, id, WM_SETFONT, (WPARAM)hfont, TRUE );
}

static void set_message_with_filename(HWND hDlg)
{
    WCHAR originalText[1000];
    WCHAR newText[1000 + MAX_PROGRAM_NAME_LENGTH];

    GetDlgItemTextW(hDlg, IDC_STATIC_TXT1, originalText, ARRAY_SIZE(originalText));
    wsprintfW(newText, originalText, g_ProgramName);
    SetDlgItemTextW(hDlg, IDC_STATIC_TXT1, newText);
}

static void load_crash_log( HANDLE file )
{
    DWORD len, pos = 0, size = 65536;

    crash_log = malloc( size );
    SetFilePointer( file, 0, NULL, FILE_BEGIN );
    while (ReadFile( file, crash_log + pos, size - pos - 1, &len, NULL ) && len)
    {
        pos += len;
        if (pos == size - 1) crash_log = realloc( crash_log, size *= 2 );
    }
    crash_log[pos] = 0;
}

static void save_crash_log( HWND hwnd )
{
    OPENFILENAMEW save;
    HANDLE handle;
    DWORD err, written;
    WCHAR *p, path[MAX_PATH], buffer[1024];

    memset( &save, 0, sizeof(save) );
    lstrcpyW( path, L"backtrace.txt" );

    LoadStringW( GetModuleHandleW(0), IDS_TEXT_FILES, buffer, ARRAY_SIZE(buffer));
    p = buffer + lstrlenW(buffer) + 1;
    lstrcpyW(p, L"*.txt");
    p += lstrlenW(p) + 1;
    LoadStringW( GetModuleHandleW(0), IDS_ALL_FILES, p, ARRAY_SIZE(buffer) - (p - buffer) );
    p += lstrlenW(p) + 1;
    lstrcpyW(p, L"*.*");
    p += lstrlenW(p) + 1;
    *p = '\0';

    save.lStructSize = sizeof(OPENFILENAMEW);
    save.hwndOwner   = hwnd;
    save.hInstance   = GetModuleHandleW(0);
    save.lpstrFilter = buffer;
    save.lpstrFile   = path;
    save.nMaxFile    = MAX_PATH;
    save.Flags       = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT |
                       OFN_HIDEREADONLY | OFN_ENABLESIZING;
    save.lpstrDefExt = L"txt";

    if (!GetSaveFileNameW( &save )) return;
    handle = CreateFileW( save.lpstrFile, GENERIC_WRITE, FILE_SHARE_READ,
                        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0 );
    if (handle != INVALID_HANDLE_VALUE)
    {
        if (!WriteFile( handle, crash_log, strlen(crash_log), &written, NULL ))
            err = GetLastError();
        else if (written != strlen(crash_log))
            err = GetLastError();
        else
        {
            CloseHandle( handle );
            return;
        }
        CloseHandle( handle );
        DeleteFileW( save.lpstrFile );
    }
    else err = GetLastError();

    LoadStringW( GetModuleHandleW(0), IDS_SAVE_ERROR, buffer, ARRAY_SIZE(buffer));
    FormatMessageW( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL, err, 0, (LPWSTR)&p, 0, NULL);
    MessageBoxW( 0, p, buffer, MB_OK | MB_ICONERROR);
    LocalFree( p );
}

static INT_PTR WINAPI crash_dlg_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
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
                ShellExecuteW( NULL, L"open", ((NMLINK *)lParam)->item.szUrl, NULL, NULL, SW_SHOW );
            break;
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
            case IDOK:
            case IDCANCEL:
            case ID_DEBUG:
            case ID_DETAILS:
                EndDialog(hwnd, LOWORD(wParam));
                return TRUE;
        }
        return TRUE;
    }
    return FALSE;
}

static INT_PTR WINAPI details_dlg_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    static POINT orig_size, min_size, edit_size, text_pos, save_pos, close_pos;

    switch (msg)
    {
    case WM_INITDIALOG:
    {
        RECT rect;
        WCHAR buffer[256];

        set_fixed_font( hwnd, IDC_CRASH_TXT );
        LoadStringW( GetModuleHandleW(0), IDS_LOADING, buffer, 256 );
        SetDlgItemTextW( hwnd, IDC_CRASH_TXT, buffer );
        EnableWindow( GetDlgItem( hwnd, IDC_CRASH_TXT ), FALSE );
        EnableWindow( GetDlgItem( hwnd, ID_SAVEAS ), FALSE );

        GetClientRect( hwnd, &rect );
        orig_size.x = rect.right;
        orig_size.y = rect.bottom;

        GetWindowRect( hwnd, &rect );
        min_size.x = rect.right - rect.left;
        min_size.y = rect.bottom - rect.top;

        GetWindowRect( GetDlgItem( hwnd, IDOK ), &rect );
        MapWindowPoints( 0, hwnd, (POINT *)&rect, 2 );
        close_pos.x = rect.left;
        close_pos.y = rect.top;

        GetWindowRect( GetDlgItem( hwnd, ID_SAVEAS ), &rect );
        MapWindowPoints( 0, hwnd, (POINT *)&rect, 2 );
        save_pos.x = rect.left;
        save_pos.y = rect.top;

        GetWindowRect( GetDlgItem( hwnd, IDC_STATIC_TXT2 ), &rect );
        MapWindowPoints( 0, hwnd, (POINT *)&rect, 2 );
        text_pos.x = rect.left;
        text_pos.y = rect.top;

        GetWindowRect( GetDlgItem( hwnd, IDC_CRASH_TXT ), &rect );
        MapWindowPoints( 0, hwnd, (POINT *)&rect, 2 );
        edit_size.x = rect.right - rect.left;
        edit_size.y = rect.bottom - rect.top;
        return TRUE;
    }

    case WM_GETMINMAXINFO:
        ((MINMAXINFO *)lparam)->ptMinTrackSize = min_size;
        return TRUE;

    case WM_SIZE:
        if (wparam == SIZE_RESTORED || wparam == SIZE_MAXIMIZED)
        {
            int off_x = (short)LOWORD( lparam ) - orig_size.x;
            int off_y = (short)HIWORD( lparam ) - orig_size.y;

            SetWindowPos( GetDlgItem( hwnd, IDOK ), 0, close_pos.x + off_x,
                          close_pos.y + off_y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
            SetWindowPos( GetDlgItem( hwnd, ID_SAVEAS ), 0, save_pos.x + off_x,
                          save_pos.y + off_y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
            SetWindowPos( GetDlgItem( hwnd, IDC_STATIC_TXT2 ), 0, text_pos.x,
                          text_pos.y + off_y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
            SetWindowPos( GetDlgItem( hwnd, IDC_CRASH_TXT ), 0, 0, 0, edit_size.x + off_x,
                          edit_size.y + off_y, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );
        }
        return TRUE;

    case WM_NOTIFY:
        switch (((NMHDR *)lparam)->code)
        {
        case NM_CLICK:
        case NM_RETURN:
            if (wparam == IDC_STATIC_TXT2)
                ShellExecuteW( NULL, L"open", ((NMLINK *)lparam)->item.szUrl, NULL, NULL, SW_SHOW );
            break;
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case ID_SAVEAS:
            save_crash_log( hwnd );
            break;
        case IDOK:
        case IDCANCEL:
            PostQuitMessage( 0 );
            break;
        }
        return TRUE;
    }
    return FALSE;
}

int display_crash_dialog(void)
{
    static const INITCOMMONCONTROLSEX init = { sizeof(init), ICC_LINK_CLASS };

    /* dbg_curr_process->handle is not set */
    HANDLE hProcess;

    if (!DBG_IVAR(ShowCrashDialog) || !is_visible())
        return TRUE;

    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dbg_curr_pid);
    g_ProgramName = get_program_name(hProcess);
    CloseHandle(hProcess);
    if (!wcscmp( g_ProgramName, L"winedevice.exe" )) return TRUE;
    InitCommonControlsEx( &init );
    return DialogBoxW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDD_CRASH_DLG), NULL, crash_dlg_proc);
}

static DWORD WINAPI crash_details_thread( void *event )
{
    MSG msg;
    HWND dialog;

    dialog = CreateDialogW( GetModuleHandleW(0), MAKEINTRESOURCEW(IDD_DETAILS_DLG), 0, details_dlg_proc );
    if (!dialog) return 1;

    for (;;)
    {
        if (MsgWaitForMultipleObjects( 1, &event, FALSE, INFINITE, QS_ALLINPUT ) == 0)
        {
            load_crash_log( dbg_houtput );
            SetDlgItemTextA( dialog, IDC_CRASH_TXT, crash_log );
            EnableWindow( GetDlgItem( dialog, IDC_CRASH_TXT ), TRUE );
            EnableWindow( GetDlgItem( dialog, ID_SAVEAS ), TRUE );
            break;
        }
        while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE ))
        {
            if (msg.message == WM_QUIT) return 0;
            TranslateMessage( &msg );
            DispatchMessageW( &msg );
        }
    }

    while (GetMessageW( &msg, 0, 0, 0 ))
    {
        TranslateMessage( &msg );
        DispatchMessageW( &msg );
    }
    return 0;
}

HANDLE display_crash_details( HANDLE event )
{
    return CreateThread( NULL, 0, crash_details_thread, event, 0, NULL );
}
