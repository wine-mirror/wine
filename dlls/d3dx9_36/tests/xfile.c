/*
 * Copyright 2012 Christian Costa
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
#include "d3dx9.h"
#include "d3dx9xof.h"

START_TEST(xfile)
{
    ID3DXFile *file;
    HRESULT ret;

    ret = D3DXFileCreate(NULL);
    ok(ret == E_POINTER, "D3DXCreateFile returned %#x, expected %#x\n", ret, E_POINTER);

    ret = D3DXFileCreate(&file);
    ok(ret == S_OK, "D3DXCreateFile failed with %#x\n", ret);

    file->lpVtbl->Release(file);
}
