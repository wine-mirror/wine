/*
 * Some unit tests for d3dxof
 *
 * Copyright (C) 2008 Christian Costa
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

#include <assert.h>
#include <stdio.h>
#include "wine/test.h"
#include "initguid.h"
#include "dxfile.h"

static inline void debugstr_guid( char* buf, CONST GUID *id )
{
    sprintf(buf, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
            id->Data1, id->Data2, id->Data3,
            id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3],
            id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7] );
}

static HMODULE hd3dxof;
static HRESULT (WINAPI *pDirectXFileCreate)(LPDIRECTXFILE*);

char template[] =
"xof 0302txt 0064\n"
"template Header\n"
"{\n"
"<3D82AB43-62DA-11CF-AB39-0020AF71E433>\n"
"WORD major;\n"
"WORD minor;\n"
"DWORD flags;\n"
"}\n";

char object[] =
"xof 0302txt 0064\n"
"Header Object\n"
"{\n"
"1; 2; 3;\n"
"}\n";

char empty_txt_file[]  = "xof 0302txt 0064";
char empty_bin_file[]  = "xof 0302bin 0064";
/* MSZip data is generated with the command "MAKECAB.EXE /D Compress=ON /D CompressionType=MSZip file packed"
 * Data in cab is after the filename (null terminated) and the 32-bit checksum:
 * size (16-bit), packed_size (16-bit) and compressed data (with leading 16-bit CK signature)
 * Data in x files is preceding by 2 16-bit words: size with xof header (16 bytes) and a 0 value
 * It does not seem possible to generate a MSZip data with no byte, so put just 1 byte here */
/* "\n" packed with MSZip => not text */
char empty_tzip_file[] = "xof 0302tzip0064\x11\x00\x00\x00\x01\x00\x05\x00\x43\x4b\xe3\x02\x00";
/* "\n" packed with MSZip => not token (token are 16-bit and there is only 1 byte) */
char empty_bzip_file[] = "xof 0302bzip0064\x11\x00\x00\x00\x01\x00\x05\x00\x43\x4b\xe3\x02\x00";
char empty_cmp_file[]  = "xof 0302cmp 0064";
char empty_xxxx_file[] = "xof 0302xxxx0064";

static void init_function_pointers(void)
{
    /* We have to use LoadLibrary as no d3dxof functions are referenced directly */
    hd3dxof = LoadLibraryA("d3dxof.dll");

    pDirectXFileCreate = (void *)GetProcAddress(hd3dxof, "DirectXFileCreate");
}

static ULONG getRefcount(IUnknown *iface)
{
    IUnknown_AddRef(iface);
    return IUnknown_Release(iface);
}

