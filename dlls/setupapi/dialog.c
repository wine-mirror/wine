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

/* Handles the messages sent to the SetupPromptForDisk dialog
*/
static INT_PTR CALLBACK promptdisk_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_COMMAND:
            switch(wParam)
            {
                case IDCANCEL:
                    EndDialog(hwnd, DPROMPT_CANCEL);
                    return TRUE;
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
