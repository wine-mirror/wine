/* Unit test suite for SHLWAPI ordinal functions
 *
 * Copyright 2004 Jon Griffiths
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
 */

#include <stdlib.h>
#include <stdio.h>

#include "wine/test.h"
#include "winbase.h"
#include "winerror.h"
#include "winuser.h"
#define NO_SHLWAPI_REG
#define NO_SHLWAPI_PATH
#define NO_SHLWAPI_GDI
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"

/* Function ptrs for ordinal calls */
static HMODULE hShlwapi;
static int (WINAPI *pSHSearchMapInt)(const int*,const int*,int,int);
static HRESULT (WINAPI *pGetAcceptLanguagesA)(LPSTR,LPDWORD);

static void test_GetAcceptLanguagesA(void)
{   HRESULT retval;
    DWORD buffersize, buffersize2;
    char buffer[100];

    if (!pGetAcceptLanguagesA)
	return;

    SetLastError(ERROR_SUCCESS);
    retval = pGetAcceptLanguagesA( NULL, NULL);
    ok(retval == E_FAIL,
       "function result wrong: got %08lx; expected E_FAIL\n", retval);
    ok(ERROR_SUCCESS == GetLastError(), "last error set to %ld\n", GetLastError());

    buffersize = sizeof(buffer);
    SetLastError(ERROR_SUCCESS);
    retval = pGetAcceptLanguagesA( NULL, &buffersize);
    ok(retval == E_FAIL,
       "function result wrong: got %08lx; expected E_FAIL\n", retval);
    ok(buffersize == sizeof(buffer),
       "buffersize was changed (2nd parameter; not on Win2k)\n");
    ok(ERROR_SUCCESS == GetLastError(), "last error set to %ld\n", GetLastError());

    SetLastError(ERROR_SUCCESS);
    retval = pGetAcceptLanguagesA( buffer, NULL);
    ok(retval == E_FAIL,
       "function result wrong: got %08lx; expected E_FAIL\n", retval);
    ok(ERROR_SUCCESS == GetLastError(), "last error set to %ld\n", GetLastError());

    buffersize = 0;
    memset(buffer, 0, sizeof(buffer));
    SetLastError(ERROR_SUCCESS);
    retval = pGetAcceptLanguagesA( buffer, &buffersize);
    ok(retval == E_FAIL,
       "function result wrong: got %08lx; expected E_FAIL\n", retval);
    ok(buffersize == 0,
       "buffersize wrong(changed) got %08lx; expected 0 (2nd parameter; not on Win2k)\n", buffersize);
    ok(ERROR_SUCCESS == GetLastError(), "last error set to %ld\n", GetLastError());

    buffersize = buffersize2 = 1;
    memset(buffer, 0, sizeof(buffer));
    retval = pGetAcceptLanguagesA( buffer, &buffersize);
    todo_wine ok(retval == E_INVALIDARG, "function result wrong: got %08lx; expected E_INVALIDARG\n", retval);
    todo_wine ok(buffersize == 0,
                 "buffersize wrong: got %08lx, expected 0 (2nd parameter;Win2k)\n", buffersize);
    todo_wine ok(ERROR_INSUFFICIENT_BUFFER == GetLastError(),
                 "last error wrong: got %08lx; expected ERROR_INSUFFICIENT_BUFFER\n", GetLastError());
    todo_wine ok(buffersize2 == strlen(buffer),
                 "buffer content (length) wrong: got %08x, expected %08lx \n", strlen(buffer), buffersize2);

    buffersize = sizeof(buffer);
    memset(buffer, 0, sizeof(buffer));
    SetLastError(ERROR_SUCCESS);
    retval = pGetAcceptLanguagesA( buffer, &buffersize);
    ok(retval == S_OK, "function result wrong: got %08lx, expected S_OK\n", retval);
    ok(ERROR_SUCCESS == GetLastError(), "last error set to %ld\n", GetLastError());

    buffersize = buffersize2 = strlen(buffer);
    memset(buffer, 0, sizeof(buffer));
    SetLastError(ERROR_SUCCESS);
    retval = pGetAcceptLanguagesA( buffer, &buffersize);
    todo_wine ok(retval == E_INVALIDARG, "function result wrong: got %08lx; expected E_INVALIDARG\n", retval);
    todo_wine ok(buffersize == 0,
                 "buffersize wrong: got %08lx, expected 0 (2nd parameter;Win2k)\n", buffersize);
    todo_wine ok(ERROR_INSUFFICIENT_BUFFER == GetLastError(),
                 "last error wrong: got %08lx; expected ERROR_INSUFFICIENT_BUFFER\n", GetLastError());
    ok(buffersize2 == strlen(buffer),
       "buffer content (length) wrong: got %08x, expected %08lx \n", strlen(buffer), buffersize2);
}

static void test_SHSearchMapInt(void)
{
  int keys[8], values[8];
  int i = 0;

  if (!pSHSearchMapInt)
    return;

  memset(keys, 0, sizeof(keys));
  memset(values, 0, sizeof(values));
  keys[0] = 99; values[0] = 101;

  /* NULL key/value lists crash native, so skip testing them */

  /* 1 element */
  i = pSHSearchMapInt(keys, values, 1, keys[0]);
  ok(i == values[0], "Len 1, expected %d, got %d\n", values[0], i);

  /* Key doesn't exist */
  i = pSHSearchMapInt(keys, values, 1, 100);
  ok(i == -1, "Len 1 - bad key, expected -1, got %d\n", i);

  /* Len = 0 => not found */
  i = pSHSearchMapInt(keys, values, 0, keys[0]);
  ok(i == -1, "Len 1 - passed len 0, expected -1, got %d\n", i);

  /* 2 elements, len = 1 */
  keys[1] = 98; values[1] = 102;
  i = pSHSearchMapInt(keys, values, 1, keys[1]);
  ok(i == -1, "Len 1 - array len 2, expected -1, got %d\n", i);

  /* 2 elements, len = 2 */
  i = pSHSearchMapInt(keys, values, 2, keys[1]);
  ok(i == values[1], "Len 2, expected %d, got %d\n", values[1], i);

  /* Searches forward */
  keys[2] = 99; values[2] = 103;
  i = pSHSearchMapInt(keys, values, 3, keys[0]);
  ok(i == values[0], "Len 3, expected %d, got %d\n", values[0], i);
}


START_TEST(ordinal)
{
  hShlwapi = LoadLibraryA("shlwapi.dll");
  ok(hShlwapi != 0, "LoadLibraryA failed\n");
  if (!hShlwapi)
    return;

  pGetAcceptLanguagesA = (void*)GetProcAddress(hShlwapi, (LPSTR)14);
  pSHSearchMapInt = (void*)GetProcAddress(hShlwapi, (LPSTR)198);

  test_GetAcceptLanguagesA();
  test_SHSearchMapInt();
  FreeLibrary(hShlwapi);
}
