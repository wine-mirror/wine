/*
 * Unit test suite for comdlg32 API functions: font dialogs
 *
 * Copyright 2009 Vincent Povirk for CodeWeavers
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
 *
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "winuser.h"
#include "objbase.h"

#include "commdlg.h"

#include "wine/test.h"

static UINT_PTR CALLBACK CFHookProcOK(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        PostMessageA(hdlg, WM_COMMAND, IDOK, FALSE);
        return 0;
    default:
        return 0;
    }
}

static void test_ChooseFontA(void)
{
    LOGFONTA lfa;
    CHOOSEFONTA cfa;
    BOOL ret;

    memset(&lfa, 0, sizeof(LOGFONTA));
    lfa.lfHeight = -16;
    lfa.lfWeight = FW_NORMAL;
    strcpy(lfa.lfFaceName, "Symbol");

    memset(&cfa, 0, sizeof(CHOOSEFONTA));
    cfa.lStructSize = sizeof(cfa);
    cfa.lpLogFont = &lfa;
    cfa.Flags = CF_ENABLEHOOK|CF_INITTOLOGFONTSTRUCT|CF_SCREENFONTS;
    cfa.lpfnHook = CFHookProcOK;

    ret = ChooseFontA(&cfa);

    ok(ret == TRUE, "ChooseFontA returned FALSE\n");
    ok(lfa.lfHeight == -16, "Expected -16, got %i\n", lfa.lfHeight);
    ok(lfa.lfWeight == FW_NORMAL, "Expected FW_NORMAL, got %i\n", lfa.lfWeight);
    ok(strcmp(lfa.lfFaceName, "Symbol") == 0, "Expected Symbol, got %s\n", lfa.lfFaceName);
}

START_TEST(fontdlg)
{
    test_ChooseFontA();
}
