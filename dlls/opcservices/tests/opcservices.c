/*
 *    OPC Services tests
 *
 * Copyright 2018 Nikolay Sivov for CodeWeavers
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

#include "windows.h"
#include "initguid.h"
#include "msopc.h"
#include "urlmon.h"

#include "wine/test.h"

static IOpcFactory *create_factory(void)
{
    IOpcFactory *factory = NULL;
    CoCreateInstance(&CLSID_OpcFactory, NULL, CLSCTX_INPROC_SERVER, &IID_IOpcFactory, (void **)&factory);
    return factory;
}

static void test_package(void)
{
    static const WCHAR typeW[] = L"type/subtype";
    IOpcRelationshipSet *relset, *relset2;
    IOpcPartUri *part_uri, *part_uri2;
    IOpcPartSet *partset, *partset2;
    OPC_COMPRESSION_OPTIONS options;
    IStream *stream, *stream2;
    IOpcPart *part, *part2;
    IOpcRelationship *rel;
    IOpcFactory *factory;
    IOpcPackage *package;
    LARGE_INTEGER move;
    ULARGE_INTEGER pos;
    IUri *target_uri;
    char buff[16];
    IOpcUri *uri;
    HRESULT hr;
    BSTR str;
    BOOL ret;

    factory = create_factory();

    hr = IOpcFactory_CreatePackage(factory, &package);
    ok(hr == S_OK, "Failed to create a package, hr %#lx.\n", hr);

    hr = IOpcPackage_GetPartSet(package, &partset);
    ok(SUCCEEDED(hr), "Failed to create a part set, hr %#lx.\n", hr);

    hr = IOpcPackage_GetPartSet(package, &partset2);
    ok(SUCCEEDED(hr), "Failed to create a part set, hr %#lx.\n", hr);
    ok(partset == partset2, "Expected same part set instance.\n");
    IOpcPartSet_Release(partset2);

    /* CreatePart */
    hr = IOpcFactory_CreatePartUri(factory, L"/uri", &part_uri);
    ok(SUCCEEDED(hr), "Failed to create part uri, hr %#lx.\n", hr);

    hr = IOpcFactory_CreatePartUri(factory, L"/uri", &part_uri2);
    ok(SUCCEEDED(hr), "Failed to create part uri, hr %#lx.\n", hr);

    part = (void *)0xdeadbeef;
    hr = IOpcPartSet_CreatePart(partset, NULL, typeW, OPC_COMPRESSION_NONE, &part);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    ok(part == NULL, "Unexpected pointer %p.\n", part);

    hr = IOpcPartSet_CreatePart(partset, part_uri, typeW, OPC_COMPRESSION_NONE, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IOpcPartSet_CreatePart(partset, part_uri, typeW, 0xdeadbeef, &part);
    ok(SUCCEEDED(hr), "Failed to create a part, hr %#lx.\n", hr);
    hr = IOpcPart_GetCompressionOptions(part, &options);
    ok(SUCCEEDED(hr), "Failed to get compression options, hr %#lx.\n", hr);
    ok(options == 0xdeadbeef, "Unexpected compression options %#x.\n", options);

    part2 = (void *)0xdeadbeef;
    hr = IOpcPartSet_CreatePart(partset, part_uri, typeW, OPC_COMPRESSION_NONE, &part2);
    ok(hr == OPC_E_DUPLICATE_PART, "Unexpected hr %#lx.\n", hr);
    ok(part2 == NULL, "Unexpected instance %p.\n", part2);

    part2 = (void *)0xdeadbeef;
    hr = IOpcPartSet_CreatePart(partset, part_uri2, typeW, OPC_COMPRESSION_NONE, &part2);
    ok(hr == OPC_E_DUPLICATE_PART, "Unexpected hr %#lx.\n", hr);
    ok(part2 == NULL, "Unexpected instance %p.\n", part2);
    IOpcPartUri_Release(part_uri2);

    hr = IOpcPartSet_GetPart(partset, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    part2 = (void *)0xdeadbeef;
    hr = IOpcPartSet_GetPart(partset, NULL, &part2);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    ok(part2 == NULL, "Unexpected pointer %p.\n", part2);

    hr = IOpcPartSet_GetPart(partset, part_uri, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IOpcPartSet_GetPart(partset, part_uri, &part2);
    ok(SUCCEEDED(hr), "Failed to get part, hr %#lx.\n", hr);
    IOpcPart_Release(part2);

    hr = IOpcFactory_CreatePartUri(factory, L"target", &part_uri2);
    ok(SUCCEEDED(hr), "Failed to create part uri, hr %#lx.\n", hr);

    hr = IOpcPartSet_GetPart(partset, part_uri2, &part2);
    ok(hr == OPC_E_NO_SUCH_PART, "Unexpected hr %#lx.\n", hr);
    IOpcPartUri_Release(part_uri2);

    hr = IOpcPart_GetContentStream(part, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IOpcPart_GetContentStream(part, &stream);
    ok(SUCCEEDED(hr), "Failed to get content stream, hr %#lx.\n", hr);

    hr = IStream_Write(stream, "abc", 3, NULL);
    ok(hr == S_OK, "Failed to write content, hr %#lx.\n", hr);

    move.QuadPart = 0;
    hr = IStream_Seek(stream, move, STREAM_SEEK_CUR, &pos);
    ok(SUCCEEDED(hr), "Seek failed, hr %#lx.\n", hr);
    ok(pos.QuadPart == 3, "Unexpected position.\n");

    hr = IOpcPart_GetContentStream(part, &stream2);
    ok(SUCCEEDED(hr), "Failed to get content stream, hr %#lx.\n", hr);
    ok(stream != stream2, "Unexpected instance.\n");

    move.QuadPart = 0;
    hr = IStream_Seek(stream2, move, STREAM_SEEK_CUR, &pos);
    ok(SUCCEEDED(hr), "Seek failed, hr %#lx.\n", hr);
    ok(pos.QuadPart == 0, "Unexpected position.\n");

    memset(buff, 0, sizeof(buff));
    hr = IStream_Read(stream2, buff, sizeof(buff), NULL);
    ok(hr == S_OK, "Failed to read content, hr %#lx.\n", hr);
    ok(!memcmp(buff, "abc", 3), "Unexpected content.\n");

    move.QuadPart = 0;
    hr = IStream_Seek(stream2, move, STREAM_SEEK_CUR, &pos);
    ok(SUCCEEDED(hr), "Seek failed, hr %#lx.\n", hr);
    ok(pos.QuadPart == 3, "Unexpected position.\n");

    IStream_Release(stream);
    IStream_Release(stream2);

    hr = IOpcPart_GetRelationshipSet(part, &relset);
    ok(SUCCEEDED(hr), "Failed to get relationship set, hr %#lx.\n", hr);

    hr = IOpcPart_GetRelationshipSet(part, &relset2);
    ok(SUCCEEDED(hr), "Failed to get relationship set, hr %#lx.\n", hr);
    ok(relset == relset2, "Expected same part set instance.\n");

    hr = CreateUri(L"target", Uri_CREATE_ALLOW_RELATIVE, 0, &target_uri);
    ok(SUCCEEDED(hr), "Failed to create target uri, hr %#lx.\n", hr);

    hr = IOpcRelationshipSet_CreateRelationship(relset, NULL, typeW, target_uri, OPC_URI_TARGET_MODE_INTERNAL, &rel);
    ok(SUCCEEDED(hr), "Failed to create relationship, hr %#lx.\n", hr);

    hr = IOpcRelationship_GetSourceUri(rel, &uri);
    ok(SUCCEEDED(hr), "Failed to get source uri, hr %#lx.\n", hr);
    ok(uri == (IOpcUri *)part_uri, "Unexpected source uri.\n");

    IOpcUri_Release(uri);

    IOpcRelationship_Release(rel);
    IUri_Release(target_uri);

    IOpcRelationshipSet_Release(relset);
    IOpcRelationshipSet_Release(relset2);

    ret = 123;
    hr = IOpcPartSet_PartExists(partset, NULL, &ret);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    ok(ret == 123, "Unexpected return value.\n");

    hr = IOpcPartSet_PartExists(partset, part_uri, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    ret = FALSE;
    hr = IOpcPartSet_PartExists(partset, part_uri, &ret);
    ok(SUCCEEDED(hr), "Unexpected hr %#lx.\n", hr);
    ok(ret, "Expected part to exist.\n");

    IOpcPartUri_Release(part_uri);
    IOpcPart_Release(part);

    /* Relationships */
    hr = IOpcPackage_GetRelationshipSet(package, &relset);
    ok(SUCCEEDED(hr), "Failed to get relationship set, hr %#lx.\n", hr);

    hr = IOpcPackage_GetRelationshipSet(package, &relset2);
    ok(SUCCEEDED(hr), "Failed to get relationship set, hr %#lx.\n", hr);
    ok(relset == relset2, "Expected same part set instance.\n");
    IOpcRelationshipSet_Release(relset);
    IOpcRelationshipSet_Release(relset2);

    IOpcPartSet_Release(partset);
    IOpcPackage_Release(package);

    /* Root uri */
    hr = IOpcFactory_CreatePackageRootUri(factory, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    hr = IOpcFactory_CreatePackageRootUri(factory, &uri);
    ok(SUCCEEDED(hr), "Failed to create root uri, hr %#lx.\n", hr);
    hr = IOpcUri_GetRawUri(uri, &str);
    ok(SUCCEEDED(hr), "Failed to get raw uri, hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"/"), "Unexpected uri %s.\n", wine_dbgstr_w(str));
    SysFreeString(str);
    IOpcUri_Release(uri);

    IOpcFactory_Release(factory);
}

#define test_stream_stat(stream, size) test_stream_stat_(__LINE__, stream, size)
static void test_stream_stat_(unsigned int line, IStream *stream, ULONG size)
{
    STATSTG statstg;
    HRESULT hr;

    hr = IStream_Stat(stream, NULL, 0);
    ok_(__FILE__, line)(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    memset(&statstg, 0xff, sizeof(statstg));
    hr = IStream_Stat(stream, &statstg, 0);
    ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to get stat info, hr %#lx.\n", hr);

    ok_(__FILE__, line)(statstg.pwcsName == NULL, "Unexpected name %s.\n", wine_dbgstr_w(statstg.pwcsName));
    ok_(__FILE__, line)(statstg.type == STGTY_STREAM, "Unexpected type.\n");
    ok_(__FILE__, line)(statstg.cbSize.QuadPart == size, "Unexpected size %lu, expected %lu.\n",
            statstg.cbSize.LowPart, size);
    ok_(__FILE__, line)(statstg.grfMode == STGM_READ, "Unexpected mode.\n");
    ok_(__FILE__, line)(statstg.grfLocksSupported == 0, "Unexpected lock mode.\n");
    ok_(__FILE__, line)(statstg.grfStateBits == 0, "Unexpected state bits.\n");
}

static void test_file_stream(void)
{
    WCHAR temppathW[MAX_PATH], pathW[MAX_PATH];
    IOpcFactory *factory;
    LARGE_INTEGER move;
    IStream *stream;
    char buff[64];
    HRESULT hr;
    ULONG size;

    factory = create_factory();

    hr = IOpcFactory_CreateStreamOnFile(factory, NULL, OPC_STREAM_IO_READ, NULL, 0, &stream);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    GetTempPathW(ARRAY_SIZE(temppathW), temppathW);
    lstrcpyW(pathW, temppathW);
    lstrcatW(pathW, L"opcfileread.ext");
    DeleteFileW(pathW);

    hr = IOpcFactory_CreateStreamOnFile(factory, pathW, OPC_STREAM_IO_READ, NULL, 0, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    /* File does not exist */
    hr = IOpcFactory_CreateStreamOnFile(factory, pathW, OPC_STREAM_IO_READ, NULL, 0, &stream);
    ok(FAILED(hr), "Unexpected hr %#lx.\n", hr);

    hr = IOpcFactory_CreateStreamOnFile(factory, pathW, OPC_STREAM_IO_WRITE, NULL, 0, &stream);
    ok(SUCCEEDED(hr), "Failed to create a write stream, hr %#lx.\n", hr);

    test_stream_stat(stream, 0);

    size = lstrlenW(pathW) * sizeof(WCHAR);
    hr = IStream_Write(stream, pathW, size, NULL);
    ok(hr == S_OK, "Stream write failed, hr %#lx.\n", hr);

    test_stream_stat(stream, size);
    IStream_Release(stream);

    /* Invalid I/O mode */
    hr = IOpcFactory_CreateStreamOnFile(factory, pathW, 10, NULL, 0, &stream);
    ok(hr == E_INVALIDARG, "Failed to create a write stream, hr %#lx.\n", hr);

    /* Write to read-only stream. */
    hr = IOpcFactory_CreateStreamOnFile(factory, pathW, OPC_STREAM_IO_READ, NULL, 0, &stream);
    ok(SUCCEEDED(hr), "Failed to create a read stream, hr %#lx.\n", hr);

    test_stream_stat(stream, size);
    hr = IStream_Write(stream, pathW, size, NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED), "Stream write failed, hr %#lx.\n", hr);
    IStream_Release(stream);

    /* Read from write-only stream. */
    hr = IOpcFactory_CreateStreamOnFile(factory, pathW, OPC_STREAM_IO_WRITE, NULL, 0, &stream);
    ok(SUCCEEDED(hr), "Failed to create a read stream, hr %#lx.\n", hr);

    test_stream_stat(stream, 0);
    hr = IStream_Write(stream, pathW, size, NULL);
    ok(hr == S_OK, "Stream write failed, hr %#lx.\n", hr);
    test_stream_stat(stream, size);

    move.QuadPart = 0;
    hr = IStream_Seek(stream, move, STREAM_SEEK_SET, NULL);
    ok(SUCCEEDED(hr), "Seek failed, hr %#lx.\n", hr);

    hr = IStream_Read(stream, buff, sizeof(buff), NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED), "Stream read failed, hr %#lx.\n", hr);

    IStream_Release(stream);

    IOpcFactory_Release(factory);
    DeleteFileW(pathW);
}

static void test_relationship(void)
{
    static const WCHAR typeW[] = L"type";
    IUri *target_uri, *target_uri2, *uri;
    IOpcRelationship *rel, *rel2, *rel3;
    IOpcUri *source_uri, *source_uri2;
    IOpcRelationshipSet *rels;
    OPC_URI_TARGET_MODE mode;
    IOpcFactory *factory;
    IOpcPackage *package;
    IUnknown *unk;
    HRESULT hr;
    WCHAR *id;
    BOOL ret;
    BSTR str;

    factory = create_factory();

    hr = IOpcFactory_CreatePackage(factory, &package);
    ok(hr == S_OK, "Failed to create a package, hr %#lx.\n", hr);

    hr = CreateUri(L"target", Uri_CREATE_ALLOW_RELATIVE, 0, &target_uri);
    ok(SUCCEEDED(hr), "Failed to create target uri, hr %#lx.\n", hr);

    hr = CreateUri(L"file://host/file.txt", 0, 0, &target_uri2);
    ok(SUCCEEDED(hr), "Failed to create target uri, hr %#lx.\n", hr);

    hr = IOpcPackage_GetRelationshipSet(package, &rels);
    ok(SUCCEEDED(hr), "Failed to get part set, hr %#lx.\n", hr);

    rel = (void *)0xdeadbeef;
    hr = IOpcRelationshipSet_CreateRelationship(rels, NULL, NULL, NULL, OPC_URI_TARGET_MODE_INTERNAL, &rel);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    ok(rel == NULL, "Unexpected instance %p.\n", rel);

    rel = (void *)0xdeadbeef;
    hr = IOpcRelationshipSet_CreateRelationship(rels, NULL, typeW, NULL, OPC_URI_TARGET_MODE_INTERNAL, &rel);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    ok(rel == NULL, "Unexpected instance %p.\n", rel);

    hr = IOpcRelationshipSet_CreateRelationship(rels, NULL, NULL, target_uri, OPC_URI_TARGET_MODE_INTERNAL, &rel);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    /* Absolute target uri with internal mode */
    rel = (void *)0xdeadbeef;
    hr = IOpcRelationshipSet_CreateRelationship(rels, NULL, typeW, target_uri2, OPC_URI_TARGET_MODE_INTERNAL, &rel);
    ok(hr == OPC_E_INVALID_RELATIONSHIP_TARGET, "Unexpected hr %#lx.\n", hr);
    ok(rel == NULL, "Unexpected instance %p.\n", rel);

    hr = IOpcRelationshipSet_CreateRelationship(rels, NULL, typeW, target_uri, OPC_URI_TARGET_MODE_INTERNAL, &rel);
    ok(SUCCEEDED(hr), "Failed to create relationship, hr %#lx.\n", hr);

    /* Autogenerated relationship id */
    hr = IOpcRelationship_GetId(rel, &id);
    ok(SUCCEEDED(hr), "Failed to get id, hr %#lx.\n", hr);
    ok(lstrlenW(id) == 9 && *id == 'R', "Unexpected relationship id %s.\n", wine_dbgstr_w(id));

    hr = IOpcRelationshipSet_CreateRelationship(rels, id, typeW, target_uri, OPC_URI_TARGET_MODE_INTERNAL, &rel2);
    ok(hr == OPC_E_DUPLICATE_RELATIONSHIP, "Failed to create relationship, hr %#lx.\n", hr);

    hr = IOpcRelationshipSet_CreateRelationship(rels, id, typeW, target_uri2, OPC_URI_TARGET_MODE_INTERNAL, &rel2);
    ok(hr == OPC_E_DUPLICATE_RELATIONSHIP, "Failed to create relationship, hr %#lx.\n", hr);

    hr = IOpcRelationshipSet_CreateRelationship(rels, NULL, typeW, target_uri, OPC_URI_TARGET_MODE_INTERNAL, &rel2);
    ok(SUCCEEDED(hr), "Failed to create relationship, hr %#lx.\n", hr);

    ret = 123;
    hr = IOpcRelationshipSet_RelationshipExists(rels, NULL, &ret);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    ok(ret == 123, "Unexpected result %d.\n", ret);

    hr = IOpcRelationshipSet_RelationshipExists(rels, id, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    ret = FALSE;
    hr = IOpcRelationshipSet_RelationshipExists(rels, id, &ret);
    ok(SUCCEEDED(hr), "Failed to get relationship, hr %#lx.\n", hr);
    ok(ret, "Unexpected result %d.\n", ret);

    hr = IOpcRelationshipSet_GetRelationship(rels, id, &rel3);
    ok(SUCCEEDED(hr), "Failed to get relationship, hr %#lx.\n", hr);
    IOpcRelationship_Release(rel3);

    hr = IOpcRelationshipSet_GetRelationship(rels, id, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    rel3 = (void *)0xdeadbeef;
    hr = IOpcRelationshipSet_GetRelationship(rels, NULL, &rel3);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    ok(rel3 == NULL, "Expected null pointer.\n");

    *id = 'r';
    rel3 = (void *)0xdeadbeef;
    hr = IOpcRelationshipSet_GetRelationship(rels, id, &rel3);
    ok(hr == OPC_E_NO_SUCH_RELATIONSHIP, "Unexpected hr %#lx.\n", hr);
    ok(rel3 == NULL, "Expected null pointer.\n");

    ret = TRUE;
    hr = IOpcRelationshipSet_RelationshipExists(rels, id, &ret);
    ok(SUCCEEDED(hr), "Unexpected hr %#lx.\n", hr);
    ok(!ret, "Unexpected result %d.\n", ret);

    CoTaskMemFree(id);

    hr = IOpcRelationship_GetTargetUri(rel, &uri);
    ok(SUCCEEDED(hr), "Failed to get target uri, hr %#lx.\n", hr);
    ok(uri == target_uri, "Unexpected uri.\n");
    IUri_Release(uri);

    hr = IOpcRelationship_GetTargetMode(rel, &mode);
    ok(SUCCEEDED(hr), "Failed to get target mode, hr %#lx.\n", hr);
    ok(mode == OPC_URI_TARGET_MODE_INTERNAL, "Unexpected mode %d.\n", mode);

    /* Source uri */
    hr = IOpcFactory_CreatePackageRootUri(factory, &source_uri);
    ok(SUCCEEDED(hr), "Failed to create root uri, hr %#lx.\n", hr);

    hr = IOpcFactory_CreatePackageRootUri(factory, &source_uri2);
    ok(SUCCEEDED(hr), "Failed to create root uri, hr %#lx.\n", hr);
    ok(source_uri != source_uri2, "Unexpected uri instance.\n");

    IOpcUri_Release(source_uri);
    IOpcUri_Release(source_uri2);

    hr = IOpcRelationship_GetSourceUri(rel, &source_uri);
    ok(SUCCEEDED(hr), "Failed to get source uri, hr %#lx.\n", hr);

    hr = IOpcUri_QueryInterface(source_uri, &IID_IOpcPartUri, (void **)&unk);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);

    str = NULL;
    hr = IOpcUri_GetRawUri(source_uri, &str);
    ok(SUCCEEDED(hr), "Failed to get raw uri, hr %#lx.\n", hr);
    ok(!lstrcmpW(L"/", str), "Unexpected uri %s.\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hr = IOpcRelationship_GetSourceUri(rel2, &source_uri2);
    ok(SUCCEEDED(hr), "Failed to get source uri, hr %#lx.\n", hr);
    ok(source_uri2 == source_uri, "Unexpected source uri.\n");

    IOpcUri_Release(source_uri2);
    IOpcUri_Release(source_uri);

    IOpcRelationship_Release(rel2);
    IOpcRelationship_Release(rel);

    IOpcRelationshipSet_Release(rels);

    IUri_Release(target_uri);
    IUri_Release(target_uri2);
    IOpcPackage_Release(package);
    IOpcFactory_Release(factory);
}

static void test_rel_part_uri(void)
{
    static const struct
    {
        const WCHAR *uri;
        const WCHAR *rel_uri;
        HRESULT hr;
    } rel_part_uri_tests[] =
    {
        { L"/uri", L"/_rels/uri.rels" },
        { L"/path/uri", L"/path/_rels/uri.rels" },
        { L"path/uri", L"/path/_rels/uri.rels" },
        { L"../path/uri", L"/path/_rels/uri.rels" },
        { L"../../path/uri", L"/path/_rels/uri.rels" },
        { L"/uri.ext", L"/_rels/uri.ext.rels" },
        { L"/", L"/_rels/.rels" },
        { L"uri", L"/_rels/uri.rels" },
        { L"/path/../uri", L"/_rels/uri.rels" },
        { L"/path/path/../../uri", L"/_rels/uri.rels" },
        { L"/_rels/uri.ext.rels", L"", OPC_E_NONCONFORMING_URI },
    };
    static const struct
    {
        const WCHAR *uri;
        BOOL ret;
    } is_rel_part_tests[] =
    {
        { L"/uri", FALSE },
        { L"uri", FALSE },
        { L"/_rels/uri", FALSE },
        { L"/_rels/uri/uri", FALSE },
        { L"/_rels/uri/uri.rels", FALSE },
        { L"/uri/uri.rels", FALSE },
        { L"/uri/_rels/uri.rels", TRUE },
        { L"/_rels/.rels", TRUE },
    };
    IOpcPartUri *part_uri;
    IOpcFactory *factory;
    IOpcUri *source_uri;
    unsigned int i;
    HRESULT hr;

    factory = create_factory();

    hr = IOpcFactory_CreatePartUri(factory, L"/uri", &part_uri);
    ok(SUCCEEDED(hr), "Failed to create part uri, hr %#lx.\n", hr);

    hr = IOpcPartUri_GetRelationshipsPartUri(part_uri, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IOpcPartUri_IsRelationshipsPartUri(part_uri, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IOpcPartUri_GetSourceUri(part_uri, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    source_uri = (void *)0xdeadbeef;
    hr = IOpcPartUri_GetSourceUri(part_uri, &source_uri);
    ok(hr == OPC_E_RELATIONSHIP_URI_REQUIRED, "Unexpected hr %#lx.\n", hr);
    ok(source_uri == NULL, "Expected null uri.\n");

    IOpcPartUri_Release(part_uri);

    for (i = 0; i < ARRAY_SIZE(rel_part_uri_tests); ++i)
    {
        BOOL is_root = FALSE;
        IOpcPartUri *rel_uri;
        IOpcUri *part_uri;

        if (!wcscmp(rel_part_uri_tests[i].uri, L"/"))
        {
            hr = IOpcFactory_CreatePackageRootUri(factory, &part_uri);
            is_root = TRUE;
        }
        else
            hr = IOpcFactory_CreatePartUri(factory, rel_part_uri_tests[i].uri, (IOpcPartUri **)&part_uri);
        ok(SUCCEEDED(hr), "Failed to create part uri, hr %#lx.\n", hr);

        rel_uri = (void *)0xdeadbeef;
        hr = IOpcUri_GetRelationshipsPartUri(part_uri, &rel_uri);
        if (SUCCEEDED(hr))
        {
            IOpcPartUri *rel_uri2;
            IOpcUri *source_uri2;
            IUnknown *unk = NULL;
            BOOL ret;
            BSTR str;

            hr = IOpcPartUri_GetSourceUri(rel_uri, &source_uri);
            ok(SUCCEEDED(hr), "Failed to get source uri, hr %#lx.\n", hr);
            hr = IOpcPartUri_GetSourceUri(rel_uri, &source_uri2);
            ok(SUCCEEDED(hr), "Failed to get source uri, hr %#lx.\n", hr);
            ok(source_uri != source_uri2, "Unexpected instance.\n");

            hr = IOpcUri_IsEqual(source_uri, NULL, NULL);
            ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

            ret = 123;
            hr = IOpcUri_IsEqual(source_uri, NULL, &ret);
            ok(is_root ? hr == E_POINTER : hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(is_root ? ret == 123 : !ret, "Unexpected result.\n");

            ret = FALSE;
            hr = IOpcUri_IsEqual(source_uri, (IUri *)source_uri2, &ret);
            ok(SUCCEEDED(hr), "IsEqual failed, hr %#lx.\n", hr);
            ok(ret, "Expected equal uris.\n");

            hr = IOpcUri_QueryInterface(source_uri, &IID_IOpcPartUri, (void **)&unk);
            ok(hr == (is_root ? E_NOINTERFACE : S_OK), "Unexpected hr %#lx, %s.\n", hr, wine_dbgstr_w(rel_part_uri_tests[i].uri));
            if (unk)
                IUnknown_Release(unk);

            IOpcUri_Release(source_uri2);
            IOpcUri_Release(source_uri);

            hr = IOpcUri_GetRelationshipsPartUri(part_uri, &rel_uri2);
            ok(SUCCEEDED(hr), "Failed to get rels part uri, hr %#lx.\n", hr);
            ok(rel_uri2 != rel_uri, "Unexpected instance.\n");
            IOpcPartUri_Release(rel_uri2);

            hr = IOpcPartUri_GetRawUri(rel_uri, &str);
            ok(SUCCEEDED(hr), "Failed to get rel uri, hr %#lx.\n", hr);
            todo_wine_if(i == 3 || i == 4 || i == 8 || i == 9)
            ok(!lstrcmpW(str, rel_part_uri_tests[i].rel_uri), "%u: unexpected rel uri %s, expected %s.\n",
                    i, wine_dbgstr_w(str), wine_dbgstr_w(rel_part_uri_tests[i].rel_uri));
            SysFreeString(str);

            IOpcPartUri_Release(rel_uri);
        }
        else
        {
            ok(hr == rel_part_uri_tests[i].hr, "%u: unexpected hr %#lx.\n", i, hr);
            ok(rel_uri == NULL, "%u: unexpected out pointer.\n", i);
        }

        IOpcUri_Release(part_uri);
    }

    for (i = 0; i < ARRAY_SIZE(is_rel_part_tests); ++i)
    {
        IOpcPartUri *part_uri;
        BOOL ret;

        hr = IOpcFactory_CreatePartUri(factory, is_rel_part_tests[i].uri, &part_uri);
        ok(SUCCEEDED(hr), "Failed to create part uri, hr %#lx.\n", hr);

        ret = 123;
        hr = IOpcPartUri_IsRelationshipsPartUri(part_uri, &ret);
        ok(SUCCEEDED(hr), "Unexpected hr %#lx.\n", hr);
        ok(ret == is_rel_part_tests[i].ret, "%u: unexpected result %d.\n", i, ret);

        IOpcPartUri_Release(part_uri);
    }

    IOpcFactory_Release(factory);
}

static void test_part_enumerator(void)
{
    IOpcPartEnumerator *partenum, *partenum2;
    IOpcPart *part, *part2;
    IOpcPartUri *part_uri;
    IOpcPackage *package;
    IOpcFactory *factory;
    IOpcPartSet *parts;
    HRESULT hr;
    BOOL ret;

    factory = create_factory();

    hr = IOpcFactory_CreatePackage(factory, &package);
    ok(hr == S_OK, "Failed to create a package, hr %#lx.\n", hr);

    hr = IOpcPackage_GetPartSet(package, &parts);
    ok(SUCCEEDED(hr), "Failed to get part set, hr %#lx.\n", hr);

    hr = IOpcPartSet_GetEnumerator(parts, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IOpcPartSet_GetEnumerator(parts, &partenum);
    ok(SUCCEEDED(hr), "Failed to get enumerator, hr %#lx.\n", hr);

    hr = IOpcPartSet_GetEnumerator(parts, &partenum2);
    ok(SUCCEEDED(hr), "Failed to get enumerator, hr %#lx.\n", hr);
    ok(partenum != partenum2, "Unexpected instance.\n");
    IOpcPartEnumerator_Release(partenum2);

    hr = IOpcPartEnumerator_GetCurrent(partenum, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IOpcPartEnumerator_GetCurrent(partenum, &part);
    ok(hr == OPC_E_ENUM_INVALID_POSITION, "Unexpected hr %#lx.\n", hr);

    hr = IOpcPartEnumerator_MoveNext(partenum, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    ret = TRUE;
    hr = IOpcPartEnumerator_MoveNext(partenum, &ret);
    ok(hr == S_OK, "Failed to move, hr %#lx.\n", hr);
    ok(!ret, "Unexpected result %d.\n", ret);

    ret = TRUE;
    hr = IOpcPartEnumerator_MovePrevious(partenum, &ret);
    ok(hr == S_OK, "Failed to move, hr %#lx.\n", hr);
    ok(!ret, "Unexpected result %d.\n", ret);

    hr = IOpcFactory_CreatePartUri(factory, L"/uri", &part_uri);
    ok(SUCCEEDED(hr), "Failed to create part uri, hr %#lx.\n", hr);

    hr = IOpcPartSet_CreatePart(parts, part_uri, L"type/subtype", OPC_COMPRESSION_NONE, &part);
    ok(SUCCEEDED(hr), "Failed to create a part, hr %#lx.\n", hr);
    IOpcPartUri_Release(part_uri);

    part2 = (void *)0xdeadbeef;
    hr = IOpcPartEnumerator_GetCurrent(partenum, &part2);
    ok(hr == OPC_E_ENUM_COLLECTION_CHANGED, "Unexpected hr %#lx.\n", hr);
    ok(part2 == NULL, "Unexpected instance.\n");

    hr = IOpcPartEnumerator_MoveNext(partenum, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    ret = 123;
    hr = IOpcPartEnumerator_MoveNext(partenum, &ret);
    ok(hr == OPC_E_ENUM_COLLECTION_CHANGED, "Unexpected hr %#lx.\n", hr);
    ok(ret == 123, "Unexpected result %d.\n", ret);

    hr = IOpcPartEnumerator_MovePrevious(partenum, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    ret = 123;
    hr = IOpcPartEnumerator_MovePrevious(partenum, &ret);
    ok(hr == OPC_E_ENUM_COLLECTION_CHANGED, "Unexpected hr %#lx.\n", hr);
    ok(ret == 123, "Unexpected result %d.\n", ret);

    hr = IOpcPartEnumerator_Clone(partenum, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    partenum2 = (void *)0xdeadbeef;
    hr = IOpcPartEnumerator_Clone(partenum, &partenum2);
    ok(hr == OPC_E_ENUM_COLLECTION_CHANGED, "Unexpected hr %#lx.\n", hr);
    ok(partenum2 == NULL, "Unexpected instance.\n");

    IOpcPartEnumerator_Release(partenum);

    hr = IOpcPartSet_GetEnumerator(parts, &partenum);
    ok(SUCCEEDED(hr), "Failed to get enumerator, hr %#lx.\n", hr);

    part2 = (void *)0xdeadbeef;
    hr = IOpcPartEnumerator_GetCurrent(partenum, &part2);
    ok(hr == OPC_E_ENUM_INVALID_POSITION, "Unexpected hr %#lx.\n", hr);
    ok(part2 == NULL, "Unexpected instance.\n");

    hr = IOpcPartEnumerator_MoveNext(partenum, &ret);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(ret, "Unexpected result %d.\n", ret);

    hr = IOpcPartEnumerator_GetCurrent(partenum, &part2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(part2 == part, "Unexpected instance.\n");
    IOpcPart_Release(part2);

    hr = IOpcPartEnumerator_MoveNext(partenum, &ret);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!ret, "Unexpected result %d.\n", ret);

    part2 = (void *)0xdeadbeef;
    hr = IOpcPartEnumerator_GetCurrent(partenum, &part2);
    ok(hr == OPC_E_ENUM_INVALID_POSITION, "Unexpected hr %#lx.\n", hr);
    ok(part2 == NULL, "Unexpected instance.\n");

    hr = IOpcPartEnumerator_MovePrevious(partenum, &ret);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(ret, "Unexpected result %d.\n", ret);

    hr = IOpcPartEnumerator_GetCurrent(partenum, &part2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(part2 == part, "Unexpected instance.\n");
    IOpcPart_Release(part2);

    hr = IOpcPartEnumerator_MovePrevious(partenum, &ret);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!ret, "Unexpected result %d.\n", ret);

    hr = IOpcPartEnumerator_GetCurrent(partenum, &part2);
    ok(hr == OPC_E_ENUM_INVALID_POSITION, "Unexpected hr %#lx.\n", hr);

    hr = IOpcPartEnumerator_Clone(partenum, &partenum2);
    ok(SUCCEEDED(hr), "Clone failed, hr %#lx.\n", hr);

    hr = IOpcPartEnumerator_MoveNext(partenum2, &ret);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(ret, "Unexpected result %d.\n", ret);

    hr = IOpcPartEnumerator_GetCurrent(partenum2, &part2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IOpcPart_Release(part2);

    hr = IOpcPartEnumerator_GetCurrent(partenum, &part2);
    ok(hr == OPC_E_ENUM_INVALID_POSITION, "Unexpected hr %#lx.\n", hr);

    IOpcPartEnumerator_Release(partenum2);

    IOpcPartEnumerator_Release(partenum);

    IOpcPart_Release(part);

    IOpcPartSet_Release(parts);

    IOpcPackage_Release(package);
    IOpcFactory_Release(factory);
}

static void test_rels_enumerator(void)
{
    IOpcRelationshipEnumerator *relsenum, *relsenum2;
    IOpcRelationship *rel, *rel2;
    IOpcPackage *package;
    IOpcFactory *factory;
    IOpcRelationshipSet *rels;
    IUri *target_uri;
    HRESULT hr;
    BOOL ret;

    factory = create_factory();

    hr = IOpcFactory_CreatePackage(factory, &package);
    ok(hr == S_OK, "Failed to create a package, hr %#lx.\n", hr);

    hr = IOpcPackage_GetRelationshipSet(package, &rels);
    ok(SUCCEEDED(hr), "Failed to get part set, hr %#lx.\n", hr);

    hr = IOpcRelationshipSet_GetEnumerator(rels, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IOpcRelationshipSet_GetEnumerator(rels, &relsenum);
    ok(SUCCEEDED(hr), "Failed to get enumerator, hr %#lx.\n", hr);

    hr = IOpcRelationshipSet_GetEnumerator(rels, &relsenum2);
    ok(SUCCEEDED(hr), "Failed to get enumerator, hr %#lx.\n", hr);
    ok(relsenum != relsenum2, "Unexpected instance.\n");
    IOpcRelationshipEnumerator_Release(relsenum2);

    hr = IOpcRelationshipEnumerator_GetCurrent(relsenum, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IOpcRelationshipEnumerator_GetCurrent(relsenum, &rel);
    ok(hr == OPC_E_ENUM_INVALID_POSITION, "Unexpected hr %#lx.\n", hr);

    hr = IOpcRelationshipEnumerator_MoveNext(relsenum, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    ret = TRUE;
    hr = IOpcRelationshipEnumerator_MoveNext(relsenum, &ret);
    ok(hr == S_OK, "Failed to move, hr %#lx.\n", hr);
    ok(!ret, "Unexpected result %d.\n", ret);

    ret = TRUE;
    hr = IOpcRelationshipEnumerator_MovePrevious(relsenum, &ret);
    ok(hr == S_OK, "Failed to move, hr %#lx.\n", hr);
    ok(!ret, "Unexpected result %d.\n", ret);

    hr = CreateUri(L"target", Uri_CREATE_ALLOW_RELATIVE, 0, &target_uri);
    ok(SUCCEEDED(hr), "Failed to create target uri, hr %#lx.\n", hr);

    hr = IOpcRelationshipSet_CreateRelationship(rels, NULL, L"type/subtype", target_uri, OPC_URI_TARGET_MODE_INTERNAL, &rel);
    ok(SUCCEEDED(hr), "Failed to create relationship, hr %#lx.\n", hr);

    IUri_Release(target_uri);

    rel2 = (void *)0xdeadbeef;
    hr = IOpcRelationshipEnumerator_GetCurrent(relsenum, &rel2);
    ok(hr == OPC_E_ENUM_COLLECTION_CHANGED, "Unexpected hr %#lx.\n", hr);
    ok(rel2 == NULL, "Unexpected instance.\n");

    hr = IOpcRelationshipEnumerator_MoveNext(relsenum, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    ret = 123;
    hr = IOpcRelationshipEnumerator_MoveNext(relsenum, &ret);
    ok(hr == OPC_E_ENUM_COLLECTION_CHANGED, "Unexpected hr %#lx.\n", hr);
    ok(ret == 123, "Unexpected result %d.\n", ret);

    hr = IOpcRelationshipEnumerator_MovePrevious(relsenum, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    ret = 123;
    hr = IOpcRelationshipEnumerator_MovePrevious(relsenum, &ret);
    ok(hr == OPC_E_ENUM_COLLECTION_CHANGED, "Unexpected hr %#lx.\n", hr);
    ok(ret == 123, "Unexpected result %d.\n", ret);

    hr = IOpcRelationshipEnumerator_Clone(relsenum, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    relsenum2 = (void *)0xdeadbeef;
    hr = IOpcRelationshipEnumerator_Clone(relsenum, &relsenum2);
    ok(hr == OPC_E_ENUM_COLLECTION_CHANGED, "Unexpected hr %#lx.\n", hr);
    ok(relsenum2 == NULL, "Unexpected instance.\n");

    IOpcRelationshipEnumerator_Release(relsenum);

    hr = IOpcRelationshipSet_GetEnumerator(rels, &relsenum);
    ok(SUCCEEDED(hr), "Failed to get enumerator, hr %#lx.\n", hr);

    rel2 = (void *)0xdeadbeef;
    hr = IOpcRelationshipEnumerator_GetCurrent(relsenum, &rel2);
    ok(hr == OPC_E_ENUM_INVALID_POSITION, "Unexpected hr %#lx.\n", hr);
    ok(rel2 == NULL, "Unexpected instance.\n");

    hr = IOpcRelationshipEnumerator_MoveNext(relsenum, &ret);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(ret, "Unexpected result %d.\n", ret);

    hr = IOpcRelationshipEnumerator_GetCurrent(relsenum, &rel2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(rel2 == rel, "Unexpected instance.\n");
    IOpcRelationship_Release(rel2);

    hr = IOpcRelationshipEnumerator_MoveNext(relsenum, &ret);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!ret, "Unexpected result %d.\n", ret);

    rel2 = (void *)0xdeadbeef;
    hr = IOpcRelationshipEnumerator_GetCurrent(relsenum, &rel2);
    ok(hr == OPC_E_ENUM_INVALID_POSITION, "Unexpected hr %#lx.\n", hr);
    ok(rel2 == NULL, "Unexpected instance.\n");

    hr = IOpcRelationshipEnumerator_MovePrevious(relsenum, &ret);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(ret, "Unexpected result %d.\n", ret);

    hr = IOpcRelationshipEnumerator_GetCurrent(relsenum, &rel2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(rel2 == rel, "Unexpected instance.\n");
    IOpcRelationship_Release(rel2);

    hr = IOpcRelationshipEnumerator_MovePrevious(relsenum, &ret);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!ret, "Unexpected result %d.\n", ret);

    hr = IOpcRelationshipEnumerator_GetCurrent(relsenum, &rel2);
    ok(hr == OPC_E_ENUM_INVALID_POSITION, "Unexpected hr %#lx.\n", hr);

    hr = IOpcRelationshipEnumerator_Clone(relsenum, &relsenum2);
    ok(SUCCEEDED(hr), "Clone failed, hr %#lx.\n", hr);

    hr = IOpcRelationshipEnumerator_MoveNext(relsenum2, &ret);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(ret, "Unexpected result %d.\n", ret);

    hr = IOpcRelationshipEnumerator_GetCurrent(relsenum2, &rel2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IOpcRelationship_Release(rel2);

    hr = IOpcRelationshipEnumerator_GetCurrent(relsenum, &rel2);
    ok(hr == OPC_E_ENUM_INVALID_POSITION, "Unexpected hr %#lx.\n", hr);

    IOpcRelationshipEnumerator_Release(relsenum2);

    IOpcRelationshipEnumerator_Release(relsenum);

    IOpcRelationship_Release(rel);

    IOpcRelationshipSet_Release(rels);

    IOpcPackage_Release(package);
    IOpcFactory_Release(factory);
}

static void test_relative_uri(void)
{
    static const struct
    {
        const WCHAR *part;
        const WCHAR *combined;
        const WCHAR *relative;
        const WCHAR *relative_broken;
    }
    relative_uri_tests[] =
    {
        { L"/", L"/path/path2", L"path/path2", L"/path/path2" },
        { L"/", L"/path", L"path", L"/path" },
        { L"/path/path2", L"/path/path2/path3", L"path2/path3" },
        { L"/path/path2", L"/path3", L"../path3" },
        { L"/path", L"/path", L"" },
        { L"/path", L"../path", L"" },
        { L"/path2", L"/path", L"path" },
        { L"../path", L"/path", L"" },
        { L"../../path", L"/path", L"" },
    };
    IOpcFactory *factory;
    unsigned int i;

    factory = create_factory();

    for (i = 0; i < ARRAY_SIZE(relative_uri_tests); ++i)
    {
        const WCHAR *relative_broken;
        IOpcPartUri *combined_uri;
        IUri *relative_uri;
        IOpcUri *part_uri;
        IUnknown *unk;
        HRESULT hr;
        BSTR str;

        relative_broken = relative_uri_tests[i].relative_broken;

        if (!wcscmp(relative_uri_tests[i].part, L"/"))
            hr = IOpcFactory_CreatePackageRootUri(factory, &part_uri);
        else
            hr = IOpcFactory_CreatePartUri(factory, relative_uri_tests[i].part, (IOpcPartUri **)&part_uri);
        ok(SUCCEEDED(hr), "%u: failed to create part uri, hr %#lx.\n", i, hr);

        hr = IOpcFactory_CreatePartUri(factory, relative_uri_tests[i].combined, &combined_uri);
        ok(SUCCEEDED(hr), "%u: failed to create part uri, hr %#lx.\n", i, hr);

        hr = IOpcUri_GetRelativeUri(part_uri, combined_uri, &relative_uri);
    todo_wine
        ok(SUCCEEDED(hr), "%u: failed t oget relative uri, hr %#lx.\n", i, hr);

    if (SUCCEEDED(hr))
    {
        hr = IUri_QueryInterface(relative_uri, &IID_IOpcUri, (void **)&unk);
        ok(hr == E_NOINTERFACE, "%u: unexpected hr %#lx.\n", i, hr);

        hr = IUri_GetRawUri(relative_uri, &str);
        ok(SUCCEEDED(hr), "%u: failed to get raw uri, hr %#lx.\n", i, hr);
        ok(!lstrcmpW(str, relative_uri_tests[i].relative) || broken(relative_broken && !lstrcmpW(str, relative_broken)),
                "%u: unexpected relative uri %s.\n", i, wine_dbgstr_w(str));
        SysFreeString(str);

        IUri_Release(relative_uri);
    }
        IOpcUri_Release(part_uri);
        IOpcPartUri_Release(combined_uri);
    }

    IOpcFactory_Release(factory);
}

static void test_combine_uri(void)
{
    static const struct
    {
        const WCHAR *uri;
        const WCHAR *relative;
        const WCHAR *combined;
    }
    combine_tests[] =
    {
        { L"/", L"path", L"/path" },
        { L"/path1", L"path2", L"/path2" },
        { L"/path1", L"../path2", L"/path2" },
        { L"/path1/../path2", L"path3", L"/path3" },
    };
    IOpcFactory *factory;
    unsigned int i;

    factory = create_factory();

    for (i = 0; i < ARRAY_SIZE(combine_tests); ++i)
    {
        IOpcPartUri *combined_uri;
        IUri *relative_uri;
        IOpcUri *uri;
        HRESULT hr;
        BSTR str;

        if (!wcscmp(combine_tests[i].uri, L"/"))
            hr = IOpcFactory_CreatePackageRootUri(factory, &uri);
        else
            hr = IOpcFactory_CreatePartUri(factory, combine_tests[i].uri, (IOpcPartUri **)&uri);
        ok(SUCCEEDED(hr), "%u: failed to create uri, hr %#lx.\n", i, hr);

        hr = CreateUri(combine_tests[i].relative, Uri_CREATE_ALLOW_RELATIVE, 0, &relative_uri);
        ok(SUCCEEDED(hr), "%u: failed to create relative uri, hr %#lx.\n", i, hr);

        combined_uri = (void *)0xdeadbeef;
        hr = IOpcUri_CombinePartUri(uri, NULL, &combined_uri);
        ok(hr == E_POINTER, "%u: failed to combine uris, hr %#lx.\n", i, hr);
        ok(!combined_uri, "Unexpected instance.\n");

        hr = IOpcUri_CombinePartUri(uri, relative_uri, NULL);
        ok(hr == E_POINTER, "%u: failed to combine uris, hr %#lx.\n", i, hr);

        hr = IOpcUri_CombinePartUri(uri, relative_uri, &combined_uri);
        ok(SUCCEEDED(hr), "%u: failed to combine uris, hr %#lx.\n", i, hr);

        hr = IOpcPartUri_GetRawUri(combined_uri, &str);
        ok(SUCCEEDED(hr), "%u: failed to get raw uri, hr %#lx.\n", i, hr);
        todo_wine_if(i == 2 || i == 3)
        ok(!lstrcmpW(str, combine_tests[i].combined), "%u: unexpected uri %s.\n", i, wine_dbgstr_w(str));
        SysFreeString(str);

        IOpcPartUri_Release(combined_uri);

        IOpcUri_Release(uri);
        IUri_Release(relative_uri);
    }

    IOpcFactory_Release(factory);
}

static void test_create_part_uri(void)
{
    static const struct
    {
        const WCHAR *input;
        const WCHAR *raw_uri;
    }
    create_part_uri_tests[] =
    {
        { L"path", L"/path" },
        { L"../path", L"/path" },
        { L"../../path", L"/path" },
        { L"/path", L"/path" },
        { L"/path1/path2/path3/../path4", L"/path1/path2/path4" },
    };
    IOpcFactory *factory;
    unsigned int i;
    HRESULT hr;

    factory = create_factory();

    for (i = 0; i < ARRAY_SIZE(create_part_uri_tests); ++i)
    {
        const WCHAR *raw_uri = create_part_uri_tests[i].raw_uri;
        IOpcPartUri *part_uri;
        IUri *uri;
        BSTR str;
        BOOL ret;

        hr = IOpcFactory_CreatePartUri(factory, create_part_uri_tests[i].input, &part_uri);
        ok(SUCCEEDED(hr), "%u: failed to create part uri, hr %#lx.\n", i, hr);

        hr = IOpcPartUri_GetRawUri(part_uri, &str);
        ok(SUCCEEDED(hr), "Failed to get raw uri, hr %#lx.\n", hr);
        todo_wine_if(i == 1 || i == 2 || i == 4)
        ok(!lstrcmpW(str, raw_uri), "%u: unexpected raw uri %s.\n", i, wine_dbgstr_w(str));
        SysFreeString(str);

        hr = CreateUri(raw_uri, Uri_CREATE_ALLOW_RELATIVE, 0, &uri);
        ok(SUCCEEDED(hr), "Failed to create uri, hr %#lx.\n", hr);

        ret = FALSE;
        hr = IOpcPartUri_IsEqual(part_uri, uri, &ret);
        ok(SUCCEEDED(hr), "IsEqual failed, hr %#lx.\n", hr);
        todo_wine_if(i == 1 || i == 2 || i == 4)
        ok(!!ret, "%u: unexpected result %d.\n", i, ret);

        IOpcPartUri_Release(part_uri);
        IUri_Release(uri);
    }

    IOpcFactory_Release(factory);
}

static HRESULT WINAPI custom_package_QueryInterface(IOpcPackage *iface, REFIID iid, void **out)
{
    if (IsEqualIID(iid, &IID_IOpcPackage) || IsEqualIID(iid, &IID_IUnknown))
    {
        *out = iface;
        IOpcPackage_AddRef(iface);
        return S_OK;
    }

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI custom_package_AddRef(IOpcPackage *iface)
{
    return 2;
}

static ULONG WINAPI custom_package_Release(IOpcPackage *iface)
{
    return 1;
}

static HRESULT WINAPI custom_package_GetPartSet(IOpcPackage *iface, IOpcPartSet **part_set)
{
    return 0x80000001;
}

static HRESULT WINAPI custom_package_GetRelationshipSet(IOpcPackage *iface, IOpcRelationshipSet **relationship_set)
{
    return 0x80000001;
}

static const IOpcPackageVtbl custom_package_vtbl =
{
    custom_package_QueryInterface,
    custom_package_AddRef,
    custom_package_Release,
    custom_package_GetPartSet,
    custom_package_GetRelationshipSet,
};

static void test_write_package(void)
{
    IOpcPackage custom_package = { &custom_package_vtbl };
    IOpcFactory *factory;
    IOpcPackage *package;
    IStream *stream;
    HRESULT hr;

    factory = create_factory();

    hr = IOpcFactory_CreatePackage(factory, &package);
    ok(hr == S_OK, "Failed to create a package, hr %#lx.\n", hr);

    hr = IOpcFactory_WritePackageToStream(factory, NULL, OPC_WRITE_FORCE_ZIP32, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(SUCCEEDED(hr), "Failed to create a stream, hr %#lx.\n", hr);

    hr = IOpcFactory_WritePackageToStream(factory, NULL, OPC_WRITE_FORCE_ZIP32, stream);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IOpcFactory_WritePackageToStream(factory, &custom_package, OPC_WRITE_FORCE_ZIP32, stream);
    ok(hr == 0x80000001, "Unexpected hr %#lx.\n", hr);

    IStream_Release(stream);

    IOpcFactory_Release(factory);
    IOpcPackage_Release(package);
}

static void test_read_package(void)
{
    static const struct
    {
        const WCHAR *type;
        const WCHAR *uri;
        OPC_COMPRESSION_OPTIONS options;
    }
    parts[] =
    {
        { L"type1/subtype1", L"/entry1.11", OPC_COMPRESSION_NONE },
        { L"type1/subtype2", L"/entry2.12", OPC_COMPRESSION_NORMAL },
        { L"type2/subtype1", L"/entry3.21", OPC_COMPRESSION_MAXIMUM },
        { L"type2/subtype2", L"/entry4.22", OPC_COMPRESSION_FAST },
        { L"type1/subtype1", L"/entry5.11", OPC_COMPRESSION_SUPERFAST },
        { L"type1/subtype2", L"/entry6.12", OPC_COMPRESSION_NONE },
        { L"type2/subtype1", L"/entry7.21", OPC_COMPRESSION_NORMAL },
        { L"type2/subtype2", L"/entry8.22", OPC_COMPRESSION_MAXIMUM },
        { L"type1/subtype1", L"/dir1/entry1.11", OPC_COMPRESSION_FAST },
    };
    const LARGE_INTEGER start = {0};
    IOpcFactory *factory;
    IOpcPackage *package;
    IOpcPartSet *partset;
    IOpcPartUri *uri;
    IStream *stream;
    IOpcPart *part;
    BYTE buf[256];
    HRESULT hr;
    int i;

    factory = create_factory();

    hr = IOpcFactory_CreatePackage(factory, &package);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IOpcPackage_GetPartSet(package, &partset);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(parts); ++i)
    {
        hr = IOpcFactory_CreatePartUri(factory, parts[i].uri, &uri);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IOpcPartSet_CreatePart(partset, uri, parts[i].type, parts[i].options, &part);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IOpcPart_GetContentStream(part, &stream);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        memset(buf, i, sizeof(buf));
        hr = IStream_Write(stream, buf, sizeof(buf), NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        IStream_Release(stream);

        IOpcPartUri_Release(uri);
        IOpcPart_Release(part);
    }
    IOpcPartSet_Release(partset);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IOpcFactory_WritePackageToStream(factory, package, OPC_WRITE_FORCE_ZIP32, stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IOpcPackage_Release(package);
    hr = IStream_Seek(stream, start, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IOpcFactory_ReadPackageFromStream(factory, stream, OPC_READ_DEFAULT, &package);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IStream_Release(stream);
    if (FAILED(hr)) goto done;

    hr = IOpcPackage_GetPartSet(package, &partset);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(parts); ++i)
    {
        OPC_COMPRESSION_OPTIONS options;
        WCHAR *type = NULL;
        BYTE exp_buf[256];
        ULONG read = 0;

        winetest_push_context("parts[%d]", i);

        hr = IOpcFactory_CreatePartUri(factory, parts[i].uri, &uri);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IOpcPartSet_GetPart(partset, uri, &part);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IOpcPart_GetContentType(part, &type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        todo_wine ok(!wcscmp(type, parts[i].type), "Unexpected type %s != %s.\n", debugstr_w(type), debugstr_w(parts[i].type));
        CoTaskMemFree(type);

        options = ~0u;
        hr = IOpcPart_GetCompressionOptions(part, &options);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(options == parts[i].options, "Unexpected options %d != %d.\n", options, parts[i].options);

        hr = IOpcPart_GetContentStream(part, &stream);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        memset(buf, 0, sizeof(buf));
        memset(exp_buf, i, sizeof(exp_buf));
        hr = IStream_Read(stream, buf, sizeof(buf), &read);
        ok(hr == S_OK, "Failed to read from stream, hr %#lx.\n", hr);
        ok(read == sizeof(buf), "Got read %lu != %Iu.\n", read, sizeof(buf));
        ok(!memcmp(buf, exp_buf, read), "Got mismatching data.\n");
        IStream_Release(stream);

        IOpcPart_Release(part);
        IOpcPartUri_Release(uri);

        winetest_pop_context();
    }

    IOpcPartSet_Release(partset);
    IOpcPackage_Release(package);
done:
    IOpcFactory_Release(factory);
}

START_TEST(opcservices)
{
    IOpcFactory *factory;
    HRESULT hr;

    hr = CoInitialize(NULL);
    ok(SUCCEEDED(hr), "Failed to initialize COM, hr %#lx.\n", hr);

    if (!(factory = create_factory())) {
        win_skip("Failed to create IOpcFactory factory.\n");
        CoUninitialize();
        return;
    }

    test_package();
    test_file_stream();
    test_relationship();
    test_rel_part_uri();
    test_part_enumerator();
    test_rels_enumerator();
    test_relative_uri();
    test_combine_uri();
    test_create_part_uri();
    test_write_package();
    test_read_package();

    IOpcFactory_Release(factory);

    CoUninitialize();
}
