/* Unit test suite for wintrust crypt functions
 *
 * Copyright 2007 Paul Vriens
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
#include <stdio.h>

#include "windows.h"
#include "mscat.h"

#include "wine/test.h"

static char selfname[MAX_PATH];

static CHAR CURR_DIR[MAX_PATH];

static HMODULE hWintrust = 0;

static BOOL (WINAPI * pCryptCATAdminAcquireContext)(HCATADMIN*, const GUID*, DWORD);
static BOOL (WINAPI * pCryptCATAdminReleaseContext)(HCATADMIN, DWORD);
static BOOL (WINAPI * pCryptCATAdminCalcHashFromFileHandle)(HANDLE hFile, DWORD*, BYTE*, DWORD);

#define WINTRUST_GET_PROC(func) \
    p ## func = (void*)GetProcAddress(hWintrust, #func); \
    if(!p ## func) { \
      trace("GetProcAddress(%s) failed\n", #func); \
    }

static BOOL InitFunctionPtrs(void)
{
    hWintrust = LoadLibraryA("wintrust.dll");

    if(!hWintrust)
    {
        skip("Could not load wintrust.dll\n");
        return FALSE;
    }

    WINTRUST_GET_PROC(CryptCATAdminAcquireContext)
    WINTRUST_GET_PROC(CryptCATAdminReleaseContext)
    WINTRUST_GET_PROC(CryptCATAdminCalcHashFromFileHandle)

    return TRUE;
}

static void test_context(void)
{
    BOOL ret;
    HCATADMIN hca;
    static GUID dummy   = { 0xdeadbeef, 0xdead, 0xbeef, { 0xde,0xad,0xbe,0xef,0xde,0xad,0xbe,0xef }};
    static GUID unknown = { 0xC689AABA, 0x8E78, 0x11D0, { 0x8C,0x47,0x00,0xC0,0x4F,0xC2,0x95,0xEE }}; /* WINTRUST.DLL */

    if (!pCryptCATAdminAcquireContext || !pCryptCATAdminReleaseContext)
    {
        skip("CryptCATAdminAcquireContext and/or CryptCATAdminReleaseContext are not available\n");
        return;
    }

    /* All NULL */
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminAcquireContext(NULL, NULL, 0);
    todo_wine
    {
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    }

    /* NULL GUID */
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminAcquireContext(&hca, NULL, 0);
    ok(ret, "Expected success\n");
    ok(GetLastError() == ERROR_SUCCESS ||
       GetLastError() == 0xdeadbeef /* Vista */,
       "Expected ERROR_SUCCESS or 0xdeadbeef, got %d\n", GetLastError());
    ok(hca != NULL, "Expected a context handle, got NULL\n");

    /* All NULL */
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminReleaseContext(NULL, 0);
    todo_wine
    {
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    }

    /* Proper release */
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminReleaseContext(hca, 0);
    ok(ret, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef,
       "Expected no change in last error, got %d\n", GetLastError());

    /* Try to release a second time */
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminReleaseContext(hca, 0);
    todo_wine
    {
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    }

    /* NULL context handle and dummy GUID */
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminAcquireContext(NULL, &dummy, 0);
    todo_wine
    {
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    }

    /* Correct context handle and dummy GUID */
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminAcquireContext(&hca, &dummy, 0);
    ok(ret, "Expected success\n");
    ok(GetLastError() == ERROR_SUCCESS ||
       GetLastError() == 0xdeadbeef /* Vista */,
       "Expected ERROR_SUCCESS or 0xdeadbeef, got %d\n", GetLastError());
    ok(hca != NULL, "Expected a context handle, got NULL\n");

    ret = pCryptCATAdminReleaseContext(hca, 0);
    ok(ret, "Expected success\n");

    /* Correct context handle and GUID */
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminAcquireContext(&hca, &unknown, 0);
    ok(ret, "Expected success\n");
    ok(GetLastError() == ERROR_SUCCESS ||
       GetLastError() == 0xdeadbeef /* Vista */,
       "Expected ERROR_SUCCESS or 0xdeadbeef, got %d\n", GetLastError());
    ok(hca != NULL, "Expected a context handle, got NULL\n");

    ret = pCryptCATAdminReleaseContext(hca, 0);
    ok(ret, "Expected success\n");

    /* Flags not equal to 0 */
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminAcquireContext(&hca, &unknown, 1);
    ok(ret, "Expected success\n");
    ok(GetLastError() == ERROR_SUCCESS ||
       GetLastError() == 0xdeadbeef /* Vista */,
       "Expected ERROR_SUCCESS or 0xdeadbeef, got %d\n", GetLastError());
    ok(hca != NULL, "Expected a context handle, got NULL\n");

    ret = pCryptCATAdminReleaseContext(hca, 0);
    ok(ret, "Expected success\n");
}