static void test_refcount(void)
{
    HRESULT hr;
    ULONG ref;
    LPDIRECTXFILE lpDirectXFile = NULL;
    LPDIRECTXFILEENUMOBJECT lpdxfeo;
    LPDIRECTXFILEDATA lpdxfd;
    DXFILELOADMEMORY dxflm;

    if (!pDirectXFileCreate)
    {
        win_skip("DirectXFileCreate is not available\n");
        return;
    }

    hr = pDirectXFileCreate(&lpDirectXFile);
    ok(hr == DXFILE_OK, "DirectXFileCreate: %x\n", hr);
    if(!lpDirectXFile)
    {
        skip("Couldn't create DirectXFile interface\n");
        return;
    }

    ref = getRefcount( (IUnknown *) lpDirectXFile);
    ok(ref == 1, "Got refcount %d, expected 1\n", ref);
    ref = IDirectXFile_AddRef(lpDirectXFile);
    ok(ref == 2, "Got refcount %d, expected 1\n", ref);
    ref = IDirectXFile_Release(lpDirectXFile);
    ok(ref == 1, "Got refcount %d, expected 1\n", ref);

    hr = IDirectXFile_RegisterTemplates(lpDirectXFile, template, strlen(template));
    ok(hr == DXFILE_OK, "IDirectXFileImpl_RegisterTemplates: %x\n", hr);

    dxflm.lpMemory = &object;
    dxflm.dSize = strlen(object);
    hr = IDirectXFile_CreateEnumObject(lpDirectXFile, &dxflm, DXFILELOAD_FROMMEMORY, &lpdxfeo);
    ok(hr == DXFILE_OK, "IDirectXFile_CreateEnumObject: %x\n", hr);
    ref = getRefcount( (IUnknown *) lpDirectXFile);
    ok(ref == 1, "Got refcount %d, expected 1\n", ref);
    ref = getRefcount( (IUnknown *) lpdxfeo);
    ok(ref == 1, "Got refcount %d, expected 1\n", ref);

    hr = IDirectXFileEnumObject_GetNextDataObject(lpdxfeo, &lpdxfd);
    ok(hr == DXFILE_OK, "IDirectXFileEnumObject_GetNextDataObject: %x\n", hr);
    ref = getRefcount( (IUnknown *) lpDirectXFile);
    ok(ref == 1, "Got refcount %d, expected 1\n", ref);
    ref = getRefcount( (IUnknown *) lpdxfeo);
    ok(ref == 1, "Got refcount %d, expected 1\n", ref);
    /* Enum object gets references to all top level objects */
    ref = getRefcount( (IUnknown *) lpdxfd);
    ok(ref == 2, "Got refcount %d, expected 2\n", ref);

    ref = IDirectXFile_Release(lpDirectXFile);
    ok(ref == 0, "Got refcount %d, expected 0\n", ref);
    /* Nothing changes for all other objects */
    ref = getRefcount( (IUnknown *) lpdxfeo);
    ok(ref == 1, "Got refcount %d, expected 1\n", ref);
    ref = getRefcount( (IUnknown *) lpdxfd);
    ok(ref == 2, "Got refcount %d, expected 1\n", ref);

    ref = IDirectXFileEnumObject_Release(lpdxfeo);
    ok(ref == 0, "Got refcount %d, expected 0\n", ref);
    /* Enum object releases references to all top level objects */
    ref = getRefcount( (IUnknown *) lpdxfd);
    ok(ref == 1, "Got refcount %d, expected 1\n", ref);

    ref = IDirectXFileData_Release(lpdxfd);
    ok(ref == 0, "Got refcount %d, expected 0\n", ref);
}

static void test_CreateEnumObject(void)
{
    HRESULT hr;
    ULONG ref;
    LPDIRECTXFILE lpDirectXFile = NULL;
    LPDIRECTXFILEENUMOBJECT lpdxfeo;
    LPDIRECTXFILEDATA lpdxfd;
    DXFILELOADMEMORY dxflm;
    BYTE* pdata;
    DWORD size;

    if (!pDirectXFileCreate)
    {
        win_skip("DirectXFileCreate is not available\n");
        return;
    }

    hr = pDirectXFileCreate(&lpDirectXFile);
    ok(hr == DXFILE_OK, "DirectXFileCreate: %x\n", hr);
    if(!lpDirectXFile)
    {
        skip("Couldn't create DirectXFile interface\n");
        return;
    }

    hr = IDirectXFile_RegisterTemplates(lpDirectXFile, template, strlen(template));
    ok(hr == DXFILE_OK, "IDirectXFileImpl_RegisterTemplates: %x\n", hr);

    dxflm.lpMemory = &object;
    dxflm.dSize = strlen(object);
    /* Check that only lowest 4 bits are relevant in DXFILELOADOPTIONS */
    hr = IDirectXFile_CreateEnumObject(lpDirectXFile, &dxflm, 0xFFFFFFF0 + DXFILELOAD_FROMMEMORY, &lpdxfeo);
    ok(hr == DXFILE_OK, "IDirectXFile_CreateEnumObject: %x\n", hr);

    hr = IDirectXFileEnumObject_GetNextDataObject(lpdxfeo, &lpdxfd);
    ok(hr == DXFILE_OK, "IDirectXFileEnumObject_GetNextDataObject: %x\n", hr);

    hr = IDirectXFileData_GetData(lpdxfd, NULL, &size, (void**)&pdata);
    ok(hr == DXFILE_OK, "IDirectXFileData_GetData: %x\n", hr);

    ok(size == 8, "Retrieved data size is wrong\n");
    ok((*((WORD*)pdata) == 1) && (*((WORD*)(pdata+2)) == 2) && (*((DWORD*)(pdata+4)) == 3), "Retrieved data is wrong\n");

    ref = IDirectXFileEnumObject_Release(lpdxfeo);
    ok(ref == 0, "Got refcount %d, expected 0\n", ref);

    ref = IDirectXFile_Release(lpDirectXFile);
    ok(ref == 0, "Got refcount %d, expected 0\n", ref);

    ref = IDirectXFileData_Release(lpdxfd);
    ok(ref == 0, "Got refcount %d, expected 0\n", ref);
}

