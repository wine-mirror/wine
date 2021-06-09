/*
 * Write.exe - this program only calls wordpad.exe
 *
 * Copyright 2007 by Mikolaj Zalewski
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

#define WIN32_LEAN_AND_MEAN

#include <stdarg.h>

#include <windows.h>
#include <shlobj.h>
#include "resources.h"

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hOldInstance, LPSTR szCmdParagraph, int res)
{
    WCHAR path[MAX_PATH];
    STARTUPINFOW stinf;
    PROCESS_INFORMATION info;

    if (FAILED(SHGetFolderPathW(NULL, CSIDL_PROGRAM_FILES, NULL, 0, path)))
	goto failed;
    lstrcatW(path, L"\\Windows NT\\Accessories\\wordpad.exe");

    stinf.cb = sizeof(STARTUPINFOW);
    GetStartupInfoW(&stinf);

    if (!CreateProcessW(path, GetCommandLineW(), NULL, NULL, FALSE, 0, NULL, NULL, &stinf, &info))
	goto failed;
    CloseHandle(info.hProcess);
    CloseHandle(info.hThread);
    return 0;

failed:
    LoadStringW(GetModuleHandleW(NULL), IDS_FAILED, path, MAX_PATH);
    MessageBoxW(NULL, path, NULL, MB_OK|MB_ICONERROR);
    return 1;
}
