/*
 * Invoke the CrossOver menu management scripts.
 *
 * Copyright 2012 Francois Gouget
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

#define COBJMACROS

#include <windows.h>
#include <winternl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <errno.h>
#include "wine/debug.h"
#include "cxmenu.h"


WINE_DEFAULT_DEBUG_CHANNEL(menubuilder);


int cx_mode = 1;
int cx_dump_menus = 0;

static BOOL cx_link_is_64_bit(IShellLinkW *sl, int recurse_level);


/*
 * Functions to invoke the CrossOver menu management scripts.
 */

static int cx_wineshelllink(LPCWSTR linkW, int is_desktop, LPCWSTR rootW,
                            LPCWSTR pathW, LPCWSTR argsW,
                            LPCWSTR icon_name, LPCWSTR description, LPCWSTR arch)
{
    const char *argv[20];
    int pos = 0;
    int retcode;

    argv[pos++] = "wineshelllink";
    argv[pos++] = "--utf8";
    argv[pos++] = "--root";
    argv[pos++] = wchars_to_utf8_chars(rootW);
    argv[pos++] = "--link";
    argv[pos++] = wchars_to_utf8_chars(linkW);
    argv[pos++] = "--path";
    argv[pos++] = wchars_to_utf8_chars(pathW);
    argv[pos++] = is_desktop ? "--desktop" : "--menu";
    if (argsW && *argsW)
    {
        argv[pos++] = "--args";
        argv[pos++] = wchars_to_utf8_chars(argsW);
    }
    if (icon_name)
    {
        argv[pos++] = "--icon";
        argv[pos++] = wchars_to_utf8_chars(icon_name);
    }
    if (description && *description)
    {
        argv[pos++] = "--descr";
        argv[pos++] = wchars_to_utf8_chars(description);
    }
    if (arch && *arch)
    {
        argv[pos++] = "--arch";
        argv[pos++] = wchars_to_utf8_chars(arch);
    }
    argv[pos] = NULL;

    retcode = __wine_unix_spawnvp((char **)argv, TRUE);
    if (retcode!=0)
        WINE_ERR("%s returned %d\n", argv[0], retcode);
    return retcode;
}

static WCHAR* cx_escape_string(const WCHAR* src)
{
    const WCHAR* s;
    WCHAR *dst, *d;
    DWORD len;

    len=1;
    for (s=src; *s; s++)
    {
        switch (*s)
        {
        case '\"':
        case '\\':
            len+=2;
            break;
        default:
            len+=1;
        }
    }

    dst=d=HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    for (s=src; *s; s++)
    {
        switch (*s)
        {
        case '\"':
            *d='\\';
            d++;
            *d='\"';
            d++;
            break;
        case '\\':
            *d='\\';
            d++;
            *d='\\';
            d++;
            break;
        default:
            *d=*s;
            d++;
        }
    }
    *d='\0';

    return dst;
}

static void cx_print_value(const char* name, const WCHAR* value)
{
    if (value)
    {
        WCHAR* str = cx_escape_string(value);
        char *strA;

        strA = wchars_to_utf8_chars(str);

        printf("\"%s\"=\"%s\"\n", name, strA);

        HeapFree(GetProcessHeap(), 0, str);
        HeapFree(GetProcessHeap(), 0, strA);
    }
}

static void cx_dump_menu(LPCWSTR linkW, int is_desktop, LPCWSTR rootW,
                         LPCWSTR pathW, LPCWSTR argsW,
                         LPCWSTR icon_name, LPCWSTR description, LPCWSTR arch)
{
    /* -----------------------------------------------------------
    **   The Menu hack in particular is tracked by 
    ** CrossOver Hack 13785.
    ** (the whole system of cx_mode hacks in winemenubuilder
    ** is a diff from winehq which does not appear to be tracked
    ** by a bug in the hacks milestone.)
    ** ----------------------------------------------------------- */
    char *link = wchars_to_utf8_chars(linkW);

    printf("[%s]\n", link);
    cx_print_value("IsMenu", (is_desktop ? L"0" : L"1"));
    cx_print_value("Root", rootW);
    cx_print_value("Path", pathW);
    cx_print_value("Args", argsW);
    cx_print_value("Icon", icon_name);
    cx_print_value("Description", description);
    cx_print_value("Arch", arch);
    printf("\n");

    HeapFree(GetProcessHeap(), 0, link);
}

int cx_process_menu(LPCWSTR linkW, BOOL is_desktop, DWORD root_csidl,
                    LPCWSTR pathW, LPCWSTR argsW,
                    LPCWSTR icon_name, LPCWSTR description, IShellLinkW *sl)
{
    const WCHAR *arch = NULL; /* CrossOver hack 14227 */
    WCHAR rootW[MAX_PATH];
    int rc;

    /* CrossOver hack 14227 */
    if (sl && cx_link_is_64_bit(sl, 0)) arch = L"x86_64";

    SHGetSpecialFolderPathW(NULL, rootW, root_csidl, FALSE);

    WINE_TRACE("link=%s %s: %s path=%s args=%s icon=%s desc=%s arch=%s\n",
               debugstr_w(linkW), is_desktop ? "desktop" : "menu", debugstr_w(rootW),
               debugstr_w(pathW), debugstr_w(argsW), debugstr_w(icon_name), debugstr_w(description),
               debugstr_w(arch));

    if (cx_dump_menus)
    {
        rc = 0;
        cx_dump_menu(linkW, is_desktop, rootW, pathW, argsW, icon_name, description, arch);
    }
    else
        rc = cx_wineshelllink(linkW, is_desktop, rootW, pathW, argsW, icon_name, description, arch);

    return rc;
}