static void test_file_types(void)
{
    HRESULT hr;
    LPDIRECTXFILE dxfile = NULL;
    LPDIRECTXFILEENUMOBJECT enum_object;
    DXFILELOADMEMORY lminfo;

    if (!pDirectXFileCreate)
    {
        win_skip("DirectXFileCreate is not available\n");
        return;
    }

    hr = pDirectXFileCreate(&dxfile);
    ok(hr == DXFILE_OK, "DirectXFileCreate: %x\n", hr);
    if (!dxfile)
    {
        skip("Couldn't create DirectXFile interface\n");
        return;
    }

    hr = IDirectXFile_RegisterTemplates(dxfile, empty_txt_file, strlen(empty_txt_file));
    ok(hr == DXFILE_OK, "IDirectXFileImpl_RegisterTemplates: %x\n", hr);

    hr = IDirectXFile_RegisterTemplates(dxfile, empty_bin_file, strlen(empty_bin_file));
    ok(hr == DXFILE_OK, "IDirectXFileImpl_RegisterTemplates: %x\n", hr);

    hr = IDirectXFile_RegisterTemplates(dxfile, empty_tzip_file, sizeof(empty_tzip_file) - 1);
    todo_wine ok(hr == DXFILE_OK, "IDirectXFileImpl_RegisterTemplates: %x\n", hr);

    hr = IDirectXFile_RegisterTemplates(dxfile, empty_bzip_file, sizeof(empty_bzip_file) - 1);
    todo_wine ok(hr == DXFILE_OK, "IDirectXFileImpl_RegisterTemplates: %x\n", hr);

    hr = IDirectXFile_RegisterTemplates(dxfile, empty_cmp_file, strlen(empty_cmp_file));
    ok(hr == DXFILEERR_BADFILETYPE, "IDirectXFileImpl_RegisterTemplates: %x\n", hr);

    hr = IDirectXFile_RegisterTemplates(dxfile, empty_xxxx_file, strlen(empty_xxxx_file));
    ok(hr == DXFILEERR_BADFILETYPE, "IDirectXFileImpl_RegisterTemplates: %x\n", hr);

    lminfo.lpMemory = empty_txt_file;
    lminfo.dSize = strlen(empty_txt_file);
    hr = IDirectXFile_CreateEnumObject(dxfile, &lminfo, DXFILELOAD_FROMMEMORY, &enum_object);
    ok(hr == DXFILE_OK, "IDirectXFile_CreateEnumObject: %x\n", hr);
    if (hr == DXFILE_OK) IDirectXFileEnumObject_Release(enum_object);

    lminfo.lpMemory = empty_bin_file;
    lminfo.dSize = strlen(empty_bin_file);
    hr = IDirectXFile_CreateEnumObject(dxfile, &lminfo, DXFILELOAD_FROMMEMORY, &enum_object);
    ok(hr == DXFILE_OK, "IDirectXFile_CreateEnumObject: %x\n", hr);
    if (hr == DXFILE_OK) IDirectXFileEnumObject_Release(enum_object);

    lminfo.lpMemory = empty_tzip_file;
    lminfo.dSize = sizeof(empty_tzip_file) - 1;
    hr = IDirectXFile_CreateEnumObject(dxfile, &lminfo, DXFILELOAD_FROMMEMORY, &enum_object);
    todo_wine ok(hr == DXFILE_OK, "IDirectXFile_CreateEnumObject: %x\n", hr);
    if (hr == DXFILE_OK) IDirectXFileEnumObject_Release(enum_object);

    lminfo.lpMemory = empty_bzip_file;
    lminfo.dSize = sizeof(empty_bzip_file) - 1;
    hr = IDirectXFile_CreateEnumObject(dxfile, &lminfo, DXFILELOAD_FROMMEMORY, &enum_object);
    todo_wine ok(hr == DXFILE_OK, "IDirectXFile_CreateEnumObject: %x\n", hr);
    if (hr == DXFILE_OK) IDirectXFileEnumObject_Release(enum_object);

    lminfo.lpMemory = empty_cmp_file;
    lminfo.dSize = strlen(empty_cmp_file);
    hr = IDirectXFile_CreateEnumObject(dxfile, &lminfo, DXFILELOAD_FROMMEMORY, &enum_object);
    ok(hr == DXFILEERR_BADFILETYPE, "IDirectXFile_CreateEnumObject: %x\n", hr);

    lminfo.lpMemory = empty_xxxx_file;
    lminfo.dSize = strlen(empty_xxxx_file);
    hr = IDirectXFile_CreateEnumObject(dxfile, &lminfo, DXFILELOAD_FROMMEMORY, &enum_object);
    ok(hr == DXFILEERR_BADFILETYPE, "IDirectXFile_CreateEnumObject: %x\n", hr);

    IDirectXFile_Release(dxfile);
}

