/*
 * Wordpad implementation - Registry functions
 *
 * Copyright 2007 by Alexander N. Sørnes <alex@thehandofagony.com>
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

#include <windows.h>
#include <shlobj.h>
#include <richedit.h>

#include "wordpad.h"

static LRESULT registry_get_handle(HKEY *hKey, LPDWORD action, LPCWSTR subKey)
{
    LONG ret;
    LPWSTR key = calloc(lstrlenW(L"Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Wordpad\\")+lstrlenW(subKey)+1, sizeof(WCHAR));

    if(!key) return 1;

    lstrcpyW(key, L"Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Wordpad\\");
    lstrcatW(key, subKey);

    if(action)
    {
        ret = RegCreateKeyExW(HKEY_CURRENT_USER, key, 0, NULL, REG_OPTION_NON_VOLATILE,
                              KEY_READ | KEY_WRITE, NULL, hKey, action);
    } else
    {
        ret = RegOpenKeyExW(HKEY_CURRENT_USER, key, 0, KEY_READ | KEY_WRITE, hKey);
    }

    free(key);
    return ret;
}

void registry_set_options(HWND hMainWnd)
{
    HKEY hKey = 0;
    DWORD action;

    if(registry_get_handle(&hKey, &action, L"Options") == ERROR_SUCCESS)
    {
        WINDOWPLACEMENT wp;
        DWORD isMaximized;

        wp.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(hMainWnd, &wp);
        isMaximized = (wp.showCmd == SW_SHOWMAXIMIZED);

        RegSetValueExW(hKey, L"FrameRect", 0, REG_BINARY, (LPBYTE)&wp.rcNormalPosition, sizeof(RECT));
        RegSetValueExW(hKey, L"Maximized", 0, REG_DWORD, (LPBYTE)&isMaximized, sizeof(DWORD));

        registry_set_pagemargins(hKey);
        RegCloseKey(hKey);
    }

    if(registry_get_handle(&hKey, &action, L"Settings") == ERROR_SUCCESS)
    {
        registry_set_previewpages(hKey);
        RegCloseKey(hKey);
    }
}

void registry_read_winrect(RECT* rc)
{
    HKEY hKey = 0;
    DWORD size = sizeof(RECT);

    if(registry_get_handle(&hKey, 0, L"Options") != ERROR_SUCCESS ||
       RegQueryValueExW(hKey, L"FrameRect", 0, NULL, (LPBYTE)rc, &size) !=
       ERROR_SUCCESS || size != sizeof(RECT))
        SetRect(rc, 0, 0, 600, 300);

    RegCloseKey(hKey);
}

void registry_read_maximized(DWORD *bMaximized)
{
    HKEY hKey = 0;
    DWORD size = sizeof(DWORD);

    if(registry_get_handle(&hKey, 0, L"Options") != ERROR_SUCCESS ||
       RegQueryValueExW(hKey, L"Maximized", 0, NULL, (LPBYTE)bMaximized, &size) !=
       ERROR_SUCCESS || size != sizeof(DWORD))
    {
        *bMaximized = FALSE;
    }

    RegCloseKey(hKey);
}

static void truncate_path(LPWSTR file, LPWSTR out, LPWSTR pos1, LPWSTR pos2)
{
    *++pos1 = 0;

    lstrcatW(out, file);
    lstrcatW(out, L"...");
    lstrcatW(out, pos2);
}

static void format_filelist_filename(LPWSTR file, LPWSTR out)
{
    LPWSTR pos_basename;
    LPWSTR truncpos1, truncpos2;
    WCHAR myDocs[MAX_PATH];

    SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, myDocs);
    pos_basename = file_basename(file);
    truncpos1 = NULL;
    truncpos2 = NULL;

    *(pos_basename-1) = 0;
    if(!lstrcmpiW(file, myDocs) || (lstrlenW(pos_basename) > FILELIST_ENTRY_LENGTH))
    {
        truncpos1 = pos_basename;
        *(pos_basename-1) = '\\';
    } else
    {
        LPWSTR pos;
        BOOL morespace = FALSE;

        *(pos_basename-1) = '\\';

        for(pos = file; pos < pos_basename; pos++)
        {
            if(*pos == '\\' || *pos == '/')
            {
                if(truncpos1)
                {
                    if((pos - file + lstrlenW(pos_basename)) > FILELIST_ENTRY_LENGTH)
                        break;

                    truncpos1 = pos;
                    morespace = TRUE;
                    break;
                }

                if((pos - file + lstrlenW(pos_basename)) > FILELIST_ENTRY_LENGTH)
                    break;

                truncpos1 = pos;
            }
        }

        if(morespace)
        {
            for(pos = pos_basename; pos >= truncpos1; pos--)
            {
                if(*pos == '\\' || *pos == '/')
                {
                    if((truncpos1 - file + lstrlenW(pos_basename) + pos_basename - pos) > FILELIST_ENTRY_LENGTH)
                        break;

                    truncpos2 = pos;
                }
            }
        }
    }

    if(truncpos1 == pos_basename)
        lstrcatW(out, pos_basename);
    else if(truncpos1 == truncpos2 || !truncpos2)
        lstrcatW(out, file);
    else
        truncate_path(file, out, truncpos1, truncpos2);
}

void registry_read_filelist(HWND hMainWnd)
{
    HKEY hFileKey;

    if(registry_get_handle(&hFileKey, 0, L"Recent file list") == ERROR_SUCCESS)
    {
        WCHAR itemText[MAX_PATH+3], buffer[MAX_PATH];
        /* The menu item name is not the same as the file name, so we need to store
        the file name here */
        static WCHAR file1[MAX_PATH], file2[MAX_PATH], file3[MAX_PATH], file4[MAX_PATH];
        LPWSTR pFile[] = {file1, file2, file3, file4};
        DWORD pathSize = MAX_PATH*sizeof(WCHAR);
        int i;
        WCHAR key[6];
        MENUITEMINFOW mi;
        HMENU hMenu = GetMenu(hMainWnd);

        mi.cbSize = sizeof(MENUITEMINFOW);
        mi.fMask = MIIM_ID | MIIM_DATA | MIIM_STRING | MIIM_FTYPE;
        mi.fType = MFT_STRING;
        mi.dwTypeData = itemText;
        mi.wID = ID_FILE_RECENT1;

        RemoveMenu(hMenu, ID_FILE_RECENT_SEPARATOR, MF_BYCOMMAND);
        for(i = 0; i < FILELIST_ENTRIES; i++)
        {
            wsprintfW(key, L"File%d", i+1);
            RemoveMenu(hMenu, ID_FILE_RECENT1+i, MF_BYCOMMAND);
            if(RegQueryValueExW(hFileKey, (LPWSTR)key, 0, NULL, (LPBYTE)pFile[i], &pathSize)
               != ERROR_SUCCESS)
                break;

            mi.dwItemData = (ULONG_PTR)pFile[i];
            wsprintfW(itemText, L"&%d ", i+1);

            lstrcpyW(buffer, pFile[i]);

            format_filelist_filename(buffer, itemText);

            InsertMenuItemW(hMenu, ID_FILE_EXIT, FALSE, &mi);
            mi.wID++;
            pathSize = MAX_PATH*sizeof(WCHAR);
        }
        mi.fType = MFT_SEPARATOR;
        mi.fMask = MIIM_FTYPE | MIIM_ID;
        InsertMenuItemW(hMenu, ID_FILE_EXIT, FALSE, &mi);

        RegCloseKey(hFileKey);
    }
}

