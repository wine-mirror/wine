/*
 * Copyright (C) 2007 Francois Gouget
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
#ifndef __WINE_WFEXT_H
#define __WINE_WFEXT_H

#ifdef __cplusplus
extern "C" {
#endif

#define MENU_TEXT_LEN         40

#define FMMENU_FIRST          1
#define FMMENU_LAST           99

#define FMEVENT_LOAD          100
#define FMEVENT_UNLOAD        101
#define FMEVENT_INITMENU      102
#define FMEVENT_USER_REFRESH  103
#define FMEVENT_SELCHANGE     104
#define FMEVENT_TOOLBARLOAD   105
#define FMEVENT_HELPSTRING    106
#define FMEVENT_HELPMENUITEM  107

#define FMFOCUS_DIR           1
#define FMFOCUS_TREE          2
#define FMFOCUS_DRIVES        3
#define FMFOCUS_SEARCH        4

#define FM_GETFOCUS           (WM_USER + 0x0200)
#define FM_GETDRIVEINFOA      (WM_USER + 0x0201)
#define FM_GETSELCOUNT        (WM_USER + 0x0202)
#define FM_GETSELCOUNTLFN     (WM_USER + 0x0203)
#define FM_GETFILESELA        (WM_USER + 0x0204)
#define FM_GETFILESELLFNA     (WM_USER + 0x0205)
#define FM_REFRESH_WINDOWS    (WM_USER + 0x0206)
#define FM_RELOAD_EXTENSIONS  (WM_USER + 0x0207)
#define FM_GETDRIVEINFOW      (WM_USER + 0x0211)
#define FM_GETFILESELW        (WM_USER + 0x0214)
#define FM_GETFILESELLFNW     (WM_USER + 0x0215)

#define FM_GETDRIVEINFO       WINELIB_NAME_AW(FM_GETDRIVEINFO)
#define FM_GETFILESEL         WINELIB_NAME_AW(FM_GETFILESEL)
#define FM_GETFILESELLFN      WINELIB_NAME_AW(FM_GETFILESELLFN)

LONG WINAPI FMExtensionProc(HWND,WORD,LONG);
LONG WINAPI FMExtensionProcW(HWND,WORD,LONG);

#ifdef __cplusplus
}
#endif

#endif
