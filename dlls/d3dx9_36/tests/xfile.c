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

char templates_bad_file_type1[] =
"xOf 0302txt 0064\n";

char templates_bad_file_version[] =
"xof 0102txt 0064\n";

char templates_bad_file_type2[] =
"xof 0302foo 0064\n";

char templates_bad_file_float_size[] =
"xof 0302txt 0050\n";

char templates_parse_error[] =
"xof 0302txt 0064"
"foobar;\n";

char templates[] =
"xof 0302txt 0064"
"template Header"
"{"
"<3D82AB43-62DA-11CF-AB39-0020AF71E433>"
"WORD major;"
"WORD minor;"
"DWORD flags;"
"}\n";


void test_templates(void)
{
    ID3DXFile *d3dxfile;
    HRESULT ret;

    ret = D3DXFileCreate(NULL);
    ok(ret == E_POINTER, "D3DXCreateFile returned %#x, expected %#x\n", ret, E_POINTER);

    ret = D3DXFileCreate(&d3dxfile);
    ok(ret == S_OK, "D3DXCreateFile failed with %#x\n", ret);

    ret = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, (const void *)templates_bad_file_type1, (SIZE_T)(sizeof(templates_bad_file_type1) - 1));
    ok(ret == D3DXFERR_BADFILETYPE, "RegisterTemplates returned %#x, expected %#x\n", ret, D3DXFERR_BADFILETYPE);

    ret = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, (const void *)templates_bad_file_version, (SIZE_T)(sizeof(templates_bad_file_version) - 1));
    ok(ret == D3DXFERR_BADFILEVERSION, "RegisterTemplates returned %#x, expected %#x\n", ret, D3DXFERR_BADFILEVERSION);

    ret = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, (const void *)templates_bad_file_type2, (SIZE_T)(sizeof(templates_bad_file_type2) - 1));
    ok(ret == D3DXFERR_BADFILETYPE, "RegisterTemplates returned %#x, expected %#x\n", ret, D3DXFERR_BADFILETYPE);

    ret = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, (const void *)templates_bad_file_float_size, (SIZE_T)(sizeof(templates_bad_file_float_size) - 1));
    ok(ret == D3DXFERR_BADFILEFLOATSIZE, "RegisterTemplates returned %#x, expected %#x\n", ret, D3DXFERR_BADFILEFLOATSIZE);

    ret = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, (const void *)templates_parse_error, (SIZE_T)(sizeof(templates_parse_error) - 1));
    todo_wine ok(ret == D3DXFERR_PARSEERROR, "RegisterTemplates returned %#x, expected %#x\n", ret, D3DXFERR_PARSEERROR);

    ret = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, (const void *)templates, (SIZE_T)(sizeof(templates) - 1));
    ok(ret == S_OK, "RegisterTemplates with %#x\n", ret);

    d3dxfile->lpVtbl->Release(d3dxfile);
}


START_TEST(xfile)
{
    test_templates();
}
