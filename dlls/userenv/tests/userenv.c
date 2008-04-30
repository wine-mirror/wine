/*
 * Unit test suite for userenv functions
 *
 * Copyright 2008 Google (Lei Zhang)
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

#include <stdlib.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"

#include "userenv.h"

#include "wine/test.h"

#define expect(EXPECTED,GOT) ok((GOT)==(EXPECTED), "Expected %d, got %d\n", (EXPECTED), (GOT))

static void test_create_env(void)
{
    BOOL r;
    HANDLE htok;
    WCHAR * env;
    WCHAR * env2;

    r = CreateEnvironmentBlock(NULL, NULL, FALSE);
    expect(FALSE, r);

    r = CreateEnvironmentBlock((LPVOID) &env, NULL, FALSE);
    todo_wine expect(TRUE, r);

    r = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY|TOKEN_DUPLICATE, &htok);
    expect(TRUE, r);

    r = CreateEnvironmentBlock(NULL, htok, FALSE);
    expect(FALSE, r);

    r = CreateEnvironmentBlock((LPVOID) &env2, htok, FALSE);
    todo_wine expect(TRUE, r);
}

START_TEST(userenv)
{
    test_create_env();
}
