/*
 * Unit tests for shelllinks
 *
 * Copyright 2004 Mike McCormack
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * This is a test program for the SHGet{Special}Folder{Path|Location} functions
 * of shell32, that get either a filesytem path or a LPITEMIDLIST (shell
 * namespace) path for a given folder (CSIDL value).
 *
 */

#define COBJMACROS

#include <stdarg.h>
#include <stdio.h>
#include "windef.h"
#include "winbase.h"
#include "shlguid.h"
#include "shobjidl.h"
#include "wine/test.h"

static void test_empty_shelllink()
{
    HRESULT r;
    IShellLinkW *sl;
    WCHAR buffer[0x100];
    IPersistFile *pf;

    CoInitialize(NULL);

    /* empty shelllink ? */
    r = CoCreateInstance( &CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                            &IID_IShellLinkW, (LPVOID*)&sl );
    ok (r == S_OK, "no shelllink\n");
    if (r == S_OK)
    {
        static const WCHAR lnk[]= { 'C',':','\\','t','e','s','t','.','l','n','k',0 };

        buffer[0]='x';
        buffer[1]=0;
        r = IShellLinkW_GetPath(sl, buffer, 0x100, NULL, SLGP_RAWPATH );
        ok (r == S_OK, "GetPath failed\n");
        ok (buffer[0]==0, "path wrong\n");

        r = IShellLinkW_QueryInterface(sl, &IID_IPersistFile, (LPVOID*) &pf );
        ok (r == S_OK, "no IPersistFile\n");
        todo_wine
        {
        if (r == S_OK )
        {
            r = IPersistFile_Save(pf, lnk, TRUE);
            ok (r == S_OK, "save failed\n");
            IPersistFile_Release(pf);
        }

        IShellLinkW_Release(sl);

        ok (DeleteFileW(lnk), "failed to delete link\n");
        }
    }
}

START_TEST(shelllink)
{
    test_empty_shelllink();
}
