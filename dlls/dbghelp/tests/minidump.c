/*
 * Copyright 2024 Eric Pouech for CodeWeavers
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

#include <assert.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windows.h"
#include "dbghelp.h"
#include "wine/test.h"
#include "winternl.h"
#include "winnt.h"
#include "wine/test.h"

static unsigned popcount32(ULONG val)
{
    val -= val >> 1 & 0x55555555;
    val = (val & 0x33333333) + (val >> 2 & 0x33333333);
    return ((val + (val >> 4)) & 0x0f0f0f0f) * 0x01010101 >> 24;
}

static inline unsigned popcount64(ULONG64 val)
{
    return popcount32(val >> 32) + popcount32(val);
}

#define minidump_open_for_read(a) _minidump_open_for_read(__LINE__, (a))
static MINIDUMP_HEADER *_minidump_open_for_read(unsigned line, const char *filename)
{
    MINIDUMP_HEADER *hdr;
    HANDLE file, map;
    BOOL ret;

    file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ok_(__FILE__, line)(file != INVALID_HANDLE_VALUE, "Couldn't reopen file\n");
    map = CreateFileMappingA(file, NULL, PAGE_READONLY, 0, 0, NULL);
    ok_(__FILE__, line)(map != 0, "Couldn't map file\n");
    hdr = MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
    ok(hdr != NULL, "Couldn't map file\n");

    ok(hdr && hdr->Signature == MINIDUMP_SIGNATURE, "Unexpected signature\n");

    ret = CloseHandle(map);
    ok_(__FILE__, line)(ret, "Couldn't unmap file\n");
    ret = CloseHandle(file);
    ok_(__FILE__, line)(ret, "Couldn't close file\n");

    return hdr;
}

#define minidump_close_for_read(a) _minidump_close_for_read(__LINE__, (a))
static void _minidump_close_for_read(unsigned line, MINIDUMP_HEADER *data)
{
    BOOL ret;

    ret = UnmapViewOfFile(data);
    ok_(__FILE__, line)(ret, "Couldn't unmap file\n");
}

static BOOL minidump_write(HANDLE proc, const WCHAR *filename, MINIDUMP_TYPE type, BOOL windows_can_fail)
{
    HANDLE              file;
    BOOL                ret, ret2;

    file = CreateFileW(filename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL);

    ok(file != INVALID_HANDLE_VALUE, "Failed to create minidump %ls\n", filename);

    ret = MiniDumpWriteDump(proc, GetProcessId(proc), file, type, NULL, NULL, NULL);
    /* using new features that are not supported on old dbghelp versions */
    ok(ret || broken(windows_can_fail), "Couldn't write minidump content\n");

    ret2 = CloseHandle(file);
    ok(ret2, "Couldn't close file\n");

    return ret;
}

typedef DWORD64 stream_mask_t;
#define STREAM2MASK(st) (((stream_mask_t)1) << (st))

#define BASIC_STREAM_MASK \
    (STREAM2MASK(ThreadListStream) | STREAM2MASK(ModuleListStream) | STREAM2MASK(MemoryListStream) | \
     STREAM2MASK(SystemInfoStream) | STREAM2MASK(MiscInfoStream) | STREAM2MASK(SystemMemoryInfoStream) | \
     STREAM2MASK(ProcessVmCountersStream))
/* streams added in Win8 & Win10... */
#define BASIC_STREAM_BROKEN_MASK (STREAM2MASK(SystemMemoryInfoStream) | STREAM2MASK(ProcessVmCountersStream))
#define BASIC_STREAM_TODO_MASK (STREAM2MASK(SystemMemoryInfoStream) | STREAM2MASK(ProcessVmCountersStream))
static void test_minidump_contents(void)
{
    static const struct minidump_streams
    {
        MINIDUMP_TYPE type;
        stream_mask_t streams_mask;
        stream_mask_t todo_wine_mask;
    }
    streams_table[] =
    {
/* 0 */ {MiniDumpNormal,                BASIC_STREAM_MASK, BASIC_STREAM_TODO_MASK},
        {MiniDumpWithDataSegs,          BASIC_STREAM_MASK, BASIC_STREAM_TODO_MASK},
        {MiniDumpWithProcessThreadData, BASIC_STREAM_MASK, BASIC_STREAM_TODO_MASK},
        {MiniDumpWithThreadInfo,        BASIC_STREAM_MASK | STREAM2MASK(ThreadInfoListStream), BASIC_STREAM_TODO_MASK | STREAM2MASK(ThreadInfoListStream)},
        {MiniDumpWithCodeSegs,          BASIC_STREAM_MASK, BASIC_STREAM_TODO_MASK},
/* 5 */ {MiniDumpWithTokenInformation,  BASIC_STREAM_MASK | STREAM2MASK(TokenStream), BASIC_STREAM_TODO_MASK | STREAM2MASK(TokenStream)},
        {MiniDumpWithModuleHeaders,     BASIC_STREAM_MASK, BASIC_STREAM_TODO_MASK}, /* requires win8 at least */
        {MiniDumpWithAvxXStateContext,  BASIC_STREAM_MASK, BASIC_STREAM_TODO_MASK}, /* requires win10 at least */
        {MiniDumpWithIptTrace,          BASIC_STREAM_MASK, BASIC_STREAM_TODO_MASK},
    };
    MINIDUMP_HEADER *hdr;
    void *where;
    ULONG size;
    BOOL ret;
    int i, j;

    for (i = 0; i < ARRAY_SIZE(streams_table); i++)
    {
        winetest_push_context("streams_table[%d]", i);
        if (minidump_write(GetCurrentProcess(), L"foo.mdmp", streams_table[i].type, i >= 6))
        {
            hdr = minidump_open_for_read("foo.mdmp");
            /* native keeps (likely padding) some unused streams at the end of directory, but lists them here */
            ok(hdr->NumberOfStreams >= popcount64(streams_table[i].streams_mask), "Unexpected number of streams %u <> %u\n",
               hdr->NumberOfStreams, popcount64(streams_table[i].streams_mask));
            ok(hdr->Flags == streams_table[i].type, "Unexpected flags\n");
            /* start at 3, 0=unused (cf above), 1,2=reserved shall be skipped */
            for (j = 3; j < 25 /* last documented stream */; j++)
            {
                ret = MiniDumpReadDumpStream(hdr, j, NULL, &where, &size);
                todo_wine_if(streams_table[i].todo_wine_mask & STREAM2MASK(j))
                if (streams_table[i].streams_mask & STREAM2MASK(j))
                    ok((ret && where) || broken(BASIC_STREAM_BROKEN_MASK & STREAM2MASK(j)), "Expecting stream %d to be present\n", j);
                else
                    ok(!ret, "Not expecting stream %d to be present\n", j);
            }

            minidump_close_for_read(hdr);

            ret = DeleteFileA("foo.mdmp");
            ok(ret, "Couldn't delete file\n");
        }
        winetest_pop_context();
    }
}

START_TEST(minidump)
{
    test_minidump_contents();
}
