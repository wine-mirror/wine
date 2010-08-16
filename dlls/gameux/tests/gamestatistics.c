/*
 * IGameStatisticsMgr tests
 *
 * Copyright (C) 2010 Mariusz Pluciński
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

#include "gameux.h"
#include "wine/test.h"

static void test_create( void )
{
    HRESULT hr;

    IGameStatisticsMgr* gsm = NULL;

    /* interface available up from Win7 */
    hr = CoCreateInstance( &CLSID_GameStatistics, NULL, CLSCTX_INPROC_SERVER, &IID_IGameStatisticsMgr, (LPVOID*)&gsm);
    if(gsm)
    {
        ok( hr == S_OK, "IGameStatisticsMgr creating failed (result: 0x%08x)\n", hr);
        IGameStatisticsMgr_Release(gsm);
    }
    else
        win_skip("IGameStatisticsMgr cannot be created\n");
}

START_TEST(gamestatistics)
{
    HRESULT hr;

    hr = CoInitialize( NULL );
    ok( hr == S_OK, "failed to init COM\n");

    test_create();

    CoUninitialize();
}
