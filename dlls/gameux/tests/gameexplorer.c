/*
 * IGameExplorer tests
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

#include "initguid.h"

#include "windows.h"
#include "ole2.h"
#include "objsafe.h"
#include "objbase.h"
#include "gameux.h"

#include "wine/test.h"


static void test_create( void )
{
    HRESULT hr;

    IGameExplorer* ge = NULL;

    /* interface available up from Vista */
    hr = CoCreateInstance( &CLSID_GameExplorer, NULL, CLSCTX_INPROC_SERVER, &IID_IGameExplorer, (LPVOID*)&ge);
    if(ge)
    {
        ok(hr == S_OK, "IGameExplorer creating failed (result false)\n");
        IGameExplorer_Release(ge);
    }
    else
        win_skip("IGameExplorer cannot be created\n");
}

START_TEST(gameexplorer)
{
    HRESULT r;

    r = CoInitialize( NULL );
    ok( r == S_OK, "failed to init COM\n");

    test_create();

    CoUninitialize();
}
