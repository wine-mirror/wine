/*
 * Copyright (C) 2006 Louis Lenders
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

static HRESULT (WINAPI *ValidateVertexShader)(DWORD*,DWORD*,DWORD*,int,DWORD*);

static void test_ValidateVertexShader(void)
{
    HRESULT ret;
    static DWORD simple_vs[] = {0xFFFE0101,             /* vs_1_1               */
        0x00000009, 0xC0010000, 0x90E40000, 0xA0E40000, /* dp4 oPos.x, v0, c0   */
        0x00000009, 0xC0020000, 0x90E40000, 0xA0E40001, /* dp4 oPos.y, v0, c1   */
        0x00000009, 0xC0040000, 0x90E40000, 0xA0E40002, /* dp4 oPos.z, v0, c2   */
        0x00000009, 0xC0080000, 0x90E40000, 0xA0E40003, /* dp4 oPos.w, v0, c3   */
        0x0000FFFF};

    ret=ValidateVertexShader(0,0,0,0,0);
    ok(ret==E_FAIL,"ValidateVertexShader returned %lx but expected E_FAIL\n",ret);

    ret=ValidateVertexShader(0,0,0,1,0);
    ok(ret==E_FAIL,"ValidateVertexShader returned %lx but expected E_FAIL\n",ret);

    ret=ValidateVertexShader(simple_vs,0,0,0,0);
    ok(ret==S_OK,"ValidateVertexShader returned %lx but expected S_OK\n",ret);

    ret=ValidateVertexShader(simple_vs,0,0,1,0);
    ok(ret==S_OK,"ValidateVertexShader returned %lx but expected S_OK\n",ret);
    /* seems to do some version checking */
    *simple_vs=0xFFFE0100;             /* vs_1_0               */
    ret=ValidateVertexShader(simple_vs,0,0,0,0);
    ok(ret==S_OK,"ValidateVertexShader returned %lx but expected S_OK\n",ret);

    *simple_vs=0xFFFE0102;            /* bogus version         */
    ret=ValidateVertexShader(simple_vs,0,0,1,0);
    ok(ret==E_FAIL,"ValidateVertexShader returned %lx but expected E_FAIL\n",ret);
    /* I've seen that applications pass 2nd and 3rd parameter always as 0;simple test with non-zero parameters */
    *simple_vs=0xFFFE0101;             /* vs_1_1               */
    ret=ValidateVertexShader(simple_vs,simple_vs,0,1,0);
    ok(ret==E_FAIL,"ValidateVertexShader returned %lx but expected E_FAIL\n",ret);

    ret=ValidateVertexShader(simple_vs,0,simple_vs,1,0);
    ok(ret==E_FAIL,"ValidateVertexShader returned %lx but expected E_FAIL\n",ret);
    /* I've seen 4th parameter is always passed as either 0 or 1, but passing other values doesn't seem to hurt*/
    ret=ValidateVertexShader(simple_vs,0,0,12345,0);
    ok(ret==S_OK,"ValidateVertexShader returned %lx but expected S_OK\n",ret);
    /* What is 5th parameter ???? Following works ok */
    ret=ValidateVertexShader(simple_vs,0,0,1,simple_vs);
    ok(ret==S_OK,"ValidateVertexShader returned %lx but expected S_OK\n",ret);
}

START_TEST(d3d8_main)
{
    HMODULE d3d8_handle = LoadLibraryA( "d3d8.dll" );
    ValidateVertexShader = (void*)GetProcAddress (d3d8_handle, "ValidateVertexShader" );
    test_ValidateVertexShader();
}