void registry_set_filelist(LPCWSTR newFile, HWND hMainWnd)
{
    HKEY hKey;
    DWORD action;

    if(registry_get_handle(&hKey, &action, L"Recent file list") == ERROR_SUCCESS)
    {
        LPCWSTR pFiles[FILELIST_ENTRIES];
        int i;
        HMENU hMenu = GetMenu(hMainWnd);
        MENUITEMINFOW mi;
        WCHAR buffer[6];

        mi.cbSize = sizeof(MENUITEMINFOW);
        mi.fMask = MIIM_DATA;

        for(i = 0; i < FILELIST_ENTRIES; i++)
            pFiles[i] = NULL;

        for(i = 0; i < FILELIST_ENTRIES && GetMenuItemInfoW(hMenu, ID_FILE_RECENT1+i, FALSE, &mi); i++)
            pFiles[i] = (LPWSTR)mi.dwItemData;

        if(lstrcmpiW(newFile, pFiles[0]))
        {
            for(i = 0; i < FILELIST_ENTRIES && pFiles[i]; i++)
            {
                if(!lstrcmpiW(pFiles[i], newFile))
                {
                    int j;
                    for(j = 0; j < i; j++)
                    {
                        pFiles[i-j] = pFiles[i-j-1];
                    }
                    pFiles[0] = NULL;
                    break;
                }
            }

            if(!pFiles[0])
            {
                pFiles[0] = newFile;
            } else
            {
                for(i = 0; i < FILELIST_ENTRIES-1; i++)
                    pFiles[FILELIST_ENTRIES-1-i] = pFiles[FILELIST_ENTRIES-2-i];

                pFiles[0] = newFile;
            }

            for(i = 0; i < FILELIST_ENTRIES && pFiles[i]; i++)
            {
                wsprintfW(buffer, L"File%d", i+1);
                RegSetValueExW(hKey, (LPWSTR)&buffer, 0, REG_SZ, (const BYTE*)pFiles[i],
                               (lstrlenW(pFiles[i])+1)*sizeof(WCHAR));
            }
        }
        RegCloseKey(hKey);
    }
    registry_read_filelist(hMainWnd);
}

