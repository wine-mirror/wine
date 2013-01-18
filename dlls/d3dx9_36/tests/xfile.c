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

static const char templates_bad_file_type1[] =
"xOf 0302txt 0064\n";

static const char templates_bad_file_version[] =
"xof 0102txt 0064\n";

static const char templates_bad_file_type2[] =
"xof 0302foo 0064\n";

static const char templates_bad_file_float_size[] =
"xof 0302txt 0050\n";

static const char templates_parse_error[] =
"xof 0302txt 0064"
"foobar;\n";

static const char templates[] =
"xof 0302txt 0064"
"template Header"
"{"
"<3D82AB43-62DA-11CF-AB39-0020AF71E433>"
"WORD major;"
"WORD minor;"
"DWORD flags;"
"}\n";

static char objects[] =
"xof 0302txt 0064\n"
"Header Object\n"
"{\n"
"1; 2; 3;\n"
"}\n";

static void test_templates(void)
{
    ID3DXFile *d3dxfile;
    HRESULT ret;

    ret = D3DXFileCreate(NULL);
    ok(ret == E_POINTER, "D3DXCreateFile returned %#x, expected %#x\n", ret, E_POINTER);

    ret = D3DXFileCreate(&d3dxfile);
    ok(ret == S_OK, "D3DXCreateFile failed with %#x\n", ret);

    ret = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, templates_bad_file_type1, sizeof(templates_bad_file_type1) - 1);
    ok(ret == D3DXFERR_BADFILETYPE, "RegisterTemplates returned %#x, expected %#x\n", ret, D3DXFERR_BADFILETYPE);

    ret = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, templates_bad_file_version, sizeof(templates_bad_file_version) - 1);
    ok(ret == D3DXFERR_BADFILEVERSION, "RegisterTemplates returned %#x, expected %#x\n", ret, D3DXFERR_BADFILEVERSION);

    ret = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, templates_bad_file_type2, sizeof(templates_bad_file_type2) - 1);
    ok(ret == D3DXFERR_BADFILETYPE, "RegisterTemplates returned %#x, expected %#x\n", ret, D3DXFERR_BADFILETYPE);

    ret = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, templates_bad_file_float_size, sizeof(templates_bad_file_float_size) - 1);
    ok(ret == D3DXFERR_BADFILEFLOATSIZE, "RegisterTemplates returned %#x, expected %#x\n", ret, D3DXFERR_BADFILEFLOATSIZE);

    ret = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, templates_parse_error, sizeof(templates_parse_error) - 1);
    todo_wine ok(ret == D3DXFERR_PARSEERROR, "RegisterTemplates returned %#x, expected %#x\n", ret, D3DXFERR_PARSEERROR);

    ret = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, templates, sizeof(templates) - 1);
    ok(ret == S_OK, "RegisterTemplates failed with %#x\n", ret);

    d3dxfile->lpVtbl->Release(d3dxfile);
}

static void test_lock_unlock(void)
{
    ID3DXFile *d3dxfile;
    D3DXF_FILELOADMEMORY memory;
    ID3DXFileEnumObject *enum_object;
    ID3DXFileData *data_object;
    const void *data;
    SIZE_T size;
    HRESULT ret;

    ret = D3DXFileCreate(&d3dxfile);
    ok(ret == S_OK, "D3DXCreateFile failed with %#x\n", ret);

    ret = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, templates, sizeof(templates) - 1);
    ok(ret == S_OK, "RegisterTemplates failed with %#x\n", ret);

    memory.lpMemory = objects;
    memory.dSize = sizeof(objects) - 1;

    ret = d3dxfile->lpVtbl->CreateEnumObject(d3dxfile, &memory, D3DXF_FILELOAD_FROMMEMORY, &enum_object);
    ok(ret == S_OK, "CreateEnumObject failed with %#x\n", ret);

    ret = enum_object->lpVtbl->GetChild(enum_object, 0, &data_object);
    ok(ret == S_OK, "GetChild failed with %#x\n", ret);

    ret = data_object->lpVtbl->Unlock(data_object);
    ok(ret == S_OK, "Unlock failed with %#x\n", ret);
    ret = data_object->lpVtbl->Lock(data_object, &size, &data);
    ok(ret == S_OK, "Lock failed with %#x\n", ret);
    ret = data_object->lpVtbl->Lock(data_object, &size, &data);
    ok(ret == S_OK, "Lock failed with %#x\n", ret);
    ret = data_object->lpVtbl->Unlock(data_object);
    ok(ret == S_OK, "Unlock failed with %#x\n", ret);
    ret = data_object->lpVtbl->Unlock(data_object);
    ok(ret == S_OK, "Unlock failed with %#x\n", ret);

    data_object->lpVtbl->Release(data_object);
    enum_object->lpVtbl->Release(enum_object);
    d3dxfile->lpVtbl->Release(d3dxfile);
}

START_TEST(xfile)
{
    test_templates();
    test_lock_unlock();
}