/* TODO: Check whether SHA-1 is the algorithm that's always used */
static void test_calchash(void)
{
    BOOL ret;
    HANDLE file;
    DWORD hashsize = 0;
    BYTE* hash;
    BYTE expectedhash[20] = {0x3a,0xa1,0x19,0x08,0xec,0xa6,0x0d,0x2e,0x7e,0xcc,0x7a,0xca,0xf5,0xb8,0x2e,0x62,0x6a,0xda,0xf0,0x19};
    CHAR temp[MAX_PATH];
    DWORD written;

    if (!pCryptCATAdminCalcHashFromFileHandle)
    {
        skip("CryptCATAdminCalcHashFromFileHandle is not available\n");
        return;
    }
    
    /* All NULL */
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminCalcHashFromFileHandle(NULL, NULL, NULL, 0);
    todo_wine
    {
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    }

    /* NULL filehandle, rest is legal */
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminCalcHashFromFileHandle(NULL, &hashsize, NULL, 0);
    todo_wine
    {
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    }

    /* Correct filehandle, rest is NULL */
    file = CreateFileA(selfname, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminCalcHashFromFileHandle(file, NULL, NULL, 0);
    todo_wine
    {
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    }
    CloseHandle(file);

    /* All OK, but dwFlags set to 1 */
    file = CreateFileA(selfname, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminCalcHashFromFileHandle(file, &hashsize, NULL, 1);
    todo_wine
    {
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    }
    CloseHandle(file);

    /* All OK, requesting the size of the hash */
    file = CreateFileA(selfname, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminCalcHashFromFileHandle(file, &hashsize, NULL, 0);
    ok(ret, "Expected success\n");
    todo_wine
    {
    ok(hashsize == 20," Expected a hash size of 20, got %d\n", hashsize);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    }
    CloseHandle(file);

    /* All OK, retrieve the hash
     * Double the hash buffer to see what happens to the size parameter
     */
    file = CreateFileA(selfname, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    hashsize *= 2;
    hash = HeapAlloc(GetProcessHeap(), 0, hashsize);
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminCalcHashFromFileHandle(file, &hashsize, hash, 0);
    ok(ret, "Expected success\n");
    todo_wine
    {
    ok(hashsize == 20," Expected a hash size of 20, got %d\n", hashsize);
    ok(GetLastError() == ERROR_SUCCESS,
       "Expected ERROR_SUCCESS, got %d\n", GetLastError());
    }
    CloseHandle(file);
    HeapFree(GetProcessHeap(), 0, hash);

    /* Do the same test with a file created and filled by ourselves (and we thus
     * have a known hash for).
     */
    GetTempFileNameA(CURR_DIR, "hsh", 0, temp); 
    file = CreateFileA(temp, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    WriteFile(file, "Text in this file is needed to create a know hash", 49, &written, NULL);
    CloseHandle(file);

    /* All OK, first request the size and then retrieve the hash */
    file = CreateFileA(temp, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    hashsize = 0;
    pCryptCATAdminCalcHashFromFileHandle(file, &hashsize, NULL, 0);
    hash = HeapAlloc(GetProcessHeap(), 0, hashsize);
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminCalcHashFromFileHandle(file, &hashsize, hash, 0);
    ok(ret, "Expected success\n");
    todo_wine
    {
    ok(GetLastError() == ERROR_SUCCESS,
       "Expected ERROR_SUCCESS, got %d\n", GetLastError());
    ok(!memcmp(hash, expectedhash, sizeof(expectedhash)), "Hashes didn't match\n");
    }
    CloseHandle(file);

    HeapFree(GetProcessHeap(), 0, hash);
    DeleteFileA(temp);
}

START_TEST(crypt)
{
    int myARGC;
    char** myARGV;

    if(!InitFunctionPtrs())
        return;

    myARGC = winetest_get_mainargs(&myARGV);
    strcpy(selfname, myARGV[0]);

    GetCurrentDirectoryA(MAX_PATH, CURR_DIR);
   
    test_context();
    test_calchash();

    FreeLibrary(hWintrust);
}
