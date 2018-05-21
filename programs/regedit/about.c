/*
 * Regedit About Dialog Box
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
#include <shellapi.h>

#include "main.h"

void ShowAboutBox(HWND hWnd)
{
    WCHAR title[64];
    HICON icon = LoadImageW( hInst, MAKEINTRESOURCEW(IDI_REGEDIT), IMAGE_ICON, 48, 48, LR_SHARED );
    LoadStringW(hInst, IDS_APP_TITLE, title, ARRAY_SIZE(title));
    ShellAboutW(hWnd, title, NULL, icon);
}
