/*
 * Copyright 2025 Piotr Caban
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

#include "roapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(vccorlib);

HRESULT __cdecl InitializeData(int type)
{
    HRESULT hres;

    FIXME("(%d) semi-stub\n", type);

    if (!type) return S_OK;

    hres = RoInitialize(type == 1 ? RO_INIT_SINGLETHREADED : RO_INIT_MULTITHREADED);
    if (FAILED(hres)) return hres;
    return S_OK;
}

void __cdecl UninitializeData(int type)
{
    TRACE("(%d)\n", type);

    if (type) RoUninitialize();
}
