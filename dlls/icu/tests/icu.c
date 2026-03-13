/*
 * Unit test suite for ICU
 *
 * Copyright 2025 Zhiyi Zhang for CodeWeavers
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
#include <stdint.h>
#include "windef.h"
#include "winbase.h"
#include "icu.h"
#include "wine/test.h"

static void test_u_init(void)
{
    UErrorCode error = U_ZERO_ERROR;

    u_init(&error);
    ok(error == U_ZERO_ERROR, "Got unexpected error %d.\n", error);
}

START_TEST(icu)
{
    test_u_init();
}
