/* OLEDB Database tests
 *
 * Copyright 2012 Alistair Leslie-Hughes
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

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "msdadc.h"
#include "msdasc.h"

#include "wine/test.h"


static void test_database(void)
{
    HRESULT hr;
    IDBInitialize *dbinit = NULL;
    IDataInitialize *datainit = NULL;

    hr = CoCreateInstance(&CLSID_MSDAINITIALIZE, NULL, CLSCTX_INPROC_SERVER, &IID_IDataInitialize,(void**)&datainit);
    if(FAILED(hr))
    {
        win_skip("Unable to load oledb library\n");
        return;
    }

    hr = IDataInitialize_GetDataSource(datainit, NULL, CLSCTX_INPROC_SERVER, NULL, &IID_IDBInitialize, (IUnknown **)&dbinit);
    ok(hr == S_OK, "got %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        IDBProperties *props = NULL;

        hr = IDBInitialize_QueryInterface(dbinit, &IID_IDBProperties, (void**)&props);
        ok(hr == S_OK, "got %08x\n", hr);
        if(SUCCEEDED(hr))
        {
            IDBProperties_Release(props);
        }

        IDBInitialize_Release(dbinit);
    }

    IDataInitialize_Release(datainit);
}

START_TEST(database)
{
    OleInitialize(NULL);

    test_database();

    OleUninitialize();
}
