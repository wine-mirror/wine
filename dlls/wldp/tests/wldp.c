/*
 * Copyright 2023 Hans Leidekker for CodeWeavers
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

#include "windef.h"
#include "winbase.h"
#include "wldp.h"

#include "wine/test.h"

static HRESULT (WINAPI *pWldpGetLockdownPolicy)(WLDP_HOST_INFORMATION *, DWORD *, DWORD);
static HRESULT (WINAPI *pWldpQueryWindowsLockdownMode)(WLDP_WINDOWS_LOCKDOWN_MODE *);

static void test_WldpGetLockdownPolicy(void)
{
    WLDP_HOST_INFORMATION info;
    DWORD state;
    HRESULT hr;

    if (!pWldpGetLockdownPolicy)
    {
        win_skip( "WldpGetLockdownPolicy not available\n" );
        return;
    }

    state = 0xdeadbeef;
    hr = pWldpGetLockdownPolicy( NULL, &state, 0 );
    ok( hr == 0x10000000, "got %#lx\n", hr );
    ok( state == WLDP_LOCKDOWN_DEFINED_FLAG, "got %#lx\n", state );

    memset( &info, 0, sizeof(info) );
    info.dwRevision = 1;
    info.dwHostId = WLDP_HOST_ID_GLOBAL;
    state = 0xdeadbeef;
    hr = pWldpGetLockdownPolicy( &info, &state, 0 );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( state == WLDP_LOCKDOWN_DEFINED_FLAG, "got %#lx\n", state );
}

static void test_WldpQueryWindowsLockdownMode(void)
{
    WLDP_WINDOWS_LOCKDOWN_MODE mode;
    HRESULT hr;

    if (!pWldpQueryWindowsLockdownMode)
    {
        win_skip( "WldpQueryWindowsLockdownMode not available\n" );
        return;
    }

    mode = 0xdeadbeef;
    hr = pWldpQueryWindowsLockdownMode( &mode );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( mode == WLDP_WINDOWS_LOCKDOWN_MODE_UNLOCKED, "got %u\n", mode );
}

START_TEST(wldp)
{
    HMODULE hwldp = LoadLibraryW( L"wldp" );

    pWldpGetLockdownPolicy = (void *)GetProcAddress( hwldp, "WldpGetLockdownPolicy" );
    pWldpQueryWindowsLockdownMode = (void *)GetProcAddress( hwldp, "WldpQueryWindowsLockdownMode" );

    test_WldpGetLockdownPolicy();
    test_WldpQueryWindowsLockdownMode();
}