static IShellLinkW* load_link(const WCHAR* link)
{
    HRESULT r;
    IShellLinkW *sl;
    IPersistFile *pf;

    r = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkW, (void**)&sl);
    if (SUCCEEDED(r))
        r = IShellLinkW_QueryInterface(sl, &IID_IPersistFile, (void**)&pf);
    if (SUCCEEDED(r))
    {
        r = IPersistFile_Load(pf, link, STGM_READ);
        IPersistFile_Release(pf);
    }

    if (FAILED(r))
    {
        IShellLinkW_Release(sl);
        sl = NULL;
    }

    return sl;
}

static BOOL cx_link_is_64_bit(IShellLinkW *sl, int recurse_level)
{
    HRESULT hr;
    WCHAR temp[MAX_PATH];
    WCHAR path[MAX_PATH];
    const WCHAR *ext;
    DWORD type;

    hr = IShellLinkW_GetPath(sl, temp, sizeof(temp)/sizeof(temp[0]), NULL, SLGP_RAWPATH);
    if (hr != S_OK || !temp[0])
        return FALSE;
    ExpandEnvironmentStringsW(temp, path, sizeof(path)/sizeof(path[0]));

    ext = PathFindExtensionW(path);
    if (!lstrcmpiW(ext, L".lnk") && recurse_level < 5)
    {
        IShellLinkW *sl2 = load_link(path);
        if (sl2)
        {
            BOOL ret = cx_link_is_64_bit(sl2, recurse_level + 1);
            IShellLinkW_Release(sl2);
            return ret;
        }
    }

    if (!GetBinaryTypeW(path, &type))
        return FALSE;

    return (type == SCS_64BIT_BINARY);
}

/*
 * A CrossOver winemenubuilder extension.
 */

static BOOL cx_process_dir(WCHAR* dir)
{
    HANDLE hFind;
    WIN32_FIND_DATAW item;
    int lendir, len;
    WCHAR* path;
    BOOL rc;

    WINE_TRACE("scanning directory %s\n", wine_dbgstr_w(dir));
    lendir = lstrlenW(dir);
    lstrcatW(dir, L"\\*");
    hFind=FindFirstFileW(dir, &item);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        WINE_TRACE("unable to open the '%s' directory\n", wine_dbgstr_w(dir));
        return FALSE;
    }

    rc = TRUE;
    path = HeapAlloc(GetProcessHeap(), 0, (lendir+1+MAX_PATH+2+1)*sizeof(WCHAR));
    lstrcpyW(path, dir);
    path[lendir] = '\\';
    while (1)
    {
        if (wcscmp(item.cFileName, L".") && wcscmp(item.cFileName, L".."))
        {
            WINE_TRACE("  %s\n", wine_dbgstr_w(item.cFileName));
            len=lstrlenW(item.cFileName);
            if ((item.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
                (len >= 5 && lstrcmpiW(item.cFileName+len-4, L".lnk") == 0) ||
                (len >= 5 && lstrcmpiW(item.cFileName+len-4, L".url") == 0))
            {
                lstrcpyW(path+lendir+1, item.cFileName);
                if (item.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    if (!(item.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))  /* skip symlinks */
                    {
                        if (!cx_process_dir(path))
                            rc = FALSE;
                    }
                }
                else if (len >= 5 && lstrcmpiW(item.cFileName+len-4, L".url") == 0)
                {
                    WINE_TRACE("  url %s\n", wine_dbgstr_w(path));

                    if (!Process_URL(path, FALSE))
                        rc = FALSE;
                }
                else
                {
                    WINE_TRACE("  link %s\n", wine_dbgstr_w(path));

                    if (!Process_Link(path, FALSE))
                        rc=FALSE;
                }
            }
        }

        if (!FindNextFileW(hFind, &item))
        {
            if (GetLastError() != ERROR_NO_MORE_FILES)
            {
                WINE_TRACE("got error %d while scanning the '%s' directory\n", GetLastError(), wine_dbgstr_w(dir));
                rc = FALSE;
            }
            FindClose(hFind);
            break;
        }
    }

    HeapFree(GetProcessHeap(), 0, path);
    return rc;
}

BOOL cx_process_all_menus(void)
{
    static const DWORD locations[] = {
        /* CSIDL_STARTUP, Not interested in this one */
        CSIDL_DESKTOPDIRECTORY, CSIDL_STARTMENU,
        /* CSIDL_COMMON_STARTUP, Not interested in this one */
        CSIDL_COMMON_DESKTOPDIRECTORY, CSIDL_COMMON_STARTMENU };
    WCHAR dir[MAX_PATH+2]; /* +2 for cx_process_dir() */
    DWORD i, len, attr;
    BOOL rc;

    rc = TRUE;
    for (i = 0; i < sizeof(locations)/sizeof(locations[0]); i++)
    {
        if (!SHGetSpecialFolderPathW(0, dir, locations[i], FALSE))
        {
            WINE_TRACE("unable to get the path of folder %08x\n", locations[i]);
            /* Some special folders are not defined in some bottles
             * so this is not an error
             */
            continue;
        }

        len = lstrlenW(dir);
        if (len >= MAX_PATH)
        {
            /* We've just trashed memory! Hopefully we are OK */
            WINE_TRACE("Ignoring special folder %08x because its path is too long: %s\n", locations[i], wine_dbgstr_w(dir));
            rc = FALSE;
            continue;
        }

        /* Only scan directories. This is particularly important for Desktop
         * which may be a symbolic link to the native desktop.
         */
        attr = GetFileAttributesW( dir );
        if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY))
            WINE_TRACE("%s is not a directory, skipping it\n", debugstr_w(dir));
        else if (!cx_process_dir(dir))
            rc = FALSE;
    }
    return rc;
}
