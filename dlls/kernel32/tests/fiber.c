/*
 * Unit tests for fiber functions
 *
 * Copyright (c) 2010 André Hentschel
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

static LPVOID (WINAPI *pCreateFiber)(SIZE_T,LPFIBER_START_ROUTINE,LPVOID);
static LPVOID (WINAPI *pConvertThreadToFiber)(LPVOID);
static BOOL (WINAPI *pConvertFiberToThread)(void);
static void (WINAPI *pSwitchToFiber)(LPVOID);
static void (WINAPI *pDeleteFiber)(LPVOID);

static LPVOID fibers[2];
static BYTE testparam = 187;

static BOOL init_funcs(void)
{
    HMODULE hKernel32 = GetModuleHandle("kernel32");

#define X(f) if (!(p##f = (void*)GetProcAddress(hKernel32, #f))) return FALSE;
    X(CreateFiber);
    X(ConvertThreadToFiber);
    X(ConvertFiberToThread);
    X(SwitchToFiber);
    X(DeleteFiber);
#undef X

    return TRUE;
}

static VOID WINAPI FiberMainProc(LPVOID lpFiberParameter)
{
    BYTE *tparam = (BYTE *)lpFiberParameter;
    ok(*tparam == 187, "Parameterdata expected not to be changed\n");
    pSwitchToFiber(fibers[0]);
}

static void test_ConvertThreadToFiber(void)
{
    fibers[0] = pConvertThreadToFiber(&testparam);
    ok(fibers[0] != 0, "ConvertThreadToFiber failed with error %d\n", GetLastError());
}

static void test_ConvertFiberToThread(void)
{
    ok(pConvertFiberToThread() , "ConvertThreadToFiber failed with error %d\n", GetLastError());
}

static void test_CreateFiber(void)
{
    fibers[1] = pCreateFiber(0,FiberMainProc,&testparam);
    ok(fibers[1] != 0, "CreateFiber failed with error %d\n", GetLastError());

    pSwitchToFiber(fibers[1]);
    pDeleteFiber(fibers[1]);
}

START_TEST(fiber)
{
    if (!init_funcs())
    {
        win_skip("Needed functions are not available\n");
        return;
    }

    test_ConvertThreadToFiber();
    test_ConvertFiberToThread();
    test_ConvertThreadToFiber();
    test_CreateFiber();
}