/* Set it to 1 to expand the string when dumping the object. This is useful when there is
 * only one string in a sub-object (very common). Use with care, this may lead to a crash. */
#define EXPAND_STRING 0

static void process_data(LPDIRECTXFILEDATA lpDirectXFileData, int* plevel)
{
    HRESULT hr;
    char name[100];
    GUID clsid;
    CONST GUID* clsid_type = NULL;
    char str_clsid[40];
    char str_clsid_type[40];
    DWORD len= 100;
    LPDIRECTXFILEOBJECT pChildObj;
    int i;
    int j = 0;
    LPBYTE pData;
    DWORD k, size;

    hr = IDirectXFileData_GetId(lpDirectXFileData, &clsid);
    ok(hr == DXFILE_OK, "IDirectXFileData_GetId: %x\n", hr);
    hr = IDirectXFileData_GetName(lpDirectXFileData, name, &len);
    ok(hr == DXFILE_OK, "IDirectXFileData_GetName: %x\n", hr);
    hr = IDirectXFileData_GetType(lpDirectXFileData, &clsid_type);
    ok(hr == DXFILE_OK, "IDirectXFileData_GetType: %x\n", hr);
    hr = IDirectXFileData_GetData(lpDirectXFileData, NULL, &size, (void**)&pData);
    ok(hr == DXFILE_OK, "IDirectXFileData_GetData: %x\n", hr);
    for (i = 0; i < *plevel; i++)
        printf("  ");
    debugstr_guid(str_clsid, &clsid);
    debugstr_guid(str_clsid_type, clsid_type);
    printf("Found object '%s' - %s - %s - %d\n", name, str_clsid, str_clsid_type, size);

    if (EXPAND_STRING && size == 4)
    {
        char * str = *(char**)pData;
        printf("string %s\n", str);
    }
    else if (size)
    {
        for (k = 0; k < size; k++)
        {
            if (k && !(k%16))
                printf("\n");
            printf("%02x ", pData[k]);
        }
        printf("\n");
    }
    (*plevel)++;
    while (SUCCEEDED(hr = IDirectXFileData_GetNextObject(lpDirectXFileData, &pChildObj)))
    {
        LPDIRECTXFILEDATA p1;
        LPDIRECTXFILEDATAREFERENCE p2;
        LPDIRECTXFILEBINARY p3;
        j++;

        hr = IDirectXFileObject_QueryInterface(pChildObj, &IID_IDirectXFileData, (void **) &p1);
        if (SUCCEEDED(hr))
        {
            for (i = 0; i < *plevel; i++)
                printf("  ");
            printf("Found Data (%d)\n", j);
            process_data(p1, plevel);
            IDirectXFileData_Release(p1);
        }
        hr = IDirectXFileObject_QueryInterface(pChildObj, &IID_IDirectXFileDataReference, (void **) &p2);
        if (SUCCEEDED(hr))
        {
            LPDIRECTXFILEDATA pfdo;
            for (i = 0; i < *plevel; i++)
                printf("  ");
            printf("Found Data Reference (%d)\n", j);
#if 0
            hr = IDirectXFileDataReference_GetId(lpDirectXFileData, &clsid);
            ok(hr == DXFILE_OK, "IDirectXFileData_GetId: %x\n", hr);
            hr = IDirectXFileDataReference_GetName(lpDirectXFileData, name, &len);
            ok(hr == DXFILE_OK, "IDirectXFileData_GetName: %x\n", hr);
#endif
            IDirectXFileDataReference_Resolve(p2, &pfdo);
            process_data(pfdo, plevel);
            IDirectXFileData_Release(pfdo);
            IDirectXFileDataReference_Release(p2);
        }
        hr = IDirectXFileObject_QueryInterface(pChildObj, &IID_IDirectXFileBinary, (void **) &p3);
        if (SUCCEEDED(hr))
        {
            for (i = 0; i < *plevel; i++)
                printf("  ");
            printf("Found Binary (%d)\n", j);
            IDirectXFileBinary_Release(p3);
        }
    }
    (*plevel)--;
    ok(hr == DXFILE_OK || hr == DXFILEERR_NOMOREOBJECTS, "IDirectXFileData_GetNextObject: %x\n", hr);
}

