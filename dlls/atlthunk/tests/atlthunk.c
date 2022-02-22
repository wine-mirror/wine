/*
 * Copyright 2019 Jacek Caban for CodeWeavers
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

#include "windows.h"
#include "atlthunk.h"
#include "wine/test.h"

static LRESULT WINAPI test_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    ok(msg == 1, "msg = %u\n", msg);
    ok(wparam == 2, "wparam = %Id\n", wparam);
    ok(lparam == 3, "lparam = %Id\n", lparam);
    return (LRESULT)hwnd | 0x1000;
}

static void test_thunk_proc(void)
{
    AtlThunkData_t *thunks[513];
    WNDPROC thunk_proc;
    LRESULT res;
    int i;

    for(i=0; i < ARRAY_SIZE(thunks); i++)
    {
        thunks[i] = AtlThunk_AllocateData();
        ok(thunks[i] != NULL, "thunk = NULL\n");

        AtlThunk_InitData(thunks[i], test_proc, i);
    }

    for(i=0; i < ARRAY_SIZE(thunks); i++)
    {
        thunk_proc = AtlThunk_DataToCode(thunks[i]);
        ok(thunk_proc != NULL, "thunk_proc = NULL\n");

        res = thunk_proc((HWND)0x1234, 1, 2, 3);
        ok(res == (i | 0x1000), "res = %Iu\n", res);
    }

    for(i=0; i < ARRAY_SIZE(thunks); i++)
        AtlThunk_FreeData(thunks[i]);
}

START_TEST(atlthunk)
{
    test_thunk_proc();
}
