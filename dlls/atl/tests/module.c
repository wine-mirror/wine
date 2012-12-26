/*
 * ATL test program
 *
 * Copyright 2010 Marcus Meissner
 *
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

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include <atlbase.h>

#include <wine/test.h>

#define MAXSIZE 512
static void test_StructSize(void)
{
    _ATL_MODULEW *tst;
    HRESULT hres;
    int i;

    tst = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAXSIZE);

    for (i=0;i<MAXSIZE;i++) {
        tst->cbSize = i;
        hres = AtlModuleInit(tst, NULL, NULL);

        switch (i)  {
        case FIELD_OFFSET(_ATL_MODULEW, dwAtlBuildVer):
        case sizeof(_ATL_MODULEW):
#ifdef _WIN64
        case sizeof(_ATL_MODULEW) + sizeof(void *):
#endif
            ok (hres == S_OK, "AtlModuleInit with %d failed (0x%x).\n", i, (int)hres);
            break;
        default:
            ok (FAILED(hres), "AtlModuleInit with %d succeeded? (0x%x).\n", i, (int)hres);
            break;
        }
    }

    HeapFree (GetProcessHeap(), 0, tst);
}

START_TEST(module)
{
    test_StructSize();
}
