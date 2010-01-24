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
static LPVOID (WINAPI *pConvertThreadToFiberEx)(LPVOID,DWORD);
static LPVOID (WINAPI *pCreateFiberEx)(SIZE_T,SIZE_T,DWORD,LPFIBER_START_ROUTINE,LPVOID);

static LPVOID fibers[2];
static BYTE testparam = 185;

static VOID init_funcs(void)
{
    HMODULE hKernel32 = GetModuleHandle("kernel32");

#define X(f) p##f = (void*)GetProcAddress(hKernel32, #f);
    X(CreateFiber);
    X(ConvertThreadToFiber);
    X(ConvertFiberToThread);
    X(SwitchToFiber);
    X(DeleteFiber);
    X(ConvertThreadToFiberEx);
    X(CreateFiberEx);
#undef X
}

static VOID WINAPI FiberMainProc(LPVOID lpFiberParameter)
{
    BYTE *tparam = (BYTE *)lpFiberParameter;
    ok(*tparam == 185, "Parameterdata expected not to be changed\n");
    pSwitchToFiber(fibers[0]);
}

static void test_ConvertThreadToFiber(void)
{
    if (pConvertThreadToFiber)
    {
        fibers[0] = pConvertThreadToFiber(&testparam);
        ok(fibers[0] != 0, "ConvertThreadToFiber failed with error %d\n", GetLastError());
    }
    else
    {
        win_skip( "ConvertThreadToFiber not present\n" );
    }
}

static void test_ConvertThreadToFiberEx(void)
{
    if (pConvertThreadToFiberEx)
    {
        fibers[0] = pConvertThreadToFiberEx(&testparam, 0);
        ok(fibers[0] != 0, "ConvertThreadToFiberEx failed with error %d\n", GetLastError());
    }
    else
    {
        win_skip( "ConvertThreadToFiberEx not present\n" );
    }
}

static void test_ConvertFiberToThread(void)
{
    if (pConvertFiberToThread)
    {
        ok(pConvertFiberToThread() , "ConvertFiberToThread failed with error %d\n", GetLastError());
    }
    else
    {
        win_skip( "ConvertFiberToThread not present\n" );
    }
}

static void test_FiberHandling(void)
{
    fibers[0] = pCreateFiber(0,FiberMainProc,&testparam);
    ok(fibers[0] != 0, "CreateFiber failed with error %d\n", GetLastError());
    pDeleteFiber(fibers[0]);

    test_ConvertThreadToFiber();
    test_ConvertFiberToThread();
    if (pConvertThreadToFiberEx)
        test_ConvertThreadToFiberEx();
    else
        test_ConvertThreadToFiber();


    fibers[1] = pCreateFiber(0,FiberMainProc,&testparam);
    ok(fibers[1] != 0, "CreateFiber failed with error %d\n", GetLastError());

    pSwitchToFiber(fibers[1]);
    pDeleteFiber(fibers[1]);

    if (!pCreateFiberEx)
    {
        win_skip( "CreateFiberEx not present\n" );
        return;
    }

    fibers[1] = pCreateFiberEx(0,0,0,FiberMainProc,&testparam);
    ok(fibers[1] != 0, "CreateFiberEx failed with error %d\n", GetLastError());

    pSwitchToFiber(fibers[1]);
    pDeleteFiber(fibers[1]);
}

START_TEST(fiber)
{
    init_funcs();

    if (!pCreateFiber)
    {
        win_skip( "Fibers not supported by win95\n" );
        return;
    }

    test_FiberHandling();
}
