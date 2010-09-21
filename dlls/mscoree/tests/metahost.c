/*
 * Copyright 2010 Vincent Povirk
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

#include <stdarg.h>

#include "windef.h"
#include "ole2.h"

#include "initguid.h"
#include "metahost.h"
#include "wine/test.h"

static HMODULE hmscoree;

static HRESULT (WINAPI *pCLRCreateInstance)(REFCLSID clsid, REFIID riid, LPVOID *ppInterface);

static ICLRMetaHost *metahost;

BOOL init_pointers(void)
{
    HRESULT hr = E_FAIL;

    hmscoree = LoadLibraryA("mscoree.dll");

    if (hmscoree)
        pCLRCreateInstance = (void *)GetProcAddress(hmscoree, "CLRCreateInstance");

    if (pCLRCreateInstance)
        hr = pCLRCreateInstance(&CLSID_CLRMetaHost, &IID_ICLRMetaHost, (void**)&metahost);

    if (FAILED(hr))
    {
        todo_wine win_skip(".NET 4 is not installed\n");
        FreeLibrary(hmscoree);
        return FALSE;
    }

    return TRUE;
}

void cleanup(void)
{
    ICLRMetaHost_Release(metahost);

    FreeLibrary(hmscoree);
}

START_TEST(metahost)
{
    if (!init_pointers())
        return;

    cleanup();
}