int reg_formatindex(WPARAM format)
{
    return (format & SF_TEXT) ? 1 : 0;
}

void registry_read_options(void)
{
    HKEY hKey;

    if(registry_get_handle(&hKey, 0, L"Options") != ERROR_SUCCESS)
        registry_read_pagemargins(NULL);
    else
    {
        registry_read_pagemargins(hKey);
        RegCloseKey(hKey);
    }

    if(registry_get_handle(&hKey, 0, L"Settings") != ERROR_SUCCESS) {
        registry_read_previewpages(NULL);
    } else {
        registry_read_previewpages(hKey);
        RegCloseKey(hKey);
    }
}

static void registry_read_formatopts(int index, LPCWSTR key, DWORD barState[], DWORD wordWrap[])
{
    HKEY hKey;
    DWORD action = 0;
    BOOL fetched = FALSE;
    barState[index] = 0;
    wordWrap[index] = 0;

    if(registry_get_handle(&hKey, &action, key) != ERROR_SUCCESS)
        return;

    if(action == REG_OPENED_EXISTING_KEY)
    {
        DWORD size = sizeof(DWORD);

        if(RegQueryValueExW(hKey, L"BarState0", 0, NULL, (LPBYTE)&barState[index],
           &size) == ERROR_SUCCESS)
            fetched = TRUE;
    }

    if(!fetched)
        barState[index] = (1 << BANDID_TOOLBAR) | (1 << BANDID_FORMATBAR) | (1 << BANDID_RULER) | (1 << BANDID_STATUSBAR);

    fetched = FALSE;
    if(action == REG_OPENED_EXISTING_KEY)
    {
        DWORD size = sizeof(DWORD);
        if(RegQueryValueExW(hKey, L"Wrap", 0, NULL, (LPBYTE)&wordWrap[index],
           &size) == ERROR_SUCCESS)
            fetched = TRUE;
    }

    if (!fetched)
    {
        if(index == reg_formatindex(SF_RTF))
            wordWrap[index] = ID_WORDWRAP_WINDOW;
        else if(index == reg_formatindex(SF_TEXT))
            wordWrap[index] = ID_WORDWRAP_NONE;
    }

    RegCloseKey(hKey);
}

void registry_read_formatopts_all(DWORD barState[], DWORD wordWrap[])
{
    registry_read_formatopts(reg_formatindex(SF_RTF), L"RTF", barState, wordWrap);
    registry_read_formatopts(reg_formatindex(SF_TEXT), L"Text", barState, wordWrap);
}

static void registry_set_formatopts(int index, LPCWSTR key, DWORD barState[], DWORD wordWrap[])
{
    HKEY hKey;
    DWORD action = 0;

    if(registry_get_handle(&hKey, &action, key) == ERROR_SUCCESS)
    {
        RegSetValueExW(hKey, L"BarState0", 0, REG_DWORD, (LPBYTE)&barState[index],
                       sizeof(DWORD));
        RegSetValueExW(hKey, L"Wrap", 0, REG_DWORD, (LPBYTE)&wordWrap[index],
                       sizeof(DWORD));
        RegCloseKey(hKey);
    }
}

void registry_set_formatopts_all(DWORD barState[], DWORD wordWrap[])
{
    registry_set_formatopts(reg_formatindex(SF_RTF), L"RTF", barState, wordWrap);
    registry_set_formatopts(reg_formatindex(SF_TEXT), L"Text", barState, wordWrap);
}
