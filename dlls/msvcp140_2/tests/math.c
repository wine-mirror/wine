/*
 * Special math functions
 *
 * Copyright 2024 Piotr Caban for CodeWeavers
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

#include "wine/test.h"
#include <math.h>

static double (__stdcall *p___std_smf_hypot3)(double x, double y, double z);

#define SETNOFAIL(x,y) x = (void*)GetProcAddress(msvcp, y)
#define SET(x,y) do { SETNOFAIL(x, y); ok(x != NULL, "Export '%s' not found\n", y); } while(0)
static BOOL init(void)
{
    HMODULE msvcp = LoadLibraryA("msvcp140_2.dll");

    if (!msvcp)
    {
        win_skip("msvcp140_2.dll not installed\n");
        return FALSE;
    }

#ifdef __i386__
    SET(p___std_smf_hypot3, "___std_smf_hypot3@24");
#else
    SET(p___std_smf_hypot3, "__std_smf_hypot3");
#endif

    return TRUE;
}

static inline BOOL compare_double(double f, double g, unsigned int ulps)
{
    ULONGLONG x = *(ULONGLONG *)&f;
    ULONGLONG y = *(ULONGLONG *)&g;

    if (f < 0)
        x = ~x + 1;
    else
        x |= ((ULONGLONG)1) << 63;
    if (g < 0)
        y = ~y + 1;
    else
        y |= ((ULONGLONG)1) << 63;

    return (x > y ? x - y : y - x) <= ulps;
}

static void test_hypot3(void)
{
    double r;

    r = p___std_smf_hypot3(0, 0, 0);
    ok(compare_double(r, 0.0, 1), "r = %.23e\n", r);

    r = p___std_smf_hypot3(9, 12, 20);
    ok(compare_double(r, 25.0, 1), "r = %.23e\n", r);
}

START_TEST(math)
{
    if (!init())
        return;

    test_hypot3();
}
