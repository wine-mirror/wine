/*
 * Copyright (C) 2006 Mike McCormack
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

#ifndef __WINE_PROFINFO_H
#define __WINE_PROFINFO_H

typedef struct _PROFILEINFOA {
    DWORD dwSize;
    DWORD dwFlags;
    LPSTR lpUserName;
    LPSTR lpProfilePath;
    LPSTR lpDefaultPath;
    LPSTR lpServerName;
    LPSTR lpPolicyPath;
    HANDLE hProfile;
} PROFILEINFOA, *LPPROFILEINFOA;

typedef struct _PROFILEINFOW {
    DWORD dwSize;
    DWORD dwFlags;
    LPWSTR lpUserName;
    LPWSTR lpProfilePath;
    LPWSTR lpDefaultPath;
    LPWSTR lpServerName;
    LPWSTR lpPolicyPath;
    HANDLE hProfile;
} PROFILEINFOW, *LPPROFILEINFOW;

DECL_WINELIB_TYPE_AW(PROFILEINFO)
DECL_WINELIB_TYPE_AW(LPPROFILEINFO)

#endif /* __WINE_PROFINFO_H */
