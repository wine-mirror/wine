/*
 * SetupAPI dialog functions
 *
 * Copyright 2009 Ricardo Filipe
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

#include <stdarg.h>

#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "commdlg.h"
#include "setupapi.h"
#include "winnls.h"
#include "setupapi_private.h"

#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

struct promptdisk_params {
    PCWSTR DialogTitle;
    PCWSTR DiskName;
    PCWSTR PathToSource;
    PCWSTR FileSought;
    PCWSTR TagFile;
    DWORD DiskPromptStyle;
    PWSTR PathBuffer;
    DWORD PathBufferSize;
    PDWORD PathRequiredSize;
};

/* initiates the fields of the SetupPromptForDisk dialog according to the parameters
*/
static void promptdisk_init(HWND hwnd, struct promptdisk_params *params)
{
    WCHAR format[256];
    WCHAR message[256];

    SetWindowLongPtrW(hwnd, DWLP_USER, (LONG_PTR)params);

    if(params->DialogTitle)
        SetWindowTextW(hwnd, params->DialogTitle);
    if(params->PathToSource)
        SetDlgItemTextW(hwnd, IDC_PATH, params->PathToSource);

    if(!(params->DiskPromptStyle & IDF_OEMDISK))
    {
        LoadStringW(SETUPAPI_hInstance, IDS_PROMPTDISK, format,
            sizeof(format)/sizeof(format[0]));

        if(params->DiskName)
            snprintfW(message, sizeof(message)/sizeof(message[0]), format,
                params->FileSought, params->DiskName);
        else
        {
            WCHAR unknown[256];
            LoadStringW(SETUPAPI_hInstance, IDS_UNKNOWN, unknown,
                sizeof(unknown)/sizeof(unknown[0]));
            snprintfW(message, sizeof(message)/sizeof(message[0]), format,
                params->FileSought, unknown);
        }
        SetDlgItemTextW(hwnd, IDC_FILENEEDED, message);

        LoadStringW(SETUPAPI_hInstance, IDS_INFO, message,
            sizeof(message)/sizeof(message[0]));
        SetDlgItemTextW(hwnd, IDC_INFO, message);
        LoadStringW(SETUPAPI_hInstance, IDS_COPYFROM, message,
            sizeof(message)/sizeof(message[0]));
        SetDlgItemTextW(hwnd, IDC_COPYFROM, message);
    }
    if(params->DiskPromptStyle & IDF_NOBROWSE)
        ShowWindow(GetDlgItem(hwnd, IDC_RUNDLG_BROWSE), SW_HIDE);
}

/* When the user clicks the browse button in SetupPromptForDisk dialog
 * it copies the path of the selected file to the dialog path field
 */
static void promptdisk_browse(HWND hwnd, struct promptdisk_params *params)
{
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.hwndOwner = hwnd;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFile = HeapAlloc(GetProcessHeap(), 0, MAX_PATH*sizeof(WCHAR));
    strcpyW(ofn.lpstrFile, params->FileSought);

    if(GetOpenFileNameW(&ofn))
    {
        WCHAR* last_slash = strrchrW(ofn.lpstrFile, '\\');
        if (last_slash) *last_slash = 0;
        SetDlgItemTextW(hwnd, IDC_PATH, ofn.lpstrFile);
    }
    HeapFree(GetProcessHeap(), 0, ofn.lpstrFile);
}

/* Handles the messages sent to the SetupPromptForDisk dialog
*/
static INT_PTR CALLBACK promptdisk_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_INITDIALOG:
            promptdisk_init(hwnd, (struct promptdisk_params *)lParam);
            return TRUE;
        case WM_COMMAND:
            switch(wParam)
            {
                case IDCANCEL:
                    EndDialog(hwnd, DPROMPT_CANCEL);
                    return TRUE;
                case IDC_RUNDLG_BROWSE:
                {
                    struct promptdisk_params *params =
                        (struct promptdisk_params *)GetWindowLongPtrW(hwnd, DWLP_USER);
                    promptdisk_browse(hwnd, params);
                    return TRUE;
                }
            }
    }
    return FALSE;
}

/***********************************************************************
 *      SetupPromptForDiskW (SETUPAPI.@)
 */
UINT WINAPI SetupPromptForDiskW(HWND hwndParent, PCWSTR DialogTitle, PCWSTR DiskName,
        PCWSTR PathToSource, PCWSTR FileSought, PCWSTR TagFile, DWORD DiskPromptStyle,
        PWSTR PathBuffer, DWORD PathBufferSize, PDWORD PathRequiredSize)
{
    struct promptdisk_params params;
    UINT ret;

    TRACE("%p, %s, %s, %s, %s, %s, 0x%08x, %p, %d, %p\n", hwndParent, debugstr_w(DialogTitle),
          debugstr_w(DiskName), debugstr_w(PathToSource), debugstr_w(FileSought),
          debugstr_w(TagFile), DiskPromptStyle, PathBuffer, PathBufferSize,
          PathRequiredSize);

    if(!FileSought)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return DPROMPT_CANCEL;
    }
    params.DialogTitle = DialogTitle;
    params.DiskName = DiskName;
    params.PathToSource = PathToSource;
    params.FileSought = FileSought;
    params.TagFile = TagFile;
    params.DiskPromptStyle = DiskPromptStyle;
    params.PathBuffer = PathBuffer;
    params.PathBufferSize = PathBufferSize;
    params.PathRequiredSize = PathRequiredSize;

    ret = DialogBoxParamW(SETUPAPI_hInstance, MAKEINTRESOURCEW(IDPROMPTFORDISK),
        hwndParent, promptdisk_proc, (LPARAM)&params);

    if(ret == DPROMPT_CANCEL)
        SetLastError(ERROR_CANCELLED);
    return ret;
}
