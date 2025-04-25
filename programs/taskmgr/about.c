/*
 *  ReactOS Task Manager
 *
 *  about.c
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
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

#include <stdio.h>
#include <stdlib.h>

#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>

#include "taskmgr.h"


void OnAbout(void)
{
            WCHAR appname[256];
            LoadStringW( hInst, IDC_TASKMGR, appname, ARRAY_SIZE( appname ));
            ShellAboutW( hMainWnd, appname, L"Brian Palmer <brianp@reactos.org>",
                         LoadImageA( hInst, (LPSTR)IDI_TASKMANAGER, IMAGE_ICON, 48, 48, LR_SHARED ));
}