static void test_dump(void)
{
    HRESULT hr;
    ULONG ref;
    LPDIRECTXFILE lpDirectXFile = NULL;
    LPDIRECTXFILEENUMOBJECT lpDirectXFileEnumObject = NULL;
    LPDIRECTXFILEDATA lpDirectXFileData = NULL;
    HANDLE hFile;
    LPVOID pvData = NULL;
    DWORD cbSize;

    if (!pDirectXFileCreate)
    {
        win_skip("DirectXFileCreate is not available\n");
        goto exit;
    }

    /* Dump data only if there is an object and a template */
    hFile = CreateFileA("objects.txt", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
      return;
    CloseHandle(hFile);

    hFile = CreateFileA("templates.txt", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
      return;

    pvData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 10000);

    if (!ReadFile(hFile, pvData, 10000, &cbSize, NULL))
    {
      skip("Template file is too big\n");
      goto exit;
    }

    printf("Load %d bytes\n", cbSize);

    hr = pDirectXFileCreate(&lpDirectXFile);
    ok(hr == DXFILE_OK, "DirectXFileCreate: %x\n", hr);
    if(!lpDirectXFile)
    {
        skip("Couldn't create DirectXFile interface\n");
        goto exit;
    }

    hr = IDirectXFile_RegisterTemplates(lpDirectXFile, pvData, strlen(pvData));
    ok(hr == DXFILE_OK, "IDirectXFileImpl_RegisterTemplates: %x\n", hr);

    hr = IDirectXFile_CreateEnumObject(lpDirectXFile, (LPVOID)"objects.txt", DXFILELOAD_FROMFILE, &lpDirectXFileEnumObject);
    ok(hr == DXFILE_OK, "IDirectXFile_CreateEnumObject: %x\n", hr);

    while (SUCCEEDED(hr = IDirectXFileEnumObject_GetNextDataObject(lpDirectXFileEnumObject, &lpDirectXFileData)))
    {
	int level = 0;
        printf("\n");
	process_data(lpDirectXFileData, &level);
        IDirectXFileData_Release(lpDirectXFileData);
    }
    ok(hr == DXFILE_OK || hr == DXFILEERR_NOMOREOBJECTS, "IDirectXFileEnumObject_GetNextDataObject: %x\n", hr);

    ref = IDirectXFile_Release(lpDirectXFileEnumObject);
    ok(ref == 0, "Got refcount %d, expected 0\n", ref);

    ref = IDirectXFile_Release(lpDirectXFile);
    ok(ref == 0, "Got refcount %d, expected 0\n", ref);

    CloseHandle(hFile);

exit:
    HeapFree(GetProcessHeap(), 0, pvData);
}

START_TEST(d3dxof)
{
    init_function_pointers();

    test_refcount();
    test_CreateEnumObject();
    test_file_types();
    test_dump();

    FreeLibrary(hd3dxof);
}
