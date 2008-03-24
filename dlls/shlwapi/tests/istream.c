/* Unit test suite for SHLWAPI ShCreateStreamOnFile functions.
 *
 * Copyright 2008 Reece H. Dunn
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

#define COBJMACROS

#include <stdarg.h>
#include <stdio.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "shlwapi.h"


/* Function pointers for the SHCreateStreamOnFile functions */
static HMODULE hShlwapi;
static HRESULT (WINAPI *pSHCreateStreamOnFileA)(LPCSTR file, DWORD mode, IStream **stream);


static void test_SHCreateStreamOnFileA(DWORD mode)
{
    IStream * stream;
    HRESULT ret;
    ULONG refcount;
    static const char * test_file = "c:\\test.txt";

    printf("SHCreateStreamOnFileA: testing mode %d\n", mode);

    /* invalid arguments */

    stream = NULL;
    ret = (*pSHCreateStreamOnFileA)(NULL, mode, &stream);
    todo_wine
    ok(ret == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND), "SHCreateStreamOnFileA: expected HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND), got 0x%08x\n", ret);
    ok(stream == NULL, "SHCreateStreamOnFileA: expected a NULL IStream object, got %p\n", stream);

#if 0 /* This test crashes on WinXP SP2 */
    ret = (*pSHCreateStreamOnFileA)(test_file, mode, NULL);
    ok(ret == E_INVALIDARG, "SHCreateStreamOnFileA: expected E_INVALIDARG, got 0x%08x\n", ret);
#endif

    stream = NULL;
    ret = (*pSHCreateStreamOnFileA)(test_file, mode | STGM_CONVERT, &stream);
    ok(ret == E_INVALIDARG, "SHCreateStreamOnFileA: expected E_INVALIDARG, got 0x%08x\n", ret);
    ok(stream == NULL, "SHCreateStreamOnFileA: expected a NULL IStream object, got %p\n", stream);

    stream = NULL;
    ret = (*pSHCreateStreamOnFileA)(test_file, mode | STGM_DELETEONRELEASE, &stream);
    ok(ret == E_INVALIDARG, "SHCreateStreamOnFileA: expected E_INVALIDARG, got 0x%08x\n", ret);
    ok(stream == NULL, "SHCreateStreamOnFileA: expected a NULL IStream object, got %p\n", stream);

    /* file does not exist */

    stream = NULL;
    ret = (*pSHCreateStreamOnFileA)(test_file, mode | STGM_FAILIFTHERE, &stream);
    todo_wine
    ok(ret == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "SHCreateStreamOnFileA: expected HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), got 0x%08x\n", ret);
    ok(stream == NULL, "SHCreateStreamOnFileA: expected a NULL IStream object, got %p\n", stream);

    stream = NULL;
    ret = (*pSHCreateStreamOnFileA)(test_file, mode | STGM_CREATE, &stream);
    todo_wine
    ok(ret == S_OK, "SHCreateStreamOnFileA: expected S_OK, got 0x%08x\n", ret);
    todo_wine
    ok(stream != NULL, "SHCreateStreamOnFileA: expected a valid IStream object, got NULL\n");

    if (stream) {
        refcount = IStream_Release(stream);
        ok(refcount == 0, "SHCreateStreamOnFileA: expected 0, got %d\n", refcount);
    }

    /* NOTE: don't delete the file, as it will be used for the file exists tests. */

    /* file exists */

    stream = NULL;
    ret = (*pSHCreateStreamOnFileA)(test_file, mode | STGM_FAILIFTHERE, &stream);
    todo_wine
    ok(ret == S_OK, "SHCreateStreamOnFileA: expected S_OK, got 0x%08x\n", ret);
    todo_wine
    ok(stream != NULL, "SHCreateStreamOnFileA: expected a valid IStream object, got NULL\n");

    if (stream) {
        refcount = IStream_Release(stream);
        ok(refcount == 0, "SHCreateStreamOnFileA: expected 0, got %d\n", refcount);
    }

    stream = NULL;
    ret = (*pSHCreateStreamOnFileA)(test_file, mode | STGM_CREATE, &stream);
    todo_wine
    ok(ret == S_OK, "SHCreateStreamOnFileA: expected S_OK, got 0x%08x\n", ret);
    todo_wine
    ok(stream != NULL, "SHCreateStreamOnFileA: expected a valid IStream object, got NULL\n");

    if (stream) {
        refcount = IStream_Release(stream);
        ok(refcount == 0, "SHCreateStreamOnFileA: expected 0, got %d\n", refcount);

        ok(DeleteFileA(test_file), "SHCreateStreamOnFileA: could not delete file '%s', got error %d\n", test_file, GetLastError());
    }
}


START_TEST(istream)
{
    hShlwapi = GetModuleHandleA("shlwapi.dll");

    pSHCreateStreamOnFileA = (void*)GetProcAddress(hShlwapi, "SHCreateStreamOnFileA");

    if (!pSHCreateStreamOnFileA)
        printf("SHCreateStreamOnFileA not found... skipping tests.\n");
    else {
        test_SHCreateStreamOnFileA(STGM_READ);
        test_SHCreateStreamOnFileA(STGM_WRITE);
        test_SHCreateStreamOnFileA(STGM_READWRITE);
    }
}
